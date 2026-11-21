/*
 * ve: normal mode, and the visual modes.
 *
 * A command is a count or none, a register or none, and a key.  Most
 * keys are motions, which take the cursor somewhere.  An operator (d,
 * c, y, <, >, !) is followed by a motion, and works on what is between
 * the cursor and where the motion goes; or by itself again, for whole
 * lines; or by i or a and what names an object the cursor is in.  The
 * other keys are each a command of their own, many of them a short way
 * to write an operator and a motion, and are done as that.
 */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ve.h"

struct mv {
	struct pos m_to;	/* where it goes */
	int	m_lines;	/* it is of whole lines */
	int	m_incl;		/* the character it goes to is part of it */
	int	m_jump;		/* far enough to leave a mark behind */
	int	m_vert;		/* up or down: the column wanted stays */
};

static int	oppend;		/* an operator is waiting for this motion */
static int	sdir = 1;	/* which way the last search went */
static int	fkey, fchar;	/* the last f, F, t or T, and what for */
static char	pline[256];	/* a line typed on the last row */
static long	wl;		/* the word motions: the line in lbuf2, */
static int	wn;		/*   and its length */

#define	BLANK(c)	((c) == ' ' || (c) == '\t')

int
key(void)
{
	register int k;

	while ((k = getkey()) == K_NONE)
		if (resized)
			s_refresh();
	return (k);
}

/*
 * The first byte of line n that is not a blank.
 */
int
firstnb(long n)
{
	register int i = 0;

	(void)t_get(&buf, n, lbuf2);
	while (BLANK(lbuf2[i]))
		i++;
	return (i);
}

void
setpcmark(void)
{
	marks[NMARK - 1] = cur;
}

/*
 * The register a character names, or -1.
 */
int
regof(int c)
{
	if (c == 0 || c == '"')
		return (0);
	if (c >= 'a' && c <= 'z')
		return (c - 'a' + 1);
	if (c >= 'A' && c <= 'Z')
		return (c - 'A' + 1);
	return (-1);
}

static int
before(struct pos *a, struct pos *b)
{
	return (a->p_line < b->p_line ||
	    a->p_line == b->p_line && a->p_col < b->p_col);
}

/*
 * What kind of character for the word motions: 0 a blank, 1 one of a
 * word, 2 any other; with big there are only blanks and the rest.
 */
static int
cls(int c, int big)
{
	c &= 0377;
	if (c == ' ' || c == '\t' || c == 0)
		return (0);
	if (big || c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' ||
	    c >= '0' && c <= '9' || c == '_' || c >= 0200)
		return (1);
	return (2);
}

static void
wload(long n)
{
	wl = n;
	wn = t_get(&buf, n, lbuf2);
}

/*
 * To the start of the next word, count times.  An empty line is a
 * word.  After the last word of the file it is the end of the file.
 */
static void
fword(struct pos *p, long count, int big)
{
	register int c, cl;

	wload(p->p_line);
	c = p->p_col;
	while (count-- > 0) {
		cl = c < wn ? cls(lbuf2[c], big) : 0;
		if (cl)
			while (c < wn && cls(lbuf2[c], big) == cl)
				c++;
		for (;;) {
			while (c < wn && cls(lbuf2[c], big) == 0)
				c++;
			if (c < wn)
				break;
			if (wl == buf.t_lines)
				goto out;
			wload(wl + 1);
			c = 0;
			if (wn == 0)
				break;
		}
	}
out:
	p->p_line = wl;
	p->p_col = c;
}

/*
 * Back to the start of a word.
 */
static void
bword(struct pos *p, long count, int big)
{
	register int c, cl;

	wload(p->p_line);
	c = p->p_col;
	while (count-- > 0) {
		for (;;) {
			if (c == 0) {
				if (wl == 1)
					goto out;
				wload(wl - 1);
				c = wn;
				if (wn == 0)
					break;
				continue;
			}
			c--;
			if (cls(lbuf2[c], big))
				break;
		}
		if (wn == 0)
			continue;
		cl = cls(lbuf2[c], big);
		while (c > 0 && cls(lbuf2[c - 1], big) == cl)
			c--;
	}
out:
	p->p_line = wl;
	p->p_col = c;
}

/*
 * On to the end of a word.
 */
static void
eword(struct pos *p, long count, int big)
{
	register int c, cl;

	wload(p->p_line);
	c = p->p_col;
	while (count-- > 0) {
		c++;
		for (;;) {
			while (c < wn && cls(lbuf2[c], big) == 0)
				c++;
			if (c < wn)
				break;
			if (wl == buf.t_lines) {
				c = wn ? wn - 1 : 0;
				goto out;
			}
			wload(wl + 1);
			c = 0;
		}
		cl = cls(lbuf2[c], big);
		while (c + 1 < wn && cls(lbuf2[c + 1], big) == cl)
			c++;
	}
out:
	p->p_line = wl;
	p->p_col = c;
}

/*
 * From p go the way of dir, a level deeper at each up and back at each
 * down, to the down that ends the level p is on; depth is how deep p
 * is to start with.  That is how a bracket's partner is found.
 */
static int
pairscan(struct pos *p, int up, int down, int dir, int depth)
{
	register int i, c, len;
	long n = p->p_line;

	len = t_get(&buf, n, lbuf2);
	i = p->p_col;
	for (;;) {
		while (i < 0 || i >= len) {
			n += dir;
			if (n < 1 || n > buf.t_lines || intr)
				return (-1);
			len = t_get(&buf, n, lbuf2);
			i = dir > 0 ? 0 : len - 1;
		}
		c = lbuf2[i] & 0377;
		if (c == up)
			depth++;
		else if (c == down && --depth == 0) {
			p->p_line = n;
			p->p_col = i;
			return (1);
		}
		i += dir;
	}
}

