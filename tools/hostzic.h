/*
 * Taken in ahead of zic.c when the build makes the zone files with a
 * zic of the machine it runs on (usr/src/i286/Makefile): there time_t
 * is 64 bits, and zic, which works out from time_t which years a file
 * covers, would write each rule's changes up to the year 292 billion.
 * The system's time_t is a long, 32 bits, as the PDP-11's is, so that
 * the files run from 1901 to 2038 and are those the system's own zic
 * makes, byte for byte (tests/programs).
 */
#include <sys/types.h>
#include <time.h>
#include <stdint.h>

static struct tm *
zic_gmtime(const int32_t *t)
{
	time_t	t64 = *t;

	return gmtime(&t64);
}

#define	time_t	int32_t
#define	gmtime	zic_gmtime
