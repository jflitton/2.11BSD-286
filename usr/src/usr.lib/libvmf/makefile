#
#       @(#)Makefile    1.2 (2.11BSD) 2025/12/8
#
# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, where the PDP-11's profiled one
# went in profiled, and the small's left here; each model has a library
# of its own, which is installed where cc links a program of that model
# with it, /usr/lib, /usr/lib/m and /usr/lib/l.  The PDP-11's ld -X -r
# is not done: the 286's ld makes nothing but a program.
#
OBJECTS = vmf.o

all:  genlib genlib_m genlib_l vmf.0

CFLAGS = -O

all:   genlib genlib_m genlib_l

vmf.0:	vmf.3
	nroff -man vmf.3 > vmf.0

install:
	install -c -m 644 genlib /usr/lib/libvmf.a
	ranlib /usr/lib/libvmf.a
	install -c -m 644 genlib_m /usr/lib/m/libvmf.a
	ranlib /usr/lib/m/libvmf.a
	install -c -m 644 genlib_l /usr/lib/l/libvmf.a
	ranlib /usr/lib/l/libvmf.a
	install -c -o root -g staff -m 444 vmf.0 /usr/man/cat3/vmf.0

clean:
	rm -f ${OBJECTS} m/${OBJECTS} l/${OBJECTS} genlib genlib_m genlib_l vmf.0

.c.o:
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

genlib genlib_m genlib_l: ${OBJECTS}
	@echo "building medium model genlib"
	@cd m; ar cru ../genlib_m ${OBJECTS}
	@echo "building large model genlib"
	@cd l; ar cru ../genlib_l ${OBJECTS}
	@echo "buiding normal genlib"
	@ar cru genlib ${OBJECTS}
