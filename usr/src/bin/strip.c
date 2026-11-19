/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	defined(DOSCCS) && !defined(lint)
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)strip.c	5.1.2 (2.11BSD) 2025/12/25";
#endif

#include <a.out.h>
#include <signal.h>
#include <stdio.h>
#include <sys/file.h>

struct	xexec head;
int	status;

main(argc, argv)
	char *argv[];
{
	register i;

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	for (i = 1; i < argc; i++) {
		strip(argv[i]);
		if (status > 1)
			break;
	}
	exit(status);
}

strip(name)
	char *name;
{
	register f;
	long size;

	f = open(name, O_RDWR);
	if (f < 0) {
		fprintf(stderr, "strip: "); perror(name);
		status = 1;
		goto out;
	}
	if (read(f, (char *)&head, sizeof (head)) < 0 || N_BADMAG(head.e)) {
		printf("strip: %s not in a.out format\n", name);
		status = 1;
		goto out;
	}
#ifdef i286
	/*
	 * The file is to end with its last segment.  The symbols and then
	 * their names follow that, and the header has the size of each.
	 */
	if (head.e.a_syms == 0 && head.e.a_strs == 0)
		goto out;
	{
		register int i;

		size = N_TXTOFF(head.e);
		for (i = 0; i < head.e.a_nseg && i < NXSEG; i++)
			size += (head.x[i].xs_filesz + 1) & ~1L;
	}
	head.e.a_syms = 0;
	head.e.a_strs = 0;
#else
	if ((head.e.a_syms == 0) && ((head.e.a_flag & 1) != 0))
		goto out;

	size = N_DATOFF(head) + head.e.a_data;
	head.e.a_syms = 0;
	head.e.a_flag |= 1;
#endif
	if (ftruncate(f, size) < 0) {
		fprintf(stderr, "strip: "); perror(name);
		status = 1;
		goto out;
	}
	(void) lseek(f, (long)0, L_SET);
	(void) write(f, (char *)&head.e, sizeof (head.e));
out:
	close(f);
}
