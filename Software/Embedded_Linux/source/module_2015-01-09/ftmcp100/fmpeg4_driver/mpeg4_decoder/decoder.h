#ifndef _DECODER_H_
#define _DECODER_H_

#ifdef LINUX
#include "../common/mp4.h"
#include "../common/portab.h"
#else
#include "mp4.h"
#include "portab.h"
#endif
#include "Mp4Vdec.h"
#include "global_d.h"
#include "image_dd.h"

/*****************************************************************************
 * Structures
 ****************************************************************************/

	#define DECODER_COMMON \
		/* internal Used *******************************************/\
		int bMp4_quant;\
		int bH263;				/* short header mode*/\
		int data_partitioned;\
		uint8_t * output_base_phy;\
		uint8_t * output_base_ref_phy;\
		uint8_t   output_fmt;\
		uint32_t output_stride;\
		uint32_t output_height;\
		uint8_t * output_base_u_phy;\
		uint8_t * output_base_v_phy;\
		uint8_t * output_base_u_ref_phy;\
		uint8_t * output_base_v_ref_phy;\
		uint16_t *pu16ACDC_ptr_phy;\
	    uint8_t *pu8DeBlock_phy;\
        uint32_t width;\
		uint32_t height;\
		IMAGE cur;\
		IMAGE refn;\
		uint32_t mb_width;\
		uint32_t mb_height;\
		MACROBLOCK *mbs;\
		uint32_t crop_x;\
		uint32_t crop_y;\
		uint32_t crop_w;\
		uint32_t crop_h;\


typedef struct
{
	// adjust element location for FTMCP100E
	// internal Used *******************************************
	DECODER_COMMON

	// external Used *******************************************
	uint32_t u32CacheAlign;
	DMA_MALLOC_PTR_dec pfnDmaMalloc;
	DMA_FREE_PTR_dec pfnDmaFree;
	MALLOC_PTR_dec pfnMalloc;
	FREE_PTR_dec pfnFree;
	uint32_t u32MaxWidth;
	uint32_t u32MaxHeight;
	uint32_t u32BS_used_byte;
	uint32_t u32VirtualBaseAddr;
	uint8_t *pu8BS_start_virt;
	uint8_t *pu8BS_start_phy;
	uint8_t *pu8BS_ptr_virt;
	uint8_t *pu8BS_ptr_phy;
	uint32_t u32BS_buf_sz;
	uint32_t u32BS_buf_sz_remain;
	int bBS_end_of_data;
	uint16_t *pu16ACDC_ptr_virt;
#ifdef ENABLE_DEBLOCKING
	uint8_t *pu8DeBlock_virt;
	//uint8_t *pu8DeBlock_phy;
#endif
#if (defined(TWO_P_INT) || defined(TWO_P_EXT))
	uint32_t * mbs_virt;
#endif
	uint8_t custom_intra_matrix;
	uint8_t custom_inter_matrix;
	uint8_t u8intra_matrix[64];			// 64 elements
	uint8_t u8inter_matrix[64];			// 64 elements
	uint32_t u32intra_matrix_fix1[64];			// 64 elements
	uint32_t u32inter_matrix_fix1[64];			// 64 elements

	int reversible_vlc;
	uint16_t time_inc_bits;
	uint8_t quant_bits;
	uint8_t shape;
	uint8_t interlacing;
	int32_t frames;				// total frame number
	int8_t scalability;
	uint32_t time_increment_resolution;
	uint32_t vop_type;		// ben add for video graph, 2009 0914
	Bitstream bs;				// ben add for video graph, 2009 0914

	int64_t time;				// for record time
	int64_t time_base;
	int64_t last_time_base;
	int64_t last_non_b_time;
	uint32_t time_pp;
	uint32_t time_bp;
	
#ifdef ENABLE_DEINTERLACE
	uint32_t u32EnableDeinterlace;
#endif
}
DECODER_ext;

typedef struct
{
	// adjust element location for FTMCP100E
	// internal Used *******************************************
	DECODER_COMMON
}
DECODER_int;

int decoder_create(FMP4_DEC_PARAM_MY* ptParam, void ** pptDecHandle);
int decoder_create_fake(FMP4_DEC_PARAM_MY* ptParam, void * ptDecHandle);
void decoder_destroy(void * ptDecHandle);
int decoder_decode(void * ptDecHandle, FMP4_DEC_RESULT * ptResult);
extern int decoder_iframe(DECODER * dec, Bitstream * bs);
extern int decoder_pframe(DECODER * dec, Bitstream * bs, int rounding);
extern int decoder_nframe(DECODER * dec);


#endif
