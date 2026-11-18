/*
 * Copyright (c) 1982, 1986, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)uio.h	8.5.2 (2.11BSD GTE) 12/20/94
 *
 * Copied from 4.4-Lite and modified for 2.11BSD.  Modifications were removal
 * of prototypes, limits for dynamic allocation of iovec structs and changing
 * uio_resid to u_short from int.
 */

#ifndef _SYS_UIO_H_
#define	_SYS_UIO_H_

/*
 * XXX
 * iov_base should be a void *.
 */
/*
 * The kernel's is always the first form: a program in a model with near
 * data pointers has its library pass the kernel far ones.
 */
struct iovec {
#ifdef KERNEL
	char __far *iov_base;	/* Base address. */
#else
	char	*iov_base;	/* Base address. */
#endif
	size_t	 iov_len;	/* Length. */
};

enum	uio_rw { UIO_READ, UIO_WRITE };

/* Segment flag values. */
enum uio_seg {
	UIO_USERSPACE,		/* from user data space */
	UIO_SYSSPACE,		/* from system space */
	UIO_USERISPACE		/* from user I space */
};

struct uio {
	struct	iovec *uio_iov;
	int	uio_iovcnt;
	off_t	uio_offset;
	u_short	uio_resid;
	enum	uio_seg uio_segflg;
	enum	uio_rw uio_rw;
};
#ifdef KERNEL
struct inode;
#ifdef SUPERVISOR
/* the network's own, for the kernel's uio (i286/net_sys.c) */
int	uiomove(caddr_t cp, u_int n, struct uio KSPACE *uio);
#else
int	uiomove(void __far *cp, u_int n, struct uio *uio);
#endif
int	rdwri(enum uio_rw rw, struct inode *ip, void __far *base, int len,
	    off_t offset, enum uio_seg segflg, int ioflg, int *aresid);
#endif

#endif /* !_SYS_UIO_H_ */
