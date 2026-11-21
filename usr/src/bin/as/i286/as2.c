/*
 * The assembler for the 80286: the instructions and the directives.
 *
 * Where an instruction can be written in more than one way the bytes
 * are the ones NASM chooses, which are the shortest.
 */
#include "as.h"

/*
 * In the order of the characters, for findinsn.
 */
struct insn insns[] = {
	"aaa",		C_NONE,		0,	0x37,
	"aad",		C_NONE2,	0,	0xD50A,
	"aam",		C_NONE2,	0,	0xD40A,
	"aas",		C_NONE,		0,	0x3F,
	"adc",		C_ALU,		0,	2,
	"add",		C_ALU,		0,	0,
	"align",	C_DIR,		0,	D_ALIGN,
	"and",		C_ALU,		0,	4,
	"arpl",		C_ARPL,		0,	0x63,
	"bits",		C_DIR,		0,	D_IGNORE,
	"bound",	C_LXS,		0,	0x62,
	"call",		C_CALL,		0,	0xE8,
	"cbw",		C_NONE,		0,	0x98,
	"clc",		C_NONE,		0,	0xF8,
	"cld",		C_NONE,		0,	0xFC,
	"cli",		C_NONE,		0,	0xFA,
	"clts",		C_NONE2,	0,	0x0F06,
	"cmc",		C_NONE,		0,	0xF5,
	"cmp",		C_ALU,		0,	7,
	"cmpsb",	C_NONE,		0,	0xA6,
	"cmpsw",	C_NONE,		0,	0xA7,
	"common",	C_DIR,		0,	D_COMMON,
	"cpu",		C_DIR,		0,	D_IGNORE,
	"cwd",		C_NONE,		0,	0x99,
	"daa",		C_NONE,		0,	0x27,
	"das",		C_NONE,		0,	0x2F,
	"db",		C_DIR,		1,	D_DB,
	"dd",		C_DIR,		4,	D_DB,
	"dec",		C_INCDEC,	0,	1,
	"div",		C_UN,		0,	6,
	"dw",		C_DIR,		2,	D_DB,
	"enter",	C_ENTER,	0,	0xC8,
	"extern",	C_DIR,		0,	D_EXTERN,
	"f2xm1",	C_NONE2,	0,	0xD9F0,
	"fabs",		C_NONE2,	0,	0xD9E1,
	"fadd",		C_FARITH,	0,	0,
	"faddp",	C_FARITHP,	0,	0xC0,
	"fchs",		C_NONE2,	0,	0xD9E0,
	"fclex",	C_NONE2,	1,	0xDBE2,
	"fcom",		C_FARITH,	0,	2,
	"fcomp",	C_FARITH,	0,	3,
	"fcompp",	C_NONE2,	0,	0xDED9,
	"fdecstp",	C_NONE2,	0,	0xD9F6,
	"fdiv",		C_FARITH,	0,	6,
	"fdivp",	C_FARITHP,	0,	0xF8,
	"fdivr",	C_FARITH,	0,	7,
	"fdivrp",	C_FARITHP,	0,	0xF0,
	"ffree",	C_FSTI,		0,	0xDDC0,
	"fiadd",	C_FIARITH,	0,	0,
	"ficom",	C_FIARITH,	0,	2,
	"ficomp",	C_FIARITH,	0,	3,
	"fidiv",	C_FIARITH,	0,	6,
	"fidivr",	C_FIARITH,	0,	7,
	"fild",		C_FILD,		0,	0,
	"fimul",	C_FIARITH,	0,	1,
	"fincstp",	C_NONE2,	0,	0xD9F7,
	"finit",	C_NONE2,	1,	0xDBE3,
	"fist",		C_FILD,		0,	2,
	"fistp",	C_FILD,		0,	3,
	"fisub",	C_FIARITH,	0,	4,
	"fisubr",	C_FIARITH,	0,	5,
	"fld",		C_FLD,		0,	0,
	"fld1",		C_NONE2,	0,	0xD9E8,
	"fldcw",	C_FMEM,		0,	0xD905,
	"fldenv",	C_FMEM,		0,	0xD904,
	"fldl2e",	C_NONE2,	0,	0xD9EA,
	"fldl2t",	C_NONE2,	0,	0xD9E9,
	"fldlg2",	C_NONE2,	0,	0xD9EC,
	"fldln2",	C_NONE2,	0,	0xD9ED,
	"fldpi",	C_NONE2,	0,	0xD9EB,
	"fldz",		C_NONE2,	0,	0xD9EE,
	"fmul",		C_FARITH,	0,	1,
	"fmulp",	C_FARITHP,	0,	0xC8,
	"fnclex",	C_NONE2,	0,	0xDBE2,
	"fninit",	C_NONE2,	0,	0xDBE3,
	"fnop",		C_NONE2,	0,	0xD9D0,
	"fnsave",	C_FMEM,		0,	0xDD06,
	"fnstcw",	C_FMEM,		0,	0xD907,
	"fnstenv",	C_FMEM,		0,	0xD906,
	"fnstsw",	C_FSTSW,	0,	0,
	"fpatan",	C_NONE2,	0,	0xD9F3,
	"fprem",	C_NONE2,	0,	0xD9F8,
	"fptan",	C_NONE2,	0,	0xD9F2,
	"frndint",	C_NONE2,	0,	0xD9FC,
	"frstor",	C_FMEM,		0,	0xDD04,
	"fsave",	C_FMEM,		1,	0xDD06,
	"fscale",	C_NONE2,	0,	0xD9FD,
	"fsqrt",	C_NONE2,	0,	0xD9FA,
	"fst",		C_FLD,		0,	2,
	"fstcw",	C_FMEM,		1,	0xD907,
	"fstenv",	C_FMEM,		1,	0xD906,
	"fstp",		C_FLD,		0,	3,
	"fstsw",	C_FSTSW,	1,	0,
	"fsub",		C_FARITH,	0,	4,
	"fsubp",	C_FARITHP,	0,	0xE8,
	"fsubr",	C_FARITH,	0,	5,
	"fsubrp",	C_FARITHP,	0,	0xE0,
	"ftst",		C_NONE2,	0,	0xD9E4,
	"fwait",	C_NONE,		0,	0x9B,
	"fxam",		C_NONE2,	0,	0xD9E5,
	"fxch",		C_FSTI,		1,	0xD9C8,
	"fxtract",	C_NONE2,	0,	0xD9F4,
	"fyl2x",	C_NONE2,	0,	0xD9F1,
	"fyl2xp1",	C_NONE2,	0,	0xD9F9,
	"global",	C_DIR,		0,	D_GLOBAL,
	"group",	C_DIR,		0,	D_IGNORE,
	"hlt",		C_NONE,		0,	0xF4,
	"idiv",		C_UN,		0,	7,
	"imul",		C_IMUL,		0,	5,
	"in",		C_IN,		0,	0xE4,
	"inc",		C_INCDEC,	0,	0,
	"insb",		C_NONE,		0,	0x6C,
	"insw",		C_NONE,		0,	0x6D,
	"int",		C_INT,		0,	0xCD,
	"int3",		C_NONE,		0,	0xCC,
	"into",		C_NONE,		0,	0xCE,
	"iret",		C_NONE,		0,	0xCF,
	"ja",		C_JCC,		0,	0x77,
	"jae",		C_JCC,		0,	0x73,
	"jb",		C_JCC,		0,	0x72,
	"jbe",		C_JCC,		0,	0x76,
	"jc",		C_JCC,		0,	0x72,
	"jcxz",		C_LOOP,		0,	0xE3,
	"je",		C_JCC,		0,	0x74,
	"jg",		C_JCC,		0,	0x7F,
	"jge",		C_JCC,		0,	0x7D,
	"jl",		C_JCC,		0,	0x7C,
	"jle",		C_JCC,		0,	0x7E,
	"jmp",		C_JMP,		0,	0xEB,
	"jna",		C_JCC,		0,	0x76,
	"jnae",		C_JCC,		0,	0x72,
	"jnb",		C_JCC,		0,	0x73,
	"jnbe",		C_JCC,		0,	0x77,
	"jnc",		C_JCC,		0,	0x73,
	"jne",		C_JCC,		0,	0x75,
	"jng",		C_JCC,		0,	0x7E,
	"jnge",		C_JCC,		0,	0x7C,
	"jnl",		C_JCC,		0,	0x7D,
	"jnle",		C_JCC,		0,	0x7F,
	"jno",		C_JCC,		0,	0x71,
	"jnp",		C_JCC,		0,	0x7B,
	"jns",		C_JCC,		0,	0x79,
	"jnz",		C_JCC,		0,	0x75,
	"jo",		C_JCC,		0,	0x70,
	"jp",		C_JCC,		0,	0x7A,
	"jpe",		C_JCC,		0,	0x7A,
	"jpo",		C_JCC,		0,	0x7B,
	"js",		C_JCC,		0,	0x78,
	"jz",		C_JCC,		0,	0x74,
	"lahf",		C_NONE,		0,	0x9F,
	"lar",		C_LAR,		0,	0x02,
	"lds",		C_LXS,		0,	0xC5,
	"lea",		C_LXS,		0,	0x8D,
	"leave",	C_NONE,		0,	0xC9,
	"les",		C_LXS,		0,	0xC4,
	"lgdt",		C_PROT,		0,	0x0102,
	"lidt",		C_PROT,		0,	0x0103,
	"lldt",		C_PROT,		0,	0x0002,
	"lmsw",		C_PROT,		0,	0x0106,
	"lock",		C_PREFIX,	0,	0xF0,
	"lodsb",	C_NONE,		0,	0xAC,
	"lodsw",	C_NONE,		0,	0xAD,
	"loop",		C_LOOP,		0,	0xE2,
	"loope",	C_LOOP,		0,	0xE1,
	"loopne",	C_LOOP,		0,	0xE0,
	"loopnz",	C_LOOP,		0,	0xE0,
	"loopz",	C_LOOP,		0,	0xE1,
	"lsl",		C_LAR,		0,	0x03,
	"ltr",		C_PROT,		0,	0x0003,
	"mov",		C_MOV,		0,	0,
	"movsb",	C_NONE,		0,	0xA4,
	"movsw",	C_NONE,		0,	0xA5,
	"mul",		C_UN,		0,	4,
	"neg",		C_UN,		0,	3,
	"nop",		C_NONE,		0,	0x90,
	"not",		C_UN,		0,	2,
	"or",		C_ALU,		0,	1,
	"out",		C_OUT,		0,	0xE6,
	"outsb",	C_NONE,		0,	0x6E,
	"outsw",	C_NONE,		0,	0x6F,
	"pop",		C_POP,		0,	0,
	"popa",		C_NONE,		0,	0x61,
	"popf",		C_NONE,		0,	0x9D,
	"push",		C_PUSH,		0,	0,
	"pusha",	C_NONE,		0,	0x60,
	"pushf",	C_NONE,		0,	0x9C,
	"rcl",		C_SHIFT,	0,	2,
	"rcr",		C_SHIFT,	0,	3,
	"rep",		C_PREFIX,	0,	0xF3,
	"repe",		C_PREFIX,	0,	0xF3,
	"repne",	C_PREFIX,	0,	0xF2,
	"repnz",	C_PREFIX,	0,	0xF2,
	"repz",		C_PREFIX,	0,	0xF3,
	"resb",		C_DIR,		1,	D_RES,
	"resd",		C_DIR,		4,	D_RES,
	"resw",		C_DIR,		2,	D_RES,
	"ret",		C_RET,		0,	0xC3,
	"retf",		C_RET,		0,	0xCB,
	"retn",		C_RET,		0,	0xC3,
	"rol",		C_SHIFT,	0,	0,
	"ror",		C_SHIFT,	0,	1,
	"sahf",		C_NONE,		0,	0x9E,
	"sal",		C_SHIFT,	0,	4,
	"sar",		C_SHIFT,	0,	7,
	"sbb",		C_ALU,		0,	3,
	"scasb",	C_NONE,		0,	0xAE,
	"scasw",	C_NONE,		0,	0xAF,
	"section",	C_DIR,		0,	D_SEGMENT,
	"segment",	C_DIR,		0,	D_SEGMENT,
	"sgdt",		C_PROT,		0,	0x0100,
	"shl",		C_SHIFT,	0,	4,
	"shr",		C_SHIFT,	0,	5,
	"sidt",		C_PROT,		0,	0x0101,
	"sldt",		C_PROT,		0,	0x0000,
	"smsw",		C_PROT,		0,	0x0104,
	"stc",		C_NONE,		0,	0xF9,
	"std",		C_NONE,		0,	0xFD,
	"sti",		C_NONE,		0,	0xFB,
	"stosb",	C_NONE,		0,	0xAA,
	"stosw",	C_NONE,		0,	0xAB,
	"str",		C_PROT,		0,	0x0001,
	"sub",		C_ALU,		0,	5,
	"test",		C_TEST,		0,	0,
	"times",	C_TIMES,	0,	0,
	"verr",		C_PROT,		0,	0x0004,
	"verw",		C_PROT,		0,	0x0005,
	"wait",		C_NONE,		0,	0x9B,
	"xchg",		C_XCHG,		0,	0,
	"xlat",		C_NONE,		0,	0xD7,
	"xlatb",	C_NONE,		0,	0xD7,
	"xor",		C_ALU,		0,	6,
};
#define	NINSNS	(sizeof insns / sizeof insns[0])

