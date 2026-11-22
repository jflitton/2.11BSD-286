/*
 * mkstr - take a program's error messages out of its C, as mkstr(1)
 * does, for the programs of this system that are built so: ex and
 * tcsh (Makefile).
 *
 * usage: mkstr [-e name[=new]]... [-n n] strings outdir file ...
 *
 * Each file is copied to outdir, under its own name, with every string
 * that is the first argument of a call to error() (or of a call to
 * name(), for each -e) replaced by its offset in strings, which gets
 * the message.  The program reads a message from the file when it has
 * one to print, and has only the number in its code and none of the
 * text in its data: ex keeps its messages in /usr/share/misc/exstrings
 * this way (ucb/mkstr).  As mkstr does, a name matches at its end, so
 * that "error(" is also serror( and cerror(, and a string must follow
 * the parenthesis directly.
 *
 * tcsh's mkprstr (bin/tcsh) is a mkstr of its own for its xprintf(),
 * and the other two options do what it does differently: with name=new
 * the call becomes one to new(), which reads the string from the file
 * and prints with it; with -n a string written in fewer than n
 * characters is left where it is, as costing less than the number.
 *
 * In strings each message is followed by a NUL and a newline, and a
 * message that is there already is used again.  It begins with an
 * empty one, so that no message has the offset 0, which a program
 * takes to mean none: ex's error(0).
 *
 * This is one program where upstream has two because it is run both
 * where the system is built, compiled for that machine, and on the
 * system, when it makes itself (docs/self-hosting.md): what it writes
 * is the same in both, to the byte.
 */
#include <stdio.h>

#define	NNAMES	8		/* -e */
#define	NAMELEN	32
#define	STRMAX	24000		/* bytes of messages */
#define	NMSG	1500		/* messages */
#define	MSGMAX	1024		/* one message */

struct name {
	char	n_name[NAMELEN];	/* with its parenthesis */
	char	n_new[NAMELEN];
	int	n_len;
} names[NNAMES];
int	nnames;
int	shortest;

char	strs[STRMAX];		/* the file of messages */
unsigned nstrs;
unsigned msgoff[NMSG];		/* where each message is in it */
int	msglen[NMSG];
int	nmsg;

char	msg[MSGMAX];		/* the string being read, and its length */
int	mlen;

char	pend[NAMELEN];		/* what was read last and is not written */
int	npend;

extern char *strcpy(), *strcat(), *rindex();

fatal(s, t)
	char *s, *t;
{
	fprintf(stderr, "mkstr: ");
	fprintf(stderr, s, t);
	fprintf(stderr, "\n");
	exit(1);
}

addname(s)
	char *s;
{
	register struct name *np;
	register char *p;

	if (nnames >= NNAMES || strlen(s) > NAMELEN - 2)
		fatal("%s: too many names, or too long a one", s);
	np = &names[nnames++];
	for (p = s; *p && *p != '='; p++)
		;
	strncpy(np->n_name, s, p - s);
	np->n_name[p - s] = 0;
	strcpy(np->n_new, *p ? p + 1 : np->n_name);
	strcat(np->n_name, "(");
	strcat(np->n_new, "(");
	np->n_len = strlen(np->n_name);
}

/*
 * The name that what was read last ends with, if it does.
 */
struct name *
match()
{
	register struct name *np;

	for (np = names; np < &names[nnames]; np++)
		if (npend >= np->n_len && strncmp(pend + npend - np->n_len,
		    np->n_name, np->n_len) == 0)
			return (np);
	return (0);
}

/*
 * Read the string whose text begins here, as mkstr reads one, into
 * msg: a backslash and newline join lines, and a newline inside it is
 * kept.  How many characters it was written in.
 */
long
string(in, file)
	register FILE *in;
	char *file;
{
	register int c, n;
	int i;
	long raw;

	mlen = 0;
	raw = 0;
	for (;;) {
		if ((c = getc(in)) == EOF)
			fatal("%s: a string does not end", file);
		raw++;
		if (c == '"')
			return (raw - 1);
		if (c == '\\') {
			if ((c = getc(in)) == EOF)
				fatal("%s: a string does not end", file);
			raw++;
			if (c == '\n')
				continue;
			if (c >= '0' && c <= '7') {
				n = c - '0';
				for (i = 0; i < 2; i++) {
					c = getc(in);
					if (c < '0' || c > '7') {
						if (c != EOF)
							ungetc(c, in);
						break;
					}
					raw++;
					n = n * 8 + c - '0';
				}
				c = n;
			} else switch (c) {
			case 'a': c = '\007'; break;
			case 'b': c = '\b'; break;
			case 't': c = '\t'; break;
			case 'r': c = '\r'; break;
			case 'n': c = '\n'; break;
			case 'f': c = '\f'; break;
			case 'v': c = '\013'; break;
			}
		}
		if (mlen >= MSGMAX)
			fatal("%s: too long a string", file);
		msg[mlen++] = c;
	}
}

/*
 * Where the message is in the file of them, to which it is added if
 * it is not there.
 */
unsigned
offset()
{
	register int i;

	for (i = 0; i < nmsg; i++)
		if (msglen[i] == mlen && bcmp(strs + msgoff[i], msg, mlen) == 0)
			return (msgoff[i]);
	if (nmsg >= NMSG || nstrs + mlen + 2 > STRMAX)
		fatal("too many messages", (char *)0);
	msgoff[nmsg] = nstrs;
	msglen[nmsg++] = mlen;
	bcopy(msg, strs + nstrs, mlen);
	nstrs += mlen;
	strs[nstrs++] = 0;
	strs[nstrs++] = '\n';
	return (msgoff[nmsg - 1]);
}

copy(file, outdir)
	char *file, *outdir;
{
	register FILE *in, *out;
	register int c;
	register struct name *np;
	char name[256], *base;
	long pos;
	int i;

	if ((in = fopen(file, "r")) == NULL)
		fatal("cannot read %s", file);
	base = rindex(file, '/');
	base = base ? base + 1 : file;
	if (strlen(outdir) + strlen(base) + 2 > sizeof name)
		fatal("%s: too long a name", file);
	strcpy(name, outdir);
	strcat(name, "/");
	strcat(name, base);
	if ((out = fopen(name, "w")) == NULL)
		fatal("cannot write %s", name);
	npend = 0;
	while ((c = getc(in)) != EOF) {
		if (c == '"' && (np = match()) != 0) {
			pos = ftell(in);
			if (string(in, file) >= shortest) {
				npend -= np->n_len;
				fwrite(pend, 1, npend, out);
				npend = 0;
				fprintf(out, "%s%u", np->n_new, offset());
				continue;
			}
			fseek(in, pos, 0);
		}
		if (npend == sizeof pend) {
			putc(pend[0], out);
			for (i = 1; i < npend; i++)
				pend[i - 1] = pend[i];
			npend--;
		}
		pend[npend++] = c;
	}
	fwrite(pend, 1, npend, out);
	if (ferror(out) || fclose(out) == EOF)
		fatal("cannot write %s", name);
	fclose(in);
}

main(argc, argv)
	int argc;
	char **argv;
{
	register FILE *f;
	register int i;

	argc--, argv++;
	while (argc > 1 && argv[0][0] == '-' &&
	    (argv[0][1] == 'e' || argv[0][1] == 'n') && argv[0][2] == 0) {
		if (argv[0][1] == 'n')
			shortest = atoi(argv[1]);
		else
			addname(argv[1]);
		argc -= 2, argv += 2;
	}
	if (argc < 3) {
		fprintf(stderr, "usage: mkstr [-e name[=new]]... [-n n] strings outdir file ...\n");
		exit(1);
	}
	if (nnames == 0)
		addname("error");
	strs[nstrs++] = 0;
	strs[nstrs++] = '\n';
	for (i = 2; i < argc; i++)
		copy(argv[i], argv[1]);
	if ((f = fopen(argv[0], "w")) == NULL)
		fatal("cannot write %s", argv[0]);
	fwrite(strs, 1, nstrs, f);
	if (ferror(f) || fclose(f) == EOF)
		fatal("cannot write %s", argv[0]);
	exit(0);
}
