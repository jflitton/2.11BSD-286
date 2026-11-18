/*
 * brk and sbrk.  The break is the end of the program's data segment, as
 * the kernel has it (sys/sys/kern_mman.c): the segment's size.  It
 * starts above the stack, which lies after the data, and the start-up
 * code (../csu/crt0.s) is told where.  Below that it would cut into the
 * stack, so a lower break is taken as that one, as on the PDP-11.
 *
 * The break cannot be moved to the very end of a 64K segment this way:
 * that address is 0 in 16 bits.
 */
#include <errno.h>

extern char *_curbrk, *_minbrk;
extern int errno;
int _brk();

char *
brk(addr)
	char *addr;
{
	if (addr < _minbrk)
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
	unsigned new = (unsigned)old + incr;

	if (incr > 0 && new <= (unsigned)old ||
	    incr < 0 && new > (unsigned)old) {
		errno = ENOMEM;
		return ((char *)-1);
	}
	if (incr != 0 && brk((char *)new) == (char *)-1)
		return ((char *)-1);
	return (old);
}
