#ifdef LINUX
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/version.h>
#include "../common/define.h"
#include "encoder.h"
#include "global_e.h"
#include "image_e.h"
#include "motion.h"
#include "bitstream.h"
#include "mbfunctions.h"
#include "dma_e.h"
#include "me_e.h"
#include "../common/quant_matrix.h"
#include "../common/dma.h"
#include "../common/vpe_m.h"
#include "../common/mp4.h"
#else
#include "define.h"
//#include "encoder.h"
#include "global_e.h"
#include "image_e.h"
#include "motion.h"
#include "bitstream.h"
#include "mbfunctions.h"
#include "quant_matrix.h"
#include "dma.h"
#include "dma_e.h"
#include "me_e.h"
#include "vpe_m.h"
#endif
#ifdef TWO_P_EXT
#include "encoder_ext.h"
#else
#include "encoder.h"
#endif
#include "fmcp_type.h"

//#define PERFORMANCE_TEST


#ifdef EVALUATION_PERFORMANCE
#include "../common/performance.h"
static struct timeval tv_init, tv_curr;
extern FRAME_TIME encframe;
extern TOTAL_TIME enctotal;
extern uint64 time_delta(struct timeval *start, struct timeval *stop);
extern void enc_performance_count(void);
extern void enc_performance_report(void);
extern void enc_performance_reset(void);
extern unsigned int get_counter(void);
// ============== for debug ================
#elif defined(PERFORMANCE_TEST)
//#include "../common/performance.h"
//extern unsigned int get_counter(void);
unsigned int hw_start, hw_end;
unsigned int hw_total;
unsigned int frame_cnt;


unsigned int get_counter(void)
{
	struct timeval tv;
	do_gettimeofday(&tv);
  
	return tv.tv_sec * 1000000 + tv.tv_usec;
}
#endif

static void __inline image_null(IMAGE * image)
{
	image->y_phy = 
	image->u_phy = 
	image->v_phy = NULL;
	image->y = 
	image->u = 
	image->v = NULL;
}
static __inline void mpeg_enc_init(int32_t base)
{

    volatile MP4_t * ptMP4 = (MP4_t *)(base  + MP4_OFF);

    #ifdef TWO_P_EXT
		extern void encoder_em_init(uint32_t base);
	    // reset command for em
    	encoder_em_init (base);
	#endif
	
         // set ACDC Predictor Buffer Address
	ptMP4->ACDCPAR = PREDICTOR0_OFF;
	ptMP4->VADR = RUN_LEVEL_OFF;
	// to set the prediction MV buffer start address
	ptMP4->PMVADDR = PMV_BUFFER_OFF;
}

/*****************************************************************************
 * Encoder ROI
 * This function is used to regular the roi parameters
 *
 ******************************************************************************/
static int encoder_roi_init( Encoder *pEnc, FMP4_ENC_PARAM * ptParam)
{
	int temp;
	int allign_byte_h = PIXEL_Y;
	int allign_byte_v = PIXEL_Y;


	if (ptParam->bROIEnable) {
		if (ptParam->roi_x < 0)
			ptParam->roi_x = pEnc->roi_x;
		if (ptParam->roi_y < 0)
			ptParam->roi_y = pEnc->roi_y;
		if (ptParam->roi_w < 0)
			ptParam->roi_w = pEnc->roi_width;
		if (ptParam->roi_h < 0)
			ptParam->roi_h = pEnc->roi_height;

		if (0 == ptParam->roi_w)
			ptParam->roi_w = pEnc->width;
		if (0 == ptParam->roi_h)
			ptParam->roi_h = pEnc->height;
		switch (pEnc->img_fmt) {
			case 2: // RASTER_SCAN_420
				allign_byte_h = 8;
				allign_byte_v = 2;
				break;
			case 3: // RASTER_SCAN_422
				allign_byte_h = 2;
				allign_byte_v = 1;
				break;
			default: // H2642D, MPEG42D
				break;
		}

		// roi_x
		temp = ptParam->roi_x & (allign_byte_h - 1);
		if (temp) {
			printk("[mp4e]error: roi_x %d must align at %d\n", ptParam->roi_x, allign_byte_h);
			return -1;
		}
		// roi_y
		temp = ptParam->roi_y & (allign_byte_v - 1);
		if (temp) {
			printk("[mp4e]error: roi_y %d must align at %d\n", ptParam->roi_y, allign_byte_v);
			return -1;
		}
		// roi_w
		temp = ptParam->roi_w & (PIXEL_Y - 1);
		if (temp) {
			printk("[mp4e]error: roi_w %d must align at %d\n", ptParam->roi_w, PIXEL_Y);
			return -1;
		}
		// roi_h
		temp = ptParam->roi_h & (PIXEL_Y - 1);
		if (temp) {
			printk("[mp4e]error: roi_h %d must align at %d\n", ptParam->roi_h, PIXEL_Y);
			return -1;
		}


		if ((ptParam->roi_x + ptParam->roi_w) > pEnc->width) {
			printk ("[mp4e]error: roi_x (%d) + roi_w (%d) > image_width (%d)\n",
									ptParam->roi_x, ptParam->roi_w, pEnc->width);
			return -1;
		}
		if ((ptParam->roi_y + ptParam->roi_h) > pEnc->height) {
			printk ("[mp4e]error: roi_y (%d) + roi_h (%d) > image_height (%d)\n",
									ptParam->roi_y, ptParam->roi_h, pEnc->height);
			return -1;
		}
		if (ptParam->roi_w > (2048-16)) {
			printk ("[mp4e]error: max encoded width is 2032, but %d\n", ptParam->roi_w);
			return -1;
		}
		pEnc->roi_x = ptParam->roi_x;
		pEnc->roi_y = ptParam->roi_y;
		pEnc->roi_width = ptParam->roi_w;
		pEnc->roi_height = ptParam->roi_h;
	}
	else {
		pEnc->roi_x = 0;
		pEnc->roi_y = 0;
		pEnc->roi_width = pEnc->width;
		pEnc->roi_height = pEnc->height;
	}

	pEnc->roi_mb_width	= pEnc->roi_width /PIXEL_Y;
	pEnc->roi_mb_height = pEnc->roi_height /PIXEL_Y;
	pEnc->mb_stride	= pEnc->mb_width;
	// we override the mb_width and mb_height rihgt here for ROI's sake
	pEnc->mb_width = pEnc->roi_mb_width;
	pEnc->mb_height = pEnc->roi_mb_height;	
	pEnc->mb_offset = pEnc->mb_stride * (pEnc->roi_y>>4) + (pEnc->roi_x>>4);
	return 0;
}

int encoder_ifp(Encoder_x * pEnc_x, FMP4_ENC_PARAM *enc_p)
{
	pEnc_x->iMaxKeyInterval = enc_p->u32IPInterval;
	pEnc_x->iFrameNum = 0;
	return 0;
}

int encoder_create_fake(Encoder_x * pEnc_x, FMP4_ENC_PARAM_MY * ptParam)
{
	volatile MDMA *pmdma;
	Encoder *pEnc;
	int i;
	int fincr,fbase;
	int max_mb_w,max_mb_h ;
	int img_fmt;
	FMP4_ENC_PARAM *enc_p = &ptParam->pub.enc_p;

    // Tuba 20140409: fix bug reinit must reset parameter @ encoder_create_fake
	pEnc_x->max_width = ptParam->pub.enc_p.u32MaxWidth;
	pEnc_x->max_height = ptParam->pub.enc_p.u32MaxHeight;
	max_mb_w = (pEnc_x->max_width + 15) /16;
	if (max_mb_w > 2048/16 ) {
		printk ("MPEG4E Setting error: max frame width is 2048, but %d\n",	pEnc_x->max_width);
		return -1;
	}

	if ((ptParam->pfnMalloc == NULL) || (ptParam->pfnFree == NULL))  {
       	printk("NO Malloc Function!\n");
		return -1;
	}

	if (pEnc_x == NULL) {
       	printk("encoder_create_fake: null pointer\n");
		return -1;
	}
	pEnc = &pEnc_x->Enc;
	img_fmt = ptParam->pub.img_fmt;
	if (img_fmt == -1)	// no specify
		img_fmt = pEnc->img_fmt;
#ifdef DMAWRP
	if( (img_fmt < 0) || (img_fmt > 3))	// support MP4_2D, RASTER_SCAN_420, RASTER_SCAN_422
#else
	if( (img_fmt != 0) && (img_fmt != 1))		// support H264_2D/MP4_2D only
#endif
	{
		printk("NOT support this input image format %d\n", img_fmt);
		return -1;
	}
	pEnc->img_fmt = img_fmt;

	fincr = enc_p->fincr;
	fbase = enc_p->fbase;

	// not allow fbase stay at 2^n, due to
	// Quicktime will parsing error when framerate is 2, 4, 8, 16,...
 	for (i = 0; i < 16; i ++) {
		if (fbase == (1 << i)) {
			fincr *= 3;
			fbase *= 3;
			break;
		}
	}

	if (fbase > 65535) {
		int div = (int) fbase / 65535;
		fbase = (int) (fbase / div);
		fincr = (int) (fincr / div);
	}

	if (enc_p->bShortHeader) {
		int i;
		for (i = 0; i < SOURCE_FMT_SZ; i ++) {
			if ((enc_p->u32FrameWidth == vop_width[i]) &&
				(enc_p->u32FrameHeight == vop_height[i]))
				break;
		}
		if (i == SOURCE_FMT_SZ) {
			printk ("unsupport dimension in sh-mode\n");
			return -1;
		}
	}

	pEnc_x->pfnDmaMalloc = ptParam->pfnDmaMalloc;
	pEnc_x->pfnDmaFree = ptParam->pfnDmaFree;
	pEnc_x->pfnMalloc = ptParam->pfnMalloc;
	pEnc_x->pfnFree = ptParam->pfnFree;
	pEnc_x->u32CacheAlign = ptParam->u32CacheAlign;
	pEnc_x->iFrameNum = 0;
	pEnc_x->iMaxKeyInterval = enc_p->u32IPInterval;

	// **** Fill members of Encoder structure ****
#ifndef MPEG4_COMMON_PRED_BUFFER
	if ((pEnc_x->max_width < enc_p->u32MaxWidth) ||
	   (pEnc_x->max_height < enc_p->u32MaxHeight)) {
		printk ("MPEG4 encoder max w/h over the original one (new: %dx%d, org: %dx%d)\n",
			pEnc_x->max_width, pEnc_x->max_height,
			enc_p->u32MaxWidth, enc_p->u32MaxHeight);
		return -1;
	}
	if ((pEnc_x->max_width < enc_p->u32FrameWidth) ||
	   (pEnc_x->max_height < enc_p->u32FrameHeight)) {
		printk ("MPEG4 encoder w/h over the max w/h (max: %dx%d, set: %dx%d)\n",
			pEnc_x->max_width, pEnc_x->max_height,
			enc_p->u32FrameWidth, enc_p->u32FrameHeight);
		return -1;
	}
#endif
	pEnc->width = enc_p->u32FrameWidth;
	pEnc->height = enc_p->u32FrameHeight;
#ifndef ENABLE_VARIOUS_RESOLUTION
    if (ptParam->encp_a2.u32CropHeight >= 16) {
        printk("crop height(%d) should smaller than 16, now set to be zero\n", ptParam->encp_a2.u32CropHeight);
        pEnc->cropping_height = 0;
    }
    else
        pEnc->cropping_height = ptParam->encp_a2.u32CropHeight;  // TUba 20110614_0: add crop height to handle 1080p
#else
    if (enc_p->u32CropWidth >= 16) {
        printk("cropping width(%d) should smaller than 16, now set to be zero\n", enc_p->u32CropWidth);
        pEnc->cropping_width = 0;
    }
    else
        pEnc->cropping_width = enc_p->u32CropWidth;
    if (enc_p->u32CropHeight >= 16) {
        printk("cropping height(%d) should smaller than 16, now set to be zero\n", enc_p->u32CropHeight);
        pEnc->cropping_height = 0;
    }
    else
        pEnc->cropping_height = enc_p->u32CropHeight;
#endif
    // Tuba 20120316 start: force syntax the same as 8120
    pEnc->forceSyntaxSameAs8120 = ptParam->encp_a2.u32Force8120;
    //  Tuba 20120316 end
	pEnc->mb_width = (pEnc->width + 15) / 16;
	pEnc->mb_height = (pEnc->height + 15) / 16;
	max_mb_w = (pEnc_x->max_width + 15) /16;
	max_mb_h = (pEnc_x->max_height + 15) /16;

	if (encoder_roi_init(pEnc, enc_p) < 0)
		return  -1;

	pEnc->u32VirtualBaseAddr = ptParam->u32VirtualBaseAddr;
	#ifdef DMAWRP
		pEnc->u32BaseAddr_wrapper = ptParam->va_base_wrp;
	#endif
	pEnc->bResyncMarker = enc_p->bResyncMarker ? 1: 0;
    pEnc->module_time_base=-1;
	pEnc->ac_disable = enc_p->ac_disable;

	pEnc->bEnable_4mv = enc_p->bEnable4MV;		// default mode
	pEnc_x->bH263Quant = enc_p->bH263Quant ;	// default mode 0
	if (enc_p->bShortHeader) {
		pEnc->bH263 = 1;
	}
	else {
		pEnc->bH263 = 0;
		if (enc_p->bEnable4MV)
			pEnc->bEnable_4mv = 1;
		
		if (enc_p->bH263Quant == 0)
			pEnc_x->bH263Quant = 0;
	}
         
	pEnc->u8Temporal_ref = 0;
	pEnc->fbase = fbase;
	pEnc->fincr = fincr;
	pEnc->m_ticks = 0;
	// set default quantization method: H263
	pEnc->bMp4_quant = H263_QUANT;
	// set default intra_ & inter_ matrix
	init_quant_matrix (get_default_intra_matrix(),
			pEnc->u8intra_matrix,
			pEnc->u32intra_matrix_fix1);
	init_quant_matrix(get_default_inter_matrix(),
			pEnc->u8inter_matrix,
			pEnc->u32inter_matrix_fix1);
	// set default custom quant: NO
	pEnc->custom_intra_matrix =
	pEnc->custom_inter_matrix = 0;

	/* motion dection initialization */
	/* we had move motion detection function out */
#ifdef MOTION_DETECTION_IN_DRV
	memset(pEnc->dev_buffer, 0, sizeof(int)*(max_mb_w * max_mb_h));
	pEnc->dev_flag = 0;
	pEnc->frame_counter = 0;
    pEnc->motion_dection_enable= ptParam->motion_dection_enable;
	pEnc->range_mb_x0_LU = ptParam->range_mb_x0_LU;
	pEnc->range_mb_y0_LU = ptParam->range_mb_y0_LU;
	pEnc->range_mb_x0_RD = ptParam->range_mb_x0_RD;
	pEnc->range_mb_y0_RD = ptParam->range_mb_y0_RD;
	pEnc->range_mb_x1_LU = ptParam->range_mb_x1_LU;
	pEnc->range_mb_y1_LU = ptParam->range_mb_y1_LU;
	pEnc->range_mb_x1_RD = ptParam->range_mb_x1_RD;
	pEnc->range_mb_y1_RD = ptParam->range_mb_y1_RD;
	pEnc->range_mb_x2_LU = ptParam->range_mb_x2_LU;
	pEnc->range_mb_y2_LU = ptParam->range_mb_y2_LU;
	pEnc->range_mb_x2_RD = ptParam->range_mb_x2_RD;
	pEnc->range_mb_y2_RD = ptParam->range_mb_y2_RD;
	pEnc->MV_th0 = ptParam->MV_th0;
	pEnc->sad_th0 = ptParam->sad_th0;
	pEnc->delta_dev_th0 = ptParam->delta_dev_th0;
	pEnc->MV_th1 = ptParam->MV_th1;
	pEnc->sad_th1 = ptParam->sad_th1;
	pEnc->delta_dev_th1 = ptParam->delta_dev_th1;
	pEnc->MV_th2 = ptParam->MV_th2;
	pEnc->sad_th2 = ptParam->sad_th2;
	pEnc->delta_dev_th2 = ptParam->delta_dev_th2;
	pEnc->md_interval = ptParam->md_interval;
#endif

	/* Fill rate control parameters */
	#ifdef TWO_P_EXT
	  pEnc->ptMP4 = (volatile MP4_t *)(MP4_OFF); //pEnc->ptMP4 = (volatile MP4_t *)(pEnc->u32VirtualBaseAddr + MP4_OFF);
	#else
      pEnc->ptMP4 = (volatile MP4_t *)(pEnc->u32VirtualBaseAddr + MP4_OFF);
	#endif
	mpeg_enc_init(pEnc->u32VirtualBaseAddr); //mpeg_init(pEnc->ptMP4);

	{
		pmdma = (volatile MDMA *)(pEnc->u32VirtualBaseAddr + DMA_OFF);
		pmdma->Status = 1;
	}

    pEnc->disableModeDecision = pEnc->sucIntraOverNum = pEnc->forceOverIntra = 0;

    // Tuba 20140409: fix bug reinit must reset parameter @ encoder_create_fake
    pEnc->m_ticks = 0;
	pEnc->m_seconds = 0;

	return 0;
}


