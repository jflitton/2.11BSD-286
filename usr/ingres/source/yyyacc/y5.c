#if     !defined(lint) && defined(DOSCCS)
static char sccsid[] = "@(#)y5.c      1.1 (2.11BSD) 2026/5/18";
#endif

# include	<stdio.h>
/* fake portable I/O routines, for those
    sites so backward as to not have the
     port. library */

FILE	*cin, *cout;
FILE	*Fin, *Fout;

FILE *
copen(s, c)
	char *s; int c;
  	{
	FILE	*f;

	switch	(c)
		{
		case	'r':
			Fin = f = fopen( s, "r" );
			break;
		case	'a':
			f = fopen( s, "a" );
		case	'w':
     			f = fopen( s, "w" );
			break;
		default:
			error("bad copen");
		}
  	return(f);
 	}

cflush(x)
  FILE *x;
  { /* fake! sets file to x */

  fflush(Fout);
  Fout = x;
  }
