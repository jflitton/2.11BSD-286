# Building and testing

The toolchain and why it was chosen, how the system is built, the
reference material, the tools and the tests.

## Layout

| path | what | in git |
|------|------|--------|
| `usr/src/` | the 2.11BSD source tree, at its real path | yes |
| `usr/src/sys/` | the kernel; `sys/`, `h/`, `net*/` are machine-independent, `pdp*/` and `mdec/` are the PDP-11 | yes |
| `usr/include/` | the system's headers; mostly links into `usr/src/sys` | yes |
| `sys` | link to `usr/src/sys`, as on the running system; the links in `usr/include` go through it | yes |
| `tools/` | host-side tools (below) | yes |
| `tests/` | tests that boot in an emulator (below) | yes |
| `docs/` | our notes | yes |
| `LICENSE`, `COPYRIGHT` | the port's license, and whose each part of the tree is | yes |
| `reference/MANIFEST` | what third-party material we use, where it comes from, checksums | yes |
| `reference/upstream/` | the install tape, patches, setup guide | no |
| `reference/manuals/` | Intel, IBM, Adaptec and DEC manuals; Ralf Brown's Interrupt List | no |
| `reference/prior-art/` | MINIX, RetroBSD, DiscoBSD, ELKS, Coherent sources | no |
| `reference/toolchain/` | the cross-compiler's distribution archive | no |
| `toolchain/` | the installed cross-compiler | no |
| `obj/`, `images/`, `vm/` | build output, disk images, emulator state | no |

Where the 286 code goes, beside its PDP-11 counterpart in each case:

| new | beside | holds |
|-----|--------|-------|
| `usr/src/sys/i286/` | `pdp/` | startup, traps, system calls, copy routines, segments, switching between processes, signals, the 287, the clock, the console on the display, a RAM disk; the calls between the kernel and the network, and the network's side of them ([the network](network.md)) |
| `usr/src/sys/i286at/` | `pdpuba/` | AT-bus drivers: the Adaptec SCSI adapter and its disks, the AT disk controller (IDE), and the PC partition table; the serial ports, the keyboard, and the console's VT102 terminal emulator ([the console](console.md)) |
| `usr/src/sys/i286if/` | `pdpif/` | network interfaces: the NE1000 and NE2000 and the cards that copy them ([the network](network.md)) |
| `usr/src/sys/i286stand/` | `pdpstand/`, `mdec/` | what loads the kernel: the diskette's boot sector, and for a hard disk its first sector, the boot block and `/boot`; `make -C usr/src/sys/i286stand` builds them in `obj/stand` |
| `usr/src/sys/PC/` | `GENERIC/` | the kernel's configuration and Makefile: `make -C usr/src/sys/PC` builds `obj/PC/unix` |
| `usr/src/lib/libc/i286/` | `lib/libc/pdp/` | libc's machine-dependent part, and the Makefile that builds the library from it and the tree's portable C: `make -C usr/src/lib/libc/i286` makes `obj/lib/s/libc.lib` |
| `usr/src/i286/` | the tree's own Makefiles | the list of programs built and the Makefile that builds them in `obj/bin`; the root file system's contents and `/etc` files, which `make -C usr/src/i286 root` makes into `obj/root/root.fs` ([the C library and programs](userland.md)) |

The kernel picks its machine through the link `usr/src/sys/machine`, which
points at `pdp` upstream and at `i286` here, as does `usr/include/machine`.

Assembly is NASM syntax, in files ending `.s` as elsewhere in the tree.

## Toolchain

The system is cross-compiled with Open Watcom v2, which runs on Linux (x64,
arm64) and macOS (x64, arm64).

```sh
tools/fetch-reference.py toolchain   # the archive, about 150 MB
tools/install-watcom.sh              # unpack this host's build into toolchain/
. tools/env.sh                       # wcc, wasm, wlink, wlib, wdis on PATH
```

The build also uses the C compiler of the machine it runs on, `cc`, for
the tree's own `yacc` and `lex`, which make the parsers and scanners of
the system's programs, for the tree's `nroff`, which formats the manual
([the C library and programs](userland.md)), and for `tests/vt`.

Why Open Watcom, from compiling test code and 448 unmodified files of this
tree with each candidate (November 2026):

- It has the large and huge memory models, where pointers are far by
  default, so an existing program gets several data segments without source
  changes. gcc-ia16 stops at the medium model: one data segment, and
  anything outside it needs `__far` on every pointer.
