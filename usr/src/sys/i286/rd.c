/*
 * Disks in memory.  Unit 0 is the image the boot program found after
 * the kernel and loaded above 1M (boot.h), if there was one: a root
 * file system with no disk driver needed.  Unit 1 is rdswap clicks of
 * memory taken when the system starts, for a swap area.
 */
#include "rd.h"
#if NRD > 0
#include "param.h"
#include "../machine/boot.h"
#include "buf.h"
#include "user.h"
#include "conf.h"
#include "map.h"
#include "systm.h"

u_int	rdswap = 512;		/* (patchable) clicks for unit 1 */

static struct rd {
	memaddr	rd_base;	/* where it is, in clicks */
	u_int	rd_size;	/* sectors, which are clicks */
} rd[NRD];

/*
 * Called at start-up, before any memory has been handed out, with the
 * first click above 1M.  Returns the first click after unit 0.
 */
memaddr
rdinit(base)
	memaddr base;
{
	rd[0].rd_base = base;
	rd[0].rd_size = bootinfo.bi_rdsize;
	return (base + rd[0].rd_size);
}

rdroot()
{
	if (rd[1].rd_base == 0 && rdswap &&
	    (rd[1].rd_base = malloc(coremap, rdswap)) != 0)
		rd[1].rd_size = rdswap;
}

rdopen(dev)
	dev_t dev;
{
	if (minor(dev) >= NRD || rd[minor(dev)].rd_size == 0)
		return (ENXIO);
	return (0);
}

daddr_t
rdsize(dev)
	dev_t dev;
{
	if (minor(dev) >= NRD)
		return (-1);
	return (rd[minor(dev)].rd_size);
}

rdstrategy(bp)
	register struct buf *bp;
{
	register struct rd *rp;
	long addr;

	rp = &rd[minor(bp->b_dev)];
	if (minor(bp->b_dev) >= NRD || bp->b_blkno < 0 ||
	    bp->b_blkno + btoc(bp->b_bcount) > rp->rd_size) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
	} else if (bp->b_bcount) {
		addr = ctob(rp->rd_base + bp->b_blkno);
		if (bp->b_flags & B_READ)
			pcopy(addr, bfpaddr(bp), bp->b_bcount);
		else
			pcopy(bfpaddr(bp), addr, bp->b_bcount);
	}
	bp->b_resid = 0;
	iodone(bp);
}
#endif /* NRD */
