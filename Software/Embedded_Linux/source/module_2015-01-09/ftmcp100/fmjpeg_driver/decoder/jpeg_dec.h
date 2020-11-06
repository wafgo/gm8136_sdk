/**
 *  @file jpeg_dec.h
 *  @brief The header file for Gm JPEG Decoder API.
 *
 */
#ifndef _JPEG_DEC_H_
#define _JPEG_DEC_H_

#include "jdeclib.h"
#include "ioctl_jd.h"
/// The Gm JPEG Decoder Parameters Structure.
typedef struct {
    FJPEG_DEC_PARAM pub;
	// The virtual base address of hardware core.
    unsigned int  *pu32BaseAddr;
    unsigned char * bs_buffer_virt;		// bitstream virtual base address
    unsigned int bs_buffer_phy;			// bitstream physical base address
    unsigned int bs_buffer_len;				// bitstream buffer length
    MALLOC_PTR 	pfnMalloc;
    FREE_PTR  	         pfnFree;								 
    unsigned char        u32CacheAlign;
} FJPEG_DEC_PARAM_MY;


/*****************************************************************************
 * JPEG Decoder entry point
 ****************************************************************************/
 
/// Gm JPEG Decoder API Functions Reference.
/**
 * 
 * \defgroup jpeg_decoder_ops_grp Gm JPEG Decoder API Reference
 *
 * The following section describes all the operations Gm JPEG decoder can perform.
 *
 * @{
 */

/// To create an JPEG decoder object and return handle to user.
/**  
 * And the returned handle have to be used in the rest of JPEG decoder operations.
 *
 * @param ptParam is the structure containing necessary decoder object parameters
 * @return return an JPEG decoder handle (void pointer)
 *
 * @see FJPEG_DEC_PARAM
 * @see FJpegDecDecode
 * @see FJpegDecDestroy
 *
 */
extern void * FJpegDecCreate(FJPEG_DEC_PARAM_MY * ptParam);
extern void FJpegDecParaA2(void *, FJPEG_DEC_PARAM_A2* ptPA2);
extern void FJpegDecReCreate(FJPEG_DEC_PARAM_MY * ptParam, void *);
/// To read header of JPEG bitstream and obtain related decoding information.
/**  
 * Obtain the informations wby reading the header of JPEG bitstream.
 *
 * @param dec_handle is the handle of JPEG decoder object.
 * @param pDecResult contains the header informations about decoded image.
 * @return return void. If there is error during reading the JPEG header, the program
 *                      will simply print error message to stderr and exit the program.
 *
 * @see FJPEG_DEC_PARAM
 * @see FJpegDecDecode
 * @see FJpegDecDestroy
 *
 */
extern int FJpegDecReadHeader(void *dec_handle, FJPEG_DEC_FRAME * ptFrame,  unsigned int pass);

/// To begin to decode JPEG bitstream.
/**
 * After calling FJpegDecReadHeader() function , user can start to decode JPEG bitstream by 
 * calling this function. And pass the allocated YUV buffer image pointer through 
 * 'FJPEG_DEC_PARAM' structure.
 *
 * @param dec_handle is the handle of JPEG decoder object.
 * @param ptParam is the structure containing necessary decoder object parameters such as
 *        the allocated YUV buffer space pointer and the number of components.
 * @return return void. If there is error during decoding, the program
 *                      will simply print error message to stderr and exit the program.
 *
 * @see FJPEG_DEC_PARAM
 * @see FJpegDecCreate
 * @see FJpegDecDestroy
 *
 */
extern int FJpegDecDecode(void *dec_handle,FJPEG_DEC_FRAME *ptFrame);


/// To destroy a JPEG decoder object
/**
 * After decoding, we need to release the JPEG decoder object.
 *
 * @param dec_handle is the handle of JPEG decoder object.
 * @return return void.
 * @see FJpegDecCreate
 *
 */
extern void FJpegDecDestroy(void *dec_handle);
extern void FJpegDecSetDisp(void *dec_handle, FJPEG_DEC_DISPLAY * ptDisp);
extern void FJpegDec_fill_bs(void *dec_handle, void * src_init, int bs_sz_init);
extern void FJpegDec_assign_bs(void *dec_handle, char * bs_va, char * bs_pa, int bs_cur_sz);

extern int FJpegDD_Tri(void *dec_handle, FJPEG_DEC_FRAME *ptFrame);
extern int FJpegDD_End(void *dec_handle);
extern void FJpegDD_GetInfo(void *dec_handle, FJPEG_DEC_FRAME *ptFrame);
extern void FJpegDD_show(void *dec_handle);
#ifdef SUPPORT_VG_422T
extern int FJpegD_420to422(void *dec_handle, unsigned int in_y, unsigned int in_u, unsigned int in_v, unsigned int out_yuv );
#endif
#ifdef SUPPORT_VG_422P
extern int FJpegD_422Packed(void * dec_handle,unsigned int in_y,unsigned int in_u,unsigned int in_v,unsigned int out_yuv);
#endif


/**
 * @}
 */
#endif
