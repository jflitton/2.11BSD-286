/*
 * What the compiler's tests are run by (tests/ccom/run.sh).  This is
 * compiled by the cross compiler: it calls each test, which the tree's
 * own compiler compiled, and the tests call back here with each value
 * they have worked out and what it ought to be.  So nothing is compared
 * or printed by the compiler under test, and every call either way is
 * between the two compilers.
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
