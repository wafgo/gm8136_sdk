/**
 *  @file jpeg_enc.h
 *  @brief The header file for Gm JPEG Encoder API.
 *
 */
#ifndef _JPEG_ENC_H_
#define _JPEG_ENC_H_

#include "jenclib.h"
#include "ioctl_je.h"

#define IMAGE_COMP 3


typedef struct {
	FJPEG_ENC_PARAM pub;

	unsigned int  *pu32BaseAddr;    /**< User can use this variable to set the base
    	                               *   address of hardware core.
        	                           */                                     

	/// The function pointer to user-defined DMA memory allocation function.
	DMA_MALLOC_PTR  pfnDmaMalloc;  /**< This variable contains the function pointer to the user-defined 
                                 *   DMA malloc function since under OS environment, our hardware device
                                 *   may need the physical address instead of virtual address.
                                 *
                                 *   @see pfnDmaFree
                                 *   @see DMA_MALLOC_PTR
                                 *   @see DMA_FREE_PTR
                                 */
	/// The function pointer to user-defined DMA free allocation function.
	DMA_FREE_PTR   pfnDmaFree;    /**< This variable contains the function pointer to the user-defined 
                                 *   DMA free function since under OS environment, our hardware device
                                 *   may need the physical address instead of virtual address.
                                 *
                                 *   @see pfnDmaFree
                                 *   @see DMA_MALLOC_PTR
                                 *   @see DMA_FREE_PTR
                                 */
	MALLOC_PTR 	pfnMalloc;
	FREE_PTR  	pfnFree;								 
    unsigned char u32CacheAlign;     
								 

   unsigned char* bs_buffer_virt;			/* bitstream virtual address */
   unsigned char * bs_buffer_phy;			/* bitstream physical address */
} FJPEG_ENC_PARAM_MY;

#ifndef JPEGENC_EXPOPRT
  #ifdef __cplusplus
	  #define JPEGENC_EXPOPRT extern "C"
	#else
	  #define JPEGENC_EXPOPRT extern
	#endif
#endif


/*****************************************************************************
 * JPEG Encoder entry point
 ****************************************************************************/
JPEGENC_EXPOPRT void * FJpegEncCreate(FJPEG_ENC_PARAM_MY * my);
JPEGENC_EXPOPRT int FJpegEncCreate_fake(FJPEG_ENC_PARAM_MY * my, void * pthandler);
JPEGENC_EXPOPRT unsigned int FJpegEncEncode(void *enc_handle, FJPEG_ENC_FRAME * ptFrame);
JPEGENC_EXPOPRT void FJpegEncDestroy(void *enc_handle);
JPEGENC_EXPOPRT unsigned int FJpegEncSetQTbls(void *enc_handle, unsigned int *luma, unsigned int * chroma);
JPEGENC_EXPOPRT unsigned int FJpegEncGetQTbls(void *enc_handle, FJPEG_ENC_QTBLS *qtbl);

#endif
