/*
 * The console, for now: the first serial port, 9600 bits a second, output
 * only and without interrupts.
 */
#define	COM1	0x3F8
#define	DATA	0		/* divisor, low byte, when DLAB is set */
#define	IER	1		/* divisor, high byte, when DLAB is set */
#define	LCR	3
#define	MCR	4
#define	LSR	5

#define	LCR_8N1		0x03
#define	LCR_DLAB	0x80
#define	MCR_DTR_RTS	0x03
#define	LSR_THRE	0x20	/* ready for a character */

cninit()
{
	outb(COM1 + LCR, LCR_DLAB);
	outb(COM1 + DATA, 115200L / 9600);
	outb(COM1 + IER, 0);
	outb(COM1 + LCR, LCR_8N1);
	outb(COM1 + IER, 0);
	outb(COM1 + MCR, MCR_DTR_RTS);
}

/*
 * Print a character on the console.  Waits for the port to be ready, but
 * not for ever: with nothing connected the status never changes.
 */
cnputc(c)
	char c;
{
	register int timo;

	if (c == '\n')
		cnputc('\r');
	timo = 30000;
	while ((inb(COM1 + LSR) & LSR_THRE) == 0)
		if (--timo == 0)
			break;
	outb(COM1 + DATA, c);
}
