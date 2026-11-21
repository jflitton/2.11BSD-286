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
 * %define NAME TEXT, or -DNAME=TEXT, which is kept for both passes;
 * %define NAME(A,B) TEXT, where what NAME is followed by in brackets
 * takes the place of A and B; and %assign NAME VALUE, which is the
 * number.
 */
#define	NPARAM	4
struct def {
	struct	def *d_next;
	char	*d_name;
	char	*d_text;
	int	d_keep;
	int	d_nparam;	/* -1 without brackets */
	char	*d_param[NPARAM];
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

/*
 * Where lines come from other than the files: a macro that is being
 * expanded, with its arguments, or what %rep repeats, one in another.
 */
#define	NFRAME	8
struct frame {
	struct	mline *f_line;	/* the next */
	struct	mline *f_body;	/* the first, to go round again */
	int	f_count;	/* how many more times round */
	int	f_macro;	/* a macro's lines, which have %1 ... */
	char	*f_arg[9];
	char	f_args[LINESZ];
} frame[NFRAME];
int	nframe;

/*
 * %if, %ifdef and %ifndef, and their %elif and %else: for each one
 * open, whether its lines are being assembled, or skipped until one of
 * its conditions holds, or skipped because one has, or because all of
 * it is inside one that is being skipped.
 */
#define	NIF	8
#define	I_TAKE	0
#define	I_SEEK	1
#define	I_DONE	2
#define	I_OUTER	3
char	ifstate[NIF];
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
			error("%%if without %%endif", "");
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
	nframe = 0;
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
 * The next line as it is, from a macro or %rep or the files, into
 * line: 0 when there are no more.
 */
rawline()
{
	register struct frame *f;
	register struct mline *ml;
	register char *p;
	register c;

	for (;;) {
		if (nframe > 0) {
			f = &frame[nframe-1];
			if ((ml = f->f_line) == NULL) {
				if (f->f_count > 0) {
					f->f_count--;
					f->f_line = f->f_body;
				} else
					nframe--;
				continue;
			}
			f->f_line = ml->ml_next;
			if (f->f_macro)
				margsub(f, ml->ml_text);
			else
				strcpy(line, ml->ml_text);
			return(1);
		}
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
			while ((c = getc(infp[nin-1])) != EOF && c != '\n')
				;
		}
		return(1);
	}
}

/*
 * The next line to assemble, in line with lp at its start: 0 when there
 * are no more.  The lines that begin with % are done here.
 */
nextline()
{
	register char *p;

	for (;;) {
		if (!rawline())
			return(0);
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
			defsub(line);
		paste();
		lp = line;
		return(1);
	}
}

/*
 * The value of what is left of a line that begins with %, after its
 * names are put in their places: it has to be a number.
 */
long
ppvalue(what)
char *what;
{
	struct expr e;
	char *p;

	p = lp;
	if (defs)
		defsub(p);
	lp = p;
	expr(&e);
	if (!isnum(&e) || e.e_fwd)
		error("%%%s of what is not a number known here", what);
	else if (!eol())
		error("more on the line than there should be: %s", lp);
	return(e.e_val);
}

/*
 * A line that begins with %.
 */
percent()
{
	char name[NAMESZ], num[16];
	register struct def *dp;
	register char *p;
	register n;
	long v;

	getname(name);
	skipb();
	if (strcmp(name, "ifdef") == 0 || strcmp(name, "ifndef") == 0 ||
	    strcmp(name, "if") == 0) {
		if (nif >= NIF)
			fatal("%%if too deep", "");
		if (skipping)
			ifstate[nif] = I_OUTER;
		else if (name[2] == 0)
			ifstate[nif] = ppvalue(name) ? I_TAKE : I_SEEK;
		else {
			n = name[2] == 'n';
			getname(name);
			for (dp = defs; dp; dp = dp->d_next)
				if (strcmp(dp->d_name, name) == 0)
					break;
			ifstate[nif] = (dp != NULL) != n ? I_TAKE : I_SEEK;
		}
		nif++;
	} else if (strcmp(name, "elif") == 0) {
		if (nif == 0)
			error("%%elif without %%if", "");
		else if (ifstate[nif-1] == I_TAKE)
			ifstate[nif-1] = I_DONE;
		else if (ifstate[nif-1] == I_SEEK)
			ifstate[nif-1] = ppvalue(name) ? I_TAKE : I_SEEK;
	} else if (strcmp(name, "else") == 0) {
		if (nif == 0)
			error("%%else without %%if", "");
		else if (ifstate[nif-1] == I_TAKE)
			ifstate[nif-1] = I_DONE;
		else if (ifstate[nif-1] == I_SEEK)
			ifstate[nif-1] = I_TAKE;
	} else if (strcmp(name, "endif") == 0) {
		if (nif == 0)
			error("%%endif without %%if", "");
		else
			nif--;
	} else if (strcmp(name, "macro") == 0) {
		getname(name);
		defmacro(skipping ? (char *)NULL : name);
		return;
	} else if (strcmp(name, "rep") == 0) {
		v = skipping ? 0 : ppvalue(name);
		rep(v);
		return;
	} else if (strcmp(name, "endrep") == 0) {
		error("%%endrep without %%rep", "");
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
		define(name, lp, 0);
		return;
	} else if (strcmp(name, "assign") == 0) {
		getname(name);
		v = ppvalue(name);
		sprintf(num, "%ld", v);
		lp = num;
		define(name, lp, 0);
		return;
	} else {
		error("%%%s is not known", name);
		return;
	}
	skipping = nif > 0 && ifstate[nif-1] != I_TAKE;
}

cmddef(s)
register char *s;
{
	register char *p;

	for (p = s; *p && *p != '='; p++)
		;
	if (*p)
		*p++ = 0;
	lp = p;
	define(s, p, 1);
}

/*
 * Define NAME as the rest of the line at lp, less a comment and the
 * blanks before it.  If the rest begins with a bracket, what is in it
 * are the names of what NAME is to be followed by.
 */
define(name, text, keep)
char *name, *text;
{
	register struct def *dp;
	register char *p, *e;
	char param[NAMESZ];
	int np;
	char *pv[NPARAM];

	if (name[0] == 0) {
		error("%%define of what?", "");
		return;
	}
	np = -1;
	if (*lp == '(') {
		lp++;
		for (np = 0; ; ) {
			skipb();
			if (*lp == ')')
				break;
			getname(param);
			if (param[0] == 0 || np >= NPARAM) {
				error("%%define %s( of what?", name);
				return;
			}
			pv[np++] = save(param);
			skipb();
			if (*lp == ',')
				lp++;
		}
		lp++;
	}
	skipb();
	text = lp;
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
	dp->d_nparam = np;
	while (--np >= 0)
		dp->d_param[np] = pv[np];
}

/*
 * The text of a %define with brackets, with what it was followed by in
 * place of the names of those: into to, which has n bytes.  What it was
 * followed by is at *pp, after the name, which is moved past them.
 * 0 if it was not followed by brackets, and is not to be replaced.
 */
defargs(dp, pp, to, n)
register struct def *dp;
char **pp, *to;
{
	char args[LINESZ], name[NAMESZ];
	char *av[NPARAM];
	register char *p, *q;
	register i;
	int na, depth;
	char *s;

	p = *pp;
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p != '(')
		return(0);
	p++;
	q = args;
	na = 0;
	av[na++] = q;
	for (depth = 0; *p && *p != '\n'; p++) {
		if (*p == '(')
			depth++;
		else if (*p == ')' && depth-- == 0)
			break;
		else if (*p == ',' && depth == 0) {
			*q++ = 0;
			if (na >= NPARAM)
				break;
			av[na++] = q;
			continue;
		}
		*q++ = *p;
	}
	*q = 0;
	if (*p != ')' || (dp->d_nparam == 0 ? args[0] != 0 :
	    na != dp->d_nparam)) {
		error("%s( with what it does not take", dp->d_name);
		return(0);
	}
	*pp = p + 1;
	q = to;
	for (s = dp->d_text; *s; ) {
		if (!idstart(*s)) {
			if (q >= to + n - 1)
				goto toolong;
			*q++ = *s++;
			continue;
		}
		lp = s;
		getname(name);
		s = lp;
		p = name;
		for (i = 0; i < dp->d_nparam; i++)
			if (strcmp(dp->d_param[i], name) == 0) {
				p = av[i];
				break;
			}
		if (strlen(p) >= to + n - q)
			goto toolong;
		strcpy(q, p);
		q += strlen(q);
	}
	*q = 0;
	return(1);
toolong:
	error("line too long", "");
	*to = 0;
	return(1);
}

/*
 * Put the text of each defined name in the line, from buf on, in its
 * place, and again in what comes of that, until none is left.
 */
defsub(buf)
char *buf;
{
	register n;

	for (n = 0; defsub1(buf); )
		if (++n >= 16) {
			error("a %%define that never ends", "");
			break;
		}
}

defsub1(buf)
char *buf;
{
	char new[LINESZ], text[LINESZ];
	char name[NAMESZ];
	char *p;
	register char *q;
	register struct def *dp;
	char *s;
	int c, any, room;

	any = 0;
	room = line + LINESZ - buf;
	q = new;
	for (p = buf; (c = *p) != 0 && c != ';'; ) {
		if (q >= new + room - NAMESZ)
			goto toolong;
		if (c == '\'' || c == '"') {
			do {
				*q++ = *p++;
				if (q >= new + room - NAMESZ)
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
		p = lp;
		for (dp = defs; dp; dp = dp->d_next)
			if (strcmp(dp->d_name, name) == 0)
				break;
		s = name;
		if (dp && dp->d_nparam >= 0) {
			if (defargs(dp, &p, text, sizeof text)) {
				s = text;
				any++;
			}
		} else if (dp) {
			s = dp->d_text;
			any++;
		}
		if (strlen(s) >= new + room - q)
			goto toolong;
		strcpy(q, s);
		q += strlen(q);
	}
	if (any == 0)
		return(0);
	*q++ = '\n';
	*q = 0;
	strcpy(buf, new);
	return(1);
toolong:
	error("line too long", "");
	return(0);
}

/*
 * %+ between two things makes one of them: vec %+ 3 is vec3.
 */
paste()
{
	register char *p, *q;
	register c;

	for (p = q = line; (c = *p) != 0; ) {
		if (c == '\'' || c == '"') {
			do
				*q++ = *p++;
			while (*p && *p != c);
			if (*p)
				*q++ = *p++;
			continue;
		}
		if (c == ';')
			break;
		if (c == '%' && p[1] == '+') {
			while (q > line && (q[-1] == ' ' || q[-1] == '\t'))
				q--;
			for (p += 2; *p == ' ' || *p == '\t'; p++)
				;
			continue;
		}
		*q++ = *p++;
	}
	while (*p)
		*q++ = *p++;
	*q = 0;
}

/*
 * The lines to the next %endrep that is not another's, to go through
 * n times.
 */
rep(n)
long n;
{
	register struct frame *f;
	register struct mline *ml, **mlp;
	register char *p;
	struct mline *body;
	int depth;

	body = NULL;
	mlp = &body;
	for (depth = 0; ; ) {
		if (!rawline()) {
			error("%%rep without %%endrep", "");
			return;
		}
		stmt++;
		for (p = line; *p == ' ' || *p == '\t'; p++)
			;
		if (strncmp(p, "%rep", 4) == 0 && !idchar(p[4]))
			depth++;
		else if (strncmp(p, "%endrep", 7) == 0 && !idchar(p[7]) &&
		    depth-- == 0)
			break;
		if (n > 0) {
			ml = (struct mline *)alloc(sizeof *ml);
			ml->ml_text = save(line);
			ml->ml_next = NULL;
			*mlp = ml;
			mlp = &ml->ml_next;
		}
	}
	if (n <= 0 || body == NULL)
		return;
	if (nframe >= NFRAME)
		fatal("%%rep and macros too deep", "");
	f = &frame[nframe++];
	f->f_body = f->f_line = body;
	f->f_count = n - 1;
	f->f_macro = 0;
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
		if (!rawline()) {
			error("%%macro without %%endmacro", "");
			return;
		}
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
	register struct frame *f;
	register char *p;
	register n;

	for (mp = macros; mp; mp = mp->m_next)
		if (strcmp(mp->m_name, name) == 0)
			break;
	if (mp == NULL)
		return(0);
	if (nframe >= NFRAME) {
		error("macros too deep: %s", name);
		return(1);
	}
	f = &frame[nframe++];
	strcpy(f->f_args, lp);
	for (n = 0; n < 9; n++)
		f->f_arg[n] = "";
	p = f->f_args;
	for (n = 0; n < 9; n++) {
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p == 0 || *p == '\n' || *p == ';')
			break;
		f->f_arg[n] = p;
		while (*p && *p != ',' && *p != '\n' && *p != ';')
			p++;
		if (*p == ',')
			*p++ = 0;
		else
			*p = 0;
	}
	while (*lp)
		lp++;
	f->f_body = f->f_line = mp->m_body;
	f->f_count = 0;
	f->f_macro = 1;
	return(1);
}

/*
 * A line of a macro, with its arguments in it, into line.
 */
margsub(f, s)
register struct frame *f;
register char *s;
{
	register char *q, *a;

	for (q = line; *s; ) {
		if (*s == '%' && s[1] >= '1' && s[1] <= '9') {
			a = f->f_arg[s[1] - '1'];
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
	int c;

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
		c = *lp == ':';
		if (c)
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
		/* a word alone with no colon is more likely an
		   instruction this does not know than a label */
		if (!c && eol()) {
			error("%s is no instruction known here", name);
			return;
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
