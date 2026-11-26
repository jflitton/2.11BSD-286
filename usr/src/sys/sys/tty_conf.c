/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tty_conf.c	1.3 (2.11BSD) 2026/5/15
 */

#include "param.h"
#include "../pdp/seg.h"

#include "file.h"
#include "ioctl.h"
#include "tty.h"
#include "errno.h"
#include "conf.h"

int	nodev();
int	nulldev();

int	ttyopen(),ttylclose(),ttread(),ttwrite(),nullioctl(),ttstart();
int	ttymodem(), nullmodem(), ttyinput();

#include "tb.h"
#if NTB > 0
int	tbopen(),tbclose(),tbread(),tbinput(),tbioctl();
#endif
#include "sl.h"
#if NSL > 0
int	SLOPEN(),SLCLOSE(),SLINPUT(),SLTIOCTL(),SLSTART();
#endif


struct	linesw linesw[] =
{
	ttyopen, ttylclose, ttread, ttwrite, nullioctl,	/* 0- OTTYDISC */
	ttyinput, nodev, nulldev, ttstart, ttymodem,
	nodev, nodev, nodev, nodev, nodev,		/* 1 - was bk */
	nodev, nodev, nodev, nodev, nodev,
	ttyopen, ttylclose, ttread, ttwrite, nullioctl,	/* 2- NTTYDISC */
	ttyinput, nodev, nulldev, ttstart, ttymodem,
#if NTB > 0
	tbopen, tbclose, tbread, nodev, tbioctl,
	tbinput, nodev, nulldev, ttstart, nullmodem,	/* 3- TABLDISC */
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
#if NSL > 0
	SLOPEN, SLCLOSE, nodev, nodev, SLTIOCTL,
	SLINPUT, nodev, nulldev, SLSTART, nulldev,	/* 4- SLIPDISC */
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
};

int	nldisp = sizeof (linesw) / sizeof (linesw[0]);

/*
 * Do nothing specific version of line
 * discipline specific ioctl command.
 */
/*ARGSUSED*/
nullioctl(tp, cmd, data, flags)
	struct tty *tp;
	u_int cmd;
	char *data;
	int flags;
{

#ifdef lint
	tp = tp; data = data; flags = flags;
#endif
	return (-1);
}

#if NSL > 0
SLOPEN(dev, tp)
	dev_t dev;
	struct tty *tp;
{
	int error;

	if (!suser())
		return (EPERM);
	if (tp->t_line == SLIPDISC)
		return (EBUSY);
	error = KScall(slopen, sizeof(dev_t) + sizeof(struct tty *), dev, tp);
	if (!error)
		ttyflush(tp, FREAD | FWRITE);
	return(error);
}

SLCLOSE(tp, flag)
	struct tty *tp;
	int flag;
{

	ttywflush(tp);
	tp->t_line = 0;
	KScall(slclose, sizeof(struct tty *), tp);
}

SLTIOCTL(tp, cmd, data, flag)
	struct tty *tp;
	int cmd, flag;
	caddr_t data;
{

	return(KScall(sltioctl, sizeof(struct tty *) + sizeof(int) +
	    sizeof(caddr_t) + sizeof(int), tp, cmd, data, flag));
}

/*
 * At the terminal's interrupt.  The PDP-11's saves its mapping of the
 * kernel's data around the call; on the 286 KScall puts DS back.
 */
SLSTART(tp)
	struct tty *tp;
{

	KScall(slstart, sizeof(struct tty *), tp);
}

/*
 * The PDP-11's gives the network each character here.  The 286's puts
 * it on the raw queue, as ttyinput does, for the network to take at its
 * software interrupt, which it asks for when a frame has ended
 * (net/if_sl.c).  At most SLRAWQ characters wait.
 */
#define	SLRAWQ		2048
#define	SLFRAME_END	0300

SLINPUT(c, tp)
	register int c;
	register struct tty *tp;
{

	c &= 0377;
	if (tp->t_rawq.c_cc < SLRAWQ)
		(void) putc(c, &tp->t_rawq);
	if (c == SLFRAME_END)
		KScall(slframe, 0);
}
#endif
