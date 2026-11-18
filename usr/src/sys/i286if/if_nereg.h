/*
 * The NE1000, the NE2000 and their many copies: a National Semiconductor
 * DP8390 network interface controller, or a copy of one, whose registers
 * are the card's first 16 ports, and the card's own two: a data port, the
 * only way to its buffer memory, and a reset port.
 */

/* DP8390 registers, in page 0 unless noted */
#define	NE_CR		0x00		/* command, in every page */
#define	NE_PSTART	0x01		/* w: first page of the receive ring */
#define	NE_PSTOP	0x02		/* w: the page after its last */
#define	NE_BNRY		0x03		/* boundary: the last page read */
#define	NE_TPSR		0x04		/* w: first page of a transmission */
#define	NE_TSR		0x04		/* r: transmit status */
#define	NE_TBCR0	0x05		/* w: bytes to transmit */
#define	NE_NCR		0x05		/* r: collisions */
#define	NE_TBCR1	0x06		/* w */
#define	NE_ISR		0x07		/* interrupt status */
#define	NE_RSAR0	0x08		/* w: remote DMA start address */
#define	NE_RSAR1	0x09		/* w */
#define	NE_RBCR0	0x0a		/* w: remote DMA byte count */
#define	NE_RBCR1	0x0b		/* w */
#define	NE_RCR		0x0c		/* w: receive configuration */
#define	NE_TCR		0x0d		/* w: transmit configuration */
#define	NE_CNTR0	0x0d		/* r: frame alignment errors */
#define	NE_DCR		0x0e		/* w: data configuration */
#define	NE_CNTR1	0x0e		/* r: CRC errors */
#define	NE_IMR		0x0f		/* w: interrupt mask */
#define	NE_CNTR2	0x0f		/* r: frames missed */
#define	NE_PAR0		0x01		/* page 1: station address, 6 */
#define	NE_CURR		0x07		/* page 1: page the next frame goes in */
#define	NE_MAR0		0x08		/* page 1: multicast filter, 8 */

/* the card's */
#define	NE_DATA		0x10		/* data port: remote DMA */
#define	NE_RESET	0x1f		/* a read resets the card */

/* CR */
#define	CR_STP		0x01		/* stop */
#define	CR_STA		0x02		/* start */
#define	CR_TXP		0x04		/* transmit */
#define	CR_RREAD	0x08		/* remote read */
#define	CR_RWRITE	0x10		/* remote write */
#define	CR_NODMA	0x20		/* abort or end remote DMA */
#define	CR_PAGE1	0x40

/* ISR and IMR */
#define	ISR_PRX		0x01		/* frame received */
#define	ISR_PTX		0x02		/* frame transmitted */
#define	ISR_RXE		0x04		/* receive error */
#define	ISR_TXE		0x08		/* transmit error */
#define	ISR_OVW		0x10		/* the receive ring is full */
#define	ISR_CNT		0x20		/* a tally counter is half full */
#define	ISR_RDC		0x40		/* remote DMA complete */
#define	ISR_RST		0x80		/* stopped; not an interrupt */

/* TSR */
#define	TSR_COL		0x04		/* collided, and sent */
#define	TSR_ABT		0x08		/* too many collisions: not sent */

/* RCR */
#define	RCR_AB		0x04		/* accept broadcasts */
#define	RCR_PRO		0x10		/* accept everything */
#define	RCR_MON		0x20		/* monitor: accept nothing */

/* TCR */
#define	TCR_LOOP	0x02		/* internal loopback */

/* DCR */
#define	DCR_WTS		0x01		/* word transfers */
#define	DCR_LS		0x08		/* not loopback */
#define	DCR_FT1		0x40		/* FIFO threshold: 8 bytes */

/*
 * What the controller puts before each frame in the receive ring, which
 * starts at the beginning of a 256-byte page.
 */
struct ne_ring {
	u_char	nr_rsr;			/* receive status */
	u_char	nr_next;		/* page of the next frame */
	u_short	nr_count;		/* bytes, these 4 included */
};
#define	RSR_PRX		0x01		/* nr_rsr: received intact */
