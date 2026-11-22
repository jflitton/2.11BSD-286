/*
 * mkstubs - write the C library's system call stubs, in assembly, from
 * a table.
 *
 * usage: mkstubs [-m s|m|l] [-k sysent] [-d dir] table syscall.h
 *
 * For the small memory model, the medium or the large (-m).  C calls a
 * stub, near in the small model and far in the others, with its
 * arguments on the stack.  Data pointers are near in the small and the
 * medium; a pointer to a function is near in the small model alone.
 * In the large model both are far, as the kernel wants them, and DS is
 * not the program's data segment: the stub reaches its arguments, and
 * errno, through SS.  The stub puts the call's number in AX and its
 * arguments, a word each, in BX, CX, DX, SI, DI and BP, and executes
 * INT 30h.  To the kernel a pointer is always two words, offset and
 * then selector: for a near one the stub passes DS, or CS for a
 * function, or 0 with a null pointer.  A call with more than six words
 * of arguments passes instead the far address of them all, which the
 * stub builds on its stack.
 *
 * The kernel returns with carry clear and the result in AX, or DX:AX;
 * or with carry set and the error number in AX, which goes to errno,
 * the C function returning -1.
 *
 * With -d each stub is written to a file of its own in dir, NAME.s,
 * and the error path and errno to cerror.s, so that a library made of
 * them gives a program only the calls it makes.  Without, every stub
 * is written to the standard output, as one file, with errno and the
 * error path.
 *
 * Each line of the table is
 *
 *	NAME[=CALL] ARGUMENTS [RESULT]
 *
 * NAME is the C function's.  CALL is the system call's name in
 * <syscall.h>, whose SYS_CALL is its number; it is NAME if not given.
 * ARGUMENTS is a letter for each argument of the C function, or - for
 * none: i an int, l a long, p a data pointer, f a pointer to a
 * function, P a data pointer that is far in every model.  RESULT says
 * what to do with what the kernel returns: nothing (the value in AX,
 * or DX:AX, as it is), or
 *
 *	fork	AX is 0 in the child, which the kernel marks with DX = 1
 *	vfork	the same, for a child that shares its parent's memory
 *	pair	the first argument points to two ints, for AX and DX;
 *		the kernel is passed nothing
 *	zero	the function returns 0
 *	errno0	errno is cleared first, because -1 can be a good result
 *	null	the result is a far pointer, in DX:AX, and a null one
 *		says the call failed
 *
 * Blank lines and lines beginning with # are ignored.
 *
 * With -k, the kernel's table of calls (sys/sys/init_sysent.c) is read
 * too, and each stub must pass as many words as the kernel takes for
 * its call.
 *
 * It is run both where the system is built, compiled for that machine,
 * and on the system, when it makes itself (docs/self-hosting.md): what
 * it writes is the same in both, to the byte.
 */
#include <stdio.h>
#include <string.h>

#define	NLINE	200		/* lines of one stub */
#define	LINELEN	64
#define	NWORD	24		/* words of one call's arguments */
#define	NSYS	256		/* system calls */
#define	NAMELEN	32
#define	BUFLEN	256

#define	DIGIT(c)	((c) >= '0' && (c) <= '9')
#define	LETTER(c)	((c) >= 'a' && (c) <= 'z' || (c) >= 'A' && (c) <= 'Z' \
			    || (c) == '_')

/* What the kernel is passed in a word. */
#define	ARG	0		/* the word of the arguments at w_off */
#define	SEL	1		/* the selector of the near data pointer there */
#define	CSEL	2		/* that of the near pointer to a function */

struct word {
	int	w_kind;
	int	w_off;		/* from the first argument */
} words[NWORD];
int	nwords;

char	*regs[] = { "bx", "cx", "dx", "si", "di", "bp" };
#define	NREGS	6

char	*head[] = {
	"        bits    16",
	"        cpu     286",
	"",
	"        segment _TEXT public align=2 class=CODE",
	0
};
char	*bss[] = {
	"        segment _BSS public align=2 class=BSS",
	"        group   DGROUP _BSS",
	0
};

/*
 * What differs between the models: the return, and so where the
 * arguments start, and the size of a pointer.
 */
int	farcode;
int	fardata;
char	*ret;
int	ra;			/* bytes of return address */
char	*ss;			/* what the stack and the data are reached through */

char	line[NLINE][LINELEN];	/* the stub being made */
int	nline;

char	sysname[NSYS][NAMELEN];	/* <syscall.h> */
int	sysnum[NSYS];
int	nsys;
int	count[NSYS];		/* words the kernel takes, by number */

char	*table;
int	lineno;
char	made[BUFLEN];

char *
l()
{

	if (nline >= NLINE) {
		fprintf(stderr, "%s:%d: too long a stub\n", table, lineno);
		exit(1);
	}
	return (line[nline++]);
}

void
put(s)
	char *s;
{

	strcpy(l(), s);
}

/*
 * The words the kernel is passed for arguments of the types in args.
 */
wordsof(name, args)
	char *name, *args;
{
	register struct word *w;
	register int off;
	int far;

	w = words;
	off = 0;
	for (; *args; args++) {
		if (w + 2 > &words[NWORD]) {
			fprintf(stderr, "%s:%d: %s: too many arguments\n",
			    table, lineno, name);
			exit(1);
		}
		switch (*args) {
		case 'i':
			w->w_kind = ARG;
			w++->w_off = off;
			off += 2;
			continue;
		case 'l':
		case 'P':
			far = 1;
			break;
		case 'p':
			far = fardata;
			break;
		case 'f':
			far = farcode;
			break;
		default:
			fprintf(stderr, "%s:%d: %s: argument type '%c'\n",
			    table, lineno, name, *args);
			exit(1);
		}
		w->w_kind = ARG;
		w++->w_off = off;
		if (far) {
			w->w_kind = ARG;
			w++->w_off = off + 2;
			off += 4;
		} else {
			w->w_kind = *args == 'p' ? SEL : CSEL;
			w++->w_off = off;
			off += 2;
		}
	}
	return (w - words);
}

/*
 * Instructions that put a word in reg, the arguments being at base,
 * with ov a segment override for it.
 */
void
load(reg, kind, off, base, n, ov)
	char *reg, *base, *ov;
{
	char *tmp;

	if (kind == ARG) {
		sprintf(l(), "        mov     %s, [%s%s+%d]", reg, ov, base, off);
		return;
	}
	tmp = strcmp(reg, "bp") == 0 ? "ax" : reg;
	sprintf(l(), "        mov     %s, %s", tmp, kind == SEL ? "ds" : "cs");
	sprintf(l(), "        cmp     word [%s+%d], 0", base, off);
	sprintf(l(), "        jne     .p%d", n);
	sprintf(l(), "        xor     %s, %s", tmp, tmp);
	sprintf(l(), ".p%d:", n);
	if (tmp != reg)
		sprintf(l(), "        mov     %s, %s", reg, tmp);
}

void
call(number)
{

	sprintf(l(), "        mov     ax, %d", number);
	put("        int     0x30");
}

/*
 * A stub, with its jumps to the error path as the small model has
 * them.
 */
void
stub(name, number, args, result)
	char *name, *args, *result;
{
	register int k;
	register struct word *w;
	register char *p;

	nline = 0;
	put("");
	sprintf(l(), "        global  _%s", name);
	sprintf(l(), "_%s:", name);
	if (strcmp(result, "vfork") == 0) {
		/*
		 * The child runs on the parent's stack until it executes
		 * another program, and will have written over anything
		 * left there by the time the parent returns.  So nothing
		 * is left there: the return address waits in CX, and its
		 * segment in BX, which the kernel preserves.
		 */
		put("        pop     cx");
		if (!farcode) {
			call(number);
			put("        jc      .err");
			put("        or      dx, dx");
			put("        jz      .parent");
			put("        xor     ax, ax");
			put(".parent:");
			put("        jmp     cx");
			put(".err:   push    cx");
			put("        jmp     cerror");
			return;
		}
		put("        pop     bx");
		call(number);
		put("        push    bx");
		put("        push    cx");
		put("        jc      cerror");
		put("        or      dx, dx");
		put("        jz      .parent");
		put("        xor     ax, ax");
		put(".parent:");
		put("        retf");
		return;
	}
	if (strcmp(result, "pair") == 0) {
		/*
		 * The kernel is passed nothing; the argument is where the
		 * stub puts the two results.
		 */
		call(number);
		put("        jc      cerror");
		put("        mov     bx, sp");
		sprintf(l(), fardata ? "        lds     bx, [ss:bx+%d]" :
		    "        mov     bx, [bx+%d]", ra);
		put("        mov     [bx], ax");
		put("        mov     [bx+2], dx");
		put("        xor     ax, ax");
		sprintf(l(), "        %s", ret);
		return;
	}
	nwords = wordsof(name, args);
	if (strcmp(result, "errno0") == 0)
		sprintf(l(), "        mov     word [%s_errno], 0", ss);
	if (nwords <= 3) {
		/*
		 * Few enough for BX, CX and DX: no frame, and BX, the
		 * base, loaded last.  The arguments are above the return
		 * address.
		 */
		put("        mov     bx, sp");
		for (k = nwords - 1; k >= 0; k--) {
			w = &words[k];
			load(regs[k], w->w_kind, w->w_off + ra, "bx", k, ss);
		}
		call(number);
		put("        jc      cerror");
	} else {
		put("        push    bp");
		put("        mov     bp, sp");
		put("        push    si");
		put("        push    di");
		if (nwords <= NREGS) {
			/* BP, the frame pointer, is loaded last. */
			for (k = 0; k < nwords; k++) {
				w = &words[k];
				load(regs[k], w->w_kind, w->w_off + 2 + ra,
				    "bp", k, "");
			}
		} else {
			for (k = nwords - 1; k >= 0; k--) {
				w = &words[k];
				if (w->w_kind == ARG) {
					sprintf(l(),
					    "        push    word [bp+%d]",
					    w->w_off + 2 + ra);
					continue;
				}
				load("ax", w->w_kind, w->w_off + 2 + ra,
				    "bp", k, "");
				put("        push    ax");
			}
			put("        mov     bx, sp");
			put("        mov     cx, ss");
		}
		call(number);
		if (nwords > NREGS)
			put("        lea     sp, [bp-4]      ; flags as they are");
		put("        pop     di");
		put("        pop     si");
		put("        pop     bp");
		put("        jc      cerror");
	}
	if (strcmp(result, "fork") == 0) {
		put("        or      dx, dx");
		put("        jz      .parent");
		put("        xor     ax, ax");
		put(".parent:");
	} else if (strcmp(result, "zero") == 0)
		put("        xor     ax, ax");
	else if (strcmp(result, "null") == 0) {
		for (k = 0; k < nline; k++) {
			p = line[k] + strlen(line[k]) - 6;
			if (p >= line[k] && strcmp(p, "cerror") == 0)
				strcpy(p, "cnull");
		}
	} else if (*result && strcmp(result, "errno0") != 0) {
		fprintf(stderr, "%s:%d: %s: result '%s'\n", table, lineno,
		    name, result);
		exit(1);
	}
	sprintf(l(), "        %s", ret);
}

/*
 * Write the stub.  Where its code is far, so is its jump to cerror:
 * the system's linker puts files' code in segments in the order they
 * are linked, and a stub may not be in the one cerror is in.
 */
void
outstub(f)
	register FILE *f;
{
	register int i, n;
	register char *to;

	n = 0;
	for (i = 0; i < nline; i++) {
		to = 0;
		if (farcode && strcmp(line[i], "        jc      cnull") == 0)
			to = "cnull";
		if (farcode && strcmp(line[i], "        jc      cerror") == 0)
			to = "cerror";
		if (to) {
			fprintf(f, "        jnc     .ok%d\n", n);
			fprintf(f, "        jmp     far %s\n", to);
			fprintf(f, ".ok%d:\n", n);
			n += 3;
			continue;
		}
		if (farcode && strcmp(line[i], "        jmp     cerror") == 0)
			fprintf(f, "        jmp     far cerror\n");
		else
			fprintf(f, "%s\n", line[i]);
		n++;
	}
}

void
outlines(f, p)
	FILE *f;
	register char **p;
{

	for (; *p; p++)
		fprintf(f, "%s\n", *p);
}

/*
 * errno, and the path every stub takes with an error: reached by a
 * jump, near or far as the stub's own return is.
 */
void
cerror(f)
	register FILE *f;
{

	outlines(f, bss);
	fprintf(f, "        global  _errno, cerror, cnull\n\n");
	fprintf(f, "        segment _BSS\n");
	fprintf(f, "_errno: resw    1\n\n");
	fprintf(f, "        segment _TEXT\n");
	fprintf(f, "; The kernel said no: the error number is in AX.\n");
	fprintf(f, "cerror: mov     [%s_errno], ax\n", ss);
	fprintf(f, "        mov     ax, -1\n");
	fprintf(f, "        cwd\n");
	fprintf(f, "        %s\n", ret);
	fprintf(f, "; The same for a call whose result is a far pointer.\n");
	fprintf(f, "cnull:  mov     [%s_errno], ax\n", ss);
	fprintf(f, "        xor     ax, ax\n");
	fprintf(f, "        cwd\n");
	fprintf(f, "        %s\n", ret);
}

FILE *
efopen(name, mode)
	char *name, *mode;
{
	register FILE *f;

	f = fopen(name, mode);
	if (f == NULL) {
		perror(name);
		exit(1);
	}
	return (f);
}

/*
 * SYS_ names and numbers from <syscall.h>.
 */
void
numbers(name)
	char *name;
{
	register FILE *f;
	char buf[BUFLEN], nm[BUFLEN];
	int n;

	f = efopen(name, "r");
	while (fgets(buf, sizeof buf, f) != NULL) {
		if (sscanf(buf, "#define SYS_%s %d", nm, &n) != 2)
			continue;
		if (nsys >= NSYS || strlen(nm) >= NAMELEN) {
			fprintf(stderr, "%s: SYS_%s: no room\n", name, nm);
			exit(1);
		}
		strcpy(sysname[nsys], nm);
		sysnum[nsys++] = n;
	}
	fclose(f);
}

/*
 * The number of argument words of each call in the kernel's table,
 * whose lines are
 *
 *	4, read,		/+   3 = read +/
 *	ifnet(4, bind),		/+ 104 = bind +/
 */
void
sysent(name)
	char *name;
{
	register FILE *f;
	register char *p;
	register int n;
	int num;
	char buf[BUFLEN];

	f = efopen(name, "r");
	while (fgets(buf, sizeof buf, f) != NULL) {
		for (p = buf; *p == ' ' || *p == '\t'; p++)
			;
		if (LETTER(*p)) {
			while (LETTER(*p) || DIGIT(*p))
				p++;
			if (*p++ != '(')
				continue;
		}
		if (!DIGIT(*p))
			continue;
		for (n = 0; DIGIT(*p); p++)
			n = n * 10 + *p - '0';
		if (*p != ',')
			continue;
		while (*p && (p[0] != '/' || p[1] != '*'))
			p++;
		if (*p == 0)
			continue;
		for (p += 2; *p == ' ' || *p == '\t'; p++)
			;
		if (!DIGIT(*p))
			continue;
		for (num = 0; DIGIT(*p); p++)
			num = num * 10 + *p - '0';
		if (p[0] == ' ' && p[1] == '=' && num < NSYS)
			count[num] = n;
	}
	fclose(f);
}

main(argc, argv)
	char **argv;
{
	register FILE *f, *out;
	register char *p;
	register int i;
	char *model, *kernel, *dir, *call, *args, *result;
	char buf[BUFLEN], w[4][BUFLEN], path[1024];
	int n, have;

	model = "s";
	kernel = dir = 0;
	while (argc > 2 && argv[1][0] == '-' && argv[1][1] && !argv[1][2]) {
		switch (argv[1][1]) {
		case 'm':
			model = argv[2];
			break;
		case 'k':
			kernel = argv[2];
			break;
		case 'd':
			dir = argv[2];
			break;
		default:
			argc = 0;
		}
		argc -= 2;
		argv += 2;
	}
	if (argc != 3 || !model[0] || model[1] ||
	    model[0] != 's' && model[0] != 'm' && model[0] != 'l') {
		fprintf(stderr,
	"usage: mkstubs [-m s|m|l] [-k sysent] [-d dir] table syscall.h\n");
		exit(1);
	}
	farcode = model[0] != 's';
	fardata = model[0] == 'l';
	ret = farcode ? "retf" : "ret";
	ra = farcode ? 4 : 2;
	ss = fardata ? "ss:" : "";
	table = argv[1];
	numbers(argv[2]);
	for (i = 0; i < NSYS; i++)
		count[i] = -1;
	if (kernel)
		sysent(kernel);
	p = table;
	for (i = 0; table[i]; i++)
		if (table[i] == '/')
			p = &table[i + 1];
	sprintf(made, "; Made by mkstubs from %s: do not edit.\n\n", p);

	out = stdout;
	if (dir) {
		sprintf(path, "%s/cerror.s", dir);
		out = efopen(path, "w");
	}
	fprintf(out, "%s", made);
	outlines(out, head);
	cerror(out);
	if (dir)
		fclose(out);

	f = efopen(table, "r");
	while (fgets(buf, sizeof buf, f) != NULL) {
		lineno++;
		n = sscanf(buf, "%s %s %s %s", w[0], w[1], w[2], w[3]);
		if (n < 1 || w[0][0] == '#')
			continue;
		if (n != 2 && n != 3) {
			fprintf(stderr,
			    "%s:%d: cannot make sense of this line\n",
			    table, lineno);
			exit(1);
		}
		call = w[0];
		for (p = w[0]; *p; p++)
			if (*p == '=') {
				*p = 0;
				call = p + 1;
				break;
			}
		for (i = 0; i < nsys; i++)
			if (strcmp(sysname[i], call) == 0)
				break;
		if (i == nsys) {
			fprintf(stderr, "%s:%d: no SYS_%s in %s\n", table,
			    lineno, call, argv[2]);
			exit(1);
		}
		args = strcmp(w[1], "-") == 0 ? "" : w[1];
		result = n == 3 ? w[2] : "";
		stub(w[0], sysnum[i], args, result);
		if (kernel) {
			have = strcmp(result, "pair") == 0 ? 0 :
			    wordsof(w[0], args);
			n = sysnum[i] < NSYS ? count[sysnum[i]] : -1;
			if (n != have) {
				fprintf(stderr,
			"%s:%d: %s passes %d words; the kernel takes %d\n",
				    table, lineno, w[0], have, n);
				exit(1);
			}
		}
		if (dir) {
			sprintf(path, "%s/%s.s", dir, w[0]);
			out = efopen(path, "w");
			fprintf(out, "%s", made);
			outlines(out, head);
			outlines(out, bss);
			fprintf(out, "        extern  cerror, cnull, _errno\n");
			fprintf(out, "        segment _TEXT\n");
		}
		outstub(out);
		if (dir)
			fclose(out);
	}
	if (ferror(stdout) || fflush(stdout) == EOF) {
		perror("mkstubs");
		exit(1);
	}
	exit(0);
}
