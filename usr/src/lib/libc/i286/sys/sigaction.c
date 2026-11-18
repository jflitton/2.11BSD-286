/*
 * sigaction.  The kernel's call takes one argument more, the trampoline
 * through which it is to start a handler (sigtramp.s), and has the
 * handler as a far address, whatever the program's model.
 */
#include <signal.h>

struct ksigaction {
	unsigned ks_off;	/* the handler: offset */
	unsigned ks_seg;	/*   and code segment, 0 with SIG_DFL or SIG_IGN */
	sigset_t ks_mask;
	int	ks_flags;
};

extern void _sigtramp();
extern int _sigaction(void (*)(), int, struct ksigaction *, struct ksigaction *);

unsigned _getcs(void);
#pragma aux _getcs = "mov ax, cs" value [ax];

int
sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	struct ksigaction ka, ko;

	if (act) {
		ka.ks_off = (unsigned)act->sa_handler;
		ka.ks_seg = ka.ks_off > 1 ? _getcs() : 0;
		ka.ks_mask = act->sa_mask;
		ka.ks_flags = act->sa_flags;
	}
	if (_sigaction(_sigtramp, sig, act ? &ka : 0, oact ? &ko : 0) < 0)
		return (-1);
	if (oact) {
		oact->sa_handler = (sig_t)ko.ks_off;
		oact->sa_mask = ko.ks_mask;
		oact->sa_flags = ko.ks_flags;
	}
	return (0);
}
