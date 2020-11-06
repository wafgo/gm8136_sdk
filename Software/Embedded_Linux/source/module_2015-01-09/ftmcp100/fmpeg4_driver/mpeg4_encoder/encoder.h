
#ifndef _ENCODER_H_
#define _ENCODER_H_

#ifdef LINUX
#include "../common/portab.h"
#include "ftmcp100.h"
#include "global_e.h"
#include "../common/image.h"
#include "Mp4Venc.h"
#else
#include "portab.h"
#include "ftmcp100.h"
#include "global_e.h"
#include "image.h"
//#include "ratecontrol.h"
#include "Mp4Venc.h"
#endif

//#define _DEBUG_PSNR
/*****************************************************************************
 * Constants
 ****************************************************************************/

/* Quatization type */
#define H263_QUANT	0
#define MPEG4_QUANT	1

/* Indicates no quantizer changes in INTRA_Q/INTER_Q modes */
#define NO_CHANGE 64

// fcode: ftmcp100 always 1
#define FCODE 1
// bcode: no support BVOP

#define stride_MB			384

/*****************************************************************************
 * Types
 ****************************************************************************/
typedef struct
{
	int x;
	int y;
} VECTOR;

typedef enum
{
	I_VOP = 0,
	P_VOP = 1,
	B_VOP = 2
}
VOP_TYPE;



typedef struct
{
	uint32_t quant;

	VOP_TYPE coding_type;
	uint32_t bRounding_type;

	uint32_t seconds;
	uint32_t ticks;

	IMAGE image;
	IMAGE reconstruct;

	MACROBLOCK_E *mbs;

         MACROBLOCK_E *mbs_virt; //leo
	int32_t activity0;
	int32_t activity1;
	int32_t activity2;
} FRAMEINFO;


typedef struct
{
	// images
	FRAMEINFO *current1;
	FRAMEINFO *reference;

	/// The base address of hardware core.
	uint32_t u32VirtualBaseAddr;
	/// the base address of MPEG4 registers
	volatile MP4_t * ptMP4;

	uint8_t custom_intra_matrix;
	uint8_t custom_inter_matrix;
	uint8_t u8intra_matrix[64];			// 64 elements
	uint8_t u8inter_matrix[64];			// 64 elements
	uint32_t u32intra_matrix_fix1[64];			// 64 elements
	uint32_t u32inter_matrix_fix1[64];			// 64 elements
	uint16_t *pred_value_phy;
	uint16_t *pred_value_virt;

	// enable short header feature
	int bH263;
	// enable 4 motion vector feature
	int bEnable_4mv;
	// enable Resync. Marker
	int bResyncMarker;
	int bMp4_quant;
	// temporal reference for decoder when short-header enabled
	uint8_t u8Temporal_ref;
	// vop width, height
	uint32_t width;
	uint32_t height;

	/* ROI data structure */
	// ROI's (region of interest) upper-left corner coordinate(x,y) of captured frame.
	int roi_x;				// encoded x-start
	int roi_y;				// encoded y-start
	int mb_offset;		// mb offset between "original top-left-corner" and  "encoded top-left-corner"
	int mb_stride;		// original mb width
	int roi_width;		// encoded width no matter roi_enable
	int roi_height;		// encoded height no matter roi_enable
	int roi_mb_width;		// encoded mb width no matter roi_enable
	int roi_mb_height;	// encoded mb width no matter roi_enable
	// vop width, height (unit: MacroBlock)
	int mb_width;		// equal to roi_mb_width if roi_enable
	int mb_height;		// equal to roi_mb_height if roi_enable

	// frame rate increment & base
	uint32_t fincr;
	uint32_t fbase;

	/* vars that not "quite" frame independant */
	/* rounding type; alternate 0-1 after each interframe */
	uint32_t bRounding_type;
	uint32_t m_seconds;
	uint32_t m_ticks;
	
/* motion dection data structure */
	//  store the md information of MBs, i.e., deviation, for N interval
	int *dev_buffer;
	int dev_flag;		// index the first frame
	int frame_counter;	// used for md N interval
	int motion_dection_enable;  //1:enable 0:disable
	int range_mb_x0_LU; /**< The x-component of coordinate of motion detection region 0, upper-left point. */
	int range_mb_y0_LU; /**< The y-component of coordinate of motion detection region 0, upper-left point. */
	int range_mb_x0_RD; /**< The x-component of coordinate of motion detection region 0, lower-right point. */
	int range_mb_y0_RD; /**< The y-component of coordinate of motion detection region 0, lower-right point. */
	int range_mb_x1_LU; /**< The x-component of coordinate of motion detection region 1, upper-left point. */
	int range_mb_y1_LU; /**< The y-component of coordinate of motion detection region 1, upper-left point. */
	int range_mb_x1_RD; /**< The x-component of coordinate of motion detection region 1, lower-right point. */
	int range_mb_y1_RD; /**< The y-component of coordinate of motion detection region 1, lower-right point. */
	int range_mb_x2_LU; /**< The x-component of coordinate of motion detection region 2, upper-left point. */
	int range_mb_y2_LU; /**< The y-component of coordinate of motion detection region 2, upper-left point. */
	int range_mb_x2_RD; /**< The x-component of coordinate of motion detection region 2, lower-right point. */
	int range_mb_y2_RD; /**< The y-component of coordinate of motion detection region 2, lower-right point. */
	int	MV_th0;         /**< This variable specifies the threshold of motion vector in region 0. */
	int	sad_th0;        /**< This variable specifies the threshold of sad in region 0. */
	int	delta_dev_th0;  /**< This variable specifies the threshold of delta deviation in region 0. */
	int	MV_th1;         /**< This variable specifies the threshold of motion vector in region 1. */
	int	sad_th1;        /**< This variable specifies the threshold of sad in region 1. */
	int	delta_dev_th1;  /**< This variable specifies the threshold of delta deviation in region 1. */
	int	MV_th2;         /**< This variable specifies the threshold of motion vector in region 2. */
	int	sad_th2;        /**< This variable specifies the threshold of sad in region 2. */
	int	delta_dev_th2;  /**< This variable specifies the threshold of delta deviation in region 2. */
	int	md_interval;  /**< This variable specifies the interval of frame/time for motion detection operation. */

    int module_time_base; //VOP header module_time_base=XXXX is the number

	int     ac_disable;
	void * bitstream;	// bitstream start phy. address
	unsigned int img_fmt; 	 // keep the source format MP4 2D or H.264 2D, or Raster scan
										// 0: 2D format, CbCr interleave, named H264_2D
										// 1: 2D format, CbCr non-interleave, named MP4_2D
										// 2: 1D format, YUV420 planar, named RASTER_SCAN_420
										// 3: 1D format, YUV420 packed, named RASTER_SCAN_422
	unsigned int	 u32BaseAddr_wrapper;
    unsigned int cropping_height;   // Tuba 20110614_0: add crop height to encode 1080p
    unsigned int cropping_width;    // Tuba 20140314: add crop width
    uint32_t intraMBNum;    // Tuba 20111209_0: output intra mb number in P frame
    uint32_t sucIntraOverNum;       // Tuba 20111209_1
    uint32_t disableModeDecision;   // Tuba 20111209_1
    uint32_t forceOverIntra;        // Tuba 20111209_1
    int forceSyntaxSameAs8120;      // Tuba 20120316
} Encoder;
  //---------------------------------------------------------------------------
