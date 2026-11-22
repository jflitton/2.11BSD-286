# The system makes itself

The system is built where it is developed, by its own compiler,
assembler and linker compiled for that machine
([building](development.md#building)). It can also be built on
itself: with its sources on a disk, the 286 makes its own kernel, the
programs that load it, its libraries, its programs, and the databases,
tables and manual it has besides, each to the byte what the other build
makes, installs them, and starts again on what it made.

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
make data; make install-data                     # what is not a program
```

| in | `make` makes | `make install` |
|---|---|---|
| `sys/i286stand` | `boot`, `hdboot.bin`, `mbr.bin`, `fdboot.bin` | `/boot`, `/mdec/wduboot` |
| `sys/PC` | `unix`, and `unix.map` | `/unix`; the one that was is `/unix.old` |
| `lib/libc/i286/s`, `m`, `l` | `libc.a`, `crt0.o` | `/lib`, `/lib/m`, `/lib/l` |
| `i286` | `lib/MODEL/libNAME.a`; `bin/NAME` | `/usr/lib`, `/usr/lib/m`, `/usr/lib/l`; each program where the root file system has it, without its symbols, with its mode and owner |
| `i286`, `make data` | [what is not a program](#what-is-not-a-program) | each where the root file system has it |

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

## What is not a program

`make data` makes the files the system has that are neither programs
nor copies of a source, with the system's own programs, where the
other build has a program of the machine it runs on, or of Python,
that does what they do. `make data-NAME` makes one kind:

| | made | by |
|---|---|---|
| `etc` | `/etc/passwd` and its database; the database of `/etc/aliases`; `/etc/fstab` | `mkpasswd -p`; `sendmail -bi`; `sed` |
| | `/etc/syserrlst`; `/usr/share/misc/termcap` | `mkerrlst`, compiled there; `ex`, as `share/termcap`'s Makefile runs it |
| `roff` | `mdoc`'s macros and `-me`'s without their comments; the 25 tables of `/usr/share/term` | `sed`; each table's C compiled with `mktab` and run |
| `learn` | the two objects `learn`'s lessons in C link with | `cc` |
| `spell` | `/usr/dict/hlista`, `hlistb`, `hstop` | `spellin` |
| `zones` | `/usr/share/zoneinfo` | `zic` |
| `games` | `fortunes.dat`; `monop`'s cards; `boggle`'s dictionary; `phantasia`'s files and help; `cribbage`'s instructions; `warp`'s maps | the games' own `strfile`, `initdeck`, `comp`, `setfiles` and `sm`, compiled there; `nroff` |
| `man` | the 621 pages of `/usr/man/cat1` to `cat8`, and `whatis` | `nroff -man -h`; `sed` and `sort` |

Each is made in the directory of `/usr/src/i286` that the other build
has it in in `obj`: `root/termcap`, `spell/hlista`, `zoneinfo/GMT`,
`games/fortunes.dat`, `man/cat1/ls.0`. `make install-data` puts each
where the system has it, and gives a file's other names to the new one:
a zone's, a page's (317 of them), a table's. The password file and its
database, the aliases' and `fstab` are made and not installed: they are
a new system's, and a system that is in use has its own users and
disks.

`/usr/games/lib/battle_strings`, and the files of `ex`'s messages and
`tcsh`'s, are made with those programs and installed with them.

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
| what is not a program, and installing it | 2 hours 40 minutes |
| all of it | 12 hours |

The compiler does some 20 lines a second ([performance](performance.md)).
Of the programs, `tcsh`, `ex`, `hack`, `sendmail` and `window` are half
an hour each or more.

## What is checked

`tests/self/run.sh` boots the system with that file system, has it make
everything above, and compares each object, each program, the kernel
and `/boot`, and each file that is not a program, with what the same
sources make where the system is built: 2600 files. A library is
compared by its members, and by which they are and in what order, since
the archive has the day it was made in it; a page of the manual has at
its foot the day it was formatted, and two that differ are compared
again without those lines. The libraries are installed before the
programs are linked, so the programs are linked with what the system
made; the programs are then installed, and each of the 317 as
installed, stripped, must be what the root file system has from the
other build. What is not a program is made by the programs the system
has just made and installed, and is installed too: each of 1103 files
and other names of one must be what was made. Last the
kernel is made to say that it was made there, installed with `/boot`,
and the machine started again: it must come up on that kernel, with
`init`, `getty`, `login` and the shell that it made.

`PARTS="stand kernel"` makes less (`stand`, `kernel`, `libc`, `libs`,
`progs`, `data`), `ONLY="awk make"` only those programs, and
`DATA="man zones"` only those kinds of what is not a program.

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
- A file that is to be the same each time it is made cannot have in it
  what a program's memory happened to hold, or the time. `setfiles`
  wrote the members it had not set of `phantasia`'s structures as its
  stack had them, and `initdeck` the addresses of its tables: both
  write zeros now. `strfile -r` seeds its generator with the time; the
  one that makes `fortunes.dat` is linked with a `random()` that is
  `rand()` from its first number (`usr/src/i286/norandom.c`), as the
  other build's stand-in for it has it.
- `mktab` wrote a table's numbers with `%d`, and `nroff` reads digits
  and nothing else: where an `int` is 16 bits, the bits a printer's
  table has `nroff` clear of the terminal's modes are a negative
  number, which it read as 0. The tables the other build made were
  right, an `int` being longer there; `mktab` writes them unsigned.
- `make install-progs` puts the files of `ex`'s messages, `tcsh`'s and
  `battlestar`'s in place with those programs: each has in it where in
  its file a message is.

## What it does not make yet

- The files of the root file system that are copies of a source, or
  empty: `/etc`'s, the headers, the logs. Nothing makes them; a new
  disk would have them from `/usr/src`.
- The system calls' stubs of the C library, which `tools/mkstubs.py`
  writes from `syscalls.tab`: `srcmk.py` runs it, and the system
  assembles what it wrote.
- The Makefiles themselves, as above: they are written from the
  repository's, and a change made to one on the system is lost when
  they are written again.
- The file system images and the disk (`tools/mkfs.py`,
  `tools/mkdisk.py`): the system installs what it makes onto the disk
  it runs from.
