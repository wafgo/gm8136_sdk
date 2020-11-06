//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2008  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:  MDIN3xx.H
// Description 		:  This file contains typedefine for the driver files	
// Ref. Docment		: 
// Revision History 	:

#ifndef		__MDIN3xx_H__
#define		__MDIN3xx_H__

// -----------------------------------------------------------------------------
// Include files
// -----------------------------------------------------------------------------
#ifndef		__MDINTYPE_H__
#include	 "mdintype.h"
#endif
#ifndef		__MDINDLY_H__
#include	 "mdindly.h"
#endif
#ifndef		__MDINI2C_H__
#include	 "mdini2c.h"
#endif
#ifndef		__MDINBUS_H__
#include	 "mdinbus.h"
#endif
#ifndef		__MDINPCI_H__
#include	 "mdinpci.h"
#endif
#ifndef		__MDINOSD_H__
#include	 "mdinosd.h"
#endif
#ifndef		__MDINCUR_H__
#include	 "mdincur.h"
#endif
#ifndef		__MDINGAC_H__
#include	 "mdingac.h"
#endif

// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------

typedef	enum {
#if	defined(SYSTEM_USE_MDIN380)
	MDIN_IN_DATA36_MAP0 = 0,	// input data 36pin map mode 0
	MDIN_IN_DATA36_MAP1 = 1,	// input data 36pin map mode 1
	MDIN_IN_DATA36_MAP2 = 2,	// input data 36pin map mode 2
	MDIN_IN_DATA36_MAP3 = 3,	// input data 36pin map mode 3
#endif

	MDIN_IN_DATA24_MAP0 = 4,	// input data 24pin map mode 0
	MDIN_IN_DATA24_MAP2 = 6,	// input data 24pin map mode 2
	MDIN_IN_DATA24_MAP3 = 7		// input data 24pin map mode 3

}	MDIN_IN_DATA_MAP_t;

typedef	enum {
	MDIN_DIG_OUT_M_NONE = 0,	// output data map mode 0 for main video

#if	defined(SYSTEM_USE_MDIN380)
	MDIN_DIG_OUT_M_MAP0 = 0,	// output data map mode 0 for main video
	MDIN_DIG_OUT_M_MAP1,		// output data map mode 1 for main video
	MDIN_DIG_OUT_M_MAP2,		// output data map mode 2 for main video
	MDIN_DIG_OUT_M_MAP3,		// output data map mode 3 for main video
	MDIN_DIG_OUT_M_MAP4,		// output data map mode 4 for main video
	MDIN_DIG_OUT_M_MAP5,		// output data map mode 5 for main video
	MDIN_DIG_OUT_M_MAP6,		// output data map mode 6 for main video
	MDIN_DIG_OUT_M_MAP7,		// output data map mode 7 for main video
	MDIN_DIG_OUT_M_MAP8,		// output data map mode 8 for main video
	MDIN_DIG_OUT_M_MAP9,		// output data map mode 9 for main video
	MDIN_DIG_OUT_M_MAP10,		// output data map mode 10 for main video
	MDIN_DIG_OUT_M_MAP11,		// output data map mode 11 for main video
	MDIN_DIG_OUT_M_MAP12,		// output data map mode 12 for main video
	MDIN_DIG_OUT_M_MAP13,		// output data map mode 13 for main video
	MDIN_DIG_OUT_M_MAP14,		// output data map mode 14 for main video
	MDIN_DIG_OUT_M_MAP15,		// output data map mode 15 for main video

	MDIN_DIG_OUT_X_MAP0,		// output data map mode 0 for aux video
	MDIN_DIG_OUT_X_MAP1,		// output data map mode 1 for aux video
	MDIN_DIG_OUT_X_MAP2,		// output data map mode 2 for aux video
	MDIN_DIG_OUT_X_MAP3,		// output data map mode 3 for aux video
	MDIN_DIG_OUT_X_MAP4,		// output data map mode 4 for aux video
	MDIN_DIG_OUT_X_MAP5,		// output data map mode 5 for aux video
	MDIN_DIG_OUT_X_MAP6,		// output data map mode 6 for aux video
	MDIN_DIG_OUT_X_MAP7,		// output data map mode 7 for aux video
	MDIN_DIG_OUT_X_MAP8,		// output data map mode 8 for aux video
	MDIN_DIG_OUT_X_MAP9,		// output data map mode 9 for aux video
	MDIN_DIG_OUT_X_MAP10,		// output data map mode 10 for aux video
	MDIN_DIG_OUT_X_MAP11,		// output data map mode 11 for aux video
	MDIN_DIG_OUT_X_MAP12,		// output data map mode 12 for aux video
	MDIN_DIG_OUT_X_MAP13,		// output data map mode 13 for aux video
	MDIN_DIG_OUT_X_MAP14,		// output data map mode 14 for aux video
	MDIN_DIG_OUT_X_MAP15		// output data map mode 15 for aux video
#endif

#if	defined(SYSTEM_USE_MDIN325)||defined(SYSTEM_USE_MDIN325A)
	MDIN_DIG_OUT_M_MAP5 = 5,	// output data map mode 5 for main video
	MDIN_DIG_OUT_X_MAP5 = 21	// output data map mode 5 for aux video
#endif

}	MDIN_DIG_OUT_MAP_t;

typedef	enum {
	MDIN_HOST_DATA_NONE = 0,	// host data map mode 0

#if	defined(SYSTEM_USE_MDIN380)
	MDIN_HOST_DATA_MAP0 = 0,	// host data map mode 0
	MDIN_HOST_DATA_MAP1 = 1,	// host data map mode 1
	MDIN_HOST_DATA_MAP2 = 2,	// host data map mode 2
	MDIN_HOST_DATA_MAP3 = 3		// host data map mode 3
#endif

}	MDIN_HOST_DATA_MAP_t;

typedef	enum {
	MDIN_HCLK_CRYSTAL = 0,		// host clk is crystal
	MDIN_HCLK_MEM_DIV = 3,		// host clk is mclk/2

#if	defined(SYSTEM_USE_MDIN380)
	MDIN_HCLK_HCLK_IN = 1,		// host clk is HCLK_IN
#endif

}	MDIN_HOST_CLK_MODE_t;

typedef	enum {
	MDIN_COLUMN_8BIT = 0,		// column address is 8bit(256Mb DDR)
	MDIN_COLUMN_7BIT = 1		// column address is 7bit(128Mb DDR)

}	MDIN_COLUMN_ADDR_t;

typedef	enum {
	MDIN_DID_325	= 0x90,		// MDIN-325
	MDIN_DID_340	= 0x91,		// MDIN-340
	MDIN_DID_380	= 0x92,		// MDIN-380
	MDIN_DID_325A	= 0x93,		// MDIN-325A
	MDIN_DID_345A	= 0x94		// MDIN-345A

}	MDIN_DEVICE_ID_t;

typedef	enum {
	MDIN_PRIORITY_NORM = 0,		// priority value for memory request is normal
	MDIN_PRIORITY_HIGH			// priority value for memory request is high

}	MDIN_PRIORITY_t;

typedef	enum {
	MDIN_IRQ_VSYNC_PULSE = 0,	// enable/disable IRQ(Vertical sync pulse)
	MDIN_IRQ_TIMER = 1,			// enable/disable IRQ(Timer)
	MDIN_IRQ_HDMI_TX = 7,		// enable/disable IRQ(HDMI)
	MDIN_IRQ_HDMI_CEC = 9,		// enable/disable IRQ(CEC)
	MDIN_IRQ_GA_FINISH = 14		// enable/disable IRQ(GA)

}	MDIN_IRQ_FLAG_t;

typedef	enum {
	MDIN_PAD_DAC_OUT = 0,		// enable/disable analog DAC output
	MDIN_PAD_DATA_OUT,			// enable/disable digital data output
	MDIN_PAD_CLOCK_OUT,			// enable/disable digital clock output
	MDIN_PAD_SYNC_OUT,			// enable/disable HSYNC_OUT & VSYNC_OUT
	MDIN_PAD_DE_OUT,			// enable/disable DE_OUT
	MDIN_PAD_ENC_OUT,			// enable/disable encoder DAC output
/*
	MDIN_PAD_FLDID_OUT,			// enable/disable FIELDID_A
	MDIN_PAD_VACT_OUT,			// enable/disable VACTIVE_A
	MDIN_PAD_HACT_OUT,			// enable/disable HACTIVE_A
*/
	MDIN_PAD_ALL_OUT			// enable/disable all above port

}	MDIN_PAD_OUT_t;

typedef	enum {
	MDIN_PLL_SOURCE_CLKB = 0,	// CLKB is internal PLL source for video clock
	MDIN_PLL_SOURCE_XTAL,		// XTAL is internal PLL source for video clock
	MDIN_PLL_SOURCE_CLKA,		// CLKA is internal PLL source for video clock
	MDIN_PLL_SOURCE_HACT		// HACT is internal PLL source for video clock

}	MDIN_PLL_SOURCE_t;

typedef	enum {
	MDIN_CLK_DRV_VCLK = 0,		// enable/disable vclk(video clock)
	MDIN_CLK_DRV_VCLK_OUT,		// enable/disable vclk_out(video output clock)
	MDIN_CLK_DRV_AUX,			// enable/disable clk_aux_in, clk_aux_lm, clk_aux_out
	MDIN_CLK_DRV_DAC,			// enable/disable dac_clk(DAC clock)
	MDIN_CLK_DRV_MCLK,			// enable/disable mclk, ddr_clk(memory clock)
	MDIN_CLK_DRV_HDMIP,			// enable/disable hdmi_pclk
	MDIN_CLK_DRV_VENC,			// enable/disable venc_clk, venc_dac_clk
	MDIN_CLK_DRV_HDMIV,			// enable/disable hdmi_iclk, hdmi_vclk
	MDIN_CLK_DRV_HDMIO,			// enable/disable hdmi_oclk, hdmi_cecclk
	MDIN_CLK_DRV_HDMIAS,		// enable/disable hdmi_asclk
	MDIN_CLK_DRV_HDMIS,			// enable/disable hdmi_mclk, hdmi_sck
	MDIN_CLK_DRV_M1,			// enable/disable clk_m1(input path)
	MDIN_CLK_DRV_M2,			// enable/disable clk_m2(input path)
	MDIN_CLK_DRV_A2,			// enable/disable clk_a2(A2 input path)
	MDIN_CLK_DRV_AB1,			// enable/disable clk_a1, clk_b1(A1 & B1 input path)
	MDIN_CLK_DRV_B2,			// enable/disable clk_b2(B2 input path)
	MDIN_CLK_DRV_ALL			// enable/disable all above clock

}	MDIN_CLK_DRV_t;

typedef	enum {
	MDIN_MFCFILT_HY = 0,		// horizontal interpolation filter for Y
	MDIN_MFCFILT_HC,			// horizontal interpolation filter for C
	MDIN_MFCFILT_VY,			// vertical interpolation filter for Y
	MDIN_MFCFILT_VC				// vertical interpolation filter for C

}	MDIN_MFC_FILTER_t;

typedef	enum {
	MDIN_YtoC_DELAY0 = 0,		// no adjustment
	MDIN_YtoC_DELAY3,			// Y is delayed by 3-pixel with respect to C
	MDIN_YtoC_DELAY2,			// Y is delayed by 2-pixel with respect to C
	MDIN_YtoC_DELAY1,			// Y is delayed by 1-pixel with respect to C
	MDIN_CtoY_DELAY0,			// no adjustment
	MDIN_CtoY_DELAY1,			// C is delayed by 1-pixel with respect to Y
	MDIN_CtoY_DELAY2,			// C is delayed by 2-pixel with respect to Y
	MDIN_CtoY_DELAY3			// C is delayed by 3-pixel with respect to Y

}	MDIN_YC_OFFSET_t;

typedef	enum {
	MDIN_DEOUT_PIN_DE = 0,		// 2-D active signal is output
	MDIN_DEOUT_PIN_COMP,		// composite sync signal is output
	MDIN_DEOUT_PIN_FLDID,		// fieldid signal is output
	MDIN_DEOUT_PIN_HACT			// hactive signal is output

}	MDIN_DEOUT_MODE_t;

typedef	enum {
	MDIN_HSOUT_PIN_HSYNC = 0,	// hsync signal is output
	MDIN_HSOUT_PIN_COMP,		// composite sync signal is output
	MDIN_HSOUT_PIN_DE,			// 2-D active signal is output
	MDIN_HSOUT_PIN_HACT			// hactive signal is output

}	MDIN_HSOUT_MODE_t;

typedef	enum {
	MDIN_VSOUT_PIN_VSYNC = 0,	// vsync signal is output
	MDIN_VSOUT_PIN_COMP,		// composite sync signal is output
	MDIN_VSOUT_PIN_DE,			// 2-D active signal is output
	MDIN_VSOUT_PIN_VACT			// vactive signal is output

}	MDIN_VSOUT_MODE_t;

//--------------------------------------------------------------------------------------------------------------------------
// for Video Process & MFC filter
//--------------------------------------------------------------------------------------------------------------------------
typedef	struct
{
	WORD	w;			// Horizontal size of video area
	WORD	h;			// Vertical size of video area
	WORD	x;			// Horizontal position of the top-left corner of video area
	WORD	y;			// Vertical position of the top-left corner of video area

}	stPACKED MDIN_VIDEO_WINDOW, *PMDIN_VIDEO_WINDOW;

typedef	struct
{
	WORD	sw;			// Horizontal size of source video for size-scaling
	WORD	sh;			// vertical size of source video for size-scaling
	WORD	dw;			// Horizontal size of destination video for size-scaling
	WORD	dh;			// vertical size of destination video for size-scaling

}	stPACKED MDIN_SCALE_FACTOR, *PMDIN_SCALE_FACTOR;

typedef	struct
{
	WORD	coef[9];		// CSC coefficient 0~8
	WORD	in[3];			// added to G/Y,B/Cb,R/Cr channel before CSC
	WORD	out[3];			// added to G/Y,B/Cb,R/Cr channel after CSC
	WORD	ctrl;			// for CSC control & 32-bit alignment

}	stPACKED MDIN_CSCCTRL_INFO, *PMDIN_CSCCTRL_INFO;

typedef	struct
{
	WORD	lower[64];		// coefficients of lower 32-phase of mfc filter
	WORD	upper[64];		// coefficients of upper 32-phase of mfc filter

}	stPACKED MDIN_MFCFILT_COEF, *PMDIN_MFCFILT_COEF;

typedef	enum {
	VIDSRC_720x480i60 = 0,		// 720x480i 60Hz
	VIDSRC_720x576i50,			// 720x576i 50Hz
	VIDSRC_720x480p60,			// 720x480p 60Hz
	VIDSRC_720x576p50,			// 720x576p 50Hz
	VIDSRC_1280x720p60,			// 1280x720p 60Hz
	VIDSRC_1280x720p50,			// 1280x720p 50Hz
	VIDSRC_1920x1080i60,		// 1920x1080i 60Hz
	VIDSRC_1920x1080i50,		// 1920x1080i 50Hz
	VIDSRC_1920x1080p60,		// 1920x1080p 60Hz
	VIDSRC_1920x1080p50,		// 1920x1080p 50Hz

	VIDSRC_640x480p60,			// 640x480p 60Hz
	VIDSRC_800x600p60,			// 800x600p 60Hz
	VIDSRC_1024x768p60,			// 1024x768p 60Hz
	VIDSRC_1280x1024p60,		// 1280x1024p 60Hz

	VIDSRC_1280x720pRGB,		// 1280x720p 60Hz
	VIDSRC_1280x960p60,			// 1280x960p 60Hz
	VIDSRC_1360x768p60,			// 1360x768p 60Hz
	VIDSRC_1440x900p60,			// 1440x900p 60Hz

#if VGA_SOURCE_EXTENSION == 1
	VIDSRC_640x480p72,			// 640x480p 72Hz
	VIDSRC_640x480p75,			// 640x480p 75Hz
	VIDSRC_800x600p56,			// 800x600p 56Hz
	VIDSRC_800x600p72,			// 800x600p 72Hz
	VIDSRC_800x600p75,			// 800x600p 75Hz
	VIDSRC_1024x768p70,			// 1024x768p 70Hz
	VIDSRC_1024x768p75,			// 1024x768p 75Hz
	VIDSRC_1152x864p75,			// 1152x864p 75Hz
	VIDSRC_1280x800p60,			// 1280x800p 60Hz
#endif
	VIDSRC_FORMAT_END			// source format end

}	MDIN_SRCVIDEO_FORMAT_t;

typedef	enum {
	MDIN_SRC_RGB444_8 = 0,		// RGB 4:4:4 8-bit with separate sync
	MDIN_SRC_YUV444_8,			// YCbCr 4:4:4 8-bit with separate sync
	MDIN_SRC_EMB422_8,			// YCbCr 4:2:2 8-bit with embedded sync
	MDIN_SRC_SEP422_8,			// YCbCr 4:2:2 8-bit with separate sync
	MDIN_SRC_EMB422_10,			// YCbCr 4:2:2 10-bit with embedded sync
	MDIN_SRC_SEP422_10,			// YCbCr 4:2:2 10-bit with separate sync

	MDIN_SRC_MUX656_8,			// Y/C-mux 4:2:2 8-bit with embedded sync
	MDIN_SRC_SEP656_8,			// Y/C-mux 4:2:2 8-bit with seperate sync
	MDIN_SRC_MUX656_10,			// Y/C-mux 4:2:2 10-bit with embedded sync
	MDIN_SRC_SEP656_10			// Y/C-mux 4:2:2 10-bit with seperate sync

}	MDIN_SRCPORT_MODE_t;

typedef	struct
{
	WORD	Htot;		// horizontal active size of video
	WORD	attb;		// H/V sync polarity, scan type, etc
	WORD	Hact;		// horizontal active size of video
	WORD	Vact;		// verical active size of video

}	stPACKED MDIN_SRCVIDEO_ATTB, *PMDIN_SRCVIDEO_ATTB;

typedef	struct
{
	BYTE	frmt;		// video format of input (one of MDIN_SRCVIDEO_FORMAT_t)
	BYTE	mode;		// video mode of input port (one of MDIN_SRCPORT_MODE_t)
	WORD	fine;		// fine control of CbCr-swap, in-FieldID, H-active, etc

	WORD	offH;		// pixel offset between HACT and video active
	WORD	offV;		// line offset between VACT and video active

	MDIN_SRCVIDEO_ATTB	stATTB;		// attribute of input video
	PMDIN_CSCCTRL_INFO	pCSC;		// input CSC control

}	stPACKED MDIN_SRCVIDEO_INFO, *PMDIN_SRCVIDEO_INFO;

typedef	struct
{
	WORD	posHD;		// the position of horizontal reference pulse
	WORD	posVD;		// the position of vertical reference pulse
	WORD	posVB;		// the position of vertical reference pulse for bottom field

	WORD	totHS;		// the horizontal total size of output video
	WORD	bgnHA;		// the start position of horizontal active output
	WORD	endHA;		// the end position of horizontal active output
	WORD	bgnHS;		// the start position of horizontal sync output
	WORD	endHS;		// the end position of horizontal sync output

	WORD	totVS;		// the vertical total size of output video
	WORD	bgnVA;		// the start position of vertical active output
	WORD	endVA;		// the end position of vertical active output
	WORD	bgnVS;		// the start position of vertical sync output
	WORD	endVS;		// the end position of vertical sync output

	WORD	bgnVAB;		// the start position of Vact for the bottom field
	WORD	endVAB;		// the end position of Vact for the bottom field
	WORD	bgnVSB;		// the start position of Vsync for the bottom field
	WORD	endVSB;		// the end position of Vsync for the bottom field
	WORD	posFLD;		// the transition position of Vsync for the bottom field

	WORD	vclkP;		// Pre-divider value of the internal PLL for video clock
	WORD	vclkM;		// Post-divider value of the internal PLL for video clock
	WORD	vclkS;		// Post-scaler value of the internal PLL for video clock

	WORD	xclkS;		// S parameter of fractional divider in aux video clock
	WORD	xclkF;		// F parameter of fractional divider in aux video clock
	WORD	xclkT;		// T parameter of fractional divider in aux video clock

}	stPACKED MDIN_OUTVIDEO_SYNC, *PMDIN_OUTVIDEO_SYNC;

typedef	struct
{
	WORD	ctrl[37];	// control the operation of the internal DAC
	WORD	dummy;		// dummy for 32-bit alignment

}	stPACKED MDIN_DACSYNC_CTRL, *PMDIN_DACSYNC_CTRL;

typedef	enum {
	VIDOUT_720x480i60 = 0,		// 720x480i 60Hz
	VIDOUT_720x576i50,			// 720x576i 50Hz
	VIDOUT_720x480p60,			// 720x480p 60Hz
	VIDOUT_720x576p50,			// 720x576p 50Hz
	VIDOUT_1280x720p60,			// 1280x720p 60Hz
	VIDOUT_1280x720p50,			// 1280x720p 50Hz
	VIDOUT_1920x1080i60,		// 1920x1080i 60Hz
	VIDOUT_1920x1080i50,		// 1920x1080i 50Hz
	VIDOUT_1920x1080p60,		// 1920x1080p 60Hz
	VIDOUT_1920x1080p50,		// 1920x1080p 50Hz

	VIDOUT_640x480p60,			// 640x480p 60Hz
	VIDOUT_640x480p75,			// 640x480p 75Hz
	VIDOUT_800x600p60,			// 800x600p 60Hz
	VIDOUT_800x600p75,			// 800x600p 75Hz
	VIDOUT_1024x768p60,			// 1024x768p 60Hz
	VIDOUT_1024x768p75,			// 1024x768p 75Hz
	VIDOUT_1280x1024p60,		// 1280x1024p 60Hz
	VIDOUT_1280x1024p75,		// 1280x1024p 75Hz

	VIDOUT_1360x768p60,			// 1360x768p 60Hz
	VIDOUT_1600x1200p60,		// 1600x1200p 60Hz

	VIDOUT_1440x900p60,			// 1440x900p 60Hz
	VIDOUT_1440x900p75,			// 1440x900p 75Hz
	VIDOUT_1680x1050p60,		// 1680x1050p 60Hz

	VIDOUT_1680x1050pRB,		// 1680x1050pRB 60Hz
	VIDOUT_1920x1080pRB,		// 1920x1080pRB 60Hz
	VIDOUT_1920x1200pRB,		// 1920x1200pRB 60Hz
	VIDOUT_FORMAT_END			// output format end

}	MDIN_OUTVIDEO_FORMAT_t;

typedef	enum {
	MDIN_OUT_RGB444_8 = 0,		// RGB 4:4:4 8-bit with separate sync on digital out port
	MDIN_OUT_YUV444_8,			// YCbCr 4:4:4 8-bit with separate sync on digital out port
	MDIN_OUT_EMB422_8,			// YCbCr 4:2:2 8-bit with embedded sync on digital out port
	MDIN_OUT_SEP422_8,			// YCbCr 4:2:2 8-bit with separate sync on digital out port
	MDIN_OUT_EMB422_10,			// YCbCr 4:2:2 10-bit with embedded sync on digital out port
	MDIN_OUT_SEP422_10,			// YCbCr 4:2:2 10-bit with separate sync on digital out port
	MDIN_OUT_MUX656_8,			// Y/C-mux 4:2:2 8-bit with embedded sync on digital out port
	MDIN_OUT_MUX656_10			// Y/C-mux 4:2:2 10-bit with embedded sync on digital out port

}	MDIN_OUTPORT_MODE_t;

typedef	struct
{
	WORD	Htot;		// horizontal active size of video
	WORD	attb;		// H/V sync polarity, scan type, etc
	WORD	Hact;		// horizontal active size of video
	WORD	Vact;		// verical active size of video

}	stPACKED MDIN_OUTVIDEO_ATTB, *PMDIN_OUTVIDEO_ATTB;

typedef	struct
{
	BYTE	frmt;		// video format of output (one of MDIN_OUTVIDEO_FORMAT_t)
	BYTE	mode;		// video mode of output port (one of MDIN_OUTPORT_MODE_t)
	WORD	fine;		// fine control of CbCr/PbPr-Swap, DE, H/V-act, H/V-sync, etc

	BYTE	brightness;		// default = 128, range: from 0 to 255
	BYTE	contrast;		// default = 128, range: from 0 to 255
	BYTE	saturation;		// default = 128, range: from 0 to 255
	BYTE	hue;			// default = 128, range: from 0 to 255

#if RGB_GAIN_OFFSET_TUNE == 1
	BYTE	r_gain;			// default = 128, range: 0~255, only used to RGB-out mode
	BYTE	g_gain;			// default = 128, range: 0~255, only used to RGB-out mode
	BYTE	b_gain;			// default = 128, range: 0~255, only used to RGB-out mode
	BYTE	r_offset;		// default = 128, range: 0~255, only used to RGB-out mode
	BYTE	g_offset;		// default = 128, range: 0~255, only used to RGB-out mode
	BYTE	b_offset;		// default = 128, range: 0~255, only used to RGB-out mode
	WORD	dummy;			// dummy byte for 32-bit alignment
#endif

	MDIN_OUTVIDEO_ATTB	stATTB;		// attribute of out video
	PMDIN_OUTVIDEO_SYNC	pSYNC;		// sync-info of out video

	PMDIN_CSCCTRL_INFO	pCSC;		// output CSC control
	PMDIN_DACSYNC_CTRL	pDAC;		// output DAC control

}	stPACKED MDIN_OUTVIDEO_INFO, *PMDIN_OUTVIDEO_INFO;

typedef	enum {
	MDIN_MAP_AUX_ON_NR_ON = 0,	// Y_m=6,Ynr=2,C_m=6,Ymh=2,Y_x=2,C_x=2
	MDIN_MAP_AUX_ON_NR_OFF,		// Y_m=6,Ynr=0,C_m=6,Ymh=2,Y_x=2,C_x=2
	MDIN_MAP_AUX_OFF_NR_ON,		// Y_m=6,Ynr=2,C_m=6,Ymh=2,Y_x=0,C_x=0
	MDIN_MAP_AUX_OFF_NR_OFF,	// Y_m=6,Ynr=0,C_m=6,Ymh=2,Y_x=0,C_x=0

	MDIN_MAP_USER_DEFINE,		// user must set Y_m,Ynr,C_m,Ymh,Y_x,C_x
	MDIN_MAP_FORMAT_END			// source format end

}	MDIN_FRAMEMAP_FORMAT_t;

typedef struct
{
	BYTE	frmt;		// map format of frame buffer (one of MDIN_FRAMEMAP_FORMAT_t)
	BYTE	dummy;		// dummy for 32-bit alignment

	BYTE	Y_m;		// number of frames in frame buffer for main-Y
	BYTE	Ynr;		// number of frames in frame buffer for main-Ynr
	BYTE	C_m;		// number of frames in frame buffer for main-C
	BYTE	Ymh;		// number of frames in frame buffer for motion history
	BYTE	Y_x;		// number of frames in frame buffer for aux-Y
	BYTE	C_x;		// number of frames in frame buffer for aux-C

}	stPACKED MDIN_FRAMEMAP_INFO, *PMDIN_FRAMEMAP_INFO;

typedef	enum {
	MDIN_DEINT_NONSTILL = 0,		// adaptive mode : combination of intra and inter
	MDIN_DEINT_ADAPTIVE,			// adaptive mode : non-motion detect
	MDIN_DEINT_INTRA_ONLY,			// only intra-field interpolation mode
	MDIN_DEINT_INTER_ONLY			// only inter-field interpolation mode

}	MDIN_DEINT_MODE_t;

typedef enum {
	MDIN_DEINT_FILM_OFF = 0,		// disables film mode
	MDIN_DEINT_FILM_32 = 5,			// enables 3:2 film mode only
	MDIN_DEINT_FILM_22 = 6,			// enables 2:2 film mode only
	MDIN_DEINT_FILM_ALL = 4			// enables both 3:2 and 2:2 film modes

}	MDIN_DEINT_FILM_t;

typedef	struct
{
	BYTE	mode;		// mode of deint (one of MDIN_DEINT_MODE_t)
	BYTE	film;		// mode of flim (one of MDIN_DEINT_FILM_t)
	BYTE	gain;		// gain of 3DNR (range is 0~60 or 0~33 in 1080p)
	BYTE	dummy;		// dummy for 32-bit alignment

	WORD	fine;		// fine control of 3dnr, ccs, cue
	WORD	attb;		// deinterlace on/off, in-Vfreq, etc

}	stPACKED MDIN_DEINTCTL_INFO, *PMDIN_DEINTCTL_INFO;

// user define for fine control of deinterlacer
#define		MDIN_DEINT_3DNR_ON			0x0001	// 3D noise reduction is on
#define		MDIN_DEINT_3DNR_OFF			0x0000	// 3D noise reduction is off
#define		MDIN_DEINT_CCS_ON			0x0002	// cross color suppression is on
#define		MDIN_DEINT_CCS_OFF			0x0000	// cross color suppression is off
#define		MDIN_DEINT_CUE_ON			0x0004	// chroma upsampling error correction is on
#define		MDIN_DEINT_CUE_OFF			0x0000	// chroma upsampling error correction is off
#define		MDIN_DEINT_FRC_DOWN			0x0008	// frame rate down conversion is down
#define		MDIN_DEINT_FRC_NORM			0x0000	// frame rate down conversion is normal

// define for attribute of deinterlacer, but only use in API function
#define		MDIN_DEINT_IPC_PROC			0x0001	// deinterlace ON
#define		MDIN_DEINT_PAL_CCS			0x0002	// PAL-CCS mode
#define		MDIN_DEINT_444_PROC			0x0004	// internal processing is 4:4:4
#define		MDIN_DEINT_IPC_2TAB			0x0008	// ipc-2tab mode ON
#define		MDIN_DEINT_p5_SCALE			0x0010	// scale ratio <= 0.5
#define		MDIN_DEINT_DN_SCALE			0x0020	// scale ratio < 1.0
#define		MDIN_DEINT_MFC_BUFF			0x0040	// mfc_buffer_en ON
#define		MDIN_DEINT_HD_1080i			0x0100	// source is 1080i
#define		MDIN_DEINT_HD_1080p			0x0200	// source is 1080p

typedef struct {
	MDIN_SCALE_FACTOR	stFFC;	// front format conversion
	MDIN_SCALE_FACTOR	stCRS;	// central region of MFC
	MDIN_SCALE_FACTOR	stORS;	// outside region of MFC

	MDIN_VIDEO_WINDOW	stCUT;	// clip window of input video
	MDIN_VIDEO_WINDOW	stSRC;	// zoom window for frame memory
	MDIN_VIDEO_WINDOW	stDST;	// view window for frame memory
	MDIN_VIDEO_WINDOW	stMEM;	// mmap window for frame memory
	MDIN_VIDEO_WINDOW	st4CH;	// view window for 4-CH display

}	stPACKED MDIN_MFCSCALE_INFO, *PMDIN_MFCSCALE_INFO;

typedef enum {
	PATH_MAIN_A_AUX_A = 0,			// main-src is port A, aux-src is port A
	PATH_MAIN_A_AUX_B,				// main-src is port A, aux-src is port B
	PATH_MAIN_A_AUX_M,				// main-src is port A, aux-src is main video
//	PATH_MAIN_B_AUX_A,				// main-src is port B, aux-src is port A
	PATH_MAIN_B_AUX_B,				// main-src is port B, aux-src is port B
	PATH_MAIN_B_AUX_M				// main-src is port B, aux-src is main video

}	MDIN_SRCVIDEO_PATH_t;

typedef enum {
	DAC_PATH_MAIN_OUT = 0,			// DAC-output is main video (RGB or YPbPr)
	DAC_PATH_AUX_OUT,				// DAC-output is aux video (RGB or YPbPr)
	DAC_PATH_VENC_YC,				// DAC-output is V-Encoder (S-Video)
	DAC_PATH_MAIN_PIP,				// out-video is PIP or POP on main
	DAC_PATH_AUX_4CH,				// out-video is 4ch-D1 multi-window
	DAC_PATH_AUX_2HD				// out-video is 2ch-HD multi-window

}	MDIN_DACVIDEO_PATH_t;

