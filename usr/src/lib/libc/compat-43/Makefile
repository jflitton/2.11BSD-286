#
# Public domain 1997/8/28
#
#	@(#)Makefile	1.1 (2.11BSD) 1997/11/30
#
CFLAGS=	-O ${DEFS}
SRCS= creat.c gethostid.c sethostid.c sigcompat.c \
	setregid.c setreuid.c setrgid.c setruid.c
OBJS= creat.o gethostid.o sethostid.o sigcompat.o \
	setregid.o setreuid.o setrgid.o setruid.o

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

all: compat-43lib compat-43lib_m compat-43lib_l

compat-43lib compat-43lib_m compat-43lib_l: ${OBJS}
	@echo "building medium model compat-43lib"
	@cd m; ar cru ../compat-43lib_m ${OBJS}
	@echo "building large model compat-43lib"
	@cd l; ar cru ../compat-43lib_l ${OBJS}
	@echo "building small model compat-43lib"
	@ar cru compat-43lib ${OBJS}

clean:
	rm -f compat-43lib compat-43lib_m compat-43lib_l *.o m/*.o l/*.o Makefile.bak tags

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

# DO NOT DELETE THIS LINE -- make depend uses it
