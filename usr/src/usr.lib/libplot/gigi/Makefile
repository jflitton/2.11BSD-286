# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.3 (2.11BSD) 2025/11/27
#
#
CFLAGS=	-O
OBJS=	arc.o box.o circle.o close.o cont.o dot.o erase.o label.o \
	line.o linemod.o move.o open.o point.o space.o

# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, and the small's left here, and
# each model has a library of its own (../Makefile).
.c.o:
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

../libplotgigi:	${OBJS}
	ar ru ../libplotgigi ${OBJS}
	cd m; ar ru ../../libplotgigi_m ${OBJS}
	cd l; ar ru ../../libplotgigi_l ${OBJS}

${OBJS}:	gigi.h

clean:
	rm -f ${OBJS} m/*.o l/*.o errs a.out core

install:
