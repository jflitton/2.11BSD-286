/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_exec.c	1.8 (2.11BSD) 1999/9/6
 */

#include "param.h"
#include "../machine/reg.h"
#include "../machine/seg.h"
#include "../machine/psl.h"
#include "../machine/frame.h"

#include "systm.h"
#include "map.h"
#include "user.h"
#include "proc.h"
#include "buf.h"
#include "inode.h"
#include "acct.h"
#include "namei.h"
#include "fs.h"
#include "mount.h"
#include "file.h"
#include "text.h"
#include "signalvar.h"
extern	char	sigprop[];	/* XXX */

/*
 * exec system call, with and without environments.
 */
struct execa {
	char __far *fname;
	char __far *argp;
	char __far *envp;
};

/*
 * A pointer in the lists of arguments and environment is the size of a
 * data pointer in the program concerned: an offset alone, in the same
 * segment as the list, or an offset and a selector.
 */
#define	PTRSIZE(model)	((model) == A_LARGE ? 2 * NBPW : NBPW)
#define	BADPTR		((uaddr_t)-1L)

static uaddr_t
fuptr(addr, psz)
	uaddr_t addr;
	int psz;
{
	register int off, sel;

	if ((off = fuword(addr)) == -1)
		return (BADPTR);
	if (psz == NBPW)
		return (off ? UADDR(USEL(addr), off) : (uaddr_t)NULL);
	if ((sel = fuword(addr + NBPW)) == -1)
		return (BADPTR);
	return (UADDR(sel, off));
}

static
suptr(addr, ptr, psz)
	uaddr_t addr, ptr;
	int psz;
{
	(void) suword(addr, UOFF(ptr));
	if (psz != NBPW)
		(void) suword(addr + NBPW, USEL(ptr));
}

execv()
{
	((struct execa *)u.u_ap)->envp = NULL;
	execve();
}

