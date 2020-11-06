#define LOCAL_MEM_GLOBAL
#ifdef LINUX
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#endif
#include "Mp4Vdec.h"
#include "dma_b.h"
#include "../common/define.h"
#include "../common/portab.h"
#include "../common/dma.h"
#include "../common/mp4.h"
#include "../common/vpe_m.h"
#include "../common/dma_m.h"
#include "../common/image.h"
#include "me.h"
#include "decoder.h"
#include "bitstream_d.h"
#include "image_d.h"
#include "local_mem_d.h"
//#include "mbprediction.h"
#include "mem_transfer1.h"
#include "ioctl_mp4d.h"

#define BIT_0ST		BIT0
#define BIT_1ST		BIT1
#define BIT_2ST		BIT2
#define BIT_3ST		BIT3
#define BIT_4ST		BIT4
#define BIT_5ST		BIT5

#define BIT_DMA_RGB_GO	BIT8
#define BIT_PRE_DT		BIT9

#ifdef ENABLE_DEBLOCKING
#define I_FRAME_PIPE	5
#else
#define I_FRAME_PIPE	4
#endif
#define MBS_LOCAL_I	(I_FRAME_PIPE + 1)

#ifdef ENABLE_DEBLOCKING
#define P_FRAME_PIPE	6
#else
#define P_FRAME_PIPE	5
#endif
#define MBS_LOCAL_P	(P_FRAME_PIPE + 1)
#define MBS_ACDC		(4)
#define NVOP_MAX	0x1000


#ifdef TWO_P_INT
static __inline uint32_t predict_acdc_P(const MACROBLOCK * pMBs,
			const MACROBLOCK_b * mbb,
			const int32_t mb_width,
			const int32_t bound,
			const int32_t i,
			const MACROBLOCK * left)
{
	uint32_t u32temp;
	const int32_t mbpos = mbb->mbpos;

	#if 0
	const MACROBLOCK * up = &pMBs[((i + 3) % MBS_ACDC)];
	const MACROBLOCK * up_left = &pMBs[((i + 2) % MBS_ACDC)];
	#else
	MACROBLOCK *up=(MACROBLOCK *)&pMBs[0];
	MACROBLOCK *up_left=(MACROBLOCK *)&pMBs[0];
	if(i>=2)
		up=(MACROBLOCK *)&pMBs[(i-2)%MBS_ACDC];
	if(i>=3)	
		up_left=(MACROBLOCK *)&pMBs[(i-3)%MBS_ACDC];	
	#endif

	if (mbb->toggle)
		u32temp = MCCTL_REMAP | MCCTL_DECGO;
	else
		u32temp = MCCTL_DECGO;

	// left macroblock valid ?
	// No need to check bound here, hardware will check it
	if ((mbb->x == 0) || (left->mode != MODE_INTRA && left->mode != MODE_INTRA_Q))
		u32temp |= MCCTL_ACDC_L;

	// diag macroblock valid ?
	if ((mbb->x == 0) || (mbpos < (bound + mb_width + 1)) ||
		(up_left->mode != MODE_INTRA && up_left->mode != MODE_INTRA_Q))
		u32temp |= MCCTL_ACDC_D;

	// top macroblock valid ?
	if ((mbpos < (bound + mb_width)) || (up->mode != MODE_INTRA && up->mode != MODE_INTRA_Q))
		u32temp |= MCCTL_ACDC_T;

	return u32temp;
}
#else
static __inline uint32_t predict_acdc_P(const MACROBLOCK * pMBs,
			const MACROBLOCK_b * mbb,
			const int32_t mb_width,
			const int32_t bound)
{
	uint32_t u32temp;
	const int32_t mbpos = mbb->mbpos;
	

	if (mbb->toggle)
		u32temp = MCCTL_REMAP | MCCTL_DECGO;
	else
		u32temp = MCCTL_DECGO;

	// left macroblock valid ?
	// No need to check bound here, hardware will check it
	if ((mbb->x == 0) || (pMBs[mbpos - 1].mode != MODE_INTRA && pMBs[mbpos - 1].mode != MODE_INTRA_Q))
		u32temp |= MCCTL_ACDC_L;

	// diag macroblock valid ?
	if ((mbb->x == 0) || (mbpos < (bound + mb_width + 1)) ||
		(pMBs[mbpos - 1 - mb_width].mode != MODE_INTRA &&
		 pMBs[mbpos - 1 - mb_width].mode != MODE_INTRA_Q))
		u32temp |= MCCTL_ACDC_D;

	// top macroblock valid ?
	if ((mbpos < (bound + mb_width))  ||
		(pMBs[mbpos - mb_width].mode != MODE_INTRA &&
		 pMBs[mbpos - mb_width].mode != MODE_INTRA_Q))
		u32temp |= MCCTL_ACDC_T;

	return u32temp;
}
#endif


static __inline uint32_t predict_acdc_I(const MACROBLOCK_b * mbb,
			const int32_t mb_width,
			const int32_t bound)
{
	uint32_t u32temp;

	if (mbb->toggle)
		u32temp = MCCTL_REMAP | MCCTL_DECGO;
	else
		u32temp = MCCTL_DECGO;

	// left macroblock valid ?
	// No need to check bound here, hardware will check it
	if (mbb->x == 0)
		u32temp |= (MCCTL_ACDC_L | MCCTL_ACDC_D);

	// diag macroblock valid ?
	if (mbb->mbpos < (bound + mb_width + 1)) {
		u32temp |= MCCTL_ACDC_D;
		// top macroblock valid ?
		if ((mbb->mbpos < (bound + mb_width)))
			u32temp |= MCCTL_ACDC_T;
	}

	return u32temp;
}

static uint32_t FindRsmkOrVopS(Bitstream * const bs, volatile MP4_t * ptMP4)
{
	uint32_t u32temp;

	while (1) {
		// MPEG4 search re-sync marker before decoding
		u32temp = ptMP4->VLDCTL & ~0x000F;
		ptMP4->VLDCTL = u32temp | mVLDCTL_CMD4b(2);
		ptMP4->MCCTL |= MCCTL_DECGO;
		mFa526DrainWrBuf();
		while (((u32temp = ptMP4->VLDSTS) & BIT0) == 0) ;

		if ((u32temp & 0xF000) == 0xE000) {
			// timeout
			for (; (ptMP4->VLDSTS & BIT10)==0; ) ;			/// wait for autobuffer clean
			BitstreamUpdatePos_phy(bs, 
				(uint32_t *)(ptMP4->ASADR - 256 + (ptMP4->BITLEN & 0x3f) - 0xc),
				ptMP4->VADR & 0x1f);
			//BitstreamUpdatePos(bs,
			//	(uint32_t *)(ptMP4->ASADR - 256 + (ptMP4->BITLEN & 0x3f) - 0xc),
			//	ptMP4->VADR & 0x1f);
			if ((BitstreamPos(bs) >> 3) >= bs->length) { // nothing found
				u32temp = BIT2;
				break;
			}
		} else {
			break;
		}
	}
	return u32temp;
}

static void dma_chain_init_i(DECODER * dec)
{
	uint32_t * dma_cmd_tgl0 = (uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_0);
	uint32_t * dma_cmd_tgl1 = (uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_1);
	int32_t chn_store_preditor, chn_load_preditor;
	// for the 1st time dma trigger, use tgl_1 dma chain,
	// fill these 2 register to avoid the dma register unknow
	if ( dec->output_fmt == OUTPUT_FMT_YUV) {
		chn_store_preditor = CHN_STORE_YUV_PREDITOR;
		chn_load_preditor = CHN_LOAD_YUV_PREDITOR;
	} else {
		chn_store_preditor = CHN_STORE_RGB_PREDITOR;
		chn_load_preditor = CHN_LOAD_RGB_PREDITOR;
	}
	dma_cmd_tgl1[chn_store_preditor] =
	dma_cmd_tgl1[chn_load_preditor] =
			mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy)
			| mDmaSysInc3b(DMA_INCS_128);
	// image u
	dma_cmd_tgl0[CHN_IMG_U] =
			mDmaSysMemAddr29b((uint32_t) dec->cur.u_phy)
			|mDmaSysInc3b(DMA_INCS_128);		// 2 * SIZE_U
	dma_cmd_tgl1[CHN_IMG_U] =
			mDmaSysMemAddr29b((uint32_t) dec->cur.u_phy+ 64)
			|mDmaSysInc3b(DMA_INCS_128);		// 2 * SIZE_U
	// image v
	dma_cmd_tgl0[CHN_IMG_V] =
			mDmaSysMemAddr29b((uint32_t) dec->cur.v_phy)
			|mDmaSysInc3b(DMA_INCS_128);		// 2 * SIZE_V
	dma_cmd_tgl1[CHN_IMG_V] =
			mDmaSysMemAddr29b((uint32_t) dec->cur.v_phy+ 64)
			|mDmaSysInc3b(DMA_INCS_128);		// 2 * SIZE_V
	#ifdef TWO_P_INT
	dma_cmd_tgl0[CHN_STORE_MBS] =
			mDmaSysMemAddr29b((uint32_t) dec->mbs)
			|mDmaSysInc3b(DMA_INCS_48);		// 2 * MB_SIZE
	dma_cmd_tgl1[CHN_STORE_MBS] =
			mDmaSysMemAddr29b((uint32_t) dec->mbs + MB_SIZE)
			|mDmaSysInc3b(DMA_INCS_48);		// 2 * MB_SIZE
	#endif
}

static void dma_chain_init_p(DECODER * dec)
{
	uint32_t * dma_cmd_tgl0 = (uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_0);
	uint32_t * dma_cmd_tgl1 = (uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_1);
	int32_t chn_store_preditor, chn_load_preditor;
	// for the 1st time dma trigger, use tgl_1 dma chain,
	// fill these 2 register to avoid the dma register unknow
	if ( dec->output_fmt == OUTPUT_FMT_YUV) {
		chn_store_preditor = CHN_STORE_YUV_PREDITOR;
		chn_load_preditor = CHN_LOAD_YUV_PREDITOR;
	} else {
		chn_store_preditor = CHN_STORE_RGB_PREDITOR;
		chn_load_preditor = CHN_LOAD_RGB_PREDITOR;
	}

	dma_cmd_tgl1[chn_store_preditor] =
	dma_cmd_tgl1[chn_load_preditor] =
			mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy)
			| mDmaSysInc3b(DMA_INCS_128);
	// image u
	dma_cmd_tgl0[CHN_IMG_U] =
			mDmaSysMemAddr29b((uint32_t) dec->cur.u_phy + 64)
			|mDmaSysInc3b(DMA_INCS_128);		// 2 * SIZE_U
	dma_cmd_tgl1[CHN_IMG_U] =
			mDmaSysMemAddr29b((uint32_t) dec->cur.u_phy)
			|mDmaSysInc3b(DMA_INCS_128);		// 2 * SIZE_U
	// image v
	dma_cmd_tgl0[CHN_IMG_V] =
			mDmaSysMemAddr29b((uint32_t) dec->cur.v_phy + 64)
			|mDmaSysInc3b(DMA_INCS_128);		// 2 * SIZE_V
	dma_cmd_tgl1[CHN_IMG_V] =
			mDmaSysMemAddr29b((uint32_t) dec->cur.v_phy)
			|mDmaSysInc3b(DMA_INCS_128);		// 2 * SIZE_V
	#ifdef TWO_P_INT
	dma_cmd_tgl0[CHN_STORE_MBS] =
			mDmaSysMemAddr29b((uint32_t) dec->mbs)
			|mDmaSysInc3b(DMA_INCS_48);		// 2 * MB_SIZE
	dma_cmd_tgl1[CHN_STORE_MBS] =
			mDmaSysMemAddr29b((uint32_t) dec->mbs + MB_SIZE)
			|mDmaSysInc3b(DMA_INCS_48);		// 2 * MB_SIZE
	#endif
}
// pls refer to dma_b.c --> "pipeline arrangement"


// Conf0: configure output as RGB or CbYCrY
// Conf1: configure output as YUV420
/*
				<-  mb_vld	      -><- mb_dmc	    -><- mb_img       -><- mb_rgb    ->
												<- mb_dt    ->
												<- mb_yuv  ->
<-00st stage	    -><-  1st stage      -><- 2nd stage  -><- 3nd stage   -><- 4nd stage ->
----------------------------cycle start -----------------------------------------------
				(3)goMC_vld------(9)goMC_dmc-----(16)goMC_dt
				(4)goDMA_LD-----(10)goDMA_ST-----(17)goDMA_img--(23)goDMA_ RGB(R-x)(Conf0)
											|---(18)goDMA_yuv(Conf1)
~~change to next mb~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
								(11)preMoveImg(1S-2S)
								(12)preMoveYuv(1S-2S)(Conf1)
												(19)preMoveRGB(3S-R)(D)(Conf0)
												(19')(Conf1)(3S-x)
				(5)wait VLD(0S-1S)
(1)preVLD(x-0S)
				(6)preStoreACDC
(2)preLoadACDC
				(7)waitDMA_LD----(13)waitDMA_ST---(20)waitDMA_img---(24)waitDMA_RGB(Conf0)
											|---(21)waitDMA_yuv(Conf1)
												(22)waitDT(D-x)(Conf0)
								(14)prDT(2S)(x-D)(Conf0)
								(15)wait DMC(2S-3S)
				(8)preDMC(1S)
----------------------------cycle end --------------------------------------------------
*/


int decoder_iframe(DECODER * dec, Bitstream * bs)
{

	volatile MP4_t * ptMP4 = (MP4_t *)(Mp4Base (dec) + MP4_OFF);
	volatile MDMA * ptDma = (MDMA *)(Mp4Base (dec) + DMA_OFF);
	uint32_t * dma_cmd_tgl0 = (uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_0);
	uint32_t * dma_cmd_tgl1 = (uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_1);
	uint32_t * dma_cmd_tgl;
	int32_t bound = 0;
	int32_t i;

	uint32_t u32temp;
	uint32_t u32cmd_mc;
	uint32_t u32pipe = 0;
	uint32_t u32cmd_mc_reload;
	uint32_t mb_cnt_in_vp = 0;			// MB count in video packet
	uint32_t * pu32table = &((uint32_t *)( Mp4Base (dec) + TABLE_OUTPUT_OFF))[4];
	uint32_t u32grpc;

	#ifdef TWO_P_INT
	MACROBLOCK mbs_local1[MBS_LOCAL_I + 1];
	MACROBLOCK * mbs_local;
	#endif
	MACROBLOCK *mb_vld;
	MACROBLOCK *mb_dmc;
	MACROBLOCK *mb_img;
	MACROBLOCK *mb_rgb;
	MACROBLOCK *mb_last;
	#ifdef ENABLE_DEBLOCKING
	MACROBLOCK *mb_dt;
	#define mb_debk mb_img
	#define mb_debk_next mb_dt
	#define mb_img_next mb_dt
	#define mb_dt_next mb_rgb
	#define mb_rgb_next mb_last
	#define mb_yuv mb_dt
	#define mb_yuv_next mb_dt_next
	#else
	#define mb_img_next mb_rgb
	#define mb_dt_next mb_rgb
	#define mb_rgb_next mb_last
	#define mb_yuv mb_img
	#define mb_yuv_next mb_rgb
	#endif
	MACROBLOCK_b mbb[I_FRAME_PIPE];
	MACROBLOCK_b *mbb_vld = &mbb[I_FRAME_PIPE - 1];
	MACROBLOCK_b *mbb_dmc=0;
	MACROBLOCK_b *mbb_img=0;
	MACROBLOCK_b *mbb_rgb=0;
	#ifdef ENABLE_DEBLOCKING
    MACROBLOCK_b *mbb_dt=0;
    #define mbb_debk mbb_img
    #define mbb_img_next mbb_dt
    #define mbb_dt_next mbb_rgb
    #define mbb_yuv mbb_dt
	#else
    #define mbb_img_next mbb_rgb
    #define mbb_dt mbb_img
    #define mbb_dt_next mbb_rgb
    #define mbb_yuv mbb_img
	#endif
	
	uint32_t u32output_mb_end;
	uint32_t u32output_mb_start = 0;
	uint32_t u32output_mb_yend;
	uint32_t u32output_mb_ystart = 0;
	#ifdef ENABLE_DEBLOCKING
    uint32_t u32qcr = 0;
	#endif
    int32_t chn_load_predictor, chn_store_preditor;
    int32_t rgb_pixel_size;

	if ( dec->output_fmt == OUTPUT_FMT_YUV) {
		chn_load_predictor = CHN_LOAD_YUV_PREDITOR;
		chn_store_preditor = CHN_STORE_YUV_PREDITOR;
		rgb_pixel_size = RGB_PIXEL_SIZE_1;
	} else if (dec->output_fmt == OUTPUT_FMT_RGB888) {
		chn_load_predictor = CHN_LOAD_RGB_PREDITOR;
		chn_store_preditor = CHN_STORE_RGB_PREDITOR;
		rgb_pixel_size = RGB_PIXEL_SIZE_4;
	} else if (dec->output_fmt == OUTPUT_FMT_CbYCrY) {
		chn_load_predictor = CHN_LOAD_RGB_PREDITOR;
		chn_store_preditor = CHN_STORE_RGB_PREDITOR;
		rgb_pixel_size = RGB_PIXEL_SIZE_2;
	}
	#ifdef TWO_P_INT
	// align to 8 bytes
	mbs_local = (MACROBLOCK *)(((uint32_t)&mbs_local1[0] + 7) & ~0x7);
	#endif
	// width
	u32output_mb_start = dec->crop_x / PIXEL_Y;
	u32output_mb_end = (dec->crop_w / PIXEL_Y) + u32output_mb_start;
	// height
	u32output_mb_ystart = dec->crop_y / PIXEL_Y;
	u32output_mb_yend = (dec->crop_h / PIXEL_Y) + u32output_mb_ystart;

	///////////////////////////////////////////////////////////////////////////
	// Enable auto-buffering
	ptMP4->VLDCTL = VLDCTL_ABFSTART;
	
	#ifdef ENABLE_DEBLOCKING
    // Debking ping-pong buffer
    ptMP4->MECTL = MECTL_VOPSTART;
	#endif

	#ifdef ENABLE_DEBLOCKING
    if ( dec->output_fmt < OUTPUT_FMT_YUV) {
		u32cmd_mc_reload = MCCTL_FRAME_DT | MCCTL_INTRA | MCCTL_FRAME_DEBK;
    } else {
	    u32cmd_mc_reload = MCCTL_INTRA | MCCTL_FRAME_DEBK;
    }
	#else
	u32cmd_mc_reload = MCCTL_INTRA;
	#endif
	
	if (dec->bMp4_quant) {
		#ifdef ENABLE_DEBLOCKING
	    u32cmd_mc_reload |= MCCTL_MP4Q;
		#else
		u32cmd_mc_reload |= MCCTL_MP4Q ;
		#endif
	}
	
	if (dec->bH263) {
		u32cmd_mc_reload |= MCCTL_SVH;
	}
	u32cmd_mc = u32cmd_mc_reload;

	dma_chain_init_i(dec);
	
	#ifdef ENABLE_DEBLOCKING
	#ifdef ENABLE_DEBLOCKING_SYNC_DEBLOCKING_DONE
	ptDma->GRPS = 
		(0 << (ID_CHN_ACDC * 2))		// sync to VLD_done
	    |(3 << (ID_CHN_ST_DEBK * 2))	// sync to Deblock_done ( the same signal as VLC_done)
		|(1 << (ID_CHN_LD_DEBK * 2));	// sync to DT_done
	#else
	ptDma->GRPS = 
	    (0 << (ID_CHN_ACDC * 2))		// sync to VLD_done
		|(1 << (ID_CHN_LD_DEBK * 2));	// sync to DT_done
	#endif //#ifdef ENABLE_DEBLOCKING_SYNC_DEBLOCKING_DONE
	#else
	  ptDma->GRPS = 0;
	#endif //#ifdef ENABLE_DEBLOCKING	

	// the last command in the chain never skipped.
	u32grpc = (2 << (ID_CHN_1MV*2))		// 1mv dma: disable
		|(2 << (ID_CHN_4MV*2))		// 4mv dma: disable
		|(2 << (ID_CHN_REF_UV*2))	// ref uv dma: disable
		|(2 << (ID_CHN_IMG*2))		// IMG dma: disable
		#ifdef ENABLE_DEBLOCKING
        |(2 << (ID_CHN_ST_DEBK * 2))	// DEB_ST dma: disable
        |(2 << (ID_CHN_LD_DEBK * 2))	// DEB_LD dma: disable
		#endif
		#ifdef TWO_P_INT
		|(2 << (ID_CHN_STORE_MBS * 2))	// disable MBS dma
		|(2 << (ID_CHN_ACDC_MBS * 2))	// disable acdc MBS dma
		#endif
		|(3 << (ID_CHN_ACDC*2));		// St_ACDC dma: sync to VLD_done
										// Ld_ACDC dma: sync to VLD_done

        if ( dec->output_fmt == OUTPUT_FMT_YUV) {
			u32grpc |=	
			#ifdef ENABLE_DEBLOCKING
		         (2 << (ID_CHN_YUV_TOP*2))		// YUV top dma: disable.
				|(2 << (ID_CHN_YUV_MID*2))		// YUV mid dma: disable.
				|(2 << (ID_CHN_YUV_BOT*2));		// YUV bot dma: disable.
			#else
			(2 << (ID_CHN_YUV*2));		// YUV dma: disable.
			#endif
       	} else {
        	u32grpc |=	
			#ifdef ENABLE_DEBLOCKING
            	 (2 << (ID_CHN_RGB_TOP*2))		// RGB top dma: disable.
				|(2 << (ID_CHN_RGB_MID*2))		// RGB mid dma: disable..
				|(2 << (ID_CHN_RGB_BOT*2));		// RGB bot dma: disable.
			#else
				(2 << (ID_CHN_RGB*2));		// RGB dma: disable.
			#endif
         }

										
										
	#ifdef ENABLE_DEBLOCKING
	#ifdef TWO_P_INT
	mb_vld = mb_dmc = mb_img = mb_dt = mb_rgb = mb_last = &mbs_local[0];
	#else
	mb_vld = mb_dmc = mb_img = mb_dt = mb_rgb = mb_last = &dec->mbs[0];
	#endif // #ifdef TWO_P_INT	
	#else
	#ifdef TWO_P_INT
	mb_vld = mb_dmc = mb_img = mb_rgb = mb_last = &mbs_local[0];
	#else
	mb_vld = mb_dmc = mb_img = mb_rgb = mb_last = &dec->mbs[0];
	#endif
	#endif //#ifdef ENABLE_DEBLOCKING
	
	mb_vld->mb_jump = 0;
	mbb_vld->mbpos = -1;
	mbb_vld->x = -1;
	mbb_vld->y = 0;
	mbb_vld->toggle = 1;
	i = -1;
	while (1) {
		#ifdef ENABLE_DEBLOCKING
//(3)goMC_vld------(9)goMC_dmc-----(18)goMC_DeBk---(23)goMC_dt(Conf20de)
        if (u32cmd_mc & (MCCTL_DECGO | MCCTL_DMCGO | MCCTL_DTGO | MCCTL_DEBK_GO)) 
		#else
//(3)goMC_vld-------(9)goMC_dmc--(16)goMC_dt
	    if (u32cmd_mc & (MCCTL_DECGO | MCCTL_DMCGO | MCCTL_DTGO)) 
		#endif
		{
			// swap 2 buffer
			// setup Dezigzag-scan output address and dequant input address
			// VLD output to DZQ[31..16]			DZQ[15..0] is input of DMC
			if (i & BIT0)
				ptMP4->QAR = ((QCOEFF_OFF_1 & 0xFFFF) << 16) | (QCOEFF_OFF_2 & 0xFFFF);
			else
				ptMP4->QAR = ((QCOEFF_OFF_2 & 0xFFFF) << 16) | (QCOEFF_OFF_1 & 0xFFFF);
			
			#ifdef ENABLE_DEBLOCKING
			ptMP4->QCR0 = u32qcr;
			#endif
			ptMP4->MCCTL = u32cmd_mc;
		}
		
		#ifdef ENABLE_DEBLOCKING 
//(0)goDMA_LD--(4)goDMA_ST---------------------(18)goDMA_img---(23')goDMA_ yuv(Conf21de)(R-x)
//							|-----------------------------------------------(29)goDMA_ FRAME(Conf20de)(R-x)
//							|-(11)goDMA_LDD------------------(24)goDMA_STD(4S-x)
		#else	  
//(4)goDMA_LD-----(10)goDMA_ST-----(17)goDMA_img--(23)goDMA_ RGB(R-x)(Conf0)
//							|-----(18)goDMA_yuv(Conf1)
		#endif
		
		// to add delay for debug test ... remember to remove it
	    //{ int delay; for(delay=0;delay<0x10000;delay++); }
	      
		#ifdef ENABLE_DEBLOCKING 
   		u32pipe &=  ~(BIT_DMA_RGB_GO | BIT_4ST);
   		#else
		u32pipe &=  ~BIT_DMA_RGB_GO;
		#endif
		ptDma->GRPC = u32grpc;
		if (mbb_vld->toggle == 0)
			ptDma->CCA = (DMA_DEC_CMD_OFF_0 + CHN_IMG_Y * 4) | DMA_LC_LOC;
		else
			ptDma->CCA = (DMA_DEC_CMD_OFF_1 + CHN_IMG_Y * 4) | DMA_LC_LOC;
		// dont care SMaddr
		// dont care LMaddr
		// dont care BlkWidth
		ptDma->Control =
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DONT_CARE) |
			mDmaSType2b(DONT_CARE) |
			mDmaLType2b(DONT_CARE) |
			mDmaLen12b(0) |
			mDmaID4b(0);
		
		u32grpc = (2 << (ID_CHN_1MV*2))		// 1mv dma: disable
				|(2 << (ID_CHN_4MV*2))		// 4mv dma: disable
				|(2 << (ID_CHN_REF_UV*2))	// ref uv dma: disable
				|(2 << (ID_CHN_IMG*2))		// IMG dma: disable
				#ifdef ENABLE_DEBLOCKING
                |(2 << (ID_CHN_ST_DEBK * 2))	// ST_DEBK dma: disable
				|(2 << (ID_CHN_LD_DEBK * 2))	// LD_DEBK dma: disable
				#endif
				#ifdef TWO_P_INT
				|(2 << (ID_CHN_STORE_MBS * 2))	// disable MBS dma
				|(2 << (ID_CHN_ACDC_MBS * 2))	// disable acdc MBS dma
				#endif
				|(3 << (ID_CHN_ACDC*2));		// St_ACDC dma: sync to VLD_done
											// Ld_ACDC dma: sync to VLD_done
		if (dec->output_fmt == OUTPUT_FMT_YUV) {
			u32grpc |=
				#ifdef ENABLE_DEBLOCKING
   				(2 << (ID_CHN_YUV_TOP*2))		// YUV top dma: disable.
				|(2 << (ID_CHN_YUV_MID*2))		// YUV mid dma: disable.
				|(2 << (ID_CHN_YUV_BOT*2));		// YUV bot dma: disable.
		        #else
				(2 << (ID_CHN_YUV*2));		// YUV dma: disable.
				#endif
		} else {
		    u32grpc |=
				#ifdef ENABLE_DEBLOCKING
   				(2 << (ID_CHN_RGB_TOP*2))		// RGB top dma: disable.
				|(2 << (ID_CHN_RGB_MID*2))		// RGB mid dma: disable.
				|(2 << (ID_CHN_RGB_BOT*2));		// RGB bot dma: disable.
				#else
				(2 << (ID_CHN_RGB*2));		// RGB dma: disable.
				#endif
		}

//~~change to next mb~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///////////////////////////////////////////////////////////////////////////
// update mb pointer
		i ++;
		#ifdef ENABLE_DEBLOCKING
		mbb_rgb = mbb_dt;
		mbb_dt = mbb_img;
		#else
		mbb_rgb = mbb_img;
		#endif
		mbb_img = mbb_dmc;
		mbb_dmc = mbb_vld;
		mbb_vld = &mbb[i%I_FRAME_PIPE];
		mbb_vld->toggle = i & BIT0;
		mb_last = mb_rgb;
		#ifdef ENABLE_DEBLOCKING
		mb_rgb = mb_dt;
		mb_dt = mb_img;
		#else
		mb_rgb = mb_img;
		#endif
		mb_img = mb_dmc;
		mb_dmc = mb_vld;
		if (mbb_vld->toggle == 0)
			dma_cmd_tgl = dma_cmd_tgl0;
		else
			dma_cmd_tgl = dma_cmd_tgl1;

//(11)preMoveImg(1S-2S)
//(12)preMoveYuv(1S-2S)(Conf1)
 		if (u32pipe & BIT_1ST) {
			u32pipe &=  ~BIT_1ST;
			u32pipe |=  BIT_2ST;
			
			#ifdef TWO_P_INT
			u32grpc &= ~( (uint32_t)(3 << (ID_CHN_IMG * 2)) |		// exec IMG dma
						(uint32_t)(3 << (ID_CHN_STORE_MBS * 2)));	// exec MBS dma
			// assign start address of local memory
			dma_cmd_tgl[CHN_STORE_MBS + 1] =
				mDmaLocMemAddr14b(&mbs_local[(i-2) % MBS_LOCAL_I]);
			#else
			u32grpc &= ~(uint32_t)(3 << (ID_CHN_IMG * 2));			// exec IMG dma
			#endif
			
			// update address of destination
			// dont forget to update the next one after the one mb-jumpping(dma ping-pong chain)
			// mb_rgb is the next one after mb_img
			if ((mb_img->mb_jump != 0) || (mb_img_next->mb_jump != 0)) {		
				#ifdef TWO_P_INT
				// mbs
				dma_cmd_tgl[CHN_STORE_MBS] = 
					mDmaSysMemAddr29b(&dec->mbs[mbb_img->mbpos]) |
					mDmaSysInc3b(DMA_INCS_48);		// 2 *  MB_SIZE
				#endif
				
				// y
				dma_cmd_tgl[CHN_IMG_Y] = 
					mDmaSysMemAddr29b((uint32_t) dec->cur.y_phy + (uint32_t)mbb_img->x * 2 * SIZE_U +
					(uint32_t)mbb_img->y * dec->mb_width * SIZE_Y) | mDmaSysInc3b(DMA_INCS_256);		
					// 2 *  SIZE_U
				// u
				dma_cmd_tgl[CHN_IMG_U] =
					mDmaSysMemAddr29b((uint32_t) dec->cur.u_phy + (uint32_t)mbb_img->mbpos * SIZE_U) |
					mDmaSysInc3b(DMA_INCS_128);		// SIZE_U
				// v
				dma_cmd_tgl[CHN_IMG_V] =
					mDmaSysMemAddr29b((uint32_t) dec->cur.v_phy + (uint32_t)mbb_img->mbpos * SIZE_V) |
					mDmaSysInc3b(DMA_INCS_128);		// SIZE_V

			} else if ((mbb_img->x == 0) || (mbb_img->x == 1)) {
				dma_cmd_tgl[CHN_IMG_Y] =
					mDmaSysMemAddr29b((uint32_t) dec->cur.y_phy + (uint32_t) mbb_img->mbpos * SIZE_Y - 
					(uint32_t) mbb_img->x * 2 * SIZE_U) | mDmaSysInc3b(DMA_INCS_256);		// 2 *  SIZE_U
			}
		#ifdef ENABLE_DEBLOCKING	
		} // ????????????????
		#endif
	
    		#ifdef ENABLE_DEBLOCKING
//(18')preMoveFrameYuv(3S)(x-R)(Conf21de)
	   		if ( dec->output_fmt == OUTPUT_FMT_YUV) {
				if (u32pipe & BIT_3ST) {
					if ((mbb_yuv->x >= u32output_mb_start) && (mbb_yuv->x < u32output_mb_end) &&
			    		(mbb_yuv->y >= u32output_mb_ystart) && (mbb_yuv->y < u32output_mb_yend)) {
						u32pipe |=  BIT_DMA_RGB_GO;
						if (mbb_yuv->y == 0)
							u32grpc &= ~(3 << (ID_CHN_YUV_MID * 2));		// Only middle
						else if (mbb_yuv->y == (dec->mb_height- 1))
							u32grpc &= ~((3 << (ID_CHN_YUV_TOP * 2))		// all
								|(3 << (ID_CHN_YUV_MID * 2))
								|(3 << (ID_CHN_YUV_BOT * 2)));
						else
							u32grpc &= ~((3 << (ID_CHN_YUV_TOP * 2))		// only top & middle
								|(3 << (ID_CHN_YUV_MID * 2)));
						
						if (((mb_yuv->mb_jump != 0) || (mb_yuv_next->mb_jump != 0)) ||
							(mbb_yuv->x == u32output_mb_start) || (mbb_yuv->x == (u32output_mb_start + 1))) {
							uint32_t mid_addr;
							// y output
							mid_addr = (uint32_t) dec->output_base_phy +
								((mbb_yuv->x - u32output_mb_start)  +
								(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride) * PIXEL_Y;
							dma_cmd_tgl[CHN_YUV_MID_Y] = 
								mDmaSysMemAddr29b(mid_addr)
								|mDmaSysInc3b(DMA_INCS_32);		// 2 * PIXEL_Y
							dma_cmd_tgl[CHN_YUV_TOP_Y] = 
								mDmaSysMemAddr29b(mid_addr - (PIXEL_Y / 2) * dec->output_stride)
								|mDmaSysInc3b(DMA_INCS_32);		// 2 * PIXEL_Y
							dma_cmd_tgl[CHN_YUV_BOT_Y] = 
								mDmaSysMemAddr29b(mid_addr + (PIXEL_Y / 2) * dec->output_stride)
								|mDmaSysInc3b(DMA_INCS_32);		// 2 * PIXEL_Y
							// u output
							mid_addr = (uint32_t) dec->output_base_u_phy +
								((mbb_yuv->x - u32output_mb_start)+
								(mbb_yuv->y - u32output_mb_ystart)  * dec->output_stride /2) * PIXEL_U;
							dma_cmd_tgl[CHN_YUV_MID_U] =
								mDmaSysMemAddr29b(mid_addr)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_U
							dma_cmd_tgl[CHN_YUV_TOP_U] =
								mDmaSysMemAddr29b(mid_addr - (PIXEL_U / 2) * dec->output_stride / 2)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_U
							dma_cmd_tgl[CHN_YUV_BOT_U] =
								mDmaSysMemAddr29b(mid_addr + (PIXEL_U / 2) * dec->output_stride / 2)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_U
							// v output
							mid_addr = (uint32_t) dec->output_base_v_phy +
								((mbb_yuv->x - u32output_mb_start)+
								(mbb_yuv->y - u32output_mb_ystart)  * dec->output_stride /2) * PIXEL_V;
							dma_cmd_tgl[CHN_YUV_MID_V] =
								mDmaSysMemAddr29b(mid_addr)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_V
							dma_cmd_tgl[CHN_YUV_TOP_V] =
								mDmaSysMemAddr29b(mid_addr - (PIXEL_V / 2) * dec->output_stride / 2)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_V
							dma_cmd_tgl[CHN_YUV_BOT_V] =
								mDmaSysMemAddr29b(mid_addr + (PIXEL_V / 2) * dec->output_stride / 2)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_V
						}
					}
				}
	   		} else {
//(26)preMoveFrameRGB(D)(x-R)(Conf20de)
				if (u32pipe & BIT_PRE_DT) {
					// update address of destination
					u32pipe |=  BIT_DMA_RGB_GO;
					if (mbb_rgb->y == 0)
						u32grpc &= ~(3 << (ID_CHN_RGB_MID * 2));		// Only middle
					else if (mbb_rgb->y == (dec->mb_height- 1))
						u32grpc &= ~((3 << (ID_CHN_RGB_TOP * 2))		// all
							|(3 << (ID_CHN_RGB_MID * 2))
							|(3 << (ID_CHN_RGB_BOT * 2)));
					else
						u32grpc &= ~((3 << (ID_CHN_RGB_TOP * 2))		// only top & middle
							|(3 << (ID_CHN_RGB_MID * 2)));

					// dont forget the next one after the one mb-jumpping
					// mb_last is the next one after mb_rgb
					if ((mb_rgb->mb_jump != 0) || (mb_last->mb_jump != 0) ||
						(mbb_rgb->x == u32output_mb_start) ||
						(mbb_rgb->x == (u32output_mb_start + 1))) {

						uint32_t mid_addr;
						// y output
						if ( dec->output_fmt == OUTPUT_FMT_YUV) {
							mid_addr = (uint32_t) dec->output_base_phy +
								((mbb_rgb->x - u32output_mb_start)  +
								(mbb_rgb->y - u32output_mb_ystart) * dec->output_stride) * PIXEL_Y * RGB_PIXEL_SIZE_1;
							dma_cmd_tgl[CHN_RGB_MID] =
								mDmaSysMemAddr29b(mid_addr) |
								mDmaSysInc3b(DMA_INCS_64);		//PIXEL_Y * 2;
							dma_cmd_tgl[CHN_RGB_TOP] =
								mDmaSysMemAddr29b(mid_addr -
								(PIXEL_Y * RGB_PIXEL_SIZE_1 / 2) * dec->output_stride) |
								mDmaSysInc3b(DMA_INCS_64);		//PIXEL_Y * 2;
							dma_cmd_tgl[CHN_RGB_BOT] =
								mDmaSysMemAddr29b(mid_addr +
								(PIXEL_Y * RGB_PIXEL_SIZE_1 / 2) * dec->output_stride) |
								mDmaSysInc3b(DMA_INCS_64);		//PIXEL_Y * 2;
				} else if ( dec->output_fmt == OUTPUT_FMT_RGB888) {
							mid_addr = (uint32_t) dec->output_base_phy +
								((mbb_rgb->x - u32output_mb_start)  +
								(mbb_rgb->y - u32output_mb_ystart) * dec->output_stride) * PIXEL_Y * RGB_PIXEL_SIZE_4;
							dma_cmd_tgl[CHN_RGB_MID] =
								mDmaSysMemAddr29b(mid_addr) |
								mDmaSysInc3b(DMA_INCS_128);		//PIXEL_Y * 2;
							dma_cmd_tgl[CHN_RGB_TOP] =
								mDmaSysMemAddr29b(mid_addr -
								(PIXEL_Y * RGB_PIXEL_SIZE_4 / 2) * dec->output_stride) |
								mDmaSysInc3b(DMA_INCS_128);		//PIXEL_Y * 2;
							dma_cmd_tgl[CHN_RGB_BOT] =
								mDmaSysMemAddr29b(mid_addr +
								(PIXEL_Y * RGB_PIXEL_SIZE_4 / 2) * dec->output_stride) |
							mDmaSysInc3b(DMA_INCS_128);		//PIXEL_Y * 2;
						} else {
							mid_addr = (uint32_t) dec->output_base_phy +
								((mbb_rgb->x - u32output_mb_start)  +
								(mbb_rgb->y - u32output_mb_ystart) * dec->output_stride) * PIXEL_Y * RGB_PIXEL_SIZE_2;
							dma_cmd_tgl[CHN_RGB_MID] =
								mDmaSysMemAddr29b(mid_addr) |
								mDmaSysInc3b(DMA_INCS_64);		//PIXEL_Y * 2;
							dma_cmd_tgl[CHN_RGB_TOP] =
								mDmaSysMemAddr29b(mid_addr -
								(PIXEL_Y * RGB_PIXEL_SIZE_2 / 2) * dec->output_stride) |
								mDmaSysInc3b(DMA_INCS_64);		//PIXEL_Y * 2;
							dma_cmd_tgl[CHN_RGB_BOT] =
								mDmaSysMemAddr29b(mid_addr +
								(PIXEL_Y * RGB_PIXEL_SIZE_2 / 2) * dec->output_stride) |
								mDmaSysInc3b(DMA_INCS_64);		//PIXEL_Y * 2;
						}
					}
 				}
	   		}
			#else //#ifdef ENABLE_DEBLOCKING
			if ( dec->output_fmt == OUTPUT_FMT_YUV) {
				if ((mbb_yuv->x >= u32output_mb_start) && (mbb_yuv->x < u32output_mb_end) &&
			    	(mbb_yuv->y >= u32output_mb_ystart) && (mbb_yuv->y < u32output_mb_yend)) {
					u32grpc &= ~(uint32_t)(3 << (ID_CHN_YUV * 2));		// exec YUV dma
					if (((mb_yuv->mb_jump != 0) || (mb_yuv_next->mb_jump != 0)) ||
						(mbb_yuv->x == u32output_mb_start) || (mbb_yuv->x == (u32output_mb_start + 1))) {
                        #ifdef ENABLE_DEINTERLACE 
                        if(dec->u32EnableDeinterlace) {
  				    		// Take special note that we implement the deinterlace function based on the
  				    		// following two limitations :
                        	//   - The number of macroblock rows must be even number.
                        	//   - The image width and height must be equal to the display width and height.
  				    		//unsigned int field_selector = (mbb_yuv->y <= (dec->height>>1)/PIXEL_Y)?0:1;
  				    		unsigned int field_selector = (mbb_yuv->y < (dec->mb_height>>1))?0:1;
                        	unsigned int field_mb_y = mbb_yuv->y % (dec->mb_height>>1);
				    		// y output
							dma_cmd_tgl[CHN_YUV_Y] =
								mDmaSysMemAddr29b((uint32_t) dec->output_base_phy + (field_selector*dec->output_stride) +
								((mbb_yuv->x - u32output_mb_start)  +
								//(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride) * PIXEL_Y)
								field_mb_y * (dec->output_stride<<1)) * PIXEL_Y)
								|mDmaSysInc3b(DMA_INCS_32);		// 2 * PIXEL_Y
							// u output
							dma_cmd_tgl[CHN_YUV_U] =
								mDmaSysMemAddr29b((uint32_t) dec->output_base_u_phy + (field_selector*(dec->output_stride>>1)) +
								((mbb_yuv->x - u32output_mb_start)  +
								//(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride/2) * PIXEL_U)
								field_mb_y * dec->output_stride) * PIXEL_U)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_U
							// v output
							dma_cmd_tgl[CHN_YUV_V] =
								mDmaSysMemAddr29b((uint32_t) dec->output_base_v_phy + (field_selector*(dec->output_stride>>1)) +
								((mbb_yuv->x - u32output_mb_start)  +
								//(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride/2) * PIXEL_V)
								field_mb_y * dec->output_stride) * PIXEL_V)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_V
                        } else {
                           	// y output
							dma_cmd_tgl[CHN_YUV_Y] =
								mDmaSysMemAddr29b((uint32_t) dec->output_base_phy +
								((mbb_yuv->x - u32output_mb_start)  +
								(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride) * PIXEL_Y)
								|mDmaSysInc3b(DMA_INCS_32);		// 2 * PIXEL_Y
							// u output
							dma_cmd_tgl[CHN_YUV_U] =
								mDmaSysMemAddr29b((uint32_t) dec->output_base_u_phy +
								((mbb_yuv->x - u32output_mb_start)+
								(mbb_yuv->y - u32output_mb_ystart)  * dec->output_stride /2) * PIXEL_U)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_U
							// v output
							dma_cmd_tgl[CHN_YUV_V] =
								mDmaSysMemAddr29b((uint32_t) dec->output_base_v_phy +
								((mbb_yuv->x - u32output_mb_start) +
								(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride /2) * PIXEL_V)
								|mDmaSysInc3b(DMA_INCS_16);
							
                        }
						#else //#ifdef ENABLE_DEINTERLACE
						// y output
						dma_cmd_tgl[CHN_YUV_Y] =
							mDmaSysMemAddr29b((uint32_t) dec->output_base_phy +
							((mbb_yuv->x - u32output_mb_start)  +
							(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride) * PIXEL_Y)
							|mDmaSysInc3b(DMA_INCS_32);		// 2 * PIXEL_Y
						// u output
						dma_cmd_tgl[CHN_YUV_U] =
							mDmaSysMemAddr29b((uint32_t) dec->output_base_u_phy +
							((mbb_yuv->x - u32output_mb_start)+
							(mbb_yuv->y - u32output_mb_ystart)  * dec->output_stride /2) * PIXEL_U)
							|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_U
						// v output
						dma_cmd_tgl[CHN_YUV_V] =
							mDmaSysMemAddr29b((uint32_t) dec->output_base_v_phy +
							((mbb_yuv->x - u32output_mb_start) +
							(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride /2) * PIXEL_V)
							|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_V
				   		#endif //#ifdef ENABLE_DEINTERLACE
					}
				}	
			}
		}
		#endif //#ifdef ENABLE_DEBLOCKING
		
  		#ifndef ENABLE_DEBLOCKING
//(19)preMoveRGB(3S-R)(D)(Conf0)
//(19')(Conf1)(3S-x)
 		if (u32pipe & BIT_3ST) {
 			u32pipe &= ~BIT_3ST;
			if ( dec->output_fmt < OUTPUT_FMT_YUV) {
			    // update address of destination
			    // dont forget the next one after the one mb-jumpping
			    // mb_last is the next one after mb_rgb
				if ((mbb_rgb->x >= u32output_mb_start) && (mbb_rgb->x < u32output_mb_end) &&
						(mbb_rgb->y >= u32output_mb_ystart) && (mbb_rgb->y < u32output_mb_yend)) {
					u32pipe |=  BIT_DMA_RGB_GO;
				    if ((mb_rgb->mb_jump != 0) || (mb_last->mb_jump != 0) ||
							(mbb_rgb->x == u32output_mb_start) || (mbb_rgb->x == (u32output_mb_start + 1))) {
						if ( dec->output_fmt  == OUTPUT_FMT_RGB888) {
							dma_cmd_tgl[CHN_RGB] =
								mDmaSysMemAddr29b((uint32_t) dec->output_base_phy +
								(mbb_rgb->x - u32output_mb_start) * PIXEL_Y * RGB_PIXEL_SIZE_4 +
								(mbb_rgb->y - u32output_mb_ystart) * dec->output_stride * PIXEL_Y * RGB_PIXEL_SIZE_4)
								 | mDmaSysInc3b(DMA_INCS_128);
						
						} else {
							dma_cmd_tgl[CHN_RGB] =
								mDmaSysMemAddr29b((uint32_t) dec->output_base_phy +
								(mbb_rgb->x - u32output_mb_start) * PIXEL_Y * RGB_PIXEL_SIZE_2 +
								(mbb_rgb->y - u32output_mb_ystart) * dec->output_stride * PIXEL_Y * RGB_PIXEL_SIZE_2)
								 | mDmaSysInc3b(DMA_INCS_64);		//PIXEL_Y * 2;
						}
					}
				    u32grpc &= ~(uint32_t)(3 << (ID_CHN_RGB * 2));		// exec RGB dma
//	 		    if ((u32pipe & BIT_PRE_DT) == 0)
//					u32grpc |= 1 << (ID_CHN_RGB*2);				// RGB dma: skip but inscr.
			    }
		 	}
 		}
  		#endif // #ifndef ENABLE_DEBLOCKING
	        
//(6)wait VLD(0S-1S)
		if (u32pipe & BIT_0ST) {
			u32pipe &= ~ BIT_0ST;
			u32pipe |=  BIT_1ST;
			vpe_prob_vld_start();
			mFa526DrainWrBuf();
			while (((u32temp = ptMP4->VLDSTS) & BIT0) == 0)
				;
			vpe_prob_vld_end();
			// check error code
			if (u32temp  & 0xF000) {
				mVpe_Indicator(0x92000000 | (u32temp  & 0xF000));
				mVpe_FAIL(1, u32temp);
				//return; //tckuo
			 	//waitDMC & waitDT
				while ((ptMP4->CPSTS & (BIT14 | BIT1)) != (BIT14 | BIT1))
					;
				u32temp = FindRsmkOrVopS(bs, ptMP4);
				break;
			}
			// Resync marker detected
			if (u32temp & BIT1) {
				if (bRead_video_packet_header(dec, &bound) == -1) {
					mVpe_FAIL(2, u32temp);
					return 0;//ivan
					break;
				}
				if (bound != mbb_dmc->mbpos) {
					// correct 'dmc'ed(img) jump to correct next-'dmc'ed(dmc)
					mb_img->mb_jump = bound - mbb_dmc->mbpos;
					#ifndef TWO_P_INT
					// correct 'vld'ed(dmc) information
					mb_dmc = &dec->mbs[bound];
					#endif
					mb_dmc->mb_jump = 0;
					mbb_dmc->y = (int32_t)(bound / dec->mb_width);
					//mbb_dmc->x = bound % dec->mb_width;
					mbb_dmc->x = bound - mbb_dmc->y * dec->mb_width;
					mbb_dmc->mbpos = bound;
					
					// correct store acdc pointer
					// dont care about 'load preditor', it will not be used until next line is reached
					dma_cmd_tgl[chn_store_preditor] =
							mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy + 64 * mbb_vld->x)
							| mDmaSysInc3b(DMA_INCS_128);
							
					if (mbb_vld->toggle == 0)
						dma_cmd_tgl1[chn_store_preditor] =
								mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy + 64 * (mbb_vld->x + 1))
								| mDmaSysInc3b(DMA_INCS_128);
					else
						dma_cmd_tgl0[chn_store_preditor] =
								mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy + 64 * (mbb_vld->x + 1))
								| mDmaSysInc3b(DMA_INCS_128);
				}
				mb_cnt_in_vp = (u32temp >> 16) - 1;
				//pu32table = &Table_Output[4];
				pu32table = &((uint32_t *)( Mp4Base (dec) + TABLE_OUTPUT_OFF))[4];
			}
			mbb_dmc->cbp = (*pu32table >> 23) & 0x3F;
			mbb_dmc->quant = (ptMP4->VOP0 >> 8) & 0xFF;
			if (dec->data_partitioned)
				pu32table += 4;
		}

		mbb_vld->mbpos = mbb_dmc->mbpos + 1;
		mbb_vld->x = mbb_dmc->x + 1;
		mbb_vld->y = mbb_dmc->y;
		if (mbb_vld->x == dec->mb_width) {
			mbb_vld->x = 0;
			mbb_vld->y ++;
		}
		// indicator
		mVpe_Indicator(0x91000000 | (mbb_vld->y << 12) | mbb_vld->x);


//(1)preVLD(x-0S)
		if (mbb_vld->y < dec->mb_height) {
			u32pipe |= BIT_0ST;
			// init mb_vld
			#ifdef TWO_P_INT
			mb_vld = &mbs_local[i%MBS_LOCAL_I];
			#else
			mb_vld = &dec->mbs[mbb_vld->mbpos];
			#endif
			mb_vld->mb_jump = 0;
			// get acdcPredict command parameter
			u32cmd_mc = predict_acdc_I(mbb_vld, (int32_t)dec->mb_width, bound);
			u32cmd_mc |= u32cmd_mc_reload;
		} else {
			u32cmd_mc = u32cmd_mc_reload;
		}
		#ifdef ENABLE_DEBLOCKING
//(19)preStoreDeBk(3S)
		if (u32pipe & BIT_3ST) {
			// only 0 ~ (mb_height-2) MB_row need to store its deblocking bottom MB
			if (mbb_dt->y < (dec->mb_height - 1)) {
			  	#ifdef ENABLE_DEBLOCKING_SYNC_DEBLOCKING_DONE
			    u32grpc |=  (3 << (ID_CHN_ST_DEBK * 2));		// exec sync to Deblock_DONE
			  	#else
				u32grpc &= ~(3 << (ID_CHN_ST_DEBK * 2));		// exec STORE_DEBLOCK
			  	#endif
				if ((mb_dt->mb_jump != 0) || 
					(mb_dt_next->mb_jump != 0) ||
					(mbb_dt->x == 0) || 
					(mbb_dt->x == 1)) {
					if ( dec->output_fmt == OUTPUT_FMT_YUV) {
						dma_cmd_tgl[CHN_STORE_YUV_DEBK] =
							mDmaSysMemAddr29b((uint32_t)dec->pu8DeBlock_phy + 256 * mbb_dt->x)
							| mDmaSysInc3b(DMA_INCS_512);
					} else {
						dma_cmd_tgl[CHN_STORE_RGB_DEBK] =
							mDmaSysMemAddr29b((uint32_t)dec->pu8DeBlock_phy + 256 * mbb_dt->x)
							| mDmaSysInc3b(DMA_INCS_512);
					}
				}
			}
		}
//(6)preLoadDeBk(1S)
		if (u32pipe & BIT_1ST) {
			// only 1 ~ (mb_height-1) MB_row need to load its deblocking top MB
			if (mbb_dmc->y > 0) {
				u32grpc |= 3 << (ID_CHN_LD_DEBK * 2);	// exec sync to DT_DONE
				if ((mb_dmc->mb_jump != 0) || (mb_debk->mb_jump != 0) ||
					(mbb_dmc->x == 0) || (mbb_dmc->x == 1)) {
					if ( dec->output_fmt == OUTPUT_FMT_YUV) {
						dma_cmd_tgl[CHN_LOAD_YUV_DEBK] =
							mDmaSysMemAddr29b((uint32_t)dec->pu8DeBlock_phy + 256 * mbb_dmc->x)
							| mDmaSysInc3b(DMA_INCS_512);
					} else {
						dma_cmd_tgl[CHN_LOAD_RGB_DEBK] =
							mDmaSysMemAddr29b((uint32_t)dec->pu8DeBlock_phy + 256 * mbb_dmc->x)
							| mDmaSysInc3b(DMA_INCS_512);

					}
				}
			}
		}
		#endif

//(7)preStoreACDC
//(2)preLoadACDC
		if ((mb_img->mb_jump != 0) || (mb_dmc->mb_jump != 0) ||(mbb_vld->x == 0)) {
			dma_cmd_tgl[chn_store_preditor] =
					mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy)
					| mDmaSysInc3b(DMA_INCS_128);
			dma_cmd_tgl[chn_load_predictor] =
					mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy + 64)
					| mDmaSysInc3b(DMA_INCS_128);
		} else if (mbb_vld->x == 1) {
			dma_cmd_tgl[chn_store_preditor] =
					mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy + 64)
					| mDmaSysInc3b(DMA_INCS_128);
		}

		if (mbb_vld->x == (dec->mb_width - 1)) {
				dma_cmd_tgl[chn_load_predictor] =
					mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy)
					| mDmaSysInc3b(DMA_INCS_128);
		}
		

//(7)waitDMA_load--(13)waitDMA_store----------------(20)waitDMA_img-(24)waitDMA_RGB(Conf0)
//										|-------(21)waitDMA_yuv(Conf1)
		vpe_prob_dma_start();
		mFa526DrainWrBuf();
		while((ptDma->Status & BIT0) == 0)
			;
		vpe_prob_dma_end();
		#ifdef ENABLE_DEBLOCKING		
//(28)waitDT(D-x)(Conf20de)
		if ( dec->output_fmt != OUTPUT_FMT_YUV) {
 			if (u32pipe & BIT_PRE_DT) {
				u32pipe &=  ~BIT_PRE_DT;
				vpe_prob_dt_start();
				mFa526DrainWrBuf();
				while((ptMP4->CPSTS & BIT14) == 0)
					;
				vpe_prob_dt_end();
			}
		}
//(21)preDT(3S)(x-D)(Conf20de)
//(22)wait DeBk(3S-4S)
 		if (u32pipe & BIT_3ST) {
			u32pipe &= ~BIT_3ST;
 			u32pipe |= BIT_4ST;
			if ( dec->output_fmt != OUTPUT_FMT_YUV)
			    if ((mbb_dt->x >= u32output_mb_start)
				&& (mbb_dt->x < u32output_mb_end)
				&& (mbb_dt->y >= u32output_mb_ystart)
				&& (mbb_dt->y < u32output_mb_yend)) {
				u32pipe |= BIT_PRE_DT;
				// pls refer to pipeline arrangement of dma_b.c
				if (mbb_vld->toggle == 0) {
					// dont care YUV2RGB source addr, DeBk module will handle the p-p
					if ( dec->output_fmt == OUTPUT_FMT_RGB888) 
						ptMP4->MCCADDR = BUFFER_RGB_OFF_0(RGB_PIXEL_SIZE_4);// YUV2RGB dest. addr, [10: 7] valid
					else
						ptMP4->MCCADDR = BUFFER_RGB_OFF_0(RGB_PIXEL_SIZE_2);
						
				} else {
					// dont care YUV2RGB source addr, DeBk module will handle the p-p
					if ( dec->output_fmt == OUTPUT_FMT_RGB888) 
						ptMP4->MCCADDR = BUFFER_RGB_OFF_1(RGB_PIXEL_SIZE_4);// YUV2RGB dest. addr, [10: 7] valid
					else
						ptMP4->MCCADDR = BUFFER_RGB_OFF_1(RGB_PIXEL_SIZE_2);// YUV2RGB dest. addr, [10: 7] valid
				}
				if (mbb_dt->y == (dec->mb_height - 1))
					u32cmd_mc |= MCCTL_DT_BOT;
				u32cmd_mc |= MCCTL_DTGO;
			    }
 			}
			mFa526DrainWrBuf();
			while ((ptMP4->CPSTS & BIT16) == 0)
				;
 		}
 		
		#else //#ifdef ENABLE_DEBLOCKING

//(22)waitDT(D-x)(Conf0)
	    if ( dec->output_fmt < OUTPUT_FMT_YUV) {
     	    if (u32pipe & BIT_PRE_DT) {
				u32pipe &=  ~BIT_PRE_DT;
				vpe_prob_dt_start();
				mFa526DrainWrBuf();
				while((ptMP4->CPSTS & BIT14) == 0)
					;
				vpe_prob_dt_end();
		    }
//(14)preDT(2S-D)(Conf0)
     		if (u32pipe & BIT_2ST) {
				if ((mbb_dt->x >= u32output_mb_start)
				&& (mbb_dt->x < u32output_mb_end)
				&& (mbb_dt->y >= u32output_mb_ystart)
				&& (mbb_dt->y < u32output_mb_yend)) {
					u32pipe |= BIT_PRE_DT;
					// pls refer to pipeline arrangement of dma_b.c
					if (mbb_vld->toggle == 0) {
						ptMP4->MECADDR = INTER_Y_OFF_0;		// YUV2RGB source addr, [10: 7] valid
						if ( dec->output_fmt == OUTPUT_FMT_RGB888) 
							ptMP4->MCCADDR = BUFFER_RGB_OFF_0(RGB_PIXEL_SIZE_4);// YUV2RGB dest. addr, [10: 7] valid
						else
							ptMP4->MCCADDR = BUFFER_RGB_OFF_0(RGB_PIXEL_SIZE_2);
					} else {
						ptMP4->MECADDR = INTER_Y_OFF_1;		// YUV2RGB source addr, [10: 7] valid
						if ( dec->output_fmt == OUTPUT_FMT_RGB888) 
							ptMP4->MCCADDR = BUFFER_RGB_OFF_1(RGB_PIXEL_SIZE_4);// YUV2RGB dest. addr, [10: 7] valid
						else
							ptMP4->MCCADDR = BUFFER_RGB_OFF_1(RGB_PIXEL_SIZE_2);
					}	
					u32cmd_mc |= MCCTL_DTGO;
				}
 		    }
      	} 
		#endif //#ifdef ENABLE_DEBLOCKING
//(15) wait DMC(2S-3S)
		if (u32pipe & BIT_2ST) {
		  	#ifndef ENABLE_DEBLOCKING
			u32pipe &= ~BIT_2ST;
			u32pipe |= BIT_3ST;
		  	#endif
			vpe_prob_dmc_start();
			mFa526DrainWrBuf();
			while ((ptMP4->CPSTS & BIT1) == 0)
				;
			vpe_prob_dmc_end();
		}
		
 	  	#ifdef ENABLE_DEBLOCKING		
		u32qcr = 0;
//(16)preDeBk(2S-3S)
		if (u32pipe & BIT_2ST) {
			u32pipe &= ~BIT_2ST;
			u32pipe |= BIT_3ST;
			u32cmd_mc |= MCCTL_DEBK_GO;
			if (mbb_debk->x == 0)
				u32cmd_mc |= MCCTL_DEBK_LEFT;
			if (mbb_debk->y == 0)
				u32cmd_mc |= MCCTL_DEBK_TOP;
			// pls refer to pipeline arrangement of dma_b.c
			if (mbb_vld->toggle == 0) {
				ptMP4->MECADDR = INTER_Y_OFF_0;		// DeBk source addr, [10: 7] valid
				// dont care DeBk dest. addr, DeBk module will handle the p-p
			} else {
				ptMP4->MECADDR = INTER_Y_OFF_1;		// DeBk source addr, [10: 7] valid
				// dont care DeBk dest. addr, DeBk module will handle the p-p
			}
			u32qcr = mbb_debk->quant << 25;
		}
	  	#endif
		
//(8)preDMC(1S)
		if (u32pipe & BIT_1ST) {
			if (mbb_vld->toggle == 0)
				// dmc_input(& output) direct to INTER_Y_OFF_1
				ptMP4->MCIADDR = INTER_Y_OFF_1;
			else
				// dmc_input(& output) direct to INTER_Y_OFF_0
				ptMP4->MCIADDR = INTER_Y_OFF_0;
			u32cmd_mc |= MCCTL_DMCGO;
			// fill CBP in this stage
		  	#ifdef ENABLE_DEBLOCKING
			u32qcr |= (mbb_dmc->quant << 18) | mbb_dmc->cbp;
		  	#else
			ptMP4->QCR0 = (mbb_dmc->quant << 18) | mbb_dmc->cbp;
		  	#endif
		}
		
		if (u32pipe == 0)
			break;
	}
	
	for (; (ptMP4->VLDSTS & BIT10)==0; ) ;			/// wait for autobuffer clean
	// stop auto-buffering
	ptMP4->VLDCTL = VLDCTL_ABFSTOP;
	return 0;
}

/*

LD:	Load ACDC predictor
ST:	Store ACDC predictor
R:	move Reference image
I:	move reconstructed image
Y:	move display YUV frame
RGB:move display RGB frame
V:	goVLD
MC:	goDMC
DT:	goDT


vld_toggle	|	load		|	goVLD	|	ref	|	goDMC	|	goDT	|	rgb	|
			|			|	store	|		|			|	img		|		|
			|			|			|		|			|	yuv		|		|
============================================================
1			|	LD0
0			|	LD1		|	V0(ST)
1			|	LD0		|	V1(ST)	|	R0
0			|	LD1		|	V0(ST)	|	R1	|	MC0
1			|	LD0		|	V1(ST)	|	R0	|	MC1		|	DT0(IY0)
0			|	LD1		|	V0(ST)	|	R1	|	MC0		|	DT1(IY1)	|	RGB0
1			|	LD0		|	V1(ST)	|	R0	|	MC1		|	DT0(IY0)	|	RGB1
============================================================

*/


//R: BIT_DMA_RGB_GO

/*
			<-  mb_vld	-><- mb_ref      -><- mb_dmc  -><- mb_img     -><- mb_rgb    ->
													    <- mb_dt    ->
													    <- mb_yuv  ->
<-    0st     -><-  1st stage	-><- 2nd stage  -><- 3nd stage -><- 4nd stage  -><- 5nd stage ->
----------------------------cycle start --------------------------------------------------
			(3)goMC_vld-----------------(15)goMC_dmc--(23)goMC_dt
			(4)goDMA_LD_ST-(12)goDMA_ref---------------(24)goDMA_img-(30)goDMA_rgb(R-x)(Conf0)
												|----(25)goDMA_yuv(Conf1)
									     (16)prePXI(3S)
			(5)goME_pmv----------------(17)goME_pxi
************************************************************************** change to next mb *
									      (18)preMoveImg(3S)
									      (19)preMoveYuv(3S)(Conf1)
													  (26)preMoveRGB(4S-R)(D)(Conf0)
													  (26')Conf1(4S-x)
			(6)waitME_PMV(0S)
			(7)storeMV(0S)
			(8)preMoveRef(0S-1S)
(1)preVLD(x-0S)
			(9)preStoreACDC
(2)preLoadACDC
										(20)waitDMC(3S-4S)
							(13)preDMC(2S-3S)
 													  (27)waitDT(D-x)(Conf0)
 										(21)preDT(4S)(x-D)(Conf0)
			(10)waitVLD(1S-2S)
										(22)waitME_pxi(4S)
			(11)waitDMA_LD_ST-(14)waitDMA_ref------------(28)waitDMA_img--(31)waitDMA_rgb(Conf0)
												|-----(29)waitDMA_yuv(Conf1)

----------------------------cycle end --------------------------------------------------
*/

int
decoder_pframe(DECODER * dec,
			   Bitstream * bs,
			   int rounding)
{
	volatile MP4_t * ptMP4 = (MP4_t *)(Mp4Base (dec) + MP4_OFF);
	volatile MDMA * ptDma = (MDMA *)(Mp4Base (dec) + DMA_OFF);
	uint32_t * dma_cmd_tgl0 = (uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_0);
	uint32_t * dma_cmd_tgl1 = (uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_1);
	uint32_t * dma_cmd_tgl;
	uint32_t u32temp;
	uint32_t u32pipe = 0;
	uint32_t u32cmd_mc = 0;
	uint32_t u32cmd_mc_reload = 0;
	uint32_t u32cmd_me = 0;
	uint32_t mb_cnt_in_vp = 0;			// MB count in video packet
	uint32_t mbpos_in_vp = 0;			// MB pos in video packet
	uint32_t * pu32table = &((uint32_t *)( Mp4Base (dec) + TABLE_OUTPUT_OFF))[4];
	int32_t i;
	int32_t bound = 0;
	uint32_t u32ErrorCount = 0;
	uint32_t u32grpc;

	#ifdef TWO_P_INT
	MACROBLOCK mbs_local1[MBS_LOCAL_P+1];	// 
	MACROBLOCK * mbs_local;
	MACROBLOCK mbs_ACDC1[MBS_ACDC + 1];
	MACROBLOCK * mbs_ACDC;
	#endif
	MACROBLOCK *mb_vld;
	MACROBLOCK *mb_ref;
	MACROBLOCK *mb_dmc;
	MACROBLOCK *mb_img;
  	#ifdef ENABLE_DEBLOCKING
    MACROBLOCK *mb_dt;
  	#endif
	MACROBLOCK *mb_rgb;
	MACROBLOCK *mb_last;
  	#ifdef ENABLE_DEBLOCKING
	#define mb_debk mb_img
	#define mb_debk_next mb_dt
	#define mb_img_next mb_dt
	#define mb_dt_next mb_rgb
	#define mb_rgb_next mb_last
	#define mb_yuv mb_dt
	#define mb_yuv_next mb_dt_next
	#ifdef TWO_P_INT
	#define mb_dmc_next mb_img
	#endif
  	#else
	#define mb_img_next mb_rgb
	#define mb_dmc_next mb_img
	#define mb_dt mb_img
	#define mb_dt_next mb_rgb
	#define mb_rgb_next mb_last
	#define mb_yuv mb_img
	#define mb_yuv_next mb_rgb
  	#endif
	MACROBLOCK_b mbb[P_FRAME_PIPE];
	MACROBLOCK_b *mbb_vld = &mbb[P_FRAME_PIPE - 1];
	MACROBLOCK_b *mbb_ref=0;
	MACROBLOCK_b *mbb_dmc=0;
	MACROBLOCK_b *mbb_img=0;
  	#ifdef ENABLE_DEBLOCKING
	MACROBLOCK_b *mbb_dt=0;
  	#endif
	MACROBLOCK_b *mbb_rgb=0;
  	#ifdef ENABLE_DEBLOCKING
	#define mbb_debk mbb_img
	#define mbb_img_next mbb_dt
	#define mbb_dt_next mbb_rgb
	#define mbb_yuv mbb_dt
  	#else
	#define mbb_img_next mbb_rgb
	#define mbb_dt mbb_img
	#define mbb_dt_next mbb_rgb
	#define mbb_yuv mbb_img
  	#endif
	uint32_t u32output_mb_end;
	uint32_t u32output_mb_start = 0;
	uint32_t u32output_mb_yend;
	uint32_t u32output_mb_ystart = 0;
	int32_t chn_load_predictor, chn_store_predictor;
	int32_t rgb_pixel_size;

	if (dec->output_fmt == OUTPUT_FMT_YUV)
		rgb_pixel_size = RGB_PIXEL_SIZE_1;
	else if ( dec->output_fmt == OUTPUT_FMT_RGB888)
		rgb_pixel_size = RGB_PIXEL_SIZE_4;
	else
		rgb_pixel_size = RGB_PIXEL_SIZE_2;
  	#ifdef ENABLE_DEBLOCKING
	uint32_t u32qcr = 0;
	// init quant value of the 1st MB as vop quant, prevent 1st MB is marked as NOT_CODED
	//mbb_vld->quant = quant;
	mbb_vld->quant = (ptMP4->VOP0 >> 8) & 0xFF;
  	#endif
    if ( dec->output_fmt == OUTPUT_FMT_YUV) {
		chn_load_predictor = CHN_LOAD_YUV_PREDITOR;
		chn_store_predictor =CHN_STORE_YUV_PREDITOR;
		rgb_pixel_size = RGB_PIXEL_SIZE_1;
	} else if ( dec->output_fmt == OUTPUT_FMT_RGB888) {
		chn_load_predictor = CHN_LOAD_RGB_PREDITOR;
		chn_store_predictor= CHN_STORE_RGB_PREDITOR;
		rgb_pixel_size = RGB_PIXEL_SIZE_4;
	} else {
		chn_load_predictor = CHN_LOAD_RGB_PREDITOR;
		chn_store_predictor= CHN_STORE_RGB_PREDITOR;
		rgb_pixel_size = RGB_PIXEL_SIZE_2;
	}
	#ifdef TWO_P_INT
	// align to 8 bytes
	mbs_ACDC = (MACROBLOCK *)(((uint32_t)&mbs_ACDC1[0] + 7) & ~0x7);
	mbs_local = (MACROBLOCK *)(((uint32_t)&mbs_local1[0] + 7) & ~0x7);
	#endif
	// width
	u32output_mb_start = dec->crop_x / PIXEL_Y;
	u32output_mb_end = (dec->crop_w / PIXEL_Y) + u32output_mb_start;
	// height
	u32output_mb_ystart = dec->crop_y / PIXEL_Y;
	u32output_mb_yend = (dec->crop_h / PIXEL_Y) + u32output_mb_ystart;
	///////////////////////////////////////////////////////////////////////////
	// Enable auto-buffering
	ptMP4->VLDCTL = VLDCTL_ABFSTART;
	
	ptMP4->HOFFSET = 0;

	// reset PMV counter
  	#ifdef ENABLE_DEBLOCKING
	// set Debking ping-pong buffer from buffer1 (pls refer to dma_b.c)
	ptMP4->MECTL = MECTL_VOPSTART | MECTL_DEBLOCK_PP1;
  	#else
	ptMP4->MECTL = MECTL_VOPSTART;
  	#endif
  
  	#ifdef ENABLE_DEBLOCKING  
  	// init mc & me command
    if (dec->output_fmt < OUTPUT_FMT_YUV)
	  u32cmd_mc_reload = MCCTL_FRAME_DT | MCCTL_FRAME_DEBK;
    else 
	  u32cmd_mc_reload = MCCTL_FRAME_DEBK;
  	#endif

	// init mc & me command
	if (dec->bMp4_quant) {
	  	#ifdef ENABLE_DEBLOCKING  
	  	u32cmd_mc_reload |= MCCTL_MP4Q;
	  	#else
		u32cmd_mc_reload = MCCTL_MP4Q;
	  	#endif
	}
	
	if (dec->bH263)
		u32cmd_mc_reload |= MCCTL_SVH;

	dma_chain_init_p(dec);

  	#ifdef ENABLE_DEBLOCKING
    	#ifdef ENABLE_DEBLOCKING_SYNC_DEBLOCKING_DONE
      	ptDma->GRPS = 
	         (0 << (ID_CHN_ACDC * 2))		// sync to VLD_done
	        |(3 << (ID_CHN_ST_DEBK * 2))	// sync to Deblock_done ( the same signal as VLC_done)
			|(1 << (ID_CHN_LD_DEBK * 2));	// sync to DT_done	  
    	#else
   	  	ptDma->GRPS = 
			(0 << (ID_CHN_ACDC * 2))		// sync to VLD_done
			|(1 << (ID_CHN_LD_DEBK * 2));	// sync to DT_done
		#endif
  	#else
		ptDma->GRPS = 0;
  	#endif
	
	// the last command in the chain never skipped.
	u32grpc = (2 << (ID_CHN_1MV*2))		// disable 1mv
			|(2 << (ID_CHN_4MV*2))		// disable 4mv
			|(2 << (ID_CHN_REF_UV*2))	// ref uv dma: disable
			|(2 << (ID_CHN_IMG*2))		// disable IMG
		  #ifdef ENABLE_DEBLOCKING
			|(2 << (ID_CHN_ST_DEBK * 2))	// DEB_ST dma: disable
			|(2 << (ID_CHN_LD_DEBK * 2))	// DEB_LD dma: disable
		  #endif
		  #ifdef TWO_P_INT
			|(2 << (ID_CHN_STORE_MBS * 2))	// disable MBS dma
			|(2 << (ID_CHN_ACDC_MBS * 2))	// disable acdc MBS dma
		  #endif
			|(3 << (ID_CHN_ACDC*2));		// LdSt_ACDC sync to VLD_done
										// load & store non-meaning
	 if (dec->output_fmt == OUTPUT_FMT_YUV) {
	 	  u32grpc |=
		  #ifdef ENABLE_DEBLOCKING
  			(2 << (ID_CHN_YUV_TOP*2))		// YUV top dma: disable.
			|(2 << (ID_CHN_YUV_MID*2))		// YUV mid dma: disable.
			|(2 << (ID_CHN_YUV_BOT*2));		// YUV bot dma: disable.
		  #else
			(2 << (ID_CHN_YUV*2));		         // YUV dma: disable.
		  #endif
	 } else {
	      u32grpc |=
	 	  #ifdef ENABLE_DEBLOCKING
 			(2 << (ID_CHN_RGB_TOP*2))		// RGB top dma: disable.
			|(2 << (ID_CHN_RGB_MID*2))		// RGB mid dma: disable..
			|(2 << (ID_CHN_RGB_BOT*2));		// RGB bot dma: disable.
		  #else
			(2 << (ID_CHN_RGB*2));		// RGB dma: disable.
		  #endif
	}									
  	#ifdef ENABLE_DEBLOCKING 
	{
  		#ifdef TWO_P_INT
		mb_vld = mb_ref = mb_dmc = mb_img = mb_dt = mb_rgb = mb_last = &mbs_local[0];
		#else
		mb_vld = mb_ref = mb_dmc = mb_img = mb_dt = mb_rgb = mb_last = &dec->mbs[0];
		#endif
  	}
  	#else
  	{
		#ifdef TWO_P_INT
		mb_vld = mb_ref = mb_dmc = mb_img = mb_rgb = mb_last = &mbs_local[0];
		#else
		mb_vld = mb_ref = mb_dmc = mb_img = mb_rgb = mb_last = &dec->mbs[0];
		#endif
  	}
  	#endif
	mb_vld->mb_jump = 0;
	mb_ref->mode = MODE_INTER;	// for simulation issue

	mbb_vld->mbpos = -1;
	mbb_vld->x = -1;
	mbb_vld->y = 0;
	mbb_vld->toggle = 1;
	i = -1;
	while (1) {
		
//(3)goMC_vld-----------------(15)goMC_dmc--(23)goMC_dt
      	#ifdef ENABLE_DEBLOCKING
   		if (u32cmd_mc & (MCCTL_DECGO | MCCTL_DMCGO | MCCTL_DTGO | MCCTL_DEBK_GO)) {
	        ptMP4->QCR0 = u32qcr;
	        ptMP4->MCCTL = u32cmd_mc;
	    }
	  	#else
		if (u32cmd_mc & (MCCTL_DECGO | MCCTL_DMCGO | MCCTL_DTGO)) {
			ptMP4->MCCTL = u32cmd_mc;
		}
      	#endif

//(4)goDMA_LD_ST-(12)goDMA_ref---------------(24)goDMA_img-(30)goDMA_rgb(Conf0)
//										|--(25)goDMA_yuv(Conf1)
		#ifdef ENABLE_DEBLOCKING
   		u32pipe &=  ~(BIT_DMA_RGB_GO | BIT_5ST);
   		#else
		u32pipe &=  ~BIT_DMA_RGB_GO;
		#endif
		  		  
		ptDma->GRPC = u32grpc;
		if (mb_ref->mode == MODE_INTER4V) {
			if (mbb_vld->toggle == 0)
				ptDma->CCA = (DMA_DEC_CMD_OFF_0 + CHN_REF_4MV_Y0 * 4) | DMA_LC_LOC;
			else
				ptDma->CCA = (DMA_DEC_CMD_OFF_1 + CHN_REF_4MV_Y0 * 4) | DMA_LC_LOC;
		} else {
			if (mbb_vld->toggle == 0)
				ptDma->CCA = (DMA_DEC_CMD_OFF_0 + CHN_REF_1MV_Y * 4) | DMA_LC_LOC;
			else
				ptDma->CCA = (DMA_DEC_CMD_OFF_1 + CHN_REF_1MV_Y * 4) | DMA_LC_LOC;
		}
		// dont care SMaddr
		// dont care LMaddr
		// dont care BlkWidth
		ptDma->Control =
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DONT_CARE) |
			mDmaSType2b(DONT_CARE) |
			mDmaLType2b(DONT_CARE) |
			mDmaLen12b(0) |
			mDmaID4b(0);
		

		u32grpc = (2 << (ID_CHN_1MV*2))			// disable 1mv
				|(2 << (ID_CHN_4MV*2))		// disable 4mv
				|(2 << (ID_CHN_REF_UV*2))	// ref uv dma: disable
				|(2 << (ID_CHN_IMG*2))		// disable IMG
			  	#ifdef ENABLE_DEBLOCKING
				|(2 << (ID_CHN_ST_DEBK * 2))	// DEB_ST dma: disable
				|(2 << (ID_CHN_LD_DEBK * 2))	// DEB_LD dma: disable
			  	#endif
			
			  	#ifdef TWO_P_INT
				|(2 << (ID_CHN_STORE_MBS * 2))	// disable MBS dma
				|(2 << (ID_CHN_ACDC_MBS * 2))	// disable acdc MBS dma
			  	#endif
				|(3 << (ID_CHN_ACDC*2));		// LdSt_ACDC sync to VLD_done
											// load & store non-meaning
		if ( dec->output_fmt == OUTPUT_FMT_YUV) {
			  u32grpc |=
			  	#ifdef ENABLE_DEBLOCKING
  				(2 << (ID_CHN_YUV_TOP*2))	        // YUV top dma: disable.
				|(2 << (ID_CHN_YUV_MID*2))	// YUV mid dma: disable.
				|(2 << (ID_CHN_YUV_BOT*2));	// YUV bot dma: disable.
			  	#else
				(2 << (ID_CHN_YUV*2));		         // YUV dma: disable.
			  	#endif
		} else {
		   	  u32grpc |=
				#ifdef ENABLE_DEBLOCKING
   				(2 << (ID_CHN_RGB_TOP*2))	         // RGB top dma: disable.
				|(2 << (ID_CHN_RGB_MID*2))	// RGB mid dma: disable..
				|(2 << (ID_CHN_RGB_BOT*2));	// RGB bot dma: disable.
			  	#else
				(2 << (ID_CHN_RGB*2));		         // RGB dma: disable.
			  	#endif
		}

//(16)prePXI(3S)
		if (u32pipe & BIT_3ST) {
			// keep swaping the MV-buffer
			switch (mb_dmc->mode) {
				case MODE_INTER:
				case MODE_INTER_Q:
				case MODE_NOT_CODED:
					u32cmd_me |= MECTL_PXI_1MV;
					break;
				case MODE_INTER4V:
					break;
				default:
					// skip pxi command, but count mb;
					u32cmd_me |= MECTL_SKIP_PXI;
					break;
			}
		} else {	// skip pxi
			u32cmd_me |= (MECTL_SKIP_PXI | MECTL_PXI_MBCNT_DIS);
		}
		
//(5)goME_pmv----------------(17)goME_pxi
		if (u32pipe & (BIT_0ST | BIT_2ST | BIT_3ST)) {
			ptMP4->MECTL = u32cmd_me;
		}
		
//************************************** change to next mb *******************
		// update mb pointer
	  	#ifdef ENABLE_DEBLOCKING
		mbb_rgb = mbb_dt;
		mbb_dt = mbb_img;
	  	#else
		mbb_rgb = mbb_img;
	  	#endif
		mbb_img = mbb_dmc;
		mbb_dmc = mbb_ref;
		mbb_ref = mbb_vld;
		mb_last = mb_rgb;
	  	#ifdef ENABLE_DEBLOCKING
		mb_rgb = mb_dt;
		mb_dt = mb_img;
	  	#else
		mb_rgb = mb_img;
	  	#endif
		mb_img = mb_dmc;
		mb_dmc = mb_ref;
		mb_ref = mb_vld;

		i ++;
		mbb_vld = &mbb[i%P_FRAME_PIPE];
		mbb_vld->toggle = i & BIT0;
		if (mbb_vld->toggle == 0)
			dma_cmd_tgl = dma_cmd_tgl0;
		else
			dma_cmd_tgl = dma_cmd_tgl1;

//(18)preMoveImg(3S)
//(19)preMoveYuv(3S)(Conf1)
 		if (u32pipe & BIT_3ST) {
			u32grpc &= ~(uint32_t)(3 << (ID_CHN_IMG * 2));		// exec IMG dma
			// update address of destination
			// dont forget to update the next one after the one mb-jumpping(dma ping-pong chain)
			// mb_rgb is the next one after mb_img
			if ((mb_img->mb_jump != 0) || (mb_img_next->mb_jump != 0)) {
				// y
				dma_cmd_tgl[CHN_IMG_Y] = 
					mDmaSysMemAddr29b((uint32_t) dec->cur.y_phy + (uint32_t)mbb_img->x * 2 * SIZE_U +
					(uint32_t)mbb_img->y * dec->mb_width * SIZE_Y)
					|mDmaSysInc3b(DMA_INCS_256);		// 2 *  SIZE_U
				// u
				dma_cmd_tgl[CHN_IMG_U] =
					mDmaSysMemAddr29b((uint32_t) dec->cur.u_phy + (uint32_t)mbb_img->mbpos * SIZE_U)
					|mDmaSysInc3b(DMA_INCS_128);		// SIZE_U
				// v
				dma_cmd_tgl[CHN_IMG_V] =
					mDmaSysMemAddr29b((uint32_t) dec->cur.v_phy + (uint32_t)mbb_img->mbpos * SIZE_V)
					|mDmaSysInc3b(DMA_INCS_128);		// SIZE_V

			} else if ((mbb_img->x == 0) || (mbb_img->x == 1)) {
				dma_cmd_tgl[CHN_IMG_Y] =
					mDmaSysMemAddr29b((uint32_t) dec->cur.y_phy +
					(uint32_t) mbb_img->mbpos * SIZE_Y -
					(uint32_t) mbb_img->x * 2 * SIZE_U)
					|mDmaSysInc3b(DMA_INCS_256);		// 2 *  SIZE_U
			}
      	#ifdef ENABLE_DEBLOCKING
		} // if (u32pipe & BIT_3ST)
	  	#endif

    		#ifdef ENABLE_DEBLOCKING
//(42)preMoveFrameYuv(4S)(x-R)(Conf21de)
//(53)preMoveFrameRGB(D)(x-R)(Conf20de)
	    	if (dec->output_fmt == OUTPUT_FMT_YUV) {
				if (u32pipe & BIT_4ST) {
					if ((mbb_yuv->x >= u32output_mb_start) && (mbb_yuv->x < u32output_mb_end) &&
			    		(mbb_yuv->y >= u32output_mb_ystart) && (mbb_yuv->y < u32output_mb_yend)) {
						u32pipe |=  BIT_DMA_RGB_GO;
						if (mbb_yuv->y == 0)
							u32grpc &= ~(3 << (ID_CHN_YUV_MID * 2));		// Only middle
						else if (mbb_yuv->y == (dec->mb_height- 1))
							u32grpc &= ~((3 << (ID_CHN_YUV_TOP * 2))		// all
								|(3 << (ID_CHN_YUV_MID * 2))
								|(3 << (ID_CHN_YUV_BOT * 2)));
						else
							u32grpc &= ~((3 << (ID_CHN_YUV_TOP * 2))		// only top & middle
								|(3 << (ID_CHN_YUV_MID * 2)));
						if (((mb_yuv->mb_jump != 0) || (mb_yuv_next->mb_jump != 0)) ||
							(mbb_yuv->x == u32output_mb_start) || (mbb_yuv->x == (u32output_mb_start + 1))) {
							uint32_t mid_addr;
							// y output
							mid_addr = (uint32_t) dec->output_base_phy +
								((mbb_yuv->x - u32output_mb_start)  +
								(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride) * PIXEL_Y;
							dma_cmd_tgl[CHN_YUV_MID_Y] = 
								mDmaSysMemAddr29b(mid_addr)
								|mDmaSysInc3b(DMA_INCS_32);		// 2 * PIXEL_Y
							dma_cmd_tgl[CHN_YUV_TOP_Y] = 
								mDmaSysMemAddr29b(mid_addr - (PIXEL_Y / 2) * dec->output_stride)
								|mDmaSysInc3b(DMA_INCS_32);		// 2 * PIXEL_Y
							dma_cmd_tgl[CHN_YUV_BOT_Y] = 
								mDmaSysMemAddr29b(mid_addr + (PIXEL_Y / 2) * dec->output_stride)
								|mDmaSysInc3b(DMA_INCS_32);		// 2 * PIXEL_Y
							// u output
							mid_addr = (uint32_t) dec->output_base_u_phy +
								((mbb_yuv->x - u32output_mb_start)+
								(mbb_yuv->y - u32output_mb_ystart)  * dec->output_stride /2) * PIXEL_U;
							dma_cmd_tgl[CHN_YUV_MID_U] =
								mDmaSysMemAddr29b(mid_addr)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_U
							dma_cmd_tgl[CHN_YUV_TOP_U] =
								mDmaSysMemAddr29b(mid_addr - (PIXEL_U / 2) * dec->output_stride / 2)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_U
							dma_cmd_tgl[CHN_YUV_BOT_U] =
								mDmaSysMemAddr29b(mid_addr + (PIXEL_U / 2) * dec->output_stride / 2)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_U
							// v output
							mid_addr = (uint32_t) dec->output_base_v_phy +
								((mbb_yuv->x - u32output_mb_start)+
								(mbb_yuv->y - u32output_mb_ystart)  * dec->output_stride /2) * PIXEL_V;
							dma_cmd_tgl[CHN_YUV_MID_V] =
								mDmaSysMemAddr29b(mid_addr)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_V
							dma_cmd_tgl[CHN_YUV_TOP_V] =
								mDmaSysMemAddr29b(mid_addr - (PIXEL_V / 2) * dec->output_stride / 2)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_V
							dma_cmd_tgl[CHN_YUV_BOT_V] =
								mDmaSysMemAddr29b(mid_addr + (PIXEL_V / 2) * dec->output_stride / 2)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_V
						}
					}
				}
	    	} else { //if (dec->output_fmt == OUTPUT_FMT_YUV) {
				if (u32pipe & BIT_PRE_DT) {
					if ((mbb_rgb->x >= u32output_mb_start) && (mbb_rgb->x < u32output_mb_end) &&
			    		(mbb_rgb->y >= u32output_mb_ystart) && (mbb_rgb->y < u32output_mb_yend)) {
						u32pipe |=  BIT_DMA_RGB_GO;
						if (mbb_rgb->y == 0)
							u32grpc &= ~(3 << (ID_CHN_RGB_MID * 2));		// Only middle
						else if (mbb_rgb->y == (dec->mb_height- 1))
							u32grpc &= ~((3 << (ID_CHN_RGB_TOP * 2))		// all
								|(3 << (ID_CHN_RGB_MID * 2))
								|(3 << (ID_CHN_RGB_BOT * 2)));
						else
							u32grpc &= ~((3 << (ID_CHN_RGB_TOP * 2))		// only top & middle
								|(3 << (ID_CHN_RGB_MID * 2)));
					}
					// update address of destination
					// dont forget the next one after the one mb-jumpping
					// mb_last is the next one after mb_rgb
					if ((mb_rgb->mb_jump != 0) || (mb_rgb_next->mb_jump != 0) ||
						(mbb_rgb->x == u32output_mb_start) || 
						(mbb_rgb->x == (u32output_mb_start + 1))) {

						uint32_t mid_addr;
						// y output
						mid_addr = (uint32_t) dec->output_base_phy +
							((mbb_rgb->x - u32output_mb_start)  +
							(mbb_rgb->y - u32output_mb_ystart) * dec->output_stride) * PIXEL_Y * rgb_pixel_size;
						dma_cmd_tgl[CHN_RGB_MID] =
							mDmaSysMemAddr29b(mid_addr) |
							mDmaSysInc3b(RGB_DMA_INC);		//PIXEL_Y * 2;
						dma_cmd_tgl[CHN_RGB_TOP] =
							mDmaSysMemAddr29b(mid_addr -
							(PIXEL_Y * rgb_pixel_size / 2) * dec->output_stride) |
							mDmaSysInc3b(RGB_DMA_INC);		//PIXEL_Y * 2;
						dma_cmd_tgl[CHN_RGB_BOT] =
							mDmaSysMemAddr29b(mid_addr +
							(PIXEL_Y * rgb_pixel_size / 2) * dec->output_stride) |
							mDmaSysInc3b(RGB_DMA_INC);		//PIXEL_Y * 2;
					}
 				}
	    	}

    		#else //#ifdef ENABLE_DEBLOCKING
		   	if (dec->output_fmt == OUTPUT_FMT_YUV) {
				if ((mbb_yuv->x >= u32output_mb_start) && (mbb_yuv->x < u32output_mb_end) &&
			    	(mbb_yuv->y >= u32output_mb_ystart) && (mbb_yuv->y < u32output_mb_yend)) {
					u32grpc &= ~(uint32_t)(3 << (ID_CHN_YUV * 2));		// exec YUV dma
					if (((mb_yuv->mb_jump != 0) || (mb_yuv_next->mb_jump != 0)) ||
						(mbb_yuv->x == u32output_mb_start) || (mbb_yuv->x == (u32output_mb_start + 1))) {
                        #ifdef ENABLE_DEINTERLACE
                        if(dec->u32EnableDeinterlace) {
  				    		// Take special note that we implement the deinterlace function based on the
  				    		// following two limitations :
                        	//   - The number of macroblock rows must be even number.
                        	//   - The image width and height must be equal to the display width and height.
  				    		//unsigned int field_selector = (mbb_yuv->y <= (dec->height>>1)/PIXEL_Y)?0:1;
  				    		unsigned int field_selector = (mbb_yuv->y < (dec->mb_height>>1))?0:1;
	                    	unsigned int field_mb_y = mbb_yuv->y % (dec->mb_height>>1);
				    		// y output
							dma_cmd_tgl[CHN_YUV_Y] =
								mDmaSysMemAddr29b((uint32_t) dec->output_base_phy + (field_selector*dec->output_stride) +
								((mbb_yuv->x - u32output_mb_start)  +
								//(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride) * PIXEL_Y)
								field_mb_y * (dec->output_stride<<1)) * PIXEL_Y)
								|mDmaSysInc3b(DMA_INCS_32);		// 2 * PIXEL_Y
							// u output
							dma_cmd_tgl[CHN_YUV_U] =
								mDmaSysMemAddr29b((uint32_t) dec->output_base_u_phy + (field_selector*(dec->output_stride>>1)) +
								((mbb_yuv->x - u32output_mb_start)  +
								//(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride/2) * PIXEL_U)
								field_mb_y * dec->output_stride) * PIXEL_U)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_U
							// v output
							dma_cmd_tgl[CHN_YUV_V] =
								mDmaSysMemAddr29b((uint32_t) dec->output_base_v_phy + (field_selector*(dec->output_stride>>1)) +
								((mbb_yuv->x - u32output_mb_start)  +
								//(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride/2) * PIXEL_V)
								field_mb_y * dec->output_stride) * PIXEL_V)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_V
                       	} else { //if(dec->u32EnableDeinterlace) {
                            // y output
							dma_cmd_tgl[CHN_YUV_Y] =
								mDmaSysMemAddr29b((uint32_t) dec->output_base_phy +
								((mbb_yuv->x - u32output_mb_start)  +
								(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride) * PIXEL_Y)
								|mDmaSysInc3b(DMA_INCS_32);		// 2 * PIXEL_Y
							// u output
							dma_cmd_tgl[CHN_YUV_U] =
								mDmaSysMemAddr29b((uint32_t) dec->output_base_u_phy +
								((mbb_yuv->x - u32output_mb_start)  +
								(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride/2) * PIXEL_U)
								|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_U
							// v output
							dma_cmd_tgl[CHN_YUV_V] =
								mDmaSysMemAddr29b((uint32_t) dec->output_base_v_phy +
								((mbb_yuv->x - u32output_mb_start)  +
								(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride/2) * PIXEL_V)
							|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_V
                    	}
				  		#else //#ifdef ENABLE_DEINTERLACE
						// y output
						dma_cmd_tgl[CHN_YUV_Y] =
							mDmaSysMemAddr29b((uint32_t) dec->output_base_phy +
							((mbb_yuv->x - u32output_mb_start)  +
							(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride) * PIXEL_Y)
							|mDmaSysInc3b(DMA_INCS_32);		// 2 * PIXEL_Y
						// u output
						dma_cmd_tgl[CHN_YUV_U] =
							mDmaSysMemAddr29b((uint32_t) dec->output_base_u_phy +
							((mbb_yuv->x - u32output_mb_start)  +
							(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride/2) * PIXEL_U)
							|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_U
						// v output
						dma_cmd_tgl[CHN_YUV_V] =
							mDmaSysMemAddr29b((uint32_t) dec->output_base_v_phy +
							((mbb_yuv->x - u32output_mb_start)  +
							(mbb_yuv->y - u32output_mb_ystart) * dec->output_stride/2) * PIXEL_V)
							|mDmaSysInc3b(DMA_INCS_16);		// 2 * PIXEL_V
                        #endif
					}
				}
		   	}
		} // if (u32pipe & BIT_3ST)
		#endif //#ifdef ENABLE_DEBLOCKING

		#ifndef ENABLE_DEBLOCKING
//(26)preMoveRGB(4S-R)(D)(Conf0)
//(26')Conf1(4S-x)
 		if (u32pipe & BIT_4ST) {
			u32pipe &= ~ BIT_4ST;
		    if (dec->output_fmt < OUTPUT_FMT_YUV) {
				if ((mbb_rgb->x >= u32output_mb_start) && (mbb_rgb->x < u32output_mb_end) &&
			    	(mbb_rgb->y >= u32output_mb_ystart) && (mbb_rgb->y < u32output_mb_yend)) {
					u32pipe |=  BIT_DMA_RGB_GO;
					u32grpc &= ~(uint32_t)(3 << (ID_CHN_RGB * 2));		// exec RGB dma
				}
				// update address of destination
				// dont forget to update the next one after the one mb-jumpping
				// since dma is a ping-pong chain
				// mb_last is the next one after mb_rgb
				if ((mb_rgb->mb_jump != 0) || (mb_rgb_next->mb_jump != 0) ||
					(mbb_rgb->x == u32output_mb_start) ||
					(mbb_rgb->x == (u32output_mb_start + 1))) {
					dma_cmd_tgl[CHN_RGB] =
						mDmaSysMemAddr29b((uint32_t) dec->output_base_phy +	((mbb_rgb->x - u32output_mb_start)  +
						(mbb_rgb->y - u32output_mb_ystart) * dec->output_stride) * PIXEL_Y * rgb_pixel_size) ;
						//mDmaSysInc3b(RGB_DMA_INC);		//PIXEL_Y * 2;
					if ( dec->output_fmt == OUTPUT_FMT_YUV) 
						dma_cmd_tgl[CHN_RGB] |= mDmaSysInc3b(DMA_INCS_64);
					else if  ( dec->output_fmt == OUTPUT_FMT_RGB888)
						dma_cmd_tgl[CHN_RGB] |= mDmaSysInc3b(DMA_INCS_128);	
					else 
						dma_cmd_tgl[CHN_RGB] |= mDmaSysInc3b(DMA_INCS_64);
				}
		    }
 		}
 		
		#endif // #ifndef ENABLE_DEBLOCKING

		mFa526DrainWrBuf();

DECODER_PFRAME_RECHECK:
//(6)waitME_PMV(0S)
//(7)storeMV(0S)
//(8)preMoveRef(0S-1S)
		if (u32pipe & BIT_0ST) {
			u32pipe &= ~ BIT_0ST;
			u32pipe |= BIT_1ST;
			vpe_prob_me_start();
			// check PMV_done
			while ((ptMP4->CPSTS & BIT3) == 0)
				;
			vpe_prob_me_end();
			u32temp = ptMP4->VLDSTS;
			while (u32temp  & 0xF000) {
				mVpe_Indicator(0x92000000 | (u32temp  & 0xF000));
				mVpe_FAIL(3, u32temp);
				return 0;//ivan
				
			 	//waitDMC & waitDT
				while ((ptMP4->CPSTS & (BIT14 | BIT1)) != (BIT14 | BIT1))
					;
				// waitME
				while ((ptMP4->CPSTS & BIT0) == 0)
					;
				u32temp = FindRsmkOrVopS(bs, ptMP4);
			}
			// Start code detected
			if (u32temp & BIT2) {
				u32ErrorCount ++;
				// correct 'ref'ed(dmc) jump to correct next-'ref'ed(ref)
				mb_dmc->mb_jump = dec->mb_width * dec->mb_height - mbb_ref->mbpos;
				// correct 'vld'ed(ref) information
				mbb_ref->x = 0;
				mbb_ref->y = dec->mb_height;
				mbb_ref->mbpos = dec->mb_width * dec->mb_height;
				// no more VLD stage
				u32pipe &= ~BIT_0ST;
				u32cmd_mc &= ~MCCTL_DECGO;
			}
			// Resync marker detected
			if (u32temp & BIT1) {
				if (bRead_video_packet_header(dec, &bound) == -1) {
					mVpe_FAIL(4, u32temp);
					return 0;//ivan
					break;
				}
				if (bound != mbb_ref->mbpos) {
					u32ErrorCount ++;
					// correct 'ref'ed(dmc) jump to correct next-'ref'ed(ref)
					mb_dmc->mb_jump = bound - mbb_ref->mbpos;
					#ifndef TWO_P_INT
					// correct 'vld'ed(ref) information
					mb_ref = &dec->mbs[bound];
					#endif
					mb_ref->mb_jump = 0;
					mbb_ref->y = (int32_t)(bound / dec->mb_width);
					//mbb_ref->x = bound % dec->mb_width;
					mbb_dmc->x = bound - mbb_dmc->y * dec->mb_width;
					mbb_ref->mbpos = bound;
					// correct store acdc pointer
					// dont care about 'load preditor', it will not be used until next line is reached
					dma_cmd_tgl[chn_store_predictor] =
						mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy + 64 * mbb_vld->x)
						| mDmaSysInc3b(DMA_INCS_128);
					if (mbb_vld->toggle == 0)
						dma_cmd_tgl1[chn_store_predictor] =
							mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy + 64 * (mbb_vld->x + 1))
							| mDmaSysInc3b(DMA_INCS_128);
					else
						dma_cmd_tgl0[chn_store_predictor] =
							mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy + 64 * (mbb_vld->x + 1))
							| mDmaSysInc3b(DMA_INCS_128);
				}
				mb_cnt_in_vp = (u32temp >> 16) - 1;
				//pu32table = &Table_Output[4];
				pu32table = &((uint32_t *)(Mp4Base (dec) + TABLE_OUTPUT_OFF))[4];
				mbpos_in_vp = 0;
			}
			u32temp = *pu32table;
			if (u32temp & BIT5) {	// not coded
				mb_ref->mode = MODE_NOT_CODED;
			  	#ifdef ENABLE_DEBLOCKING
				// same as the previous one
				mbb_ref->quant = mbb_dmc->quant;
			  	#endif // #ifdef ENABLE_DEBLOCKING
				if (dec->data_partitioned) {
					// point to next entry if the one is the last
					if (mbpos_in_vp >= ((u32temp >> 8) & 0xFFF))
						pu32table += 4;
				}
			} else {
				mb_ref->mode = u32temp & 0x07;
				mbb_ref->cbp = (u32temp >> 23) & 0x3F;
				mbb_ref->quant = (ptMP4->VOP0 >> 8) & 0xFF;
				if (dec->data_partitioned)			// point to next entry
					pu32table += 4;
			}
			mbpos_in_vp ++;
			//storeMV(0S)
			if (mb_ref->mode <= MODE_INTER4V) {
				uint32_t * pu32me = (uint32_t *) (Mp4Base (dec) + ME_CMD_Q_OFF);
				// which buffer valid?
				mb_ref->MVuv.u32num = pu32me[MVUV_BUFF];

				if (mb_ref->mode == MODE_INTER4V) {
					mb_ref->mvs[0].u32num= pu32me[MV_BUFF + 0];
					mb_ref->mvs[1].u32num= pu32me[MV_BUFF + 1];
					mb_ref->mvs[2].u32num= pu32me[MV_BUFF + 2];
					mb_ref->mvs[3].u32num= pu32me[MV_BUFF + 3];
				} else {	// MODE_INTER or MODE_INTER_Q
					mb_ref->mvs[0].u32num=
					mb_ref->mvs[1].u32num=
					mb_ref->mvs[2].u32num=
					mb_ref->mvs[3].u32num= pu32me[MV_BUFF + 3];
				}
			} else  {
				//added by ivan
				mb_ref->MVuv.u32num =
				mb_ref->mvs[0].u32num =
				mb_ref->mvs[1].u32num =
				mb_ref->mvs[2].u32num =
				mb_ref->mvs[3].u32num = 0;
			}

			//preMoveRef
			switch (mb_ref->mode) {
				case MODE_INTER:
				case MODE_INTER_Q:
					u32grpc &= ~(uint32_t)((3 << (ID_CHN_1MV* 2))		// exec REF1MV dma
									|(3 << (ID_CHN_REF_UV*2)));		// exec REFUV dma
					dma_mvRef1MV(dec, mb_ref, mbb_ref);
					break;
				case MODE_INTER4V:
					u32grpc &= ~(uint32_t)((3 << (ID_CHN_4MV* 2))		// exec REF4MV dma
									|(3 << (ID_CHN_REF_UV*2)));		// exec REFUV dma
					dma_mvRef4MV(dec, mb_ref, mbb_ref);
					break;
				case MODE_NOT_CODED:
					u32grpc &= ~(uint32_t)((3 << (ID_CHN_1MV* 2))		// exec REF1MV dma
									|(3 << (ID_CHN_REF_UV*2)));		// exec REFUV dma
					dma_mvRefNotCoded(dec, mbb_ref);
					break;
				default:
					break;
			}
			#ifdef TWO_P_INT
			// added by Leo for the special case while MB width is less or euqal to 4
			if (dec->mb_width <=4) {
			  	mbs_ACDC[(i-1)%MBS_ACDC] = mbs_local[(i-1)%MBS_LOCAL_P];
			}
			#endif
		} //if (u32pipe & BIT_0ST) {

		mbb_vld->x = mbb_ref->x + 1;
		mbb_vld->y = mbb_ref->y;
		mbb_vld->mbpos = mbb_ref->mbpos + 1;
		if (mbb_vld->x == dec->mb_width) {
			mbb_vld->x = 0;
			mbb_vld->y ++;
		}
		// indicator
		mVpe_Indicator(0x91000000 | mbb_vld->y << 12 | mbb_vld->x);

//(1)preVLD(x-0S)
		u32cmd_mc = u32cmd_mc_reload;
		u32cmd_me = mMECTL_RND1b(rounding) | MECTL_MEGO;
		if (mbb_vld->y < dec->mb_height) {
			u32pipe |= BIT_0ST;
			// init mb_vld
			#ifdef TWO_P_INT
			mb_vld = &mbs_local[i%MBS_LOCAL_P];
			mb_vld->mb_jump = 0;
			// init acdcPredict command parameter
			u32temp = predict_acdc_P(mbs_ACDC, mbb_vld, (int32_t)dec->mb_width,	bound, i, mb_ref);
			// allocate mbs_ACDC for next time preVLD
			// modified by Leo for the special case when MB width is less and euqal to 4
			if ((dec->mb_width > 4) && (mbb_vld->mbpos >= (dec->mb_width - 2))) {
			//if (mbb_vld->mbpos >= (dec->mb_width - 2)) {
				u32grpc &= ~(3 << (ID_CHN_ACDC_MBS * 2));		// exec acdc mbs dma
				dma_cmd_tgl[CHN_ACDC_MBS] =
					mDmaSysMemAddr29b(&dec->mbs[mbb_vld->mbpos-(dec->mb_width - 2)]);
				dma_cmd_tgl[CHN_ACDC_MBS + 1] =
					mDmaLocMemAddr14b(&mbs_ACDC[i%MBS_ACDC]);
			}
			#else
			mb_vld = &dec->mbs[mbb_vld->mbpos];
			mb_vld->mb_jump = 0;
			// init acdcPredict command parameter
			u32temp = predict_acdc_P(dec->mbs, mbb_vld, (int32_t)dec->mb_width, bound);
			#endif
			u32cmd_mc = u32temp | u32cmd_mc_reload;
		}
		
		
		#ifdef ENABLE_DEBLOCKING		
//(43)preStoreDeBk(4S-5S)
		if (u32pipe & BIT_4ST) {
			u32pipe &= ~ BIT_4ST;
			u32pipe |= BIT_5ST;
			// only 0 ~ (mb_height-2) MB_row need to store its deblocking bottom MB
			if (mbb_dt->y < (dec->mb_height - 1)) {
			  	#ifdef ENABLE_DEBLOCKING_SYNC_DEBLOCKING_DONE
			    u32grpc |=  (3 << (ID_CHN_ST_DEBK * 2));		// exec sync to Deblock_DONE
			  	#else
				u32grpc &= ~(3 << (ID_CHN_ST_DEBK * 2));		// exec STORE_DEBLOCK
			  	#endif
				if ((mb_dt->mb_jump != 0) || (mb_dt_next->mb_jump != 0) ||
					(mbb_dt->x == 0) || (mbb_dt->x == 1)) {
					if ( dec->output_fmt == OUTPUT_FMT_YUV) { 
						dma_cmd_tgl[CHN_STORE_YUV_DEBK] =
								mDmaSysMemAddr29b((uint32_t)dec->pu8DeBlock_phy + 256 * mbb_dt->x)
								| mDmaSysInc3b(DMA_INCS_512);
					} else {
						dma_cmd_tgl[CHN_STORE_RGB_DEBK] =
								mDmaSysMemAddr29b((uint32_t)dec->pu8DeBlock_phy + 256 * mbb_dt->x)
								| mDmaSysInc3b(DMA_INCS_512);

					}
				}
			}
		}
//(21)preLoadDeBk(2S)
		if (u32pipe & BIT_2ST) {
			// only 1 ~ (mb_height-1) MB_row need to load its deblocking top MB
			if (mbb_dmc->y > 0) {
				u32grpc |= 3 << (ID_CHN_LD_DEBK * 2);	// exec sync to DT_DONE
				if ((mb_dmc->mb_jump != 0) || (mb_debk->mb_jump != 0) ||
					(mbb_dmc->x == 0) || (mbb_dmc->x == 1)) {
					if ( dec->output_fmt == OUTPUT_FMT_YUV) { 
						dma_cmd_tgl[CHN_LOAD_YUV_DEBK] =
							mDmaSysMemAddr29b((uint32_t)dec->pu8DeBlock_phy + 256 * mbb_dmc->x)
							| mDmaSysInc3b(DMA_INCS_512);
					} else {
						dma_cmd_tgl[CHN_LOAD_RGB_DEBK] =
							mDmaSysMemAddr29b((uint32_t)dec->pu8DeBlock_phy + 256 * mbb_dmc->x)
							| mDmaSysInc3b(DMA_INCS_512);
					}
				}
			}
		}
		#endif

//(9)preStoreACDC
//(2)preLoadACDC
		if ((mb_ref->mb_jump != 0) || (mb_dmc->mb_jump != 0) ||(mbb_vld->x == 0)) {
			dma_cmd_tgl[chn_store_predictor] =
				mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy)
				| mDmaSysInc3b(DMA_INCS_128);
			dma_cmd_tgl[chn_load_predictor] =
				mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy + 64)
				| mDmaSysInc3b(DMA_INCS_128);
		} else if (mbb_vld->x == 1) {
			dma_cmd_tgl[chn_store_predictor] =
				mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy + 64)
				| mDmaSysInc3b(DMA_INCS_128);
		}
		if (mbb_vld->x == (dec->mb_width - 1)) {
			dma_cmd_tgl[chn_load_predictor] =
				mDmaSysMemAddr29b((uint32_t)dec->pu16ACDC_ptr_phy)
				| mDmaSysInc3b(DMA_INCS_128);
		}
//(20)waitDMC(3S-4S)
		if (u32pipe & BIT_3ST) {
			u32pipe &= ~ BIT_3ST;
			u32pipe |= BIT_4ST;
			vpe_prob_dmc_start();
			while ((ptMP4->CPSTS & BIT1) == 0)
				;
			vpe_prob_dmc_end();
		}
		
	  	#ifdef ENABLE_DEBLOCKING
		u32qcr = 0;
	  	#endif
	  
//(13)preDMC(2S-3S)
		if (u32pipe & BIT_2ST) {
			u32pipe &= ~ BIT_2ST;
			u32pipe |= BIT_3ST;
			#ifdef TWO_P_INT
			u32grpc &= ~(uint32_t)(3 << (ID_CHN_STORE_MBS * 2));	// exec MBS dma
			// assign start address of local memory
			dma_cmd_tgl[CHN_STORE_MBS + 1] =
				mDmaLocMemAddr14b(&mbs_local[(i-2) % MBS_LOCAL_P]);
			// mbs
			if ((mb_dmc->mb_jump != 0) || (mb_dmc_next->mb_jump != 0)) {
				dma_cmd_tgl[CHN_STORE_MBS] = 
					mDmaSysMemAddr29b(&dec->mbs[mbb_dmc->mbpos]) |
					mDmaSysInc3b(DMA_INCS_48);		// 2 *  MB_SIZE
			}
			#endif
			if (mb_dmc->mode <= MODE_INTRA_Q) {
				if (mb_dmc->mode <= MODE_INTER4V)
					u32cmd_mc |= MCCTL_DMCGO;
				else
					u32cmd_mc |= (MCCTL_DMCGO | MCCTL_INTRA);
			    // fill CBP in this stage
			  	#ifdef ENABLE_DEBLOCKING
  				u32qcr = (mbb_dmc->quant << 18) |
				            mbb_dmc->cbp;
			  	#else
				ptMP4->QCR0 = (mbb_dmc->quant << 18) |
							mbb_dmc->cbp;
			  	#endif
			}
		}
		
		#ifdef ENABLE_DEBLOCKING
		if (dec->output_fmt < OUTPUT_FMT_YUV) {
//(54)waitDT(D-x)(Conf20de)
 			if (u32pipe & BIT_PRE_DT) {
				u32pipe &= ~ BIT_PRE_DT;
				vpe_prob_dt_start();
				while((ptMP4->CPSTS & BIT14) == 0)
					;
				vpe_prob_dt_end();
			}
		}

 		if (u32pipe & BIT_5ST) {
//(44)preDT(5S)(x-D)(Conf20de)
		   if (dec->output_fmt < OUTPUT_FMT_YUV) {
				if ((mbb_dt->x >= u32output_mb_start)
					&& (mbb_dt->x < u32output_mb_end)
					&& (mbb_dt->y >= u32output_mb_ystart)
					&& (mbb_dt->y < u32output_mb_yend)) {
					u32pipe |= BIT_PRE_DT;
					// pls refer to pipeline arrangement of dma_b.c
					if (mbb_vld->toggle == 0) {
						// dont care YUV2RGB source addr, DeBk module will handle the p-p
						if ( dec->output_fmt == OUTPUT_FMT_YUV) {
							ptMP4->MCCADDR = BUFFER_RGB_OFF_0(RGB_PIXEL_SIZE_1);// YUV2RGB dest. addr, [10: 7] valid
						} else if ( dec->output_fmt == OUTPUT_FMT_RGB888) {
							ptMP4->MCCADDR = BUFFER_RGB_OFF_0(RGB_PIXEL_SIZE_4);// YUV2RGB dest. addr, [10: 7] valid
						} else {
							ptMP4->MCCADDR = BUFFER_RGB_OFF_0(RGB_PIXEL_SIZE_2);// YUV2RGB dest. addr, [10: 7] valid
						}
					} else {
						// dont care YUV2RGB source addr, DeBk module will handle the p-p
						ptMP4->MCCADDR = BUFFER_RGB_OFF_1();// YUV2RGB dest. addr, [10: 7] valid
					}
					if (mbb_dt->y == (dec->mb_height - 1))
						u32cmd_mc |= MCCTL_DT_BOT;
					u32cmd_mc |= MCCTL_DTGO;
				}
			} 
//(44')wait DeBk(5S)
			mFa526DrainWrBuf();
			while ((ptMP4->CPSTS & BIT16) == 0)
				;
 		}//if (u32pipe & BIT_5ST) {
 		
 		#else // #ifdef ENABLE_DEBLOCKING
 
		if (dec->output_fmt < OUTPUT_FMT_YUV) {
//(27)waitDT(D-x)(Conf0)
 			if (u32pipe & BIT_PRE_DT) {
				u32pipe &= ~ BIT_PRE_DT;
				vpe_prob_dt_start();
				while((ptMP4->CPSTS & BIT14) == 0)
					;
				vpe_prob_dt_end();
			}
  
//(21)preDT(4S)(x-D)(Conf0)
 			if (u32pipe & BIT_4ST) {
				if ((mbb_dt->x >= u32output_mb_start)
					&& (mbb_dt->x < u32output_mb_end)
					&& (mbb_dt->y >= u32output_mb_ystart)
					&& (mbb_dt->y < u32output_mb_yend)) {
					u32pipe |= BIT_PRE_DT;
					// pls refer to pipeline arrangement of dma_b.c
					if (mbb_vld->toggle == 0) {
						ptMP4->MECADDR = INTER_Y_OFF_0;		// YUV2RGB source addr, [10: 7] valid
						if ( dec->output_fmt == OUTPUT_FMT_YUV) {
							ptMP4->MCCADDR = BUFFER_RGB_OFF_0(RGB_PIXEL_SIZE_1);// YUV2RGB dest. addr, [10: 7] valid
						} else if ( dec->output_fmt == OUTPUT_FMT_RGB888) {
							ptMP4->MCCADDR = BUFFER_RGB_OFF_0(RGB_PIXEL_SIZE_4);// YUV2RGB dest. addr, [10: 7] valid
						} else {
							ptMP4->MCCADDR = BUFFER_RGB_OFF_0(RGB_PIXEL_SIZE_2);// YUV2RGB dest. addr, [10: 7] valid
						}
					} else {
						ptMP4->MECADDR = INTER_Y_OFF_1;		// YUV2RGB source addr, [10: 7] valid
						if ( dec->output_fmt == OUTPUT_FMT_YUV) {
							ptMP4->MCCADDR = BUFFER_RGB_OFF_1(RGB_PIXEL_SIZE_1);// YUV2RGB dest. addr, [10: 7] valid
						} else if ( dec->output_fmt == OUTPUT_FMT_RGB888) {
							ptMP4->MCCADDR = BUFFER_RGB_OFF_1(RGB_PIXEL_SIZE_4);// YUV2RGB dest. addr, [10: 7] valid
						} else {
							ptMP4->MCCADDR = BUFFER_RGB_OFF_1(RGB_PIXEL_SIZE_2);// YUV2RGB dest. addr, [10: 7] valid
						}
					}			  
					u32cmd_mc |= MCCTL_DTGO;
				}
 			}
		} //if (dec->output_fmt < OUTPUT_FMT_YUV) {
		#endif // #ifdef ENABLE_DEBLOCKING
	
		#ifdef ENABLE_DEBLOCKING
//(35')preDeBk(4S)
		if (u32pipe & BIT_4ST) {
			u32cmd_mc |= MCCTL_DEBK_GO;
			if (mbb_debk->x == 0)
				u32cmd_mc |= MCCTL_DEBK_LEFT;
			if (mbb_debk->y == 0)
				u32cmd_mc |= MCCTL_DEBK_TOP;
			// pls refer to pipeline arrangement of dma_b.c
			if (mbb_vld->toggle == 0) {
				ptMP4->MECADDR = INTER_Y_OFF_0;		// DeBk source addr, [10: 7] valid
				// dont care DeBk dest. addr, DeBk module will handle the p-p
			} else {
				ptMP4->MECADDR = INTER_Y_OFF_1;		// DeBk source addr, [10: 7] valid
				// dont care DeBk dest. addr, DeBk module will handle the p-p
			}
			u32qcr |= mbb_debk->quant << 25;
		}
#endif
	
//(10)waitVLD(1S-2S)
		if (u32pipe & BIT_1ST) {
			u32pipe &= ~BIT_1ST;
			u32pipe |= BIT_2ST;
			vpe_prob_vld_start();
			while (((u32temp = ptMP4->VLDSTS) & BIT0) == 0)
				;
			vpe_prob_vld_end();
			if (u32temp  & 0xF000)
				goto DECODER_PFRAME_RECHECK;
		}

		//(*)ChgQCOEFF_vld----------------------------(*)ChgQCOEFF_dmc
		// switch 3 buffer
		// setup Dezigzag-scan output address and dequant input address
		// VLD output to DZQ[31..16]		 DZQ[15..0] is input of DMC
		switch (i % 3) {
			case 0:
				ptMP4->QAR = ((QCOEFF_OFF_0 & 0xFFFF) << 16) | (QCOEFF_OFF_1 & 0xFFFF);
				break;
			case 1:
				ptMP4->QAR = ((QCOEFF_OFF_1 & 0xFFFF) << 16) | (QCOEFF_OFF_2 & 0xFFFF);
				break;
			default:
				ptMP4->QAR = ((QCOEFF_OFF_2 & 0xFFFF) << 16) | (QCOEFF_OFF_0 & 0xFFFF);
				break;
		}

//(22)waitME_pxi(4S)
		if (u32pipe & BIT_4ST) {
			vpe_prob_me_start();
			// check ME done
			while ((ptMP4->CPSTS & BIT0) == 0)
				;
			vpe_prob_me_end();
		}
		
		if (u32pipe & BIT_3ST) {
			// toggle meiaddr & mciaddr when mode != MODE_STUFFING
			if (mbb_vld->toggle == 0) {
				// interpolation_output direct to INTER_Y_OFF_1
				ptMP4->MEIADDR = INTER_Y_OFF_1;
				// dmc_input(& output) direct to INTER_Y_OFF_1
				ptMP4->MCIADDR = INTER_Y_OFF_1;
			}
			else {
				// interpolation_output direct to INTER_Y_OFF_0
				ptMP4->MEIADDR = INTER_Y_OFF_0;
				// dmc_input(& output) direct to INTER_Y_OFF_0
				ptMP4->MCIADDR = INTER_Y_OFF_0;
			}
		}
		

//(11)waitDMA_LD_ST-(14)waitDMA_ref------------(28)waitDMA_img--(31)waitDMA_rgb(Conf0)
//										|---(29)waitDMA_yuv(Conf1)
		
		vpe_prob_dma_start();
		while((ptDma->Status & BIT0) == 0)
			;
		vpe_prob_dma_end();
		

		if (u32pipe == 0)
			break;
	}

#if 0
	i = 0;
	while (u32ErrorCount) {
		mb_vld = &dec->mbs[i];
		if (mb_vld->mb_jump != 0) {
			error_concealment_p(dec, i, mb_vld->mb_jump + 1);
			u32ErrorCount --;
			i += dec->mbs[i].mb_jump;
		} else
			i ++;
	}
#endif

	for (; (ptMP4->VLDSTS & BIT10)==0; ) ;			/// wait for autobuffer clean
	// stop auto-buffering
	ptMP4->VLDCTL = VLDCTL_ABFSTOP;
	return 0;
}

// temp use bank2 & bank3 (0x8000 ~ 0x9000)
static __inline void
nframe_dma(DECODER * dec,
	uint32_t img_sour,
	uint32_t img_dest,
	uint32_t u32length)
{
	uint32_t * dma_cmd_n = (uint32_t *)(Mp4Base (dec) + DMA_CMD_N_OFF);
	volatile MDMA * ptDma = (MDMA *)(Mp4Base (dec) + DMA_OFF);
         int32_t chn_nvop_in, chn_nvop_out;
         if ( dec->output_fmt == OUTPUT_FMT_YUV) {
		chn_nvop_in = CHN_NVOP_YUV_IN;
		chn_nvop_out = CHN_NVOP_YUV_OUT;
	} else {
		chn_nvop_in = CHN_NVOP_RGB_IN;
		chn_nvop_out = CHN_NVOP_RGB_OUT;
	}
	// init dma parameter
	dma_cmd_n[chn_nvop_in + 1] = mDmaLocMemAddr14b(REF_Y_OFF_0);
	// dont care block width
	dma_cmd_n[chn_nvop_in + 3] =
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(NVOP_MAX/4);

	dma_cmd_n[chn_nvop_out + 1] = mDmaLocMemAddr14b(REF_Y_OFF_0);
	// dont care block width
	dma_cmd_n[chn_nvop_out + 3] =
		mDmaIntChainMask1b(FALSE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(FALSE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(NVOP_MAX/4);
	
	while (u32length) {
		// indicator
		mVpe_Indicator(0x93000000 | u32length);
		dma_cmd_n[chn_nvop_in + 0] = mDmaSysMemAddr29b(img_sour);
		dma_cmd_n[chn_nvop_out + 0] = mDmaSysMemAddr29b(img_dest);
		if (u32length < NVOP_MAX) {
			dma_cmd_n[chn_nvop_in + 3] =
				mDmaIntChainMask1b(TRUE) |
				mDmaEn1b(TRUE) |
				mDmaChainEn1b(TRUE) |
				mDmaDir1b(DMA_DIR_2LOCAL) |
				mDmaSType2b(DMA_DATA_SEQUENTAIL) |
				mDmaLType2b(DMA_DATA_SEQUENTAIL) |
				mDmaLen12b(u32length/4);
			dma_cmd_n[chn_nvop_out + 3] =
				mDmaIntChainMask1b(FALSE) |
				mDmaEn1b(TRUE) |
				mDmaChainEn1b(FALSE) |
				mDmaDir1b(DMA_DIR_2SYS) |
				mDmaSType2b(DMA_DATA_SEQUENTAIL) |
				mDmaLType2b(DMA_DATA_SEQUENTAIL) |
				mDmaLen12b(u32length/4);
			// update transfer length
			u32length = 0;
		}else {
			// update transfer length & image source & destination
			u32length -= NVOP_MAX;
			img_sour += NVOP_MAX;
			img_dest += NVOP_MAX;
		}
		ptDma->CCA = (DMA_CMD_N_OFF + chn_nvop_in * 4) | DMA_LC_LOC;
		// dont care SMaddr
		// dont care LMaddr
		// dont care BlkWidth
		ptDma->Control =
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DONT_CARE) |
			mDmaSType2b(DONT_CARE) |
			mDmaLType2b(DONT_CARE) |
			mDmaLen12b(0);
		// wait for dma finish
		mFa526DrainWrBuf();
		while((ptDma->Status & BIT0) == 0)
			;
	}

}
int decoder_nframe(DECODER* dec)
{
	// display
	if (dec->output_base_phy != dec->output_base_ref_phy) {
		uint32_t src;
		uint32_t dst;
		int row;
		uint32_t u32output_mb_end;
		uint32_t u32output_mb_start = 0;
		uint32_t u32output_mb_yend;
		uint32_t u32output_mb_ystart = 0;
		//uint32_t u32temp;
		int32_t rgb_pixel_size;

		if (dec->output_fmt == OUTPUT_FMT_YUV)
			rgb_pixel_size = RGB_PIXEL_SIZE_1;
		else if ( dec->output_fmt == OUTPUT_FMT_RGB888)
			rgb_pixel_size = RGB_PIXEL_SIZE_4;
		else
			rgb_pixel_size = RGB_PIXEL_SIZE_2;
		
		// width
		u32output_mb_start = dec->crop_x / PIXEL_Y;
		u32output_mb_end = (dec->crop_w / PIXEL_Y) + u32output_mb_start;
		// height
		u32output_mb_ystart = dec->crop_y / PIXEL_Y;
		u32output_mb_yend = (dec->crop_h / PIXEL_Y) + u32output_mb_ystart;

		src = (uint32_t) dec->output_base_ref_phy +
			u32output_mb_ystart * SIZE_Y * dec->output_stride +
			u32output_mb_start * PIXEL_Y;
		dst = (uint32_t) dec->output_base_phy;
		for (row = (u32output_mb_yend - u32output_mb_ystart) * PIXEL_Y; row > 0; row --) {
			nframe_dma(dec, src, dst, (u32output_mb_end - u32output_mb_start) * PIXEL_Y * rgb_pixel_size);
			src += dec->output_stride * rgb_pixel_size;
			dst += dec->output_stride * rgb_pixel_size;
		}
		if ( dec->output_fmt == OUTPUT_FMT_YUV) {
			// u
			src = (uint32_t) dec->output_base_u_ref_phy +
			u32output_mb_ystart * SIZE_U * dec->output_stride +
			u32output_mb_start * PIXEL_U;
			dst = (uint32_t) dec->output_base_u_phy;
			for (row = (u32output_mb_yend - u32output_mb_ystart) * PIXEL_U; row > 0; row --) {
				nframe_dma(dec,	src, dst, (u32output_mb_end - u32output_mb_start) * PIXEL_U);
				src += dec->output_stride / 2;
				dst += dec->output_stride / 2;
			}
			// v
			src = (uint32_t) dec->output_base_v_ref_phy +
				u32output_mb_ystart * SIZE_V * dec->output_stride +
				u32output_mb_start * PIXEL_V;
			dst = (uint32_t) dec->output_base_v_phy;
			for (row = (u32output_mb_yend - u32output_mb_ystart) * PIXEL_V; row > 0; row --) {
				nframe_dma(dec,	src, dst, (u32output_mb_end - u32output_mb_start) * PIXEL_V);
				src += dec->output_stride / 2;
				dst += dec->output_stride / 2;
			}
		}
	}
	return 0;
}


