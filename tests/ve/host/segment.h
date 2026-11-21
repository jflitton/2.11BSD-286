/*
 * <segment.h> for ve built where the tests are run (tests/ve/run.sh):
 * a segment is 64K from malloc, which is all ve asks for at a time.
 */
void	*segalloc(long size);
int	segsize(void *seg, long size);
int	segfree(void *seg);
