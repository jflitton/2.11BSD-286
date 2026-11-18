/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_swap.c	1.3 (2.11BSD GTE) 3/10/93
 */

#include "param.h"
#include "../machine/seg.h"
#include "user.h"
#include "proc.h"
#include "text.h"
#include "map.h"
#include "buf.h"
#include "systm.h"
#include "vm.h"

/*
 * Is this slot of a process's table a segment of its own, with memory?
 */
#define	OWNSEG(sp) \
	(((sp)->ps_flag & (PS_USED|PS_TEXT)) == PS_USED && (sp)->ps_size)

/*
 * Swap a process in.
 * Allocate the text, if it is not in memory, and each of the process's
 * own segments; if not all of it can be had, none of it is kept.  The
 * user structure and kernel stack never left.
 */
swapin(p)
	register struct proc *p;
{
	register struct text *xp;
	register struct pseg __far *sp;
	register int n;
	memaddr x = NULL;

	/* Malloc the text segment first, as it tends to be largest. */
	xp = p->p_textp;
	if (xp) {
		xlock(xp);
		if (!xp->x_caddr && !xp->x_ccount) {
			x = malloc(coremap, xp->x_size);
			if (!x) {
				xunlock(xp);
				return(0);
			}
		}
	}
	for (sp = PSEG(p), n = 0; n < NSEG; n++, sp++) {
		if (!OWNSEG(sp))
			continue;
		if ((sp->ps_addr = malloc(coremap, (size_t)sp->ps_size)) == NULL) {
			while (--n >= 0) {
				--sp;
				if (OWNSEG(sp))
					mfree(coremap, (size_t)sp->ps_size,
					    sp->ps_addr);
			}
			if (x)
				mfree(coremap, xp->x_size, x);
			if (xp)
				xunlock(xp);
			return(0);
		}
	}
	if (xp) {
		if (x) {
			xp->x_caddr = x;
			if ((xp->x_flag & XLOAD) == 0)
				swap(xp->x_daddr, x, xp->x_size, B_READ);
		}
		xp->x_ccount++;
		xunlock(xp);
	}
	for (sp = PSEG(p), n = 0; n < NSEG; n++, sp++) {
		if (OWNSEG(sp)) {
			swap(sp->ps_swap, sp->ps_addr, (size_t)sp->ps_size,
			    B_READ);
			mfree(swapmap, ctod((size_t)sp->ps_size), sp->ps_swap);
		}
		setldt(p, n);
	}
	if (p->p_stat == SRUN)
		setrq(p);
	p->p_flag |= SLOAD;
	p->p_time = 0;
#ifdef UCB_METER
	cnt.v_swpin++;
#endif
	return(1);
}

/*
 * Swap out process p.
 * gseg is a segment of its own that is being made larger, with osize
 * its old size, and is supplied during core expansion swaps: only what
 * it had is written, into room for what it is to have.  Otherwise gseg
 * is -1.
 * The freecore flag causes its core to be freed -- it may be
 * off when called to create an image for a child process
 * in newproc.
 *
 * panic: out of swap space
 */
swapout(p, freecore, gseg, osize)
	register struct proc *p;
	int freecore, gseg;
	u_int osize;
{
	register struct pseg __far *sp;
	register int n;
	u_int size;
	memaddr a;

	p->p_flag |= SLOCK;
	if (p->p_textp)
		xccdec(p->p_textp);
	for (sp = PSEG(p), n = 0; n < NSEG; n++, sp++) {
		if (!OWNSEG(sp))
			continue;
		if ((a = malloc(swapmap, ctod((size_t)sp->ps_size))) == NULL)
			panic("out of swap space");
		size = n == gseg ? osize : sp->ps_size;
		if (size) {
			swap(a, sp->ps_addr, size, B_WRITE);
			if (freecore == X_FREECORE)
				mfree(coremap, size, sp->ps_addr);
		}
		sp->ps_swap = a;
	}
	if (p == u.u_procp)
		u.u_ru.ru_nswap++;
	p->p_flag &= ~(SLOAD|SLOCK);
	p->p_time = 0;

#ifdef UCB_METER
	cnt.v_swpout++;
#endif

	if (runout) {
		runout = 0;
		wakeup((caddr_t)&runout);
	}
}
