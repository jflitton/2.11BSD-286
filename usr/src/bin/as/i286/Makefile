# The assembler for the 80286.
#
# ../as is the PDP-11's, which is written in what it assembles.  This
# one is in C and reads the syntax of NASM, as much of it as the C
# compiler's second pass writes (../../lib/ccom/i286) and the C
# library's own assembly uses (../../lib/libc/i286): those were written
# for NASM before the system had an assembler, and either does for them
# still.  It writes the system's object files (a.out.h), which ld
# (../../ld/i286) makes programs of.  tests/as has it assemble what
# NASM does, and the two are to agree.
#
# From the top of the repository:
#
#	make -C usr/src/bin/as/i286 host
#
# makes obj/host/ccom/as, compiled by and for the machine the system is
# built on, where tools/ncc runs it on what the compiler writes.
# ../../lib/ccom/i286's host makes it too.  GNU make.
#
# The object file's header and its symbols are structures of shorts and
# longs, a long being 32 bits (a.out.h, nlist.h, sys/exec.h), which the
# assembler writes as it has them.  On that machine a long is not, and
# a pointer is more: obj/host/ccom/include has those headers for it, made
# here from the system's own with the types put right (hostinc).  It
# has to be a machine that keeps a number's low byte first, as the 286
# does.

TOP:=	$(abspath ../../../../..)
H=	$(TOP)/obj/host/ccom
HI=	$(TOP)/obj/host/ccom/include
INC=	$(TOP)/usr/include
HOSTCC=	cc
HOSTCFLAGS= -std=gnu89 -w -fno-strict-aliasing -fcommon -fsigned-char \
	-fno-builtin \
	-Wno-error=return-mismatch -Wno-error=return-type \
	-Wno-error=implicit-function-declaration -Wno-error=implicit-int \
	-Wno-error=int-conversion -Wno-error=incompatible-pointer-types

SRCS=	as0.c as1.c as2.c as3.c

host:	$(H)/as

$(H) $(HI)/sys:
	@mkdir -p $@

HINC=	$(HI)/a.out.h $(HI)/nlist.h $(HI)/ar.h $(HI)/ranlib.h \
	$(HI)/sys/exec.h

hostinc: $(HINC)

$(HI)/a.out.h $(HI)/ar.h $(HI)/ranlib.h: $(HI)/%.h: $(INC)/%.h | $(HI)/sys
	@cp $< $@

$(HI)/nlist.h: $(INC)/nlist.h | $(HI)/sys
	@sed -e '/char \*n_name;.*In memory/d' \
	    -e 's/off_t n_strx/int n_strx/' \
	    -e 's/u_int	n_value/unsigned short n_value/' $< > $@

$(HI)/sys/exec.h: $(TOP)/usr/src/sys/h/exec.h | $(HI)/sys
	@sed 's/unsigned long/unsigned int/' $< > $@

$(H)/as: $(SRCS) as.h $(HINC) | $(H)
	@echo hostcc as
	@$(HOSTCC) $(HOSTCFLAGS) -I$(HI) -I. -o $@ $(SRCS)

clean:
	rm -f $(H)/as
	rm -rf $(HI)

.PHONY: host hostinc clean
