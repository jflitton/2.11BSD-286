/*
 *	C compiler, part 2, for the 80286: longs multiplied, divided and
 *	shifted
 *
 * The 286 multiplies, divides and shifts words; for a long it shifts
 * a place at a time, and the C library's routines multiply and divide
 * (lib/libc/i286/crt).  This is apart from the rest of the code
 * generator, c10.c, whose assembly is about as much as the system's
 * assembler has room for when it runs on the 286.
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
		n = r->c.value & 037;
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
		if (n <= 3) {
			while (n--)
				lshift1(op);
			return;
		}
		printf("\tmov\tcx,%d\n", n);
		lbl2 = 0;
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
