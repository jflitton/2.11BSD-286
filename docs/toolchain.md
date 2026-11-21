# The compiler, the assembler and the linker

The system has its own C compiler and what goes with one: `cc`,
`/lib/cpp`, the compiler's passes, `as`, `ld`, `ar`, `ranlib`, `nm`,
`size`, `strip` and `lorder`, with `/usr/include` and the libraries. A
program is compiled on the system as on any UNIX:

```sh
cc -O -o prog prog.c sub.c -lm      # small model
cc -mm -o big big.c                 # medium model: far code
cc -ml -o bigger bigger.c           # large model: far data as well
```

The system itself is still built with the cross compiler, Open Watcom
([building](development.md)). The same tools, compiled for the machine
the system is built on, make it too: the programs (`tools/cc -ccom`),
and `tests/ccom/programs.sh` boots a system that is all their work;
the kernel ([below](#the-kernel)); and the programs that load it
([booting](boot.md)). And the system makes them of
their sources itself ([below](#the-system-makes-its-own-compiler)).

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

`cc` makes a small-model program unless it is given `-mm` or `-ml`
([memory models](memory-model.md)). The flag goes to every step: the
first pass makes pointers to functions far, the second makes calls far
and is told the name of the segment for the file's code, the assembler
records the model in the object, and the linker refuses to mix them.
A function the file has defined already is called near, with CS
pushed for its far return; and a static function whose address
nothing takes returns near, taking that CS off the stack (`ret 2`),
which is 11 clocks in protected mode where a far return is 25.

The start-up code and the libraries are per model:

| | small | medium | large |
|---|---|---|---|
| start-up | `/lib/crt0.o` | `/lib/m/crt0.o` | `/lib/l/crt0.o` |
| C library | `/lib/libc.a` | `/lib/m/libc.a` | `/lib/l/libc.a` |
| others (`-lNAME`) | `/usr/lib/libNAME.a` | `/usr/lib/m/libNAME.a` | `/usr/lib/l/libNAME.a` |

In the small and medium models a pointer to data is near unless it is
declared `__far`, which is how a program reaches the segments
`segalloc` gives it. A far pointer to `void` is assigned to and from
any other pointer to data without a cast, as a near one is; that is
what `segalloc` returns and `segfree` takes, and `ve` is written with
it ([ve](userland.md#ve)). In the medium and large models a function
may be said to be `__far`, which every function there is. `cc -mm`
gives the preprocessor `__MEDIUM__`, as the cross compiler has it, for
what has to know the size of a pointer to a function (the C library's
`sigaction`).

In the large model a pointer that a `*` declares is far unless it is
said to be `__near`, and so is the address of anything: the first pass
does that, and the second is left to reach the program's data, which
is the stack's segment, through SS. DS belongs to no one there: it is
loaded with the segment of whatever is pointed at, by the library's
assembly as by anything else, and a function need not put it back
([the large model as it is built](memory-model.md#the-large-model-as-it-is-built)).
So a variable is `[ss:_x]`, and where it is reached through BX, SI or
DI, as an array is, `[ss:bx+_x]`; one in the frame is `[bp-4]` as
always; and its address is made far with SS. The second pass is given
the address of a variable as a near one made far, and takes what is
added to it, and what it points at, back to the near address, so that
`a[i]` and `s.m` are still operands of one instruction. `cc -ml` gives
the preprocessor `__LARGE__`, which the headers know the model by, and
`NULL` as `0L`, so that it is four bytes where it is passed to a
function with no prototype; and the assembler `FARDATA` as well as
`FAR`, as the library's assembly is assembled. What the cross compiler
has and this one has not: a variable of more than 32K is in the
program's own data unless it is said to be `__far`, where the cross
compiler gives one a segment of its own; and there is no `__huge`.

## Far variables, and pointers into a segment

In any model a variable that is static or external may be declared
`__far`, before its name: `struct pseg __far pseg[NPROC * NSEG];`. It
is not in the program's data but in a segment of the file's own,
`FILE_FAR` (`FAR_DATA` in the small model, where every file's is one),
of class `FAR_DATA`, which the linker puts with others as they fit
([below](#the-linker)). The first pass makes the variable what its
address, made far, points at; the second makes that address from the
name, with `seg _x` for the segment (`XTOF`), and what is added to it,
as an index or a member's offset, is added to the near address. A far
variable is defined where it has no value, not common.

Two more kinds of pointer are for the kernel, and are as near as an
ordinary one:

- `__ss` before a `*`: a pointer to what is in SS. That is the frame,
  where SS is not DS.
- `__based(NAME)` before a `*`: a pointer into the segment `NAME`;
  before a variable's name, a variable in that segment. A file has one
  such segment.

What either points at is reached by making it far, with SS or `NAME`
for its segment (`STOF`, `BTOF`), as a far pointer's is; the address
of what one points at is of the same kind.

`-nd=NAME` to `tools/ncc` (`c0 -S`, `c1 -NNAME`) is for code whose DS
is not SS, as the kernel's network's is ([the
network](network.md)). The data is `NAME_DATA` and `NAME_BSS`, of the
group `NAME_GROUP`, which is not the program's data segment; what has
no value is defined there, not common. The address of what is in the
frame is an `__ss` pointer, a structure is copied with the far
addresses of both, and what is on the stack is reached through SS. A
structure of more than two words is not returned there, since the
place for it may be in either segment. An `__ss` pointer made an
ordinary one, by a cast, an assignment or an argument, or passed to a
function with no prototype, is said with the function it is in
("pointer into SS made near, in FUNC"), since the code shows nothing
of it ([netcheck](network.md#what-the-compiler-does-not-see-netcheck)).

These are what the cross compiler has as `__far` on a variable,
`__based(__segname(...))` and `-nd -zu`; the kernel's headers say each
in the way of the compiler that is compiling them
(`i286/machparam.h`).

## In-line functions

A function declared `__inline` is put where it is called, in place of
the call. Its body is assembly and nothing else, an `asm()` to a line:

```c
__inline u_short
htons(u_short x)
{
	asm("xchg al,ah");
}
```

The first argument is in AX, or in DX:AX if it is a long, as an
expression's value always is to the second pass; any others are
pushed as for a call, the last first, and taken off after. The body
leaves its value where a function does, and may change what a function
may: AX, BX, CX, DX, ES and the flags. A line that begins with a name
and a colon is a label and any other an instruction. `%%NAME` is a
label of that one place's own, as in a macro of NASM's; a name that
begins with `_` is a C name, of which the assembler is told as of any
other the file uses. The first pass sends the body to the second
(`INLDEF`), which keeps it with the file's external names; the
function itself is not compiled and has no address, and a header may
define it in every file that includes it. A body is 510 characters at
most, with eight labels. What `c2` does to the code around it it does
to the body too: it knows `jz` and the other names of the conditional
jumps, and takes `[ss:_x]` and `[_x]` for what may be the same place.

The kernel's spl routines and byte swaps are such (`i286/machparam.h`),
where the cross compiler has the same as `#pragma aux`.

`ld -mm` looks in the `m` directories for `-l`, and `ld -ml` in the
`l` ones. The libraries on the system are `libc`, `libm`,
`libtermcap`, `libcurses`, `libutil`, `libdbm`, `libln`, `liby` and
`libwrap`, for each model.

## The assembler

`as` reads the syntax of NASM, as much of it as the compiler writes and
the C library's own assembly uses. Those were written for NASM before
the system had an assembler, and either assembles them still, which is
how the assembler is tested: `tests/as` gives both the same files and
compares what they make.

```
as [-o object] [-b] [-Dname[=text]] [-Idir] [-ms|-mm|-ml] file.s
```

What it has:

- Every instruction of the 8086, the 186 and the 286, the protected
  mode's among them, and the 287's.
- Operands as NASM writes them: `word [bp-4]`, `byte [es:bx+_tab+2]`,
  `far [bx]`, `seg _name`, `st1`.
- `db`, `dw`, `dd`, `resb`, `resw`, `resd`, `times`, `align`,
  `alignb`, `equ`, `global`, `extern` (with NASM's `:wrt GROUP`, which
  means nothing here), `common`, `segment`, `group`.
- Labels local to the last label without a dot (`.loop`), `$` and
  `$$`, and a far jump or call to a segment that is a number,
  `jmp 0:go`.
- Values with `+ - * / % << >> & ^ |`, and compared with `= == != <>
  < <= > >=`, `&&`, `||` and `!`.
- `%include`; `%define`, with arguments in brackets or without, and
  `%assign`; `%if` and `%elif` with a value, `%ifdef`, `%ifndef`,
  `%else`, `%endif`; `%rep` and `%endrep`; `%+`, which makes one word
  of two; and `%macro` with `%1` to `%9`, one macro using another.

The text, the data and the bss are three of a file's segments: every
`segment` of class `CODE` is the text, of class `BSS` the bss, and of
`DATA` the data, in the order the file has them. A segment of any
other class, as `FAR_DATA`, is a named segment of the object's own
([below](#object-files)), as the kernel's network's data are. `bits`
and `cpu` are read and mean nothing. A group's name stands for the
selector of the segment the linker makes of it, and so does a named
segment's; `DGROUP`'s is the program's data, as `seg` of anything in it
is. The large model's start-up code writes `DGROUP` so.

A word alone on a line, with no colon, that is not an instruction the
assembler knows is an error, where NASM takes it for a label: that is
how an instruction left out of the assembler's table is found.

A jump to a label takes the short form if that reaches and the long one
if not; a conditional jump out of reach becomes the opposite condition
round a jump, which the 286 needs. The compiler leaves that to the
assembler, as the PDP-11's does. It is worked out between the two
passes, starting from every jump short and lengthening those that do
not reach until none is left, so the code is as short as it can be.

What it remembers between the passes has, on the 286, what is left of
a small-model program's 64K when its own variables, its stack and its
files' buffers have some 27K. A symbol there is 14 bytes and its name;
the compiler's labels, `L12` and `LL12`, of which a file has hundreds,
are kept by their numbers and have no name; and a jump is 12 bytes,
1024 of them to a segment of their own (`segalloc`), outside the 64K.
The file of the system that asks most of it, the scanner `lex` makes
for `style`, with 1,691 labels, takes the break to 55,296 bytes of the
63,488 it can have.

With `-b` it writes an image, the bytes and nothing else, as NASM's
`-f bin` does: a boot block. Its addresses are those of where it is to
be, which `org` says, and it is all text, with nothing from another
file and no selector. A `times` whose count is reckoned from `$`, as
`times 510-($-$$) db 0` is, is taken to fill to a place, which stays
where it is when the jumps before it are made long. The boot blocks
are each one file, and so `as` makes them, as NASM did; `/boot`, which
is linked, `ld -b` makes ([below](#the-linker)).

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
| segment entries | text, data, bss, and any named segments; each with the number of its relocation entries |
| text, data, named segments | each made up to an even length; the bss has nothing in the file, and a named segment has up to the last byte that is put there |
| relocation | each segment's in turn; 6 bytes an entry, in the order of the addresses |
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
and its value is where in that. (`n_ovly` is 0 for those in an object;
in a program it is the number of the symbol's segment.) An undefined
symbol with a value is common storage of that size, as on the PDP-11.

The segments after the three are named: each is data that is not in
the program's data segment, as a far variable is. A symbol of type
`N_SEGNAME`, whose `n_ovly` is the segment's number, names it, and one
of `N_GRPNAME` with the same `n_ovly` names its group if it has one;
those are first in the symbol table. A symbol in a named segment is
`N_DATA`, with the segment's number in `n_ovly`, and relocation names
the segment by its number as it does the three. `nm` shows the two
kinds of name as `s` and `g`.

A library is `ar`'s archive with `ranlib`'s table of contents, both as
upstream has them.

## The linker

```
ld [-o out] [-ms|-mm|-ml] [-b] [-s] [-x] [-stack n] [-packcode n] [-gdt n]
   [-e sym] [-u sym] [-L dir] [-M] [-t] file ... -lx ...
```

It reads its arguments twice, as upstream's does: once for the symbols
and the sizes, taking from each library the members that define what is
still undefined, and once to copy the text and the data, relocating
them. A library is searched until nothing more is wanted from it, and
libraries are searched in the order given.

A program's data, with the bss and then the common storage after it, is
the segment after its text. In the small model the text is one segment. In the
medium model each file's text is whole in one segment and files share a
segment, in the order they are linked, until the next would not fit;
`-packcode n` makes the segments smaller than 64K. The large model's
text is the medium's, and its data is the one segment too: what its
far pointers point at beyond that, the program is given as it runs. A near call or jump
from one file to another that has landed in a different segment is an
error: the library's system call stubs reach their common error path
with a far jump for that reason.

The named segments come after the data. What every file has of a named
segment is put together, in the order they are linked. The segments of
a group are one segment of the program, in the order they were come
upon; the others share segments, as files share the text, until the
next would not fit. A program may have 31 segments in all.

A segment's selector follows from its number, so the linker fills
selectors in and the program is loaded as it is. With `-gdt n` the
selectors are the kernel's, of the global descriptor table from slot
`n` on ([booting](boot.md)), and not a process's own. `-stack n` sets the
stack the header asks for (8192 without). The entry point is the symbol
`..start`, which the start-up code defines, or `-e`. `_etext`, `_edata`
and `_end` are defined if something asks for them.

With `-b` the program is an image, as `/boot` is: its text, its data
and then its bss and common storage in one segment of the small model,
and in the file the bytes of the text and the data and nothing else,
as the cross linker's "DOS .COM" has it. Its addresses are counted from
its first byte, which is its entry point, with CS, DS and SS all the
one segment; nothing in it may be a selector. It is the PDP-11's
program whose text and data are not apart, without the header.

The PDP-11's `-i`, `-n` and `-z` are taken and ignored; its overlays
(`-Z`, `-L`, `-Y`) and `-r` are not there.

## Where the system is built

The same sources are compiled for the machine the system is built on
(`make -C usr/src/lib/ccom/i286 host`, which makes `cpp`, `c0`, `c1`,
`c2`, `as` and `ld` in `obj/host/ccom`):

| | |
|---|---|
| `tools/ncc` | compiles one file: `cpp`, given what `cc` gives `/lib/cpp` on the system, then `c0`, `c1`, `c2` with `-O`, and `as` |
| `tools/cc -ccom` | `cc` with those in place of the cross compiler, `as` for assembly, and `ld` |
| `tools/ar.py` | makes a library with its table of contents, as `ar` and `ranlib` do on the system |
| `make -C usr/src/lib/libc/i286 CCOM=1` | the C library that way, in `obj/nlib` |
| `make -C usr/src/i286 CCOM=1` | every program that way, in `obj/nbin` |
| `make -C usr/src/sys/i286stand CCOM=1` | the boot blocks by `as -b` and `/boot` by `tools/ncc` and `ld -b`, in `obj/nstand` |

The libraries on the root file system are those of `obj/nlib` whichever
compiler made the programs, since the cross compiler's objects are of
another format. `make root` makes them first.

The headers the objects are written with have a `long` of 32 bits and a
pointer of 16; the host's copies of them are made with the types put
right (`obj/host/ccom/include`). The host has to keep a number's low
byte first. `cpp` there is given the tree's `usr/include` for the
system's headers, a type of 32 bits for what `#if` reckons in, which is
a `long` on the 286, and its buffers and tables as large as they are
there (`lib/cpp/i286`), so that it writes what `/lib/cpp` writes.

## The system makes its own compiler

`tests/ccom/self.sh` boots the system from a disk that has, on a file
system of their own, the sources of the eleven programs above that are
C: `cc`, `cpp`, `c0`, `c1`, `c2`, `as`, `ld`, `ar`, `ranlib`, `nm` and
`size` (`make -C usr/src/i286 ccprogs` lists what each is made of). The
system compiles, assembles and links each one, and each must be, to the
byte, what `tools/cc -ccom` makes of it where the system is built. Then
it puts them where the ones it booted with were, and does it again with
those: the second time the compiler is the one the system made.

All 22 are the same. Where the system is built the files go through
its own preprocessor too, built for that machine, so that what is
compared is the preprocessor's work as well.

Two things came of the first time it was tried, neither of which a
program of two small files had shown ([what the programs
needed](userland.md#the-compiler)): `/lib/cpp` as the cross compiler
compiled it knew no macro that had no arguments, and the assembler's
objects had their symbols in another order on the 286 than where the
system is built.

## The kernel

`make -C usr/src/sys/PC CCOM=1` makes the kernel with the system's own
tools as they are built where the system is built, in `obj/nPC`: each
C file by `tools/ncc -k -O -mm`, with the kernel's own `-I`
directories in their order, and the network's with `-nd=NET` besides; the
kernel's assembly by `as`; and the whole by `ld -gdt 16`. Its segments
are the cross compiler's kernel's, in the same order:

| | this kernel | the cross compiler's |
|---|---|---|
| code | 185,040 bytes in 3 segments | 165,870 in 3 |
| data (DGROUP) | 63,604 | 63,544 |
| the network's (NET_GROUP) | 58,806 | 58,802 |
| the character blocks (CLIST) and the far variables | 46,352 and 27,704 | 32,768, 13,584 and 27,704 |

The spl routines and the byte swaps of the network (`htons` and the
rest) are in line by both compilers ([above](#in-line-functions)), and
`tools/netcheck.py` reads its objects as it reads the cross compiler's,
with what the compiler said of the network's pointers into SS
([the network](network.md#what-the-compiler-does-not-see-netcheck)).
With `NATIVE_KERNEL=1`, `tests/system` and `tests/network` boot the
system with this kernel.

What the kernel took of the compiler was far variables, the two kinds
of based pointer and `-nd` ([above](#far-variables-and-pointers-into-a-segment));
of the assembler `%if`, `%assign`, `%rep`, `%+`, a `%define` with
arguments, a macro that uses another, `alignb` and `fsetpm`, and named
segments in its objects; and of the linker those segments and `-gdt`.

## What is not there

- The system making its kernel itself, on the 286: its `make` and
  `cc` have not been given the kernel's Makefile and flags.
- Profiling (`cc -p`, `mcrt0.o`, `prof`), `adb` and `ld -r`.
- In the large model, a variable of more than 32K in a segment of its
  own, which the cross compiler makes, and `__huge` pointers.
