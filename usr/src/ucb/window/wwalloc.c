#if	!defined(lint) && defined(DOSCCS)
static char sccsid[] = "@(#)wwalloc.c	3.8 (2.11BSD) 2026/5/9";
#endif

/*
 * Copyright (c) 1983 Regents of the University of California,
 * All rights reserved.  Redistribution permitted subject to
 * the terms of the Berkeley Software License Agreement.
 */

#include "ww.h"

char **
wwalloc(row, col, nrow, ncol, size)
{
	register char *p, **pp;
	register int i;
	long n;

	/*
	 * fast, call malloc only once: for what a block can be, and not
	 * what is left of the size in 16 bits
	 */
	n = (long)sizeof (char **) * nrow + (long)size * nrow * ncol;
	pp = n > 65000L ? 0 : (char **)malloc((unsigned)n);
	if (pp == 0) {
		wwerrno = WWE_NOMEM;
		return 0;
	}
	p = (char *)&pp[nrow];
	col *= size;
	size /= sizeof (char);		/* paranoid */
	size *= ncol;
	for (i = 0; i < nrow; i++) {
		pp[i] = p - col;
		p += size;
	}
	return pp - row;
}

wwfree(p, row)
register char **p;
{
	free((char *)(p + row));
}
