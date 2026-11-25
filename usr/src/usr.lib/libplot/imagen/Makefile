# Copyright (c) 1985 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.6 (2.11BSD) 2025/11/27
#
CFLAGS=	-O
OBJS=	arc.o box.o charset.o circle.o close.o cont.o dot.o \
	erase.o label.o line.o linemod.o move.o open.o point.o \
	scale.o space.o

# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, and the small's left here, and
# each model has a library of its own (../Makefile).
.c.o:
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

# default printer resolution dpi
DEFRES=	240

all:	../libplotimagen

../libplotimagen: ${OBJS}
	ar ru ../libplotimagen ${OBJS}
	cd m; ar ru ../../libplotimagen_m ${OBJS}
	cd l; ar ru ../../libplotimagen_l ${OBJS}

close.o erase.o label.o line.o open.o: imp.h imPcodes.h
cont.o move.o:	imp.h

space.o: space.c Makefile
	${CC} -mm -c ${CFLAGS} -DDEFRES=${DEFRES} space.c
	mv space.o m/space.o
	${CC} -ml -c ${CFLAGS} -DDEFRES=${DEFRES} space.c
	mv space.o l/space.o
	${CC} -c ${CFLAGS} -DDEFRES=${DEFRES} space.c

clean:
	rm -f ${OBJS} m/*.o l/*.o a.out core errs

install:
