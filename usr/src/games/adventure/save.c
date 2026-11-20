/* save (III)   J. Gillogly
 * save user core image for restarting
 * usage: save(<command file (argv[0] from main)>,<output file>)
 * bugs
 *   -  impure code (i.e. changes in instructions) is not handled
 *      (but people that do that get what they deserve)
 */

static char sccsid[] = "	save.c	4.1	82/05/11	";

#include <stdio.h>
#include <a.out.h>
int filesize;                    /* accessible to caller         */

char *sbrk();

#ifdef i286
/*
 * On the 286 a program is a header, a table of its segments and the
 * contents of each (docs/executable-format.md).  The code is copied from
 * the command; the data segment is written as it is in memory, from 0 to
 * the end of the BSS, all of it initialized now.  The heap does not follow
 * the BSS here, as it does on the PDP-11: the stack and the arguments lie
 * between them, and a run with more arguments and environment has its
 * heap start higher.  So the first run moves the break up by as much room
 * as they can take before it reads its data (heapfrom), the heap is
 * written after the segments, and heapback reads it into place when the
 * program is run again.  filesize is where the messages begin, after it.
 */
#include <sys/param.h>

unsigned heaplo;		/* where the heap begins */
long heapat;			/* where it is in the file */
extern char end, *_minbrk;
extern int datfd;
char *brk();

heapfrom()
{
	/*
	 * NCARGS bytes of strings, a pointer to each, at most one to every
	 * byte, two null pointers and two words; and a click, to which the
	 * kernel rounds the stack and they together.
	 */
	sbrk(3 * NCARGS + 8 + 512);
	heaplo = (unsigned) sbrk(0);
}

heapback()
{
	unsigned n = (unsigned) filesize - (unsigned) heapat;

	if (heaplo < (unsigned) _minbrk || brk((char *) heaplo + n) != 0)
		return (-1);
	lseek(datfd, heapat, 0);
	return (read(datfd, (char *) heaplo, n) == n ? 0 : -1);
}