/*
 * The names of the file's segments, and which of the object's three
 * each is part of.
 */
char	*segname[NSEGN];
char	segsect[NSEGN];
int	nsegn;

tabcheck()
{
	register struct insn *ip;

	for (ip = insns; ip < &insns[NINSNS-1]; ip++)
		if (strcmp(ip->i_name, ip[1].i_name) >= 0)
			fatal("the table is out of order at %s", ip->i_name);
}

struct insn *
findinsn(s)
register char *s;
{
	register struct insn *lo, *hi, *ip;
	register c;

	lo = insns;
	hi = &insns[NINSNS-1];
	while (lo <= hi) {
		ip = lo + (hi - lo) / 2;
		c = *s - *ip->i_name;
		if (c == 0)
			c = strcmp(s, ip->i_name);
		if (c == 0)
			return(ip);
		if (c < 0)
			hi = ip - 1;
		else
			lo = ip + 1;
	}
	return(NULL);
}

operr()
{
	error("the operands do not suit the instruction", "");
}

/*
 * Is this a number that is known and that a byte holds, with its sign?
 */
small(e)
register struct expr *e;
{
	register unsigned v;

	if (e->e_sect != SABS || e->e_seg || e->e_fwd)
		return(0);
	v = (unsigned)e->e_val & 0177777;
	return(v < 0200 || v >= 0177600);
}

isrm(o)
register struct opnd *o;
{
	return(o->o_kind == K_REG || o->o_kind == K_REG8 ||
	    o->o_kind == K_MEM);
}

/*
 * The size of what an instruction works on, 1 or 2, from its operands.
 */
opsize(a, b)
register struct opnd *a, *b;
{
	register s, t;

	s = a->o_kind == K_REG || a->o_kind == K_SREG ? 2 :
	    a->o_kind == K_REG8 ? 1 : a->o_size;
	if (b) {
		t = b->o_kind == K_REG || b->o_kind == K_SREG ? 2 :
		    b->o_kind == K_REG8 ? 1 : b->o_size;
		if (s == 0)
			s = t;
		else if (t && t != s && b->o_kind != K_IMM)
			error("the operands are of two sizes", "");
	}
	if (s != 1 && s != 2) {
		error("a byte or a word?  The operands do not say", "");
		s = 2;
	}
	return(s);
}

/*
 * A segment's prefix, if the operand has one.
 */
prefix(o)
register struct opnd *o;
{
	if (o->o_kind == K_MEM && o->o_over)
		ob(o->o_over);
}

/*
 * The byte that says where an operand is, and its displacement.
 */
modrm(r, o)
register struct opnd *o;
{
	register unsigned v;

	r <<= 3;
	if (o->o_kind != K_MEM) {
		ob(0300 | r | o->o_reg);
		return;
	}
	if (o->o_rm < 0) {
		ob(r | 6);
		oword(&o->o_e);
		return;
	}
	if (small(&o->o_e)) {
		v = (unsigned)o->o_e.e_val & 0377;
		if (v == 0 && o->o_rm != 6)
			ob(r | o->o_rm);
		else {
			ob(0100 | r | o->o_rm);
			ob(v);
		}
		return;
	}
	ob(0200 | r | o->o_rm);
	oword(&o->o_e);
}

