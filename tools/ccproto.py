#!/usr/bin/env python3
"""List what a program is compiled with on the system, for tools/mkfs.py.

usage: ccproto.py TOP LIBRARY ... > cc.proto

The lines are for the root file system's proto (usr/src/i286/root.proto
takes them in): /usr/include, and the libraries.

/usr/include is TOP/usr/include as the tree has it: its own files, and
the kernel's headers that it reaches through links, sys (sys/h), i286
with machine as another name for it (sys/i286), net, netinet and
netns.  What is of the PDP-11 alone is left out (pdp, pdpif, pdpuba,
netimp), and pascal's.  A header that is a link in the tree is one
there.

The libraries are the system's own objects (a.out.h), as its own
compiler, assembler and tools/ar.py made them in TOP/obj/nlib: for the
small model /lib/crt0.o and /lib/libc.a, and /usr/lib/libNAME.a for
each LIBRARY; for the medium model the same in /lib/m and /usr/lib/m,
where the system's ld looks when it is given -mm.  lex's library,
libln.a, is libl.a as well, a link, as upstream installs it: -ll.
"""
import os
import sys

LINKED = ["sys", "i286", "net", "netinet", "netns"]
OWN = ["arpa", "protocols"]


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    top = sys.argv[1]
    libs = sys.argv[2:]
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

    nlib = os.path.join(top, "obj", "nlib")
    out += ["d /lib 755", "d /lib/m 755", "d /usr/lib 755", "d /usr/lib/m 755"]
    for m, sub in (("s", ""), ("m", "/m")):
        out.append("f /lib%s/crt0.o 444 %s" % (sub, os.path.join(nlib, m, "crt0.o")))
        out.append("f /lib%s/libc.a 444 %s" % (sub, os.path.join(nlib, m, "libc.a")))
        for l in libs:
            out.append("f /usr/lib%s/lib%s.a 444 %s" %
                       (sub, l, os.path.join(nlib, m, "lib%s.a" % l)))
            if l == "ln":
                out.append("h /usr/lib%s/libl.a /usr/lib%s/libln.a" %
                           (sub, sub))
    print("\n".join(out))


if __name__ == "__main__":
    main()
