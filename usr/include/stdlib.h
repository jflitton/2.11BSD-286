/*-
 * Copyright (c) 1990, 1993
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
 *	@(#)stdlib.h	8.3.3 (2.11BSD) 2026/9/16
 *
 * Adapted from the 4.4-Lite CD
 */

#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <machine/ansi.h>

/*
 *  XXX - do this here to avoid programs having to include unistd.h first
 *  XXX - to get size_t
*/
#include <unistd.h>

typedef struct {
	int quot;		/* quotient */
	int rem;		/* remainder */
} div_t;

typedef struct {
	long quot;		/* quotient */
	long rem;		/* remainder */
} ldiv_t;

#ifndef NULL
#define	NULL	0
#endif

#define	EXIT_FAILURE	1
#define	EXIT_SUCCESS	0

#define	RAND_MAX	0x7fff

void	abort(void);
int	abs(int);
int	atexit(void (*)(void));
double	atof(const char *);
int	atoi(const char *);
long	atol(const char *);
void	*calloc(size_t, size_t);
void	exit(int);
void	free(void *);
char	*getenv(const char *);
void	*malloc(size_t);
void	qsort(void *, size_t, size_t, int (*)(const void *, const void *));
int	rand(void);
void	*realloc(void *, size_t);
void	srand(unsigned);
double	strtod(const char *, char **);
long	strtol(const char *, char **, int);
unsigned long strtoul(const char *, char **, int);
int	system(const char *);
char	*mktemp(char *);
int	mkstemp(char *);
int	putenv(const char *);
int	setenv(const char *, char *, int);

void	*alloca(size_t);
#if defined(i286) && defined(__WATCOMC__)
/*
 * In line: the compiler knows the stack pointer has moved, and a
 * function that calls alloca takes it back from its frame pointer.
 */
#ifdef __LARGE__
#pragma aux alloca = "inc ax" "and al, 0feh" "sub sp, ax" "mov ax, sp" \
	"mov dx, ss" parm [ax] value [dx ax] modify [sp];
#else
#pragma aux alloca = "inc ax" "and al, 0feh" "sub sp, ax" "mov ax, sp" \
	parm [ax] value [ax] modify [sp];
#endif
#endif

int	daemon(int, int);
char	*devname(dev_t, mode_t);
void	devname_close(void);		/* 2.11BSD 35th anniversary edition */
int	swapdevname(char *, int);	/* 2.11BSD 35th anniversary edition */
int	getloadavg(double *, int);

extern char *optarg;			/* getopt(3) external variables */
extern int opterr, optind, optopt;
getopt(int, char * const *, const char *);

extern char *suboptarg;			/* getsubopt(3) external variable */
int	getsubopt(char **, char * const *, char **);

long	random(void);
char	*setstate(char *);
void	srandom(unsigned);
void	unsetenv(const char *);
#endif /* _STDLIB_H_ */