/*
 * The brackets of the kind given that the cursor is in, or on.
 */
static int
enclosing(int open, int close, struct pos *a, struct pos *b)
{
	*a = cur;
	(void)t_get(&buf, cur.p_line, lbuf2);
	if (pairscan(a, close, open, -1,
	    (lbuf2[cur.p_col] & 0377) == close ? 0 : 1) < 0)
		return (-1);
	*b = *a;
	return (pairscan(b, open, close, 1, 0));
}

/*
 * f, F, t and T: to the count'th c in the line, which is in lbuf, or
 * to just before it.
 */
static int
findch(int k, int c, long count, struct pos *p, int len, int again)
{
	register int i = p->p_col, d = k == 'f' || k == 't' ? 1 : -1;

	if (again && (k == 't' || k == 'T'))
		i += d;
	while (count-- > 0)
		do {
			i += d;
			if (i < 0 || i >= len)
				return (-1);
		} while ((lbuf[i] & 0377) != c);
	if (k == 't')
		i--;
	else if (k == 'T')
		i++;
	p->p_col = i;
	return (1);
}

/*
 * The motion of the key k: 1 and m says where to, 0 if k is not a
 * motion, -1 if it is one and cannot be done, -2 if it was given up.
 */
static int
motion(int k, long count, int hascount, register struct mv *m)
{
	register int c, len;
	long n, last;
	struct pos p;
	int tr = s_textrows();

	m->m_lines = m->m_incl = m->m_jump = m->m_vert = 0;
	p = cur;
	len = t_get(&buf, cur.p_line, lbuf);
	switch (k) {
	case 'h':
	case K_LEFT:
	case CTL('H'):
	case 0177:
		if (p.p_col == 0)
			return (-1);
		p.p_col = count > p.p_col ? 0 : p.p_col - (int)count;
		break;
	case 'l':
	case K_RIGHT:
	case ' ':
		c = oppend ? len : len - 1;
		if (p.p_col >= c)
			return (-1);
		p.p_col = count > c - p.p_col ? c : p.p_col + (int)count;
		break;
	case '0':
	case K_HOME:
		p.p_col = 0;
		break;
	case '^':
		p.p_col = firstnb(p.p_line);
		break;
	case '|':
		p.p_col = colofdisp(lbuf, len, count > BIGCOL ? BIGCOL :
		    (int)count - 1);
		break;
	case '$':
	case K_END:
		if (count - 1 > buf.t_lines - p.p_line)
			return (-1);
		p.p_line += count - 1;
		len = t_len(&buf, p.p_line);
		p.p_col = len ? len - 1 : 0;
		m->m_incl = 1;
		break;
	case 'j':
	case K_DOWN:
	case CTL('N'):
	case CTL('J'):
	case '+':
	case '\r':
	case '_':
		if (k == '_')
			count--;
		else if (p.p_line >= buf.t_lines)
			return (-1);
		p.p_line = count > buf.t_lines - p.p_line ? buf.t_lines :
		    p.p_line + count;
		goto line;
	case 'k':
	case K_UP:
	case CTL('P'):
	case '-':
		if (p.p_line <= 1)
			return (-1);
		p.p_line = count >= p.p_line ? 1 : p.p_line - count;
	line:
		m->m_lines = 1;
		if (k == '+' || k == '-' || k == '\r' || k == '_')
			p.p_col = firstnb(p.p_line);
		else {
			len = t_get(&buf, p.p_line, lbuf2);
			p.p_col = colofdisp(lbuf2, len, wantcol);
			m->m_vert = 1;
		}
		break;
	case 'g':
		if (key() != 'g')
			return (-1);
		if (!hascount)
			count = 1;
		/* fall through */
	case 'G':
		p.p_line = !hascount && k == 'G' || count > buf.t_lines ?
		    buf.t_lines : count;
		p.p_col = firstnb(p.p_line);
		m->m_lines = m->m_jump = 1;
		break;
	case 'H':
	case 'M':
	case 'L':
		last = topline + tr - 1;
		if (last > buf.t_lines)
			last = buf.t_lines;
		if (k == 'H')
			n = topline + count - 1;
		else if (k == 'L')
			n = last - (count - 1);
		else
			n = topline + (last - topline) / 2;
		p.p_line = n > last ? last : n < topline ? topline : n;
		p.p_col = firstnb(p.p_line);
		m->m_lines = m->m_jump = 1;
		break;
	case 'w':
	case 'W':
		fword(&p, count, k == 'W');
		if (oppend && p.p_line > cur.p_line &&
		    p.p_col <= firstnb(p.p_line)) {
			/* not into the next line for its first word */
			p.p_line--;
			p.p_col = t_len(&buf, p.p_line);
		} else if (!oppend && p.p_col >= wn && wn)
			p.p_col = wn - 1;
		if (p.p_line == cur.p_line && p.p_col == cur.p_col)
			return (-1);
		break;
	case 'b':
	case 'B':
		bword(&p, count, k == 'B');
		if (p.p_line == cur.p_line && p.p_col == cur.p_col)
			return (-1);
		break;
	case 'e':
	case 'E':
		eword(&p, count, k == 'E');
		if (p.p_line == cur.p_line && p.p_col == cur.p_col)
			return (-1);
		m->m_incl = 1;
		break;
	case 'f':
	case 'F':
	case 't':
	case 'T':
		c = key();
		if (c == CTL('V'))
			c = key();
		if (c == ESC || c > 0377)
			return (-1);
		fkey = k;
		fchar = c;
		if (findch(k, c, count, &p, len, 0) < 0)
			return (-1);
		m->m_incl = k == 'f' || k == 't';
		break;
	case ';':
	case ',':
		if (fkey == 0)
			return (-1);
		c = k == ';' ? fkey : fkey ^ 040;
		if (findch(c, fchar, count, &p, len, 1) < 0)
			return (-1);
		m->m_incl = c == 'f' || c == 't';
		break;
	case '%':
		if (hascount) {
			if (count > 100)
				return (-1);
			p.p_line = (count * buf.t_lines + 99) / 100;
			p.p_col = firstnb(p.p_line);
			m->m_lines = m->m_jump = 1;
			break;
		}
		for (c = p.p_col; c < len; c++)
			if (strchr("()[]{}", lbuf[c]) != 0)
				break;
		if (c >= len)
			return (-1);
		p.p_col = c;
		switch (lbuf[c]) {
		case '(':
			c = pairscan(&p, '(', ')', 1, 0);
			break;
		case ')':
			c = pairscan(&p, ')', '(', -1, 0);
			break;
		case '[':
			c = pairscan(&p, '[', ']', 1, 0);
			break;
		case ']':
			c = pairscan(&p, ']', '[', -1, 0);
			break;
		case '{':
			c = pairscan(&p, '{', '}', 1, 0);
			break;
		default:
			c = pairscan(&p, '}', '{', -1, 0);
			break;
		}
		if (c < 0)
			return (-1);
		m->m_incl = m->m_jump = 1;
		break;
	case '}':
		n = p.p_line;
		while (count-- > 0) {
			while (n < buf.t_lines && t_len(&buf, n) == 0)
				n++;
			while (n < buf.t_lines && t_len(&buf, n) != 0)
				n++;
		}
		if (n == p.p_line && p.p_col >= len - 1)
			return (-1);
		p.p_line = n;
		p.p_col = (len = t_len(&buf, n)) != 0 ? (oppend ? len :
		    len - 1) : 0;
		m->m_jump = 1;
		break;
	case '{':
		n = p.p_line;
		while (count-- > 0) {
			while (n > 1 && t_len(&buf, n) == 0)
				n--;
			while (n > 1 && t_len(&buf, n) != 0)
				n--;
		}
		if (n == p.p_line && p.p_col == 0)
			return (-1);
		p.p_line = n;
		p.p_col = 0;
		m->m_jump = 1;
		break;
	case '/':
	case '?':
		pline[0] = 0;
		if (s_prompt(k, pline, sizeof (pline)) < 0)
			return (-2);
		if (re_set(pline) < 0)
			return (-1);
		sdir = k == '/' ? 1 : -1;
		k = 'n';
		goto look;
	case '*':
	case '#':
		for (c = p.p_col; c < len && cls(lbuf[c], 0) != 1; c++)
			;
		if (c >= len)
			return (-1);
		while (c > 0 && cls(lbuf[c - 1], 0) == 1)
			c--;
		p.p_col = c;
		(void)strcpy(pline, "\\<");
		for (n = 2; c < len && cls(lbuf[c], 0) == 1 &&
		    n < sizeof (pline) - 4; c++)
			pline[(int)n++] = lbuf[c];
		(void)strcpy(pline + (int)n, "\\>");
		if (re_set(pline) < 0)
			return (-1);
		sdir = k == '*' ? 1 : -1;
		k = 'n';
		/* fall through */
	case 'n':
	case 'N':
	look:
		while (count-- > 0)
			if (!search(k == 'n' ? sdir : -sdir, &p))
				return (-1);
		m->m_jump = 1;
		break;
	case '\'':
	case '`':
		c = key();
		if (c >= 'a' && c <= 'z')
			c -= 'a';
		else if (c == '\'' || c == '`')
			c = NMARK - 1;
		else
			return (-1);
		if (marks[c].p_line == 0) {
			emsg("Mark not set");
			return (-1);
		}
		p = marks[c];
		if (p.p_line > buf.t_lines)
			p.p_line = buf.t_lines;
		if (k == '\'') {
			p.p_col = firstnb(p.p_line);
			m->m_lines = 1;
		} else if (p.p_col > (len = t_len(&buf, p.p_line)))
			p.p_col = len;
		m->m_jump = 1;
		break;
	default:
		return (0);
	}
	m->m_to = p;
	return (1);
}

/*
 * The cursor has been moved by the key k.
 */
static void
moved(int k, struct mv *m)
{
	clampcur(0);
	if (k == '$' || k == K_END)
		wantcol = BIGCOL;
	else if (!m->m_vert) {
		(void)t_get(&buf, cur.p_line, lbuf2);
		wantcol = dispcol(lbuf2, cur.p_col);
	}
}

/*
 * Keep what is from a to b in a register: whole lines if lines, and
 * else from the byte a is at to just before the one b is.  It goes in
 * the register that has no name, and in reg too if that is another.
 */
int
yank(int reg, struct pos *a, struct pos *b, int lines)
{
	register struct reg *r = &regs[0];
	register int from, to;
	long n;
	int len;

	t_free(&r->r_text);
	r->r_lines = lines;
	if (lines) {
		if (t_copy(&r->r_text, 1L, &buf, a->p_line,
		    b->p_line - a->p_line + 1) < 0)
			goto nomem;
	} else
		for (n = a->p_line; n <= b->p_line; n++) {
			len = t_get(&buf, n, lbuf);
			from = n == a->p_line && a->p_col < len ? a->p_col :
			    n == a->p_line ? len : 0;
			to = n == b->p_line && b->p_col < len ? b->p_col : len;
			if (to < from)
				to = from;
			if (t_ins(&r->r_text, n - a->p_line + 1, lbuf + from,
			    to - from) < 0)
				goto nomem;
		}
	if (reg > 0) {
		t_free(&regs[reg].r_text);
		regs[reg].r_lines = lines;
		if (t_copy(&regs[reg].r_text, 1L, &r->r_text, 1L,
		    r->r_text.t_lines) < 0) {
			t_free(&regs[reg].r_text);
			goto nomem;
		}
	}
	return (0);
nomem:
	t_free(&r->r_text);
	emsg("%s", t_err());
	return (-1);
}

