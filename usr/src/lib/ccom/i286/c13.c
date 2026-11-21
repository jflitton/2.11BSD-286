/*
 * C second pass for the 80286 -- tables
 *
 * The operators are the PDP-11 compiler's (../c13.c); the instructions
 * are not.
 */

#if	!defined(lint) && defined(DOSCCS)
static	char	sccsid[] = "@(#)c13.c	2.1 (2.11BSD GTE) 10/4/94";
#endif

#include "c1.h"
/*
 * Operator dope table-- see description in c0.
 */
int opdope[] = {
	000000,	/* EOFC (0) */
	000000,	/* ; */
	000000,	/* { */
	000000,	/* } */
	036000,	/* [ */
	002000,	/* ] */
	036000,	/* ( */
	002000,	/* ) */
	014201,	/* : */
	007001,	/* , */
	000000,	/* field selection (10) */
	000000,	/* reverse field selection */
	000001,	/* temporary field selection */
	000001,	/* int->ptr */
	000001,	/* ptr->int */
	000001,	/* long->ptr */
	000001,	/* field assignment */
	000001,	/* >> unsigned */
	000001,	/* >>= unsigned */
	000000,	/* keyword */
	000400,	/* name (20) */
	000400,	/* short constant */
	000400,	/* string */
	000400,	/* float */
	000400,	/* double */
	000400,	/* long const */
	000400,	/* long const <= 16 bits */
	000400,	/* autoi, *r++ */
	000400,	/* autod, *--r */
	000400,	/* () empty arglist */
	034213,	/* ++pre (30) */
	034213,	/* --pre */
	034213,	/* ++post */
	034213,	/* --post */
	034220,	/* !un */
	034202,	/* &un */
	034220,	/* *un */
	034200,	/* -un */
	034220,	/* ~un */
	036001,	/* . (structure reference) */
	030101,	/* + (40) */
	030001,	/* - */
	032101,	/* * */
	032001,	/* / */
	032001,	/* % */
	026061,	/* >> */
	026061,	/* << */
	020161,	/* & */
	016161,	/* | */
	016161,	/* ^ */
	036001,	/* -> (50) */
	001000,	/* int -> double */
	001000,	/* double -> int */
	000001,	/* && */
	000001,	/* || */
	030001, /* &~ */
	001000,	/* double -> long */
	001000,	/* long -> double */
	001000,	/* integer -> long */
	000000,	/* long -> integer */
	022005,	/* == (60) */
	022005,	/* != */
	024005,	/* <= */
	024005,	/* < */
	024005,	/* >= */
	024005,	/* > */
	024005,	/* <p */
	024005,	/* <=p */
	024005,	/* >p */
	024005,	/* >=p */
	012213,	/* += (70) */
	012213,	/* -= */
	012213,	/* *= */
	012213,	/* /= */
	012213,	/* %= */
	012253,	/* >>= */
	012253,	/* <<= */
	012253,	/* &= */
	012253,	/* |= */
	012253,	/* ^= */
	012213,	/* = (80) */
	030001, /* & for tests */
	032001,	/*  * (long) */
	032001,	/*  / (long) */
	032001,	/* % (long) */
	012253,	/* &= ~ */
	012213,	/* *= (long) */
	012213,	/* /= (long) */
	012213,	/* %= (long) */
	000000,	/* (89) */
	014201,	/* question '?' (90) */
	026061,	/* long << */
	012253,	/* long <<= */
	000101,	/* max */
	000101,	/* maxp */
	000101,	/* min */
	000101,	/* minp */
	000001,	/* , */
	000000,	/* call1 */
	000000,	/* call2 */
	036001,	/* call (100) */
	036000,	/* mcall */
	000000,	/* goto */
	000000,	/* jump cond */
	000000,	/* branch cond */
	000400,	/* set nregs */
	000000, /* load */
	030001,	/* ptoi1 */
	000000,	/* (108) */
	000000,	/* int->char */
	000000,	/* force r0 (110) */
	000000,	/* branch */
	000000,	/* label */
	000000,	/* nlabel */
	000000,	/* rlabel */
	000000,	/* structure assign */
	000001,	/* struct assignment setup */
	032001,	/* unsigned / */
	032001,	/* unsigned % */
	012213,	/* unsigned /= */
	012213,	/* unsigned %= (120) */
	032001, /* unsigned long * */
	032001, /* unsigned long / */
	032001, /* unsigned long % */
	012213, /* unsigned long *= */
	012213, /* unsigned long /= */
	012213, /* unsigned long %= */
	01000,  /* unsigned long -> float(double) */
	026061, /* unsigned long >> */
	012253, /* unsigned long >>= (129) */
	000000,	/* far * (130) */
	000001,	/* far + */
	000001,	/* far - */
	000013,	/* far += */
	000013,	/* far -= */
	000000,	/* near->far */
	000000,	/* near->far, of code (136) */
	000000,	/* a far variable's address made far (137) */
	000000,	/* a pointer into SS made far */
	000000,	/* a pointer into the based segment made far (139) */
};

