/*
 * A terminal on a screen.  What a program writes to one of the
 * console's screens is drawn here as a DEC VT102 would draw it.  The
 * screen is a page of the display adapter's memory: VT_ROWS rows of
 * VT_COLS cells, each a character of the adapter's font (code page 437)
 * and an attribute byte.  ../i286/cons.c gives each of its screens one
 * and hands this what is written to it.
 *
 * The VT102 is the reference: its user guide, with DEC's later manuals
 * where it leaves a case open.  Everything it does to its screen is
 * here except double-width and double-height lines, 132 columns and
 * smooth scrolling, which a PC's display cannot do.  To that are added
 * a few things of later terminals that programs written for PC consoles
 * expect, all of which Linux's console also has: colours (SGR 30 to 49),
 * erasing in the background colour, ICH, ECH, CHA, VPA, CNL, CPL, HPR,
 * VPR, SCOSC and SCORC (CSI s and u), and hiding the cursor (DECTCEM).
 * The keyboard half of the terminal is kbd.c.
 *
 * Nothing here touches the hardware, so tests/vt can check it on the
 * machine the system is built on.
 */
#include "param.h"
#include "vt.h"

#define	ESC		033

/* v_state */
#define	S_TEXT		0	/* not in a sequence */
#define	S_ESC		1	/* after ESC */
#define	S_ESCI		2	/* after ESC and an intermediate character */
#define	S_CSI		3	/* in a control sequence's parameters */
#define	S_CSII		4	/* in its intermediate characters */
#define	S_VT52Y		5	/* after the VT52's ESC Y: the row is next */
#define	S_VT52X		6	/*   and then the column */

/* v_ren */
#define	R_BOLD		1
#define	R_UNDER		2
#define	R_BLINK		4
#define	R_REVERSE	8

#define	FG		7	/* the default colours: grey */
#define	BG		0	/*   on black */
#define	UNDERFG		3	/* underlined characters' colour: cyan.  A
				   colour display cannot underline */
#define	NORMAL		0x07	/* the attribute of plain text */
#define	MAXPAR		9999	/* a larger parameter is taken as this */

#define	CELL(vp, x, y)	((vp)->v_cells + (y) * VT_COLS + (x))
#define	ERASED(vp)	((u_int)(vp)->v_eattr << 8 | ' ')
#define	LASTROW		(VT_ROWS - 1)
#define	LASTCOL		(VT_COLS - 1)

/* An attribute byte with foreground and background exchanged. */
#define	SWAP(a)		((a) & 0x88 | (a) >> 4 & 0x07 | ((a) & 0x07) << 4)

/*
 * The DEC special graphics character set, 0137 to 0176, in code page
 * 437.  The symbols the font lacks, which are the control characters
 * HT, FF, CR, LF, NL and VT as single characters and "not equal", are a
 * small square, and the scan lines 1, 3 and 7 the line in the middle.
 */
static u_char graphics[32] = {
	' ',  0x04, 0xB1, 0xFE, 0xFE, 0xFE, 0xFE, 0xF8,	/* _ ` a b c d e f */
	0xF1, 0xFE, 0xFE, 0xD9, 0xBF, 0xDA, 0xC0, 0xC5,	/* g h i j k l m n */
	0xC4, 0xC4, 0xC4, 0xC4, '_',  0xC3, 0xB4, 0xC1,	/* o p q r s t u v */
	0xC2, 0xB3, 0xF3, 0xF2, 0xE3, 0xFE, 0x9C, 0xFA,	/* w x y z { | } ~ */
};

static
fill(p, n, w)
	register u_short __far *p;
	register int n;
	u_short w;
{
	while (--n >= 0)
		*p++ = w;
}

/*
 * Work out the attribute bytes from the rendition and the colours.
 */
