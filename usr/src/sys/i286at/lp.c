/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)lp.c	1.4 (2.11BSD) 2026/11/26
 */

/*
 * Parallel ports: the PC's printer ports, as /dev/lp0 to lp2.  This is
 * upstream's LP-11 driver (pdpuba/lp.c), with a PC's port in place of
 * the LP-11's two registers.  The ports are looked for where the BIOS
 * looks for them and in its order, 0x3BC, 0x378 and 0x278, each there
 * if its data register keeps what is written to it.  The interrupt is
 * 7, or 5 at 0x278, as a PC/AT's ports have it; the PS/2's own port
 * has 7 at any of the three.
 *
 * A character goes to the printer when the printer is not busy and the
 * last has been acknowledged: it is put on the data lines and strobed,
 * and the printer's acknowledge interrupts, which sends the next.  The
 * LP-11 said it was busy the moment it was given a character; a printer
 * on a PC says so only once it has seen the strobe.  The clock looks
 * every 10 seconds, or 2 while characters are going, as it did for the
 * LP-11: a lost acknowledge holds the printer up until then, and a
 * printer that was off line or out of paper prints again once it is put
 * right.  On a PS/2 the port holds its interrupt until its status is
 * read, which the interrupt's handler does each time.
 *
 * A PC's printer returns its carriage at a carriage return and moves the
 * paper at a line feed, so a line feed or a form feed is sent after a
 * carriage return when the carriage is not at the left.
 */
#include "lp.h"
#if NLP > 0
#include "param.h"
#include "systm.h"
#include "user.h"
#include "ioctl.h"
#include "tty.h"
#include "uio.h"
#include "kernel.h"

#define	LPPRI		(PZERO + 8)
#define	LPLWAT		40
#define	LPHWAT		400

#define	LPBUFSIZE	512
#define CAP		1
#ifndef LP_MAXCOL
#define LP_MAXCOL	132
#endif

#define LPUNIT(dev)	(minor(dev) >> 3)

/* A port's registers, from its address. */
#define	LP_DATA		0
#define	LP_STATUS	1
#define	LP_CTRL		2

#define	LPS_NERR	0x08	/* no error */
#define	LPS_SEL		0x10	/* on line */
#define	LPS_PE		0x20	/* out of paper */
#define	LPS_NBUSY	0x80	/* not busy */
#define	LPS_ERR(st)	(((st) & (LPS_NERR | LPS_SEL | LPS_PE)) != \
			    (LPS_NERR | LPS_SEL))

#define	LPC_STROBE	0x01
#define	LPC_NINIT	0x04	/* not resetting the printer */
#define	LPC_SELIN	0x08	/* selecting it */
#define	LPC_IE		0x10	/* interrupt at its acknowledge */
#define	LPC_IDLE	(LPC_NINIT | LPC_SELIN)

static	int lpport[] = { 0x3BC, 0x378, 0x278 };
static	char lpirq[] = { 7, 7, 5 };

struct lp_softc {
	struct	clist sc_outq;
	int	sc_state;
	int	sc_physcol;
	int	sc_logcol;
	int	sc_physline;
	char	sc_flags;
	int	sc_lpchar;
} lp_softc[NLP];

int	lp_addr[NLP];		/* the ports' addresses, 0 if none */
static	char lp_irq[NLP];
static	u_int lpirqset;		/* the interrupts set, a bit each */

int lptout();
static	int lpintr();

/* bits for state */
#define	OPEN		1	/* device is open */
#define	TOUT		2	/* timeout is active */
#define	MOD		4	/* device state has been modified */
#define	ASLP		8	/* awaiting draining of printer */
#define	SENT		16	/* a character is out, not acknowledged */

/*
 * Find the ports, as the system starts (machdep.c).
 */
lpattach()
{
	register int i, unit, port;

	for (i = unit = 0; i < 3 && unit < NLP; i++) {
		port = lpport[i];
		outb(port + LP_DATA, 0xAA);
		if (inb(port + LP_DATA) != 0xAA)
			continue;
		outb(port + LP_DATA, 0x55);
		if (inb(port + LP_DATA) != 0x55)
			continue;
		outb(port + LP_DATA, 0);
		outb(port + LP_CTRL, LPC_IDLE);
		lp_addr[unit] = port;
		lp_irq[unit] = lpirq[i];
		lp_softc[unit].sc_lpchar = -1;
		printf("lp%d at 0x%x irq %d\n", unit, port, lpirq[i]);
		unit++;
	}
}

/*ARGSUSED*/
lpopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct lp_softc *sc;
	register int unit, s;
	int port, irq;

	if ((unit = LPUNIT(dev)) >= NLP ||
	   (sc = &lp_softc[unit])->sc_state&OPEN ||
	   (port = lp_addr[unit]) == 0)
		return (ENXIO);
	irq = lp_irq[unit];
	if ((lpirqset & (1 << irq)) == 0) {
		if (setirq(irq, IPL_TTY, lpintr, irq) < 0)
			return (EBUSY);		/* a network card has it */
		lpirqset |= 1 << irq;
	}
	if (LPS_ERR(inb(port + LP_STATUS)))
		return (EIO);
	sc->sc_state |= OPEN;
	sc->sc_flags = minor(dev) & 07;
	s = spltty();
	outb(port + LP_CTRL, LPC_IDLE | LPC_IE);
	if ((sc->sc_state&TOUT) == 0) {
		sc->sc_state |= TOUT;
		timeout(lptout, (caddr_t)dev, 10*hz);
	}
	splx(s);
	lpcanon(dev, '\f');
	return (0);
}

/*ARGSUSED*/
lpclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct lp_softc *sc = &lp_softc[LPUNIT(dev)];

	lpcanon(dev, '\f');
	sc->sc_state &= ~OPEN;
}

lpwrite(dev, uio, flag)
	register dev_t dev;
	register struct uio *uio;
	int flag;
{
	register int n;
	register char *cp;
	char inbuf[LPBUFSIZE];
	int error;

	while (n = MIN(LPBUFSIZE, uio->uio_resid)) {
		cp = inbuf;
		error = uiomove(cp, (int)n, uio);
		if (error)
			return (error);
		do
			lpcanon(dev, *cp++);
		while (--n);
	}
	return (0);
}

lpcanon(dev, c)
	dev_t dev;
	register int c;
{
	struct lp_softc *sc = &lp_softc[LPUNIT(dev)];
	register int logcol, physcol, s;

	if (sc->sc_flags&CAP) {
		register c2;

		if (c >= 'a' && c <= 'z')
			c += 'A'-'a'; else
		switch (c) {

		case '{':
			c2 = '(';
			goto esc;

		case '}':
			c2 = ')';
			goto esc;

		case '`':
			c2 = '\'';
			goto esc;

		case '|':
			c2 = '!';
			goto esc;

		case '~':
			c2 = '^';

		esc:
			lpcanon(dev, c2);
			sc->sc_logcol--;
			c = '-';
		}
	}
	logcol = sc->sc_logcol;
	physcol = sc->sc_physcol;
	if (c == ' ')
		logcol++;
	else switch(c) {

	case '\t':
		logcol = (logcol + 8) & ~7;
		break;

	case '\f':
		if (sc->sc_physline == 0 && physcol == 0)
			break;
		/* fall into ... */

	case '\n':
		if (physcol > 0)
			lpoutput(dev, '\r');
		lpoutput(dev, c);
		if (c == '\f')
			sc->sc_physline = 0;
		else
			sc->sc_physline++;
		physcol = 0;
		/* fall into ... */

	case '\r':
		s = spltty();
		logcol = 0;
		lpstart(LPUNIT(dev));
		splx(s);
		break;

	case '\b':
		if (logcol > 0)
			logcol--;
		break;

	default:
		if (logcol < physcol) {
			lpoutput(dev, '\r');
			physcol = 0;
		}
		if (logcol < LP_MAXCOL) {
			while (logcol > physcol) {
				lpoutput(dev, ' ');
				physcol++;
			}
			lpoutput(dev, c);
			physcol++;
		}
		logcol++;
	}
	if (logcol > 1000)	/* ignore long lines  */
		logcol = 1000;
	sc->sc_logcol = logcol;
	sc->sc_physcol = physcol;
}

