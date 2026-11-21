/*
 * ve: changing the file, and undoing it.  Every change to the file is
 * a line put in, a line taken out or a line made different, and goes
 * through b_ins, b_del and b_put here, which see to what follows from
 * it: the screen, the marks, and what is needed to undo it.
 *
 * What is kept to undo a command is one run of lines: where it starts,
 * how many lines it is now, and the lines that were there before.  The
 * first change of a command starts the run, and a change outside it
 * makes it reach that far, taking in the lines between, which are the
 * same before and after.  To undo, the two are exchanged, a line at a
 * time so that it takes no memory to speak of, and then the record is
 * what will do the change again; so the records are a stack, those
 * that can be undone below those that can be done again.
 */
#include <string.h>
#include "ve.h"

int	changed;

struct undo {
	long	u_at;		/* the first line of the run */
	long	u_new;		/* how many lines it is in the file */
	struct text u_old;	/* the lines it was */
	struct pos u_cur;	/* where the cursor was */
};

static struct undo ustack[NUNDO];
static int	nu;		/* ustack[0] to [nu - 1] can be undone, */
static int	nr;		/*   and the nr after them done again */
static struct undo now;		/* the command at work */
static int	open;		/* it has changed something */
static int	lost;		/* and there was no memory to keep it */

void
u_clear(void)
{
	register int i;

	for (i = 0; i < nu + nr; i++)
		t_free(&ustack[i].u_old);
	nu = nr = 0;
	if (open)
		t_free(&now.u_old);
	open = 0;
}

void
u_begin(void)
{
	open = lost = 0;
	now.u_cur = cur;
}

void
u_end(void)
{
	register int i;

	if (lost && !msgattr)
		emsg("No room to keep what that changed: it cannot be undone");
	if (!open)
		return;
	open = 0;
	for (i = nu; i < nu + nr; i++)
		t_free(&ustack[i].u_old);
	nr = 0;
	if (nu == NUNDO) {
		t_free(&ustack[0].u_old);
		for (i = 1; i < NUNDO; i++)
			ustack[i - 1] = ustack[i];
		nu--;
	}
	ustack[nu++] = now;
}

/*
 * Line n is about to be changed or taken out, or with ins a line put
 * in before it: make the run reach it.
 */
static void
cover(long n, int ins)
{
	long cnt, last;

	if (lost)
		return;
	if (!open) {
		open = 1;
		t_init(&now.u_old);
		now.u_at = n;
		now.u_new = 0;
		if (ins)
			return;
	}
	if (n < now.u_at) {
		cnt = now.u_at - n;
		if (t_copy(&now.u_old, 1L, &buf, n, cnt) < 0)
			goto nomem;
		now.u_at = n;
		now.u_new += cnt;
	}
	last = ins ? n - 1 : n;
	if (last >= now.u_at + now.u_new) {
		cnt = last - (now.u_at + now.u_new) + 1;
		if (t_copy(&now.u_old, now.u_old.t_lines + 1, &buf,
		    now.u_at + now.u_new, cnt) < 0)
			goto nomem;
		now.u_new += cnt;
	}
	return;
nomem:
	/* the change is made all the same, and nothing can be undone */
	u_clear();
	lost = 1;
}

/*
 * Make the len bytes at s a new line n, before the one that is n now.
 */
int
b_ins(long n, char *s, int len)
{
	register int i;

	if (len > LINEMAX) {
		emsg("Line too long");
		return (-1);
	}
	cover(n, 1);
	if (t_ins(&buf, n, s, len) < 0) {
		emsg("%s", t_err());
		return (-1);
	}
	if (open)
		now.u_new++;
	for (i = 0; i < NMARK; i++)
		if (marks[i].p_line >= n)
			marks[i].p_line++;
	s_shift(n, 1L);
	modified = changed = 1;
	return (0);
}

/*
 * Take line n out.  The file always has a line: its only one is made
 * empty.
 */
void
b_del(long n)
{
	register int i;

	if (n < 1 || n > buf.t_lines)
		return;
	if (buf.t_lines == 1) {
		(void)b_put(1L, "", 0);
		return;
	}
	cover(n, 0);
	t_del(&buf, n);
	if (open)
		now.u_new--;
	for (i = 0; i < NMARK; i++)
		if (marks[i].p_line == n)
			marks[i].p_line = 0;
		else if (marks[i].p_line > n)
			marks[i].p_line--;
	s_shift(n, -1L);
	modified = changed = 1;
}

/*
 * Make line n the len bytes at s.
 */
int
b_put(long n, char *s, int len)
{
	if (len > LINEMAX) {
		emsg("Line too long");
		return (-1);
	}
	cover(n, 0);
	if (t_put(&buf, n, s, len) < 0) {
		emsg("%s", t_err());
		return (-1);
	}
	s_line(n);
	modified = changed = 1;
	return (0);
}

/*
 * Move count lines of from, from line first on, into to before its
 * line at, and say how many went: fewer than count if memory ran out.
 * Each is taken out of from as it goes, so no more memory is needed
 * than there was, give or take a block.
 */
static long
move(struct text *to, long at, struct text *from, long first, long count)
{
	long n;
	int len;

	for (n = 0; n < count; n++) {
		len = t_get(from, first, lbuf2);
		if (t_ins(to, at + n, lbuf2, len) < 0)
			break;
		t_del(from, first);
	}
	return (n);
}

/*
 * Exchange what a record has with what the file has.
 */
static int
swap(register struct undo *u)
{
	struct text t;
	long oldn = u->u_old.t_lines, i, n;
	struct pos c;
	int bad;

	t_init(&t);
	bad = move(&t, 1L, &buf, u->u_at, u->u_new) < u->u_new;
	if (!bad && (i = move(&buf, u->u_at, &u->u_old, 1L, oldn)) < oldn) {
		/* back with what did go */
		(void)move(&u->u_old, 1L, &buf, u->u_at, i);
		bad = 1;
	}
	if (bad) {
		(void)move(&buf, u->u_at, &t, 1L, t.t_lines);
		t_free(&t);
		emsg("%s", t_err());
		return (-1);
	}
	for (i = 0; i < NMARK; i++) {
		n = marks[i].p_line;
		if (n >= u->u_at + u->u_new)
			marks[i].p_line = n + oldn - u->u_new;
		else if (n >= u->u_at + oldn)
			marks[i].p_line = 0;
	}
	u->u_old = t;
	u->u_new = oldn;
	if (buf.t_lines == 0)
		(void)t_ins(&buf, 1L, "", 0);
	c = cur;
	cur = u->u_cur;
	u->u_cur = c;
	clampcur(0);
	s_from(u->u_at);
	modified = 1;
	return (0);
}

void
undo(void)
{
	if (open)
		emsg("Not while the file is being changed");
	else if (nu == 0)
		msg("Already at the oldest change");
	else if (swap(&ustack[nu - 1]) == 0) {
		nu--;
		nr++;
	}
}

void
redo_change(void)
{
	if (open)
		emsg("Not while the file is being changed");
	else if (nr == 0)
		msg("Already at the newest change");
	else if (swap(&ustack[nu]) == 0) {
		nu++;
		nr--;
	}
}
