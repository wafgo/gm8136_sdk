//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2008  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:	MDINAUX.C
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

// ----------------------------------------------------------------------
// Static Global Data section variables
// ----------------------------------------------------------------------
static WORD xpll_S = 0, xpll_F = 0, xpll_T = 0;

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
MDIN_ERROR_t MDINAUX_SetVideoPLL(WORD S, WORD F, WORD T)
{
//	division ratio = (S+F/T) or (S+1-F/T)
	if (xpll_S==S&&xpll_F==F&&xpll_T==T) return MDIN_NO_ERROR;

	if (MDINHIF_RegField(MDIN_HOST_ID, 0x035, 0,  8, T)) return MDIN_I2C_ERROR;	// T parameter
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x036, 0, 13, F)) return MDIN_I2C_ERROR;	// F parameter
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x042, 10, 6, S)) return MDIN_I2C_ERROR;	// S parameter

	xpll_S = S; xpll_F = F; xpll_T = T;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINAUX_SetSrcVideoFrmt(PMDIN_VIDEO_INFO pINFO)
{
	WORD mode, nID;
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_x;

	// set InPort
	pSRC->stATTB.attb &= ~(MDIN_USE_INPORT_M|MDIN_USE_INPORT_A);
	switch (pINFO->srcPATH) {
		case PATH_MAIN_A_AUX_A: //case PATH_MAIN_B_AUX_A:
			memcpy((PBYTE)&pSRC->stATTB, (PBYTE)&pINFO->stSRC_a.stATTB, sizeof(MDIN_SRCVIDEO_ATTB));
			pSRC->frmt = pINFO->stSRC_a.frmt; pSRC->mode = pINFO->stSRC_a.mode;
			pSRC->stATTB.attb |=  MDIN_USE_INPORT_A; mode = nID = 2; break;

		case PATH_MAIN_A_AUX_B: case PATH_MAIN_B_AUX_B:
			memcpy((PBYTE)&pSRC->stATTB, (PBYTE)&pINFO->stSRC_b.stATTB, sizeof(MDIN_SRCVIDEO_ATTB));
			pSRC->frmt = pINFO->stSRC_b.frmt; pSRC->mode = pINFO->stSRC_b.mode;
			pSRC->stATTB.attb |=  MDIN_USE_INPORT_B; mode = nID = 3; break;

		default:
			memcpy((PBYTE)&pSRC->stATTB, (PBYTE)&pINFO->stOUT_m.stATTB, sizeof(MDIN_OUTVIDEO_ATTB));
			pSRC->frmt = pINFO->stOUT_m.frmt; pSRC->mode = pINFO->stOUT_m.mode;
			pSRC->stATTB.attb |=  MDIN_USE_INPORT_M; mode = nID = 0; break;
	}

	mode |= (nID)? 0 : ((MBIT(pSRC->stATTB.attb,MDIN_NEGATIVE_HACT))?	(1<<7) : 0);
	mode |= (nID)? 0 : ((MBIT(pSRC->stATTB.attb,MDIN_NEGATIVE_VACT))?	(1<<6) : 0);
	mode |= ((MBIT(pSRC->fine,MDIN_HIGH_IS_TOPFLD))?					(1<<5) : 0);
	mode |= ((MBIT(pSRC->fine,MDIN_CbCrSWAP_ON))?						(1<<4) : 0);
	mode |= ((MBIT(pSRC->stATTB.attb,MDIN_PIXELSIZE_422))?				(1<<3) : 0);
	mode |= ((MBIT(pSRC->stATTB.attb,MDIN_SCANTYPE_PROG))?				(1<<2) : 0);

	// aux_input_ctrl
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x140, mode)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINAUX_SetOutVideoFrmt(PMDIN_VIDEO_INFO pINFO)
{
	WORD mode = 0;
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_x;

	if (pOUT->frmt>=VIDOUT_FORMAT_END) return MDIN_INVALID_FORMAT;

	// set attb[H/V-Polarity,ScanType], H/V size
	memcpy(&pOUT->stATTB, (PBYTE)&defMDINOutVideo[pOUT->frmt], sizeof(MDIN_OUTVIDEO_ATTB));

	switch (pINFO->dacPATH) {
		case DAC_PATH_MAIN_PIP:		// get frmt, mode and stATTB of main outvideo
			memcpy(&pOUT->stATTB, (PBYTE)&pINFO->stOUT_m.stATTB, sizeof(MDIN_OUTVIDEO_ATTB));
			pOUT->frmt = pINFO->stOUT_m.frmt; pOUT->mode = pINFO->stOUT_m.mode; break;
	}

	// set Quality, System
	pOUT->stATTB.attb &= ~(MDIN_QUALITY_HD|MDIN_VIDEO_SYSTEM);
	switch (pOUT->frmt) {
		case VIDOUT_720x480i60:		case VIDOUT_720x576i50:
		case VIDOUT_720x480p60:		case VIDOUT_720x576p50:
			pOUT->stATTB.attb |= (MDIN_QUALITY_SD|MDIN_VIDEO_SYSTEM); break;
		case VIDOUT_1280x720p60:	case VIDOUT_1280x720p50:
			pOUT->stATTB.attb |= (MDIN_QUALITY_HD|MDIN_VIDEO_SYSTEM); break;
		case VIDOUT_1920x1080i60:	case VIDOUT_1920x1080i50:
		case VIDOUT_1920x1080p60:	case VIDOUT_1920x1080p50:
			pOUT->stATTB.attb |= (MDIN_QUALITY_HD|MDIN_VIDEO_SYSTEM); break;
/*
	#if	defined(SYSTEM_USE_MDIN325)||defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380)
		case VIDOUT_CVIDEO_NTSC:	case VIDOUT_CVIDEO_PAL:
			pOUT->stATTB.attb |= (MDIN_QUALITY_SD|MDIN_VIDEO_SYSTEM); break;
	#endif
*/
		default:
			pOUT->stATTB.attb |= (MDIN_QUALITY_SD|MDIN_PCVGA_SYSTEM); break;
	}

	// set Precision, PixelSize, ColorSpace
	pOUT->stATTB.attb &= ~(MDIN_PRECISION_8|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV);
	switch (pOUT->mode) {
		case MDIN_OUT_RGB444_8:		mode = (0<<11)|(0<<7);
			pOUT->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_444|MDIN_COLORSPACE_RGB); break;
		case MDIN_OUT_YUV444_8:		mode = (1<<11)|(0<<7);
			pOUT->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_444|MDIN_COLORSPACE_YUV); break;
		case MDIN_OUT_EMB422_8:		mode = (1<<11)|(2<<7);
			pOUT->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_OUT_SEP422_8:		mode = (1<<11)|(0<<7);
			pOUT->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_OUT_EMB422_10:	mode = (1<<11)|(2<<7);
			pOUT->stATTB.attb |= (MDIN_PRECISION_10|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_OUT_SEP422_10:	mode = (1<<11)|(0<<7);
			pOUT->stATTB.attb |= (MDIN_PRECISION_10|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_OUT_MUX656_8:		mode = (1<<11)|(3<<7);
			pOUT->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_OUT_MUX656_10:	mode = (1<<11)|(3<<7);
			pOUT->stATTB.attb |= (MDIN_PRECISION_10|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
	}
/*
#if	defined(SYSTEM_USE_MDIN325)||defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380)
	if (pOUT->frmt==VIDOUT_CVIDEO_NTSC||pOUT->frmt==VIDOUT_CVIDEO_PAL) {
			mode = (1<<11)|(3<<7);		// same MDIN_OUT_MUX656_8
			pOUT->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV);
	}
#endif
*/
	// for 4-CH input mode, 2-HD input mode
	if (pINFO->dacPATH==DAC_PATH_AUX_4CH||pINFO->dacPATH==DAC_PATH_AUX_2HD) mode |= (1<<8);

	// aux_out_ctrl
	mode |= (RBIT(pOUT->stATTB.attb,MDIN_PIXELSIZE_422))?	(1<<15) : 0;
	mode |= (MBIT(pOUT->stATTB.attb,MDIN_POSITIVE_HSYNC))?	(1<<14) : 0;
	mode |= (MBIT(pOUT->stATTB.attb,MDIN_POSITIVE_VSYNC))?	(1<<13) : 0;
	mode |= (MBIT(pOUT->fine,MDIN_HSYNC_IS_DE))?			(1<<12) : 0;
	mode |= (MBIT(pOUT->fine,MDIN_CbCrSWAP_ON))?			(1<< 6) : 0;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x145, mode)) return MDIN_I2C_ERROR;

	// aux_sync_level
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x146, 0x0000)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINAUX_SetSrcVideoCSC(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_x;
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_x;
	WORD i; MDIN_CSCCTRL_INFO stCSC, *pCSC;

#if SOURCE_CSC_STD_RANGE == 1
	if (pSRC->stATTB.attb&MDIN_COLORSPACE_YUV) {
		if (pOUT->stATTB.attb&MDIN_COLORSPACE_YUV)
			 pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscBypass_StdRange;	// YUV(HD or SD) to YUV(HD or SD)
		else pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscBypass_StdRange;	// YUV(HD or SD) to RGB(HD or SD)
	}		
	else if (pSRC->stATTB.attb&MDIN_QUALITY_HD) {
		if (pOUT->stATTB.attb&MDIN_COLORSPACE_YUV)
			 pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscRGBtoYUV_HD_StdRange;	// RGB(HD) to YUV(HD or SD)
		else pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscRGBtoYUV_HD_StdRange;	// RGB(HD) to RGB(HD or SD)
	}
	else {
		if (pOUT->stATTB.attb&MDIN_COLORSPACE_YUV)
			 pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscRGBtoYUV_SD_StdRange;	// RGB(SD) to YUV(HD or SD)
		else pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscRGBtoYUV_SD_StdRange;	// RGB(SD) to RGB(HD or SD)
	}
#else	/* SOURCE_CSC_STD_RANGE == 0 */
	if (pSRC->stATTB.attb&MDIN_COLORSPACE_YUV) {
		if (pOUT->stATTB.attb&MDIN_COLORSPACE_YUV)
			 pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscBypass_StdRange;	// YUV(HD or SD) to YUV(HD or SD)
		else pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscBypass_StdRange;	// YUV(HD or SD) to RGB(HD or SD)
	}		
	else if (pSRC->stATTB.attb&MDIN_QUALITY_HD) {
		if (pOUT->stATTB.attb&MDIN_COLORSPACE_YUV)
			 pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscRGBtoYUV_HD_StdRange;	// RGB(HD) to YUV(HD or SD)
		else pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscRGBtoYUV_HD_FullRange;	// RGB(HD) to RGB(HD or SD)
	}
	else {
		if (pOUT->stATTB.attb&MDIN_COLORSPACE_YUV)
			 pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscRGBtoYUV_SD_StdRange;	// RGB(SD) to YUV(HD or SD)
		else pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscRGBtoYUV_SD_FullRange;	// RGB(SD) to RGB(HD or SD)
	}
#endif	/* SOURCE_CSC_STD_RANGE */

	memcpy(&stCSC, (PBYTE)pCSC, sizeof(MDIN_CSCCTRL_INFO));
	for (i=0; i<3; i++) stCSC.in[i] = (SHORT)stCSC.in[i]/4;
	for (i=0; i<3; i++) stCSC.out[i]= (SHORT)stCSC.out[i]/4;
	stCSC.ctrl = (pSRC->stATTB.attb&MDIN_COLORSPACE_YUV)? 0x7f7c : 0x7f3c;

	// use user-defined CSC if exist
	if (pSRC->pCSC!=NULL) memcpy(&stCSC, (PBYTE)pSRC->pCSC, sizeof(MDIN_CSCCTRL_INFO));

	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x181, (PBYTE)&stCSC, sizeof(MDIN_CSCCTRL_INFO)-2)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x180, stCSC.ctrl)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINAUX_SetOutVideoCSC(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_x;
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_x;

	SHORT i, coef[9];	MDIN_CSCCTRL_INFO stCSC, *pCSC;
	LONG contrast, saturation, brightness, coshue, sinhue;

#if OUTPUT_CSC_STD_RANGE == 1
	if (pSRC->stATTB.attb&MDIN_QUALITY_HD) {
		if (pOUT->stATTB.attb&MDIN_COLORSPACE_YUV) {
			if (pOUT->stATTB.attb&MDIN_QUALITY_HD)
				 pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscBypass_StdRange;	// RGB or YUV(HD) to YUV(HD)
			else pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscHDtoSD_StdRange;	// RGB or YUV(HD) to YUV(SD)
		}
		else {
			if (pOUT->stATTB.attb&MDIN_QUALITY_HD)
				 pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscYUVtoRGB_HD_StdRange;	// RGB or YUV(HD) to RGB(HD)
			else pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscYUVtoRGB_HD_StdRange;	// RGB or YUV(HD) to RGB(SD)
		}
	}
	else {
		if (pOUT->stATTB.attb&MDIN_COLORSPACE_YUV) {
			if (pOUT->stATTB.attb&MDIN_QUALITY_HD)
				 pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscSDtoHD_StdRange;	// RGB or YUV(SD) to YUV(HD)
			else pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscBypass_StdRange;	// RGB or YUV(SD) to YUV(SD)
		}
		else {
			if (pOUT->stATTB.attb&MDIN_QUALITY_HD)
				 pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscYUVtoRGB_SD_StdRange;	// RGB or YUV(SD) to RGB(HD)
			else pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscYUVtoRGB_SD_StdRange;	// RGB or YUV(SD) to RGB(SD)
		}
	}
#else	/* OUTPUT_CSC_STD_RANGE == 0 */
	if (pSRC->stATTB.attb&MDIN_QUALITY_HD) {
		if (pOUT->stATTB.attb&MDIN_COLORSPACE_YUV) {
			if (pOUT->stATTB.attb&MDIN_QUALITY_HD)
				 pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscBypass_StdRange;	// RGB or YUV(HD) to YUV(HD)
			else pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscHDtoSD_StdRange;	// RGB or YUV(HD) to YUV(SD)
		}
		else {
			if (pOUT->stATTB.attb&MDIN_QUALITY_HD)
				 pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscYUVtoRGB_HD_FullRange;	// RGB or YUV(HD) to RGB(HD)
			else pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscYUVtoRGB_HD_FullRange;	// RGB or YUV(HD) to RGB(SD)
		}
	}
	else {
		if (pOUT->stATTB.attb&MDIN_COLORSPACE_YUV) {
			if (pOUT->stATTB.attb&MDIN_QUALITY_HD)
				 pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscSDtoHD_StdRange;	// RGB or YUV(SD) to YUV(HD)
			else pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscBypass_StdRange;	// RGB or YUV(SD) to YUV(SD)
		}
		else {
			if (pOUT->stATTB.attb&MDIN_QUALITY_HD)
				 pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscYUVtoRGB_SD_FullRange;	// RGB or YUV(SD) to RGB(HD)
			else pCSC = (PMDIN_CSCCTRL_INFO)&MDIN_CscYUVtoRGB_SD_FullRange;	// RGB or YUV(SD) to RGB(SD)
		}
	}
#endif	/* OUTPUT_CSC_STD_RANGE */

	memcpy(&stCSC, (PBYTE)pCSC, sizeof(MDIN_CSCCTRL_INFO));
	for (i=0; i<3; i++) stCSC.in[i] = (SHORT)stCSC.in[i]/4;
	for (i=0; i<3; i++) stCSC.out[i]= (SHORT)stCSC.out[i]/4;
	stCSC.ctrl = 0x7f7c;

	// use user-defined CSC if exist
	if (pOUT->pCSC!=NULL) memcpy(&stCSC, (PBYTE)pOUT->pCSC, sizeof(MDIN_CSCCTRL_INFO));

	saturation = (LONG)(WORD)pOUT->saturation;
	contrast = (LONG)(WORD)pOUT->contrast;
	brightness = (LONG)(WORD)pOUT->brightness;
	coshue = (LONG)(SHORT)MDIN_CscCosHue[pOUT->hue];
	sinhue = (LONG)(SHORT)MDIN_CscSinHue[pOUT->hue];
	for (i=0; i<9; i++) coef[i] = (SHORT)stCSC.coef[i];

	stCSC.coef[0] = CLIP12((((coef[0]*contrast)>>7)))&0xfff;
	stCSC.coef[1] = CLIP12((((coef[1]*coshue+coef[2]*sinhue)*saturation)>>17))&0xfff;
	stCSC.coef[2] = CLIP12((((coef[2]*coshue-coef[1]*sinhue)*saturation)>>17))&0xfff;
	stCSC.coef[3] = CLIP12((((coef[3]*contrast)>>7)))&0xfff;
	stCSC.coef[4] = CLIP12((((coef[4]*coshue+coef[5]*sinhue)*saturation)>>17))&0xfff;
	stCSC.coef[5] = CLIP12((((coef[5]*coshue-coef[4]*sinhue)*saturation)>>17))&0xfff;
	stCSC.coef[6] = CLIP12((((coef[6]*contrast)>>7)))&0xfff;
	stCSC.coef[7] = CLIP12((((coef[7]*coshue+coef[8]*sinhue)*saturation)>>17))&0xfff;
	stCSC.coef[8] = CLIP12((((coef[8]*coshue-coef[7]*sinhue)*saturation)>>17))&0xfff;
	stCSC.in[0]	  = CLIP09((((SHORT)stCSC.in[0])+(brightness-128)))&0x1ff;

#if RGB_GAIN_OFFSET_TUNE == 1
	if ((pOUT->stATTB.attb&MDIN_COLORSPACE_YUV)==0) {	// only apply for RGB output
		stCSC.coef[0] = CLIP12((((coef[0]*contrast*(LONG)pOUT->g_gain)>>14)))&0xfff;
		stCSC.coef[3] = CLIP12((((coef[0]*contrast*(LONG)pOUT->b_gain)>>14)))&0xfff;
		stCSC.coef[6] = CLIP12((((coef[0]*contrast*(LONG)pOUT->r_gain)>>14)))&0xfff;
		stCSC.out[0]  = CLIP09((((SHORT)pOUT->g_offset)-128)+stCSC.out[0])&0xfff;
		stCSC.out[1]  = CLIP09((((SHORT)pOUT->b_offset)-128)+stCSC.out[1])&0xfff;
		stCSC.out[2]  = CLIP09((((SHORT)pOUT->r_offset)-128)+stCSC.out[2])&0xfff;
	}
#endif

	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x191, (PBYTE)&stCSC, sizeof(MDIN_CSCCTRL_INFO)-2)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x190, stCSC.ctrl)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINAUX_SetOutVideoSYNC(PMDIN_VIDEO_INFO pINFO)
{
	MDIN_OUTVIDEO_SYNC	stSYNC, *pSYNC;		WORD mode;
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_x;

	pSYNC = (PMDIN_OUTVIDEO_SYNC)&defMDINOutSync[pOUT->frmt];
	if (pOUT->pSYNC!=NULL) pSYNC = pOUT->pSYNC;	// use user-defined SYNC if exist

	memcpy(&stSYNC, (PBYTE)pSYNC, sizeof(MDIN_OUTVIDEO_SYNC));
	stSYNC.posFLD |= (pOUT->stATTB.attb&MDIN_SCANTYPE_PROG)? 0 : (1<<15);	// interlace out

	// restore Hsync delay
	if (pINFO->dacPATH!=DAC_PATH_MAIN_PIP) {stSYNC.bgnHA += 40;	stSYNC.endHA += 40;}		

	// MUX656 format
	if (pOUT->mode==MDIN_OUT_MUX656_8||pOUT->mode==MDIN_OUT_MUX656_10) {
		stSYNC.totHS = stSYNC.totHS*2;
		stSYNC.bgnHA = stSYNC.bgnHA*2;	stSYNC.endHA = stSYNC.endHA*2;
		stSYNC.bgnHS = stSYNC.bgnHS*2;	stSYNC.endHS = stSYNC.endHS*2;
	}

	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x16e, (PBYTE)&stSYNC, 36)) return MDIN_I2C_ERROR;

	// aux_etc_ctrl2 - aux_free_run
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x142, 2, 1, MBIT(pOUT->fine,MDIN_SYNC_FREERUN))) return MDIN_I2C_ERROR;

	// aux_frame_ptr_ctrl - aux_rpt_chk_offset
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x144, 0, 4, 10)) return MDIN_I2C_ERROR;	// fix 10

	// aux_ptr_ctrl - aux_sg_mch_en, aux_dig_mch_en ==> for 4-CH input mode, 2-HD input mode
	if		(pINFO->dacPATH==DAC_PATH_AUX_4CH)	mode = 2;
	else if (pINFO->dacPATH==DAC_PATH_AUX_2HD)	mode = 2;
	else										mode = 0;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x147, 14, 2, mode)) return MDIN_I2C_ERROR;

	// aux_cid_sel - aux_mwin_v, aux_mwin_h ==> for 4-CH input mode, 2-HD input mode
	if		(pINFO->dacPATH==DAC_PATH_AUX_4CH)	mode = (1<<2)|1;
	else if (pINFO->dacPATH==DAC_PATH_AUX_2HD)	mode = (0<<2)|1;
	else										mode = (0<<2)|0;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x14a, 12, 4, mode)) return MDIN_I2C_ERROR;

	// fcmc_arb_ctrl_2 - auxxr_pri ==> for 4-CH input mode
	mode = (pINFO->dacPATH==DAC_PATH_AUX_4CH)? 15 : 6;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1d6,  8, 4, mode)) return MDIN_I2C_ERROR;

	// fcmc_arb_ctrl_3 - auxxr_starv ==> for 4-CH input mode
	mode = (pINFO->dacPATH==DAC_PATH_AUX_4CH)? 1 : 7;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1d7,  0, 4, mode)) return MDIN_I2C_ERROR;

	// bg_color_y, cbcr
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x14c, 0, 8, 0)) return MDIN_I2C_ERROR;		// fix black
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x14d, 0x8080)) return MDIN_I2C_ERROR;

	// set aux video clock
	if (MDINAUX_SetVideoPLL(stSYNC.xclkS, stSYNC.xclkF, stSYNC.xclkT)) return MDIN_I2C_ERROR;
