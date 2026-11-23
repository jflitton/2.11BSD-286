#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.12.5 (2.11BSD) 2025/11/29
#
CFLAGS=	-O
TAGSFILE=tags

# Subdirectories whose routines are included in the making of the
# master tags file (/usr/lib/tags); the Fortran libraries should
# be on this list.
#
TAGSDIR=libcurses libdbm libln libmp libtermlib libutil liberrlst

# Programs that live in subdirectories and have makefiles of their own.
#
# The 80286 has the libraries that its programs are linked with, each
# of its three models, the small's in /usr/lib, the medium's in
# /usr/lib/m and the large's in /usr/lib/l, where cc -mm and -ml look.

SUBDIR=	libcurses libdbm libln \
	libom libtermlib liby libutil \
	liberrlst

all:	${SUBDIR}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

FRC:

install: FRC
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} install); done
	rm -f /usr/lib/libm.a /usr/lib/m/libm.a /usr/lib/l/libm.a
	ln /usr/lib/libom.a /usr/lib/libm.a
	ln /usr/lib/m/libom.a /usr/lib/m/libm.a
	ln /usr/lib/l/libom.a /usr/lib/l/libm.a

tags:
	-for i in ${TAGSDIR}; do \
		(cd $$i; make ${MFLAGS} TAGSFILE=../${TAGSFILE} tags); \
	done

clean:
	rm -f a.out core *.s *.o tags
	-for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done

depend:
