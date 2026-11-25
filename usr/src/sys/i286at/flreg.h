/*
 * The diskette driver's one request of its own (fl.c), after the RX02's
 * (rxreg.h): format the diskette, as the kind the drive takes that has
 * the kilobytes given, or with 0, as the drive's largest.
 */
#define	FLIOC_FORMAT	_IOW('r', 1, int)
