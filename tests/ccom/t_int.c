/*
 * Arithmetic on ints, unsigneds and chars, with operands of each kind
 * the code generator tells apart: constants, variables in memory, in
 * registers and on the stack, and things reached through pointers.
 */
#define	CK(got, want)	check(__LINE__, got, want)
#define	U(x)		((unsigned)(x))

int	ga = 1234, gb = -77, gc;
unsigned gu = 60000, gv = 7;
char	gch = -5;
unsigned char guc = 250;
int	garr[8] = { 3, -1, 4, 1, -5, 9, 2, 6 };
char	gstr[] = "hello";
static int sa = 300;

static
ident(x)
{
	return x;
}

static
addsub()
{
	int a, b, c;
	register int r, s;
	int *p;

	a = 100;
	b = -30;
	r = 7;
	s = -9;
	p = &a;
	CK(a + b, 70);
	CK(a - b, 130);
	CK(a + 1, 101);
	CK(a - 1, 99);
	CK(b + -1, -31);
	CK(a + ga, 1334);
	CK(ga + gb, 1157);
	CK(ga - a, 1134);
	CK(r + s, -2);
	CK(r - s, 16);
	CK(r + a, 107);
	CK(a - r, 93);
	CK(*p + b, 70);
	CK(b - *p, -130);
	CK(*p + *p, 200);
	CK(-a, -100);
	CK(-r, -7);
	CK(~a, -101);
	CK(- -b, -30);
	CK(a + b + r + s + ga + gb, 1225);
	CK((a + b) - (r + s), 72);
	CK((a - r) + (b - s), 72);
	CK(ident(a) + ident(b), 70);
	CK(ident(a) - ident(r), 93);
	CK(a - ident(r), 93);
	CK(ident(a + 1) + (b + ident(s)), 62);
	c = a;
	c += b;
	CK(c, 70);
	c -= r;
	CK(c, 63);
	c += 1;
	CK(c, 64);
	c -= 1;
	CK(c, 63);
	c += ident(7);
	CK(c, 70);
	r += a;
	CK(r, 107);
	r -= 100;
	CK(r, 7);
	r += s;
	CK(r, -2);
	ga += 6;
	CK(ga, 1240);
	ga -= a;
	CK(ga, 1140);
	ga = 1234;
	*p += 5;
	CK(a, 105);
	*p -= ident(5);
	CK(a, 100);
	CK(c = a + b, 70);
	CK(c += 5, 75);
	CK(sa + 1, 301);
	sa += sa;
	CK(sa, 600);
	sa = 300;
	/* 16 bits, and they wrap */
	a = 32767;
	a += 1;
	CK(a, -32767 - 1);
	CK(a - 1, 32767);
	gu = 65535;
	gu += 2;
	CK(gu, 1);
	gu = 60000;
}

static
logic()
{
	int a, b;
	register int r;
	int *p;

	a = 0x0FF0;
	b = 0x3C3C;
	r = 0x00FF;
	p = &b;
	CK(a & b, 0x0C30);
	CK(a | b, 0x3FFC);
	CK(a ^ b, 0x33CC);
	CK(a & 0xFF, 0xF0);
	CK(a | 1, 0x0FF1);
	CK(a ^ -1, ~0x0FF0);
	CK(a & r, 0xF0);
	CK(r | a, 0x0FFF);
	CK(r ^ *p, 0x3CC3);
	CK(*p & a, 0x0C30);
	CK(a & ~b, 0x03C0);
	CK(ident(a) & ident(b), 0x0C30);
	CK(ident(a) | (b & r), 0x0FFC);
	a &= b;
	CK(a, 0x0C30);
	a |= 3;
	CK(a, 0x0C33);
	a ^= r;
	CK(a, 0x0CCC);
	r &= 0x0F;
	CK(r, 0x0F);
	r |= b;
	CK(r, 0x3C3F);
	r ^= r;
	CK(r, 0);
	*p &= 0xFF00;
	CK(b, 0x3C00);
	*p |= ident(5);
	CK(b, 0x3C05);
	*p ^= a;
	CK(b, 0x30C9);
	gc = 0x5555;
	gc &= 0x0F0F;
	CK(gc, 0x0505);
	gc |= a;
	CK(gc, 0x0DCD);
	CK(!a, 0);
	CK(!r, 1);
	CK(!!a, 1);
}

