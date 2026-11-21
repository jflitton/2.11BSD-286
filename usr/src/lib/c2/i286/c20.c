/*
 *	 C object code improver, for the 80286
 *
 *	This is ../c20.c with the 286's instructions for the PDP-11's,
 *	reading and writing what the compiler's second pass writes
 *	for NASM.  What it does to jumps is as it was.  A routine ends
 *	at the ret after its leave, and not where the next is made
 *	global.
 */

#include "c2.h"

struct optab optab[] = {
	"jmp",	JBR,
	"je",	CBR | JEQ<<8,
	"jne",	CBR | JNE<<8,
	"jle",	CBR | JLE<<8,
	"jge",	CBR | JGE<<8,
	"jl",	CBR | JLT<<8,
	"jg",	CBR | JGT<<8,
	"jb",	CBR | JLO<<8,
	"ja",	CBR | JHI<<8,
	"jbe",	CBR | JLOS<<8,
	"jae",	CBR | JHIS<<8,
	"jns",	CBR | JPL<<8,
	"js",	CBR | JMI<<8,
	"ret",	EROU,
	"retf",	EROU | 1<<8,
	"dw",	JSW,
	"mov",	MOV,
	"lea",	LEA,
	"push",	PUSH,
	"pop",	POP,
	"cmp",	CMP,
	"test",	TEST,
	"add",	ADD,
	"sub",	SUB,
	"and",	AND,
	"or",	OR,
	"xor",	XOR,
	"adc",	ADC,
	"sbb",	SBB,
	"inc",	INC,
	"dec",	DEC,
	"neg",	NEG,
	"not",	NOT,
	"shl",	SHIFT,
	"shr",	SHIFT | 1<<8,
	"sar",	SHIFT | 2<<8,
	"rcl",	SHIFT | 3<<8,
	"rcr",	SHIFT | 4<<8,
	"xchg",	XCHG,
	"cbw",	CBW,
	"cwd",	CWD,
	"imul",	MUL,
	"mul",	MUL | 1<<8,
	"idiv",	MUL | 2<<8,
	"div",	MUL | 3<<8,
	"call",	JSR,
	"leave",LEAVE,
	"les",	LES,
	0,	0};

char revbr[] = {JNE, JEQ, JGT, JLT, JGE, JLE, JHIS, JLOS, JHI, JLO, JMI, JPL};
int	isn	= 1;

/*
 * The compiler goes from the code to the data and back as it comes to
 * them, and has NASM told so.  Here a routine is held until all of it
 * is read, and the data goes straight out: so what is written says
 * for itself which segment it is for.
 */
char	textseg[LSIZE] = "_TEXT";
int	indata;			/* what is being read is data */
int	intext;			/* what was last written was code */
char	pendb[LSIZE];		/* what came after a label on its line */
int	pending;
int	cont;			/* in the middle of a long line */

/*
 * Space for a routine is taken a block at a time, and all of it is
 * had back when the routine is done.
 */
#define	ABLK	2040
struct blk {
	struct	blk *bnext;
	char	bspace[ABLK];
};
struct	blk *blks;
struct	blk *curblk;
int	bused;

