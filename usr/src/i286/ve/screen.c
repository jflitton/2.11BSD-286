/*
 * ve: the screen.  A copy is kept of what the terminal shows, a byte a
 * place for the character and another for whether it stands out, each
 * in a segment of its own so that the window may be of any size.  To
 * show the file a row is made up as it should be and compared with the
 * copy, and only what differs is written to the terminal.  When lines
 * are put in or taken out, or the window moves a little over the file,
 * the terminal is asked to move what it shows, and the rows that then
 * need making are the few that came free.
 *
 * A long line is not folded: the window moves sideways over the file
 * to keep the cursor in sight.  The last row is for messages, the mode,
 * and where the cursor is in the file.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <segment.h>
#include "ve.h"

long	topline = 1;		/* the line on the first row */
int	leftcol;		/* the column at the left edge */
char	msgbuf[COLMAX + 1];	/* what the last row says */
int	msgattr;		/* and whether it stands out */

static char __far *scrt, __far *scra;	/* the copy: characters, standout */
static long	scrsize;
static char	dirty[ROWMAX];	/* rows to be made again */
static long	scrtop = 1;	/* the line the copy has on its first row */
static int	scrleft;
static int	scrnw;		/* how wide the line numbers were */
static int	clearfirst = 1;
static long	pend_at;	/* lines put in or taken out since the */
static long	pend_n;		/*   screen was last made: where, how many */
static int	pend;		/* 0 none, 1 those, 2 more than can be told */
static char	nt[COLMAX + 1], na[COLMAX + 1], ot[COLMAX + 1], oa[COLMAX + 1];

int
s_textrows(void)
{
	return (rows - 1);
}

/*
 * A new screen, or one of another size.
 */
void
s_init(void)
{
	long size = (long)rows * cols;

	if (scrt == 0) {
		scrt = (char __far *)segalloc(size);
		scra = (char __far *)segalloc(size);
		if (scrt == 0 || scra == 0) {
			t_cooked();
			(void)write(2, "ve: no memory for the screen\n", 29);
			exit(1);
		}
	} else if (size > scrsize) {
		if (segsize(scrt, size) < 0 || segsize(scra, size) < 0)
			quit(-1);
	}
	if (size > scrsize)
		scrsize = size;
	s_all();
}

/*
 * Everything is to be made again, on a cleared terminal.
 */
void
s_all(void)
{
	clearfirst = 1;
	pend = 0;
}

/*
 * The rows to be made again are told by their places on the screen as
 * the terminal has it.  While lines put in or taken out are waiting
 * to be done there, a line below them is still on the row it had.
 */
void
s_line(long n)
{
	if (pend == 1 && n >= pend_at) {
		if (pend_n > 0 && n < pend_at + pend_n)
			return;		/* a new line: it will be made */
		n -= pend_n;
	}
	if (n >= scrtop && n < scrtop + s_textrows())
		dirty[(int)(n - scrtop)] = 1;
}

void
s_from(long n)
{
	register int r;

	if (pend == 1) {
		if (pend_at < n)
			n = pend_at;
		pend = 2;
	}
	if (n >= scrtop + s_textrows())
		return;
	for (r = n <= scrtop ? 0 : (int)(n - scrtop); r < s_textrows(); r++)
		dirty[r] = 1;
}

/*
 * n lines were put in before line at, or if n is less than 0 that many
 * taken out from line at on.  One run of either is remembered, to be
 * done on the terminal as it was in the file; when there is more than
 * that, everything from there down is made again.
 */
void
s_shift(long at, long n)
{
	if (at >= scrtop + s_textrows())
		return;
	if (pend == 0 && at >= scrtop) {
		pend = 1;
		pend_at = at;
		pend_n = n;
	} else if (pend == 1 && n > 0 && pend_n > 0 && at >= pend_at &&
	    at <= pend_at + pend_n)
		pend_n += n;
	else if (pend == 1 && n < 0 && pend_n < 0 && at == pend_at)
		pend_n += n;
	else
		s_from(at);
}

/*
 * The column on the screen of byte n of the line s, counting from 0
 * and before the window is moved sideways: a tab goes to the next
 * multiple of the tab stop, and a control character is two wide.
 */
int
dispcol(char *s, int n)
{
	register int c, col = 0;

	while (n-- > 0 && (c = *s++ & 0377) != 0) {
		if (c == '\t' && !o_list)
			col += o_ts - col % o_ts;
		else if (c < ' ' || c == 0177)
			col += 2;
		else
			col++;
	}
	return (col);
}

/*
 * The byte of the line s, which is len long, that is at column col or
 * the last before it.
 */
