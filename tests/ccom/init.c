/*
 * /etc/init for the compiler's test on the system (tests/ccom/run.sh):
 * it runs each of the programs named below, which are prog.c compiled
 * for the two models, and says how many of them failed.  The line
 * "@exit NN" at the end is for tests/harness/runtest.py.
 */
#include <stdio.h>
#include <sys/reboot.h>

char	*progs[] = { "/bin/small", "/bin/medium", 0 };

run(name)
char *name;
{
	int pid, status;

	pid = fork();
	if (pid == 0) {
		execl(name, name + 5, (char *)0);
		_exit(126);
	}
	if (pid < 0)
		return(-1);
	status = -1;
	while (wait(&status) != pid)
		;
	return(status);
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
