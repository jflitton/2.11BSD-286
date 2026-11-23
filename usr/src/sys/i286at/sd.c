/*
 * SCSI disks on the Adaptec host adapter (aha.c).
 *
 * The bus is looked over when a disk is first opened, and each disk
 * found, a direct access device with 512-byte sectors at unit 0 of its
 * target, becomes the next drive, in order of SCSI id.
 *
 * A minor device number is the drive in bits 3 and up and the partition
 * in the low three.
 */
#include "sd.h"
#if NSD > 0
#include "param.h"
#include "systm.h"
#include "user.h"
#include "buf.h"
#include "conf.h"
#include "disklabel.h"
#include "disk.h"
#include "pcdisk.h"
#include "ahareg.h"
#include "dk.h"

#define	SDTRIES		4	/* times a transfer is tried */

struct sd {
	struct	dkdevice sd_dk;
	daddr_t	sd_start;	/* first sector of this system's part */
	struct	pcgeom sd_geom;
	u_int	sd_spc;		/* sectors per cylinder */
	int	sd_target;
} sd[NSD];
int	nsd;			/* how many were found */
static char sdlooked;		/* 1 looking, 2 have looked */
#ifdef UCB_METER
static	int	sd_dkn = -1;	/* number for iostat */
#endif

struct disklabel __far sdlabel[NSD];

/*
 * The queue of transfers for all the drives, in the order disksort
 * gives them.  b_resid is not a transfer's until it is done: disksort
 * keeps the cylinder there.
 */
struct buf sdtab;
static int sdtries;		/* of the command that is out */

/*
 * Transfers that are waiting together and follow one another on a disk
 * go in one command, each to or from its own place: a command costs
 * the processor and the disk as much for 1K as for 16K, and a file
 * read or written in order is a row of such transfers, a block each.
 * Those of a file being read are put in the queue together, with the
 * disk held back until they all are (breadrun, bioplug).  SDNSEG is
 * the most that go together, and SDMAXLEN the most bytes.
 */
#define	SDNSEG		AHA_NSEG
#define	SDMAXLEN	64		/* sectors */

/*
 * The transfers of the command that is out, in order, and how many.
 * They are off the queue while it is: disksort puts a read after the
 * first write it finds, and what it put among the transfers of a
 * command would be given back with them when the command ended, never
 * having been read.
 */
static struct buf *sdcur;
static int sdn;
static int sdalone;		/* this many more go one to a command: a
				   command of several failed, and which
				   of them it was is to be found */
static u_char sdbuf[36];	/* what a target says of itself */

int	sdstrategy();
static int sdintr();

static long
get4(p)
	register u_char *p;
{
	return ((long)p[0] << 24 | (long)p[1] << 16 | (u_int)(p[2] << 8) | p[3]);
}

/*
 * The sense key a target sent with the CCB, or -1 if it sent none.
 */
static int
sdsense(st)
	int st;
{
	register struct ccb *cp = &ahaccb;

	if (st == MBI_ERROR && cp->cc_hastat == HS_OK &&
	    cp->cc_tarstat == SS_CHECK)
		return (cp->cc_cdb[cp->cc_cdblen + 2] & 0x0F);
	return (-1);
}

/*
 * What is at a target?  If a disk that can be used, it is the next
 * drive.  Called at splbio.
 */
static
sdprobe(target)
	int target;
{
	register struct ccb *cp = &ahaccb;
	register struct sd *sp;
	register int i;
	int st, n, type;
	long size, secsize, cyls;
	char name[30];

	bzero(sdbuf, sizeof (sdbuf));
	ahafill(target, CCB_DATAIN, 6, fartophys(sdbuf), 36);
	cp->cc_cdb[0] = SC_INQUIRY;
	cp->cc_cdb[4] = 36;
	st = ahapoll();
	if (st == MBI_ERROR && cp->cc_hastat == HS_SELTIMEOUT)
		return;			/* nothing there */
	if (st != MBI_OK && !(st == MBI_ERROR && cp->cc_hastat == HS_DATARUN &&
	    cp->cc_tarstat == SS_GOOD)) {
		printf("sd: id %d: inquiry failed (%d, %x, %x)\n",
		    target, st, cp->cc_hastat, cp->cc_tarstat);
		return;
	}
	if (sdbuf[0] & 0xE0)
		return;			/* no unit 0 */
	type = sdbuf[0] & 0x1F;
	/* Maker, product and revision, with the padding squeezed out. */
	for (n = 0, i = 8; i < 36; i++) {
		if (sdbuf[i] <= ' ' || sdbuf[i] > '~' || i == 16 || i == 32) {
			if (n && name[n - 1] != ' ')
				name[n++] = ' ';
			if (sdbuf[i] <= ' ' || sdbuf[i] > '~')
				continue;
		}
		name[n++] = sdbuf[i];
	}
	while (n && name[n - 1] == ' ')
		n--;
	name[n] = 0;
	if (type != 0) {
		printf("sd: id %d: %s, type %d, not a disk\n", target, name,
		    type);
		return;
	}
	/* A target just switched on or reset refuses the first command. */
	for (i = 0; i < SDTRIES; i++) {
		bzero(sdbuf, 8);
		ahafill(target, CCB_DATAIN, 10, fartophys(sdbuf), 8);
		cp->cc_cdb[0] = SC_RDCAP;
		if ((st = ahapoll()) == MBI_OK)
			break;
	}
	if (st != MBI_OK) {
		printf("sd: id %d: %s: read capacity failed (%d, %x, %x, key %d)\n",
		    target, name, st, cp->cc_hastat, cp->cc_tarstat,
		    sdsense(st));
		return;
	}
	size = get4(sdbuf) + 1;
	secsize = get4(sdbuf + 4);
	if (secsize != 512) {
		printf("sd: id %d: %s: %D-byte sectors unsupported\n", target,
		    name, secsize);
		return;
	}
	if (nsd >= NSD) {
		printf("sd: id %d: %s: too many disks\n", target, name);
		return;
	}
	/*
	 * The drive has no cylinders and heads.  The label is given the
	 * ones the adapter's BIOS makes up, which the partition table
	 * and whatever boots the machine count in.
	 */
	sp = &sd[nsd];
	sp->sd_target = target;
	sp->sd_geom.pg_size = size;
	sp->sd_geom.pg_type = DTYPE_SCSI;
	if (ahaxlate() && size > 0x200000L) {
		sp->sd_geom.pg_heads = 255;
		sp->sd_geom.pg_sects = 63;
	} else {
		sp->sd_geom.pg_heads = 64;
		sp->sd_geom.pg_sects = 32;
	}
	sp->sd_spc = sp->sd_geom.pg_heads * sp->sd_geom.pg_sects;
	cyls = size / sp->sd_spc;
	sp->sd_geom.pg_cyls = cyls > 65535L ? 65535 : cyls;
	sp->sd_dk.dk_label = &sdlabel[nsd];
	printf("sd%d: %s, %D MB, id %d", nsd, name, size / 2048, target);
	if ((i = ahasync(target)) > 0)
		printf(", sync %d.%d MB/s", i / 10, i % 10);
	else if (i == 0)
		printf(", async");
	printf("\n");
	nsd++;
#ifdef UCB_METER
	/* how fast it moves words is not known: iostat leaves it out */
	if (sd_dkn < 0)
		dk_alloc(&sd_dkn, NSD, "sd", 0L);
#endif
}

sdopen(dev, flag, mode)
	dev_t dev;
	int flag, mode;
{
	register int unit = dkunit(dev);
	register int s, t;

	while (sdlooked == 1)
		sleep((caddr_t)&sdlooked, PRIBIO);
	if (sdlooked == 0) {
		sdlooked = 1;
		s = splbio();
		if (ahaattach() == 0)
			for (t = 0; t < 8; t++)
				if (t != ahaid())
					sdprobe(t);
		splx(s);
		sdlooked = 2;
		wakeup((caddr_t)&sdlooked);
	}
	if (unit >= nsd)
		return (ENXIO);
	return (pcdkopen(&sd[unit].sd_dk, dev, mode, sdstrategy, "sd",
	    &sd[unit].sd_geom, &sd[unit].sd_start));
}

sdclose(dev, flag, mode)
	dev_t dev;
	int flag, mode;
{
	return (pcdkclose(&sd[dkunit(dev)].sd_dk, dev, mode));
}

daddr_t
sdsize(dev)
	dev_t dev;
{
	register struct sd *sp = &sd[dkunit(dev)];

	if (dkunit(dev) >= nsd || (sp->sd_dk.dk_flags & DKF_ALIVE) == 0)
		return (-1);
	return (sp->sd_dk.dk_parts[dkpart(dev)].p_size);
}

sdioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	return (pcdkioctl(&sd[dkunit(dev)].sd_dk, dev, cmd, data, flag,
	    sdstrategy, &sd[dkunit(dev)].sd_geom));
}

/*
 * Send a command: again the one that is out, if there is one, and
 * otherwise one for the transfer at the head of the queue and for as
 * many after it in the queue as go on from where it ends, which leave
 * the queue.  All of each transfer at once, straight to or from where
 * its data is.  Called at splbio.
 */
static
sdstart()
{
	register struct buf *bp, *np;
	register struct ccb *cp = &ahaccb;
	struct sd *sp;
	daddr_t sect;
	int dir, i;
	u_int n;		/* sectors: nothing here is counted in a
				   long, whose shifts are slow */

	if ((bp = sdcur) == NULL) {
		if ((bp = sdtab.b_actf) == NULL)
			return;
		n = bp->b_bcount >> 9;
		sdn = 1;
		np = bp;
		if (sdalone == 0)
			while (np->av_forw != NULL && sdn < SDNSEG &&
			    np->av_forw->b_dev == bp->b_dev &&
			    ((np->av_forw->b_flags ^ bp->b_flags) & B_READ) == 0 &&
			    np->av_forw->b_blkno == bp->b_blkno + n &&
			    n < SDMAXLEN &&
			    (np->av_forw->b_bcount >> 9) <= SDMAXLEN - n) {
				np = np->av_forw;
				n += np->b_bcount >> 9;
				sdn++;
			}
		sdcur = bp;
		sdtab.b_actf = np->av_forw;
		np->av_forw = NULL;
	} else
		for (n = 0, np = bp; np != NULL; np = np->av_forw)
			n += np->b_bcount >> 9;
	sdtab.b_active = 1;
	sp = &sd[dkunit(bp->b_dev)];
#ifdef UCB_METER
	if (sd_dkn >= 0 && sdtries == 0) {
		int dkn = sd_dkn + dkunit(bp->b_dev);

		dk_busy |= 1 << dkn;
		dk_xfer[dkn]++;
		dk_wds[dkn] += n << 3;
	}
#endif
	sect = bp->b_blkno + sp->sd_start +
	    sp->sd_dk.dk_parts[dkpart(bp->b_dev)].p_offset;
	dir = bp->b_flags & B_READ ? CCB_DATAIN : CCB_DATAOUT;
	if (sdn == 1)
		ahafill(sp->sd_target, dir, 10, bfpaddr(bp), bp->b_bcount);
	else {
		for (np = bp, i = 0; np != NULL; i++, np = np->av_forw)
			ahaseg(i, bfpaddr(np), np->b_bcount);
		ahafillsegs(sp->sd_target, dir, 10, sdn);
	}
	cp->cc_cdb[0] = bp->b_flags & B_READ ? SC_READ10 : SC_WRITE10;
	cp->cc_cdb[2] = sect >> 24;
	cp->cc_cdb[3] = sect >> 16;
	cp->cc_cdb[4] = sect >> 8;
	cp->cc_cdb[5] = sect;
	cp->cc_cdb[7] = n >> 8;
	cp->cc_cdb[8] = n;
	ahago(sdintr);
}

/*
 * The command that is out is over: how is what ahago says of it.
 */
