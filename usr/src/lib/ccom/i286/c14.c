/*
 *	C compiler, part 2, for the 80286: floating point
 *
 * The 287 does it.  Its registers are a stack, and an expression is
 * compiled onto that: what AX is to the rest of the code generator
 * (c10.c), the top of the 287's stack is here.  An operand that is in
 * memory, a float or a double or an int or a long, is used where it
 * is; any other is compiled onto the stack above the first.  The stack
 * has eight places, and an expression that would need more, which is
 * one bracketed to the right eight deep, is not looked for.
 *
 * A function may use all of the 287's stack and returns its value on
 * it (docs/abi.md), so nothing is left there when one is called: what
 * has been computed is put on the processor's stack until the function
 * has returned.
 *
 * The 287 runs beside the processor, which does not wait for it
 * before going on to an instruction of its own.  So an fwait follows
 * whatever the 287 stores in memory.
 */

#include "c1.h"

#define	NFCON	50

struct fcon {
	double	fval;
	int	flbl;
} fcontab[NFCON];
int	nfcon;

/*
 * Compile the tree, which is of floating type, for the context: for
 * its value (FORREG) that is left on the 287's stack.  Returns what
 * gexpr does.
 */
gfloat(tree, ctx)
register union tree *tree;
{
	register op;

	op = tree->t.op;
	switch (ctx) {

	case FOREFF:
		if (op==ASSIGN)
			fassign(tree, 1);
		else if (opdope[op]&ASSGOP || op==INCBEF || op==DECBEF
		    || op==INCAFT || op==DECAFT)
			fasgop(tree, 1);
		else {
			fexpr(tree);
			printf("\tfstp\tst0\n");
		}
		return(DONE);

	case FORCC:
		fexpr(tree);
		ftest();
		return(CCLOGIC);

	case FORSP:
		fpush(tree);
		return(DONE);
	}
	fexpr(tree);
	return(INREG);
}

/*
 * Compile the tree onto the 287's stack.
 */
fexpr(tree)
register union tree *tree;
{
	register union tree *l, *r;
	register op;
	int lbl, lbl2;

again:
	op = tree->t.op;
	l = tree->t.tr1;
	if (opdope[op]&BINARY)
		r = tree->t.tr2;
	switch (op) {

	case FCON:
	case SFCON:
		if (tree->f.fvalue==0 && !fneg(tree->f.fvalue))
			printf("\tfldz\n");
		else if (tree->f.fvalue==1)
			printf("\tfld1\n");
		else
			fins("ld", tree);
		return;

	case STAR:
	case FSTAR:
	case AUTOI:
	case AUTOD:
		gaddr(tree);
	case NAME:
		fins("ld", tree);
		return;

	/*
	 * An int or a long made floating.  An unsigned int is a long
	 * by now, and an unsigned long is loaded as the low half of
	 * a number of 64 bits.
	 */
	case ITOF:
	case LTOF:
		if (r = fmem(tree)) {
			fins("ld", r);
			return;
		}
	case ULTOF:
		rcexpr(l, FORREG);
		if (op==ULTOF)
			printf("\tpush\t0\n\tpush\t0\n");
		if (op!=ITOF)
			printf("\tpush\tdx\n");
		printf("\tpush\tax\n\tmov\tbx,sp\n");
		printf("\tfild\t%s [%sbx]\n", op==ITOF? "word":
		    op==LTOF? "dword": "qword", sovr);
		popstk(op==ITOF? 2: op==LTOF? 4: 8);
		return;

	case NEG:
		fexpr(l);
		printf("\tfchs\n");
		return;

	case CALL:
		gcall(tree, TNULL, 0);
		return;

	case PLUS:
	case MINUS:
	case TIMES:
	case DIVIDE:
		if (hascall(r)) {
			fexpr(l);
			fsave();
			fexpr(r);
			printf("\tmov\tbx,sp\n\tf%s\tqword [%sbx]\n",
			    fname(op, 1), sovr);
			printf("\tadd\tsp,8\n");
			return;
		}
		fexpr(l);
		if (l = fmem(r))
			fins(fname(op, 0), l);
		else {
			fexpr(r);
			printf("\tf%sp\tst1\n", fname(op, 0));
		}
		return;

	case ASSIGN:
		fassign(tree, 0);
		return;

	case ASPLUS:
	case ASMINUS:
	case ASTIMES:
	case ASDIV:
	case INCBEF:
	case DECBEF:
	case INCAFT:
	case DECAFT:
		fasgop(tree, 0);
		return;

	case QUEST:
		cbranch(l, lbl = isn++, 0);
		fexpr(r->t.tr1);
		branch(lbl2 = isn++, 0, 0);
		label(lbl);
		fexpr(r->t.tr2);
		label(lbl2);
		return;

	case SEQNC:
		rcexpr(l, FOREFF);
		tree = r;
		goto again;
	}
	if (op==MOD || op==ASMOD)
		error("Illegal operation on floating point");
	else
		error("Compiler error: fexpr, op %d", op);
}

/*
 * Is p an operand in memory that the 287 can take: floating, or an
 * int or a long to be made floating?  If so it is returned, made an
 * operand.  Only BX and the processor's registers are disturbed.
 */
union tree *
fmem(p)
register union tree *p;
{
	register union tree *q;

	switch (p->t.op) {

	case FCON:
	case SFCON:
	case NAME:
		return(p);

	case STAR:
	case FSTAR:
	case AUTOI:
	case AUTOD:
		gaddr(p);
		return(p);

	case ITOF:
	case LTOF:
		q = p->t.tr1;
		if (q->t.type!=INT && q->t.type!=LONG)
			break;
		if (q->t.op==NAME && q->n.class!=REG)
			return(q);
		if (IND(q)) {
			gaddr(q);
			return(q);
		}
	}
	return(NULL);
}

/*
 * The 287's instruction for an operator, less its f; if rev, the one
 * that takes its operands the other way round.
 */
char *
fname(op, rev)
{
	switch (op) {

	case PLUS:
	case ASPLUS:
	case INCBEF:
	case INCAFT:
		return("add");

	case MINUS:
	case ASMINUS:
	case DECBEF:
	case DECAFT:
		return(rev? "subr": "sub");

	case TIMES:
	case ASTIMES:
		return("mul");

	case DIVIDE:
	case ASDIV:
		return(rev? "divr": "div");
	}
	error("Illegal operation on floating point");
	return("add");
}

/*
 * l = r.  The value is left on the 287's stack unless pop is set.
 */
fassign(tree, pop)
register union tree *tree;
{
	register union tree *l;

	l = tree->t.tr1;
	flval(l, tree->t.tr2);
	fins(pop? "stp": "st", l);
	printf("\tfwait\n");
}

/*
 * l op= r, and ++ and --.  The value is left on the 287's stack unless
 * pop is set: the old one, if the operator comes after its operand.
 */
fasgop(tree, pop)
register union tree *tree;
{
	register union tree *l;
	register op;

	op = tree->t.op;
	l = tree->t.tr1;
	flval(l, tree->t.tr2);
	if (!pop && (op==INCAFT || op==DECAFT)) {
		/* the old value is under the new */
		fins("ld", l);
		printf("\tfxch\tst1\n");
		printf("\tf%s\tst0,st1\n", fname(op, 1));
		pop++;
	} else
		fins(fname(op, 1), l);
	fins(pop? "stp": "st", l);
	printf("\tfwait\n");
}

/*
 * Compile r onto the 287's stack and make l, which is to be stored
 * in, an operand.  If finding where l is calls a function, that is
 * done first.
 */
flval(l, r)
register union tree *l, *r;
{
	register union tree *q;
	register far;

	if ((l->t.op==STAR || l->t.op==FSTAR) && hascall(l)) {
		far = l->t.op==FSTAR;
		q = bxform(l);
		rcexpr(q, FORSP);
		fexpr(r);
		printf("\tpop\tbx\n");
		if (far)
			printf("\tpop\tes\n");
		return;
	}
	fexpr(r);
	if (l->t.op!=NAME)
		gaddr(l);
}

/*
 * Compare what is on the 287's stack with zero, for the processor's
 * flags, and take it off.
 */
ftest()
{
	printf("\tftst\n\tfstsw\tax\n\tfstp\tst0\n\tsahf\n");
}

/*
 * Compare two floating values for the processor's flags, which are set
 * as by a comparison of unsigned numbers.  Returns the comparison to
 * branch on: the other way round, if the operands were.
 */
fcompare(tree)
register union tree *tree;
{
	register union tree *l, *r;
	register op;

	op = tree->t.op;
	l = tree->t.tr1;
	r = tree->t.tr2;
	if (r->t.op==FCON && r->f.fvalue==0) {
		fexpr(l);
		ftest();
	} else if (hascall(r)) {
		fexpr(l);
		fsave();
		fexpr(r);
		printf("\tmov\tbx,sp\n\tfcomp\tqword [%sbx]\n\tfstsw\tax\n",
		    sovr);
		printf("\tadd\tsp,8\n\tsahf\n");
		op = maprel[op-EQUAL];
	} else {
		fexpr(l);
		if (l = fmem(r))
			fins("comp", l);
		else {
			fexpr(r);
			printf("\tfcompp\n");
			op = maprel[op-EQUAL];
		}
		printf("\tfstsw\tax\n\tsahf\n");
	}
	if (op>=LESSEQ && op<=GREAT)
		op += LESSEQP-LESSEQ;
	return(op);
}

/*
 * Jump to the label if the floating tree is true, or if it is false,
 * as cond says.
 */
fbranch(tree, lbl, cond)
register union tree *tree;
{
	register op;

	if (opdope[tree->t.op]&RELAT)
		op = fcompare(tree);
	else {
		fexpr(tree);
		ftest();
		op = NEQUAL;
	}
	branch(lbl, op, !cond);
}

/*
 * Push the tree, as a double: an argument.  A double in memory is
 * pushed from there.
 */
fpush(tree)
register union tree *tree;
{
	register i;

	if (tree->t.type==DOUBLE && (tree->t.op==NAME || tree->t.op==STAR
	 || tree->t.op==FSTAR)) {
		if (tree->t.op!=NAME)
			gaddr(tree);
		tree->n.offset += 8;
		for (i = 0; i<4; i++) {
			tree->n.offset -= 2;
			ins1("push", tree, 0);
		}
		return;
	}
	fexpr(tree);
	fsave();
}

/*
 * Move what is on the 287's stack to the processor's, as a double.
 */
fsave()
{
	printf("\tsub\tsp,8\n\tmov\tbx,sp\n\tfstp\tqword [%sbx]\n\tfwait\n",
	    sovr);
}

/*
 * A floating value made an int, in AX, or a long, in DX:AX.  C drops
 * the fraction, and the 287 as it is found rounds: the library's
 * __CHP (lib/libc/i286/crt) has it drop the fraction of what is on
 * its stack.  An unsigned long is taken from a number of 64 bits.
 */
ffix(tree)
register union tree *tree;
{
	fexpr(tree->t.tr1);
	callh("__CHP");
	if (tree->t.op==FTOI) {
		printf("\tpush\tax\n\tmov\tbx,sp\n\tfistp\tword [%sbx]\n", sovr);
		printf("\tfwait\n\tpop\tax\n");
	} else if (tree->t.type==UNLONG) {
		printf("\tsub\tsp,8\n\tmov\tbx,sp\n\tfistp\tqword [%sbx]\n",
		    sovr);
		printf("\tfwait\n\tpop\tax\n\tpop\tdx\n\tpop\tcx\n\tpop\tcx\n");
	} else {
		printf("\tsub\tsp,4\n\tmov\tbx,sp\n\tfistp\tdword [%sbx]\n",
		    sovr);
		printf("\tfwait\n\tpop\tax\n\tpop\tdx\n");
	}
}

/*
 * Is a function called in the tree?
 */
hascall(p)
register union tree *p;
{
	register op;

	if (p==NULL)
		return(0);
	op = p->t.op;
	if (op==CALL)
		return(1);
	if (op==0 || opdope[op]&LEAF)
		return(0);
	if (hascall(p->t.tr1))
		return(1);
	return(opdope[op]&BINARY && hascall(p->t.tr2));
}

/*
 * An instruction of the 287's on an operand in memory, which may be an
 * int or a long, or a floating constant.  The name comes without its
 * f.
 */
fins(s, p)
char *s;
register union tree *p;
{
	register lbl;

	if (p->t.op==FCON || p->t.op==SFCON) {
		lbl = flabel(p->f.fvalue);
		printf("\tf%s\tqword [%sL%d]\n", s, ovr, lbl);
		return;
	}
	printf("\tf%s%s\t", p->t.type==FLOAT || p->t.type==DOUBLE? "": "i", s);
	pname(p, FLT);
	putchar('\n');
}

/*
 * The label of a floating constant, which is put in the data the
 * first time it is asked for.
 */
flabel(v)
double v;
{
	register struct fcon *fp;

	for (fp = fcontab; fp < &fcontab[nfcon]; fp++)
		if (fp->fval==v && fneg(fp->fval)==fneg(v))
			return(fp->flbl);
	if (nfcon>=NFCON)
		fp = &fcontab[NFCON-1];
	else
		nfcon++;
	fp->fval = v;
	fp->flbl = isn++;
	sect(DATA);
	printf("\talign\t2,db 0\nL%d:", fp->flbl);
	fdata(DOUBLE, v);
	sect(PROG);
	return(fp->flbl);
}

/*
 * Has v its sign bit set?  Minus zero has, though it is not less than
 * zero.
 */
fneg(v)
double v;
{
	union {
		double	d;
		short	w[4];
	} u;

	u.d = v;
	return(u.w[3]<0);
}

/*
 * A floating constant as data, of the type: as it is in this
 * machine's memory, which is as it is in the 286's.
 */
fdata(type, v)
double v;
{
	union {
		double	d;
		float	f;
		unsigned short w[4];
	} u;

	if (type==FLOAT) {
		u.f = v;
		printf("\tdw\t%u,%u\n", u.w[0], u.w[1]);
	} else {
		u.d = v;
		printf("\tdw\t%u,%u,%u,%u\n", u.w[0], u.w[1], u.w[2], u.w[3]);
	}
}

/*
 * A floating constant's value from its text: the double nearest to
 * what is written, and of two as near, the one whose last bit is 0.
 * It is worked out in integers, so that a constant is the same bits
 * wherever the compiler runs: the library's atof does it in floating
 * point, and on the 286 is one off in the last place for a constant in
 * ten or so, which what compiled the same file elsewhere was not.
 *
 * A number is NW words of 16 bits, the least first.  The constant is
 * num/den, a whole number of its digits over a power of 10, or times
 * one; they are shifted until the quotient has 54 or 55 bits, which
 * are found a bit at a time, and those are rounded to 53.  The
 * smallest numbers, which have fewer bits than that, are 0 here.
 * nw is how many words the longer of the two has in use.
 */
#define	NW	100

static unsigned short num[NW], den[NW];
static int nw;

static
mul10(a, d)
register unsigned short *a;
{
	register int i;
	unsigned long c;

	c = d;
	for (i = 0; i < nw; i++) {
		c += (unsigned long)a[i] * 10;
		a[i] = c & 0177777;
		c >>= 16;
	}
	if (c && nw < NW)
		a[nw++] = c;
}

static
nbits(a)
register unsigned short *a;
{
	register int i, n;
	register unsigned w;

	for (i = nw-1; i >= 0 && a[i]==0; i--)
		;
	if (i < 0)
		return(0);
	n = i*16;
	for (w = a[i]; w; w >>= 1)
		n++;
	return(n);
}

