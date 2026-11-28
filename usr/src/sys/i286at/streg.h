/*
 * SCSI tapes, st.c: the commands of a sequential access device, and
 * what its sense data says of a read, a write or a move.  (SCSI-2,
 * X3.131-1994, 10.)
 */

#define	SC_TUR		0x00	/* test unit ready */
#define	SC_REWIND	0x01
#define	SC_LIMITS	0x05	/* read block limits */
#define	SC_READ		0x08
#define	SC_WRITE	0x0A
#define	SC_WEOF		0x10	/* write filemarks: tape marks */
#define	SC_SPACE	0x11
#define	SC_MSELECT	0x15	/* mode select */
#define	SC_MSENSE	0x1A	/* mode sense */
#define	SC_LOAD		0x1B	/* load, or with 0 unload */

#define	SC_FIXED	0x01	/* SC_READ, SC_WRITE: the count is of
				   blocks, and not bytes of one record */
#define	SP_BLOCKS	0	/* SC_SPACE: over records */
#define	SP_MARKS	1	/* over tape marks */

/* The third byte of the sense data, with the key in its low four bits. */
#define	SF_MARK		0x80	/* a tape mark was read */
#define	SF_EOM		0x40	/* at the end of the tape, or near it */
#define	SF_ILI		0x20	/* a record was not the length asked for */

#define	SA_NOTAPE	0x3A	/* additional sense code: no tape in the
				   drive */

/*
 * The registers MTIOCGET gives: mt_dsreg has mtio.h's MTF_ bits in its
 * high byte, and mt_erreg the third byte of the last sense data in its
 * low byte, the key under SF_ bits, with the additional sense code
 * above it.  For mt(1).
 */
#define	STDS_BITS \
"\20\12EOM\14WRTLCK\15WRITTEN"
#define	STER_BITS \
"\20\6ILI\7EOM\10MARK"
