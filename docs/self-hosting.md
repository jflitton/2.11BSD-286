# The system makes itself

The system is built where it is developed, by its own compiler,
assembler and linker compiled for that machine
([building](development.md#building)). It can also be built on
itself: with its sources on a disk, the 286 makes its own kernel, the
programs that load it, its libraries, its programs, and the databases,
tables and manual it has besides, each to the byte what the other build
makes, installs them, and starts again on what it made; and it makes of
another disk one that starts the system, with all of that and the
sources on it, as 2.11BSD has a disk made.

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

The programs are made from the Makefiles of their own directories,
which are upstream's: `usr/src/bin/Makefile`, `usr/src/bin/sh/Makefile`
and the rest, as 2.11BSD has them, changed where the 286 is not the
PDP-11 ([below](#upstreams-makefiles)). They are in the repository and
on the system the same, and a change made to one there is a change to
the system's sources.

The kernel, the programs that load it, the libraries and what is not a
program are not made from such yet. The repository's Makefiles for
those are GNU make's, and the system's own `make` reads none of them:
no pattern rules, no functions, no conditionals. So `srcmk.py` writes
Makefiles that it does read, with a rule for every file, from the same
tables: each of the repository's Makefiles prints, for `make table`,
what it is made of, and that is all `srcmk.py` knows. What is to be
changed is changed in the repository's Makefile, and the system's is
written again.

## Making it

On the system, as root:

```sh
mount /dev/wd0d /usr/src

cd /usr/src/sys/i286stand; make; make install    # /boot, the boot blocks
cd /usr/src/sys/PC; make; make install           # the kernel
cd /usr/src/lib/libc/i286; make; make install    # the C library, three models
cd /usr/src/i286
make libs; make install-libs                     # the other libraries
make data; make install-data                     # what is not a program

cd /usr/src/bin; make; make install              # the programs, and so
cd /usr/src/sbin; make; make install             # in libexec, ucb, usr.bin,
                                                 # usr.sbin, games and new
```

| in | `make` makes | `make install` |
|---|---|---|
| `sys/i286stand` | `boot`, `hdboot.bin`, `flboot.bin`, `mbr.bin`, `fdboot.bin` | `/boot`, `/mdec/wduboot`, `/mdec/fluboot` |
| `sys/PC` | `unix`, and `unix.map` | `/unix`; the one that was is `/unix.old` |
| `lib/libc/i286/s`, `m`, `l` | `libc.a`, `crt0.o` | `/lib`, `/lib/m`, `/lib/l` |
| `i286` | `lib/MODEL/libNAME.a` | `/usr/lib`, `/usr/lib/m`, `/usr/lib/l` |
| `bin`, `sbin`, `libexec`, `ucb`, `usr.bin`, `usr.sbin`, `games`, `new` | each program, beside its sources, and its page of the manual | each where upstream's Makefile installs it, with the mode, owner and group it gives |
| `i286`, `make data` | [what is not a program](#what-is-not-a-program) | each where the root file system has it |

A kernel that does not start is left behind by naming the other to
`/boot`: `wd(0,0)unix.old` ([booting](boot.md)). `make` in a program's
own directory, `/usr/src/bin/sed`, makes that one. `make clean` in any
of them removes what was made.

The kernel and the C library have a rule for each object, and are made
again when a header changes. The C library's stubs for the system
calls are written there too, a file of assembly for each call and each
model, from `lib/libc/i286/sys/syscalls.tab`: by `mkstubs`
(`sys/mkstubs.c` beside it), which both builds compile and run, and
which has each agree with the kernel's table of calls.

## Upstream's Makefiles

A program's Makefile is the one 2.11BSD has for it, and makes it as
that says: `yacc` and `lex` their parsers and scanners; `awk`'s table
by a program of `awk`'s own; `csh`, `make`, `fsck`, `yacc` and `Mail`
with their strings taken out by `xstr`; `ex` and `tcsh` with their
messages taken out first, into `/usr/share/misc/exstrings` by `mkstr`
and into `/etc/tcsh6.00strgs` by `tcsh`'s own `mkprstr`; the page of
the manual with the program. The other build follows it: the files in
the order the Makefile links them, `xstr`, `mkstr` and `mkprstr`
compiled for the machine it runs on (`tools/xstrcc`), and in the root
the place, mode and other names upstream installs with. What was
changed in them for the 286:

- A program that is more than the small model has the model in its
  Makefile, `-mm` or `-ml`, where the PDP-11 had overlays (`ld -Z`):
  `csh`, `tcsh`, `ex`, `sendmail`, `rogue`, `warp`, `hack` and
  `phantasia` the medium, `make` and `window` the large.
- One whose stack is more than the 8K a program is linked with has
  `-stack N`: `ftp`, `ftpd`, `rshd`, `rexecd`, `tcpd` and the others of
  its directory, `Mail`, `sendmail`, `refer`, `sortbib`, `syslogd`;
  `dprog` has less, for the room in its data.
- `bin/as` and `bin/ld` make the 286's assembler and linker, which are
  in `i286` there; `timed`'s checksum is `i286/cksum.c`, as the compiler
  leaves it.
- A directory's list of programs has not the ones the port does not
  have: the PDP-11's debugger and tape formats, Fortran and Pascal,
  `uucp`, `lpr`, the quotas. `usr/src/local`, whose programs all
  compile, is not made.
- `-i`, `-n` and `-z`, the PDP-11's separate instruction and data
  spaces, are left where they are: the system's `ld` takes them and
  does nothing.

`/lib/cpp` says `unix` and `BSD2_11`, as upstream's does for the
PDP-11: the Makefiles do not, and the sources ask.

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

## A disk

A disk that starts the system is made as 2.11BSD has one made
(`usr/doc/2.10/setup.2.10`), by the same programs, with the system
itself where upstream has a tape: the root is a dump of the root of
the system that made and installed itself above, and the sources are
`/usr/src`. For the AT disk controller's second drive, as root (`sd1`
is the second SCSI disk):

```sh
disklabel wd1 > label           # the label it has, or the kernel's for it
ed label                        # its partitions, as below
disklabel -W wd1; disklabel -R -r wd1 label; disklabel -N wd1

newfs -i 8192 wd1a              # the root
dump 0f - /dev/rwd0a | restor rf - /dev/rwd1a
fsck -y /dev/rwd1a
disklabel -B wd1                # the boot block, /mdec/wduboot

newfs wd1d                      # the sources
mount /dev/wd1d /mnt
cd /usr/src; tar cf - . | (cd /mnt; tar xpf -)
cd /; umount /mnt
fsck -y -t /tmp/scratch /dev/rwd1d
```

The label is given a root, a swap area, the whole of the disk as is the
custom, and room for the sources, in sectors:

```
4 partitions:
  a: 96000 0 2.11BSD 1024 1024
  b: 32768 96000 swap
  c: 272000 0 unused 0 0
  d: 128000 128768 2.11BSD 1024 1024
```

- A disk with no label has one partition, all of what the system has
  of it ([disks](disks.md#whose-disk-it-is)), which `disklabel` has
  open and the new label makes smaller: `-W` has the kernel let it be
  changed, and `-N` not again. `-r` writes the sector, the kernel
  writing a label only over one.
- The root is to have no more inodes than `fsck` can check as the
  system starts, where it has no scratch file: 6000 on one of 48000
  blocks, which is `newfs -i 8192`
  ([userland](userland.md#tested-and-not)). It can be no smaller
  than the one that is dumped.
- `restor` writes the file system through its raw device and leaves
  the list of free blocks to `fsck`, as upstream's document says to
  have it checked; `fsck` also marks it clean, so that the system
  starts from it without checking it and starting again.
- The dump is of a root that is in use, and has what a running system
  writes: its logs, `/var/run`, `/tmp`. From single user it has less.
- `/etc/fstab` is the old disk's, which is right for a disk of the same
  kind that will be the first of it: `wd0`. One that will be `sd0` has
  it edited.
- `tar` takes what was made in `/usr/src` with the sources; `make
  clean` in each directory first leaves the sources alone.
- A disk shared with another system has the table of both in its first
  sector, which the system cannot write: it boots if what is there
  loads the boot block of the system's part, as `mbr.bin` does.

## A diskette

A diskette that starts the system is made as upstream has an RX02
made ([rx(4)](../usr/src/man/man4/rx.4)), with the kernel on it; the
root stays on the hard disk
([disks](disks.md#the-diskette-controller-sysi286atflc)). In a 3.5
inch drive, with a diskette of 1.44 MB:

```
flformat /dev/rfl0b
newfs -T fl1440 /dev/rfl0b
mount /dev/fl0b /mnt
cp /boot /unix /mnt
umount /mnt
dd if=/mdec/fluboot of=/dev/rfl0b count=1
```

In a 5.25 inch drive it is `fl1200`, a diskette of 1.2 MB. A diskette
of 720K is `fl0a` and `fl720`, and is for files: `newfs` and `mount`,
or `tar cf /dev/rfl0a`.

## A machine with nothing on its disk

It is started from a diskette that has, beside `/boot` and the kernel,
the file `miniroot`: a root file system of 800K that `/boot` loads into
memory with the kernel, with the programs that label a disk, make a
file system and restore a dump. 2.11BSD started its own such programs
from tape with no kernel under them, each with drivers of its own
(`sys/pdpstand`); the 286's kernel runs with its root in memory, so the
programs are the system's and the drivers the kernel's, as on 4.3BSD's
miniroot. The diskette is made after 4.3BSD's `buildmini`, on a
formatted diskette, and the root goes with it as a dump:

```
flformat /dev/rfl0b
sh /usr/src/sys/i286stand/buildmini fl0b fl1440
dump 0Bf 1440 /dev/rfl0b /dev/rwd0a
```

`dump` asks for the next diskette as each is full, 1440 blocks to
each; the root as it is made would take about twelve. On the new machine, started
from the first diskette, the shell is at the console, and the rest is
as [a disk](#a-disk) is made, the dump read from the diskettes:

```
disklabel wd0 > label
ed label
disklabel -W wd0
disklabel -R -r wd0 label
disklabel -N wd0
newfs -i 8192 wd0a
restor rf /dev/rfl0b /dev/rwd0a
fsck /dev/rwd0a
disklabel -B wd0
reboot
```

- The miniroot's devices are made by `/dev/MAKEDEV`, which is
  upstream's with the 286's devices in place of the PDP-11's
  (`usr/src/i286/etc/MAKEDEV`); the root's own are still listed in
  `root.proto`, and the two agree.
- The machine needs 800K above 1M for the miniroot, and what the
  programs take beside.
- `/boot` loads `miniroot` only from a diskette.

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
| a disk of it, with the sources | 20 minutes, on the emulator's disks, which answer at once |

The compiler does some 20 lines a second ([performance](performance.md)).
Of the programs, `tcsh`, `ex`, `hack`, `sendmail` and `window` are half
an hour each or more.

## What is checked

`tests/self/run.sh` boots the system with that file system, has it make
everything above, and compares each object, each program, the kernel
and `/boot`, and each file that is not a program, with what the same
sources make where the system is built: 2948 files, the stubs'
assembly among them. A library is
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
`data`), and `DATA="man zones"` only those kinds of what is not a
program.

`tests/self/progs.sh` has the system make the programs, from the
Makefiles of their own directories: `make` and `make install` in each
of `bin`, `sbin`, `libexec`, `ucb`, `usr.bin`, `usr.sbin`, `games` and
`new`, or in the directories it is given (`sh tests/self/progs.sh
bin/sed usr.bin/yacc`). Each file that was installed is to be what the
root had there before, which is what the other build makes, with the
mode it had, and each of the root's programs whose sources are in those
directories is to have been installed. The manual's pages are not
compared, having the day they were formatted in them, nor is a file's
owner and group.

All of it is half a day of the 286's time, which is for a release and
not for a change. A change is checked two ways, and
`sh tests/self/changed.sh` does both:

- Booting nothing. The Makefiles are run where the system is built,
  by that machine's `make`, with the system's compiler, assembler and
  linker as they are compiled for it. `sh tests/make/run.sh` runs
  upstream's, `make`, `make install` and `make clean` in each
  directory, with that machine's `make` given the rules the system's
  has and none of its own, and compares what was installed with the
  root file system: 491 files in four minutes, each with the contents
  and the mode the root has. `HOST=1 sh tests/self/run.sh` runs the
  ones `srcmk.py` writes, for `/boot`, the kernel and the libraries but
  the C library (`tests/self/host.sh`). That says the Makefiles name
  the right files, flags and order, and install each where the system
  has it. It does not say the system's `make` reads them so, nor that
  the tools do on the 286 what they do there. A program a Makefile
  makes and then runs is the 286's and does not run there:
  `tests/make/left` lists what is made with one, `awk`, `tcsh`, `hack`,
  `battlestar` and nine more programs, and the tables and games' files
  such programs write. `tests/make/differs` lists what the two builds
  do not yet make the same way: `sendmail`, whose Makefile takes its
  messages out with a script of its own; `phantasia`, one file of which
  its Makefile compiles without `-O`; and `backgammon`, whose Makefile
  writes the day into it.
- One machine is then booted to make what the changed files are part
  of and nothing else (`tests/self/select.py`): the programs a source
  is in, the libraries, the kernel, the C library or the boot programs.
  A change to the tools that do the making, or to a header of
  `/usr/include`, has the system make its compiler (`tests/ccom/self.sh`),
  the kernel and a program of each model, `cat`, `csh` and `make`. A
  change to the Makefiles or to what writes them has it make the five
  programs above. `changed.sh -n` says which, and why, and runs
  nothing.

Both boot the machine with a faster processor unless `CPU=286` is in
the environment: IBM's 486SLC3 at 100 MHz, an upgrade for this same
machine, in place of its 286 (`tests/harness/86box-486.cfg`). What the
system makes there is the same to the byte, in less than half the time:
five of the largest programs in 304 seconds where the 286 takes 710.
It says nothing of whether a 286 runs the system, which is for
`run.sh` and the other suites, on the 286.

`sh tests/self/split.sh` is all of it in two machines or three at
once, each with a fixed share: three is the most, since each keeps a
processor of the host busy for as long as it runs.

`tests/disk/run.sh` has the system make a disk that way, of a second
drive of 133 MB with nothing on it, not a label. `tools/fsck.py` must
find nothing wrong with the new disk's two file systems, and lists
each (`-l`): the root must have what the system's own has, name for
name, each file's contents, mode and owner, each other name, link and
device, but for what a system that is running writes; and the sources
what `/usr/src` has. Then the machine is started from that disk alone,
and must come up and make `/boot` and the boot block from its sources,
the same as the ones it started from.

`tests/floppy/run.sh` has it format a diskette of 1.44 MB that was
never formatted, in the second drive, and make of it one that starts
the system, as above. `tools/fsck.py` must find nothing wrong with it,
and it must hold `/boot` and `/unix` as the root has them, begin with
the boot block, and have no sector left as it was. The machine is then
started from it, with a hard disk the BIOS finds nothing to start on,
and there makes the same of a diskette of 1.2 MB in a 5.25 inch drive;
started from that one, it formats a diskette of 720K, writes the kernel
and the boot programs to it with `tar` and reads them back the same,
and makes a file system on it.

`tests/install/run.sh` has it make the diskette to install from, and
dump its root to a second disk as to a tape. A machine with that
diskette and a disk with nothing on it is then started: it must come
up on the miniroot, and there label the disk, make the file system,
restore the dump, check it and write the boot block. The disk must
have nothing wrong with it and hold what the system's root has, as
`tests/disk` compares one, and the machine must start from it alone. A
dump on more than one diskette has not been run.

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
- `realloc` of no memory is `malloc`. The C library's read the word
  before the address it was given, which before address 0 is the top
  of a PDP-11 program's stack and outside a 286 program's data: `mkstr`
  ended there on the first file with a line across two of `fgetln`'s
  buffers, which was `ex.c`.
- `/usr/lib/libtermlib.a`, the other name of `libtermcap.a`, a link as
  upstream installs it: `battlestar`'s Makefile and others ask for
  `-ltermlib`.
- `tcsh`'s `sh.err.c` has the 286's flags under `#ifdef i286`, which
  the `unifdef` its Makefile runs can read; it does not read
  `#if defined`.

## What it does not make yet

- The Makefiles of the kernel, of the programs that load it, of the
  libraries and of what is not a program, as above: they are written
  from the repository's, and a change made to one on the system is lost
  when they are written again. Upstream has one for each of those
  too.
- A disk's first sector when it is shared with another system
  (`mbr.bin`, which is made): there is no device for it.
