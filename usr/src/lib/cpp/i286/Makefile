# The preprocessor, /lib/cpp, where the system is built.
#
# On the 286 cpp is made by ../Makefile, of the small model, keeping
# what it has in memory (LIBVMF=0): the text of its macros and their
# names in segments it asks the system for (segalloc(2)), and the rest
# in its own data.  From the top of the repository:
#
#	make -C usr/src/lib/cpp/i286 host
#
# makes obj/host/ccom/cpp, the same compiled by and for the machine the
# system is built on, where tools/ncc runs it on every file the system's
# compiler compiles there, as cc runs /lib/cpp on the 286.
# ../../ccom/i286's host makes it too.  GNU make.
#
# It is to write what /lib/cpp writes.  It is given what it is given on
# the 286, with i286 defined, so that it defines i286 and nothing else of
# its own; the tree's headers for its own directory of them, which is
# /usr/include there (STDINC); a type of 32 bits for what #if reckons
# in, which is a long on the 286 and more here (CPPLONG); and NOSEGS,
# there being no segments to ask for here, so that what the 286's keeps
# in them is in memory of its own.  Its buffers and tables are those of
# a machine whose int is 16 bits, which i286 says, and as large as
# there; and a sum in #if that overflows does here what it does there.

TOP:=	$(abspath ../../../../..)
H=	$(TOP)/obj/host/ccom
HOSTCC=	cc
HOSTCFLAGS= -std=gnu89 -w -fno-strict-aliasing -fcommon -fsigned-char \
	-fwrapv -Wno-error=return-mismatch -Wno-error=return-type \
	-Wno-error=implicit-function-declaration -Wno-error=implicit-int \
	-Wno-error=int-conversion -Wno-error=incompatible-pointer-types

host:	$(H)/cpp

$(H):
	@mkdir -p $@

$(H)/cpp: ../cpp.c ../cpc.c ../token.c ../cpp.h | $(H)
	@echo hostcc cpp
	@$(HOSTCC) $(HOSTCFLAGS) -Di286 -DLIBVMF=0 -DNOSEGS \
	    -DCPPLONG=int '-DSTDINC="$(TOP)/usr/include"' -I.. -o $@ \
	    ../cpp.c ../cpc.c ../token.c

clean:
	rm -f $(H)/cpp

.PHONY: host clean
