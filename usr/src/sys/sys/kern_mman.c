/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_mman.c	1.3 (2.11BSD) 2000/2/20
 */

#include "param.h"
#include "../machine/seg.h"
#include "../machine/frame.h"
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

/*
 * More data segments than the program came with.  A process's memory
 * is the list of its segments (machine/seg.h), and these three calls
 * add to the list, change what was added and take it away again.  What
 * they make is the process's own as its data is: copied at fork,
 * swapped with the rest, written to a core file, and gone when the
 * process executes another program or ends.  It counts toward the most
 * a process may have (maxmem), with the data.
 *
 * A piece of more than 64K takes as many slots of the table as it
 * needs, one after another, every segment full but the last, so that
 * its selectors go up by eight: a program finds byte n of it at offset
 * n % 65536 of selector first + 8 * (n / 65536).  All but the last
 * slot of such a piece are marked PS_MORE, and it is known by its
 * first.
 */

static int segrun(struct pseg __far *, int);
static int segnamed(uaddr_t, int *);

/*
 * How many slots the piece that starts at seg has; 0 if no piece got
 * with segalloc starts there.
 */
static
segrun(sp, seg)
	register struct pseg __far *sp;
	register int seg;
{
	register int n;

	if (seg < 1 || seg >= NSEG ||
	    (sp[seg].ps_flag & (PS_USED|PS_ALLOC)) != (PS_USED|PS_ALLOC) ||
	    (sp[seg - 1].ps_flag & (PS_ALLOC|PS_MORE)) == (PS_ALLOC|PS_MORE))
		return (0);
	for (n = 1; sp[seg + n - 1].ps_flag & PS_MORE; n++)
		;
	return (n);
}

/*
 * Make the piece at slot seg of the running process, of n slots now,
 * size bytes long: 0 takes it away.  The caller has seen that the
 * slots it grows into are free and that there may be so much.  What is
 * new is zero.
 */
static
segset(seg, n, size)
	int seg, n;
	long size;
{
	register struct proc *p = u.u_procp;
	register struct pseg __far *sp;
	register struct trapframe *tf = (struct trapframe *)u.u_ar0;
	int want, c, d;
	long new, old;

	want = (size + 0xFFFFL) >> 16;
	for (sp = &PSEG(p)[seg]; n > 0 || want > 0; seg++, sp++, n--, want--) {
		new = want > 1 ? 0x10000L : want == 1 ? size : 0;
		size -= new;
		if (new == 0) {
			/*
			 * The process may have left its selector in DS
			 * or ES, as code that has just used a far pointer
			 * does; going back to it with that would be a
			 * fault.
			 */
			if (tf->tf_ds == LSEL(seg))
				tf->tf_ds = 0;
			if (tf->tf_es == LSEL(seg))
				tf->tf_es = 0;
			segfree(seg);
			continue;
		}
		old = n > 0 ? (long)sp->ps_limit + 1 : 0;
		c = btoc(new);
		d = c - sp->ps_size;
		if (d) {
			expand(c, seg);
			if (d > 0)
				clear(sp->ps_addr + c - d, d);
		}
		/* as for the break: the click's end is not new to it */
		if (new > old && (old & (NBPG - 1)))
			pclear(ctob((long)sp->ps_addr) + old,
			    (u_int)MIN(new - old, NBPG - (old & (NBPG - 1))));
		sp->ps_limit = new - 1;
		sp->ps_flag = want > 1 ? PS_USED | PS_ALLOC | PS_MORE :
		    PS_USED | PS_ALLOC;
		setldt(p, seg);
	}
	u.u_dsize = p->p_dsize;
}

/*
 * Would d clicks more be too much?
 */
static
segroom(d)
	long d;
{
	return (d <= 0 ||
	    (long)u.u_procp->p_dsize + u.u_tsize + d + USIZE <= maxmem);
}

/*
 * segalloc(size): a new piece of size bytes, all zero.  The result is
 * the far address of its start.
 */
segalloc()
{
	struct a {
		long	size;
	};
	register struct pseg __far *sp = PSEG(u.u_procp);
	register int seg, n;
	long size;
	int want;

	size = ((struct a *)u.u_ap)->size;
	if (size <= 0) {
		u.u_error = EINVAL;
		return;
	}
	want = (size + 0xFFFFL) >> 16;
	n = 0;
	if (size <= (long)(NSEG - 1) << 16)
		for (seg = 1; seg < NSEG && n < want; seg++)
			n = sp[seg].ps_flag & PS_USED ? 0 : n + 1;
	if (n < want || !segroom((long)btoc(size))) {
		u.u_error = ENOMEM;
		return;
	}
	seg -= want;
	segset(seg, 0, size);
	u.u_r.r_val1 = 0;
	u.u_r.r_val2 = LSEL(seg);
}

/*
 * The piece a process names by the address of its start, for segsize
 * and segfree: its slot, with how many it has in *np, or 0.
 */
static
segnamed(addr, np)
	uaddr_t addr;
	int *np;
{
	register int seg = LSLOT(USEL(addr));

	if (!ISLSEL(USEL(addr)) || UOFF(addr) != 0 ||
	    (*np = segrun(PSEG(u.u_procp), seg)) == 0) {
		u.u_error = EINVAL;
		return (0);
	}
	return (seg);
}

/*
 * segsize(addr, size): make the piece that starts at addr size bytes
 * long.  It stays where it is in the table, so a piece that grows past
 * a multiple of 64K needs the slots after its own to be free.
 */
segsize()
{
	register struct a {
		char __far *addr;
		long	size;
	} *uap = (struct a *)u.u_ap;
	register struct pseg __far *sp = PSEG(u.u_procp);
	register int seg, i;
	int n, want;
	long d;

	if ((seg = segnamed((uaddr_t)uap->addr, &n)) == 0)
		return;
	if (uap->size <= 0) {
		u.u_error = EINVAL;
		return;
	}
	d = btoc(uap->size);
	for (i = seg; i < seg + n; i++)
		d -= sp[i].ps_size;
	if (uap->size > (long)(NSEG - seg) << 16 || !segroom(d)) {
		u.u_error = ENOMEM;
		return;
	}
	want = (uap->size + 0xFFFFL) >> 16;
	for (i = seg + n; i < seg + want; i++)
		if (sp[i].ps_flag & PS_USED) {
			u.u_error = ENOMEM;
			return;
		}
	segset(seg, n, uap->size);
}

/*
 * segfree(addr): give back the piece that starts at addr.  Not the one
 * the process has its stack in.
 */
usegfree()
{
	struct a {
		char __far *addr;
	};
	register struct trapframe *tf = (struct trapframe *)u.u_ar0;
	register int seg;
	int n;

	if ((seg = segnamed((uaddr_t)((struct a *)u.u_ap)->addr, &n)) == 0)
		return;
	if (LSLOT(tf->tf_ss) >= seg && LSLOT(tf->tf_ss) < seg + n) {
		u.u_error = EBUSY;
		return;
	}
	segset(seg, n, 0L);
}
