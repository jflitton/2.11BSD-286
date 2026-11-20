/*
 * Longs: arithmetic, the routines for * / %, shifts, comparisons and
 * the conversions to and from the shorter types.
 */
#define	CK(got, want)	check(__LINE__, got, want)
#define	LK(got, want)	lcheck(__LINE__, (long)(got), (long)(want))

long	gl = 100000L, gm = -3L, gn;
unsigned long gul = 3000000000L;
long	glarr[4] = { 1L, 65536L, -65536L, 0x12345678L };
struct lst {
	int	pad;
	long	lv;
	char	c;
	long	lw;
} gst = { 7, 70000L, 'x', -70000L };

static long
lident(x)
long x;
{
	return x;
}

static
ident(x)
{
	return x;
}

static
arith()
{
	long a, b, c;
	long *p;
	int i;
	unsigned u;
	char ch;
	unsigned char uc;
	struct lst *sp;

	a = 123456L;
	b = -1000L;
	p = &a;
	i = -5;
	u = 50000;
	ch = -2;
	uc = 200;
	sp = &gst;
	LK(a + b, 122456L);
	LK(a - b, 124456L);
	LK(b - a, -124456L);
	LK(a + 1, 123457L);
	LK(a - 1, 123455L);
	LK(a + 65536L, 188992L);
	LK(a + 0x10000L, 188992L);
	LK(a - 123456L, 0L);
	LK(a + gl, 223456L);
	LK(gl + gm, 99997L);
	LK(gl - a, -23456L);
	LK(*p + b, 122456L);
	LK(b + *p, 122456L);
	LK(b - *p, -124456L);
	LK(a + sp->lv, 193456L);
	LK(sp->lw - sp->lv, -140000L);
	LK(glarr[1] + glarr[3], 0x12355678L);
	LK(-a, -123456L);
	LK(-b, 1000L);
	LK(~a, -123457L);
	LK(a & 0xFFFFL, 57920L);
	LK(a & 0xFF00FFL, 65600L);
	LK(a | 0x80000000L, 0x8001E240L);
	LK(a ^ b, -123304L);
	LK(a & b, 122880L);
	LK(a | b, -424L);
	LK(lident(a) + lident(b), 122456L);
	LK(lident(a) - lident(gl), 23456L);
	LK(a - lident(b), 124456L);
	LK((a + b) - (gl + gm), 22459L);
	LK((a ^ gl) & (b | 0xFFL), 25824L);
	/* with the shorter types */
	LK(a + i, 123451L);
	LK(a + u, 173456L);
	LK(a + ch, 123454L);
	LK(a + uc, 123656L);
	LK(a - i, 123461L);
	LK(i + a, 123451L);
	LK(a + ident(i), 123451L);
	LK(a + 5, 123461L);
	LK((long)i, -5L);
	LK((long)u, 50000L);
	LK((long)ch, -2L);
	LK((long)uc, 200L);
	LK((long)i * 1000, -5000L);
	LK((long)u + u, 100000L);
	LK((long)u * 3, 150000L);
	CK((int)a, 0xE240);
	CK((int)b, -1000);
	CK((int)(a >> 16), 1);
	CK((char)a, 0x40);
	c = i;
	LK(c, -5L);
	c = u;
	LK(c, 50000L);
	c = ch;
	LK(c, -2L);
	c = uc;
	LK(c, 200L);
	i = a;
	CK(i, 0xE240);
	u = b;
	CK(u, -1000);
	ch = a;
	CK(ch, 0x40);
	/* assignment */
	c = a;
	LK(c, 123456L);
	c += b;
	LK(c, 122456L);
	c -= 22456L;
	LK(c, 100000L);
	c += 1;
	LK(c, 100001L);
	c -= 2;
	LK(c, 99999L);
	c += 0x10000L;
	LK(c, 165535L);
	c &= 0xFFFFL;
	LK(c, 34463L);
	c |= 0x70000L;
	LK(c, 493215L);
	c ^= a;
	LK(c, 419039L);
	c += i;
	LK(c, 411423L);
	c = 0;
	LK(c, 0L);
	c = -1;
	LK(c, -1L);
	c = 70000L;
	LK(c++, 70000L);
	LK(c, 70001L);
	LK(++c, 70002L);
	LK(c--, 70002L);
	LK(--c, 70000L);
	c = 65535L;
	c++;
	LK(c, 65536L);
	c--;
	LK(c, 65535L);
	*p += 4;
	LK(a, 123460L);
	*p -= lident(4L);
	LK(a, 123456L);
	*p = b;
	LK(a, -1000L);
	*p = 123456L;
	gn = a + b;
	LK(gn, 122456L);
	gn += gl;
	LK(gn, 222456L);
	gn -= a;
	LK(gn, 99000L);
	gst.lv += 5;
	LK(gst.lv, 70005L);
	sp->lv -= 5;
	LK(sp->lv, 70000L);
	sp->lw = sp->lv + 1;
	LK(gst.lw, 70001L);
	sp->lw = -70000L;
	glarr[0] += glarr[1];
	LK(glarr[0], 65537L);
	glarr[0] = 1;
	LK(c = a, 123456L);
	LK(c += 4, 123460L);
}

static
muldiv()
{
	long a, b, c;
	unsigned long ua, ub;
	long *p, *q;
	int i;

	a = 123456L;
	b = -789L;
	ua = 4000000000L;
	ub = 70000L;
	p = &a;
	i = 100;
	LK(a * b, -97406784L);
	LK(a * 3, 370368L);
	LK(a * i, 12345600L);
	LK(i * a, 12345600L);
	LK(a * a, -1938485248L);
	LK(a / b, -156L);
	LK(a % b, 372L);
	LK(a / 1000, 123L);
	LK(a % 1000, 456L);
	LK(-a / 1000, -123L);
	LK(-a % 1000, -456L);
	LK(a / i, 1234L);
	LK(a % i, 56L);
	LK(*p / b, -156L);
	LK(b / *p, 0L);
	LK(b % *p, -789L);
	LK(a / 65536L, 1L);
	LK(a % 65536L, 57920L);
	LK(ua / ub, 57142L);
	LK(ua % ub, 60000L);
	LK(ua / 10, 400000000L);
	LK(ua % 10, 0L);
	LK(ua / 3, 1333333333L);
	LK(ua % 7, 3L);
	LK(lident(a) * lident(b), -97406784L);
	LK(lident(a) / lident(b), -156L);
	LK(lident(a) % lident(b), 372L);
	LK(gl * gm, -300000L);
	LK(gl / gm, -33333L);
	LK(gl % gm, 1L);
	LK(gul / 1000, 3000000L);
	LK(gul % 1000, 0L);
	LK(a * b + a / b - a % b, -97407312L);
	c = a;
	c *= b;
	LK(c, -97406784L);
	c /= 1000;
	LK(c, -97406L);
	c %= 1000;
	LK(c, -406L);
	c *= c;
	LK(c, 164836L);
	c /= b;
	LK(c, -208L);
	ua /= ub;
	LK(ua, 57142L);
	ua *= ub;
	LK(ua, 3999940000L);
	ua %= 1000000L;
	LK(ua, 940000L);
	*p *= 2;
	LK(a, 246912L);
	*p /= lident(2L);
	LK(a, 123456L);
	*p %= 10000;
	LK(a, 3456L);
	*p *= b;
	LK(a, -2726784L);
	q = &c;				/* both through pointers */
	*p /= *q;
	LK(a, 13109L);
	*p *= *q;
	LK(a, -2726672L);
	*q *= *q;
	LK(c, 43264L);
	gn = 7;
	gn *= gl;
	LK(gn, 700000L);
	gn /= gm;
	LK(gn, -233333L);
	gn %= 1000;
	LK(gn, -333L);
	gst.lv *= 3;
	LK(gst.lv, 210000L);
	gst.lv /= 3;
	LK(gst.lv, 70000L);
}

static
shifts()
{
	long a, c;
	unsigned long u;
	long *p;
	int n;

	a = -123456L;
	u = 0x87654321L;
	p = &a;
	LK(a << 1, -246912L);
	LK(a << 3, -987648L);
	LK(a << 8, -31604736L);
	LK(a << 16, 0x1DC00000L);
	LK(a << 20, 0xDC000000L);
	LK(a >> 1, -61728L);
	LK(a >> 3, -15432L);
	LK(a >> 8, -483L);
	LK(a >> 16, -2L);
	LK(a >> 20, -1L);
	LK(u >> 1, 0x43B2A190L);
	LK(u >> 4, 0x08765432L);
	LK(u >> 16, 0x8765L);
	LK(u >> 28, 8L);
	LK(u << 4, 0x76543210L);
	LK(u << 16, 0x43210000L);
	for (n = 0; n < 32; n++) {
		c = 1;
		c <<= n;
		LK(1L << n, c);
		LK((unsigned long)c >> n, 1L);
	}
	n = 5;
	LK(a << n, -3950592L);
	LK(a >> n, -3858L);
	LK(u >> n, 0x043B2A19L);
	n = 17;
	LK(a >> n, -1L);
	LK(u >> n, 0x43B2L);
	LK(u << n, 0x86420000L);
	n = 0;
	LK(a << n, -123456L);
	LK(u >> n, 0x87654321L);
	LK(*p >> 2, -30864L);
	LK(lident(a) << ident(2), -493824L);
	c = a;
	c <<= 2;
	LK(c, -493824L);
	c >>= 4;
	LK(c, -30864L);
	n = 3;
	c >>= n;
	LK(c, -3858L);
	c <<= n;
	LK(c, -30864L);
	u >>= 12;
	LK(u, 0x87654L);
	u <<= 20;
	LK(u, 0x65400000L);
	u >>= n;
	LK(u, 0x0CA80000L);
	*p <<= 1;
	LK(a, -246912L);
	*p >>= ident(1);
	LK(a, -123456L);
	gl <<= 2;
	LK(gl, 400000L);
	gl >>= 2;
	LK(gl, 100000L);
	gul >>= 31;
	LK(gul, 1L);
	gul = 3000000000L;
}

static
cmp1(a, b)
long a, b;
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
ucmp1(a, b)
unsigned long a, b;
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
cmp0(a)
long a;
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

static
cmpk(a)
long a;
{
	register r;

	r = 0;
	if (a == 100000L) r |= 1;
	if (a != 100000L) r |= 2;
	if (a < 100000L) r |= 4;
	if (a <= 100000L) r |= 8;
	if (a > 100000L) r |= 16;
	if (a >= 100000L) r |= 32;
	if (a < 5) r |= 64;
	if (a > -5) r |= 128;
	return r;
}

#define	LT	(2|4|8)
#define	EQ	(1|8|32)
#define	GT	(2|16|32)

static
compares()
{
	long a, b;
	long *p;

	CK(cmp1(1L, 2L), LT);
	CK(cmp1(2L, 1L), GT);
	CK(cmp1(2L, 2L), EQ);
	CK(cmp1(-1L, 1L), LT);
	CK(cmp1(1L, -1L), GT);
	CK(cmp1(0x10000L, 0xFFFFL), GT);
	CK(cmp1(0xFFFFL, 0x10000L), LT);
	CK(cmp1(0x18000L, 0x17FFFL), GT);
	CK(cmp1(0x10001L, 0x18000L), LT);
	CK(cmp1(0x28000L, 0x30001L), LT);
	CK(cmp1(-65536L, -65537L), GT);
	CK(cmp1(-65537L, -65536L), LT);
	CK(cmp1(0x7FFFFFFFL, 0x80000000L), GT);
	CK(cmp1(0x80000000L, 0x7FFFFFFFL), LT);
	CK(cmp1(0x12345678L, 0x12345678L), EQ);
	CK(cmp1(0x12345678L, 0x12345679L), LT);
	CK(cmp1(0x12355678L, 0x12345679L), GT);
	CK(ucmp1(1L, 2L), LT);
	CK(ucmp1(0x7FFFFFFFL, 0x80000000L), LT);
	CK(ucmp1(0x80000000L, 0x7FFFFFFFL), GT);
	CK(ucmp1(0xFFFFFFFFL, 0L), GT);
	CK(ucmp1(0x18000L, 0x17FFFL), GT);
	CK(ucmp1(0x8000L, 0x7FFFL), GT);
	CK(ucmp1(0x80000000L, 0x80000000L), EQ);
	CK(cmp0(0L), 1|8|32|128);
	CK(cmp0(1L), 2|16|32|64);
	CK(cmp0(-1L), 2|4|8|64);
	CK(cmp0(0x10000L), 2|16|32|64);
	CK(cmp0(-0x10000L), 2|4|8|64);
	CK(cmp0(0x8000L), 2|16|32|64);
	CK(cmp0(0x80000000L), 2|4|8|64);
	CK(cmpk(100000L), EQ|128);
	CK(cmpk(100001L), GT|128);
	CK(cmpk(99999L), LT|128);
	CK(cmpk(34464L), LT|128);
	CK(cmpk(165536L), GT|128);
	CK(cmpk(-100000L), LT|64);
	CK(cmpk(4L), LT|64|128);
	CK(cmpk(-5L), LT|64);
	a = 70000L;
	b = -70000L;
	p = &a;
	CK(a > b, 1);
	CK(a < b, 0);
	CK(a == 70000L, 1);
	CK(a != 70000L, 0);
	CK(*p > 69999L, 1);
	CK(*p >= gl, 0);
	CK(gl > *p, 1);
	CK(a + b == 0, 1);
	CK(a + b != 0, 0);
	CK(a + b + 1 > 0, 1);
	CK(!a, 0);
	CK(!(a + b), 1);
	CK(a && b, 1);
	CK(a - 70000L || b + 70000L, 0);
	CK(gl == 100000L && gm == -3, 1);
	CK(gul > 2000000000L, 1);
	CK(gst.lv > gst.lw, 1);
	CK(lident(a) > lident(b), 1);
	CK(lident(a) == a, 1);
	CK(a > 5, 1);
	CK(b < 5, 1);
	CK((a > b ? a : b) == a, 1);
	LK(a > b ? a : b, 70000L);
	LK(a < b ? a : b, -70000L);
	LK(a ? 1L : 2L, 1L);
}

static long
fact(n)
long n;
{
	if (n <= 1)
		return 1;
	return n * fact(n - 1);
}

static long
mix(a, i, b, c)
long a, b;
int i;
char c;
{
	return a + i + b + c;
}

static
calls()
{
	long a;
	long lsum3();

	LK(fact(10L), 3628800L);
	LK(fact(12L), 479001600L);
	LK(mix(100000L, -5, 200000L, 'A'), 300060L);
	LK(mix(-1L, 1, 0L, 0), 0L);
	LK(lsum3(100000L, -7, 5L), 99998L);
	a = lsum3(gl, 3, gm);
	LK(a, 100000L);
	LK(lsum3(a + 1, (int)a, a * 2), 268929L);
	LK(lident(lident(lident(-99999L))), -99999L);
}

t_long()
{
	arith();
	muldiv();
	shifts();
	compares();
	calls();
}
