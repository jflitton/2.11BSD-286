# The link editor for the 80286.
#
# ../ld.c is the PDP-11's.  This one is written again on the plan of
# that: what a program is made of here is segments, and an object
# file's relocation is a list of the words to change (a.out.h), where
# the PDP-11's is a word for each word.
#
# From the top of the repository:
#
#	make -C usr/src/bin/ld/i286 host
#
# makes obj/host/ccom/ld, compiled by and for the machine the system is
# built on, where tools/cc -ccom runs it on what the assembler made
# (../../as/i286, whose Makefile says what that machine has to be and
# makes the headers this is compiled with there).  ../../lib/ccom/i286's
# host makes it too.  GNU make.

TOP:=	$(abspath ../../../../..)
H=	$(TOP)/obj/host/ccom
HI=	$(TOP)/obj/host/ccom/include
HOSTCC=	cc
HOSTCFLAGS= -std=gnu89 -w -fno-strict-aliasing -fcommon -fsigned-char \
	-fno-builtin \
	-Wno-error=return-mismatch -Wno-error=return-type \
	-Wno-error=implicit-function-declaration -Wno-error=implicit-int \
	-Wno-error=int-conversion -Wno-error=incompatible-pointer-types

host:	$(H)/ld

$(H):
	@mkdir -p $@

hostinc:
	@$(MAKE) --no-print-directory -C ../../as/i286 hostinc

$(H)/ld: ld.c $(TOP)/usr/include/a.out.h $(TOP)/usr/include/nlist.h \
		$(TOP)/usr/src/sys/h/exec.h | $(H) hostinc
	@echo hostcc ld
	@$(HOSTCC) $(HOSTCFLAGS) -I$(HI) -o $@ ld.c

clean:
	rm -f $(H)/ld

.PHONY: host hostinc clean
