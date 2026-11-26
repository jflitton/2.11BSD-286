#include "defs.h"
#include <sys/file.h>
#include <sys/stat.h>

	MAP	txtmap;
	MAP	datmap;
	int	wtflag;
	int	kernel;
	int	fcor;
	int	fsym;
	long	maxfile;
	long	txtsiz;
	long	datsiz;
	long	bss;
	long	datbas;
	long	stksiz;
	char	*errflg;
	int	magic;
	int	model;
	long	entrypt;
	long	var[];
	int	argcount;
	int	signo;
	u_int	corhdr[ctob(USIZE)/sizeof(u_int)];
	u_int	*uar0 = &corhdr[UAR0];
	char	*symfil  = "a.out";
	char	*corfil  = "core";
	struct	xexec	xhdr;
	struct	pseg	pseg[NSEG];
	off_t	symoff, stroff;

/*
 * The selector of segment n of the program, counting from 1: the slot of
 * the process's table, or of the kernel's in the global table.
 */
u_int
segsel(n)
	int	n;
{
	IF n <= 0
	THEN	return(0);
	ELIF kernel
	THEN	return(SEL(GDT_KERNEL + n - 1));
	ELSE	return(LSEL(n));
	FI
}

/* ISYM for a selector of a code segment of the program, else DSYM */
segtype(sel)
	register u_int sel;
{
	register struct seg *sp;

	FOR sp = txtmap.seg; sp < &txtmap.seg[txtmap.nseg]; sp++
	DO	IF SAMESEG(sp->sel, sel)
		THEN	return(sp->type == XS_CODE ? ISYM : DSYM);
		FI
	OD
	return(DSYM);
}

/*
 * The program: its segments, from the header and the table of them
 * (sys/exec.h), and its symbols.  A file of another format is one
 * stretch of bytes.
 */
setsym()
{
	register int n;
	register struct seg *sp;
	register struct exseg *xp;
	long	off;

	bzero(&txtmap, sizeof (txtmap));
	fsym=getfile(symfil,1);
	txtmap.ufd=fsym;

	IF read(fsym, &xhdr.e, sizeof (xhdr.e)) == sizeof (xhdr.e)
	    ANDF xhdr.e.a_magic == A_MAGIC
	    ANDF xhdr.e.a_nseg > 0 ANDF xhdr.e.a_nseg <= NXSEG
	    ANDF read(fsym, xhdr.x, xhdr.e.a_nseg * sizeof (struct exseg))
		== xhdr.e.a_nseg * sizeof (struct exseg)
	THEN	magic = A_MAGIC;
		model = xhdr.e.a_model;
		off = N_TXTOFF(xhdr.e);
		FOR n = 0; n < xhdr.e.a_nseg; n++
		DO	sp = &txtmap.seg[n];
			xp = &xhdr.x[n];
			sp->sel = segsel(n + 1);
			sp->type = xp->xs_type;
			sp->b = 0;
			sp->e = xp->xs_filesz;
			sp->z = xp->xs_memsz;
			sp->f = off;
			off += (xp->xs_filesz + 1) & ~1L;
			IF sp->type == XS_CODE
			THEN	txtsiz += xp->xs_memsz;
			ELSE	datsiz += xp->xs_filesz;
				bss += xp->xs_memsz - xp->xs_filesz;
			FI
		OD
		txtmap.nseg = n;
		txtmap.def1 = segsel(xhdr.e.a_eseg);
		txtmap.def2 = segsel(xhdr.e.a_dseg);
		entrypt = ADDR(txtmap.def1, xhdr.e.a_entry);
		stksiz = xhdr.e.a_stack;
		datbas = ADDR(txtmap.def2, 0);
		symoff = N_SYMOFF(xhdr);
		stroff = N_STROFF(xhdr);
		symINI(&xhdr.e);
	ELSE	magic = 0;
		txtmap.e1 = maxfile;
	FI
}

/*
 * The core file, or with -k the kernel's memory.
 *
 * A core file (sys/kern_sig.c, core()) is the user structure, which has
 * the registers at its top (UAR0), then the process's table of segments,
 * then the contents of each segment that is in use and is not the
 * program's shared code, in the order of the table.  The code is read
 * from the program.
 *
 * /dev/kmem takes the kernel's segment N, counting from 1, at N << 16.
 */
setcor()
{
	register int n;
	register struct seg *sp;
	register struct pseg *ps;
	long	off;
	struct	stat st;

	fcor=getfile(corfil,2);
	bzero(&datmap, sizeof (datmap));
	datmap.ufd=fcor;
	datmap.def1 = datmap.def2 = txtmap.def2;
	IF kernel ANDF magic
	THEN	FOR n = 0; n < txtmap.nseg; n++
		DO	sp = &datmap.seg[n];
			*sp = txtmap.seg[n];
			sp->b = 0;
			sp->e = sp->z;
			sp->f = (long)(n + 1) << 16;
		OD
		datmap.nseg = n;
		stksiz = 0;
		return;
	FI
	IF fcor >= 0 ANDF fstat(fcor, &st) == 0
	    ANDF read(fcor, corhdr, sizeof corhdr) == sizeof corhdr
	    ANDF read(fcor, pseg, sizeof pseg) == sizeof pseg
	THEN	off = sizeof corhdr + sizeof pseg;
		FOR n = 0, ps = pseg; n < NSEG; n++, ps++
		DO	IF (ps->ps_flag & (PS_USED|PS_TEXT)) != PS_USED
			THEN	continue;
			FI
			sp = &datmap.seg[datmap.nseg++];
			sp->sel = LSEL(n);
			sp->type = ps->ps_flag & PS_CODE ? XS_CODE : XS_DATA;
			sp->b = 0;
			sp->e = sp->z = (long)ps->ps_limit + 1;
			sp->f = off;
			off += sp->e;
		OD
		/* what is not a core file has no such table */
		IF datmap.nseg == 0 ORF off > st.st_size
		THEN	bzero(datmap.seg, sizeof (datmap.seg));
			datmap.nseg = 0;
			datmap.e1 = maxfile;
			return;
		FI
		datmap.def1 = LSEL(((U *)corhdr)->u_dseg);
		datmap.def2 = uar0[SS];
		txtmap.def1 = uar0[CS];
		txtsiz = ctob((long)((U *)corhdr)->u_tsize);
		datsiz = ctob((long)((U *)corhdr)->u_dsize);
		stksiz = ctob((long)((U *)corhdr)->u_ssize);
		datbas = ADDR(datmap.def1, 0);
	ELSE	datmap.e1 = maxfile;
	FI
}

getfile(filnam,cnt)
	char	*filnam;
	int	cnt;
{
	register int f;

	IF strcmp("-",filnam)
	THEN    f = open(filnam,wtflag);
		IF f < 0 ANDF argcount>cnt
		THEN    IF wtflag
			THEN    f = open(filnam, O_CREAT|O_TRUNC|wtflag,0644);
			FI
			IF f < 0
			THEN printf("cannot open `%s'\n", filnam);
			FI
		FI
	ELSE    f = -1;
	FI
	return(f);
}
