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

#define	NSD		4
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
static int sdtries;		/* of the transfer at its head */
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
		printf("aha: id %d: will not say what it is (%d, %x, %x)\n",
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
		printf("aha: id %d: %s, not a disk (type %d)\n", target, name,
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
		printf("aha: id %d: %s: will not say how big it is (%d, %x, %x, key %d)\n",
		    target, name, st, cp->cc_hastat, cp->cc_tarstat,
		    sdsense(st));
		return;
	}
	size = get4(sdbuf) + 1;
	secsize = get4(sdbuf + 4);
	if (secsize != 512) {
		printf("aha: id %d: %s: sectors of %D bytes\n", target, name,
		    secsize);
		return;
	}
	if (nsd >= NSD) {
		printf("aha: id %d: %s: one disk too many\n", target, name);
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
	printf("sd%d: id %d: %s, %D MB\n", nsd, target, name, size / 2048);
	ahahow(target);
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
	return (ioctldisklabel(dev, cmd, data, flag, &sd[dkunit(dev)].sd_dk,
	    sdstrategy));
}

/*
 * Send the command for the transfer at the head of the queue: all of
 * it at once, straight to or from where its data is.  Called at
 * splbio.
 */
static
sdstart()
{
	register struct buf *bp;
	register struct ccb *cp = &ahaccb;
	register struct sd *sp;
	daddr_t sect;
	u_int n;

	if ((bp = sdtab.b_actf) == NULL)
		return;
	sdtab.b_active = 1;
	sp = &sd[dkunit(bp->b_dev)];
#ifdef UCB_METER
	if (sd_dkn >= 0 && sdtries == 0) {
		int dkn = sd_dkn + dkunit(bp->b_dev);

		dk_busy |= 1 << dkn;
		dk_xfer[dkn]++;
		dk_wds[dkn] += bp->b_bcount >> 6;
	}
#endif
	sect = bp->b_blkno + sp->sd_start +
	    sp->sd_dk.dk_parts[dkpart(bp->b_dev)].p_offset;
	n = bp->b_bcount >> 9;
	ahafill(sp->sd_target, bp->b_flags & B_READ ? CCB_DATAIN : CCB_DATAOUT,
	    10, bfpaddr(bp), bp->b_bcount);
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
 * The command for the transfer at the head of the queue is over: how
 * is what ahago says of it.
 */
static
sdintr(how)
	int how;
{
	register struct buf *bp = sdtab.b_actf;
	register struct ccb *cp = &ahaccb;
	register int key;

	if (bp == NULL || sdtab.b_active == 0)
		return;
	key = sdsense(how);
	if (how == MBI_OK || key == SK_RECOVERED)
		bp->b_resid = 0;
	else {
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
		harderr(bp, "sd");
		if (how < 0)
			printf("%s\n", how == AHA_DEAD ? "the adapter is dead" :
			    "no answer");
		else if (key >= 0)
			printf("sense key %d, code %x/%x\n", key,
			    cp->cc_cdb[cp->cc_cdblen + 12],
			    cp->cc_cdb[cp->cc_cdblen + 13]);
		else
			printf("mailbox %d, adapter %x, target %x\n", how,
			    cp->cc_hastat, cp->cc_tarstat);
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
	}
	sdtries = 0;
	sdtab.b_active = 0;
	sdtab.b_actf = bp->av_forw;
#ifdef UCB_METER
	if (sd_dkn >= 0)
		dk_busy &= ~(1 << (sd_dkn + dkunit(bp->b_dev)));
#endif
	iodone(bp);
	sdstart();
}

sdstrategy(bp)
	register struct buf *bp;
{
	register struct sd *sp;
	register int s;

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
	if (sdtab.b_active == 0)
		sdstart();
	splx(s);
	return;
bad:
	bp->b_flags |= B_ERROR;
	bp->b_resid = bp->b_bcount;
done:
	iodone(bp);
}
