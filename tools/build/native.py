"""Where the system is built with the cross compiler (TOOLS=cross), the
directories of usr/src whose objects are the system's own all the same:
made by the system's compiler, assembler and linker, as compiled for
this machine, for cc.py, as.py and ld.py.

    lib, usr.lib    the libraries, which the root has of the system's
                    own objects whoever made its programs
    usr.bin/f77, usr.bin/efl, usr.bin/struct, usr.bin/prof, ucb/pascal
                    older C than the cross compiler takes, as the
                    system's own compiler does (prof's unsigned UNIT,
                    of a typedef); and pascal's px of the
                    system's assembler and its pi of the system's
                    linker, in what pi puts at the head of an object
    a directory not in usr/src
                    the system's own tree, where a Makefile that
                    compiles at install time, as learn's does
                    getnum.c in /usr/share/learn/C, leaves an object
                    for the system's linker
"""

NATIVE = ("lib", "usr.lib", "usr.bin/f77", "usr.bin/efl", "usr.bin/struct",
          "usr.bin/prof", "ucb/pascal")


def native(here):
    """Whether HERE, a directory named from usr/src, is one of them."""
    return here.startswith("..") or \
        any(here == d or here.startswith(d + "/") for d in NATIVE)
