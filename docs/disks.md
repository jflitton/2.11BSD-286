# Disks

What a disk looks like to the kernel on a PC, and the one driver for a
real disk so far. It runs in the emulator under `tests/kernel`, which
boots the kernel with its root file system and swap area on a hard disk.

## Devices

| block | character | | driver |
|-------|-----------|---|--------|
| 0 | 7 | disks in memory: 0 the image the boot program loaded, 1 a swap area | `sys/i286/rd.c` |
| 1 | 8 | hard disks on the AT disk controller | `sys/i286at/wd.c` |

A hard disk's minor number is the drive in bits 3 and up and the
partition, `a` to `h`, in the low three: `wd0a` is 0, `wd1c` is 10.

## Whose disk it is

A PC's hard disk can be shared between systems by the table at the end
of its first sector, which gives each of up to four systems a run of
sectors and says by a number whose each run is. On the first open of a
drive its driver calls `pcpart` (`sys/i286at/pcpart.c`), which reads that
sector.

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
would let a file system be written on them.

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
  table.
- There is no device for the whole of a shared disk, so the table cannot
  be read or written from inside the system.

## Where the root is

If the boot program loaded a RAM disk image, that is the root, and the
swap area is the second disk in memory. If it did not, the root is
`diskroot` (`sys/i286/conf.c`), which is the first hard disk's partition
`a`, and the swap area is partition `b` of the same drive.

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

## Tools

`tools/mkdisk.py` makes a disk image: a geometry, a partition table or
none, a label, and partitions filled from file system images.
`tools/fsck.py` checks a file system in an image, or in a partition of a
disk image, without changing it, and can print a file from it. It is how
the tests see what the kernel left on a disk.

## Tested, and not

`tests/kernel` boots from a hard disk twice.

| | first | second |
|---|---|---|
| the disk | 615 cylinders, 4 heads, 17 sectors, with a partition table | 1,024 cylinders, 2 heads, 17 sectors, all the system's |
| sectors named by | cylinder, head and sector | number |
| sectors to an interrupt | 16 | 1 |

Each time the test program reads the label, writes and reads 20K
straight to a partition from an odd address in its memory, reads the
same through the buffer pool, reads and writes across the end of the
partition, and writes, reads and removes files of 300K. Its 24 processes
that do not fit in memory are swapped to the disk. After the kernel has
halted, `fsck.py` must find nothing wrong with the file system and must
find the last file the test wrote.

Not done, or not tested:

- A drive that stops answering is waited for with the processor, for
  some seconds, and an interrupt that never comes is waited for for
  ever: there are no timeouts.
- No test makes a transfer fail, so the retries and the error report
  have not run.
- A second drive has only been found absent.
- Nothing is done about bad sectors.
- No crash dump is written to a disk.
- The machine cannot be booted from a hard disk: there is no boot
  program for one. The kernel still comes from the diskette.
- All of it has run in the emulator only.