static
sdintr(how)
	int how;
{
	register struct buf *bp = sdcur, *np;
	register struct ccb *cp = &ahaccb;
	register int key;
	static int failed;

	if (bp == NULL || sdtab.b_active == 0)
		return;
	if ((ahatest & 4) && sdn > 1 && failed < SDTRIES) {
		failed++;
		how = MBI_ABORTED;
	}
	key = sdsense(how);
	if (how != MBI_OK && key != SK_RECOVERED) {
		/*
		 * Tried again, unless it could not go better: the adapter
		 * is gone, or the target, or the target has understood
		 * and said no.
		 */
		if (how != AHA_DEAD && key != SK_ILLEGAL && key != SK_PROTECT &&
		    !(how == MBI_ERROR && cp->cc_hastat == HS_SELTIMEOUT) &&
		    ++sdtries < SDTRIES) {
			sdstart();
			return;
		}
		/*
		 * Of several, the error is one's: they go back to the
		 * head of the queue and each is sent by itself, to
		 * succeed or to have the error.
		 */
		if (sdn > 1) {
			printf("sd%d: %d-block command failed, retrying singly\n",
			    dkunit(bp->b_dev), sdn);
			for (np = bp; np->av_forw != NULL; np = np->av_forw)
				;
			if ((np->av_forw = sdtab.b_actf) == NULL)
				sdtab.b_actl = np;
			sdtab.b_actf = bp;
			sdcur = NULL;
			sdalone = sdn;
			sdtries = 0;
			sdstart();
			return;
		}
		harderr(bp, "sd");
		if (how < 0)
			printf("%s\n", how == AHA_DEAD ? "adapter dead" :
			    "timeout");
		else if (key >= 0)
			printf("sense key %d, code %x/%x\n", key,
			    cp->cc_cdb[cp->cc_cdblen + 12],
			    cp->cc_cdb[cp->cc_cdblen + 13]);
		else
			printf("mailbox %d, adapter %x, target %x\n", how,
			    cp->cc_hastat, cp->cc_tarstat);
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
	}
	sdtries = 0;
#ifdef UCB_METER
	if (sd_dkn >= 0)
		dk_busy &= ~(1 << (sd_dkn + dkunit(bp->b_dev)));
#endif
	sdcur = NULL;
	while (bp != NULL) {
		np = bp->av_forw;
		bp->b_resid = bp->b_flags & B_ERROR ? bp->b_bcount : 0;
		iodone(bp);
		bp = np;
	}
	if (sdalone)
		sdalone--;
	sdtab.b_active = 0;
	sdstart();
}

sdstrategy(bp)
	register struct buf *bp;
{
	register struct sd *sp;
	register int s;

	if (bp == NULL) {		/* what was held back may go (bioplug) */
		s = splbio();
		if (sdtab.b_active == 0)
			sdstart();
		splx(s);
		return;
	}
	if (dkunit(bp->b_dev) >= nsd ||
	    ((sp = &sd[dkunit(bp->b_dev)])->sd_dk.dk_flags & DKF_ALIVE) == 0) {
		bp->b_error = ENXIO;
		goto bad;
	}
	if (bp->b_bcount & 511) {	/* whole sectors only */
		bp->b_error = EINVAL;
		goto bad;
	}
	s = partition_check(bp, &sp->sd_dk);
	if (s < 0)
		goto bad;
	if (s == 0)			/* at the partition's end */
		goto done;
	if (bp->b_bcount == 0) {
		bp->b_resid = 0;
		goto done;
	}
	bp->b_cylin = (bp->b_blkno + sp->sd_start +
	    sp->sd_dk.dk_parts[dkpart(bp->b_dev)].p_offset) / sp->sd_spc;
	s = splbio();
	disksort(&sdtab, bp);
	if (sdtab.b_active == 0 && bioplug == 0)
		sdstart();
	splx(s);
	return;
bad:
	bp->b_flags |= B_ERROR;
	bp->b_resid = bp->b_bcount;
done:
	iodone(bp);
}
#endif /* NSD */
