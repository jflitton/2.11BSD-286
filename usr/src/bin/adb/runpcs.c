#if     !defined(lint) && defined(DOSCCS)
static  char    sccsid[] = "@(#)runpcs.c 3.0 (2.11BSD) 2026/11/25";
#endif

#include "defs.h"
#include <sys/file.h>

	MSG	NOFORK;
	MSG	ENDPCS;
	MSG     BADWAIT;
	char    *lp;
extern	sig_t	sigint;
extern	sig_t	sigqit;
	BKPTR   bkpthead;
	REGLIST reglist[];
	char	lastc;
	u_int	corhdr[];
	u_int	*uar0;
	int	fcor;
	int	fsym;
	char	*errflg;
	int	signo;
	long	dot;
	char	*symfil;
	int	wtflag;
	int	pid;
	long	expv;
	int	adrflg;
	long	loopcnt;
	long	var[];
	MAP	txtmap;
	MAP	datmap;
	long	userpc=1;
extern	int	errno;

/* service routines for sub process control */

getsig(sig)
	{
	return(expr(0) ? shorten(expv) : sig);
	}

/* where the process is: CS and IP */
long
pcaddr()
{
	return(ADDR(uar0[CS], uar0[IP]));
}

runpcs(runmode, execsig)
{
	int	rc;
	register BKPTR       bkpt;

	IF adrflg
	THEN userpc=fulladr(dot, ISP);
	FI
	printf("%s: running\n", symfil);

	WHILE (loopcnt--)>0
	DO
#ifdef DEBUG
		printf("\ncontinue %X %d\n",userpc,execsig);
#endif
		/* If we're at a breakpoint, execute that using execbkpt */
		if (bkpt=scanbkpt(userpc == 1 ? pcaddr() : userpc)) {
			execbkpt(bkpt);
			userpc = 1;
		}
		/*
		** If we're single stepping and already executed the
		** instruction at the breakpoint, don't do another one
		*/
		if (signo == 0 && (!bkpt || runmode != PT_STEP)) {
			setbp();
			stty(0,&usrtty);
			ptrace(runmode,pid,userpc,execsig);
			bpwait(); chkerr();
		}
		readregs();

		/*
		** look for bkpt
		** in step mode we would have actually hit the breakpoint,
		** not stepped through it, so look at PC instead of past
		** the INT 3
		*/
		IF signo==0 ANDF
			(bkpt=scanbkpt(ADDR(uar0[CS], uar0[IP] - (runmode == PT_STEP ? 0:1))))
		THEN /*stopped at bkpt*/
		     userpc=bkpt->loc;
		     uar0[IP]=AOFF(bkpt->loc);
		     IF bkpt->flag==BKPTEXEC
			ORF ((bkpt->flag=BKPTEXEC, command(bkpt->comm,':'))
			     ANDF --bkpt->count)
		     THEN execbkpt(bkpt); execsig=0; loopcnt++;
			  userpc=1;
		     ELSE bkpt->count=bkpt->initcnt;
			  rc=1;
		     FI
		ELSE rc=0; execsig=signo; userpc=1;
		FI
	OD
	return(rc);
}

endpcs()
{
	register BKPTR       bkptr;

	IF pid
	THEN ptrace(PT_KILL,pid,0L,0); pid=0; userpc=1;
	     FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	     DO IF bkptr->flag
		THEN bkptr->flag=BKPTSET;
		FI
	     OD
	     gone();
	FI
}

/* the process is gone, and its breakpoints with it */
gone()
{
	register BKPTR       bkptr;

	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO	bkptr->inproc=0;
	OD
	userpc=1;
}

setup()
{

	close(fsym); fsym = -1;
	IF (pid = fork()) == 0
	THEN ptrace(PT_TRACE_ME,0,0L,0);
	     signal(SIGINT,sigint); signal(SIGQUIT,sigqit);
	     doexec(); exit(0);
	ELIF pid == -1
	THEN error(NOFORK);
	ELSE bpwait(); readregs(); lp[0]=EOR; lp[1]=0;
	     fsym=open(symfil,wtflag);
	     txtmap.ufd=fsym;
	     IF errflg
	     THEN printf("%s: cannot execute\n",symfil);
		  endpcs(); error((char *)0);
	     FI
	FI
}

/*
 * Execute the instruction under a breakpoint: put its byte back, step
 * over it from the breakpoint, and put the INT 3 back.
 */
execbkpt(bkptr)
BKPTR   bkptr;
{
#ifdef DEBUG
	printf("execbkpt: %X %d\n",bkptr->loc,bkptr->count);
#endif
	del1bp(bkptr);
	stty(0,&usrtty);
	ptrace(PT_STEP,pid,bkptr->loc,0);
	bpwait(); chkerr();
	set1bp(bkptr);
	bkptr->flag=BKPTSET;
}

doexec()
{
	char	*argl[MAXARG];
	char	args[LINSIZ];
	char	*p, **ap, *filnam;

	ap=argl; p=args;
	*ap++=symfil;
	REP     IF rdc()==EOR THEN break; FI
		/*
		 * If we find an argument beginning with a `<' or a `>', open
		 * the following file name for input and output, respectively
		 * and back the argument collocation pointer, p, back up.
		 */
		*ap = p;
		WHILE lastc!=EOR ANDF lastc!=SP ANDF lastc!=TB DO *p++=lastc; readchar(); OD
		*p++=0; filnam = *ap+1;
		IF **ap=='<'
		THEN    close(0);
			IF open(filnam,0)<0
			THEN    printf("%s: cannot open\n",filnam); exit(0);
			FI
			p = *ap;
		ELIF **ap=='>'
		THEN    close(1);
			IF open(filnam, O_CREAT|O_WRONLY, 0666)<0
			THEN    printf("%s: cannot create\n",filnam); exit(0);
			FI
			p = *ap;
		ELSE    ap++;
		FI
	PER lastc!=EOR DONE
	*ap++=0;
	execv(symfil, argl);
}

BKPTR   scanbkpt(adr)
	long	adr;
{
	register BKPTR       bkptr;

	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag ANDF bkptr->loc==adr
	   THEN break;
	   FI
	OD
	return(bkptr);
}

delbp()
{
	register BKPTR       bkptr;

	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag
	   THEN del1bp(bkptr);
	   FI
	OD
}

/* put back the byte the INT 3 took the place of */
del1bp(bkptr)
BKPTR bkptr;
{
	register int w;

	IF bkptr->inproc
	THEN	w = get(bkptr->loc, ISP);	/* with the byte */
		bkptr->inproc = 0;
		put(bkptr->loc, ISP, w);
	FI
}

setbp()
{
	register BKPTR       bkptr;

	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag
	   THEN set1bp(bkptr);
	   FI
	OD
}

/*
 * Put an INT 3 in place of the first byte of the instruction, keeping
 * the byte.  While the breakpoint is in the process, get and put see the
 * byte and not the INT 3 (access.c).
 */
set1bp(bkptr)
BKPTR bkptr;
{
	register int w;

	IF bkptr->inproc THEN return; FI
	errflg = 0;
	w = get(bkptr->loc, ISP);
	IF errflg == 0
	THEN	bkptr->ins = w & LOBYTE;
		bkptr->inproc = 1;
		put(bkptr->loc, ISP, w);
	FI
	IF errflg
	THEN	bkptr->inproc = 0;
		errflg = 0;
		printf("cannot set breakpoint: ");
		psymoff(bkptr->loc,ISYM,"\n");
	FI
}

bpwait()
{
	register int w;
	int stat;

	signal(SIGINT, SIG_IGN);
	WHILE (w = wait(&stat))!=pid ANDF w != -1 DONE
	signal(SIGINT,sigint);
	gtty(0,&usrtty);
	stty(0,&adbtty);
	IF w == -1
	THEN pid=0;
	     errflg=BADWAIT;
	     gone();
	ELIF (stat & 0177) != 0177
	THEN IF signo = stat&0177
	     THEN sigprint();
	     FI
	     IF stat&0200
	     THEN printf(" - core dumped");
		  close(fcor);
		  setcor();
	     FI
	     pid=0;
	     errflg=ENDPCS;
	     gone();
	ELSE signo = stat>>8;
	     IF signo!=SIGTRAP
	     THEN sigprint();
	     ELSE signo=0;
	     FI
	     flushbuf();
	FI
}

/*
 * The registers, from the frame at the top of the process's user
 * structure, and the 287's, which the kernel has put in it.
 */
readregs()
{
	register u_int i;

	FOR i=0; i<NREG; i++
	DO uar0[reglist[i].roffs] = ptrace(PT_READ_U, pid,
		(long)((UAR0 + reglist[i].roffs) * sizeof (u_int)), 0);
	OD
	txtmap.def1 = uar0[CS];
	datmap.def2 = uar0[SS];
	FOR i = offsetof(struct user, u_fps);
	    i < offsetof(struct user, u_fps) + sizeof (struct fps);
	    i += sizeof (u_int)
	DO	corhdr[i / sizeof (u_int)] = ptrace(PT_READ_U, pid, (long)i, 0);
	OD
}
