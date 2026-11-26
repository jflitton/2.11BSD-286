#if	!defined(lint) && defined(DOSCCS)
static	char	sccsid[] = "@(#)opset.c 3.0 (2.11BSD) 2026/11/25";
#endif

/*
 * The instructions of the 286 and the 287, for the i format, as NASM
 * writes them and the system's assembler reads them (bin/as/i286), in
 * adb's radix, hex.  Where a jump or a call goes, and a memory operand
 * that is an address alone, are given as a symbol where one is near
 * enough.  A byte that begins no instruction of the 286's is given as
 * db, and is one byte long.
 */
#include "defs.h"
#include <string.h>

	char	*errflg;
	long	dot;
	int	dotinc;

/*
 * One string for each of the first bytes: the mnemonic, and after a |
 * the operands.  An operand is a register, written as it is, or one of
 *
 *	Eb Ew	the mod r/m byte's register or memory, a byte or a word
 *	Gb Gw	its reg field, as a register
 *	Sw	its reg field, as a segment register
 *	M-	memory, of no size: lea and bound
 *	Mp	memory holding a far pointer: les and lds
 *	Ib Iw	an immediate byte or word
 *	Is	an immediate byte, extended with its sign to a word
 *	Ia	aam's and aad's base, not written when it is ten
 *	Jb Jw	where a jump or call to an offset from the next goes
 *	Ap	a selector and offset: a far jump or call
 *	Ob Ow	a byte or word at an offset, mov's short form
 *	Zb Zw	the register the low three bits of the opcode name
 *
 * A mnemonic @N is group N's, by the reg field (grptab).  A 0 is a
 * prefix, or not an instruction.  The 287's are in fptab.
 */
static	char	*optab[256] = {
/* 00 */ "add|Eb,Gb", "add|Ew,Gw", "add|Gb,Eb", "add|Gw,Ew",
	 "add|al,Ib", "add|ax,Iw", "push|es", "pop|es",
/* 08 */ "or|Eb,Gb", "or|Ew,Gw", "or|Gb,Eb", "or|Gw,Ew",
	 "or|al,Ib", "or|ax,Iw", "push|cs", 0,
/* 10 */ "adc|Eb,Gb", "adc|Ew,Gw", "adc|Gb,Eb", "adc|Gw,Ew",
	 "adc|al,Ib", "adc|ax,Iw", "push|ss", "pop|ss",
/* 18 */ "sbb|Eb,Gb", "sbb|Ew,Gw", "sbb|Gb,Eb", "sbb|Gw,Ew",
	 "sbb|al,Ib", "sbb|ax,Iw", "push|ds", "pop|ds",
/* 20 */ "and|Eb,Gb", "and|Ew,Gw", "and|Gb,Eb", "and|Gw,Ew",
	 "and|al,Ib", "and|ax,Iw", 0, "daa",
/* 28 */ "sub|Eb,Gb", "sub|Ew,Gw", "sub|Gb,Eb", "sub|Gw,Ew",
	 "sub|al,Ib", "sub|ax,Iw", 0, "das",
/* 30 */ "xor|Eb,Gb", "xor|Ew,Gw", "xor|Gb,Eb", "xor|Gw,Ew",
	 "xor|al,Ib", "xor|ax,Iw", 0, "aaa",
/* 38 */ "cmp|Eb,Gb", "cmp|Ew,Gw", "cmp|Gb,Eb", "cmp|Gw,Ew",
	 "cmp|al,Ib", "cmp|ax,Iw", 0, "aas",
/* 40 */ "inc|Zw", "inc|Zw", "inc|Zw", "inc|Zw",
	 "inc|Zw", "inc|Zw", "inc|Zw", "inc|Zw",
/* 48 */ "dec|Zw", "dec|Zw", "dec|Zw", "dec|Zw",
	 "dec|Zw", "dec|Zw", "dec|Zw", "dec|Zw",
/* 50 */ "push|Zw", "push|Zw", "push|Zw", "push|Zw",
	 "push|Zw", "push|Zw", "push|Zw", "push|Zw",
/* 58 */ "pop|Zw", "pop|Zw", "pop|Zw", "pop|Zw",
	 "pop|Zw", "pop|Zw", "pop|Zw", "pop|Zw",
/* 60 */ "pusha", "popa", "bound|Gw,M-", "arpl|Ew,Gw",
	 0, 0, 0, 0,
/* 68 */ "push|Iw", "imul|Gw,Ew,Iw", "push|Is", "imul|Gw,Ew,Is",
	 "insb", "insw", "outsb", "outsw",
/* 70 */ "jo|Jb", "jno|Jb", "jc|Jb", "jnc|Jb",
	 "jz|Jb", "jnz|Jb", "jna|Jb", "ja|Jb",
/* 78 */ "js|Jb", "jns|Jb", "jpe|Jb", "jpo|Jb",
	 "jl|Jb", "jnl|Jb", "jng|Jb", "jg|Jb",
/* 80 */ "@1|Eb,Ib", "@1|Ew,Iw", "@1|Eb,Ib", "@1|Ew,Is",
	 "test|Eb,Gb", "test|Ew,Gw", "xchg|Gb,Eb", "xchg|Gw,Ew",
/* 88 */ "mov|Eb,Gb", "mov|Ew,Gw", "mov|Gb,Eb", "mov|Gw,Ew",
	 "mov|Ew,Sw", "lea|Gw,M-", "mov|Sw,Ew", "@6|Ew",
/* 90 */ "nop", "xchg|ax,Zw", "xchg|ax,Zw", "xchg|ax,Zw",
	 "xchg|ax,Zw", "xchg|ax,Zw", "xchg|ax,Zw", "xchg|ax,Zw",
/* 98 */ "cbw", "cwd", "call|Ap", "wait",
	 "pushf", "popf", "sahf", "lahf",
/* A0 */ "mov|al,Ob", "mov|ax,Ow", "mov|Ob,al", "mov|Ow,ax",
	 "movsb", "movsw", "cmpsb", "cmpsw",
/* A8 */ "test|al,Ib", "test|ax,Iw", "stosb", "stosw",
	 "lodsb", "lodsw", "scasb", "scasw",
/* B0 */ "mov|Zb,Ib", "mov|Zb,Ib", "mov|Zb,Ib", "mov|Zb,Ib",
	 "mov|Zb,Ib", "mov|Zb,Ib", "mov|Zb,Ib", "mov|Zb,Ib",
/* B8 */ "mov|Zw,Iw", "mov|Zw,Iw", "mov|Zw,Iw", "mov|Zw,Iw",
	 "mov|Zw,Iw", "mov|Zw,Iw", "mov|Zw,Iw", "mov|Zw,Iw",
/* C0 */ "@2|Eb,Ib", "@2|Ew,Ib", "ret|Iw", "ret",
	 "les|Gw,Mp", "lds|Gw,Mp", "@7|Eb,Ib", "@7|Ew,Iw",
/* C8 */ "enter|Iw,Ib", "leave", "retf|Iw", "retf",
	 "int3", "int|Ib", "into", "iret",
/* D0 */ "@2|Eb,1", "@2|Ew,1", "@2|Eb,cl", "@2|Ew,cl",
	 "aam|Ia", "aad|Ia", 0, "xlatb",
/* D8 */ 0, 0, 0, 0, 0, 0, 0, 0,
/* E0 */ "loopne|Jb", "loope|Jb", "loop|Jb", "jcxz|Jb",
	 "in|al,Ib", "in|ax,Ib", "out|Ib,al", "out|Ib,ax",
/* E8 */ "call|Jw", "jmp|Jw", "jmp|Ap", "jmp|Jb",
	 "in|al,dx", "in|ax,dx", "out|dx,al", "out|dx,ax",
/* F0 */ 0, 0, 0, 0, "hlt", "cmc", "@3|Eb", "@3|Ew",
/* F8 */ "clc", "stc", "cli", "sti", "cld", "std", "@4|Eb", "@5|Ew",
};