/*
 * Take out what is from a to b, keeping it in a register if there is
 * memory to.
 */
int
dodelete(int reg, struct pos *a, struct pos *b, int lines)
{
	long n;
	int len, l2, bc, kept;

	/* with no memory to keep it, it goes all the same */
	if ((kept = yank(reg, a, b, lines) == 0) == 0)
		msgbuf[0] = msgattr = 0;
	if (lines) {
		for (n = a->p_line; n <= b->p_line; n++)
			b_del(a->p_line);
		cur.p_line = a->p_line > buf.t_lines ? buf.t_lines : a->p_line;
		cur.p_col = firstnb(cur.p_line);
		if (!kept)
			msg("No room to keep what was taken out");
		return (0);
	}
	len = t_get(&buf, a->p_line, lbuf);
	l2 = t_get(&buf, b->p_line, lbuf2);
	bc = b->p_col > l2 ? l2 : b->p_col;
	if (a->p_col > len)
		a->p_col = len;
	if (a->p_col + l2 - bc > LINEMAX) {
		emsg("Line too long");
		return (-1);
	}
	memcpy(lbuf + a->p_col, lbuf2 + bc, l2 - bc);
	if (b_put(a->p_line, lbuf, a->p_col + l2 - bc) < 0)
		return (-1);
	for (n = a->p_line; n < b->p_line; n++)
		b_del(a->p_line + 1);
	cur = *a;
	if (!kept)
		msg("No room to keep what was taken out");
	return (0);
}

/*
 * Put what a register has into the file count times, after the cursor
 * or before it: lines after or before the cursor's line, and anything
 * else in the line.
 */
void
put(int reg, int after, long count)
{
	register struct reg *r = &regs[reg];
	long n = r->r_text.t_lines, at, i, c;
	int len, col, l1, ln;

	if (n == 0) {
		emsg("Nothing in the register");
		return;
	}
	if (r->r_lines) {
		at = cur.p_line + (after ? 1 : 0);
		for (c = 0; c < count; c++)
			for (i = 1; i <= n; i++) {
				len = t_get(&r->r_text, i, lbuf);
				if (b_ins(at + c * n + i - 1, lbuf, len) < 0)
					goto out;
			}
	out:
		cur.p_line = at;
		clampcur(0);
		cur.p_col = firstnb(cur.p_line);
		return;
	}
	len = t_get(&buf, cur.p_line, lbuf);
	col = cur.p_col + (after && len > 0 ? 1 : 0);
	l1 = t_len(&r->r_text, 1L);
	if (n == 1) {
		if (count > LINEMAX || len + l1 * count > LINEMAX) {
			emsg("Line too long");
			return;
		}
		(void)t_get(&r->r_text, 1L, lbuf2);
		nmove(lbuf + col, lbuf + col + l1 * (int)count, len - col);
		for (c = 0; c < count; c++)
			memcpy(lbuf + col + (int)c * l1, lbuf2, l1);
		if (b_put(cur.p_line, lbuf, len + l1 * (int)count) == 0 &&
		    l1 * (int)count > 0)
			cur.p_col = col + l1 * (int)count - 1;
		return;
	}
	/*
	 * More than a line: the first goes on what is before the cursor,
	 * what is after it goes on the last, and the rest come between.
	 */
	ln = t_len(&r->r_text, n);
	if (col + l1 > LINEMAX || ln + len - col > LINEMAX) {
		emsg("Line too long");
		return;
	}
	(void)t_get(&r->r_text, n, lbuf2);
	memcpy(lbuf2 + ln, lbuf + col, len - col);
	if (b_ins(cur.p_line + 1, lbuf2, ln + len - col) < 0)
		return;
	(void)t_get(&r->r_text, 1L, lbuf2);
	memcpy(lbuf + col, lbuf2, l1);
	(void)b_put(cur.p_line, lbuf, col + l1);
	for (i = 2; i < n; i++) {
		len = t_get(&r->r_text, i, lbuf);
		if (b_ins(cur.p_line + i - 1, lbuf, len) < 0)
			break;
	}
	cur.p_col = col;
}

/*
 * Move lines first to last n shifts to the right, or to the left if n
 * is less than 0.
 */
