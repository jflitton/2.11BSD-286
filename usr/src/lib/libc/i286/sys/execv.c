/*
 * execv(name, argv): execve with the program's own environment.
 */
extern char **environ;

execv(name, argv)
	char *name, **argv;
{
	return (execve(name, argv, environ));
}