/*
 * The groups, whose instruction is the mod r/m byte's reg field.  An
 * operand after a | is in place of the table's: group 3's test has an
 * immediate as well, and group 5's far call and jump take a far pointer
 * in memory.  Group 8 is 0F 00, and 9 is 0F 01.
 */
static	char	*grptab[10][8] = {
	{ 0 },
/* 1 */	{ "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" },
/* 2 */	{ "rol", "ror", "rcl", "rcr", "shl", "shr", 0, "sar" },
/* 3 */	{ "test|+I", 0, "not", "neg", "mul", "imul", "div", "idiv" },
/* 4 */	{ "inc", "dec" },
/* 5 */	{ "inc", "dec", "call", "call|Fp", "jmp", "jmp|Fp", "push" },
/* 6 */	{ "pop" },
/* 7 */	{ "mov" },
/* 8 */	{ "sldt", "str", "lldt", "ltr", "verr", "verw" },
/* 9 */	{ "sgdt|M-", "sidt|M-", "lgdt|M-", "lidt|M-", "smsw", 0, "lmsw" },
};

/*
 * The 287's: for each of D8 to DF and each reg field, the instruction
 * on memory, with a letter for the size of the memory (d, q, t, w, or
 * - for what has no size of its own: an environment, a control word).
 */
static	char	*fptab[8][8] = {
/* D8 */ { "fadd|d", "fmul|d", "fcom|d", "fcomp|d",
	   "fsub|d", "fsubr|d", "fdiv|d", "fdivr|d" },
/* D9 */ { "fld|d", 0, "fst|d", "fstp|d",
	   "fldenv|-", "fldcw|-", "fnstenv|-", "fnstcw|-" },
/* DA */ { "fiadd|d", "fimul|d", "ficom|d", "ficomp|d",
	   "fisub|d", "fisubr|d", "fidiv|d", "fidivr|d" },
/* DB */ { "fild|d", 0, "fist|d", "fistp|d", 0, "fld|t", 0, "fstp|t" },
/* DC */ { "fadd|q", "fmul|q", "fcom|q", "fcomp|q",
	   "fsub|q", "fsubr|q", "fdiv|q", "fdivr|q" },
/* DD */ { "fld|q", 0, "fst|q", "fstp|q", "frstor|-", 0, "fnsave|-", "fnstsw|-" },
/* DE */ { "fiadd|w", "fimul|w", "ficom|w", "ficomp|w",
	   "fisub|w", "fisubr|w", "fidiv|w", "fidivr|w" },
/* DF */ { "fild|w", 0, "fist|w", "fistp|w",
	   "fbld|t", "fild|q", "fbstp|t", "fistp|q" },
};

/*
 * The 287's on registers.  D8's arithmetic is st0 with sti, DC's sti with
 * st0, and DE's the same, popping; in DC and DE the reversed subtraction
 * and division have each other's reg fields.  A row is the reg field's,
 * for an instruction whose rm field names the register; the rest are
 * one each.
 */
static	char	*fparith[8] = {
	"fadd", "fmul", "fcom", "fcomp", "fsub", "fsubr", "fdiv", "fdivr" };
static	char	*fpd9[32] = {
/* E0 */ "fchs", "fabs", 0, 0, "ftst", "fxam", 0, 0,
/* E8 */ "fld1", "fldl2t", "fldl2e", "fldpi", "fldlg2", "fldln2", "fldz", 0,
/* F0 */ "f2xm1", "fyl2x", "fptan", "fpatan", "fxtract", 0, "fdecstp", "fincstp",
/* F8 */ "fprem", "fyl2xp1", "fsqrt", 0, "frndint", "fscale", 0, 0,
};
static	char	*fpdb[8] = {
	"fneni", "fndisi", "fnclex", "fninit", "fsetpm" };

static	char	*breg[8] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
static	char	*wreg[8] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
static	char	*sreg[8] = { "es", "cs", "ss", "ds" };
static	char	*rmname[8] = {
	"bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx" };

