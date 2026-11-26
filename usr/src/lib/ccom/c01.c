/*
 * C compiler
 */

#if	!defined(lint) && defined(DOSCCS)
static	char	sccsid[] = "@(#)c01.c	2.2 (2.11BSD) 2026/1/26";
#endif

#include "c0.h"

union tree *protochk();

/*
 * Called from tree, this routine takes the top 1, 2, or 3
 * operands on the expression stack, makes a new node with
 * the operator op, and puts it on the stack.
 * Essentially all the work is in inserting
 * appropriate conversions.
 */
build(op)
{
	register int t1;
	int t2, t;
	register union tree *p1, *p2, *p3;
	int dope, leftc, cvn, pcvn;

	/*
	 * a[i] => *(a+i)
	 */
	if (op==LBRACK) {
		build(PLUS);
		op = STAR;
	}
	dope = opdope[op];
	t2 = INT;
	if ((dope&BINARY)!=0) {
		p2 = chkfun(disarray(*--cp));
		if (p2)
			t2 = p2->t.type;
	}
	p1 = *--cp;
	/*
	 * sizeof gets turned into a number here.
	 */
	if (op==SIZEOF) {
		p1 = cblock(length(p1));
		p1->c.type = UNSIGN;
		*cp++ = p1;
		return;
	}
	if (op!=AMPER) {
		p1 = disarray(p1);
		if (op!=CALL)
			p1 = chkfun(p1);
	}
	t1 = p1->t.type;
	if (t1==CHAR)
		t1 = INT;
	else if (t1==UNCHAR)
		t1 = UNSIGN;
	if (t2==CHAR)
		t2 = INT;
	else if (t2==UNCHAR)
		t2 = UNSIGN;
	pcvn = 0;
	t = INT;
#ifdef	i286
	if ((op==CAST || op==ASSIGN) && (t2&XTYPE)==SPTR
	 && (t1&XTYPE)!=SPTR && !ISFAR(t1) && t1!=VOID)
		ssnear();
#endif
	switch (op) {

	case CAST:
		if ((t1&XTYPE)==FUNC || (t1&XTYPE)==ARRAY)
			error("Disallowed conversion");
		if (p1->t.type==UNCHAR) {
			*cp++ = block(ETYPE, UNSIGN, (int *)NULL, (union str *)NULL,
			   TNULL, TNULL);
			*cp++ = p2;
			build(CAST);
			*cp++ = cblock(0377);
			build(AND);
			return;
		}
		if (p2->t.type==CHAR || p2->t.type==UNCHAR)
			p2 = block(PLUS, t2, (int *)NULL, (union str *)NULL,
			   p2, cblock(0));
		break;

	/* end of expression */
	case 0:
		*cp++ = p1;
		return;

	/* no-conversion operators */
	case QUEST:
		if (p2->t.op!=COLON)
			error("Illegal conditional");
		else
			if (fold(QUEST, p1, p2))
				return;

	/*
	 * Bug fix, because copying type is not enough,
	 * i.e. t = (t, t) + 1;
	 * Original code was:
	 *	case SEQNC:
	 *		t = t2;
	 *	case COMMA:
	 */
	case SEQNC:
		*cp++ = block(op, t2, p2->t.subsp, p2->t.strp, p1, p2);
		return;

	case COMMA:
	case LOGAND:
	case LOGOR:
		*cp++ = block(op, t, p2->t.subsp, p2->t.strp, p1, p2);
		return;

	case EXCLA:
		t1 = INT;
		break;

	case CALL:
#ifdef	i286
		if (((t1&XTYPE) == PTR || ISFAR(t1))
		 && (decref(t1)&XTYPE) == FUNC) {
#else
		if ((t1&XTYPE) == PTR && (decref(t1)&XTYPE) == FUNC) {
#endif
			/*
			 * Modification to allow calling a function via a
			 * pointer to a function ("f") without explicitly
			 * dereferencing the pointer.  That is: f(...) is now
			 * legal as well as (*f)(...).  The insistence that
			 * decref(t1) be FUNC prevents pointers to pointers to
			 * functions and so on from being automatically
			 * dereferenced which would be incorrect and introduce
			 * porting problems.  Note that for purity FUNC's
			 * should really always be referenced (as in pcc) and
			 * that the new notation is actually more consistent
			 * with the rest of C ...
			 */
			*cp++ = p1;
			build(STAR);
			*cp++ = p2;
			build(CALL);
			return;
		}
		if ((t1&XTYPE) != FUNC)
			error("Call of non-function");
		if (p1->t.subsp)
			p2 = protochk(p1->t.subsp[0], p2);
#ifdef	i286
		else if (p2->t.op!=NULLOP)
			ssargs(p2);
#endif
		*cp++ = block(CALL,decref(t1),p1->t.subsp+1,p1->t.strp,p1,p2);
		return;

	case STAR:
		if ((t1&XTYPE) == FUNC)
			error("Illegal indirection");
#ifdef	i286
		/*
		 * What a pointer into SS or into the based segment points
		 * at is what the pointer made far does.
		 */
		if (ISBASED(t1))
			p1 = basedfar(p1);
#endif
		*cp++ = block(STAR, decref(t1), p1->t.subsp, p1->t.strp, p1, TNULL);
		return;

	case AMPER:
		if (p1->t.op==NAME || p1->t.op==STAR) {
#ifdef	i286
			/*
			 * The address of what a based pointer points at is
			 * that pointer, and as near as it is.
			 */
			if (p1->t.op==STAR && (p1->t.tr1->t.op==STOF
			 || p1->t.tr1->t.op==BTOF)) {
				p2 = p1->t.tr1->t.tr1;
				*cp++ = block(PLUS, adrtype(p1), p1->t.subsp,
				    p1->t.strp, p2, cblock(0));
				return;
			}
			*cp++ = block(op,adrtype(p1),p1->t.subsp,p1->t.strp,p1,TNULL);
#else
			*cp++ = block(op,incref(p1->t.type),p1->t.subsp,p1->t.strp,p1,TNULL);
#endif
			return;
		}
		error("Illegal lvalue");
		break;

	/*
	 * a.b goes to (&a)->b
	 */
	case DOT:
#ifdef	i286
		/*
		 * The PDP-11's function gives back the address of the
		 * structure, and the 286's the structure, in registers
		 * or where its caller says (i286/c11.c).  So f().b goes
		 * to (t = f(), &t)->b, t being a temporary.
		 */
		if (p1->t.op==CALL && t1==STRUCT) {
			register struct nmlist *np;

			np = strtemp(p1);
			*cp++ = nblock(np);
			*cp++ = p1;
			build(ASSIGN);
			*cp++ = nblock(np);
			build(AMPER);
			build(SEQNC);
			p1 = *--cp;
		} else {
#else
		if (p1->t.op==CALL && t1==STRUCT) {
			t1 = incref(t1);
			setype(p1, t1, p1);
		} else {
#endif
			*cp++ = p1;
			build(AMPER);
			p1 = *--cp;
		}

	/*
	 * In a->b, a is given the type ptr-to-structure element;
	 * then the offset is added in without conversion;
	 * then * is tacked on to access the member.
	 */
	case ARROW:
		if (p2->t.op!=NAME || p2->t.tr1->n.hclass!=MOS) {
			error("Illegal structure ref");
			*cp++ = p1;
			return;
		}
		p2 = structident(p1, p2);
		t2 = p2->n.htype;
		if (t2==INT && p2->t.tr1->n.hflag&FFIELD)
			t2 = UNSIGN;
#ifdef	i286
		/*
		 * A member reached through a far pointer is far.  And
		 * the member's type is given as it is, not as what a
		 * pointer to it points at: there may be no room in a
		 * type for that pointer.
		 */
		t = ISFAR(p1->t.type)? fincref(t2): ISBASED(p1->t.type)?
		    incref(t2) + (p1->t.type&XTYPE) - PTR: incref(t2);
		chkw(p1, -1);
		setype(p1, t, p2);
		*cp++ = block(PLUS, t, p2->t.subsp, p2->t.strp,
		   p1, cblock(p2->t.tr1->n.hoffset));
		build(STAR);
		cp[-1]->t.type = t2;
#else
		t = incref(t2);
		chkw(p1, -1);
		setype(p1, t, p2);
		*cp++ = block(PLUS, t, p2->t.subsp, p2->t.strp,
		   p1, cblock(p2->t.tr1->n.hoffset));
		build(STAR);
#endif
		if (p2->t.tr1->n.hflag&FFIELD)
			*cp++ = block(FSEL, UNSIGN, (int *)NULL, (union str *)NULL, 
			    *--cp, p2->t.tr1->n.hstrp);
		return;
	}
	if ((dope&LVALUE)!=0)
		chklval(p1);
	if ((dope&LWORD)!=0)
		chkw(p1, LONG);
	if ((dope&RWORD)!=0)
		chkw(p2, LONG);
	if ((t1==VOID && op!=CAST) || (t2==VOID && (op!=CAST || t1!=VOID))) {
		error("Illegal use of void object");
		t = t1 = t2 = INT;
	}
	if ((dope&BINARY)==0) {
		if (op==ITOF)
			t1 = DOUBLE;
		else if (op==FTOI)
			t1 = INT;
		if (!fold(op, p1, (union tree *)NULL))
			*cp++ = block(op, t1, p1->t.subsp, p1->t.strp, p1,TNULL);
		return;
	}
#ifdef	i286
	if (ISFAR(p1->t.type) || ISFAR(p2->t.type)) {
		fbuild(op, p1, p2);
		return;
	}
#endif
	cvn = 0;
	if (t1==STRUCT || t2==STRUCT) {
		if (t1!=t2 || p1->t.strp != p2->t.strp)
			error("Incompatible structures");
		cvn = 0;
	} else
		cvn = cvtab[lintyp(t1)][lintyp(t2)];
	leftc = (cvn>>4)&017;
	cvn &= 017;
	t = leftc? t2:t1;
	if ((t==INT||t==CHAR) && (t1==UNSIGN||t2==UNSIGN))
		t = UNSIGN;
	if (dope&ASSGOP || op==CAST) {
		/*
		 * Weird "lhs op= rhs" requiring a temporary to evaluate as
		 * "lhs = lhs op rhs" so lhs can be converted up for the
		 * operation and then the result converted back down for
		 * the assignment.  As a special sub case, "(int) op= (long)"
		 * doesn't require a temporary except for /= and %= ...
		 */
		if (leftc && op>=ASPLUS && op<=ASXOR
		    && (leftc!=LTI || op==ASDIV || op==ASMOD)) {
			assignop(op, p1, p2);
			return;
		}
		t = t1;
		if (op==ASSIGN) {
			if (cvn==PTI) {
				if (t1 != (PTR|VOID) && t2 != (PTR|VOID))
#ifdef	i286
				/* nor of a far pointer to void */
				if (t1 != (FPTR|VOID) && t2 != (FPTR|VOID))
#endif
				if (t1!=t2 || ((t1&TYPE)==STRUCT && p1->t.strp!=p2->t.strp))
					werror("mixed pointer assignment");
				cvn = leftc = 0;
			} if ((cvn==ITP || cvn==LTP)
			   && (p2->t.op!=CON || p2->c.value!=0)
			   && (p2->t.op!=LCON || p2->l.lvalue!=0)) {
			/*
			 * Allow "i = p" and "p = i" with a warning, where
			 * i is some form of integer (not 0) and p is a
			 * pointer.  Note that in both this patch and the
			 * similar one for "?:" below, code from the CAST
			 * immediately below and the illegal conversion
			 * check farther below is simply stolen.  It would
			 * require either a recursive call to build or a
			 * fairly large rewrite to eliminate the
			 * duplication.
			 */
				werror("illegal combination of pointer and integer, op =");
				if (cvn==ITP)
					cvn = leftc = 0;
				else
					if (leftc==0)
						cvn = LTI;
					else {
						cvn = ITL;
						leftc = 0;
					}
			}
		} else if (op==CAST) {
			if (cvn==ITP||cvn==PTI)
				cvn = leftc = 0;
			else if (cvn==LTP) {
				if (leftc==0)
					cvn = LTI;
				else {
					cvn = ITL;
					leftc = 0;
				}
			}
		}
		if (leftc)
			cvn = leftc;
		leftc = 0;
	} else if (op==COLON) {
		if ((unsigned)t1>=PTR && t1==t2)
			cvn = 0;
		/*
		 * Allow "e ? i : p" and "e ? p : i" with warning.
		 */
		if (cvn==ITP || cvn==LTP) {
			p3 = leftc? p1: p2;
			if ((p3->t.op!=CON || p3->c.value!=0)
			 && (p3->t.op!=LCON || p3->l.lvalue!=0)) {
				werror("illegal combination of pointer and integer, op :");
				if (cvn==ITP)
					cvn = leftc = 0;
				else
					if (leftc==0)
						cvn = LTI;
					else {
						cvn = ITL;
						leftc = 0;
					}
			}
		}
	} else if (dope&RELAT) {
		if (op>=LESSEQ && ((unsigned)t1>=PTR||(unsigned)t2>=PTR||(t1==UNSIGN||t1==UNLONG||t2==UNSIGN||t2==UNLONG)
		 && (t==INT||t==CHAR||t==UNSIGN||t==UNLONG)))
			op += LESSEQP-LESSEQ;
		if (cvn==ITP || cvn==PTI)
			cvn = 0;
	}
	if (cvn==PTI) {
		cvn = 0;
		if (op==MINUS) {
			pcvn++;
			p1 = block(ITOL, LONG, (int *)NULL, (union str *)NULL, p1, TNULL);
			p2 = block(ITOL, LONG, (int *)NULL, (union str *)NULL, p2, TNULL);
			t = LONG;
		} else {
			if (t1!=t2 || (t1!=(PTR+CHAR) && t1!=(PTR+UNCHAR)))
				cvn = XX;
		}
	}
	if (cvn) {
		if ((cvn==ITP || cvn==LTP) && (opdope[op]&PCVOK)==0) {
			p3 = leftc? p1: p2;
			if ((p3->t.op!=CON || p3->c.value!=0)
			 && (p3->t.op!=LCON || p3->l.lvalue!=0))
				cvn = XX;
			else
				cvn = 0;
		}
		t1 = plength(p1);
		t2 = plength(p2);
		if (cvn==XX || (cvn==PTI&&t1!=t2))
			error("Illegal conversion");
		else if (leftc)
			p1 = convert(p1, t, cvn, t2);
		else
			p2 = convert(p2, t, cvn, t1);
	}
	if (dope&RELAT)
		t = INT;
	if (t==FLOAT)
		t = DOUBLE;
	if (t==CHAR)
		t = INT;
	if (op==CAST) {
		if (t!=DOUBLE && (t!=INT || p2->t.type!=CHAR || p2->t.type!=UNCHAR)) {
			p2->t.type = t;
			p2->t.subsp = p1->t.subsp;
			p2->t.strp = p1->t.strp;
		}
		if (t==INT && p1->t.type==CHAR)
			p2 = block(ITOC, INT, (int *)NULL, (union str *)NULL, p2, TNULL);
		*cp++ = p2;
		return;
	}
	if (pcvn)
		t2 = plength(p1->t.tr1);
	if (fold(op, p1, p2)==0) {
		p3 = leftc?p2:p1;
		*cp++ = block(op, t, p3->t.subsp, p3->t.strp, p1, p2);
	}
	if (pcvn) {
		p1 = *--cp;
		*cp++ = convert(p1, 0, PTI, t2);
	}
}

#ifdef	i286
/*
 * Build an operator one of whose operands is a far pointer.  A far
 * pointer is not of the kinds the conversion table knows.  One is had
 * from a near pointer with NTOF, or CTOF if it is to a function, and
 * from an int as a long is; it is a long as it stands, and its offset
 * is the low word of that.  Two are equal if all of them is, but which
 * is the lower is asked of their offsets, and it is their offsets that
 * are subtracted: they are taken to be in one segment, as by the cross
 * compiler.  An int is added to the offset.
 */
fbuild(op, p1, p2)
register union tree *p1, *p2;
{
	register int t1, t2;
	union tree *p;

	t1 = p1->t.type;
	t2 = p2->t.type;
	if (opdope[op]&RELAT && op>=LESSEQ) {
		if (ISFAR(t1))
			p1 = tonear(p1, t1-(FPTR-PTR), p1);
		if (ISFAR(t2))
			p2 = tonear(p2, t2-(FPTR-PTR), p2);
		*cp++ = p1;
		*cp++ = p2;
		build(op);
		return;
	}
	if (opdope[op]&RELAT || op==COLON) {
		if (!ISFAR(t1))
			p1 = tofar(p1, t2, p2);
		else if (!ISFAR(t2))
			p2 = tofar(p2, t1, p1);
		if (op==COLON) {
			*cp++ = block(op, p1->t.type, p1->t.subsp, p1->t.strp,
			    p1, p2);
			return;
		}
		*cp++ = block(op, INT, (int *)NULL, (union str *)NULL, p1, p2);
		return;
	}
	switch (op) {

	case ASSIGN:
	case CAST:
		if (ISFAR(t1)) {
			if (op==ASSIGN && (unsigned)t2<PTR && p2->t.op!=CON
			 && p2->t.op!=LCON)
				werror("illegal combination of pointer and integer, op =");
			p2 = tofar(p2, t1, p1);
			if (op==CAST) {
				p2->t.type = t1;
				p2->t.subsp = p1->t.subsp;
				p2->t.strp = p1->t.strp;
				*cp++ = p2;
			} else
				*cp++ = block(op, t1, p1->t.subsp, p1->t.strp,
				    p1, p2);
			return;
		}
		/*
		 * A far pointer to something else: a long of it, or its
		 * offset, and then as that would be.
		 */
		if (t1==LONG || t1==UNLONG)
			p2->t.type = UNLONG;
		else if (t1==FLOAT || t1==DOUBLE || t1==STRUCT)
			goto bad;
		else if ((unsigned)t1>=PTR) {
			if (op==ASSIGN)
				werror("far pointer made near");
			p2 = tonear(p2, t1, p1);
		} else
			p2 = tonear(p2, INT, p2);
		*cp++ = p1;
		*cp++ = p2;
		build(op);
		return;

	case MINUS:
		if ((unsigned)t2>=PTR) {
			if (ISFAR(t1))
				p1 = tonear(p1, t1-(FPTR-PTR), p1);
			if (ISFAR(t2))
				p2 = tonear(p2, t2-(FPTR-PTR), p2);
			*cp++ = p1;
			*cp++ = p2;
			build(op);
			return;
		}
		break;

	case PLUS:
		if (!ISFAR(t1)) {
			p = p1;
			p1 = p2;
			p2 = p;
			t1 = p1->t.type;
			t2 = p2->t.type;
		}
		break;

	case ASPLUS:
	case ASMINUS:
	case INCBEF:
	case DECBEF:
	case INCAFT:
	case DECAFT:
		break;

	default:
		goto bad;
	}
	if (!ISFAR(t1))
		goto bad;
	if (t2==LONG || t2==UNLONG)
		p2 = block(LTOI, INT, (int *)NULL, (union str *)NULL, p2, TNULL);
	else if (t2!=INT && t2!=CHAR && t2!=UNSIGN && t2!=UNCHAR)
		goto bad;
	p2 = convert(p2, INT, ITP, plength(p1));
	*cp++ = block(op, t1, p1->t.subsp, p1->t.strp, p1, p2);
	return;
bad:
	error("Illegal operation on a far pointer");
	*cp++ = p1;
}

/*
 * Make p a far pointer of the type t, which is q's.
 */
union tree *
tofar(p, t, q)
register union tree *p, *q;
{
	register int pt;
	register union tree *lp;

	pt = p->t.type;
	if (ISFAR(pt))
		return(p);
	if (p->t.op==CON && (p->c.value==0 || (unsigned)pt<PTR)) {
		lp = (union tree *)Tblock(sizeof(struct lnode));
		lp->l.op = LCON;
		lp->l.lvalue = (unsigned short)p->c.value;
		p = lp;
	} else if ((pt&XTYPE)==SPTR || (pt&XTYPE)==BPTR)
		return(block((pt&XTYPE)==SPTR? STOF: BTOF, t,
		    q->t.subsp, q->t.strp, p, TNULL));
	else if ((unsigned)pt>=PTR)
		return(block((decref(pt)&XTYPE)==FUNC? CTOF: NTOF, t,
		    q->t.subsp, q->t.strp, p, TNULL));
	else if (pt==INT || pt==CHAR || pt==UNSIGN || pt==UNCHAR)
		return(block(ITOL, t, q->t.subsp, q->t.strp, p, TNULL));
	else if (pt!=LONG && pt!=UNLONG) {
		error("Illegal conversion");
		return(p);
	}
	p->t.type = t;
	p->t.subsp = q->t.subsp;
	p->t.strp = q->t.strp;
	return(p);
}

/*
 * The offset of the far pointer p, as something of the type t, which
 * is q's.
 */
union tree *
tonear(p, t, q)
union tree *p, *q;
{
	return(block(LTOI, t, q->t.subsp, q->t.strp, p, TNULL));
}

/*
 * A far variable, which is not in the data segment: what the far
 * pointer to it points at.  That is its near address made far with
 * XTOF, whose segment is the variable's own, which the second pass
 * has from the name.
 */
union tree *
farname(cs)
register struct nmlist *cs;
{
	register union tree *p, *a;

	if ((cs->htype&XTYPE)==FUNC) {
		error("%s: a far function in the small model", cs->name);
		return(nblock(cs));
	}
	p = nblock(cs);
	if (cs->hflag&FBASED) {
		/* in the based segment: the address is a based pointer */
		a = block(AMPER, incref(p->t.type) + (BPTR-PTR), p->t.subsp,
		    p->t.strp, p, TNULL);
		a = basedfar(a);
	} else {
		a = block(AMPER, incref(p->t.type), p->t.subsp, p->t.strp, p,
		    TNULL);
		a = block(XTOF, fincref(p->t.type), p->t.subsp, p->t.strp, a,
		    TNULL);
	}
	return(block(STAR, p->t.type, p->t.subsp, p->t.strp, a, TNULL));
}

/*
 * A pointer into SS, or into the based segment, made far.
 */
union tree *
basedfar(p)
register union tree *p;
{
	register int t;

	t = p->t.type;
	return(block((t&XTYPE)==SPTR? STOF: BTOF, t + (FPTR-(t&XTYPE)),
	    p->t.subsp, p->t.strp, p, TNULL));
}
#endif

union tree *
structident(p1, p2)
register union tree *p1, *p2;
{
	register struct nmlist *np;
	int vartypes = 0, namesame = 1;

	np = (struct nmlist *)p2->t.tr1;
	for (;;) {
#ifdef	i286
		if (namesame && (p1->t.type==STRUCT+PTR || p1->t.type==STRUCT+FPTR
		 || p1->t.type==STRUCT+SPTR || p1->t.type==STRUCT+BPTR)
		 && p1->t.strp == np->sparent) {
#else
		if (namesame && p1->t.type==STRUCT+PTR && p1->t.strp == np->sparent) {
#endif
			p2->t.type = np->htype;
			p2->t.strp = np->hstrp;
			p2->t.subsp = np->hsubsp;
			p2->t.tr1 = (union tree *)np;
			return(p2);
		}
		np = np->nextnm;
		if (np==NULL)
			break;
		namesame = 0;
		if (strcmp(p2->t.tr1->n.name, np->name) != 0)
			continue;
		if ((p2->t.tr1->n.hflag&FKIND) != (np->hflag&FMOS))
			continue;
		namesame = 1;
		if (p2->t.tr1->n.htype==np->htype && p2->t.tr1->n.hoffset==np->hoffset)
			continue;
		vartypes++;
	}
	if (vartypes)
		error("Ambiguous structure reference for %s",p2->t.tr1->n.name);
	else
		werror("%s not member of cited struct/union",p2->t.tr1->n.name);
	return(p2);
}

/*
 * Generate the appropriate conversion operator.
 */
union tree *
convert(p, t, cvn, len)
union tree *p;
{
	register int op;

	if (cvn==0)
		return(p);
	op = cvntab[cvn];
	if (opdope[op]&BINARY) {
		if (len==0)
			error("Illegal conversion");
		return(block(op, t, (int *)NULL, (union str *)NULL, p, cblock(len)));
	}
	return(block(op, t, (int *)NULL, (union str *)NULL, p, TNULL));
}

/*
 * Traverse an expression tree, adjust things
 * so the types of things in it are consistent
 * with the view that its top node has
 * type at.
 * Used with structure references.
 */
setype(p, t, newp)
register union tree *p, *newp;
register t;
{
	for (;; p = p->t.tr1) {
		p->t.subsp = newp->t.subsp;
		p->t.strp = newp->t.strp;
		p->t.type = t;
		if (p->t.op==AMPER)
			t = decref(t);
		else if (p->t.op==STAR)
#ifdef	i286
			/* what is under it is yet to be given its type */
			t = ISFAR(p->t.tr1->t.type)? fincref(t): incref(t);
#else
			t = incref(t);
#endif
		else if (p->t.op!=PLUS)
			break;
	}
}

/*
 * A mention of a function name is turned into
 * a pointer to that function.
 */
union tree *
chkfun(p)
register union tree *p;
{
	register int t;

	if (((t = p->t.type)&XTYPE)==FUNC && p->t.op!=ETYPE)
#ifdef	i286
		return(block(AMPER,adrtype(p),p->t.subsp,p->t.strp,p,TNULL));
#else
		return(block(AMPER,incref(t),p->t.subsp,p->t.strp,p,TNULL));
#endif
	return(p);
}

/*
 * A mention of an array is turned into
 * a pointer to the base of the array.
 */
union tree *
disarray(p)
register union tree *p;
{
	register int t;

	if (p==NULL)
		return(p);
	/* check array & not MOS and not typer */
	if (((t = p->t.type)&XTYPE)!=ARRAY
	 || p->t.op==NAME && p->t.tr1->n.hclass==MOS
	 || p->t.op==ETYPE)
		return(p);
	p->t.subsp++;
	*cp++ = p;
	setype(p, decref(t), p);
	build(AMPER);
	return(*--cp);
}

/*
 * make sure that p is a ptr to a node
 * with type int or char or 'okt.'
 * okt might be nonexistent or 'long'
 * (e.g. for <<).
 */
chkw(p, okt)
union tree *p;
{
	register int t = p->t.type;

	if (t == UNLONG)
		t = LONG;
	if (t!=INT && (unsigned)t<PTR && t!=CHAR && t!=UNCHAR && t!=UNSIGN && t!=okt)
		error("Illegal type of operand");
	return;
}

/*
 *'linearize' a type for looking up in the
 * conversion table
 */
lintyp(t)
{
	switch(t) {

	case INT:
	case CHAR:
	case UNSIGN:
	case UNCHAR:
		return(0);

	case FLOAT:
	case DOUBLE:
		return(1);

	case UNLONG:
	case LONG:
		return(2);

	default:
		return(3);
	}
}

#ifdef	i286
/*
 * A pointer into SS, as the address of what is in the frame is where SS
 * is not DS (-S), made an ordinary one, which is taken to be into DS:
 * by a cast, an assignment, a return, or an argument that a prototype
 * says is of another kind.  Or passed where no prototype says what it
 * is to be, which the function called takes as it takes it.  Either is
 * wrong unless the pointer is for code whose DS is SS, as the kernel's
 * is to the network's, or is followed where SS is reached, and nothing
 * shows it in the code; so where SS is not DS each is said, with the
 * function it is in, and tools/netcheck.py counts them in the kernel's
 * network.
 */
ssnear()
{
	if (ssflag)
		werror("pointer into SS made near, in %s",
		    funcsym? funcsym->name: "?");
}

/*
 * The arguments of a call that no prototype checks.
 */
ssargs(p)
register union tree *p;
{
	if (p->t.op==COMMA) {
		ssargs(p->t.tr1);
		ssargs(p->t.tr2);
	} else if ((p->t.type&XTYPE)==SPTR)
		ssnear();
}
#endif

/*
 * Report an error.
 */

extern int Wflag = 0;	/* Non-zero means do not print warnings */

/* VARARGS1 */
werror(s, p1, p2, p3, p4, p5, p6)
char *s;
{
	if (Wflag)
		return;
	if (filename[0])
		fprintf(stderr, "%s:", filename);
	fprintf(stderr, "%d: warning: ", line);
	fprintf(stderr, s, p1, p2, p3, p4, p5, p6);
	fprintf(stderr, "\n");
}

/* VARARGS1 */
error(s, p1, p2, p3, p4, p5, p6)
char *s;
{
	nerror++;
	if (filename[0])
		fprintf(stderr, "%s:", filename);
	fprintf(stderr, "%d: ", line);
	fprintf(stderr, s, p1, p2, p3, p4, p5, p6);
	fprintf(stderr, "\n");
}

/*
 * Generate a node in an expression tree,
 * setting the operator, type, dimen/struct table ptrs,
 * and the operands.
 */
union tree *
block(op, t, subs, str, p1,p2)
int *subs;
union str *str;
union tree *p1, *p2;
{
	register union tree *p;

	p = (union tree *)Tblock(sizeof(struct tnode));
	p->t.op = op;
	p->t.type = t;
	p->t.subsp = subs;
	p->t.strp = str;
	p->t.tr1 = p1;
	if (opdope[op]&BINARY)
		p->t.tr2 = p2;
	else
		p->t.tr2 = NULL;
	return(p);
}

union tree *
nblock(ds)
register struct nmlist *ds;
{
	return(block(NAME, ds->htype, ds->hsubsp, ds->hstrp, (union tree *)ds, TNULL));
}

/*
 * Generate a block for a constant
 */
union tree *
cblock(v)
{
	register union tree *p;

	p = (union tree *)Tblock(sizeof(struct cnode));
	p->c.op = CON;
	p->c.type = INT;
	p->c.subsp = NULL;
	p->c.strp = NULL;
	p->c.value = v;
	return(p);
}

/*
 * A block for a float constant
 */
union tree *
fblock(t, string)
char *string;
{
	register union tree *p;

	p = (union tree *)Tblock(sizeof(struct fnode));
	p->f.op = FCON;
	p->f.type = t;
	p->f.subsp = NULL;
	p->f.strp = NULL;
	p->f.cstr = string;
	return(p);
}

/*
 * Assign a block for use in the
 * expression tree.
 */
char *
Tblock(n)
{
	register char *p;

	p = treebase;
	if (p==NULL) {
		error("c0 internal error: Tblock");
		exit(1);
	}
	if ((treebase += n) >= coremax) {
		if (sbrk(1024) == (char *)-1) {
			error("Out of space");
			exit(1);
		}
		coremax += 1024;
	}
	return(p);
}

char *
starttree()
{
	register char *st;

	st = treebase;
	if (st==NULL)
		treebot = treebase = locbase+DCLSLOP;
	return(st);
}

endtree(tp)
char *tp;
{
	treebase = tp;
	if (tp==NULL)
		treebot = NULL;
}

/*
 * Assign a block for use in a declaration
 */
char *
Dblock(n)
{
	register char *p;

	p = locbase;
	locbase += n;
	if (treebot && locbase > treebot) {
		error("Too much declaring in an expression");
		exit(1);
	}
	if (locbase > coremax) {
		if (sbrk(1024) == (char *)-1) {
			error("out of space");
			exit(1);
		}
		coremax += 1024;
	}
	return(p);
}

/*
 * Check that a tree can be used as an lvalue.
 */
chklval(p)
register union tree *p;
{
	if (p->t.op==FSEL)
		p = p->t.tr1;
	if (p->t.op!=NAME && p->t.op!=STAR)
		error("Lvalue required");
}

/*
 * reduce some forms of `constant op constant'
 * to a constant.  More of this is done in the next pass
 * but this is used to allow constant expressions
 * to be used in switches and array bounds.
 */
fold(op, p1, p2)
register union tree *p1;
union tree *p2;
{
	register int v1, v2;
	int unsignf;

	if (p1->t.op!=CON)
		return(0);
	unsignf = p1->c.type==UNSIGN;
	unsignf |= p1->c.type==UNLONG;
	if (op==QUEST) {
		if (p2->t.tr1->t.op==CON && p2->t.tr2->t.op==CON) {
			p1->c.value = p1->c.value? p2->t.tr1->c.value: p2->t.tr2->c.value;
			*cp++ = p1;
			p1->t.type = p2->t.type;
			return(1);
		}
		return(0);
	}
	if (p2) {
		if (p2->t.op!=CON)
			return(0);
		v2 = p2->c.value;
		unsignf |= p2->c.type==UNSIGN;
		unsignf |= p2->c.type==UNLONG;
	}
	v1 = p1->c.value;
	/*
	 * The arithmetic is the 286's, in 16 bits, where the compiler's own
	 * int is wider (tools/ncc), as Xconst has it in the next pass.
	 */
	if (unsignf) {
		v1 = (unsigned short)v1;
		if (p2 && op!=LSHIFT && op!=RSHIFT)
			v2 = (unsigned short)v2;
	} else {
		v1 = (short)v1;
		if (p2)
			v2 = (short)v2;
	}
	switch (op) {

	case PLUS:
		v1 += v2;
		break;

	case MINUS:
		v1 -= v2;
		break;

	case TIMES:
		v1 *= v2;
		break;

	case DIVIDE:
		if (v2==0)
			goto divchk;
		if (unsignf) {
			if (v2==1)
				break;
			if (v2<0) {
				v1 = (unsigned)v1 >= (unsigned)v2;
				break;
			}
			v1 = (unsigned)v1 / v2;
			break;
		}
		v1 /= v2;
		break;

	case MOD:
		if (v2==0)
			goto divchk;
		if (unsignf) {
			if (v2==1) {
				v1 = 0;
				break;
			}
			if (v2<0) {
				if ((unsigned)v1 >= (unsigned)v2)
					v1 -= v2;
				break;
			}
			v1 = (unsigned)v1 % v2;
			break;
		}
		v1 %= v2;
		break;

	case AND:
		v1 &= v2;
		break;

	case OR:
		v1 |= v2;
		break;

	case EXOR:
		v1 ^= v2;
		break;

	case NEG:
		v1 = - v1;
		break;

	case COMPL:
		v1 = ~ v1;
		break;

	case LSHIFT:
		v1 <<= v2;
		break;

	case RSHIFT:
		if (unsignf) {
			v1 = (unsigned)v1 >> v2;
			break;
		}
		v1 >>= v2;
		break;

	case EQUAL:
		v1 = v1==v2;
		break;

	case NEQUAL:
		v1 = v1!=v2;
		break;

	case LESS:
		v1 = v1<v2;
		break;

	case GREAT:
		v1 = v1>v2;
		break;

	case LESSEQ:
		v1 = v1<=v2;
		break;

	case GREATEQ:
		v1 = v1>=v2;
		break;

	case LESSP:
		v1 = (unsigned)v1<v2;
		break;

	case GREATP:
		v1 = (unsigned)v1>v2;
		break;

	case LESSEQP:
		v1 = (unsigned)v1<=v2;
		break;

	case GREATQP:
		v1 = (unsigned)v1>=v2;
		break;

	divchk:
		error("Divide check");
		nerror--;
	default:
		return(0);
	}
	p1->c.value = (short)v1;
	*cp++ = p1;
	if (unsignf)
		p1->t.type = UNSIGN;
	return(1);
}

/*
 * Compile an expression expected to have constant value,
 * for example an array bound or a case value.
 */
conexp()
{
	register union tree *t;
#ifdef	i286
	LTYPE v;
#endif

	initflg++;
	if (t = tree(1))
		if (t->t.op != CON) {
#ifdef	i286
			/*
			 * The 286's headers count bytes in longs: the
			 * size of the user structure is ctob(USIZE).
			 */
			if (lconval(t, &v)) {
				initflg--;
				return((int)v);
			}
#endif
			error("Constant required");
		}
	initflg--;
	return(t->c.value);
}

#ifdef	i286
/*
 * The value of a tree that is a constant but a long one, which is not
 * made one constant until the second pass.  Returns 0 if it is not.
 */
lconval(t, vp)
register union tree *t;
LTYPE *vp;
{
	LTYPE a, b;

	if (t==NULL)
		return(0);
	switch (t->t.op) {

	case CON:
		if (t->c.type==UNSIGN)
			*vp = (unsigned short)t->c.value;
		else
			*vp = (short)t->c.value;
		return(1);

	case LCON:
		*vp = t->l.lvalue;
		return(1);

	case ITOL:
		return(lconval(t->t.tr1, vp));

	case LTOI:
		if (!lconval(t->t.tr1, &a))
			return(0);
		*vp = (short)a;
		return(1);

	case NEG:
		if (!lconval(t->t.tr1, &a))
			return(0);
		*vp = -a;
		return(1);

	case PLUS:
	case MINUS:
	case TIMES:
	case DIVIDE:
	case LSHIFT:
	case RSHIFT:
	case AND:
	case OR:
		if (!lconval(t->t.tr1, &a) || !lconval(t->t.tr2, &b))
			return(0);
		switch (t->t.op) {

		case PLUS:
			a += b;
			break;

		case MINUS:
			a -= b;
			break;

		case TIMES:
			a *= b;
			break;

		case DIVIDE:
			if (b==0)
				return(0);
			a /= b;
			break;

		case LSHIFT:
			a <<= b;
			break;

		case RSHIFT:
			a >>= b;
			break;

		case AND:
			a &= b;
			break;

		case OR:
			a |= b;
			break;
		}
		*vp = a;
		return(1);
	}
	return(0);
}
#endif

/*
 * Handle peculiar assignment ops that need a temporary.
 */
assignop(op, p1, p2)
register union tree *p1, *p2;
{
	register struct nmlist *np;

	op += PLUS - ASPLUS;
	if (p1->t.op==NAME) {
		*cp++ = p1;
		*cp++ = p1;
		*cp++ = p2;
		build(op);
		build(ASSIGN);
		return;
	}
#ifdef	i286
	np = gentemp(adrtype(p1));
#else
	np = gentemp(incref(p1->t.type));
#endif
	*cp++ = nblock(np);
	*cp++ = p1;
	build(AMPER);
	build(ASSIGN);
	*cp++ = nblock(np);
	build(STAR);
	*cp++ = nblock(np);
	build(STAR);
	*cp++ = p2;
	build(op);
	build(ASSIGN);
	build(SEQNC);
}

/*
 * Generate an automatic temporary for
 * use in certain assignment ops
 */
struct nmlist *
gentemp(type)
{
	register struct nmlist *tp;

	tp = (struct nmlist *)Tblock(sizeof(struct nmlist));
	tp->hclass = AUTO;
	tp->htype = type;
	tp->hflag = 0;
	tp->hsubsp = NULL;
	tp->hstrp = NULL;
	tp->hblklev = blklev;
	autolen -= rlength((union tree *)tp);
	tp->hoffset = autolen;
	if (autolen < maxauto)
		maxauto = autolen;
	return(tp);
}

#ifdef	i286
/*
 * Generate an automatic temporary for the structure
 * that a function returns
 */
struct nmlist *
strtemp(p)
register union tree *p;
{
	register struct nmlist *tp;

	tp = (struct nmlist *)Tblock(sizeof(struct nmlist));
	tp->hclass = AUTO;
	tp->htype = p->t.type;
	tp->hflag = 0;
	tp->hsubsp = p->t.subsp;
	tp->hstrp = p->t.strp;
	tp->hblklev = blklev;
	autolen -= rlength((union tree *)tp);
	tp->hoffset = autolen;
	if (autolen < maxauto)
		maxauto = autolen;
	return(tp);
}
#endif
