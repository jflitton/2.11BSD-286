/*
 * Diskettes on the PC/AT's diskette controller: an NEC 765 at 0x3F0,
 * with interrupt request 6 and DMA channel 2, and its two drives.  As
 * rx(4) has the RX02's, a diskette has no label and no partitions, and
 * the letter of the device says the density:
 *
 *	fl0a	drive 0, double density: 720K, in a 3.5 inch drive
 *	fl0b	drive 0, high density: 1.44 MB in a 3.5 inch drive and
 *		1.2 MB in a 5.25 inch one
 *
 * A minor device number is the drive in bits 3 and up and the density
 * in the low three.  What a drive is, the machine's setup program wrote
 * in the clock chip's memory.
 */
#include "fl.h"
#if NFL > 0
#include "param.h"
#include "systm.h"
#include "user.h"
#include "buf.h"
#include "conf.h"
#include "file.h"
#include "ioctl.h"
#include "disk.h"
#include "kernel.h"
#include "flreg.h"

#define	FL_IRQ		6
#define	FL_CYLS		80
#define	FL_HEADS	2

/* The controller's ports. */
#define	FL_DOR		0x3F2	/* motors, the drive selected, reset */
#define	FL_STATUS	0x3F4
#define	FL_DATA		0x3F5	/* commands in, results out */
#define	FL_RATE		0x3F7	/* write: the data rate */

#define	FLD_RUN		0x04	/* without it the controller is reset */
#define	FLD_DMA		0x08	/* it may interrupt and ask for DMA */
#define	FLD_MOTOR	0x10	/* drive 0's motor */

#define	FLS_READY	0x80	/* it will take a byte, or give one: */
#define	FLS_GIVES	0x40	/*   give */

#define	FLC_SPECIFY	0x03
#define	FLC_RECAL	0x07	/* to cylinder 0 */
#define	FLC_SENSE	0x08	/* what was the interrupt for? */
#define	FLC_SEEK	0x0F
#define	FLC_WRITE	0x45
#define	FLC_READ	0x46
#define	FLC_FORMAT	0x4D	/* a track */

#define	ST0_BAD		0xC0	/* the command did not end well */
#define	ST0_SEEKEND	0x20
#define	ST0_BITS	"\20\10bad\7abnormal\6seekend\5check\4notready"
#define	ST1_PROTECTED	0x02
#define	ST1_BITS	"\20\10end\6crc\5overrun\3nodata\2protected\1nomark"

/* The clock chip's memory. */
#define	RTC_ADDR	0x70
#define	RTC_DATA	0x71
#define	RTC_DRIVES	0x10	/* drive 0 in the high four bits */
#define	RTCD_1200	2	/* 5.25 inch, high density */
#define	RTCD_720	3	/* 3.5 inch, double density */
#define	RTCD_1440	4	/* 3.5 inch, high density */

/* The DMA controller, for channel 2. */
#define	DMA_ADDR	0x04
#define	DMA_COUNT	0x05
#define	DMA_MASK	0x0A
#define	DMA_MODE	0x0B
#define	DMA_FLIP	0x0C	/* the next byte written is a low one */
#define	DMA_PAGE	0x81	/* the address's top eight bits */
#define	DMAM_READ	0x46	/* to memory, one transfer */
#define	DMAM_WRITE	0x4A
#define	DMA_SHUT	0x06	/* DMA_MASK: channel 2 off, */
#define	DMA_OPEN	0x02	/*   and on */

#define	FLTRIES		5	/* times a transfer is tried */
#define	FLPATIENCE	20000	/* looks at the status for one byte */
#define	FLLATE		3	/* seconds an interrupt is waited for */
#define	FLIDLE		3	/* seconds with nothing to do before the
				   motor is stopped */

struct fltype {
	int	ft_size;	/* sectors on the diskette */
	char	ft_sects;	/*   and on a track */
	char	ft_rate;	/* for FL_RATE */
	char	ft_gap;		/* between sectors: read and written, */
	char	ft_fgap;	/*   and as a track is formatted */
	char	*ft_name;
} fltypes[] = {
	{ 1440, 9, 2, 0x2A, 0x50, "720K" },
	{ 2400, 15, 0, 0x1B, 0x54, "1.2 MB" },
	{ 2880, 18, 0, 0x1B, 0x6C, "1.44 MB" },
};

static char fldrive[NFL];	/* what each drive is: RTCD_... */
static int flcyl[NFL];		/* where its head is; -1 if not known */
static int flmotor = -1;	/* the drive whose motor is on */
static int flidle;		/* seconds since there was anything to do */
static char flwatching;		/* flwatch is running */

/*
 * The queue of transfers, in the order disksort gives them, and where
 * the one at its head has got to.  b_resid is not a transfer's until it
 * is done: disksort keeps the cylinder there.  flfbuf is the request to
 * format a track, which waits in the queue with the others.
 */
struct buf fltab;
static struct buf flfbuf;
static struct {
	char	x_state;
	char	x_reset;	/* the controller is to be reset first */
	char	x_bounce;	/* the sector goes by way of flbuf */
	char	x_late;		/* no interrupt came */
	int	x_tries;
	int	x_wait;		/* seconds more to wait for the interrupt */
	int	x_cyl;		/* where the head was sent */
	int	x_sect;		/* next sector of the diskette */
	u_int	x_left;		/* sectors still to move, */
	u_int	x_now;		/*   and of them, by this command */
	long	x_addr;		/* where the data is, a physical address */
} flx;
#define	FL_IDLE		0	/* x_state: nothing is under way */
#define	FL_WAIT		1	/* a motor is starting, or a head settling */
#define	FL_RESET	2	/* waiting for the interrupt after a reset, */
#define	FL_RECAL	3	/*   a move to cylinder 0, */
#define	FL_SEEK		4	/*   a move to x_cyl, */
#define	FL_XFER		5	/*   sectors, or a track formatted */

/*
 * A sector whose place in memory has a multiple of 64K inside it goes
 * through here, DMA not counting past one; and what a track is
 * formatted with.  512 bytes of the 1024 have no such multiple inside.
 */
static char flbuf[1024];
static char *flbp;
static long flbpa;		/* its physical address */
static u_char flst[7];		/* the last command's results */
extern long kdbase;

int	flstrategy();
static int flintr(), flwatch(), flwake();

static int
flout(c)
	int c;
{
	register int n = FLPATIENCE;

	while ((inb(FL_STATUS) & (FLS_READY | FLS_GIVES)) != FLS_READY)
		if (--n == 0)
			return (-1);
	outb(FL_DATA, c);
	return (0);
}

static int
flin()
{
	register int n = FLPATIENCE;

	while ((inb(FL_STATUS) & (FLS_READY | FLS_GIVES)) !=
	    (FLS_READY | FLS_GIVES))
		if (--n == 0)
			return (-1);
	return (inb(FL_DATA));
}

static int
flcmd(c, n)
	register u_char *c;
	register int n;
{
	while (--n >= 0)
		if (flout(*c++) < 0)
			return (-1);
	return (0);
}

/*
 * Ask what an interrupt was for.  Returns -1 if the controller will not
 * say, or had none to speak of.
 */
static int
flsense(st0, cyl)
	int *st0, *cyl;
{
	if (flout(FLC_SENSE) < 0 || (*st0 = flin()) < 0)
		return (-1);
	if ((*st0 & ST0_BAD) == 0x80)
		return (-1);
	if ((*cyl = flin()) < 0)
		return (-1);
	return (0);
}

static
fldma(mode, addr, count)
	int mode;
	long addr;
	u_int count;
{
	count--;
	outb(DMA_MASK, DMA_SHUT);
	outb(DMA_FLIP, 0);
	outb(DMA_MODE, mode);
	outb(DMA_ADDR, (int)addr);
	outb(DMA_ADDR, (int)addr >> 8);
	outb(DMA_PAGE, (int)(addr >> 16));
	outb(DMA_COUNT, count);
	outb(DMA_COUNT, count >> 8);
	outb(DMA_MASK, DMA_OPEN);
}

/*
 * What the device is on the drive it names, or NULL if the drive is not
 * there or has no such density.
 */
static struct fltype *
flwhat(dev)
	dev_t dev;
{
	register int unit = dkunit(dev), high = dkpart(dev);

	if (unit >= NFL || high > 1)
		return (NULL);
	switch (fldrive[unit]) {
	case RTCD_1200:
		return (high ? &fltypes[1] : NULL);
	case RTCD_720:
		return (high ? NULL : &fltypes[0]);
	case RTCD_1440:
		return (&fltypes[high ? 2 : 0]);
	}
	return (NULL);
}

