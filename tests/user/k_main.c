/*
 * The kernel half of tests/user: in place of the kernel's main, trap and
 * syscall.  It loads the program in u_test.c into memory above 1M as a
 * process with a local descriptor table of its own, checks copyin and
 * copyout against it, and runs it in ring 3.  The process makes system
 * calls and faults on purpose; this counts what arrives and checks that
 * every entry from the process finds the kernel stack empty.
 */
#define	KERNEL
#include "seg.h"
#include "boot.h"
#include "frame.h"
#include "trap.h"
#include "psl.h"

#define	EFAULT	14
#define	EINVAL	22

/* The executable's header and segment table: docs/executable-format.md. */
struct xhdr {
	unsigned short	x_magic, x_model, x_nseg, x_eseg, x_eoff, x_dseg;
	unsigned short	x_stack, x_resv;
	unsigned long	x_syms, x_strs;
};
struct xseg {
	unsigned short	xs_type, xs_resv;
	unsigned long	xs_filesz, xs_memsz;
};

#define	NLDT	8
struct segdesc	ldt[NLDT];

extern unsigned char userprog[];
extern unsigned	nofault;

int	pass, fail;
unsigned resume;		/* for pm_asm.s; nothing here uses it */
unsigned ucsel, udsel;		/* the process's code and data selectors */
unsigned ulimit;		/* the limit of its data segment */
unsigned ticks, uticks;		/* clock interrupts; those in the process */
unsigned nsys, ufaults, kfaults;
int	badframe;		/* entries that found the stack not empty */
int	sysoff;			/* system calls entered with interrupts off */
int	victim = 0x1234;
char	zeros[64];

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

finish(code)
	int code;
{
	disable();
	outb(0x21, 0xFF);
	check(badframe == 0, "every entry from the process found the kernel stack empty");
	check(sysoff == 0, "system calls run with interrupts on");
	check(nofault == 0, "nofault is clear");
	printf("kernel: pass=%d fail=%d; %u system calls, %u faults from the process\n",
	    pass, fail, nsys, ufaults);
	printf("\n@exit %02x\n", code ? code : fail != 0);
	for (;;) {
		disable();
		idle();
	}
}

long
physaddr(off)
	unsigned off;
{
	return (((long)bootinfo.bi_seg[bootinfo.bi_dseg - 1].bs_para << 4) + off);
}

setdesc(sd, addr, limit, access)
	register struct segdesc *sd;
	long addr;
	unsigned limit;
	int access;
{
	sd->sd_limit = limit;
	sd->sd_base = (unsigned)addr;
	sd->sd_basehi = (unsigned)(addr >> 16);
	sd->sd_access = access;
	sd->sd_zero = 0;
}

/*
 * Make the process: a descriptor in its table for each segment of the
 * program, the memory for them one after another from 1M, and the
 * program's contents copied in.  The stack goes after the data, in the
 * same segment.  Returns the process's first stack pointer.
 */
load()
{
	register struct xhdr *xh = (struct xhdr *)userprog;
	register struct xseg *xs = (struct xseg *)(xh + 1);
	unsigned char *p = (unsigned char *)(xs + xh->x_nseg);
	long mem = 0x100000L;
	unsigned size, off, n, sel, usp = 0;
	int i, error = 0;

	check(xh->x_magic == 0x0286 && xh->x_nseg < NLDT, "the program's header");
	setdesc(&gdt[GDT_LDT], physaddr((unsigned)ldt), sizeof ldt - 1, SD_LDT);
	lldt(SEL(GDT_LDT));
	for (i = 1; i <= xh->x_nseg; i++, xs++) {
		size = xs->xs_memsz;
		if (i == xh->x_dseg) {
			size = (size + xh->x_stack + 1) & ~1;
			usp = size;
			ulimit = size - 1;
		}
		sel = LSEL(i);
		/* As data while it is filled; code cannot be written. */
		setdesc(&ldt[i], mem, size - 1, SD_UDATA);
		error |= copyout(p, UADDR(sel, 0), (unsigned)xs->xs_filesz);
		for (off = xs->xs_filesz; off < size; off += n) {
			n = size - off < sizeof zeros ? size - off : sizeof zeros;
			error |= copyout(zeros, UADDR(sel, off), n);
		}
		if (xs->xs_type == 1)
			ldt[i].sd_access = SD_UCODE;
		p += (xs->xs_filesz + 1) & ~1;
		mem += ((long)size + 1) & ~1;
	}
	ucsel = LSEL(xh->x_eseg);
	udsel = LSEL(xh->x_dseg);
	check(error == 0 && usp != 0, "the program loaded");
	return (usp);
}

