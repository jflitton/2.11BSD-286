#if	!defined(lint) && defined(DOSCCS)
static	char	sccsid[] = "@(#)ansi.c	1.1 (2.11BSD) 2025/12/25";
#endif

#include "c0.h"

#define ANSIDEBUG

/* Used in prototype storage */
#define	ELLIPSIS	(PTR|AUTO)	/* impossible type */
#define	PNULL		(PTR|REG)	/* impossible type */

#ifdef	i286
#define ISPTR(t)	(((t) & XTYPE) == PTR || ((t) & XTYPE) == FPTR \
			|| ISBASED(t))
#else
#define ISPTR(t)	(((t) & XTYPE) == PTR)
#endif
#define ISFTN(t)	(((t) & XTYPE) == FUNC)
#define ISARY(t)	(((t) & XTYPE) == ARRAY)

static FILE *protofile;
#define protopos(poff)	fseek(protofile, (long)poff, 0);
#define	protoget()	getw(protofile)
#define	protobase()	ftell(protofile)
#define	pushproto(t)	putw(t, protofile)

#define	NCPROT	20 /* max number of args checked */
/*
 * Check for (and declare) an ANSI-style function (or prototype).
 */
ansidecl()
{
	static struct nmlist argtype;
	register int s, pp;
	struct nmlist nm, nm2;
	struct tdim dim;
	int skw, t, i, rv;
	int prot[NCPROT];

	if ((peeksym = s = symbol()) == RPARN)
		return 0;

	/* identifier list? */
	if (s == NAME && csym->hclass != TYPEDEF) {
		declare(ARG1, &argtype, 0);
		peeksym = RPARN;
		return 0;
	}
	pp = 0;

	/* special case: x(void); or x(void){ */
	if (s == KEYW && cval == VOID && nextchar() == ')') {
		peeksym = -1;
		fseek(protofile, (long)0, 2);
		rv = (int)protobase();
		goto ret;
	}

	do {
		skw = ARG;
		if ((s = symbol()) == ELLIPS)
			break;
		peeksym = s;

		memset(&nm, 0, sizeof(nm));
		memset(&nm2, 0, sizeof(nm2));
		if (getkeywords(&skw, &nm) == 0)
			error("prototype error");
		dim.rank = 0;

		s = getype(&dim, &nm2);
		t = nm.htype & ~TYPE; /* s = t1, t = type */
		while (s & XTYPE) {
#ifdef	i286
			t = t << TYLEN | ptrto(t, s & XTYPE);
#else
			t = t << TYLEN | (s & XTYPE);
#endif
			s = (unsigned)s >> TYLEN;
		}
		t |= (nm.htype & TYPE);
		if (ISARY(t))
#ifdef	i286
			t = t - ARRAY + DPTR;
#else
			t = t - ARRAY + PTR;
#endif
		if (pp < NCPROT)
			prot[pp++] = t;
		/*
		 * Declaration of functions are only when blklev == 1, 
		 * otherwise it's a prototype.
		 * For other declarations, fake an auto variable.
		 */
		if (blklev == 1 && defsym) {
			if (defsym->hblklev < blklev)
				defsym = pushdecl(defsym);
			if (paraml==NULL)
				paraml = defsym;
			else
				parame->sparent = (union str *)defsym;
			parame = defsym;

			defsym->hclass = skw;
			defsym->htype = t;
			if (dim.rank)
				defsym->hsubsp =
				    (int *)Dblock(dim.rank * sizeof(int));
			for (i = 0; i < dim.rank; i++)
				defsym->hsubsp[i] = dim.dimens[i];
			defsym->hstrp = nm.hstrp;
		} else if (defsym) { /* && blklev > 1 */
			if (defsym->hblklev == blklev)
				defsym->hclass = AUTO;
		}
		s = symbol();
	} while (s == COMMA);

	fseek(protofile, (long)0, 2);
	rv = (int)protobase();
	for (t = 0; t < pp; t++)
		pushproto(prot[t]);

	if (s == ELLIPS) {
		pushproto(ELLIPSIS);
	} else
		peeksym = s;

ret:	pushproto(PNULL);

	/*
	 * Use the block end cleanup routines to remove unused variables.
	 */
	if (blklev > 1) {
		blkend();
		blklev++;
	}
	return rv;
}

protosetup(buf)
	char *buf;
{
	static char tbuf[] = "/tmp/proto.XXXXXX";

	mktemp(tbuf);
	protofile = fopen(tbuf, "w+");
	setbuf(protofile, buf);
	unlink(tbuf);
	pushproto(0); /* not first entry */
}

static int prv;

/*
 * check prototype against arguments.
 * return -1 on error, 0 on OK, 1 if ...
 */
static union tree *
cktypep(p)
	register union tree *p;
{
	register t, u;
	int e;

	if (prv) {
#ifdef	i286
		ssargs(p);
#endif
		return p;
	}

	if ((t = protoget()) == ELLIPSIS) {
		prv = 1;	/* anything is accepted, check no more */
#ifdef	i286
		ssargs(p);
#endif
		return p;
	}
	if (t == (u = p->t.type))
		return p;	/* same type, correct */

	e = nerror;
	*cp++ = block(ETYPE, t, 0, 0, 0, 0);
	*cp++ = p;
	build(CAST);
	p = *--cp;
	if (e == nerror) {
		if ((t == (PTR|VOID) && ISPTR(u)) ||
		    (ISPTR(t) && u == (PTR|VOID)))
			;
#ifdef	i286
		else if (ISFAR(t) && t-(FPTR-PTR) == u)
			;	/* a near pointer made far, as it is for */
		else if (ISFAR(t) && ISBASED(u) && t-(FPTR-(u&XTYPE)) == u)
			;	/* and one into SS or the based segment */
		else if ((t == (FPTR|VOID) && ISPTR(u)) ||
		    (ISFAR(t) && u == (FPTR|VOID)))
			;	/* far pointers to void, as near ones above */
#endif
		else if (ISPTR(t) && ISPTR(u))
			werror("mixed pointer assignment");
		else if (ISPTR(t) && u == INT &&
		    p->t.op == CON && p->c.value == 0)
			;
#ifdef	i286
		/*
		 * A zero made a far pointer, or the 0L that NULL is in
		 * the large model.
		 */
		else if (ISPTR(t) && (u == INT || u == LONG || u == UNLONG) &&
		    p->t.op == LCON && p->l.lvalue == 0)
			;
#endif
		else if (ISPTR(t) || ISPTR(u))
			werror("illegal combination of pointer and integer");
	}
	return p;
}

static union tree *
pcheck(p)
	register union tree *p;
{

	if (p->t.op == COMMA) {
		p->t.tr1 = pcheck(p->t.tr1);
		p->t.tr2 = cktypep(p->t.tr2);
	} else
		p = cktypep(p);
	return p;
}

union tree *
protochk(poff, p)
	register union tree *p;
{
	register i;

	if (poff == 0) {
#ifdef	i286
		if (p->t.op != NULLOP)
			ssargs(p);
#endif
		return p; /* no prototype */
	}
	protopos(poff);
	if (p->t.op == NULLOP) { /* x(); */
		if (protoget() != PNULL)
			goto perr;
		return p;
	}
	prv = 0;
	p = pcheck(p);
	if (prv >= 0) {
		if (prv == 0 && (i = protoget()) != PNULL && i != ELLIPSIS)
			goto perr;
		return p;
	}

perr:	error("call does not match prototype");
	return p;
}

/*
 * Check correctness of a function redeclaration.
 */
declproto(sym, dim)
	struct nmlist *sym;
	struct tdim *dim;
{
	register i;
	int prot[NCPROT];

	if (dim->rank == 0 || dim->dimens[0] == 0)
		return; /* no new prototype */
	if (sym->hsubsp == NULL || sym->hsubsp[0] == 0) {
		/* prototype declared */
		sym->hsubsp = (int *)Dblock(dim->rank*sizeof(dim->rank));
		for (i = 0; i < dim->rank; i++)
			sym->hsubsp[i] = dim->dimens[i];
		return;
	}
	/* compare prototypes.  Only the first 20 */
	protopos(dim->dimens[0]);
	i = 0;
	do {
		prot[i++] = protoget();
	} while (i < NCPROT && prot[i-1] != PNULL);
	protopos(sym->hsubsp[0]);
	i = 0;
	do {
		if (prot[i++] != protoget()) {
			error("%s redeclared", sym->name);
			break;
		}
	} while (i < NCPROT && prot[i-1] != PNULL);
}

/*
 * verify that an old-style function matches a previous prototype.
 */
protokrchk(l)
	register struct nmlist *l;
{
	register i, j;
	int prot[NCPROT];

	if (l == NULL || funcsym->hsubsp == NULL || funcsym->hsubsp[0] == 0)
		return; /* no prototype */
	i = 0;
	while (l && i < NCPROT) {
		prot[i] = l->htype;
		if (ISARY(prot[i]))
#ifdef	i286
			prot[i] = prot[i] - ARRAY + DPTR;
#else
			prot[i] = prot[i] - ARRAY + PTR;
#endif
		i++;
		l = &l->sparent->P;
	}
	if (i < NCPROT)
		prot[i] = PNULL;

	protopos(funcsym->hsubsp[0]);
	i = 0;
	do {
		if ((j = protoget()) == ELLIPSIS)
			continue;
		if (prot[i++] != j) {
			error("%s prototype error", funcsym->name);
			break;
		}
	} while (i < NCPROT && prot[i-1] != PNULL);
}

#ifdef ANSIDEBUG
/*
 * output a nice description of the type of t
 */
tprint(t)
{
	char *s;
	static char * tnames[] = {
		"int",
		"char",
		"float",
		"double",
		"struct",
		"??? 5 ",
		"long",
		"unsigned",
		"uchar",
		"ulong",
		"void",
		"...",
		"??? 12 ",
		"??? 13 ",
		"PNULL",
		"??? 15 ",
		};

	for(;; t = decref(t)) {
		if (ISPTR(t))
			s = "PTR ";
		else if (ISFTN(t))
			s = "FTN ";
		else if (ISARY(t))
			s = "ARY ";
		else {
			fprintf(stderr, "%s", tnames[t]);
			return;
		}
		fprintf(stderr, "%s", s);
	}
}

prtp(p, t)
	char *p;
{
	fprintf(stderr, "%s: ", p);
	tprint(t);
	fprintf(stderr, "\n");
}

prsp(p, sp)
	char *p; struct nmlist *sp;
{
	fprintf(stderr, "%s: nmlist %p\n", p, sp);
	if (sp == NULL)
		return;
	fprintf(stderr, "class %d flags %x type %x offset %d blklev %d name %s\n", sp->hclass, sp->hflag, sp->htype, sp->hoffset, sp->hblklev, sp->name);
}

prpr(c, p)
	char *c;
	union tree *p;
{
	fprintf(stderr, "%s %o) op %d type %x tr1 %o tr2 %o\n",
		c, p, p->t.op, p->t.type, p->t.tr1, p->t.tr2);
}
#endif
