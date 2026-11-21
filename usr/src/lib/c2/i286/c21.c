/*
 * C object code improver, for the 80286-- second part
 *
 * What ../c21.c does for the PDP-11, done again for this machine:
 * the moves that fetch what a register has already, and the operands
 * that a register stands in for.  Two things are new, which that
 * compiler's tables had no need of and this one's generator has: a
 * register is seen to be dead, so that what was moved into it only to
 * be used once is used where it was, and a move whose result nothing
 * reads is taken out.
 *
 * mov sets no flags here, so one can go from before a jump as the
 * PDP-11's could not.
 */

#include "c2.h"

char	*rname[] = {"ax", "bx", "cx", "dx", "si", "di"};
char	*lname[] = {"al", "bl", "cl", "dl"};
char	*hname[] = {"ah", "bh", "ch", "dh"};

/*
 * The routines that take what they work on in registers
 * (lib/libc/i286/crt).  Any other is C's, whose arguments are all on
 * the stack, but for where a structure is to be returned, which is in
 * SI; and which may leave anything in AX, BX, CX and DX.
 */
char	*helper[] = {"__I4M", "__U4M", "__I4D", "__U4D", "__CHP", 0};

int	gen;
int	budget;

/*
 * What the registers had at each jump to a label that has been come
 * to, less what they did not all have: kept by the label, in its code.
 * If every jump to the label is before it, that is what they have at
 * it.  Each register's is a count and then that many characters.
 */
struct snap {
	int	sgen;		/* the pass that it is of */
	int	scnt;		/* the jumps that there have been */
	int	ssize;
	char	sregs[2];
};
int	rgen;

#define	delete(p)	(p->back->forw = p->forw, p->forw->back = p->back)

rmove()
{
	register struct node *p;
	register int r, d, s;
	int ch;
	char lbuf[MAXCPS + 2];

	clearreg();
	rgen++;
	for (p=first.forw; p!=0; p = p->forw) {
	switch (p->op) {

	case LABEL:
		merge(p);
		continue;

	case JBR:
		if (p->ref)
			snap(p->ref);
		clearreg();
		continue;

	case CBR:
		if (p->ref)
			snap(p->ref);
		continue;

	case MOV:
		dualop(p);
		d = isreg(rt1);
		s = isreg(rt2);
		if (equstr(rt1, "es")) {
			/*
			 * ES is given a selector, which it may have
			 * already: loading it takes the 286 17 clocks in
			 * protected mode.
			 */
			lbuf[0] = 0;
			if (s >= 0)
				strcpy(lbuf, regs[s]);
			else if (equstr(rt2, "ss") || equstr(rt2, "ds")
			 || wordmem(rt2) && !segpre(rt2) && strlen(rt2) <= MAXCPS)
				strcpy(lbuf, rt2);
			if (lbuf[0] && equstr(regs[ES], lbuf)) {
				delete(p);
				redunm++;
				nchange++;
				continue;
			}
			dest(rt1);
			strcpy(regs[ES], lbuf);
			continue;
		}
		if (d >= 0) {
			/*
			 * A register is loaded.
			 */
			if (d==s || equstr(regs[d], rt2) && rt2[0]
			 || s>=0 && equstr(regs[s], rt1)
			 || s>=0 && regs[d][0] && equstr(regs[d], regs[s])) {
				delete(p);
				redunm++;
				nchange++;
				continue;
			}
			if (dead(d, p)) {
				delete(p);
				ndead++;
				nchange++;
				continue;
			}
			if (fwdsub(p, d))
				continue;
			if (s<0 && (wordmem(rt2) || !ismem(rt2))
			 && (r = findrand(rt2)) >= 0) {
				/* another has it, and is shorter */
				s = r;
				p->code = copy(rt1, ",");
				p->code = copy(p->code, rname[s]);
				nsaddr++;
				nchange++;
			}
			dest(rt1);
			if (s >= 0)
				savereg(d, regs[s][0]? regs[s]: rname[s]);
			else
				savereg(d, rt2);
			continue;
		}
		if ((d = lowreg(rt1)) >= 0 && regs[d][0]=='w'
		 && rt2[0]=='b' && ismem(rt2) && equstr(regs[d]+4, rt2+4)) {
			/*
			 * The low byte of a word that the register has.
			 */
			delete(p);
			redunm++;
			nchange++;
			continue;
		}
		if (wordmem(rt1)) {
			if (s >= 0) {
				if (equstr(regs[s], rt1)) {
					/* it is there already */
					delete(p);
					redunm++;
					nchange++;
					continue;
				}
				dest(rt1);
				if (regs[s][0]==0 || ismem(regs[s]))
					savereg(s, rt1);
				continue;
			}
			if (!ismem(rt2) && (r = findrand(rt2)) >= 0) {
				p->code = copy(rt1, ",");
				p->code = copy(p->code, rname[r]);
				nsaddr++;
				nchange++;
			}
		}
		dest(rt1);
		continue;

	case LEA:
		dualop(p);
		if ((d = isreg(rt1)) < 0 || strlen(rt2) >= MAXCPS) {
			clearreg();
			continue;
		}
		lbuf[0] = '&';
		strcpy(lbuf+1, rt2);
		if (equstr(regs[d], lbuf)) {
			delete(p);
			redunm++;
			nchange++;
			continue;
		}
		if (dead(d, p)) {
			delete(p);
			ndead++;
			nchange++;
			continue;
		}
		if ((r = findrand(lbuf)) >= 0) {
			p->op = MOV;
			p->code = copy(rt1, ",");
			p->code = copy(p->code, rname[r]);
			nsaddr++;
			nchange++;
		}
		dest(rt1);
		savereg(d, lbuf);
		continue;

	case LES:
		/*
		 * ES and BX from a far pointer, which they may have
		 * already.  BX then has the word where its offset is.
		 */
		dualop(p);
		d = isreg(rt1);
		if (d==BX && equstr(regs[ESBX], rt2) && rt2[0]) {
			delete(p);
			redunm++;
			nchange++;
			continue;
		}
		dest(rt1);
		dest("es");
		if (d==BX && strlen(rt2) <= MAXCPS && rt2[0]=='f'
		 && rt2[1]=='a' && rt2[2]=='r' && rt2[3]==' '
		 && !mention(rt2, BX) && !segpre(rt2)) {
			strcpy(regs[ESBX], rt2);
			strcpy(lbuf, "word");
			strcat(lbuf, rt2+3);
			savereg(BX, lbuf);
		}
		continue;

	case PUSH:
		singop(p);
		if ((wordmem(rt1) || !ismem(rt1)) && isreg(rt1) < 0
		 && (r = findrand(rt1)) >= 0) {
			p->code = copy(rname[r], "");
			nsaddr++;
			nchange++;
		}
		continue;

	case POP:
		singop(p);
		if (p->back->op==JSR && equstr(rt1, "cx") && spdead(p)
		 && dead(CX, p)) {
			/* an argument taken off, before a leave */
			delete(p);
			ndead++;
			nchange++;
			continue;
		}
		dest(rt1);
		dest("sp");
		continue;

	case CMP:
	case TEST:
		dualop(p);
		if (isreg(rt1) < 0 && !wordmem(rt1))
			continue;
		ch = 0;
		if (isreg(rt2) < 0 && (wordmem(rt2) || !ismem(rt2))
		 && (r = findrand(rt2)) >= 0) {
			strcpy(rt2, rname[r]);
			ch++;
		}
		if (wordmem(rt1) && (r = findrand(rt1)) >= 0) {
			strcpy(rt1, rname[r]);
			ch++;
		}
		if (ch) {
			p->code = copy(rt1, ",");
			p->code = copy(p->code, rt2);
			nsaddr++;
			nchange++;
		}
		continue;

	case XOR:
		dualop(p);
		if ((d = isreg(rt1)) >= 0 && equstr(rt1, rt2)) {
			/*
			 * The register is cleared.
			 */
			if (equstr(regs[d], "0") && flagsdead(p)) {
				delete(p);
				redunm++;
				nchange++;
				continue;
			}
			if (dead(d, p) && flagsdead(p)) {
				delete(p);
				ndead++;
				nchange++;
				continue;
			}
			dest(rt1);
			savereg(d, "0");
			continue;
		}

	case ADD:
	case SUB:
	case AND:
	case OR:
	case ADC:
	case SBB:
		dualop(p);
		if (p->op==ADD && equstr(rt1, "sp") && spdead(p)
		 && flagsdead(p)) {
			delete(p);
			ndead++;
			nchange++;
			continue;
		}
		if ((isreg(rt1) >= 0 || wordmem(rt1)) && isreg(rt2) < 0
		 && (wordmem(rt2) || !ismem(rt2))
		 && (r = findrand(rt2)) >= 0) {
			p->code = copy(rt1, ",");
			p->code = copy(p->code, rname[r]);
			nsaddr++;
			nchange++;
		}
		dest(rt1);
		continue;

	case INC:
	case DEC:
	case NEG:
	case NOT:
	case SHIFT:
		dualop(p);
		dest(rt1);
		continue;

	case XCHG:
		dualop(p);
		dest(rt1);
		dest(rt2);
		continue;

	case CBW:
		/*
		 * It does nothing to a small number; and nothing that is
		 * read, if what it puts in AH is not.
		 */
		if (small(regs[AX])) {
			delete(p);
			redunm++;
			nchange++;
			continue;
		}
		if (ahdead(p)) {
			delete(p);
			ndead++;
			nchange++;
			continue;
		}
		dest("ax");
		continue;

	case CWD:
		if (dead(DX, p)) {
			delete(p);
			ndead++;
			nchange++;
			continue;
		}
		dest("dx");
		continue;

	case MUL:
		dualop(p);
		if (rt2[0])
			dest(rt1);
		else {
			dest("ax");
			dest("dx");
		}
		continue;

	default:
		clearreg();
	}
	}
}

/*
 * A jump to the label lp, with the registers as they are.
 */
snap(lp)
register struct node *lp;
{
	register struct snap *sp;
	register char *p, *q;
	register int i;
	int n;

	sp = (struct snap *)lp->code;
	if (sp && sp->sgen==rgen) {
		sp->scnt++;
		for (p = sp->sregs, i = 0; i<NTRACK; i++) {
			n = *p++;
			if (!equstr(p, regs[i]))
				*p = 0;
			p += n;
		}
		return;
	}
	n = 0;
	for (i = 0; i<NTRACK; i++)
		n += strlen(regs[i]) + 2;
	if (sp==0 || sp->ssize < n) {
		sp = (struct snap *)alloc(sizeof(struct snap) + n);
		sp->ssize = n;
		lp->code = (char *)sp;
	}
	sp->sgen = rgen;
	sp->scnt = 1;
	for (p = sp->sregs, i = 0; i<NTRACK; i++) {
		q = regs[i];
		*p++ = strlen(q) + 1;
		while (*p++ = *q++)
			;
	}
}

/*
 * The label lp is come to.  What the registers have is what they had
 * at every jump to it, and have now if it is come to from above as
 * well; or nothing, if a jump to it is still to come.
 */
merge(lp)
register struct node *lp;
{
	register struct snap *sp;
	register char *p;
	register int i;
	int n, fall;

	sp = (struct snap *)lp->code;
	if (sp==0 || sp->sgen!=rgen || sp->scnt!=lp->refc) {
		clearreg();
		return;
	}
	fall = 1;
	switch (lp->back->op) {

	case JBR:
	case JMP:
	case JSW:
	case EROU:
		fall = 0;
	}
	for (p = sp->sregs, i = 0; i<NTRACK; i++) {
		n = *p++;
		if (fall==0)
			strcpy(regs[i], p);
		else if (!equstr(regs[i], p))
			regs[i][0] = 0;
		p += n;
	}
}

/*
 * Is the stack pointer given what it is to have before anything goes
 * by it?  A leave does that.
 */
spdead(p)
register struct node *p;
{
	register int n;

	for (n = 12; --n >= 0 && (p = p->forw);) switch (p->op) {

	case LABEL:
	case CBW:
	case CWD:
		continue;

	case JBR:
		if ((p = p->ref) == 0)
			return(0);
		continue;

	case LEAVE:
		return(1);

	case MOV:
	case LEA:
	case LES:
	case CMP:
	case TEST:
	case ADD:
	case SUB:
	case AND:
	case OR:
	case XOR:
	case ADC:
	case SBB:
	case INC:
	case DEC:
	case NEG:
	case NOT:
	case SHIFT:
	case MUL:
		if (word(p->code, "sp"))
			return(0);
		continue;

	default:
		return(0);
	}
	return(0);
}

