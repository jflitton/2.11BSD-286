/*
 * execle(name, arg0, arg1, ..., 0, envp): the arguments are already a
 * list, on the stack, as execve wants one, and the environment follows
 * its null.
 */
execle(name, arg0)
	char *name, *arg0;
{
	register char **ap = &arg0;

	while (*ap++)
		;
	return (execve(name, &arg0, *(char ***)ap));
}
