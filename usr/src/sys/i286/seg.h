/*
 * Segments: the global descriptor table, a process's segments, and how
 * the kernel reaches memory outside its own data segment.
 * mch_defs.inc says some of this to the assembler; change both.
 */
#ifndef _SEG_
#define	_SEG_

/*
 * The global descriptor table.  The kernel's own segments have fixed
 * places in it, because tools/ne2aout.py writes their selectors into the
 * kernel when it is linked (--gdt GDT_KERNEL): segment N of the
 * executable, counting from 1, is slot GDT_KERNEL + N - 1.
 */
#define	GDT_NULL	0	/* the processor wants this one empty */
#define	GDT_TSS		1	/* the task state segment */
#define	GDT_LDT		2	/* the running process's descriptor table */
#define	GDT_TMP1	3	/* copy and clear point these two at the */
#define	GDT_TMP2	4	/*   memory they are working on */
#define	GDT_BIO		5	/* what a disk driver is moving data to or from */
				/* 5 to 15: kept for later */
#define	GDT_KERNEL	16	/* the first of the kernel's segments */
#define	NKSEG		16	/* how many of them there may be */
#define	GDT_DYN		(GDT_KERNEL + NKSEG)	/* handed out by gdtalloc */
#define	NGDT		64

#define	SEL(slot)	((slot) << 3)	/* slot to selector, ring 0 */

/*
 * A process's segments are in its own local descriptor table, and their
 * selectors are written into the program when it is linked: segment N of
 * the executable, counting from 1, is slot N.  Slot 0 is not used.
 */
#define	NSEG		32	/* slots in a process's table */
#define	LSEL(slot)	(((slot) << 3) | 7)	/* in the LDT, ring 3 */
#define	LSLOT(sel)	((unsigned)(sel) >> 3)
#define	ISLSEL(sel)	(((sel) & 7) == 7)	/* could be a process's own */

/*
 * An address in a process: a selector and an offset.  The kernel's own
 * pointers are near, so this is the only kind that can hold one.  It is
 * not for dereferencing: copyin and copyout do that, with the process's
 * privilege and ready for the fault.  A kernel address converts to one by
 * assignment, which is what lets a struct uio point at either.
 */
typedef	char __far	*uaddr_t;
#define	UADDR(sel, off)	((uaddr_t)(((unsigned long)(sel) << 16) | (unsigned)(off)))
#define	USEL(ua)	((unsigned)((unsigned long)(ua) >> 16))
#define	UOFF(ua)	((unsigned)(unsigned long)(ua))

/*
 * A segment descriptor as the 286 has it.
 */
struct segdesc {
	unsigned short	sd_limit;	/* size in bytes, less one */
	unsigned short	sd_base;	/* physical address, low 16 bits */
	unsigned char	sd_basehi;	/* and bits 16 to 23 */
	unsigned char	sd_access;
	unsigned short	sd_zero;	/* must be zero for the 386's sake */
};

/* sd_access */
#define	SD_P		0x80		/* present */
#define	SD_DPL		0x60		/* ring that may use it */
#define	SD_S		0x10		/* code or data, not a system segment */
#define	SD_EXEC		0x08		/* with SD_S: code */
#define	SD_CODE		0x9B		/* ring 0 code, readable */
#define	SD_DATA		0x93		/* ring 0 data, writable */
#define	SD_UCODE	0xFB		/* ring 3 code, readable */
#define	SD_UDATA	0xF3		/* ring 3 data, writable */
#define	SD_TSS		0x81		/* task state segment, not busy */
#define	SD_LDT		0x82		/* local descriptor table */
#define	SD_INTGATE	0x86		/* gate that turns interrupts off */
#define	SD_TRAPGATE	0x87		/* gate that leaves them as they were */

/*
 * A crash dump (machdep.c, dumpsys) is the memory from address 0, the
 * hole from 640K to 1M with it, so that where a byte is in the dump is
 * its address.  The kernel has no use for the first bytes, the vectors
 * of real mode, and this is put there first, as the PDP-11's kernel
 * puts its registers in low memory for a dump.  A symbol of the kernel's
 * segment N is in the dump at the base of the descriptor in slot
 * GDT_KERNEL + N - 1 of the table there, plus its offset (savecore, adb).
 */
struct dumphdr {
	long	dh_magic;	/* DH_MAGIC */
	long	dh_gdt;		/* the global descriptor table's address */
	long	dh_size;	/* bytes in the dump */
	label_t	dh_regs;	/* dumpsys's registers, as setjmp keeps
				   them: the frame there is where adb's $c
				   starts */
};
#define	DH_MAGIC	0x55AA0286L

/*
 * The task state segment.  The kernel has one and never switches tasks;
 * the processor takes the kernel stack pointer from it when a process
 * traps into the kernel.
 */
struct tss {
	unsigned short	ts_link;
	unsigned short	ts_sp0, ts_ss0;	/* stack for ring 0 */
	unsigned short	ts_sp1, ts_ss1;
	unsigned short	ts_sp2, ts_ss2;
	unsigned short	ts_ip, ts_flags;
	unsigned short	ts_ax, ts_cx, ts_dx, ts_bx;
	unsigned short	ts_sp, ts_bp, ts_si, ts_di;
	unsigned short	ts_es, ts_cs, ts_ss, ts_ds;
	unsigned short	ts_ldt;
};

/*
 * What the kernel keeps for each slot of a process's descriptor table.
 * A process's memory is this list of segments.  The code segments of a
 * program are shared: they lie one after another in the memory of its
 * text structure (text.h), and ps_addr is where in it.  Every other
 * segment is the process's own piece of memory.
 */
struct pseg {
	unsigned short	ps_addr;	/* where it is in memory, in clicks */
	unsigned short	ps_swap;	/* where on the swap device, while
					   the process is swapped out */
	unsigned short	ps_limit;	/* size in bytes, less one */
	unsigned char	ps_flag;
	unsigned char	ps_size;	/* size in clicks */
};

/* ps_flag */
#define	PS_USED		0x01		/* the slot holds a segment */
#define	PS_TEXT		0x02		/* shared code, part of p_textp */
#define	PS_WTEXT	0x04		/* code that is writable for now:
					   being loaded, or traced */
#define	PS_CODE		0x08		/* the process's own, but code */
#define	PS_ALLOC	0x10		/* asked for with segalloc, and not
					   one the program came with */
#define	PS_MORE		0x20		/* with PS_ALLOC: the next slot goes
					   on with the same piece of memory,
					   which is more than 64K */

#if defined(KERNEL) && !defined(LOCORE)
extern struct segdesc	gdt[NGDT];
extern struct tss	ktss;

/*
 * The tables of every process's segments and descriptors, NSEG entries a
 * process, in segments of the kernel's of their own (param.c).
 */
extern struct pseg __far	pseg[];
extern struct segdesc __far	ldt[];
#define	PSEG(p)		(&pseg[((p) - proc) * NSEG])
#define	PLDT(p)		(&ldt[((p) - proc) * NSEG])

/*
 * Memory outside the kernel's data segment is reached through far
 * pointers.  A buffer's data is at the far pointer in its header; nothing
 * is mapped in or out.
 */
#define	mapin(bp)	((bp)->b_kaddr)
#define	mapout(bp)

/*
 * These take far pointers, so every caller must have seen these
 * declarations: param.h includes this file for that reason.  The
 * address in the process is the from of copyin and copyinstr, the to of
 * copyout and copyoutstr, and the addr of the fetches and stores.
 * bcopy, bzero and bcmp are for memory of the kernel's own, in its data
 * segment or out of it.
 */
int	copyin(void __far *from, void __far *to, unsigned n);
int	copyout(void __far *from, void __far *to, unsigned n);
int	copyinstr(void __far *from, void __far *to, unsigned max,
	    unsigned *done);
int	copyoutstr(void __far *from, void __far *to, unsigned max,
	    unsigned *done);
int	copystr(void __far *from, void __far *to, unsigned max,
	    unsigned *done);
int	fubyte(void __far *addr);
int	fuword(void __far *addr);
int	subyte(void __far *addr, int c);
int	suword(void __far *addr, int w);
void	bcopy(void __far *from, void __far *to, unsigned n);
void	bzero(void __far *p, unsigned n);
int	bcmp(void __far *a, void __far *b, unsigned n);
long	fartophys(void __far *p);
void	pcopy(long from, long to, unsigned count);
void	pclear(long addr, unsigned count);
void	pinsw(int port, long addr, unsigned count);
void	poutsw(int port, long addr, unsigned count);
void	insb(int port, void __far *buf, unsigned count);
void	insw(int port, void __far *buf, unsigned count);
void	outsb(int port, void __far *buf, unsigned count);
void	outsw(int port, void __far *buf, unsigned count);
unsigned gdtalloc(long paddr, unsigned limit);
#endif

#endif /* _SEG_ */
