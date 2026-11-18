/*
 * NE1000, NE2000 and compatible Ethernet cards (docs/network.md).
 *
 * The card is a National Semiconductor DP8390, or a copy of one, with
 * buffer memory that only the controller reaches: frames go in and out
 * of it through the card's data port, by what the controller calls
 * remote DMA.  The memory holds a buffer for the frame being sent and a
 * ring the controller puts received frames in.
 *
 * Cards called NE2000-compatible differ, among themselves and from the
 * NE2000, and the probe finds out what each does rather than going by a
 * name:
 *
 * - The station address PROM, read a byte at a time, gives each byte
 *   twice on a card with a 16-bit data path and once on an 8-bit one.
 * - The data port is used 16 bits at a time only on a card whose PROM
 *   reads doubled and that passes a check: a block written 16 bits at a
 *   time and read back a byte at a time, and the other way about.  A
 *   16-bit card in an 8-bit slot, or an 8-bit card that answers like a
 *   16-bit one, would pass a check that used 16 bits both ways, and lay
 *   its memory out wrong.  Otherwise a byte at a time, with the 286's
 *   rep insb and rep outsb if they pass a check of their own, and a
 *   loop if not.
 * - The buffer memory is found by writing to it: 32K at 0x4000 (some
 *   emulations of the NE2000), 16K at 0x4000 (the NE2000), 8K at 0x2000
 *   (the NE1000), or from 0x100 (Cabletron's).  Each 8K of a size is
 *   written before any is read back, so that memory that repeats is not
 *   taken for more of it.
 * - The interrupt request line is found by making the card request,
 *   unless neirq gives it.
 *
 * The controller is run as National's later data sheets say, which the
 * copies follow: recovery from an overflow of the receive ring as they
 * prescribe it, keeping what the ring holds; a frame header that the
 * controller wrote wrong, as some copies do, answered by dropping
 * everything in the ring; and the 8259's edge-triggered request kept
 * from being lost when the card still wants service.  A watchdog catches
 * what an interrupt was lost for, and a transmission that never ends.
 *
 * Frames are copied between the card and mbufs directly.  A received ARP
 * frame goes to arpinput at once, as on the PDP-11, and an IP one to the
 * protocol's queue.  One frame is sent at a time.
 */

#include "ne.h"
#if NNE > 0

#include "param.h"
#include "mbuf.h"
#include "socket.h"
#include "ioctl.h"
#include "errno.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#include "../netinet/if_ether.h"

#include "if_nereg.h"

/*
 * Where to look for cards, in order, and each unit's interrupt request
 * line, or 0 to find it.  A card set to line 2 requests on line 9.
 */
int	neport[] = { 0x300, 0x280, 0x320, 0x340, 0x360, 0 };
char	neirq[NNE];
#define	NE_IRQ		3		/* if it cannot be found */

#define	NE_TXPAGES	6		/* the transmit buffer: 1536 bytes */
#define	NE_SPIN		10000		/* reads of a register: 10 ms or more */
#define	NE_TXTIME	3		/* seconds a frame may take to go */
#define	NE_IMASK	(ISR_PRX|ISR_PTX|ISR_RXE|ISR_TXE|ISR_OVW|ISR_CNT)
#define	DCR_SET		(DCR_FT1|DCR_LS)	/* and DCR_WTS for 16 bits */

struct	ne_softc {
	struct	arpcom ns_ac;		/* Ethernet common part */
#define	ns_if	ns_ac.ac_if		/* network-visible interface */
#define	ns_addr	ns_ac.ac_enaddr		/* station address */
	int	ns_port;		/* the card's first I/O port */
	char	ns_irq;
	u_char	ns_flags;
	u_char	ns_tx;			/* first page of the transmit buffer */
	u_char	ns_pstart;		/* the receive ring's first page, */
	u_char	ns_pstop;		/*   and the page after its last */
	u_char	ns_next;		/* page of the next frame to take */
	char	ns_oactive;		/* a frame is going out */
	char	ns_txtime;		/* seconds it has taken */
	char	ns_lost;		/* watchdogs in a row that found work */
} ne_softc[NNE];

#define	NS_WORD		0x01		/* 16 bits at a time */
#define	NS_FAST		0x02		/* rep insb and rep outsb */

/*
 * The memory layouts to try, in order: first and last-plus-one page.
 */
static struct nemem {
	u_char	nm_start, nm_stop;
} nemem[] = {
	{ 0x40, 0xc0 },
	{ 0x40, 0x80 },
	{ 0x20, 0x40 },
	{ 0x01, 0x40 },
	{ 0x01, 0x20 },
	{ 0, 0 }
};

