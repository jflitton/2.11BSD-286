/*
 * The Adaptec AHA-1540 and 1542 SCSI host adapters, B, C, CF and CP:
 * the adapter itself.  sd.c is the disks on it.
 *
 * One adapter, found at whichever of its six possible addresses it
 * answers at; the interrupt request and DMA channel are read from it.
 * It moves data to and from memory itself, as a master of the AT bus,
 * so it is given physical addresses and nothing is copied: that is why
 * its mailboxes and CCB, which are in the kernel's data segment, are
 * handed to it through fartophys.
 *
 * One SCSI command is out at a time, in the one CCB: a driver fills it
 * with ahafill, or with ahaseg and ahafillsegs when its data is in
 * pieces, and the command's bytes, and gives it to ahago or ahapoll.
 */
#include "sd.h"
#if NSD > 0
#include "param.h"
#include "systm.h"
#include "kernel.h"
#include "errno.h"
#include "ahareg.h"

static int ahaports[] = { 0x330, 0x334, 0x230, 0x234, 0x130, 0x134 };
#define	NPORTS	(sizeof (ahaports) / sizeof (ahaports[0]))

#define	SPIN	300000L		/* looks at a port: a second or so */

/*
 * Seconds a command may be out before the adapter and the SCSI bus are
 * reset.  (patchable)
 */
int	ahatimeout = 30;

/*
 * For testing what is otherwise seen only when hardware fails, a sum
 * of: 1, the 20th interrupt is ignored as if it had not arrived; 2, the
 * 30th command is never given to the adapter; 4, the first command for
 * several of a disk's transfers at once is taken to have failed each
 * time it is tried (sdintr); 8, a command's end is not looked at until
 * the clock's next tick, as if the disk took that long, so that
 * transfers gather in the queue behind the one that is out, as they do
 * on a real disk and not on an emulator's.  (patchable)
 */
int	ahatest = 0;

struct ccb ahaccb;
static struct mbox ahamb[2 * AHA_NMBOX];	/* out, then in */
static struct ahaseg ahasegs[AHA_NSEG];	/* the CCB's data, when in pieces */

static struct {
	int	a_base;
	int	a_irq;
	int	a_dma;
	int	a_id;		/* the adapter's own on the SCSI bus */
	char	a_state;
	char	a_busy;		/* the CCB is out */
	char	a_mbo, a_mbi;	/* next mailbox to use, and to look in */
	char	a_warned;
	int	a_ticks;	/* seconds the CCB has been out */
	int	(*a_done)();
	long	a_ccbpa;	/* the CCB's physical address */
	long	a_segpa;	/* and that of the list of its pieces */
	u_char	a_board[4];
	u_char	a_xbios[2];
} aha;
#define	A_UNKNOWN	0	/* a_state: not looked for yet */
#define	A_ABSENT	1
#define	A_PRESENT	2

static int ahaintr(), ahawatch(), ahacmd(), ahalate();

/*
 * How the adapter takes its turns on the AT bus, which is what a disk's
 * speed comes to when the SCSI bus is as quick as that one: the
 * microseconds it holds the bus, 2 to 15, and lets it go, 1 to 64 and
 * taken in fours, and its speed there in tenths of a megabyte a second,
 * 0 for as its switches or its own set-up program left it.
 *
 * The turns are the adapter's own, 11 and 4.  A real disk read 32K at a
 * time gave 2.03 MB/s at 7 and 5, which other drivers for this adapter
 * use so that a diskette drive's DMA is not kept waiting, and this
 * kernel has no driver for one; 2.38 at 11 and 4; and 2.77 at 15 and 1.
 * The processor is kept off the bus 200 ns for each byte moved at 5
 * MB/s however the turns are cut, but with 15 and 1 it has the bus a
 * sixteenth of the time while a transfer lasts, which for 64K is 14 ms:
 * an interrupt's handler takes sixteen times as long then, a serial
 * port with no FIFO can lose what comes in, and the memory's refresh,
 * which is why 15 is the most, has no room.
 *
 * sysctl changes the three (machdep.aha_buson, aha_busoff, aha_speed),
 * and they are given again when the adapter is reset.  A speed the
 * machine's memory cannot keep up with spoils what is read and written.
 */
static int ahatune[3] = { 11, 4, 0 };
static u_char ahaspeeds[] = { 50, 67, 80, 100, 57 };	/* AC_SPEED's codes */

