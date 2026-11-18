/*
 * The keyboard, on the PS/2's keyboard controller (an 8042 or one like
 * it).  What is typed goes to the console's screen that is shown
 * (../i286/cons.c), as a VT102's keyboard would send it, in the modes
 * the program on that screen has set (vt.h).  The controller turns the
 * keyboard's scan codes into those of the PC/XT, set 1, and the keys
 * are those of the US layout.
 *
 * The keys the VT102 lacks send what DEC's VT220 does: F1 to F4 are
 * PF1 to PF4, F5 to F12 the VT220's F6 to F14, Home, End, Insert,
 * Delete, Page Up and Page Down its editing keys.  Backspace sends BS,
 * as the VT102's does; root's .profile makes that the erase character,
 * as upstream's has always done.  The keypad with Num Lock off is the
 * cursor and editing keys; on, its digits, or the VT102's application
 * keypad when the program asks for that.  Alt with a key sends ESC
 * before it, except that Alt-F1 to Alt-F4 choose the screen.  Scroll
 * Lock stops and starts the output, as the VT102's NO SCROLL key does.
 */
#include "param.h"
#include "systm.h"
#include "vt.h"

#define	KBD_DATA	0x60
#define	KBD_STAT	0x64
#define	KBS_OBF		0x01	/* a byte is waiting */
#define	KBS_IBF		0x02	/* the last byte written has not been taken */
#define	KBS_AUX		0x20	/* the waiting byte is the mouse's */
#define	KBD_IRQ		1

#define	KB_SETLEDS	0xED	/* command: the lights are next */
#define	KB_ACK		0xFA
#define	KB_RESEND	0xFE
#define	LED_SCROLL	0x01
#define	LED_NUM		0x02
#define	LED_CAPS	0x04

/*
 * What a key is, in keymap: a character, or one of these.  The
 * keypad's are K_KP plus 0 to 9 for its digits, then its other keys.
 */
#define	K_F1		0200	/* to K_F12, 0213 */
#define	K_UP		0220
#define	K_DOWN		0221
#define	K_RIGHT		0222
#define	K_LEFT		0223
#define	K_HOME		0224
#define	K_INS		0225
#define	K_DEL		0226
#define	K_END		0227
#define	K_PGUP		0230
#define	K_PGDN		0231
#define	K_KP		0240
#define	K_KPDOT		(K_KP + 10)
#define	K_KPMINUS	(K_KP + 11)
#define	K_KPPLUS	(K_KP + 12)
#define	K_KPSTAR	(K_KP + 13)
#define	K_KPSLASH	(K_KP + 14)
#define	K_KPENTER	(K_KP + 15)
#define	K_LSHIFT	0260
#define	K_RSHIFT	0261
#define	K_LCTRL		0262
#define	K_RCTRL		0263
#define	K_LALT		0264
#define	K_RALT		0265
#define	K_CAPS		0266
#define	K_NUM		0267
#define	K_SCROLL	0270

/* shifts: the modifier keys held, a bit each, in K_LSHIFT's order */
#define	SH_SHIFT	0x03
#define	SH_CTRL		0x0C
#define	SH_ALT		0x30

/* The keys by scan code, and what the shift key makes of the first. */
static u_char keymap[0x59] = {
/* 00 */ 0,	  033,	   '1',	    '2',     '3',     '4',     '5',	'6',
/* 08 */ '7',	  '8',	   '9',	    '0',     '-',     '=',     '\b',	'\t',
/* 10 */ 'q',	  'w',	   'e',	    'r',     't',     'y',     'u',	'i',
/* 18 */ 'o',	  'p',	   '[',	    ']',     '\r',    K_LCTRL, 'a',	's',
/* 20 */ 'd',	  'f',	   'g',	    'h',     'j',     'k',     'l',	';',
/* 28 */ '\'',	  '`',	   K_LSHIFT, '\\',   'z',     'x',     'c',	'v',
/* 30 */ 'b',	  'n',	   'm',	    ',',     '.',     '/',     K_RSHIFT, K_KPSTAR,
/* 38 */ K_LALT,  ' ',	   K_CAPS,  K_F1,    K_F1+1,  K_F1+2,  K_F1+3,	K_F1+4,
/* 40 */ K_F1+5,  K_F1+6,  K_F1+7,  K_F1+8,  K_F1+9,  K_NUM,   K_SCROLL, K_KP+7,
/* 48 */ K_KP+8,  K_KP+9,  K_KPMINUS, K_KP+4, K_KP+5, K_KP+6,  K_KPPLUS, K_KP+1,
/* 50 */ K_KP+2,  K_KP+3,  K_KP+0,  K_KPDOT, 0,	      0,       0,	K_F1+10,
/* 58 */ K_F1+11,
};

static u_char shifted[0x36] = {
/* 00 */ 0,	  033,	   '!',	    '@',     '#',     '$',     '%',	'^',
/* 08 */ '&',	  '*',	   '(',	    ')',     '_',     '+',     '\b',	'\t',
/* 10 */ 'Q',	  'W',	   'E',	    'R',     'T',     'Y',     'U',	'I',
/* 18 */ 'O',	  'P',	   '{',	    '}',     '\r',    K_LCTRL, 'A',	'S',
/* 20 */ 'D',	  'F',	   'G',	    'H',     'J',     'K',     'L',	':',
/* 28 */ '"',	  '~',	   K_LSHIFT, '|',    'Z',     'X',     'C',	'V',
/* 30 */ 'B',	  'N',	   'M',	    '<',     '>',     '?',
};

/* The VT220's numbers for F5 to F12, as ESC [ n ~. */
static u_char fkeys[8] = { 15, 17, 18, 19, 20, 21, 23, 24 };

/* The editing keys' numbers, from K_HOME on. */
static u_char editkeys[6] = { 1, 2, 3, 4, 5, 6 };

/*
 * What the keypad's keys send: in the application keypad, after ESC O
 * (ESC ? for a VT52), and otherwise.  The digits, then . - + * / Enter.
 */
static char kpapp[] = "pqrstuvwxynmkjoM";
static char kpchars[] = "0123456789.-+*/\r";

/* The keypad with Num Lock off: the keys printed under its digits. */
static u_char kpnav[11] = {
	K_INS, K_END, K_DOWN, K_PGDN, K_LEFT, 0, K_RIGHT, K_HOME, K_UP,
	K_PGUP, K_DEL
};

static	u_char shifts;		/* SH_ bits */
static	u_char locks = LED_NUM;	/* LED_ bits: the lock keys that are on */
static	u_char e0;		/* the last byte was the prefix E0 */
static	u_char skip;		/* bytes of the Pause key still to come */
static	u_char held;		/* the key last pressed, if not let go: its
				   code, with 0200 for E0 */
static	u_char ledstep;		/* 1: KB_SETLEDS sent, 2: the lights sent;
				   waiting for the keyboard to take it */
static	u_char ledsent;		/* the lights sent */
static	int kbdintr();

/*
 * Write a byte to the keyboard, once the controller can take it.
 */
static
kbdsend(c)
	int c;
{
	register int timo;

	for (timo = 10000; timo > 0; timo--)
		if ((inb(KBD_STAT) & KBS_IBF) == 0)
			break;
	outb(KBD_DATA, c);
}

/*
 * Make the lights show the locks.  The keyboard answers each byte, and
 * the next is sent when it does (kbdbyte).
 */
static
kbdleds()
{
	if (ledstep == 0) {
		ledstep = 1;
		kbdsend(KB_SETLEDS);
	}
}

kbdinit()
{
	register int i;

	for (i = 0; i < 16 && (inb(KBD_STAT) & KBS_OBF); i++)
		(void) inb(KBD_DATA);
	setirq(KBD_IRQ, IPL_TTY, kbdintr, 0);
	kbdleds();
}

/*
 * Send what a key makes, n bytes at cp, prefixed with ESC if Alt is
 * held.
 */
static
send(cp, n)
	char *cp;
	int n;
{
	if (shifts & SH_ALT)
		cninput("\033", 1);
	cninput(cp, n);
}

/* ESC, then intro if it is not 0, then final. */
static
seq(intro, final)
	int intro, final;
{
	char buf[3];
	register char *cp = buf;

	*cp++ = '\033';
	if (intro)
		*cp++ = intro;
	*cp++ = final;
	send(buf, cp - buf);
}

/* ESC [ n ~, the VT220's way. */
static
tilde(n)
	int n;
{
	char buf[6];
	register char *cp = buf;

	*cp++ = '\033';
	*cp++ = '[';
	if (n >= 10)
		*cp++ = '0' + n / 10;
	*cp++ = '0' + n % 10;
	*cp++ = '~';
	send(buf, cp - buf);
}

/* A character with Control held. */
static
ctrl(c)
	register int c;
{
	if (c >= 0100 && c < 0177)
		return (c & 037);
	switch (c) {
	case ' ':
	case '2':
		return (0);
	case '6':
		return (036);
	case '-':
	case '/':
		return (037);
	case '?':
		return (0177);
	}
	return (c);
}

/*
 * A key other than a character or a modifier.
 */
static
special(k, flags)
	register int k;
	int flags;
{
	int vt52 = (flags & VT_VT52) != 0;
	char c;

	if (k >= K_F1 && k < K_F1 + 4)
		seq(vt52 ? 0 : 'O', 'P' + k - K_F1);
	else if (k >= K_F1 + 4 && k < K_F1 + 12)
		tilde(fkeys[k - K_F1 - 4]);
	else if (k >= K_UP && k <= K_LEFT)
		seq(vt52 ? 0 : flags & VT_CKM ? 'O' : '[', "ABCD"[k - K_UP]);
	else if (k >= K_HOME && k <= K_PGDN)
		tilde(editkeys[k - K_HOME]);
	else if (k >= K_KP && k <= K_KPENTER) {
		k -= K_KP;
		if (k <= 10 && (locks & LED_NUM) == 0) {
			if (kpnav[k])
				special(kpnav[k], flags);
		} else if (flags & VT_KPAM)
			seq(vt52 ? '?' : 'O', kpapp[k]);
		else if (k == 15 && (flags & VT_LNM))
			send("\r\n", 2);
		else {
			c = kpchars[k];
			send(&c, 1);
		}
	}
}

/*
 * A key was pressed: its code, with 0200 for E0, and what it is.
 */
static
press(code, k)
	int code;
	register int k;
{
	int flags = cnkbdflags();
	char c;

	switch (k) {
	case K_LSHIFT:
	case K_RSHIFT:
	case K_LCTRL:
	case K_RCTRL:
	case K_LALT:
	case K_RALT:
		shifts |= 1 << (k - K_LSHIFT);
		return;
	case K_CAPS:
	case K_NUM:
	case K_SCROLL:
		if (held == code)
			return;		/* repeating */
		held = code;
		locks ^= k == K_CAPS ? LED_CAPS : k == K_NUM ? LED_NUM :
		    LED_SCROLL;
		kbdleds();
		if (k == K_SCROLL)
			cnhold();
		return;
	}
	if (held == code && (flags & VT_ARM) == 0)
		return;
	held = code;
	if ((shifts & SH_ALT) && k >= K_F1 && k < K_F1 + 4) {
		cnswitch(k - K_F1);
		return;
	}
	if (k >= 0200) {
		special(k, flags);
		return;
	}
	if ((shifts & SH_SHIFT) && code < sizeof (shifted))
		k = shifted[code];
	if (locks & LED_CAPS)
		if (k >= 'a' && k <= 'z' || k >= 'A' && k <= 'Z')
			k ^= 040;
	if (shifts & SH_CTRL)
		k = ctrl(k);
	if (k == '\r' && (flags & VT_LNM))
		send("\r\n", 2);
	else {
		c = k;
		send(&c, 1);
	}
}

/* A key was let go. */
static
release(code, k)
	int code, k;
{
	if (k >= K_LSHIFT && k <= K_RALT)
		shifts &= ~(1 << (k - K_LSHIFT));
	if (held == code)
		held = 0;
}

/* What an E0 key is. */
static
extended(c)
	int c;
{
	switch (c) {
	case 0x1C:	return (K_KPENTER);
	case 0x1D:	return (K_RCTRL);
	case 0x35:	return (K_KPSLASH);
	case 0x38:	return (K_RALT);
	case 0x47:	return (K_HOME);
	case 0x48:	return (K_UP);
	case 0x49:	return (K_PGUP);
	case 0x4B:	return (K_LEFT);
	case 0x4D:	return (K_RIGHT);
	case 0x4F:	return (K_END);
	case 0x50:	return (K_DOWN);
	case 0x51:	return (K_PGDN);
	case 0x52:	return (K_INS);
	case 0x53:	return (K_DEL);
	}
	return (0);	/* Print Screen and its false shifts, and others */
}

/*
 * A byte from the keyboard.
 */
static
kbdbyte(c)
	register int c;
{
	register int k, code;

	switch (c) {
	case KB_ACK:
		if (ledstep == 1) {
			ledstep = 2;
			ledsent = locks;
			kbdsend(ledsent);
		} else if (ledstep == 2) {
			ledstep = 0;
			if (ledsent != locks)
				kbdleds();
		}
		return;
	case KB_RESEND:
		if (ledstep)
			kbdsend(ledstep == 1 ? KB_SETLEDS : ledsent);
		return;
	case 0x00:		/* too much typed at once */
	case 0xFF:
		return;
	case 0xE0:
		e0 = 1;
		return;
	case 0xE1:		/* Pause: E1 1D 45 E1 9D C5 */
		skip = 2;
		return;
	}
	if (skip) {
		skip--;
		return;
	}
	code = c & 0177;
	if (e0) {
		k = extended(code);
		code |= 0200;
		e0 = 0;
	} else
		k = code < sizeof (keymap) ? keymap[code] : 0;
	if (k == 0)
		return;
	if (c & 0200)
		release(code, k);
	else
		press(code, k);
}

/*ARGSUSED*/
static
kbdintr(arg, tf)
	int arg;
	caddr_t tf;
{
	register int i, st;

	for (i = 0; i < 16; i++) {
		st = inb(KBD_STAT);
		if ((st & KBS_OBF) == 0)
			break;
		if (st & KBS_AUX)
			(void) inb(KBD_DATA);
		else
			kbdbyte(inb(KBD_DATA));
	}
}
