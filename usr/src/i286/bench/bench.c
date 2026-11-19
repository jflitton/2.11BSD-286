/*
 * bench [-m] [-s scale] [-u users] [-l account] [test ...]: time the
 * things a system does most.
 *
 * This port's own (docs/performance.md).  2.11BSD has no benchmark.
 * Each test does a fixed amount of one kind of work and says how long
 * it took, by the system's clock, how much of that the processor spent
 * in the program and its children and how much in the kernel for them,
 * and what one piece of the work comes to: microseconds each, or
 * kilobytes a second.  The work is the same every time, so two runs
 * differ only as the system or the machine does.  With no test named,
 * all are run, in the table's order; -s does scale times the work, for
 * a machine too quick for the clock, which ticks 60 times a second.
 *
 * The last two tests are users who came in by telnet, -u of them at
 * once and 4 if it is not given, logged in to the account -l names,
 * root if it is not given: an account with no password, whose shell is
 * sh.
 *
 * With -m each test writes "@@" as it starts its work and as it ends
 * it, for a harness that times from outside the machine
 * (tests/harness/runtest.py --mark): before the first it waits until
 * the terminal has sent what was written, so that both marks leave as
 * they are written.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/telnet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	MARK	"@@"

static int	 marks;			/* -m */
static int	 bad;			/* a test failed */
static char	*self;			/* this program, for exec */
static char	*name;			/* the test that is running */
static char	 buf[4096];
static char	 file[40];		/* a file of the test's, in /tmp */
static int	 users = 4;		/* -u */
static char	*account = "root";	/* -l */

static struct timeval	t0;
static long		u0, s0;		/* processor time, ms, at the start */

/*
 * The functions are not static, so that a profile of the system can name
 * them (tools/kprof.py).
 */
void	 cputime(long *, long *);
void	 begin(void);
void	 finish(long, int);
int	 failed(char *);
void	 reap(int);
int	 writeall(int, long);
int	 listener(int *);
int	 dial(int);
int	 tnproto(int);
int	 tnread(void);
int	 tnwait(char **);
int	 tnfail(char *);
int	 tnlogin(void);
int	 tnkeys(long);
int	 tncmds(long);
void	 tnlate(int);
void	 sessions(long, long, int (*)(long));

/*
 * The time the processor has spent in this process and the children it
 * has waited for, in milliseconds: in the programs, and in the kernel.
 */
void
cputime(long *user, long *sys)
{
	struct rusage me, kids;

	(void)getrusage(RUSAGE_SELF, &me);
	(void)getrusage(RUSAGE_CHILDREN, &kids);
	*user = (me.ru_utime.tv_sec + kids.ru_utime.tv_sec) * 1000 +
	    (me.ru_utime.tv_usec + kids.ru_utime.tv_usec) / 1000;
	*sys = (me.ru_stime.tv_sec + kids.ru_stime.tv_sec) * 1000 +
	    (me.ru_stime.tv_usec + kids.ru_stime.tv_usec) / 1000;
}

/* The work starts here. */
void
begin(void)
{
	int n;

	printf("%-8s ", name);
	(void)fflush(stdout);
	if (marks) {
		while (ioctl(1, TIOCOUTQ, (char *)&n) == 0 && n > 0)
			;
		(void)write(1, MARK, sizeof (MARK) - 1);
	}
	cputime(&u0, &s0);
	(void)gettimeofday(&t0, (struct timezone *)0);
}

/*
 * And ends here: n pieces of it were done, which are kilobytes moved if
 * rate is set.
 */
void
finish(long n, int rate)
{
	struct timeval t1;
	long ms, u1, s1, each;

	(void)gettimeofday(&t1, (struct timezone *)0);
	if (marks)
		(void)write(1, MARK, sizeof (MARK) - 1);
	cputime(&u1, &s1);
	ms = (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_usec - t0.tv_usec) / 1000;
	u1 -= u0;
	s1 -= s0;
	if (ms <= 0)
		ms = 1;
	if (rate)
		each = n * 1000 / ms;
	else if (ms < 2000000L)
		each = ms * 1000 / n;
	else
		each = ms / n * 1000;
	printf("%5ld.%02ld %4ld.%02ld %4ld.%02ld %7ld %8ld %s\n",
	    ms / 1000, ms % 1000 / 10, u1 / 1000, u1 % 1000 / 10,
	    s1 / 1000, s1 % 1000 / 10, n, each, rate ? "K/s" : "us");
}

