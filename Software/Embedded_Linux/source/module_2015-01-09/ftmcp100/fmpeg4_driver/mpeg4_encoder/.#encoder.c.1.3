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
#include "encoder.h"
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


#define BIT_0ST		BIT0
#define BIT_1ST		BIT1
#define BIT_2ST		BIT2
#define BIT_3ST		BIT3
#define BIT_4ST		BIT4
#define BIT_5ST		BIT5
#define BIT_6ST		BIT6


//  rounded values for lambda param for weight of motion bits as in modified H.26L
//static int32_t lambda_vec16[32] =	
//static int8_t lambda_vec16[32] =	
int8_t lambda_vec16[32] =	
{ 0, (int) (1.00235 + 0.5), (int) (1.15582 + 0.5), (int) (1.31976 + 0.5),
		(int) (1.49591 + 0.5), (int) (1.68601 + 0.5),
	(int) (1.89187 + 0.5), (int) (2.11542 + 0.5), (int) (2.35878 + 0.5),
		(int) (2.62429 + 0.5), (int) (2.91455 + 0.5),
	(int) (3.23253 + 0.5), (int) (3.58158 + 0.5), (int) (3.96555 + 0.5),
		(int) (4.38887 + 0.5), (int) (4.85673 + 0.5),
	(int) (5.37519 + 0.5), (int) (5.95144 + 0.5), (int) (6.59408 + 0.5),
		(int) (7.31349 + 0.5), (int) (8.12242 + 0.5),
	(int) (9.03669 + 0.5), (int) (10.0763 + 0.5), (int) (11.2669 + 0.5),
		(int) (12.6426 + 0.5), (int) (14.2493 + 0.5),
	(int) (16.1512 + 0.5), (int) (18.442 + 0.5), (int) (21.2656 + 0.5),
		(int) (24.8580 + 0.5), (int) (29.6436 + 0.5),
	(int) (36.4949 + 0.5)
};


static void __inline 
i_dma_init(Encoder * pEnc)
{
	uint32_t * dma_cmd_tgl0 = (uint32_t *)(Mp4EncBase(pEnc) + DMA_ENC_CMD_OFF_0); //uint32_t * dma_cmd_tgl0 = (uint32_t *)(pEnc->u32VirtualBaseAddr + DMA_CMD_OFF_0);
	uint32_t * dma_cmd_tgl1 = (uint32_t *)(Mp4EncBase(pEnc) + DMA_ENC_CMD_OFF_1); //uint32_t * dma_cmd_tgl1 = (uint32_t *)(pEnc->u32VirtualBaseAddr + DMA_CMD_OFF_1);
	// image source
	IMAGE * rec = &pEnc->current1->reconstruct;

    #ifdef TWO_P_INT
		dma_cmd_tgl0[CHN_STORE_MBS] =
				mDmaSysMemAddr29b((uint32_t) pEnc->current1->mbs + MB_E_SIZE) |
				mDmaSysInc3b(DMA_INCS_48);		// 2 * MB_SIZE
		dma_cmd_tgl1[CHN_STORE_MBS] =
				mDmaSysMemAddr29b((uint32_t) pEnc->current1->mbs) |
				mDmaSysInc3b(DMA_INCS_48);		// 2 * MB_SIZE
	#endif

	// acdc preditor load/store
	dma_cmd_tgl0[CHN_STORE_PREDITOR] =
	dma_cmd_tgl0[CHN_LOAD_PREDITOR] =
			mDmaSysMemAddr29b(pEnc->pred_value_phy) |
			mDmaSysInc3b(DMA_INCS_128);
	// watch out
	dma_cmd_tgl0[CHN_LOAD_PREDITOR + 1] =
			mDmaLocMemAddr14b(PREDICTOR0_OFF);
	dma_cmd_tgl1[CHN_LOAD_PREDITOR + 1] =
			mDmaLocMemAddr14b(PREDICTOR4_OFF);

	// current reconstruct y, system memory address
	dma_cmd_tgl0[CHN_CUR_RE_Y] =
			mDmaSysMemAddr29b(rec->y_phy) |
			mDmaSysInc3b (DMA_INCS_512);
	dma_cmd_tgl1[CHN_CUR_RE_Y] =
			mDmaSysMemAddr29b(rec->y_phy + SIZE_Y) |
			mDmaSysInc3b (DMA_INCS_512);
	// current reconstruct u
	dma_cmd_tgl0[CHN_CUR_RE_U] =
			mDmaSysMemAddr29b(rec->u_phy) |
			mDmaSysInc3b (DMA_INCS_128);
	dma_cmd_tgl1[CHN_CUR_RE_U] =
			mDmaSysMemAddr29b(rec->u_phy + SIZE_U) |
			mDmaSysInc3b (DMA_INCS_128);
	// current reconstruct v
	dma_cmd_tgl0[CHN_CUR_RE_V] =
			mDmaSysMemAddr29b(rec->v_phy) |
			mDmaSysInc3b (DMA_INCS_128);
	dma_cmd_tgl1[CHN_CUR_RE_V] =
			mDmaSysMemAddr29b(rec->v_phy + SIZE_V) |
			mDmaSysInc3b (DMA_INCS_128);

	// current reconstruct buffer, local memory address, only ping-pong buffer
	dma_cmd_tgl0[CHN_CUR_RE_Y + 1] = mDmaLocMemAddr14b(INTER_Y0);
	dma_cmd_tgl0[CHN_CUR_RE_U + 1] = mDmaLocMemAddr14b(INTER_U0);
	dma_cmd_tgl0[CHN_CUR_RE_V + 1] = mDmaLocMemAddr14b(INTER_V0);
	dma_cmd_tgl1[CHN_CUR_RE_Y + 1] = mDmaLocMemAddr14b(INTER_Y1);
	dma_cmd_tgl1[CHN_CUR_RE_U + 1] = mDmaLocMemAddr14b(INTER_U1);
	dma_cmd_tgl1[CHN_CUR_RE_V + 1] = mDmaLocMemAddr14b(INTER_V1);
}

static void __inline
p_dma_init(Encoder * pEnc)
{
	uint32_t * dma_cmd_tgl0 = (uint32_t *)(Mp4EncBase(pEnc) + DMA_ENC_CMD_OFF_0); //uint32_t * dma_cmd_tgl0 = (uint32_t *)(pEnc->u32VirtualBaseAddr + DMA_CMD_OFF_0);
	uint32_t * dma_cmd_tgl1 = (uint32_t *)(Mp4EncBase(pEnc) + DMA_ENC_CMD_OFF_1); //uint32_t * dma_cmd_tgl1 = (uint32_t *)(pEnc->u32VirtualBaseAddr + DMA_CMD_OFF_1);

	#ifdef TWO_P_INT
	dma_cmd_tgl0[CHN_STORE_MBS] =
		mDmaSysMemAddr29b((uint32_t) pEnc->current1->mbs + MB_E_SIZE)
		|mDmaSysInc3b(DMA_INCS_48);		// 2 * MB_E_SIZE
	dma_cmd_tgl1[CHN_STORE_MBS] =
		mDmaSysMemAddr29b((uint32_t) pEnc->current1->mbs)
		|mDmaSysInc3b(DMA_INCS_48);		// 2 * MB_E_SIZE
	dma_cmd_tgl0[CHN_LOAD_REF_MBS] =
		mDmaSysMemAddr29b((uint32_t) pEnc->reference->mbs)
		|mDmaSysInc3b(DMA_INCS_48);		// 2 * MB_E_SIZE
	dma_cmd_tgl1[CHN_LOAD_REF_MBS] =
		mDmaSysMemAddr29b((uint32_t) pEnc->reference->mbs + MB_E_SIZE)
		|mDmaSysInc3b(DMA_INCS_48);		// 2 * MB_E_SIZE
	#endif

	// acdc preditor load/store
	dma_cmd_tgl0[CHN_STORE_PREDITOR] =
	dma_cmd_tgl0[CHN_LOAD_PREDITOR] =
		mDmaSysMemAddr29b(pEnc->pred_value_phy)
		| mDmaSysInc3b(DMA_INCS_128);
	
	dma_cmd_tgl1[CHN_STORE_PREDITOR] =
	dma_cmd_tgl1[CHN_LOAD_PREDITOR] =
		mDmaSysMemAddr29b(pEnc->pred_value_phy)
		| mDmaSysInc3b(DMA_INCS_128);
	// watch out, different with i_dma_init
	dma_cmd_tgl0[CHN_LOAD_PREDITOR + 1] =
		mDmaLocMemAddr14b(PREDICTOR4_OFF);
	dma_cmd_tgl1[CHN_LOAD_PREDITOR + 1] =
		mDmaLocMemAddr14b(PREDICTOR0_OFF);

	// reference reconstruct 
	{
		IMAGE * ref = &pEnc->reference->reconstruct;

		dma_cmd_tgl0[CHN_REF_RE_Y] =
			mDmaSysMemAddr29b((uint32_t) ref->y_phy - (pEnc->mb_width * SIZE_Y)) |
	 		mDmaSysInc3b (DMA_INCS_512);
		dma_cmd_tgl1[CHN_REF_RE_Y] =
			mDmaSysMemAddr29b((uint32_t) ref->y_phy - (pEnc->mb_width * SIZE_Y) + SIZE_Y) |
	 		mDmaSysInc3b (DMA_INCS_512);

		dma_cmd_tgl0[CHN_REF_RE_U] =
			mDmaSysMemAddr29b((uint32_t) ref->u_phy - (pEnc->mb_width * SIZE_U)) |
	 		mDmaSysInc3b (DMA_INCS_128);
		dma_cmd_tgl1[CHN_REF_RE_U] =
			mDmaSysMemAddr29b((uint32_t) ref->u_phy - (pEnc->mb_width * SIZE_U) + SIZE_U) |
	 		mDmaSysInc3b (DMA_INCS_128);

		dma_cmd_tgl0[CHN_REF_RE_V] =
			mDmaSysMemAddr29b((uint32_t) ref->v_phy - (pEnc->mb_width * SIZE_V)) |
	 		mDmaSysInc3b (DMA_INCS_128);
		dma_cmd_tgl1[CHN_REF_RE_V] =
			mDmaSysMemAddr29b((uint32_t) ref->v_phy - (pEnc->mb_width * SIZE_V) + SIZE_V) |
	 		mDmaSysInc3b (DMA_INCS_128);
	}

	// current reconstruct 
	{
		IMAGE * rec = &pEnc->current1->reconstruct;
        		// current reconstruct y, system memory address
		dma_cmd_tgl0[CHN_CUR_RE_Y] =
			mDmaSysMemAddr29b(rec->y_phy) |
 			mDmaSysInc3b (DMA_INCS_512);
		dma_cmd_tgl1[CHN_CUR_RE_Y] =
			mDmaSysMemAddr29b(rec->y_phy + SIZE_Y) |
 			mDmaSysInc3b (DMA_INCS_512);
		// current reconstruct u
		dma_cmd_tgl0[CHN_CUR_RE_U] =
			mDmaSysMemAddr29b(rec->u_phy) |
	 		mDmaSysInc3b (DMA_INCS_128);
		dma_cmd_tgl1[CHN_CUR_RE_U] =
			mDmaSysMemAddr29b(rec->u_phy + SIZE_U) |
	 		mDmaSysInc3b (DMA_INCS_128);
		// current reconstruct v
		dma_cmd_tgl0[CHN_CUR_RE_V] =
			mDmaSysMemAddr29b(rec->v_phy) |
 			mDmaSysInc3b (DMA_INCS_128);
		dma_cmd_tgl1[CHN_CUR_RE_V] =
			mDmaSysMemAddr29b(rec->v_phy + SIZE_V) |
 			mDmaSysInc3b (DMA_INCS_128);
	}
}

/*****************************************************************************
 * "original" IP frame encoder entry point
 *
 * Returned values :
 *    - void
 ****************************************************************************/

static __inline uint32_t
predict_acdc_I(const MACROBLOCK_E_b * mbb,
			const int32_t resync_marker)
{
	uint32_t u32temp = MCCTL_MCGO;

	if (mbb->toggle)
		u32temp |= MCCTL_REMAP;

	// left macroblock valid ?
	if (mbb->x == 0)
		u32temp |= (MCCTL_ACDC_L | MCCTL_ACDC_D);

	// diag macroblock valid ?
	// top macroblock valid ?
	if ((resync_marker) || (mbb->y == 0))
		u32temp |= (MCCTL_ACDC_T | MCCTL_ACDC_D);
	return u32temp;
}

#ifdef TWO_P_INT
static __inline uint32_t
predict_acdc_P(const MACROBLOCK_E * pCurRowMBs,
            const MACROBLOCK_E * pUpperRowMBs,
			const MACROBLOCK_E_b * mbb,
			const int32_t mb_width,
			const int32_t resync_marker)
{
    const int32_t mbpos = mbb->mbpos;
	uint32_t u32temp = MCCTL_MCGO |
					(MODE_INTRA << 6) |
					MCCTL_INTRA;

	if (!mbb->toggle)
		u32temp |= MCCTL_REMAP;

	// left macroblock valid ?
	if (mbb->x == 0)
		u32temp |= MCCTL_ACDC_L | MCCTL_ACDC_D;
	//else if (pCurRowMBs[mbpos - 1].mode != MODE_INTRA)
	else if (pCurRowMBs[(mbpos-1)%ENC_MBS_LOCAL_P].mode != MODE_INTRA)
		u32temp |= MCCTL_ACDC_L;

    if (resync_marker) {
        // top and diag macroblock are invalid since our resync marker is inserted on each row
        u32temp |= MCCTL_ACDC_D | MCCTL_ACDC_T;
    } else {
	  // diag macroblock valid ?
	  if (mbb->y == 0 || mbb->x == 0) // we use this condition to avoid 'mbpos - 1 - mb_width' out of range
        u32temp |= MCCTL_ACDC_D;
      //else if(pMBs[mbpos - 1 - mb_width].mode != MODE_INTRA)      
      else if(pUpperRowMBs[(mbpos - 1 - mb_width)%ENC_MBS_ACDC].mode != MODE_INTRA)
        u32temp |= MCCTL_ACDC_D;

      // top macroblock valid ?
      if (mbb->y == 0) // we use this condition to avoid 'mbpos - mb_width' out of range
        u32temp |= MCCTL_ACDC_T;
      //else if(pMBs[mbpos - mb_width].mode != MODE_INTRA)
      else if(pUpperRowMBs[(mbpos - mb_width)%ENC_MBS_ACDC].mode != MODE_INTRA)
        u32temp |= MCCTL_ACDC_T;
    }
	return u32temp;

}
#else // #ifdef TWO_P_INT
static __inline uint32_t
predict_acdc_P(const MACROBLOCK_E * pMBs,
			const MACROBLOCK_E_b * mbb,
			const int32_t mb_width,
			const int32_t resync_marker)
{
    const int32_t mbpos = mbb->mbpos;
	uint32_t u32temp = MCCTL_MCGO |
					(MODE_INTRA << 6) |
					MCCTL_INTRA;

	if (!mbb->toggle)
		u32temp |= MCCTL_REMAP;

	// left macroblock valid ?
	if (mbb->x == 0)
		u32temp |= MCCTL_ACDC_L | MCCTL_ACDC_D;
	else if (pMBs[mbpos - 1].mode != MODE_INTRA)
		u32temp |= MCCTL_ACDC_L;

    if (resync_marker) {
        // top and diag macroblock are invalid since our resync marker is inserted on each row
        u32temp |= MCCTL_ACDC_D | MCCTL_ACDC_T;
    } else {
	  // diag macroblock valid ?
	  if (mbb->y == 0 || mbb->x == 0) // we use this condition to avoid 'mbpos - 1 - mb_width' out of range
        u32temp |= MCCTL_ACDC_D;
      else if(pMBs[mbpos - 1 - mb_width].mode != MODE_INTRA)
        u32temp |= MCCTL_ACDC_D;

      // top macroblock valid ?
      if (mbb->y == 0) // we use this condition to avoid 'mbpos - mb_width' out of range
        u32temp |= MCCTL_ACDC_T;
      else if(pMBs[mbpos - mb_width].mode != MODE_INTRA)
        u32temp |= MCCTL_ACDC_T;
    }
	return u32temp;
/*
	const int32_t mbpos = mbb->mbpos;
	int32_t bound;
	uint32_t u32temp = MCCTL_MCGO |
					(MODE_INTRA << 6) |
					MCCTL_INTRA;

	bound = mbpos;
	if (resync_marker)
		bound = mbpos - mbb->x;

	if (mbb->toggle)
		u32temp |= MCCTL_REMAP;

	// left macroblock valid ?
	if (mbb->x == 0)
		u32temp |= MCCTL_ACDC_L | MCCTL_ACDC_D;
	else if (pMBs[mbpos - 1].mode != MODE_INTRA)
		u32temp |= MCCTL_ACDC_L;

	// diag macroblock valid ?
	if (mbb->y == 0)
		u32temp |= MCCTL_ACDC_D | MCCTL_ACDC_T;
	// Put the following 2 condiction into together may casue an illegal memory access
	else if (mbpos < (bound + mb_width + 1))
		u32temp |= MCCTL_ACDC_D;
	else if(pMBs[mbpos - 1 - mb_width].mode != MODE_INTRA)
		u32temp |= MCCTL_ACDC_D;

	// top macroblock valid ?
	// Put the following 2 condiction into together may casue an illegal memory access
	if (mbpos < (bound + mb_width))
		u32temp |= MCCTL_ACDC_T;
	else if(pMBs[mbpos - mb_width].mode != MODE_INTRA)
		u32temp |= MCCTL_ACDC_T;

	return u32temp;
*/
}
#endif // #ifdef TWO_P_INT

/*
				<-  mb_img	    -><- mb_mc	   -><- mb_rec	   ->
								<- mb_vlc	   ->
<-00st stage	    -><-  1st stage      -><-  2st stage      -><-  3st stage      ->
----------------------------cycle start -----------------------------------------------
								(10)goMC_mc
				(3)goDMA_LD------(11)goDMA_ST---(14)goDMA_recon(3S-x)
				(4)goDMA_img
~~change to next mb~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
(0)preMoveImg(x-0S)
								 (11')preMoveRecon(2S-3S)
								 (11'')wait VLC(3S)
				(5)(1S-2S)
(1)(0S-1S)
(2)preLoadACDC
				(6)preStoreACDC
								(12)wait MC(3S)
				(7)preMC(2S)
				(8)waitDMA_LD----(13)waitDMA_ST---(15)waitDMA_recon
				(9)waitDMA_img
----------------------------cycle end --------------------------------------------------
*/

void encoder_iframe(Encoder_x * pEnc_x)
{
	Encoder *pEnc = &(pEnc_x->Enc);
	volatile MDMA * ptDma = (volatile MDMA *)(Mp4EncBase(pEnc) + DMA_OFF); //volatile MDMA * ptDma = (volatile MDMA *)(pEnc->u32VirtualBaseAddr + DMA_OFF);
	volatile MP4_t * ptMP4 = pEnc->ptMP4;

	uint32_t * dma_cmd_tgl0 = (uint32_t *)(Mp4EncBase(pEnc) + DMA_ENC_CMD_OFF_0); //uint32_t * dma_cmd_tgl0 = (uint32_t *)(pEnc->u32VirtualBaseAddr + DMA_CMD_OFF_0);
	uint32_t * dma_cmd_tgl1 = (uint32_t *)(Mp4EncBase(pEnc) + DMA_ENC_CMD_OFF_1); //uint32_t * dma_cmd_tgl1 = (uint32_t *)(pEnc->u32VirtualBaseAddr + DMA_CMD_OFF_1);
	uint32_t * dma_cmd_tgl;
	int32_t i;

	uint32_t u32cmd_mc= 0;
	uint32_t u32cmd_mc_reload;
	uint32_t u32grpc;
	uint32_t u32pipe = 0;

	#ifdef TWO_P_INT
	MACROBLOCK_E mbs_local1[ENC_MBS_LOCAL_I + 1];
	MACROBLOCK_E * mbs_local;
	#endif	
	MACROBLOCK_E *mb_img;
	MACROBLOCK_E *mb_mc;
	MACROBLOCK_E_b mbb[ENC_I_FRAME_PIPE];
	MACROBLOCK_E_b *mbb_img = &mbb[ENC_I_FRAME_PIPE - 1];
	MACROBLOCK_E_b *mbb_mc;

    #ifdef TWO_P_INT
	// align to 8 bytes
	mbs_local = (MACROBLOCK_E *)(((uint32_t)&mbs_local1[0] + 7) & ~0x7);
	#endif
    u32cmd_mc_reload =
		#ifndef GM8120
	    	MCCTL_ENC_MODE |
	    #endif
		MCCTL_INTRA_F |
		(pEnc->bH263 << 14) |
		(MODE_INTRA << 6) |
		(pEnc->bMp4_quant << 3) |
	  	MCCTL_INTRA;
	if ( pEnc->ac_disable ==1)
		u32cmd_mc_reload |= MCCTL_DIS_ACP;

	i_dma_init (pEnc);
	// the last command in the chain never skipped.
	u32grpc =
		(2 << (ID_CHN_REF_REC*2)) |		// REF_REC dma: disable
		//(0 << (ID_CHN_IMG*2)) |					// current image dma: enable
		(2 << (ID_CHN_IMG*2)) |					// current image dma: disable
		(2 << (ID_CHN_CUR_REC_Y*2)) |		// CUR_REC_Y dma: disable
		(2 << (ID_CHN_CUR_REC_UV*2))	 |	// CUR_REC_UV dma: disable
		#ifdef TWO_P_INT
		(2 << (ID_CHN_STORE_MBS * 2))	     |  // disable MBS dma
		(2 << (ID_CHN_ACDC_MBS  * 2))	     |  // disable acdc MBS dma
		(2 << (ID_CHN_LOAD_REF_MBS  * 2))	 |  // disable ref MBS dma
        		#endif
		(3 << (ID_CHN_ACDC*2));			// St_Ld_ACDC dma: sync to MC_done
		
        	#ifdef TWO_P_INT
        	mb_img = mb_mc = &mbs_local[0];
        	#else
        	mb_img = mb_mc = &pEnc->current1->mbs[0];
        	#endif

	/* motion dection */
	pEnc->current1->activity0 = 0;
	pEnc->current1->activity1 = 0;
	pEnc->current1->activity2 = 0;
	
	mbb_img->mbpos = -1;
	mbb_img->x = -1;
	mbb_img->y = 0;
	mbb_img->toggle = -1;
	i = -1;

	ptDma->GRPS =
		(2 << (ID_CHN_CUR_REC_Y*2))	 |	// ID_CHN_CUR_REC_Y dma: sync to MC_don
		(2 << (ID_CHN_CUR_REC_UV*2))	 |	// CUR_REC_UV dma: sync to MC_don
		#ifdef TWO_P_INT
		(2 << (ID_CHN_STORE_MBS * 2))	     |  // MBS dma : sync to MC done
		(2 << (ID_CHN_ACDC_MBS  * 2))	     |  // acdc MBS dma : sync to MC done
      		(2 << (ID_CHN_LOAD_REF_MBS  * 2))	 |  // ref MBS dma : sync to MC done
		#endif
		(2 << (ID_CHN_ACDC*2));			// St_Ld_ACDC dma: sync to MC_done
#if 0
{
	int i;
	volatile int * wrpreg = (volatile int *)pEnc->u32BaseAddr_wrapper;
	for (i = 0; i <= 0x20; i += 4) {
		printk("wrp %x: 0x%x\n", (int)wrpreg, *wrpreg);
		wrpreg ++;
	}
}
#endif
	while (1) {
//(10)goMC_mc
		if (u32cmd_mc & MCCTL_MCGO)
			ptMP4->MCCTL = u32cmd_mc;

//(3)goDMA_LD------(11)goDMA_ST---(14)goDMA_recon(3S-x)
//(4)goDMA_img
		if (mbb_img->toggle == -1) {		// first time
			pEnc->current1->coding_type = I_VOP;
			if (pEnc->bH263) {
				pEnc->bRounding_type =
				pEnc->current1->bRounding_type = 0;
				BitstreamWriteShortHeader(pEnc, pEnc->current1, 0);
			} else {
				pEnc->bRounding_type =
				pEnc->current1->bRounding_type = 1;
				if ( pEnc_x->vol_header & 0x10000000)
					BitstreamWriteVolHeader(pEnc, pEnc->current1);
				BitstreamWriteVopHeader(pEnc, pEnc->current1, 1);
			}
		} else { // not first time
			u32pipe &= ~BIT_3ST;
			if (mbb_img->toggle == 0)
				ptDma->CCA = (DMA_ENC_CMD_OFF_0 + CHN_IMG_Y * 4) | DMA_LC_LOC;
			else
				ptDma->CCA = (DMA_ENC_CMD_OFF_1 + CHN_IMG_Y * 4) | DMA_LC_LOC;
			ptDma->GRPC = u32grpc;
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
		}

//~~change to next mb~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		///////////////////////////////////////////////////////////////////////////
		// update mb pointer
		i ++;
		mbb_mc = mbb_img;
		mbb_img = &mbb[i%ENC_I_FRAME_PIPE];
		mbb_img->toggle = i & BIT0;
		mbb_img->mbpos = mbb_mc->mbpos + 1;
		mbb_img->x = mbb_mc->x + 1;
		mbb_img->y = mbb_mc->y;
		if (mbb_img->x == pEnc->mb_width) {
			mbb_img->x = 0;
			mbb_img->y ++;
		}
		if (mbb_img->toggle == 0)
			dma_cmd_tgl = dma_cmd_tgl0;
		else
			dma_cmd_tgl = dma_cmd_tgl1;
		mb_mc = mb_img;

		u32grpc =
			(2 << (ID_CHN_REF_REC*2)) |		// REF_REC dma: disable
			//(0 << (ID_CHN_IMG*2)) |			// current image dma: enable
			(2 << (ID_CHN_IMG*2)) | 		// current image dma: disable
			(2 << (ID_CHN_CUR_REC_Y*2)) |		// CUR_REC_Y dma: disable
			(2 << (ID_CHN_CUR_REC_UV*2))	 |	// CUR_REC_UV dma: disable
			#ifdef TWO_P_INT
	        		(2 << (ID_CHN_STORE_MBS * 2))	     |  // disable MBS dma
	        		(2 << (ID_CHN_ACDC_MBS  * 2))	     |  // disable acdc MBS dma
      	        		(2 << (ID_CHN_LOAD_REF_MBS  * 2))	 |  // disable ref MBS dma
			#endif
			(3 << (ID_CHN_ACDC*2));			// St_Ld_ACDC dma: sync to MC_done

//(0)preMoveImg(x-0S)
		if (mbb_img->y < pEnc->mb_height) {
			u32pipe |= BIT_0ST;
			u32grpc &= ~(uint32_t)(3 << (ID_CHN_IMG * 2));		// exec IMG dma
			// init mb_img
			#ifdef TWO_P_INT
	  		mb_img = &mbs_local[(mbb_img->mbpos)%ENC_MBS_LOCAL_I];
			#else
	  		mb_img = &pEnc->current1->mbs[mbb_img->mbpos];
			#endif
			// init quant from frame-quant
			mb_img->quant = pEnc->current1->quant;
			// init mode
			mb_img->mode = MODE_INTRA;
			if(pEnc->bEnable_4mv) {
				mb_img->mvs[0].u32num = 0;
				mb_img->mvs[1].u32num = 0;
				mb_img->mvs[2].u32num = 0;
			}
			mb_img->mvs[3].u32num = 0;
	  		if (pEnc->img_fmt <= 1) {	// H264_2D/MP4_2D
	    		if (mbb_img->x == 0 || mbb_img->x == 1) {
					IMAGE * img = &pEnc->current1->image;
	      			unsigned int mb_pos = mbb_img->y * pEnc->mb_stride + mbb_img->x + pEnc->mb_offset;
	      			// y
	      			dma_cmd_tgl[CHN_IMG_Y] =
 	        				mDmaSysMemAddr29b(img->y_phy + mb_pos * SIZE_Y) |
 	        				mDmaSysInc3b (DMA_INCS_512);		// 2 * SIZE_Y
 	        		if (pEnc->img_fmt == 1) {	// MP4_2D
						// u
						dma_cmd_tgl[CHN_IMG_U] =
								mDmaSysMemAddr29b(img->u_phy + mb_pos * SIZE_U) |
								mDmaSysInc3b (DMA_INCS_128);		// 2 * SIZE_U
						// v
						dma_cmd_tgl[CHN_IMG_V] =
								mDmaSysMemAddr29b(img->v_phy + mb_pos * SIZE_V) |
								mDmaSysInc3b (DMA_INCS_128);		// 2 * SIZE_V
 	        		}
					else {									// H264_2D
							// u
							dma_cmd_tgl[CHN_IMG_U] =
									mDmaSysMemAddr29b(img->u_phy + mb_pos * SIZE_U*2) |
									mDmaSysInc3b (DMA_INCS_256);		// 4 * SIZE_U
 	        		}
	    		}
	  		} 
		}
//(11')preMoveRecon(2S-3S)
//(11'')wait VLC(3S)
		if (u32pipe & BIT_2ST) {
			u32pipe &= ~BIT_2ST;
			u32pipe |= BIT_3ST;
			u32grpc &= ~(uint32_t)
			((3 << (ID_CHN_CUR_REC_Y * 2)) |		// exec CUR_REC_Y dma
			(3 << (ID_CHN_CUR_REC_UV * 2)));	// exec CUR_REC_UV dma
			// wait for VLC_done
			while ((ptMP4->CPSTS & BIT15) == 0)
				;
		}
//(5)(1S-2S)
		if (u32pipe & BIT_1ST) {
			u32pipe &= ~BIT_1ST;
			u32pipe |= BIT_2ST;
		}
//(1)(0S-1S)
		if (u32pipe & BIT_0ST) {
			u32pipe &= ~BIT_0ST;
			u32pipe |= BIT_1ST;
		}
//(2)preLoadACDC
//(6)preStoreACDC
		if (mbb_mc->x == 0) {
			dma_cmd_tgl[CHN_STORE_PREDITOR] =
				mDmaSysMemAddr29b((uint32_t)pEnc->pred_value_phy)
				| mDmaSysInc3b(DMA_INCS_128);
			dma_cmd_tgl[CHN_LOAD_PREDITOR] =
				mDmaSysMemAddr29b((uint32_t)pEnc->pred_value_phy + 64)
				| mDmaSysInc3b(DMA_INCS_128);
		} else if (mbb_mc->x == 1) {
			dma_cmd_tgl[CHN_STORE_PREDITOR] =
				mDmaSysMemAddr29b((uint32_t)pEnc->pred_value_phy + 64)
				| mDmaSysInc3b(DMA_INCS_128);
		}
		if (mbb_mc->x == (pEnc->mb_width - 1)) {
			dma_cmd_tgl[CHN_LOAD_PREDITOR] =
				mDmaSysMemAddr29b((uint32_t)pEnc->pred_value_phy)
				| mDmaSysInc3b(DMA_INCS_128);
		}
//(12)wait MC(3S)
		if (u32pipe & BIT_3ST) {			
				
			// When MC engine starts, it will also invoke AC_DC engine and then VLC engine.
			// Therefore, in some occasions (especially when quantization value is equal to 1),
			// MC_done did not mean the VLC engine is also done. 
			// VLC_done :  ------------------                     ---------------
			//                              |_____________________|
			// AC_done  :  -----            -------------------------------------
			//                 |____________|
			// MC_done  :  -----                              -------------
			//                 |______________________________|           |______
			//
			// From the illustration shown above, you can see it is possible the previous 
			// (wait VLC(3S)) might miss the vlc_done check since VLC engine has not even
			// started yet.
			// So we should check the MC done with VLC done together in order to avoid errors.
			
			// wait for MC done and VLC_done			
			while ((ptMP4->CPSTS & (BIT1|BIT15)) != (BIT1|BIT15))
				;				  
		}
	    
//(7)preMC(2S)
		u32cmd_mc = u32cmd_mc_reload;
		if (u32pipe & BIT_2ST) {
			u32cmd_mc |= predict_acdc_I (mbb_mc, pEnc->bResyncMarker);

			ptMP4->QCR0 = mb_mc->quant << 18;
			// set the MC Interpolation and Result Block Start Address.
			// set the MC Current Block Start Address.
			if (mbb_mc->toggle) {
				ptMP4->MCIADDR = INTER_Y1;
	 			ptMP4->MCCADDR = CUR_Y1;
			} else {
	 			ptMP4->MCIADDR = INTER_Y0;
				ptMP4->MCCADDR = CUR_Y0;
			}
			if ((pEnc->bResyncMarker) && (mbb_mc->y != 0) && (mbb_mc->x == 0)) {
				if (pEnc->bH263) {
					BitstreamPutBits(ptMP4, VIDO_RESYN_MARKER, 17);
					BitstreamPutBits(ptMP4, mbb_mc->y, 5);
					BitstreamPutBits(ptMP4, 0, 2);		// ID
					BitstreamPutBits(ptMP4, mb_mc->quant, 5);
				} else	{
					BitstreamPadAlways(ptMP4);
					BitstreamPutBits(ptMP4, VIDO_RESYN_MARKER, 17);
					BitstreamPutBits(ptMP4, mbb_mc->mbpos, log2bin(pEnc->mb_width *  pEnc->mb_height - 1));
					BitstreamPutBits(ptMP4, mb_mc->quant, 5);
					BitstreamPutBit(ptMP4, 0);
				}
			}
			
			#ifdef TWO_P_INT
			u32grpc &= ~(uint32_t)(3 << (ID_CHN_STORE_MBS * 2));		// exec STORE MBS dma			  
			u32grpc |= (3 << (ID_CHN_STORE_MBS * 2)); // and make it sync to MC done
	  		// assign start address of local memory
	  		dma_cmd_tgl[CHN_STORE_MBS + 1] =
				mDmaLocMemAddr14b(&mbs_local[(i-1) % ENC_MBS_LOCAL_I]);
			#endif
		}
//(8)waitDMA_LD----(13)waitDMA_ST---(15)waitDMA_recon
//(9)waitDMA_img
// wait for  DMA_done
		mPOLL_MARKER_S(ptMP4);
		while((ptDma->Status & 0x1) == 0)
			;
		mPOLL_MARKER_E(ptMP4);

		if (u32pipe == 0)
			break;
	}
	BitstreamPadAlways(pEnc->ptMP4);
	pEnc->frame_counter++;
	
	if (pEnc->frame_counter > pEnc->md_interval)
		pEnc->frame_counter = 0;
}


