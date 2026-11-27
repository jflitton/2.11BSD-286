/*
 * For tests/prof: functions called a number of times that is known,
 * one of them static, a call of the C library's in C and one of its
 * system calls, and fib, which takes long enough to be seen in the
 * profile of where the time went.
 */
#include <stdio.h>

int
fib(n)
	int n;
{
	return n < 2 ? n : fib(n - 1) + fib(n - 2);
}

int
sq(n)
	int n;
{
	return n * n;
}

static int
half(n)
	int n;
{
	return n / 2;
}

main()
{
	register int i;
	long s;

	s = 0;
	for (i = 0; i < 1000; i++)
		s += sq(i % 100);
	for (i = 0; i < 7; i++)
		getpid();
	for (i = -5; i < 5; i++)
		s += abs(i);
	for (i = 0; i < 3; i++)
		s += half(i);
	printf("fib(20) %d, %ld\n", fib(20), s);
	return 0;
}