/* Give the adapter the value of one of them: 0 if it took it. */
static int
ahaset(which, v)
	int which, v;
{
	static u_char cmd[] = { AC_BUSON, AC_BUSOFF, AC_SPEED };
	u_char c[2], r[1];
	register int i;

	if (which == 2) {
		if (v == 0)
			return (0);
		for (i = 0; ahaspeeds[i] != v; i++)
			if (i == sizeof (ahaspeeds) - 1)
				return (-1);
		v = i;
	}
	c[0] = cmd[which];
	c[1] = v;
	return (ahacmd(c, 2, r, 0));
}

static
put3(p, v)
	register u_char *p;
	long v;
{
	p[0] = v >> 16;
	p[1] = v >> 8;
	p[2] = v;
}

/*
 * Wait until the bits of a port that are in mask have all of allof and
 * none of noneof.  Returns 0, or -1 if they never do.
 */
static int
ahawait(port, mask, allof, noneof, spins)
	int port, mask, allof, noneof;
	register long spins;
{
	register int v;

	while (--spins >= 0) {
		v = inb(port) & mask;
		if ((v & allof) == allof && (v & noneof) == 0)
			return (0);
	}
	return (-1);
}

/*
 * Give the adapter a command of n bytes and take nin bytes of reply.
 * Returns 0, or a negative number that says where it went wrong.
 */
static int
ahacmd(cmd, n, reply, nin)
	u_char *cmd, *reply;
	int n, nin;
{
	register int b = aha.a_base;
	register int i;
	long spins;
	int s;

	for (i = 0; i < n; i++) {
		if (ahawait(b + AHA_STAT, ST_CDF, 0, ST_CDF, SPIN))
			return (-1);
		outb(b + AHA_DATA, cmd[i]);
	}
	for (i = 0; i < nin; i++) {
		for (spins = SPIN; spins > 0; spins--) {
			if (inb(b + AHA_STAT) & ST_DF)
				break;
			if (inb(b + AHA_INTR) & IF_HACC)
				goto done;	/* over early: not known */
		}
		if (spins == 0)
			return (-2);
		reply[i] = inb(b + AHA_DATA);
	}
done:
	if (ahawait(b + AHA_INTR, IF_HACC, IF_HACC, 0, SPIN))
		return (-3);
	s = inb(b + AHA_STAT);
	outb(b + AHA_CTRL, CTRL_IRST);
	if (s & ST_INVDCMD)
		return (-4);
	return (i < nin ? -5 : 0);
}

/*
 * Reset the adapter, and the SCSI bus too if how is CTRL_SCRST, and get
 * it ready for SCSI commands.  Returns 0; -1 if there is no adapter at
 * the address; a greater number, having said why, if there is one that
 * cannot be used.
 */
static int
ahasetup(how)
	int how;
{
	register int b = aha.a_base;
	u_char c[5], r[4];
	long spins;
	int e;

	if (inb(b + AHA_STAT) == 0xFF)
		return (-1);		/* nothing answers there */
	outb(b + AHA_CTRL, how | CTRL_SRST | CTRL_IRST);
	for (spins = SPIN / 20; spins > 0; spins--)
		(void) inb(b + AHA_STAT);
	if (ahawait(b + AHA_STAT, ST_MASK, ST_INIT | ST_IDLE,
	    ST_STST | ST_DIAGF | ST_INVDCMD | ST_DF | ST_CDF, 10 * SPIN) ||
	    (inb(b + AHA_INTR) & IF_MASK))
		return (-1);		/* something else does */
	c[0] = AC_INQUIRY;
	if (ahacmd(c, 1, aha.a_board, 4))
		return (-1);

	/*
	 * A C, CF or CP whose BIOS is counting 255 heads keeps its
	 * mailboxes locked, so that a driver that does not know of that
	 * way of counting cannot use the disk: it is unlocked with the
	 * code it gives.  Older adapters do not know the command.
	 */
	c[0] = AC_EXTBIOS;
	if (ahacmd(c, 1, aha.a_xbios, 2) == 0 &&
	    ((aha.a_xbios[0] & XB_XLATE) || aha.a_xbios[1])) {
		c[0] = AC_MBENABLE;
		c[1] = 0;
		c[2] = aha.a_xbios[1];
		if (e = ahacmd(c, 3, r, 0))
			printf("aha: mailbox unlock failed (%d)\n", e);
	} else
		aha.a_xbios[0] = aha.a_xbios[1] = 0;

	c[0] = AC_RETCONF;
	if (e = ahacmd(c, 1, r, 3)) {
		printf("aha: at 0x%x: no configuration (%d)\n", b, e);
		return (1);
	}
	aha.a_dma = r[0] & 0x80 ? 7 : r[0] & 0x40 ? 6 : r[0] & 0x20 ? 5 :
	    r[0] & 0x01 ? 0 : -1;
	aha.a_irq = r[1] & 0x01 ? 9 : r[1] & 0x02 ? 10 : r[1] & 0x04 ? 11 :
	    r[1] & 0x08 ? 12 : r[1] & 0x20 ? 14 : r[1] & 0x40 ? 15 : -1;
	aha.a_id = r[2] & 7;
	if (aha.a_dma < 0 || aha.a_irq < 0) {
		printf("aha: at 0x%x: no irq or dma set\n", b);
		return (2);
	}
	/*
	 * The channel is the adapter's way of asking for the bus, not
	 * the DMA controller's to drive: put it in cascade mode.
	 */
	if (aha.a_dma) {
		outb(0xD6, 0xC0 | aha.a_dma - 4);
		outb(0xD4, aha.a_dma - 4);
	} else {
		outb(0x0B, 0xC0);
		outb(0x0A, 0);
	}
	for (e = 0; e < 3; e++)
		(void) ahaset(e, ahatune[e]);

	bzero(ahamb, sizeof (ahamb));
	aha.a_mbo = aha.a_mbi = 0;
	aha.a_ccbpa = fartophys(&ahaccb);
	aha.a_segpa = fartophys(ahasegs);
	c[0] = AC_MBINIT;
	c[1] = AHA_NMBOX;
	put3(&c[2], fartophys(ahamb));
	if (e = ahacmd(c, 5, r, 0)) {
		printf("aha: at 0x%x: mailbox init failed (%d)\n", b, e);
		return (3);
	}
	return (0);
}

