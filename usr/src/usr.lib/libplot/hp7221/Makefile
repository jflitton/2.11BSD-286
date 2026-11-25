# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.2 (2.11BSD) 1/2/94
#
#
CFLAGS=	-O
OBJS=	arc.o box.o circle.o close.o cont.o dot.o erase.o label.o \
	line.o linemod.o move.o open.o point.o space.o subr.o

# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, and the small's left here, and
# each model has a library of its own (../Makefile).
.c.o:
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

../libplot7221:	${OBJS}
	ar ru ../libplot7221 ${OBJS}
	cd m; ar ru ../../libplot7221_m ${OBJS}
	cd l; ar ru ../../libplot7221_l ${OBJS}

${OBJS}:	hp7221.h

clean:
	rm -f ${OBJS} m/*.o l/*.o errs a.out core

install:
