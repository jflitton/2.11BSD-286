/*
 * brk and sbrk.  The break is the end of the program's data segment, as
 * the kernel has it (sys/sys/kern_mman.c): the segment's size.  It
 * starts above the stack, which lies after the data, and the start-up
 * code (../csu/crt0.s) is told where.  Below that it would cut into the
 * stack, so a lower break is taken as that one, as on the PDP-11.
 *
 * The break cannot be moved to the very end of a 64K segment this way:
 * that address is 0 in 16 bits.
 *
 * sbrk's increment is an int, and 32768 or more is a negative one.  The
 * PDP-11's sbrk adds it to the break in 16 bits and asks no more, so
 * such an increment grows the segment when taking it away would pass
 * zero, and dd, whose buffer may be 54K, counts on it.  So does this
 * one.  What it refuses is an increment that would carry the break past
 * the end of the segment and leave it lower than it was.
 */
#include <errno.h>

/*
 * In the large model the break is a far address, as every address is,
 * in the program's own data segment, and it is its offset that moves.
 */
#ifdef __LARGE__
#define	OFF(p)		((unsigned)(unsigned long)(p))
#define	AT(p, off) \
	((char *)(((unsigned long)(p) & 0xFFFF0000L) | (unsigned)(off)))
#else
#define	OFF(p)		((unsigned)(p))
#define	AT(p, off)	((char *)(off))
#endif

extern char *_curbrk, *_minbrk;
extern int errno;
int _brk();

char *
brk(addr)
	char *addr;
{
	if (OFF(addr) < OFF(_minbrk))
		addr = _minbrk;
	if (_brk(addr) < 0)
		return ((char *)-1);
	_curbrk = addr;
	return ((char *)0);
}

char *
sbrk(incr)
	int incr;
{
	char *old = _curbrk;
	unsigned new = OFF(old) + incr;

	if (incr > 0 && new <= OFF(old)) {
		errno = ENOMEM;
		return ((char *)-1);
	}
	if (incr != 0 && brk(AT(old, new)) == (char *)-1)
		return ((char *)-1);
	return (old);
}
