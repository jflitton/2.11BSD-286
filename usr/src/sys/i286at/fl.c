/*
 * Diskettes on the PC/AT's diskette controller: an NEC 765 at 0x3F0,
 * with interrupt request 6 and DMA channel 2, and its two drives.  As
 * rx(4) has the RX02's, a diskette has no partitions; the minor device
 * number is the drive.  What a drive is, the machine's setup program
 * wrote in the clock chip's memory: a 3.5 inch drive takes diskettes of
 * 1.44 MB and 720K, and a 5.25 inch one those of 1.2 MB.
 *
 * What the diskette in a drive is, the driver finds when the drive is
 * first used after it was last closed: it reads the header of a sector
 * at the rate of each kind the drive takes, the one of most sectors
 * first, until one is read.  A diskette that is formatted is said to be
 * of one kind or the other, and a request for a label (DIOCGDINFO) is
 * answered with one for the kind found.
 */
#include "fl.h"
#if NFL > 0
#include "param.h"
#include "systm.h"
#include "user.h"
#include "buf.h"
#include "conf.h"
#include "file.h"
#include "stat.h"
#include "ioctl.h"
#include "disklabel.h"
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
#define	FLC_READID	0x4A	/* the header of whatever sector comes */
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
	char	*ft_type;	/* what disktab calls it */
} fltypes[] = {
	{ 1440, 9, 2, 0x2A, 0x50, "720K", "fl720" },
	{ 2400, 15, 0, 0x1B, 0x54, "1.2 MB", "fl1200" },
	{ 2880, 18, 0, 0x1B, 0x6C, "1.44 MB", "fl1440" },
};

/*
 * The kinds of diskette each kind of drive takes, as fltypes[] has them,
 * the one of most sectors first: RTCD_1200, RTCD_720 and RTCD_1440.
 */
static int flkinds[3][2] = { { 1, -1 }, { 0, -1 }, { 2, 0 } };

#define	flunit(dev)	minor(dev)

static char fldrive[NFL];	/* what each drive is: RTCD_..., */
static struct fltype *flfound[NFL];	/* what its diskette was found
					   to be, or NULL, */
static char flopens[NFL];	/* and how it is open: */
#define	FLO_BLK		1	/*   as a block device, */
#define	FLO_CHR		2	/*   a character one */
static struct fltype *flftype;	/* what flfbuf formats a diskette as */
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
	struct fltype *x_kind;	/* what a sector's header is read as; NULL
				   if it is not a header that is read */
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
 * The i'th kind of diskette the drive takes, or NULL if it takes no
 * more, or is not there.
 */
static struct fltype *
fltry(unit, i)
	int unit, i;
{
	register int d = fldrive[unit] - RTCD_1200;

	if (d < 0 || i > 1 || flkinds[d][i] < 0)
		return (NULL);
	return (&fltypes[flkinds[d][i]]);
}

/*
 * What the diskette in the drive is, or until that is found, the most
 * it may be; NULL if there is no such drive.
 */
static struct fltype *
flkind(unit)
	register int unit;
{
	if (unit >= NFL)
		return (NULL);
	return (flfound[unit] ? flfound[unit] : fltry(unit, 0));
}

