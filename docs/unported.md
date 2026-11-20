# What is not ported

The programs, libraries and data in upstream's tree that the system does
not have yet, grouped by what each waits on, so that the next batch can
be chosen from here. The list was made by comparing the programs that
`usr/src/i286/Makefile` builds with the directories of `bin`, `sbin`,
`ucb`, `usr.bin`, `usr.sbin`, `libexec`, `usr.lib`, `games`, `new` and
`local`. When a program is ported, take it off.

A program marked *medium* was linked with overlays on the PDP-11 and
will be a medium-model program here, as `csh`, `ex`, `tcsh` and
`sendmail` are ([the C library and programs](userland.md)).

## What the kernel lacks first

Some programs wait on more than themselves:

| what | for |
|---|---|
| A library for the large model, and the helpers for `__huge` pointers | programs with more than 64K of data |
| Quotas (`sys/sys/quota_*.c` are in the tree, and not built) | `quota`, `quotacheck`, `quotaon`, `edquota`, `repquota` |
| SLIP (`sys/net/if_sl.c`; `NSL` is 0) | `slattach` |
| A driver for the parallel port | the line printer's programs, but for a printer on a serial port |
| A diskette driver | `mtools`, and `tar` to a diskette |
| A tape driver | `mt`, `tcopy`, `ansitape`, `dump` and `restor` to tape |
| Crash dumps written to the swap area | `savecore` |

## The compiler and object files

Upstream's `cc` is the PDP-11's compiler, so a compiler on the system is
a new code generator for the 286 and its own linker and object format,
not a port. Everything here waits on that.

The code generator is begun. `usr/src/lib/ccom/i286` is a second pass
for the 286: upstream's reader of the first pass's trees and its
optimizer of expressions, and a generator of its own where upstream has
its tables of PDP-11 code. The first pass is upstream's, with what
differs for the 286 under `#ifdef i286`. So far it does the small
model and the medium, with floating point on the 287 and far pointers,
and not profiling. It does not run on the system yet: it is
built for the machine the system is built on (`tools/ncc`), writes
assembly for NASM, and its objects are linked by the cross tools
(`tests/ccom`). An assembler, a linker and a format for objects are
still to come, and `/lib/cpp`, which needs `libvmf`.

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
  `tools/ncc` takes out: `ntohs` and the like are then functions.
- More than four of `*`, `[]` and `()` in one type, where the PDP-11's
  compiler had five: the far pointer took the room. Nothing in the
  tree has five.

The two compilers do not lay every structure out alike, and what one
compiles cannot yet share these with what the other does:

- A structure of characters alone is filled out to a word by the
  tree's compiler, as on the PDP-11, and not by the cross compiler:
  `struct { char c[3]; }` is 4 bytes to one and 3 to the other.
- An `enum` is an `int` to the tree's compiler, and to the cross
  compiler the smallest type that holds its values.
- A member that is a structure or union with no name is the cross
  compiler's; the tree's compiler has instead the old use of a
  member's name with any structure. `sed.h` has its union both ways,
  chosen by `__WATCOMC__`, which `tools/ncc` does not define.

Of 124 structures in the headers of `usr/include` that both compilers
were given, one differs in size: `struct uio`, which only the kernel
uses, by its two `enum`s. About a quarter of the headers were not
compared.

| program | where |
|---|---|
| `cc`, `/lib/cpp`, `c2`, `ccom` | `bin/cc`, `lib/cpp`, `lib/c2`, `lib/ccom` |
| `as`, `ld`, `ar`, `nm`, `size`, `ranlib`, `lorder` | `bin`, `usr.bin` |
| `adb` and its scripts | `bin/adb`, `share/adb` |
| `lint`, `prof`, `mkdep`, `error` | `usr.bin`, `ucb/error` |
| `f77` (*medium*), `efl` (*medium*), `ratfor`, `struct`, `pascal` | `usr.bin`, `ucb/pascal` |
| `liby`, `libF77`, `libI77`, `libU77` | `usr.lib` |
| `calendar`, which runs `/lib/cpp` | `usr.bin/calendar` |

## Typesetting and text

`nroff` is ported; these are what it is given documents through, and
the rest of the writer's tools.

| program | where | notes |
|---|---|---|
| `tbl`, `eqn`, `neqn` | `usr.bin` | `eqn` has a grammar for the tree's `yacc` |
| `refer`, `addbib`, `lookbib`, `sortbib`, `indxbib` | `usr.bin/refer` | |
| `spell`, `spellin`, `spellout` and the word lists | `usr.bin/spell` | |
| `diction`, `explain` | `usr.bin/diction` | |
| `vgrind`, `vfontedpr` | `ucb/vgrind` | its output is for `troff` |
| `indent` | `ucb/indent` | |
| `learn` and its lessons | `usr.bin/learn`, `share/learn` | most lessons want `cc` |
| `troff`, and `tc`, which shows its output on a Tektronix 4015 | `usr.bin/roff/troff`, `usr.bin` | `troff` drives a C/A/T phototypesetter |
| `tk` | `usr.bin` | pages `nroff`'s output on a Tektronix 4014 |
| `graph`, `plot` and its filters, `libplot` | `usr.bin`, `usr.lib/libplot` | for plotters and graphics terminals |

## The network

| program | where | notes |
|---|---|---|
| `named` and its tools (*medium*) | `usr.sbin/named` | |
| `sendmail.MX` (*medium*) | `usr.sbin/sendmail.MX` | a `sendmail` that looks up MX records with `named` |
| `identd`, `libident` | `libexec/identd`, `usr.lib/libident` | |
| `mkhosts` | `usr.sbin/mkhosts` | `/etc/hosts` as a database |
| `rdist` | `ucb/rdist` | has a grammar |
| `tip` | `usr.bin/tip` | |
| `uucp` (*medium*), `rmail` | `usr.bin/uucp`, `bin/rmail` | |
| `tn3270` | `ucb/tn3270` | |
| `slattach` | `sbin/slattach` | waits on SLIP |
| `trpt`, `trsp` | `usr.sbin` | want the kernel's TCP and SPP traces turned on |
| `testnet` | `sbin/testnet` | upstream's `netstart` asks it whether the kernel has a network; the port's does not |

## Disks, files and administration

| program | where | notes |
|---|---|---|
| `dump`, `restor` | `sbin` | to a file as well as a tape |
| `chflags` | `bin/chflags` | the kernel has the calls |
| `zic`, `zdump` and the zone files | `share/zoneinfo` | with none, the time is UTC |
| `gcore`, `savecore` | `ucb`, `sbin` | `savecore` waits on crash dumps |
| `quota`, `quotacheck`, `quotaon`, `edquota`, `repquota` | `ucb`, `sbin`, `usr.sbin` | wait on quotas |
| `window` | `ucb/window` | |
| `sendbug` | `ucb/sendbug` | |
| `la` | `new/la` | the load averages |

## Printers and tapes

| program | where | notes |
|---|---|---|
| `lpr`, `lpq`, `lprm`, `lpc`, `lpd`, `pac` | `usr.sbin/lpr` | |
| the printers' filters | `usr.sbin/lpr/filters` | most are for Versatec and Varian plotters |
| `mt`, `tcopy`, `rmt` | `bin`, `ucb`, `usr.sbin` | `rmt` serves another machine's `dump` |
| `ansitape` | `new/ansi` | |
| `vmsbackup` | `new/vmsbackup` | reads VMS's backup tapes |

## Games

All are ported but `games/pdp`, which is in PDP-11 assembly and is left
out.

## `new` and `local`

| program | where | notes |
|---|---|---|
| `jove` (*medium*) | `new/jove` | |
| `rn` (*medium*), `nntp` | `new/rn`, `new/nntp` | |
| `afio`, `cxref` | `local` | may not be sold |
| `qterm`, `welcome` | `local` | |
| `less` | `local/less` | may not be sold for profit without its author's consent |
| `kermit` (*medium*) | `new/kermit5.188` | may not be sold or go with a commercial product |
| `zmodem` | `local/zmodem` | for non-commercial use |
| `ddd` | `local/ddd` | for non-commercial use |
| `rcs` | `new/rcs` | may not be distributed without its author's permission |
| `mtools` | `local/mtools` | waits on a diskette driver |

[COPYRIGHT](../COPYRIGHT) has the restrictive notices in full.

## Libraries

Built: `libc`, `libm`, `libutil`, `libtermcap`, `libcurses`, `libdbm`,
`libwrap` and `libln`; `liberrlst`'s messages are made into
`/etc/syserrlst` at build time, by `tools/mkerrlst.py` in the place of
`mkerrlst`. Not built, besides those above: `libmp` (numbers of any
size) and `libvmf` (virtual memory in a file).

## Not to be ported

These are the PDP-11's, or are for what the PDP-11 needed:

| program | why |
|---|---|
| `gt307` | a clock board for the PDP-11 |
| `rxformat` | formats an RX02 diskette |
| `bad144` | DEC's table of bad sectors; neither driver forwards them |
| `sysgen` | gives Qbus and UNIBUS devices their addresses |
| `tp` | DECtape's format |
| `lib2648` | graphics on an HP 2648 terminal |
| `ctimed` | does `ctime` for programs that had no room for it |
| `mkovmake`, `libstubs`, `strcompact`, `symcompact`, `symdump`, `symorder` | overlays, and the PDP-11's kernel's symbols |
| `m11`, `l11` | the Macro-11 assembler, written in Macro-11, and its linker |
| `libom` | older mathematics, which `libm` replaces |
| `games/pdp` | PDP-11 assembly |
