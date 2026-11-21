/*
 * Not run: compiled with -nd=ND, where SS is not DS and the address of
 * what is in the frame is a pointer into SS, for what the compiler says
 * of one made an ordinary pointer, or passed where no prototype says
 * what it is to be (tools/netcheck.py counts those in the kernel's
 * network).  Each line that ends with NEAR is to draw "pointer into SS
 * made near", once, and no other line.
 */
struct s {
	int	a, b;
};

int	proto(int *);
int	fproto(int __far *);
int	ssproto(int __ss *);
int	vproto(void *);
int	ellipsis(int, ...);
int	noproto();
char	*gp;

char *
f(a)
	int a;
{
	int i, j;
	char buf[8];
	struct s st;
	int __ss *sp;
	char __far *fp;
	char *p;

	sp = &i;			/* stays in SS */
	ssproto(&i);
	ssproto(sp + 1);
	fproto(&i);			/* made far, with SS */
	fp = buf;
	ellipsis(1, fp);
	p = (char *)&i;			/* NEAR */
	gp = buf;			/* NEAR */
	proto(&j);			/* NEAR */
	vproto(&st);			/* NEAR */
	ellipsis(1, &i);		/* NEAR */
	noproto(a, sp);			/* NEAR */
	j = (int)&i;			/* NEAR */
	j = *(int *)&st;		/* NEAR */
	return (buf);			/* NEAR */
}