- It generates 287 instructions in line. gcc-ia16 calls a software routine
  for every floating-point operation.
- Its far-pointer code loads segment registers only with selectors it was
  given and does no segment arithmetic, which protected mode requires.
- Based pointers (`__based`) let near-data code reach another segment with
  a 16-bit offset. That is the kernel's situation when it touches a buffer
  or a user's memory.
- Its linker writes segmented executables with a segment table and
  per-segment relocations. ELKS loads that format for the same compiler.
- Compiled code needs only a handful of helper routines (32-bit multiply
  and divide, float-to-integer rounding, huge-pointer arithmetic), small
  enough to write here instead of taking Open Watcom's library.

What it costs:

- It cannot run on the 286, so the system does not compile itself. None of
  the compilers small enough to run there (pcc, ACK, C86, the Fuzix
  compiler, bcc) has far data today. A native compiler is a later project.
- Objects are OMF and the assembler takes Intel syntax; nothing of the
  tree's own `as`, `ld`, `ar`, `nm` or a.out format carries over unchanged.
- It is stricter than the tree's compiler. About one file in seven fails
  as distributed, mostly old-style definitions that disagree with a
  prototype and functions that mix `return;` with `return x;`. gcc-ia16
  rejects about as many.

Flags that worked in those trials: `-2` (286 instructions), `-fpi87 -fp2`
(in-line 287), `-ecc` (arguments on the stack, caller removes them, which
old-style calls with mismatched argument counts depend on), `-s` (no stack
probes), `-zl -zls` (no references to Open Watcom's libraries), `-ms`,
`-mm`, `-ml`, `-mh` for the memory model, and `-Drestrict=` because the
headers use `restrict` while other files use `inline` as an identifier.
One the trials did not show the need for: `-j`, which makes a plain `char`
signed. Open Watcom's is unsigned unless told, the tree's code keeps
negative numbers in them, and nothing fails to compile either way.

## Building

```sh
. tools/env.sh
make -C usr/src/sys/PC              # the kernel, obj/PC/unix
make -C usr/src/sys/i286stand       # the boot sectors and /boot, obj/stand
make -C usr/src/lib/libc/i286       # the C library, obj/lib
make -C usr/src/i286 all root       # the programs, obj/bin, and the root
                                    # file system, obj/root/root.fs
python3 tools/mkdisk.py -p -g 977,5,17 -b obj/stand/hdboot.bin \
    -m obj/stand/mbr.bin disk.img a=obj/root/root.fs b=swap:4000
```

The last makes a bootable image of a 40 MB IDE disk with the root and
2 MB of swap. The root file system is 24 MB, with 15 MB in it;
`make root ROOTSIZE=` makes it larger, and `ROOTDISK=sd0` makes it for a
SCSI disk, whose image `mkdisk.py -t scsi` makes. [Booting](boot.md) and
[disks](disks.md) say more, and [the root file
system](userland.md#the-root-file-system) what is in it.

## Branches

- `upstream` is 2.11BSD exactly as distributed, one commit per patch level.
  Nothing of ours goes on it.
- `main` branches from it. `git diff upstream main -- usr sys` is the port.

How `upstream` was made and how to take a new patch: [upstream](upstream.md).

## Reference material

```sh
tools/fetch-reference.py            # fetch whatever is missing
tools/fetch-reference.py --list     # what there is
tools/fetch-reference.py manuals    # only entries whose path contains "manuals"
```

`reference/MANIFEST` says what each item is for. To add one, add a line with
`-` as its checksum and run `tools/fetch-reference.py --pin`.

2.11BSD's own papers are the documentation for the code we keep. They are
on the tape rather than in git; after a fetch,
`tar xf reference/upstream/PL500/tape/file6 -C reference/upstream/PL500/usr doc`
unpacks them as troff source. The ones that matter here:

| under `reference/upstream/PL500/usr/doc/` | what |
|------|------|
| `2.10/kchanges.2.11`, `kchanges.2.10`, `setup.2.11` | what the 2.11BSD kernel changed, and how the system is set up |
| `2.10/ovpap`, `ovtutorial` | how overlays work, in the kernel and in user programs |
| `2.10/assembler` | the PDP-11 assembler `sys/pdp/*.s` is written in |
| `smm/02.config` | kernel configuration |
| `smm/14.fastfs`, `smm/15.net` | the file system and the networking implementation |
| `smm/19.porttour` | a tour of the portable C compiler |

The driver also knows the AHA-1542CP, the Plug and Play member of the
1540C series. Its guide says it also runs in machines without Plug and
Play, at the settings on its switches and in SCSISelect (defaults: port
330h, IRQ 11, DMA 5), so the 1540C technical reference describes what the
driver talks to.

## Tools

| tool | does |
|------|------|
| `tools/fetch-reference.py` | fetches `reference/` from the manifest |
| `tools/install-watcom.sh` | installs the cross-compiler for this host into `toolchain/` |
| `tools/env.sh` | sourced, puts the toolchain on `PATH` |
| `tools/cc286`, `tools/cc286.h` | compile one C file with the system's memory model and calling convention |
| `tools/cc` | compiles and links programs against the C library, as `cc` would; with `-ccom`, by the system's own compiler, assembler and linker ([toolchain](toolchain.md)) |
| `tools/ncc` | compiles one file with the system's own compiler and assembler, as built for this machine |
| `tools/ar.py` | makes a library of the system's objects with its table of contents, as `ar` and `ranlib` do |
| `tools/ccproto.py` | lists `/usr/include` and the libraries for the root file system |
| `tools/commonize.py` | gives an object's uninitialized variables common storage, as the tree's C expects |
| `tools/mkerrlst.py` | makes `/etc/syserrlst`, the system's error messages |
| `tools/strfile.py` | makes `fortune`'s table of fortunes, as `strfile -r` does |
| `tools/initdeck.py` | packs `monop`'s cards, as `initdeck` does |
| `tools/setfiles.py` | makes `phantasia`'s files of characters and of the grail, as its `setfiles` does when it is installed |
| `tools/mkpasswd.py` | makes `/etc/passwd` and the password database from `master.passwd`, as `mkpasswd -p` does |
| `tools/mkman.py` | formats the manual's pages for `/usr/man` with the host's build of `nroff`, with upstream's links among them and the `whatis` database |
| `tools/ne2aout.py` | turns the linker's output into the system's executable format; `--dump` prints one |
| `tools/mkfd.py` | makes a bootable diskette image from an executable, with a file system image for the kernel's RAM disk if wanted |
| `tools/mkfs.py` | makes a file system image from a list of files, with their modes, owners and groups |
| `tools/mkdisk.py` | makes a hard disk image: a partition table or none, a label, partitions, and the sectors that boot it |
| `tools/fsck.py` | checks a file system in an image without changing it, or prints a file from it |
| `tools/mkstubs.py` | writes system call stubs from a table of the calls' arguments, checked against the kernel's |
| `tools/kpatch.py` | changes a variable in an executable file, by name |
| `tools/kprof.py` | says where the system spent its time: the functions, in the kernel and in programs, that the samples `runtest.py --profile` took of the emulated processor fell in |
| `tools/strip.py` | copies an executable file without its symbol table, as `strip` leaves one: the programs in the root file system are such copies, unless it is made with `STRIP=no` |
| `tools/farlint.py` | looks for far pointers that kernel code truncates without the compiler noticing |
| `tools/lmlint.py` | looks a program over before it is made large-model: a `0` for a pointer or a pointer for an `int` where no prototype says which, a function that returns a pointer called undeclared, and arguments that differ in number from the definition |
| `tools/preflight.py` | looks a program over before it is built: what the compiler rejects, calls that disagree with a definition in another file or in the library, lines that match what has gone wrong before, and whether it links and how large it is |
| `tools/netcheck.py` | looks in the kernel's objects for calls and references between it and the network that do not switch the data segment; run at every link |
| `tools/untap.py` | splits a SimH `.tap` tape image into its files |
| `tools/tar2git.py` | commits tar archives to a branch without unpacking them |
| `tools/import-upstream.sh` | rebuilds the `upstream` import from the tape, to check it |

## Tests

Tests run real 286 code in an emulator. `tests/harness/runtest.py` puts a
program on a diskette image behind the boot sector
(`usr/src/sys/i286stand/fdboot.s`), boots it, prints what it wrote to the
serial port and takes the `@exit` line it ends with as the result. It can
also give the machine a hard disk, and a second one on the same
controller, boot from the first with no diskette, type
at the serial port, type on the keyboard once the screen shows a
prompt, let the system start the machine again, and give the machine an
NE1000 or NE2000 on a network of the emulator's own, with the host able
to telnet to it there, as a terminal of a type and window size given.

```sh
. tools/env.sh
tests/crt/run.sh        # the compiler's helper routines: 5,161 checks
tests/pm/run.sh         # the kernel's start-up and trap entry: 54 checks
tests/user/run.sh       # a process in ring 3: 58 checks
tests/kernel/run.sh     # the kernel, with a test program as init, booted
                        # six times: 82 checks, then 98 on each of five disks
tests/libc/run.sh       # the C library, with a test program as init: 74
                        # checks in the small model, 74 in the medium
                        # and 84 in the large; with CCOM=1, of the
                        # library and the program as the system's own
                        # compiler makes them, which has not the large
tests/ccom/run.sh       # the system's own C compiler, run here: 1573
                        # checks of the code it generates, as it is and
                        # again as c2 improves it; 58 by a program it
                        # compiled, in each of two models, that the
                        # system's assembler and linker made; and the
                        # compiler's two passes and c2 as it compiled
                        # them, run on the system
tests/ccom/self.sh      # the system making its compiler: cc, cpp, the
                        # passes, as, ld and the rest, eleven programs
                        # from their sources on the 286, twice, the
                        # second time with the ones it had just made,
                        # each to be what is made of it here
tests/ccom/programs.sh  # the same compiler, assembler and linker on
                        # the system's programs: tests/programs, with
                        # all of them made that way but window
tests/as/run.sh         # the system's assembler against NASM: 289
                        # files that both are to make the same of (no
                        # emulator)
tests/preflight/run.sh  # tools/preflight.py, on two files with one of each
                        # thing it looks for (no emulator)
tests/bench/run.sh      # how fast the system is: its benchmark, each
                        # of 18 tests timed by the system's clock and by
                        # the emulator's, and a profile of each; fails
                        # only if the measuring does
tests/network/run.sh    # the network's programs, on the system booted to
                        # multi-user with an emulated NE2000: 34 checks,
                        # of ftp, rsh, rcp, tftp, finger, tcpd, nc, the
                        # daemons, hunt and mail, and rlogin, biff and
                        # talk at terminals
tests/programs/run.sh   # the programs that work on text and files, on the
                        # system booted to multi-user, nroff and man,
                        # tbl, eqn, refer, spell and their kind, zic
                        # and /usr/adm's scripts, disklabel, fsck and
                        # the other checkers, dump and restor on a second
                        # disk, cc and what goes with it compiling a
                        # program there, yacc and lex with their libraries, mkdep,
                        # calendar and learn, the games, segalloc and
                        # ve: 164 checks, each a few
                        # lines for the shell and what they must write,
                        # and more, man, script, lock, tset, five games,
                        # window, ve and shutdown at a terminal
tests/system/run.sh     # the system, booted from a hard disk to multi-user:
                        # checks its root with fsck and starts again,
                        # logs in on a serial port and on two of the
                        # console's screens, uses sh, csh and tcsh,
                        # edits with ex and vi, adds a user with a
                        # password, makes a file system, reads the log
                        # and the accounting, logs in again over the
                        # loopback interface with telnet as that user,
                        # who gives cron and at something to run, and
                        # from the host over an emulated NE2000, as an
                        # xterm in a big window
tests/vt/run.sh         # the console's terminal emulator, on the machine
                        # the system is built on: 302 checks
tests/ve/run.sh         # the editor ve, built for that machine too: 115
                        # checks, of what it makes of a file and of
                        # the screen it draws (no emulator)
```

- `tests/crt` is a small-model program in real mode, built through the
  normal path (`cc286`, `wlink`, `ne2aout.py`) behind the start-up code in
  `tests/harness/start.s`.
- `tests/ccom` is another, whose tests are compiled by the tree's own
  compiler, `usr/src/lib/ccom`, with the second pass for the 286,
  `usr/src/lib/ccom/i286`. Both passes are built for the machine the
  system is built on and run there by `tools/ncc`; for these tests NASM
  assembles what they write and the cross tools link it, since the
  tests call the cross compiler's code to have each value checked: the
  two compilers' calling conventions are tested against each other.
  `REF=1` compiles the tests with the cross compiler instead, to check
  a test. The tests are run twice: as the two passes leave them, and
  with `tools/ncc -O`, which has `c2` improve the code
  (`usr/src/lib/c2/i286`); everything else the compiler makes is made
  with `-O`. It ends by booting the kernel with programs that are the
  system's own tools' work throughout, compiled, assembled and linked
  by them (`tools/cc -ccom`) with the C library as they made it: an
  `/etc/init` that runs one program built for the small model and again
  for the medium, and then the compiler as it compiled itself, the two
  passes on a C file and `c2` on a file of assembly, each of which is
  to make there exactly what it made here.
  The library is in `obj/nlib`, where `make -C usr/src/lib/libc/i286
  CCOM=1` puts it and `tools/cc -ccom` looks for it.
  `tests/ccom/programs.sh` has those tools make the system's programs,
  and the libraries they use, and runs `tests/programs` on a root file
  system of those; `obj/nbin/native` lists which they were, and
  `obj/nbin/cross` any that only the cross compiler could make:
  `window`, which is of the large model.
- `tests/as` has the system's assembler and NASM assemble the same
  files: one with every instruction, the C library's assembly, `ve`'s
  and the system call stubs for both models, and what the compiler
  writes for
  the programs of `tests/ccom`. `tests/as/objcmp.py` reads NASM's
  object and the assembler's and says where they differ.
- `tests/pm` is a medium-model program that takes the place of the
  kernel's `main` and `trap`, linked with the kernel's own start-up
  (`usr/src/sys/i286`). It checks the descriptor tables, far calls between
  code segments, the far versions of the helper routines, that each kind of
  protection fault arrives as the right exception and can be returned from,
  memory above 1M, the clock interrupt and the 287.
- `tests/user` is two programs. A small-model one runs as a process in
  ring 3: it makes system calls, passes the kernel addresses it has no
  right to, and tries everything a process must not do. A medium-model
  one is the kernel under it, with the kernel's own entry and copy
  routines and a stand-in for `trap` and `syscall`; it carries the first
  as data.
- `tests/kernel` builds the kernel and boots it with a root file system
  made by `tools/mkfs.py`, which the kernel loads as its RAM disk. Its
  `/etc/init` is a test program: files, the break, `fork`, `execve`,
  `vfork`, `wait`, pipes, signals and faults, twelve processes at once in
  five slots, and 24 processes that need more memory between them than
  the kernel has been left, so that some are swapped out and in.
  It then boots five more times with the root file system and the swap
  area on a hard disk, IDE and SCSI ([disks](disks.md)), where the
  test also reads and writes a partition directly and keeps files of
  300K, and afterwards `tools/fsck.py` checks what the kernel left on
  the disk. Three of the five boot from the hard disk itself: the BIOS
  starts the boot block, which loads `/boot`, which loads `/unix`
  ([booting](boot.md)).
- `tests/libc` builds the C library and boots the kernel with a program
  that calls into each part of it as `/etc/init`. The program is built
  for each of the three models; the small one runs the medium one and
  then the large one when it is done, and the large one has checks of
  its own, of `malloc` over many segments and of `__huge` pointers.
  `CCOM=1 tests/libc/run.sh` does the same with the library and the
  program compiled by the system's own compiler, in its two models.
- `tests/system` builds the kernel, `/boot`, the library, the programs and
  the root file system as for a real machine, and boots them from an IDE
  disk, whose root is not marked clean, so that `/etc/rc` has `fsck`
  check it and starts the system again. It waits for `login:`, types
  `root`, gives the shell and then
  the C shell some commands, edits a file with `ex` and `vi`, uses
  `tcsh`'s line editor, all on the first serial port. Then it logs in on
  two of the console's screens with the keyboard, edits with `vi` on
  one, and checks what the screens showed. Back on the serial port it
  adds a user with `adduser`, makes a file system, checks it and mounts
  it, runs `ps`, `vmstat` and `iostat`, looks in the log for what the
  kernel and `logger` wrote there, fills a small file system so that the
  kernel complains, reads the accounting with `lastcomm` and `sa`,
  reckons with `expr`, `awk`, `bc` and `dc`, uses `egrep` and `make`,
  has the system's `yacc` and `lex` make of `awk`'s grammar and scanner
  the same C as the build's own made, pings the loopback interface,
  and logs in over it with `telnet` as the new user, who runs `ps`, `w`
  and `netstat`, gives `cron` and `at` each a job that must run as the
  user, and changes the full name and the password. It
  sets up the Ethernet interface, an NE2000 of the emulator's on a
  network of the emulator's own, pings the gateway, and the host logs in
  over it with telnet, sends a file each way, and edits with `vi` in a
  window of 200 by 80. Then it halts the
  machine, after which `tools/fsck.py` checks the disk
  ([the C library and programs](userland.md),
  [the console](console.md), [the network](network.md)).
- `tests/programs` boots that system too, with a file system of checks
  beside the root: 157 of them, each a few lines for the shell and what
  they must write, to the byte, for `sort`, `sed`, `diff`, `tar`,
  `patch`, `m4`, `compress` and the other programs that work on text and
  files, for `nroff`, which must format three of the manual's pages
  as the build did, and `man`, for `tbl`, `eqn`, `refer`, `spell` and
  the rest that documents go through, for `zic` and `zdump` and
  `/usr/adm`'s `daily`, `weekly` and `monthly`, for `disklabel`, `fsck`
  and its elders, `dump`, `restor`, `rdump`, `gcore`, `chflags` and
  `fstat`, with a second disk to write labels, file systems and dumps
  on, and for the games that can be played from a file. Then it uses
  the ones that need a terminal, `more`, `man`, `script`, `lock` and
  `tset`, `battlestar`, `hack`, `rogue`, `robots` and `hangman`, and
  `window`, and stops the system with `shutdown` ([the C library and
  programs](userland.md)).
- `tests/network` boots it with the Ethernet card and has it run 34
  checks of the same kind: `finger`, `rsh`, `rcp`, `tftp` and `ftp`, as
  a user and as the anonymous one, against their daemons on the loopback
  interface; `tcpd`'s rules and a connection it refuses; `nc`, `whois`
  and `traceroute`; `rwhod`, `timed`, `ntpd` and `routed` on the
  Ethernet card, with the programs that ask them, and `hunt` looking
  for a game there; and mail, sent, read, taken by SMTP and fetched by
  POP3. Then root uses `rlogin`, with a password and without, and talks
  with `talk` to a user whom the host has logged in with `telnet`, and
  whom `comsat` tells of a letter ([the C library and
  programs](userland.md#the-networks-programs)).
- `tests/bench` boots it and runs `bench`, this port's benchmark, while
  the harness times each test by the marks it writes and samples where
  the processor is. It prints what each test took and leaves
  `tools/kprof.py`'s account of where the time went. It is run for
  those, and to compare one build with another; it fails if `bench`,
  the timing or the profile does not work, or `top`, `tracebuf` or
  `sysctl`, which it tries on the way
  ([performance](performance.md)).
- `tests/vt` is the console's terminal emulator, which touches no
  hardware, built with the C compiler of the machine the system is built
  on and run there: what each sequence leaves on the screen, where it
  leaves the cursor and what the terminal answers, as a VT102 would.
- `tests/ve` is the editor `ve`, built there in the same way from the
  sources the system's is built from, with the system's termcap library
  and with `tests/ve/host` for what it has from the 286: segments, which
  come from `malloc`, the copying between them, and the terminal, of
  which there is none; and the file its blocks go to can be made to
  fill (`VEFULL`). Each check in `tests/ve/checks` types keys at it
  and gives what it must leave in a file, or on the screen, which
  `check.py` works out from what `ve` wrote as a terminal would. That
  it runs on the system is `tests/programs`'s to say.

The emulator is [86Box](https://github.com/86Box/86Box), as an
AT-compatible 286 with a 287 and one wait state
(`tests/harness/86box.cfg`): a real 286, which faults on anything a 286
lacks. `EMU=qemu` uses `qemu-system-i386` instead, which proves less: it
runs 386 instructions, and it does not enforce segment limits or the
write protection of code segments. `tests/pm` checks all three and so
refuses to run there.

`runtest.py` works with a released 86Box, which opens a window and runs in
real time. It does better with one built with the automation socket, an
option of 86Box's source that no release has turned on at the time of
writing: no window, the helper-routine tests in 3 seconds instead of 23,
and for a test that gives no result, the machine's screen and where it was
executing. To build one, follow 86Box's own build instructions with one
option added:

```sh
cmake -S . -B build/automation --preset regular -D AUTOMATION=ON
cmake --build build/automation
```

Then set `BOX86` to the program, or link the application as
`toolchain/86Box.app`. `doc/specifications/automation.md` in 86Box's source
describes the socket; adding `-D GDBSTUB=ON` gives a second build for
debugging, which waits for a GDB client before it runs.
