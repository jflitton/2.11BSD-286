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
		printf("\tfild\t%s [bx]\n", op==ITOF? "word":
		    op==LTOF? "dword": "qword");
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
			printf("\tmov\tbx,sp\n\tf%s\tqword [bx]\n",
			    fname(op, 1));
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
		printf("\tmov\tbx,sp\n\tfcomp\tqword [bx]\n\tfstsw\tax\n");
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
	printf("\tsub\tsp,8\n\tmov\tbx,sp\n\tfstp\tqword [bx]\n\tfwait\n");
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
		printf("\tpush\tax\n\tmov\tbx,sp\n\tfistp\tword [bx]\n");
		printf("\tfwait\n\tpop\tax\n");
	} else if (tree->t.type==UNLONG) {
		printf("\tsub\tsp,8\n\tmov\tbx,sp\n\tfistp\tqword [bx]\n");
		printf("\tfwait\n\tpop\tax\n\tpop\tdx\n\tpop\tcx\n\tpop\tcx\n");
	} else {
		printf("\tsub\tsp,4\n\tmov\tbx,sp\n\tfistp\tdword [bx]\n");
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
		printf("\tf%s\tqword [L%d]\n", s, lbl);
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
