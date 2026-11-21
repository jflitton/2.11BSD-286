/*
 *	C compiler part 2 for the 80286 -- expression optimizer
 *
 * This is the PDP-11 compiler's c12.c with what was the PDP-11's own
 * taken out or turned round:
 *
 *	- a & b stays an AND, and a &= b an ASAND; the PDP-11 had only
 *	  a bit-clear instruction and made them ANDN and ASANDN.
 *	- a right shift stays one.  RSHIFT and ASRSH are arithmetic; ULSH
 *	  and ASULSH are the logical right shifts, and their counts are
 *	  positive.  The PDP-11 shifted left by a negative count.
 *	- an auto-increment or auto-decrement operand (AUTOI, AUTOD) is
 *	  made only where the step is the size of the operand, which on
 *	  the PDP-11 the instruction saw to.
 *	- the low word of a long is at the lower address.
 *	- floating constants are not put into the PDP-11's format.
 *	- the long multiply and divide that assign (LASTIMES and the
 *	  rest) keep their left operand as it is; the PDP-11 called a
 *	  routine with its address.  And whether a long is divided as
 *	  unsigned is asked of the operands (hardlongs).
 *	- int is 16 bits whatever the compiler itself is compiled with:
 *	  constants are brought back to that after arithmetic on them.
 */

#if	!defined(lint) && defined(DOSCCS)
static	char	sccsid[] = "@(#)c12.c	2.1 (2.11BSD) 2025/12/25";
#endif

#include "c1.h"

#define	MAX(a,b)	((a)>(b)? (a): (b))

static char *divcheck = "Divide Check";

union tree *
optim(tree)
register union tree *tree;
{
	register op, dope;
	int d1, d2;
	union tree *t;

	if (tree==NULL)
		return(NULL);
	if ((op = tree->t.op)==0)
		return(tree);
	if (op==NAME && tree->n.class==AUTO) {
		tree->n.class = OFFS;
		tree->n.regno = BP;
		tree->n.offset = tree->n.nloc;
	}
	dope = opdope[op];
	if ((dope&LEAF) != 0)
		return(tree);
	if ((dope&BINARY) == 0)
		return(unoptim(tree));
	/*
	 * is known to be binary
	 * A call keeps its type: what a function of type char returns
	 * is made an int where it is called (gexpr, in c10.c).
	 */
	if (tree->t.type==CHAR && op!=CALL)
		tree->t.type = INT;
	switch(op) {
	/*
	 * int->ptr via multiplication
	 * Longs are just truncated.
	 */
	case LTOP:
		tree->t.op = ITOP;
		tree->t.tr1 = unoptim(tnode(LTOI,INT,tree->t.tr1, TNULL));
	case ITOP:
		tree->t.op = TIMES;
		break;

	case MINUS:
		if ((t = isconstant(tree->t.tr2)) && (!uns(t) || tree->t.type!=LONG)
		 && (t->t.type!=INT || t->c.value!=-32768)) {
			tree->t.op = PLUS;
			t->c.value = W(-t->c.value);
		}
		break;

	/*
	 * A far pointer and an int.  Constants are gathered into one
	 * addition, which may then be part of an operand (faradr, in
	 * c10.c), and adding nothing is nothing.
	 */
	case FPLUS:
	case FMINUS:
		tree->t.tr1 = optim(tree->t.tr1);
		tree->t.tr2 = optim(tree->t.tr2);
		/*
		 * What is added to a near pointer made far is added to
		 * the near pointer, which is then made far: so the
		 * address of a variable stays one, which an operand may
		 * be made of (FSTAR, in unoptim).
		 */
		if (tree->t.tr1->t.op==NTOF || tree->t.tr1->t.op==XTOF
		 || tree->t.tr1->t.op==STOF || tree->t.tr1->t.op==BTOF) {
			t = tree->t.tr1;
			tree->t.op += PLUS-FPLUS;
			tree->t.tr1 = t->t.tr1;
			tree->t.type = t->t.tr1->t.type;
			t->t.tr1 = optim(tree);
			t->t.degree = MAX(degree(t->t.tr1), 1);
			return(t);
		}
		t = tree->t.tr2;
		if (t->t.op==CON) {
			if (op==FMINUS) {
				tree->t.op = FPLUS;
				t->c.value = W(-t->c.value);
			}
			if (tree->t.tr1->t.op==FPLUS
			 && tree->t.tr1->t.tr2->t.op==CON) {
				t->c.value = W(t->c.value
				    + tree->t.tr1->t.tr2->c.value);
				tree->t.tr1 = tree->t.tr1->t.tr1;
			}
			if (t->c.value==0)
				return(tree->t.tr1);
		}
		tree->t.degree = MAX(MAX(degree(tree->t.tr1), 1),
		    degree(tree->t.tr2)) + 1;
		return(tree);

	case ASFPLUS:
	case ASFMINUS:
		tree->t.tr1 = optim(tree->t.tr1);
		tree->t.tr2 = optim(tree->t.tr2);
		if (tree->t.tr2->t.op==CON && tree->t.tr2->c.value==0)
			return(tree->t.tr1);
		tree->t.degree = MAX(MAX(degree(tree->t.tr1), 1),
		    degree(tree->t.tr2)) + 1;
		return(tree);
	}
	op = tree->t.op;
	dope = opdope[op];
	if (dope&LVALUE && tree->t.tr1->t.op==FSEL)
		return(lvfield(tree));
	if ((dope&COMMUTE)!=0) {
		d1 = tree->t.type;
		tree = acommute(tree);
		if (tree->t.op == op)
			tree->t.type = d1;
		if (tree->t.op!=AND)
			return(tree);
		/*
		 * long & pos-int is simpler
		 */
		if ((tree->t.type==LONG || tree->t.type==UNLONG) && tree->t.tr2->t.op==ITOL
		 && (tree->t.tr2->t.tr1->t.op==CON && tree->t.tr2->t.tr1->c.value>=0
		   || uns(tree->t.tr2->t.tr1))) {
			tree->t.type = UNSIGN;
			t = tree->t.tr2;
			tree->t.tr2 = tree->t.tr2->t.tr1;
			t->t.tr1 = tree;
			tree->t.tr1 = tnode(LTOI, UNSIGN, tree->t.tr1, TNULL);
			return(optim(t));
		}
		/*
		 * Keep constants to the right
		 */
		if ((tree->t.tr1->t.op==ITOL && tree->t.tr1->t.tr1->t.op==CON)
		  || tree->t.tr1->t.op==LCON) {
			t = tree->t.tr1;
			tree->t.tr1 = tree->t.tr2;
			tree->t.tr2 = t;
		}
		return(tree);
	}
	tree->t.tr1 = optim(tree->t.tr1);
	tree->t.tr2 = optim(tree->t.tr2);
	if (tree->t.type == LONG || tree->t.type==UNLONG) {
		t = lconst(tree->t.op, tree->t.tr1, tree->t.tr2);
		if (t)
			return(t);
	}
	if ((dope&RELAT) != 0) {
		if ((d1=degree(tree->t.tr1)) < (d2=degree(tree->t.tr2))
		 || d1==d2 && tree->t.tr1->t.op==NAME && tree->t.tr2->t.op!=NAME) {
			t = tree->t.tr1;
			tree->t.tr1 = tree->t.tr2;
			tree->t.tr2 = t;
			tree->t.op = maprel[op-EQUAL];
		}
		if (tree->t.tr1->t.type==CHAR && tree->t.tr2->t.op==CON
		 && (opdope[tree->t.tr1->t.op]&LEAF || tree->t.tr1->t.op==STAR)
		 && tree->t.tr2->c.value <= 127 && tree->t.tr2->c.value >= 0)
			tree->t.tr2->t.type = CHAR;
	}
	d1 = MAX(degree(tree->t.tr1), islong(tree->t.type));
	d2 = MAX(degree(tree->t.tr2), 0);
	switch (op) {

	/*
	 * In assignment to fields, treat all-zero and all-1 specially.
	 */
	case FSELA:
		if (tree->t.tr2->t.op==CON && tree->t.tr2->c.value==0) {
			tree->t.op = ASAND;
			tree->t.tr2->c.value = ~tree->F.mask;
			return(optim(tree));
		}
		if (tree->t.tr2->t.op==CON && tree->F.mask==tree->t.tr2->c.value) {
			tree->t.op = ASOR;
			return(optim(tree));
		}

	case LTIMES:
	case LDIV:
	case LMOD:
	case LASTIMES:
	case LASDIV:
	case LASMOD:
	case UDIV:
	case UMOD:
	case ASUDIV:
	case ASUMOD:
	case ULASMOD:
	case ULTIMES:
	case ULDIV:
	case ULMOD:
	case ULASTIMES:
	case ULASDIV:
		tree->t.degree = 10;
		break;

	case CALL:
		tree->t.degree = 10;
		break;

	case QUEST:
	case COLON:
		tree->t.degree = MAX(d1, d2);
		break;

	case PTOI:
	case DIVIDE:
	case ASDIV:
	case ASTIMES:
		if (tree->t.tr2->t.op==CON && tree->t.tr2->c.value==1) {
			if (op==PTOI)
				return(optim(tnode(LTOI,INT,paint(tree->t.tr1,LONG), TNULL)));
			return(paint(tree->t.tr1, tree->t.type));
		}
	case MOD:
	case ASMOD:
		if (uns(tree->t.tr1) && tree->t.op!=PTOI && ispow2(tree))
			return(pow2(tree));
		if ((op==MOD||op==ASMOD) && tree->t.type==DOUBLE) {
			error("Floating %% not defined");
			tree->t.type = INT;
		}
		d1 += 2;
		d2 += 2;
		if (tree->t.type==LONG || tree->t.type==UNLONG)
			return(hardlongs(tree));
		if ((op==MOD || op==DIVIDE || op==ASMOD || op==ASDIV)
		 && (uns(tree->t.tr1) || uns(tree->t.tr2))) {
			if (op>=ASDIV) {
				tree->t.op += ASUDIV - ASDIV;
			} else
				tree->t.op += UDIV - DIVIDE;
			op = tree->t.op;
		}
		goto constant;

	case ASPLUS:
	case ASMINUS:
		if (tree->t.tr2->t.op==CON && tree->t.tr2->c.value==0)
			return(tree->t.tr1);
		goto def;

	case LSHIFT:
	case RSHIFT:
	case ASRSH:
	case ASLSH:
		if (tree->t.tr2->t.op==CON && tree->t.tr2->c.value==0)
			return(paint(tree->t.tr1, tree->t.type));
		if (tree->t.type == LONG || tree->t.type==UNLONG) {
			d1++;
			d2++;
		}
		/*
		 * A right shift of something unsigned, or by something
		 * unsigned, brings in zeros.
		 */
		if (uns(tree->t.tr1) || uns(tree->t.tr2)) {
			if (op==RSHIFT)
				tree->t.op = op = ULSH;
			else if (op==ASRSH)
				tree->t.op = op = ASULSH;
		}
		/* fall through */

	case ULSH:
	case ASULSH:

	constant:
		if (tree->t.tr1->t.op==CON && tree->t.tr2->t.op==CON) {
			Xconst(op, &tree->t.tr1->c.value, tree->t.tr2->c.value, tree->t.type);
			return(tree->t.tr1);
		}


	def:
	default:
		if (dope&RELAT) {
			if (tree->t.tr1->t.type==LONG || tree->t.tr1->t.type==UNLONG)	/* long relations are a mess */
				d1 = 10;
			if (opdope[tree->t.tr1->t.op]&RELAT && tree->t.tr2->t.op==CON
			 && tree->t.tr2->c.value==0) {
				tree = tree->t.tr1;
				switch(op) {
				case GREATEQ:
					return((union tree *)&cone);
				case LESS:
					return((union tree *)&czero);
				case LESSEQ:
				case EQUAL:
					tree->t.op = notrel[tree->t.op-EQUAL];
				}
				return(tree);
			}
		}
		tree->t.degree = d1==d2? d1+islong(tree->t.type): MAX(d1, d2);
		break;
	}
	return(tree);
}

union tree *
unoptim(tree)
register union tree *tree;
{
	register union tree *subtre, *p;

	if (tree==NULL)
		return(NULL);
    again:
	if (tree->t.op==AMPER && tree->t.tr1->t.op==STAR) {
		subtre = tree->t.tr1->t.tr1;
		subtre->t.type = tree->t.type;
		return(optim(subtre));
	}
	/*
	 * The address of what a far pointer points at is the pointer.
	 */
	if (tree->t.op==AMPER && tree->t.tr1->t.op==FSTAR)
		return(optim(tree->t.tr1->t.tr1));
	subtre = tree->t.tr1 = optim(tree->t.tr1);
	switch (tree->t.op) {

	case INCAFT:
	case DECAFT:
		if (tree->t.type!=subtre->t.type) 
			paint(subtre, tree->t.type);
		break;

	case ITOL:
		if (subtre->t.op==CON && subtre->t.type==INT && subtre->c.value<0) {
			subtre = getblk(sizeof(struct lconst));
			subtre->t.op = LCON;
			subtre->t.type = LONG;
			subtre->l.lvalue = tree->t.tr1->c.value;
			return(subtre);
		}
		break;

	case FTOI:
		if (uns(tree)) {
			tree->t.op = FTOL;
			tree->t.type = LONG;
			tree = tnode(LTOI, UNSIGN, tree, TNULL);
		}
		break;

	case LTOF:
		if (subtre->t.op==LCON) {
			tree = getblk(sizeof(struct ftconst));
			tree->t.op = FCON;
			tree->t.type = DOUBLE;
			tree->c.value = isn++;
			tree->f.fvalue = subtre->l.lvalue;
			return(optim(tree));
		}
		if (subtre->t.type==UNLONG) 
			tree->t.op = ULTOF;
		break;

	case ITOF:
		if (subtre->t.op==CON) {
			tree = getblk(sizeof(struct ftconst));
			tree->t.op = FCON;
			tree->t.type = DOUBLE;
			tree->f.value = isn++;
			if (uns(subtre))
				tree->f.fvalue = (unsigned)subtre->c.value;
			else
				tree->f.fvalue = subtre->c.value;
			return(optim(tree));
		}
		if (uns(subtre)) {
			tree->t.tr1 = tnode(ITOL, LONG, subtre, TNULL);
			tree->t.op = LTOF;
			return(optim(tree));
		}
		break;

	case ITOC:
		/*
		 * Sign-extend characters.  A variable in memory is read
		 * as its low byte, which is its first; one in a register
		 * is not, SI and DI having no halves.
		 */
		if (subtre->t.op==CON) {
			subtre->c.value = (subtre->c.value&0377) - ((subtre->c.value&0200)<<1);
			subtre->t.type = tree->t.type;
			return(subtre);
		} else if (subtre->t.op==NAME && tree->t.type==INT
		    && subtre->n.class!=REG) {
			subtre->t.type = CHAR;
			return(subtre);
		}
		break;

	case LTOI:
		switch (subtre->t.op) {

		case LCON:
			subtre->t.op = CON;
			subtre->t.type = tree->t.type;
			subtre->c.value = W(subtre->l.lvalue);
			return(subtre);

		/*
		 * The low word of a long is its first.
		 */
		case NAME:
			subtre->t.type = tree->t.type;
			return(subtre);

		case STAR:
			subtre->t.type = tree->t.type;
			subtre->t.tr1->t.type = tree->t.type+PTR;
			return(optim(subtre));

		case ITOL:
			return(paint(subtre->t.tr1, tree->t.type));

		/*
		 * The offset of a far pointer that was a near one is
		 * that one, and of one that was added to is a sum.
		 */
		case NTOF:
		case CTOF:
		case XTOF:
		case STOF:
		case BTOF:
			return(paint(subtre->t.tr1, tree->t.type));

		case FPLUS:
		case FMINUS:
			subtre->t.op += PLUS-FPLUS;
			subtre->t.tr1 = tnode(LTOI, tree->t.type, subtre->t.tr1, TNULL);
			subtre->t.type = tree->t.type;
			return(optim(subtre));

		case PLUS:
		case MINUS:
		case AND:
		case ANDN:
		case OR:
		case EXOR:
			subtre->t.tr2 = tnode(LTOI, tree->t.type, subtre->t.tr2, TNULL);
		case NEG:
		case COMPL:
			subtre->t.tr1 = tnode(LTOI, tree->t.type, subtre->t.tr1, TNULL);
			subtre->t.type = tree->t.type;
			return(optim(subtre));
		}
		break;

	case FSEL:
		tree->t.op = AND;
		tree->t.tr1 = tree->t.tr2->t.tr1;
		tree->t.tr2->t.tr1 = subtre;
		tree->t.tr2->t.op = RSHIFT;
		tree->t.tr1->c.value = W((1 << tree->t.tr1->c.value) - 1);
		return(optim(tree));

	case FSELR:
		tree->t.op = LSHIFT;
		tree->t.type = UNSIGN;
		tree->t.tr1 = tree->t.tr2;
		tree->t.tr1->t.op = AND;
		tree->t.tr2 = tree->t.tr2->t.tr2;
		tree->t.tr1->t.tr2 = subtre;
		tree->t.tr1->t.tr1->c.value = W((1 << tree->t.tr1->t.tr1->c.value) - 1);
		return(optim(tree));

	case AMPER:
		if (subtre->t.op==STAR)
			return(subtre->t.tr1);
		if (subtre->t.op==NAME && subtre->n.class == OFFS) {
			p = tnode(PLUS, tree->t.type, subtre, tree);
			subtre->t.type = tree->t.type;
			tree->t.op = CON;
			tree->t.type = INT;
			tree->t.degree = 0;
			tree->c.value = subtre->n.offset;
			subtre->n.class = REG;
			subtre->n.nloc = subtre->n.regno;
			subtre->n.offset = 0;
			return(optim(p));
		}
		if (subtre->t.op==LOAD) {
			tree->t.tr1 = subtre->t.tr1;
			goto again;
		}
		break;

	/*
	 * What a near pointer made far points at is what the near
	 * pointer does: the segment it is given is where those are.
	 */
	case FSTAR:
		if (subtre->t.op==NTOF) {
			tree->t.op = STAR;
			tree->t.tr1 = subtre->t.tr1;
			goto again;
		}
		/*
		 * What a pointer into SS points at, where that pointer is
		 * a place in the frame, is [bp+n], which is through SS.
		 * A structure is left far, as it is copied so.
		 */
		if (subtre->t.op==STOF && tree->t.type!=STRUCT
		 && (p = subtre->t.tr1)->t.op==PLUS && p->t.tr2->t.op==CON
		 && p->t.tr1->t.op==NAME && p->t.tr1->n.class==REG
		 && p->t.tr1->n.nloc==BP) {
			tree->t.op = STAR;
			tree->t.tr1 = p;
			goto again;
		}
		break;

	case STAR:
		if (subtre->t.op==AMPER) {
			subtre->t.tr1->t.type = tree->t.type;
			return(subtre->t.tr1);
		}
		if (tree->t.type==STRUCT)
			break;
		if (subtre->t.op==NAME && subtre->n.class==REG) {
			subtre->t.type = tree->t.type;
			subtre->n.class = OFFS;
			subtre->n.regno = subtre->n.nloc;
			return(subtre);
		}
		p = subtre->t.tr1;
		/*
		 * *p++ and *--p, where p is a register variable and is
		 * moved by the size of what is fetched, are operands
		 * of their own.  The 286 has no such addressing, but
		 * the code generator makes them of what it has (autoop,
		 * in c10.c), and so they happen when the PDP-11's did:
		 * as the operand is used, not after the expression.
		 */
		if ((subtre->t.op==INCAFT||subtre->t.op==DECBEF)
		 && tree->t.type!=LONG && tree->t.type!=UNLONG
		 && p->t.op==NAME && p->n.class==REG && p->t.type==subtre->t.type
		 && subtre->t.tr2->t.op==CON
		 && subtre->t.tr2->c.value==autosize(tree->t.type)) {
			p->t.type = tree->t.type;
			p->t.op = subtre->t.op==INCAFT? AUTOI: AUTOD;
			return(p);
		}
		if (subtre->t.op==PLUS && p->t.op==NAME && p->n.class==REG) {
			if (subtre->t.tr2->t.op==CON) {
				p->n.offset += subtre->t.tr2->c.value;
				p->n.class = OFFS;
				p->t.type = tree->t.type;
				p->n.regno = p->n.nloc;
				return(p);
			}
			if (subtre->t.tr2->t.op==AMPER) {
				subtre = subtre->t.tr2->t.tr1;
				subtre->n.class += XOFFS-EXTERN;
				subtre->n.regno = p->n.nloc;
				subtre->t.type = tree->t.type;
				return(subtre);
			}
		}
		if (subtre->t.op==MINUS && p->t.op==NAME && p->n.class==REG
		 && subtre->t.tr2->t.op==CON) {
			p->n.offset -= subtre->t.tr2->c.value;
			p->n.class = OFFS;
			p->t.type = tree->t.type;
			p->n.regno = p->n.nloc;
			return(p);
		}
		break;
	case EXCLA:
		if ((opdope[subtre->t.op]&RELAT)==0)
			break;
		tree = subtre;
		tree->t.op = notrel[tree->t.op-EQUAL];
		break;

	case COMPL:
		if (tree->t.type==CHAR)
			tree->t.type = INT;
		if (tree->t.op == subtre->t.op)
			return(paint(subtre->t.tr1, tree->t.type));
		if (subtre->t.op==CON) {
			subtre->c.value = W(~subtre->c.value);
			return(paint(subtre, tree->t.type));
		}
		if (subtre->t.op==LCON) {
			subtre->l.lvalue = ~subtre->l.lvalue;
			return(subtre);
		}
		if (subtre->t.op==ITOL) {
			if (subtre->t.tr1->t.op==CON) {
				tree = getblk(sizeof(struct lconst));
				tree->t.op = LCON;
				tree->t.type = LONG;
				if (uns(subtre->t.tr1))
					tree->l.lvalue = ~(long)(unsigned)subtre->t.tr1->c.value;
				else
					tree->l.lvalue = ~subtre->t.tr1->c.value;
				return(tree);
			}
			if (uns(subtre->t.tr1))
				break;
			subtre->t.op = tree->t.op;
			subtre->t.type = subtre->t.tr1->t.type;
			tree->t.op = ITOL;
			tree->t.type = LONG;
			goto again;
		}

	case NEG:
		if (tree->t.type==CHAR)
			tree->t.type = INT;
		if (tree->t.op==subtre->t.op)
			return(paint(subtre->t.tr1, tree->t.type));
		if (subtre->t.op==CON) {
			subtre->c.value = W(-subtre->c.value);
			return(paint(subtre, tree->t.type));
		}
		if (subtre->t.op==LCON) {
			subtre->l.lvalue = -subtre->l.lvalue;
			return(subtre);
		}
		if (subtre->t.op==ITOL && subtre->t.tr1->t.op==CON) {
			tree = getblk(sizeof(struct lconst));
			tree->t.op = LCON;
			tree->t.type = LONG;
			if (uns(subtre->t.tr1))
				tree->l.lvalue = -(long)(unsigned)subtre->t.tr1->c.value;
			else
				tree->l.lvalue = -subtre->t.tr1->c.value;
			return(tree);
		}
		if (subtre->t.op==FCON) {
			subtre->f.fvalue = -subtre->f.fvalue;
			return(subtre);
		}
	}
	if ((opdope[tree->t.op]&LEAF)==0)
		tree->t.degree = MAX(islong(tree->t.type), degree(subtre));
	return(tree);
}

/*
 * Deal with assignments to partial-word fields.
 * The game is that select(x) += y turns into
 * select(x += select(y)) where the shifts and masks
 * are chosen properly.  The outer select
 * is discarded where the value doesn't matter.
 * Sadly, overflow is undetected on += and the like.
 * Pure assignment is handled specially.
 */

union tree *
lvfield(t)
register union tree *t;
{
	register union tree *t1, *t2;

	switch (t->t.op) {

	case ASSIGN:
		t2 = getblk(sizeof(struct fasgn));
		t2->t.op = FSELA;
		t2->t.type = UNSIGN;
		t1 = t->t.tr1->t.tr2;
		t2->F.mask = W(((1<<t1->t.tr1->c.value)-1)<<t1->t.tr2->c.value);
		t2->t.tr1 = t->t.tr1;
		t2->t.tr2 = t->t.tr2;
		t = t2;

	case ASAND:
	case ASPLUS:
	case ASMINUS:
	case ASOR:
	case ASXOR:
	case INCBEF:
	case INCAFT:
	case DECBEF:
	case DECAFT:
		t1 = t->t.tr1;
		t1->t.op = FSELR;
		t->t.tr1 = t1->t.tr1;
		t1->t.tr1 = t->t.tr2;
		t->t.tr2 = t1;
		t1 = t1->t.tr2;
		/*
		 * The word is and-ed with the value in its place, and
		 * with ones everywhere else.
		 */
		if (t->t.op==ASAND)
			t->t.tr2 = tnode(OR, UNSIGN, t->t.tr2,
			    tconst(W(~(((1<<t1->t.tr1->c.value)-1)
			    <<t1->t.tr2->c.value)), INT));
		t1 = tnode(COMMA, INT, tconst(t1->t.tr1->c.value, INT),
			tconst(t1->t.tr2->c.value, INT));
		return(optim(tnode(FSELT, UNSIGN, t, t1)));

	}
	error("Unimplemented field operator");
	return(t);
}