int
colofdisp(char *s, int len, int col)
{
	register int n, c, w, at = 0;

	for (n = 0; n < len; n++) {
		c = s[n] & 0377;
		if (c == '\t' && !o_list)
			w = o_ts - at % o_ts;
		else if (c < ' ' || c == 0177)
			w = 2;
		else
			w = 1;
		if (at + w > col)
			break;
		at += w;
	}
	return (n);
}

/*
 * How wide the line numbers are, when they are shown.
 */
static int
numwidth(void)
{
	register int w = 3;
	long n;

	if (!o_number)
		return (0);
	for (n = buf.t_lines; n >= 1000; n /= 10)
		w++;
	return (w + 1);
}

/*
 * Is byte col of line n in the visual area?
 */
static int
invisual(long n, int col, struct pos *a, struct pos *b)
{
	if (n < a->p_line || n > b->p_line)
		return (0);
	if (mode == M_VLINE)
		return (1);
	if (n == a->p_line && col < a->p_col)
		return (0);
	if (n == b->p_line && col > b->p_col)
		return (0);
	return (1);
}

/*
 * Make row r as it should be, in nt and na.
 */
static void
mkrow(int r)
{
	register int c, col, w;
	register char *s;
	long n = topline + r;
	int nw, len, i, at, hl, vis;
	struct pos *a, *b;

	memset(nt, ' ', cols);
	memset(na, 0, cols);
	if (n > buf.t_lines) {
		nt[0] = '~';
		return;
	}
	nw = numwidth();
	if (nw) {
		(void)sprintf(ot, "%*ld ", nw - 1, n);
		memcpy(nt, ot, nw < cols ? nw : cols);
	}
	vis = mode == M_VISUAL || mode == M_VLINE;
	if (vis) {
		if (vstart.p_line < cur.p_line || vstart.p_line == cur.p_line &&
		    vstart.p_col <= cur.p_col) {
			a = &vstart;
			b = &cur;
		} else {
			a = &cur;
			b = &vstart;
		}
	}
	len = t_get(&buf, n, lbuf2);
	s = lbuf2;
	col = 0;
	for (i = 0; i <= len; i++) {
		c = i < len ? s[i] & 0377 : o_list ? '$' : ' ';
		hl = vis && invisual(n, i, a, b) && (i < len || len == 0 ||
		    mode == M_VLINE && i == len);
		if (i == len && !o_list && !hl)
			break;
		if (i < len && c == '\t' && !o_list)
			w = o_ts - col % o_ts;
		else if (i < len && (c < ' ' || c == 0177))
			w = 2;
		else
			w = 1;
		for (; w > 0; w--, col++) {
			at = col - leftcol + nw;
			if (at < nw)
				continue;
			if (at >= cols)
				return;
			if (i < len && c == '\t' && !o_list)
				nt[at] = ' ';
			else if (i < len && (c < ' ' || c == 0177))
				nt[at] = w == 2 ? '^' : c == 0177 ? '?' : c + '@';
			else
				nt[at] = c;
			na[at] = hl;
		}
		if (i == len)
			break;
	}
}

/*
 * The last row: the message if there is one, or the mode, and at the
 * right where the cursor is.
 */
static void
mkstatus(void)
{
	char ruler[40];
	register int n, len;
	long last;
	char *m;

	memset(nt, ' ', cols);
	memset(na, 0, cols);
	m = msgbuf;
	if (*m == 0)
		switch (mode) {
		case M_INSERT:
			m = "-- INSERT --";
			break;
		case M_REPLACE:
			m = "-- REPLACE --";
			break;
		case M_VISUAL:
			m = "-- VISUAL --";
			break;
		case M_VLINE:
			m = "-- VISUAL LINE --";
			break;
		}
	len = strlen(m);
	if (len > cols - 1)
		len = cols - 1;
	memcpy(nt, m, len);
	if (m == msgbuf && msgattr)
		memset(na, 1, len);
	if (cols < 40 || len > cols - 20)
		return;
	(void)t_get(&buf, cur.p_line, lbuf2);
	n = dispcol(lbuf2, cur.p_col) + 1;
	last = topline + s_textrows() - 1;
	if (topline <= 1 && last >= buf.t_lines && !loading)
		m = "All";
	else if (topline <= 1)
		m = "Top";
	else if (loading)
		m = "...";		/* how long the file is, is not known */
	else if (last >= buf.t_lines)
		m = "Bot";
	else {
		(void)sprintf(ot, "%d%%", (int)(topline * 100 / buf.t_lines));
		m = ot;
	}
	if (n == cur.p_col + 1)
		(void)sprintf(ruler, "%ld,%d", cur.p_line, n);
	else
		(void)sprintf(ruler, "%ld,%d-%d", cur.p_line, cur.p_col + 1, n);
	for (len = strlen(ruler); len < 13; len++)
		ruler[len] = ' ';
	(void)sprintf(ruler + len, " %s", m);
	len = strlen(ruler);
	memcpy(nt + cols - 19, ruler, len > 18 ? 18 : len);
}