main(argc, argv)
char **argv;
{
	register int niter, maxiter, isend;
	int nflag;

	if (argc>1 && argv[1][0]=='+') {
		argc--;
		argv++;
		debug++;
	}
	nflag = 0;
	if (argc>1 && argv[1][0]=='-' && argv[1][1]=='n') {
		argc--;
		argv++;
		nflag++;
	}
	if (argc>1) {
		if (freopen(argv[1], "r", stdin) == NULL) {
			fprintf(stderr, "C2: can't find %s\n", argv[1]);
			exit(1);
		}
	}
	if (argc>2) {
		if (freopen(argv[2], "w", stdout) == NULL) {
			fprintf(stderr, "C2: can't create %s\n", argv[2]);
			exit(1);
		}
	}
	maxiter = 0;
	opsetup();
	do {
		isend = input();
		niter = 0;
		do {
			refcount();
			do {
				iterate();
				clearreg();
				niter++;
			} while (nchange);
			comjump();
			bools();
			rmove();
		} while ((nchange || jumpsw()) && niter < 1000);
		output();
		if (niter > maxiter)
			maxiter = niter;
		curblk = blks;
		bused = 0;
	} while (isend);
	if (nflag) {
		fprintf(stderr, "%d iterations\t\t\t", maxiter);
		fprintf(stderr, "%d jumps to jumps\n", nbrbr);
		fprintf(stderr, "%d inst. after jumps\t\t", iaftbr);
		fprintf(stderr, "%d jumps to the next\n", njp1);
		fprintf(stderr, "%d redundant labels\t\t", nrlab);
		fprintf(stderr, "%d cross-jumps\n", nxjump);
		fprintf(stderr, "%d code motions\t\t\t", ncmot);
		fprintf(stderr, "%d branches reversed\n", nrevbr);
		fprintf(stderr, "%d redundant moves\t\t", redunm);
		fprintf(stderr, "%d simplified addresses\n", nsaddr);
		fprintf(stderr, "%d loops inverted\t\t", loopiv);
		fprintf(stderr, "%d moves into what uses them\n", nfwd);
		fprintf(stderr, "%d common seqs before jmp's\t", ncomj);
		fprintf(stderr, "%d skips over jumps\n", nskip);
		fprintf(stderr, "%d moves of what is not used\t", ndead);
		fprintf(stderr, "%d conditions' values\n", nbool);
	}
	fflush(stdout);
	if (ferror(stdout)) {
		fprintf(stderr, "C2: can't write\n");
		exit(1);
	}
	exit(0);
}

input()
{
	register struct node *p, *lastp;
	register int oper;

	lastp = &first;
	first.forw = 0;
	for (;;) {
		oper = getline();
		if (oper==0 && lastp==&first) {
			/*
			 * What is before the routine, and is not the
			 * routine's, is not kept for it.
			 */
			printf("\t%s\n", line);
			continue;
		}
		p = (struct node *)alloc(sizeof first);
		p->op = oper&0377;
		p->subop = oper>>8;
		p->labno = 0;
		p->code = 0;
		switch (oper&0377) {

		case LABEL:
			if (line[0]=='L' && (p->labno = getnum(line+1)))
				break;
			p->op = DLABEL;
			p->code = copy(line, "");
			break;

		case JBR:
		case CBR:
		case JSW:
			if (*curlp=='L' && (p->labno = getnum(curlp+1)))
				break;
			if (p->op==JBR) {
				p->op = JMP;
				p->code = copy(curlp, "");
			} else {
				p->op = 0;
				p->subop = 0;
				p->code = copy(line, "");
			}
			break;

		case EROU:
			/*
			 * The routine's last is after its leave.  Any
			 * other is a call of what has been pushed, and
			 * comes back to the label pushed before that.
			 */
			if (lastp->op!=LEAVE) {
				p->op = 0;
				p->subop = 0;
				p->code = copy(line, "");
			}
			break;

		default:
			p->code = copy(curlp, "");
			break;

		}
		p->forw = 0;
		p->back = lastp;
		lastp->forw = p;
		lastp = p;
		p->ref = 0;
		p->refc = 0;
		p->mark = 0;
		if (p->op==EROU)
			return(1);
		if (p->op==END)
			return(0);
	}
}

/*
 * The next line of the code: what it is, with the label in line, or
 * the instruction there and curlp at its operands.  Comments and
 * empty lines are left out, and data is written as it is read.
 */
getline()
{
	register char *lp, *p;
	register int nl;

	for (;;) {
		if (pending) {
			pending = 0;
			line[0] = '\t';
			strcpy(line+1, pendb);
		} else if (fgets(line, LSIZE, stdin) == NULL) {
			line[0] = 0;
			curlp = line;
			return(END);
		}
		for (lp = line; *lp; lp++)
			;
		nl = 0;
		if (lp > line && lp[-1]=='\n') {
			*--lp = 0;
			nl++;
		}
		if (cont || (nl==0 && lp >= &line[LSIZE-1])) {
			/*
			 * Longer than there is room for, which data
			 * may be.
			 */
			if (indata==0) {
				fprintf(stderr, "C2: Sorry, input line too long\n");
				exit(1);
			}
			fputs(line, stdout);
			if (nl)
				putchar('\n');
			cont = !nl;
			continue;
		}
		while (lp > line && (lp[-1]==' ' || lp[-1]=='\t'))
			*--lp = 0;
		for (p = line; *p==' ' || *p=='\t'; p++)
			;
		if (*p==0 || *p==';')
			continue;
		if (p!=line && segment(p))
			continue;
		if (indata) {
			printf("%s\n", line);
			continue;
		}
		if (p==line) {
			if ((lp = index(line, ':')) == 0) {
				curlp = line;
				return(0);
			}
			*lp++ = 0;
			while (*lp==' ' || *lp=='\t')
				lp++;
			if (*lp) {
				strcpy(pendb, lp);
				pending++;
			}
			return(LABEL);
		}
		for (lp = line; *lp++ = *p++;)
			;
		return(oplook());
	}
}

/*
 * A line that begins or goes back to a segment.  The code's is
 * remembered, and written when there is code to write.
 */
segment(p)
register char *p;
{
	register char *np;

	for (np = "segment"; *np; )
		if (*p++ != *np++)
			return(0);
	if (*p!=' ' && *p!='\t')
		return(0);
	while (*p==' ' || *p=='\t')
		p++;
	for (np = p; *np && *np!=' ' && *np!='\t'; np++)
		;
	if (*np) {
		/*
		 * With what the segment is: its definition.
		 */
		printf("%s\n", line);
		intext = 0;
		for (np = line; np = index(np, '='); np++)
			if (np[1]=='C' && np[2]=='O' && np[3]=='D' && np[4]=='E') {
				for (np = textseg; *p!=' ' && *p!='\t';)
					*np++ = *p++;
				*np = 0;
				break;
			}
		return(1);
	}
	if (p[0]=='_' && (equstr(p, "_DATA") || equstr(p, "_BSS"))) {
		indata = 1;
		intext = 0;
		printf("%s\n", line);
		return(1);
	}
	indata = 0;
	strcpy(textseg, p);
	return(1);
}

getnum(ap)
char *ap;
{
	register char *p;
	register n, c;

	p = ap;
	n = 0;
	while ((c = *p++) >= '0' && c <= '9')
		n = n*10 + c - '0';
	if (*--p != 0)
		return(0);
	return(n);
}

/*
 * A label: the compiler's are L and a number, and those made here,
 * whose numbers are kept below 0, are LL and the number.
 */
plab(n)
{
	if (n < 0)
		printf("LL%d", -n);
	else
		printf("L%d", n);
}

output()
{
	register struct node *t;
	register struct optab *oper;

	t = &first;
	if (t->forw && t->forw->op!=END && intext==0) {
		printf("\tsegment\t%s\n", textseg);
		intext = 1;
	}
	while (t = t->forw) switch (t->op) {

	case END:
		return;

	case LABEL:
		plab(t->labno);
		printf(":\n");
		continue;

	case DLABEL:
		printf("%s:\n", t->code);
		continue;

	case JMP:
		printf("\tjmp\t%s\n", t->code);
		continue;

	case 0:
		printf("\t%s\n", t->code);
		continue;

	default:
		for (oper = optab; oper->opstring!=0; oper++)
			if ((oper->opcode&0377) == t->op
			 && (oper->opcode>>8) == t->subop) {
				printf("\t%s", oper->opstring);
				break;
			}
		if (t->code)
			printf("\t%s\n", t->code);
		else if (t->op==JBR || t->op==CBR || t->op==JSW) {
			putchar('\t');
			plab(t->labno);
			putchar('\n');
		} else
			printf("\n");
		continue;
	}
}