execve()
{
	int nc;
	register char __far *cp;
	register struct buf *bp;
	struct execa *uap = (struct execa *)u.u_ap;
	int na, ne;
	uaddr_t ap, ucp;
	register int cc;
	unsigned len;
	int indir, uid, gid, psz;
	char *sharg;
	struct inode *ip;
	memaddr bno;
	char cfname[MAXCOMLEN + 1];
#define	SHSIZE	32
	char cfarg[SHSIZE];
	union {
		char	ex_shell[SHSIZE];	/* #! and name of interpreter */
		struct	exec ex_exec;
	} exdata;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;
	int resid, error;

	NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, uap->fname);
	if ((ip = namei(ndp)) == NULL)
		return;
	bno = 0;
	bp = 0;
	indir = 0;
	uid = u.u_uid;
	gid = u.u_groups[0];
	if (ip->i_fs->fs_flags & MNT_NOEXEC) {
		u.u_error = EACCES;
		goto bad;
	}
	if ((ip->i_fs->fs_flags & MNT_NOSUID) == 0) {
		if (ip->i_mode & ISUID)
			uid = ip->i_uid;
		if (ip->i_mode & ISGID)
			gid = ip->i_gid;
	}
  again:
	if (access(ip, IEXEC))
		goto bad;
	if ((u.u_procp->p_flag & P_TRACED) && access(ip, IREAD))
		goto bad;
	if ((ip->i_mode & IFMT) != IFREG ||
	    (ip->i_mode & (IEXEC|(IEXEC>>3)|(IEXEC>>6))) == 0) {
		u.u_error = EACCES;
		goto bad;
	}

	/*
	 * Read in first few bytes of file for the magic number and the
	 * rest of the header (exec.h).
	 * Also an ASCII line beginning with #! is
	 * the file name of a ``shell'' and arguments may be prepended
	 * to the argument list if given here.
	 *
	 * SHELL NAMES ARE LIMITED IN LENGTH.
	 *
	 * ONLY ONE ARGUMENT MAY BE PASSED TO THE SHELL FROM
	 * THE ASCII LINE.
	 */
	exdata.ex_shell[0] = '\0';	/* for zero length files */
	u.u_error = rdwri(UIO_READ, ip, &exdata, sizeof(exdata), (off_t)0,
				UIO_SYSSPACE, IO_UNIT, &resid);
	if (u.u_error)
		goto bad;
	if (resid > sizeof(exdata) - sizeof(exdata.ex_exec) &&
	    exdata.ex_shell[0] != '#') {
		u.u_error = ENOEXEC;
		goto bad;
	}

	if (exdata.ex_exec.a_magic != A_MAGIC) {
		if (exdata.ex_shell[0] != '#' ||
		    exdata.ex_shell[1] != '!' ||
		    indir) {
			u.u_error = ENOEXEC;
			goto bad;
		}
/*
 * If setuid/gid scripts were to be disallowed this is where it would
 * have to be done.
 *		u.u_uid = uid;
 *		u.u_gid = u_groups[0];
*/
		{
		register char *cp;

		cp = &exdata.ex_shell[2];		/* skip "#!" */
		while (cp < &exdata.ex_shell[SHSIZE]) {
			if (*cp == '\t')
				*cp = ' ';
			else if (*cp == '\n') {
				*cp = '\0';
				break;
			}
			cp++;
		}
		if (*cp != '\0') {
			u.u_error = ENOEXEC;
			goto bad;
		}
		cp = &exdata.ex_shell[2];
		while (*cp == ' ')
			cp++;
		ndp->ni_dirp = cp;
		while (*cp && *cp != ' ')
			cp++;
		cfarg[0] = '\0';
		if (*cp) {
			*cp++ = '\0';
			while (*cp == ' ')
				cp++;
			if (*cp)
				bcopy((caddr_t)cp, (caddr_t)cfarg, SHSIZE);
		}
		}
		indir = 1;
		iput(ip);
		ndp->ni_nameiop = LOOKUP | FOLLOW;
		ndp->ni_segflg = UIO_SYSSPACE;
		ip = namei(ndp);
		if (ip == NULL)
			return;
		bcopy((caddr_t)ndp->ni_dent.d_name, (caddr_t)cfname, MAXCOMLEN);
		cfname[MAXCOMLEN] = '\0';
		goto again;
	}

	/*
	 * Collect arguments on "file" in swap space.
	 */
	na = 0;
	ne = 0;
	nc = 0;
	cc = 0;
	psz = PTRSIZE(u.u_model);
	bno = malloc(swapmap, ctod((int)btoc(NCARGS + MAXBSIZE)));
	if (bno == 0) {
		swkill(u.u_procp, "exec");
		goto bad;
	}
	/*
	 * Copy arguments into file in argdev area.
	 */
	if (uap->argp) for (;;) {
		ap = NULL;
		sharg = NULL;
		if (indir && na == 0) {
			sharg = cfname;
			ap = (uaddr_t)sharg;
			uap->argp += psz;	/* ignore argv[0] */
		} else if (indir && (na == 1 && cfarg[0])) {
			sharg = cfarg;
			ap = (uaddr_t)sharg;
		} else if (indir && (na == 1 || na == 2 && cfarg[0]))
			ap = uap->fname;
		else if (uap->argp) {
			ap = fuptr(uap->argp, psz);
			uap->argp += psz;
		}
		if (ap == NULL && uap->envp) {
			uap->argp = NULL;
			if ((ap = fuptr(uap->envp, psz)) != NULL)
				uap->envp += psz, ne++;
		}
		if (ap == NULL)
			break;
		na++;
		if (ap == BADPTR) {
			u.u_error = EFAULT;
			break;
		}
		do {
			if (cc <= 0) {
				/*
				 * We depend on NCARGS being a multiple of
				 * CLSIZE*NBPG.  This way we need only check
				 * overflow before each buffer allocation.
				 */
				if (nc >= NCARGS-1) {
					error = E2BIG;
					break;
				}
				if (bp) {
					mapout(bp);
					bdwrite(bp);
				}
				cc = CLSIZE*NBPG;
				bp = getblk(swapdev, dbtofsb(clrnd(bno)) + lblkno(nc));
				cp = mapin(bp);
			}
			if (sharg) {
				error = copystr(sharg, cp, (unsigned)cc, &len);
				sharg += len;
			} else {
				error = copyinstr(ap, cp, (unsigned)cc, &len);
				ap += len;
			}
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENOENT);
		if (error) {
			u.u_error = error;
			if (bp) {
				mapout(bp);
				bp->b_flags |= B_AGE;
				bp->b_flags &= ~B_DELWRI;
				brelse(bp);
			}
			bp = 0;
			goto badarg;
		}
	}
	if (bp) {
		mapout(bp);
		bdwrite(bp);
	}
	bp = 0;
	nc = (nc + NBPW-1) & ~(NBPW-1);
	psz = PTRSIZE(exdata.ex_exec.a_model);
	if (u.u_error == 0)
		getxfile(ip, &exdata.ex_exec, nc + (na+2)*psz + 2*NBPW, uid,
		    gid);
	if (u.u_error) {
badarg:
		for (cc = 0;cc < nc; cc += CLSIZE * NBPG) {
			daddr_t blkno;

			blkno = dbtofsb(clrnd(bno)) + lblkno(cc);
			if (incore(swapdev,blkno)) {
				bp = bread(swapdev,blkno);
				bp->b_flags |= B_AGE;		/* throw away */
				bp->b_flags &= ~B_DELWRI;	/* cancel io */
				brelse(bp);
				bp = 0;
			}
		}
		goto bad;
	}
	iput(ip);
	ip = NULL;

	/*
	 * Copy back arglist: at the top of the stack a word of zero, below
	 * it the strings, below them the count of arguments, the pointers
	 * to them, a null, the pointers to the environment and a null.
	 * getxfile left the stack pointer at the top of the stack.
	 */
	ucp = UADDR(u.u_ar0[SS], u.u_ar0[SP] - nc - NBPW);
	ap = ucp - na*psz - 2*psz - NBPW;
	u.u_ar0[SP] = UOFF(ap);
	(void) suword(ap, na-ne);
	ap += NBPW;
	nc = 0;
	cc = 0;
	for (;;) {
		if (na == ne) {
			suptr(ap, (uaddr_t)NULL, psz);
			ap += psz;
		}
		if (--na < 0)
			break;
		suptr(ap, ucp, psz);
		ap += psz;
		do {
			if (cc <= 0) {
				if (bp) {
					mapout(bp);
					brelse(bp);
				}
				cc = CLSIZE*NBPG;
				bp = bread(swapdev, dbtofsb(clrnd(bno)) + lblkno(nc));
				bp->b_flags |= B_AGE;		/* throw away */
				bp->b_flags &= ~B_DELWRI;	/* cancel io */
				cp = mapin(bp);
			}
			error = copyoutstr(cp, ucp, (unsigned)cc, &len);
			ucp += len;
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENOENT);
		if (error == EFAULT)
			panic("exec: EFAULT");
	}
	suptr(ap, (uaddr_t)NULL, psz);
	if (bp) {
		mapout(bp);
		bp->b_flags |= B_AGE;
		brelse(bp);
		bp = NULL;
	}
	execsigs(u.u_procp);
	{
	register char *cp;

	for	(cp = u.u_pofile, cc = 0; cc <= u.u_lastfile; cc++, cp++)
		{
		if	(*cp & UF_EXCLOSE)
			{
			(void)closef(u.u_ofile[cc]);
			u.u_ofile[cc] = NULL;
			*cp = 0;
			}
		}
	}
	while (u.u_lastfile >= 0 && u.u_ofile[u.u_lastfile] == NULL)
		u.u_lastfile--;

	/*
	 * Remember file name for accounting.
	 */
	u.u_acflag &= ~AFORK;
	if (indir)
		bcopy((caddr_t)cfname, (caddr_t)u.u_comm, MAXCOMLEN);
	else
		bcopy((caddr_t)ndp->ni_dent.d_name, (caddr_t)u.u_comm, MAXCOMLEN);
	/*
	 * The system call returns to the new program: it is not to have
	 * the registers a result would be put in.
	 */
	u.u_error = EJUSTRETURN;
bad:
	if (bp) {
		mapout(bp);
		bp->b_flags |= B_AGE;
		brelse(bp);
	}
	if (bno)
		mfree(swapmap, ctod((int)btoc(NCARGS + MAXBSIZE)), bno);
	if (ip)
		iput(ip);
}

/*
 * Reset signals for an exec of the specified process.  In 4.4 this function
 * was in kern_sig.c but since in 2.11 kern_sig and kern_exec will likely be
 * in different overlays placing this here potentially saves a kernel overlay
 * switch.
 */
void
execsigs(p)
	register struct proc *p;
{
	register int nc;
	unsigned long mask;

	/*
	 * Reset caught signals.  Held signals remain held
	 * through p_sigmask (unless they were caught,
	 * and are now ignored by default).
	 */
	while (p->p_sigcatch) {
		nc = ffs(p->p_sigcatch);
		mask = sigmask(nc);
		p->p_sigcatch &= ~mask;
		if (sigprop[nc] & SA_IGNORE) {
			if (nc != SIGCONT)
				p->p_sigignore |= mask;
			p->p_sig &= ~mask;
		}
		u.u_signal[nc] = SIG_DFL;
	}
	/*
	 * Reset stack state to the user stack (disable the alternate stack).
	 */
	u.u_sigstk.ss_flags = SA_DISABLE;
	u.u_sigstk.ss_size = 0;
	u.u_sigstk.ss_base = 0;
	u.u_psflags = 0;
}
/*
 * Read the table entry for segment n of an executable file.
 */
static
getxseg(ip, n, xs)
	struct inode *ip;
	int n;
	register struct exseg *xs;
{
	return (rdwri(UIO_READ, ip, xs, sizeof (*xs),
	    (off_t)sizeof (struct exec) + (off_t)(n - 1) * sizeof (*xs),
	    UIO_SYSSPACE, IO_UNIT, (int *)0));
}

/*
 * Read size bytes of an executable file, from offset, into segment n of
 * the running process.  A whole segment is one byte more than a count
 * can say, so it goes in two pieces.
 */
static
getxdata(ip, n, size, offset)
	struct inode *ip;
	int n;
	long size;
	off_t offset;
{
	register u_int count;
	register int error;
	u_int done = 0;

	while (size) {
		count = size > 0x8000 ? 0x8000 : size;
		if (error = rdwri(UIO_READ, ip, UADDR(LSEL(n), done),
		    (int)count, offset, UIO_USERSPACE, IO_UNIT, (int *)0))
			return (error);
		done += count;
		offset += count;
		size -= count;
	}
	return (0);
}

/*
 * Read in and set up memory for executed file.
 * u.u_error set on error
 *
 * The program's code segments become, or already are, the memory of the
 * file's text structure, one after another, and the process's table
 * says where in it each is.  Every data segment is a segment of the
 * process's own.  The one the header names holds the stack as well,
 * after the data, and above that is where the break starts.  The stack
 * is the size the header gives with nargc bytes more for the arguments
 * and environment, which execve copies to the top of it, rounded up to
 * a click.
 */
