/*
 * What the boot program hands the kernel: where it loaded each of the
 * kernel's segments.  The diskette's boot sector has the table in
 * itself (../i286stand/fdboot.s), and /boot makes one
 * (../i286stand/boot.c); docs/boot.md describes it.
 */
#define	BOOTMAGIC	0x0286
#define	BOOTNSEG	16

struct bootseg {
	unsigned short	bs_para;	/* where, as a real-mode segment */
	unsigned short	bs_nsect;	/* sectors read */
	unsigned short	bs_limit;	/* size in memory, less one */
	unsigned short	bs_type;	/* 1 code, 2 data */
};

struct bootinfo {
	unsigned short	bi_magic;
	unsigned short	bi_nseg;	/* number of segments */
	unsigned short	bi_eseg;	/* entry point: segment, from 1 */
	unsigned short	bi_eoff;	/*		offset */
	unsigned short	bi_dseg;	/* the data segment, from 1 */
	unsigned short	bi_drive;	/* the BIOS's drive number */
	unsigned short	bi_rdsect;	/* a RAM disk image on that drive: */
	unsigned short	bi_rdsize;	/*   first sector, and how many; or
					   with no first sector, one the
					   boot program has put above 1M */
	unsigned short	bi_root;	/* the device the kernel came from, as
					   the kernel numbers them, or 0 if
					   the boot program cannot say */
	struct bootseg	bi_seg[BOOTNSEG];
};

#ifdef KERNEL
extern struct bootinfo	bootinfo;
extern unsigned short	basemem;	/* K of memory below 640K */
extern unsigned short	extmem;		/* K of memory above 1M */
#endif
