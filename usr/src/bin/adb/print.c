#include "defs.h"
#include <sys/file.h>

	MSG	LONGFIL;
	MSG     NOTOPEN;
	MSG     A68BAD;
	MSG     A68LNK;
	MSG     BADMOD;
	MSG	NOPCS;
	MAP     txtmap;
	MAP     datmap;
extern	struct	SYMbol	*symbol;
	int     kernel;
	int	infile;
	int	outfile;
	char	*lp;
	int	maxoff;
	int	maxpos;
	int	radix = 16;
	long	localval;
	BKPTR   bkpthead;
	char    lastc;
	u_int	corhdr[];
	u_int	*uar0;
	int	fcor;
	char	*errflg;
	int	signo;
	long	dot;
	long	var[];
	char	*symfil;
	char	*corfil;
	int	pid;
	long	adrval;
	int	adrflg;
	long	cntval;
	int	cntflg;
	int	model;
	u_int	frame;		/* findfn.c */
	long	retpc;
	u_int	argp;
	int	narg;
	u_int	ssel;

	REGLIST reglist [] = {
		"ax", AX,
		"bx", BX,
		"cx", CX,
		"dx", DX,
		"si", SI,
		"di", DI,
		"bp", BP,
		"sp", RSP,
		"ip", IP,
		"cs", CS,
		"ds", DS,
		"es", ES,
		"ss", SS,
		"fl", FLAGS
		};


/* general printing routines ($) */

printtrace(modif)
{
	int	i, stack;
	register BKPTR       bkptr;
	char	*comptr;
	register struct	SYMbol	*symp;

	IF cntflg==0 THEN cntval = -1; FI

	switch (modif) {

	    case '<':
			IF cntval == 0
			THEN	WHILE readchar() != EOR
				DO OD
				lp--;
				break;
			FI
			IF rdc() == '<'
			THEN	stack = 1;
			ELSE	stack = 0; lp--;
			FI
			/* fall thru ... */
	    case '>':
		{
			char		file[64];
			char		Ifile[128];
			extern char	*Ipath;
			int             index;

			index=0;
			IF rdc()!=EOR
			THEN    REP file[index++]=lastc;
				    IF index>=63 THEN error(LONGFIL); FI
				PER readchar()!=EOR DONE
				file[index]=0;
				IF modif=='<'
				THEN	IF Ipath THEN
						strcpy(Ifile, Ipath);
						strcat(Ifile, "/");
						strcat(Ifile, file);
					FI
					IF strcmp(file, "-") != 0
					THEN	iclose(stack, 0);
						infile=open(file,0);
						IF infile<0
						THEN	infile = open(Ifile, 0);
						FI
					ELSE	lseek(infile, 0L, 0);
					FI
					IF infile<0
					THEN    infile=0; error(NOTOPEN);
					ELSE	IF cntflg
						THEN var[9] = cntval;
						ELSE var[9] = 1;
						FI
					FI
				ELSE    oclose();
					outfile = open(file, O_CREAT|O_WRONLY, 0644);
					lseek(outfile,0L,2);
				FI

			ELSE	IF modif == '<'
				THEN	iclose(-1, 0);
				ELSE	oclose();
				FI
			FI
			lp--;
		}
		break;

	    case 'o':
		radix = 8; break;

	    case 'd':
		radix = 10; break;

	    case 'x':
		radix = 16; break;

	    case 'q': case 'Q': case '%':
		done();

	    case 'w': case 'W':
		maxpos=(adrflg?adrval:MAXPOS);
		break;

	    case 's': case 'S':
		maxoff=(adrflg?adrval:MAXOFF);
		break;

	    case 'v': 
		printf("variables\n");
		FOR i=0;i<=35;i++
		DO IF var[i]
		   THEN printc((i<=9 ? '0' : 'a'-10) + i);
			printf(" = %X\n",var[i]);
		   FI
		OD
		break;

	    case 'm': case 'M':
		printmap("? map",&txtmap);
		printmap("/ map",&datmap);
		break;

	    case 0: case '?':
		IF pid
		THEN printf("pcs id = %d\n",pid);
		ELSE printf("no process\n");
		FI
		sigprint(); flushbuf();

	    case 'r': case 'R':
		printregs();
		return;

	    case 'f': case 'F':
		printfregs(modif=='F');
		return;

	    case 'c': case 'C':
		firstframe(adrval, adrflg);
		WHILE cntval--
		DO      chkerr();
			printf("%4x: ", frame);
			printf("%s(", cache_sym(symbol));
			FOR i = 0; i < narg; i++
			DO	IF i THEN printc(','); FI
				printf("%x", get(ADDR(ssel, argp + 2 * i), DSP));
			OD
 			/* Add return-PC info.  Force printout of
 			 * symbol+offset (never just a number! ) by using
 			 * max possible offset.
 			 */
 			printf(") from ");
 			{
				int	savmaxoff = maxoff;

 				maxoff = ((unsigned)-1)>>1;
 				psymoff(retpc,ISYM,"");
 				maxoff = savmaxoff;
 			}
 			printc('\n');
			errflg = 0;
			IF !nextframe() THEN break; FI
		OD
		errflg = 0;
		break;

	    /*print externals*/
	    case 'e': case 'E':
		symset(-1);
		WHILE (symp=symget())
		DO chkerr();
		   IF (symp->type == (N_EXT|N_DATA)) || (symp->type == (N_EXT|N_BSS))
		   THEN printf("%s:%16t%x\n", no_cache_sym(symp),
				get(ADDR(segsel(symp->ovno), symp->value),DSP));
		   FI
		OD
		break;

	    /*set default c frame*/
	    /*print breakpoints*/
	    case 'b': case 'B':
		printf("breakpoints\ncount%8tbkpt%24tcommand\n");
		FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
		DO IF bkptr->flag
		   THEN printf("%-8.8d",bkptr->count);
			psymoff(bkptr->loc,ISYM,"%24t");
			comptr=bkptr->comm;
			WHILE *comptr DO printc(*comptr++); OD
		   FI
		OD
		break;

	    default: error(BADMOD);
	}

}

