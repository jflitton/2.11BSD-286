/* "@(#)i286x.c 1.0 (2.11BSD) 2026/11/26"; */

/*
 * The data, for the 80286, which the driver and the first pass both
 * write (pdp11x.c is the PDP-11's).  They are not assembly, as the
 * PDP-11's are, but the second pass's own operators, as the C
 * compiler's first pass gives it its data: the second pass reads them
 * after the code, as it reads that pass's strings, and so knows which
 * names the file defines.  Those it does not declare extern, which
 * the 286's assembler would take for a name defined twice.
 */

#include <stdio.h>
#include "defines"
#include "i286defs"
#include "dmrdefs"

/* an operator, as putdmr.c's p2op writes one */
asmop(fp, op)
FILE *fp;
int op;
{
putc(op, fp);
putc(0376, fp);
}

/* a word, its low byte first */
asmint(fp, n)
FILE *fp;
int n;
{
putc(n & 0377, fp);
putc((n >> 8) & 0377, fp);
}

asmstr(fp, s)
FILE *fp;
register char *s;
{
do
	putc(*s, fp);
		while(*s++);
}

/* a line of assembly, which goes out as it is */
asmline(fp, s)
FILE *fp;
char *s;
{
asmop(fp, P2ASSEM);
asmstr(fp, s);
}

/* a label, which the file defines */
asmlabel(fp, s)
FILE *fp;
char *s;
{
asmop(fp, P2NLABEL);
asmstr(fp, s);
}

prchars(fp, s)
FILE *fp;
register int *s;
{
char buff[8*SZSHORT];
register int i;

sprintf(buff, "\tdb\t");
for(i = 0; i<SZSHORT ; ++i)
	sprintf(buff+strlen(buff), "%d%s", *s++ & 0377,
		(i==SZSHORT-1 ? "" : ",") );
asmline(fp, buff);
}

/* k bytes of nothing: of zeros, but in the bss */
prskip(fp, k)
FILE *fp;
ftnint k;
{
for( ; k > 30000 ; k -= 30000)
	{
	asmop(fp, P2SSPACE);
	asmint(fp, 30000);
	}
if(k > 0)
	{
	asmop(fp, P2SSPACE);
	asmint(fp, (int) k);
	}
}
