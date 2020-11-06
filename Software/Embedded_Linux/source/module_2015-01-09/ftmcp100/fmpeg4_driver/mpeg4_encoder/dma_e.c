#define DMA_E_GLOBALS
#ifdef LINUX
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/version.h>

#include "ftmcp100.h"
#include "encoder.h"
#include "local_mem_e.h"
#include "dma_e.h"
#include "../common/dma_m.h"
#include "../common/portab.h"
#include "../common/define.h"
#else
#include "portab.h"
#include "ftmcp100.h"
#include "dma_m.h"
#include "encoder.h"
#include "local_mem_e.h"
#include "dma_e.h"
#include "define.h"
#endif
#ifdef DMAWRP
#include "mp4_wrp_register.h"
#endif

// pipeline arrangement
/*
************************************************** syntax start
*********** dma **********
xx0d:	using dma pp 0
xx1d:	using dma pp 1
LD:		Load ACDC predictor
ST:		Store ACDC predictor
CRec:	move current reconstructed image
RRec:	move reference reconstructed image
img:		move image

*********** goMCCTL **********
M:	goMC
	0:	mc_interpolation input(& result) direct to INTER_Y0
		mc_current input direct to CUR_Y0
	1:	mc_interpolation input(& result) direct to INTER_Y1
		mc_current input direct to CUR_Y1
R:	remap
	0:	use predictor0
	1:	use predictor4
************************************************** syntax end

// Relationship:
// 1. LD0d -> R0
// 2. img0d -> M0 -> CRec0d

i-frame
mc_		|			|				| goMC		|storeCRec
toggle	|			| load			| store		|
		|			| img			|			|
===============================================================
-1		|
0		|			| LD0d(img0d)		|
1		|			| LD1d(img1d)		| R0(M0)		|
0		|			| LD0d(img0d)		| R1(M1)		| CRec0d			... a
1		|			| LD1d(img1d)		| R0(M0)		| CRec1d			... b
===============================================================

p-frame
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
==>
compare a & c
0		|			| LD0d(img0d)		| R1(M1)		| CRec0d			... a
0		|RRec0d		| LD0d(img0d)		| R1(M1)		| CRec0d			... c

compare b & d
1		|			| LD1d(img1d)		| R0(M0)		| CRec1d			... b
1		|RRec1d		| LD1d(img1d)		| R0(M0)		| CRec1d			... d
*/

