/*
 *  C compiler, part 2, for the 80286
 *
 * What reads the first pass's intermediate code, and what makes
 * branches, switches and the entry and exit of a function.  The reader
 * and the logic of the branches are the PDP-11 compiler's (../c11.c);
 * the instructions are the 286's, written for NASM.  And what writes
 * the data's initial values, and puts an in-line function's body where
 * it is called, which are here and not in c10.c with the rest of the
 * code generator: its assembly is about as much as the system's
 * assembler has room for when it runs on the 286.
 */

#include "c1.h"

degree(t)
register union tree *t;
{
	register union tree *t1;

	if (t==NULL || t->t.op==0)
		return(0);
	if (t->t.op == CON)
		return(-3);
	if (t->t.op == AMPER)
		return(-2);
	if (t->t.op==ITOL) {
		if ((t1 = isconstant(t)) && (t1->c.value>=0 || uns(t1)))
			return(-2);
		if (uns(t1 = t->t.tr1) && opdope[t1->t.op]&LEAF)
			return(-1);
	}
	if ((opdope[t->t.op] & LEAF) != 0) {
		if (t->t.type==CHAR || t->t.type==UNCHAR || t->t.type==FLOAT)
			return(1);
		return(0);
	}
	return(t->t.degree);
}

isfloat(t)
register union tree *t;
{

	if ((opdope[t->t.op]&RELAT)!=0)
		t = t->t.tr1;
	if (t->t.type==FLOAT || t->t.type==DOUBLE)
		return('f');
	return(0);
}

/*
 * The bytes an argument of the type takes on the stack.
 */
arlength(t)
{
	if ((unsigned)t>=PTR)
		return(2);
	switch(t) {

	case INT:
	case CHAR:
	case UNSIGN:
	case UNCHAR:
		return(2);

	case UNLONG:
	case LONG:
		return(4);

	case FLOAT:
	case DOUBLE:
		return(8);
	}
	error("botch: peculiar type %d", t);
	return(1024);
}

/*
 * The names of the externals the file has referred to or defined.
 * NASM wants to be told of each that is referred to and not defined,
 * and takes it amiss to be told of one that is defined further on; so
 * it is told at the end.  An assembler in the UNIX tradition takes any
 * name it is not given for one.
 *
 * The same goes for common storage.  A file may say "int x;" and then
 * "int x = 1;", or the first in a header; the first pass asks for
 * common storage at the one and defines x at the other, which is what
 * x then is.
 *
 * The names are kept where the trees are, since both are had from the
 * break and nothing else may be: malloc would take for itself what
 * getblk is about to (c12.c).  A name first met in the middle of an
 * expression is among that expression's trees, and before those are
 * given up it is moved down to where they began, after which they
 * begin above it (xsettle).
 *
 * An in-line function's body is kept after its name (inldef).
 */
struct	xsym {
	struct	xsym *xnext;
	unsigned xsize;		/* of common storage asked for */
	long	xretpos;	/* where a static function's return is */
	char	xdefd;
	char	xinline;	/* an in-line function: the body follows */
	char	xaddr;		/* the name is used but to be called */
	char	xname[1];
};
struct	xsym	*xsyms;
int	laststatic;		/* the last SYMDEF was of something static */
char	statname[40];		/* the static function being compiled */
char	*statfn;

struct xsym *
xlook(s)
register char *s;
{
	register struct xsym *xp;

	for (xp = xsyms; xp; xp = xp->xnext)
		if (strcmp(xp->xname, s)==0)
			return(xp);
	xp = (struct xsym *)getblk(sizeof(*xp) + strlen(s));
	strcpy(xp->xname, s);
	xp->xdefd = 0;
	xp->xinline = 0;
	xp->xaddr = 0;
	xp->xretpos = 0;
	xp->xsize = 0;
	xp->xnext = xsyms;
	xsyms = xp;
	return(xp);
}

/*
 * How much of the space an entry has.
 */
xlen(xp)
register struct xsym *xp;
{
	register n;

	n = strlen(xp->xname);
	if (xp->xinline)
		n += strlen(xp->xname + n + 1) + 1;
	return(sizeof(*xp) + n);
}

/*
 * An in-line function (../c02.c): its name and then its body, which
 * comes to the end of its line of the first pass's output.
 */
inldef(s)
char *s;
{
	char body[INLMAX];
	register struct xsym *xp;
	register char *p;
	register c;

	for (p = body; (c = getchar()) != 0 && c != EOF; )
		if (p < &body[INLMAX-1])
			*p++ = c;
	*p = '\0';
	xp = (struct xsym *)getblk(sizeof(*xp) + strlen(s) + 1 + strlen(body));
	strcpy(xp->xname, s);
	strcpy(xp->xname + strlen(s) + 1, body);
	xp->xdefd = 0;
	xp->xinline = 1;
	xp->xaddr = 0;
	xp->xretpos = 0;
	xp->xsize = 0;
	xp->xnext = xsyms;
	xsyms = xp;
}

/*
 * The name s is used other than to be called: as an address, or in an
 * in-line function's body.
 */
xaddrof(s)
char *s;
{
	xlook(s)->xaddr = 1;
}

/*
 * A static function is called near, with CS pushed, by the functions
 * of the file it is in, whose code is all in one segment; or far, if
 * it is called before it is defined, from that segment too.  One whose
 * address nothing takes returns near, and takes CS off with the
 * return: in protected mode 11 clocks where a far return is 25.  It
 * was written to return far, since whether its address is taken is
 * known only at the end of the file.
 */
nearrets()
{
	register struct xsym *xp;

	fflush(stdout);
	for (xp = xsyms; xp; xp = xp->xnext) {
		if (xp->xretpos==0 || xp->xaddr)
			continue;
		fseek(stdout, xp->xretpos, 0);
		printf("\tret 2");
		fflush(stdout);
	}
	fseek(stdout, 0L, 2);
}

/*
 * The body of the in-line function named s, or NULL if it is not one.
 */
char *
inlbody(s)
char *s;
{
	register struct xsym *xp;

	xp = xlook(s);
	if (!xp->xinline)
		return(NULL);
	return(xp->xname + strlen(xp->xname) + 1);
}

/*
 * A call of an in-line function (__inline, ../c02.c) is its body, put
 * where the call would be.  The first argument is in AX, or in DX:AX if
 * it is a long, as an expression's value is; any others are pushed as
 * for a call, the last first, and taken off after.  The body leaves the
 * function's value where a function does, and may change what a call
 * may: AX, BX, CX, DX, ES and the flags.
 */
inlcall(tree, body)
union tree *tree;
char *body;
{
	register union tree *p;
	register n;

	n = 0;
	p = tree->t.tr2;
	if (p->t.op) {
		while (p->t.op==COMMA) {
			n += comarg(p->t.tr2);
			p = p->t.tr1;
		}
		if (p->t.op==STRASG || isfloat(p))
			error("An in-line function's first argument is a word or a long");
		else
			rcexpr(p, FORREG);
	}
	inlout(body);
	popstk(n);
}

/*
 * The body, a line at a time.  One that begins with a name and a colon
 * is a label, and any other an instruction, which a tab goes before.
 * %%NAME is a label of this one place's own, as in a macro of NASM's.
 * A name that begins with _ is a C name, and the assembler is told of
 * it as of any the file uses (xrefs).
 */
#define	NINL	8		/* labels in one body */
inlout(s)
register char *s;
{
	char names[NINL][16], name[40];
	int nums[NINL];
	register char *p;
	register i;
	int nl, label;

	nl = 0;
	while (*s) {
		for (p = s; *p=='%' || inlid(*p); p++)
			;
		if (p==s || *p!=':') {
			while (*s==' ' || *s=='\t')
				s++;
			putchar('\t');
		}
		while (*s && *s!='\n') {
			if (!inlid(*s) && (s[0]!='%' || s[1]!='%')) {
				putchar(*s++);
				continue;
			}
			/* a word */
			label = *s=='%';
			if (label)
				s += 2;
			for (p = name; inlid(*s); s++)
				if (p < &name[sizeof name - 1])
					*p++ = *s;
			*p = '\0';
			if (!label) {
				if (name[0]=='_')
					xaddrof(name);
				printf("%s", name);
				continue;
			}
			for (i = 0; i<nl && strcmp(names[i], name); i++)
				;
			if (i==nl) {
				if (nl==NINL || strlen(name) >= sizeof names[0]) {
					error("An in-line function's labels are %d of up to %d characters",
					    NINL, sizeof names[0] - 1);
					return;
				}
				strcpy(names[nl], name);
				nums[nl++] = isn++;
			}
			printf("L%d", nums[i]);
		}
		putchar('\n');
		if (*s)
			s++;
	}
}

inlid(c)
register c;
{
	return(c>='a' && c<='z' || c>='A' && c<='Z' || c>='0' && c<='9'
	    || c=='_' || c=='.' || c=='$' || c=='@' || c=='?');
}

/*
 * Give up the trees, and keep the names that are among them.  Those
 * are the first of the list, the newest and so the highest first; the
 * lowest is moved first, so that none is put where another still is.
 */
xsettle()
{
	register struct xsym *xp, *next, *old;
	register char *to, *from;
	int n;

	old = NULL;
	for (xp = xsyms; xp && (char *)xp >= funcbase; xp = next) {
		next = xp->xnext;
		xp->xnext = old;
		old = xp;
	}
	next = old;
	old = xp;
	for (xp = next; xp; xp = next) {
		next = xp->xnext;
		n = xlen(xp);
		if (n&01)
			n++;
		to = funcbase;
		funcbase += n;
		xp->xnext = old;
		old = (struct xsym *)to;
		from = (char *)xp;
		if (to != from)
			while (--n >= 0)
				*to++ = *from++;
	}
	xsyms = old;
	curbase = funcbase;
}

xref(s)
char *s;
{
	xlook(s);
}

xdef(s)
char *s;
{
	xlook(s)->xdefd = 1;
}

/*
 * Has the file defined the name by now?
 */
isdefd(s)
char *s;
{
	return(xlook(s)->xdefd);
}

xcomm(s, n)
char *s;
unsigned n;
{
	register struct xsym *xp;

	xp = xlook(s);
	if (n > xp->xsize)
		xp->xsize = n;
}

xrefs()
{
	register struct xsym *xp;

	for (xp = xsyms; xp; xp = xp->xnext) {
		if (xp->xdefd || xp->xinline)
			continue;
		/*
		 * Where the data is not the program's, what has no value
		 * is defined in the bss, as the cross compiler does it.
		 */
		if (xp->xsize && ssflag) {
			sect(BSS);
			printf("\talignb\t2\n\tglobal\t%s\n%s:\n\tresb\t%u\n",
			    xp->xname, xp->xname, xp->xsize);
		} else if (xp->xsize)
			printf("\tcommon\t%s %u:near\n", xp->xname, xp->xsize);
		else
			printf("\textern\t%s\n", xp->xname);
	}
}

/*
 * Begin, or go back to, a segment.
 */
sect(s)
{
	static int farsaid;

	if (cursect==s)
		return;
	cursect = s;
	if (s==FARSEG) {
		printf("\tsegment\t%s", farseg);
		if (!farsaid++)
			printf(" public align=2 class=FAR_DATA");
		putchar('\n');
		return;
	}
	if (s==BASEDSEG) {
		printf("\tsegment\t%s\n", basedseg);
		return;
	}
	printf("\tsegment\t%s\n", s==PROG? textseg: s==DATA? datseg: bssseg);
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
		 * A far pointer to a function, or in the large model to
		 * a variable: its address and its segment, which is for
		 * whatever loads the program to put there.
		 */
		if (tree->t.op==NTOF && lflag || tree->t.op==XTOF)
			tree = tree->t.tr1;
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
 * A switch.  The value is in AX.  The tables of labels are in the
 * code, where every process running the program shares them.
 */
pswitch(afp, alp, deflab)
struct swtab *afp, *alp;
{
	int ncase, i, j, tabs, worst, best, range;
	register struct swtab *swp, *fp, *lp;
	int *poctab;

	fp = afp;
	lp = alp;
	if (fp==lp) {
		printf("\tjmp\tL%d\n", deflab);
		return;
	}
	isn++;
	if (sort(fp, lp))
		return;
	ncase = lp-fp;
	lp--;
	range = W(lp->swval - fp->swval);
	/* direct switch */
	if (range>0 && range <= 3*ncase) {
		if (fp->swval)
			printf("\tsub\tax,%d\n", fp->swval);
		printf("\tcmp\tax,%d\n\tja\tL%d\n", range, deflab);
		printf("\tshl\tax,1\n\txchg\tax,bx\n\tjmp\t[cs:bx+L%d]\nL%d:\n",
		    isn, isn);
		isn++;
		for (i=fp->swval; ; i++) {
			if (i==fp->swval) {
				printf("\tdw\tL%d\n", fp->swlab);
				if (fp==lp)
					break;
				fp++;
			} else
				printf("\tdw\tL%d\n", deflab);
		}
		return;
	}
	/* simple switch */
	if (ncase<10) {
		for (fp = afp; fp<=lp; fp++)
			breq(fp->swval, fp->swlab);
		printf("\tjmp\tL%d\n", deflab);
		return;
	}
	/* hash switch */
	best = 077777;
	poctab = (int *)getblk(((ncase+2)/2) * sizeof(*poctab));
	for (i=ncase/4; i<=ncase/2; i++) {
		for (j=0; j<i; j++)
			poctab[j] = 0;
		for (swp=fp; swp<=lp; swp++)
			poctab[UNS(swp->swval)%i]++;
		worst = 0;
		for (j=0; j<i; j++)
			if (poctab[j]>worst)
				worst = poctab[j];
		if (i*worst < best) {
			tabs = i;
			best = i*worst;
		}
	}
	i = isn++;
	printf("\txor\tdx,dx\n\tmov\tcx,%d\n\tdiv\tcx\n", tabs);
	printf("\tmov\tbx,dx\n\tshl\tbx,1\n\tjmp\t[cs:bx+L%d]\nL%d:\n", i, i);
	isn++;
	for (i=0; i<tabs; i++)
		printf("\tdw\tL%d\n", isn+i);
	for (i=0; i<tabs; i++) {
		printf("L%d:\n", isn++);
		for (swp=fp; swp<=lp; swp++) {
			if (UNS(swp->swval)%tabs == i)
				breq(UNS(swp->swval)/tabs, swp->swlab);
		}
		printf("\tjmp\tL%d\n", deflab);
	}
}

breq(v, l)
{
	if (v==0)
		printf("\ttest\tax,ax\n");
	else
		printf("\tcmp\tax,%d\n", v);
	printf("\tje\tL%d\n", l);
}

sort(afp, alp)
struct swtab *afp, *alp;
{
	register struct swtab *cp, *fp, *lp;
	int intch, t;

	fp = afp;
	lp = alp;
	while (fp < --lp) {
		intch = 0;
		for (cp=fp; cp<lp; cp++) {
			if (cp->swval == cp[1].swval) {
				error("Duplicate case (%d)", cp->swval);
				return(1);
			}
			if (cp->swval > cp[1].swval) {
				intch++;
				t = cp->swval;
				cp->swval = cp[1].swval;
				cp[1].swval = t;
				t = cp->swlab;
				cp->swlab = cp[1].swlab;
				cp[1].swlab = t;
			}
		}
		if (intch==0)
			break;
	}
	return(0);
}

ispow2(tree)
register union tree *tree;
{
	register int d;

	if (!isfloat(tree) && tree->t.tr2->t.op==CON) {
		d = tree->t.tr2->c.value;
		if (d>1 && (d&(d-1))==0)
			return(d);
	}
	return(0);
}

/*
 * Multiplying by a power of two is a shift; and so, for what is
 * unsigned, is dividing by one, and the remainder is a mask.
 */
union tree *
pow2(tree)
register union tree *tree;
{
	register int d, i;

	if (d = ispow2(tree)) {
		for (i=0; (d>>=1)!=0; i++);
		tree->t.tr2->c.value = i;
		switch (tree->t.op) {

		case TIMES:
			tree->t.op = LSHIFT;
			break;

		case ASTIMES:
			tree->t.op = ASLSH;
			break;

		case DIVIDE:
		case UDIV:
			tree->t.op = ULSH;
			break;

		case ASDIV:
		case ASUDIV:
			tree->t.op = ASULSH;
			break;

		case MOD:
		case UMOD:
			tree->t.op = AND;
			tree->t.tr2->c.value = W((1<<i)-1);
			break;

		case ASMOD:
		case ASUMOD:
			tree->t.op = ASAND;
			tree->t.tr2->c.value = W((1<<i)-1);
			break;

		default:
			error("pow2 botch");
		}
		tree = optim(tree);
	}
	return(tree);
}

/*
 * Is the tree the constant zero?
 */
iszero(p)
register union tree *p;
{
	if (p->t.op==ITOL)
		p = p->t.tr1;
	if (p->t.op==CON)
		return(p->c.value==0);
	if (p->t.op==LCON)
		return(p->l.lvalue==0);
	return(0);
}

/*
 * Branch to the label if the tree is true, or if it is false, as cond
 * is 1 or 0.
 */
cbranch(atree, lbl, cond)
union tree *atree;
register lbl;
{
	int l1, op;
	register union tree *tree;

again:
	if ((tree=atree)==NULL)
		return;
	switch(tree->t.op) {

	case LOGAND:
		if (cond) {
			cbranch(tree->t.tr1, l1=isn++, 0);
			cbranch(tree->t.tr2, lbl, 1);
			label(l1);
		} else {
			cbranch(tree->t.tr1, lbl, 0);
			cbranch(tree->t.tr2, lbl, 0);
		}
		return;

	case LOGOR:
		if (cond) {
			cbranch(tree->t.tr1, lbl, 1);
			cbranch(tree->t.tr2, lbl, 1);
		} else {
			cbranch(tree->t.tr1, l1=isn++, 1);
			cbranch(tree->t.tr2, lbl, 0);
			label(l1);
		}
		return;

	case EXCLA:
		cbranch(tree->t.tr1, lbl, !cond);
		return;

	case SEQNC:
		rcexpr(tree->t.tr1, FOREFF);
		atree = tree->t.tr2;
		goto again;

	case ITOL:
		tree = tree->t.tr1;
		break;

	case QUEST:
		l1 = isn;
		isn += 2;
		cbranch(tree->t.tr1, l1, 0);
		cbranch(tree->t.tr2->t.tr1, lbl, cond);
		branch(l1+1, 0, 0);
		label(l1);
		cbranch(tree->t.tr2->t.tr2, lbl, cond);
		label(l1+1);
		return;

	}
	op = tree->t.op;
	if (isfloat(tree)) {
		fbranch(tree, lbl, cond);
		return;
	}
	if (opdope[op]&RELAT
	 && tree->t.tr1->t.op==ITOL && tree->t.tr2->t.op==ITOL
	 && uns(tree->t.tr1->t.tr1) == uns(tree->t.tr2->t.tr1)) {
		tree->t.tr1 = tree->t.tr1->t.tr1;
		tree->t.tr2 = tree->t.tr2->t.tr1;
		if (op>=LESSEQ && op<=GREAT
		 && uns(tree->t.tr1))
			tree->t.op = op = op+LESSEQP-LESSEQ;
	}
	if (tree->t.type==LONG || tree->t.type==UNLONG
	  || opdope[op]&RELAT&&(tree->t.tr1->t.type==LONG || tree->t.tr1->t.type==UNLONG)) {
		longrel(tree, lbl, cond);
		return;
	}
	rcexpr(tree, FORCC);
	op = tree->t.op;
	if ((opdope[op]&RELAT)==0)
		op = 200+NEQUAL;
	else if (iszero(tree->t.tr2))
		op += 200;
	branch(lbl, op, !cond);
}

/*
 * A jump to the label: on the condition op if c is 0, on its opposite
 * if c is 1, and whatever happens if op is 0.
 *
 * Above 200 the flags are a value's and not a comparison's.  Then the
 * sign flag says whether it is below zero; and if the overflow flag
 * may be set (ccof), <= and > cannot be had from one jump.
 */
branch(lbl, op, c)
{
	register struct instab *ip;
	register char *s;
	int skip;

	if (op==0) {
		printf("\tjmp\tL%d\n", lbl);
		return;
	}
	if (ccof && (op==200+LESSEQ || op==200+GREAT)) {
		if ((op==200+LESSEQ) == (c==0))
			printf("\tjs\tL%d\n\tje\tL%d\n", lbl, lbl);
		else {
			skip = isn++;
			printf("\tjs\tL%d\n\tjne\tL%d\nL%d:\n", skip, lbl, skip);
		}
		return;
	}
	for (ip = branchtab; ip->iop; ip++)
		if (ip->iop==op) {
			s = c? ip->str2: ip->str1;
			if (*s)
				printf("\t%s\tL%d\n", s, lbl);
			return;
		}
	error("No branch for op %d", op);
}

/*
 * How to compare longs: the high words, then if they are equal the
 * low ones, which are unsigned.  The first index is whether the
 * comparison is with 0; the second, which test:
 *	cmp	high words
 *	jg	YES		(first)
 *	jl	NO		(second)
 *	cmp	low words
 *	ja	YES		(third)
 *  NO:
 * The last is indexed by the comparison, from EQUAL.  A 0 is a test
 * that is not needed.
 */
char	lrtab[2][3][10] = {
	0, NEQUAL, LESS, LESS, GREAT, GREAT, LESSP, LESSP, GREATP, GREATP,
	NEQUAL,	0, GREAT, GREAT, LESS, LESS, GREATP, GREATP, LESSP, LESSP,
	EQUAL,NEQUAL,LESSEQP,LESSP, GREATQP,GREATP,LESSEQP,LESSP,GREATQP,GREATP,

	0, NEQUAL, LESS, LESS,	GREATEQ,GREAT, LESSP, LESSP, GREATQP, GREATP,
	NEQUAL,	0, GREAT, 0, 0,	LESS, GREATP, 0, 0, LESSP,
	EQUAL,	NEQUAL,	EQUAL,	0, 0, NEQUAL, EQUAL, 0, 0, NEQUAL,
};

/*
 * Branch on a long: a comparison of two, or one that is or is not
 * zero.
 */
longrel(tree, lbl, cond)
register union tree *tree;
{
	register union tree *l, *r;
	register char *tp;
	int op, zero, l2, mem;

	l = tree;
	r = NULL;
	op = NEQUAL;
	if (opdope[tree->t.op]&RELAT) {
		op = tree->t.op;
		l = tree->t.tr1;
		r = tree->t.tr2;
	}
	if (!cond)
		op = notrel[op-EQUAL];
	zero = r==NULL || iszero(r);
	tp = &lrtab[zero][0][op-EQUAL];
	l2 = 0;
	if (zero) {
		/*
		 * A variable is tested where it is.
		 */
		mem = l->t.op==NAME;
		if ((l->t.op==STAR || l->t.op==FSTAR) && canbx(l)) {
			gaddr(l);
			mem++;
		}
		if (op==EQUAL || op==NEQUAL) {
			if (mem) {
				insrp("mov", AX, l, 0);
				insrp("or", AX, l, HI);
			} else {
				rcexpr(l, FORREG);
				printf("\tor\tax,dx\n");
			}
			branch(lbl, op, 0);
			return;
		}
		if (mem)
			inspn("cmp", l, HI, 0);
		else {
			rcexpr(l, FORREG);
			printf("\ttest\tdx,dx\n");
		}
	} else {
		r = loperands(tree);
		if (r)
			insrp("cmp", DX, r, HI);
		else
			printf("\tcmp\tdx,cx\n");
	}
	if (tp[0])
		branch(lbl, tp[0], 0);
	if (tp[10]) {
		l2 = isn++;
		branch(l2, tp[10], 0);
	}
	if (tp[20]) {
		if (zero) {
			if (mem)
				inspn("cmp", l, 0, 0);
			else
				printf("\ttest\tax,ax\n");
		} else if (r)
			insrp("cmp", AX, r, 0);
		else
			printf("\tcmp\tax,bx\n");
		branch(lbl, tp[20], 0);
	}
	if (l2)
		label(l2);
}

label(l)
{
	printf("L%d:\n", l);
}

/*
 * Take a bytes of arguments off the stack.  CX is free.
 */
popstk(a)
{
	if (a==2)
		printf("\tpop\tcx\n");
	else if (a)
		printf("\tadd\tsp,%d\n", a);
}

werror(s)
char *s;
{

	fprintf(stderr, "%d: %s\n",line,s);
}

/* VARARGS1 */
error(s, p1, p2, p3, p4, p5, p6)
char *s;
{

	nerror++;
	fprintf(stderr, "%d: ", line);
	fprintf(stderr, s, p1, p2, p3, p4, p5, p6);
	putc('\n', stderr);
}

/*
 * A function's entry and exit.
 *
 *		enter	N,0		N bytes of automatic variables
 *		mov	[bp-2],si	if it returns a structure where
 *					it is told to (strasg)
 *		push	si		if SI holds a register variable
 *		push	di		if DI does
 *		...
 *		pop	di
 *		pop	si
 *		leave
 *		ret
 *
 * With no automatic variables the first is push bp and mov bp,sp, which
 * are shorter than enter with nothing; and with one word of them a
 * push after those, which is shorter than either.
 *
 * So the first argument is at [bp+4] and the first automatic variable
 * at [bp-2], as the first pass has them.  That pass finds how much the
 * function needs only at its end, and says so then (SETSTK).  The
 * PDP-11 compiler has the function begin with a jump to its end, where
 * the stack is set and control comes back; here room is left in the
 * output for the entry (SAVE), and filled in when what goes there is
 * known.
 */
#define	PROLEN	72

long	proloc;			/* where the entry goes */
int	minreg;			/* the lowest register variable in use */
int	strret;			/* the function returns a structure where
				   it is told to */

entry()
{
	register i;

	proloc = ftell(stdout);
	for (i = 0; i<PROLEN; i++)
		putchar(' ');
	putchar('\n');
	minreg = BP;
	strret = 0;
}

setstk(n)
{
	char buf[PROLEN+24];
	register char *s;
	register i;

	s = buf;
	if (n>2) {
		sprintf(s, "\tenter\t%u,0\n", UNS(n));
		if (strret)
			strcat(s, "\tmov\t[bp-2],si\n");
	} else {
		strcpy(s, "\tpush\tbp\n\tmov\tbp,sp\n");
		if (n==2)
			strcat(s, strret? "\tpush\tsi\n": "\tpush\tax\n");
	}
	if (minreg<=SI)
		strcat(s, "\tpush\tsi\n");
	if (minreg<=DI)
		strcat(s, "\tpush\tdi\n");
	fflush(stdout);
	fseek(stdout, proloc, 0);
	i = strlen(buf);
	fputs(buf, stdout);
	while (i++ < PROLEN)
		putchar(' ');
	fflush(stdout);
	fseek(stdout, 0L, 2);
}

retrn()
{
	if (minreg<=DI)
		printf("\tpop\tdi\n");
	if (minreg<=SI)
		printf("\tpop\tsi\n");
	printf("\tleave\n");
	if (mflag && statfn) {
		fflush(stdout);
		xlook(statfn)->xretpos = ftell(stdout);
	}
	printf(mflag? "\tretf \n": "\tret\n");
}

/*
 * Read in an intermediate file.
 */
#define	STKS	100
getree()
{
	union tree *expstack[STKS], **sp;
	register union tree *tp;
	register int t, op;
	char s[80];		/* big for asm() stuff & long variable names */
	struct swtab *swp;
	int lbl, cond, jump;
	double ctod();

	xsettle();
	sp = expstack;
	jump = 0;
	for (;;) {
		if (sp >= &expstack[STKS])
			error("Stack overflow botch");
		op = geti();
		if ((op&0177400) != 0177000) {
			error("Intermediate file error");
			exit(1);
		}
		lbl = 0;
		op &= 0377;
		/*
		 * A jump is kept back until what follows it is seen:
		 * the first pass often jumps to the next line, as from
		 * a return at the end of a function.  Its note of the
		 * registers in use may come between.
		 */
		if (jump && op!=LABEL && op!=SETREG) {
			branch(jump, 0, 0);
			jump = 0;
		}
		switch(op) {

	case SINIT:
		printf("\tdw\t%d\n", geti());
		break;

	case EOFC:
		return;

	case BDATA:
		if (geti() == 1) {
			printf("\tdb\t");
			for (;;)  {
				printf("%d", geti()&0377);
				if (geti() != 1)
					break;
				printf(",");
			}
			printf("\n");
		}
		break;

	case PROG:
		sect(PROG);
		break;

	case DATA:
		sect(DATA);
		break;

	case BSS:
		sect(BSS);
		break;

	case FARSEG:
		sect(FARSEG);
		break;

	/*
	 * The based segment, which is said here before anything is put
	 * in it or its name is used for its selector.
	 */
	case SEGNAME:
		outname(s);
		strncpy(basedseg, s, sizeof basedseg - 1);
		printf("\tsegment\t%s public align=2 class=FAR_DATA\n",
		    basedseg);
		t = cursect;
		cursect = 0;
		if (t)
			sect(t);
		break;

	case BASEDSEG:
		sect(BASEDSEG);
		break;

	case SYMDEF:
		outname(s);
		if (*s)
			printf("\tglobal\t%s\n", s);
		laststatic = *s=='\0';
		break;

	case INLDEF:
		outname(s);
		inldef(s);
		break;

	case RETRN:
		retrn();
		break;

	case CSPACE:
		outname(s);
		xcomm(s, UNS(geti()));
		break;

	case SSPACE:
		t = geti();
		if (cursect==BSS || cursect==FARSEG || cursect==BASEDSEG)
			printf("\tresb\t%u\n", UNS(t));
		else
			printf("\ttimes %u db 0\n", UNS(t));
		totspace += (unsigned)UNS(t);
		break;

	case EVEN:
		if (cursect==BSS)
			printf("\talignb\t2\n");
		else if (cursect==DATA || cursect==FARSEG ||
		    cursect==BASEDSEG)
			printf("\talign\t2,db 0\n");
		else
			printf("\talign\t2\n");
		break;

	case SAVE:
		entry();
		break;

	case SETSTK:
		setstk(geti());
		break;

	case PROFIL:
		t = geti();
		outname(s);
		error("Profiling is not done yet");
		break;

	case ASSEM:
		outname(s);
		printf("%s\n", s);
		break;

	/*
	 * What a debugger would be told of a local name.
	 */
	case SNAME:
		outname(s);
		printf("; %s = L%d\n", s+1, geti());
		break;

	case ANAME:
		outname(s);
		printf("; %s = [bp", s+1);
		poff(geti());
		printf("]\n");
		break;

	case RNAME:
		outname(s);
		printf("; %s = %s\n", s+1, rname[geti()]);
		break;

	case SWIT:
		t = geti();
		line = geti();
		xsettle();
		while(swp=(struct swtab *)getblk(sizeof(*swp)), swp->swlab = geti())
			swp->swval = geti();
		pswitch((struct swtab *)funcbase, swp, t);
		break;

	case C3BRANCH:		/* for fortran [sic] */
		lbl = geti();
		geti();
		geti();
		goto xpr;

	case CBRANCH:
		lbl = geti();
		cond = geti();

	case EXPR:
	xpr:
		line = geti();
		if (sp != &expstack[1]) {
			error("Expression input botch");
			exit(1);
		}
		--sp;
		*sp = tp = optim(*sp);
		if (op==CBRANCH)
			cbranch(tp, lbl, cond);
		else if (op==EXPR)
			rcexpr(tp, FOREFF);
		else
			error("The arithmetic IF is not done yet");
		xsettle();
		break;

	case NAME:
		t = geti();
		if (t==EXTERN) {
			tp = getblk(sizeof(struct xtname));
			tp->t.type = geti();
			outname(s);
			tp->x.name = (char *)getblk(strlen(s) + 1);
			strcpy(tp->x.name, s);
			xref(s);
		} else {
			tp = getblk(sizeof(struct tname));
			tp->t.type = geti();
			tp->n.nloc = geti();
		}
		tp->t.op = NAME;
		tp->n.class = t;
		tp->n.regno = 0;
		tp->n.offset = 0;
		*sp++ = tp;
		break;

	case CON:
		t = geti();
		*sp++ = tconst(geti(), t);
		break;

	case LCON:
		geti();	/* ignore type, assume long */
		t = geti();
		op = geti();
		if (t==0 && op>=0 || t == -1 && op<0) {
			*sp++ = tnode(ITOL, LONG, tconst(op, INT), TNULL);
			break;
		}
		tp = getblk(sizeof(struct lconst));
		tp->t.op = LCON;
		tp->t.type = LONG;
		tp->l.lvalue = ((LTYPE)t<<16) + UNS(op);
		*sp++ = tp;
		break;

	case FCON:
		t = geti();
		outname(s);
		tp = getblk(sizeof(struct ftconst));
		tp->t.op = FCON;
		tp->t.type = t;
		tp->f.value = isn++;
		tp->f.fvalue = ctod(s);
		*sp++ = tp;
		break;

	case FSEL:
		tp = tnode(FSEL, geti(), *--sp, TNULL);
		t = geti();
		tp->t.tr2 = tnode(COMMA, INT, tconst(geti(), INT), tconst(t, INT));
		if (tp->t.tr2->t.tr1->c.value==16)
			tp = paint(tp->t.tr1, tp->t.type);
		*sp++ = tp;
		break;

	case STRASG:
		tp = getblk(sizeof(struct fasgn));
		tp->t.op = STRASG;
		tp->t.type = geti();
		tp->F.mask = geti();
		tp->t.tr1 = *--sp;
		tp->t.tr2 = NULL;
		*sp++ = tp;
		break;

	case NULLOP:
		*sp++ = tnode(0, 0, TNULL, TNULL);
		break;

	case LABEL:
		t = geti();
		if (jump && jump!=t)
			branch(jump, 0, 0);
		jump = 0;
		label(t);
		break;

	case NLABEL:
		outname(s);
		xdef(s);
		printf("%s:\n", s);
		break;

	case RLABEL:
		outname(s);
		xdef(s);
		printf("%s:\n", s);
		statfn = NULL;
		if (laststatic) {
			strncpy(statname, s, sizeof statname - 1);
			statfn = statname;
		}
		break;

	case BRANCH:
		jump = geti();
		break;

	case SETREG:
		t = geti();
		if (t < minreg)
			minreg = t;
		break;

	default:
		if (opdope[op]&BINARY) {
			if (sp < &expstack[1]) {
				error("Binary expression botch");
				exit(1);
			}
			tp = *--sp;
			--sp;
			*sp = tnode(op, geti(), *sp, tp);
			sp++;
		} else
			sp[-1] = tnode(op, geti(), sp[-1], TNULL);
		break;
	}
	}
}

geti()
{
	register short i;

	i = getchar() & 0xff;
	i |= (getchar() & 0xff) << 8;
	return(i);
}

outname(s)
register char *s;
{
	register int c;

	while (c = getchar())
		*s++ = c;
	*s++ = '\0';
}

/*
 * Structures.  One of a word or of two is assigned, passed and
 * returned as an int or a long is.  Any other is copied with the
 * string instructions, whose registers are saved, since they may hold
 * register variables.
 *
 * A function that returns one of the others is told where to put it:
 * its caller gives it the address in SI (docs/abi.md), and saves its
 * own SI round the call.  The function keeps the address in the first
 * word of its frame, where the first pass has left room for it.
 *
 * What a function is given to fill is nothing else that it can reach.
 * So an automatic variable is filled directly, as is what the caller
 * was itself given; anything else is filled from a copy on the stack.
 */
strasg(atp)
union tree *atp;
{
	register union tree *tp, *l, *r;
	register nwords;
	int direct;

	nwords = atp->F.mask/2;
	tp = atp->t.tr1;
	while (tp->t.op == SEQNC) {
		rcexpr(tp->t.tr1, FOREFF);
		tp = tp->t.tr2;
	}
	if (tp->t.op==RFORCE) {		/* function return */
		tp = tp->t.tr1;
		if (nwords<=2) {
			rcexpr(paint(tp, nwords==1? INT: LONG), FORREG);
			return;
		}
		if (ssflag)
			error("a structure of more than two words is not \
returned where DS is not SS");
		strret = 1;
		l = getblk(sizeof(struct tname));
		l->t.op = NAME;
		l->t.type = STRUCT+PTR;
		l->n.class = OFFS;
		l->n.regno = BP;
		l->n.offset = -2;
		if (tp->t.op==CALL) {
			gcall(tp, l, 0);
			return;
		}
		atp->t.tr1 = tnode(ASSIGN, STRUCT, tnode(STAR, STRUCT, l, TNULL), tp);
		strasg(atp);
		return;
	}
	if (tp->t.op==CALL) {		/* its value is not wanted */
		if (nwords<=2) {
			rcexpr(paint(tp, nwords==1? INT: LONG), FOREFF);
			return;
		}
		printf("\tsub\tsp,%d\n", nwords*2);
		gcall(tp, TNULL, 1);
		printf("\tadd\tsp,%d\n", nwords*2);
		return;
	}
	if (tp->t.op != ASSIGN) {
		error("Illegal structure operation");
		return;
	}
	if (nwords<=2) {
		rcexpr(paint(tp, nwords==1? INT: LONG), FOREFF);
		return;
	}
	l = tp->t.tr1;
	r = tp->t.tr2;
	if (l->t.op!=NAME && l->t.op!=STAR && l->t.op!=FSTAR
	 || r->t.op!=NAME && r->t.op!=STAR && r->t.op!=FSTAR
	 && r->t.op!=CALL) {
		error("unimplemented structure assignment");
		return;
	}
	/*
	 * To or from what a far pointer points at: both addresses are
	 * made far.  So is every one where DS is not SS, and one may be
	 * in the frame and the other not.
	 */
	if (l->t.op==FSTAR || r->t.op==FSTAR || ssflag) {
		if (r->t.op==CALL) {
			printf("\tsub\tsp,%d\n", nwords*2);
			gcall(r, TNULL, 1);
			faraddr(l);
			printf("\tmov\tbx,ax\n\tmov\tcx,dx\n");
			printf("\tmov\tax,sp\n\tmov\tdx,%s\n",
			    ssflag? "ss": dseg);
			fmovs(nwords);
			printf("\tadd\tsp,%d\n", nwords*2);
			return;
		}
		faraddr(l);
		printf("\tpush\tdx\n\tpush\tax\n");
		faraddr(r);
		printf("\tpop\tbx\n\tpop\tcx\n");
		fmovs(nwords);
		return;
	}
	direct = isauto(l);
	l = optim(tnode(AMPER, STRUCT+PTR, l, TNULL));
	if (r->t.op==CALL) {
		if (direct) {
			gcall(r, l, 0);
			return;
		}
		printf("\tsub\tsp,%d\n", nwords*2);
		gcall(r, TNULL, 1);
		rcexpr(l, FORREG);
		printf("\tmov\tbx,sp\n");
		movs(nwords);
		printf("\tadd\tsp,%d\n", nwords*2);
		return;
	}
	r = optim(tnode(AMPER, STRUCT+PTR, r, TNULL));
	if (easy(r, BX)) {
		rcexpr(l, FORREG);
		greg(r, BX);
	} else {
		rcexpr(r, FORSP);
		rcexpr(l, FORREG);
		printf("\tpop\tbx\n");
	}
	movs(nwords);
}

/*
 * Is p a variable in the frame?
 */
isauto(p)
register union tree *p;
{
	return(p->t.op==NAME && p->n.class==OFFS && p->n.regno==BP);
}

/*
 * Copy n words from where BX points to where AX does.  In the large
 * model both are in SS, and DS is made it too.
 */
movs(n)
register n;
{
	printf("\tpush\tsi\n\tpush\tdi\n");
	printf("\tmov\tdi,ax\n\tmov\tsi,bx\n");
	if (lflag)
		printf("\tmov\tax,ss\n\tmov\tds,ax\n\tmov\tes,ax\n");
	else
		printf("\tpush\tds\n\tpop\tes\n");
	if (n<=4)
		while (n--)
			printf("\tmovsw\n");
	else
		printf("\tmov\tcx,%d\n\trep movsw\n", n);
	printf("\tpop\tdi\n\tpop\tsi\n");
}

/*
 * The address of the structure p, as a far pointer, in DX:AX.
 */
faraddr(p)
register union tree *p;
{
	register frame;

	if (p->t.op==FSTAR) {
		rcexpr(p->t.tr1, FORREG);
		return;
	}
	/* (asked first: making its address makes p something else) */
	frame = ssflag && isauto(p);
	rcexpr(optim(tnode(AMPER, STRUCT+PTR, p, TNULL)), FORREG);
	printf("\tmov\tdx,%s\n", frame? "ss": dseg);
}

/*
 * Copy n words from where DX:AX points to where CX:BX does.
 */
fmovs(n)
register n;
{
	printf("\tpush\tsi\n\tpush\tdi\n\tpush\tds\n");
	printf("\tmov\tdi,bx\n\tmov\tes,cx\n\tmov\tsi,ax\n\tmov\tds,dx\n");
	if (n<=4)
		while (n--)
			printf("\tmovsw\n");
	else
		printf("\tmov\tcx,%d\n\trep movsw\n", n);
	printf("\tpop\tds\n\tpop\tdi\n\tpop\tsi\n");
}

/*
 * Reduce the degree-of-reference by one.
 * e.g. turn "ptr-to-int" into "int".
 */
decref(t)
register t;
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
{
	return(((t&~TYPE)<<TYLEN) | (t&TYPE) | PTR);
}
