#ifndef _MP4VDEC_H_
	#define _MP4VDEC_H_

//	#include "../common/duplex.h"
	#include "../common/portab.h"
	#include "ioctl_mp4d.h"
#ifdef REF_BUFFER_FLOW
    #include "../mem_pool.h"
#endif

	// avaiable format, start
	#define OUTPUT_FMT_CbYCrY	0
	#define OUTPUT_FMT_RGB555	1
	#define OUTPUT_FMT_RGB888	2
	#define OUTPUT_FMT_RGB565	3
	#define OUTPUT_FMT_YUV		4
	// avaiable format, end


	/* API return values */
	typedef enum {
		GM_ERR_BITSTREAM = -2,		 	// bitstream failed.
		GM_ERR_FAIL2 = -1,		 	// Operation failed.
		GM_ERR_OK2 = 0, 				// Operation succeed.
		GM_ERR_MEMORY2 = 1,		// Operation failed (out of memory).
		GM_ERR_API2 = 2,			// Operation failed (API version error).
		GM_ERR_NULL_FUNCTION2 = 3	// Operation failed (malloc & free functions point to NULL).
	} MP4_RET;

	typedef void *(* DMA_MALLOC_PTR_dec)(uint32_t size, uint8_t align_size,
							uint8_t reserved_size, void ** phy_ptr);
	typedef void (* DMA_FREE_PTR_dec)(void * virt_ptr, void * phy_ptr);

	typedef void *(* MALLOC_PTR_dec)(uint32_t size, uint8_t align_size,
							uint8_t reserved_size);
	typedef void (* FREE_PTR_dec)(void * virt_ptr);

/*****************************************************************************
 * Decoder structures
 ****************************************************************************/
	typedef struct
	{
		FMP4_DEC_PARAM pub;
		uint32_t u32VirtualBaseAddr;
		uint32_t u32BSBufSize;
		uint32_t u32CacheAlign;
		uint8_t * pu8ReConFrameCur_phy;	// physical address. buffer for current reconstruct frame.
		uint8_t * pu8ReConFrameCur;		// virtual address.
				// Don't care when pfnDmaMalloc != NULL
				// 8 Byte boundary
				// byte size = ((width + 15)/16) x ((height + 15)/16) x 256 x 1.5
		uint8_t * pu8ReConFrameRef_phy;	// physical address. buffer for reference reconstruct frame.
		uint8_t * pu8ReConFrameRef;		// virtual address.
				// Don't care when pfnDmaMalloc != NULL
				// 8 Byte boundary
				// byte size = ((width + 15)/16) x ((height + 15)/16) x 256 x 1.5
		uint16_t * pu16ACDC_phy;			// physical address. buffer for ACDC predictor
		uint16_t * pu16ACDC;				// virtual address.
   				// Don't care when pfnDmaMalloc != NULL
				// 8 Byte boundary
				// byte size = ((width + 15)/16) x 64
		uint8_t * pu8DeBlock_phy;			// physical address. buffer for temp deblocking buffer
		uint8_t * pu8DeBlock;				// virtual address.
	  //#endif
				// Don't care when pfnDmaMalloc != NULL
				// 8 Byte boundary
				// byte size = ((width + 15)/16) x 64
		uint8_t * pu8BitStream_phy;			// physical address. buffer for bitstream
		uint8_t * pu8BitStream;				// virtual address.
				// Don't care when pfnDmaMalloc != NULL
				// byte size = u32BSBufSize
		#if (defined(TWO_P_INT) || defined(TWO_P_EXT))
		uint32_t * pu32Mbs_phy;			// physical address. buffer for Macroblocks
		uint32_t * pu32Mbs;				// virtual address.
				// Don't care when pfnDmaMalloc != NULL
				// 8 Byte boundary
				// byte size = ((width + 15)/16) x 64
		#endif
		DMA_MALLOC_PTR_dec pfnDmaMalloc;	// "set to NULL", means the 4 above buffers will be prepared enternallly
		DMA_FREE_PTR_dec pfnDmaFree;			// Don't care when pfnDmaMalloc == NULL
		MALLOC_PTR_dec pfnMalloc;
		FREE_PTR_dec pfnFree;
	} FMP4_DEC_PARAM_MY;

	typedef struct
	{
		uint32_t u32VopWidth;
		uint32_t u32VopHeight;
		uint32_t u32UsedBytes;
		uint8_t * pu8FrameBaseAddr_phy;		// output frame buffer
		uint8_t * pu8FrameBaseAddr_U_phy;	// frame buffer (U) if output format is yuv420
		uint8_t * pu8FrameBaseAddr_V_phy;	// frame buffer (V) if output format is yuv420
    #ifdef REF_BUFFER_FLOW
        struct ref_buffer_info_t *pReconBuffer;
    #endif
	} FMP4_DEC_RESULT;


	#ifdef MP4VDEC_GLOBALS
		#define MP4VDEC_EXT
	#else
		#define MP4VDEC_EXT extern
	#endif

	MP4VDEC_EXT int
	Mp4VDec_Init(FMP4_DEC_PARAM_MY * ptParam, void ** pptDecHandle);

	MP4VDEC_EXT 
	int Mp4VDec_ReInit(FMP4_DEC_PARAM_MY * ptParam, void * ptDecHandle);

	MP4VDEC_EXT uint32_t
	Mp4VDec_QueryEmptyBuffer (void * ptDecHandle);

	MP4VDEC_EXT uint32_t
	Mp4VDec_QueryFilledBuffer (void * ptDecHandle);

	MP4VDEC_EXT void
	Mp4VDec_InvalidBS (void * ptDecHandle);

	MP4VDEC_EXT MP4_RET
	Mp4VDec_FillBuffer(void * ptDecHandle, uint8_t * ptBuf, uint32_t u32BufSize);

	MP4VDEC_EXT void
	Mp4VDec_EndOfData (void * ptDecHandle);

	MP4VDEC_EXT void
	Mp4VDec_SetOutputAddr (void * ptDecHandle,
		uint8_t * pu8output_phy, uint8_t * pu8output_u_phy, uint8_t * pu8output_v_phy);

	MP4VDEC_EXT void
	Mp4VDec_SetCrop (void * ptDecHandle, unsigned int x, unsigned int y, unsigned int w, unsigned int h);

	MP4VDEC_EXT int
	Mp4VDec_OneFrame(void * ptDecHandle, FMP4_DEC_RESULT * ptResult);

	MP4VDEC_EXT void
	Mp4VDec_Release(void * ptDecHandle);

	MP4VDEC_EXT void Mp4VDec_AssignBS(void * ptDecHandle, uint8_t * ptBuf_virt, uint8_t * BS_phy, uint32_t u32BufSize);

#ifdef __cplusplus
}
#endif

#endif