//	if (MDINAUX_SetVideoPLL(6, 1, 4)) return MDIN_I2C_ERROR;	// fix 65MHz for test purpose

	// aux_cid_sel - aux_channel_sel ==> for 4-CH input mode, 2-HD input mode
	if		(pINFO->dacPATH==DAC_PATH_AUX_4CH)	mode = 0xe4;
	else if (pINFO->dacPATH==DAC_PATH_AUX_2HD)	mode = 0x08;
	else										mode = 0xe4;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x14a, 0, 8, 8)) return MDIN_I2C_ERROR;		// fix channel
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static BOOL MDINAUX_IsCROP(PMDIN_VIDEO_INFO pINFO)
{
	return (pINFO->stCROP_x.w==0||pINFO->stCROP_x.h==0)? FALSE : TRUE;
}

//--------------------------------------------------------------------------------------------------------------------------
static BOOL MDINAUX_IsZOOM(PMDIN_VIDEO_INFO pINFO)
{
	return (pINFO->stZOOM_x.w==0||pINFO->stZOOM_x.h==0)? FALSE : TRUE;
}

//--------------------------------------------------------------------------------------------------------------------------
static BOOL MDINAUX_IsVIEW(PMDIN_VIDEO_INFO pINFO)
{
	return (pINFO->stVIEW_x.w==0||pINFO->stVIEW_x.h==0)? FALSE : TRUE;
}