flopen(dev, flag, mode)
	dev_t dev;
	int flag, mode;
{
	static char looked;
	register int unit, t;

	if (!looked) {
		looked = 1;
		if (inb(FL_STATUS) == 0xFF)
			return (ENXIO);		/* no controller */
		outb(RTC_ADDR, RTC_DRIVES);
		t = inb(RTC_DATA);
		fldrive[0] = t >> 4 & 15;
		fldrive[1] = t & 15;
		for (unit = 0; unit < NFL; unit++) {
			if (fldrive[unit] > RTCD_1440)	/* 2.88 MB */
				fldrive[unit] = RTCD_1440;
			flcyl[unit] = -1;
			if (fldrive[unit] >= RTCD_1200)
				printf("fl%d: %s\n", unit,
				    fltypes[fldrive[unit] == RTCD_720 ? 0 :
				    fldrive[unit] == RTCD_1200 ? 1 : 2].ft_name);
		}
		flbpa = kdbase + (u_int)flbuf;
		flbp = flbuf;
		if ((flbpa & 0xFFFF) > 0x10000L - 512) {
			flbpa += 512;
			flbp += 512;
		}
		flx.x_reset = 1;
		setirq(FL_IRQ, IPL_BIO, flintr, 0);
	}
	if (flwhat(dev) == NULL)
		return (ENXIO);
	return (0);
}

daddr_t
flsize(dev)
	dev_t dev;
{
	register struct fltype *tp = flwhat(dev);

	return (tp ? (daddr_t)tp->ft_size : -1);
}

/*
 * The transfer at the head of the queue is over, well or badly.
 */
static
fldone(bp)
	register struct buf *bp;
{
	fltab.b_active = 0;
	fltab.b_actf = bp->av_forw;
	bp->b_resid = bp == &flfbuf ? 0 :
	    bp->b_bcount - ((u_int)(flx.x_sect - (int)bp->b_blkno) << 9);
	iodone(bp);
}

/*
 * Do the next thing the transfer at the head of the queue wants, which
 * an interrupt or a clock will say is done: reset the controller, start
 * the motor, find cylinder 0, move the head, or move sectors, as many
 * as are left on the track and before a multiple of 64K in memory.
 * When it has been tried FLTRIES times with nothing moved, report it
 * and go on to the next.  Called at splbio.
 */
