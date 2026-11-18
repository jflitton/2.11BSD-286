/*
 * A PC's hard disk can be shared between systems.  The table at the end
 * of its first sector gives up to four of them a run of sectors each,
 * and says by a number whose each is.  A disk driver calls pcpart to
 * find this system's, and treats that as the disk: its label is in the
 * second sector of it and the label's partitions are counted from its
 * start.  A disk without one is this system's from its first sector to
 * its last.
 */
#include "param.h"
#include "systm.h"
#include "user.h"
#include "buf.h"
#include "pcpart.h"

/*
 * Dev is a partition that the driver, for now, has made the whole of
 * the disk, and strat its strategy routine.  Returns the first sector
 * of this system's part of the disk and stores its size, or returns 0
 * when the disk is not divided, or has nothing for this system, or
 * cannot be read.
 */
daddr_t
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