/*
 * The value of a condition is left in AX so:
 *
 *		jcc	L1
 *		xor	ax,ax
 *		jmp	L2
 *	L1:	mov	ax,1
 *	L2:
 *
 * A mov leaves the flags as they are, so the one value can go in
 * before the jump, which is then round the other:
 *
 *		mov	ax,1
 *		jcc	L2
 *		dec	ax
 *	L2:
 */
bools()
{
	register struct node *p, *p1, *p2;
	struct node *p3, *p4, *p5;
	int n1, n2;
	char nbuf[16];

	for (p = first.forw; p!=0; p = p->forw) {
		if (p->op!=CBR || p->ref==0)
			continue;
		p1 = p->forw;
		if ((p2 = p1->forw)==0 || p2->op!=JBR || p2->ref==0)
			continue;
		if ((p3 = p2->forw)!=p->ref || p3->refc!=1)
			continue;
		p4 = p3->forw;
		if ((p5 = p4->forw)!=p2->ref)
			continue;
		if (!axcon(p1, &n1) || !axcon(p4, &n2))
			continue;
		p3->forw = p5;
		p5->back = p3;
		p4->back = p->back;
		p4->forw = p;
		p->back->forw = p4;
		p->back = p4;
		p4->op = MOV;
		sprintf(nbuf, "ax,%d", n2);
		p4->code = copy(nbuf, "");
		p->ref = p5;
		p->labno = p5->labno;
		p1->forw = p5;
		p5->back = p1;
		if (n1==n2-1 || n1==n2+1) {
			p1->op = n1<n2? DEC: INC;
			p1->code = copy("ax", "");
		}
		nbool++;
		nchange++;
	}
}

/*
 * Does p give AX a number, and what number?
 */
axcon(p, np)
register struct node *p;
int *np;
{
	register char *s;
	register n;
	int neg;

	if ((s = p->code)==0)
		return(0);
	if (p->op==XOR) {
		*np = 0;
		return(equstr(s, "ax,ax"));
	}
	if (p->op!=MOV || s[0]!='a' || s[1]!='x' || s[2]!=',')
		return(0);
	s += 3;
	neg = 0;
	if (*s=='-') {
		neg++;
		s++;
	}
	if (*s<'0' || *s>'9')
		return(0);
	n = 0;
	while (*s>='0' && *s<='9')
		n = n*10 + *s++ - '0';
	if (*s)
		return(0);
	*np = neg? -n: n;
	return(1);
}

/*
 * A number that is the same as a byte and as a word.
 */
small(s)
register char *s;
{
	register n;

	if (*s==0)
		return(0);
	n = 0;
	while (*s>='0' && *s<='9') {
		n = n*10 + *s++ - '0';
		if (n > 127)
			return(0);
	}
	return(*s==0);
}

/*
 * p moves something into the register d, and rt1 and rt2 are its
 * operands.  If the next instruction only reads the register, and
 * nothing after it does, have it read what was moved, and take the
 * move out.
 */
fwdsub(p, d)
register struct node *p;
{
	register struct node *q;
	register char *a;
	static char src[LSIZE];
	int sreg, smem, sseg;

	q = p->forw;
	a = rname[d];
	strcpy(src, rt2);
	sreg = isreg(src) >= 0;
	smem = ismem(src);
	sseg = segreg(src);
	if (smem && !wordmem(src) || !sreg && !smem && !sseg && !immed(src))
		return(0);
	switch (q->op) {

	case PUSH:
		if (q->code==0 || !equstr(q->code, a) || !dead(d, q))
			return(0);
		q->code = copy(src, "");
		break;

	case MOV:
	case ADD:
	case SUB:
	case AND:
	case OR:
	case XOR:
	case ADC:
	case SBB:
	case CMP:
	case TEST:
		if (sseg)
			return(0);
		dualop(q);
		if (equstr(rt2, a) && !mention(rt1, d)) {
			/*
			 * It is what q moves, or adds, or compares with.
			 */
			if (smem && isreg(rt1) < 0)
				goto no;
			if (!sreg && !smem && isreg(rt1) < 0 && !wordmem(rt1))
				goto no;
			if (!dead(d, q))
				goto no;
			q->code = copy(rt1, ",");
			q->code = copy(q->code, src);
			break;
		}
		if ((q->op==CMP || q->op==TEST) && equstr(rt1, a)
		 && !mention(rt2, d)) {
			if (!sreg && !smem || smem && ismem(rt2))
				goto no;
			if (!dead(d, q))
				goto no;
			q->code = copy(src, ",");
			q->code = copy(q->code, rt2);
			break;
		}
	no:
		dualop(p);
		return(0);

	default:
		return(0);
	}
	delete(p);
	nfwd++;
	nchange++;
	return(1);
}

