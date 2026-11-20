/*
 * C compiler
 */

#if	!defined(lint) && defined(DOSCCS)
static	char	sccsid[] = "@(#)c04.c	2.0 (2.11BSD) 2020/1/7";
#endif
#include "c0.h"

/*
 * Reduce the degree-of-reference by one.
 * e.g. turn "ptr-to-int" into "int".
 */
decref(t)
register int t;
{
	if ((t & ~TYPE) == 0) {
		error("Illegal indirection");
		return(t);
	}
	return(((unsigned)t>>TYLEN) & ~TYPE | t&TYPE);
}

/*
 * Increase the degree of reference by
 * one; e.g. turn "int" to "ptr-to-int".
 */
incref(t)
register int t;
{
#ifdef	i286
	/*
	 * What does not fit is lost, as it is where an int is 16 bits.
	 */
	return((((t&~TYPE)<<TYLEN) | (t&TYPE) | PTR) & 0177777);
#else
	return(((t&~TYPE)<<TYLEN) | (t&TYPE) | PTR);
#endif
}

#ifdef	i286
/*
 * The same, where the pointer is a far one.
 */
fincref(t)
{
	return(incref(t) + (FPTR-PTR));
}

/*
 * The modifier m is to go on a type whose modifiers so far are in
 * type: in the medium model a pointer to a function is a far one.
 */
ptrto(type, m)
{
	if (m==PTR && mflag && (type&XTYPE)==FUNC)
		return(FPTR);
	return(m);
}

/*
 * The type of the address of p.  It is far if p is reached through a
 * far pointer, or is a function in the medium model.
 */
adrtype(p)
register union tree *p;
{
	if (p->t.op==STAR && ISFAR(p->t.tr1->t.type)
	 || mflag && (p->t.type&XTYPE)==FUNC)
		return(fincref(p->t.type));
	return(incref(p->t.type));
}

/*
 * A type as the second pass is given it: a far pointer is an unsigned
 * long there, and so is what a pointer to one points at, and so on.
 */
ptype(t)
register int t;
{
	register unsigned m;
	register int n;

	n = 4;
	for (m = t; m&XTYPE; m >>= TYLEN) {
		if ((m&XTYPE)==FPTR)
			return(t & ((1<<n)-1) & ~TYPE | UNLONG);
		n += TYLEN;
	}
	return(t);
}
#endif

/*
 * Make a tree that causes a branch to lbl
 * if the tree's value is non-zero together with the cond.
 */
cbranch(t, lbl, cond)
union tree *t;
{
	treeout(t, 0);
	outcode("BNNN", CBRANCH, lbl, cond, line);
}

/*
 * Write out a tree.
 */
rcexpr(tp)
register union tree *tp;
{
	/*
	 * Special optimization
	 */
	if (tp->t.op==INIT && tp->t.tr1->t.op==CON) {
		if (tp->t.type==CHAR || tp->t.type==UNCHAR) {
			outcode("B1N0", BDATA, tp->t.tr1->c.value);
			return;
		} else if (tp->t.type==INT || tp->t.type==UNSIGN) {
			outcode("BN", SINIT, tp->t.tr1->c.value);
			return;
		}
	}
	treeout(tp, 0);
	outcode("BN", EXPR, line);
}

#ifdef	i286
#define	PTYPE(t)	ptype(t)
#else
#define	PTYPE(t)	(t)
#endif

treeout(tp, isstruct)
register union tree *tp;
{
	register struct nmlist *hp;
	register nextisstruct;

	if (tp == NULL || tp->t.op==NULLOP) {
		outcode("B", XNULLOP);
		return;
	}
	nextisstruct = tp->t.type==STRUCT;
	switch(tp->t.op) {

	case NAME:
		hp = &tp->t.tr1->n;
		if (hp->hclass==TYPEDEF)
			error("Illegal use of type name");
		outcode("BNN", NAME, hp->hclass==0?STATIC:hp->hclass,
		    PTYPE(tp->t.type));
		if (hp->hclass==EXTERN)
			outcode("S", hp->name);
		else
			outcode("N", hp->hoffset);
		break;

	case LCON:
		outcode("BNNN", tp->l.op, PTYPE(tp->l.type),
		   (unsigned short)(tp->l.lvalue>>16),
		   (unsigned short)tp->l.lvalue);
		break;

	case CON:
		outcode("BNN", tp->c.op, tp->c.type, tp->c.value);
		break;

	case FCON:
		outcode("BNF", tp->f.op, tp->f.type, tp->f.cstr);
		break;

	case STRING:
		outcode("BNNN", NAME, STATIC, tp->t.type, tp->t.tr1);
		break;

	case FSEL:
		treeout(tp->t.tr1, nextisstruct);
		outcode("BNNN", tp->t.op, tp->t.type,
		   tp->t.tr2->fld.bitoffs, tp->t.tr2->fld.flen);
		break;

	case ETYPE:
		error("Illegal use of type");
		break;

	case AMPER:
		treeout(tp->t.tr1, 1);
		outcode("BN", tp->t.op, PTYPE(tp->t.type));
		break;


	case CALL:
		treeout(tp->t.tr1, 1);
		treeout(tp->t.tr2, 0);
		outcode("BN", CALL, PTYPE(tp->t.type));
		break;

	default:
#ifdef	i286
		if (farout(tp, nextisstruct))
			break;
#endif
		treeout(tp->t.tr1, nextisstruct);
		if (opdope[tp->t.op]&BINARY)
			treeout(tp->t.tr2, nextisstruct);
		outcode("BN", tp->t.op, PTYPE(tp->t.type));
		break;
	}
	if (nextisstruct && isstruct==0)
		outcode("BNN", STRASG, STRUCT, tp->t.strp->S.ssize);
}

#ifdef	i286
/*
 * Write the tree if it is something done with a far pointer that the
 * second pass has an operator of its own for: what one points at, and
 * arithmetic, which is on its offset alone.  The value of p++ is that
 * of (p += n) - n.
 */
farout(tp, nextisstruct)
register union tree *tp;
{
	register int op, t;

	if (tp->t.tr1==NULL || !ISFAR(tp->t.tr1->t.type))
		return(0);
	t = PTYPE(tp->t.type);
	switch (op = tp->t.op) {

	case STAR:
		treeout(tp->t.tr1, nextisstruct);
		outcode("BN", FSTAR, t);
		return(1);

	case PLUS:
	case MINUS:
		if (ISFAR(tp->t.tr2->t.type))
			return(0);
		op += FPLUS-PLUS;
		break;

	case ASPLUS:
	case INCBEF:
	case INCAFT:
		op = ASFPLUS;
		break;

	case ASMINUS:
	case DECBEF:
	case DECAFT:
		op = ASFMINUS;
		break;

	default:
		return(0);
	}
	treeout(tp->t.tr1, 0);
	treeout(tp->t.tr2, 0);
	outcode("BN", op, t);
	if (tp->t.op==INCAFT || tp->t.op==DECAFT) {
		treeout(tp->t.tr2, 0);
		outcode("BN", tp->t.op==INCAFT? FMINUS: FPLUS, t);
	}
	return(1);
}
#endif

/*
 * Generate a branch
 */
branch(lab)
{
	outcode("BN", BRANCH, lab);
}

/*
 * Generate a label
 */
label(l)
{
	outcode("BN", LABEL, l);
}

/*
 * ap is a tree node whose type
 * is some kind of pointer; return the size of the object
 * to which the pointer points.
 */
plength(p)
register union tree *p;
{
	register t, l;

	if (p==0 || ((t=p->t.type)&~TYPE) == 0)		/* not a reference */
		return(1);
	if ((t & TYPE) == VOID)
		t = (t & ~TYPE) | CHAR; /* avoid void pointer errors */
	p->t.type = decref(t);
	l = length(p);
	p->t.type = t;
	return(l);
}

/*
 * return the number of bytes in the object
 * whose tree node is acs.
 */
length(cs)
union tree *cs;
{
	register t, elsz;
	long n;
	int nd;

	t = cs->t.type;
	n = 1;
	nd = 0;
	while ((t&XTYPE) == ARRAY) {
		t = decref(t);
		n *= cs->t.subsp[nd++];
	}
	if ((t&~TYPE)==FUNC)
		return(0);
#ifdef	i286
	if ((t&XTYPE)==FPTR)
		elsz = SZFPTR;
	else
#endif
	if ((unsigned)t>=PTR)
		elsz = SZPTR;
	else switch(t&TYPE) {

	case VOID:
		error("Illegal use of void object");
		return(2);

	case INT:
	case UNSIGN:
		elsz = SZINT;
		break;

	case CHAR:
	case UNCHAR:
		elsz = 1;
		break;

	case FLOAT:
		elsz = SZFLOAT;
		break;

	case UNLONG:
	case LONG:
		elsz = SZLONG;
		break;

	case DOUBLE:
		elsz = SZDOUB;
		break;

	case STRUCT:
		if ((elsz = cs->t.strp->S.ssize) == 0)
			error("Undefined structure");
		break;
	default:
		error("Compiler error (length)");
		return(0);
	}
	n *= elsz;
	if (n >= (unsigned)50000)
		werror("very large data structure");
	return(n);
}