flopen(dev, flag, mode)
	dev_t dev;
	int flag, mode;
{
	static char looked;
	register int unit = flunit(dev), i, t;

	if (!looked) {
		looked = 1;
		if (inb(FL_STATUS) == 0xFF)
			return (ENXIO);		/* no controller */
		outb(RTC_ADDR, RTC_DRIVES);
		t = inb(RTC_DATA);
		fldrive[0] = t >> 4 & 15;
		fldrive[1] = t & 15;
		for (i = 0; i < NFL; i++) {
			if (fldrive[i] > RTCD_1440)	/* 2.88 MB */
				fldrive[i] = RTCD_1440;
			flcyl[i] = -1;
			if (fldrive[i] >= RTCD_1200)
				printf("fl%d: %s\n", i,
				    fltypes[fldrive[i] == RTCD_720 ? 0 :
				    fldrive[i] == RTCD_1200 ? 1 : 2].ft_name);
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
	if (flkind(unit) == NULL)
		return (ENXIO);
	flopens[unit] |= mode == S_IFBLK ? FLO_BLK : FLO_CHR;
	return (0);
}

/*
 * The last of the drive's opens as one or the other is closed.  Open as
 * neither, the next diskette in it may be of another kind.  A transfer
 * still to be done finds again what this one is.
 */
flclose(dev, flag, mode)
	dev_t dev;
	int flag, mode;
{
	register int unit = flunit(dev), s;

	s = splbio();
	flopens[unit] &= ~(mode == S_IFBLK ? FLO_BLK : FLO_CHR);
	if (flopens[unit] == 0)
		flfound[unit] = NULL;
	splx(s);
	return (0);
}

daddr_t
flsize(dev)
	dev_t dev;
{
	register struct fltype *tp = flkind(flunit(dev));

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
 * the motor, find cylinder 0, move the head, read a sector's header to
 * find what the diskette is, or move sectors, as many as are left on
 * the track and before a multiple of 64K in memory.  When it has been
 * tried FLTRIES times with nothing moved, report it and go on to the
 * next.  Called at splbio.
 */
static
flgo()
{
	register struct buf *bp;
	register struct fltype *tp;
	int unit, head, s, n, i, finding;
	long room;
	u_char c[9];

	while ((bp = fltab.b_actf) != NULL) {
		unit = flunit(bp->b_dev);
		if (fltab.b_active == 0) {
			fltab.b_active = 1;
			flx.x_sect = bp->b_blkno;
			flx.x_addr = bfpaddr(bp);
			flx.x_left = bp == &flfbuf ? 1 : bp->b_bcount >> 9;
			flx.x_tries = 0;
			flx.x_late = 0;
		}
		/*
		 * Until what the diskette is is found, each kind the drive
		 * takes is tried in turn.  Found, it may be less than the
		 * transfer was let have.
		 */
		finding = 0;
		if (bp == &flfbuf)
			tp = flftype;
		else if ((tp = flfound[unit]) == NULL) {
			finding = 1;
			if ((tp = fltry(unit, flx.x_tries & 1)) == NULL)
				tp = fltry(unit, 0);
		} else if (flx.x_sect + flx.x_left > tp->ft_size) {
			if (flx.x_sect >= tp->ft_size) {
				if (flx.x_sect > tp->ft_size) {
					bp->b_error = EINVAL;
					bp->b_flags |= B_ERROR;
				}
				fldone(bp);		/* at the end */
				continue;
			}
			flx.x_left = tp->ft_size - flx.x_sect;
		}
		if (flx.x_tries >= FLTRIES) {
			if (flx.x_late) {
				printf("fl%d: not ready\n", unit);
				bp->b_error = ENXIO;
			} else if (finding) {
				printf("fl%d: format unknown st0=%b st1=%b\n",
				    unit, flst[0], ST0_BITS, flst[1], ST1_BITS);
				bp->b_error = EIO;
			} else {
				printf("fl%d: hard error sn%D (sector %d) "
				    "st0=%b st1=%b\n", unit, bp->b_blkno,
				    flx.x_sect, flst[0], ST0_BITS,
				    flst[1], ST1_BITS);
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
		flx.x_cyl = finding ? flcyl[unit] : flx.x_sect / n;
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
		flx.x_kind = NULL;
		if (finding) {
			/* Whatever the cylinder, on the first side. */
			flx.x_kind = tp;
			c[0] = FLC_READID;
			c[1] = unit;
			n = 2;
		} else if (bp == &flfbuf) {
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
	register struct fltype *tp = flkind(flunit(bp->b_dev));
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
 * A label for what the diskette is, which it has not of its own: one
 * partition, all of it, as pcdisk.c makes for a disk with none.  What
 * the diskette is is found first, if it is not known, by reading it.
 */
static
fllabel(dev, lp)
	dev_t dev;
	register struct disklabel *lp;
{
	register struct fltype *tp;
	register struct buf *bp;
	struct partition *pi = &lp->d_partitions[0];
	int unit = flunit(dev), i, error;

	if (flfound[unit] == NULL) {
		bp = geteblk();
		bp->b_dev = dev;
		bp->b_blkno = 0;
		bp->b_bcount = 512;
		bp->b_flags = B_READ;
		flstrategy(bp);
		iowait(bp);
		error = geterror(bp);
		brelse(bp);
		if (error)
			return (error);
	}
	if ((tp = flfound[unit]) == NULL)
		return (EIO);
	bzero(lp, sizeof (*lp));
	lp->d_magic = DISKMAGIC;
	lp->d_type = DTYPE_FLOPPY;
	for (i = 0; tp->ft_type[i]; i++)
		lp->d_typename[i] = tp->ft_type[i];
	lp->d_secsize = 512;
	lp->d_nsectors = tp->ft_sects;
	lp->d_ntracks = FL_HEADS;
	lp->d_ncylinders = FL_CYLS;
	lp->d_secpercyl = tp->ft_sects * FL_HEADS;
	lp->d_secperunit = tp->ft_size;
	lp->d_rpm = 300;
	lp->d_interleave = 1;
	lp->d_magic2 = DISKMAGIC;
	lp->d_bbsize = 512;
	lp->d_sbsize = 1024;
	lp->d_npartitions = 1;
	pi->p_size = tp->ft_size;
	pi->p_fstype = FS_V71K;
	pi->p_frag = 1;
	pi->p_fsize = 1024;
	lp->d_checksum = dkcksum(lp);
	return (0);
}

/*
 * Format the diskette as the kind the drive takes that has the
 * kilobytes asked for, or if none are, the one of most sectors: each
 * track in turn, through the queue.  It is then that kind.
 */
flioctl(dev, cmd, data, flag)
	dev_t dev;
	u_int cmd;
	caddr_t data;
	int flag;
{
	register struct buf *bp = &flfbuf;
	register struct fltype *tp;
	int unit = flunit(dev), track, s, i, error = 0;

	if (cmd == DIOCGDINFO)
		return (fllabel(dev, (struct disklabel *)data));
	if (cmd != FLIOC_FORMAT)
		return (ENXIO);
	if ((flag & FWRITE) == 0)
		return (EBADF);
	for (i = 0; (tp = fltry(unit, i)) != NULL; i++)
		if (*(int *)data == 0 || *(int *)data * 2 == tp->ft_size)
			break;
	if (tp == NULL)
		return (EINVAL);
	s = splbio();
	while (bp->b_flags & B_BUSY) {
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO);
	}
	bp->b_flags = B_BUSY;
	flftype = tp;
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
	s = splbio();
	flfound[unit] = error ? NULL : tp;
	splx(s);
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
		unit = flunit(bp->b_dev);
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
		if (flx.x_kind) {
			flfound[unit] = flx.x_kind;	/* what it is */
			flx.x_tries = 0;
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
