/*
 * A program the system's own compiler compiled, run on the system
 * from a RAM disk (tests/ccom/run.sh, and init.c, which runs it): it
 * is linked with the C library, which the cross compiler compiled, and
 * calls into it with arguments of every kind, is called back from it,
 * and makes system calls.
 *
 * It is there twice: as /bin/small in the small model, and as
 * /bin/medium in the medium, where its functions are far.
 */
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

struct item {
	char	*name;
	int	n;
	long	l;
};

struct item items[] = {
	{ "pear", 3, 300000L },
	{ "apple", 1, 100000L },
	{ "quince", 4, 400000L },
	{ "banana", 2, 200000L },
	{ "zucchini", 5, 500000L },
};
#define	NITEMS	(sizeof(items) / sizeof(items[0]))

int	checks, failed;
char	line[128];
extern int errno;
extern char *malloc();
extern long atol(), lseek();

ok(what, cond)
char *what;
{
	checks++;
	if (!cond) {
		failed++;
		printf("FAILED: %s\n", what);
	}
}

same(what, got, want)
char *what, *got, *want;
{
	checks++;
	if (strcmp(got, want) != 0) {
		failed++;
		printf("FAILED: %s: got \"%s\", want \"%s\"\n", what, got, want);
	}
}

static
byname(a, b)
struct item *a, *b;
{
	return strcmp(a->name, b->name);
}

static
byl(a, b)
struct item *a, *b;
{
	if (a->l > b->l)
		return -1;
	return a->l < b->l;
}

static
strings()
{
	char buf[40];
	register char *p;
	register int i;

	strcpy(buf, "hello");
	strcat(buf, ", ");
	strcat(buf, "world");
	same("strcat", buf, "hello, world");
	ok("strlen", strlen(buf) == 12);
	ok("strcmp", strcmp("abc", "abd") < 0 && strcmp("b", "a") > 0);
	ok("strncmp", strncmp("abcd", "abce", 3) == 0);
	p = index(buf, 'w');
	ok("index", p == buf + 7);
	ok("rindex", rindex(buf, 'o') == buf + 8);
	for (i = 0, p = buf; *p; p++)
		if (*p == 'l')
			i++;
	ok("count", i == 3);
	bcopy(buf + 7, buf, 6);
	same("bcopy", buf, "world");
	ok("atoi", atoi("-1234") == -1234);
	ok("atol", atol("1234567") == 1234567L);
}

static
format()
{
	long l;
	unsigned u;
	char c;

	l = -123456789L;
	u = 54321;
	c = 'x';
	sprintf(line, "%d %u %ld %lx", -42, u, l, 0x12345678L);
	same("%d %u %ld %lx", line, "-42 54321 -123456789 12345678");
	sprintf(line, "%s|%5d|%-5d|%05d|%c|%o|%x", "str", 42, 42, 42, c, 8, 255);
	same("widths", line, "str|   42|42   |00042|x|10|ff");
	sprintf(line, "%ld %d %ld %d", 1L, 2, 3L, 4);
	same("mixed", line, "1 2 3 4");
	sprintf(line, "%c%c%c", 'a', c, "z"[0]);
	same("%c", line, "axz");
	ok("sscanf", sscanf("17 -900000 word", "%d %ld %s", &u, &l, line) == 3
	    && u == 17 && l == -900000L && strcmp(line, "word") == 0);
}

static
library()
{
	register int i;
	char *p, *q;
	long sum;

	qsort((char *)items, NITEMS, sizeof(items[0]), byname);
	same("qsort by name", items[0].name, "apple");
	same("qsort by name", items[4].name, "zucchini");
	for (i = 1; i < NITEMS; i++)
		ok("order", strcmp(items[i - 1].name, items[i].name) < 0);
	qsort((char *)items, NITEMS, sizeof(items[0]), byl);
	ok("qsort by long", items[0].n == 5 && items[4].n == 1);
	sum = 0;
	for (i = 0; i < NITEMS; i++)
		sum += items[i].l * items[i].n;
	ok("sum", sum == 5500000L);
	p = malloc(1000);
	q = malloc(2000);
	ok("malloc", p != 0 && q != 0 && p != q);
	for (i = 0; i < 1000; i++)
		p[i] = i;
	for (i = 0; i < 2000; i++)
		q[i] = ~i;
	for (i = 0; i < 1000; i++)
		if (p[i] != (char)i || q[i] != (char)~i)
			break;
	ok("malloc memory", i == 1000);
	free(p);
	free(q);
}

