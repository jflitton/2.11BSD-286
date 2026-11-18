/*
 * What is on the kernel stack when trap() is called, lowest address
 * first.  mch_trap.s builds it; what trap() changes here is what the
 * interrupted code gets back.
 */
struct trapframe {
	unsigned short	tf_bx, tf_cx, tf_dx;	/* these six are a system */
	unsigned short	tf_si, tf_di, tf_bp;	/* call's arguments, in order */
	unsigned short	tf_ax;
	unsigned short	tf_es, tf_ds;
	unsigned short	tf_vec;		/* which exception or interrupt: trap.h */
	unsigned short	tf_err;		/* its error code, or 0 */
	unsigned short	tf_ip, tf_cs, tf_flags;
	unsigned short	tf_sp, tf_ss;	/* only if a process was interrupted */
};

/* A system call's arguments, as an array of words. */
#define	tf_arg(tf)	(&(tf)->tf_bx)
#define	NARGREG		6

/* Did the trap come from a process?  Its code segments are ring 3. */
#define	USERMODE(cs)	(((cs) & 3) == 3)
