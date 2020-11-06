//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2008  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:  MDINCOEF.C
// Description 		:  This file contains typedefine for the driver files	
// Ref. Docment		: 
// Revision History 	:

// -----------------------------------------------------------------------------
// Include files
// -----------------------------------------------------------------------------
#include	"mdin3xx.h"

// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static Global Data section variables
// ----------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------
// RGB to YCbCr for input CSC
ROMDATA MDIN_CSCCTRL_INFO MDIN_CscRGBtoYUV_SD_FullRange		= {
	{(WORD) 258,		// coef0
	 (WORD)  50,		// coef1
	 (WORD) 132,		// coef2
	 (WORD)-149,		// coef3
	 (WORD) 225,		// coef4
	 (WORD) -76,		// coef5
	 (WORD)-188,		// coef6
	 (WORD) -36,		// coef7
	 (WORD) 225 },		// coef8
	{(WORD)   0,		// in_bias0
	 (WORD)   0,		// in_bias1
	 (WORD)   0 },		// in_bias2
	{(WORD)  64,		// out_bias0
	 (WORD) 512,		// out_bias1
	 (WORD) 512 },		// out_bias2
	 (WORD)0x1fc		// ctrl
};

ROMDATA MDIN_CSCCTRL_INFO MDIN_CscRGBtoYUV_SD_StdRange		= {
	{(WORD) 301,		// coef0    
	 (WORD)  58,		// coef1    
	 (WORD) 153,		// coef2    
	 (WORD)-174,		// coef3    
	 (WORD) 262,		// coef4    
	 (WORD) -88,		// coef5    
	 (WORD)-219,		// coef6    
	 (WORD) -42,		// coef7    
	 (WORD) 262 },		// coef8    
	{(WORD)   0,		// in_bias0 
	 (WORD)   0,		// in_bias1 
	 (WORD)   0 },		// in_bias2 
	{(WORD)   0,		// out_bias0
	 (WORD) 512,		// out_bias1
	 (WORD) 512 },		// out_bias2
	 (WORD)0x1fc		// ctrl
};

ROMDATA MDIN_CSCCTRL_INFO MDIN_CscRGBtoYUV_HD_FullRange		= {
	{(WORD) 314,		// coef0    
	 (WORD)  32,		// coef1    
	 (WORD)  94,		// coef2    
	 (WORD)-173,		// coef3    
	 (WORD) 225,		// coef4    
	 (WORD) -52,		// coef5    
	 (WORD)-204,		// coef6    
	 (WORD) -20,		// coef7    
	 (WORD) 225 },		// coef8    
	{(WORD)   0,		// in_bias0 
	 (WORD)   0,		// in_bias1 
	 (WORD)   0 },		// in_bias2 
	{(WORD)  64,		// out_bias0
	 (WORD) 512,		// out_bias1
	 (WORD) 512 },		// out_bias2
	 (WORD)0x1fc		// ctrl
};

ROMDATA MDIN_CSCCTRL_INFO MDIN_CscRGBtoYUV_HD_StdRange		= {
	{(WORD) 366,		// coef0    
	 (WORD)  37,		// coef1    
	 (WORD) 109,		// coef2    
	 (WORD)-202,		// coef3    
	 (WORD) 262,		// coef4    
	 (WORD) -60,		// coef5    
	 (WORD)-238,		// coef6    
	 (WORD) -24,		// coef7    
	 (WORD) 262 },		// coef8    
	{(WORD)   0,		// in_bias0 
	 (WORD)   0,		// in_bias1 
	 (WORD)   0 },		// in_bias2 
	{(WORD)   0,		// out_bias0
	 (WORD) 512,		// out_bias1
	 (WORD) 512 },		// out_bias2
	 (WORD)0x1fc		// ctrl
};

// YCbCr to RGB for output CSC
ROMDATA MDIN_CSCCTRL_INFO MDIN_CscYUVtoRGB_SD_FullRange		= {
	{(WORD) 596,		// coef0    
	 (WORD)-200,		// coef1    
	 (WORD)-416,		// coef2    
	 (WORD) 596,		// coef3    
	 (WORD)1033,		// coef4    
	 (WORD)   0,		// coef5    
	 (WORD) 596,		// coef6    
	 (WORD)   0,		// coef7    
	 (WORD) 817 },		// coef8    
	{(WORD) -64,		// in_bias0 
	 (WORD)-512,		// in_bias1 
	 (WORD)-512 },		// in_bias2 
	{(WORD)   0,		// out_bias0
	 (WORD)   0,		// out_bias1
	 (WORD)   0 },		// out_bias2
	 (WORD)0x1fc		// ctrl
};

ROMDATA MDIN_CSCCTRL_INFO MDIN_CscYUVtoRGB_SD_StdRange		= {
	{(WORD) 512,		// coef0    
	 (WORD)-172,		// coef1    
	 (WORD)-357,		// coef2    
	 (WORD) 512,		// coef3    
	 (WORD) 887,		// coef4    
	 (WORD)   0,		// coef5    
	 (WORD) 512,		// coef6    
	 (WORD)   0,		// coef7    
	 (WORD) 702 },		// coef8    
	{(WORD) -64,		// in_bias0 
	 (WORD)-512,		// in_bias1 
	 (WORD)-512 },		// in_bias2 
	{(WORD)   0,		// out_bias0
	 (WORD)   0,		// out_bias1
	 (WORD)   0 },		// out_bias2
	 (WORD)0x1fc		// ctrl
};

ROMDATA MDIN_CSCCTRL_INFO MDIN_CscYUVtoRGB_HD_FullRange		= {
	{(WORD) 596,		// coef0    
	 (WORD)-109,		// coef1    
	 (WORD)-273,		// coef2    
	 (WORD) 596,		// coef3    
	 (WORD)1083,		// coef4    
	 (WORD)   0,		// coef5    
	 (WORD) 596,		// coef6    
	 (WORD)   0,		// coef7    
	 (WORD) 918 },		// coef8    
	{(WORD) -64,		// in_bias0 
	 (WORD)-512,		// in_bias1 
	 (WORD)-512 },		// in_bias2 
	{(WORD)   0,		// out_bias0
	 (WORD)   0,		// out_bias1
	 (WORD)   0 },		// out_bias2
	 (WORD)0x1fc		// ctrl
};

