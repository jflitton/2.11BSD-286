#	Makefile	1.7 (2.11BSD) 2025/12/25
#
CFLAGS=	-O ${DEFS}
# strchr and strrchr.c are not used.  The assembly versions in pdp/gen are
# used instead.
# The 80286 has them: its index and rindex are ../gen's C, which has
# not the other names that the PDP-11's assembly gives them.
SRCS=	memccpy.c memchr.c memcmp.c memcpy.c memset.c strchr.c strrchr.c
OBJS=	memccpy.o memchr.o memcmp.o memcpy.o memset.o strchr.o strrchr.o
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

compat-sys5lib compat-sys5lib_m compat-sys5lib_l: ${OBJS}
	@echo "building medium model compat-sys5lib"
	@cd m; ar cru ../compat-sys5lib_m ${OBJS}
	@echo "building large model compat-sys5lib"
	@cd l; ar cru ../compat-sys5lib_l ${OBJS}
	@echo "building small model compat-sys5lib"
	@ar cru compat-sys5lib ${OBJS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f compat-sys5lib compat-sys5lib_m compat-sys5lib_l *.o m/*.o l/*.o Makefile.bak

depend:
	mkdep ${CFLAGS} ${SRCS}
