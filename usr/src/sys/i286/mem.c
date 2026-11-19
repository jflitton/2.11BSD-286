/*
 * Memory special files: minor device 0 is physical memory, 1 the
 * kernel's memory, 2 the null device and 3 a source of zeros.
 *
 * An offset in the kernel's memory is a segment of the kernel and an
 * offset in it, as a far pointer is: the segment, counting from 1 as in
 * the kernel's executable, in the high 16 bits, and the offset in the
 * low.  nlist gives a symbol's value so (n_ovly the segment, n_value the
 * offset).  Segment 0 is the kernel's data segment, so that an offset
 * below 64K is an address in it, as on the PDP-11.
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
		case 1: {
			u_int seg = (u_int)(uio->uio_offset >> 16);
			u_int off = (u_int)uio->uio_offset;
			register struct segdesc *sd;
			char __far *kp;
			long lim;

			if (uio->uio_offset < 0 || seg > NKSEG)
				return (EFAULT);
			if (seg == 0) {
				kp = (caddr_t)off;
				lim = 0x10000L;
			} else {
				sd = &gdt[GDT_KERNEL + seg - 1];
				if ((sd->sd_access & SD_P) == 0)
					return (EFAULT);
				/* code is not to be written */
				if (uio->uio_rw == UIO_WRITE &&
				    (sd->sd_access & SD_EXEC))
					return (EFAULT);
				kp = UADDR(SEL(GDT_KERNEL + seg - 1), off);
				lim = (long)sd->sd_limit + 1;
			}
			if (off >= lim)
				return (EFAULT);
			c = iov->iov_len;
			if (c > lim - off)
				c = lim - off;
			error = uiomove(kp, c, uio);
			break;
		}

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
