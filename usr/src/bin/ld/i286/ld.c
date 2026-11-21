/*
 * ld - the link editor for the 80286.
 *
 * It makes a program (sys/exec.h) of object files and of what they
 * need from libraries (a.out.h, ar.h, ranlib.h).  The way of it is
 * ../ld.c's, the PDP-11's: a first pass over the arguments that reads
 * each file's symbols and finds how big everything is, and which
 * members of the libraries are wanted; the middle, where what was left
 * common is given room and every symbol has its place; and a second
 * pass that copies the text and the data, relocating them.  What is
 * the PDP-11's alone is not here: its overlays, its text and data in
 * one address space or two, a word of relocation for each word.
 *
 * A program here is segments.  Its data, with the bss and the common
 * storage after it, is one, the last.  Its text is one segment in the
 * small model.  In the medium model, where every function is called
 * far, it is as many as it takes: each file's text is whole in one of
 * them, and files share a segment, in the order they are linked, until
 * the next would not fit.  A segment's selector is fixed by its number
 * (LSEL, machine/seg.h), so a word that is to hold one is filled in
 * here and the program is loaded as it is.
 *
 *	ld [-o out] [-ms] [-mm] [-s] [-x] [-stack n] [-packcode n]
 *	    [-e sym] [-u sym] [-L dir] [-M] [-t] file ... -lx ...
 */
#include <stdio.h>
#include <a.out.h>
#include <ar.h>
#include <ranlib.h>

#define	NHASH	509
#define	NAMESZ	128		/* the longest name of a symbol */
#define	NCODE	30		/* segments of text: a program has 31 */
#define	NDIRS	16

#define	N_COMM	05		/* common: for here only (a.out.h) */

#define	SELECTOR(n)	((n) << 3 | 7)	/* of segment n: LSEL */
#define	even(n)		(((n) + 1) & ~1L)

struct sym {
	struct	sym *s_next;	/* the next with the same hash */
	struct	sym *s_link;	/* the next to have been entered */
	char	*s_name;
	unsigned s_value;	/* N_COMM: its size */
	char	s_type;
	char	s_seg;		/* N_TEXT: which segment */
};

struct	sym *hashtab[NHASH];
struct	sym *sym0, *symn;
unsigned nglobal;		/* how many are for the symbol table */
long	globlen;		/*	and how long their names are */
unsigned nlocal;		/* the same of the files' own symbols */
long	loclen;

/*
 * The options.
 */
int	model = A_SMALL;
int	sflag;			/* no symbol table */
int	xflag;			/* none of the files' own symbols */
int	Mflag;			/* say where everything is */
int	trace;
unsigned stack = 8192;
long	limit = 65536L;		/* the most text in a segment */
char	*entry = "..start";
char	*ofilename = "a.out";
char	*dirs[NDIRS];
int	ndir;

/*
 * Where the next file's text, data and bss will go, and where the
 * one in hand has gone.  Both passes go through the files in the same
 * order, and so find the same.
 */
int	curseg;			/* the segment of text being filled */
long	tpos, dpos, bpos;
long	seglen[NCODE+1];
int	tseg;
unsigned tbase, dbase, bbase;

int	ncode;			/* segments of text */
int	dseg;			/* and the data's, which is the next */
long	dsize;			/* the data that has contents */
long	bssorg;			/* where the bss starts in the segment */
long	comorg;			/* and the common storage */
long	memsize;		/* all of it */

/*
 * The members that are wanted of each library, by where each is in
 * the file: in the order they were found to be, with a -1 after the
 * last of a library.
 */
long	*liblist;
int	nlib, maxlib;
int	libnext;

/*
 * The file being read, three times over: the header and the contents,
 * the relocation or the symbols, and the names.
 */
char	*filname;
FILE	*fa, *fb, *fc;
struct	exec filhdr;
struct	exseg filseg[3];
long	symoff, stroff;		/* its symbol table, and its names */
unsigned nsyms;

struct	sym **symmap;		/* the second pass: its symbols, by number */
unsigned mapsize;

FILE	*tout, *dout, *sout, *strout;
long	strx;			/* how long the string table is so far */

int	errlev;
int	pass;

struct	sym *lookup(), *enter();
char	*alloc(), *save();
long	get4(), atol(), member();
extern	char *malloc(), *realloc();

main(argc, argv)
char **argv;
{
	if (argc < 2) {
		fprintf(stderr, "usage: ld [-o out] [-ms] [-mm] [-s] [-x] \
[-stack n] [-packcode n] [-e sym] [-u sym] [-L dir] [-M] file ... -lx ...\n");
		exit(1);
	}
	args(argc, argv);
	pass = 1;
	curseg = 1;
	args(argc, argv);
	middle();
	setupout();
	pass = 2;
	curseg = 1;
	tpos = dpos = bpos = 0;
	libnext = 0;
	args(argc, argv);
	finishout();
	exit(errlev != 0);
}

/*
 * The arguments, in order.  The flags are all taken first, wherever
 * they are, and then each pass takes the files.
 */
args(argc, argv)
char **argv;
{
	register char *ap;
	register i;

	for (i = 1; i < argc; i++) {
		ap = argv[i];
		if (*ap != '-') {
			if (pass)
				loadarg(ap);
			continue;
		}
		ap++;
		if (strcmp(ap, "stack") == 0 || strcmp(ap, "packcode") == 0) {
			if (++i >= argc)
				error(1, "-%s: no number", ap);
			if (*ap == 's')
				stack = (unsigned)atol(argv[i]);
			else
				limit = atol(argv[i]);
			if (limit < 2 || limit > 65536L)
				error(1, "-packcode: not a segment's size", "");
			continue;
		}
		switch (*ap) {

		case 'o':
			if (++i >= argc)
				error(1, "-o: no name", "");
			ofilename = argv[i];
			break;

		case 'e':
		case 'u':
			if (++i >= argc)
				error(1, "-%s: no symbol", ap);
			if (*ap == 'e')
				entry = argv[i];
			if (pass == 0 && lookup(argv[i]) == NULL)
				enter(argv[i]);
			break;

		case 'L':
			if (ap[1] == 0) {
				if (++i >= argc)
					error(1, "-L: no directory", "");
				ap = argv[i];
			} else
				ap++;
			if (pass == 0) {
				if (ndir >= NDIRS)
					error(1, "too many -L", "");
				dirs[ndir++] = ap;
			}
			break;

		case 'l':
			if (pass)
				library(ap + 1);
			break;

		case 'm':
			if (ap[1] == 's')
				model = A_SMALL;
			else if (ap[1] == 'm')
				model = A_MEDIUM;
			else
				error(1, "-%s: no such model", ap);
			break;

		case 's':
			sflag = xflag = 1;
			break;

		case 'x':
		case 'X':
			xflag = 1;
			break;

		case 'M':
			Mflag = 1;
			break;

		case 't':
			trace = 1;
			break;

		case 'i':	/* the PDP-11's: every program here has */
		case 'n':	/*   its text apart from its data, and */
		case 'z':	/*   shares it */
			break;

		default:
			error(1, "-%s: no such flag", ap);
		}
	}
}

/*
 * -lx: libx.a, in the directories of -L and then the system's, which
 * have the small model's libraries, and the medium's in a directory m
 * of each.
 */
library(x)
char *x;
{
	static char *sysdirs[] = { "/lib", "/usr/lib", "/usr/local/lib", 0 };
	char path[256];
	register i;

	for (i = 0; i < ndir || sysdirs[i - ndir]; i++) {
		if (i < ndir)
			sprintf(path, "%s/lib%s.a", dirs[i], x);
		else
			sprintf(path, "%s%s/lib%s.a", sysdirs[i - ndir],
			    model == A_MEDIUM ? "/m" : "", x);
		if (access(path, 4) == 0) {
			loadarg(path);
			return;
		}
	}
	error(1, "cannot find library -l%s", x);
}

/*
 * A file: an object, all of which is loaded, or a library.
 */
