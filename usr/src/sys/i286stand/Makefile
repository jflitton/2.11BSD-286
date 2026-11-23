#	Makefile	the 286's /boot and boot blocks, after pdpstand's
#
# boot		/boot, which loads the kernel: the boot block loads it
#		at 4K, and what it may be with the memory its file does not
#		hold is BOOTMAX, for its stack is at the top of the first
#		64K (srt0.s)
# hdboot.bin	the boot block of a hard disk, the first sector of the
#		system's part of it, which loads /boot: /mdec/wduboot,
#		and /mdec/sduboot, the same
# flboot.bin	the boot block of a diskette with a file system:
#		/mdec/fluboot
# mbr.bin	the first sector of a hard disk shared between systems,
#		which starts the boot block of the part that is active
# fdboot.bin	the first sector of a diskette with no file system and
#		a kernel on it, which holds a table of where the kernel is
#
# as -b writes the bytes alone, and ld -b /boot's code and data as one
# segment and nothing else.  The kernel's selectors and how they all
# work are in docs/boot.md.  GNUmakefile makes the same where the system
# is built.

ROOT=
CRT=	../../lib/libc/i286/crt
VPATH=	${CRT}

CFLAGS=	-O -ms
BOOTMAX=57344

ALL=	fdboot.bin mbr.bin hdboot.bin flboot.bin boot

all: ${ALL}

.SUFFIXES: .bin

.s.bin:
	as -b -o $@ $<

.s.o:
	as -ms -I${CRT}/ -o $@ $<

.c.o:
	cc ${CFLAGS} -c $<

# srt0.o first: the program starts at its first byte.  It is boot only
# if it is not too big.
boot: srt0.o boot.o i4m.o i4d.o
	ld -b -M -o boot.new srt0.o boot.o i4m.o i4d.o > boot.map
	@awk '/^image: / && $$2 > ${BOOTMAX} { print "boot: " $$2 " bytes in memory; the limit is ${BOOTMAX}"; exit 1 }' boot.map
	mv boot.new boot

install: boot hdboot.bin flboot.bin
	install -c -m 744 boot ${ROOT}/boot
	cp hdboot.bin ${ROOT}/mdec/wduboot
	rm -f ${ROOT}/mdec/sduboot
	ln ${ROOT}/mdec/wduboot ${ROOT}/mdec/sduboot
	cp flboot.bin ${ROOT}/mdec/fluboot

clean:
	rm -f ${ALL} boot.new *.o boot.map

boot.o:		boot.c
srt0.o:		srt0.s
i4m.o:		i4m.s
i4d.o:		i4d.s
fdboot.bin:	fdboot.s
mbr.bin:	mbr.s
hdboot.bin:	hdboot.s
flboot.bin:	flboot.s
