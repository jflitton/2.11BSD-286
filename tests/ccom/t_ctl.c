/*
 * Comparisons and control: each relation, signed and unsigned, between
 * operands of each kind and against zero after each way of getting a
 * value; the logical operators; loops; switches of each of the three
 * kinds the compiler makes.
 */
#define	CK(got, want)	check(__LINE__, got, want)

#define	LT	(2|4|8)
#define	EQ	(1|8|32)
#define	GT	(2|16|32)

int	gx, gy;
unsigned gux;
char	gc1;

static
ident(x)
{
	return x;
}

static
rel(a, b)
{
	register r;

	r = 0;
	if (a == b) r |= 1;
	if (a != b) r |= 2;
	if (a < b) r |= 4;
	if (a <= b) r |= 8;
	if (a > b) r |= 16;
	if (a >= b) r |= 32;
	return r;
}

static
urel(a, b)
unsigned a, b;
{
	register r;

	r = 0;
	if (a == b) r |= 1;
	if (a != b) r |= 2;
	if (a < b) r |= 4;
	if (a <= b) r |= 8;
	if (a > b) r |= 16;
	if (a >= b) r |= 32;
	return r;
}

/* the same with the operands in registers, and as values */
static
rrel(a, b)
register a, b;
{
	return (a == b) | (a != b) << 1 | (a < b) << 2 | (a <= b) << 3
	    | (a > b) << 4 | (a >= b) << 5;
}

static
prel(a, b)
char *a, *b;
{
	return (a == b) | (a != b) << 1 | (a < b) << 2 | (a <= b) << 3
	    | (a > b) << 4 | (a >= b) << 5;
}

static
crel(a, b)
char a, b;
{
	register r;

	r = 0;
	if (a == b) r |= 1;
	if (a != b) r |= 2;
	if (a < b) r |= 4;
	if (a <= b) r |= 8;
	if (a > b) r |= 16;
	if (a >= b) r |= 32;
	return r;
}

/* against a constant */
static
krel(a)
{
	register r;

	r = 0;
	if (a == 100) r |= 1;
	if (a != 100) r |= 2;
	if (a < 100) r |= 4;
	if (a <= 100) r |= 8;
	if (a > 100) r |= 16;
	if (a >= 100) r |= 32;
	return r;
}

/* against zero: a variable in memory */
static
zrel(a)
{
	register r;

	r = 0;
	if (a == 0) r |= 1;
	if (a != 0) r |= 2;
	if (a < 0) r |= 4;
	if (a <= 0) r |= 8;
	if (a > 0) r |= 16;
	if (a >= 0) r |= 32;
	if (a) r |= 64;
	if (!a) r |= 128;
	return r;
}

/* one in a register */
static
zrrel(a)
register a;
{
	register r;

	r = 0;
	if (a == 0) r |= 1;
	if (a != 0) r |= 2;
	if (a < 0) r |= 4;
	if (a <= 0) r |= 8;
	if (a > 0) r |= 16;
	if (a >= 0) r |= 32;
	if (a) r |= 64;
	if (!a) r |= 128;
	return r;
}

/* a sum, whose flags the addition sets, and which may overflow */
static
zsum(a, b)
{
	register r;

	r = 0;
	if (a + b == 0) r |= 1;
	if (a + b != 0) r |= 2;
	if (a + b < 0) r |= 4;
	if (a + b <= 0) r |= 8;
	if (a + b > 0) r |= 16;
	if (a + b >= 0) r |= 32;
	if (a + b) r |= 64;
	if (!(a + b)) r |= 128;
	return r;
}

/* the same where the sum is stored */
static
zasg(a, b)
{
	register r;
	int t;

	r = 0;
	t = a; if ((t += b) == 0) r |= 1;
	t = a; if ((t += b) != 0) r |= 2;
	t = a; if ((t += b) < 0) r |= 4;
	t = a; if ((t += b) <= 0) r |= 8;
	t = a; if ((t += b) > 0) r |= 16;
	t = a; if ((t += b) >= 0) r |= 32;
	t = a; if (t += b) r |= 64;
	t = a; if (!(t -= -b)) r |= 128;
	return r;
}

/* a function's value, a character and an and */
static
zcall(a)
{
	register r;
	char c;

	r = 0;
	c = a;
	if (ident(a) < 0) r |= 1;
	if (ident(a) > 0) r |= 2;
	if (ident(a)) r |= 4;
	if (c < 0) r |= 8;
	if (c > 0) r |= 16;
	if (c) r |= 32;
	if (c <= 0) r |= 64;
	if (a & 0x4004) r |= 128;
	if ((a & 0x8000) != 0) r |= 256;
	if (a & gx) r |= 512;
	if (c & 4) r |= 1024;
	return r;
}

static
zuns(a)
unsigned a;
{
	register r;
	unsigned char uc;

	r = 0;
	uc = a;
	if (a == 0) r |= 1;
	if (a != 0) r |= 2;
	if (a > 0) r |= 4;
	if (a >= 0) r |= 8;
	if (a <= 0) r |= 16;
	if (uc < 0) r |= 32;
	if (uc > 0) r |= 64;
	if (uc >= 128) r |= 128;
	if (uc) r |= 256;
	return r;
}

static
relations()
{
	int a;
	char *s;

	CK(rel(1, 2), LT);
	CK(rel(2, 1), GT);
	CK(rel(2, 2), EQ);
	CK(rel(-1, 1), LT);
	CK(rel(1, -1), GT);
	CK(rel(-2, -1), LT);
	CK(rel(32767, -32767 - 1), GT);
	CK(rel(-32767 - 1, 32767), LT);
	CK(rel(0, 0), EQ);
	CK(urel(1, 2), LT);
	CK(urel(2, 1), GT);
	CK(urel(-1, 1), GT);
	CK(urel(1, -1), LT);
	CK(urel(0x8000, 0x7FFF), GT);
	CK(urel(0x7FFF, 0x8000), LT);
	CK(urel(0xFFFF, 0xFFFF), EQ);
	CK(rrel(1, 2), LT);
	CK(rrel(2, 1), GT);
	CK(rrel(-5, -5), EQ);
	CK(rrel(32767, -32767 - 1), GT);
	CK(rrel(-32767 - 1, 32767), LT);
	s = "abc";
	CK(prel(s, s + 1), LT);
	CK(prel(s + 2, s + 1), GT);
	CK(prel(s, s), EQ);
	CK(prel((char *)0x7FFF, (char *)0x8000), LT);
	CK(prel((char *)0xFFF0, (char *)0x10), GT);
	CK(crel('a', 'b'), LT);
	CK(crel('b', 'a'), GT);
	CK(crel(-1, 1), LT);
	CK(crel(127, -128), GT);
	CK(crel(-3, -3), EQ);
	CK(krel(100), EQ);
	CK(krel(99), LT);
	CK(krel(101), GT);
	CK(krel(-100), LT);
	CK(krel(32767), GT);
	CK(krel(-32767 - 1), LT);
	CK(zrel(0), 1|8|32|128);
	CK(zrel(5), 2|16|32|64);
	CK(zrel(-5), 2|4|8|64);
	CK(zrel(-32767 - 1), 2|4|8|64);
	CK(zrrel(0), 1|8|32|128);
	CK(zrrel(5), 2|16|32|64);
	CK(zrrel(-5), 2|4|8|64);
	CK(zsum(3, -3), 1|8|32|128);
	CK(zsum(3, -2), 2|16|32|64);
	CK(zsum(3, -4), 2|4|8|64);
	CK(zsum(32767, 1), 2|4|8|64);
	CK(zsum(-32767, -2), 2|16|32|64);
	CK(zsum(-32767 - 1, -32767 - 1), 1|8|32|128);
	CK(zasg(3, -3), 1|8|32|128);
	CK(zasg(3, -2), 2|16|32|64);
	CK(zasg(3, -4), 2|4|8|64);
	CK(zasg(32767, 1), 2|4|8|64);
	CK(zasg(-32767, -2), 2|16|32|64);
	gx = 0x10;
	CK(zcall(0), 64);
	CK(zcall(5), 2|4|16|32|128|1024);
	CK(zcall(-3), 1|4|8|32|64|128|256|512|1024);
	CK(zcall(0x100), 2|4|64);
	CK(zcall(0x4090), 2|4|8|32|64|128|512);
	CK(zuns(0), 1|8|16);
	CK(zuns(5), 2|4|8|64|256);
	CK(zuns(0x8080), 2|4|8|64|128|256);
	CK(zuns(0x100), 2|4|8);
	a = 5;
	CK(a == 5, 1);
	CK(a != 5, 0);
	CK(a < 5, 0);
	CK(a <= 5, 1);
	CK((a > 4) + (a > 5), 1);
	CK(-(a >= 5), -1);
	CK(!(a == 5), 0);
	CK(a == ident(5), 1);
	CK(ident(a) < ident(6), 1);
	CK(a - 5 == 0, 1);
	CK((a & 4) != 0, 1);
	CK((a & 2) != 0, 0);
}

