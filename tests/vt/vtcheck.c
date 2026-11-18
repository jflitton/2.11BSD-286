/*
 * Check the console's terminal emulator, usr/src/sys/i286at/vt.c, on the
 * machine the system is built on.  Each case writes to a terminal and
 * looks at its screen, its cursor and what it answered.  What a case
 * expects is what DEC's VT102 does, or, where vt.c goes beyond the
 * VT102, what Linux's console and xterm do.  A last case writes random
 * bytes, to show that nothing a program writes takes the emulator off
 * its screen: the screen has guard cells before and after it, which no
 * case may change.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "param.h"
#include "vt.h"

int	vtinit(), vtreset(), vtwrite(), vtkputc();

#define	GUARD	(2 * VT_COLS)
#define	CANARY	0xDEAD
static u_short	memory[GUARD + VT_ROWS * VT_COLS + GUARD];
#define	screen	(memory + GUARD)
static struct vt v;
static char	replies[256];
static int	bells, pass, fail;
static const char *name;	/* the case being checked */

int
vtbell(struct vt *vp)
{
	(void)vp;
	bells++;
	return (0);
}

int
vtreply(struct vt *vp, char *s)
{
	(void)vp;
	strncat(replies, s, sizeof (replies) - strlen(replies) - 1);
	return (0);
}

static void	guards(void);

static void
start(const char *n, int mono)
{
	int i;

	guards();
	name = n;
	for (i = 0; i < GUARD + VT_ROWS * VT_COLS + GUARD; i++)
		memory[i] = CANARY;
	for (i = 0; i < VT_ROWS * VT_COLS; i++)
		screen[i] = 0x0720;
	vtinit(&v, screen, mono);
	replies[0] = '\0';
	bells = 0;
}

static void
w(const char *s)
{
	vtwrite(&v, (u_char *)s, (int)strlen(s));
}

static void
wn(const char *s, int n)
{
	vtwrite(&v, (u_char *)s, n);
}

/* "R0", "R1" ... on the first n rows, leaving the cursor on the last. */
static void
lines(int n)
{
	char buf[16];
	int i;

	for (i = 0; i < n; i++) {
		snprintf(buf, sizeof (buf), i ? "\r\nR%d" : "R%d", i);
		w(buf);
	}
}

/* Row y's characters, without the blanks at the end. */
static const char *
row(int y)
{
	static char buf[VT_COLS + 1];
	int x, end = 0;

	for (x = 0; x < VT_COLS; x++) {
		buf[x] = screen[y * VT_COLS + x] & 0xFF;
		if (buf[x] != ' ')
			end = x + 1;
	}
	buf[end] = '\0';
	return (buf);
}

static const char *
printable(const char *s)
{
	static char buf[512];
	char *cp = buf;

	for (; *s && cp < buf + sizeof (buf) - 5; s++)
		if (*s == 033)
			cp += sprintf(cp, "\\033");
		else
			*cp++ = *s;
	*cp = '\0';
	return (buf);
}

static void
check(int ok, const char *fmt, ...)
{
	va_list ap;

	if (ok) {
		pass++;
		return;
	}
	fail++;
	printf("vt: %s: ", name);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	putchar('\n');
}

static void
isrow(int y, const char *want)
{
	const char *got = row(y);

	check(strcmp(got, want) == 0, "row %d is \"%s\", not \"%s\"",
	    y, got, want);
}

static void
iscursor(int x, int y)
{
	check(v.v_x == x && v.v_y == y, "cursor at column %d row %d, not %d %d",
	    v.v_x, v.v_y, x, y);
}

static void
iscell(int x, int y, int want)
{
	int got = screen[y * VT_COLS + x];

	check(got == want, "cell at column %d row %d is %04x, not %04x",
	    x, y, got, want);
}

static void
isreply(const char *want)
{
	char got[sizeof (replies)];

	strcpy(got, printable(replies));
	check(strcmp(replies, want) == 0, "answered \"%s\", not \"%s\"",
	    got, printable(want));
	replies[0] = '\0';
}

/* Did the last case write outside the screen? */
static void
guards(void)
{
	int i, bad = 0;

	if (name == NULL)
		return;
	for (i = 0; i < GUARD; i++)
		if (memory[i] != CANARY ||
		    memory[GUARD + VT_ROWS * VT_COLS + i] != CANARY)
			bad++;
	check(bad == 0, "wrote %d cells outside the screen", bad);
}

/* A string of n copies of c, then tail. */
static const char *
repeat(int c, int n, const char *tail)
{
	static char buf[VT_COLS + 8];

	memset(buf, c, n);
	strcpy(buf + n, tail);
	return (buf);
}

static void
text(void)
{
	start("plain text", 0);
	w("hello");
	isrow(0, "hello");
	iscursor(5, 0);
	iscell(0, 0, 0x0700 | 'h');

	start("CR and LF", 0);
	w("ab\r\ncd");
	isrow(0, "ab");
	isrow(1, "cd");
	iscursor(2, 1);

	start("LF keeps the column", 0);
	w("ab\ncd");
	isrow(1, "  cd");
	iscursor(4, 1);

	start("VT and FF are LF", 0);
	w("a\013b\014c");
	isrow(1, " b");
	isrow(2, "  c");

	start("codes from 0200 up are the font's", 0);
	w("\263\315");
	iscell(0, 0, 0x07B3);
	iscell(1, 0, 0x07CD);

	start("NUL and DEL show nothing", 0);
	wn("a\0b\177c", 5);
	isrow(0, "abc");

	start("index and next line", 0);
	w("ab\033Dc\033Ed");
	isrow(1, "  c");
	isrow(2, "d");

	start("the bell", 0);
	w("\007a\007");
	check(bells == 2, "rang %d times", bells);
	isrow(0, "a");
}

static void
wrap(void)
{
	const char *line = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
	    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

	start("the last column waits", 0);
	w(line);
	iscursor(79, 0);
	check(v.v_flags & VT_WRAP, "no wrap to come");
	isrow(1, "");
	w("y");
	isrow(0, line);
	isrow(1, "y");
	iscursor(1, 1);

	start("CR LF after the last column is one line", 0);
	w(line);
	w("\r\nz");
	isrow(1, "z");
	isrow(2, "");

	start("BS after the last column", 0);
	w(line);
	w("\bQ");
	isrow(0, repeat('x', 78, "Qx"));
	iscursor(79, 0);
	check(!(v.v_flags & VT_WRAP), "wrap to come");

	start("no autowrap", 0);
	w("\033[?7l");
	w(line);
	w("yz");
	isrow(0, repeat('x', 79, "z"));
	isrow(1, "");
	iscursor(79, 0);

	start("the last line scrolls", 0);
	w("\033[25;1H");
	w(line);
	w("y");
	isrow(23, line);
	isrow(24, "y");

	start("erasing ends the wait", 0);
	w(line);
	w("\033[KZ");
	isrow(0, repeat('x', 79, "Z"));
	isrow(1, "");

	start("autowrap off ends the wait", 0);
	w(line);
	w("\033[?7lZ");
	isrow(0, repeat('x', 79, "Z"));
	isrow(1, "");
}