printmap(s,amap)
	char	*s;
	MAP	*amap;
{
	int file;
	register struct seg *sp;

	file=amap->ufd;
	printf("%s%12t`%s'\n",s,(file<0 ? "-" : (file==fcor ? corfil : symfil)));
	IF amap->nseg
	THEN	FOR sp = amap->seg; sp < &amap->seg[amap->nseg]; sp++
		DO	printf("%4x %s%8tb = %-8xe = %-8xf = %-16X",
			    sp->sel, sp->type == XS_CODE ? "code" : "data",
			    (u_int)sp->b, (u_int)sp->e, sp->f);
			IF sp->z > sp->e
			THEN	printf("z = %X", sp->z);
			FI
			printc(EOR);
		OD
		return;
	FI
	printf("b1 = %-16X",amap->b1);
	printf("e1 = %-16X",amap->e1);
	printf("f1 = %-16X",amap->f1);
	printf("\nb2 = %-16X",amap->b2);
	printf("e2 = %-16X",amap->e2);
	printf("f2 = %-16X",amap->f2);
	printc(EOR);
}

/*
 * A register of the 287's, as FSAVE leaves it: ten bytes, the
 * fraction's 64 bits from the lowest word, then the sign and the
 * exponent.  As a double, which is all of it but the last bits of the
 * fraction.
 */
double
extended(w)
	register u_short *w;
{
	union {
		double	d;
		u_short	s[4];
	} u;
	register int e;

	e = w[4] & 0x7fff;
	IF e == 0 ANDF (w[0] | w[1] | w[2] | w[3]) == 0
	THEN	u.s[3] = w[4] & 0x8000;
		u.s[0] = u.s[1] = u.s[2] = 0;
		return(u.d);
	FI
	IF e == 0x7fff
	THEN	e = 0x7ff;
	ELSE	e = e - 16383 + 1023;
		IF e >= 0x7ff THEN e = 0x7ff; FI
		IF e <= 0 THEN e = 0; FI
	FI
	u.s[3] = (w[4] & 0x8000) | (e << 4) | ((w[3] >> 11) & 0xf);
	u.s[2] = (w[3] << 5) | (w[2] >> 11);
	u.s[1] = (w[2] << 5) | (w[1] >> 11);
	u.s[0] = (w[1] << 5) | (w[0] >> 11);
	return(u.d);
}

/*
 * The 287: its control, status and tag words, and its stack, st0 at its
 * top.  FSAVE puts seven words and then the eight registers in the
 * order of the stack.
 */
printfregs(longpr)
	int	longpr;
	{
	register int i, tag;
	register u_short *st;
	int	top;

	st = (u_short *)((U*)corhdr)->u_fps.u_fpstate;
	printf("cw%8t%x\nsw%8t%x\ntw%8t%x\n", st[0], st[1], st[2]);
	top = (st[1] >> 11) & 07;
	for	(i = 0; i < 8; i++)
		{
		tag = (st[2] >> (2 * ((top + i) & 07))) & 03;
		printf("st%d%8t", i);
		if	(tag == 3)
			printf("empty\n");
		else
			printf("%-24.*f\n", longpr ? 18 : 9,
			    extended(&st[7 + 5 * i]));
		}
	}

printregs()
{
	register REGPTR      p;
	int	v;

	IF kernel THEN error(NOPCS); FI
	FOR p=reglist; p < &reglist[NREG]; p++
	DO      printf("%s%8t%x%8t", p->rname, v=uar0[p->roffs]);
		IF p->roffs == IP
		THEN	valpr(pcaddr(), ISYM);
		ELIF p->roffs != CS ANDF p->roffs != DS ANDF p->roffs != ES
		    ANDF p->roffs != SS ANDF p->roffs != FLAGS
		THEN	valpr(leng(v), DSYM);
		FI
		printc(EOR);
	OD
	printpc();
}

getreg(regnam)
{
	register REGPTR      p;
	register char	*regptr;
	char	regnxt;

	IF kernel THEN return(NOREG); FI        /* not supported */
	regnxt=readchar();
	FOR p=reglist; p<&reglist[NREG]; p++
	DO      regptr=p->rname;
		IF (regnam == *regptr++) ANDF (regnxt == *regptr)
		THEN    return(p->roffs);
		FI
	OD
	lp--;
	return(NOREG);
}

printpc()
{
	dot=pcaddr();
	psymoff(dot,ISYM,":%16t"); printins(ISP,chkget(dot,ISP));
	printc(EOR);
}

sigprint()
{
	extern char	*sys_siglist[];		/* signal list */

	if (signo >= 0 && signo < NSIG)
		printf("%s",sys_siglist[signo]);
	else
		printf("unknown signal %d",signo);
}
