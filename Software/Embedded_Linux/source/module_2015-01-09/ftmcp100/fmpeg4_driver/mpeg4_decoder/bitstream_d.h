#ifndef _BITSTREAM_D_H_
#define _BITSTREAM_D_H_

#ifdef LINUX
#include "../common/portab.h"
#include "decoder.h"
#include "../common/mp4.h"
#else
#include "portab.h"
#include "decoder.h"
#endif

/* Quatization type */
#define H263_QUANT	0
#define MPEG4_QUANT	1



//extern FILE *temp_f;
/*****************************************************************************
 * Constants
 ****************************************************************************/

/* comment any #defs we dont use */

#define VIDOBJ_START_CODE		0x00000100	/* ..0x0000011f  */
#define VIDOBJLAY_START_CODE	0x00000120	/* ..0x0000012f */
#define VISOBJSEQ_START_CODE	0x000001b0
#define VISOBJSEQ_STOP_CODE		0x000001b1	/* ??? */
#define USERDATA_START_CODE		0x000001b2
#define GRPOFVOP_START_CODE		0x000001b3
/*#define VIDSESERR_ERROR_CODE  0x000001b4 */
#define VISOBJ_START_CODE		0x000001b5
/*#define SLICE_START_CODE      0x000001b7 */
/*#define EXT_START_CODE        0x000001b8 */


#define VISOBJ_TYPE_VIDEO				1
/*#define VISOBJ_TYPE_STILLTEXTURE      2 */
/*#define VISOBJ_TYPE_MESH              3 */
/*#define VISOBJ_TYPE_FBA               4 */
/*#define VISOBJ_TYPE_3DMESH            5 */


#define VIDOBJLAY_TYPE_SIMPLE			1
/*#define VIDOBJLAY_TYPE_SIMPLE_SCALABLE    2 */
#define VIDOBJLAY_TYPE_CORE				3
#define VIDOBJLAY_TYPE_MAIN				4


/*#define VIDOBJLAY_AR_SQUARE           1 */
/*#define VIDOBJLAY_AR_625TYPE_43       2 */
/*#define VIDOBJLAY_AR_525TYPE_43       3 */
/*#define VIDOBJLAY_AR_625TYPE_169      8 */
/*#define VIDOBJLAY_AR_525TYPE_169      9 */
#define VIDOBJLAY_AR_EXTPAR				15


#define VIDOBJLAY_SHAPE_RECTANGULAR		0
#define VIDOBJLAY_SHAPE_BINARY			1
#define VIDOBJLAY_SHAPE_BINARY_ONLY		2
#define VIDOBJLAY_SHAPE_GRAYSCALE		3

#define VO_START_CODE	0x8
#define VOL_START_CODE	0x12
#define VOP_START_CODE	0x1b6

#define READ_MARKER()	BitstreamSkip(bs, 1)
#define ZERO_BIT()		BitstreamSkip(bs, 1)
//#define WRITE_MARKER()	BitstreamPutBit(bs, 1)

/* vop coding types  */
/* intra, prediction, backward, sprite, not_coded */
#define I_VOP	0
#define P_VOP	1
#define B_VOP	2
#define S_VOP	3
#define N_VOP	4
#define I_VOP_D	5
#define P_VOP_D	6

//static int32_t num_gobs_in_vop[8] = {0, 6, 9, 18, 18, 18, 0, 0};
//static int32_t num_marcoblocks_in_gob[8] = {0, 8, 11, 22, 88, 352, 0, 0};

/* resync-specific */
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

int
read_video_packet_header(Bitstream *bs, DECODER *dec, const int addbits, 
			int *quant, int *fcode_forward,	int *fcode_backward);

bool
bRead_video_packet_header(DECODER * dec,
//						int *fcode_forward,
//						int *fcode_backward,
						int32_t * mbnum);

/* header stuff */
int BitstreamReadHeaders(Bitstream * bs,
						 DECODER_ext * dec,
						 uint32_t * rounding,
						 uint32_t * quant,
						 uint32_t * fcode_forward,
						 uint32_t * fcode_backward,
						 uint32_t * intra_dc_threshold_bit);



/*****************************************************************************
 * Inlined functions
 ****************************************************************************/
