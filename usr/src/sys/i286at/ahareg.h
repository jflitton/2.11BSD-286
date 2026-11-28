/*
 * The Adaptec AHA-1540 and 1542 SCSI host adapters: B, C, CF and CP.
 * The adapter is given commands a byte at a time through its ports, and
 * SCSI commands as blocks of memory, the CCBs, whose addresses it is
 * handed in mailboxes.  Every address it is given is physical, 24 bits,
 * most significant byte first.  (AHA-1540C series technical reference.)
 */

/* Ports, from the adapter's base. */
#define	AHA_CTRL	0	/* write */
#define	AHA_STAT	0	/* read */
#define	AHA_DATA	1	/* write: command bytes; read: replies */
#define	AHA_INTR	2	/* read: why it is interrupting */

#define	CTRL_HRST	0x80	/* reset and test itself */
#define	CTRL_SRST	0x40	/* reset */
#define	CTRL_IRST	0x20	/* take back the interrupt request */
#define	CTRL_SCRST	0x10	/* reset the SCSI bus */

#define	ST_STST		0x80	/* testing itself */
#define	ST_DIAGF	0x40	/* and failed */
#define	ST_INIT		0x20	/* wants its mailboxes set up */
#define	ST_IDLE		0x10
#define	ST_CDF		0x08	/* not ready for another byte */
#define	ST_DF		0x04	/* has a byte of reply */
#define	ST_INVDCMD	0x01	/* did not know that command */
#define	ST_MASK		0xFD

#define	IF_ANY		0x80
#define	IF_SCRD		0x08	/* the SCSI bus was reset */
#define	IF_HACC		0x04	/* adapter command done */
#define	IF_MBOA		0x02	/* a mailbox out is free again */
#define	IF_MBIF		0x01	/* a mailbox in has something */
#define	IF_MASK		0x8F

/* Adapter commands. */
#define	AC_MBINIT	0x01	/* count, address: its mailboxes */
#define	AC_START	0x02	/* look in the mailboxes out */
#define	AC_INQUIRY	0x04	/* replies board, options, two of firmware */
#define	AC_BUSON	0x07	/* how long it holds the AT bus, */
#define	AC_BUSOFF	0x08	/*   and lets it go, microseconds */
#define	AC_SPEED	0x09	/* its speed on the AT bus, a code */
#define	AC_RETCONF	0x0B	/* replies DMA channel, interrupt, SCSI id */
#define	AC_RETSETUP	0x0D	/* count: replies that much of how it is set */
#define	AC_EXTBIOS	0x28	/* C and later: replies flags, lock code */
#define	AC_MBENABLE	0x29	/* C and later: 0, lock code: unlock */

#define	XB_XLATE	0x08	/* AC_EXTBIOS: its BIOS counts 255 heads and
				   63 sectors on disks over 1 GB */

struct mbox {
	u_char	mb_status;
	u_char	mb_ccb[3];	/* the CCB's address */
};
#define	AHA_NMBOX	4	/* out, and as many in */

#define	MBO_FREE	0
#define	MBO_START	1

/* What a mailbox in says of a CCB; AHA_LOST and AHA_DEAD are ahago's own. */
#define	MBI_FREE	0
#define	MBI_OK		1
#define	MBI_ABORTED	2
#define	MBI_NOTFOUND	3
#define	MBI_ERROR	4	/* see cc_hastat and cc_tarstat */
#define	AHA_LOST	(-1)	/* it never answered, and has been reset */
#define	AHA_DEAD	(-2)	/* and would not come back */

#define	AHA_NCDB	12
#define	AHA_NSENSE	14

struct ccb {
	u_char	cc_op;
	u_char	cc_idlun;	/* target << 5 | direction | unit */
	u_char	cc_cdblen;
	u_char	cc_senselen;
	u_char	cc_datalen[3];
	u_char	cc_dataptr[3];
	u_char	cc_linkptr[3];
	u_char	cc_linkid;
	u_char	cc_hastat;	/* how the adapter fared */
	u_char	cc_tarstat;	/* the status byte the target sent */
	u_char	cc_reserved[2];
	u_char	cc_cdb[AHA_NCDB + AHA_NSENSE];	/* the SCSI command, and
				   straight after it the sense data, if
				   the target had any */
};

#define	CCB_INITIATOR	0x00
#define	CCB_PIECES	0x02	/* cc_op: the data is in pieces, and
				   cc_dataptr and cc_datalen are of a
				   list of them, struct ahaseg */
#define	CCB_RESID	0x03	/* C and later: cc_datalen is given back
				   as what was not moved (aharesid) */
#define	CCB_DATAIN	0x08	/* cc_idlun: target to memory */
#define	CCB_DATAOUT	0x10
#define	CCB_NODATA	0x18

/*
 * A piece of a command's data.  A 1542B takes 17 of them, a C 255; a
 * driver that sends its data in pieces has a list of AHA_NSEG.
 */
struct ahaseg {
	u_char	sg_len[3];
	u_char	sg_addr[3];
};
#define	AHA_NSEG	16

#define	HS_OK		0x00	/* cc_hastat */
#define	HS_SELTIMEOUT	0x11	/* no such target */
#define	HS_DATARUN	0x12	/* more or less data than was asked for */

#define	SS_GOOD		0x00	/* cc_tarstat */
#define	SS_CHECK	0x02	/* there is sense data */
#define	SS_BUSY		0x08

/* SCSI commands and sense keys. */
#define	SC_INQUIRY	0x12
#define	SC_RDCAP	0x25
#define	SC_READ10	0x28
#define	SC_WRITE10	0x2A

#define	SK_RECOVERED	1
#define	SK_NOTREADY	2
#define	SK_MEDIUM	3
#define	SK_HARDWARE	4
#define	SK_ILLEGAL	5
#define	SK_ATTENTION	6
#define	SK_PROTECT	7
#define	SK_BLANK	8	/* nothing written there */
#define	SK_OVERFLOW	0x0D	/* no room left for what was written */

#ifdef KERNEL
extern int ahatest;		/* what is made to go wrong (aha.c) */
#endif
