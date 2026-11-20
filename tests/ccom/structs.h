/*
 * The structures that t_struct.c and support.c pass each other: of one
 * word and of two, which go in registers; and larger ones, which do
 * not.
 */
struct s2 {
	int	a;
};
struct s4 {
	int	a, b;
};
struct s6 {
	int	a, b, c;
};
struct s10 {
	char	tag;
	long	l;
	int	v[2];
};
struct s24 {
	int	v[12];
};