lpoutput(dev, c)
	dev_t dev;
	int c;
{
	register struct	lp_softc *sc = &lp_softc[LPUNIT(dev)];
	int s;

	if (sc->sc_outq.c_cc >= LPHWAT) {
		s = spltty();
		lpstart(LPUNIT(dev));				/* unchoke */
		while (sc->sc_outq.c_cc >= LPHWAT) {
			sc->sc_state |= ASLP;		/* the printer is behind */
			sleep((caddr_t)sc, LPPRI);
		}
		splx(s);
	}
	while (putc(c, &sc->sc_outq))
		sleep((caddr_t)&lbolt, LPPRI);
}

/*
 * A port has interrupted: its printer has acknowledged a character.
 * Reading the status lets a PS/2's port take its interrupt back.
 */
/*ARGSUSED*/
static
lpintr(irq, tf)
	int irq;
	caddr_t tf;
{
	register int unit;

	for (unit = 0; unit < NLP; unit++)
		if (lp_addr[unit] && lp_irq[unit] == irq) {
			(void) inb(lp_addr[unit] + LP_STATUS);
			lp_softc[unit].sc_state &= ~SENT;
			lpstart(unit);
		}
}

/*
 * Send the next character, if the last has been acknowledged and the
 * printer is ready.  At spltty.
 */
lpstart(unit)
	int unit;
{
	register struct lp_softc *sc = &lp_softc[unit];
	register int port = lp_addr[unit];
	int n, st;

	n = sc->sc_outq.c_cc;
	if (sc->sc_lpchar < 0)
		sc->sc_lpchar = getc(&sc->sc_outq);
	if ((sc->sc_state&SENT) == 0 && sc->sc_lpchar >= 0) {
		st = inb(port + LP_STATUS);
		if ((st & LPS_NBUSY) && !LPS_ERR(st)) {
			outb(port + LP_DATA, sc->sc_lpchar);
			outb(port + LP_CTRL, LPC_IDLE | LPC_IE | LPC_STROBE);
			outb(port + LP_CTRL, LPC_IDLE | LPC_IE);
			sc->sc_lpchar = getc(&sc->sc_outq);
			sc->sc_state |= SENT | MOD;
		}
	}
	if (n>LPLWAT && sc->sc_outq.c_cc<=LPLWAT && sc->sc_state&ASLP) {
		sc->sc_state &= ~ASLP;
		wakeup((caddr_t)sc);		/* top half should go on */
	}
}

lptout(dev)
	dev_t dev;
{
	register struct lp_softc *sc;
	register int unit = LPUNIT(dev);
	int s;

	sc = &lp_softc[unit];
	if ((sc->sc_state&MOD) != 0) {
		sc->sc_state &= ~MOD;		/* something happened */
						/* so don't sweat */
		timeout(lptout, (caddr_t)dev, 2*hz);
		return;
	}
	s = spltty();
	if ((sc->sc_state&OPEN) == 0 && sc->sc_outq.c_cc == 0 &&
	    sc->sc_lpchar < 0) {
		sc->sc_state &= ~(TOUT | SENT);	/* no longer open */
		outb(lp_addr[unit] + LP_CTRL, LPC_IDLE);
		splx(s);
		return;
	}
	sc->sc_state &= ~SENT;		/* its acknowledge is not coming */
	lpstart(unit);			/* ready to go */
	splx(s);
	timeout(lptout, (caddr_t)dev, 10*hz);
}
#endif
