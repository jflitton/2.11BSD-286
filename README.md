# 2.11BSD for the 80286

The 286 deserves a high-quality open-source UNIX complete with TCP/IP,
and now it has one.

This is 2.11BSD, at its current patch level, ported to the 286 in
protected mode. It boots from a hard disk to multi-user. Users can log
In via the local consoles (4 total), at a serial terminal, or over the
network with `telnet` or `rlogin`.  Shells include `sh`, `csh`, and `tcsh`.
Most programs have been ported (317 to date), among them `vi`, mail, the
manual and a C compiler. It runs in 86Box and on real machines.

## Lineage

2.11BSD is the last of Berkeley's releases for the PDP-11. It brought
parts of 4.3BSD (which ran on 32-bit VAX), with its TCP/IP, the C shell,
`vi` and `sendmail`, to a 16-bit machine. It first came out in 1991 and
is still maintained: patch level 500, from 2026, is what
this port starts from.

The PDP-11 and the 286 are close enough that the tree carries over:

- Both are 16-bit machines whose programs see 64K at a time
- Both map those spaces into a larger physical memory, under a kernel
  that user programs cannot touch: the PDP-11 into as much as 4 MB, the
  286 into 16 MB, through a table of descriptors for each process.
- Neither pages. 2.11BSD swaps whole processes.
- Where the PDP-11 runs out, the 286 goes on. A program with more than
  64K of code needed overlays on the PDP-11; here its code is in several
  segments, with far calls between them. And a process can have more
  than one segment of data, which the PDP-11 could not provide.

## Hardware

- An AT-compatible 286 with a 287
- 1 MB of memory required; 2 MB or more for several users at once
  ([how much memory](docs/performance.md#how-much-memory)).
- A hard disk: IDE on an AT controller, or SCSI on an Adaptec
  AHA-1540/1542.
- VGA and keyboard, a terminal on a serial port, or both.
- NE1000 and NE2000 compatible network cards (optional)

## Status

- Programs are small- or medium-model, but for `window`, the first of
  the large model, which has as much data as the system gives a
  process. A program of any model can ask for more data segments
  (`segalloc(2)`), which is where the editor `ve` keeps the text it is
  at work on ([memory model](docs/memory-model.md)).
- `cc` is on the system, with an assembler and a linker of its own
  ([the compiler](docs/toolchain.md)). They can build every program and
  library, themselves included, on the 286, and the kernel where the
  system is built ([the kernel](docs/toolchain.md#the-kernel)): the
  system is built with them.
- Not there yet: `named` and `identd`; `troff`; the line printer's
  programs and UUCP; quotas in the kernel. [What is not
  ported](docs/unported.md) has the whole list.
- Upstream's programs are kept as 2.11BSD has them, changed only for
  bugs, the compiler and each machine's settings. Newer behaviour goes
  in programs of the port's own: so far `adduser`, `bench` and `ve`, an
  editor in the manner of `vim`.

## Building

The system is built on Linux or macOS by its own compiler, assembler
and linker, which the host's C compiler makes first. The repository
must be on a case-sensitive file system: the tree has file names that
differ only by case.

```sh
make -C usr/src/sys/PC                # the kernel
make -C usr/src/sys/i286stand         # the boot sectors and /boot
make -C usr/src/lib/libc/i286         # the C library
make -C usr/src/i286 all root         # the programs and the root file system
python3 tools/mkdisk.py -p -b obj/nstand/hdboot.bin -m obj/nstand/mbr.bin \
    disk.img a=obj/root/root.fs b=swap:4000
```

A cross compiler, Open Watcom v2 with NASM, builds it as well, with
`TOOLS=cross` given to each `make`
([building](docs/development.md#building)); some of the tests use it.

The last line makes a bootable image of a 20 MB IDE disk. [Building and
testing](docs/development.md) has the rest: SCSI disks, the tools, and
the tests, which boot the system in 86Box and check what it does.

## Documentation

- [Building and testing](docs/development.md): the layout, the
  toolchain, the tools and the tests.
- [The upstream branch](docs/upstream.md): 2.11BSD as distributed, so
  that `git diff upstream main` is the port.
- The design: [memory model](docs/memory-model.md), [calling convention
  and kernel interface](docs/abi.md), [executable
  format](docs/executable-format.md), [the compiler, the assembler and
  the linker](docs/toolchain.md), [booting](docs/boot.md),
  [processes](docs/processes.md), [disks](docs/disks.md).
- The system: [the console](docs/console.md), [the
  network](docs/network.md), [the C library and programs](docs/userland.md),
  [performance](docs/performance.md).
- [What is not ported](docs/unported.md), and what each program waits
  on.

## License

The port's own work is under the BSD 3-clause license in
[LICENSE](LICENSE). The 2.11BSD tree keeps its own terms, file by file;
[COPYRIGHT](COPYRIGHT) says whose each part is.
