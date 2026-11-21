/*
 * Where the symbol and string tables are in an executable of the 286's
 * format (sys/exec.h), for the macros N_SYMOFF and N_STROFF in a.out.h.
 * The PDP-11's ../../pdp/gen/nsym.c adds up its text, data and overlays;
 * here it is the segments, each made up to an even length, after the
 * header and the table of them.
 */

#include <a.out.h>

off_t
n_symoff(ep)
	register struct xexec *ep;
{
	register int i;
	off_t l;

	l = N_TXTOFF(ep->e);
	for (i = 0; i < ep->e.a_nseg && i < NXSEG; i++) {
		l += (ep->x[i].xs_filesz + 1) & ~1L;
		/* (an object file's relocation) */
		l += (long)ep->x[i].xs_nrel * sizeof(struct reloc);
	}
	return (l);
}

off_t
n_stroff(ep)
	register struct xexec *ep;
{
	return (n_symoff(ep) + ep->e.a_syms);
}