char	*opntab[] = {
	0,			/* 0 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	":",
	",",
	"field select",		/* 10 */
	0,
	0,
	"int->ptr",
	"ptr->int",
	"long->ptr",
	"field assign",
	">>",
	">>=",
	"keyword",
	"name",			/* 20 */
	"short constant",
	"string",
	"float",
	"double",
	"long constant",
	"long constant",
	"*r++",
	"*--r",
	"()",
	"++pre",		/* 30 */
	"--pre",
	"++post",
	"--post",
	"!un",
	"&",
	"*",
	"-",
	"~",
	".",
	"+",			/* 40 */
	"-",
	"*",
	"/",
	"%",
	">>",
	"<<",
	"&",
	"|",
	"^",
	"->",			/* 50 */
	"int->double",
	"double->int",
	"&&",
	"||",
	"&~",
	"double->long",
	"long->double",
	"integer->long",
	"long->integer",
	"==",			/* 60 */
	"!=",
	"<=",
	"<",
	">=",
	">",
	"<p",
	"<=p",
	">p",
	">=p",
	"+=",			/* 70 */
	"-=",
	"*=",
	"/=",
	"%=",
	">>=",
	"<<=",
	"&=",
	"|=",
	"^=",
	"=",			/* 80 */
	"& for tests",
	"*",
	"/",
	"%",
	"&= ~",
	"*=",
	"/=",
	"%=",
	0,
	"?",			/* 90 */
	"<<",
	"<<=",
	"\\/",
	"\\/",
	"/\\",
	"/\\",
	",",
	"call1",
	"call2",
	"call",			/* 100 */
	"mcall",
	"goto",
	"jump cond",
	"branch cond",
	"set nregs",
	"load value",
	"ptr->integer",
	0,
	"int->char",
	"force register",	/* 110 */
	"branch",
	"label",
	"nlabel",
	"rlabel",
	"=structure",
	"= (struct setup)",
	"/",
	"%",
	"/=",
	"%=",			/* 120 */
	"*",			/* unsigned long */
	"/",			/* unsigned long */
	"%",			/* unsigned long */
	"*=",			/* unsigned long */
	"/=",			/* unsigned long */
	"%=",			/* unsigned long */
	"u_long->double", 	/* unsigned long */
	">>",			/* unsigned long */
	">>=",			/* 129 unsigned long */
	"far *",		/* 130 */
	"far +",
	"far -",
	"far +=",
	"far -=",
	"near->far",
	"near->far (code)",	/* 136 */
	"far variable",		/* 137 */
	"ss->far",
	"based->far",		/* 139 */
};

/*
 * The conditional jumps.  The first is for the test, the second for
 * the opposite one.  Those at 200 and up come after something other
 * than a comparison has set the flags as a value has them, where only
 * the Z and S flags are to be trusted (see branch in c11.c, which does
 * the signed <= and > itself), and where nothing is below zero
 * unsigned.
 */
char	je[]	= "je";
char	jne[]	= "jne";
char	jle[]	= "jle";
char	jg[]	= "jg";
char	jl[]	= "jl";
char	jge[]	= "jge";
char	jbe[]	= "jbe";
char	ja[]	= "ja";
char	jb[]	= "jb";
char	jae[]	= "jae";
char	js[]	= "js";
char	jns[]	= "jns";
char	jmp[]	= "jmp";
char	nop[]	= "";

struct instab branchtab[] = {
	EQUAL,	je,	jne,
	NEQUAL,	jne,	je,
	LESSEQ,	jle,	jg,
	LESS,	jl,	jge,
	GREATEQ,jge,	jl,
	GREAT,	jg,	jle,
	LESSEQP,jbe,	ja,
	LESSP,	jb,	jae,
	GREATQP,jae,	jb,
	GREATP,	ja,	jbe,
	200+EQUAL,	je,	jne,
	200+NEQUAL,	jne,	je,
	200+LESSEQ,	jle,	jg,
	200+LESS,	js,	jns,
	200+GREATEQ,	jns,	js,
	200+GREAT,	jg,	jle,
	200+LESSEQP,	je,	jne,
	200+LESSP,	nop,	jmp,
	200+GREATQP,	jmp,	nop,
	200+GREATP,	jne,	je,
	0,	0,	0 };
