//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2008  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:	MDIN3xx.C
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

#define YC_SWAP         0


// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static Global Data section variables
// ----------------------------------------------------------------------
WORD		mdinERR = 0, mdinREV = 0;

static WORD fbADDR, GetDID, GetMEM, GetROW, GetIRQ = 0;
static WORD GetPRI, GetSTV, GetCLK, GetPAD, GetENC;
static WORD vpll_P = 0, vpll_M = 0, vpll_S = 0;
static BOOL frez_M = 0;

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
static MDIN_ERROR_t MDIN3xx_SetVideoPLL(WORD P, WORD M, WORD S)
{
	if (vpll_P==P&&vpll_M==M&&vpll_S==S) return MDIN_NO_ERROR;

	if (MDINHIF_RegField(MDIN_HOST_ID, 0x020, 0, 1, 1)) return MDIN_I2C_ERROR;	// disable PLL
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x02c, P)) return MDIN_I2C_ERROR;		// pre-divider
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x02e, M)) return MDIN_I2C_ERROR;		// post-divider
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x030, S)) return MDIN_I2C_ERROR;		// post-scaler
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x020, 0, 1, 0)) return MDIN_I2C_ERROR;	// enable PLL

	vpll_P = P; vpll_M = M; vpll_S = S;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetMemoryPLL(WORD P, WORD M, WORD S)
{
//	Mclk[MHz] = (Fin[MHz]*M)/(P*2^S)
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x020, 1, 1, 1)) return MDIN_I2C_ERROR;	// disable PLL
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x032, P)) return MDIN_I2C_ERROR;		// pre-divider
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x034, (S<<7)|M)) return MDIN_I2C_ERROR;	// post-scaler & post-divider
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x020, 1, 1, 0)) return MDIN_I2C_ERROR;	// enable PLL
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_ResetOutSync(WORD delay)
{
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x099, 0, 13, delay)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x143, 0, 11, delay)) return MDIN_I2C_ERROR;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x099, 15, 1, 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x143, 11, 1, 0)) return MDIN_I2C_ERROR;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x099, 15, 1, 1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x143, 11, 1, 1)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetSrcClockPath(PMDIN_VIDEO_INFO pINFO)
{
	BOOL inA, inB, mode;
	PMDIN_SRCVIDEO_INFO pSRC_a, pSRC_b;

	switch (pINFO->srcPATH) {
		case PATH_MAIN_A_AUX_A: pSRC_a = &pINFO->stSRC_a; pSRC_b = &pINFO->stSRC_a;
							inA = MDIN_clk_ab_CLK_A; inB = MDIN_clk_ab_CLK_A; break;
		case PATH_MAIN_A_AUX_B: pSRC_a = &pINFO->stSRC_a; pSRC_b = &pINFO->stSRC_b;
							inA = MDIN_clk_ab_CLK_A; inB = MDIN_clk_ab_CLK_B; break;
//		case PATH_MAIN_B_AUX_A: pSRC_a = &pINFO->stSRC_b; pSRC_b = &pINFO->stSRC_a;
//							inA = MDIN_clk_ab_CLK_B; inB = MDIN_clk_ab_CLK_A; break;
		case PATH_MAIN_B_AUX_B: pSRC_a = &pINFO->stSRC_b; pSRC_b = &pINFO->stSRC_b;
							inA = MDIN_clk_ab_CLK_B; inB = MDIN_clk_ab_CLK_B; break;
		case PATH_MAIN_A_AUX_M: pSRC_a = &pINFO->stSRC_a; pSRC_b = &pINFO->stSRC_b;
							inA = MDIN_clk_ab_CLK_A; inB = MDIN_clk_ab_CLK_B; break;
		case PATH_MAIN_B_AUX_M: pSRC_a = &pINFO->stSRC_b; pSRC_b = &pINFO->stSRC_b;
							inA = MDIN_clk_ab_CLK_B; inB = MDIN_clk_ab_CLK_B; break;
		default:				pSRC_a = &pINFO->stSRC_a; pSRC_b = &pINFO->stSRC_b;
							inA = MDIN_clk_ab_CLK_A; inB = MDIN_clk_ab_CLK_B; break;
	}

	// set clk_a from CLK_A or CLK_B
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x047, 4, 1, inA)) return MDIN_I2C_ERROR;

	// set clk_b from CLK_A or CLK_B
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x047, 5, 1, inB)) return MDIN_I2C_ERROR;

	// set clk_a1 from clk_a or from clk_a/2 in 4-CH input mode
	mode = (pINFO->dacPATH==DAC_PATH_AUX_4CH)? MDIN_a1_clka_D2P0 : MDIN_a1_clka_D1P0;
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x048, 8, 2, mode)) return MDIN_I2C_ERROR;

	// set clk_b1 from clk_b or from clk_b/2 in 4-CH input mode
	mode = (pINFO->dacPATH==DAC_PATH_AUX_4CH)? MDIN_b1_clkb_D2P0 : MDIN_b1_clkb_D1P0;
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x049, 8, 2, mode)) return MDIN_I2C_ERROR;

	// set clk_a2 from clk_a or from clk_a/2 in 4-CH input mode
	mode = (pINFO->dacPATH==DAC_PATH_AUX_4CH)? MDIN_a1_clka_D2P0 : MDIN_a1_clka_D1P0;
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x048, 3, 2, mode)) return MDIN_I2C_ERROR;

	// set clk_b2 from clk_b or from clk_b/2 in 4-CH input mode
	mode = (pINFO->dacPATH==DAC_PATH_AUX_4CH)? MDIN_b1_clkb_D2P0 : MDIN_b1_clkb_D1P0;
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x049, 3, 2, mode)) return MDIN_I2C_ERROR;

	// set clk_m1_sel from clk_a or clk_a/2 in mux or clk_a/4 in 4-CH input mode
	if (pSRC_a->mode==MDIN_SRC_MUX656_8||pSRC_a->mode==MDIN_SRC_MUX656_10||
		pSRC_a->mode==MDIN_SRC_SEP656_8||pSRC_a->mode==MDIN_SRC_SEP656_10)
		 mode = MDIN_m1_clka_D2P2;
	else mode = MDIN_m1_clka_D1P0;
	if (pINFO->dacPATH==DAC_PATH_AUX_4CH) mode = 4;	// 4-CH input mode
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x048, 13, 3, mode)) return MDIN_I2C_ERROR;

	// set clk_m2_sel from clk_b or clk_b/2 in mux or clk_a/4 in 4-CH input mode
	if (pSRC_b->mode==MDIN_SRC_MUX656_8||pSRC_b->mode==MDIN_SRC_MUX656_10||
		pSRC_b->mode==MDIN_SRC_SEP656_8||pSRC_b->mode==MDIN_SRC_SEP656_10)
		 mode = MDIN_m2_clkb_D2P4;
	else mode = MDIN_m2_clkb_D1P0;
	if (pINFO->dacPATH==DAC_PATH_AUX_4CH) mode = 4;	// 4-CH input mode
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x049, 13, 3, mode)) return MDIN_I2C_ERROR;

	// set clk_a2_inverse, clk_b2_inverse ==> 4-CH input mode
//	if (MDINHIF_RegField(MDIN_HOST_ID, 0x048, 2, 1, 0)) return MDIN_I2C_ERROR;
//	if (MDINHIF_RegField(MDIN_HOST_ID, 0x049, 2, 1, 0)) return MDIN_I2C_ERROR;

#if	defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380)
	// set chip_read_mode_a & chip_read_mode_b ==> 4-CH input mode
	mode = (pINFO->st4CH_x.chID==MDIN_4CHID_IN_SYNC)? 3 : 0;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x000, 14, 2, mode)) return MDIN_I2C_ERROR;
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetSrcVideoPort(PMDIN_VIDEO_INFO pINFO, WORD nID)
{
	WORD mode = 0;
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)((nID==1)? &pINFO->stSRC_a : &pINFO->stSRC_b);

	if (pSRC->frmt>=VIDSRC_FORMAT_END) return MDIN_INVALID_FORMAT;

	// set Htotal, attb[H/V-Polarity,ScanType], H/V size
	memcpy(&pSRC->stATTB, (PBYTE)&defMDINSrcVideo[pSRC->frmt], sizeof(MDIN_SRCVIDEO_ATTB));

	// set Quality, System
	pSRC->stATTB.attb &= ~(MDIN_QUALITY_HD|MDIN_VIDEO_SYSTEM);
	switch (pSRC->frmt) {
		case VIDSRC_720x480i60:		case VIDSRC_720x576i50:
		case VIDSRC_720x480p60:		case VIDSRC_720x576p50:
			pSRC->stATTB.attb |= (MDIN_QUALITY_SD|MDIN_VIDEO_SYSTEM); break;
		case VIDSRC_1280x720p60:	case VIDSRC_1280x720p50:
			pSRC->stATTB.attb |= (MDIN_QUALITY_HD|MDIN_VIDEO_SYSTEM); break;
		case VIDSRC_1920x1080i60:	case VIDSRC_1920x1080i50:
		case VIDSRC_1920x1080p60:	case VIDSRC_1920x1080p50:
			pSRC->stATTB.attb |= (MDIN_QUALITY_HD|MDIN_VIDEO_SYSTEM); break;
		default:
			pSRC->stATTB.attb |= (MDIN_QUALITY_SD|MDIN_PCVGA_SYSTEM); break;
	}

	// set InPort, Precision, PixelSize, ColorSpace
	pSRC->stATTB.attb &= ~(MDIN_USE_INPORT_A|MDIN_PRECISION_8|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV);
	switch (pSRC->mode) {
		case MDIN_SRC_RGB444_8:		mode = (0<<7)|(0<<4);	// mux with sync | src format
			pSRC->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_444|MDIN_COLORSPACE_RGB); break;
		case MDIN_SRC_YUV444_8:		mode = (0<<7)|(0<<4);	// mux with sync | src format
			pSRC->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_444|MDIN_COLORSPACE_YUV); break;
		case MDIN_SRC_EMB422_8:		mode = (0<<7)|(3<<4);	// mux with sync | src format
			pSRC->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_SRC_SEP422_8:		mode = (0<<7)|(1<<4);	// mux with sync | src format
			pSRC->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_SRC_EMB422_10:	mode = (0<<7)|(3<<4);	// mux with sync | src format
			pSRC->stATTB.attb |= (MDIN_PRECISION_10|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_SRC_SEP422_10:	mode = (0<<7)|(1<<4);	// mux with sync | src format
			pSRC->stATTB.attb |= (MDIN_PRECISION_10|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_SRC_MUX656_8:		mode = (0<<7)|(2<<4);	// mux with sync | src format
			pSRC->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_SRC_SEP656_8:		mode = (1<<7)|(2<<4);	// mux with sync | src format
			pSRC->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_SRC_MUX656_10:	mode = (0<<7)|(2<<4);	// mux with sync | src format
			pSRC->stATTB.attb |= (MDIN_PRECISION_10|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_SRC_SEP656_10:	mode = (1<<7)|(2<<4);	// mux with sync | src format
			pSRC->stATTB.attb |= (MDIN_PRECISION_10|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
	}

	mode |= (MBIT(pSRC->fine,MDIN_HACT_IS_CSYNC))?			(1<<6) : 0;
	mode |= (MBIT(pSRC->stATTB.attb,MDIN_NEGATIVE_HACT))?	(1<<2) : 0;
	mode |= (MBIT(pSRC->stATTB.attb,MDIN_NEGATIVE_VACT))?	(1<<1) : 0;
	mode |= (MBIT(pSRC->stATTB.attb,MDIN_SCANTYPE_PROG))?	(1<<0) : 0;

	// in_sync_ctrl
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x002, 12-nID*8, 4, (mode>>4)&0x0f)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x002,  8-nID*8, 3, (mode>>0)&0x07)) return MDIN_I2C_ERROR;
	
#if YC_SWAP /* Harry, for Y/C swap */
    if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x002,  11-nID*8, 1, 0x1)) return MDIN_I2C_ERROR;
#endif

	// in_fieldid_ctrl
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x001, 0+nID, 1, MBIT(pSRC->fine,MDIN_HIGH_IS_TOPFLD))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x001, 2+nID, 1, MBIT(pSRC->fine,MDIN_FIELDID_INPUT))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x01a-nID*0x019, 4, 12, pSRC->stATTB.Htot)) return MDIN_I2C_ERROR;

	// in_format_ctrl
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x000, 8+nID*3, 3, (pSRC->fine&MDIN_YCOFFSET_MASK))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x000, 2+nID, 1, MBIT(pSRC->fine,MDIN_CbCrSWAP_ON))) return MDIN_I2C_ERROR;

	// main_video_mode_control_reg - dtr_dec_en ==> for 4-CH input mode, 2-HD input mode
	switch (pINFO->dacPATH) {
		case DAC_PATH_AUX_4CH:	case DAC_PATH_AUX_2HD: mode = 3; break;
		default:		 mode = (HI4BIT(LOBYTE(mode))&2)? 1 : 0; break;
	}
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x040, 14-nID*2, 2, mode)) return MDIN_I2C_ERROR;

	// vsync_forced_rising - tenbit_input
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x021-nID, 13, 1, RBIT(pSRC->stATTB.attb,MDIN_PRECISION_8))) return MDIN_I2C_ERROR;

	// in_vact_offset_msb
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x003, 0+nID*8, 7, pSRC->offV>>6)) return MDIN_I2C_ERROR;

	// in_act_offset
	mode = ((pSRC->offV&0x3f)<<10)|(pSRC->offH&0x3ff);
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x005-nID, mode)) return MDIN_I2C_ERROR;

	// in_hact_offset_msb
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x008-nID*2, 13, 3, HIBYTE(pSRC->offH)>>2)) return MDIN_I2C_ERROR;

	// in_size_h
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x008-nID*2, 0, 13, pSRC->stATTB.Hact)) return MDIN_I2C_ERROR;

	// in_size_v
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x009-nID*2, pSRC->stATTB.Vact)) return MDIN_I2C_ERROR;

	// in_wr_ch_mask - in_mch_hdeci2_en ==> for 2-HD input mode
	mode = (pINFO->dacPATH==DAC_PATH_AUX_2HD)? 1 : 0;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x01b,  8, 2, mode)) return MDIN_I2C_ERROR;

	// in_frame_cfg1 - in_channel_en ==> for 4-CH input mode, 2-CH HD mode
	if		(pINFO->dacPATH==DAC_PATH_AUX_2HD)	mode = 0x05;
	else if (pINFO->dacPATH==DAC_PATH_AUX_4CH)	mode = 0x0f;
	else										mode = 0x00;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x01e,  8, 4, mode)) return MDIN_I2C_ERROR;

	// in_frame_cfg1 - sg_mch_en ==> for 4-CH input mode, 2-HD input mode
	if		(pINFO->dacPATH==DAC_PATH_AUX_2HD)	mode = 0x01;
	else if (pINFO->dacPATH==DAC_PATH_AUX_4CH)	mode = 0x01;
	else										mode = 0x00;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x01e,  4, 1, mode)) return MDIN_I2C_ERROR;

	//  vsync_forced_rising - dtr_port_swap_b ==> for 4-CH input mode
//	mode = (pINFO->dacPATH==DAC_PATH_AUX_4CH)? 0 : 0;
//	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x021, 12, 1, mode)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetSrcVideoFrmt(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;

	switch (pINFO->srcPATH) {
		case PATH_MAIN_A_AUX_A: case PATH_MAIN_A_AUX_B: case PATH_MAIN_A_AUX_M:
			memcpy(pSRC, (PBYTE)&pINFO->stSRC_a, sizeof(MDIN_SRCVIDEO_INFO));
			pSRC->stATTB.attb |=  MDIN_USE_INPORT_A; break;
		default:
			memcpy(pSRC, (PBYTE)&pINFO->stSRC_b, sizeof(MDIN_SRCVIDEO_INFO));
			pSRC->stATTB.attb &= ~MDIN_USE_INPORT_A; break;
	}

	// mv_mode_ctrl - main_a_sel
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x040, 3, 1, MBIT(pSRC->stATTB.attb,MDIN_USE_INPORT_A))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x040, 2, 1, 0)) return MDIN_I2C_ERROR;	// fix enable ffc
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x040, 0, 1, 0)) return MDIN_I2C_ERROR;	// fix enable in-buff
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetOutVideoFrmt(PMDIN_VIDEO_INFO pINFO)
{
	WORD mode = 0;
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_m;

	if (pOUT->frmt>=VIDOUT_FORMAT_END) return MDIN_INVALID_FORMAT;

	// set attb[H/V-Polarity,ScanType], H/V size
	memcpy(&pOUT->stATTB, (PBYTE)&defMDINOutVideo[pOUT->frmt], sizeof(MDIN_OUTVIDEO_ATTB));

	// set Quality, System
	pOUT->stATTB.attb &= ~(MDIN_WIDE_RATIO|MDIN_QUALITY_HD|MDIN_VIDEO_SYSTEM);
	switch (pOUT->frmt) {
		case VIDOUT_720x480i60:		case VIDOUT_720x576i50:
		case VIDOUT_720x480p60:		case VIDOUT_720x576p50:
			pOUT->stATTB.attb |= (MDIN_NORM_RATIO|MDIN_QUALITY_SD|MDIN_VIDEO_SYSTEM); break;

		case VIDOUT_1280x720p60:	case VIDOUT_1280x720p50:
			pOUT->stATTB.attb |= (MDIN_WIDE_RATIO|MDIN_QUALITY_HD|MDIN_VIDEO_SYSTEM); break;

		case VIDOUT_1920x1080i60:	case VIDOUT_1920x1080i50:
		case VIDOUT_1920x1080p60:	case VIDOUT_1920x1080p50:
			pOUT->stATTB.attb |= (MDIN_WIDE_RATIO|MDIN_QUALITY_HD|MDIN_VIDEO_SYSTEM); break;

		case VIDOUT_1360x768p60:
		case VIDOUT_1440x900p60:	case VIDOUT_1440x900p75:
		case VIDOUT_1680x1050p60:	case VIDOUT_1680x1050pRB:
		case VIDOUT_1920x1080pRB:	case VIDOUT_1920x1200pRB:
			pOUT->stATTB.attb |= (MDIN_WIDE_RATIO|MDIN_QUALITY_SD|MDIN_PCVGA_SYSTEM); break;

		default:
			pOUT->stATTB.attb |= (MDIN_NORM_RATIO|MDIN_QUALITY_SD|MDIN_PCVGA_SYSTEM); break;
	}

	// set Precision, PixelSize, ColorSpace
	pOUT->stATTB.attb &= ~(MDIN_PRECISION_8|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV);
	switch (pOUT->mode) {
		case MDIN_OUT_RGB444_8:		mode = (1<<8)|(0<<5)|(1<<2)|0;
			pOUT->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_444|MDIN_COLORSPACE_RGB); break;
		case MDIN_OUT_YUV444_8:		mode = (0<<8)|(2<<5)|(1<<2)|0;
			pOUT->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_444|MDIN_COLORSPACE_YUV); break;
		case MDIN_OUT_EMB422_8:		mode = (0<<8)|(3<<5)|(3<<2)|1;
			pOUT->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_OUT_SEP422_8:		mode = (0<<8)|(2<<5)|(1<<2)|1;
			pOUT->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_OUT_EMB422_10:	mode = (0<<8)|(3<<5)|(2<<2)|1;
			pOUT->stATTB.attb |= (MDIN_PRECISION_10|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_OUT_SEP422_10:	mode = (0<<8)|(2<<5)|(0<<2)|1;
			pOUT->stATTB.attb |= (MDIN_PRECISION_10|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_OUT_MUX656_8:		mode = (0<<8)|(3<<5)|(3<<2)|2;
			pOUT->stATTB.attb |= (MDIN_PRECISION_8|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
		case MDIN_OUT_MUX656_10:	mode = (0<<8)|(3<<5)|(2<<2)|2;
			pOUT->stATTB.attb |= (MDIN_PRECISION_10|MDIN_PIXELSIZE_422|MDIN_COLORSPACE_YUV); break;
	}

	// out_control - always embedded sync for hdmi-out
	mode |= (1<<5)|(1<<3)|((pOUT->fine&MDIN_CbCrSWAP_ON)? (1<<4) : 0);	// CbCr-swap
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a2, 0, 10, mode)) return MDIN_I2C_ERROR;

	// out_sync_ctrl
	mode  = (pOUT->fine&MDIN_DEOUT_IS_HACT);	// DE mode
	mode |= (pOUT->fine&(MDIN_HSYNC_IS_HACT|MDIN_VSYNC_IS_VACT))>>12;	// H/V sync mode
	mode |= (pOUT->fine&MDIN_POSITIVE_DE)?			 (1<<7) : 0;		// DE polarity
	mode |= (pOUT->stATTB.attb&MDIN_POSITIVE_HSYNC)? (1<<5) : 0;		// HSYNC polarity
	mode |= (pOUT->stATTB.attb&MDIN_POSITIVE_VSYNC)? (1<<4) : 0;		// VSYNC polarity
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a0, 0, 10, mode)) return MDIN_I2C_ERROR;

	// dac_control
	mode  = (pOUT->fine&MDIN_PbPrSWAP_ON)?			 (1<<4) : 0;	// PbPr-swap
	mode |= (pOUT->stATTB.attb&MDIN_QUALITY_HD)?	 (1<<1) : 0;	// quality
	mode |= (pOUT->stATTB.attb&MDIN_COLORSPACE_YUV)? (5<<0) : 0;	// color space
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x0a3, mode)) return MDIN_I2C_ERROR;

	// digital_blank_data
	mode  = (pOUT->stATTB.attb&MDIN_COLORSPACE_YUV)? 0x80 : 0x00;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x0a4, mode)) return MDIN_I2C_ERROR;

	// out_mux_ctrl - hdmi_src_sel ==> for 4-CH input mode, 2-HD input mode
	if		(pINFO->dacPATH==DAC_PATH_AUX_4CH)	mode = 2;
	else if (pINFO->dacPATH==DAC_PATH_AUX_2HD)	mode = 2;
	else										mode = 1;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a5, 8, 2, mode)) return MDIN_I2C_ERROR;

	// out_mux_ctrl - osd_on_data_to_aux
	mode = (pINFO->dspFLAG&MDIN_AUX_MAINOSD_ON)? 1 : 0;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a5, 7, 1, mode)) return MDIN_I2C_ERROR;

	// out_mux_ctrl - hdmi_data_mode
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a5, 5, 1, 1)) return MDIN_I2C_ERROR;	// fix 30bit
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetSrcVideoCSC(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_m;
	MDIN_CSCCTRL_INFO	stCSC, *pCSC;

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
	stCSC.ctrl = (pSRC->stATTB.attb&MDIN_COLORSPACE_YUV)? 0x01fc : 0x01f8;

	// use user-defined CSC if exist
	if (pSRC->pCSC!=NULL) memcpy(&stCSC, (PBYTE)pSRC->pCSC, sizeof(MDIN_CSCCTRL_INFO));

	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x00a, (PBYTE)&stCSC, sizeof(MDIN_CSCCTRL_INFO))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetOutVideoCSC(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_m;

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
	stCSC.in[0]	  = CLIP12((((SHORT)stCSC.in[0])+(brightness-128)*2))&0xfff;

#if RGB_GAIN_OFFSET_TUNE == 1
	if ((pOUT->stATTB.attb&MDIN_COLORSPACE_YUV)==0) {	// only apply for RGB output
		stCSC.coef[0] = CLIP12((((coef[0]*contrast*(LONG)pOUT->g_gain)>>14)))&0xfff;
		stCSC.coef[3] = CLIP12((((coef[0]*contrast*(LONG)pOUT->b_gain)>>14)))&0xfff;
		stCSC.coef[6] = CLIP12((((coef[0]*contrast*(LONG)pOUT->r_gain)>>14)))&0xfff;
		stCSC.out[0]  = CLIP12((((SHORT)pOUT->g_offset)-128)*2+stCSC.out[0])&0xfff;
		stCSC.out[1]  = CLIP12((((SHORT)pOUT->b_offset)-128)*2+stCSC.out[1])&0xfff;
		stCSC.out[2]  = CLIP12((((SHORT)pOUT->r_offset)-128)*2+stCSC.out[2])&0xfff;
	}
#endif

	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x072, (PBYTE)&stCSC, sizeof(MDIN_CSCCTRL_INFO))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetOutVideoDAC(PMDIN_VIDEO_INFO pINFO)
{
	WORD i, mode; PMDIN_DACSYNC_CTRL pDAC;
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_m;

	if (pOUT->frmt>=VIDOUT_FORMAT_END) return MDIN_INVALID_FORMAT;

	// DAC sync control for YUV
	pDAC = (PMDIN_DACSYNC_CTRL)&defMDINDACData[pOUT->frmt];
	if (pOUT->pDAC!=NULL) pDAC = pOUT->pDAC;	// use user-defined DAC if exist

	if ((pOUT->stATTB.attb&MDIN_COLORSPACE_YUV)==0) pDAC = NULL;
	if ((pOUT->stATTB.attb&MDIN_VIDEO_SYSTEM)==0) pDAC = NULL;

	for (i=0; i<sizeof(MDIN_DACSYNC_CTRL)/2-1; i++) {	// because of dummy data
		mode = (pDAC==NULL)? 0 : pDAC->ctrl[i];
		if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x0ae, mode)) return MDIN_I2C_ERROR;
		if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x0af, 0x0080|i)) return MDIN_I2C_ERROR;
	}
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetOutVideoSYNC(PMDIN_VIDEO_INFO pINFO)
{
	WORD mode;	MDIN_OUTVIDEO_SYNC stSYNC, *pSYNC;
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_m;
	PMDIN_OUTVIDEO_INFO pAUX = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_x;

	pSYNC = (PMDIN_OUTVIDEO_SYNC)&defMDINOutSync[pOUT->frmt];
	if (pOUT->pSYNC!=NULL) pSYNC = pOUT->pSYNC;	// use user-defined SYNC if exist

	memcpy(&stSYNC, (PBYTE)pSYNC, sizeof(MDIN_OUTVIDEO_SYNC));
	stSYNC.posFLD |= (pOUT->stATTB.attb&MDIN_SCANTYPE_PROG)? 0 : (1<<15);	// interlace out

	// MUX656 format
	if (pOUT->mode==MDIN_OUT_MUX656_8||pOUT->mode==MDIN_OUT_MUX656_10) {
		stSYNC.totHS = stSYNC.totHS*2;	stSYNC.vclkS = stSYNC.vclkS/2;
		stSYNC.bgnHA = stSYNC.bgnHA*2;	stSYNC.endHA = stSYNC.endHA*2;
		stSYNC.bgnHS = stSYNC.bgnHS*2;	stSYNC.endHS = stSYNC.endHS*2;
	}

	if (pINFO->dacPATH==DAC_PATH_AUX_4CH) {		// for 4-CH input mode
		memcpy(&stSYNC.vclkP, (PBYTE)&defMDINOutSync[pAUX->frmt].vclkP, 6);
		if (pAUX->frmt==VIDOUT_1920x1080p60) stSYNC.totHS = 1082;
		if (pAUX->frmt==VIDOUT_1920x1080p50) stSYNC.totHS = 1090;
	}

	if (pINFO->dacPATH==DAC_PATH_AUX_2HD) {		// for 2-HD input mode
		stSYNC.totHS /= 2; stSYNC.bgnHA /= 2; stSYNC.endHA /= 2;
	}

	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x084, (PBYTE)&stSYNC.posHD,  6)) return MDIN_I2C_ERROR;
	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x088, (PBYTE)&stSYNC.totHS, 30)) return MDIN_I2C_ERROR;

	// frame_ptr_ctrl - main_skip_dis
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x083, 6, 1, 1)) return MDIN_I2C_ERROR;	// fix main_skip_dis

	// output_sync_reset_dly
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x097, 2, 2, 1)) return MDIN_I2C_ERROR;	// fix vact_ipc_lead

	// sync_ctrl
	mode = (1<<15)|(10<<11)|MBIT(pOUT->fine,MDIN_SYNC_FREERUN);
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x098, mode)) return MDIN_I2C_ERROR;	// free-run

	// bg_color_y, cbcr
