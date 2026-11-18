/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_fork.c	1.6 (2.11BSD) 1999/8/11
 */

#include "param.h"
#include "../machine/reg.h"
#include "../machine/seg.h"

#include "systm.h"
#include "map.h"
#include "user.h"
#include "proc.h"
#include "inode.h"
#include "acct.h"
#include "file.h"
#include "vm.h"
#include "text.h"
#include "kernel.h"
#ifdef QUOTA
#include "quota.h"
#endif

/*
 * fork --
 *	fork system call
 */
fork()
{
	fork1(0);
}

/*
 * vfork --
 *	vfork system call, fast version of fork
 */
vfork()
{
	fork1(1);
}

fork1(isvfork)
{
	register int a;
	register struct proc *p1, *p2;

	a = 0;
	if (u.u_uid != 0) {
		for (p1 = allproc; p1; p1 = p1->p_nxt)
			if (p1->p_uid == u.u_uid)
				a++;
		for (p1 = zombproc; p1; p1 = p1->p_nxt)
			if (p1->p_uid == u.u_uid)
				a++;
	}
	/*
	 * Disallow if
	 *  No processes at all;
	 *  not su and too many procs owned; or
	 *  not su and would take last slot.
	 */
	p2 = freeproc;
	if (p2==NULL)
		tablefull("proc");
	if (p2==NULL || (u.u_uid!=0 && (p2->p_nxt == NULL || a>MAXUPRC))) {
		u.u_error = EAGAIN;
		goto out;
	}
	if ((p2 = newproc(isvfork)) == NULL) {
		u.u_error = ENOMEM;
		goto out;
	}
	u.u_r.r_val1 = p2->p_pid;

out:
	u.u_r.r_val2 = 0;
}

/*
 * What a new process does first.  It does not come out of newproc as
 * its parent does: it starts on a kernel stack of its own, at the
 * machine's entry for a new process, which calls this and then goes on
 * to the program with the result of fork that a child gets.
 */
forkchild()
{
	u.u_start = time.tv_sec;
	/* set forked but preserve suid/gid state */
	u.u_acflag = AFORK | (u.u_acflag & ASUGID); 
	bzero(&u.u_ru, sizeof(u.u_ru));
	bzero(&u.u_cru, sizeof(u.u_cru));
}

/*
 * Is this slot of a process's table a segment of its own, with memory?
 */
#define	OWNSEG(sp) \
	(((sp)->ps_flag & (PS_USED|PS_TEXT)) == PS_USED && (sp)->ps_size)

/*
 * newproc --
 *	Create a new process -- the internal version of system call fork.
 *	It returns the new process, to the old one; or NULL if there
 *	is no memory for the new one's user structure.
 */
