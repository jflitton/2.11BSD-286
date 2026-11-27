#!/usr/bin/env python3
"""List /usr/include, which a program is compiled with on the system,
for tools/mkfs.py.

usage: ccproto.py TOP > include.proto

The lines are for the root file system's proto (tools/build/build.sh
adds them): /usr/include as the tree has it, TOP/usr/include, as
2.11BSD's distribution has it, which no Makefile installs.  Its own
files, and the kernel's headers that it reaches through links, sys
(sys/h), i286 with machine as another name for it (sys/i286), net,
netimp, netinet and netns.  What is of the PDP-11 alone is left out
(pdp, pdpif, pdpuba), and pascal's, unixio.h, which declares routines
external, as the VAX's Pascal compiler, pc, has them and pi has not;
netimp is not, netstat being compiled with it.  A header that is a link in the tree is one there.
DEFS.inc is there too, the C library's conventions for a file of
assembly (usr/src/lib/libc/i286/crt), where cc has the assembler look
for it.
"""
import os
import sys

LINKED = ["sys", "i286", "i286at", "net", "netimp", "netinet", "netns"]
OWN = ["arpa", "protocols"]


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    top = sys.argv[1]
    inc = os.path.join(top, "usr", "include")
    out = ["d /usr/include 755"]

    def headers(d, to):
        for name in sorted(os.listdir(d)):
            path = os.path.join(d, name)
            if not name.endswith(".h"):
                continue
            if os.path.islink(path):
                out.append("l %s/%s %s" % (to, name, os.readlink(path)))
            elif os.path.isfile(path):
                out.append("f %s/%s 444 %s" % (to, name, path))

    headers(inc, "/usr/include")
    for d in OWN + LINKED:
        out.append("d /usr/include/%s 755" % d)
        headers(os.path.realpath(os.path.join(inc, d)), "/usr/include/" + d)
    out.append("l /usr/include/machine i286")
    # What a file of assembly includes for the library's conventions,
    # where cc has the assembler look.
    out.append("f /usr/include/DEFS.inc 444 %s" % os.path.join(
        top, "usr", "src", "lib", "libc", "i286", "crt", "DEFS.inc"))

    print("\n".join(out))


if __name__ == "__main__":
    main()