#define	LSTSIZ	20
struct acl {
	int nextl;
	int nextn;
	union tree *nlist[LSTSIZ];
	union tree *llist[LSTSIZ+1];
};

union tree *
acommute(tree)
register union tree *tree;
{
	struct acl acl;
	int d, i, op, flt, d1, type;
	register union tree *t1, **t2;
	union tree *t;

	acl.nextl = 0;
	acl.nextn = 0;
	op = tree->t.op;
	type = tree->t.type;
	flt = isfloat(tree);
	insert(op, tree, &acl);
	acl.nextl--;
	t2 = &acl.llist[acl.nextl];
	if (!flt) {
		/* put constants together */
		for (i=acl.nextl; i>0; i--) {
			d = t2[-1]->t.type==UNSIGN||t2[0]->t.type==UNSIGN?UNSIGN:INT;
			if (t2[0]->t.op==CON && t2[-1]->t.op==CON) {
				acl.nextl--;
				t2--;
				Xconst(op, &t2[0]->c.value, t2[1]->c.value, d);
				t2[0]->t.type = d;
			} else if (t = lconst(op, t2[-1], t2[0])) {
				acl.nextl--;
				t2--;
				t2[0] = t;
			}
		}
	}
	if (op==PLUS || op==OR) {
		/* toss out "+0" */
		if (acl.nextl>0 && ((t1 = isconstant(*t2)) && t1->c.value==0
		 || (*t2)->t.op==LCON && (*t2)->l.lvalue==0)) {
			acl.nextl--;
			t2--;
		}
		if (acl.nextl <= 0) {
			if ((*t2)->t.type==CHAR || (*t2)->t.type==UNCHAR)
				*t2 = tnode(LOAD, tree->t.type, *t2, TNULL);
			(*t2)->t.type = tree->t.type;
			return(*t2);
		}
		/* subsume constant in "&x+c" */
		if (op==PLUS && t2[0]->t.op==CON && t2[-1]->t.op==AMPER) {
			t2--;
			t2[0]->t.tr1->n.offset += t2[1]->c.value;
			acl.nextl--;
		}
	} else if (op==TIMES || op==AND) {
		t1 = acl.llist[acl.nextl];
		if (t1->t.op==CON) {
			if (t1->c.value==0) {
				for (i=0; i<acl.nextl; i++)
					if (sideeffects(acl.llist[i]))
						break;
				if (i==acl.nextl)
					return(t1);
			}
			if (op==TIMES && t1->c.value==1 && acl.nextl>0)
				if (--acl.nextl <= 0) {
					t1 = acl.llist[0];
					if (uns(tree))
						paint(t1, tree->t.type);
					return(t1);
				}
		}
	}
	if (op==PLUS && !flt)
		distrib(&acl);
	tree = *(t2 = &acl.llist[0]);
	d = MAX(degree(tree), islong(tree->t.type));
	if (op==TIMES && !flt)
		d++;
	for (i=0; i<acl.nextl; i++) {
		t1 = acl.nlist[i];
		t1->t.tr2 = t = *++t2;
		d1 = degree(t);
		t1->t.degree = d = d==d1? d+islong(t1->t.type): MAX(d, d1);
		t1->t.tr1 = tree;
		tree = t1;
		if (tree->t.type==LONG || tree->t.type==UNLONG) {
			if (tree->t.op==TIMES)
				tree = hardlongs(tree);
			else if (tree->t.op==PLUS && (t = isconstant(tree->t.tr1))
			       && t->c.value < 0 && !uns(t)) {
				tree->t.op = MINUS;
				t->c.value = - t->c.value;
				t = tree->t.tr1;
				tree->t.tr1 = tree->t.tr2;
				tree->t.tr2 = t;
			}
		}
	}
	if (tree->t.op==TIMES && ispow2(tree))
		tree->t.degree = MAX(degree(tree->t.tr1), islong(tree->t.type));
	paint(tree, type);
	return(tree);
}

