/*
 * The console: the first serial port, at 9600 bits a second.  It is a
 * terminal like any other, and cnputc is also how the kernel prints,
 * without interrupts, so that a message gets out whatever state the
 * kernel is in.
 */
#include "param.h"
#include "conf.h"
#include "user.h"
#include "proc.h"
#include "ioctl.h"
#include "tty.h"
#include "systm.h"

#define	COM1	0x3F8
#define	COM1IRQ	4
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

struct	tty cons[1];
int	cnstart(), ttrstrt();
static	int cnintr();
extern	char partab[];

cninit()
{
	outb(COM1 + LCR, LCR_DLAB);
	outb(COM1 + DATA, 115200L / 9600);
	outb(COM1 + IER, 0);
	outb(COM1 + LCR, LCR_8N1);
	outb(COM1 + IER, 0);
	outb(COM1 + MCR, MCR_DTR_RTS);
}

/*ARGSUSED*/
cnopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	static int attached;

	if (minor(dev))
		return (ENXIO);
	tp = &cons[0];
	tp->t_oproc = cnstart;
	if ((tp->t_state&TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_state = TS_ISOPEN|TS_CARR_ON;
		tp->t_flags = ANYP|ECHO|XTABS|CRMOD;
	}
	if (tp->t_state&TS_XCLUDE && u.u_uid != 0)
		return (EBUSY);
	if (!attached) {
		attached = 1;
		setirq(COM1IRQ, IPL_TTY, cnintr, 0);
		outb(COM1 + MCR, MCR_DTR_RTS | MCR_OUT2);
		outb(COM1 + IER, IER_RX | IER_TX);
	}
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*ARGSUSED*/
cnclose(dev, flag)
	dev_t dev;
{
	register struct tty *tp = &cons[0];

	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);
	return (0);
}

/*ARGSUSED*/
cnread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = &cons[0];

	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

/*ARGSUSED*/
cnwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = &cons[0];

	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

/*ARGSUSED*/
cnioctl(dev, cmd, addr, flag)
	dev_t dev;
	register u_int cmd;
	caddr_t addr;
{
	register struct tty *tp = &cons[0];
	register int error;

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, addr, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, addr, flag);
	if (error < 0)
		error = ENOTTY;
	return (error);
}

/*
 * The port wants something: take every character that has arrived, and
 * send another if the last has gone.
 */
/*ARGSUSED*/
static
cnintr(arg, tf)
	int arg;
	caddr_t tf;
{
	register struct tty *tp = &cons[0];
	register int lsr;

	for (;;) {
		lsr = inb(COM1 + LSR);
		if (lsr & LSR_RXRDY) {
			(*linesw[tp->t_line].l_rint)(inb(COM1 + DATA), tp);
			continue;
		}
		if ((lsr & LSR_THRE) && (tp->t_state & TS_BUSY)) {
			tp->t_state &= ~TS_BUSY;
			(*linesw[tp->t_line].l_start)(tp);
			continue;
		}
		if (inb(COM1 + IIR) & IIR_NONE)
			break;
	}
}

cnstart(tp)
	register struct tty *tp;
{
	register int c, s;

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	ttyowake(tp);
	if (tp->t_outq.c_cc == 0)
		goto out;
	if ((inb(COM1 + LSR) & LSR_THRE) == 0)
		goto out;
	c = getc(&tp->t_outq);
	if (tp->t_flags & (RAW|LITOUT))
		c &= 0xff;
	else if ((tp->t_flags & (EVENP | ODDP)) == EVENP)
		c |= partab[c] & 0200;
	else if ((tp->t_flags & (EVENP | ODDP)) == ODDP)
		c |= (partab[c] & 0200) ^ 0200;
	outb(COM1 + DATA, c);
	tp->t_state |= TS_BUSY;
out:
	splx(s);
}

/*
 * Print a character on the console.  Waits for the port to be ready, but
 * not for ever: with nothing connected the status never changes.
 */
cnputc(c)
	char c;
{
	register int timo;

	if (c == '\n')
		cnputc('\r');
	timo = 30000;
	while ((inb(COM1 + LSR) & LSR_THRE) == 0)
		if (--timo == 0)
			break;
	outb(COM1 + DATA, c);
}