struct proc *
newproc(isvfork)
	int isvfork;
{
	register struct proc *rpp, *rip;
	register int n;
	static int pidchecked = 0;
	struct file *fp;
	int s, swapped;
	memaddr a, a2;
	register struct pseg __far *sp;
	struct pseg __far *psp, __far *csp;

	/*
	 * The new process's user structure and kernel stack must be in
	 * memory, whatever becomes of the rest of it.
	 */
	if ((a = malloc(coremap, USIZE)) == NULL)
		return (NULL);

	/*
	 * First, just locate a slot for a process
	 * and copy the useful info from this process into it.
	 * The panic "cannot happen" because fork has already
	 * checked for the existence of a slot.
	 */
	mpid++;
retry:
	if (mpid >= 30000) {
		mpid = 100;
		pidchecked = 0;
	}
	if (mpid >= pidchecked) {
		int doingzomb = 0;

		pidchecked = 30000;
		/*
		 * Scan the proc table to check whether this pid
		 * is in use.  Remember the lowest pid that's greater
		 * than mpid, so we can avoid checking for a while.
		 */
		rpp = allproc;
again:
		for (; rpp != NULL; rpp = rpp->p_nxt) {
			if (rpp->p_pid == mpid || rpp->p_pgrp == mpid) {
				mpid++;
				if (mpid >= pidchecked)
					goto retry;
			}
			if (rpp->p_pid > mpid && pidchecked > rpp->p_pid)
				pidchecked = rpp->p_pid;
			if (rpp->p_pgrp > mpid && pidchecked > rpp->p_pgrp)
				pidchecked = rpp->p_pgrp;
		}
		if (!doingzomb) {
			doingzomb = 1;
			rpp = zombproc;
			goto again;
		}
	}
	if ((rpp = freeproc) == NULL)
		panic("no procs");

	freeproc = rpp->p_nxt;			/* off freeproc */

	/*
	 * Make a proc table entry for the new process.
	 */
	rip = u.u_procp;
#ifdef QUOTA
	QUOTAMAP();
	u.u_quota->q_cnt++;
	QUOTAUNMAP();
#endif
	rpp->p_stat = SIDL;
	rpp->p_realtimer.it_value = 0;
	rpp->p_flag = SLOAD;
	rpp->p_uid = rip->p_uid;
	rpp->p_pgrp = rip->p_pgrp;
	rpp->p_nice = rip->p_nice;
	rpp->p_textp = rip->p_textp;
	rpp->p_pid = mpid;
	rpp->p_ppid = rip->p_pid;
	rpp->p_pptr = rip;
	rpp->p_time = 0;
	rpp->p_cpu = 0;
	rpp->p_sigmask = rip->p_sigmask;
	rpp->p_sigcatch = rip->p_sigcatch;
	rpp->p_sigignore = rip->p_sigignore;
	/* take along any pending signals like stops? */
#ifdef UCB_METER
	if (isvfork) {
		forkstat.cntvfork++;
		forkstat.sizvfork += rip->p_dsize;
	} else {
		forkstat.cntfork++;
		forkstat.sizfork += rip->p_dsize;
	}
#endif
	rpp->p_wchan = 0;
	rpp->p_slptime = 0;
	{
	struct proc **hash = &pidhash[PIDHASH(rpp->p_pid)];

	rpp->p_hash = *hash;
	*hash = rpp;
	}
	/*
	 * some shuffling here -- in most UNIX kernels, the allproc assign
	 * is done after grabbing the struct off of the freeproc list.  We
	 * wait so that if the clock interrupts us and vmtotal walks allproc
	 * the text pointer isn't garbage.
	 */
	rpp->p_nxt = allproc;			/* onto allproc */
	rpp->p_nxt->p_prev = &rpp->p_nxt;	/*   (allproc is never NULL) */
	rpp->p_prev = &allproc;
	allproc = rpp;

	/*
	 * Increase reference counts on shared objects.
	 */
	for (n = 0; n <= u.u_lastfile; n++) {
		fp = u.u_ofile[n];
		if (fp == NULL)
			continue;
		fp->f_count++;
	}
	if ((rip->p_textp != NULL) && !isvfork) {
		rip->p_textp->x_count++;
		rip->p_textp->x_ccount++;
	}
	u.u_cdir->i_count++;
	if (u.u_rdir)
		u.u_rdir->i_count++;

	/*
	 * The new process's memory.  A vfork child has the parent's, and
	 * the parent goes without until the child is done with it.  Any
	 * other gets a copy of each of the parent's own segments; if there
	 * is not the memory for all of them, the copies are made on the
	 * swap device and the child starts life swapped out.
	 */
	rpp->p_addr = a;
	psp = PSEG(rip);
	csp = PSEG(rpp);
	bcopy(psp, csp, NSEG * sizeof (struct pseg));
	rpp->p_dsize = rip->p_dsize;
	swapped = 0;
	if (!isvfork) {
		for (sp = csp, n = 0; n < NSEG; n++, sp++) {
			if (!OWNSEG(sp))
				continue;
			a2 = malloc(coremap, (size_t)sp->ps_size);
			if (a2 == NULL)
				break;
			copy(sp->ps_addr, a2, (u_int)sp->ps_size);
			sp->ps_addr = a2;
		}
		if (n < NSEG) {
			while (--n >= 0) {
				--sp;
				if (OWNSEG(sp)) {
					mfree(coremap, (size_t)sp->ps_size,
					    sp->ps_addr);
					sp->ps_addr = psp[n].ps_addr;
				}
			}
			swapped = 1;
		}
	}
	for (n = 0; n < NSEG; n++)
		setldt(rpp, n);

	/*
	 * Its user structure is a copy of this one, and it starts with
	 * the registers this process came into the kernel with.
	 */
	unew(rpp);

	if (swapped) {
		/*
		 * There was not enough core for the new process: write its
		 * image from this one's memory.
		 */
		rip->p_stat = SIDL;
		rpp->p_stat = SRUN;
		swapout(rpp, X_DONTFREE, X_OLDSIZE, X_OLDSIZE);
		rip->p_stat = SRUN;
	} else {
		s = splhigh();
		rpp->p_stat = SRUN;
		setrq(rpp);
		splx(s);
	}
	rpp->p_flag |= SSWAP;
	if (isvfork) {
		/*
		 *  Empty the parent's table, since the child now
		 *  has the segments.
		 *  Then wait for the child to finish with them.
		 */
		bzero(psp, NSEG * sizeof (struct pseg));
		rip->p_dsize = 0;
		rip->p_textp = NULL;
		rpp->p_flag |= SVFORK;
		rip->p_flag |= SVFPRNT;
		while (rpp->p_flag & SVFORK)
			sleep((caddr_t)rpp,PSWP+1);
		if ((rpp->p_flag & SLOAD) == 0)
			panic("newproc vfork");
		bcopy(csp, psp, NSEG * sizeof (struct pseg));
		bzero(csp, NSEG * sizeof (struct pseg));
		for (n = 0; n < NSEG; n++)
			setldt(rpp, n);
		u.u_dsize = rip->p_dsize = rpp->p_dsize;
		rpp->p_dsize = 0;
		rip->p_textp = rpp->p_textp;
		rpp->p_textp = NULL;
		rpp->p_flag |= SVFDONE;
		wakeup((caddr_t)rip);
		/* everything may have moved while the child had it */
		sureg();
		rip->p_flag &= ~SVFPRNT;
	}
	return (rpp);
}
