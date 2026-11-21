/*
 *	C compiler, part 2, for the 80286: longs multiplied, divided and
 *	shifted
 *
 * The 286 multiplies, divides and shifts words.  A long is shifted a
 * byte and then a place at a time, or by a loop if the count is not a
 * constant; the C library's routines multiply and divide one
 * (lib/libc/i286/crt), but where it can be done in line: by a power of
 * two, and two words made long.  This is apart from the rest of the
 * code generator, c10.c, whose assembly is about as much as the
 * system's assembler has room for when it runs on the 286.
 */

#include "c1.h"

/*
 * Multiply or divide DX:AX by r, a long operand, or by CX:BX, with the
 * library's routines (lib/libc/i286/crt), which take them there.
 */
lmulop(op, r)
register union tree *r;
{
	register char *s;

	if (r) {
		/* (the high half first: r may be at [bx]) */
		lhalf(r, CX, HI);
		lhalf(r, BX, 0);
	}
	switch (op) {

	case LTIMES:
		s = "__I4M";
		break;

	case ULTIMES:
		s = "__U4M";
		break;

	case LDIV:
	case LMOD:
		s = "__I4D";
		break;

	default:
		s = "__U4D";
	}
	callh(s);
	if (op==LMOD || op==ULMOD)
		printf("\tmov\tax,bx\n\tmov\tdx,cx\n");
}

/*
 * What of a long multiplication, division or remainder needs no call
 * of the library's.  By a power of two, a product and an unsigned
 * quotient are shifts, a signed quotient is a shift of what is made up
 * first by one less than the divisor if it is negative, and an
 * unsigned remainder is a mask.  The product of two words made long is
 * what one multiplication of the 286's gives, IMUL if they are signed
 * and MUL if not.  Returns 0 for anything else.
 */
lmulcon(tree)
union tree *tree;
{
	register union tree *l, *r, *rr;
	int op, i, u, lbl;
	LTYPE v;

	op = tree->t.op;
	l = tree->t.tr1;
	r = tree->t.tr2;
	i = 0;
	if (r->t.op==LCON) {
		v = r->l.lvalue;
		i = 1;
	} else if (r->t.op==ITOL && r->t.tr1->t.op==CON) {
		v = uns(r->t.tr1)? (LTYPE)UNS(r->t.tr1->c.value):
		    (LTYPE)W(r->t.tr1->c.value);
		i = 1;
	}
	if (i && v > 1 && (v&(v-1))==0 && op!=LMOD) {
		for (i = 0; ((LTYPE)1<<i) != v; i++)
			;
		rcexpr(l, FORREG);
		switch (op) {

		case LTIMES:
		case ULTIMES:
			lshiftc(LSHIFT, i);
			break;

		case ULDIV:
			lshiftc(ULSH, i);
			break;

		case LDIV:
			lbl = isn++;
			printf("\ttest\tdx,dx\n\tjns\tL%d\n", lbl);
			printf("\tadd\tax,%u\n\tadc\tdx,%u\n", UNS(v-1),
			    UNS((v-1)>>16));
			label(lbl);
			lshiftc(RSHIFT, i);
			break;

		case ULMOD:
			printf("\tand\tax,%u\n\tand\tdx,%u\n", UNS(v-1),
			    UNS((v-1)>>16));
			break;
		}
		return(1);
	}
	if (op!=LTIMES && op!=ULTIMES || l->t.op!=ITOL
	 || CHR(l->t.tr1->t.type))
		return(0);
	u = uns(l->t.tr1);
	if (r->t.op==ITOL && !CHR(r->t.tr1->t.type) && uns(r->t.tr1)==u)
		rr = r->t.tr1;
	else if (i==1 && (u? v>=0 && v<=65535L: v>=-32768L && v<=32767))
		rr = tconst((int)v, u? UNSIGN: INT);
	else
		return(0);
	if (rr->t.op==CON) {
		rcexpr(l->t.tr1, FORREG);
		printf("\tmov\tcx,%d\n", W(rr->c.value));
		printf("\t%s\tcx\n", u? "mul": "imul");
	} else if (adrble(rr)) {
		rcexpr(l->t.tr1, FORREG);
		ins1(u? "mul": "imul", rr, 0);
	} else {
		rcexpr(rr, FORSP);
		rcexpr(l->t.tr1, FORREG);
		printf("\tpop\tcx\n\t%s\tcx\n", u? "mul": "imul");
	}
	return(1);
}

/*
 * Shift a long one place.
 */
lshift1(op)
{
	if (op==LSHIFT)
		printf("\tshl\tax,1\n\trcl\tdx,1\n");
	else
		printf("\t%s\tdx,1\n\trcr\tax,1\n", opname(op));
}

/*
 * Shift DX:AX n places, n a constant.
 */
lshiftc(op, n)
register n;
{
	if (n >= 16) {
		if (op==LSHIFT)
			printf("\tmov\tdx,ax\n\txor\tax,ax\n");
		else if (op==ULSH)
			printf("\tmov\tax,dx\n\txor\tdx,dx\n");
		else
			printf("\tmov\tax,dx\n\tcwd\n");
		if (n -= 16)
			shiftc(op, op==LSHIFT? DX: AX, n);
		return;
	}
	/*
	 * Eight places are a byte: the bytes are moved, and what is left
	 * is shifted a place at a time, four clocks a place where a loop
	 * takes twelve.
	 */
	if (n >= 8) {
		if (op==LSHIFT)
			printf("\tmov\tdh,dl\n\tmov\tdl,ah\n\tmov\tah,al\n\txor\tal,al\n");
		else {
			printf("\tmov\tal,ah\n\tmov\tah,dl\n\tmov\tdl,dh\n");
			if (op==ULSH)
				printf("\txor\tdh,dh\n");
			else
				printf("\tsar\tdh,7\n");
		}
		n -= 8;
	}
	while (n--)
		lshift1(op);
}

/*
 * Shift the long l by r, leaving it in DX:AX.  If lv is given it is l
 * already made an operand, and is loaded from there once the count is
 * in hand.
 */
lshift(op, l, r, lv)
register union tree *l, *r;
union tree *lv;
{
	register n;
	int lbl, lbl2;

	if (r->t.op==ITOL)
		r = r->t.tr1;
	else if (LNG(r->t.type))
		r = optim(tnode(LTOI, INT, r, TNULL));
	if (r->t.op==CON) {
		if (lv)
			load(lv, AX);
		else
			rcexpr(l, FORREG);
		lshiftc(op, r->c.value & 037);
		return;
	} else {
		if (lv) {
			if (adrble(r))
				insrp("mov", CX, r, 0);
			else {
				if (lv->n.class==FOFFS)
					printf("\tpush\tes\n");
				printf("\tpush\tbx\n");
				rcexpr(r, FORREG);
				printf("\tmov\tcx,ax\n\tpop\tbx\n");
				if (lv->n.class==FOFFS)
					printf("\tpop\tes\n");
			}
			load(lv, AX);
		} else if (adrble(r)) {
			rcexpr(l, FORREG);
			insrp("mov", CX, r, 0);
		} else {
			rcexpr(r, FORSP);
			rcexpr(l, FORREG);
			printf("\tpop\tcx\n");
		}
		printf("\tjcxz\tL%d\n", lbl2 = isn++);
	}
	label(lbl = isn++);
	lshift1(op);
	printf("\tloop\tL%d\n", lbl);
	if (lbl2)
		label(lbl2);
}