static
shl(a, n)
register unsigned short *a;
{
	register int i, w;
	int b;

	w = n >> 4;
	b = n & 017;
	if ((nw += w+1) > NW)
		nw = NW;
	for (i = nw-1; i >= 0; i--) {
		a[i] = i>=w ? a[i-w] << b : 0;
		if (b && i > w)
			a[i] |= a[i-w-1] >> (16-b);
	}
}

static
shr1(a)
register unsigned short *a;
{
	register int i;

	for (i = 0; i < nw-1; i++)
		a[i] = (a[i]>>1) | ((a[i+1]&1) << 15);
	a[nw-1] >>= 1;
}

/*
 * If a is not less than b, take b from it.
 */
static
sub(a, b)
register unsigned short *a, *b;
{
	register int i;
	long c;

	for (i = nw-1; i >= 0; i--)
		if (a[i] != b[i]) {
			if (a[i] < b[i])
				return(0);
			break;
		}
	c = 0;
	for (i = 0; i < nw; i++) {
		c += (long)a[i] - (long)b[i];
		a[i] = c & 0177777;
		c = c<0 ? -1 : 0;
	}
	return(1);
}

double
ctod(s)
register char *s;
{
	union {
		double	d;
		unsigned short w[4];
	} u;
	unsigned short q[4];
	register int i, c;
	int e, e10, dot, minus, sh, drop, low, half, field;

	for (i = 0; i < NW; i++)
		num[i] = den[i] = 0;
	den[0] = 1;
	nw = 1;
	u.w[0] = u.w[1] = u.w[2] = u.w[3] = 0;
	e10 = 0;
	dot = 0;
	for (; (c = *s) != 0; s++) {
		if (c=='.') {
			dot++;
			continue;
		}
		if (c<'0' || c>'9')
			break;
		mul10(num, c-'0');
		if (dot)
			e10--;
	}
	if (c=='e' || c=='E') {
		minus = 0;
		if (*++s=='+')
			s++;
		else if (*s=='-') {
			minus++;
			s++;
		}
		for (e = 0; *s>='0' && *s<='9'; s++)
			if (e < 1000)
				e = e*10 + *s-'0';
		e10 += minus ? -e : e;
	}
	if (nbits(num)==0 || e10 < -420)
		return(u.d);
	if (e10 > 330) {
		u.w[3] = 077760;
		return(u.d);
	}
	for (; e10 > 0; e10--)
		mul10(num, 0);
	for (; e10 < 0; e10++)
		mul10(den, 0);
	sh = 54 - (nbits(num) - nbits(den));
	if (sh > 0)
		shl(num, sh);
	else
		shl(den, -sh);
	shl(den, 54);
	q[0] = q[1] = q[2] = q[3] = 0;
	for (i = 54; i >= 0; i--) {
		if (sub(num, den))
			q[i>>4] |= 1 << (i&017);
		shr1(den);
	}
	/* 54 bits or 55: 1 or 2 are dropped, and what is left of num. */
	drop = q[3] & 0100 ? 2 : 1;
	low = q[0] & ((1<<drop) - 1);
	half = 1 << (drop-1);
	for (i = 0; i < drop; i++) {
		q[0] = (q[0]>>1) | ((q[1]&1) << 15);
		q[1] = (q[1]>>1) | ((q[2]&1) << 15);
		q[2] = (q[2]>>1) | ((q[3]&1) << 15);
		q[3] >>= 1;
	}
	if (low > half || low==half && (nbits(num) || q[0]&1)) {
		for (i = 0; i < 4 && ++q[i]==0; i++)
			;
		if (q[3] & 040) {
			/* 2^53: its low bits are all 0 */
			q[3] >>= 1;
			drop++;
		}
	}
	field = 1023 + 52 + drop - sh;
	if (field <= 0)
		return(u.d);
	if (field >= 2047) {
		u.w[3] = 077760;
		return(u.d);
	}
	u.w[0] = q[0];
	u.w[1] = q[1];
	u.w[2] = q[2];
	u.w[3] = (q[3] & 017) | (field << 4);
	return(u.d);
}
