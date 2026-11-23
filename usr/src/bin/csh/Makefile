#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley Software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.5 (2.11BSD) 2026/4/25
#
# C Shell with process control; VM/UNIX VAX Makefile
# Bill Joy UC Berkeley; Jim Kulp IIASA, Austria
#
# To profile, put -DPROF in DEFS and -pg in CFLAGS, and recompile.

# The 80286: csh is of the medium model, its code being more than a
# segment of 64K, where the PDP-11 had it in overlays.
DEFS=	-DTELL -DVFORK -DFILEC
MODEL=	-mm
CFLAGS=	$(DEFS) -O ${MODEL}
LDFLAGS=-i ${MODEL}
XSTR=	/usr/ucb/xstr
CTAGS=	/usr/ucb/ctags
LIBES=  -lc

BASE=	alloc11.o printf.o sh.char.o sh.dir.o sh.dol.o \
	sh.err.o sh.exp.o sh.file.o sh.func.o sh.glob.o sh.hist.o \
	sh.init.o sh.lex.o sh.parse.o sh.print.o sh.proc.o sh.sem.o \
	sh.set.o
OV1= 	sh.o sh.exec.o sh.time.o
OV2=	sh.misc.o sh.exec2.o

OBJS=	${BASE} ${OV1} ${OV2}

# Special massaging of C files for sharing of strings
.c.o:
	${CC} -E ${CFLAGS} $*.c | ${XSTR} -c -
	${CC} -c ${CFLAGS} x.c 
	mv -f x.o $*.o
	rm -f x.c

# strings.o must be last since it can change when previous files compile
csh: ${OBJS} strings.o
	rm -f csh
	${CC} ${LDFLAGS} ${BASE} ${OV1} ${OV2} strings.o -o csh

strings.o: strings
	${XSTR}
	${CC} ${MODEL} -c xs.c
	mv -f xs.o strings.o
	rm -f xs.c

install: csh
	install -m 755 -o bin -g bin -s csh /bin/csh

clean:
	rm -f a.out strings x.c xs.c csh errs *.o

tags:	*.h sh*.c
	${CTAGS} -t *.h sh*.c
