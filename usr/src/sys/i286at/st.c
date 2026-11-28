/*
 * SCSI tapes on the Adaptec host adapter (aha.c), as mtio(4) has
 * upstream's tapes: a read or a write is a record, files end at tape
 * marks, and ioctl moves the tape.
 *
 * The bus is looked over when a tape is first opened, and each
 * sequential access device found, at unit 0 of its target, becomes the
 * next drive, in order of SCSI id.  Any SCSI tape drive will do: the
 * driver asks a drive what it can do, not which it is.
 *
 * A minor device number is the drive in its low two bits, with 4 for a
 * tape that is not rewound when it is closed, as upstream's drives
 * have it.  8, which picks the higher of two densities there, does
 * nothing here: a drive writes at the density it chooses for the tape,
 * and reads whatever it finds.  There is no block device; a tape is
 * read and written through the raw one.
 *
 * A drive whose least record is a byte, as a DAT is, has its records
 * the size they were written: a write is one record, and a read takes
 * one, as a nine-track tape's are (variable).  Any other, as a QIC
 * drive, has them all one size, 512 bytes for QIC: a read or a write is
 * of whole blocks, as many as it asks for (fixed).
 */
#include "st.h"
#if NST > 0
#include "param.h"
#include "systm.h"
#include "kernel.h"
#include "user.h"
#include "buf.h"
#include "conf.h"
#include "file.h"
#include "ioctl.h"
#include "mtio.h"
#include "ahareg.h"
#include "streg.h"

#define	STUNIT(dev)	(minor(dev) & 3)
#define	T_NOREWIND	4

#define	STTIME		900	/* seconds a command may take, */
#define	STLONG		14400	/*   and one that may move the whole tape */
#define	STWAIT		60	/* a drive loading a tape is waited for */

struct st {
	int	st_target;
	char	st_open;
	char	st_fixed;	/* records are blocks of st_bsize */
	char	st_written;	/* the last that was done was a write */
	char	st_mark;	/* a tape mark was read with blocks before
				   it: the next read is the file's end */
	char	st_eom;		/* past the warning near the end of the
				   tape: writes are refused, */
	char	st_ieot;	/*   unless MTIOCIEOT has them let be */
	char	st_wrlock;	/* the tape is write protected */
	u_int	st_bsize;
	struct	tty *st_ttyp;	/* the opener's, for errors */
	/* What the last command came to, from its sense data. */
	u_char	st_sense;	/* the third byte: SF_ bits and the key */
	u_char	st_asc;		/* additional sense code */
	long	st_info;	/* information: what was not done */
	/* A command other than a read or a write. */
	struct	buf st_cbuf;
	u_char	st_cdb[6];
	char	st_cdir;	/* CCB_NODATA, CCB_DATAIN or CCB_DATAOUT */
	u_char	st_clen;	/* bytes of st_data it moves */
	int	st_csecs;	/* seconds it may take */
	u_char	st_data[12];
} st[NST];
int	nst;			/* how many were found */
static char stlooked;		/* 1 looking, 2 have looked */

/*
 * The queue of the drives' transfers and commands, which go out one at
 * a time, in turn, in the tapes' CCB.
 */
struct buf sttab;
static struct ccb stccb;

int	ststrategy();
static int stintr();

static long
get3(p)
	register u_char *p;
{
	return ((long)p[0] << 16 | (u_int)(p[1] << 8) | p[2]);
}

static long
get4(p)
	register u_char *p;
{
	return ((long)p[0] << 24 | (long)p[1] << 16 | (u_int)(p[2] << 8) | p[3]);
}

/*
 * What is at a target?  If a tape drive, it is the next drive.  Called
 * at splbio.
 */
