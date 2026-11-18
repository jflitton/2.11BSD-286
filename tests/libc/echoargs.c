/*
 * For the C library's test: write the arguments, each followed by |, and
 * exit with their count.
 */
#include <stdio.h>

main(argc, argv)
	int argc;
	char **argv;
{
	int i;

	for (i = 0; i < argc; i++)
		printf("%s|", argv[i]);
	printf("\n");
	exit(argc);
}