static
shifts()
{
	int a, n;
	unsigned u;
	register int r;
	register unsigned ru;

	a = -256;
	u = 0xFF00;
	n = 3;
	r = 5;
	ru = 0x8001;
	CK(a << 1, -512);
	CK(a << 3, -2048);
	CK(a >> 1, -128);
	CK(a >> 4, -16);
	CK(u >> 1, 0x7F80);
	CK(u >> 8, 0xFF);
	CK(u << 4, 0xF000);
	CK(a << n, -2048);
	CK(a >> n, -32);
	CK(u >> n, 0x1FE0);
	CK(r << n, 40);
	CK(r << 2, 20);
	CK(ru >> 1, 0x4000);
	CK(ru >> 15, 1);
	CK(ru << 1, 2);
	CK(1 << n, 8);
	CK(a >> ident(2), -64);
	CK(ident(u) >> ident(4), -16);
	CK(ident(3) << ident(n), 24);
	a <<= 2;
	CK(a, -1024);
	a >>= 3;
	CK(a, -128);
	a >>= n;
	CK(a, -16);
	a <<= n;
	CK(a, -128);
	u >>= 4;
	CK(u, 0x0FF0);
	u <<= n;
	CK(u, 0x7F80);
	u >>= n;
	CK(u, 0x0FF0);
	r <<= 3;
	CK(r, 40);
	r >>= 1;
	CK(r, 20);
	ru >>= 4;
	CK(ru, 0x0800);
	gc = -8;
	gc >>= 1;
	CK(gc, -4);
	gu >>= 2;
	CK(gu, 15000);
	gu <<= 2;
	CK(gu, U(60000));
}

static
muldiv()
{
	int a, b, c;
	unsigned u, v;
	register int r;
	int *p;

	a = 123;
	b = -7;
	u = 50000;
	v = 300;
	r = 11;
	p = &b;
	CK(a * b, -861);
	CK(a * 5, 615);
	CK(a * 8, 984);
	CK(a * -1, -123);
	CK(a * r, 1353);
	CK(r * *p, -77);
	CK(*p * *p, 49);
	CK(a / b, -17);
	CK(a % b, 4);
	CK(b / 2, -3);
	CK(b % 2, -1);
	CK(-a / 10, -12);
	CK(-a % 10, -3);
	CK(a / r, 11);
	CK(a % r, 2);
	CK(a / *p, -17);
	CK(u / v, 166);
	CK(u % v, 200);
	CK(u / 7, 7142);
	CK(u % 7, 6);
	CK(u / 2, 25000);
	CK(u / 16, 3125);
	CK(u % 16, 0);
	CK(u % 256, 80);
	CK(u * 2, U(34464));
	CK(u / U(50000), 1);
	CK(u / U(50001), 0);
	CK(ident(a) * ident(b), -861);
	CK(ident(a) / ident(b), -17);
	CK(ident(a) % ident(r), 2);
	CK(a * b + a / r - a % r, -852);
	CK((a + b) * (a - b), 15080);
	CK((a + b) / (r - 1), 11);
	c = a;
	c *= b;
	CK(c, -861);
	c /= 3;
	CK(c, -287);
	c %= 100;
	CK(c, -87);
	c *= 2;
	CK(c, -174);
	c /= r;
	CK(c, -15);
	r *= 3;
	CK(r, 33);
	r /= 2;
	CK(r, 16);
	r %= 5;
	CK(r, 1);
	u /= v;
	CK(u, 166);
	u *= v;
	CK(u, U(49800));
	u %= 1000;
	CK(u, 800);
	u /= 8;
	CK(u, 100);
	*p *= a;
	CK(b, -861);
	*p /= ident(-7);
	CK(b, 123);
	*p %= 100;
	CK(b, 23);
	ga *= 2;
	CK(ga, 2468);
	ga /= 2;
	CK(ga, 1234);
	ga %= 1000;
	CK(ga, 234);
	ga = 1234;
	gu /= gv;
	CK(gu, 8571);
	gu = 60000;
	CK(gu % gv, 3);
}