//	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x09a, 0x0000)) return MDIN_I2C_ERROR;	// fix black
//	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x09b, 0x8080)) return MDIN_I2C_ERROR;

	// set main video clock
	if (MDIN3xx_SetVideoPLL(stSYNC.vclkP, stSYNC.vclkM, stSYNC.vclkS)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static BOOL MDIN3xx_IsCROP(PMDIN_VIDEO_INFO pINFO)
{
	return (pINFO->stCROP_m.w==0||pINFO->stCROP_m.h==0)? FALSE : TRUE;
}

//--------------------------------------------------------------------------------------------------------------------------
static BOOL MDIN3xx_IsZOOM(PMDIN_VIDEO_INFO pINFO)
{
	return (pINFO->stZOOM_m.w==0||pINFO->stZOOM_m.h==0)? FALSE : TRUE;
}

//--------------------------------------------------------------------------------------------------------------------------
static BOOL MDIN3xx_IsVIEW(PMDIN_VIDEO_INFO pINFO)
{
	return (pINFO->stVIEW_m.w==0||pINFO->stVIEW_m.h==0)? FALSE : TRUE;
}

//--------------------------------------------------------------------------------------------------------------------------
static BOOL MDIN3xx_IsFFCSize(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_MFCSCALE_INFO	pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_m;

	return (pINFO->ffcH_m==0||pINFO->ffcH_m>pMFC->stFFC.dw)? FALSE : TRUE;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetMFCScaleWind(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_m;
	PMDIN_MFCSCALE_INFO	pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_m;
	WORD mode, nID = (pSRC->stATTB.attb&MDIN_USE_INPORT_A)? 1 : 0;

	memset(&pINFO->stMFC_m, 0, sizeof(MDIN_MFCSCALE_INFO));	// clear

	// config clip_window
	pMFC->stCUT.w  = pSRC->stATTB.Hact;	pMFC->stCUT.h  = pSRC->stATTB.Vact;
	if (MDIN3xx_IsCROP(pINFO)) memcpy(&pMFC->stCUT, &pINFO->stCROP_m, 8);
	pMFC->stCUT.x += pSRC->offH;		pMFC->stCUT.y += pSRC->offV;

	// config dst_size_h/v, dst_posi_h/v
	pMFC->stDST.w  = pOUT->stATTB.Hact;	pMFC->stDST.h  = pOUT->stATTB.Vact;
	if (MDIN3xx_IsVIEW(pINFO)) memcpy(&pMFC->stDST, &pINFO->stVIEW_m, 8);

	// for 2-HD input mode
	if (pINFO->dacPATH==DAC_PATH_AUX_2HD) pMFC->stDST.w /= 2;

	// config src_size_h2/v2, dst_size_h2/v2
	pMFC->stFFC.sw = pMFC->stCUT.w;		pMFC->stFFC.sh = pMFC->stCUT.h;
	pMFC->stFFC.dw = pMFC->stCUT.w;		pMFC->stFFC.dh = pMFC->stCUT.h;
	if (pMFC->stCUT.w>pMFC->stDST.w)	pMFC->stFFC.dw = pMFC->stDST.w; // H-down

	// check size of front format conversion
	if (MDIN3xx_IsFFCSize(pINFO))		pMFC->stFFC.dw = pINFO->ffcH_m;	// H-ffc

	// config mem_size_h, mem_size_v
	pMFC->stMEM.w  = pMFC->stFFC.dw;	pMFC->stMEM.h  = pMFC->stFFC.dh;

	// config src_size_h/v, src_posi_h/v
	pMFC->stSRC.w  = pMFC->stFFC.dw;	pMFC->stSRC.h  = pMFC->stFFC.dh;
	if (MDIN3xx_IsZOOM(pINFO)) memcpy(&pMFC->stSRC, &pINFO->stZOOM_m, 8);

	// config src_size_h/v_cr, dst_size_h/v_cr
	pMFC->stCRS.sw = pMFC->stSRC.w;		pMFC->stCRS.sh = pMFC->stSRC.h;
	pMFC->stCRS.dw = pMFC->stDST.w;		pMFC->stCRS.dh = pMFC->stDST.h;

	// config src_size_h/v_or, dst_size_h/v_or
	pMFC->stORS.sw = pMFC->stSRC.w;		pMFC->stORS.sh = pMFC->stSRC.h;
	pMFC->stORS.dw = pMFC->stDST.w;		pMFC->stORS.dh = pMFC->stDST.h;

#if __MDIN3xx_DBGPRT__ == 1
	UARTprintf("[MAIN] sCUT.w=%d, sCUT.h=%d\n", pMFC->stCUT.w, pMFC->stCUT.h);
	UARTprintf("[MAIN] CROP.w=%d, CROP.h=%d\n", pINFO->stCROP_m.w, pINFO->stCROP_m.h);
	UARTprintf("[MAIN] sDST.w=%d, sDST.h=%d\n", pMFC->stDST.w, pMFC->stDST.h);
	UARTprintf("[MAIN] VIEW.w=%d, VIEW.h=%d\n", pINFO->stVIEW_m.w, pINFO->stVIEW_m.h);
	UARTprintf("[MAIN] sMEM.w=%d, sMEM.h=%d\n", pMFC->stMEM.w, pMFC->stMEM.h);
	UARTprintf("[MAIN] sSRC.w=%d, sSRC.h=%d\n", pMFC->stSRC.w, pMFC->stSRC.h);
	UARTprintf("[MAIN] ZOOM.w=%d, ZOOM.h=%d\n", pINFO->stZOOM_m.w, pINFO->stZOOM_m.h);
#endif

	// in_vact_offset_msb
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x003, 0+nID*8, 7, pMFC->stCUT.y>>6)) return MDIN_I2C_ERROR;

	// in_act_offset
	mode = ((pMFC->stCUT.y&0x3f)<<10)|(pMFC->stCUT.x&0x3ff);
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x005-nID, mode)) return MDIN_I2C_ERROR;

	// in_size_h
	mode = ((pMFC->stCUT.x&0xfc00)<<3)|pMFC->stCUT.w;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x008-nID*2, mode)) return MDIN_I2C_ERROR;

	// in_size_v
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x009-nID*2, pMFC->stCUT.h)) return MDIN_I2C_ERROR;

	// src_size_h/v, src_posi_h/v
	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x032, (PBYTE)&pMFC->stSRC, sizeof(MDIN_VIDEO_WINDOW))) return MDIN_I2C_ERROR;

	// win_size_h/v, win_posi_h/v
	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x036, (PBYTE)&pMFC->stDST, sizeof(MDIN_VIDEO_WINDOW))) return MDIN_I2C_ERROR;

	// src_size_h2
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x03a, pMFC->stFFC.sw)) return MDIN_I2C_ERROR;

	// dst_size_h2
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x03c, pMFC->stFFC.dw)) return MDIN_I2C_ERROR;

	// mem_size_v
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x03d, pMFC->stFFC.dh)) return MDIN_I2C_ERROR;

	// src_size_h/v_cr, dst_size_h/v_cr for uniform mode
	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x108, (PBYTE)&pMFC->stCRS, sizeof(MDIN_SCALE_FACTOR))) return MDIN_I2C_ERROR;

	// src_size_h/v_or, dst_size_h/v_or for non-uniform mode
	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x10c, (PBYTE)&pMFC->stORS, sizeof(MDIN_SCALE_FACTOR))) return MDIN_I2C_ERROR;

	// mvf_count_value - mvf_count_value_mo
	mode = ((pMFC->stSRC.w%384)%12)? (pMFC->stSRC.w%384)/12 : (pMFC->stSRC.w%384)/12-1;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x206,  8, 5, MAX(mode,8)&0x1f)) return MDIN_I2C_ERROR;

	// mvf_count_value - mvf_count_value_data
	mode = ((pMFC->stSRC.w%192)%6)? (pMFC->stSRC.w%192)/6 : (pMFC->stSRC.w%192)/6-1;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x206,  0, 5, MAX(mode,8)&0x1f)) return MDIN_I2C_ERROR;

	// if src is interlace, stMEM.h/2. if dst is interlace, stDST.h/2
	if ((pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)==0) pMFC->stMEM.h /= 2;
	if ((pOUT->stATTB.attb&MDIN_SCANTYPE_PROG)==0) pMFC->stDST.h /= 2;

	// MUX656 format - win_size_h, dst_size_h_cr, dst_size_h_or
	if (pOUT->mode!=MDIN_OUT_MUX656_8&&pOUT->mode!=MDIN_OUT_MUX656_10) return MDIN_NO_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x036, 2*pMFC->stDST.w )) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x10a, 2*pMFC->stCRS.dw)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x10e, 2*pMFC->stORS.dw)) return MDIN_I2C_ERROR;

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_GetMFCFilterDone(void)
{
/*
	WORD rVal = 0, count = 100;

	if (MDINDLY_10uSec(5)) return MDIN_I2C_ERROR;	// delay 50us

	while (count&&(rVal==0)) {
		if (MDINHIF_RegRead(MDIN_LOCAL_ID, 0x13f, &rVal)) return MDIN_I2C_ERROR;
		rVal &= 0x40;	count--;
	}

#if __MDIN3xx_DBGPRT__ == 1
	if (count==0) UARTprintf("MDIN3xx_GetMFCFilterDone() TimeOut Error!!!\n");
#endif

	return (count)? MDIN_NO_ERROR : MDIN_TIMEOUT_ERROR;
*/
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetMFCFilterCoef(PMDIN_VIDEO_INFO pINFO, WORD nID)
{
	WORD i, err, buff[5] = {0,0,0,0,0};	PMDIN_MFCFILT_COEF pCoef;

	switch (nID) {
		case MDIN_MFCFILT_HY: pCoef = (PMDIN_MFCFILT_COEF)&MDIN_MFCFilter_Spline_0_50;
					if (pINFO->pHY_m) pCoef = pINFO->pHY_m; break;	// user-defined HY
		case MDIN_MFCFILT_HC: pCoef = (PMDIN_MFCFILT_COEF)&MDIN_MFCFilter_Spline_0_50;
					if (pINFO->pHC_m) pCoef = pINFO->pHC_m; break;	// user-defined HC
		case MDIN_MFCFILT_VY: pCoef = (PMDIN_MFCFILT_COEF)&MDIN_MFCFilter_Spline_0_50;
					if (pINFO->pVY_m) pCoef = pINFO->pVY_m; break;	// user-defined VY
		default: 			  pCoef = (PMDIN_MFCFILT_COEF)&MDIN_MFCFilter_Spline_0_50;
					if (pINFO->pVC_m) pCoef = pINFO->pVC_m; break;	// user-defined VC
	}

	// for 8-bit I2C operation
//	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x13f, MAKEWORD(nID,0))) return MDIN_I2C_ERROR;
//	if (MDIN3xx_GetMFCFilterDone()) {mdinERR = 6; return MDIN_TIMEOUT_ERROR;}

	for (i=0; i<sizeof(MDIN_MFCFILT_COEF)/4; i++) {			// for 2-word write
		memcpy(buff, ((PWORD)pCoef)+i*2, 4);	buff[4] = MAKEWORD(nID,(0x80|i));

#if defined(SYSTEM_USE_MDIN380)&&defined(SYSTEM_USE_PCI_HIF)
		if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x13b, buff[0])) return MDIN_I2C_ERROR;
		if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x13c, buff[1])) return MDIN_I2C_ERROR;
		if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x13d, buff[2])) return MDIN_I2C_ERROR;
		if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x13e, buff[3])) return MDIN_I2C_ERROR;
		if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x13f, buff[4])) return MDIN_I2C_ERROR;
#else
		if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x13b, (PBYTE)buff, 10)) return MDIN_I2C_ERROR;