static	int	space;		/* the instruction is in this space, */
static	u_int	nspw;		/*   or with NSP is this word */
static	int	ilen;		/* bytes of it read so far */
static	long	cadr;		/* the word read last, where */
static	u_int	cword;		/*   and what */
static	char	*seg;		/* a segment override not yet written */
static	int	modrm;		/* the mod r/m byte */
static	int	opcode;
static	int	nopnd;		/* operands written */

static int
ibyte()
{
	register int b;
	long	a;

	IF space == NSP
	THEN	b = ilen < 2 ? (nspw >> (ilen * 8)) & LOBYTE : 0;
	ELSE	a = inkdot(ilen);
		IF cadr != -1L ANDF a == cadr + 1
		THEN	b = (cword >> 8) & LOBYTE;
			cadr = -1L;
		ELSE	cword = get(a, space);
			cadr = a;
			b = cword & LOBYTE;
		FI
	FI
	ilen++;
	return(b);
}

static u_int
iword()
{
	register u_int w;

	w = ibyte();
	return(w | (ibyte() << 8));
}

static
comma()
{
	printf(nopnd++ ? "," : "%8t");
}

/* a signed displacement or immediate */
static
psigned(v, plus)
	register int v;
	int plus;
{
	IF v < 0
	THEN	printf("-%x", -v);
	ELSE	printf(plus ? "+%x" : "%x", v);
	FI
}

/* the memory the mod r/m byte names; size is a word for it or 0 */
static
pmem(size)
	char	*size;
{
	register int mod, rm;
	u_int	d;

	mod = modrm >> 6;
	rm = modrm & 07;
	IF size THEN printf("%s ", size); FI
	printc('[');
	IF seg
	THEN	printf("%s:", seg);
	FI
	IF mod == 0 ANDF rm == 6
	THEN	d = iword();
		IF seg
		THEN	printf("%x", d);
		ELSE	psymoff(leng(d), DSYM, "");
		FI
	ELSE	printf("%s", rmname[rm]);
		IF mod == 1
		THEN	psigned((char)ibyte(), 1);
		ELIF mod == 2
		THEN	psigned((short)iword(), 1);
		FI
	FI
	printc(']');
	seg = 0;
}

/* where a jump goes: disp from the end of the instruction */
static
ptarget(disp)
	int	disp;
{
	u_int	off;

	off = AOFF(dot) + ilen + disp;
	psymoff(ADDR(ASEL(dot), off), ISYM, "");
}

/*
 * The operands, from the table's string for them.  sized says that no
 * register among them gives the size of a memory operand, which is then
 * written as byte or word.
 */
static
operands(s, sized)
	register char *s;
	int	sized;
{
	register int c;
	u_int	off, sel;
	char	*size;

	WHILE *s
	DO	comma();
		c = *s++;
		IF c >= 'a' ANDF c <= 'z' ORF c == '1'
		THEN	/* a register, or shift's 1, as it is */
			printc(c);
			WHILE *s ANDF *s != ','
			DO printc(*s++); OD
		ELSE	switch (c) {
			case 'E':
				IF (modrm >> 6) == 3
				THEN	printf("%s", (*s == 'b' ? breg : wreg)[modrm & 07]);
				ELSE	pmem(sized ? (*s == 'b' ? "byte" : "word") : (char *)0);
				FI
				break;
			case 'G':
				printf("%s", (*s == 'b' ? breg : wreg)[(modrm >> 3) & 07]);
				break;
			case 'S':
				printf("%s", sreg[(modrm >> 3) & 07]);
				break;
			case 'M':
				pmem((char *)0);
				break;
			case 'F':
				pmem("far");
				break;
			case 'I':
				IF *s == 'b'
				THEN printf("%x", ibyte());
				ELIF *s == 'w'
				THEN printf("%x", iword());
				ELIF *s == 's'
				THEN psigned((char)ibyte(), 0);
				FI
				break;
			case 'J':
				IF *s == 'b'
				THEN	ptarget((char)ibyte());
				ELSE	ptarget((short)iword());
				FI
				break;
			case 'A':
				off = iword();
				sel = iword();
				psymoff(ADDR(sel, off), ISYM, "");
				break;
			case 'O':
				size = 0;
				modrm = 6;	/* as [disp16] */
				pmem(size);
				break;
			case 'Z':
				printf("%s", (*s == 'b' ? breg : wreg)[opcode & 07]);
				break;
			}
			s++;
		FI
		IF *s == ',' THEN s++; FI
	OD
}