loadarg(name)
char *name;
{
	char magic[SARMAG];

	filname = name;
	if ((fa = fopen(name, "r")) == NULL || (fb = fopen(name, "r")) == NULL
	    || (fc = fopen(name, "r")) == NULL)
		error(1, "cannot open %s", name);
	if (fread(magic, 1, SARMAG, fa) == SARMAG &&
	    strncmp(magic, ARMAG, SARMAG) == 0) {
		if (pass == 1)
			lib1();
		else
			while (liblist[libnext] != -1)
				load2(liblist[libnext++]);
		libnext++;
	} else if (pass == 2)
		load2(0L);
	else if (!load1(0L))
		error(1, "%s is not an object file or a library", name);
	fclose(fa);
	fclose(fb);
	fclose(fc);
}

/*
 * The first pass over a library: load whatever member defines a symbol
 * that is not defined yet, which the table that ranlib made says; and
 * again, for as long as a member that was loaded wanted another.
 */
lib1()
{
	struct ar_hdr ah;
	char name[NAMESZ];
	register struct sym *sp;
	long nran, tab, strs, off, sx, pos;
	register long i;
	int loaded;

	if (fread((char *)&ah, sizeof ah, 1, fa) != 1 ||
	    strncmp(ah.ar_name, RANLIBMAG, sizeof RANLIBMAG - 1) != 0)
		error(1, "%s has no table of contents: run ranlib", filname);
	nran = get4(fa) / 8;
	tab = SARMAG + sizeof ah + 4;
	strs = tab + nran * 8 + 4;
	do {
		loaded = 0;
		fseek(fb, tab, 0);
		fseek(fc, strs, 0);
		pos = 0;
		for (i = 0; i < nran; i++) {
			sx = get4(fb);
			off = get4(fb);
			if (sx != pos) {
				fseek(fc, strs + sx, 0);
				pos = sx;
			}
			pos += getstr(fc, name);
			sp = lookup(name);
			if (sp == NULL || sp->s_type != N_UNDF)
				continue;
			addlib(member(off));
			loaded++;
			if (!load1(liblist[nlib-1]))
				error(1, "%s: a member is not an object",
				    filname);
			/* which read its symbols with these two */
			fseek(fb, tab + (i + 1) * 8, 0);
			pos = -1;
		}
	} while (loaded);
	addlib(-1L);
}

/*
 * Where the contents of a member start, given where its header is.
 */
long
member(off)
long off;
{
	struct ar_hdr ah;

	fseek(fa, off, 0);
	if (fread((char *)&ah, sizeof ah, 1, fa) != 1 ||
	    strncmp(ah.ar_fmag, ARFMAG, sizeof ah.ar_fmag) != 0)
		error(1, "%s: not as its table of contents has it", filname);
	off += sizeof ah;
	if (strncmp(ah.ar_name, AR_EFMT1, sizeof AR_EFMT1 - 1) == 0)
		off += atol(ah.ar_name + sizeof AR_EFMT1 - 1);
	return(off);
}

addlib(off)
long off;
{
	if (nlib >= maxlib) {
		maxlib += 128;
		if (liblist == NULL)
			liblist = (long *)malloc(maxlib * sizeof(long));
		else
			liblist = (long *)realloc((char *)liblist,
			    maxlib * sizeof(long));
		if (liblist == NULL)
			error(1, "out of memory", "");
	}
	liblist[nlib++] = off;
}

/*
 * Read the header of the object that starts at off, and find its
 * symbol table: 0 if it is no object.
 */
gethdr(off)
long off;
{
	register i;

	fseek(fa, off, 0);
	if (fread((char *)&filhdr, sizeof filhdr, 1, fa) != 1 ||
	    filhdr.a_magic != A_OBJECT || filhdr.a_nseg != 3 ||
	    fread((char *)filseg, sizeof filseg[0], 3, fa) != 3)
		return(0);
	if (pass == 1 && filhdr.a_model && filhdr.a_model != model)
		error(0, "%s is for another model", filname);
	symoff = off + sizeof filhdr + sizeof filseg;
	for (i = 0; i < 3; i++)
		symoff += even(filseg[i].xs_filesz) +
		    (long)filseg[i].xs_nrel * sizeof(struct reloc);
	stroff = symoff + filhdr.a_syms;
	nsyms = filhdr.a_syms / sizeof(struct nlist);
	return(1);
}

/*
 * Where the text, the data and the bss of the object in hand go.
 */
