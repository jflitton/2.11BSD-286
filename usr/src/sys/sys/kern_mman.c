/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_mman.c	1.3 (2.11BSD) 2000/2/20
 */

#include "param.h"
#include "../machine/seg.h"
#include "user.h"
#include "proc.h"
#include "vm.h"
#include "text.h"
#include "systm.h"

/*
 * Set the break: the end of the segment that holds the program's data
 * and stack.  The argument is an address in that segment; the segment
 * becomes exactly that long, so that a stray reference past the break
 * is a fault.  The memory gained is zero.
 */
sbrk()
{
	struct a {
		char __far *nsiz;
	};
	register struct proc *p = u.u_procp;
	register struct pseg __far *sp;
	register int n, d;
	long new, old;

	new = UOFF(((struct a *)u.u_ap)->nsiz);
	if (new == 0)
		new = 0x10000L;		/* the end of a full segment */
	old = (long)u.u_break ? (long)u.u_break : 0x10000L;
	if (u.u_dseg == 0 || new < u.u_stack0 + ctob((long)u.u_ssize)) {
		u.u_error = ENOMEM;
		return;
	}
	sp = &PSEG(p)[u.u_dseg];
	n = btoc(new);
	d = n - sp->ps_size;
	/* Check resource limits */
	if (u.u_rlimit[RLIMIT_DATA].rlim_cur != RLIM_INFINITY &&
	    new > u.u_rlimit[RLIMIT_DATA].rlim_cur ||
	    d > 0 && (long)p->p_dsize + u.u_tsize + d + USIZE > maxmem) {
		u.u_error = ENOMEM;
		return;
	}
	if (d) {
		expand(n, u.u_dseg);
		if (d > 0)
			clear(sp->ps_addr + n - d, d);
	}
	/*
	 * What is left of the last click was cleared when the click was
	 * got, but the program has had the use of some of it since.
	 */
	if (new > old && (old & (NBPG - 1)))
		pclear(ctob((long)sp->ps_addr) + old,
		    (u_int)MIN(new - old, NBPG - (old & (NBPG - 1))));
	sp->ps_limit = new - 1;
	setldt(p, u.u_dseg);
	u.u_break = new;
	u.u_dsize = p->p_dsize;
}
