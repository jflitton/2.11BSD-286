/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ufs_bmap.c	1.2 (2.11BSD) 1996/9/19
 */

#include "param.h"
#include "../machine/seg.h"

#include "systm.h"
#include "conf.h"
#include "dir.h"
#include "inode.h"
#include "user.h"
#include "buf.h"
#include "fs.h"
#include "mount.h"
#include "uio.h"

/*
 * Bmap defines the structure of file system storage
 * by returning the physical block number on a device given the
 * inode and the logical block number in a file.
 * When convenient, it also leaves the physical
 * block number of the next block of the file in rablock
 * for use in read-ahead, and in rasize how many of the file's blocks
 * from that one on follow one another on the disk: they can be read
 * with one command (breada).  It counts them only when asked (B_RUN),
 * as far as RAMAX and as the inode or the indirect block in hand says,
 * and otherwise rasize is 1.
 */
daddr_t
bmap(ip, bn, rwflg, flags)
	register struct inode *ip;
	daddr_t bn;
	int rwflg, flags;
{
	register int i;
	register struct buf *bp;
	struct buf *nbp;
	int j, sh;
	daddr_t nb, __far *bap, __far *rap, ra;
	int n;
	int async = ip->i_fs->fs_flags & MNT_ASYNC;

	if (bn < 0) {
		u.u_error = EFBIG;
		return((daddr_t)0);
	}
	ra = rablock = 0;
	n = rasize = 0;

	/*
	 * blocks 0..NADDR-4 are direct blocks
	 */
	if (bn < NADDR-3) {
		i = bn;
		nb = ip->i_addr[i];
		if (nb == 0) {
			if (rwflg == B_READ || (bp = balloc(ip, flags)) == NULL)
				return((daddr_t)-1);
			nb = dbtofsb(bp->b_blkno);
/*
 * directory blocks are usually the only thing written synchronously at this
 * point (so they never appear with garbage in them on the disk).  This is
 * overridden if the filesystem was mounted 'async'.
*/
			if (flags & B_SYNC)
				bwrite(bp);
			else
				bdwrite(bp);
			ip->i_addr[i] = nb;
			ip->i_flag |= IUPD|ICHG;
		}
		if (i < NADDR-4 && (ra = ip->i_addr[i+1]) != 0) {
			n = 1;
			if (flags & B_RUN)
				for (; i + 1 + n < NADDR-3 &&
				    ip->i_addr[i + 1 + n] == ra + n; n++)
					;
			rablock = ra;
			rasize = n;
		}
		return(nb);
	}

	/*
	 * addresses NADDR-3, NADDR-2, and NADDR-1
	 * have single, double, triple indirect blocks.
	 * the first step is to determine
	 * how many levels of indirection.
	 */
	sh = 0;
	nb = 1;
	bn -= NADDR-3;
	for (j = 3;j > 0;j--) {
		sh += NSHIFT;
		nb <<= NSHIFT;
		if (bn < nb)
			break;
		bn -= nb;
	}
	if (j == 0) {
		u.u_error = EFBIG;
		return((daddr_t)0);
	}

	/*
	 * fetch the first indirect block
	 */
	nb = ip->i_addr[NADDR-j];
	if (nb == 0) {
		if (rwflg == B_READ || (bp = balloc(ip, flags | B_CLRBUF)) == NULL)
			return((daddr_t) -1);
		nb = dbtofsb(bp->b_blkno);
		/*
		 * Write synchronously if requested so that indirect blocks
		 * never point at garbage.
		 */
		if (async)
			bdwrite(bp);
		else
			bwrite(bp);
		ip->i_addr[NADDR-j] = nb;
		ip->i_flag |= IUPD|ICHG;
	}

	/*
	 * fetch through the indirect blocks
	 */
	for(;j <= 3;j++) {
		bp = bread(ip->i_dev, nb);
		if ((bp->b_flags & B_ERROR) || bp->b_resid) {
			brelse(bp);
			return((daddr_t)0);
		}
		bap = (daddr_t __far *)mapin(bp);
		sh -= NSHIFT;
		i = (bn>>sh) & NMASK;
		nb = bap[i];
		/*
		 * calculate read-ahead
		 */
		if (i < NINDIR-1) {
			ra = bap[i+1];
			n = ra != 0;
			if (n && j == 3 && (flags & B_RUN))
				for (rap = &bap[i+2]; n < RAMAX &&
				    i + 1 + n < NINDIR && *rap == ra + n;
				    n++, rap++)
					;
		}
		mapout(bp);
		if (nb == 0) {
			/*
			 * An indirect block is cleared, for what is not
			 * yet in it is holes.  A block of the file's
			 * data is cleared only if the caller asks, as a
			 * direct one is above: one written whole has
			 * nothing in it to keep, and clearing it first
			 * was a tenth of the time of writing a file.
			 */
			if (rwflg == B_READ || (nbp = balloc(ip,
			    j < 3 ? flags | B_CLRBUF : flags)) == NULL) {
				brelse(bp);
				return((daddr_t) -1);
			}
			nb = dbtofsb(nbp->b_blkno);
/*
 * Write synchronously so indirect blocks never point at garbage and blocks
 * in directories never contain garbage.  This check used to be based on the
 * type of inode, if it was a directory then 'sync' writes were done.  See the
 * comments earlier about filesystems being mounted 'async'.
*/
			if (!async && (j < 3 || (flags & B_SYNC)))
				bwrite(nbp);
			else
				bdwrite(nbp);
			bap = (daddr_t __far *)mapin(bp);
			bap[i] = nb;
			mapout(bp);
			bdwrite(bp);
		} else
			brelse(bp);
	}
	rablock = ra;
	rasize = n;
	return(nb);
}
