#define DMA_B_GLOBALS
#ifdef LINUX
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/version.h>
#include "me.h"
#include "../common/portab.h"
#include "../common/dma.h"
#include "dma_b.h"
#include "../common/dma_m.h"
#include "decoder.h"
#include "local_mem_d.h"
#include "../common/image.h"
#include "../common/define.h"
#include "../common/mp4.h"
#include "me.h"
#else
#include "me.h"
#include "portab.h"
#include "dma.h"
#include "dma_b.h"
#include "dma_m.h"
#include "decoder.h"
#include "local_mem_d.h"
#include "image.h"
#include "define.h"
#include "mp4.h"
#include "me.h"
#endif

#define DEFAULT_WIDTH	176
#define DEFAULT_HEIGHT	144
#define DEFAULT_STRIDE	DEFAULT_WIDTH
// pipeline arrangement
/*
************************************************** syntax start
*********** dma **********
xx0d:	using dma pp 0
xx1d:	using dma pp 1
LD:		Load ACDC predictor
ST:		Store ACDC predictor
I:		move reconstructed image
Y:		move display YUV frame
R:		move reference frame
RG:		move display RGB frame
LDD:	Load upper row for deblocking
STD:		Store lower row for deblocking

*********** goMCCTL **********
V:	goVLD
M:	goDMC
	0:	dmc_input(& output) direct to INTER_Y_OFF_0
	1:	dmc_input(& output) direct to INTER_Y_OFF_1
DT:	goDT
	0:	YUV2RGB dest. addr:  BUFFER_RGB_OFF_0
	1:	YUV2RGB dest. addr:  BUFFER_RGB_OFF_1
Deb:	goDeb
	0:	DeBk source addr:  INTER_Y_OFF_0
	1:	DeBk source addr:  INTER_Y_OFF_1
************************************************** syntax end

// Relationship:
// 1. LD0d -> V0
// 2. M0 -> I0d -> Y0d
// 3. M0 -> Deb0 -> STD0d
// 4. LDD0d -> Deb0 -> STD0d
// 5. DT0 -> RG0d
// 6. R0d -> P0

i-frame
vld_		|		|	goVLD	|  goDMC	|	img			| rgb		|
toggle	|	load	|	store		|		|	goDT			|		|
		|		|			|		|	yuv			|		|
===============================================================
1		|	LD0d
0		|	LD1d|	V0(ST)
1		|	LD0d|	V1(ST)	|  M0
0		|	LD1d|	V0(ST)	|  M1	|	I0d(DT0,Y0d)
1		|	LD0d|	V1(ST)	|  M0	|	I1d(DT1,Y1d)	| RG0d	|
0		|	LD1d|	V0(ST)	|  M1	|	I0d(DT0,Y0d)	| RG1d	| ...a
1		|	LD0d|	V1(ST)	|  M0	|	I1d(DT1,Y1d)	| RG0d	| ...b
===============================================================

p-frame
vld_		|		|	goVLD	|	ref	|	goDMC	|	img			|rgb	|
toggle	|	load	|	store		|		|	Pxi		|	goDT			|	|
		|		|			|		|			|	yuv			|	|
===============================================================
1		|	LD0d
0		|	LD1d|	V0(ST)
1		|	LD0d|	V1(ST)	|	R0d	|
0		|	LD1d|	V0(ST)	|	R1d	|  M1(P0)		|
1		|	LD0d|	V1(ST)	|	R0d	|  M0(P1)		|	I1d(DT1,Y1d)	|
0		|	LD1d|	V0(ST)	|	R1d	|  M1(P0)		|	I0d(DT0,Y0d)	| RG1d	| ...c
1		|	LD0d|	V1(ST)	|	R0d	|  M0(P1)		|	I1d(DT1,Y1d)	| RG0d	| ...d
===============================================================

==>
compare a & c
0		|	LD1d|	V0(ST)	|  			M1		|	I0d(DT0,Y0d)	| RG1d	| ...a
0		|	LD1d|	V0(ST)	|	R1d	|  M1(P0)		|	I0d(DT0,Y0d)	| RG1d	| ...c

1		|	LD0d|	V1(ST)	| 			 M0		|	I1d(DT1,Y1d)	| RG0d	| ...b
1		|	LD0d|	V1(ST)	|	R0d	|  M0(P1)		|	I1d(DT1,Y1d)	| RG0d	| ...d

*/

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#ifndef LINUX
__align(8)
#endif
uint32_t u32dma_yuv_const0[] = {
	///////////////////////////// DMA_TOGGLE 0 //////////////////////////
	// 1. CHN_REF_4MV_Y0,Y1,Y2,Y3
	// 2. CHN_REF_1MV_Y
	// 6. CHN_REF_U,V
	// 3. CHN_IMG_Y, U,V
	#ifdef TWO_P_EXT
	// 3-1. CHN_STORE_MBS
	// 3-2. CHN_ACDC_MBS
	#endif
	// 4. CHN_YUV_Y, U,V or CHN_RGB
	#ifdef ENABLE_DEBLOCKING
	// 5. CHN_LD_DEBK
	// 6. CHN_ST_DEBK
	#endif
	// 5. CHN_LD
	// 6. CHN_ST

	// 1
	// CHN_REF_4MV_Y0 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y0 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y0_OFF_1) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y0 + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y0 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),
	// CHN_REF_4MV_Y1 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y1 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y1_OFF_1) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y1 + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y1 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),
	// CHN_REF_4MV_Y2 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y2 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y2_OFF_1) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y2 + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y2 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),
	// CHN_REF_4MV_Y3 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y3 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y3_OFF_1) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y3 + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y3 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),

	// 2
	// CHN_REF_1MV_Y + 0
		DONT_CARE,
	// CHN_REF_1MV_Y + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(3) |								// 3 block/row
		mDmaLocMemAddr14b(REF_Y_OFF_1) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_1MV_Y + 2
		mDmaSysWidth6b(3 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (3 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b(((8 * PIXEL_U) / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_1MV_Y + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (3 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(9 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_1MV),

	// 6
	// CHN_REF_REF_U + 0
		DONT_CARE,
	// CHN_REF_REF_U + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_U_OFF_1) | 
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_REF_U + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b(((8 * PIXEL_U) / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_REF_U + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_REF_UV),
	// CHN_REF_REF_V + 0
		DONT_CARE,
	// CHN_REF_REF_V + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_V - 1) * (8 * PIXEL_V) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_V_OFF_1) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_REF_V + 2
		mDmaSysWidth6b(2 * SIZE_V / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * SIZE_V / 4) + 1 - (2 * SIZE_V / 4)) |
		mDmaLocWidth4b(PIXEL_V / 4) |
		mDmaLocOff8b(((8 * PIXEL_V) / 4) + 1 - (PIXEL_V / 4)),
	// CHN_REF_REF_V + 3
		mDmaLoc3dOff8b((8 * PIXEL_V / 4) + 1 - (2 * PIXEL_V / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_REF_UV),

	// 3.
	// CHN_IMG_Y + 0
		DONT_CARE,
	// CHN_IMG_Y + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (2 * PIXEL_U) / 4) |	// jump to next block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(INTER_Y_OFF_0) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_IMG_Y + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((2 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_IMG_Y + 3
		mDmaLoc3dOff8b(1) |						// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_IMG),
	// CHN_IMG_U + 0
		DONT_CARE,
	// CHN_IMG_U + 1
		mDmaLocMemAddr14b(INTER_U_OFF_0),
	// CHN_IMG_U + 2
		DONT_CARE, 								// dont care block width
	// CHN_IMG_U + 3
		mDmaLoc3dOff8b(DONT_CARE) |
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_U / 4) |
		mDmaID4b(ID_CHN_IMG),
	// CHN_IMG_V + 0
		DONT_CARE,
	// CHN_IMG_V + 1
		mDmaLocMemAddr14b(INTER_V_OFF_0),
	// CHN_IMG_V + 2
		DONT_CARE,								// dont care block width
	// CHN_IMG_V + 3
		mDmaLoc3dOff8b(DONT_CARE) |
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_V / 4) |
		mDmaID4b(ID_CHN_IMG),
	#if (defined(TWO_P_EXT) || defined(TWO_P_INT))
	///////////////////////////////////////////////////
	// 3-1. CHN_STORE_MBS
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
		mDmaLen12b(sizeof(MACROBLOCK) / 4) |
		mDmaID4b(ID_CHN_STORE_MBS),
	///////////////////////////////////////////////////
	// 3-2. CHN_ACDC_MBS
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
		mDmaLen12b(sizeof(MACROBLOCK) / 4) |
		mDmaID4b(ID_CHN_ACDC_MBS),
	#endif
  #ifdef ENABLE_DEBLOCKING
    ///////////////////////////////////////////////////
	// 4. 
		// CHN_YUV_TOP_Y + 0
			DONT_CARE,
		// CHN_YUV_TOP_Y + 1
			mDmaLocMemAddr14b(DEBK_TOP_Y_OFF_1),
		// CHN_YUV_TOP_Y + 2
			mDmaSysWidth6b(PIXEL_Y / 8) |
			mDmaSysOff14b((DEFAULT_STRIDE / 4) + 1 - (PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_TOP_Y + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_Y/2) / 4) |
			mDmaID4b(ID_CHN_YUV_TOP),
		// CHN_YUV_TOP_U + 0
			DONT_CARE,
		// CHN_YUV_TOP_U + 1
			mDmaLocMemAddr14b(DEBK_TOP_U_OFF_1),
		// CHN_YUV_TOP_U + 2
			mDmaSysWidth6b(PIXEL_U / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_U / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_TOP_U + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_U / 2) / 4) |
			mDmaID4b(ID_CHN_YUV_TOP),
		// CHN_YUV_TOP_V + 0
			DONT_CARE,
		// CHN_YUV_TOP_V + 1
			mDmaLocMemAddr14b(DEBK_TOP_V_OFF_1),
		// CHN_YUV_TOP_V + 2
			mDmaSysWidth6b(PIXEL_V / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_V / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_TOP_V + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_V / 2) / 4) |
			mDmaID4b(ID_CHN_YUV_TOP),
		// CHN_YUV_MID_Y + 0
			DONT_CARE,
		// CHN_YUV_MID_Y + 1
			mDmaLocMemAddr14b(DEBK_MID_Y_OFF_1),
		// CHN_YUV_MID_Y + 2
			mDmaSysWidth6b(PIXEL_Y / 8) |
			mDmaSysOff14b((DEFAULT_STRIDE / 4) + 1 - (PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_MID_Y + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_Y/2) / 4) |
			mDmaID4b(ID_CHN_YUV_MID),
		// CHN_YUV_MID_U + 0
			DONT_CARE,
		// CHN_YUV_MID_U + 1
			mDmaLocMemAddr14b(DEBK_MID_U_OFF_1),
		// CHN_YUV_MID_U + 2
			mDmaSysWidth6b(PIXEL_U / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_U / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_MID_U + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_U / 2) / 4) |
			mDmaID4b(ID_CHN_YUV_MID),
		// CHN_YUV_MID_V + 0
			DONT_CARE,
		// CHN_YUV_MID_V + 1
			mDmaLocMemAddr14b(DEBK_MID_V_OFF_1),
		// CHN_YUV_MID_V + 2
			mDmaSysWidth6b(PIXEL_V / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_V / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_MID_V + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_V / 2) / 4) |
			mDmaID4b(ID_CHN_YUV_MID),
		// CHN_YUV_BOT_Y + 0
			DONT_CARE,
		// CHN_YUV_BOT_Y + 1
			mDmaLocMemAddr14b(DEBK_BOT_Y_OFF_1),
		// CHN_YUV_BOT_Y + 2
			mDmaSysWidth6b(PIXEL_Y / 8) |
			mDmaSysOff14b((DEFAULT_STRIDE / 4) + 1 - (PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_BOT_Y + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_Y/2) / 4) |
			mDmaID4b(ID_CHN_YUV_BOT),
		// CHN_YUV_BOT_U + 0
			DONT_CARE,
		// CHN_YUV_BOT_U + 1
			mDmaLocMemAddr14b(DEBK_BOT_U_OFF_1),
		// CHN_YUV_BOT_U + 2
			mDmaSysWidth6b(PIXEL_U / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_U / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_BOT_U + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_U / 2) / 4) |
			mDmaID4b(ID_CHN_YUV_BOT),
		// CHN_YUV_BOT_V + 0
			DONT_CARE,
		// CHN_YUV_BOT_V + 1
			mDmaLocMemAddr14b(DEBK_BOT_V_OFF_1),
		// CHN_YUV_BOT_V + 2
			mDmaSysWidth6b(PIXEL_V / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_V / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_BOT_V + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_V / 2) / 4) |
			mDmaID4b(ID_CHN_YUV_BOT),

  #else //#ifdef ENABLE_DEBLOCKING
	
	///////////////////////////////////////////////////
	// 4. 
		// CHN_YUV_Y + 0
			DONT_CARE,
		// CHN_YUV_Y + 1
			mDmaLoc2dWidth4b(16) |				// 16 lines/MB
			mDmaLoc2dOff8b(DONT_CARE) |
			mDmaLoc3dWidth4b(DONT_CARE) |			// 2 block/row
			mDmaLocMemAddr14b(INTER_Y_OFF_0) |
			mDmaLocInc2b(DMA_INCL_0),
		// CHN_YUV_Y + 2
			mDmaSysWidth6b(PIXEL_Y / 8) |
			mDmaSysOff14b((DEFAULT_STRIDE / 4) + 1 - (PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_Y + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b(SIZE_Y / 4) |
			mDmaID4b(ID_CHN_YUV),
		// CHN_YUV_U + 0
			DONT_CARE,
		// CHN_YUV_U + 1
			mDmaLocMemAddr14b(INTER_U_OFF_0),
		// CHN_YUV_U + 2
			mDmaSysWidth6b(PIXEL_U / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_U / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_U + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b(SIZE_U / 4) |
			mDmaID4b(ID_CHN_YUV),
		// CHN_YUV_V + 0
			DONT_CARE,
		// CHN_YUV_V + 1
			mDmaLocMemAddr14b(INTER_V_OFF_0),
		// CHN_YUV_V + 2
			mDmaSysWidth6b(PIXEL_V / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_V / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_V + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b(SIZE_V / 4) |
			mDmaID4b(ID_CHN_YUV),
  #endif //#ifdef ENABLE_DEBLOCKING
	
  #ifdef ENABLE_DEBLOCKING
   #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
   
   #else // #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
	///////////////////////////////////////////////////
	// 5
	// CHN_STORE_DEBK + 0
		DONT_CARE,
	// CHN_STORE_DEBK + 1
		mDmaLocMemAddr14b(DEBK_ST_OFF_1),
	// CHN_STORE_DEBK + 2
		DONT_CARE,
	// CHN_STORE_DEBK + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_ST_DEBK),
	///////////////////////////////////////////////////
	// 6
	// CHN_LOAD_DEBK + 0
		DONT_CARE,
	// CHN_LOAD_DEBK + 1
		mDmaLocMemAddr14b(DEBK_LD_OFF_1),
	// CHN_LOAD_DEBK + 2
		DONT_CARE,
	// CHN_LOAD_DEBK + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_LD_DEBK),
   #endif // #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
  #endif // #ifdef ENABLE_DEBLOCKING
	
	///////////////////////////////////////////////////
	// 5
	// CHN_LOAD_PREDITOR + 0
		DONT_CARE,
	// CHN_LOAD_PREDITOR + 1
		mDmaLocMemAddr14b(PREDICTOR4_OFF),		// watch-out
	// CHN_LOAD_PREDITOR + 2
		mDmaSysWidth6b(DONT_CARE) |
		mDmaSysOff14b(DONT_CARE) |
		mDmaLocWidth4b(4) |
		mDmaLocOff8b(8 + 1 - 4),
	// CHN_LOAD_PREDITOR + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(0x10) |
		mDmaID4b(ID_CHN_ACDC),
	////////////////////////////////////////////////////////////////
	// 6
	// CHN_STORE_PREDITOR + 0
		DONT_CARE,
	// CHN_STORE_PREDITOR + 1
		mDmaLocMemAddr14b(PREDICTOR8_OFF),
	// CHN_STORE_PREDITOR + 2
		mDmaSysWidth6b(DONT_CARE) |
		mDmaSysOff14b(DONT_CARE) |
		mDmaLocWidth4b(4) |
		mDmaLocOff8b(8 + 1 - 4),
	// CHN_STORE_PREDITOR + 3,
		mDmaLoc3dOff8b(DONT_CARE) |
      #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
        mDmaIntChainMask1b(TRUE) |
      #else
		mDmaIntChainMask1b(FALSE) |
	  #endif
		mDmaEn1b(TRUE) |
	  #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
	    mDmaChainEn1b(TRUE) |
	  #else
		mDmaChainEn1b(FALSE) |
	  #endif
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(0x10) |
		mDmaID4b(ID_CHN_ACDC),
		
		
  #ifdef ENABLE_DEBLOCKING
   #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
    ///////////////////////////////////////////////////
	// 5
	// CHN_STORE_DEBK + 0
		DONT_CARE,
	// CHN_STORE_DEBK + 1
		mDmaLocMemAddr14b(DEBK_ST_OFF_1),
	// CHN_STORE_DEBK + 2
		DONT_CARE,
	// CHN_STORE_DEBK + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_ST_DEBK),
	///////////////////////////////////////////////////
	// 6
	// CHN_LOAD_DEBK + 0
		DONT_CARE,
	// CHN_LOAD_DEBK + 1
		mDmaLocMemAddr14b(DEBK_LD_OFF_1),
	// CHN_LOAD_DEBK + 2
		DONT_CARE,
	// CHN_LOAD_DEBK + 3
        mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
        mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_LD_DEBK),		
	///////////////////////////////////////////////////
	// Add CHN_DMA_DUMMY here because the DMA engine can't work well when the last entry of
	// DMA command chain is disabled.
	// CHN_DMA_DUMMY + 0
		DONT_CARE,
	// CHN_DMA_DUMMY + 1
		DONT_CARE,
	// CHN_DMA_DUMMY + 2
		DONT_CARE,
	// CHN_DMA_DUMMY + 3
		mDmaIntChainMask1b(FALSE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(FALSE) |			// off the chain
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(0) |
		mDmaID4b(ID_CHN_AWYS)
   
   #else // #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
	
   #endif // #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
  #endif // #ifdef ENABLE_DEBLOCKING
};
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )


#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
#ifndef LINUX
__align(8)
#endif
uint32_t u32dma_yuv_const1[] = {
	///////////////////////////// DMA_TOGGLE 1 //////////////////////////
	// 1
	// CHN_REF_4MV_Y0 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y0 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y0_OFF_0) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y0 + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y0 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),
	// CHN_REF_4MV_Y1 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y1 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y1_OFF_0) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y1
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y1 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),
	// CHN_REF_4MV_Y2 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y2 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y2_OFF_0) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y2 + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y2 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),
	// CHN_REF_4MV_Y3 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y3 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y3_OFF_0) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y3 + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y3 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),

	// 2
	// CHN_REF_1MV_Y + 0
		DONT_CARE,
	// CHN_REF_1MV_Y + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(3) |								// 3 block/row
		mDmaLocMemAddr14b(REF_Y_OFF_0) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_1MV_Y + 2
		mDmaSysWidth6b(3 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (3 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b(((8 * PIXEL_U) / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_1MV_Y + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (3 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(9 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_1MV),

	// 6
	// CHN_REF_REF_U + 0
		DONT_CARE,
	// CHN_REF_REF_U + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_U_OFF_0) | 
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_REF_U + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b(((8 * PIXEL_U) / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_REF_U + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_REF_UV),
	// CHN_REF_REF_V + 0
		DONT_CARE,
	// CHN_REF_REF_V + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_V - 1) * (8 * PIXEL_V) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_V_OFF_0) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_REF_V + 2
		mDmaSysWidth6b(2 * SIZE_V / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * SIZE_V / 4) + 1 - (2 * SIZE_V / 4)) |
		mDmaLocWidth4b(PIXEL_V / 4) |
		mDmaLocOff8b(((8 * PIXEL_V) / 4) + 1 - (PIXEL_V / 4)),
	// CHN_REF_REF_V + 3
		mDmaLoc3dOff8b((8 * PIXEL_V / 4) + 1 - (2 * PIXEL_V / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_REF_UV),

	// 3.
	// CHN_IMG_Y + 0
		DONT_CARE,
	// CHN_IMG_Y + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (2 * PIXEL_U) / 4) |	// jump to next block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(INTER_Y_OFF_1) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_IMG_Y + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((2 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_IMG_Y + 3
		mDmaLoc3dOff8b(1) |						// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_IMG),
	// CHN_IMG_U + 0
		DONT_CARE,
	// CHN_IMG_U + 1
		mDmaLocMemAddr14b(INTER_U_OFF_1),
	// CHN_IMG_U + 2
		DONT_CARE, 								// dont care block width
	// CHN_IMG_U + 3
		mDmaLoc3dOff8b(DONT_CARE) |
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_U / 4) |
		mDmaID4b(ID_CHN_IMG),
	// CHN_IMG_V + 0
		DONT_CARE,
	// CHN_IMG_V + 1
		mDmaLocMemAddr14b(INTER_V_OFF_1),
	// CHN_IMG_V + 2
		DONT_CARE,								// dont care block width
	// CHN_IMG_V + 3
		mDmaLoc3dOff8b(DONT_CARE) |
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_V / 4) |
		mDmaID4b(ID_CHN_IMG),
	#if (defined(TWO_P_EXT) || defined(TWO_P_INT))
	///////////////////////////////////////////////////
	// 3-1. CHN_STORE_MBS
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
		mDmaLen12b(sizeof(MACROBLOCK) / 4) |
		mDmaID4b(ID_CHN_STORE_MBS),
	///////////////////////////////////////////////////
	// 3-2. CHN_ACDC_MBS
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
		mDmaLen12b(sizeof(MACROBLOCK)/ 4) |
		mDmaID4b(ID_CHN_ACDC_MBS),
	#endif
	
  #ifdef ENABLE_DEBLOCKING	
///////////////////////////////////////////////////
	// 4
		// CHN_YUV_TOP_Y + 0
			DONT_CARE,
		// CHN_YUV_TOP_Y + 1
			mDmaLocMemAddr14b(DEBK_TOP_Y_OFF_0),
		// CHN_YUV_TOP_Y + 2
			mDmaSysWidth6b(PIXEL_Y / 8) |
			mDmaSysOff14b((DEFAULT_STRIDE / 4) + 1 - (PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_TOP_Y + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_Y/2) / 4) |
			mDmaID4b(ID_CHN_YUV_TOP),
		// CHN_YUV_TOP_U + 0
			DONT_CARE,
		// CHN_YUV_TOP_U + 1
			mDmaLocMemAddr14b(DEBK_TOP_U_OFF_0),
		// CHN_YUV_TOP_U + 2
			mDmaSysWidth6b(PIXEL_U / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_U / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_TOP_U + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_U / 2) / 4) |
			mDmaID4b(ID_CHN_YUV_TOP),
		// CHN_YUV_TOP_V + 0
			DONT_CARE,
		// CHN_YUV_TOP_V + 1
			mDmaLocMemAddr14b(DEBK_TOP_V_OFF_0),
		// CHN_YUV_TOP_V + 2
			mDmaSysWidth6b(PIXEL_V / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_V / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_TOP_V + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_V / 2) / 4) |
			mDmaID4b(ID_CHN_YUV_TOP),
		// CHN_YUV_MID_Y + 0
			DONT_CARE,
		// CHN_YUV_MID_Y + 1
			mDmaLocMemAddr14b(DEBK_MID_Y_OFF_0),
		// CHN_YUV_MID_Y + 2
			mDmaSysWidth6b(PIXEL_Y / 8) |
			mDmaSysOff14b((DEFAULT_STRIDE / 4) + 1 - (PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_MID_Y + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_Y/2) / 4) |
			mDmaID4b(ID_CHN_YUV_MID),
		// CHN_YUV_MID_U + 0
			DONT_CARE,
		// CHN_YUV_MID_U + 1
			mDmaLocMemAddr14b(DEBK_MID_U_OFF_0),
		// CHN_YUV_MID_U + 2
			mDmaSysWidth6b(PIXEL_U / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_U / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_MID_U + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_U / 2) / 4) |
			mDmaID4b(ID_CHN_YUV_MID),
		// CHN_YUV_MID_V + 0
			DONT_CARE,
		// CHN_YUV_MID_V + 1
			mDmaLocMemAddr14b(DEBK_MID_V_OFF_0),
		// CHN_YUV_MID_V + 2
			mDmaSysWidth6b(PIXEL_V / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_V / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_MID_V + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_V / 2) / 4) |
			mDmaID4b(ID_CHN_YUV_MID),
		// CHN_YUV_BOT_Y + 0
			DONT_CARE,
		// CHN_YUV_BOT_Y + 1
			mDmaLocMemAddr14b(DEBK_BOT_Y_OFF_0),
		// CHN_YUV_BOT_Y + 2
			mDmaSysWidth6b(PIXEL_Y / 8) |
			mDmaSysOff14b((DEFAULT_STRIDE / 4) + 1 - (PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_BOT_Y + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_Y/2) / 4) |
			mDmaID4b(ID_CHN_YUV_BOT),
		// CHN_YUV_BOT_U + 0
			DONT_CARE,
		// CHN_YUV_BOT_U + 1
			mDmaLocMemAddr14b(DEBK_BOT_U_OFF_0),
		// CHN_YUV_BOT_U + 2
			mDmaSysWidth6b(PIXEL_U / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_U / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_BOT_U + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_U / 2) / 4) |
			mDmaID4b(ID_CHN_YUV_BOT),
		// CHN_YUV_BOT_V + 0
			DONT_CARE,
		// CHN_YUV_BOT_V + 1
			mDmaLocMemAddr14b(DEBK_BOT_V_OFF_0),
		// CHN_YUV_BOT_V + 2
			mDmaSysWidth6b(PIXEL_V / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_V / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_BOT_V + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((SIZE_V / 2) / 4) |
			mDmaID4b(ID_CHN_YUV_BOT),
  #else // #ifdef ENABLE_DEBLOCKING
	
	///////////////////////////////////////////////////
	// 4
		// CHN_YUV_Y + 0
			DONT_CARE,
		// CHN_YUV_Y + 1
			mDmaLoc2dWidth4b(16) |				// 16 lines/MB
			mDmaLoc2dOff8b(DONT_CARE) |
			mDmaLoc3dWidth4b(DONT_CARE) |			// 2 block/row
			mDmaLocMemAddr14b(INTER_Y_OFF_1) |
			mDmaLocInc2b(DMA_INCL_0),
		// CHN_YUV_Y + 2
			mDmaSysWidth6b(PIXEL_Y / 8) |
			mDmaSysOff14b((DEFAULT_STRIDE / 4) + 1 - (PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_Y + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b(SIZE_Y / 4) |
			mDmaID4b(ID_CHN_YUV),
		// CHN_YUV_U + 0
			DONT_CARE,
		// CHN_YUV_U + 1
			mDmaLocMemAddr14b(INTER_U_OFF_1),
		// CHN_YUV_U + 2
			mDmaSysWidth6b(PIXEL_U / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_U / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_U + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b(SIZE_U / 4) |
			mDmaID4b(ID_CHN_YUV),
		// CHN_YUV_V + 0
			DONT_CARE,
		// CHN_YUV_V + 1
			mDmaLocMemAddr14b(INTER_V_OFF_1),
		// CHN_YUV_V + 2
			mDmaSysWidth6b(PIXEL_V / 8) |
			mDmaSysOff14b(((DEFAULT_STRIDE / 2) / 4) + 1 - (PIXEL_V / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_YUV_V + 3
			mDmaLoc3dOff8b(DONT_CARE) |
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b(SIZE_V / 4) |
			mDmaID4b(ID_CHN_YUV),
  #endif // #ifdef ENABLE_DEBLOCKING
  
  #ifdef ENABLE_DEBLOCKING  
   #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
   
   #else
  	///////////////////////////////////////////////////
	// 5
	// CHN_STORE_DEBK + 0
		DONT_CARE,
	// CHN_STORE_DEBK + 1
		mDmaLocMemAddr14b(DEBK_ST_OFF_0),
	// CHN_STORE_DEBK + 2
		DONT_CARE,
	// CHN_STORE_DEBK + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_ST_DEBK),
	///////////////////////////////////////////////////
	// 6
	// CHN_LOAD_DEBK + 0
		DONT_CARE,
	// CHN_LOAD_DEBK + 1
		mDmaLocMemAddr14b(DEBK_LD_OFF_0),
	// CHN_LOAD_DEBK + 2
		DONT_CARE,
	// CHN_LOAD_DEBK + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_LD_DEBK),
   #endif // #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
  #endif // #ifdef ENABLE_DEBLOCKING

	///////////////////////////////////////////////////
	// 5
	// CHN_LOAD_PREDITOR + 0
		DONT_CARE,
	// CHN_LOAD_PREDITOR + 1
		mDmaLocMemAddr14b(PREDICTOR0_OFF),		// watch-out
	// CHN_LOAD_PREDITOR + 2
		mDmaSysWidth6b(DONT_CARE) |
		mDmaSysOff14b(DONT_CARE) |
		mDmaLocWidth4b(4) |
		mDmaLocOff8b(8 + 1 - 4),
	// CHN_LOAD_PREDITOR + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(0x10) |
		mDmaID4b(ID_CHN_ACDC),
	////////////////////////////////////////////////////////////////
	// 6
	// CHN_STORE_PREDITOR + 0
		DONT_CARE,
	// CHN_STORE_PREDITOR + 1
		mDmaLocMemAddr14b(PREDICTOR8_OFF),
	// CHN_STORE_PREDITOR + 2
		mDmaSysWidth6b(DONT_CARE) |
		mDmaSysOff14b(DONT_CARE) |
		mDmaLocWidth4b(4) |
		mDmaLocOff8b(8 + 1 - 4),
	// CHN_STORE_PREDITOR + 3,
		mDmaLoc3dOff8b(DONT_CARE) |
	  #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
	    mDmaIntChainMask1b(TRUE) |
	  #else
		mDmaIntChainMask1b(FALSE) |
	  #endif
		mDmaEn1b(TRUE) |
	  #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
	    mDmaChainEn1b(TRUE) |
	  #else
		mDmaChainEn1b(FALSE) |
	  #endif
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(0x10) |
		mDmaID4b(ID_CHN_ACDC)
		
  #ifdef ENABLE_DEBLOCKING  
   #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
       ,
    ///////////////////////////////////////////////////
	// 5
	// CHN_STORE_DEBK + 0
		DONT_CARE,
	// CHN_STORE_DEBK + 1
		mDmaLocMemAddr14b(DEBK_ST_OFF_0),
	// CHN_STORE_DEBK + 2
		DONT_CARE,
	// CHN_STORE_DEBK + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_ST_DEBK),
	///////////////////////////////////////////////////
	// 6
	// CHN_LOAD_DEBK + 0
		DONT_CARE,
	// CHN_LOAD_DEBK + 1
		mDmaLocMemAddr14b(DEBK_LD_OFF_0),
	// CHN_LOAD_DEBK + 2
		DONT_CARE,
	// CHN_LOAD_DEBK + 3
        mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
        mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_LD_DEBK),
	///////////////////////////////////////////////////
	// Add CHN_DMA_DUMMY here because the DMA engine can't work well when the last entry of
	// DMA command chain is disabled.
	// CHN_DMA_DUMMY + 0
		DONT_CARE,
	// CHN_DMA_DUMMY + 1
		DONT_CARE,
	// CHN_DMA_DUMMY + 2
		DONT_CARE,
	// CHN_DMA_DUMMY + 3
		mDmaIntChainMask1b(FALSE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(FALSE) |          // off the chain
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(0) |
		mDmaID4b(ID_CHN_AWYS)
   #else
  	
   #endif // #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
  #endif // #ifdef ENABLE_DEBLOCKING

};
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
#ifndef LINUX
__align(8)
#endif
uint32_t u32dma_rgb_const0[] = {
	///////////////////////////// DMA_TOGGLE 0 //////////////////////////
	// 1. CHN_REF_4MV_Y0,Y1,Y2,Y3
	// 2. CHN_REF_1MV_Y
	// 6. CHN_REF_U,V
	// 3. CHN_IMG_Y, U,V
	#ifdef TWO_P_EXT
	// 3-1. CHN_STORE_MBS
	// 3-2. CHN_ACDC_MBS
	#endif
	// 4. CHN_YUV_Y, U,V or CHN_RGB
	#ifdef ENABLE_DEBLOCKING
	// 5. CHN_LD_DEBK
	// 6. CHN_ST_DEBK
	#endif
	// 5. CHN_LD
	// 6. CHN_ST

	// 1
	// CHN_REF_4MV_Y0 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y0 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y0_OFF_1) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y0 + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y0 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),
	// CHN_REF_4MV_Y1 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y1 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y1_OFF_1) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y1 + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y1 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),
	// CHN_REF_4MV_Y2 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y2 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y2_OFF_1) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y2 + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y2 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),
	// CHN_REF_4MV_Y3 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y3 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y3_OFF_1) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y3 + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y3 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),

	// 2
	// CHN_REF_1MV_Y + 0
		DONT_CARE,
	// CHN_REF_1MV_Y + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(3) |								// 3 block/row
		mDmaLocMemAddr14b(REF_Y_OFF_1) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_1MV_Y + 2
		mDmaSysWidth6b(3 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (3 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b(((8 * PIXEL_U) / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_1MV_Y + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (3 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(9 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_1MV),

	// 6
	// CHN_REF_REF_U + 0
		DONT_CARE,
	// CHN_REF_REF_U + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_U_OFF_1) | 
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_REF_U + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b(((8 * PIXEL_U) / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_REF_U + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_REF_UV),
	// CHN_REF_REF_V + 0
		DONT_CARE,
	// CHN_REF_REF_V + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_V - 1) * (8 * PIXEL_V) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_V_OFF_1) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_REF_V + 2
		mDmaSysWidth6b(2 * SIZE_V / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * SIZE_V / 4) + 1 - (2 * SIZE_V / 4)) |
		mDmaLocWidth4b(PIXEL_V / 4) |
		mDmaLocOff8b(((8 * PIXEL_V) / 4) + 1 - (PIXEL_V / 4)),
	// CHN_REF_REF_V + 3
		mDmaLoc3dOff8b((8 * PIXEL_V / 4) + 1 - (2 * PIXEL_V / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_REF_UV),

	// 3.
	// CHN_IMG_Y + 0
		DONT_CARE,
	// CHN_IMG_Y + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (2 * PIXEL_U) / 4) |	// jump to next block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(INTER_Y_OFF_0) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_IMG_Y + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((2 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_IMG_Y + 3
		mDmaLoc3dOff8b(1) |						// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_IMG),
	// CHN_IMG_U + 0
		DONT_CARE,
	// CHN_IMG_U + 1
		mDmaLocMemAddr14b(INTER_U_OFF_0),
	// CHN_IMG_U + 2
		DONT_CARE, 								// dont care block width
	// CHN_IMG_U + 3
		mDmaLoc3dOff8b(DONT_CARE) |
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_U / 4) |
		mDmaID4b(ID_CHN_IMG),
	// CHN_IMG_V + 0
		DONT_CARE,
	// CHN_IMG_V + 1
		mDmaLocMemAddr14b(INTER_V_OFF_0),
	// CHN_IMG_V + 2
		DONT_CARE,								// dont care block width
	// CHN_IMG_V + 3
		mDmaLoc3dOff8b(DONT_CARE) |
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_V / 4) |
		mDmaID4b(ID_CHN_IMG),
	#if (defined(TWO_P_EXT) || defined(TWO_P_INT))
	///////////////////////////////////////////////////
	// 3-1. CHN_STORE_MBS
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
		mDmaLen12b(sizeof(MACROBLOCK) / 4) |
		mDmaID4b(ID_CHN_STORE_MBS),
	///////////////////////////////////////////////////
	// 3-2. CHN_ACDC_MBS
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
		mDmaLen12b(sizeof(MACROBLOCK) / 4) |
		mDmaID4b(ID_CHN_ACDC_MBS),
	#endif
  #ifdef ENABLE_DEBLOCKING
    ///////////////////////////////////////////////////
	// 4. 
		// CHN_RGB_TOP + 0
			DONT_CARE,
		// CHN_RGB_TOP + 1
			mDmaLocMemAddr14b(BUFFER_RGB_TOP_OFF_1(RGB_PIXEL_SIZE_2)),
		// CHN_RGB_TOP + 2
			mDmaSysWidth6b(RGB_PIXEL_SIZE_2 * PIXEL_Y / 8) |
			mDmaSysOff14b((RGB_PIXEL_SIZE_2 * DEFAULT_STRIDE / 4) + 1 - (RGB_PIXEL_SIZE_2 * PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_RGB_TOP + 3
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((RGB_PIXEL_SIZE_2 * SIZE_Y / 2) / 4) |
			mDmaID4b(ID_CHN_RGB_TOP),
		// CHN_RGB_MID + 0
			DONT_CARE,
		// CHN_RGB_MID + 1
			mDmaLocMemAddr14b(BUFFER_RGB_MID_OFF_1(RGB_PIXEL_SIZE_2)),
		// CHN_RGB_MID + 2
			mDmaSysWidth6b(RGB_PIXEL_SIZE_2 * PIXEL_Y / 8) |
			mDmaSysOff14b((RGB_PIXEL_SIZE_2 * DEFAULT_STRIDE / 4) + 1 - (RGB_PIXEL_SIZE_2 * PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_RGB_MID + 3
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((RGB_PIXEL_SIZE_2 * SIZE_Y / 2) / 4) |
			mDmaID4b(ID_CHN_RGB_MID),
		// CHN_RGB_BOT + 0
			DONT_CARE,
		// CHN_RGB_BOT + 1
			mDmaLocMemAddr14b(BUFFER_RGB_BOT_OFF_1(RGB_PIXEL_SIZE_2)),
		// CHN_RGB_BOT + 2
			mDmaSysWidth6b(RGB_PIXEL_SIZE_2 * PIXEL_Y / 8) |
			mDmaSysOff14b((RGB_PIXEL_SIZE_2 * DEFAULT_STRIDE / 4) + 1 - (RGB_PIXEL_SIZE_2 * PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_RGB_BOT + 3
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((RGB_PIXEL_SIZE_2 * SIZE_Y / 2) / 4) |
			mDmaID4b(ID_CHN_RGB_BOT),
		
  #else //#ifdef ENABLE_DEBLOCKING
	
	///////////////////////////////////////////////////
	// 4. 
		// CHN_RGB + 0
			DONT_CARE,
		// CHN_RGB + 1
			mDmaLocMemAddr14b(BUFFER_RGB_OFF_1(RGB_PIXEL_SIZE_2)),		// watch-out
		// CHN_RGB + 2
			mDmaSysWidth6b(RGB_PIXEL_SIZE_2 * PIXEL_Y / 8) |
			mDmaSysOff14b((RGB_PIXEL_SIZE_2 * DEFAULT_STRIDE / 4) + 1 - (RGB_PIXEL_SIZE_2 * PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_RGB + 3
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b(RGB_PIXEL_SIZE_2 * SIZE_Y / 4) |
			mDmaID4b(ID_CHN_RGB),
  #endif //#ifdef ENABLE_DEBLOCKING
	
  #ifdef ENABLE_DEBLOCKING
   #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
   
   #else // #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
	///////////////////////////////////////////////////
	// 5
	// CHN_STORE_DEBK + 0
		DONT_CARE,
	// CHN_STORE_DEBK + 1
		mDmaLocMemAddr14b(DEBK_ST_OFF_1),
	// CHN_STORE_DEBK + 2
		DONT_CARE,
	// CHN_STORE_DEBK + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_ST_DEBK),
	///////////////////////////////////////////////////
	// 6
	// CHN_LOAD_DEBK + 0
		DONT_CARE,
	// CHN_LOAD_DEBK + 1
		mDmaLocMemAddr14b(DEBK_LD_OFF_1),
	// CHN_LOAD_DEBK + 2
		DONT_CARE,
	// CHN_LOAD_DEBK + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_LD_DEBK),
   #endif // #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
  #endif // #ifdef ENABLE_DEBLOCKING
	
	///////////////////////////////////////////////////
	// 5
	// CHN_LOAD_PREDITOR + 0
		DONT_CARE,
	// CHN_LOAD_PREDITOR + 1
		mDmaLocMemAddr14b(PREDICTOR4_OFF),		// watch-out
	// CHN_LOAD_PREDITOR + 2
		mDmaSysWidth6b(DONT_CARE) |
		mDmaSysOff14b(DONT_CARE) |
		mDmaLocWidth4b(4) |
		mDmaLocOff8b(8 + 1 - 4),
	// CHN_LOAD_PREDITOR + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(0x10) |
		mDmaID4b(ID_CHN_ACDC),
	////////////////////////////////////////////////////////////////
	// 6
	// CHN_STORE_PREDITOR + 0
		DONT_CARE,
	// CHN_STORE_PREDITOR + 1
		mDmaLocMemAddr14b(PREDICTOR8_OFF),
	// CHN_STORE_PREDITOR + 2
		mDmaSysWidth6b(DONT_CARE) |
		mDmaSysOff14b(DONT_CARE) |
		mDmaLocWidth4b(4) |
		mDmaLocOff8b(8 + 1 - 4),
	// CHN_STORE_PREDITOR + 3,
		mDmaLoc3dOff8b(DONT_CARE) |
      #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
        mDmaIntChainMask1b(TRUE) |
      #else
		mDmaIntChainMask1b(FALSE) |
	  #endif
		mDmaEn1b(TRUE) |
	  #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
	    mDmaChainEn1b(TRUE) |
	  #else
		mDmaChainEn1b(FALSE) |
	  #endif
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(0x10) |
		mDmaID4b(ID_CHN_ACDC),
		
		
  #ifdef ENABLE_DEBLOCKING
   #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
    ///////////////////////////////////////////////////
	// 5
	// CHN_STORE_DEBK + 0
		DONT_CARE,
	// CHN_STORE_DEBK + 1
		mDmaLocMemAddr14b(DEBK_ST_OFF_1),
	// CHN_STORE_DEBK + 2
		DONT_CARE,
	// CHN_STORE_DEBK + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_ST_DEBK),
	///////////////////////////////////////////////////
	// 6
	// CHN_LOAD_DEBK + 0
		DONT_CARE,
	// CHN_LOAD_DEBK + 1
		mDmaLocMemAddr14b(DEBK_LD_OFF_1),
	// CHN_LOAD_DEBK + 2
		DONT_CARE,
	// CHN_LOAD_DEBK + 3
        mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
        mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_LD_DEBK),		
	///////////////////////////////////////////////////
	// Add CHN_DMA_DUMMY here because the DMA engine can't work well when the last entry of
	// DMA command chain is disabled.
	// CHN_DMA_DUMMY + 0
		DONT_CARE,
	// CHN_DMA_DUMMY + 1
		DONT_CARE,
	// CHN_DMA_DUMMY + 2
		DONT_CARE,
	// CHN_DMA_DUMMY + 3
		mDmaIntChainMask1b(FALSE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(FALSE) |			// off the chain
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(0) |
		mDmaID4b(ID_CHN_AWYS)
   
   #else // #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
	
   #endif // #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
  #endif // #ifdef ENABLE_DEBLOCKING
};

#ifndef LINUX
__align(8)
#endif
uint32_t u32dma_rgb_const1[] = {
	///////////////////////////// DMA_TOGGLE 1 //////////////////////////
	// 1
	// CHN_REF_4MV_Y0 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y0 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y0_OFF_0) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y0 + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y0 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),
	// CHN_REF_4MV_Y1 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y1 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y1_OFF_0) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y1
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y1 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),
	// CHN_REF_4MV_Y2 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y2 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y2_OFF_0) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y2 + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y2 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),
	// CHN_REF_4MV_Y3 + 0
		DONT_CARE,
	// CHN_REF_4MV_Y3 + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_Y3_OFF_0) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_4MV_Y3 + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_4MV_Y3 + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b( 4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_4MV),

	// 2
	// CHN_REF_1MV_Y + 0
		DONT_CARE,
	// CHN_REF_1MV_Y + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(3) |								// 3 block/row
		mDmaLocMemAddr14b(REF_Y_OFF_0) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_1MV_Y + 2
		mDmaSysWidth6b(3 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (3 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b(((8 * PIXEL_U) / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_1MV_Y + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (3 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(9 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_1MV),

	// 6
	// CHN_REF_REF_U + 0
		DONT_CARE,
	// CHN_REF_REF_U + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (8 * PIXEL_U) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_U_OFF_0) | 
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_REF_U + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b(((8 * PIXEL_U) / 4) + 1 - (PIXEL_U / 4)),
	// CHN_REF_REF_U + 3
		mDmaLoc3dOff8b((8 * PIXEL_U / 4) + 1 - (2 * PIXEL_U / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_REF_UV),
	// CHN_REF_REF_V + 0
		DONT_CARE,
	// CHN_REF_REF_V + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_V - 1) * (8 * PIXEL_V) / 4) |	// jump to new block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(REF_V_OFF_0) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_REF_REF_V + 2
		mDmaSysWidth6b(2 * SIZE_V / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * SIZE_V / 4) + 1 - (2 * SIZE_V / 4)) |
		mDmaLocWidth4b(PIXEL_V / 4) |
		mDmaLocOff8b(((8 * PIXEL_V) / 4) + 1 - (PIXEL_V / 4)),
	// CHN_REF_REF_V + 3
		mDmaLoc3dOff8b((8 * PIXEL_V / 4) + 1 - (2 * PIXEL_V / 4)) |// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_REF_UV),

	// 3.
	// CHN_IMG_Y + 0
		DONT_CARE,
	// CHN_IMG_Y + 1
		mDmaLoc2dWidth4b(8) |								// 8 lines/block
		mDmaLoc2dOff8b(1 - (PIXEL_U - 1) * (2 * PIXEL_U) / 4) |	// jump to next block
		mDmaLoc3dWidth4b(2) |								// 2 block/row
		mDmaLocMemAddr14b(INTER_Y_OFF_1) |
		mDmaLocInc2b(DMA_INCL_0),
	// CHN_IMG_Y + 2
		mDmaSysWidth6b(2 * SIZE_U / 8) |
		mDmaSysOff14b((DEFAULT_WIDTH * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
		mDmaLocWidth4b(PIXEL_U / 4) |
		mDmaLocOff8b((2 * PIXEL_U / 4) + 1 - (PIXEL_U / 4)),
	// CHN_IMG_Y + 3
		mDmaLoc3dOff8b(1) |						// jump to next row
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_2D) |
		mDmaLType2b(DMA_DATA_4D) |
		mDmaLen12b(4 * SIZE_U / 4) |
		mDmaID4b(ID_CHN_IMG),
	// CHN_IMG_U + 0
		DONT_CARE,
	// CHN_IMG_U + 1
		mDmaLocMemAddr14b(INTER_U_OFF_1),
	// CHN_IMG_U + 2
		DONT_CARE, 								// dont care block width
	// CHN_IMG_U + 3
		mDmaLoc3dOff8b(DONT_CARE) |
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_U / 4) |
		mDmaID4b(ID_CHN_IMG),
	// CHN_IMG_V + 0
		DONT_CARE,
	// CHN_IMG_V + 1
		mDmaLocMemAddr14b(INTER_V_OFF_1),
	// CHN_IMG_V + 2
		DONT_CARE,								// dont care block width
	// CHN_IMG_V + 3
		mDmaLoc3dOff8b(DONT_CARE) |
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(SIZE_V / 4) |
		mDmaID4b(ID_CHN_IMG),
	#if (defined(TWO_P_EXT) || defined(TWO_P_INT))
	///////////////////////////////////////////////////
	// 3-1. CHN_STORE_MBS
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
		mDmaLen12b(sizeof(MACROBLOCK) / 4) |
		mDmaID4b(ID_CHN_STORE_MBS),
	///////////////////////////////////////////////////
	// 3-2. CHN_ACDC_MBS
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
		mDmaLen12b(sizeof(MACROBLOCK)/ 4) |
		mDmaID4b(ID_CHN_ACDC_MBS),
	#endif
	
  #ifdef ENABLE_DEBLOCKING	
///////////////////////////////////////////////////
	// 4
		// CHN_RGB_TOP + 0
			DONT_CARE,
		// CHN_RGB_TOP + 1
			mDmaLocMemAddr14b(BUFFER_RGB_TOP_OFF_0(RGB_PIXEL_SIZE_2)),
		// CHN_RGB_TOP + 2
			mDmaSysWidth6b(RGB_PIXEL_SIZE_2 * PIXEL_Y / 8) |
			mDmaSysOff14b((RGB_PIXEL_SIZE_2 * DEFAULT_STRIDE / 4) + 1 - (RGB_PIXEL_SIZE_2 * PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_RGB_TOP + 3
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((RGB_PIXEL_SIZE_2 * SIZE_Y / 2) / 4) |
			mDmaID4b(ID_CHN_RGB_TOP),
		// CHN_RGB_MID + 0
			DONT_CARE,
		// CHN_RGB_MID + 1
			mDmaLocMemAddr14b(BUFFER_RGB_MID_OFF_0(RGB_PIXEL_SIZE_2)),
		// CHN_RGB_MID + 2
			mDmaSysWidth6b(RGB_PIXEL_SIZE_2 * PIXEL_Y / 8) |
			mDmaSysOff14b((RGB_PIXEL_SIZE_2 * DEFAULT_STRIDE / 4) + 1 - (RGB_PIXEL_SIZE_2 * PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_RGB_MID + 3
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((RGB_PIXEL_SIZE_2 * SIZE_Y / 2) / 4) |
			mDmaID4b(ID_CHN_RGB_MID),
		// CHN_RGB_BOT + 0
			DONT_CARE,
		// CHN_RGB_BOT + 1
			mDmaLocMemAddr14b(BUFFER_RGB_BOT_OFF_0(RGB_PIXEL_SIZE_2)),
		// CHN_RGB_BOT + 2
			mDmaSysWidth6b(RGB_PIXEL_SIZE_2 * PIXEL_Y / 8) |
			mDmaSysOff14b((RGB_PIXEL_SIZE_2 * DEFAULT_STRIDE / 4) + 1 - (RGB_PIXEL_SIZE_2 * PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_RGB_BOT + 3
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b((RGB_PIXEL_SIZE_2 * SIZE_Y / 2) / 4) |
			mDmaID4b(ID_CHN_RGB_BOT),
		
  #else // #ifdef ENABLE_DEBLOCKING
	
	///////////////////////////////////////////////////
	// 4
		// CHN_RGB + 0
			DONT_CARE,
		// CHN_RGB + 1
			mDmaLocMemAddr14b(BUFFER_RGB_OFF_0(RGB_PIXEL_SIZE_2)),		// watch-out
		// CHN_RGB + 2
			mDmaSysWidth6b(RGB_PIXEL_SIZE_2 * PIXEL_Y / 8) |
			mDmaSysOff14b((RGB_PIXEL_SIZE_2 * DEFAULT_STRIDE / 4) + 1 - (RGB_PIXEL_SIZE_2 * PIXEL_Y / 4)) |
			mDmaLocWidth4b(DONT_CARE) |
			mDmaLocOff8b(DONT_CARE),
		// CHN_RGB + 3
			mDmaIntChainMask1b(TRUE) |
			mDmaEn1b(TRUE) |
			mDmaChainEn1b(TRUE) |
			mDmaDir1b(DMA_DIR_2SYS) |
			mDmaSType2b(DMA_DATA_2D) |
			mDmaLType2b(DMA_DATA_SEQUENTAIL) |
			mDmaLen12b(RGB_PIXEL_SIZE_2 * SIZE_Y / 4) |
			mDmaID4b(ID_CHN_RGB),
  #endif // #ifdef ENABLE_DEBLOCKING
  
  #ifdef ENABLE_DEBLOCKING  
   #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
   
   #else
  	///////////////////////////////////////////////////
	// 5
	// CHN_STORE_DEBK + 0
		DONT_CARE,
	// CHN_STORE_DEBK + 1
		mDmaLocMemAddr14b(DEBK_ST_OFF_0),
	// CHN_STORE_DEBK + 2
		DONT_CARE,
	// CHN_STORE_DEBK + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_ST_DEBK),
	///////////////////////////////////////////////////
	// 6
	// CHN_LOAD_DEBK + 0
		DONT_CARE,
	// CHN_LOAD_DEBK + 1
		mDmaLocMemAddr14b(DEBK_LD_OFF_0),
	// CHN_LOAD_DEBK + 2
		DONT_CARE,
	// CHN_LOAD_DEBK + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_LD_DEBK),
   #endif // #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
  #endif // #ifdef ENABLE_DEBLOCKING

	///////////////////////////////////////////////////
	// 5
	// CHN_LOAD_PREDITOR + 0
		DONT_CARE,
	// CHN_LOAD_PREDITOR + 1
		mDmaLocMemAddr14b(PREDICTOR0_OFF),		// watch-out
	// CHN_LOAD_PREDITOR + 2
		mDmaSysWidth6b(DONT_CARE) |
		mDmaSysOff14b(DONT_CARE) |
		mDmaLocWidth4b(4) |
		mDmaLocOff8b(8 + 1 - 4),
	// CHN_LOAD_PREDITOR + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(0x10) |
		mDmaID4b(ID_CHN_ACDC),
	////////////////////////////////////////////////////////////////
	// 6
	// CHN_STORE_PREDITOR + 0
		DONT_CARE,
	// CHN_STORE_PREDITOR + 1
		mDmaLocMemAddr14b(PREDICTOR8_OFF),
	// CHN_STORE_PREDITOR + 2
		mDmaSysWidth6b(DONT_CARE) |
		mDmaSysOff14b(DONT_CARE) |
		mDmaLocWidth4b(4) |
		mDmaLocOff8b(8 + 1 - 4),
	// CHN_STORE_PREDITOR + 3,
		mDmaLoc3dOff8b(DONT_CARE) |
	  #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
	    mDmaIntChainMask1b(TRUE) |
	  #else
		mDmaIntChainMask1b(FALSE) |
	  #endif
		mDmaEn1b(TRUE) |
	  #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
	    mDmaChainEn1b(TRUE) |
	  #else
		mDmaChainEn1b(FALSE) |
	  #endif
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_2D) |
		mDmaLen12b(0x10) |
		mDmaID4b(ID_CHN_ACDC)
		
  #ifdef ENABLE_DEBLOCKING  
   #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK	
       ,
    ///////////////////////////////////////////////////
	// 5
	// CHN_STORE_DEBK + 0
		DONT_CARE,
	// CHN_STORE_DEBK + 1
		mDmaLocMemAddr14b(DEBK_ST_OFF_0),
	// CHN_STORE_DEBK + 2
		DONT_CARE,
	// CHN_STORE_DEBK + 3
		mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2SYS) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_ST_DEBK),
	///////////////////////////////////////////////////
	// 6
	// CHN_LOAD_DEBK + 0
		DONT_CARE,
	// CHN_LOAD_DEBK + 1
		mDmaLocMemAddr14b(DEBK_LD_OFF_0),
	// CHN_LOAD_DEBK + 2
		DONT_CARE,
	// CHN_LOAD_DEBK + 3
        mDmaIntChainMask1b(TRUE) |
		mDmaEn1b(TRUE) |
        mDmaChainEn1b(TRUE) |				// chain to store_preditor
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b((SIZE_U * 3)/4) |			// Y2, Y3, (U, V) /2
		mDmaID4b(ID_CHN_LD_DEBK),
	///////////////////////////////////////////////////
	// Add CHN_DMA_DUMMY here because the DMA engine can't work well when the last entry of
	// DMA command chain is disabled.
	// CHN_DMA_DUMMY + 0
		DONT_CARE,
	// CHN_DMA_DUMMY + 1
		DONT_CARE,
	// CHN_DMA_DUMMY + 2
		DONT_CARE,
	// CHN_DMA_DUMMY + 3
		mDmaIntChainMask1b(FALSE) |
		mDmaEn1b(TRUE) |
		mDmaChainEn1b(FALSE) |          // off the chain
		mDmaDir1b(DMA_DIR_2LOCAL) |
		mDmaSType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLType2b(DMA_DATA_SEQUENTAIL) |
		mDmaLen12b(0) |
		mDmaID4b(ID_CHN_AWYS)
   #else
  	
   #endif // #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
  #endif // #ifdef ENABLE_DEBLOCKING

};
void 
dma_dec_commandq_init(DECODER_ext * dec)
{
	uint32_t * dma_cmd_tgl;
    if ( dec->output_fmt == OUTPUT_FMT_YUV) {
		dma_cmd_tgl = (uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_0);
		memcpy (dma_cmd_tgl, u32dma_yuv_const0, sizeof (u32dma_yuv_const0));
		dma_cmd_tgl = (uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_1);
		memcpy (dma_cmd_tgl, u32dma_yuv_const1, sizeof (u32dma_yuv_const1));
    } else {
		dma_cmd_tgl = (uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_0);
		memcpy (dma_cmd_tgl, u32dma_rgb_const0, sizeof (u32dma_rgb_const0));
		dma_cmd_tgl = (uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_1);
		memcpy (dma_cmd_tgl, u32dma_rgb_const1, sizeof (u32dma_rgb_const1));
    }
}
void 
dma_dec_commandq_init_each(DECODER_ext * dec)
{
	uint32_t * dma_cmd_tgl;
	int i;

         int32_t rgb_pixel_size;

	if (dec->output_fmt == OUTPUT_FMT_YUV)
		rgb_pixel_size = RGB_PIXEL_SIZE_1;
	else if ( dec->output_fmt == OUTPUT_FMT_RGB888)
		rgb_pixel_size = RGB_PIXEL_SIZE_4;
	else
		rgb_pixel_size = RGB_PIXEL_SIZE_2;
	
	for (i = 0; i < 2; i ++) {
		if (i == 0)
			dma_cmd_tgl = (uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_0);
		else
			dma_cmd_tgl = (uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_1);
    #ifdef ENABLE_DEBLOCKING
   
	  if ( dec->output_fmt == OUTPUT_FMT_YUV) {
		  dma_cmd_tgl[CHN_YUV_TOP_Y + 2] =
		  dma_cmd_tgl[CHN_YUV_MID_Y + 2] =
		  dma_cmd_tgl[CHN_YUV_BOT_Y + 2] =
			  mDmaSysWidth6b(PIXEL_Y / 8) |
			  mDmaSysOff14b((dec->output_stride / 4) + 1 - (PIXEL_Y / 4)) |
			  mDmaLocWidth4b(DONT_CARE) |
			  mDmaLocOff8b(DONT_CARE);
		  dma_cmd_tgl[CHN_YUV_TOP_U + 2] =
		  dma_cmd_tgl[CHN_YUV_TOP_V + 2] =
		  dma_cmd_tgl[CHN_YUV_MID_U + 2] =
		  dma_cmd_tgl[CHN_YUV_MID_V + 2] =
		  dma_cmd_tgl[CHN_YUV_BOT_U + 2] =
		  dma_cmd_tgl[CHN_YUV_BOT_V + 2] =
			  mDmaSysWidth6b(PIXEL_U / 8) |
			  mDmaSysOff14b(((dec->output_stride /2) / 4) + 1 - (PIXEL_U / 4)) |
			  mDmaLocWidth4b(DONT_CARE) |
			  mDmaLocOff8b(DONT_CARE);	
	  } else {
		  dma_cmd_tgl[CHN_RGB_TOP + 2] =
		  dma_cmd_tgl[CHN_RGB_MID + 2] =
		  dma_cmd_tgl[CHN_RGB_BOT + 2] =
			  mDmaSysWidth6b(rgb_pixel_size * PIXEL_Y / 8) |
			  mDmaSysOff14b((rgb_pixel_size * dec->output_stride / 4) + 1 - (rgb_pixel_size * PIXEL_Y / 4)) |
			  mDmaLocWidth4b(DONT_CARE) |
			  mDmaLocOff8b(DONT_CARE);
	  }
	  
    #else // #ifdef ENABLE_DEBLOCKING

	if ( dec->output_fmt == OUTPUT_FMT_YUV) {
		  #ifdef ENABLE_DEINTERLACE
                    if(dec->u32EnableDeinterlace) {
		        dma_cmd_tgl[CHN_YUV_Y + 2] =
				mDmaSysWidth6b(PIXEL_Y / 8) |
				mDmaSysOff14b(((dec->output_stride<<1) / 4) + 1 - (PIXEL_Y / 4)) |
				mDmaLocWidth4b(DONT_CARE) |
				mDmaLocOff8b(DONT_CARE);
			dma_cmd_tgl[CHN_YUV_U + 2] =
			dma_cmd_tgl[CHN_YUV_V + 2] =
				mDmaSysWidth6b(PIXEL_U / 8) |
				mDmaSysOff14b((((dec->output_stride<<1) /2) / 4) + 1 - (PIXEL_U / 4)) |
				mDmaLocWidth4b(DONT_CARE) |
				mDmaLocOff8b(DONT_CARE);
                    } else {
                        dma_cmd_tgl[CHN_YUV_Y + 2] =
				mDmaSysWidth6b(PIXEL_Y / 8) |
				mDmaSysOff14b((dec->output_stride / 4) + 1 - (PIXEL_Y / 4)) |
				mDmaLocWidth4b(DONT_CARE) |
				mDmaLocOff8b(DONT_CARE);
			dma_cmd_tgl[CHN_YUV_U + 2] =
			dma_cmd_tgl[CHN_YUV_V + 2] =
				mDmaSysWidth6b(PIXEL_U / 8) |
				mDmaSysOff14b(((dec->output_stride /2) / 4) + 1 - (PIXEL_U / 4)) |
				mDmaLocWidth4b(DONT_CARE) |
				mDmaLocOff8b(DONT_CARE);

                    }

		  #else
			dma_cmd_tgl[CHN_YUV_Y + 2] =
				mDmaSysWidth6b(PIXEL_Y / 8) |
				mDmaSysOff14b((dec->output_stride / 4) + 1 - (PIXEL_Y / 4)) |
				mDmaLocWidth4b(DONT_CARE) |
				mDmaLocOff8b(DONT_CARE);
			dma_cmd_tgl[CHN_YUV_U + 2] =
			dma_cmd_tgl[CHN_YUV_V + 2] =
				mDmaSysWidth6b(PIXEL_U / 8) |
				mDmaSysOff14b(((dec->output_stride /2) / 4) + 1 - (PIXEL_U / 4)) |
				mDmaLocWidth4b(DONT_CARE) |
				mDmaLocOff8b(DONT_CARE);
                  #endif
	} else {
			dma_cmd_tgl[CHN_RGB + 2] =
				mDmaSysWidth6b(rgb_pixel_size * PIXEL_Y / 8) |
				mDmaSysOff14b((rgb_pixel_size * dec->output_stride / 4) + 1 - (rgb_pixel_size * PIXEL_Y / 4)) |
				mDmaLocWidth4b(DONT_CARE) |
				mDmaLocOff8b(DONT_CARE);
         }
    #endif // #ifdef ENABLE_DEBLOCKING
    
		dma_cmd_tgl[CHN_REF_4MV_Y0 + 2] = 
		dma_cmd_tgl[CHN_REF_4MV_Y1 + 2] = 
		dma_cmd_tgl[CHN_REF_4MV_Y2 + 2] = 
		dma_cmd_tgl[CHN_REF_4MV_Y3 + 2] = 
			mDmaSysWidth6b(2 * SIZE_U / 8) |
			mDmaSysOff14b((dec->mb_width * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
			mDmaLocWidth4b(PIXEL_U / 4) |
			mDmaLocOff8b((8 * PIXEL_U / 4) + 1 - (PIXEL_U / 4));
		dma_cmd_tgl[CHN_REF_1MV_Y + 2] =
			mDmaSysWidth6b(3 * SIZE_U / 8) |
			mDmaSysOff14b((dec->mb_width * 2 * SIZE_U / 4) + 1 - (3 * SIZE_U / 4)) |
			mDmaLocWidth4b(PIXEL_U / 4) |
			mDmaLocOff8b(((8 * PIXEL_U) / 4) + 1 - (PIXEL_U / 4));
		dma_cmd_tgl[CHN_REF_U + 2] = 
		dma_cmd_tgl[CHN_REF_V + 2] = 
			mDmaSysWidth6b(2 * SIZE_U / 8) |
			mDmaSysOff14b((dec->mb_width * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
			mDmaLocWidth4b(PIXEL_U / 4) |
			mDmaLocOff8b(((8 * PIXEL_U) / 4) + 1 - (PIXEL_U / 4));

		dma_cmd_tgl[CHN_IMG_Y + 2] =
			mDmaSysWidth6b(2 * SIZE_U / 8) |
			mDmaSysOff14b((dec->mb_width * 2 * SIZE_U / 4) + 1 - (2 * SIZE_U / 4)) |
			mDmaLocWidth4b(PIXEL_U / 4) |
			mDmaLocOff8b((2 * PIXEL_U / 4) + 1 - (PIXEL_U / 4));
	}
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

 #if ( defined(ONE_P_EXT) || defined(TWO_P_INT) )
 void 
dma_mvRefNotCoded(DECODER * dec,
				const MACROBLOCK_b * mbb)

{
	// ref->toggle is opposite to vld->toggle
	uint32_t * dma_cmd_tgl = mbb->toggle ?
				(uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_0):
				(uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_1);
	// system memory start
	// ref y
	dma_cmd_tgl[CHN_REF_1MV_Y] =
		(uint32_t) ((int)dec->refn.y_phy + mbb->x * 2 * SIZE_U + (int)dec->mb_width * mbb->y * SIZE_Y);
	// ref u
	dma_cmd_tgl[CHN_REF_U] =
		(uint32_t) ((int)dec->refn.u_phy + mbb->mbpos * SIZE_U);
	// ref v
	dma_cmd_tgl[CHN_REF_V] =
		(uint32_t) ((int)dec->refn.v_phy + mbb->mbpos * SIZE_V);
}

void 
dma_mvRef1MV(DECODER * dec,
				const MACROBLOCK *mb,
				const MACROBLOCK_b *mbb)
{
	// ref->toggle is opposite to vld->toggle
	uint32_t * dma_cmd_tgl = mbb->toggle ?
				(uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_0):
				(uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_1);
	int refoffset;
	int refx;
	int refy;
	/////////////////////////////////////////////////////////
	// Y
	refx = mbb->x * PIXEL_Y * 2 + (int32_t)mb->mvs[0].vec.s16x;			// unit: 0.5 pixel
	refy = mbb->y * PIXEL_Y * 2 + (int32_t)mb->mvs[0].vec.s16y; 		// unit: 0.5 pixel

	if (refx < 0) {
		// out boundary, force refx = 0
		refx = 0;
	} else if (refx >= ((int)dec->width * 2)) {
		// out boundary, force refx = dec->width * 2 - 16
		refx = (int)dec->width * 2 - PIXEL_U * 2;
	}

	if (refy < 0) {
		// out boundary, force refy = 0
		refy = 0;
	} else if (refy >= ((int)dec->height * 2)) {
		// out boundary, force refy = dec->height * 2 - 16
		 refy = (int)dec->height * 2 - PIXEL_U * 2;
	}

	// byte offset = block position * SIZE_U
	refoffset = ((refx >> 4) + ((int)dec->mb_width * 2) * (refy >> 4)) *SIZE_U;
	// system memory start addr.
	dma_cmd_tgl[CHN_REF_1MV_Y] = (uint32_t) ((int)dec->refn.y_phy + refoffset);

	/////////////////////////////////////////////////////////
	// UV
	refx = mbb->x * PIXEL_U * 2 + (int32_t)mb->MVuv.vec.s16x;		// unit: 0.5 pixel
	refy = mbb->y * PIXEL_U * 2 + (int32_t)mb->MVuv.vec.s16y; 		// unit: 0.5 pixel

	if (refx < 0) {
		// out boundary, force refx = 0
		refx = 0;
	} else if (refx >= (int)dec->width) {
		// out boundary, force refy = dec->width - 16
		refx = (int)dec->width - PIXEL_U * 2;
	}

	if (refy < 0) {
		// out boundary, force refy = 0
		refy = 0;
	} else if (refy >= (int)dec->height) {
		// out boundary, force refy = dec->height - 16
		 refy = (int)dec->height - PIXEL_U * 2;
	}

	// byte offset = block position * SIZE_U
	refoffset = ((refx >> 4) + (int)dec->mb_width  * (refy >> 4)) * SIZE_U;
	// system memory start addr.
	dma_cmd_tgl[CHN_REF_U] = (uint32_t) ((int)dec->refn.u_phy + refoffset);
	dma_cmd_tgl[CHN_REF_V] = (uint32_t) ((int)dec->refn.v_phy + refoffset);
}



void 
dma_mvRef4MV(DECODER * dec,
				const MACROBLOCK *mb,
				const MACROBLOCK_b *mbb)
{
	// ref->toggle is opposite to vld->toggle
	uint32_t * dma_cmd_tgl = mbb->toggle ?
				(uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_0):
				(uint32_t *)(Mp4Base (dec) + DMA_DEC_CMD_OFF_1);
	int refoffset;
	int refx;
	int refy;
	int i;

	/////////////////////////////////////////////////////////
	// Y0 ~ Y3
 	for (i = 0; i < 4; i ++) {
		refx = (2 * mbb->x + (i &1)) * PIXEL_U * 2
			+ (int32_t)mb->mvs[i].vec.s16x; 		// unit: 0.5 pixel
		refy = (2 * mbb->y + ((i >> 1)&1)) * PIXEL_U * 2
			+ (int32_t)mb->mvs[i].vec.s16y; 		// unit: 0.5 pixel

		if (refx < 0) {
			// out boundary, force refx = 0
			refx = 0;
 		} else if (refx >= ((int)dec->width * 2)) {
			// out boundary, force refx = dec->width * 2 - 16
			refx = (int)dec->width * 2 - PIXEL_U * 2;
 		}

		if (refy < 0) {
			// out boundary, force refy = 0
			refy = 0;
		} else if (refy >= ((int)dec->height * 2)) {
			// out boundary, force refy = dec->height * 2 - 16
			 refy = (int)dec->height * 2 - PIXEL_U * 2;
		}

		// byte offset = block position * SIZE_U
		refoffset = ((refx >> 4) + ((int)dec->mb_width * 2) * (refy >> 4)) *SIZE_U;
		// system memory start addr.
		dma_cmd_tgl[CHN_REF_4MV_Y0 + 4 * i] =
				(uint32_t) ((int)dec->refn.y_phy + refoffset);
	}
	/////////////////////////////////////////////////////////
	// UV
	refx = mbb->x * PIXEL_U * 2 + (int32_t)mb->MVuv.vec.s16x;		// unit: 0.5 pixel
	refy = mbb->y * PIXEL_U * 2 + (int32_t)mb->MVuv.vec.s16y; 		// unit: 0.5 pixel
	if (refx < 0) {
		// out boundary, force refx = 0
		refx = 0;
	} else if (refx >= (int)dec->width) {
		// out boundary, force refy = dec->width - 16
		refx = (int)dec->width - PIXEL_U * 2;
	}

	if (refy < 0) {
		// out boundary, force refy = 0
		refy = 0;
	} else if (refy >= (int)dec->height) {
		// out boundary, force refy = dec->height - 16
		 refy = (int)dec->height - PIXEL_U * 2;
	}

	// byte offset = block position * SIZE_U
	refoffset = ((refx >> 4) + (int)dec->mb_width  * (refy >> 4)) * SIZE_U;
	// system memory start addr.
	dma_cmd_tgl[CHN_REF_U] =
				(uint32_t) ((int)dec->refn.u_phy + refoffset);
	dma_cmd_tgl[CHN_REF_V] =
				(uint32_t) ((int)dec->refn.v_phy + refoffset);
}
#endif
