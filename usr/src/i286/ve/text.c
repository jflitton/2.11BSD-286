/*
 * ve: where the text is kept.
 *
 * Lines are in blocks of BLKSIZE bytes.  A text (struct text) is a
 * chain of blocks: the file is one, and each register and each change
 * that can be undone has another.  A block holds whole lines, one after
 * another, each a count of two bytes and then that many characters,
 * with no newline; the top bit of the count marks the line, for
 * :global.  What is known of a block is not in the block but in a
 * table: how much of it is in use, how many lines that is, and its
 * neighbours in the chain.  To find line n the chain is walked by
 * those counts, from whichever is nearer of its start, its end and the
 * block last used, and then the block is walked by the lines' lengths.
 * Nothing moves from block to block but when a block is too full for a
 * line that is to go in it, and then what follows the line goes to a
 * new one.
 *
 * Only some of the blocks are in memory: as many as the cache may
 * hold, which is 128K unless it is set (:set cache).  The memory is
 * segments that the system is asked for as they are needed
 * (segalloc(2)), 16 blocks to one of 64K, and so is the table.  When
 * there are more blocks than that, one that was not used lately is
 * written to a file in /tmp, which has no name once it is open, and
 * read again when it is wanted.  A block is given a place in the file
 * the first time it is put out, and keeps it; a file that fits in the
 * cache never has the file made.
 *
 * Two things are seen to so that the file's filling the disk, which
 * can happen, never happens where nothing can be done about it.  A new
 * block is given its memory when it is made (balloc), putting another
 * out if it must, and if that fails there is no new block: the text
 * does not grow, and the caller says so.  And once there is a file one
 * place in memory is always kept free, so that a block can be read
 * before another is put out to make room again; if the one put out
 * has no place in the file it takes that of the block just read, which
 * is then the one without.
 *
 * A line is given and taken through a buffer of the caller's own, so
 * nothing outside this file has a far pointer.
 */
#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>
#include <fcntl.h>
#include <segment.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ve.h"

#define	BLKSIZE	4096
#define	BLKSHIFT 12
#define	SEGSLOTS 16		/* blocks of memory in a segment */
#define	SEGSTEP	4		/* a segment is made this many larger */
#define	MAXSEGS	15
#define	MAXSLOT	(MAXSEGS * SEGSLOTS)
#define	MINSLOT	8		/* the least the cache may be */
#define	NBLK	4096		/* blocks there may be: 16 megabytes */
#define	TABSTEP	256		/* the table is made this many larger */
#define	NOSLOT	255

struct blk {
	unsigned b_used;	/* bytes of it in use */
	unsigned b_lines;	/* lines in it */
	short	b_next, b_prev;	/* in its text; b_next too if it is free */
	short	b_home;		/* its place in the file of blocks, or -1 */
	u_char	b_slot;		/* its place in memory, or NOSLOT */
	u_char	b_spare;
};

static struct blk __far *blk;	/* the table, a segment of its own */
static int ntab;		/* blocks it has room for */
static int nblk;		/* blocks there are */
static int freeb = -1;		/* the first free one */
static long inuse;		/* blocks that are part of a text */

static char __far *segs[MAXSEGS];	/* the memory */
static short sblk[MAXSLOT];	/* the block in each place, or -1 */
static char sflag[MAXSLOT];
#define	S_REF	1		/* used since the hand last passed */
#define	S_DIRTY	2		/* not as its place in the file has it */
static int nslot;		/* places there are */
static int nfree;		/* of them with no block */
static int maxslot = 32;	/* places there may be */
static int hand;		/* where the next to go is looked for */
static int keep0 = -1, keep1 = -1;	/* the two last asked for */
static int tfd = -1;		/* the file of blocks */
static int nhome;		/* places in it */
static int tookhome;
static char tname[16];
static char why[80] = "Out of memory";

#define	SLOTP(s) (segs[(s) >> 4] + (((unsigned)(s) & 15) << BLKSHIFT))
#define	RAW(p)	(*(u_short __far *)(p))
#define	MARK	0x8000		/* in a line's count: :global wants it */
#define	LEN(p)	(RAW(p) & ~MARK)

/*
 * Why the text could not be made larger, when it could not.
 */
char *
t_err(void)
{
	return (why);
}

static void
tfail(void)
{
	(void)sprintf(why, "/tmp: %.60s", strerror(errno));
}

/*
 * SEGSTEP more places in memory, if there may be more.
 */
static int
grow(void)
{
	register int s = nslot >> 4, i;

	if (nslot >= maxslot)
		return (-1);
	if ((nslot & 15) == 0) {
		segs[s] = (char __far *)segalloc((long)SEGSTEP * BLKSIZE);
		if (segs[s] == 0)
			return (-1);
	} else if (segsize(segs[s],
	    (long)((nslot & 15) + SEGSTEP) * BLKSIZE) < 0)
		return (-1);
	for (i = 0; i < SEGSTEP; i++) {
		sblk[nslot + i] = -1;
		sflag[nslot + i] = 0;
	}
	nslot += SEGSTEP;
	nfree += SEGSTEP;
	return (0);
}

static int
freeslot(void)
{
	register int s;

	for (s = 0; s < nslot; s++)
		if (sblk[s] < 0)
			return (s);
	return (-1);
}

/*
 * Put the block in place s out of memory.  If it has no place in the
 * file it is given home, that of a block which is in memory now, or
 * with -1 a new one at the end.  -1 if it could not be written.
 */
static int
putout(register int s, int home)
{
	register int b = sblk[s];
	int h = blk[b].b_home;

	tookhome = 0;
	if (h < 0 || (sflag[s] & S_DIRTY)) {
		if (h < 0 && (h = home) < 0)
			h = nhome;
		if (tfd < 0) {
			(void)strcpy(tname, "/tmp/veXXXXXX");
			if ((tfd = mkstemp(tname)) < 0) {
				tfail();
				return (-1);
			}
			(void)unlink(tname);
			(void)fcntl(tfd, F_SETFD, 1);
		}
		if (lseek(tfd, (long)h << BLKSHIFT, L_SET) < 0 ||
		    fout(tfd, SLOTP(s), BLKSIZE) != BLKSIZE) {
			tfail();
			return (-1);
		}
		if (h == nhome)
			nhome++;
		else if (h == home && blk[b].b_home < 0)
			tookhome = 1;
		blk[b].b_home = h;
	}
	blk[b].b_slot = NOSLOT;
	sblk[s] = -1;
	sflag[s] = 0;
	nfree++;
	return (0);
}

/*
 * Put out a block that was not used lately, and not one of the two
 * last asked for.
 */
static int
evict(int home)
{
	register int s, tries;

	for (tries = 3 * nslot; tries > 0; tries--) {
		if (++hand >= nslot)
			hand = 0;
		s = hand;
		if (sblk[s] < 0 || s == keep0 || s == keep1)
			continue;
		if (sflag[s] & S_REF) {
			sflag[s] &= ~S_REF;
			continue;
		}
		return (putout(s, home));
	}
	(void)strcpy(why, "Out of memory");
	return (-1);
}

/*
 * The file of blocks cannot be read or written: nothing is to be done
 * but stop.
 */
static void
tdead(void)
{
	tfail();
	fatal(why);
}

/*
 * Bring block b into memory, and say where.
 */
static int
pagein(register int b)
{
	register int s = freeslot();
	int h = blk[b].b_home;

	if (s < 0 || h < 0)
		fatal("The text is lost: a block is nowhere");
	if (lseek(tfd, (long)h << BLKSHIFT, L_SET) < 0 ||
	    fin(tfd, SLOTP(s), blk[b].b_used) != blk[b].b_used)
		tdead();
	sblk[s] = b;
	sflag[s] = 0;
	blk[b].b_slot = s;
	nfree--;
	keep1 = keep0;
	keep0 = s;
	/* and a place free again for the next */
	if (nfree == 0 && grow() < 0) {
		if (evict(h) < 0)
			tdead();
		if (tookhome) {
			blk[b].b_home = -1;
			sflag[s] |= S_DIRTY;
		}
	}
	return (s);
}

/*
 * Where block b is in memory; with w it is to be written on.  What
 * this gives is good until the call after the next.
 */
static char __far *
bp(register int b, int w)
{
	register int s;

	if ((s = blk[b].b_slot) == NOSLOT)
		s = pagein(b);
	else if (s != keep0) {
		keep1 = keep0;
		keep0 = s;
	}
	sflag[s] |= w ? S_REF | S_DIRTY : S_REF;
	return (SLOTP(s));
}

/*
 * A block that nothing is using, in memory; or -1 if there is no more
 * memory, or no more room in the file for what must go out of it.
 */
static int
balloc(void)
{
	register int b, s;

	(void)strcpy(why, "Out of memory");
	/* a place for it, and one over when there is a file */
	while (nfree < (nhome ? 2 : 1))
		if (grow() < 0 && (nslot < MINSLOT || evict(-1) < 0))
			return (-1);
	if ((b = freeb) >= 0)
		freeb = blk[b].b_next;
	else {
		if (nblk >= ntab) {
			if (ntab >= NBLK) {
				(void)strcpy(why, "Too much text");
				return (-1);
			}
			if (ntab == 0) {
				blk = (struct blk __far *)segalloc(
				    (long)TABSTEP * sizeof (struct blk));
				if (blk == 0)
					return (-1);
			} else if (segsize(blk, (long)(ntab + TABSTEP) *
			    sizeof (struct blk)) < 0)
				return (-1);
			ntab += TABSTEP;
		}
		b = nblk++;
		blk[b].b_home = -1;
	}
	s = freeslot();
	sblk[s] = b;
	sflag[s] = S_REF | S_DIRTY;
	nfree--;
	keep1 = keep0;
	keep0 = s;
	blk[b].b_slot = s;
	blk[b].b_used = blk[b].b_lines = 0;
	blk[b].b_next = blk[b].b_prev = -1;
	inuse++;
	return (b);
}

static void
bfree(register int b)
{
	register int s = blk[b].b_slot;

	if (s != NOSLOT) {
		sblk[s] = -1;
		sflag[s] = 0;
		nfree++;
		blk[b].b_slot = NOSLOT;
		if (keep0 == s)
			keep0 = -1;
		if (keep1 == s)
			keep1 = -1;
	}
	blk[b].b_next = freeb;
	freeb = b;
	inuse--;
}

/*
 * How many K of text there is, and at *file how much of it has a place
 * in the file of blocks: for :file.
 */
long
t_blocks(long *file)
{
	*file = (long)nhome * (BLKSIZE / 1024);
	return (inuse * (BLKSIZE / 1024));
}

/*
 * Let the cache be k K of memory, as near as may be, or with -1 leave
 * it; and say what it is.  If it is made smaller, what is in the
 * memory given up is put out.
 */
int
t_cache(int k)
{
	register int n, s;

	if (k < 0)
		return (maxslot * (BLKSIZE / 1024));
	n = k / (SEGSTEP * (BLKSIZE / 1024)) * SEGSTEP;
	if (n < MINSLOT)
		n = MINSLOT;
	if (n > MAXSLOT)
		n = MAXSLOT;
	keep0 = keep1 = -1;
	while (nslot > n) {
		for (s = nslot - SEGSTEP; s < nslot; s++)
			if (sblk[s] >= 0 && putout(s, -1) < 0)
				break;
		if (s < nslot)
			break;		/* no room for it in the file */
		nslot -= SEGSTEP;
		nfree -= SEGSTEP;
		if ((nslot & 15) == 0)
			(void)segfree(segs[nslot >> 4]);
		else
			(void)segsize(segs[nslot >> 4],
			    (long)(nslot & 15) * BLKSIZE);
	}
	maxslot = n > nslot ? n : nslot;
	hand = 0;
	/* one place free, as there must be */
	while (nhome && nfree == 0)
		if (evict(-1) < 0) {
			maxslot = nslot + SEGSTEP;
			if (maxslot > MAXSLOT || grow() < 0)
				tdead();
		}
	return (maxslot * (BLKSIZE / 1024));
}

void
t_init(struct text *t)
{
	t->t_head = t->t_tail = t->t_blk = -1;
	t->t_lines = 0;
}

void
t_free(struct text *t)
{
	register int b, n;

	for (b = t->t_head; b >= 0; b = n) {
		n = blk[b].b_next;
		bfree(b);
	}
	t_init(t);
}

/*
 * Find line n, which the text has: the block it is in is t->t_blk, and
 * the result is where in the block it starts.  The block is in memory
 * at lp, to be read there before anything else is asked for.
 */
static char __far *lp;

static unsigned
locate(register struct text *t, long n)
{
	register int b;
	register unsigned off;
	long first, line, d;
	char __far *p;

	/* the block: from the one last used, the first or the last */
	b = t->t_blk;
	first = t->t_first;
	d = b < 0 ? t->t_lines + 1 : n < first ? first - n : n - first;
	if (d > n || d > t->t_lines - n) {
		if (n > t->t_lines / 2) {
			b = t->t_tail;
			first = t->t_lines - blk[b].b_lines + 1;
		} else {
			b = t->t_head;
			first = 1;
		}
	}
	while (n < first) {
		b = blk[b].b_prev;
		first -= blk[b].b_lines;
	}
	while (n >= first + blk[b].b_lines) {
		first += blk[b].b_lines;
		b = blk[b].b_next;
	}
	/* the line: from the one last found, if that is before it */
	if (b == t->t_blk && t->t_line && t->t_line <= n) {
		line = t->t_line;
		off = t->t_off;
	} else {
		line = first;
		off = 0;
	}
	lp = p = bp(b, 0);
	while (line < n) {
		off += LEN(p + off) + 2;
		line++;
	}
	t->t_blk = b;
	t->t_first = first;
	t->t_line = n;
	t->t_off = off;
	return (off);
}

/*
 * Copy line n to to, with a null after it, and return its length; -1
 * if the text has no such line.
 */
int
t_get(struct text *t, long n, char *to)
{
	register unsigned off, len;
	char __far *p;

	if (n < 1 || n > t->t_lines) {
		*to = 0;
		return (-1);
	}
	off = locate(t, n);
	p = lp + off;
	len = LEN(p);
	fcopy(p + 2, (char __far *)to, len);
	to[len] = 0;
	return (len);
}

/*
 * The length of line n, or -1.
 */
int
t_len(struct text *t, long n)
{
	register unsigned off;

	if (n < 1 || n > t->t_lines)
		return (-1);
	off = locate(t, n);
	return (LEN(lp + off));
}

/*
 * Mark line n, or take its mark away; and whether it has one.  A line
 * that is changed or copied loses its mark.
 */
void
t_setmark(struct text *t, long n, int on)
{
	register unsigned off;
	char __far *p;

	if (n < 1 || n > t->t_lines)
		return;
	off = locate(t, n);
	p = bp(t->t_blk, 1) + off;
	RAW(p) = on ? RAW(p) | MARK : RAW(p) & ~MARK;
}

int
t_ismark(struct text *t, long n)
{
	register unsigned off;

	if (n < 1 || n > t->t_lines)
		return (0);
	off = locate(t, n);
	return ((RAW(lp + off) & MARK) != 0);
}

/*
 * How many lines there are in the len bytes at p.
 */
static unsigned
count(char __far *p, unsigned len)
{
	register unsigned off, n;

	for (off = n = 0; off < len; n++)
		off += LEN(p + off) + 2;
	return (n);
}

/*
 * Make the len bytes at from a new line n, before the one that is n
 * now; n may be one more than the text has.  -1 if there is no memory
 * for it.
 */
int
t_ins(register struct text *t, long n, char *from, int len)
{
	register int b, nb;
	int pb;
	unsigned off, need, tail, moved;
	char __far *p, __far *q;

	need = len + 2;
	if (t->t_head < 0) {
		if ((b = balloc()) < 0)
			return (-1);
		t->t_head = t->t_tail = b;
		off = 0;
		t->t_blk = b;
		t->t_first = 1;
	} else if (n > t->t_lines) {
		/* after the last */
		b = t->t_tail;
		off = blk[b].b_used;
		t->t_blk = b;
		t->t_first = t->t_lines - blk[b].b_lines + 1;
	} else {
		off = locate(t, n);
		b = t->t_blk;
		if (off == 0) {
			/*
			 * Before the first line of a block: at the end of
			 * the block before, if it has room; and if not, and
			 * this one is half full, in a block of its own, so
			 * that lines put in one after another fill blocks
			 * and do not carry what follows them along.
			 */
			pb = blk[b].b_prev;
			if (pb >= 0 && blk[pb].b_used + need <= BLKSIZE) {
				t->t_first -= blk[pb].b_lines;
				b = pb;
				off = blk[b].b_used;
			} else if (blk[b].b_used >= BLKSIZE / 2) {
				if ((nb = balloc()) < 0)
					return (-1);
				blk[nb].b_prev = pb;
				blk[nb].b_next = b;
				blk[b].b_prev = nb;
				if (pb >= 0)
					blk[pb].b_next = nb;
				else
					t->t_head = nb;
				b = nb;
			}
			t->t_blk = b;
		}
	}
	t->t_line = 0;
	if (blk[b].b_used + need > BLKSIZE) {
		/*
		 * No room: another block after this one, which gets what
		 * follows the new line here; and the line itself, if it is
		 * the last of this block, or will not fit before the rest.
		 */
		if ((nb = balloc()) < 0)
			return (-1);
		blk[nb].b_prev = b;
		if ((blk[nb].b_next = blk[b].b_next) >= 0)
			blk[blk[nb].b_next].b_prev = nb;
		else
			t->t_tail = nb;
		blk[b].b_next = nb;
		tail = blk[b].b_used - off;
		if (tail) {
			q = bp(nb, 1);
			p = bp(b, 1);
			fcopy(p + off, q, tail);
			moved = count(q, tail);
			blk[nb].b_used = tail;
			blk[nb].b_lines = moved;
			blk[b].b_used = off;
			blk[b].b_lines -= moved;
		}
		if (tail == 0 || off + need > BLKSIZE) {
			if (tail == 0)
				t->t_first += blk[b].b_lines;
			else
				t->t_first = n;
			b = nb;
			off = 0;
			t->t_blk = b;
		}
	}
	p = bp(b, 1) + off;
	if (blk[b].b_used > off)
		fmove(p, p + need, blk[b].b_used - off);
	RAW(p) = len;
	fcopy((char __far *)from, p + 2, len);
	blk[b].b_used += need;
	blk[b].b_lines++;
	t->t_lines++;
	return (0);
}

/*
 * Take line n away.
 */
void
t_del(register struct text *t, long n)
{
	register int b;
	int gone, into;
	unsigned off, len;
	char __far *p, __far *q;

	if (n < 1 || n > t->t_lines)
		return;
	off = locate(t, n);
	b = t->t_blk;
	p = bp(b, 1) + off;
	len = LEN(p) + 2;
	blk[b].b_used -= len;
	if (blk[b].b_used > off)
		fcopy(p + len, p, blk[b].b_used - off);
	t->t_lines--;
	t->t_line = 0;
	blk[b].b_lines--;
	/*
	 * A block is given back when it is empty, and before that when
	 * what is left in it fits in the block before, or what is in
	 * the block after fits here: then the two are made one.
	 */
	if (blk[b].b_lines == 0) {
		gone = b;
		into = -1;
	} else if ((into = blk[b].b_prev) >= 0 &&
	    blk[into].b_used + blk[b].b_used <= BLKSIZE)
		gone = b;
	else if ((gone = blk[b].b_next) >= 0 &&
	    blk[gone].b_used + blk[b].b_used <= BLKSIZE)
		into = b;
	else
		return;
	if (into >= 0) {
		q = bp(gone, 0);
		p = bp(into, 1);
		fcopy(q, p + blk[into].b_used, blk[gone].b_used);
		blk[into].b_used += blk[gone].b_used;
		blk[into].b_lines += blk[gone].b_lines;
	}
	if (blk[gone].b_prev >= 0)
		blk[blk[gone].b_prev].b_next = blk[gone].b_next;
	else
		t->t_head = blk[gone].b_next;
	if (blk[gone].b_next >= 0)
		blk[blk[gone].b_next].b_prev = blk[gone].b_prev;
	else
		t->t_tail = blk[gone].b_prev;
	bfree(gone);
	t->t_blk = -1;
}

/*
 * Make line n the len bytes at from.  -1 if there is no memory: then
 * the line is as it was.
 */
int
t_put(register struct text *t, long n, char *from, int len)
{
	register int b;
	unsigned off, old, rest;
	char __far *p;

	if (n < 1 || n > t->t_lines)
		return (-1);
	off = locate(t, n);
	b = t->t_blk;
	p = bp(b, 1) + off;
	old = LEN(p);
	if (blk[b].b_used - old + len > BLKSIZE) {
		/* it has outgrown its block: put it in again */
		if (t_ins(t, n + 1, from, len) < 0)
			return (-1);
		t_del(t, n);
		return (0);
	}
	rest = blk[b].b_used - (off + 2 + old);
	if (rest && old != (unsigned)len)
		fmove(p + 2 + old, p + 2 + len, rest);
	RAW(p) = len;
	fcopy((char __far *)from, p + 2, len);
	blk[b].b_used = blk[b].b_used - old + len;
	return (0);
}

/*
 * Copy count lines of from, from line first on, into to before its
 * line at.  -1 if memory ran out on the way: to has what it has.
 */
int
t_copy(struct text *to, long at, struct text *from, long first, long count)
{
	static char line[LINEMAX + 2];
	register int len;

	while (count-- > 0) {
		if ((len = t_get(from, first++, line)) < 0)
			break;
		if (t_ins(to, at++, line, len) < 0)
			return (-1);
	}
	return (0);
}
