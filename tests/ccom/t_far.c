/*
 * Far pointers: what they point at, arithmetic on them, which is on
 * the offset alone, their making from near pointers and back, and
 * calls through them.  Some are to and from the other compiler's
 * functions (support.c), which has functions that are far besides:
 * this compiler has none in the small model, where the tests are.
 */
#include "structs.h"

#define	CK(got, want)	check(__LINE__, got, want)
#define	LK(got, want)	lcheck(__LINE__, (long)(got), (long)(want))

struct fnode {
	int	val;
	long	big;
	char	name[6];
	struct fnode __far *next;
	char __far *text;
	double	d;
};

struct fops {
	int	n;
	int	(__far *fn)();
};

/*
 * The other compiler's.
 */
char __far *fr_addr();
int (__far *fr_func())();
long fr_seg();
long fr_proto(char __far *p, int __far *q, int n);
struct s4 mk4();
struct s6 mk6();
struct s24 mk24();

struct fnode fnodes[3];
char	fbuf[16];
int	fints[8];
long	flongs[4];
double	fdbl[2];
struct s4 fs4;
struct s6 fs6;
struct s24 fs24, ft24;
struct fops fops;

char __far *fcp;
int __far *fip;
struct fnode __far *fnp;
int (__far *ffn)();
int (__far *ftab[2])();
char __far *fnull = 0;

static char __far *
pass(p, i)
char __far *p;
{
	return(p + i);
}

static
sizes()
{
	CK(sizeof(fcp), 4);
	CK(sizeof(char __far *), 4);
	CK(sizeof(ffn), 4);
	CK(sizeof(ftab), 8);
	CK(sizeof(struct fnode), 28);
	CK(sizeof(fnodes), 84);
	CK((char *)&fnodes[0].next - (char *)&fnodes[0], 12);
	CK((char *)&fnodes[0].text - (char *)&fnodes[0], 16);
	CK((char *)&fnodes[0].d - (char *)&fnodes[0], 20);
	CK(sizeof(struct fops), 6);
	CK(sizeof(*fnp), 28);
	CK(sizeof(fnp->next), 4);
	CK(sizeof(*fip), 2);
}

static
convert()
{
	char *np;
	long l;

	fcp = fbuf;
	LK((long)fcp & 0xFFFFL, (unsigned)fbuf);
	LK((long)fcp >> 16, fr_seg());
	np = fbuf + 3;
	fcp = np;
	LK((long)fcp >> 16, fr_seg());
	CK((int)fcp, (int)fbuf + 3);
	LK(fnull, 0);
	fcp = (char __far *)fbuf;
	np = (char *)fcp;
	CK(np == fbuf, 1);
	l = (long)fcp;
	fip = (int __far *)l;
	CK((char __far *)fip == fcp, 1);
	fip = fints;
	fcp = (char __far *)fip;
	CK((char *)fcp == (char *)fints, 1);
	fcp = (char __far *)0x12345678L;
	LK(fcp, 0x12345678L);
	CK((int)fcp, 0x5678);
	CK((char)(int)fcp, 0x78);
	fcp = "abc";
	CK(*fcp, 'a');
	CK(fcp[2], 'c');
	LK((long)fcp >> 16, fr_seg());
	fcp = 0;
	LK(fcp, 0);
}

