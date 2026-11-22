# The kernel for a PC/AT-class 286 machine.
#
# From the top of the repository, after `. tools/env.sh`:
#
#	make -C usr/src/sys/PC
#
# makes obj/nPC/unix, the kernel, and obj/nPC/unix.img, a diskette that
# boots it, with the system's own compiler, assembler and linker, as
# they are built for this machine (tools/ncc, docs/toolchain.md), and
# none of the cross compiler's.  Everything made goes in obj/nPC.  GNU
# make.
#
#	make -C usr/src/sys/PC TOOLS=cross
#
# makes the same in obj/PC with the cross compiler, Open Watcom, and
# NASM.

TOP:=	$(abspath ../../../..)
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
O=	$(TOP)/obj/nPC
else
O=	$(TOP)/obj/PC
endif
H=	$(SYS)/h
M=	$(SYS)/i286
A=	$(SYS)/i286at
S=	$(SYS)/sys
N=	$(SYS)/net
I=	$(SYS)/netinet
E=	$(SYS)/i286if
C=	$(SYS)/PC
CRT=	$(TOP)/usr/src/lib/libc/i286/crt

OPTS=	-DINET -DUCB_METER
DEFS=	-DKERNEL $(OPTS) -I$(C) -I$(H) -I$(TOP)/usr/include
# 131 and 107: calls to functions with no prototype, and functions that
# return nothing in particular; the tree's C is full of both.  121: a
# name after #endif.
CFLAGS=	-mm $(DEFS) -wcd=131 -wcd=107 -wcd=121
CC=	$(TOP)/tools/cc286
AS=	nasm -f obj -DFAR -I$(M)/ -I$(CRT)/

# mch_start.o must come first: the kernel starts at its beginning.
MCH=	mch_start.o mch_trap.o mch_copy.o mch_swtch.o mch_xxx.o mch_net.o
CRTO=	i4m.o i4d.o
MACH=	clock.o conf.o cons.o machdep.o mem.o rd.o trap.o
DEV=	aha.o com.o kbd.o pcdisk.o sd.o vt.o wd.o
KERN=	init_main.o init_sysent.o kern_acct.o kern_clock.o \
	kern_descrip.o kern_exec.o kern_exit.o kern_fork.o kern_mman.o \
	kern_proc.o kern_prot.o kern_prot2.o kern_resource.o kern_sig.o \
	kern_sig2.o kern_subr.o kern_synch.o kern_sysctl.o kern_time.o \
	kern_xxx.o subr_log.o subr_prf.o subr_rmap.o subr_xxx.o \
	sys_generic.o sys_inode.o sys_kern.o sys_pipe.o sys_process.o \
	syscalls.o tty.o tty_conf.o tty_pty.o tty_subr.o tty_tty.o \
	uipc_syscalls.o \
	ufs_alloc.o ufs_bio.o ufs_bmap.o ufs_disksubr.o ufs_dsort.o \
	ufs_fio.o ufs_inode.o ufs_mount.o ufs_namei.o ufs_subr.o \
	ufs_syscalls.o ufs_syscalls2.o vfs_vnops.o vm_proc.o vm_sched.o \
	vm_swap.o vm_swp.o vm_text.o
# The network, which has a data segment of its own, NET_GROUP, and runs
# on the kernel's stacks (docs/network.md): -nd names its data segment,
# and -zu says that SS is not it.
# The Internet checksum is assembly; in_cksum.o is the C it was written
# from, to build with instead when one is in doubt.
CKSUM=	mch_cksum.o
NET=	af.o if.o if_loop.o raw_cb.o raw_usrreq.o route.o \
	if_ether.o in.o $(CKSUM) in_pcb.o in_proto.o ip_icmp.o \
	ip_input.o ip_output.o raw_ip.o tcp_input.o tcp_output.o \
	tcp_debug.o tcp_subr.o tcp_timer.o tcp_usrreq.o udp_usrreq.o \
	net_data.o net_sys.o sys_socket.o uipc_domain.o uipc_mbuf.o uipc_proto.o \
	uipc_socket.o uipc_socket2.o uipc_usrreq.o \
	if_ne.o
NETFLAGS= -DSUPERVISOR -zu -nd=NET

OBJS=	$(MCH) $(CRTO) $(MACH) $(DEV) $(KERN) $(NET) vers.o param.o

VPATH=	$(M):$(A):$(S):$(N):$(I):$(E):$(C):$(CRT)

all:	$(O) $(O)/unix.img

$(O):
	mkdir -p $(O)

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
$(addprefix $(O)/,$(NET)): NCFLAGS += -DSUPERVISOR -nd=NET

# What the compiler says of a file is kept in NAME.log, as the cross
# compiler's is: netcheck counts, in the network's, the pointers into SS
# it says it made near, which are not shown.
$(O)/%.o: %.c $(HDRS) $(HOST)/cpp $(HOST)/c0 $(HOST)/c1 $(HOST)/c2 \
		$(HOST)/as
	@echo cc $*.c
	@$(TOP)/tools/ncc $(NCFLAGS) -o $@ $< > $(O)/$*.log 2>&1 || \
	    { cat $(O)/$*.log; rm -f $@; exit 1; }
	@grep -v 'pointer into SS made near' $(O)/$*.log || true

$(O)/%.o: %.s $(M)/mch_defs.inc $(HOST)/as
	@echo as $*.s
	@$(HOST)/as -mm -DFAR -I$(M)/ -I$(CRT)/ -o $@ $<

$(O)/vers.o: $(O)/vers.c
	@cd $(O) && $(TOP)/tools/ncc -O -mm -o vers.o vers.c

# The kernel's selectors are the GDT's from slot 16 (docs/boot.md), and
# its stacks are in its data (uslot, param.c).  netcheck is run as on
# the cross compiler's (below), and the kernel is not left if it fails.
$(O)/unix: $(addprefix $(O)/,$(OBJS)) $(HOST)/ld $(TOP)/tools/netcheck.py
	@echo ld unix
	@cd $(O) && $(HOST)/ld -mm -gdt 16 -stack 0 -M -o unix $(OBJS) \
	    > unix.map || { rm -f unix; exit 1; }
	@echo netcheck
	@cd $(O) && python3 -I $(TOP)/tools/netcheck.py --net $(NET) \
	    --kernel $(filter-out $(NET),$(OBJS)) || { rm -f unix; exit 1; }

$(HOST)/cpp $(HOST)/c0 $(HOST)/c1 $(HOST)/c2 $(HOST)/as $(HOST)/ld:
	$(MAKE) -C $(TOP)/usr/src/lib/ccom/i286 host
else
$(addprefix $(O)/,$(NET)): CFLAGS += $(NETFLAGS)

$(O)/%.o: %.c $(HDRS)
	@echo cc $*.c
	@cd $(O) && { $(CC) $(CFLAGS) -o $*.o $(abspath $<) > $*.log 2>&1; r=$$?; \
	    cat $*.log; \
	    if [ $$r != 0 ] || grep -q 'Error!\|W112\|W102' $*.log; \
	    then rm -f $*.o; exit 1; fi; }

$(O)/%.o: %.s $(M)/mch_defs.inc
	@echo as $*.s
	@$(AS) -o $@ $<

$(O)/vers.o: $(O)/vers.c
	@cd $(O) && $(CC) $(CFLAGS) -o vers.o vers.c
endif

$(O)/vers.c: FRC
	@echo 'char version[] = "2.11 BSD UNIX for the 286 ('`date +%Y-%m-%d`')";' \
	    > $@

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
$(O)/unix: $(addprefix $(O)/,$(OBJS))
	@cd $(O) && { wlink op q format os2 name unix.ne disable 1014,1172 \
	    op nodefaultlibs,stack=2,map=unix.map \
	    file $(subst $(space),$(comma),$(OBJS)) > unix.log 2>&1; r=$$?; \
	    cat unix.log; \
	    if [ $$r != 0 ] || grep -q 'W1027' unix.log; \
	    then rm -f unix.ne; exit 1; fi; }
	@echo netcheck
	@cd $(O) && python3 -I $(TOP)/tools/netcheck.py --net $(NET) \
	    --kernel $(filter-out $(NET),$(OBJS))
	cd $(O) && python3 -I $(TOP)/tools/ne2aout.py --model m --gdt 16 \
	    --map unix.map unix.ne unix
endif

ifdef CCOM
$(O)/fdboot.bin: $(SYS)/i286stand/fdboot.s $(HOST)/as
	$(HOST)/as -b -o $@ $<
else
$(O)/fdboot.bin: $(SYS)/i286stand/fdboot.s
	nasm -f bin -o $@ $<
endif

$(O)/unix.img: $(O)/unix $(O)/fdboot.bin
	python3 -I $(TOP)/tools/mkfd.py $(O)/fdboot.bin $(O)/unix $@

# What the system's own Makefile for its kernel is written from, the one
# its make runs in /usr/src/sys/PC (tools/srcmk.py): where a source is
# looked for, the options, the network's objects, and every object.
table:
	@echo VPATH $(subst $(TOP)/usr/src/,,$(subst :, ,$(VPATH)))
	@echo HDRS $(subst $(TOP)/usr/src/,,$(HDRS))
	@echo OPTS $(OPTS)
	@echo NET $(NET)
	@echo OBJS $(OBJS)

clean:
	rm -rf $(O)

FRC:
