/*
 * What the compiler's tests are run by (tests/ccom/run.sh).  This is
 * compiled by the cross compiler: it calls each test, which the tree's
 * own compiler compiled, and the tests call back here with each value
 * they have worked out and what it ought to be.  So nothing is compared
 * or printed by the compiler under test, and every call either way is
 * between the two compilers.
 *
 * For the large model it is compiled by the tree's compiler with them,
 * and run on the system, where printf is the C library's and putc is
 * putchar (-Dputc=putchar).
 */
extern void putc();

int	nchecks;
int	nfailed;

cnputc(c)
{
	if (c == '\n')
		putc('\r');
	putc(c);
}

/*
 * Check number n, which is the line it is on, of the test called name.
 */
char	*testname;

check(n, got, want)
	int n, got, want;
{
	nchecks++;
	if (got != want) {
		nfailed++;
		printf("%s:%d: got %d (0x%x), want %d (0x%x)\n", testname, n,
		    got, got, want, want);
	}
}

lcheck(n, got, want)
	int n;
	long got, want;
{
	nchecks++;
	if (got != want) {
		nfailed++;
		printf("%s:%d: got %ld (0x%lx), want %ld (0x%lx)\n", testname,
		    n, got, got, want, want);
	}
}

/*
 * For the tests of calls: functions of this compiler's that the tests
 * call, and one that calls back.
 */
int
sum5(a, b, c, d, e)
	int a, b, c, d, e;
{
	return a + 2 * b + 3 * c + 4 * d + 5 * e;
}

long
lsum3(a, b, c)
	long a;
	int b;
	long c;
{
	return a + b + c;
}

char
lowbyte(x)
	int x;
{
	return x;
}

int
callback(f, x)
	int (*f)();
	int x;
{
	return (*f)(x, x + 1) + 1;
}

/*
 * Leaves the registers a function may destroy holding something else.
 */
int
scramble(x)
	int x;
{
	static int junk[8];
	register int i, j;

	for (i = 0, j = 7; i < 8; i++, j--)
		junk[i] = junk[j] * 31 + x + i;
	return junk[x & 7];
}

/*
 * For the tests of structures: functions of this compiler's that make
 * them and take them, and ones that call the tests' own.
 */
#include "structs.h"

struct s2
mk2(a)
	int a;
{
	struct s2 s;

	s.a = a;
	return s;
}

struct s4
mk4(a, b)
	int a, b;
{
	struct s4 s;

	s.a = a;
	s.b = b;
	return s;
}

struct s6
mk6(a, b, c)
	int a, b, c;
{
	struct s6 s;

	s.a = a;
	s.b = b;
	s.c = c;
	return s;
}

struct s10
mk10(tag, l, v)
	int tag;
	long l;
	int v;
{
	struct s10 s;

	s.tag = tag;
	s.l = l;
	s.v[0] = v;
	s.v[1] = -v;
	return s;
}

struct s24
mk24(a)
	int a;
{
	struct s24 s;
	int i;

	for (i = 0; i < 12; i++)
		s.v[i] = a + i;
	return s;
}

int
take2(x, s, y)
	int x, y;
	struct s2 s;
{
	return x + 2 * s.a + 3 * y;
}

int
take4(x, s, y)
	int x, y;
	struct s4 s;
{
	return x + 2 * s.a + 3 * s.b + 4 * y;
}

int
take6(x, s, y)
	int x, y;
	struct s6 s;
{
	return x + 2 * s.a + 3 * s.b + 4 * s.c + 5 * y;
}

long
take10(x, s, y)
	int x, y;
	struct s10 s;
{
	return x + s.tag + s.l + 2 * s.v[0] + 5 * s.v[1] + y;
}

int
take24(x, s, y)
	int x, y;
	struct s24 s;
{
	int i, n;

	n = x - y;
	for (i = 0; i < 12; i++)
		n += (i + 1) * s.v[i];
	return n;
}

int
call2(mk, take)
	struct s2 (*mk)();
	int (*take)();
{
	struct s2 s;

	s = (*mk)(7);
	return (*take)(1, s, 2) + s.a;
}

int
call4(mk, take)
	struct s4 (*mk)();
	int (*take)();
{
	struct s4 s;

	s = (*mk)(7, 8);
	return (*take)(1, s, 2) + s.b;
}

int
call6(mk, take)
	struct s6 (*mk)();
	int (*take)();
{
	struct s6 s;

	s = (*mk)(7, 8, 9);
	return (*take)(1, s, 2) + s.c;
}

long
call10(mk, take)
	struct s10 (*mk)();
	long (*take)();
{
	struct s10 s;

	s = (*mk)('x', 100000L, 30);
	return (*take)(1, s, 2) + s.v[1];
}

int
call24(mk, take)
	struct s24 (*mk)();
	int (*take)();
{
	struct s24 s;

	s = (*mk)(5);
	return (*take)(1, s, 2) + s.v[11];
}

/*
 * For the tests of floating point.  A value is checked as it is, bit
 * for bit: the tests are of numbers that a double holds exactly.
 */
dcheck(n, got, want)
	int n;
	double got, want;
{
	nchecks++;
	if (got != want) {
		nfailed++;
		printf("%s:%d: got %ld/1024, want %ld/1024\n", testname, n,
		    (long)(got * 1024), (long)(want * 1024));
	}
}

double
dsum3(a, b, c)
	double a, c;
	int b;
{
	return a + 2 * b + 4 * c;
}

float
fhalf(x)
	float x;
{
	return x / 2;
}

double
dcallback(f, x)
	double (*f)();
	double x;
{
	return (*f)(x, 3, x + 1) + 0.5;
}

/*
 * Leaves the 287's stack full of something else.
 */
double
dscramble(x)
	double x;
{
	double a, b, c, d;

	a = x + 1;
	b = a * x;
	c = b - a;
	d = c / (a + 2);
	return (a * b + c * d) * ((a - b) * (c - d)) * 0 + x;
}

/*
 * For the tests of far pointers: a buffer whose address is given out
 * far, functions that take far pointers, and functions that are far
 * themselves, which are called through pointers to them.
 */
static char farbuf[] = "far away";

char __far *
fr_addr()
{
	return farbuf;
}

long
fr_seg()
{
	return (unsigned long)(char __far *)farbuf >> 16;
}

int
fr_peek(p, i)
	char __far *p;
	int i;
{
	return p[i];
}

int
fr_sum(p, n)
	int __far *p;
	int n;
{
	int s;

	s = 0;
	while (n-- > 0)
		s += *p++;
	return s;
}

long
fr_proto(char __far *p, int __far *q, int n)
{
	return *p + 2L * q[n];
}

static int __far
fr_add(a, b)
	int a, b;
{
	return a + b;
}

static int __far
fr_mul(a, b)
	int a, b;
{
	return a * b;
}

int (__far *
fr_func(which))()
	int which;
{
	return which ? fr_mul : fr_add;
}

/*
 * The tests, which run.sh lists in a file it writes.
 */
extern struct test {
	char	*name;
	int	(*fn)();
} tests[];

int
main()
{
	register struct test *tp;

	for (tp = tests; tp->name; tp++) {
		testname = tp->name;
		(*tp->fn)();
	}
	printf("%d checks, %d failed\n", nchecks, nfailed);
	return nfailed != 0;
}