typedef struct
{
	DMA_MALLOC_PTR_enc pfnDmaMalloc;	         // The function pointer to user-defined DMA memory allocation function.  
	DMA_FREE_PTR_enc pfnDmaFree;			// The function pointer to user-defined DMA free allocation function.
	MALLOC_PTR_enc pfnMalloc;				// The function pointer to user-defined memory allocation function.  
	FREE_PTR_enc pfnFree;					// The function pointer to user-defined free allocation function.
	uint32_t u32CacheAlign;
	/// The H.263 quantization method.		                            
	int32_t bH263Quant;
	// frame number from current i-frame
	int iFrameNum;
	// specify how many p-frames between i-frame
	int iMaxKeyInterval;
	// additional information for 
    //bool vol_header;
    int vol_header; // low byte for P frame , high byte for I frame
	Encoder Enc;
	// support max vop width, height
	uint32_t max_width;
	uint32_t max_height;

    //uint32_t intraMBNum;    // Tuba 20111209_0: output intra mb number in P frame
}
Encoder_x;

/*****************************************************************************
 * Inline functions
 ****************************************************************************/

/*****************************************************************************
 * Prototypes
 ****************************************************************************/
Encoder_x *encoder_create(FMP4_ENC_PARAM_MY* ptParam);
int encoder_create_fake(Encoder_x * pEnc_x, FMP4_ENC_PARAM_MY* ptParam);
int encoder_ifp(Encoder_x * pEnc_x, FMP4_ENC_PARAM *enc_p);
void encoder_destroy(Encoder_x * pEnc_x);
int encoder_encode(Encoder_x * pEnc_x,
				   FMP4_ENC_FRAME * pFrame, int force_vol);


#define SWAP(A,B)    { void * tmp = A; A = B; B = tmp; }


// mainly used for embedded CPU firmware
#define ENC_I_FRAME_PIPE	3
#define ENC_MBS_LOCAL_I	(ENC_I_FRAME_PIPE + 1)
#define ENC_P_FRAME_PIPE	5
#define ENC_MBS_LOCAL_P	(ENC_P_FRAME_PIPE + 1)

#define ENC_MBS_ACDC		(5)
#define ENC_MBS_LOCAL_REF_P	(ENC_P_FRAME_PIPE + 1)
extern void encoder_iframe(Encoder_x * pEnc_x);
extern void encoder_pframe(Encoder_x * pEnc_x);
extern void encoder_nframe(Encoder_x * pEnc_x);

#endif
