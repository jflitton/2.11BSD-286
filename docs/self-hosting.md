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
python3 tools/srcmk.py obj/src          # obj/src/proto
python3 tools/mkfs.py -s 64000 obj/src/proto obj/src.fs
```

`obj/src.fs` is a file system to mount on `/usr/src`: a partition of
the system's disk (`tools/mkdisk.py ... d=obj/src.fs`,
[disks](disks.md)). It has the sources in the directories they have in
the repository, 20 MB of them, and room for what is made of them, which
is left beside them. The root file system has the directory, and `/sys`
is a name for `/usr/src/sys`.

The compiler, the libraries, the programs, what is not a program and the
manual are made by upstream's Makefiles: `usr/src/Makefile`'s
`make build`, and the Makefiles of the directories it names,
`usr/src/lib/Makefile`, `usr/src/lib/libc/Makefile`,
`usr/src/usr.lib/Makefile`, `usr/src/bin/Makefile`,
`usr/src/bin/sh/Makefile`, `usr/src/share/Makefile`,
`usr/src/man/Makefile` and the rest, as 2.11BSD has them, changed
where the 286 is not the PDP-11 ([below](#upstreams-makefiles)). They are in the repository and on the
system the same, and a change made to one there is a change to the
system's sources.

So are the kernel and the programs that load it. The kernel's is the
one `config` writes in `sys/PC` from the machine's file, `sys/conf/PC`,
as 2.11BSD has a kernel's written in a directory of `/sys`
([configuring the kernel](development.md#configuring-the-kernel)), and
`/boot`'s and the boot blocks' is `sys/i286stand`'s, after `pdpstand`'s.
Beside each is a `GNUmakefile`, which GNU make reads first, for the
other build.

## Making it

On the system, as root:

```sh
mount /dev/wd0d /usr/src

cd /usr/src; make build; make installsrc         # all but the kernel

cd /usr/src/sys/i286stand; make; make install    # /boot, the boot blocks
cd /usr/src/sys/conf; ./config PC                # the kernel's directory
cd /usr/src/sys/PC; make; make install           # the kernel
```

`make build` is upstream's. It makes the C library and installs it;
makes the compiler with that library and installs it, `cpp`, `c0`,
`c1`, `c2` and `c2l`, keeping each that was as `/lib/oc0` and so on;
makes the library and the compiler again, with what it installed, and
installs them; and makes the other libraries and installs them. Then it
makes `share`, `bin`, `sbin`, `etc`, `games`, `libexec`, `local`,
`new`, `ucb`, `usr.bin`, `usr.sbin` and `man`, in that order, with
those, and `make installsrc` installs them. The kernel and the programs
that load it are made apart, as upstream's are. Each directory is
made by itself the same way, `make; make install` in
`/usr/src/lib/libc`, `/usr/src/bin` or `/usr/src/man`:

| in | `make` makes | `make install` |
|---|---|---|
| `sys/i286stand` | `boot`, `hdboot.bin`, `flboot.bin`, `mbr.bin`, `fdboot.bin` | `/boot`; `/mdec/wduboot`, and `sduboot` another name of it; `/mdec/fluboot` |
| `sys/conf`, `./config PC` | `sys/PC`: its Makefiles, the devices' counts, `param.c`; and `sys/h/localopts.h` | |
| `sys/PC` | `unix`, and `unix.map` | `/unix`, root's and `kmem`'s, 744; the one that was is `/unix.old` |
| `lib` | the compiler and the C library, as `lib/ccom`, `lib/c2`, `lib/cpp` and `lib/libc` make them | |
| `lib/ccom`, `lib/c2`, `lib/cpp` | `c0` and `c1`; `c2` and `c2l`; `cpp`, and `cpp.0` | `/lib`, the one that was as `oc0`, `oc1`, `oc2` and `oc2l`; `/usr/man/cat1/cpp.0` |
| `lib/libc` | `libc.a`, `libc_m.a`, `libc_l.a`, and `crt0.o` of each model in `i286/csu` | `/lib`, `/lib/m`, `/lib/l` |
| `usr.lib` | each library, of each model, beside its sources | `/usr/lib`, `/usr/lib/m`, `/usr/lib/l` |
| `share` | `termcap`; `zic` and `zdump` ([what is not a program](#what-is-not-a-program)) | `/usr/share`: `misc`, `tmac`, `me`, `ms`, `tabset`, `learn`, `zoneinfo`; `/etc/termcap`, a symbolic link; `zic` and `zdump` in `/usr/sbin` |
| `bin`, `sbin`, `libexec`, `ucb`, `usr.bin`, `usr.sbin`, `games`, `local`, `new` | each program, beside its sources, and its page of the manual | each where upstream's Makefile installs it, with the mode, owner and group it gives |
| `man` | the pages of `man1` to `man8` | `/usr/man/cat1` to `cat8`, with their other names; `/usr/man/whatis` |

A kernel that does not start is left behind by naming the other to
`/boot`: `wd(0,0)unix.old` ([booting](boot.md)). `make` in a program's
own directory, `/usr/src/bin/sed`, makes that one. `make clean` in any
of them removes what was made.

`config` is for when `sys/conf/PC` has changed: `sys/PC` is what it
writes from that file already. The kernel's objects are made again
when a header changes once `make depend` has said which headers each
has, as upstream's are. The C library's stubs for the system calls are written as it
is made, a file of assembly for each call and each model, from
`lib/libc/i286/sys/syscalls.tab`: by `mkstubs` (`sys/mkstubs.c` beside
it), which both builds compile and run, and which has each agree with
the kernel's table of calls.

## Upstream's Makefiles

A program's Makefile is the one 2.11BSD has for it, and makes it as
that says: `yacc` and `lex` their parsers and scanners; `awk`'s table
by a program of `awk`'s own; `csh`, `make`, `fsck`, `yacc` and `Mail`
with their strings taken out by `xstr`; `ex` and `tcsh` with their
messages taken out first, into `/usr/share/misc/exstrings` by `mkstr`
and into `/etc/tcsh6.00strgs` by `tcsh`'s own `mkprstr`; the page of
the manual with the program. The other build runs the same Makefiles,
with `xstr`, `mkstr` and `mkprstr` compiled for the machine it runs on
(`tools/build/commands.sh`), and gives what they install in the root
the place, mode, owner, group and other names upstream installs with:
a file is root's, and of its directory's group, unless its Makefile
says another. What was changed in them for the 286:

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
- `lib/ccom` makes the 286's second pass, from `i286`, where the
  PDP-11's was made with its tables (`optable`, through `cvopt`);
  `lib/c2` the 286's `c2`, in `i286`, of the small model, and of the
  large as `c2l`, its objects in `l`; `lib/cpp` a `cpp` that keeps what
  it has in memory and in segments, and not in a file through
  `libvmf`. `lib`'s list has not `libkern`, the PDP-11 kernel's, and
  `make build` makes neither that nor `libvmf`.
- A directory's list of programs has not the ones the port does not
  have: the PDP-11's debugger and tape formats, Fortran and Pascal,
  `uucp`. `usr/src/local`'s list has the port's own
  programs, `adduser`, `bench` and `ve`, in place of upstream's, which
  all compile and are not made. Nor has `share`'s list `font` and
  `fontinfo`, the typesetter's and the Versatec's, which the port has no `troff` nor
  `vtroff` for; and `man`'s lists have the pages of what the port has,
  and no `man3f`, Fortran's library.
- `as(1)`, `ld(1)` and `a.out(5)` are the 286's, in `bin/as/i286` and
  `bin/ld/i286`, and `bin/as`'s and `bin/ld`'s Makefiles install them,
  `a.out(5)` with `ld` as `ar(5)` is with `ar`; `man1`'s and `man5`'s
  have not upstream's.
- `-i`, `-n` and `-z`, the PDP-11's separate instruction and data
  spaces, are left where they are: the system's `ld` takes them and
  does nothing.

A library is made for each of the 286's three models, where upstream's
was made a second time for profiling: a file is compiled three times,
the medium model's object put in `m` and the large's in `l`, where the
profiled one went in `profiled`, and each model has a library, which
is installed where `cc` links a program of that model with it (`/lib`,
`/lib/m`, `/lib/l`; `/usr/lib` and the same). Besides:

- The PDP-11's `ld -x -r` of each object, which took out its local
  symbols, is not done: the 286's `ld` makes nothing but a program.
  `libdbm` and `liberrlst`, which upstream makes with `ld -r`, are each a
  library of `ar`'s with `ranlib`'s table, which the 286's `ld` looks
  in.
- The C library's machine dependent directory is `i286`, whose
  Makefiles are after `pdp`'s: `crt`, `csu`, `gen`, `net` and `sys`,
  this last with the stubs. `gen` has the C that the PDP-11 has in
  assembly, as its Makefile says to where the machine is not that one;
  `compat-sys5` has `strchr` and `strrchr`, which on the PDP-11 are
  other names in `index`'s assembly; the large model's `malloc` is
  `i286/gen/lmalloc.c`, which gets segments from the system where
  `malloc.c` moves the break.
- No profiling, and so no `mon.o` and `mcrt0.o`. No `fakcu.o`, an empty
  `_cleanup` for a program without `stdio`, which works by the order of
  the library: the 286's `ld` once took it in place of `stdio`'s, and
  the program did not flush at exit. No `fpfcom.o`, `printf` without
  floating point, which is taken unless the compiler marks a program
  that has it, and the 286's does not.
- `usr.lib` makes the eight libraries the programs are linked with.
  `libwrap` is `tcpd`'s, which its Makefile makes and does not install.

The other build makes the C library with the same Makefile, `lorder
*.o | tsort` and two `ar ma`, with `tools/lorder.py`, which prints what
the system's `lorder` does, and the tree's `tsort` compiled for the
machine it runs on.

And one is mended: `hack`'s made the game's directory anew and left it
empty, and the game does not start without what its `READ_ME` says it
reads and writes there. Its install puts those in: `data`, `help`,
`hh` and `rumors`, an empty `record` and `perm`, and `save`.

`/lib/cpp` says `unix` and `BSD2_11`, as upstream's does for the
PDP-11: the Makefiles do not, and the sources ask.

## What is not a program

`share`'s Makefiles install what the system has that is neither a
program nor a page of the manual, and make some of it: `termcap`, of
`termcap.src` by `ex` and `reorder`, with `/etc/termcap` a name for it;
`mdoc`'s macros and `-me`'s without their comments, by `sed`; `learn`'s
lessons, copied by `tar`, and the two objects its lessons in C link
with, by `cc`; the time zones, by the `zic` it makes, with `zdump`.
`man`'s format each page of `man1` to `man8` with `manroff`, `nroff -man
-h`, and `eqn(1)` through `eqn` first, install them with their other
names, and make `/usr/man/whatis` of every page in `/usr/man` with
`makewhatis.sed` and `sort -u`. It goes last, as upstream's Makefile
in `/usr/src` has it, after the programs, whose Makefiles install their
own pages.

The programs' Makefiles make the rest: `/etc/syserrlst`, which
`mkerrlst`'s install writes; spell's lists; the tables of
`/usr/share/term`; the games' files; the files of `ex`'s and `tcsh`'s
messages; `refer`'s index of `/usr/dict/papers`, which its install has
its `runinv` make with `mkey` and `inv`. The other build makes some of
those with programs of the machine it runs on or of Python that do the
same, to the byte: `tools/runinv.py`, `tools/mkerrlst.py`,
`tools/mktermcap.py`, the system's `zic` and `eqn` compiled for it.
`eqn` keeps a pointer in yacc's values, which there are a long
(`usr.bin/eqn/e.h`).

The password file's database, the aliases' and `fstab` are a new
system's, and a system that is in use has its own users and disks:
`etc`'s Makefile makes the first for a new root (`make distribution`).

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
flformat /dev/rfl0
newfs /dev/rfl0
mount /dev/fl0 /mnt
cp /boot /unix /mnt
umount /mnt
dd if=/mdec/fluboot of=/dev/rfl0 count=1
```