/* is there a register among the operands to give memory its size? */
static
unsized(s)
	register char *s;
{
	WHILE *s
	DO	IF *s == 'G' ORF *s == 'Z'
		    ORF (*s == 'a' ANDF (s[1] == 'l' ORF s[1] == 'x'))
		THEN	return(1);
		FI
		WHILE *s ANDF *s++ != ',' DONE
	OD
	return(0);
}

/* a byte that begins no instruction */
static
nogood(b)
{
	printf("db%8t%x", b);
	ilen = 1;
}

/* the operands in a table's string, after its | */
static char *
argsof(s)
	register char *s;
{
	WHILE *s ANDF *s != '|' DO s++; OD
	return(*s ? s + 1 : s);
}

/* write the mnemonic, before the |, and return the operands */
static char *
mnemonic(s)
	register char *s;
{
	WHILE *s ANDF *s != '|'
	DO printc(*s++); OD
	return(*s ? s + 1 : s);
}

/*
 * The 287's.  It is ESC, D8 to DF, then a mod r/m byte: memory, or with
 * mod 3 an instruction on its registers.
 */
static
fpins(first)
{
	register int esc, reg, rm;
	register char *s;
	char	*size;

	esc = opcode & 07;
	modrm = ibyte();
	reg = (modrm >> 3) & 07;
	rm = modrm & 07;
	IF (modrm >> 6) != 3
	THEN	IF (s = fptab[esc][reg]) == 0
		THEN	nogood(first); return;
		FI
		s = mnemonic(s);
		switch (*s) {
		case 'd':	size = "dword"; break;
		case 'q':	size = "qword"; break;
		case 't':	size = "tword"; break;
		case 'w':	size = "word"; break;
		default:	size = 0; break;
		}
		comma();
		pmem(size);
		return;
	FI
	switch (esc) {
	case 0:		/* D8: st0 with sti */
		printf("%s", fparith[reg]);
		IF reg == 2 ORF reg == 3
		THEN	printf("%8tst%d", rm);
		ELSE	printf("%8tst0,st%d", rm);
		FI
		return;
	case 1:		/* D9 */
		IF reg == 0
		THEN	printf("fld%8tst%d", rm); return;
		ELIF reg == 1
		THEN	printf("fxch%8tst%d", rm); return;
		ELIF reg == 2 ANDF rm == 0
		THEN	printf("fnop"); return;
		ELIF reg >= 4 ANDF (s = fpd9[(reg - 4) * 8 + rm])
		THEN	printf("%s", s); return;
		FI
		break;
	case 3:		/* DB */
		IF reg == 4 ANDF rm <= 4
		THEN	printf("%s", fpdb[rm]); return;
		FI
		break;
	case 4:		/* DC: sti with st0 */
	case 6:		/* DE: the same, and pop */
		IF reg == 2 ORF reg == 3
		THEN	IF esc == 6 ANDF reg == 3 ANDF rm == 1
			THEN	printf("fcompp"); return;
			FI
			break;
		FI
		printf("%s%s%8tst%d,st0", fparith[reg ^ (reg >= 4)],
		    esc == 6 ? "p" : "", rm);
		return;
	case 5:		/* DD */
		IF reg == 0
		THEN	printf("ffree%8tst%d", rm); return;
		ELIF reg == 2 ORF reg == 3
		THEN	printf("%s%8tst%d", reg == 2 ? "fst" : "fstp", rm);
			return;
		FI
		break;
	case 7:		/* DF */
		IF modrm == 0xe0
		THEN	printf("fnstsw%8tax"); return;
		FI
		break;
	}
	nogood(first);
}

/*
 * 0F, then: the 286's instructions for protected mode.
 */
