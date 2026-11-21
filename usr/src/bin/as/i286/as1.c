/*
 * The assembler for the 80286: values, operands, and the jumps whose
 * length is found between the passes.
 */
#include "as.h"

struct	opnd op[3];
int	nop;
unsigned here;			/* where the line began: $ */
struct	sym *exsym;		/* the symbol in the last value read, */
int	nexsym;			/*	if this is 1 */

/*
 * Numbers: decimal, 0x and hexadecimal, or hexadecimal and then h.
 */
long
number()
{
	register char *p;
	register c, base;
	long n;

	base = 10;
	if (*lp == '0' && (lp[1] == 'x' || lp[1] == 'X')) {
		lp += 2;
		base = 16;
	} else {
		for (p = lp; idchar(*p); p++)
			;
		if (p[-1] == 'h' || p[-1] == 'H')
			base = 16;
	}
	n = 0;
	for (;; lp++) {
		c = *lp;
		if (c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'a' && c <= 'f' && base == 16)
			c -= 'a' - 10;
		else if (c >= 'A' && c <= 'F' && base == 16)
			c -= 'A' - 10;
		else
			break;
		n = n * base + c;
	}
	if (*lp == 'h' || *lp == 'H')
		lp++;
	if (idchar(*lp))
		error("that is no number: %s", lp);
	return(n);
}

/*
 * The value of a symbol where the line is.  One not yet defined is
 * taken in the first pass to be from elsewhere, and e_fwd says in both
 * passes that its place is not known, so that both make as much room
 * for it.
 */
symval(e, sp)
register struct expr *e;
register struct sym *sp;
{
	sp->s_flags |= FUSED;
	exsym = sp;
	nexsym++;
	if (sp->s_stmt == 0) {
		if ((sp->s_flags & (FEXTERN|FCOMMON|FGLOBAL)) == 0) {
			if (pass == 2)
				error("%s is not defined", sp->s_name);
			e->e_fwd = 1;
		}
		e->e_sym = sp;
		e->e_sect = SUNDEF;
		return;
	}
	if (sp->s_stmt > stmt)
		e->e_fwd = 1;
	e->e_sect = sp->s_sect;
	e->e_val = sp->s_value;
}

isnum(e)
register struct expr *e;
{
	return(e->e_sect == SABS && !e->e_seg);
}

/*
 * A value that takes no operator but a sign.
 */
primary(e)
register struct expr *e;
{
	char name[NAMESZ], low[NAMESZ];
	register c;

	e->e_val = 0;
	e->e_sym = NULL;
	e->e_sect = SABS;
	e->e_seg = 0;
	e->e_fwd = 0;
	skipb();
	c = *lp;
	if (c == '(') {
		lp++;
		binary(e, 0);
		skipb();
		if (*lp == ')')
			lp++;
		else
			error("no ) where one should be", "");
	} else if (c == '-' || c == '~' || c == '+' || c == '!') {
		lp++;
		primary(e);
		if (c != '+') {
			if (!isnum(e))
				error("an address cannot have its sign changed",
				    "");
			e->e_val = c == '-' ? -e->e_val : c == '~' ?
			    ~e->e_val : !e->e_val;
		}
	} else if (c >= '0' && c <= '9') {
		e->e_val = number();
	} else if (c == '\'' || c == '"') {
		lp++;
		if (*lp && *lp != c)
			e->e_val = *lp++ & 0377;
		if (*lp && *lp != c)
			e->e_val |= (*lp++ & 0377) << 8;
		if (*lp == c)
			lp++;
		else
			error("a character has no quote after it", "");
	} else if (c == '$' && !idchar(lp[1])) {
		lp++;
		e->e_sect = sect;
		e->e_val = here;
	} else if (idstart(c)) {
		getname(name);
		lower(low, name);
		if (strcmp(low, "seg") == 0) {
			primary(e);
			if (e->e_sect == SABS || e->e_seg)
				error("seg of what has no segment", "");
			e->e_seg = 1;
		} else if ((c = segof(name)) != 0) {
			/* a group's name, or a segment's: its selector */
			e->e_sect = c;
			e->e_seg = 1;
		} else
			symval(e, symbol(name));
	} else
		error("a value cannot begin so: %s", lp);
}

/*
 * The operators, by how tightly they bind: || then && then = == != <>
 * < <= > >= then | ^ & then << >> then + - then * / %.  getop says
 * which is at lp, as one character, and how long it is in len.
 */
