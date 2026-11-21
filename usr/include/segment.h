/*
 * More data segments than a program came with: segalloc(2).
 *
 * A program's own data is one segment of 64K at most, and a pointer
 * into it is 16 bits.  What these give is outside it, and is reached
 * by a far pointer, a selector and an offset, which a program of any
 * memory model may have:
 *
 *	char __far *p;
 *
 *	p = segalloc(40000L);
 *	p[39999] = 1;
 */
#ifndef _SEGMENT_H_
#define	_SEGMENT_H_

void __far	*segalloc(long size);
int	segsize(void __far *seg, long size);
int	segfree(void __far *seg);

/*
 * The address of byte n of a piece of more than 64K that starts at p:
 * the piece is segments of 64K one after another, and the selector of
 * each is the one before it and 8.
 */
#define	SEGADDR(p, n) \
	((char __far *)(((unsigned long)(p) & 0xFFFF0000L) + \
	    (((unsigned long)(n) >> 16) << 19) + ((unsigned long)(n) & 0xFFFFL)))

#endif /* _SEGMENT_H_ */
