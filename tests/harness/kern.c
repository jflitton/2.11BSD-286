/*
 * For a test that links the kernel's machine-dependent assembly without
 * the kernel: what that assembly expects the rest of the kernel to have.
 * The test supplies main, trap and syscall itself.
 */
#define	UBYTES	(7 * 512)	/* as mch_defs.inc has it */

char	uslot[UBYTES];		/* a user structure and kernel stack */
char	*uptr;
int	runrun;
int	cpl;
unsigned ipending, spending;
int	noproc;			/* idle() sets it */

/*
 * An interrupt request: the test's trap() sees to those as well.
 */
intr(tf)
	char *tf;
{
	trap(tf);
}

ast()
{
}

spllower()
{
}