static
flgo()
{
	register struct buf *bp;
	register struct fltype *tp;
	int unit, head, s, n, i;
	long room;
	u_char c[9];

	while ((bp = fltab.b_actf) != NULL) {
		unit = dkunit(bp->b_dev);
		tp = flwhat(bp->b_dev);
		if (fltab.b_active == 0) {
			fltab.b_active = 1;
			flx.x_sect = bp->b_blkno;
			flx.x_addr = bfpaddr(bp);
			flx.x_left = bp->b_bcount >> 9;
			if (flx.x_left > tp->ft_size - flx.x_sect)
				flx.x_left = tp->ft_size - flx.x_sect;
			if (bp == &flfbuf)
				flx.x_left = 1;
			flx.x_tries = 0;
			flx.x_late = 0;
		}
		if (flx.x_tries >= FLTRIES) {
			if (flx.x_late) {
				printf("fl%d: not ready\n", unit);
				bp->b_error = ENXIO;
			} else {
				harderr(bp, "fl");
				printf("(sector %d) st0=%b st1=%b\n", flx.x_sect,
				    flst[0], ST0_BITS, flst[1], ST1_BITS);
				bp->b_error = flst[1] & ST1_PROTECTED ?
				    EROFS : EIO;
			}
			bp->b_flags |= B_ERROR;
			fldone(bp);
			continue;
		}
		flidle = 0;
		if (!flwatching) {
			flwatching = 1;
			timeout(flwatch, (caddr_t)0, hz);
		}
		if (flx.x_reset) {
			outb(DMA_MASK, DMA_SHUT);
			outb(FL_DOR, 0);
			for (i = 0; i < 10; i++)	/* held a while */
				(void) inb(FL_STATUS);
			flmotor = -1;
			flcyl[0] = flcyl[1] = -1;
			flx.x_state = FL_RESET;
			flx.x_wait = FLLATE;
			outb(FL_DOR, FLD_RUN | FLD_DMA);
			return;
		}
		if (flmotor != unit) {
			flmotor = unit;
			outb(FL_DOR, FLD_RUN | FLD_DMA | FLD_MOTOR << unit | unit);
			flx.x_state = FL_WAIT;
			timeout(flwake, (caddr_t)0, hz / 2);
			return;
		}
		outb(FL_RATE, tp->ft_rate);
		n = tp->ft_sects * FL_HEADS;
		flx.x_cyl = flx.x_sect / n;
		if (flcyl[unit] < 0) {
			flx.x_cyl = 0;
			c[0] = FLC_RECAL;
			c[1] = unit;
			if (flcmd(c, 2) < 0)
				goto dead;
			flx.x_state = FL_RECAL;
			flx.x_wait = FLLATE;
			return;
		}
		if (flcyl[unit] != flx.x_cyl) {
			c[0] = FLC_SEEK;
			c[1] = unit;
			c[2] = flx.x_cyl;
			if (flcmd(c, 3) < 0)
				goto dead;
			flx.x_state = FL_SEEK;
			flx.x_wait = FLLATE;
			return;
		}
		s = flx.x_sect % n;
		head = s / tp->ft_sects;
		s %= tp->ft_sects;
		if (bp == &flfbuf) {
			/* What each sector of the track is to call itself. */
			for (i = 0; i < tp->ft_sects; i++) {
				flbp[i * 4] = flx.x_cyl;
				flbp[i * 4 + 1] = head;
				flbp[i * 4 + 2] = i + 1;
				flbp[i * 4 + 3] = 2;	/* 512 bytes */
			}
			fldma(DMAM_WRITE, flbpa, tp->ft_sects * 4);
			c[0] = FLC_FORMAT;
			c[1] = head << 2 | unit;
			c[2] = 2;
			c[3] = tp->ft_sects;
			c[4] = tp->ft_fgap;
			c[5] = 0xF6;		/* what fills them */
			n = 6;
		} else {
			flx.x_now = flx.x_left;
			if (flx.x_now > tp->ft_sects - s)
				flx.x_now = tp->ft_sects - s;
			room = 0x10000L - (flx.x_addr & 0xFFFF);
			flx.x_bounce = room < 512;
			if (flx.x_bounce) {
				flx.x_now = 1;
				if ((bp->b_flags & B_READ) == 0)
					pcopy(flx.x_addr, flbpa, 512);
			} else if ((long)flx.x_now << 9 > room)
				flx.x_now = room >> 9;
			fldma(bp->b_flags & B_READ ? DMAM_READ : DMAM_WRITE,
			    flx.x_bounce ? flbpa : flx.x_addr, flx.x_now << 9);
			c[0] = bp->b_flags & B_READ ? FLC_READ : FLC_WRITE;
			c[1] = head << 2 | unit;
			c[2] = flx.x_cyl;
			c[3] = head;
			c[4] = s + 1;
			c[5] = 2;		/* 512 bytes */
			c[6] = tp->ft_sects;
			c[7] = tp->ft_gap;
			c[8] = 0xFF;
			n = 9;
		}
		if (flcmd(c, n) < 0)
			goto dead;
		flx.x_state = FL_XFER;
		flx.x_wait = FLLATE;
		return;
	dead:
		flx.x_reset = 1;
		flx.x_late = 1;
		flx.x_tries = FLTRIES;
	}
	flx.x_state = FL_IDLE;
}

flstrategy(bp)
	register struct buf *bp;
{
	register struct fltype *tp = flwhat(bp->b_dev);
	register int s;

	if (tp == NULL) {
		bp->b_error = ENXIO;
		goto bad;
	}
	if ((bp->b_bcount & 511) || bp->b_blkno < 0 ||
	    bp->b_blkno > tp->ft_size) {	/* whole sectors only */
		bp->b_error = EINVAL;
		goto bad;
	}
	if (bp->b_blkno == tp->ft_size || bp->b_bcount == 0) {
		bp->b_resid = bp->b_bcount;	/* at the diskette's end */
		goto done;
	}
	bp->b_cylin = (int)bp->b_blkno / (tp->ft_sects * FL_HEADS);
	s = splbio();
	disksort(&fltab, bp);
	if (flx.x_state == FL_IDLE)
		flgo();
	splx(s);
	return;
bad:
	bp->b_flags |= B_ERROR;
	bp->b_resid = bp->b_bcount;
done:
	iodone(bp);
}

/*
 * Format the diskette: each track in turn, through the queue.
 */
