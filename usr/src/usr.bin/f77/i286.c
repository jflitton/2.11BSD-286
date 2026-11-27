/* "@(#)i286.c 1.0 (2.11BSD) 2026/11/26"; */

#include "defs"
#include "string_defs"
#include "dmrdefs"

/*
      80286-SPECIFIC PRINTING ROUTINES

  What pdp11.c is to the PDP-11.  The code goes to the C compiler's
  second pass for the 286 (lib/ccom/i286) as the PDP-11's goes to its
  own; the data go to it too, as its operators (i286x.c), where the
  PDP-11's were assembly.  A constant is written as the words it is,
  low first: a long's, and a real's or a double precision's in the
  287's form.
*/

int maxregvar = 0;
int regnum[] = { 3 };

/*
 * A return.  c2 takes a routine to end at the ret after its leave
 * (lib/c2/i286), and a procedure's prologues come after its body and
 * jump back into it: so each return jumps to the one return, which is
 * after the prologues (prendproc).
 */
LOCAL int retlab;

goret(type)
int type;
{
	if(retlab == 0)
		retlab = newlabel();
	putgoto(retlab);
}

/*
 * move argument slot arg1 (relative to the first argument)
 * to slot arg2 (relative to ARGREG)
 */

mvarg(type, arg1, arg2)
int type, arg1, arg2;
{
mvarg1(arg1+ARGOFFSET, arg2);
if(type == TYLONG)
	mvarg1(arg1+ARGOFFSET+2, arg2+2);
}

mvarg1(m, n)
int m, n;
{
	p2reg(ARGREG, P2SHORT|P2PTR);
	p2op2(P2ICON, P2SHORT);
	p2i(n);
	p2op2(P2PLUS, P2SHORT|P2PTR);
	p2op2(P2INDIRECT, P2SHORT);
	p2reg(AUTOREG, P2SHORT|P2PTR);
	p2op2(P2ICON, P2SHORT);
	p2i(m);
	p2op2(P2PLUS, P2SHORT|P2PTR);
	p2op2(P2INDIRECT, P2SHORT);
	p2op2(P2ASSIGN, P2SHORT);
	putstmt();
}

/*
 * A constant goes to the second pass as a line of assembly; the same
 * line is what a DATA statement's is, in the file the driver sorts.
 */
LOCAL prdata(fp, s)
FILE *fp;
char *s;
{
if(fp == initfile)
	fprintf(fp, "%s\n", s);
else
	asmline(fp, s);
}

prlabel(fp, k)
FILE *fp;
int k;
{
char buff[20];

sprintf(buff, "L%d", k);
asmlabel(fp, buff);
}

prconi(fp, type, n)
FILE *fp;
int type;
ftnint n;
{
char buff[30];

if(type == TYLONG)
	sprintf(buff, "\tdw\t%u,%u", (unsigned) n, (unsigned) (n >> 16));
else
	sprintf(buff, "\tdw\t%u", (unsigned) n);
prdata(fp, buff);
}

prcona(fp, a)
FILE *fp;
ftnint a;
{
char buff[30];

sprintf(buff, "\tdw\tL%ld", a);
prdata(fp, buff);
}

prconr(fp, type, x)
FILE *fp;
int type;
double x;
{
union {
	float f;
	double d;
	unsigned short w[4];
	} u;
char buff[50];

if(type == TYREAL)
	{
	u.f = x;
	sprintf(buff, "\tdw\t%u,%u", u.w[0], u.w[1]);
	}
else
	{
	u.d = x;
	sprintf(buff, "\tdw\t%u,%u,%u,%u", u.w[0], u.w[1], u.w[2], u.w[3]);
	}
prdata(fp, buff);
}

preven(k)
int k;
{
if(k > 1)
	asmop(asmfile, P2EVEN);
}

char *memname(stg, mem)
int stg, mem;
{
static char s[20];

switch(stg)
	{
	case STGCOMMON:
	case STGEXT:
		sprintf(s, "_%s", varstr(XL, extsymtab[mem].extname) );
		break;

	case STGBSS:
	case STGINIT:
		sprintf(s, "v.%d", mem);
		break;

	case STGCONST:
		sprintf(s, "L%d", mem);
		break;

	case STGEQUIV:
		sprintf(s, "q.%d", mem);
		break;

	default:
		error("memname: invalid vstg %d", stg, 0, FATAL1);
	}
return(s);
}

prlocvar(s, len)
char *s;
ftnint len;
{
asmlabel(asmfile, s);
prskip(asmfile, len);
}