static int nside;

static
side(v)
{
	nside++;
	return v;
}

static
logical()
{
	int a, b, c;

	a = 3;
	b = 0;
	c = -1;
	CK(a && c, 1);
	CK(a && b, 0);
	CK(b && a, 0);
	CK(a || b, 1);
	CK(b || b, 0);
	CK(b || c, 1);
	CK(!a, 0);
	CK(!b, 1);
	CK(!a || !b, 1);
	CK(!a && !b, 0);
	CK(a && b || c, 1);
	CK(a && (b || c), 1);
	CK((a || b) && b, 0);
	CK(a > 2 && c < 0 && b == 0, 1);
	CK(a > 2 && c < 0 && b != 0, 0);
	CK(a < 2 || c > 0 || b == 0, 1);
	CK(a < 2 || c > 0 || b != 0, 0);
	CK(!(a > 2 && c < 0), 0);
	CK(!(a < 2 || c > 0), 1);
	/* the second is not evaluated when the first decides */
	nside = 0;
	CK(b && side(1), 0);
	CK(nside, 0);
	CK(a || side(1), 1);
	CK(nside, 0);
	CK(a && side(0), 0);
	CK(nside, 1);
	CK(b || side(7), 1);
	CK(nside, 2);
	CK(a ? 10 : 20, 10);
	CK(b ? 10 : 20, 20);
	CK(a ? b ? 1 : 2 : 3, 2);
	CK(b ? 1 : c ? 2 : 3, 2);
	CK((a > b ? a : b) + (a < b ? a : b), 3);
	CK(a ? side(4) : side(5), 4);
	CK(nside, 3);
	CK((a, b, c), -1);
	CK((side(1), side(2)), 2);
	CK(nside, 5);
	a = b ? 100 : c ? 200 : 300;
	CK(a, 200);
	if (a == 200 && b == 0)
		a = 1;
	else
		a = 2;
	CK(a, 1);
	if (a == 2 || b)
		a = 3;
	else if (c)
		a = 4;
	else
		a = 5;
	CK(a, 4);
}

static
loops()
{
	register int i, n;
	int j, k;
	int a[40];

	n = 0;
	for (i = 0; i < 10; i++)
		n += i;
	CK(n, 45);
	n = 0;
	for (i = 10; i > 0; i--)
		n += i;
	CK(n, 55);
	n = 0;
	for (i = 10; --i >= 0; )
		n += i;
	CK(n, 45);
	i = 0;
	n = 0;
	while (i < 100) {
		i += 7;
		if (i % 2)
			continue;
		if (i > 80)
			break;
		n += i;
	}
	CK(n, 14 + 28 + 42 + 56 + 70);
	CK(i, 84);
	i = 0;
	do
		i++;
	while (i < 5);
	CK(i, 5);
	i = 10;
	do
		i++;
	while (i < 5);
	CK(i, 11);
	n = 0;
	for (j = 0; j < 5; j++)
		for (k = 0; k < 5; k++) {
			if (k == j)
				continue;
			if (k > 3)
				break;
			n += j * k;
		}
	CK(n, 46);
	j = 3;
	n = 0;
	while (j--)
		n += 10;
	CK(n, 30);
	CK(j, -1);
	j = 3;
	n = 0;
	while (--j)
		n += 10;
	CK(n, 20);
	n = 0;
	i = 0;
again:
	n += i;
	if (++i < 5)
		goto again;
	CK(n, 10);
	/*
	 * A body too long for a short jump to reach round.
	 */
	for (i = 0; i < 40; i++)
		a[i] = i;
	n = 0;
	for (i = 0; i < 40; i++) {
		if (a[i] & 1) {
			n += a[i] * 3 + gx - gy + (a[i] << 2) - (a[i] >> 1);
			gy = n ^ a[i];
			gx = (gy & 0xFF) + a[(i + 1) % 40] - a[(i + 39) % 40];
			gy = gy / 7 + gx % 5 - a[i] * a[i];
			n -= gy & 0x0F0F;
			a[i] = n;
		} else {
			n += a[i] + 1;
		}
	}
	CK(n != 0, 1);
	CK(i, 40);
	gx = gy = 0;
}

static
sw1(x)
{
	switch (x) {
	case 0:
		return 10;
	case 1:
		return 11;
	case 2:
	case 3:
		return 23;
	case 5:
		return 15;
	case 6:
		return 16;
	default:
		return -1;
	}
}

static
sw2(x)
{
	int r;

	r = 0;
	switch (x) {
	case -100:
		r = 1;
		break;
	case 7:
		r = 2;
	case 1000:
		r += 3;
		break;
	case 32767:
		r = 4;
		break;
	case -32767 - 1:
		r = 5;
	}
	return r;
}

static
sw3(x)
{
	switch (x) {
	case 3: return 1;
	case 17: return 2;
	case 101: return 3;
	case 250: return 4;
	case 1001: return 5;
	case 2500: return 6;
	case 4097: return 7;
	case 9000: return 8;
	case 12345: return 9;
	case 20000: return 10;
	case 30003: return 11;
	case -1: return 12;
	case -20000: return 13;
	case -4096: return 14;
	case 77: return 15;
	case 78: return 16;
	}
	return 0;
}

static
sw4(c)
char c;
{
	register n;

	n = 0;
	switch (c) {
	case 'a': case 'e': case 'i': case 'o': case 'u':
		n = 1;
		break;
	case ' ':
	case '\t':
		n = 2;
		break;
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		n = 3;
		break;
	}
	return n;
}

static
sw5(x, y)
{
	switch (x) {
	case 1:
		switch (y) {
		case 1: return 11;
		case 2: return 12;
		}
		return 10;
	case 2:
		switch (y) {
		default: return 20;
		case 5: return 25;
		}
	}
	return 0;
}

static
switches()
{
	int i, n;

	CK(sw1(0), 10);
	CK(sw1(1), 11);
	CK(sw1(2), 23);
	CK(sw1(3), 23);
	CK(sw1(4), -1);
	CK(sw1(5), 15);
	CK(sw1(6), 16);
	CK(sw1(7), -1);
	CK(sw1(-1), -1);
	CK(sw1(1000), -1);
	CK(sw1(-32767), -1);
	CK(sw2(-100), 1);
	CK(sw2(7), 5);
	CK(sw2(1000), 3);
	CK(sw2(32767), 4);
	CK(sw2(-32767 - 1), 5);
	CK(sw2(0), 0);
	CK(sw2(8), 0);
	n = 0;
	for (i = -5; i < 13000; i += 7)
		n += sw3(i);
	CK(n, 7);
	CK(sw3(3), 1);
	CK(sw3(17), 2);
	CK(sw3(101), 3);
	CK(sw3(250), 4);
	CK(sw3(1001), 5);
	CK(sw3(2500), 6);
	CK(sw3(4097), 7);
	CK(sw3(9000), 8);
	CK(sw3(12345), 9);
	CK(sw3(20000), 10);
	CK(sw3(30003), 11);
	CK(sw3(-1), 12);
	CK(sw3(-20000), 13);
	CK(sw3(-4096), 14);
	CK(sw3(77), 15);
	CK(sw3(78), 16);
	CK(sw3(79), 0);
	CK(sw3(0), 0);
	CK(sw3(4096), 0);
	CK(sw4('a'), 1);
	CK(sw4('u'), 1);
	CK(sw4('b'), 0);
	CK(sw4(' '), 2);
	CK(sw4('\t'), 2);
	CK(sw4('7'), 3);
	CK(sw4('0'), 3);
	CK(sw4(':'), 0);
	CK(sw4(-1), 0);
	CK(sw5(1, 1), 11);
	CK(sw5(1, 2), 12);
	CK(sw5(1, 3), 10);
	CK(sw5(2, 5), 25);
	CK(sw5(2, 1), 20);
	CK(sw5(3, 1), 0);
}

/* what is true or false, compared with zero */
static
truth(a, b)
{
	register int n;

	n = 0;
	if ((a && b) == 0)
		n += 1;
	if ((a || b) != 0)
		n += 2;
	if (!a == 0)
		n += 4;
	if ((a < b) == 0)
		n += 8;
	if ((a && b) != 0)
		n += 16;
	if ((a == b || a > b) == 0)
		n += 32;
	n += ((a && !b) == 0) * 64;
	return n;
}

t_ctl()
{
	CK(truth(0, 0), 73);
	CK(truth(1, 2), 118);
	CK(truth(2, 0), 15);
	relations();
	logical();
	loops();
	switches();
}
