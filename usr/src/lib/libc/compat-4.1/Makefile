#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.4.2 (2.11BSD) 2025/11/29
#
CFLAGS=	-O ${DEFS}
SRCS=ftime.c gtty.c nice.c pause.c rand.c stty.c \
	times.c utime.c
OBJS=	ftime.o gtty.o nice.o pause.o rand.o stty.o \
	times.o utime.o
TAGSFILE=tags

# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, where the PDP-11's profiled one
# went in profiled, and the small's left here; each model has a library
# of its own.  The PDP-11's ld -x -r, which took out the local symbols,
# is not done: the 286's ld makes nothing but a program.

.c.o:
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

compat-4.1lib compat-4.1lib_m compat-4.1lib_l: ${OBJS}
	@echo "building medium model compat-4.1lib"
	@cd m; ar cru ../compat-4.1lib_m ${OBJS}
	@echo "building large model compat-4.1lib"
	@cd l; ar cru ../compat-4.1lib_l ${OBJS}
	@echo "building small model compat-4.1lib"
	@ar cru compat-4.1lib ${OBJS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f compat-4.1lib compat-4.1lib_m compat-4.1lib_l *.o m/*.o l/*.o tags Makefile.bak

depend:	${SRCS}
	mkdep ${CFLAGS} ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

ftime.o: ftime.c /usr/include/sys/types.h /usr/include/sys/select.h
ftime.o: /usr/include/sys/types.h /usr/include/sys/time.h
ftime.o: /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/types.h
gtty.o: gtty.c /usr/include/sgtty.h /usr/include/sys/ioctl.h
gtty.o: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
nice.o: nice.c /usr/include/sys/time.h /usr/include/sys/types.h
nice.o: /usr/include/sys/select.h /usr/include/sys/types.h /usr/include/time.h
nice.o: /usr/include/sys/types.h /usr/include/sys/resource.h
pause.o: pause.c /usr/include/signal.h
rand.o: rand.c
stty.o: stty.c /usr/include/sgtty.h /usr/include/sys/ioctl.h
stty.o: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
times.o: times.c /usr/include/sys/time.h /usr/include/sys/types.h
times.o: /usr/include/sys/select.h /usr/include/sys/types.h /usr/include/time.h
times.o: /usr/include/sys/types.h /usr/include/sys/resource.h
times.o: /usr/include/sys/types.h /usr/include/sys/times.h
utime.o: utime.c /usr/include/sys/time.h /usr/include/sys/types.h
utime.o: /usr/include/sys/select.h /usr/include/sys/types.h /usr/include/time.h
utime.o: /usr/include/sys/types.h /usr/include/sys/types.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
