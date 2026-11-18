/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_proc.c	1.2 (2.11BSD GTE) 12/24/92
 */

#include "param.h"
#include "user.h"
#include "proc.h"
#include "text.h"
#include "map.h"
#include "kernel.h"

/*
 * Change the size of one of the running process's own segments: seg is
 * its place in the process's table (machine/seg.h), newsize its new size
 * in clicks.  A segment that was not there has size 0.
 * If the size is shrinking, it's easy -- just release the extra core.
 * If it's growing, the memory that follows it is taken if that is free.
 * Otherwise, if there is core, just allocate it and copy the image.  If
 * there is no core, arrange for the process to be swapped out after
 * adjusting the size requirement -- when it comes in, enough core will
 * be allocated.
 *
 * The caller sets the segment's limit and flags, clears what is new,
 * and has its descriptor remade.
 */
expand(newsize, seg)
	int newsize, seg;
{
	register struct proc *p;
	register struct pseg __far *sp;
	register int n;
	memaddr a1, a2;

	p = u.u_procp;
	sp = &PSEG(p)[seg];
	n = sp->ps_size;
	sp->ps_size = newsize;
	if (newsize)
		sp->ps_flag = PS_USED;
	p->p_dsize += newsize - n;
	a1 = sp->ps_addr;
	if (n >= newsize) {
		mfree(coremap, n - newsize, a1 + newsize);
		return;
	}
	if (n && mallocat(coremap, newsize - n, a1 + n))
		return;
	if (setjmp(&u.u_ssave)) {
		/*
		 * Swapped out and in again: everything is somewhere else.
		 */
		sureg();
		return;
	}
	a2 = malloc(coremap, newsize);
	if (a2 == NULL) {
		swapout(p, X_FREECORE, seg, n);
		p->p_flag |= SSWAP;
		swtch();
		/* NOTREACHED */
	}
	sp->ps_addr = a2;
	if (n) {
		copy(a1, a2, n);
		mfree(coremap, n, a1);
	}
}

/*
 * Take a segment out of the running process's table: free it if it is
 * the process's own, forget it if it is part of the text.
 */
segfree(seg)
	int seg;
{
	register struct pseg __far *sp;

	sp = &PSEG(u.u_procp)[seg];
	if ((sp->ps_flag & PS_USED) && (sp->ps_flag & PS_TEXT) == 0)
		expand(0, seg);
	sp->ps_flag = 0;
	sp->ps_size = 0;
	setldt(u.u_procp, seg);
}