place()
{
	long t;

	t = filseg[0].xs_memsz;
	tpos = even(tpos);
	if (t && tpos + t > limit) {
		if (model == A_SMALL || tpos == 0) {
			if (pass == 1)
				error(0, model == A_SMALL ?
				    "%s: more than 64K of text; try the medium model" :
				    "%s: its text is more than a segment",
				    filname);
		} else {
			seglen[curseg] = tpos;
			if (++curseg > NCODE)
				error(1, "too many segments of text", "");
			tpos = 0;
		}
	}
	tseg = curseg;
	tbase = tpos;
	tpos += t;
	dpos = even(dpos);
	dbase = dpos;
	dpos += filseg[1].xs_memsz;
	bpos = even(bpos);
	bbase = bpos;
	bpos += filseg[2].xs_memsz;
	if (pass == 1 && dpos + bpos > 65536L)
		error(1, "%s: more than 64K of data", filname);
}

/*
 * The first pass over an object: its symbols.
 */
load1(off)
long off;
{
	struct nlist n;
	char name[NAMESZ];
	register struct sym *sp;
	register unsigned i;
	register type;
	unsigned v;
	long pos;

	if (!gethdr(off))
		return(0);
	if (trace)
		printf("%s\n", filname);
	place();
	fseek(fb, symoff, 0);
	pos = -1;
	for (i = 0; i < nsyms; i++) {
		if (fread((char *)&n, sizeof n, 1, fb) != 1)
			error(1, "%s: bad symbol table", filname);
		if (n.n_un.n_strx != pos) {
			pos = n.n_un.n_strx;
			fseek(fc, stroff + pos, 0);
		}
		pos += getstr(fc, name);
		type = n.n_type & N_TYPE;
		v = n.n_value;
		if ((n.n_type & N_EXT) == 0) {
			if (!xflag) {
				nlocal++;
				loclen += strlen(name) + 1;
			}
			continue;
		}
		if ((sp = lookup(name)) == NULL)
			sp = enter(name);
		if (type == N_UNDF) {
			if (v == 0)
				continue;
			/* common */
			if (sp->s_type == N_UNDF) {
				sp->s_type = N_COMM;
				sp->s_value = v;
			} else if (sp->s_type == N_COMM && v > sp->s_value)
				sp->s_value = v;
			continue;
		}
		if (sp->s_type != N_UNDF && sp->s_type != N_COMM) {
			error(0, "%s is defined more than once", name);
			continue;
		}
		sp->s_type = type;
		sp->s_seg = 0;
		switch (type) {

		case N_TEXT:
			sp->s_seg = tseg;
			v += tbase;
			break;

		case N_DATA:
			v += dbase;
			break;

		case N_BSS:
			v += bbase;
			break;
		}
		sp->s_value = v;
	}
	return(1);
}

/*
 * Between the passes.  The bss follows the data, and what is still
 * common follows the bss.
 */
middle()
{
	register struct sym *sp;
	long c;
	int undef;

	seglen[curseg] = even(tpos);
	ncode = curseg;
	dseg = ncode + 1;
	dsize = even(dpos);
	bssorg = dsize;
	comorg = bssorg + even(bpos);
	c = 0;
	undef = 0;
	for (sp = sym0; sp; sp = sp->s_link) {
		switch (sp->s_type) {

		case N_BSS:
			sp->s_value += (unsigned)bssorg;
			break;

		case N_COMM:
			c = even(c);
			sp->s_type = N_BSS;
			c += sp->s_value;
			sp->s_value = (unsigned)(comorg + c - sp->s_value);
			break;
		}
	}
	memsize = comorg + even(c);
	/* The ends of things, for whatever asks where they are. */
	if ((sp = lookup("_etext")) != NULL && sp->s_type == N_UNDF) {
		sp->s_type = N_TEXT;
		sp->s_seg = ncode;
		sp->s_value = (unsigned)seglen[ncode];
	}
	if ((sp = lookup("_edata")) != NULL && sp->s_type == N_UNDF) {
		sp->s_type = N_DATA;
		sp->s_value = (unsigned)dsize;
	}
	if ((sp = lookup("_end")) != NULL && sp->s_type == N_UNDF) {
		sp->s_type = N_BSS;
		sp->s_value = (unsigned)memsize;
	}
	for (sp = sym0; sp; sp = sp->s_link) {
		if (sp->s_type == N_UNDF) {
			if (strcmp(sp->s_name, entry) == 0 &&
			    strcmp(entry, "..start") == 0)
				continue;
			if (undef++ == 0)
				fprintf(stderr, "Undefined:\n");
			fprintf(stderr, "%s\n", sp->s_name);
			errlev = 1;
			continue;
		}
		if (sp->s_name[0] == '.' && sp->s_name[1] == '.')
			continue;
		nglobal++;
		globlen += strlen(sp->s_name) + 1;
	}
	if (memsize + stack > 65536L)
		error(0, "the data and the stack are more than 64K", "");
	if (errlev)
		exit(1);
}

/*
 * Open the output, once for each part of it that is written as the
 * second pass goes: the text, the data, the symbol table and the names.
 */
setupout()
{
	long off;
	register i;

	if ((tout = fopen(ofilename, "w")) == NULL)
		error(1, "cannot create %s", ofilename);
	dout = fopen(ofilename, "r+");
	if (!sflag) {
		sout = fopen(ofilename, "r+");
		strout = fopen(ofilename, "r+");
	}
	if (dout == NULL || !sflag && (sout == NULL || strout == NULL))
		error(1, "cannot open %s again", ofilename);
	off = sizeof(struct exec) + (long)dseg * sizeof(struct exseg);
	fseek(tout, off, 0);
	for (i = 1; i <= ncode; i++)
		off += seglen[i];
	fseek(dout, off, 0);
	off += dsize;
	if (!sflag) {
		fseek(sout, off, 0);
		off += (long)(nlocal + nglobal) * sizeof(struct nlist);
		fseek(strout, off, 0);
		strx = 4;
		put4(4 + loclen + globlen, strout);
	}
}

/*
 * A symbol for the program's table.
 */
putsym(name, type, seg, value)
char *name;
unsigned value;
{
	struct nlist n;

	n.n_un.n_strx = strx;
	n.n_type = type;
	n.n_ovly = seg;
	n.n_value = value;
	fwrite((char *)&n, sizeof n, 1, sout);
	fputs(name, strout);
	putc(0, strout);
	strx += strlen(name) + 1;
}

/*
 * The second pass over an object: its text and its data.
 */
load2(off)
long off;
{
	struct nlist n;
	char name[NAMESZ];
	register struct sym *sp;
	register unsigned i;
	register type;
	long pos;

	if (!gethdr(off))
		error(1, "%s: not what it was", filname);
	place();
	if (nsyms > mapsize) {
		if (symmap)
			free((char *)symmap);
		mapsize = nsyms + 64;
		symmap = (struct sym **)malloc(mapsize * sizeof *symmap);
		if (symmap == NULL)
			error(1, "out of memory", "");
	}
	fseek(fb, symoff, 0);
	pos = -1;
	for (i = 0; i < nsyms; i++) {
		fread((char *)&n, sizeof n, 1, fb);
		if (n.n_un.n_strx != pos) {
			pos = n.n_un.n_strx;
			fseek(fc, stroff + pos, 0);
		}
		pos += getstr(fc, name);
		if (n.n_type & N_EXT) {
			symmap[i] = lookup(name);
			continue;
		}
		symmap[i] = NULL;
		if (xflag)
			continue;
		type = n.n_type & N_TYPE;
		switch (type) {

		case N_TEXT:
			putsym(name, type, tseg, n.n_value + tbase);
			break;

		case N_DATA:
			putsym(name, type, dseg, n.n_value + dbase);
			break;

		case N_BSS:
			putsym(name, type, dseg, n.n_value + bbase +
			    (unsigned)bssorg);
			break;

		default:
			putsym(name, type, 0, n.n_value);
		}
	}
	pos = off + sizeof filhdr + sizeof filseg;
	copy(pos, pos + even(filseg[0].xs_filesz) + even(filseg[1].xs_filesz),
	    0, tout);
	copy(pos + even(filseg[0].xs_filesz),
	    pos + even(filseg[0].xs_filesz) + even(filseg[1].xs_filesz) +
	    (long)filseg[0].xs_nrel * sizeof(struct reloc), 1, dout);
}