sideeffects(tp)
register union tree *tp;
{
	register dope;

	if (tp==NULL)
		return(0);
	dope = opdope[tp->t.op];
	if (dope&LEAF) {
		if (tp->t.op==AUTOI || tp->t.op==AUTOD)
			return(1);
		return(0);
	}
	if (dope&ASSGOP)
		return(1);
	switch(tp->t.op) {
	case CALL:
	case FSELA:
	case STRASG:
		return(1);
	}
	if (sideeffects(tp->t.tr1))
		return(1);
	if (dope&BINARY)
		return(sideeffects(tp->t.tr2));
	return(0);
}

distrib(list)
struct acl *list;
{
/*
 * Find a list member of the form c1c2*x such
 * that c1c2 divides no other such constant, is divided by
 * at least one other (say in the form c1*y), and which has
 * fewest divisors. Reduce this pair to c1*(y+c2*x)
 * and iterate until no reductions occur.
 */
	register union tree **p1, **p2;
	union tree *t;
	int ndmaj, ndmin;
	union tree **dividend, **divisor;
	union tree **maxnod, **mindiv;

    loop:
	maxnod = &list->llist[list->nextl];
	ndmaj = 1000;
	dividend = 0;
	for (p1 = list->llist; p1 <= maxnod; p1++) {
		if ((*p1)->t.op!=TIMES || (*p1)->t.tr2->t.op!=CON)
			continue;
		ndmin = 0;
		for (p2 = list->llist; p2 <= maxnod; p2++) {
			if (p1==p2 || (*p2)->t.op!=TIMES || (*p2)->t.tr2->t.op!=CON)
				continue;
			if ((*p1)->t.tr2->c.value == (*p2)->t.tr2->c.value) {
				(*p2)->t.tr2 = (*p1)->t.tr1;
				(*p2)->t.op = PLUS;
				(*p1)->t.tr1 = (*p2);
				*p1 = optim(*p1);
				squash(p2, maxnod);
				list->nextl--;
				goto loop;
			}
			if (((*p2)->t.tr2->c.value % (*p1)->t.tr2->c.value) == 0)
				goto contmaj;
			if (((*p1)->t.tr2->c.value % (*p2)->t.tr2->c.value) == 0) {
				ndmin++;
				mindiv = p2;
			}
		}
		if (ndmin > 0 && ndmin < ndmaj) {
			ndmaj = ndmin;
			dividend = p1;
			divisor = mindiv;
		}
    contmaj:;
	}
	if (dividend==0)
		return;
	t = list->nlist[--list->nextn];
	p1 = dividend;
	p2 = divisor;
	t->t.op = PLUS;
	t->t.type = (*p1)->t.type;
	t->t.tr1 = (*p1);
	t->t.tr2 = (*p2)->t.tr1;
	(*p1)->t.tr2->c.value /= (*p2)->t.tr2->c.value;
	(*p2)->t.tr1 = t;
	t = optim(*p2);
	if (p1 < p2) {
		*p1 = t;
		squash(p2, maxnod);
	} else {
		*p2 = t;
		squash(p1, maxnod);
	}
	list->nextl--;
	goto loop;
}