#define	O_OROR	'o'
#define	O_ANDAND 'a'
#define	O_NE	'!'
#define	O_LE	'l'
#define	O_GE	'g'
#define	O_SHL	'L'
#define	O_SHR	'R'

getop(len)
int *len;
{
	register c, d;

	c = lp[0];
	d = lp[1];
	*len = 2;
	if (c == '|' && d == '|')
		return(O_OROR);
	if (c == '&' && d == '&')
		return(O_ANDAND);
	if (c == '=' && d == '=')
		return('=');
	if (c == '!' && d == '=' || c == '<' && d == '>')
		return(O_NE);
	if (c == '<' && d == '=')
		return(O_LE);
	if (c == '>' && d == '=')
		return(O_GE);
	if (c == '<' && d == '<')
		return(O_SHL);
	if (c == '>' && d == '>')
		return(O_SHR);
	*len = 1;
	switch (c) {
	case '=': case '<': case '>': case '|': case '^': case '&':
	case '+': case '-': case '*': case '/': case '%':
		return(c);
	}
	*len = 0;
	return(0);
}

prec(c)
{
	switch (c) {
	case O_OROR:
		return(1);
	case O_ANDAND:
		return(2);
	case '=':
	case O_NE:
	case '<':
	case O_LE:
	case '>':
	case O_GE:
		return(3);
	case '|':
		return(4);
	case '^':
		return(5);
	case '&':
		return(6);
	case O_SHL:
	case O_SHR:
		return(7);
	case '+':
	case '-':
		return(P_ADD);
	case '*':
	case '/':
	case '%':
		return(9);
	}
	return(0);
}

binary(e, min)
register struct expr *e;
{
	struct expr r;
	register c, p;
	int len;

	primary(e);
	for (;;) {
		skipb();
		c = getop(&len);
		p = prec(c);
		if (p == 0 || p <= min)
			return;
		lp += len;
		binary(&r, p);
		combine(e, c, &r);
	}
}

/*
 * e = e c r.  An address may have a number added to it or taken from
 * it, and one of this file's may be taken from another in the same
 * segment, which leaves a number.
 */
combine(e, c, r)
register struct expr *e, *r;
{
	long v;

	e->e_fwd |= r->e_fwd;
	if (e->e_seg || r->e_seg) {
		error("seg cannot be reckoned with", "");
		return;
	}
	if (c == '+' && e->e_sect == SABS) {
		v = e->e_val;
		e->e_sect = r->e_sect;
		e->e_sym = r->e_sym;
		e->e_val = r->e_val + v;
		return;
	}
	if (c == '-' && r->e_sect != SABS) {
		if (e->e_sect == r->e_sect && e->e_sect != SUNDEF) {
			e->e_sect = SABS;
			e->e_val -= r->e_val;
		} else if (pass == 1 && (e->e_fwd || r->e_fwd)) {
			e->e_sect = SABS;
			e->e_sym = NULL;
		} else
			error("an address cannot be taken from that", "");
		return;
	}
	if (r->e_sect != SABS || e->e_sect != SABS && c != '+' && c != '-') {
		error("an address cannot be reckoned so", "");
		return;
	}
	v = r->e_val;
	switch (c) {

	case '+':
		e->e_val += v;
		break;

	case '-':
		e->e_val -= v;
		break;

	case '*':
		e->e_val *= v;
		break;

	case '/':
	case '%':
		if (v == 0) {
			error("division by zero", "");
			break;
		}
		if (c == '/')
			e->e_val /= v;
		else
			e->e_val %= v;
		break;

	case '|':
		e->e_val |= v;
		break;

	case '^':
		e->e_val ^= v;
		break;

	case '&':
		e->e_val &= v;
		break;

	case O_SHL:
		e->e_val <<= (int)v;
		break;

	case O_SHR:
		e->e_val >>= (int)v;
		break;

	case O_OROR:
		e->e_val = e->e_val || v;
		break;

	case O_ANDAND:
		e->e_val = e->e_val && v;
		break;

	case '=':
		e->e_val = e->e_val == v;
		break;

	case O_NE:
		e->e_val = e->e_val != v;
		break;

	case '<':
		e->e_val = e->e_val < v;
		break;

	case O_LE:
		e->e_val = e->e_val <= v;
		break;

	case '>':
		e->e_val = e->e_val > v;
		break;

	case O_GE:
		e->e_val = e->e_val >= v;
		break;
	}
}

expr(e)
struct expr *e;
{
	nexsym = 0;
	binary(e, 0);
}

/*
 * The registers, and the words that say how big or how far.  What
 * regname returns is the kind of operand times 8 and the register.
 */
char	*reg16[] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di", 0 };
char	*reg8[] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh", 0 };
char	*sreg[] = { "es", "cs", "ss", "ds", 0 };
char	segpfx[] = { 0x26, 0x2E, 0x36, 0x3E };

regname(s)
register char *s;
{
	register char **p;

	if (s[0] == 0 || s[1] == 0)
		return(-1);
	if (s[2] == 0) {
		for (p = reg16; *p; p++)
			if (s[0] == **p && s[1] == (*p)[1])
				return(K_REG*8 + (p - reg16));
		for (p = reg8; *p; p++)
			if (s[0] == **p && s[1] == (*p)[1])
				return(K_REG8*8 + (p - reg8));
		for (p = sreg; *p; p++)
			if (s[0] == **p && s[1] == (*p)[1])
				return(K_SREG*8 + (p - sreg));
		return(-1);
	}
	if (s[0] == 's' && s[1] == 't' && s[2] >= '0' && s[2] <= '7' &&
	    s[3] == 0)
		return(K_ST*8 + s[2] - '0');
	return(-1);
}

sizeword(s)
register char *s;
{
	switch (*s) {

	case 'b':
		if (strcmp(s, "byte") == 0)
			return(1);
		break;

	case 'w':
		if (strcmp(s, "word") == 0)
			return(2);
		break;

	case 'd':
		if (strcmp(s, "dword") == 0)
			return(4);
		break;

	case 'q':
		if (strcmp(s, "qword") == 0)
			return(8);
		break;

	case 't':
		if (strcmp(s, "tword") == 0)
			return(10);
		break;

	case 'f':
		if (strcmp(s, "far") == 0)
			return(16 + D_FAR);
		break;

	case 'n':
		if (strcmp(s, "near") == 0)
			return(16 + D_NEAR);
		break;

	case 's':
		if (strcmp(s, "short") == 0)
			return(16 + D_SHORT);
		break;
	}
	return(0);
}

/*
 * One operand.
 */
operand(o)
register struct opnd *o;
{
	char name[NAMESZ];
	register char *p;
	register r;

	o->o_kind = K_NONE;
	o->o_reg = 0;
	o->o_size = 0;
	o->o_dist = 0;
	o->o_rm = -1;
	o->o_over = 0;
	for (;;) {
		skipb();
		p = lp;
		if (!idstart(*lp))
			break;
		getname(name);
		lower(name, name);
		if ((r = sizeword(name)) != 0) {
			if (r < 16)
				o->o_size = r;
			else
				o->o_dist = r - 16;
			continue;
		}
		if ((r = regname(name)) >= 0) {
			o->o_kind = r >> 3;
			o->o_reg = r & 7;
			return;
		}
		lp = p;
		break;
	}
	if (*lp == '[') {
		memory(o);
		return;
	}
	o->o_kind = K_IMM;
	binary(&o->o_e, 0);
}

/*
 * What is in brackets: a segment and a colon, BX or BP, SI or DI, and
 * a displacement, any of them.
 */
memory(o)
register struct opnd *o;
{
	char name[NAMESZ];
	struct expr t;
	register char *p;
	register r;
	int neg, first, bx, bp, si, di;

	o->o_kind = K_MEM;
	o->o_e.e_val = 0;
	o->o_e.e_sym = NULL;
	o->o_e.e_sect = SABS;
	o->o_e.e_seg = 0;
	o->o_e.e_fwd = 0;
	bx = bp = si = di = 0;
	lp++;
	first = 1;
	for (;;) {
		skipb();
		neg = 0;
		if (*lp == '+')
			lp++;
		else if (*lp == '-') {
			neg = 1;
			lp++;
		} else if (!first)
			break;
		first = 0;
		skipb();
		p = lp;
		if (idstart(*lp)) {
			getname(name);
			lower(name, name);
			if ((r = regname(name)) >= 0) {
				skipb();
				if (r >> 3 == K_SREG && *lp == ':') {
					lp++;
					o->o_over = segpfx[r & 7];
					first = 1;
					continue;
				}
				if (neg)
					r = -1;
				switch (r) {

				case K_REG*8 + 3:
					bx++;
					break;

				case K_REG*8 + 5:
					bp++;
					break;

				case K_REG*8 + 6:
					si++;
					break;

				case K_REG*8 + 7:
					di++;
					break;

				default:
					error("%s cannot be in an address",
					    name);
				}
				continue;
			}
			lp = p;
		}
		binary(&t, P_ADD);
		combine(&o->o_e, neg ? '-' : '+', &t);
	}
	if (*lp == ']')
		lp++;
	else
		error("no ] where one should be: %s", lp);
	if (bx > 1 || bp > 1 || si > 1 || di > 1 || bx && bp || si && di)
		error("no such address", "");
	if (bx)
		o->o_rm = si ? 0 : di ? 1 : 7;
	else if (bp)
		o->o_rm = si ? 2 : di ? 3 : 6;
	else if (si)
		o->o_rm = 4;
	else if (di)
		o->o_rm = 5;
}