typedef enum {
	VENC_PATH_PORT_A = 0,			// VENC-source is port A
	VENC_PATH_PORT_B,				// VENC-source is port B
	VENC_PATH_PORT_X				// VENC-source is aux video

}	MDIN_VENC_SRCPATH_t;

typedef enum {
	VID_VENC_NTSC_M = 0,			// the monochrome standard for NTSC
	VID_VENC_NTSC_J,				// NTSC system in japan
	VID_VENC_NTSC_443,				// NTSC system in 4.43MHz
	VID_VENC_PAL_B,					// (B) PAL system
	VID_VENC_PAL_D,					// (D) PAL system
	VID_VENC_PAL_G,					// (G) PAL system
	VID_VENC_PAL_H,					// (H) PAL system
	VID_VENC_PAL_I,					// (I) PAL system
	VID_VENC_PAL_M,					// (M) PAL system
	VID_VENC_PAL_N,					// (N) PAL system
	VID_VENC_PAL_Nc,				// (Nc) PAL system
	VID_VENC_FMT_END				// source format end

}	MDIN_VENC_FORMAT_t;

typedef	enum {
	HDMI_OUT_RGB444_8 = 0,		// RGB 4:4:4 8-bit with separate sync on hdmi src port
	HDMI_OUT_YUV444_8 = 1,		// YCbCr 4:4:4 8-bit with separate sync on hdmi out port
	HDMI_OUT_SEP422_8 = 3		// YCbCr 4:2:2 8-bit with separate sync on hdmi out port

}	MDIN_HDMIOUT_MODE_t;

typedef struct {
	WORD	mode;		// video mode of hdmi out port (one of MDIN_HDMIOUT_MODE_t)
	WORD	fine;

}	stPACKED MDIN_HTXVIDEO_INFO, *PMDIN_HTXVIDEO_INFO;

// user define for fine control of hdmi video
#define		HDMI_DEEP_COLOR_ON			0x0001	// deep color mode ON
#define		HDMI_DEEP_COLOR_OFF			0x0000	// deep color mode OFF
#define		HDMI_CLK_EDGE_RISE			0x0010	// latch input on falling edge
#define		HDMI_CLK_EDGE_FALL			0x0000	// latch input on rising edge
#define		HDMI_USE_AUTO_FRMT			0x0020	// use native format of EDID
#define		HDMI_USE_USER_FRMT			0x0000	// use user-defined format
#define		HDMI_USE_FORCE_DVI			0x1000	// use force DVI mode

typedef	enum {
	AUDIO_INPUT_I2S_0 = 0,		// I2S0 audio input for HDMI transmitter
	AUDIO_INPUT_I2S_1,			// I2S1 audio input for HDMI transmitter
	AUDIO_INPUT_I2S_2,			// I2S2 audio input for HDMI transmitter
	AUDIO_INPUT_I2S_3,			// I2S3 audio input for HDMI transmitter
	AUDIO_INPUT_SPDIF			// SPDIF audio input for HDMI transmitter

}	MDIN_SRCAUDIO_FORMAT_t;

typedef enum
{
	AUDIO_MCLK_128Fs	= 0x00,	// audio MCLK input for HDMI transmitter
	AUDIO_MCLK_256Fs	= 0x10,
	AUDIO_MCLK_384Fs	= 0x20,
	AUDIO_MCLK_512Fs	= 0x30,
	AUDIO_MCLK_768Fs	= 0x40,
	AUDIO_MCLK_1024Fs	= 0x50,
	AUDIO_MCLK_1152Fs	= 0x60,
	AUDIO_MCLK_192Fs	= 0x70

}	MDIN_SRCAUDIO_MCLK_t;

typedef enum {
	AUDIO_FREQ_22kHz	= 0x04,	// audio sampling frequency of I2S input
	AUDIO_FREQ_24kHz	= 0x06,
	AUDIO_FREQ_32kHz	= 0x03,
	AUDIO_FREQ_44kHz	= 0x00,
	AUDIO_FREQ_48kHz	= 0x02,
	AUDIO_FREQ_88kHz	= 0x08,
	AUDIO_FREQ_96kHz	= 0x0a,
	AUDIO_FREQ_176kHz	= 0x0c,
	AUDIO_FREQ_192kHz	= 0x0e

}	MDIN_I2SAUDIO_FREQ_t;

typedef struct {
	BYTE	frmt;
	BYTE	freq;		// [one of MDIN_SRCAUDIO_MCLK_t | one of MDIN_I2SAUDIO_FREQ_t]
	WORD	fine;		// [length|attb]

}	stPACKED MDIN_HTXAUDIO_INFO, *PMDIN_HTXAUDIO_INFO;

// user define for fine control of hdmi audio
#define		AUDIO_MULTI_CHANNEL			0x8000	// use up to 8-channel

#define		AUDIO_MAX24B_MINUS4			0x0300	// audio sample length = 24 - 4 = 20bit
#define		AUDIO_MAX24B_MINUS3			0x0d00	// audio sample length = 24 - 3 = 21bit
#define		AUDIO_MAX24B_MINUS2			0x0500	// audio sample length = 24 - 2 = 22bit
#define		AUDIO_MAX24B_MINUS1			0x0900	// audio sample length = 24 - 1 = 23bit
#define		AUDIO_MAX24B_MINUS0			0x0b00	// audio sample length = 24 - 0 = 24bit

#define		AUDIO_MAX20B_MINUS4			0x0200	// audio sample length = 20 - 4 = 16bit
#define		AUDIO_MAX20B_MINUS3			0x0c00	// audio sample length = 20 - 3 = 17bit
#define		AUDIO_MAX20B_MINUS2			0x0400	// audio sample length = 20 - 2 = 18bit
#define		AUDIO_MAX20B_MINUS1			0x0800	// audio sample length = 20 - 1 = 19bit
#define		AUDIO_MAX20B_MINUS0			0x0a00	// audio sample length = 20 - 0 = 20bit

#define		AUDIO_SCK_EDGE_RISE			0x0040	// SCK sample edge is falling for I2S
#define		AUDIO_SCK_EDGE_FALL			0x0000	// SCK sample edge is rising for I2S
#define		AUDIO_WS_POLAR_HIGH			0x0008	// left polarity when WS is high for I2S
#define		AUDIO_WS_POLAR_LOW			0x0000	// left polarity when WS is low for I2S
#define		AUDIO_SD_JUST_RIGHT			0x0004	// data is right-justified for I2S
#define		AUDIO_SD_JUST_LEFT			0x0000	// data is left-justified for I2S
#define		AUDIO_SD_LSB_FIRST			0x0002	// data direction is LSB first for I2S
#define		AUDIO_SD_MSB_FIRST			0x0000	// data direction is MSB first for I2S
#define		AUDIO_SD_NONE_SHIFT			0x0001	// WS to SD first bit is no shift for I2S
#define		AUDIO_SD_1ST_SHIFT			0x0000	// WS to SD first bit is shift for I2S

typedef struct {

	BYTE	err;		// EDID error
	BYTE	type;		// display type ("2":HDMI, "1":DVI)
	BYTE	mode;		// video mode of hdmi output
	BYTE	frmt;		// video format of hdmi output
	WORD	phy;		// physical address of sink

	BYTE	proc;		// process ID for hdmi control
	BYTE	auth;		// authen ID for hdcp control

}	stPACKED MDIN_HDMICTRL_INFO, *PMDIN_HDMICTRL_INFO;

typedef	enum {
	MDIN_4CHID_IN_HBLK = 0,		// 4CH-ID extract from LSB 4 bits of H blanking
	MDIN_4CHID_IN_SYNC = 1		// 4CH-ID extract from protection bits of SAV/EAV

}	MDIN_4CHID_FORMAT_t;

typedef	enum {
	MDIN_4CHID_A1A2B1B2 = 0,	// order of 4CH-display is A1,A2,B1,B2
	MDIN_4CHID_A1B1A2B2 = 1,	// order of 4CH-display is A1,B1,A2,B2
	MDIN_4CHID_A2A1B2B1 = 2,	// order of 4CH-display is A2,A1,B2,B1
	MDIN_4CHID_A2B2A1B1 = 3,	// order of 4CH-display is A2,B2,A1,B1
	MDIN_4CHID_B1B2A1A2 = 4,	// order of 4CH-display is B1,B2,A1,A2
	MDIN_4CHID_B1A1B2A2 = 5,	// order of 4CH-display is B1,A1,B2,A2
	MDIN_4CHID_B2B1A2A1 = 6,	// order of 4CH-display is B2,B1,A2,A1
	MDIN_4CHID_B2A2B1A1 = 7		// order of 4CH-display is B2,A2,B1,A1

}	MDIN_4CHID_ORDER_t;

typedef	enum {
	MDIN_4CHVIEW_ALL = 0,		// 4CH-display is all 4-split screen
	MDIN_4CHVIEW_CH01,			// 4CH-display is only ch01 screen
	MDIN_4CHVIEW_CH02,			// 4CH-display is only ch02 screen
	MDIN_4CHVIEW_CH03,			// 4CH-display is only ch03 screen
	MDIN_4CHVIEW_CH04,			// 4CH-display is only ch04 screen
	MDIN_4CHVIEW_CH12,			// 4CH-display is dual ch01/ch02 screen
	MDIN_4CHVIEW_CH34,			// 4CH-display is dual ch03/ch04 screen
	MDIN_4CHVIEW_CH13,			// 4CH-display is dual ch01/ch03 screen
	MDIN_4CHVIEW_CH24,			// 4CH-display is dual ch02/ch04 screen
	MDIN_4CHVIEW_CH14,			// 4CH-display is dual ch01/ch04 screen
	MDIN_4CHVIEW_CH23			// 4CH-display is dual ch02/ch03 screen

}	MDIN_4CHVIEW_MODE_t;

typedef struct {

	BYTE	chID;		// CH-ID extract mode (one of MDIN_4CHID_FORMAT_t)
	BYTE	order;		// order of 4CH-display (one of MDIN_4CHID_ORDER_t)
	BYTE	view;		// view of 4CH-display (one of MDIN_4CHVIEW_MODE_t)
	BYTE	dummy;		// dummy for 32-bit alignment

}	stPACKED MDIN_4CHVIDEO_INFO, *PMDIN_4CHVIDEO_INFO;