squash(p, maxp)
union tree **p, **maxp;
{
	register union tree **np;

	for (np = p; np < maxp; np++)
		*np = *(np+1);
}

/*
 * Arithmetic on two constants.  The work is done in the compiler's own
 * int, and the answer made the 286's.
 */
Xconst(op, vp, v, type)
register int *vp, v;
{
	if (type==INT)
		*vp = W(*vp);
	else
		*vp = UNS(*vp);
	if (type!=INT && op!=LSHIFT && op!=RSHIFT && op!=ULSH)
		v = UNS(v);
	Xconst1(op, vp, v, type);
	*vp = W(*vp);
}

Xconst1(op, vp, v, type)
register int *vp, v;
{
	switch (op) {

	case PTOI:
		(*vp) /= (unsigned)v;
		return;

	case PLUS:
		*vp += v;
		return;

	case TIMES:
		*vp *= v;
		return;

	case AND:
		*vp &= v;
		return;

	case OR:
		*vp |= v;
		return;

	case EXOR:
		*vp ^= v;
		return;

	case UDIV:
	case UMOD:
		type = UNSIGN;
	case DIVIDE:
	case MOD:
		if (type==UNSIGN && v!=0 && v<=1) {
			if (op==UDIV || op==DIVIDE) {
				if (v==1)
					return;
				*vp = *(unsigned *)vp >= (unsigned)v;
				return;
			} else {
				if (v==1) {
					*vp = 0;
					return;
				}
				if (*(unsigned *)vp >= (unsigned)v)
					*vp -= v;
				return;
			}
		}
		if (v==0)
			werror(divcheck);
		else
			if (type==INT)
				if (op==DIVIDE || op==UDIV)
					*vp /= v;
				else
					*vp %= v;
			else
				if (op==DIVIDE || op==UDIV)
					*(unsigned *)vp /= (unsigned)v;
				else
					*(unsigned *)vp %= (unsigned)v;
			return;

	case RSHIFT:
		if (v<0 || v>15)
			v = 15;
		*vp >>= v;
		return;

	case ULSH:
		if (v<0 || v>15)
			*vp = 0;
		else
			*vp = UNS(*vp) >> v;
		return;

	case LSHIFT:
		if (v<0 || v>15)
			*vp = 0;
		else
			*vp <<= v;
		return;
	}
	error("C error: Xconst");
}

