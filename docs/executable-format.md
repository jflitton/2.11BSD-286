# Executable format

The format of programs and of the kernel image. `tools/ne2aout.py` writes
it. The kernel's `execve` reads it (`sys/h/exec.h`,
`sys/sys/kern_exec.c`), and `tools/mkfd.py` does, to make a diskette the
boot sector can load ([booting](boot.md)). Nothing outside this tree
depends on it, so the layout can still change.

## How an executable is made

```sh
. tools/env.sh
tools/cc286 -ml -o a.o a.c                     # one object per source file
wlink op q format os2 name prog.ne \
      op nodefaultlibs,start=_start,map=prog.map file a.o,b.o
tools/ne2aout.py --model l --map prog.map prog.ne prog
tools/ne2aout.py --dump prog
```

wlink writes a segmented "new executable", the format of OS/2 1.x. The
converter reads its segment table and fixups and writes the system's own
format. A linker of our own can replace both steps later without the
kernel noticing.

Why not load wlink's output directly, as ELKS does: it has no a.out symbol
table, and 21 programs in the tree (`ps`, `pstat`, `netstat`, `dmesg` and
others) find kernel variables with `nlist`.

## Selectors are fixed when the file is made

A far pointer or far call in a 286 program holds a selector. Most systems
leave those to be filled in when the program is loaded. Here every process
has its own local descriptor table, so the numbering can be the same for
every program:

| | selector of segment N, counting from 1 |
|---|---|
| a program | `(N << 3) \| 7`: slot N of the process's LDT, ring 3 |
| the kernel | `(FIRST + N - 1) << 3`: GDT slots from FIRST, ring 0 (`--gdt FIRST`; the kernel's FIRST is 16, `GDT_KERNEL` in `sys/i286/seg.h`) |
| the real-mode test harness | `BASE + (N - 1) * 0x1000`: a real-mode segment (`--real BASE`) |

So segment 1 of any program is selector 0x0F, segment 2 is 0x17, and so
on. The converter writes those values into the image. Loading a program is
then filling in a descriptor table from the segment table: nothing in the
image is modified, and a code segment is identical for every process
running it, in memory and on the swap device.

## Layout

All fields little-endian.

Header, 24 bytes:

| offset | size | |
|--------|------|---|
| 0 | 2 | magic, 0x0286 |
| 2 | 2 | memory model: 1 small, 2 medium, 3 large |
| 4 | 2 | number of segments |
| 6 | 2 | entry point: segment |
| 8 | 2 | entry point: offset |
| 10 | 2 | the data segment that DS and SS start out as |
| 12 | 2 | stack size: the kernel adds this much to that segment, after its data, and room for the arguments and environment |
| 14 | 2 | reserved, zero |
| 16 | 4 | size of the symbol table, bytes |
| 20 | 4 | size of the string table, bytes |

Then one 12-byte entry per segment:

| offset | size | |
|--------|------|---|
| 0 | 2 | 1 code, 2 data |
| 2 | 2 | reserved, zero |
| 4 | 4 | bytes present in the file |
| 8 | 4 | bytes in memory; the excess over the file is zero-filled |

The sizes are 32 bits because a full segment is 65,536 bytes, one more
than 16 bits hold. The converter leaves the zeros at the end of a data
segment out of the file, so a far array that starts out all zero takes
no room in it.

Then the contents of each segment in order, each padded to an even length;
then the symbol table; then the string table.

The 2.11BSD header has a table of up to 15 overlay sizes after it
(`struct ovlhdr`). The segment table is that idea made general.

## Symbols

Entries are the tree's `struct nlist` as it stands, 8 bytes: a 4-byte
offset into the string table, a type byte, the overlay number, and a
16-bit value. The overlay number field carries the segment number, and the
value is the offset within that segment. The string table begins with its
own length in 4 bytes, as in 2.11BSD.

## Not settled

- Where the stack sits in a small-model data segment. A 286 segment has no
  holes, so the PDP-11's stack at the top of the address space with free
  space below it is not available. The kernel puts a stack of fixed size
  after the data, with the break above it. Putting it at the bottom of the
  segment instead would turn overflow into a fault instead of silent
  damage to the data below it.
- How a medium- or large-model program's code is divided. The linker
  packs it into segments of nearly 64K, in the order the files are
  linked, and `tools/cc -packcode N` makes them smaller. Code that calls
  itself often would be better in one segment, where calls could be
  near.
