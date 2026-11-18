/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)user.h	1.6 (2.11BSD) 1999/9/13
 */

#ifdef KERNEL
#include "../machine/fperr.h"
#include "dir.h"
#include "exec.h"
#include "time.h"
#include "resource.h"
#else
#include <machine/fperr.h>
#include <sys/dir.h>
#include <sys/exec.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

/*
 * Per-process data that the kernel needs only while the process runs.
 * The user structure and the process's kernel stack are one piece of
 * USIZE clicks, the stack growing down from the top toward the structure.
 * A process runs with them in one of the kernel's slots (machdep.c) and
 * uptr pointing there, so that u is the running process's; while another
 * process has the slot they are kept at p_addr.  It is cross referenced
 * with the proc structure for the same process.
 */
#define	MAXCOMLEN	MAXNAMLEN	/* <= MAXNAMLEN, >= sizeof(ac_comm) */

struct	pcb {			/* fake pcb structure */
	int	(*pcb_sigc)();	/* pointer to trampoline code in user space */
};

struct	fps {
	short	u_fpstate[47];	/* the 287, as FSAVE leaves it */
};

struct user {
	struct	pcb u_pcb;
	struct	fps u_fps;
	short	u_fpsaved;		/* FP regs saved for this proc */
	struct	fperr u_fperr;		/* floating point error save */
	struct	proc *u_procp;		/* pointer to proc structure */
	int	*u_ar0;			/* address of users saved R0 */
	char	u_comm[MAXCOMLEN + 1];

/* syscall parameters, results and catches */
	int	u_arg[12];		/* arguments to current system call */
	int	*u_ap;			/* pointer to arglist */
	label_t	u_qsave;		/* for non-local gotos on interrupts */
	union {				/* syscall return values */
		struct	{
			int	R_val1;
			int	R_val2;
		} u_rv;
#define	r_val1	u_rv.R_val1
#define	r_val2	u_rv.R_val2
		long	r_long;
		off_t	r_off;
		time_t	r_time;
	} u_r;
	char	u_error;		/* return error code */
	char	u_dummy0;

/* 1.1 - processes and protection */
	uid_t	u_uid;			/* effective user id */
	uid_t	u_svuid;		/* saved user id */
	uid_t	u_ruid;			/* real user id */
	gid_t	u_svgid;		/* saved group id */
	gid_t	u_rgid;			/* real group id */
	gid_t	u_groups[NGROUPS];	/* groups, 0 terminated */

/* 1.2 - memory management */
	size_t	u_tsize;		/* text size (clicks) */
	size_t	u_dsize;		/* data size (clicks) */
	size_t	u_ssize;		/* stack size (clicks) */
	label_t	u_ssave;		/* label variable for swapping */
	label_t	u_rsave;		/* save info when exchanging stacks */
	short	u_dseg;			/* the segment that holds the stack,
					   the data and the break */
	u_short	u_stack0;		/* size of the stack at its bottom */
	u_short	u_break;		/* the break: its size in bytes
					   (0 is 65536) */
	short	u_model;		/* memory model of the program */

/* 1.3 - signal management */
	int	(*u_signal[NSIG])();	/* disposition of signals */
	long	u_sigmask[NSIG];	/* signals to be blocked */
	long	u_sigonstack;		/* signals to take on sigstack */
	long	u_sigintr;		/* signals that interrupt syscalls */
	long	u_oldmask;		/* saved mask from before sigpause */
	int	u_code;			/* ``code'' to trap */
	char	dummy2;			/* Room for another flags byte */
	char	u_psflags;		/* Process Signal flags */
	struct	sigaltstack u_sigstk;	/* signal stack info */

/* 1.4 - descriptor management */
	struct	file *u_ofile[NOFILE];	/* file structures for open files */
	char	u_pofile[NOFILE];	/* per-process flags of open files */
	int	u_lastfile;		/* high-water mark of u_ofile */
#define	UF_EXCLOSE 	0x1		/* auto-close on exec */
#define	UF_MAPPED 	0x2		/* mapped from device */
	struct	inode *u_cdir;		/* current directory */
	struct	inode *u_rdir;		/* root directory of current process */
	struct	tty *u_ttyp;		/* controlling tty pointer */
	dev_t	u_ttyd;			/* controlling tty dev */
	short	u_cmask;		/* mask for file creation */

/* 1.5 - timing and statistics */
	struct	k_rusage u_ru;		/* stats for this proc */
	struct	k_rusage u_cru;		/* sum of stats for reaped children */
	struct	k_itimerval u_timer[2];	/* profile/virtual timers */
	long	u_start;
	char	u_acflag;
	char	u_dupfd;		/* XXX - see kern_descrip.c/fdopen */

	struct uprof {			/* profile arguments */
		short __far *pr_base;	/* buffer base */
		unsigned pr_size;	/* buffer size */
		unsigned pr_off;	/* pc offset */
		unsigned pr_scale;	/* pc scaling */
	} u_prof;

/* 1.6 - resource controls */
	struct	rlimit u_rlimit[RLIM_NLIMITS];
	struct	quota *u_quota;		/* user's quota structure */

/* namei & co. */
	struct	nameicache {		/* last successful directory search */
		off_t nc_prevoffset;	/* offset at which last entry found */
		ino_t nc_inumber;	/* inum of cached directory */
		dev_t nc_dev;		/* dev of cached directory */
	} u_ncache;
	short	u_xxxx[2];		/* spare */
	char	u_login[MAXLOGNAME];	/* setlogin/getlogin */
	short	u_stack[1];		/* kernel stack per user
					 * extends from u + USIZE*64
					 * backward not to reach here
					 */
};

#include <sys/errno.h>

#ifdef KERNEL
#ifdef SUPERVISOR
/*
 * The network's view of uptr, which KScall and SKcall keep (mch_net.s):
 * the network's data segment cannot hold the kernel's own.
 */
extern	struct user KSPACE *netuptr;
#define	u	(*netuptr)
#else
extern	struct user *uptr;	/* the running process's */
#define	u	(*uptr)
#endif
#endif