/*
 * Make the terminal's row r what nt and na have: write the places
 * where the copy differs, and clear to the end where that is less to
 * write.  The very last place on the screen is never written, since a
 * terminal may roll its screen up when that is.
 */
static void
putrow(int r)
{
	register int i, a, b;
	int nend, oend, width = cols;
	char __far *pt = scrt + (unsigned)r * (unsigned)cols;
	char __far *pa = scra + (unsigned)r * (unsigned)cols;

	if (r == rows - 1)
		width--;
	fcopy(pt, (char __far *)ot, cols);
	fcopy(pa, (char __far *)oa, cols);
	for (a = 0; a < width && nt[a] == ot[a] && na[a] == oa[a]; a++)
		;
	if (a == width)
		return;
	for (b = width - 1; b > a && nt[b] == ot[b] && na[b] == oa[b]; b--)
		;
	for (nend = width; nend > 0 && nt[nend - 1] == ' ' && !na[nend - 1];
	    nend--)
		;
	for (oend = width; oend > 0 && ot[oend - 1] == ' ' && !oa[oend - 1];
	    oend--)
		;
	if (oend > nend && b >= nend) {
		/* the end of the row is to be blank */
		if (a < nend) {
			t_move(r, a);
			for (i = a; i < nend; i++) {
				t_attr(na[i]);
				t_putc(nt[i]);
			}
			t_move(r, nend);
		} else
			t_move(r, a > nend ? nend : a);
		t_clreol();
	} else {
		t_move(r, a);
		for (i = a; i <= b; i++) {
			t_attr(na[i]);
			t_putc(nt[i]);
		}
	}
	fcopy((char __far *)nt, pt, cols);
	fcopy((char __far *)na, pa, cols);
}

/*
 * Move rows top to bot of the terminal and of the copy up n rows, or
 * down if n is less than 0: 0 if the terminal cannot.
 */
static int
scroll(int top, int bot, int n)
{
	register int r;
	unsigned w = cols;
	int k = n < 0 ? -n : n;

	if (k > bot - top || !t_scroll(top, bot, n))
		return (0);
	memset(nt, ' ', cols);
	memset(na, 0, cols);
	if (n > 0) {
		fmove(scrt + (top + k) * w, scrt + top * w, (bot - top + 1 - k) * w);
		fmove(scra + (top + k) * w, scra + top * w, (bot - top + 1 - k) * w);
		for (r = top; r <= bot; r++) {
			dirty[r] = r + k <= bot ? dirty[r + k] : 1;
			if (r + k > bot) {
				fcopy((char __far *)nt, scrt + r * w, w);
				fcopy((char __far *)na, scra + r * w, w);
			}
		}
	} else {
		fmove(scrt + top * w, scrt + (top + k) * w, (bot - top + 1 - k) * w);
		fmove(scra + top * w, scra + (top + k) * w, (bot - top + 1 - k) * w);
		for (r = bot; r >= top; r--) {
			dirty[r] = r - k >= top ? dirty[r - k] : 1;
			if (r - k < top) {
				fcopy((char __far *)nt, scrt + r * w, w);
				fcopy((char __far *)na, scra + r * w, w);
			}
		}
	}
	return (1);
}

/*
 * Move the window over the file so that the cursor is in it.
 */
void
s_scrollto(void)
{
	register int n, w;
	int tr = s_textrows(), far;

	/* a little way, and the window follows; far, and it is put round */
	far = 0;
	if (cur.p_line < topline) {
		if (topline - cur.p_line >= tr)
			far = 1;
		else
			topline = cur.p_line;
	} else if (cur.p_line >= topline + tr) {
		if (cur.p_line - (topline + tr) >= tr)
			far = 1;
		else
			topline = cur.p_line - tr + 1;
	}
	if (far) {
		topline = cur.p_line - tr / 2;
		if (topline > buf.t_lines - tr + 1)
			topline = buf.t_lines - tr + 1;	/* none of it empty */
	}
	if (topline > buf.t_lines)
		topline = buf.t_lines;
	if (topline < 1)
		topline = 1;
	(void)t_get(&buf, cur.p_line, lbuf2);
	n = dispcol(lbuf2, cur.p_col);
	w = cols - numwidth();
	if (w < 2)
		w = 2;
	if (n < leftcol)
		leftcol = n < w / 2 ? 0 : n - w / 2;
	else if (n >= leftcol + w - 1)
		leftcol = n - w / 2;
}

/*
 * Make the terminal show the file as it is, and put its cursor where
 * the file's is.
 */
