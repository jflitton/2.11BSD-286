/*
 * /etc/init for the compiler's test on the system (tests/ccom/run.sh):
 * it runs each of the programs named below, which are prog.c compiled
 * for the two models, and says how many of them failed.  The line
 * "@exit NN" at the end is for tests/harness/runtest.py.
 *
 * The last is the object code improver, c2, as the compiler compiled
 * it and as it then improved itself.  It is given the assembly that
 * the compiler wrote for prog.c, and what it makes of that here is to
 * be what it made of it where the system was built, to the byte.
 */
#include <stdio.h>
#include <sys/reboot.h>

char	*progs[] = { "/bin/small", "/bin/medium", "/bin/c2", 0 };
char	c2in[] = "/etc/prog.s";
char	c2out[] = "/tmp/prog.s";
char	c2want[] = "/etc/prog.want";

run(name)
char *name;
{
	int pid, status;

	pid = fork();
	if (pid == 0) {
		if (name == progs[2])
			execl(name, name + 5, c2in, c2out, (char *)0);
		else
			execl(name, name + 5, (char *)0);
		_exit(126);
	}
	if (pid < 0)
		return(-1);
	status = -1;
	while (wait(&status) != pid)
		;
	if (status == 0 && name == progs[2])
		status = differ(c2out, c2want);
	return(status);
}

/*
 * Are two files not the same?  And how long the first is, for what is
 * printed to say that something was compared.
 */
long	c2len;

differ(a, b)
char *a, *b;
{
	static char abuf[512], bbuf[512];
	register int fa, fb, n, i;

	fa = open(a, 0);
	fb = open(b, 0);
	if (fa < 0 || fb < 0)
		return(1);
	c2len = 0;
	for (;;) {
		n = read(fa, abuf, sizeof abuf);
		if (n < 0 || read(fb, bbuf, sizeof bbuf) != n)
			return(1);
		if (n == 0)
			break;
		for (i = 0; i < n; i++)
			if (abuf[i] != bbuf[i])
				return(1);
		c2len += n;
	}
	close(fa);
	close(fb);
	return(c2len == 0);
}

main()
{
	register char **p;
	int status[4], failed, n;

	/* As init nothing is open yet: the console becomes 0, 1 and 2. */
	open("/dev/console", 2);
	dup(0);
	dup(0);
	failed = 0;
	for (p = progs, n = 0; *p; p++, n++)
		if (status[n] = run(*p))
			failed++;
	for (p = progs, n = 0; *p; p++, n++)
		if (status[n])
			printf("ccom: %s: status %04x\n", *p, status[n]);
	if (status[n-1] == 0)
		printf("ccom: c2: %ld bytes, as it makes them anywhere\n",
		    c2len);
	printf("ccom: %d programs, %d failed\n", n, failed);
	printf("@exit %02x\n", failed);
	fflush(stdout);
	close(0);			/* the last close of the console */
	close(1);			/*   waits for what was written */
	close(2);			/*   to it to go out */
	sync();
	reboot(RB_HALT);
	for (;;)
		;
}
