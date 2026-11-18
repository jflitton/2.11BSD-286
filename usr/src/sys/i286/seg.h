/*
 * Segments: the layout of the global descriptor table.  mch_defs.inc says
 * the same to the assembler; change both.
 *
 * The kernel's own segments have fixed places in the table, because
 * tools/ne2aout.py writes their selectors into the kernel when it is
 * linked (--gdt GDT_KERNEL): segment N of the executable, counting from 1,
 * is slot GDT_KERNEL + N - 1.
 */
#define	GDT_NULL	0	/* the processor wants this one empty */
#define	GDT_TSS		1	/* the task state segment */
#define	GDT_LDT		2	/* the running process's descriptor table */
				/* 3 to 15: kept for later */
#define	GDT_KERNEL	16	/* the first of the kernel's segments */
#define	NKSEG		16	/* how many of them there may be */
#define	NGDT		(GDT_KERNEL + NKSEG)

#define	SEL(slot)	((slot) << 3)	/* slot to selector, ring 0 */

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
#define	SD_CODE		0x9B		/* ring 0 code, readable */
#define	SD_DATA		0x93		/* ring 0 data, writable */
#define	SD_TSS		0x81		/* task state segment, not busy */
#define	SD_LDT		0x82		/* local descriptor table */
#define	SD_INTGATE	0x86		/* gate that turns interrupts off */
#define	SD_TRAPGATE	0x87		/* gate that leaves them as they were */

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

#ifdef KERNEL
extern struct segdesc	gdt[NGDT];
extern struct tss	ktss;
#endif
