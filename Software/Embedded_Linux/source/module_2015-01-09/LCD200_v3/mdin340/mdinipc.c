//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2008  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:	MDINIPC.C
// Description 		:
// Ref. Docment		: 
// Revision History 	:

// ----------------------------------------------------------------------
// Include files
// ----------------------------------------------------------------------
#include	<linux/string.h>
#include	"mdin3xx.h"

#if __MDIN3xx_DBGPRT__ == 1
#include	"..\common\ticortex.h"	// for UARTprintf
#endif

// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
static BYTE enInterWND = 0;

// ----------------------------------------------------------------------
// Static Global Data section variables
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// External Variable 
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static Prototype Functions
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static functions
// ----------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetIPCBlock(void)
{
	return MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x200, (PBYTE)MDIN_Deinter_Default, 256);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetDeintMode(PMDIN_VIDEO_INFO pINFO, MDIN_DEINT_MODE_t mode)
{
	PMDIN_DEINTCTL_INFO pIPC = (PMDIN_DEINTCTL_INFO)&pINFO->stIPC_m;

//	pIPC->mode = (pIPC->attb&MDIN_DEINT_IPC_PROC)? mode : MDIN_DEINT_NONSTILL;
	pIPC->mode = mode; if ((pIPC->attb&MDIN_DEINT_IPC_PROC)==0) mode = MDIN_DEINT_NONSTILL;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x210, 12, 2, mode)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetDeintFilm(PMDIN_VIDEO_INFO pINFO, MDIN_DEINT_FILM_t mode)
{
	PMDIN_DEINTCTL_INFO pIPC = (PMDIN_DEINTCTL_INFO)&pINFO->stIPC_m;

//	pIPC->film = (pIPC->attb&MDIN_DEINT_IPC_PROC)? mode : MDIN_DEINT_FILM_OFF;
	pIPC->film = mode; if ((pIPC->attb&MDIN_DEINT_IPC_PROC)==0) mode = MDIN_DEINT_FILM_OFF;
	if (pINFO->dacPATH==DAC_PATH_AUX_4CH) mode = MDIN_DEINT_FILM_OFF;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x250, 12, 3, mode)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_ScaleDownProgNR(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	PMDIN_DEINTCTL_INFO pIPC = (PMDIN_DEINTCTL_INFO)&pINFO->stIPC_m;
	WORD nID = (pSRC->stATTB.attb&MDIN_USE_INPORT_A)? 1 : 0;

	if (OnOff==0||(pIPC->attb&MDIN_DEINT_IPC_PROC)) return MDIN_NO_ERROR;
	if ((pIPC->attb&MDIN_DEINT_DN_SCALE)==0) return MDIN_NO_ERROR;

	// nr_t_en, deinterlace_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x205, 0, 1, 1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x205, 4, 1, 1)) return MDIN_I2C_ERROR;

	// fieldid_bypass, progressive
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x001, 2+nID,   1, 1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x002, 8-nID*8, 1, 0)) return MDIN_I2C_ERROR;

	// in_test_ptrn
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x042, 0x0084)) return MDIN_I2C_ERROR;

	MDINDLY_mSec(100);		// delay 100ms

	// restore deinterlace_en, progressive, in_test_ptrn
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x205, 4, 1, 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x002, 8-nID*8, 1, 1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x042, 0x0000)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetDeintUltraNR(BOOL OnOff)
{
	WORD rVal;

	if (MDINHIF_RegRead(MDIN_LOCAL_ID, 0x260, &rVal)) return MDIN_I2C_ERROR;
	if ((rVal&0x0600)!=0x0600) OnOff = OFF;		// if 3DNR is off, then OFF
	
	// nr_n_deint_thres
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x264,  8, 8, (OnOff)? 255 : 20)) return MDIN_I2C_ERROR;

	// nr_deint_thres
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x264,  0, 8, (OnOff)? 255 : 18)) return MDIN_I2C_ERROR;

	// mo_nr_th
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x265,  4, 3, (OnOff)?   7 :  4)) return MDIN_I2C_ERROR;

	// n_mo_nr_th
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x265,  0, 3, (OnOff)?   7 :  4)) return MDIN_I2C_ERROR;

	// fast_history_nr
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x266,  0, 4, (OnOff)?   0 :  8)) return MDIN_I2C_ERROR;

	// nr_inter_diff_or_en, nr_avg_inter_diff_or_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x267, 14, 2, (OnOff)?   3 :  0)) return MDIN_I2C_ERROR;

	// nr_inter_diff_thres
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x267,  8, 6, (OnOff)?   0 : 63)) return MDIN_I2C_ERROR;

	// avg_nr_diff_thres
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x267,  0, 8, (OnOff)?   0 : 16)) return MDIN_I2C_ERROR;

	// inter_diff_exp_tab
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x268,  4, 2, (OnOff)?   3 :  2)) return MDIN_I2C_ERROR;

	// nr_avg_inter_diff_exp_tab
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x268,  0, 2, (OnOff)?   3 :  2)) return MDIN_I2C_ERROR;

	// y_surface_for_nr_surface_hor_th
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x23b,  8, 8, (OnOff)?   0 :  3)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableDeint3DNR(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_DEINTCTL_INFO pIPC = (PMDIN_DEINTCTL_INFO)&pINFO->stIPC_m;
	BYTE mode = (pIPC->fine&MDIN_DEINT_FRC_DOWN)? 1 : 0;

	if (OnOff)	pIPC->fine |=  MDIN_DEINT_3DNR_ON;
	else		pIPC->fine &= ~MDIN_DEINT_3DNR_ON;

	// if ycbcr444_en==1 and progressive input, 3dnr_en = off
	if ((pIPC->attb&MDIN_DEINT_444_PROC)&&(pIPC->attb&MDIN_DEINT_IPC_PROC)==0) OnOff = OFF;

	// if 1080p input and dual output, 3dnr_en = off
	if (pIPC->attb&MDIN_DEINT_HD_1080p&&pINFO->dacPATH==DAC_PATH_AUX_OUT) OnOff = OFF;

	// for 2-HD input mode
	if (pINFO->dacPATH==DAC_PATH_AUX_2HD) OnOff = OFF;

	// if vertical scale ratio<0.5, 3dnr_en = off
	if (pIPC->attb&MDIN_DEINT_p5_SCALE) OnOff = OFF;

	// if revision=0 and vertical scale ratio<1.0 and progressive input, 3dnr_en = off
	if ((mdinREV==0)&&
		(pIPC->attb&MDIN_DEINT_DN_SCALE)&&(pIPC->attb&MDIN_DEINT_IPC_PROC)==0) OnOff = OFF;

	// if 1080p input, 3dnr_en = off
#if !defined(SYSTEM_USE_MCLK202)
	if (pIPC->attb&MDIN_DEINT_HD_1080p) OnOff = OFF;
#endif

	// common_mode_at_IPC - nr_t_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x205,  0, 1, (OnOff)? mode : 0)) return MDIN_I2C_ERROR;

	// nr_mode_reg - nr_en, nr_write_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x260, 10, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x260, 12, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;

	// nr_motion_set_reg - nr_mo_sel, nr_all_use_mo
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x261, 6, 2, (OnOff)? 2 : 3)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x261, 0, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;

	// nr_motion_offset_reg - cu_motion_use, n_motion_use, slow_motion_use, still_motion_use
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x262, 15, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x262, 14, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x262, 13, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x262, 12, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;

	// mo_history_for_nr_condition - nr_mo_his_update_use_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x266, 11, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;

	// border_ctrl_reg1 - no_his_border_adp_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x240, 13, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;

	// set progressive NR scale down
	if (MDIN3xx_ScaleDownProgNR(pINFO, OnOff)) return MDIN_NO_ERROR;

	// if 3dnr_en=off & progressive input, mfc_prepend_load = OFF
	OnOff = ((OnOff==OFF)&&((pIPC->attb&MDIN_DEINT_IPC_PROC)==0))? OFF : ON;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x100,  6, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;

	// set ultraNR => if 3dnr_en=on & 3dnr_gain>50, ultraNR = ON
	return MDIN3xx_SetDeintUltraNR((pIPC->gain>50)? ON : OFF);
}

