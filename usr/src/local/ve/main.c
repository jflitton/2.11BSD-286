/*
 * ve [-R] [+line] [file ...]: a screen editor in the manner of vim.
 *
 * This port's own (docs/userland.md).  It is for a terminal of any
 * size and a file of any length to speak of: the text is kept outside
 * the program's data segment, some of it in memory and the rest in a
 * file (text.c).  What it has of
 * vim is the part most used: the modes and the commands of normal mode
 * with their counts, operators, motions and text objects; the visual
 * modes; registers; undo and redo without end to speak of; and the
 * commands after a colon.  ve(1) says which.
 */
#include <sys/param.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include "ve.h"

struct text buf;
struct pos cur = { 1, 0 };
int	wantcol;
int	mode;
struct pos vstart;
char	fname[MAXPATHLEN];
char	altname[MAXPATHLEN];
long	altline;
int	modified;
int	readonly;
char	lbuf[LINEMAX + 2];
char	lbuf2[LINEMAX + 2];
struct pos marks[NMARK];
struct reg regs[NREG];
int	o_number, o_ic, o_ai, o_ts = 8, o_sw = 8, o_et, o_ws = 1, o_list;
char	**files;
int	nfiles, filen;

int	gone;
int	active;
static char	mbuf[LINEMAX + 200];

/*
 * Something to say on the last row, until the next key.
 */
void
msg(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void)vsprintf(mbuf, fmt, ap);
	va_end(ap);
	mbuf[COLMAX] = 0;
	(void)strcpy(msgbuf, mbuf);
	msgattr = 0;
}

/*
 * The same when it is that something could not be done.
 */
void
emsg(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void)vsprintf(mbuf, fmt, ap);
	va_end(ap);
	mbuf[COLMAX] = 0;
	(void)strcpy(msgbuf, mbuf);
	msgattr = 1;
}

/*
 * Keep the cursor in the file and in its line: with ins it may be
 * after the last character, where the next would be put in.
 */
void
clampcur(int ins)
{
	register int len;

	if (cur.p_line > buf.t_lines)
		cur.p_line = buf.t_lines;
	if (cur.p_line < 1)
		cur.p_line = 1;
	len = t_len(&buf, cur.p_line);
	if (!ins && len > 0)
		len--;
	if (cur.p_col > len)
		cur.p_col = len;
	if (cur.p_col < 0)
		cur.p_col = 0;
}

/*
 * The end.  If it is not of the user's asking and the file has changes
 * that are nowhere else, they are written to a file beside it.
 */
void
quit(int how)
{
	static char save[MAXPATHLEN + 8];

	(void)signal(SIGHUP, SIG_IGN);
	(void)signal(SIGTERM, SIG_IGN);
	if (how < 0 && modified) {
		(void)sprintf(save, "%s.save", *fname ? fname : "ve");
		(void)writefile(save, 1L, buf.t_lines, 2);
	}
	t_close();
	exit(how < 0 ? 1 : how);
}

/*
 * The end when the text cannot be had: there is nothing to save.
 */
void
fatal(char *why)
{
	(void)signal(SIGHUP, SIG_IGN);
	(void)signal(SIGTERM, SIG_IGN);
	if (active)
		t_close();
	(void)fprintf(stderr, "ve: %s\n", why);
	exit(1);
}

/*
 * Read the rest of the file, which a command needs: it may take a
 * while, so say what the wait is for.  -1 if it was interrupted, and
 * then the command is not to be done.
 */
int
waitload(void)
{
	if (!loading)
		return (0);
	if (active) {
		msg("Reading \"%.100s\"", fname);
		s_refresh();
		t_flush();
		msgbuf[0] = 0;
	}
	if (loaduntil(ALL) < 0) {
		emsg("Interrupted");
		return (-1);
	}
	return (0);
}

/*
 * Whether k begins a command that can be done before the file is all
 * read: one that moves over what is there and changes nothing, or
 * that sees for itself to what it needs (search, ex).
 */
static int
early(register int k)
{
	switch (k) {
	case 'h': case 'j': case 'k': case 'l': case ' ':
	case CTL('H'): case 0177:
	case K_UP: case K_DOWN: case K_LEFT: case K_RIGHT:
	case K_HOME: case K_END: case K_PGUP: case K_PGDN:
	case '0': case '^': case '$': case '+': case '-': case '\r':
	case '\n':
	case 'w': case 'b': case 'e': case 'W': case 'B': case 'E':
	case 'f': case 'F': case 't': case 'T': case ';': case ',':
	case 'H': case 'M': case 'L': case 'z':
	case CTL('F'): case CTL('B'): case CTL('D'): case CTL('U'):
	case CTL('E'): case CTL('Y'): case CTL('L'): case CTL('Z'):
	case '/': case '?': case 'n': case 'N': case '*': case '#':
	case ':': case 'Z': case ESC: case CTL('C'):
		return (1);
	}
	return (0);
}

static void
onhup(int sig)
{
	gone = 1;
}

static void
onintr(int sig)
{
	intr = 1;
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: ve [-R] [+line] [file ...]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	register int k;
	long line = 0;
	int i;

	for (argv++, argc--; argc > 0; argv++, argc--) {
		if (argv[0][0] == '+') {
			line = argv[0][1] ? atol(argv[0] + 1) : -1;
			continue;
		}
		if (argv[0][0] != '-')
			break;
		if (strcmp(argv[0], "--") == 0) {
			argv++, argc--;
			break;
		}
		if (strcmp(argv[0], "-R") == 0)
			readonly = 1;
		else
			usage();
	}
	files = argv;
	nfiles = argc;
	t_open();
	t_init(&buf);
	for (i = 0; i < NREG; i++)
		t_init(&regs[i].r_text);
	(void)t_ins(&buf, 1L, "", 0);
	exinit();
	if (nfiles) {
		i = readonly;
		(void)edit(files[0]);
		readonly |= i;
	}
	if (line) {
		(void)loaduntil(line < 0 ? ALL : line + 2L * rows);
		cur.p_line = line < 0 ? buf.t_lines : line;
		clampcur(0);
	}
	(void)signal(SIGHUP, onhup);
	(void)signal(SIGTERM, onhup);
	(void)signal(SIGINT, onintr);
	(void)siginterrupt(SIGHUP, 1);
	(void)siginterrupt(SIGTERM, 1);
	(void)siginterrupt(SIGINT, 1);
	(void)signal(SIGQUIT, SIG_IGN);
	t_raw(0);
	active = 1;
	s_init();
	for (;;) {
		if (gone)
			quit(-1);
		if (loading) {
			/* what the window may show, and as much again */
			intr = 0;
			(void)loaduntil(cur.p_line + 2L * rows);
		}
		if (!stuffed())
			s_refresh();
		/* the rest of the file, while nothing is typed */
		while (loading && !t_pending()) {
			intr = 0;
			(void)loaduntil(buf.t_lines + 1);
			if (!loading)
				s_refresh();
		}
		u_begin();
		rec_start();
		i = stuffed();
		k = getkey();
		if (k != K_NONE && loading && !early(k) && waitload() < 0)
			k = K_NONE;
		if (k != K_NONE) {
			msgbuf[0] = msgattr = 0;
			changed = 0;
			intr = 0;
			normal(k);
			/* what is left of a command done again is no command */
			if (i && stuffed())
				stuff((short *)0, 0);
		}
		rec_end(changed);
		u_end();
	}
}