uint32_t u32dma_e_const0[] = {
	///////////////////////////// DMA_TOGGLE 0 //////////////////////////
	// 1. CHN_REF_RE_Y, U, V
	// 2. CHN_IMG_Y, U, V
	// 3. CHN_CUR_RE_Y, U, V
	// 4. CHN_LD
	// 5. CHN_ST

	///////////////////////////////////////////////
	// CHN_REF_RE_Y + 0
		DONT_CARE,
	// CHN_REF_RE_Y + 1
		DONT_CARE,
	// CHN_REF_RE_Y + 2
		DONT_CARE,
	// CHN_REF_RE_Y + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(3 * SIZE_Y / 4) |
		mDmaID4b(ID_CHN_REF_REC),
	// CHN_REF_RE_U + 0
		DONT_CARE,
	// CHN_REF_RE_U + 1
		DONT_CARE,
	// CHN_REF_RE_U + 2
		DONT_CARE,
	// CHN_REF_RE_U + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(3 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_REF_REC),
	// CHN_REF_RE_V + 0
		DONT_CARE,
	// CHN_REF_RE_V + 1
		DONT_CARE,
	// CHN_REF_RE_V + 2
		DONT_CARE,
	// CHN_REF_RE_V + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(3 * SIZE_V / 4) |
		mDmaID4b(ID_CHN_REF_REC),
	///////////////////////////////////////////////
	// CHN_IMG_Y + 0
		DONT_CARE,
	// CHN_IMG_Y + 1
		mDmaLocMemAddr14b(CUR_Y0), 
	// CHN_IMG_Y + 2
		DONT_CARE,
	// CHN_IMG_Y + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_Y / 4) |
		mDmaID4b(ID_CHN_IMG),
	// CHN_IMG_U + 0
		DONT_CARE,
	// CHN_IMG_U + 1
		mDmaLocMemAddr14b(CUR_U0), 
	// CHN_IMG_U + 2
		DONT_CARE,
	// CHN_IMG_U + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_U / 4) |
		mDmaID4b(ID_CHN_IMG),
	// CHN_IMG_V + 0
		DONT_CARE,
	// CHN_IMG_V + 1
		mDmaLocMemAddr14b(CUR_V0), 
	// CHN_IMG_V + 2
		DONT_CARE,
	// CHN_IMG_V + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_V / 4) |
		mDmaID4b(ID_CHN_IMG),
	///////////////////////////////////////////////
	// CHN_CUR_RE_Y + 0
		DONT_CARE,
	// CHN_CUR_RE_Y + 1
		DONT_CARE,
	// CHN_CUR_RE_Y + 2
		DONT_CARE,
	// CHN_CUR_RE_Y + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_Y / 4) |
		mDmaID4b(ID_CHN_CUR_REC_Y),
	// CHN_CUR_RE_U + 0
		DONT_CARE,
	// CHN_CUR_RE_U + 1
		DONT_CARE,
	// CHN_CUR_RE_U + 2
		DONT_CARE,
	// CHN_CUR_RE_U + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_U / 4) |
		mDmaID4b(ID_CHN_CUR_REC_UV),
	// CHN_CUR_RE_V + 0
		DONT_CARE,
	// CHN_CUR_RE_V + 1
		DONT_CARE,
	// CHN_CUR_RE_V + 2
		DONT_CARE,
	// CHN_CUR_RE_V + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_V / 4) |
		mDmaID4b(ID_CHN_CUR_REC_UV),
    #if defined(TWO_P_EXT)
	///////////////////////////////////////////////////
	// CHN_STORE_MBS
	// CHN_STORE_MBS + 0
		DONT_CARE,
	// CHN_STORE_MBS + 1
		DONT_CARE,
	// CHN_STORE_MBS + 2
		DONT_CARE,
	// CHN_STORE_MBS + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(sizeof(MACROBLOCK_E) / 4) |
		mDmaID4b(ID_CHN_STORE_MBS),
	///////////////////////////////////////////////////
	// CHN_ACDC_MBS
	// CHN_ACDC_MBS + 0
		DONT_CARE,
	// CHN_ACDC_MBS + 1
		DONT_CARE,
	// CHN_ACDC_MBS + 2
		DONT_CARE,
	// CHN_ACDC_MBS + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(sizeof(MACROBLOCK_E) / 4) |
		mDmaID4b(ID_CHN_ACDC_MBS),
	///////////////////////////////////////////////////
	// CHN_LOAD_REF_MBS
	// CHN_LOAD_REF_MBS + 0
		DONT_CARE,
	// CHN_LOAD_REF_MBS + 1
		DONT_CARE,
	// CHN_LOAD_REF_MBS + 2
		DONT_CARE,
	// CHN_LOAD_REF_MBS + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(sizeof(MACROBLOCK_E) / 4) |
		mDmaID4b(ID_CHN_LOAD_REF_MBS),
	#endif
	///////////////////////////////////////////////
	// CHN_LD + 0
		DONT_CARE,
	// CHN_LD + 1
		mDmaLocMemAddr14b(PREDICTOR0_OFF),
	// CHN_LD + 2
		mDmaSysWidth6b(DONT_CARE) |
		mDmaSysOff14b(DONT_CARE) |
		mDmaLocWidth4b(4) |
		mDmaLocOff8b(8 + 1 - 4),
	// CHN_LD + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(0x10) |
		mDmaID4b(ID_CHN_ACDC),
	///////////////////////////////////////////////
	// CHN_ST + 0
		DONT_CARE,
	// CHN_ST + 1
		mDmaLocMemAddr14b(PREDICTOR8_OFF),
	// CHN_ST + 2
		mDmaSysWidth6b(DONT_CARE) |
		mDmaSysOff14b(DONT_CARE) |
		mDmaLocWidth4b(4) |
		mDmaLocOff8b(8 + 1 - 4),
	// CHN_ST + 3
		mDmaIntChainMask1b(FALSE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(FALSE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(0x10) |
		mDmaID4b(ID_CHN_ACDC)
};

uint32_t u32dma_e_const1[] = {
	///////////////////////////// DMA_TOGGLE 1 //////////////////////////
	// 1. CHN_REF_RE_Y, U, V
	// 2. CHN_IMG_Y, U, V
	// 3. CHN_CUR_RE_Y, U, V
	// 4. CHN_LD
	// 5. CHN_ST

	///////////////////////////////////////////////
	// CHN_REF_RE_Y + 0
		DONT_CARE,
	// CHN_REF_RE_Y + 1
		DONT_CARE,
	// CHN_REF_RE_Y + 2
		DONT_CARE,
	// CHN_REF_RE_Y + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(3 * SIZE_Y / 4) |
		mDmaID4b(ID_CHN_REF_REC),
	// CHN_REF_RE_U + 0
		DONT_CARE,
	// CHN_REF_RE_U + 1
		DONT_CARE,
	// CHN_REF_RE_U + 2
		DONT_CARE,
	// CHN_REF_RE_U + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(3 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_REF_REC),
	// CHN_REF_RE_V + 0
		DONT_CARE,
	// CHN_REF_RE_V + 1
		DONT_CARE,
	// CHN_REF_RE_V + 2
		DONT_CARE,
	// CHN_REF_RE_V + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(3 * SIZE_V / 4) |
		mDmaID4b(ID_CHN_REF_REC),
	///////////////////////////////////////////////
	// CHN_IMG_Y + 0
		DONT_CARE,
	// CHN_IMG_Y + 1
		mDmaLocMemAddr14b(CUR_Y1), 
	// CHN_IMG_Y + 2
		DONT_CARE,
	// CHN_IMG_Y + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_Y / 4) |
		mDmaID4b(ID_CHN_IMG),
	// CHN_IMG_U + 0
		DONT_CARE,
	// CHN_IMG_U + 1
		mDmaLocMemAddr14b(CUR_U1), 
	// CHN_IMG_U + 2
		DONT_CARE,
	// CHN_IMG_U + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_U / 4) |
		mDmaID4b(ID_CHN_IMG),
	// CHN_IMG_V + 0
		DONT_CARE,
	// CHN_IMG_V + 1
		mDmaLocMemAddr14b(CUR_V1), 
	// CHN_IMG_V + 2
		DONT_CARE,
	// CHN_IMG_V + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_V / 4) |
		mDmaID4b(ID_CHN_IMG),
	///////////////////////////////////////////////
	// CHN_CUR_RE_Y + 0
		DONT_CARE,
	// CHN_CUR_RE_Y + 1
		DONT_CARE,
	// CHN_CUR_RE_Y + 2
		DONT_CARE,
	// CHN_CUR_RE_Y + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_Y / 4) |
		mDmaID4b(ID_CHN_CUR_REC_Y),
	// CHN_CUR_RE_U + 0
		DONT_CARE,
	// CHN_CUR_RE_U + 1
		DONT_CARE,
	// CHN_CUR_RE_U + 2
		DONT_CARE,
	// CHN_CUR_RE_U + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_U / 4) |
		mDmaID4b(ID_CHN_CUR_REC_UV),
	// CHN_CUR_RE_V + 0
		DONT_CARE,
	// CHN_CUR_RE_V + 1
		DONT_CARE,
	// CHN_CUR_RE_V + 2
		DONT_CARE,
	// CHN_CUR_RE_V + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_V / 4) |
		mDmaID4b(ID_CHN_CUR_REC_UV),
    #if defined(TWO_P_EXT)
	///////////////////////////////////////////////////
	// CHN_STORE_MBS
	// CHN_STORE_MBS + 0
		DONT_CARE,
	// CHN_STORE_MBS + 1
		DONT_CARE,
	// CHN_STORE_MBS + 2
		DONT_CARE,
	// CHN_STORE_MBS + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(sizeof(MACROBLOCK_E) / 4) |
		mDmaID4b(ID_CHN_STORE_MBS),
    ///////////////////////////////////////////////////
	// CHN_ACDC_MBS
	// CHN_ACDC_MBS + 0
		DONT_CARE,
	// CHN_ACDC_MBS + 1
		DONT_CARE,
	// CHN_ACDC_MBS + 2
		DONT_CARE,
	// CHN_ACDC_MBS + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(sizeof(MACROBLOCK_E) / 4) |
		mDmaID4b(ID_CHN_ACDC_MBS),
	///////////////////////////////////////////////////
	// CHN_LOAD_REF_MBS
	// CHN_LOAD_REF_MBS + 0
		DONT_CARE,
	// CHN_LOAD_REF_MBS + 1
		DONT_CARE,
	// CHN_LOAD_REF_MBS + 2
		DONT_CARE,
	// CHN_LOAD_REF_MBS + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(sizeof(MACROBLOCK_E) / 4) |
		mDmaID4b(ID_CHN_LOAD_REF_MBS),
	#endif
	///////////////////////////////////////////////
	// CHN_LD + 0
		DONT_CARE,
	// CHN_LD + 1
		mDmaLocMemAddr14b(PREDICTOR4_OFF),
	// CHN_LD + 2
		mDmaSysWidth6b(DONT_CARE) |
		mDmaSysOff14b(DONT_CARE) |
		mDmaLocWidth4b(4) |
		mDmaLocOff8b(8 + 1 - 4),
	// CHN_LD + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(0x10) |
		mDmaID4b(ID_CHN_ACDC),
	///////////////////////////////////////////////
	// CHN_ST + 0
		DONT_CARE,
	// CHN_ST + 1
		mDmaLocMemAddr14b(PREDICTOR8_OFF),
	// CHN_ST + 2
		mDmaSysWidth6b(DONT_CARE) |
		mDmaSysOff14b(DONT_CARE) |
		mDmaLocWidth4b(4) |
		mDmaLocOff8b(8 + 1 - 4),
	// CHN_ST + 3
		mDmaIntChainMask1b(FALSE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(FALSE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(0x10) |
		mDmaID4b(ID_CHN_ACDC)
};
void 
dma_enc_commandq_init(Encoder * pEnc)
{
	uint32_t * dma_cmd_tgl;
	
	dma_cmd_tgl = (uint32_t *)(pEnc->u32VirtualBaseAddr + DMA_ENC_CMD_OFF_0);
	memcpy (dma_cmd_tgl, u32dma_e_const0, sizeof (u32dma_e_const0));
	dma_cmd_tgl = (uint32_t *)(pEnc->u32VirtualBaseAddr + DMA_ENC_CMD_OFF_1);
	memcpy (dma_cmd_tgl, u32dma_e_const1, sizeof (u32dma_e_const1));
}
void 
dma_enc_commandq_init_each(Encoder * pEnc)
{
	uint32_t * dma_cmd = 
		(uint32_t *) (pEnc->u32VirtualBaseAddr + DMA_ENC_CMD_OFF_0);

	// reference reconstructed buffer
	dma_cmd[CHN_REF_RE_Y + 2] =
	dma_cmd[CHN_REF_RE_Y + 2 + DMA_CMD_STRIDE] =
		mDmaSysWidth6b(SIZE_Y / 8) |
		mDmaSysOff14b((pEnc->mb_width * SIZE_Y / 4) + 1 - (SIZE_Y / 4)) |
		mDmaLocWidth4b(PIXEL_Y / 4) |
		mDmaLocOff8b((4 * PIXEL_Y / 4) + 1 - (PIXEL_Y / 4));
	dma_cmd[CHN_REF_RE_U + 2] =
	dma_cmd[CHN_REF_RE_U + 2 + DMA_CMD_STRIDE] =
		mDmaSysWidth6b(SIZE_U / 8) |
		mDmaSysOff14b((pEnc->mb_width * SIZE_U / 4) + 1 - (SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((4 * PIXEL_U / 4) + 1 - (PIXEL_U / 4));
	dma_cmd[CHN_REF_RE_V + 2] =
	dma_cmd[CHN_REF_RE_V + 2 + DMA_CMD_STRIDE] =
		mDmaSysWidth6b(SIZE_V / 8) |
		mDmaSysOff14b((pEnc->mb_width * SIZE_V / 4) + 1 - (SIZE_V/ 4)) |
		mDmaLocWidth4b(PIXEL_V / 4) |
		mDmaLocOff8b((4 * PIXEL_V / 4) + 1 - (PIXEL_V / 4));

}

void dma_enc_cmd_init_each(Encoder * pEnc)
{
	uint32_t * dma_cmd_tgl0 = (uint32_t *) (pEnc->u32VirtualBaseAddr + DMA_ENC_CMD_OFF_0);
	uint32_t * dma_cmd_tgl1 = (uint32_t *) (pEnc->u32VirtualBaseAddr + DMA_ENC_CMD_OFF_1);

	switch (pEnc->img_fmt) {
		case 0:	// H264_2D
			// input image
			dma_cmd_tgl0[CHN_IMG_U + 1] =
				mDmaLoc2dWidth4b(2) | 							// 2 lines
				mDmaLoc2dOff8b(1 - SIZE_U/4) | // jump to new block
				mDmaLocMemAddr14b(CUR_U0) |
				mDmaLocInc2b(DMA_INCL_0),
			dma_cmd_tgl1[CHN_IMG_U + 1] =
				mDmaLoc2dWidth4b(2) | 							// 2 lines
				mDmaLoc2dOff8b(1 - SIZE_U/4) | // jump to new block
				mDmaLocMemAddr14b(CUR_U1) |
				mDmaLocInc2b(DMA_INCL_0),
			dma_cmd_tgl0[CHN_IMG_U + 2] =
			dma_cmd_tgl1[CHN_IMG_U + 2] =
				mDmaSysWidth6b(DONT_CARE) |
				mDmaSysOff14b(DONT_CARE) |
				mDmaLocWidth4b(PIXEL_U / 4) |
				mDmaLocOff8b((SIZE_U / 4) + 1 - (PIXEL_U / 4));
			dma_cmd_tgl0[CHN_IMG_U + 3] =
			dma_cmd_tgl1[CHN_IMG_U + 3] =
				mDmaIntChainMask1b(TRUE) |
				mDmaEn1b(TRUE) |
				mDmaChainEn1b(TRUE) |
				mDmaDir1b(DMA_DIR_2LOCAL) |
				mDmaSType2b(DMA_DATA_SEQUENTAIL) |
				mDmaLType2b(DMA_DATA_3D) |
				mDmaLen12b(2 * SIZE_U / 4) |
				mDmaID4b(ID_CHN_IMG);
			// dummy chn_img_v, dont care cmd 1, 2
			dma_cmd_tgl0[CHN_IMG_V + 3] =
			dma_cmd_tgl1[CHN_IMG_V + 3] =
				mDmaIntChainMask1b(TRUE) |
				mDmaEn1b(TRUE) |
				mDmaChainEn1b(TRUE) |
				mDmaLen12b(0) |
				mDmaID4b(ID_CHN_IMG);
			break;
		default:		// MP4_2D, 420_1D, 422_1D.
			dma_cmd_tgl0[CHN_IMG_U + 3] =
			dma_cmd_tgl1[CHN_IMG_U + 3] =
			dma_cmd_tgl0[CHN_IMG_V + 3] =
			dma_cmd_tgl1[CHN_IMG_V + 3] =
				mDmaIntChainMask1b(TRUE) |
				mDmaEn1b(TRUE) |
				mDmaChainEn1b(TRUE) |
				mDmaDir1b(DMA_DIR_2LOCAL) |
				mDmaSType2b(DMA_DATA_SEQUENTAIL) |
				mDmaLType2b(DMA_DATA_SEQUENTAIL) |
				mDmaLen12b(SIZE_U / 4) |
				mDmaID4b(ID_CHN_IMG);
			break;
	}

#ifdef DMAWRP
	{
		volatile WRP_reg * wrpreg = (volatile WRP_reg *)pEnc->u32BaseAddr_wrapper;
		IMAGE * img = &pEnc->current1->image;
		wrpreg->pf_go = (1L << 31);	/// reset wrapper
		switch (pEnc->img_fmt) {
			case 2: // Raster scan 420
			case 3: // Raster scan 422
				wrpreg->aim_hfp = pEnc->roi_x >> 4;
				wrpreg->aim_hbp = pEnc->mb_stride - (pEnc->roi_x >> 4) - pEnc->roi_mb_width;
				wrpreg->Aim_width = pEnc->roi_mb_width;
				wrpreg->Aim_height = pEnc->roi_mb_height;
				if (pEnc->img_fmt == 3) {
					wrpreg->Vfmt = (ID_CHN_IMG << 8) |
												BIT6 |	// always 1
												BIT5 |	// encode mode
												5;			// packed 422 mode
					wrpreg->Vsmsa = ((unsigned int)img->y_phy + ((pEnc->roi_x + pEnc->roi_y * pEnc->width) << 1)) >> 2;
				}
				else {
					wrpreg->Vfmt = (ID_CHN_IMG << 8) |
											BIT6 |	// always 1
											BIT5 |			// encode mode
											0;					// planar 420 mode
					wrpreg->Ysmsa = ((unsigned int)img->y_phy + ((pEnc->roi_x 		) + (pEnc->roi_y * pEnc->width		 ))) >> 2;
					wrpreg->Usmsa = ((unsigned int)img->u_phy + ((pEnc->roi_x >> 1) + (pEnc->roi_y * pEnc->width >> 2))) >> 2;
					wrpreg->Vsmsa = ((unsigned int)img->v_phy + ((pEnc->roi_x >> 1) + (pEnc->roi_y * pEnc->width >> 2))) >> 2;	 
				}
				wrpreg->pf_go = 0x01;// DMA wrapper go
				break;
			default: // MP4_2D/H264_2D
				wrpreg->Vfmt = BIT7;				// bypass mode
				break;
		}
	}
#endif
}
