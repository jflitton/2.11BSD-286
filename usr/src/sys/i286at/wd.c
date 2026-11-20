/*
 * Hard disks on the PC/AT's disk controller: the drives known as IDE,
 * which answer at the ports of the AT's WD1003 and take its commands.
 * One controller, at 0x1F0 with interrupt request 14, and its two
 * drives.  A drive must be able to say what it is (the identify
 * command), which the ST-506 drives of a real WD1003 cannot.
 *
 * A minor device number is the drive in bits 3 and up and the partition
 * in the low three.
 */
#include "param.h"
#include "systm.h"
#include "user.h"
#include "buf.h"
#include "conf.h"
#include "disklabel.h"
#include "disk.h"
#include "pcdisk.h"
#include "dk.h"

#define	NWD		2
#define	WD_IRQ		14

/* The controller's ports. */
#define	WD_DATA		0x1F0	/* 16 bits wide */
#define	WD_ERROR	0x1F1	/* read */
#define	WD_SECCNT	0x1F2
#define	WD_SECTOR	0x1F3
#define	WD_CYLLO	0x1F4
#define	WD_CYLHI	0x1F5
#define	WD_SDH		0x1F6	/* drive, head and how sectors are named */
#define	WD_STATUS	0x1F7	/* read: and it takes back the request */
#define	WD_COMMAND	0x1F7	/* write */
#define	WD_CTL		0x3F6

#define	WDS_BUSY	0x80
#define	WDS_READY	0x40
#define	WDS_WRFAULT	0x20
#define	WDS_DRQ		0x08	/* it wants data moved */
#define	WDS_ERR		0x01
#define	WDS_BITS	"\20\10busy\7ready\6fault\5seeked\4drq\3corrected\2index\1error"
#define	WDE_BITS	"\20\10badblock\7ecc\5noid\3aborted\2track0\1mark"

#define	WDSD_CHS	0xA0	/* sectors by cylinder, head and sector */
#define	WDSD_LBA	0xE0	/* sectors by number */

#define	WDC_READ	0x20
#define	WDC_WRITE	0x30
#define	WDC_SETGEOM	0x91
#define	WDC_READM	0xC4	/* several sectors to an interrupt */
#define	WDC_WRITEM	0xC5
#define	WDC_SETMULTI	0xC6
#define	WDC_IDENTIFY	0xEC

#define	WDCTL_HD15	0x08	/* a WD1003 wants this for heads 8 to 15 */
#define	WDCTL_NOINTR	0x02

#define	WDTRIES		4	/* times a transfer is tried */
/*
 * How many times to look at the status before giving a drive up: some
 * seconds on a 286.
 */
#define	WDPATIENCE	400000L

struct wd {
	struct	dkdevice wd_dk;
	daddr_t	wd_start;	/* first sector of this system's part */
	struct	pcgeom wd_geom;
#define	wd_size		wd_geom.pg_size
#define	wd_cyls		wd_geom.pg_cyls
#define	wd_heads	wd_geom.pg_heads
#define	wd_sects	wd_geom.pg_sects
	u_int	wd_spc;		/* sectors per cylinder */
	char	wd_multi;	/* sectors to an interrupt */
	char	wd_lba;		/* sectors are named by number */
	char	wd_state;
} wd[NWD];
#ifdef UCB_METER
static	int	wd_dkn = -1;	/* number for iostat */
#endif
#define	WD_UNKNOWN	0	/* wd_state: not looked for yet */
#define	WD_ABSENT	1
#define	WD_PRESENT	2

struct disklabel __far wdlabel[NWD];

/*
 * The most sectors to move at one interrupt, if a drive can move
 * several: a power of 2.  1 has every drive move one, the WD1003's way.
 * (patchable)
 */
int	wdmulti = 16;

/*
 * The queue of transfers for both drives, in the order disksort gives
 * them, and where the one at its head has got to.  b_resid is not a
 * transfer's until it is done: disksort keeps the cylinder there.
 */
struct buf wdtab;
static struct {
	daddr_t	x_sect;		/* next sector, counted from the drive's
				   first */
	long	x_addr;		/* where its data is, a physical address */
	u_int	x_left;		/* sectors still to move */
	u_int	x_now;		/* of them, how many have been written to
				   the drive and not yet acknowledged */
	int	x_tries;
} wdx;

int	wdstrategy();
static int wdintr();

/*
 * Wait for the controller to be free and, if bits is not 0, for one of
 * those bits of its status.  Returns the status, or -1 if it will not.
 */
static int
wdwait(bits, patience)
	int bits;
	register long patience;
{
	register int s;

	while (--patience >= 0) {
		s = inb(WD_STATUS);
		if ((s & WDS_BUSY) == 0 && (bits == 0 || (s & bits)))
			return (s);
	}
	return (-1);
}

/*
 * Give a command that moves no data, and wait for it.  Returns 0 if the
 * drive did it.
 */
static int
wdsimple(unit, cmd, count, head)
	int unit, cmd, count, head;
{
	register int s;

	outb(WD_SECCNT, count);
	outb(WD_SDH, WDSD_CHS | unit << 4 | head);
	outb(WD_COMMAND, cmd);
	s = wdwait(0, WDPATIENCE);
	return (s < 0 || (s & WDS_ERR));
}

/*
 * Is the drive there, and what is it?
 */
static
wdprobe(unit)
	int unit;
{
	register struct wd *wp = &wd[unit];
	register struct buf *bp;
	u_short __far *id;
	char model[41];
	static char first = 1;
	int s, i, m;

	wp->wd_state = WD_ABSENT;
	bp = geteblk();
	s = splbio();
	/*
	 * The drive's requests are kept from the interrupt controller
	 * while it is asked questions, and its answers waited for.
	 */
	outb(WD_CTL, WDCTL_NOINTR);
	outb(WD_SDH, WDSD_CHS | unit << 4);
	if (inb(WD_SDH) != (WDSD_CHS | unit << 4) ||
	    wdwait(0, WDPATIENCE) < 0)
		goto out;		/* no controller, or a dead one */
	outb(WD_COMMAND, WDC_IDENTIFY);
	if (inb(WD_STATUS) == 0)
		goto out;		/* nothing drives the bus: no drive */
	i = wdwait(WDS_DRQ | WDS_ERR, WDPATIENCE);
	if (i < 0 || (i & WDS_ERR))
		goto out;
	pinsw(WD_DATA, bfpaddr(bp), 256);
	id = (u_short __far *)mapin(bp);
	wp->wd_cyls = id[1];
	wp->wd_heads = id[3];
	wp->wd_sects = id[6];
	wp->wd_spc = wp->wd_heads * wp->wd_sects;
	wp->wd_size = (long)wp->wd_cyls * wp->wd_spc;
	wp->wd_lba = (id[49] & 0x200) != 0;
	if (wp->wd_lba && (id[60] | id[61]))
		wp->wd_size = (long)id[61] << 16 | id[60];
	m = id[47] & 0xFF;
	for (i = 0; i < 40; i += 2) {	/* its name, two characters a word */
		model[i] = id[27 + i / 2] >> 8;
		model[i + 1] = id[27 + i / 2];
	}
	mapout(bp);
	while (i > 0 && (model[i - 1] == ' ' || model[i - 1] == 0))
		i--;
	model[i] = 0;
	if (wp->wd_spc == 0 || wp->wd_size == 0)
		goto out;
	/*
	 * Have it count sectors as it said it does, which an old drive
	 * may not until told, and move as many to an interrupt as it
	 * can.
	 */
	(void) wdsimple(unit, WDC_SETGEOM, wp->wd_sects, wp->wd_heads - 1);
	wp->wd_multi = 1;
	if (m > wdmulti)
		m = wdmulti;
	while (m & (m - 1))		/* a power of 2 */
		m &= m - 1;
	if (m > 1 && wdsimple(unit, WDC_SETMULTI, m, 0) == 0)
		wp->wd_multi = m;
	wp->wd_geom.pg_type = DTYPE_ESDI;
	wp->wd_state = WD_PRESENT;
out:
	(void) inb(WD_STATUS);
	splx(s);
	bp->b_flags = B_INVAL | B_AGE;
	brelse(bp);
	if (wp->wd_state != WD_PRESENT)
		return;
	printf("wd%d: %s, %D MB: %d cylinders, %d heads, %d sectors\n",
	    unit, model, wp->wd_size / 2048, wp->wd_cyls, wp->wd_heads,
	    wp->wd_sects);
	printf("wd%d: sectors named by %s, moved %d at a time\n", unit,
	    wp->wd_lba ? "number" : "cylinder and head", wp->wd_multi);
	if (first) {
		first = 0;
		setirq(WD_IRQ, IPL_BIO, wdintr, 0);
	}
#ifdef UCB_METER
	/* words a second as a drive of 3600 turns a minute moves them */
	if (wd_dkn < 0)
		dk_alloc(&wd_dkn, NWD, "wd", 60L * wp->wd_sects * 256L);
#endif
}