Encoder_x *encoder_create(FMP4_ENC_PARAM_MY * ptParam)
{
	Encoder *pEnc;
	Encoder_x * pEnc_x;
	int max_mb_w,max_mb_h ;

#ifdef PERFORMANCE_TEST
hw_start = hw_end = 0;
hw_total = 0;;
frame_cnt = 0;
#endif

	if ((ptParam->pfnMalloc == NULL) || (ptParam->pfnFree == NULL))  {
       	printk("NO Malloc Function!\n");
		return NULL;
	}
	pEnc_x = (Encoder_x *) ptParam->pfnMalloc(sizeof(Encoder_x),
							ptParam->u32CacheAlign, ptParam->u32CacheAlign);
	if (pEnc_x == NULL) {
		printk ("encoder_create: malloc fail\n");
		return NULL;
	}
	pEnc = &pEnc_x->Enc;
	memset(pEnc_x, 0, sizeof(Encoder_x));
/*  // Tuba 20140409: fix bug reinit must reset parameter @ encoder_create_fake
	pEnc_x->max_width = ptParam->pub.enc_p.u32MaxWidth;
	pEnc_x->max_height = ptParam->pub.enc_p.u32MaxHeight;
	max_mb_w = (pEnc_x->max_width + 15) /16;
	max_mb_h = (pEnc_x->max_height + 15) /16;
	if (max_mb_w > 2048/16 ) {
		printk ("MPEG4E Setting error: max frame width is 2048, but %d\n",	pEnc_x->max_width);
		goto Gm_err_memory;
	}
*/
	#ifdef MOTION_DETECTION_IN_DRV
	{
		unsigned int mbs_size = sizeof(int) * (max_mb_w * max_mb_h + 1);
		/* try to allocate dev buffer */
		pEnc->dev_buffer = (int *) pEnc_x->pfnMalloc(mbs_size, pEnc_x->u32CacheAlign, pEnc_x->u32CacheAlign);
		if (pEnc->dev_buffer == NULL) {
			printk("Fail to allocate memory pEnc->dev_buffer size %d\n", mbs_size);
			goto Gm_err_memory;
		}
	}
	#endif

	if (encoder_create_fake(pEnc_x,  ptParam) < 0)
		goto Gm_err_memory;

    // Tuba 20140409: fix bug reinit must reset parameter @ encoder_create_fake
	max_mb_w = (pEnc_x->max_width + 15) /16;
	max_mb_h = (pEnc_x->max_height + 15) /16;

	/* try to allocate frame memory */
	pEnc->current1 = (FRAMEINFO *)pEnc_x->pfnMalloc(sizeof(FRAMEINFO), pEnc_x->u32CacheAlign, pEnc_x->u32CacheAlign);
	if (pEnc->current1 == NULL) {
		printk("Fail to allocate memory pEnc->current1 size %d\n",sizeof(FRAMEINFO));
		goto Gm_err_memory;
	}
	memset(pEnc->current1, 0, sizeof(FRAMEINFO));

	pEnc->reference = (FRAMEINFO *)pEnc_x->pfnMalloc(sizeof(FRAMEINFO), pEnc_x->u32CacheAlign, pEnc_x->u32CacheAlign);
	if (pEnc->reference == NULL) {
		printk("Fail to allocate memory pEnc->reference size %d\n",sizeof(FRAMEINFO));
		goto Gm_err_memory;
	}	
	memset(pEnc->reference, 0, sizeof(FRAMEINFO));

	#ifdef TWO_P_EXT
	{
	    #ifdef REF_POOL_MANAGEMENT
            #ifndef REF_BUFFER_FLOW	// Tuba 20140409: reduce number of reference buffer
            pEnc->current1->mbs_virt    = (MACROBLOCK_E *)ptParam->recon_buf->mb_virt;
            pEnc->current1->mbs         = (MACROBLOCK_E *)ptParam->recon_buf->mb_phy;
            pEnc->reference->mbs_virt   = (MACROBLOCK_E *)ptParam->refer_buf->mb_virt;
            pEnc->reference->mbs        = (MACROBLOCK_E *)ptParam->refer_buf->mb_phy;
            #endif
        //printk("assign mbinfo 0x%x, 0x%x\n", (unsigned int)pEnc->current1->mbs, (unsigned int)pEnc->reference->mbs);
        #else
		unsigned int mbs_size = sizeof(MACROBLOCK_E) * (max_mb_w * max_mb_h + 1);    
		pEnc->current1->mbs_virt = (MACROBLOCK_E *) pEnc_x->pfnDmaMalloc(mbs_size,
														pEnc_x->u32CacheAlign,
														pEnc_x->u32CacheAlign,
														(void **)&pEnc->current1->mbs);
		pEnc->reference->mbs_virt = (MACROBLOCK_E *) pEnc_x->pfnDmaMalloc(mbs_size,
														pEnc_x->u32CacheAlign,
														pEnc_x->u32CacheAlign,
														(void **)&pEnc->reference->mbs);
		if (pEnc->current1->mbs_virt ==NULL)  {
			printk("Fail to allocate memory pEnc->ref/rec->mbs size %d\n", mbs_size);
			goto Gm_err_memory;
		}
        #endif
	}
	#else
	{
		unsigned int mbs_size = sizeof(MACROBLOCK_E) * (max_mb_w * max_mb_h + 1);    
		pEnc->current1->mbs = (MACROBLOCK_E *) pEnc_x->pfnMalloc(mbs_size,
													pEnc_x->u32CacheAlign, pEnc_x->u32CacheAlign);
 		pEnc->reference->mbs = (MACROBLOCK_E *) pEnc_x->pfnMalloc(mbs_size,
								 					pEnc_x->u32CacheAlign, pEnc_x->u32CacheAlign);
		if (pEnc->current1->mbs == NULL) {
			printk("Fail to allocate memory pEnc->ref/rec->mbs size %d\n", mbs_size);
			goto Gm_err_memory;
		}
	}
	#endif

	/* try to allocate image memory */
	image_null(&pEnc->current1->image);
	image_null(&pEnc->reference->image);

	if (pEnc_x->pfnDmaMalloc) {
    #ifdef MPEG4_COMMON_PRED_BUFFER // Tuba 20131108: use common pred buffer
        pEnc->pred_value_virt = (uint16_t *)ptParam->mp4e_pred_virt;
        pEnc->pred_value_phy = (uint16_t *)ptParam->mp4e_pred_phy;
    #else
		pEnc->pred_value_virt = (uint16_t *)pEnc_x->pfnDmaMalloc(
					max_mb_w * 4 * 16,	//64,
					8,									//pEnc_x->u32CacheAlign,
					pEnc_x->u32CacheAlign,
					(void **)&(pEnc->pred_value_phy));
    #endif
		if (pEnc->pred_value_virt == NULL) {
			//printk("Fail to allocate pEnc->pred_value_virt 0x%x!\n", max_mb_w*4*16);
			printk("pred buffer is NULL\n");
			goto Gm_err_memory;
		}
    #ifndef REF_POOL_MANAGEMENT
		if (image_create_edge(&pEnc->current1->reconstruct, max_mb_w, max_mb_h, pEnc_x) < 0)
			goto Gm_err_memory;
		if (image_create_edge(&pEnc->reference->reconstruct, max_mb_w, max_mb_h, pEnc_x) < 0) {
			goto Gm_err_memory;
		}
    #else
        #ifndef REF_BUFFER_FLOW // Tuba 20140409: reduce number of reference buffer
            image_adjust_edge(&pEnc->current1->reconstruct, 
                (pEnc_x->max_width+15)/16, (pEnc_x->max_height+15)/16, (unsigned char*)ptParam->recon_buf->ref_phy);
            image_adjust_edge(&pEnc->reference->reconstruct, 
                (pEnc_x->max_width+15)/16, (pEnc_x->max_height+15)/16, (unsigned char*)ptParam->refer_buf->ref_phy);
        #endif
        //printk("assign ref 0x%x 0x%x\n", (unsigned int)pEnc->current1->reconstruct.y_phy, (unsigned int)pEnc->reference->reconstruct.y_phy);
    #endif
	}
	else {	// if it is NULL, use internal-provided buffer
		pEnc->pred_value_virt = NULL;
		pEnc->pred_value_phy = ptParam->p16ACDC;
		image_adjust_edge(&pEnc->current1->reconstruct, max_mb_w, max_mb_h, ptParam->pu8ReConFrameCur);
		image_adjust_edge(&pEnc->reference->reconstruct, max_mb_w, max_mb_h,ptParam->pu8ReConFrameRef);
	}
	return pEnc_x;

	/*
	 * We handle all GM_ERR_MEMORY here, this makes the code lighter
	 */
Gm_err_memory:
#ifndef MPEG4_COMMON_PRED_BUFFER // Tuba 20131108: use common pred buffer
	if (pEnc->pred_value_virt)
		ptParam->pfnDmaFree(pEnc->pred_value_virt, pEnc->pred_value_phy);
#endif
	if (pEnc->dev_buffer)
	    ptParam->pfnFree(pEnc->dev_buffer);
	if (pEnc->reference) {
    #ifndef REF_POOL_MANAGEMENT
		image_destroy_edge(&pEnc->reference->reconstruct, max_mb_w, pEnc_x);
		#ifdef TWO_P_EXT
			if (pEnc->reference->mbs_virt)
				ptParam->pfnDmaFree(pEnc->reference->mbs_virt, pEnc->current1->mbs);
		#else
			if (pEnc->reference->mbs)
				ptParam->pfnFree(pEnc->reference->mbs);
		#endif
    #endif
		ptParam->pfnFree(pEnc->reference);
	}

	if (pEnc->current1) {
    #ifndef REF_POOL_MANAGEMENT
		image_destroy_edge(&pEnc->current1->reconstruct, max_mb_w, pEnc_x);
		#ifdef TWO_P_EXT
			if (pEnc->current1->mbs_virt)
				ptParam->pfnDmaFree(pEnc->current1->mbs_virt, pEnc->current1->mbs);
		#else
			if (pEnc->current1->mbs)
				ptParam->pfnFree(pEnc->current1->mbs);
		#endif
    #endif
		ptParam->pfnFree(pEnc->current1);
	}
	ptParam->pfnFree(pEnc_x);
	return NULL;
}

void encoder_destroy(Encoder_x * pEnc_x)
{
	Encoder *pEnc = &(pEnc_x->Enc);
#ifndef REF_POOL_MANAGEMENT
	int max_mb_w = (pEnc_x->max_width + 15) /16;
#endif

    if((pEnc==0)||(pEnc_x->pfnDmaFree==0))
        return;
#ifndef MPEG4_COMMON_PRED_BUFFER // Tuba 20131108: use common pred buffer
	pEnc_x->pfnDmaFree(pEnc->pred_value_virt, pEnc->pred_value_phy);
#endif
#ifndef REF_POOL_MANAGEMENT
    image_destroy_edge(&pEnc->current1->reconstruct, max_mb_w, pEnc_x);
    image_destroy_edge(&pEnc->reference->reconstruct, max_mb_w, pEnc_x);
#endif
	if (pEnc->dev_buffer)
		pEnc_x->pfnFree(pEnc->dev_buffer);
#ifndef REF_POOL_MANAGEMENT
	#if defined(TWO_P_EXT)
       	pEnc_x->pfnDmaFree(pEnc->current1->mbs_virt, pEnc->current1->mbs);
       	pEnc_x->pfnDmaFree(pEnc->reference->mbs_virt, pEnc->reference->mbs);
	#else
       	pEnc_x->pfnFree(pEnc->current1->mbs);
       	pEnc_x->pfnFree(pEnc->reference->mbs);
	#endif
#endif
	
	pEnc_x->pfnFree(pEnc->current1);
	pEnc_x->pfnFree(pEnc->reference);
	pEnc_x->pfnFree(pEnc_x);
	return;
}

