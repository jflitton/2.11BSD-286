#!/bin/sh
# Test the compiler's helper routines (usr/src/lib/libc/i286/crt) in QEMU.
# Run from the top of the repository after `. tools/env.sh`.
set -e
top=$(cd "$(dirname "$0")/../.." && pwd)
crt=$top/usr/src/lib/libc/i286/crt
mkdir -p "$top/obj/tests/crt"
python3 -I "$top/tests/crt/gen.py" > "$top/obj/tests/crt/crt_test.c"
exec python3 -I "$top/tests/harness/runtest.py" crt \
    "$top/obj/tests/crt/crt_test.c" "$crt/i4m.s" "$crt/i4d.s" "$crt/chp.s"