wdopen(dev, flag, mode)
	dev_t dev;
	int flag, mode;
{
	register int unit = dkunit(dev);
	register struct wd *wp;

	if (unit >= NWD)
		return (ENXIO);
	wp = &wd[unit];
	while (wp->wd_dk.dk_flags & DKF_OPENING)
		sleep((caddr_t)&wp->wd_dk, PRIBIO);
	if (wp->wd_state == WD_UNKNOWN) {
		wp->wd_dk.dk_flags |= DKF_OPENING;
		wp->wd_dk.dk_label = &wdlabel[unit];
		wdprobe(unit);
		wp->wd_dk.dk_flags &= ~DKF_OPENING;
		wakeup((caddr_t)&wp->wd_dk);
	}
	if (wp->wd_state != WD_PRESENT)
		return (ENXIO);
	return (pcdkopen(&wp->wd_dk, dev, mode, wdstrategy, "wd",
	    &wp->wd_geom, &wp->wd_start));
}

wdclose(dev, flag, mode)
	dev_t dev;
	int flag, mode;
{
	return (pcdkclose(&wd[dkunit(dev)].wd_dk, dev, mode));
}

daddr_t
wdsize(dev)
	dev_t dev;
{
	register struct wd *wp = &wd[dkunit(dev)];

	if (dkunit(dev) >= NWD || (wp->wd_dk.dk_flags & DKF_ALIVE) == 0)
		return (-1);
	return (wp->wd_dk.dk_parts[dkpart(dev)].p_size);
}

wdioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	return (pcdkioctl(&wd[dkunit(dev)].wd_dk, dev, cmd, data, flag,
	    wdstrategy, &wd[dkunit(dev)].wd_geom));
}

/*
 * Move the next sectors of a write to the drive, which has asked for
 * them.
 */
static
wdout(wp)
	register struct wd *wp;
{
	wdx.x_now = wdx.x_left < wp->wd_multi ? wdx.x_left : wp->wd_multi;
	poutsw(WD_DATA, wdx.x_addr, wdx.x_now << 8);
}

/*
 * Give the drive the command for what is left of the transfer at the
 * head of the queue.  Returns -1 if the drive does not take it.
 */
static int
wdcmd(bp)
	register struct buf *bp;
{
	register struct wd *wp = &wd[dkunit(bp->b_dev)];
	int unit = dkunit(bp->b_dev), s;
	u_int cyl, r;

	outb(WD_CTL, wp->wd_heads > 8 ? WDCTL_HD15 : 0);
	if (wp->wd_lba) {
		outb(WD_SDH, WDSD_LBA | unit << 4 |
		    (int)(wdx.x_sect >> 24) & 0x0F);
		if (wdwait(WDS_READY, WDPATIENCE) < 0)
			return (-1);
		outb(WD_SECTOR, (int)wdx.x_sect);
		outb(WD_CYLLO, (int)(wdx.x_sect >> 8));
		outb(WD_CYLHI, (int)(wdx.x_sect >> 16));
	} else {
		cyl = wdx.x_sect / wp->wd_spc;
		r = wdx.x_sect % wp->wd_spc;
		outb(WD_SDH, WDSD_CHS | unit << 4 | r / wp->wd_sects);
		if (wdwait(WDS_READY, WDPATIENCE) < 0)
			return (-1);
		outb(WD_SECTOR, r % wp->wd_sects + 1);
		outb(WD_CYLLO, cyl);
		outb(WD_CYLHI, cyl >> 8);
	}
	outb(WD_SECCNT, wdx.x_left);
	if (bp->b_flags & B_READ) {
		outb(WD_COMMAND, wp->wd_multi > 1 ? WDC_READM : WDC_READ);
		return (0);
	}
	outb(WD_COMMAND, wp->wd_multi > 1 ? WDC_WRITEM : WDC_WRITE);
	/* It asks for the first sectors without an interrupt. */
	s = wdwait(WDS_DRQ | WDS_ERR | WDS_WRFAULT, WDPATIENCE);
	if (s < 0 || (s & (WDS_ERR | WDS_WRFAULT)))
		return (-1);
	wdout(wp);
	return (0);
}

/*
 * The transfer at the head of the queue is over, well or badly.
 */
