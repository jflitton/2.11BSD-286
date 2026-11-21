/*
 * The assembler for the 80286: its two passes, the lines it reads and
 * what is done to them before they are assembled, and its symbols.
 */
#include "as.h"

int	pass;
int	nerr;
unsigned stmt;			/* lines read in this pass, of any file */
int	model;			/* -ms, -mm or -ml: for the object's header */
char	*outfile = "a.out";
char	line[LINESZ];
char	*lp;			/* where in the line */

/*
 * The files being read: the source, and what it %includes.
 */
FILE	*infp[NINCL];
char	*inname[NINCL];
unsigned inlno[NINCL];
int	nin;
char	*incdir[NINCL];
int	nincdir;
char	*source;

/*
 * %define NAME TEXT, or -DNAME=TEXT, which is kept for both passes.
 */
struct def {
	struct	def *d_next;
	char	*d_name;
	char	*d_text;
	int	d_keep;
};
struct	def *defs;

/*
 * %macro NAME N and its lines, in which %1 to %N are the arguments.
 */
struct mline {
	struct	mline *ml_next;
	char	*ml_text;
};
struct macro {
	struct	macro *m_next;
	char	*m_name;
	struct	mline *m_body;
};
struct	macro *macros;
struct	mline *mexp;		/* the next line of the one in hand */
char	margs[LINESZ];
char	*marg[9];

/*
 * %ifdef: for each one open, whether its lines are being skipped (1),
 * or all of it is because it is inside one that is (2).
 */
#define	NIF	8
char	ifskip[NIF];
int	nif;
int	skipping;

struct	sym *hashtab[NHASH];
struct	sym *lastlab;		/* the last label without a dot */
struct	sym *text0, *textn;	/* the text's labels in order */

char	*blk;			/* alloc's */
int	nblk;

main(argc, argv)
char **argv;
{
	register char *p;

	for (argc--, argv++; argc > 0; argc--, argv++) {
		p = *argv;
		if (*p != '-') {
			if (source)
				fatal("one file at a time", "");
			source = p;
			continue;
		}
		switch (p[1]) {

		case 'o':
			if (--argc <= 0)
				usage();
			outfile = *++argv;
			break;

		case 'D':
			if (p[2] == 0) {
				if (--argc <= 0)
					usage();
				p = *++argv;
			} else
				p += 2;
			cmddef(p);
			break;

		case 'I':
			if (p[2] == 0) {
				if (--argc <= 0)
					usage();
				p = *++argv;
			} else
				p += 2;
			if (nincdir >= NINCL)
				fatal("too many -I", "");
			incdir[nincdir++] = p;
			break;

		case 'm':
			if (p[2] == 's')
				model = 1;
			else if (p[2] == 'm')
				model = 2;
			else if (p[2] == 'l')
				model = 3;
			else
				usage();
			break;

		default:
			usage();
		}
	}
	if (source == NULL)
		usage();
	tabcheck();
	for (pass = 1; pass <= 2; pass++) {
		start();
		while (nextline())
			statement();
		if (nif)
			error("%%ifdef without %%endif", "");
		if (pass == 1) {
			if (nerr)
				exit(1);
			relax();
			outstart();
		}
	}
	outfinish();
	if (nerr) {
		unlink(outfile);
		exit(1);
	}
	exit(0);
}

usage()
{
	fprintf(stderr,
	    "usage: as [-o object] [-Dname[=text]] [-Idir] [-ms|-mm|-ml] file.s\n");
	exit(1);
}

/*
 * Begin a pass.  The names the source defines for itself are forgotten,
 * to be defined where they are again.
 */
start()
{
	register struct def *dp, **dpp;
	register i;

	for (dpp = &defs; (dp = *dpp) != NULL; )
		if (dp->d_keep)
			dpp = &dp->d_next;
		else
			*dpp = dp->d_next;
	macros = NULL;
	mexp = NULL;
	nif = 0;
	skipping = 0;
	stmt = 0;
	lastlab = NULL;
	for (i = 0; i < NSECT; i++)
		loc[i] = 0;
	sect = STEXT;
	jumpstart();
	nin = 0;
	include(source);
	if (nin == 0)
		exit(1);
}