union tree *
lconst(op, lp, rp)
register union tree *lp, *rp;
{
	LTYPE l, r;

	if (lp->t.op==LCON)
		l = lp->l.lvalue;
	else if (lp->t.op==ITOL && lp->t.tr1->t.op==CON) {
		if (lp->t.tr1->t.type==INT)
			l = lp->t.tr1->c.value;
		else
			l = (unsigned)lp->t.tr1->c.value;
	} else
		return(0);
	if (rp->t.op==LCON)
		r = rp->l.lvalue;
	else if (rp->t.op==ITOL && rp->t.tr1->t.op==CON) {
		if (rp->t.tr1->t.type==INT)
			r = rp->t.tr1->c.value;
		else
			r = (unsigned)rp->t.tr1->c.value;
	} else
		return(0);
	switch (op) {

	case PLUS:
		l += r;
		break;

	case MINUS:
		l -= r;
		break;

	case TIMES:
	case LTIMES:
		l *= r;
		break;

	case DIVIDE:
	case LDIV:
		if (r==0)
			error(divcheck);
		else
			l /= r;
		break;

	case MOD:
	case LMOD:
		if (r==0)
			error(divcheck);
		else
			l %= r;
		break;

	case AND:
		l &= r;
		break;

	case ANDN:
		l &= ~r;
		break;

	case OR:
		l |= r;
		break;

	case EXOR:
		l ^= r;
		break;

	case LSHIFT:
		l <<= r;
		break;

	case RSHIFT:
		l >>= r;
		break;

	default:
		return(0);
	}
	if (lp->t.op==LCON) {
		lp->l.lvalue = l;
		return(lp);
	}
	lp = getblk(sizeof(struct lconst));
	lp->t.op = LCON;
	lp->t.type = LONG;
	lp->l.lvalue = l;
	return(lp);
}

insert(op, tree, list)
register union tree *tree;
register struct acl *list;
{
	register d;
	int d1, i;
	union tree *t;

ins:
	if (tree->t.op != op)
		tree = optim(tree);
	if (tree->t.op == op && list->nextn < LSTSIZ-2) {
		list->nlist[list->nextn++] = tree;
		insert(op, tree->t.tr1, list);
		insert(op, tree->t.tr2, list);
		return;
	}
	if (!isfloat(tree)) {
		/* c1*(x+c2) -> c1*x+c1*c2 */
		if ((tree->t.op==TIMES||tree->t.op==LSHIFT)
		  && tree->t.tr2->t.op==CON && tree->t.tr2->c.value>0
		  && tree->t.tr1->t.op==PLUS && tree->t.tr1->t.tr2->t.op==CON) {
			d = tree->t.tr2->c.value;
			if (tree->t.op==TIMES)
				tree->t.tr2->c.value *= tree->t.tr1->t.tr2->c.value;
			else
				tree->t.tr2->c.value = tree->t.tr1->t.tr2->c.value << d;
			tree->t.tr1->t.tr2->c.value = d;
			tree->t.tr1->t.op = tree->t.op;
			tree->t.op = PLUS;
			tree = optim(tree);
			if (op==PLUS)
				goto ins;
		}
	}
	d = degree(tree);
	for (i=0; i<list->nextl; i++) {
		if ((d1=degree(list->llist[i]))<d) {
			t = list->llist[i];
			list->llist[i] = tree;
			tree = t;
			d = d1;
		}
	}
	list->llist[list->nextl++] = tree;
}

union tree *
tnode(op, type, tr1, tr2)
union tree *tr1, *tr2;
{
	register union tree *p;

	p = getblk(sizeof(struct tnode));
	p->t.op = op;
	p->t.type = type;
	p->t.degree = 0;
	p->t.tr1 = tr1;
	p->t.tr2 = tr2;
	return(p);
}

union tree *
tconst(val, type)
{
	register union tree *p;

	p = getblk(sizeof(struct tconst));
	p->t.op = CON;
	p->t.type = type;
	p->c.value = val;
	return(p);
}

union tree *
getblk(size)
{
	register union tree *p;

	if (size&01)
		size++;
	p = (union tree *)curbase;
	if ((curbase += size) >= coremax) {
		if (sbrk(1024) == (char *)-1) {
			error("Out of space-- c1");
			exit(1);
		}
		coremax += 1024;
	}
	return(p);
}

islong(t)
{
	if (t==LONG || t==UNLONG)
		return(2);
	return(1);
}

union tree *
isconstant(t)
register union tree *t;
{
	if (t->t.op==CON || t->t.op==SFCON)
		return(t);
	if (t->t.op==ITOL && t->t.tr1->t.op==CON)
		return(t->t.tr1);
	return(NULL);
}

/*
 * Multiplying and dividing longs is done by routines, which the
 * operators LTIMES and the rest stand for.  Whether the unsigned ones
 * are wanted is asked of the operands: the first pass makes a cast from
 * unsigned long to long, which costs nothing, by changing the type of
 * what is cast, and (long)(a / b) would otherwise divide with a sign.
 */
union tree *
hardlongs(t)
register union tree *t;
{
	register u;

	u = t->t.type==UNLONG || t->t.tr1->t.type==UNLONG
	    || t->t.tr2->t.type==UNLONG;
	switch(t->t.op) {

	case TIMES:
	case DIVIDE:
	case MOD:
		if (u)
			t->t.op += ULTIMES-TIMES;
		else
			t->t.op += LTIMES-TIMES;
		break;

	case ASTIMES:
	case ASDIV:
	case ASMOD:
		if (u)
			t->t.op += ULASTIMES-ASTIMES;
		else
			t->t.op += LASTIMES-ASTIMES;
		break;

	default:
		return(t);
	}
	return(optim(t));
}

/*
 * Is tree of unsigned type?
 */
uns(tp)
union tree *tp;
{
	register t;

	t = tp->t.type;
	if (t==UNSIGN || t==UNCHAR || t==UNLONG || t&XTYPE)
		return(1);
	return(0);
}
