#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.5 (2.11BSD) 2025/11/29

OBJS=	gethnamadr.o gethostent.o
SRCS=	gethnamadr.c gethostent.c
CFLAGS=	-O ${DEFS}

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

hostlib hostlib_m hostlib_l: ${OBJS}
	@echo "building medium model hostlib"
	@cd m; ar cru ../hostlib_m ${OBJS}
	@echo "building large model hostlib"
	@cd l; ar cru ../hostlib_l ${OBJS}
	@echo "building small model hostlib"
	@ar cru hostlib ${OBJS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f *.o errs a.out core hostlib hostlib_m hostlib_l m/*.o l/*.o \
		tags Makefile.bak

depend:	${SRCS}
	mkdep ${CFLAGS} ${SRCS}
