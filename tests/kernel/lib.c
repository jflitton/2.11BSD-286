/*
 * What the kernel's test programs have for a C library.  The system
 * calls are made by stubs written from syscalls.tab.
 */
#include "lib.h"

static char obuf[128];
static int olen;

flush()
{
	if (olen)
		write(1, obuf, olen);
	olen = 0;
}

/*
 * tests/harness/printf.c prints through this.
 */
cnputc(c)
	int c;
{
	obuf[olen++] = c;
	if (c == '\n' || olen == sizeof (obuf))
		flush();
}

int checks, failed;

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

exit(code)
	int code;
{
	flush();
	_exit(code);
}

strlen(s)
	char *s;
{
	register char *p = s;

	while (*p)
		p++;
	return (p - s);
}

strcmp(a, b)
	register char *a, *b;
{
	while (*a == *b) {
		if (*a == 0)
			return (0);
		a++, b++;
	}
	return (*a - *b);
}

pipe(fd)
	int fd[2];
{
	return (_pipe(fd));
}

/*
 * The kernel's sigaction takes the trampoline too, and far pointers in
 * the structure, whatever the program's own are.
 */
long
signal(sig, handler)
	int sig;
	int (*handler)();
{
	struct ksigaction sa, osa;

	sa.sa_handler = handler;
	sa.sa_seg = (unsigned)handler > 1 ? getcs() : 0;
	sa.sa_mask = 0;
	sa.sa_flags = 0;
	if (_sigaction(sigtramp, sig, &sa, &osa) < 0)
		return (-1);
	return ((long)osa.sa_seg << 16 | (unsigned)osa.sa_handler);
}

/*
 * The break, as the kernel has it, is the size of the data segment.
 */
static unsigned curbrk;

char *
sbrk(incr)
	int incr;
{
	unsigned old;

	if (curbrk == 0)
		curbrk = startbrk();
	old = curbrk;
	if (_brk((char *)(old + incr)) < 0)
		return ((char *)-1);
	curbrk = old + incr;
	return ((char *)old);
}