void 
encoder_nframe(Encoder_x * pEnc_x) //FrameCodePNotCoded(Encoder * pEnc, bool vol_header)
{
    bool vol_header = pEnc_x->vol_header & 0xFF;
    Encoder *pEnc = &(pEnc_x->Enc);
    
	/* IMAGE *pCurrent = &pEnc->current1->image; */
	pEnc->bRounding_type = 1 - pEnc->bRounding_type;
	pEnc->current1->bRounding_type = pEnc->bRounding_type;

	/* motion dection */
	pEnc->current1->activity0 = 0;
	pEnc->current1->activity1 = 0;
	pEnc->current1->activity2 = 0;
	pEnc->frame_counter++;
	
	pEnc->current1->coding_type = N_VOP;
	if (vol_header )
		BitstreamWriteVolHeader(pEnc, pEnc->current1);
	BitstreamWriteVopHeader(pEnc, pEnc->current1, 0);
}

void 
me_cmd_each_init (Encoder * pEnc) {
	volatile uint32_t * me_cmd = (volatile uint32_t *) (Mp4EncBase(pEnc) + ME_CMD_Q_OFF); //volatile uint32_t * me_cmd = (volatile uint32_t *) (pEnc->u32VirtualBaseAddr + ME_CMD_Q_OFF);
	// me_cmd[24] ~ [28]
	if (pEnc->bEnable_4mv) {
		me_cmd[24] =
			mMECMD_TYPE3b(MEC_REF) |
			MECREF_4MV |
			mMECREF_MINSADTH16b(ThEES1);
		me_cmd[25] =
			mMECMD_TYPE3b(MEC_PMVX) |
			mMECPMV_4MVBN2b(0);
		me_cmd[26] =
			mMECMD_TYPE3b(MEC_PMVY) |
			mMECPMV_4MVBN2b(0);
		me_cmd[27] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_MPMV |
			mMECSAD_RADD12b(RADDR);
	}
	else {
		me_cmd[24] =
			mMECMD_TYPE3b(MEC_REF) |
			mMECREF_MINSADTH16b(ThEES1);
        // Tuba 20111209_1 start
        if (pEnc->disableModeDecision) {
    		me_cmd[25] =
    			mMECMD_TYPE3b(MEC_MOD) |
    			mMECMOD_INTRASADTH16b(MV16_INTER_BIAS);
        }
        else {
       		me_cmd[25] =
    			mMECMD_TYPE3b(MEC_MOD) |
    			MECMOD_EN |
    			mMECMOD_INTRASADTH16b(MV16_INTER_BIAS);
        }
        // Tuba 20111209_1 end
		me_cmd[26] =
			mMECMD_TYPE3b(MEC_PXI);
		me_cmd[27] =
			mMECMD_TYPE3b(MEC_PXI) |
			MECPXI_UV |
			mMECPXI_RADD12b((REF_U + 32*8) >> 2);
		me_cmd[28] =
			mMECMD_TYPE3b(MEC_PXI) |
			MECPXI_E_LAST |
			MECPXI_UV |
			mMECPXI_RADD12b(((uint32_t) (REF_V + 32*8)) >> 2);
	}
}

// dma
//
// toggle		0			0				1					0
//		--------	-------------	-------------	------------------------
//		|Ref		|	|Ref + img	|	|Ref + img	|	|Ref + img + rec + acdc	|	...
//-------		-----			-----			----						---
//mc_done
//
//--------------------------------------------------					----
//													|					|	 ...
//													---------------------
//
//me_done
//
//----------------------------------			----					----
//									|			|	|					|	 ...
//									-------------	---------------------

/*p-frame
mc_		|			|				| goMC		| storeCRec
toggle	|LoadRRec	| loadACDC		| storeACDC	|
		|			| img			|
===============================================================
-1		|
0		|RRec0d		|
1		|RRec1d		| LD1d(img1d)		|
0		|RRec0d		| LD0d(img0d)		| R1(M1)		|
1		|RRec1d		| LD1d(img1d)		| R0(M0)		| CRec1d
0		|RRec0d		| LD0d(img0d)		| R1(M1)		| CRec0d			... c
1		|RRec1d		| LD1d(img1d)		| R0(M0)		| CRec1d			... d
===============================================================
*/
/*
				<-  mb_ref	    -><-  mb_img	    -><- mb_me	   -><- mb_mc	   -><- mb_rec	   ->
																<- mb_vlc	   ->
<-00st stage	    -><-  1st stage      -><-  2st stage        -><-  3st stage      -><-  4st stage      -><-  5st stage      ->
----------------------------cycle start -----------------------------------------------
																(13)goMC_mc(4S)
												(12')goME(3S)
				(1)goDMA_Ref-----(6)goDMA_LD---------------------(14)goDMA_ST---(19)goDMA_recon(5S-x)
								(7)goDMA_img
~~change to next mb~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
				(2)preMoveImg(0S-1S)
(0)preMoveRef(x-0S)
																(15)preMoveRecon(4S)
																(16)wait VLC(4S)
								(9)preLoadACDC---(12-1)preStoreACDC(4S)
																(17)wait MC(4S-5S)
												(12-2)waitME_done(3S)
												(12-3)preMC(3S-4S)
								(10)preME(2S)
								(8)(2S-3S)
				(3)(1S-2S)
				(5)waitDMA_Ref----(11)waitDMA_LD-------------------(18)waitDMA_ST---(20)waitDMA_recon
								(12)waitDMA_img
----------------------------cycle end --------------------------------------------------
*/

#ifdef MOTION_DETECTION_IN_DRV
void 
do_motion_dection_1mv(MACROBLOCK_E *mb,MACROBLOCK_E_b *mbb,Encoder *pEnc)
{
    volatile MP4_t *ptMP4 = pEnc->ptMP4;
    unsigned int sad16=0;
    unsigned int dev=0;

    dev=ptMP4->CURDEV;	
	if((pEnc->dev_flag)&&(pEnc->frame_counter>=pEnc->md_interval))
	{
	    sad16=ptMP4->MIN_SADMV;
	    if((mbb->x>=pEnc->range_mb_x0_LU)&&(mbb->y>=pEnc->range_mb_y0_LU)&&(mbb->x<pEnc->range_mb_x0_RD)&&(mbb->y<pEnc->range_mb_y0_RD))
		{
			if (((mb->mvs[3].vec.s16x*mb->mvs[3].vec.s16x+mb->mvs[3].vec.s16x*mb->mvs[3].vec.s16x)>pEnc->MV_th0)&&(sad16>pEnc->sad_th0))
				pEnc->current1->activity0++;
			else if (abs(pEnc->dev_buffer[mbb->mbpos-1]-dev)>pEnc->delta_dev_th0)
				pEnc->current1->activity0++;
		}
		if((mbb->x>=pEnc->range_mb_x1_LU)&&(mbb->y>=pEnc->range_mb_y1_LU)&&(mbb->x<pEnc->range_mb_x1_RD)&&(mbb->y<pEnc->range_mb_y1_RD))
		{
			if (((mb->mvs[3].vec.s16x*mb->mvs[3].vec.s16x+mb->mvs[3].vec.s16x*mb->mvs[3].vec.s16x)>pEnc->MV_th1)&&(sad16>pEnc->sad_th1))
				pEnc->current1->activity1 ++;
			else if (abs(pEnc->dev_buffer[mbb->mbpos-1]-dev)>pEnc->delta_dev_th1)
				pEnc->current1->activity1 ++;
		}
		if((mbb->x>=pEnc->range_mb_x2_LU)&&(mbb->y>=pEnc->range_mb_y2_LU)&&(mbb->x<pEnc->range_mb_x2_RD)&&(mbb->y<pEnc->range_mb_y2_RD))
		{
			if (((mb->mvs[3].vec.s16x*mb->mvs[3].vec.s16x+mb->mvs[3].vec.s16x*mb->mvs[3].vec.s16x)>pEnc->MV_th2)&&(sad16>pEnc->sad_th2))
				pEnc->current1->activity2 ++;
			else if (abs(pEnc->dev_buffer[mbb->mbpos-1]-dev)>pEnc->delta_dev_th2)
				pEnc->current1->activity2 ++;
		}
	}
    pEnc->dev_buffer[mbb->mbpos-1] = dev;
}

void 
do_motion_dection_4mv(MACROBLOCK_E *mb,MACROBLOCK_E_b *mbb,Encoder *pEnc)
{
/* motion dection */
	if (pEnc->dev_flag) {
		if (pEnc->frame_counter >= pEnc->md_interval) {
			if (pMB->mode == MODE_INTER4V) {
				tmp1 = (pMB->mvs[0].vec.s16x*pMB->mvs[0].vec.s16x + pMB->mvs[0].vec.s16y*pMB->mvs[0].vec.s16y) +
	    	    		(pMB->mvs[1].vec.s16x*pMB->mvs[1].vec.s16x + pMB->mvs[1].vec.s16y*pMB->mvs[1].vec.s16y) +
	            		(pMB->mvs[2].vec.s16x*pMB->mvs[2].vec.s16x + pMB->mvs[2].vec.s16y*pMB->mvs[2].vec.s16y) +
	            		(pMB->mvs[3].vec.s16x*pMB->mvs[3].vec.s16x + pMB->mvs[3].vec.s16y*pMB->mvs[3].vec.s16y);
			} else if (pMB->mode == MODE_INTER) {
				tmp1 = (pMB->mvs[3].vec.s16x*pMB->mvs[3].vec.s16x + pMB->mvs[3].vec.s16y*pMB->mvs[3].vec.s16y);
			}
			if ( (x >= pEnc->range_mb_x0_LU) && (y >= pEnc->range_mb_y0_LU) &&
		     	(x <  pEnc->range_mb_x0_RD) && (y <  pEnc->range_mb_y0_RD) )
			{
				if (( tmp1 > pEnc->MV_th0) && (pMB->sad16 > pEnc->sad_th0))
					pEnc->current1->activity0 ++;
				else if (abs(pEnc->dev_buffer[counter-1]-pMB->dev) > pEnc->delta_dev_th0)
					pEnc->current1->activity0 ++;
			}
			if ( (x >= pEnc->range_mb_x1_LU) && (y >= pEnc->range_mb_y1_LU) &&
	     		(x <  pEnc->range_mb_x1_RD) && (y <  pEnc->range_mb_y1_RD) )
			{
				if (( tmp1 > pEnc->MV_th1) && (pMB->sad16 > pEnc->sad_th1))
					pEnc->current1->activity1 ++;
				else if (abs(pEnc->dev_buffer[counter-1]-pMB->dev) > pEnc->delta_dev_th1)
					pEnc->current1->activity1 ++;
			}
			if ( (x >= pEnc->range_mb_x2_LU) && (y >= pEnc->range_mb_y2_LU) &&
	     		(x <  pEnc->range_mb_x2_RD) && (y <  pEnc->range_mb_y2_RD) )
			{
				if (( tmp1 > pEnc->MV_th2) && (pMB->sad16 > pEnc->sad_th2))
					pEnc->current1->activity2 ++;
				else if (abs(pEnc->dev_buffer[counter-1]-pMB->dev) > pEnc->delta_dev_th2)
					pEnc->current1->activity2 ++;
			}
			pEnc->dev_buffer[counter-1] = pMB->dev;
		}
	} else
		pEnc->dev_buffer[counter-1] = pMB->dev;

}
#endif

