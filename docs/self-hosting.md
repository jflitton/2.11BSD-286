# The system makes itself

The system is built where it is developed, by its own compiler,
assembler and linker compiled for that machine
([building](development.md#building)). It can also be built on
itself: with its sources on a disk, the 286 makes its own kernel, the
programs that load it, its libraries and its programs, each to the byte
what the other build makes, installs them, and starts again on what it
made.

## The sources

`tools/srcmk.py` writes what the system needs for that:

```sh
python3 tools/srcmk.py obj/src          # the Makefiles, and obj/src/proto
python3 tools/mkfs.py -s 64000 obj/src/proto obj/src.fs
```

`obj/src.fs` is a file system to mount on `/usr/src`: a partition of
the system's disk (`tools/mkdisk.py ... d=obj/src.fs`,
[disks](disks.md)). It has the sources in the directories they have in
the repository, 20 MB of them, and room for what is made of them, which
is left beside them. The root file system has the directory, and `/sys`
is a name for `/usr/src/sys`.

The Makefiles of the repository are GNU make's, and the system's own
`make` reads none of them: no pattern rules, no functions, no
conditionals. So `srcmk.py` writes Makefiles that it does read, with a
rule for every file, from the same tables: each of the repository's
Makefiles prints, for `make table`, what it is made of, and that is all
`srcmk.py` knows. What is to be changed is changed in the repository's
Makefile, and the system's is written again.

## Making it

On the system, as root:

```sh
mount /dev/wd0d /usr/src

cd /usr/src/sys/i286stand; make; make install    # /boot, the boot blocks
cd /usr/src/sys/PC; make; make install           # the kernel
cd /usr/src/lib/libc/i286; make; make install    # the C library, three models
cd /usr/src/i286
make libs; make install-libs                     # the other libraries
make progs; make install-progs                   # the programs
```

| in | `make` makes | `make install` |
|---|---|---|
| `sys/i286stand` | `boot`, `hdboot.bin`, `mbr.bin`, `fdboot.bin` | `/boot`, `/mdec/wduboot` |
| `sys/PC` | `unix`, and `unix.map` | `/unix`; the one that was is `/unix.old` |
| `lib/libc/i286/s`, `m`, `l` | `libc.a`, `crt0.o` | `/lib`, `/lib/m`, `/lib/l` |
| `i286` | `lib/MODEL/libNAME.a`; `bin/NAME` | `/usr/lib`, `/usr/lib/m`, `/usr/lib/l`; each program where the root file system has it, without its symbols, with its mode and owner |

A kernel that does not start is left behind by naming the other to
`/boot`: `wd(0,0)unix.old` ([booting](boot.md)). `make bin/NAME` in
`/usr/src/i286` makes one program. `make clean` in any of them removes
what was made.

A program is one command of `cc`, which compiles all its files, as it
is where the system is built; so a change to one file of `sendmail`
compiles all of them. The kernel and the C library have a rule for each
object. Everything is made again when a header changes.

Some programs' sources are themselves made, and the system makes those
as the other build does, in the same directories of `/usr/src/i286` as
that one has in `obj`: `yacc` and `lex` their parsers and scanners;
`awk`'s table by a program of `awk`'s own, compiled and run there;
`ex`'s and `tcsh`'s C with the messages taken out by `mkstr`
(`usr/src/i286/mkstr.c`, which both builds compile and run);
`battlestar`'s likewise by its own; `hack`'s files through the
preprocessor and its `crypt`.

## What it takes

On the machine the tests run, a 10 MHz 286 with 4 MB as 86Box has it
(which does it some eight times as fast as the machine would):

| | time on the 286 |
|---|---|
| `/boot`, the boot blocks and the kernel | 37 minutes |
| the C library, each model | 20 to 25 minutes |
| the other libraries, in the three models | 24 minutes |
| the 318 programs | 7 hours 10 minutes |
| installing the programs | 7 minutes |
| all of it | 9 and a half hours |

The compiler does some 20 lines a second ([performance](performance.md)).
Of the programs, `tcsh`, `ex`, `hack`, `sendmail` and `window` are half
an hour each or more.

## What is checked

`tests/self/run.sh` boots the system with that file system, has it make
everything above, and compares each object, each program, the kernel
and `/boot` with what the same sources make where the system is built:
1814 files. A library is compared by its members, and by which they
are and in what order, since the archive has the day it was made in
it. The libraries are installed before the
programs are linked, so the programs are linked with what the system
made; the programs are then installed, and each of the 317 as
installed, stripped, must be what the root file system has from the
other build. Last the
kernel is made to say that it was made there, installed with `/boot`,
and the machine started again: it must come up on that kernel, with
`init`, `getty`, `login` and the shell that it made.

`PARTS="stand kernel"` makes less (`stand`, `kernel`, `libc`, `libs`,
`progs`), and `ONLY="awk make"` only those programs.

## What it needed

- `make` is a program of the large model. It was compiled with the
  buffers of a machine with more memory, 28K of them on a stack of 8K,
  and died reading any Makefile; and what the small model leaves for a
  Makefile, the kernel's alone would fill. Its table of names was 1021
  long, and the programs' Makefile names 4000 files: it is 8191 now,
  from the heap, with a hash that spreads them. It gives a command 254
  arguments at most, so a library is put together a hundred objects at
  a time.
- `/lib/c2`, which has a whole function in memory, had no room for the
  kernel's two longest, `namei` and `tcp_input`; `cc` then gave the
  assembler the code as it was, and said nothing. `/lib/c2l` is `c2` of
  the large model, and `cc` gives it what `c2` cannot hold. It is the
  slower of the two, so it is not the one tried first.
- `cc -nd=NAME`, for the kernel's network, whose data is a segment of
  its own ([the network](network.md)).
- The compiler's second pass works out a floating constant's value
  itself, in integers (`ctod`, `lib/ccom/i286/c14.c`). It called
  `atof`, which on the 286 is one off in the last bit for a constant in
  ten or so, and where the system is built is the C library's there:
  eleven files of the mathematical library came out different, and
  every program that has such a constant.
- A file is named `/usr/src/...` to the compiler in both builds
  (`tools/ncc`): `assert` puts the name in the program.
- `/usr/include/DEFS.inc`, the library's conventions for a file of
  assembly, and `cc` has the assembler look there; and
  `/usr/include/netimp`, for `netstat`.

## What it does not make yet

- The files of the root file system that are not programs: the manual
  as formatted, the databases of `passwd`, `termcap`, `spell` and the
  time zones, the games' data ([programs](userland.md)). The tools
  that make them are Python, where the system is built.
- The system calls' stubs of the C library, which `tools/mkstubs.py`
  writes from `syscalls.tab`: `srcmk.py` runs it, and the system
  assembles what it wrote.
- The Makefiles themselves, as above: they are written from the
  repository's, and a change made to one on the system is lost when
  they are written again.
- The file system images and the disk (`tools/mkfs.py`,
  `tools/mkdisk.py`): the system installs what it makes onto the disk
  it runs from.
