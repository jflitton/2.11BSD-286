/*
 * ve: patterns, and looking for them in the file.
 *
 * A pattern is what ed and vi have, with some of vim's:
 *
 *	c	the character c, if it is none of these
 *	\c	the character c, if it is none of those below
 *	.	any character
 *	[...]	any of those between the brackets, a-z being a to z, or
 *		with ^ first any that is not
 *	^	at the start of the pattern: the start of the line
 *	$	at the end of the pattern: the end of the line
 *	x*	any number of x, which is a character, a . or a [...]
 *	x\+	one or more of x
 *	x\?	one x or none (\= too)
 *	\< \>	the start and the end of a word
 *	\( \)	around a part of the pattern, to be told of later by its
 *		number, 1 to 9
 *	\s \S	a blank or a tab, or neither
 *	\d	a digit
 *	\w	a letter, a digit or _
 *	\t	a tab
 *
 * It is made into a list of steps, and a line is matched by trying
 * them in turn from each place in it, going back to try less for a
 * step that may take more than one character when what follows fails.
 */
#include <string.h>
#include "ve.h"

#define	NPAT	400
#define	NGRP	10

#define	END	0
#define	CHAR	1		/* and the character */
#define	ANY	2
#define	CLASS	3		/* and 32 bytes, a bit a character */
#define	BOL	4
#define	EOL	5
#define	BOW	6
#define	EOW	7
#define	LPAR	8		/* and its number */
#define	RPAR	9
#define	STAR	10		/* and what there may be any number of */
#define	PLUS	11
#define	OPT	12

int	re_have;		/* there is a pattern */
char	*re_start, *re_end;	/* what the last match was of */
char	*re_gs[NGRP], *re_ge[NGRP];	/* and each \( \) of it */

static unsigned char pat[NPAT];
static char	*bol;

#define	LOW(c)	((c) >= 'A' && (c) <= 'Z' ? (c) + 040 : (c))
#define	ISW(c)	((c) >= 'a' && (c) <= 'z' || (c) >= 'A' && (c) <= 'Z' || \
		(c) >= '0' && (c) <= '9' || (c) == '_' || (c) >= 0200)

static void
setbit(unsigned char *map, int c)
{
	map[(c & 0377) >> 3] |= 1 << (c & 7);
}

static void
range(unsigned char *map, int a, int b)
{
	for (; a <= b; a++)
		setbit(map, a);
}

/*
 * Make s the pattern; or if s is empty the pattern stays the one it
 * was.  -1 if it is not a pattern, and there is a message.
 */
int
re_set(char *s)
{
	static unsigned char np[NPAT];
	register int c, n = 0;
	int last = -1, start, i, neg, ngrp = 1, depth = 0, op;
	int stack[NGRP];
	char *beg = s, *beg2;
	unsigned char *map;

	if (*s == 0) {
		if (!re_have)
			emsg("No pattern to use again");
		return (re_have ? 0 : -1);
	}
	while ((c = *s++ & 0377) != 0) {
		if (n + 36 > NPAT) {
			emsg("Pattern too long");
			return (-1);
		}
		start = n;
		switch (c) {
		case '^':
			if (s - 1 != beg)
				goto lit;
			np[n++] = BOL;
			last = -1;
			continue;
		case '$':
			if (*s)
				goto lit;
			np[n++] = EOL;
			last = -1;
			continue;
		case '.':
			np[n++] = ANY;
			break;
		case '[':
			beg2 = *s == '^' ? s + 1 : s;
			if (*beg2 == 0 || strchr(beg2 + 1, ']') == 0)
				goto lit;
			np[n++] = CLASS;
			map = np + n;
			memset((char *)map, 0, 32);
			n += 32;
			if ((neg = *s == '^') != 0)
				s++;
			do {
				if ((c = *s++ & 0377) == 0) {
					emsg("[ with no ]");
					return (-1);
				}
				if (c == '\\' && (*s == ']' || *s == '\\'))
					c = *s++;
				else if (c == '\\' && *s == 't') {
					c = '\t';
					s++;
				}
				if (*s == '-' && s[1] != ']' && s[1]) {
					range(map, c, s[1] & 0377);
					s += 2;
				} else
					setbit(map, c);
			} while (*s != ']');
			s++;
			if (neg)
				for (i = 0; i < 32; i++)
					map[i] = ~map[i];
			break;
		case '*':
			op = STAR;
		rep:
			if (last < 0) {
				if (op == STAR)
					goto lit;
				emsg("Nothing for \\%c to repeat", c);
				return (-1);
			}
			for (i = n; i > last; i--)
				np[i] = np[i - 1];
			np[last] = op;
			n++;
			last = -1;
			continue;
		case '\\':
			switch (c = *s++ & 0377) {
			case 0:
				emsg("Pattern ends with \\");
				return (-1);
			case '<':
				np[n++] = BOW;
				last = -1;
				continue;
			case '>':
				np[n++] = EOW;
				last = -1;
				continue;
			case '(':
				if (ngrp >= NGRP) {
					emsg("Too many \\(");
					return (-1);
				}
				np[n++] = LPAR;
				np[n++] = stack[depth++] = ngrp++;
				last = -1;
				continue;
			case ')':
				if (depth == 0) {
					emsg("\\) with no \\(");
					return (-1);
				}
				np[n++] = RPAR;
				np[n++] = stack[--depth];
				last = -1;
				continue;
			case '+':
				op = PLUS;
				goto rep;
			case '?':
			case '=':
				op = OPT;
				goto rep;
			case 's':
			case 'S':
			case 'd':
			case 'w':
				np[n++] = CLASS;
				map = np + n;
				memset((char *)map, 0, 32);
				n += 32;
				if (c == 'd' || c == 'w')
					range(map, '0', '9');
				if (c == 'w') {
					range(map, 'a', 'z');
					range(map, 'A', 'Z');
					setbit(map, '_');
				}
				if (c == 's' || c == 'S') {
					setbit(map, ' ');
					setbit(map, '\t');
				}
				if (c == 'S')
					for (i = 0; i < 32; i++)
						map[i] = ~map[i];
				break;
			case 't':
				c = '\t';
				/* fall through */
			default:
				np[n++] = CHAR;
				np[n++] = c;
				break;
			}
			break;
		default:
		lit:
			np[n++] = CHAR;
			np[n++] = c;
			break;
		}
		last = start;
	}
	if (depth) {
		emsg("\\( with no \\)");
		return (-1);
	}
	np[n++] = END;
	memcpy((char *)pat, (char *)np, n);
	re_have = 1;
	return (0);
}

/*
 * Is the character c what the step at p takes?
 */
static int
one(register unsigned char *p, register int c)
{
	register int l;

	switch (*p++) {
	case CHAR:
		return (c == *p || o_ic && LOW(c) == LOW(*p));
	case ANY:
		return (1);
	case CLASS:
		if (p[c >> 3] & 1 << (c & 7))
			return (1);
		if (!o_ic)
			return (0);
		l = LOW(c);
		if (l == c && c >= 'a' && c <= 'z')
			l = c - 040;
		return ((p[l >> 3] & 1 << (l & 7)) != 0);
	}
	return (0);
}

/*
 * Does the pattern from pc on match at sp?
 */
static int
match(register unsigned char *pc, register char *sp)
{
	register int c;
	unsigned char *atom;
	char *start, *save;
	int op;

	for (;;)
		switch (op = *pc++) {
		case END:
			re_end = sp;
			return (1);
		case CHAR:
		case ANY:
		case CLASS:
			if ((c = *sp++ & 0377) == 0 || !one(pc - 1, c))
				return (0);
			pc += op == CHAR ? 1 : op == CLASS ? 32 : 0;
			break;
		case BOL:
			if (sp != bol)
				return (0);
			break;
		case EOL:
			if (*sp)
				return (0);
			break;
		case BOW:
			c = *sp & 0377;
			if (!ISW(c))
				return (0);
			if (sp != bol) {
				c = sp[-1] & 0377;
				if (ISW(c))
					return (0);
			}
			break;
		case EOW:
			c = *sp & 0377;
			if (sp == bol || ISW(c))
				return (0);
			c = sp[-1] & 0377;
			if (!ISW(c))
				return (0);
			break;
		case LPAR:
			c = *pc++;
			save = re_gs[c];
			re_gs[c] = sp;
			if (match(pc, sp))
				return (1);
			re_gs[c] = save;
			return (0);
		case RPAR:
			c = *pc++;
			save = re_ge[c];
			re_ge[c] = sp;
			if (match(pc, sp))
				return (1);
			re_ge[c] = save;
			return (0);
		default:
			/* as many as there are, and then fewer */
			atom = pc;
			pc += *pc == CHAR ? 2 : *pc == CLASS ? 33 : 1;
			start = sp;
			while ((c = *sp & 0377) != 0 && one(atom, c)) {
				sp++;
				if (op == OPT)
					break;
			}
			if (op == PLUS)
				start++;
			for (; sp >= start; sp--)
				if (match(pc, sp))
					return (1);
			return (0);
		}
}

/*
 * Look for the pattern in the line s, at byte from or after it: 1 if
 * it is there, and re_start and re_end tell where.
 */
int
re_match(char *s, int from)
{
	register char *sp;
	register int i;
	int len;

	if (!re_have || from > (len = nscan(s, 0, LINEMAX + 1)))
		return (0);
	bol = s;
	for (sp = s + from; ; sp++) {
		if (pat[0] == CHAR && !o_ic) {
			/* straight to where it could start */
			i = nscan(sp, pat[1], s + len - sp);
			if (sp + i == s + len)
				return (0);
			sp += i;
		}
		for (i = 0; i < NGRP; i++)
			re_gs[i] = re_ge[i] = 0;
		if (match(pat, sp)) {
			re_start = sp;
			return (1);
		}
		if (*sp == 0 || pat[0] == BOL)
			return (0);
	}
}

/*
 * Where the pattern last starts in the line s before byte limit, or -1.
 */
static int
lastmatch(char *s, int limit)
{
	register int from = 0, found = -1;

	while (re_match(s, from) && re_start - s < limit) {
		found = re_start - s;
		from = found + 1;
	}
	return (found);
}

/*
 * Look for the pattern from p on, forward if dir is more than 0 and
 * else back, going round the end of the file if that is wanted.  1 and
 * p is where it was found, or 0 and there is a message.
 */
int
search(int dir, struct pos *p)
{
	long n = p->p_line;
	int at, len, wrapped = 0;

	if (!re_have) {
		emsg("No pattern to use again");
		return (0);
	}
	/*
	 * Each line once, and the one it starts in a second time for
	 * what is before where it started.  A file that is not all
	 * read yet is read as far as the search goes, and to its end
	 * before the search goes round.
	 */
	for (;;) {
		if (intr) {
			emsg("Interrupted");
			return (0);
		}
		len = t_get(&buf, n, lbuf2);
		if (dir > 0) {
			at = n == p->p_line && !wrapped ? p->p_col + 1 : 0;
			at = at <= len && re_match(lbuf2, at) ?
			    (int)(re_start - lbuf2) : -1;
		} else
			at = lastmatch(lbuf2, n == p->p_line && !wrapped ?
			    p->p_col : len + 1);
		if (at >= 0) {
			p->p_line = n;
			p->p_col = at;
			if (wrapped)
				msg(dir > 0 ? "Search went on from the top" :
				    "Search went on from the bottom");
			return (1);
		}
		if (wrapped && n == p->p_line)
			break;
		n += dir > 0 ? 1 : -1;
		if (loading && (n < 1 || n > buf.t_lines) &&
		    loaduntil(n < 1 && o_ws ? ALL : n) < 0) {
			emsg("Interrupted");
			return (0);
		}
		if (n < 1 || n > buf.t_lines) {
			if (!o_ws)
				break;
			n = n < 1 ? buf.t_lines : 1;
			wrapped = 1;
		}
	}
	emsg("Pattern not found");
	return (0);
}
