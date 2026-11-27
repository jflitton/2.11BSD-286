#
# The 286's, after the PDP-11's ../../pdp/gen, each made for the three
# models: the small's here, the medium's in m and the large's in l, the
# assembly with -DFAR for both, which call far, and -DFARDATA for the
# large, whose pointers are far (../crt/DEFS.inc).  What the PDP-11 has
# in assembly of strings, bcopy and the like ../../gen has in C, which
# is the 286's; and its C, atof, ecvt and setjmperr, is the 286's too.
# nsym.c is nlist's reading of this system's objects.  The large
# model's malloc is lmalloc.c, which gets segments from the system
# where ../../gen's malloc.c moves the break: it is malloc.o in the
# large model's library, which ../Makefile puts in place of that one.
# For profiling, the small model's alone, the C is compiled with cc -p
# into profiled, and the assembly there is the small model's.
#
SRCS=	alloca.s ldexp.s modf.s setjmp.s nsym.c lmalloc.c
OBJS=	alloca.o ldexp.o modf.o setjmp.o nsym.o atof.o ecvt.o setjmperr.o
CFLAGS=	-O ${DEFS}
AS=	as
PDP=	../../pdp/gen

.s.o:
	${AS} -mm -DFAR -I../crt -o m/$*.o $*.s
	${AS} -ml -DFAR -DFARDATA -I../crt -o l/$*.o $*.s
	${AS} -ms -I../crt -o $*.o $*.s
	cp $*.o profiled/$*.o

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	mv $*.o profiled/$*.o
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

genlib genlib_p genlib_m genlib_l: ${OBJS} l/malloc.o
	@echo "building profiled genlib"
	@cd profiled; ar cru ../genlib_p ${OBJS}
	@echo "building medium model genlib"
	@cd m; ar cru ../genlib_m ${OBJS}
	@echo "building large model genlib"
	@cd l; ar cru ../genlib_l ${OBJS} malloc.o
	@echo "building small model genlib"
	@ar cru genlib ${OBJS}

atof.o: ${PDP}/atof.c
	${CC} -p ${CFLAGS} -c ${PDP}/atof.c
	mv atof.o profiled/atof.o
	${CC} -mm ${CFLAGS} -c ${PDP}/atof.c
	mv atof.o m/atof.o
	${CC} -ml ${CFLAGS} -c ${PDP}/atof.c
	mv atof.o l/atof.o
	${CC} ${CFLAGS} -c ${PDP}/atof.c

ecvt.o: ${PDP}/ecvt.c
	${CC} -p ${CFLAGS} -c ${PDP}/ecvt.c
	mv ecvt.o profiled/ecvt.o
	${CC} -mm ${CFLAGS} -c ${PDP}/ecvt.c
	mv ecvt.o m/ecvt.o
	${CC} -ml ${CFLAGS} -c ${PDP}/ecvt.c
	mv ecvt.o l/ecvt.o
	${CC} ${CFLAGS} -c ${PDP}/ecvt.c

setjmperr.o: ${PDP}/setjmperr.c
	${CC} -p ${CFLAGS} -c ${PDP}/setjmperr.c
	mv setjmperr.o profiled/setjmperr.o
	${CC} -mm ${CFLAGS} -c ${PDP}/setjmperr.c
	mv setjmperr.o m/setjmperr.o
	${CC} -ml ${CFLAGS} -c ${PDP}/setjmperr.c
	mv setjmperr.o l/setjmperr.o
	${CC} ${CFLAGS} -c ${PDP}/setjmperr.c

l/malloc.o: lmalloc.c
	${CC} -ml ${CFLAGS} -c lmalloc.c
	mv lmalloc.o l/malloc.o

clean:
	rm -f *.o profiled/*.o m/*.o l/*.o genlib genlib_p genlib_m genlib_l

alloca.o: alloca.s ../crt/DEFS.inc
ldexp.o: ldexp.s ../crt/DEFS.inc
modf.o: modf.s ../crt/DEFS.inc
setjmp.o: setjmp.s ../crt/DEFS.inc
