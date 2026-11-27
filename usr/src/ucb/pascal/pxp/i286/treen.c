/*
 * tree1 to tree5, of pxp, for the 286: the PDP-11's treen.s, which
 * called tree with the count of the arguments put before them, in C,
 * as pi has them (../pi/treen.c), giving back what tree gives.
 */
#include "../0.h"

int *
tree1(arg1)
	int arg1;
{

	return (tree(1, arg1));
}

int *
tree2(arg1, arg2)
	int arg1, arg2;
{

	return (tree(2, arg1, arg2));
}

int *
tree3(arg1, arg2, arg3)
	int arg1, arg2, arg3;
{

	return (tree(3, arg1, arg2, arg3));
}

int *
tree4(arg1, arg2, arg3, arg4)
	int arg1, arg2, arg3, arg4;
{

	return (tree(4, arg1, arg2, arg3, arg4));
}

int *
tree5(arg1, arg2, arg3, arg4, arg5)
	int arg1, arg2, arg3, arg4, arg5;
{

	return (tree(5, arg1, arg2, arg3, arg4, arg5));
}
