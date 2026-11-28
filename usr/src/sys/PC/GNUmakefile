# The kernel of the machine this directory is for, made where the system
# is built.  config put this here as GNUmakefile (../conf/Make.host),
# which GNU make reads before the Makefile, the system's, that config
# wrote beside it.  From the top of the repository, after
# `. tools/env.sh`:
#
#	make -C usr/src/sys/PC
#
# makes obj/nPC/unix, the kernel, and obj/nPC/unix.img, a diskette that
# boots it, with the system's own compiler, assembler and linker, as
# they are built for this machine (tools/ncc, docs/toolchain.md), and
# none of the cross compiler's.  Everything made goes in obj/nPC, after
# the name of the directory.
#
#	make -C usr/src/sys/PC TOOLS=cross
#
# makes the same in obj/PC with the cross compiler, Open Watcom, and
# NASM.
#
# The options, the objects and the order they are linked in are the
# Makefile's.  config is run again first, in obj, on what ../conf has
# now, and what it writes is to be what this directory has: when the
# machine's file there has changed, config is to be run.

TOP:=	$(abspath ../../../..)
MACHINE:= $(notdir $(CURDIR))
# Whose tools make it: the system's own compiler, assembler and linker,
# as built for this machine (TOOLS=native), or the cross compiler's
# (TOOLS=cross).  CCOM is set for the first, and is not to be given.
TOOLS?=	native
ifeq ($(filter native cross,$(TOOLS)),)
$(error TOOLS is native or cross)
endif
override CCOM:=	$(if $(filter native,$(TOOLS)),1)
SYS:=	$(TOP)/usr/src/sys
ifdef CCOM
O=	$(TOP)/obj/n$(MACHINE)
else
O=	$(TOP)/obj/$(MACHINE)
endif
H=	$(SYS)/h
M=	$(SYS)/i286
A=	$(SYS)/i286at
S=	$(SYS)/sys
N=	$(SYS)/net
I=	$(SYS)/netinet
E=	$(SYS)/i286if
C=	$(CURDIR)
CRT=	$(TOP)/usr/src/lib/libc/i286/crt

all:	$(O)/unix.img

# What config wrote in the Makefile: OPTS, and KERNOBJ, the objects in
# the order they are linked, of which NETOBJ are the network's.  A
# macro's lines are joined, and what it names is put in.
macro=	$(shell awk '/^$(1)=/ { p = 1; sub(/^$(1)=/, "") } \
	    p { l = $$0; sub(/\\$$/, "", l); printf "%s ", l } \
	    p && !/\\$$/ { exit }' Makefile)
$(foreach m,OPTS CONF BASE NETOBJ KERNOBJ,$(eval $(m):= $(strip $(call macro,$(m)))))

CONFFILES:= $(wildcard $(SYS)/conf/config $(SYS)/conf/$(MACHINE) \
	$(SYS)/conf/Make.* $(SYS)/conf/param.c)
$(O)/config/made: $(CONFFILES) $(wildcard *) $(H)/localopts.h
	@rm -rf $(O)/config
	@mkdir -p $(O)/config/conf $(O)/config/h
	@cp $(CONFFILES) $(O)/config/conf
	@cd $(O)/config/conf && sh config $(MACHINE) > /dev/null
	@cd $(O)/config/$(MACHINE) && for f in *; do \
	    cmp -s $$f $(C)/$$f || { echo "$(MACHINE)/$$f is not what" \
	    "config writes from ../conf/$(MACHINE): run config"; exit 1; }; \
	done
	@cmp -s $(O)/config/h/localopts.h $(H)/localopts.h || { \
	    echo "h/localopts.h is not what config writes: run config"; \
	    exit 1; }
	@touch $@

DEFS=	-DKERNEL $(OPTS) -I$(C) -I$(H) -I$(SYS) -I$(TOP)/usr/include
# 131 and 107: calls to functions with no prototype, and functions that
# return nothing in particular; the tree's C is full of both.  121: a
# name after #endif.
CFLAGS=	-mm $(DEFS) -wcd=131 -wcd=107 -wcd=121
CC=	$(TOP)/tools/cc286
AS=	nasm -f obj -DFAR -I$(M)/ -I$(CRT)/

# The network, which has a data segment of its own, NET_GROUP, and runs
# on the kernel's stacks (docs/network.md): -nd names its data segment,
# and -zu says that SS is not it.
NETFLAGS= -DSUPERVISOR -zu -nd=NET

VPATH=	$(M):$(A):$(S):$(N):$(I):$(E):$(C):$(CRT)

# A truncated pointer or a mismatched argument is an error here, though
# the compiler only warns: with far pointers in the kernel either one is
# a wild store.
# Every object is made again when any header changes: crude, and never
# wrong.
HDRS:=	$(wildcard $(H)/*.h $(M)/*.h $(A)/*.h $(C)/*.h $(N)/*.h $(I)/*.h $(E)/*.h)

ifdef CCOM
# The system's own tools.  The network's data is NET_GROUP, as the
# cross compiler's -nd and -zu make it: -nd=NET does both.
HOST=	$(TOP)/obj/host/ccom
NCFLAGS= -k -O -mm $(DEFS)
$(addprefix $(O)/,$(NETOBJ)): NCFLAGS += -DSUPERVISOR -nd=NET

# What the compiler says of a file is kept in NAME.log, as the cross
# compiler's is: netcheck counts, in the network's, the pointers into SS
# it says it made near, which are not shown.
$(O)/%.o: %.c $(HDRS) $(HOST)/cpp $(HOST)/c0 $(HOST)/c1 $(HOST)/c2 \
		$(HOST)/as | $(O)/config/made
	@echo cc $*.c
	@$(TOP)/tools/ncc $(NCFLAGS) -o $@ $< > $(O)/$*.log 2>&1 || \
	    { cat $(O)/$*.log; rm -f $@; exit 1; }
	@grep -v 'pointer into SS made near' $(O)/$*.log || true

$(O)/%.o: %.s $(M)/mch_defs.inc $(HOST)/as | $(O)/config/made
	@echo as $*.s
	@$(HOST)/as -mm -DFAR -I$(M)/ -I$(CRT)/ -o $@ $<

$(O)/vers.o: $(O)/vers.c
	@cd $(O) && $(TOP)/tools/ncc $(NCFLAGS) -o vers.o vers.c

# The kernel's selectors are the GDT's from slot 16 (docs/boot.md), and
# its stacks are in its data (uslot, param.c).  netcheck is run as on
# the cross compiler's (below), and the kernel is not left if it fails.
$(O)/unix: $(addprefix $(O)/,$(KERNOBJ)) $(HOST)/ld $(TOP)/tools/netcheck.py
	@echo ld unix
	@cd $(O) && $(HOST)/ld -mm -gdt 16 -stack 0 -M -o unix $(KERNOBJ) \
	    > unix.map || { rm -f unix; exit 1; }
	@echo netcheck
	@cd $(O) && python3 -I $(TOP)/tools/netcheck.py --net $(NETOBJ) \
	    --kernel $(filter-out $(NETOBJ),$(KERNOBJ)) || { rm -f unix; exit 1; }

$(HOST)/cpp $(HOST)/c0 $(HOST)/c1 $(HOST)/c2 $(HOST)/as $(HOST)/ld:
	$(MAKE) -C $(TOP)/usr/src/lib/ccom/i286 host
else
$(addprefix $(O)/,$(NETOBJ)): CFLAGS += $(NETFLAGS)

$(O)/%.o: %.c $(HDRS) | $(O)/config/made
	@echo cc $*.c
	@cd $(O) && { $(CC) $(CFLAGS) -o $*.o $(abspath $<) > $*.log 2>&1; r=$$?; \
	    cat $*.log; \
	    if [ $$r != 0 ] || grep -q 'Error!\|W112\|W102' $*.log; \
	    then rm -f $*.o; exit 1; fi; }

$(O)/%.o: %.s $(M)/mch_defs.inc | $(O)/config/made
	@echo as $*.s
	@$(AS) -o $@ $<

$(O)/vers.o: $(O)/vers.c
	@cd $(O) && $(CC) $(CFLAGS) -o vers.o vers.c
endif

# newvers.sh is told the day: it reads it from what date prints, which
# here is not always what the system's prints.  It writes vers.c in a
# directory of its own, which takes the place of the one there only if
# it differs: the kernel is linked again only when something has
# changed, and makes of it at once do not write the same file.
$(O)/vers.c: FRC | $(O)/config/made
	@cd $(O) && d=vers.$$$$ && mkdir $$d && \
	    (cd $$d && DATE=$${DATE:-$$(date +%Y-%m-%d)} \
	    sh $(SYS)/conf/newvers.sh) && \
	    { cmp -s $$d/vers.c vers.c || mv $$d/vers.c vers.c; } && \
	    rm -rf $$d

comma:=	,
empty:=
space:=	$(empty) $(empty)

# netcheck finds the calls and references between the kernel and the
# network that do not go through KScall and SKcall (docs/network.md).
#
# Message 1014 is the lack of a stack segment: the stacks are in the
# kernel's data (uslot, param.c).  The linker would still count 4K for a
# stack against the 64K the data segment may be, so it is told 2 bytes,
# and 1172 is its saying that is under 512.  1027 is a variable defined
# in two files, which a header that declares one without "extern" causes
# in every file that includes it; the linker keeps the first and goes on,
# and here that is an error.
ifndef CCOM
$(O)/unix: $(addprefix $(O)/,$(KERNOBJ))
	@cd $(O) && { wlink op q format os2 name unix.ne disable 1014,1172 \
	    op nodefaultlibs,stack=2,map=unix.map \
	    file $(subst $(space),$(comma),$(KERNOBJ)) > unix.log 2>&1; r=$$?; \
	    cat unix.log; \
	    if [ $$r != 0 ] || grep -q 'W1027' unix.log; \
	    then rm -f unix.ne; exit 1; fi; }
	@echo netcheck
	@cd $(O) && python3 -I $(TOP)/tools/netcheck.py --net $(NETOBJ) \
	    --kernel $(filter-out $(NETOBJ),$(KERNOBJ))
	cd $(O) && python3 -I $(TOP)/tools/ne2aout.py --model m --gdt 16 \
	    --map unix.map unix.ne unix
endif

ifdef CCOM
$(O)/fdboot.bin: $(SYS)/i286stand/fdboot.s $(HOST)/as | $(O)/config/made
	$(HOST)/as -b -o $@ $<
else
$(O)/fdboot.bin: $(SYS)/i286stand/fdboot.s | $(O)/config/made
	nasm -f bin -o $@ $<
endif

$(O)/unix.img: $(O)/unix $(O)/fdboot.bin
	python3 -I $(TOP)/tools/mkfd.py $(O)/fdboot.bin $(O)/unix $@

# The objects, for tests/self to have the system's make compared.
table:
	@echo OBJS $(KERNOBJ)

clean:
	rm -rf $(O)

FRC:
