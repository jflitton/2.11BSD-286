/*
 * For the first pass run on the machine the system is built on, where
 * it is not the compiler but something to try the second pass with
 * (Makefile).  Its outcode, in ../c04.c, steps through its arguments
 * in memory, as the PDP-11 and the 286 have them; a machine that
 * passes them in registers needs this one instead.  There are never
 * more than seven after the string.  And its error and werror, in
 * ../c01.c, take whatever they are given as ints, which a pointer here
 * is not.
 */
#include "c0.h"

extern int Wflag;

werror(s, p1, p2, p3, p4, p5, p6)
char *s;
long p1, p2, p3, p4, p5, p6;
{
	if (Wflag)
		return;
	if (filename[0])
		fprintf(stderr, "%s:", filename);
	fprintf(stderr, "%d: warning: ", line);
	fprintf(stderr, s, p1, p2, p3, p4, p5, p6);
	fprintf(stderr, "\n");
}

error(s, p1, p2, p3, p4, p5, p6)
char *s;
long p1, p2, p3, p4, p5, p6;
{
	nerror++;
	if (filename[0])
		fprintf(stderr, "%s:", filename);
	fprintf(stderr, "%d: ", line);
	fprintf(stderr, s, p1, p2, p3, p4, p5, p6);
	fprintf(stderr, "\n");
}

outcode(s, a1, a2, a3, a4, a5, a6, a7)
char *s;
long a1, a2, a3, a4, a5, a6, a7;
{
	long av[7], *ap;
	FILE *bufp;
	char *np;
	int n;

	bufp = stdout;
	if (strflg)
		bufp = sbufp;
	av[0] = a1; av[1] = a2; av[2] = a3; av[3] = a4;
	av[4] = a5; av[5] = a6; av[6] = a7;
	ap = av;
	for (;;) switch(*s++) {
	case 'B':
		fputc((int)*ap++, bufp);
		fputc(0376, bufp);
		continue;

	case 'N':
		fputc((int)*ap, bufp);
		fputc((int)*ap++>>8, bufp);
		continue;

	case 'F':
		np = (char *)*ap++;
		n = 1000;
		goto str;

	case 'S':
		np = (char *)*ap++;
		n = MAXCPS-1;
		if (*np)
			fputc('_', bufp);
	str:
		while(n-- && *np) {
			fputc(*np++ & 0177, bufp);
		}
		fputc(0, bufp);
		continue;

	case '1':
		fputc(1, bufp);
		fputc(0, bufp);
		continue;

	case '0':
		fputc(0, bufp);
		fputc(0, bufp);
		continue;

	case '\0':
		if (ferror(bufp)) {
			error("Write error on temp");
			exit(1);
		}
		return;

	default:
		error("Botch in outcode");
	}
}
