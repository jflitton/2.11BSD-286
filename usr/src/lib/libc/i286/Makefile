# The C library for the 286: small, medium and large models.
#
# From the top of the repository, after `. tools/env.sh`:
#
#	make -C usr/src/lib/libc/i286
#
# makes, for each model, obj/lib/M/crt0.o, the start-up code every
# program is linked with first, and obj/lib/M/libc.lib, the library: M
# is s for the small model, m for the medium and l for the large.
# MODEL=s, MODEL=m or MODEL=l makes only the one.  GNU make.
#
# The medium model's code is far: each file's is a segment of its own,
# and every function returns far.  Its data is near, in one segment, as
# the small model's is.  The C is the same for both, compiled with -ms or
# -mm; the assembly is assembled with -DFAR for the medium model
# (crt/DEFS.inc), and the system call stubs are made for it.
#
# The large model has the medium's code, and data in as many segments
# as it takes: every pointer is far, and DS is whatever was last
# pointed at (docs/memory-model.md).  The C is compiled with -ml and
# the assembly assembled with -DFARDATA as well.  Its malloc is
# gen/lmalloc.c, which gets segments from the system where the others'
# moves the break; and it has the routines compiled code calls for
# arithmetic on __huge pointers (crt/pia.s), which only the cross
# compiler has.
#
# Each variable a file defines without a value gets common storage
# (tools/commonize.py), as for programs (tools/cc).
#
# With CCOM=1 the library is made by the system's own tools, as built
# for this machine: the C compiled by its compiler (tools/ncc -O,
# ../../ccom/i286 and ../../c2/i286) and the assembly, which is the
# same, assembled by its assembler (../../../bin/as/i286).  What is
# made goes to obj/nlib/M, crt0.o and libc.a: objects of the system's
# own kind (a.out.h) in a library of its kind, with ranlib's table
# (tools/ar.py), which tools/cc -ccom has the system's linker link a
# program with.  That compiler gives such variables common storage
# itself.  The library is made again when any of the tools is.  It has
# alloca (gen/alloca.s), which the cross compiler has in line and its
# library has not.
#
# The library is the tree's portable C, as ../Makefile and the Makefiles
# of its directories list it, with this directory in place of ../pdp:
#
#	csu	start-up
#	sys	the system call stubs, made from sys/syscalls.tab by
#		tools/mkstubs.py, and the calls that need more than a stub
#	gen	setjmp and longjmp, and the floating-point routines the
#		PDP-11 has in assembly; its other assembly (strings, bcopy
#		and the like) is replaced by the C in ../gen that it stood
#		in for
#	net	byte order
#	crt	the routines compiled code calls for long arithmetic
#
# Left out: gen/fakcu.c, an empty _cleanup for exit() to call in a
# program without stdio.  It works by the order of the PDP-11's library,
# which ld reads once, and here the linker took it in place of stdio's,
# which then did not flush at exit.  Every program has stdio's.
#
# Left out: stdio/fpfcom.c, printf's floating point for a program that
# has none, which on the PDP-11 is taken unless the compiler's mark of a
# program with floating point, fltused, brings in stdio/pfcom.c.  This
# compiler makes no such mark, so every program that formats has the
# real one.
#
# nlist is gen/nlist.c as it is, with gen/nsym.c here in place of
# ../pdp/gen/nsym.c: what it reads is this system's format, which the
# headers give it (sys/exec.h, a.out.h).
#
# Left out for now: profiling (mcrt0, gmon), and the PDP-11's
# compat-2.9.

TOP:=	$(abspath ../../../../..)
LIBC:=	$(TOP)/usr/src/lib/libc
MD:=	$(LIBC)/i286

ifeq ($(MODEL),)

all clean:
	@$(MAKE) --no-print-directory MODEL=s $@
	@$(MAKE) --no-print-directory MODEL=m $@
	@$(MAKE) --no-print-directory MODEL=l $@

.PHONY:	all clean

else

ifeq ($(filter $(MODEL),s m l),)
$(error MODEL is s, m or l)
endif
# What the assembly is told of the model (crt/DEFS.inc).
FAR=	$(if $(filter m l,$(MODEL)),-DFAR) $(if $(filter l,$(MODEL)),-DFARDATA)

O=	$(TOP)/obj/$(if $(CCOM),nlib,lib)/$(MODEL)
OBJ=	$(O)/obj

ifeq ($(CCOM),)
CC=	$(TOP)/tools/cc286
# 131 and 107: calls to functions with no prototype, and functions that
# return nothing in particular; the tree's C is full of both.  121: a
# name after #endif.
CFLAGS=	-m$(MODEL) -I$(TOP)/usr/include -wcd=131 -wcd=107 -wcd=121
else
CC=	$(TOP)/tools/ncc
CFLAGS=	-O -m$(MODEL) -I$(TOP)/usr/include
NCC:=	$(TOP)/tools/ncc $(addprefix $(TOP)/obj/host/ccom/,c0 c1 c2 as)
endif
ifeq ($(CCOM),)
AS=	nasm -f obj -I$(MD)/crt/ $(FAR)
SAS=	nasm -f obj
LIBC_A=	$(O)/libc.lib
else
AS=	$(TOP)/obj/host/ccom/as -m$(MODEL) -I$(MD)/crt $(FAR)
SAS=	$(TOP)/obj/host/ccom/as -m$(MODEL)
LIBC_A=	$(O)/libc.a
endif

GEN=	abort alarm atoi atol calloc closedir crypt ctime ctype_ daemon \
	devname disklabel err execvp fstab gcvt getenv getgrent \
	getgrgid getgrnam getlogin getgrouplist gethostname getpagesize \
	getpass getpwent getloadavg getmntinfo getttyent getttynam \
	getusershell getwd initgroups isatty isctype malloc mktemp ndbm \
	nlist opendir perror popen psignal qsort random readdir regex scandir \
	seekdir setmode sethostname setenv siglist signal siginterrupt \
	sigsetops sleep strcasecmp strftime swab sysctl syslog system \
	swapdevname syserrlst telldir time timezone ttyname ttyslot ualarm \
	usleep valloc strdup uname wait wait3 waitpid
# what ../pdp/gen has in assembly
GENC=	abs bcmp bcopy bzero fabs ffs frexp index insque remque rindex \
	strcat strcmp strcpy strlen strncat strncmp strncpy
# and what it has in C
PDPGEN=	atof ecvt setjmperr
STDIO=	clrerr exit fclose fdopen findfp fflush fopen fprintf fpurge fread \
	freopen fseek ftell fwrite fscanf fvwrite getc getchar getw makebuf \
	rget fputc printf putc putchar putw rewind scanf setbuf setbuffer \
	setvbuf snprintf sscanf stdio tmpfile tmpnam tempnam sprintf ungetc \
	vfprintf vfscanf vprintf vscanf vsnprintf vsprintf wbuf flags \
	refill wsetup pfcom acvt puts fputs fgetc fgets gets fgetln
STDLIB=	getopt getsubopt strtol strtoul imaxabs imaxdiv lconv localeconv \
	setlocale strtomax
STRING=	strcspn strpbrk strerror strsep strspn strstr strtok
COMPAT41= ftime gtty nice pause rand stty times utime
COMPAT43= creat gethostid sethostid sigcompat setregid setreuid setrgid \
	setruid
# ../pdp/gen has strchr and strrchr as other names for index and rindex
COMPAT5= memccpy memchr memcmp memcpy memset strchr strrchr
INET=	inet_addr inet_network inet_netof inet_ntoa inet_lnaof inet_maddr
NET=	getnbyaddr getnbyname getnent getpent getpname getproto getsbyname \
	getsbyport getsent herror rcmd res_comp res_debug res_init \
	res_mkquery res_query res_send rexec ruserpass
NAMED=	gethnamadr sethostent
NS=	ns_addr ns_ntoa
MDSYS=	brk execl execle execv iovec setlogin sigaction sigaltstack
MDGENC=	nsym
MDSYSS=	sigtramp
MDGEN=	ldexp modf setjmp $(if $(CCOM),alloca)
MDNET=	htons
CRT=	i4m i4d chp $(if $(filter l,$(MODEL)),pia)

COBJS=	$(GEN) $(GENC) $(PDPGEN) $(STDIO) $(STDLIB) $(STRING) $(COMPAT41) \
	$(COMPAT43) $(COMPAT5) $(INET) $(NET) $(NAMED) $(NS) $(MDSYS) \
	$(MDGENC)
SOBJS=	$(MDSYSS) $(MDGEN) $(MDNET) $(CRT)
OBJS=	$(addprefix $(OBJ)/,$(addsuffix .o,$(COBJS) $(SOBJS)))

all:	$(O)/crt0.o $(LIBC_A)

$(OBJ) $(OBJ)/sys:
	mkdir -p $@

# Every object is made again when any header changes: crude, and never
# wrong.
HDRS:=	$(wildcard $(TOP)/usr/include/*.h $(TOP)/usr/src/sys/h/*.h \
	$(TOP)/usr/src/sys/i286/*.h $(LIBC)/stdio/*.h) $(NCC)

define compile
	@echo cc $(notdir $<)
	@cd $(OBJ) && { $(CC) $(CFLAGS) $(1) -o $*.o $< > $*.log 2>&1; r=$$?; \
	    cat $*.log; \
	    if [ $$r != 0 ] || grep -q 'Error!' $*.log; \
	    then rm -f $*.o; exit 1; fi; }
	$(if $(CCOM),,@python3 -I $(TOP)/tools/commonize.py $(OBJ)/$*.o)
endef

# what the large model has of its own: lmalloc.c for malloc.c
ifeq ($(MODEL),l)
$(OBJ)/%.o: $(MD)/gen/l%.c $(HDRS) | $(OBJ)
	$(call compile)
endif
$(OBJ)/%.o: $(LIBC)/gen/%.c $(HDRS) | $(OBJ)
	$(call compile)
# this directory's before ../pdp's, which has a nsym.c too
$(OBJ)/%.o: $(MD)/gen/%.c $(HDRS) | $(OBJ)
	$(call compile)
$(OBJ)/%.o: $(LIBC)/pdp/gen/%.c $(HDRS) | $(OBJ)
	$(call compile)
$(OBJ)/%.o: $(LIBC)/stdio/%.c $(HDRS) | $(OBJ)
	$(call compile,-I$(LIBC)/stdio)
$(OBJ)/%.o: $(LIBC)/stdlib/%.c $(HDRS) | $(OBJ)
	$(call compile)
$(OBJ)/%.o: $(LIBC)/string/%.c $(HDRS) | $(OBJ)
	$(call compile)
$(OBJ)/%.o: $(LIBC)/compat-4.1/%.c $(HDRS) | $(OBJ)
	$(call compile)
$(OBJ)/%.o: $(LIBC)/compat-43/%.c $(HDRS) | $(OBJ)
	$(call compile)
$(OBJ)/%.o: $(LIBC)/compat-sys5/%.c $(HDRS) | $(OBJ)
	$(call compile)
$(OBJ)/%.o: $(LIBC)/inet/%.c $(HDRS) | $(OBJ)
	$(call compile)
$(OBJ)/%.o: $(LIBC)/net/%.c $(HDRS) | $(OBJ)
	$(call compile)
$(OBJ)/%.o: $(LIBC)/net/named/%.c $(HDRS) | $(OBJ)
	$(call compile)
$(OBJ)/%.o: $(LIBC)/ns/%.c $(HDRS) | $(OBJ)
	$(call compile)
$(OBJ)/%.o: $(MD)/sys/%.c $(HDRS) | $(OBJ)
	$(call compile)

$(OBJ)/%.o: $(MD)/sys/%.s $(MD)/crt/DEFS.inc $(NCC) | $(OBJ)
	@echo as $(notdir $<)
	@$(AS) -o $@ $<
$(OBJ)/%.o: $(MD)/gen/%.s $(MD)/crt/DEFS.inc $(NCC) | $(OBJ)
	@echo as $(notdir $<)
	@$(AS) -o $@ $<
$(OBJ)/%.o: $(MD)/net/%.s $(MD)/crt/DEFS.inc $(NCC) | $(OBJ)
	@echo as $(notdir $<)
	@$(AS) -o $@ $<
$(OBJ)/%.o: $(MD)/crt/%.s $(MD)/crt/DEFS.inc $(NCC) | $(OBJ)
	@echo as $(notdir $<)
	@$(AS) -o $@ $<

$(O)/crt0.o: $(MD)/csu/crt0.s $(MD)/crt/DEFS.inc $(NCC) | $(OBJ)
	$(AS) -o $@ $<

# The stubs: one file each, so that a program gets only the calls it
# makes.  Each is checked against the kernel's table of calls.
$(OBJ)/sys/made: $(MD)/sys/syscalls.tab $(TOP)/tools/mkstubs.py \
		$(TOP)/usr/include/syscall.h $(TOP)/usr/src/sys/sys/init_sysent.c \
		$(NCC) | $(OBJ)/sys
	rm -f $(OBJ)/sys/*.s $(OBJ)/sys/*.o
	python3 -I $(TOP)/tools/mkstubs.py --model $(MODEL) \
	    --sysent $(TOP)/usr/src/sys/sys/init_sysent.c -d $(OBJ)/sys \
	    $(MD)/sys/syscalls.tab $(TOP)/usr/include/syscall.h
	cd $(OBJ)/sys && for f in *.s; do \
	    $(SAS) -o $${f%.s}.o $$f || exit 1; done
	touch $@

$(O)/libc.lib: $(OBJS) $(OBJ)/sys/made $(MD)/Makefile
	@echo wlib libc.lib
	@rm -f $@ $(OBJ)/libc.rsp
	@for f in $(OBJS) $(OBJ)/sys/*.o; do echo "+$$f"; done > $(OBJ)/libc.rsp
	@wlib -q -n -b $@ @$(OBJ)/libc.rsp

$(O)/libc.a: $(OBJS) $(OBJ)/sys/made $(MD)/Makefile $(TOP)/tools/ar.py
	@echo ar libc.a
	@rm -f $@ $(O)/libc.lib
	@python3 -I $(TOP)/tools/ar.py $@ $(OBJS) $(OBJ)/sys/*.o

clean:
	rm -rf $(O)

endif
