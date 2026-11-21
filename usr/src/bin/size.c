#if	defined(DOSCCS) && !defined(lint)
static	char *sccsid = "@(#)size.c	4.6 (2.11BSD) 2026/9/13";
#endif

/*
 * size
 */

#include	<stdio.h>
#include 	<a.out.h>
#include	<string.h>
#include	<errno.h>

int	header;

main(argc, argv)
char **argv;
{
#ifdef i286
	/*
	 * The 286's programs and objects are segments (sys/exec.h): the
	 * text is those of code, the data what the others have in the
	 * file, and the bss what they have besides.  A program in the
	 * medium model has more than one of text, and their sizes are
	 * given as the PDP-11's overlays' are.
	 */
	struct xexec xbuf;
#define	buf	xbuf.e
	long text, data, bss;
#else
	struct exec buf;
#endif
	long sum;
	register int i;
	int gorp, err = 0;
	register FILE *f;
#ifndef i286
	struct ovlhdr	ovlbuf;		/* overlay structure */
	long	coresize;		/* total text size */
#endif
	short	skip;			/* skip over overlay sizes of 0 */

	if (argc==1) {
		*argv = "a.out";
		argc++;
		--argv;
	}
	gorp = argc;
	while(--argc) {
		++argv;
		if ((f = fopen(*argv, "r"))==NULL) {
			printf("size: %s: %s\n", *argv, strerror(errno));
			err++;
			continue;
		}
#ifdef i286
		if (fread((char *)&xbuf, sizeof(buf), 1, f) != 1 ||
		    N_BADMAG(buf) || buf.a_nseg > NXSEG ||
		    fread((char *)xbuf.x, sizeof(xbuf.x[0]), buf.a_nseg, f) !=
		    buf.a_nseg) {
#else
		if (fread((char *)&buf, sizeof(buf), 1, f) != 1 ||
		    N_BADMAG(buf)) {
#endif
			printf("size: %s not an object file\n", *argv);
			fclose(f);
			err++;
			continue;
		}
		if (header == 0) {
			printf("text\tdata\tbss\tdec\thex\n");
			header = 1;
		}
#ifdef i286
		text = data = bss = 0;
		skip = 0;
		for (i = 0; i < buf.a_nseg; i++)
			if (xbuf.x[i].xs_type == XS_CODE) {
				text += xbuf.x[i].xs_memsz;
				skip++;
			} else {
				data += xbuf.x[i].xs_filesz;
				bss += xbuf.x[i].xs_memsz - xbuf.x[i].xs_filesz;
			}
		sum = text + data + bss;
		printf("%ld\t%ld\t%ld\t%ld\t%lx", text, data, bss, sum, sum);
		if (gorp>2)
			printf("\t%s", *argv);
		if (skip > 1) {
			printf("\ttext segments: ");
			for (i = 0, skip = 0; i < buf.a_nseg; i++)
				if (xbuf.x[i].xs_type == XS_CODE)
					printf(skip++ ? ",%ld" : "%ld",
					    (long)xbuf.x[i].xs_memsz);
		}
#else
		printf("%u\t%u\t%u\t", buf.a_text,buf.a_data,buf.a_bss);
		sum = (long) buf.a_text + (long) buf.a_data + (long) buf.a_bss;
		printf("%ld\t%lx", sum, sum);
		if (gorp>2)
			printf("\t%s", *argv);
		if (buf.a_magic == A_MAGIC5 || buf.a_magic == A_MAGIC6) {
			fread(&ovlbuf,sizeof(ovlbuf),1,f);
			coresize = buf.a_text;
			for (i = 0; i < NOVL; i++)
				coresize += ovlbuf.ov_siz[i];
			printf("\ttotal text: %ld\n\toverlays: ", coresize);
			for (i = 0,skip = 0; i < NOVL; i++) {
				if (!ovlbuf.ov_siz[i]) {
					++skip;
					continue;
				}
				for (;skip;--skip)
					fputs(",0",stdout);
				if (i > 0)
					putchar(',');
				printf("%u", ovlbuf.ov_siz[i]);
			}
		}
#endif
		printf("\n");
		fclose(f);
	}
	exit(err);
}
