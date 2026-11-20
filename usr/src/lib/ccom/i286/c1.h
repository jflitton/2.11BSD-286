/*
 * C code generator header, for the 80286
 *
 * The trees, the operators and the types are the PDP-11 compiler's
 * (../c1.h), and are what the first pass writes.
 */

#include <stdio.h>
#include <setjmp.h>

/*
 * A long constant is 32 bits.  Where the compiler itself is built with
 * a long of 64, as it may be when it is tried on another machine, it is
 * built with LTYPE the type that has 32 (Makefile).
 */
#ifndef	LTYPE
#define	LTYPE	long
#endif
#define	NULL	0
#define	TNULL	(union tree *)NULL
#define	UNS(x)	((unsigned short)(x))
#define	W(x)	((int)(short)(x))	/* as the 286's int */

/*
 *  Tree node for unary and binary
 */
struct	tnode {
	int	op;
	int	type;
	int	degree;
	union	tree *tr1;
	union	tree *tr2;
};

/*
 * tree names for locals
 */
struct	tname {
	int	op;
	int	type;
	char	class;
	char	regno;
	int	offset;
	int	nloc;
};

/*
 * tree names for externals
 */
struct	xtname {
	int	op;
	int	type;
	char	class;
	char	regno;
	int	offset;
	char	*name;
};

/*
 * short constants
 */
struct	tconst {
	int	op;
	int	type;
	int	value;
};

/*
 * long constants
 */
struct	lconst {
	int	op;
	int	type;
	LTYPE	lvalue;
};

/*
 * Floating constants
 */
struct	ftconst {
	int	op;
	int	type;
	int	value;
	double	fvalue;
};

/*
 * Node used for field assignments
 */
struct	fasgn {
	int	op;
	int	type;
	int	degree;
	union	tree *tr1;
	union	tree *tr2;
	int	mask;
};

union	tree {
	struct	tnode t;
	struct tname n;
	struct	xtname x;
	struct	tconst c;
	struct	lconst l;
	struct	ftconst f;
	struct	fasgn F;
};

struct	instab {
	int	iop;
	char	*str1;
	char	*str2;
};

struct	swtab {
	int	swlab;
	int	swval;
};

/*
 * The 286's registers.  3, 4 and 5 are the numbers the first pass
 * gives the register variables and the frame pointer.
 */
#define	AX	0
#define	BX	1
#define	CX	2
#define	DI	3
#define	SI	4
#define	BP	5
#define	DX	6

/*
 * What an expression is compiled for (rcexpr): its effect alone, its
 * value in AX or DX:AX, the flags, or its value on the stack.
 */
#define	FOREFF	0
#define	FORREG	1
#define	FORCC	2
#define	FORSP	3

/*
 * What gexpr leaves: the value in AX or DX:AX in each case but the
 * last, and in the second and third the flags Z and S as the value has
 * them, with the overflow flag clear in the third.
 */
#define	INREG	0
#define	CCARITH	1
#define	CCLOGIC	2
#define	DONE	3	/* nothing: the context was seen to */

/*
 * Flags for pname.
 */
#define	HI	01	/* the high word of a long */
#define	BYT	02	/* as a byte */
#define	FLT	04	/* as the 287 takes it: of the size its type has */
#define	FARP	010	/* as the far pointer it holds, for a call */

#define	LNG(t)	((t)==LONG || (t)==UNLONG)
/* is the tree reached through a pointer? */
#define	IND(p)	((p)->t.op==STAR || (p)->t.op==FSTAR \
		|| (p)->t.op==AUTOI || (p)->t.op==AUTOD)
#define	CHR(t)	((t)==CHAR || (t)==UNCHAR)

char	maprel[];
char	notrel[];
int	isn;
int	line;
int	nerror;			/* number of hard errors */
struct	instab	branchtab[];
int	opdope[];
char	*opntab[];
char	*rname[];
char	*bname[];
int	ccof;			/* the overflow flag may be set */
int	mflag;			/* the medium model: functions are far */
char	*textseg;		/* the name of the segment the code is in */
int	cursect;		/* PROG, DATA or BSS: the segment being filled */
char	*funcbase;
char	*curbase;
char	*coremax;
struct	tconst czero, cone;
long	totspace;
long	ftell();
char	*sbrk();
union	tree *optim();
union	tree *unoptim();
union	tree *pow2();
union	tree *tnode();
union	tree *sdelay();
union	tree *ncopy();
union	tree *getblk();
union	tree *strfunc();
union	tree *isconstant();
union	tree *tconst();
union	tree *hardlongs();
union	tree *lconst();
union	tree *acommute();
union	tree *lvfield();
union	tree *paint();
union	tree *operands();
union	tree *loperands();
union	tree *bxform();
union	tree *faradr();
union	tree *adrof();
union	tree *fmem();
char	*fname();
char	*opname();
char	*opname2();

/*
	operators
*/
#define	EOFC	0
#define	SEMI	1
#define	LBRACE	2
#define	RBRACE	3
#define	LBRACK	4
#define	RBRACK	5
#define	LPARN	6
#define	RPARN	7
#define	COLON	8
#define	COMMA	9
#define	FSEL	10
#define	FSELR	11
#define	FSELT	12
#define	FSELA	16
#define	ULSH	17
#define	ASULSH	18

#define	KEYW	19
#define	NAME	20
#define	CON	21
#define	STRING	22
#define	FCON	23
#define	SFCON	24
#define	LCON	25
#define	SLCON	26

#define	AUTOI	27
#define	AUTOD	28
#define	NULLOP	218
#define	INCBEF	30
#define	DECBEF	31
#define	INCAFT	32
#define	DECAFT	33
#define	EXCLA	34
#define	AMPER	35
#define	STAR	36
#define	NEG	37
#define	COMPL	38

#define	DOT	39
#define	PLUS	40
#define	MINUS	41
#define	TIMES	42
#define	DIVIDE	43
#define	MOD	44
#define	RSHIFT	45
#define	LSHIFT	46
#define	AND	47
#define	ANDN	55
#define	OR	48
#define	EXOR	49
#define	ARROW	50
#define	ITOF	51
#define	FTOI	52
#define	LOGAND	53
#define	LOGOR	54
#define	FTOL	56
#define	LTOF	57
#define	ITOL	58
#define	LTOI	59
#define	ITOP	13
#define	PTOI	14
#define	LTOP	15

#define	EQUAL	60
#define	NEQUAL	61
#define	LESSEQ	62
#define	LESS	63
#define	GREATEQ	64
#define	GREAT	65
#define	LESSEQP	66
#define	LESSP	67
#define	GREATQP	68
#define	GREATP	69

#define	ASPLUS	70
#define	ASMINUS	71
#define	ASTIMES	72
#define	ASDIV	73
#define	ASMOD	74
#define	ASRSH	75
#define	ASLSH	76
#define	ASAND	77
#define	ASOR	78
#define	ASXOR	79
#define	ASSIGN	80
#define	TAND	81
#define	LTIMES	82
#define	LDIV	83
#define	LMOD	84
#define	ASANDN	85
#define	LASTIMES 86
#define	LASDIV	87
#define	LASMOD	88

#define	QUEST	90
/* #define	MAX	93	/* not used; wanted macros in param.h */
#define	MAXP	94
/* #define	MIN	95	/* not used; wanted macros in param.h */
#define	MINP	96
#define	LLSHIFT	91
#define	ASLSHL	92
#define	SEQNC	97
#define	CALL1	98
#define	CALL2	99
#define	CALL	100
#define	MCALL	101
#define	JUMP	102
#define	CBRANCH	103
#define	INIT	104
#define	SETREG	105
#define	LOAD	106
#define	PTOI1	107
#define	ITOC	109
#define	RFORCE	110

/*
 * Intermediate code operators
 */
#define	BRANCH	111
#define	LABEL	112
#define	NLABEL	113
#define	RLABEL	114
#define	STRASG	115
#define	STRSET	116
#define	UDIV	117
#define	UMOD	118
#define	ASUDIV	119
#define	ASUMOD	120
#define	ULTIMES	121	/* present for symmetry */
#define	ULDIV	122
#define	ULMOD	123
#define	ULASTIMES 124	/* present for symmetry */
#define	ULASDIV	125
#define	ULASMOD	126
#define	ULTOF	127
#define	ULLSHIFT 128	/* << for unsigned long */
#define	UASLSHL	129	/* <<= for unsigned long */

/*
 * Far pointers.  One is an unsigned long in every tree: its offset the
 * low word, and its segment the high.  These are what is done with one
 * that is not done with a long.
 */
#define	FSTAR	130	/* what a far pointer points at */
#define	FPLUS	131	/* a far pointer and an int: the offset's sum */
#define	FMINUS	132
#define	ASFPLUS	133
#define	ASFMINUS 134
#define	NTOF	135	/* a near pointer made far: the data segment's */
#define	CTOF	136	/* a near pointer to a function made far */
#define	MAXOP	136

#define	BDATA	200
#define	PROG	202
#define	DATA	203
#define	BSS	204
#define	CSPACE	205
#define	SSPACE	206
#define	SYMDEF	207
#define	SAVE	208
#define	RETRN	209
#define	EVEN	210
#define	PROFIL	212
#define	SWIT	213
#define	EXPR	214
#define	SNAME	215
#define	RNAME	216
#define	ANAME	217
#define	SETSTK	219
#define	SINIT	220
#define	GLOBAL	221
#define	C3BRANCH	222
#define	ASSEM	223

/*
 *	types
 */
#define	INT	0
#define	CHAR	1
#define	FLOAT	2
#define	DOUBLE	3
#define	STRUCT	4
#define	RSTRUCT	5
#define	LONG	6
#define	UNSIGN	7
#define	UNCHAR	8
#define	UNLONG	9
#define	VOID	10

/*
 * Three bits for each of what a type is made with, as the first pass
 * has them here (../c0.h).  Its fourth, the far pointer, it does not
 * pass on.
 */
#define	TYLEN	3
#define	TYPE	017
#define	XTYPE	(07<<4)
#define	PTR	020
#define	FUNC	040
#define	ARRAY	060

/*
	storage	classes
*/
#define	KEYWC	1
#define	MOS	10
#define	AUTO	11
#define	EXTERN	12
#define	STATIC	13
#define	REG	14
#define	STRTAG	15
#define	ARG	16
#define	OFFS	20
#define	XOFFS	21
#define	SOFFS	22
#define	FOFFS	23	/* through a far pointer: [es:bx+n] */

/*
	Flag	bits
*/

#define	BINARY	01
#define	LVALUE	02
#define	RELAT	04
#define	ASSGOP	010
#define	LWORD	020
#define	RWORD	040
#define	COMMUTE	0100
#define	RASSOC	0200
#define	LEAF	0400
#define	CNVRT	01000
