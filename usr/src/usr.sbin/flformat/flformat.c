/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)flformat.c	1.1 (2.11BSD) 2026/11/25";
#endif

#include <stdio.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <i286at/flreg.h>

char *devname;

/*
 * Format a PC's diskettes: as the drive's largest, or with -f, as the
 * size of diskette given in kilobytes.
 */

main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	int fd, kb = 0;
	int i, c;

	while ((c = getopt(argc, argv, "f:")) != EOF)
		switch (c) {
		case 'f':
			kb = atoi(optarg);
			if (kb != 720 && kb != 1200 && kb != 1440)
				usage();
			break;
		default:
			usage();
		}
	if (argc - optind != 1)
		usage();
	devname = argv[optind];
	if ((fd = open(devname, O_RDWR)) < 0) {
		perror(devname);
		exit(1);
	}
	if (isatty(fileno(stdin))) {
		printf("Format %s (y/n)? ", devname);
		i = c = getchar();
		while (c != '\n' && c != EOF)
			c = getchar();
		if (i != 'y')
			exit(0);
	} else
		printf("Formatting %s\n", devname);
	if (ioctl(fd, FLIOC_FORMAT, &kb) == 0)
		exit(0);
	else {
		perror(devname);
		exit(1);
	}
}

usage()
{
	fprintf(stderr, "usage: flformat [-f 720|1200|1440] /dev/rfl?\n");
	exit(1);
}