static
stprobe(target)
	int target;
{
	register int i;
	char name[32];

	if (ahainquiry(&stccb, target, name, "st") != 1)
		return;
	if (nst >= NST) {
		printf("st: id %d: %s: too many tapes\n", target, name);
		return;
	}
	st[nst].st_target = target;
	printf("st%d: %s, id %d", nst, name, target);
	if ((i = ahasync(target)) > 0)
		printf(", sync %d.%d MB/s", i / 10, i % 10);
	else if (i == 0)
		printf(", async");
	printf("\n");
	nst++;
}

/*
 * Send a drive a command other than a read or a write, and wait for it:
 * op, then b1, then count in the three bytes after it, as a negative
 * number for a move backward; len bytes of st_data go with it, in the
 * direction dir, and it may take secs seconds.  Returns 0 or an error
 * number, and what it came to is in st_sense and the rest.
 */
static int
stcmd(dev, op, b1, count, dir, len, secs)
	dev_t dev;
	int op, b1, dir, len, secs;
	long count;
{
	register struct st *sp = &st[STUNIT(dev)];
	register struct buf *bp = &sp->st_cbuf;
	register int s;
	int error;

	s = splbio();
	while (bp->b_flags & B_BUSY) {
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO);
	}
	bp->b_flags = B_BUSY;
	splx(s);
	sp->st_cdb[0] = op;
	sp->st_cdb[1] = b1;
	sp->st_cdb[2] = count >> 16;
	sp->st_cdb[3] = count >> 8;
	sp->st_cdb[4] = count;
	sp->st_cdb[5] = 0;
	sp->st_cdir = dir;
	sp->st_clen = len;
	sp->st_csecs = secs;
	if (dir == CCB_DATAIN)
		bzero(sp->st_data, sizeof (sp->st_data));
	bp->b_dev = dev;
	bp->b_bcount = len;
	ststrategy(bp);
	s = splbio();
	while ((bp->b_flags & B_DONE) == 0)
		sleep((caddr_t)bp, PRIBIO);
	splx(s);
	error = geterror(bp);
	if (bp->b_flags & B_WANTED)
		wakeup((caddr_t)bp);
	bp->b_flags &= ~(B_BUSY|B_WANTED);
	return (error);
}

/*
 * Is the drive ready, with a tape in it?  A drive just switched on or
 * reset, or given a tape, says so once (unit attention), and is asked
 * again; one loading a tape is waited for, a second at a time, up to
 * STWAIT, or until a signal.
 */
static int
stready(dev)
	dev_t dev;
{
	register struct st *sp = &st[STUNIT(dev)];
	register int i, key = 0;
	int error;

	for (i = 0; i < STWAIT; i++) {
		if (stcmd(dev, SC_TUR, 0, 0L, CCB_NODATA, 0, STTIME) == 0)
			return (0);
		key = sp->st_sense & 0x0F;
		if (key == SK_ATTENTION)
			continue;
		if (key != SK_NOTREADY || sp->st_asc == SA_NOTAPE)
			break;
		error = tsleep((caddr_t)&sp->st_cdir, (PZERO + 1) | PCATCH, hz);
		if (error != EWOULDBLOCK)
			return (error);
	}
	if (key == SK_NOTREADY && sp->st_asc == SA_NOTAPE)
		uprintf("st%d: no tape\n", STUNIT(dev));
	else
		uprintf("st%d: not ready, sense key %d, code %x\n",
		    STUNIT(dev), key, sp->st_asc);
	return (EIO);
}

/*
 * How the drive records, from its block limits and its mode: in records
 * of any length if its least is a byte, to which it is set if it is
 * not; and otherwise in blocks of the size it is set to, or of its
 * least if that is not between its limits.  A drive that gives no
 * limits is left as it is set, and one that gives neither them nor
 * its size is taken to write records of any length.  And whether the
 * tape is write protected, from the mode.
 */
static int
stmode(dev)
	dev_t dev;
{
	register struct st *sp = &st[STUNIT(dev)];
	register u_char *d = sp->st_data;
	long want, have, min, max;
	int error;

	min = -1;
	if (stcmd(dev, SC_LIMITS, 0, 0L, CCB_DATAIN, 6, STTIME) == 0) {
		max = get3(d + 1);
		min = (u_int)(d[4] << 8 | d[5]);
	}
	if (error = stcmd(dev, SC_MSENSE, 0, 12L, CCB_DATAIN, 12, STTIME))
		return (error);
	sp->st_wrlock = (d[2] & 0x80) != 0;
	have = d[3] >= 8 ? get3(d + 9) : -1;
	if (min >= 0 && min <= 1)
		want = 0;
	else if (min > 1 && (have < min || max && have > max))
		want = min;
	else
		want = have >= 0 ? have : 0;
	if (want != have) {
		/* The mode as it was, the density with it, but the size. */
		d[0] = d[1] = 0;
		d[2] &= 0x7F;
		if (d[3] < 8)
			d[4] = 0;
		d[3] = 8;
		d[5] = d[6] = d[7] = d[8] = 0;
		d[9] = want >> 16;
		d[10] = want >> 8;
		d[11] = want;
		if (stcmd(dev, SC_MSELECT, 0, 12L, CCB_DATAOUT, 12, STTIME) == 0 ||
		    have < 0)
			have = want;
	}
	if (have > 32768L) {
		uprintf("st%d: blocks of %D bytes unsupported\n", STUNIT(dev),
		    have);
		return (EIO);
	}
	sp->st_fixed = have != 0;
	sp->st_bsize = have;
	return (0);
}

stopen(dev, flag)
	dev_t dev;
	int flag;
{
	register int unit = STUNIT(dev);
	register struct st *sp;
	register int s, t;
	int error;

	while (stlooked == 1)
		sleep((caddr_t)&stlooked, PRIBIO);
	if (stlooked == 0) {
		stlooked = 1;
		s = splbio();
		if (ahaattach() == 0)
			for (t = 0; t < 8; t++)
				if (t != ahaid())
					stprobe(t);
		splx(s);
		stlooked = 2;
		wakeup((caddr_t)&stlooked);
	}
	if (unit >= nst)
		return (ENXIO);
	sp = &st[unit];
	if (sp->st_open)
		return (EBUSY);
	sp->st_open = 1;
	sp->st_ttyp = u.u_ttyp;
	if ((error = stready(dev)) == 0 && (error = stmode(dev)) == 0 &&
	    (flag & FWRITE) && sp->st_wrlock) {
		uprintf("st%d: write protected\n", unit);
		error = EIO;
	}
	if (error) {
		sp->st_open = 0;
		return (error);
	}
	sp->st_written = sp->st_mark = sp->st_eom = sp->st_ieot = 0;
	return (0);
}

/*
 * A tape that was written ends with a tape mark, and one opened only to
 * write has one if nothing was; then it is rewound, unless its minor
 * number says not.  Upstream's drivers write two marks, a nine-track
 * tape's end, and go back over the second; a QIC drive writes only at
 * the end of what is on the tape, and a drive says where that is when
 * it is read past (SK_BLANK), which is a file's end too.
 */
stclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct st *sp = &st[STUNIT(dev)];

	if (flag == FWRITE || (flag & FWRITE) && sp->st_written)
		(void) stcmd(dev, SC_WEOF, 0, 1L, CCB_NODATA, 0, STLONG);
	if ((minor(dev) & T_NOREWIND) == 0)
		(void) stcmd(dev, SC_REWIND, 0, 0L, CCB_NODATA, 0, STLONG);
	sp->st_open = 0;
	return (0);
}

