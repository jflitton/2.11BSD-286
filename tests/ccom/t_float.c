/*
 * Floating point: float and double, their arithmetic and comparisons,
 * what they are made from and made into, and calls with them, to and
 * from functions the other compiler compiled.  The numbers are ones a
 * double holds exactly, so that every check is of every bit.
 */
#define	CK(got, want)	check(__LINE__, got, want)
#define	LK(got, want)	lcheck(__LINE__, (long)(got), (long)(want))
#define	DK(got, want)	dcheck(__LINE__, (double)(got), (double)(want))

double dsum3(), dcallback(), dscramble();
float fhalf();

double	gd = 2.5, ge = -0.75, gz;
float	gf = 1.25, gg;
double	da[4] = { 1.0, -2.0, 0.5, 1024 };
float	fa[3] = { 3, -0.125, 100.5 };
double	dneg = -3.5;
double	dint = 7;
double	dbig = 1e10;
double	dsmall = 0.0009765625;
double	d24 = 16777217.0;
struct fs {
	char	c;
	float	f;
	int	i;
	double	d;
} gfs = { 'x', 0.5, 3, -8.25 };
int	fgi = 7;
long	fgl = 100000L;
unsigned fgu = 50000;
unsigned long fgul = 3000000000;
char	fgc = -3;
unsigned char fguc = 200;
int	fcalls;

static
arith()
{
	double a, b, c;
	float f, g;
	register double *dp;
	register int i;

	/* what they were made with */
	DK(gd, 2.5);
	DK(ge, -0.75);
	DK(gz, 0);
	DK(gf, 1.25);
	DK(da[0], 1);
	DK(da[1], -2);
	DK(da[2], 0.5);
	DK(da[3], 1024);
	DK(fa[0], 3);
	DK(fa[1], -0.125);
	DK(fa[2], 100.5);
	DK(dneg, -3.5);
	DK(dint, 7);
	DK(dbig, 10000000000.0);
	DK(dsmall * 1024, 1);
	DK(gfs.f, 0.5);
	DK(gfs.d, -8.25);
	CK(sizeof(float), 4);
	CK(sizeof(double), 8);
	CK(sizeof(struct fs), 16);

	a = 6;
	b = 1.5;
	DK(a + b, 7.5);
	DK(a - b, 4.5);
	DK(b - a, -4.5);
	DK(a * b, 9);
	DK(a / b, 4);
	DK(b / a, 0.25);
	DK(-a, -6);
	DK(-(a - b), -4.5);
	DK(a + gd, 8.5);
	DK(gd - a, -3.5);
	DK(a * 2, 12);
	DK(2 * a, 12);
	DK(a / 4, 1.5);
	DK(3 / a, 0.5);
	DK(a + 1, 7);
	DK(a - 0, 6);
	DK(a + b * gd - ge / 0.25, 12.75);
	DK((a + b) * (gd - ge), 24.375);
	DK((a + b) * (gd - ge) / ((a - b + 3.5) * (b + b)), 1.015625);
	DK(a - (b - (gd - (ge - 1))), 8.75);
	DK(a / (b / (ge / (gd / 5))), -6);
	c = a;
	DK(c, 6);
	c = a + b;
	DK(c, 7.5);
	a = b = c = 0.125;
	DK(a + b + c, 0.375);

	/* float is computed as double and kept as float */
	f = 0.5;
	g = f * 3;
	DK(g, 1.5);
	DK(f + g, 2);
	DK(f - gf, -0.75);
	gg = gf / f;
	DK(gg, 2.5);
	gg = d24;
	scramble(1);
	DK(gg, 16777216.0);
	a = d24;
	DK(a, 16777217.0);
	gg = a + 2;
	scramble(1);
	DK(gg, 16777220.0);
	a = gf;
	DK(a, 1.25);

	/* through pointers, in arrays and structures */
	dp = da;
	DK(*dp, 1);
	DK(dp[1], -2);
	DK(*dp++ + *dp++, -1);
	DK(*dp, 0.5);
	*dp = *dp * 4;
	DK(da[2], 2);
	da[2] = 0.5;
	i = 3;
	DK(da[i], 1024);
	DK(da[i - 1] * da[i], 512);
	da[i - 2] = da[i] / da[0] + i;
	DK(da[1], 1027);
	da[1] = -2;
	DK(fa[i - 1] - fa[0], 97.5);
	DK(gfs.d + gfs.f * gfs.i, -6.75);
	gfs.f = gfs.d;
	DK(gfs.f, -8.25);
	gfs.f = 0.5;
}

static
asgops()
{
	double a;
	float f;
	register double *dp;
	register float *fp;
	int i;

	a = 10;
	a += 2.5;
	DK(a, 12.5);
	a -= 0.5;
	DK(a, 12);
	a *= 1.5;
	DK(a, 18);
	a /= 8;
	DK(a, 2.25);
	a += a;
	DK(a, 4.5);
	a -= gd * 2;
	DK(a, -0.5);
	a *= a;
	DK(a, 0.25);
	a /= a + a;
	DK(a, 0.5);
	DK(a += 1, 1.5);
	DK(a -= 3, -1.5);
	DK(a *= -4, 6);
	DK(a /= 4, 1.5);
	f = 1;
	f += 0.5;
	DK(f, 1.5);
	f *= f;
	DK(f, 2.25);
	f -= 0.25;
	DK(f, 2);
	f /= 16;
	DK(f, 0.125);
	gd += 1;
	DK(gd, 3.5);
	gd -= 1;
	DK(gd, 2.5);
	gf *= 2;
	DK(gf, 2.5);
	gf /= 2;
	DK(gf, 1.25);
	dp = &da[1];
	*dp -= 2;
	DK(da[1], -4);
	*dp /= 2;
	DK(da[1], -2);
	fp = fa;
	*fp++ += 1;
	DK(fa[0], 4);
	*fp++ *= 8;
	DK(fa[1], -1);
	fa[0] = 3;
	fa[1] = -0.125;
	CK(fp == &fa[2], 1);

	/* ++ and -- */
	a = 1.5;
	DK(a++, 1.5);
	DK(a, 2.5);
	DK(++a, 3.5);
	DK(a--, 3.5);
	DK(--a, 1.5);
	a++;
	++a;
	DK(a, 3.5);
	a--;
	--a;
	DK(a, 1.5);
	f = 0.5;
	DK(f++, 0.5);
	DK(++f, 2.5);
	DK(f-- + 1, 3.5);
	DK(2 * f++ - 0.5, 2.5);
	DK(f-- * a, 3.75);
	DK(--f, 0.5);
	gd++;
	DK(gd--, 3.5);
	DK(gd, 2.5);
	dp = da;
	DK((*dp)++, 1);
	DK(*dp, 2);
	--*dp;
	DK(da[0], 1);

	/* an int or a long, and a floating value */
	i = 10;
	i += 2.5;
	CK(i, 12);
	i *= 1.5;
	CK(i, 18);
	i /= 2.5;
	CK(i, 7);
	i -= 0.5;
	CK(i, 6);
	fgl = 100000L;
	fgl *= 2.5;
	LK(fgl, 250000L);
	fgl /= 2.5;
	LK(fgl, 100000L);
	a = 1.5;
	a += i;
	DK(a, 7.5);
	a *= fgl;
	DK(a, 750000.0);
}

static
convert()
{
	double a;
	float f;
	int i;
	long l;
	unsigned u;
	unsigned long ul;
	char c;
	register int r;

	/* made floating */
	a = fgi;
	DK(a, 7);
	a = fgl;
	DK(a, 100000.0);
	a = fgu;
	DK(a, 50000.0);
	a = fgul;
	DK(a, 3000000000.0);
	a = fgc;
	DK(a, -3);
	a = fguc;
	DK(a, 200);
	f = fgi;
	DK(f, 7);
	i = -32768;
	a = i;
	DK(a, -32768.0);
	l = -2147483647L;
	a = l;
	DK(a, -2147483647.0);
	u = 65535;
	a = u;
	DK(a, 65535.0);
	ul = 4294967295;
	a = ul;
	DK(a, 4294967295.0);
	r = -9;
	a = r;
	DK(a, -9);
	DK(r * 0.5, -4.5);
	DK(fgi + 0.5, 7.5);
	DK(0.5 + fgi, 7.5);
	DK(gd * fgi, 17.5);
	DK(gd * fgl, 250000.0);
	DK(gd * fgu, 125000.0);
	DK(fgl / 8.0, 12500);
	DK((fgi + 1) * gd, 20);
	DK((double)fgi / 2, 3.5);
	DK((double)(fgi / 2), 3);
	DK((float)fgi / 2, 3.5);

	/* made an int or a long: the fraction is dropped */
	a = 7.9;
	i = a;
	CK(i, 7);
	a = -7.9;
	i = a;
	CK(i, -7);
	CK((int)2.5, 2);
	CK((int)-2.5, -2);
	CK((int)gd, 2);
	CK((int)ge, 0);
	CK((int)(gd * 3), 7);
	CK((int)(ge * 3), -2);
	a = 100000.75;
	l = a;
	LK(l, 100000L);
	LK((long)(a * -3), -300002L);
	LK((long)(dbig / 8), 1250000000L);
	a = 50000.5;
	u = a;
	CK(u == 50000, 1);
	a = 3000000000.5;
	ul = a;
	CK(ul == 3000000000, 1);
	a = 65.9;
	c = a;
	CK(c, 'A');
	f = 9.99;
	CK((int)f, 9);
	CK((int)(f + 0.02), 10);
	r = gd * 4;
	CK(r, 10);
	CK(fgi + (int)gd, 9);
	CK(fgi * (int)(gd + gd), 35);
	LK(fgl + (long)gd, 100002L);
	CK(fgi > (int)gd, 1);
}

double
ident(x)
double x;
{
	fcalls++;
	return x;
}

static
compare()
{
	double a, b;
	float f;
	int i;

	a = 1.5;
	b = 2.5;
	CK(a < b, 1);
	CK(a <= b, 1);
	CK(a > b, 0);
	CK(a >= b, 0);
	CK(a == b, 0);
	CK(a != b, 1);
	CK(b < a, 0);
	CK(b <= a, 0);
	CK(b > a, 1);
	CK(b >= a, 1);
	CK(a < a, 0);
	CK(a <= a, 1);
	CK(a > a, 0);
	CK(a >= a, 1);
	CK(a == a, 1);
	CK(a != a, 0);
	CK(-a < a, 1);
	CK(-a > -b, 1);
	CK(-b >= -a, 0);

	/* with something computed, and with a function's value */
	CK(a + 1 == b, 1);
	CK(a < b - 1, 0);
	CK(a + a > b + 0.25, 1);
	CK(a * b <= b * a, 1);
	CK(a < ident(b), 1);
	CK(ident(a) < b, 1);
	CK(ident(a) > ident(b), 0);
	CK(b - a >= ident(1.0), 1);
	CK(a + ident(1.0) != b, 0);

	/* with zero, and as true or false */
	CK(a > 0, 1);
	CK(a < 0, 0);
	CK(-a < 0, 1);
	CK(-a >= 0, 0);
	CK(gz == 0, 1);
	CK(gz != 0, 0);
	CK(gz <= 0, 1);
	CK(gz >= 0, 1);
	CK(gz < 0, 0);
	CK(a - 1.5 == 0, 1);
	CK(!a, 0);
	CK(!gz, 1);
	CK(a && b, 1);
	CK(a && gz, 0);
	CK(gz || a, 1);
	CK(gz || gz, 0);
	i = 0;
	if (a)
		i += 1;
	if (gz)
		i += 2;
	if (a - b)
		i += 4;
	if (a - 1.5)
		i += 8;
	if (!gz)
		i += 16;
	if (a < b && b > 2 || gz)
		i += 32;
	while (a < 4)
		a += 1;
	CK(i, 53);
	DK(a, 4.5);

	/* float, int and long against them */
	f = 0.5;
	CK(f < a, 1);
	CK(f == 0.5, 1);
	CK(f * 2 == 1, 1);
	CK(fgi > a, 1);
	CK(fgi == 7.0, 1);
	CK(a < fgi, 1);
	CK(fgl > 99999.5, 1);
	CK(fgu >= 50000.5, 0);
	CK(gf > gd, 0);
	CK(gf <= gd, 1);

	/* ?: */
	DK(a > 4 ? a : b, 4.5);
	DK(a < 4 ? a : b, 2.5);
	DK((gz ? a : b) + 1, 3.5);
	DK(fgi == 7 ? 1.5 : gd, 1.5);
	b = a < 4 ? 1 : fgi;
	DK(b, 7);
	CK(a > 4 ? 3 : 4, 3);
}

