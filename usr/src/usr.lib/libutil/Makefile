#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such redistribution and
# use acknowledge that the software was developed by the University
# of California, Berkeley.  The name of the University may not be
# used to endorse or promote products derived from this software
# without specific prior written permission.  THIS SOFTWARE IS PROVIDED
# ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
# WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND
# FITNESS FOR A PARTICULAR PURPOSE.
#
# @(#)Makefile	5.4.2 (2.11BSD) 2025/11/27
#
# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, where the PDP-11's profiled one
# went in profiled, and the small's left here; each model has a library
# of its own, which is installed where cc links a program of that model
# with it, /usr/lib, /usr/lib/m and /usr/lib/l.  The PDP-11's ld -x -r,
# which took out the local symbols, is not done: the 286's ld makes
# nothing but a program.

CFLAGS=	-O
SRCS=	login.c logout.c logwtmp.c dkcksum.c
OBJS=	login.o logout.o logwtmp.o dkcksum.o

.c.o:
	@${CC} -mm ${CFLAGS} -c $*.c
	@mv $*.o m/$*.o
	@${CC} -ml ${CFLAGS} -c $*.c
	@mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

all: libutil.a libutil_m.a libutil_l.a

libutil.a libutil_m.a libutil_l.a: ${OBJS}
	@echo building small model libutil
	@ar ru libutil.a ${OBJS}
	ranlib libutil.a
	@echo building medium model libutil
	@cd m; ar ru ../libutil_m.a ${OBJS}
	ranlib libutil_m.a
	@echo building large model libutil
	@cd l; ar ru ../libutil_l.a ${OBJS}
	ranlib libutil_l.a

clean:
	rm -f ${OBJS} m/*.o l/*.o libutil.a libutil_m.a libutil_l.a

cleandir: clean
	rm -f .depend

depend:
	mkdep ${CFLAGS} ${SRCS}

install: all
	install -c -o root -g staff -m 644 libutil.a /usr/lib
	ranlib -t /usr/lib/libutil.a
	install -c -o root -g staff -m 644 libutil_m.a /usr/lib/m/libutil.a
	ranlib -t /usr/lib/m/libutil.a
	install -c -o root -g staff -m 644 libutil_l.a /usr/lib/l/libutil.a
	ranlib -t /usr/lib/l/libutil.a

tags:
	ctags ${SRCS}
