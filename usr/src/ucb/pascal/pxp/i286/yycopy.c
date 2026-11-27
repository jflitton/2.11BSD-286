/*
 * OYcopy, of pxp, for the 286: the PDP-11's yycopy.s, which copies Y
 * to OY, in C, as pi has it (../pi/yycopy.c).
 */
#include "../0.h"
#include "../yy.h"

OYcopy()
{
	register int *r0 = &OY;
	register int *r1 = &Y;
	register int r2 = sizeof (struct yytok) / sizeof (int);

	do
		*r0++ = *r1++;
	while (--r2 > 0);
}