/*
 * What precedes a frame's data in the ring.
 */
struct nehead {
	struct	ne_ring h_ring;
	struct	ether_header h_eh;
};

int	neintr(), neinit(), neoutput(), neioctl(), newatch();
extern struct ifnet loif;

static int	neprobe(struct ne_softc *sc, int port);
static int	ne_service(struct ne_softc *sc);
static void	ne_setup(struct ne_softc *sc);
static void	nestart(struct ne_softc *sc);
static void	nestop(struct ne_softc *sc);

/*
 * Some time: n reads of a register, a microsecond each at the least on
 * the AT's bus, whatever the processor.
 */
static void
ne_wait(int port, int n)
{
	while (--n >= 0)
		(void) inb(port + NE_CR);
}

/*
 * Start a remote DMA of count bytes at addr in the card's memory.  The
 * controller is started for it: some copies do no remote DMA while it
 * is stopped.  Until ne_setup, its receiver is off and its transmitter
 * looped back, so starting it does nothing else.
 */
static void
ne_rdma(int port, u_int addr, u_int count, int cmd)
{
	outb(port + NE_CR, CR_NODMA | CR_STA);
	outb(port + NE_ISR, ISR_RDC);
	outb(port + NE_RBCR0, count);
	outb(port + NE_RBCR1, count >> 8);
	outb(port + NE_RSAR0, addr);
	outb(port + NE_RSAR1, addr >> 8);
	outb(port + NE_CR, cmd | CR_STA);
}

/*
 * Wait for a remote write to be done, which it is once the card has
 * taken every byte.  (A remote read is not waited for: some copies never
 * say that one is done.)
 */
static int
ne_rdcwait(int port)
{
	register int i;

	for (i = 0; i < NE_SPIN; i++)
		if (inb(port + NE_ISR) & ISR_RDC) {
			outb(port + NE_ISR, ISR_RDC);
			return (1);
		}
	return (0);
}

/*
 * Move n bytes between buf and the data port, as the card is set up to.
 * Sixteen bits at a time, an odd last byte is half a word.
 */
static void
ne_in(struct ne_softc *sc, char ANYSPACE *buf, int n)
{
	register int port = sc->ns_port + NE_DATA;

	if (sc->ns_flags & NS_WORD) {
		insw(port, buf, n >> 1);
		if (n & 1)
			buf[n - 1] = inw(port);
	} else if (sc->ns_flags & NS_FAST)
		insb(port, buf, n);
	else
		while (--n >= 0)
			*buf++ = inb(port);
}

static void
ne_out(struct ne_softc *sc, char ANYSPACE *buf, int n)
{
	register int port = sc->ns_port + NE_DATA;

	if (sc->ns_flags & NS_WORD) {
		outsw(port, buf, n >> 1);
		if (n & 1)
			outw(port, buf[n - 1] & 0xff);
	} else if (sc->ns_flags & NS_FAST)
		outsb(port, buf, n);
	else
		while (--n >= 0)
			outb(port, *buf++);
}

/*
 * Read n bytes at addr in the card's memory into buf; write them.  A
 * transfer 16 bits at a time is of whole words.
 */
static void
ne_read(struct ne_softc *sc, u_int addr, char ANYSPACE *buf, int n)
{
	ne_rdma(sc->ns_port, addr, sc->ns_flags & NS_WORD ? (n + 1) & ~1 : n,
	    CR_RREAD);
	ne_in(sc, buf, n);
	outb(sc->ns_port + NE_ISR, ISR_RDC);
}

static int
ne_write(struct ne_softc *sc, u_int addr, char ANYSPACE *buf, int n)
{
	ne_rdma(sc->ns_port, addr, sc->ns_flags & NS_WORD ? (n + 1) & ~1 : n,
	    CR_RWRITE);
	ne_out(sc, buf, n);
	return (ne_rdcwait(sc->ns_port));
}

/*
 * The page the controller will put the next frame in.
 */
static int
ne_curr(int port)
{
	register int c;

	outb(port + NE_CR, CR_NODMA | CR_PAGE1 | CR_STA);
	c = inb(port + NE_CURR);
	outb(port + NE_CR, CR_NODMA | CR_STA);
	return (c);
}

/*
 * The controller stopped, its receiver off, its transmitter looped back
 * and its interrupts masked, with dcr for its data configuration: the
 * start of the data sheet's initialization.
 */
