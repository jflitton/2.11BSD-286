/*
 * For the kernel's test programs.
 */
struct ksigaction {
	int	(*sa_handler)();	/* a far address, in two halves */
	unsigned sa_seg;
	long	sa_mask;
	int	sa_flags;
};

struct sigcontext {
	int	sc_onstack;
	long	sc_mask;
	int	sc_ax, sc_bx, sc_cx, sc_dx, sc_si, sc_di, sc_bp;
	int	sc_ds, sc_es;
	int	sc_ip, sc_cs, sc_flags, sc_sp, sc_ss;
};

extern int errno;
extern int checks, failed;
extern char **environ;
int	sigtramp();
long	signal();
char	*sbrk();
long	lseek();

#define	SIGHUP	1
#define	SIGINT	2
#define	SIGKILL	9
#define	SIGSEGV	11
#define	SIGPIPE	13
#define	SIGTERM	15
#define	SIGCHLD	20
#define	SIGUSR1	30

#define	O_RDONLY	0
#define	O_WRONLY	1
#define	O_RDWR		2
#define	O_CREAT		0x0200
#define	O_TRUNC		0x0400

#define	ENOENT	2
#define	ENXIO	6
#define	EBADF	9
#define	ECHILD	10
#define	EFAULT	14
#define	ENOMEM	12
