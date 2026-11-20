/*
 * What the drivers for a PC's hard disks have in common.
 *
 * A PC's hard disk can be shared between systems.  The table at the end
 * of its first sector gives up to four of them a run of sectors each,
 * and says by a number whose each is.  A driver treats this system's
 * run as the disk: its label is in the second sector of it and the
 * label's partitions are counted from its start.  A disk without a
 * table, or with nothing in it for this system, is this system's from
 * its first sector to its last.
 */
#include "param.h"
#include "systm.h"
#include "user.h"
#include "buf.h"
#include "stat.h"
#include "ioctl.h"
#include "syslog.h"
#include "disklabel.h"
#include "disk.h"
#include "pcdisk.h"

/*
 * Dev is a partition that the driver, for now, has made the whole of
 * the disk, and strat its strategy routine.  Returns the first sector
 * of this system's part of the disk and stores its size, or returns 0
 * when the disk is not divided, or has nothing for this system, or
 * cannot be read.
 */
static daddr_t
pcpart(dev, strat, sizep)
	dev_t dev;
	int (*strat)();
	daddr_t *sizep;
{
	register struct buf *bp;
	register struct pcpart __far *pp;
	u_char __far *sect;
	daddr_t start = 0;
	int i;

	bp = geteblk();
	bp->b_dev = dev;
	bp->b_blkno = 0;
	bp->b_bcount = 512;
	bp->b_flags = B_BUSY | B_READ;
	bp->b_cylin = 0;
	(*strat)(bp);
	biowait(bp);
	if ((bp->b_flags & B_ERROR) == 0) {
		sect = (u_char __far *)mapin(bp);
		pp = (struct pcpart __far *)(sect + PCP_TABLE);
		if (sect[510] == 0x55 && sect[511] == 0xAA)
			for (i = 0; i < PCP_NPART; i++, pp++)
				if (pp->pp_type == PCP_2BSD && pp->pp_start &&
				    pp->pp_size) {
					start = pp->pp_start;
					*sizep = pp->pp_size;
					break;
				}
		mapout(bp);
	}
	u.u_error = 0;
	bp->b_flags = B_INVAL | B_AGE;
	brelse(bp);
	return (start);
}

/*
 * A label for a drive that has none, or before its own is read: one
 * partition, of size sectors.
 */
static
dfltlabel(dk, lp, gp, size)
	struct dkdevice *dk;
	register struct disklabel *lp;
	register struct pcgeom *gp;
	daddr_t size;
{
	register struct partition *pi = &lp->d_partitions[0];

	bzero(lp, sizeof (*lp));
	lp->d_type = gp->pg_type;
	lp->d_secsize = 512;
	lp->d_nsectors = gp->pg_sects;
	lp->d_ntracks = gp->pg_heads;
	lp->d_ncylinders = gp->pg_cyls;
	lp->d_secpercyl = gp->pg_heads * gp->pg_sects;
	lp->d_secperunit = size;
	lp->d_npartitions = 1;
	pi->p_size = size;
	pi->p_fstype = FS_V71K;
	pi->p_frag = 1;
	pi->p_fsize = 1024;
	bcopy(pi, dk->dk_parts, sizeof (lp->d_partitions));
}

/*
 * Whether each of a label's partitions is within this system's part of
 * the drive, size sectors: one that ran past its end would be over
 * another system's.
 */
static
pcfits(lp, size)
	register struct disklabel *lp;
	daddr_t size;
{
	register struct partition *pi;
	int i;

	if (lp->d_npartitions > MAXPARTITIONS)
		return (0);
	for (pi = lp->d_partitions, i = 0; i < lp->d_npartitions; i++, pi++)
		if (pi->p_offset > size || pi->p_size > size - pi->p_offset)
			return (0);
	return (1);
}

/*
 * On the first open of a drive: find this system's part of it and read
 * the label there.  Dev is any partition of the drive, strat and name
 * the driver's, and dk->dk_label somewhere to keep the label.  *startp
 * is where the driver keeps the first sector of the system's part,
 * which its strategy routine adds to every sector number: it is 0 while
 * the drive's own first sector is read, and then set.  May sleep.
 */
static
pcdklabel(dk, dev, strat, name, gp, startp)
	register struct dkdevice *dk;
	dev_t dev;
	int (*strat)();
	char *name;
	struct pcgeom *gp;
	daddr_t *startp;
{
	struct disklabel locallabel;
	register struct disklabel *lp = &locallabel;
	daddr_t start, size, psize;
	char *msg;

	dev &= ~7;			/* partition a */
	*startp = 0;
	size = gp->pg_size;
	dfltlabel(dk, lp, gp, size);	/* the whole drive */
	start = pcpart(dev, strat, &psize);
	if (start != 0 && start < size && psize <= size - start) {
		*startp = start;
		size = psize;
		dfltlabel(dk, lp, gp, size);
	}
	gp->pg_part = size;
	msg = readdisklabel(dev, strat, lp);
	if (msg == 0 && !pcfits(lp, size))
		msg = "label does not fit";
	if (msg != 0) {
		log(LOG_NOTICE, "%s%da is entire disk: %s\n", name,
		    dkunit(dev), msg);
		dfltlabel(dk, lp, gp, size);
	}
	bcopy(lp, dk->dk_label, sizeof (struct disklabel));
	bcopy(lp->d_partitions, dk->dk_parts, sizeof (lp->d_partitions));
}

/*
 * The rest of a disk driver's open routine, once the drive is known to
 * be there: read the label if this is the first open, and note which
 * partition is open and how.  *startp is where the driver keeps the
 * first sector of the system's part.
 */
pcdkopen(dk, dev, mode, strat, name, gp, startp)
	register struct dkdevice *dk;
	dev_t dev;
	int mode;
	int (*strat)();
	char *name;
	struct pcgeom *gp;
	daddr_t *startp;
{
	register int mask = 1 << dkpart(dev);

	while (dk->dk_flags & (DKF_OPENING | DKF_CLOSING))
		sleep((caddr_t)dk, PRIBIO);
	if (dk->dk_openmask == 0) {
		dk->dk_flags |= DKF_OPENING | DKF_ALIVE;
		pcdklabel(dk, dev, strat, name, gp, startp);
		dk->dk_flags &= ~DKF_OPENING;
		wakeup((caddr_t)dk);
	}
	if (dkpart(dev) >= dk->dk_label->d_npartitions)
		return (ENXIO);
	dkoverlapchk(dk->dk_openmask, dev, dk->dk_label, name);
	if (mode == S_IFCHR)
		dk->dk_copenmask |= mask;
	else if (mode == S_IFBLK)
		dk->dk_bopenmask |= mask;
	else
		return (EINVAL);
	dk->dk_openmask |= mask;
	return (0);
}

pcdkclose(dk, dev, mode)
	register struct dkdevice *dk;
	dev_t dev;
	int mode;
{
	register int mask = 1 << dkpart(dev);

	if (mode == S_IFCHR)
		dk->dk_copenmask &= ~mask;
	else if (mode == S_IFBLK)
		dk->dk_bopenmask &= ~mask;
	else
		return (EINVAL);
	dk->dk_openmask = dk->dk_bopenmask | dk->dk_copenmask;
	return (0);
}

/*
 * A driver's ioctl routine, for the label.  A label given to the drive
 * is held to what one read from it is: a partition beyond this system's
 * part of the drive is refused, before the label is set or written.
 */
pcdkioctl(dk, dev, cmd, data, flag, strat, gp)
	struct dkdevice *dk;
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
	int (*strat)();
	struct pcgeom *gp;
{
	if ((cmd == DIOCSDINFO || cmd == DIOCWDINFO) &&
	    !pcfits((struct disklabel *)data, gp->pg_part))
		return (ENOSPC);
	return (ioctldisklabel(dev, cmd, data, flag, dk, strat));
}