save(cmdfile,outfile)                   /* save core image              */
char *cmdfile,*outfile;
{       register int i,fd;
	int fdaout;
	struct xexec x;
	long at[NXSEG], off;
	unsigned heaphi, p, n;
	char buff[512];

	fflush(stdout);                 /* else it is saved, and written*/
					/* again by each run of it      */
	fdaout=getcmd(cmdfile);         /* open command wherever it is  */
	if (fdaout<0) return(-1);       /* can do nothing without text  */
	if ((fd=open(outfile,0))>0)     /* this restriction is so that  */
	{       printf("Can't use an existing file\n"); /* we don't try */
		close(fd);              /* to write over the commnd file*/
		return(-1);
	}
	if (read(fdaout,&x.e,sizeof x.e)!=sizeof x.e || x.e.a_magic!=A_MAGIC ||
	    x.e.a_nseg>NXSEG || read(fdaout,x.x,x.e.a_nseg*sizeof x.x[0]) !=
	    x.e.a_nseg*sizeof x.x[0])
	{       printf("%s is not a program\n",cmdfile);
		return(-1);
	}
	if ((fd=creat(outfile,0755))== -1)
	{       printf("Cannot create %s\n",outfile);
		return(-1);
	}
	x.e.a_syms = x.e.a_strs = 0;    /* throw away symbol table      */
	off = sizeof x.e + x.e.a_nseg*sizeof x.x[0];
	for (i=0; i<x.e.a_nseg; i++)    /* where each is in the command */
	{       at[i] = off;
		off += (x.x[i].xs_filesz+1) & ~1L;
	}
	off = sizeof x.e + x.e.a_nseg*sizeof x.x[0];
	for (i=0; i<x.e.a_nseg; i++)    /* the data is all initialized  */
	{       if (x.x[i].xs_type==XS_DATA)
			x.x[i].xs_filesz = x.x[i].xs_memsz = (unsigned)&end;
		off += (x.x[i].xs_filesz+1) & ~1L;
	}
	heapat = off;
	heaphi = (unsigned)sbrk(0);
	off += heaphi-heaplo;
	if (off>65535L)                 /* filesize is an unsigned      */
	{       printf("%s would be too big\n",outfile);
		close(fd);
		unlink(outfile);
		return(-1);
	}
	filesize = off;
	n = x.e.a_nseg*sizeof x.x[0];   /* make the new header          */
	if (write(fd,&x.e,sizeof x.e)!=sizeof x.e || write(fd,x.x,n)!=n)
		goto bad;
	for (i=0; i<x.e.a_nseg; i++)
	{       if (x.x[i].xs_type==XS_DATA)
		/*
		 * Through buff: a system call takes a pointer of 0 for
		 * none, and so cannot be given the data from address 0.
		 */
			for (p=0; p<(unsigned)&end; p+=n)
			{       n = (unsigned)&end-p<sizeof buff?
				    (unsigned)&end-p:sizeof buff;
				bcopy((char *)p,buff,n);
				if (write(fd,buff,n)!=n) goto bad;
			}
		else
		{       lseek(fdaout,at[i],0);
			for (off=x.x[i].xs_filesz; off>0; off-=n)
			{       n = off<sizeof buff?(int)off:sizeof buff;
				if (read(fdaout,buff,n)!=n ||
				    write(fd,buff,n)!=n) goto bad;
			}
		}
		if ((x.x[i].xs_filesz & 1) && write(fd,"",1)!=1)
			goto bad;
	}
	n = heaphi-heaplo;
	if (write(fd,(char *)heaplo,n)!=n)
		goto bad;
	close(fd);
	close(fdaout);
	return(0);
bad:    printf("Cannot write %s\n",outfile);
	close(fd);
	close(fdaout);
	unlink(outfile);
	return(-1);
}
#else
save(cmdfile,outfile)                   /* save core image              */
char *cmdfile,*outfile;
{       register char *c;
	register int i,fd;
	int fdaout;
	struct exec header;
	int counter;
	char buff[512],pwbuf[120];
	fflush(stdout);                 /* else it is saved, and written*/
					/* again by each run of it      */
	fdaout=getcmd(cmdfile);         /* open command wherever it is  */
	if (fdaout<0) return(-1);       /* can do nothing without text  */
	if ((fd=open(outfile,0))>0)     /* this restriction is so that  */
	{       printf("Can't use an existing file\n"); /* we don't try */
		close(fd);              /* to write over the commnd file*/
		return(-1);
	}
	if ((fd=creat(outfile,0755))== -1)
	{       printf("Cannot create %s\n",outfile);
		return(-1);
	}
	/* can get the text segment from the command that we were
	 * called with, and change all data from uninitialized to
	 * initialized.  It will start at the top again, so the user
	 * is responsible for checking whether it was restarted
	 * could ignore sbrks and breaks for the first pass
	 */
	read(fdaout,&header,sizeof header);/* get the header       */
	header.a_bss = 0;                  /* no data uninitialized        */
	header.a_syms = 0;                 /* throw away symbol table      */
	switch (header.a_magic)            /* find data segment            */
	{   case 0407:                     /* non sharable code            */
		c = (char *) header.a_text;/* data starts right after text */
		header.a_data=sbrk(0)-c;   /* current size (incl allocs)   */
		break;
	    case 0410:                     /* sharable code                */
		c = (char *)
#ifdef pdp11
		    (header.a_text	   /* starts after text            */
		    & 0160000)             /* on an 8K boundary            */
		    +  020000;             /* i.e. the next one up         */
#endif
#ifdef vax
		    (header.a_text	   /* starts after text            */
		    & 037777776000)        /* on an 1K boundary            */
		    +        02000;        /* i.e. the next one up         */
#endif
#ifdef z8000
		    (header.a_text	   /* starts after text            */
		    & 0174000)             /* on an 2K boundary            */
		    +  004000;             /* i.e. the next one up         */
#endif
		header.a_data=sbrk(0)-c;   /* current size (incl allocs)   */
		break;
	    case 0411:                     /* sharable with split i/d      */
		c = 0;                     /* can't reach text             */
		header.a_data=(int)sbrk(0);/* current size (incl allocs)   */
		break;
	    case 0413:
		c = (char *) header.a_text;/* starts after text            */
		lseek(fdaout, 1024L, 0);   /* skip unused part of 1st block*/
	}
	if (header.a_data<0)               /* data area very big           */
		return(-1);                /* fail for now                 */

	filesize=sizeof header+header.a_text+header.a_data;
	write(fd,&header,sizeof header);   /* make the new header          */
	if (header.a_magic==0413)
		lseek(fd, 1024L, 0);       /* Start on 1K boundary	   */
	counter=header.a_text;             /* size of text                 */
	while (counter>512)                /* copy 512-byte blocks         */
	{       read(fdaout,buff,512);     /* as long as possible          */
		write(fd,buff,512);
		counter -= 512;
	}
	read(fdaout,buff,counter);         /* then pick up the rest        */
	write(fd,buff,counter);
	write(fd,c,header.a_data);         /* write all data in 1 glob     */
	close(fd);
}
#endif

#define	NULL	0

char	*execat(), *getenv();

getcmd(command)         /* get command name (wherever it is) like shell */
char *command;
{
	char *pathstr;
	register char *cp;
	char fname[128];
	int fd;

	if ((pathstr = getenv("PATH")) == NULL)
		pathstr = ":/bin:/usr/bin";
	cp = index(command, '/')? "": pathstr;

	do {
		cp = execat(cp, command, fname);
		if ((fd=open(fname,0))>0)
			return(fd);
	} while (cp);

	printf("Couldn't open %s\n",command);
	return(-1);
}

static char *
execat(s1, s2, si)
register char *s1, *s2;
char *si;
{
	register char *s;

	s = si;
	while (*s1 && *s1 != ':' && *s1 != '-')
		*s++ = *s1++;
	if (si != s)
		*s++ = '/';
	while (*s2)
		*s++ = *s2++;
	*s = '\0';
	return(*s1? ++s1: 0);
}
