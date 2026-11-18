/*
 * Test of the kernel's start-up and trap entry (sys/i286/mch_start.s,
 * mch_trap.s): this takes the place of the kernel's main and trap.  It
 * checks that the processor is in protected mode with the descriptor
 * tables as seg.h says, that each kind of protection fault arrives as the
 * right exception and can be returned from, and that memory above 1M,
 * the clock interrupt and the 287 work.
 */
#define	KERNEL
#include "seg.h"
#include "boot.h"
#include "frame.h"
#include "trap.h"

extern long	farmul();

int	pass, fail;
unsigned resume;		/* where a fault that was meant goes on from */
int	expect = -1;		/* the exception that is meant */
int	taken;			/* how many times it came */
unsigned lasterr;		/* and its error code */
unsigned ticks;			/* clock interrupts */
int	inton;			/* interrupts were on inside trap() */

long	la = 100000L, lb = 3L, lc = 1000000L, ld = 7L;
double	da = 1.5, db = 2.25;

check(ok, what)
	int ok;
	char *what;
{
	if (ok)
		pass++;
	else {
		fail++;
		printf("FAILED: %s\n", what);
	}
}

done(code)
	int code;
{
	printf("\n@exit %02x\n", code);
	for (;;) {
		disable();
		idle();
	}
}

trap(tf)
	register struct trapframe *tf;
{
	if (getflags() & 0x200)
		inton++;
	if (tf->tf_vec == T_IRQ0) {
		ticks++;
		outb(0x20, 0x20);	/* tell the controller it was handled */
		return;
	}
	if (tf->tf_vec == expect) {
		taken++;
		lasterr = tf->tf_err;
		if (expect == T_NOFPU)
			cleartask();	/* and run the instruction again */
		else
			tf->tf_ip = resume;
		return;
	}
	printf("unexpected trap %d, error %x, at %x:%x, flags %x\n",
	    tf->tf_vec, tf->tf_err, tf->tf_cs, tf->tf_ip, tf->tf_flags);
	printf("ax %x bx %x cx %x dx %x si %x di %x bp %x ds %x es %x\n",
	    tf->tf_ax, tf->tf_bx, tf->tf_cx, tf->tf_dx, tf->tf_si,
	    tf->tf_di, tf->tf_bp, tf->tf_ds, tf->tf_es);
	done(0xFF);
}

/* Nothing here makes a system call. */
syscall(tf)
	struct trapframe *tf;
{
	printf("unexpected system call at %x:%x\n", tf->tf_cs, tf->tf_ip);
	done(0xFF);
}

/* The next fault should be exception vec. */
want(vec)
	int vec;
{
	expect = vec;
	taken = 0;
	lasterr = 0xFFFF;
}

/* Was it, once, with this error code? */
got(err, what)
	unsigned err;
	char *what;
{
	check(taken == 1 && lasterr == err, what);
	if (taken != 1 || lasterr != err)
		printf("  taken %d times, error %x\n", taken, lasterr);
	expect = -1;
}

/* Make slot a ring 0 data segment of 64K at a physical address. */
setseg(slot, addr, access)
	int slot, access;
	long addr;
{
	register struct segdesc *sd = &gdt[slot];

	sd->sd_limit = 0xFFFF;
	sd->sd_base = (unsigned)addr;
	sd->sd_basehi = (unsigned)(addr >> 16);
	sd->sd_access = access;
	sd->sd_zero = 0;
}

struct tabptr {
	unsigned short	limit;
	unsigned short	base;
	unsigned short	basehi;
};

long
physaddr(off)
	unsigned off;
{
	return (((long)bootinfo.bi_seg[bootinfo.bi_dseg - 1].bs_para << 4) + off);
}

tables()
{
	struct tabptr tp;
	register int i;
	unsigned cs, ds, sel;
	long addr;

	check((getmsw() & 3) == 3, "protected mode, with a 287");
	cs = getcs();
	ds = getds();
	i = (cs >> 3) - GDT_KERNEL;
	check((cs & 7) == 0 && i >= 0 && i < bootinfo.bi_nseg &&
	    bootinfo.bi_seg[i].bs_type == 1, "CS is one of the code segments");
	check(ds == SEL(GDT_KERNEL + bootinfo.bi_dseg - 1), "DS is the data segment");
	check(getss() == ds, "SS is the data segment");

	getgdtr(&tp);
	addr = physaddr((unsigned)gdt);
	check(tp.limit == NGDT * 8 - 1 && tp.base == (unsigned)addr &&
	    (tp.basehi & 0xFF) == (unsigned)(addr >> 16), "GDT register");
	getidtr(&tp);
	check(tp.limit == NIDT * 8 - 1, "IDT register");
	check(gettr() == SEL(GDT_TSS), "task register");
	check(segaccess(SEL(GDT_TSS)) == (SD_TSS | 2), "task state segment is busy");
	check(ktss.ts_ss0 == ds && ktss.ts_sp0 >= getsp(), "ring 0 stack in the TSS");

	for (i = 0; i < bootinfo.bi_nseg; i++) {
		sel = SEL(GDT_KERNEL + i);
		check(seglimit(sel) == bootinfo.bi_seg[i].bs_limit, "segment limit");
		check(segaccess(sel) == (bootinfo.bi_seg[i].bs_type == 1 ?
		    SD_CODE : SD_DATA), "segment type");
	}
	check(seglimit(SEL(GDT_KERNEL + bootinfo.bi_nseg)) == -1,
	    "nothing after the last segment");

	check(bootinfo.bi_nseg >= 3, "more than one code segment");
	check(farcs() != cs && segaccess(farcs()) == SD_CODE,
	    "far call to another code segment");
}

