#
# The routines that compiled code calls for long arithmetic, and the
# conventions of the C library's assembly (DEFS.inc), for the 286.  As
# the PDP-11's ../../pdp/crt, and each made for the three models: the
# small's here, the medium's in m and the large's in l, with -DFAR for
# both, which call far, and -DFARDATA for the large, whose pointers are
# far.  The large model has pia too: arithmetic on __huge pointers.
#
SRCS=	chp.s i4d.s i4m.s pia.s
OBJS=	chp.o i4d.o i4m.o
AS=	as

.s.o:
	${AS} -mm -DFAR -o m/$*.o $*.s
	${AS} -ml -DFAR -DFARDATA -o l/$*.o $*.s
	${AS} -ms -o $*.o $*.s

crtlib crtlib_m crtlib_l: ${OBJS} l/pia.o
	@echo "building medium model crtlib"
	@cd m; ar cru ../crtlib_m ${OBJS}
	@echo "building large model crtlib"
	@cd l; ar cru ../crtlib_l ${OBJS} pia.o
	@echo "building small model crtlib"
	@ar cru crtlib ${OBJS}

l/pia.o: pia.s DEFS.inc
	${AS} -ml -DFAR -DFARDATA -o l/pia.o pia.s

clean:
	rm -f *.o m/*.o l/*.o crtlib crtlib_m crtlib_l

chp.o: chp.s DEFS.inc
i4d.o: i4d.s DEFS.inc
i4m.o: i4m.s DEFS.inc