#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
/* initialise bitstream structure */
static void __inline
BitstreamForward(Bitstream * const bs,
				 const uint32_t bits)
{
	bs->pos += bits;

	if (bs->pos >= 32) {
		uint32_t b = bs->buf;

		*bs->tail++ = b;
		bs->buf = 0;
		bs->pos -= 32;
	}
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
static void __inline
BitstreamPutBits(Bitstream * const bs,
				 const uint32_t value,
				 const uint32_t size)
{
	uint32_t shift = 32 - bs->pos - size;

	if (shift <= 32) {
		bs->buf |= value << shift;
		BitstreamForward(bs, size);
	} else {
		uint32_t remainder;

		shift = size - (32 - bs->pos);
		bs->buf |= value >> shift;
		BitstreamForward(bs, size - shift);
		remainder = shift;

		shift = 32 - shift;

		bs->buf |= value << shift;
		BitstreamForward(bs, remainder);
	}
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )


union VECTORx
{
	uint32_t u32num;
	struct
	{
		uint8_t s80;
		uint8_t s81;
		uint8_t s82;
		uint8_t s83;
	} vec;
};

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) || defined(TWO_P_INT) )
// assume CPU is fa526, access by litlle endian
#ifdef TWO_P_INT
static unsigned long
#else
static unsigned long __inline
#endif
get_word(uint32_t *ptr)
{
#ifdef BS_IS_BIG_ENDIAN
	union VECTORx *ptrx = (union VECTORx *)ptr;
	return (ptrx->vec.s83 << 0) |
		(ptrx->vec.s82 << 8) |
		(ptrx->vec.s81 << 16) |
		(ptrx->vec.s80 << 24);
#else	
	//return ptr[0] | (ptr[1]<<8) | (ptr[2]<<16) | (ptr[3]<<24);
	return *(uint32_t *)ptr;
#endif
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) || defined(TWO_P_INT) )
#ifdef TWO_P_INT
static void
#else
static void __inline
#endif
BitstreamInit(Bitstream * const bs,
			  void *const bitstream,
			  void * const bitstream_phy,
			  uint32_t length)
{
	uint32_t tmp;

	bs->start_pos = ((uint32_t) bitstream & 0x3) * 8;
	tmp = ((uint32_t) bitstream & 0xfffffffc);
	bs->start = bs->tail = (uint32_t *) tmp;
	tmp = *bs->start;

	bs->bufa = get_word(bs->start);

	bs->bufb = get_word((bs->start + 1));

	bs->buf = 0;
	tmp = ((uint32_t) bitstream_phy & 0xfffffffc);
	bs->start_phy = bs->tail_phy = (uint32_t *)tmp;
	bs->pos = bs->start_pos;
	bs->length = length;
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) || defined(TWO_P_INT) )

//#ifdef TWO_P_INT
//static void
//#else
static void __inline
//#endif
BitstreamUpdatePos(Bitstream * const bs, uint32_t *tail, unsigned int pos)
{
	//if (tail != NULL) {
	if (tail != 0) {
		bs->tail = tail;

		bs->bufa = get_word(bs->tail);
		bs->bufb = get_word(bs->start + 1);
//		bs->bufa = *bs->tail;
//		bs->bufb = *(bs->tail + 1);
		bs->pos = pos;
	}
	bs->tail_phy = bs->start_phy + (bs->tail - bs->start);
}

#ifdef TWO_P_INT
static void
#else
static void __inline
#endif
BitstreamUpdatePos_phy(Bitstream * const bs, uint32_t *tail_phy, unsigned int pos)
{
	//if (tail_phy != NULL) {
	if (tail_phy != 0) {
		bs->tail_phy = tail_phy;
		bs->pos = pos;
	}
	bs->tail = bs->start + (bs->tail_phy - bs->start_phy);
	bs->bufa = get_word(bs->tail);
	bs->bufb = get_word(bs->start + 1);
}

#ifdef TWO_P_INT
static uint32_t
#else
static uint32_t __inline
#endif
BitstreamPos(const Bitstream * const bs)
{
	return((uint32_t)(8*((ptr_t)bs->tail - (ptr_t)bs->start) + bs->pos - bs->start_pos));
}


#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
/* reads n bits from bitstream without changing the stream pos */
static uint32_t __inline
BitstreamShowBits(Bitstream * const bs,
				  const uint32_t bits)
{
	int nbit = (bits + bs->pos) - 32;

	if (nbit > 0) {
		return ((bs->bufa & (0xffffffff >> bs->pos)) << nbit) | (bs->bufb >> (32 - nbit));
	} else {
		return (bs->bufa & (0xffffffff >> bs->pos)) >> (32 - bs->pos - bits);
	}
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

/* skip n bits forward in bitstream */
#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
static void __inline
BitstreamSkip(Bitstream * const bs,
			  const uint32_t bits)
{
	bs->pos += bits;

	if (bs->pos >= 32) {
//		uint32_t tmp;

		bs->bufa = bs->bufb;
//		tmp = *((uint32_t *) bs->tail + 2);
//		bs->bufb = tmp;
		bs->bufb = get_word(bs->tail + 2);
		bs->tail++;
		bs->pos -= 32;
	}
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
/* number of bits to next byte alignment */
static uint32_t __inline
BitstreamNumBitsToByteAlign(Bitstream *bs)
{
	uint32_t n = (32 - bs->pos) % 8;
	return n == 0 ? 8 : n;
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
/* number of bits to next byte alignment */
static uint32_t __inline
BitstreamNumBitsToByteAlign_a(Bitstream *bs)
{
	return (32 - bs->pos) % 8;
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
/* show nbits from next byte alignment */
static uint32_t __inline
BitstreamShowBitsFromByteAlign(Bitstream *bs, int bits)
{
	int bspos = bs->pos + BitstreamNumBitsToByteAlign(bs);
	int nbit = (bits + bspos) - 32;

	if (bspos >= 32) {
		return bs->bufb >> (32 - nbit);
	} else	if (nbit > 0) {
		return ((bs->bufa & (0xffffffff >> bspos)) << nbit) | (bs->bufb >> (32 - nbit));
	} else {
		return (bs->bufa & (0xffffffff >> bspos)) >> (32 - bspos - bits);
	}

}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
/* move forward to the next byte boundary */
static void __inline
BitstreamByteAlign(Bitstream * const bs)
{
	uint32_t remainder = bs->pos % 8;

	if (remainder) {
		BitstreamSkip(bs, 8 - remainder);
	}
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
static uint32_t __inline
BitstreamByteLength(const Bitstream * const bs)
{
	return((uint32_t) ( (ptr_t)bs->tail - (ptr_t)bs->start ) );
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
static uint32_t __inline
BitstreamLength(const Bitstream * const bs)
{
	return((uint32_t)bs->pos - bs->start_pos);
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

/* read n bits from bitstream */
#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
static uint32_t __inline
BitstreamGetBits(Bitstream * const bs,
				 const uint32_t n)
{
	uint32_t ret = BitstreamShowBits(bs, n);

	BitstreamSkip(bs, n);
	return ret;
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
/* read single bit from bitstream */
static uint32_t __inline
BitstreamGetBit(Bitstream * const bs)
{
	return BitstreamGetBits(bs, 1);
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

/* write n bits to bitstream */
#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
static uint32_t __inline
BitstreamShowBits_R(Bitstream * const bs,
				  const uint32_t bits)
{
	int nbit = bs->pos - bits;
	uint32_t a, b;

	if (nbit >= 0) {
		return (bs->bufa & (0xffffffff >> nbit)) >> (32-bs->pos);
	} else {
		if (bs->pos == 0) {
			a = 0;
			b = (bs->bufb & (0xffffffff >> (32 + nbit)));
		} else {
			a = ((bs->bufa  >> (32-bs->pos)));
			b = (bs->bufb & (0xffffffff >> (32 + nbit))) << (bs->pos);
		}
		return a | b;
	}
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
static void __inline
BitstreamSkip_R(Bitstream * const bs,
			  const uint32_t bits)
{
	bs->pos -= bits;

	if (bs->pos < 0) {
		uint32_t tmp;

		bs->bufa = bs->bufb;
		tmp = *((uint32_t *) bs->tail - 1);
		bs->bufb = tmp;
		bs->tail--;
		bs->pos += 32;
	}
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
static uint32_t __inline
BitstreamGetBits_R(Bitstream * const bs,
				 const uint32_t n)
{
	uint32_t ret = BitstreamShowBits_R(bs, n);

	BitstreamSkip_R(bs, n);
	return ret;
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )



#endif /* _BITSTREAM_D_H_ */

