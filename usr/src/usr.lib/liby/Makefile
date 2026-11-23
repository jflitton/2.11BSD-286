#
# Copyright (c) 1985 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.2 (2.11BSD) 2025/11/25
#
# Make a Yacc library
#
# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, and the small's left here; each
# model has a library of its own, which is installed where cc links a
# program of that model with it, /usr/lib, /usr/lib/m and /usr/lib/l.
#
CFLAGS=-O

.c.o:
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

liby.a:	libmai.o libzer.o
	ar rv liby.a libmai.o libzer.o
	ranlib liby.a
	cd m; ar rv ../liby_m.a libmai.o libzer.o
	ranlib liby_m.a
	cd l; ar rv ../liby_l.a libmai.o libzer.o
	ranlib liby_l.a

install: liby.a
	install -c -m 644 -o root liby.a /usr/lib/liby.a
	ranlib /usr/lib/liby.a
	install -c -m 644 -o root liby_m.a /usr/lib/m/liby.a
	ranlib /usr/lib/m/liby.a
	install -c -m 644 -o root liby_l.a /usr/lib/l/liby.a
	ranlib /usr/lib/l/liby.a

clean:
	rm -f *.o m/*.o l/*.o *.a errs
