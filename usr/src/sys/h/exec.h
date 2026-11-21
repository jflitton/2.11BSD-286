/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)exec.h	1.2 (2.11BSD GTE) 10/31/93
 */

#ifndef _EXEC_
#define _EXEC_
/*
 * Header prepended to each a.out file.  All of the file is: this header,
 * a struct exseg for each of the a_nseg segments, the contents of each
 * segment in turn, each made up to an even length, the symbol table, and
 * the string table.
 *
 * Segments are numbered from 1, and a running program addresses segment
 * N with a selector that is fixed when the file is made (LSEL(N),
 * machine/seg.h).
 *
 * An object file, which the assembler writes and the linker reads, is
 * the same with A_OBJECT for its magic number and three segments: the
 * text, the data, and the bss, of which nothing is in the file.  It has
 * relocation between the segments' contents and the symbol table
 * (a.out.h).
 */
struct	exec {
unsigned short	a_magic;	/* magic number */
unsigned short	a_model;	/* memory model */
unsigned short	a_nseg;		/* number of segments */
unsigned short	a_eseg;		/* entry point: segment, */
unsigned short	a_entry;	/*	and offset */
unsigned short	a_dseg;		/* the segment DS and SS start as */
unsigned short	a_stack;	/* size of the stack, which follows that
				   segment's data */
unsigned short	a_unused;	/* not used */
unsigned long	a_syms;		/* size of symbol table */
unsigned long	a_strs;		/* size of string table */
};

struct	exseg {
unsigned short	xs_type;	/* XS_CODE or XS_DATA */
unsigned short	xs_nrel;	/* in an object, how many entries of
				   relocation it has; else 0 */
unsigned long	xs_filesz;	/* bytes of it in the file */
unsigned long	xs_memsz;	/* bytes in memory: the rest are zero */
};

/*
 * The header with the table of segments after it, where the PDP-11 has
 * the sizes of its overlays (struct ovlhdr): what a program reads to
 * find the symbol table, which follows the segments (a.out.h).  A file
 * has fewer segments than a process has slots (NSEG, machine/seg.h).
 */
#define	NXSEG		31
struct	xexec {
	struct	exec	e;
	struct	exseg	x[NXSEG];
};

#define	A_MAGIC		0x0286	/* a program */
#define	A_OBJECT	0x0186	/* an object file */

#define	A_SMALL		1	/* one code segment, near data */
#define	A_MEDIUM	2	/* many code segments, near data */
#define	A_LARGE		3	/* many of both: pointers are far */

#define	XS_CODE		1
#define	XS_DATA		2

#endif
