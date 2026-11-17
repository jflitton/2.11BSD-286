/*
 * Public domain - 2002/1/30 - sms
 *
 *	@(#)stdint.h	1.2 (2.11BSD) 2026/1/26
*/

#ifndef	_STDINT_H_
#define	_STDINT_H_

/*
 * Note about 64-bit types:
 * C99 requires the presence of 64-bit types (in some form), but currently
 * the pdp11 compiler do not support them (long long is mapped to long).
 * They are #ifdef'd out here so that programs that want them will 
 * fail in compilation.
 * The 64-bit macros are however present, since they cannot cause any problem.
 */

/* C99 7.18.1.1 Exact-width integer types */
typedef	char		      int8_t;
typedef	unsigned char	      uint8_t;
typedef	int		      int16_t;
typedef	unsigned int	      uint16_t;
typedef	long		      int32_t;
typedef	unsigned long	      uint32_t;
#ifdef notyet
typedef	long long	      int64_t;
typedef unsigned long long    uint64_t;
#endif

/* C99 7.18.1.2 Minimum-width integer types */
typedef int8_t		int_least8_t;
typedef uint8_t		uint_least8_t;
typedef int16_t		int_least16_t;
typedef uint16_t	uint_least16_t;
typedef int32_t		int_least32_t;
typedef uint32_t	uint_least32_t;
#ifdef notyet
typedef int64_t		int_least64_t;
typedef uint64_t	uint_least64_t;
#endif

/* C99 7.18.1.3 Fastest minimum-width integer types */
typedef int8_t		int_fast8_t;
typedef uint8_t		uint_fast8_t;
typedef int16_t		int_fast16_t;
typedef uint16_t	uint_fast16_t;
typedef int32_t		int_fast32_t;
typedef uint32_t	uint_fast32_t;
#ifdef notyet
typedef int64_t		int_fast64_t;
typedef uint64_t	uint_fast64_t;
#endif

/* C99 7.18.1.4 Integer types capable of holding object pointers */
typedef		 int		intptr_t;
typedef unsigned int		uintptr_t;

/* C99 7.18.1.5 Greatest-width integer types */
typedef int32_t		intmax_t;	/* XXX int64_t - change below too */
typedef uint32_t	uintmax_t;	/* XXX uint64_t - change below too */

/* C99 7.18.3 other integer types */
typedef	unsigned int sig_atomic_t;

/* BSD compat types */
typedef uint8_t			      u_int8_t;
typedef uint16_t		      u_int16_t;
typedef uint32_t		      u_int32_t;
#ifdef notyet
typedef uint64_t		      u_int64_t;
#endif

/* C99 7.18.2 Limits of specified-width integer types */
#define INT8_MIN	(-0x7f-1)			/* int8_t	  */
#define INT16_MIN	(-0x7fff-1)			/* int16_t	  */
#define INT32_MIN	(-0x7fffffffL-1)		/* int32_t	  */
#define INT64_MIN	(-0x7fffffffffffffffLL-1)	/* int64_t	  */
#define INT8_MAX	0x7f				/* int8_t	  */
#define INT16_MAX	0x7fff				/* int16_t	  */
#define INT32_MAX	0x7fffffffL			/* int32_t	  */
#define INT64_MAX	0x7fffffffffffffffLL		/* int64_t	  */
#define UINT8_MAX	0xffU				/* uint8_t	  */
#define UINT16_MAX	0xffffU				/* uint16_t	  */
#define UINT32_MAX	0xffffffffUL			/* uint32_t	  */
#define UINT64_MAX	0xffffffffffffffffULL		/* uint64_t	  */
#define INT_LEAST8_MIN	(-0x7f-1)			/* int_least8_t	  */
#define INT_LEAST16_MIN (-0x7fff-1)			/* int_least16_t  */
#define INT_LEAST32_MIN (-0x7fffffffL-1)		/* int_least32_t  */
#define INT_LEAST64_MIN (-0x7fffffffffffffffLL-1)	/* int_least64_t  */
#define INT_LEAST8_MAX	0x7f				/* int_least8_t	  */
#define INT_LEAST16_MAX 0x7fff				/* int_least16_t  */
#define INT_LEAST32_MAX 0x7fffffffL			/* int_least32_t  */
#define INT_LEAST64_MAX 0x7fffffffffffffffLL		/* int_least64_t  */
#define UINT_LEAST8_MAX	 0xffU				/* uint_least8_t  */
#define UINT_LEAST16_MAX 0xffffU			/* uint_least16_t */
#define UINT_LEAST32_MAX 0xffffffffUL			/* uint_least32_t */
#define UINT_LEAST64_MAX 0xffffffffffffffffULL		/* uint_least64_t */
#define INT_FAST8_MIN	(-0x7f-1)			/* int_fast8_t	  */
#define INT_FAST16_MIN	(-0x7fff-1)			/* int_fast16_t	  */
#define INT_FAST32_MIN	(-0x7fffffffL-1)		/* int_fast32_t	  */
#define INT_FAST64_MIN	(-0x7fffffffffffffffLL-1)	/* int_fast64_t	  */
#define INT_FAST8_MAX	0x7f				/* int_fast8_t	  */
#define INT_FAST16_MAX	0x7fff				/* int_fast16_t	  */
#define INT_FAST32_MAX	0x7fffffffL			/* int_fast32_t	  */
#define INT_FAST64_MAX	0x7fffffffffffffffLL		/* int_fast64_t	  */
#define UINT_FAST8_MAX	0xffU				/* uint_fast8_t	  */
#define UINT_FAST16_MAX 0xffffU				/* uint_fast16_t  */
#define UINT_FAST32_MAX 0xffffffffUL			/* uint_fast32_t  */
#define UINT_FAST64_MAX 0xffffffffffffffffULL		/* uint_fast64_t  */
#define INTPTR_MIN	(-0x7fff-1)			/* intptr_t	  */
#define INTPTR_MAX	0x7fff				/* intptr_t	  */
#define UINTPTR_MAX	0xffffU				/* uintptr_t	  */
#define INTMAX_MIN	(-0x7fffffffL-1)		/* intmax_t	  */
#define INTMAX_MAX	0x7fffffffL			/* intmax_t	  */
#define UINTMAX_MAX	0xffffffffUL			/* uintmax_t	  */

/* C99 7.18.3 Limits of other integer types */
#define PTRDIFF_MIN	(-0x7fff-1)			/* ptrdiff_t	  */
#define PTRDIFF_MAX	0x7fff				/* ptrdiff_t	  */
#define SIG_ATOMIC_MIN	0xffffU				/* sig_atomic_t	  */
#define SIG_ATOMIC_MAX	0xffffU				/* sig_atomic_t	  */
#define SIZE_MAX	0xffffU				/* size_t	  */

/* C99 7.18.4 Macros for integer constants */
#define INT8_C(c)	c
#define INT16_C(c)	c
#define INT32_C(c)	c ## L
#define INT64_C(c)	c ## LL
#define UINT8_C(c)	c ## U
#define UINT16_C(c)	c ## U
#define UINT32_C(c)	c ## UL
#define UINT64_C(c)	c ## ULL
#define INTMAX_C(c)	c ## LL
#define UINTMAX_C(c)	c ## ULL

#endif /* _STDINT_H_ */
