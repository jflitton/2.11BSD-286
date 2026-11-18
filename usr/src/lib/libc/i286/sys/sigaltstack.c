/*
 * sigaltstack and sigstack.  To the kernel the stack's address in each
 * structure is far, whatever the program's model (docs/processes.md).
 */
#include <signal.h>

struct ksigaltstack {
	char __far *kss_base;
	int	kss_size;
	int	kss_flags;
};

struct ksigstack {
	char __far *kss_sp;
	int	kss_onstack;
};

extern int _sigaltstack(struct ksigaltstack *, struct ksigaltstack *);
extern int _sigstack(struct ksigstack *, struct ksigstack *);

int
sigaltstack(const struct sigaltstack *ss, struct sigaltstack *oss)
{
	struct ksigaltstack k, ok;

	if (ss) {
		k.kss_base = ss->ss_base;
		k.kss_size = ss->ss_size;
		k.kss_flags = ss->ss_flags;
	}
	if (_sigaltstack(ss ? &k : 0, oss ? &ok : 0) < 0)
		return (-1);
	if (oss) {
		oss->ss_base = (char *)ok.kss_base;
		oss->ss_size = ok.kss_size;
		oss->ss_flags = ok.kss_flags;
	}
	return (0);
}

int
sigstack(const struct sigstack *ss, struct sigstack *oss)
{
	struct ksigstack k, ok;

	if (ss) {
		k.kss_sp = ss->ss_sp;
		k.kss_onstack = ss->ss_onstack;
	}
	if (_sigstack(ss ? &k : 0, oss ? &ok : 0) < 0)
		return (-1);
	if (oss) {
		oss->ss_sp = (char *)ok.kss_sp;
		oss->ss_onstack = ok.kss_onstack;
	}
	return (0);
}
