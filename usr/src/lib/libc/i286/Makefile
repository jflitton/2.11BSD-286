#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.6.2 (2.11BSD) 2025/11/29
#
# Machine dependent routines for the 286 are located here, as the
# PDP-11's are in ../pdp, whose Makefile this is, and they are made for
# each of the 286's three models: a library for the small, i286lib, the
# medium, i286lib_m, and the large, i286lib_l.  ../Makefile puts each
# in that model's C library, and in the large model's, gen's malloc.o
# in place of ../gen's.  The start-up code, csu, ../Makefile makes and
# installs.  GNUmakefile is the C library of each model as the system
# is made where it is built, which GNU make reads before this.
#
#	crt	the routines compiled code calls for long arithmetic
#	gen	setjmp and longjmp, alloca, and the PDP-11's floating-point
#		routines; nlist's reading of this system's objects
#	net	byte order
#	sys	the system call stubs, made from sys/syscalls.tab by
#		sys/mkstubs.c, and the calls that need more than a stub
#
ALL=	crt gen net sys
TAGSFILE=tags

i286lib i286lib_m i286lib_l: ${ALL}
	-mkdir tmp
	for i in ${ALL}; do (cd tmp; ar x ../$$i/$${i}lib); done
	ar cr i286lib `ls tmp/*.o | sort -t/ +1`
	rm -f tmp/*
	for i in ${ALL}; do (cd tmp; ar x ../$$i/$${i}lib_m); done
	ar cr i286lib_m `ls tmp/*.o | sort -t/ +1`
	rm -f tmp/*
	for i in ${ALL}; do (cd tmp; ar x ../$$i/$${i}lib_l); done
	ar cr i286lib_l `ls tmp/*.o | sort -t/ +1`
	rm -rf tmp

${ALL}:	FRC
	cd $@; make ${MFLAGS} DEFS=${DEFS}

FRC:

clean:
	for i in ${ALL} csu; do (cd $$i; make ${MFLAGS} clean); done
	rm -f i286lib i286lib_m i286lib_l tmp/*.o;
	-rmdir tmp
