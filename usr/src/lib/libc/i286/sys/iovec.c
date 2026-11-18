/*
 * readv and writev.  To the kernel a struct iovec holds a far pointer,
 * whatever the program's model (docs/processes.md); a small-model
 * program's has a near one.
 */
#include <sys/types.h>
#include <sys/uio.h>
#include <errno.h>

#define	MAXIOV	16		/* as many as the kernel takes */

struct kiovec {
	char __far *kiov_base;
	int	kiov_len;
};

extern int errno;
extern int _readv(int, struct kiovec *, int);
extern int _writev(int, struct kiovec *, int);

static int
convert(struct kiovec *kv, const struct iovec *iov, int n)
{
	int k;

	if (n < 0 || n > MAXIOV) {
		errno = EINVAL;
		return (-1);
	}
	for (k = 0; k < n; k++) {
		kv[k].kiov_base = iov[k].iov_base;
		kv[k].kiov_len = iov[k].iov_len;
	}
	return (0);
}

int
readv(int fd, const struct iovec *iov, int n)
{
	struct kiovec kv[MAXIOV];

	if (convert(kv, iov, n) < 0)
		return (-1);
	return (_readv(fd, kv, n));
}

int
writev(int fd, const struct iovec *iov, int n)
{
	struct kiovec kv[MAXIOV];

	if (convert(kv, iov, n) < 0)
		return (-1);
	return (_writev(fd, kv, n));
}
