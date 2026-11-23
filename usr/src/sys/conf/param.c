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
 * This file is copied into each directory where we compile the kernel,
 * with the values of the machine's configuration file put in it
 * (config): those are the sizes of this kernel's tables, to suit the
 * machine and its load.  What is declared near here is in the kernel's
 * one data segment, 64K for everything; what is declared far has a
 * segment of its own.
 */
#define	MAXUSERS %MAXUSERS%

/*
 * The most buffers there can be, 1K of data each.  How many there are
 * is nbuf, which the kernel works out as it starts from the memory it
 * finds (startup, machdep.c).  The data is outside the kernel's data
 * segment; the headers are in it, 28 bytes each, and they are what
 * keeps this from being more.
 */
#define	NBUF %NBUF%

/*
 * Slots: the places in the kernel's data segment where a process's user
 * structure and kernel stack can be while it runs.  Slot 0 is the
 * swapper's.  The processes that share one of the rest are copied in and
 * out as they take turns, so more slots is faster and costs UBYTES each.
 */
#define	NSLOT %NSLOT%

int	hz = %LINEHZ%;
int	mshz = (1000000L + %LINEHZ% - 1) / %LINEHZ%;
struct	timezone tz = { %TIMEZONE%, %DST% };

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
int	nbuf;			/* by the memory found (startup): */
int	bufshare = 16;		/* this part of it, a sixteenth (patchable), */
int	nbufmax = NBUF;		/* and no more than this (patchable, to less) */
int	nslot = NSLOT;

/*
 * The character blocks of the terminals' queues, 32 bytes each for 30
 * characters, in a segment of their own (CLSPACE, machparam.h), which
 * could hold 2047.  This is enough for every terminal to have all it
 * may at once, so that nothing typed is lost for want of one
 * (ttyinput): a pseudo-terminal's output to its high water mark and
 * one piece of a write past it, 2100 characters, and twice TTYHOG, 255,
 * of what was typed, 90 blocks for each of 8; and the four screens and
 * two serial lines, whose mark is 650, 44 each.  984, and the first is
 * not used (cinit).
 */
#define NCLIST 1024
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
struct	cblock CLSPACE cfree[NCLIST];
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
