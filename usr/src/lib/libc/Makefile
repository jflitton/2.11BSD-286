#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	4.19.4 (2.11BSD) 2025/11/25
#
# The system has two monitoring schemes, utilizing either ``prof''
# or ``gprof''. DFLMON determines the default profiler; mon.o
# specifies that ``prof'' is the default, gmon.o specifies that
# ``gprof'' is the default.
#
# Machine dependent routines are located in a subtree which parallels
# the top directories.  This subtree is identified by the machine name.
#
# Compatibility routines are kept in directories with a prefixing
# ``compat'' (so they all sort together).
# 
# The C run-time startup code is always machine dependent and expected
# to be located in ${MACHINE}/csu
#
# All files contain sccsid strings, but these are not compiled into
# library objects by default, as a space-saving measure.  To produce
# a library that contains these strings in every object except
# system call stubs, add -DLIBC_SCCS to DEFS below; to put these
# strings into system call stubs, use -DSYSLIBC_SCCS.
#
# To compile a non-floating point versions of some standard library
# routines add -DNONFP.  This will speed up some operations if you don't
# have hardware floating point.
#
# There are two possible methods of doing host name look up.  They are
# to use the name server or /etc/hosts. HOSTLOOKUP defines which method
# is to be compiled into libc  :
#								     
#	defining HOSTLOOKUP to be "named", compiles the         
#	host lookup routines that use the BIND name server.
#
#	defining HOSTLOOKUP to be "hosttable", compiles
#	the host lookup routines that use /etc/hosts.
#
HOSTLOOKUP=	named
#HOSTLOOKUP=	hosttable
#							
# The 80286: its machine dependent routines are in i286, and the
# library is made for each of its three models, as for the PDP-11 it
# was made for profiling: libc.a for the small model, libc_m.a for the
# medium, libc_l.a for the large, each of what the directories made for
# that model (their Makefiles), and installed where cc links a program
# of that model with it, /lib, /lib/m and /lib/l.  It has no profiling,
# and so no monitor.  exit() and printf each have two of a function to
# choose from by the order of the library, and the 286 has one of each
# (gen, stdio): not fakcu.o and fpfcom.o, nor their moves after exit.o
# and vfprintf.o.
#
INSTALL=install -m 644
MACHINE=i286
DEFS=
COMPAT=	compat-4.1 compat-sys5 compat-43
ALL=	gen inet net ns stdio stdlib string ${MACHINE} ${COMPAT}
TAGSFILE=tags

libc.a libc_m.a libc_l.a: ${MACHINE}/csu ${ALL} net/${HOSTLOOKUP}
	rm -rf tmp
	mkdir tmp
	cd tmp; \
		for i in ${ALL}; do ar x ../$$i/$${i}lib; done; \
		ar x ../net/${HOSTLOOKUP}/hostlib; \
		ar cr ../libc.a `lorder *.o | tsort`
	ar ma findfp.o libc.a exit.o
	ar ma pfcom.o libc.a vfprintf.o
	rm -rf tmp
	mkdir tmp
	cd tmp; \
		for i in ${ALL}; do ar x ../$$i/$${i}lib_m; done; \
		ar x ../net/${HOSTLOOKUP}/hostlib_m; \
		ar cr ../libc_m.a `lorder *.o | tsort`
	ar ma findfp.o libc_m.a exit.o
	ar ma pfcom.o libc_m.a vfprintf.o
	rm -rf tmp
	mkdir tmp
	cd tmp; \
		for i in ${ALL}; do ar x ../$$i/$${i}lib_l; done; \
		ar x ../net/${HOSTLOOKUP}/hostlib_l; \
		ar cr ../libc_l.a `lorder *.o | tsort`
	ar ma findfp.o libc_l.a exit.o
	ar ma pfcom.o libc_l.a vfprintf.o
	rm -rf tmp

${MACHINE}/csu ${ALL} net/${HOSTLOOKUP}: FRC
	cd $@; make ${MFLAGS} DEFS=${DEFS}

FRC:
	@sleep 1

install: libc.a libc_m.a libc_l.a
	${INSTALL} libc.a /lib/libc.a
	ranlib /lib/libc.a
	${INSTALL} libc_m.a /lib/m/libc.a
	ranlib /lib/m/libc.a
	${INSTALL} libc_l.a /lib/l/libc.a
	ranlib /lib/l/libc.a
	cd ${MACHINE}/csu; make install

tags:
	for i in ${ALL}; do \
		(cd $$i; make ${MFLAGS} TAGSFILE=../${TAGSFILE} tags); \
	done
	cd net/${HOSTLOOKUP}; \
		make ${MFLAGS} TAGSFILE=../../${TAGSFILE} tags

clean:
	for i in ${MACHINE}/csu ${ALL} net/hosttable net/named; \
		do (cd $$i; make ${MFLAGS} clean); done
	rm -rf tmp libc.a libc_m.a libc_l.a

depend:
	for i in ${MACHINE}/csu ${ALL} net/hosttable net/named; \
		do (cd $$i; make ${MFLAGS} DEFS=${DEFS} depend); done
