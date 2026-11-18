/*
 * What the console's terminal emulator, usr/src/sys/i286at/vt.c, needs
 * of the kernel's param.h, for checking it on the machine the system is
 * built on: the types, bcopy, and no far pointers.
 */
#include <sys/types.h>
#include <strings.h>

#define	__far

int	vtbell(), vtreply();
