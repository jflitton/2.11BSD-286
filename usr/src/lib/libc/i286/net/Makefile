#
# Byte order for the 286, after the PDP-11's ../../pdp/net: htons and
# ntohs, htonl and ntohl, made for the three models, the small's here,
# the medium's in m and the large's in l (../crt/DEFS.inc); and in
# profiled, for profiling, the small's as it is.
#
SRCS=	htons.s
OBJS=	htons.o
AS=	as

.s.o:
	${AS} -mm -DFAR -I../crt -o m/$*.o $*.s
	${AS} -ml -DFAR -DFARDATA -I../crt -o l/$*.o $*.s
	${AS} -ms -I../crt -o $*.o $*.s
	cp $*.o profiled/$*.o

netlib netlib_p netlib_m netlib_l: ${OBJS}
	@echo "building profiled netlib"
	@cd profiled; ar cru ../netlib_p ${OBJS}
	@echo "building medium model netlib"
	@cd m; ar cru ../netlib_m ${OBJS}
	@echo "building large model netlib"
	@cd l; ar cru ../netlib_l ${OBJS}
	@echo "building small model netlib"
	@ar cru netlib ${OBJS}

clean:
	rm -f *.o profiled/*.o m/*.o l/*.o netlib netlib_p netlib_m netlib_l

htons.o: htons.s ../crt/DEFS.inc