static __inline void inc_frame_num(Encoder * pEnc)
{
	pEnc->m_ticks += pEnc->fincr;
	pEnc->m_seconds = pEnc->m_ticks / pEnc->fbase;
	pEnc->m_ticks -= pEnc->m_seconds * pEnc->fbase;
//	pEnc->m_ticks = pEnc->m_ticks % pEnc->fbase;
}

static uint32_t FlushBitstream(volatile MP4_t * ptMP4)
{
	uint32_t data_64B;
	uint32_t data_4B;
	uint32_t data_1b;
	uint32_t flush_size;
	int wait_cnt = 0;		
	// check autobuffer done
	while ((ptMP4->VLDSTS & BIT10) == 0) {
		if (++wait_cnt == 1000) {
			printk ("check autobuffer done fail\n");
			return 0;
		}
	}
	data_64B = ptMP4->ASADR;			// system address for current access
									// 64 bytes boundary
	data_4B = ptMP4->BITLEN & 0x3c;	// Auto-buffering local memory access pointer
									// 4 bytes (2 words) boundary
	data_1b = ptMP4->VADR & 0x1F;	// indicate bit offset within 2 words
	data_64B = data_64B + data_4B + ((data_1b + 7) >> 3);

	// Because during testing the real chip FIE8100, we found there is
	// difference between real chip (FIE8100) and FPGA target for autobuffer mechanism. That is,
	// whenever we enable the autobuffer mechanism, the bitstream buffer offset pointer can be
	// reset to zero under FPAG target. But in real chip (FIE8100), because of gated-clock,the
	// bitstream buffer offset pointer can not be reset to zero. So we figure out the following
	// code to fix such problem.
	flush_size= ptMP4->BITLEN;		// total byte in local_memory
	flush_size = 256 - flush_size;		// total byte to 256 bytes
	flush_size >>= 1;					// total half-word to 256 bytes
	ptMP4->BITDATA = 0;				// set dummy data
	// push dummy data to local memory
	// to make "autobuffering mechanism" to flush local memory to system memory
	for (; flush_size > 0; flush_size--)
		ptMP4->BITLEN = 16;

	wait_cnt = 0;
	// check autobuffer done
 	while ((ptMP4->VLDSTS & BIT10) == 0) {
		if (++wait_cnt == 1000) {
			printk ("check autobuffer 2 done fail\n");
			return 0;
		}
	}
	//	stop autobuffer
	ptMP4->VLDCTL = VLDCTL_ABFSTOP;
	// clear packing index within VLC-module
	ptMP4->VADR = RUN_LEVEL_OFF;
	return data_64B;
}
int encoder_roi_update(Encoder *pEnc, FMP4_ENC_FRAME * pFrame)
{   
	int temp;
	int allign_byte_h = PIXEL_Y;
	int allign_byte_v = PIXEL_Y;

	if (pFrame->roi_x < 0)
		pFrame->roi_x = pEnc->roi_x;
	if (pFrame->roi_y < 0)
		pFrame->roi_y = pEnc->roi_y;
	
	switch (pEnc->img_fmt) {
		case 2: // RASTER_SCAN_420
			allign_byte_h = 8;
			allign_byte_v = 2;
			break;
		case 3: // RASTER_SCAN_422
			allign_byte_h = 2;
			allign_byte_v = 1;
			break;
		default: // H2642D, MPEG42D
			break;
	}
	
	// roi_x
	temp = pFrame->roi_x & (allign_byte_h - 1);
	if (temp) {
		printk("[mp4e]error: roi_x %d must align at %d\n", pFrame->roi_x, allign_byte_h);
		return -1;
	}
	// roi_y
	temp = pFrame->roi_y & (allign_byte_v - 1);
	if (temp) {
		printk("[mp4e]error: roi_y %d must align at %d\n", pFrame->roi_y, allign_byte_v);
		return -1;
	}
	
	if ((pFrame->roi_x + pEnc->roi_width) > pEnc->width) {
		printk ("[mp4e]error: roi_x (%d) + roi_w (%d) > image_width (%d)\n",
								pFrame->roi_x, pEnc->roi_width, pEnc->width);
		return -1;
	}
	if ((pFrame->roi_y + pEnc->roi_height) > pEnc->height) {
		printk ("[mp4e]error: roi_y (%d) + roi_h (%d) > image_height (%d)\n",
								pFrame->roi_y, pEnc->roi_height, pEnc->height);
		return -1;
	}
	pEnc->roi_x = pFrame->roi_x;
	pEnc->roi_y = pFrame->roi_y;

	pEnc->mb_offset = pEnc->mb_stride * (pEnc->roi_y>>4) + (pEnc->roi_x>>4);
	return 0;
}

