#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.4 (2.11BSD) 2025/11/27
#
# The 80286: the library is one for each of its three models, installed
# where cc links a program of that model with it, /usr/lib, /usr/lib/m
# and /usr/lib/l.  Where the PDP-11's is the object that ld -r -x
# makes, the 286's ld makes nothing but a program: it is a library of
# ar's, with the table of ranlib's that the 286's ld looks in.
#
CFLAGS=-O -DNOSCCS
TAGSFILE=tags

all: libdbm.a libdbm_m.a libdbm_l.a

libdbm.a: dbm.c
	${CC} -c ${CFLAGS} dbm.c
	ar cr libdbm.a dbm.o
	ranlib libdbm.a
	rm -f dbm.o

libdbm_m.a: dbm.c
	${CC} -mm -c ${CFLAGS} dbm.c
	ar cr libdbm_m.a dbm.o
	ranlib libdbm_m.a
	rm -f dbm.o

libdbm_l.a: dbm.c
	${CC} -ml -c ${CFLAGS} dbm.c
	ar cr libdbm_l.a dbm.o
	ranlib libdbm_l.a
	rm -f dbm.o

install: all
	install -c -m 644 libdbm.a /usr/lib
	install -c -m 644 libdbm_m.a /usr/lib/m/libdbm.a
	install -c -m 644 libdbm_l.a /usr/lib/l/libdbm.a
	-cmp -s dbm.h /usr/include/dbm.h || \
		install -c -m 444 dbm.h /usr/include

tags:
	ctags -a -f ${TAGSFILE} `pwd`/dbm.c

clean:
	rm -f *.o libdbm.a libdbm_m.a libdbm_l.a errs a.out core
