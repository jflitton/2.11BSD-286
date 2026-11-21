/*
 * Code whose DS is not SS, as the kernel's network's is (tools/ncc -nd,
 * docs/toolchain.md): compiled with -nd=ND, its data is in a group of
 * its own, ND_GROUP, which nd.s makes DS before it is entered, and its
 * frame is reached through SS.  What is in the frame and what is in the
 * data are told apart where a structure is copied or passed, where the
 * address of what is in the frame is made far, and where something in
 * the frame is reached through its address cast to an ordinary pointer,
 * as the network's in_makeaddr does it.  A pointer to what is in SS is
 * __ss, and one into a segment of its own __based.  The checks go by
 * way of nd.s too, which gives support.c its own DS back.  In the large
 * model, on the system, this is compiled as any other test is, and its
 * DS is SS.
 */
#include "structs.h"

#ifdef ND
#define	t_nd	nd_t_nd		/* nd.s's t_nd sets DS and calls this */
#define	check	ndcheck
#define	lcheck	ndlcheck
#define	SS	__ss
#define	BASED	__based(NDSEG)
#else
#define	SS
#define	BASED
#endif

#define	CK(got, want)	check(__LINE__, got, want)
#define	LK(got, want)	lcheck(__LINE__, (long)(got), (long)(want))

struct pt {
	int	x, y;
	long	z;
};

static struct pt gpt = { 1, 2, 300000L };
static struct pt gpt2;
static int gint = 5;
static int gtab[4];
static char gstr[] = "net";
static int BASED btab[8];
static int BASED *bptr;

/*
 * A value made in the frame and given back through a cast of its
 * address.
 */
static long
inc(a)
long a;
{
	long l;

	l = a + 1;
	return (*(long *)&l);
}

static struct s4
mk(a, b)
{
	struct s4 s;

	s.a = a;
	s.b = b;
	return (s);
}

static
ssum(p, n)
int SS *p;
{
	int s;

	for (s = 0; n-- > 0; )
		s += *p++;
	return (s);
}

static
dsum(p, n)
int *p;
{
	int s;

	for (s = 0; n-- > 0; )
		s += *p++;
	return (s);
}

static
fsum(p, n)
int __far *p;
{
	int s;

	for (s = 0; n-- > 0; )
		s += *p++;
	return (s);
}

static long
takept(p)
struct pt p;
{
	return (p.x + p.y + p.z);
}

static
setpt(p, v)
struct pt SS *p;
{
	p->x = v;
	p->y = v + 1;
	p->z = v * 1000L;
}

t_nd()
{
	struct pt lpt, lpt2;
	int loc[4];
	register i;
	int SS *ip;
	int __far *fp;

	lpt = gpt;			/* from the data to the frame */
	CK(lpt.y, 2);
	LK(lpt.z, 300000L);
	lpt.x = 10;
	gpt2 = lpt;			/* and back */
	CK(gpt2.x, 10);
	LK(gpt2.z, 300000L);
	lpt2 = lpt;			/* within the frame */
	CK(lpt2.y, 2);
	LK(takept(lpt), 300012L);
	LK(takept(gpt), 300003L);
	setpt(&lpt, 4);
	CK(lpt.y, 5);
	LK(lpt.z, 4000L);
	for (i = 0; i < 4; i++) {
		loc[i] = i + 1;
		gtab[i] = 10 * i;
	}
	CK(ssum(loc, 4), 10);
	CK(dsum(gtab, 4), 60);
	ip = loc;
	CK(ip[2], 3);
	ip = &loc[1];
	*ip = 7;
	CK(loc[1], 7);
	fp = loc;			/* made far with SS */
	CK(fsum(fp, 4), 15);
	fp = gtab;			/* and with DS */
	CK(fsum(fp, 4), 60);
	fp = &lpt.x;
	CK(*fp, 4);
	LK(inc(41L), 42L);
	CK(mk(3, 4).b, 4);
	CK(gstr[1], 'e');
	CK(gint, 5);
	for (i = 0; i < 8; i++)
		btab[i] = i * i;
	bptr = btab;
	CK(bptr[3], 9);
	CK(*(bptr + 7), 49);
	bptr = &btab[2];
	CK(*bptr, 4);
	CK(bptr - btab, 2);
	CK(sizeof btab, 16);
}
