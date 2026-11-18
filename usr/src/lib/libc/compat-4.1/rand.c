/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)rand.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

static	long	randx = 1;

srand(x)
unsigned x;
{
	randx = x;
}

/*
 * With a 16-bit int the result is the generator's upper bits, which are
 * its best, up to RAND_MAX (0x7fff); the other way returns its low 16
 * bits, the poorest, and half the time a negative number.
 */
rand()
{
#if defined(pdp11) || defined(i286)
	return(((randx = randx * 1103515245 + 12345)>>16) & 0x7fff);
#else
	return((randx = randx * 1103515245 + 12345) & 0x7fffffff);
#endif
}