static
attrs(vp)
	register struct vt *vp;
{
	register int a, e;

	if (vp->v_mono) {
		a = vp->v_ren & R_UNDER ? 0x01 : NORMAL;
		e = NORMAL;
	} else {
		a = vp->v_ren & R_UNDER && vp->v_fg == FG ? UNDERFG : vp->v_fg;
		a |= vp->v_bg << 4;
		e = vp->v_fg | vp->v_bg << 4;
	}
	if (vp->v_ren & R_BOLD)
		a |= 0x08;
	if (((vp->v_ren & R_REVERSE) != 0) != ((vp->v_flags & VT_SCNM) != 0))
		a = SWAP(a);
	if (vp->v_ren & R_BLINK)
		a |= 0x80;
	if (vp->v_flags & VT_SCNM)
		e = SWAP(e);
	vp->v_attr = a;
	vp->v_eattr = e;
}

/*
 * Move rows top + n to bot up n, and erase the n at the bottom; or the
 * other way about.
 */
static
scrollup(vp, top, bot, n)
	register struct vt *vp;
	int top, bot;
	register int n;
{
	if (n > bot - top + 1)
		n = bot - top + 1;
	if (n <= bot - top)
		bcopy(CELL(vp, 0, top + n), CELL(vp, 0, top),
		    (bot - top + 1 - n) * VT_COLS * sizeof (u_short));
	fill(CELL(vp, 0, bot + 1 - n), n * VT_COLS, ERASED(vp));
}

static
scrolldown(vp, top, bot, n)
	register struct vt *vp;
	int top, bot;
	register int n;
{
	if (n > bot - top + 1)
		n = bot - top + 1;
	if (n <= bot - top)
		bcopy(CELL(vp, 0, top), CELL(vp, 0, top + n),
		    (bot - top + 1 - n) * VT_COLS * sizeof (u_short));
	fill(CELL(vp, 0, top), n * VT_COLS, ERASED(vp));
}

/*
 * Index: down a line, scrolling if the cursor is at the bottom of the
 * scrolling region.  Below the region it stops at the last row.
 */
static
lf(vp)
	register struct vt *vp;
{
	if (vp->v_y == vp->v_bot)
		scrollup(vp, vp->v_top, vp->v_bot, 1);
	else if (vp->v_y < LASTROW)
		vp->v_y++;
	vp->v_flags &= ~VT_WRAP;
}

/* Reverse index. */
static
ri(vp)
	register struct vt *vp;
{
	if (vp->v_y == vp->v_top)
		scrolldown(vp, vp->v_top, vp->v_bot, 1);
	else if (vp->v_y > 0)
		vp->v_y--;
	vp->v_flags &= ~VT_WRAP;
}

/*
 * Put the cursor at column x of row y, which counts from the top of the
 * scrolling region in origin mode, and cannot leave it then.
 */
static
cup(vp, x, y)
	register struct vt *vp;
	register int x, y;
{
	int top, bot;

	if (vp->v_flags & VT_OM) {
		top = vp->v_top;
		bot = vp->v_bot;
		y += top;
	} else {
		top = 0;
		bot = LASTROW;
	}
	vp->v_x = x < 0 ? 0 : x > LASTCOL ? LASTCOL : x;
	vp->v_y = y < top ? top : y > bot ? bot : y;
	vp->v_flags &= ~VT_WRAP;
}

/*
 * Cursor up and down: n rows, stopping at the edge of the scrolling
 * region if the cursor is in it, and of the screen if not.
 */
static
up(vp, n)
	register struct vt *vp;
	int n;
{
	register int y = vp->v_y - n, lim;

	lim = vp->v_y >= vp->v_top ? vp->v_top : 0;
	vp->v_y = y < lim ? lim : y;
	vp->v_flags &= ~VT_WRAP;
}

static
down(vp, n)
	register struct vt *vp;
	int n;
{
	register int y = vp->v_y + n, lim;

	lim = vp->v_y <= vp->v_bot ? vp->v_bot : LASTROW;
	vp->v_y = y > lim ? lim : y;
	vp->v_flags &= ~VT_WRAP;
}

static
right(vp, n)
	register struct vt *vp;
	int n;
{
	register int x = vp->v_x + n;

	vp->v_x = x > LASTCOL ? LASTCOL : x;
	vp->v_flags &= ~VT_WRAP;
}

static
left(vp, n)
	register struct vt *vp;
	int n;
{
	register int x = vp->v_x - n;

	vp->v_x = x < 0 ? 0 : x;
	vp->v_flags &= ~VT_WRAP;
}

/* To the next tab stop, or the last column. */
static
tab(vp)
	register struct vt *vp;
{
	register int x = vp->v_x;

	while (x < LASTCOL) {
		x++;
		if (vp->v_tabs[x >> 3] & 1 << (x & 7))
			break;
	}
	vp->v_x = x;
}

/*
 * Erase in display (ED) and in line (EL): from the cursor to the end, 0;
 * from the beginning to the cursor, 1; or all of it, 2.
 */
static
ed(vp, how)
	register struct vt *vp;
	int how;
{
	register int at = vp->v_y * VT_COLS + vp->v_x;

	switch (how) {
	case 0:
		fill(vp->v_cells + at, VT_ROWS * VT_COLS - at, ERASED(vp));
		break;
	case 1:
		fill(vp->v_cells, at + 1, ERASED(vp));
		break;
	case 2:
		fill(vp->v_cells, VT_ROWS * VT_COLS, ERASED(vp));
		break;
	}
	vp->v_flags &= ~VT_WRAP;
}

static
el(vp, how)
	register struct vt *vp;
	int how;
{
	switch (how) {
	case 0:
		fill(CELL(vp, vp->v_x, vp->v_y), VT_COLS - vp->v_x,
		    ERASED(vp));
		break;
	case 1:
		fill(CELL(vp, 0, vp->v_y), vp->v_x + 1, ERASED(vp));
		break;
	case 2:
		fill(CELL(vp, 0, vp->v_y), VT_COLS, ERASED(vp));
		break;
	}
	vp->v_flags &= ~VT_WRAP;
}

/*
 * Insert and delete lines (IL, DL) at the cursor's, within the scrolling
 * region; outside it they do nothing.  The cursor goes to the first
 * column, as DEC's later terminals say.
 */
static
il(vp, n, del)
	register struct vt *vp;
	int n, del;
{
	if (vp->v_y < vp->v_top || vp->v_y > vp->v_bot)
		return;
	if (del)
		scrollup(vp, vp->v_y, vp->v_bot, n);
	else
		scrolldown(vp, vp->v_y, vp->v_bot, n);
	vp->v_x = 0;
	vp->v_flags &= ~VT_WRAP;
}

/*
 * Insert, delete and erase characters (ICH, DCH, ECH) at the cursor.
 */
static
ich(vp, n, how)
	register struct vt *vp;
	register int n;
	int how;
{
	register u_short __far *p = CELL(vp, vp->v_x, vp->v_y);
	int rest = VT_COLS - vp->v_x;

	if (n > rest)
		n = rest;
	switch (how) {
	case '@':
		bcopy(p, p + n, (rest - n) * sizeof (u_short));
		fill(p, n, ERASED(vp));
		break;
	case 'P':
		bcopy(p + n, p, (rest - n) * sizeof (u_short));
		fill(p + rest - n, n, ERASED(vp));
		break;
	case 'X':
		fill(p, n, ERASED(vp));
		break;
	}
	vp->v_flags &= ~VT_WRAP;
}

/* Save the cursor (DECSC), and put it back as it was (DECRC). */
static
save(vp)
	register struct vt *vp;
{
	register struct vtsave *sp = &vp->v_save;

	sp->s_x = vp->v_x;
	sp->s_y = vp->v_y;
	sp->s_ren = vp->v_ren;
	sp->s_fg = vp->v_fg;
	sp->s_bg = vp->v_bg;
	sp->s_g[0] = vp->v_g[0];
	sp->s_g[1] = vp->v_g[1];
	sp->s_gl = vp->v_gl;
	sp->s_flags = vp->v_flags & (VT_OM | VT_WRAP);
}

static
restore(vp)
	register struct vt *vp;
{
	register struct vtsave *sp = &vp->v_save;

	vp->v_x = sp->s_x;
	vp->v_y = sp->s_y;
	vp->v_ren = sp->s_ren;
	vp->v_fg = sp->s_fg;
	vp->v_bg = sp->s_bg;
	vp->v_g[0] = sp->s_g[0];
	vp->v_g[1] = sp->s_g[1];
	vp->v_gl = sp->s_gl;
	vp->v_flags = vp->v_flags & ~(VT_OM | VT_WRAP) | sp->s_flags;
	attrs(vp);
}

/*
 * The modes a terminal starts in, and the state DECRC returns to when
 * nothing was saved.  The cursor and the screen are left alone.
 */
static
modes0(vp)
	register struct vt *vp;
{
	register int i;
	int x = vp->v_x, y = vp->v_y;

	vp->v_flags = VT_AWM | VT_TCEM | VT_ARM;
	vp->v_top = 0;
	vp->v_bot = LASTROW;
	vp->v_state = S_TEXT;
	vp->v_ren = 0;
	vp->v_fg = FG;
	vp->v_bg = BG;
	vp->v_g[0] = vp->v_g[1] = 'B';
	vp->v_gl = 0;
	for (i = 0; i < VT_COLS / 8; i++)
		vp->v_tabs[i] = 1;		/* every eighth column */
	attrs(vp);
	vp->v_x = vp->v_y = 0;
	save(vp);
	vp->v_x = x;
	vp->v_y = y;
}

/*
 * A terminal on the screen at cells, which is left as it is, with the
 * cursor at its top left; the caller may move the cursor.  mono says
 * the display is monochrome, which has its own attributes.
 */
vtinit(vp, cells, mono)
	register struct vt *vp;
	u_short __far *cells;
	int mono;
{
	vp->v_cells = cells;
	vp->v_mono = mono;
	vp->v_x = vp->v_y = 0;
	modes0(vp);
}

/* Reset (RIS): the starting modes, and the screen cleared. */
vtreset(vp)
	register struct vt *vp;
{
	modes0(vp);
	vp->v_x = vp->v_y = 0;
	fill(vp->v_cells, VT_ROWS * VT_COLS, ERASED(vp));
}

/*
 * Put the cell w where the cursor is, and move the cursor on; with
 * insert, the rest of the line moves right to make room.  At the last
 * column the cursor stays where it is, and the next character goes on
 * the next line, if autowrap is on, or over this one.
 */
static
put(vp, w, insert)
	register struct vt *vp;
	u_short w;
	int insert;
{
	register u_short __far *p;

	if (vp->v_flags & VT_WRAP) {
		vp->v_x = 0;
		lf(vp);
	}
	p = CELL(vp, vp->v_x, vp->v_y);
	if (insert && vp->v_x < LASTCOL)
		bcopy(p, p + 1, (LASTCOL - vp->v_x) * sizeof (u_short));
	*p = w;
	if (vp->v_x < LASTCOL)
		vp->v_x++;
	else if (vp->v_flags & VT_AWM)
		vp->v_flags |= VT_WRAP;
}

/*
 * A character to show, in the character set in use.  Codes from 0200
 * up are the font's own.
 */
static
glyph(vp, c)
	register struct vt *vp;
	register int c;
{
	register int set;

	if (c < 0200) {
		set = vp->v_g[vp->v_gl];
		if (set == '0' && c >= 0137)
			c = graphics[c - 0137];
		else if (set == 'A' && c == '#')
			c = 0x9C;		/* the pound sign */
	}
	put(vp, (u_int)vp->v_attr << 8 | c, vp->v_flags & VT_IRM);
}

/*
 * A control character.  They do what they do in the middle of an escape
 * sequence too, except CAN and SUB, which end it, and ESC, which starts
 * a new one.
 */
