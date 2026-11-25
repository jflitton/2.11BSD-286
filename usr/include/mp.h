#define	MINT	struct mint
MINT
{	int	len;
	short	*val;
};
#define	FREE(x)	{if(x.len!=0) {free((char *)x.val); x.len=0;}}
#ifndef	DBG
#define	shfree(u)	free((char *)u)
#else
#include "stdio.h"
#define	shfree(u)	{ if(dbg) fprintf(stderr, "free %o\n", u); free((char *)u);}
extern	int	dbg;
#endif
/* The halves of a long: the 286's has its low word first, the PDP-11's its high. */
struct	half
#ifdef i286
{	short	low;
	short	high;
};
#else
{	short	high;
	short	low;
};
#endif
extern	MINT	*itom();
extern	short	*xalloc();

#ifdef	lint
extern	xv_oid;
#define	VOID	xv_oid =
#else
#define	VOID
#endif
