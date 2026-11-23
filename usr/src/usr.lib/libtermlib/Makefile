#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.2 (2.11BSD) 2025/11/27
#
# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, where the PDP-11's profiled one
# went in profiled, and the small's left here; each model has a library
# of its own, which is installed where cc links a program of that model
# with it, /usr/lib, /usr/lib/m and /usr/lib/l.  The PDP-11's ld -x -r,
# which took out the local symbols, is not done: the 286's ld makes
# nothing but a program.
#
CFLAGS=	-O -DCM_N -DCM_GT -DCM_B -DCM_D -DNOSCCS
LD = ld
SRCS=	termcap.c tgoto.c tputs.c
OBJS=	termcap.o tgoto.o tputs.o
TAGSFILE=tags

.c.o:
	$(CC) $(CFLAGS) -c -mm $*.c
	mv $*.o m/$*.o
	$(CC) $(CFLAGS) -c -ml $*.c
	mv $*.o l/$*.o
	$(CC) $(CFLAGS) -c $*.c

termcap.a termcap_m.a termcap_l.a: ${OBJS}
	ar cr termcap.a ${OBJS}
	cd m; ar cr ../termcap_m.a ${OBJS}
	cd l; ar cr ../termcap_l.a ${OBJS}

install: termcap.a termcap_m.a termcap_l.a
	install -c -m 644 -o root termcap.a /usr/lib/libtermcap.a
	-rm -f /usr/lib/libtermlib.a
	ln /usr/lib/libtermcap.a /usr/lib/libtermlib.a
	ranlib /usr/lib/libtermcap.a
	install -c -m 644 -o root termcap_m.a /usr/lib/m/libtermcap.a
	-rm -f /usr/lib/m/libtermlib.a
	ln /usr/lib/m/libtermcap.a /usr/lib/m/libtermlib.a
	ranlib /usr/lib/m/libtermcap.a
	install -c -m 644 -o root termcap_l.a /usr/lib/l/libtermcap.a
	-rm -f /usr/lib/l/libtermlib.a
	ln /usr/lib/l/libtermcap.a /usr/lib/l/libtermlib.a
	ranlib /usr/lib/l/libtermcap.a

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	-rm -f *.o m/*.o l/*.o
	-rm -f termcap.a termcap_m.a termcap_l.a
