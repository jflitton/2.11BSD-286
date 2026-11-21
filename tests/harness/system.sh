# What the suites that boot the system as a machine has it are to boot
# it of: sourced by tests/system, tests/network, tests/programs,
# tests/bench, tests/libc and tests/ccom, with top set to the top of the
# repository.  The system is built either of two ways, both supported:
# by the cross compiler, Open Watcom, or by the system's own compiler,
# assembler and linker as they are built here (docs/toolchain.md).
#
#   NATIVE=1           the system's own tools': the kernel, the C library
#                      and the others, and every program on the disk
#   NATIVE=0           the cross compiler's
#   NATIVE_KERNEL=     the kernel the one way (usr/src/sys/PC, obj/PC, or
#                      with CCOM=1 obj/nPC) whatever NATIVE says
#   NATIVE_PROGRAMS=   the programs and the libraries likewise
#                      (usr/src/i286 and usr/src/lib/libc/i286, obj/bin
#                      and obj/lib, or with CCOM=1 obj/nbin and obj/nlib)
#   NATIVE_BOOT=       /boot and the boot blocks likewise
#                      (usr/src/sys/i286stand, obj/stand, or with CCOM=1
#                      obj/nstand); it is exported, for the diskette's
#                      boot sector (tests/harness/runtest.py)
#
# Without NATIVE it is DEFAULT, below: which build is the default is
# that line and nothing else.  The libraries on the disk, for cc there,
# are the system's own tools' work whichever it is (usr/src/i286,
# syslibs).  This says where the parts are:
#
#   kernel   the kernel that is /unix
#   bin      the programs, each with its symbols
#   ccflag   what tools/cc is given to make a program as those were made
#   stand    the boot programs, the boot sectors and /boot
#   case     what the name of a suite's directory in obj/tests has after
#            it, so that one case does not overwrite another: nothing
#            for the cross compiler's, -n for the system's own tools',
#            and -n with k, p and b after it for their kernel, their
#            programs and their boot programs when it is not all three
#
# and buildsys makes them, and the root file system of them,
# obj/root/root.fs, which it fails without.  One case at a time: the
# root is the same file whichever it is.

DEFAULT=0

quiet() { grep -v '^cc \|^as \|^wlib \|^ar \|Warning!\|Note!\|^unix:' || true; }

# make, quietly, and the suite ends if it fails.
mk() {
    if ! make -s "$@" > "$top/obj/tests/make.$$.log" 2>&1; then
        quiet < "$top/obj/tests/make.$$.log"
        rm -f "$top/obj/tests/make.$$.log"
        echo "$(basename "$0"): make $* failed" >&2
        exit 1
    fi
    quiet < "$top/obj/tests/make.$$.log"
    rm -f "$top/obj/tests/make.$$.log"
}

NATIVE=${NATIVE:-$DEFAULT}
NATIVE_KERNEL=${NATIVE_KERNEL:-$NATIVE}
NATIVE_PROGRAMS=${NATIVE_PROGRAMS:-$NATIVE}
NATIVE_BOOT=${NATIVE_BOOT:-$NATIVE}
export NATIVE_BOOT
for v in "$NATIVE" "$NATIVE_KERNEL" "$NATIVE_PROGRAMS" "$NATIVE_BOOT"; do
    case $v in
    0|1) ;;
    *) echo "$(basename "$0"): NATIVE and the others are 0 or 1" >&2; exit 2 ;;
    esac
done
kernel=$top/obj/PC/unix
bin=$top/obj/bin
ccflag=
stand=$top/obj/stand
if [ "$NATIVE_KERNEL" = 1 ]; then
    kernel=$top/obj/nPC/unix
fi
if [ "$NATIVE_PROGRAMS" = 1 ]; then
    bin=$top/obj/nbin
    ccflag=-ccom
fi
if [ "$NATIVE_BOOT" = 1 ]; then
    stand=$top/obj/nstand
fi
case $NATIVE_KERNEL$NATIVE_PROGRAMS$NATIVE_BOOT in
000) case= ;;
111) case=-n ;;
*)  case=-n
    if [ "$NATIVE_KERNEL" = 1 ]; then case=${case}k; fi
    if [ "$NATIVE_PROGRAMS" = 1 ]; then case=${case}p; fi
    if [ "$NATIVE_BOOT" = 1 ]; then case=${case}b; fi ;;
esac
mkdir -p "$top/obj/tests"

# The kernel, the boot programs and the C library.
buildbase() {
    if [ "$NATIVE_KERNEL" = 1 ]; then
        mk -C "$top/usr/src/sys/PC" CCOM=1
    else
        mk -C "$top/usr/src/sys/PC"
    fi
    if [ "$NATIVE_BOOT" = 1 ]; then
        mk -C "$top/usr/src/sys/i286stand" CCOM=1
    else
        mk -C "$top/usr/src/sys/i286stand"
    fi
    if [ "$NATIVE_PROGRAMS" = 1 ]; then
        mk -C "$top/usr/src/lib/ccom/i286" host
        mk -C "$top/usr/src/lib/libc/i286" CCOM=1
    else
        mk -C "$top/usr/src/lib/libc/i286"
    fi
}

# And the programs, and the root file system of them.  A program that
# tests/ccom/programs.sh had the cross compiler make in obj/nbin, which
# obj/nbin/cross lists, is made again by the system's own.  The root is
# made with CCOM=1 for those, and not with O=obj/nbin alone, which would
# have the cross compiler make again there whatever it found out of date.
buildsys() {
    buildbase
    if [ "$NATIVE_PROGRAMS" = 1 ]; then
        if [ -s "$bin/cross" ]; then
            (cd "$bin" && rm -f $(cat cross)) && : > "$bin/cross"
        fi
        mk -C "$top/usr/src/i286" CCOM=1 all
        mk -C "$top/usr/src/i286" CCOM=1 root KERNEL="$kernel" STAND="$stand"
    else
        mk -C "$top/usr/src/i286" all
        mk -C "$top/usr/src/i286" root KERNEL="$kernel" STAND="$stand"
    fi
}
