#	Makefile	4.4	1/2/94
#
CFLAGS=	-O
OBJS=	arc.o box.o circle.o close.o dot.o erase.o label.o \
	line.o linmod.o move.o open.o point.o space.o subr.o

# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, and the small's left here, and
# each model has a library of its own (../Makefile).
.c.o:
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

../lib300s:	${OBJS}
	ar ru ../lib300s ${OBJS}
	cd m; ar ru ../../lib300s_m ${OBJS}
	cd l; ar ru ../../lib300s_l ${OBJS}

clean:
	rm -f ${OBJS} m/*.o l/*.o errs a.out core

install:
