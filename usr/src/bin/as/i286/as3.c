/*
 * The assembler for the 80286: the object file (a.out.h).
 *
 * The second pass writes the text and the data each where it goes in
 * the file, and the text's relocation after them; the data's is kept
 * in a file of its own until how much the text has is known.
 */
#include "as.h"
#include <a.out.h>

int	sect;			/* the segment being assembled */
unsigned loc[NSECT];		/* and where in each */

static	FILE	*sfp[NSECT];	/* the contents */
static	FILE	*rfp[NSECT];	/* the relocation */
static	unsigned nrel[NSECT];
static	unsigned size[NSECT];	/* as the first pass found them */
static	char	tmpname[] = "/tmp/asXXXXXX";
static	unsigned nsym;

#define	even(n)	(((long)(n) + 1) & ~1L)

/*
 * A byte of the segment.
 */
ob(b)
{
	if (pass == 2) {
		if (sect != SBSS)
			putc(b, sfp[sect]);
		else if (b)
			error("the bss cannot have anything in it", "");
	}
	if (loc[sect]++ == 0177777)
		fatal("more than 64K in a segment", "");
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
	if (sect == SBSS)
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

	if (sect == SBSS)
		return;
	r.r_addr = loc[sect];
	if (e->e_sect == SUNDEF) {
		if (e->e_sym == NULL || e->e_sym->s_index < 0)
			return;
		r.r_sym = e->e_sym->s_index;
		type |= R_EXT;
	} else
		r.r_sym = e->e_sect;
	r.r_type = type;
	fwrite((char *)&r, sizeof r, 1, rfp[sect]);
	nrel[sect]++;
}

/*
 * A value in a word: a number, an address, or the selector of an
 * address's segment.
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
	ow((int)e->e_val);
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
 * many times as there are parts of it being written at once.
 */
outstart()
{
	register struct sym *sp;
	register i;
	long off;

	for (i = 0; i < NSECT; i++)
		size[i] = loc[i];
	nsym = 0;
	for (i = 0; i < NHASH; i++)
		for (sp = hashtab[i]; sp; sp = sp->s_next)
			if (wanted(sp))
				sp->s_index = nsym++;
	if ((sfp[STEXT] = fopen(outfile, "w")) == NULL ||
	    (sfp[SDATA] = fopen(outfile, "r+")) == NULL ||
	    (rfp[STEXT] = fopen(outfile, "r+")) == NULL)
		fatal("cannot create %s", outfile);
	mktemp(tmpname);
	if ((rfp[SDATA] = fopen(tmpname, "w+")) == NULL)
		fatal("cannot create %s", tmpname);
	unlink(tmpname);
	off = sizeof(struct exec) + 3 * sizeof(struct exseg);
	fseek(sfp[STEXT], off, 0);
	off += even(size[STEXT]);
	fseek(sfp[SDATA], off, 0);
	off += even(size[SDATA]);
	fseek(rfp[STEXT], off, 0);
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
 * After it: the rest of the object, and last its header.
 */
outfinish()
{
	register struct sym *sp;
	register FILE *f;
	register i;
	struct nlist n;
	struct exec hdr;
	struct exseg seg[3];
	long strx;
	int c;

	if (nerr)
		return;
	for (i = STEXT; i <= SBSS; i++)
		if (loc[i] != size[i])
			fatal("the passes do not agree (phase error)", "");
	for (i = STEXT; i <= SDATA; i++) {
		if (size[i] & 1)
			putc(0, sfp[i]);
		if (ferror(sfp[i]))
			fatal("error writing %s", outfile);
		fclose(sfp[i]);
	}
	f = rfp[STEXT];
	rewind(rfp[SDATA]);
	while ((c = getc(rfp[SDATA])) != EOF)
		putc(c, f);
	fclose(rfp[SDATA]);
	strx = 4;
	for (i = 0; i < NHASH; i++)
		for (sp = hashtab[i]; sp; sp = sp->s_next) {
			if (sp->s_index < 0)
				continue;
			n.n_un.n_strx = strx;
			strx += strlen(sp->s_name) + 1;
			/* (its type says which segment: nm takes n_ovly
			   for an overlay, as a program has them) */
			n.n_ovly = 0;
			n.n_value = sp->s_value;
			if (sp->s_stmt == 0) {
				n.n_type = N_UNDF | N_EXT;
				if ((sp->s_flags & FCOMMON) == 0)
					n.n_value = 0;
			} else {
				if (sp->s_sect == SABS)
					n.n_type = N_ABS;
				else
					n.n_type = N_TEXT + sp->s_sect - STEXT;
				if (sp->s_flags & FGLOBAL)
					n.n_type |= N_EXT;
			}
			fwrite((char *)&n, sizeof n, 1, f);
		}
	put4(strx, f);
	for (i = 0; i < NHASH; i++)
		for (sp = hashtab[i]; sp; sp = sp->s_next)
			if (sp->s_index >= 0) {
				fputs(sp->s_name, f);
				putc(0, f);
			}
	fseek(f, 0L, 0);
	hdr.a_magic = A_OBJECT;
	hdr.a_model = model;
	hdr.a_nseg = 3;
	hdr.a_eseg = 0;
	hdr.a_entry = 0;
	hdr.a_dseg = 0;
	hdr.a_stack = 0;
	hdr.a_unused = 0;
	hdr.a_syms = (long)nsym * sizeof(struct nlist);
	hdr.a_strs = strx;
	seg[0].xs_type = XS_CODE;
	seg[1].xs_type = seg[2].xs_type = XS_DATA;
	for (i = 0; i < 3; i++) {
		seg[i].xs_nrel = nrel[STEXT + i];
		seg[i].xs_filesz = i == 2 ? 0 : size[STEXT + i];
		seg[i].xs_memsz = size[STEXT + i];
	}
	fwrite((char *)&hdr, sizeof hdr, 1, f);
	fwrite((char *)seg, sizeof seg[0], 3, f);
	if (ferror(f))
		fatal("error writing %s", outfile);
	fclose(f);
}
