/*
 * Location of a process's registers relative to u.u_ar0, which points at
 * its struct trapframe (frame.h) as an array of words.
 */
#define	BX	0
#define	CX	1
#define	DX	2
#define	SI	3
#define	DI	4
#define	BP	5
#define	AX	6
#define	ES	7
#define	DS	8
#define	IP	11
#define	CS	12
#define	FLAGS	13
#define	SP	14
#define	SS	15

/*
 * The names shared code uses: the two result registers, the stack
 * pointer, the program counter and the status word.
 */
#define	R0	AX
#define	R1	DX
#define	R6	SP
#define	R7	IP
#define	PC	IP
#define	RPS	FLAGS
