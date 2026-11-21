/*
 * What ve has from the 286 system and its own assembly, for ve built
 * where the tests are run (tests/ve/run.sh): segments, the copying
 * between them, and the terminal's settings.
 */
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>

void *
segalloc(long size)
{
	return (size > 65536L ? 0 : calloc(1, 65536L));
}

int
segsize(void *seg, long size)
{
	return (size > 65536L ? -1 : 0);
}

int
segfree(void *seg)
{
	free(seg);
	return (0);
}

void
fcopy(char *from, char *to, unsigned n)
{
	memcpy(to, from, n);
}

void
fmove(char *from, char *to, unsigned n)
{
	memmove(to, from, n);
}

unsigned
fscan(char *p, int c, unsigned n)
{
	char *q = memchr(p, c, n);

	return (q ? q - p : n);
}

/*
 * The window is as big as VEROWS and VECOLS say, or 24 by 80.
 */
int
ve_ioctl(int fd, long what, char *arg)
{
	struct winsize *ws = (struct winsize *)arg;
	char *s;

	if (what != (long)TIOCGWINSZ) {
		if (what == -1L || what == -3L || what == -5L || what == -7L)
			memset(arg, 0, what == -7L ? sizeof (int) : 6);
		return (0);
	}
	ws->ws_row = (s = getenv("VEROWS")) != 0 ? atoi(s) : 24;
	ws->ws_col = (s = getenv("VECOLS")) != 0 ? atoi(s) : 80;
	return (0);
}

/*
 * The file of blocks: with VEFULL in the environment, it has room for
 * that many bytes and no more.
 */
#include <errno.h>
#include <unistd.h>

int
fin(int fd, char *p, unsigned n)
{
	return (read(fd, p, n));
}

int
fout(int fd, char *p, unsigned n)
{
	char *s = getenv("VEFULL");

	if (s && lseek(fd, 0L, SEEK_CUR) + n > atol(s)) {
		errno = ENOSPC;
		return (-1);
	}
	return (write(fd, p, n));
}