stioctl(dev, cmd, data, flag)
	dev_t dev;
	u_int cmd;
	caddr_t data;
	int flag;
{
	register struct st *sp = &st[STUNIT(dev)];
	register struct mtget *mtget;
	long n;
	int op, error;

	switch (cmd) {
	case MTIOCTOP:
		op = ((struct mtop *)data)->mt_op;
		n = ((struct mtop *)data)->mt_count;
		if (op < MTWEOF || op > MTFLUSH || op == MTCACHE ||
		    op == MTNOCACHE)
			return (ENXIO);
		if (op <= MTBSR && (n <= 0 || n > 0x7FFFFFL))
			return (EINVAL);
		/*
		 * A tape mark the drive has read past, but which has not
		 * been given as a file's end yet: the tape is before it,
		 * as far as the reader knows.
		 */
		if (sp->st_mark) {
			sp->st_mark = 0;
			if (op == MTFSF && --n == 0)
				return (0);
			if (op == MTBSF || op == MTFSR || op == MTBSR) {
				error = stcmd(dev, SC_SPACE, SP_MARKS, -1L,
				    CCB_NODATA, 0, STLONG);
				if (error)
					return (error);
			}
		}
		sp->st_written = 0;
		switch (op) {
		case MTWEOF:
			error = stcmd(dev, SC_WEOF, 0, n, CCB_NODATA, 0,
			    STLONG);
			break;
		case MTFSF:
		case MTBSF:
			error = stcmd(dev, SC_SPACE, SP_MARKS,
			    op == MTFSF ? n : -n, CCB_NODATA, 0, STLONG);
			break;
		case MTFSR:
		case MTBSR:
			error = stcmd(dev, SC_SPACE, SP_BLOCKS,
			    op == MTFSR ? n : -n, CCB_NODATA, 0, STLONG);
			break;
		case MTREW:
			error = stcmd(dev, SC_REWIND, 0, 0L, CCB_NODATA, 0,
			    STLONG);
			break;
		case MTOFFL:
			error = stcmd(dev, SC_LOAD, 0, 0L, CCB_NODATA, 0,
			    STLONG);
			break;
		case MTNOP:
			error = 0;
			break;
		case MTFLUSH:		/* no marks: what is buffered */
			error = stcmd(dev, SC_WEOF, 0, 0L, CCB_NODATA, 0,
			    STLONG);
			break;
		default:
			return (ENXIO);
		}
		if (op == MTREW || op == MTOFFL || op == MTBSF || op == MTBSR)
			sp->st_eom = 0;
		return (error);
	case MTIOCGET:
		mtget = (struct mtget *)data;
		mtget->mt_type = MT_ISST;
		mtget->mt_dsreg = ((sp->st_eom ? MTF_EOM : 0) |
		    (sp->st_wrlock ? MTF_WRTLCK : 0) |
		    (sp->st_written ? MTF_WRITTEN : 0)) << 8;
		mtget->mt_erreg = sp->st_asc << 8 | sp->st_sense;
		mtget->mt_resid = sp->st_info;
		mtget->mt_fileno = mtget->mt_blkno = 0;
		return (0);
	case MTIOCIEOT:
		sp->st_ieot = 1;
		return (0);
	case MTIOCEEOT:
		sp->st_ieot = 0;
		return (0);
	}
	return (ENXIO);
}

ststrategy(bp)
	register struct buf *bp;
{
	register struct st *sp = &st[STUNIT(bp->b_dev)];
	register int s;

	if (bp != &sp->st_cbuf) {
		if (sp->st_fixed && bp->b_bcount % sp->st_bsize) {
			bp->b_error = EINVAL;
			goto bad;
		}
		if (bp->b_flags & B_READ) {
			sp->st_written = 0;
			if (sp->st_mark) {	/* the file's end */
				sp->st_mark = 0;
				bp->b_resid = bp->b_bcount;
				iodone(bp);
				return;
			}
		} else {
			sp->st_written = 1;
			if (sp->st_eom && !sp->st_ieot) {
				bp->b_error = ENOSPC;
				goto bad;
			}
		}
	}
	bp->av_forw = NULL;
	s = splbio();
	if (sttab.b_actf == NULL)
		sttab.b_actf = bp;
	else
		sttab.b_actl->av_forw = bp;
	sttab.b_actl = bp;
	if (sttab.b_active == 0)
		ststart();
	splx(s);
	return;
bad:
	bp->b_flags |= B_ERROR;
	bp->b_resid = bp->b_bcount;
	iodone(bp);
}

