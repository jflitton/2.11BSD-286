/*
 * /boot: load the kernel from a file system and start it.
 *
 * The boot block (hdboot.s, or flboot.s on a diskette) loads this from
 * the root directory of the disk the machine was booted from.  It asks what to load, like this:
 *
 *	Boot from sd(0,0)
 *	:
 *	sd(0,0)unix: 64316+23002+57230+41288
 *
 * The answer is the name of a file, by default unix.  Before the name
 * can come the device that the file is on, as the kernel is to know it:
 * the driver, and in brackets the drive and the partition, from 0.  The
 * kernel takes that as its root.  With nothing typed for some seconds
 * the default is loaded from the device shown.  The last line is what
 * is being loaded and the size in memory of each of its segments.
 *
 * Disks are read through the BIOS, and only the one booted from: the
 * drive number typed is what the kernel is told, and no more.  From a
 * diskette the kernel is so told that its root is the first hard disk,
 * unless another is typed; but if the diskette has a file miniroot
 * beside the kernel, that is loaded too, above 1M, and is the root.  What
 * device the kernel will make of the BIOS's drive is a guess, shown in
 * the first line; docs/boot.md says how it is made and when it is wrong.
 *
 * The console is the display and keyboard, through the BIOS, and the
 * first serial port as well if there is one, as the kernel sets it.
 *
 * srt0.s starts this and makes the BIOS's calls.  The program is one
 * segment of code and data, at most 56K with what the file does not
 * hold.
 */
#include <sys/param.h>
#include <sys/fs.h>
#include <sys/inode.h>
#include <sys/dir.h>
#include <sys/disklabel.h>
#include <sys/exec.h>
#include "../i286/boot.h"
#include "../i286at/pcdisk.h"

#define	KERNSEG	0x1000		/* where the kernel goes, as a segment */
#define	WAIT	5		/* seconds before the default is loaded */
#define	SECTOR	512

#define	FAR(seg, off)	((char __far *)((u_long)(seg) << 16 | (u_int)(off)))

struct devname {
	char	*dn_name;
	int	dn_major;	/* its block device (../i286/conf.c) */
} devname[] = {
	"wd",	1,
	"sd",	2,
	0
};

int	drive;			/* the BIOS's number for the disk */
int	nhead, nsect;		/* its shape, as the BIOS has it */
daddr_t	syspart;		/* the first sector of the system's part, */
daddr_t	fsstart;		/*   and of the file system in it */
struct	disklabel label;
int	labelled;		/* there is a label, and that is it */

struct	dinode ino;		/* the file being read, */
long	fpos;			/*   and how far it has been read */
char	blk[DEV_BSIZE];		/* a block of it, */
daddr_t	blklbn;			/*   which is this block of the file */
daddr_t	ind[2][NINDIR];		/* two of its blocks of block numbers, */
daddr_t	indbn[2];		/*   which are these of the file system */

struct	bootinfo bi;
int	quiet;			/* fopen is not to say what it did not find */

int	printf(char *, ...);

/*
 * The console.
 */
#define	COM1		0x3F8
#define	LCR		3
#define	MCR		4
#define	LSR		5
#define	LSR_RXRDY	0x01
#define	LSR_THRE	0x20

int	com;			/* the serial port, if there is one */

cninit()
{
	/* A port that is there gives back what was written to it. */
	outb(COM1 + LCR, 0x1B);
	if (inb(COM1 + LCR) != 0x1B)
		return;
	outb(COM1 + LCR, 0x80);
	outb(COM1, (int)(115200L / 9600));
	outb(COM1 + 1, 0);
	outb(COM1 + LCR, 3);		/* 8 bits, no parity */
	outb(COM1 + 1, 0);		/* no interrupts */
	outb(COM1 + MCR, 3);
	com = COM1;
}

putchar(c)
	int c;
{
	register int n;

	if (c == '\n')
		putchar('\r');
	biosputc(c);
	if (com) {
		for (n = 0; --n && (inb(com + LSR) & LSR_THRE) == 0; )
			;
		outb(com, c);
	}
}

/*
 * A character that has been typed, or -1.
 */
typed()
{
	register int c;

	if ((c = bioskey()) >= 0)
		return (c);
	if (com && (inb(com + LSR) & LSR_RXRDY))
		return (inb(com) & 0177);
	return (-1);
}

/*
 * Read a line.  If secs is not 0 and nothing at all is typed in that
 * many seconds, the line is empty.
 */
getline(buf, size, secs)
	char *buf;
	int size, secs;
{
	register char *cp = buf;
	register int c;
	u_int __far *clock = (u_int __far *)FAR(0x40, 0x6C);
	u_int since = *clock;

	for (;;) {
		if ((c = typed()) < 0) {
			/* The BIOS's clock ticks 18.2 times a second. */
			if (secs && *clock - since > secs * 182 / 10)
				break;
			continue;
		}
		secs = 0;
		if (c == '\r' || c == '\n')
			break;
		if (c == '\b' || c == 0177) {
			if (cp > buf) {
				cp--;
				printf("\b \b");
			}
		} else if (c >= ' ' && cp < buf + size - 1) {
			*cp++ = c;
			putchar(c);
		}
	}
	putchar('\n');
	*cp = 0;
}

static
printn(n, base)
	u_long n;
	int base;
{
	char buf[12];
	register char *cp = buf;

	do {
		*cp++ = "0123456789abcdef"[n % base];
		n /= base;
	} while (n);
	while (cp > buf)
		putchar(*--cp);
}

/*
 * %d %u %x, each with l for a long, %s and %c.
 */
printf(char *fmt, ...)
{
	register int *ap = (int *)(&fmt + 1);
	register char *s;
	int c, islong;
	long n;

	while (c = *fmt++) {
		if (c != '%') {
			putchar(c);
			continue;
		}
		if (islong = *fmt == 'l')
			fmt++;
		switch (c = *fmt++) {
		case 'd': case 'u': case 'x':
			if (islong) {
				n = *(long *)ap;
				ap += 2;
			} else if (c == 'd')
				n = *ap++;
			else
				n = (u_int)*ap++;
			if (c == 'd' && n < 0) {
				putchar('-');
				n = -n;
			}
			printn((u_long)n, c == 'x' ? 16 : 10);
			break;
		case 's':
			for (s = (char *)*ap++; *s; s++)
				putchar(*s);
			break;
		case 'c':
			putchar(*ap++);
			break;
		default:
			putchar(c);
		}
	}
}

/*
 * The disk.  Read count sectors that are in one track.
 */
rdsect(sn, count, buf)
	daddr_t sn;
	int count;
	char *buf;
{
	daddr_t track = sn / nsect;
	int sect = sn % nsect;
	int tries, error;

	if (track / nhead > 1023) {
		printf("sector %ld is past the cylinders the BIOS can read\n",
		    sn);
		return (-1);
	}
	for (tries = 3; tries; tries--) {
		error = biosread(drive, (int)(track / nhead),
		    (int)(track % nhead), sect + 1, count, buf);
		if (error == 0)
			return (0);
		biosreset(drive);
	}
	printf("sector %ld: the BIOS reports error %x\n", sn, error);
	return (-1);
}

/*
 * A diskette's shape.  The BIOS knows what the drive can hold and not
 * what this diskette does: the sectors on a track are the most that
 * the first track has one of.
 */
flgeom()
{
	static int sizes[] = { 18, 15, 9, 0 };
	register int *sp, tries;

	nhead = 2;
	for (sp = sizes; *sp; sp++)
		for (tries = 3; tries; tries--) {
			if (biosread(drive, 0, 0, *sp, 1, blk) == 0) {
				nsect = *sp;
				return (0);
			}
			biosreset(drive);
		}
	return (-1);
}

/*
 * Read a block of the file system.
 */
rdblk(bn, buf)
	daddr_t bn;
	char *buf;
{
	daddr_t sn = fsstart + bn * (DEV_BSIZE / SECTOR);

	if (sn % nsect != nsect - 1)
		return (rdsect(sn, 2, buf));
	if (rdsect(sn, 1, buf) < 0)
		return (-1);
	return (rdsect(sn + 1, 1, buf + SECTOR));
}

/*
 * Whose the disk is, as the kernel decides it (../i286at/pcdisk.c): the
 * first entry of the table in its first sector with the system's
 * number, and with none, all of it.  Then the label, which is the
 * second sector of the system's part.
 */
finddisk()
{
	register struct pcpart *pp;
	register struct disklabel *lp;

	syspart = 0;
	labelled = 0;
	if (rdsect((daddr_t)0, 1, blk) < 0)
		return (-1);
	if ((u_char)blk[510] == 0x55 && (u_char)blk[511] == 0xAA) {
		pp = (struct pcpart *)(blk + PCP_TABLE);
		for (; pp < (struct pcpart *)(blk + PCP_TABLE) + PCP_NPART;
		    pp++)
			if (pp->pp_type == PCP_2BSD) {
				syspart = pp->pp_start;
				break;
			}
	}
	if (rdsect(syspart + LABELSECTOR, 1, blk) < 0)
		return (-1);
	lp = (struct disklabel *)(blk + LABELOFFSET);
	if (lp->d_magic == DISKMAGIC && lp->d_magic2 == DISKMAGIC &&
	    lp->d_npartitions <= MAXPARTITIONS) {
		label = *lp;
		labelled = 1;
	}
	return (0);
}

/*
 * Files.  Make inode n the file being read.
 */
iopen(n)
	ino_t n;
{
	blklbn = -1;
	indbn[0] = indbn[1] = 0;
	fpos = 0;
	if (rdblk((daddr_t)itod(n), blk) < 0)
		return (-1);
	ino = ((struct dinode *)blk)[itoo(n)];
	return (0);
}

/*
 * One of the file's blocks of block numbers, kept in ind[which].
 */
daddr_t *
indirect(which, bn)
	int which;
	daddr_t bn;
{
	if (bn == 0)
		return (0);
	if (indbn[which] != bn) {
		indbn[which] = 0;
		if (rdblk(bn, (char *)ind[which]) < 0)
			return (0);
		indbn[which] = bn;
	}
	return (ind[which]);
}

/*
 * Where in the file system block lbn of the file is: 0 if it is a
 * hole, -1 if it cannot be found out.
 */
daddr_t
bmap(lbn)
	daddr_t lbn;
{
	register daddr_t *ip;
	daddr_t bn;

	if (lbn < NDADDR)
		return (ino.di_addr[lbn]);
	lbn -= NDADDR;
	bn = ino.di_addr[NDADDR];
	if (lbn >= NINDIR) {
		/* Through two blocks.  No file here needs three. */
		lbn -= NINDIR;
		if (lbn >= (long)NINDIR * NINDIR) {
			printf("the file is too big\n");
			return (-1);
		}
		if (ino.di_addr[NDADDR + 1] == 0)
			return (0);
		if ((ip = indirect(0, ino.di_addr[NDADDR + 1])) == 0)
			return (-1);
		bn = ip[lbn / NINDIR];
		lbn %= NINDIR;
	}
	if (bn == 0)
		return (0);
	if ((ip = indirect(1, bn)) == 0)
		return (-1);
	return (ip[lbn]);
}

/*
 * Have block lbn of the file in blk.
 */
getblk(lbn)
	daddr_t lbn;
{
	daddr_t bn;

	if (lbn == blklbn)
		return (0);
	blklbn = -1;
	if ((bn = bmap(lbn)) < 0)
		return (-1);
	if (bn == 0)
		fclear((char __far *)blk, DEV_BSIZE);
	else if (rdblk(bn, blk) < 0)
		return (-1);
	blklbn = lbn;
	return (0);
}

/*
 * Read the next count bytes of the file to anywhere in memory.
 */
fread(to, count)
	char __far *to;
	u_int count;
{
	register u_int off, n;

	while (count) {
		if (getblk(fpos / DEV_BSIZE) < 0)
			return (-1);
		off = fpos % DEV_BSIZE;
		n = DEV_BSIZE - off;
		if (n > count)
			n = count;
		fcopy(to, blk + off, n);
		to += n;
		count -= n;
		fpos += n;
	}
	return (0);
}

/*
 * The inode number of a name in the directory being read, or 0.  An
 * entry does not cross from one block to the next, and the entries of
 * a block that was never filled end with one of no length.
 */
ino_t
dlook(name, len)
	char *name;
	int len;
{
	register struct direct *dp;
	register int i;
	daddr_t lbn;

	for (lbn = 0; lbn * DEV_BSIZE < ino.di_size; lbn++) {
		if (getblk(lbn) < 0)
			return (0);
		for (dp = (struct direct *)blk; (char *)dp < blk + DEV_BSIZE;
		    dp = (struct direct *)((char *)dp + dp->d_reclen)) {
			if (dp->d_reclen == 0)
				break;
			if (dp->d_ino == 0 || dp->d_namlen != len)
				continue;
			for (i = 0; i < len && dp->d_name[i] == name[i]; i++)
				;
			if (i == len)
				return (dp->d_ino);
		}
	}
	return (0);
}

/*
 * Make the file of that name the one being read.
 */
fopen(path)
	register char *path;
{
	register char *cp;
	ino_t n = ROOTINO;

	for (;;) {
		if (iopen(n) < 0)
			return (-1);
		while (*path == '/')
			path++;
		if (*path == 0)
			break;
		for (cp = path; *cp && *cp != '/'; cp++)
			;
		if ((ino.di_mode & IFMT) != IFDIR ||
		    (n = dlook(path, cp - path)) == 0) {
			if (!quiet)
				printf("no such file\n");
			return (-1);
		}
		path = cp;
	}
	if ((ino.di_mode & IFMT) != IFREG) {
		printf("not a file\n");
		return (-1);
	}
	return (0);
}

/*
 * A diskette may have a root file system for the kernel to keep in
 * memory, the file miniroot (buildmini): put it above 1M, where the
 * kernel has its RAM disk, a block at a time through the BIOS.  The
 * kernel is in memory by now, and only blk is read into.
 */
miniroot()
{
	long size, lbn;

	quiet = 1;
	lbn = fopen("miniroot");
	quiet = 0;
	if (lbn < 0)
		return;
	size = (ino.di_size + DEV_BSIZE - 1) / DEV_BSIZE;
	printf(", miniroot %ldK", size);
	if (size > biosext()) {
		printf(": more than there is above 1M");
		return;
	}
	for (lbn = 0; lbn < size; lbn++)
		if (getblk(lbn) < 0 || extmove(blk, (u_int)(lbn << 10),
		    0x10 + (u_int)(lbn >> 6)) < 0) {
			printf(": not loaded");
			return;
		}
	bi.bi_rdsize = size * (DEV_BSIZE / SECTOR);
}

/*
 * Load the kernel that is the file being read: each segment in turn
 * from KERNSEG up, at a multiple of 512 bytes, which is what the kernel
 * counts memory in.  The size of each is printed when it is in.
 * Returns if it cannot.
 */
load(root)
	dev_t root;
{
	struct exec x;
	struct exseg xs[BOOTNSEG];
	register struct exseg *xp;
	register struct bootseg *bs;
	u_int para = KERNSEG, top = biosmem() * 64;
	long done;
	u_int n;

	if (fread((char __far *)&x, sizeof x) < 0)
		return;
	if (ino.di_size < sizeof x || x.a_magic != A_MAGIC || x.a_nseg == 0 ||
	    x.a_nseg > BOOTNSEG || x.a_eseg == 0 || x.a_eseg > x.a_nseg ||
	    x.a_dseg == 0 || x.a_dseg > x.a_nseg) {
		printf("not a kernel\n");
		return;
	}
	if (fread((char __far *)xs, x.a_nseg * sizeof xs[0]) < 0)
		return;
	bs = bi.bi_seg;
	for (xp = xs; xp < xs + x.a_nseg; xp++, bs++) {
		if (xp->xs_memsz == 0 || xp->xs_memsz > 0x10000 ||
		    xp->xs_filesz > xp->xs_memsz) {
			printf("not a kernel\n");
			return;
		}
		if (para + (xp->xs_memsz + 15) / 16 > top) {
			printf("\ntoo big for the %dK of memory below 640K\n",
			    top / 64);
			return;
		}
		for (done = 0; done < xp->xs_filesz; done += n) {
			n = 0x4000;
			if (xp->xs_filesz - done < n)
				n = xp->xs_filesz - done;
			if (fread(FAR(para, done), n) < 0)
				return;
		}
		fpos += xp->xs_filesz & 1;
		for (; done < xp->xs_memsz; done += n) {
			n = 0x4000;
			if (xp->xs_memsz - done < n)
				n = xp->xs_memsz - done;
			fclear(FAR(para, done), n);
		}
		bs->bs_para = para;
		bs->bs_nsect = (xp->xs_filesz + SECTOR - 1) / SECTOR;
		bs->bs_limit = xp->xs_memsz - 1;
		bs->bs_type = xp->xs_type;
		printf("%s%ld", xp == xs ? "" : "+", xp->xs_memsz);
		para += (xp->xs_memsz + SECTOR - 1) / SECTOR * (SECTOR / 16);
	}
	bi.bi_magic = BOOTMAGIC;
	bi.bi_nseg = x.a_nseg;
	bi.bi_eseg = x.a_eseg;
	bi.bi_eoff = x.a_entry;
	bi.bi_dseg = x.a_dseg;
	bi.bi_drive = drive;
	bi.bi_rdsect = bi.bi_rdsize = 0;
	bi.bi_root = root;
	if ((drive & 0x80) == 0)
		miniroot();
	printf("\n");
	startkernel(&bi, bi.bi_seg[x.a_eseg - 1].bs_para, x.a_entry,
	    bi.bi_seg[x.a_dseg - 1].bs_para);
}

/*
 * A number in what was typed, which must end with the character given.
 */
char *
number(cp, np, end)
	register char *cp;
	int *np, end;
{
	register int n = 0;

	if (*cp < '0' || *cp > '9')
		return (0);
	while (*cp >= '0' && *cp <= '9')
		n = n * 10 + *cp++ - '0';
	*np = n;
	return (*cp == end ? cp + 1 : 0);
}

main(bootdrive)
	int bootdrive;
{
	char line[64];
	register char *cp, *name;
	register struct devname *dn, *dflt;
	struct partition *pp;
	int unit, part, secs;

	drive = bootdrive;
	cninit();
	printf("\n");
	if ((drive & 0x80 ? biosgeom(drive, &nhead, &nsect) : flgeom()) < 0 ||
	    finddisk() < 0) {
		printf("boot: cannot read drive %x\n", drive);
		for (;;)
			;
	}
	/*
	 * What the kernel will call this disk: by its label a SCSI disk
	 * or not, and as many drives along as the BIOS has it.
	 */
	dflt = &devname[0];
	if (labelled && label.d_type == DTYPE_SCSI)
		dflt = &devname[1];
	for (secs = WAIT;; secs = 0) {
		dn = dflt;
		unit = drive & 0x7F;
		part = 0;
		printf("Boot from %s(%d,%d)\n: ", dn->dn_name, unit, part);
		getline(line, sizeof line, secs);
		for (name = line; *name == ' '; name++)
			;
		for (cp = name; *cp && *cp != '('; cp++)
			;
		if (*cp) {
			*cp++ = 0;
			for (dn = devname; dn->dn_name; dn++)
				if (strcmp(dn->dn_name, name) == 0)
					break;
			if (dn->dn_name == 0 ||
			    (cp = number(cp, &unit, ',')) == 0 ||
			    (cp = number(cp, &part, ')')) == 0 ||
			    unit > 31 || part >= MAXPARTITIONS) {
				printf("the device is wd or sd, then (drive,partition)\n");
				continue;
			}
			name = cp;
		}
		if (*name == 0)
			name = "unix";
		fsstart = syspart;
		if (labelled) {
			pp = &label.d_partitions[part];
			if (part >= label.d_npartitions || pp->p_size == 0) {
				printf("no such partition\n");
				continue;
			}
			fsstart += pp->p_offset;
		} else if (part) {
			printf("the disk has no label, so only partition 0\n");
			continue;
		}
		printf("%s(%d,%d)%s: ", dn->dn_name, unit, part, name);
		if (fopen(name) == 0)
			load(makedev(dn->dn_major, unit << 3 | part));
	}
}

strcmp(a, b)
	register char *a, *b;
{
	while (*a == *b++)
		if (*a++ == 0)
			return (0);
	return (1);
}
