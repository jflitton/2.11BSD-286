/*
 * The Internet checksum of the mlen bytes at w, for the 286: 16-bit
 * words with the low byte first, summed in a long and folded.
 * ../pdp/cksum.c is the PDP-11's, whose assembly an ed script finishes.
 */

#include "../globals.h"
#include <protocols/timed.h>

in_cksum(w, mlen)
	register u_short *w;
	register u_int mlen;
{
	register u_long sum = 0;

	while (mlen > 1) {
		sum += *w++;
		mlen -= 2;
	}
	if (mlen == 1)
		sum += *(u_char *)w;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return (~(u_short)sum);
}
