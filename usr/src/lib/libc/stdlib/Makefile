#
#	@(#)Makefile	1.2 (2.11BSD) 2026/11/26
#
# This is the Makefile for 'stdlib'.  New routines ported from 4.4BSD's
# libc/stdlib directory go here but existing libc/gen files are being left
# where they are.
#

DEFS=
CFLAGS=	 -O ${DEFS}
CC=	cc

SRCS=	getopt.c getsubopt.c strtol.c strtoul.c imaxabs.c imaxdiv.c lconv.c \
	localeconv.c setlocale.c strtomax.c
OBJS=	getopt.o getsubopt.o strtol.o strtoul.o imaxabs.o imaxdiv.o lconv.o \
	localeconv.o setlocale.o strtomax.o

# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, and the small's left here; and
# for profiling, which is the small model's alone, with cc -p into
# profiled, as the PDP-11's; each has a library of its own.  The
# PDP-11's ld -x -r, which took out the local symbols, is not done: the
# 286's ld makes nothing but a program.

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	mv $*.o profiled/$*.o
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

all: stdliblib stdliblib_p stdliblib_m stdliblib_l

stdliblib stdliblib_p stdliblib_m stdliblib_l: ${OBJS}
	@echo "building profiled stdliblib"
	@cd profiled; ar cru ../stdliblib_p ${OBJS}
	@echo "building medium model stdliblib"
	@cd m; ar cru ../stdliblib_m ${OBJS}
	@echo "building large model stdliblib"
	@cd l; ar cru ../stdliblib_l ${OBJS}
	@echo "building small model stdliblib"
	@ar cru stdliblib ${OBJS}

clean:
	rm -f ${OBJS} profiled/*.o m/*.o l/*.o tags Makefile.bak stdliblib stdliblib_p stdliblib_m stdliblib_l

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