/*
 * The number of bytes in an object, rounded up to a word.
 */
rlength(cs)
union tree *cs;
{
	return((length(cs)+ALIGN) & ~ALIGN);
}

/*
 * After an "if (...) goto", look to see if the transfer
 * is to a simple label.
 */
simplegoto()
{
	register struct nmlist *csp;

	if ((peeksym=symbol())==NAME && nextchar()==';') {
		csp = csym;
		if (csp->hblklev == 0)
			csp = pushdecl(csp);
		if (csp->hclass==0 && csp->htype==0) {
			csp->htype = ARRAY;
			csp->hflag |= FLABL;
			if (csp->hoffset==0)
				csp->hoffset = isn++;
		}
		if ((csp->hclass==0||csp->hclass==STATIC)
		 &&  csp->htype==ARRAY) {
			peeksym = -1;
			return(csp->hoffset);
		}
	}
	return(0);
}

/*
 * Return the next non-white-space character
 */
nextchar()
{
	while (spnextchar()==' ')
		peekc = 0;
	return(peekc);
}

/*
 * Return the next character, translating all white space
 * to blank and handling line-ends.
 */
spnextchar()
{
	register c;

	if ((c = peekc)==0)
		c = getchar();
	if (c=='\t' || c=='\014')	/* FF */
		c = ' ';
	else if (c=='\n') {
		c = ' ';
		line++;
	}
	peekc = c;
	return(c);
}

/*
 * is a break or continue legal?
 */
chconbrk(l)
{
	if (l==0)
		error("Break/continue error");
}

/*
 * The goto statement.
 */
dogoto()
{
	register union tree *np;
	register char *st;

	st = starttree();
	*cp++ = tree(0);
	build(STAR);
	chkw(np = *--cp, -1);
	rcexpr(block(JUMP, 0, (int *)NULL, (union str *)NULL, np, TNULL));
	endtree(st);
}

/*
 * The return statement, which has to convert
 * the returned object to the function's type.
 */
doret()
{
	if (nextchar() != ';') {
		register char *st;

		st = starttree();
		*cp++ = (union tree *)&funcblk;
		*cp++ = tree(0);
		build(ASSIGN);
		cp[-1] = cp[-1]->t.tr2;
		build(RFORCE);
		rcexpr(*--cp);
		endtree(st);
	}
	branch(retlab);
}

/*
 * Write a character on the error output.
 */
/*
 * Coded output:
 *   B: beginning of line; an operator
 *   N: a number
 *   S: a symbol (external)
 *   1: number 1
 *   0: number 0
 */
/* VARARGS1 */
outcode(s, a)
char *s;
{
	register *ap;
	register FILE *bufp;
	register char *np;
	int n;

	bufp = stdout;
	if (strflg)
		bufp = sbufp;
	ap = &a;
	for (;;) switch(*s++) {
	case 'B':
		fputc(*ap++, bufp);
		fputc(0376, bufp);
		continue;

	case 'N':
		fputc(*ap, bufp);
		fputc(*ap++>>8, bufp);
		continue;

	case 'F':
		np = (char *)*ap++;
		n = 1000;
		goto str;

	case 'S':
		np = (char *)*ap++;
		n = MAXCPS-1;
		if (*np)
			fputc('_', bufp);
	str:
		while(n-- && *np) {
			fputc(*np++ & 0177, bufp);
		}
		fputc(0, bufp);
		continue;

	case '1':
		fputc(1, bufp);
		fputc(0, bufp);
		continue;

	case '0':
		fputc(0, bufp);
		fputc(0, bufp);
		continue;

	case '\0':
		if (ferror(bufp)) {
			error("Write error on temp");
			exit(1);
		}
		return;

	default:
		error("Botch in outcode");
	}
}

unsigned int
hash(sp)
register char *sp;
{
	register unsigned int h;

	h = 0;
	for (; *sp; sp++) {
		h += h;
		h += *sp;
	}
	return(h%HSHSIZ);
}