/* copyin and copyout, from the kernel's side. */
copies()
{
	static char out[] = "abcdefg", in[8];
	char two[2];
	int i;

	check(copyout(out, UADDR(udsel, ulimit - 8), 7) == 0 &&
	    copyin(UADDR(udsel, ulimit - 8), in, 7) == 0, "copy out and back in");
	for (i = 0; i < 7 && in[i] == out[i]; i++)
		;
	check(i == 7 && in[7] == 0, "what came back");
	check(copyin(UADDR(udsel, ulimit - 7), in, 8) == 0,
	    "copy in the last bytes of a segment");
	check(copyin(UADDR(udsel, ulimit - 6), in, 8) == EFAULT,
	    "copy in across the end of a segment");
	check(copyout(out, UADDR(udsel, ulimit), 2) == EFAULT,
	    "copy out across the end of a segment");
	check(copyin(UADDR(udsel, 0xFFFF), in, 2) == EFAULT, "offset that wraps");
	check(copyin(UADDR(ucsel, 0), in, 4) == 0, "copy in from code");
	check(copyout(out, UADDR(ucsel, 0), 4) == EFAULT, "copy out to code");
	check(copyin(UADDR(getds(), (unsigned)&victim), two, 2) == EFAULT,
	    "copy in from the kernel's data");
	check(copyout(out, UADDR(getds(), (unsigned)&victim), 2) == EFAULT &&
	    victim == 0x1234, "copy out to the kernel's data");
	check(copyin(UADDR(getcs(), 0), two, 2) == EFAULT, "copy in from the kernel's code");
	check(copyin(UADDR(0, 0), two, 2) == EFAULT, "null selector");
	check(copyin(UADDR(LSEL(NLDT - 1), 0), two, 2) == EFAULT, "empty descriptor");
	check(copyin(UADDR(LSEL(NLDT), 0), two, 2) == EFAULT,
	    "selector past the end of the table");
	check(kfaults == 10 && nofault == 0, "each of those was one fault in the kernel");
}

trap(tf)
	register struct trapframe *tf;
{
	register int vec = tf->tf_vec;

	if (vec == T_IRQ0) {
		ticks++;
		if (USERMODE(tf->tf_cs))
			uticks++;
		outb(0x20, 0x20);
		return;
	}
	if (USERMODE(tf->tf_cs)) {
		if ((unsigned)(tf + 1) != ktss.ts_sp0)
			badframe++;
		if (tf->tf_cx == 0xFEED) {	/* meant: see u_start.s */
			ufaults++;
			tf->tf_ax = vec;
			tf->tf_dx = tf->tf_err;
			tf->tf_ip = tf->tf_bx;
			tf->tf_cx = 0;
			return;
		}
		printf("unexpected trap %d in the process, error %x, at %x:%x\n",
		    vec, tf->tf_err, tf->tf_cs, tf->tf_ip);
		printf("ax %x bx %x cx %x dx %x sp %x:%x ds %x es %x\n",
		    tf->tf_ax, tf->tf_bx, tf->tf_cx, tf->tf_dx, tf->tf_ss,
		    tf->tf_sp, tf->tf_ds, tf->tf_es);
		finish(0xFE);
	}
	if (nofault && (vec == T_PROT || vec == T_SEGNP || vec == T_STACK)) {
		kfaults++;
		tf->tf_ip = nofault;
		return;
	}
	printf("unexpected trap %d in the kernel, error %x, at %x:%x\n",
	    vec, tf->tf_err, tf->tf_cs, tf->tf_ip);
	finish(0xFF);
}

