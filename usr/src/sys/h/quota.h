/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)quota.h	7.1.1 (2.11BSD) 1996/1/23
 */

/*
 * MELBOURNE DISC QUOTAS
 *
 * Various junk to do with various quotas (etc) imposed upon
 * the average user (big brother finally hits UNIX).
 *
 * The following structure exists in core for each logged on user.
 * It contains global junk relevant to that user's quotas.
 *
 * The u_quota field of each user struct contains a pointer to
 * the quota struct relevant to the current process, this is changed
 * by 'setuid' sys call, &/or by the Q_SETUID quota() call.
 */
struct quota {
	struct	quota __far *q_forw, __far *q_back;	/* hash chain, MUST be first */
	short	q_cnt;			/* ref count (# processes) */
	uid_t	q_uid;			/* real uid of owner */
	int	q_flags;		/* struct management flags */
#define	Q_LOCK	0x01		/* quota struct locked (for disc i/o) */
#define	Q_WANT	0x02		/* issue a wakeup when lock goes off */
#define	Q_NEW	0x04		/* new quota - no proc1 msg sent yet */
#define	Q_NDQ	0x08		/* account has NO disc quota */
	struct	quota __far *q_freef, __far * __far *q_freeb;
	struct	dquot __far *q_dq[NMOUNT];	/* disc quotas for mounted filesys's */
};

#define	NOQUOTA	((struct quota __far *) 0)

#if defined(KERNEL) && defined(QUOTA) && !defined(SUPERVISOR)
extern struct	quota __far *quota, __far *quotaNQUOTA;
extern int	nquota;
#endif

/*
 * The following structure defines the format of the disc quota file
 * (as it appears on disc) - the file is an array of these structures
 * indexed by user number.  The setquota sys call establishes the inode
 * for each quota file (a pointer is retained in the mount structure).
 *
 * The following constants define the number of warnings given a user
 * before the soft limits are treated as hard limits (usually resulting
 * in an allocation failure).  The warnings are normally manipulated
 * each time a user logs in through the Q_DOWARN quota call.  If
 * the user logs in and is under the soft limit the warning count
 * is reset to MAX_*_WARN, otherwise a message is printed and the
 * warning count is decremented.  This makes MAX_*_WARN equivalent to
 * the number of logins before soft limits are treated as hard limits.
 */
#define	MAX_IQ_WARN	3
#define	MAX_DQ_WARN	3

struct	dqblk {
	u_long	dqb_bhardlimit;	/* absolute limit on disc blks alloc */
	u_long	dqb_bsoftlimit;	/* preferred limit on disc blks */
	u_long	dqb_curblocks;	/* current block count */
	u_short	dqb_ihardlimit;	/* maximum # allocated inodes + 1 */
	u_short	dqb_isoftlimit;	/* preferred inode limit */
	u_short	dqb_curinodes;	/* current # allocated inodes */
	u_char	dqb_bwarn;	/* # warnings left about excessive disc use */
	u_char	dqb_iwarn;	/* # warnings left about excessive inodes */
};

/*
 * The following structure records disc usage for a user on a filesystem.
 * There is one allocated for each quota that exists on any filesystem
 * for the current user. A cache is kept of other recently used entries.
 */
struct	dquot {
	struct	dquot __far *dq_forw, __far *dq_back;/* MUST be first entry */
	union	{
		struct	quota __far *Dq_own;	/* the quota that points to this */
		struct {		/* free list */
			struct	dquot __far *Dq_freef, __far * __far *Dq_freeb;
		} dq_f;
	} dq_u;
	short	dq_flags;
#define	DQ_LOCK		0x01		/* this quota locked (no MODS) */
#define	DQ_WANT		0x02		/* wakeup on unlock */
#define	DQ_MOD		0x04		/* this quota modified since read */
#define	DQ_FAKE		0x08		/* no limits here, just usage */
#define	DQ_BLKS		0x10		/* has been warned about blk limit */
#define	DQ_INODS	0x20		/* has been warned about inode limit */
	short	dq_cnt;			/* count of active references */
	uid_t	dq_uid;			/* user this applies to */
	dev_t	dq_dev;			/* filesystem this relates to */
	struct dqblk dq_dqb;		/* actual usage & quotas */
};

