/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)init_sysent.c	1.16 (2.11BSD) 2000/2/20
 */

/*
 * System call switch table.
 */

#include "param.h"
#include "systm.h"
#include "user.h"

int	nosys();

/* 1.1 processes and protection */
int	getpid();
int	getppid(), fork(),rexit(),execv(),execve();
int	wait4(), getuid(),getgid(),getgroups(),setgroups();
int	geteuid(), getegid(), getlogin();
int	getpgrp(),setpgrp(), setlogin();
int	setgid(), setegid(), setuid(), seteuid();

/* 1.2 memory management */
int	sbrk();

/* 1.3 signals */
int	sigstack(),sigreturn();
int	sigaction(), sigprocmask(), sigpending(), sigaltstack(), sigsuspend();
int	sigwait(), kill(), killpg();

/* 1.4 timing and statistics */
int	gettimeofday(),settimeofday();
int	getitimer(),setitimer();
int	adjtime();

/* 1.5 descriptors */
int	getdtablesize(),dup(),dup2(),close();
int	pselect(),select(),fcntl(),flock();

/* 1.6 resource controls */
int	getpriority(),setpriority(),getrusage(),getrlimit(),setrlimit();
#ifdef QUOTA
int	setquota(),qquota();
#else
/*
 * Without quotas the calls say so with EINVAL, which is what login and
 * the quota programs look for, rather than end the caller with SIGSYS.
 */
#define	setquota	noquota
#define	qquota		noquota
static
noquota()
{
	u.u_error = EINVAL;
}
#endif

/* 1.7 system operation support */
int	umount(),smount();
int	sync(),reboot(),__sysctl();

/* 2.1 generic operations */
int	read(),write(),readv(),writev(),ioctl();

/* 2.2 file system */
int	chdir(), fchdir(), chroot();
int	mkdir(),rmdir(), chflags(), fchflags();
int	open(),mknod(),unlink(),stat(),fstat(),lstat();
int	chown(),fchown(),chmod(),fchmod(),utimes();
int	link(),symlink(),readlink(),rename();
int	lseek(),truncate(),ftruncate(),saccess(),fsync();
int	statfs(), fstatfs(), getfsstat();

/* 2.3 communications */
int	socket(),bind(),listen(),accept(),connect();
int	socketpair(),sendto(),send(),recvfrom(),recv();
int	sendmsg(),recvmsg(),shutdown(),setsockopt(),getsockopt();
int	getsockname(),getpeername(),pipe();

int	umask();		/* XXX */

/* 2.4 processes */
int	ptrace();

/* 2.5 terminals */

#ifdef INET
#define ifnet(narg, name)	narg, name
#define errnet(narg, name)	narg, name
#else
int	nonet();
#define ifnet(narg, name)	0, nosys
#define errnet(narg, name)	0, nonet
#endif

/* BEGIN JUNK */
int	profil();		/* 'cuz sys calls are interruptible */
int	vhangup();		/* should just do in exit() */
int	vfork();		/* awaiting fork w/ copy on write */
/* END JUNK */

/*
 * Reserved/unimplemented system calls in the range 0-150 inclusive
 * are reserved for use in future Berkeley releases.
 * Additional system calls implemented in vendor and other
 * redistributions should be placed in the reserved range at the end
 * of the current calls.
 */
/*
 * This table is the switch used to transfer to the appropriate routine for
 * processing a system call.  Each row contains the number of words of
 * arguments the call takes, and a pointer to the routine.  A pointer is
 * two words, an offset and a selector, and so is a long.  Up to six
 * words arrive in registers; a call with more passes the address of them
 * all instead -- see trap.c.
 */
struct sysent sysent[] = {
	1, nosys,			/*   0 = indir or out-of-range */
	1, rexit,			/*   1 = exit */
	0, fork,			/*   2 = fork */
	4, read,			/*   3 = read */
	4, write,			/*   4 = write */
	4, open,			/*   5 = open */
	1, close,			/*   6 = close */
	6, wait4,			/*   7 = wait4 */
	0, nosys,			/*   8 = (old creat) */
	4, link,			/*   9 = link */
	2, unlink,			/*  10 = unlink */
	6, execv,			/*  11 = execv */
	2, chdir,			/*  12 = chdir */
	1, fchdir,			/*  13 = fchdir */
	4, mknod,			/*  14 = mknod */
	3, chmod,			/*  15 = chmod */
	4, chown,			/*  16 = chown; now 3 args */
	3, chflags,			/*  17 = chflags */
	2, fchflags,			/*  18 = fchflags */
	4, lseek,			/*  19 = lseek */
	0, getpid,			/*  20 = getpid */
	5, smount,			/*  21 = mount */
	2, umount,			/*  22 = umount */
	10, __sysctl,			/*  23 = __sysctl */
	0, getuid,			/*  24 = getuid */
	0, geteuid,			/*  25 = geteuid */
	5, ptrace,			/*  26 = ptrace */
	0, getppid,			/*  27 = getppid */
	4, statfs,			/*  28 = statfs */
	3, fstatfs,			/*  29 = fstatfs */
	4, getfsstat,			/*  30 = getfsstat */
	7, sigaction,			/*  31 = sigaction */
	5, sigprocmask,			/*  32 = sigprocmask */
	3, saccess,			/*  33 = access */
	2, sigpending,			/*  34 = sigpending */
	4, sigaltstack,			/*  35 = sigaltstack */
	0, sync,			/*  36 = sync */
	2, kill,			/*  37 = kill */
	4, stat,			/*  38 = stat */
	3, getlogin,			/*  39 = getlogin */
	4, lstat,			/*  40 = lstat */
	1, dup,				/*  41 = dup */
	0, pipe,			/*  42 = pipe */
	2, setlogin,			/*  43 = setlogin */
	5, profil,			/*  44 = profil */
	1, setuid,			/*  45 = setuid */
	1, seteuid,			/*  46 = seteuid */
	0, getgid,			/*  47 = getgid */
	0, getegid,			/*  48 = getegid */
	1, setgid,			/*  49 = setgid */
	1, setegid,			/*  50 = setegid */
	0, nosys,			/*  51 = unused */
	0, nosys,			/*  52 = (2.9) set phys addr */
	0, nosys,			/*  53 = (2.9) lock in core */
	5, ioctl,			/*  54 = ioctl */
	1, reboot,			/*  55 = reboot */
	4, sigwait,			/*  56 = sigwait */
	4, symlink,			/*  57 = symlink */
	5, readlink,			/*  58 = readlink */
	6, execve,			/*  59 = execve */
	1, umask,			/*  60 = umask */
	2, chroot,			/*  61 = chroot */
	3, fstat,			/*  62 = fstat */
	0, nosys,			/*  63 = reserved */
	0, nosys,			/*  64 = (old getpagesize) */
	11, pselect,			/*  65 = pselect */
	0, vfork,			/*  66 = vfork */
	0, nosys,			/*  67 = unused */
	0, nosys,			/*  68 = unused */
	2, sbrk,			/*  69 = sbrk */
	0, nosys,			/*  70 = unused */
	0, nosys,			/*  71 = unused */
	0, nosys,			/*  72 = unused */
	0, nosys,			/*  73 = unused */
	0, nosys,			/*  74 = unused */
	0, nosys,			/*  75 = unused */
	0, vhangup,			/*  76 = vhangup */
	0, nosys,			/*  77 = unused */
	0, nosys,			/*  78 = unused */
	3, getgroups,			/*  79 = getgroups */
	3, setgroups,			/*  80 = setgroups */
	1, getpgrp,			/*  81 = getpgrp */
	2, setpgrp,			/*  82 = setpgrp */
	5, setitimer,			/*  83 = setitimer */
	0, nosys,			/*  84 = (old wait,wait3) */
	0, nosys,			/*  85 = unused */
	3, getitimer,			/*  86 = getitimer */
	0, nosys,			/*  87 = (old gethostname) */
	0, nosys,			/*  88 = (old sethostname) */
	0, getdtablesize,		/*  89 = getdtablesize */
	2, dup2,			/*  90 = dup2 */
	0, nosys,			/*  91 = unused */
	3, fcntl,			/*  92 = fcntl */
	9, select,			/*  93 = select */
	0, nosys,			/*  94 = unused */
	1, fsync,			/*  95 = fsync */
	3, setpriority,			/*  96 = setpriority */
	errnet(3, socket),		/*  97 = socket */
	ifnet(4, connect),		/*  98 = connect */
	ifnet(5, accept),		/*  99 = accept */
	2, getpriority,			/* 100 = getpriority */
	ifnet(5, send),			/* 101 = send */
	ifnet(5, recv),			/* 102 = recv */
	2, sigreturn,			/* 103 = sigreturn */
	ifnet(4, bind),			/* 104 = bind */
	ifnet(6, setsockopt),		/* 105 = setsockopt */
	ifnet(2, listen),		/* 106 = listen */
	2, sigsuspend,			/* 107 = sigsuspend */
	0, nosys,			/* 108 = (old sigvec) */
	0, nosys,			/* 109 = (old sigblock) */
	0, nosys,			/* 110 = (old sigsetmask) */
	0, nosys,			/* 111 = (old sigpause)  */
	4, sigstack,			/* 112 = sigstack COMPAT-43 */
	ifnet(4, recvmsg),		/* 113 = recvmsg */
	ifnet(4, sendmsg),		/* 114 = sendmsg */
	0, nosys,			/* 115 = unused */
	4, gettimeofday,		/* 116 = gettimeofday */
	3, getrusage,			/* 117 = getrusage */
	ifnet(7, getsockopt),		/* 118 = getsockopt */
	0, nosys,			/* 119 = unused */
	4, readv,			/* 120 = readv */
	4, writev,			/* 121 = writev */
	4, settimeofday,		/* 122 = settimeofday */
	3, fchown,			/* 123 = fchown */
	2, fchmod,			/* 124 = fchmod */
	ifnet(9, recvfrom),		/* 125 = recvfrom */
	0, nosys,			/* 126 = (old setreuid) */
	0, nosys,			/* 127 = (old setregid) */
	4, rename,			/* 128 = rename */
	4, truncate,			/* 129 = truncate */
	3, ftruncate,			/* 130 = ftruncate */
	2, flock,			/* 131 = flock */
	0, nosys,			/* 132 = nosys */
	ifnet(8, sendto),		/* 133 = sendto */
	ifnet(2, shutdown),		/* 134 = shutdown */
	errnet(5, socketpair),		/* 135 = socketpair */
	3, mkdir,			/* 136 = mkdir */
	2, rmdir,			/* 137 = rmdir */
	4, utimes,			/* 138 = utimes */
	0, nosys,			/* 139 = unused */
	4, adjtime,			/* 140 = adjtime */
	ifnet(5, getpeername),		/* 141 = getpeername */
	0, nosys,			/* 142 = (old gethostid) */
	0, nosys,			/* 143 = (old sethostid) */
	3, getrlimit,			/* 144 = getrlimit */
	3, setrlimit,			/* 145 = setrlimit */
	2, killpg,			/* 146 = killpg */
	0, nosys,			/* 147 = nosys */
	4, setquota,			/* 148 = quota */
	5, qquota,			/* 149 = qquota */
	ifnet(5, getsockname),		/* 150 = getsockname */
	/*
	 * Syscalls 151-180 inclusive are reserved for vendor-specific
	 * system calls.  (This includes various calls added for compatibity
	 * with other Unix variants.)
	 */

	/*
	 * 2BSD special calls
	 */
	0, nosys,			/* 151 = unused */
	0, nosys,			/* 152 = nostk */
	0, nosys,			/* 153 = fetchi */
	0, nosys,			/* 154 = ucall */
	0, nosys,			/* 155 = fperr */
};

int	nsysent = sizeof (sysent) / sizeof (sysent[0]);