void
shift(long first, long last, int n)
{
	int len, i, w, k;

	for (; first <= last; first++) {
		if ((len = t_get(&buf, first, lbuf)) == 0)
			continue;
		i = indentof(lbuf, &w);
		w += n * o_sw;
		if (w < 0)
			w = 0;
		if (w > LINEMAX - len) {
			emsg("Line too long");
			return;
		}
		k = mkindent(lbuf2, w);
		memcpy(lbuf2 + k, lbuf + i, len - i);
		if (b_put(first, lbuf2, k + len - i) < 0)
			return;
	}
}

/*
 * Make count lines, from line n on, into one.
 */
int
join(long n, long count)
{
	int len, l2, i, col = -1;

	while (--count > 0 && n < buf.t_lines) {
		len = t_get(&buf, n, lbuf);
		l2 = t_get(&buf, n + 1, lbuf2);
		for (i = 0; BLANK(lbuf2[i]); i++)
			;
		if (len + 1 + l2 - i > LINEMAX) {
			emsg("Line too long");
			return (-1);
		}
		col = len;
		if (len > 0 && i < l2 && !BLANK(lbuf[len - 1]) &&
		    lbuf2[i] != ')')
			lbuf[len++] = ' ';
		memcpy(lbuf + len, lbuf2 + i, l2 - i);
		if (b_put(n, lbuf, len + l2 - i) < 0)
			return (-1);
		b_del(n + 1);
	}
	if (col < 0)
		return (-1);
	cur.p_line = n;
	cur.p_col = col;
	return (0);
}

/*
 * Do an operator to what is from a to b.
 */
static void
doop(int op, int reg, struct pos *a, struct pos *b, int lines, int amount)
{
	long n = b->p_line - a->p_line + 1;
	int i, w;

	switch (op) {
	case 'y':
		if (yank(reg, a, b, lines) < 0)
			break;
		if (cur.p_line != a->p_line || !lines)
			cur = *a;
		if (n > 2)
			msg("%ld lines yanked", n);
		break;
	case 'd':
		if (dodelete(reg, a, b, lines) == 0 && n > 2)
			msg("%ld fewer lines", lines ? n : n - 1);
		break;
	case 'c':
		if (lines) {
			/* one line is left, with nothing but its indent */
			(void)yank(reg, a, b, 1);
			msgbuf[0] = msgattr = 0;
			(void)t_get(&buf, a->p_line, lbuf);
			i = o_ai ? indentof(lbuf, &w) : 0;
			if (b_put(a->p_line, lbuf, i) < 0)
				break;
			for (n = a->p_line; n < b->p_line; n++)
				b_del(a->p_line + 1);
			cur.p_line = a->p_line;
			cur.p_col = i;
			ai_pending = i > 0;
		} else if (dodelete(reg, a, b, 0) < 0)
			break;
		insert(M_INSERT, 1L);
		break;
	case '<':
	case '>':
		shift(a->p_line, b->p_line, op == '>' ? amount : -amount);
		cur.p_line = a->p_line;
		cur.p_col = firstnb(cur.p_line);
		if (n > 2)
			msg("%ld lines moved %s", n, op == '>' ? "right" : "left");
		break;
	case '!':
		(void)sprintf(pline, "%ld,%ld!", a->p_line, b->p_line);
		if (s_prompt(':', pline, sizeof (pline)) > 0)
			ex(pline);
		break;
	}
}

/*
 * The object of the kind c that the cursor is in, for an operator
 * after i or a: a to b, and whether it is of whole lines.  With a
 * it has what is around it too: the blanks after a word, the brackets
 * or the quotes themselves, the empty lines after a paragraph.
 */
static int
textobj(int kind, int c, struct pos *a, struct pos *b, int *lines)
{
	register int i, s, e;
	int len, cl, open, close, q;
	long n;

	*lines = 0;
	*a = cur;
	*b = cur;
	len = t_get(&buf, cur.p_line, lbuf);
	switch (c) {
	case 'w':
	case 'W':
		if (len == 0)
			return (-1);
		i = cur.p_col;
		cl = cls(lbuf[i], c == 'W');
		for (s = i; s > 0 && cls(lbuf[s - 1], c == 'W') == cl; s--)
			;
		for (e = i + 1; e < len && cls(lbuf[e], c == 'W') == cl; e++)
			;
		if (kind == 'a') {
			i = e;
			if (cl == 0)
				/* blanks, and the word after them */
				for (cl = cls(lbuf[e], c == 'W'); e < len &&
				    cls(lbuf[e], c == 'W') == cl; e++)
					;
			else
				while (e < len && BLANK(lbuf[e]))
					e++;
			if (e == i)
				while (s > 0 && BLANK(lbuf[s - 1]))
					s--;
		}
		a->p_col = s;
		b->p_col = e;
		return (0);
	case '(':
	case ')':
	case 'b':
		open = '(';
		close = ')';
		break;
	case '[':
	case ']':
		open = '[';
		close = ']';
		break;
	case '{':
	case '}':
	case 'B':
		open = '{';
		close = '}';
		break;
	case '<':
	case '>':
		open = '<';
		close = '>';
		break;
	case '"':
	case '\'':
	case '`':
		/* the quotes of the line go in pairs from its start */
		for (i = 0, q = -1; i < len; i++) {
			if (lbuf[i] == '\\' && c != '`')
				i++;
			else if (lbuf[i] != c)
				continue;
			else if (q < 0)
				q = i;
			else if (i >= cur.p_col)
				break;
			else
				q = -1;
		}
		if (q < 0 || i >= len)
			return (-1);
		a->p_col = kind == 'a' ? q : q + 1;
		b->p_col = kind == 'a' ? i + 1 : i;
		return (0);
	case 'p':
		/* lines that are all empty or all not, like the cursor's */
		cl = len == 0;
		for (n = cur.p_line; n > 1 && (t_len(&buf, n - 1) == 0) == cl;
		    n--)
			;
		a->p_line = n;
		for (n = cur.p_line; n < buf.t_lines &&
		    (t_len(&buf, n + 1) == 0) == cl; n++)
			;
		if (kind == 'a')
			for (; n < buf.t_lines &&
			    (t_len(&buf, n + 1) == 0) != cl; n++)
				;
		b->p_line = n;
		*lines = 1;
		return (0);
	default:
		return (-1);
	}
	if (enclosing(open, close, a, b) < 0)
		return (-1);
	if (kind == 'a') {
		b->p_col++;
		return (0);
	}
	len = t_len(&buf, a->p_line);
	if (a->p_col + 1 >= len && b->p_line > a->p_line + 1 &&
	    b->p_col <= firstnb(b->p_line)) {
		/* brackets on lines of their own: the lines between */
		a->p_line++;
		b->p_line--;
		*lines = 1;
	} else
		a->p_col++;
	return (0);
}

/*
 * An operator: read what it is to work on, and do it.
 */
static void
operator(int op, long count, int hascount, int reg)
{
	struct mv m;
	struct pos a, b;
	register int k, r;
	int lines = 0, len, cl;
	long c2 = 0;

	k = key();
	if (k >= '1' && k <= '9') {
		do {
			c2 = c2 * 10 + k - '0';
			k = key();
		} while (k >= '0' && k <= '9' && c2 < 100000L);
		count = hascount ? count * c2 : c2;
		hascount = 1;
	}
	if (!hascount)
		count = 1;
	a = cur;
	b = cur;
	if (k == op) {
		b.p_line = count - 1 > buf.t_lines - cur.p_line ? buf.t_lines :
		    cur.p_line + count - 1;
		lines = 1;
	} else if (k == 'i' || k == 'a') {
		if (textobj(k, key(), &a, &b, &lines) < 0) {
			t_bell();
			return;
		}
	} else {
		len = t_get(&buf, cur.p_line, lbuf);
		if (op == 'c' && (k == 'w' || k == 'W') && cur.p_col < len &&
		    !BLANK(lbuf[cur.p_col])) {
			/* cw changes to the end of the word, as ce does */
			cl = cls(lbuf[cur.p_col], k == 'W');
			for (r = cur.p_col; r + 1 < len &&
			    cls(lbuf[r + 1], k == 'W') == cl; r++)
				;
			b.p_col = r;
			if (count > 1)
				eword(&b, count - 1, k == 'W');
			m.m_to = b;
			m.m_lines = 0;
			m.m_incl = 1;
		} else {
			oppend = 1;
			r = motion(k, count, hascount, &m);
			oppend = 0;
			if (r <= 0) {
				if (msgbuf[0] == 0 && k != ESC && r > -2)
					t_bell();
				return;
			}
		}
		if (before(&m.m_to, &cur)) {
			a = m.m_to;
			b = cur;
		} else
			b = m.m_to;
		lines = m.m_lines;
		if (m.m_incl && !lines) {
			if (b.p_col < t_len(&buf, b.p_line))
				b.p_col++;
		} else if (!lines && b.p_col == 0 && b.p_line > a.p_line) {
			/*
			 * To the start of a line: not that line.  And if
			 * from the start of one, it is whole lines.
			 */
			b.p_line--;
			if (a.p_col <= firstnb(a.p_line))
				lines = 1;
			else
				b.p_col = t_len(&buf, b.p_line);
		}
	}
	doop(op, reg, &a, &b, lines, 1);
}

/*
 * The visual modes: the cursor is moved, and an operator then works on
 * what is between where it started and where it is.
 */
