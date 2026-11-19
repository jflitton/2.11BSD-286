/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)conf.h	1.3 (2.11BSD Berkeley) 12/23/92
 */

/*
 * Declaration of block device
 * switch. Each entry (row) is
 * the only link between the
 * main unix code and the driver.
 * The initialization of the
 * device switches is in the
 * file conf.c.
 */
struct bdevsw
{
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_strategy)();
	int	(*d_root)();		/* XXX root attach routine */
	daddr_t	(*d_psize)();
	int	d_flags;
};

/*
 * d_flags, besides the B_TAPE of buf.h: the driver sends the transfers
 * it finds waiting together, that follow one another on the disk, as
 * one command.  Reading several blocks ahead of a file is worth doing
 * on such a disk, and on another only makes the queue longer (rwip).
 * Such a driver also leaves an idle disk idle while bioplug is not
 * zero, and starts it when its strategy routine is called with no
 * buffer (breadrun).
 */
#define	D_RUNS		0x0001

#if defined(KERNEL) && !defined(SUPERVISOR)
extern struct	bdevsw bdevsw[];
#endif

/*
 * Character device switch.
 */
struct cdevsw
{
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_read)();
	int	(*d_write)();
	int	(*d_ioctl)();
	int	(*d_stop)();
	struct tty *d_ttys;
	int	(*d_select)();
	int	(*d_strategy)();
};
#if defined(KERNEL) && !defined(SUPERVISOR)
extern struct	cdevsw cdevsw[];
#endif

/*
 * tty line control switch.
 */
struct linesw
{
	int	(*l_open)();
	int	(*l_close)();
	int	(*l_read)();
	int	(*l_write)();
	int	(*l_ioctl)();
	int	(*l_rint)();
	int	(*l_rend)();
	int	(*l_meta)();
	int	(*l_start)();
	int	(*l_modem)();
};
#if defined(KERNEL) && !defined(SUPERVISOR)
extern struct	linesw linesw[];
#endif
