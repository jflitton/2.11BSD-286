/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)param.c	2.3 (2.11BSD) 2018/12/19
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/time.h"
#include "../h/resource.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/text.h"
#include "../h/file.h"
#include "../h/dir.h"
#include "../h/inode.h"
#include "../h/fs.h"
#include "../h/mount.h"
#include "../h/callout.h"
#include "../h/map.h"
#include "../h/clist.h"
#include "../h/namei.h"
#include "../machine/seg.h"

/*
 * System parameter formulae.
 *
 * This file is the sizes of this kernel's tables, to be changed to suit
 * the machine and its load.  What is declared near here is in the
 * kernel's one data segment, 64K for everything; what is declared far has
 * a segment of its own.
 */
#define	MAXUSERS 6
#define	NBUF 64

/*
 * Slots: the places in the kernel's data segment where a process's user
 * structure and kernel stack can be while it runs.  Slot 0 is the
 * swapper's.  The processes that share one of the rest are copied in and
 * out as they take turns, so more slots is faster and costs UBYTES each.
 */
#define	NSLOT 6

int	hz = 60;
int	mshz = (1000000L + 60 - 1) / 60;
struct	timezone tz = { 0, 0 };

#define	NPROC (10 + 7 * MAXUSERS)
int	nproc = NPROC;
#define NTEXT (26 + MAXUSERS)
int	ntext = NTEXT;
#define NINODE ((NPROC + 16 + MAXUSERS) + 22)
int	ninode = NINODE;
#define NFILE ((8 * NINODE / 10) + 20)
int	nfile = NFILE;
#define NCALL (16 + MAXUSERS)
int	ncallout = NCALL;
int	nbuf = NBUF;
int	nslot = NSLOT;

#define NCLIST (20 + 8 * MAXUSERS)
int	nclist = NCLIST;

#define	NNCH (NINODE * 11 / 10)
int	nchsize = NNCH;

/*
 * These have to be allocated somewhere; allocating
 * them here forces loader errors if this file is omitted
 * (if they've been externed everywhere else; hah!).
 */
struct	proc proc[NPROC], *procNPROC;
struct	text text[NTEXT], *textNTEXT;
struct	inode inode[NINODE], *inodeNINODE;
struct	file file[NFILE], *fileNFILE;
struct	callout callout[NCALL];
struct	mount mount[NMOUNT];
struct	buf buf[NBUF], bfreelist[BQUEUES];
struct	bufhd bufhash[BUFHSZ];
struct	cblock cfree[NCLIST];
struct	namecache namecache[NNCH];
char	uslot[NSLOT][UBYTES];
struct	proc *slotproc[NSLOT];

#define CMAPSIZ	NPROC			/* size of core allocation map */
#define SMAPSIZ	((9 * NPROC) / 10)	/* size of swap allocation map */

struct mapent	_coremap[CMAPSIZ];
struct map	coremap[1] = {
	_coremap,
	&_coremap[CMAPSIZ],
	"coremap",
};

struct mapent	_swapmap[SMAPSIZ];
struct map	swapmap[1] = {
	_swapmap,
	&_swapmap[SMAPSIZ],
	"swapmap",
};

/*
 * Outside the kernel's data segment: every process's list of segments
 * and its descriptor table, and what each mounted file system is called.
 */
struct pseg __far	pseg[NPROC * NSEG];
struct segdesc __far	ldt[NPROC * NSEG];
struct xmount __far	xmount[NMOUNT];
