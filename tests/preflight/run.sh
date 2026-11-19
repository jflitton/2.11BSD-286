#!/bin/sh
# Test tools/preflight.py: a program of two files (tests/preflight/prog)
# with one of each thing it looks for across files, and calls that are
# right, which it must say nothing about.
# Run from the top of the repository after `. tools/env.sh`, with the
# libraries built.
top=$(cd "$(dirname "$0")/../.." && pwd)
out=$(python3 -I "$top/tools/preflight.py" -v "$top/tests/preflight/prog")
fail=0
while read -r want; do
    if ! printf '%s\n' "$out" | /usr/bin/grep -qF -- "$want"; then
        echo "tests/preflight: not found: $want" >&2
        fail=1
    fi
done <<'END'
prog: 2 files
  compiles
  wrong size: 4
atol (lib/libc/gen/atol.c): an int to the caller, which has no other declaration: returns 'long'
prog/a.c:25
big (tests/preflight/prog/b.c): an int to the caller, which has no other declaration: returns 'long'
prog/a.c:21
lseek (a system call): argument 2 is 'int', taken as 'long'
prog/a.c:24
seek (tests/preflight/prog/b.c): argument 2 is 'int', taken as 'long'
prog/a.c:17
  count: 1
sum (tests/preflight/prog/b.c): takes 2
prog/a.c:16
  wrong size in the medium model: 1
handle (tests/preflight/prog/b.c): argument 1 is 'int', taken as 'int (*)()'
prog/a.c:19
  same size: 1
say (tests/preflight/prog/b.c): argument 1 is 'int', taken as 'char *'
prog/a.c:20
  build: links, small model
large frames: half 2
END
if [ $fail != 0 ]; then
    printf '%s\n' "$out" >&2
    exit 1
fi
echo "tests/preflight: passed"