In a 5.25 inch drive the same makes a diskette of 1.2 MB: the driver
finds what the diskette is, and tells `newfs`. A diskette of 720K,
formatted with `flformat -f 720 /dev/rfl0`, is for files: `newfs` and
`mount`, or `tar cf /dev/rfl0`.

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
flformat /dev/rfl0
sh /usr/src/sys/i286stand/buildmini fl0
dump 0Bf 1440 /dev/rfl0 /dev/rwd0a
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
restor rf /dev/rfl0 /dev/rwd0a
fsck /dev/rwd0a
disklabel -B wd0
reboot
```

- The miniroot's devices are made by `/dev/MAKEDEV`, which is
  upstream's with the 286's devices in place of the PDP-11's
  (`usr/src/etc/MAKEDEV`), which makes the root's too, as `etc`'s
  `distribution` runs it (`MAKEDEV all`).
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
| `share` and the manual, and installing them | 2 hours 40 minutes |
| all of it | 12 hours |
| a disk of it, with the sources | 20 minutes, on the emulator's disks, which answer at once |

The compiler does some 20 lines a second ([performance](performance.md)).
Of the programs, `tcsh`, `ex`, `hack`, `sendmail` and `window` are half
an hour each or more.

Those are of the directories made one at a time. `make build` makes the
C library twice, and the compiler twice besides; with the faster
processor ([below](#what-is-checked)) it and `make installsrc` took 4
hours 37 minutes, 42 minutes of the host's.

## What is checked

`tests/self/run.sh` boots the system with that file system, has it make
`/boot`, run `config` and make the kernel, and compares each object,
the kernel and `/boot`, and what `config` writes, with what the same
sources make where the system is built. Last the kernel is made to say
that it was made there, installed with `/boot`, and the machine started
again: it must come up on that kernel, with `init`, `getty`, `login` and
the shell that it made. `PARTS=stand` or `PARTS=kernel` makes one of
the two.

`tests/self/progs.sh` has the system make the compiler, the libraries,
the programs, what is not a program and the manual with `make build`
and `make installsrc` in `/usr/src`, or `make` and `make install` in
each of the directories it is given (`sh tests/self/progs.sh lib/libc
bin/sed usr.bin/yacc man`). Nor is `make` to say that a command
failed, which a Makefile that runs `make` in its directories goes on
from. The root is then to be as it was before, which is what the
same Makefiles made where the system is built, name for name: each file
with its contents, mode, owner, group and other names, each directory
with its mode, owner and group, each device and symbolic link. A
library has the day it was made in it, and whose its members were, and
is compared member for member, each by its name and contents and in
their order, the table of contents among them. Nothing is to be
gone and nothing new, but what a system that is running writes
(`tests/self/running`), the `NAME.old` that `tcsh`'s and `init`'s
Makefiles keep of what they install over, and the compiler's `oc0`,
`oc1`, `oc2` and `oc2l`. Each of the root's programs
whose sources are in those directories is to have been installed, each
library of `/lib` and `/usr/lib`, and each page of the manual whose
source is in one of them. A page has at its foot the day it was
formatted, and is compared without it; the files of
`tests/self/differs`, below, are not compared.

All of it is half a day of the 286's time, which is for a release and
not for a change. A change is checked two ways, and
`sh tests/self/changed.sh` does both:

- Booting nothing. The root the tests boot is made by the same
  Makefiles where the system is built (`make -C usr/src root`,
  `tools/build/build.sh`): `make build` and `make installsrc` in
  `usr/src`, after `etc`'s `distribution`, which makes the root's
  directories, `/etc` and `/dev`, run by that machine's `make` with the
  rules the system's has and none of its own, and with the system's
  compiler, assembler and linker as they are compiled for it. What
  `make install` would put in the system's root is put in one of the
  build's own, with what it would have of mode, owner and group noted
  beside it, of which `tools/mkfs.py` makes the root file system. That
  machine's `make` takes the root's directories to be there, as the
  system's does; so is what a system has already that the Makefiles
  read and none installs, `/usr/dict`, and the scripts `man`'s
  Makefile installs and others run, `manroff` and `makewhatis`, which
  are installed first. `sh tests/make/run.sh` makes it and compares
  it with `tests/make/list`, each file, directory, device and link with
  its mode, owner and group: 2681 names, in eight minutes the first
  time and two after a change. `HOST=1 sh tests/self/run.sh` makes `/boot`
  and the kernel with their Makefiles the same way and
  compares what they make, each object (`tests/self/host.sh`). That
  says the Makefiles name the right files, flags and order, and install
  each where the system has it. It does not say the system's `make`
  reads them so, nor that the tools do on the 286 what they do there.
  A program a Makefile makes and then runs is the 286's, and is run
  there as that machine has it: compiled for it of the same C, as
  `awk`'s `proc`, `tcsh`'s `mkprstr`, `hack`'s `crypt` and `makedefs`,
  `warp`'s `sm`, `boggle`'s `comp` and `nroff`'s tables; or, where it
  writes what the 286 lays out, by a script of the other build,
  `strfile`, `initdeck`, `setfiles`, `mkerrlst` and `refer`'s
  `runinv`; `ex` for `termcap` and `awk`'s `tokenscript`, `zic`,
  `spellin`, `tbl` and `eqn` are the tree's for that machine. A line
  that does no more than run one with nothing to stand for it, as
  `ntp`'s `ntest`, is passed over; and `adventure`'s game, which only a
  286 can write, is not made: its Makefile installs instead what makes
  it there when it is first played, the program, its text and a
  Makefile of the port's (`tools/build/adventure.mk`). That
  machine's `make` takes the `./` from a name, and then looks for it
  in `VPATH`, where the system's does not: such a name is given it
  whole (`ranlib`'s `./misc.o`, which is not `ar`'s); and its `cc`
  links an object named twice once, as the system's does (`hack`'s).
  `tests/self/differs` lists what the system does not make the same
  way, which `tests/self/progs.sh` does not compare: `backgammon`, whose
  Makefile writes the day into it; `adventure`'s game; and two files
  that are made with the time, `fortune`'s table and `phantasia`'s
  void. Before the root was made so, by the port's own Makefile, the
  system's `sendmail`, `hack`, `backgammon`, `phantasia` and
  `adventure` were run instead: `tests/programs` and `tests/network`
  take a root as `ROOTFS`, and passed on one the system installed them
  in, but for `last`, which found there the logins of the machine that
  made them.
- One machine is then booted to make what the changed files are part
  of and nothing else (`tests/self/select.py`): with `make` in the
  directory whose Makefile makes a file, and in those of the programs
  whose sources it is among, and the kernel or the boot programs. A
  change to the tools that do the making, or to a header of
  `/usr/include` or `sys/h`, has the system make its compiler
  (`tests/ccom/self.sh`), the kernel and a program of each model,
  `sed`, `csh` and `make`. A change to the other build, or to what
  writes `/usr/src` for the system, has it make the five programs
  above that the other test does not. `changed.sh -n` says which, and
  why, and runs nothing.

Both boot the machine with a faster processor unless `CPU=286` is in
the environment: IBM's 486SLC3 at 100 MHz, an upgrade for this same
machine, in place of its 286 (`tests/harness/86box-486.cfg`). What the
system makes there is the same to the byte, in less than half the time:
five of the largest programs in 304 seconds where the 286 takes 710.
It says nothing of whether a 286 runs the system, which is for
`run.sh` and the other suites, on the 286.

`sh tests/self/split.sh` is all of it, a directory at a time, in two
machines or three at once, each with a fixed share: three is the most,
since each keeps a processor of the host busy for as long as it runs.
With the faster processor that is a quarter of an hour.

`tests/disk/run.sh` has the system make a disk that way, of a second
drive of 133 MB with nothing on it, not a label. `tools/fsck.py` must
find nothing wrong with the new disk's two file systems, and lists
each (`-l`): the root must have what the system's own has, name for
name, each file's contents, mode and owner, each other name, link and
device, but for what a system that is running writes; and the sources
what `/usr/src` has. Then the machine is started from that disk alone,
and must come up and make `/boot` and the boot block from its sources,
the same as the ones it started from. Last it is started from the first
disk with both, and the second started from `/boot`'s prompt with
`wd(1)` ([another system's disk](boot.md#another-systems-disk)): only
the second has the kernel then loaded, which comes up on the first
disk's root.

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
  arguments at most, and the kernel 5120 characters of them: the C
  library's Makefile gives `ar` its 359 objects through the shell, in
  some 3500.
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
  write zeros now. `strfile -r` seeds its generator with the time, so
  that `fortunes.dat` is not compared (`tests/self/differs`).
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

- The Makefiles of the kernel, of the programs that load it and of what
  is not a program, as above: they are written
  from the repository's, and a change made to one on the system is lost
  when they are written again. Upstream has one for each of those
  too.
- A disk's first sector when it is shared with another system
  (`mbr.bin`, which is made): there is no device for it.