/*
 * A copy of the one string with the other after it.
 */
char *
copy(a, b)
char *a, *b;
{
	register char *p, *np;
	char *onp;
	register n;

	n = 1;
	for (p = a; *p++;)
		n++;
	for (p = b; *p++;)
		n++;
	if (n==1)
		return(0);
	onp = np = alloc(n);
	for (p = a; *np++ = *p++;)
		;
	np--;
	for (p = b; *np++ = *p++;)
		;
	return(onp);
}

opsetup()
{
	register struct optab *optp, **ophp;
	register char *p;

	for (optp = optab; p = optp->opstring; optp++) {
		ophp = &ophash[(((p[0]<<3)+(p[1]<<1)+p[2])&077777) % OPHS];
		while (*ophp++)
			if (ophp >= &ophash[OPHS])
				ophp = ophash;
		*--ophp = optp;
	}
}

oplook()
{
	register struct optab *optp;
	register char *lp, *np;
	static char tmpop[32];
	struct optab **ophp;

	np = tmpop;
	for (lp = line; *lp && *lp!=' ' && *lp!='\t';) {
		if (np >= &tmpop[30]) {
			curlp = line;
			return(0);
		}
		*np++ = *lp++;
	}
	*np++ = 0;
	*np = 0;
	while (*lp=='\t' || *lp==' ')
		lp++;
	curlp = lp;
	ophp = &ophash[(((tmpop[0]<<3)+(tmpop[1]<<1)+tmpop[2])&077777) % OPHS];
	while (optp = *ophp) {
		np = optp->opstring;
		lp = tmpop;
		while (*lp == *np++)
			if (*lp++ == 0)
				return(optp->opcode);
		ophp++;
		if (ophp >= &ophash[OPHS])
			ophp = ophash;
	}
	curlp = line;
	return(0);
}

/*
 * The number of the label that is named at p, if one is: L and
 * digits, and no more of a name after them.
 */
lnum(p)
register char *p;
{
	register n;

	if (*p++ != 'L' || *p<'0' || *p>'9')
		return(0);
	n = 0;
	while (*p>='0' && *p<='9')
		n = n*10 + *p++ - '0';
	if (idchar(*p))
		return(0);
	return(n);
}

refcount()
{
	register struct node *p, *lp;
	static struct node *labhash[LABHS];
	register struct node **hp, *tp;
	register char *cp;
	int n;

	for (hp = labhash; hp < &labhash[LABHS];)
		*hp++ = 0;
	for (p = first.forw; p!=0; p = p->forw)
		if (p->op==LABEL) {
			labhash[(unsigned)p->labno % LABHS] = p;
			p->refc = 0;
		}
	for (p = first.forw; p!=0; p = p->forw) {
		if (p->op==JBR || p->op==CBR || p->op==JSW) {
			p->ref = 0;
			lp = labhash[(unsigned)p->labno % LABHS];
			if (lp==0 || p->labno!=lp->labno)
			for (lp = first.forw; lp!=0; lp = lp->forw) {
				if (lp->op==LABEL && p->labno==lp->labno)
					break;
			}
			if (lp) {
				tp = nonlab(lp)->back;
				if (tp!=lp) {
					p->labno = tp->labno;
					lp = tp;
				}
				p->ref = lp;
				lp->refc++;
			}
		} else if (p->code && p->op!=DLABEL && p->op!=LABEL) {
			/*
			 * A label that an operand names, as the jump
			 * through a switch's table does, is not one
			 * to move or to take away.
			 */
			for (cp = p->code; cp = index(cp, 'L'); cp++) {
				if (cp!=p->code && idchar(cp[-1]))
					continue;
				if ((n = lnum(cp)) == 0)
					continue;
				lp = labhash[(unsigned)n % LABHS];
				if (lp==0 || n!=lp->labno)
				for (lp = first.forw; lp!=0; lp = lp->forw) {
					if (lp->op==LABEL && n==lp->labno)
						break;
				}
				if (lp)
					lp->refc++;
			}
		}
	}
	for (p = first.forw; p!=0; p = p->forw)
		if (p->op==LABEL && p->refc==0
		 && (lp = nonlab(p))->op && lp->op!=JSW)
			decref(p);
}

