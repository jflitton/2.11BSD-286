#if !defined(lint) && defined(DOSCCS)
static	char sccsid[] = "@(#)cc.c 5.3 (2.11BSD) 2026/11/26";
#endif

/*
 * cc - front end for C compiler
 *
 * On the 286 (#ifdef i286) a program is of one of three models, the
 * small, which is what there is without a flag, with -mm the medium,
 * whose functions are called far, or with -ml the large, whose
 * pointers are all far as well (docs/memory-model.md).  The
 * compiler's passes, the assembler and the loader are each told which,
 * and the start-up code and the libraries are the model's: /lib and
 * /usr/lib have the small model's, /lib/m and /usr/lib/m the medium's
 * and /lib/l and /usr/lib/l the large's, where the loader looks for
 * them.  The second pass is told the name of the segment a file's
 * functions are to be in, which is the file's.  The assembler has no
 * -V and -u: every program has its text apart from its data, and a
 * name that is not defined has to be said to be external, which the
 * second pass does.  A file of assembly is given FAR defined in the
 * medium and large models, and FARDATA too in the large, as the C
 * library's own are (lib/libc/i286/crt/DEFS.inc), which is in
 * /usr/include, where the assembler is told to look for what such a
 * file includes, after the directories of -I.  In the large model
 * the preprocessor is given __LARGE__ defined, as the headers know the
 * model by, and NULL as 0L, which is four bytes, as a pointer is, where
 * it is passed to a function that has no prototype; in the medium,
 * __MEDIUM__.
 *
 * Profiling, -p, is the small model's alone: the system's record of
 * where a program spends its time is of IP (profil(2)), which does not
 * say which segment of code it was in.  The start-up code is then
 * /lib/mcrt0.o, whether -p comes before -ms or after, and the library
 * /usr/lib/libc_p.a, as on the PDP-11.
 *
 * c2 has a function in memory whole, and one too long for its data
 * segment it says it has no space for: the file is then given to
 * /lib/c2l, the same program of the large model, which is slower and
 * has the memory.
 *
 * With -nd=NAME a file's data is not in the program's data segment,
 * which is the stack's, but in one of its own, NAME_DATA and NAME_BSS
 * of the group NAME_GROUP: the first pass is told that SS is not the
 * data segment (-S), and the second the name (-NNAME).  The kernel's
 * network is made so (sys/PC/Makefile).
 */
#include <sys/param.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/dir.h>
#include <time.h>

char	*cpp = "/lib/cpp";
char	*ccom = "/lib/c0";
char	*ccom1 = "/lib/c1";
char	*c2 = "/lib/c2";
#ifdef i286
char	*c2big = "/lib/c2l";	/* c2 of the large model */
#endif
char	*as = "/bin/as";
char	*ld = "/bin/ld";
char	*crt0 = "/lib/crt0.o";
#ifdef i286
int	medium;			/* -mm, or -ml */
int	large;			/* -ml */
char	*ndata;			/* -nd=NAME: -NNAME, for c1 */
char	*textseg();
#endif

char	tmp0[30];		/* big enough for /tmp/ctm%05.5d */
char	*tmp1, *tmp2, *tmp3, *tmp4, *tmp5;
char	*outfile;
char	*savestr(), *strspl(), *setsuf();
void	idexit();
char	**av, **clist, **llist, **plist;
int	cflag, eflag, oflag, pflag, sflag, wflag, exflag, proflag;
int	Mflag, debug;
int	exfail;
int	quiet;			/* callsys: the program's errors to /dev/null */
char	*chpass;
char	*npassname;

int	nc, nl, np, nxo, na;

#define	cunlink(s)	if (s) unlink(s)

main(argc, argv)
	char **argv;
{
	time_t tid;
	char *t, *ctbuf;
	char *assource;
	int i, j, c, c2fail;

	/* ld currently adds upto 5 args; 10 is room to spare */
	av = (char **)calloc(argc+10, sizeof (char **));
	clist = (char **)calloc(argc, sizeof (char **));
	llist = (char **)calloc(argc, sizeof (char **));
	plist = (char **)calloc(argc+4, sizeof (char **));
	for (i = 1; i < argc; i++) {
		if (*argv[i] == '-') switch (argv[i][1]) {

		case 'S':
			sflag++;
			cflag++;
			continue;
		case 'o':
			if (++i < argc) {
				outfile = argv[i];
				switch (getsuf(outfile)) {

				case 'c':
					error("-o would overwrite %s",
					    outfile);
					exit(8);
				}
			}
			continue;
		case 'O':
			oflag++;
			continue;
		case 'p':
			proflag++;
			crt0 = "/lib/mcrt0.o";
			if (argv[i][2] == 'g')
				crt0 = "/usr/lib/gcrt0.o";
			continue;
		case 'w':
			wflag++;
			continue;
		case 'E':
			exflag++;
		case 'P':
			pflag++;
			plist[np++] = argv[i];
		case 'c':
			cflag++;
			continue;
		case 'M':
			exflag++;
			pflag++;
			Mflag++;
			/* and fall through */
		case 'D':
		case 'I':
		case 'U':
		case 'C':
			plist[np++] = argv[i];
			continue;
		case 'L':
			llist[nl++] = argv[i];
			continue;
		case 't':
			if (chpass)
				error("-t overwrites earlier option", (char *)0);
			chpass = argv[i]+2;
			if (chpass[0]==0)
				chpass = "012p";
			continue;
		case 'B':
			if (npassname)
				error("-B overwrites earlier option", (char *)0);
			npassname = argv[i]+2;
			if (npassname[0]==0)
				npassname = "/usr/c/";
			continue;
		case 'd':
			if (argv[i][2] == '\0') {
				debug++;
				continue;
			}
			continue;
#ifdef i286
		case 'm':
			if (argv[i][2] == 'm' && argv[i][3] == 0) {
				medium = 1;
				large = 0;
			} else if (argv[i][2] == 'l' && argv[i][3] == 0) {
				medium = large = 1;
			} else if (argv[i][2] == 's' && argv[i][3] == 0) {
				medium = large = 0;
			} else {
				error("%s: the models are -ms, -mm and -ml",
				    argv[i]);
				exit(8);
			}
			continue;
		case 'n':
			if (strncmp(argv[i], "-nd=", 4) == 0 && argv[i][4]) {
				ndata = strspl("-N", argv[i]+4);
				continue;
			}
			break;
#endif
		}
		t = argv[i];
		c = getsuf(t);
		if (c=='c' || c=='s' || exflag) {
			clist[nc++] = t;
			t = setsuf(t, 'o');
		}
		if (nodup(llist, t)) {
			llist[nl++] = t;
			if (getsuf(t)=='o')
				nxo++;
		}
	}
#ifdef i286
	if (proflag && medium) {
		error("-p: profiling is of the small model alone", (char *)0);
		exit(8);
	}
	if (!proflag)
		crt0 = large ? "/lib/l/crt0.o" : medium ? "/lib/m/crt0.o" :
		    "/lib/crt0.o";
#endif
	if (npassname && chpass ==0)
		chpass = "012p";
	if (chpass && npassname==0)
		npassname = "/usr/new/";
	if (chpass)
	for (t=chpass; *t; t++) {
		switch (*t) {

		case '0':
			ccom = strspl(npassname, "c0");
			continue;
		case '1':
			ccom1 = strspl(npassname, "c1");
			continue;
		case '2':
			c2 = strspl(npassname, "c2");
#ifdef i286
			c2big = strspl(npassname, "c2l");
#endif
			continue;
		case 'p':
			cpp = strspl(npassname, "cpp");
			continue;
		}
	}
	if (nc==0)
		goto nocom;
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, idexit);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, idexit);
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, idexit);
	if (pflag==0)
		sprintf(tmp0, "/tmp/ctm%05.5d", getpid());
	tmp1 = strspl(tmp0, "1");
	tmp2 = strspl(tmp0, "2");
	tmp3 = strspl(tmp0, "3");
	if (pflag==0)
		tmp4 = strspl(tmp0, "4");
	if (oflag)
		tmp5 = strspl(tmp0, "5");

	for (i=0; i<nc; i++) {
		if (nc > 1 && !Mflag) {
			printf("%s:\n", clist[i]);
			fflush(stdout);
		}
		if (!Mflag && getsuf(clist[i]) == 's') {
			assource = clist[i];
			goto assemble;
		} else
			assource = tmp3;
		if (pflag)
			tmp4 = setsuf(clist[i], 'i');
		av[0] = "cpp";
		na = 1;
#ifdef i286
		if (large) {
			av[na++] = "-D__LARGE__";
			av[na++] = "-DNULL=0L";
		} else if (medium)
			av[na++] = "-D__MEDIUM__";
#endif
		for (j = 0; j < np; j++)
			av[na++] = plist[j];
		av[na++] = clist[i];
		if (!exflag)
			av[na++] = tmp4;
		av[na++] = 0;
		if (callsys(cpp, av)) {
			exfail++;
			eflag++;
		}
		if (pflag || exfail) {
			cflag++;
			continue;
		}
		if (sflag) {
			if (nc==1 && outfile)
				tmp3 = outfile;
			else
				tmp3 = setsuf(clist[i], 's');
			assource = tmp3;
		}
		av[0] = "c0";
		av[1] = tmp4; av[2] = tmp1; av[3] = tmp2; na = 4;
		if (proflag)
			av[na++] = "-P";
		if (wflag)
			av[na++] = "-w";
#ifdef i286
		if (medium)
			av[na++] = large ? "-L" : "-M";
		if (ndata)
			av[na++] = "-S";
#endif
		av[na] = 0;
		if (callsys(ccom, av)) {
			cflag++;
			eflag++;
			continue;
		}
		av[0] = "c1"; av[1] = tmp1; av[2] = tmp2;
		av[3] = oflag ? tmp5 : tmp3;
		na = 4;
#ifdef i286
		if (medium)
			av[na++] = textseg(clist[i]);
		if (ndata)
			av[na++] = ndata;
#endif
		av[na] = 0;
		if (callsys(ccom1, av)) {
			cflag++;
			eflag++;
			continue;
		}
		if (oflag) {
			av[0] = "c2"; av[1] = tmp5; av[2] = tmp3; av[3] = 0;
#ifdef i286
			/*
			 * c2 says nothing: c2l, given the file when c2 fails,
			 * says again what c2 would have, but that it had no
			 * space for a function, which c2l has.
			 */
			quiet = 1;
			c2fail = callsys(c2, av);
			quiet = 0;
			if (c2fail && callsys(c2big, av)) {
#else
			if (callsys(c2, av)) {
#endif
				unlink(tmp3);
				tmp3 = assource = tmp5;
			} else
				unlink(tmp5);
		}
		if (sflag)
			continue;
	assemble:
		cunlink(tmp1); cunlink(tmp2); cunlink(tmp4);
#ifdef i286
		av[0] = "as";
		av[1] = large ? "-ml" : medium ? "-mm" : "-ms";
		av[2] = medium ? "-DFAR" : "-DNEAR";
		na = 3;
		if (large)
			av[na++] = "-DFARDATA";
		if (assource == clist[i]) {
			/* DEFS.inc, and what the file includes of its own */
			av[na++] = "-I/usr/include";
			for (j = 0; j < np; j++)
				if (plist[j][1] == 'I')
					av[na++] = plist[j];
		}
#else
		av[0] = "as"; av[1] = "-V";
		av[2] = "-u"; na = 3;
#endif
		av[na++] = "-o";
		if (cflag && nc==1 && outfile)
			av[na++] = outfile;
		else
			av[na++] = setsuf(clist[i], 'o');
		av[na++] = assource;
		av[na] = 0;
		if (callsys(as, av) > 1) {
			cflag++;
			eflag++;
			continue;
		}
	}
nocom:
	if (cflag==0 && nl!=0) {
		i = 0;
#ifdef i286
		av[0] = "ld"; av[2] = crt0;
		av[1] = large ? "-ml" : medium ? "-mm" : "-ms";
		na = 3;
#else
		av[0] = "ld"; av[1] = "-X"; av[2] = crt0; na = 3;
#endif
		if (outfile) {
			av[na++] = "-o";
			av[na++] = outfile;
		}
		while (i < nl)
			av[na++] = llist[i++];
		if (proflag)
			av[na++] = "-lc_p";
		else
			av[na++] = "-lc";
		av[na++] = 0;
		eflag |= callsys(ld, av);
		if (nc==1 && nxo==1 && eflag==0)
			unlink(setsuf(clist[0], 'o'));
	}
	dexit();
}

void
idexit()
{

	eflag = 100;
	dexit();
}

dexit()
{

	if (!pflag) {
		cunlink(tmp1);
		cunlink(tmp2);
		if (sflag==0)
			cunlink(tmp3);
		cunlink(tmp4);
		cunlink(tmp5);
	}
	exit(eflag);
}

error(s, x)
	char *s, *x;
{
	FILE *diag = exflag ? stderr : stdout;

	fprintf(diag, "cc: ");
	fprintf(diag, s, x);
	putc('\n', diag);
	exfail++;
	cflag++;
	eflag++;
}

getsuf(as)
char as[];
{
	register int c;
	register char *s;
	register int t;

	s = as;
	c = 0;
	while (t = *s++)
		if (t=='/')
			c = 0;
		else
			c++;
	s -= 3;
	if (c <= MAXNAMLEN && c > 2 && *s++ == '.')
		return (*s);
	return (0);
}

char *
setsuf(as, ch)
	char *as;
{
	register char *s, *s1;

	s = s1 = savestr(as);
	while (*s)
		if (*s++ == '/')
			s1 = s;
	s[-1] = ch;
	return (s1);
}

callsys(f, v)
	char *f, **v;
{
	int t, status;
	char **cpp;

	if (debug) {
		fprintf(stderr, "%s:", f);
		for (cpp = v; *cpp != 0; cpp++)
			fprintf(stderr, " %s", *cpp);
		fprintf(stderr, "\n");
	}
	t = vfork();
	if (t == -1) {
		printf("No more processes\n");
		return (100);
	}
	if (t == 0) {
		if (quiet) {
			close(2);
			open("/dev/null", 1);
		}
		execv(f, v);
		printf("Can't find %s\n", f);
		fflush(stdout);
		_exit(100);
	}
	while (t != wait(&status))
		;
	if ((t=(status&0377)) != 0 && t!=14) {
		if (t!=2) {
			printf("Fatal error in %s\n", f);
			eflag = 8;
		}
		dexit();
	}
	return ((status>>8) & 0377);
}

nodup(l, os)
	char **l, *os;
{
	register char *t, *s;
	register int c;

	s = os;
	if (getsuf(s) != 'o')
		return (1);
	while (t = *l++) {
		while (c = *s++)
			if (c != *t++)
				break;
		if (*t==0 && c==0)
			return (0);
		s = os;
	}
	return (1);
}

#ifdef i286
/*
 * What the second pass is told in the medium model: -M and the name
 * of the segment the file's functions go in, which is the file's own
 * name without what follows its last dot, anything in it that is no
 * letter or digit made _, and _TEXT.  In the large model it is -L.
 */
char *
textseg(file)
	char *file;
{
	char buf[MAXNAMLEN + 12];
	register char *s, *t;

	for (s = t = file; *s; )
		if (*s++ == '/')
			t = s;
	s = buf;
	*s++ = '-';
	*s++ = large ? 'L' : 'M';
	if (isdigit(*t))
		*s++ = '_';
	while (*t && s < &buf[MAXNAMLEN]) {
		if (*t == '.' && t[1] && t[2] == 0)
			break;
		*s++ = isalnum(*t) ? *t : '_';
		t++;
	}
	strcpy(s, "_TEXT");
	return (savestr(buf));
}
#endif

#define	NSAVETAB	1024
char	*savetab;
int	saveleft;

char *
savestr(cp)
	register char *cp;
{
	register int len;

	len = strlen(cp) + 1;
	if (len > saveleft) {
		saveleft = NSAVETAB;
		if (len > saveleft)
			saveleft = len;
		savetab = (char *)malloc(saveleft);
		if (savetab == 0) {
			fprintf(stderr, "ran out of memory (savestr)\n");
			exit(1);
		}
	}
	strncpy(savetab, cp, len);
	cp = savetab;
	savetab += len;
	saveleft -= len;
	return (cp);
}

char *
strspl(left, right)
	char *left, *right;
{
	char buf[BUFSIZ];

	strcpy(buf, left);
	strcat(buf, right);
	return (savestr(buf));
}