/*
 * The adapter's speed on the AT bus as it reports it, in tenths of a
 * megabyte a second, for reading memory or for writing it.  The byte is
 * one of AC_SPEED's five codes or, with its top bit, the lengths of a
 * cycle's parts: the pulse, 100 ns and 50 more for each of three bits'
 * count, the read's and the write's apart, and the pause after it, 100
 * ns or 150.  A cycle moves two bytes.  An adapter set up by its own
 * program reports the second kind.  (Technical reference, 5-6.)
 */
static int
aharate(b, wr)
	register int b;
	int wr;
{
	if (b < sizeof (ahaspeeds))
		return (ahaspeeds[b]);
	if (!(b & 0x80))
		return (0);
	return (20000 / (100 + 50 * ((wr ? b : b >> 4) & 7) +
	    (b & 0x08 ? 150 : 100)));
}

/*
 * What the adapter and a target agreed, which the adapter knows once it
 * has sent the target a command: the rate in tenths of a megabyte a
 * second if synchronous, 0 if not, -1 if the adapter will not say.
 * The period is counted from 100 ns by the C series, by its technical
 * reference (5-11), and a real CF's came to what its disk said it had
 * agreed; from 200 by a B, by what other drivers do.  Called at
 * splbio, with no command outstanding.
 */
ahasync(target)
	int target;
{
	u_char c[2], r[16];
	register int v;

	c[0] = AC_RETSETUP;
	c[1] = sizeof (r);
	if (ahacmd(c, 2, r, sizeof (r)))
		return (-1);
	v = r[8 + target];
	if (!(v & 0x80))
		return (0);
	return (10000 / ((aha.a_board[0] >= 'D' ? 100 : 200) +
	    50 * (v >> 4 & 7)));
}

/*
 * sysctl's side of ahatune.  What is read is what the adapter says it
 * has, when there is one and no command is out, and otherwise what it
 * was last given.  It is given a new value only between commands, so a
 * write may find it busy and need trying again.
 */
ahasysctl(which, oldp, oldlenp, newp, newlen)
	int which;
	void __far *oldp;
	size_t *oldlenp;
	void __far *newp;
	size_t newlen;
{
	u_char c[2], r[4];
	int v, e, s, idle;

	v = ahatune[which];
	s = splbio();
	idle = aha.a_state == A_PRESENT && !aha.a_busy;
	c[0] = AC_RETSETUP;
	c[1] = sizeof (r);
	if (idle && ahacmd(c, 2, r, sizeof (r)) == 0)
		v = which < 2 ? r[2 + which] : aharate(r[1], 0);
	splx(s);
	if (e = sysctl_int(oldp, oldlenp, newp, newlen, &v))
		return (e);
	if (!newp)
		return (0);
	if (which == 0 && (v < 2 || v > 15) ||
	    which == 1 && (v < 1 || v > 64) || which == 2 && v == 0)
		return (EINVAL);
	s = splbio();
	if (aha.a_state != A_PRESENT)
		e = ENXIO;
	else if (aha.a_busy)
		e = EBUSY;
	else if (ahaset(which, v))
		e = which == 2 ? EINVAL : EIO;
	else
		ahatune[which] = v;
	splx(s);
	return (e);
}