static
wddone(bp)
	register struct buf *bp;
{
	wdtab.b_active = 0;
	wdtab.b_actf = bp->av_forw;
	bp->b_resid = wdx.x_left << 9;
#ifdef UCB_METER
	if (wd_dkn >= 0)
		dk_busy &= ~(1 << (wd_dkn + dkunit(bp->b_dev)));
#endif
	iodone(bp);
}

/*
 * Start, or after an error start again, the transfer at the head of
 * the queue; when it has been tried WDTRIES times with nothing moved,
 * report it and go on to the next.  Called at splbio.
 */
static
wdstart()
{
	register struct buf *bp;
	register struct wd *wp;
	int status, error;

	while ((bp = wdtab.b_actf) != NULL) {
		wp = &wd[dkunit(bp->b_dev)];
		if (wdtab.b_active == 0) {
			wdtab.b_active = 1;
			wdx.x_sect = bp->b_blkno + wp->wd_start +
			    wp->wd_dk.dk_parts[dkpart(bp->b_dev)].p_offset;
			wdx.x_addr = bfpaddr(bp);
			wdx.x_left = bp->b_bcount >> 9;
			wdx.x_tries = 0;
#ifdef UCB_METER
			if (wd_dkn >= 0) {
				int dkn = wd_dkn + dkunit(bp->b_dev);

				dk_busy |= 1 << dkn;
				dk_xfer[dkn]++;
				dk_wds[dkn] += bp->b_bcount >> 6;
			}
#endif
		}
		if (wdx.x_tries++ < WDTRIES && wdcmd(bp) == 0)
			return;
		if (wdx.x_tries <= WDTRIES)
			continue;
		status = inb(WD_STATUS);
		error = inb(WD_ERROR);
		harderr(bp, "wd");
		printf("(sector %D of the drive) status=%b error=%b\n",
		    wdx.x_sect, status, WDS_BITS, error, WDE_BITS);
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		wddone(bp);
	}
}

wdstrategy(bp)
	register struct buf *bp;
{
	register struct wd *wp;
	register int s;

	if (dkunit(bp->b_dev) >= NWD ||
	    ((wp = &wd[dkunit(bp->b_dev)])->wd_dk.dk_flags & DKF_ALIVE) == 0) {
		bp->b_error = ENXIO;
		goto bad;
	}
	if (bp->b_bcount & 511) {	/* whole sectors only */
		bp->b_error = EINVAL;
		goto bad;
	}
	s = partition_check(bp, &wp->wd_dk);
	if (s < 0)
		goto bad;
	if (s == 0)			/* at the partition's end */
		goto done;
	if (bp->b_bcount == 0) {
		bp->b_resid = 0;
		goto done;
	}
	bp->b_cylin = (bp->b_blkno + wp->wd_start +
	    wp->wd_dk.dk_parts[dkpart(bp->b_dev)].p_offset) / wp->wd_spc;
	s = splbio();
	disksort(&wdtab, bp);
	if (wdtab.b_active == 0)
		wdstart();
	splx(s);
	return;
bad:
	bp->b_flags |= B_ERROR;
	bp->b_resid = bp->b_bcount;
done:
	iodone(bp);
}

/*
 * The drive has sectors that were asked for, or has written the ones
 * it was given, or has failed to.
 */
static
wdintr(arg, tf)
	int arg;
	caddr_t tf;
{
	register struct buf *bp = wdtab.b_actf;
	register struct wd *wp;
	register int status;
	u_int n;

	status = inb(WD_STATUS);
	if (wdtab.b_active == 0 || bp == NULL)
		return;			/* nothing was asked of it */
	wp = &wd[dkunit(bp->b_dev)];
	if (status & (WDS_BUSY | WDS_ERR | WDS_WRFAULT))
		goto again;
	if (bp->b_flags & B_READ) {
		if ((status & WDS_DRQ) == 0)
			goto again;
		n = wdx.x_left < wp->wd_multi ? wdx.x_left : wp->wd_multi;
		pinsw(WD_DATA, wdx.x_addr, n << 8);
	} else
		n = wdx.x_now;
	wdx.x_sect += n;
	wdx.x_addr += (long)n << 9;
	wdx.x_left -= n;
	/* It has done something: the next error is the first of its kind. */
	wdx.x_tries = 1;
	if (wdx.x_left == 0) {
		wddone(bp);
		wdstart();
	} else if ((bp->b_flags & B_READ) == 0) {
		if ((status & WDS_DRQ) == 0)
			goto again;
		wdout(wp);
	}
	return;
again:
	wdstart();
}
