#include "defs.h"

	MSG	NOPCS;
	char	localok;
extern	struct	SYMbol	*symbol;
	char	*errflg;
	u_int	*uar0;
	int	model;
	int	kernel;
	u_int	kframe;
	MAP	txtmap;
	MAP	datmap;

/*
 * The stack, a frame at a time.  A function of the compiler's begins
 * with ENTER, or with PUSH BP and MOV BP,SP (lib/ccom/i286/c11.c), and
 * BP is then its frame: at [bp] its caller's BP, at [bp+2] where it
 * returns to, with the segment of that at [bp+4] where calls are far, as
 * in the medium and large models and the kernel, and its arguments
 * above.  The cross compiler's (tools/cc) may push SI and DI before
 * BP, and then where it returns to is above them; it calls a static
 * function near, in any model, and leaves it out of the symbols.  A
 * function made without a frame, as the cross compiler may make one
 * that needs none, is not seen.
 */
	u_int	frame;		/* the frame, an offset in the stack segment */
	u_int	nextbp;		/* its caller's */
	long	callpc;		/* where in its function, 0 if not known */
	long	retpc;		/* where it returns to */
	u_int	argp;		/* where its arguments are */
	int	narg;		/*   and how many words of them */
	u_int	ssel;		/* the stack segment */

static	int	far;		/* calls are far */
static	int	nearret;	/* but this frame's function was called near */

static u_int
sget(off)
	u_int	off;
{
	return(get(ADDR(ssel, off), DSP));
}

static int
iget(adr)
	long	adr;
{
	return(get(adr, ISP) & LOBYTE);
}

/* the address a frame's function returns to, kept at off */
static long
getret(off)
	u_int	off;
{
	IF far ANDF !nearret
	THEN	return(ADDR(sget(off + 2), sget(off)));
	FI
	return(ADDR(ASEL(callpc) ? ASEL(callpc) : txtmap.def1, sget(off)));
}

/* how many words of arguments the caller takes off the stack after */
static
nargs(ret)
	long	ret;
{
	register int op;

	op = iget(ret);
	IF (op == 0x83 ORF op == 0x81) ANDF iget(ret + 1) == 0xc4
	THEN	return((op == 0x83 ? iget(ret + 2) : get(ret + 2, ISP)) / 2);
	ELIF op >= 0x59 ANDF op <= 0x5b		/* pop cx, dx or bx */
	THEN	op = iget(ret + 1);
		return(op >= 0x59 ANDF op <= 0x5b ? 2 : 1);
	ELIF op == 0x44 ANDF iget(ret + 1) == 0x44	/* inc sp twice */
	THEN	return(1);
	FI
	return(0);
}

/* the function the call before ret called, if it says; else 0 */
static long
calltarget(ret)
	long	ret;
{
	IF far ANDF !nearret ANDF iget(ret - 5) == 0x9a
	THEN	return(ADDR(get(ret - 2, ISP), get(ret - 4, ISP)));
	ELIF iget(ret - 3) == 0xe8
	    ANDF (!far ORF nearret ORF iget(ret - 4) == 0x0e)
	THEN	return(ADDR(ASEL(ret), AOFF(ret) + get(ret - 2, ISP)));
	FI
	return(0L);
}

/*
 * Is what is before ret a call, which returns there: near or far as
 * this frame's function was called?  Through a pointer it is FF with
 * the operation in bits 3 to 5 of the next byte, 2 near and 3 far, and
 * the operand's length in that byte's mode.
 */
static
aftercall(ret)
	long	ret;
{
	register int n, m, len;
	int	near = !far ORF nearret;

	IF calltarget(ret) THEN return(1); FI
	FOR n = 2; n <= 4; n++
	DO	m = iget(ret - n + 1);
		IF iget(ret - n) != 0xff ORF (m >> 3 & 7) != (near ? 2 : 3)
		THEN	continue;
		FI
		len = m >> 6;
		IF len == 0
		THEN	len = (m & 7) == 6 ? 4 : 2;
		ELIF len == 3
		THEN	len = near ? 2 : 0;
		ELSE	len += 2;
		FI
		IF len == n THEN return(1); FI
	OD
	return(0);
}

/*
 * How many words the function that begins at f pushes before its
 * frame: SI and DI, where the cross compiler saves them first.
 */
static
savedat(f)
	long	f;
{
	register int n, op;

	FOR n = 0; n < 2; n++
	DO	op = iget(f + n);
		IF op != 0x56 ANDF op != 0x57 THEN break; FI
	OD
	return(n);
}

/* where the function that pc is in begins, by its symbol; 0 if none */
static long
funcstart(pc)
	long	pc;
{
	register int n;

	n = findsym(pc, ISYM);
	IF symbol == NULL THEN return(0L); FI
	return(fulladr(pc, ISP) - (u_int)n);
}

/* savedat for the function that pc is in */
static
saved(pc)
	long	pc;
{
	long	f;

	f = funcstart(pc);
	return(f ? savedat(f) : 0);
}

/*
 * The rest of a frame, from where it keeps its caller's BP, its function
 * having pushed k words before it, and been called near if near.
 */
static
fillframe(f, k, near)
	u_int	f;
	int	k, near;
{
	frame = f;
	nextbp = sget(f);
	nearret = far ANDF near;
	retpc = getret(f + 2 + 2 * k);
	argp = f + 2 * k + (far ANDF !near ? 6 : 4);
}

/*
 * Does the function that begins at f push n words, and then BP for its
 * frame?
 */
static
hasframe(f, n)
	long	f;
	int	n;
{
	register int op;

	IF savedat(f) != n THEN return(0); FI
	op = iget(f + n);
	return(op == 0x55 ORF op == 0xc8);	/* push bp, enter */
}

/*
 * The frame at f: its function having pushed k words before it, as its
 * code says, if that has it return to just after a call.  Else as many
 * as the function pushes that the call before where the frame returns
 * to is to, the frame being that function's, called far or near: the
 * function the frame within it returned to made no frame of its own,
 * or has no symbol, as the cross compiler's static functions have none
 * and are called near.  The frame's function is then the one the call
 * is to (callpc is 0); so too when the call names a function that is
 * not the one returned to, the frame within having been an exception's
 * and not a call's.  k is -1 when no code says.  Returns 0 if neither
 * finds a frame.
 */
static
pickframe(f, k)
	u_int	f;
	int	k;
{
	register int n, near;
	long	pc, start;

	IF k >= 0
	THEN	errflg = 0;
		fillframe(f, k, 0);
		IF errflg == 0 ANDF segtype(ASEL(retpc)) == ISYM
		    ANDF aftercall(retpc) ANDF errflg == 0
		THEN	pc = calltarget(retpc);
			start = funcstart(callpc);
			IF pc ANDF (AOFF(pc) != AOFF(start)
			    ORF !SAMESEG(ASEL(pc), ASEL(start)))
			THEN	callpc = 0;
			FI
			errflg = 0;
			return(1);
		FI
	FI
	FOR near = 0; near <= far; near++
	DO	FOR n = 0; n <= 2; n++
		DO	errflg = 0;
			fillframe(f, n, near);
			IF errflg == 0 ANDF segtype(ASEL(retpc)) == ISYM
			    ANDF (pc = calltarget(retpc)) ANDF hasframe(pc, n)
			    ANDF errflg == 0
			THEN	callpc = 0;
				return(1);
			FI
		OD
	OD
	errflg = 0;
	return(0);
}

/* the function of the frame, in symbol, from what is known of it */
static
framesym()
{
	long	pc;

	localok = FALSE;
	pc = callpc ? callpc : calltarget(retpc);
	IF pc == 0
	THEN	symbol = NULL;
	ELSE	findsym(pc, ISYM);
	FI
	narg = nargs(retpc);
}

/*
 * The innermost frame.  With an address it is the frame there, whose
 * function is the one its caller's call names; in a crash dump of the
 * kernel without one it is the frame of the routine that took the dump
 * (kframe).  Else it is the registers': a function at its first
 * instruction has not pushed BP, and one at MOV BP,SP has not made BP
 * its frame, and its frame is at SP.
 */
firstframe(adr, given)
	long	adr;
	int	given;
{
	register u_int sp;
	register int w;

	far = model != A_SMALL;
	nearret = 0;
	ssel = datmap.def2;
	IF !given ANDF kernel ANDF kframe
	THEN	adr = kframe;
		given = TRUE;
	FI
	IF given
	THEN	IF ASEL(adr) THEN ssel = ASEL(adr); FI
		callpc = 0;
		IF !pickframe(AOFF(adr), -1) THEN fillframe(AOFF(adr), 0, 0); FI
		framesym();
		return;
	FI
	IF kernel THEN error(NOPCS); FI
	callpc = pcaddr();
	sp = uar0[RSP];
	w = findsym(callpc, ISYM);
	IF symbol ANDF w == 0
	THEN	frame = sp - 2;
		nextbp = uar0[BP];
		retpc = getret(sp);
		argp = sp + (far ? 4 : 2);
	ELIF (w = get(callpc, ISP)) == 0xe589 ORF w == 0xec8b
	THEN	fillframe(sp, saved(callpc), 0);
	ELSE	fillframe(uar0[BP], saved(callpc), 0);
	FI
	framesym();
}

/*
 * The caller's frame: 0 at the end of the stack, or where the frames do
 * not go up the stack as they must.  Its function is the one where the
 * return goes, by the byte before, which is the call's.
 */
nextframe()
{
	IF nextbp == 0 ORF nextbp <= frame ORF errflg
	THEN	return(0);
	FI
	callpc = retpc - 1;
	/*
	 * Not one at the start-up's frame, whose return address is no
	 * code's: the large model's crt0 keeps the arguments it passes
	 * main at BP.
	 */
	IF !pickframe(nextbp, saved(callpc)) THEN return(0); FI
	framesym();
	IF errflg THEN return(0); FI
	return(1);
}
