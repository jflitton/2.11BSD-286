# The programs that load the kernel.
#
# From the top of the repository, after `. tools/env.sh`:
#
#	make -C usr/src/sys/i286stand
#
# makes, in obj/stand:
#
#	fdboot.bin	the diskette's boot sector, which holds a table of
#			where on the diskette the kernel is (tools/mkfd.py)
#	mbr.bin		the first sector of a hard disk shared between
#			systems
#	hdboot.bin	the first sector of the system's part of a hard
#			disk, which loads
#	boot		the file /boot, which loads the kernel
#
# tools/mkdisk.py puts the two sectors on a disk image.  docs/boot.md
# has how they work.  GNU make.

TOP:=	$(abspath ../../../..)
SYS:=	$(TOP)/usr/src/sys
O=	$(TOP)/obj/stand
CRT=	$(TOP)/usr/src/lib/libc/i286/crt
CC=	$(TOP)/tools/cc286
# 131 and 107: calls to functions with no prototype, and functions that
# return nothing in particular.  121: a name after #endif.
CFLAGS=	-ms -I$(TOP)/usr/include -wcd=131 -wcd=107 -wcd=121

# What /boot may be with the memory its file does not hold: it is loaded
# at 4K and its stack is at the top of the first 64K (srt0.s).
BOOTMAX=0xE000

VPATH=	$(CRT)

all:	$(O) $(O)/fdboot.bin $(O)/mbr.bin $(O)/hdboot.bin $(O)/boot

$(O):
	mkdir -p $(O)

$(O)/%.bin: %.s
	nasm -f bin -o $@ $<

$(O)/%.o: %.s
	nasm -f obj -I$(CRT)/ -o $@ $<

$(O)/boot.o: boot.c $(wildcard $(SYS)/h/*.h $(SYS)/i286/*.h $(SYS)/i286at/*.h)
	@cd $(O) && { $(CC) $(CFLAGS) -o boot.o $(abspath $<) > boot.log 2>&1; \
	    r=$$?; cat boot.log; \
	    if [ $$r != 0 ] || grep -q 'Error!\|W112\|W102' boot.log; \
	    then rm -f boot.o; exit 1; fi; }

# srt0.o first: the program starts at its first byte.  The linker's
# "DOS .COM" is the layout wanted, code and data in one segment and
# nothing else in the file, and nothing here makes it start at 100h as
# DOS would have it.  Message 1014 is the lack of a stack segment.
$(O)/boot: $(addprefix $(O)/,srt0.o boot.o i4m.o i4d.o)
	cd $(O) && wlink op q format dos com name boot.com disable 1014 \
	    op nodefaultlibs,map=boot.map file srt0.o,boot.o,i4m.o,i4d.o
	@python3 -c 'import re, sys; \
	    n = int(re.search(r"Memory size:\s+(\w+)", open(sys.argv[1]).read()).group(1), 16); \
	    sys.exit("boot: %d bytes in memory; the limit is %d" % (n, $(BOOTMAX)) if n > $(BOOTMAX) else 0)' \
	    $(O)/boot.map
	mv $(O)/boot.com $@

clean:
	rm -rf $(O)
