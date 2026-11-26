#include "defs.h"

	MSG	BADDAT;
	MSG	BADTXT;
	MAP	txtmap;
	MAP	datmap;
	int	wtflag;
	char	*errflg;
	int	pid;
	BKPTR	bkpthead;
	static	within();
	static	struct seg *findseg();
	static	long nextadr();
	static	int bkget(), bkput(), pword();
extern	int	errno;

/* file handling and access routines */

put(adr,space,value)
	long   adr;
	int	space, value;
{
	acces(WT,adr,space,value);
}

u_int
get(adr, space)
	long	adr;
	int	space;
{
	return(acces(RD,adr,space,0));
}

u_int
chkget(n, space)
	long	n;
	int	space;
{
	register int w;

	w = get(n, space);
	chkerr();
	return(w);
}

/*
 * An address with its segment: one whose selector is 0 is in the
 * segment that its space has by default.  In a file of no format adb
 * knows, which has none, it is as it was.
 */
long
fulladr(adr, space)
	long	adr;
	int	space;
{
	register MAPPTR amap;
	register u_int sel;

	IF ASEL(adr) ORF space == NSP
	THEN	return(adr);
	FI
	amap = (space&DSP ? &datmap : &txtmap);
	sel = (space&STAR ? amap->def2 : amap->def1);
	return(sel ? ADDR(sel, AOFF(adr)) : adr);
}

/*
 * A word, in the process with ptrace or in a file.  On the 286 a word
 * can be at an odd address.  The second byte of a word at the last byte
 * of a segment is not there, and reads as 0.
 */
acces(mode,adr,space,value)
	long	adr;
	int	mode, space, value;
{
	int	w, w1, rd, file;
	long	a;
	char	b[2], *e;

	rd = mode==RD;
	IF space == NSP THEN return(0); FI
	adr = fulladr(adr, space);

	IF pid          /* tracing on? */
	THEN IF !rd THEN value = bkput(adr, value); FI
	     w = pword(rd, space, adr, value);
	     IF rd THEN w = bkget(adr, w); FI
	     return(w);
	FI

	IF mode==WT ANDF wtflag==0
	THEN    error("not in write mode");
	FI
	a = adr;
	IF !chkmap(&a, space, !rd, &file)
	THEN return(0);
	FI
	IF rd
	THEN	b[0] = b[1] = 0;
		IF a >= 0 ANDF (lseek(file, a, 0) == -1L ORF read(file, b, 1) != 1)
		THEN	errflg = (space & DSP ? BADDAT : BADTXT);
			return(0);
		FI
		e = errflg;
		a = nextadr(adr);
		IF a != -1L ANDF chkmap(&a, space, 0, &file) ANDF a >= 0
		THEN	IF lseek(file, a, 0) == -1L ORF read(file, b + 1, 1) != 1
			THEN	b[1] = 0;
			FI
		FI
		errflg = e;
		return((b[0] & LOBYTE) | (b[1] & LOBYTE) << 8);
	FI
	w1 = 0;
	b[0] = value;
	b[1] = value >> 8;
	IF a < 0 ORF lseek(file, a, 0) == -1L ORF write(file, b, 1) != 1
	THEN	errflg = (space & DSP ? BADDAT : BADTXT);
		return(0);
	FI
	a = nextadr(adr);
	IF a != -1L ANDF chkmap(&a, space, 1, &file) ANDF a >= 0
	THEN	IF lseek(file, a, 0) == -1L ORF write(file, b + 1, 1) != 1
		THEN	errflg = (space & DSP ? BADDAT : BADTXT);
		FI
	FI
	return(w1);
}

/* the byte after adr, in its segment; -1 past the segment's end */
static long
nextadr(adr)
	long	adr;
{
	IF (txtmap.nseg ORF datmap.nseg) ANDF ASEL(adr)
	THEN	IF AOFF(adr) == 0xffff THEN return(-1L); FI
		return(ADDR(ASEL(adr), AOFF(adr) + 1));
	FI
	return(adr + 1);
}

/*
 * A word in the process.  The kernel reads and writes the word at an
 * address whole: one at the last byte of its segment is had as the high
 * byte of the word before it.
 */
