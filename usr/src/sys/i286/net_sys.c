/*
 * The network's side of its joins with the kernel: starting it, its
 * software interrupt, the routines it keeps its own copies of, and the
 * entry points the kernel calls that move data between the kernel's
 * stack and mbufs.  Compiled with the rest of the network, with its own
 * data segment (docs/network.md, net_mac.h).  This is what the PDP-11's
 * sys_net.c is to its network, without the UNIBUS.
 */

#include "param.h"
#include "user.h"
#include "uio.h"
#include "mbuf.h"
#include "socket.h"
#include "socketvar.h"
#include "protosw.h"
#include "domain.h"
#include "ioctl.h"
#include "stat.h"
#include "file.h"

#include "../net/if.h"
#include "../net/netisr.h"

#include "loop.h"
#include "ne.h"

int	soaccept(), sogetnam(), sogetpeer(), putchar();

int	hz;			/* the kernel's, given by NETSETHZ */

void
netsethz(ticks)
	int ticks;
{
	hz = ticks;
}

/*
 * Called once by the kernel as it starts (init_main.c).
 */
netstart()
{
	register int s;

	mbinit();
#if NLOOP > 0
	printf("attaching lo0\n");
	loattach();
#endif
#if NNE > 0
	neattach();
#endif
	s = splimp();
	ifinit();
	domaininit();			/* must follow interfaces */
	splx(s);
}

/*
 * The network's software interrupt, which setsoftnet asks for and
 * spllower (machdep.c) runs at splnet: the input queues of the
 * protocols the interfaces have put packets on.  From the PDP-11's
 * net_trap.s.
 */
netintr()
{
	register int bits, s;

	for (;;) {
		s = splhigh();
		bits = netisr;
		netisr = 0;
		splx(s);
		if (bits == 0)
			break;
#ifdef INET
		if (bits & (1 << NETISR_IP))
			ipintr();
#endif
		if (bits & (1 << NETISR_RAW))	/* last: the others schedule it */
			rawintr();
	}
}

/*
 * Panic is called on fatal errors.  It prints "net panic: mesg" and
 * then calls the kernel entry netcrash() to bring down the net.
 */
panic(s)
	register char *s;
{
	printf("net panic: %s\n", s);
	NETCRASH();
	/* NOTREACHED */
}

/*
 * uiomove, for the uio of a system call, which is on the kernel's stack;
 * cp is in the network's data.  The addresses in the process are far,
 * and copyin and copyout take far pointers on both sides.
 */
uiomove(cp, n, uio)
	caddr_t cp;
	u_int n;
	register struct uio KSPACE *uio;
{
	register struct iovec KSPACE *iov;
	register u_int cnt;
	int error;

	if (uio->uio_segflg != UIO_USERSPACE)
		panic("net uiomove");
	while (n && uio->uio_resid) {
		iov = (struct iovec KSPACE *)(int)uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;
		if (uio->uio_rw == UIO_READ)
			error = copyout(cp, iov->iov_base, cnt);
		else
			error = copyin(iov->iov_base, cp, cnt);
		if (error)
			return (error);
		iov->iov_base += cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		n -= cnt;
	}
	return (0);
}

/*
 * The kernel's entry points that take an mbuf it made on its stack, as
 * the PDP-11's kernel does for a name (uipc_syscalls.c): the network
 * cannot follow a pointer to that through its protocols, so each copies
 * it to or from an mbuf of its own around the real call.
 */
static struct mbuf *
kmcopyin(km, type)
	register struct mbuf KSPACE *km;
	int type;
{
	register struct mbuf *m;

	if (km == 0)
		return (0);
	m = m_get(M_WAIT, type);
	if (m == 0)
		return (0);
	m->m_len = km->m_len;
	bcopy((char KSPACE *)km + km->m_off, mtod(m, caddr_t),
	    (u_int)km->m_len);
	return (m);
}

static void
kmcopyout(m, km)
	register struct mbuf *m;
	register struct mbuf KSPACE *km;
{
	km->m_len = m->m_len;
	bcopy(mtod(m, caddr_t), (char KSPACE *)km + km->m_off,
	    (u_int)m->m_len);
}

ksobind(so, knam)
	struct socket *so;
	struct mbuf KSPACE *knam;
{
	register struct mbuf *nam;
	int error;

	if ((nam = kmcopyin(knam, MT_SONAME)) == 0)
		return (ENOBUFS);
	error = sobind(so, nam);
	m_freem(nam);
	return (error);
}

ksoconnect(so, knam)
	struct socket *so;
	struct mbuf KSPACE *knam;
{
	register struct mbuf *nam;
	int error;

	if ((nam = kmcopyin(knam, MT_SONAME)) == 0)
		return (ENOBUFS);
	error = soconnect(so, nam);
	m_freem(nam);
	return (error);
}

/*
 * Accept, and the socket's own name and its peer's: the protocol fills
 * in the mbuf, which goes back to the kernel's.
 */
static
knameout(so, knam, f)
	struct socket *so;
	struct mbuf KSPACE *knam;
	int (*f)();
{
	register struct mbuf *nam;
	int error;

	if ((nam = m_getclr(M_WAIT, MT_SONAME)) == 0)
		return (ENOBUFS);
	error = (*f)(so, nam);
	kmcopyout(nam, knam);
	m_freem(nam);
	return (error);
}

ksoaccept(so, knam)
	struct socket *so;
	struct mbuf KSPACE *knam;
{
	return (knameout(so, knam, soaccept));
}

ksogetnam(so, knam)
	struct socket *so;
	struct mbuf KSPACE *knam;
{
	return (knameout(so, knam, sogetnam));
}

ksogetpeer(so, knam)
	struct socket *so;
	struct mbuf KSPACE *knam;
{
	return (knameout(so, knam, sogetpeer));
}

ksosetopt(so, level, optname, km)
	struct socket *so;
	int level, optname;
	struct mbuf KSPACE *km;
{
	register struct mbuf *m = 0;
	int error;

	if (km && (m = kmcopyin(km, MT_SOOPTS)) == 0)
		return (ENOBUFS);
	error = sosetopt(so, level, optname, m);
	if (m)
		m_freem(m);
	return (error);
}

ksosend(so, knam, uio, flags, krights)
	struct socket *so;
	struct mbuf KSPACE *knam, KSPACE *krights;
	struct uio KSPACE *uio;
	int flags;
{
	register struct mbuf *nam = 0, *rights = 0;
	int error;

	if (knam && (nam = kmcopyin(knam, MT_SONAME)) == 0)
		return (ENOBUFS);
	if (krights && (rights = kmcopyin(krights, MT_RIGHTS)) == 0) {
		if (nam)
			m_freem(nam);
		return (ENOBUFS);
	}
	error = sosend(so, nam, uio, flags, rights);
	if (nam)
		m_freem(nam);
	if (rights)
		m_freem(rights);
	return (error);
}

/*
 * The kernel's buffer for an ioctl is IOCPARM_MASK+1 bytes on its stack
 * (sys_generic.c), but the command the kernel passes has lost the size,
 * which is in its high word.  The socket's own commands, which fset and
 * fioctl also send with a buffer of an int, are done here; for an
 * interface's, a route's or a protocol's the whole buffer goes to a
 * cluster and back.
 */
ksoo_ioctl(so, cmd, data)
	register struct socket *so;
	int cmd;
	register char KSPACE *data;	/* not caddr_t KSPACE: that would
					   be a based variable, not pointer */
{
	register struct mbuf *m;
	int error;

	switch (cmd) {

	case FIONBIO:
		if (*(int KSPACE *)data)
			so->so_state |= SS_NBIO;
		else
			so->so_state &= ~SS_NBIO;
		return (0);

	case FIOASYNC:
		if (*(int KSPACE *)data)
			so->so_state |= SS_ASYNC;
		else
			so->so_state &= ~SS_ASYNC;
		return (0);

	case FIONREAD:
		*(long KSPACE *)data = so->so_rcv.sb_cc;
		return (0);

	case SIOCSPGRP:
		so->so_pgrp = *(int KSPACE *)data;
		return (0);

	case SIOCGPGRP:
		*(int KSPACE *)data = so->so_pgrp;
		return (0);

	case SIOCATMARK:
		*(int KSPACE *)data = (so->so_state&SS_RCVATMARK) != 0;
		return (0);
	}
	m = m_get(M_WAIT, MT_DATA);
	if (m == 0)
		return (u.u_error = ENOBUFS);
	MCLGET(m);
	if (m->m_len != MCLBYTES) {
		m_freem(m);
		return (u.u_error = ENOBUFS);
	}
	bcopy(data, mtod(m, caddr_t), IOCPARM_MASK + 1);
	error = soo_ioctl(so, cmd, mtod(m, caddr_t));
	bcopy(mtod(m, caddr_t), data, IOCPARM_MASK + 1);
	m_freem(m);
	return (error);
}

