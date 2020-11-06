/**
 *  @file mp4venc.h
 *  @brief The header file for Gm MPEG4 encoder API.
 *
 */
#ifndef _MP4VENC_H_
#define _MP4VENC_H_

//#include "../common/duplex.h"
#include "../common/portab.h"
#include "ioctl_mp4e.h"
#ifdef REF_POOL_MANAGEMENT
    #ifdef SHARED_POOL
        #include "shared_pool.h"
    #else
        #include "mem_pool.h"
    #endif
#endif

#define GM_ERR_FAIL   -1 /*< Operation failed.
			    *
 * The requested Gm operation failed. If this error code is returned from :
 * <ul>
 * <li>the Gm_init function : you must not try to use an Gm's instance from
 *                              this point of the code. Clean all instances you
 *                              already created and exit the program cleanly.
 * <li>Gm_encore or Gm_decore : something was wrong and en/decoding
 *                                  operation was not completed sucessfully.
 *                                  you can stop the en/decoding process or just 
 *									ignore and go on.
 * <li>Gm_stop : you can safely ignore it if you call this function at the
 *                 end of your program.
 * </ul>
 */

#define GM_ERR_OK      0 /*< Operation succeed.
			    *
 * The requested Gm operation succeed, you can continue to use Gm's
 * functions.
 */

#define GM_ERR_MEMORY  1 /*< Operation failed.
			    *
 * Insufficent memory was available on the host system.
 */

#define GM_ERR_FORMAT  2 /*< Operation failed.
			    *
 * The format of the parameters or input stream were incorrect.
 */

#define GM_ERR_NULL_FUNCTION 3	// Operation failed (malloc & free functions point to NULL).

/* @} */
/**
 *  By using typedef to create a type name for DMA memory allocation function.
 *  And user can allocate memory on a specified alignment memory 
 *  by using the parameter align_size.
 *
 *  @param size is the bytes to allocate.
 *  @param align_size is the alignment value which must be power of 2.
 *  @param reserved_size is the specifed cache line.
 *  @param phy_ptr is used to return the physical address of allocated aligned memory block.
 *  @return return a void pointer to virtual address of the allocated memory block.
 *  @see FMP4_ENC_PARAM
 *  @see DMA_FREE_PTR
 */
typedef void *(* DMA_MALLOC_PTR_enc)(unsigned int size,unsigned char align_size,unsigned char reserved_size, void ** phy_ptr);

/**
 *  By using typedef to create a type name for DMA memory free function.
 *  And user can use this type of function to free a block of memory that
 *  was allocated by the DMA_MALLOC_PTR type of function.
 *
 *  @param virt_ptr is a pointer to the memory block with virtual address that was returned
 *                  previously by the type of DMA_MALLOC_PTR function.
 *  @param phy_ptr is a pointer to the memory block with physical address that was returned
 *                 previously by the type of DMA_MALLOC_PTR function.
 *  @return return void.
 *  @see FMP4_ENC_PARAM
 *  @see DMA_MALLOC_PTR
 */
typedef void (* DMA_FREE_PTR_enc)(void * virt_ptr, void * phy_ptr);

/// The Gm MPEG4 Encoder Parameters Structure.
/**
 *  While creating encoder object by using FMpeg4EncCreate() operation, FMP4_ENC_PARAM 
 *  pointer is served as FMpeg4EncCreate()'s parameter to internally initialize related encoder
 *  object settings.\n
 *  See  @ref encoder_ops_grp \n
 *  @see FMpeg4EncCreate 
 *  
 */
typedef void *(* MALLOC_PTR_enc)(unsigned int size, unsigned char align_size,
							unsigned char reserved_size);
typedef void (* FREE_PTR_enc)(void * virt_ptr);

typedef struct {
	FMP4_ENC_PARAM_WRP pub;
    FMP4_ENC_PARAM_A2 encp_a2;  // Tuba 20110614_0: addr new parameters to set encoder
	/// The base address of hardware core.
	unsigned int u32VirtualBaseAddr;    /**< User can use this variable to set the base
                                    *   address of hardware core.
                                    */
	unsigned int va_base_wrp;		// wrapper virtual base address
	/// The CPU cache alignment
	unsigned int u32CacheAlign;	/**< User needs to specify the CPU cache line in ,<B>bytes</B>.\n
                                    *   ex: The cache line is 16 when the CPU is FA526
                                    */
	/// The address of current reconstruct frame buffer.
	unsigned char *pu8ReConFrameCur; /**< 
                                    *  To specify the current reconstruct frame buffer address.\n
                                    *  In some occasions,user may want to provide his own reconstructed 
                                    *  buffer.\n
                                    *  Hence, if this variable is not set to NULL, the user-provided current
                                    *  reconstruct frame buffer is used and pointed by this variable.\n
                                    *  Otherwise, if this variable is set to NULL, encoder will internally
                                    *  use the installed @ref pfnDmaMalloc function to allocate the necessary
                                    *  current reconstruct frame buffer.\n\n
                                    *
                                    *  <B>value of @ref pu8ReConFrameCur</B>:
                                    *     <ul>
                                    *       <li> != <B>NULL</B> : Use user-provided buffer as current reconstruct
                                    *                             frame buffer. The requirement of the buffer size in bytes
                                    *                             is '( ((frame width + 15)/16) * (2+((frame height + 15)/16)) + 2) x 256 x 1.5'
                                    *       <li> == <B>NULL</B> : Use internally current reconstruct frame buffer
                                    *                             which is allocated by using installed @ref pfnDmaMalloc
                                    *                             function.
                                    *     </ul>
                                    *   <B>N.B.</B> : the current reconstruct frame buffer address must be <B>physical address</B> with <B>8-byte aligned</B>.
                                    *                                    
                                    *   @see pfnDmaMalloc
                                    *   @see pfnDmaFree
                                    */
	/// The address of reference reconstruct frame buffer.
	unsigned char *pu8ReConFrameRef; /**< 
                                    *  To specify the reference reconstruct frame buffer address.\n
                                    *  In some occasions,user may want to provide his own reconstructed 
                                    *  buffer.\n
                                    *  Hence, if this variable is not set to NULL, the user-provided reference
                                    *  reconstruct frame buffer is used and pointed by this variable.\n
                                    *  Otherwise, if this variable is set to NULL, encoder will internally
                                    *  use the installed @ref pfnDmaMalloc function to allocate the necessary
                                    *  reference reconstruct frame buffer.\n\n
                                    *
                                    *  <B>value of @ref pu8ReConFrameRef</B>:
                                    *     <ul>
                                    *       <li> != <B>NULL</B> : Use user-provided buffer as reference reconstruct
                                    *                             frame buffer. The requirement of the buffer size in bytes
                                    *                             is '( ((frame width + 15)/16) * (2+((frame height + 15)/16)) + 2) x 256 x 1.5'
                                    *       <li> == <B>NULL</B> : Use internally reference reconstruct frame buffer
                                    *                             which is allocated by using installed @ref pfnDmaMalloc
                                    *                             function.
                                    *     </ul>
                                    *   <B>N.B.</B> : the reference reconstruct frame buffer address must be <B>physical address</B> with <B>8-byte aligned</B>.
                                    *                                    
                                    *   @see pfnDmaMalloc
                                    *   @see pfnDmaFree
                                    */
	/// The address of internal AC DC predictor buffer.
	unsigned short *p16ACDC; /**< 
                   *  To specify the internal AC DC perdictor buffer address for encoder.\n
                   *  If this variable is not set to NULL, the user-provided buffer
                   *  pointed by this variable is used.\n
                   *  Otherwise, if this variable is set to NULL, encoder will internally
                   *  use the installed @ref pfnDmaMalloc function to allocate the necessary
                   *  AC DC predictor buffer.\n\n
                   *
                   *  <B>value of @ref p16ACDC</B>:
                   *     <ul>
                   *       <li> != <B>NULL</B> : Use user-provided buffer as AC DC predictor buffer.
                   *                             The requirement of the buffer size in bytes
                   *                             is '((frame width + 15)/16) * 64'
                   *       <li> == <B>NULL</B> : Use internally AC DC predictor buffer
                   *                             which is allocated by using installed @ref pfnDmaMalloc
                   *                             function.
                   *     </ul>
                   *   <B>N.B.</B> : the AC DC predictor buffer address must be <B>physical address</B> with <B>16-byte aligned</B>.
                   *
                   *   @see pfnDmaMalloc
                   *   @see pfnDmaFree
                   */
  
	/// The function pointer to user-defined DMA memory allocation function.
  DMA_MALLOC_PTR_enc pfnDmaMalloc;  /**< This variable contains the function pointer to the user-defined 
                                 *   DMA malloc function since under OS environment, our hardware device
                                 *   may need the physical address instead of virtual address.
                                 *
                                 *   @see pfnDmaFree
                                 *   @see DMA_MALLOC_PTR
                                 *   @see DMA_FREE_PTR
                                 */
	/// The function pointer to user-defined DMA free allocation function.
  DMA_FREE_PTR_enc   pfnDmaFree;    /**< This variable contains the function pointer to the user-defined 
                                 *   DMA free function since under OS environment, our hardware device
                                 *   may need the physical address instead of virtual address.
                                 *
                                 *   @see pfnDmaFree
                                 *   @see DMA_MALLOC_PTR
                                 *   @see DMA_FREE_PTR
                                 */
  MALLOC_PTR_enc pfnMalloc;
  FREE_PTR_enc pfnFree;

  // for the sake of motion detection
  int motion_dection_enable;  //1:enable 0:disable
  /// The x-component of coordinate of motion detection region 0, upper-left point
  int 	range_mb_x0_LU; /**< The x-component of coordinate of motion detection region 0, upper-left point. */
  /// The y-component of coordinate of motion detection region 0, upper-left point.
  int 	range_mb_y0_LU; /**< The y-component of coordinate of motion detection region 0, upper-left point. */
  /// The x-component of coordinate of motion detection region 0, lower-right point.
  int 	range_mb_x0_RD; /**< The x-component of coordinate of motion detection region 0, lower-right point. */
  /// The y-component of coordinate of motion detection region 0, lower-right point.
  int 	range_mb_y0_RD; /**< The y-component of coordinate of motion detection region 0, lower-right point. */
  /// The x-component of coordinate of motion detection region 1, upper-left point.
  int 	range_mb_x1_LU; /**< The x-component of coordinate of motion detection region 1, upper-left point. */
  /// The y-component of coordinate of motion detection region 1, upper-left point.
  int 	range_mb_y1_LU; /**< The y-component of coordinate of motion detection region 1, upper-left point. */
  /// The x-component of coordinate of motion detection region 1, lower-right point.
  int 	range_mb_x1_RD; /**< The x-component of coordinate of motion detection region 1, lower-right point. */
  /// The y-component of coordinate of motion detection region 1, lower-right point.
  int 	range_mb_y1_RD; /**< The y-component of coordinate of motion detection region 1, lower-right point. */
  /// The x-component of coordinate of motion detection region 2, upper-left point.
  int 	range_mb_x2_LU; /**< The x-component of coordinate of motion detection region 2, upper-left point. */
  /// The y-component of coordinate of motion detection region 2, upper-left point.
  int 	range_mb_y2_LU; /**< The y-component of coordinate of motion detection region 2, upper-left point. */
  /// The x-component of coordinate of motion detection region 2, lower-right point.
  int 	range_mb_x2_RD; /**< The x-component of coordinate of motion detection region 2, lower-right point. */
  /// The y-component of coordinate of motion detection region 2, lower-right point
  int 	range_mb_y2_RD; /**< The y-component of coordinate of motion detection region 2, lower-right point. */
  /// The threshold of motion vector in region 0.
  int	MV_th0;         /**< This variable specifies the threshold of motion vector in region 0. */
  /// The threshold of sad in region 0.
  int	sad_th0;        /**< This variable specifies the threshold of sad in region 0. */
  /// The threshold of delta deviation in region 0.
  int	delta_dev_th0;  /**< This variable specifies the threshold of delta deviation in region 0. */
  /// The threshold of motion vector in region 1.
  int	MV_th1;         /**< This variable specifies the threshold of motion vector in region 1. */
  /// The threshold of sad in region 1.
  int	sad_th1;        /**< This variable specifies the threshold of sad in region 1. */
  /// The threshold of delta deviation in region 1.
  int	delta_dev_th1;  /**< This variable specifies the threshold of delta deviation in region 1. */
  /// The threshold of motion vector in region 2.
  int	MV_th2;         /**< This variable specifies the threshold of motion vector in region 2. */
  /// The threshold of sad in region 2.
  int	sad_th2;        /**< This variable specifies the threshold of sad in region 2. */
  /// The threshold of delta deviation in region 2.
  int	delta_dev_th2;  /**< This variable specifies the threshold of delta deviation in region 2. */
  /// The interval of frame/time for motion detection operation.
  int	md_interval;  /**< This variable specifies the interval of frame/time for motion detection operation. */
#ifdef MPEG4_COMMON_PRED_BUFFER // Tuba 20131108: use common pred buffer
    unsigned int mp4e_pred_virt;
    unsigned int mp4e_pred_phy;
#endif
#if defined(REF_POOL_MANAGEMENT)&!defined(REF_BUFFER_FLOW)  // Tuba 20140409: reduce number of reference buffer
    struct ref_buffer_info_t *recon_buf;
    struct ref_buffer_info_t *refer_buf;
#endif                                            
} FMP4_ENC_PARAM_MY;

/**
 *  Denote INTER mode with 1MV.
 */
#define INTER_MODE		0
/**
 *  Denote INTER mode with 4MV.
 */
#define INTER4V_MODE	2
/**
 *  Denote INTRA mode.
 */
#define	INTRA_MODE		3

/// The Encoder Macroblock Information Structure.
/**
 *  This data structure is mainly used for FMpeg4EncGetMBInfo() operation which
 *  contains various kind of macroblock information such as motion vectors,
 *  quantization value and macroblock mode.\n
 *  
 */

#ifndef MP4VENC_EXPOPRT
  #ifdef __cplusplus
	  #define MP4VENC_EXPOPRT extern "C"
	#else
	  #define MP4VENC_EXPOPRT extern
	#endif
#endif

/*****************************************************************************
 * Encoder entry point
 ****************************************************************************/
 
/// Gm MPEG4 Encoder API Functions Reference.
/**
 * 
 * \defgroup encoder_ops_grp Gm MPEG4 Encoder API Reference
 *
 * The following section describes all the operations Gm MPEG4 encoder can perform.
 *
 * @{
 */

/// To create an encoder object and return handle to user.
/**  
 * And the returned handle must be used in the rest of encoder operations.
 *
 * @param ptParam is the structure containing necessary encoding parameters
 * @return return an encoder handle (void pointer)
 * @see FMP4_ENC_PARAM
 * @see FMpeg4EncDestroy
 *
 */
