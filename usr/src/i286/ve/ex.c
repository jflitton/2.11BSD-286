/*
 * ve: the commands after a colon, and files.
 *
 * A command is the lines it is for, its name, and what else it needs:
 *
 *	[line][,line] name[!] [what]
 *
 * A line is a number, . for the cursor's, $ for the last, 'x for the
 * one marked x, /pattern/ or ?pattern? for the next or the last before
 * that has the pattern, and any of those with +n or -n after it; % is
 * all of them, 1,$.  The names are vi's, and may be cut as short as vi
 * has them.  docmd has the list.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ve.h"

#define	IOSIZE	4096
static char	iob[IOSIZE + LINEMAX + 1];	/* a piece of a file */
static char	sbuf[2 * LINEMAX + 2];	/* a line as :s makes it */
static char	xb[MAXPATHLEN + 256];	/* what is after a command, with the
					   names of files put in */
static char	subrep[160];		/* what :s last put in, */
static int	subglobal;		/*   and whether everywhere in a line */
static int	subhave;
static char	lastbang[200];		/* the last command for the shell */
static int	inglobal;		/* :g is at work */
static int	listing;		/* lines have been shown over the screen */
static int	listrow, liststop;
static int	warned;			/* of files not yet edited */

#define	BLANK(c)	((c) == ' ' || (c) == '\t')
#define	DIGIT(c)	((c) >= '0' && (c) <= '9')
#define	ALPHA(c)	((c) >= 'a' && (c) <= 'z' || (c) >= 'A' && (c) <= 'Z')

static char *
skipb(register char *s)
{
	while (BLANK(*s))
		s++;
	return (s);
}

/*
 * Is the name at s, n long, the command full, of which at least min
 * letters must be given?
 */
static int
is(char *s, int n, char *full, int min)
{
	return (n >= min && n <= (int)strlen(full) && strncmp(s, full, n) == 0);
}

/*
 * Show a line over the screen, which is rolled up for it; when a
 * screenful has been shown, wait to be told to go on.
 */
static void
list(char *s)
{
	register int n;

	if (!active || liststop)
		return;
	if (!listing) {
		listing = 1;
		listrow = 0;
		t_attr(0);
		t_move(rows - 1, 0);
		t_clreol();
	}
	if (listrow >= rows - 1) {
		t_attr(1);
		t_puts("-- more --");
		t_attr(0);
		n = key();
		t_putc('\r');
		t_clreol();
		if (n == 'q' || n == ESC || n == CTL('C')) {
			liststop = 1;
			return;
		}
		listrow = n == '\r' || n == 'j' ? rows - 2 : 0;
	}
	for (n = 0; *s && n < cols - 1; s++) {
		if (*s == '\t') {
			do
				t_putc(' ');
			while (++n % o_ts && n < cols - 1);
			continue;
		}
		t_putc((*s & 0377) < ' ' || *s == 0177 ? '?' : *s);
		n++;
	}
	t_putc('\r');
	t_putc('\n');
	listrow++;
}

/*
 * What is after a command's name, with % made the file's name and #
 * the name of the file before it; 0 if there is not such a name.
 */
static char *
expand(register char *s)
{
	register char *p = xb, *q;

	for (; *s && p < xb + sizeof (xb) - MAXPATHLEN - 1; s++) {
		if (*s == '\\' && (s[1] == '%' || s[1] == '#'))
			*p++ = *++s;
		else if (*s == '%' || *s == '#') {
			q = *s == '%' ? fname : altname;
			if (*q == 0) {
				emsg("No file name to put for %c", *s);
				return (0);
			}
			(void)strcpy(p, q);
			p += strlen(p);
		} else
			*p++ = *s;
	}
	while (p > xb && BLANK(p[-1]))
		p--;
	*p = 0;
	return (xb);
}

/*
 * Reading a file: the bytes come in pieces, and are put together into
 * lines, which go into the file after line gat.  A line longer than a
 * line may be is cut, and null characters are left out.
 */
static long	gat, glines;
#define	gbuf	(iob + IOSIZE)	/* a line that is not all read yet: not
				   lbuf, which whatever is done between
				   two pieces of a file may use */
static int	glen, graw, gsplit, gfull;
static int	gpart;		/* the file to be edited was not all read */

static void
gflush(void)
{
	if (gfull)
		return;
	if (graw ? t_ins(&buf, gat + glines + 1, gbuf, glen) :
	    b_ins(gat + glines + 1, gbuf, glen))
		gfull = 1;
	else
		glines++;
	glen = 0;
}

static void
gappend(register char *p, register int k)
{
	register int n;

	while (glen + k > LINEMAX) {
		n = LINEMAX - glen;
		memcpy(gbuf + glen, p, n);
		glen = LINEMAX;
		gflush();
		gsplit++;
		p += n;
		k -= n;
	}
	memcpy(gbuf + glen, p, k);
	glen += k;
}

/*
 * Read a piece of what fd has, and put the lines of it into the file:
 * how much was read, and 0 or less at the end.
 */
static long	gchars;
static int	gnulls;

