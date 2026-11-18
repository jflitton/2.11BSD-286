/*
 * Trap type values: the vector the processor took, tf_vec in a struct
 * trapframe.  The first 17 are the processor's exceptions.  The two
 * interrupt controllers are moved to 32 to 47, clear of the vectors Intel
 * reserves, and the system call follows them.
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
#define	T_SYSCALL	48	/* system call: INT 30h */
#define	NIDT		49