/*
 * Copy the text or the data of the object in hand, which is at off
 * and has its relocation at roff, to where it goes, made up to an even
 * length: each word that is to be relocated is, on its way.
 */
copy(off, roff, s, out)
long off, roff;
register FILE *out;
{
	struct reloc r;
	register unsigned n, pos;
	unsigned w, size;

	size = (unsigned)filseg[s].xs_filesz;
	fseek(fa, off, 0);
	fseek(fb, roff, 0);
	pos = 0;
	for (n = filseg[s].xs_nrel; n > 0; n--) {
		if (fread((char *)&r, sizeof r, 1, fb) != 1 ||
		    r.r_addr < pos || r.r_addr > size - 2 || size < 2)
			error(1, "%s: bad relocation", filname);
		while (pos < r.r_addr) {
			putc(getc(fa), out);
			pos++;
		}
		w = getc(fa);
		w |= getc(fa) << 8;
		w = relocate(&r, w, s);
		putc(w, out);
		putc(w >> 8, out);
		pos += 2;
	}
	while (pos < size) {
		putc(getc(fa), out);
		pos++;
	}
	/* what is not in the file is zero: none of the text, and only
	   the bss after the data */
	size = (unsigned)filseg[s].xs_memsz;
	if (size & 1)
		size++;
	while (pos < size) {
		putc(0, out);
		pos++;
	}
}

/*
 * A word w of segment s of the object in hand, relocated.
 */
relocate(r, w, s)
register struct reloc *r;
unsigned w;
{
	register struct sym *sp;
	register seg;
	unsigned v;
	char *name;

	name = "its own";
	if (r->r_type & R_EXT) {
		if (r->r_sym >= nsyms || (sp = symmap[r->r_sym]) == NULL)
			error(1, "%s: bad relocation", filname);
		name = sp->s_name;
		v = sp->s_value;
		seg = sp->s_type == N_TEXT ? sp->s_seg :
		    sp->s_type == N_ABS ? 0 : dseg;
	} else switch (r->r_sym) {

	case 1:
		seg = tseg;
		v = tbase;
		break;

	case 2:
		seg = dseg;
		v = dbase;
		break;

	case 3:
		seg = dseg;
		v = bbase + (unsigned)bssorg;
		break;

	default:
		error(1, "%s: bad relocation", filname);
	}
	switch (r->r_type & R_TYPE) {

	case R_OFF:
		return(w + v);

	case R_SEG:
		if (seg == 0)
			error(0, "%s has no segment", name);
		return(SELECTOR(seg));

	case R_PCREL:
		if (seg != (s ? dseg : tseg)) {
			fprintf(stderr, "ld: %s: ", filname);
			error(0, "%s is called near from another segment",
			    name);
		}
		return(w + v - (s ? dbase : tbase));
	}
	error(1, "%s: bad relocation", filname);
	return(0);
}

/*
 * After the second pass: the symbols that all the files share, and
 * the header.
 */
finishout()
{
	register struct sym *sp;
	register i;
	struct exec hdr;
	struct exseg seg;

	hdr.a_magic = A_MAGIC;
	hdr.a_model = model;
	hdr.a_nseg = dseg;
	hdr.a_eseg = 1;
	hdr.a_entry = 0;
	hdr.a_dseg = dseg;
	hdr.a_stack = stack;
	hdr.a_unused = 0;
	hdr.a_syms = 0;
	hdr.a_strs = 0;
	for (sp = sym0; sp; sp = sp->s_link) {
		i = sp->s_type == N_TEXT ? sp->s_seg :
		    sp->s_type == N_ABS ? 0 : dseg;
		if (strcmp(sp->s_name, entry) == 0) {
			if (sp->s_type == N_TEXT) {
				hdr.a_eseg = i;
				hdr.a_entry = sp->s_value;
			} else if (sp->s_type != N_UNDF)
				error(0, "%s is not in the text", entry);
		}
		if (sp->s_name[0] == '.' && sp->s_name[1] == '.')
			continue;
		if (Mflag)
			printf("%2d:%04x %s\n", i, sp->s_value, sp->s_name);
		if (!sflag)
			putsym(sp->s_name, sp->s_type | N_EXT, i,
			    sp->s_value);
	}
	if (!sflag) {
		hdr.a_syms = (long)(nlocal + nglobal) * sizeof(struct nlist);
		hdr.a_strs = strx;
		if (strx != 4 + loclen + globlen)
			error(1, "the passes do not agree", "");
		if (ferror(sout) || ferror(strout))
			error(1, "error writing %s", ofilename);
		fclose(sout);
		fclose(strout);
	}
	if (ferror(dout))
		error(1, "error writing %s", ofilename);
	fclose(dout);
	fseek(tout, 0L, 0);
	fwrite((char *)&hdr, sizeof hdr, 1, tout);
	seg.xs_nrel = 0;
	for (i = 1; i <= dseg; i++) {
		seg.xs_type = i == dseg ? XS_DATA : XS_CODE;
		seg.xs_filesz = i == dseg ? dsize : seglen[i];
		seg.xs_memsz = i == dseg ? memsize : seglen[i];
		fwrite((char *)&seg, sizeof seg, 1, tout);
		if (Mflag)
			printf("segment %d: %s, %ld bytes\n", i,
			    i == dseg ? "data" : "text", (long)seg.xs_memsz);
	}
	if (ferror(tout))
		error(1, "error writing %s", ofilename);
	fclose(tout);
	if (errlev)
		unlink(ofilename);
	else
		chmod(ofilename, 0777 & ~umask(0));
}

