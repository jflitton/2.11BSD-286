/*
 * The network's variables that upstream's headers define, each in every
 * file that includes the header, for the PDP-11's compiler to make one of
 * as a common block.  Here the headers declare them extern, and they are
 * defined once, in this file (as the kernel's own are, by the files that
 * own them).
 */

#include "param.h"
#include "mbuf.h"
#include "socket.h"
#include "domain.h"
#include "protosw.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../net/raw_cb.h"
#include "../net/netisr.h"

#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/in_pcb.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/tcp.h"
#include "../netinet/tcp_seq.h"
#include "../netinet/tcp_timer.h"
#include "../netinet/tcp_var.h"
#include "../netinet/tcpip.h"
#include "../netinet/tcp_debug.h"

/* mbuf.h */
struct	mbstat mbstat;
int	nmbclusters;
struct	mbuf *mfree, *mclfree;
char	mclrefcnt[NMBCLUSTERS + 1];
int	m_want;

/* domain.h */
struct	domain *domains;

/* net/if.h, net/route.h, net/raw_cb.h, net/netisr.h */
struct	ifnet *ifnet;
struct	ifqueue rawintrq;
struct	mbuf *rthost[RTHASHSIZ];
struct	mbuf *rtnet[RTHASHSIZ];
struct	rtstat rtstat;
struct	rawcb rawcb;
int	netisr;

/* netinet/in_var.h, ip_var.h, tcp_var.h, tcp_seq.h, tcp_debug.h */
struct	ifqueue ipintrq;
struct	ipstat ipstat;
struct	ipq ipq;
u_short	ip_id;
struct	inpcb tcb;
struct	tcpstat tcpstat;
tcp_seq	tcp_iss;
struct	tcp_debug tcp_debug[TCP_NDEBUG];
int	tcp_debx;
