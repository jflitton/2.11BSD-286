/*
 * The device switches: what connects the rest of the kernel to the
 * drivers.  A device's major number is its place in one of these.
 */
#include "param.h"
#include "conf.h"
#include "buf.h"
#include "time.h"
#include "ioctl.h"
#include "resource.h"
#include "inode.h"
#include "proc.h"
#include "clist.h"
#include "tty.h"
#include "pty.h"

int	nulldev();
int	nodev();
int	rawrw();

#include "rd.h"
#if NRD > 0
int	rdopen(), rdstrategy(), rdroot();
daddr_t	rdsize();
#else
#define	rdopen		nodev
#define	rdroot		nulldev
#define	rdstrategy	nodev
#define	rdsize		NULL
#endif

#include "wd.h"
#if NWD > 0
int	wdopen(), wdclose(), wdstrategy(), wdioctl();
daddr_t	wdsize();
#else
#define	wdopen		nodev
#define	wdclose		nodev
#define	wdstrategy	nodev
#define	wdioctl		nodev
#define	wdsize		NULL
#endif

#include "sd.h"
#if NSD > 0
int	sdopen(), sdclose(), sdstrategy(), sdioctl();
daddr_t	sdsize();
#else
#define	sdopen		nodev
#define	sdclose		nodev
#define	sdstrategy	nodev
#define	sdioctl		nodev
#define	sdsize		NULL
#endif

#include "fl.h"
#if NFL > 0
int	flopen(), flclose(), flstrategy(), flioctl();
daddr_t	flsize();
#else
#define	flopen		nodev
#define	flclose		nodev
#define	flstrategy	nodev
#define	flioctl		nodev
#define	flsize		NULL
#endif

/*
 * Where the root file system is when the boot program brought no RAM
 * disk for it: the first hard disk's first partition.  (patchable)
 */
dev_t	diskroot = makedev(1, 0);