/*
 * A constant: a number or an address, and not a register of any kind.
 */
immed(s)
register char *s;
{
	if (anyreg(s) >= 0)
		return(0);
	if (s[0] && s[1] && s[2]==0 && (s[1]=='s' || s[1]=='p'))
		return(0);		/* sp, bp, and cs, ds, es, ss */
	return(1);
}

/*
 * Does anything read what is in the register r after p, before it is
 * given something else?  Each way that a conditional jump goes is
 * followed, a little way.
 */
dead(r, p)
struct node *p;
{
	register struct node *q;

	if (++gen <= 0) {
		for (q = first.forw; q; q = q->forw)
			q->mark = 0;
		gen = 1;
	}
	budget = 100;
	return(dscan(r, p->forw, 0));
}

dscan(r, p, depth)
register struct node *p;
{
	register int u;

	for (; p; p = p->forw) {
		if (--budget < 0)
			return(0);
		switch (p->op) {

		case LABEL:
			/*
			 * Come round to again, there is nothing new
			 * to find this way.
			 */
			if (p->mark==gen)
				return(1);
			p->mark = gen;
			continue;

		case JBR:
			if (p->ref==0)
				return(0);
			p = p->ref->back;
			continue;

		case CBR:
			if (p->ref==0 || depth >= 4)
				return(0);
			if (!dscan(r, p->ref, depth+1))
				return(0);
			continue;

		case EROU:
			/*
			 * AX and DX are what is returned, and SI and DI
			 * are whoever called's.
			 */
			return(r==BX || r==CX);
		}
		if ((u = ruse(p, r)) > 0)
			return(0);
		if (u < 0)
			return(1);
	}
	return(0);
}

/*
 * Is AH, as p leaves it, given something new before anything reads it,
 * each way a jump goes, as dead() has it of a register?  What names AL
 * alone, as a byte is stored or compared, leaves AH be.
 */
ahdead(p)
struct node *p;
{
	register struct node *q;

	if (++gen <= 0) {
		for (q = first.forw; q; q = q->forw)
			q->mark = 0;
		gen = 1;
	}
	budget = 100;
	return(ahscan(p->forw, 0));
}

ahscan(p, depth)
register struct node *p;
{
	static char u1[LSIZE], u2[LSIZE];

	for (; p; p = p->forw) {
		if (--budget < 0)
			return(0);
		switch (p->op) {

		case LABEL:
			if (p->mark==gen)
				return(1);
			p->mark = gen;
			continue;

		case JBR:
			if (p->ref==0)
				return(0);
			p = p->ref->back;
			continue;

		case CBR:
			if (p->ref==0 || depth >= 4)
				return(0);
			if (!ahscan(p->ref, depth+1))
				return(0);
			continue;

		case CBW:
			return(1);

		case MOV:
		case LEA:
		case POP:
			split(p->code, u1, u2);
			if (word(u2, "ax") || word(u2, "ah"))
				return(0);
			if (isreg(u1)==AX || equstr(u1, "ah"))
				return(1);
			if (word(u1, "ax") || word(u1, "ah"))
				return(0);
			continue;

		case XOR:
		case SUB:
			split(p->code, u1, u2);
			if (isreg(u1)==AX && equstr(u1, u2))
				return(1);
		case PUSH:
		case CMP:
		case TEST:
		case ADD:
		case AND:
		case OR:
		case ADC:
		case SBB:
		case INC:
		case DEC:
		case NEG:
		case NOT:
		case SHIFT:
		case XCHG:
			if (word(p->code, "ax") || word(p->code, "ah"))
				return(0);
			continue;

		case JSR:
			return(ruse(p, AX) < 0);
		}
		return(0);
	}
	return(0);
}

/*
 * What p does with the register r: 1 if it reads it, or any of it, -1
 * if it gives it something new without reading it, and 0 if it leaves
 * it be.  What is not known reads everything.
 */
