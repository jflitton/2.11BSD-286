/*
 * execl(name, arg0, arg1, ..., 0): the arguments are already a list, on
 * the stack, as execve wants one.
 */
extern char **environ;

execl(name, arg0)
	char *name, *arg0;
{
	return (execve(name, &arg0, environ));
}