static void
visual(int lines)
{
	struct mv m;
	struct pos a, b;
	register int k, c;
	long count, n;
	int hascount, reg = 0, isl, from, to, len, r;

	vstart = cur;
	mode = lines ? M_VLINE : M_VISUAL;
	for (;;) {
		s_from(1L);
		if (!stuffed())
			s_refresh();
		k = key();
		msgbuf[0] = msgattr = 0;
		count = 0;
		hascount = 0;
		for (;;) {
			if (k == '"') {
				if ((reg = regof(key())) < 0)
					goto out;
			} else if (k >= '1' && k <= '9' || k == '0' && hascount) {
				if (count < 100000L)
					count = count * 10 + k - '0';
				hascount = 1;
			} else
				break;
			k = key();
		}
		if (!hascount)
			count = 1;
		if (before(&cur, &vstart)) {
			a = cur;
			b = vstart;
		} else {
			a = vstart;
			b = cur;
		}
		isl = mode == M_VLINE;
		switch (k) {
		case ESC:
		case CTL('C'):
			goto out;
		case 'v':
		case 'V':
			if (mode == (k == 'v' ? M_VISUAL : M_VLINE))
				goto out;
			mode = k == 'v' ? M_VISUAL : M_VLINE;
			continue;
		case 'o':
			b = cur;
			cur = vstart;
			vstart = b;
			continue;
		case 'X':
		case 'D':
			isl = 1;
			/* fall through */
		case 'x':
		case K_DEL:
			k = 'd';
			goto op;
		case 'Y':
			isl = 1;
			k = 'y';
			goto op;
		case 'C':
		case 'S':
		case 'R':
			isl = 1;
			/* fall through */
		case 's':
			k = 'c';
			/* fall through */
		case 'd':
		case 'y':
		case 'c':
		case '<':
		case '>':
		case '!':
		op:
			mode = M_NORMAL;
			if (!isl && b.p_col < t_len(&buf, b.p_line))
				b.p_col++;
			doop(k, reg, &a, &b, isl, (int)count);
			goto out;
		case 'J':
			mode = M_NORMAL;
			n = b.p_line - a.p_line + 1;
			if (join(a.p_line, n < 2 ? 2L : n) < 0 && msgbuf[0] == 0)
				t_bell();
			goto out;
		case '~':
		case 'u':
		case 'U':
		case 'r':
			c = 0;
			if (k == 'r' && ((c = key()) == ESC || c > 0377))
				goto out;
			for (n = a.p_line; n <= b.p_line; n++) {
				len = t_get(&buf, n, lbuf);
				from = isl || n > a.p_line ? 0 : a.p_col;
				to = isl || n < b.p_line ? len : b.p_col + 1;
				for (; from < to && from < len; from++) {
					r = lbuf[from];
					if (k == 'r')
						r = c;
					else if (r >= 'a' && r <= 'z' && k != 'u')
						r -= 040;
					else if (r >= 'A' && r <= 'Z' && k != 'U')
						r += 040;
					lbuf[from] = r;
				}
				if (b_put(n, lbuf, len) < 0)
					break;
			}
			cur = a;
			goto out;
		case ':':
			mode = M_NORMAL;
			(void)sprintf(pline, "%ld,%ld", a.p_line, b.p_line);
			if (s_prompt(':', pline, sizeof (pline)) > 0)
				ex(pline);
			changed = 0;
			goto out;
		case 'i':
		case 'a':
			if (textobj(k, key(), &a, &b, &r) < 0) {
				t_bell();
				continue;
			}
			vstart = a;
			cur = b;
			if (r)
				mode = M_VLINE;
			else if (cur.p_col > 0)
				cur.p_col--;
			continue;
		}
		r = motion(k, count, hascount, &m);
		if (r <= 0) {
			if (msgbuf[0] == 0 && r > -2)
				t_bell();
			continue;
		}
		cur = m.m_to;
		moved(k, &m);
	}
out:
	mode = M_NORMAL;
	s_from(1L);
}

/*
 * A command of normal mode, of which k is the first key.
 */
