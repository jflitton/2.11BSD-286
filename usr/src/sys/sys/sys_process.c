/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)sys_process.c	1.2 (2.11BSD) 1999/9/5
 */

#include "param.h"
#include "../machine/psl.h"
#include "../machine/reg.h"
#include "../machine/seg.h"

#include "systm.h"
#include "user.h"
#include "proc.h"
#include "inode.h"
#include "text.h"
#include "vm.h"
#include "ptrace.h"

/*
 * Tracing variables.
 * Used to pass trace command from
 * parent to child being traced.
 * This data base cannot be
 * shared and is locked
 * per user.
 */
struct
{
	int	ip_lock;
	int	ip_req;
	char __far *ip_addr;
	int	ip_data;
} ipc;

/*
 * sys-trace system call.
 */
ptrace()
{
	register struct proc *p;
	register struct a {
		int	req;
		int	pid;
		int __far *addr;
		int	data;
	} *uap;

	uap = (struct a *)u.u_ap;
	if (uap->req <= 0) {
		u.u_procp->p_flag |= P_TRACED;
		return;
	}
	p = pfind(uap->pid);
	if (p == 0 || p->p_stat != SSTOP || p->p_ppid != u.u_procp->p_pid ||
	    !(p->p_flag & P_TRACED)) {
		u.u_error = ESRCH;
		return;
	}
	while (ipc.ip_lock)
		sleep((caddr_t)&ipc, PZERO);
	ipc.ip_lock = p->p_pid;
	ipc.ip_data = uap->data;
	ipc.ip_addr = (char __far *)uap->addr;
	ipc.ip_req = uap->req;
	p->p_flag &= ~P_WAITED;
	setrun(p);
	while (ipc.ip_req > 0)
		sleep((caddr_t)&ipc, PZERO);
	u.u_r.r_val1 = (short)ipc.ip_data;
	if (ipc.ip_req < 0)
		u.u_error = EIO;
	ipc.ip_lock = 0;
	wakeup((caddr_t)&ipc);
}

/*
 * Code that the child process
 * executes to implement the command
 * of the parent process in tracing.
 */
procxmt()
{
	register int i;
	register *p;
	register struct text *xp;
	register struct pseg __far *sp;

	if (ipc.ip_lock != u.u_procp->p_pid)
		return(0);
	u.u_procp->p_slptime = 0;
	i = ipc.ip_req;
	ipc.ip_req = 0;
	wakeup((caddr_t)&ipc);
	switch (i) {

	/* read user I */
	/* read user D */
	case PT_READ_I:
	case PT_READ_D:
		if (fubyte(ipc.ip_addr) == -1)
			goto error;
		ipc.ip_data = fuword(ipc.ip_addr);
		break;

	/* read u */
	case PT_READ_U:
		i = UOFF(ipc.ip_addr);
		if (i<0 || i >= UBYTES)
			goto error;
		ipc.ip_data = ((physadr)&u)->r[i/sizeof(int)];
		break;

	/* write user I */
	/* Must set up to allow writing */
	case PT_WRITE_I:
		/*
		 * If text, must assure exclusive use
		 */
		if (xp = u.u_procp->p_textp) {
			if (xp->x_count!=1 || xp->x_iptr->i_mode&ISVTX)
				goto error;
			xp->x_flag |= XTRC;
		}
		/*
		 * A code segment is made writable data while it is
		 * written.
		 */
		i = LSLOT(USEL(ipc.ip_addr));
		if (!ISLSEL(USEL(ipc.ip_addr)) || i >= NSEG)
			goto error;
		sp = &PSEG(u.u_procp)[i];
		if (sp->ps_flag & PS_TEXT) {
			sp->ps_flag |= PS_WTEXT;
			setldt(u.u_procp, i);
		}
		if ((i = suword(ipc.ip_addr, 0)) >= 0)
			suword(ipc.ip_addr, ipc.ip_data);
		if (sp->ps_flag & PS_TEXT) {
			sp->ps_flag &= ~PS_WTEXT;
			setldt(u.u_procp, LSLOT(USEL(ipc.ip_addr)));
		}
		if (i<0)
			goto error;
		if (xp)
			xp->x_flag |= XWRIT;
		break;

	/* write user D */
	case PT_WRITE_D:
		if (suword(ipc.ip_addr, 0) < 0)
			goto error;
		suword(ipc.ip_addr, ipc.ip_data);
		break;

	/* write u */
	/* the 287's registers, and the process's own in its frame */
	case PT_WRITE_U:
		i = UOFF(ipc.ip_addr);
		if (i<0 || i >= UBYTES)
			goto error;
		p = (int *)&((physadr)&u)->r[i/sizeof(int)];
		if (p >= (int *)&u.u_fps && p < (int *)(&u.u_fps + 1))
			goto ok;
		if (p < u.u_ar0 || p > &u.u_ar0[SS])
			goto error;
		switch (p - u.u_ar0) {
		case CS:
			if (!selok(ipc.ip_data, 1))
				goto error;
			goto ok;
		case SS:
			if (!selok(ipc.ip_data, 2))
				goto error;
			goto ok;
		case DS:
		case ES:
			if (ipc.ip_data && !selok(ipc.ip_data, 0))
				goto error;
			goto ok;
		case FLAGS:
			ipc.ip_data |= PSL_USERSET;	/* interrupts on */
			ipc.ip_data &= ~PSL_USERCLR;	/* no I/O */
			goto ok;
		case BX: case CX: case DX: case SI: case DI: case BP:
		case AX: case IP: case SP:
			goto ok;
		}
		goto error;

	ok:
		*p = ipc.ip_data;
		break;

	/* set signal and continue */
	/*  one version causes a trace-trap */
	case PT_STEP:
		u.u_ar0[RPS] |= PSL_T;
		/* FALL THROUGH TO ... */
	case PT_CONTINUE:
		if (ipc.ip_addr != (char __far *)1) {
			if (!selok(USEL(ipc.ip_addr), 1))
				goto error;
			u.u_ar0[CS] = USEL(ipc.ip_addr);
			u.u_ar0[PC] = UOFF(ipc.ip_addr);
		}
		if (ipc.ip_data > NSIG)
			goto error;
		u.u_procp->p_ptracesig = ipc.ip_data;
		return(1);

	/* force exit */
	case PT_KILL:
		exit(u.u_procp->p_ptracesig);
		/*NOTREACHED*/

	default:
	error:
		ipc.ip_req = -1;
	}
	return(0);
}