/* A test could not be done, for the reason in errno. */
int
failed(char *what)
{
	(void)fflush(stdout);
	fprintf(stderr, "bench: %s: %s: %s\n", name, what, strerror(errno));
	bad = 1;
	return (-1);
}

/* Wait for the child pid. */
void
reap(int pid)
{
	int w, status;

	while ((w = wait(&status)) != pid && w != -1)
		;
}

/* Write k kilobytes to fd, a buffer at a time. */
int
writeall(int fd, long k)
{
	long left;
	int n, done;

	for (left = k * 1024; left > 0; left -= done) {
		n = left < sizeof (buf) ? (int)left : sizeof (buf);
		if ((done = write(fd, buf, n)) <= 0)
			return (-1);
	}
	return (0);
}

/*
 * The processor, in the program's own code: the sieve of Eratosthenes
 * that BYTE printed in September 1981, which finds the 1899 primes it
 * knows of below 16384, n times.
 */
#define	SIZE	8190
static char	flags[SIZE + 1];

void
sieve(long n)
{
	register int i, k;
	int prime, count = 0;
	long iter;

	begin();
	for (iter = 0; iter < n; iter++) {
		count = 0;
		for (i = 0; i <= SIZE; i++)
			flags[i] = 1;
		for (i = 0; i <= SIZE; i++) {
			if (flags[i]) {
				prime = i + i + 3;
				for (k = i + prime; k <= SIZE; k += prime)
					flags[k] = 0;
				count++;
			}
		}
	}
	finish(n, 0);
	if (count != 1899) {
		fprintf(stderr, "bench: sieve: found %d primes\n", count);
		bad = 1;
	}
}

/*
 * 32-bit arithmetic, which this processor does a half at a time and
 * the compiler's library divides: a multiplication, a division and a
 * remainder each time round.
 */
static unsigned long	sink;

void
longs(long n)
{
	register unsigned long x = 1;
	unsigned long y = 0;
	long i;

	begin();
	for (i = 0; i < n; i++) {
		x = x * 1103515245L + 12345;
		y += (x >> 8) % 1000 + x / 77777L;
	}
	finish(n, 0);
	sink = y;
}

/* The way into the kernel and out: the system call that does least. */
void
syscalls(long n)
{
	long i;

	begin();
	for (i = 0; i < n; i++)
		(void)getpid();
	finish(n, 0);
}

/* A system call that goes through a file: a byte to /dev/null. */
void
nullwrite(long n)
{
	long i;
	int fd;

	if ((fd = open("/dev/null", O_WRONLY)) < 0) {
		(void)failed("/dev/null");
		return;
	}
	begin();
	for (i = 0; i < n; i++)
		(void)write(fd, buf, 1);
	finish(n, 0);
	(void)close(fd);
}

/* Looking a name up: three directories deep. */
void
lookup(long n)
{
	struct stat st;
	long i;

	begin();
	for (i = 0; i < n; i++)
		if (stat("/usr/libexec/telnetd", &st) < 0) {
			(void)failed("/usr/libexec/telnetd");
			return;
		}
	finish(n, 0);
}

/*
 * Switching between processes: a byte goes to a child by one pipe and
 * comes back by another, so each round is two switches and four small
 * reads and writes.
 */
void
pingpong(long n)
{
	int there[2], back[2], pid;
	char c = 0;
	long i;

	if (pipe(there) < 0 || pipe(back) < 0 || (pid = fork()) < 0) {
		(void)failed("pipe or fork");
		return;
	}
	if (pid == 0) {
		(void)close(there[1]);
		(void)close(back[0]);
		while (read(there[0], &c, 1) == 1)
			(void)write(back[1], &c, 1);
		_exit(0);
	}
	(void)close(there[0]);
	(void)close(back[1]);
	begin();
	for (i = 0; i < n; i++)
		if (write(there[1], &c, 1) != 1 || read(back[0], &c, 1) != 1) {
			(void)failed("pipe");
			break;
		}
	if (i == n)
		finish(n, 0);
	(void)close(there[1]);
	(void)close(back[0]);
	reap(pid);
}

/* Moving data between processes: n kilobytes through a pipe. */
void
pipeio(long n)
{
	int p[2], pid, ok = 1;

	if (pipe(p) < 0 || (pid = fork()) < 0) {
		(void)failed("pipe or fork");
		return;
	}
	if (pid == 0) {
		(void)close(p[1]);
		while (read(p[0], buf, sizeof (buf)) > 0)
			;
		_exit(0);
	}
	(void)close(p[0]);
	begin();
	if (writeall(p[1], n) < 0)
		ok = failed("pipe") == 0;
	(void)close(p[1]);
	reap(pid);
	if (ok)
		finish(n, 1);
}

/* Making a process and ending it: this one, copied. */
void
forks(long n)
{
	long i;
	int pid;

	begin();
	for (i = 0; i < n; i++) {
		if ((pid = fork()) < 0) {
			(void)failed("fork");
			return;
		}
		if (pid == 0)
			_exit(0);
		reap(pid);
	}
	finish(n, 0);
}

/* Running a program: this one, which with -x does nothing. */
void
execs(long n)
{
	long i;
	int pid;

	begin();
	for (i = 0; i < n; i++) {
		if ((pid = fork()) < 0) {
			(void)failed("fork");
			return;
		}
		if (pid == 0) {
			execl(self, "bench", "-x", (char *)0);
			_exit(1);
		}
		reap(pid);
	}
	finish(n, 0);
}

/*
 * A command as a user types one: the shell is started and runs two
 * programs with a pipe between them.
 */
void
shell(long n)
{
	long i;
	int pid;

	begin();
	for (i = 0; i < n; i++) {
		if ((pid = fork()) < 0) {
			(void)failed("fork");
			return;
		}
		if (pid == 0) {
			execl("/bin/sh", "sh", "-c",
			    "ls /bin | wc -l >/dev/null", (char *)0);
			_exit(1);
		}
		reap(pid);
	}
	finish(n, 0);
}

/* Making a file and removing it. */
void
creats(long n)
{
	long i;
	int fd;

	begin();
	for (i = 0; i < n; i++) {
		if ((fd = creat(file, 0600)) < 0) {
			(void)failed(file);
			return;
		}
		(void)close(fd);
		(void)unlink(file);
	}
	finish(n, 0);
}

/* Writing a file of n kilobytes, until the disk has it. */
void
fswrite(long n)
{
	int fd;

	if ((fd = creat(file, 0600)) < 0) {
		(void)failed(file);
		return;
	}
	begin();
	if (writeall(fd, n) < 0 || fsync(fd) < 0)
		(void)failed(file);
	else
		finish(n, 1);
	(void)close(fd);
	(void)unlink(file);
}

/*
 * Reading a file of n kilobytes from one end to the other.  It has just
 * been written, so what of it the kernel's buffers have room for comes
 * from them, the end of it.
 */
void
fsread(long n)
{
	int fd, r;

	if ((fd = creat(file, 0600)) < 0 || writeall(fd, n) < 0 ||
	    close(fd) < 0 || (fd = open(file, O_RDONLY)) < 0) {
		(void)failed(file);
		(void)unlink(file);
		return;
	}
	sync();
	begin();
	while ((r = read(fd, buf, sizeof (buf))) > 0)
		;
	if (r < 0)
		(void)failed(file);
	else
		finish(n, 1);
	(void)close(fd);
	(void)unlink(file);
}

/*
 * What a program writes to a user who came in by the network goes
 * through a terminal's output and out of a pseudo-terminal, where the
 * server reads it: n kilobytes of lines to a pseudo-terminal, and the
 * other side of it read here.
 */
void
pty(long n)
{
	static char line[] = "/dev/ptyXX";
	char *x, *y;
	long lines, seen = 0;
	int master = -1, slave, pid, r, i;

	for (x = "pqrs"; *x && master < 0; x++)
		for (y = "0123456789abcdef"; *y && master < 0; y++) {
			line[8] = *x;
			line[9] = *y;
			master = open(line, O_RDWR);
		}
	if (master < 0) {
		(void)failed("no pseudo-terminal");
		return;
	}
	line[5] = 't';
	lines = n * 1024 / 64;
	(void)memset(buf, 'x', sizeof (buf));
	for (i = 63; i < sizeof (buf); i += 64)
		buf[i] = '\n';
	if ((pid = fork()) < 0) {
		(void)failed("fork");
		(void)close(master);
		return;
	}
	if (pid == 0) {
		(void)close(master);
		if ((slave = open(line, O_RDWR)) < 0)
			_exit(1);
		(void)writeall(slave, n);
		_exit(0);
	}
	begin();
	while (seen < lines && (r = read(master, buf, sizeof (buf))) > 0)
		for (i = 0; i < r; i++)
			if (buf[i] == '\n')
				seen++;
	if (seen < lines) {
		errno = EIO;
		(void)failed(line);
	} else
		finish(n, 1);
	(void)close(master);
	reap(pid);
	line[5] = 'p';
}

/* A socket that listens on a port of this machine's own address. */
int
listener(int *port)
{
	struct sockaddr_in sin;
	int s, len = sizeof (sin);

	(void)memset((char *)&sin, 0, sizeof (sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(0x7f000001L);
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return (failed("socket"));
	if (bind(s, (struct sockaddr *)&sin, sizeof (sin)) < 0 ||
	    listen(s, 1) < 0 ||
	    getsockname(s, (struct sockaddr *)&sin, &len) < 0) {
		(void)close(s);
		return (failed("bind"));
	}
	*port = sin.sin_port;
	return (s);
}

/* And one connected to it. */
int
dial(int port)
{
	struct sockaddr_in sin;
	int s;

	(void)memset((char *)&sin, 0, sizeof (sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(0x7f000001L);
	sin.sin_port = port;
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return (failed("socket"));
	if (connect(s, (struct sockaddr *)&sin, sizeof (sin)) < 0) {
		(void)close(s);
		return (failed("connect"));
	}
	return (s);
}

/* TCP: n kilobytes to a process on this machine. */
void
tcp(long n)
{
	int l, s, pid, port, ok = 1;

	if ((l = listener(&port)) < 0)
		return;
	if ((pid = fork()) < 0) {
		(void)failed("fork");
		(void)close(l);
		return;
	}
	if (pid == 0) {
		if ((s = accept(l, (struct sockaddr *)0, (int *)0)) < 0)
			_exit(1);
		while (read(s, buf, sizeof (buf)) > 0)
			;
		_exit(0);
	}
	(void)close(l);
	if ((s = dial(port)) >= 0) {
		(void)memset(buf, 'x', sizeof (buf));
		begin();
		if (writeall(s, n) < 0)
			ok = failed("write") == 0;
		(void)close(s);
	}
	reap(pid);
	if (s >= 0 && ok)
		finish(n, 1);
}

/*
 * A key typed by a user on the network and its echo: a byte goes over
 * TCP to a process on this machine, and one comes back.
 */
void
tcprr(long n)
{
	int l, s, pid, port;
	char c = 0;
	long i;

	if ((l = listener(&port)) < 0)
		return;
	if ((pid = fork()) < 0) {
		(void)failed("fork");
		(void)close(l);
		return;
	}
	if (pid == 0) {
		if ((s = accept(l, (struct sockaddr *)0, (int *)0)) < 0)
			_exit(1);
		while (read(s, &c, 1) == 1)
			(void)write(s, &c, 1);
		_exit(0);
	}
	(void)close(l);
	if ((s = dial(port)) >= 0) {
		begin();
		for (i = 0; i < n; i++)
			if (write(s, &c, 1) != 1 || read(s, &c, 1) != 1) {
				(void)failed("connection");
				break;
			}
		if (i == n)
			finish(n, 0);
		(void)close(s);
	}
	reap(pid);
}

/*
 * Users on the network.  One who comes in by telnet has the server,
 * telnetd, between the connection and a pseudo-terminal, and a shell at
 * the terminal, so a key goes through TCP, the server and the terminal's
 * input, and its echo back through the terminal's output, the server
 * and TCP; and there are several users, whose servers and shells take
 * turns.  Here each user is a process that connects to this machine's
 * own telnet server, logs in and types, as fast as the system answers.
 * So the users cost the machine what their own machines would have
 * done, and nothing goes through a network interface.
 *
 * What follows is one user's, in a process of its own.
 */
#define	MAXUSERS	8
#define	NSEEN		16

static int	 tn;			/* the connection */
static int	 tnready;		/* where to tell bench how it went */
static char	 in[512];		/* what the server sent last */
static char	 seen[NSEEN];		/* the end of all it has sent */
static int	 tnstate;		/* in its protocol: 0, IAC, WILL, SB, SE */

static char	*atlogin[] = { "login: ", 0 };
static char	*atshell[] = { "# ", "$ ", "Password:", "\nlogin: ", 0 };
static char	*atprompt[] = { "\nbench:", 0 };

/* A line a user types that the shell does nothing for: with its end, 32 keys. */
static char	 typed[] = ": abcdefghijklmnopqrstuvwxyz012";

/*
 * Take telnet's protocol out of the n bytes in in, and say how many are
 * left.  What it asks is answered: the server may echo and need send no
 * go-aheads, and nothing else is agreed to, so the terminal has no type
 * and no size.
 */
int
tnproto(int n)
{
	static int cmd;
	register int state = tnstate;
	unsigned char say[3];
	register char *p, *q;
	int c;

	for (p = q = in; n > 0; n--) {
		c = *p++ & 0377;
		switch (state) {
		case 0:
			if (c == IAC)
				state = IAC;
			else
				*q++ = c;
			break;
		case IAC:
			state = 0;
			if (c == IAC)
				*q++ = c;
			else if (c == SB)
				state = SB;
			else if (c >= WILL)
				cmd = c, state = WILL;
			break;
		case WILL:
			state = 0;
			if (cmd == WONT || cmd == DONT)
				break;
			say[0] = IAC;
			if (cmd == DO)
				say[1] = WONT;
			else
				say[1] = c == TELOPT_ECHO || c == TELOPT_SGA ?
				    DO : DONT;
			say[2] = c;
			(void)write(tn, (char *)say, 3);
			break;
		case SB:
			if (c == IAC)
				state = SE;
			break;
		case SE:
			state = c == SE ? 0 : SB;
			break;
		}
	}
	tnstate = state;
	return (q - in);
}

/*
 * Read what the server has sent, as much as there is: how many bytes a
 * user would see of it, which are in in, or -1 if there will be no
 * more.  It is looked at a piece at a time and not a byte, so that the
 * user costs the machine little.
 */
int
tnread(void)
{
	register int i;
	int n;

	if ((n = read(tn, in, sizeof (in))) <= 0)
		return (-1);
	if (tnstate || memchr(in, (char)IAC, n) != 0)
		n = tnproto(n);
	if (n >= NSEEN)
		(void)memcpy(seen, in + n - NSEEN, NSEEN);
	else if (n > 0) {
		for (i = 0; i < NSEEN - n; i++)
			seen[i] = seen[i + n];
		(void)memcpy(seen + NSEEN - n, in, n);
	}
	return (n);
}

/*
 * Read until all that has come ends with one of the texts, and say
 * which, or -1 if the connection ends first.  The server stops sending
 * where it waits for the user, so that is where a piece ends.
 */
int
tnwait(char **texts)
{
	register char *s;
	int i, n;

	for (;;) {
		if (tnread() < 0)
			return (-1);
		for (i = 0; (s = texts[i]) != 0; i++) {
			n = strlen(s);
			if (memcmp(seen + NSEEN - n, s, n) == 0)
				return (i);
		}
	}
}

/* This user could not do its part, which bench is told. */
int
tnfail(char *what)
{
	fprintf(stderr, "bench: %s: %s\n", name, what);
	(void)write(tnready, "x", 1);
	_exit(1);
	return (-1);
}

/* Nor if the server does not answer. */
void
tnlate(int sig)
{
	(void)tnfail("no answer from the telnet server");
}

/*
 * Log in, and have the shell prompt with what no output ends with.
 */
int
tnlogin(void)
{
	if ((tn = dial(htons(23))) < 0)
		return (tnfail("no telnet server"));
	if (tnwait(atlogin) < 0)
		return (tnfail("no login:"));
	(void)write(tn, account, strlen(account));
	(void)write(tn, "\r\n", 2);
	switch (tnwait(atshell)) {
	case 0:
	case 1:
		break;
	case 2:
		return (tnfail("the account has a password: see -l"));
	case 3:
		return (tnfail("the account cannot log in: see -l"));
	default:
		return (tnfail("the connection ended at login"));
	}
	(void)write(tn, "PS1=bench:\r\n", 12);
	if (tnwait(atprompt) < 0)
		return (tnfail("no prompt from the shell"));
	return (0);
}

/*
 * Type n lines a key at a time, each key when the one before has been
 * echoed, as the fastest of typists would.
 */
int
tnkeys(long n)
{
	register char *p;
	long i;

	for (i = 0; i < n; i++) {
		for (p = typed; *p; p++)
			if (write(tn, p, 1) != 1 || tnread() != 1 || in[0] != *p)
				return (tnfail("a key was not echoed"));
		if (write(tn, "\r\n", 2) != 2 || tnwait(atprompt) < 0)
			return (tnfail("no prompt after a line"));
	}
	return (0);
}

/* Run n commands, each cat of the test's file, and read what they write. */
int
tncmds(long n)
{
	char cmd[sizeof (file) + 8];
	long i;
	int len;

	(void)sprintf(cmd, "cat %s\r\n", file);
	len = strlen(cmd);
	for (i = 0; i < n; i++)
		if (write(tn, cmd, len) != len || tnwait(atprompt) < 0)
			return (tnfail("no prompt after a command"));
	return (0);
}

/*
 * The users: each logs in, and when all have, each does the work and
 * the time is theirs together.  They log out when it has been taken,
 * and are waited for, so the next test starts with their servers gone.
 * There were count pieces of work between them, each given per.
 */
void
sessions(long count, long per, int (*work)(long))
{
	int ready[2], go[2], pid[MAXUSERS], i, ok = 1;
	char c;

	if (pipe(ready) < 0 || pipe(go) < 0) {
		(void)failed("pipe");
		return;
	}
	for (i = 0; i < users; i++) {
		if ((pid[i] = fork()) < 0) {
			(void)failed("fork");
			break;
		}
		if (pid[i] == 0) {
			(void)close(ready[0]);
			(void)close(go[1]);
			tnready = ready[1];
			(void)signal(SIGALRM, tnlate);
			(void)alarm(120);
			(void)tnlogin();
			(void)alarm(0);
			(void)write(tnready, "r", 1);
			(void)read(go[0], &c, 1);
			(void)(*work)(per);
			(void)write(tnready, "d", 1);
			(void)alarm(60);
			(void)write(tn, "\004", 1);
			while (tnread() >= 0)
				;
			_exit(0);
		}
	}
	(void)close(ready[1]);
	(void)close(go[0]);
	if (i < users)
		ok = 0;
	for (i = 0; i < users && ok; i++)
		ok = read(ready[0], &c, 1) == 1 && c == 'r';
	if (ok) {
		begin();
		(void)close(go[1]);
		for (i = 0; i < users && ok; i++)
			ok = read(ready[0], &c, 1) == 1 && c == 'd';
		if (ok)
			finish(count, 0);
	}
	if (!ok) {
		/* The user that could not go on has said why. */
		(void)fflush(stdout);
		bad = 1;
		for (i = 0; i < users; i++)
			if (pid[i] > 0)
				(void)kill(pid[i], SIGTERM);
		(void)close(go[1]);
	}
	(void)close(ready[0]);
	while (wait((int *)0) != -1)
		;
}

/*
 * Typing: n lines from each user, a key and its echo at a time.  The
 * shell has a line to read at the end of each, and does nothing for it.
 */
void
telkey(long n)
{
	sessions(n * users * sizeof (typed), n, tnkeys);
}

/*
 * Commands: n from each user, each cat of a screenful, 24 lines of 80.
 * So the shell makes a process that runs a program, and what the
 * program writes goes out through the terminal, the server and TCP.
 */
void
telcmd(long n)
{
	int fd, i;

	(void)memset(buf, 'x', 24 * 79);
	for (i = 78; i < 24 * 79; i += 79)
		buf[i] = '\n';
	if ((fd = creat(file, 0644)) < 0 || write(fd, buf, 24 * 79) != 24 * 79 ||
	    close(fd) < 0) {
		(void)failed(file);
		(void)unlink(file);
		return;
	}
	sessions(n * users, n, tncmds);
	(void)unlink(file);
}

/*
 * The tests, and how much each does: enough to take a few seconds on a
 * 10 MHz 286.
 */
static struct test {
	char	*name;
	void	(*fun)(long);
	long	n;
	char	*what;
} tests[] = {
	{ "sieve",	sieve,		40,	"the primes below 16384, by sieve" },
	{ "long",	longs,		20000,	"32-bit multiply, divide, remainder" },
	{ "getpid",	syscalls,	20000,	"the least system call" },
	{ "write",	nullwrite,	10000,	"a byte to /dev/null" },
	{ "stat",	lookup,		2000,	"stat of /usr/libexec/telnetd" },
	{ "pipe",	pingpong,	500,	"a byte to another process and back" },
	{ "pipeio",	pipeio,		2048,	"kilobytes through a pipe" },
	{ "fork",	forks,		300,	"fork, exit and wait" },
	{ "exec",	execs,		100,	"fork, exec, exit and wait" },
	{ "shell",	shell,		10,	"sh -c \"ls /bin | wc -l\"" },
	{ "creat",	creats,		100,	"create a file and remove it" },
	{ "fswrite",	fswrite,	1024,	"kilobytes written to a file" },
	{ "fsread",	fsread,		1024,	"kilobytes read from a file" },
	{ "pty",	pty,		16,	"kilobytes through a pseudo-terminal" },
	{ "tcp",	tcp,		256,	"kilobytes over TCP to this machine" },
	{ "tcprr",	tcprr,		500,	"a byte over TCP and one back" },
	{ "telkey",	telkey,		5,	"lines typed by each user on telnet" },
	{ "telcmd",	telcmd,		10,	"commands by each user on telnet" },
	{ 0 }
};

void
usage(void)
{
	struct test *t;

	fprintf(stderr,
	    "usage: bench [-m] [-s scale] [-u users] [-l account] [test ...]\n");
	for (t = tests; t->name; t++)
		fprintf(stderr, "\t%-8s %s, %ld\n", t->name, t->what, t->n);
	exit(2);
}

int
main(int argc, char **argv)
{
	struct test *t;
	char *av0 = argv[0];
	long scale = 1;
	int ch, i;

	while ((ch = getopt(argc, argv, "l:ms:u:x")) != EOF)
		switch (ch) {
		case 'l':
			account = optarg;
			break;
		case 'm':
			marks = 1;
			break;
		case 's':
			if ((scale = atol(optarg)) < 1)
				usage();
			break;
		case 'u':
			users = atoi(optarg);
			if (users < 1 || users > MAXUSERS)
				usage();
			break;
		case 'x':		/* the exec test's program */
			exit(0);
		default:
			usage();
		}
	argc -= optind;
	argv += optind;
	for (i = 0; i < argc; i++) {
		for (t = tests; t->name && strcmp(t->name, argv[i]); t++)
			;
		if (t->name == 0)
			usage();
	}
	self = strchr(av0, '/') ? av0 : "/usr/bin/bench";
	(void)sprintf(file, "/tmp/bench%d", getpid());
	(void)signal(SIGPIPE, SIG_IGN);
	printf("%-8s %s    real    user     sys   count     each\n", "test",
	    marks ? "    " : "");
	for (t = tests; t->name; t++) {
		for (i = 0; i < argc && strcmp(t->name, argv[i]); i++)
			;
		if (argc && i == argc)
			continue;
		name = t->name;
		(*t->fun)(t->n * scale);
		(void)fflush(stdout);
	}
	exit(bad);
}