/*
 * Symbols.
 */
hash(name)
register char *name;
{
	register unsigned h;

	for (h = 0; *name; name++)
		h = h * 31 + *name;
	return(h % NHASH);
}

struct sym *
lookup(name)
char *name;
{
	register struct sym *sp;

	for (sp = hashtab[hash(name)]; sp; sp = sp->s_next)
		if (strcmp(sp->s_name, name) == 0)
			return(sp);
	return(NULL);
}

struct sym *
enter(name)
char *name;
{
	register struct sym *sp;
	register h;

	sp = (struct sym *)alloc(sizeof *sp);
	sp->s_name = save(name);
	sp->s_value = 0;
	sp->s_type = N_UNDF;
	sp->s_seg = 0;
	sp->s_link = NULL;
	if (symn)
		symn->s_link = sp;
	else
		sym0 = sp;
	symn = sp;
	h = hash(name);
	sp->s_next = hashtab[h];
	hashtab[h] = sp;
	return(sp);
}

/*
 * Memory, which is never given back.
 */
char *
alloc(n)
register n;
{
	static char *blk;
	static int nblk;
	register char *p;

	n = (n + sizeof(char *) - 1) & ~(sizeof(char *) - 1);
	if (n > nblk) {
		nblk = n > 1024 ? n : 1024;
		if ((blk = malloc(nblk)) == NULL)
			error(1, "out of memory", "");
	}
	p = blk;
	blk += n;
	nblk -= n;
	return(p);
}

char *
save(s)
char *s;
{
	register char *p;

	p = alloc(strlen(s) + 1);
	strcpy(p, s);
	return(p);
}

/*
 * A long, and a name with its null, as they are in a file: how long
 * the name was in the file.
 */
long
get4(f)
register FILE *f;
{
	long n;

	n = getc(f) & 0377;
	n |= (long)(getc(f) & 0377) << 8;
	n |= (long)(getc(f) & 0377) << 16;
	n |= (long)(getc(f) & 0377) << 24;
	return(n);
}

put4(n, f)
long n;
register FILE *f;
{
	putc((int)n, f);
	putc((int)(n >> 8), f);
	putc((int)(n >> 16), f);
	putc((int)(n >> 24), f);
}

getstr(f, name)
register FILE *f;
char *name;
{
	register char *p;
	register c, n;

	p = name;
	n = 0;
	while ((c = getc(f)) != 0 && c != EOF) {
		if (p < name + NAMESZ - 1)
			*p++ = c;
		n++;
	}
	*p = 0;
	return(n + 1);
}

/*
 * A message, which may have a %s; with n the last.
 */
error(n, s, a)
char *s, *a;
{
	fprintf(stderr, "ld: ");
	fprintf(stderr, s, a);
	fprintf(stderr, "\n");
	errlev = 1;
	if (n) {
		if (pass == 2)
			unlink(ofilename);
		exit(1);
	}
}
