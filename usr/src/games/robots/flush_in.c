/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
static char sccsid[] = "@(#)flush_in.c     5.2 (2.11BSD) 2026/5/12";
#endif

# include	<curses.h>
# include	<sys/file.h>

/*
 * flush_in:
 *	Flush all pending input.
 */
flush_in()
{
# ifdef TIOCFLUSH
	static int	flags = FREAD;	/* the input: 0 is the output too */

	ioctl(fileno(stdin), TIOCFLUSH, &flags);
# else TIOCFLUSH
	crmode();
# endif TIOCFLUSH
}