/*
 * A value that is a byte or a word.
 */
oimm(e, w)
struct expr *e;
{
	if (w)
		oword(e);
	else
		obyte(e);
}

assemble(ip)
register struct insn *ip;
{
	register struct opnd *a, *b;
	register n, w;
	struct expr e;

	n = ip->i_op;
	a = &op[0];
	b = &op[1];
	switch (ip->i_class) {

	case C_DIR:
		directive(ip);
		return;

	case C_PREFIX:
		ob(n);
		statement();
		return;
	}
	here = loc[sect];
	getops();
	switch (ip->i_class) {

	case C_NONE:
		if (nop)
			break;
		ob(n);
		return;

	case C_NONE2:
		if (nop)
			break;
		if (ip->i_arg)
			ob(0x9B);
		ob(n >> 8);
		ob(n);
		return;

	case C_ALU:
		if (nop != 2 || !isrm(a))
			break;
		w = opsize(a, b) == 2;
		if (b->o_kind == K_REG || b->o_kind == K_REG8) {
			prefix(a);
			ob(n << 3 | w);
			modrm(b->o_reg, a);
		} else if (b->o_kind == K_MEM) {
			if (a->o_kind == K_MEM)
				break;
			prefix(b);
			ob(n << 3 | 2 | w);
			modrm(a->o_reg, b);
		} else if (b->o_kind == K_IMM) {
			prefix(a);
			if (!w) {
				if (a->o_kind == K_REG8 && a->o_reg == 0)
					ob(n << 3 | 4);
				else {
					ob(0x80);
					modrm(n, a);
				}
				obyte(&b->o_e);
			} else if (small(&b->o_e)) {
				ob(0x83);
				modrm(n, a);
				ob((int)b->o_e.e_val);
			} else {
				if (a->o_kind == K_REG && a->o_reg == 0)
					ob(n << 3 | 5);
				else {
					ob(0x81);
					modrm(n, a);
				}
				oword(&b->o_e);
			}
		} else
			break;
		return;

	case C_MOV:
		if (nop != 2)
			break;
		if (a->o_kind == K_SREG) {
			if (b->o_kind != K_REG && b->o_kind != K_MEM)
				break;
			prefix(b);
			ob(0x8E);
			modrm(a->o_reg, b);
			return;
		}
		if (b->o_kind == K_SREG) {
			if (a->o_kind != K_REG && a->o_kind != K_MEM)
				break;
			prefix(a);
			ob(0x8C);
			modrm(b->o_reg, a);
			return;
		}
		if (!isrm(a))
			break;
		w = opsize(a, b) == 2;
		if (b->o_kind == K_REG || b->o_kind == K_REG8) {
			prefix(a);
			if (a->o_kind == K_MEM && a->o_rm < 0 &&
			    b->o_reg == 0) {
				ob(0xA2 | w);
				oword(&a->o_e);
			} else {
				ob(0x88 | w);
				modrm(b->o_reg, a);
			}
		} else if (b->o_kind == K_MEM) {
			if (a->o_kind == K_MEM)
				break;
			prefix(b);
			if (b->o_rm < 0 && a->o_reg == 0) {
				ob(0xA0 | w);
				oword(&b->o_e);
			} else {
				ob(0x8A | w);
				modrm(a->o_reg, b);
			}
		} else if (b->o_kind == K_IMM) {
			if (a->o_kind == K_MEM) {
				prefix(a);
				ob(0xC6 | w);
				modrm(0, a);
			} else
				ob(0xB0 | w << 3 | a->o_reg);
			oimm(&b->o_e, w);
		} else
			break;
		return;

	case C_TEST:
		if (nop != 2 || !isrm(a))
			break;
		w = opsize(a, b) == 2;
		if (b->o_kind == K_REG || b->o_kind == K_REG8) {
			prefix(a);
			ob(0x84 | w);
			modrm(b->o_reg, a);
		} else if (b->o_kind == K_MEM) {
			if (a->o_kind == K_MEM)
				break;
			prefix(b);
			ob(0x84 | w);
			modrm(a->o_reg, b);
		} else if (b->o_kind == K_IMM) {
			prefix(a);
			if (a->o_kind != K_MEM && a->o_reg == 0)
				ob(0xA8 | w);
			else {
				ob(0xF6 | w);
				modrm(0, a);
			}
			oimm(&b->o_e, w);
		} else
			break;
		return;

	case C_XCHG:
		if (nop != 2 || !isrm(a) || !isrm(b))
			break;
		w = opsize(a, b) == 2;
		if (a->o_kind == K_MEM) {
			a = b;
			b = &op[0];
		}
		if (a->o_kind == K_MEM)
			break;
		if (w && b->o_kind == K_REG && (a->o_reg == 0 ||
		    b->o_reg == 0)) {
			ob(0x90 | a->o_reg | b->o_reg);
			return;
		}
		prefix(b);
		ob(0x86 | w);
		modrm(a->o_reg, b);
		return;

	case C_LXS:
		if (nop != 2 || a->o_kind != K_REG || b->o_kind != K_MEM)
			break;
		prefix(b);
		ob(n);
		modrm(a->o_reg, b);
		return;

	case C_PUSH:
		if (nop != 1)
			break;
		if (a->o_kind == K_REG)
			ob(0x50 | a->o_reg);
		else if (a->o_kind == K_SREG)
			ob(0x06 | a->o_reg << 3);
		else if (a->o_kind == K_MEM) {
			prefix(a);
			ob(0xFF);
			modrm(6, a);
		} else if (a->o_kind == K_IMM) {
			if (small(&a->o_e)) {
				ob(0x6A);
				ob((int)a->o_e.e_val);
			} else {
				ob(0x68);
				oword(&a->o_e);
			}
		} else
			break;
		return;

	case C_POP:
		if (nop != 1)
			break;
		if (a->o_kind == K_REG)
			ob(0x58 | a->o_reg);
		else if (a->o_kind == K_SREG && a->o_reg != 1)
			ob(0x07 | a->o_reg << 3);
		else if (a->o_kind == K_MEM) {
			prefix(a);
			ob(0x8F);
			modrm(0, a);
		} else
			break;
		return;

	case C_INCDEC:
		if (nop != 1 || !isrm(a))
			break;
		if (a->o_kind == K_REG) {
			ob(0x40 | n << 3 | a->o_reg);
			return;
		}
		/* fall through */

	case C_UN:
		if (nop != 1 || !isrm(a))
			break;
		prefix(a);
		ob((ip->i_class == C_UN ? 0xF6 : 0xFE) |
		    (opsize(a, (struct opnd *)NULL) == 2));
		modrm(n, a);
		return;

	case C_IMUL:
		if (nop == 1) {
			if (!isrm(a))
				break;
			prefix(a);
			ob(0xF6 | (opsize(a, (struct opnd *)NULL) == 2));
			modrm(n, a);
			return;
		}
		if (nop == 2) {
			op[2] = op[1];
			op[1] = op[0];
		} else if (nop != 3)
			break;
		if (a->o_kind != K_REG || b->o_kind != K_REG &&
		    b->o_kind != K_MEM || op[2].o_kind != K_IMM)
			break;
		prefix(b);
		if (small(&op[2].o_e)) {
			ob(0x6B);
			modrm(a->o_reg, b);
			ob((int)op[2].o_e.e_val);
		} else {
			ob(0x69);
			modrm(a->o_reg, b);
			oword(&op[2].o_e);
		}
		return;

	case C_SHIFT:
		if (nop != 2 || !isrm(a))
			break;
		w = opsize(a, (struct opnd *)NULL) == 2;
		prefix(a);
		if (b->o_kind == K_REG8 && b->o_reg == 1) {
			ob(0xD2 | w);
			modrm(n, a);
		} else if (b->o_kind != K_IMM)
			break;
		else if (isnum(&b->o_e) && !b->o_e.e_fwd &&
		    b->o_e.e_val == 1) {
			ob(0xD0 | w);
			modrm(n, a);
		} else {
			ob(0xC0 | w);
			modrm(n, a);
			obyte(&b->o_e);
		}
		return;

	case C_JMP:
	case C_JCC:
		if (nop != 1)
			break;
		jump(ip);
		return;

	case C_CALL:
		if (nop != 1)
			break;
		if (a->o_kind == K_MEM) {
			prefix(a);
			ob(0xFF);
			modrm(a->o_dist == D_FAR ? 3 : 2, a);
		} else if (a->o_kind == K_REG) {
			ob(0xFF);
			ob(0320 | a->o_reg);
		} else if (a->o_kind != K_IMM)
			break;
		else if (a->o_dist == D_FAR)
			farptr(0x9A, &a->o_e);
		else {
			ob(0xE8);
			orel(&a->o_e);
		}
		return;

	case C_LOOP:
		if (nop != 1 || a->o_kind != K_IMM)
			break;
		ob(n);
		oshort(&a->o_e);
		return;

	case C_RET:
		if (nop == 0)
			ob(n);
		else if (nop == 1 && a->o_kind == K_IMM) {
			ob(n - 1);
			oword(&a->o_e);
		} else
			break;
		return;

	case C_INT:
		if (nop != 1 || a->o_kind != K_IMM)
			break;
		ob(n);
		obyte(&a->o_e);
		return;

	case C_OUT:
		if (nop != 2)
			break;
		a = b;
		b = &op[0];
		/* fall through */

	case C_IN:
		if (nop != 2 || a->o_kind != K_REG && a->o_kind != K_REG8 ||
		    a->o_reg != 0)
			break;
		n |= a->o_kind == K_REG;
		if (b->o_kind == K_REG && b->o_reg == 2)
			ob(n | 8);
		else if (b->o_kind == K_IMM) {
			ob(n);
			obyte(&b->o_e);
		} else
			break;
		return;

	case C_ENTER:
		if (nop != 2 || a->o_kind != K_IMM || b->o_kind != K_IMM)
			break;
		ob(n);
		oword(&a->o_e);
		obyte(&b->o_e);
		return;

	case C_PROT:
		if (nop != 1 || a->o_kind != K_REG && a->o_kind != K_MEM)
			break;
		prefix(a);
		ob(0x0F);
		ob(n >> 8);
		modrm(n & 7, a);
		return;

	case C_LAR:
		if (nop != 2 || a->o_kind != K_REG || b->o_kind != K_REG &&
		    b->o_kind != K_MEM)
			break;
		prefix(b);
		ob(0x0F);
		ob(n);
		modrm(a->o_reg, b);
		return;

	case C_ARPL:
		if (nop != 2 || b->o_kind != K_REG || a->o_kind != K_REG &&
		    a->o_kind != K_MEM)
			break;
		prefix(a);
		ob(n);
		modrm(b->o_reg, a);
		return;

	default:
		if (float87(ip))
			return;
	}
	operr();
}