#define	dq_own		dq_u.Dq_own
#define	dq_freef	dq_u.dq_f.Dq_freef
#define	dq_freeb	dq_u.dq_f.Dq_freeb
#define	dq_bhardlimit	dq_dqb.dqb_bhardlimit
#define	dq_bsoftlimit	dq_dqb.dqb_bsoftlimit
#define	dq_curblocks	dq_dqb.dqb_curblocks
#define	dq_ihardlimit	dq_dqb.dqb_ihardlimit
#define	dq_isoftlimit	dq_dqb.dqb_isoftlimit
#define	dq_curinodes	dq_dqb.dqb_curinodes
#define	dq_bwarn	dq_dqb.dqb_bwarn
#define	dq_iwarn	dq_dqb.dqb_iwarn

#define	NODQUOT		((struct dquot __far *) 0)
#define	LOSTDQUOT	((struct dquot __far *) 1)

#if defined(KERNEL) && defined(QUOTA) && !defined(SUPERVISOR)
extern struct	dquot __far *dquot, __far *dquotNDQUOT;
extern int	ndquot;
#endif

#define	QUOTAFILENAME	"quotas"
#define	QUOTAGROUP	"operator"

/*
 * Definitions for the 'quota' system call.
 */
#define	Q_SETDLIM	1	/* set disc limits & usage */
#define	Q_GETDLIM	2	/* get disc limits & usage */
#define	Q_SETDUSE	3	/* set disc usage only */
#define	Q_SYNC		4	/* update disc copy of quota usages */
#define	Q_SETUID	16	/* change proc to use quotas for uid */
#define	Q_SETWARN	25	/* alter inode/block warning counts */
#define	Q_DOWARN	26	/* warn user about excessive space/inodes */

/*
 * Used in Q_SETDUSE.
 */
struct	dqusage {
	u_short	du_curinodes;
	u_long	du_curblocks;
};

/*
 * Used in Q_SETWARN.
 */
struct	dqwarn {
	u_char	dw_bwarn;
	u_char	dw_iwarn;
};

#if defined(KERNEL) && defined(QUOTA) && !defined(SUPERVISOR)
#include "../machine/seg.h"

struct	qhash {
	struct	qhash __far *qh_forw;
	struct	qhash __far *qh_back;
};

struct	dqhead {
	struct	dqhead __far *dqh_forw;
	struct	dqhead __far *dqh_back;
};

#ifdef pdp11
#define	QUOTAMAP()	mapseg5(quotreg, quotdesc)
#define	QUOTAUNMAP()	normalseg5()

memaddr	quotreg;
u_short	quotdesc;
#else
/*
 * The 286 has the quotas in far data of their own (param.c), where the
 * PDP-11 maps them in at SEG5, and reaches them through far pointers:
 * there is nothing to map.  A process waiting for a quota or a dquot
 * sleeps on its offset, (caddr_t)q, the kernel's channels being 16
 * bits; one of the kernel's own at the same address is woken with it,
 * and the other way round, and each looks again, as every sleeper here
 * does.
 */
#define	QUOTAMAP()
#define	QUOTAUNMAP()
#endif
extern struct	dquot __far * __far *ix_dquot;

#define	NQHASH		16	/* small power of 2 */
#define	NDQHASH		37	/* 4.3bsd used 51 which isn't even prime */
#define	NQUOTA		40
#define	NDQUOT		150

/*
 * What takes or gives a far pointer has a prototype, so that the
 * compiler checks it.
 */
struct	inode; struct mount;
struct	quota __far *getquota(uid_t uid, int lookuponly, int nodq);
struct	quota __far *qfind(uid_t uid);
int	delquota(struct quota __far *q);
struct	dquot __far *discquota(uid_t uid, struct inode *ip);
struct	dquot __far *dqalloc(uid_t uid, dev_t dev);
struct	dquot __far *dqp(struct quota __far *q, dev_t dev);
struct	dquot __far *inoquota(struct inode *ip);
int	dqrele(struct dquot __far *dq);
int	putdq(struct mount *mp, struct dquot __far *dq, int free);
int	opendq(struct mount *mp, char __far *fname);
int	qstart(struct quota __far *q);
int	qwarn(struct dquot __far *dq);
#endif
