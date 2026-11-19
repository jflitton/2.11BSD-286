/*
 * The Internet checksum (RFC 1071) over the first len bytes of a chain of
 * mbufs: the ones' complement of the ones' complement sum of the data as
 * 16-bit words.  On a little-endian machine the sum of the words as the
 * processor loads them is the byte-swapped sum of them in network order,
 * so the result, stored back the same way, is right as it stands.
 *
 * An mbuf may hold an odd number of bytes; its last one is the low half
 * of a word whose high half is the next mbuf's first.
 *
 * This is the reference C.  The kernel is built with the assembly
 * version, mch_cksum.s, which is about five times as fast;
 * `make CKSUM=in_cksum.o` in sys/PC builds it with this one instead.
 */
#include "param.h"
#include "mbuf.h"

in_cksum(m, len)
	register struct mbuf *m;
	register int len;
{
	register u_char *cp;
	register int mlen;
	long sum = 0;
	u_short carry = 0;
	int odd = 0;

	for (; m && len > 0; m = m->m_next) {
		cp = mtod(m, u_char *);
		mlen = MIN(m->m_len, len);
		len -= mlen;
		if (odd && mlen > 0) {
			sum += carry | (u_short)*cp++ << 8;
			mlen--;
			odd = 0;
		}
		while (mlen >= 2) {
			sum += *(u_short *)cp;
			cp += 2;
			mlen -= 2;
		}
		if (mlen) {
			carry = *cp;
			odd = 1;
		}
	}
	if (len > 0)
		printf("cksum: out of data\n");
	if (odd)
		sum += carry;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += sum >> 16;
	return ((int)~sum & 0xffff);
}