/*
 * A jump, or a conditional one.  One to a place that is named has the
 * shorter form if that reaches, which the first pass cannot say
 * (relax).
 */
jump(ip)
register struct insn *ip;
{
	register struct opnd *o;
	register struct jump *jp;

	o = &op[0];
	if (ip->i_class == C_JMP) {
		if (o->o_kind == K_MEM) {
			prefix(o);
			ob(0xFF);
			modrm(o->o_dist == D_FAR ? 5 : 4, o);
			return;
		}
		if (o->o_kind == K_REG) {
			ob(0xFF);
			ob(0340 | o->o_reg);
			return;
		}
		if (o->o_kind == K_IMM && o->o_dist == D_FAR) {
			farptr(0xEA, &o->o_e);
			return;
		}
	}
	if (o->o_kind != K_IMM) {
		operr();
		return;
	}
	if (o->o_dist == D_SHORT) {
		ob(ip->i_op);
		oshort(&o->o_e);
		return;
	}
	/* (near says nothing: NASM makes a short jump of it if one reaches) */
	if (nexsym == 0 && o->o_e.e_sect == sect &&
	    (unsigned)o->o_e.e_val == here) {
		/* jmp $ */
		ob(ip->i_op);
		oshort(&o->o_e);
		return;
	}
	if (pass == 1) {
		jp = newjump();
		jp->j_kind = ip->i_class == C_JMP ? J_JMP : J_COND;
		if (sect != STEXT || nexsym != 1)
			jp->j_long = 2;
		else {
			jp->j_sym = exsym;
			jp->j_off = (int)o->o_e.e_val;
			if (o->o_e.e_sect != SUNDEF)
				jp->j_off -= exsym->s_value;
		}
	} else
		jp = nextjump();
	if (jp->j_long)
		longjump(ip, &o->o_e);
	else {
		ob(ip->i_op);
		oshort(&o->o_e);
	}
}

