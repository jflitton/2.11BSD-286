/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
static char *sccsid = "@(#)printf.c	5.2 (Berkeley) 6/6/85";
#endif

#include <stdarg.h>

/*
 * Hacked "printf" which prints through putchar.
 * DONT USE WITH STDIO!
 */
#ifdef pdp11
printf(fmt, args)
char *fmt;
{
	_doprnt(fmt, &args, 0);
}

vprintf(fmt, ap)
char *fmt;
va_list ap;
{
	_doprnt(fmt, ap, 0);
}

_strout(count, string, adjust, foo, fillch)
register char *string;
register int count;
int adjust;
register struct { int a[6]; } *foo;
{

	if (foo != 0)
		abort();
	while (adjust < 0) {
		if (*string=='-' && fillch=='0') {
			putchar(*string++);
			count--;
		}
		putchar(fillch);
		adjust++;
	}
	while (--count>=0)
		putchar(*string++);
	while (adjust) {
		putchar(fillch);
		adjust--;
	}
}
#else !pdp11
/*
 * What doprnt11.s does on the PDP-11, in C.  A conversion is %, a - to
 * pad on the right, a width, padded with zeros if it begins with 0, a .
 * and a precision, which limits a string, and then
 *
 *	d o x u		an int in decimal, octal or hex, or unsigned
 *	ld lo lx lu	a long, so; D O X U the same
 *	l L		otherwise, an unsigned int
 *	c s		a character, a string
 *
 * A width or precision of * is the next argument.  Any other character
 * is printed as it is, so %% prints %.
 */
static
pad(n, c)
	register int n, c;
{
	while (--n >= 0)
		putchar(c);
}

#define	NUMBER(v) \
	for (v = 0; ; f++) \
		if (*f == '*') \
			v = va_arg(ap, int); \
		else if (*f >= '0' && *f <= '9') \
			v = v * 10 + *f - '0'; \
		else \
			break

printf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

vprintf(fmt, ap)
	const char *fmt;
	va_list ap;
{
	register const char *f = fmt;
	register char *s;
	register int c;
	int left, zero, width, prec, len, neg, base;
	unsigned long n;
	long v;
	char buf[12];		/* a long in octal, or a sign and 10 digits */

	while ((c = *f++) != '\0') {
		if (c != '%') {
			putchar(c);
			continue;
		}
		left = *f == '-';
		if (left)
			f++;
		zero = *f == '0';
		NUMBER(width);
		prec = 0;
		if (*f == '.') {
			f++;
			NUMBER(prec);
		}
		if ((c = *f++) == '\0')
			break;
		if (c == 'l' || c == 'L')
			switch (*f) {
			case 'd': case 'o': case 'x': case 'u':
				c = *f++ - 'a' + 'A';
				break;
			default:
				c = 'u';
			}
		s = buf;
		switch (c) {
		case 'd':
		case 'D':
			v = c == 'd' ? va_arg(ap, int) : va_arg(ap, long);
			neg = v < 0;
			n = neg ? -(unsigned long)v : v;
			base = 10;
			goto number;
		case 'o':
		case 'x':
		case 'u':
			n = va_arg(ap, unsigned);
			goto unsign;
		case 'O':
		case 'X':
		case 'U':
			n = va_arg(ap, unsigned long);
		unsign:
			neg = 0;
			base = c == 'o' || c == 'O' ? 8 :
			    c == 'x' || c == 'X' ? 16 : 10;
		number:
			s = &buf[sizeof buf];
			do
				*--s = "0123456789abcdef"[n % base];
			while ((n /= base) != 0);
			if (neg)
				*--s = '-';
			len = &buf[sizeof buf] - s;
			break;
		case 'c':
			buf[0] = va_arg(ap, int);
			len = buf[0] != '\0';
			zero = 0;
			break;
		case 's':
			if ((s = va_arg(ap, char *)) == 0)
				s = "(null)";
			for (len = 0; s[len] != '\0'; len++)
				if (prec > 0 && len == prec)
					break;
			zero = 0;
			break;
		default:
			buf[0] = c;
			len = 1;
		}
		width -= len;
		if (!left) {
			if (zero && *s == '-') {
				putchar(*s++);
				len--;
			}
			pad(width, zero ? '0' : ' ');
		}
		while (--len >= 0)
			putchar(*s++);
		if (left)
			pad(width, zero ? '0' : ' ');
	}
}
#endif pdp11
