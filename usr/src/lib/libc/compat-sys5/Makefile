#	Makefile	1.8 (2.11BSD) 2026/11/27
#
CFLAGS=	-O ${DEFS}
# strchr and strrchr.c are not used.  The assembly versions in pdp/gen are
# used instead.
# The 80286 has them: its index and rindex have not the other names
# that the PDP-11's assembly gives them.  Its memcpy is in assembly, in
# ../i286/gen.
SRCS=	memccpy.c memchr.c memcmp.c memset.c strchr.c strrchr.c
OBJS=	memccpy.o memchr.o memcmp.o memset.o strchr.o strrchr.o
TAGSFILE=tags

# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, and the small's left here; and
# for profiling, which is the small model's alone, with cc -p into
# profiled, as the PDP-11's; each has a library of its own.  The
# PDP-11's ld -x -r, which took out the local symbols, is not done: the
# 286's ld makes nothing but a program.

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	mv $*.o profiled/$*.o
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

compat-sys5lib compat-sys5lib_p compat-sys5lib_m compat-sys5lib_l: ${OBJS}
	@echo "building profiled compat-sys5lib"
	@cd profiled; ar cru ../compat-sys5lib_p ${OBJS}
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
	rm -f compat-sys5lib compat-sys5lib_p compat-sys5lib_m compat-sys5lib_l *.o profiled/*.o m/*.o l/*.o Makefile.bak

depend:
	mkdep ${CFLAGS} ${SRCS}