iterate()
{
	register struct node *p, *rp, *p1;

	nchange = 0;
	for (p = first.forw; p!=0; p = p->forw) {
		CHECK(0);
		if ((p->op==JBR||p->op==CBR||p->op==JSW) && p->ref) {
			rp = nonlab(p->ref);
			if (rp->op==JBR && rp->labno && p->labno!=rp->labno) {
				nbrbr++;
				p->labno = rp->labno;
				decref(p->ref);
				rp->ref->refc++;
				p->ref = rp->ref;
				CHECK(1);
				nchange++;
			}
		}
		if (p->op==CBR && (p1 = p->forw)->op==JBR && p->ref) {
			rp = p->ref;
			do
				rp = rp->back;
			while (rp->op==LABEL);
			if (rp==p1) {
				decref(p->ref);
				p->ref = p1->ref;
				p->labno = p1->labno;
				p1->forw->back = p;
				p->forw = p1->forw;
				p->subop = revbr[p->subop];
				nchange++;
				CHECK(2);
				nskip++;
			}
		}
		if (p->op==JBR || p->op==JMP) {
			while (p->forw && p->forw->op!=LABEL
				&& p->forw->op!=DLABEL
				&& p->forw->op!=EROU && p->forw->op!=END
				&& p->forw->op!=0 && p->forw->op!=JSW) {
				nchange++;
				iaftbr++;
				if (p->forw->ref)
					decref(p->forw->ref);
				p->forw = p->forw->forw;
				p->forw->back = p;
				CHECK(3);
			}
			rp = p->forw;
			while (rp && rp->op==LABEL) {
				if (p->ref == rp) {
					p->back->forw = p->forw;
					p->forw->back = p->back;
					p = p->back;
					decref(rp);
					nchange++;
					CHECK(4);
					njp1++;
					break;
				}
				rp = rp->forw;
			}
		}
		if (p->op==JBR || p->op==JMP) {
			xjump(p);
			p = codemove(p);
		}
	}
}

xjump(p1)
register struct node *p1;
{
	register struct node *p2, *p3;

	if ((p2 = p1->ref)==0)
		return;
	for (;;) {
		while ((p1 = p1->back) && p1->op==LABEL);
		while ((p2 = p2->back) && p2->op==LABEL);
		if (!equop(p1, p2) || p1==p2)
			return;
		p3 = insertl(p2);
		p1->op = JBR;
		p1->subop = 0;
		p1->ref = p3;
		p1->labno = p3->labno;
		p1->code = 0;
		nxjump++;
		CHECK(5);
		nchange++;
	}
}

struct node *
insertl(oldp)
register struct node *oldp;
{
	register struct node *lp;

	if (oldp->op == LABEL) {
		oldp->refc++;
		return(oldp);
	}
	if (oldp->back->op == LABEL) {
		oldp = oldp->back;
		oldp->refc++;
		return(oldp);
	}
	lp = (struct node *)alloc(sizeof first);
	lp->op = LABEL;
	lp->subop = 0;
	lp->labno = -isn++;
	lp->ref = 0;
	lp->code = 0;
	lp->refc = 1;
	lp->mark = 0;
	lp->back = oldp->back;
	lp->forw = oldp;
	oldp->back->forw = lp;
	oldp->back = lp;
	CHECK(6);
	return(lp);
}

struct node *
codemove(p)
struct node *p;
{
	register struct node *p1, *p2, *p3;
	struct node *t, *tl;
	int n;

