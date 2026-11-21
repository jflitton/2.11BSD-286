/*
 * /etc/init for the compiler's test on the system (tests/ccom/run.sh):
 * it runs each of the programs named below and says how many of them
 * failed.  The first three are prog.c compiled for the models, and the
 * fourth the tests of t_*.c, which run.sh runs in real mode in the
 * small model, compiled for the large.  The
 * line "@exit NN" at the end is for tests/harness/runtest.py.
 *
 * The rest are the compiler as it compiled itself: its two passes, and
 * the object code improver, which improved all three.  The passes are
 * given prog.c as the preprocessor left it, and the improver the
 * assembly they wrote for it where the system was built; what each
 * makes here is to be what was made there, to the byte.
 */
#include <stdio.h>
#include <sys/reboot.h>

char	*a_small[] = { "/bin/small", 0 };
char	*a_medium[] = { "/bin/medium", 0 };
char	*a_large[] = { "/bin/large", 0 };
char	*a_tlarge[] = { "/bin/tlarge", 0 };
char	*a_c0[] = { "/bin/c0", "/etc/prog.i", "/tmp/t1", "/tmp/t2", 0 };
char	*a_c1[] = { "/bin/c1", "/tmp/t1", "/tmp/t2", "/tmp/prog.s", 0 };
char	*a_c2[] = { "/bin/c2", "/etc/prog.s", "/tmp/prog.o", 0 };

struct step {
	char	**s_argv;
	char	*s_out;		/* what it writes, which is to be */
	char	*s_want;	/*	the same as this */
} steps[] = {
	{ a_small, 0, 0 },
	{ a_medium, 0, 0 },
	{ a_large, 0, 0 },
	{ a_tlarge, 0, 0 },
	{ a_c0, 0, 0 },
	{ a_c1, "/tmp/prog.s", "/etc/prog.s" },
	{ a_c2, "/tmp/prog.o", "/etc/prog.want" },
};
#define	NSTEP	7

long	samelen[NSTEP];
long	flen;

run(n)
{
	register struct step *sp;
	int pid, status;

	sp = &steps[n];
	pid = fork();
	if (pid == 0) {
		execv(sp->s_argv[0], sp->s_argv);
		_exit(126);
	}
	if (pid < 0)
		return(-1);
	status = -1;
	while (wait(&status) != pid)
		;
	if (status == 0 && sp->s_out) {
		status = differ(sp->s_out, sp->s_want);
		samelen[n] = flen;
	}
	return(status);
}

/*
 * Are two files not the same?  And how long the first is, for what is
 * printed to say that something was compared.
 */
differ(a, b)
char *a, *b;
{
	static char abuf[512], bbuf[512];
	register int fa, fb, n, i;

	fa = open(a, 0);
	fb = open(b, 0);
	if (fa < 0 || fb < 0)
		return(1);
	flen = 0;
	for (;;) {
		n = read(fa, abuf, sizeof abuf);
		if (n < 0 || read(fb, bbuf, sizeof bbuf) != n)
			return(1);
		if (n == 0)
			break;
		for (i = 0; i < n; i++)
			if (abuf[i] != bbuf[i])
				return(1);
		flen += n;
	}
	close(fa);
	close(fb);
	return(flen == 0);
}

main()
{
	register struct step *sp;
	int status[NSTEP], failed, n;

	/* As init nothing is open yet: the console becomes 0, 1 and 2. */
	open("/dev/console", 2);
	dup(0);
	dup(0);
	failed = 0;
	for (n = 0; n < NSTEP; n++)
		if (status[n] = run(n))
			failed++;
	for (sp = steps, n = 0; n < NSTEP; sp++, n++) {
		if (status[n])
			printf("ccom: %s: status %04x\n", sp->s_argv[0],
			    status[n]);
		else if (sp->s_out)
			printf("ccom: %s: %ld bytes, as it makes them anywhere\n",
			    sp->s_argv[0] + 5, samelen[n]);
	}
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