/*
 * The long form of a jump; for a conditional one, which has none, the
 * opposite condition's round a jump.
 */
longjump(ip, e)
register struct insn *ip;
struct expr *e;
{
	if (ip->i_class == C_JCC) {
		ob(ip->i_op ^ 1);
		ob(3);
	}
	ob(0xE9);
	orel(e);
}

/*
 * A far jump or call: the offset, and then the segment.
 */
farptr(opc, e)
register struct expr *e;
{
	ob(opc);
	if (e->e_sect == SABS || e->e_seg)
		error("far to what has no segment", "");
	oword(e);
	e->e_seg = 1;
	oword(e);
}

/*
 * The 287's.
 */
float87(ip)
register struct insn *ip;
{
	register struct opnd *a, *b;
	register n, i;

	n = ip->i_op;
	a = &op[0];
	b = &op[1];
	switch (ip->i_class) {

	case C_FARITH:
		if (nop == 0) {
			/* the same as its p form on st1 */
			if (n == 2 || n == 3) {
				ob(0xD8);
				ob(0xC1 | n << 3);
			} else {
				ob(0xDE);
				ob(0xC1 | (n >= 4 ? n ^ 1 : n) << 3);
			}
			return(1);
		}
		if (nop == 1 && a->o_kind == K_MEM) {
			if (a->o_size != 4 && a->o_size != 8)
				return(0);
			prefix(a);
			ob(a->o_size == 4 ? 0xD8 : 0xDC);
			modrm(n, a);
			return(1);
		}
		if (nop == 1 && a->o_kind == K_ST)
			i = a->o_reg;
		else if (nop == 2 && a->o_kind == K_ST && b->o_kind == K_ST) {
			if (a->o_reg == 0)
				i = b->o_reg;
			else if (b->o_reg == 0 && n != 2 && n != 3) {
				ob(0xDC);
				ob(0xC0 | (n >= 4 ? n ^ 1 : n) << 3 |
				    a->o_reg);
				return(1);
			} else
				return(0);
		} else
			return(0);
		ob(0xD8);
		ob(0xC0 | n << 3 | i);
		return(1);

	case C_FARITHP:
		i = 1;
		if (nop >= 1) {
			if (a->o_kind != K_ST || nop == 2 &&
			    (b->o_kind != K_ST || b->o_reg != 0) || nop > 2)
				return(0);
			i = a->o_reg;
		}
		ob(0xDE);
		ob(n | i);
		return(1);

	case C_FIARITH:
		if (nop != 1 || a->o_kind != K_MEM ||
		    a->o_size != 2 && a->o_size != 4)
			return(0);
		prefix(a);
		ob(a->o_size == 2 ? 0xDE : 0xDA);
		modrm(n, a);
		return(1);

	case C_FLD:
		if (nop != 1)
			return(0);
		if (a->o_kind == K_ST) {
			ob(n == 0 ? 0xD9 : 0xDD);
			ob((n == 0 ? 0xC0 : n == 2 ? 0xD0 : 0xD8) | a->o_reg);
			return(1);
		}
		if (a->o_kind != K_MEM)
			return(0);
		prefix(a);
		if (a->o_size == 4)
			ob(0xD9);
		else if (a->o_size == 8)
			ob(0xDD);
		else if (a->o_size == 10 && n != 2) {
			ob(0xDB);
			n = n == 0 ? 5 : 7;
		} else
			return(0);
		modrm(n, a);
		return(1);

	case C_FILD:
		if (nop != 1 || a->o_kind != K_MEM)
			return(0);
		prefix(a);
		if (a->o_size == 2)
			ob(0xDF);
		else if (a->o_size == 4)
			ob(0xDB);
		else if (a->o_size == 8 && n != 2) {
			ob(0xDF);
			n = n == 0 ? 5 : 7;
		} else
			return(0);
		modrm(n, a);
		return(1);

	case C_FSTI:
		i = 1;
		if (nop == 1 && a->o_kind == K_ST)
			i = a->o_reg;
		else if (nop != 0 || !ip->i_arg)
			return(0);
		ob(n >> 8);
		ob(n | i);
		return(1);

	case C_FMEM:
		if (nop != 1 || a->o_kind != K_MEM)
			return(0);
		if (ip->i_arg)
			ob(0x9B);
		prefix(a);
		ob(n >> 8);
		modrm(n & 7, a);
		return(1);

	case C_FSTSW:
		if (nop != 1)
			return(0);
		if (ip->i_arg)
			ob(0x9B);
		if (a->o_kind == K_REG && a->o_reg == 0) {
			ob(0xDF);
			ob(0xE0);
		} else if (a->o_kind == K_MEM) {
			prefix(a);
			ob(0xDD);
			modrm(7, a);
		} else
			return(0);
		return(1);
	}
	return(0);
}

/*
 * The directives.
 */
directive(ip)
register struct insn *ip;
{
	char name[NAMESZ];
	struct expr e;
	register struct sym *sp;
	register struct jump *jp;
	register c;
	int fill;
	long n;

	here = loc[sect];
	switch (ip->i_op) {

	case D_DB:
		for (;;) {
			skipb();
			c = *lp;
			if (ip->i_arg == 1 && (c == '\'' || c == '"')) {
				for (lp++; *lp && *lp != c && *lp != '\n'; )
					ob(*lp++);
				if (*lp == c)
					lp++;
				else
					error("a string has no quote after it",
					    "");
			} else {
				expr(&e);
				if (ip->i_arg == 1)
					obyte(&e);
				else if (ip->i_arg == 2)
					oword(&e);
				else {
					if (!isnum(&e))
						error("dd of an address", "");
					ow((int)e.e_val);
					ow((int)(e.e_val >> 16));
				}
			}
			skipb();
			if (*lp != ',')
				break;
			lp++;
		}
		return;

	case D_RES:
		expr(&e);
		if (!isnum(&e) || e.e_fwd || e.e_val < 0)
			error("room for how many?", "");
		else
			oskip(e.e_val * ip->i_arg);
		return;

	case D_ALIGN:
		expr(&e);
		n = e.e_val;
		if (!isnum(&e) || e.e_fwd || n <= 0 || n > 4096) {
			error("align to what?", "");
			return;
		}
		fill = sect == STEXT ? 0x90 : 0;
		skipb();
		if (*lp == ',') {
			lp++;
			skipb();
			getname(name);
			lower(name, name);
			expr(&e);
			if (strcmp(name, "db") == 0 && isnum(&e))
				fill = (int)e.e_val;
			else if (strcmp(name, "resb") != 0)
				error("align with what?", "");
		}
		if (sect == STEXT) {
			if (pass == 1) {
				jp = newjump();
				jp->j_kind = J_ALIGN;
				jp->j_off = (int)n;
			} else
				nextjump();
		}
		c = (int)n;
		while (loc[sect] % c)
			if (sect == SBSS)
				oskip(1L);
			else
				ob(fill);
		return;

	case D_GLOBAL:
	case D_EXTERN:
		for (;;) {
			skipb();
			getname(name);
			if (name[0] == 0) {
				error("no name where one should be: %s", lp);
				return;
			}
			sp = symbol(name);
			sp->s_flags |= ip->i_op == D_GLOBAL ? FGLOBAL :
			    FEXTERN;
			skipb();
			if (*lp != ',')
				break;
			lp++;
		}
		return;

	case D_COMMON:
		skipb();
		getname(name);
		expr(&e);
		if (name[0] == 0 || !isnum(&e) || e.e_fwd || e.e_val <= 0) {
			error("common what, of what size?", "");
			return;
		}
		sp = symbol(name);
		if (pass == 1) {
			if (sp->s_stmt) {
				error("%s is defined twice", name);
				return;
			}
			sp->s_flags |= FCOMMON;
			if ((unsigned)e.e_val > sp->s_value)
				sp->s_value = (unsigned)e.e_val;
		}
		skipb();
		if (*lp == ':') {
			lp++;
			getname(name);
		}
		return;

	case D_SEGMENT:
		segment();
		return;

	case D_IGNORE:
		while (!eol())
			lp++;
		return;
	}
}

/*
 * segment NAME, with what it is the first time: all that matters of
 * that is its class.  Without one the name has to say.
 */
segment()
{
	char name[NAMESZ], word[NAMESZ];
	register i, s;

	skipb();
	getname(name);
	if (name[0] == 0) {
		error("segment what?", "");
		return;
	}
	s = 0;
	while (!eol()) {
		if (!idstart(*lp)) {
			lp++;
			continue;
		}
		getname(word);
		lower(word, word);
		if (strcmp(word, "class") == 0 && *lp == '=') {
			lp++;
			getname(word);
			lower(word, word);
			s = strcmp(word, "code") == 0 ? STEXT :
			    strcmp(word, "bss") == 0 ? SBSS : SDATA;
		}
	}
	for (i = 0; i < nsegn; i++)
		if (strcmp(segname[i], name) == 0)
			break;
	if (i < nsegn) {
		if (s && s != segsect[i])
			error("segment %s was of another class", name);
		sect = segsect[i];
		return;
	}
	if (s == 0) {
		lower(word, name);
		for (i = 0; word[i]; i++) {
			if (strncmp(word + i, "text", 4) == 0 ||
			    strncmp(word + i, "code", 4) == 0)
				s = STEXT;
			if (strncmp(word + i, "bss", 3) == 0)
				s = SBSS;
		}
		if (s == 0)
			s = SDATA;
	}
	if (nsegn >= NSEGN)
		fatal("too many segments", "");
	segname[nsegn] = save(name);
	segsect[nsegn++] = s;
	sect = s;
}
