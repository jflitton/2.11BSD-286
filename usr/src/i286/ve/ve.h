/*
 * ve: a screen editor in the manner of vim.  This port's own
 * (docs/userland.md).
 *
 * The text is not in the program's data segment: it is in blocks of
 * BLKSIZE bytes, each holding some lines one after another, of which
 * some are in segments asked of the system (segalloc(2)) and the rest
 * in a file, so a file may be far larger than the 64K a program's own
 * data may be, and than the memory there is.  Everything else here is
 * near, and a line is copied to a buffer of the program's own to be
 * looked at or changed (text.c).
 */
#include <sys/types.h>

#define	LINEMAX	2000		/* bytes in a line; a longer one is split */
#define	COLMAX	400		/* columns of the widest screen */
#define	ROWMAX	200		/* and rows of the tallest */
#define	NUNDO	40		/* changes that can be undone */
#define	NREG	28		/* registers: unnamed, a to z, and small */
#define	REDOMAX	512		/* keys of a change that . repeats */

/*
 * Lines of text, kept in blocks: the file, each register, and what
 * each change that can be undone took out.
 */
struct text {
	short	t_head, t_tail;	/* its first and last block, or -1 */
	long	t_lines;	/* how many lines it has */
	short	t_blk;		/* a block lately used, or -1, */
	long	t_first;	/*   and the number of its first line; */
	long	t_line;		/*   a line in it, */
	unsigned t_off;		/*   and where in it that starts */
};

/* text.c */
void	t_init(struct text *);
void	t_free(struct text *);
int	t_get(struct text *, long, char *);
int	t_ins(struct text *, long, char *, int);
void	t_del(struct text *, long);
int	t_put(struct text *, long, char *, int);
int	t_copy(struct text *, long, struct text *, long, long);
long	t_blocks(long *);
int	t_cache(int);
char	*t_err(void);
int	t_len(struct text *, long);
#define	ALL	0x7fffffffL	/* more lines than any file has */
void	t_setmark(struct text *, long, int);
int	t_ismark(struct text *, long);

/* fmem.s */
void	fcopy(char __far *, char __far *, unsigned);
void	fmove(char __far *, char __far *, unsigned);
unsigned fscan(char __far *, int, unsigned);
int	fin(int, char __far *, unsigned);
int	fout(int, char __far *, unsigned);
/* the same for what is in the program's own data */
#define	nmove(from, to, n) \
	fmove((char __far *)(from), (char __far *)(to), (unsigned)(n))
#define	nscan(p, c, n)	fscan((char __far *)(p), c, (unsigned)(n))

/*
 * A position in the file: a line, from 1, and a byte in it, from 0.
 */
struct pos {
	long	p_line;
	int	p_col;
};

/* A register: lines, and whether whole lines were yanked. */
struct reg {
	struct text r_text;
	int	r_lines;
};

/* Keys that are not characters. */
#define	K_UP	0x101
#define	K_DOWN	0x102
#define	K_LEFT	0x103
#define	K_RIGHT	0x104
#define	K_HOME	0x105
#define	K_END	0x106
#define	K_PGUP	0x107
#define	K_PGDN	0x108
#define	K_INS	0x109
#define	K_DEL	0x10A
#define	K_NONE	0x1FF		/* nothing came: the window changed */
#define	ESC	033
#define	CTL(c)	((c) & 037)

/* Modes. */
#define	M_NORMAL 0
#define	M_INSERT 1
#define	M_REPLACE 2
#define	M_VISUAL 3		/* characters */
#define	M_VLINE	4		/* whole lines */

/* main.c */
extern struct text buf;		/* the file */
extern struct pos cur;		/* where the cursor is */
extern int	wantcol;	/* the column it keeps to going up and down */
extern int	mode;
extern struct pos vstart;	/* where the visual area began */
extern char	fname[];	/* the file's name */
extern int	modified;
extern int	readonly;
extern char	lbuf[];		/* a line, for whoever is at work */
extern char	lbuf2[];
extern struct pos marks[];
extern struct reg regs[];
extern int	o_number, o_ic, o_ai, o_ts, o_sw, o_et, o_ws, o_list;
extern char	**files;	/* the files named when it was started */
extern int	nfiles, filen;
void	msg(char *, ...);
void	emsg(char *, ...);
void	quit(int);
void	fatal(char *);
int	waitload(void);
void	clampcur(int);
extern int	active;		/* the screen is the editor's */
extern int	gone;		/* the terminal hung up, or the end was asked */
extern char	altname[];	/* the file edited before this one */
extern long	altline;
#define	NMARK	27		/* a to z, and where the cursor last jumped from */
#define	BIGCOL	30000		/* the column of a cursor kept at line ends */

/* term.c */
extern int	rows, cols;
extern int	resized;
int	t_pending(void);
void	t_open(void);
void	t_close(void);
void	t_raw(int);
void	t_ti(void);
void	t_cooked(void);
int	getkey(void);
void	ungetkey(int);
void	stuff(short *, int);
int	stuffed(void);
void	t_flush(void);
void	t_move(int, int);
void	t_putc(int);
void	t_puts(char *);
void	t_attr(int);
void	t_clreol(void);
void	t_clear(void);
void	t_bell(void);
int	t_scroll(int, int, int);
void	t_size(void);
extern short	redo[];
extern int	nredo;
void	rec_start(void);
void	rec_end(int);
void	rec_key(int);
extern int	intr;		/* the interrupt key was struck */
int	t_cansusp(void);

/* screen.c */
extern long	topline;
extern int	leftcol;
extern char	msgbuf[];
extern int	msgattr;
void	s_init(void);
void	s_all(void);
void	s_line(long);
void	s_from(long);
void	s_shift(long, long);
void	s_refresh(void);
void	s_scrollto(void);
int	s_textrows(void);
int	dispcol(char *, int);
int	colofdisp(char *, int, int);
int	s_prompt(int, char *, int);
void	s_wait(void);

/* edit.c: every change to the file goes through these */
int	b_ins(long, char *, int);
void	b_del(long);
int	b_put(long, char *, int);
void	u_begin(void);
void	u_end(void);
void	undo(void);
void	redo_change(void);
void	u_clear(void);
extern int	changed;	/* the command at work changed the file */

/* normal.c */
void	normal(int);
int	yank(int, struct pos *, struct pos *, int);
int	dodelete(int, struct pos *, struct pos *, int);
void	put(int, int, long);
void	shift(long, long, int);
int	join(long, long);
int	key(void);
int	firstnb(long);
int	regof(int);
void	setpcmark(void);

/* insert.c */
void	insert(int, long);
int	openline(int);
int	mkindent(char *, int);
int	indentof(char *, int *);
extern int	ai_pending;

/* ex.c */
void	ex(char *);
long	readfile(char *, long);
int	writefile(char *, long, long, int);
int	edit(char *);
extern int	loading;	/* the file is not all read yet */
int	loaduntil(long);
void	exinit(void);
int	shell(char *);

/* search.c */
int	re_set(char *);
int	re_match(char *, int);
int	search(int, struct pos *);
extern char	*re_start, *re_end;
extern char	*re_gs[], *re_ge[];
extern int	re_have;