/*
 * The protocol may fill in more of the structure than soo_stat clears.
 */
ksoo_stat(so, ub)
	struct socket *so;
	struct stat KSPACE *ub;
{
	register struct mbuf *m;
	int error;

	if ((m = m_getclr(M_WAIT, MT_DATA)) == 0)
		return (ENOBUFS);
	error = soo_stat(so, mtod(m, struct stat *));
	bcopy(mtod(m, caddr_t), ub, sizeof (struct stat));
	m_freem(m);
	return (error);
}

/*
 * Copy what an mbuf from soreceive or sogetopt holds out to the process,
 * no more than *len, and say in *len how much that was.
 */
netcopyout(m, to, len)
	struct mbuf *m;
	uaddr_t to;
	int KSPACE *len;
{
	if (*len > m->m_len)
		*len = m->m_len;
	return (copyout(mtod(m, caddr_t), to, *len));
}

/*
 * sysctl_int (kern_sysctl.c), for a variable of the network's: the
 * lengths are the kernel's, on its stack, and the buffers the process's.
 */
sysctl_int(oldp, oldlenp, newp, newlen, valp)
	void __far *oldp;
	size_t KSPACE *oldlenp;
	void __far *newp;
	size_t newlen;
	int *valp;
{
	int error = 0;

	if (newp && newlen != sizeof(int))
		return (EINVAL);
	if (oldp)
		error = copyout(valp, oldp, MIN(sizeof(int), *oldlenp));
	if (error == 0 && newp)
		error = copyin(newp, valp, sizeof(int));
	*oldlenp = sizeof(int);
	return (error);
}

/*
 * printf, for the network's messages, whose strings are in its own data:
 * a character at a time to the kernel's putchar.  From the PDP-11's
 * sys_net.c, which has it from subr_prf.c.
 */
#define	TOCONS	0x1
#define	TOLOG	0x4

static void
pchar(c)
	int c;
{
	SKcall(putchar, sizeof(int) + sizeof(int) + sizeof(struct tty *),
	    c, TOCONS | TOLOG, (struct tty *)0);
}

static void
printn(n, b)
	long n;
	u_int b;
{
	char buf[12];
	register char KSPACE *cp = buf;
	register int offset = 0;

	if (n < 0)
		switch (b) {
		case 8:
		case 16:
			offset = b - 1;
			n++;
			break;
		case 10:
			pchar('-');
			n = -n;
			break;
		}
	do {
		*cp++ = "0123456789ABCDEF"[offset + n % b];
	} while (n = n / b);
	do
		pchar(*--cp);
	while (cp > buf);
}

/*
 * The arguments are on the stack, after fmt: not with va_list, whose
 * pointer would be one into the network's data.
 */
printf(char *fmt, ...)
{
	register int c;
	register u_int KSPACE *adx = (u_int KSPACE *)&fmt + 1;
	u_int b;
	char *s;

	for (;;) {
		while ((c = *fmt++) != '%') {
			if (c == '\0')
				return;
			pchar(c);
		}
		c = *fmt++;
		switch (c) {
		case 'l':
			c = *fmt++;
			switch (c) {
			case 'x':
				b = 16;
				goto lnumber;
			case 'd':
				b = 10;
				goto lnumber;
			case 'o':
				b = 8;
				goto lnumber;
			default:
				pchar('%');
				pchar('l');
				pchar(c);
			}
			break;
		case 'X':
			b = 16;
			goto lnumber;
		case 'D':
			b = 10;
			goto lnumber;
		case 'O':
			b = 8;
lnumber:		printn(*(long KSPACE *)adx, b);
			adx += (sizeof(long) / sizeof(int)) - 1;
			break;
		case 'x':
			b = 16;
			goto number;
		case 'd':
		case 'u':
			b = 10;
			goto number;
		case 'o':
			b = 8;
number:			printn((long)*adx, b);
			break;
		case 'c':
			pchar(*adx);
			break;
		case 's':
			s = (char *)*adx;
			while (c = *s++)
				pchar(c);
			break;
		case '%':
			pchar(c);
			break;
		default:
			pchar('%');
			pchar(c);
			break;
		}
		adx++;
	}
}
