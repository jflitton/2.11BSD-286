# The system's programs, built for the 286 with the small-model C
# library (../lib/libc/i286).
#
# From the top of the repository, after `. tools/env.sh` and making the
# library:
#
#	make -C usr/src/i286
#
# makes each program in obj/bin, under its own name, and the libraries
# they use in obj/lib/s.  GNU make.
#
# A program is listed with its sources, relative to usr/src, in
# NAME_SRCS, and may have NAME_FLAGS for the compiler and linker (tools/cc)
# and NAME_LIBS.  Each is compiled and linked in one run of tools/cc;
# what the compiler says goes to obj/bin/log/NAME, and is shown only if
# it fails.

TOP:=	$(abspath ../../..)
SRC:=	$(TOP)/usr/src
O=	$(TOP)/obj/bin
L=	$(TOP)/obj/lib/s
CC=	$(TOP)/tools/cc
LIBC=	$(L)/crt0.o $(L)/libc.lib

# Programs of one file, bin/NAME.c.
BINSTD=	cat chgrp chmod cmp cp date dd df du echo ed grep hostid kill ln \
	mv nice od pagesize pr pwd rm rmdir sync tee time who wall write
$(foreach p,$(BINSTD),$(eval $(p)_SRCS = bin/$(p).c))

init_SRCS=	sbin/init/init.c
getty_SRCS=	$(addprefix libexec/getty/,main.c init.c subr.c gettytab.c \
		get_date.c)
login_SRCS=	bin/login/login.c $(addprefix libexec/getty/,gettytab.c \
		subr.c init.c)
login_FLAGS=	-I$(SRC)/libexec/getty
login_LIBS=	-lutil
sh_SRCS=	$(addprefix bin/sh/,setbrk.c builtin.c blok.c stak.c cmd.c \
		fault.c main.c word.c string.c name.c args.c xec.c service.c \
		error.c io.c print.c macro.c expand.c ctype.c msg.c)
ls_SRCS=	bin/ls/ls.c bin/ls/stat_flags.c
mkdir_SRCS=	bin/mkdir/mkdir.c
stty_SRCS=	bin/stty/stty.c
test_SRCS=	bin/test/test.c bin/test/operators.c
hostname_SRCS=	bin/hostname/hostname.c
mknod_SRCS=	sbin/mknod/mknod.c
mount_SRCS=	sbin/mount/mount.c sbin/mount/mount_ufs.c \
		sbin/mount/getmntopts.c
umount_SRCS=	sbin/umount/umount.c
reboot_SRCS=	sbin/reboot/reboot.c

PROGS=	init getty login sh ls mkdir stty test hostname mknod mount umount \
	reboot $(BINSTD)

LIBUTIL= $(addprefix usr.lib/libutil/,login.c logout.c logwtmp.c dkcksum.c)

all:	$(addprefix $(O)/,$(PROGS))

.PHONY:	all root clean

$(O) $(O)/log $(L)/util:
	mkdir -p $@

HDRS:=	$(wildcard $(TOP)/usr/include/*.h $(SRC)/sys/h/*.h $(SRC)/sys/i286/*.h)

define prog
$(O)/$(1): $$(addprefix $(SRC)/,$$($(1)_SRCS)) $(LIBC) $(HDRS) \
		$$(patsubst -l%,$(L)/lib%.lib,$$($(1)_LIBS)) | $(O)/log
	@echo cc $(1)
	@$(CC) $$($(1)_FLAGS) -o $$@ $$(addprefix $(SRC)/,$$($(1)_SRCS)) \
	    $$($(1)_LIBS) > $(O)/log/$(1) 2>&1 || { cat $(O)/log/$(1); exit 1; }
endef
$(foreach p,$(PROGS),$(eval $(call prog,$(p))))

$(L)/libutil.lib: $(addprefix $(SRC)/,$(LIBUTIL)) $(HDRS) | $(L)/util
	@echo cc libutil
	@cd $(L)/util && rm -f *.o && \
	    $(CC) -c $(addprefix $(SRC)/,$(LIBUTIL)) > log 2>&1 || \
	    { cat log; exit 1; }
	@rm -f $@ && wlib -q -n -b $@ \
	    $(addprefix +,$(patsubst %.c,$(L)/util/%.o,$(notdir $(LIBUTIL))))

# The root file system, obj/root/root.fs, as root.proto lists it.  It
# needs the kernel (../sys/PC) and /boot (../sys/i286stand) made first.
R=	$(TOP)/obj/root
ROOTSIZE= 4000

root:	$(R)/root.fs

$(R):
	mkdir -p $@

# /etc/passwd is master.passwd without the passwords and the fields only
# the master file has: class, change and expire.
$(R)/passwd: $(SRC)/etc/master.passwd | $(R)
	sed -E 's/^([^:]*):[^:]*:([^:]*):([^:]*):[^:]*:[^:]*:[^:]*:/\1:*:\2:\3:/' \
	    $< > $@

$(R)/syserrlst: $(SRC)/usr.lib/liberrlst/errlst.c $(TOP)/tools/mkerrlst.py | $(R)
	python3 -I $(TOP)/tools/mkerrlst.py $< $@

$(R)/empty: | $(R)
	: > $@

$(R)/root.fs: root.proto $(wildcard etc/*) $(addprefix $(O)/,$(PROGS)) \
		$(R)/passwd $(R)/syserrlst $(R)/empty \
		$(TOP)/obj/PC/unix $(TOP)/obj/stand/boot
	python3 -I $(TOP)/tools/mkfs.py -s $(ROOTSIZE) root.proto $@

clean:
	rm -rf $(O) $(L)/util $(L)/libutil.lib $(R)
