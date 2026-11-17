/*
 * Copyright (c) 1983, 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)time.h	1.4 (2.11BSD) 2026/6/7
 */

#ifndef	_TIME_H_
#define _TIME_H_

#include	<sys/types.h>		/* for time_t */

/*
 * Structure returned by gmtime and localtime calls (see ctime(3)).
 */
struct tm {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
	long	tm_gmtoff;
	char	*tm_zone;
};

extern  size_t strftime(char *, size_t, const char *, const struct tm *);
extern	struct tm *gmtime(time_t *), *localtime(time_t *);
extern	char *asctime(struct tm *), *ctime(time_t *);
extern	time_t	time(time_t *);
#endif