/*
 * Find the adapter, the first time, and say whether there is one:
 * 0 if so.  Called at splbio.
 */
ahaattach()
{
	register int i, e;
	register char *name;
	u_char c[2], r[4];

	if (aha.a_state != A_UNKNOWN)
		return (aha.a_state != A_PRESENT);
	aha.a_state = A_ABSENT;
	for (i = 0; i < NPORTS; i++) {
		aha.a_base = ahaports[i];
		if ((e = ahasetup(0)) >= 0)
			break;
	}
	if (i == NPORTS || e != 0)
		return (1);
	/* The 1540 of each is the same without a diskette controller. */
	switch (aha.a_board[0]) {
	case 'D':
		name = "C";
		break;
	case 'E':
		name = "CF";
		break;
	case 'F':
		name = "CP";
		break;
	default:
		name = "";
	}
	printf("aha: AHA-1542%s at 0x%x irq %d dma %d id %d", name,
	    aha.a_base, aha.a_irq, aha.a_dma, aha.a_id);
	/* How it shares the AT bus, and its speed there: read/write. */
	c[0] = AC_RETSETUP;
	c[1] = sizeof (r);
	if (ahacmd(c, 2, r, sizeof (r)) == 0) {
		i = aharate(r[1], 0);
		e = aharate(r[1], 1);
		printf(", bus on %d off %d us, %d.%d", r[2], r[3],
		    i / 10, i % 10);
		if (e != i)
			printf("/%d.%d", e / 10, e % 10);
		printf(" MB/s");
	}
	printf("\n");
	aha.a_state = A_PRESENT;
	setirq(aha.a_irq, IPL_BIO, ahaintr, 0);
	timeout(ahawatch, (caddr_t)0, hz);
	return (0);
}

/* The adapter's own SCSI id: there is no target there. */
ahaid()
{
	return (aha.a_id);
}

/* Does the adapter's BIOS count 255 heads and 63 sectors on big disks? */
ahaxlate()
{
	return (aha.a_xbios[0] & XB_XLATE);
}

/*
 * Start filling the CCB: a command of cdblen bytes for a target, which
 * moves len bytes at a physical address in the direction dir.  The
 * caller puts the command's bytes in ahaccb.cc_cdb.
 */
ahafill(target, dir, cdblen, addr, len)
	int target, dir, cdblen;
	long addr;
	u_int len;
{
	register struct ccb *cp = &ahaccb;

	bzero(cp, sizeof (*cp));
	cp->cc_op = CCB_INITIATOR;
	cp->cc_idlun = target << 5 | dir;
	cp->cc_cdblen = cdblen;
	cp->cc_senselen = AHA_NSENSE;
	put3(cp->cc_datalen, (long)len);
	put3(cp->cc_dataptr, addr);
}

/*
 * The same for a command whose data is in n pieces, each somewhere of
 * its own in memory, which the adapter fills or empties in turn:
 * ahaseg says where each is, and then this fills the CCB.
 */
ahaseg(i, addr, len)
	int i;
	long addr;
	u_int len;
{
	put3(ahasegs[i].sg_len, (long)len);
	put3(ahasegs[i].sg_addr, addr);
}

ahafillsegs(target, dir, cdblen, n)
	int target, dir, cdblen, n;
{
	ahafill(target, dir, cdblen, aha.a_segpa,
	    n * sizeof (struct ahaseg));
	ahaccb.cc_op = CCB_PIECES;
}

/* Put the CCB in a mailbox and tell the adapter. */
static
ahaout()
{
	register struct mbox *mb = &ahamb[aha.a_mbo];
	static int count;

	aha.a_mbo = (aha.a_mbo + 1) % AHA_NMBOX;
	if ((ahatest & 2) && ++count == 30)
		return;
	put3(mb->mb_ccb, aha.a_ccbpa);
	mb->mb_status = MBO_START;
	if (ahawait(aha.a_base + AHA_STAT, ST_CDF, 0, ST_CDF, SPIN))
		printf("aha: not ready\n");
	outb(aha.a_base + AHA_DATA, AC_START);
}

