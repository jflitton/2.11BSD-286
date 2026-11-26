/*
 *
 *      UNIX debugger - common definitions
 *
 *	1998/4/21 - remove local redefinitions used with ptrace
 *	2026/11/25 - the 286: segmented addresses, its registers and
 *		     instructions; no overlays
 *
 *      Layout of a.out file (fsym):
 *
 *	This has changed over time - see a.out.h, sys/exec.h and nlist.h
 *	for the current a.out definition and format.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/seg.h>
#include <sgtty.h>
#include <setjmp.h>
#include <unistd.h>
#include <a.out.h>
#include <sys/ptrace.h>

#define	MAXSYMLEN	32
#define MAXCOM	64
#define MAXARG	32
#define LINSIZ	512

#define BEGIN	{
#define END	}

#define IF	if(
#define THEN	){
#define ELSE	} else {
#define ELIF	} else if (
#define FI	}

#define FOR	for(
#define WHILE	while(
#define DO	){
#define OD	}
#define REP	do{
#define PER	}while(
#define DONE	);
#define LOOP	for(;;){
#define POOL	}

#define ANDF	&&
#define ORF	||

#define LPRMODE "%X"
#define OFFMODE "+%x"

/*
 * An address is as a far pointer has it: the selector of its segment in
 * the high word and the offset in the low.  One whose selector is 0 is in
 * the segment that its space has by default, the code segment for `?' and
 * the data segment for `/' (map.def1 and def2); so in a small-model
 * program a 16-bit address means what it does on the PDP-11 with
 * separate instructions and data.  In a file of no format adb knows, an
 * address is an offset in the file.
 */
#define	ADDR(s, o)	(((long)(u_int)(s) << 16) | (u_int)(o))
#define	ASEL(a)		((u_int)((u_long)(a) >> 16))
#define	AOFF(a)		((u_int)(a))

/*
 * Selectors of one segment: the low two bits, the RPL, do not change
 * which segment a selector is, and the kernel marks a far pointer with
 * them (SUPERFUN, machine/machparam.h).
 */
#define	SAMESEG(a, b)	((((a) ^ (b)) & ~3) == 0)

/*
 * This is the memory resident portion of a symbol.  The only difference
 * between this and a 'nlist' entry is the size of the string offset   By
 * restricting the string table to be less than 64kb we save 2 bytes per
 * symbol - approximately 7.5kb of D space in the case of /unix's symbol
 * table.  In practice this restriction is unlikely to be reached since
 * even the kernel's symbol table of ~28kb only has a strings table of 21kb.
 * On the 286 ovno is the symbol's segment, counting from 1.
*/

	struct	SYMbol
		{
		u_short	value;
		char	type;
		char	ovno;
		u_short	soff;		/* low order of string table offset */
		};

#define SYMTYPE(symflag) (( symflag>=041 || (symflag>=02 && symflag<=04))\
				?  ((symflag&07)>=3 ? DSYM : (symflag&07))\
				: NSYM)

typedef	char		MSG[];
typedef	struct map	MAP;
typedef	MAP		*MAPPTR;
typedef	struct bkpt	BKPT;
typedef	BKPT		*BKPTR;
typedef	struct user	U;

/*
 * File address maps.  A program, a core file and the kernel's memory are
 * a segment for each selector: from offset b to e of it is in the file
 * from f on, and from e to z is zero, as the end of a data segment that
 * is not in the file is.  A file of no format adb knows has upstream's
 * two stretches of the file, b1 to e1 from f1 and b2 to e2 from f2.
 */
struct seg {
	u_short	sel;
	u_short	type;		/* XS_CODE or XS_DATA */
	long	b;
	long	e;
	long	f;
	long	z;
};

struct map {
	long	b1;
	long	e1;
	long	f1;
	long	b2;
	long	e2;
	long	f2;
	int	nseg;		/* 0 for a file of no known format */
	struct	seg seg[NSEG];
	u_short	def1;		/* the segment of selector 0 */
	u_short	def2;		/*   and with a star after ? or / */
	int	ufd;
};

/* a breakpoint is an INT 3 in place of the first byte of an instruction */
struct bkpt {
	long	loc;
	int	ins;		/* the byte it took the place of */
	int	count;
	int	initcnt;
	char	flag;
	char	inproc;		/* the INT 3 is in the process */
	char	comm[MAXCOM];
	BKPT	*nxtbkpt;
};

typedef	struct reglist	REGLIST;
typedef	REGLIST		*REGPTR;
struct reglist {
	char	*rname;
	int	roffs;
};

/*
 * Internal variables ---
 *  They are addressed by name. (e.g. (`0'-`9', `a'-`b'))
 *  thus there can only be 36 of them.
 */

#define VARB    11
#define VARD    13
#define VARE    14
#define VARM    22
#define VARS    28
#define VART    29

#define RD      0
#define WT      1
#define NSP     0
#define ISP     1
#define DSP     2
#define STAR    4
#define DSYM    7
#define ISYM    2
#define ASYM    1
#define NSYM    0
#define ESYM    0177
#define BKPTSET 1
#define BKPTEXEC 2

#define BPT     0xcc		/* INT 3 */

#define NOREG   32767           /* impossible return from getreg() */
#define NREG    14		/* the frame's registers but tf_vec, tf_err */

/*
 * A process's registers are in the frame (machine/frame.h) at the top of
 * its kernel stack, which is the top of the user structure's USIZE
 * clicks, wherever that is in the kernel: the processor takes the stack
 * pointer from the task state segment, which says the top of the slot.
 * u.u_ar0 points there, but in the kernel's data segment, not the file's.
 */
#define	UAR0	((ctob(USIZE) - sizeof (struct trapframe)) / sizeof (u_int))

#define MAXOFF  255
#define MAXPOS  80
#define MAXLIN  128

#define TRUE	 (-1)
#define FALSE	0
#define LOBYTE	0377
#define HIBYTE	0177400
#define STRIP	0177

/* <machine/reg.h>'s SP is the stack pointer's place in the frame: RSP */
#undef	SP
#define	RSP	(offsetof(struct trapframe, tf_sp) / sizeof (u_int))
#define SP	' '
#define TB	'\t'
#define EOR     '\n'
#define QUOTE   0200
#define EVEN    (~1)

/* long to ints and back (puns) */
union {
	int     I[2];
	long   L;
} itolws;

#define leng(a)         ((long)((unsigned)(a)))
#define shorten(a)      ((int)(a))
#define itol(a,b)       (itolws.I[0]=(a), itolws.I[1]=(b), itolws.L)

/*
 * The process's address is far whatever the model of adb (the C
 * library's ptrace takes it as offset and selector): a long here.
 */
int	ptrace(int, int, long, int);

/*
 * What takes or gives an address, which is long: declared so, that a
 * 16-bit value passed is made one.
 */
long	inkdot(int);
long	fulladr(long, int);
long	pcaddr(void);
u_int	segsel(int);
u_int	get(long, int);
u_int	chkget(long, int);
int	put(long, int, int);
int	psymoff(long, int, char *);
int	valpr(long, int);
u_int	findsym(long, int);
int	localsym(long);
int	firstframe(long, int);
int	printins(int, u_int);
long	round(long, long);
BKPTR	scanbkpt(long);

/* result type declarations */
struct	SYMbol	*lookupsym();
struct	SYMbol	*symget();
char	*exform();
char	*cache_sym(), *no_cache_sym();

struct sgttyb adbtty, usrtty;
jmp_buf erradb;