#if	defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380)
//--------------------------------------------------------------------------------------------------------------------------
static void MDINAUX_Get4CHScale(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_4CHVIDEO_INFO p4CH = (PMDIN_4CHVIDEO_INFO)&pINFO->st4CH_x;
	PMDIN_MFCSCALE_INFO pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_x;

	switch (p4CH->view) {
		case MDIN_4CHVIEW_CH01: case MDIN_4CHVIEW_CH02: 
		case MDIN_4CHVIEW_CH03: case MDIN_4CHVIEW_CH04:
				 pMFC->st4CH.w = pMFC->stDST.w/1; pMFC->st4CH.h = pMFC->stDST.h/1; break;

		case MDIN_4CHVIEW_CH12:	case MDIN_4CHVIEW_CH13: 
		case MDIN_4CHVIEW_CH14: case MDIN_4CHVIEW_CH23: 
		case MDIN_4CHVIEW_CH24:	case MDIN_4CHVIEW_CH34:
				 pMFC->st4CH.w = pMFC->stDST.w/2; pMFC->st4CH.h = pMFC->stDST.h/1; break;
		default: pMFC->st4CH.w = pMFC->stDST.w/2; pMFC->st4CH.h = pMFC->stDST.h/2; break;
	}
}
#endif	/* defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380) */

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINAUX_SetMFCScaleWind(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_x;
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_x;
	PMDIN_MFCSCALE_INFO pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_x;

	memset(&pINFO->stMFC_x, 0, sizeof(MDIN_MFCSCALE_INFO));	// clear

	// config clip_window
	pMFC->stCUT.w  = pSRC->stATTB.Hact;	pMFC->stCUT.h  = pSRC->stATTB.Vact;
	if (MDINAUX_IsCROP(pINFO)) memcpy(&pMFC->stCUT, &pINFO->stCROP_x, 8);
//	pMFC->stCUT.x += pSRC->offH;		pMFC->stCUT.y += pSRC->offV;

	// for 2-HD input mode
	if (pINFO->dacPATH==DAC_PATH_AUX_2HD) pMFC->stCUT.w /= 2;

	// config win_size_h/v, win_posi_h/v - stDST
	pMFC->stDST.w  = pOUT->stATTB.Hact;	pMFC->stDST.h  = pOUT->stATTB.Vact;
	if (MDINAUX_IsVIEW(pINFO)) memcpy(&pMFC->stDST, &pINFO->stVIEW_x, 8);

	// for 2-HD input mode
	if (pINFO->dacPATH==DAC_PATH_AUX_2HD) pMFC->stDST.h /= 2;

	// config mem_size_h/v (src_size_h2/v2, dst_size_h2/v2) - stFFC
	pMFC->stFFC.sw = pMFC->stCUT.w;		pMFC->stFFC.sh = pMFC->stCUT.h;
	pMFC->stFFC.dw = pMFC->stCUT.w;		pMFC->stFFC.dh = pMFC->stCUT.h;

	if ((pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)==0) pMFC->stFFC.sh /= 2;
	if (pMFC->stFFC.sw>pMFC->stDST.w)	pMFC->stFFC.dw = pMFC->stDST.w; // H-down
	if (pMFC->stFFC.sh>pMFC->stDST.h)	pMFC->stFFC.dh = pMFC->stDST.h; // V-down

#if	defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380)
	if (pINFO->dacPATH==DAC_PATH_AUX_4CH) MDINAUX_Get4CHScale(pINFO);
	if (pINFO->dacPATH==DAC_PATH_AUX_4CH&&pMFC->stFFC.sw>pMFC->stDST.w/2)
		pMFC->stFFC.dw = pMFC->stDST.w/2;	// H-down for 4-CH input mode
	if (pINFO->dacPATH==DAC_PATH_AUX_4CH&&pMFC->stFFC.sh>pMFC->st4CH.h)
		pMFC->stFFC.dh = pMFC->st4CH.h;		// V-down for 4-CH input mode
#endif

	// config mem_size_h, mem_size_v
	pMFC->stMEM.w  = pMFC->stFFC.dw;	pMFC->stMEM.h  = pMFC->stFFC.dh;

	// config src_size_h/v, src_posi_h/v - stSRC
	pMFC->stSRC.w  = pMFC->stFFC.dw;	pMFC->stSRC.h  = pMFC->stFFC.dh;
	if (MDINAUX_IsZOOM(pINFO)) memcpy(&pMFC->stSRC, &pINFO->stZOOM_x, 8);

	// config dst_size_h/v - stCRS
	pMFC->stCRS.dw = pMFC->stDST.w;		pMFC->stCRS.dh = pMFC->stDST.h;

	// for 4-CH input mode
	if (pINFO->dacPATH==DAC_PATH_AUX_4CH) pMFC->stCRS.dw = pMFC->st4CH.w;
	if (pINFO->dacPATH==DAC_PATH_AUX_4CH) pMFC->stCRS.dh = pMFC->st4CH.h;

	// for 2-HD input mode
	if (pINFO->dacPATH==DAC_PATH_AUX_2HD) pMFC->stCRS.dw /= 2;

#if __MDIN3xx_DBGPRT__ == 1
	UARTprintf("[AUX] sCUT.w=%d, sCUT.h=%d\n", pMFC->stCUT.w, pMFC->stCUT.h);
	UARTprintf("[AUX] CROP.w=%d, CROP.h=%d\n", pINFO->stCROP_x.w, pINFO->stCROP_x.h);
	UARTprintf("[AUX] sDST.w=%d, sDST.h=%d\n", pMFC->stDST.w, pMFC->stDST.h);
	UARTprintf("[AUX] VIEW.w=%d, VIEW.h=%d\n", pINFO->stVIEW_x.w, pINFO->stVIEW_x.h);
	UARTprintf("[AUX] sMEM.w=%d, sMEM.h=%d\n", pMFC->stMEM.w, pMFC->stMEM.h);
	UARTprintf("[AUX] sSRC.w=%d, sSRC.h=%d\n", pMFC->stSRC.w, pMFC->stSRC.h);
	UARTprintf("[AUX] ZOOM.w=%d, ZOOM.h=%d\n", pINFO->stZOOM_x.w, pINFO->stZOOM_x.h);
#endif

	// aux_src_offset
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x160, pMFC->stCUT.x)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x161, pMFC->stCUT.y)) return MDIN_I2C_ERROR;

	// aux_src_size
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x162, pMFC->stCUT.w)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x163, pMFC->stCUT.h)) return MDIN_I2C_ERROR;

	// aux_mem_size, aux_src_posi
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x164, pMFC->stSRC.x)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x165, pMFC->stSRC.y)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x166, pMFC->stSRC.w)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x167, pMFC->stSRC.h)) return MDIN_I2C_ERROR;

	// aux_dst_size_h/v
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x168, pMFC->stCRS.dw)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x169, pMFC->stCRS.dh)) return MDIN_I2C_ERROR;

	// aux_win_size, aux_win_posi
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x16a, pMFC->stDST.x)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x16b, pMFC->stDST.y)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x16c, pMFC->stDST.w)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x16d, pMFC->stDST.h)) return MDIN_I2C_ERROR;

	// if src is interlace, stCUT.h/2. if dst is interlace, stDST.h/2
	if ((pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)==0) pMFC->stCUT.h /= 2;