void
normal(int k)
{
	struct mv m;
	register int c, i;
	long count = 0, n;
	int hascount = 0, reg = 0, len, tr = s_textrows();
	static short keys[REDOMAX + 8];

	for (;;) {
		if (k == '"') {
			if ((reg = regof(key())) < 0) {
				t_bell();
				return;
			}
		} else if (k >= '1' && k <= '9' || k == '0' && hascount) {
			if (count < 100000L)
				count = count * 10 + k - '0';
			hascount = 1;
		} else
			break;
		k = key();
	}
	n = hascount ? count : 1;
	m.m_vert = 0;
	switch (k) {
	case ESC:
	case CTL('C'):
		return;
	case 'd':
	case 'c':
	case 'y':
	case '<':
	case '>':
	case '!':
		operator(k, count, hascount, reg);
		break;
	case 'x':
	case K_DEL:
		ungetkey('l');
		operator('d', count, hascount, reg);
		break;
	case 'X':
		ungetkey('h');
		operator('d', count, hascount, reg);
		break;
	case 'D':
		ungetkey('$');
		operator('d', count, hascount, reg);
		break;
	case 'C':
		ungetkey('$');
		operator('c', count, hascount, reg);
		break;
	case 's':
		if (t_len(&buf, cur.p_line) == 0) {
			insert(M_INSERT, 1L);
			break;
		}
		ungetkey('l');
		operator('c', count, hascount, reg);
		break;
	case 'S':
		ungetkey('c');
		operator('c', count, hascount, reg);
		break;
	case 'Y':
		ungetkey('y');
		operator('y', count, hascount, reg);
		break;
	case 'p':
	case 'P':
		put(reg, k == 'p', n);
		break;
	case 'r':
		c = key();
		if (c == CTL('V'))
			c = key();
		len = t_get(&buf, cur.p_line, lbuf);
		if (c == ESC || c > 0377 || n > len - cur.p_col) {
			t_bell();
			break;
		}
		if (c == '\r') {
			/* the character becomes the end of a line */
			if (b_ins(cur.p_line + 1, lbuf + cur.p_col + 1,
			    len - cur.p_col - 1) == 0) {
				(void)b_put(cur.p_line, lbuf, cur.p_col);
				cur.p_line++;
				cur.p_col = 0;
			}
			break;
		}
		memset(lbuf + cur.p_col, c, (int)n);
		if (b_put(cur.p_line, lbuf, len) == 0)
			cur.p_col += (int)n - 1;
		break;
	case 'R':
		insert(M_REPLACE, n);
		break;
	case 'J':
		if (join(cur.p_line, n < 2 ? 2L : n) < 0 && msgbuf[0] == 0)
			t_bell();
		break;
	case '~':
		len = t_get(&buf, cur.p_line, lbuf);
		if (len == 0) {
			t_bell();
			break;
		}
		for (i = cur.p_col; n-- > 0 && i < len; i++) {
			c = lbuf[i];
			if (c >= 'a' && c <= 'z')
				lbuf[i] = c - 040;
			else if (c >= 'A' && c <= 'Z')
				lbuf[i] = c + 040;
		}
		if (b_put(cur.p_line, lbuf, len) == 0)
			cur.p_col = i;
		break;
	case '.':
		if (nredo == 0) {
			t_bell();
			break;
		}
		/* with a count, that takes the place of the one it had */
		i = len = 0;
		if (hascount) {
			(void)sprintf(pline, "%ld", count);
			for (; pline[len]; len++)
				keys[len] = pline[len];
			if (redo[0] >= '1' && redo[0] <= '9')
				while (i < nredo && redo[i] >= '0' && redo[i] <= '9')
					i++;
		}
		for (; i < nredo && len < REDOMAX; i++)
			keys[len++] = redo[i];
		stuff(keys, len);
		break;
	case 'u':
		while (n-- > 0 && msgbuf[0] == 0)
			undo();
		break;
	case CTL('R'):
		while (n-- > 0 && msgbuf[0] == 0)
			redo_change();
		break;
	case 'a':
		if (t_len(&buf, cur.p_line) > 0)
			cur.p_col++;
		insert(M_INSERT, n);
		break;
	case 'A':
		cur.p_col = t_len(&buf, cur.p_line);
		insert(M_INSERT, n);
		break;
	case 'I':
		cur.p_col = firstnb(cur.p_line);
		/* fall through */
	case 'i':
	case K_INS:
		insert(M_INSERT, n);
		break;
	case 'o':
	case 'O':
		if (openline(k == 'o') == 0)
			insert(M_INSERT, 1L);
		break;
	case 'v':
	case 'V':
		visual(k == 'V');
		break;
	case ':':
		pline[0] = 0;
		if (hascount)
			(void)sprintf(pline, ".,.+%ld", count - 1);
		if (s_prompt(':', pline, sizeof (pline)) > 0)
			ex(pline);
		changed = 0;
		break;
	case 'Z':
		c = key();
		if (c == 'Z')
			/* nothing has changed in a file not all read yet */
			ex(loading ? "q" : "x");
		else if (c == 'Q')
			ex("q!");
		else
			t_bell();
		break;
	case '&':
		ex("&");
		break;
	case CTL('G'):
		ex("f");
		break;
	case CTL('^'):
		ex("e #");
		break;
	case CTL('L'):
		s_all();
		break;
	case 'm':
		c = key();
		if (c >= 'a' && c <= 'z')
			marks[c - 'a'] = cur;
		else if (c == '\'' || c == '`')
			setpcmark();
		else
			t_bell();
		break;
	case 'z':
		c = key();
		if (hascount && count <= buf.t_lines)
			cur.p_line = count;
		if (c == '\r' || c == 't' || c == '+')
			topline = cur.p_line;
		else if (c == '.' || c == 'z')
			topline = cur.p_line - tr / 2;
		else if (c == '-' || c == 'b')
			topline = cur.p_line - tr + 1;
		else
			t_bell();
		if (topline < 1)
			topline = 1;
		break;
	case CTL('E'):
		topline = n > buf.t_lines - topline ? buf.t_lines : topline + n;
		if (cur.p_line < topline)
			cur.p_line = topline;
		m.m_vert = 1;
		break;
	case CTL('Y'):
		topline = n >= topline ? 1 : topline - n;
		if (cur.p_line > topline + tr - 1)
			cur.p_line = topline + tr - 1;
		m.m_vert = 1;
		break;
	case CTL('F'):
	case K_PGDN:
		if (topline >= buf.t_lines) {
			t_bell();
			break;
		}
		n *= tr > 2 ? tr - 2 : 1;
		topline = n > buf.t_lines - topline ? buf.t_lines : topline + n;
		cur.p_line = topline;
		m.m_vert = 1;
		break;
	case CTL('B'):
	case K_PGUP:
		if (topline <= 1) {
			t_bell();
			break;
		}
		n *= tr > 2 ? tr - 2 : 1;
		topline = n >= topline ? 1 : topline - n;
		cur.p_line = topline + tr - 1;
		m.m_vert = 1;
		break;
	case CTL('D'):
		if (cur.p_line >= buf.t_lines) {
			t_bell();
			break;
		}
		n = hascount ? count : tr / 2;
		if (topline + n <= buf.t_lines)
			topline += n;
		cur.p_line += n;
		m.m_vert = 1;
		break;
	case CTL('U'):
		if (cur.p_line <= 1) {
			t_bell();
			break;
		}
		n = hascount ? count : tr / 2;
		topline = n >= topline ? 1 : topline - n;
		cur.p_line = n >= cur.p_line ? 1 : cur.p_line - n;
		m.m_vert = 1;
		break;
	case CTL('Z'):
		if (!t_cansusp()) {
			emsg("The shell here cannot stop a program");
			break;
		}
		t_cooked();
		(void)kill(0, SIGTSTP);
		t_raw(0);
		resized = 1;
		s_all();
		break;
	default:
		c = motion(k, n, hascount, &m);
		if (c <= 0) {
			if (msgbuf[0] == 0 && c > -2)
				t_bell();
			return;
		}
		if (m.m_jump)
			setpcmark();
		cur = m.m_to;
		break;
	}
	moved(k, &m);
	if (m.m_vert) {
		(void)t_get(&buf, cur.p_line, lbuf2);
		cur.p_col = colofdisp(lbuf2, (int)strlen(lbuf2), wantcol);
		clampcur(0);
	}
}
