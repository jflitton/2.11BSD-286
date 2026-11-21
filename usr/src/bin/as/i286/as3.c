/*
 * The assembler for the 80286: the object file (a.out.h).
 *
 * The second pass writes the text and the data each where it goes in
 * the file, and the text's relocation after them; the relocation of
 * the data, and of the named segments, is kept in a file of its own
 * until how much the text has is known.  What a named segment has in
 * it, which is not much, is written where it goes as it comes.
 *
 * An image (-b) is the text and nothing else, its addresses those of
 * where it is to be (org).
 */
#include "as.h"
#include <a.out.h>

int	sect;			/* the segment being assembled */
unsigned loc[NSECT];		/* and where in each */

static	FILE	*sfp[NSECT];	/* the contents of the text and the data */
static	FILE	*nfp;		/*	and of the named segments */
static	int	nfpsect;	/* which one nfp is at */
static	FILE	*rfp;		/* the text's relocation */
static	FILE	*xfp;		/* everyone else's */
static	unsigned nrel[NSECT];
static	unsigned size[NSECT];	/* as the first pass found them */
static	unsigned fsize[NSECT];	/* a named one's that has contents */
static	unsigned wpos[NSECT];	/* and how much of that is written */
static	long	noff[NSECT];	/* where in the file that goes */
static	char	tmpname[] = "/tmp/asXXXXXX";
static	unsigned nsym;

#define	even(n)	(((long)(n) + 1) & ~1L)

/*
 * A byte of the segment.  A named segment has in the file only as much
 * as goes up to its last byte that is put there; room made for what is
 * zero before that is written as zeros when that byte comes.
 */
ob(b)
{
	register s;

	s = sect;
	if (pass == 2) {
		if (s >= SNAMED) {
			if (nfpsect != s) {
				fseek(nfp, noff[s] + wpos[s], 0);
				nfpsect = s;
			}
			for (; wpos[s] < loc[s]; wpos[s]++)
				putc(0, nfp);
			putc(b, nfp);
			wpos[s]++;
		} else if (s != SBSS)
			putc(b, sfp[s]);
		else if (b)
			error("the bss cannot have anything in it", "");
	}
	if (loc[s]++ == 0177777)
		fatal("more than 64K in a segment", "");
	if (s >= SNAMED && loc[s] > fsize[s])
		fsize[s] = loc[s];
}

ow(w)
{
	ob(w);
	ob(w >> 8);
}

/*
 * Room for n bytes, which are zero.
 */
oskip(n)
long n;
{
	if (n + loc[sect] > 0177777L)
		fatal("more than 64K in a segment", "");
	if (sect == SBSS || sect >= SNAMED)
		loc[sect] += (unsigned)n;
	else
		while (--n >= 0)
			ob(0);
}

/*
 * Note that the word about to be written is to have something done to
 * it when the program is linked.
 */
static
reloc(type, e)
register struct expr *e;
{
	struct reloc r;
	short s;

	if (sect == SBSS)
		return;
	if (flat) {
		/* an image is not linked: its own text is all it has */
		if (type != R_OFF || e->e_sect != STEXT)
			error("an image has no segments and no other files", "");
		return;
	}
	r.r_addr = loc[sect];
	if (e->e_sect == SUNDEF) {
		if (e->e_sym == NULL || e->e_sym->s_index < 0)
			return;
		r.r_sym = e->e_sym->s_index;
		type |= R_EXT;
	} else
		r.r_sym = e->e_sect;
	r.r_type = type;
	if (sect == STEXT)
		fwrite((char *)&r, sizeof r, 1, rfp);
	else {
		s = sect;
		fwrite((char *)&s, sizeof s, 1, xfp);
		fwrite((char *)&r, sizeof r, 1, xfp);
	}
	nrel[sect]++;
}

/*
 * A value in a word: a number, an address, or the selector of an
 * address's segment.  In an image an address is where it is put.
 */
oword(e)
register struct expr *e;
{
	if (e->e_seg) {
		if (pass == 2)
			reloc(R_SEG, e);
		ow(0);
		return;
	}
	if (pass == 2 && e->e_sect != SABS)
		reloc(R_OFF, e);
	ow((int)e->e_val + (flat && e->e_sect == STEXT ? origin : 0));
}

obyte(e)
register struct expr *e;
{
	if (pass == 2 && !isnum(e))
		error("an address will not go in a byte", "");
	ob((int)e->e_val);
}

/*
 * A word that is how far it is to somewhere from the end of the word.
 */
orel(e)
register struct expr *e;
{
	if (pass == 2) {
		if (e->e_seg || e->e_sect == SABS)
			error("a jump or a call to what is no address", "");
		else if (e->e_sect != sect)
			reloc(R_PCREL, e);
	}
	ow((int)e->e_val - (int)(loc[sect] + 2));
}

/*
 * The same in a byte, for which it must be near and in this segment.
 */
oshort(e)
register struct expr *e;
{
	long d;

	d = (e->e_val & 0177777L) - ((long)loc[sect] + 1);
	if (pass == 2 && (e->e_seg || e->e_sect != sect || d < -128 ||
	    d > 127))
		error("a short jump does not reach", "");
	ob((int)d);
}

/*
 * Is the symbol one for the object's table?  Those are the ones other
 * files know, and this file's own labels that are not the compiler's.
 */
static
wanted(sp)
register struct sym *sp;
{
	if (sp->s_flags & FCOMMON)
		return(1);
	if (sp->s_stmt == 0)
		return(sp->s_flags & FUSED);
	if (sp->s_flags & FGLOBAL)
		return(1);
	return(sp->s_sect != SABS && (sp->s_flags & FLOCAL) == 0);
}

/*
 * Before the second pass: number the symbols, and open the object as
 * many times as there are parts of it being written at once.  The
 * names of the named segments, and of their groups, are the first
 * symbols.
 */
outstart()
{
	register struct sym *sp;
	register i;
	long off;

	for (i = 0; i < nsect; i++)
		size[i] = loc[i];
	if (flat) {
		if (origin + (long)size[STEXT] > 0200000L)
			fatal("an image cannot go past 64K", "");
		if ((sfp[STEXT] = fopen(outfile, "w")) == NULL)
			fatal("cannot create %s", outfile);
		return;
	}
	nsym = 0;
	for (i = SNAMED; i < nsect; i++)
		nsym += sectgrp[i] ? 2 : 1;
	for (i = 0; i < NHASH; i++)
		for (sp = hashtab[i]; sp; sp = sp->s_next)
			if (wanted(sp))
				sp->s_index = nsym++;
	if ((sfp[STEXT] = fopen(outfile, "w")) == NULL ||
	    (sfp[SDATA] = fopen(outfile, "r+")) == NULL ||
	    (nfp = fopen(outfile, "r+")) == NULL ||
	    (rfp = fopen(outfile, "r+")) == NULL)
		fatal("cannot create %s", outfile);
	mktemp(tmpname);
	if ((xfp = fopen(tmpname, "w+")) == NULL)
		fatal("cannot create %s", tmpname);
	unlink(tmpname);
	off = sizeof(struct exec) + (nsect - 1) * sizeof(struct exseg);
	fseek(sfp[STEXT], off, 0);
	off += even(size[STEXT]);
	fseek(sfp[SDATA], off, 0);
	off += even(size[SDATA]);
	nfpsect = 0;
	for (i = SNAMED; i < nsect; i++) {
		noff[i] = off;
		off += even(fsize[i]);
		wpos[i] = 0;
	}
	fseek(rfp, off, 0);
}

static
put4(n, f)
long n;
register FILE *f;
{
	putc((int)n, f);
	putc((int)(n >> 8), f);
	putc((int)(n >> 16), f);
	putc((int)(n >> 24), f);
}

/*
 * A symbol for the table.
 */
static
putsym(f, strx, type, ovly, value)
FILE *f;
long strx;
unsigned value;
{
	struct nlist n;

	n.n_un.n_strx = strx;
	n.n_type = type;
	n.n_ovly = ovly;
	n.n_value = value;
	fwrite((char *)&n, sizeof n, 1, f);
}

/*
 * After it: the rest of the object, and last its header.
 */
