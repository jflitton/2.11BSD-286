#!/bin/sh
# Test the C library: build it, build a test program with it as
# /etc/init on a RAM disk, and boot the kernel with that
# (usr/src/lib/libc/i286, tests/libc/init.c).
#
# With CCOM=1 the library and the test programs are compiled by the
# system's own compiler (tools/cc -ccom) and not the cross compiler.
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/libc needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/libc
if [ -n "$CCOM" ]; then
    name=libc-ccom
    cc="$top/tools/cc -ccom"
    make -s -C "$top/usr/src/lib/ccom/i286" host
else
    name=libc
    cc=$top/tools/cc
fi
out=$top/obj/tests/$name
mkdir -p "$out"

make -s -C "$top/usr/src/sys/PC" | grep -v '^cc \|^as ' || true
make -s -C "$top/usr/src/lib/libc/i286" ${CCOM:+CCOM=1} |
    grep -v '^cc \|^as \|^wlib \|^ar ' || true

cd "$out"
# Each for the small model, and for the medium as NAME-m, its code packed
# into segments of 8K so that there are several; and for the large as
# NAME-l, likewise, but not by the system's own compiler, which has no
# such model.
cp "$here/proto" proto
for p in init echoargs; do
    $cc ${CCOM:+-DNOLARGE} -o $p "$here/$p.c"
    $cc -mm -packcode 8192 -o $p-m "$here/$p.c"
    if [ -z "$CCOM" ]; then
        $cc -ml -packcode 8192 -o $p-l "$here/$p.c"
    fi
done
if [ -z "$CCOM" ]; then
    echo "f /bin/echoargs-l 755 echoargs-l" >> proto
    echo "f /bin/libc-l 755 init-l" >> proto
fi
# The library reads users from master.passwd (or its ndbm form, which
# this has not), and error messages from syserrlst.
cat > passwd <<'END'
root::0:0:The Super User:/:/bin/sh
test::9:20:A Test:/usr/test:/bin/csh
END
cat > master.passwd <<'END'
root::0:0::0:0:The Super User:/:/bin/sh
test::9:20::0:0:A Test:/usr/test:/bin/csh
END
python3 -I "$top/tools/mkerrlst.py" "$top/usr/src/usr.lib/liberrlst/errlst.c" syserrlst
cat > group <<'END'
wheel:*:0:root
staff:*:20:test
END
python3 -I "$top/tools/mkfs.py" -s 600 proto root.fs
python3 -I "$top/tests/harness/runtest.py" --until halting \
    --boot $name "$top/obj/PC/unix" root.fs