void
s_refresh(void)
{
	register int r;
	int tr = s_textrows(), k;
	long d;

	if (resized) {
		resized = 0;
		t_size();
		s_init();
		tr = s_textrows();
	}
	s_scrollto();
	if (clearfirst) {
		clearfirst = 0;
		t_clear();
		memset(nt, ' ', cols);
		memset(na, 0, cols);
		for (r = 0; r < rows; r++) {
			fcopy((char __far *)nt, scrt + (unsigned)r * (unsigned)cols, cols);
			fcopy((char __far *)na, scra + (unsigned)r * (unsigned)cols, cols);
			dirty[r] = 1;
		}
		scrtop = topline;
		scrleft = leftcol;
		pend = 0;
	}
	/* lines put in or taken out */
	if (pend == 1) {
		pend = 0;
		d = pend_at - scrtop;
		k = pend_n > tr ? tr : pend_n < -tr ? -tr : (int)pend_n;
		if (!scroll((int)d, tr - 1, -k))
			s_from(pend_at);
		else
			for (r = (int)d; r < tr && r < d + k; r++)
				dirty[r] = 1;
	}
	pend = 0;
	if (numwidth() != scrnw) {
		scrnw = numwidth();
		s_from(scrtop);
	}
	if (leftcol != scrleft) {
		scrleft = leftcol;
		for (r = 0; r < tr; r++)
			dirty[r] = 1;
	}
	/* the window has moved over the file */
	if ((d = topline - scrtop) != 0) {
		if (d >= tr || d <= -tr || !scroll(0, tr - 1, (int)d))
			for (r = 0; r < tr; r++)
				dirty[r] = 1;
		scrtop = topline;
	}
	for (r = 0; r < tr; r++)
		if (dirty[r]) {
			dirty[r] = 0;
			mkrow(r);
			putrow(r);
		}
	mkstatus();
	putrow(rows - 1);
	t_attr(0);
	(void)t_get(&buf, cur.p_line, lbuf2);
	t_move((int)(cur.p_line - topline),
	    dispcol(lbuf2, cur.p_col) - leftcol + numwidth());
	t_flush();
}

/*
 * Read a line on the last row, after the character c, into line,
 * which holds max: for a command or something to look for.  What is
 * in line to begin with is shown as typed already.  The length, or -1
 * if it was given up with ESC or by rubbing out the c itself.
 */
#define	NHIST	8
#define	HISTLEN	80
static char	hist[NHIST][HISTLEN];
static int	nhist;

int
s_prompt(int c, char *line, int max)
{
	register int n, k;
	int h = nhist, first, w, i;

	n = strlen(line);
	for (;;) {
		/* show the end of it, if it is longer than the row */
		w = cols - 2;
		first = n > w ? n - w : 0;
		memset(nt, ' ', cols);
		memset(na, 0, cols);
		nt[0] = c;
		for (i = first; i < n; i++)
			nt[1 + i - first] = line[i] < ' ' ? '?' : line[i];
		putrow(rows - 1);
		t_attr(0);
		t_move(rows - 1, 1 + n - first);
		k = getkey();
		switch (k) {
		case K_NONE:
			if (resized)
				s_refresh();
			continue;
		case ESC:
		case CTL('C'):
			return (-1);
		case '\r':
		case '\n':
			line[n] = 0;
			if (n && n < HISTLEN - 1) {
				hist[nhist % NHIST][0] = c;
				(void)strcpy(hist[nhist % NHIST] + 1, line);
				nhist++;
			}
			return (n);
		case CTL('H'):
		case 0177:
			if (n == 0)
				return (-1);
			n--;
			continue;
		case CTL('U'):
			n = 0;
			continue;
		case CTL('W'):
			while (n && line[n - 1] == ' ')
				n--;
			while (n && line[n - 1] != ' ')
				n--;
			continue;
		case K_UP:
		case K_DOWN:
			/* what was typed before, after the same character */
			i = h;
			for (;;) {
				i += k == K_UP ? -1 : 1;
				if (i < 0 || i < nhist - NHIST || i >= nhist)
					break;
				if (hist[i % NHIST][0] == c) {
					h = i;
					(void)strcpy(line, hist[i % NHIST] + 1);
					n = strlen(line);
					break;
				}
			}
			continue;
		case CTL('V'):
			k = getkey();
			break;
		}
		if (k > 0377 || k == 0 || n >= max - 1)
			continue;
		line[n++] = k;
	}
}

/*
 * Something was written over the screen: wait for a key, and then
 * make the screen again.
 */
void
s_wait(void)
{
	t_attr(1);
	t_puts("[Press a key to continue]");
	t_attr(0);
	t_flush();
	while (getkey() == K_NONE)
		;
	s_all();
}