double
n_dsum3(a, b, c)
double a, c;
{
	return a + 2 * b + 4 * c;
}

float
n_fhalf(x)
float x;
{
	return x / 2;
}

double
rec(n)
{
	double x;

	if (n == 0)
		return 0.5;
	x = rec(n - 1);
	return x * 2 + n;
}

double
poly(x, n)
double x;
register int n;
{
	register double *dp;
	double s;

	s = 0;
	for (dp = &da[4]; n > 0; n--)
		s = s * x + *--dp;
	return s;
}

static
calls()
{
	double a, b;
	float f;
	register int r;
	register double *dp;
	double (*fp)();

	a = 1.5;
	b = -0.25;
	r = 77;
	dp = &a;

	/* to the other compiler's */
	DK(dsum3(a, 3, b), 6.5);
	DK(dsum3(1.0, 2, 3.0), 17);
	DK(dsum3(a + b, r, a * b), 153.75);
	DK(dsum3(gf, fgi, gfs.f), 17.25);
	DK(dsum3(*dp, 0, 0.0), 1.5);
	DK(fhalf(a), 0.75);
	DK(fhalf(3.0), 1.5);
	f = fhalf(gf);
	DK(f, 0.625);

	/* to this compiler's */
	DK(n_dsum3(a, 3, b), 6.5);
	DK(n_dsum3(1.0, 2, 3.0), 17);
	DK(n_fhalf(a), 0.75);
	f = 5;
	DK(n_fhalf(f), 2.5);
	DK(rec(4), 34);
	DK(poly(2.0, 4), 8191);
	DK(poly(0.5, 4), 128.125);

	/* what a function returns, in the middle of something */
	fcalls = 0;
	DK(a + ident(b), 1.25);
	DK(ident(a) + b, 1.25);
	DK(a - ident(b), 1.75);
	DK(a * ident(b), -0.375);
	DK(a / ident(b), -6);
	DK(ident(a) / ident(b), -6);
	DK(a * b + ident(a) * ident(b) - a / ident(2.0), -1.5);
	DK((a + b) * (ident(a) - (b + ident(b))), 2.5);
	DK(a + (b + (a + (b + ident(8.0)))), 10.5);
	DK(dsum3(ident(a), (int)ident(3.0), ident(b)), 6.5);
	DK(ident(ident(ident(a) + 1) + 1), 3.5);
	DK(a + dscramble(b), 1.25);
	DK(a * dscramble(2.0) + b, 2.75);
	CK(fcalls, 19);
	a += ident(1.0);
	DK(a, 2.5);
	a = 1.5;
	da[(int)ident(2.0)] = a + 1;
	DK(da[2], 2.5);
	da[(int)ident(2.0)] -= 2;
	DK(da[2], 0.5);
	ident(a);
	dscramble(a);

	/* through a pointer, and called back */
	fp = dsum3;
	DK((*fp)(a, 3, b), 6.5);
	fp = n_dsum3;
	DK((*fp)(a, 3, b), 6.5);
	DK(dcallback(n_dsum3, 1.0), 15.5);
	DK(dcallback(dsum3, 1.0), 15.5);

	/* the register variables are as they were */
	CK(r, 77);
	CK(dp == &a, 1);
	DK(*dp, 1.5);
}

t_float()
{
	arith();
	asgops();
	convert();
	compare();
	calls();
}
