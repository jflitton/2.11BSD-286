/*
 * A small printf for the tests: %d %u %x %o %c %s, with l for long and a
 * field width.  The kernel's own comes with the rest of sys/kern.
 */
static
printn(n, base, width)
	unsigned long n;
	int base, width;
{
	char buf[12];
	register char *cp = buf;

	do {
		*cp++ = "0123456789abcdef"[n % base];
		n /= base;
	} while (n);
	while (width-- > cp - buf)
		cnputc('0');
	while (cp > buf)
		cnputc(*--cp);
}

printf(fmt, args)
	char *fmt;
	int args;
{
	register char *s;
	register int *ap = &args;
	int c, width, islong;
	long n;

	while (c = *fmt++) {
		if (c != '%') {
			cnputc(c);
			continue;
		}
		width = 0;
		while (*fmt >= '0' && *fmt <= '9')
			width = width * 10 + *fmt++ - '0';
		islong = 0;
		if (*fmt == 'l') {
			islong = 1;
			fmt++;
		}
		c = *fmt++;
		switch (c) {
		case 'd': case 'u': case 'x': case 'o':
			if (islong) {
				n = *(long *)ap;
				ap += 2;
			} else if (c == 'd')
				n = *ap++;
			else
				n = (unsigned)*ap++;
			if (c == 'd' && n < 0) {
				cnputc('-');
				n = -n;
			}
			printn((unsigned long)n, c == 'x' ? 16 : c == 'o' ? 8 : 10, width);
			break;
		case 'c':
			cnputc(*ap++);
			break;
		case 's':
			for (s = (char *)*ap++; *s; s++)
				cnputc(*s);
			break;
		default:
			cnputc(c);
		}
	}
}
