# The C library for the 286, small model.
#
# From the top of the repository, after `. tools/env.sh`:
#
#	make -C usr/src/lib/libc/i286
#
# makes obj/lib/s/crt0.o, the start-up code every program is linked
# with first, and obj/lib/s/libc.lib, the library.  GNU make.
#
# Each variable a file defines without a value gets common storage
# (tools/commonize.py), as for programs (tools/cc).
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
# Left out for now: nlist (gen/nlist.c and pdp/gen/nsym.c read the
# PDP-11's a.out format, not this system's), profiling (mcrt0, gmon),
# and the PDP-11's compat-2.9.

TOP:=	$(abspath ../../../../..)
LIBC:=	$(TOP)/usr/src/lib/libc
MD:=	$(LIBC)/i286
O=	$(TOP)/obj/lib/s
OBJ=	$(O)/obj

CC=	$(TOP)/tools/cc286
# 131 and 107: calls to functions with no prototype, and functions that
# return nothing in particular; the tree's C is full of both.  121: a
# name after #endif.
CFLAGS=	-ms -I$(TOP)/usr/include -wcd=131 -wcd=107 -wcd=121
AS=	nasm -f obj -I$(MD)/crt/

GEN=	abort alarm atoi atol calloc closedir crypt ctime ctype_ daemon \
	devname disklabel err execvp fstab gcvt getenv getgrent \
	getgrgid getgrnam getlogin getgrouplist gethostname getpagesize \
	getpass getpwent getloadavg getmntinfo getttyent getttynam \
	getusershell getwd initgroups isatty isctype malloc mktemp ndbm \
	opendir perror popen psignal qsort random readdir regex scandir \
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
MDSYSS=	sigtramp
MDGEN=	ldexp modf setjmp
MDNET=	htons
CRT=	i4m i4d chp

COBJS=	$(GEN) $(GENC) $(PDPGEN) $(STDIO) $(STDLIB) $(STRING) $(COMPAT41) \
	$(COMPAT43) $(COMPAT5) $(INET) $(NET) $(NAMED) $(NS) $(MDSYS)
SOBJS=	$(MDSYSS) $(MDGEN) $(MDNET) $(CRT)
OBJS=	$(addprefix $(OBJ)/,$(addsuffix .o,$(COBJS) $(SOBJS)))

all:	$(O)/crt0.o $(O)/libc.lib

$(OBJ) $(OBJ)/sys:
	mkdir -p $@

# Every object is made again when any header changes: crude, and never
# wrong.
HDRS:=	$(wildcard $(TOP)/usr/include/*.h $(TOP)/usr/src/sys/h/*.h \
	$(TOP)/usr/src/sys/i286/*.h $(LIBC)/stdio/*.h)

define compile
	@echo cc $(notdir $<)
	@cd $(OBJ) && { $(CC) $(CFLAGS) $(1) -o $*.o $< > $*.log 2>&1; r=$$?; \
	    cat $*.log; \
	    if [ $$r != 0 ] || grep -q 'Error!' $*.log; \
	    then rm -f $*.o; exit 1; fi; }
	@python3 -I $(TOP)/tools/commonize.py $(OBJ)/$*.o
endef

$(OBJ)/%.o: $(LIBC)/gen/%.c $(HDRS) | $(OBJ)
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

$(OBJ)/%.o: $(MD)/sys/%.s | $(OBJ)
	@echo as $(notdir $<)
	@$(AS) -o $@ $<
$(OBJ)/%.o: $(MD)/gen/%.s | $(OBJ)
	@echo as $(notdir $<)
	@$(AS) -o $@ $<
$(OBJ)/%.o: $(MD)/net/%.s | $(OBJ)
	@echo as $(notdir $<)
	@$(AS) -o $@ $<
$(OBJ)/%.o: $(MD)/crt/%.s | $(OBJ)
	@echo as $(notdir $<)
	@$(AS) -o $@ $<

$(O)/crt0.o: $(MD)/csu/crt0.s | $(OBJ)
	$(AS) -o $@ $<

# The stubs: one file each, so that a program gets only the calls it
# makes.  Each is checked against the kernel's table of calls.
$(OBJ)/sys/made: $(MD)/sys/syscalls.tab $(TOP)/tools/mkstubs.py \
		$(TOP)/usr/include/syscall.h $(TOP)/usr/src/sys/sys/init_sysent.c \
		| $(OBJ)/sys
	rm -f $(OBJ)/sys/*.s $(OBJ)/sys/*.o
	python3 -I $(TOP)/tools/mkstubs.py \
	    --sysent $(TOP)/usr/src/sys/sys/init_sysent.c -d $(OBJ)/sys \
	    $(MD)/sys/syscalls.tab $(TOP)/usr/include/syscall.h
	cd $(OBJ)/sys && for f in *.s; do \
	    nasm -f obj -o $${f%.s}.o $$f || exit 1; done
	touch $@

$(O)/libc.lib: $(OBJS) $(OBJ)/sys/made $(MD)/Makefile
	@echo wlib libc.lib
	@rm -f $@ $(OBJ)/libc.rsp
	@for f in $(OBJS) $(OBJ)/sys/*.o; do echo "+$$f"; done > $(OBJ)/libc.rsp
	@wlib -q -n -b $@ @$(OBJ)/libc.rsp

clean:
	rm -rf $(O)
