/*
 *	C compiler, part 2, for the 80286: the code generator
 *
 * This takes the place of the PDP-11 compiler's c10.c and of its code
 * tables (../c10.c, ../optable).  Those tables are written for a
 * machine whose registers are all alike, and any of which will hold a
 * value, index memory or be half of a long.  The 286's are not:
 *
 *	AX	the value of an expression; with DX above it, a long
 *	BX	the address of whatever is reached through a pointer
 *	CX	the second operand, when it cannot be used where it is;
 *		the count of a shift; with BX below it, a second long
 *	DX	the high word of a long, of a product and of a dividend
 *	SI, DI	register variables
 *	BP	the frame
 *
 * So there is no choosing of registers here.  An expression is
 * compiled into AX, and what makes the code good or bad is how its
 * operands are reached.  An operand may be:
 *
 *	one as it stands (adrble): a constant, an address, a variable,
 *	    which an instruction takes directly;
 *	easy: computable in any one register with nothing else disturbed
 *	    but BX, as a sum of such things is, or what a simple pointer
 *	    points at (greg);
 *	anything else, which is compiled into AX, and saved on the stack
 *	    if AX is wanted before it is used.
 *
 * What is reached through a pointer is made an operand by putting the
 * pointer in BX; the tree for it is then rewritten in place as the
 * variable [bx+n] (bxform, gaddr).
 *
 * A far pointer is a long to everything here but the few operators
 * that are its own (c1.h).  What is reached through one is made an
 * operand by putting the pointer in ES:BX, and is then the variable
 * [es:bx+n].  ES is no more lasting than BX: whatever leaves BX alone
 * leaves it alone.
 *
 * Floating point is the 287's, and in c14.c.
 *
 * The output is for NASM.  Data is near.  Functions are near in the
 * small model, and with -M, the medium model, far: each file's are
 * then in a segment of its own, named by what follows the -M.
 */

#include "c1.h"

char	maprel[] = {	EQUAL, NEQUAL, GREATEQ, GREAT, LESSEQ,
			LESS, GREATQP, GREATP, LESSEQP, LESSP
};

char	notrel[] = {	NEQUAL, EQUAL, GREAT, GREATEQ, LESS,
			LESSEQ, GREATP, GREATQP, LESSP, LESSEQP
};

struct tconst czero = { CON, INT, 0};
struct tconst cone  = { CON, INT, 1};

char	*rname[] = { "ax", "bx", "cx", "di", "si", "bp", "dx" };
char	*bname[] = { "al", "bl", "cl", "?", "?", "?", "dl" };

int	isn	= 10000;

main(argc, argv)
int	argc;
char	*argv[];
{
	char	buf1[BUFSIZ],
		buf2[BUFSIZ];

	if (argc<4) {
		error("Arg count");
		exit(1);
	}
	textseg = "_TEXT";
	if (argc>4 && argv[4][0]=='-' && argv[4][1]=='M') {
		mflag++;
		textseg = argv[4]+2;
	}
	if (freopen(argv[1], "r", stdin)==NULL) {
		error("Missing temp file");
		exit(1);
	}
	setbuf(stdin,buf1);	/* sbrk problems */
	if ((freopen(argv[3], "w", stdout)) == NULL) {
		error("Can't create the output");
		exit(1);
	}
	setbuf(stdout,buf2);	/* sbrk problems */
	funcbase = curbase = coremax = sbrk(0);
	printf("\tbits\t16\n\tcpu\t286\n");
	printf("\tsegment\t%s public align=2 class=CODE\n", textseg);
	printf("\tsegment\t_DATA public align=2 class=DATA\n");
	printf("\tsegment\t_BSS public align=2 class=BSS\n");
	printf("\tgroup\tDGROUP _DATA _BSS\n");
	getree();
	/*
	 * tack on the string file.
	 */
	sect(DATA);
	if (*argv[2] != '-') {
		if (freopen(argv[2], "r", stdin)==NULL) {
			error("Missing temp file");
			exit(1);
		}
		setbuf(stdin,buf1);	/* sbrk problems */
		getree();
	}
	xrefs();
	if (totspace >= (unsigned)56000)
		werror("possibly too much data");
	exit(nerror!=0);
}

/*
 * Compile the tree for the context ctx:
 *	FOREFF	for what it does
 *	FORREG	for its value, which is left in AX, or in DX:AX if long
 *	FORCC	for the flags, as its value against zero sets them or,
 *		if it is a comparison, as that does
 *	FORSP	for its value, which is pushed
 * AX, BX, CX, DX and the flags are free to it; what it pushes it pops.
 */
rcexpr(atree, ctx)
union tree *atree;
{
	register union tree *tree;
	register r;

again:
	if ((tree = atree)==NULL)
		return;
	if (tree->t.type==VOID) {
		if (ctx!=FOREFF)
			error("Illegal use of void");
		tree->t.type = INT;
	}
	/*
	 * fieldselect(...) : if only the effect is wanted,
	 * ignore the select, otherwise
	 * do the shift and mask.
	 */
	if (tree->t.op == FSELT) {
		if (ctx==FOREFF)
			atree = tree = tree->t.tr1;
		else {
			tree->t.op = FSEL;
			atree = tree = optim(tree);
		}
	}
	switch (tree->t.op) {

	/*
	 * Structure assignments
	 */
	case STRASG:
		strasg(tree);
		return;

	/*
	 * An initializing expression
	 */
	case INIT:
		tree = optim(tree);
		doinit(tree->t.type, tree->t.tr1);
		return;

	/*
	 * Put the value of an expression in AX or DX:AX,
	 * for a switch or a return
	 */
	case RFORCE:
		rcexpr(tree->t.tr1, FORREG);
		return;

	/*
	 * sequential execution
	 */
	case SEQNC:
		rcexpr(tree->t.tr1, FOREFF);
		atree = tree->t.tr2;
		goto again;

	/*
	 * Try to change * to shift.
	 */
	case TIMES:
	case ASTIMES:
		atree = tree = pow2(tree);
	}
	/*
	 * Try to find postfix ++ and -- operators that can be
	 * pulled out and done after the rest of the expression.
	 * They are done with instructions that change the flags.
	 */
	if (ctx!=FORCC && (opdope[tree->t.op]&LEAF)==0) {
		if (delay(&atree, ctx)) {
			tree = atree;
			ctx = FOREFF;
		}
	}
	r = gexpr(tree, ctx);
	if (r==DONE)
		return;
	switch (ctx) {

	case FORSP:
		if (LNG(tree->t.type))
			printf("\tpush\tdx\n");
		printf("\tpush\tax\n");
		return;

	case FORCC:
		if (r==INREG) {
			printf("\ttest\tax,ax\n");
			r = CCLOGIC;
		}
		ccof = r==CCARITH;
		return;
	}
}

/*
 * Compile the tree for the context.  The value is left in AX or DX:AX
 * unless DONE is returned, which says that there is nothing more for
 * rcexpr to do, or the context is FORCC and what is returned says the
 * flags are set.
 */
gexpr(tree, ctx)
register union tree *tree;
{
	register union tree *l, *r;
	register op;
	int type, lbl, lbl2, n;

	op = tree->t.op;
	type = tree->t.type;
	/*
	 * When the value of a relational or a logical expression is
	 * desired, more work must be done.
	 */
	if ((opdope[op]&RELAT || op==LOGAND || op==LOGOR || op==EXCLA)
	 && ctx!=FORCC) {
		cbranch(tree, lbl = isn++, 1);
		printf("\txor\tax,ax\n");
		branch(lbl2 = isn++, 0, 0);
		label(lbl);
		printf("\tmov\tax,1\n");
		label(lbl2);
		return(INREG);
	}
	/*
	 * Floating point is the 287's (c14.c).  cbranch has taken the
	 * comparisons.
	 */
	if (isfloat(tree)) {
		if (opdope[op]&RELAT)
			error("Compiler error: gexpr");
		return(gfloat(tree, ctx));
	}
	if (opdope[op]&RELAT)
		return(grel(tree));
	l = tree->t.tr1;
	if (opdope[op]&BINARY)
		r = tree->t.tr2;
	switch (op) {

	case QUEST:
		if (ctx!=FOREFF)
			ctx = FORREG;
		cbranch(l, lbl = isn++, 0);
		rcexpr(r->t.tr1, ctx);
		branch(lbl2 = isn++, 0, 0);
		label(lbl);
		rcexpr(r->t.tr2, ctx);
		label(lbl2);
		return(ctx==FOREFF? DONE: INREG);

	/*
	 * Operands as they stand
	 */
	case AMPER:
		if (!adrble(tree))
			break;
	case NAME:
	case CON:
	case LCON:
		return(gleaf(tree, ctx));

	case STAR:
	case FSTAR:
	case AUTOI:
	case AUTOD:
		gaddr(tree);
		return(gleaf(tree, ctx));

	/*
	 * A far pointer and an int, which is added to its offset.
	 */
	case FPLUS:
	case FMINUS:
		if (ctx==FOREFF) {
			rcexpr(l, FOREFF);
			rcexpr(r, FOREFF);
			return(DONE);
		}
		faradd(tree);
		return(INREG);

	case ASFPLUS:
	case ASFMINUS:
		return(gasgfar(tree, ctx));

	/*
	 * A near pointer made far: the segment is the data's, or the
	 * code's if it points at a function.  That is so of a null
	 * pointer too, as it is with the cross compiler: only the
	 * constant 0 is nowhere as a far pointer, and the first pass
	 * has seen to that.
	 */
	case NTOF:
	case CTOF:
		rcexpr(l, FORREG);
		printf("\tmov\tdx,%s\n", op==NTOF? "ds": "cs");
		return(INREG);

	/*
	 * Conversions
	 */
	case LOAD:
		rcexpr(l, FORREG);
		return(INREG);

	case ITOC:
		rcexpr(l, FORREG);
		printf("\tcbw\n");
		return(INREG);

	case LTOI:
		rcexpr(l, FORREG);
		return(INREG);

	case FTOI:
	case FTOL:
		ffix(tree);
		return(INREG);

	case ITOL:
		if (adrble(tree))
			return(gleaf(tree, ctx));
		rcexpr(l, FORREG);
		printf(uns(l)? "\txor\tdx,dx\n": "\tcwd\n");
		return(INREG);

	/*
	 * The difference of two pointers, a long, as a count of
	 * what they point at: it is divided by the size.
	 */
	case PTOI:
		rcexpr(l, FORREG);
		n = r->c.value;
		if (r->t.op==CON && (n==2 || n==4 || n==8)) {
			while (n >>= 1)
				printf("\tsar\tdx,1\n\trcr\tax,1\n");
		} else {
			insrp("mov", CX, r, 0);
			printf("\tidiv\tcx\n");
		}
		return(INREG);

	case NEG:
		rcexpr(l, FORREG);
		if (LNG(type)) {
			printf("\tneg\tdx\n\tneg\tax\n\tsbb\tdx,0\n");
			return(INREG);
		}
		printf("\tneg\tax\n");
		return(CCARITH);

	case COMPL:
		rcexpr(l, FORREG);
		if (LNG(type))
			printf("\tnot\tdx\n");
		printf("\tnot\tax\n");
		return(INREG);

	/*
	 * Only the low byte of what a function of type char returns
	 * is its value: the cross compiler's functions leave the rest
	 * of AX as it happens to be.
	 */
	case CALL:
		gcall(tree, TNULL, 0);
		if (ctx!=FOREFF)
			stored(type);
		return(INREG);

	case PLUS:
		if (!LNG(type) && lea(tree, AX))
			return(INREG);
	case MINUS:
	case AND:
	case OR:
	case EXOR:
		if (LNG(type)) {
			larith(op, loperands(tree));
			return(INREG);
		}
		if (op==AND && ctx==FORCC)
			return(gtest(tree));
		if (r = operands(tree))
			return(arith(op, AX, r));
		printf("\t%s\tax,cx\n", opname(op));
		return(op==PLUS||op==MINUS? CCARITH: CCLOGIC);

	case TIMES:
	case DIVIDE:
	case MOD:
	case UDIV:
	case UMOD:
		mulop(op, operands(tree));
		return(INREG);

	case LSHIFT:
	case RSHIFT:
	case ULSH:
		if (LNG(type)) {
			lshift(op, l, r, (union tree *)NULL);
			return(INREG);
		}
		mulop(op, operands(tree));
		return(INREG);

	case LTIMES:
	case LDIV:
	case LMOD:
	case ULTIMES:
	case ULDIV:
	case ULMOD:
		lmulop(op, loperands(tree));
		return(INREG);

	case ASSIGN:
		return(gassign(tree, ctx));

	case ASPLUS:
	case ASMINUS:
	case ASAND:
	case ASOR:
	case ASXOR:
	case INCBEF:
	case DECBEF:
	case INCAFT:
	case DECAFT:
		return(gasgop(tree, ctx));

	case ASTIMES:
	case ASDIV:
	case ASMOD:
	case ASUDIV:
	case ASUMOD:
	case ASLSH:
	case ASRSH:
	case ASULSH:
	case LASTIMES:
	case LASDIV:
	case LASMOD:
	case ULASTIMES:
	case ULASDIV:
	case ULASMOD:
		return(gasgop2(tree, ctx));

	/*
	 * Assignment to a field: the word it is in, the value in its
	 * place there, and the mask for it.
	 */
	case FSELA:
		gvalue(l, r);
		n = CHR(l->t.type)? BYT: 0;
		inspn("and", l, n, ~tree->F.mask);
		inspr("or", l, n, AX);
		return(INREG);
	}
	if (type==STRUCT)
		error("Illegal operation on structure");
	else {
		error("No code for op %d type %d", op, type);
		if (op>0 && op<=MAXOP && opntab[op])
			fprintf(stderr, "\t(%s)\n", opntab[op]);
	}
	return(DONE);
}

/*
 * An operand that needs no computing: push it, test it or load it.
 */
gleaf(p, ctx)
register union tree *p;
{
	register type;

	type = p->t.type;
	switch (ctx) {

	case FOREFF:
		return(DONE);

	case FORSP:
		if (CHR(type))
			break;
		if (LNG(type))
			ins1("push", p, HI);
		ins1("push", p, 0);
		return(DONE);

	case FORCC:
		if (p->t.op!=NAME || LNG(type) || type==UNCHAR)
			break;
		if (p->n.class==REG) {
			insrp("test", p->n.nloc, p, 0);
			return(CCLOGIC);
		}
		inspn("cmp", p, type==CHAR? BYT: 0, 0);
		return(CCLOGIC);
	}
	load(p, AX);
	return(INREG);
}

/*
 * A comparison, for the flags.  One with zero is the flags of what is
 * compared; cbranch knows which.
 */
