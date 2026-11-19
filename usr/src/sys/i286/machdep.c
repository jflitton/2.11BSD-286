/*
 * Machine-dependent parts of the kernel that are not the trap handler:
 * start-up, memory outside the kernel's data segment, a process's
 * descriptors, slots and the switch between processes, interrupts and
 * their priority, the 287, signal delivery, and stopping.
 */
#include "param.h"
#include "../machine/psl.h"
#include "../machine/reg.h"
#include "../machine/frame.h"
#include "../machine/trap.h"
#include "../machine/boot.h"
#include "signalvar.h"
#include "user.h"
#include "proc.h"
#include "buf.h"
#include "map.h"
#include "text.h"
#include "inode.h"
#include "file.h"
#include "fs.h"
#include "mount.h"
#include "clist.h"
#include "callout.h"
#include "reboot.h"
#include "systm.h"
#include "kernel.h"
#include "namei.h"
#include "vm.h"
#include "ioctl.h"
#include "tty.h"
#include "sysctl.h"
#include "../machine/cpu.h"

struct user *uptr;		/* the running process's user structure */
/*
 * Where in a user structure the name of the program is.  Nothing in the
 * kernel uses it: a profiler outside the machine does, which reads the
 * running program's name from memory at each sample (tools/kprof.py).
 */
int	ucomm = (int)((struct user *)0)->u_comm;
extern char uslot[][UBYTES];	/* param.c */
extern struct proc *slotproc[];	/* who is in each slot */
extern int nslot;

size_t	physmem;		/* clicks of memory in the machine */
extern memaddr bpaddr;		/* where the buffers' data is */
dev_t	bootdev;		/* what the system was loaded from */
extern dev_t diskroot;		/* conf.c */
u_int	extlimit = 0;		/* (patchable) if not 0, the most memory
				   above 1M to use, in K: for testing
				   with less than the machine has */
caddr_t	bootcsr;		/* not used: a PDP-11 device address */
long	kdbase;			/* physical address of the kernel's data */
long	ldtbase;		/* and of every process's descriptor tables */
struct proc *ldtproc;		/* whose table the processor has loaded */
struct proc *fpproc;		/* whose numbers are in the 287 */

/*
 * Machine dependent startup code
 */
startup()
{
	register int i;
	register struct bootseg *bs;
	memaddr first, last, rdinit();
	long end, e;

	cninit();
	printf("\n%s\n", version);
	/*
	 * The root is the RAM disk the boot program loaded; or the disk
	 * it says it loaded the kernel from; or, when it does not say,
	 * diskroot.
	 */
	if (bootinfo.bi_rdsize)
		bootdev = makedev(0, 0);
	else if (bootinfo.bi_root)
		bootdev = bootinfo.bi_root;
	else
		bootdev = diskroot;
	/* (a null pointer would not do: converted to far it is still null) */
	kdbase = fartophys(&kdbase) - (u_int)&kdbase;
	ldtbase = fartophys(ldt);

	/*
	 * Memory: what is left below 640K after the kernel, which the
	 * boot program loaded from 64K up, and all of it above 1M.  The
	 * first 4K is left alone; the BIOS keeps its notes there, and
	 * click 0 could not be in the map anyway.
	 */
	end = 0;
	for (bs = bootinfo.bi_seg, i = 0; i < bootinfo.bi_nseg; i++, bs++) {
		e = ((long)bs->bs_para << 4) + bs->bs_limit + 1;
		if (e > end)
			end = e;
	}
	if (extlimit && extmem > extlimit)
		extmem = extlimit;
	first = btoc(end);
	last = (memaddr)basemem * 2;
	mfree(coremap, bootinfo.bi_seg[0].bs_para / 32 - 8, 8);
	mfree(coremap, last - first, first);
	if (extmem) {
		first = rdinit((memaddr)2048);
		mfree(coremap, 2048 + (memaddr)extmem * 2 - first, first);
	}
	physmem = extmem ? 2048 + (memaddr)extmem * 2 : last;
	maxmem = MAXMEM;

	procNPROC = proc + nproc;
	textNTEXT = text + ntext;
	inodeNINODE = inode + ninode;
	fileNFILE = file + nfile;

	/*
	 * The buffers' data.  binit gives each 64K of it a segment.
	 */
	if ((bpaddr = malloc(coremap, btoc((long)nbuf * MAXBSIZE))) == 0)
		panic("buffers");

	/*
	 * The first slot is the swapper's, and it is running in it.
	 */
	slotproc[0] = &proc[0];

	loginit();

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];
}

/*
 * The physical address of something the kernel has a far pointer to.
 */
long
fartophys(p)
	void __far *p;
{
	register struct segdesc *dp;

	dp = &gdt[USEL((uaddr_t)p) >> 3];
	return (((long)dp->sd_basehi << 16 | dp->sd_base) + UOFF((uaddr_t)p));
}

/*
 * Give a piece of memory a segment of the kernel's, for good: limit is
 * its size less one.  Returns the selector.
 */
u_int
gdtalloc(paddr, limit)
	long paddr;
	u_int limit;
{
	static int next = GDT_DYN;
	register struct segdesc *dp;

	if (next >= NGDT)
		panic("gdtalloc");
	dp = &gdt[next];
	dp->sd_limit = limit;
	dp->sd_base = paddr;
	dp->sd_basehi = paddr >> 16;
	dp->sd_access = SD_DATA;
	return (SEL(next++));
}

/*
 * Copy count clicks of memory from one place to another, and clear
 * count clicks.  The places are click numbers, and must not overlap.
 */
copy(from, to, count)
	memaddr from, to;
	register u_int count;
{
	register u_int n;

	while (count) {
		n = count > 64 ? 64 : count;
		pcopy(ctob((long)from), ctob((long)to), n * NBPG);
		from += n;
		to += n;
		count -= n;
	}
}

clear(addr, count)
	memaddr addr;
	register u_int count;
{
	register u_int n;

	while (count) {
		n = count > 64 ? 64 : count;
		pclear(ctob((long)addr), n * NBPG);
		addr += n;
		count -= n;
	}
}

/*
 * A process's descriptors.  Its segments are listed in its part of pseg;
 * setldt makes the descriptor for one of them say what the list does,
 * and sureg does that for all of the running process's.  They are called
 * whenever a segment has been made, moved, resized or removed, which is
 * also when the processor's own copy of a descriptor goes out of date:
 * that copy is replaced when the segment register is next loaded, and
 * every return to the process loads all four.
 */
setldt(p, n)
	register struct proc *p;
	int n;
{
	register struct pseg __far *sp;
	register struct segdesc __far *dp;
	long base;

	sp = &PSEG(p)[n];
	dp = &PLDT(p)[n];
	if ((sp->ps_flag & PS_USED) == 0) {
		dp->sd_access = 0;
		return;
	}
	if (sp->ps_flag & PS_TEXT) {
		base = ctob((long)(p->p_textp->x_caddr + sp->ps_addr));
		dp->sd_access = sp->ps_flag & PS_WTEXT ? SD_UDATA : SD_UCODE;
	} else {
		base = ctob((long)sp->ps_addr);
		dp->sd_access = sp->ps_flag & PS_CODE ? SD_UCODE : SD_UDATA;
	}
	dp->sd_limit = sp->ps_limit;
	dp->sd_base = base;
	dp->sd_basehi = base >> 16;
	dp->sd_zero = 0;
}

sureg()
{
	register int n;

	for (n = 1; n < NSEG; n++)
		setldt(u.u_procp, n);
}