include(name)
char *name;
{
	register FILE *f;
	register i;
	char path[LINESZ];

	if (nin >= NINCL) {
		error("%%include too deep: %s", name);
		return;
	}
	f = fopen(name, "r");
	for (i = 0; f == NULL && i < nincdir; i++) {
		strcpy(path, incdir[i]);
		if (path[0] && path[strlen(path) - 1] != '/')
			strcat(path, "/");
		strcat(path, name);
		f = fopen(path, "r");
	}
	if (f == NULL) {
		error("cannot open %s", name);
		return;
	}
	infp[nin] = f;
	inname[nin] = save(name);
	inlno[nin] = 0;
	nin++;
}

/*
 * The next line to assemble, in line with lp at its start: 0 when there
 * are no more.  The lines that begin with % are done here.
 */
nextline()
{
	register char *p;
	register c;

	for (;;) {
		if (mexp) {
			margsub(mexp->ml_text);
			mexp = mexp->ml_next;
		} else {
			if (nin == 0)
				return(0);
			if (fgets(line, LINESZ, infp[nin-1]) == NULL) {
				fclose(infp[nin-1]);
				if (--nin == 0)
					return(0);
				continue;
			}
			inlno[nin-1]++;
			p = line + strlen(line);
			if (p == line + LINESZ - 1 && p[-1] != '\n') {
				error("line too long", "");
				while ((c = getc(infp[nin-1])) != EOF &&
				    c != '\n')
					;
			}
		}
		if (++stmt == 0)
			fatal("too many lines", "");
		for (p = line; *p == ' ' || *p == '\t'; p++)
			;
		if (*p == '%') {
			lp = p + 1;
			percent();
			continue;
		}
		if (skipping)
			continue;
		if (defs)
			defsub();
		lp = line;
		return(1);
	}
}

/*
 * A line that begins with %.
 */
percent()
{
	char name[NAMESZ];
	register struct def *dp;
	register char *p;
	register n;

	getname(name);
	skipb();
	if (strcmp(name, "ifdef") == 0 || strcmp(name, "ifndef") == 0) {
		if (nif >= NIF)
			fatal("%%ifdef too deep", "");
		if (skipping)
			ifskip[nif] = 2;
		else {
			n = name[2] == 'n';
			getname(name);
			for (dp = defs; dp; dp = dp->d_next)
				if (strcmp(dp->d_name, name) == 0)
					break;
			ifskip[nif] = n ? dp != NULL : dp == NULL;
		}
		nif++;
	} else if (strcmp(name, "else") == 0) {
		if (nif == 0)
			error("%%else without %%ifdef", "");
		else if (ifskip[nif-1] != 2)
			ifskip[nif-1] = !ifskip[nif-1];
	} else if (strcmp(name, "endif") == 0) {
		if (nif == 0)
			error("%%endif without %%ifdef", "");
		else
			nif--;
	} else if (strcmp(name, "macro") == 0) {
		getname(name);
		defmacro(skipping ? (char *)NULL : name);
		return;
	} else if (skipping) {
		return;
	} else if (strcmp(name, "include") == 0) {
		if (*lp != '"') {
			error("%%include \"file\"", "");
			return;
		}
		for (p = ++lp; *p && *p != '"'; p++)
			;
		*p = 0;
		include(lp);
		return;
	} else if (strcmp(name, "define") == 0) {
		getname(name);
		skipb();
		define(name, lp, 0);
		return;
	} else {
		error("%%%s is not known", name);
		return;
	}
	skipping = nif > 0 && ifskip[nif-1];
}

cmddef(s)
register char *s;
{
	register char *p;

	for (p = s; *p && *p != '='; p++)
		;
	if (*p)
		*p++ = 0;
	define(s, p, 1);
}

/*
 * Define NAME as the rest of the line, less a comment and the blanks
 * before it.
 */
define(name, text, keep)
char *name, *text;
{
	register struct def *dp;
	register char *p, *e;

	if (name[0] == 0) {
		error("%%define of what?", "");
		return;
	}
	for (p = e = text; *p && *p != '\n' && *p != ';'; p++)
		if (*p != ' ' && *p != '\t')
			e = p + 1;
	*e = 0;
	for (dp = defs; dp; dp = dp->d_next)
		if (strcmp(dp->d_name, name) == 0)
			break;
	if (dp == NULL) {
		dp = (struct def *)alloc(sizeof *dp);
		dp->d_name = save(name);
		dp->d_next = defs;
		defs = dp;
	}
	dp->d_text = save(text);
	dp->d_keep = keep;
}

/*
 * Put the text of each defined name in the line in its place.  What
 * comes of that is not looked at again.
 */
defsub()
{
	char new[LINESZ];
	char name[NAMESZ];
	register char *p, *q;
	register struct def *dp;
	char *s;
	int c, any;

	any = 0;
	q = new;
	for (p = line; (c = *p) != 0 && c != ';'; ) {
		if (q >= new + LINESZ - NAMESZ)
			goto toolong;
		if (c == '\'' || c == '"') {
			do {
				*q++ = *p++;
				if (q >= new + LINESZ - NAMESZ)
					goto toolong;
			} while (*p && *p != c);
			if (*p)
				*q++ = *p++;
			continue;
		}
		if (!idstart(c)) {
			*q++ = *p++;
			if (c >= '0' && c <= '9')
				while (idchar(*p))
					*q++ = *p++;
			continue;
		}
		lp = p;
		getname(name);
		for (dp = defs; dp; dp = dp->d_next)
			if (strcmp(dp->d_name, name) == 0)
				break;
		s = dp ? dp->d_text : name;
		if (dp)
			any++;
		if (strlen(s) >= new + LINESZ - q)
			goto toolong;
		strcpy(q, s);
		q += strlen(q);
		p = lp;
	}
	if (any == 0)
		return;
	*q++ = '\n';
	*q = 0;
	strcpy(line, new);
	return;
toolong:
	error("line too long", "");
}

/*
 * %macro NAME N: keep the lines to %endmacro.  With no name they are
 * only passed over.
 */
defmacro(name)
char *name;
{
	register struct macro *mp;
	register struct mline *ml, **mlp;
	register char *p;

	mp = NULL;
	if (name) {
		mp = (struct macro *)alloc(sizeof *mp);
		mp->m_name = save(name);
		mp->m_body = NULL;
		mp->m_next = macros;
		macros = mp;
		mlp = &mp->m_body;
	}
	for (;;) {
		if (fgets(line, LINESZ, infp[nin-1]) == NULL) {
			error("%%macro without %%endmacro", "");
			return;
		}
		inlno[nin-1]++;
		stmt++;
		for (p = line; *p == ' ' || *p == '\t'; p++)
			;
		if (strncmp(p, "%endmacro", 9) == 0)
			return;
		if (mp) {
			ml = (struct mline *)alloc(sizeof *ml);
			ml->ml_text = save(line);
			ml->ml_next = NULL;
			*mlp = ml;
			mlp = &ml->ml_next;
		}
	}
}

/*
 * If NAME is a macro, have its lines read next, with the rest of this
 * line as its arguments.
 */
macro(name)
char *name;
{
	register struct macro *mp;
	register char *p;
	register n;

	for (mp = macros; mp; mp = mp->m_next)
		if (strcmp(mp->m_name, name) == 0)
			break;
	if (mp == NULL)
		return(0);
	if (mexp) {
		error("a macro in a macro: %s", name);
		return(1);
	}
	strcpy(margs, lp);
	for (n = 0; n < 9; n++)
		marg[n] = "";
	p = margs;
	for (n = 0; n < 9; n++) {
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p == 0 || *p == '\n' || *p == ';')
			break;
		marg[n] = p;
		while (*p && *p != ',' && *p != '\n' && *p != ';')
			p++;
		if (*p == ',')
			*p++ = 0;
		else
			*p = 0;
	}
	while (*lp)
		lp++;
	mexp = mp->m_body;
	return(1);
}

/*
 * A line of a macro, with its arguments in it, into line.
 */
margsub(s)
register char *s;
{
	register char *q, *a;

	for (q = line; *s; ) {
		if (*s == '%' && s[1] >= '1' && s[1] <= '9') {
			a = marg[s[1] - '1'];
			s += 2;
		} else
			a = NULL;
		if (q + (a ? strlen(a) : 1) >= line + LINESZ - 1) {
			error("line too long", "");
			break;
		}
		if (a) {
			strcpy(q, a);
			q += strlen(q);
			while (q > line && (q[-1] == ' ' || q[-1] == '\t'))
				q--;
		} else
			*q++ = *s++;
	}
	*q = 0;
}

/*
 * Messages.  One has a %s for a string at most.
 */
error(s, a)
char *s, *a;
{
	if (nin > 0)
		fprintf(stderr, "%s:%u: ", inname[nin-1], inlno[nin-1]);
	else
		fprintf(stderr, "as: ");
	fprintf(stderr, s, a);
	fprintf(stderr, "\n");
	if (++nerr > 30)
		fatal("too many errors", "");
}

fatal(s, a)
char *s, *a;
{
	nerr = 0;
	error(s, a);
	if (pass == 2)
		unlink(outfile);
	exit(1);
}

/*
 * Memory, which is never given back.
 */
char *
alloc(n)
register n;
{
	register char *p;
	extern char *malloc();

	n = (n + sizeof(char *) - 1) & ~(sizeof(char *) - 1);
	if (n > nblk) {
		nblk = n > 1024 ? n : 1024;
		if ((blk = malloc(nblk)) == NULL)
			fatal("out of memory", "");
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
 * Symbols.
 */
struct sym *
lookup(name)
char *name;
{
	register struct sym *sp;
	register char *p;
	register unsigned h;

	/*
	 * The symbols are written in the table's order, so the sum is
	 * kept to the 16 bits an int has on the 286: an object is then
	 * the same wherever it is assembled.
	 */
	h = 0;
	for (p = name; *p; p++)
		h = (h * 31 + *p) & 0177777;
	h %= NHASH;
	for (sp = hashtab[h]; sp; sp = sp->s_next)
		if (strcmp(sp->s_name, name) == 0)
			return(sp);
	sp = (struct sym *)alloc(sizeof *sp);
	sp->s_name = save(name);
	sp->s_link = NULL;
	sp->s_value = 0;
	sp->s_stmt = 0;
	sp->s_index = -1;
	sp->s_sect = SUNDEF;
	sp->s_flags = 0;
	p = name;
	if (*p == 'L' && p[1] == 'L')
		p++;
	if (*p == 'L' && p[1] >= '0' && p[1] <= '9')
		sp->s_flags = FLOCAL;
	for (p = name; *p; p++)
		if (*p == '.')
			sp->s_flags = FLOCAL;
	if (strcmp(name, "..start") == 0)
		sp->s_flags = FGLOBAL;
	sp->s_next = hashtab[h];
	hashtab[h] = sp;
	return(sp);
}

/*
 * A name as it is looked up: one that begins with a single dot belongs
 * to the last label that had none, and has that one's name before it.
 */
struct sym *
symbol(name)
char *name;
{
	char full[2*NAMESZ];

	if (name[0] == '.' && name[1] != '.' && lastlab) {
		strcpy(full, lastlab->s_name);
		strcat(full, name);
		return(lookup(full));
	}
	return(lookup(name));
}

/*
 * Give a symbol its value: a label, or with SABS an equ.  The second
 * pass is to find every label where the first one left it.
 */
defsym(sp, s, v)
register struct sym *sp;
unsigned v;
{
	if (pass == 1) {
		if (sp->s_stmt || sp->s_flags & (FEXTERN|FCOMMON)) {
			error("%s is defined twice", sp->s_name);
			return;
		}
		sp->s_stmt = stmt;
		sp->s_sect = s;
		sp->s_value = v;
		if (s == STEXT) {
			if (textn)
				textn->s_link = sp;
			else
				text0 = sp;
			textn = sp;
		}
		return;
	}
	if (s == SABS)
		sp->s_value = v;
	else if (sp->s_sect != s || sp->s_value != v)
		fatal("%s is not where it was (phase error)", sp->s_name);
}

/*
 * Scanning.
 */
skipb()
{
	while (*lp == ' ' || *lp == '\t')
		lp++;
}

idstart(c)
register c;
{
	return(c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_' ||
	    c == '.' || c == '?' || c == '@');
}

idchar(c)
register c;
{
	return(idstart(c) || c >= '0' && c <= '9' || c == '$' || c == '#' ||
	    c == '~');
}

/*
 * The name at lp, which is moved past it.
 */
getname(name)
char *name;
{
	register char *p;

	p = name;
	if (idstart(*lp))
		while (idchar(*lp)) {
			if (p < name + NAMESZ - 1)
				*p++ = *lp;
			lp++;
		}
	*p = 0;
}

/*
 * The same in lower case, for what is a word of the assembler's
 * whichever way it is written.
 */
lower(to, from)
register char *to, *from;
{
	register c;

	while ((c = *from++) != 0)
		*to++ = c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
	*to = 0;
}

/*
 * Is the line at its end?
 */
eol()
{
	skipb();
	return(*lp == 0 || *lp == '\n' || *lp == ';' || *lp == '\r');
}

/*
 * A line: any labels, and then an instruction or a directive.
 */
statement()
{
	char name[NAMESZ], low[NAMESZ];
	register struct insn *ip;
	register struct sym *sp;
	struct expr e;
	char *p;
	long n;

	for (;;) {
		if (eol())
			return;
		if (!idstart(*lp)) {
			error("an instruction cannot begin so: %s", lp);
			return;
		}
		getname(name);
		skipb();
		lower(low, name);
		ip = NULL;
		if (*lp != ':') {
			ip = findinsn(low);
			if (ip == NULL && macros && macro(name))
				return;
		}
		if (ip)
			break;
		/* a label, or a name for a value */
		if (*lp == ':')
			lp++;
		skipb();
		p = lp;
		if (idstart(*lp)) {
			getname(low);
			lower(low, low);
			if (strcmp(low, "equ") == 0) {
				expr(&e);
				if (e.e_sect != SABS || e.e_seg)
					error("%s equ what is not a number",
					    name);
				sp = symbol(name);
				defsym(sp, SABS, (unsigned)e.e_val);
				break;
			}
			lp = p;
		}
		sp = symbol(name);
		if (name[0] != '.' || name[1] == '.')
			lastlab = sp;
		defsym(sp, sect, loc[sect]);
	}
	if (ip) {
		if (ip->i_class == C_TIMES) {
			expr(&e);
			if (e.e_sect != SABS || e.e_fwd || e.e_val < 0) {
				error("times what?", "");
				return;
			}
			skipb();
			getname(low);
			lower(low, low);
			ip = findinsn(low);
			if (ip == NULL || ip->i_class == C_TIMES) {
				error("times of what?", "");
				return;
			}
			p = lp;
			for (n = e.e_val; n > 0; n--) {
				lp = p;
				assemble(ip);
			}
		} else
			assemble(ip);
	}
	if (!eol())
		error("more on the line than there should be: %s", lp);
}
