/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)msgbuf.h	1.2 (2.11BSD) 1998/12/5
 */

#define	MSG_MAGIC	0x063061
#define	MSG_BSIZE	4096

struct	msgbuf {
	long	msg_magic;
	int	msg_bufx;
	int	msg_bufr;
	u_short	msg_click;	/* not used */
#ifdef KERNEL
	char __far *msg_bufc;	/* outside the kernel's data segment */
#else
	char	*msg_bufc;	/* the offset; dmesg points it at its copy */
	u_short	msg_bufs;	/* and the selector, one of the kernel's */
#endif
};

#define	logMSG	0		/* /dev/klog */
#define	logDEV	1		/* /dev/erlg */
#define	logACCT	2		/* /dev/acct */