/*
 * Send the command for what is at the head of the queue.  A read or a
 * write is straight to or from where its data is, and has the adapter
 * give back what it did not move if it can, which is how short a record
 * was that a drive read without saying so.  Called at splbio.
 */
static
ststart()
{
	register struct buf *bp;
	register struct st *sp;
	register struct ccb *cp = &stccb;
	long n;

	if ((bp = sttab.b_actf) == NULL)
		return;
	sttab.b_active = 1;
	sp = &st[STUNIT(bp->b_dev)];
	if (bp == &sp->st_cbuf) {
		ahafill(cp, sp->st_target, sp->st_cdir, 6,
		    fartophys(sp->st_data), sp->st_clen);
		bcopy(sp->st_cdb, cp->cc_cdb, 6);
		ahago(cp, stintr, sp->st_csecs);
		return;
	}
	ahafill(cp, sp->st_target, bp->b_flags & B_READ ? CCB_DATAIN :
	    CCB_DATAOUT, 6, bfpaddr(bp), bp->b_bcount);
	if (aharesid())
		cp->cc_op = CCB_RESID;
	n = bp->b_bcount;
	if (sp->st_fixed) {
		cp->cc_cdb[1] = SC_FIXED;
		n /= sp->st_bsize;
	}
	cp->cc_cdb[0] = bp->b_flags & B_READ ? SC_READ : SC_WRITE;
	cp->cc_cdb[2] = n >> 16;
	cp->cc_cdb[3] = n >> 8;
	cp->cc_cdb[4] = n;
	ahago(cp, stintr, STTIME);
}

/*
 * A read or a write that failed: the opener is told, and the log.
 */
static
sterr(bp, how)
	register struct buf *bp;
	int how;
{
	register struct st *sp = &st[STUNIT(bp->b_dev)];
	register struct ccb *cp = &stccb;
	char *what = bp->b_flags & B_READ ? "read" : "write";

	if (how == AHA_LOST || how == AHA_DEAD)
		tprintf(sp->st_ttyp, "st%d: %s: %s\n", STUNIT(bp->b_dev), what,
		    how == AHA_DEAD ? "adapter dead" : "timeout");
	else if (how == MBI_ERROR && cp->cc_tarstat == SS_CHECK)
		tprintf(sp->st_ttyp, "st%d: %s: sense key %d, code %x\n",
		    STUNIT(bp->b_dev), what, sp->st_sense & 0x0F, sp->st_asc);
	else
		tprintf(sp->st_ttyp, "st%d: %s: mailbox %d, adapter %x, target %x\n",
		    STUNIT(bp->b_dev), what, how, cp->cc_hastat,
		    cp->cc_tarstat);
	bp->b_flags |= B_ERROR;
	bp->b_error = EIO;
	bp->b_resid = bp->b_bcount;
}

/*
 * What a read or a write that stopped short did not move, from the
 * sense data's information, in blocks or bytes: all of it if the drive
 * does not say.
 */
static u_int
stleft(bp, valid)
	register struct buf *bp;
	int valid;
{
	register struct st *sp = &st[STUNIT(bp->b_dev)];
	long n;

	if (!valid)
		return (bp->b_bcount);
	n = sp->st_fixed ? sp->st_info * sp->st_bsize : sp->st_info;
	return (n < 0 || n > bp->b_bcount ? bp->b_bcount : (u_int)n);
}

