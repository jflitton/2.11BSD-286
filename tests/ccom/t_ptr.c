/*
 * Pointers and arrays, structures and fields, and calls: to and from
 * functions the other compiler compiled, through pointers, and with
 * register variables that are to come back unharmed.
 */
#define	CK(got, want)	check(__LINE__, got, want)
#define	LK(got, want)	lcheck(__LINE__, (long)(got), (long)(want))

struct pt {
	int	x, y;
};
struct rec {
	char	tag;
	int	n;
	long	l;
	struct pt p;
	char	name[6];
	struct rec *next;
};
struct odd {
	char	a[3];
	int	v;
	char	b;
};
struct bits {
	unsigned f1 : 3;
	unsigned f2 : 5;
	unsigned f3 : 1;
	unsigned f4 : 7;
	int	after;
};
union un {
	int	i;
	char	c[2];
	long	l;
};

int	ia[10] = { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };
char	ca[10] = "abcdefghi";
long	la[5] = { 100000L, 200000L, 300000L, 400000L, 500000L };
int	m2[3][4] = { { 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { 9, 10, 11, 12 } };
struct pt pts[4] = { { 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 } };
struct rec r1 = { 'A', 100, 100000L, { 7, 8 }, "one" };
struct rec r2, r3;
struct odd odds[3];
struct bits gbits;
char	*names[] = { "zero", "one", "two", "three" };
int	*ipp = &ia[3];
char	buf[32];

static
ident(x)
{
	return x;
}

/* the address of characters, as that of something else */
unsigned char pack[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

struct pt *
aspt(i)
{
	return (struct pt *)&pack[i];
}

static
casts()
{
	struct pt *pp;
	long *lp;

	pp = (struct pt *)pack;
	CK(pp->x, 0x0201);
	CK(pp->y, 0x0403);
	pp = (struct pt *)&pack[4];
	CK(pp->x, 0x0605);
	CK(aspt(8)->y, 0x0c0b);
	CK(((struct pt *)r1.name)->x, 'o' + ('n' << 8));
	lp = (long *)&pack[2];
	LK(*lp, 0x06050403L);
	CK(*(int *)&pack[10], 0x0c0b);
}

static
arrays()
{
	int i, j;
	register int r;
	int *p, *q;
	register int *rp;
	char *cp;
	long *lp;
	int loc[6];

	i = 3;
	r = 4;
	CK(ia[0], 10);
	CK(ia[9], 19);
	CK(ia[i], 13);
	CK(ia[r], 14);
	CK(ia[i + 1], 14);
	CK(ia[i + r], 17);
	CK(ia[i - 1], 12);
	CK(ia[ident(5)], 15);
	CK(ia[ia[0] - 10], 10);
	CK(ca[0], 'a');
	CK(ca[i], 'd');
	CK(ca[r], 'e');
	CK(ca[i + r], 'h');
	CK(ca[ident(2)], 'c');
	CK(i[ca], 'd');
	LK(la[0], 100000L);
	LK(la[i], 400000L);
	LK(la[r], 500000L);
	LK(la[i - 2] + la[r - 2], 500000L);
	CK(m2[0][0], 1);
	CK(m2[2][3], 12);
	CK(m2[1][i], 8);
	CK(m2[i - 1][r - 2], 11);
	CK(m2[ident(1)][ident(1)], 6);
	CK(*ipp, 13);
	CK(ipp[1], 14);
	CK(ipp[-1], 12);
	CK(*names[2], 't');
	CK(names[3][4], 'e');
	CK(names[i][r], 'e');
	ia[i] = 99;
	CK(ia[3], 99);
	ia[i] += 1;
	CK(ia[3], 100);
	ia[r]++;
	CK(ia[4], 15);
	ia[i + r] = ia[i] - ia[r];
	CK(ia[7], 85);
	ia[3] = 13;
	ia[4] = 14;
	ia[7] = 17;
	ca[i] = 'X';
	CK(ca[3], 'X');
	ca[r] += 1;
	CK(ca[4], 'f');
	ca[3] = 'd';
	ca[4] = 'e';
	la[i] = la[0] + la[1];
	LK(la[3], 300000L);
	la[3] = 400000L;
	m2[1][2] = m2[2][1] * m2[0][3];
	CK(m2[1][2], 40);
	m2[1][2] = 7;
	for (j = 0; j < 6; j++)
		loc[j] = j * j;
	CK(loc[5], 25);
	CK(loc[i] + loc[r], 25);
	loc[i] = loc[i - 1] + loc[i + 1];
	CK(loc[3], 20);
	/* pointers */
	p = ia;
	q = &ia[6];
	rp = &ia[2];
	CK(*p, 10);
	CK(*q, 16);
	CK(*rp, 12);
	CK(*(p + 3), 13);
	CK(*(p + i), 13);
	CK(*(q - i), 13);
	CK(*(rp + r), 16);
	CK(p[r], 14);
	CK(rp[i], 15);
	CK(rp[-1], 11);
	CK(q - p, 6);
	CK(p - q, -6);
	CK(q - rp, 4);
	CK(rp - p, 2);
	CK(&ia[9] - &ia[0], 9);
	CK(q > p, 1);
	CK(p + 6 == q, 1);
	CK(p + 5 != q, 1);
	p++;
	CK(*p, 11);
	p += 2;
	CK(*p, 13);
	p += i;
	CK(*p, 16);
	p -= r;
	CK(*p, 12);
	p = p + r - 1;
	CK(*p, 15);
	--p;
	CK(*p, 14);
	rp += 2;
	CK(*rp, 14);
	rp -= i;
	CK(*rp, 11);
	rp = p + 1;
	CK(*rp, 15);
	rp = &p[2];
	CK(*rp, 16);
	*rp = 77;
	CK(ia[6], 77);
	*rp-- = 16;
	CK(*rp, 15);
	CK(ia[6], 16);
	*p = *rp + *q;
	CK(ia[4], 31);
	*p = 14;
	lp = la;
	LK(*lp, 100000L);
	LK(lp[2], 300000L);
	LK(*(lp + i), 400000L);
	lp += 2;
	LK(*lp++, 300000L);
	LK(*lp, 400000L);
	CK(lp - la, 3);
	CK(&la[4] - lp, 1);
	LK(*--lp, 300000L);
	cp = ca + 2;
	CK(*cp, 'c');
	CK(cp[1], 'd');
	CK(cp[-2], 'a');
	CK(cp - ca, 2);
	CK(*(cp + i), 'f');
	p = (int *)0;
	CK(p == 0, 1);
	CK(!p, 1);
	p = ia;
	CK(p != 0, 1);
	CK(!p, 0);
	if (p && *p == 10)
		i = 1;
	else
		i = 0;
	CK(i, 1);
}

static
strings()
{
	register char *s, *d;
	char *t;
	int n;
	char local[12];

	d = buf;
	s = "copy me";
	while (*d++ = *s++)
		;
	CK(buf[0], 'c');
	CK(buf[6], 'e');
	CK(buf[7], 0);
	CK(d - buf, 8);
	n = 0;
	for (s = buf; *s; s++)
		n++;
	CK(n, 7);
	n = 0;
	s = buf;
	while (*s++)
		n++;
	CK(n, 7);
	n = 0;
	s = buf;
	while (*s != '\0') {
		if (*s == ' ')
			*s = '_';
		s++;
		n++;
	}
	CK(n, 7);
	CK(buf[4], '_');
	t = "abc";
	CK(t[0] + t[1] + t[2], 'a' + 'b' + 'c');
	CK("xyz"[1], 'y');
	CK(*"q", 'q');
	d = local;
	t = buf;
	while ((*d = *t) != 0) {
		d++;
		t++;
	}
	CK(local[0], 'c');
	CK(local[7], 0);
	/* backwards */
	s = buf + 7;
	d = local;
	while (s > buf)
		*d++ = *--s;
	*d = 0;
	CK(local[0], 'e');
	CK(local[6], 'c');
	n = 0;
	for (s = local; *s; )
		n = n * 2 + (*s++ & 1);
	CK(n, 0x7B);
}

static
structs()
{
	struct rec loc;
	struct rec *rp;
	register struct rec *rr;
	struct pt *pp, pa, pb;
	struct odd *op;
	union un u;
	int i;

	CK(r1.tag, 'A');
	CK(r1.n, 100);
	LK(r1.l, 100000L);
	CK(r1.p.x, 7);
	CK(r1.p.y, 8);
	CK(r1.name[1], 'n');
	CK(sizeof(struct pt), 4);
	CK(sizeof(struct rec), 18 + sizeof(struct rec *));
	CK(sizeof(struct odd), 8);
	rp = &r1;
	rr = &r1;
	CK(rp->tag, 'A');
	CK(rp->n, 100);
	LK(rp->l, 100000L);
	CK(rp->p.y, 8);
	CK(rp->name[2], 'e');
	CK(rr->tag, 'A');
	CK(rr->n + rr->p.x, 107);
	LK(rr->l + rr->n, 100100L);
	CK(rr->name[0], 'o');
	rp->n = 5;
	CK(r1.n, 5);
	rr->n += 95;
	CK(r1.n, 100);
	rr->p.x = rr->p.y * 2;
	CK(r1.p.x, 16);
	rp->p.x = 7;
	rr->tag++;
	CK(r1.tag, 'B');
	rp->tag = 'A';
	rr->l += 5;
	LK(r1.l, 100005L);
	rp->l = 100000L;
	/* whole structures */
	r2 = r1;
	CK(r2.tag, 'A');
	CK(r2.n, 100);
	LK(r2.l, 100000L);
	CK(r2.p.y, 8);
	CK(r2.name[2], 'e');
	r2.n = 200;
	r2.next = &r1;
	loc = r2;
	CK(loc.n, 200);
	CK(loc.next->n, 100);
	rp = &r3;
	*rp = loc;
	CK(r3.n, 200);
	CK(r3.name[0], 'o');
	rr = &r2;
	rr->n = 300;
	*rp = *rr;
	CK(r3.n, 300);
	r3.next = &r2;
	r2.next = &r3;
	CK(r2.next->next->n, 300);
	CK(rr->next->next->next->n, 300);
	rr = rr->next;
	CK(rr == &r3, 1);
	pa.x = 1;
	pa.y = 2;
	pb = pa;
	CK(pb.x + pb.y, 3);
	pp = &pts[1];
	CK(pp->x, 3);
	CK(pp[1].y, 6);
	CK((pp + 2)->x, 7);
	CK(pp - pts, 1);
	CK(&pts[3] - pp, 2);
	pp++;
	CK(pp->x, 5);
	*pp = pa;
	CK(pts[2].y, 2);
	pts[2] = pts[3];
	CK(pts[2].x, 7);
	pts[2].x = 5;
	pts[2].y = 6;
	i = 2;
	CK(pts[i].x, 5);
	CK(pts[i + 1].y, 8);
	pts[i].y += pts[i - 1].y;
	CK(pts[2].y, 10);
	pts[2].y = 6;
	/* a structure whose size is no power of two */
	for (i = 0; i < 3; i++) {
		odds[i].v = i * 100;
		odds[i].a[2] = 'a' + i;
		odds[i].b = 'x' + i;
	}
	CK(odds[2].v, 200);
	CK(odds[1].a[2], 'b');
	CK(odds[2].b, 'z');
	op = &odds[2];
	CK(op - odds, 2);
	CK(op->v, 200);
	op--;
	CK(op->b, 'y');
	CK(&odds[2] - op, 1);
	odds[0] = odds[2];
	CK(odds[0].v, 200);
	CK(odds[0].b, 'z');
	/* unions */
	u.l = 0x12345678L;
	CK(u.i, 0x5678);
	CK(u.c[0], 0x78);
	CK(u.c[1], 0x56);
	u.c[0] = 1;
	CK(u.i, 0x5601);
	LK(u.l, 0x12345601L);
}

static
fields()
{
	struct bits b;
	register struct bits *bp;

	b.after = 0x5A5A;
	b.f1 = 5;
	b.f2 = 19;
	b.f3 = 1;
	b.f4 = 100;
	CK(b.f1, 5);
	CK(b.f2, 19);
	CK(b.f3, 1);
	CK(b.f4, 100);
	CK(b.after, 0x5A5A);
	b.f2 = 0;
	CK(b.f1, 5);
	CK(b.f2, 0);
	CK(b.f3, 1);
	b.f2 = 31;
	CK(b.f2, 31);
	CK(b.f4, 100);
	b.f1 = 9;
	CK(b.f1, 1);
	CK(b.f2, 31);
	b.f3 = 0;
	CK(b.f3, 0);
	CK(b.f4, 100);
	b.f4 += 20;
	CK(b.f4, 120);
	b.f4++;
	CK(b.f4, 121);
	b.f2 -= 1;
	CK(b.f2, 30);
	b.f2 |= 1;
	CK(b.f2, 31);
	b.f2 &= 6;
	CK(b.f2, 6);
	CK(b.f1 + b.f2, 7);
	CK(b.f2 == 6, 1);
	CK(b.f3 ? 1 : 2, 2);
	if (b.f1)
		b.f3 = 1;
	CK(b.f3, 1);
	CK(b.after, 0x5A5A);
	bp = &gbits;
	bp->f1 = 7;
	bp->f2 = ident(21);
	bp->f4 = 127;
	CK(gbits.f1, 7);
	CK(gbits.f2, 21);
	CK(gbits.f3, 0);
	CK(gbits.f4, 127);
	bp->f4 = bp->f1 + bp->f2;
	CK(bp->f4, 28);
	bp->f1 = 0;
	CK(bp->f2, 21);
	CK(bp->f4, 28);
}

static
add2(a, b)
{
	return a + b;
}

static
sub2(a, b)
{
	return a - b;
}

static
mul2(a, b)
{
	return a * b;
}

static int (*optab[])() = { add2, sub2, mul2 };

struct ops {
	int	code;
	int	(*fn)();
};
static struct ops ops[] = { { 1, add2 }, { 2, sub2 }, { 3, mul2 } };

static
apply(f, a, b)
int (*f)();
{
	return (*f)(a, b);
}

static
many(a, b, c, d, e, f, g, h)
char c;
long e;
{
	return a - b + c - d + (int)e - f + g - h;
}

static
fib(n)
{
	if (n < 2)
		return n;
	return fib(n - 1) + fib(n - 2);
}

static
ack(m, n)
{
	if (m == 0)
		return n + 1;
	if (n == 0)
		return ack(m - 1, 1);
	return ack(m - 1, ack(m, n - 1));
}

static char
retc(x)
{
	return x;
}

static unsigned
retu(x)
{
	return x;
}

static
counter()
{
	static int n = 5;

	return n++;
}

static int *
retp(i)
{
	return &ia[i];
}

static
calls()
{
	int (*fp)();
	register int (*rfp)();
	register int r, s;
	int i;
	struct ops *op;
	extern int sum5(), callback(), scramble();
	extern char lowbyte();

	CK(add2(3, 4), 7);
	CK(sub2(3, 4), -1);
	CK(mul2(add2(1, 2), sub2(9, 4)), 15);
	CK(add2(add2(add2(1, 2), 3), add2(4, add2(5, 6))), 21);
	fp = add2;
	CK((*fp)(10, 20), 30);
	CK(fp(10, 21), 31);
	fp = sub2;
	CK((*fp)(10, 20), -10);
	rfp = mul2;
	CK((*rfp)(6, 7), 42);
	CK(rfp(6, 8), 48);
	CK((*optab[0])(8, 2), 10);
	CK((*optab[1])(8, 2), 6);
	i = 2;
	CK((*optab[i])(8, 2), 16);
	op = &ops[1];
	CK((*op->fn)(9, 4), 5);
	CK((*ops[2].fn)(9, 4), 36);
	CK(apply(add2, 5, 6), 11);
	CK(apply(optab[2], 5, 6), 30);
	CK(apply(fp, 5, 6), -1);
	CK(many(1, 2, 3, 4, 5L, 6, 7, 8), -4);
	CK(many(100, 0, 'a', 0, 70000L, 0, 0, 0), 100 + 'a' + 4464);
	CK(fib(15), 610);
	CK(ack(2, 3), 9);
	CK(retc(0x1234), 0x34);
	CK(retc(0x12F0), -16);
	CK(retu(-1) > 0, 1);
	CK(counter(), 5);
	CK(counter(), 6);
	CK(counter() + counter(), 15);
	CK(*retp(4), 14);
	CK(retp(4)[1], 15);
	*retp(4) = 44;
	CK(ia[4], 44);
	ia[4] = 14;
	/* the other compiler's */
	CK(sum5(1, 2, 3, 4, 5), 55);
	CK(sum5(add2(1, 0), 2, ident(3), 4, mul2(5, 1)), 55);
	CK(lowbyte(0x1281), -127);
	CK(callback(add2, 10), 22);
	CK(callback(mul2, 10), 111);
	CK(callback(optab[1], 10), 0);
	/* register variables across calls */
	r = 1234;
	s = -4321;
	rfp = add2;
	i = scramble(3);
	CK(r, 1234);
	CK(s, -4321);
	i = fib(10);
	CK(i, 55);
	CK(r + s, -3087);
	CK((*rfp)(r, s), -3087);
	r = scramble(r) & 0 | r;
	CK(r, 1234);
	CK(add2(r, scramble(1) & 0), 1234);
	CK(add2(scramble(1) & 0, s), -4321);
}

t_ptr()
{
	casts();
	arrays();
	strings();
	structs();
	fields();
	calls();
}
