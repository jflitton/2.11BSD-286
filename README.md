# 2.11BSD for the 80286

A port of 2.11BSD, at its current patch level, to the 286 in protected mode.
The aim is a system that maps cleanly onto the 286 and runs larger programs
than the PDP-11 one could, by giving a process more than one data segment.

Target machine: an IBM PS/2 Model 30 286, which is an AT-compatible, with an
80287, an Adaptec AHA-1542CP SCSI adapter and an NE2000-compatible network
card. The 287 is required.

This repository must live on a case-sensitive file system: the 2.11BSD tree
has file names that differ only by case.

## Map

| path | what | in git |
|------|------|--------|
| `usr/src/` | the 2.11BSD source tree, at its real path | yes |
| `usr/src/sys/` | the kernel; `sys/`, `h/`, `net*/` are machine-independent, `pdp*/` and `mdec/` are the PDP-11 | yes |
| `usr/include/` | the system's headers; mostly links into `usr/src/sys` | yes |
| `sys` | link to `usr/src/sys`, as on the running system; the links in `usr/include` go through it | yes |
| `tools/` | host-side tools (below) | yes |
| `tests/` | tests that boot in an emulator (below) | yes |
| `docs/` | our notes | yes |
| `reference/MANIFEST` | what third-party material we use, where it comes from, checksums | yes |
| `reference/upstream/` | the install tape, patches, setup guide | no |
| `reference/manuals/` | Intel, IBM, Adaptec and DEC manuals; Ralf Brown's Interrupt List | no |
| `reference/prior-art/` | MINIX, RetroBSD, DiscoBSD, ELKS, Coherent sources | no |
| `reference/toolchain/` | the cross-compiler's distribution archive | no |
| `toolchain/` | the installed cross-compiler | no |
| `obj/`, `images/`, `vm/` | build output, disk images, emulator state (none yet) | no |

Where the 286 code will go, beside its PDP-11 counterpart in each case:

| new | beside | holds |
|-----|--------|-------|
| `usr/src/sys/i286/` | `pdp/` | startup, traps, segments and tasks, 287, copy routines |
| `usr/src/sys/i286at/` | `pdpuba/` | AT-bus drivers: SCSI, console, serial, clock |
| `usr/src/sys/i286if/` | `pdpif/` | network interfaces |
| `usr/src/sys/i286stand/` | `pdpstand/`, `mdec/` | boot blocks and standalone programs |
| `usr/src/sys/PS2_30/` | `GENERIC/` | the kernel configuration |
| `usr/src/lib/libc/i286/` | `lib/libc/pdp/` | libc's machine-dependent part; `crt/` exists, the rest is to come |

The kernel picks its machine through the link `usr/src/sys/machine`, which
points at `pdp` upstream.

## Branches

- `upstream` is 2.11BSD exactly as distributed, one commit per patch level.
  Nothing of ours goes on it.
- `main` branches from it. `git diff upstream main -- usr sys` is the port.

How `upstream` was made and how to take a new patch: [docs/upstream.md](docs/upstream.md).

## Toolchain

The system is cross-compiled with Open Watcom v2, which runs on Linux (x64,
arm64) and macOS (x64, arm64).

```sh
tools/fetch-reference.py toolchain   # the archive, about 150 MB
tools/install-watcom.sh              # unpack this host's build into toolchain/
. tools/env.sh                       # wcc, wasm, wlink, wlib, wdis on PATH
```

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

The AHA-1542CP is the Plug and Play member of the 1540C series. Its guide
says it also runs in machines without Plug and Play, at the settings on its
switches and in SCSISelect (defaults: port 330h, IRQ 11, DMA 5), so the
1540C technical reference describes what the driver talks to.

Not found: the *IBM PS/2 Model 30 286 Technical Reference*. No copy has
turned up on line. (Held's *IBM PS/2 User's Reference Manual*, 1989, is a
DOS and OS/2 user's guide with nothing on ports or interrupts; not kept.)

## Tools

| tool | does |
|------|------|
| `tools/fetch-reference.py` | fetches `reference/` from the manifest |
| `tools/install-watcom.sh` | installs the cross-compiler for this host into `toolchain/` |
| `tools/env.sh` | sourced, puts the toolchain on `PATH` |
| `tools/cc286`, `tools/cc286.h` | compile one C file with the system's memory model and calling convention |
| `tools/ne2aout.py` | turns the linker's output into the system's executable format; `--dump` prints one |
| `tools/untap.py` | splits a SimH `.tap` tape image into its files |
| `tools/tar2git.py` | commits tar archives to a branch without unpacking them |
| `tools/import-upstream.sh` | rebuilds the `upstream` import from the tape, to check it |

## Tests

Tests run real 286 code in an emulator, in real mode for now.
`tests/harness/` has a boot sector that loads a small-model program's two
segments from a diskette image, start-up code with a serial console, and
`runtest.py`, which builds a program through the normal path (`cc286`,
`wlink`, `ne2aout.py`), boots it, prints what it wrote to the serial port
and takes main's return value as the result.

```sh
. tools/env.sh
tests/crt/run.sh        # the compiler's helper routines: 5,161 checks
```

The emulator is [86Box](https://github.com/86Box/86Box), as an IBM PS/2
Model 30 286 with a 287 (`tests/harness/86box.cfg`): a real 286, which
faults on anything a 286 lacks. `EMU=qemu` uses `qemu-system-i386` instead,
which is a later processor and so proves less.

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

Assembly is NASM syntax, in files ending `.s` as elsewhere in the tree.

## Status (2026-11-17)

- `upstream` holds patch level 500, checked file by file against the tape.
- Reference material is listed and fetched.
- The cross-compiler is chosen and installs from the manifest (see
  Toolchain). Nothing compiled with it has been run yet.
- The design is decided and written down:
  [memory model](docs/memory-model.md),
  [calling convention and kernel interface](docs/abi.md),
  [executable format](docs/executable-format.md).
- A C program can be compiled, linked, converted to the executable format
  and run in an emulated PS/2 Model 30 286, in real mode.
- The helper routines compiled code calls for 32-bit multiply and divide
  and for float-to-integer conversion are written
  (`usr/src/lib/libc/i286/crt`) and pass their tests. The three for
  `__huge` pointers are not written yet.
- Nothing of the system itself is ported yet. Next: a kernel skeleton that
  reaches protected mode in an emulator.