static
control(vp, c)
	register struct vt *vp;
	int c;
{
	switch (c) {
	case 007:		/* BEL */
		vtbell(vp);
		break;
	case 010:		/* BS: from the last column after a character
				   went in it, to the one before */
		vp->v_flags &= ~VT_WRAP;
		if (vp->v_x > 0)
			vp->v_x--;
		break;
	case 011:		/* HT */
		tab(vp);
		break;
	case 012:		/* LF, VT and FF */
	case 013:
	case 014:
		lf(vp);
		if (vp->v_flags & VT_LNM)
			vp->v_x = 0;
		break;
	case 015:		/* CR */
		vp->v_x = 0;
		vp->v_flags &= ~VT_WRAP;
		break;
	case 016:		/* SO */
		vp->v_gl = 1;
		break;
	case 017:		/* SI */
		vp->v_gl = 0;
		break;
	case 030:		/* CAN */
	case 032:		/* SUB */
		vp->v_state = S_TEXT;
		break;
	case ESC:
		vp->v_state = S_ESC;
		vp->v_inter = 0;
		break;
	}
}

/* Write the decimal number n at cp, and return where it ends. */
static char *
decimal(cp, n)
	register char *cp;
	register int n;
{
	if (n >= 10)
		cp = decimal(cp, n / 10);
	*cp++ = '0' + n % 10;
	return (cp);
}

/*
 * Answer as if typed: "\033[" and then the numbers, separated by
 * semicolons, and the final character.
 */
static
answer(vp, final, nnum, n1, n2)
	struct vt *vp;
	int final, nnum, n1, n2;
{
	char buf[16];
	register char *cp = buf;

	*cp++ = ESC;
	*cp++ = '[';
	cp = decimal(cp, n1);
	if (nnum > 1) {
		*cp++ = ';';
		cp = decimal(cp, n2);
	}
	*cp++ = final;
	*cp = '\0';
	vtreply(vp, buf);
}

/* The screen alignment pattern (DECALN): a screen full of E. */
static
decaln(vp)
	register struct vt *vp;
{
	fill(vp->v_cells, VT_ROWS * VT_COLS, (u_int)vp->v_eattr << 8 | 'E');
	vp->v_top = 0;
	vp->v_bot = LASTROW;
	vp->v_flags &= ~VT_OM;
	cup(vp, 0, 0);
}

/*
 * The final character of an escape sequence, ESC and then it, perhaps
 * with an intermediate character between.
 */
static
esc(vp, c)
	register struct vt *vp;
	int c;
{
	register int i;

	switch (vp->v_inter) {
	case 0:
		break;
	case '(':		/* designate G0 */
	case ')':		/*   or G1 */
		vp->v_g[vp->v_inter == ')'] =
		    c == '0' || c == '2' ? '0' : c == 'A' ? 'A' : 'B';
		return;
	case '#':
		if (c == '8')
			decaln(vp);
		return;		/* 3 to 6, the double sizes, cannot be */
	default:
		return;
	}
	switch (c) {
	case '[':		/* CSI */
		vp->v_state = S_CSI;
		vp->v_npar = vp->v_priv = vp->v_any = vp->v_bad = 0;
		for (i = 0; i < VT_NPAR; i++)
			vp->v_par[i] = 0;
		break;
	case '7':		/* DECSC */
		save(vp);
		break;
	case '8':		/* DECRC */
		restore(vp);
		break;
	case 'D':		/* IND */
		lf(vp);
		break;
	case 'E':		/* NEL */
		vp->v_x = 0;
		lf(vp);
		break;
	case 'M':		/* RI */
		ri(vp);
		break;
	case 'H':		/* HTS */
		vp->v_tabs[vp->v_x >> 3] |= 1 << (vp->v_x & 7);
		break;
	case 'Z':		/* DECID: say what this is, as DA does */
		vtreply(vp, "\033[?6c");
		break;
	case 'c':		/* RIS */
		vtreset(vp);
		break;
	case '=':		/* DECKPAM */
		vp->v_flags |= VT_KPAM;
		break;
	case '>':		/* DECKPNM */
		vp->v_flags &= ~VT_KPAM;
		break;
	}
}

/*
 * Set or reset modes (SM, RM, and DEC's own with "?").  132 columns
 * (DECCOLM) and smooth scrolling cannot be, and are ignored.
 */
static
setmode(vp, on)
	register struct vt *vp;
	int on;
{
	register int i, bit;
	int n = vp->v_npar < VT_NPAR ? vp->v_npar + 1 : VT_NPAR;

	for (i = 0; i < n; i++) {
		bit = 0;
		if (vp->v_priv == '?')
			switch (vp->v_par[i]) {
			case 1:
				bit = VT_CKM;
				break;
			case 2:		/* DECANM: reset is VT52 mode */
				if (!on)
					vp->v_flags |= VT_VT52;
				break;
			case 5:
				if (((vp->v_flags & VT_SCNM) != 0) != on) {
					register u_short __far *p;

					vp->v_flags ^= VT_SCNM;
					attrs(vp);
					for (p = vp->v_cells;
					    p < vp->v_cells + VT_ROWS * VT_COLS;
					    p++)
						*p = (u_int)SWAP(*p >> 8) << 8 |
						    *p & 0xFF;
				}
				break;
			case 6:
				bit = VT_OM;
				break;
			case 7:
				bit = VT_AWM;
				break;
			case 8:
				bit = VT_ARM;
				break;
			case 25:
				bit = VT_TCEM;
				break;
			}
		else
			switch (vp->v_par[i]) {
			case 4:
				bit = VT_IRM;
				break;
			case 20:
				bit = VT_LNM;
				break;
			}
		if (on)
			vp->v_flags |= bit;
		else
			vp->v_flags &= ~bit;
		if (bit == VT_OM)
			cup(vp, 0, 0);
		if (bit == VT_AWM && !on)
			vp->v_flags &= ~VT_WRAP;
	}
}

/* Select graphic rendition (SGR). */
static
sgr(vp)
	register struct vt *vp;
{
	register int i, p;
	int c, n = vp->v_npar < VT_NPAR ? vp->v_npar + 1 : VT_NPAR;

	for (i = 0; i < n; i++) {
		p = vp->v_par[i];
		switch (p) {
		case 0:
			vp->v_ren = 0;
			vp->v_fg = FG;
			vp->v_bg = BG;
			break;
		case 1:
			vp->v_ren |= R_BOLD;
			break;
		case 4:
			vp->v_ren |= R_UNDER;
			break;
		case 5:
			vp->v_ren |= R_BLINK;
			break;
		case 7:
			vp->v_ren |= R_REVERSE;
			break;
		case 22:
			vp->v_ren &= ~R_BOLD;
			break;
		case 24:
			vp->v_ren &= ~R_UNDER;
			break;
		case 25:
			vp->v_ren &= ~R_BLINK;
			break;
		case 27:
			vp->v_ren &= ~R_REVERSE;
			break;
		case 39:
			vp->v_fg = FG;
			break;
		case 49:
			vp->v_bg = BG;
			break;
		default:
			/* ANSI's colour numbers are the PC's backwards */
			c = p % 10;
			c = (c & 1) << 2 | c & 2 | (c & 4) >> 2;
			if (p >= 30 && p <= 37)
				vp->v_fg = c;
			else if (p >= 40 && p <= 47)
				vp->v_bg = c;
			break;
		}
	}
	attrs(vp);
}

