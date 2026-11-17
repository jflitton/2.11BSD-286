/*	ctype.h	4.3 (2.11BSD) 2026/1/26 */

#ifndef _CTYPE_H_
#define _CTYPE_H_

/*
 * Prototypes.  Must be located before the macros that defines the same 
 * functionality.  Specified in ISO C.
 */
int isalnum(int c);
int isalpha(int c);
int isblank(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
int tolower(int c);
int toupper(int c);

#define	_U	01	/* A-Z */
#define	_L	02	/* a-z */
#define	_N	04	/* 0-9 */
#define	_S	010	/* ' '\f\n\r\t\v */
#define _P	020	/* !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~ */
#define _C	040	/* \0-\037\177 */
#define _X	0100	/* A-Fa-f */
#define	_B	0200	/* ' ' */

extern	char	_ctype_[];

/* Macros to optimize some of the functions above */
#define isalnum(c)	((_ctype_+1)[c]&(_U|_L|_N))
#define	isalpha(c)	((_ctype_+1)[c]&(_U|_L))
#define iscntrl(c)	((_ctype_+1)[c]&_C)
#define	isdigit(c)	((_ctype_+1)[c]&_N)
#define isgraph(c)	((_ctype_+1)[c]&(_P|_U|_L|_N))
#define	islower(c)	((_ctype_+1)[c]&_L)
#define isprint(c)	((_ctype_+1)[c]&(_P|_U|_L|_N|_B))
#define ispunct(c)	((_ctype_+1)[c]&_P)
#define	isspace(c)	((_ctype_+1)[c]&_S)
#define	isupper(c)	((_ctype_+1)[c]&_U)
#define	isxdigit(c)	((_ctype_+1)[c]&(_N|_X))

/* Historical macros */
#define toascii(c)	((c)&0177)
#define isascii(c)	((unsigned)(c)<=0177)

#endif /* !_CTYPE_H_ */
