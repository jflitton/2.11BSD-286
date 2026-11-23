#
#	@(#)Makefile	1.2 (2.11BSD) 2025/11/29
#
# This is the Makefile for the 'string' functions.  New routines ported from 
# 4.4BSD's libc/string directory go here but existing libc/gen files are 
# being left where they are.
#

DEFS=
CFLAGS=	 -O ${DEFS}
CC=	cc

SRCS=	strcspn.c strpbrk.c strerror.c strsep.c strspn.c strstr.c strtok.c
OBJS=	strcspn.o strpbrk.o strerror.o strsep.o strspn.o strstr.o strtok.o

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

all: stringlib stringlib_m stringlib_l

stringlib stringlib_m stringlib_l: ${OBJS}
	@echo "building medium model stringlib"
	@cd m; ar cru ../stringlib_m ${OBJS}
	@echo "building large model stringlib"
	@cd l; ar cru ../stringlib_l ${OBJS}
	@echo "building small model stringlib"
	@ar cru stringlib ${OBJS}

clean:
	rm -f ${OBJS} m/*.o l/*.o tags Makefile.bak stringlib stringlib_m stringlib_l

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
