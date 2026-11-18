/*
 * Memory special files: minor device 0 is physical memory, 1 the
 * kernel's data segment, 2 the null device and 3 a source of zeros.
 */
#include "param.h"
#include "user.h"
#include "conf.h"
#include "uio.h"

mmrw(dev, uio, flag)
	dev_t dev;
	register struct uio *uio;
	int flag;
{
	register struct iovec *iov;
	register u_int c;
	int error = 0;
	char	buf[256];

	if (minor(dev) == 3)
		bzero(buf, sizeof (buf));
	while (uio->uio_resid && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}
		switch (minor(dev)) {

/* minor device 0 is physical memory (/dev/mem) */
		case 0:
			c = MIN(iov->iov_len, sizeof (buf));
			if (uio->uio_rw == UIO_READ) {
				pcopy(uio->uio_offset, fartophys(buf), c);
				error = uiomove(buf, c, uio);
			} else {
				long off = uio->uio_offset;

				error = uiomove(buf, c, uio);
				if (error == 0)
					pcopy(fartophys(buf), off, c);
			}
			break;

/* minor device 1 is kernel memory (/dev/kmem) */
		case 1:
			c = iov->iov_len;
			if (uio->uio_offset >= 0x10000L)
				return (EFAULT);
			if (c > 0x10000L - uio->uio_offset)
				c = 0x10000L - uio->uio_offset;
			error = uiomove((caddr_t)(u_int)uio->uio_offset, c,
			    uio);
			break;

/* minor device 2 is EOF/RATHOLE (/dev/null) */
		case 2:
			if (uio->uio_rw == UIO_READ)
				return (0);
			c = iov->iov_len;
			iov->iov_base += c;
			iov->iov_len -= c;
			uio->uio_offset += c;
			uio->uio_resid -= c;
			break;

/* minor device 3 is ZERO (/dev/zero) */
		case 3:
			if (uio->uio_rw == UIO_WRITE)
				return (EIO);
			c = MIN(iov->iov_len, sizeof (buf));
			error = uiomove(buf, c, uio);
			break;

		default:
			return (EINVAL);
		} /* switch */
	} /* while */
	return (error);
}
