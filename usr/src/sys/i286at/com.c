/*
 * Serial ports: the PC's COM1 and COM2, 8250s or their successors, as
 * terminals /dev/tty00 and /dev/tty01.  A port's lines are not watched:
 * it is taken to have a terminal on it whenever it is open.  Parity is
 * made in software, as the tty layer asks; the port itself always sends
 * eight bits without it.
 *
 * The kernel also prints its messages on COM1 (comcnputc), without
 * interrupts, so that they get out whatever state it is in; cons.c
 * prints them on the screen as well.
 */
#include "param.h"
#include "conf.h"
#include "user.h"
#include "proc.h"
#include "ioctl.h"
#include "tty.h"
#include "systm.h"

#define	NCOM	2

#define	DATA	0		/* divisor, low byte, when DLAB is set */
#define	IER	1		/* divisor, high byte, when DLAB is set */
#define	IIR	2
#define	LCR	3
#define	MCR	4
#define	LSR	5

#define	IER_RX		0x01	/* interrupt when a character arrives */
#define	IER_TX		0x02	/* and when one has gone */
#define	IIR_NONE	0x01	/* nothing is asking */
#define	LCR_8N1		0x03
#define	LCR_DLAB	0x80
#define	MCR_DTR_RTS	0x03
#define	MCR_OUT2	0x08	/* lets the port's interrupt out */
#define	LSR_RXRDY	0x01
#define	LSR_THRE	0x20	/* ready for a character */

#define	CLOCK		115200L	/* the ports' clock, over 16 */

static	int comport[NCOM] = { 0x3F8, 0x2F8 };
static	char comirq[NCOM] = { 4, 3 };
static	char compresent[NCOM];
static	char comattached[NCOM];
static	char comcurspeed[NCOM];	/* what the port is set to, B0 if not */
struct	tty com_tty[NCOM];
int	comstart();
static	int comintr();
extern	char partab[];

/* The divisor for each speed, as the tty layer numbers them. */
static	u_short comdiv[16] = {
	0, 2304, 1536, 1047, 857, 768, 576, 384,
	192, 96, 64, 48, 24, 12, 6, 3
};

/*
 * Set a port to send and receive at the speed the tty layer numbers so.
 * The divisor is written only if the speed is not the port's already:
 * every TIOCSETN asks, and a shell's line editor sends one at each
 * prompt, and an 8250 whose divisor is written while a character is
 * going out may send it wrong.  While DLAB is set, a character written
 * to the port goes into the divisor and is never sent; so interrupts are
 * off meanwhile, or comintr, seeing the port still ready, would empty
 * the output queue into it.
 */
static
comspeed(unit, speed)
	int unit, speed;
{
	register int port = comport[unit];
	register u_int div = comdiv[speed & 017];
	int s;

	if (div == 0 || (speed & 017) == comcurspeed[unit])
		return;
	s = intsoff();
	outb(port + LCR, LCR_DLAB);
	outb(port + DATA, div);
	outb(port + IER, div >> 8);
	outb(port + LCR, LCR_8N1);
	intson(s);
	comcurspeed[unit] = speed & 017;
}

/*
 * Find the ports, and ready COM1 for the kernel's messages.  A port is
 * there if its line control register keeps what is written to it.
 */
comcninit()
{
	register int unit, port;

	for (unit = 0; unit < NCOM; unit++) {
		port = comport[unit];
		outb(port + LCR, 0x1B);
		if (inb(port + LCR) != 0x1B)
			continue;
		outb(port + LCR, LCR_8N1);
		if (inb(port + LCR) != LCR_8N1)
			continue;
		compresent[unit] = 1;
		outb(port + IER, 0);
		comspeed(unit, B9600);
		outb(port + MCR, MCR_DTR_RTS);
	}
}

