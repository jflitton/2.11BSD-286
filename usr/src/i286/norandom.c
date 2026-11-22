/*
 * A random() that is rand(), and a srandom() that does nothing: what a
 * program that makes a file in a random order is linked with where the
 * system is made, so that the file is the same each time (strfile -r,
 * for fortunes.dat; tools/strfile.py does the same where the system is
 * built).
 */
srandom(seed)
unsigned seed;
{
}

long
random()
{
	return((long)rand());
}
