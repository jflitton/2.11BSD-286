#	Makefile	4.3	1/2/94
#
CFLAGS=	-O
OBJS=	arc.o box.o circle.o close.o cont.o dot.o erase.o label.o \
	line.o linmod.o move.o open.o point.o putsi.o space.o

# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, and the small's left here, and
# each model has a library of its own (../Makefile).
.c.o:
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

../libplot:	${OBJS}
	ar ru ../libplot ${OBJS}
	cd m; ar ru ../../libplot_m ${OBJS}
	cd l; ar ru ../../libplot_l ${OBJS}

clean:
	rm -f ${OBJS} m/*.o l/*.o errs a.out core

install:
