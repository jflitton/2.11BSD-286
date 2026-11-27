#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.6.5 (2.11BSD) 2026/11/26
#
#	crt0	Normal C run time startoff
#	mcrt0	C run time start off for profiling, ``prof'' conventions
#
# The 286's, after the PDP-11's ../../pdp/csu: crt0 for each model, the
# small's here, the medium's in m and the large's in l, which cc links a
# program of that model with first, from /lib, /lib/m and /lib/l.
# Profiling is the small model's alone, and mcrt0 is that model's.  The
# 286's ld does not join objects into one, as the PDP-11's -r did
# moncrt0.o and mon.o into mcrt0.o: mon.o is in the small model's C
# library, and in libc_p.a, which cc -p links a program with, as the
# PDP-11's is (../../Makefile), with mcount.o, which is a file of its
# own here and not put after mon.o's assembly.
#
SRCS=	crt0.s mon.c mcount.s
OBJS=	crt0.o mcrt0.o mon.o mcount.o
CFLAGS=	-O ${DEFS}
AS=	as

all: ${OBJS}

install: ${OBJS}
	install -c -m 0644 crt0.o /lib/crt0.o
	install -c -m 0644 m/crt0.o /lib/m/crt0.o
	install -c -m 0644 l/crt0.o /lib/l/crt0.o
	install -c -m 0644 mcrt0.o /lib/mcrt0.o

crt0.o:	crt0.s ../crt/DEFS.inc
	${AS} -mm -DFAR -I../crt -o m/crt0.o crt0.s
	${AS} -ml -DFAR -DFARDATA -I../crt -o l/crt0.o crt0.s
	${AS} -ms -I../crt -o crt0.o crt0.s

mcrt0.o: crt0.s ../crt/DEFS.inc
	${AS} -ms -DMCRT0 -I../crt -o mcrt0.o crt0.s

mon.o: mon.c
	${CC} ${CFLAGS} -c mon.c

mcount.o: mcount.s ../crt/DEFS.inc
	${AS} -ms -I../crt -o mcount.o mcount.s

clean:
	rm -f ${OBJS} m/*.o l/*.o