/*
 * A name the file defines is global.  One it uses and does not define
 * the second pass declares extern itself, and common storage it is told
 * of, which is of 64K at most: its size is a word.
 */
prext(name, leng, init)
char *name;
ftnint leng;
int init;
{
char buff[XL+20];

if(init)
	{
	sprintf(buff, "\tglobal\t_%s", name);
	asmline(asmfile, buff);
	}
else if(leng > 0)
	{
	if(leng > 65535L)
		error("common block %s is more than 64K", name, 0, ERR1);
	sprintf(buff, "_%s", name);
	asmop(asmfile, P2CSPACE);
	asmstr(asmfile, buff);
	asmint(asmfile, (int) leng);
	}
}

prendproc()
{
	if(retlab)
		{
		putlabel(retlab);
		p2op(P2RETURN);
		retlab = 0;
		}
}

prtail()
{
	p2op(P2EOF);
}

/*
 * An entry's prologue.  The second pass saves SI and DI, which the
 * prologue and every return of the procedure agree on as they must: SI
 * has the arguments, and DI may be a register variable.  It is told so
 * after SAVE, which forgets what it was told before.
 */
prolog(ep, argvec)
struct entrypoint *ep;
struct addrblock *argvec;
{
int i, argslot, proflab;
register chainp p;
register struct nameblock *q;
register struct dimblock *dp;
char *funcname;
struct constblock *mkaddcon();

if(procclass == CLMAIN) {
	funcname = "MAIN_";
	prentry(funcname);
}

if(ep->entryname) {
	funcname = varstr(XL, ep->entryname->extname);
	prentry(funcname);
}

if(procclass == CLBLOCK)
	return;
if(profileflag)
	proflab = newlabel();
p2op(P2SAVE);
p2op2(P2SETREG, ARGREG-MAXREGVAR);
if(profileflag) {
	p2op2(P2PROFIL, proflab);
	p2str(funcname);
}
p2op2(P2SETSTK, ( (((int) autoleng)+1) & ~01) );

if(argvec == NULL)
	addreg(argloc = ARGOFFSET);
else
	{
	addreg( argloc = argvec->memoffset->xconst.ci );
	if(proctype == TYCHAR)
		{
		mvarg(TYADDR, 0, chslot);
		mvarg(TYLENG, SZADDR, chlgslot);
		argslot = SZADDR + SZLENG;
		}
	else if( ISCOMPLEX(proctype) )
		{
		mvarg(TYADDR, 0, cxslot);
		argslot = SZADDR;
		}
	else
		argslot = 0;

	for(p = ep->arglist ; p ; p =p->nextp)
		{
		q = p->datap;
		mvarg(TYADDR, argslot, q->vardesc.varno);
		argslot += SZADDR;
		}
	for(p = ep->arglist ; p ; p = p->nextp)
		{
		q = p->datap;
		if(q->vtype==TYCHAR || q->vclass==CLPROC)
			{
			if( q->vleng && ! ISCONST(q->vleng) )
				mvarg(TYLENG, argslot, q->vleng->memno);
			argslot += SZLENG;
			}
		}
	}

for(p = ep->arglist ; p ; p = p->nextp)
	if(dp = ( (struct nameblock *) (p->datap) ) ->vdim)
		{
		for(i = 0 ; i < dp->ndim ; ++i)
			if(dp->dims[i].dimexpr)
				puteq( fixtype(cpexpr(dp->dims[i].dimsize)),
					fixtype(cpexpr(dp->dims[i].dimexpr)));
		if(dp->basexpr)
			puteq( 	cpexpr(fixtype(dp->baseoffset)),
				cpexpr(fixtype(dp->basexpr)));
		}

if(typeaddr)
	puteq( cpexpr(typeaddr), mkaddcon(ep->typelabel) );
putgoto(ep->entrylabel);
}

prentry(s)
char *s;
{
	p2op(P2RLABEL);
	putc('_', textfile);
	p2str(s);
}

addreg(k)
int k;
{
	p2reg(ARGREG, P2SHORT);
	p2reg(AUTOREG, P2SHORT);
	p2op2(P2ICON, P2SHORT);
	p2i(k);
	p2op2(P2PLUS, P2SHORT);
	p2op2(P2ASSIGN, P2SHORT);
	putstmt();
}

/*
 * The PDP-11's told adb where each variable was, by names of its
 * assembler's that the 286's has not.
 */
prdbginfo()
{
}
