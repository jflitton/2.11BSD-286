/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dk.h	7.1 (Berkeley) 6/4/86
 */

/*
 * Instrumentation
 */
#define	CPUSTATES	4

#define	CP_USER		0
#define	CP_NICE		1
#define	CP_SYS		2
#define	CP_IDLE		3

#define	DK_NDRIVE	10

/*
 * Declared here and defined in kern_clock.c: this linker, unlike the
 * PDP-11's, does not make one of a variable defined in several files.
 */
#if defined(KERNEL) && defined(UCB_METER) && !defined(SUPERVISOR)
extern long	cp_time[CPUSTATES];	/* number of ticks spent in each cpu state */
extern int	dk_ndrive;		/* number of drives being monitored */
extern int	dk_busy;		/* bit array of drive busy flags */
extern long	dk_time[DK_NDRIVE];	/* ticks spent with drive busy */
extern long	dk_seek[DK_NDRIVE];	/* number of seeks */
extern long	dk_xfer[DK_NDRIVE];	/* number of transfers */
extern long	dk_wds[DK_NDRIVE];	/* number of clicks transfered */
extern long	dk_wps[DK_NDRIVE];	/* words per second */
extern char	*dk_name[DK_NDRIVE];	/* names of monitored drives */
extern int	dk_unit[DK_NDRIVE];	/* unit numbers of monitored drives */
extern int	dk_n;			/* number of dk numbers assigned so far */

extern long	tk_nin;			/* number of tty characters input */
extern long	tk_nout;		/* number of tty characters output */
#endif
