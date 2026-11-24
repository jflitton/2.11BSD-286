# The system, made where it is built.  GNU make reads this before the
# Makefile, which is the system's, upstream's.  From the top of the
# repository, after `. tools/env.sh` and making the kernel and the boot
# programs (sys/PC, sys/i286stand):
#
#	make -C usr/src
#
# makes the system with upstream's Makefiles, as make build and make
# installsrc make it on the system, and with etc's distribution, in a
# root of its own, obj/nbuild/dest, with the system's own compiler,
# assembler and linker as they are built for this machine
# (tools/build/build.sh, docs/development.md);
#
#	make -C usr/src root
#
# makes of that the root file system, obj/root/root.fs, with the kernel
# and the boot programs.  TOOLS=cross has the cross compiler, Open
# Watcom, make the programs, in obj/build.  KERNEL, STAND, ROOTDISK,
# ROOTSIZE and STRIP are build.sh's.

TOOLS?=	native
export TOOLS KERNEL STAND ROOTDISK ROOTSIZE STRIP

all root:
	@sh ../../tools/build/build.sh $(filter root,$@)

.PHONY:	all root
