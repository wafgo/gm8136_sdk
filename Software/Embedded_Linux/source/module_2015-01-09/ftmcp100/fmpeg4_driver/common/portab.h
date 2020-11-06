#ifndef _PORTAB_H_
#define _PORTAB_H_
/*****************************************************************************
 *  Common things
 ****************************************************************************/

	#ifdef LINUX
		#define __int64		long long
		#define __align8 	__attribute__ ((aligned (8)))
		#define __align16 	__attribute__ ((aligned (16)))
		#define FUN_ABS(x)		abs(x)
	#else		/// CodeWarrior
		#define __align16 	__align(16)
		#include <string.h>
//		#include <stdlib.h>
		#define FUN_ABS(x)		abs(x)
	#endif

	#ifdef FPGA
		#ifdef LINUX
			#define DEBUG(x)			printk(x)
		#else
			//#include <stdio.h>
			#define DEBUG(x)			printf(x)
		#endif
	#else
		#define DEBUG(x)
	#endif

	/* Debug level masks */
	#define DPRINTF_ERROR		0x00000001
	#define DPRINTF_STARTCODE	0x00000002
	#define DPRINTF_HEADER		0x00000004
	#define DPRINTF_TIMECODE	0x00000008
	#define DPRINTF_MB			0x00000010
	#define DPRINTF_COEFF		0x00000020
	#define DPRINTF_MV			0x00000040
	#define DPRINTF_DEBUG		0x80000000

	/* debug level for this library */
	#define DPRINTF_LEVEL		0

	/* Buffer size for non C99 compliant compilers (msvc) */
	#define DPRINTF_BUF_SZ  	1024

/*****************************************************************************
 *  Types used in Gm sources
 ****************************************************************************/

/*----------------------------------------------------------------------------
 | Standard Unix include file (sorry, we put all unix into "linux" case)
 *---------------------------------------------------------------------------*/


	//#    define boolean int
//	#define boolean unsigned char
	#define int8_t   char
	#define uint8_t  unsigned char
	#define int16_t  short
	#define uint16_t unsigned short
	#define int32_t  int
	#define uint32_t unsigned int
	#define int64_t  __int64
	#define uint64_t unsigned __int64
	#define bool int
#ifndef LINUX
	#define NULL 0
#endif

/*****************************************************************************
 *  Some things that are only architecture dependant
 ****************************************************************************/
	#define CACHE_LINE  16
	#define ptr_t uint32_t
// define bitstream in/out will follow big-endian format
// byte offset:	0		1		2		3		4		5		6		7
// value:			0x00		0x00		0x01		0x02		0x00		0x00		0x01		0xb6
	#define BS_IS_BIG_ENDIAN

#ifndef DEFINE_VECTOR
	#define DEFINE_VECTOR
	union VECTOR1
	{
		uint32_t u32num;
		struct
		{
			int16_t s16y;
			int16_t s16x;
		} vec;
	};
#endif

/*----------------------------------------------------------------------------
 | gcc SPARC specific macros/functions
 *---------------------------------------------------------------------------*/
	#define BSWAP(a) \
                ((a) = (((a) & 0xff) << 24)  | (((a) & 0xff00) << 8) | \
                       (((a) >> 8) & 0xff00) | (((a) >> 24) & 0xff))


#endif
