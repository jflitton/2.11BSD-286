# The compiler, the assembler and the linker

The system has its own C compiler and what goes with one: `cc`,
`/lib/cpp`, the compiler's passes, `as`, `ld`, `ar`, `ranlib`, `nm`,
`size`, `strip` and `lorder`, with `/usr/include` and the libraries. A
program is compiled on the system as on any UNIX:

```sh
cc -O -o prog prog.c sub.c -lm      # small model
cc -mm -o big big.c                 # medium model: far code
```

The system itself is still built with the cross compiler, Open Watcom
([building](development.md)). The same tools, compiled for the machine
the system is built on, make it too (`tools/cc -ccom`), and
`tests/ccom/programs.sh` boots a system that is all their work. And
the system makes them of their sources itself ([below](#the-system-makes-its-own-compiler)).

## What runs what

`cc` is upstream's, and runs for each file:

| | | from |
|---|---|---|
| `/lib/cpp` | the preprocessor | upstream's (`lib/cpp`) |
| `/lib/c0` | the compiler's first pass: C to trees | upstream's (`lib/ccom`), with what differs for the 286 under `#ifdef i286` |
| `/lib/c1` | its second pass: trees to assembly | the 286's own (`lib/ccom/i286`): upstream's reader and optimizer of trees, with a code generator in place of the PDP-11's tables |
| `/lib/c2` | with `-O`, the object code improver | the 286's own (`lib/c2/i286`), on upstream's plan |
| `/bin/as` | the assembler | new (`bin/as/i286`) |
| `/bin/ld` | the linker | new (`bin/ld/i286`), on upstream's plan |

Upstream's assembler is written in PDP-11 assembly and its linker is
about overlays and the PDP-11's relocation, so those two are written
again. `ar`, `ranlib`, `nm`, `size`, `strip` and `lorder` are
upstream's, taught the format where they read it.

The compiler's two passes and `c2` are compiled by the compiler itself,
whatever compiles the rest of the system: they are written for it.
`tests/ccom` has them compile a program on the system and compares what
they write with what they wrote where the system was built.

## Models

`cc` makes a small-model program unless it is given `-mm`
([memory models](memory-model.md)). The flag goes to every step: the
first pass makes pointers to functions far, the second makes calls far
and is told the name of the segment for the file's code, the assembler
records the model in the object, and the linker refuses to mix them.

The start-up code and the libraries are per model:

| | small | medium |
|---|---|---|
| start-up | `/lib/crt0.o` | `/lib/m/crt0.o` |
| C library | `/lib/libc.a` | `/lib/m/libc.a` |
| others (`-lNAME`) | `/usr/lib/libNAME.a` | `/usr/lib/m/libNAME.a` |

`ld -mm` looks in the `m` directories for `-l`. The libraries on the
system are `libc`, `libm`, `libtermcap`, `libcurses`, `libutil`,
`libdbm`, `libln`, `liby` and `libwrap`.

## The assembler

`as` reads the syntax of NASM, as much of it as the compiler writes and
the C library's own assembly uses. Those were written for NASM before
the system had an assembler, and either assembles them still, which is
how the assembler is tested: `tests/as` gives both the same files and
compares what they make.

```
as [-o object] [-Dname[=text]] [-Idir] [-ms|-mm] file.s
```

What it has:

- Every instruction of the 8086, the 186 and the 286, the protected
  mode's among them, and the 287's.
- Operands as NASM writes them: `word [bp-4]`, `byte [es:bx+_tab+2]`,
  `far [bx]`, `seg _name`, `st1`.
- `db`, `dw`, `dd`, `resb`, `resw`, `resd`, `times`, `align`, `equ`,
  `global`, `extern`, `common`, `segment`.
- Labels local to the last label without a dot (`.loop`), and `$`.
- `%include`, `%define` without arguments, `%ifdef`, `%ifndef`,
  `%else`, `%endif`, and `%macro` with `%1` to `%9`.

A file's segments are three: every `segment` of class `CODE` is the
text, of class `BSS` the bss, and any other the data, in the order the
file has them. `group`, `bits` and `cpu` are read and mean nothing.

A jump to a label takes the short form if that reaches and the long one
if not; a conditional jump out of reach becomes the opposite condition
round a jump, which the 286 needs. The compiler leaves that to the
assembler, as the PDP-11's does. It is worked out between the two
passes, starting from every jump short and lengthening those that do
not reach until none is left, so the code is as short as it can be.

A name that is not defined in the file must be declared `extern`, which
the compiler does. Labels the compiler made (`L12`) and local ones are
not put in the object's symbol table. The others are written in the
order of the assembler's table of them, which is reckoned in 16 bits
whatever an `int` is where the assembler runs, so that an object is the
same to the byte wherever it is assembled.

## Object files

An object is the [executable format](executable-format.md) with another
magic number (`A_OBJECT`, `sys/exec.h`), three segments always, and
relocation (`a.out.h`):

| | |
|---|---|
| header | as a program's; the model, or 0 if the assembler was not told |
| three segment entries | text, data, bss; each with the number of its relocation entries |
| text, data | each made up to an even length; the bss has nothing in the file |
| text's relocation, then data's | 6 bytes an entry, in the order of the addresses |
| symbol table, string table | as a program's |

A relocation entry names a word of its segment, and a symbol by its
number in the symbol table or one of the file's own three segments:

| type | what the linker does to the word |
|---|---|
| `R_OFF` | adds where the symbol, or the segment's part from this file, is in its segment |
| `R_SEG` | puts in the selector of the segment that is in |
| `R_PCREL` | adds how far that is from where this file's part of the word's own segment begins: a near call or jump |

The PDP-11 has a word of relocation for every word of text and data. A
286 instruction has its addresses at any byte, and few words are
addresses, so here it is a list.

A symbol's type says which of the three it is in, text, data or bss,
and its value is where in that. (`n_ovly` is 0 in an object; in a
program it is the number of the symbol's segment.) An undefined symbol
with a value is common storage of that size, as on the PDP-11.

A library is `ar`'s archive with `ranlib`'s table of contents, both as
upstream has them.

## The linker

```
ld [-o out] [-ms|-mm] [-s] [-x] [-stack n] [-packcode n] [-e sym]
   [-u sym] [-L dir] [-M] [-t] file ... -lx ...
```

It reads its arguments twice, as upstream's does: once for the symbols
and the sizes, taking from each library the members that define what is
still undefined, and once to copy the text and the data, relocating
them. A library is searched until nothing more is wanted from it, and
libraries are searched in the order given.

A program's data, with the bss and then the common storage after it, is
its last segment. In the small model the text is one segment. In the
medium model each file's text is whole in one segment and files share a
segment, in the order they are linked, until the next would not fit;
`-packcode n` makes the segments smaller than 64K. A near call or jump
from one file to another that has landed in a different segment is an
error: the library's system call stubs reach their common error path
with a far jump for that reason.

A segment's selector follows from its number, so the linker fills
selectors in and the program is loaded as it is. `-stack n` sets the
stack the header asks for (8192 without). The entry point is the symbol
`..start`, which the start-up code defines, or `-e`. `_etext`, `_edata`
and `_end` are defined if something asks for them.

The PDP-11's `-i`, `-n` and `-z` are taken and ignored; its overlays
(`-Z`, `-L`, `-Y`) and `-r` are not there.

## Where the system is built

The same sources are compiled for the machine the system is built on
(`make -C usr/src/lib/ccom/i286 host`, which makes `c0`, `c1`, `c2`,
`as` and `ld` in `obj/host/ccom`):

| | |
|---|---|
| `tools/ncc` | compiles one file: the cross compiler's preprocessor, then `c0`, `c1`, `c2` with `-O`, and `as` |
| `tools/cc -ccom` | `cc` with those in place of the cross compiler, `as` for assembly, and `ld` |
| `tools/ar.py` | makes a library with its table of contents, as `ar` and `ranlib` do on the system |
| `make -C usr/src/lib/libc/i286 CCOM=1` | the C library that way, in `obj/nlib` |
| `make -C usr/src/i286 CCOM=1` | every program that way, in `obj/nbin` |

The libraries on the root file system are those of `obj/nlib` whichever
compiler made the programs, since the cross compiler's objects are of
another format. `make root` makes them first.

The headers the objects are written with have a `long` of 32 bits and a
pointer of 16; the host's copies of them are made with the types put
right (`obj/host/ccom/include`). The host has to keep a number's low
byte first.

## The system makes its own compiler

`tests/ccom/self.sh` boots the system from a disk that has, on a file
system of their own, the sources of the eleven programs above that are
C: `cc`, `cpp`, `c0`, `c1`, `c2`, `as`, `ld`, `ar`, `ranlib`, `nm` and
`size` (`make -C usr/src/i286 ccprogs` lists what each is made of). The
system compiles, assembles and links each one, and each must be, to the
byte, what `tools/cc -ccom` makes of it where the system is built. Then
it puts them where the ones it booted with were, and does it again with
those: the second time the compiler is the one the system made.

All 22 are the same.

Two things came of the first time it was tried, neither of which a
program of two small files had shown ([what the programs
needed](userland.md#the-compiler)): `/lib/cpp` as the cross compiler
compiled it knew no macro that had no arguments, and the assembler's
objects had their symbols in another order on the 286 than where the
system is built.

## What is not there

- The compiler cannot compile the kernel: far variables and functions,
  `__interrupt`, `__huge` and the cross compiler's pragmas are not in
  it.
- Profiling (`cc -p`, `mcrt0.o`, `prof`), `adb` and `ld -r`.
- The large model.
