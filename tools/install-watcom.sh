#!/bin/sh
# Install the Open Watcom v2 cross-compiler for this host.
#
# usage: tools/install-watcom.sh
#
# Unpacks the build of Open Watcom that matches this machine, and its
# manuals, from reference/toolchain/open-watcom-2.0-2026-11-01.tar.xz
# (tools/fetch-reference.py toolchain) into toolchain/openwatcom.  Nothing is
# installed outside the repository.  Afterwards `. tools/env.sh` puts the
# tools on PATH.
#
# Only the compiler, assembler, linker and object tools are used.  Open
# Watcom's own C library and headers are left in the archive: the system is
# built against its own.
set -e
top=$(cd "$(dirname "$0")/.." && pwd)
archive=$top/reference/toolchain/open-watcom-2.0-2026-11-01.tar.xz
dest=$top/toolchain/openwatcom

case "$(uname -s)-$(uname -m)" in
    Darwin-arm64)   bin=armo64 ;;
    Darwin-x86_64)  bin=bino64 ;;
    Linux-x86_64)   bin=binl64 ;;
    Linux-aarch64)  bin=arml64 ;;
    *) echo "no Open Watcom build for $(uname -s) $(uname -m)" >&2; exit 1 ;;
esac

if [ ! -f "$archive" ]; then
    echo "missing $archive; run tools/fetch-reference.py toolchain" >&2
    exit 1
fi

rm -rf "$dest"
mkdir -p "$dest"
tar xJf "$archive" -C "$dest" "./$bin" ./docs ./license.txt ./readme.txt
ln -s "$bin" "$dest/bin"
echo "installed $bin in $dest"
WATCOM=$dest "$dest/bin/wcc" 2>&1 | sed -n 1,2p
