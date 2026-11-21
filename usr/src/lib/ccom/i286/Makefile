# The C compiler's second pass for the 80286.
#
# ../c0, the first pass, is the PDP-11 compiler's, with the few things
# that differ here under #ifdef i286.  c1, the second, is this
# directory's: ../c1's reader of the first pass's output and its
# optimizer of expressions, and a code generator of its own in place
# of ../c1's and its tables (c10.c).  It writes assembly for NASM.
#
# From the top of the repository:
#
#	make -C usr/src/lib/ccom/i286 host
#
# makes obj/host/ccom/c0 and obj/host/ccom/c1: both passes compiled by
# and for the machine the system is built on, where tools/ncc runs
# them.  They are for working on the compiler, whose output goes
# through the cross tools from there and is run by tests/ccom.  It
# makes obj/host/ccom/c2 with them, the object code improver
# (../../c2/i286), which is run on what c1 writes.  GNU make.
#
# The passes are written for a machine whose int is 16 bits, whose long
# is 32 and whose arguments are all on the stack.  Both are given a
# type of 32 bits for their long constants (LTYPE).  The second keeps
# its other constants to 16 bits whatever it is compiled with (W, in
# c1.h).  The first does not; on this machine it may differ from itself
# on the 286 where a constant overflows.  And its outcode, and what
# prints its messages, are replaced, by hostout.c.

TOP:=	$(abspath ../../../../..)
SRC=	..
H=	$(TOP)/obj/host/ccom
HOSTCC=	cc
HOSTCFLAGS= -std=gnu89 -w -fno-strict-aliasing -fcommon -fsigned-char \
	-Wno-error=return-mismatch -Wno-error=return-type \
	-Wno-error=implicit-function-declaration -Wno-error=implicit-int \
	-Wno-error=int-conversion -Wno-error=incompatible-pointer-types \
	-Di286 -DLTYPE=int

C0=	c00.c c02.c c03.c c05.c ansi.c
C1=	c10.c c11.c c12.c c13.c c14.c

host:	$(H)/c0 $(H)/c1
	@$(MAKE) --no-print-directory -C $(SRC)/../c2/i286 host

$(H):
	@mkdir -p $@

$(H)/c0: $(addprefix $(SRC)/,$(C0) c01.c c04.c c0.h) hostout.c | $(H)
	@echo hostcc c0
	@sed 's/^w*error(s, p1/upstream_&/' $(SRC)/c01.c > $(H)/c01.c
	@sed 's/^outcode(s, a)/upstream_outcode(s, a)/' $(SRC)/c04.c > $(H)/c04.c
	@$(HOSTCC) $(HOSTCFLAGS) -I$(SRC) -o $@ $(addprefix $(SRC)/,$(C0)) \
	    $(H)/c01.c $(H)/c04.c hostout.c

$(H)/c1: $(C1) c1.h | $(H)
	@echo hostcc c1
	@$(HOSTCC) $(HOSTCFLAGS) -o $@ $(C1)

clean:
	rm -rf $(H)
