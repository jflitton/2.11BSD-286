/*
 * Exceptions and system calls.  mch_trap.s calls trap and syscall with
 * the registers of what was interrupted, as a struct trapframe on the
 * kernel stack.
 */
#include "param.h"
#include "../machine/psl.h"
#include "../machine/reg.h"
#include "../machine/frame.h"
#include "../machine/trap.h"
#include "systm.h"
#include "user.h"
#include "proc.h"
#include "signalvar.h"
#include "vm.h"
#include <syscall.h>

extern caddr_t nofault;
extern int sctrace;
extern int cpl;				/* machdep.c */
extern u_int ipending, spending;
int	nosys();			/* mch_copy.s */
extern int trapret(), tr_popes(), tr_popds(), tr_iret();

/*
 * Called from mch_trap.s when the processor takes an exception.  The
 * frame's tf_vec says which.  From a process most of them are a signal
 * to it.  From the kernel only two things are expected: a fault in one
 * of the routines that copy to and from a process, which have left a
 * recovery address in nofault; and a fault on the way back from the
 * kernel, to a process or to the kernel itself after an interrupt,
 * loading a selector that is no longer good.
 */
trap(tf)
	register struct trapframe *tf;
{
	register struct proc *p;
	register int i;
	time_t syst;

#ifdef UCB_METER
	cnt.v_trap++;
#endif
	if (!USERMODE(tf->tf_cs)) {
		ktrap(tf);
		return;
	}
	enable();
	syst = u.u_ru.ru_stime;
	p = u.u_procp;
	u.u_ar0 = (int *)tf;
	switch (tf->tf_vec) {

	case T_NOFPU:
		fpdna();
		goto out;

	case T_DIVIDE:
	case T_OFLOW:
	case T_BOUND:
		i = SIGFPE;
		u.u_code = tf->tf_vec;
		break;

	case T_DEBUG:
	case T_BPT:
		i = SIGTRAP;
		tf->tf_flags &= ~PSL_T;
		break;

	case T_BADOP:
		i = SIGILL;
		u.u_code = tf->tf_vec;
		break;

	case T_FPUSEG:
		/* the 287 is in no state to go on with */
		fpdrop();
		i = SIGSEGV;
		break;

	case T_SEGNP:
		i = SIGBUS;
		break;

	case T_STACK:
	case T_PROT:
		i = SIGSEGV;
		break;

	case T_FPUERR:
		i = SIGFPE;
		u.u_code = tf->tf_vec;
		break;

	default:
		printf("trap type %d from a process\n", tf->tf_vec);
		i = SIGILL;
		break;
	}
	if (sctrace)
		printf("%s[%d]: trap %d at %x:%x\n", u.u_comm, p->p_pid,
		    tf->tf_vec, tf->tf_cs, tf->tf_ip);
	psignal(p, i);
out:
	userret(tf, syst);
}

/*
 * The way back to a process from a trap or a system call: take the
 * signals that are waiting for it, and let another process run if one
 * should.
 */
static
userret(tf, syst)
	register struct trapframe *tf;
	time_t syst;
{
	register struct proc *p = u.u_procp;
	register int i;

	while (i = CURSIG(p))
		postsig(i);
	/*
	 * curpri = setpri(p), in line: this is the way out of every
	 * system call.
	 */
	i = (p->p_cpu & 0377) / 16 + PUSER + p->p_nice;
	if (i > 127)
		i = 127;
	if (i < curpri)
		runrun++;
	curpri = p->p_pri = i;
	if (runrun) {
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
	}
	if (u.u_prof.pr_scale)
		addupc(tf->tf_ip, &u.u_prof, (int)(u.u_ru.ru_stime - syst));
	/*
	 * A process runs at level 0, as the PDP-11's return to user mode
	 * makes it by restoring the PS.  Here the level is a variable, and
	 * swtch leaves it where the process that switched set it: a new
	 * process, or one that was preempted, would otherwise run with
	 * every interrupt put off until the kernel next lowered it.
	 * Most often the level is 0 already and nothing is waiting for
	 * it to drop, and there is nothing to call spl0 for: what
	 * arrives at level 0 is not put off.
	 */
	if (cpl | ipending | spending)
		spl0();
}

/*
 * Called from mch_trap.s on the way back to a process from an interrupt,
 * when another process should run.
 */
ast(tf)
	struct trapframe *tf;
{
	u.u_ar0 = (int *)tf;
	userret(tf, u.u_ru.ru_stime);
}

/*
 * A new process starts here, called from forkret in mch_swtch.s on its
 * own kernel stack, with the frame it will go to its program by.  The
 * first process is given a program to go to.  Any other has its
 * parent's frame, in the middle of fork, and gets what fork returns in
 * a child: the parent's process ID, and 1 in DX to say which it is.
 */
childret(tf)
	register struct trapframe *tf;
{
	u.u_ar0 = (int *)tf;
	if (u.u_procp->p_pid == 1)
		icodestart(tf);
	else {
		tf->tf_ax = u.u_procp->p_ppid;
		tf->tf_dx = 1;
		tf->tf_flags &= ~PSL_C;
	}
	forkchild();
	userret(tf, u.u_ru.ru_stime);
}

/*
 * An exception in the kernel.
 */
