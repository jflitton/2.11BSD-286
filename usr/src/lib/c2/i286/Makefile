# The object code improver for the 80286.
#
# ../c2 is the PDP-11's.  This is that program for the code that the
# compiler's second pass writes here (../../ccom/i286): c20.c is
# ../c20.c, reading NASM's assembly and with the 286's instructions in
# its table, and c21.c, which knows what the instructions do, is
# written again.
#
# From the top of the repository:
#
#	make -C usr/src/lib/c2/i286 host
#
# makes obj/host/ccom/c2, compiled by and for the machine the system is
# built on, where tools/ncc -O runs it between the second pass and the
# assembler.  ../../ccom/i286's host makes it too.  GNU make.
#
# getline is the name of something else in that machine's library, and
# is given another there.

TOP:=	$(abspath ../../../../..)
H=	$(TOP)/obj/host/ccom
HOSTCC=	cc
HOSTCFLAGS= -std=gnu89 -w -fno-strict-aliasing -fcommon -fsigned-char \
	-fno-builtin \
	-Wno-error=return-mismatch -Wno-error=return-type \
	-Wno-error=implicit-function-declaration -Wno-error=implicit-int \
	-Wno-error=int-conversion -Wno-error=incompatible-pointer-types

host:	$(H)/c2

$(H):
	@mkdir -p $@

$(H)/c2: c20.c c21.c c2.h | $(H)
	@echo hostcc c2
	@sed 's/getline()/c2getline()/' c20.c > $(H)/c20.c
	@$(HOSTCC) $(HOSTCFLAGS) -I. -o $@ $(H)/c20.c c21.c

clean:
	rm -f $(H)/c2 $(H)/c20.c