#endif

		err = MDIN3xx_GetMFCFilterDone(); if (err) break;	// check done flag
	}
	if (err==MDIN_TIMEOUT_ERROR) mdinERR = 6;
	return (MDIN_ERROR_t)err;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetMFCScaleCtrl(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_MFCSCALE_INFO	pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_m;
	WORD mode = (pMFC->stFFC.sh>=pMFC->stDST.h*2)? (3<<2)|3 : (2<<2)|2;

	// mfc_control1 - HY(low), HC(low), VY(dual), VC(low)
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x100, 8, 8, (2<<6)|(2<<4)|mode)) return MDIN_I2C_ERROR;
//	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x100, 7, 1, 1)) return MDIN_I2C_ERROR;		// fix blank update
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x100, 7, 1, 0)) return MDIN_I2C_ERROR;		// fix immediately
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x100, 2, 1, 0)) return MDIN_I2C_ERROR;		// enable MFC filter

	// if src is interlace & scale ratio <= 0.5 , then use bilinear filter
	mode = ((pMFC->stFFC.dh>pMFC->stMEM.h)&&((mode&3)==3))? 1 : 0;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x100, 1, 1, mode)) return MDIN_I2C_ERROR;

	// mfc coefficient
	if (pINFO->pHY_m==NULL&&pINFO->pHC_m==NULL&&pINFO->pVY_m==NULL&&pINFO->pVC_m==NULL) return MDIN_NO_ERROR;
	if (MDIN3xx_SetMFCFilterCoef(pINFO, MDIN_MFCFILT_HY)) return MDIN_I2C_ERROR;
	if (MDIN3xx_SetMFCFilterCoef(pINFO, MDIN_MFCFILT_HC)) return MDIN_I2C_ERROR;
	if (MDIN3xx_SetMFCFilterCoef(pINFO, MDIN_MFCFILT_VY)) return MDIN_I2C_ERROR;
	if (MDIN3xx_SetMFCFilterCoef(pINFO, MDIN_MFCFILT_VC)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_Set10BITProcess(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	PMDIN_MFCSCALE_INFO pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_m;

	// set 10-bit processing
	if (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG) pSRC->stATTB.attb &= ~MDIN_PRECISION_8;
	if (pINFO->dacPATH==DAC_PATH_AUX_2HD) pSRC->stATTB.attb |= MDIN_PRECISION_8;	// for 2-HD input mode
	if (pMFC->stMEM.w>1536) pSRC->stATTB.attb |= MDIN_PRECISION_8;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x040,  8, 1, RBIT(pSRC->stATTB.attb,MDIN_PRECISION_8))) return MDIN_I2C_ERROR;

	// common_mode_at_IPC - mvfw_count_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x205,  8, 1, MBIT(pSRC->stATTB.attb,MDIN_PRECISION_8))) return MDIN_I2C_ERROR;

	// internal_mode_set_1 - mvfr_count_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x208, 14, 1, MBIT(pSRC->stATTB.attb,MDIN_PRECISION_8))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetFFCNRProcess(PMDIN_VIDEO_INFO pINFO)
{
	WORD nID;	PMDIN_FNRFILT_COEF pCoef;
//	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	PMDIN_MFCSCALE_INFO pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_m;

	if 		(pMFC->stFFC.sw<=pMFC->stFFC.dw)			nID = 0;
	else if (pMFC->stFFC.dw>=1024&&pMFC->stFFC.dw<1440)	nID = 1;
	else if (pMFC->stFFC.dw>= 720&&pMFC->stFFC.dw<1024)	nID = 2;
	else if (pMFC->stFFC.dw>= 640&&pMFC->stFFC.dw< 720)	nID = 3;
	else if (pMFC->stFFC.dw>= 480&&pMFC->stFFC.dw< 640)	nID = 4;
	else												nID = 0;

//	Current version of API support only for 1920x1080i/p input
	if (pMFC->stFFC.sw!=1920) nID = 0;

	pCoef = (PMDIN_FNRFILT_COEF)&MDIN_FrontNRFilter_Default[nID];
	if (MDIN3xx_SetFrontNRFilterCoef(pCoef)) return MDIN_I2C_ERROR;
	return MDIN3xx_EnableFrontNRFilter((nID)? ON : OFF);
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetIPCCtrlFlags(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	PMDIN_DEINTCTL_INFO pIPC = (PMDIN_DEINTCTL_INFO)&pINFO->stIPC_m;
	PMDIN_MFCSCALE_INFO pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_m;

	// set ipc-proc flag, if input is interlace
	if (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)
		 pIPC->attb &= ~MDIN_DEINT_IPC_PROC;
	else pIPC->attb |=  MDIN_DEINT_IPC_PROC;

	// set pal-ccs flag, if input is interlace and 50Hz
	if (pSRC->frmt==VIDSRC_720x576i50||pSRC->frmt==VIDSRC_1920x1080i50)
		 pIPC->attb |=  MDIN_DEINT_PAL_CCS;
	else pIPC->attb &= ~MDIN_DEINT_PAL_CCS;

	// set ipc-2tab flag, if scale ratio < 0.5
	if (pMFC->stSRC.h>pMFC->stDST.h*2)
		 pIPC->attb |=  MDIN_DEINT_IPC_2TAB;
	else pIPC->attb &= ~MDIN_DEINT_IPC_2TAB;

	// set p5-scale flag, if scale ratio <= 0.5
	if (pMFC->stSRC.h>=pMFC->stDST.h*2)
		 pIPC->attb |=  MDIN_DEINT_p5_SCALE;
	else pIPC->attb &= ~MDIN_DEINT_p5_SCALE;

	// set dn-scale flag, if scale ratio < 1.0
	if (pMFC->stSRC.h>pMFC->stDST.h)
		 pIPC->attb |=  MDIN_DEINT_DN_SCALE;
	else pIPC->attb &= ~MDIN_DEINT_DN_SCALE;

	// set hd-1080i flag, if source is 1080i
	if (pSRC->frmt==VIDSRC_1920x1080i60||pSRC->frmt==VIDSRC_1920x1080i50)
		 pIPC->attb |=  MDIN_DEINT_HD_1080i;
	else pIPC->attb &= ~MDIN_DEINT_HD_1080i;

	// set hd-1080p flag, if source is 1080p
	if (pSRC->frmt==VIDSRC_1920x1080p60||pSRC->frmt==VIDSRC_1920x1080p50)
		 pIPC->attb |=  MDIN_DEINT_HD_1080p;
	else pIPC->attb &= ~MDIN_DEINT_HD_1080p;

	// set mfc-buff flag, if ipc on & src_v <= dst_v
	if (pIPC->attb&MDIN_DEINT_IPC_PROC&&pMFC->stSRC.h<=pMFC->stDST.h)
		 pIPC->attb |=  MDIN_DEINT_MFC_BUFF;
	else pIPC->attb &= ~MDIN_DEINT_MFC_BUFF;

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetDeinterCtrl(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_DEINTCTL_INFO pIPC = (PMDIN_DEINTCTL_INFO)&pINFO->stIPC_m;
	PMDIN_MFCSCALE_INFO	pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_m;

	// main_video_mode_control_reg - mfc_buffer_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x040, 6, 1, MBIT(pIPC->attb,MDIN_DEINT_MFC_BUFF))) return MDIN_I2C_ERROR;

	// mfc_control2 - fix BNR filtering only
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x101, 0, 3, (1<<1)|MBIT(pIPC->attb,MDIN_DEINT_MFC_BUFF))) return MDIN_I2C_ERROR;

	// common_mode_at_IPC - v_posi_half_en, v_size_half_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x205, 7, 1, MBIT(pIPC->attb,MDIN_DEINT_IPC_PROC))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x205, 6, 1, MBIT(pIPC->attb,MDIN_DEINT_IPC_PROC))) return MDIN_I2C_ERROR;

	// common_mode_at_IPC - deinterlace_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x205, 4, 1, MBIT(pIPC->attb,MDIN_DEINT_IPC_PROC))) return MDIN_I2C_ERROR;

	// common_mode_at_IPC - ycbcr444_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x205, 1, 1, MBIT(pIPC->attb,MDIN_DEINT_444_PROC))) return MDIN_I2C_ERROR;

	// internal_mode_set_1 - ipc_2tab_mode
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x208, 13, 1, MBIT(pIPC->attb,MDIN_DEINT_IPC_2TAB))) return MDIN_I2C_ERROR;

	// internal_mode_set_1 - block_match_en, block_match_adp_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x208, 11, 1, RBIT(pIPC->attb,MDIN_DEINT_IPC_PROC))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x208, 10, 1, RBIT(pIPC->attb,MDIN_DEINT_IPC_PROC))) return MDIN_I2C_ERROR;

	// 8bit_motion_thres - deint_thres
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x211, 8, 8, 18)) return MDIN_I2C_ERROR; // fix deint_thres

	// caption_v_posi
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x251, pMFC->stMEM.h*3/4)) return MDIN_I2C_ERROR;

	// c_dein_mode, c_caption_mode
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x249, 12, 2, (pIPC->attb&MDIN_DEINT_p5_SCALE)? 2 : 1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x252,  0, 2, (pIPC->attb&MDIN_DEINT_p5_SCALE)? 1 : 3)) return MDIN_I2C_ERROR;

	// nr_scene_change - nr_scene_change_detect_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x26d, 7, 1, MBIT(pIPC->attb,MDIN_DEINT_IPC_PROC))) return MDIN_I2C_ERROR;

	if (pIPC->attb&MDIN_DEINT_HD_1080i) {	// c_dein_mode is intra only mode
		if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x249, 12, 2, 2)) return MDIN_I2C_ERROR;
		if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x252,  0, 2, 1)) return MDIN_I2C_ERROR;
	}

	// film22_be_ctrl_reg2
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x258, 0, 8, (pIPC->attb&MDIN_DEINT_HD_1080i)? 24 : 20)) return MDIN_I2C_ERROR;

	// film22_be_ctrl_reg3
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x25a, 0, 8, (pIPC->attb&MDIN_DEINT_HD_1080i)? 30 : 40)) return MDIN_I2C_ERROR;

	if (MDIN3xx_SetDeintMode(pINFO, (MDIN_DEINT_MODE_t)pIPC->mode)) return MDIN_I2C_ERROR;
	if (MDIN3xx_SetDeintFilm(pINFO, (MDIN_DEINT_FILM_t)pIPC->film)) return MDIN_I2C_ERROR;
	if (MDIN3xx_SetDeint3DNRGain(pINFO, (BYTE)pIPC->gain)) return MDIN_I2C_ERROR;
	if (MDIN3xx_EnableDeint3DNR(pINFO, MBIT(pIPC->fine,MDIN_DEINT_3DNR_ON))) return MDIN_I2C_ERROR;
	if (MDIN3xx_EnableDeintCCS(pINFO, MBIT(pIPC->fine,MDIN_DEINT_CCS_ON))) return MDIN_I2C_ERROR;
	if (MDIN3xx_EnableDeintCUE(pINFO, MBIT(pIPC->fine,MDIN_DEINT_CUE_ON))) return MDIN_I2C_ERROR;

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetMemoryMap(PMDIN_VIDEO_INFO pINFO, BYTE nID, BYTE num, WORD addr)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	PMDIN_MFCSCALE_INFO	pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_m;
	WORD bpp, col, mode, bpc;	DWORD cpl, rpf;

	col = (GetMEM&MDIN_COLUMN_7BIT)? 128 : 256;
	bpp = ((nID%4)==2||pSRC->stATTB.attb&MDIN_PRECISION_8)?  8 : 10;
	bpc = ((nID%4)==2||pSRC->stATTB.attb&MDIN_PRECISION_8)? 64 : 60;

	if ((nID%4)==1) bpp /= 2;	// case Ymh
	if ((nID%4)==2) pMFC = &pINFO->stMFC_x;	// case Y_x, C_x

	cpl = bpp*pMFC->stMEM.w; cpl = (cpl/bpc) + ((cpl%bpc)? 1 : 0);
	rpf = cpl*pMFC->stMEM.h; rpf = (rpf/col) + ((rpf%col)? 1 : 0);
	rpf = (rpf/2) + (rpf%2);

#if __MDIN3xx_DBGPRT__ == 1
	UARTprintf("used FB cpl is %d, rpf is %d\n", cpl, rpf);
#endif

	if (pINFO->dacPATH==DAC_PATH_AUX_4CH) num *= 4;	// for 4-CH input mode
	if (pINFO->dacPATH==DAC_PATH_AUX_2HD) num *= 3;	// for 2-HD input mode
	GetROW = LOWORD(rpf)*num;	fbADDR = addr + GetROW;

	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1c1+nID, addr)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1c9+(nID%4), LOWORD(rpf))) return MDIN_I2C_ERROR;

	mode = ((bpp==8)? 0 : ((bpp==10)? 1 : ((bpp==4)? 2 : 3)))<<10 | LOWORD(cpl);
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1cd+(nID%4), mode)) return MDIN_I2C_ERROR;

#if __MDIN3xx_DBGPRT__ == 1
	switch (nID) {
		case 0: UARTprintf("used FB[Y_m+Ynr] row is %d, GetROW=%d\n", fbADDR, GetROW); break;
		case 1: UARTprintf("used FB[Ymh] row is %d, GetROW=%d\n", fbADDR, GetROW); break;
		case 2: UARTprintf("used FB[Y_x] row is %d, GetROW=%d\n", fbADDR, GetROW); break;
		case 4: UARTprintf("used FB[C_m] row is %d, GetROW=%d\n", fbADDR, GetROW); break;
		case 6: UARTprintf("used FB[C_x] row is %d, GetROW=%d\n", fbADDR, GetROW); break;
	}
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetFrameBuffer(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	PMDIN_FRAMEMAP_INFO pMAP = (PMDIN_FRAMEMAP_INFO)&pINFO->stMAP_m;
	WORD numY, numC, mode, addr = 0;

	if (pMAP->frmt>=MDIN_MAP_FORMAT_END) return MDIN_INVALID_FORMAT;

	// numY is [frameY|delayY], numC is [frameC|delayC]
	switch (pMAP->frmt) {
		case MDIN_MAP_AUX_ON_NR_ON:
			numY = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0x31 : 0x62;
			numC = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0x31 : 0x62;
			pMAP->Y_m = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 3 : 6;
			pMAP->Ynr = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 1 : 2;
			pMAP->C_m = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 3 : 6;
			pMAP->Ymh = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 1 : 2;
			pMAP->Y_x = 2; pMAP->C_x = 2; break;

		case MDIN_MAP_AUX_ON_NR_OFF:
			numY = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0x20 : 0x62;
			numC = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0x20 : 0x62;
			pMAP->Y_m = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 2 : 6;
			pMAP->Ynr = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0 : 0;
			pMAP->C_m = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 2 : 6;
			pMAP->Ymh = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0 : 2;
			pMAP->Y_x = 2; pMAP->C_x = 2; break;

		case MDIN_MAP_AUX_OFF_NR_ON:
			numY = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0x31 : 0x62;
			numC = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0x31 : 0x62;
			pMAP->Y_m = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 3 : 6;
			pMAP->Ynr = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 1 : 2;
			pMAP->C_m = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 3 : 6;
			pMAP->Ymh = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 1 : 2;
			pMAP->Y_x = 0; pMAP->C_x = 0; break;

		case MDIN_MAP_AUX_OFF_NR_OFF:
			numY = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0x20 : 0x62;
			numC = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0x20 : 0x62;
			pMAP->Y_m = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 2 : 6;
			pMAP->Ynr = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0 : 0;
			pMAP->C_m = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 2 : 6;
			pMAP->Ymh = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0 : 2;
			pMAP->Y_x = 0; pMAP->C_x = 0; break;

		default:
			numY = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0x31 : 0x62;
			numC = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0x31 : 0x62;
			break;
	}

	// for 4-CH input mode, 2-HD input mode
	switch (pINFO->dacPATH) {
		case DAC_PATH_AUX_4CH:
			numY = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0x31 : 0x63;
			numC = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0x31 : 0x63;
			pMAP->Y_m = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 3 : 6;
			pMAP->Ynr = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 1 : 2;
			pMAP->C_m = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 3 : 6;
			pMAP->Ymh = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 1 : 2;
			pMAP->Y_x = 2; pMAP->C_x = 2; break;

		case DAC_PATH_AUX_2HD:
			numY = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0x31 : 0x62;
			numC = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0x31 : 0x62;
			pMAP->Y_m = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 3 : 6;
			pMAP->Ynr = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0 : 0;
			pMAP->C_m = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 3 : 6;
			pMAP->Ymh = (pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)? 0 : 2;
			pMAP->Y_x = 2; pMAP->C_x = 2; break;
	}

	// if src is interlace and SD quality ==> for PAL-CCS operation
	if ((pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)==0&&(pSRC->stATTB.attb&MDIN_QUALITY_HD)==0) {
		pMAP->C_m = 8; numC = (numC&0x0f)|(8<<4);
	}

#if __MDIN3xx_DBGPRT__ == 1
	UARTprintf("numY=0x%02X numC=0x%02X Y_m=%d Ynr=%d C_m=%d Ymh=%d Y_x=%d C_x=%d\n",
		numY, numC, pMAP->Y_m, pMAP->Ynr, pMAP->C_m, pMAP->Ymh, pMAP->Y_x, pMAP->C_x);
#endif

	// in_frame_cfg1 - in_frame_config
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x01e, 0, 2, 3)) return MDIN_I2C_ERROR;	// fix manu mode

	// in_frame_cfg2
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x01f, MAKEWORD(numY,numC))) return MDIN_I2C_ERROR;

	// internal_mode_set_2 - y_num_frame, c_num_frame
	mode = MAKEBYTE(HI4BIT(LOBYTE(numY)),HI4BIT(LOBYTE(numC)));
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x209, 0, 8, mode)) return MDIN_I2C_ERROR;

	// aux_frame_ptr_ctrl - aux_frame_config, aux_num_frames, aux_frame_delay
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x144, 11, 5, 0)) return MDIN_I2C_ERROR; // fix auto mode

	// mem_config
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1c0, 12, 4, (1<<2))) return MDIN_I2C_ERROR;

	// map of video data frame (Y_m+Ynr)
	pMAP->Y_m += pMAP->Ynr;					// fbuf0y
	if (MDIN3xx_SetMemoryMap(pINFO, 0+0, pMAP->Y_m, addr)) return MDIN_I2C_ERROR;

	// map of video data frame (C_m)
	addr += GetROW;							// fbuf0c
	if (MDIN3xx_SetMemoryMap(pINFO, 4+0, pMAP->C_m, addr)) return MDIN_I2C_ERROR;

	// map of video data frame (Ymh)
	addr += GetROW;							// fbuf1y
	if (MDIN3xx_SetMemoryMap(pINFO, 0+1, pMAP->Ymh, addr)) return MDIN_I2C_ERROR;

	// map of video data frame (Y_x)
	addr += GetROW;							// fubf2y
	if (MDIN3xx_SetMemoryMap(pINFO, 0+2, pMAP->Y_x, addr)) return MDIN_I2C_ERROR;

	// map of video data frame (C_x)
	addr += GetROW;							// fbuf2c
	if (MDIN3xx_SetMemoryMap(pINFO, 4+2, pMAP->C_x, addr)) return MDIN_I2C_ERROR;

	// req_mapping_1
	mode = (0<<12)|(0<<8)|((8+0)<<0);	// mvfw_nr=0, mvfr_y=0, mvfr_c=0
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1d1, mode)) return MDIN_I2C_ERROR;

	// req_mapping_2
	mode = (1<<12)|(1<<8);				// mvfw_mh=1, mvfr_mh=1
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1d2, mode)) return MDIN_I2C_ERROR;

	// req_mapping_3
	mode = (0<<12)|((8+0)<<4);			// input_y=0, input_c=0
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1d3, mode)) return MDIN_I2C_ERROR;

	// req_mapping_4
	mode = (2<<12)|(2<<8)|((8+2)<<4)|((8+2)<<0);	// aux_y=2, aux_c=2
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1d4, mode)) return MDIN_I2C_ERROR;

	// f_count_1 ==> for 4-CH input mode, 2-HD input mode
	mode = (pINFO->dacPATH==DAC_PATH_AUX_4CH)? 0x8220 : 0x4120;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1dd, mode)) return MDIN_I2C_ERROR;

	// f_count_2 ==> for 4-CH input mode, 2-HD input mode
	mode = (pINFO->dacPATH==DAC_PATH_AUX_4CH)? 0x8020 : 0x3020;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1de, mode)) return MDIN_I2C_ERROR;

	// ad_start_row, ad_end_row
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1f8, fbADDR)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1f9, fbADDR+15)) return MDIN_I2C_ERROR;
	fbADDR += 16;

#if __MDIN3xx_DBGPRT__ == 1
	UARTprintf("used FB[AUDIO] row is %d, GetROW=%d\n", fbADDR, 16);
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_EnableWriteFRMB(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	BOOL ctrl;

	// main_display
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x040, 5, 1, (OnOff)? ON : OFF)) return MDIN_I2C_ERROR;

	// main_freeze
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x040, 1, 1, (OnOff)? frez_M : 1)) return MDIN_I2C_ERROR;

	// aux_display
	ctrl = MBIT(pINFO->dspFLAG,MDIN_AUX_DISPLAY_ON);
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x142, 0, 1, (OnOff)? ctrl : OFF)) return MDIN_I2C_ERROR;

	// aux_freeze
	ctrl = MBIT(pINFO->dspFLAG, MDIN_AUX_FREEZE_ON);
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x142, 1, 1, (OnOff)? ctrl : ON)) return MDIN_I2C_ERROR;

	// aux_pip
	ctrl = (pINFO->dacPATH==DAC_PATH_MAIN_PIP)? 1 : 0;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x144, 6, 1, (OnOff)? ctrl : OFF)) return MDIN_I2C_ERROR;

	// ipc_top_en
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x205, 3, 1, (OnOff)? ON : OFF)) return MDIN_I2C_ERROR;
	return MDINDLY_mSec(40);		// delay 40ms
}

#if	defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380)
//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_Set4CHProcess(PMDIN_VIDEO_INFO pINFO)
{
	WORD rVal;	BYTE nA1, nA2, nB1, nB2;

	if (pINFO->dacPATH!=DAC_PATH_AUX_4CH) return MDIN_NO_ERROR;

	if (MDINHIF_RegRead(MDIN_LOCAL_ID, 0x0fd, &rVal)) return MDIN_I2C_ERROR;
	nA1 = HI4BIT(HIBYTE(rVal))&3;	nA2 = LO4BIT(HIBYTE(rVal))&3;
	nB1 = HI4BIT(LOBYTE(rVal))&3;	nB2 = LO4BIT(LOBYTE(rVal))&3;

	switch (pINFO->st4CH_x.order) {
		case MDIN_4CHID_A1A2B1B2: rVal = nA1|(nA2<<2)|(nB1<<4)|(nB2<<6); break;
		case MDIN_4CHID_A1B1A2B2: rVal = nA1|(nB1<<2)|(nA2<<4)|(nB2<<6); break;
		case MDIN_4CHID_A2A1B2B1: rVal = nA2|(nA1<<2)|(nB2<<4)|(nB1<<6); break;
		case MDIN_4CHID_A2B2A1B1: rVal = nA2|(nB2<<2)|(nA1<<4)|(nB1<<6); break;
		case MDIN_4CHID_B1B2A1A2: rVal = nB1|(nB2<<2)|(nA1<<4)|(nA2<<6); break;
		case MDIN_4CHID_B1A1B2A2: rVal = nB1|(nA1<<2)|(nB2<<4)|(nA2<<6); break;
		case MDIN_4CHID_B2B1A2A1: rVal = nB2|(nB1<<2)|(nA2<<4)|(nA1<<6); break;
		case MDIN_4CHID_B2A2B1A1: rVal = nB2|(nA2<<2)|(nB1<<4)|(nA1<<6); break;
		default:				  rVal = nA1|(nA2<<2)|(nB1<<4)|(nB2<<6); break;
	}
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x14a, 0, 8, rVal)) return MDIN_I2C_ERROR;

#if __MDIN3xx_DBGPRT__ == 1
	UARTprintf("4CH-ID Order rd = %d %d %d %d\n", nA1, nA2, nB1, nB2);
	nA1 = rVal&3; nA2 = (rVal>>2)&3; nB1 = (rVal>>4)&3; nB2 = rVal>>6;
	UARTprintf("4CH-ID Order wr = %d %d %d %d\n", nB2, nB1, nA2, nA1);
#endif

	nA1 = rVal&3; nA2 = (rVal>>2)&3; nB1 = (rVal>>4)&3; nB2 = rVal>>6;

	switch (pINFO->st4CH_x.view) {
		case MDIN_4CHVIEW_CH01:	rVal = nA1|(nA2<<2)|(nB1<<4)|(nB2<<6); break;
		case MDIN_4CHVIEW_CH02: rVal = nA2|(nA2<<2)|(nB1<<4)|(nB2<<6); break;
		case MDIN_4CHVIEW_CH03: rVal = nB1|(nA2<<2)|(nB1<<4)|(nB2<<6); break;
		case MDIN_4CHVIEW_CH04: rVal = nB2|(nA2<<2)|(nB1<<4)|(nB2<<6); break;
		case MDIN_4CHVIEW_CH12: rVal = nA1|(nA2<<2)|(nB1<<4)|(nB2<<6); break;
		case MDIN_4CHVIEW_CH13: rVal = nA1|(nB1<<2)|(nB1<<4)|(nB2<<6); break;
		case MDIN_4CHVIEW_CH14: rVal = nA1|(nB2<<2)|(nB1<<4)|(nB2<<6); break;
		case MDIN_4CHVIEW_CH23: rVal = nA2|(nB1<<2)|(nB1<<4)|(nB2<<6); break;
		case MDIN_4CHVIEW_CH24: rVal = nA2|(nB2<<2)|(nB1<<4)|(nB2<<6); break;
		case MDIN_4CHVIEW_CH34: rVal = nB1|(nB2<<2)|(nB1<<4)|(nB2<<6); break;
		default:				rVal = nA1|(nA2<<2)|(nB1<<4)|(nB2<<6); break;
	}
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x14a, 0, 8, rVal)) return MDIN_I2C_ERROR;

#if __MDIN3xx_DBGPRT__ == 1
	UARTprintf("4CH View Mode = %d %d %d %d\n", nB2, nB1, nA2, nA1);
#endif

	return MDIN_NO_ERROR;
}
#endif	/* defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380) */

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Video Process Block (Input/Output/Scaler/Deinterlace/CSC/PLL)
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_VideoProcess(PMDIN_VIDEO_INFO pINFO)
{
	if (pINFO->exeFLAG==MDIN_UPDATE_CLEAR)	return MDIN_NO_ERROR;
	pINFO->exeFLAG = MDIN_UPDATE_CLEAR;		// clear update flag

	if (MDIN3xx_EnableWriteFRMB(pINFO, 0)) return MDIN_I2C_ERROR;	// disable write FB
	if (MDIN3xx_SetSrcClockPath(pINFO)) return MDIN_I2C_ERROR;		// set source clock path

	if (MDIN3xx_SetSrcVideoPort(pINFO, 1)) return MDIN_I2C_ERROR;	// set source video port A
	if (MDIN3xx_SetSrcVideoPort(pINFO, 0)) return MDIN_I2C_ERROR;	// set source video port B

	if (MDIN3xx_SetSrcVideoFrmt(pINFO)) return MDIN_I2C_ERROR;		// set source video format
	if (MDIN3xx_SetOutVideoFrmt(pINFO)) return MDIN_I2C_ERROR;		// set output video format

	if (MDIN3xx_SetSrcVideoCSC(pINFO)) return MDIN_I2C_ERROR;		// set source video CSC
	if (MDIN3xx_SetOutVideoCSC(pINFO)) return MDIN_I2C_ERROR;		// set output video CSC

	if (MDIN3xx_SetOutVideoDAC(pINFO)) return MDIN_I2C_ERROR;		// set output video DAC
	if (MDIN3xx_SetOutVideoSYNC(pINFO)) return MDIN_I2C_ERROR;		// set output video SYNC

	memset((PBYTE)&pINFO->stZOOM_m, 0, sizeof(MDIN_VIDEO_WINDOW));	// clear main ZOOM window
	memset((PBYTE)&pINFO->stZOOM_x, 0, sizeof(MDIN_VIDEO_WINDOW));	// clear aux ZOOM window

	if (MDIN3xx_SetMFCScaleWind(pINFO)) return MDIN_I2C_ERROR;		// set MFC scaler window
	if (MDIN3xx_SetMFCScaleCtrl(pINFO)) return MDIN_I2C_ERROR;		// set MFC scaler control

	if (MDIN3xx_Set10BITProcess(pINFO)) return MDIN_I2C_ERROR;		// set 10BIT process
	if (MDIN3xx_SetFFCNRProcess(pINFO)) return MDIN_I2C_ERROR;		// set FFC-NR process

	if (MDIN3xx_SetIPCCtrlFlags(pINFO)) return MDIN_I2C_ERROR;		// set IPC control flags
	if (MDIN3xx_SetDeinterCtrl(pINFO)) return MDIN_I2C_ERROR;		// set deinterlace control

	if (MDINAUX_VideoProcess(pINFO)) return MDIN_I2C_ERROR;			// set aux-video process
	if (MDIN3xx_SetFrameBuffer(pINFO)) return MDIN_I2C_ERROR;		// set frame buffer memory

#if	defined(SYSTEM_USE_MDIN340)||defined(SYSTEM_USE_MDIN380)
	if (MDINHTX_VideoProcess(pINFO)) return MDIN_I2C_ERROR;			// set htx-video process
#endif

	if (MDIN3xx_ResetOutSync(100)) return MDIN_I2C_ERROR;			// reset output sync
	if (MDIN3xx_SoftReset()) return MDIN_I2C_ERROR;					// soft reset
	if (MDIN3xx_EnableWriteFRMB(pINFO, 1)) return MDIN_I2C_ERROR;	// enable write FB

#if	defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380)
	if (MDIN3xx_Set4CHProcess(pINFO)) return MDIN_I2C_ERROR;		// set 4CH-display process
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetZoomProcess(PMDIN_VIDEO_INFO pINFO)
{
	if (MDIN3xx_SetMFCScaleWind(pINFO)) return MDIN_I2C_ERROR;
	if (MDIN3xx_SetMFCScaleCtrl(pINFO)) return MDIN_I2C_ERROR;
	if (MDIN3xx_Set10BITProcess(pINFO)) return MDIN_I2C_ERROR;
	if (MDIN3xx_SetFFCNRProcess(pINFO)) return MDIN_I2C_ERROR;
	if (MDIN3xx_SetIPCCtrlFlags(pINFO)) return MDIN_I2C_ERROR;
	if (MDIN3xx_SetDeinterCtrl(pINFO)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetScaleProcess(PMDIN_VIDEO_INFO pINFO)
{
//	if (MDIN3xx_EnableWriteFRMB(pINFO, 0)) return MDIN_I2C_ERROR;	// disable write FB

	if (MDIN3xx_SetZoomProcess(pINFO)) return MDIN_I2C_ERROR;
	if (MDINAUX_SetScaleProcess(pINFO)) return MDIN_I2C_ERROR;
	if (MDIN3xx_SetFrameBuffer(pINFO)) return MDIN_I2C_ERROR;

#if	defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380)
	if (MDIN3xx_Set4CHProcess(pINFO)) return MDIN_I2C_ERROR;
#endif

//	if (MDIN3xx_EnableWriteFRMB(pINFO, 1)) return MDIN_I2C_ERROR;	// enable write FB
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Source Video Block
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetSrcCompHsync(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	WORD nID = (pSRC->stATTB.attb&MDIN_USE_INPORT_A)? 1 : 0;

	if (OnOff)	pSRC->fine |=  MDIN_HACT_IS_CSYNC;
	else		pSRC->fine &= ~MDIN_HACT_IS_CSYNC;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x002, 6+nID*8, 1, MBIT(pSRC->fine,MDIN_HACT_IS_CSYNC))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetSrcInputFieldID(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	WORD nID = (pSRC->stATTB.attb&MDIN_USE_INPORT_A)? 1 : 0;

	if (OnOff)	pSRC->fine |=  MDIN_FIELDID_INPUT;
	else		pSRC->fine &= ~MDIN_FIELDID_INPUT;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x001, 2+nID, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetSrcHighTopFieldID(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	WORD nID = (pSRC->stATTB.attb&MDIN_USE_INPORT_A)? 1 : 0;

	if (OnOff)	pSRC->fine |=  MDIN_HIGH_IS_TOPFLD;
	else		pSRC->fine &= ~MDIN_HIGH_IS_TOPFLD;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x001, 0+nID, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetSrcYCOffset(PMDIN_VIDEO_INFO pINFO, MDIN_YC_OFFSET_t val)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	WORD nID = (pSRC->stATTB.attb&MDIN_USE_INPORT_A)? 1 : 0;

	pSRC->fine &= ~MDIN_YCOFFSET_MASK; pSRC->fine |= (val&MDIN_YCOFFSET_MASK);
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x000, 8+nID*3, 3, val)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetSrcCbCrSwap(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	WORD nID = (pSRC->stATTB.attb&MDIN_USE_INPORT_A)? 1 : 0;

	if (OnOff)	pSRC->fine |=  MDIN_CbCrSWAP_ON;
	else		pSRC->fine &= ~MDIN_CbCrSWAP_ON;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x000, 2+nID, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetSrcHACTPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	WORD nID = (pSRC->stATTB.attb&MDIN_USE_INPORT_A)? 1 : 0;

	if (edge)	pSRC->stATTB.attb |=  MDIN_NEGATIVE_HACT;
	else		pSRC->stATTB.attb &= ~MDIN_NEGATIVE_HACT;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x002, 10-nID*8, 1, MBIT(edge,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetSrcVACTPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	WORD nID = (pSRC->stATTB.attb&MDIN_USE_INPORT_A)? 1 : 0;

	if (edge)	pSRC->stATTB.attb |=  MDIN_NEGATIVE_VACT;
	else		pSRC->stATTB.attb &= ~MDIN_NEGATIVE_VACT;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x002, 9-nID*8, 1, MBIT(edge,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetSrcOffset(PMDIN_VIDEO_INFO pINFO, WORD offH, WORD offV)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	WORD nID = (pSRC->stATTB.attb&MDIN_USE_INPORT_A)? 1 : 0;

	pSRC->offH = offH;	pSRC->offV = offV;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x003, 0+nID*8, 7, offV>>6)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x005-nID, ((offV&0x3f)<<10)|(offH&0x3ff))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x008-nID*2, 13, 3, HIBYTE(offH)>>2)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetSrcCSCCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_CSCCTRL_INFO pCSC)
{
	pINFO->stSRC_m.pCSC = pCSC;
	return MDIN3xx_SetSrcVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Output Video Block
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetOutCbCrSwap(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_m;

	if (OnOff)	pOUT->fine |=  MDIN_CbCrSWAP_ON;
	else		pOUT->fine &= ~MDIN_CbCrSWAP_ON;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a2, 4, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetOutPbPrSwap(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_m;

	if (OnOff)	pOUT->fine |=  MDIN_PbPrSWAP_ON;
	else		pOUT->fine &= ~MDIN_PbPrSWAP_ON;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a3, 4, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetOutDEPinMode(PMDIN_VIDEO_INFO pINFO, MDIN_DEOUT_MODE_t mode)
{
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_m;

	pOUT->stATTB.attb &= ~(MDIN_DEOUT_IS_HACT);
	pOUT->stATTB.attb |= ((WORD)mode<<10);

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a0, 8, 2, mode)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetOutHSPinMode(PMDIN_VIDEO_INFO pINFO, MDIN_HSOUT_MODE_t mode)
{
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_m;

	pOUT->stATTB.attb &= ~(MDIN_HSYNC_IS_HACT);
	pOUT->stATTB.attb |= ((WORD)mode<<14);

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a0, 2, 2, mode)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetOutVSPinMode(PMDIN_VIDEO_INFO pINFO, MDIN_VSOUT_MODE_t mode)
{
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_m;

	pOUT->stATTB.attb &= ~(MDIN_VSYNC_IS_VACT);
	pOUT->stATTB.attb |= ((WORD)mode<<12);

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a0, 0, 2, mode)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetOutDEPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge)
{
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_m;

	if (edge)	pOUT->stATTB.attb |=  MDIN_POSITIVE_DE;
	else		pOUT->stATTB.attb &= ~MDIN_POSITIVE_DE;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a0, 7, 1, MBIT(edge,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetOutHSPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge)
{
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_m;

	if (edge)	pOUT->stATTB.attb |=  MDIN_POSITIVE_HSYNC;
	else		pOUT->stATTB.attb &= ~MDIN_POSITIVE_HSYNC;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a0, 5, 1, MBIT(edge,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetOutVSPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge)
{
	PMDIN_OUTVIDEO_INFO pOUT = (PMDIN_OUTVIDEO_INFO)&pINFO->stOUT_m;

	if (edge)	pOUT->stATTB.attb |=  MDIN_POSITIVE_VSYNC;
	else		pOUT->stATTB.attb &= ~MDIN_POSITIVE_VSYNC;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a0, 4, 1, MBIT(edge,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetPictureBrightness(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_m.brightness = value;
	return MDIN3xx_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetPictureContrast(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_m.contrast = value;
	return MDIN3xx_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetPictureSaturation(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_m.saturation = value;
	return MDIN3xx_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetPictureHue(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_m.hue = value;
	return MDIN3xx_SetOutVideoCSC(pINFO);
}

#if RGB_GAIN_OFFSET_TUNE == 1
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetPictureGainR(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_m.r_gain = value;
	return MDIN3xx_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetPictureGainG(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_m.g_gain = value;
	return MDIN3xx_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetPictureGainB(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_m.b_gain = value;
	return MDIN3xx_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetPictureOffsetR(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_m.r_offset = value;
	return MDIN3xx_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetPictureOffsetG(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_m.g_offset = value;
	return MDIN3xx_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetPictureOffsetB(PMDIN_VIDEO_INFO pINFO, BYTE value)
{
	pINFO->stOUT_m.b_offset = value;
	return MDIN3xx_SetOutVideoCSC(pINFO);
}
#endif

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetOutCSCCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_CSCCTRL_INFO pCSC)
{
	pINFO->stOUT_m.pCSC = (PMDIN_CSCCTRL_INFO)pCSC;
	return MDIN3xx_SetOutVideoCSC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetOutSYNCCtrl(PMDIN_VIDEO_INFO pINFO, PMDIN_OUTVIDEO_SYNC pSYNC)
{
	pINFO->stOUT_m.pSYNC = (PMDIN_OUTVIDEO_SYNC)pSYNC;
	return MDIN3xx_SetOutVideoSYNC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetOutDACCtrl(PMDIN_VIDEO_INFO pINFO, PMDIN_DACSYNC_CTRL pDAC)
{
	pINFO->stOUT_m.pDAC = (PMDIN_DACSYNC_CTRL)pDAC;
	return MDIN3xx_SetOutVideoDAC(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Video Window & MFC filter
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_GetVideoWindowFRMB(PMDIN_VIDEO_INFO pINFO, PMDIN_VIDEO_WINDOW pFRMB)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;

	pFRMB->w = pINFO->stMFC_m.stMEM.w;
	pFRMB->h = pINFO->stMFC_m.stMEM.h;
	pFRMB->x = pFRMB->y = 0;

	if ((pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)==0) pFRMB->h *= 2;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetVideoWindowCROP(PMDIN_VIDEO_INFO pINFO, MDIN_VIDEO_WINDOW stCROP)
{
	memcpy(&pINFO->stCROP_m, &stCROP, sizeof(MDIN_VIDEO_WINDOW));
	return MDIN3xx_SetScaleProcess(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetVideoWindowZOOM(PMDIN_VIDEO_INFO pINFO, MDIN_VIDEO_WINDOW stZOOM)
{
	memcpy(&pINFO->stZOOM_m, &stZOOM, sizeof(MDIN_VIDEO_WINDOW));
	return MDIN3xx_SetZoomProcess(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetVideoWindowVIEW(PMDIN_VIDEO_INFO pINFO, MDIN_VIDEO_WINDOW stVIEW)
{
	memcpy(&pINFO->stVIEW_m, &stVIEW, sizeof(MDIN_VIDEO_WINDOW));
	return MDIN3xx_SetScaleProcess(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetMFCHYFilterCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_MFCFILT_COEF pCoef)
{
	pINFO->pHY_m = (PMDIN_MFCFILT_COEF)pCoef;
	return MDIN3xx_SetMFCFilterCoef(pINFO, MDIN_MFCFILT_HY);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetMFCHCFilterCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_MFCFILT_COEF pCoef)
{
	pINFO->pHC_m = (PMDIN_MFCFILT_COEF)pCoef;
	return MDIN3xx_SetMFCFilterCoef(pINFO, MDIN_MFCFILT_HC);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetMFCVYFilterCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_MFCFILT_COEF pCoef)
{
	pINFO->pVY_m = (PMDIN_MFCFILT_COEF)pCoef;
	return MDIN3xx_SetMFCFilterCoef(pINFO, MDIN_MFCFILT_VY);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetMFCVCFilterCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_MFCFILT_COEF pCoef)
{
	pINFO->pVC_m = (PMDIN_MFCFILT_COEF)pCoef;
	return MDIN3xx_SetMFCFilterCoef(pINFO, MDIN_MFCFILT_VC);
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for clock Block
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetDelayCLK_A(MDIN_CLK_DELAY_t delay)
{
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x048, 10, 3, delay)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetDelayCLK_B(MDIN_CLK_DELAY_t delay)
{
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x049, 10, 3, delay)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetDelayVCLK_OUT(MDIN_CLK_DELAY_t delay)
{
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x024, 0, 3, delay)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetDelayVCLK_OUT_B(MDIN_CLK_DELAY_t delay)
{
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x024, 4, 3, delay)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetClock_clk_a(MDIN_CLK_SOURCE_t src)
{
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x047, 4, 1, src)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetClock_clk_b(MDIN_CLK_SOURCE_t src)
{
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x047, 5, 1, src)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetClock_clk_a1(MDIN_CLK_PATH_A1_t src)
{
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x048, 8, 2, src)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetClock_clk_a2(MDIN_CLK_PATH_A2_t src)
{
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x048, 3, 2, src)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetClock_clk_b1(MDIN_CLK_PATH_B1_t src)
{
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x049, 8, 2, src)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetClock_clk_b2(MDIN_CLK_PATH_B2_t src)
{
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x049, 3, 2, src)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetClock_clk_m1(MDIN_CLK_PATH_M1_t src)
{
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x048, 13, 3, src)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetClock_clk_m2(MDIN_CLK_PATH_M2_t src)
{
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x049, 13, 3, src)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Filters Block
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetFrontNRFilterCoef(PMDIN_FNRFILT_COEF pCoef)
{
	if (pCoef==NULL) pCoef = (PMDIN_FNRFILT_COEF)MDIN_FrontNRFilter_Default;
	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x022, (PBYTE)pCoef->y, 16)) return MDIN_I2C_ERROR;
	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x02c, (PBYTE)pCoef->c,  8)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableFrontNRFilter(BOOL OnOff)
{
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x02a, 3, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x02a, 0, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetPeakingFilterCoef(PMDIN_FNRFILT_COEF pCoef)
{
	if (pCoef==NULL) pCoef = (PMDIN_FNRFILT_COEF)MDIN_PeakingFilter_Default;
	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x120, (PBYTE)pCoef, 16)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetPeakingFilterLevel(BYTE val)
{
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x129, 8, 8, val)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnablePeakingFilter(BOOL OnOff)
{
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x128, 0, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableLTI(BOOL OnOff)
{
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x063, (OnOff)? 0x02f9 : 0)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableCTI(BOOL OnOff)
{
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x065, (OnOff)? 0x02f9 : 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x067, (OnOff)? 0x02f9 : 0)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetColorEnFilterCoef(PMDIN_COLOREN_COEF pCoef)
{
	if (pCoef==NULL) pCoef = (PMDIN_COLOREN_COEF)MDIN_ColorEnFilter_Default;
	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x068, (PBYTE)pCoef, 8)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableColorEnFilter(BOOL OnOff)
{
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x06c, 0, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetBlockNRFilterCoef(PMDIN_BNRFILT_COEF pCoef)
{
	if (pCoef==NULL) pCoef = (PMDIN_BNRFILT_COEF)MDIN_BlockNRFilter_Default;
	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x12a, (PBYTE)pCoef, 8)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableBlockNRFilter(BOOL OnOff)
{
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x12e, (OnOff)? 5 : 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x208, 3, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetMosquitFilterCoef(PMDIN_MOSQUIT_COEF pCoef)
{
	if (pCoef==NULL) pCoef = (PMDIN_MOSQUIT_COEF)MDIN_MosquitFilter_Default;
	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x050, (PBYTE)pCoef, 18)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableMosquitFilter(BOOL OnOff)
{
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x058, 0, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetSkinTonFilterCoef(PMDIN_SKINTON_COEF pCoef)
{
	if (pCoef==NULL) pCoef = (PMDIN_SKINTON_COEF)MDIN_SkinTonFilter_Default;
	if (MDINHIF_MultiWrite(MDIN_LOCAL_ID, 0x3f0, (PBYTE)pCoef, 10)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableSkinTonFilter(BOOL OnOff)
{
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x3f3, 10, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetBWExtensionPoint(PMDIN_BWPOINT_COEF pCoef)
{
	if (pCoef==NULL) return MDIN_INVALID_PARAM;

	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x3d0, MAKEWORD(pCoef->src[0],pCoef->src[1]))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x3d1, MAKEWORD(pCoef->src[2],pCoef->src[3]))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x3d2, MAKEWORD(pCoef->out[0],pCoef->out[1]))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x3d3, MAKEWORD(pCoef->out[2],pCoef->out[3]))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableBWExtension(BOOL OnOff)
{
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x3c0, 0, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}
/*
//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Video-Encoder Block
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableVENCColorMode(BOOL OnOff)
{
	if (MDINHIF_RegWrite(0x4C6, 0x0008)) return MDIN_I2C_ERROR;			// reg_oen
	if (MDINHIF_RegField(0x5D4, 13, 1, (OnOff)? 0 : 1)) return MDIN_I2C_ERROR;
	return (MDINHIF_RegWrite(0x4C4, 1))? MDIN_I2C_ERROR:MDIN_NO_ERROR;	// update local register
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableVENCBlueScreen(BOOL OnOff)
{
	if (MDINHIF_RegWrite(0x4C6, 0x0008)) return MDIN_I2C_ERROR;			// reg_oen
	if (MDINHIF_RegField(0x5D4, 10, 1, (OnOff)? ON : OFF)) return MDIN_I2C_ERROR;
	return (MDINHIF_RegWrite(0x4C4, 1))? MDIN_I2C_ERROR:MDIN_NO_ERROR;	// update local register
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableVENCPeakingFilter(BOOL OnOff)
{
	if (MDINHIF_RegWrite(0x4C6, 0x0008)) return MDIN_I2C_ERROR;			// reg_oen
	if (MDINHIF_RegField(0x5DC, 3, 1, (OnOff)? ON : OFF)) return MDIN_I2C_ERROR;
	return (MDINHIF_RegWrite(0x4C4, 1))? MDIN_I2C_ERROR:MDIN_NO_ERROR;	// update local register
}
*/
//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Memory Block
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetMemoryConfig(void)
{
	if (MDIN3xx_GetDeviceID(&GetDID)) return MDIN_I2C_ERROR;			// get device-ID
	if (GetDID<MDIN_DID_325||GetDID>MDIN_DID_345A) return MDIN_INVALID_DEV_ID;

	GetMEM = (GetDID==MDIN_DID_325||GetDID==MDIN_DID_340)? MDIN_COLUMN_7BIT : MDIN_COLUMN_8BIT;

	// set memory PLL - 256MB(199.125MHz), 128MB(162.000MHz)
//	if (MDIN3xx_SetMemoryPLL((GetMEM)? 1 : 4, (GetMEM)? 12 : 59, 0)) return MDIN_I2C_ERROR;

#if defined(SYSTEM_USE_MCLK189)
	if (MDIN3xx_SetMemoryPLL(1, (GetMEM)? 12 : 14, 0)) return MDIN_I2C_ERROR;
#else
	if (MDIN3xx_SetMemoryPLL(1, (GetMEM)? 12 : 15, 0)) return MDIN_I2C_ERROR;
#endif

	if (MDINDLY_mSec(1)) return MDIN_I2C_ERROR;		// delay 1ms

	if (MDIN3xx_GetVersionID(&mdinREV)) return MDIN_I2C_ERROR;			// get version-ID

	// ddr_init
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x094, 0x072f)) return MDIN_I2C_ERROR;	// dfi isft
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x090, 0, 1, 1)) return MDIN_I2C_ERROR;	// dfi ireset
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x090, 1, 1, 1)) return MDIN_I2C_ERROR;	// dfi iusrst
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x090, 2, 1, 1)) return MDIN_I2C_ERROR;	// dfi idllrst
	if (MDINDLY_mSec(1)) return MDIN_I2C_ERROR;		// delay 1ms

	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1a3, 0x1867)) return MDIN_I2C_ERROR;	// ddr timing tune
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1a7, 0x4008)) return MDIN_I2C_ERROR;	// ddr initialize
	if (MDINHIF_RegWrite(MDIN_HOST_ID,  0x093, 0x2f80)) return MDIN_I2C_ERROR;	// latency init
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1a7, 0x2008)) return MDIN_I2C_ERROR;	// ddr access enable

	// bank_2kb, ddr_7bit, mem_7bit, audio_8bit, osd_7bit, axosd_7bit
	if (MDINHIF_RegField(MDIN_HOST_ID,  0x005,  8, 1, ~GetMEM&1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1a7, 12, 1,  GetMEM&1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1c0, 11, 1,  GetMEM&1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1fb,  7, 1, ~GetMEM&1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x300,  1, 1,  GetMEM&1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x2a0,  6, 1,  GetMEM&1)) return MDIN_I2C_ERROR;

	// ddr timing tune
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1a3, 11, 2, (GetMEM)? 0 : 3)) return MDIN_I2C_ERROR;

	// hdmi_clk_ctrl1 - audio_delay_bypass
	if (MDINHIF_RegField(MDIN_HOST_ID,  0x03e, 15, 1, 1)) return MDIN_I2C_ERROR;

	// arbiter_ctrl - refresh_prog_count
	GetPRI = (GetDID==MDIN_DID_325A)? 0x1b84 : (GetDID==MDIN_DID_380)? 0x3b84 : 0x0004;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1a0, GetPRI)) return MDIN_I2C_ERROR;	// fix
//	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1a0, 0x0004)) return MDIN_I2C_ERROR;	// fix

	// arbiter_pri, arbiter_starv
	GetPRI = 0x1ae4;	GetSTV = 0x7277;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1a1, GetPRI)) return MDIN_I2C_ERROR;	// fix
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1a2, GetSTV)) return MDIN_I2C_ERROR;	// fix

	// fcmc_arb_ctrl1
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1d5, 0xeac2)) return MDIN_I2C_ERROR;	// fix

	// ddr_if_ctrl - ddr_drv_strength
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1a7, 0, 12, 4)) return MDIN_I2C_ERROR;	// fix

	// ack_count - dma_ack_count
	if (MDINHIF_RegField(MDIN_HOST_ID,  0x005, 0, 8, 16)) return MDIN_I2C_ERROR;	// fix

	// endian_swap for BUS & I2C
#if	CPU_ALIGN_BIG_ENDIAN == 1
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x005, 11, 2, 1)) return MDIN_I2C_ERROR;	// fix big-endian
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x006, 14, 2, 1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x012, 0x0001)) return MDIN_I2C_ERROR;	// fix big-endian
#else
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x005, 11, 2, 3)) return MDIN_I2C_ERROR;	// fix big-endian
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x006, 14, 2, 3)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x012, 0x0003)) return MDIN_I2C_ERROR;	// fix big-endian
#endif

	// osd_endian_mode
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x300, 2, 1, 0)) return MDIN_I2C_ERROR;	// fix big-endian
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x300,12, 1, 1)) return MDIN_I2C_ERROR;	// fix BG-BOX ON

	// axosd_endian_mode
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x2a0, 7, 1, 0)) return MDIN_I2C_ERROR;	// fix big-endian

	// sdram_32bit
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x3fe, 0x0000)) return MDIN_I2C_ERROR;	// fix 64bit

	// default frame buffer map
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x040, 1, 1, 1)) return MDIN_I2C_ERROR;	// main_freeze
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x142, 1, 1, 1)) return MDIN_I2C_ERROR;	// aux_freeze
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x205, 3, 1, 0)) return MDIN_I2C_ERROR;	// ipc_top_off

	// fix in_wr_posi_h bug
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x040,  8, 1, 1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x01c, 10, 1, 1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x01c, 10, 1, 0)) return MDIN_I2C_ERROR;

	// default pad out, clock drive, venc dac
	switch (GetDID) {
		case MDIN_DID_325:	GetPAD = 0x00; GetCLK = 0x07a0; GetENC = 0; break;
		case MDIN_DID_340:	GetPAD = 0x16; GetCLK = 0x0044; GetENC = 1; break;
		case MDIN_DID_325A:	GetPAD = 0x00; GetCLK = 0x07a0; GetENC = 0; break;
		default:			GetPAD = 0x00; GetCLK = 0x0000; GetENC = 0; break;
	}

