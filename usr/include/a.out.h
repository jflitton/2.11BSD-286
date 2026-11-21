/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)a.out.h	5.6.1 (2.11BSD GTE) 1/6/94
 */

#ifndef	_AOUT_H_
#define	_AOUT_H_

#include <sys/exec.h>

/*
 * The 286's format (sys/exec.h).  The PDP-11's magic numbers and its
 * overlays are not in it.  The contents of the segments start after the
 * table of them, and the symbol table follows the last; each segment is
 * made up to an even length in the file.
 */
#define	N_BADMAG(x)	((x).a_magic != A_MAGIC && (x).a_magic != A_OBJECT)

/*
 * An object file's relocation: after the segments' contents, the
 * entries for the first segment (xs_nrel of them, sys/exec.h), then
 * those for the second, each in the order of r_addr.  An entry is for
 * a word of the segment, which is to have something done to it that
 * depends on where a symbol, or one of the file's own segments,
 * is put when the program is linked.
 *
 * The PDP-11 has a word of relocation for each word of the text and
 * the data.  An instruction of the 286's has its addresses at any
 * byte, and few of its words are addresses.
 */
struct reloc {
	unsigned short	r_addr;	/* where the word is in its segment */
	unsigned short	r_sym;	/* with R_EXT a symbol, by its place in
				   the symbol table from 0; else which of
				   this file's segments, from 1 */
	unsigned short	r_type;
};

#define	R_OFF	1	/* add where that is in its segment: an address */
#define	R_SEG	2	/* the selector of the segment it is in */
#define	R_PCREL	3	/* add how far it is from where this file's part
			   of the word's segment begins: the word has
			   what is to be added, less where the word ends */
#define	R_TYPE	0x0f
#define	R_EXT	0x10	/* r_sym is a symbol */

/*
 * An object has three segments, the text, the data and the bss, and
 * may have more after them: named segments of data that is not in the
 * program's data segment, as a far variable is, or the kernel's
 * network's (NET_GROUP).  Each is named by a symbol of type N_SEGNAME
 * whose n_ovly is its number, and if it is in a group, the group by
 * one of type N_GRPNAME with the same n_ovly; those come first in the
 * symbol table.  What every file has of a named segment is put
 * together, in the order they are linked.  The segments of one group
 * are one segment of the program, and the others share segments as
 * they fit.  A symbol in a named segment has its type N_DATA and its
 * number in n_ovly, which is 0 for the three.
 */

#define	N_TXTOFF(x) \
	((off_t)sizeof(struct exec) + (x).a_nseg * (off_t)sizeof(struct exseg))

/*
 * These take the header with the table of segments, a struct xexec, and
 * call functions because the sizes of the segments have to be summed.
 */

#include <sys/types.h>

off_t	n_stroff(), n_symoff();

#define	N_STROFF(e) (n_stroff(&e))
#define	N_SYMOFF(e) (n_symoff(&e))

#define	_AOUT_INCLUDE_
#include <nlist.h>

#endif	/* !_AOUT_H_ */