static
chars()
{
	char c, d;
	unsigned char uc;
	char *cp;
	register int r;
	int i;

	c = 100;
	d = -3;
	uc = 200;
	cp = &c;
	CK(c, 100);
	CK(d, -3);
	CK(uc, 200);
	CK(c + d, 97);
	CK(c - d, 103);
	CK(uc + c, 300);
	CK(uc + uc, 400);
	CK(c * d, -300);
	CK(uc / 3, 66);
	CK(d / 2, -1);
	CK(gch, -5);
	CK(guc, 250);
	CK(gch + guc, 245);
	CK(*cp + 1, 101);
	CK(gstr[0], 'h');
	CK(gstr[4] - gstr[1], 'o' - 'e');
	CK(-d, 3);
	CK(~uc, -201);
	CK(c & 0x0F, 4);
	CK(uc & 0xF0, 0xC0);
	CK(d & 0xFF, 0xFD);
	CK(uc >> 3, 25);
	CK(d >> 1, -2);
	CK(c << 2, 400);
	c += 27;
	CK(c, 127);
	c += 1;
	CK(c, -128);
	c -= 1;
	CK(c, 127);
	uc += 56;
	CK(uc, 0);
	uc -= 1;
	CK(uc, 255);
	uc += d;
	CK(uc, 252);
	d *= 5;
	CK(d, -15);
	d /= 4;
	CK(d, -3);
	d <<= 2;
	CK(d, -12);
	d >>= 1;
	CK(d, -6);
	uc >>= 2;
	CK(uc, 63);
	uc |= 0x80;
	CK(uc, 191);
	uc &= d;
	CK(uc, 186);
	*cp = 65;
	CK(c, 65);
	*cp += 1;
	CK(c, 66);
	(*cp)++;
	CK(c, 67);
	CK(c++, 67);
	CK(c, 68);
	CK(--c, 67);
	CK(uc--, 186);
	CK(++uc, 186);
	/* a char takes the low byte of what is put in it */
	i = 0x1234;
	c = i;
	CK(c, 0x34);
	i = 0x12F0;
	c = i;
	CK(c, -16);
	uc = i;
	CK(uc, 0xF0);
	r = 0x4181;
	c = r;
	CK(c, -127);
	uc = r;
	CK(uc, 0x81);
	CK((char)i, -16);
	CK((char)r, -127);
	CK((unsigned char)r, 0x81);
	CK(c = 300, 44);
	CK(uc = -1, 255);
	gch = guc;
	CK(gch, -6);
	guc = gch;
	CK(guc, 250);
	gch = -5;
	i = gch;
	CK(i, -5);
	i = guc;
	CK(i, 250);
	r = gch;
	CK(r, -5);
	r = guc;
	CK(r, 250);
}

static
incdec()
{
	int a, b;
	register int r;
	int *p;
	char *cp;
	register char *rp;

	a = 10;
	r = 20;
	p = garr;
	CK(a++, 10);
	CK(a, 11);
	CK(++a, 12);
	CK(a--, 12);
	CK(--a, 10);
	CK(r++, 20);
	CK(++r, 22);
	CK(r--, 22);
	CK(--r, 20);
	CK(ga++, 1234);
	CK(--ga, 1234);
	b = a++ + r++;
	CK(b, 30);
	CK(a + r, 32);
	b = --a - --r;
	CK(b, -10);
	CK(*p++, 3);
	CK(*p, -1);
	CK(*++p, 4);
	CK(*p--, 4);
	CK(*--p, 3);
	CK((*p)++, 3);
	CK(*p, 4);
	CK(--*p, 3);
	p += 2;
	CK(*p, 4);
	p -= 1;
	CK(*p, -1);
	CK(p - garr, 1);
	cp = gstr;
	CK(*cp++, 'h');
	CK(*cp++, 'e');
	CK(*cp, 'l');
	CK(cp - gstr, 2);
	rp = gstr;
	CK(*rp++, 'h');
	CK(*rp++ + *rp++, 'e' + 'l');
	CK(*rp, 'l');
	CK(*--rp, 'l');
	CK(rp - gstr, 2);
	for (b = 0, rp = gstr; *rp++; )
		b++;
	CK(b, 5);
	for (b = 0, rp = gstr; *rp; rp++)
		b += *rp;
	CK(b, 'h' + 'e' + 'l' + 'l' + 'o');
}

t_int()
{
	addsub();
	logic();
	shifts();
	muldiv();
	chars();
	incdec();
}