/* The final character of a control sequence: do it. */
static
csi(vp, c)
	register struct vt *vp;
	int c;
{
	register int p0 = vp->v_par[0], n;

	n = p0 ? p0 : 1;		/* a count, which defaults to 1 */
	if (vp->v_bad || vp->v_inter)
		return;
	if (vp->v_priv == '?') {
		switch (c) {
		case 'h':
			setmode(vp, 1);
			break;
		case 'l':
			setmode(vp, 0);
			break;
		case 'n':		/* DSR: the printer's status */
			if (p0 == 15)
				vtreply(vp, "\033[?13n");	/* none */
			break;
		}
		return;
	}
	if (vp->v_priv)
		return;
	switch (c) {
	case 'A':		/* CUU */
		up(vp, n);
		break;
	case 'B':		/* CUD */
	case 'e':		/* VPR */
		down(vp, n);
		break;
	case 'C':		/* CUF */
	case 'a':		/* HPR */
		right(vp, n);
		break;
	case 'D':		/* CUB */
		left(vp, n);
		break;
	case 'E':		/* CNL */
		vp->v_x = 0;
		down(vp, n);
		break;
	case 'F':		/* CPL */
		vp->v_x = 0;
		up(vp, n);
		break;
	case 'G':		/* CHA */
	case '`':		/* HPA */
		vp->v_x = n > VT_COLS ? LASTCOL : n - 1;
		vp->v_flags &= ~VT_WRAP;
		break;
	case 'd':		/* VPA */
		cup(vp, vp->v_x, n - 1);
		break;
	case 'H':		/* CUP */
	case 'f':		/* HVP */
		cup(vp, vp->v_par[1] ? vp->v_par[1] - 1 : 0, n - 1);
		break;
	case 'J':		/* ED */
		ed(vp, p0);
		break;
	case 'K':		/* EL */
		el(vp, p0);
		break;
	case 'L':		/* IL */
		il(vp, n, 0);
		break;
	case 'M':		/* DL */
		il(vp, n, 1);
		break;
	case '@':		/* ICH */
	case 'P':		/* DCH */
	case 'X':		/* ECH */
		ich(vp, n, c);
		break;
	case 'c':		/* DA: a VT102 */
		if (p0 == 0)
			vtreply(vp, "\033[?6c");
		break;
	case 'g':		/* TBC */
		if (p0 == 0)
			vp->v_tabs[vp->v_x >> 3] &= ~(1 << (vp->v_x & 7));
		else if (p0 == 3)
			for (n = 0; n < VT_COLS / 8; n++)
				vp->v_tabs[n] = 0;
		break;
	case 'h':		/* SM */
		setmode(vp, 1);
		break;
	case 'l':		/* RM */
		setmode(vp, 0);
		break;
	case 'm':		/* SGR */
		sgr(vp);
		break;
	case 'n':		/* DSR */
		if (p0 == 5)
			vtreply(vp, "\033[0n");		/* no malfunction */
		else if (p0 == 6)			/* CPR */
			answer(vp, 'R', 2, vp->v_y + 1 -
			    (vp->v_flags & VT_OM ? vp->v_top : 0), vp->v_x + 1);
		break;
	case 'r':		/* DECSTBM */
		n = vp->v_npar >= 1 && vp->v_par[1] ? vp->v_par[1] : VT_ROWS;
		p0 = p0 ? p0 : 1;
		if (p0 < n && n <= VT_ROWS) {
			vp->v_top = p0 - 1;
			vp->v_bot = n - 1;
			cup(vp, 0, 0);
		}
		break;
	case 's':		/* SCOSC */
		save(vp);
		break;
	case 'u':		/* SCORC */
		restore(vp);
		break;
	case 'x':		/* DECREQTPARM: no parity, 8 bits, 9600 */
		if (p0 == 0)
			vtreply(vp, "\033[2;1;1;112;112;1;0x");
		else if (p0 == 1)
			vtreply(vp, "\033[3;1;1;112;112;1;0x");
		break;
	}
}

/*
 * ESC and then c, in VT52 mode.
 */