static
pword(rd, space, adr, value)
	int	rd, space;
	long	adr;
	int	value;
{
	register int w, mode;

	mode = (space&DSP ? (rd ? PT_READ_D : PT_WRITE_D)
	    : (rd ? PT_READ_I : PT_WRITE_I));
	errno = 0;
	w = ptrace(mode, pid, adr, value);
	IF errno ANDF AOFF(adr) != 0
	THEN	errno = 0;
		w = ptrace(rd ? mode : (space&DSP ? PT_READ_D : PT_READ_I),
		    pid, adr - 1, 0);
		IF errno == 0
		THEN	IF rd
			THEN	w = (w >> 8) & LOBYTE;
			ELSE	w = (w & LOBYTE) | (value << 8);
				ptrace(mode, pid, adr - 1, w);
			FI
		FI
	FI
	IF errno
	THEN	errflg = (space&DSP ? BADDAT : BADTXT);
		return(0);
	FI
	return(w);
}

/*
 * Breakpoints in the process: what is read there is the byte the INT 3
 * took the place of, and what is written there is kept as that byte,
 * the INT 3 staying.
 */
static
bkget(adr, w)
	long	adr;
	register int w;
{
	register BKPTR bk;

	FOR bk = bkpthead; bk; bk = bk->nxtbkpt
	DO	IF !bk->inproc THEN continue; FI
		IF bk->loc == adr
		THEN	w = (w & ~LOBYTE) | bk->ins;
		ELIF bk->loc == nextadr(adr)
		THEN	w = (w & LOBYTE) | (bk->ins << 8);
		FI
	OD
	return(w);
}

static
bkput(adr, w)
	long	adr;
	register int w;
{
	register BKPTR bk;

	FOR bk = bkpthead; bk; bk = bk->nxtbkpt
	DO	IF !bk->inproc THEN continue; FI
		IF bk->loc == adr
		THEN	bk->ins = w & LOBYTE;
			w = (w & ~LOBYTE) | BPT;
		ELIF bk->loc == nextadr(adr)
		THEN	bk->ins = (w >> 8) & LOBYTE;
			w = (w & LOBYTE) | (BPT << 8);
		FI
	OD
	return(w);
}

/*
 * Where in its file the byte at *adr is, which *adr is made, and the
 * file in *file: 1, or 0 with errflg set if it is in neither.  It is -1
 * for a byte of a segment's end that is zero and not in the file.  A
 * core file does not have the program's shared code, which is read
 * from the program.
 */
chkmap(adr,space,wt,file)
	register long	*adr;
	register int	space;
	int	wt, *file;
{
	register MAPPTR amap;
	register struct seg *sp;
	long	off;

	amap=((space&DSP?&datmap:&txtmap));
	IF amap->nseg ORF (space&DSP ANDF !wt ANDF txtmap.nseg ANDF ASEL(*adr))
	THEN	sp = findseg(amap, ASEL(*adr));
		IF sp == 0 ANDF space&DSP ANDF !wt
		THEN	amap = &txtmap;
			sp = findseg(amap, ASEL(*adr));
		FI
		off = AOFF(*adr);
		IF sp == 0 ORF off < sp->b ORF off >= (sp->z > sp->e ? sp->z : sp->e)
		THEN	goto err;
		FI
		*file = amap->ufd;
		IF off >= sp->e
		THEN	IF wt THEN goto err; FI
			*adr = -1L;
		ELSE	*adr = sp->f + off - sp->b;
		FI
		return(1);
	FI

	*file = amap->ufd;
	switch(space&(ISP|DSP|STAR)) {

		case ISP:
		case DSP:
			IF within(*adr, amap->b1, amap->e1)
			THEN *adr += (amap->f1) - (amap->b1);
				break;
			FI
			/* falls through */

		case ISP+STAR:
		case DSP+STAR:
			IF within(*adr, amap->b2, amap->e2)
			THEN *adr += (amap->f2) - (amap->b2);
				break;
			FI
			/* falls through */

		default:
		err:
			errflg = (space&DSP ? BADDAT: BADTXT);
			return(0);
	}
	return(1);
}

static struct seg *
findseg(amap, sel)
	register MAPPTR amap;
	register u_int sel;
{
	register struct seg *sp;

	FOR sp = amap->seg; sp < &amap->seg[amap->nseg]; sp++
	DO	IF SAMESEG(sp->sel, sel) THEN return(sp); FI
	OD
	return(0);
}

static
within(adr,lbd,ubd)
	long	adr, lbd, ubd;
{
	return(adr>=lbd && adr<ubd);
}