static int
gchunk(int fd)
{
	register char *p, *q, *z;
	char *e;
	int n, k, eol, clean;

	if ((n = read(fd, iob, IOSIZE)) <= 0)
		return (n);
	gchars += n;
	clean = nscan(iob, 0, n) == n;
	for (p = iob, e = iob + n; p < e; p = q) {
		k = nscan(p, '\n', e - p);
		q = p + k;
		eol = q < e;
		if (eol && clean && glen == 0 && k <= LINEMAX) {
			/* a whole line, as nearly all are: as it lies */
			if (graw ? t_ins(&buf, gat + glines + 1, p, k) :
			    b_ins(gat + glines + 1, p, k))
				gfull = 1;
			else
				glines++;
			q++;
			continue;
		}
		while (!clean && (z = memchr(p, 0, k)) != 0) {
			gappend(p, (int)(z - p));
			gnulls++;
			k -= z + 1 - p;
			p = z + 1;
		}
		gappend(p, k);
		if (eol) {
			q++;
			gflush();
		}
	}
	return (n);
}

/*
 * What is read is to go after line after.  With raw the file is being
 * read to be edited, and this is not a change to it that could be
 * undone.
 */
static void
gstart(long after, int raw)
{
	gat = after;
	glines = gchars = 0;
	glen = gsplit = gfull = gnulls = 0;
	graw = raw;
}

/*
 * All was read, or as much as will be, stopped being that it was
 * interrupted: close fd, and say how many lines there were, in a
 * message and to the caller.
 */
static long
gend(int fd, char *name, int stopped)
{
	int k = glen != 0;

	if (k)
		gflush();
	(void)close(fd);
	gpart = graw && (gfull || stopped);
	if (gfull)
		emsg("%s: only %ld lines of \"%.60s\" read", t_err(), glines,
		    name);
	else
		msg("\"%.60s\" %ld lines, %ld characters%s%s%s%s", name, glines,
		    gchars, k ? ", no newline at the end" : "",
		    gsplit ? ", long lines cut" : "",
		    gnulls ? ", nulls left out" : "",
		    stopped ? ", INTERRUPTED" : "");
	return (glines);
}

/*
 * Read what fd has into the file after line after, and close it.  The
 * number of lines, and a message says so.
 */
static long
getfile(int fd, long after, char *name)
{
	gstart(after, 0);
	while (!gfull && !intr && gchunk(fd) > 0)
		;
	return (gend(fd, name, intr));
}

/*
 * The file that is edited is not read all at once: enough of it to
 * fill the window, and the rest as it is wanted (loaduntil), which is
 * while nothing is typed or when a command needs what is not there
 * yet (main.c).  Until it is all read nothing can change it, so the
 * lines that are read go after those that were, and how many those are
 * is how many the file has so far.
 */
int	loading;
static int	lfd = -1;
static int	lreg;		/* it is a file: a short piece is its last */

static void
loadsome(void)
{
	char *m;
	int n;

	if (!gfull && (n = gchunk(lfd)) > 0 && !(lreg && n < IOSIZE))
		return;
	/* an error is not to be hidden by this, nor a search's message */
	m = msgattr ? strcpy(sbuf, msgbuf) : (char *)0;
	(void)gend(lfd, fname, 0);
	lfd = -1;
	loading = 0;
	if (gpart)
		readonly = 1;		/* not to be written over itself */
	else if (access(fname, W_OK) < 0) {
		readonly = 1;
		if (strlen(msgbuf) < COLMAX - 20)
			(void)strcat(msgbuf, ", read only");
	}
	if (m && !gpart)
		emsg("%s", m);
	if (buf.t_lines == 0)
		(void)t_ins(&buf, 1L, "", 0);
}

/*
 * See that the file has been read as far as line n, if it is that
 * long.  -1 if that was interrupted.
 */
int
loaduntil(long n)
{
	while (loading && buf.t_lines < n) {
		if (intr)
			return (-1);
		loadsome();
	}
	return (0);
}

/*
 * Whether the command at s is one for which the rest of the file need
 * not be read first: one that leaves it for another, or for good.
 */
static int
early(register char *s)
{
	register int n;

	for (n = 0; ALPHA(s[n]); n++)
		;
	return (is(s, n, "quit", 1) || is(s, n, "qall", 2) ||
	    is(s, n, "edit", 1) || is(s, n, "next", 1) ||
	    is(s, n, "previous", 4) || is(s, n, "Next", 1) ||
	    is(s, n, "rewind", 3));
}

/*
 * Read the file name into the file after line after: the number of
 * lines, or -1.
 */
long
readfile(char *name, long after)
{
	struct stat st;
	int fd;

	if ((fd = open(name, O_RDONLY, 0)) < 0) {
		emsg("%.100s: %s", name, strerror(errno));
		return (-1);
	}
	if (fstat(fd, &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR) {
		(void)close(fd);
		emsg("%.100s is a directory", name);
		return (-1);
	}
	return (getfile(fd, after, name));
}

/*
 * Write lines a1 to a2 to the file name.  flags: 1 to add to the end
 * of it, 2 to say nothing.
 */
int
writefile(char *name, long a1, long a2, int flags)
{
	register int len, n = 0;
	int fd;
	long i, chars = 0, lines = a2 - a1 + 1;

	if (loading) {
		emsg("\"%.100s\" is not all read yet", fname);
		return (-1);
	}
	fd = open(name, O_WRONLY | O_CREAT | (flags & 1 ? O_APPEND : O_TRUNC),
	    0666);
	if (fd < 0) {
		emsg("%.100s: %s", name, strerror(errno));
		return (-1);
	}
	if (buf.t_lines == 1 && t_len(&buf, 1L) == 0)
		lines = 0;		/* a file of no lines at all */
	for (i = a1; i < a1 + lines; i++) {
		if (n >= IOSIZE) {
			/* in whole pieces, which the system has least to
			   do for */
			if (write(fd, iob, IOSIZE) != IOSIZE)
				goto bad;
			n -= IOSIZE;
			memcpy(iob, iob + IOSIZE, n);
		}
		len = t_get(&buf, i, iob + n);
		iob[n + len] = '\n';
		n += len + 1;
		chars += len + 1;
	}
	if (n && write(fd, iob, n) != n)
		goto bad;
	if (close(fd) < 0) {
		emsg("%.100s: %s", name, strerror(errno));
		return (-1);
	}
	if (!(flags & 2))
		msg("\"%.100s\" %ld lines, %ld characters", name, lines, chars);
	return (0);
bad:
	emsg("%.100s: %s", name, strerror(errno));
	(void)close(fd);
	return (-1);
}

/*
 * Edit the file name, in place of the one being edited.
 */
int
edit(char *name)
{
	static char nm[MAXPATHLEN];
	struct stat st;
	long line = 1;
	int fd, i;

	(void)strncpy(nm, name, sizeof (nm) - 1);
	if (strcmp(nm, fname) == 0)
		line = cur.p_line;
	else {
		if (strcmp(nm, altname) == 0)
			line = altline;
		(void)strcpy(altname, fname);
		altline = cur.p_line;
		(void)strcpy(fname, nm);
	}
	if (loading) {
		/* the one that was being read is given up */
		(void)close(lfd);
		lfd = -1;
		loading = 0;
	}
	t_free(&buf);
	u_clear();
	for (i = 0; i < NMARK; i++)
		marks[i].p_line = 0;
	modified = readonly = warned = 0;
	if ((fd = open(fname, O_RDONLY, 0)) < 0) {
		if (errno == ENOENT)
			msg("\"%.100s\" is a new file", fname);
		else
			emsg("%.100s: %s", fname, strerror(errno));
	} else if (fstat(fd, &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR) {
		(void)close(fd);
		emsg("%.100s is a directory", fname);
	} else {
		gstart(0L, 1);
		lreg = (st.st_mode & S_IFMT) == S_IFREG;
		lfd = fd;
		loading = 1;
		msg("\"%.100s\"", fname);
		i = intr;
		intr = 0;
		(void)loaduntil(line + 2L * rows);
		intr = i;
	}
	if (buf.t_lines == 0)
		(void)t_ins(&buf, 1L, "", 0);
	cur.p_line = line;
	cur.p_col = 0;
	clampcur(0);
	topline = cur.p_line > s_textrows() / 2 ? cur.p_line - s_textrows() / 2 : 1;
	s_all();
	return (0);
}

/*
 * Run a command by the shell with what it writes coming here: the
 * descriptor to read that from, or -1.  What it reads is in, or the
 * terminal if that is -1.
 */
static int	cpid;

static int
pipefrom(char *cmd, int in)
{
	int pd[2];

	if (pipe(pd) < 0) {
		emsg("No pipe: %s", strerror(errno));
		return (-1);
	}
	if ((cpid = vfork()) < 0) {
		emsg("No new process: %s", strerror(errno));
		(void)close(pd[0]);
		(void)close(pd[1]);
		return (-1);
	}
	if (cpid == 0) {
		if (in >= 0) {
			(void)dup2(in, 0);
			(void)close(in);
		}
		(void)dup2(pd[1], 1);
		(void)dup2(pd[1], 2);
		(void)close(pd[0]);
		(void)close(pd[1]);
		(void)execl("/bin/sh", "sh", "-c", cmd, (char *)0);
		_exit(127);
	}
	(void)close(pd[1]);
	return (pd[0]);
}

static void
pwait(void)
{
	int w, st;

	while ((w = wait(&st)) != cpid && (w >= 0 || errno == EINTR))
		;
}

/*
 * Give the terminal to the shell for the command cmd, or if cmd is 0
 * to a shell for as long as the user wants one.
 */
int
shell(char *cmd)
{
	int pid, w, st = 0;
	char *sh;
	sig_t oi;

	if (!active)
		return (-1);
	if ((sh = getenv("SHELL")) == 0 || *sh == 0 || cmd)
		sh = "/bin/sh";
	t_cooked();
	oi = signal(SIGINT, SIG_IGN);
	if ((pid = vfork()) == 0) {
		(void)signal(SIGINT, SIG_DFL);
		(void)signal(SIGQUIT, SIG_DFL);
		if (cmd)
			(void)execl(sh, "sh", "-c", cmd, (char *)0);
		else
			(void)execl(sh, sh, (char *)0);
		_exit(127);
	}
	if (pid < 0)
		emsg("No new process: %s", strerror(errno));
	else
		while ((w = wait(&st)) != pid && (w >= 0 || errno == EINTR))
			;
	(void)signal(SIGINT, oi);
	(void)signal(SIGQUIT, SIG_IGN);
	intr = 0;
	t_raw(1);
	if (cmd && pid > 0)
		s_wait();
	t_ti();
	resized = 1;
	s_all();
	return (st);
}

/*
 * A line of a command, at *sp: 1 and *np is its number, 0 and *np is
 * the cursor's if there is none there, -1 if there is no such line.
 */
static int
address(char **sp, long *np)
{
	register char *s = skipb(*sp);
	register int c;
	long n = cur.p_line, k;
	int have = 1;
	struct pos p;
	char pat[128], *e;

	switch (*s) {
	case '.':
		s++;
		break;
	case '$':
		n = buf.t_lines;
		s++;
		break;
	case '\'':
		c = s[1];
		c = c >= 'a' && c <= 'z' ? c - 'a' : c == '\'' ? NMARK - 1 : -1;
		if (c < 0 || marks[c].p_line == 0) {
			emsg("Mark not set");
			return (-1);
		}
		n = marks[c].p_line;
		s += 2;
		break;
	case '/':
	case '?':
		c = *s++;
		for (e = pat; *s && *s != c && e < pat + sizeof (pat) - 1; ) {
			if (*s == '\\' && s[1] == c)
				s++;
			*e++ = *s++;
		}
		*e = 0;
		if (*s == c)
			s++;
		if (re_set(pat) < 0)
			return (-1);
		/* from the line after, or before */
		p.p_line = cur.p_line;
		p.p_col = c == '/' ? BIGCOL : 0;
		if (!search(c == '/' ? 1 : -1, &p))
			return (-1);
		msgbuf[0] = msgattr = 0;
		n = p.p_line;
		break;
	default:
		if (DIGIT(*s))
			for (n = 0; DIGIT(*s); s++)
				n = n * 10 + *s - '0';
		else
			have = 0;
		break;
	}
	for (;;) {
		s = skipb(s);
		if (*s != '+' && *s != '-')
			break;
		c = *s++;
		if (DIGIT(*s))
			for (k = 0; DIGIT(*s); s++)
				k = k * 10 + *s - '0';
		else
			k = 1;
		n += c == '+' ? k : -k;
		have = 1;
	}
	*sp = s;
	*np = n;
	return (have);
}

/*
 * Take what is between the c at *sp and the next c, or the end, out of
 * the line to to, which holds max; a c that has \ before it is part of
 * it, and with keep its \ too.
 */
static void
delimited(char **sp, int c, char *to, int max, int keep)
{
	register char *s = *sp, *e = to;

	while (*s && *s != c && e < to + max - 2) {
		if (*s == '\\' && s[1]) {
			if (s[1] != c || keep)
				*e++ = *s;
			s++;
		}
		*e++ = *s++;
	}
	*e = 0;
	if (*s == c)
		s++;
	*sp = s;
}

/*
 * :s/pattern/new/ and :s/pattern/new/g, on lines a1 to a2: put new
 * for the first of the pattern in each line, or for every one; :s or
 * :& alone does the last one again, with g for every one, or with &
 * as that one had it.  In new
 * & is what the pattern matched, \1 to \9 are its parts, and \n or a
 * carriage return ends a line and starts another.
 */
static void
substitute(long a1, long a2, char *arg)
{
	register char *p, *r;
	register int out;
	char pat[128];
	int c, len, from, k, did;
	long n, nsub = 0, nlines = 0, last = 0;
	char *ms, *me, *q, *prev;

	if (*arg && *arg != '&' && !ALPHA(*arg)) {
		c = *arg++;
		delimited(&arg, c, pat, sizeof (pat), 0);
		if (re_set(pat) < 0)
			return;
		delimited(&arg, c, subrep, sizeof (subrep), 1);
		subhave = 1;
		subglobal = 0;
	} else if (!subhave || !re_have) {
		emsg("No substitution to do again");
		return;
	} else if (strchr(arg, '&') == 0)
		subglobal = 0;		/* again, but not with its g */
	for (; *arg; arg++)
		if (*arg == 'g')
			subglobal = 1;
	for (n = a1; n <= a2 && !intr; n++) {
		len = t_get(&buf, n, lbuf);
		if (!re_match(lbuf, 0))
			continue;
		out = from = 0;
		prev = 0;
		do {
			ms = re_start;
			me = re_end;
			if (me == ms && ms == prev) {
				/* nothing, straight after the last: not that */
				if (from >= len)
					break;
				sbuf[out++] = lbuf[from++];
				prev = 0;
				continue;
			}
			prev = me;
			k = ms - (lbuf + from);
			if (out + k + (int)(me - ms) > sizeof (sbuf) - 200)
				goto toolong;
			memcpy(sbuf + out, lbuf + from, k);
			out += k;
			for (r = subrep; *r; r++) {
				if (out > sizeof (sbuf) - 200)
					goto toolong;
				p = q = 0;
				if (*r == '&') {
					p = ms;
					q = me;
				} else if (*r == '\\' && r[1] >= '1' && r[1] <= '9') {
					r++;
					if ((p = re_gs[*r - '0']) != 0)
						q = re_ge[*r - '0'];
				} else if (*r == '\\' && r[1] == 'n' || *r == '\r') {
					if (*r == '\\')
						r++;
					sbuf[out++] = '\n';
					continue;
				} else {
					if (*r == '\\' && r[1])
						r++;
					sbuf[out++] = *r;
					continue;
				}
				if (p == 0 || q == 0)
					continue;
				if (out + (int)(q - p) > sizeof (sbuf) - 200)
					goto toolong;
				memcpy(sbuf + out, p, (int)(q - p));
				out += q - p;
			}
			nsub++;
			from = me - lbuf;
			if (me == ms) {
				/* nothing was matched: on by a character */
				if (from < len)
					sbuf[out++] = lbuf[from];
				from++;
			}
		} while (subglobal && from <= len && re_match(lbuf, from));
		if (from < len) {
			if (out + len - from > sizeof (sbuf) - 2)
				goto toolong;
			memcpy(sbuf + out, lbuf + from, len - from);
			out += len - from;
		}
		sbuf[out] = '\n';
		for (p = sbuf, did = 0; p <= sbuf + out; p = q + 1, did++) {
			q = memchr(p, '\n', sbuf + out + 1 - p);
			if (did == 0 ? b_put(n, p, (int)(q - p)) :
			    b_ins(++n, p, (int)(q - p)))
				return;
			if (did)
				a2++;
		}
		nlines++;
		last = n;
	}
	if (intr)
		emsg("Interrupted");
	else if (nsub == 0) {
		if (!inglobal)
			emsg("Pattern not found");
		return;
	}
	cur.p_line = last;
	cur.p_col = 0;
	if (nlines > 1 && !inglobal)
		msg("%ld substitutions on %ld lines", nsub, nlines);
	return;
toolong:
	emsg("Line %ld would be too long", n);
}

/*
 * :g/pattern/command, or with not :v: do the command for each of the
 * lines a1 to a2 that has the pattern, or that has not.  They are
 * marked first, and then looked for, since the command may move them.
 */
static void
global(long a1, long a2, char *arg, int not)
{
	char pat[128];
	long n, was, found;
	int c;

	if (inglobal) {
		emsg("Not from inside itself");
		return;
	}
	if (*arg == 0 || ALPHA(*arg) || DIGIT(*arg)) {
		emsg("A pattern is needed");
		return;
	}
	c = *arg++;
	delimited(&arg, c, pat, sizeof (pat), 0);
	if (re_set(pat) < 0)
		return;
	arg = skipb(arg);
	if (*arg == 0)
		arg = "p";
	for (n = a1, found = 0; n <= a2 && !intr; n++) {
		(void)t_get(&buf, n, lbuf);
		if (re_match(lbuf, 0) != not) {
			t_setmark(&buf, n, 1);
			found++;
		}
	}
	if (found == 0) {
		emsg("Pattern not found");
		return;
	}
	inglobal = 1;
	msgbuf[0] = msgattr = 0;
	msgattr = 0;
	do {
		found = 0;
		for (n = 1; n <= buf.t_lines; n++) {
			if (!t_ismark(&buf, n))
				continue;
			t_setmark(&buf, n, 0);
			if (intr || msgattr)
				continue;	/* only the marks, now */
			found = 1;
			cur.p_line = n;
			cur.p_col = 0;
			was = buf.t_lines;
			ex(arg);
			/*
			 * Look again at where it was: if it has gone
			 * from there, the next has taken its place.
			 */
			n -= buf.t_lines < was ? was - buf.t_lines + 1 : 1;
			if (n < 0)
				n = 0;
		}
	} while (found);
	inglobal = 0;
	if (intr || msgattr)
		for (n = 1; n <= buf.t_lines; n++)
			t_setmark(&buf, n, 0);
	if (intr)
		emsg("Interrupted");
	clampcur(0);
}

/*
 * :set, with what is to be set.
 */
static int	o_cache;

static struct opt {
	char	*o_name, *o_short;
	int	*o_val;
	int	o_num;		/* it is a number; 2, the cache's size */
} opts[] = {
	{ "autoindent",	"ai",	&o_ai,		0 },
	{ "cache",	"cache", &o_cache,	2 },
	{ "expandtab",	"et",	&o_et,		0 },
	{ "ignorecase",	"ic",	&o_ic,		0 },
	{ "list",	"list",	&o_list,	0 },
	{ "number",	"nu",	&o_number,	0 },
	{ "shiftwidth",	"sw",	&o_sw,		1 },
	{ "tabstop",	"ts",	&o_ts,		1 },
	{ "wrapscan",	"ws",	&o_ws,		0 },
	{ 0 }
};

static void
set(register char *s)
{
	register struct opt *o;
	register char *e;
	char *v;
	int no, n;

	o_cache = t_cache(-1);
	if (*s == 0 || strcmp(s, "all") == 0) {
		sbuf[0] = 0;
		for (o = opts; o->o_name; o++)
			if (o->o_num)
				(void)sprintf(sbuf + strlen(sbuf), "%s=%d  ",
				    o->o_short, *o->o_val);
			else
				(void)sprintf(sbuf + strlen(sbuf), "%s%s  ",
				    *o->o_val ? "" : "no", o->o_short);
		msg("%s", sbuf);
		return;
	}
	while (*s) {
		for (e = s; *e && !BLANK(*e); e++)
			;
		if (*e)
			*e++ = 0;
		if ((v = strchr(s, '=')) != 0)
			*v++ = 0;
		no = 0;
		for (o = opts; o->o_name; o++)
			if (strcmp(s, o->o_name) == 0 || strcmp(s, o->o_short) == 0)
				break;
		if (o->o_name == 0 && s[0] == 'n' && s[1] == 'o') {
			no = 1;
			for (o = opts; o->o_name; o++)
				if (strcmp(s + 2, o->o_name) == 0 ||
				    strcmp(s + 2, o->o_short) == 0)
					break;
		}
		if (o->o_name == 0 || no && (o->o_num || v)) {
			emsg("There is no such thing to set: %.40s", s);
			return;
		}
		if (!o->o_num) {
			if (v) {
				emsg("%s is set or not, and has no value", o->o_name);
				return;
			}
			*o->o_val = !no;
		} else if (v == 0)
			msg("%s=%d", o->o_name, *o->o_val);
		else if (o->o_num == 2) {
			if ((n = atoi(v)) < 32 || n > 960) {
				emsg("%s is from 32 to 960, in K", o->o_name);
				return;
			}
			o_cache = t_cache(n);
		} else if ((n = atoi(v)) < 1 || n > 40) {
			emsg("%s is from 1 to 40", o->o_name);
			return;
		} else
			*o->o_val = n;
		s = skipb(e);
	}
	s_from(1L);
}

static void
doquit(int force)
{
	if (!force && modified) {
		emsg("Not written since it was changed (q! to quit all the same)");
		return;
	}
	if (!force && !warned && filen + 1 < nfiles) {
		warned = 1;
		emsg("%d more files to edit", nfiles - filen - 1);
		return;
	}
	quit(0);
}

/*
 * Go to another of the files named when ve was started.
 */
static void
gofile(int n, int force)
{
	if (n < 0 || n >= nfiles) {
		emsg(n < 0 ? "No file before this one" : "No more files");
		return;
	}
	if (!force && modified) {
		emsg("Not written since it was changed (! to go all the same)");
		return;
	}
	filen = n;
	(void)edit(files[n]);
}

/*
 * Do the commands in a file, a line each; with quiet it is nothing
 * if there is no such file.
 */
static void
source(char *name, int quiet)
{
	static char sb[1024];
	register char *p, *q;
	int fd, n;

	if ((fd = open(name, O_RDONLY, 0)) < 0) {
		if (!quiet)
			emsg("%.100s: %s", name, strerror(errno));
		return;
	}
	n = read(fd, sb, sizeof (sb) - 1);
	(void)close(fd);
	if (n < 0)
		n = 0;
	sb[n] = 0;
	for (p = sb; *p && !msgattr; p = q) {
		if ((q = strchr(p, '\n')) != 0)
			*q++ = 0;
		else
			q = p + strlen(p);
		ex(p);
	}
}

/*
 * The file in the user's home directory that sets things as that user
 * wants them.
 */
void
exinit(void)
{
	char *home;

	if ((home = getenv("HOME")) != 0 && strlen(home) < MAXPATHLEN) {
		(void)sprintf(xb, "%s/.verc", home);
		source(xb, 1);
	}
}

/*
 * The lines a command is for must be in the file, and in order; with
 * zero the first may be the place before the first line.
 */
static int
inrange(long a1, long a2, int zero)
{
	if (a1 < (zero ? 0 : 1) || a2 > buf.t_lines || a1 > a2) {
		emsg(a1 > a2 && a2 >= 0 ? "The lines are the wrong way round" :
		    "There is no such line");
		return (0);
	}
	return (1);
}

/*
 * A command.
 */
void
ex(char *line)
{
	char cbuf[256];
	char *s, *arg, *name;
	long a1, a2, d, n, i;
	int naddr = 0, r, force = 0, nlen, c, len;
	struct pos a, b;
	struct text t;

	(void)strncpy(cbuf, line, sizeof (cbuf) - 1);
	cbuf[sizeof (cbuf) - 1] = 0;
	s = skipb(cbuf);
	while (*s == ':')
		s = skipb(s + 1);
	if (*s == 0 || *s == '"')
		return;
	if (loading && !early(s) && waitload() < 0)
		return;
	a1 = a2 = cur.p_line;
	if (*s == '%') {
		a1 = 1;
		a2 = buf.t_lines;
		naddr = 2;
		s++;
	} else {
		if ((r = address(&s, &a1)) < 0)
			return;
		if (r)
			naddr = 1;
		a2 = a1;
		s = skipb(s);
		if (*s == ',' || *s == ';') {
			s++;
			if (address(&s, &a2) < 0)
				return;
			naddr = 2;
		}
	}
	name = s = skipb(s);
	if (ALPHA(*s))
		while (ALPHA(*s))
			s++;
	else if (*s)
		s++;
	nlen = s - name;
	if (nlen > 1 && name[0] == 'k' || nlen > 1 && name[0] == 's' &&
	    !is(name, nlen, "set", 2) && !is(name, nlen, "shell", 2) &&
	    !is(name, nlen, "source", 2) && !is(name, nlen, "substitute", 2)) {
		/* ka, and sg for s with its g */
		s = name + 1;
		nlen = 1;
	}
	if (*s == '!' && name[0] != '!') {
		force = 1;
		s++;
	}
	arg = skipb(s);
	c = name[0];

	if (nlen == 0) {
		/* only a line: go there */
		if (a2 < 1)
			a2 = 1;
		setpcmark();
		cur.p_line = a2 > buf.t_lines ? buf.t_lines : a2;
		cur.p_col = firstnb(cur.p_line);
	} else if (is(name, nlen, "write", 1) || is(name, nlen, "wq", 2) ||
	    is(name, nlen, "wall", 2) || is(name, nlen, "wqall", 3) ||
	    is(name, nlen, "xit", 1) || is(name, nlen, "xall", 2) ||
	    is(name, nlen, "exit", 3)) {
		r = 0;
		if (arg[0] == '>' && arg[1] == '>') {
			r = 1;
			arg = skipb(arg + 2);
		}
		if ((arg = expand(arg)) == 0)
			return;
		if (*arg == 0)
			arg = fname;
		if (naddr == 0) {
			a1 = 1;
			a2 = buf.t_lines;
		} else if (!inrange(a1, a2, 0))
			return;
		if (c != 'w' && !modified && *fname)
			;		/* x: nothing to write */
		else if (*arg == 0) {
			emsg("The file has no name");
			return;
		} else {
			if (!force && strcmp(arg, fname) == 0 && readonly) {
				emsg("The file is read only (w! to write it all the same)");
				return;
			}
			if (!force && !r && strcmp(arg, fname) != 0 &&
			    access(arg, F_OK) == 0) {
				emsg("There is such a file (w! to write over it)");
				return;
			}
			if (writefile(arg, a1, a2, r) < 0)
				return;
			if (naddr == 0 && !r) {
				if (*fname == 0)
					(void)strcpy(fname, arg);
				if (strcmp(arg, fname) == 0)
					modified = 0;
			}
		}
		if (c != 'w' || nlen > 1 && name[1] == 'q')
			doquit(force);
	} else if (is(name, nlen, "quit", 1) || is(name, nlen, "qall", 2))
		doquit(force);
	else if (is(name, nlen, "edit", 1)) {
		if ((arg = expand(arg)) == 0)
			return;
		if (*arg == 0 && *fname == 0) {
			emsg("The file has no name");
			return;
		}
		if (!force && modified) {
			emsg("Not written since it was changed (e! to lose that)");
			return;
		}
		(void)edit(*arg ? arg : fname);
	} else if (is(name, nlen, "read", 1)) {
		if (naddr == 0)
			a2 = cur.p_line;
		if (!inrange(a2, a2, 1))
			return;
		if (force || *arg == '!') {
			if (*arg == '!')
				arg++;
			if ((arg = expand(arg)) == 0)
				return;
			if ((r = pipefrom(arg, -1)) < 0)
				return;
			n = getfile(r, a2, arg);
			pwait();
		} else {
			if ((arg = expand(arg)) == 0)
				return;
			if (*arg == 0 && *(arg = fname) == 0) {
				emsg("The file has no name");
				return;
			}
			n = readfile(arg, a2);
		}
		if (n > 0) {
			cur.p_line = a2 + 1;
			cur.p_col = 0;
		}
	} else if (is(name, nlen, "file", 1)) {
		if ((arg = expand(arg)) == 0)
			return;
		if (*arg && strlen(arg) < MAXPATHLEN) {
			(void)strcpy(altname, fname);
			altline = cur.p_line;
			(void)strcpy(fname, arg);
			readonly = 0;
		}
		n = t_blocks(&d);
		cbuf[0] = 0;
		if (d)
			(void)sprintf(cbuf, ", %ldK in /tmp", d);
		msg("\"%.100s\"%s%s line %ld of %ld, %ldK of text%s",
		    *fname ? fname : "no name", modified ? " changed," : "",
		    readonly ? " read only," : "", cur.p_line, buf.t_lines,
		    n, cbuf);
	} else if (is(name, nlen, "next", 1))
		gofile(filen + 1, force);
	else if (is(name, nlen, "previous", 4) || is(name, nlen, "Next", 1))
		gofile(filen - 1, force);
	else if (is(name, nlen, "rewind", 3))
		gofile(0, force);
	else if (is(name, nlen, "args", 2)) {
		sbuf[0] = 0;
		for (r = 0; r < nfiles && strlen(sbuf) < COLMAX - 80; r++)
			(void)sprintf(sbuf + strlen(sbuf), r == filen ?
			    "[%.60s] " : "%.60s ", files[r]);
		msg("%s", sbuf);
	} else if (is(name, nlen, "delete", 1) || is(name, nlen, "yank", 1)) {
		if (!inrange(a1, a2, 0))
			return;
		if ((r = regof(*arg)) < 0) {
			emsg("There is no such register");
			return;
		}
		a.p_line = a1;
		b.p_line = a2;
		a.p_col = b.p_col = 0;
		if (c == 'd')
			(void)dodelete(r, &a, &b, 1);
		else
			(void)yank(r, &a, &b, 1);
	} else if (is(name, nlen, "put", 2)) {
		if (naddr == 0)
			a2 = cur.p_line;
		if (!inrange(a2, a2, 1))
			return;
		if ((r = regof(*arg)) < 0) {
			emsg("There is no such register");
			return;
		}
		/* as lines, whatever it was kept as */
		len = regs[r].r_lines;
		regs[r].r_lines = 1;
		cur.p_line = a2 ? a2 : 1;
		put(r, a2 != 0, 1L);
		regs[r].r_lines = len;
	} else if (is(name, nlen, "move", 1) || is(name, nlen, "copy", 2) ||
	    is(name, nlen, "t", 1)) {
		if (!inrange(a1, a2, 0))
			return;
		if (address(&arg, &d) <= 0 || d < 0 || d > buf.t_lines) {
			if (msgbuf[0] == 0)
				emsg("Where to?");
			return;
		}
		n = a2 - a1 + 1;
		if (c == 'm' && d >= a1 - 1 && d <= a2) {
			if (d >= a1 && d < a2)
				emsg("Lines cannot be moved into themselves");
			return;
		}
		t_init(&t);
		if (t_copy(&t, 1L, &buf, a1, n) < 0) {
			t_free(&t);
			emsg("%s", t_err());
			return;
		}
		for (i = 1; i <= n; i++) {
			len = t_get(&t, i, lbuf);
			if (b_ins(d + i, lbuf, len) < 0)
				break;
		}
		t_free(&t);
		if (i <= n) {
			/* take out what did go in */
			while (--i >= 1)
				b_del(d + 1);
			return;
		}
		cur.p_line = d + n;
		if (c == 'm') {
			for (i = 0; i < n; i++)
				b_del(d < a1 ? a1 + n : a1);
			if (d > a2)
				cur.p_line = d;
		}
		cur.p_col = 0;
	} else if (is(name, nlen, "substitute", 1) || c == '&') {
		if (inrange(a1, a2, 0))
			substitute(a1, a2, arg);
	} else if (is(name, nlen, "global", 1) || is(name, nlen, "vglobal", 1)) {
		if (naddr == 0) {
			a1 = 1;
			a2 = buf.t_lines;
		}
		if (inrange(a1, a2, 0))
			global(a1, a2, arg, c == 'v' || force);
	} else if (is(name, nlen, "set", 2))
		set(arg);
	else if (c == '!') {
		if (*arg == '!') {
			(void)sprintf(sbuf, "%.180s%.200s", lastbang, arg + 1);
			arg = sbuf;
		}
		if ((arg = expand(arg)) == 0)
			return;
		if (*arg == 0 || strlen(arg) >= sizeof (lastbang)) {
			emsg(*arg ? "Too long a command" : "What command?");
			return;
		}
		(void)strcpy(lastbang, arg);
		if (naddr == 0) {
			(void)shell(lastbang);
			return;
		}
		/*
		 * With lines: they are what the command reads, and what
		 * it writes takes their place.
		 */
		if (!inrange(a1, a2, 0))
			return;
		(void)sprintf(sbuf, "/tmp/ve%05d", getpid());
		if (writefile(sbuf, a1, a2, 2) < 0)
			return;
		r = open(sbuf, O_RDONLY, 0);
		(void)unlink(sbuf);
		if (r < 0 || (c = pipefrom(lastbang, r)) < 0) {
			if (r >= 0)
				(void)close(r);
			return;
		}
		(void)close(r);
		n = getfile(c, a2, lastbang);
		pwait();
		if (msgattr)
			return;
		a.p_line = a1;
		b.p_line = a2;
		a.p_col = b.p_col = 0;
		(void)dodelete(0, &a, &b, 1);
		cur.p_line = a1;
		clampcur(0);
		msg("%ld lines for %ld", n, a2 - a1 + 1);
	} else if (is(name, nlen, "shell", 2))
		(void)shell((char *)0);
	else if (c == '=')
		msg("%ld", naddr ? a2 : buf.t_lines);
	else if (is(name, nlen, "print", 1) || is(name, nlen, "Print", 1) ||
	    is(name, nlen, "number", 2) || c == '#') {
		if (!inrange(a1, a2, 0))
			return;
		for (n = a1; n <= a2 && !liststop; n++)
			if (c == 'p' || c == 'P') {
				(void)t_get(&buf, n, lbuf);
				list(lbuf);
			} else {
				(void)sprintf(sbuf, "%6ld  ", n);
				(void)t_get(&buf, n, sbuf + 8);
				list(sbuf);
			}
		cur.p_line = a2;
	} else if (is(name, nlen, "undo", 1))
		undo();
	else if (is(name, nlen, "redo", 3))
		redo_change();
	else if (is(name, nlen, "join", 1)) {
		if (naddr < 2)
			a2 = a1 + 1;
		if (inrange(a1, a2, 0))
			(void)join(a1, a2 - a1 + 1);
	} else if (c == '>' || c == '<') {
		for (r = 1; *arg == c; arg++)
			r++;
		if (inrange(a1, a2, 0)) {
			shift(a1, a2, c == '>' ? r : -r);
			cur.p_line = a2;
			cur.p_col = firstnb(a2);
		}
	} else if (c == 'k' || is(name, nlen, "mark", 2)) {
		if (*arg < 'a' || *arg > 'z' || !inrange(a2, a2, 0)) {
			if (msgbuf[0] == 0)
				emsg("A mark is a letter, a to z");
			return;
		}
		marks[*arg - 'a'].p_line = a2;
		marks[*arg - 'a'].p_col = 0;
	} else if (is(name, nlen, "source", 2)) {
		if ((arg = expand(arg)) != 0)
			source(arg, 0);
	} else if (is(name, nlen, "cd", 2) || is(name, nlen, "chdir", 3)) {
		if (*arg == 0 && (arg = getenv("HOME")) == 0)
			arg = "/";
		if (chdir(arg) < 0)
			emsg("%.100s: %s", arg, strerror(errno));
	} else if (is(name, nlen, "help", 1))
		(void)shell("man ve");
	else if (is(name, nlen, "nohlsearch", 3))
		;
	else
		emsg("There is no such command: %.40s", name);
	clampcur(0);
	if (listing && !inglobal) {
		listing = liststop = 0;
		s_wait();
	}
}