#define	NSYS	10

/*
 * The arguments are in the frame: the process passed them in registers.
 * A pointer is two of them, the offset and then the selector.
 */
syscall(tf)
	register struct trapframe *tf;
{
	register unsigned *args = tf_arg(tf);
	unsigned code = tf->tf_ax, n, i, off;
	unsigned more[8];
	char kbuf[64];
	int error = 0;
	long val = 0;

	nsys++;
	if (!USERMODE(tf->tf_cs) || (unsigned)(tf + 1) != ktss.ts_sp0)
		badframe++;
	if ((getflags() & PSL_I) == 0)
		sysoff++;
	if (code >= NSYS) {
		error = EINVAL;
		goto out;
	}

	switch (code) {
	case 0:			/* exit(code) */
		finish(args[0]);
	case 1:			/* write(buf, n) */
		for (off = 0; off < args[2] && !error; off += n) {
			n = args[2] - off < sizeof kbuf ? args[2] - off : sizeof kbuf;
			error = copyin(UADDR(args[1], args[0] + off), kbuf, n);
			for (i = 0; i < n && !error; i++)
				cnputc(kbuf[i]);
		}
		val = args[2];
		break;
	case 2:			/* add(a, b) */
		val = (int)(args[0] + args[1]);
		break;
	case 3:			/* ladd(a, b), longs */
		val = *(long *)&args[0] + *(long *)&args[2];
		break;
	case 4:			/* fill(buf, n, c) */
		for (i = 0; i < sizeof kbuf; i++)
			kbuf[i] = args[3];
		for (off = 0; off < args[2] && !error; off += n) {
			n = args[2] - off < sizeof kbuf ? args[2] - off : sizeof kbuf;
			error = copyout(kbuf, UADDR(args[1], args[0] + off), n);
		}
		val = args[2];
		break;
	case 5:			/* uticks() */
		val = uticks;
		break;
	case 6:			/* readfar(offset, selector, n): sum of the bytes */
		n = args[2] < sizeof kbuf ? args[2] : sizeof kbuf;
		error = copyin(UADDR(args[1], args[0]), kbuf, n);
		for (i = 0; i < n && !error; i++)
			val += kbuf[i] & 0xFF;
		break;
	case 7:			/* fillfar(offset, selector, n) */
		n = args[2] < sizeof kbuf ? args[2] : sizeof kbuf;
		for (i = 0; i < n; i++)
			kbuf[i] = 0x5A;
		error = copyout(kbuf, UADDR(args[1], args[0]), n);
		break;
	case 8:			/* sum6(a, b, c, d, e, f) */
		for (i = 0; i < NARGREG; i++)
			val += args[i];
		val = (unsigned)val;
		break;
	case 9:			/* sum8(pointer to eight words) */
		error = copyin(UADDR(args[1], args[0]), (char *)more, sizeof more);
		for (i = 0; i < 8 && !error; i++)
			val += more[i];
		val = (unsigned)val;
		break;
	}
out:
	if (error) {
		tf->tf_ax = error;
		tf->tf_flags |= PSL_C;
	} else {
		tf->tf_ax = (unsigned)val;
		tf->tf_dx = (unsigned)(val >> 16);
		tf->tf_flags &= ~PSL_C;
	}
}

main()
{
	register struct trapframe *tf;
	register int i;
	unsigned usp;

	cninit();
	printf("\n2.11BSD/286 process test\n");
	usp = load();
	copies();

	/* The process's frame, as if it had been interrupted at its start. */
	tf = (struct trapframe *)ktss.ts_sp0 - 1;
	for (i = 0; i < sizeof *tf; i++)
		((char *)tf)[i] = 0;
	tf->tf_cs = ucsel;
	tf->tf_ip = ((struct xhdr *)userprog)->x_eoff;
	tf->tf_flags = PSL_USERSET;
	tf->tf_ss = tf->tf_ds = tf->tf_es = udsel;
	tf->tf_sp = usp;
	outb(0x21, 0xFE);		/* the clock */
}
