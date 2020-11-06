/**
 *  @file bitstream.h
 *  @brief This file contains some operations for bitstream manipulation.
 *
 */
#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

#ifdef LINUX
#include "../common/portab.h"
#include "../common/dma_m.h"
#include "ftmcp100.h"
#include "encoder.h"
#include "local_mem_e.h"
#include "../common/define.h"
#else
#include "portab.h"
#include "dma_m.h"
#include "ftmcp100.h"
#include "encoder.h"
#include "local_mem_e.h"
#include "define.h"
#endif

/*****************************************************************************
 * Constants
 ****************************************************************************/

// comment any #defs we dont use
#define VIDO_RESYN_MARKER 0x1
#if 1
#define VUDOBJ_CODE				0x000001
#define VIDOBJ_START_CODE		0x00	/* ..0x0000011f  */
#define VIDOBJLAY_START_CODE	0x20	/* ..0x0000012f */
#define VISOBJSEQ_START_CODE	0xb0
#define VISOBJSEQ_STOP_CODE		0xb1	/* ??? */
#define USERDATA_START_CODE		0xb2
#define GRPOFVOP_START_CODE		0xb3
//#define VIDSESERR_ERROR_CODE  0xb4
#define VISOBJ_START_CODE		0xb5
//#define SLICE_START_CODE      0xb7
//#define EXT_START_CODE        0xb8
#else
#define VIDOBJ_START_CODE		0x00000100	/* ..0x0000011f  */
#define VIDOBJLAY_START_CODE	0x00000120	/* ..0x0000012f */
#define VISOBJSEQ_START_CODE	0x000001b0
#define VISOBJSEQ_STOP_CODE		0x000001b1	/* ??? */
#define USERDATA_START_CODE		0x000001b2
#define GRPOFVOP_START_CODE		0x000001b3
//#define VIDSESERR_ERROR_CODE  	0x000001b4
#define VISOBJ_START_CODE		0x000001b5
//#define SLICE_START_CODE      	0x000001b7
//#define EXT_START_CODE        	0x000001b8
#endif

#define VISOBJ_TYPE_VIDEO				1
//#define VISOBJ_TYPE_STILLTEXTURE      2
//#define VISOBJ_TYPE_MESH              3
//#define VISOBJ_TYPE_FBA               4
//#define VISOBJ_TYPE_3DMESH            5


#define VIDOBJLAY_TYPE_SIMPLE			1
//#define VIDOBJLAY_TYPE_SIMPLE_SCALABLE    2
#define VIDOBJLAY_TYPE_CORE				3
#define VIDOBJLAY_TYPE_MAIN				4


//#define VIDOBJLAY_AR_SQUARE           1
//#define VIDOBJLAY_AR_625TYPE_43       2
//#define VIDOBJLAY_AR_525TYPE_43       3
//#define VIDOBJLAY_AR_625TYPE_169      8
//#define VIDOBJLAY_AR_525TYPE_169      9
#define VIDOBJLAY_AR_EXTPAR				15


#define VIDOBJLAY_SHAPE_RECTANGULAR		0
#define VIDOBJLAY_SHAPE_BINARY			1
#define VIDOBJLAY_SHAPE_BINARY_ONLY		2
#define VIDOBJLAY_SHAPE_GRAYSCALE		3

#define VO_START_CODE	0x8
#define VOL_START_CODE	0x12
#define VOP_START_CODE0	0x0
#define VOP_START_CODE1	0x1b6
#define VIDO_SHORT_HEADER 0x20
#define VIDO_SHORT_HEADER_END 0x3f

//#define READ_MARKER()	BitstreamSkip(bs, 1)
#define WRITE_MARKER()	BitstreamPutBit(ptMP4, 1)

// vop coding types 
// intra, prediction, backward, sprite, not_coded
#define I_VOP	0
#define P_VOP	1
#define B_VOP	2
#define S_VOP	3
#define N_VOP	4

// resync-specific
#define NUMBITS_VP_RESYNC_MARKER  17
#define RESYNC_MARKER 1

static const char stuffing_codes[8] =
{
	        /* nbits     stuffing code */
	0,		/* 1          0 */
	1,		/* 2          01 */
	3,		/* 3          011 */
	7,		/* 4          0111 */
	0xf,	/* 5          01111 */
	0x1f,	/* 6          011111 */
	0x3f,   /* 7          0111111 */
	0x7f,	/* 8          01111111 */
};

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

void BitstreamWriteVolHeader(const Encoder * pEnc,
			 const FRAMEINFO * frame);

void BitstreamWriteVopHeader(const Encoder * pEnc,
			 const FRAMEINFO * frame,
			 int vop_coded);
			 
void BitstreamWriteShortHeader(Encoder * pEnc,
						const FRAMEINFO * frame,
						int vop_coded);

/*****************************************************************************
 * Inlined functions
 ****************************************************************************/
#ifdef TWO_P_INT
static uint32_t
#else
static __inline uint32_t 
#endif
log2bin(uint32_t value)
{
/* Changed by Chenm001 */
	int n = 0;

	while (value) {
		value >>= 1;
		n++;
	}
	return n;
}
 
/* write n bits to bitstream */

#ifdef TWO_P_INT
static void
#else
static __inline void 
#endif
BitstreamPutBits(volatile MP4_t *ptMP4, const uint32_t value,
				 const uint32_t size)
{
	//mFa526DrainWrBuf();
	ptMP4->BITDATA = value;
	ptMP4->BITLEN = size;
}


/* pad bitstream to the next byte boundary */
#ifdef TWO_P_INT
static void
#else
static __inline void 
#endif
BitstreamPad(volatile MP4_t *ptMP4)
{
	int bits;

	mFa526DrainWrBuf();
	bits = ptMP4->VADR;
	bits = 8 - (bits & 0x7);			//		bits % 8

	if (bits < 8) {
		ptMP4->BITDATA =  stuffing_codes[bits - 1];
		ptMP4->BITLEN = bits;
	}
}


#ifdef TWO_P_INT
static void
#else
static __inline void 
#endif
BitstreamPadAlways(volatile MP4_t * ptMP4)
{
	int bits;
	uint32_t code;

	mFa526DrainWrBuf();
	bits = ptMP4->VADR;
	code =  0x7f >> (bits & 0x7);		
	bits = 8 - (bits & 0x7);			//		bits % 8ú
	ptMP4->BITDATA = code;
	ptMP4->BITDATA = code; 

	// it looks the stuff code did not insert into the bitstream
	// but original Roger's code without the following line can still work...mmm...
//	SET_BADR(code)	
	ptMP4->BITLEN = bits;

//	BitstreamPutBits(bs, stuffing_codes[bits - 1], bits);
}


/* write single bit to bitstream */


#ifdef TWO_P_INT
static void
#else
static __inline void 
#endif
BitstreamPutBit(volatile MP4_t * ptMP4, const uint32_t bit)
{
	//mFa526DrainWrBuf();
	ptMP4->BITDATA = bit;
	ptMP4->BITLEN = 1;
}

#endif /* _BITSTREAM_H_ */
