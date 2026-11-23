/*
 * ve: insert mode, and replace mode.  What is typed goes into the file
 * at the cursor, or with R over what is there, until ESC.  The arrows
 * and their kin move the cursor without leaving the mode, and
 * backspace takes out what is before the cursor whatever it is, the
 * end of the line before included.
 */
#include <string.h>
#include "ve.h"

int	ai_pending;		/* the line has an indent that was put there
				   for the user, who has typed nothing yet */

static char	itext[256];	/* what was typed, to do it count times */
static int	ilen;
static int	isimple;	/* and whether that is all that was done */

#define	BLANK(c)	((c) == ' ' || (c) == '\t')

/*
 * How many bytes of s are its indent, and at *w how wide that is.
 */
int
indentof(char *s, int *w)
{
	register int i, n = 0;

	for (i = 0; BLANK(s[i]); i++)
		n = s[i] == '\t' ? n + o_ts - n % o_ts : n + 1;
	*w = n;
	return (i);
}

/*
 * Make an indent w wide at out, and return its length.
 */
int
mkindent(char *out, int w)
{
	register int n = 0;

	if (!o_et)
		for (; w >= o_ts; w -= o_ts)
			out[n++] = '\t';
	for (; w > 0; w--)
		out[n++] = ' ';
	return (n);
}

/*
 * A new line after the cursor's, or before it, with the same indent if
 * that is wanted, and the cursor on it: for o and O.
 */
int
openline(int after)
{
	int i = 0, w;
	long n = cur.p_line + (after ? 1 : 0);

	(void)t_get(&buf, cur.p_line, lbuf);
	if (o_ai)
		i = indentof(lbuf, &w);
	if (b_ins(n, lbuf, i) < 0)
		return (-1);
	cur.p_line = n;
	cur.p_col = i;
	ai_pending = i > 0;
	return (0);
}

static void
note(int c)
{
	if (isimple && ilen < sizeof (itext))
		itext[ilen++] = c;
	else
		isimple = 0;
}

/*
 * The character c, at the cursor.
 */
static int
typed(int c)
{
	register int len = t_get(&buf, cur.p_line, lbuf);
	register int col = cur.p_col;

	ai_pending = 0;
	if (mode == M_REPLACE && col < len)
		lbuf[col] = c;
	else {
		if (len >= LINEMAX) {
			emsg("Line too long");
			return (-1);
		}
		nmove(lbuf + col, lbuf + col + 1, len - col);
		lbuf[col] = c;
		len++;
	}
	if (b_put(cur.p_line, lbuf, len) < 0)
		return (-1);
	cur.p_col++;
	return (0);
}

/*
 * The line ends at the cursor, and what was after it is a new line,
 * indented as this one if that is wanted.  An indent that was put
 * here and had nothing typed after it is taken away again.
 */
static int
newline(void)
{
	register int len = t_get(&buf, cur.p_line, lbuf);
	register int col = cur.p_col;
	int i = 0, w, from = col;

	if (o_ai) {
		i = indentof(lbuf, &w);
		if (i > col)
			i = col;
		while (from < len && BLANK(lbuf[from]))
			from++;
	}
	memcpy(lbuf2, lbuf, i);
	memcpy(lbuf2 + i, lbuf + from, len - from);
	if (b_ins(cur.p_line + 1, lbuf2, i + len - from) < 0)
		return (-1);
	(void)b_put(cur.p_line, lbuf, ai_pending ? 0 : col);
	cur.p_line++;
	cur.p_col = i;
	ai_pending = i > 0 && from == len;
	return (0);
}

/*
 * Take out the n bytes before the cursor.
 */
static void
rubout(int n)
{
	register int len = t_get(&buf, cur.p_line, lbuf);
	register int col = cur.p_col;

	nmove(lbuf + col, lbuf + col - n, len - col);
	if (b_put(cur.p_line, lbuf, len - n) == 0)
		cur.p_col -= n;
}

/*
 * Make line n and the one after it one line, with nothing between.
 */
static int
glue(long n)
{
	int len = t_get(&buf, n, lbuf);
	int l2 = t_get(&buf, n + 1, lbuf2);

	if (len + l2 > LINEMAX) {
		emsg("Line too long");
		return (-1);
	}
	memcpy(lbuf + len, lbuf2, l2);
	if (b_put(n, lbuf, len + l2) < 0)
		return (-1);
	b_del(n + 1);
	cur.p_line = n;
	cur.p_col = len;
	return (0);
}

void
insert(int m, long count)
{
	register int k, col;
	int len, i, w, n, tr;

	mode = m;
	msgbuf[0] = msgattr = 0;
	ilen = 0;
	isimple = 1;
	for (;;) {
		clampcur(1);
		if (!stuffed())
			s_refresh();
		if ((k = getkey()) == K_NONE)
			continue;
		msgbuf[0] = msgattr = 0;
		if (k == ESC || k == CTL('C'))
			break;
		col = cur.p_col;
		len = t_len(&buf, cur.p_line);
		tr = s_textrows();
		switch (k) {
		case '\r':
		case '\n':
			if (newline() == 0)
				note('\n');
			continue;
		case CTL('H'):
		case 0177:
			if (mode == M_REPLACE && col > 0) {
				cur.p_col--;
				isimple = 0;
			} else if (col > 0) {
				rubout(1);
				if (ilen > 0 && itext[ilen - 1] != '\n')
					ilen--;
				else
					isimple = 0;
			} else if (cur.p_line > 1) {
				(void)glue(cur.p_line - 1);
				if (ilen > 0 && itext[ilen - 1] == '\n')
					ilen--;
				else
					isimple = 0;
			} else
				t_bell();
			ai_pending = 0;
			continue;
		case CTL('W'):
			/* the word before the cursor, and blanks after it */
			(void)t_get(&buf, cur.p_line, lbuf);
			for (n = col; n > 0 && BLANK(lbuf[n - 1]); n--)
				;
			if (n > 0) {
				i = lbuf[n - 1] & 0377;
				w = i >= 'a' && i <= 'z' || i >= 'A' && i <= 'Z' ||
				    i >= '0' && i <= '9' || i == '_' || i >= 0200;
				for (; n > 0 && !BLANK(lbuf[n - 1]); n--) {
					i = lbuf[n - 1] & 0377;
					if ((i >= 'a' && i <= 'z' ||
					    i >= 'A' && i <= 'Z' ||
					    i >= '0' && i <= '9' || i == '_' ||
					    i >= 0200) != w)
						break;
				}
			}
			if (col > n)
				rubout(col - n);
			isimple = ai_pending = 0;
			continue;
		case CTL('U'):
			if (col > 0)
				rubout(col);
			isimple = ai_pending = 0;
			continue;
		case CTL('T'):
		case CTL('D'):
			/* the indent, to the next place a shift goes to */
			(void)t_get(&buf, cur.p_line, lbuf);
			i = indentof(lbuf, &w);
			w = k == CTL('T') ? w + o_sw - w % o_sw :
			    w % o_sw ? w - w % o_sw : w - o_sw;
			if (w < 0)
				w = 0;
			if (w > LINEMAX - len) {
				t_bell();
				continue;
			}
			n = mkindent(lbuf2, w);
			memcpy(lbuf2 + n, lbuf + i, len - i);
			if (b_put(cur.p_line, lbuf2, n + len - i) == 0)
				cur.p_col = col < i ? n : col - i + n;
			isimple = 0;
			continue;
		case K_LEFT:
			if (col > 0)
				cur.p_col--;
			break;
		case K_RIGHT:
			if (col < len)
				cur.p_col++;
			break;
		case K_UP:
			if (cur.p_line > 1)
				cur.p_line--;
			break;
		case K_DOWN:
			if (cur.p_line < buf.t_lines)
				cur.p_line++;
			break;
		case K_HOME:
			cur.p_col = 0;
			break;
		case K_END:
			cur.p_col = len;
			break;
		case K_PGUP:
			cur.p_line = cur.p_line > tr ? cur.p_line - tr : 1;
			break;
		case K_PGDN:
			cur.p_line += tr;
			break;
		case K_INS:
			mode = mode == M_INSERT ? M_REPLACE : M_INSERT;
			break;
		case K_DEL:
			if (col < len) {
				cur.p_col++;
				rubout(1);
			} else if (cur.p_line < buf.t_lines)
				(void)glue(cur.p_line);
			else
				t_bell();
			break;
		case '\t':
			if (!o_et)
				goto type;
			(void)t_get(&buf, cur.p_line, lbuf2);
			n = dispcol(lbuf2, col);
			for (n = o_ts - n % o_ts; n > 0 && typed(' ') == 0; n--)
				note(' ');
			continue;
		case CTL('V'):
			k = key();
			/* fall through */
		default:
		type:
			if (k > 0377 || k == 0 || k == '\n')
				t_bell();
			else if (typed(k) == 0)
				note(k);
			continue;
		}
		/* the cursor was moved, or the like */
		isimple = ai_pending = 0;
	}
	if (isimple && ilen > 0)
		while (--count > 0)
			for (i = 0; i < ilen; i++)
				if (itext[i] == '\n' ? newline() : typed(itext[i]))
					count = 0;
	if (ai_pending) {
		(void)b_put(cur.p_line, "", 0);
		cur.p_col = 0;
		ai_pending = 0;
	}
	if (cur.p_col > 0)
		cur.p_col--;
	mode = M_NORMAL;
}