#if SYSTEM_USE_DAC_OUT == 0
	GetPAD |= 0x09;	GetCLK |= 0x0008;	// disable DAC pad/sync & DAC clock
#endif

	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x01e, GetPAD)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x03c, GetCLK)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x045, 13, 1, GetENC)) return MDIN_I2C_ERROR;

	// clear variables
	mdinERR = 0; GetIRQ = 0; vpll_P = 0, vpll_M = 0, vpll_S = 0; frez_M = 0;
	if (MDINAUX_SetVideoPLL(0, 0, 0)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
DWORD		MDIN3xx_GetAddressFRMB(void)
{
	WORD unit = (GetMEM&MDIN_COLUMN_7BIT)? 2048 : 4096;
	return (DWORD)fbADDR*unit;
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for ETC(reset, port out, freeze, etc) Block
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_GetChipID(PWORD pID)
{
	if (MDINHIF_RegRead(MDIN_HOST_ID, 0x000, pID)) return MDIN_I2C_ERROR;

#if __MDIN3xx_DBGPRT__ == 1
	UARTprintf("MDIN3xx_GetChipID = 0x%04X\n", *pID);
#endif
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_GetDeviceID(PWORD pID)
{
	if (MDINHIF_RegRead(MDIN_HOST_ID, 0x004, pID)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_GetVersionID(PWORD pID)
{
	if (MDINHIF_RegRead(MDIN_HOST_ID, 0x007, pID)) return MDIN_I2C_ERROR;
	*pID &= 0x0001;

#if __MDIN3xx_DBGPRT__ == 1
	UARTprintf("MDIN3xx_GetVersionID = 0x%04X\n", *pID);
#endif
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
WORD		 MDIN3xx_GetSizeOfBank(void)
{
	return (GetMEM&MDIN_COLUMN_7BIT)? 1024 : 2048;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_HardReset(void)
{
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x072, 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x072, 1)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SoftReset(void)
{
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1a0,  6, 1, 1)) return MDIN_I2C_ERROR;	// disable refresh
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1a7, 13, 1, 0)) return MDIN_I2C_ERROR;	// disable access

	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x070, 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x070, 1)) return MDIN_I2C_ERROR;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1a7, 13, 1, 1)) return MDIN_I2C_ERROR;	// enable access
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x1a0,  6, 1, 0)) return MDIN_I2C_ERROR;	// enable refresh
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableIRQ(MDIN_IRQ_FLAG_t irq, BOOL OnOff)
{
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x00e, irq, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
//	if (MDINHIF_RegField(MDIN_HOST_ID, 0x016, irq, 1, 0)) return MDIN_I2C_ERROR;	// fix edge detect
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x016, 0x0080)) return MDIN_I2C_ERROR;		// fix edge detect
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x08e, 11, 1, 0)) return MDIN_I2C_ERROR;		// fix active low
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_GetParseIRQ(void)
{
	WORD irq;
	if (MDINHIF_RegRead(MDIN_HOST_ID, 0x010, &irq)) return MDIN_I2C_ERROR;
	GetIRQ |= irq;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
BOOL		 MDIN3xx_IsOccurIRQ(MDIN_IRQ_FLAG_t irq)
{
	BOOL out = (GetIRQ&(1<<irq))? ON : OFF;
	GetIRQ &= ~(1<<irq);
	return out;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetPriorityHIF(MDIN_PRIORITY_t mode)
{
	WORD val = (mode==MDIN_PRIORITY_NORM)? GetPRI : GetPRI|0x000f;

	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x1a1, val)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetVCLKPLLSource(MDIN_PLL_SOURCE_t src)
{
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x03a, (src==MDIN_PLL_SOURCE_HACT)? 1 : 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x02a, (src==MDIN_PLL_SOURCE_HACT)? 0 : src)) return MDIN_I2C_ERROR;
	return MDIN3xx_SoftReset();
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableOutputPAD(MDIN_PAD_OUT_t id, BOOL OnOff)
{
	if (id==MDIN_PAD_ALL_OUT) {
		if (MDINHIF_RegWrite(MDIN_HOST_ID, 0x01e, (OnOff)? GetPAD : 0x1f)) return MDIN_I2C_ERROR;
		return MDINHIF_RegField(MDIN_HOST_ID, 0x045, 13, 1, (OnOff)? GetENC : 1);
	}

	if (id==MDIN_PAD_ENC_OUT) {
		return MDINHIF_RegField(MDIN_HOST_ID, 0x045, 13, 1, (OnOff)? GetENC : 1);
	}

	return MDINHIF_RegField(MDIN_HOST_ID, 0x01e, id, 1, RBIT(OnOff,1));
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableClockDrive(MDIN_CLK_DRV_t id, BOOL OnOff)
{
	if (id==MDIN_CLK_DRV_ALL) {
		return MDINHIF_RegWrite(MDIN_HOST_ID, 0x03c, (OnOff)? GetCLK : 0xffff);
	}

	return MDINHIF_RegField(MDIN_HOST_ID, 0x03c, id, 1, RBIT(OnOff,1));
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableMainDisplay(BOOL OnOff)
{
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x040, 5, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	if (OnOff==ON)	MDINDLY_mSec(80);	// delay 80ms when ON

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x043, 1, 1, RBIT(OnOff,1))) return MDIN_I2C_ERROR;
	if (OnOff==OFF)	MDINDLY_mSec(40);	// delay 40ms when OFF
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableMainFreeze(BOOL OnOff)
{
	frez_M = MBIT(OnOff,1);
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x040, 1, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_EnableAuxSyncPIP(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	if (OnOff==ON)	MDINDLY_mSec(80);	// delay 80ms when ON
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x144, 6, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	if (OnOff==OFF)	MDINDLY_mSec(40);	// delay 40ms when OFF
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableAuxDisplay(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	if (pINFO->dacPATH==DAC_PATH_MAIN_PIP) return MDIN3xx_EnableAuxSyncPIP(pINFO, OnOff);

	if (OnOff)	pINFO->dspFLAG |=  MDIN_AUX_DISPLAY_ON;
	else		pINFO->dspFLAG &= ~MDIN_AUX_DISPLAY_ON;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x142, 0, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	if (OnOff==ON)	MDINDLY_mSec(80);	// delay 80ms when ON

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x14c, 9, 1, RBIT(OnOff,1))) return MDIN_I2C_ERROR;
	if (OnOff==OFF)	MDINDLY_mSec(40);	// delay 40ms when OFF
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableAuxFreeze(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	if (OnOff)	pINFO->dspFLAG |=  MDIN_AUX_FREEZE_ON;
	else		pINFO->dspFLAG &= ~MDIN_AUX_FREEZE_ON;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x142, 1, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableAuxWithMainOSD(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	if (OnOff)	pINFO->dspFLAG |=  MDIN_AUX_MAINOSD_ON;
	else		pINFO->dspFLAG &= ~MDIN_AUX_MAINOSD_ON;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a5, 7, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetInDataMapMode(MDIN_IN_DATA_MAP_t mode)
{
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x000, 4, 3, mode)) return MDIN_I2C_ERROR;	// in data map mode
	return MDIN_NO_ERROR;
}

#if	defined(SYSTEM_USE_MDIN325)||defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380)
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetDIGOutMapMode(MDIN_DIG_OUT_MAP_t mode)
{
#if	defined(SYSTEM_USE_MDIN325)||defined(SYSTEM_USE_MDIN325A)
	mode = (MDIN_DIG_OUT_MAP_t)((mode&0xf0)|MDIN_DIG_OUT_M_MAP5);	// fix map mode 5 for MDIN325
#endif

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x0a5, 0, 4, LO4BIT(mode))) return MDIN_I2C_ERROR;	// main dig out mode
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x141, 8, 4, LO4BIT(mode))) return MDIN_I2C_ERROR;	// aux dig out mode
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x141, 7, 1, HI4BIT(mode))) return MDIN_I2C_ERROR;	// main/aux select
	return MDIN_NO_ERROR;
}
#endif

