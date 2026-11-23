#	@(#)Makefile	4.3	(2.11BSD) 2025/11/25
#
# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, where the PDP-11's profiled one
# went in profiled, and the small's left here; each model has a library
# of its own, which is installed where cc links a program of that model
# with it, /usr/lib, /usr/lib/m and /usr/lib/l.  The PDP-11's ld -x -r,
# which took out the local symbols, is not done: the 286's ld makes
# nothing but a program.
#

CFLAGS=-O

SRCS=	asin.c atan.c hypot.c jn.c j0.c j1.c pow.c gamma.c \
	log.c sin.c sqrt.c tan.c tanh.c sinh.c erf.c exp.c floor.c

FILES= asin.o atan.o hypot.o jn.o j0.o j1.o pow.o gamma.o \
	log.o sin.o sqrt.o tan.o tanh.o sinh.o erf.o exp.o floor.o

TAGSFILE=tags

.c.o:
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

libom.a libom_m.a libom_l.a : $(FILES)
	cd m; ar cru ../libom_m.a $(FILES)
	cd l; ar cru ../libom_l.a $(FILES)
	ar cru libom.a $(FILES)

install: libom.a libom_m.a libom_l.a
	install -c -m 644 -o root libom.a /usr/lib
	ranlib /usr/lib/libom.a
	install -c -m 644 -o root libom_m.a /usr/lib/m/libom.a
	ranlib /usr/lib/m/libom.a
	install -c -m 644 -o root libom_l.a /usr/lib/l/libom.a
	ranlib /usr/lib/l/libom.a

tags:
	cwd=`pwd`; \
 	for i in ${SRCS}; do \
 		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
 	done

clean:
	rm -f *.o m/*.o l/*.o libom.a libom_m.a libom_l.a tags