ROMDATA MDIN_CSCCTRL_INFO MDIN_CscYUVtoRGB_HD_StdRange		= {
	{(WORD) 512,		// coef0    
	 (WORD) -96,		// coef1    
	 (WORD)-239,		// coef2    
	 (WORD) 512,		// coef3    
	 (WORD) 950,		// coef4    
	 (WORD)   0,		// coef5    
	 (WORD) 512,		// coef6    
	 (WORD)   0,		// coef7    
	 (WORD) 804 },		// coef8    
	{(WORD)   0,		// in_bias0 
	 (WORD)-512,		// in_bias1 
	 (WORD)-512 },		// in_bias2 
	{(WORD)   0,		// out_bias0
	 (WORD)   0,		// out_bias1
	 (WORD)   0 },		// out_bias2
	 (WORD)0x1fc		// ctrl
};
/*
ROMDATA MDIN_CSCCTRL_INFO MDIN_CscBypass_FullRange			= {
	{(WORD) 596,		// coef0    
	 (WORD)   0,		// coef1    
	 (WORD)   0,		// coef2    
	 (WORD)   0,		// coef3    
	 (WORD) 596,		// coef4    
	 (WORD)   0,		// coef5    
	 (WORD)   0,		// coef6    
	 (WORD)   0,		// coef7    
	 (WORD)  596 },		// coef8    
	{(WORD) -64,		// in_bias0 
	 (WORD)-512,		// in_bias1 
	 (WORD)-512 },		// in_bias2 
	{(WORD)   0,		// out_bias0
	 (WORD) 512,		// out_bias1
	 (WORD) 512 },		// out_bias2
	 (WORD)0x1fc		// ctrl
};

ROMDATA MDIN_CSCCTRL_INFO MDIN_CscBypass_RemoveBlack_StdRange		= {
	{(WORD) 512,		// coef0    
	 (WORD)   0,		// coef1    
	 (WORD)   0,		// coef2    
	 (WORD)   0,		// coef3    
	 (WORD) 512,		// coef4    
	 (WORD)   0,		// coef5    
	 (WORD)   0,		// coef6    
	 (WORD)   0,		// coef7    
	 (WORD) 512 },		// coef8    
	{(WORD) -64,		// in_bias0 
	 (WORD)-512,		// in_bias1 
	 (WORD)-512 },		// in_bias2 
	{(WORD)   0,		// out_bias0 (remove black level)
	 (WORD) 512,		// out_bias1
	 (WORD) 512 },		// out_bias2
	 (WORD)0x1fc		// ctrl
};
*/
ROMDATA MDIN_CSCCTRL_INFO MDIN_CscBypass_StdRange			= {
	{(WORD) 512,		// coef0    
	 (WORD)   0,		// coef1    
	 (WORD)   0,		// coef2    
	 (WORD)   0,		// coef3    
	 (WORD) 512,		// coef4    
	 (WORD)   0,		// coef5    
	 (WORD)   0,		// coef6    
	 (WORD)   0,		// coef7    
	 (WORD) 512 },		// coef8    
	{(WORD) -64,		// in_bias0 
	 (WORD)-512,		// in_bias1 
	 (WORD)-512 },		// in_bias2 
	{(WORD)  64,		// out_bias0
	 (WORD) 512,		// out_bias1
	 (WORD) 512 },		// out_bias2
	 (WORD)0x1fc		// ctrl
};

ROMDATA MDIN_CSCCTRL_INFO MDIN_CscSDtoHD_StdRange			= {
	{(WORD) 512,		// coef0    
	 (WORD)   0,		// coef1    
	 (WORD)   0,		// coef2    
	 (WORD)   0,		// coef3    
	 (WORD) 522,		// coef4    
	 (WORD)  59,		// coef5    
	 (WORD)   0,		// coef6    
	 (WORD)  38,		// coef7    
	 (WORD) 525 },		// coef8    
	{(WORD) -64,		// in_bias0 
	 (WORD)-512,		// in_bias1 
	 (WORD)-512 },		// in_bias2 
	{(WORD)  64,		// out_bias0
	 (WORD) 512,		// out_bias1
	 (WORD) 512 },		// out_bias2
	 (WORD)0x1fc		// ctrl
};

ROMDATA MDIN_CSCCTRL_INFO MDIN_CscHDtoSD_StdRange			= {
	{(WORD) 512,		// coef0    
	 (WORD)  51,		// coef1    
	 (WORD)  98,		// coef2    
	 (WORD)   0,		// coef3    
	 (WORD) 507,		// coef4    
	 (WORD) -56,		// coef5    4040
	 (WORD)   0,		// coef6    
	 (WORD) -37,		// coef7   4059 
	 (WORD) 504 },		// coef8    
	{(WORD) -64,		// in_bias0 
	 (WORD)-512,		// in_bias1 
	 (WORD)-512 },		// in_bias2 
	{(WORD)  64,		// out_bias0
	 (WORD) 512,		// out_bias1
	 (WORD) 512 },		// out_bias2
	 (WORD)0x1fc		// ctrl
};

//--------------------------------------------------------------------------------------------------------------------------
// hue range -60~60, cos(hue) table for picture HUE control
ROMDATA WORD MDIN_CscCosHue[]		= {
	0x0200, 0x0207, 0x020e, 0x0216, 0x021d, 0x0224, 0x022b, 0x0232,
	0x0239, 0x0240, 0x0247, 0x024e, 0x0254, 0x025b, 0x0262, 0x0269,
	0x026f, 0x0276, 0x027d, 0x0283, 0x028a, 0x0290, 0x0297, 0x029d,
	0x02a3, 0x02a9, 0x02b0, 0x02b6, 0x02bc, 0x02c2, 0x02c8, 0x02ce,
	0x02d4, 0x02da, 0x02e0, 0x02e6, 0x02eb, 0x02f1, 0x02f7, 0x02fc,
	0x0302, 0x0307, 0x030d, 0x0312, 0x0318, 0x031d, 0x0322, 0x0327,
	0x032c, 0x0331, 0x0336, 0x033b, 0x0340, 0x0345, 0x034a, 0x034f,
	0x0353, 0x0358, 0x035d, 0x0361, 0x0366, 0x036a, 0x036e, 0x0373,
	0x0377, 0x037b, 0x037f, 0x0383, 0x0387, 0x038b, 0x038f, 0x0393,
	0x0396, 0x039a, 0x039e, 0x03a1, 0x03a5, 0x03a8, 0x03ac, 0x03af,
	0x03b2, 0x03b5, 0x03b8, 0x03bb, 0x03be, 0x03c1, 0x03c4, 0x03c7,
	0x03ca, 0x03cc, 0x03cf, 0x03d1, 0x03d4, 0x03d6, 0x03d9, 0x03db,
	0x03dd, 0x03df, 0x03e1, 0x03e3, 0x03e5, 0x03e7, 0x03e9, 0x03eb,
	0x03ec, 0x03ee, 0x03ef, 0x03f1, 0x03f2, 0x03f4, 0x03f5, 0x03f6,
	0x03f7, 0x03f8, 0x03f9, 0x03fa, 0x03fb, 0x03fc, 0x03fd, 0x03fd,
	0x03fe, 0x03fe, 0x03ff, 0x03ff, 0x03ff, 0x0400, 0x0400, 0x0400,
	0x0400, 0x0400, 0x0400, 0x0400, 0x03ff, 0x03ff, 0x03ff, 0x03fe,
	0x03fe, 0x03fd, 0x03fd, 0x03fc, 0x03fb, 0x03fa, 0x03f9, 0x03f8,
	0x03f7, 0x03f6, 0x03f5, 0x03f4, 0x03f2, 0x03f1, 0x03ef, 0x03ee,
	0x03ec, 0x03eb, 0x03e9, 0x03e7, 0x03e5, 0x03e3, 0x03e1, 0x03df,
	0x03dd, 0x03db, 0x03d9, 0x03d6, 0x03d4, 0x03d1, 0x03cf, 0x03cc,
	0x03ca, 0x03c7, 0x03c4, 0x03c1, 0x03be, 0x03bb, 0x03b8, 0x03b5,
	0x03b2, 0x03af, 0x03ac, 0x03a8, 0x03a5, 0x03a1, 0x039e, 0x039a,
	0x0396, 0x0393, 0x038f, 0x038b, 0x0387, 0x0383, 0x037f, 0x037b,
	0x0377, 0x0373, 0x036e, 0x036a, 0x0366, 0x0361, 0x035d, 0x0358,
	0x0353, 0x034f, 0x034a, 0x0345, 0x0340, 0x033b, 0x0336, 0x0331,
	0x032c, 0x0327, 0x0322, 0x031d, 0x0318, 0x0312, 0x030d, 0x0307,
	0x0302, 0x02fc, 0x02f7, 0x02f1, 0x02eb, 0x02e6, 0x02e0, 0x02da,
	0x02d4, 0x02ce, 0x02c8, 0x02c2, 0x02bc, 0x02b6, 0x02b0, 0x02a9,
	0x02a3, 0x029d, 0x0297, 0x0290, 0x028a, 0x0283, 0x027d, 0x0276,
	0x026f, 0x0269, 0x0262, 0x025b, 0x0254, 0x024e, 0x0247, 0x0240,
	0x0239, 0x0232, 0x022b, 0x0224, 0x021d, 0x0216, 0x020e, 0x0207
};

// hue range -60~60, sin(hue) table for picture HUE control
ROMDATA WORD MDIN_CscSinHue[]		= {
	0xfc89, 0xfc8d, 0xfc92, 0xfc96, 0xfc9a, 0xfc9f, 0xfca3, 0xfca8,
	0xfcad, 0xfcb1, 0xfcb6, 0xfcbb, 0xfcc0, 0xfcc5, 0xfcca, 0xfccf,
	0xfcd4, 0xfcd9, 0xfcde, 0xfce3, 0xfce8, 0xfcee, 0xfcf3, 0xfcf9,
	0xfcfe, 0xfd04, 0xfd09, 0xfd0f, 0xfd15, 0xfd1a, 0xfd20, 0xfd26,
	0xfd2c, 0xfd32, 0xfd38, 0xfd3e, 0xfd44, 0xfd4a, 0xfd50, 0xfd57,
	0xfd5d, 0xfd63, 0xfd69, 0xfd70, 0xfd76, 0xfd7d, 0xfd83, 0xfd8a,
	0xfd91, 0xfd97, 0xfd9e, 0xfda5, 0xfdac, 0xfdb2, 0xfdb9, 0xfdc0,
	0xfdc7, 0xfdce, 0xfdd5, 0xfddc, 0xfde3, 0xfdea, 0xfdf2, 0xfdf9,
	0xfe00, 0xfe07, 0xfe0f, 0xfe16, 0xfe1d, 0xfe25, 0xfe2c, 0xfe34,
	0xfe3b, 0xfe43, 0xfe4a, 0xfe52, 0xfe59, 0xfe61, 0xfe69, 0xfe70,
	0xfe78, 0xfe80, 0xfe88, 0xfe8f, 0xfe97, 0xfe9f, 0xfea7, 0xfeaf,
	0xfeb7, 0xfebf, 0xfec7, 0xfecf, 0xfed7, 0xfedf, 0xfee7, 0xfeef,
	0xfef7, 0xfeff, 0xff07, 0xff0f, 0xff17, 0xff20, 0xff28, 0xff30,
	0xff38, 0xff40, 0xff49, 0xff51, 0xff59, 0xff61, 0xff6a, 0xff72,
	0xff7a, 0xff83, 0xff8b, 0xff93, 0xff9c, 0xffa4, 0xffac, 0xffb5,
	0xffbd, 0xffc5, 0xffce, 0xffd6, 0xffdf, 0xffe7, 0xffef, 0xfff8,
	0x0000, 0x0008, 0x0011, 0x0019, 0x0021, 0x002a, 0x0032, 0x003b,
	0x0043, 0x004b, 0x0054, 0x005c, 0x0064, 0x006d, 0x0075, 0x007d,
	0x0086, 0x008e, 0x0096, 0x009f, 0x00a7, 0x00af, 0x00b7, 0x00c0,
	0x00c8, 0x00d0, 0x00d8, 0x00e0, 0x00e9, 0x00f1, 0x00f9, 0x0101,
	0x0109, 0x0111, 0x0119, 0x0121, 0x0129, 0x0131, 0x0139, 0x0141,
	0x0149, 0x0151, 0x0159, 0x0161, 0x0169, 0x0171, 0x0178, 0x0180,
	0x0188, 0x0190, 0x0197, 0x019f, 0x01a7, 0x01ae, 0x01b6, 0x01bd,
	0x01c5, 0x01cc, 0x01d4, 0x01db, 0x01e3, 0x01ea, 0x01f1, 0x01f9,
	0x0200, 0x0207, 0x020e, 0x0216, 0x021d, 0x0224, 0x022b, 0x0232,
	0x0239, 0x0240, 0x0247, 0x024e, 0x0254, 0x025b, 0x0262, 0x0269,
	0x026f, 0x0276, 0x027d, 0x0283, 0x028a, 0x0290, 0x0297, 0x029d,
	0x02a3, 0x02a9, 0x02b0, 0x02b6, 0x02bc, 0x02c2, 0x02c8, 0x02ce,
	0x02d4, 0x02da, 0x02e0, 0x02e6, 0x02eb, 0x02f1, 0x02f7, 0x02fc,
	0x0302, 0x0307, 0x030d, 0x0312, 0x0318, 0x031d, 0x0322, 0x0327,
	0x032c, 0x0331, 0x0336, 0x033b, 0x0340, 0x0345, 0x034a, 0x034f,
	0x0353, 0x0358, 0x035d, 0x0361, 0x0366, 0x036a, 0x036e, 0x0373
};

//--------------------------------------------------------------------------------------------------------------------------
/*
// Spline, B=0.00, C=1.00, 4-tap, 64-phase for interpolation filter coefficients
ROMDATA MDIN_MFCFILT_COEF MDIN_MFCFilter_Spline_1_00		= {
// Spline, B=0.00, C=1.00, 4-tap, lower 32-phase
	{(WORD) 2048, (WORD)    0,		// phase 00
	 (WORD) 2044, (WORD)  -60,		// phase 02
	 (WORD) 2033, (WORD) -113,		// phase 04
	 (WORD) 2014, (WORD) -158,		// phase 06
	 (WORD) 1988, (WORD) -196,		// phase 08
	 (WORD) 1956, (WORD) -228,		// phase 10
	 (WORD) 1918, (WORD) -254,		// phase 12
	 (WORD) 1873, (WORD) -273,		// phase 14
	 (WORD) 1824, (WORD) -288,		// phase 16
	 (WORD) 1770, (WORD) -298,		// phase 18
	 (WORD) 1711, (WORD) -303,		// phase 20
	 (WORD) 1647, (WORD) -303,		// phase 22
	 (WORD) 1580, (WORD) -300,		// phase 24
	 (WORD) 1509, (WORD) -293,		// phase 26
	 (WORD) 1436, (WORD) -284,		// phase 28
	 (WORD) 1359, (WORD) -271,		// phase 30
	 (WORD) 1280, (WORD) -256,		// phase 32
	 (WORD) 1199, (WORD) -239,		// phase 34
	 (WORD) 1117, (WORD) -221,		// phase 36
	 (WORD) 1033, (WORD) -201,		// phase 38
	 (WORD)  948, (WORD) -180,		// phase 40
	 (WORD)  863, (WORD) -159,		// phase 42
	 (WORD)  778, (WORD) -138,		// phase 44
	 (WORD)  692, (WORD) -116,		// phase 46
	 (WORD)  608, (WORD)  -96,		// phase 48
	 (WORD)  525, (WORD)  -77,		// phase 50
	 (WORD)  443, (WORD)  -59,		// phase 52
	 (WORD)  362, (WORD)  -42,		// phase 54
	 (WORD)  284, (WORD)  -28,		// phase 56
	 (WORD)  208, (WORD)  -16,		// phase 58
	 (WORD)  136, (WORD)   -8,		// phase 60
	 (WORD)   66, (WORD)   -2 },	// phase 62

// Lanczos, B=0.10, C=1.00, 4-tap, upper 32-phase
	{(WORD) 1136, (WORD)  456,		// phase 00
	 (WORD) 1136, (WORD)  427,		// phase 02
	 (WORD) 1133, (WORD)  399,		// phase 04
	 (WORD) 1129, (WORD)  372,		// phase 06
	 (WORD) 1123, (WORD)  346,		// phase 08
	 (WORD) 1115, (WORD)  320,		// phase 10
	 (WORD) 1106, (WORD)  295,		// phase 12
	 (WORD) 1095, (WORD)  271,		// phase 14
	 (WORD) 1082, (WORD)  248,		// phase 16
	 (WORD) 1068, (WORD)  225,		// phase 18
	 (WORD) 1052, (WORD)  204,		// phase 20
	 (WORD) 1034, (WORD)  184,		// phase 22
	 (WORD) 1015, (WORD)  165,		// phase 24
	 (WORD)  994, (WORD)  147,		// phase 26
	 (WORD)  972, (WORD)  130,		// phase 28
	 (WORD)  949, (WORD)  114,		// phase 30
	 (WORD)  925, (WORD)   99,		// phase 32
	 (WORD)  899, (WORD)   86,		// phase 34
	 (WORD)  872, (WORD)   74,		// phase 36
	 (WORD)  845, (WORD)   62,		// phase 38
	 (WORD)  816, (WORD)   52,		// phase 40
	 (WORD)  788, (WORD)   42,		// phase 42
	 (WORD)  758, (WORD)   34,		// phase 44
	 (WORD)  728, (WORD)   27,		// phase 46
	 (WORD)  698, (WORD)   20,		// phase 48
	 (WORD)  667, (WORD)   15,		// phase 50
	 (WORD)  636, (WORD)   11,		// phase 52
	 (WORD)  606, (WORD)    7,		// phase 54
	 (WORD)  575, (WORD)    4,		// phase 56
	 (WORD)  545, (WORD)    2,		// phase 58
	 (WORD)  515, (WORD)    1,		// phase 60
	 (WORD)  485, (WORD)    0 }		// phase 62
};

// Spline, B=0.00, C=0.75, 4-tap, 64-phase for interpolation filter coefficients
ROMDATA MDIN_MFCFILT_COEF MDIN_MFCFilter_Spline_0_75		= {
// Spline, B=0.00, C=0.75, 4-tap, lower 32-phase
	{(WORD) 2048, (WORD)    0,		// phase 00
	 (WORD) 2044, (WORD)  -45,		// phase 02
	 (WORD) 2031, (WORD)  -84,		// phase 04
	 (WORD) 2009, (WORD) -118,		// phase 06
	 (WORD) 1981, (WORD) -147,		// phase 08
	 (WORD) 1945, (WORD) -171,		// phase 10
	 (WORD) 1903, (WORD) -190,		// phase 12
	 (WORD) 1854, (WORD) -205,		// phase 14
	 (WORD) 1800, (WORD) -216,		// phase 16
	 (WORD) 1740, (WORD) -223,		// phase 18
	 (WORD) 1676, (WORD) -227,		// phase 20
	 (WORD) 1607, (WORD) -227,		// phase 22
	 (WORD) 1535, (WORD) -225,		// phase 24
	 (WORD) 1459, (WORD) -220,		// phase 26
	 (WORD) 1380, (WORD) -213,		// phase 28
	 (WORD) 1299, (WORD) -203,		// phase 30
	 (WORD) 1216, (WORD) -192,		// phase 32
	 (WORD) 1131, (WORD) -179,		// phase 34
	 (WORD) 1046, (WORD) -165,		// phase 36
	 (WORD)  959, (WORD) -150,		// phase 38
	 (WORD)  873, (WORD) -135,		// phase 40
	 (WORD)  787, (WORD) -119,		// phase 42
	 (WORD)  702, (WORD) -103,		// phase 44
	 (WORD)  618, (WORD)  -87,		// phase 46
	 (WORD)  536, (WORD)  -72,		// phase 48
	 (WORD)  456, (WORD)  -57,		// phase 50
	 (WORD)  379, (WORD)  -44,		// phase 52
	 (WORD)  305, (WORD)  -31,		// phase 54
	 (WORD)  235, (WORD)  -21,		// phase 56
	 (WORD)  169, (WORD)  -12,		// phase 58
	 (WORD)  107, (WORD)   -6,		// phase 60
	 (WORD)   51, (WORD)   -2 },	// phase 62

// Lanczos, B=0.10, C=1.00, 4-tap, upper 32-phase
	{(WORD) 1136, (WORD)  456,		// phase 00
	 (WORD) 1136, (WORD)  427,		// phase 02
	 (WORD) 1133, (WORD)  399,		// phase 04
	 (WORD) 1129, (WORD)  372,		// phase 06
	 (WORD) 1123, (WORD)  346,		// phase 08
	 (WORD) 1115, (WORD)  320,		// phase 10
	 (WORD) 1106, (WORD)  295,		// phase 12
	 (WORD) 1095, (WORD)  271,		// phase 14
	 (WORD) 1082, (WORD)  248,		// phase 16
	 (WORD) 1068, (WORD)  225,		// phase 18
	 (WORD) 1052, (WORD)  204,		// phase 20
	 (WORD) 1034, (WORD)  184,		// phase 22
	 (WORD) 1015, (WORD)  165,		// phase 24
	 (WORD)  994, (WORD)  147,		// phase 26
	 (WORD)  972, (WORD)  130,		// phase 28
	 (WORD)  949, (WORD)  114,		// phase 30
	 (WORD)  925, (WORD)   99,		// phase 32
	 (WORD)  899, (WORD)   86,		// phase 34
	 (WORD)  872, (WORD)   74,		// phase 36
	 (WORD)  845, (WORD)   62,		// phase 38
	 (WORD)  816, (WORD)   52,		// phase 40
	 (WORD)  788, (WORD)   42,		// phase 42
	 (WORD)  758, (WORD)   34,		// phase 44
	 (WORD)  728, (WORD)   27,		// phase 46
	 (WORD)  698, (WORD)   20,		// phase 48
	 (WORD)  667, (WORD)   15,		// phase 50
	 (WORD)  636, (WORD)   11,		// phase 52
	 (WORD)  606, (WORD)    7,		// phase 54
	 (WORD)  575, (WORD)    4,		// phase 56
	 (WORD)  545, (WORD)    2,		// phase 58
	 (WORD)  515, (WORD)    1,		// phase 60
	 (WORD)  485, (WORD)    0 }		// phase 62
};
*/
// Spline, B=0.00, C=0.50, 4-tap, 64-phase for interpolation filter coefficients
ROMDATA MDIN_MFCFILT_COEF MDIN_MFCFilter_Spline_0_50		= {
// Spline, B=0.00, C=0.50, 4-tap, lower 32-phase
	{(WORD) 2048, (WORD)    0,		// phase 00
	 (WORD) 2043, (WORD)  -30,		// phase 02
	 (WORD) 2029, (WORD)  -56,		// phase 04
	 (WORD) 2006, (WORD)  -79,		// phase 06
	 (WORD) 1974, (WORD)  -98,		// phase 08
	 (WORD) 1935, (WORD) -114,		// phase 10
	 (WORD) 1888, (WORD) -127,		// phase 12
	 (WORD) 1835, (WORD) -137,		// phase 14
	 (WORD) 1776, (WORD) -144,		// phase 16
	 (WORD) 1711, (WORD) -149,		// phase 18
	 (WORD) 1642, (WORD) -151,		// phase 20
	 (WORD) 1568, (WORD) -152,		// phase 22
	 (WORD) 1490, (WORD) -150,		// phase 24
	 (WORD) 1409, (WORD) -147,		// phase 26
	 (WORD) 1325, (WORD) -142,		// phase 28
	 (WORD) 1239, (WORD) -135,		// phase 30
	 (WORD) 1152, (WORD) -128,		// phase 32
	 (WORD) 1064, (WORD) -120,		// phase 34
	 (WORD)  975, (WORD) -110,		// phase 36
	 (WORD)  886, (WORD) -100,		// phase 38
	 (WORD)  798, (WORD)  -90,		// phase 40
	 (WORD)  711, (WORD)  -79,		// phase 42
	 (WORD)  626, (WORD)  -69,		// phase 44
	 (WORD)  544, (WORD)  -58,		// phase 46
	 (WORD)  464, (WORD)  -48,		// phase 48
	 (WORD)  388, (WORD)  -38,		// phase 50
	 (WORD)  316, (WORD)  -29,		// phase 52
	 (WORD)  248, (WORD)  -21,		// phase 54
	 (WORD)  186, (WORD)  -14,		// phase 56
	 (WORD)  129, (WORD)   -8,		// phase 58
	 (WORD)   79, (WORD)   -4,		// phase 60
	 (WORD)   36, (WORD)   -1 },	// phase 62

// Lanczos, B=0.10, C=1.00, 4-tap, upper 32-phase
	{(WORD) 1136, (WORD)  456,		// phase 00
	 (WORD) 1136, (WORD)  427,		// phase 02
	 (WORD) 1133, (WORD)  399,		// phase 04
	 (WORD) 1129, (WORD)  372,		// phase 06
	 (WORD) 1123, (WORD)  346,		// phase 08
	 (WORD) 1115, (WORD)  320,		// phase 10
	 (WORD) 1106, (WORD)  295,		// phase 12
	 (WORD) 1095, (WORD)  271,		// phase 14
	 (WORD) 1082, (WORD)  248,		// phase 16
	 (WORD) 1068, (WORD)  225,		// phase 18
	 (WORD) 1052, (WORD)  204,		// phase 20
	 (WORD) 1034, (WORD)  184,		// phase 22
	 (WORD) 1015, (WORD)  165,		// phase 24
	 (WORD)  994, (WORD)  147,		// phase 26
	 (WORD)  972, (WORD)  130,		// phase 28
	 (WORD)  949, (WORD)  114,		// phase 30
	 (WORD)  925, (WORD)   99,		// phase 32
	 (WORD)  899, (WORD)   86,		// phase 34
	 (WORD)  872, (WORD)   74,		// phase 36
	 (WORD)  845, (WORD)   62,		// phase 38
	 (WORD)  816, (WORD)   52,		// phase 40
	 (WORD)  788, (WORD)   42,		// phase 42
	 (WORD)  758, (WORD)   34,		// phase 44
	 (WORD)  728, (WORD)   27,		// phase 46
	 (WORD)  698, (WORD)   20,		// phase 48
	 (WORD)  667, (WORD)   15,		// phase 50
	 (WORD)  636, (WORD)   11,		// phase 52
	 (WORD)  606, (WORD)    7,		// phase 54
	 (WORD)  575, (WORD)    4,		// phase 56
	 (WORD)  545, (WORD)    2,		// phase 58
	 (WORD)  515, (WORD)    1,		// phase 60
	 (WORD)  485, (WORD)    0 }		// phase 62
};
/*
// Bi-linear (2-tap), 32-phase for interpolation filter coefficients
ROMDATA MDIN_MFCFILT_COEF MDIN_MFCFilter_Bilinear		= {
	{(WORD) 2048, (WORD)    0,		// phase 00
	 (WORD) 1984, (WORD)    0,		// phase 02
	 (WORD) 1920, (WORD)    0,		// phase 04
	 (WORD) 1856, (WORD)    0,		// phase 06
	 (WORD) 1792, (WORD)    0,		// phase 08
	 (WORD) 1728, (WORD)    0,		// phase 10
	 (WORD) 1664, (WORD)    0,		// phase 12
	 (WORD) 1600, (WORD)    0,		// phase 14
	 (WORD) 1536, (WORD)    0,		// phase 16
	 (WORD) 1472, (WORD)    0,		// phase 18
	 (WORD) 1408, (WORD)    0,		// phase 20
	 (WORD) 1344, (WORD)    0,		// phase 22
	 (WORD) 1280, (WORD)    0,		// phase 24
	 (WORD) 1216, (WORD)    0,		// phase 26
	 (WORD) 1152, (WORD)    0,		// phase 28
	 (WORD) 1088, (WORD)    0,		// phase 30
	 (WORD) 1024, (WORD)    0,		// phase 32
	 (WORD)  960, (WORD)    0,		// phase 34
	 (WORD)  896, (WORD)    0,		// phase 36
	 (WORD)  832, (WORD)    0,		// phase 38
	 (WORD)  768, (WORD)    0,		// phase 40
	 (WORD)  704, (WORD)    0,		// phase 42
	 (WORD)  640, (WORD)    0,		// phase 44
	 (WORD)  576, (WORD)    0,		// phase 46
	 (WORD)  512, (WORD)    0,		// phase 48
	 (WORD)  448, (WORD)    0,		// phase 50
	 (WORD)  384, (WORD)    0,		// phase 52
	 (WORD)  320, (WORD)    0,		// phase 54
	 (WORD)  256, (WORD)    0,		// phase 56
	 (WORD)  192, (WORD)    0,		// phase 58
	 (WORD)  128, (WORD)    0,		// phase 60
	 (WORD)   64, (WORD)    0 },	// phase 62

// Lanczos, B=0.10, C=1.00, 4-tap, upper 32-phase
	{(WORD) 1136, (WORD)  456,		// phase 00
	 (WORD) 1136, (WORD)  427,		// phase 02
	 (WORD) 1133, (WORD)  399,		// phase 04
	 (WORD) 1129, (WORD)  372,		// phase 06
	 (WORD) 1123, (WORD)  346,		// phase 08
	 (WORD) 1115, (WORD)  320,		// phase 10
	 (WORD) 1106, (WORD)  295,		// phase 12
	 (WORD) 1095, (WORD)  271,		// phase 14
	 (WORD) 1082, (WORD)  248,		// phase 16
	 (WORD) 1068, (WORD)  225,		// phase 18
	 (WORD) 1052, (WORD)  204,		// phase 20
	 (WORD) 1034, (WORD)  184,		// phase 22
	 (WORD) 1015, (WORD)  165,		// phase 24
	 (WORD)  994, (WORD)  147,		// phase 26
	 (WORD)  972, (WORD)  130,		// phase 28
	 (WORD)  949, (WORD)  114,		// phase 30
	 (WORD)  925, (WORD)   99,		// phase 32
	 (WORD)  899, (WORD)   86,		// phase 34
	 (WORD)  872, (WORD)   74,		// phase 36
	 (WORD)  845, (WORD)   62,		// phase 38
	 (WORD)  816, (WORD)   52,		// phase 40
	 (WORD)  788, (WORD)   42,		// phase 42
	 (WORD)  758, (WORD)   34,		// phase 44
	 (WORD)  728, (WORD)   27,		// phase 46
	 (WORD)  698, (WORD)   20,		// phase 48
	 (WORD)  667, (WORD)   15,		// phase 50
	 (WORD)  636, (WORD)   11,		// phase 52
	 (WORD)  606, (WORD)    7,		// phase 54
	 (WORD)  575, (WORD)    4,		// phase 56
	 (WORD)  545, (WORD)    2,		// phase 58
	 (WORD)  515, (WORD)    1,		// phase 60
	 (WORD)  485, (WORD)    0 }		// phase 62
};

// Most-Blur (2-tap), 32-phase for interpolation filter coefficients
ROMDATA MDIN_MFCFILT_COEF MDIN_MFCFilter_BSpline_Most_Blur	= {
	{(WORD) 1365, (WORD)  341,		// phase 00
	 (WORD) 1364, (WORD)  310,		// phase 02
	 (WORD) 1358, (WORD)  281,		// phase 04
	 (WORD) 1348, (WORD)  254,		// phase 06
	 (WORD) 1335, (WORD)  229,		// phase 08
	 (WORD) 1319, (WORD)  205,		// phase 10
	 (WORD) 1300, (WORD)  183,		// phase 12
	 (WORD) 1278, (WORD)  163,		// phase 14
	 (WORD) 1253, (WORD)  144,		// phase 16
	 (WORD) 1226, (WORD)  127,		// phase 18
	 (WORD) 1197, (WORD)  111,		// phase 20
	 (WORD) 1165, (WORD)   96,		// phase 22
	 (WORD) 1131, (WORD)   83,		// phase 24
	 (WORD) 1096, (WORD)   71,		// phase 26
	 (WORD) 1059, (WORD)   61,		// phase 28
	 (WORD) 1021, (WORD)   51,		// phase 30
	 (WORD)  981, (WORD)   43,		// phase 32
	 (WORD)  941, (WORD)   35,		// phase 34
	 (WORD)  900, (WORD)   29,		// phase 36
	 (WORD)  858, (WORD)   23,		// phase 38
	 (WORD)  815, (WORD)   18,		// phase 40
	 (WORD)  773, (WORD)   14,		// phase 42
	 (WORD)  730, (WORD)   10,		// phase 44
	 (WORD)  687, (WORD)    8,		// phase 46
	 (WORD)  645, (WORD)    5,		// phase 48
	 (WORD)  604, (WORD)    3,		// phase 50
	 (WORD)  563, (WORD)    2,		// phase 52
	 (WORD)  523, (WORD)    1,		// phase 54
	 (WORD)  483, (WORD)    1,		// phase 56
	 (WORD)  446, (WORD)    0,		// phase 58
	 (WORD)  409, (WORD)    0,		// phase 60
	 (WORD)  374, (WORD)    0 }, 	// phase 62

// Lanczos, B=0.10, C=1.00, 4-tap, upper 32-phase
	{(WORD) 1136, (WORD)  456,		// phase 00
	 (WORD) 1136, (WORD)  427,		// phase 02
	 (WORD) 1133, (WORD)  399,		// phase 04
	 (WORD) 1129, (WORD)  372,		// phase 06
	 (WORD) 1123, (WORD)  346,		// phase 08
	 (WORD) 1115, (WORD)  320,		// phase 10
	 (WORD) 1106, (WORD)  295,		// phase 12
	 (WORD) 1095, (WORD)  271,		// phase 14
	 (WORD) 1082, (WORD)  248,		// phase 16
	 (WORD) 1068, (WORD)  225,		// phase 18
	 (WORD) 1052, (WORD)  204,		// phase 20
	 (WORD) 1034, (WORD)  184,		// phase 22
	 (WORD) 1015, (WORD)  165,		// phase 24
	 (WORD)  994, (WORD)  147,		// phase 26
	 (WORD)  972, (WORD)  130,		// phase 28
	 (WORD)  949, (WORD)  114,		// phase 30
	 (WORD)  925, (WORD)   99,		// phase 32
	 (WORD)  899, (WORD)   86,		// phase 34
	 (WORD)  872, (WORD)   74,		// phase 36
	 (WORD)  845, (WORD)   62,		// phase 38
	 (WORD)  816, (WORD)   52,		// phase 40
	 (WORD)  788, (WORD)   42,		// phase 42
	 (WORD)  758, (WORD)   34,		// phase 44
	 (WORD)  728, (WORD)   27,		// phase 46
	 (WORD)  698, (WORD)   20,		// phase 48
	 (WORD)  667, (WORD)   15,		// phase 50
	 (WORD)  636, (WORD)   11,		// phase 52
	 (WORD)  606, (WORD)    7,		// phase 54
	 (WORD)  575, (WORD)    4,		// phase 56
	 (WORD)  545, (WORD)    2,		// phase 58
	 (WORD)  515, (WORD)    1,		// phase 60
	 (WORD)  485, (WORD)    0 }		// phase 62
};

// More-Blur (2-tap), 32-phase for interpolation filter coefficients
ROMDATA MDIN_MFCFILT_COEF MDIN_MFCFilter_BSpline_More_Blur	= {
	{(WORD) 1707, (WORD)  171,		// phase 00
	 (WORD) 1703, (WORD)  140,		// phase 02
	 (WORD) 1693, (WORD)  113,		// phase 04
	 (WORD) 1677, (WORD)   88,		// phase 06
	 (WORD) 1655, (WORD)   65,		// phase 08
	 (WORD) 1627, (WORD)   46,		// phase 10
	 (WORD) 1594, (WORD)   28,		// phase 12
	 (WORD) 1556, (WORD)   13,		// phase 14
	 (WORD) 1515, (WORD)    0,		// phase 16
	 (WORD) 1469, (WORD)  -11,		// phase 18
	 (WORD) 1419, (WORD)  -20,		// phase 20
	 (WORD) 1366, (WORD)  -27,		// phase 22
	 (WORD) 1311, (WORD)  -33,		// phase 24
	 (WORD) 1253, (WORD)  -38,		// phase 26
	 (WORD) 1192, (WORD)  -40,		// phase 28
	 (WORD) 1130, (WORD)  -42,		// phase 30
	 (WORD) 1067, (WORD)  -43,		// phase 32
	 (WORD) 1002, (WORD)  -42,		// phase 34
	 (WORD)  937, (WORD)  -41,		// phase 36
	 (WORD)  872, (WORD)  -39,		// phase 38
	 (WORD)  807, (WORD)  -36,		// phase 40
	 (WORD)  742, (WORD)  -33,		// phase 42
	 (WORD)  678, (WORD)  -29,		// phase 44
	 (WORD)  615, (WORD)  -25,		// phase 46
	 (WORD)  555, (WORD)  -21,		// phase 48
	 (WORD)  496, (WORD)  -17,		// phase 50
	 (WORD)  439, (WORD)  -13,		// phase 52
	 (WORD)  385, (WORD)  -10,		// phase 54
	 (WORD)  335, (WORD)   -7,		// phase 56
	 (WORD)  287, (WORD)   -4,		// phase 58
	 (WORD)  244, (WORD)   -2,		// phase 60
	 (WORD)  205, (WORD)    0 },	// phase 62

// Lanczos, B=0.10, C=1.00, 4-tap, upper 32-phase
	{(WORD) 1136, (WORD)  456,		// phase 00
	 (WORD) 1136, (WORD)  427,		// phase 02
	 (WORD) 1133, (WORD)  399,		// phase 04
	 (WORD) 1129, (WORD)  372,		// phase 06
	 (WORD) 1123, (WORD)  346,		// phase 08
	 (WORD) 1115, (WORD)  320,		// phase 10
	 (WORD) 1106, (WORD)  295,		// phase 12
	 (WORD) 1095, (WORD)  271,		// phase 14
	 (WORD) 1082, (WORD)  248,		// phase 16
	 (WORD) 1068, (WORD)  225,		// phase 18
	 (WORD) 1052, (WORD)  204,		// phase 20
	 (WORD) 1034, (WORD)  184,		// phase 22
	 (WORD) 1015, (WORD)  165,		// phase 24
	 (WORD)  994, (WORD)  147,		// phase 26
	 (WORD)  972, (WORD)  130,		// phase 28
	 (WORD)  949, (WORD)  114,		// phase 30
	 (WORD)  925, (WORD)   99,		// phase 32
	 (WORD)  899, (WORD)   86,		// phase 34
	 (WORD)  872, (WORD)   74,		// phase 36
	 (WORD)  845, (WORD)   62,		// phase 38
	 (WORD)  816, (WORD)   52,		// phase 40
	 (WORD)  788, (WORD)   42,		// phase 42
	 (WORD)  758, (WORD)   34,		// phase 44
	 (WORD)  728, (WORD)   27,		// phase 46
	 (WORD)  698, (WORD)   20,		// phase 48
	 (WORD)  667, (WORD)   15,		// phase 50
	 (WORD)  636, (WORD)   11,		// phase 52
	 (WORD)  606, (WORD)    7,		// phase 54
	 (WORD)  575, (WORD)    4,		// phase 56
	 (WORD)  545, (WORD)    2,		// phase 58
	 (WORD)  515, (WORD)    1,		// phase 60
	 (WORD)  485, (WORD)    0 }		// phase 62
};
*/
//--------------------------------------------------------------------------------------------------------------------------
// Coefficients for filter
ROMDATA MDIN_FNRFILT_COEF MDIN_FrontNRFilter_Default[]	= {
	// 1920 to 1440 ~ 1920 ==>  bypass
  {	{0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},	// Y
	{0x0400, 0x0000, 0x0000, 0x0000}  },								// C

	// 1920 to 1024 ~ 1440 ==> cut0.563
  {	{0x0240, 0x0140, 0x0FC2, 0x0FA7, 0x002D, 0x0010, 0x0FFA, 0x0000},	// Y
	{0x0222, 0x00F9, 0x0FF6, 0x0000}  },								// C

	// 1920 to 720 ~ 1024  ==> cut0.375
  {	{0x017E, 0x0125, 0x006A, 0x0FDE, 0x0FC7, 0x0FF2, 0x000F, 0x000C},	// Y
	{0x0156, 0x0116, 0x003F, 0x0000}  },								// C

	// 1920 to 640 ~ 720   ==> cut0.333
  {	{0x0158, 0x011A, 0x008B, 0x0FFE, 0x0FBD, 0x0FCF, 0x0003, 0x0022},	// Y
	{0x0156, 0x0116, 0x003F, 0x0000}  },								// C

	// 1920 to 480 ~ 640   ==> cut0.250
  {	{0x0100, 0x00E1, 0x0094, 0x003F, 0x0000, 0x0FE9, 0x0FEC, 0x0FF7},	// Y
	{0x0100, 0x00D7, 0x007E, 0x002B}  }									// C
};

ROMDATA MDIN_PEAKING_COEF MDIN_PeakingFilter_Default[]	= {
	{{0x0200, 0x0000, 0x0700, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}}
};

ROMDATA MDIN_COLOREN_COEF MDIN_ColorEnFilter_Default[]	= {
	{{0x0100, 0x0080, 0x0000, 0x0000}}
};

ROMDATA MDIN_BNRFILT_COEF MDIN_BlockNRFilter_Default[]	= {
	{{0x0200, 0x0100, 0x0000, 0x0000}}
};

ROMDATA MDIN_MOSQUIT_COEF MDIN_MosquitFilter_Default[]	= {
	{{0x0004, 0x3080, 0x0700, 0x0100, 0x0010, 0x00c4, 0x0102, 0x0000, 0x0c62}}
};

ROMDATA MDIN_SKINTON_COEF MDIN_SkinTonFilter_Default[]	= {
	{{0x435a, 0x0dd3, 0x0200, 0xc3e0, 0x0230}}
};

ROMDATA MDIN_AUXFILT_COEF MDIN_AuxDownFilter_Default[]	= {
	{{0x0100, 0x0000, 0x0000, 0x0000, 0x0000},	// HY
	{0x0100, 0x0000, 0x0000, 0x0000},			// HC
	{0x0100, 0x0000, 0x0000},					// VY
	{0x0100, 0x0000}}							// VC    
};

//--------------------------------------------------------------------------------------------------------------------------
// default value for Deinterlacer
ROMDATA MDIN_DEINTER_COEF MDIN_Deinter_Default[]		= {
//     0       1       2       3       4       5       6       7       8       9       A       B       C       D       E       F
   {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xb1d9, 0x1010, 0x0000, 0x5f47, 0x0068, 0x0b23, 0x5907, 0x1017, 0x3f08, 0x0800, 0x0810, // 200
	0x1f1c, 0x1212, 0x0c14, 0x0014, 0x0308, 0x0608, 0xb000, 0x0000, 0x44c4, 0x2710, 0x2710, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 210
	0x007f, 0x08c8, 0x3804, 0x00ff, 0x00ff, 0xffff, 0x1008, 0x000a, 0x0001, 0x000f, 0x0a14, 0x000a, 0x2600, 0x0000, 0x6650, 0x5232, // 220
	0x122a, 0xab32, 0x9957, 0x0000, 0x0000,	0x0000, 0x063f, 0x968a, 0x1011, 0xffff, 0xffff, 0x0304, 0xffff, 0x0000, 0x0000, 0x0000, // 230
	0x6808, 0x8440, 0x8040, 0x0008, 0x0000,	0x0000, 0xc038, 0x0000, 0x730a, 0x150a, 0x0014, 0x0288, 0x0000, 0x0000, 0x0000, 0x0000, // 240
	0xc208, 0x00b4, 0x002f, 0x60d0, 0x0313,	0xa206, 0x000c, 0x0602, 0x1e14, 0x0104, 0x0128, 0x0108, 0xffff, 0xb044, 0x0000, 0x0000, // 250
	0x1f1a, 0x00a1, 0xf044, 0x0000, 0x1412,	0x1244, 0x0bf8, 0x3f10, 0x0062, 0x4040, 0x105c, 0x0000, 0x5180, 0x0001, 0x0000, 0x000e, // 260
	0x040a, 0x0108, 0x7f10, 0x1482, 0x050f,	0x6c6c, 0x0000, 0x0000, 0x0000, 0x00a0, 0xb000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}	// 270
};

#if defined(SYSTEM_USE_MDIN325)||defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380)
//--------------------------------------------------------------------------------------------------------------------------
// default value for Video Encoder
ROMDATA MDIN_ENCODER_COEF MDIN_Encoder_Default[]		= {
//     0       1       2       3       4       5       6       7       8       9       A       B       C       D       E       F
  {	0x2000, 0x16b9, 0x84c7, 0x888e, 0x0200, 0x8070, 0xa8f0, 0x1200, 0x1800, 0x7c1f, 0x21f0, 0x1000, 0xa021, 0x0800, 0x0000, 0x0000,	// 1E0
	0x0000, 0x0000, 0x0000 },		// NTSC system																					// 1F0

  {	0xA000, 0x16bb, 0x84c7, 0xd089, 0x0200, 0x8075, 0x00fc, 0x1200, 0x1801, 0x8acb, 0x2a09, 0x3000, 0xa023, 0x0a00, 0x0600, 0x0000,	// 1E0
	0x0400, 0x0000, 0x0000 }		// PAL system																					// 1F0
};
#endif	/* defined(SYSTEM_USE_MDIN325)||defined(SYSTEM_USE_MDIN325A)||defined(SYSTEM_USE_MDIN380) */

// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------

/*  FILE_END_HERE */
