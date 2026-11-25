/*
** kernel/2.11bsd.c - Low level kernel access functions for 2.11BSD
**
** This program is in the public domain and may be used freely by anyone
** who wants to. 
**
** Last update: 17 June 2026 use swapdevname(3) in place of fixed swap device
**
** Please send bug fixes/bug reports to: Steven Schultz <sms@moe.2bsd.com>
*/

#include <stdio.h>
#include <nlist.h>
#include <syslog.h>
#include <unistd.h>
#include <stdarg.h>

#include "kvm.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#define KERNEL
#include <sys/file.h>
#undef	KERNEL
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <fcntl.h>
#include <sys/sysctl.h>
  
#include <netinet/in.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <net/route.h>
#include <netinet/in_pcb.h>

#include "identd.h"
#include "error.h"

#ifdef i286
/*
 * The 80286's kernel is one file, /unix.  Its network has a data segment
 * of its own, which /dev/kmem reaches by the segment's number in the
 * high 16 bits of the offset, and the network's pointers are offsets in
 * that segment: netoff is the number the symbols give _tcb.  A user
 * structure is in one of the kernel's slots, in its data segment, while
 * its process is the one in the slot, which slotproc says, and in memory
 * at p_addr otherwise, whether or not the process is swapped out.
 */
	struct nlist nl[] =
		{
#define	N_TCB		0
		{ "_tcb" },
#define	N_SLOTPROC	1
		{ "_slotproc" },
#define	N_USLOT		2
		{ "_uslot" },
		{ "" }
		};
#define	snl	nl
#define	SN_TCB	N_TCB

	static	int	kmemfd;
	static	int	getu();

#ifndef	_PATH_KMEM
#define	_PATH_KMEM	"/dev/kmem"
#endif
#else
	struct nlist nl[] =
		{
#define N_NETDATA 0
	  	{ "_netdata" },
	  	{ "" }
		};

	struct	nlist	snl[] =
		{
#define	SN_TCB	0
  		{ "_tcb" },
		{ "" }
		};

#define	kmemfd	memfd
#endif

	static	off_t	netoff;
	static struct inpcb tcb;
	static	int	memfd, swapfd;

/* Where the network's address A is, through kmemfd. */
#define	NETADDR(a)	((off_t)(u_short)(a) + netoff)

#ifndef	_PATH_MEM
#define	_PATH_MEM	"/dev/mem"
#endif

#ifndef	_PATH_UNIX
#define	_PATH_UNIX	"/unix"
#endif

#ifndef	_PATH_NETNIX
#define	_PATH_NETNIX	"/netnix"
#endif

	static	char	Msg_open[] = "open %s";
	static	char	Msg_nlist[] = "nlist %s";
	static	char	Msg_sysctl[] = "sysctl %s";
	static	char	Msg_readu[] = "can't read u pid %d";
	static	char	Msg_malloc[] = "malloc(%d)";
	static	char	Swap[20];
	static	char	Netnix[] = _PATH_NETNIX;

	static	struct	file	*sockp2fp();
	static	void	error(), error1();

#ifdef i286
int
k_open()
	{
	char	*p;

	if	(path_kmem)
		p = path_kmem;
	else
		p = _PATH_KMEM;
	kmemfd = open(p, O_RDONLY);
	if	(kmemfd < 0)
		error1(Msg_open, p);
	memfd = open(_PATH_MEM, O_RDONLY);
	if	(memfd < 0)
		error1(Msg_open, _PATH_MEM);

	if	(path_unix)
		p = path_unix;
	else
		p = _PATH_UNIX;
	if	(nlist(p, nl) != 0)
		error1(Msg_nlist, p);

	netoff = (off_t)nl[N_TCB].n_ovly << 16;
	return(0);
	}
#else
int
k_open()
	{
	u_int	netdata;
	char	*p;

	if	(path_kmem)
		p = path_kmem;
	else
		p = _PATH_MEM;
	memfd = open(p, O_RDONLY);
	if	(memfd < 0)
    		error1(Msg_open, p);

	swapdevname(Swap, sizeof (Swap));
	swapfd = open(Swap, O_RDONLY);
	if	(swapfd < 0)
		error1(Msg_open, Swap);
  
	if	(path_unix)
		p = path_unix;
	else
		p = _PATH_UNIX;
	if	(nlist(p, nl) != 0)
    		error1(Msg_nlist, p);

	if	(nlist(Netnix, snl) != 0)
		error1(Msg_nlist, Netnix);
	if	(getbuf(memfd, (off_t)nl[N_NETDATA].n_value, &netdata,
			sizeof (int), "netdata") == 0)
		exit(1);

	netoff = (off_t)ctob((long)netdata);
	return(0);
	}
#endif

/*
 * Get a piece of kernel memory with error handling.
 * Returns 1 if call succeeded, else 0 (zero).
*/
static int
getbuf(fd, addr, buf, len, what)
	int fd;
	off_t addr;
	void *buf;
	int len;
	char *what;
	{

	if	(lseek(fd, addr, L_SET) == -1)
		{
		if	(syslog_flag)
			syslog(LOG_ERR, "getbuf: lseek %ld - %s: %m",what,addr);
		return(0);
		}
	if	(read(fd, buf, len) != len)
		{
		if	(syslog_flag)
			syslog(LOG_ERR, "getbuf: read %d bytes - %s : %m",
				len, what);
		return(0);
  		}
	return(1);
	}

/*
 * Traverse the inpcb list until a match is found.
 * Returns NULL if no match.
*/
static struct socket *
getlist(pcbp, faddr, fport, laddr, lport)
	register struct inpcb *pcbp;
	register struct in_addr *faddr;
	int fport;
	register struct in_addr *laddr;
	int lport;
	{
	struct inpcb *head;

	if	(!pcbp)
		return(NULL);
	head = pcbp->inp_prev;
	do 
		{
		if	(pcbp->inp_faddr.s_addr == faddr->s_addr &&
			 pcbp->inp_laddr.s_addr == laddr->s_addr &&
			 pcbp->inp_fport        == fport &&
			 pcbp->inp_lport        == lport )
			return(pcbp->inp_socket);
		} while (pcbp->inp_next != head &&
			getbuf(kmemfd, NETADDR(pcbp->inp_next),
				pcbp, sizeof(struct inpcb), "tcblist"));
	return(NULL);
	}

/*
 * Return the user number for the connection owner
*/
int
k_getuid(faddr, fport, laddr, lport, uid)
	struct	in_addr *faddr;
	u_short	fport;
	struct	in_addr *laddr;
	u_short lport;
	uid_t	*uid;
	{
	register struct	proc	*pp;
	register struct	user	*up;
	struct	file	*fp;
	struct	kinfo_proc *kpp, *xproc, *endproc;
	struct	kinfo_file *xfile, *endfile;
	struct socket *sockp;
	register int i;
	int	mib[3], size;
	struct	user	uarea;
  
  /* -------------------- FILE DESCRIPTOR TABLE -------------------- */

	mib[0] = CTL_KERN;
	mib[1] = KERN_FILE;
	i = sysctl(mib, 2, NULL, &size, NULL, 0);
	if	(i == -1)
		error1(Msg_sysctl, "1");
	xfile = (struct kinfo_file *) malloc(size);
	if	(!xfile)
    		error1(Msg_malloc, size);
 	i = sysctl(mib, 2, xfile, &size, NULL, 0); 
	if	(i == -1)
		error1(Msg_sysctl, "2");
	endfile = &xfile[size / sizeof (struct kinfo_file)];

  /* -------------------- TCP PCB LIST -------------------- */
	if	(!getbuf(kmemfd, NETADDR(snl[SN_TCB].n_value), &tcb, 
			sizeof(tcb), "tcb"))
		{
		free(xfile);
		return(-1);
		}
	tcb.inp_prev = (struct inpcb *) snl[SN_TCB].n_value;
	sockp = getlist(&tcb, faddr, fport, laddr, lport);
	if	(!sockp)
		{
		free(xfile);
		return(-1);
		}
  /* ------------------- FIND FILE CONTAINING TCP CONTROL BLOCK --------- */
	fp = sockp2fp(xfile, endfile, sockp);
	if	(!fp)
		{
		free(xfile);
		return(-1);
		}

  /* -------------------- PROCESS TABLE ------------------- */
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_ALL;
	i = sysctl(mib, 3, NULL, &size, NULL, 0);
	if	(i == -1)
		error1(Msg_sysctl, "3");
	xproc = (struct kinfo_proc *)malloc(size);
	if	(!xproc)
    		error1(Msg_malloc, size);
	i = sysctl(mib, 3, xproc, &size, NULL, 0);
	if	(i == -1)
		error1(Msg_sysctl, "4");
	endproc = &xproc[size / sizeof (struct kinfo_proc)];

	for	(kpp = xproc; kpp < endproc; kpp++)
		{
		pp = &kpp->kp_proc;
		if	(pp->p_stat == SZOMB)
			continue;

    /* ------------------- GET U_AREA FOR PROCESS ----------------- */
#ifdef i286
		if	(getu(kpp, &uarea) == 0)
			error1(Msg_readu, pp->p_pid);
#else
		if	(pp->p_flag & SLOAD)
			{
			if	(getbuf(memfd, ctob((off_t)pp->p_addr),&uarea, 
					sizeof (uarea), "u") == 0)
				error1(Msg_readu, pp->p_pid);
			}
		else
			{
			lseek(swapfd, dbtob((off_t)pp->p_addr), L_SET);
			i = read(swapfd, &uarea, sizeof (uarea));
			if	(i != sizeof (uarea))
				error1(Msg_readu, pp->p_pid);
			}
#endif
		up = &uarea;

    /* ----------------- SCAN PROCESS's FILE TABLE ------------------- */
		for	(i = up->u_lastfile; i >= 0; i--)
			{
			if	(up->u_ofile[i] == fp)
				{
				*uid = up->u_ruid;
				free(xproc);
				free(xfile);
				return(0);
				} 		/* if */
			} 			/* for -- ofiles */
		}   				/* for -- proc */
	free(xproc);
	free(xfile);
	return -1;
	}

#ifdef i286
/*
 * Read the user structure of KPP's process into UP: from its slot, if
 * the process is the one there, else from memory at p_addr.
 * Returns 1 if it could be read, else 0.
 */
static int
getu(kpp, up)
	register struct kinfo_proc *kpp;
	struct user *up;
	{
	struct	proc	*inslot;
	int	slot = kpp->kp_proc.p_slot;

	if	(getbuf(kmemfd, (off_t)nl[N_SLOTPROC].n_value +
			slot * sizeof (inslot), &inslot, sizeof (inslot),
			"slotproc") == 0)
		return(0);
	if	(inslot == kpp->kp_eproc.e_paddr)
		return(getbuf(kmemfd, (off_t)nl[N_USLOT].n_value +
			(off_t)slot * UBYTES, up, sizeof (*up), "u"));
	return(getbuf(memfd, ctob((off_t)kpp->kp_proc.p_addr), up,
		sizeof (*up), "u"));
	}
#endif

static struct file *
sockp2fp(xfile, endfile, sockp)
	struct	kinfo_file *xfile, *endfile;
	struct	socket	*sockp;
	{
	register struct kinfo_file *kfp;
	register struct file *fp = NULL;

	for	(kfp = xfile; kfp < endfile; kfp++)
		{
		if	((kfp->kp_file.f_type == DTYPE_SOCKET) &&
			 (kfp->kp_file.f_data == sockp))
			{
			fp = kfp->kp_filep;
			break;
			}
		}
	return(fp);
	}

/*
 * These are helper functions which greatly reduce the bloat caused by
 * inline expansion of the ERROR and ERROR1 macros (this module went from
 * about 2700 bytes of code to 1400 bytes!).  Those macros _should_
 * have been written as variadic functions using vfprintf and vsyslog!
 *
 * error1() only accepts single word (int, not long) as a second arg.
*/

static void
error(msg)
	char	*msg;
	{
	ERROR(msg);
	/* NOTREACHED */
	}

/* ARGSUSED1 */
static void
error1(char *msg, ...)
	{
	va_list	ap;
	int	arg2;

	va_start(ap, msg);
	arg2 = va_arg(ap, int);
	va_end(ap);
	ERROR1(msg, arg2);
	/* NOTREACHED */
	}
