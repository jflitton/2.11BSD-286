/*
 * Header for object code improver, for the 80286
 *
 *	What is read is the assembly that the compiler's second pass
 *	writes (../../ccom/i286), which is NASM's:
 *
 *		name:
 *			mov	ax,word [bp-4]
 *
 *	A label begins its line, and anything else comes after a tab.
 */

#include <stdio.h>

#define	MAXCPS	40

#ifndef	CHECK
#define	CHECK(x)
#endif

/*
 * What equop will not take for the same in two places is below MOV.
 * 0 is a line that is not known here: an instruction this has no
 * table for, or a directive.  Nothing is moved over one or taken out.
 */
#define	JBR	1
#define	CBR	2
#define	JMP	3
#define	LABEL	4
#define	DLABEL	5
#define	EROU	7		/* ret, retf */
#define	JSW	9
#define	MOV	10
#define	LEA	11
#define	PUSH	12
#define	POP	13
#define	CMP	14
#define	TEST	15
#define	ADD	16
#define	SUB	17
#define	AND	18
#define	OR	19
#define	XOR	20
#define	ADC	21
#define	SBB	22
#define	INC	23
#define	DEC	24
#define	NEG	25
#define	NOT	26
#define	SHIFT	27		/* shl, shr, sar, rcl, rcr */
#define	XCHG	28
#define	CBW	29
#define	CWD	30
#define	MUL	31		/* imul, mul, idiv, div */
#define	JSR	32
#define	LEAVE	33
#define	END	34
#define	LES	35		/* ES and a register from a far pointer */

#define	JEQ	0
#define	JNE	1
#define	JLE	2
#define	JGE	3
#define	JLT	4
#define	JGT	5
#define	JLO	6
#define	JHI	7
#define	JLOS	8
#define	JHIS	9
#define	JPL	10
#define	JMI	11

#define	LSIZE	512

struct node {
	char	op;
	char	subop;
	struct	node	*forw;
	struct	node	*back;
	struct	node	*ref;
	int	labno;
	char	*code;
	int	refc;
	int	mark;
};

struct optab {
	char	*opstring;
	int	opcode;
};
extern	struct optab optab[];

char	line[LSIZE];
struct	node	first;
char	*curlp;
int	nbrbr;
int	nsaddr;
int	redunm;
int	iaftbr;
int	njp1;
int	nrlab;
int	nxjump;
int	ncmot;
int	nrevbr;
int	loopiv;
int	nredunj;
int	nskip;
int	ncomj;
int	nfwd;
int	ndead;
int	nbool;

int	nchange;
int	isn;
int	debug;
extern	char revbr[];

/*
 * What each register is known to hold: an operand as the compiler
 * writes it, "word [bp-4]" or "12" or "si", or "&[bp-4]" for an
 * address that lea made; SS, which is the same throughout, as "ss".
 * And in regs[ESBX], the far pointer that les gave ES and BX, "far
 * [bp+6]", while they have it still.
 */
#define	AX	0
#define	BX	1
#define	CX	2
#define	DX	3
#define	SI	4
#define	DI	5
#define	NREG	6
#define	ESBX	6
char	regs[NREG + 1][MAXCPS + 1];
char	rt1[LSIZE];
char	rt2[LSIZE];
extern	char *rname[];

#define	LABHS	127
#define	OPHS	101

struct optab *ophash[OPHS];
struct	node *nonlab();
char	*copy();
struct	node *insertl();
struct	node *codemove();
char	*alloc();
char	*index();
