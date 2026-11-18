# The system's programs, built for the 286 with the C library
# (../lib/libc/i286).
#
# From the top of the repository, after `. tools/env.sh` and making the
# library:
#
#	make -C usr/src/i286
#
# makes each program in obj/bin, under its own name, and the libraries
# they use in obj/lib/M, M being the model's letter.  GNU make.
#
# A program is listed with its sources, relative to usr/src unless they
# are absolute, in NAME_SRCS, and may have NAME_FLAGS for the compiler
# and linker (tools/cc), NAME_LIBS, and NAME_MODEL: s, the small model,
# unless it is m, the medium (docs/memory-model.md).  Each is compiled
# and linked in one run of tools/cc; what the compiler says goes to
# obj/bin/log/NAME, and is shown only if it fails.

TOP:=	$(abspath ../../..)
SRC:=	$(TOP)/usr/src
O=	$(TOP)/obj/bin
LIB=	$(TOP)/obj/lib
CC=	$(TOP)/tools/cc

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
sleep_SRCS=	usr.bin/sleep.c
csh_SRCS=	$(addprefix bin/csh/,alloc11.c printf.c sh.c sh.char.c \
		sh.dir.c sh.dol.c sh.err.c sh.exec.c sh.exec2.c sh.exp.c \
		sh.file.c sh.func.c sh.glob.c sh.hist.c sh.init.c sh.lex.c \
		sh.misc.c sh.parse.c sh.print.c sh.proc.c sh.sem.c sh.set.c \
		sh.time.c)
csh_FLAGS=	-DTELL -DVFORK -DFILEC
csh_MODEL=	m

# ex, which is also vi.  Its error messages are taken out of its C into
# a file, /usr/share/misc/exstrings, that it reads them from
# (tools/mkstr.py): the PDP-11 needed the room in its 64K of data, and
# so does the medium model.  The sources mkstr makes are in obj/ex.
EX=	ex.c ex_addr.c ex_cmds.c ex_cmds2.c ex_cmdsub.c ex_data.c \
	ex_extern.c ex_get.c ex_io.c ex_put.c ex_re.c ex_set.c ex_subr.c \
	ex_tagio.c ex_temp.c ex_tty.c ex_unix.c ex_v.c ex_vadj.c ex_vget.c \
	ex_vmain.c ex_voper.c ex_vops.c ex_vops2.c ex_vops3.c ex_vput.c \
	ex_vwind.c printf.c
EXFLAGS=	-I$(SRC)/ucb/ex -DTABS=8 -DLISPCODE -DCHDIR -DVFORK -DFASTTAG \
		-DTEMP512K
EXO=	$(TOP)/obj/ex
ex_SRCS=	$(addprefix $(EXO)/,$(EX))
ex_FLAGS=	$(EXFLAGS)
ex_LIBS=	-ltermcap
ex_MODEL=	m
exrecover_SRCS=	ucb/ex/exrecover.c ucb/ex/ex_extern.c
exrecover_FLAGS= $(EXFLAGS)
expreserve_SRCS= ucb/ex/expreserve.c
expreserve_FLAGS= $(EXFLAGS)

# tcsh.  Its messages are taken out of its C into /etc/tcsh6.00strgs too,
# by its own kind of mkstr, mkprstr: a call to xprintf with a string
# becomes one to mkprintf with the string's place in the file
# (jpnerror.c).  Three of its headers are made from its C, as its
# Makefile makes them; they and the C mkstr makes are in obj/tcsh.
TCSH=	sh.c sh.dir.c sh.dol.c sh.err.c sh.exec.c sh.char.c sh.exp.c \
	sh.func.c sh.glob.c sh.hist.c sh.init.c sh.lex.c sh.misc.c \
	sh.parse.c sh.print.c sh.proc.c sh.sem.c sh.set.c sh.time.c glob.c \
	tw.help.c tw.init.c tw.parse.c tw.spell.c ed.chared.c ed.refresh.c \
	ed.screen.c ed.init.c ed.inputl.c ed.defns.c ed.xmap.c tc.alloc.c \
	tc.bind.c tc.const.c tc.disc.c tc.func.c tc.os.c tc.printf.c \
	tc.prompt.c tc.sched.c tc.str.c tc.vers.c tc.who.c alloc11.c \
	jpnerror.c
TCO=	$(TOP)/obj/tcsh
TCHDRS=	$(addprefix $(TCO)/,sh.err.h ed.defns.h tc.const.h)
tcsh_SRCS=	$(addprefix $(TCO)/,$(TCSH))
tcsh_FLAGS=	-I$(TCO) -I$(SRC)/bin/tcsh
tcsh_LIBS=	-ltermcap
tcsh_MODEL=	m

PROGS=	init getty login sh csh ls mkdir stty test hostname mknod mount \
	umount reboot sleep ex exrecover expreserve tcsh $(BINSTD)

# The libraries the programs use besides the C library: NAME_LIB lists a
# library's sources, and NAME_LIBFLAGS what it is compiled with.  Each is
# made, in obj/lib/M/libNAME.lib, for whichever models the programs that
# use it are.
util_LIB=	$(addprefix usr.lib/libutil/,login.c logout.c logwtmp.c \
		dkcksum.c)
termcap_LIB=	$(addprefix usr.lib/libtermlib/,termcap.c tgoto.c tputs.c)
termcap_LIBFLAGS= -DCM_N -DCM_GT -DCM_B -DCM_D -DNOSCCS
LIBRARIES=	util termcap

all:	$(addprefix $(O)/,$(PROGS))

.PHONY:	all root clean

$(O) $(O)/log $(EXO) $(TCO):
	mkdir -p $@

HDRS:=	$(wildcard $(TOP)/usr/include/*.h $(SRC)/sys/h/*.h $(SRC)/sys/i286/*.h)

# A program's model, the directory of that model's libraries, and its
# sources.
model=	$(or $($(1)_MODEL),s)
lib=	$(LIB)/$(call model,$(1))
srcs=	$(foreach s,$($(1)_SRCS),$(if $(filter /%,$(s)),$(s),$(SRC)/$(s)))

define prog
$(O)/$(1): $$(call srcs,$(1)) $(HDRS) \
		$(call lib,$(1))/crt0.o $(call lib,$(1))/libc.lib \
		$$(patsubst -l%,$(call lib,$(1))/lib%.lib,$$($(1)_LIBS)) | $(O)/log
	@echo cc $(1)
	@$(CC) -m$(call model,$(1)) $$($(1)_FLAGS) -o $$@ \
	    $$(call srcs,$(1)) $$($(1)_LIBS) \
	    > $(O)/log/$(1) 2>&1 || { cat $(O)/log/$(1); exit 1; }
endef
$(foreach p,$(PROGS),$(eval $(call prog,$(p))))

# ex's sources with its messages taken out, and the file they are in.
$(EXO)/exstrings: $(addprefix $(SRC)/ucb/ex/,$(EX)) \
		$(wildcard $(SRC)/ucb/ex/*.h) $(TOP)/tools/mkstr.py | $(EXO)
	python3 -I $(TOP)/tools/mkstr.py $@ $(EXO) \
	    $(addprefix $(SRC)/ucb/ex/,$(EX))
$(ex_SRCS): $(EXO)/exstrings ;

# tcsh's, likewise, and its headers: sh.err.h has the numbers of its
# errors, from sh.err.c, which has two sets of the flags that go with
# them, the PDP-11's 16-bit ones and others; ed.defns.h the numbers of
# the editor's functions; tc.const.h a declaration of each of its
# constant strings.
T=	$(SRC)/bin/tcsh
$(TCO)/tcsh6.00strgs: $(addprefix $(T)/,$(TCSH)) $(wildcard $(T)/*.h) \
		$(TOP)/tools/mkstr.py | $(TCO)
	python3 -I $(TOP)/tools/mkstr.py -e xprintf=mkprintf -n 5 $@ $(TCO) \
	    $(addprefix $(T)/,$(TCSH))
$(tcsh_SRCS): $(TCO)/tcsh6.00strgs $(TCHDRS) ;
$(TCO)/sh.err.h: $(T)/sh.err.c | $(TCO)
	{ echo '#ifndef _h_sh_err'; echo '#define _h_sh_err'; \
	  sed '/^#if.*pdp11/,/^#else/d' $< | grep '^#define.*ERR_'; \
	  echo '#endif'; } > $@
$(TCO)/ed.defns.h: $(T)/ed.defns.c | $(TCO)
	{ echo '#ifndef _h_ed_defns'; echo '#define _h_ed_defns'; \
	  grep '^#define.*[FV]_' $<; echo '#endif'; } > $@
$(TCO)/tc.const.h: $(T)/tc.const.c | $(TCO)
	grep '^Char STR' $< | \
	    sed 's/Char \([a-zA-Z0-9_]*\).*/extern Char \1[];/' | sort > $@

define library
$(LIB)/$(2)/lib$(1).lib: $(addprefix $(SRC)/,$($(1)_LIB)) $(HDRS) \
		| $(LIB)/$(2)/$(1)
	@echo cc lib$(1), $(2)
	@cd $(LIB)/$(2)/$(1) && rm -f *.o && \
	    $(CC) -m$(2) $($(1)_LIBFLAGS) -c \
	    $(addprefix $(SRC)/,$($(1)_LIB)) > log 2>&1 || { cat log; exit 1; }
	@rm -f $$@ && wlib -q -n -b $$@ \
	    $(patsubst %.c,+$(LIB)/$(2)/$(1)/%.o,$(notdir $($(1)_LIB)))
$(LIB)/$(2)/$(1):
	mkdir -p $$@
endef
$(foreach l,$(LIBRARIES),$(foreach m,s m,$(eval $(call library,$(l),$(m)))))

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

$(R)/termcap: $(wildcard $(SRC)/share/termcap/*) $(TOP)/tools/mktermcap.py \
		| $(R)
	python3 -I $(TOP)/tools/mktermcap.py $(SRC)/share/termcap $@

$(R)/root.fs: root.proto $(wildcard etc/*) $(addprefix $(O)/,$(PROGS)) \
		$(R)/passwd $(R)/syserrlst $(R)/empty $(R)/termcap \
		$(TOP)/obj/PC/unix $(TOP)/obj/stand/boot
	python3 -I $(TOP)/tools/mkfs.py -s $(ROOTSIZE) root.proto $@

clean:
	rm -rf $(O) $(EXO) $(TCO) $(R) $(foreach l,$(LIBRARIES),$(LIB)/[sm]/$(l) \
	    $(LIB)/[sm]/lib$(l).lib)
