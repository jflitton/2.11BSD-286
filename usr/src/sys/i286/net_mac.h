/*
 * Calls between the kernel and the network, which has a data segment of
 * its own (docs/network.md).
 *
 * The network runs on the kernel's stacks with DS its own segment, so SS
 * is always the kernel's data segment.  KScall(func, nbytes, args...)
 * calls a network function from the kernel; SKcall a kernel function
 * from the network.  Each copies nbytes of arguments to a new frame,
 * makes DS the callee's segment for the call and puts the caller's back
 * after it (mch_net.s).
 *
 * A pointer that crosses means nothing on the other side unless it is
 * only handed back, as the kernel holds a socket or the network a
 * process.  The network reaches the kernel's data through SS, with
 * KSPACE pointers (machparam.h): that is how it sees u, and the
 * structures a system call keeps on its stack.  The kernel never looks
 * at the network's data.  So the calls that take a pointer to something
 * on the kernel's stack, which on the PDP-11 both sides could read, go to
 * the k* entry points in i286/net_sys.c, which copy it to or from an
 * mbuf; and those whose pointer arguments are the network's own locals,
 * which are on the kernel's stack too, cast them to the kernel's near
 * pointers here.
 *
 * Taken from the PDP-11's (pdp/net_mac.h), where both sides could read
 * the kernel's stack.  The serial line interface's calls are left out
 * (it is not configured).
 */

int	KScall(), SKcall();

/*
 * Each routine one side calls in the other has a prototype here, where
 * both sides see it: what the caller passes and what the routine takes
 * are one declaration, which the compiler holds the definition to.
 */
struct	socket; struct mbuf; struct uio; struct file; struct inode;
struct	proc; struct stat;
int	netintr(void);
int	net_sysctl(int KSPACE *name, u_int namelen, void __far *oldp,
	    size_t KSPACE *oldlenp, void __far *newp, size_t newlen);
int	ufavail(void);

#ifdef SUPERVISOR
/*
 * The network's own copies of kernel routines that would take pointers
 * to its data (i286/net_sys.c), under names of their own in the one image
 * the two are linked into; and kernel routines it calls through SKcall,
 * which upstream's network had copies of.
 */
#define	printf		netprintf
#define	panic		netpanic
#define	uiomove		netuiomove
#define	hz		nethz
#define	mfree		mfreelist	/* the kernel's is a routine */
#define	sysctl_int	netsysctl_int
int	sysctl_int(void __far *oldp, size_t KSPACE *oldlenp,
	    void __far *newp, size_t newlen, int *valp);
int	suser(), ufalloc();
#define	suser()		SKcall(suser, 0)
#define	ufalloc(i)	SKcall(ufalloc, sizeof(int), i)
#define	ufavail()	SKcall(ufavail, 0)

/*
 * The kernel's variables that the network reads: their addresses in the
 * kernel's data segment, which it reaches through SS (mch_net.s).  It
 * must never name a kernel variable itself; the address would be taken
 * in its own segment.
 */
struct	timeval;
extern struct timeval KSPACE *ktime;
extern int KSPACE *klbolt;
extern caddr_t kselwait;
#endif

struct socket *asoqremque(struct socket *so, int n);
#define	ASOQREMQUE(so, n) \
	KScall(asoqremque, sizeof(struct socket *) + sizeof(int), so, n)

int connwhile(struct socket *so);
#define	CONNWHILE(so) \
	KScall(connwhile, sizeof(struct socket *), so)

int fpfetch(struct file *fp, struct file *fpp);
#define	FPFETCH(fp, fpp) \
	SKcall(fpfetch, sizeof(struct file *) + sizeof(struct file *), \
	    fp, (struct file *)(fpp))

void fpflags(struct file *fp, int set, int clear);
#define	FPFLAGS(fp, set, clear) \
	SKcall(fpflags, sizeof(struct file *) + sizeof(int) + \
	    sizeof(int), fp, set, clear)

int gsignal();
#define	GSIGNAL(pgrp, sig) \
	SKcall(gsignal, sizeof(int) + sizeof(int), pgrp, sig)

struct mbuf *m_free(struct mbuf *m);
#define	M_FREE(m) \
	KScall(m_free, sizeof(struct mbuf *), m)

int m_freem(struct mbuf *m);
#define	M_FREEM(m) \
	KScall(m_freem, sizeof(struct mbuf *), m)

int netcopyout(struct mbuf *m, uaddr_t to, int KSPACE *len);
#define	NETCOPYOUT(from, to, len) \
	KScall(netcopyout, sizeof(struct mbuf *) + sizeof(uaddr_t) + \
	    sizeof(int *), from, (uaddr_t)(to), len)

int netcrash(void);
#define	NETCRASH() \
	SKcall(netcrash, 0)

int iput();
#define	IPUT(ip) \
	SKcall(iput, sizeof(struct inode *), ip)

struct proc *netpfind(int pid);
#define	NETPFIND(pid) \
	SKcall(netpfind, sizeof(int), pid)

int netpsignal(struct proc *p, int sig);
#define	NETPSIGNAL(p, sig) \
	SKcall(netpsignal, sizeof(struct proc *) + sizeof(int), p, sig)

void netsethz(int ticks);
#define	NETSETHZ() \
	KScall(netsethz, sizeof (hz), hz)

int netstart(void);
#define	NETSTART() \
	KScall(netstart, 0)

int selwakeup();
#define	SELWAKEUP(p, coll) \
	SKcall(selwakeup, sizeof(struct proc *) + sizeof(long), p, coll)

int sleep();
#define	SLEEP(chan, pri) \
	SKcall(sleep, sizeof(caddr_t) + sizeof(int), SUPERADD(chan), pri)

int soacc1(struct socket *so);
#define	SOACC1(so) \
	KScall(soacc1, sizeof(struct socket *), so)

int ksoaccept(struct socket *so, struct mbuf KSPACE *knam);
#define	SOACCEPT(so, nam) \
	KScall(ksoaccept, sizeof(struct socket *) + sizeof(struct mbuf *), \
	    so, nam)

int ksobind(struct socket *so, struct mbuf KSPACE *knam);
#define	SOBIND(so, nam) \
	KScall(ksobind, sizeof(struct socket *) + sizeof(struct mbuf *), \
	    so, nam)

int soclose(struct socket *so);
#define	SOCLOSE(so) \
	KScall(soclose, sizeof(struct socket *), so)

int ksoconnect(struct socket *so, struct mbuf KSPACE *knam);
#define	SOCON1(so, nam) \
	KScall(ksoconnect, sizeof(struct socket *) + \
	    sizeof(struct mbuf *), so, nam)

int soconnect2(struct socket *so1, struct socket *so2);
#define	SOCON2(so1, so2) \
	KScall(soconnect2, sizeof(struct socket *) + \
	    sizeof(struct socket *), so1, so2)

int socreate(int dom, struct socket * KSPACE *aso, int type,
	    int proto);
#define	SOCREATE(dom, aso, type, proto) \
	KScall(socreate, sizeof(int) + sizeof(struct socket **) + \
	    sizeof(int) + sizeof(int), dom, aso, type, proto)

int ksogetnam(struct socket *so, struct mbuf KSPACE *knam);
#define	SOGETNAM(so, m) \
	KScall(ksogetnam, sizeof(struct socket *) + sizeof(struct mbuf *), \
	    so, m)

int sogetopt(struct socket *so, int level, int optname,
	    struct mbuf * KSPACE *mp);
#define	SOGETOPT(so, level, optname, mp) \
	KScall(sogetopt, sizeof(struct socket *) + sizeof(int) + \
	    sizeof(int) + sizeof(struct mbuf **), so, level, optname, mp)

int ksogetpeer(struct socket *so, struct mbuf KSPACE *knam);
#define	SOGETPEER(so, m) \
	KScall(ksogetpeer, sizeof(struct socket *) + sizeof(struct mbuf *), \
	    so, m)

int solisten(struct socket *so, int backlog);
#define	SOLISTEN(so, backlog) \
	KScall(solisten, sizeof(struct socket *) + sizeof(int), so, backlog)

/* note; pass pointer to a socket, not pointer to file structure */
int ksoo_ioctl(struct socket *so, int cmd, char KSPACE *data);
#define	SOO_IOCTL(fp, cmd, data) \
	KScall(ksoo_ioctl, sizeof(struct socket *) + sizeof(int) + \
	    sizeof(caddr_t), fp->f_socket, cmd, data)

/* note; pass pointer to a socket, not pointer to file structure */
int soo_select(struct socket *so, int which);
#define	SOO_SELECT(fp, which) \
	KScall(soo_select, sizeof(struct socket *) + sizeof(int), \
	    fp->f_socket, which)

int ksoo_stat(struct socket *so, struct stat KSPACE *ub);
#define	SOO_STAT(so, ub) \
	KScall(ksoo_stat, sizeof(struct socket *) + sizeof(struct stat *), \
	    so, ub)

int soreceive(struct socket *so, struct mbuf * KSPACE *aname,
	    struct uio KSPACE *uio, int flags, struct mbuf * KSPACE *rightsp);
#define	SORECEIVE(so, aname, uiop, flags, rightsp) \
	KScall(soreceive, sizeof(struct socket *) + sizeof(struct mbuf **) + \
	    sizeof(struct uio *) + sizeof(int) + sizeof(struct mbuf **), \
	    so, aname, uiop, flags, rightsp)

int ksosend(struct socket *so, struct mbuf KSPACE *knam,
	    struct uio KSPACE *uio, int flags, struct mbuf KSPACE *krights);
#define	SOSEND(so, nam, uiop, flags, rights) \
	KScall(ksosend, sizeof(struct socket *) + sizeof(struct mbuf *) + \
	    sizeof(struct uio *) + sizeof(int) + sizeof(struct mbuf *), \
	    so, nam, uiop, flags, rights)

int ksosetopt(struct socket *so, int level, int optname,
	    struct mbuf KSPACE *km);
#define	SOSETOPT(so, level, optname, m0) \
	KScall(ksosetopt, sizeof(struct socket *) + sizeof(int) + \
	    sizeof(int) + sizeof(struct mbuf *), so, level, optname, m0)

int soshutdown(struct socket *so, int how);
#define	SOSHUTDOWN(so, how) \
	KScall(soshutdown, sizeof(struct socket *) + sizeof(int), so, how)

/*
 * A network function the clock is to call: SUPERFUN marks it, and
 * softclock (kern_clock.c) calls it with KScall.
 */
int timeout();
#define	TIMEOUT(fun, arg, t) \
	SKcall(timeout, sizeof(int (*)()) + sizeof(caddr_t) + sizeof(int), \
	    SUPERFUN(fun), (caddr_t)arg, t)

/*
 * A network interface's interrupt handler, marked the same way: intr
 * (machdep.c) calls it with KScall and its argument.
 */
int setirq(int irq, int level, int (*handler)(), int arg);
#define	SETIRQ(irq, level, fun, arg) \
	SKcall(setirq, sizeof(int) + sizeof(int) + sizeof(int (*)()) + \
	    sizeof(int), irq, level, SUPERFUN(fun), arg)

int unpbind(char __far *path, int len, struct inode **ipp,
	    struct socket *unpsock);
#define	UNPBIND(path, len, ipp, unpsock) \
	SKcall(unpbind, sizeof(char __far *) + sizeof(int) + \
	    sizeof(struct inode **) + sizeof(struct socket *), \
	    (char __far *)(path), len, (struct inode **)(ipp), unpsock)

int unpconn(char __far *path, int len, struct socket **so2,
	    struct inode **ipp);
#define	UNPCONN(path, len, so2, ipp) \
	SKcall(unpconn, sizeof(char __far *) + sizeof(int) + \
	    sizeof(struct socket **) + sizeof(struct inode **), \
	    (char __far *)(path), len, (struct socket **)(so2), \
	    (struct inode **)(ipp))

void unpdet(struct inode *ip);
#define	UNPDET(ip) \
	SKcall(unpdet, sizeof(struct inode *), ip)

int wakeup();
#define	WAKEUP(chan) \
	SKcall(wakeup, sizeof(caddr_t), SUPERADD(chan))
