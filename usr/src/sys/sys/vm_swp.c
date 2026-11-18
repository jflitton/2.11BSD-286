/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_swp.c	2.3 (2.11BSD) 11/30/94
 */

#include "param.h"
#include "../machine/seg.h"

#include "user.h"
#include "proc.h"
#include "buf.h"
#include "conf.h"
#include "systm.h"
#include "vm.h"
#include "trace.h"
#include "uio.h"

/*
 * swap I/O
 */
swap(blkno, coreaddr, count, rdflg)
	memaddr blkno, coreaddr;
	register int count;
	int rdflg;
{
	register struct buf *bp;
	register int tcount;
	int s;
	long paddr;

#ifdef UCB_METER
	if (rdflg) {
		cnt.v_pswpin += count;
		cnt.v_pgin++;
	}
	else {
		cnt.v_pswpout += count;
		cnt.v_pgout++;
	}
#endif
	bp = geteblk();			/* allocate a buffer header */

	while (count) {
		bp->b_flags = B_BUSY | B_PHYS | B_INVAL | rdflg;
		bp->b_dev = swapdev;
		tcount = count;
		if (tcount >= 64)	/* prevent byte-count wrap */
			tcount = 64;
		bp->b_bcount = ctob(tcount);
		bp->b_blkno = blkno;
		paddr = ctob((long)coreaddr);
		bp->b_un.b_addr = (caddr_t)loint(paddr);
		bp->b_xmem = hiint(paddr);
		trace(TR_SWAPIO);
		(*bdevsw[major(swapdev)].d_strategy)(bp);
		s = splbio();
		while ((bp->b_flags & B_DONE) == 0)
			sleep((caddr_t)bp, PSWP);
		splx(s);
		if ((bp->b_flags & B_ERROR) || bp->b_resid)
			panic("hard err: swap");
		count -= tcount;
		coreaddr += tcount;
		blkno += ctod(tcount);
	}
	brelse(bp);
}

/*
 * rout is the name of the routine where we ran out of swap space.
 */
swkill(p, rout)
	register struct proc *p;
	char *rout;
{

	tprintf(u.u_ttyp, "sorry, pid %d killed in %s: no swap space\n", 
		p->p_pid, rout);
	/*
	 * To be sure no looping (e.g. in vmsched trying to
	 * swap out) mark process locked in core (as though
	 * done by user) after killing it so noone will try
	 * to swap it out.
	 */
	psignal(p, SIGKILL);
	p->p_flag |= SULOCK;
}

/*
 * Raw I/O. The arguments are
 *	The strategy routine for the device
 *	A buffer, which may be a special buffer header
 *	  owned exclusively by the device for this purpose or
 *	  NULL if one is to be allocated.
 *	The device number
 *	Read/write flag
 * Essentially all the work is computing physical addresses and
 * validating them.
 *
 * rewritten to use the iov/uio mechanism from 4.3bsd.  the physbuf routine
 * was inlined.  essentially the chkphys routine performs the same task
 * as the useracc routine on a 4.3 system. 3/90 sms
 *
 * If the buffer pointer is NULL then one is allocated "dynamically" from
 * the system cache.  the 'invalid' flag is turned on so that the brelse()
 * done later doesn't place the buffer back in the cache.  the 'phys' flag
 * is left on so that the address of the buffer is recalcuated in getnewbuf().
 * The BYTE/WORD stuff began to be removed after testing proved that either
 * 1) the underlying hardware gives an error or 2) nothing bad happens.
 * besides, 4.3BSD doesn't do the byte/word check and noone could remember
 * why the byte/word check was added in the first place - likely historical
 * paranoia.  chkphys() inlined.  5/91 sms
 *
 * Refined (and streamlined) the flow by using a 'for' construct 
 * (a la 4.3Reno).  Avoid allocating/freeing the buffer for each iovec
 * element (i must have been confused at the time).  6/91-sms
 *
 * Finished removing the BYTE/WORD code as part of implementing the common
 * raw read&write routines , systems had been running fine for several
 * months with it ifdef'd out.  9/91-sms
 */
physio(strat, bp, dev, rw, uio)
	int (*strat)();
	register struct buf *bp;
	dev_t dev;
	int rw;
	register struct uio *uio;
{
	int error = 0, s, c, allocbuf = 0;
	register struct iovec *iov;
	register struct pseg __far *sp;
	u_int sel, off;
	long paddr;

	if (!bp) {
		allocbuf++;
		bp = geteblk();
	}
	u.u_procp->p_flag |= SLOCK;
	for ( ; uio->uio_iovcnt; uio->uio_iov++, uio->uio_iovcnt--) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0)
			continue;
		/*
		 * The transfer is to or from the process's memory itself,
		 * by its physical address.  It must lie in one segment,
		 * and that one of the process's own: not its code, which
		 * others share.
		 */
		sel = USEL(iov->iov_base);
		off = UOFF(iov->iov_base);
		sp = &PSEG(u.u_procp)[LSLOT(sel)];
		if (!ISLSEL(sel) || LSLOT(sel) >= NSEG ||
		    (sp->ps_flag & (PS_USED|PS_TEXT)) != PS_USED ||
		    off > sp->ps_limit ||
		    iov->iov_len - 1 > sp->ps_limit - off) {
			error = EFAULT;
			break;
		}
		if (!allocbuf) {
			s = splbio();
			while (bp->b_flags & B_BUSY) {
				bp->b_flags |= B_WANTED;
				sleep((caddr_t)bp, PRIBIO+1);
			}
			splx(s);
		}
		bp->b_error = 0;
		while (iov->iov_len) {
			bp->b_flags = B_BUSY|B_PHYS|B_INVAL|rw;
			bp->b_dev = dev;
			paddr = ctob((long)sp->ps_addr) + UOFF(iov->iov_base);
			bp->b_un.b_addr = (caddr_t)loint(paddr);
			bp->b_xmem = hiint(paddr);
			bp->b_blkno = uio->uio_offset >> PGSHIFT;
			bp->b_bcount = iov->iov_len;
			c = bp->b_bcount;
			(*strat)(bp);
			s = splbio();
			while ((bp->b_flags & B_DONE) == 0)
				sleep((caddr_t)bp, PRIBIO);
			if (bp->b_flags & B_WANTED)	/* rare */
				wakeup((caddr_t)bp);
			splx(s);
			c -= bp->b_resid;
			iov->iov_base += c;
			iov->iov_len -= c;
			uio->uio_resid -= c;
			uio->uio_offset += c;
			/* temp kludge for tape drives */
			if (bp->b_resid || (bp->b_flags & B_ERROR))
				break;
		}
		bp->b_flags &= ~(B_BUSY|B_WANTED);
		error = geterror(bp);
		/* temp kludge for tape drives */
		if (bp->b_resid || error)
			break;
	}
	if (allocbuf)
		brelse(bp);
	u.u_procp->p_flag &= ~SLOCK;
	return(error);
}

rawrw(dev, uio, flag)
	dev_t dev;
	register struct uio *uio;
	int flag;
	{
	return(physio(cdevsw[major(dev)].d_strategy, (struct buf *)NULL, dev,
		uio->uio_rw == UIO_READ ? B_READ : B_WRITE, uio));
	}