/*
 * The command that is out is over: how is what ahago says of it.  A
 * read or a write that stopped short, at a tape mark, at the end of what
 * is written, at the end of the tape, or with a record shorter than the
 * read, says what it did not do in its sense data.  A drive that reads
 * a short record and says nothing of it, as 86Box's do, leaves only the
 * adapter's count of what it did not move (CCB_RESID) to say; an
 * adapter that gives that back only when there is some leaves the whole
 * length there when there was none, and 86Box's leaves 0 there with
 * sense data, which is why the count is taken only without.  A command
 * that is not a read or a write succeeds or fails, and its caller looks
 * at st_sense.
 */
static
stintr(how)
	int how;
{
	register struct buf *bp = sttab.b_actf;
	register struct st *sp;
	register struct ccb *cp = &stccb;
	u_char *sn;
	long left;
	int key, sense, valid, ok;

	if (bp == NULL || sttab.b_active == 0)
		return;
	sttab.b_actf = bp->av_forw;
	sttab.b_active = 0;
	sp = &st[STUNIT(bp->b_dev)];
	sp->st_sense = sp->st_asc = 0;
	sp->st_info = 0;
	left = 0;
	if (cp->cc_op == CCB_RESID) {
		left = get3(cp->cc_datalen);
		if (how == MBI_OK && left >= bp->b_bcount)
			left = 0;
	}
	sense = how == MBI_ERROR && cp->cc_tarstat == SS_CHECK &&
	    (cp->cc_hastat == HS_OK || cp->cc_hastat == HS_DATARUN);
	valid = 0;
	if (sense) {
		sn = &cp->cc_cdb[cp->cc_cdblen];
		sp->st_sense = sn[2];
		sp->st_asc = sn[12];
		if (valid = sn[0] & 0x80)
			sp->st_info = get4(sn + 3);
	}
	key = sp->st_sense & 0x0F;
	ok = how == MBI_OK || sense && key <= SK_RECOVERED &&
	    (sp->st_sense & (SF_MARK|SF_EOM|SF_ILI)) == 0;
	/* Fewer bytes than asked for, with nothing said of it. */
	if (how == MBI_ERROR && cp->cc_hastat == HS_DATARUN &&
	    cp->cc_tarstat == SS_GOOD && (bp == &sp->st_cbuf || left > 0))
		ok = 1;
	bp->b_resid = 0;
	if (bp == &sp->st_cbuf) {
		if (!ok) {
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
		}
	} else if (ok)
		bp->b_resid = sense ? 0 : left;
	else if (!sense || key > SK_RECOVERED && key != SK_BLANK &&
	    key != SK_OVERFLOW)
		sterr(bp, how);
	else if (bp->b_flags & B_READ) {
		if (sp->st_sense & SF_ILI && (sp->st_fixed || sp->st_info < 0)) {
			if (sp->st_fixed)
				tprintf(sp->st_ttyp, "st%d: a block not of %u bytes\n",
				    STUNIT(bp->b_dev), sp->st_bsize);
			else
				tprintf(sp->st_ttyp, "st%d: a record of %D bytes, read with %u\n",
				    STUNIT(bp->b_dev),
				    bp->b_bcount - sp->st_info, bp->b_bcount);
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			bp->b_resid = bp->b_bcount;
		} else if (!sp->st_fixed && !(sp->st_sense & SF_ILI))
			bp->b_resid = bp->b_bcount;	/* a mark, or the end */
		else {
			bp->b_resid = stleft(bp, valid);
			if ((sp->st_sense & SF_MARK || key == SK_BLANK) &&
			    bp->b_resid < bp->b_bcount)
				sp->st_mark = 1;
		}
	} else {
		/*
		 * Written as far as the warning near the end of the tape,
		 * or not written, there being no room: later writes are
		 * refused, and there is room for the tape mark.
		 */
		sp->st_eom = 1;
		if (key == SK_OVERFLOW) {
			bp->b_flags |= B_ERROR;
			bp->b_error = ENOSPC;
			bp->b_resid = bp->b_bcount;
		} else
			bp->b_resid = valid ? stleft(bp, valid) : 0;
	}
	iodone(bp);
	ststart();
}
#endif /* NST */