ruse(p, r)
register struct node *p;
{
	register char *s, **hp;
	static char u1[LSIZE], u2[LSIZE];

	switch (p->op) {

	case LEAVE:
		return(0);

	case CBW:
		return(r==AX);

	case CWD:
		return(r==AX? 1: r==DX? -1: 0);

	case JSR:
		if (r==SI)
			return(1);
		s = p->code;
		if (s[0]=='f' && s[1]=='a' && s[2]=='r' && s[3]==' ')
			s += 4;
		if (ismem(s) || anyreg(s) >= 0) {
			if (mention(s, r))
				return(1);
		} else for (hp = helper; *hp; hp++)
			if (equstr(s, *hp))
				return(r < SI);
		return(r<SI? -1: 0);

	case PUSH:
	case CMP:
	case TEST:
	case ADD:
	case AND:
	case OR:
	case ADC:
	case SBB:
	case INC:
	case DEC:
	case NEG:
	case NOT:
	case SHIFT:
	case XCHG:
		return(mention(p->code, r));

	case XOR:
	case SUB:
		split(p->code, u1, u2);
		if (isreg(u1)==r && equstr(u1, u2))
			return(-1);
		return(mention(p->code, r));

	case MUL:
		split(p->code, u1, u2);
		if (u2[0]==0 && (r==AX || r==DX))
			return(1);
		return(mention(p->code, r));

	case MOV:
	case LEA:
	case LES:
	case POP:
		split(p->code, u1, u2);
		if (mention(u2, r))
			return(1);
		if (isreg(u1)==r)
			return(-1);
		if (lowreg(u1)==r) {
			/*
			 * Its low byte, and then the high one made
			 * from that, or cleared.
			 */
			if (p->forw->op==CBW && r==AX)
				return(-1);
			s = p->forw->code;
			if (p->forw->op==XOR && s[0]==hname[r][0] && s[1]=='h'
			 && s[2]==',' && s[3]==s[0] && s[4]=='h' && s[5]==0)
				return(-1);
		}
		return(mention(u1, r));
	}
	return(1);
}

/*
 * Are the flags that p sets read by nothing?
 */
flagsdead(p)
register struct node *p;
{
	register int n;

	for (n = 16; --n >= 0 && (p = p->forw);) switch (p->op) {

	case LABEL:
	case MOV:
	case LEA:
	case LES:
	case PUSH:
	case POP:
	case XCHG:
	case CBW:
	case CWD:
	case NOT:
	case LEAVE:
		continue;

	case JBR:
		if ((p = p->ref) == 0)
			return(0);
		continue;

	case CMP:
	case TEST:
	case ADD:
	case SUB:
	case AND:
	case OR:
	case XOR:
	case NEG:
	case JSR:
	case EROU:
		return(1);

	default:
		return(0);
	}
	return(0);
}

jumpsw()
{
	register struct node *p, *p1;
	register t;
	register struct node *tp;
	int nj;

	t = 0;
	nj = 0;
	for (p=first.forw; p!=0; p = p->forw)
		p->refc = ++t;
	for (p=first.forw; p!=0; p = p1) {
		p1 = p->forw;
		if (p->op == CBR && p1->op==JBR && p->ref && p1->ref
		 && abs(p->refc - p->ref->refc) > abs(p1->refc - p1->ref->refc)) {
			if (p->ref==p1->ref)
				continue;
			p->subop = revbr[p->subop];
			tp = p1->ref;
			p1->ref = p->ref;
			p->ref = tp;
			t = p1->labno;
			p1->labno = p->labno;
			p->labno = t;
			nrevbr++;
			nj++;
		}
	}
	return(nj);
}

abs(x)
register int x;
{
	return(x<0? -x: x);
}

equop(ap1, p2)
struct node *ap1, *p2;
{
	register char *cp1, *cp2;
	register struct node *p1;

	p1 = ap1;
	if (p1->op!=p2->op || p1->subop!=p2->subop)
		return(0);
	if (p1->op<MOV)
		return(0);
	cp1 = p1->code;
	cp2 = p2->code;
	if (cp1==0 && cp2==0)
		return(1);
	if (cp1==0 || cp2==0)
		return(0);
	while (*cp1 == *cp2++)
		if (*cp1++ == 0)
			return(1);
	return(0);
}

decref(p)
register struct node *p;
{
	if (--p->refc <= 0) {
		nrlab++;
		p->back->forw = p->forw;
		p->forw->back = p->back;
	}
}

struct node *
nonlab(p)
register struct node *p;
{
	CHECK(10);
	while (p && p->op==LABEL)
		p = p->forw;
	return(p);
}

clearreg()
{
	register int i;

	for (i=0; i<NTRACK; i++)
		regs[i][0] = '\0';
}

/*
 * The register r has what the operand s is, if that is something
 * that can be said to stay as it is.
 */
savereg(r, s)
register char *s;
{
	register char *p;

	p = regs[r];
	*p = 0;
	if (strlen(s) > MAXCPS || mention(s, r))
		return;
	if (ismem(s)) {
		if (!wordmem(s) && *s!='&' || segpre(s))
			return;
	} else if (isreg(s) < 0 && !immed(s) && !equstr(s, "ss"))
		return;
	strcpy(p, s);
}

/*
 * The operand s is written, or changed: forget what is known of it,
 * and what is known by way of it.
 */
dest(s)
register char *s;
{
	register int i, r;

	if ((r = anyreg(s)) >= 0) {
		regs[r][0] = 0;
		for (i = 0; i<NTRACK; i++)
			if (mention(regs[i], r))
				regs[i][0] = 0;
		if (r==BX)
			regs[ESBX][0] = 0;
		return;
	}
	if (!ismem(s)) {
		if (equstr(s, "es")) {
			regs[ESBX][0] = 0;
			regs[ES][0] = 0;
			return;
		}
		if (!equstr(s, "sp")) {
			clearreg();
			return;
		}
		/*
		 * What was above the stack is not now.
		 */
		for (i = 0; i<NTRACK; i++)
			if (indirect(regs[i]))
				regs[i][0] = 0;
		return;
	}
	for (i = 0; i<NTRACK; i++)
		if (regs[i][0] && regs[i][0]!='&' && ismem(regs[i])
		 && alias(regs[i], s))
			regs[i][0] = 0;
}

/*
 * May writing one of two places in memory change the other?  Only two
 * places that are named, or are in the frame, are known not to be the
 * same; anything by way of a register may be anywhere.
 */
alias(a, b)
char *a, *b;
{
	static char ba[LSIZE], bb[LSIZE];
	register int oa, ob;

	if (indirect(a) || indirect(b))
		return(1);
	oa = maddr(a, ba);
	ob = maddr(b, bb);
	if (!equstr(ba, bb))
		return(0);
	return(oa < ob+msize(b) && ob < oa+msize(a));
}

/*
 * The address in a memory operand: what is between the brackets goes
 * to base, less a number added to it or taken from it at the end,
 * which is returned.  SS is left out: [ss:_x] may be [_x], where DS is
 * SS, and an in-line function's body says ss: where the compiler does
 * not (../../ccom/i286/c10.c).
 */
maddr(s, base)
register char *s;
char *base;
{
	register char *p, *e;
	register n;

	s = index(s, '[')+1;
	if (s[0]=='s' && s[1]=='s' && s[2]==':')
		s += 3;
	for (p = base; *s && *s!=']';)
		*p++ = *s++;
	*p = 0;
	e = p;
	while (p > base && p[-1]>='0' && p[-1]<='9')
		p--;
	if (p==e || p==base || p[-1]!='+' && p[-1]!='-')
		return(0);
	n = 0;
	for (s = p; *s; s++)
		n = n*10 + *s - '0';
	if (p[-1]=='-')
		n = -n;
	p[-1] = 0;
	return(n);
}

/*
 * The number of bytes a memory operand is, or more than any is if it
 * does not say.
 */
msize(s)
register char *s;
{
	switch (*s) {

	case 'b':
		return(1);

	case 'w':
		return(2);

	case 'd':
	case 'f':
		return(4);
	}
	return(8);
}

indirect(s)
register char *s;
{
	register int i;

	if (!ismem(s))
		return(0);
	for (i = 0; i<NREG; i++)
		if (mention(s, i))
			return(1);
	return(segpre(s));
}

/*
 * Is the memory operand s by way of a segment register other than SS?
 * SS is the program's data's in every model, and in the large one the
 * compiler names it where the others name none (../../ccom/i286), so
 * what is there is as much known as what is at a name or in the frame.
 */
segpre(s)
register char *s;
{
	if ((s = index(s, '[')) == 0)
		return(0);
	if (s[1]=='s' && s[2]=='s' && s[3]==':')
		return(0);
	return(index(s, ':') != 0);
}

