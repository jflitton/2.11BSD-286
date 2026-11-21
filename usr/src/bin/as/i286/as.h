/*
 * The assembler for the 80286.
 *
 * It reads what the C compiler's second pass writes (lib/ccom/i286)
 * and what the C library's own assembly is written in: the syntax of
 * NASM, as much of it as those use, so that either assembler does for
 * them.  It writes an object file (a.out.h), or with -b the bytes and
 * nothing else, as NASM's -f bin does: a boot block.
 *
 * There are two passes over the source.  The first finds how long
 * everything is, taking each jump to be a short one, and remembers the
 * jumps; between the two the jumps that do not reach are made long ones
 * and the labels moved to suit (relax); the second writes the object.
 */
#include <stdio.h>

#define	LINESZ	512		/* a line of the source, with its macros */
#define	NAMESZ	64		/* a name */
#define	NHASH	257
#define	NINCL	4		/* %includes, one in another */
#define	NSEGN	24		/* names of segments in one file */
#define	NNAMED	12		/* segments of the object's own beyond its
				   three */

/*
 * Where a symbol is, or a value: one of the object's segments, by its
 * number there, or nowhere.  The first three are the text, the data
 * and the bss; those after them are named, as a far variable's is
 * (a.out.h).
 */
#define	SUNDEF	0		/* not defined here */
#define	STEXT	1
#define	SDATA	2
#define	SBSS	3
#define	SNAMED	4		/* the first named one */
#define	NSECT	(SNAMED + NNAMED)
#define	SABS	31		/* a number */

struct sym {
	struct	sym *s_next;	/* the next with the same hash */
	struct	sym *s_link;	/* the text's next label, by address */
	char	*s_name;
	unsigned s_value;
	unsigned s_stmt;	/* the line that defined it; 0 for none yet */
	int	s_index;	/* its number in the object, or -1 */
	char	s_sect;
	char	s_flags;
};

#define	FGLOBAL	01		/* global */
#define	FEXTERN	02		/* extern */
#define	FCOMMON	04		/* common: s_value is its size */
#define	FUSED	010		/* something refers to it */
#define	FLOCAL	020		/* a label of the compiler's, or one with
				   a dot: not for the symbol table */

/*
 * A value.  It is a number, or a place in one of this file's segments
 * (e_sect and e_val), or a symbol from elsewhere and a number to add
 * (e_sym, with e_sect SUNDEF).
 */
struct expr {
	long	e_val;
	struct	sym *e_sym;
	char	e_sect;
	char	e_seg;		/* `seg' of it: its segment's selector */
	char	e_fwd;		/* it names what is defined further on,
				   so nothing is to be made of its size */
};

/*
 * An operand.
 */
struct opnd {
	char	o_kind;
	char	o_reg;		/* which register */
	char	o_size;		/* 1, 2, 4, 8 or 10 as written; else 0 */
	char	o_dist;		/* far, near or short, as written */
	char	o_rm;		/* K_MEM: its r/m, or -1 with no register */
	char	o_over;		/* K_MEM: a segment's prefix, or 0 */
	unsigned o_segv;	/* K_FAR: the segment */
	struct	expr o_e;	/* the displacement, or the value */
};

#define	K_NONE	0
#define	K_REG	1		/* ax cx dx bx sp bp si di */
#define	K_REG8	2		/* al cl dl bl ah ch dh bh */
#define	K_SREG	3		/* es cs ss ds */
#define	K_MEM	4
#define	K_IMM	5
#define	K_ST	6		/* st0 ... st7 */
#define	K_FAR	7		/* SEG:OFF, the segment a number */

#define	D_FAR	1
#define	D_NEAR	2
#define	D_SHORT	3

/*
 * An instruction or a directive: its class says what reads its
 * operands, and i_op and i_arg are for that.
 */
struct insn {
	char	*i_name;
	char	i_class;
	char	i_arg;
	int	i_op;
};

#define	C_DIR	0		/* a directive: i_op says which */
#define	C_TIMES	1		/* times N and then another */
#define	C_PREFIX 2		/* a byte, and an instruction after it */
#define	C_NONE	3		/* no operands: a byte */
#define	C_NONE2	4		/*	two, and with i_arg a wait first */
#define	C_ALU	5		/* add or adc sbb and sub xor cmp: i_op is
				   which, 0 to 7 */
#define	C_MOV	6
#define	C_TEST	7
#define	C_XCHG	8
#define	C_LXS	9		/* lea les lds bound: i_op is the opcode */
#define	C_PUSH	10
#define	C_POP	11
#define	C_INCDEC 12		/* i_op is 0 or 1 */
#define	C_UN	13		/* not neg mul imul div idiv: 2 to 7 */
#define	C_IMUL	14
#define	C_SHIFT	15		/* rol ror rcl rcr shl shr sar: 0 to 7 */
#define	C_JMP	16
#define	C_CALL	17
#define	C_JCC	18		/* i_op is the short jump's opcode */
#define	C_LOOP	19		/* loop jcxz: short only */
#define	C_RET	20		/* i_op is the opcode with no operand */
#define	C_INT	21
#define	C_IN	22
#define	C_OUT	23
#define	C_ENTER	24
#define	C_FARITH 25		/* fadd fmul fcom fcomp fsub ... : 0 to 7 */
#define	C_FARITHP 26		/* faddp ...: i_op is DE's second byte */
#define	C_FIARITH 27		/* fiadd ...: 0 to 7 */
#define	C_FLD	28		/* fld fst fstp: 0, 2, 3 */
#define	C_FILD	29		/* fild fist fistp: 0, 2, 3 */
#define	C_FSTI	30		/* fxch ffree: two bytes, and st's number */
#define	C_FMEM	31		/* fldcw ...: i_op is the opcode and /n */
#define	C_FSTSW	32
#define	C_PROT	33		/* lgdt ...: 0F, i_op's high byte, /n */
#define	C_LAR	34		/* lar lsl: 0F and i_op */
#define	C_ARPL	35

#define	D_DB	1		/* i_arg is the size of one */
#define	D_RES	2		/*	the same */
#define	D_ALIGN	3
#define	D_GLOBAL 4
#define	D_EXTERN 5
#define	D_COMMON 6
#define	D_SEGMENT 7
#define	D_IGNORE 8		/* bits, cpu */
#define	D_GROUP	9		/* its name: the selector of the data */
#define	D_ORG	10		/* where an image is to be (-b) */

/*
 * A jump whose length is not known until all of them are.  An `align'
 * in the text is here too, since what it puts in changes as they do,
 * and a `times' whose count is reckoned from $, which fills to a place.
 */
struct jump {
	unsigned j_loc;		/* where it is, all before it short */
	unsigned j_stmt;	/* the line it is on */
	int	j_adj;		/* how far what is before it has grown */
	struct	sym *j_sym;	/* where it goes */
	int	j_off;		/*	and this far past it */
	char	j_kind;
	char	j_long;
};

#define	J_JMP	0		/* 2 bytes, or 3 */
#define	J_COND	1		/* 2 bytes, or 5: a jump round a jump */
#define	J_ALIGN	2		/* j_off is the boundary */
#define	J_FILL	3

#define	JBLK	128
struct jblk {
	struct	jblk *jb_next;
	struct	jump jb_j[JBLK];
};

extern	int	pass;
extern	int	nerr;
extern	unsigned stmt;
extern	int	sect;
extern	unsigned loc[NSECT];
extern	int	nsect;
extern	char	*sectname[NSECT];
extern	char	*sectgrp[NSECT];
extern	char	*lp;
extern	struct	sym *hashtab[NHASH];
extern	struct	sym *lastlab;
extern	struct	sym *text0;
extern	struct	sym *exsym;
extern	int	nexsym;
extern	char	*outfile;
extern	struct	insn insns[];
extern	int	ninsns;
extern	struct	opnd op[3];
extern	int	nop;
extern	unsigned here;
extern	int	model;
extern	int	flat;
extern	unsigned origin;
extern	int	usedhere;

#define	P_ADD	8		/* how tightly + and - bind (binary) */

struct	sym *lookup(), *symbol();
struct	insn *findinsn();
struct	jump *newjump(), *nextjump();
char	*alloc(), *save();
long	number();