/*
 * The operands of an instruction, into op: how many there are.
 */
getops()
{
	nop = 0;
	nexsym = 0;
	if (eol())
		return(0);
	for (;;) {
		if (nop >= 3) {
			error("too many operands", "");
			break;
		}
		operand(&op[nop++]);
		skipb();
		if (*lp != ',')
			break;
		lp++;
	}
	return(nop);
}

/*
 * The jumps.  The first pass makes a list of them, which the second
 * goes through again.
 */
struct	jblk *jblk0, *jblkn;
int	njump;			/* in the last block */
struct	jblk *jblkp;		/* the second pass's place */
int	jnext;

jumpstart()
{
	jblkp = jblk0;
	jnext = 0;
}

struct jump *
newjump()
{
	register struct jblk *bp;
	register struct jump *jp;

	if (jblkn == NULL || njump >= JBLK) {
		bp = (struct jblk *)alloc(sizeof *bp);
		bp->jb_next = NULL;
		if (jblkn)
			jblkn->jb_next = bp;
		else
			jblk0 = bp;
		jblkn = bp;
		njump = 0;
	}
	jp = &jblkn->jb_j[njump++];
	jp->j_loc = loc[sect];
	jp->j_stmt = stmt;
	jp->j_adj = 0;
	jp->j_sym = NULL;
	jp->j_off = 0;
	jp->j_long = 0;
	return(jp);
}

struct jump *
nextjump()
{
	if (jblkp && jnext >= JBLK) {
		jblkp = jblkp->jb_next;
		jnext = 0;
	}
	if (jblkp == NULL || jblkp == jblkn && jnext >= njump)
		fatal("the passes do not agree about a jump", "");
	return(&jblkp->jb_j[jnext++]);
}

/*
 * Between the passes: every jump was taken to be a short one.  Make
 * those that do not reach long ones, which moves what is after them
 * and may put others out of reach, until none is.  A label has in
 * s_index, which is not yet in use, how far it has moved.
 */
relax()
{
	register struct jblk *bp;
	register struct jump *jp;
	register struct sym *sp;
	int i, n, grow, pad, changed;
	long d;

	do {
		changed = 0;
		grow = 0;
		sp = text0;
		for (bp = jblk0; bp; bp = bp->jb_next) {
			n = bp == jblkn ? njump : JBLK;
			for (jp = bp->jb_j, i = 0; i < n; i++, jp++) {
				while (sp && sp->s_stmt <= jp->j_stmt) {
					sp->s_index = grow;
					sp = sp->s_link;
				}
				jp->j_adj = grow;
				if (jp->j_kind == J_ALIGN) {
					pad = jp->j_off;
					grow += (pad - (jp->j_loc + grow) %
					    pad) % pad - (pad - jp->j_loc %
					    pad) % pad;
				} else if (jp->j_long == 1)
					grow += jp->j_kind == J_COND ? 3 : 1;
			}
		}
		for (; sp; sp = sp->s_link)
			sp->s_index = grow;
		for (bp = jblk0; bp; bp = bp->jb_next) {
			n = bp == jblkn ? njump : JBLK;
			for (jp = bp->jb_j, i = 0; i < n; i++, jp++) {
				if (jp->j_kind == J_ALIGN || jp->j_long)
					continue;
				sp = jp->j_sym;
				if (sp && sp->s_stmt && sp->s_sect == STEXT) {
					d = (long)sp->s_value + sp->s_index +
					    jp->j_off - ((long)jp->j_loc +
					    jp->j_adj + 2);
					if (d >= -128 && d <= 127)
						continue;
				}
				jp->j_long = 1;
				changed = 1;
			}
		}
	} while (changed);
	for (sp = text0; sp; sp = sp->s_link) {
		sp->s_value += sp->s_index;
		sp->s_index = -1;
	}
	loc[STEXT] += grow;
}
