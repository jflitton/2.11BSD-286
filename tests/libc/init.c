/*
 * The C library's test, run by the kernel as /etc/init from a RAM disk
 * (tests/libc/run.sh).  It calls into each part of the library, checks
 * what comes back, and reports on the console.  The line "@exit NN" at
 * the end is for tests/harness/runtest.py.
 */
#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/reboot.h>
#include <sys/dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

double	modf(), ldexp(), frexp(), atof();
char	*sbrk();
char	*ctime();

int	checks, failed;

check(ok, what)
	int ok;
	char *what;
{
	checks++;
	if (!ok) {
		failed++;
		printf("FAILED: %s\n", what);
	}
}

/* sprintf into a buffer and compare */
char	sbuf[256];
#define	fmt(want, args)	(sprintf args, strcmp(sbuf, want) == 0 ? 1 : \
	(printf("  got \"%s\", wanted \"%s\"\n", sbuf, want), 0))

stdio()
{
	FILE *f;
	char line[80];
	int n, a, b;
	long l;
	char word[20];
	double d;

	check(fmt("42 -7 00042 42   |", (sbuf, "%d %d %05d %-5d|", 42, -7, 42, 42)),
	    "printf: integers");
	check(fmt("123456789 -2147483648 4294967295",
	    (sbuf, "%ld %ld %lu", 123456789L, (long)-2147483647L - 1, 0xffffffffL)),
	    "printf: longs");
	check(fmt("ff FF 0xff 377 65535", (sbuf, "%x %X %#x %o %u", 255, 255, 255, 255, -1)),
	    "printf: other bases");
	check(fmt("x abc   ab|%", (sbuf, "%c %-5s %.2s|%%", 'x', "abc", "abc")),
	    "printf: characters and strings");
	check(fmt("3.250000 3.25 -0.50 1.500000e+10 0.0001 1e+20",
	    (sbuf, "%f %.2f %.2f %e %g %g", 3.25, 3.25, -0.5, 1.5e10, 0.0001, 1e20)),
	    "printf: floating point");
	check(fmt("   3.14", (sbuf, "%7.2f", 3.14159)), "printf: width and precision");

	f = fopen("/tmp/stdio", "w");
	check(f != NULL, "fopen for writing");
	if (f == NULL)
		return;
	for (n = 0; n < 200; n++)
		fprintf(f, "line %d of %s\n", n, "the file");
	check(fclose(f) == 0, "fclose");
	f = fopen("/tmp/stdio", "r");
	check(f != NULL, "fopen for reading");
	if (f == NULL)
		return;
	n = 0;
	while (fgets(line, sizeof line, f))
		n++;
	check(n == 200, "fgets reads back every line");
	check(feof(f), "and then end of file");
	check(fseek(f, 0L, 0) == 0 && fscanf(f, "line %d of %s", &a, word) == 2 &&
	    a == 0 && strcmp(word, "the") == 0, "fseek and fscanf");
	l = ftell(f);
	check(l == 13, "ftell");
	n = getc(f);
	check(n == ' ' && ungetc(n, f) == n && getc(f) == ' ', "getc and ungetc");
	fclose(f);
	check(sscanf("17 0x1f -3.5e2", "%d %x %lf", &a, &b, &d) == 3 &&
	    a == 17 && b == 31 && d == -350.0, "sscanf");
	check(unlink("/tmp/stdio") == 0, "unlink the file");
}

strings()
{
	char a[64], *p;

	strcpy(a, "hello");
	strcat(a, ", world");
	check(strlen(a) == 12 && strcmp(a, "hello, world") == 0, "strcpy, strcat, strlen");
	check(strcmp("abc", "abd") < 0 && strcmp("b", "a") > 0 &&
	    strncmp("abcx", "abcy", 3) == 0, "strcmp and strncmp");
	check(index(a, 'o') == a + 4 && rindex(a, 'o') == a + 8 &&
	    strchr(a, 'w') == a + 7 && strrchr(a, 'z') == NULL,
	    "index, rindex, strchr, strrchr");
	check(strstr(a, "wor") == a + 7 && strpbrk(a, ",") == a + 5 &&
	    strspn(a, "hel") == 4, "strstr, strpbrk, strspn");
	bcopy(a, a + 2, 6);
	check(strncmp(a, "hehello,", 8) == 0, "bcopy that overlaps");
	memset(a, 'x', 10);
	a[10] = 0;
	check(strcmp(a, "xxxxxxxxxx") == 0 && memcmp(a, "xxxy", 4) < 0, "memset, memcmp");
	bzero(a, sizeof a);
	check(a[0] == 0 && a[63] == 0, "bzero");
	strcpy(a, "one two  three");
	p = strtok(a, " ");
	check(p && strcmp(p, "one") == 0 && strcmp(strtok(NULL, " "), "two") == 0 &&
	    strcmp(strtok(NULL, " "), "three") == 0 && strtok(NULL, " ") == NULL,
	    "strtok");
	p = strdup("copy");
	check(p && strcmp(p, "copy") == 0, "strdup");
}

int
cmp(const void *a, const void *b)
{
	return (*(int *)a - *(int *)b);
}

conversions()
{
	int v[10], i, sorted, e;
	char *end;
	double ip, d;

	check(atoi("-1234") == -1234 && atol("70000") == 70000L, "atoi and atol");
	check(strtol("0x7fff", &end, 0) == 32767 && *end == 0 &&
	    strtol("-077", (char **)0, 0) == -63 &&
	    strtoul("4000000000", (char **)0, 10) == 4000000000UL,
	    "strtol and strtoul");
	check(atof("3.25") == 3.25 && atof("-1e3") == -1000.0 && atof(".5") == 0.5,
	    "atof");
	d = modf(-3.75, &ip);
	check(d == -0.75 && ip == -3.0, "modf");
	check(ldexp(0.75, 4) == 12.0, "ldexp");
	d = frexp(12.0, &e);
	check(d == 0.75 && e == 4, "frexp");
	for (i = 0; i < 10; i++)
		v[i] = (i * 7) % 10;
	qsort(v, 10, sizeof v[0], cmp);
	for (sorted = 1, i = 0; i < 10; i++)
		if (v[i] != i)
			sorted = 0;
	check(sorted, "qsort");
}

memory()
{
	char *p[40], *q, *brk0;
	int i, ok;

	brk0 = sbrk(0);
	ok = 1;
	for (i = 0; i < 40; i++) {
		p[i] = malloc(100 + i * 17);
		if (p[i] == NULL)
			ok = 0;
		else
			memset(p[i], i, 100 + i * 17);
	}
	check(ok, "malloc forty blocks");
	for (i = 0; i < 40; i += 2)
		free(p[i]);
	ok = 1;
	for (i = 1; i < 40; i += 2)
		if (p[i][100 + i * 17 - 1] != i)
			ok = 0;
	check(ok, "the others are untouched by freeing half");
	q = malloc(10000);
	check(q != NULL && sbrk(0) > brk0, "a big block moves the break");
	if (q) {
		q[9999] = 1;
		q = realloc(q, 15000);
		check(q != NULL && q[9999] == 1, "realloc keeps the contents");
	}
	q = calloc(50, 4);
	for (ok = q != NULL, i = 0; ok && i < 200; i++)
		if (q[i])
			ok = 0;
	check(ok, "calloc clears");
}

jmp_buf	jb;
int	caught;

deep(n)
{
	if (n == 0)
		longjmp(jb, 5);
	deep(n - 1);
}

void
handler(sig)
	int sig;
{
	caught = sig;
}

void
jumper(sig)
	int sig;
{
	longjmp(jb, sig);
}

signals()
{
	struct sigaction sa, osa;
	sigset_t set, oset;
	int r;
	volatile int stage = 0;

	r = setjmp(jb);
	if (r == 0) {
		stage = 1;
		deep(20);
	}
	check(r == 5 && stage == 1, "longjmp out of a deep call");
	r = _setjmp(jb);
	if (r == 0)
		_longjmp(jb, 0);
	check(r == 1, "_longjmp with 0 returns 1");

	sa.sa_handler = handler;
	sa.sa_mask = 0;
	sa.sa_flags = 0;
	check(sigaction(SIGUSR1, &sa, &osa) == 0 && osa.sa_handler == SIG_DFL,
	    "sigaction");
	kill(getpid(), SIGUSR1);
	check(caught == SIGUSR1, "a handler runs and returns");
	caught = 0;
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigprocmask(SIG_BLOCK, &set, &oset);
	kill(getpid(), SIGUSR1);
	check(caught == 0, "a blocked signal waits");
	sigprocmask(SIG_SETMASK, &oset, (sigset_t *)0);
	check(caught == SIGUSR1, "and arrives when unblocked");

	/* longjmp from a handler puts the mask back as setjmp found it */
	signal(SIGUSR2, jumper);
	if ((r = setjmp(jb)) == 0)
		kill(getpid(), SIGUSR2);
	sigprocmask(SIG_BLOCK, (sigset_t *)0, &set);
	check(r == SIGUSR2 && !sigismember(&set, SIGUSR2),
	    "longjmp out of a handler unblocks its signal");

	signal(SIGALRM, handler);
	caught = 0;
	alarm(1);
	pause();
	check(caught == SIGALRM, "alarm and pause");
}

timing()
{
	struct timeval tv, t0, t1;
	long us;

	gettimeofday(&t0, (struct timezone *)0);
	tv.tv_sec = 0;
	tv.tv_usec = 300000L;
	check(select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &tv) == 0,
	    "select with only a time limit");
	gettimeofday(&t1, (struct timezone *)0);
	us = (t1.tv_sec - t0.tv_sec) * 1000000L + (t1.tv_usec - t0.tv_usec);
	check(us >= 250000L && us < 2000000L, "and it waits that long");
	if (us < 250000L || us >= 2000000L)
		printf("  waited %ld microseconds\n", us);
	gettimeofday(&t0, (struct timezone *)0);
	check(sleep(2) == 0, "sleep");
	gettimeofday(&t1, (struct timezone *)0);
	check(t1.tv_sec - t0.tv_sec >= 1 && t1.tv_sec - t0.tv_sec <= 3,
	    "and it sleeps that long");
}

processes()
{
	int pid, status, fd[2], n;
	char buf[80];
	struct iovec iov[3];

	pid = fork();
	if (pid == 0)
		_exit(7);
	check(pid > 0 && wait(&status) == pid && (status & 0xff) == 0 &&
	    (status >> 8) == 7, "fork and wait");

	check(pipe(fd) == 0, "pipe");
	pid = fork();
	if (pid == 0) {
		close(fd[0]);
		dup2(fd[1], 1);
		close(fd[1]);
		execl("/bin/echoargs", "echoargs", "one", "two", (char *)0);
		_exit(99);
	}
	close(fd[1]);
	n = read(fd[0], buf, sizeof buf - 1);
	buf[n > 0 ? n : 0] = 0;
	close(fd[0]);
	check(waitpid(pid, &status, 0) == pid && (status >> 8) == 3,
	    "execl a program, which exits with its count of arguments");
	check(strcmp(buf, "echoargs|one|two|\n") == 0, "and gets them");

	pipe(fd);
	iov[0].iov_base = "ab";
	iov[0].iov_len = 2;
	iov[1].iov_base = "cde";
	iov[1].iov_len = 3;
	check(writev(fd[1], iov, 2) == 5, "writev");
	iov[0].iov_base = buf;
	iov[0].iov_len = 1;
	iov[1].iov_base = buf + 10;
	iov[1].iov_len = 4;
	check(readv(fd[0], iov, 2) == 5 && buf[0] == 'a' &&
	    strncmp(buf + 10, "bcde", 4) == 0, "readv");
	close(fd[0]);
	close(fd[1]);
}

misc()
{
	struct passwd *pw;
	struct group *gr;
	struct tm *tm;
	DIR *d;
	struct direct *de;
	time_t t;
	int found, fd;
	char *argv[5];
	extern int optind;
	extern char *optarg;

	pw = getpwnam("root");
	check(pw && pw->pw_uid == 0 && strcmp(pw->pw_dir, "/") == 0 &&
	    strcmp(pw->pw_passwd, "") == 0, "getpwnam");
	pw = getpwuid(9);
	check(pw && strcmp(pw->pw_name, "test") == 0 &&
	    strcmp(pw->pw_shell, "/bin/csh") == 0, "getpwuid");
	gr = getgrgid(0);
	check(gr && strcmp(gr->gr_name, "wheel") == 0, "getgrgid");

	t = 0;
	tm = gmtime(&t);
	check(tm->tm_year == 70 && tm->tm_mon == 0 && tm->tm_mday == 1, "gmtime");
	t = 1000000000L;
	tm = gmtime(&t);
	check(tm->tm_year == 101 && tm->tm_mon == 8 && tm->tm_mday == 9 &&
	    tm->tm_hour == 1 && tm->tm_min == 46, "gmtime of a billion seconds");
	check(time((time_t *)0) > 1000000000L, "time");
	check(strlen(ctime(&t)) == 25, "ctime");

	d = opendir("/etc");
	found = 0;
	if (d) {
		while (de = readdir(d))
			if (strcmp(de->d_name, "passwd") == 0)
				found = 1;
		closedir(d);
	}
	check(found, "opendir and readdir");

	errno = 0;
	fd = open("/nonexistent", O_RDONLY);
	check(fd == -1 && errno == ENOENT, "a failed call sets errno");
	check(strcmp(strerror(ENOENT), "No such file or directory") == 0, "strerror");

	check(getenv("HOME") == NULL, "no HOME yet");
	setenv("HOME", "/usr/home", 1);
	check(getenv("HOME") && strcmp(getenv("HOME"), "/usr/home") == 0,
	    "setenv and getenv");

	argv[0] = "prog";
	argv[1] = "-a";
	argv[2] = "-bvalue";
	argv[3] = "file";
	argv[4] = 0;
	check(getopt(4, argv, "ab:") == 'a' && getopt(4, argv, "ab:") == 'b' &&
	    strcmp(optarg, "value") == 0 && getopt(4, argv, "ab:") == -1 &&
	    optind == 3, "getopt");

	check(isatty(0) && strcmp(ttyname(0), "/dev/console") == 0, "isatty and ttyname");
}

main(argc, argv)
	int argc;
	char **argv;
{
	/* Nothing is open yet: the console becomes 0, 1 and 2. */
	open("/dev/console", O_RDWR);
	dup(0);
	dup(0);
	printf("libc: started as %s, pid %d\n", argv[0], getpid());

	stdio();
	strings();
	conversions();
	memory();
	signals();
	timing();
	processes();
	misc();

	printf("libc: %d checks, %d failed\n", checks, failed);
	printf("@exit %02x\n", failed > 255 ? 255 : failed);
	fflush(stdout);
	close(0);			/* the last close of the console */
	close(1);			/*   waits for what was written */
	close(2);			/*   to it to go out */
	sync();
	reboot(RB_HALT);
	for (;;)
		;
}