void 
encoder_pframe(Encoder_x * pEnc_x) //static void FrameCodeP(Encoder_x * pEnc_x, bool vol_header)
{
	Encoder *pEnc = &(pEnc_x->Enc);
	bool vol_header = pEnc_x->vol_header & 0xFF; 
	volatile MDMA * ptDma = (volatile MDMA *)(Mp4EncBase(pEnc) + DMA_OFF); //volatile MDMA * ptDma = (volatile MDMA *)(pEnc->u32VirtualBaseAddr + DMA_OFF);
	volatile MP4_t * ptMP4 = pEnc->ptMP4;
	volatile uint32_t * me_cmd = (volatile uint32_t *) (Mp4EncBase(pEnc) + ME_CMD_Q_OFF); //volatile uint32_t * me_cmd = (volatile uint32_t *) (pEnc->u32VirtualBaseAddr + ME_CMD_Q_OFF);
	uint32_t * dma_cmd_tgl0 = (uint32_t *)(Mp4EncBase(pEnc) + DMA_ENC_CMD_OFF_0); //uint32_t * dma_cmd_tgl0 = (uint32_t *)(pEnc->u32VirtualBaseAddr + DMA_CMD_OFF_0);
	uint32_t * dma_cmd_tgl1 = (uint32_t *)(Mp4EncBase(pEnc) + DMA_ENC_CMD_OFF_1); //uint32_t * dma_cmd_tgl1 = (uint32_t *)(pEnc->u32VirtualBaseAddr + DMA_CMD_OFF_1);
	uint32_t * dma_cmd_tgl;
	int32_t i;

	uint32_t u32cmd_mc=0;
	uint32_t u32cmd_mc_reload;
         #ifdef FIX_RESYNC_BUG
         uint32_t u32cmd_me=0;
         #endif // #ifdef FIX_RESYNC_BUG
	uint32_t u32grpc;
	uint32_t u32pipe = 0;
	
	#ifdef TWO_P_INT
	MACROBLOCK_E mbs_local1[ENC_MBS_LOCAL_P + 1];
	MACROBLOCK_E * mbs_local;
	MACROBLOCK_E mbs_ACDC1[ENC_MBS_ACDC + 1];
	MACROBLOCK_E * mbs_ACDC;
	MACROBLOCK_E mbs_REF1[ENC_MBS_LOCAL_REF_P + 1];
	MACROBLOCK_E * mbs_REF;
	#endif
	
	MACROBLOCK_E *mb_ref;
	MACROBLOCK_E *mb_img;
	MACROBLOCK_E *mb_me;
	MACROBLOCK_E *mb_mc;
	MACROBLOCK_E *mb_rec;
	MACROBLOCK_E_b mbb[ENC_P_FRAME_PIPE];
	MACROBLOCK_E_b *mbb_ref = &mbb[ENC_P_FRAME_PIPE - 1];
	MACROBLOCK_E_b *mbb_img=0;
	MACROBLOCK_E_b *mbb_me=0;
	MACROBLOCK_E_b *mbb_mc=0;
	MACROBLOCK_E_b *mbb_rec=0;
	
	#ifdef TWO_P_INT
	MBS_NEIGHBOR_INFO mbs_neighbor_info;
	// align to 8 bytes
	mbs_neighbor_info.mbs_cur_row = 
	mbs_local = (MACROBLOCK_E *)(((uint32_t)&mbs_local1[0] + 7) & ~0x7);
	  
	mbs_neighbor_info.mbs_upper_row = 
	mbs_ACDC = (MACROBLOCK_E *)(((uint32_t)&mbs_ACDC1[0] + 7) & ~0x7);
	  
	mbs_neighbor_info.mbs_prev_ref =
	mbs_REF = (MACROBLOCK_E *)(((uint32_t)&mbs_REF1[0] + 7) & ~0x7);
	#endif

	u32cmd_mc_reload =
		#ifndef GM8120
      		MCCTL_ENC_MODE | 
    	#endif
		(pEnc->bH263 << 14) |
		(pEnc->bMp4_quant << 3);
	if ( pEnc->ac_disable ==1)
		u32cmd_mc_reload |= MCCTL_DIS_ACP;
	
	p_dma_init (pEnc);
	
	// the last command in the chain never skipped.
	u32grpc =
		(0 << (ID_CHN_REF_REC*2)) |		// REF_REC dma: enable
		(2 << (ID_CHN_IMG*2)) |			// 4mv dma: disable
		(2 << (ID_CHN_CUR_REC_Y*2)) |		// CUR_REC_Y dma: disable
		(2 << (ID_CHN_CUR_REC_UV*2))  |	// CUR_REC_UV dma: disable
		#ifdef TWO_P_INT
	         (2 << (ID_CHN_STORE_MBS * 2))	 |  // disable MBS dma
		(2 << (ID_CHN_ACDC_MBS  * 2))	 |  // disable acdc MBS dma
		(2 << (ID_CHN_LOAD_REF_MBS  * 2))	 |  // disable ref MBS dma
                  #endif
		(3 << (ID_CHN_ACDC*2));			// St_Ld_ACDC dma: sync to MC_done
		
	#ifdef TWO_P_INT
	mb_ref =
	mb_img =
	mb_me =
	mb_mc =
	mb_rec = &mbs_local[0];
	#else
	mb_ref =
	mb_img =
	mb_me =
	mb_mc =
	mb_rec = &pEnc->current1->mbs[0];
	#endif
	// let me_cmd[18] get motion vector 0 from left MB at first time
	mb_mc->mvs[1].u32num = 0;
	mb_mc->mvs[3].u32num = 0;

         /* motion dection */
	pEnc->current1->activity0 = 0;
	pEnc->current1->activity1 = 0;
	pEnc->current1->activity2 = 0;
	
	mbb_ref->mbpos = -1;
	mbb_ref->x = -1;
	mbb_ref->y = 0;
	mbb_ref->toggle = -1;
	i = -1;

	ptDma->GRPS =
	        (2 << (ID_CHN_CUR_REC_Y*2))	 |	// ID_CHN_CUR_REC_Y dma: sync to MC_done
	        (2 << (ID_CHN_CUR_REC_UV*2))  |	// CUR_REC_UV dma: sync to MC_done
	        #ifdef TWO_P_INT
	        (2 << (ID_CHN_STORE_MBS * 2))	 |  // MBS dma : sync to MC done		      
                 (1 << (ID_CHN_ACDC_MBS  * 2))	 |  // acdc MBS dma : sync to ME done for delay purpose       //MC done
                 (1 << (ID_CHN_LOAD_REF_MBS  * 2))	 |  // ref MBS dma : sync to ME done for delay purpose    //MC done
	        #endif
	        (2 << (ID_CHN_ACDC*2));			// St_Ld_ACDC dma: sync to MC_done
	ptMP4->MECTL = MECTL_VPKTSTART | MECTL_VOPSTART;
	// to set the VOP size (including VOP width and VOP height)
	ptMP4->VOPDM = (pEnc->mb_width << 20) | (pEnc->mb_height << 4);
	// the reason why we set the MCCADDR register just here for once is because during
	// P-frame encoding , unlike I-frame encoding , while ME engine is activated, the
	// ME engine will copy the current blocks to the address that was set by MCCADDR
	// register by the way.
	ptMP4->MCCADDR = CUR_YUV;
	me_cmd_each_init (pEnc);

	// to set the ME Coefficient Register
	ptMP4->MECR = NEIGH_TEND_16X16*lambda_vec16[pEnc->current1->quant];

    pEnc->intraMBNum = 0;    // Tuba 20111209_0: output intra mb number in P frame

	while (1) {
		//(13)goMC_mc(4S)
		if (u32pipe & BIT_4ST) {
			#ifdef GM8120
				// for non-GM8180 platform, we don't have the 'MECP_done' signal to make sure the 'ME Copy'
				// is done or not. So we use the dummy DMA to address such potential problem on the last
				// MB before invoking the 'MC go' command.
				// if it is the last MB for MC
				if(mbb_mc->x == (pEnc->mb_width-1) && mbb_mc->y == (pEnc->mb_height-1) ) { // if it is the last MB for MC		      
					// to do dummy DMA
					ptDma->Control =
						mDmaIntChainMask1b(FALSE) |
						mDmaEn1b(TRUE) |
						mDmaChainEn1b(FALSE) |
						mDmaDir1b(DONT_CARE) |
						mDmaSType2b(DONT_CARE) |
						mDmaLType2b(DONT_CARE) |
						mDmaLen12b(0) |
						mDmaID4b(0);		      
					// wait for  DMA_done
					mPOLL_MARKER_S(ptMP4);
					while((ptDma->Status & 0x1) == 0)
						;
					mPOLL_MARKER_E(ptMP4);
				}
			#endif
			ptMP4->MCCTL = u32cmd_mc;
		}
		//(12')goME(3S)
		if (u32pipe & BIT_3ST) {
			#ifdef FIX_RESYNC_BUG
				ptMP4->MECTL = mMECTL_RND1b(pEnc->current1->bRounding_type) | u32cmd_me;
			#else
				ptMP4->MECTL = mMECTL_RND1b(pEnc->current1->bRounding_type) | MECTL_MEGO;
			#endif // #ifdef FIX_RESYNC_BUG
		}

		//(1)goDMA_Ref-----(6)goDMA_LD---------------------(14)goDMA_ST---(19)goDMA_recon(5S-x)
		//(7)goDMA_img
		if (mbb_ref->toggle == -1) {		// first time
			pEnc->current1->coding_type = P_VOP;
			if (pEnc->bH263) {
				pEnc->bRounding_type = pEnc->current1->bRounding_type = 0;
				BitstreamWriteShortHeader(pEnc, pEnc->current1, 1);
			}
			else {
				pEnc->bRounding_type = 1 - pEnc->bRounding_type;
				pEnc->current1->bRounding_type = pEnc->bRounding_type;
				if (vol_header)
					BitstreamWriteVolHeader(pEnc, pEnc->current1);
				BitstreamWriteVopHeader(pEnc, pEnc->current1, 1);
			}
		}
		else {  // not first time
			u32pipe &= ~BIT_5ST;
			if (mbb_ref->toggle == 0)
				ptDma->CCA = (DMA_ENC_CMD_OFF_0 + CHN_REF_RE_Y * 4) | DMA_LC_LOC;
			else
				ptDma->CCA = (DMA_ENC_CMD_OFF_1 + CHN_REF_RE_Y * 4) | DMA_LC_LOC;
			ptDma->GRPC = u32grpc;
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
		}
		//~~change to next mb~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	    ///////////////////////////////////////////////////////////////////////////
	    // update mb pointer
	    i ++;
	    mbb_rec = mbb_mc;
	    mbb_mc = mbb_me;
	    mbb_me = mbb_img;
	    mbb_img = mbb_ref;
	    mbb_ref = &mbb[i%ENC_P_FRAME_PIPE];
	    mbb_ref->toggle = i & BIT0;
	    mbb_ref->mbpos = mbb_img->mbpos + 1;
	    mbb_ref->x = mbb_img->x + 1;
	    mbb_ref->y = mbb_img->y;
		
	    #ifdef GM8180_OPTIMIZATION
	    mbb_ref->mecmd_pipe = 0;
	    #endif
		
	    if (mbb_ref->x == pEnc->mb_width) {
			mbb_ref->x = 0;
			mbb_ref->y ++;
	    }
	    if (mbb_ref->toggle == 0)
		dma_cmd_tgl = dma_cmd_tgl0;
	    else
		dma_cmd_tgl = dma_cmd_tgl1;
	    mb_rec = mb_mc;
	    mb_mc = mb_me;
	    mb_me = mb_img;
	    mb_img = mb_ref;
	    mVpe_Indicator(0x90000000 | (mbb_ref->y << 8 ) | (mbb_ref->x));
             //	mVpe_Indicator(0x90000000 | (mbb_mc->toggle << 16 ) | (mbb_ref->y << 8 ) | (mbb_ref->x));

	    u32grpc =
		(2 << (ID_CHN_REF_REC*2)) |		// REF_REC dma: disable
		(2 << (ID_CHN_IMG*2)) |			// 4mv dma: disable
		(2 << (ID_CHN_CUR_REC_Y*2)) |		// CUR_REC_Y dma: disable
		(2 << (ID_CHN_CUR_REC_UV*2))	 |	// CUR_REC_UV dma: disable
		#ifdef TWO_P_INT
	         (2 << (ID_CHN_STORE_MBS * 2))	 |  // disable MBS dma
		(2 << (ID_CHN_ACDC_MBS  * 2))	 |  // disable acdc MBS dma
		(2 << (ID_CHN_LOAD_REF_MBS  * 2))	 |  // disable ref MBS dma
		#endif
		(3 << (ID_CHN_ACDC*2));			// St_Ld_ACDC dma: sync to MC_done

                  //(2)preMoveImg(0S-1S)
		if (u32pipe & BIT_0ST) {
			u32pipe &= ~BIT_0ST;
			u32pipe |= BIT_1ST;
			u32grpc &= ~(uint32_t)(3 << (ID_CHN_IMG * 2));		// exec IMG dma

			if (pEnc->img_fmt <= 1) {	// H264_2D/MP4_2D
				if (mbb_img->x == 0 || mbb_img->x == 1) {
					IMAGE * img = &pEnc->current1->image;
					unsigned int mb_pos = mbb_img->y * pEnc->mb_stride+ mbb_img->x + pEnc->mb_offset;
					// y
					dma_cmd_tgl[CHN_IMG_Y] =
							mDmaSysMemAddr29b(img->y_phy + mb_pos * SIZE_Y) |
							mDmaSysInc3b (DMA_INCS_512);		// 2 * SIZE_Y
					if (pEnc->img_fmt == 1) { // MP4_2D
						// u
						dma_cmd_tgl[CHN_IMG_U] =
								mDmaSysMemAddr29b(img->u_phy + mb_pos * SIZE_U) |
								mDmaSysInc3b (DMA_INCS_128);		// 2 * SIZE_U
						// v
						dma_cmd_tgl[CHN_IMG_V] =
								mDmaSysMemAddr29b(img->v_phy + mb_pos * SIZE_V) |
								mDmaSysInc3b (DMA_INCS_128);		// 2 * SIZE_V
					}
					else {									// H264_2D
							// u
							dma_cmd_tgl[CHN_IMG_U] =
									mDmaSysMemAddr29b(img->u_phy + mb_pos * SIZE_U*2) |
									mDmaSysInc3b (DMA_INCS_256);		// 4 * SIZE_U
 	        		}
			    }
			} 
		}
                  //(0)preMoveRef(x-0S)
		if (mbb_ref->y < pEnc->mb_height) {
			u32pipe |= BIT_0ST;
			u32grpc &= ~(uint32_t)(3 << (ID_CHN_REF_REC * 2));		// exec Ref dma
			// init mb_ref
			#ifdef TWO_P_INT
			mb_ref = &mbs_local[(mbb_ref->mbpos)%ENC_MBS_LOCAL_P];
			#else
			mb_ref = &pEnc->current1->mbs[mbb_ref->mbpos];
			#endif
			// init quant from frame-quant
			mb_ref->quant = pEnc->current1->quant;
			dma_cmd_tgl[CHN_REF_RE_Y + 1] =
				mDmaLocMemAddr14b((uint32_t) REF_Y + (mbb_ref->mbpos & 0x3) * PIXEL_Y);
			dma_cmd_tgl[CHN_REF_RE_U + 1] =
				mDmaLocMemAddr14b((uint32_t) REF_U + (mbb_ref->mbpos & 0x3) * PIXEL_U);
			dma_cmd_tgl[CHN_REF_RE_V + 1] =
				mDmaLocMemAddr14b((uint32_t) REF_V + (mbb_ref->mbpos & 0x3) * PIXEL_V);

			#ifdef TWO_P_INT
			u32grpc &= ~(uint32_t)(3 << (ID_CHN_LOAD_REF_MBS * 2));		// exec LOAD REF MBS dma
			u32grpc |= (3 << (ID_CHN_LOAD_REF_MBS * 2)); // and make it sync to specified condition
			// assign start address of local memory
			dma_cmd_tgl[CHN_LOAD_REF_MBS + 1] =
			     mDmaLocMemAddr14b(&mbs_REF[i % ENC_MBS_LOCAL_REF_P]);
			#endif
	         }
                  //(15)preMoveRecon(4S)
                  //(16)wait VLC(4S)
		if (u32pipe & BIT_4ST) {
			uint32_t offset;
			switch (i % 3) {
				case 0:
					offset = INTER_Y2;	break;
				case 1:
					offset = INTER_Y0;	break;
				default:
					offset = INTER_Y1;	break;
			}
			dma_cmd_tgl[CHN_CUR_RE_Y + 1] = mDmaLocMemAddr14b(offset + 0);
			dma_cmd_tgl[CHN_CUR_RE_U + 1] = mDmaLocMemAddr14b(offset + (INTER_U0 - INTER_Y0));
			dma_cmd_tgl[CHN_CUR_RE_V + 1] = mDmaLocMemAddr14b(offset + (INTER_V0 - INTER_Y0));
 			u32grpc &= ~(uint32_t)
						((3 << (ID_CHN_CUR_REC_Y * 2)) |	// exec CUR_REC_Y dma
						(3 << (ID_CHN_CUR_REC_UV * 2)));	// exec CUR_REC_UV dma
			//#if 1			// fix me_done bug by software
			#ifndef GM8180_OPTIMIZATION
			  //u32grpc |= (3 << (ID_CHN_CUR_REC_UV * 2));  // enable it to sync to sync MC_done
			  u32grpc |= ( (3 << (ID_CHN_CUR_REC_Y  * 2)) |    // exec CUR_REC_Y dma to sync MC_done
			               (3 << (ID_CHN_CUR_REC_UV * 2)) );   // exec CUR_REC_UV dma to sync MC_done
			#endif
			
			mPOLL_VLC_DONE_MARKER_START(ptMP4)
			// wait for VLC_done
			while ((ptMP4->CPSTS & BIT15) == 0)
				;
			mPOLL_VLC_DONE_MARKER_END(ptMP4)
		}
                  //(9)preLoadACDC---(12-1)preStoreACDC(4S)
		if (u32pipe & BIT_4ST) {
			if (mbb_mc->x == 0) {
				dma_cmd_tgl[CHN_STORE_PREDITOR] =
					mDmaSysMemAddr29b((uint32_t)pEnc->pred_value_phy) |
					mDmaSysInc3b(DMA_INCS_128);
				dma_cmd_tgl[CHN_LOAD_PREDITOR] =
					mDmaSysMemAddr29b((uint32_t)pEnc->pred_value_phy + 64) |
					mDmaSysInc3b(DMA_INCS_128);
			}
			else if (mbb_mc->x == 1) {
				dma_cmd_tgl[CHN_STORE_PREDITOR] =
					mDmaSysMemAddr29b((uint32_t)pEnc->pred_value_phy + 64) |
					mDmaSysInc3b(DMA_INCS_128);
			}
			if (mbb_mc->x == (pEnc->mb_width - 1)) {
				dma_cmd_tgl[CHN_LOAD_PREDITOR] =
					mDmaSysMemAddr29b((uint32_t)pEnc->pred_value_phy) |
					mDmaSysInc3b(DMA_INCS_128);
			}
		}
                  //(17)wait MC(4S-5S)
		if (u32pipe & BIT_4ST) {
		    #ifdef FIE8160_OPTIMIZATION
		    uint32_t u32temp;
		    #endif
		    
		    u32pipe &= ~BIT_4ST;
		    u32pipe |= BIT_5ST;
			
		    mPOLL_MC_DONE_MARKER_START(ptMP4);

		    // When MC engine starts, it will also invoke AC_DC engine and then VLC engine.
		    // Therefore, in some occasions (especially when quantization value is equal to 1),
		    // MC_done did not mean the VLC engine is also done. 
		    // VLC_done :  ------------------                     ---------------
		    //                              |_____________________|
		    // AC_done  :  -----            -------------------------------------
		    //                 |____________|
		    // MC_done  :  -----                              -------------
		    //                 |______________________________|           |______
		    //
		    // From the illustration shown above, you can see it is possible the previous 
		    // (wait VLC(4S)) might miss the vlc_done check since VLC engine has not even
		    // started yet.
		    // So we should check the MC done with VLC done together in order to avoid errors.
		    #ifdef GM8180_OPTIMIZATION
		    // wait for MC done and VLC_done			
		    while (((u32temp=ptMP4->CPSTS) & (BIT1|BIT15)) != (BIT1|BIT15))    {
		        #define ME_CMD_FIELD (BIT20|BIT21|BIT22)
		        #define BK_NUM_FIELD (BIT18|BIT19)
		        register unsigned int me_cmd_type   = ((u32temp& ME_CMD_FIELD)>>20);
		        register unsigned int bk_num_number = ((u32temp& BK_NUM_FIELD)>>18);
		        register unsigned char fire_me_cmd = 0;
		         //if((!(pEnc->bEnable_4mv)) && (me_cmd_type >= MEC_REF) && (bk_num_number ==0) && (!(mbb_me->mecmd_pipe & BIT0)) )
		         //if(pEnc->bEnable_4mv) {
		         //if(bk_num_number == 1) {
		         //if((bk_num_number == 1)&&(me_cmd_type <= MEC_REF)) {
		         //mbb_me->mecmd_pipe |= BIT0; fire_me_cmd = 1;			        
		         //} else if (bk_num_number == 2) {
		         //} else if ((bk_num_number == 2)&&(me_cmd_type <= MEC_REF)) {
		         //mbb_me->mecmd_pipe |= (BIT1|BIT0); fire_me_cmd = 1;			        
		         //} else if (bk_num_number == 3) {
		         //} else if ((bk_num_number == 3)&&(me_cmd_type <= MEC_REF)) {
		         //mbb_me->mecmd_pipe |= (BIT2|BIT1|BIT0); fire_me_cmd = 1;			      
		         //} else if (me_cmd_type >= MEC_MOD) {
		         //mbb_me->mecmd_pipe |= (BIT3|BIT2|BIT1|BIT0); fire_me_cmd = 1;
		         //}
                           //} else {
			//if((me_cmd_type >= MEC_DIA) && (bk_num_number ==0))
			if(me_cmd_type > MEC_DIA)  {
			    //if(mbb_mc->x < (pEnc->mb_width - 1) && mbb_mc->y < (pEnc->mb_height - 1)) {			          
			    //printf("i=%d, u32ttemp = 0x%x , me_cmd_type = 0x%x\n",i,u32temp,me_cmd_type);
			    mbb_me->mecmd_pipe |= BIT0;
			    fire_me_cmd = 1;
			}
			//}
			if(fire_me_cmd) {
			    mPOLL_MECMD_SET_MARKER_START_0(ptMP4)
			    #ifdef TWO_P_INT			
			    me_cmd_set(mbb_me, mb_mc, pEnc, &mbs_neighbor_info);
			    #else
			    me_cmd_set(mbb_me, mb_mc, pEnc);
			    #endif
			    mPOLL_MECMD_SET_MARKER_END_0(ptMP4) 
			}
	              }
		     #else
		     // wait for MC done and VLC_done			
		     while ((ptMP4->CPSTS & (BIT1|BIT15)) != (BIT1|BIT15))
			  ;
		     #endif
		     mPOLL_MC_DONE_MARKER_END(ptMP4);
		}
		
                  //(12-2)waitME_done(3S)
                  //(12-3)preMC(3S-4S)
		u32cmd_mc = u32cmd_mc_reload;
		if (u32pipe & BIT_3ST) {
		    int32_t s32temp;
		    #ifdef GM8180_OPTIMIZATION
		    uint32_t u32temp;
		    #endif
		    
		    u32pipe &= ~BIT_3ST;
		    u32pipe |= BIT_4ST;
		    // MC will get interplation mb from MCIADDR
		    // MC will put reconstruct frame into MCIADDR 
		    switch (i % 3) {
			case 0:
				ptMP4->MCIADDR = INTER_Y0;	break;
			case 1:
				ptMP4->MCIADDR = INTER_Y1;	break;
			default:
				ptMP4->MCIADDR = INTER_Y2;	break;
		    }

		    ptMP4->QCR0 = mb_mc->quant << 18;
		    if ((pEnc->bResyncMarker) && (mbb_mc->y != 0) && (mbb_mc->x == 0)) {
			if (pEnc->bH263) {
				BitstreamPutBits(ptMP4, VIDO_RESYN_MARKER, 17);
				BitstreamPutBits(ptMP4, mbb_mc->y, 5);
				BitstreamPutBits(ptMP4, 0, 2);		// ID
				BitstreamPutBits(ptMP4, mb_mc->quant, 5);
			}
			else	{
				BitstreamPadAlways(ptMP4);
				BitstreamPutBits(ptMP4, VIDO_RESYN_MARKER, 17);
				BitstreamPutBits(ptMP4, mbb_mc->mbpos, log2bin(pEnc->mb_width *  pEnc->mb_height - 1));
				BitstreamPutBits(ptMP4, mb_mc->quant, 5);
				BitstreamPutBit(ptMP4, 0);
			}
                     }
			
		   #ifdef TWO_P_INT
		   u32grpc &= ~(uint32_t)(3 << (ID_CHN_STORE_MBS * 2));		// exec STORE MBS dma
		   u32grpc |= (3 << (ID_CHN_STORE_MBS * 2)); // and make it sync to specified condition
		   // assign start address of local memory
		  dma_cmd_tgl[CHN_STORE_MBS + 1] =
			mDmaLocMemAddr14b(&mbs_local[mbb_mc->mbpos % ENC_MBS_LOCAL_P]);
		  #endif
			
		  mPOLL_ME_DONE_MARKER_START(ptMP4);
			
		  #ifdef GM8180_OPTIMIZATION
		  //waitME_done
		  while (((s32temp = (int32_t)ptMP4->CPSTS) & BIT0) == 0) {
		      #define ME_CMD_FIELD (BIT20|BIT21|BIT22)
	               #define BK_NUM_FIELD (BIT18|BIT19)
		      unsigned char fire_me_cmd = 0;
		      u32temp = (uint32_t) s32temp;
	               //printf("pEnc->bEnable_4mv = %d, u32ttemp = 0x%x , me_cmd_type = 0x%x, bk_num_number=%d\n",pEnc->bEnable_4mv,u32temp,me_cmd_type,bk_num_number);
		      if(pEnc->bEnable_4mv) {
		          unsigned int me_cmd_type   = ((u32temp& ME_CMD_FIELD)>>20);
		          unsigned int bk_num_number = ((u32temp& BK_NUM_FIELD)>>18);
			 //if((me_cmd_type == MEC_PMVX) && (bk_num_number == 1) && (!(mbb_me->mecmd_pipe & BIT0))) {
			 //if(bk_num_number == 1) {
			 //if((bk_num_number == 1)&&(me_cmd_type <= MEC_REF)) {
			 //mbb_me->mecmd_pipe |= BIT0; fire_me_cmd = 1;
			 //} else if ((me_cmd_type == MEC_PMVX) && (bk_num_number == 2) && (!(mbb_me->mecmd_pipe & BIT1))) {
			 //} else if (bk_num_number == 2) {
			 //} else if ((bk_num_number == 2)&&(me_cmd_type <= MEC_REF)) {
			 //mbb_me->mecmd_pipe |= (BIT1|BIT0); fire_me_cmd = 1;
			 //} else if ((me_cmd_type == MEC_PMVX) && (bk_num_number == 3) && (!(mbb_me->mecmd_pipe & BIT2))) {
			 //} else if (bk_num_number == 3) {
			 //} else if ((bk_num_number == 3)&&(me_cmd_type <= MEC_REF)) {
			 //mbb_me->mecmd_pipe |= (BIT2|BIT1|BIT0); fire_me_cmd = 1;
			 //} else if ((me_cmd_type == MEC_MOD) && (!(mbb_me->mecmd_pipe & BIT3)) ) {
			 //} else if (me_cmd_type >= MEC_MOD) {
			 //mbb_me->mecmd_pipe |= (BIT3|BIT2|BIT1|BIT0); fire_me_cmd = 1;
			 //}
			 if(fire_me_cmd) {
			     //printf("pEnc->bEnable_4mv = %d, u32temp = 0x%x , me_cmd_type = 0x%x, bk_num_number=%d\n",pEnc->bEnable_4mv,u32temp,me_cmd_type,bk_num_number);
			     mPOLL_MECMD_SET_MARKER_START_1(ptMP4)
			     #ifdef TWO_P_INT
			     me_cmd_set(mbb_me, mb_mc, pEnc, &mbs_neighbor_info);
			     #else
			     me_cmd_set(mbb_me, mb_mc, pEnc);
			     #endif
			     mPOLL_MECMD_SET_MARKER_END_1(ptMP4) 
			 }
                        }			    
                   }
		 #else
		 //waitME_done
		 while (((s32temp = (int32_t)ptMP4->CPSTS) & BIT0) == 0)
			;
		 #endif
		 mPOLL_ME_DONE_MARKER_END(ptMP4);
				
		 if (s32temp & BIT7) {
		 	// intra mode
			mb_mc->mode = MODE_INTRA;
			mb_mc->mvs[0].u32num =
			mb_mc->mvs[1].u32num =
			mb_mc->mvs[2].u32num =
			mb_mc->mvs[3].u32num = 0;
			#ifdef TWO_P_INT
			u32cmd_mc |= predict_acdc_P (mbs_local,mbs_ACDC,
							mbb_mc, pEnc->mb_width, pEnc->bResyncMarker);
			#else
			u32cmd_mc |= predict_acdc_P (pEnc->current1->mbs,
							mbb_mc, pEnc->mb_width, pEnc->bResyncMarker);
			#endif
            pEnc->intraMBNum++;    // Tuba 20111209_0: output intra mb number in P frame
		  } else {
			// inter mode
			if (s32temp & BIT6) {
				// 1mv mode
				mb_mc->mode = MODE_INTER;
				// store back the motion vector to MV0[1]
//				u32temp = me_cmd[1] = me_cmd[3];
				if (mbb_mc->toggle)
					s32temp = (int32_t)me_cmd[7];
				else
					s32temp = (int32_t)me_cmd[3];
				s32temp >>= 12;
				mb_mc->mvs[0].vec.s16x =
				mb_mc->mvs[1].vec.s16x =
				mb_mc->mvs[2].vec.s16x =
				mb_mc->mvs[3].vec.s16x = (s32temp<<18)>>25;
				mb_mc->mvs[0].vec.s16y =
				mb_mc->mvs[1].vec.s16y =
				mb_mc->mvs[2].vec.s16y =
				mb_mc->mvs[3].vec.s16y = (s32temp<<25)>>25;
				// Hmm.. it is used in bit 16 of MCCTL control register (at location 0x1001c) to indicate
				// whether current motion vector is zero or not...
				mbb_mc->mvz = (s32temp == 0) ? BIT16: 0;

				// restore sad16 and dev
				mb_mc->sad16 = ptMP4->MIN_SADMV;
				//printk("dev 0x%x sad 0x%x\n", mb_mc->dev, mb_mc->sad16);
					
				#ifdef MOTION_DETECTION_IN_DRV
                    		/*motion dection */
				if(pEnc->motion_dection_enable==1)
                    			do_motion_dection_1mv(mb_mc,mbb_mc,pEnc);
				#endif
								
			  }
			  else {
				// 4mv mode
				register uint32_t volatile * me_temp = &me_cmd[0];
				if (mbb_mc->toggle)
					me_temp += 4;
				mb_mc->mode = MODE_INTER4V;
				s32temp = (int32_t)me_temp[0]>>12;
				mb_mc->mvs[0].vec.s16x = (s32temp<<18)>>25;
				mb_mc->mvs[0].vec.s16y = (s32temp<<25)>>25;
				s32temp = (int32_t)me_temp[1]>>12;
				mb_mc->mvs[1].vec.s16x = (s32temp<<18)>>25;
				mb_mc->mvs[1].vec.s16y = (s32temp<<25)>>25;
				s32temp = (int32_t)me_temp[2]>>12;
				mb_mc->mvs[2].vec.s16x = (s32temp<<18)>>25;
				mb_mc->mvs[2].vec.s16y = (s32temp<<25)>>25;
				s32temp = (int32_t)me_temp[3]>>12;
				mb_mc->mvs[3].vec.s16x = (s32temp<<18)>>25;
				mb_mc->mvs[3].vec.s16y = (s32temp<<25)>>25;
				// It is used in bit 16 of MCCTL control register (at location 0x1001c) to indicate
				// whether current motion vector is zero or not...
				mbb_mc->mvz = 0;
					
				// restore sad16 and dev
				mb_mc->sad16 = ptMP4->MIN_SADMV;
									
				#ifdef MOTION_DETECTION_IN_DRV
                    		/*motion dection */
                    		if(pEnc->motion_dection_enable==1)
                    			do_motion_dection_4mv(mb_mc,mbb_mc,pEnc);
				#endif	
			  }
			  u32cmd_mc |= mbb_mc->mvz | mb_mc->mode << 6 | MCCTL_MCGO;
		     } 
			
			// The reason why we query the PXI done is that :
			// We all assume that the PXI command (one of ME command) is finished
			// when ME engine is done. But from the waveform, the truth is that
			// only when ME engine is done, can the PXI command begin to execute.
			// So if you change the HOFFSET after ME engine was done, it will severely
			// affect the behavior of PXI command and cause bitstream error.
			// Hence, we should query the PXI done status before setting the HOFFSET.
			// And the PXI_done bit was right in the bit 3 of CPSTS register (which is
			// called PMV_DONE in decoding mode) and this bit definition of encoding 
			// did not appear in the data sheet.
			//			
			mPOLL_PMV_DONE_MARKER_START(ptMP4)
			// wait for PXI command done (called PMV_DONE in decoding mode)
			if(i>2) { // do not do the query for the very first time
			  while ((ptMP4->CPSTS & BIT3) == 0)			  
				  ;
		    }
		    mPOLL_PMV_DONE_MARKER_END(ptMP4)
		}
		
		
                  //(10)preME(2S)
		if (u32pipe & BIT_2ST) {
		    #ifdef FIX_RESYNC_BUG
		    u32cmd_me = MECTL_MEGO;
		    if ((pEnc->bResyncMarker) && (mbb_me->x == 0))
		        u32cmd_me |= MECTL_VPKTSTART;
		    #endif // #ifdef FIX_RESYNC_BUG

		    // ME will put interplation mb into MEIADDR
		    switch (i % 3) {
			case 0:
				ptMP4->MEIADDR = INTER_Y1;	break;
			case 1:
				ptMP4->MEIADDR = INTER_Y2;    break;
			default:
				ptMP4->MEIADDR = INTER_Y0;    break;
		    }
			
		    // to set Horizontal Offset Register 
		    //ptMP4->HOFFSET = (mbb_me->mbpos & 0x3)*16;
			
		    // ME will fetch current MB from MECADDR
		    if (mbb_me->toggle)
			ptMP4->MECADDR = CUR_Y0;
		    else
			ptMP4->MECADDR = CUR_Y1;

                      #ifdef GM8180_OPTIMIZATION
                      //if( ((!(pEnc->bEnable_4mv)) && (!(mbb_me->mecmd_pipe&BIT16))) || (pEnc->bEnable_4mv && ((mbb_me->mecmd_pipe&(BIT16|BIT17|BIT18|BIT19))!=(BIT16|BIT17|BIT18|BIT19))) ) {
                      //if( (!(pEnc->bEnable_4mv)) && (!(mbb_me->mecmd_pipe&BIT16)) ) {
                      mbb_me->mecmd_pipe |= (BIT0|BIT1);
                      //if(pEnc->bEnable_4mv)
                      //mbb_me->mecmd_pipe |= (BIT1|BIT2|BIT3);
                      #endif
            
                      mPOLL_MECMD_SET_MARKER_START_2(ptMP4)
		    #ifdef TWO_P_INT			
		    me_cmd_set(mbb_me, mb_mc, pEnc, &mbs_neighbor_info);
		    #else
		    me_cmd_set(mbb_me, mb_mc, pEnc);
		    #endif
		    mPOLL_MECMD_SET_MARKER_END_2(ptMP4)
			
		    #ifdef GM8180_OPTIMIZATION
		    //}
		    #endif
			
			
		    #ifdef TWO_P_INT
		    // load upper row data for mbs_ACDC
		    if (mbb_me->mbpos >= (pEnc->mb_width-3)) {
		         uint32_t upper_pos =  (mbb_me->mbpos + 3 - pEnc->mb_width);
			u32grpc &= ~(3 << (ID_CHN_ACDC_MBS * 2));		// exec acdc mbs dma
			u32grpc |= (3 << (ID_CHN_ACDC_MBS * 2));       // and make it sync to specified condition
			dma_cmd_tgl[CHN_ACDC_MBS] =
				mDmaSysMemAddr29b(&pEnc->current1->mbs[upper_pos]);
			dma_cmd_tgl[CHN_ACDC_MBS + 1] =
				mDmaLocMemAddr14b(&mbs_ACDC[upper_pos%ENC_MBS_ACDC]);
		    }
		    #endif
		    /*
		    // The reason why we query the PXI done is that :
		    // We all assume that the PXI command (one of ME command) is finished
		    // when ME engine is done. But from the waveform, the truth is that
		    // only when ME engine is done, can the PXI command begin to execute.
		    // So if you change the HOFFSET after ME engine was done, it will severely
		    // affect the behavior of PXI command and cause bitstream error.
		    // Hence, we should query the PXI done status before setting the HOFFSET.
		    // And the PXI_done bit was right in the bit 3 of CPSTS register (which is
		    // called PMV_DONE in decoding mode) and this bit definition of encoding 
		    // did not appear in the data sheet.
		    //			
		    mPOLL_PMV_DONE_MARKER_START(ptMP4)
		    // wait for PXI command done (called PMV_DONE in decoding mode)
		    if(i>2) { // do not do the query for the very first time
		        while ((ptMP4->CPSTS & BIT3) == 0)			  
				  ;
		    }
		    mPOLL_PMV_DONE_MARKER_END(ptMP4)
		    */
				
		    // to set Horizontal Offset Register 
		    ptMP4->HOFFSET = (mbb_me->mbpos & 0x3)*16;
			
                  }
                  //(8)(2S-3S)
		if (u32pipe & BIT_2ST) {
			u32pipe &= ~BIT_2ST;
			u32pipe |= BIT_3ST;
		}
                  //(3)(1S-2S)
		if (u32pipe & BIT_1ST) {
			u32pipe &= ~BIT_1ST;
			u32pipe |= BIT_2ST;
		}
                  //(5)waitDMA_Ref----(11)waitDMA_LD-------------------(18)waitDMA_ST---(20)waitDMA_recon
                  //(12)waitDMA_img
                  // wait for  DMA_done

		mPOLL_MARKER_S(ptMP4);
		while((ptDma->Status & 0x1) == 0)
			;
		mPOLL_MARKER_E(ptMP4);
		
		if(i==2) { //if (mbb_mc->x == -1) {	  
                      dma_cmd_tgl0[CHN_STORE_PREDITOR] =
                      dma_cmd_tgl0[CHN_LOAD_PREDITOR] =
                          mDmaSysMemAddr29b(pEnc->pred_value_phy)
                          | mDmaSysInc3b(DMA_INCS_128);
                  
                      dma_cmd_tgl1[CHN_STORE_PREDITOR] =
                          mDmaSysMemAddr29b(pEnc->pred_value_phy)
                          | mDmaSysInc3b(DMA_INCS_128);
                      dma_cmd_tgl1[CHN_LOAD_PREDITOR] =
                          mDmaSysMemAddr29b(pEnc->pred_value_phy+64)
                          | mDmaSysInc3b(DMA_INCS_128);		        
                  }

		#ifndef GM8120
		// check MECP_done
		mPOLL_MECP_DONE_MARKER_START(ptMP4);
		while ((ptMP4->CPSTS & BIT17) == 0)
		   ;
		mPOLL_MECP_DONE_MARKER_END(ptMP4);
		#endif
		if (u32pipe == 0)
			break;
	}
				
	BitstreamPadAlways(pEnc->ptMP4);

	pEnc->frame_counter++;
	if (pEnc->frame_counter > pEnc->md_interval)
		pEnc->frame_counter = 0;
	pEnc->dev_flag = 1;
}


