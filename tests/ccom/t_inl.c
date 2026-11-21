/*
 * In-line functions (__inline): a body of assembly put where the
 * function is called, its first argument in AX, or DX:AX if a long,
 * and the others on the stack; labels of each place's own, a C name in
 * the body, and what the object code improver knows of a variable that
 * the body changes.  The cross compiler, for REF=1, has the same in C.
 */
#define	CK(got, want)	check(__LINE__, got, want)
#define	LK(got, want)	lcheck(__LINE__, (long)(got), (long)(want))

int	inlv;
int	inlw = 0x1234;
long	inll = 0x12345678L;

#ifdef __WATCOMC__
static unsigned swap16(unsigned x) { return (x << 8 | x >> 8); }
static unsigned long swap32(unsigned long x)
{
	return ((unsigned long)swap16((unsigned)x) << 16 | swap16((unsigned)(x >> 16)));
}
static void bump(void) { inlv++; }
static int add2(int a, int b) { return (a + b); }
static long lsub(long a, int b) { return (a - b); }
static int clamp(int x) { return (x > 100 ? 100 : x); }
static int nz(int x) { return (x != 0); }
static int seen(void) { return (inlv); }
#else
__inline unsigned swap16(unsigned x) { asm("xchg al,ah"); }
__inline unsigned long swap32(unsigned long x)
{
	asm("xchg al,ah");
	asm("	xchg	dl,dh");
	asm(" xchg ax, dx");
}
__inline void bump(void) { asm("inc word [ss:_inlv]"); }
__inline int add2(int a, int b)
{
	asm("mov bx,sp");
	asm("add ax,[ss:bx]");
}
__inline long lsub(long a, int b)
{
	asm("mov bx,sp");
	asm("sub ax,[ss:bx]");
	asm("sbb dx,0");
}
__inline int clamp(int x)
{
	asm("cmp ax,100");
	asm("jle %%ok");
	asm("mov ax,100");
	asm("%%ok:");
}
__inline int nz(int x)
{
	asm("test ax,ax");
	asm("jz %%zero");
	asm("mov ax,1");
	asm("%%zero:");
}
__inline int seen(void) { asm("mov ax,[ss:_inlv]"); }
#endif

static
ident(x)
{
	return (x);
}

t_inl()
{
	int a, i;
	register int r;
	long l;

	CK(swap16(0x1234), 0x3412);
	CK(swap16(inlw), 0x3412);
	CK(swap16(swap16(inlw)), 0x1234);
	r = 0x00ff;
	CK(swap16(r), 0xff00);
	CK(swap16(r + 1), 0x0001);
	LK(swap32(0x12345678L), 0x78563412L);
	LK(swap32(inll), 0x78563412L);
	l = swap32(inll);
	LK(swap32(l), inll);
	if (swap16(inlw) == 0x3412)
		a = 1;
	else
		a = 0;
	CK(a, 1);
	CK(ident(swap16(0x0102)) + 1, 0x0202);

	/* two arguments, and a long with another */
	CK(add2(3, 4), 7);
	CK(add2(inlw, ident(1)), 0x1235);
	LK(lsub(inll, 8), 0x12345670L);
	LK(lsub(0x10000L, 1), 0xffffL);

	/* the same labels twice in a function, and in a loop */
	CK(clamp(5), 5);
	CK(clamp(500), 100);
	a = 0;
	for (i = 98; i < 103; i++)
		a += clamp(i);
	CK(a, 98 + 99 + 100 + 100 + 100);
	CK(nz(0), 0);
	CK(nz(-3), 1);
	CK(nz(clamp(0)) + nz(clamp(7)), 1);

	/* a variable that the body changes is fetched again after */
	inlv = 5;
	a = inlv;
	bump();
	CK(inlv, 6);
	CK(a, 5);
	r = inlv;
	bump();
	bump();
	CK(inlv, 8);
	CK(r, 6);
	CK(seen(), 8);
	inlv = 0;
	for (i = 0; i < 10; i++)
		bump();
	CK(seen(), 10);
}