static
files()
{
	register FILE *fp;
	register int i, c;
	int fd, n;
	long total;

	fp = fopen("/tmp/ccom", "w");
	ok("fopen w", fp != NULL);
	if (fp == NULL)
		return;
	for (i = 0; i < 200; i++)
		fprintf(fp, "line %d of %ld\n", i, 200L);
	ok("fclose", fclose(fp) == 0);
	fp = fopen("/tmp/ccom", "r");
	ok("fopen r", fp != NULL);
	if (fp == NULL)
		return;
	n = 0;
	total = 0;
	while ((c = getc(fp)) != EOF) {
		total += c;
		if (c == '\n')
			n++;
	}
	fclose(fp);
	ok("lines", n == 200);
	ok("bytes", total == 203220L);
	fd = open("/tmp/ccom", 0);
	ok("open", fd >= 0);
	ok("lseek", lseek(fd, 0L, 2) == 3090L);
	ok("lseek back", lseek(fd, -16L, 1) == 3074L);
	n = read(fd, line, 100);
	line[n > 0 ? n : 0] = 0;
	same("read", line, "line 199 of 200\n");
	close(fd);
	ok("unlink", unlink("/tmp/ccom") == 0);
	ok("gone", open("/tmp/ccom", 0) < 0 && errno != 0);
}

static
processes()
{
	int pid, status;

	ok("getpid", getpid() > 1);
	pid = fork();
	if (pid == 0)
		_exit(37);
	ok("fork", pid > 0);
	status = 0;
	ok("wait", wait(&status) == pid);
	ok("status", (status >> 8) == 37 && (status & 0377) == 0);
}

/*
 * Floating point, with the library's: printf and atof and the like
 * take doubles and return them.  A child has the 287 too, and what the
 * parent had on it is still there when it has gone.
 */
static
floating()
{
	double atof(), modf(), ldexp(), frexp();
	double d, ip, sum;
	float f;
	int i, e, pid, status;

	d = 3.25;
	f = -0.5;
	sprintf(line, "%f %.2f %.2f %e %g %g", d, d, f, 1.5e10, 0.0001, 1e20);
	same("printf of floating point", line,
	    "3.250000 3.25 -0.50 1.500000e+10 0.0001 1e+20");
	sprintf(line, "%d %.1f %ld %.3f %s", 7, d * 2, 100000L, f / 4, "end");
	same("printf of several", line, "7 6.5 100000 -0.125 end");
	ok("atof", atof("3.25") == d && atof("-1e3") == -1000.0
	    && atof(".5") + f == 0);
	d = modf(-3.75, &ip);
	ok("modf", d == -0.75 && ip == -3.0);
	ok("ldexp", ldexp(0.75, 4) == 12.0);
	d = frexp(12.0, &e);
	ok("frexp", d == 0.75 && e == 4);
	sum = 0;
	for (i = 1; i <= 100; i++)
		sum += i * 0.5;
	ok("sum", sum == 2525.0 && (int)(sum / 100) == 25);
	sprintf(line, "%.4f", atof("2") / 3);
	same("division", line, "0.6667");

	d = 1.5;
	pid = fork();
	if (pid == 0) {
		for (i = 0; i < 1000; i++)
			d = d * 1.0009765625 + i;
		_exit(d > 1000);
	}
	d = d * 3 + ldexp(1.0, pid > 0);
	status = 0;
	ok("child", wait(&status) == pid && (status >> 8) == 1);
	ok("after the child", d + 0.25 == 6.75);
}

/*
 * Functions through pointers, which in the medium model are far: in a
 * table, given to the library, given to the kernel, which calls one
 * for a signal, and in what setjmp saves.
 */
static int caught;
static jmp_buf env;

static void
catch(sig)
int sig;
{
	caught = sig;
}

static
twice(x)
{
	return 2 * x;
}

static
thrice(x)
{
	return 3 * x;
}

static int (*ops[])() = { twice, thrice, 0 };

static
jump(n)
{
	if (n > 3)
		longjmp(env, n);
	jump(n + 1);
}

static
functions()
{
	int (*f)();
	register int i, n;

	f = twice;
	ok("pointer", (*f)(21) == 42 && f == twice && f != thrice);
	n = 0;
	for (i = 0; ops[i]; i++)
		n += (*ops[i])(i + 1);
	ok("table", n == 2 + 6 && i == 2);
	f = ops[1];
	ok("from the table", f == thrice && f(5) == 15);
	ok("size", sizeof(f) == (MEDIUM? 4: 2) && sizeof(ops) == 3 * sizeof(f));
	signal(SIGUSR1, catch);
	kill(getpid(), SIGUSR1);
	ok("signal", caught == SIGUSR1);
	ok("the handler", signal(SIGUSR1, SIG_DFL) == catch);
	if ((n = setjmp(env)) == 0) {
		jump(0);
		ok("longjmp", 0);
	}
	ok("setjmp", n == 4);
}

main(argc, argv)
char **argv;
{
	printf("ccom: started as %s, pid %d\n", argv[0], getpid());
	strings();
	format();
	library();
	files();
	processes();
	floating();
	functions();
	printf("ccom: %s: %d checks, %d failed\n", argv[0], checks, failed);
	exit(failed != 0);
}
