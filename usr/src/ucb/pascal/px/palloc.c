#include "0x.h"
#include "E.h"

extern char *sbrk(), *brk();

alloc(need)
	char *need;
{
	register cnt, *wp;
	register char *have;

	need = (char *)((unsigned)(need+1) &~ 1);
	if ((have=high-memptr) < need) {
		if (sbrk(need > have + 1024 ? need-have:1024) == -1)
			error(EOUTOFMEM);
		high = sbrk(0);
	}
	wp = memptr;
	cnt = ((unsigned)need >> 1) & 077777;
	do {
		*wp++ = 0;
	} while (--cnt);
	wp = memptr;
	memptr += (unsigned)need;
	stklim();
	return(wp);
}

setmem()
{
	high = bottmem = memptr = sbrk(0);
	stklim();
}

#ifdef i286
/*
 * pfree, where the C library's free is, with the malloc of its stdio,
 * which ctime brings (pdattim.c)
 */
pfree(cptr)
#else
free(cptr)
#endif
	char *cptr;
{
}

stklim()
{
#ifdef i286
	extern char end[];

	/*
	 * The 286's stack is not above what is allocated, as the PDP-11's
	 * is, but below it, from the break the program was loaded with
	 * down to end; 1024 bytes of it are kept for the C px calls.
	 */
	maxstk = end + 1024;
#else
	maxstk = (char *)((((unsigned)memptr + 07777) &~ 07777) + 512);
#endif
}