typedef struct {
	BYTE	exeFLAG;				// the flag of execution of video process
	BYTE	dspFLAG;				// the flag of display of aux-video

	BYTE	srcPATH;				// path of input video (one of MDIN_SRCVIDEO_PATH_t)
	BYTE	dacPATH;				// path of DAC output (one of MDIN_DACVIDEO_PATH_t)

	BYTE	encPATH;				// input-path of VENC (one of MDIN_VENC_SRCPATH_t)
	BYTE	encFRMT;				// video format of VENC (one of MDIN_VENC_FORMAT_t)

	WORD	ffcH_m;					// the size of front format conversion

	MDIN_SRCVIDEO_INFO	stSRC_a;	// source video info of inport A
	MDIN_SRCVIDEO_INFO	stSRC_b;	// source video info of inport B
	MDIN_SRCVIDEO_INFO	stSRC_x;	// source video info of aux-path
									// fine/offset/CSC defined by user, others use in API function

	MDIN_OUTVIDEO_INFO	stOUT_m;	// output video info of main path
	MDIN_VIDEO_WINDOW	stCROP_m;	// crop window of main path for image cut
	MDIN_VIDEO_WINDOW	stZOOM_m;	// zoom window of main path for overscan
	MDIN_VIDEO_WINDOW	stVIEW_m;	// view window of main path for aspect ratio

	MDIN_OUTVIDEO_INFO	stOUT_x;	// output video info of aux path
	MDIN_VIDEO_WINDOW	stCROP_x;	// crop window of aux path for image cut
	MDIN_VIDEO_WINDOW	stZOOM_x;	// zoom window of aux path for overscan
	MDIN_VIDEO_WINDOW	stVIEW_x;	// view window of aux path for aspect ratio

	MDIN_FRAMEMAP_INFO	stMAP_m;	// frame buffer map of main&aux path
	MDIN_DEINTCTL_INFO	stIPC_m;	// deinterlacer of main path only

	PMDIN_MFCFILT_COEF	pHY_m;		// MFCHY filter coefficient of main path only
	PMDIN_MFCFILT_COEF	pHC_m;		// MFCHC filter coefficient of main path only
	PMDIN_MFCFILT_COEF	pVY_m;		// MFCVY filter coefficient of main path only
	PMDIN_MFCFILT_COEF	pVC_m;		// MFCVC filter coefficient of main path only

	MDIN_SRCVIDEO_INFO	stSRC_m;	// source video info of main path, but only use in API function

	MDIN_MFCSCALE_INFO	stMFC_m;	// scaler info of main path, but only use in API function
	MDIN_MFCSCALE_INFO	stMFC_x;	// scaler info of aux path, but only use in API function

#if	defined(SYSTEM_USE_MDIN340)||defined(SYSTEM_USE_MDIN380)
	MDIN_HTXVIDEO_INFO	stVID_h;	// video info of HDMI-TX path
	MDIN_HTXAUDIO_INFO	stAUD_h;	// audio info of HDMI-TX path
	MDIN_HDMICTRL_INFO	stCTL_h;	// control info of HDMI-TX, but only use in API function
#endif

#if	defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380)
	MDIN_4CHVIDEO_INFO	st4CH_x;	// video info of 4-CH display
#endif

}	stPACKED MDIN_VIDEO_INFO, *PMDIN_VIDEO_INFO;

// user define for update video process
#define		MDIN_UPDATE_MAINFMT			0x01	// update video process as main video change
#define		MDIN_UPDATE_AUXFMT			0x02	// update video process as aux video change
#define		MDIN_UPDATE_ENCFMT			0x04	// update video process as v-encoder change
#define		MDIN_UPDATE_HDMIFMT			0x08	// update video process as hdmi video change
#define		MDIN_UPDATE_CLEAR			0x00	// clear flag of update video process

// user define for display of aux-video
#define		MDIN_AUX_DISPLAY_ON			0x01	// display on of aux-video
#define		MDIN_AUX_DISPLAY_OFF		0x00	// display off of aux-video
#define		MDIN_AUX_FREEZE_ON			0x02	// freeze on of aux-video
#define		MDIN_AUX_FREEZE_OFF			0x00	// freeze off of aux-video
#define		MDIN_AUX_MAINOSD_ON			0x04	// main-osd on of aux-video
#define		MDIN_AUX_MAINOSD_OFF		0x00	// main-osd off of aux-video

// user define for fine control of source video
#define		MDIN_CbCrSWAP_ON			0x0010	// swap the ordering of chominance data
#define		MDIN_CbCrSWAP_OFF			0x0000	// swap the ordering of chominance data
#define		MDIN_FIELDID_INPUT			0x0020	// FIELDID input is used as a field-id signal
#define		MDIN_FIELDID_BYPASS			0x0000	// FIELDID is generated from H/V active signal
#define		MDIN_HIGH_IS_TOPFLD			0x0040	// high level of field-id represents top field
#define		MDIN_LOW_IS_TOPFLD			0x0000	// low level of field-id represents top field
#define		MDIN_HACT_IS_CSYNC			0x0008	// HACTIVE_A or B input is composite sync
#define		MDIN_HACT_IS_HSYNC			0x0000	// HACTIVE_A or B input is Hsync
#define		MDIN_YCOFFSET_MASK			0x0007	// masking of YC offset value

// user define for fine control of output video
//#define	MDIN_CbCrSWAP_ON			0x0010	// pre-defined at source video
//#define	MDIN_CbCrSWAP_OFF			0x0000	//
#define		MDIN_PbPrSWAP_ON			0x0020	// PbPr are swapped on DAC output
#define		MDIN_PbPrSWAP_OFF			0x0000	// PbPr are not swapped on DAC output
#define		MDIN_SYNC_FREERUN			0x0040	// out-sync runs freely regardless of in-sync
#define		MDIN_SYNC_FRMLOCK			0x0000	// out-sync is frame-locked to the in-sync
#define		MDIN_POSITIVE_DE			0x0080	// DE data region is high level
#define		MDIN_NEGATIVE_DE			0x0000	// DE data region is low level
#define		MDIN_DEOUT_IS_DE			0x0000	// DE_OUT pin is data enable signal
#define		MDIN_DEOUT_IS_COMP			0x0400	// DE_OUT pin is composite sync
#define		MDIN_DEOUT_IS_FLDID			0x0800	// DE_OUT pin is field-id signal
#define		MDIN_DEOUT_IS_HACT			0x0c00	// DE_OUT pin is Hactive signal
#define		MDIN_VSYNC_IS_VSYNC			0x0000	// VSYNC_OUT pin is Vsync signal
#define		MDIN_VSYNC_IS_COMP			0x1000	// VSYNC_OUT pin is composite sync
#define		MDIN_VSYNC_IS_DE			0x2000	// VSYNC_OUT pin is data enable signal
#define		MDIN_VSYNC_IS_VACT			0x3000	// VSYNC_OUT pin is Vactive signal
#define		MDIN_HSYNC_IS_HSYNC			0x0000	// HSYNC_OUT pin is Hsync signal
#define		MDIN_HSYNC_IS_COMP			0x4000	// HSYNC_OUT pin is composite sync
#define		MDIN_HSYNC_IS_DE			0x8000	// HSYNC_OUT pin is data enable signal
#define		MDIN_HSYNC_IS_HACT			0xc000	// HSYNC_OUT pin is Hactive signal

// user define for attribute of source & output video
#define		MDIN_NEGATIVE_HACT			0x0001	// high level contains active data region
#define		MDIN_POSITIVE_HACT			0x0000	// low level contains active data region
#define		MDIN_NEGATIVE_VACT			0x0002	// high level contains active data region
#define		MDIN_POSITIVE_VACT			0x0000	// low level contains active data region
#define		MDIN_POSITIVE_HSYNC			0x0004	// polarity of Hsync is rising edge
#define		MDIN_NEGATIVE_HSYNC			0x0000	// polarity of Hsync is falling edge
#define		MDIN_POSITIVE_VSYNC			0x0008	// polarity of Vsync is rising edge
#define		MDIN_NEGATIVE_VSYNC			0x0000	// polarity of Vsync is falling edge
#define		MDIN_SCANTYPE_PROG			0x0010	// progressive scan
#define		MDIN_SCANTYPE_INTR			0x0000	// interlaced scan

// define for attribute of source & output video, but only use in API function
#define		MDIN_WIDE_RATIO				0x0020	// aspect ratio is 16:9
#define		MDIN_NORM_RATIO				0x0000	// aspect ratio is 4:3
#define		MDIN_QUALITY_HD				0x0100	// HD quality
#define		MDIN_QUALITY_SD				0x0000	// SD quality
#define		MDIN_PRECISION_8			0x0200	// 8-bit precision per color component
#define		MDIN_PRECISION_10			0x0000	// 10-bit precision per color component
#define		MDIN_PIXELSIZE_422			0x0400	// pixel-size is 4:2:2 format
#define		MDIN_PIXELSIZE_444			0x0000	// pixel-size is 4:4:4 format
#define		MDIN_COLORSPACE_YUV			0x0800	// color space is YCbCr domain
#define		MDIN_COLORSPACE_RGB			0x0000	// color space is RGB domain
#define		MDIN_VIDEO_SYSTEM			0x1000	// video system
#define		MDIN_PCVGA_SYSTEM			0x0000	// PC-VGA system
#define		MDIN_USE_INPORT_A			0x2000	// input port A is selected for input video
#define		MDIN_USE_INPORT_B			0x0000	// input port B is selected for input video
#define		MDIN_USE_INPORT_M			0x4000	// main video is selected for input video

//--------------------------------------------------------------------------------------------------------------------------
// for Filters Block
//--------------------------------------------------------------------------------------------------------------------------
typedef	struct
{
	WORD	y[8];			// coefficients of front NR filter for Y
	WORD	c[4];			// coefficients of front NR filter for C

}	stPACKED MDIN_FNRFILT_COEF, *PMDIN_FNRFILT_COEF;

typedef	struct
{
	WORD	hy[5];			// coefficients of aux H-filter for Y
	WORD	hc[4];			// coefficients of aux H-filter for C
	WORD	vy[3];			// coefficients of aux V-filter for Y
	WORD	vc[2];			// coefficients of aux V-filter for C

}	stPACKED MDIN_AUXFILT_COEF, *PMDIN_AUXFILT_COEF;

typedef	struct
{
	WORD	coef[8];		// coefficients of horizontal peaking filter

}	stPACKED MDIN_PEAKING_COEF, *PMDIN_PEAKING_COEF;

typedef	struct
{
	WORD	coef[4];		// coefficients of NR surface filter

}	stPACKED MDIN_BNRFILT_COEF, *PMDIN_BNRFILT_COEF;

typedef	struct
{
	WORD	coef[4];		// coefficients of color enhancement filter

}	stPACKED MDIN_COLOREN_COEF, *PMDIN_COLOREN_COEF;

typedef	struct
{
	WORD	coef[9];		// coefficients of mosquito NR filter

}	stPACKED MDIN_MOSQUIT_COEF, *PMDIN_MOSQUIT_COEF;

typedef	struct
{
	WORD	coef[5];		// coefficients of skin-tone filter

}	stPACKED MDIN_SKINTON_COEF, *PMDIN_SKINTON_COEF;

typedef	struct
{
	BYTE	src[4];		// input segment point for B/W extension
	BYTE	out[4];		// output segment point for B/W extension

}	stPACKED MDIN_BWPOINT_COEF, *PMDIN_BWPOINT_COEF;