static void
ne_quiet(int port, int dcr)
{
	outb(port + NE_CR, CR_NODMA | CR_STP);
	outb(port + NE_DCR, dcr);
	outb(port + NE_RBCR0, 0);
	outb(port + NE_RBCR1, 0);
	outb(port + NE_RCR, RCR_MON);
	outb(port + NE_TCR, TCR_LOOP);
	outb(port + NE_IMR, 0);
	outb(port + NE_ISR, 0xff);
}

/*
 * Is there a DP8390 at port?  Its tally counters go to zero when they
 * are read; if they do not, the two ports changed are put back.
 */
static int
ne_is8390(int port)
{
	int cr, mar5;

	cr = inb(port + NE_CR);
	if (cr == 0xff)
		return (0);
	outb(port + NE_CR, CR_NODMA | CR_PAGE1 | CR_STP);
	mar5 = inb(port + NE_MAR0 + 5);
	outb(port + NE_MAR0 + 5, 0xff);
	outb(port + NE_CR, CR_NODMA | CR_STP);
	(void) inb(port + NE_CNTR0);
	if (inb(port + NE_CNTR0) != 0) {
		outb(port + NE_CR, cr);
		outb(port + NE_MAR0 + 5, mar5);
		return (0);
	}
	return (1);
}

/*
 * Reset the card: a read of its reset port, written back at once, as
 * some copies need.  It says it has reset in the interrupt status.
 */
static int
ne_reset(int port)
{
	register int i;

	outb(port + NE_RESET, inb(port + NE_RESET));
	for (i = 0; i < NE_SPIN; i++)
		if (inb(port + NE_ISR) & ISR_RST)
			break;
	outb(port + NE_ISR, 0xff);
	return (i < NE_SPIN);
}

/*
 * Is there memory from page start to page stop?  A pattern is written at
 * the start of each 8K in it before any is read back.  pat and got are
 * 16 bytes.
 */
static int
ne_memok(struct ne_softc *sc, int start, int stop, char ANYSPACE *pat,
    char ANYSPACE *got)
{
	register int p, i;

	for (p = start; p < stop; p = (p + 0x20) & ~0x1f) {
		for (i = 0; i < 16; i++)
			pat[i] = p + i * 7;
		if (!ne_write(sc, p << 8, pat, 16))
			return (0);
	}
	for (p = start; p < stop; p = (p + 0x20) & ~0x1f) {
		ne_read(sc, p << 8, got, 16);
		for (i = 0; i < 16; i++)
			if (got[i] != (char)(p + i * 7))
				return (0);
	}
	return (1);
}

/*
 * Do transfers work as flags says?  64 bytes are written so and read
 * back a byte at a time with a loop, which the PROM and the memory have
 * been read with, and then written that way and read back so.
 */
static int
ne_xfercheck(struct ne_softc *sc, int flags, char ANYSPACE *pat,
    char ANYSPACE *got)
{
	register int i, pass;
	int ok = 1;

	for (pass = 0; pass < 2; pass++) {
		for (i = 0; i < 64; i++)
			pat[i] = i * 13 + 7 + pass * 101;
		sc->ns_flags = pass ? 0 : flags;
		outb(sc->ns_port + NE_DCR,
		    DCR_SET | (sc->ns_flags & NS_WORD ? DCR_WTS : 0));
		if (!ne_write(sc, sc->ns_tx << 8, pat, 64))
			ok = 0;
		sc->ns_flags = pass ? flags : 0;
		outb(sc->ns_port + NE_DCR,
		    DCR_SET | (sc->ns_flags & NS_WORD ? DCR_WTS : 0));
		ne_read(sc, sc->ns_tx << 8, got, 64);
		for (i = 0; i < 64; i++)
			if (got[i] != pat[i])
				ok = 0;
	}
	sc->ns_flags = 0;
	outb(sc->ns_port + NE_DCR, DCR_SET);
	return (ok);
}

/*
 * The 8259s' request registers.
 */
static u_int
ne_irr()
{
	register u_int irr;

	outb(0x20, 0x0a);
	irr = inb(0x20);
	outb(0xa0, 0x0a);
	return (irr | inb(0xa0) << 8);
}

/*
 * Which line does the card request on?  It is made to request, by the
 * end of a remote write of two bytes to its transmit buffer, and the
 * line found in the 8259s' requests that is set then and was not before
 * or after: the card's is masked, having no handler yet.  (Others make
 * a card request with a remote read of nothing, done as soon as it
 * starts; but an emulated card may only pulse its line for that, which
 * leaves no request behind.)  Twice, to be sure; -1 if no one line is
 * found.  The controller is in byte mode, as neprobe leaves it.
 */
static int
ne_findirq(struct ne_softc *sc)
{
	register int port = sc->ns_port;
	u_int before, during, after, got = 0;
	register int n, irq;

	for (n = 0; n < 2; n++) {
		before = ne_irr();
		outb(port + NE_IMR, ISR_RDC);
		ne_rdma(port, sc->ns_tx << 8, 2, CR_RWRITE);
		outb(port + NE_DATA, 0);
		outb(port + NE_DATA, 0);
		ne_wait(port, 20);
		during = ne_irr();
		outb(port + NE_IMR, 0);
		outb(port + NE_ISR, 0xff);
		ne_wait(port, 20);
		after = ne_irr();
		during &= ~before & ~after & ~(1 << 2);	/* 2: the cascade */
		if (during == 0 || (during & (during - 1)) ||
		    (n > 0 && during != got))
			return (-1);
		got = during;
	}
	for (irq = 0; !(got & 1); irq++)
		got >>= 1;
	return (irq);
}

/*
 * Look for a card at port, and find what it is.  Leaves it quiet.
 */
static int
neprobe(struct ne_softc *sc, int port)
{
	char buf[2][64];
	register int i, doubled;
	register struct nemem *nm;

	if (!ne_is8390(port) || !ne_reset(port))
		return (0);
	sc->ns_port = port;
	sc->ns_flags = 0;
	ne_quiet(port, DCR_SET);

	/*
	 * The station address PROM, at 0 in the card's memory.
	 */
	ne_read(sc, 0, buf[0], 32);
	doubled = 1;
	for (i = 0; i < 32; i += 2)
		if (buf[0][i] != buf[0][i + 1])
			doubled = 0;
	for (i = 0; i < 6; i++)
		sc->ns_addr[i] = buf[0][doubled ? i * 2 : i];
	if ((sc->ns_addr[0] & 1) || (sc->ns_addr[0] | sc->ns_addr[1] |
	    sc->ns_addr[2] | sc->ns_addr[3] | sc->ns_addr[4] |
	    sc->ns_addr[5]) == 0) {
		printf("ne at 0x%x: station address %s\n", port,
		    ether_sprintf(sc->ns_addr));
		return (0);
	}

	for (nm = nemem; nm->nm_start; nm++)
		if (ne_memok(sc, nm->nm_start, nm->nm_stop, buf[0], buf[1]))
			break;
	if (nm->nm_start == 0) {
		printf("ne at 0x%x: no buffer memory found\n", port);
		return (0);
	}
	sc->ns_tx = nm->nm_start;
	sc->ns_pstart = nm->nm_start + NE_TXPAGES;
	sc->ns_pstop = nm->nm_stop;

	if (doubled && ne_xfercheck(sc, NS_WORD, buf[0], buf[1]))
		sc->ns_flags = NS_WORD;
	else if (ne_xfercheck(sc, NS_FAST, buf[0], buf[1]))
		sc->ns_flags = NS_FAST;
	else
		sc->ns_flags = 0;
	ne_quiet(port, DCR_SET);
	return (1);
}

/*
 * Find the cards, at boot (net_sys.c).
 */
neattach()
{
	register struct ne_softc *sc;
	register struct ifnet *ifp;
	register int *pp;
	int unit = 0, irq, found;

	for (pp = neport; *pp && unit < NNE; pp++) {
		sc = &ne_softc[unit];
		if (!neprobe(sc, *pp))
			continue;
		found = 0;
		if ((irq = neirq[unit]) == 0) {
			irq = ne_findirq(sc);
			found = irq >= 0;
			if (!found)
				irq = NE_IRQ;
		} else if (irq == 2)
			irq = 9;
		printf("ne%d at 0x%x irq %d%s: %s, %dK, %s\n", unit,
		    sc->ns_port, irq, neirq[unit] || found ? "" : " (not found)",
		    ether_sprintf(sc->ns_addr), (sc->ns_pstop - sc->ns_tx) / 4,
		    sc->ns_flags & NS_WORD ? "16-bit" :
		    sc->ns_flags & NS_FAST ? "8-bit" : "8-bit, by a loop");
		if (SETIRQ(irq, IPL_IMP, neintr, unit) < 0) {
			printf("ne%d: irq %d is taken\n", unit, irq);
			continue;
		}
		sc->ns_irq = irq;
		ifp = &sc->ns_if;
		ifp->if_unit = unit;
		ifp->if_name = "ne";
		ifp->if_mtu = ETHERMTU;
		ifp->if_flags = IFF_BROADCAST;
		ifp->if_init = neinit;
		ifp->if_output = neoutput;
		ifp->if_ioctl = neioctl;
		ifp->if_watchdog = newatch;
		if_attach(ifp);
		unit++;
	}
	if (unit == 0)
		printf("ne0 not found\n");
}

/*
 * Set the controller going, as the data sheet says: at splimp.
 */
static void
ne_setup(struct ne_softc *sc)
{
	register int port = sc->ns_port, i;

	ne_quiet(port, DCR_SET | (sc->ns_flags & NS_WORD ? DCR_WTS : 0));
	outb(port + NE_TPSR, sc->ns_tx);
	outb(port + NE_PSTART, sc->ns_pstart);
	outb(port + NE_PSTOP, sc->ns_pstop);
	outb(port + NE_BNRY, sc->ns_pstop - 1);
	outb(port + NE_CR, CR_NODMA | CR_PAGE1 | CR_STP);
	for (i = 0; i < 6; i++)
		outb(port + NE_PAR0 + i, sc->ns_addr[i]);
	for (i = 0; i < 8; i++)
		outb(port + NE_MAR0 + i, 0);
	outb(port + NE_CURR, sc->ns_pstart);
	outb(port + NE_CR, CR_NODMA | CR_STP);
	sc->ns_next = sc->ns_pstart;
	sc->ns_oactive = 0;
	outb(port + NE_ISR, 0xff);
	outb(port + NE_IMR, NE_IMASK);
	outb(port + NE_CR, CR_NODMA | CR_STA);
	outb(port + NE_TCR, 0);
	outb(port + NE_RCR,
	    RCR_AB | (sc->ns_if.if_flags & IFF_PROMISC ? RCR_PRO : 0));
}

/*
 * Bring the interface up, once it has an address.
 */
neinit(unit)
	int unit;
{
	register struct ne_softc *sc = &ne_softc[unit];
	register struct ifnet *ifp = &sc->ns_if;
	int s;

	if (ifp->if_addrlist == 0)
		return;
	s = splimp();
	ne_setup(sc);
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_timer = 1;
	nestart(sc);
	splx(s);
}

static void
nestop(struct ne_softc *sc)
{
	register int port = sc->ns_port;

	outb(port + NE_IMR, 0);
	outb(port + NE_CR, CR_NODMA | CR_STP);
	outb(port + NE_ISR, 0xff);
	sc->ns_if.if_flags &= ~IFF_RUNNING;
	sc->ns_if.if_timer = 0;
	sc->ns_oactive = 0;
}

/*
 * Copy the frame in the chain m to the transmit buffer, and free it.
 * The length to send, or -1 if the card did not take it.  A frame
 * shorter than Ethernet's least is padded with zeros.  Sixteen bits at
 * a time, an odd byte at the end of an mbuf waits for the first of the
 * next.
 */
static int
ne_put(struct ne_softc *sc, struct mbuf *m)
{
	register struct mbuf *mp;
	register int n, len;
	int data = sc->ns_port + NE_DATA, word = sc->ns_flags & NS_WORD;
	int tlen, odd = -1;
	u_char *cp;

	for (len = 0, mp = m; mp; mp = mp->m_next)
		len += mp->m_len;
	tlen = MAX(len, ETHERMIN + sizeof(struct ether_header));
	ne_rdma(sc->ns_port, sc->ns_tx << 8, word ? (tlen + 1) & ~1 : tlen,
	    CR_RWRITE);
	for (mp = m; mp; mp = mp->m_next) {
		if ((n = mp->m_len) <= 0)
			continue;
		cp = mtod(mp, u_char *);
		if (word) {
			if (odd >= 0) {
				outw(data, odd | *cp++ << 8);
				odd = -1;
				n--;
			}
			if (n & 1)
				odd = cp[--n];
		}
		ne_out(sc, (char ANYSPACE *)cp, n);
	}
	if (word) {
		if (odd >= 0) {
			outw(data, odd);
			len++;
		}
		for (; len < tlen; len += 2)
			outw(data, 0);
	} else
		for (; len < tlen; len++)
			outb(data, 0);
	m_freem(m);
	return (ne_rdcwait(sc->ns_port) ? tlen : -1);
}

/*
 * Send the next frame waiting, if the card is free: at splimp.
 */
static void
nestart(struct ne_softc *sc)
{
	register int port = sc->ns_port;
	struct mbuf *m;
	int len;

	if (sc->ns_oactive || !(sc->ns_if.if_flags & IFF_RUNNING))
		return;
	IF_DEQUEUE(&sc->ns_if.if_snd, m);
	if (m == 0)
		return;
	if ((len = ne_put(sc, m)) < 0) {
		printf("ne%d: card took no frame\n", sc->ns_if.if_unit);
		sc->ns_if.if_oerrors++;
		ne_setup(sc);
		return;
	}
	outb(port + NE_TPSR, sc->ns_tx);
	outb(port + NE_TBCR0, len);
	outb(port + NE_TBCR1, len >> 8);
	outb(port + NE_CR, CR_NODMA | CR_TXP | CR_STA);
	sc->ns_oactive = 1;
	sc->ns_txtime = 0;
}

/*
 * Copy n bytes at addr in the receive ring to buf, going round its end.
 * The address after them.
 */
static u_int
ne_ringread(struct ne_softc *sc, u_int addr, char *buf, int n)
{
	u_int end = sc->ns_pstop << 8;
	register int c;

	while (n > 0) {
		c = addr + n > end ? end - addr : n;
		ne_read(sc, addr, (char ANYSPACE *)buf, c);
		addr += c;
		buf += c;
		n -= c;
		if (addr >= end)
			addr = sc->ns_pstart << 8;
	}
	return (addr);
}

/*
 * Copy totlen bytes of a frame at addr in the ring into mbufs, with the
 * interface's address before them, which the protocols' input queues
 * want.  Each mbuf but the last has an even number of bytes, so that a
 * transfer of 16 bits at a time ends at its end.
 */
static struct mbuf *
ne_get(struct ne_softc *sc, u_int addr, register int totlen)
{
	register struct mbuf *m;
	struct mbuf *top = 0, *tail = 0;
	register int len;
	int off;

	while (totlen > 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0) {
			m_freem(top);
			return (0);
		}
		off = top ? 0 : sizeof(struct ifnet *);
		len = MLEN;
		if (totlen + off > MLEN && totlen >= 512) {
			MCLGET(m);
			len = m->m_len;
		}
		len = MIN(totlen, len - off);
		m->m_len = off + len;
		if (off)
			*mtod(m, struct ifnet **) = &sc->ns_if;
		addr = ne_ringread(sc, addr, mtod(m, char *) + off, len);
		if (tail)
			tail->m_next = m;
		else
			top = m;
		tail = m;
		totlen -= len;
	}
	return (top);
}

/*
 * A frame has come, of len bytes with its Ethernet header, and maybe its
 * CRC.  Its data follows the header at the head of the ring.
 */
static void
ne_input(struct ne_softc *sc, u_short type, int len)
{
	register struct mbuf *m;
	register struct ifqueue *inq;

	len -= sizeof(struct ether_header);
	if (len > ETHERMTU)
		len = ETHERMTU;			/* the CRC */
	sc->ns_if.if_ipackets++;
	m = ne_get(sc, (sc->ns_next << 8) + sizeof(struct nehead), len);
	if (m == 0)
		return;
	switch (ntohs(type)) {
	case ETHERTYPE_IP:
		schednetisr(NETISR_IP);
		inq = &ipintrq;
		break;
	case ETHERTYPE_ARP:
		arpinput(&sc->ns_ac, m);
		return;
	default:
		m_freem(m);
		return;
	}
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		m_freem(m);
		return;
	}
	IF_ENQUEUE(inq, m);
}

/*
 * Take the frames in the ring.  A header that cannot be right means the
 * controller wrote it wrong, and all the ring holds is dropped: the
 * controller is to take its frames from where it is putting the next.
 */
static void
ne_rint(struct ne_softc *sc)
{
	register int port = sc->ns_port;
	struct nehead h;
	int curr, next, end, n;
	u_int count;

	for (n = sc->ns_pstop - sc->ns_pstart; n > 0; n--) {
		curr = ne_curr(port);
		if (sc->ns_next == curr)
			return;
		ne_read(sc, sc->ns_next << 8, (char ANYSPACE *)&h, sizeof h);
		count = h.h_ring.nr_count;
		next = h.h_ring.nr_next;
		end = sc->ns_next + ((count + 255) >> 8);
		if (end >= sc->ns_pstop)
			end -= sc->ns_pstop - sc->ns_pstart;
		if (next < sc->ns_pstart || next >= sc->ns_pstop ||
		    count < sizeof h || count > sizeof(struct ne_ring) +
		    sizeof(struct ether_header) + ETHERMTU + 4 ||
		    (next != end && next != (end + 1 == sc->ns_pstop ?
		    sc->ns_pstart : end + 1))) {
			sc->ns_if.if_ierrors++;
			next = curr;
		} else if (h.h_ring.nr_rsr & RSR_PRX)
			ne_input(sc, h.h_eh.ether_type,
			    count - sizeof(struct ne_ring));
		else
			sc->ns_if.if_ierrors++;
		sc->ns_next = next;
		outb(port + NE_BNRY,
		    (next == sc->ns_pstart ? sc->ns_pstop : next) - 1);
	}
}

/*
 * The receive ring has filled.  The data sheet's recovery: stop, wait
 * for whatever was going out to stop too, take the frames in the ring
 * with the transmitter looped back, and send again what the stop cut
 * off.  Nothing new is sent meanwhile (an ARP reply to a frame in the
 * ring would be), since it would only be looped back.
 */
static void
ne_overrun(struct ne_softc *sc)
{
	register int port = sc->ns_port;
	int txing, resend, oactive = sc->ns_oactive;

	txing = inb(port + NE_CR) & CR_TXP;
	outb(port + NE_CR, CR_NODMA | CR_STP);
	ne_wait(port, 2000);			/* 1.6 ms, at the least */
	outb(port + NE_RBCR0, 0);
	outb(port + NE_RBCR1, 0);
	resend = txing && !(inb(port + NE_ISR) & (ISR_PTX | ISR_TXE));
	outb(port + NE_TCR, TCR_LOOP);
	outb(port + NE_CR, CR_NODMA | CR_STA);
	sc->ns_oactive = 1;
	ne_rint(sc);
	sc->ns_oactive = oactive;
	outb(port + NE_ISR, ISR_OVW);
	outb(port + NE_TCR, 0);
	if (resend)
		outb(port + NE_CR, CR_NODMA | CR_TXP | CR_STA);
	sc->ns_if.if_ierrors++;
	nestart(sc);
}

/*
 * A frame has gone, or could not go.
 */
static void
ne_tint(struct ne_softc *sc, int isr)
{
	register int port = sc->ns_port;
	int tsr = inb(port + NE_TSR);

	outb(port + NE_ISR, isr & (ISR_PTX | ISR_TXE));
	if (isr & ISR_TXE) {
		sc->ns_if.if_oerrors++;
		if (tsr & TSR_ABT)
			sc->ns_if.if_collisions += 16;
	} else {
		sc->ns_if.if_opackets++;
		if (tsr & TSR_COL)
			sc->ns_if.if_collisions += inb(port + NE_NCR) & 0x0f;
	}
	sc->ns_oactive = 0;
	nestart(sc);
}

/*
 * Do what the card wants: at splimp.  How many times it was found to
 * want something.  The card's request to the 8259 is edge-triggered:
 * if it still wants service at the end, its request is dropped and made
 * again, or it would not be heard.
 */
static int
ne_service(struct ne_softc *sc)
{
	register int port = sc->ns_port;
	register int isr, n;

	if (!(sc->ns_if.if_flags & IFF_RUNNING))
		return (0);
	outb(port + NE_CR, CR_NODMA | CR_STA);
	for (n = 0; n < 10; n++) {
		if ((isr = inb(port + NE_ISR) & ~ISR_RST) == 0)
			break;
		if (isr & ISR_OVW)
			ne_overrun(sc);
		else if (isr & (ISR_PRX | ISR_RXE)) {
			outb(port + NE_ISR, isr & (ISR_PRX | ISR_RXE));
			ne_rint(sc);
		}
		if (isr & (ISR_PTX | ISR_TXE))
			ne_tint(sc, isr);
		if (isr & ISR_CNT) {
			sc->ns_if.if_ierrors += inb(port + NE_CNTR0);
			sc->ns_if.if_ierrors += inb(port + NE_CNTR1);
			sc->ns_if.if_ierrors += inb(port + NE_CNTR2);
			outb(port + NE_ISR, ISR_CNT);
		}
		if (isr & ISR_RDC)
			outb(port + NE_ISR, ISR_RDC);
	}
	if (inb(port + NE_ISR) & NE_IMASK) {
		outb(port + NE_IMR, 0);
		outb(port + NE_IMR, NE_IMASK);
	}
	return (n);
}

/*
 * The card's interrupt, at splimp (setirq).
 */
neintr(unit)
	int unit;
{
	(void) ne_service(&ne_softc[unit]);
}

/*
 * Once a second while the interface runs (if_slowtimo): take what an
 * interrupt was lost for, and start again a card whose frame has not
 * gone in NE_TXTIME seconds.  Work found here time after time means the
 * card's interrupts are not arriving.
 */
