/*
 * The process in tests/user: a small-model program in ring 3.  It checks
 * what a process can do (system calls, arithmetic, the 287, being
 * interrupted) and that everything it must not do arrives in the kernel
 * as the right exception.  k_main.c is the kernel under it.
 */
#define	EFAULT	14
#define	EINVAL	22

#define	UCODE	0x0F		/* this program's code segment */
#define	UDATA	0x17		/* and its data segment */
#define	KFIRST	0x80		/* the kernel's first segment */

extern int	errno;
extern unsigned	lasterr;
extern long	ladd();

int	pass, fail;
char	buf[16];
long	la = 100000L, lb = 234567L, lc = 1000000L, ld = 7L;
double	da = 1.5, db = 2.25;

cnputc(c)
	char c;
{
	write(&c, 1);
}

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

/* A fault that was meant: the vector that came, and its error code. */
fault(vec, want, err, what)
	int vec, want;
	unsigned err;
	char *what;
{
	check(vec == want && lasterr == err, what);
	if (vec != want || lasterr != err)
		printf("  vector %d, error %x\n", vec, lasterr);
}

calls()
{
	int i;

	check(add(2, 3) == 5 && add(-7, 2) == -5, "system call with two arguments");
	check(sum6(1, 20, 300, 4000, 5, 60) == 4386, "with six, one in each register");
	check(sum8(1, 20, 300, 4000, 5, 60, 700, 8000) == 13086,
	    "with eight, passed by address");
	errno = 0;
	check(sum8at(0xFFF8, UDATA) == -1 && errno == EFAULT,
	    "with eight, at a bad address");
	check(kept(), "a system call preserves the other registers");
	check(ladd(la, lb) == 334567L, "system call with long arguments and result");
	errno = 0;
	check(nosuch() == -1 && errno == EINVAL, "system call that does not exist");

	for (i = 0; i < sizeof buf; i++)
		buf[i] = '.';
	check(fill(buf + 1, 9, 'x') == 9, "system call that writes to a buffer");
	for (i = 1; i < 10; i++)
		if (buf[i] != 'x')
			break;
	check(i == 10 && buf[0] == '.' && buf[10] == '.', "and what it wrote");
}

/* Addresses the kernel must refuse on the process's behalf. */
badaddr()
{
	unsigned sel;
	int n;

	errno = 0;
	check(write((char *)0xFFF0, 4) == -1 && errno == EFAULT,
	    "read past the end of the data segment");
	errno = 0;
	check(fill((char *)0xFFF0, 4, 0) == -1 && errno == EFAULT,
	    "write past the end of the data segment");
	errno = 0;
	readfar(0, UCODE, 4);
	check(errno == 0, "the code segment can be read");
	check(fillfar(0, UCODE, 4) == -1 && errno == EFAULT,
	    "the code segment cannot be written");
	errno = 0;
	check(readfar(0, 0x1F, 2) == -1 && errno == EFAULT,
	    "a segment the process does not have");

	/* Every selector in the kernel's table, as given and as ring 3. */
	n = 0;
	for (sel = 0; sel < 0x100; sel += 8) {
		if (readfar(0, sel, 2) != -1)
			n++;
		if (readfar(0, sel | 3, 2) != -1)
			n++;
		if (fillfar(0, sel, 2) != -1)
			n++;
		if (fillfar(0, sel | 3, 2) != -1)
			n++;
	}
	check(n == 0, "no kernel segment can be reached through a system call");
}

faults()
{
	fault(u_cli(), 13, 0, "CLI");
	fault(u_hlt(), 13, 0, "HLT");
	fault(u_inb(), 13, 0, "IN");
	fault(u_outb(), 13, 0, "OUT");
	fault(u_lmsw(), 13, 0, "LMSW");
	fault(u_loades(KFIRST), 13, KFIRST, "loading a kernel selector");
	fault(u_loades(KFIRST | 3), 13, KFIRST, "the same, claiming ring 3");
	fault(u_loades(UDATA), -1, lasterr, "loading its own data selector");
	fault(u_farcall(KFIRST), 13, KFIRST, "far call into the kernel");
	fault(u_int20(), 13, 0x20 * 8 + 2, "INT to a gate that is the kernel's");
	fault(u_int31(), 13, 0x31 * 8 + 2, "INT past the end of the table");
	fault(u_peek(0xFFF0), 13, 0, "reading past the end of the data segment");
	fault(u_pokecs(), 13, 0, "writing the code segment");
	fault(u_stack(), 12, 0, "stack pointer outside the segment");
	fault(u_div0(), 0, 0, "divide by zero");
	fault(u_bpt(), 3, 0, "breakpoint");
	fault(u_op386(), 6, 0, "a 386 instruction");

	check((setflags(getflags() & ~0x200 | 0x3000) & 0x3200) == 0x200,
	    "flags: interrupts stay on and I/O privilege stays 0");
}

clock()
{
	int i, t;

	t = uticks();
	for (i = 0; i < 50 && uticks() < t + 2; i++)
		spin();
	check(uticks() >= t + 2, "clock interrupts while in the process");
}

arith()
{
	double d;

	check(lc / ld == 142857L && lc % ld == 1 && la * ld == 700000L,
	    "long arithmetic");
	d = da * db;
	check(d == 3.375 && (int)(d * 1000) == 3375, "287 arithmetic");
}

main()
{
	printf("in the process\n");
	check(getcs() == UCODE && getds() == UDATA && getss() == UDATA,
	    "segment registers");
	check((getflags() & 0x3200) == 0x200, "interrupts on, I/O privilege 0");
	calls();
	badaddr();
	faults();
	clock();
	arith();
	printf("user: pass=%d fail=%d\n", pass, fail);
	return (fail != 0);
}