/*ARGSUSED*/
comopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit = minor(dev), port;

	if (unit >= NCOM || !compresent[unit])
		return (ENXIO);
	tp = &com_tty[unit];
	port = comport[unit];
	tp->t_oproc = comstart;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_state = TS_ISOPEN | TS_CARR_ON;
		tp->t_flags = ANYP | ECHO | XTABS | CRMOD;
		tp->t_ispeed = tp->t_ospeed = B9600;
		comspeed(unit, B9600);
	} else if (tp->t_state & TS_XCLUDE && u.u_uid != 0)
		return (EBUSY);
	if (!comattached[unit]) {
		if (setirq(comirq[unit], IPL_TTY, comintr, unit) < 0)
			return (EBUSY);		/* a network card has it */
		comattached[unit] = 1;
	}
	outb(port + MCR, MCR_DTR_RTS | MCR_OUT2);
	outb(port + IER, IER_RX | IER_TX);
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*ARGSUSED*/
comclose(dev, flag)
	dev_t dev;
{
	register int unit = minor(dev);
	register struct tty *tp = &com_tty[unit];

	(*linesw[tp->t_line].l_close)(tp, flag);
	if (tp->t_state & TS_HUPCLS)
		outb(comport[unit] + MCR, MCR_OUT2);	/* DTR off: hang up */
	ttyclose(tp);
	return (0);
}

/*ARGSUSED*/
comread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = &com_tty[minor(dev)];

	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

/*ARGSUSED*/
comwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = &com_tty[minor(dev)];

	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

comioctl(dev, cmd, addr, flag)
	dev_t dev;
	register u_int cmd;
	caddr_t addr;
{
	register int unit = minor(dev);
	register struct tty *tp = &com_tty[unit];
	register int error;

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, addr, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, addr, flag);
	if (error < 0)
		return (ENOTTY);
	if (cmd == TIOCSETP || cmd == TIOCSETN)
		comspeed(unit, tp->t_ospeed);
	return (error);
}

/*
 * The port wants something: take every character that has arrived, and
 * send another if the last has gone.
 */
/*ARGSUSED*/
static
comintr(unit, tf)
	int unit;
	caddr_t tf;
{
	register struct tty *tp = &com_tty[unit];
	register int port = comport[unit], lsr;

	for (;;) {
		lsr = inb(port + LSR);
		if (lsr & LSR_RXRDY) {
			(*linesw[tp->t_line].l_rint)(inb(port + DATA), tp);
			continue;
		}
		if ((lsr & LSR_THRE) && (tp->t_state & TS_BUSY)) {
			tp->t_state &= ~TS_BUSY;
			(*linesw[tp->t_line].l_start)(tp);
			continue;
		}
		if (inb(port + IIR) & IIR_NONE)
			break;
	}
}

comstart(tp)
	register struct tty *tp;
{
	register int c, s;
	int port = comport[tp - com_tty];

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))
		goto out;
	ttyowake(tp);
	if (tp->t_outq.c_cc == 0)
		goto out;
	if ((inb(port + LSR) & LSR_THRE) == 0)
		goto out;
	c = getc(&tp->t_outq);
	if (tp->t_flags & (RAW | LITOUT))
		c &= 0xff;
	else if ((tp->t_flags & (EVENP | ODDP)) == EVENP)
		c |= partab[c] & 0200;
	else if ((tp->t_flags & (EVENP | ODDP)) == ODDP)
		c |= (partab[c] & 0200) ^ 0200;
	outb(port + DATA, c);
	tp->t_state |= TS_BUSY;
out:
	splx(s);
}

/*
 * Print a character on COM1, if there is one.  Waits for the port to be
 * ready, but not for ever.
 */
comcnputc(c)
	int c;
{
	register int timo;

	if (!compresent[0])
		return;
	if (c == '\n')
		comcnputc('\r');
	timo = 30000;
	while ((inb(comport[0] + LSR) & LSR_THRE) == 0)
		if (--timo == 0)
			break;
	outb(comport[0] + DATA, c);
}