typedef	struct
{
	SHORT	lx;			// Horizontal position of the upper-left corner of inter area
	SHORT	ly;			// Vertical position of the upper-left corner of inter area
	SHORT	rx;			// Horizontal position of the lower-right corner of inter area
	SHORT	ry;			// Vertical position of the lower-right corner of inter area

}	stPACKED MDIN_INTER_WINDOW, *PMDIN_INTER_WINDOW;

typedef	enum {
	MDIN_INTER_BLOCK0 = 0,	// the ID of the 1st-block of inter-only area
	MDIN_INTER_BLOCK1,		// the ID of the 2nd-block of inter-only area
	MDIN_INTER_BLOCK2,		// the ID of the 3rd-block of inter-only area
	MDIN_INTER_BLOCK3,		// the ID of the 4th-block of inter-only area
	MDIN_INTER_BLOCK4		// the ID of the 5th-block of inter-only area

}	MDIN_INTER_BLOCK_t;

typedef enum {
	MDIN_IN_TEST_DISABLE = 0,		// disable input test pattern
	MDIN_IN_TEST_WINDOW,			// window test pattern
	MDIN_IN_TEST_H_RAMP,			// horizontal ramp pattern
	MDIN_IN_TEST_V_RAMP,			// vertical ramp pattern
	MDIN_IN_TEST_H_GRAY,			// horizontal gray scale pattern
	MDIN_IN_TEST_V_GRAY,			// vertical gray scale pattern
	MDIN_IN_TEST_H_COLOR,			// horizontal color bar pattern
	MDIN_IN_TEST_V_COLOR			// vertical color bar pattern

}	MDIN_IN_TEST_t;

typedef enum {
	MDIN_OUT_TEST_DISABLE = 0,		// disable output test pattern
	MDIN_OUT_TEST_WHITE,			// white test pattern
	MDIN_OUT_TEST_CROSS,			// cross test pattern
	MDIN_OUT_TEST_HATCH,			// cross hatch pattern
	MDIN_OUT_TEST_COLOR,			// color bar pattern
	MDIN_OUT_TEST_GRAY,				// gray scale pattern
	MDIN_OUT_TEST_WINDOW,			// white window pattern
	MDIN_OUT_TEST_H_RAMP,			// horizontal ramp pattern
	MDIN_OUT_TEST_WH_RAMP,			// wide horizontal ramp pattern
	MDIN_OUT_TEST_WV_RAMP,			// wide vertical ramp pattern
	MDIN_OUT_TEST_DIAGONAL,			// diagonal lines pattern
	MDIN_OUT_TEST_RED,				// red test pattern
	MDIN_OUT_TEST_GREEN,			// green test pattern
	MDIN_OUT_TEST_BLUE				// blue test pattern

}	MDIN_OUT_TEST_t;

typedef enum {
	MDIN_MCH_TEST_CH1 = 0,			// channel1 on the multi-channel test mode
	MDIN_MCH_TEST_CH2,				// channel2 on the multi-channel test mode
	MDIN_MCH_TEST_CH3,				// channel3 on the multi-channel test mode
	MDIN_MCH_TEST_CH4,				// channel4 on the multi-channel test mode

	MDIN_MCH_TEST_OFF				// disable on the multi-channel test mode

}	MDIN_MCH_TEST_ID_t;

//--------------------------------------------------------------------------------------------------------------------------
// for Video Input Clock Block
//--------------------------------------------------------------------------------------------------------------------------
typedef enum {
	MDIN_clk_ab_CLK_B = 0,			// use CLK_B input clock
	MDIN_clk_ab_CLK_A				// use CLK_A input clock

}	MDIN_CLK_SOURCE_t;

typedef	enum {
	MDIN_CLK_DELAY0 = 0,			// 0ns delayed
	MDIN_CLK_DELAY1,				// 1ns delayed
	MDIN_CLK_DELAY2,				// 2ns delayed
	MDIN_CLK_DELAY3,				// 3ns delayed
	MDIN_CLK_DELAY4,				// inversion & 0ns delayed
	MDIN_CLK_DELAY5,				// inversion & 1ns delayed
	MDIN_CLK_DELAY6,				// inversion & 2ns delayed
	MDIN_CLK_DELAY7,				// inversion 7 3ns delayed

}	MDIN_CLK_DELAY_t;

typedef enum {
	MDIN_a1_clka_D1P0 = 0,			// use clk_a/1, phase = 0
	MDIN_a1_clka_D2P0 = 1,			// use clk_a/2, phase = 0
	MDIN_a1_clka_D4P0 = 3,			// use clk_a/4, phase = 0

}	MDIN_CLK_PATH_A1_t;

typedef enum {
	MDIN_a2_clka_D1P4 = 0,			// use clk_a/1, phase = 180
	MDIN_a2_clka_D2P4 = 1,			// use clk_a/2, phase = 180
	MDIN_a2_clka_D4P2 = 3			// use clk_a/4, phase = 90

}	MDIN_CLK_PATH_A2_t;

typedef enum {
	MDIN_b1_clkb_D1P0 = 0,			// use clk_b/1, phase = 0
	MDIN_b1_clkb_D2P0 = 1,			// use clk_b/2, phase = 0
	MDIN_b1_clka_D2P2 = 2,			// use clk_a/2, phase = 90
	MDIN_b1_clka_D4P4 = 3			// use clk_a/4, phase = 180

}	MDIN_CLK_PATH_B1_t;

typedef enum {
	MDIN_b2_clkb_D1P4 = 0,			// use clk_b/1, phase = 180
	MDIN_b2_clkb_D2P4 = 1,			// use clk_b/2, phase = 180
	MDIN_b2_clka_D2P6 = 2,			// use clk_a/2, phase = 270
	MDIN_b2_clka_D4P6 = 3			// use clk_a/4, phase = 270

}	MDIN_CLK_PATH_B2_t;

typedef enum {
	MDIN_m1_clka_D1P0 = 0,			// use clk_a/1, phase = 0
	MDIN_m1_clka_D2P2 = 1,			// use clk_a/2, phase = 90
	MDIN_m1_clka_D4P2 = 2,			// use clk_a/4, phase = 90
	MDIN_m1_clka_D8P4 = 3			// use clk_a/8, phase = 180

}	MDIN_CLK_PATH_M1_t;

typedef enum {
	MDIN_m2_clkb_D1P0 = 0,			// use clk_b/1, phase = 0
	MDIN_m2_clkb_D2P4 = 1,			// use clk_b/2, phase = 90
	MDIN_m2_clka_D4P3 = 2,			// use clk_a/4, phase = 135
	MDIN_m2_clka_D8P0 = 3,			// use clk_a/8, phase = 0
	MDIN_m2_clkb_D4P3 = 4			// use clk_b/4, phase = 135

}	MDIN_CLK_PATH_M2_t;

//--------------------------------------------------------------------------------------------------------------------------
// for AUTO-SYNC Block
//--------------------------------------------------------------------------------------------------------------------------
typedef enum {
	AUTO_SYNC_IS_VALID = 0x00,		// auto-detection is valid
	AUTO_SYNC_SRC_PTRN = 0x01,		// input sync is test pattern
	AUTO_SYNC_BAD_SCAN = 0x02,		// ScanType is not valid
	AUTO_SYNC_BAD_HFRQ = 0x04,		// H-freq is not valid
	AUTO_SYNC_BAD_HTOT = 0x08,		// H-total is not valid
	AUTO_SYNC_BAD_VFRQ = 0x10,		// V-freq is not valid
	AUTO_SYNC_BAD_VTOT = 0x20,		// V-total is not valid
	AUTO_SYNC_BAD_HACT = 0x40,		// H-active is not valid
	AUTO_SYNC_BAD_VACT = 0x80		// V-active is not valid

}	MDIN_AUTOSYNC_ERROR_t;

typedef struct {
	BYTE	port;		// the selection of input port

	BYTE	err;		// auto sync error (MDIN_AUTOSYNC_ERROR_t)
	WORD	sync;		// the status of change and lost of input video sync, scan type
	WORD	Hfrq;		// the horizontal frequency of input video sync
	WORD	Htot;		// the total pixels of input video sync
	WORD	Vfrq;		// the vertical frequency of input video sync
	WORD	Vtot;		// the total lines of input video sync
	WORD	Hact;		// the active pixels of input video data
	WORD	Vact;		// the active lines of input video data

}	stPACKED MDIN_AUTOSYNC_INFO, *PMDIN_AUTOSYNC_INFO;

// define for input port of auto sync
#define		MDIN_AUTOSYNC_INB			0x01	// input port B is selected for auto sync
#define		MDIN_AUTOSYNC_INA			0x00	// input port A is selected for auto sync

// define for sync status of source video
#define		MDIN_AUTOSYNC_LOST			0x0001	// sync lost
#define		MDIN_AUTOSYNC_CHG			0x0002	// sync change
#define		MDIN_AUTOSYNC_INTR			0x0004	// interlaced scan

//--------------------------------------------------------------------------------------------------------------------------
// for Deinterlacer & Video Encoder Block
//--------------------------------------------------------------------------------------------------------------------------
typedef	struct
{
	WORD	coef[128];		// coefficients of deinterlacer

}	stPACKED MDIN_DEINTER_COEF, *PMDIN_DEINTER_COEF;

typedef	struct
{
	WORD	coef[19];		// coefficients of video encoder

}	stPACKED MDIN_ENCODER_COEF, *PMDIN_ENCODER_COEF;

//--------------------------------------------------------------------------------------------------------------------------
// for HDMI-TX Block
//--------------------------------------------------------------------------------------------------------------------------
typedef enum {
	HTX_CABLE_PLUG_OUT = 0,			// cable plug-out
	HTX_CABLE_EDID_CHK,				// check edid
	HTX_CABLE_HDMI_OUT,				// cable hdmi-out
	HTX_CABLE_DVI_OUT				// cable dvi-out

}	HTX_CABLE_STATUS_t;

typedef enum {
	EDID_BAD_HEADER = 8,			// header is not correct
	EDID_VER_NOT861B,				// version is not 861B
	EDID_1ST_CRC_ERR,				// 1st block CRC error
	EDID_2ND_CRC_ERR,				// 2nd block CRC error
	EDID_EXT_NOT861B,				// not 861B extension
	EDID_VER_NOTHDMI,				// no hdmi signature
	EDID_MAP_NOBLOCK				// block map error

}	HTX_EDID_ERROR_t;

typedef enum {
	HTX_DISPLAY_DVI = 0,			// display type is dvi
	HTX_DISPLAY_HDMI				// display type is hdmi

}	HTX_DISPLAY_TYPE_t;

