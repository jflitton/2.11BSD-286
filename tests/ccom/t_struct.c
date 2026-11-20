/*
 * Structures as values: assigned, passed to functions and returned by
 * them, to and from functions the other compiler compiled.  One of a
 * word or of two is returned in registers, and any other where the
 * caller says, which it does in SI: a register variable's register.
 */
#include "structs.h"

#define	CK(got, want)	check(__LINE__, got, want)
#define	LK(got, want)	lcheck(__LINE__, (long)(got), (long)(want))

struct s2 mk2();
struct s4 mk4();
struct s6 mk6();
struct s10 mk10();
struct s24 mk24();
long take10(), call10();

struct s2 g2;
struct s4 g4;
struct s6 g6, h6;
struct s10 g10;
struct s24 g24;
struct s6 a6[4];
int ncalls;

/*
 * This compiler's own, which the other's functions call too.
 */
struct s2
n_mk2(a)
{
	struct s2 s;

	s.a = a;
	return s;
}

struct s4
n_mk4(a, b)
{
	struct s4 s;

	s.a = a;
	s.b = b;
	return s;
}

struct s6
n_mk6(a, b, c)
{
	struct s6 s;

	s.a = a;
	s.b = b;
	s.c = c;
	return s;
}

struct s10
n_mk10(tag, l, v)
long l;
{
	struct s10 s;

	s.tag = tag;
	s.l = l;
	s.v[0] = v;
	s.v[1] = -v;
	return s;
}

/* register variables, and so SI, in a function that is given it */
struct s24
n_mk24(a)
{
	struct s24 s;
	register int i;
	register int *p;

	p = s.v;
	for (i = 0; i < 12; i++)
		*p++ = a + i;
	scramble(i);
	return s;
}

n_take2(x, s, y)
struct s2 s;
{
	return x + 2 * s.a + 3 * y;
}

n_take4(x, s, y)
struct s4 s;
{
	return x + 2 * s.a + 3 * s.b + 4 * y;
}

n_take6(x, s, y)
struct s6 s;
{
	return x + 2 * s.a + 3 * s.b + 4 * s.c + 5 * y;
}

long
n_take10(x, s, y)
struct s10 s;
{
	return x + s.tag + s.l + 2 * s.v[0] + 5 * s.v[1] + y;
}

n_take24(x, s, y)
struct s24 s;
{
	register int i, n;

	n = x - y;
	for (i = 0; i < 12; i++)
		n += (i + 1) * s.v[i];
	return n;
}

/* what one returns is what another does */
struct s6
pass6(a)
{
	ncalls++;
	return mk6(a, a + 1, a + 2);
}

struct s24
pass24(a)
{
	register int r;

	r = a;
	ncalls++;
	return n_mk24(r + 1);
}

struct s4
pass4(a)
{
	return n_mk4(a, -a);
}

/* returns a global, a parameter, and through a pointer */
struct s6
retg()
{
	return g6;
}

struct s6
retarg(x, s, y)
struct s6 s;
{
	s.b += x + y;
	return s;
}

struct s24
retptr(p)
register struct s24 *p;
{
	return *p;
}

/* a structure that depends on the one it will replace */
struct s6
swap6()
{
	struct s6 s;

	s.a = g6.c;
	s.b = g6.b;
	s.c = g6.a;
	return s;
}

struct s6
rec6(n)
{
	struct s6 s;

	if (n == 0)
		return n_mk6(0, 0, 0);
	s = rec6(n - 1);
	s.a += n;
	s.b += 2 * n;
	s.c = n;
	return s;
}

static
values()
{
	struct s2 l2;
	struct s4 l4;
	struct s6 l6, *p6;
	struct s10 l10;
	struct s24 l24, *p24;
	int i;

	/* from the other compiler's */
	l2 = mk2(21);
	CK(l2.a, 21);
	l4 = mk4(41, 42);
	CK(l4.a, 41);
	CK(l4.b, 42);
	l6 = mk6(61, 62, 63);
	CK(l6.a, 61);
	CK(l6.b, 62);
	CK(l6.c, 63);
	l10 = mk10('t', 123456L, 9);
	CK(l10.tag, 't');
	LK(l10.l, 123456L);
	CK(l10.v[0], 9);
	CK(l10.v[1], -9);
	l24 = mk24(100);
	CK(l24.v[0], 100);
	CK(l24.v[11], 111);

	/* to a global, through a pointer, into an array */
	g2 = mk2(22);
	CK(g2.a, 22);
	g4 = mk4(43, 44);
	CK(g4.a + g4.b, 87);
	g6 = mk6(64, 65, 66);
	CK(g6.a, 64);
	CK(g6.b, 65);
	CK(g6.c, 66);
	g24 = mk24(200);
	CK(g24.v[5], 205);
	p6 = &l6;
	*p6 = mk6(1, 2, 3);
	CK(l6.a + l6.b * 10 + l6.c * 100, 321);
	i = 2;
	a6[i] = mk6(4, 5, 6);
	a6[i + 1] = n_mk6(7, 8, 9);
	CK(a6[2].b, 5);
	CK(a6[3].c, 9);
	CK(a6[1].c, 0);
	p24 = &g24;
	*p24 = n_mk24(300);
	CK(g24.v[0], 300);
	CK(g24.v[11], 311);

	/* from this compiler's */
	l2 = n_mk2(-2);
	CK(l2.a, -2);
	l4 = n_mk4(-4, -5);
	CK(l4.a, -4);
	CK(l4.b, -5);
	g6 = n_mk6(11, 12, 13);
	CK(g6.a, 11);
	CK(g6.c, 13);
	g10 = n_mk10('u', -70000L, 5);
	CK(g10.tag, 'u');
	LK(g10.l, -70000L);
	CK(g10.v[1], -5);

	/* one that is made of the one it replaces */
	g6 = swap6();
	CK(g6.a, 13);
	CK(g6.b, 12);
	CK(g6.c, 11);
	g6 = retg();
	CK(g6.a, 13);
	h6 = retg();
	CK(h6.c, 11);

	/* passed on */
	ncalls = 0;
	l6 = pass6(70);
	CK(l6.a, 70);
	CK(l6.c, 72);
	g24 = pass24(400);
	CK(g24.v[0], 401);
	CK(g24.v[11], 412);
	l4 = pass4(9);
	CK(l4.a, 9);
	CK(l4.b, -9);
	CK(ncalls, 2);
	l6 = retarg(1, l6, 2);
	CK(l6.b, 74);
	l24 = retptr(&g24);
	CK(l24.v[3], 404);
	l6 = rec6(5);
	CK(l6.a, 15);
	CK(l6.b, 30);
	CK(l6.c, 5);

	/* called for nothing */
	ncalls = 0;
	pass6(1);
	pass24(1);
	pass4(1);
	mk6(1, 2, 3);
	CK(ncalls, 2);
}

static
members()
{
	register int r;
	int i;

	/* a member of what a function returns */
	CK(mk2(5).a, 5);
	CK(mk4(5, 6).b, 6);
	CK(mk6(5, 6, 7).a, 5);
	CK(mk6(5, 6, 7).c, 7);
	CK(mk24(10).v[7], 17);
	CK(n_mk2(5).a, 5);
	CK(n_mk4(5, 6).a, 5);
	CK(n_mk6(5, 6, 7).b, 6);
	CK(n_mk24(10).v[11], 21);
	LK(mk10('a', 99999L, 1).l, 99999L);
	CK(n_mk10('a', 99999L, 1).tag, 'a');
	i = 3;
	CK(mk24(10).v[i], 13);
	CK(mk6(1, 2, 3).b + mk6(10, 20, 30).c, 32);
	CK(mk6(1, 2, 3).a + n_mk6(10, 20, 30).b * mk4(3, 4).b, 81);
	r = 1000;
	r += mk6(1, 2, 3).c;
	CK(r, 1003);
	if (mk6(0, 1, 0).b)
		r++;
	if (mk6(0, 1, 0).c)
		r += 100;
	CK(r, 1004);
	CK(sum5(mk6(1, 2, 3).a, n_mk6(1, 2, 3).b, mk4(3, 4).a, mk2(4).a, 5), 55);
}

static
args()
{
	struct s2 l2;
	struct s4 l4;
	struct s6 l6;
	register struct s6 *p6;
	struct s10 l10;
	struct s24 l24;
	register struct s24 *p24;

	l2.a = 3;
	l4.a = 3;
	l4.b = 4;
	l6.a = 3;
	l6.b = 4;
	l6.c = 5;
	l10.tag = 1;
	l10.l = 1000000L;
	l10.v[0] = 7;
	l10.v[1] = 8;
	l24 = mk24(1);
	g6 = l6;
	g6.c = 50;
	p6 = &l6;
	p24 = &l24;

	/* to the other compiler's */
	CK(take2(1, l2, 2), 13);
	CK(take4(1, l4, 2), 27);
	CK(take6(1, l6, 2), 49);
	CK(take6(1, g6, 2), 229);
	CK(take6(1, *p6, 2), 49);
	LK(take10(1, l10, 2), 1000058L);
	CK(take24(1, l24, 2), 649);
	CK(take24(1, *p24, 2), 649);
	a6[1] = l6;
	a6[1].a = -3;
	CK(take6(0, a6[1], 0), 26);

	/* to this compiler's */
	CK(n_take2(1, l2, 2), 13);
	CK(n_take4(1, l4, 2), 27);
	CK(n_take6(1, l6, 2), 49);
	CK(n_take6(1, *p6, 2), 49);
	LK(n_take10(1, l10, 2), 1000058L);
	CK(n_take24(1, l24, 2), 649);
	CK(n_take24(1, *p24, 2), 649);

	/* what one function returns, given to another */
	CK(take2(1, mk2(3), 2), 13);
	CK(take4(1, mk4(3, 4), 2), 27);
	CK(take6(1, mk6(3, 4, 5), 2), 49);
	CK(take24(1, mk24(1), 2), 649);
	CK(n_take2(1, n_mk2(3), 2), 13);
	CK(n_take4(1, n_mk4(3, 4), 2), 27);
	CK(n_take6(1, n_mk6(3, 4, 5), 2), 49);
	CK(n_take24(1, n_mk24(1), 2), 649);
	CK(n_take6(1, mk6(3, 4, 5), 2), 49);
	CK(take24(1, n_mk24(1), 2), 649);
	LK(take10(1, n_mk10(1, 1000000L, 7), 2), 999983L);
	CK(take6(take6(0, mk6(1, 0, 0), 0), n_mk6(3, 4, 5), take2(0, mk2(1), 0)),
	    50);

	/* the register variables are as they were */
	CK(p6->c, 5);
	CK(p24->v[11], 12);
	CK(p6 == &l6, 1);
	CK(p24 == &l24, 1);
}

struct fns {
	int	pad;
	struct s6 (*mk)();
	int	(*take)();
};

static
pointers()
{
	struct fns f;
	register struct fns *fp;
	struct s6 (*mk)();
	struct s24 (*mk24p)();
	struct s6 l6;
	struct s24 l24;

	/* the pointer to the function is in SI */
	fp = &f;
	fp->mk = mk6;
	fp->take = n_take6;
	l6 = (*fp->mk)(3, 4, 5);
	CK(l6.b, 4);
	CK((*fp->take)(1, l6, 2), 49);
	CK((*fp->take)(1, (*fp->mk)(3, 4, 5), 2), 49);
	g6 = (*fp->mk)(6, 7, 8);
	CK(g6.c, 8);
	CK((*fp->mk)(6, 7, 8).a, 6);
	mk = n_mk6;
	l6 = (*mk)(9, 8, 7);
	CK(l6.c, 7);
	mk24p = n_mk24;
	l24 = (*mk24p)(50);
	CK(l24.v[6], 56);
	mk24p = mk24;
	g24 = (*mk24p)(60);
	CK(g24.v[6], 66);
	CK(fp == &f, 1);

	/* the other compiler calls this one's */
	CK(call2(n_mk2, n_take2), 28);
	CK(call4(n_mk4, n_take4), 55);
	CK(call6(n_mk6, n_take6), 94);
	LK(call10(n_mk10, n_take10), 100003L);
	CK(call24(n_mk24, n_take24), 977);
	CK(call6(pass6, n_take6), 94);
	CK(call24(pass24, n_take24), 1056);
}

t_struct()
{
	values();
	members();
	args();
	pointers();
}
