#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.7.13 (2.11BSD) 2026/6/5
#
# Several routines have been rewritten in assembly language for the VAX and
# the PDP. If you are not running on a VAX or PDP, you should use the
# original C sources which are included in this directory.
#
# The 80286 has them, MDSRC: what the PDP-11's ../pdp/gen has in
# assembly.  And it has no fakcu.o, the empty _cleanup that exit()
# calls in a program without stdio: that works by the PDP-11's order
# of the library, and the 286's ld took it in place of stdio's, which
# then did not flush at exit.
#
SRCS=	${STDSRC} ${MDSRC}
OBJS=	${STDOBJ} ${MDOBJ}
MDSRC=	abs.c bcmp.c bcopy.c bzero.c fabs.c ffs.c frexp.c index.c insque.c \
	remque.c rindex.c strcat.c strcmp.c strcpy.c strlen.c strncat.c \
	strncmp.c strncpy.c
MDOBJ=	abs.o bcmp.o bcopy.o bzero.o fabs.o ffs.o frexp.o index.o insque.o \
	remque.o rindex.o strcat.o strcmp.o strcpy.o strlen.o strncat.o \
	strncmp.o strncpy.o

CFLAGS=	-O ${DEFS}
STDSRC=	abort.c alarm.c atoi.c atol.c calloc.c closedir.c crypt.c \
	ctime.c ctype_.c daemon.c devname.c disklabel.c err.c \
	execvp.c fakcu.c \
	fstab.c gcvt.c getenv.c getgrent.c getgrgid.c getgrnam.c getlogin.c \
	getgrouplist.c gethostname.c getpagesize.c \
	getpass.c getpwent.c getloadavg.c getmntinfo.c \
	getttyent.c getttynam.c getusershell.c getwd.c \
	initgroups.c isatty.c isctype.c malloc.c mktemp.c ndbm.c nlist.c \
	opendir.c perror.c popen.c psignal.c qsort.c random.c readdir.c \
	regex.c scandir.c seekdir.c \
	setmode.c sethostname.c \
	setenv.c siglist.c signal.c siginterrupt.c \
	sigsetops.c \
	sleep.c strcasecmp.c strftime.c swab.c sysctl.c syslog.c system.c \
	swapdevname.c \
	syserrlst.c \
	telldir.c time.c timezone.c ttyname.c ttyslot.c ualarm.c usleep.c \
	valloc.c strdup.c uname.c wait.c wait3.c waitpid.c
STDOBJ=	abort.o alarm.o atoi.o atol.o calloc.o closedir.o crypt.o \
	ctime.o ctype_.o daemon.o devname.o disklabel.o err.o \
	execvp.o \
	fstab.o gcvt.o getenv.o getgrent.o getgrgid.o getgrnam.o getlogin.o \
	getgrouplist.o gethostname.o getpagesize.o \
	getpass.o getpwent.o getloadavg.o getmntinfo.o \
	getttyent.o getttynam.o getusershell.o getwd.o \
	initgroups.o isatty.o isctype.o malloc.o mktemp.o ndbm.o nlist.o \
	opendir.o perror.o popen.o psignal.o qsort.o random.o readdir.o \
	regex.o scandir.o seekdir.o \
	setmode.o sethostname.o \
	setenv.o siglist.o signal.o siginterrupt.o \
	sigsetops.o \
	sleep.o strcasecmp.o strftime.o swab.o sysctl.o syslog.o system.o \
	swapdevname.o \
	syserrlst.o \
	telldir.o time.o timezone.o ttyname.o ttyslot.o ualarm.o usleep.o \
	valloc.o strdup.o uname.o wait.o wait3.o waitpid.o

TAGSFILE=tags

# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, where the PDP-11's profiled one
# went in profiled, and the small's left here; each model has a library
# of its own.  The PDP-11's ld -x -r, which took out the local symbols,
# is not done: the 286's ld makes nothing but a program.

.c.o:
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

genlib genlib_m genlib_l: ${OBJS}
	@echo "building medium model genlib"
	@cd m; ar cru ../genlib_m ${OBJS}
	@echo "building large model genlib"
	@cd l; ar cru ../genlib_l ${OBJS}
	@echo "building small model genlib"
	@ar cru genlib ${OBJS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f genlib genlib_m genlib_l *.o m/*.o l/*.o tags Makefile.bak

depend:	${SRCS}
	mkdep ${CFLAGS} ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
