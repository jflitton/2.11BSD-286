/*
 * The flags word.
 */
#define	PSL_C		0x0001		/* carry */
#define	PSL_P		0x0004		/* parity */
#define	PSL_A		0x0010		/* auxiliary carry */
#define	PSL_Z		0x0040		/* zero */
#define	PSL_N		0x0080		/* sign */
#define	PSL_T		0x0100		/* single step */
#define	PSL_I		0x0200		/* interrupts enabled */
#define	PSL_D		0x0400		/* string instructions go downward */
#define	PSL_V		0x0800		/* overflow */
#define	PSL_IOPL	0x3000		/* ring that may do I/O */
#define	PSL_NT		0x4000		/* nested task */

/*
 * A process runs with interrupts on and I/O privilege 0: IN, OUT, CLI
 * and STI are the kernel's alone.
 */
#define	PSL_USERSET	PSL_I
#define	PSL_USERCLR	(PSL_IOPL | PSL_NT)