void 
encoder_reset(Encoder_x * pEnc_x)
{
	Encoder *pEnc = &(pEnc_x->Enc);	
  	volatile MDMA * ptDma = (volatile MDMA *)(Mp4EncBase(pEnc) + DMA_OFF); //volatile MDMA * ptDma = (volatile MDMA *)(pEnc->u32VirtualBaseAddr + DMA_OFF);
	
	ptDma->Control  |= 1<<27;
	while( ptDma->Control & (1<<27) );
}

#define SHOW(str) printk(#str" = %d\n", str);
void ee_show(Encoder_x * pEnc_x)
{
	Encoder *Enc = &pEnc_x->Enc;
	SHOW (pEnc_x->bH263Quant);
	SHOW (pEnc_x->iFrameNum);
	SHOW (pEnc_x->iMaxKeyInterval);
	SHOW (pEnc_x->vol_header);
	SHOW (pEnc_x->max_width);
	SHOW (pEnc_x->max_height);
	SHOW (Enc->custom_intra_matrix);
	SHOW (Enc->custom_inter_matrix);
	SHOW (Enc->bH263);
	SHOW (Enc->bEnable_4mv);
	SHOW (Enc->bResyncMarker);
	SHOW (Enc->bMp4_quant);
	SHOW (Enc->u8Temporal_ref);
	SHOW (Enc->width);
	SHOW (Enc->height);
	SHOW (Enc->roi_x);
	SHOW (Enc->roi_y);
	SHOW (Enc->mb_offset);
	SHOW (Enc->mb_stride);
	SHOW (Enc->roi_width);
	SHOW (Enc->roi_height);
	SHOW (Enc->roi_mb_width);
	SHOW (Enc->roi_mb_height);
	SHOW (Enc->mb_width);
	SHOW (Enc->mb_height);
	SHOW (Enc->fincr);
	SHOW (Enc->fbase);
	SHOW (Enc->bRounding_type);
	SHOW (Enc->m_seconds);
	SHOW (Enc->m_ticks);
	SHOW (Enc->module_time_base);
	SHOW (Enc->ac_disable);
	SHOW (Enc->img_fmt);
}
extern void mcp100_switch(void * codec, ACTIVE_TYPE type);
int ee_Tri(Encoder_x * pEnc_x, FMP4_ENC_FRAME * pFrame, int force_vol)
{
	Encoder *pEnc = &pEnc_x->Enc;
	volatile MP4_t * ptMP4 = (volatile MP4_t *)(pEnc->u32VirtualBaseAddr + MP4_OFF); //volatile MP4_t * ptMP4 = pEnc->ptMP4;
	uint32_t write_vol_header = 0;
	char not_coded_flag=1;
/*
printk("successive intra overflow number = %d\n", pEnc->sucIntraOverNum);
printk("disable mode decision = %d\n", pEnc->disableModeDecision);
printk("force intra %d\n", pEnc->forceOverIntra);
*/    
	pEnc->bitstream = pFrame->bitstream;
#ifdef EVALUATION_PERFORMANCE
  	do_gettimeofday(&tv_curr);	
  	if (  time_delta( &tv_init, &tv_curr) > TIME_INTERVAL ) {
		enc_performance_report();
		enc_performance_reset();
		tv_init.tv_sec = tv_curr.tv_sec;
		tv_init.tv_usec = tv_curr.tv_usec;
  	}
  	enctotal.count++;	
  	// encode start timestamp
  	encframe.ap_stop = get_counter();
  	enc_performance_count();
	encframe.ap_start = encframe.ap_stop;
  	encframe.start= encframe.ap_stop;
    #elif defined(PERFORMANCE_TEST)
    if (frame_cnt > 100) {
        printk("encoder time %d\n", hw_total/frame_cnt);
        hw_total = 0;
        hw_start = hw_end = 0;
        frame_cnt = 0;
    }
    hw_start = get_counter();
//printk("%d: hw start %x\n", frame_cnt, hw_start);
#endif	
	
	pFrame->length = 0; 	// this is written by the routine
	pEnc->reference->image.y_phy = pFrame->pu8YFrameBaseAddr;
	pEnc->reference->image.u_phy = pFrame->pu8UFrameBaseAddr;
	pEnc->reference->image.v_phy = pFrame->pu8VFrameBaseAddr;

    if((pFrame->module_time_base&0xffff0000)==0x55aa0000)
        pEnc->module_time_base=pFrame->module_time_base&0xffff;
	if (pEnc->reference->image.y_phy != NULL)	{
		SWAP(pEnc->current1, pEnc->reference);
		not_coded_flag=0;
    #ifdef REF_BUFFER_FLOW  // Tuba 20140409: reduce number of reference buffer
        pEnc->current1->mbs_virt    = (MACROBLOCK_E *)pFrame->pReconBuffer->mb_virt;
        pEnc->current1->mbs         = (MACROBLOCK_E *)pFrame->pReconBuffer->mb_phy;
        image_adjust_edge(&pEnc->current1->reconstruct, 
            (pEnc_x->max_width+15)/16, (pEnc_x->max_height+15)/16, (unsigned char*)pFrame->pReconBuffer->ref_phy);
    #endif
	}
	if (encoder_roi_update(pEnc, pFrame) < 0)
		return -1;

	// It was used to set the Quantization Block Address to 0x0800
	ptMP4->QAR = ((QCOEFF_OFF & 0xFFFF) << 16) | (QCOEFF_OFF & 0xFFFF);

	#ifndef GM8120
 	ptMP4->MCCTL = MCCTL_ENC_MODE | (pEnc->bH263 << 14);
	#else
	// *************** dummy mc go trigger, start
	//stop autobuffer in case previous firmware program did not terminate normally
	ptMP4->VLDCTL = VLDCTL_ABFSTOP;
	ptMP4->ASADR = (uint32_t) (pFrame->bitstream) | BIT0;
	ptMP4->VADR = RUN_LEVEL_OFF;
	ptMP4->VLDCTL = VLDCTL_ABFSTART;
	ptMP4->QCR0 = 31 << 18;
	// set the MC Interpolation and Result Block Start Address.
 	ptMP4->MCIADDR = INTER_Y0;
	// set the MC Current Block Start Address.
 	ptMP4->MCCADDR = CUR_Y0;
	// first me_go will reference this h263 bit.
	// --> need to set h263 mode correct.
 	ptMP4->MCCTL = MCCTL_MCGO | 
					#ifndef GM8120
						MCCTL_ENC_MODE |
					#endif
					MCCTL_INTRA_F | (pEnc->bH263 << 14);
 	// *************** dummy mc go trigger, stop
	#endif
	ptMP4->CMDADDR = ME_CMD_Q_OFF;
	
	pEnc->current1->seconds = pEnc->m_seconds;
	pEnc->current1->ticks = pEnc->m_ticks;
	if ((pFrame->quant > 31) || (pFrame->quant <= 0)) {
		printk ("input quant is illegal max: 31, min: 1, but %d\n", pFrame->quant);
		return -1;
	}
	pEnc->current1->quant = pFrame->quant;

    RTL_DEBUG_OUT(0x92100000 | (pFrame->quant))
	RTL_DEBUG_OUT(0x94000000 | (pEnc->current1->quant))
	
	if (pEnc_x->bH263Quant==1) {
		if (pEnc->bMp4_quant)	// MPEG4_QUANT
			write_vol_header = 1;
		pEnc->bMp4_quant = H263_QUANT;
	}
	else {
		uint8_t matrix1_changed, matrix2_changed;
		matrix1_changed = matrix2_changed = 0;
		if (pEnc->bMp4_quant == H263_QUANT)
			write_vol_header = 1;
		
		pEnc->bMp4_quant = MPEG4_QUANT;
		
		if (pFrame->quant_intra_matrix) {		// customize quant
			matrix1_changed = set_quant_matrix(pFrame->quant_intra_matrix,
						pEnc->u8intra_matrix,
						get_default_intra_matrix(),
						&(pEnc->custom_intra_matrix),
						pEnc->u32intra_matrix_fix1);
		} else {
			matrix1_changed = set_quant_matrix(get_default_intra_matrix(),
						pEnc->u8intra_matrix,
						get_default_intra_matrix(),
						&(pEnc->custom_intra_matrix),
						pEnc->u32intra_matrix_fix1);
		}	
		if (pFrame->quant_inter_matrix) {		// customize quant
			matrix2_changed = set_quant_matrix(pFrame->quant_inter_matrix,
						pEnc->u8inter_matrix,
						get_default_inter_matrix(),
						&(pEnc->custom_inter_matrix),
						pEnc->u32inter_matrix_fix1);
		} else {
			matrix2_changed = set_quant_matrix(get_default_inter_matrix(),
						pEnc->u8inter_matrix,
						get_default_inter_matrix(),
						&(pEnc->custom_inter_matrix),
						pEnc->u32inter_matrix_fix1);
		}
		
		if (write_vol_header == 0)
			write_vol_header = matrix1_changed | matrix2_changed;
	}
	mcp100_switch ((void *)pEnc, TYPE_MPEG4_ENCODER);
	dma_enc_cmd_init_each (pEnc);

	#ifdef GM8120
	// *************** dummy mc go, start
	// wait for VLC_done, MC_done
	while ((ptMP4->CPSTS & (BIT15 | BIT1)) != (BIT15 | BIT1))
		;
	// *************** dummy mc go, stop
	#endif
	//stop autobuffer in case previous firmware program did not terminate normally
	ptMP4->VLDCTL = VLDCTL_ABFSTOP;
	// bit 0 means WRITE bs in encoder mode
	ptMP4->ASADR = (uint32_t) (pFrame->bitstream) | BIT0;
	// clear packing index within VLC-module when you want one package each frame
	// Do NOT clear when you want to make bit stream with many frames 
	ptMP4->VADR = RUN_LEVEL_OFF;
	// enable auto-buffering and swap endian
	ptMP4->VLDCTL = VLDCTL_ABFSTART;

	if ( force_vol == 1)
		pEnc_x->vol_header |= 0x10000000;
	
	if(not_coded_flag)	{
		pFrame->intra = 0;
		pEnc_x->vol_header = write_vol_header;
		#ifdef TWO_P_EXT
		if (encoder_nframe_tri(pEnc_x) < 0)
			return -1;
		#else
		encoder_nframe(pEnc_x);
		#endif
	}
	else	{
		if (pFrame->intra < 0) {
            // Tuba 20111209_1 start
            if (pEnc->forceOverIntra) {
                pFrame->intra = 1;
                pEnc->forceOverIntra = 0;
            }
            // Tuba 20111209_1 end 
            else {
    			if ((pEnc_x->iFrameNum == 0) || ((pEnc_x->iMaxKeyInterval > 0)
    				&& (pEnc_x->iFrameNum >= pEnc_x->iMaxKeyInterval)))
    				pFrame->intra = 1;
    			else
    				pFrame->intra = 0;
            }
		}
		if (pFrame->intra == 1) {
            //int cnt = 0;
			pEnc_x->iFrameNum = 0;
			#ifdef TWO_P_EXT
			if (encoder_iframe_tri(pEnc_x) < 0) {
				printk("encode I frame error\n");				
				return -1;
			}
            /*
            while (cnt < 0x10) {
                Share_Node_Enc *node = (Share_Node_Enc *)(pEnc->u32VirtualBaseAddr + SHAREMEM_OFF);
                printk("CPSTS: %08x\n", ptMP4->CPSTS);
                printk("command: %x\n", node->command);
                printk("int_enable: %x\n", node->int_enabled);   //Yes:0x12345678, NO:0x87654321
                printk("int_flag: %x\n", node->int_flag);
                printk("INT: %d\n", ptMP4->INTEN);
                cnt++;
            }
            */
			#else
			encoder_iframe(pEnc_x);
			#endif
		} else {
		    pEnc_x->vol_header = write_vol_header;
			#ifdef TWO_P_EXT
			if (encoder_pframe_tri(pEnc_x) < 0)
				return -1;
			#else
			encoder_pframe(pEnc_x);
			#endif
		}
	}
	return 0;
}
int ee_End(Encoder_x * pEnc_x)
{
	int bs_sz;
	Encoder *pEnc = &(pEnc_x->Enc);	
	volatile MP4_t * ptMP4 = (volatile MP4_t *)(pEnc->u32VirtualBaseAddr + MP4_OFF); //volatile MP4_t * ptMP4 = pEnc->ptMP4;

	#ifdef TWO_P_EXT
	if (encoder_frame_sync(pEnc_x) < 0)
		return 0;
	#endif

	bs_sz= (int)(FlushBitstream(ptMP4) - (uint32_t)pEnc->bitstream);
	inc_frame_num(pEnc);
	pEnc_x->iFrameNum++;
	#ifdef EVALUATION_PERFORMANCE
		// encode end timestamp
		encframe.stop = get_counter();
    #elif defined(PERFORMANCE_TEST)
    hw_end = get_counter();
    hw_total += hw_end - hw_start;
    frame_cnt++;
  	#endif
	return bs_sz;
}

