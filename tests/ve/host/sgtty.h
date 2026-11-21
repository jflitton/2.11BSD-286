/*
 * <sgtty.h> for ve built where the tests are run (tests/ve/run.sh).
 * There is no terminal there: what term.c asks of one goes to shim.c,
 * which says only how big the window is to be.  The names are the
 * system's, and whatever the machine has by them is put aside.
 */
#define	sgttyb	ve_sgttyb
#define	tchars	ve_tchars
#define	ltchars	ve_ltchars
struct sgttyb {
	char	sg_ispeed, sg_ospeed, sg_erase, sg_kill;
	int	sg_flags;
};
struct tchars {
	char	t_intrc, t_quitc, t_startc, t_stopc, t_eofc, t_brkc;
};
struct ltchars {
	char	t_suspc, t_dsuspc, t_rprntc, t_flushc, t_werasc, t_lnextc;
};
#undef	TIOCGETP
#undef	TIOCSETN
#undef	TIOCGETC
#undef	TIOCSETC
#undef	TIOCGLTC
#undef	TIOCSLTC
#undef	TIOCLGET
#undef	TIOCLSET
#undef	CBREAK
#undef	ECHO
#undef	CRMOD
#undef	XTABS
#undef	LLITOUT
#undef	LPASS8
#undef	LNOFLSH
#define	TIOCGETP	(-1L)
#define	TIOCSETN	(-2L)
#define	TIOCGETC	(-3L)
#define	TIOCSETC	(-4L)
#define	TIOCGLTC	(-5L)
#define	TIOCSLTC	(-6L)
#define	TIOCLGET	(-7L)
#define	TIOCLSET	(-8L)
#define	CBREAK		002
#define	ECHO		010
#define	CRMOD		020
#define	XTABS		06000
#define	LLITOUT		040
#define	LPASS8		04000
#define	LNOFLSH		0200
#define	ioctl		ve_ioctl
int	ve_ioctl(int, long, char *);
