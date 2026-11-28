# Disks and tapes

What a disk looks like to the kernel on a PC, the two drivers for
real disks, and the one for SCSI tapes. They run in the emulator under `tests/kernel`, which boots
the kernel with its root file system and swap area on a hard disk, and
from the disk itself ([booting](boot.md)).

## Devices

| block | character | | driver |
|-------|-----------|---|--------|
| 0 | 7 | disks in memory: 0 the image the boot program loaded, 1 a swap area | `sys/i286/rd.c` |
| 1 | 8 | hard disks on the AT disk controller | `sys/i286at/wd.c` |
| 2 | 9 | SCSI disks on an Adaptec AHA-1540 or 1542 | `sys/i286at/sd.c`, `aha.c` |
| 3 | 11 | diskettes on the AT diskette controller | `sys/i286at/fl.c` |
| | 13 | SCSI tapes on the same Adaptec as the SCSI disks | `sys/i286at/st.c`, `aha.c` |

A hard disk's minor number is the drive in bits 3 and up and the
partition, `a` to `h`, in the low three: `wd0a` is 0, `wd1c` is 10. A
diskette drive's is the drive: `fl1` is 1.
Through the character device a disk moves whole sectors only: a count
that is not a multiple of 512 is refused (`EINVAL`). Upstream's programs
that read a file system directly read its superblock as the size of its
structure, as the PDP-11's controllers let them; here they read the 1K
block it is in.

A device's inode holds its number where a file's holds the addresses of
its blocks, and has no size: truncating one does nothing (`itrunc`), as
in 4.4BSD. Upstream's kernel lengthened it as it would a file, which
`dd` does to its output with `ftruncate` when it is given `seek` and not
`conv=notrunc`, and the next open of the device with `O_TRUNC`, as
`mkfs` opens a disk, then freed the number as though it were a block.
The kernel said "bad block", the inode was left with the number 0, 0,
the console's, and `newfs` wrote its file system on the screen.
`tests/programs` has a raw disk keep its number after both.
What the two drivers do alike, which is everything in this section and
the next, is in `sys/i286at/pcdisk.c`.

## Whose disk it is

A PC's hard disk can be shared between systems by the table at the end
of its first sector, which gives each of up to four systems a run of
sectors and says by a number whose each run is. On the first open of a
drive that sector is read.

- If the table has an entry with 2.11BSD's number, the system keeps to
  that run of sectors.
- If there is no table, or nothing in it for 2.11BSD, the whole disk is
  the system's.

Either way the label (`sys/h/disklabel.h`) is the second sector of the
system's part, and the partitions in the label are counted from the
start of that part. The code that reads, checks and writes labels,
which every 2.11BSD disk driver shares, is therefore unchanged: only
the driver knows where on the drive the system's part begins, and adds
it to every sector number.

A label with a partition that runs past the end of the system's part is
refused, as a damaged one is: a partition over another system's sectors
would let a file system be written on them. So is such a label given to
the kernel to use or to write (`DIOCSDINFO`, `DIOCWDINFO`), by
`disklabel`: the drivers' `ioctl` is `pcdkioctl`, which refuses it with
`ENOSPC` before the code all 2.11BSD's drivers share (`ioctldisklabel`)
sees it. The size of the system's part, which that needs, is kept from
the drive's first open (`pg_part`).

A drive with no label, or one refused, is given a single partition `a`
that covers the system's part. That is enough to read it, not to boot
from it: the swap area is partition `b` of the root's drive and must be
marked as swap in the label.

Three things to know:

- The number, 0xB9 (`PCP_2BSD`), is not registered anywhere, because
  nobody registers them. It was chosen as one not known to be in use and
  that has not been checked against the lists people keep.
- A disk that is all the system's must still leave the table's place in
  its first sector empty, or a boot program there could be read as a
  table. The system's boot block does.
- There is no device for the whole of a shared disk, so the table cannot
  be read or written from inside the system.

## Where the root is

If the boot program loaded a RAM disk image, or named one for the
kernel to load, that is the root, and the swap area is the second disk
in memory. If it did not, the root is the
device the boot program says it loaded the kernel from, and the swap
area is partition `b` of the same drive.

`/boot`, which loads the kernel from a hard disk, says: [booting](boot.md)
has how it decides, which begins with whether the disk's label calls it
a SCSI disk. The diskette's boot sector does not say, and then the root
is `diskroot` (`sys/i286/conf.c`): the first IDE drive's partition `a`
unless the kernel file is changed with `tools/kpatch.py`, where 512
makes it the first SCSI disk's. A crash dump goes to that swap area
too, if its driver can write one ([crash dumps](#crash-dumps)).

## The AT disk controller (`sys/i286at/wd.c`)

The controller of the PC/AT was a WD1003, and the drives called IDE
answer at its ports and take its commands. The driver is for those: one
controller at 0x1F0, interrupt request 14, two drives.

- A drive is looked for when it is first opened. It must answer the
  identify command, which gives its size and shape; the ST-506 drives of
  a real WD1003 cannot, and are not supported.
- Sectors are named by number if the drive says it can, and by cylinder,
  head and sector if not.
- A drive that can move several sectors at each interrupt is told to, up
  to 16 (`wdmulti`). A file system block is then one command and one
  interrupt.
- A transfer is one command however long it is. The queue is sorted by
  cylinder (`disksort`), reads before writes.
- A transfer that fails is tried four times in all, counting again from
  whenever some of it succeeds, and then reported.

The data moves between the controller's port and memory with one string
instruction, in `pinsw` and `poutsw` (`sys/i286/mch_copy.s`). Those take
the memory's physical address and point a descriptor kept for the
purpose at it. A buffer of the pool, a process's memory during raw I/O
and a segment being swapped are all the same to the driver, and none is
copied on the way.

## The Adaptec SCSI adapter (`sys/i286at/aha.c`, `sd.c`)

For the AHA-1540 and 1542, models B, C, CF and CP. `aha.c` is the
adapter, `sd.c` the disks on it and `st.c` the tapes
([below](#scsi-tapes-sysi286atstc)).

- The adapter is looked for at each of the six I/O addresses it can be
  set to. Its interrupt request and DMA channel are read from it, so
  nothing about it is configured in the kernel.
- A C, CF or CP whose BIOS is set for disks over 1 GB keeps its
  mailboxes locked against drivers that do not know of that, and is
  unlocked.
- The adapter is a master of the AT bus: it moves the data itself,
  between the SCSI bus and any memory it is given the physical address
  of. The driver gives it the address of the buffer, or of the process's
  memory, or of the segment being swapped, and nothing is copied. The
  DMA channel is only how it asks for the bus.
- The SCSI bus is looked over when a disk is first opened. Each disk
  found, in order of SCSI id, is the next drive: `sd0`, `sd1`, up to
  four. Only unit 0 of a target, and only 512-byte sectors.
- A disk has no cylinders or heads of its own. Its label is given the
  ones the adapter's BIOS makes up (64 heads and 32 sectors, or 255 and
  63 on a disk over 1 GB when the adapter is set so), because the
  partition table and whatever boots the machine count in those.
- How fast a disk is comes to how the data is passed, on two buses,
  and the driver says both when it finds them, as the adapter reports
  them, a line for the adapter and one for each disk:

  ```
  aha: AHA-1542CF at 0x334 irq 10 dma 5 id 7, bus on 11 off 4 us, 5.0 MB/s
  sd0: 86Box 86B_HD00 3.50, 64 MB, id 0, async
  ```

  `bus on` and `off` are how long it holds the AT bus and lets it go,
  and the speed after them is its own on that bus, two figures if it
  reads and writes at different ones. A disk that agreed with the
  adapter to pass data synchronously has `sync 5.0 MB/s`, the rate they
  agreed, where the emulator's has `async`.
- The adapter holds the AT bus 11 microseconds at a time and lets it go
  for 4, which is how it is made. Other drivers for it give it 7 and 5,
  so that a diskette drive's DMA is not kept waiting; the diskette
  driver has not been run beside it. On a real machine, with a disk that had
  agreed 5 MB/s and the AT bus at 5 MB/s, a read of the disk itself 32K
  at a time:

  | held, let go | MB/s |
  |---|---|
  | 7, 5 (taken as 4: the adapter counts the second in fours) | 2.03 |
  | 11, 4 | 2.38 |
  | 15, 1, the most and the least it can | 2.77 |

  That is 4.7 ms for each command and the data at 5 MB/s for as much
  of the time as the adapter has the bus, to within 2%. Most of the
  4.7 ms was that disk's own, which was slow to answer then for a
  reason that was not found: the same read at 11 and 4 gave 3.3 MB/s
  later the same day, 1 ms a command, with nothing changed in what
  the read goes through. The processor
  loses the bus 200 ns for each byte whichever it is, but at 15 and 1
  it has the bus a sixteenth of the time while a transfer lasts, and
  the longest, 64K, lasts 14 ms: interrupts are served that much more
  slowly, a serial port with no FIFO can lose what comes in meanwhile,
  and the refresh of the memory, which is why 15 is the most the
  adapter may hold the bus, has no room. Files gained nothing by it
  when each of their blocks, 1K, was a command of its own, and the
  command nearly all of what the block cost; see below for what they
  are now. All three can be read and set while the system runs, between
  commands: `sysctl machdep.aha_buson`, `aha_busoff` (microseconds) and
  `aha_speed` (50, 57, 67, 80 or 100, tenths of a megabyte a second).
  A speed that the machine's memory cannot keep up with spoils what is
  read and written, the file system's own blocks with the rest.
- The disks have one command out at a time, in a CCB of their own; the
  tapes have another, which may be out with it, a target leaving the
  bus while it works, so that a tape rewinding for a minute does not
  hold the disks up. The disks' command is for the transfer at the
  head of the queue and for as many of those after it, up to 16 transfers and
  32K, as go the same way and each begin on the disk where the one
  before ends: the adapter is given a list of where each one's data is
  and fills or empties them in turn, so still nothing is copied. A
  command costs the processor and the disk as much for 1K as for 16K,
  and a file is a row of 1K transfers. The transfers of the command
  that is out are off the queue until it ends. `disksort` puts a read
  after the first write in the queue, which was among them when they
  were left there, and the read was then given back with the writes
  when their command ended, never having been read: on a real disk,
  where writes do wait and join, that was a block of the free list
  read as whatever its buffer held, and "file system full".
- The transfers have to be in the queue together for that. Blocks
  written to a file get there when the disk is slower than the program
  writing. Blocks read from a file would come one at a time, each as
  it was wanted, so a file read in order on this disk is read ahead of
  by as many as 16 blocks that follow one another on the disk, started
  together eight blocks before the reader reaches them, and the driver
  is held back until all of them are in its queue (`breadrun` in
  `sys/sys/ufs_bio.c`, `bioplug`). `ramax` is the 16, and patched to 1
  leaves the one block read ahead that there always was. The IDE
  driver does not join transfers, and there a file is read ahead of by
  one block as before: more only made its queue longer.
- A command that fails is tried four times, unless the target has
  understood it and refused, or is not there. If it was for several
  transfers and still fails, each of them is sent by itself, to succeed
  or to have the error that is its own, and the driver says so.
- Once a second the driver looks at the commands that are still out. If
  the adapter says one has finished, the interrupt was lost, and the
  driver carries on and says so once. If one has been out longer than
  it may be, 30 seconds for a disk's (`ahatimeout`), the adapter and the
  SCSI bus are reset, and each command that was out is sent again or
  fails, as its driver does with it.

## SCSI tapes (`sys/i286at/st.c`)

Any SCSI tape drive on the Adaptec, beside the disks (`st(4)`). The
driver asks a drive what it can do, not which it is, and uses
upstream's interface for a tape, `mtio.h`: a read or a write is a
record, a file ends at a tape mark, and `ioctl` moves the tape.

- The bus is looked over when a tape is first opened. Each tape drive
  found, in order of SCSI id, is the next drive: `st0`, `st1`, up to
  `NST` (2 in `sys/conf/PC`, at most 4), only unit 0 of a target. The
  driver says what it found, as `sd.c` does:

  ```
  st0: ARCHIVE VIPER 150 21247 2.10, id 2, async
  st1: ARCHIVE Python 04687-XXX 4.CM, id 3, async
  ```

- The names are upstream's (`MAKEDEV st0`): `rmt0` is rewound when it
  is closed and `nrmt0`, minor 4, is not; `rmt8` and `nrmt8` are the
  same again, the higher density of two on upstream's drives, and the
  tape the programs open when given none. Here the density is the
  drive's, which writes at the one it chooses for the tape and reads
  whatever it finds, and bit 8 of the minor number does nothing. There
  is no block device: upstream's was for 1K blocks through the buffer
  cache, which nothing here reads a tape by.
- A drive whose least record is a byte, as a DAT is, is set to records
  of any length (`MODE SELECT`, a block length of 0): a write is a
  record, and a read takes one, which must be no longer than the read.
  Any other, as a QIC drive, has blocks of one size, the one it is set
  to, or its least if that is not between its limits: a read or a write
  is of whole blocks, as many as it asks for, so a write of 10K is
  twenty, and one that is not of whole blocks is refused (`EINVAL`).
  Each goes straight to or from the process's memory, as a disk's raw
  transfer does.
- A read that stops short says so: at a tape mark, which returns 0, and
  the next read is of the next file; at the end of what is written,
  which returns 0 too; or with a record shorter than the read, which
  returns the record. A read of a fixed-block tape that meets a mark
  after some blocks returns those, and the next read returns 0. The
  drive says how much it did not move in its sense data, and the driver
  goes by that. A drive that returns a short record with no error, as
  86Box's do, leaves only the adapter's count: a C, CF or CP gives it
  when the command asks (CCB opcode 3), and the driver takes it only
  when there is no sense data, since 86Box's adapter leaves 0 there
  when there is.
- 86Box's drives differ from real ones in two ways that `tests/tape`
  avoids. A fixed-block read that meets a mark after some blocks says
  it read the whole length, so the test reads whole files. Its HP
  C5683A gives limits of 512 bytes to 32K, so here it is a drive of
  512-byte blocks; the test's DAT drive is the Archive Python, whose
  records are of any length, as a real DAT drive's are.
- A tape that was written gets one tape mark when it is closed, where
  upstream's drivers write two, the end of a nine-track tape, and go
  back over the second. A QIC drive writes only at the end of what is on
  the tape, so a mark there could not be written over; the end of what
  is written reads as an end of file anyway, so a tape still ends with
  two. Then it is rewound, unless the minor number says not, and close
  waits for it: the disks do not.
- `open` waits up to a minute for a drive that is loading a tape, and
  says on the opener's terminal when there is no tape or the tape is
  write protected and was opened to write. A read's or a write's error
  goes there and to the log, with the drive's sense key and code.
- A read or a write may take 15 minutes, and a command that may move the
  whole tape, a rewind, a move over files, a tape mark, 4 hours, before
  the adapter is reset. Near the end of the tape a drive warns that it
  is there, after which writes are refused (`ENOSPC`, unless
  `MTIOCIEOT`), with room left for the mark.
- `mt status` says `SCSI tape drive` (`MT_ISST`), whether the tape is
  write protected (`WRTLCK`), past the warning (`EOM`) or last written
  (`WRITTEN`), and the third byte of the last sense data, the key under
  `MARK`, `EOM` and `ILI`, with the additional sense code above it.
- `lseek` does not move the tape: upstream's raw tapes take it only as
  where the next block is counted from.

## The diskette controller (`sys/i286at/fl.c`)

The controller of the PC/AT is an NEC 765 at 0x3F0, with interrupt
request 6 and DMA channel 2, and two drives. The driver is upstream's
for the RX02 in what it gives ([rx(4)](../usr/src/man/man4/rx.4),
[fl(4)](../usr/src/man/man4/fl.4)): a diskette has no label of its own
and no partitions. There is one device for each drive, `fl0` and
`fl1`, and the driver finds what the diskette in it is, where upstream's
RX02 had a device for each density.

| in a drive that is | sectors on a track | sectors | `disktab` |
|---|---|---|---|
| 3.5 inch, either density | 9 | 1440 (720K) | `fl720` |
| 3.5 inch, high density | 18 | 2880 (1.44 MB) | `fl1440` |
| 5.25 inch, high density | 15 | 2400 (1.2 MB) | `fl1200` |

- What a drive is comes from the clock chip's memory, where the
  machine's setup program wrote it, when a diskette device is first
  opened. Diskettes of 360K are not supported.
- What the diskette is, the driver finds when the drive is first read or
  written after it was last closed: it reads the header of whatever
  sector comes under the head (the 765's READ ID), at the rate of each
  kind the drive takes in turn, the largest first. A diskette of 720K
  read at the rate of 1.44 MB has no header that can be read, and the
  next rate is tried. One that is not formatted has none at either:
  `fl0: format unknown`, and the transfer fails.
- `flformat /dev/rfl0` formats one as the largest the drive takes, and
  `flformat -f 720 /dev/rfl0` as one of 720K, as `rxformat` does an
  RX02, with the same request of the driver (`FLIOC_FORMAT`,
  `i286at/flreg.h`), which is given the size. It does not read back
  what it wrote.
- Asked for a label (`DIOCGDINFO`), the driver gives one for what it
  found, of one partition, the whole diskette, so `newfs /dev/rfl0`
  needs no disk type. `tar` and `dump` write to the raw device, and
  `mtools` reads and writes MS-DOS's diskettes through it: `mdir a:`,
  `mcopy file b:` ([printers, and the rest of
  upstream's](userland.md#printers-and-the-rest-of-upstreams)).
- DMA does not count past a multiple of 64K in memory. A transfer is
  cut there, and a sector with one inside it goes by way of 512 bytes
  of the kernel's own that have none.
- A transfer is tried five times, and the head sent back to cylinder 0
  before every second one. An interrupt that has not come in three
  seconds will not, as when there is no diskette: the controller is
  reset and the transfer fails (`fl0: not ready`). The motor is stopped
  three seconds after the last transfer.
- A diskette changed while its file system is mounted, or while the
  drive is otherwise open, is not noticed.

A diskette with a file system, `/boot` and a kernel in it, and the
boot block `/mdec/fluboot` in its first sector, starts the system
([booting](boot.md#from-a-diskette-with-a-file-system),
[the system makes itself](self-hosting.md#a-diskette)). The root is
then a hard disk's: a diskette has no room for a root and a swap area,
as upstream's document says of its own.

On a real machine the system has formatted diskettes of 1.44 MB and
720K, made file systems on them and copied files to them, and started
from the 1.44 MB one with `/boot` and the kernel it was given; the
diskettes were then as `tools/fsck.py` wants them. A diskette of 1.2 MB
has been made in the emulator only.

## Crash dumps

After a panic the kernel brings the disks up to date, writes its memory
to the swap area and starts the machine again, as upstream's does
(`dumpsys`, `sys/i286/machdep.c`); `reboot -d` does the same without a
panic. At the next start `/etc/rc` has `savecore` save the dump
([crash dumps](userland.md#crash-dumps)).

- The dump goes 512 sectors into the swap area (`dumplo`,
  `sys/i286/conf.c`), so that what is swapped out as the system starts
  again does not write over it before `savecore` has saved it. Upstream
  configures the device (`DUMPDEV`); this kernel finds its disks as it
  starts, and takes the swap area of the one it came from when its
  driver can write a dump, which the driver's `d_dump` in `bdevsw` says
  (`sys/sys/init_main.c`). The RAM disk's cannot: the system on the
  installation diskette takes none.
- The dump is the memory from address 0 to its end, the hole from 640K
  to 1M with it, so that where a byte is in the dump is its address:
  4,587,520 bytes on the emulator's machine of 4 MB. What does not fit
  in the swap area is left out; the kernel is in the first 640K.
- The first bytes of memory, the vectors of real mode that the kernel
  does not use, are a header first (`struct dumphdr`, `machine/seg.h`),
  as the PDP-11's kernel puts its registers in low memory for a dump:
  where the global descriptor table is, through which `savecore` and
  `adb` find the kernel's segments; how much was dumped; and the
  registers where the dump was taken, from which `adb`'s `$c` starts.
- The driver writes it 32K at a time with the processor, with
  interrupts off and the controller as the panic left it. `wd` resets
  the controller first, tells the drive again how it counts sectors,
  and moves each sector when the drive asks for it. `sd` resets the
  adapter and the SCSI bus, so that a command that was out is gone, and
  sends each piece as one command and waits for it (`ahapoll`); the
  disk refuses the first command after the reset, and each is tried
  four times.
- The disks' interrupts must come in while the buffers are written after
  a panic: an exception in the kernel arrives with them off, and `boot`
  turns them on, and `tsleep` lowers the level for them ([interrupts and
  their priority](processes.md#interrupts-and-their-priority)). Before,
  a panic from an exception stopped at "syncing disks".

## Tools

`tools/mkdisk.py` makes a disk image: a geometry, a partition table or
none, a label, partitions filled from file system images or left for a
file system that `newfs` is to make, and the sectors that boot it.
`tools/fsck.py` checks a file system in an image, or in a partition of a
disk image, without changing it, and can print a file from it or list
what is in it. It is how the tests see what the kernel left on a disk.
On the system a disk that boots is made as upstream has one made, with
`disklabel`, `newfs`, `dump`, `restor` and `tar`
([the system makes itself](self-hosting.md#a-disk)).

## Tested, and not

`tests/kernel` runs the kernel on a hard disk five times. Three of
them boot from the disk with no diskette in the machine; the 2.6 GB
ones have the system where no BIOS reads, and take the kernel from a
diskette.

| | the disk | the system's part | | the kernel comes from |
|---|---|---|---|---|
| IDE | 20 MB | from the second track, by a partition table | sectors by cylinder, head and sector, 16 to an interrupt | the disk; `/boot` left to itself |
| IDE | 17 MB | all of it | sectors by number, one to an interrupt | the disk; `/boot` told a name that is no file, then `wd(0,0)sys/unix` |
| IDE | 2.6 GB | from sector 5,000,000 | sectors by number, 16 to an interrupt | a diskette |
| SCSI | 20 MB, id 0 | from the second track | adapter at 0x330, request 11, channel 5 | the disk, through the adapter's BIOS; `/boot` left to itself |
| SCSI | 2.6 GB, id 5 | from sector 5,000,000 | adapter at 0x334, request 10, channel 6, its BIOS off; one interrupt ignored, one command never sent and one for several transfers failed (`ahatest`), with the time limit at 5 seconds | a diskette, with `diskroot` changed |

Each time the test program reads the label, writes and reads 20K
straight to a partition from an odd address in its memory, reads the
same through the buffer pool, reads and writes across the end of the
partition, and writes, reads and removes files of 300K. Its 24 processes
that do not fit in memory are swapped to the disk. After the kernel has
halted, `fsck.py` must find nothing wrong with the file system and must
find the last file the test wrote. On the last boot the driver must also
have said that an interrupt was lost and that a command timed out,
and, its first command for several transfers having been made to fail
each time it was tried, that it tried them singly. The disk boots
have the 64 buffers of a machine with 1 MB, but for the first of the
SCSI ones, which has 128, so that some keep files' data
(docs/performance.md). That boot also has every command's end seen
only at the clock's next tick (`ahatest`), which is a slow disk: the
emulator's own answers before a second transfer can be queued, so
without it no write is ever joined to another, and nothing arrives
while a command for several is out. That is how the fault above went
unseen until a real disk found it.

The emulated machine's own BIOS, as the tests have it set up, does not
boot from a drive on an IDE controller, so for the IDE boots the
emulator is given the XTIDE Universal BIOS beside the controller. The
kernel's driver does not use it.

The emulator's adapter is a 1542CF, and so is the one the system has
booted from on a real machine; there what the driver said a disk had
agreed, synchronous at 200 ns a byte and 8 bytes ahead, was what the
disk's own log said. The emulator's 1542CP answers only after
something has configured it by Plug and Play, which the machine's BIOS
does not do and this kernel does not either. A real CP in such a machine
uses the settings on its switches, by its installation guide, and takes
the same commands as a CF; no real one has been tried.

Not done, or not tested:

- No test makes a transfer fail with an error, so the retries after an
  error and the error reports have not run. On SCSI the retry after a
  reset has.
- The IDE driver has no time limits: a drive that stops answering is
  waited for with the processor, for some seconds, and an interrupt
  that never comes is waited for for ever.
- A second IDE drive, the slave, is the one `tests/programs` writes
  labels and file systems on. A second SCSI disk is found and given a
  drive, but nothing has read it or written it.
- A SCSI disk that is not spinning is not started, and one that takes
  its time after a reset is not waited for beyond the four tries.
- SCSI devices that are not disks are named when the bus is looked over
  and otherwise ignored.
- The drivers do nothing about bad sectors. `badsect` keeps one out of
  a file system's use.
- `tests/crash` has the kernel panic and `reboot -d` take a dump on an
  IDE disk and on a SCSI disk, each 4.4 MB. No dump has been taken on a
  real machine.
- `disklabel -B` installs the boot block, and `/boot` is a file, but
  only `tools/mkdisk.py` writes the first sector of a shared disk, with
  its table and `mbr.s`. A disk booted from after `disklabel -B` has
  not been tried; `tests/programs` reads the sector back.
- The kernel mounts only a partition that the label says is a 2.11BSD
  file system; `disklabel` changes the label ([the C library and
  programs](userland.md#disks-and-file-systems)), and `tests/programs`
  has it make a partition one and be refused one that runs past the
  system's part. That has been tried on an IDE disk only.
- Most of it has run in the emulator only. The system has also booted
  on a real machine from a SCSI disk, and from a diskette it made there.
