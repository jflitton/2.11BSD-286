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
 * Macros for network/external number representation conversion.  The
 * kernel has them in line.
 */
#include <sys/types.h>
#ifdef KERNEL
u_short	ntohs(u_short), htons(u_short);
u_long	ntohl(u_long), htonl(u_long);
#pragma aux ntohs = "xchg al,ah" parm [ax] value [ax] modify exact [ax];
#pragma aux htons = "xchg al,ah" parm [ax] value [ax] modify exact [ax];
#pragma aux ntohl = "xchg al,ah" "xchg dl,dh" "xchg ax,dx" \
	parm [dx ax] value [dx ax] modify exact [ax dx];
#pragma aux htonl = "xchg al,ah" "xchg dl,dh" "xchg ax,dx" \
	parm [dx ax] value [dx ax] modify exact [ax dx];
#else
u_short	ntohs(), htons();
u_long	ntohl(), htonl();
#endif

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
 * Routines the machine-independent code has in C that are assembly
 * here (mch_xxx.s).
 */
#define	MCH_NAMECOMP		/* namecomp, for namei */

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
/*
 * The network has a data segment of its own, and runs on the kernel's
 * stacks (docs/network.md), so it reaches the kernel's data through SS.
 * KSPACE makes a pointer of that kind in the network's code; in the
 * kernel's it is an ordinary pointer.
 *
 * Addresses the kernel keeps for the network, as on the PDP-11: SUPERADD
 * makes a channel to sleep on that is unlike any of the kernel's own, and
 * SUPERFUN marks a network function for softclock to call with KScall
 * (bit 0 of the selector, which is otherwise 0 for the kernel's code).
 */
#ifdef SUPERVISOR
#define	KSPACE		__based(__segname("_STACK"))
#else
#define	KSPACE
#endif
/*
 * A pointer in the network's code that may point at one of its local
 * variables, on the kernel's stack, or at its own data, as a pointer
 * that starts at a list's head and walks down its links: it is far.
 */
#ifdef SUPERVISOR
#define	ANYSPACE	__far
#else
#define	ANYSPACE
#endif
#define	SUPERADD(add)	((int)(add)|01)
#define	KERNELADD(add)	((int)(add)&~01)
#define	ISSUPERADD(add)	((int)(add)&01)
#define	SUPERFUN(f)	((int (*)())((long)(f) | 0x10000L))
#define	KERNELFUN(f)	((int (*)())((long)(f) & ~0x10000L))
#define	ISSUPERFUN(f)	((long)(f) & 0x10000L)

#include "../machine/seg.h"
#endif

#endif /* ENDIAN */
