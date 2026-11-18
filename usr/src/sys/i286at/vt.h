/*
 * A terminal on a screen: what vt.c keeps for each.  The keyboard half
 * of the terminal (kbd.c) reads the modes in v_flags that change what
 * keys send.
 */
#define	VT_COLS		80
#define	VT_ROWS		25
#define	VT_NPAR		16	/* parameters a control sequence may have */

struct vt {
	u_short	__far *v_cells;	/* the screen: VT_ROWS rows of VT_COLS
				   cells, each a character in the low byte
				   and its attribute in the high */
	u_char	v_x, v_y;	/* the cursor */
	u_char	v_top, v_bot;	/* the scrolling region: first and last row */
	u_short	v_flags;
	u_char	v_state;	/* where in an escape sequence */
	u_char	v_inter;	/* its intermediate character, 0 for none,
				   0377 for more than one */
	u_char	v_priv;		/* its private marker (?, <, = or >) */
	u_char	v_npar;		/* which parameter is being read */
	u_char	v_any;		/* something of the parameters has come */
	u_char	v_bad;		/* the sequence is malformed: ignore it */
	u_short	v_par[VT_NPAR];
	u_char	v_ren;		/* graphic rendition: bold, underline ... */
	u_char	v_fg, v_bg;	/*   and the colours, 0 to 7 */
	u_char	v_attr;		/* the attribute byte characters get */
	u_char	v_eattr;	/* and the one erasing leaves */
	u_char	v_g[2];		/* the character sets G0 and G1, by their
				   final characters: B ASCII, A British,
				   0 DEC special graphics */
	u_char	v_gl;		/* which of them is in use */
	u_char	v_mono;		/* the display is monochrome */
	struct vtsave {		/* what DECSC saves */
		u_char	s_x, s_y;
		u_char	s_ren, s_fg, s_bg;
		u_char	s_g[2], s_gl;
		u_short	s_flags;	/* VT_OM and VT_WRAP */
	} v_save;
	u_char	v_tabs[VT_COLS / 8];	/* a bit for each column */
};

/* v_flags */
#define	VT_WRAP		0x0001	/* a character went in the last column:
				   the next goes on the next line */
#define	VT_AWM		0x0002	/* autowrap (DECAWM) */
#define	VT_OM		0x0004	/* origin mode (DECOM) */
#define	VT_IRM		0x0008	/* insert mode (IRM) */
#define	VT_LNM		0x0010	/* newline mode (LNM): a line feed also
				   returns, and Return sends CR LF */
#define	VT_CKM		0x0020	/* cursor keys send application sequences
				   (DECCKM) */
#define	VT_KPAM		0x0040	/* the keypad does (DECKPAM) */
#define	VT_SCNM		0x0080	/* reverse screen (DECSCNM) */
#define	VT_TCEM		0x0100	/* the cursor is shown (DECTCEM) */
#define	VT_ARM		0x0200	/* keys repeat (DECARM) */
#define	VT_VT52		0x0400	/* VT52 mode: DECANM reset */

#ifdef KERNEL
int	vtinit(), vtreset(), vtwrite(), vtkputc();
/* What vt.c calls, which the screen's driver provides. */
int	vtbell(), vtreply();
#endif