static
ktrap(tf)
	register struct trapframe *tf;
{
	register struct trapframe *utf;
	register int ip = tf->tf_ip;

	switch (tf->tf_vec) {

	case T_SEGNP:
	case T_STACK:
	case T_PROT:
		if (nofault) {
			tf->tf_ip = (u_int)nofault;
			return;
		}
		/*
		 * A selector the process left in its frame could not be
		 * loaded.  The processor's frame for this fault lies
		 * directly below what was being popped, so tf_sp here is
		 * that word of the process's frame.  A data segment
		 * register gets the null selector instead, which always
		 * loads, and the process a signal.  If it is the return
		 * itself that failed there is nowhere to send the
		 * process, and it ends.
		 */
		if (ip == (u_int)tr_popes) {
			utf = (struct trapframe *)
			    ((caddr_t)&tf->tf_sp - offsetof(struct trapframe, tf_es));
			utf->tf_es = 0;
			/*
			 * The frame may be the kernel's own, of an
			 * interrupt that found it running.  The kernel
			 * leaves in ES whatever its last far pointer put
			 * there, a process's selector as often as not, and
			 * goes on to another process, or to a new program
			 * for the same one, that has no such segment.  What
			 * was interrupted was not using ES, or would have
			 * loaded it since; it goes on with the null
			 * selector, and the process has done nothing.
			 */
			if (!USERMODE(utf->tf_cs))
				return;
		} else if (ip == (u_int)tr_popds) {
			utf = (struct trapframe *)
			    ((caddr_t)&tf->tf_sp - offsetof(struct trapframe, tf_ds));
			utf->tf_ds = 0;
		} else if (ip == (u_int)tr_iret) {
			enable();
			exit(SIGSEGV);
			/* NOTREACHED */
		} else
			break;
		psignal(u.u_procp, SIGSEGV);
		return;

	case T_NOFPU:
		/* the kernel itself touched the 287, to save or load it */
		fpclts();
		return;
	}
	disable();
	printf("trap type %d err %x in the kernel\n", tf->tf_vec, tf->tf_err);
	printf("cs:ip %x:%x flags %x ds %x es %x\n", tf->tf_cs, tf->tf_ip,
	    tf->tf_flags, tf->tf_ds, tf->tf_es);
	printf("ax %x bx %x cx %x dx %x si %x di %x bp %x sp %x\n",
	    tf->tf_ax, tf->tf_bx, tf->tf_cx, tf->tf_dx, tf->tf_si, tf->tf_di,
	    tf->tf_bp, (u_int)&tf->tf_sp);
	panic("trap");
}

/*
 * Called from mch_trap.s when a process makes a system call: the number
 * in AX and six words of arguments in BX, CX, DX, SI, DI and BP, which
 * are next to each other in the frame.  A call with more than six words
 * of arguments passes the far address of them all instead.  The words
 * are copied to u.u_arg, so the registers in the frame are still what
 * the process had if the call is to be started again.
 *
 * For bringing up programs, sctrace (tools/kpatch.py) makes every call
 * that fails, every exit and every program executed, or with 2 every
 * call, print its process, number and result.
 */
int	sctrace = 0;

/* The registers that carry arguments, to copy as one in line. */
struct argreg {
	int	a[NARGREG];
};

syscall(tf)
	register struct trapframe *tf;
{
	extern int nsysent;
	register int code;
	register struct sysent *callp;
	time_t syst;
	int i;

#ifdef UCB_METER
	cnt.v_syscall++;
#endif
	syst = u.u_ru.ru_stime;
	u.u_ar0 = (int *)tf;
	u.u_error = 0;
	code = tf->tf_ax;
	if ((u_int)code >= nsysent)
		callp = &sysent[0];	/* indir (illegal) */
	else
		callp = &sysent[code];
	if (callp->sy_narg <= NARGREG)
		*(struct argreg *)u.u_arg = *(struct argreg *)tf_arg(tf);
	else
		u.u_error = copyin(UADDR(tf->tf_cx, tf->tf_bx),
		    (caddr_t)u.u_arg, callp->sy_narg * sizeof (int));
	u.u_ap = u.u_arg;
	u.u_r.r_val1 = 0;
	u.u_r.r_val2 = tf->tf_dx;
	if (sctrace)
		sctrace1(code, callp);
	if (u.u_error == 0 && setjmp(&u.u_qsave) == 0)
		(*callp->sy_call)();
	switch (u.u_error) {
	case 0:
		tf->tf_flags &= ~PSL_C;
		tf->tf_ax = u.u_r.r_val1;
		tf->tf_dx = u.u_r.r_val2;
		break;
	case ERESTART:
		tf->tf_ip -= 2;		/* back to the INT instruction */
		break;
	case EJUSTRETURN:
		break;
	default:
		tf->tf_flags |= PSL_C;
		tf->tf_ax = u.u_error;
		break;
	}
	if (sctrace)
		sctrace2(code);
	userret(tf, syst);
}

/*
 * What sctrace prints, before a call and after it: out of syscall's
 * way, which is every call's.
 */
static
sctrace1(code, callp)
	int code;
	register struct sysent *callp;
{
	if (code == SYS_exit)
		printf("%s[%d]: exit %d\n", u.u_comm, u.u_procp->p_pid, u.u_arg[0]);
	if (callp->sy_call == nosys)
		printf("%s[%d]: call %d is not in this kernel\n", u.u_comm,
		    u.u_procp->p_pid, code);
}

static
sctrace2(code)
	int code;
{
	if (sctrace > 1 || u.u_error && u.u_error != EJUSTRETURN ||
	    code == SYS_execve && u.u_error == EJUSTRETURN)
		printf("%s[%d]: call %d: %s %d\n", u.u_comm, u.u_procp->p_pid,
		    code, u.u_error ? "error" : "result",
		    u.u_error ? u.u_error : u.u_r.r_val1);
}

/*
 * nonexistent system call-- signal process (may want to handle it)
 * flag error if process won't see signal immediately
 * Q: should we do that all the time ??
 */
nosys()
{
	if (u.u_signal[SIGSYS] == SIG_IGN || u.u_signal[SIGSYS] == SIG_HOLD)
		u.u_error = EINVAL;
	psignal(u.u_procp, SIGSYS);
}
