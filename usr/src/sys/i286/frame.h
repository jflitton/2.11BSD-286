/*
 * What is on the kernel stack when trap() is called, lowest address
 * first.  mch_trap.s builds it; what trap() changes here is what the
 * interrupted code gets back.
 */
struct trapframe {
	unsigned short	tf_di, tf_si, tf_bp;
	unsigned short	tf_xsp;		/* not restored */
	unsigned short	tf_bx, tf_dx, tf_cx, tf_ax;
	unsigned short	tf_es, tf_ds;
	unsigned short	tf_vec;		/* which exception or interrupt */
	unsigned short	tf_err;		/* its error code, or 0 */
	unsigned short	tf_ip, tf_cs, tf_flags;
	unsigned short	tf_sp, tf_ss;	/* only if a process was interrupted */
};

/*
 * tf_vec.  The first 17 are the processor's exceptions; the two interrupt
 * controllers are moved to 32 to 47, clear of the ones Intel reserves.
 */
#define	T_DIVIDE	0	/* divide error */
#define	T_DEBUG		1	/* single step */
#define	T_NMI		2
#define	T_BPT		3	/* breakpoint instruction */
#define	T_OFLOW		4	/* INTO */
#define	T_BOUND		5	/* BOUND */
#define	T_BADOP		6	/* invalid opcode */
#define	T_NOFPU		7	/* 287 instruction with TS or EM set */
#define	T_DOUBLE	8	/* double fault */
#define	T_FPUSEG	9	/* 287 operand ran past its segment */
#define	T_BADTSS	10
#define	T_SEGNP		11	/* segment not present */
#define	T_STACK		12	/* stack segment overrun or not present */
#define	T_PROT		13	/* general protection */
#define	T_FPUERR	16	/* 287 error */
#define	T_IRQ0		32	/* interrupt request 0, the clock */
#define	NIDT		48