//	if ((pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)==0) pMFC->stMEM.h /= 2;
	if ((pOUT->stATTB.attb&MDIN_SCANTYPE_PROG)==0) pMFC->stDST.h /= 2;
/*
	// if src is interlace && V up-scaler or dst is interlace, stMEM.h/2
	if ((pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)==0&&(pMFC->stCUT.h<=pMFC->stDST.h)||
		(pOUT->stATTB.attb&MDIN_SCANTYPE_PROG)==0) pMFC->stMEM.h /= 2;
*/
	// if V up-scaler && src is interlace, stMEM.h/2
	if ((pMFC->stCUT.h<=pMFC->stDST.h)&&
		(pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)==0) pMFC->stMEM.h /= 2;

	// for 4-CH input mode
	if (pINFO->dacPATH==DAC_PATH_AUX_4CH) pMFC->stDST.w = pMFC->st4CH.w;
	if (pINFO->dacPATH==DAC_PATH_AUX_4CH) pMFC->stDST.h = pMFC->st4CH.h;

	// MUX656 format - aux_dst_size_h, aux_win_size_h
	if (pOUT->mode!=MDIN_OUT_MUX656_8&&pOUT->mode!=MDIN_OUT_MUX656_10) return MDIN_NO_ERROR;

	pMFC->stCRS.dw *= 2;	pMFC->stDST.w *= 2;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x168, pMFC->stCRS.dw)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x16c, pMFC->stDST.w )) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINAUX_SetMFCScaleCtrl(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_x;
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_x;
	PMDIN_MFCSCALE_INFO pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_x;
	WORD mode = (1<<6)|(1<<5)|(1<<4)|(1<<3);	// H/V scaler bypass

	if (pMFC->stCUT.h<=pMFC->stDST.h) mode |= (6<<9);	// V up-scaler
	if (pMFC->stCUT.h >pMFC->stDST.h) mode |= (5<<9);	// V dn-scaler
//	if (pMFC->stCUT.h==pMFC->stDST.h) mode |= (1<<9);	// V bypass

	if (pMFC->stCUT.h <pMFC->stDST.h) mode &= ~(1<<5);	// V up-scaler
	if (pMFC->stCUT.h >pMFC->stDST.h) mode &= ~(5<<3);	// V dn-scaler

	if (pMFC->stMEM.w <pMFC->stDST.w) mode &= ~(1<<6);	// H up-scaler
	if (pMFC->stCUT.w >pMFC->stMEM.w) mode &= ~(1<<4);	// H dn-scaler

	// for 4-CH input mode
//	if (pINFO->dacPATH==DAC_PATH_AUX_4CH) mode &= ~(1<<6);	// H up-scaler

	// aux_etc_ctrl - H/V scaling
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x142, 3, 9, (mode>>3))) return MDIN_I2C_ERROR;

	// aux_etc_ctrl - aux_rd_ext_buf
	mode = (pINFO->dacPATH==DAC_PATH_AUX_4CH)? 0 : 1;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x141, 1, 1, mode)) return MDIN_I2C_ERROR;
//	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x141, 1, 1, 1)) return MDIN_I2C_ERROR;	// fix ext-buff

	// if src is progressive && out is interlace && V dn-scaler then aux_rd_frm2fld = 1
	if ((pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)&&(pMFC->stCUT.h>pMFC->stDST.h)&&
		(pOUT->stATTB.attb&MDIN_SCANTYPE_PROG)==0)	mode = 1;
	else											mode = 0;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x142, 13, 1, mode)) return MDIN_I2C_ERROR;

	// if src is interlace && out is interlace && V dn-scaler then aux_skip_only_en = 1
	if ((pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)==0&&(pMFC->stCUT.h>pMFC->stDST.h)&&
		(pOUT->stATTB.attb&MDIN_SCANTYPE_PROG)==0)	mode = 1;
	else											mode = 0;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x144, 5, 1, mode)) return MDIN_I2C_ERROR;

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINAUX_SetAuxVideoCLK(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_MFCSCALE_INFO pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_x;
	WORD aux_in, aux_lm, dac_in;//, pip_on;

	switch (pINFO->srcPATH) {
//		case PATH_MAIN_B_AUX_A:
		case PATH_MAIN_A_AUX_A:	aux_in = (0<<8)|0; break;
		case PATH_MAIN_A_AUX_B:
		case PATH_MAIN_B_AUX_B:	aux_in = (0<<8)|1; break;
		default:				aux_in = (1<<8)|0; break;
	}

	// clk_aux_ctrl - clk_aux_in_sel
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x046, 8, 1, HIBYTE(aux_in))) return MDIN_I2C_ERROR;

	// clk_m_ctrl - clk_m_aux_sel
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x047, 0, 1, LOBYTE(aux_in))) return MDIN_I2C_ERROR;

	switch (pINFO->dacPATH) {
		case DAC_PATH_MAIN_OUT:	dac_in = (0<<8)|0; aux_lm = (1<<8)|1; break;//pip_on = 0; break;
		case DAC_PATH_MAIN_PIP:	dac_in = (0<<8)|0; aux_lm = (0<<8)|2; break;//pip_on = 1; break;
		case DAC_PATH_AUX_OUT:	dac_in = (2<<8)|2; aux_lm = (2<<8)|3; break;//pip_on = 0; break;
		case DAC_PATH_VENC_YC:	dac_in = (1<<8)|1; aux_lm = (1<<8)|1; break;//pip_on = 0; break;
		case DAC_PATH_AUX_4CH:	dac_in = (2<<8)|0; aux_lm = (0<<8)|2; break;//pip_on = 0; break;
		case DAC_PATH_AUX_2HD:	dac_in = (2<<8)|0; aux_lm = (0<<8)|2; break;//pip_on = 0; break;
		default:				dac_in = (0<<8)|0; aux_lm = (1<<8)|0; break;//pip_on = 0; break;
	}

	// if V dn-scaler then follow aux_in, others follow aux_out
	if (pMFC->stCUT.h>pMFC->stDST.h) {		// V dn-scaler
		aux_lm &= ~(0xff);	aux_lm |= ((HIBYTE(aux_in)==1)? 2 : 0);
	}

	// dac_src_sel - dac_data_sel
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x044, 14, 2, HIBYTE(dac_in))) return MDIN_I2C_ERROR;

	// dac_clk_sel - dac_clk_sel
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x045, 14, 2, LOBYTE(dac_in))) return MDIN_I2C_ERROR;

	// clk_aux_ctrl - clk_aux_out_sel
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x046,  3, 2, HIBYTE(aux_lm))) return MDIN_I2C_ERROR;

	// clk_m_ctrl - clk_aux_lm_sel
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x047, 14, 2, LOBYTE(aux_lm))) return MDIN_I2C_ERROR;

	// aux_frame_ptr_ctrl - aux_sync_pip_en
//	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x144, 6, 1, LOBYTE(pip_on))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

#if	defined(SYSTEM_USE_MDIN325)||defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380)
//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINAUX_SetEncFrmtByAUX(PMDIN_VIDEO_INFO pINFO)
{
	if (pINFO->encPATH!=VENC_PATH_PORT_X) return MDIN_NO_ERROR;

	switch (pINFO->stOUT_x.frmt) {
		case VIDOUT_720x576i50: pINFO->encFRMT = VID_VENC_PAL_B;  break;
		default:				pINFO->encFRMT = VID_VENC_NTSC_M; break;
	}
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINAUX_SetEncVideoFrmt(PMDIN_VIDEO_INFO pINFO)
{
	BYTE mode, eclk, nID, *pBuff;

	if (MDINAUX_SetEncFrmtByAUX(pINFO)) return MDIN_I2C_ERROR;

	switch (pINFO->encFRMT) {
		case VID_VENC_NTSC_M:	mode = (0<<4)|0; nID = 0; break;
		case VID_VENC_NTSC_J:	mode = (1<<4)|0; nID = 0; break;
		case VID_VENC_NTSC_443:	mode = (0<<4)|1; nID = 0; break;
		case VID_VENC_PAL_B:	mode = (2<<4)|1; nID = 1; break;
		case VID_VENC_PAL_D:	mode = (2<<4)|1; nID = 1; break;
		case VID_VENC_PAL_G:	mode = (2<<4)|1; nID = 1; break;
		case VID_VENC_PAL_H:	mode = (2<<4)|1; nID = 1; break;
		case VID_VENC_PAL_I:	mode = (2<<4)|1; nID = 1; break;
		case VID_VENC_PAL_M:	mode = (0<<4)|2; nID = 0; break;
		case VID_VENC_PAL_N:	mode = (0<<4)|1; nID = 1; break;
		case VID_VENC_PAL_Nc:	mode = (2<<4)|3; nID = 1; break;
		default:				mode = (0<<4)|0; nID = 0; break;
	}

	pBuff = (PBYTE)&MDIN_Encoder_Default[nID];
	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x1e0, pBuff, 38)) return MDIN_I2C_ERROR;

	// venc_input_config - venc_csc_sel
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1e0, 14, 2, HI4BIT(mode))) return MDIN_I2C_ERROR;

	// venc_output_mux - venc_mod_format
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1e8,  0, 3, LO4BIT(mode))) return MDIN_I2C_ERROR;

	switch (pINFO->encPATH) {
		case VENC_PATH_PORT_A:	mode = (0<<4)|2; eclk = 2; break;
		case VENC_PATH_PORT_B:	mode = (2<<4)|2; eclk = 3; break;
		default:				mode = (0<<4)|0; eclk = 1; break;
	}

	// clk_aux_ctrl - venc_clk_sel
	if (MDINHIF_RegField(MDIN_HOST_ID,  0x046, 11, 2, LO4BIT(eclk))) return MDIN_I2C_ERROR;

	// in_frame_cfg1 - venc_src_sel
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x01e, 12, 2, HI4BIT(mode))) return MDIN_I2C_ERROR;

	// venc_input_config - venc_input_sel
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1e0, 12, 2, LO4BIT(mode))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}
#endif	/* defined(SYSTEM_USE_MDIN325)||defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380) */

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Video Process Block (Input/Output/Scaler/Deinterlace/CSC/PLL)
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_VideoProcess(PMDIN_VIDEO_INFO pINFO)
{
	if (MDINAUX_SetSrcVideoFrmt(pINFO)) return MDIN_I2C_ERROR;		// set source video format
	if (MDINAUX_SetOutVideoFrmt(pINFO)) return MDIN_I2C_ERROR;		// set output video format

	if (MDINAUX_SetSrcVideoCSC(pINFO)) return MDIN_I2C_ERROR;		// set source video CSC
	if (MDINAUX_SetOutVideoCSC(pINFO)) return MDIN_I2C_ERROR;		// set output video CSC

	if (MDINAUX_SetOutVideoSYNC(pINFO)) return MDIN_I2C_ERROR;		// set output video SYNC

	if (MDINAUX_SetMFCScaleWind(pINFO)) return MDIN_I2C_ERROR;		// set MFC scaler window
	if (MDINAUX_SetMFCScaleCtrl(pINFO)) return MDIN_I2C_ERROR;		// set MFC scaler control

	if (MDINAUX_SetAuxVideoCLK(pINFO)) return MDIN_I2C_ERROR;		// set aux-video clock

#if	defined(SYSTEM_USE_MDIN325)||defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380)
	if (MDINAUX_SetEncVideoFrmt(pINFO)) return MDIN_I2C_ERROR;		// set video-encoder format
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetScaleProcess(PMDIN_VIDEO_INFO pINFO)
{
	if (MDINAUX_SetMFCScaleWind(pINFO)) return MDIN_I2C_ERROR;		// set MFC scaler window
	if (MDINAUX_SetMFCScaleCtrl(pINFO)) return MDIN_I2C_ERROR;		// set MFC scaler control
	if (MDINAUX_SetAuxVideoCLK(pINFO)) return MDIN_I2C_ERROR;		// set aux-video clock
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Source Video Block
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetSrcHighTopFieldID(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_x;

	if (OnOff)	pSRC->fine |=  MDIN_HIGH_IS_TOPFLD;
	else		pSRC->fine &= ~MDIN_HIGH_IS_TOPFLD;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x140, 5, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetSrcCbCrSwap(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_x;

	if (OnOff)	pSRC->fine |=  MDIN_CbCrSWAP_ON;
	else		pSRC->fine &= ~MDIN_CbCrSWAP_ON;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x140, 4, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetSrcHSPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_x;

	if (edge)	pSRC->stATTB.attb |=  MDIN_NEGATIVE_HACT;
	else		pSRC->stATTB.attb &= ~MDIN_NEGATIVE_HACT;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x140, 7, 1, MBIT(edge,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetSrcVSPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_x;

	if (edge)	pSRC->stATTB.attb |=  MDIN_NEGATIVE_VACT;
	else		pSRC->stATTB.attb &= ~MDIN_NEGATIVE_VACT;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x140, 6, 1, MBIT(edge,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetSrcOffset(PMDIN_VIDEO_INFO pINFO, WORD offH, WORD offV)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_x;

	pSRC->offH = offH;	pSRC->offV = offV;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x160, offH)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x161, offV)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetSrcCSCCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_CSCCTRL_INFO pCSC)
{
	pINFO->stSRC_x.pCSC = pCSC;
	return MDINAUX_SetSrcVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Output Video Block
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetOutCbCrSwap(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_x;

	if (OnOff)	pOUT->fine |=  MDIN_CbCrSWAP_ON;
	else		pOUT->fine &= ~MDIN_CbCrSWAP_ON;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x145, 6, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetOutHSPinMode(PMDIN_VIDEO_INFO pINFO, MDIN_HSOUT_MODE_t mode)
{
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_x;

	pOUT->stATTB.attb &= ~(MDIN_HSYNC_IS_HACT);
	pOUT->stATTB.attb |= ((WORD)mode<<14);

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x145, 12, 1, MBIT(mode,MDIN_HSOUT_PIN_DE))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetOutHSPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge)
{
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_x;

	if (edge)	pOUT->stATTB.attb |=  MDIN_POSITIVE_HSYNC;
	else		pOUT->stATTB.attb &= ~MDIN_POSITIVE_HSYNC;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x145, 14, 1, MBIT(edge,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetOutVSPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge)
{
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_x;

	if (edge)	pOUT->stATTB.attb |=  MDIN_POSITIVE_VSYNC;
	else		pOUT->stATTB.attb &= ~MDIN_POSITIVE_VSYNC;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x145, 13, 1, MBIT(edge,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetPictureBrightness(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_x.brightness = value;
	return MDINAUX_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetPictureContrast(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_x.contrast = value;
	return MDINAUX_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetPictureSaturation(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_x.saturation = value;
	return MDINAUX_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetPictureHue(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_x.hue = value;
	return MDINAUX_SetOutVideoCSC(pINFO);
}

#if RGB_GAIN_OFFSET_TUNE == 1
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetPictureGainR(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_x.r_gain = value;
	return MDINAUX_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetPictureGainG(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_x.g_gain = value;
	return MDINAUX_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetPictureGainB(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_x.b_gain = value;
	return MDINAUX_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetPictureOffsetR(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_x.r_offset = value;
	return MDINAUX_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetPictureOffsetG(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_x.g_offset = value;
	return MDINAUX_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetPictureOffsetB(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_x.b_offset = value;
	return MDINAUX_SetOutVideoCSC(pINFO);
}
#endif

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetOutCSCCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_CSCCTRL_INFO pCSC)
{
	pINFO->stOUT_x.pCSC = (PMDIN_CSCCTRL_INFO)pCSC;
	return MDINAUX_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetOutSYNCCtrl(PMDIN_VIDEO_INFO pINFO, PMDIN_OUTVIDEO_SYNC pSYNC)
{
	pINFO->stOUT_x.pSYNC = (PMDIN_OUTVIDEO_SYNC)pSYNC;
	return MDINAUX_SetOutVideoSYNC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Video Window
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_GetVideoWindowFRMB(PMDIN_VIDEO_INFO pINFO, PMDIN_VIDEO_WINDOW pFRMB)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_x;

	pFRMB->w = pINFO->stMFC_x.stMEM.w;
	pFRMB->h = pINFO->stMFC_x.stMEM.h;
	pFRMB->x = pFRMB->y = 0;

	if ((pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)==0) pFRMB->h *= 2;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetVideoWindowCROP(PMDIN_VIDEO_INFO pINFO, MDIN_VIDEO_WINDOW stCROP)
{
	memcpy(&pINFO->stCROP_x, &stCROP, sizeof(MDIN_VIDEO_WINDOW));
	return MDIN3xx_SetScaleProcess(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetVideoWindowZOOM(PMDIN_VIDEO_INFO pINFO, MDIN_VIDEO_WINDOW stZOOM)
{
	memcpy(&pINFO->stZOOM_x, &stZOOM, sizeof(MDIN_VIDEO_WINDOW));
	return MDINAUX_SetScaleProcess(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetVideoWindowVIEW(PMDIN_VIDEO_INFO pINFO, MDIN_VIDEO_WINDOW stVIEW)
{
	memcpy(&pINFO->stVIEW_x, &stVIEW, sizeof(MDIN_VIDEO_WINDOW));
	return MDIN3xx_SetScaleProcess(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Filters Block
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_SetFrontNRFilterCoef(PMDIN_AUXFILT_COEF pCoef)
{
	if (pCoef==NULL) pCoef = (PMDIN_AUXFILT_COEF)MDIN_AuxDownFilter_Default;
	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x151, (PBYTE)pCoef, 28)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_EnableFrontNRFilter(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x150, 0, 2, (OnOff)? 3 : 0)) return MDIN_I2C_ERROR;
	if (pINFO->stMFC_x.stMEM.w>1024) OnOff = OFF;	// if h_size>1024, then v-filter is OFF
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x150, 2, 3, (OnOff)? 7 : 0)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}
/*
//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Video-Encoder Block
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_EnableVENCColorMode(BOOL OnOff)
{
	if (MDINHIF_RegWrite(0x4C6, 0x0008)) return MDIN_I2C_ERROR;			// reg_oen
	if (MDINHIF_RegField(0x5D4, 13, 1, (OnOff)? 0 : 1)) return MDIN_I2C_ERROR;
	return (MDINHIF_RegWrite(0x4C4, 1))? MDIN_I2C_ERROR:MDIN_NO_ERROR;	// update local register
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_EnableVENCBlueScreen(BOOL OnOff)
{
	if (MDINHIF_RegWrite(0x4C6, 0x0008)) return MDIN_I2C_ERROR;			// reg_oen
	if (MDINHIF_RegField(0x5D4, 10, 1, (OnOff)? ON : OFF)) return MDIN_I2C_ERROR;
	return (MDINHIF_RegWrite(0x4C4, 1))? MDIN_I2C_ERROR:MDIN_NO_ERROR;	// update local register
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_EnableVENCPeakingFilter(BOOL OnOff)
{
	if (MDINHIF_RegWrite(0x4C6, 0x0008)) return MDIN_I2C_ERROR;			// reg_oen
	if (MDINHIF_RegField(0x5DC, 3, 1, (OnOff)? ON : OFF)) return MDIN_I2C_ERROR;
	return (MDINHIF_RegWrite(0x4C4, 1))? MDIN_I2C_ERROR:MDIN_NO_ERROR;	// update local register
}
*/
//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for ETC(reset, port out, freeze, etc) Block
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_EnableMirrorH(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_MFCSCALE_INFO pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_x;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x148, 0, 11, (OnOff)? pMFC->stFFC.dw-256 : 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x148, 15, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINAUX_EnableMirrorV(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_x;
	PMDIN_MFCSCALE_INFO pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_x;
	WORD wVal = (OnOff)? pMFC->stMEM.h-1 : 0;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x149, 0, 11, wVal)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x148, 14, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;

	// inverse input field-id
	wVal = MBIT(pSRC->fine,MDIN_HIGH_IS_TOPFLD);	wVal = (OnOff)? ~wVal : wVal;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x140, 5, 1, wVal&0x01)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

/*  FILE_END_HERE */
