/*
 * For tests/pm: a function that the linker is told to put in a code
 * segment of its own, so that calling it is a far call that changes CS.
 */
int	farseen;

farcs()
{
	farseen = getcs();	/* not the last thing, or it becomes a jump */
	return (farseen);
}

long
farmul(a, b)
	long a, b;
{
	return (a * b);
}
