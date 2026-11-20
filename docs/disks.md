# Disks

What a disk looks like to the kernel on a PC, and the two drivers for
real disks. They run in the emulator under `tests/kernel`, which boots
the kernel with its root file system and swap area on a hard disk, and
from the disk itself ([booting](boot.md)).

## Devices

| block | character | | driver |
|-------|-----------|---|--------|
| 0 | 7 | disks in memory: 0 the image the boot program loaded, 1 a swap area | `sys/i286/rd.c` |
| 1 | 8 | hard disks on the AT disk controller | `sys/i286at/wd.c` |
| 2 | 9 | SCSI disks on an Adaptec AHA-1540 or 1542 | `sys/i286at/sd.c`, `aha.c` |

A hard disk's minor number is the drive in bits 3 and up and the
partition, `a` to `h`, in the low three: `wd0a` is 0, `wd1c` is 10.
Through the character device a disk moves whole sectors only: a count
that is not a multiple of 512 is refused (`EINVAL`). Upstream's programs
that read a file system directly read its superblock as the size of its
structure, as the PDP-11's controllers let them; here they read the 1K
block it is in.
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

If the boot program loaded a RAM disk image, that is the root, and the
swap area is the second disk in memory. If it did not, the root is the
device the boot program says it loaded the kernel from, and the swap
area is partition `b` of the same drive.

`/boot`, which loads the kernel from a hard disk, says: [booting](boot.md)
has how it decides, which begins with whether the disk's label calls it
a SCSI disk. The diskette's boot sector does not say, and then the root
is `diskroot` (`sys/i286/conf.c`): the first IDE drive's partition `a`
unless the kernel file is changed with `tools/kpatch.py`, where 512
makes it the first SCSI disk's.

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
adapter and `sd.c` the disks on it.

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
  so that a diskette drive's DMA is not kept waiting, and this kernel
  has no diskette driver. On a real machine, with a disk that had
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
- One command is out at a time. It is for the transfer at the head of
  the queue and for as many of those after it, up to 16 transfers and
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
- Once a second the driver looks at a command that is still out. If the
  adapter says it has finished, the interrupt was lost, and the driver
  carries on and says so once. If it has been out 30 seconds
  (`ahatimeout`), the adapter and the SCSI bus are reset and the command
  is sent again.

## Tools

`tools/mkdisk.py` makes a disk image: a geometry, a partition table or
none, a label, partitions filled from file system images or left for a
file system that `newfs` is to make, and the sectors that boot it.
`tools/fsck.py` checks a file system in an image, or in a partition of a
disk image, without changing it, and can print a file from it. It is how
the tests see what the kernel left on a disk.

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
  labels and file systems on. A second SCSI disk has only been found
  absent.
- A SCSI disk that is not spinning is not started, and one that takes
  its time after a reset is not waited for beyond the four tries.
- SCSI devices that are not disks are named when the bus is looked over
  and otherwise ignored.
- The drivers do nothing about bad sectors. `badsect` keeps one out of
  a file system's use.
- No crash dump is written to a disk.
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
  on a real machine from a SCSI disk.
