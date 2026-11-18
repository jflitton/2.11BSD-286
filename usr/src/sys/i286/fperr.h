/*
 * What the 287 reported at its last error: its status word, and the
 * address of the instruction.
 */
struct fperr {
	short	f_fec;
	caddr_t	f_fea;
};