#ifndef SUPPORT_VG
int encoder_encode(Encoder_x * pEnc_x, FMP4_ENC_FRAME * pFrame, int force_vol)
{
	if (ee_Tri(pEnc_x, pFrame, force_vol) < 0)
		return -1;
	#ifdef TWO_P_EXT
		if (encoder_frame_block(pEnc_x) < 0)
			return -1;
	#endif
	pFrame->length = ee_End(pEnc_x);
	return 0;
}
#endif


typedef struct
{
	int32_t mb_width;
	uint32_t * intra_matrix_fix1;
	uint32_t * inter_matrix_fix1;
} duplex_struct_mp4e;

duplex_struct_mp4e tDuplex_mp4e = {-1, NULL, NULL};

void switch2mp4e(void * codec, ACTIVE_TYPE curr)
{
	int ud_dma_each = 0;
	int ud_q = 0;
	Encoder * enc = (Encoder *)codec;
	if (curr != TYPE_MPEG4_ENCODER) {
		tDuplex_mp4e.intra_matrix_fix1 = NULL;	// force update quant.
	}
	else {
		if (tDuplex_mp4e.mb_width == -1)
			ud_dma_each = 2;
		else if (tDuplex_mp4e.mb_width != enc->mb_width)
			ud_dma_each = 1;
		if ((enc->bMp4_quant) &&
			((tDuplex_mp4e.intra_matrix_fix1 != enc->u32intra_matrix_fix1) ||
			(tDuplex_mp4e.inter_matrix_fix1 != enc->u32inter_matrix_fix1)))
			ud_q = 1;

		if (ud_dma_each) {
			if (2 == ud_dma_each) {
				/*	initial DMA command chain */
				dma_enc_commandq_init(enc);
				// init me command queue (common part: inter & intra)
				// Tuba 20111209_1
				//me_enc_common_init(enc->u32VirtualBaseAddr);
				me_enc_common_init(enc->u32VirtualBaseAddr, enc->disableModeDecision);
			}
			dma_enc_commandq_init_each(enc);
			tDuplex_mp4e.mb_width = enc->mb_width;
		}
		if (ud_q) {	// MPEG4_QUANT
			int i;
			uint32_t * ptr_d, * ptr_s;
			ptr_d = (uint32_t *)(enc->u32VirtualBaseAddr + INTRA_QUANT_TABLE_ADDR);
			ptr_s = &enc->u32intra_matrix_fix1[0];
			for (i = 0; i < 64; i ++)
				*(ptr_d ++) = *(ptr_s ++);
			ptr_d = (uint32_t *)(enc->u32VirtualBaseAddr + INTER_QUANT_TABLE_ADDR);
			ptr_s = &enc->u32inter_matrix_fix1[0];
			for (i = 0; i < 64; i ++)
				*(ptr_d ++) = *(ptr_s ++);
			tDuplex_mp4e.intra_matrix_fix1 = enc->u32intra_matrix_fix1;
			tDuplex_mp4e.inter_matrix_fix1 = enc->u32inter_matrix_fix1;
		}
	}
}