	p1 = p;
	if (p1->op!=JBR || (p2 = p1->ref)==0)
		return(p1);
	while (p2->op == LABEL)
		if ((p2 = p2->back) == 0)
			return(p1);
	if (p2->op!=JBR && p2->op!=JMP)
		goto ivloop;
	if (p1==p2)
		return(p1);
	p2 = p2->forw;
	p3 = p1->ref;
	while (p3) {
		if (p3->op==JBR || p3->op==JMP) {
			if (p1==p3 || p1->forw==p3 || p1->back==p3)
				return(p1);
			ncmot++;
			nchange++;
			CHECK(70);
			p1->back->forw = p2;
			p1->forw->back = p3;
			p2->back->forw = p3->forw;
			p3->forw->back = p2->back;
			p2->back = p1->back;
			p3->forw = p1->forw;
			decref(p1->ref);
			CHECK(7);
			return(p2);
		} else
			p3 = p3->forw;
	}
	return(p1);
ivloop:
	if (p1->forw->op!=LABEL)
		return(p1);
	p3 = p2 = p2->forw;
	n = 16;
	do {
		if ((p3 = p3->forw) == 0 || p3==p1 || --n==0)
			return(p1);
	} while (p3->op!=CBR || p3->labno!=p1->forw->labno);
	do
		if ((p1 = p1->back) == 0)
			return(p);
	while (p1!=p3);
	p1 = p;
	tl = insertl(p1);
	p3->subop = revbr[p3->subop];
	decref(p3->ref);
	p2->back->forw = p1;
	p3->forw->back = p1;
	p1->back->forw = p2;
	p1->forw->back = p3;
	t = p1->back;
	p1->back = p2->back;
	p2->back = t;
	t = p1->forw;
	p1->forw = p3->forw;
	p3->forw = t;
	p2 = insertl(p1->forw);
	p3->labno = p2->labno;
	p3->ref = p2;
	decref(tl);
	if (tl->refc<=0)
		nrlab--;
	loopiv++;
	nchange++;
	CHECK(8);
	return(p3);
}

comjump()
{
	register struct node *p1, *p2, *p3;

	for (p1 = first.forw; p1!=0; p1 = p1->forw)
		if (p1->op==JBR && (p2 = p1->ref) && p2->refc > 1)
			for (p3 = p1->forw; p3!=0; p3 = p3->forw)
				if (p3->op==JBR && p3->ref == p2)
					backjmp(p1, p3);
}

backjmp(ap1, ap2)
struct node *ap1, *ap2;
{
	register struct node *p1, *p2, *p3;

	p1 = ap1;
	p2 = ap2;
	for(;;) {
		while ((p1 = p1->back) && p1->op==LABEL);
		p2 = p2->back;
		if (equop(p1, p2)) {
			p3 = insertl(p1);
			p2->back->forw = p2->forw;
			p2->forw->back = p2->back;
			p2 = p2->forw;
			decref(p2->ref);
			p2->labno = p3->labno;
			p2->ref = p3;
			nchange++;
			ncomj++;
			CHECK(9);
		} else
			return;
	}
}

char *
alloc(n)
register n;
{
	register char *p;
	register struct blk *bp;
	extern char *malloc();

#define round(a,b) ((((a)+(b)-1)/(b))*(b))
	n=round(n,sizeof(char *));
	if (n > ABLK) {
		fprintf(stderr, "C2: Sorry, input line too long\n");
		exit(1);
	}
	if (curblk==0 || bused+n > ABLK) {
		if (curblk && curblk->bnext)
			bp = curblk->bnext;
		else {
			bp = (struct blk *)malloc(sizeof(struct blk));
			if (bp==0) {
				fprintf(stderr, "C Optimizer: out of space\n");
				exit(1);
			}
			bp->bnext = 0;
			if (curblk)
				curblk->bnext = bp;
			else
				blks = bp;
		}
		curblk = bp;
		bused = 0;
	}
	p = &curblk->bspace[bused];
	bused += n;
	return(p);
}