static
vt52(vp, c)
	register struct vt *vp;
	int c;
{
	switch (c) {
	case 'A':
		up(vp, 1);
		break;
	case 'B':
		down(vp, 1);
		break;
	case 'C':
		right(vp, 1);
		break;
	case 'D':
		left(vp, 1);
		break;
	case 'F':		/* the graphics characters */
		vp->v_g[0] = '0';
		vp->v_gl = 0;
		break;
	case 'G':		/* and the ordinary ones */
		vp->v_g[0] = 'B';
		break;
	case 'H':
		cup(vp, 0, 0);
		break;
	case 'I':
		ri(vp);
		break;
	case 'J':
		ed(vp, 0);
		break;
	case 'K':
		el(vp, 0);
		break;
	case 'Y':		/* the row and column follow, each plus 040 */
		vp->v_state = S_VT52Y;
		break;
	case 'Z':
		vtreply(vp, "\033/Z");
		break;
	case '=':
		vp->v_flags |= VT_KPAM;
		break;
	case '>':
		vp->v_flags &= ~VT_KPAM;
		break;
	case '<':		/* back to ANSI mode */
		vp->v_flags &= ~VT_VT52;
		break;
	}
}

/*
 * What was written: n characters at cp.
 */
vtwrite(vp, cp, n)
	register struct vt *vp;
	register u_char *cp;
	register int n;
{
	register int c;

	while (--n >= 0) {
		c = *cp++;
		if (c < 040) {
			control(vp, c);
			continue;
		}
		if (c == 0177)
			continue;
		switch (vp->v_state) {
		case S_TEXT:
			glyph(vp, c);
			break;

		case S_ESC:
			if (vp->v_flags & VT_VT52) {
				vp->v_state = S_TEXT;
				vt52(vp, c);
				break;
			}
			/* FALLTHROUGH */
		case S_ESCI:
			if (c >= 040 && c < 060) {
				vp->v_inter = vp->v_inter ? 0377 : c;
				vp->v_state = S_ESCI;
				break;
			}
			vp->v_state = S_TEXT;
			if (c < 0177)
				esc(vp, c);
			break;

		case S_CSI:
			if (c >= '0' && c <= '9') {
				if (vp->v_npar < VT_NPAR) {
					u_short *pp = &vp->v_par[vp->v_npar];

					*pp = *pp >= MAXPAR / 10 ? MAXPAR :
					    *pp * 10 + c - '0';
				}
				vp->v_any = 1;
				break;
			}
			if (c == ';') {
				if (vp->v_npar < VT_NPAR)
					vp->v_npar++;
				vp->v_any = 1;
				break;
			}
			if (c >= '<' && c <= '?') {	/* a private marker */
				if (vp->v_any || vp->v_priv)
					vp->v_bad = 1;
				else
					vp->v_priv = c;
				vp->v_any = 1;
				break;
			}
			if (c == ':') {
				vp->v_bad = 1;
				break;
			}
			/* FALLTHROUGH */
		case S_CSII:
			if (c >= 040 && c < 060) {
				vp->v_inter = vp->v_inter ? 0377 : c;
				vp->v_state = S_CSII;
				break;
			}
			if (c >= 0100 && c < 0177) {
				vp->v_state = S_TEXT;
				csi(vp, c);
				break;
			}
			if (c >= 060 && c < 0100) {	/* after intermediates */
				vp->v_bad = 1;
				break;
			}
			vp->v_state = S_TEXT;		/* 0200 and up */
			break;

		case S_VT52Y:
			vp->v_par[0] = c - 040;
			vp->v_state = S_VT52X;
			break;

		case S_VT52X:
			vp->v_state = S_TEXT;
			if (vp->v_par[0] <= LASTROW)
				cup(vp, c - 040, vp->v_par[0]);
			break;
		}
	}
}

/*
 * A character of the kernel's own messages.  It goes on the screen
 * plainly, whatever the terminal has been told, and does not disturb an
 * escape sequence a program has half written.
 */
vtkputc(vp, c)
	register struct vt *vp;
	register int c;
{
	switch (c) {
	case '\n':
		vp->v_x = 0;
		lf(vp);
		break;
	case '\r':
	case '\b':
	case '\t':
		control(vp, c);
		break;
	default:
		if (c >= 040 && c != 0177)
			put(vp, NORMAL << 8 | c, 0);
		break;
	}
}
