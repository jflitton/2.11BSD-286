/*
 * For tools/preflight.py's test: what it should find between two files,
 * and between a file and the library.
 */
#include <stdio.h>

char	*name();
double	half();

main()
{
	long l;
	int i;

	i = sum(1, 2);			/* right */
	i = sum(1);			/* count */
	seek(3, 0);			/* wrong size: an int for a long */
	seek(3, 0L);			/* right */
	handle(0);			/* medium: an int for a function */
	say(0);				/* same size: an int for a pointer */
	l = big(1);			/* wrong size: not declared, a long */
	big(1);				/* its result unused: nothing */
	printf("%s %g\n", name(), half(2.0));
	lseek(0, 0, 0);			/* the library: an int for a long */
	l = atol("1");			/* the library: not declared, a long */
	exit(i + (int)l);
}
