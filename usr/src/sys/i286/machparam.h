/*
 * Machine dependent constants for the 80286.
 */
#ifndef ENDIAN
/*
 * Definitions for byte order,
 * according to byte significance from low address to high.
 */
#define	LITTLE	1234		/* least-significant byte first (vax, 286) */
#define	BIG	4321		/* most-significant byte first */
#define	PDP	3412		/* LSB first in word, MSW first in long (pdp) */
#define	ENDIAN	LITTLE		/* byte order on the 286 */

/*
 * Macros for network/external number representation conversion.
 */
#include <sys/types.h>
u_short	ntohs(), htons();
u_long	ntohl(), htonl();

#include <machine/limits.h>

#define	NBPG		512		/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */
#define	PGSHIFT		9		/* LOG2(NBPG) */

#define	DEV_BSIZE	1024
#define	DEV_BSHIFT	10		/* log2(DEV_BSIZE) */
#define	DEV_BMASK	0x3ffL		/* DEV_BSIZE - 1 */

#define	CLSIZE		2
#define	CLSIZELOG2	1

/*
 * Memory is counted in clicks of 512 bytes: 16M is 32,768 of them, so a
 * click number fits in 16 bits, and a click is a sector of the swap
 * device.  A segment is at most 128 clicks, which is 65,536 bytes, one
 * more than an unsigned holds: ctob gives a long.
 */
#define	ctob(x)		((long)(x) << 9)
#define	btoc(x)		((unsigned)(((unsigned long)(x) + 511) >> 9))
#define	ctod(x)		(x)		/* clicks to sectors */

#define	btodb(bytes)		/* calculates (bytes / DEV_BSIZE) */ \
	((long)(bytes) >> DEV_BSHIFT)
#define	dbtob(db)		/* calculates (db * DEV_BSIZE) */ \
	((long)(db) << DEV_BSHIFT)

/* clicks to kilobytes */
#define	ctok(x)		((x) >> 1)

/*
 * A process's user structure and kernel stack are one piece of USIZE
 * clicks: the structure at the bottom, the stack growing down toward it
 * from the top.  While the process runs they are in one of the kernel's
 * slots (param.c); p_addr is where they are kept otherwise.
 */
#define	USIZE		7
#define	UBYTES		(USIZE * NBPG)

/*
 * Interrupt priority levels.  The 286 has none of its own: the level is
 * a variable, an interrupt that arrives while it is too high is put off
 * until it drops (machdep.c), and the spl routines only change it.
 * mch_defs.inc says the same to the assembler.
 */
#define	IPL_SOFTCLOCK	1	/* the clock's deferred work */
#define	IPL_NET		2	/* protocol processing */
#define	IPL_BIO		3	/* disks */
#define	IPL_IMP		4	/* network interfaces */
#define	IPL_TTY		5	/* terminals */
#define	IPL_CLOCK	6
#define	IPL_HIGH	7

/*
 * What the clock interrupted, as hardclock and softclock are told it.
 */
#define	CLK_USER	1	/* a process */
#define	CLK_BASE	2	/* something at priority level 0 */

/* machine dependent stuff for core files */
#define	TXTRNDSIZ	512L


#define	DELAY(n)	{ long N = ((long)(n))<<1; while (--N > 0); }

/*
 * The halves of a long, for code that takes one apart.
 */
#define	loint(long)	(((int *)&(long))[0])
#define	hiint(long)	(((int *)&(long))[1])
#ifdef KERNEL
#include "../machine/seg.h"
#endif

#endif /* ENDIAN */
