/*
 * printf, of pxp, for the 286: the PDP-11's printf.s, in C.  It does
 * what pxp asks of it, %d, %ld, %c and %s, with a width, by which what
 * is printed is put to the right, and for %s a precision, through
 * pxp's own putchar (pp.c), not the C library's printf with its stdio.
 * As the PDP-11's, %c prints both the bytes of its int that are not 0,
 * %l not before d is %u, and what is not one of these prints itself.
 * The PDP-11's printf.s was atof too, which pxp does not call: here
 * that is the C library's.
 */

static char *formp;
static char *pru(), *prl();

printf(fmt, args)
	char *fmt;
	int args;
{
	int *ap, c;
	register int n;
	char buf[126];
	register char *bp;
	int width, ndigit;
	long l;

	formp = fmt;
	ap = &args;
	while ((c = *formp++) != 0) {
		if (c != '%') {
			putchar(c);
			continue;
		}
		ndigit = 0;
		width = gnum(&ap, &c);
		if (c == '.')
			ndigit = gnum(&ap, &c);
		bp = buf;
		switch (c) {
		case 'd':
			n = *ap++;
			if (n < 0) {
				*bp++ = '-';
				n = -n;
			}
			bp = pru(bp, (unsigned)n);
			break;
		case 'l':
			if (*formp == 'd') {
				formp++;
				l = *(long *)ap;
				ap += sizeof (long) / sizeof (int);
				if (l < 0) {
					*bp++ = '-';
					l = -l;
				}
				bp = prl(bp, l);
			} else
				bp = pru(bp, (unsigned)*ap++);
			break;
		case 'c':
			n = *ap++;
			if (n & 0377)
				*bp++ = n;
			if ((n >> 8) & 0377)
				*bp++ = n >> 8;
			break;
		case 's':
			bp = (char *)*ap++;
			for (n = 0; bp[n] && (ndigit == 0 || n < ndigit); n++)
				continue;
			prstr(bp, n, width);
			continue;
		case 0:
			formp--;
			continue;
		default:
			putchar(c);
			continue;
		}
		prstr(buf, bp - buf, width);
	}
}

/*
 * The number at formp, of digits and of * for the next argument, and
 * in *cp the character after it
 */
static
gnum(app, cp)
	int **app, *cp;
{
	register int c, n;

	n = 0;
	for (;;) {
		c = *formp++;
		if (c == '*')
			c = *(*app)++;
		else if (c >= '0' && c <= '9')
			c -= '0';
		else
			break;
		n = n * 10 + c;
	}
	*cp = c;
	return (n);
}

static char *
pru(bp, u)
	register char *bp;
	unsigned u;
{

	if (u >= 10)
		bp = pru(bp, u / 10);
	*bp++ = u % 10 + '0';
	return (bp);
}

static char *
prl(bp, l)
	register char *bp;
	long l;
{
	unsigned long u;

	u = l;
	if (u >= 10)
		bp = prl(bp, (long)(u / 10));
	*bp++ = u % 10 + '0';
	return (bp);
}

static
prstr(cp, n, width)
	register char *cp;
	register int n;
	int width;
{

	for (width -= n; width > 0; width--)
		putchar(' ');
	while (n-- > 0)
		putchar(*cp++);
}
