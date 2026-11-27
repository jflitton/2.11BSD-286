/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)range.c	5.2.1 (2.11BSD) 2026/11/26
 *
 * routines to return extreme values
 * VERY MACHINE DEPENDENT
 */

union fi
{	float	f;
	long	i;
} ;

union dj
{	double	d;
	long	j[2];
} ;

float
flmax_()
{
	union fi n;
#if	pdp11
	n.i = 0x7fffffffL;
#else	pdp11
#ifdef	i286
	n.i = 0x7f7fffffL;
#else	i286
#if	vax
	n.i = 0xffff7fff;
#else	vax
	UNKNOWN MACHINE!
#endif	vax
#endif	i286
#endif	pdp11
	return(n.f);
}

double
dflmax_()
{
	union dj n;
#if	pdp11
	n.j[0] = 0x7fffffffL;
	n.j[1] = 0xffffffffL;
#else	pdp11
#ifdef	i286
	n.j[0] = 0xffffffffL;
	n.j[1] = 0x7fefffffL;
#else	i286
#if	vax
	n.j[0] = 0xffff7fff;
	n.j[1] = 0xffffffff;
#else	vax
	UNKNOWN MACHINE!
#endif	vax
#endif	i286
#endif	pdp11
	return(n.d);
}

float
flmin_()
{
	union fi n;
#if	pdp11
	n.i = 0x00800000L;
#else	pdp11
#ifdef	i286
	n.i = 0x00800000L;
#else	i286
#if	vax
	n.i = 0x00000080;
#else	vax
	UNKNOWN MACHINE!
#endif	vax
#endif	i286
#endif	pdp11
	return(n.f);
}

double
dflmin_()
{
	union dj n;
#if	pdp11
	n.j[0] = 0x00800000L;
	n.j[1] = 0;
#else	pdp11
#ifdef	i286
	n.j[0] = 0;
	n.j[1] = 0x00100000L;
#else	i286
#if	vax
	n.j[0] = 0x00000080;
	n.j[1] = 0;
#else	vax
	UNKNOWN MACHINE!
#endif	vax
#endif	i286
#endif	pdp11
	return(n.d);
}

long int
inmax_()
{
	return(0x7fffffffL);
}


float
ffrac_()
{
	union fi n;
#if	pdp11
	n.i = 0x35000000L;
#else	pdp11
#ifdef	i286
	n.i = 0x34000000L;
#else	i286
#if	vax
	n.i = 0x00003500;
#else	vax
	UNKNOWN MACHINE!
#endif	vax
#endif	i286
#endif	pdp11
	return(n.f);
}

double
dffrac_()
{
	union dj n;
#if	pdp11
	n.j[0] = 0x25000000L;
	n.j[1] = 0;
#else	pdp11
#ifdef	i286
	n.j[0] = 0;
	n.j[1] = 0x3cb00000L;
#else	i286
#if	vax
	n.j[0] = 0x00002500;
	n.j[1] = 0;
#else	vax
	UNKNOWN MACHINE!
#endif	vax
#endif	i286
#endif	pdp11
	return(n.d);
}
