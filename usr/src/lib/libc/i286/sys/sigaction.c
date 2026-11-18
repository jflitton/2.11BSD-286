/*
 * sigaction.  The kernel's call takes one argument more, the trampoline
 * through which it is to start a handler (sigtramp.s), and has the
 * handler as a far address, whatever the program's model.  In the medium
 * model a pointer to a function is one; in the small model it is an
 * offset in CS.
 */
#include <signal.h>

struct ksigaction {
	unsigned ks_off;	/* the handler: offset */
	unsigned ks_seg;	/*   and code segment, 0 with SIG_DFL or SIG_IGN */
	sigset_t ks_mask;
	int	ks_flags;
};

#if defined(__MEDIUM__) || defined(__LARGE__)
union handler {
	sig_t	h;
	struct {
		unsigned off, seg;
	} w;
};
#else
unsigned _getcs(void);
#pragma aux _getcs = "mov ax, cs" value [ax];
#endif

extern void _sigtramp();
extern int _sigaction(void (*)(), int, struct ksigaction *, struct ksigaction *);

int
sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	struct ksigaction ka, ko;
#if defined(__MEDIUM__) || defined(__LARGE__)
	union handler u;
#endif

	if (act) {
#if defined(__MEDIUM__) || defined(__LARGE__)
		u.h = act->sa_handler;
		ka.ks_off = u.w.off;
		ka.ks_seg = u.w.seg;
#else
		ka.ks_off = (unsigned)act->sa_handler;
		ka.ks_seg = ka.ks_off > 1 ? _getcs() : 0;
#endif
		ka.ks_mask = act->sa_mask;
		ka.ks_flags = act->sa_flags;
	}
	if (_sigaction(_sigtramp, sig, act ? &ka : 0, oact ? &ko : 0) < 0)
		return (-1);
	if (oact) {
#if defined(__MEDIUM__) || defined(__LARGE__)
		u.w.off = ko.ks_off;
		u.w.seg = ko.ks_seg;
		oact->sa_handler = u.h;
#else
		oact->sa_handler = (sig_t)ko.ks_off;
#endif
		oact->sa_mask = ko.ks_mask;
		oact->sa_flags = ko.ks_flags;
	}
	return (0);
}