static void
regions(void)
{
	start("scrolling", 0);
	lines(25);
	w("\r\nR25");
	isrow(0, "R1");
	isrow(24, "R25");
	iscursor(3, 24);

	start("a scrolling region", 0);
	lines(25);
	w("\033[5;10r");
	iscursor(0, 0);
	w("\033[10;1H\n");
	isrow(3, "R3");
	isrow(4, "R5");
	isrow(8, "R9");
	isrow(9, "");
	isrow(10, "R10");
	iscursor(0, 9);

	start("reverse index at the top margin", 0);
	lines(25);
	w("\033[5;10r\033[5;1H\033M");
	isrow(3, "R3");
	isrow(4, "");
	isrow(5, "R4");
	isrow(9, "R8");
	isrow(10, "R10");

	start("below the region a line feed does not scroll", 0);
	lines(25);
	w("\033[5;10r\033[25;1H\nX");
	isrow(0, "R0");
	isrow(9, "R9");
	isrow(24, "X24");

	start("origin mode", 0);
	w("\033[5;10r\033[?6h");
	iscursor(0, 4);
	w("\033[1;1HX\033[20;1HY");
	isrow(4, "X");
	isrow(9, "Y");
	w("\033[6n");
	isreply("\033[6;2R");
	w("\033[?6l");
	iscursor(0, 0);

	start("cursor movement stops at the margins", 0);
	w("\033[5;10r\033[7;1H\033[20A");
	iscursor(0, 4);
	w("\033[20B");
	iscursor(0, 9);
	w("\033[3;1H\033[20B");
	iscursor(0, 9);
	w("\033[12;1H\033[20A");
	iscursor(0, 4);
	w("\033[12;1H\033[20B");
	iscursor(0, 24);
	w("\033[3;1H\033[20A");
	iscursor(0, 0);

	start("a region of fewer than two lines is refused", 0);
	w("\033[3;3H\033[10;5r");
	iscursor(2, 2);
	w("\033[5;5r");
	iscursor(2, 2);
	w("\033[5;26r");
	iscursor(2, 2);
	check(v.v_top == 0 && v.v_bot == 24, "region %d to %d", v.v_top, v.v_bot);
	w("\033[r");
	iscursor(0, 0);
}

static void
editing(void)
{
	start("insert lines", 0);
	lines(5);
	w("\033[2;3H\033[L");
	isrow(0, "R0");
	isrow(1, "");
	isrow(2, "R1");
	isrow(5, "R4");
	iscursor(0, 1);
	w("\033[2L");
	isrow(3, "");
	isrow(4, "R1");

	start("delete lines", 0);
	lines(5);
	w("\033[2;3H\033[M");
	isrow(1, "R2");
	isrow(3, "R4");
	isrow(4, "");
	iscursor(0, 1);

	start("lines inserted in a region stay in it", 0);
	lines(25);
	w("\033[5;10r\033[8;1H\033[3L");
	isrow(6, "R6");
	isrow(7, "");
	isrow(9, "");
	isrow(10, "R10");

	start("lines deleted in a region", 0);
	lines(25);
	w("\033[5;10r\033[8;1H\033[2M");
	isrow(7, "R9");
	isrow(8, "");
	isrow(9, "");
	isrow(10, "R10");

	start("insert and delete lines outside the region do nothing", 0);
	lines(25);
	w("\033[5;10r\033[2;1H\033[L\033[M");
	isrow(1, "R1");
	isrow(4, "R4");

	start("insert, delete and erase characters", 0);
	w("abcdef\033[1;3H\033[2@");
	isrow(0, "ab  cdef");
	iscursor(2, 0);
	w("\033[2P");
	isrow(0, "abcdef");
	w("\033[3X");
	isrow(0, "ab   f");
	w("\033[100P");
	isrow(0, "ab");
	w("\r");
	w(repeat('y', 78, "\033[1;79H\033[5@z"));
	isrow(0, repeat('y', 78, "z"));

	start("insert mode", 0);
	w("abc\033[1;2H\033[4hX");
	isrow(0, "aXbc");
	w("\033[4lY");
	isrow(0, "aXYc");

	start("erase in line", 0);
	w("abcdefgh\033[1;4H\033[K");
	isrow(0, "abc");
	w("\rabcdefgh\033[1;4H\033[1K");
	isrow(0, "    efgh");
	w("\033[2K");
	isrow(0, "");

	start("erase in display", 0);
	w("abc\r\ndef\r\nghi\033[2;2H\033[J");
	isrow(0, "abc");
	isrow(1, "d");
	isrow(2, "");
	start("erase in display, to the cursor", 0);
	w("abc\r\ndef\r\nghi\033[2;2H\033[1J");
	isrow(0, "");
	isrow(1, "  f");
	isrow(2, "ghi");
	w("\033[2J");
	isrow(2, "");
	iscursor(1, 1);
}

static void
tabs(void)
{
	start("tab stops every eight columns", 0);
	w("\tX\tY");
	iscell(8, 0, 0x0700 | 'X');
	iscell(16, 0, 0x0700 | 'Y');
	w("\033[1;76H\t\tZ");
	iscell(79, 0, 0x0700 | 'Z');

	start("setting and clearing tab stops", 0);
	w("\033[3g\tA");
	iscell(79, 0, 0x0700 | 'A');
	w("\r\033[5C\033H\r\tB");
	iscell(5, 0, 0x0700 | 'B');
	w("\r\033[5C\033[g\r\tC");
	iscell(79, 0, 0x0700 | 'C');
}

static void
saving(void)
{
	start("save and restore the cursor", 0);
	w("\033[3;4H\033[1m\0337\033[H\033[0m\0338X");
	iscell(3, 2, 0x0F00 | 'X');
	iscursor(4, 2);

	start("restore with nothing saved", 0);
	w("\033[5;5H\033[7m\0338X");
	iscell(0, 0, 0x0700 | 'X');

	start("the character sets are saved", 0);
	w("\033(0\0337\033(B\0338q");
	iscell(0, 0, 0x07C4);

	start("SCO save and restore", 0);
	w("\033[3;4H\033[s\033[H\033[uX");
	iscell(3, 2, 0x0700 | 'X');
}

static void
charsets(void)
{
	start("DEC special graphics", 0);
	w("\033(0lqkxjm`a~\033(Bq");
	iscell(0, 0, 0x07DA);
	iscell(1, 0, 0x07C4);
	iscell(2, 0, 0x07BF);
	iscell(3, 0, 0x07B3);
	iscell(4, 0, 0x07D9);
	iscell(5, 0, 0x07C0);
	iscell(6, 0, 0x0704);
	iscell(7, 0, 0x07B1);
	iscell(8, 0, 0x07FA);
	iscell(9, 0, 0x0700 | 'q');

	start("G1 by SO and SI", 0);
	w("\033)0\016q\017q");
	iscell(0, 0, 0x07C4);
	iscell(1, 0, 0x0700 | 'q');

	start("the British set", 0);
	w("\033(A#\033(B#");
	iscell(0, 0, 0x079C);
	iscell(1, 0, 0x0700 | '#');
}

static void
rendition(void)
{
	start("rendition", 0);
	w("\033[1mA\033[4mB\033[0;5mC\033[0;7mD\033[1;4;5;7mE"
	    "\033[22;24;25;27mF");
	iscell(0, 0, 0x0F00 | 'A');
	iscell(1, 0, 0x0B00 | 'B');
	iscell(2, 0, 0x8700 | 'C');
	iscell(3, 0, 0x7000 | 'D');
	iscell(4, 0, 0xB800 | 'E');
	iscell(5, 0, 0x0700 | 'F');

	start("colours", 0);
	w("\033[31;44mA\033[39mB\033[49mC\033[32;1mD\033[0mE\033[33;46mF");
	iscell(0, 0, 0x1400 | 'A');
	iscell(1, 0, 0x1700 | 'B');
	iscell(2, 0, 0x0700 | 'C');
	iscell(3, 0, 0x0A00 | 'D');
	iscell(4, 0, 0x0700 | 'E');
	iscell(5, 0, 0x3600 | 'F');

	start("erasing leaves the background colour", 0);
	w("\033[44;1;7m\033[2J");
	iscell(0, 0, 0x1720);
	iscell(79, 24, 0x1720);

	start("monochrome attributes", 1);
	w("A\033[1mB\033[0;4mC\033[0;7mD\033[0;31;44mE");
	iscell(0, 0, 0x0700 | 'A');
	iscell(1, 0, 0x0F00 | 'B');
	iscell(2, 0, 0x0100 | 'C');
	iscell(3, 0, 0x7000 | 'D');
	iscell(4, 0, 0x0700 | 'E');

	start("reverse screen", 0);
	w("ab\033[?5h");
	iscell(0, 0, 0x7000 | 'a');
	iscell(5, 5, 0x7020);
	w("c\033[7md");
	iscell(2, 0, 0x7000 | 'c');
	iscell(3, 0, 0x0700 | 'd');
	w("\033[?5l");
	iscell(0, 0, 0x0700 | 'a');
	iscell(3, 0, 0x7000 | 'd');
	w("\033[0m\033[?5h\033[2J");
	iscell(40, 12, 0x7020);
}

static void
modes(void)
{
	start("newline mode", 0);
	w("\033[20hab\ncd");
	isrow(1, "cd");
	w("\033[20l\nef");
	isrow(2, "  ef");

	start("keyboard modes", 0);
	w("\033[?1h\033=");
	check((v.v_flags & (VT_CKM | VT_KPAM)) == (VT_CKM | VT_KPAM),
	    "flags %04x", v.v_flags);
	w("\033[?1l\033>");
	check((v.v_flags & (VT_CKM | VT_KPAM)) == 0, "flags %04x", v.v_flags);
	w("\033[?25l");
	check(!(v.v_flags & VT_TCEM), "cursor shown");
	w("\033[?25h");
	check(v.v_flags & VT_TCEM, "cursor hidden");
	w("\033[?8l");
	check(!(v.v_flags & VT_ARM), "keys repeat");

	start("several modes at once", 0);
	w("\033[?1;7;25l");
	check((v.v_flags & (VT_CKM | VT_AWM | VT_TCEM)) == 0, "flags %04x",
	    v.v_flags);
	w("\033[?7;25h");
	check((v.v_flags & (VT_AWM | VT_TCEM)) == (VT_AWM | VT_TCEM),
	    "flags %04x", v.v_flags);
}

static void
answers(void)
{
	start("answers", 0);
	w("\033[c");
	isreply("\033[?6c");
	w("\033[0c");
	isreply("\033[?6c");
	w("\033Z");
	isreply("\033[?6c");
	w("\033[5n");
	isreply("\033[0n");
	w("\033[12;34H\033[6n");
	isreply("\033[12;34R");
	w("\033[x");
	isreply("\033[2;1;1;112;112;1;0x");
	w("\033[1x");
	isreply("\033[3;1;1;112;112;1;0x");
	w("\033[?15n");
	isreply("\033[?13n");
	w("\033[>c\033[1c\033[2x");
	isreply("");
}

static void
parsing(void)
{
	start("CAN ends a sequence", 0);
	w("\033[2\030J");
	isrow(0, "J");

	start("SUB ends a sequence", 0);
	w("\033[2\032K");
	isrow(0, "K");

	start("controls in a sequence are done", 0);
	w("abc\033[1\bDX");
	isrow(0, "aXc");

	start("ESC starts a sequence over", 0);
	w("ab\033[3\033[1;1HX");
	isrow(0, "Xb");

	start("huge parameters", 0);
	w("\033[99999;99999H");
	iscursor(79, 24);
	w("\033[65535A");
	iscursor(79, 0);

	start("too many parameters", 0);
	w("\033[1;1;1;1;1;1;1;1;1;1;1;1;1;1;1;1;1;31mX");
	iscell(0, 0, 0x0F00 | 'X');

	start("a malformed sequence is ignored", 0);
	w("\033[1:2HX\033[2;?HY\033[1$pZ");
	isrow(0, "XYZ");
	iscursor(3, 0);

	start("unknown escapes are ignored", 0);
	w("\033 F\033%GX\033(Y\033#3Z");
	isrow(0, "XZ");

	start("a private marker other than ?", 0);
	w("\033[>1hX");
	isrow(0, "X");
	check(!(v.v_flags & VT_CKM), "flags %04x", v.v_flags);

	start("codes from 0200 up end a sequence", 0);
	w("\033[2\351X");
	isrow(0, "X");
}

static void
others(void)
{
	start("the alignment pattern", 0);
	w("\033[5;10r\033[3;3H\033#8");
	isrow(0, repeat('E', 80, ""));
	isrow(24, repeat('E', 80, ""));
	iscursor(0, 0);
	check(v.v_top == 0 && v.v_bot == 24, "region %d to %d", v.v_top, v.v_bot);

	start("reset", 0);
	w("\033[5;10r\033[?6h\033[1;31m\033(0abc\033c");
	isrow(0, "");
	iscursor(0, 0);
	check(v.v_top == 0 && v.v_bot == 24, "region %d to %d", v.v_top, v.v_bot);
	check(v.v_flags == (VT_AWM | VT_TCEM | VT_ARM), "flags %04x", v.v_flags);
	w("q");
	iscell(0, 0, 0x0700 | 'q');

	start("cursor movement", 0);
	w("\033[10;10H\033[3A");
	iscursor(9, 6);
	w("\033[2B");
	iscursor(9, 8);
	w("\033[5C");
	iscursor(14, 8);
	w("\033[20D");
	iscursor(0, 8);
	w("\033[5G");
	iscursor(4, 8);
	w("\033[3d");
	iscursor(4, 2);
	w("\033[2E");
	iscursor(0, 4);
	w("\033[1F");
	iscursor(0, 3);
	w("\033[3a");
	iscursor(3, 3);
	w("\033[2e");
	iscursor(3, 5);
	w("\033[H");
	iscursor(0, 0);
	w("\033[;5H");
	iscursor(4, 0);
	w("\033[5;f");
	iscursor(0, 4);
	w("\033[0;0H");
	iscursor(0, 0);
}

static void
vt52(void)
{
	start("VT52 mode", 0);
	w("\033[?2l");
	check(v.v_flags & VT_VT52, "not in VT52 mode");
	w("\033Y%(X");
	iscell(8, 5, 0x0700 | 'X');
	w("\033A\033A\033DY");
	iscell(8, 3, 0x0700 | 'Y');
	w("\033H\033FqZ\033Gq");
	iscell(0, 0, 0x07C4);
	iscell(1, 0, 0x0700 | 'Z');
	iscell(2, 0, 0x0700 | 'q');
	w("\033Z");
	isreply("\033/Z");
	w("\033[1;1H");
	iscell(3, 0, 0x0700 | '1');
	w("\033<");
	check(!(v.v_flags & VT_VT52), "still in VT52 mode");
	w("\033[HW");
	iscell(0, 0, 0x0700 | 'W');

	start("VT52 erasing", 0);
	w("abc\r\ndef\r\nghi\033[?2l\033Y! \033K");
	isrow(0, "abc");
	isrow(1, "");
	isrow(2, "ghi");
	w("\033H\033B\033B\033C\033J");
	isrow(2, "g");
}

static void
kernel(void)
{
	start("the kernel's messages are plain", 0);
	w("\033[1;7mab\033[1");
	vtkputc(&v, 'K');
	vtkputc(&v, '\n');
	vtkputc(&v, 'L');
	w("mX");
	iscell(0, 0, 0x7800 | 'a');
	iscell(2, 0, 0x0700 | 'K');
	iscell(0, 1, 0x0700 | 'L');
	iscell(1, 1, 0x7800 | 'X');
}

/*
 * Random bytes, weighted to escape sequences and control characters.
 * Whatever comes, the cursor and the region must stay on the screen and
 * every write within it, which the sanitizer sees to.
 */
static void
noise(void)
{
	static const char some[] = "\033[;?0123456789\r\n\b\t\016\017#()"
	    "ABCDEFGHJKLMPXZ@`acdefghlmnqrsux78=><Y";
	u_char buf[64];
	unsigned seed = 286;
	int i, j, ok = 1;

	start("random bytes", 0);
	for (i = 0; i < 20000 && ok; i++) {
		for (j = 0; j < (int)sizeof (buf); j++) {
			seed = seed * 1103515245 + 12345;
			buf[j] = seed >> 16 & 1 ? some[(seed >> 17) %
			    (sizeof (some) - 1)] : seed >> 20;
		}
		vtwrite(&v, buf, (int)sizeof (buf));
		replies[0] = '\0';
		ok = v.v_x < VT_COLS && v.v_y < VT_ROWS && v.v_top < v.v_bot &&
		    v.v_bot < VT_ROWS && v.v_gl < 2;
	}
	check(ok, "after %d writes: cursor %d %d, region %d to %d", i,
	    v.v_x, v.v_y, v.v_top, v.v_bot);
}

int
main(void)
{
	text();
	wrap();
	regions();
	editing();
	tabs();
	saving();
	charsets();
	rendition();
	modes();
	answers();
	parsing();
	others();
	vt52();
	kernel();
	noise();
	guards();
	printf("vt: pass=%d fail=%d\n", pass, fail);
	return (fail != 0);
}