static
twobyte(first)
{
	register int op;
	register char *s;

	op = ibyte();
	switch (op) {
	case 0x00:
	case 0x01:
		modrm = ibyte();
		s = grptab[8 + op][(modrm >> 3) & 07];
		IF s == 0 ORF (*argsof(s) == 'M' ANDF (modrm >> 6) == 3)
		THEN	nogood(first); return;
		FI
		s = mnemonic(s);
		operands(*s ? s : "Ew", 1);
		return;
	case 0x02:
	case 0x03:
		modrm = ibyte();
		printf(op == 2 ? "lar" : "lsl");
		operands("Gw,Ew", 0);
		return;
	case 0x05:
		printf("loadall");
		return;
	case 0x06:
		printf("clts");
		return;
	}
	nogood(first);
}

printins(idsp, ins)
	int	idsp;
	u_int	ins;
{
	register char *s;
	register int op;
	int	first;
	char	*rep, *lock, *args, *with;

	space = idsp;
	nspw = ins;
	ilen = 0;
	cadr = -1L;
	seg = rep = lock = 0;
	nopnd = 0;
	first = -1;

	/* prefixes */
	LOOP	op = ibyte();
		IF first < 0 THEN first = op; FI
		switch (op) {
		case 0x26: seg = "es"; continue;
		case 0x2e: seg = "cs"; continue;
		case 0x36: seg = "ss"; continue;
		case 0x3e: seg = "ds"; continue;
		case 0xf0: lock = "lock "; continue;
		case 0xf2: rep = "repne "; continue;
		case 0xf3: rep = "rep "; continue;
		}
		break;
	POOL
	opcode = op;

	IF op == 0x0f
	THEN	twobyte(first);
		goto out;
	ELIF op >= 0xd8 ANDF op <= 0xdf
	THEN	fpins(first);
		goto out;
	ELIF (s = optab[op]) == 0
	THEN	nogood(first);
		goto out;
	FI

	/*
	 * The mod r/m byte, if there is one, says what a group's
	 * instruction is, and whether the instruction is one at all.
	 */
	args = argsof(s);
	with = 0;
	IF *s == '@'
	THEN	modrm = ibyte();
		s = grptab[s[1] - '0'][(modrm >> 3) & 07];
		IF s == 0
		THEN	nogood(first);
			goto out;
		FI
		IF *(with = argsof(s)) == 0
		THEN	with = 0;
		ELIF *with == 'F' ANDF (modrm >> 6) == 3
		THEN	nogood(first);
			goto out;
		FI
	ELIF index(args, 'E') ORF index(args, 'G') ORF index(args, 'S')
	    ORF index(args, 'M')
	THEN	modrm = ibyte();
		IF index(args, 'M') ANDF (modrm >> 6) == 3
		    ORF index(args, 'S') ANDF (modrm & 040)
		    ORF op == 0x8e ANDF ((modrm >> 3) & 07) == 1
		THEN	nogood(first);		/* no such register, or mov cs */
			goto out;
		FI
	FI

	IF lock THEN printf("%s", lock); FI
	IF rep
	THEN	IF rep[3] == ' ' ANDF (op == 0xa6 ORF op == 0xa7 ORF op == 0xae
		    ORF op == 0xaf)
		THEN	printf("repe ");
		ELSE	printf("%s", rep);
		FI
	FI
	/* an override with no memory operand to go in */
	IF seg ANDF (index(args, 'E') == 0 ORF (modrm >> 6) == 3)
	    ANDF !index(args, 'M') ANDF !index(args, 'O')
	    ANDF (with == 0 ORF *with != 'F')
	THEN	printf("%s ", seg);
		seg = 0;
	FI
	mnemonic(s);
	IF with ANDF *with == 'F'
	THEN	operands(with, 0);
	ELIF op == 0xd4 ORF op == 0xd5
	THEN	IF (op = ibyte()) != 10
		THEN	printf("%8t%x", op);
		FI
	ELSE	operands(args, !unsized(args));
		IF with ANDF *with == '+'
		THEN	operands(opcode & 1 ? "Iw" : "Ib", 0);
		FI
	FI
out:
	dotinc = ilen;
}
