/*
 * A program for init to execute: it checks the arguments and environment
 * it was given, and says so by its exit status.
 */
#include "lib.h"

main(argc, argv, envp)
	int argc;
	char **argv, **envp;
{
	if (argc != 3 || strcmp(argv[0], "child") || strcmp(argv[1], "one") ||
	    strcmp(argv[2], "two words") || argv[3] != 0)
		return (1);
	if (strcmp(envp[0], "HOME=/") || strcmp(envp[1], "TERM=console") ||
	    envp[2] != 0 || environ != envp)
		return (2);
	if (getppid() != 1 || getpid() == 1)
		return (3);
	return (7);
}