/*
 * Slots.  A process's user structure and kernel stack are in the
 * kernel's data segment while it runs, in the slot the process was
 * given when it was made (uslot, in param.c).  The kernel stack holds
 * pointers into itself and into the user structure, so a process can
 * never run anywhere but in its own slot.  Several processes may have
 * been given the same one: then the one whose turn it is is copied in
 * from where p_addr says, after the one that was there is copied out.
 * Processes with different slots switch without any copying, and the
 * swapper has the first slot to itself.
 *
 * uswitch is called by longjmp to make process p the running one.  Its
 * caller is about to continue from the label that is off bytes into p's
 * user structure, which says how much of p's stack is in use.  Either p
 * or the process calling is the swapper: swtch always goes by way of it.
 * That is what makes it safe to copy over a slot here.  If the slot is
 * to be copied over, it is not the swapper's, so the stack this is
 * running on is not in it.
 */
uswitch(p, off)
	register struct proc *p;
	u_int off;
{
	register struct user *nu;
	int s;
	long base;

	if (p == u.u_procp)
		return;
	s = p->p_slot;
	nu = (struct user *)uslot[s];
	if (slotproc[s] != p) {
		if (slotproc[s])
			usave(slotproc[s]);
		uload(p, off);
		slotproc[s] = p;
	}
	s = intsoff();
	uptr = nu;
	ktss.ts_sp0 = (u_int)nu + UBYTES;
	if (p != &proc[0]) {
		if (p != ldtproc) {
			base = ldtbase + (long)(p - proc) *
			    (NSEG * sizeof (struct segdesc));
			gdt[GDT_LDT].sd_limit =
			    NSEG * sizeof (struct segdesc) - 1;
			gdt[GDT_LDT].sd_base = base;
			gdt[GDT_LDT].sd_basehi = base >> 16;
			gdt[GDT_LDT].sd_access = SD_LDT;
			lgdtldt();
			ldtproc = p;
		}
		if (p == fpproc)
			fpclts();
		else
			fpts();
	}
	intson(s);
}

/*
 * How far into a slot the used part of the stack begins, for a process
 * that will continue from the given label.
 */
static u_int
ustack(su, lp)
	register struct user *su;
	label_t *lp;
{
	register u_int sp;

	sp = lp->val[3];
	if (sp < (u_int)su + sizeof (struct user) || sp > (u_int)su + UBYTES)
		panic("ustack");
	return ((sp - (u_int)su) & ~1);
}

/*
 * Copy process q out of its slot.  It left by way of swtch, so it will
 * continue from u_rsave or from somewhere higher on the stack.
 */
usave(q)
	register struct proc *q;
{
	register struct user *su;
	register u_int off;
	long kb, base;

	su = (struct user *)uslot[q->p_slot];
	if (q == fpproc)
		fpstore(q);
	kb = kdbase + (u_int)su;
	base = ctob((long)q->p_addr);
	off = ustack(su, &su->u_rsave);
	pcopy(kb, base, sizeof (struct user));
	pcopy(kb + off, base + off, UBYTES - off);
}

/*
 * Copy process p into its slot, to continue from the label off bytes
 * into its user structure.
 */
uload(p, off)
	register struct proc *p;
	u_int off;
{
	register struct user *su;
	long kb, base;

	su = (struct user *)uslot[p->p_slot];
	kb = kdbase + (u_int)su;
	base = ctob((long)p->p_addr);
	pcopy(base, kb, sizeof (struct user));
	off = ustack(su, (label_t *)((caddr_t)su + off));
	pcopy(base + off, kb + off, UBYTES - off);
}

/*
 * A new process, made by the running one.  Give it a slot, the one with
 * the fewest processes.  What is kept at its p_addr is made a copy of
 * the running process's user structure, changed to be the new
 * process's, under a kernel stack that holds only the frame the running
 * process came into the kernel with.  Nothing more of the parent's
 * stack can be copied: it is full of pointers into the parent's slot.
 * The new process starts at forkret (mch_swtch.s) with that frame, as if
 * it had called setjmp(&u.u_ssave) there.
 */
unew(rpp)
	register struct proc *rpp;
{
	extern int forkret();
	register struct proc *p;
	register int i;
	int best, n[32];
	struct user *cu;
	struct proc *sproc;
	int *sar0, *sap;
	label_t ssave;
	long base, entry;

	for (i = 0; i < nslot; i++)
		n[i] = 0;
	for (p = allproc; p; p = p->p_nxt)
		if (p != rpp)
			n[p->p_slot]++;
	best = 1;
	for (i = 2; i < nslot; i++)
		if (n[i] < n[best])
			best = i;
	rpp->p_slot = best;
	cu = (struct user *)uslot[best];

	if (u.u_procp == fpproc)
		fpstore(fpproc);
	sproc = u.u_procp;
	sar0 = u.u_ar0;
	sap = u.u_ap;
	ssave = u.u_ssave;
	entry = (long)(int (__far *)())forkret;
	u.u_procp = rpp;
	u.u_ar0 = (int *)((caddr_t)cu + UBYTES - sizeof (struct trapframe));
	u.u_ap = cu->u_arg;
	u.u_ssave.val[0] = u.u_ssave.val[1] = u.u_ssave.val[2] = 0;
	u.u_ssave.val[3] = (int)u.u_ar0;
	u.u_ssave.val[4] = (u_int)entry;
	u.u_ssave.val[5] = (u_int)(entry >> 16);
	base = ctob((long)rpp->p_addr);
	pcopy(kdbase + (u_int)uptr, base, sizeof (struct user));
	u.u_procp = sproc;
	u.u_ar0 = sar0;
	u.u_ap = sap;
	u.u_ssave = ssave;
	base += UBYTES - sizeof (struct trapframe);
	if (sproc == &proc[0])
		pclear(base, sizeof (struct trapframe));
	else
		pcopy(kdbase + (u_int)sar0, base, sizeof (struct trapframe));
}

/*
 * Process 1 has no program to be a copy of.  It gets icode, the few
 * instructions in mch_start.s that execute /etc/init: one segment of
 * code, which holds its strings too, and a click of data for the
 * processor to have a stack in.
 */
icodestart(tf)
	register struct trapframe *tf;
{
	register struct proc *p = u.u_procp;
	register struct pseg __far *sp;

	sp = &PSEG(p)[1];
	expand(1, 1);
	sp->ps_limit = szicode - 1;
	setldt(p, 1);
	clear(sp->ps_addr, 1);
	(void) copyout((caddr_t)icode, UADDR(LSEL(1), 0), szicode);
	sp->ps_flag |= PS_CODE;
	setldt(p, 1);
	sp++;
	expand(1, 2);
	sp->ps_limit = NBPG - 1;
	setldt(p, 2);
	clear(sp->ps_addr, 1);
	u.u_model = A_SMALL;
	u.u_dseg = 2;
	u.u_break = NBPG;
	bzero((caddr_t)tf, sizeof (*tf));
	tf->tf_cs = LSEL(1);
	tf->tf_ds = tf->tf_es = tf->tf_ss = LSEL(2);
	tf->tf_sp = NBPG;
	tf->tf_flags = PSL_USERSET;
}

/*
 * Copy count bytes from off bytes into the user structure of a process,
 * which need not be the running one, to somewhere in the kernel's data.
 */
ucopy(p, off, to, count)
	register struct proc *p;
	u_int off, count;
	caddr_t to;
{
	if (slotproc[p->p_slot] == p)
		bcopy((caddr_t)uslot[p->p_slot] + off, to, count);
	else
		pcopy(ctob((long)p->p_addr) + off, kdbase + (u_int)to, count);
}

/*
 * A process is finished: nothing of it is to be copied out of its slot
 * or out of the 287.
 */
ufree(p)
	register struct proc *p;
{
	if (slotproc[p->p_slot] == p)
		slotproc[p->p_slot] = NULL;
	if (p == fpproc)
		fpproc = NULL;
	if (p == ldtproc)
		ldtproc = NULL;
}

/*
 * The 287.  Its registers belong to fpproc, the last process to use it.
 * A process that is switched to has the processor told to trap on the
 * next 287 instruction unless it is that one, so the registers are
 * saved and loaded only when a different process really uses it.
 *
 * fpstore puts them away in the user structure of fpproc, which must be
 * in its slot.
 */
fpstore(p)
	register struct proc *p;
{
	register struct user *su;

	su = (struct user *)uslot[p->p_slot];
	fpclts();
	fpsave((caddr_t)&su->u_fps);
	su->u_fpsaved = 1;
	fpproc = NULL;
	fpts();
}

/*
 * The trap: the running process used the 287 and the registers in it
 * are not its own.
 */
fpdna()
{
	register struct proc *p = u.u_procp;

	fpclts();
	if (fpproc == p)
		return;
	if (fpproc) {
		fpstore(fpproc);
		fpclts();
	}
	if (u.u_fpsaved)
		fprestore((caddr_t)&u.u_fps);
	else
		fpinit();
	fpproc = p;
}

/*
 * Make the running process's user structure hold what its 287
 * registers are now.
 */
fpsync()
{
	if (fpproc == u.u_procp)
		fpstore(fpproc);
}

/*
 * Forget the running process's 287 registers: it is starting a new
 * program.
 */
fpdrop()
{
	if (fpproc == u.u_procp) {
		fpproc = NULL;
		fpts();
	}
	u.u_fpsaved = 0;
}

/*
 * Interrupts.  Each of the 16 request lines has a priority level and a
 * handler, given to setirq.  cpl is the level the kernel is at.  A
 * request whose level is above it is serviced at once, at its own level;
 * any other is noted in ipending and its line shut, until splx or spl0
 * lowers cpl far enough and calls spllower.  The clock's and the
 * network's deferred work is requested in spending and done the same
 * way.
 */
int	cpl;
u_int	ipending;		/* requests put off, a bit a line */
int	intrcpl;		/* the level the last interrupt came in at,
				   which the clock wants (clock.c) */
u_int	spending;		/* software interrupts wanted */
#define	SP_CLOCK	1
#define	SP_NET		2		/* setsoftnet (mch_net.s) */
#ifdef INET
int	netintr();			/* in the network: KScall it */
#endif
static u_int imask = 0xFFFB;	/* lines shut; 2 is the second controller */
static char irqlevel[16];
static int (*irqhand[16])();
static int irqarg[16];
static u_int softpc;		/* what the clock interrupted, */
static int softps;		/*   for softclock */

#define	PIC1		0x20
#define	PIC2		0xA0
#define	PIC_EOI		0x20

static
setmask(irq)
	int irq;
{
	if (irq < 8)
		outb(PIC1 + 1, imask);
	else
		outb(PIC2 + 1, imask >> 8);
}

/*
 * handler(arg, frame) is to be called at the given level when the line
 * requests; frame is 0 if the call was put off.  A handler in the
 * network is marked with SUPERFUN, and called with KScall and its arg
 * alone (callirq).  -1 if the line has a handler already.
 */
setirq(irq, level, handler, arg)
	int irq, level, arg;
	int (*handler)();
{
	register int s;

	if (irqhand[irq])
		return (-1);
	s = intsoff();
	irqlevel[irq] = level;
	irqhand[irq] = handler;
	irqarg[irq] = arg;
	imask &= ~(1 << irq);
	setmask(irq);
	intson(s);
	return (0);
}

static
callirq(irq, tf)
	int irq;
	struct trapframe *tf;
{
	register int (*f)() = irqhand[irq];

#ifdef INET
	if (ISSUPERFUN(f)) {
		KScall(KERNELFUN(f), sizeof(int), irqarg[irq]);
		return;
	}
#endif
	(*f)(irqarg[irq], tf);
}

/*
 * Called by mch_trap.s with interrupts off, and returns so; they are on
 * while the handler runs.
 */
intr(tf)
	register struct trapframe *tf;
{
	register int irq = tf->tf_vec - T_IRQ0;
	register u_int bit = 1 << irq;
	int s;

	imask |= bit;
	setmask(irq);
	if (irq >= 8)
		outb(PIC2, PIC_EOI);
	outb(PIC1, PIC_EOI);
	if (irqhand[irq] == 0)
		return;			/* and its line stays shut */
#ifdef UCB_METER
	cnt.v_intr++;
#endif
	if (irqlevel[irq] <= cpl) {
		ipending |= bit;
		return;
	}
	s = cpl;
	intrcpl = s;
	cpl = irqlevel[irq];
	enable();
	callirq(irq, tf);
	disable();
	imask &= ~bit;
	setmask(irq);
	cpl = s;
	if (ipending | spending)
		spllower();
}

/*
 * cpl has dropped: do what was put off that it now allows.  Called with
 * interrupts off, and returns so.
 */
spllower()
{
	register int irq;
	register u_int bit;
	int s;

again:
	if (ipending)
		for (irq = 0, bit = 1; irq < 16; irq++, bit <<= 1)
			if ((ipending & bit) && irqlevel[irq] > cpl) {
				ipending &= ~bit;
				s = cpl;
				cpl = irqlevel[irq];
				enable();
				callirq(irq, (struct trapframe *)0);
				disable();
				imask &= ~bit;
				setmask(irq);
				cpl = s;
				goto again;
			}
#ifdef INET
	if ((spending & SP_NET) && cpl < IPL_NET) {
		spending &= ~SP_NET;
#ifdef UCB_METER
		cnt.v_soft++;
#endif
		s = cpl;
		cpl = IPL_NET;
		enable();
		KScall(netintr, 0);
		disable();
		cpl = s;
		goto again;
	}
#endif
	if ((spending & SP_CLOCK) && cpl < IPL_SOFTCLOCK) {
		spending &= ~SP_CLOCK;
#ifdef UCB_METER
		cnt.v_soft++;
#endif
		s = cpl;
		cpl = IPL_SOFTCLOCK;
		enable();
		softclock(softpc, softps);
		disable();
		cpl = s;
		goto again;
	}
}

/*
 * hardclock wants softclock called once the level allows.
 */
setsoftclock(pc, ps)
	u_int pc;
	int ps;
{
	softpc = pc;
	softps = ps;
	spending |= SP_CLOCK;
}

/*
 * Send an interrupt to process.
 *
 * The process continues in the trampoline code it named in u.u_pcb.pcb_sigc
 * (its library's, for its memory model), with the address of its handler
 * in DX:AX and, on its stack, the signal number, the code, the far address
 * of a signal context, and the context itself.  The trampoline calls the
 * handler and then sigreturn with the context, which puts the registers
 * back as they are here.
 */
struct sigframe {
	int	sf_signum;
	int	sf_code;
	struct	sigcontext __far *sf_scp;
	struct	sigcontext sf_sc;
};

sendsig(p, sig, mask)
	int (*p)(), sig;
	long mask;
{
	struct sigframe sf;
	register struct sigcontext *scp = &sf.sf_sc;
	register struct trapframe *tf = (struct trapframe *)u.u_ar0;
	int oonstack;
	uaddr_t n;

	oonstack = u.u_sigstk.ss_flags & SA_ONSTACK;
	/*
	 * Allocate space for the signal frame.
	 */
	if ((u.u_psflags & SAS_ALTSTACK) &&
	    !(u.u_sigstk.ss_flags & SA_ONSTACK) &&
	    (u.u_sigonstack & sigmask(sig))) {
		n = u.u_sigstk.ss_base + u.u_sigstk.ss_size - sizeof (sf);
		u.u_sigstk.ss_flags |= SA_ONSTACK;
	} else
		n = UADDR(tf->tf_ss, tf->tf_sp - sizeof (sf));
	/*
	 * Build the argument list for the signal handler.
	 */
	sf.sf_signum = sig;
	if (sig == SIGILL || sig == SIGFPE) {
		sf.sf_code = u.u_code;
		u.u_code = 0;
	} else
		sf.sf_code = 0;
	sf.sf_scp = (struct sigcontext __far *)
	    (n + ((caddr_t)scp - (caddr_t)&sf));
	/*
	 * Build the signal context to be used by sigreturn.
	 */
	scp->sc_onstack = oonstack;
	scp->sc_mask = mask;
	scp->sc_ax = tf->tf_ax;
	scp->sc_bx = tf->tf_bx;
	scp->sc_cx = tf->tf_cx;
	scp->sc_dx = tf->tf_dx;
	scp->sc_si = tf->tf_si;
	scp->sc_di = tf->tf_di;
	scp->sc_bp = tf->tf_bp;
	scp->sc_ds = tf->tf_ds;
	scp->sc_es = tf->tf_es;
	scp->sc_ip = tf->tf_ip;
	scp->sc_cs = tf->tf_cs;
	scp->sc_flags = tf->tf_flags;
	scp->sc_sp = tf->tf_sp;
	scp->sc_ss = tf->tf_ss;
	if (copyout((caddr_t)&sf, n, sizeof (sf))) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction violation to halt it in its tracks.
		 */
		fatalsig(SIGILL);
		return;
	}
	tf->tf_ax = UOFF((uaddr_t)p);
	tf->tf_dx = USEL((uaddr_t)p);
	tf->tf_ss = USEL(n);
	tf->tf_sp = UOFF(n);
	tf->tf_cs = USEL((uaddr_t)u.u_pcb.pcb_sigc);
	tf->tf_ip = UOFF((uaddr_t)u.u_pcb.pcb_sigc);
	tf->tf_flags &= ~(PSL_T | PSL_D);
}

/*
 * Is this a selector the process may have in a segment register?  what
 * is 0 for any segment it can read, 1 for code, 2 for a writable data
 * segment.
 */
selok(sel, what)
	u_int sel;
	int what;
{
	register int a;

	if (!ISLSEL(sel) || (a = lar(sel)) < 0 || (a & SD_S) == 0)
		return (0);
	switch (what) {
	case 1:
		return ((a & 0x08) != 0);
	case 2:
		return ((a & 0x0A) == 0x02);
	}
	return ((a & 0x0A) != 0x08);	/* not code that cannot be read */
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to the registers the context specifies.  Check carefully to
 * make sure that the user has not modified them to gain improper
 * priviledges or to cause a machine fault: the selectors must be ones
 * the process could have loaded itself, and the flags may not give it
 * the I/O ports or turn interrupts off.
 */
sigreturn()
{
	struct a {
		struct sigcontext __far *scp;
	};
	struct sigcontext sc;
	register struct sigcontext *scp = &sc;
	register struct trapframe *tf = (struct trapframe *)u.u_ar0;

	u.u_error = copyin((uaddr_t)((struct a *)u.u_ap)->scp, (caddr_t)scp,
	    sizeof (*scp));
	if (u.u_error)
		return;
	if (!selok(scp->sc_cs, 1) || !selok(scp->sc_ss, 2) ||
	    (scp->sc_ds && !selok(scp->sc_ds, 0)) ||
	    (scp->sc_es && !selok(scp->sc_es, 0))) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = EJUSTRETURN;
	if (scp->sc_onstack & SA_ONSTACK)
		u.u_sigstk.ss_flags |= SA_ONSTACK;
	else
		u.u_sigstk.ss_flags &= ~SA_ONSTACK;
	u.u_procp->p_sigmask = scp->sc_mask & ~sigcantmask;
	tf->tf_ax = scp->sc_ax;
	tf->tf_bx = scp->sc_bx;
	tf->tf_cx = scp->sc_cx;
	tf->tf_dx = scp->sc_dx;
	tf->tf_si = scp->sc_si;
	tf->tf_di = scp->sc_di;
	tf->tf_bp = scp->sc_bp;
	tf->tf_ds = scp->sc_ds;
	tf->tf_es = scp->sc_es;
	tf->tf_ip = scp->sc_ip;
	tf->tf_cs = scp->sc_cs;
	tf->tf_flags = (scp->sc_flags & ~PSL_USERCLR) | PSL_USERSET;
	tf->tf_sp = scp->sc_sp;
	tf->tf_ss = scp->sc_ss;
}

int waittime = -1;

boot(dev, howto)
	register dev_t dev;
	register int howto;
{
	register struct fs *fp;

	/*
	 * Force the root filesystem's superblock to be updated,
	 * so the date will be as current as possible after
	 * rebooting.
	 */
	if (fp = getfs(rootdev))
		fp->fs_fmod = 1;
	if ((howto&RB_NOSYNC)==0 && waittime < 0 && bfreelist[0].b_forw) {
		waittime = 0;
		printf("syncing disks... ");
		(void) _splnet();
		/*
		 * Release inodes held by texts before update.
		 */
		xumount(NODEV);
		fsclean();
		{ register struct buf *bp;
		  int iter, nbusy;

		  for (iter = 0; iter < 20; iter++) {
			nbusy = 0;
			for (bp = &buf[nbuf]; --bp >= buf; )
				if (bp->b_flags & B_BUSY)
					nbusy++;
			if (nbusy == 0)
				break;
			printf("%d ", nbusy);
			delay(40000L * iter);
		  }
		}
		printf("done\n");
	}
	(void) _splhigh();
	if (howto & RB_HALT) {
		printf("halting\n");
		halt();
		/*NOTREACHED*/
	}
	doboot();
	/*NOTREACHED*/
}

/*
 * Stop for good.
 */
halt()
{
	for (;;)
		cpustop();
}

/*
 * Start the machine again: the keyboard controller can pulse the
 * processor's reset line.
 */
doboot()
{
	register int i;

	disable();
	for (i = 0; i < 10; i++) {
		outb(0x64, 0xFE);
		delay(100000L);
	}
	halt();
}

delay(n)
	long n;
{
	while (--n > 0)
		;
}

/*
 * Clear a buffer of the pool.
 */
clrbuf(bp)
	register struct buf *bp;
{
	bzero(mapin(bp), MAXBSIZE);
}

/*
 * The number of the lowest bit that is set in mask, counting from 1;
 * 0 if none is.
 */
ffs(mask)
	register long mask;
{
	register int i;

	if (mask == 0)
		return (0);
	for (i = 1; (mask & 1) == 0; i++)
		mask >>= 1;
	return (i);
}

/*
 * Set every attribute to "no value".
 */
vattr_null(vp)
	register struct vattr *vp;
{
	vp->va_mode = VNOVAL;
	vp->va_uid = VNOVAL;
	vp->va_gid = VNOVAL;
	vp->va_size = VNOVAL;
	vp->va_atime = VNOVAL;
	vp->va_mtime = VNOVAL;
	vp->va_flags = VNOVAL;
	vp->va_vaflags = 0;
}

/*
 * Profiling: add ticks to the count for the place pc in the running
 * process's profile buffer.  Profiling stops if the buffer cannot be
 * written.
 */
addupc(pc, up, ticks)
	u_int pc;
	register struct uprof *up;
	int ticks;
{
	register u_int off;
	register int v;
	uaddr_t a;

	if (pc < up->pr_off)
		return;
	off = (((long)((pc - up->pr_off) >> 1) * (up->pr_scale >> 1) >> 14)
	    + 1) & ~1;
	if (off >= up->pr_size)
		return;
	a = (uaddr_t)up->pr_base + off;
	v = fuword(a);
	if (v == -1 && fubyte(a) == -1 || suword(a, v + ticks) < 0)
		up->pr_scale = 0;
}

/*
 * The machine's own part of sysctl.
 */
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void __far *oldp;
	size_t *oldlenp;
	void __far *newp;
	size_t newlen;
{
	extern struct tty cons[];

	switch (name[0]) {
	case CPU_CONSDEV:
		if (namelen != 1)
			return (ENOTDIR);
		return (sysctl_rdstruct(oldp, oldlenp, newp,
		    &cons[0].t_dev, sizeof (cons[0].t_dev)));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
