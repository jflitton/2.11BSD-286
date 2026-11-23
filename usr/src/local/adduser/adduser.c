/*
 * adduser [name]: add a user to the system, asking for what it needs.
 *
 * This port's own (docs/userland.md).  2.11BSD has adduser(8) only as a
 * manual page that says how to do it by hand; this does those steps
 * with upstream's tools.  It asks for the login name, unless it is
 * given, the full name, the group, the shell, the home directory and
 * the uid, offering for each what most users would have: staff, the C
 * shell, /home/name and the lowest uid from 100 up that nobody has.
 * Then it has chpass -a add the entry, so that the password file is
 * locked and its database made again as vipw and passwd do; makes the
 * home directory with copies of the files in /usr/skel, all the new
 * user's; and runs passwd for the password.  The entry is added with
 * the password "*", which nothing matches, so that the account cannot
 * be used until passwd has given it one.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <utmp.h>

#define	SKEL		"/usr/skel"
#define	CHPASS		"/bin/chpass"
#define	PASSWD		"/bin/passwd"
#define	FIRSTUID	100

static char	*ask(char *, char *);
static int	 badname(char *);
static int	 knownshell(char *);
static int	 makedirs(char *);
static int	 copyskel(char *, int, int);
static int	 run(char *, char *, char *, char *);

int
main(int argc, char **argv)
{
	struct group *gr;
	struct stat st;
	char *name, *gecos, *group, *shell, *home, *p;
	char line[400], dflt[MAXPATHLEN];
	int uid, gid, i;

	if (getuid() != 0) {
		fprintf(stderr, "adduser: only root can add users\n");
		exit(1);
	}
	if (argc > 2) {
		fprintf(stderr, "usage: adduser [name]\n");
		exit(1);
	}
	(void)umask(022);

	for (name = argc == 2 ? argv[1] : NULL;; name = NULL) {
		if (name == NULL)
			name = ask("Login name", NULL);
		if (badname(name))
			fprintf(stderr, "adduser: a login name is 1 to %d "
			    "lower-case letters, digits, _ and -, starting "
			    "with a letter\n", UT_NAMESIZE);
		else if (getpwnam(name) != NULL)
			fprintf(stderr, "adduser: %s is a user already\n", name);
		else
			break;
	}
	for (;;) {
		gecos = ask("Full name", "");
		if (strpbrk(gecos, ":,") == NULL)
			break;
		fprintf(stderr, "adduser: no : or , in the full name\n");
	}
	for (;;) {
		group = ask("Group", "staff");
		gr = isdigit(*group) ? getgrgid(atoi(group)) : getgrnam(group);
		if (gr != NULL)
			break;
		fprintf(stderr, "adduser: no group %s in /etc/group\n", group);
	}
	gid = gr->gr_gid;
	for (;;) {
		shell = ask("Shell", "/bin/csh");
		if (knownshell(shell))
			break;
		fprintf(stderr, "adduser: %s is not in /etc/shells\n", shell);
	}
	(void)sprintf(dflt, "/home/%s", name);
	for (;;) {
		home = ask("Home directory", dflt);
		if (*home != '/' || strchr(home, ':') != NULL)
			fprintf(stderr, "adduser: a home directory is a full "
			    "path name, with no :\n");
		else if (stat(home, &st) == 0)
			fprintf(stderr, "adduser: %s is there already\n", home);
		else
			break;
	}
	for (uid = FIRSTUID; getpwuid(uid) != NULL; uid++)
		;
	(void)sprintf(dflt, "%d", uid);
	for (;;) {
		p = ask("Uid", dflt);
		for (i = 0; isdigit(p[i]); i++)
			;
		uid = atoi(p);
		if (i == 0 || p[i] != '\0' || i > 5 || atol(p) > 32767)
			fprintf(stderr, "adduser: a uid is a number from 0 to "
			    "32767\n");
		else if (getpwuid(uid) != NULL)
			fprintf(stderr, "adduser: uid %d is %s's\n", uid,
			    getpwuid(uid)->pw_name);
		else
			break;
	}

	(void)sprintf(line, "%s:*:%d:%d::0:0:%s:%s:%s", name, uid, gid,
	    gecos, home, shell);
	printf("\n%s\n", line);
	p = ask("Add this user", "y");
	if (*p != 'y' && *p != 'Y') {
		printf("Not added.\n");
		exit(1);
	}

	/*
	 * chpass, as vipw and passwd do, returns once it has written the
	 * new master.passwd, and leaves a process of its own to make the
	 * database again; the user is in the database when that is done.
	 */
	if (run(CHPASS, "chpass", "-a", line) != 0) {
		fprintf(stderr, "adduser: chpass failed; %s not added\n", name);
		exit(1);
	}
	for (i = 0; getpwnam(name) == NULL; i++) {
		if (i == 30) {
			fprintf(stderr, "adduser: %s is not in the password "
			    "database\n", name);
			exit(1);
		}
		sleep(1);
	}

	if (makedirs(home) || chown(home, uid, gid) < 0) {
		fprintf(stderr, "adduser: %s: %s\n", home, strerror(errno));
		exit(1);
	}
	if (copyskel(home, uid, gid))
		exit(1);
	printf("Added %s, uid %d, in %s.\n", name, uid, home);

	if (run(PASSWD, "passwd", name, NULL) != 0) {
		fprintf(stderr, "adduser: %s has no password yet; give it one "
		    "with passwd %s\n", name, name);
		exit(1);
	}
	exit(0);
}

/*
 * Ask for a value, offering DFLT if there is one, which an empty answer
 * takes.  The answer is in a buffer of its own.
 */
static char *
ask(char *prompt, char *dflt)
{
	char buf[200], *p;

	for (;;) {
		if (dflt != NULL && *dflt != '\0')
			printf("%s [%s]: ", prompt, dflt);
		else
			printf("%s: ", prompt);
		(void)fflush(stdout);
		if (fgets(buf, sizeof buf, stdin) == NULL) {
			printf("\n");
			exit(1);
		}
		if ((p = strchr(buf, '\n')) != NULL)
			*p = '\0';
		if (buf[0] == '\0' && dflt != NULL)
			p = strdup(dflt);
		else if (buf[0] == '\0')
			continue;
		else
			p = strdup(buf);
		if (p == NULL) {
			fprintf(stderr, "adduser: out of memory\n");
			exit(1);
		}
		return (p);
	}
}

static int
badname(char *name)
{
	char *p;

	if (strlen(name) > UT_NAMESIZE || !islower(*name))
		return (1);
	for (p = name; *p; p++)
		if (!islower(*p) && !isdigit(*p) && *p != '_' && *p != '-')
			return (1);
	return (0);
}

/* Is SHELL one of those in /etc/shells, which chsh and ftpd go by? */
static int
knownshell(char *shell)
{
	char *p;

	setusershell();
	while ((p = getusershell()) != NULL)
		if (strcmp(p, shell) == 0)
			break;
	endusershell();
	return (p != NULL);
}

/* Make the directory PATH, and those it is in that are not there. */
static int
makedirs(char *path)
{
	char *p;

	for (p = path + 1; (p = strchr(p, '/')) != NULL; p++) {
		*p = '\0';
		if (mkdir(path, 0755) < 0 && errno != EEXIST) {
			*p = '/';
			return (-1);
		}
		*p = '/';
	}
	return (mkdir(path, 0755));
}

/* Copy each file in /usr/skel to HOME, as the user's. */
static int
copyskel(char *home, int uid, int gid)
{
	DIR *dirp;
	struct direct *dp;
	char from[MAXPATHLEN], to[MAXPATHLEN], buf[512];
	int in, out, n;

	if ((dirp = opendir(SKEL)) == NULL)
		return (0);
	while ((dp = readdir(dirp)) != NULL) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;
		(void)sprintf(from, "%s/%s", SKEL, dp->d_name);
		(void)sprintf(to, "%s/%s", home, dp->d_name);
		if ((in = open(from, O_RDONLY)) < 0)
			continue;
		if ((out = open(to, O_WRONLY|O_CREAT|O_EXCL, 0644)) < 0) {
			fprintf(stderr, "adduser: %s: %s\n", to, strerror(errno));
			(void)close(in);
			continue;
		}
		while ((n = read(in, buf, sizeof buf)) > 0)
			if (write(out, buf, n) != n) {
				fprintf(stderr, "adduser: %s: %s\n", to,
				    strerror(errno));
				break;
			}
		(void)close(in);
		(void)close(out);
		(void)chown(to, uid, gid);
	}
	closedir(dirp);
	return (0);
}

/* Run PATH with up to two arguments; its exit status, or -1. */
static int
run(char *path, char *arg0, char *arg1, char *arg2)
{
	int pid, w, status;

	(void)fflush(stdout);
	if ((pid = fork()) < 0) {
		fprintf(stderr, "adduser: fork: %s\n", strerror(errno));
		return (-1);
	}
	if (pid == 0) {
		execl(path, arg0, arg1, arg2, (char *)NULL);
		fprintf(stderr, "adduser: %s: %s\n", path, strerror(errno));
		_exit(127);
	}
	while ((w = wait(&status)) != pid && w != -1)
		;
	if (w == -1 || (status & 0377) != 0)
		return (-1);
	return (status >> 8 & 0377);
}
