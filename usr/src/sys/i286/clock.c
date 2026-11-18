/*
 * The clock: channel 0 of the 8254 timer, on interrupt request 0, and
 * the battery-backed clock that knows the date.
 */
#include "param.h"
#include "../machine/frame.h"
#include "systm.h"
#include "kernel.h"

#define	TIMER0		0x40		/* channel 0's counter */
#define	TIMER_MODE	0x43
#define	TIMER_RATE	1193182L	/* counts a second */
#define	TIMER_SQWAVE	0x36		/* channel 0, both bytes, mode 3 */

#define	RTC_ADDR	0x70		/* which byte of the clock chip */
#define	RTC_DATA	0x71
#define	RTC_SEC		0
#define	RTC_MIN		2
#define	RTC_HOUR	4
#define	RTC_DAY		7
#define	RTC_MONTH	8
#define	RTC_YEAR	9
#define	RTC_STATA	10
#define	RTC_UIP		0x80		/* the time is changing: wait */
#define	RTC_CENTURY	0x37		/* on a PS/2; an AT keeps it at 0x32 */

static int clkintr();

clkstart()
{
	register u_int count = TIMER_RATE / hz;

	outb(TIMER_MODE, TIMER_SQWAVE);
	outb(TIMER0, count);
	outb(TIMER0, count >> 8);
	setirq(0, IPL_CLOCK, clkintr, 0);
	enable();		/* for the first time since the BIOS */
}

/*
 * tf is what the clock interrupted, or 0 if the tick was put off, which
 * only the kernel can have done.
 */
static
clkintr(arg, tf)
	int arg;
	register struct trapframe *tf;
{
	if (tf == 0)
		hardclock(0, 0);
	else if (USERMODE(tf->tf_cs))
		hardclock(tf->tf_ip, CLK_USER | CLK_BASE);
	else
		hardclock(tf->tf_ip, 0);
}

static
rtc(reg)
	int reg;
{
	register int v;

	outb(RTC_ADDR, reg);
	v = inb(RTC_DATA);
	return ((v >> 4) * 10 + (v & 15));	/* it counts in decimal */
}

/*
 * The time of year, in seconds since 1970, or 0 if the clock chip does
 * not make sense.  The chip is taken to keep Greenwich time.
 */
time_t
toyclk()
{
	static char dmsize[12] =
	    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	register int i, year;
	int month, day;
	long t;

	for (i = 0; i < 10000; i++) {
		outb(RTC_ADDR, RTC_STATA);
		if ((inb(RTC_DATA) & RTC_UIP) == 0)
			break;
	}
	year = rtc(RTC_YEAR);
	month = rtc(RTC_MONTH);
	day = rtc(RTC_DAY);
	year += year < 70 ? 2000 : 1900;
	if (month < 1 || month > 12 || day < 1 || day > 31)
		return (0);
	t = 0;
	for (i = 1970; i < year; i++)
		t += (i % 4) == 0 ? 366 : 365;
	for (i = 1; i < month; i++) {
		t += dmsize[i - 1];
		if (i == 2 && (year % 4) == 0)
			t++;
	}
	t += day - 1;
	t = t * 24 + rtc(RTC_HOUR);
	t = t * 60 + rtc(RTC_MIN);
	t = t * 60 + rtc(RTC_SEC);
	return (t);
}
