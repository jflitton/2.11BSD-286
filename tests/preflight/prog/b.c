/* The other file of tools/preflight.py's test. */
sum(a, b)
	int a, b;
{
	return a + b;
}

seek(fd, where)
	int fd;
	long where;
{
	return fd + (int)where;
}

handle(f)
	int (*f)();
{
	return f != 0;
}

say(s)
	char *s;
{
	return s != 0;
}

long
big(n)
	int n;
{
	return n * 100000L;
}

char *
name()
{
	return "b";
}

double
half(d)
	double d;
{
	char buf[2000];		/* a large frame */

	buf[0] = 0;
	say(buf);
	return d / 2 + buf[0];
}