/*
 * Is s a segment register?
 */
segreg(s)
register char *s;
{
	return(s[0] && index("cdes", s[0]) && s[1]=='s' && s[2]==0);
}

ismem(s)
char *s;
{
	return(index(s, '[') != 0);
}

wordmem(s)
register char *s;
{
	return(s[0]=='w' && s[1]=='o' && s[2]=='r' && s[3]=='d' && s[4]==' '
	    && s[5]=='[');
}

singop(ap)
struct node *ap;
{
	register char *p1, *p2;

	p1 = ap->code;
	p2 = rt1;
	if (p1)
		while (*p2++ = *p1++);
	else
		*p2 = 0;
	rt2[0] = 0;
}

dualop(ap)
struct node *ap;
{
	split(ap->code, rt1, rt2);
}

/*
 * The operands in code: the first to a1, and what is after its comma
 * to a2.
 */
split(code, a1, a2)
char *code, *a1, *a2;
{
	register char *p1, *p2;

	p1 = code;
	p2 = a1;
	*a1 = 0;
	*a2 = 0;
	if (p1==0)
		return;
	while (*p1 && *p1!=',')
		*p2++ = *p1++;
	*p2++ = 0;
	p2 = a2;
	if (*p1++ !=',')
		return;
	while (*p1==' ' || *p1=='\t')
		p1++;
	while (*p2++ = *p1++)
		;
}

/*
 * The register that is known to have what the operand is.
 */
findrand(as)
char *as;
{
	register int i;

	if (*as==0)
		return(-1);
	for (i = 0; i<NREG; i++) {
		if (equstr(regs[i], as))
			return(i);
	}
	return(-1);
}

isreg(as)
char *as;
{
	register char *s;
	register int i;

	s = as;
	if (s[0]==0 || s[1]==0 || s[2]!=0)
		return(-1);
	for (i = 0; i<NREG; i++)
		if (s[0]==rname[i][0] && s[1]==rname[i][1])
			return(i);
	return(-1);
}

/*
 * The register that the operand is the low byte of.
 */
lowreg(s)
register char *s;
{
	register int i;

	if (s[0]==0 || s[1]!='l' || s[2]!=0)
		return(-1);
	for (i = 0; i<4; i++)
		if (s[0]==lname[i][0])
			return(i);
	return(-1);
}

/*
 * The register that the operand is, or is half of.
 */
anyreg(s)
register char *s;
{
	register int i;

	if ((i = isreg(s)) >= 0 || (i = lowreg(s)) >= 0)
		return(i);
	if (s[0]==0 || s[1]!='h' || s[2]!=0)
		return(-1);
	for (i = 0; i<4; i++)
		if (s[0]==hname[i][0])
			return(i);
	return(-1);
}

idchar(c)
register c;
{
	return(c>='a' && c<='z' || c>='A' && c<='Z' || c>='0' && c<='9'
	    || c=='_' || c=='$' || c=='.' || c=='@' || c=='?');
}

/*
 * Is the name w in the operand s, as a word of its own?
 */
word(s, w)
register char *s;
char *w;
{
	register char *p, *q;
	register int prev;

	prev = 0;
	for (; *s; s++) {
		if (prev==0 && *s==*w) {
			for (p = s, q = w; *q && *p==*q; p++, q++)
				;
			if (*q==0 && !idchar(*p))
				return(1);
		}
		prev = idchar(*s);
	}
	return(0);
}

/*
 * Does the operand s name the register r, or either half of it?
 */
mention(s, r)
register char *s;
register r;
{
	if (s==0 || *s==0)
		return(0);
	if (word(s, rname[r]))
		return(1);
	return(r<4 && (word(s, lname[r]) || word(s, hname[r])));
}

check()
{
	register struct node *p, *lp;
	register count;

	lp = &first;
	count = 0;
	for (p=first.forw; p!=0; p = p->forw) {
		if (++count > 10000)
			abort(0);
		if (p->back != lp)
			abort(1);
		lp = p;
	}
}

equstr(ap1, ap2)
char *ap1, *ap2;
{
	register char *p1, *p2;

	p1 = ap1;
	p2 = ap2;
	do {
		if (*p1++ != *p2)
			return(0);
	} while (*p2++);
	return(1);
}