/* Take what is in the next full mailbox in, or return MBI_FREE. */
static int
ahaget(pa)
	long *pa;
{
	register struct mbox *mb;
	register int i, st;

	for (i = 0; i < AHA_NMBOX; i++) {
		mb = &ahamb[AHA_NMBOX + (aha.a_mbi + i) % AHA_NMBOX];
		if (mb->mb_status != MBI_FREE)
			break;
	}
	if (i == AHA_NMBOX)
		return (MBI_FREE);
	aha.a_mbi = (aha.a_mbi + i + 1) % AHA_NMBOX;
	st = mb->mb_status;
	*pa = (long)mb->mb_ccb[0] << 16 | (u_int)(mb->mb_ccb[1] << 8) |
	    mb->mb_ccb[2];
	mb->mb_status = MBI_FREE;
	return (st);
}

/*
 * Send the CCB.  When it is done, done(how) is called at splbio, how
 * being what the mailbox said, MBI_...; or AHA_LOST if the adapter
 * never answered and has been reset, with the SCSI bus; or AHA_DEAD if
 * it would not be reset either, after which nothing more can be sent.
 * Called at splbio.
 */
ahago(done)
	int (*done)();
{
	if (aha.a_state != A_PRESENT) {
		(*done)(AHA_DEAD);
		return;
	}
	aha.a_done = done;
	aha.a_ticks = 0;
	aha.a_busy = 1;
	ahaout();
}

/*
 * Send the CCB and wait here for it, with the processor: for looking
 * at what is on the bus, before there is anything else to do.  Returns
 * what the mailbox said, or AHA_LOST.  Called at splbio.
 */
ahapoll()
{
	register int st;
	long pa;

	ahaout();
	if (ahawait(aha.a_base + AHA_INTR, IF_MBIF, IF_MBIF, 0, 20 * SPIN))
		return (AHA_LOST);
	st = ahaget(&pa);
	outb(aha.a_base + AHA_CTRL, CTRL_IRST);
	return (st == MBI_FREE ? AHA_LOST : st);
}

/*
 * Make the adapter ready for ahapoll whatever it was doing, for a crash
 * dump: it and the SCSI bus are reset, and a command that was out is
 * lost.  Returns 0 if it is ready.  Called with interrupts off.
 */
ahadumpreset()
{
	if (aha.a_state != A_PRESENT)
		return (-1);
	aha.a_busy = 0;
	return (ahasetup(CTRL_SCRST));
}

/* Take what the mailboxes in have.  Called at splbio. */
static
ahadone()
{
	register int st;
	long pa;

	while ((st = ahaget(&pa)) != MBI_FREE) {
		if (!aha.a_busy || pa != aha.a_ccbpa || st == MBI_NOTFOUND) {
			printf("aha: stray completion %d for %X\n", st, pa);
			continue;
		}
		aha.a_busy = 0;
		(*aha.a_done)(st);
	}
}

static
ahalate()
{
	register int s = splbio();

	ahadone();
	splx(s);
}

static
ahaintr(arg, tf)
	int arg;
	caddr_t tf;
{
	register int fl;
	static int count;

	if (aha.a_state != A_PRESENT)
		return;
	fl = inb(aha.a_base + AHA_INTR);
	if ((fl & IF_ANY) == 0)
		return;
	if ((ahatest & 1) && tf && ++count == 20)
		return;
	outb(aha.a_base + AHA_CTRL, CTRL_IRST);
	if (fl & IF_SCRD)
		printf("aha: SCSI bus reset\n");
	if ((ahatest & 8) && tf)
		timeout(ahalate, (caddr_t)0, 1);
	else
		ahadone();
}

/*
 * Once a second: has a command finished without its interrupt reaching
 * here, or been out too long?
 */
static
ahawatch()
{
	register int s = splbio();
	register int st;

	if (aha.a_state == A_PRESENT && aha.a_busy) {
		if (++aha.a_ticks >= 2 &&
		    (inb(aha.a_base + AHA_INTR) & IF_ANY)) {
			if (!aha.a_warned) {
				aha.a_warned = 1;
				printf("aha: lost interrupt (irq %d shared?)\n",
				    aha.a_irq);
			}
			ahaintr(0, (caddr_t)0);
		} else if (aha.a_ticks >= ahatimeout) {
			printf("aha: timeout after %d s, resetting adapter and bus\n",
			    ahatimeout);
			aha.a_busy = 0;
			st = AHA_LOST;
			if (ahasetup(CTRL_SCRST)) {
				printf("aha: reset failed\n");
				aha.a_state = A_ABSENT;
				st = AHA_DEAD;
			}
			(*aha.a_done)(st);
		}
	}
	splx(s);
	if (aha.a_state == A_PRESENT)
		timeout(ahawatch, (caddr_t)0, hz);
}
#endif /* NSD */
