# What the suites that boot the system as a machine has it are to boot
# it of: sourced by tests/system, tests/network, tests/programs,
# tests/bench, tests/libc and tests/ccom, with top set to the top of the
# repository.  The system is built either of two ways, both supported:
# by the cross compiler, Open Watcom, or by the system's own compiler,
# assembler and linker as they are built here (docs/toolchain.md).
#
#   TOOLS=native         the system's own tools': the kernel, the C
#                        library and the others, every program on the
#                        disk, and the boot programs
#   TOOLS=cross          the cross compiler's
#   TOOLS_KERNEL=        native or cross: the kernel the one way
#                        (usr/src/sys/PC, obj/nPC or obj/PC) whatever
#                        TOOLS says
#   TOOLS_PROGRAMS=      the programs likewise (usr/src, which
#                        tools/build/build.sh makes with upstream's
#                        Makefiles in obj/nbuild or obj/build), and the
#                        C library that tools/cc makes the tests' own
#                        programs with (usr/src/lib/libc/i286, obj/nlib or
#                        obj/lib)
#   TOOLS_BOOT=          /boot and the boot blocks likewise
#                        (usr/src/sys/i286stand, obj/nstand or
#                        obj/stand)
#
# They are the Makefiles' TOOLS, and each make here is given it; the
# last two are exported, for the programs and the diskette's boot
# sector that tests/harness/runtest.py makes.
# Without TOOLS it is DEFAULT, below, as it is native in the Makefiles.
# The libraries on the disk, for cc there, and the compiler are the
# system's own tools' work whichever it is, as make build makes them
# (lib, usr.lib).  This says where the parts are:
#
#   kernel   the kernel that is /unix
#   bin      the programs, each with its symbols, by the names they are
#            installed by
#   made     usr/src as the build's Makefiles left it, with what they
#            made there and did not install (ntp's ntest)
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

DEFAULT=native

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

TOOLS=${TOOLS:-$DEFAULT}
TOOLS_KERNEL=${TOOLS_KERNEL:-$TOOLS}
TOOLS_PROGRAMS=${TOOLS_PROGRAMS:-$TOOLS}
TOOLS_BOOT=${TOOLS_BOOT:-$TOOLS}
export TOOLS_PROGRAMS TOOLS_BOOT
for v in "$TOOLS" "$TOOLS_KERNEL" "$TOOLS_PROGRAMS" "$TOOLS_BOOT"; do
    case $v in
    native|cross) ;;
    *) echo "$(basename "$0"): TOOLS and the others are native or cross" >&2
        exit 2 ;;
    esac
done
kernel=$top/obj/PC/unix
bin=$top/obj/build/syms
made=$top/obj/build/root/usr/src
ccflag=
stand=$top/obj/stand
if [ "$TOOLS_KERNEL" = native ]; then
    kernel=$top/obj/nPC/unix
fi
if [ "$TOOLS_PROGRAMS" = native ]; then
    bin=$top/obj/nbuild/syms
    made=$top/obj/nbuild/root/usr/src
    ccflag=-ccom
fi
if [ "$TOOLS_BOOT" = native ]; then
    stand=$top/obj/nstand
fi
case $TOOLS_KERNEL$TOOLS_PROGRAMS$TOOLS_BOOT in
crosscrosscross) case= ;;
nativenativenative) case=-n ;;
*)  case=-n
    if [ "$TOOLS_KERNEL" = native ]; then case=${case}k; fi
    if [ "$TOOLS_PROGRAMS" = native ]; then case=${case}p; fi
    if [ "$TOOLS_BOOT" = native ]; then case=${case}b; fi ;;
esac
mkdir -p "$top/obj/tests"

# The kernel, the boot programs and the C library.
buildbase() {
    if [ "$TOOLS_KERNEL" = native ]; then
        mk -C "$top/usr/src/sys/PC" TOOLS=native
    else
        mk -C "$top/usr/src/sys/PC" TOOLS=cross
    fi
    if [ "$TOOLS_BOOT" = native ]; then
        mk -C "$top/usr/src/sys/i286stand" TOOLS=native
    else
        mk -C "$top/usr/src/sys/i286stand" TOOLS=cross
    fi
    if [ "$TOOLS_PROGRAMS" = native ]; then
        mk -C "$top/usr/src/lib/ccom/i286" host
        mk -C "$top/usr/src/lib/libc/i286" TOOLS=native
    else
        mk -C "$top/usr/src/lib/libc/i286" TOOLS=cross
    fi
}

# And the programs, and the root file system of them, made with
# upstream's Makefiles (usr/src/GNUmakefile).
buildsys() {
    buildbase
    mk -C "$top/usr/src" TOOLS="$TOOLS_PROGRAMS" root KERNEL="$kernel" \
        STAND="$stand"
}
