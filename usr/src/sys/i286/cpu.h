/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_AHABUSON		2	/* int: microseconds the SCSI adapter
					   holds the AT bus, */
#define	CPU_AHABUSOFF		3	/* int: and lets it go */
#define	CPU_AHASPEED		4	/* int: its speed on that bus, in
					   tenths of a megabyte a second */
#define	CPU_MAXID		5	/* number of valid machdep ids */

#ifndef	KERNEL
#define CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
	{ "aha_buson", CTLTYPE_INT }, \
	{ "aha_busoff", CTLTYPE_INT }, \
	{ "aha_speed", CTLTYPE_INT }, \
}
#endif
