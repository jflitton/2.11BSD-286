# What is not ported

The programs, libraries and data in upstream's tree that the system does
not have yet, grouped by what each waits on, so that the next batch can
be chosen from here. The list was made by comparing the programs that
the build made, then the port's own Makefile, now upstream's Makefiles
(`make -C usr/src`), with the directories of `bin`, `sbin`,
`ucb`, `usr.bin`, `usr.sbin`, `libexec`, `usr.lib`, `games`, `new` and
`local`. When a program is ported, take it off.

A program marked *medium* was linked with overlays on the PDP-11 and
will be a medium-model program here, as `csh`, `ex`, `tcsh` and
`sendmail` are ([the C library and programs](userland.md)).

## What the kernel lacks first

Some programs wait on more than themselves:

| what | for |
|---|---|
| Quotas (`sys/sys/quota_*.c` are in the tree, and not built) | `quota`, `quotacheck`, `quotaon`, `edquota`, `repquota` |
| SLIP (`sys/net/if_sl.c`; `NSL` is 0) | `slattach` |
| A driver for the parallel port | a printer on it; `lpr` and the rest are ported, for a printer on a serial port or across the network |
| A tape driver | `mt`, `tcopy`, `ansitape`, and `dump` and `restor` to a tape |
| Crash dumps written to the swap area | `savecore` |

## The compiler and object files

Upstream's `cc` is the PDP-11's compiler, so a compiler on the system
took a new code generator for the 286, and an assembler, a linker and a
format for objects of its own. Those are there now: [the compiler, the
assembler and the linker](toolchain.md) describes them, and what of
upstream's goes with them is ported. What is left:

| what | why |
|---|---|
| Profiling: `cc -p`, `mcrt0.o`, `prof` | the second pass does not do it for the 286, and the start-up code for it is not written |
| `ld -r`, and so objects joined into one | not written |

And these of upstream's programs that work on programs, which are not
ported yet:

| program | where |
|---|---|
| `adb` and its scripts | `bin/adb`, `share/adb` |
| `f77` (*medium*), `efl` (*medium*), `ratfor`, `struct`, `pascal` | `usr.bin`, `ucb/pascal` |
| `libF77`, `libI77`, `libU77` | `usr.lib` |

Upstream has no `lint`: its `lint` is a script that says the old one
did not understand prototypes and that another is to come, and that
script is what the system has.

The second pass, `usr/src/lib/ccom/i286`, is upstream's reader of the
first pass's trees and its optimizer of expressions, with a generator
of its own where upstream has its tables of PDP-11 code. It does the
small model, the medium and the large, with floating point on the 287
and far pointers. `usr/src/lib/c2/i286` is `c2` for what it writes, on
upstream's plan: what upstream's does to jumps it does, and what
upstream's knows of the PDP-11's instructions is written again for the
286's. It knows too what `les` gave ES and BX, so that a far pointer
gone through again is not fetched again while nothing can have changed
it, and that what is by way of SS is the program's data, which the
large model names so. The compiler's code is nearly a tenth larger
than the cross compiler's: the 317 programs have 5,858,994 bytes of it
as the system's own tools make them, and 5,344,626 as the cross
compiler does.

A far pointer is declared as the cross compiler has it, with `__far`
before the `*`, and is the same four bytes: `char __far *p`, `int
(__far *f)()`. In the medium model a pointer to a function is far
without being said to be. The two compilers agree on what is done with
one: it is added to in its offset alone; two are equal if both words
are, and are ordered and subtracted by their offsets; a near pointer
made far takes the data segment, or the code segment if it points at a
function, and only the constant 0 is nowhere. What the tree's compiler
does not have of the cross compiler's:

- `__far` on anything but a pointer: a variable outside the data
  segment, as the kernel's `pseg` and `xmount` are, or a far function
  in the small model. It reads the declaration, and says so if the
  name is then used.
- `__huge` and based pointers, `__interrupt`, and `#pragma aux`, which
  `tools/ncc` takes out: `ntohs` and the like are then functions, and
  so is `alloca`, which the library has for this compiler alone
  (`usr/src/lib/libc/i286/gen/alloca.s`).
- More than four of `*`, `[]` and `()` in one type, where the PDP-11's
  compiler had five: the far pointer took the room. Nothing in the
  tree has five.

The two compilers do not lay every structure out alike, and either
builds the system, so what one's system writes the other's is to read:

- A structure of characters alone is filled out to a word by the
  tree's compiler, as on the PDP-11, and put at a word, and not by the
  cross compiler: `struct { char c[3]; }` is 4 bytes to one and 3 to
  the other.
- An `enum` is an `int` to the tree's compiler, and to the cross
  compiler the smallest type that holds its values.
- Bit fields of an `int` that leave its second byte unused leave that
  byte to a `char` after them to the tree's compiler, and the whole
  word to the cross compiler.
- A member that is a structure or union with no name is the cross
  compiler's; the tree's compiler has instead the old use of a
  member's name with any structure. `sed.h` has its union both ways,
  chosen by `__WATCOMC__`, which `tools/ncc` does not define.

`tools/layouts.py` has both compile every structure and union of
`usr/include` and of the kernel's headers, in each model, 338 of them,
and of the programs' own sources, in each one's model, 569, and
compares their sizes, their members' offsets and their bit fields'
bits. What differed and is in a file or passes between the kernel and
a program is the same to both now:

- `struct tty`, which `pstat` reads through `/dev/kmem`: a byte before
  `t_chars` puts it at 50 to the cross compiler as to the other, where
  it was at 49.
- `hack`'s `struct you`, in its save files: the rest of the word of its
  bit fields is said, which leaves it as the cross compiler had it.
- `hack`'s fake corridors, in the vault guard's part of a monster in
  its save and bones files: 4 bytes to both, where they were 3 to the
  cross compiler, whose `hack` now writes a guard so, and does not read
  one it wrote before.

What still differs is in one program's memory alone: `struct uio`, the
kernel's, by its `enum`s; `popper`'s state, by an `enum`; `tar`'s
`struct header`, whose members are where they were in the 512 bytes of
`union hblock` that go on the tape; `addbib`'s skeleton; `monop`'s
`struct rr_st`, whose save files are its data as they are in memory,
which only the same program reads; and `hack`'s table of what a
character starts with. Nothing on the network differs, and the host
tools that write a structure for the root file system and the disks lay
it out as both compilers do (`tests/layouts`).


## Typesetting and text

`nroff` is ported, with `tbl`, `eqn`, `neqn`, `refer`, `spell`, `style`,
`diction` and `indent` ([the C library and programs](userland.md)).
These are the rest of the writer's tools.

| program | where | notes |
|---|---|---|
| `vgrind`, `vfontedpr` | `ucb/vgrind` | its output is for `troff` |
| `troff`, and `tc`, which shows its output on a Tektronix 4015 | `usr.bin/roff/troff`, `usr.bin` | `troff` drives a C/A/T phototypesetter |
| `tk` | `usr.bin` | pages `nroff`'s output on a Tektronix 4014 |
| `graph`, `plot` and its filters, `libplot` | `usr.bin`, `usr.lib/libplot` | for plotters and graphics terminals |

## The network

| program | where | notes |
|---|---|---|
| `sendmail.MX` (*medium*) | `usr.sbin/sendmail.MX` | a `sendmail` that looks up MX records with `named` |
| `mkhosts` | `usr.sbin/mkhosts` | `/etc/hosts` as a database. Its Makefile is run, as upstream's is, and installs nothing: `mkhosts` needs `gethostent`, which the C library has only when it reads the table of hosts itself, and it asks `named`, as upstream's does (`HOSTLOOKUP` in `lib/libc/Makefile`) |
| `uucp` (*medium*) | `usr.bin/uucp` | `rmail`, which hands mail from it to `sendmail`, is ported |
| `tn3270` | `ucb/tn3270` | |
| `slattach` | `sbin/slattach` | waits on SLIP |
| `testnet` | `sbin/testnet` | upstream's `netstart` asks it whether the kernel has a network; the port's does not |

## Disks, files and administration

| program | where | notes |
|---|---|---|
| `savecore` | `sbin` | waits on crash dumps |
| `quota`, `quotacheck`, `quotaon`, `edquota`, `repquota` | `ucb`, `sbin`, `usr.sbin` | wait on quotas |

## Printers and tapes

| program | where | notes |
|---|---|---|
| `mt`, `tcopy` | `bin`, `ucb` | |
| `ansitape` | `new/ansi` | |
| `vmsbackup` | `new/vmsbackup` | reads VMS's backup tapes |

## Games

All are ported but `games/pdp`, which is in PDP-11 assembly and is left
out.

## `new` and `local`

| program | where | notes |
|---|---|---|
| `rn` (*medium*), `nntp` | `new/rn`, `new/nntp` | |
| `kermit` (*medium*) | `new/kermit5.188` | may not be sold or go with a commercial product |
| `rcs` | `new/rcs` | may not be distributed without its author's permission |

[COPYRIGHT](../COPYRIGHT) has the restrictive notices in full.

## Libraries

Built: `libc`, `libom`, which `usr.lib`'s Makefile links to `libm` as
upstream's does, `libutil`, `libtermcap`, `libcurses`, `libdbm`,
`libwrap`, `libln`, `liby`, `libident` and `liberrlst`, whose messages
`mkerrlst` makes into `/etc/syserrlst`. Not built, besides those above:
`libmp`
(numbers of any size) and `libvmf` (virtual memory in a file).

## Not to be ported

These are the PDP-11's, or are for what the PDP-11 needed:

| program | why |
|---|---|
| `gt307` | a clock board for the PDP-11 |
| `rxformat` | formats an RX02 diskette; `flformat` is it for a PC's |
| `bad144` | DEC's table of bad sectors; neither driver forwards them |
| `sysgen` | gives Qbus and UNIBUS devices their addresses |
| `tp` | DECtape's format |
| `lib2648` | graphics on an HP 2648 terminal |
| `ctimed` | does `ctime` for programs that had no room for it |
| `mkovmake`, `libstubs`, `strcompact`, `symcompact`, `symdump`, `symorder` | overlays, and the PDP-11's kernel's symbols |
| `m11`, `l11` | the Macro-11 assembler, written in Macro-11, and its linker |
| `libkern` | the PDP-11 kernel's part of the C library; the 286's kernel compiles its own (`sys/conf/Make.i286`) |
| `games/pdp` | PDP-11 assembly |
