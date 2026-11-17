# Source this (`. tools/env.sh`) from the top of the repository to put the
# cross toolchain on PATH.  See tools/install-watcom.sh.
TOP=$(pwd)
if [ ! -x "$TOP/toolchain/openwatcom/bin/wcc" ]; then
    echo "no toolchain here; run tools/install-watcom.sh from the top of the repository" >&2
else
    WATCOM=$TOP/toolchain/openwatcom
    PATH=$WATCOM/bin:$PATH
    export TOP WATCOM PATH
fi
