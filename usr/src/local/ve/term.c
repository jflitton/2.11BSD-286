/*
 * ve: the terminal.  What is written to it goes by termcap, so that any
 * terminal with cursor addressing will do; what is read from it is
 * turned into keys, the ones with arrows on them included, which most
 * terminals send as the same few ANSI sequences whatever termcap says,
 * and those are known here without it.
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sgtty.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ve.h"

int	rows, cols;
int	resized;		/* the window's size changed: not yet seen to */
int	intr;			/* the interrupt key was struck */

extern char	*tgetstr(), *tgoto();
extern short	ospeed;
extern char	PC;

static char	tbuf[1024], area[512];
static char	*CM, *CE, *CL, *SO, *SE, *AL, *DL, *CS, *SF, *SR, *TI, *TE;
static char	obuf[1024];
static int	olen;
static int	crow = -1, ccol;	/* where the terminal's cursor is */
static int	cattr;
static int	raw;
static struct sgttyb osg;
static struct tchars otc;
static struct ltchars oltc;
static int	olocal;

void
t_flush(void)
{
	if (olen)
		(void)write(1, obuf, olen);
	olen = 0;
}

static int
outc(int c)
{
	if (olen == sizeof (obuf))
		t_flush();
	obuf[olen++] = c;
	return (c);
}

static void
cap(char *s, int lines)
{
	if (s)
		tputs(s, lines, outc);
}

static void
winch(int sig)
{
	resized = 1;
}

/*
 * The size of the window: what the system has for the terminal, or
 * else what termcap says of it.
 */
void
t_size(void)
{
	struct winsize ws;

	rows = cols = 0;
	if (ioctl(1, TIOCGWINSZ, (char *)&ws) == 0) {
		rows = ws.ws_row;
		cols = ws.ws_col;
	}
	if (rows <= 0)
		rows = tgetnum("li");
	if (cols <= 0)
		cols = tgetnum("co");
	if (rows <= 0)
		rows = 24;
	if (cols <= 0)
		cols = 80;
	if (rows > ROWMAX)
		rows = ROWMAX;
	if (cols > COLMAX)
		cols = COLMAX;
	if (rows < 2)
		rows = 2;
	if (cols < 8)
		cols = 8;
	/* the screen is kept in two segments, a byte a place in each */
	while ((long)rows * cols > 65000L)
		rows--;
}

void
t_open(void)
{
	char *term, *p = area;

	if ((term = getenv("TERM")) == 0 || tgetent(tbuf, term) <= 0) {
		(void)write(2, "ve: this terminal is not known\n", 31);
		exit(1);
	}
	CM = tgetstr("cm", &p);
	CE = tgetstr("ce", &p);
	CL = tgetstr("cl", &p);
	SO = tgetstr("so", &p);
	SE = tgetstr("se", &p);
	AL = tgetstr("al", &p);
	DL = tgetstr("dl", &p);
	CS = tgetstr("cs", &p);
	SF = tgetstr("sf", &p);
	SR = tgetstr("sr", &p);
	TI = tgetstr("ti", &p);
	TE = tgetstr("te", &p);
	if (tgetstr("pc", &p))
		PC = p[-2];
	if (SF == 0)
		SF = "\n";
	if (tgetnum("sg") > 0)
		SO = SE = 0;		/* standout that takes a place */
	if (CM == 0 || CL == 0) {
		(void)write(2, "ve: this terminal cannot move its cursor\n", 41);
		exit(1);
	}
	t_size();
	(void)signal(SIGWINCH, winch);
	(void)siginterrupt(SIGWINCH, 1);
}

/*
 * The terminal as ve wants it: each key as it is struck, nothing
 * echoed, and no character taken for a signal or changed on the way.
 * With quiet the terminal itself is not told yet (t_ti).
 */
void
t_raw(int quiet)
{
	struct sgttyb sg;
	struct tchars tc;
	struct ltchars ltc;
	int local;

	if (raw)
		return;
	(void)ioctl(0, TIOCGETP, (char *)&osg);
	(void)ioctl(0, TIOCGETC, (char *)&otc);
	(void)ioctl(0, TIOCGLTC, (char *)&oltc);
	(void)ioctl(0, TIOCLGET, (char *)&olocal);
	sg = osg;
	sg.sg_flags |= CBREAK;
	sg.sg_flags &= ~(ECHO | CRMOD | XTABS);
	ospeed = sg.sg_ospeed;
	tc = otc;
	/*
	 * ^C stays the interrupt, so that it stops what takes long; it
	 * must not throw away what is on its way to the screen.
	 */
	tc.t_intrc = CTL('C');
	tc.t_quitc = tc.t_eofc = tc.t_brkc = -1;
	ltc.t_suspc = ltc.t_dsuspc = ltc.t_rprntc = ltc.t_flushc =
	    ltc.t_werasc = ltc.t_lnextc = -1;
	local = olocal | LLITOUT | LPASS8 | LNOFLSH;
	(void)ioctl(0, TIOCSETN, (char *)&sg);
	(void)ioctl(0, TIOCSETC, (char *)&tc);
	(void)ioctl(0, TIOCSLTC, (char *)&ltc);
	(void)ioctl(0, TIOCLSET, (char *)&local);
	raw = 1;
	crow = -1;
	cattr = 0;
	if (!quiet)
		t_ti();
}

/*
 * What the terminal is to be sent before the editor has its screen;
 * some put away what is on it, to bring it back afterwards.
 */
void
t_ti(void)
{
	cap(TI, 1);
	crow = -1;
}

/*
 * And as it was, with the cursor on the last line.
 */
void
t_cooked(void)
{
	if (!raw)
		return;
	t_attr(0);
	t_move(rows - 1, 0);
	t_clreol();
	cap(TE, 1);
	t_flush();
	(void)ioctl(0, TIOCSETN, (char *)&osg);
	(void)ioctl(0, TIOCSETC, (char *)&otc);
	(void)ioctl(0, TIOCSLTC, (char *)&oltc);
	(void)ioctl(0, TIOCLSET, (char *)&olocal);
	raw = 0;
}

void
t_close(void)
{
	t_cooked();
}

/*
 * Can the editor be stopped for a while, to go on later?  If the
 * terminal has a key for that, the shell knows how.
 */
int
t_cansusp(void)
{
	return ((oltc.t_suspc & 0377) != 0377);
}

void
t_move(int r, int c)
{
	if (r == crow && c == ccol)
		return;
	cap(tgoto(CM, c, r), 1);
	crow = r;
	ccol = c;
}

/*
 * Standout, on or off.
 */
void
t_attr(int a)
{
	if (a == cattr)
		return;
	cap(a ? SO : SE, 1);
	cattr = a;
}

/*
 * A character, where the cursor is.  After the last column the cursor
 * may be there still or on the next line, as the terminal has it: it
 * is put somewhere before more is written.
 */
void
t_putc(int c)
{
	outc(c);
	if (++ccol >= cols)
		crow = -1;
}

void
t_puts(char *s)
{
	while (*s)
		t_putc(*s++);
}

/*
 * Clear from the cursor to the end of its line.
 */
void
t_clreol(void)
{
	register int n;

	if (CE) {
		t_attr(0);
		cap(CE, 1);
		return;
	}
	n = cols - ccol - (crow == rows - 1);
	t_attr(0);
	while (n-- > 0)
		outc(' ');
	crow = -1;
}

void
t_clear(void)
{
	cattr = -1;
	t_attr(0);
	cap(CL, rows);
	crow = -1;
}

void
t_bell(void)
{
	outc(007);
}

/*
 * Move what is on rows top to bot up n rows, or down if n is less than
 * 0, leaving the rows that come free blank: by a scrolling region if
 * the terminal has one, or by putting lines in and taking them out.
 * 0 if it has neither, and nothing is done.
 */
int
t_scroll(int top, int bot, int n)
{
	register int i;

	if (n == 0)
		return (1);
	t_attr(0);
	if (CS && (n > 0 || SR)) {
		cap(tgoto(CS, bot, top), 1);
		crow = -1;
		if (n > 0) {
			t_move(bot, 0);
			for (i = 0; i < n; i++)
				cap(SF, 1);
		} else {
			t_move(top, 0);
			for (i = 0; i < -n; i++)
				cap(SR, 1);
		}
		cap(tgoto(CS, rows - 1, 0), 1);
		crow = -1;
		return (1);
	}
	if (AL == 0 || DL == 0)
		return (0);
	if (n > 0) {
		t_move(top, 0);
		for (i = 0; i < n; i++)
			cap(DL, rows - top);
		t_move(bot - n + 1, 0);
		for (i = 0; i < n; i++)
			cap(AL, rows - bot + n);
	} else {
		n = -n;
		t_move(bot - n + 1, 0);
		for (i = 0; i < n; i++)
			cap(DL, rows - bot + n);
		t_move(top, 0);
		for (i = 0; i < n; i++)
			cap(AL, rows - top);
	}
	crow = -1;
	return (1);
}

/*
 * Keys.  What has been stuffed is read before the terminal is: the
 * keys of the change that . repeats.  And while a command is being
 * typed its keys are kept, so that it can be repeated.
 */
short	redo[REDOMAX];
int	nredo;
static short	rec[REDOMAX];
static int	nrec, recording, recover;
static short	stuffbuf[REDOMAX];
static int	nstuff, stuffpos;
static int	ungot = -1;
static unsigned char ibuf[64];
static int	icnt, ipos;

void
rec_start(void)
{
	nrec = recover = 0;
	recording = 1;
}

/*
 * The command is over: if keep, it is the one to repeat.
 */
void
rec_end(int keep)
{
	recording = 0;
	if (keep && !recover && nrec) {
		memcpy((char *)redo, (char *)rec, nrec * sizeof (short));
		nredo = nrec;
	}
}

void
stuff(short *keys, int n)
{
	if (n > REDOMAX)
		n = REDOMAX;
	memcpy((char *)stuffbuf, (char *)keys, n * sizeof (short));
	nstuff = n;
	stuffpos = 0;
}

int
stuffed(void)
{
	return (stuffpos < nstuff);
}

void
ungetkey(int k)
{
	ungot = k;
}

/*
 * Whether there is something typed that has not been read, or
 * something else to see to before more is.
 */
int
t_pending(void)
{
	fd_set r;
	struct timeval tv;

	if (ipos < icnt || ungot >= 0 || stuffpos < nstuff || gone || resized)
		return (1);
	t_flush();
	FD_ZERO(&r);
	FD_SET(0, &r);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	return (select(1, &r, (fd_set *)0, (fd_set *)0, &tv) != 0);
}

/*
 * A byte from the terminal; -1 if none came, because the window
 * changed; and ^C for the signal it sends.  If wait is 0, one that is there already or comes within a
 * fifth of a second, or -1.  The terminal going away ends the editor.
 */
static int
rdchar(int wait)
{
	fd_set r;
	struct timeval tv;

	if (ipos < icnt)
		return (ibuf[ipos++]);
	t_flush();
	if (!wait) {
		FD_ZERO(&r);
		FD_SET(0, &r);
		tv.tv_sec = 0;
		tv.tv_usec = 200000L;
		if (select(1, &r, (fd_set *)0, (fd_set *)0, &tv) <= 0)
			return (-1);
	}
	icnt = read(0, (char *)ibuf, sizeof (ibuf));
	ipos = 0;
	if (icnt == 0 && !wait)
		return (-1);		/* the end: the next read will say */
	if (icnt <= 0) {
		if (icnt < 0 && errno == EINTR && !gone) {
			icnt = 0;
			if (intr) {
				intr = 0;
				return (CTL('C'));
			}
			return (-1);
		}
		quit(-1);
	}
	return (ibuf[ipos++]);
}

/*
 * A key from the terminal.  ESC by itself is a key, and it also begins
 * what the keys with arrows send: ESC [ or ESC O, a number or none, and
 * a letter or ~.  Which it is, is told by whether more follows at once.
 */
static int
readkey(void)
{
	register int c, n;

	if ((c = rdchar(1)) < 0)
		return (K_NONE);
	if (c != ESC)
		return (c);
	if ((c = rdchar(0)) < 0)
		return (ESC);
	if (c != '[' && c != 'O') {
		ipos--;			/* it is the next key */
		return (ESC);
	}
	n = 0;
	while ((c = rdchar(0)) >= 0 && (c >= '0' && c <= '9' || c == ';'))
		if (c != ';' && n < 100)
			n = n * 10 + c - '0';
	switch (c) {
	case 'A':
		return (K_UP);
	case 'B':
		return (K_DOWN);
	case 'C':
		return (K_RIGHT);
	case 'D':
		return (K_LEFT);
	case 'H':
		return (K_HOME);
	case 'F':
		return (K_END);
	case '~':
		switch (n) {
		case 1:
		case 7:
			return (K_HOME);
		case 4:
		case 8:
			return (K_END);
		case 2:
			return (K_INS);
		case 3:
			return (K_DEL);
		case 5:
			return (K_PGUP);
		case 6:
			return (K_PGDN);
		}
	}
	return (K_NONE);
}

int
getkey(void)
{
	register int k;

	if (ungot >= 0) {
		k = ungot;
		ungot = -1;
		return (k);
	}
	if (stuffpos < nstuff)
		k = stuffbuf[stuffpos++];
	else
		k = readkey();
	if (recording && k != K_NONE) {
		if (nrec < REDOMAX)
			rec[nrec++] = k;
		else
			recover = 1;
	}
	return (k);
}

void
rec_key(int k)
{
	if (recording && nrec < REDOMAX)
		rec[nrec++] = k;
}