arith()
{
	long l;
	double d;

	check(farmul(la, lb) == 300000L, "long multiply");
	check(lc / ld == 142857L && lc % ld == 1, "long divide");
	l = -lc;
	check(l / ld == -142857L && l % ld == -1, "signed long divide");
	d = da * db;			/* 3.375 */
	check(d == 3.375 && (int)(d * 1000) == 3375, "287 arithmetic");
	check((long)(d * la) == 337500L, "double to long");
}

faults()
{
	unsigned ds = getds(), cs = getcs();
	unsigned limit = bootinfo.bi_seg[bootinfo.bi_dseg - 1].bs_limit;
	double d;

	want(T_BPT);
	bpt();
	got(0, "breakpoint");

	want(T_DIVIDE);
	divzero();
	got(0, "divide by zero");

	want(T_BADOP);
	op386();
	got(0, "a 386 instruction is an invalid opcode");

	want(T_PROT);
	loades(0x1000);
	got(0x1000, "selector beyond the end of the GDT");

	want(T_PROT);
	loades(SEL(GDT_KERNEL + bootinfo.bi_nseg));
	got(SEL(GDT_KERNEL + bootinfo.bi_nseg), "selector of an empty descriptor");

	setseg(3, 0L, SD_DATA & ~SD_P);
	want(T_SEGNP);
	loades(SEL(3));
	got(SEL(3), "segment not present");

	want(T_PROT);
	peek(cs, 0);
	check(taken == 0, "code segment can be read");
	poke(cs, 0, 0);
	got(0, "code segment cannot be written");

	if (limit < 0xFFFE) {
		want(T_PROT);
		peek(ds, limit & ~1);
		check(taken == 0, "last word of the data segment can be read");
		peek(ds, limit + 1);
		got(0, "data segment ends at its limit");
	}

	settask();
	want(T_NOFPU);
	d = da + db;
	got(0, "287 instruction with the task-switched flag set");
	check(d == 3.75 && (getmsw() & 8) == 0, "and it was run again");
}

/*
 * Memory above 1M: there, and not the same memory as below 1M, which is
 * what it would be with address line 20 held low.
 */
highmem()
{
	long top;

	printf("memory: %uK below 640K, %uK above 1M\n", basemem, extmem);
	check(basemem >= 512 && basemem <= 640, "base memory size");
	check(extmem >= 1024, "extended memory size");
	if (extmem < 1024)
		return;

	setseg(3, 0L, SD_DATA);		/* the first 64K: vectors no longer used */
	setseg(4, 0x100000L, SD_DATA);	/* the 64K at 1M */
	want(T_PROT);
	poke(SEL(3), 0, 0x1111);
	poke(SEL(4), 0, 0x2222);
	check(peek(SEL(3), 0) == 0x1111 && peek(SEL(4), 0) == 0x2222,
	    "memory at 1M is distinct from memory at 0");

	top = 0x100000L + ((long)extmem << 10) - 0x10000L;
	setseg(4, top, SD_DATA);	/* the last 64K */
	poke(SEL(4), 0xFFFE, 0x55AA);
	check(peek(SEL(4), 0xFFFE) == 0x55AA, "top of extended memory");
	poke(SEL(4), 0xFFFE, 0xAA55);
	check(peek(SEL(4), 0xFFFE) == 0xAA55, "top of extended memory, again");
	check(taken == 0, "no faults reaching it");
	expect = -1;
	setseg(3, 0L, 0);
	setseg(4, 0L, 0);
}

clock()
{
	inton = 0;
	outb(0x21, 0xFE);		/* let request 0, the timer, through */
	enable();
	while (ticks < 3)
		idle();
	disable();
	outb(0x21, 0xFF);
	check(ticks >= 3, "clock interrupts");
	check(inton == 0, "interrupts are off inside trap");
	check(getflags() & 0x200 ? 0 : 1, "and off again afterwards");
}

main()
{
	cninit();
	printf("\n2.11BSD/286 start-up test\n");
	printf("%d segments, data at %x:0, entered at %x:%x\n", bootinfo.bi_nseg,
	    bootinfo.bi_seg[bootinfo.bi_dseg - 1].bs_para,
	    bootinfo.bi_eseg, bootinfo.bi_eoff);
	tables();
	arith();
	faults();
	highmem();
	clock();
	printf("pm: pass=%d fail=%d\n", pass, fail);
	done(fail ? 1 : 0);
}
