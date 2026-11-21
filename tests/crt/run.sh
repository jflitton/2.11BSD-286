#!/bin/sh
# Test the compiler's helper routines (usr/src/lib/libc/i286/crt) in an
# emulator (see tests/harness/runtest.py).  With NATIVE=1, or
# NATIVE_PROGRAMS=1, the program is the work of the system's own
# compiler, assembler and linker, in obj/tests/crt-n, and not the cross
# compiler's (tests/harness/system.sh).
# Run from the top of the repository after `. tools/env.sh`.
set -e
top=$(cd "$(dirname "$0")/../.." && pwd)
crt=$top/usr/src/lib/libc/i286/crt
. "$top/tests/harness/system.sh"
name=crt
if [ "$NATIVE_PROGRAMS" = 1 ]; then
    mk -C "$top/usr/src/lib/ccom/i286" host
    name=crt-n
fi
export NATIVE_PROGRAMS
mkdir -p "$top/obj/tests/$name"
python3 -I "$top/tests/crt/gen.py" > "$top/obj/tests/$name/crt_test.c"
exec python3 -I "$top/tests/harness/runtest.py" $name \
    "$top/obj/tests/$name/crt_test.c" "$crt/i4m.s" "$crt/i4d.s" "$crt/chp.s"
