/*
 * The first program the kernel runs in its test: /etc/init.  It makes
 * the kernel do the things a system starting up does, checks what comes
 * back, and reports on the console.  The line "@exit NN" at the end is
 * for tests/harness/runtest.py.
 */
#include "lib.h"

int	caught, caughtcode;
long	work();
char	buf[2048];

catch(sig, code, scp)
	int sig, code;
	struct sigcontext *scp;
{
	caught = sig;
	caughtcode = scp->sc_cs;
}

/*
 * For a fault: continue after the two-byte instruction that faulted.
 */
skip(sig, code, scp)
	int sig, code;
	struct sigcontext *scp;
{
	caught = sig;
	scp->sc_ip += 2;
}

main(argc, argv, envp)
	int argc;
	char **argv, **envp;
{
	int fd, pid, status, n, i, pfd[2];
	char *p, *q;
	long l;

	/* Nothing is open yet: the console becomes 0, 1 and 2. */
	fd = open("/dev/console", O_RDWR, 0);
	dup(fd);
	dup(fd);
	printf("init: started, pid %d, %d arguments\n", getpid(), argc);
	check(fd == 0, "the console opened as descriptor 0");
	check(getpid() == 1, "init is process 1");
	check(argc == 2 && strcmp(argv[0], "init") == 0,
	    "init was given its name");
	check(argv[1][0] == '-', "and its flags");
	check(envp[0] == 0, "and an empty environment");

	/* Files. */
	fd = open("/etc/motd", O_RDONLY, 0);
	check(fd == 3, "open a file");
	n = read(fd, buf, sizeof (buf));
	check(n > 0 && n < sizeof (buf), "read it");
	if (n > 0)
		write(1, buf, n);
	check(read(fd, buf, sizeof (buf)) == 0, "end of file");
	check(lseek(fd, 0L, 0) == 0L && read(fd, buf, 4) == 4, "seek and read again");
	check(close(fd) == 0, "close it");
	check(open("/no/such", O_RDONLY, 0) == -1 && errno == ENOENT,
	    "a file that is not there");
	check(read(17, buf, 1) == -1 && errno == EBADF, "a bad descriptor");
	fd = open("/etc/motd", O_RDONLY, 0);
	check(read(fd, (char *)0xFF00, 0) == 0, "nothing read into nowhere");
	check(read(fd, (char *)0xFFF0, 512) == -1 && errno == EFAULT,
	    "a read that would run past the data segment");
	close(fd);

	check(mkdir("/tmp", 0777) == 0, "make a directory");
	fd = open("/tmp/x", O_RDWR | O_CREAT | O_TRUNC, 0644);
	check(fd >= 0, "create a file");
	for (i = 0; i < sizeof (buf); i++)
		buf[i] = i * 7 + (i >> 8);
	for (i = 0; i < 20; i++)
		if (write(fd, buf, sizeof (buf)) != sizeof (buf))
			break;
	check(i == 20, "write 40K to it, through the indirect block");
	check(lseek(fd, 30000L, 0) == 30000L, "seek in it");
	n = read(fd, buf, 100);
	check(n == 100 && buf[0] == (char)((30000 % 2048) * 7 + ((30000 % 2048) >> 8)),
	    "and read back what was written");
	close(fd);
	check(link("/tmp/x", "/tmp/y") == 0, "link");
	check(unlink("/tmp/x") == 0, "unlink");
	check(open("/tmp/x", O_RDONLY, 0) == -1, "the old name is gone");
	check(unlink("/tmp/y") == 0, "unlink the other name");
	sync();

	/* Memory. */
	p = sbrk(0);
	q = sbrk(3000);
	check(p == q && sbrk(0) == p + 3000, "the break moves");
	for (i = 0; i < 3000 && p[i] == 0; i++)
		;
	check(i == 3000, "new memory is zero");
	for (i = 0; i < 3000; i++)
		p[i] = i;
	check(sbrk(20000) == p + 3000, "the data segment grows");
	check(p[2999] == (char)2999 && p[3000] == 0 && p[22999] == 0,
	    "and keeps what it held");
	check(sbrk(-23000) == p + 23000 && sbrk(0) == p, "the break moves back");
	q = sbrk(100);
	check(q == p && p[50] == 0, "memory given back and taken again is zero");
	sbrk(-100);

	/* Processes. */
	pid = fork();
	if (pid == 0) {
		check(getppid() == 1, "the child's parent is init");
		p = sbrk(0);
		sbrk(512);
		p[0] = 'c';			/* the child's own memory */
		exit(42);
	}
	check(pid > 1, "fork");
	n = wait4(pid, &status, 0, (char *)0);
	check(n == pid, "wait for the child");
	check((status & 0xFF) == 0 && (status >> 8 & 0xFF) == 42,
	    "its exit status");
	check(wait4(-1, &status, 0, (char *)0) == -1 && errno == ECHILD,
	    "no more children");

	/* A pipe from a child. */
	check(pipe(pfd) == 0, "pipe");
	pid = fork();
	if (pid == 0) {
		close(pfd[0]);
		for (i = 0; i < 10; i++)
			write(pfd[1], "0123456789", 10);
		exit(0);
	}
	close(pfd[1]);
	for (n = 0; (i = read(pfd[0], buf, 37)) > 0; n += i)
		;
	check(n == 100, "read what a child wrote to a pipe, to end of file");
	close(pfd[0]);
	wait4(pid, &status, 0, (char *)0);

	/* Another program, with arguments and an environment. */
	pid = fork();
	if (pid == 0) {
		static char *av[] = { "child", "one", "two words", 0 };
		static char *ev[] = { "HOME=/", "TERM=console", 0 };

		execve("/bin/child", av, ev);
		printf("init: exec failed, error %d\n", errno);
		exit(1);
	}
	wait4(pid, &status, 0, (char *)0);
	check(status == (7 << 8), "exec a program, which found its arguments");
	if (status != (7 << 8))
		printf("init: its status was %x\n", status);

	pid = vfork();
	if (pid == 0) {
		static char *av[] = { "child", "one", "two words", 0 };
		static char *ev[] = { "HOME=/", "TERM=console", 0 };

		execve("/bin/child", av, ev);
		_exit(1);
	}
	wait4(pid, &status, 0, (char *)0);
	check(status == (7 << 8), "the same after vfork");
	p = sbrk(0);
	check(sbrk(64) == p && p[0] == 0, "and the parent has its memory back");

	/* Signals. */
	check(signal(SIGUSR1, catch) == 0L, "catch a signal");
	check(kill(getpid(), SIGUSR1) == 0 && caught == SIGUSR1,
	    "a signal to oneself is taken at once");
	check(caughtcode == getcs(), "the handler saw where it interrupted");
	pid = fork();
	if (pid == 0) {
		for (;;)
			;			/* until killed */
	}
	check(kill(pid, SIGTERM) == 0, "signal a child that is computing");
	n = wait4(pid, &status, 0, (char *)0);
	check(n == pid && (status & 0x7F) == SIGTERM, "it died of the signal");
	pid = fork();
	if (pid == 0) {
		*(char *)0xFFFE = 1;		/* past the break */
		exit(0);
	}
	wait4(pid, &status, 0, (char *)0);
	check((status & 0x7F) == SIGSEGV, "a stray store is a segmentation violation");
	pid = fork();
	if (pid == 0) {
		signal(SIGSEGV, skip);
		caught = 0;
		outb();				/* a privileged instruction */
		exit(caught == SIGSEGV ? 3 : 4);
	}
	wait4(pid, &status, 0, (char *)0);
	check(status == (3 << 8), "a fault that is caught goes to its handler, which can step past it");

	/* Many processes at once, more than there are slots. */
	for (i = 0; i < 12; i++) {
		pid = fork();
		if (pid == 0) {
			long sum = 0;
			int j, k;

			for (j = 0; j < 100; j++) {
				for (k = 0; k < 3000; k++)
					sum += k ^ i;
				if (i & 1)
					getpid();
			}
			exit(i + (sum == 100L * work(i) ? 0 : 100));
		}
		if (pid < 0)
			break;
	}
	check(i == 12, "fork twelve children");
	n = 0;
	for (i = 0; i < 12; i++) {
		pid = wait4(-1, &status, 0, (char *)0);
		if (pid > 0 && (status & 0xFF) == 0 && (status >> 8) < 12)
			n |= 1 << (status >> 8);
	}
	check(n == 0xFFF, "each computed the right thing and exited with its number");

	hogs();
	disk();

	printf("init: %d checks, %d failed\n", checks, failed);
	printf("@exit %02x\n", failed > 255 ? 255 : failed);
	flush();
	close(0);			/* the last close of the console */
	close(1);			/*   waits for what was written */
	close(2);			/*   to it to go out */
	sync();
	reboot(8);				/* RB_HALT */
	for (;;)
		;
}

long
work(i)
	int i;
{
	long sum = 0;
	int k;

	for (k = 0; k < 3000; k++)
		sum += k ^ i;
	return (sum);
}

/*
 * More memory than the machine has: NHOG processes that each grow to
 * most of a full segment, fill it, and wait until all of them have, then
 * check that what they wrote is still there.  run.sh leaves the kernel
 * too little memory for them all, so some must be swapped out to let the
 * rest in, and back in to finish.
 */
#define	NHOG	24
#define	HOGSIZE	56000

hogs()
{
	int i, pid, status, gate[2], done[2];
	unsigned k;
	long n;
	char *m, c;

	pipe(gate);
	pipe(done);
	for (i = 0; i < NHOG; i++) {
		pid = fork();
		if (pid < 0)
			break;
		if (pid == 0) {
			close(gate[1]);
			close(done[0]);
			m = sbrk(HOGSIZE);
			if (m == (char *)-1)
				exit(101);
			for (k = 0; k < HOGSIZE; k += 61)
				m[k] = k + i;
			write(done[1], "x", 1);
			read(gate[0], &c, 1);		/* until all are ready */
			for (k = 0; k < HOGSIZE; k += 61)
				if (m[k] != (char)(k + i))
					exit(102);
			exit(i);
		}
	}
	check(i == NHOG, "fork processes with more memory in all than there is");
	close(gate[0]);
	close(done[1]);
	for (k = 0; k < i; k++)
		read(done[0], &c, 1);
	for (k = 0; k < i; k++)
		write(gate[1], "g", 1);
	n = 0;
	for (k = 0; k < i; k++) {
		pid = wait4(-1, &status, 0, (char *)0);
		if (pid > 0 && (status & 0xFF) == 0 && (status >> 8) < NHOG)
			n |= 1L << (status >> 8);
		else
			printf("init: process %d ended with status %x\n", pid, status);
	}
	check(n == (1L << NHOG) - 1, "each still had its memory after being swapped");
	if (n != (1L << NHOG) - 1)
		printf("init: of %d, those that finished: %lx\n", i, n);
	close(gate[1]);
	close(done[0]);
}