//--------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetDeint3DNRGain(PMDIN_VIDEO_INFO pINFO, BYTE gain)
{
	BYTE level, max = 60; 
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	PMDIN_DEINTCTL_INFO pIPC = (PMDIN_DEINTCTL_INFO)&pINFO->stIPC_m;

	// if src is progressive && 8-bit precision then max is 33 or 50
	if ((pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)&&(pSRC->stATTB.attb&MDIN_PRECISION_8))
		 level = (mdinREV==0)? 33 : 50;	// if revision=0 then 33, others 50.
	else level = 60;

	pIPC->gain = MIN(gain,level);

	// NR factor level, NR factor not_still_1st
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x260, 0, 6, max-pIPC->gain)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x26a, 0, 6, max-pIPC->gain)) return MDIN_I2C_ERROR;

	// set ultraNR => if 3dnr_en=on & 3dnr_gain>50, ultraNR = ON
	return MDIN3xx_SetDeintUltraNR((pIPC->gain>50)? ON : OFF);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableDeintCCS(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_DEINTCTL_INFO pIPC = (PMDIN_DEINTCTL_INFO)&pINFO->stIPC_m;
	BOOL pal = (pIPC->attb&MDIN_DEINT_PAL_CCS)? 1 : 0;

	if (OnOff)	pIPC->fine |=  MDIN_DEINT_CCS_ON;
	else		pIPC->fine &= ~MDIN_DEINT_CCS_ON;

	// if progressive input, ccs_en = off
	if ((pIPC->attb&MDIN_DEINT_IPC_PROC)==0) OnOff = OFF;

	// if 1080i input, ccs_en = off
	if (pIPC->attb&MDIN_DEINT_HD_1080i) OnOff = OFF;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x248, 14, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x205, 5, 1, MBIT(OnOff,pal))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableDeintCUE(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_DEINTCTL_INFO pIPC = (PMDIN_DEINTCTL_INFO)&pINFO->stIPC_m;
	WORD rVal;

	if (OnOff)	pIPC->fine |=  MDIN_DEINT_CUE_ON;
	else		pIPC->fine &= ~MDIN_DEINT_CUE_ON;

	// if ycbcr444_en==1 and progressive input, cue_en = off
	if ((pIPC->attb&MDIN_DEINT_444_PROC)&&(pIPC->attb&MDIN_DEINT_IPC_PROC)==0) OnOff = OFF;

	// if pal_ccs_en==1, cue_en = off
	if (MDINHIF_RegRead(MDIN_LOCAL_ID, 0x205, &rVal)) return MDIN_I2C_ERROR;
	if (rVal&0x0020) OnOff = OFF;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x208, 5, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetDeintInterWND(PMDIN_INTER_WINDOW pWND, MDIN_INTER_BLOCK_t nID)
{
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x278, (WORD)pWND->lx)) return MDIN_I2C_ERROR;	// lx
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x279, nID*4+0x80)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x278, (WORD)pWND->ly)) return MDIN_I2C_ERROR;	// ly
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x279, nID*4+0x81)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x278, (WORD)pWND->rx)) return MDIN_I2C_ERROR;	// rx
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x279, nID*4+0x82)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x278, (WORD)pWND->ry)) return MDIN_I2C_ERROR;	// ry
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x279, nID*4+0x83)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableDeintInterWND(MDIN_INTER_BLOCK_t nID, BOOL OnOff)
{
	if (OnOff)	enInterWND |=  (1<<nID);
	else		enInterWND &= ~(1<<nID);

	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x278, enInterWND)) return MDIN_I2C_ERROR;		// OnOff
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x279, 0x80|0x20)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_DisplayDeintInterWND(BOOL OnOff)
{
	WORD mode = (OnOff)? 0x0612 : 0x0000;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x276, mode)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

/*  FILE_END_HERE */