outfinish()
{
	register struct sym *sp;
	register FILE *f;
	register i;
	struct exec hdr;
	struct exseg seg;
	struct reloc r;
	long strx;
	int c, t;
	short s;

	if (nerr)
		return;
	for (i = STEXT; i < nsect; i++)
		if (loc[i] != size[i])
			fatal("the passes do not agree (phase error)", "");
	if (flat) {
		/* the bytes, and nothing about them */
		if (ferror(sfp[STEXT]))
			fatal("error writing %s", outfile);
		fclose(sfp[STEXT]);
		return;
	}
	for (i = STEXT; i <= SDATA; i++) {
		if (size[i] & 1)
			putc(0, sfp[i]);
		if (ferror(sfp[i]))
			fatal("error writing %s", outfile);
		fclose(sfp[i]);
	}
	for (i = SNAMED; i < nsect; i++)
		if (fsize[i] & 1) {
			fseek(nfp, noff[i] + fsize[i], 0);
			putc(0, nfp);
		}
	if (ferror(nfp))
		fatal("error writing %s", outfile);
	fclose(nfp);
	f = rfp;
	for (t = SDATA; t < nsect; t++) {
		rewind(xfp);
		while (fread((char *)&s, sizeof s, 1, xfp) == 1 &&
		    fread((char *)&r, sizeof r, 1, xfp) == 1)
			if (s == t)
				fwrite((char *)&r, sizeof r, 1, f);
	}
	fclose(xfp);
	strx = 4;
	for (i = SNAMED; i < nsect; i++) {
		putsym(f, strx, N_SEGNAME, i, 0);
		strx += strlen(sectname[i]) + 1;
		if (sectgrp[i]) {
			putsym(f, strx, N_GRPNAME, i, 0);
			strx += strlen(sectgrp[i]) + 1;
		}
	}
	for (i = 0; i < NHASH; i++)
		for (sp = hashtab[i]; sp; sp = sp->s_next) {
			if (sp->s_index < 0)
				continue;
			/* its type says which of the three segments it
			   is in, and n_ovly which named one */
			c = 0;
			if (sp->s_stmt == 0) {
				t = N_UNDF | N_EXT;
				if ((sp->s_flags & FCOMMON) == 0)
					sp->s_value = 0;
			} else {
				if (sp->s_sect == SABS)
					t = N_ABS;
				else if (sp->s_sect >= SNAMED) {
					t = N_DATA;
					c = sp->s_sect;
				} else
					t = N_TEXT + sp->s_sect - STEXT;
				if (sp->s_flags & FGLOBAL)
					t |= N_EXT;
			}
			putsym(f, strx, t, c, sp->s_value);
			strx += strlen(symname(sp)) + 1;
		}
	put4(strx, f);
	for (i = SNAMED; i < nsect; i++) {
		fputs(sectname[i], f);
		putc(0, f);
		if (sectgrp[i]) {
			fputs(sectgrp[i], f);
			putc(0, f);
		}
	}
	for (i = 0; i < NHASH; i++)
		for (sp = hashtab[i]; sp; sp = sp->s_next)
			if (sp->s_index >= 0) {
				fputs(symname(sp), f);
				putc(0, f);
			}
	fseek(f, 0L, 0);
	hdr.a_magic = A_OBJECT;
	hdr.a_model = model;
	hdr.a_nseg = nsect - 1;
	hdr.a_eseg = 0;
	hdr.a_entry = 0;
	hdr.a_dseg = 0;
	hdr.a_stack = 0;
	hdr.a_unused = 0;
	hdr.a_syms = (long)nsym * sizeof(struct nlist);
	hdr.a_strs = strx;
	fwrite((char *)&hdr, sizeof hdr, 1, f);
	for (i = STEXT; i < nsect; i++) {
		seg.xs_type = i == STEXT ? XS_CODE : XS_DATA;
		seg.xs_nrel = nrel[i];
		seg.xs_filesz = i == SBSS ? 0 : i >= SNAMED ? fsize[i] :
		    size[i];
		seg.xs_memsz = size[i];
		fwrite((char *)&seg, sizeof seg, 1, f);
	}
	if (ferror(f))
		fatal("error writing %s", outfile);
	fclose(f);
}
