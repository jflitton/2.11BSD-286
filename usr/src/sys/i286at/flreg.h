/*
 * The diskette driver's one request of its own (fl.c), which is the
 * RX02's (rxreg.h): format the diskette, at the density of the device
 * that was opened.
 */
#define	FLIOC_FORMAT	_IO('r', 1)
