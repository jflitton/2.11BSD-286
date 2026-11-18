/*
 * The partition table in the first sector of a PC's hard disk.
 */
#define	PCP_TABLE	446	/* where it is in the sector */
#define	PCP_NPART	4

/*
 * The number that says a part of the disk is 2.11BSD's.  Nobody hands
 * these numbers out; this one is not known to be anyone else's.
 */
#define	PCP_2BSD	0xB9

struct pcpart {
	u_char	pp_active;	/* 0x80: the one to boot */
	u_char	pp_shead;	/* where it starts, as the BIOS counts: */
	u_char	pp_ssect;	/*   the sector, from 1, and in the top two
				     bits the cylinder's top two */
	u_char	pp_scyl;
	u_char	pp_type;
	u_char	pp_ehead;	/* its last sector, the same way */
	u_char	pp_esect;
	u_char	pp_ecyl;
	u_long	pp_start;	/* its first sector, counting from 0 */
	u_long	pp_size;	/* how many sectors */
};

#ifdef KERNEL
daddr_t	pcpart();
#endif