grel(tree)
register union tree *tree;
{
	register union tree *l, *r;

	l = tree->t.tr1;
	r = tree->t.tr2;
	if (iszero(r)) {
		/*
		 * If what is compared with zero is itself true or
		 * false, it is its value that is compared, and the
		 * flags it leaves are of something else.
		 */
		if (opdope[l->t.op]&RELAT || l->t.op==LOGAND
		 || l->t.op==LOGOR || l->t.op==EXCLA) {
			rcexpr(l, FORREG);
			return(INREG);
		}
		rcexpr(l, FORCC);
		return(DONE);
	}
	/*
	 * A variable and a constant are compared in place.
	 */
	if ((l->t.op==NAME || IND(l)) && !LNG(l->t.type)
	 && !isfloat(l)) {
		if (CHR(l->t.type)) {
			if (r->t.op==CON && r->t.type==CHAR) {
				if (IND(l))
					gaddr(l);
				inspn("cmp", l, BYT, r->c.value);
				return(DONE);
			}
		} else if (isimm(r, INT)) {
			if (IND(l))
				gaddr(l);
			printf("\tcmp\t");
			pname(l, 0);
			putchar(',');
			pname(r, 0);
			putchar('\n');
			return(DONE);
		}
	}
	if (r = operands(tree))
		insrp("cmp", AX, r, 0);
	else
		printf("\tcmp\tax,cx\n");
	return(DONE);
}

/*
 * An & whose result is only tested.
 */
gtest(tree)
register union tree *tree;
{
	register union tree *l, *r;
	register f;

	l = tree->t.tr1;
	r = tree->t.tr2;
	if (r->t.op==CON && (l->t.op==NAME || IND(l))
	 && !LNG(l->t.type)) {
		f = 0;
		if (CHR(l->t.type)) {
			if (r->c.value<0 || r->c.value>255)
				goto gen;
			f = BYT;
		}
		if (IND(l))
			gaddr(l);
		inspn("test", l, f, r->c.value);
		return(CCLOGIC);
	}
gen:
	if (r = operands(tree))
		insrp("test", AX, r, 0);
	else
		printf("\ttest\tax,cx\n");
	return(CCLOGIC);
}

/*
 * Compile the operands of a binary operator on words: the left into
 * AX.  The right is returned if it can be the instruction's operand,
 * which it then is; if not it is in CX and 0 is returned.
 */
union tree *
operands(tree)
union tree *tree;
{
	register union tree *l, *r;

	l = tree->t.tr1;
	r = tree->t.tr2;
	if (adrble(r)) {
		rcexpr(l, FORREG);
		return(r);
	}
	if (canbx(r) && !CHR(r->t.type)) {
		rcexpr(l, FORREG);
		gaddr(r);
		return(r);
	}
	/*
	 * A character is made an int in AX, and no other register
	 * will do that for one with a sign.
	 */
	if (CHR(r->t.type) && (r->t.op==NAME || canbx(r))) {
		rcexpr(l, FORREG);
		if (IND(r))
			gaddr(r);
		if (r->t.type==UNCHAR) {
			insrp("mov", CX, r, BYT);
			printf("\txor\tch,ch\n");
		} else {
			printf("\txchg\tax,cx\n");
			load(r, AX);
			printf("\txchg\tax,cx\n");
		}
		return(NULL);
	}
	rcexpr(r, FORREG);
	if (easy(l, AX)) {
		printf("\tmov\tcx,ax\n");
		greg(l, AX);
	} else {
		printf("\tpush\tax\n");
		rcexpr(l, FORREG);
		printf("\tpop\tcx\n");
	}
	return(NULL);
}

/*
 * The same for longs: the left into DX:AX, and the right returned, or
 * in CX:BX.
 */
union tree *
loperands(tree)
union tree *tree;
{
	register union tree *l, *r;

	l = tree->t.tr1;
	r = tree->t.tr2;
	if (adrble(r)) {
		rcexpr(l, FORREG);
		return(r);
	}
	if (canbx(r)) {
		rcexpr(l, FORREG);
		gaddr(r);
		return(r);
	}
	rcexpr(r, FORREG);
	if (adrble(l)) {
		printf("\tmov\tbx,ax\n\tmov\tcx,dx\n");
		load(l, AX);
	} else {
		printf("\tpush\tdx\n\tpush\tax\n");
		rcexpr(l, FORREG);
		printf("\tpop\tbx\n\tpop\tcx\n");
	}
	return(NULL);
}

/*
 * For storing r in l, which is a variable or what a pointer points at:
 * compile r into AX or DX:AX and make l an operand.
 */
gvalue(l, r)
register union tree *l, *r;
{
	register far;

	if ((l->t.op==STAR || l->t.op==FSTAR) && !canbx(l)) {
		far = l->t.op==FSTAR;
		rcexpr(bxform(l), FORSP);
		rcexpr(r, FORREG);
		printf("\tpop\tbx\n");
		if (far)
			printf("\tpop\tes\n");
		return;
	}
	rcexpr(r, FORREG);
	if (IND(l))
		gaddr(l);
	else if (l->t.op!=NAME)
		error("Illegal lvalue");
}

/*
 * Is p an operand as it stands: a word an instruction can take, or a
 * long whose two words are?
 */
adrble(p)
register union tree *p;
{
	register union tree *q;

	switch (p->t.op) {

	case CON:
	case LCON:
		return(1);

	case AMPER:
		q = p->t.tr1;
		return(q->t.op==NAME && (q->n.class==EXTERN || q->n.class==STATIC));

	case NAME:
		return(!CHR(p->t.type) && !isfloat(p));

	/*
	 * As a long, a constant, or an unsigned word with nothing above
	 * it.
	 */
	case ITOL:
		q = p->t.tr1;
		return(q->t.op==CON
		    || q->t.op==NAME && uns(q) && !CHR(q->t.type));
	}
	return(0);
}

/*
 * Can p be a constant in an instruction that stores into something of
 * the type?
 */
isimm(p, type)
register union tree *p;
{
	if (LNG(type))
		return(p->t.op==LCON || p->t.op==ITOL && p->t.tr1->t.op==CON);
	if (p->t.op==CON)
		return(1);
	return(p->t.op==AMPER && adrble(p) && !CHR(type));
}

/*
 * What BX must hold for the indirection p to be an operand: what it
 * points through, less any constant or address added, which goes into
 * the operand.
 */
union tree *
adrof(p)
register union tree *p;
{
	register union tree *q;

	p = p->t.tr1;
	if (p->t.op==PLUS) {
		q = p->t.tr2;
		if (q->t.op==CON || q->t.op==AMPER && adrble(q))
			return(p->t.tr1);
	}
	return(p);
}

/*
 * Can the indirection p be made an operand with only BX disturbed?
 */
canbx(p)
register union tree *p;
{
	register union tree *q;

	if (p->t.op==AUTOI || p->t.op==AUTOD)
		return(1);
	/*
	 * Through a far pointer, if the pointer is where les can load
	 * it from: BX and ES are disturbed.
	 */
	if (p->t.op==FSTAR) {
		q = faradr(p);
		return(q->t.op==NAME || IND(q) && canbx(q));
	}
	return(p->t.op==STAR && easy(adrof(p), BX));
}

/*
 * The far pointer that the indirection p goes through, less any
 * constant added, which goes into the operand.
 */
union tree *
faradr(p)
register union tree *p;
{
	p = p->t.tr1;
	if (p->t.op==FPLUS && p->t.tr2->t.op==CON)
		return(p->t.tr1);
	return(p);
}

/*
 * Can greg compute the word p in the register r, with no other
 * disturbed but BX?
 */
easy(p, r)
register union tree *p;
{
	if (LNG(p->t.type) || isfloat(p))
		return(0);
	if (CHR(p->t.type) && r!=AX)
		return(0);
	switch (p->t.op) {

	case NAME:
	case CON:
	case AUTOI:
	case AUTOD:
		return(1);

	case AMPER:
		return(adrble(p));

	case STAR:
		return(easy(adrof(p), BX));

	case FSTAR:
		return(canbx(p));

	case PLUS:
	case MINUS:
	case AND:
	case OR:
	case EXOR:
		return(adrble(p->t.tr2) && easy(p->t.tr1, r));

	case LSHIFT:
	case RSHIFT:
	case ULSH:
		return(p->t.tr2->t.op==CON && easy(p->t.tr1, r));

	case NEG:
	case COMPL:
		return(easy(p->t.tr1, r));
	}
	return(0);
}

/*
 * Compute p, which is easy, in the register r.
 */
greg(p, r)
register union tree *p;
{
	switch (p->t.op) {

	case STAR:
	case FSTAR:
	case AUTOI:
	case AUTOD:
		gaddr(p);
	case NAME:
	case CON:
	case AMPER:
		load(p, r);
		return;

	case PLUS:
		if (lea(p, r))
			return;
	case MINUS:
	case AND:
	case OR:
	case EXOR:
		greg(p->t.tr1, r);
		arith(p->t.op, r, p->t.tr2);
		return;

	case LSHIFT:
	case RSHIFT:
	case ULSH:
		greg(p->t.tr1, r);
		shiftc(p->t.op, r, p->t.tr2->c.value);
		return;

	case NEG:
		greg(p->t.tr1, r);
		printf("\tneg\t%s\n", rname[r]);
		return;

	case COMPL:
		greg(p->t.tr1, r);
		printf("\tnot\t%s\n", rname[r]);
		return;
	}
	error("compiler error: greg");
}

/*
 * A register variable, or the frame pointer, and a constant or an
 * address: the sum is had with lea, if it is not for the register
 * itself.
 */
lea(p, r)
register union tree *p;
{
	register union tree *l, *q;

	l = p->t.tr1;
	q = p->t.tr2;
	if (l->t.op!=NAME || l->n.class!=REG || l->n.nloc==r)
		return(0);
	if (q->t.op==CON) {
		printf("\tlea\t%s,[%s", rname[r], rname[l->n.nloc]);
		poff(q->c.value);
	} else if (q->t.op==AMPER && adrble(q)) {
		q = q->t.tr1;
		printf("\tlea\t%s,[%s+", rname[r], rname[l->n.nloc]);
		pbase(q);
		poff(q->n.offset);
	} else
		return(0);
	printf("]\n");
	return(1);
}

/*
 * Does computing p with greg in the register variable r read r after
 * writing it?  It does if anything but the first thing loaded is r, or
 * is reached through it.
 */
clobbers(p, r)
register union tree *p;
{
	register union tree *q;

	for (;;) switch (p->t.op) {

	case PLUS:
	case MINUS:
	case AND:
	case OR:
	case EXOR:
		q = p->t.tr2;
		if (q->t.op==NAME && (q->n.class==REG? q->n.nloc==r:
		    q->n.class!=EXTERN && q->n.class!=STATIC && q->n.regno==r))
			return(1);
	case LSHIFT:
	case RSHIFT:
	case ULSH:
	case NEG:
	case COMPL:
		p = p->t.tr1;
		continue;

	default:
		return(0);
	}
}

/*
 * p is an indirection.  Make it the operand [bx+...], and return the
 * tree whose value BX is to have by the time it is used; or, through
 * a far pointer, the operand [es:bx+n] and the tree for ES:BX.
 */
union tree *
bxform(p)
register union tree *p;
{
	register union tree *q, *s;
	int type, off;

	type = p->t.type;
	if (p->t.op==FSTAR) {
		q = faradr(p);
		off = q==p->t.tr1? 0: p->t.tr1->t.tr2->c.value;
		p->n.op = NAME;
		p->n.type = type;
		p->n.class = FOFFS;
		p->n.regno = BX;
		p->n.offset = off;
		p->n.nloc = 0;
		return(q);
	}
	q = p->t.tr1;
	off = 0;
	s = NULL;
	if (q->t.op==PLUS) {
		s = q->t.tr2;
		if (s->t.op==CON) {
			off = s->c.value;
			q = q->t.tr1;
			s = NULL;
		} else if (s->t.op==AMPER && adrble(s)) {
			s = s->t.tr1;
			q = q->t.tr1;
		} else
			s = NULL;
	}
	p->n.op = NAME;
	p->n.type = type;
	p->n.regno = BX;
	if (s) {
		p->n.offset = s->n.offset;
		if (s->n.class==EXTERN) {
			p->n.class = XOFFS;
			p->x.name = s->x.name;
		} else {
			p->n.class = SOFFS;
			p->n.nloc = s->n.nloc;
		}
	} else {
		p->n.class = OFFS;
		p->n.offset = off;
		p->n.nloc = 0;
	}
	return(q);
}

/*
 * Make the indirection p an operand: put what it points through in
 * BX.  If that is not easy, AX, CX and DX are disturbed.
 */
gaddr(p)
register union tree *p;
{
	register union tree *q;

	if (p->t.op==FSTAR) {
		q = bxform(p);
		if (q->t.op==NAME || IND(q) && canbx(q)) {
			if (IND(q))
				gaddr(q);
			insrp("les", BX, q, FARP);
		} else {
			rcexpr(q, FORREG);
			printf("\tmov\tbx,ax\n\tmov\tes,dx\n");
		}
		return;
	}
	if (p->t.op!=STAR) {
		autoop(p);
		return;
	}
	q = bxform(p);
	if (easy(q, BX))
		greg(q, BX);
	else {
		rcexpr(q, FORREG);
		printf("\txchg\tax,bx\n");
	}
}

/*
 * *p++ and *--p, where p is a register variable: the PDP-11's
 * auto-increment and auto-decrement, which the first pass's trees and
 * the optimizer still have.  The pointer is moved here, before the
 * operand is used, and the tree becomes the variable [p-n], or [p];
 * so the flags of whatever uses it are not disturbed afterwards, and
 * it can be used twice.
 */
autoop(p)
register union tree *p;
{
	register n, r;

	n = autosize(p->t.type);
	r = p->n.nloc;
	if (n==1)
		printf("\t%s\t%s\n", p->t.op==AUTOI? "inc": "dec", rname[r]);
	else
		printf("\t%s\t%s,%d\n", p->t.op==AUTOI? "add": "sub",
		    rname[r], n);
	p->n.offset = p->t.op==AUTOI? -n: 0;
	p->n.op = NAME;
	p->n.class = OFFS;
	p->n.regno = r;
}

/*
 * A far pointer and an int: the far pointer in DX:AX, and the int
 * added to its offset, or taken from it.
 */
faradd(tree)
register union tree *tree;
{
	register union tree *l, *r;
	int op;

	op = tree->t.op==FPLUS? PLUS: MINUS;
	l = tree->t.tr1;
	r = tree->t.tr2;
	if (adrble(r) || canbx(r) && !CHR(r->t.type)) {
		rcexpr(l, FORREG);
		if (IND(r))
			gaddr(r);
		arith(op, AX, r);
		return;
	}
	rcexpr(r, FORREG);
	if (l->t.op==NAME) {
		printf("\tmov\tcx,ax\n");
		load(l, AX);
	} else {
		printf("\tpush\tax\n");
		rcexpr(l, FORREG);
		printf("\tpop\tcx\n");
	}
	printf("\t%s\tax,cx\n", opname(op));
}

/*
 * l += r and l -= r, where l is a far pointer and r an int: the work
 * is done on the pointer's offset, which is its first word, where it
 * is kept.
 */
gasgfar(tree, ctx)
register union tree *tree;
{
	register union tree *l, *r;
	int op, type;

	op = tree->t.op==ASFPLUS? ASPLUS: ASMINUS;
	l = tree->t.tr1;
	r = tree->t.tr2;
	type = l->t.type;
	if (isimm(r, INT)) {
		if (IND(l))
			gaddr(l);
		else if (l->t.op!=NAME) {
			error("Illegal lvalue");
			return(DONE);
		}
		l->t.type = INT;
		opimm(op, l, r);
	} else {
		gvalue(l, r);
		l->t.type = INT;
		inspr(opname(op), l, 0, AX);
	}
	l->t.type = type;
	if (ctx==FOREFF)
		return(DONE);
	load(l, AX);
	return(INREG);
}

/*
 * How far such an operand of the type moves its pointer.
 */
autosize(type)
{
	if (CHR(type))
		return(1);
	if (type==FLOAT)
		return(4);
	if (type==DOUBLE)
		return(8);
	return(2);
}

/*
 * Load the operand p into the register r; a long into DX:AX.  A
 * character is made an int, which only AX can do.
 */
load(p, r)
register union tree *p;
{
	register type;

	type = p->t.type;
	if (LNG(type)) {
		lhalf(p, AX, 0);
		lhalf(p, DX, HI);
		return;
	}
	if (p->t.op==CON && p->c.value==0) {
		printf("\txor\t%s,%s\n", rname[r], rname[r]);
		return;
	}
	if (p->t.op==NAME) {
		if (p->n.class==REG && p->n.nloc==r)
			return;
		if (CHR(type)) {
			if (r!=AX)
				error("compiler error: load");
			insrp("mov", AX, p, BYT);
			printf(type==CHAR? "\tcbw\n": "\txor\tah,ah\n");
			return;
		}
	}
	insrp("mov", r, p, 0);
}

/*
 * Load half of the long operand p.
 */
lhalf(p, r, hi)
register union tree *p;
{
	register union tree *q;
	register LTYPE v;

	q = p;
	if (q->t.op==ITOL)
		q = q->t.tr1;
	if (q->t.op==LCON || q->t.op==CON || hi && p->t.op==ITOL) {
		if (q->t.op==LCON)
			v = hi? q->l.lvalue>>16: q->l.lvalue;
		else if (hi)
			v = q->t.op==CON && !uns(q) && q->c.value<0? -1: 0;
		else
			v = q->c.value;
		if (UNS(v)==0) {
			printf("\txor\t%s,%s\n", rname[r], rname[r]);
			return;
		}
	}
	insrp("mov", r, p, hi);
}

/*
 * Store AX, or AL, or DX:AX in the operand p.
 */
store(p)
register union tree *p;
{
	if (LNG(p->t.type)) {
		inspr("mov", p, 0, AX);
		inspr("mov", p, HI, DX);
	} else if (CHR(p->t.type))
		inspr("mov", p, BYT, AX);
	else if (p->n.class!=REG || p->n.nloc!=AX)
		inspr("mov", p, 0, AX);
}

/*
 * After a value has been stored in something of the type, make what
 * is in AX what was stored.
 */
stored(type)
{
	if (type==CHAR)
		printf("\tcbw\n");
	else if (type==UNCHAR)
		printf("\txor\tah,ah\n");
}

/*
 * The instruction for an operator; the second is the one for the high
 * word of a long.
 */
char *
opname(op)
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
		return("sub");

	case AND:
	case ASAND:
		return("and");

	case OR:
	case ASOR:
		return("or");

	case EXOR:
	case ASXOR:
		return("xor");

	case LSHIFT:
	case ASLSH:
		return("shl");

	case RSHIFT:
	case ASRSH:
		return("sar");

	case ULSH:
	case ASULSH:
		return("shr");
	}
	error("compiler error: opname %d", op);
	return("?");
}

char *
opname2(op)
{
	register char *s;

	s = opname(op);
	if (*s=='a' && s[1]=='d')
		return("adc");
	if (*s=='s')
		return("sbb");
	return(s);
}

/*
 * Is the operator an addition or a subtraction, after which the
 * overflow flag means something?
 */
isadd(op)
{
	register char *s;

	s = opname(op);
	return(*s=='s' || *s=='a' && s[1]=='d');
}

/*
 * r op= p, where r is a register and p an operand.  Returns what has
 * become of the flags.
 */
arith(op, r, p)
register union tree *p;
{
	register v;

	if (p->t.op==CON && isadd(op)) {
		v = p->c.value;
		if (*opname(op)=='s')
			v = W(-v);
		if (v==0)
			return(INREG);
		if (v==1 || v==-1) {
			printf("\t%s\t%s\n", v==1? "inc": "dec", rname[r]);
			return(CCARITH);
		}
	}
	insrp(opname(op), r, p, 0);
	return(isadd(op)? CCARITH: CCLOGIC);
}

/*
 * DX:AX op= r, a long operand, or CX:BX if there is none.
 */
larith(op, r)
register union tree *r;
{
	if (r==NULL) {
		printf("\t%s\tax,bx\n\t%s\tdx,cx\n", opname(op), opname2(op));
		return;
	}
	insrp(opname(op), AX, r, 0);
	insrp(opname2(op), DX, r, HI);
}

/*
 * Shift the register r by the constant n.
 */
shiftc(op, r, n)
{
	printf("\t%s\t%s,%d\n", opname(op), rname[r], n&037);
}

/*
 * Multiply, divide or shift AX by r, an operand, or by CX if there is
 * none.  The quotient or the remainder is left in AX; DX is lost.
 */
mulop(op, r)
register union tree *r;
{
	register char *s;

	switch (op) {

	case TIMES:
		if (r==NULL)
			printf("\timul\tcx\n");
		else if (r->t.op==CON || r->t.op==AMPER) {
			printf("\timul\tax,ax,");
			pname(r, 0);
			putchar('\n');
		} else
			ins1("imul", r, 0);
		return;

	case DIVIDE:
	case MOD:
	case UDIV:
	case UMOD:
		if (r && (r->t.op==CON || r->t.op==AMPER)) {
			insrp("mov", CX, r, 0);
			r = NULL;
		}
		if (op==UDIV || op==UMOD) {
			printf("\txor\tdx,dx\n");
			s = "div";
		} else {
			printf("\tcwd\n");
			s = "idiv";
		}
		if (r)
			ins1(s, r, 0);
		else
			printf("\t%s\tcx\n", s);
		if (op==MOD || op==UMOD)
			printf("\tmov\tax,dx\n");
		return;

	case LSHIFT:
	case RSHIFT:
	case ULSH:
		if (r && r->t.op==CON) {
			shiftc(op, AX, r->c.value);
			return;
		}
		if (r)
			insrp("mov", CX, r, 0);
		printf("\t%s\tax,cl\n", opname(op));
		return;
	}
	error("compiler error: mulop");
}

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
 * Call one of the library's routines for what the 286 has no
 * instruction for.  They are far in the medium model.
 */
callh(s)
char *s;
{
	xref(s);
	printf(mflag? "\tcall\tfar %s\n": "\tcall\t%s\n", s);
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

/*
 * Assignment.
 */
gassign(tree, ctx)
register union tree *tree;
{
	register union tree *l, *r;
	int type;

	l = tree->t.tr1;
	r = tree->t.tr2;
	type = l->t.type;
	if (ctx==FOREFF && isimm(r, type)) {
		if (IND(l))
			gaddr(l);
		else if (l->t.op!=NAME) {
			error("Illegal lvalue");
			return(DONE);
		}
		movimm(l, r);
		return(DONE);
	}
	/*
	 * Into a register variable directly, if it can be.
	 */
	if (ctx==FOREFF && l->t.op==NAME && l->n.class==REG
	 && easy(r, l->n.nloc) && !clobbers(r, l->n.nloc)) {
		greg(r, l->n.nloc);
		return(DONE);
	}
	gvalue(l, r);
	store(l);
	if (ctx!=FOREFF)
		stored(type);
	return(INREG);
}

/*
 * Store the constant r in the operand l.
 */
movimm(l, r)
register union tree *l, *r;
{
	register f;

	f = CHR(l->t.type)? BYT: 0;
	if (l->n.class==REG && r->t.op==CON && r->c.value==0) {
		load(r, l->n.nloc);
		return;
	}
	printf("\tmov\t");
	pname(l, f);
	putchar(',');
	pname(r, f);
	putchar('\n');
	if (LNG(l->t.type)) {
		printf("\tmov\t");
		pname(l, HI);
		putchar(',');
		pname(r, HI);
		putchar('\n');
	}
}

/*
 * l op= r, where the 286 can do the work in memory: + - & | ^ and the
 * four ++ and --.
 */
gasgop(tree, ctx)
register union tree *tree;
{
	register union tree *l, *r;
	int op, type, f, post;

	op = tree->t.op;
	l = tree->t.tr1;
	r = tree->t.tr2;
	type = l->t.type;
	f = CHR(type)? BYT: 0;
	post = (op==INCAFT || op==DECAFT) && ctx!=FOREFF;
	if (isimm(r, type)) {
		if (IND(l))
			gaddr(l);
		else if (l->t.op!=NAME) {
			error("Illegal lvalue");
			return(DONE);
		}
		if (post)
			load(l, AX);
		opimm(op, l, r);
	} else if (post) {
		error("compiler error: postfix");
		return(DONE);
	} else if (l->t.op==NAME && l->n.class==REG && adrble(r)) {
		insrp(opname(op), l->n.nloc, r, 0);
	} else {
		gvalue(l, r);
		inspr(opname(op), l, f, AX);
		if (LNG(type))
			inspr(opname2(op), l, HI, DX);
	}
	if (ctx==FOREFF)
		return(DONE);
	if (post)
		return(INREG);
	if (ctx==FORCC && !f && !LNG(type))
		return(isadd(op)? CCARITH: CCLOGIC);
	load(l, AX);
	return(INREG);
}

/*
 * l op= r, where l is an operand and r a constant.
 */
opimm(op, l, r)
register union tree *l, *r;
{
	register v;
	int f;

	if (LNG(l->t.type)) {
		printf("\t%s\t", opname(op));
		pname(l, 0);
		putchar(',');
		pname(r, 0);
		printf("\n\t%s\t", opname2(op));
		pname(l, HI);
		putchar(',');
		pname(r, HI);
		putchar('\n');
		return;
	}
	f = CHR(l->t.type)? BYT: 0;
	if (r->t.op==CON && isadd(op)) {
		v = r->c.value;
		if (*opname(op)=='s')
			v = W(-v);
		if (v==1 || v==-1) {
			ins1(v==1? "inc": "dec", l, f);
			return;
		}
	}
	printf("\t%s\t", opname(op));
	pname(l, f);
	putchar(',');
	pname(r, f);
	putchar('\n');
}

/*
 * l op= r, where the work is done in the registers and the result
 * stored: * / % and the shifts, and all of those on longs.
 */
gasgop2(tree, ctx)
register union tree *tree;
{
	register union tree *l, *r;
	int op, type, lng, far, radr;

	op = tree->t.op;
	l = tree->t.tr1;
	r = tree->t.tr2;
	type = l->t.type;
	/*
	 * The operator without the assignment.
	 */
	if (op==ASULSH)
		op = ULSH;
	else if (op==ASUDIV || op==ASUMOD)
		op += UDIV-ASUDIV;
	else if (op>=ULASTIMES)
		op += ULTIMES-ULASTIMES;
	else if (op>=LASTIMES)
		op += LTIMES-LASTIMES;
	else
		op += PLUS-ASPLUS;
	lng = LNG(type);
	if (l->t.op!=NAME && !IND(l)) {
		error("Illegal lvalue");
		return(DONE);
	}
	if (lng && (op==LSHIFT || op==RSHIFT || op==ULSH)) {
		if (IND(l))
			gaddr(l);
		lshift(op, l, r, l);
	} else if (l->t.op==NAME) {
		if (lng)
			lmulop(op, loperands(tree));
		else
			mulop(op, operands(tree));
	} else if (lng) {
		/*
		 * A long through a pointer.  The routines want BX, so
		 * the pointer waits on the stack: under the right
		 * operand, if that is there too.  (Pushing the right
		 * operand may leave it looking like one that need not
		 * have been: it is asked first.)
		 */
		far = l->t.op==FSTAR;
		radr = adrble(r);
		rcexpr(bxform(l), FORSP);
		if (radr)
			printf("\tmov\tbx,sp\n\t%s\tbx,[bx]\n",
			    far? "les": "mov");
		else {
			rcexpr(r, FORSP);
			printf("\tmov\tbx,sp\n\t%s\tbx,[bx+4]\n",
			    far? "les": "mov");
		}
		load(l, AX);
		if (radr)
			lmulop(op, r);
		else {
			printf("\tpop\tbx\n\tpop\tcx\n");
			lmulop(op, (union tree *)NULL);
		}
		printf("\tpop\tbx\n");
		if (far)
			printf("\tpop\tes\n");
	} else if (adrble(r)) {
		gaddr(l);
		load(l, AX);
		mulop(op, r);
	} else {
		rcexpr(r, FORSP);
		gaddr(l);
		printf("\tpop\tcx\n");
		load(l, AX);
		mulop(op, (union tree *)NULL);
	}
	store(l);
	if (ctx!=FOREFF)
		stored(type);
	return(INREG);
}

/*
 * A call.  The arguments are pushed, the last first, and taken off
 * when the function returns, with its value in AX or DX:AX.
 *
 * A function that returns a structure of more than two words is given
 * in SI the address of where to put it (strasg, in c11.c).  That is
 * the value of dest; or, if there is no dest and tmp is set, it is
 * where the stack pointer is now, the caller having made room there.
 *
 * In the medium model a function is called far.  One that this file
 * has defined is in the segment the call is in, and is called near
 * with CS pushed for it.  And in any model a function may be called
 * through a far pointer.
 */
gcall(tree, dest, tmp)
register union tree *tree;
union tree *dest;
{
	register union tree *p;
	register n;
	int direct, far, lbl;

	if (dest)
		tmp = 1;
	if (tmp)
		printf("\tpush\tsi\n");
	n = 0;
	p = tree->t.tr2;
	if (p->t.op) {
		while (p->t.op==COMMA) {
			n += comarg(p->t.tr2);
			p = p->t.tr1;
		}
		n += comarg(p);
	}
	p = tree->t.tr1;
	if (p->t.op==AUTOI || p->t.op==AUTOD)
		gaddr(p);
	direct = p->t.op==NAME && (p->n.class==EXTERN || p->n.class==STATIC);
	far = 0;
	if (p->t.op==FSTAR) {
		/*
		 * Through a far pointer: where it is kept, if that is
		 * to be had easily, and if not from DX:AX.
		 */
		p = p->t.tr1;
		far = 2;
		if (!tmp && (p->t.op==NAME || IND(p) && canbx(p)))
			far = 1;
		else
			rcexpr(p, FORREG);
	} else if (tmp && !direct) {
		/*
		 * Find the function before SI is changed: SI may be
		 * what points at it.
		 */
		if (p->t.op==STAR)
			p = p->t.tr1;
		else
			p = optim(tnode(AMPER, INT+PTR, p, TNULL));
		rcexpr(p, FORREG);
	}
	if (dest) {
		if (!easy(dest, SI))
			error("Compiler error: gcall");
		greg(dest, SI);
	} else if (tmp)
		printf("\tmov\tsi,sp\n\tadd\tsi,%d\n", n+2);
	if (direct) {
		if (!mflag)
			printf("\tcall\t");
		else if (p->n.class==STATIC || isdefd(p->x.name))
			printf("\tpush\tcs\n\tcall\t");
		else
			printf("\tcall\tfar ");
		pbase(p);
		poff(p->n.offset);
		putchar('\n');
	} else if (far==1) {
		if (IND(p))
			gaddr(p);
		ins1("call", p, FARP);
	} else if (far) {
		/*
		 * To the function in DX:AX, by returning to it, with
		 * where it is to return to pushed first.
		 */
		lbl = isn++;
		printf("\tpush\tcs\n\tpush\tword L%d\n", lbl);
		printf("\tpush\tdx\n\tpush\tax\n\tretf\n");
		label(lbl);
	} else if (tmp) {
		printf("\tcall\tax\n");
	} else if (p->t.op==NAME && p->n.class==OFFS && p->n.offset==0) {
		/* the function a register variable points at */
		printf("\tcall\t%s\n", rname[p->n.regno]);
	} else if (p->t.op==STAR && p->t.tr1->t.op==NAME
	    && !CHR(p->t.tr1->t.type)) {
		/* the function a variable points at */
		ins1("call", p->t.tr1, 0);
	} else {
		if (p->t.op==STAR)
			p = p->t.tr1;
		else
			p = optim(tnode(AMPER, INT+PTR, p, TNULL));
		rcexpr(p, FORREG);
		printf("\tcall\tax\n");
	}
	popstk(n);
	if (tmp)
		printf("\tpop\tsi\n");
}

/*
 * Compile a function argument.
 * Return the number of bytes pushed,
 * for future popping.
 *
 * A structure is pushed whole.  If it is what a function returns, room
 * is made for it and the function puts it there.
 */
comarg(tree)
register union tree *tree;
{
	register size, i;

	if (tree->t.op==STRASG) {
		size = tree->F.mask;
		tree = tree->t.tr1;
		while (tree->t.op==SEQNC) {
			rcexpr(tree->t.tr1, FOREFF);
			tree = tree->t.tr2;
		}
		if (size<=4) {
			tree = paint(tree, size<=2? INT: LONG);
			goto normal;
		}
		if (tree->t.op==CALL) {
			printf("\tsub\tsp,%d\n", size);
			gcall(tree, TNULL, 1);
			return(size);
		}
		if (tree->t.op!=NAME && tree->t.op!=STAR && tree->t.op!=FSTAR) {
			error("Unimplemented structure assignment");
			return(0);
		}
		if (size<=10) {
			if (IND(tree))
				gaddr(tree);
			tree->n.offset += size;
			for (i = 0; i<size; i += 2) {
				tree->n.offset -= 2;
				ins1("push", tree, 0);
			}
			return(size);
		}
		if (tree->t.op==FSTAR) {
			rcexpr(tree->t.tr1, FORREG);
			printf("\tsub\tsp,%d\n", size);
			printf("\tmov\tbx,sp\n\tmov\tcx,ds\n");
			fmovs(size/2);
			return(size);
		}
		rcexpr(optim(tnode(AMPER, STRUCT+PTR, tree, TNULL)), FORREG);
		printf("\tsub\tsp,%d\n", size);
		printf("\tmov\tbx,ax\n\tmov\tax,sp\n");
		movs(size/2);
		return(size);
	}
normal:
	rcexpr(tree, FORSP);
	return(arlength(tree->t.type));
}

/*
 * Delay handles postfix ++ and --
 * It observes that "x + y++" is better
 * treated as "x + y; y++".
 * If the operator is ++ or -- itself,
 * it calls rcexpr to load the operand, letting
 * the calling instance of rcexpr to do the
 * ++ for its effect.
 * Otherwise it uses sdelay to search for inc/dec
 * among the operands.
 */
delay(treep, ctx)
union tree **treep;
{
	register union tree *p, *p1;
	register r;

	p = *treep;
	if ((p->t.op==INCAFT||p->t.op==DECAFT)
	 && p->t.tr1->t.op==NAME) {
		r = p->t.tr1->n.class;
		if (r == EXTERN || r == OFFS || r == STATIC &&
				p->t.tr1->t.type == UNCHAR)
			rcexpr(p->t.tr1, ctx);
		else
			rcexpr(paint(p->t.tr1, p->t.type), ctx);
		return(1);
	}
	p1 = 0;
	if (opdope[p->t.op]&BINARY) {
		if (p->t.op==LOGAND || p->t.op==LOGOR
		 || p->t.op==QUEST || p->t.op==COLON || p->t.op==SEQNC)
			return(0);
		p1 = sdelay(&p->t.tr2);
	}
	if (p1==0)
		p1 = sdelay(&p->t.tr1);
	if (p1) {
		rcexpr(optim(p), ctx);
		*treep = p1;
		return(1);
	}
	return(0);
}

union tree *
sdelay(ap)
union tree **ap;
{
	register union tree *p, *p1;

	if ((p = *ap)==TNULL)
		return(TNULL);
	if ((p->t.op==INCAFT||p->t.op==DECAFT) && p->t.tr1->t.op==NAME) {
		*ap = paint(ncopy(p->t.tr1), p->t.type);
		return(p);
	}
	if (p->t.op==STAR || p->t.op==PLUS)
		if (p1=sdelay(&p->t.tr1))
			return(p1);
	if (p->t.op==PLUS)
		return(sdelay(&p->t.tr2));
	return(0);
}

/*
 * Propagate possible implicit type-changing operation
 */
union tree *
paint(tp, type)
register union tree *tp;
register type;
{

	if (tp->t.type==type)
		return(tp);
	if (tp->t.type==CHAR && type==INT)
		return(tp);
	/*
	 * A float is computed with as a double, but is not one where
	 * it is kept.
	 */
	if ((tp->t.type==FLOAT || tp->t.type==DOUBLE)
	 && (type==FLOAT || type==DOUBLE))
		return(tp);
	if (tp->t.type==CHAR || tp->t.type==UNCHAR)
		return(optim(tnode(LOAD, type, tp, TNULL)));
	tp->t.type = type;
	if (tp->t.op==AMPER && type&XTYPE) {
		/*
		 * The address of a character is not that of a
		 * character loaded.
		 */
		tp->t.tr1 = paint(tp->t.tr1, decref(type));
		if (tp->t.tr1->t.op==LOAD)
			tp->t.tr1 = tp->t.tr1->t.tr1;
	} else if (tp->t.op==STAR)
		tp->t.tr1 = paint(tp->t.tr1, incref(type));
	else if (tp->t.op==ASSIGN) {
		paint(tp->t.tr1, type);
		paint(tp->t.tr2, type);
	}
	return(tp);
}

/*
 * Copy a tree node for a register variable.
 * Used by sdelay because if *reg-- is turned
 * into *reg; reg-- the *reg will in turn
 * be changed to some offset class, accidentally
 * modifying the reg--.
 */
union tree *
ncopy(p)
register union tree *p;
{
	register union tree *q;

	q = getblk(sizeof(struct xtname));
	q->n.op = p->n.op;
	q->n.type = p->n.type;
	q->n.class = p->n.class;
	q->n.regno = p->n.regno;
	q->n.offset = p->n.offset;
	if (q->n.class==EXTERN || q->n.class==XOFFS)
		q->x.name = p->x.name;
	else
		q->n.nloc = p->n.nloc;
	return(q);
}

/*
 * Compile an initializing expression
 */
doinit(type, tree)
register type;
register union tree *tree;
{
	LTYPE lval;

	if (type==CHAR || type==UNCHAR) {
		if (tree->t.type&XTYPE)
			goto illinit;
		if (tree->t.op!=CON)
			goto illinit;
		printf("\tdb\t%d\n", tree->c.value&0377);
		return;
	}
	if (type&XTYPE)
		type = INT;
	switch (type) {
	case INT:
	case UNSIGN:
		if (tree->t.op==FTOI) {
			if (tree->t.tr1->t.op!=FCON && tree->t.tr1->t.op!=SFCON)
				goto illinit;
			tree = tree->t.tr1;
			tree->c.value = tree->f.fvalue;
			tree->t.op = CON;
		} else if (tree->t.op==LTOI) {
			if (tree->t.tr1->t.op!=LCON)
				goto illinit;
			tree = tree->t.tr1;
			lval = tree->l.lvalue;
			tree->t.op = CON;
			tree->c.value = W(lval);
		}
		if (tree->t.op==CON || tree->t.op==AMPER && adrble(tree)) {
			printf("\tdw\t");
			pname(tree, 0);
			putchar('\n');
		} else
			goto illinit;
		return;

	case DOUBLE:
	case FLOAT:
		if (tree->t.op!=FCON && tree->t.op!=SFCON)
			goto illinit;
		fdata(type, tree->f.fvalue);
		return;

	case UNLONG:
	case LONG:
		/*
		 * A far pointer to a function: its address and its
		 * segment, which is for whatever loads the program to
		 * put there.
		 */
		if (tree->t.op==AMPER && adrble(tree)) {
			printf("\tdw\t");
			pname(tree, 0);
			putchar(',');
			pname(tree, HI);
			putchar('\n');
			return;
		}
		if (tree->t.op==FTOL) {
			tree = tree->t.tr1;
			if (tree->t.op==SFCON)
				tree->t.op = FCON;
			if (tree->t.op!= FCON)
				goto illinit;
			lval = tree->f.fvalue;
		} else if (tree->t.op==ITOL) {
			if (tree->t.tr1->t.op != CON)
				goto illinit;
			if (uns(tree->t.tr1))
				lval = UNS(tree->t.tr1->c.value);
			else
				lval = tree->t.tr1->c.value;
		} else if (tree->t.op==LCON)
			lval = tree->l.lvalue;
		else
			goto illinit;
		printf("\tdw\t%u,%u\n", UNS(lval), UNS(lval>>16));
		return;
	}
illinit:
	error("Illegal initialization");
}

/*
 * Write an operand: a constant, the address of something, a register
 * variable, or a variable in memory, which may be reached through a
 * register.  HI asks for the high word of a long, BYT for a byte,
 * FLT for the size of the variable's type, and FARP for the far
 * pointer that is there.  The high word of an address is its segment.
 */
pname(p, flag)
register union tree *p;
{
	register i;

loop:
	switch(p->t.op) {

	case LCON:
		printf("%u", flag&HI? UNS(p->l.lvalue>>16): UNS(p->l.lvalue));
		return;

	/*
	 * A word made long.
	 */
	case ITOL:
		p = p->t.tr1;
		if (flag&HI) {
			i = p->t.op==CON && !uns(p) && p->c.value<0;
			printf("%d", i? -1: 0);
			return;
		}
		goto loop;

	case CON:
		printf("%d", flag&BYT? p->c.value&0377: p->c.value);
		return;

	case AMPER:
		p = p->t.tr1;
		if (flag&HI) {
			printf("seg ");
			pbase(p);
			return;
		}
		pbase(p);
		poff(p->n.offset);
		return;

	case NAME:
		if (p->n.class==REG) {
			printf("%s", flag&BYT? bname[p->n.nloc]: rname[p->n.nloc]);
			return;
		}
		if (flag&FLT)
			printf("%s [", p->t.type==DOUBLE? "qword":
			    p->t.type==FLOAT || LNG(p->t.type)? "dword": "word");
		else if (flag&FARP)
			printf("far [");
		else
			printf("%s [", flag&BYT? "byte": "word");
		i = p->n.offset;
		if (flag&HI)
			i += 2;
		switch(p->n.class) {

		case OFFS:
			printf("%s", rname[p->n.regno]);
			break;

		case FOFFS:
			printf("es:%s", rname[p->n.regno]);
			break;

		case SOFFS:
		case XOFFS:
			printf("%s+", rname[p->n.regno]);

		case EXTERN:
		case STATIC:
			pbase(p);
			break;

		default:
			error("Compiler error: pname");
		}
		poff(i);
		putchar(']');
		return;
	}
	error("compiler error: bad pname");
}

pbase(p)
register union tree *p;
{

	if (p->n.class==SOFFS || p->n.class==STATIC)
		printf("L%d", p->n.nloc);
	else
		printf("%s", p->x.name);
}

/*
 * A constant added to an address.
 */
poff(i)
{
	if (i>0)
		printf("+%d", i);
	else if (i<0)
		printf("%d", i);
}

/*
 * Instructions: of one operand; of a register and an operand, each
 * way round; and of an operand and a number.
 */
ins1(s, p, flag)
char *s;
union tree *p;
{
	printf("\t%s\t", s);
	pname(p, flag);
	putchar('\n');
}

insrp(s, r, p, flag)
char *s;
union tree *p;
{
	printf("\t%s\t%s,", s, flag&BYT? bname[r]: rname[r]);
	pname(p, flag);
	putchar('\n');
}

inspr(s, p, flag, r)
char *s;
union tree *p;
{
	printf("\t%s\t", s);
	pname(p, flag);
	printf(",%s\n", flag&BYT? bname[r]: rname[r]);
}

inspn(s, p, flag, n)
char *s;
union tree *p;
{
	printf("\t%s\t", s);
	pname(p, flag);
	printf(",%d\n", flag&BYT? n&0377: W(n));
}
