/*
 * The console: the display and the keyboard, as NCONS terminals.  Each
 * is a page of the display adapter's text memory with a VT102 drawing
 * on it (../i286at/vt.c), and Alt-F1 to Alt-F4 choose which is shown
 * and gets what is typed (../i286at/kbd.c).  Minor 0 is /dev/console;
 * 1 to 3 are /dev/ttyv1 to /dev/ttyv3, as the PDP-11's console driver
 * also has its extra DL-11s.  The adapter is a VGA, colour or
 * monochrome, left by the BIOS in its 80 by 25 text mode.
 *
 * The kernel prints on the first screen and on the first serial port
 * (../i286at/com.c), without interrupts, so that a message gets out
 * whatever state the kernel is in.  A machine with no screen is used
 * through a serial port; making /dev/console that port's node makes it
 * the console, which is what the tests do.
 *
 * A screen draws what is written to it as soon as it is written,
 * without interrupts: cnstart empties the output queue into the
 * emulator at the level it was called at, which is the writing
 * process's own, so nothing else waits on the drawing.
 */
#include "param.h"
#include "conf.h"
#include "user.h"
#include "proc.h"
#include "ioctl.h"
#include "tty.h"
#include "systm.h"
#include "kernel.h"
#include "../i286at/vt.h"
#include "com.h"

#define	NCONS		4
#define	PAGE		2048	/* cells from one screen to the next: the
				   memory has room for eight */

#define	MISC_READ	0x3CC	/* the VGA's miscellaneous output register */
#define	MISC_COLOR	0x01	/* CRT controller at 0x3D4, memory at B8000 */

/* The CRT controller's registers. */
#define	CRTC_CURSTART	0x0A	/* the cursor's first scan line ... */
#define	CURSOR_OFF	0x20	/*   and whether it is hidden */
#define	CRTC_START	0x0C	/* the first cell shown, high byte first */
#define	CRTC_CURSOR	0x0E	/* the cell the cursor is on, the same */

/* The bell: channel 2 of the timer drives the speaker. */
#define	TIMER2		0x42
#define	TIMER_MODE	0x43
#define	TIMER_RATE	1193182L
#define	TIMER_SQWAVE2	0xB6	/* channel 2, both bytes, mode 3 */
#define	SPEAKER		0x61	/* the speaker's gate and enable bits */
#define	BELL_HZ		800

struct	tty cons[NCONS];
static	struct vt vt[NCONS];
int	ncons;			/* NCONS, or 0 with no display; pstat
				   reads it */
static	int shown;		/* the screen on the display */
static	int crtc;		/* the CRT controller's index register */
static	int curstart;		/* CRTC_CURSTART as the BIOS left it */
static	int curoff;		/* the cursor is hidden now */
static	char ringing;
int	cnstart(), ttrstrt();
extern	char *panicstr;

static
crtcset(reg, val)
	int reg, val;
{
	outb(crtc, reg);
	outb(crtc + 1, val);
}

static
crtcget(reg)
	int reg;
{
	outb(crtc, reg);
	return (inb(crtc + 1));
}

/*
 * Put the hardware's cursor where screen unit's is, if it is the one
 * shown.  After a character in the last column it shows there.
 */
static
cncursor(unit)
	int unit;
{
	register struct vt *vp = &vt[unit];
	register u_int pos;
	int s, off;

	if (unit != shown)
		return;
	pos = unit * PAGE + vp->v_y * VT_COLS + vp->v_x;
	off = (vp->v_flags & VT_TCEM) == 0;
	s = intsoff();
	crtcset(CRTC_CURSOR, pos >> 8);
	crtcset(CRTC_CURSOR + 1, pos);
	if (off != curoff) {
		curoff = off;
		crtcset(CRTC_CURSTART, off ? curstart | CURSOR_OFF : curstart);
	}
	intson(s);
}

/*
 * Find the display and give each screen its page.  The first keeps what
 * the BIOS and the boot program wrote, and the cursor where they left
 * it; the others are cleared.
 */
cninit()
{
	register int i;
	register u_short __far *mem;
	u_int pos;
	int mono, s;

#if NCOM > 0
	comcninit();
#endif
	mono = (inb(MISC_READ) & MISC_COLOR) == 0;
	crtc = mono ? 0x3B4 : 0x3D4;
	mem = (u_short __far *)UADDR(gdtalloc(mono ? 0xB0000L : 0xB8000L,
	    0x7FFF), 0);
	/* memory that keeps what is written is a display's */
	i = mem[NCONS * PAGE - 1];
	mem[NCONS * PAGE - 1] = 0xA55A;
	if (mem[NCONS * PAGE - 1] != 0xA55A)
		return;
	mem[NCONS * PAGE - 1] = i;

	s = intsoff();
	curstart = crtcget(CRTC_CURSTART) & ~CURSOR_OFF;
	pos = crtcget(CRTC_CURSOR) << 8;
	pos |= crtcget(CRTC_CURSOR + 1);
	crtcset(CRTC_START, 0);
	crtcset(CRTC_START + 1, 0);
	intson(s);
	for (i = 0; i < NCONS; i++) {
		vtinit(&vt[i], mem + i * PAGE, mono);
		if (i)
			vtreset(&vt[i]);
	}
	if (pos < VT_ROWS * VT_COLS) {
		vt[0].v_x = pos % VT_COLS;
		vt[0].v_y = pos / VT_COLS;
	}
	ncons = NCONS;
	curoff = -1;
	cncursor(0);
	kbdinit();
}

/*ARGSUSED*/
cnopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit = minor(dev);

	if (unit >= ncons)
		return (ENXIO);
	tp = &cons[unit];
	tp->t_oproc = cnstart;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_state = TS_ISOPEN | TS_CARR_ON;
		tp->t_flags = ANYP | ECHO | CRMOD;
		tp->t_ispeed = tp->t_ospeed = B9600;
		tp->t_winsize.ws_row = VT_ROWS;
		tp->t_winsize.ws_col = VT_COLS;
	} else if (tp->t_state & TS_XCLUDE && u.u_uid != 0)
		return (EBUSY);
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*ARGSUSED*/
cnclose(dev, flag)
	dev_t dev;
{
	register struct tty *tp = &cons[minor(dev)];

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
	register struct tty *tp = &cons[minor(dev)];

	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

/*ARGSUSED*/
cnwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = &cons[minor(dev)];

	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

/*ARGSUSED*/
cnioctl(dev, cmd, addr, flag)
	dev_t dev;
	register u_int cmd;
	caddr_t addr;
{
	register struct tty *tp = &cons[minor(dev)];
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
 * Draw what is in the output queue.  TS_BUSY keeps out a second call
 * for the same screen, from an interrupt's echo, while this one is
 * drawing with the level lowered; what that call would have drawn is
 * in the queue, and this one draws it.
 */
cnstart(tp)
	register struct tty *tp;
{
	register int s, n;
	int unit = tp - cons;
	char buf[64];

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))
		goto out;
	tp->t_state |= TS_BUSY;
	while (tp->t_outq.c_cc && (tp->t_state & TS_TTSTOP) == 0) {
		n = q_to_b(&tp->t_outq, buf, sizeof (buf));
		splx(s);
		vtwrite(&vt[unit], (u_char *)buf, n);
		s = spltty();
	}
	tp->t_state &= ~TS_BUSY;
	cncursor(unit);
	ttyowake(tp);
out:
	splx(s);
}

/*
 * Print a character of the kernel's on the first screen and on COM1.  A
 * panic brings the first screen to the display.
 */
cnputc(c)
	char c;
{
	if (ncons) {
		if (panicstr && shown != 0)
			cnswitch(0);
		vtkputc(&vt[0], c & 0377);
		cncursor(0);
	}
#if NCOM > 0
	comcnputc(c);
#endif
}

/*
 * Show screen unit, and send it what is typed.
 */
cnswitch(unit)
	register int unit;
{
	register u_int start = unit * PAGE;
	int s;

	if (unit >= ncons || unit == shown)
		return;
	shown = unit;
	s = intsoff();
	crtcset(CRTC_START, start >> 8);
	crtcset(CRTC_START + 1, start);
	intson(s);
	cncursor(unit);
}

/*
 * What was typed, from the keyboard's interrupt.
 */
cninput(cp, n)
	register char *cp;
	register int n;
{
	register struct tty *tp = &cons[shown];

	if (ncons == 0 || (tp->t_state & TS_ISOPEN) == 0)
		return;
	while (--n >= 0)
		(*linesw[tp->t_line].l_rint)(*cp++ & 0377, tp);
}

/* The modes of the screen shown, which change what keys send. */
cnkbdflags()
{
	return (vt[shown].v_flags);
}

/*
 * Scroll Lock: stop the screen's output if it is going, and start it
 * if it is stopped, as the stop and start characters would.
 */
cnhold()
{
	register struct tty *tp = &cons[shown];
	char c;

	c = tp->t_state & TS_TTSTOP ? tp->t_startc : tp->t_stopc;
	if ((c & 0377) != 0377)		/* 0377: there is none */
		cninput(&c, 1);
}

/*
 * What a screen answers, as if typed there.
 */
vtreply(vp, s)
	struct vt *vp;
	register char *s;
{
	register struct tty *tp = &cons[vp - vt];
	int x;

	x = spltty();
	if (tp->t_state & TS_ISOPEN)
		while (*s)
			(*linesw[tp->t_line].l_rint)(*s++ & 0377, tp);
	splx(x);
}

static
bellstop()
{
	outb(SPEAKER, inb(SPEAKER) & ~3);
	ringing = 0;
}

/*ARGSUSED*/
vtbell(vp)
	struct vt *vp;
{
	register u_int count = TIMER_RATE / BELL_HZ;
	int s;

	if (ringing)
		return;
	ringing = 1;
	s = intsoff();
	outb(TIMER_MODE, TIMER_SQWAVE2);
	outb(TIMER2, count);
	outb(TIMER2, count >> 8);
	outb(SPEAKER, inb(SPEAKER) | 3);
	intson(s);
	timeout(bellstop, (caddr_t)0, hz / 8);
}