struct bdevsw	bdevsw[] = {
/* rd = 0 */
	rdopen,		nulldev,	rdstrategy,	rdroot,		rdsize,
	0,
/* wd = 1 */
	wdopen,		wdclose,	wdstrategy,	nulldev,	wdsize,
	0,
/* sd = 2 */
	sdopen,		sdclose,	sdstrategy,	nulldev,	sdsize,
	D_RUNS,
/* fl = 3 */
	flopen,		flclose,	flstrategy,	nulldev,	flsize,
	0,
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

int	cnopen(), cnclose(), cnread(), cnwrite(), cnioctl();
extern struct tty cons[];

#include "com.h"
#if NCOM > 0
int	comopen(), comclose(), comread(), comwrite(), comioctl();
extern struct tty com_tty[];
#else
#define	comopen		nodev
#define	comclose	nodev
#define	comread		nodev
#define	comwrite	nodev
#define	comioctl	nodev
#define	com_tty		0
#endif

int	mmrw();
#define	mmselect	seltrue

int	syopen(), syread(), sywrite(), syioctl(), syselect();

int	logopen(), logclose(), logread(), logioctl(), logselect();

int	fdopen();

#if NPTY > 0
int	ptsopen(), ptsclose(), ptsread(), ptswrite(), ptsstop();
int	ptcopen(), ptcclose(), ptcread(), ptcwrite(), ptcselect();
int	ptyioctl();
extern	struct tty pt_tty[];
#else
#define	ptsopen		nodev
#define	ptsclose	nodev
#define	ptsread		nodev
#define	ptswrite	nodev
#define	ptcopen		nodev
#define	ptcclose	nodev
#define	ptcread		nodev
#define	ptcwrite	nodev
#define	ptyioctl	nodev
#define	pt_tty		0
#define	ptcselect	nodev
#define	ptsstop		nulldev
#endif

int	ttselect(), seltrue();

struct cdevsw	cdevsw[] = {
/* cn = 0 */
	cnopen,		cnclose,	cnread,		cnwrite,
	cnioctl,	nulldev,	cons,		ttselect,
	nulldev,
/* mem = 1 */
	nulldev,	nulldev,	mmrw,		mmrw,
	nodev,		nulldev,	0,		mmselect,
	nulldev,
/* tty = 2 */
	syopen,		nulldev,	syread,		sywrite,
	syioctl,	nulldev,	0,		syselect,
	nulldev,
/* log = 3 */
	logopen,	logclose,	logread,	nodev,
	logioctl,	nulldev,	0,		logselect,
	nulldev,
/* fd = 4 */
	fdopen,		nodev,		nodev,		nodev,
	nodev,		nodev,		0,		nodev,
	nodev,
/* ptc = 5 */
	ptcopen,	ptcclose,	ptcread,	ptcwrite,
	ptyioctl,	nulldev,	pt_tty,		ptcselect,
	nulldev,
/* pts = 6 */
	ptsopen,	ptsclose,	ptsread,	ptswrite,
	ptyioctl,	ptsstop,	pt_tty,		ttselect,
	nulldev,
/* rrd = 7 */
	rdopen,		nulldev,	rawrw,		rawrw,
	nodev,		nulldev,	0,		seltrue,
	rdstrategy,
/* rwd = 8 */
	wdopen,		wdclose,	rawrw,		rawrw,
	wdioctl,	nulldev,	0,		seltrue,
	wdstrategy,
/* rsd = 9 */
	sdopen,		sdclose,	rawrw,		rawrw,
	sdioctl,	nulldev,	0,		seltrue,
	sdstrategy,
/* com = 10 */
	comopen,	comclose,	comread,	comwrite,
	comioctl,	nulldev,	com_tty,	ttselect,
	nulldev,
/* rfl = 11 */
	flopen,		flclose,	rawrw,		rawrw,
	flioctl,	nulldev,	0,		seltrue,
	flstrategy,
};
int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 *
 * A minimal stub routine can always return 0.
 */
iskmemdev(dev)
	register dev_t dev;
{
	if (major(dev) == 1 && (minor(dev) == 0 || minor(dev) == 1))
		return (1);
	return (0);
}

/*
 * Routine to determine if a device is a disk.
 *
 * A minimal stub routine can always return 0.
 */
isdisk(dev, type)
	dev_t dev;
	register int type;
{
	switch (major(dev)) {
	case 0:			/* rd */
	case 1:			/* wd */
	case 2:			/* sd */
	case 3:			/* fl */
		return (type == IFBLK);
	case 7:			/* rrd */
	case 8:			/* rwd */
	case 9:			/* rsd */
	case 11:		/* rfl */
		return (type == IFCHR);
	}
	return (0);
}

#define MAXDEV	12
static char chrtoblktbl[MAXDEV] =  {
      /* CHR */      /* BLK */
	/* 0 */		NODEV,
	/* 1 */		NODEV,
	/* 2 */		NODEV,
	/* 3 */		NODEV,
	/* 4 */		NODEV,
	/* 5 */		NODEV,
	/* 6 */		NODEV,
	/* 7 */		0,		/* rd */
	/* 8 */		1,		/* wd */
	/* 9 */		2,		/* sd */
	/* 10 */	NODEV,
	/* 11 */	3,		/* fl */
};

/*
 * Routine to convert from character to block device number.
 *
 * A minimal stub routine can always return NODEV.
 */
chrtoblk(dev)
	register dev_t dev;
{
	register int blkmaj;

	if (major(dev) >= MAXDEV || (blkmaj = chrtoblktbl[major(dev)]) == NODEV)
		return (NODEV);
	return (makedev(blkmaj, minor(dev)));
}

/*
 * This routine returns the cdevsw[] index of the block device
 * specified by the input parameter.    Used by init_main and ioctl(SIOCSTTY).
 */
blktochr(dev)
	register dev_t dev;
{
	register int maj = major(dev);
	register int i;

	for (i = 0; i < MAXDEV; i++)
		if (maj == chrtoblktbl[i])
			return (i);
	return (NODEV);
}