/*
 * A hard disk.  run.sh boots the kernel again from one that
 * tools/mkdisk.py made, with this file system in partition a, the swap
 * area in b and nothing in d, and names the driver in the file
 * /etc/disk: "wd" or "sd".
 */
#define	DIOCGDINFO	0x40D86465L	/* _IOR('d', 101, struct disklabel) */
#define	RAWSIZE		20480		/* more than the drive moves at once */

struct label {				/* the end of a struct disklabel */
	long	d_magic;
	char	d_other[110];
	int	d_npartitions;
	int	d_bbsize, d_sbsize;
	struct {
		long	p_size, p_offset;
		int	p_fsize;
		char	p_fstype, p_frag;
	} d_partitions[8];
} label;

disk()
{
	int raw, blk, fd, i;
	unsigned k;
	long size;
	char *m;

	static char rawd[] = "/dev/rXX0d", blkd[] = "/dev/XX0d";
	static char raw1[] = "/dev/rXX1a";

	if ((fd = open("/etc/disk", O_RDONLY, 0)) < 0) {
		check(open("/dev/rwd0d", O_RDWR, 0) == -1 && errno == ENXIO,
		    "there is no hard disk");
		check(open("/dev/rsd0d", O_RDWR, 0) == -1 && errno == ENXIO,
		    "and no SCSI adapter");
		return;
	}
	read(fd, rawd + 6, 2);
	close(fd);
	blkd[5] = raw1[6] = rawd[6];
	blkd[6] = raw1[7] = rawd[7];
	raw = open(rawd, O_RDWR, 0);
	check(raw >= 0, "open a partition of the hard disk");
	check(ioctl(raw, DIOCGDINFO, &label) == 0 &&
	    label.d_magic == 0x82564557L && label.d_npartitions == 4,
	    "its label");
	check(label.d_partitions[1].p_fstype == 1 &&
	    label.d_partitions[0].p_offset == 0 &&
	    label.d_partitions[3].p_offset ==
	    label.d_partitions[0].p_size + label.d_partitions[1].p_size,
	    "the label's partitions are those the disk was made with");
	size = label.d_partitions[3].p_size << 9;

	/*
	 * Straight between the disk and this process's memory, from an
	 * odd address, then through the kernel's buffers.
	 */
	m = sbrk(2 * RAWSIZE + 2);
	for (k = 0; k < RAWSIZE; k++)
		m[1 + k] = k * 3 + (k >> 8);
	check(write(raw, m + 1, RAWSIZE) == RAWSIZE, "write 20K to it");
	check(lseek(raw, 0L, 0) == 0L &&
	    read(raw, m + RAWSIZE + 1, RAWSIZE) == RAWSIZE, "read it back");
	for (k = 0; k < RAWSIZE; k++)
		if (m[RAWSIZE + 1 + k] != (char)(k * 3 + (k >> 8)))
			break;
	check(k == RAWSIZE, "and it is what was written");
	blk = open(blkd, O_RDWR, 0);
	check(blk >= 0 && lseek(blk, 5120L, 0) == 5120L &&
	    read(blk, m + RAWSIZE + 1, 3000) == 3000,
	    "read the partition as a block device");
	for (k = 0; k < 3000; k++)
		if (m[RAWSIZE + 1 + k] !=
		    (char)((k + 5120) * 3 + ((k + 5120) >> 8)))
			break;
	check(k == 3000, "and find the same");
	check(read(raw, m, 100) == -1, "part of a sector is refused");
	check(lseek(raw, size - 512, 0) == size - 512 &&
	    read(raw, m, 1024) == 512,
	    "a read across the partition's end is cut short");
	check(read(raw, m, 512) == 0, "and one at its end is end of file");
	check(lseek(raw, size - 512, 0) == size - 512 &&
	    write(raw, m, 1024) == 512 && write(raw, m, 512) <= 0,
	    "writes there likewise");
	close(raw);
	close(blk);
	sbrk(-(2 * RAWSIZE + 2));
	check(open(raw1, O_RDONLY, 0) == -1 && errno == ENXIO,
	    "there is no second drive");

	/* A file big enough to need the second level of indirect blocks. */
	fd = open("/tmp/big", O_RDWR|O_CREAT|O_TRUNC, 0644);
	for (i = 0; i < 150; i++) {
		for (k = 0; k < sizeof (buf); k++)
			buf[k] = k * 5 + i;
		if (write(fd, buf, sizeof (buf)) != sizeof (buf))
			break;
	}
	check(i == 150, "write a file of 300K");
	sync();
	lseek(fd, 0L, 0);
	for (i = 0; i < 150; i++) {
		if (read(fd, buf, sizeof (buf)) != sizeof (buf))
			break;
		for (k = 0; k < sizeof (buf); k++)
			if (buf[k] != (char)(k * 5 + i))
				break;
		if (k != sizeof (buf))
			break;
	}
	check(i == 150, "read it back");
	close(fd);
	check(link("/tmp/big", "/tmp/kept") == 0 && unlink("/tmp/big") == 0,
	    "rename it");

	/* What run.sh looks for on the disk afterwards. */
	fd = open("/tmp/big2", O_RDWR|O_CREAT|O_TRUNC, 0644);
	for (i = 0; i < 140; i++)
		write(fd, buf, sizeof (buf));
	close(fd);
	check(unlink("/tmp/big2") == 0, "remove another like it");
	fd = open("/persist", O_RDWR|O_CREAT|O_TRUNC, 0644);
	check(write(fd, "written by init, to be found on the disk\n", 41) == 41,
	    "leave a file for later");
	close(fd);
}