typedef enum {
	HDCP_AUTHEN_BGN = 0,			// clear authen
	HDCP_AUTHEN_REQ,				// request authen
	HDCP_REAUTH_REQ,				// request re-authen
	HDCP_REPEAT_REQ,				// request repeater-authen
	HDCP_SHACAL_REQ,				// requset sha handler
	HDCP_AUTHEN_END,				// authenticated

	HDCP_NOT_DEVICE,				// not hdcp device
	HDCP_BKSV_ERROR,				// BKSV error
	HDCP_FIFO_ERROR,				// SHA done error
	HDCP_R0s_NOSAME,				// R0s mis-match
	HDCP_VHx_NOSAME,				// Vi mis-match
	HDCP_CAS_EXCEED,				// max cascade exceed
	HDCP_DEV_EXCEED,				// device exceed
	HDCP_MAX_EXCEED					// max device exceed

}	HTX_HDCP_STATUS_t;

typedef struct {
	WORD	sAddr;		// DDC device address
	WORD	rAddr;		// DDC [offset|segment] address
	WORD	bytes;		// DDC count
	BYTE	cmd;		// DDC command
	BYTE	err;		// DDC bus error
	PBYTE	pBuff;		// DDC data

}	stPACKED MDIN_HDMIMDDC_INFO, *PMDIN_HDMIMDDC_INFO;

typedef	struct
{
	BYTE	id_1;			// video identification code - 1
	BYTE	id_2;			// video identification code - 2
	BYTE	isub;			// video identification sub code
	BYTE	repl;			// [aspect ratio | pixel replication]

}	stPACKED MDIN_HTXFRMT_MODE, *PMDIN_HTXFRMT_MODE;

typedef	struct
{
	WORD	p_freq;			// pixel frequency
	WORD	v_freq;			// refresh rate (x * 100)
	WORD	pixels;			// horizontal total pixel
	WORD	v_line;			// vertical total line

}	stPACKED MDIN_HTXFRMT_FREQ, *PMDIN_HTXFRMT_FREQ;

typedef	struct
{
	WORD	x;				// horizontal active video position
	WORD	y;				// vertical active video position
	WORD	w;				// horizontal active video size
	WORD	h;				// vertical active video size

}	stPACKED MDIN_HTXFRMT_WIND, *PMDIN_HTXFRMT_WIND;

typedef	struct
{
	WORD	i_adj;			// interface adjustment
	WORD	h_len;			// the width of the HSYNC pulse
	WORD	v_len;			// the width of the VSYNC pulse
	WORD	d_dly;			// the width of the area left active display
	WORD	d_top;			// the height of the area above active display
	WORD	h_syn;			// the delay from the change of H-bit
	WORD	v_syn;			// the delay from the change of V-bit
	WORD	o_fid;			// the offset for the odd field of interlace

}	stPACKED MDIN_HTXFRMT_B656, *PMDIN_HTXFRMT_B656;

typedef	struct
{
	MDIN_HTXFRMT_MODE	stMODE;
	MDIN_HTXFRMT_FREQ	stFREQ;
	MDIN_HTXFRMT_WIND	stWIND;
	MDIN_HTXFRMT_B656	stB656;

}	stPACKED MDIN_HTXVIDEO_FRMT, *PMDIN_HTXVIDEO_FRMT;

// ----------------------------------------------------------------------
// Exported Variables
// ----------------------------------------------------------------------
// mdin3xx.c
extern WORD mdinERR, mdinREV;

// mdinfrmt.c
extern ROMDATA MDIN_OUTVIDEO_SYNC defMDINOutSync[];
extern ROMDATA MDIN_DACSYNC_CTRL  defMDINDACData[];
extern ROMDATA MDIN_SRCVIDEO_ATTB defMDINSrcVideo[];
extern ROMDATA MDIN_OUTVIDEO_ATTB defMDINOutVideo[];
extern ROMDATA MDIN_HTXVIDEO_FRMT defMDINHTXVideo[];

// mdincoef.c
extern ROMDATA MDIN_CSCCTRL_INFO MDIN_CscRGBtoYUV_SD_FullRange;
extern ROMDATA MDIN_CSCCTRL_INFO MDIN_CscRGBtoYUV_SD_StdRange;
extern ROMDATA MDIN_CSCCTRL_INFO MDIN_CscRGBtoYUV_HD_FullRange;
extern ROMDATA MDIN_CSCCTRL_INFO MDIN_CscRGBtoYUV_HD_StdRange;
extern ROMDATA MDIN_CSCCTRL_INFO MDIN_CscYUVtoRGB_SD_FullRange;
extern ROMDATA MDIN_CSCCTRL_INFO MDIN_CscYUVtoRGB_SD_StdRange;
extern ROMDATA MDIN_CSCCTRL_INFO MDIN_CscYUVtoRGB_HD_FullRange;
extern ROMDATA MDIN_CSCCTRL_INFO MDIN_CscYUVtoRGB_HD_StdRange;
extern ROMDATA MDIN_CSCCTRL_INFO MDIN_CscBypass_FullRange;
extern ROMDATA MDIN_CSCCTRL_INFO MDIN_CscBypass_RemoveBlack_StdRange;
extern ROMDATA MDIN_CSCCTRL_INFO MDIN_CscBypass_StdRange;
extern ROMDATA MDIN_CSCCTRL_INFO MDIN_CscSDtoHD_StdRange;
extern ROMDATA MDIN_CSCCTRL_INFO MDIN_CscHDtoSD_StdRange;
extern ROMDATA WORD MDIN_CscCosHue[];
extern ROMDATA WORD MDIN_CscSinHue[];
extern ROMDATA MDIN_MFCFILT_COEF MDIN_MFCFilter_Spline_1_00;
extern ROMDATA MDIN_MFCFILT_COEF MDIN_MFCFilter_Spline_0_75;
extern ROMDATA MDIN_MFCFILT_COEF MDIN_MFCFilter_Spline_0_50;
extern ROMDATA MDIN_MFCFILT_COEF MDIN_MFCFilter_Bilinear;
extern ROMDATA MDIN_MFCFILT_COEF MDIN_MFCFilter_BSpline_Most_Blur;
extern ROMDATA MDIN_MFCFILT_COEF MDIN_MFCFilter_BSpline_More_Blur;
extern ROMDATA MDIN_FNRFILT_COEF MDIN_FrontNRFilter_Default[];
extern ROMDATA MDIN_PEAKING_COEF MDIN_PeakingFilter_Default[];
extern ROMDATA MDIN_COLOREN_COEF MDIN_ColorEnFilter_Default[];
extern ROMDATA MDIN_BNRFILT_COEF MDIN_BlockNRFilter_Default[];
extern ROMDATA MDIN_MOSQUIT_COEF MDIN_MosquitFilter_Default[];
extern ROMDATA MDIN_SKINTON_COEF MDIN_SkinTonFilter_Default[];
extern ROMDATA MDIN_AUXFILT_COEF MDIN_AuxDownFilter_Default[];
extern ROMDATA MDIN_DEINTER_COEF MDIN_Deinter_Default[];
extern ROMDATA MDIN_ENCODER_COEF MDIN_Encoder_Default[];

// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
// mdin3xx.c
MDIN_ERROR_t MDIN3xx_VideoProcess(PMDIN_VIDEO_INFO pINFO);
MDIN_ERROR_t MDIN3xx_SetScaleProcess(PMDIN_VIDEO_INFO pINFO);

MDIN_ERROR_t MDIN3xx_SetSrcCompHsync(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetSrcInputFieldID(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetSrcHighTopFieldID(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetSrcYCOffset(PMDIN_VIDEO_INFO pINFO, MDIN_YC_OFFSET_t val);
MDIN_ERROR_t MDIN3xx_SetSrcCbCrSwap(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetSrcHACTPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge);
MDIN_ERROR_t MDIN3xx_SetSrcVACTPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge);
MDIN_ERROR_t MDIN3xx_SetSrcOffset(PMDIN_VIDEO_INFO pINFO, WORD offH, WORD offV);
MDIN_ERROR_t MDIN3xx_SetSrcCSCCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_CSCCTRL_INFO pCSC);

MDIN_ERROR_t MDIN3xx_SetOutCbCrSwap(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetOutPbPrSwap(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetOutDEPinMode(PMDIN_VIDEO_INFO pINFO, MDIN_DEOUT_MODE_t mode);
MDIN_ERROR_t MDIN3xx_SetOutHSPinMode(PMDIN_VIDEO_INFO pINFO, MDIN_HSOUT_MODE_t mode);
MDIN_ERROR_t MDIN3xx_SetOutVSPinMode(PMDIN_VIDEO_INFO pINFO, MDIN_VSOUT_MODE_t mode);
MDIN_ERROR_t MDIN3xx_SetOutDEPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge);
MDIN_ERROR_t MDIN3xx_SetOutHSPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge);
MDIN_ERROR_t MDIN3xx_SetOutVSPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge);
MDIN_ERROR_t MDIN3xx_SetPictureBrightness(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDIN3xx_SetPictureContrast(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDIN3xx_SetPictureSaturation(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDIN3xx_SetPictureHue(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDIN3xx_SetPictureGainR(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDIN3xx_SetPictureGainG(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDIN3xx_SetPictureGainB(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDIN3xx_SetPictureOffsetR(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDIN3xx_SetPictureOffsetG(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDIN3xx_SetPictureOffsetB(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDIN3xx_SetOutCSCCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_CSCCTRL_INFO pCSC);
MDIN_ERROR_t MDIN3xx_SetOutSYNCCtrl(PMDIN_VIDEO_INFO pINFO, PMDIN_OUTVIDEO_SYNC pSYNC);
MDIN_ERROR_t MDIN3xx_SetOutDACCtrl(PMDIN_VIDEO_INFO pINFO, PMDIN_DACSYNC_CTRL pDAC);

MDIN_ERROR_t MDIN3xx_GetVideoWindowFRMB(PMDIN_VIDEO_INFO pINFO, PMDIN_VIDEO_WINDOW pFRMB);
MDIN_ERROR_t MDIN3xx_SetVideoWindowCROP(PMDIN_VIDEO_INFO pINFO, MDIN_VIDEO_WINDOW stCROP);
MDIN_ERROR_t MDIN3xx_SetVideoWindowZOOM(PMDIN_VIDEO_INFO pINFO, MDIN_VIDEO_WINDOW stZOOM);
MDIN_ERROR_t MDIN3xx_SetVideoWindowVIEW(PMDIN_VIDEO_INFO pINFO, MDIN_VIDEO_WINDOW stVIEW);

MDIN_ERROR_t MDIN3xx_SetMFCHYFilterCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_MFCFILT_COEF pCoef);
MDIN_ERROR_t MDIN3xx_SetMFCHCFilterCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_MFCFILT_COEF pCoef);
MDIN_ERROR_t MDIN3xx_SetMFCVYFilterCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_MFCFILT_COEF pCoef);
MDIN_ERROR_t MDIN3xx_SetMFCVCFilterCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_MFCFILT_COEF pCoef);

MDIN_ERROR_t MDIN3xx_SetDelayCLK_A(MDIN_CLK_DELAY_t delay);
MDIN_ERROR_t MDIN3xx_SetDelayCLK_B(MDIN_CLK_DELAY_t delay);
MDIN_ERROR_t MDIN3xx_SetDelayVCLK_OUT(MDIN_CLK_DELAY_t delay);
MDIN_ERROR_t MDIN3xx_SetDelayVCLK_OUT_B(MDIN_CLK_DELAY_t delay);
MDIN_ERROR_t MDIN3xx_SetClock_clk_a(MDIN_CLK_SOURCE_t src);
MDIN_ERROR_t MDIN3xx_SetClock_clk_b(MDIN_CLK_SOURCE_t src);
MDIN_ERROR_t MDIN3xx_SetClock_clk_a1(MDIN_CLK_PATH_A1_t src);
MDIN_ERROR_t MDIN3xx_SetClock_clk_a2(MDIN_CLK_PATH_A2_t src);
MDIN_ERROR_t MDIN3xx_SetClock_clk_b1(MDIN_CLK_PATH_B1_t src);
MDIN_ERROR_t MDIN3xx_SetClock_clk_b2(MDIN_CLK_PATH_B2_t src);
MDIN_ERROR_t MDIN3xx_SetClock_clk_m1(MDIN_CLK_PATH_M1_t src);
MDIN_ERROR_t MDIN3xx_SetClock_clk_m2(MDIN_CLK_PATH_M2_t src);

MDIN_ERROR_t MDIN3xx_SetFrontNRFilterCoef(PMDIN_FNRFILT_COEF pCoef);
MDIN_ERROR_t MDIN3xx_EnableFrontNRFilter(BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetPeakingFilterCoef(PMDIN_FNRFILT_COEF pCoef);
MDIN_ERROR_t MDIN3xx_SetPeakingFilterLevel(BYTE val);
MDIN_ERROR_t MDIN3xx_EnablePeakingFilter(BOOL OnOff);
MDIN_ERROR_t MDIN3xx_EnableLTI(BOOL OnOff);
MDIN_ERROR_t MDIN3xx_EnableCTI(BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetColorEnFilterCoef(PMDIN_COLOREN_COEF pCoef);
MDIN_ERROR_t MDIN3xx_EnableColorEnFilter(BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetBlockNRFilterCoef(PMDIN_BNRFILT_COEF pCoef);
MDIN_ERROR_t MDIN3xx_EnableBlockNRFilter(BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetMosquitFilterCoef(PMDIN_MOSQUIT_COEF pCoef);
MDIN_ERROR_t MDIN3xx_EnableMosquitFilter(BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetSkinTonFilterCoef(PMDIN_SKINTON_COEF pCoef);
MDIN_ERROR_t MDIN3xx_EnableSkinTonFilter(BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetBWExtensionPoint(PMDIN_BWPOINT_COEF pCoef);
MDIN_ERROR_t MDIN3xx_EnableBWExtension(BOOL OnOff);

MDIN_ERROR_t MDIN3xx_EnableVENCColorMode(BOOL OnOff);
MDIN_ERROR_t MDIN3xx_EnableVENCBlueScreen(BOOL OnOff);
MDIN_ERROR_t MDIN3xx_EnableVENCPeakingFilter(BOOL OnOff);

MDIN_ERROR_t MDIN3xx_SetMemoryConfig(void);
DWORD		 MDIN3xx_GetAddressFRMB(void);

MDIN_ERROR_t MDIN3xx_GetChipID(PWORD pID);
MDIN_ERROR_t MDIN3xx_GetDeviceID(PWORD pID);
MDIN_ERROR_t MDIN3xx_GetVersionID(PWORD pID);
MDIN_ERROR_t MDIN3xx_HardReset(void);
MDIN_ERROR_t MDIN3xx_SoftReset(void);
WORD		 MDIN3xx_GetSizeOfBank(void);
MDIN_ERROR_t MDIN3xx_EnableIRQ(MDIN_IRQ_FLAG_t irq, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_GetParseIRQ(void);
BOOL		 MDIN3xx_IsOccurIRQ(MDIN_IRQ_FLAG_t irq);
MDIN_ERROR_t MDIN3xx_SetPriorityHIF(MDIN_PRIORITY_t mode);
MDIN_ERROR_t MDIN3xx_SetVCLKPLLSource(MDIN_PLL_SOURCE_t src);
MDIN_ERROR_t MDIN3xx_EnableOutputPAD(MDIN_PAD_OUT_t id, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_EnableClockDrive(MDIN_CLK_DRV_t id, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_EnableMainDisplay(BOOL OnOff);
MDIN_ERROR_t MDIN3xx_EnableMainFreeze(BOOL OnOff);
MDIN_ERROR_t MDIN3xx_EnableAuxDisplay(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_EnableAuxFreeze(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_EnableAuxWithMainOSD(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetInDataMapMode(MDIN_IN_DATA_MAP_t mode);
MDIN_ERROR_t MDIN3xx_SetDIGOutMapMode(MDIN_DIG_OUT_MAP_t mode);
MDIN_ERROR_t MDIN3xx_SetHostDataMapMode(MDIN_HOST_DATA_MAP_t mode);
MDIN_ERROR_t MDIN3xx_SetFormat4CHID(PMDIN_VIDEO_INFO pINFO, MDIN_4CHID_FORMAT_t mode);
MDIN_ERROR_t MDIN3xx_SetOrder4CHID(PMDIN_VIDEO_INFO pINFO, MDIN_4CHID_ORDER_t mode);
MDIN_ERROR_t MDIN3xx_SetDisplay4CH(PMDIN_VIDEO_INFO pINFO, MDIN_4CHVIEW_MODE_t mode);
MDIN_ERROR_t MDIN3xx_EnableMirrorH(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_EnableMirrorV(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);

MDIN_ERROR_t MDIN3xx_SetSrcTestPattern(PMDIN_VIDEO_INFO pINFO, MDIN_IN_TEST_t mode);
MDIN_ERROR_t MDIN3xx_SetOutTestPattern(MDIN_OUT_TEST_t mode);

MDIN_ERROR_t MDIN3xx_GetSyncInfo(PMDIN_AUTOSYNC_INFO pAUTO, WORD hclk);

// mdinipc.c
MDIN_ERROR_t MDIN3xx_SetIPCBlock(void);
MDIN_ERROR_t MDIN3xx_SetDeintMode(PMDIN_VIDEO_INFO pINFO, MDIN_DEINT_MODE_t mode);
MDIN_ERROR_t MDIN3xx_SetDeintFilm(PMDIN_VIDEO_INFO pINFO, MDIN_DEINT_FILM_t mode);
MDIN_ERROR_t MDIN3xx_EnableDeint3DNR(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetDeint3DNRGain(PMDIN_VIDEO_INFO pINFO, BYTE gain);
MDIN_ERROR_t MDIN3xx_EnableDeintCCS(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_EnableDeintCUE(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_SetDeintInterWND(PMDIN_INTER_WINDOW pAREA, MDIN_INTER_BLOCK_t nID);
MDIN_ERROR_t MDIN3xx_EnableDeintInterWND(MDIN_INTER_BLOCK_t nID, BOOL OnOff);
MDIN_ERROR_t MDIN3xx_DisplayDeintInterWND(BOOL OnOff);

// mdinaux.c
MDIN_ERROR_t MDINAUX_SetVideoPLL(WORD S, WORD F, WORD T);
MDIN_ERROR_t MDINAUX_VideoProcess(PMDIN_VIDEO_INFO pINFO);
MDIN_ERROR_t MDINAUX_SetScaleProcess(PMDIN_VIDEO_INFO pINFO);

MDIN_ERROR_t MDINAUX_SetSrcHighTopFieldID(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDINAUX_SetSrcCbCrSwap(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDINAUX_SetSrcHSPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge);
MDIN_ERROR_t MDINAUX_SetSrcVSPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge);
MDIN_ERROR_t MDINAUX_SetSrcOffset(PMDIN_VIDEO_INFO pINFO, WORD offH, WORD offV);
MDIN_ERROR_t MDINAUX_SetSrcCSCCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_CSCCTRL_INFO pCSC);

MDIN_ERROR_t MDINAUX_SetOutCbCrSwap(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDINAUX_SetOutHSPinMode(PMDIN_VIDEO_INFO pINFO, MDIN_HSOUT_MODE_t mode);
MDIN_ERROR_t MDINAUX_SetOutHSPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge);
MDIN_ERROR_t MDINAUX_SetOutVSPolarity(PMDIN_VIDEO_INFO pINFO, BOOL edge);
MDIN_ERROR_t MDINAUX_SetPictureBrightness(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDINAUX_SetPictureContrast(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDINAUX_SetPictureSaturation(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDINAUX_SetPictureHue(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDINAUX_SetPictureGainR(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDINAUX_SetPictureGainG(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDINAUX_SetPictureGainB(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDINAUX_SetPictureOffsetR(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDINAUX_SetPictureOffsetG(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDINAUX_SetPictureOffsetB(PMDIN_VIDEO_INFO pINFO, BYTE value);
MDIN_ERROR_t MDINAUX_SetOutCSCCoef(PMDIN_VIDEO_INFO pINFO, PMDIN_CSCCTRL_INFO pCSC);
MDIN_ERROR_t MDINAUX_SetOutSYNCCtrl(PMDIN_VIDEO_INFO pINFO, PMDIN_OUTVIDEO_SYNC pSYNC);

MDIN_ERROR_t MDINAUX_GetVideoWindowFRMB(PMDIN_VIDEO_INFO pINFO, PMDIN_VIDEO_WINDOW pFRMB);
MDIN_ERROR_t MDINAUX_SetVideoWindowCROP(PMDIN_VIDEO_INFO pINFO, MDIN_VIDEO_WINDOW stCROP);
MDIN_ERROR_t MDINAUX_SetVideoWindowZOOM(PMDIN_VIDEO_INFO pINFO, MDIN_VIDEO_WINDOW stZOOM);
MDIN_ERROR_t MDINAUX_SetVideoWindowVIEW(PMDIN_VIDEO_INFO pINFO, MDIN_VIDEO_WINDOW stVIEW);

MDIN_ERROR_t MDINAUX_SetFrontNRFilterCoef(PMDIN_AUXFILT_COEF pCoef);
MDIN_ERROR_t MDINAUX_EnableFrontNRFilter(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDINAUX_EnableMirrorH(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);
MDIN_ERROR_t MDINAUX_EnableMirrorV(PMDIN_VIDEO_INFO pINFO, BOOL OnOff);

// mdinhtx.c
MDIN_ERROR_t MDINHTX_CtrlHandler(PMDIN_VIDEO_INFO pINFO);
MDIN_ERROR_t MDINHTX_VideoProcess(PMDIN_VIDEO_INFO pINFO);
MDIN_ERROR_t MDINHTX_SetHDMIBlock(PMDIN_VIDEO_INFO pINFO);


#endif	/* __MDIN3xx_H__ */