MP4VENC_EXPOPRT void * FMpeg4EncCreate(FMP4_ENC_PARAM_MY * ptParam);
MP4VENC_EXPOPRT int FMpeg4EncReCreate(void * enc, FMP4_ENC_PARAM_MY * ptParam);
MP4VENC_EXPOPRT int FMpeg4Enc_IFP(void *enc_handle, FMP4_ENC_PARAM * ptParam);
/// To set YUV frame buffer address.
/**
 * This operation was used to set the frame buffer address for each Y frame, U frame,
 * and V frame respectively before calling FMpeg4EncOneFrame() function.
 * User must prepare the YUV frame data and store them in the specified Y,U and V address.\n
 * Encoder will fetch the YUV data from the specified frame buffer address to
 * perform encoding procedure.
 *
 * @return return void.
 * @see FMpeg4EncOneFrame
 *
 */

MP4VENC_EXPOPRT int FMpeg4EncOneFrame(void *enc_handle, FMP4_ENC_FRAME * xframe, int vol);
/// To get encoded bitstream length
/**
 * After one frame is encoded, user is able to obtain the encoded bitstream length of last encoded
 * frame by using this function.
 *
 * Note that :
 *   <ul>
 *     <li> The YUV data stored in the frame buffer for encoding must be arranged as <B>2D YUV format</B>
 *          for hardware's convenience.
 *     <li> The parameters yaddr, uaddr and vaddr must be an <B>8-byte aligned</B> <I><B>(64-bit alignment)</B></I> address.
 *     <li> The parameters yaddr, uaddr and vaddr provided by user must be physical address.
 *   </ul>
 * @param enc_handle is the structure containing necessary encoding parameters
 * @param yaddr is the address where the Y frame is stored. 
 *              The address must be pyhsical address and <B>8-byte aligned</B> <I><B>(64-bit alignment)</B></I>.
 * @param uaddr is the address where the U frame is stored.
 *              The address must be pyhsical address and <B>8-byte aligned</B> <I><B>(64-bit alignment)</B></I>.
 * @param vaddr is the address where the V frame is stored.
 *              The address must be pyhsical address and <B>8-byte aligned</B> <I><B>(64-bit alignment)</B></I>.
 * @param enc_handle is the handle of encoder object.
 * @param xframe see FMP4_ENC_FRAME.
 * @return return void.
 *
 */
MP4VENC_EXPOPRT MACROBLOCK_INFO *FMpeg4EncGetMBInfo(void *enc_handle);

//MP4VENC_EXPOPRT int FMpeg4EncGetDevFlag(void *enc_handle);
/// To destroy encoder object
/**
 * To destroy the encoder object.
 *
 * @param enc_handle is the handle of encoder object.
 * @return return 1 if the encoder object is released successfully.
 * @see FMpeg4EncCreate
 *
 */
MP4VENC_EXPOPRT void FMpeg4EncDestroy(void *enc_handle);
/**
 * @}
 */


MP4VENC_EXPOPRT int FMpeg4EncHandleIntraOver(void *enc_handle, int intra);
/**
* To get intra mb number
*/

#endif