flioctl(dev, cmd, data, flag)
	dev_t dev;
	u_int cmd;
	caddr_t data;
	int flag;
{
	register struct buf *bp = &flfbuf;
	register struct fltype *tp = flwhat(dev);
	int track, s, error = 0;

	if (cmd != FLIOC_FORMAT || tp == NULL)
		return (ENXIO);
	if ((flag & FWRITE) == 0)
		return (EBADF);
	s = splbio();
	while (bp->b_flags & B_BUSY) {
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO);
	}
	bp->b_flags = B_BUSY;
	splx(s);
	for (track = 0; track < FL_CYLS * FL_HEADS && error == 0; track++) {
		bp->b_flags &= ~(B_DONE | B_ERROR);
		bp->b_dev = dev;
		bp->b_blkno = track * tp->ft_sects;
		bp->b_bcount = 0;
		bp->b_error = 0;
		bp->b_cylin = track / FL_HEADS;
		s = splbio();
		disksort(&fltab, bp);
		if (flx.x_state == FL_IDLE)
			flgo();
		splx(s);
		iowait(bp);
		error = geterror(bp);
	}
	if (bp->b_flags & B_WANTED)
		wakeup((caddr_t)bp);
	bp->b_flags = 0;
	return (error);
}

/*
 * The controller has done what it was asked, or has failed to.
 */
static
flintr(arg, tf)
	int arg;
	caddr_t tf;
{
	register struct buf *bp = fltab.b_actf;
	register int unit;
	int st0, cyl, i;
	u_char c[3];

	if (bp)
		unit = dkunit(bp->b_dev);
	switch (flx.x_state) {
	case FL_RESET:
		/* It has one to say of each drive it might have had. */
		for (i = 0; i < 4; i++)
			(void) flsense(&st0, &cyl);
		c[0] = FLC_SPECIFY;
		c[1] = 0xDF;		/* 3 ms a step at high density */
		c[2] = 0x02;		/* 2 ms for the head; DMA */
		if (flcmd(c, 3) == 0)
			flx.x_reset = 0;
		else {
			flx.x_late = 1;
			flx.x_tries = FLTRIES;
		}
		break;
	case FL_RECAL:
	case FL_SEEK:
		if (flsense(&st0, &cyl) < 0 ||
		    (st0 & (ST0_BAD | ST0_SEEKEND)) != ST0_SEEKEND ||
		    cyl != flx.x_cyl) {
			flst[0] = st0;
			flst[1] = 0;
			flcyl[unit] = -1;
			flx.x_tries++;
			break;
		}
		flcyl[unit] = cyl;
		/* The head wants 15 ms to be still. */
		flx.x_state = FL_WAIT;
		timeout(flwake, (caddr_t)0, hz / 50 + 1);
		return;
	case FL_XFER:
		for (i = 0; i < 7; i++) {
			if ((cyl = flin()) < 0)
				break;
			flst[i] = cyl;
		}
		if (i < 7) {
			flx.x_reset = 1;
			flx.x_tries++;
			break;
		}
		if (flst[0] & ST0_BAD) {
			if (flst[1] & ST1_PROTECTED)
				flx.x_tries = FLTRIES;
			else if ((++flx.x_tries & 1) == 0)
				flcyl[unit] = -1;	/* is it where it says? */
			break;
		}
		if (bp == &flfbuf)
			flx.x_left = 0;
		else {
			if (flx.x_bounce && (bp->b_flags & B_READ))
				pcopy(flbpa, flx.x_addr, 512);
			flx.x_sect += flx.x_now;
			flx.x_addr += (long)flx.x_now << 9;
			flx.x_left -= flx.x_now;
		}
		flx.x_tries = 0;
		if (flx.x_left == 0)
			fldone(bp);
		break;
	default:
		(void) flsense(&st0, &cyl);	/* nothing was asked of it */
		return;
	}
	flgo();
}

/*
 * The motor has had time to start, or the head to settle.
 */
static
flwake()
{
	register int s = splbio();

	if (flx.x_state == FL_WAIT) {
		flx.x_state = FL_IDLE;
		flgo();
	}
	splx(s);
}

/*
 * Each second while there is anything to do, and until the motor is
 * stopped after: an interrupt that has not come in FLLATE seconds will
 * not, as when the drive has no diskette in it.
 */
static
flwatch()
{
	register int s = splbio();

	switch (flx.x_state) {
	case FL_IDLE:
		if (++flidle >= FLIDLE) {
			outb(FL_DOR, FLD_RUN | FLD_DMA);
			flmotor = -1;
			flwatching = 0;
			splx(s);
			return;
		}
		break;
	case FL_WAIT:
		break;
	default:
		if (--flx.x_wait > 0)
			break;
		flx.x_reset = 1;
		flx.x_late = 1;
		flx.x_tries = FLTRIES;
		flx.x_state = FL_IDLE;
		flgo();
	}
	timeout(flwatch, (caddr_t)0, hz);
	splx(s);
}
#endif /* NFL */