getxfile(ip, ep, nargc, uid, gid)
	register struct inode *ip;
	register struct exec *ep;
	int nargc, uid, gid;
{
	register struct proc *p = u.u_procp;
	register struct pseg __far *sp;
	register struct trapframe *tf;
	struct exseg xs;
	register int n;
	u_int ts, startc;
	long size, total, stack;
	off_t offset;

	if (ep->a_nseg == 0 || ep->a_nseg >= NSEG ||
	    ep->a_eseg == 0 || ep->a_eseg > ep->a_nseg ||
	    ep->a_dseg == 0 || ep->a_dseg > ep->a_nseg ||
	    ep->a_model < A_SMALL || ep->a_model > A_LARGE) {
		u.u_error = ENOEXEC;
		return;
	}
	if (ip->i_text && (ip->i_text->x_flag & XTRC)) {
		u.u_error = ETXTBSY;
		return;
	}
	if ((ip->i_flag&ITEXT) == 0 && ip->i_count != 1) {
		register struct file *fp;

		for (fp = file; fp < fileNFILE; fp++) {
			if (fp->f_type == DTYPE_INODE &&
			    fp->f_count > 0 &&
			    (struct inode *)fp->f_data == ip &&
			    (fp->f_flag&FWRITE)) {
				u.u_error = ETXTBSY;
				return;
			}
		}
	}

	/*
	 * find text and data sizes try; them out for possible
	 * overflow of max sizes
	 */
	stack = ctob(btoc((long)ep->a_stack + nargc));
	ts = 0;
	total = 0;
	for (n = 1; n <= ep->a_nseg; n++) {
		if (u.u_error = getxseg(ip, n, &xs))
			return;
		size = xs.xs_memsz;
		if (n == ep->a_dseg) {
			if (xs.xs_type != XS_DATA)
				size = 0;
			else if (size + ep->a_stack <= 0x10000L &&
			    size + stack > 0x10000L) {
				u.u_error = E2BIG;
				return;
			}
			size += stack;
		}
		if (size == 0 || size > 0x10000L || xs.xs_filesz > xs.xs_memsz ||
		    (xs.xs_type != XS_CODE && xs.xs_type != XS_DATA) ||
		    (n == ep->a_eseg && xs.xs_type != XS_CODE)) {
			u.u_error = ENOEXEC;
			return;
		}
		if (xs.xs_type == XS_CODE)
			ts += btoc(size);
		total += btoc(size);
	}
	if (total + USIZE > maxmem) {
		u.u_error = ENOMEM;
		return;
	}

	/*
	 * allocate and clear core at this point, committed
	 * to the new image
	 */
	u.u_prof.pr_scale = 0;
	if (p->p_flag & SVFORK)
		endvfork();
	else
		xfree();
	for (n = 1; n < NSEG; n++)
		segfree(n);
	fpdrop();

	offset = sizeof (struct exec) + ep->a_nseg * (off_t)sizeof (xs);
	ts = 0;
	for (n = 1; n <= ep->a_nseg; n++) {
		if (u.u_error = getxseg(ip, n, &xs))
			goto dead;
		sp = &PSEG(p)[n];
		size = xs.xs_memsz;
		if (xs.xs_type == XS_CODE) {
			sp->ps_addr = ts;
			sp->ps_size = btoc(size);
			sp->ps_limit = size - 1;
			sp->ps_flag = PS_USED | PS_TEXT;
			ts += sp->ps_size;
		} else {
			if (n == ep->a_dseg)
				size += stack;
			expand((int)btoc(size), n);
			sp->ps_limit = size - 1;
			sp->ps_flag = PS_USED;
			setldt(p, n);
			/* clear what the file does not have */
			startc = xs.xs_filesz >> PGSHIFT;
			clear(sp->ps_addr + startc, sp->ps_size - startc);
			if (u.u_error = getxdata(ip, n, xs.xs_filesz, offset))
				goto dead;
		}
		offset += (xs.xs_filesz + 1) & ~1L;
	}
	if (ts) {
		xalloc(ip, ep, ts);
		if (u.u_error)
			goto dead;
	}
	sureg();

	/*
	 * set SUID/SGID protections, if no tracing
	 */
	if ((p->p_flag & P_TRACED)==0) {
		u.u_uid = uid;
		p->p_uid = uid;
		u.u_groups[0] = gid;
	} else
		psignal(p, SIGTRAP);
	u.u_svuid = u.u_uid;
	u.u_svgid = u.u_groups[0];
	u.u_acflag &= ~ASUGID;	/* start fresh setuid/gid priv use */

	u.u_tsize = ts;
	u.u_dsize = p->p_dsize;
	u.u_ssize = btoc(stack);
	u.u_model = ep->a_model;
	u.u_dseg = ep->a_dseg;
	sp = &PSEG(p)[ep->a_dseg];
	u.u_break = sp->ps_limit + 1;
	u.u_stack0 = u.u_break - stack;

	/*
	 * The registers the program starts with.  BX is the break, which
	 * the program has no other way to know while its stack lies
	 * between its data and the break.
	 */
	tf = (struct trapframe *)u.u_ar0;
	bzero((caddr_t)tf, sizeof (*tf));
	tf->tf_bx = u.u_break;
	tf->tf_cs = LSEL(ep->a_eseg);
	tf->tf_ip = ep->a_entry;
	tf->tf_ds = tf->tf_es = tf->tf_ss = LSEL(ep->a_dseg);
	tf->tf_sp = u.u_break;
	tf->tf_flags = PSL_USERSET;
	return;

dead:
	/*
	 * There is no going back to the old program, and the new one is
	 * not all there.
	 */
	psignal(p, SIGKILL);
}

/*
 * Read a program's code into the memory of its text structure, which is
 * new.  The process's table already says where each code segment is in
 * it.  The descriptor for each is made writable data while the segment
 * is read.  Called by xalloc.
 */
xread(ip, ep)
	struct inode *ip;
	register struct exec *ep;
{
	register struct proc *p = u.u_procp;
	register struct pseg __far *sp;
	struct exseg xs;
	register int n;
	off_t offset;
	int error;

	offset = sizeof (struct exec) + ep->a_nseg * (off_t)sizeof (xs);
	for (n = 1; n <= ep->a_nseg; n++) {
		if (error = getxseg(ip, n, &xs))
			return (error);
		if (xs.xs_type == XS_CODE) {
			sp = &PSEG(p)[n];
			sp->ps_flag |= PS_WTEXT;
			setldt(p, n);
			error = getxdata(ip, n, xs.xs_filesz, offset);
			sp->ps_flag &= ~PS_WTEXT;
			setldt(p, n);
			if (error)
				return (error);
		}
		offset += (xs.xs_filesz + 1) & ~1L;
	}
	return (0);
}
