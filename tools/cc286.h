/*
 * The system's C calling convention, given to every compilation by
 * tools/cc286 (-fi).  See docs/abi.md.
 *
 * Arguments on the stack, pushed right to left; the caller removes them.
 * External names get a leading underscore.  AX, BX, CX, DX and ES may be
 * destroyed by a call; SI, DI, BP and DS are preserved.  Because this says
 * nothing about return values, the compiler's defaults apply: AX, DX:AX,
 * the 287 stack for float and double, and a caller-supplied buffer whose
 * address is in SI for structures.
 */
#pragma aux default "_*" parm caller [] modify [ax bx cx dx es];