newatch(unit)
	int unit;
{
	register struct ne_softc *sc = &ne_softc[unit];
	int s = splimp();

	if (sc->ns_if.if_flags & IFF_RUNNING) {
		if (ne_service(sc) == 0)
			sc->ns_lost = 0;
		else if (sc->ns_lost < 5 && ++sc->ns_lost == 5)
			printf("ne%d: interrupts lost: is irq %d right?\n",
			    unit, sc->ns_irq);
		if (sc->ns_oactive && ++sc->ns_txtime > NE_TXTIME) {
			printf("ne%d: frame not sent\n", unit);
			sc->ns_if.if_oerrors++;
			ne_setup(sc);
			nestart(sc);
		}
		sc->ns_if.if_timer = 1;
	}
	splx(s);
}

/*
 * The Ethernet output routine: put the header on the packet m for dst,
 * and queue it.  dst is in the network's data, not on the stack.
 */
neoutput(ifp, m0, dst)
	register struct ifnet *ifp;
	struct mbuf *m0;
	register struct sockaddr *dst;
{
	register struct ne_softc *sc = &ne_softc[ifp->if_unit];
	register struct mbuf *m = m0;
	register struct ether_header *eh;
	struct mbuf *mcopy = 0;
	u_char edst[6];
	int type, s, error, usetrailers;

	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING)) {
		error = ENETDOWN;
		goto bad;
	}
	switch (dst->sa_family) {
	case AF_INET:
		if (!arpresolve(&sc->ns_ac, m,
		    &((struct sockaddr_in *)dst)->sin_addr, edst, &usetrailers))
			return (0);		/* not resolved yet */
		if (!bcmp(edst, etherbroadcastaddr, sizeof edst))
			mcopy = m_copy(m, 0, (int)M_COPYALL);
		type = ETHERTYPE_IP;
		break;
	case AF_UNSPEC:
		eh = (struct ether_header *)dst->sa_data;
		bcopy(eh->ether_dhost, edst, sizeof edst);
		type = eh->ether_type;
		break;
	default:
		printf("ne%d: can't handle af%d\n", ifp->if_unit,
		    dst->sa_family);
		error = EAFNOSUPPORT;
		goto bad;
	}

	/*
	 * Put the Ethernet header in front, in a new mbuf if the first has
	 * no room.
	 */
	if (m->m_off > MMAXOFF ||
	    MMINOFF + sizeof(struct ether_header) > m->m_off) {
		m = m_get(M_DONTWAIT, MT_HEADER);
		if (m == 0) {
			error = ENOBUFS;
			goto bad;
		}
		m->m_next = m0;
		m->m_off = MMINOFF;
		m->m_len = sizeof(struct ether_header);
	} else {
		m->m_off -= sizeof(struct ether_header);
		m->m_len += sizeof(struct ether_header);
	}
	eh = mtod(m, struct ether_header *);
	bcopy(edst, eh->ether_dhost, sizeof edst);
	bcopy(sc->ns_addr, eh->ether_shost, sizeof eh->ether_shost);
	eh->ether_type = htons((u_short)type);

	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		splx(s);
		m_freem(m);
		if (mcopy)
			m_freem(mcopy);
		return (ENOBUFS);
	}
	IF_ENQUEUE(&ifp->if_snd, m);
	nestart(sc);
	splx(s);
	return (mcopy ? looutput(&loif, mcopy, dst) : 0);
bad:
	m_freem(m0);
	if (mcopy)
		m_freem(mcopy);
	return (error);
}

neioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ifaddr *ifa = (struct ifaddr *)data;
	int s = splimp(), error = 0;

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		neinit(ifp->if_unit);
		if (ifa->ifa_addr.sa_family == AF_INET) {
			((struct arpcom *)ifp)->ac_ipaddr =
			    IA_SIN(ifa)->sin_addr;
			arpwhohas((struct arpcom *)ifp, &IA_SIN(ifa)->sin_addr);
		}
		break;
	case SIOCSIFFLAGS:
		if (!(ifp->if_flags & IFF_UP) && (ifp->if_flags & IFF_RUNNING))
			nestop(&ne_softc[ifp->if_unit]);
		else if ((ifp->if_flags & IFF_UP) &&
		    !(ifp->if_flags & IFF_RUNNING))
			neinit(ifp->if_unit);
		break;
	default:
		error = EINVAL;
	}
	splx(s);
	return (error);
}
#endif