static
deref()
{
	register i;
	int __far *ip;
	char __far *cp;
	long __far *lp;
	double __far *dp;

	for (i = 0; i < 8; i++)
		fints[i] = 100 + i;
	ip = fints;
	CK(*ip, 100);
	CK(ip[3], 103);
	CK(*(ip + 5), 105);
	CK(ip[i - 2], 106);
	*ip = 7;
	CK(fints[0], 7);
	ip[i - 1] = 9;
	CK(fints[7], 9);
	ip[2] += 40;
	CK(fints[2], 142);
	ip[2]++;
	++ip[2];
	CK(fints[2], 144);
	CK(ip[2]--, 144);
	CK(fints[2], 143);
	ip[1] *= 3;
	CK(fints[1], 303);
	ip[1] <<= 1;
	CK(fints[1], 606);
	ip[1] /= 6;
	CK(fints[1], 101);
	ip[4] = ip[3] + ip[5] * 2;
	CK(fints[4], 313);
	ip[6] = ip[i - 4] & 0xF0;
	CK(fints[6], 0x30);
	fip = ip;
	CK(*fip + fip[1], 108);
	CK(fip[3] == 103, 1);
	CK(fip[3] > fip[5], 0);
	CK(fr_sum(ip, 3), 7 + 101 + 143);

	cp = fbuf;
	cp[0] = 'x';
	cp[1] = -3;
	CK(fbuf[0], 'x');
	CK(cp[1], -3);
	CK(*cp == 'x', 1);
	cp[2] = cp[0] + 1;
	CK(fbuf[2], 'y');
	cp[1] &= 0x7F;
	CK(fbuf[1], 0x7D);
	if (cp[2] != 'y')
		CK(0, 1);
	CK(fr_peek(cp, 2), 'y');
	fcp = fr_addr();		/* not in this segment, or not at
					   this address in it */
	CK(*fcp, 'f');
	CK(fcp[4], 'a');
	fcp[1] = 'o';
	CK(fr_peek(fcp, 1), 'o');
	CK(fr_peek(fcp + 4, 3), 'y');

	lp = flongs;
	lp[1] = 100000L;
	*lp = lp[1] * 3;
	LK(flongs[0], 300000L);
	lp[1] += 5;
	lp[1] <<= 2;
	LK(flongs[1], 400020L);
	lp[2] = lp[1] / 7;
	LK(flongs[2], 57145L);
	lp[2] %= 1000;
	LK(flongs[2], 145L);
	lp[3] = -lp[0];
	LK(lp[3], -300000L);
	CK(lp[1] > 400000L, 1);
	CK(lp[3] < 0, 1);
	CK(lp[2] == 145, 1);
	lp[i - 6]++;
	LK(flongs[2], 146L);
	LK(lp[2]--, 146L);
	LK(flongs[2], 145L);
	lp[2] *= lp[2];
	LK(flongs[2], 21025L);

	dp = fdbl;
	dp[0] = 1.5;
	dp[1] = dp[0] * 4;
	CK((int)(dp[1] * 2), 12);
	dp[1] += dp[0];
	CK((int)(fdbl[1] * 2), 15);
	dp[0] = ip[3];
	CK((int)fdbl[0], 103);
	CK(dp[1] > dp[0], 0);
}

static
arith()
{
	char __far *cp;
	char __far *cq;
	int __far *ip;
	int n;

	cp = fbuf;
	cq = cp + 5;
	CK(cq - cp, 5);
	CK((char *)cq - fbuf, 5);
	cq -= 2;
	CK(cq - cp, 3);
	n = 4;
	cq = cp + n;
	CK(cq - cp, 4);
	cq = n + cp;
	CK(cq - cp, 4);
	cq = cp + n * 2 - 1;
	CK(cq - cp, 7);
	cq -= n;
	CK(cq - cp, 3);
	cq++;
	++cq;
	CK(cq - cp, 5);
	cq--;
	--cq;
	CK(cq - cp, 3);
	CK(cq - fbuf, 3);
	ip = fints;
	ip += 3;
	CK(ip - (int __far *)fints, 3);
	fints[3] = 33;
	fints[4] = 44;
	CK(*ip++, 33);
	CK(ip - (int __far *)fints, 4);
	CK(*ip, 44);
	CK(*--ip, 33);
	CK(*++ip, 44);
	CK(*ip--, 44);
	CK(ip - (int __far *)fints, 3);
	for (cp = fbuf, n = 0; n < 10; n++)
		*cp++ = 'a' + n;
	CK(fbuf[9], 'j');
	CK(cp - (char __far *)fbuf, 10);
	while (cp > (char __far *)fbuf)
		*--cp += 1;
	CK(fbuf[0], 'b');
	CK(fbuf[9], 'k');
	fcp = fbuf;
	fcp += 9;
	CK(*fcp, 'k');
	fcp--;
	CK(*fcp, 'j');
	CK(*fcp++, 'j');
	CK(*fcp, 'k');
	fcp -= n - 1;
	CK(*fcp, 'b');
	LK((long)(cp + 1) >> 16, (long)cp >> 16);
	fcp = pass((char __far *)0x12340010L, n);
	LK(fcp, 0x1234001AL);
	fcp -= 0x10;
	LK(fcp, 0x1234000AL);
	LK(fcp + n, 0x12340014L);
	LK(fcp - n, 0x12340000L);
	fcp++;
	LK(fcp, 0x1234000BL);
}

static
members()
{
	struct fnode __far *p;
	struct fnode __far *q;
	int n;

	fnodes[0].val = 11;
	fnodes[1].val = 22;
	fnodes[2].val = 33;
	fnodes[0].next = &fnodes[1];
	fnodes[1].next = &fnodes[2];
	fnodes[2].next = 0;
	fnodes[1].text = "text";
	p = fnodes;
	CK(p->val, 11);
	CK(p->next->val, 22);
	CK(p->next->next->val, 33);
	CK(p->next->next->next == 0, 1);
	CK(p[1].val, 22);
	CK((p + 2)->val, 33);
	CK((*p).val, 11);
	p->big = 123456L;
	p->big += p->val;
	LK(fnodes[0].big, 123467L);
	p->name[0] = 'n';
	p->name[5] = 'e';
	CK(fnodes[0].name[5], 'e');
	q = p->next;
	q->name[1] = 'q';
	CK(fnodes[1].name[1], 'q');
	CK(*q->text, 't');
	CK(q->text[3], 't');
	CK(p->next->text[1], 'e');
	q->d = 2.5;
	q->d *= 2;
	CK((int)q->d, 5);
	fcp = p->name;
	CK(*fcp, 'n');
	fcp = &p->name[5];
	CK(*fcp, 'e');
	fip = &q->val;
	*fip = 77;
	CK(fnodes[1].val, 77);
	for (n = 0, q = p; q; q = q->next)
		n += q->val;
	CK(n, 11 + 77 + 33);
	q = p->next;
	CK(q - p, 1);
	fnp = p;
	CK(fnp->next->val, 77);
	fnp->next->val++;
	CK(fnodes[1].val, 78);
	fnp->next->next->big = fnp->big * 2;
	LK(fnodes[2].big, 246934L);
	fnp++;
	CK(fnp->val, 78);
	CK(fnp[-1].val, 11);
	CK(fnp[1].val, 33);
}

static
compare()
{
	char __far *a;
	char __far *b;

	a = fbuf;
	b = fbuf + 3;
	CK(a < b, 1);
	CK(a > b, 0);
	CK(a <= b, 1);
	CK(a >= b, 0);
	CK(a == b, 0);
	CK(a != b, 1);
	b -= 3;
	CK(a == b, 1);
	CK(a <= b, 1);
	CK(a < b, 0);
	CK(a == fbuf, 1);		/* a near one is made far for it */
	CK(fbuf == a, 1);
	CK(a != fbuf + 1, 1);
	CK(a != 0, 1);
	CK(!a, 0);
	CK(a && b, 1);
	if (!a)
		CK(0, 1);
	a = 0;
	CK(a == 0, 1);
	CK(!a, 1);
	CK(a || b, 1);
	CK(a? 1: 2, 2);
	if (a)
		CK(0, 1);
	b = a? a: fbuf;
	CK(b == fbuf, 1);
	a = b? b + 1: b;
	CK(a - b, 1);
	fbuf[15] = 0;
	a = fbuf[15]? b: 0;
	LK(a, 0);
}

static long
through(p, q, n)
char __far *p;
int __far *q;
{
	return(*p + q[n]);
}

static
calls()
{
	int (__far *f)();
	int (__far *g)();
	struct fops *op;
	int n;

	fcp = pass((char __far *)fbuf, 4);
	CK(fcp - (char __far *)fbuf, 4);
	CK(pass(fcp, 3) - fcp, 3);
	fbuf[0] = 5;
	fints[2] = 300;
	LK(through((char __far *)fbuf, (int __far *)fints, 2), 305L);
	fcp = fbuf;
	fip = fints;
	LK(through(fcp, fip, 2), 305L);
	LK(fr_proto(fbuf, fints, 2), 5 + 2 * 300L);	/* made far by
							   the prototype */
	LK(fr_proto(fcp, fip + 1, 1), 5 + 2 * 300L);

	f = fr_func(0);
	g = fr_func(1);
	CK((*f)(3, 4), 7);
	CK(f(10, 4), 14);
	CK((*g)(3, 4), 12);
	CK(f == g, 0);
	CK(f != 0, 1);
	ffn = g;
	CK((*ffn)(5, 6), 30);
	CK(ffn == g, 1);
	ftab[0] = f;
	ftab[1] = g;
	n = 1;
	CK((*ftab[n])(2, 8), 16);
	CK((*ftab[n - 1])(2, 8), 10);
	CK((*fr_func(1))(7, 7), 49);
	fops.fn = f;
	op = &fops;
	CK((*op->fn)(1, 2), 3);
	CK(op->fn(20, 2), 22);
	CK((*f)((*g)(2, 3), (*f)(1, 1)), 8);
}

static
structs()
{
	struct s4 __far *p4;
	struct s6 __far *p6;
	struct s24 __far *p24;
	struct s24 __far *q24;
	struct s6 h;
	struct s24 t;
	int i;

	p6 = &fs6;
	fs6.a = 1;
	fs6.b = 2;
	fs6.c = 3;
	h = *p6;
	CK(h.c, 3);
	h.b = 20;
	*p6 = h;
	CK(fs6.b, 20);
	p24 = &fs24;
	for (i = 0; i < 12; i++)
		fs24.v[i] = i * i;
	t = *p24;
	CK(t.v[11], 121);
	t.v[5] = -5;
	*p24 = t;
	CK(fs24.v[5], -5);
	q24 = &ft24;
	*q24 = *p24;
	CK(ft24.v[11], 121);
	CK(ft24.v[5], -5);
	CK(q24->v[3], 9);
	CK(take6(1, *p6, 2), 1 + 2 + 60 + 12 + 10);
	t = mk24(0);
	*q24 = t;
	CK(take24(5, *q24, 1), 4 + 572);
	*p6 = mk6(7, 8, 9);
	CK(fs6.a, 7);
	CK(fs6.c, 9);
	*q24 = mk24(100);
	CK(ft24.v[0], 100);
	CK(ft24.v[11], 111);
	p4 = &fs4;
	*p4 = mk4(3, 4);
	CK(fs4.a, 3);
	CK(fs4.b, 4);
	fs4.b = 40;
	CK(take4(1, *p4, 2), 1 + 6 + 120 + 8);
}

/*
 * A far pointer that is written by way of what it points at, being
 * there itself, and so is to be fetched again: the object code improver
 * keeps what les gave ES and BX only while nothing can have changed it.
 */
struct fself {
	struct fself __far *p;
	int	v;
};
struct fself fself1, fself2;

static
selfarg(q)
struct fself __far *q;
{
	q = (struct fself __far *)&q;
	q->p = &fself2;
	q->v = 6;
}

static
reload()
{
	struct fself __far *q;

	fself1.p = &fself1;
	fself2.p = &fself2;
	fself1.v = 1;
	fself2.v = 2;
	fself1.p->p = &fself2;
	fself1.p->v = 5;
	CK(fself1.v, 1);
	CK(fself2.v, 5);
	selfarg(&fself1);
	CK(fself2.v, 6);
	CK(fself1.v, 1);
	q = (struct fself __far *)&q;
	q->p = &fself1;
	q->v = 7;
	CK(fself1.v, 7);
	CK(fself2.v, 6);
}

/*
 * Far variables, which are in a segment of their own and are reached
 * through it; and one of the other compiler's.
 */
struct fvs {
	int	a;
	long	b;
	char	c[5];
};
int __far fvi = 11;
long __far fvl;
char __far fvc[3][4] = { "ab", "cd" };
struct fvs __far fvst[3];
struct fvs __far fvone = { 1, 2L, "xyz" };
int __far *fvp = &fvi;
extern int __far fr_ftab[];
int fr_fpeek();

static
farvars()
{
	static int __far fvloc[4];
	register i;
	struct fvs s;
	int __far *ip;

	CK(fvi, 11);
	CK(*fvp, 11);
	CK(fvp == &fvi, 1);
	CK((long)fvp >> 16 == fr_seg(), 0);
	fvi = 12;
	fvi += 3;
	fvi++;
	CK(fvi, 16);
	CK(*fvp, 16);
	LK(fvl, 0L);
	fvl = 100000L;
	fvl *= 3;
	LK(fvl, 300000L);
	CK(fvc[1][1], 'd');
	CK(fvc[2][0], 0);
	CK(sizeof fvc, 12);
	CK(sizeof fvst, 36);
	CK(fr_peek((char __far *)fvc, 1), 'b');
	CK(fr_peek(fvc[1], 0), 'c');
	for (i = 0; i < 3; i++) {
		fvst[i].a = i * 10;
		fvst[i].b = i * 100000L;
		fvst[i].c[i] = 'p' + i;
	}
	CK(fvst[2].a, 20);
	LK(fvst[1].b, 100000L);
	CK(fvst[2].c[2], 'r');
	CK(fvst[i - 2].c[1], 'q');
	CK(&fvst[2] - &fvst[0], 2);
	CK(fvone.a, 1);
	LK(fvone.b, 2L);
	CK(fvone.c[2], 'z');
	s = fvone;
	CK(s.c[1], 'y');
	fvst[0] = s;
	CK(fvst[0].c[0], 'x');
	LK(fvst[0].b, 2L);
	fvloc[i - 1] = 7;
	CK(fvloc[2], 7);
	CK(fvloc[0], 0);
	ip = fvloc;
	ip[3] = fvloc[2] * 2;
	CK(fvloc[3], 14);
	CK(fr_ftab[2], 3);
	fr_ftab[1] = 20;
	CK(fr_fpeek(1), 20);
	CK(fr_ftab[i - 2] + fr_ftab[3], 24);
}

t_far()
{
	sizes();
	convert();
	deref();
	arith();
	members();
	compare();
	calls();
	structs();
	reload();
	farvars();
}