#if	defined(SYSTEM_USE_MDIN380)&&defined(SYSTEM_USE_BUS_HIF)
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetHostDataMapMode(MDIN_HOST_DATA_MAP_t mode)
{
	if (MDINI2C_RegField(MDIN_HOST_ID, 0x08e, 0, 2, mode)) return MDIN_I2C_ERROR;	// host data map mode
	return MDIN_NO_ERROR;
}
#endif

#if	defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380)
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetFormat4CHID(PMDIN_VIDEO_INFO pINFO, MDIN_4CHID_FORMAT_t mode)
{
	pINFO->st4CH_x.chID = mode;
	mode = (MDIN_4CHID_FORMAT_t)((pINFO->st4CH_x.chID==MDIN_4CHID_IN_SYNC)? 3 : 0);
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x000, 14, 2, mode)) return MDIN_I2C_ERROR;	// chipID read mode
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetOrder4CHID(PMDIN_VIDEO_INFO pINFO, MDIN_4CHID_ORDER_t mode)
{
	pINFO->st4CH_x.order = mode;
	return MDIN3xx_Set4CHProcess(pINFO);
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetDisplay4CH(PMDIN_VIDEO_INFO pINFO, MDIN_4CHVIEW_MODE_t mode)
{
	pINFO->st4CH_x.view = mode;
	return MDIN3xx_SetScaleProcess(pINFO);
}
#endif	/* defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380) */
/*
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableMirrorH(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_MFCSCALE_INFO pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_x;

	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x01c, 0, 11, (OnOff)? pMFC->stFFC.dw-256 : 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x01c, 15, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}
*/
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_EnableMirrorV(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	PMDIN_MFCSCALE_INFO	pMFC = (PMDIN_MFCSCALE_INFO)&pINFO->stMFC_m;
	WORD wVal, nID = (pSRC->stATTB.attb&MDIN_USE_INPORT_A)? 1 : 0;

	wVal = (OnOff)? pMFC->stMEM.h-1 : 0;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x01d, 0, 11, wVal)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x01c, 14, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;

	// inverse input field-id
	wVal = MBIT(pSRC->fine,MDIN_HIGH_IS_TOPFLD);	wVal = (OnOff)? ~wVal : wVal;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x001, 0+nID, 1, wVal&0x01)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}
/*
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_PowerSaveEnable(BOOL OnOff)
{
	if (OnOff==ON) {	// Power Save
		if (MDIN3xx_EnableClockDrive(MDIN_CLK_DRV_ALL, OFF)) return MDIN_I2C_ERROR;
		if (MDIN3xx_EnableDACPortOut(OFF))					 return MDIN_I2C_ERROR;
		if (MDINHIF_RegField(0x020, 0, 1, 1))	return MDIN_I2C_ERROR; // disable VPLL
		if (MDINHIF_RegField(0x020, 1, 1, 1))	return MDIN_I2C_ERROR; // disable MPLL
	}
	else {
		if (MDIN3xx_EnableClockDrive(MDIN_CLK_DRV_ALL, ON))	return MDIN_I2C_ERROR;
		if (MDIN3xx_EnableDACPortOut(ON))					return MDIN_I2C_ERROR;
		if (MDINHIF_RegField(0x020, 0, 1, 0))	return MDIN_I2C_ERROR; // enable VPLL
		if (MDINHIF_RegField(0x020, 1, 1, 0))	return MDIN_I2C_ERROR; // enable MPLL
	}
	return MDIN_NO_ERROR;
}
*/
//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Input Test Pattern & Output Test Pattern
//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetSrcVideoRestore(PMDIN_VIDEO_INFO pINFO, MDIN_IN_TEST_t mode)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	WORD rVal, nID = (pSRC->stATTB.attb&MDIN_USE_INPORT_A)? 1 : 0;

	if (mode!=MDIN_IN_TEST_DISABLE) return MDIN_NO_ERROR;

	// in_test_ptrn
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x042, 0, 6, 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x042, 7, 1, 0)) return MDIN_I2C_ERROR;

	// clk_m1_sel or clk_m2_sel
	if (MDINHIF_RegRead(MDIN_LOCAL_ID, 0x002, &rVal)) return MDIN_I2C_ERROR;
	rVal = (nID)? ((rVal>>4)&3) : ((rVal>>12)&3);	rVal = (rVal==2)? 1 : 0;
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x049-nID, 13, 3, rVal)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_SetSrcVideoPattern(PMDIN_VIDEO_INFO pINFO, MDIN_IN_TEST_t mode)
{
	PMDIN_SRCVIDEO_INFO pSRC = (PMDIN_SRCVIDEO_INFO)&pINFO->stSRC_m;
	WORD rVal, nID = (pSRC->stATTB.attb&MDIN_USE_INPORT_A)? 1 : 0;

	if (mode==MDIN_IN_TEST_DISABLE) return MDIN_NO_ERROR;

	// in_test_ptrn
	rVal  = (mode<<2)|((pSRC->stATTB.Hact<1280)? 0 : 2);
	if ((pSRC->stATTB.attb&MDIN_SCANTYPE_PROG)==0)	rVal |= (1<<0);
	if ((pSRC->stATTB.attb&MDIN_COLORSPACE_YUV)==0) rVal |= (1<<5);
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x042, 0, 6, rVal)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x042, 7, 1, 1)) return MDIN_I2C_ERROR;

	// clk_m1_sel or clk_m2_sel
	if (MDINHIF_RegField(MDIN_HOST_ID, 0x049-nID, 13, 3, 0)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetSrcTestPattern(PMDIN_VIDEO_INFO pINFO, MDIN_IN_TEST_t mode)
{
	if (MDIN3xx_SetSrcVideoPattern(pINFO, mode)) return MDIN_I2C_ERROR;
	if (MDIN3xx_SetSrcVideoRestore(pINFO, mode)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetOutTestPattern(MDIN_OUT_TEST_t mode)
{
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x043, 2, 4, mode)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}
/*
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_SetOutMCHPattern(MDIN_OUT_TEST_t mode, MDIN_MCH_TEST_ID_t nID)
{
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x043, 2, 4, 15)) return MDIN_I2C_ERROR;

	if (nID==MDIN_MCH_TEST_OFF) return MDIN_NO_ERROR;
	if (MDINHIF_RegField(MDIN_LOCAL_ID, 0x044, 12-nID*4, 4, mode)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}
*/
//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for Auto-Sync Detection
//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDIN3xx_GetSyncDone(void)
{
	WORD rVal = 0, count = 100;

	while (count&&(rVal==0)) {
		if (MDINHIF_RegRead(MDIN_LOCAL_ID, 0x0d0, &rVal)) return MDIN_I2C_ERROR;
		rVal &= 0x8000; count--;	MDINDLY_mSec(10);	// delay 10ms
	}
	if (count==0) mdinERR = 5;
	return (count)? MDIN_NO_ERROR : MDIN_TIMEOUT_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDIN3xx_GetSyncInfo(PMDIN_AUTOSYNC_INFO pAUTO, WORD hclk)
{
	WORD rBuff[6], nID = (pAUTO->port&MDIN_AUTOSYNC_INB)? 0x10 : 0x00;

	memset((PBYTE)pAUTO, 0, sizeof(MDIN_AUTOSYNC_INFO)); 	// clear
	pAUTO->port = (nID)? MDIN_AUTOSYNC_INB : MDIN_AUTOSYNC_INA;

	// check input test pattern
	if (MDINHIF_RegRead(MDIN_LOCAL_ID, 0x042, rBuff)) return MDIN_I2C_ERROR;
	if (rBuff[0]&0x0080) pAUTO->err |= AUTO_SYNC_SRC_PTRN;
	if (rBuff[0]&0x0080) return MDIN_NO_ERROR;

	// enable auto-detection
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x0d0, 0x1001)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x0d9, 0x0001)) return MDIN_I2C_ERROR;

	if (MDIN3xx_GetSyncDone()) return MDIN_TIMEOUT_ERROR;	// check done flag

	// disable auto-detection
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x0d0, 0x1000)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_LOCAL_ID, 0x0d9, 0x0000)) return MDIN_I2C_ERROR;

	// get sync_chg and sync_lost
	if (MDINHIF_RegRead(MDIN_LOCAL_ID, 0xe0, rBuff)) return MDIN_I2C_ERROR;
	pAUTO->sync = (nID)? HIBYTE(rBuff[0])&3 : LOBYTE(rBuff[0])&3;

	if (MDINHIF_MultiRead(MDIN_LOCAL_ID, 0xe4+nID, (PBYTE)rBuff, 12)) return MDIN_I2C_ERROR;
	if (rBuff[1]&0x0001) pAUTO->sync |= MDIN_AUTOSYNC_INTR;		// get scan-type
	pAUTO->Hfrq = ((DWORD)hclk*100000/(rBuff[0]&0x7fff));		// get H-freq
	pAUTO->Htot = (rBuff[2]&0x7fff)+(rBuff[3]&0x0fff);			// get H-total
	pAUTO->Vtot = (rBuff[4]&0x0fff)+(rBuff[5]&0x0fff);			// get V-total
	pAUTO->Vfrq = ((DWORD)pAUTO->Hfrq)*10/pAUTO->Vtot;			// get V-freq

	if ((rBuff[0]&0x8000)==0) pAUTO->err |= AUTO_SYNC_BAD_HFRQ;
	if ((rBuff[1]&0x8000)==0) pAUTO->err |= AUTO_SYNC_BAD_SCAN;
	if ((rBuff[2]&0x8000)==0) pAUTO->err |= AUTO_SYNC_BAD_HTOT;
	if ((rBuff[3]&0x8000)==0) pAUTO->err |= AUTO_SYNC_BAD_HTOT;
	if ((rBuff[4]&0x8000)==0) pAUTO->err |= AUTO_SYNC_BAD_VTOT;
	if ((rBuff[5]&0x8000)==0) pAUTO->err |= AUTO_SYNC_BAD_VTOT;
/*
#if __MDIN3xx_DBGPRT__ == 1
	UARTprintf("[AUTO-SYNC] error is 0x%02X\n", pAUTO->err);
	UARTprintf("[AUTO-SYNC] sync status is 0x%02X\n", pAUTO->sync);
	UARTprintf("[AUTO-SYNC] H-freq is %d/100 Hz\n", pAUTO->Hfrq);
	UARTprintf("[AUTO-SYNC] H-total is %d\n", pAUTO->Htot);
	UARTprintf("[AUTO-SYNC] V-freq is %d Hz\n", pAUTO->Vfrq);
	UARTprintf("[AUTO-SYNC] V-total is %d\n", pAUTO->Vtot);
#endif

	if (nID) return MDIN_NO_ERROR;	// if portB, active info is not exist.

	if (MDINHIF_MultiRead(MDIN_LOCAL_ID, 0xee, (PBYTE)rBuff, 8)) return MDIN_I2C_ERROR;
	pAUTO->Hact = (rBuff[1]&0x0fff)-(rBuff[0]&0x0fff);			// get H-active
	pAUTO->Vact = (rBuff[3]&0x0fff)-(rBuff[2]&0x0fff);			// get V-active

	if ((rBuff[0]&0x8000)==0) pAUTO->err |= AUTO_SYNC_BAD_HACT;
	if ((rBuff[1]&0x8000)==0) pAUTO->err |= AUTO_SYNC_BAD_HACT;
	if ((rBuff[2]&0x8000)==0) pAUTO->err |= AUTO_SYNC_BAD_VACT;
	if ((rBuff[3]&0x8000)==0) pAUTO->err |= AUTO_SYNC_BAD_VACT;

#if __MDIN3xx_DBGPRT__ == 1
	UARTprintf("[AUTO-SYNC] H-active is %d\n", pAUTO->Hact);
	UARTprintf("[AUTO-SYNC] V-active is %d\n", pAUTO->Vact);
#endif
*/
	return MDIN_NO_ERROR;
}

/*  FILE_END_HERE */
