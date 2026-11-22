# Booting, and the kernel's first instructions

How the kernel gets into memory and into protected mode: from a
diskette, or from the hard disk that has the root file system. Running
under `tests/pm` and `tests/kernel`. The boot programs are in
`sys/i286stand`; `make -C usr/src/sys/i286stand` puts them in
`obj/nstand`, made by the system's own assembler, compiler and linker,
and with `TOOLS=cross` in `obj/stand`, made by NASM and the cross compiler
([the compiler](toolchain.md#the-assembler)): the boot sectors are the
same to the byte, and `/boot` is another compiler's.

## From the boot program to the kernel

A boot program loads each segment of the kernel
([executable format](executable-format.md)) somewhere below 640K, clears
the part of each that the file does not fill, and jumps to the entry point
in real mode with:

| | |
|---|---|
| CS:IP | the entry point, where its segment was loaded |
| DS | where the kernel's data segment was loaded |
| ES:SI | a table of where every segment was loaded (below) |
| DL | the BIOS's number for the drive booted from |
| SS:SP | a small stack of the boot program's; interrupts on |

The BIOS is still usable at that point. The kernel uses it to ask how much
memory there is and, if the table names one, to load a RAM disk image.
With one the root file system is that image, and without, a hard disk
([disks](disks.md)).

The table, `struct bootinfo` in `sys/i286/boot.h`, all 16-bit words:

| offset | |
|--------|---|
| 0 | magic, 0x0286 |
| 2 | number of segments, at most 16 |
| 4 | entry point: segment, counting from 1 |
| 6 | entry point: offset |
| 8 | the data segment, counting from 1 |
| 10 | the BIOS's drive number |
| 12 | first sector on that drive of a RAM disk image, |
| 14 | and its size in sectors; 0 if there is none |
| 16 | the device the kernel was loaded from, as the kernel numbers devices; 0 if the boot program cannot say |
| 18 | then for each segment, 8 bytes: |
| +0 | where it was loaded, as a real-mode segment |
| +2 | sectors read from the diskette |
| +4 | its size in memory, less one |
| +6 | 1 code, 2 data |

### The diskette boot sector

`sys/i286stand/fdboot.s` is one sector that holds that table at offset 3.
`tools/mkfd.py` writes a 1.44 MB image: the boot sector, then each segment
starting on a sector, then a file system image if it was given one
(`--rd`), and fills the table in. Segments go one after
another from 1000:0, each at a multiple of 512 bytes. The multiple of 512
is for the diskette controller: its DMA cannot cross a 64K boundary within
one transfer, and whole sectors read to such addresses never do.

The same sector starts the real-mode test programs of `tests/harness`;
`mkfd.py --spread` places their segments 64K apart, where
`ne2aout.py --real` assumed they would be.

### From a hard disk

Three programs, each loaded by the one before:

| | where it is | what it does |
|---|---|---|
| `mbr.s` | the disk's first sector, if the disk is shared through a partition table | loads the first sector of the part marked as the one to boot |
| `hdboot.s` | the first sector of the system's part, which is the first sector of the file system in partition `a` | finds `/boot` in that file system and loads it |
| `boot.c`, `srt0.s` | the file `/boot` | asks what to load, loads the kernel from the file system, makes the table and starts it |

The BIOS starts the first, or on a disk that is all the system's, the
second. `mbr.s` does what every PC system expects of that sector, so
another system's will do in its place, and it will start another
system's part.

`hdboot.s` has 446 bytes, the sector less the place of a partition
table, which must stay empty on a disk that is all the system's
([disks](disks.md)). In them it decides where the system's part is, as
the kernel does, looks through the root directory for `boot`, and reads
the file through its four direct blocks and its first indirect block.
It reads a sector at a time through the BIOS, three tries each.
`tools/mkdisk.py -b` and `-m` put the two sectors on a disk image.

### From a diskette with a file system

`flboot.s` is `hdboot.s` for a diskette, which is `/mdec/fluboot` on the
system: the first sector of a diskette that is all one file system, with
`/boot` and a kernel in its root directory. It has no partition table to
look in. The BIOS says what the drive can hold and not what the diskette
in it does, so the boot block takes the sectors on a track to be the
most that the first track has one of, 18, 15 or 9, and `/boot` does the
same. `/boot` then asks as it does from a hard disk, and what it shows
is `wd(0,0)`: the kernel it loads from the diskette takes the first hard
disk's first partition as its root unless another device is typed.
`tests/floppy` starts the system so from diskettes of 1.44 MB and
1.2 MB that the system made.

Memory while they run:

| from | |
|------|---|
| 0x00600 | the boot block, which moves itself there, and above it two blocks it reads through |
| 0x01000 | `/boot`: its code and data as one segment, 0100:0000 |
| below 0x10000 | the stack, the boot block's and then `/boot`'s |
| 0x10000 | the kernel's segments, one after another, each at a multiple of 512 bytes |

`/boot` is a file of code and data exactly as they are in memory,
linked as one segment (`wlink`'s "DOS .COM", or `ld -b`). It must fit
with what the file does not hold in 56K; it takes 8K now. When the kernel starts it gives all memory from 4K
to its own first segment to processes, so nothing of the boot programs
is kept.

`/boot` writes to the display and reads the keyboard through the BIOS,
and does the same with the first serial port if there is one, set as
the kernel sets it:

```
Boot from sd(0,0)
:
sd(0,0)unix: 64322+23002+57232+41288
```

- The first line is the device it takes the disk it was booted from to
  be: the driver, the drive and the partition, as the kernel counts.
- At the colon a file can be named, by default `unix`: a path from the
  root of the file system. Before the name can come a device in the same
  form, `wd(0,0)unix.old`. With nothing typed for five seconds the
  default is loaded. A line that cannot be done is answered with why and
  asked for again, with no limit of time.
- The last line is the file being loaded and the size in memory of each
  of its segments as it goes in. Then the kernel starts.

`/boot` reads only the disk it came from, through the BIOS. The device
is what the kernel is told its root is; the partition is also which
file system the file is read from; the drive number is what the kernel
is told and no more.

How the device shown is arrived at, since the BIOS says nothing of what
its drives are connected to:

- The driver is `sd` if the disk's label says it is a SCSI disk and
  `wd` if not. `tools/mkdisk.py -t` sets that, and the label the kernel
  makes up for a disk without one has it right.
- The drive is the BIOS's number for the disk, less 80h.

That is right where every hard disk the BIOS has is of one kind. Where
it is not, as with a SCSI disk that the BIOS numbers after an IDE one,
the drive number is wrong and the device has to be typed each time;
nothing remembers it.

Limits:

- The BIOS reads the first 1024 cylinders, as it counts them. `/boot`
  and the kernel must be within them, and so must the start of the
  system's part.
- A kernel file is read through two levels of indirect blocks, which is
  64 MB. The kernel is 105K, within the first level, so the second has
  never run.
- A root on a partition other than `a` is handed on to the kernel, which
  takes the swap area to be the partition whose number is the root's
  with its low bit set, as 2.11BSD always has. That is right for `a`
  only, and nothing else has been tried.
- Nothing is passed to the kernel but the device: no single-user or
  other flags.

## The kernel's start (`sys/i286/mch_start.s`)

1. Copy the boot program's table into the kernel's data. From here on
   the boot program's memory is not needed, but its stack is until
   step 4.
2. Ask the BIOS for the size of memory below 640K (interrupt 12h) and above
   1M (interrupt 15h, function 88h).
3. If the table names a RAM disk image, read it from the boot drive a
   track at a time into a buffer at 512K, and have the BIOS move each
   piece to 1M and up (interrupt 15h, function 87h): real mode cannot
   reach that memory itself. Then stop the diskette drive's motor, which
   the BIOS would have done from its clock interrupt.
4. Turn interrupts off, until the kernel has its clock running, and move
   to the kernel stack of the first slot ([processes](processes.md)),
   which becomes the swapper's.
5. Enable address line 20. A PS/2 does it with bit 1 of port 92h, an AT
   through the keyboard controller; each is tried in turn until memory at
   1M stops being the same memory as at 0. The test writes a word 7DFEh
   past 1M, where the end of the boot sector is with the line off, and
   puts back what was there: with the line on that word is the RAM
   disk's.
6. Write a descriptor for each kernel segment into the global descriptor
   table, from the table of where they were loaded: base, limit, and ring 0
   code or data.
7. Start the two interrupt controllers again with their vectors at 32 to
   47 and every request masked. The BIOS has the first at 8 to 15, which
   on a 286 are exceptions.
8. Load the descriptor table registers, set the protected-mode bit, and
   make a far jump, which loads CS with a selector for the first time.
9. Load DS, ES and SS with the data segment's selector, load the task
   register, put the 287 into protected mode, and call `main`.

The kernel's `main` does not return: it makes the first process and
becomes the swapper. In the tests that put their own `main` under this
start-up, `main` returns having made a frame at the top of the stack, and
the start-up resumes that frame.

Everything up to step 8 is written with near jumps and calls only. Far
addresses inside the kernel are protected-mode selectors, written in when
it was linked, and mean nothing in real mode.

## The global descriptor table (`sys/i286/seg.h`)

| slot | selector | |
|------|----------|---|
| 0 | | empty, as the processor requires |
| 1 | 0x08 | the task state segment |
| 2 | 0x10 | the running process's local descriptor table |
| 3, 4 | 0x18, 0x20 | pointed at whatever memory is being copied or cleared by physical address |
| 5 | 0x28 | pointed at the memory a disk driver is moving sectors to or from |
| 6 to 15 | | kept for later |
| 16 to 31 | 0x80 on | the kernel's segments, in the executable's order |
| 32 to 63 | 0x100 on | handed out as the kernel starts: so far, one for each 64K of the buffer pool's data |

The kernel's segments have fixed slots because `ne2aout.py --gdt 16` writes
their selectors into the kernel when it is linked (or `ld -gdt 16`, for the
kernel the system's own tools make). A descriptor's limit is
the segment's exact size, so the kernel faults if it reads or writes past
the end of its data or jumps past the end of a code segment.

There is one task state segment. The kernel never switches tasks with it;
the processor reads the kernel stack pointer from it when a process traps
into the kernel.

## Exceptions and interrupts (`sys/i286/mch_trap.s`)

The interrupt descriptor table has 49 gates: the processor's exceptions at
0 to 16, the 16 interrupt requests at 32 to 47, and the system call at 48.
It is assembled as data, complete, so no code runs to fill it in. Every
gate but the system call's turns interrupts off.

Each vector pushes its number, and an error code of 0 if the processor
pushed none, and joins a common path that saves the registers, loads DS
and ES with the kernel's data segment and calls `trap(frame)` for an
exception, `syscall(frame)` for the system call, or `intr(frame)` for an
interrupt request, on a stack kept for interrupts
([processes](processes.md)). The frame,
`struct trapframe` in `sys/i286/frame.h`, is on the kernel stack. Because
that stack is inside the kernel's data segment, `frame` is an ordinary
near pointer, and whatever `trap` changes in it is what the interrupted
code resumes with.

## Not done yet

- 287 errors, which reach an AT-compatible machine as interrupt request 13
  and not as exception 16.
- None of this has run on a real machine. The emulator's BIOS is the PS/2
  Model 30 286's own, but its disk drive is not a real one: the kernel's
  reads of the RAM disk image retry three times and have never had to.
  The same goes for the hard disk boot programs' retries, and the
  adapter's BIOS they read through was a 1542CF's.
