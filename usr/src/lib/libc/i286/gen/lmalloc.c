/*
 * malloc, free and realloc for the large model.
 *
 * The small and medium models have ../../gen/malloc.c, whose arena is
 * the one data segment, made larger with sbrk up to 64K.  A program of
 * the large model has as many segments as the system will give it
 * (segalloc(2)), and this makes each of them an arena: a header, and
 * then blocks one after another to its end.  A block begins with its
 * size in bytes, the word included, which is even, and whose bit 0
 * says the block is in use.  Free blocks are joined as a search passes
 * them, as in that malloc, and a search starts where the last one left
 * off.
 *
 * A segment is made GROW bytes larger at a time until it is full, and
 * then another is asked for.  One block cannot be more than a segment
 * holds, a little under 64K: for more there is segalloc itself, and a
 * __huge pointer.
 */
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <segment.h>

#define	BUSY	1
#define	HDR	sizeof (unsigned)
#define	GROW	8192
#define	MAXSEG	0xFFF0U		/* so that an offset is never past 16 bits */

struct arena {
	struct arena *a_next;
	unsigned a_size;	/* bytes the segment has */
	unsigned a_rover;	/* the block to look at first */
	unsigned a_free;	/* bytes in free blocks: no more than this
				   is to be had here */
};
#define	FIRST	((sizeof (struct arena) + 1) & ~1)	/* the first block */

#define	BLK(a, off)	(*(unsigned *)((char *)(a) + (off)))
#define	ARENA(p)	((struct arena *)((unsigned long)(p) & 0xFFFF0000L))
#define	OFF(p)		((unsigned)(unsigned long)(p))

static struct arena *arenas, *cur;
extern int errno;

/*
 * Join the free block at off, of size bytes, with the free ones after
 * it, and return what it comes to.
 */
static unsigned
join(a, off, size)
	register struct arena *a;
	register unsigned off, size;
{
	register unsigned next;

	while ((next = off + size) < a->a_size && (BLK(a, next) & BUSY) == 0) {
		if (a->a_rover == next)
			a->a_rover = off;
		size += BLK(a, next);
	}
	BLK(a, off) = size;
	return (size);
}

/*
 * Make the block at off, of size bytes, one of need in use, and what
 * is left over a free one.
 */
static
take(a, off, size, need)
	register struct arena *a;
	register unsigned off;
	unsigned size, need;
{
	if (size - need >= 2 * HDR) {
		BLK(a, off + need) = size - need;
		size = need;
	}
	BLK(a, off) = size | BUSY;
	a->a_free -= size;
	a->a_rover = off + size < a->a_size ? off + size : FIRST;
}

/*
 * A block of need bytes in this arena, or NULL: from the rover to the
 * end, and then from the start to where the rover was.
 */
static char *
search(a, need)
	register struct arena *a;
	unsigned need;
{
	register unsigned off, size;
	unsigned end, start;
	int pass;

	if (a->a_free < need)
		return (NULL);
	start = a->a_rover;
	for (pass = 0; pass < 2; pass++) {
		off = pass ? FIRST : start;
		end = pass ? start : a->a_size;
		while (off < end) {
			size = BLK(a, off);
			if ((size & BUSY) == 0) {
				size = join(a, off, size);
				if (size >= need) {
					take(a, off, size, need);
					return ((char *)a + off + HDR);
				}
			}
			off += size & ~BUSY;
		}
	}
	return (NULL);
}

/*
 * Make the arena larger, for a block of need bytes.
 */
static char *
grow(a, need)
	register struct arena *a;
	unsigned need;
{
	register unsigned more, old;

	old = a->a_size;
	more = need > GROW ? need : GROW;
	if (more > MAXSEG - old)
		more = MAXSEG - old;
	if (more < need || segsize(a, (long)old + more) < 0)
		return (NULL);
	BLK(a, old) = more;
	a->a_size = old + more;
	a->a_free += more;
	a->a_rover = old;
	return (search(a, need));
}

void *
malloc(nbytes)
	unsigned nbytes;
{
	register struct arena *a;
	register char *p;
	unsigned need, size;

	if (nbytes == 0)
		return (NULL);
	if (nbytes > MAXSEG - FIRST - HDR) {
		errno = ENOMEM;
		return (NULL);
	}
	need = (nbytes + HDR + 1) & ~1;
	if (need < 2 * HDR)
		need = 2 * HDR;
	if (cur && (p = search(cur, need)))
		return (p);
	for (a = arenas; a; a = a->a_next)
		if (a != cur && (p = search(a, need))) {
			cur = a;
			return (p);
		}
	for (a = arenas; a; a = a->a_next)
		if (p = grow(a, need)) {
			cur = a;
			return (p);
		}
	size = need + FIRST > GROW ? need + FIRST : GROW;
	if ((a = (struct arena *)segalloc((long)size)) == NULL)
		return (NULL);
	a->a_size = size;
	a->a_rover = FIRST;
	a->a_free = size - FIRST;
	BLK(a, FIRST) = size - FIRST;
	a->a_next = arenas;
	arenas = cur = a;
	return (search(a, need));
}

/*
 * The next search starts with what was freed last.
 */
void
free(ap)
	void *ap;
{
	register struct arena *a = ARENA(ap);
	register unsigned off = OFF(ap) - HDR;

	if (ap == NULL)
		return;
	BLK(a, off) &= ~BUSY;
	a->a_free += BLK(a, off);
	a->a_rover = off;
	cur = a;
}

/*
 * realloc(p, nbytes): p may have been freed since the last call of
 * malloc, as that malloc allows, and is then taken back first.  The
 * block grows where it is if what follows it is free.
 */
void *
realloc(ap, nbytes)
	void *ap;
	unsigned nbytes;
{
	register struct arena *a = ARENA(ap);
	register unsigned off = OFF(ap) - HDR;
	register char *q;
	unsigned size, need;

	if (ap == NULL)
		return (malloc(nbytes));
	if (nbytes > MAXSEG - FIRST - HDR) {
		errno = ENOMEM;
		return (NULL);
	}
	size = BLK(a, off);
	if (size & BUSY)
		a->a_free += size &= ~BUSY;
	if (nbytes == 0) {
		BLK(a, off) = size;
		return (NULL);
	}
	need = (nbytes + HDR + 1) & ~1;
	if (need < 2 * HDR)
		need = 2 * HDR;
	size = join(a, off, size);
	if (size >= need) {
		take(a, off, size, need);
		return (ap);
	}
	a->a_free -= size;
	BLK(a, off) = size | BUSY;
	if ((q = malloc(nbytes)) == NULL)
		return (NULL);
	bcopy(ap, q, size - HDR);
	free(ap);
	return (q);
}
