//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2008  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:  MDINFRMT.C
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
// OUT sync control
ROMDATA MDIN_OUTVIDEO_SYNC defMDINOutSync[]		= {
	{	// 720x480i60
		  20,		// hdpulse_pos
		   8,		// vdpulse_pos
		 270,		// vdpulse_pos_bot
		 858,		// htotal_size
		  87,		// hactive_start
		 807,		// hactive_end
		  63,		// hsync_start
		   1,		// hsync_end
		 525,		// vtotal_size
		  21,		// vactive_start
		 261,		// vactive_end
		   6,		// vsync_start
		   3,		// vsync_end
		 284,		// vactive_start_bot
		 524,		// vactive_end_bot
		 268,		// vsync_start_bot
		 265,		// vsync_end_bot
		 430,		// vsync_bot_fld_pos
	
		   4,		// pre_div_vclk (P)
		  16,		// post_div_vclk (M)
		   8,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		  27,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   1		// axclk_gen_div_t (T)
	#else
		  29,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   1		// axclk_gen_div_t (T)
	#endif
	},
	{	// 720x576i50
		  20,		// hdpulse_pos
		   8,		// vdpulse_pos
		 320,		// vdpulse_pos_bot
		 864,		// htotal_size
		 100,		// hactive_start
		 820,		// hactive_end
		  64,		// hsync_start
		   1,		// hsync_end
		 625,		// vtotal_size
		  23,		// vactive_start
		 311,		// vactive_end
		   4,		// vsync_start
		   1,		// vsync_end
		 336,		// vactive_start_bot
		 624,		// vactive_end_bot
		 316,		// vsync_start_bot
		 313,		// vsync_end_bot
		 433,		// vsync_bot_fld_pos
	
		   4,		// pre_div_vclk (P)
		  16,		// post_div_vclk (M)
		   8,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		  27,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   1		// axclk_gen_div_t (T)
	#else
		  29,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   1		// axclk_gen_div_t (T)
	#endif
	},
	{	// 720x480p60
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		 858,		// htotal_size
		  90,		// hactive_start
		 810,		// hactive_end
		  63,		// hsync_start
		   1,		// hsync_end
		 525,		// vtotal_size
		  43,		// vactive_start
		 523,		// vactive_end
		  13,		// vsync_start
		   7,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		   4,		// pre_div_vclk (P)
		  16,		// post_div_vclk (M)
		   4,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		  13,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   1		// axclk_gen_div_t (T)
	#else
		  14,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   1		// axclk_gen_div_t (T)
	#endif
	},
	{	// 720x576p50
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		 864,		// htotal_size
		 100,		// hactive_start
		 820,		// hactive_end
		  65,		// hsync_start
		   1,		// hsync_end
		 625,		// vtotal_size
		  45,		// vactive_start
		 621,		// vactive_end
		   6,		// vsync_start
		   1,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		   4,		// pre_div_vclk (P)
		  16,		// post_div_vclk (M)
		   4,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		  13,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   1		// axclk_gen_div_t (T)
	#else
		  14,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   1		// axclk_gen_div_t (T)
	#endif
	},
	{	// 1280x720p60
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		1650,		// htotal_size
		 228,		// hactive_start
		1508,		// hactive_end
		  41,		// hsync_start
		   1,		// hsync_end
		 750,		// vtotal_size
		  26,		// vactive_start
		 746,		// vactive_end
		   6,		// vsync_start
		   1,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		   4,		// pre_div_vclk (P)
		  22,		// post_div_vclk (M)
		   2,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   5,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#else
		   5,		// axclk_gen_div_s (S)
		   5,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#endif
	},
	{	// 1280x720p50
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		1980,		// htotal_size
		 228,		// hactive_start
		1508,		// hactive_end
		  41,		// hsync_start
		   1,		// hsync_end
		 750,		// vtotal_size
		  26,		// vactive_start
		 746,		// vactive_end
		   6,		// vsync_start
		   1,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		   4,		// pre_div_vclk (P)
		  22,		// post_div_vclk (M)
		   2,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   5,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#else
		   5,		// axclk_gen_div_s (S)
		   5,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#endif
	},
	{	// 1920x1080i60
		  20,		// hdpulse_pos
		   8,		// vdpulse_pos
		 570,		// vdpulse_pos_bot
		2200,		// htotal_size
		 160,		// hactive_start
		2080,		// hactive_end
		  45,		// hsync_start
		   1,		// hsync_end
		1125,		// vtotal_size
		  21,		// vactive_start
		 561,		// vactive_end
		   6,		// vsync_start
		   1,		// vsync_end
		 584,		// vactive_start_bot
		1124,		// vactive_end_bot
		 568,		// vsync_start_bot
		 563,		// vsync_end_bot
		1101,		// vsync_bot_fld_pos
	
		   4,		// pre_div_vclk (P)
		  22,		// post_div_vclk (M)
		   2,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   5,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#else
		   5,		// axclk_gen_div_s (S)
		   5,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#endif
	},
	{	// 1920x1080i50
		  20,		// hdpulse_pos
		   8,		// vdpulse_pos
		 570,		// vdpulse_pos_bot
		2640,		// htotal_size
		 160,		// hactive_start
		2080,		// hactive_end
		  45,		// hsync_start
		   1,		// hsync_end
		1125,		// vtotal_size
		  21,		// vactive_start
		 561,		// vactive_end
		   6,		// vsync_start
		   1,		// vsync_end
		 584,		// vactive_start_bot
		1124,		// vactive_end_bot
		 568,		// vsync_start_bot
		 563,		// vsync_end_bot
		1321,		// vsync_bot_fld_pos
	
		   4,		// pre_div_vclk (P)
		  22,		// post_div_vclk (M)
		   2,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   5,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#else
		   5,		// axclk_gen_div_s (S)
		   5,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#endif
	},
	{	// 1920x1080p60
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		2200,		// htotal_size
		 160,		// hactive_start
		2080,		// hactive_end
		  45,		// hsync_start
		   1,		// hsync_end
		1125,		// vtotal_size
		  42,		// vactive_start
		1122,		// vactive_end
		   6,		// vsync_start
		   1,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		   4,		// pre_div_vclk (P)
		  22,		// post_div_vclk (M)
		   1,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   2,		// axclk_gen_div_s (S)
		   6,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#else
		   2,		// axclk_gen_div_s (S)
		   8,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#endif
	},
	{	// 1920x1080p50
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		2640,		// htotal_size
		 160,		// hactive_start
		2080,		// hactive_end
		  45,		// hsync_start
		   1,		// hsync_end
		1125,		// vtotal_size
		  42,		// vactive_start
		1122,		// vactive_end
		   6,		// vsync_start
		   1,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		   4,		// pre_div_vclk (P)
		  22,		// post_div_vclk (M)
		   1,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   2,		// axclk_gen_div_s (S)
		   6,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#else
		   2,		// axclk_gen_div_s (S)
		   8,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#endif
	},
	{	// 640x480p60
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		 800,		// htotal_size
		 112,		// hactive_start
		 752,		// hactive_end
		  97,		// hsync_start
		   1,		// hsync_end
		 525,		// vtotal_size
		  36,		// vactive_start
		 516,		// vactive_end
		   3,		// vsync_start
		   1,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		  26,		// pre_div_vclk (P)
		  97,		// post_div_vclk (M)
		   4,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		  15,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		  97		// axclk_gen_div_t (T)
	#else
		  16,		// axclk_gen_div_s (S)
		   8,		// axclk_gen_div_f (F)
		  97		// axclk_gen_div_t (T)
	#endif
	},
	{	// 640x480p75
		  20,		// hdpulse_pos
		   6,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		 840,		// htotal_size
		 152,		// hactive_start
		 792,		// hactive_end
		  65,		// hsync_start
		   1,		// hsync_end
		 500,		// vtotal_size
		  19,		// vactive_start
		 499,		// vactive_end
		   3,		// vsync_start
		   0,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		  27,		// pre_div_vclk (P)
		 126,		// post_div_vclk (M)
		   4,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		  11,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   1		// axclk_gen_div_t (T)
	#else
		  12,		// axclk_gen_div_s (S)
		   6,		// axclk_gen_div_f (F)
		   7		// axclk_gen_div_t (T)
	#endif
	},
	{	// 800x600p60
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		1056,		// htotal_size
		 184,		// hactive_start
		 984,		// hactive_end
		 129,		// hsync_start
		   1,		// hsync_end
		 628,		// vtotal_size
		  27,		// vactive_start
		 627,		// vactive_end
		   4,		// vsync_start
		   0,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		  27,		// pre_div_vclk (P)
		 160,		// post_div_vclk (M)
		   4,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   9,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   2		// axclk_gen_div_t (T)
	#else
		  10,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   8		// axclk_gen_div_t (T)
	#endif
	},
	{	// 800x600p75
		  20,		// hdpulse_pos
		   5,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		1056,		// htotal_size
		 208,		// hactive_start
		1008,		// hactive_end
		  81,		// hsync_start
		   1,		// hsync_end
		 625,		// vtotal_size
		  24,		// vactive_start
		 624,		// vactive_end
		   3,		// vsync_start
		   0,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		  24,		// pre_div_vclk (P)
		 176,		// post_div_vclk (M)
		   4,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   7,		// axclk_gen_div_s (S)
		   7,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#else
		   8,		// axclk_gen_div_s (S)
		   2,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#endif
	},
	{	// 1024x768p60
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		1344,		// htotal_size
		 264,		// hactive_start
		1288,		// hactive_end
		 137,		// hsync_start
		   1,		// hsync_end
		 806,		// vtotal_size
		  36,		// vactive_start
		 804,		// vactive_end
		   7,		// vsync_start
		   1,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		  27,		// pre_div_vclk (P)
		 130,		// post_div_vclk (M)
		   2,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   5,		// axclk_gen_div_s (S)
		   4,		// axclk_gen_div_f (F)
		   5		// axclk_gen_div_t (T)
	#else
		   6,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   4		// axclk_gen_div_t (T)
	#endif
	},
	{	// 1024x768p75
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		1312,		// htotal_size
		 240,		// hactive_start
		1264,		// hactive_end
		  97,		// hsync_start
		   1,		// hsync_end
		 800,		// vtotal_size
		  31,		// vactive_start
		 799,		// vactive_end
		   3,		// vsync_start
		   0,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		  24,		// pre_div_vclk (P)
		 140,		// post_div_vclk (M)
		   2,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   4,		// axclk_gen_div_s (S)
		   4,		// axclk_gen_div_f (F)
		   5		// axclk_gen_div_t (T)
	#else
		   5,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   7		// axclk_gen_div_t (T)
	#endif
	},
	{	// 1280x1024p60
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		1688,		// htotal_size
		 328,		// hactive_start
		1608,		// hactive_end
		 113,		// hsync_start
		   1,		// hsync_end
		1066,		// vtotal_size
		  41,		// vactive_start
		1065,		// vactive_end
		   3,		// vsync_start
		   0,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		   4,		// pre_div_vclk (P)
		  16,		// post_div_vclk (M)
		   1,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   3,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   2		// axclk_gen_div_t (T)
	#else
		   3,		// axclk_gen_div_s (S)
		   3,		// axclk_gen_div_f (F)
		   4		// axclk_gen_div_t (T)
	#endif
	},
	{	// 1280x1024p75
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		1688,		// htotal_size
		 360,		// hactive_start
		1640,		// hactive_end
		 145,		// hsync_start
		   1,		// hsync_end
		1066,		// vtotal_size
		  41,		// vactive_start
		1065,		// vactive_end
		   3,		// vsync_start
		   0,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		   4,		// pre_div_vclk (P)
		  20,		// post_div_vclk (M)
		   1,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   2,		// axclk_gen_div_s (S)
		   4,		// axclk_gen_div_f (F)
		   5		// axclk_gen_div_t (T)
	#else
		   2,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   1		// axclk_gen_div_t (T)
	#endif
	},

	{	// 1360x768p60
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		1792,		// htotal_size
		 336,		// hactive_start
		1696,		// hactive_end
		 113,		// hsync_start
		   1,		// hsync_end
		 795,		// vtotal_size
		  25,		// vactive_start
		 793,		// vactive_end
		   7,		// vsync_start
		   1,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
				
		  27,		// pre_div_vclk (P)
		 171,		// post_div_vclk (M)
		   2,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   4,		// axclk_gen_div_s (S)
		   8,		// axclk_gen_div_f (F)
		  19		// axclk_gen_div_t (T)
	#else
		   4,		// axclk_gen_div_s (S)
		  14,		// axclk_gen_div_f (F)
		  19		// axclk_gen_div_t (T)
	#endif
	},
	{	// 1600x1200p60
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		2160,		// htotal_size
		 464,		// hactive_start
		2064,		// hactive_end
		 193,		// hsync_start
		   1,		// hsync_end
		1250,		// vtotal_size
		  49,		// vactive_start
		1249,		// vactive_end
		   3,		// vsync_start
		   0,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
	
		   4,		// pre_div_vclk (P)
		  24,		// post_div_vclk (M)
		   1,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   2,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   3		// axclk_gen_div_t (T)
	#else
		   2,		// axclk_gen_div_s (S)
		   1,		// axclk_gen_div_f (F)
		   2		// axclk_gen_div_t (T)
	#endif
	},

	{	// 1440x900p60
		  20,		// hdpulse_pos	
		  10,		// vdpulse_pos	
		   0,		// vdpulse_pos_bot	
		1904,		// htotal_size	
		 352,		// hactive_start	
		1792,		// hactive_end	
		 153,		// hsync_start	
		   1,		// hsync_end	
		 934,		// vtotal_size	
		  32,		// vactive_start	
		 932,		// vactive_end	
		   7,		// vsync_start	
		   1,		// vsync_end	
		   0,		// vactive_start_bot	
		   0,		// vactive_end_bot	
		   0,		// vsync_start_bot	
		   0,		// vsync_end_bot	
		   0,		// vsync_bot_fld_pos	
					
		  54,		// pre_div_vclk (P)	
		 213,		// post_div_vclk (M)	
		   1,		// post_scale_vclk (S)	

	#if defined(SYSTEM_USE_MCLK189)
		   3,		// axclk_gen_div_s (S)
		  39,		// axclk_gen_div_f (F)
		  71		// axclk_gen_div_t (T)
	#else
		   3,		// axclk_gen_div_s (S)
		  57,		// axclk_gen_div_f (F)
		  71		// axclk_gen_div_t (T)
	#endif
	},
	{	// 1440x900p75
		  20,		// hdpulse_pos	
		  10,		// vdpulse_pos	
		   0,		// vdpulse_pos_bot	
		1936,		// htotal_size	
		 368,		// hactive_start	
		1808,		// hactive_end	
		 153,		// hsync_start	
		   1,		// hsync_end	
		 942,		// vtotal_size	
		  40,		// vactive_start	
		 940,		// vactive_end	
		   7,		// vsync_start	
		   1,		// vsync_end	
		   0,		// vactive_start_bot	
		   0,		// vactive_end_bot	
		   0,		// vsync_start_bot	
		   0,		// vsync_end_bot	
		   0,		// vsync_bot_fld_pos	
					
		  48,		// pre_div_vclk (P)	
		 243,		// post_div_vclk (M)	
		   1,		// post_scale_vclk (S)	

	#if defined(SYSTEM_USE_MCLK189)
		   2,		// axclk_gen_div_s (S)
		  62,		// axclk_gen_div_f (F)
		  81		// axclk_gen_div_t (T)
	#else
		   2,		// axclk_gen_div_s (S)
		  26,		// axclk_gen_div_f (F)
		  27		// axclk_gen_div_t (T)
	#endif
	},
	{	// 1680x1050p60
		  20,		// hdpulse_pos	
		  10,		// vdpulse_pos	
		   0,		// vdpulse_pos_bot	
		2240,		// htotal_size	
		 424,		// hactive_start	
		2104,		// hactive_end	
		 177,		// hsync_start	
		   1,		// hsync_end	
		1089,		// vtotal_size	
		  37,		// vactive_start	
		1087,		// vactive_end	
		   7,		// vsync_start	
		   1,		// vsync_end	
		   0,		// vactive_start_bot	
		   0,		// vactive_end_bot	
		   0,		// vsync_start_bot	
		   0,		// vsync_end_bot	
		   0,		// vsync_bot_fld_pos	
					
		  24,		// pre_div_vclk (P)	
		 130,		// post_div_vclk (M)	
		   1,		// post_scale_vclk (S)	

	#if defined(SYSTEM_USE_MCLK189)
		   2,		// axclk_gen_div_s (S)
		  38,		// axclk_gen_div_f (F)
		  65		// axclk_gen_div_t (T)
	#else
		   2,		// axclk_gen_div_s (S)
		  10,		// axclk_gen_div_f (F)
		  13		// axclk_gen_div_t (T)
	#endif
	},

	{
		// 1680x1050p60 Reduced Blanking
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		1840,		// htotal_size
		  80,		// hactive_start
		1760,		// hactive_end
		  33,		// hsync_start
		   1,		// hsync_end
		1080,		// vtotal_size
		  28,		// vactive_start
		1078,		// vactive_end
		   7,		// vsync_start
		   1,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
		 
		  27,		// pre_div_vclk (P)
		 119,		// post_div_vclk (M)
		   1,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   3,		// axclk_gen_div_s (S)
		   3,		// axclk_gen_div_f (F)
		  17		// axclk_gen_div_t (T)
	#else
		   3,		// axclk_gen_div_s (S)
		  25,		// axclk_gen_div_f (F)
		  62		// axclk_gen_div_t (T)
	#endif
	},
	{
		// 1920x1080p60 Reduced Blanking
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		2080,		// htotal_size
		  80,		// hactive_start
		2000,		// hactive_end
		  33,		// hsync_start
		   1,		// hsync_end
		1111,		// vtotal_size
		  30,		// vactive_start
		1110,		// vactive_end
		   7,		// vsync_start
		   2,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
		 
		  54,		// pre_div_vclk (P)
		 277,		// post_div_vclk (M)
		   1,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   2,		// axclk_gen_div_s (S)
		  35,		// axclk_gen_div_f (F)
		  48		// axclk_gen_div_t (T)
	#else
		   2,		// axclk_gen_div_s (S)
		  61,		// axclk_gen_div_f (F)
		  66		// axclk_gen_div_t (T)
	#endif
	},
	{
		// 1920x1200p60 Reduced Blanking
		  20,		// hdpulse_pos
		  10,		// vdpulse_pos
		   0,		// vdpulse_pos_bot
		2080,		// htotal_size
		  80,		// hactive_start
		2000,		// hactive_end
		  33,		// hsync_start
		   1,		// hsync_end
		1235,		// vtotal_size
		  33,		// vactive_start
		1233,		// vactive_end
		   7,		// vsync_start
		   1,		// vsync_end
		   0,		// vactive_start_bot
		   0,		// vactive_end_bot
		   0,		// vsync_start_bot
		   0,		// vsync_end_bot
		   0,		// vsync_bot_fld_pos
		 
		  27,		// pre_div_vclk (P)
		 154,		// post_div_vclk (M)
		   1,		// post_scale_vclk (S)

	#if defined(SYSTEM_USE_MCLK189)
		   2,		// axclk_gen_div_s (S)
		   5,		// axclk_gen_div_f (F)
		  11		// axclk_gen_div_t (T)
	#else
		   2,		// axclk_gen_div_s (S)
		  17,		// axclk_gen_div_f (F)
		  27		// axclk_gen_div_t (T)
	#endif
	}
};

// DAC sync control
ROMDATA MDIN_DACSYNC_CTRL defMDINDACData[]		= {
	{	// 720x480i60
	   { 76,			// 0x00 : dac_sync_level_a                                
		153,			// 0x01 : dac_sync_level_b                                
		229,			// 0x02 : dac_sync_level_c                                
		383,			// 0x03 : dac_sync_level_d                                
		460,			// 0x04 : dac_sync_level_e                                
		536,			// 0x05 : dac_sync_level_f                                
		614,			// 0x06 : dac_sync_level_g                                
		307,			// 0x07 : dac_blank_level_g                               
		307,			// 0x08 : dac_blank_level_br                              
		 32,			// 0x09 : dac_heq_pulse_size                              
		 64,			// 0x0A : dac_hsync_size                                  
		429,			// 0x0B : dac_htotal_half_size                            
		 48,			// 0x0C : dac_hfp_interval                                
		 90,			// 0x0D : dac_hbp_interval                                
		10<<12 |   1,	// 0x0E : dac_line_format1(15:12), dac_vstart_pos1(10:0)  
		 5<<12 |   3,	// 0x0F : dac_line_format2(15:12), dac_vstart_pos2(10:0)  
		10<<12 |   6,	// 0x10 : dac_line_format3(15:12), dac_vstart_pos3(10:0)  
		 0<<12 |   9,	// 0x11 : dac_line_format4(15:12), dac_vstart_pos4(10:0)  
		 0<<12 |  21,	// 0x12 : dac_line_format5(15:12), dac_vstart_pos5(10:0)  
		 2<<12 | 262,	// 0x13 : dac_line_format6(15:12), dac_vstart_pos6(10:0)  
		10<<12 | 263,	// 0x14 : dac_line_format7(15:12), dac_vstart_pos7(10:0)  
		 9<<12 | 265,	// 0x15 : dac_line_format8(15:12), dac_vstart_pos8(10:0)  
		 5<<12 | 266,	// 0x16 : dac_line_format9(15:12), dac_vstart_pos9(10:0)  
		 6<<12 | 268,	// 0x17 : dac_line_format10(15:12), dac_vstart_pos10(10:0)
		10<<12 | 269,	// 0x18 : dac_line_format11(15:12), dac_vstart_pos11(10:0)
		 8<<12 | 271,	// 0x19 : dac_line_format12(15:12), dac_vstart_pos12(10:0)
		 0<<12 | 272,	// 0x1A : dac_line_format13(15:12), dac_vstart_pos13(10:0)
		 0<<12 | 284,	// 0x1B : dac_line_format14(15:12), dac_vstart_pos14(10:0)
		10<<12 |   0,	// 0x1C : dac_line_format15(15:12), dac_vstart_pos15(10:0)
		10<<12 |   0,	// 0x1D : dac_line_format16(15:12), dac_vstart_pos16(10:0)
		 1<<15 | 717,	// 0x1E : dac_scale_g_en(15), dac_scale_g(9:0)            
		 1<<15 | 717,	// 0x1F : dac_scale_b_en(15), dac_scale_b(9:0)            
		 1<<15 | 717,	// 0x20 : dac_scale_r_en(15), dac_scale_r(9:0)            
		 1<<15 | 307,	// 0x21 : dac_clip_l_g_en(15), dac_clip_l_g(9:0)          
		 1<<15 | 1023,	// 0x22 : dac_clip_u_g_en(15), dac_clip_u_g(9:0)          
		 1<<15 | 153,	// 0x23 : dac_clip_l_br_en(15), dac_clip_l_br(9:0)        
		 1<<15 | 871},	// 0x24 : dac_clip_u_br_en(15), dac_clip_u_br(9:0)     
		 0				// dummy
	},
	{	// 720x576i50
	   { 76,			// 0x00 : dac_sync_level_a                                
		153,			// 0x01 : dac_sync_level_b                                
		229,			// 0x02 : dac_sync_level_c                                
		383,			// 0x03 : dac_sync_level_d                                
		460,			// 0x04 : dac_sync_level_e                                
		536,			// 0x05 : dac_sync_level_f                                
		614,			// 0x06 : dac_sync_level_g                                
		307,			// 0x07 : dac_blank_level_g                               
		307,			// 0x08 : dac_blank_level_br                              
		 32,			// 0x09 : dac_heq_pulse_size                              
		 64,			// 0x0A : dac_hsync_size                                  
		432,			// 0x0B : dac_htotal_half_size                            
		 44,			// 0x0C : dac_hfp_interval                                
		100,			// 0x0D : dac_hbp_interval                                
		 5<<12 |   1,	// 0x0E : dac_line_format1(15:12), dac_vstart_pos1(10:0)  
		 5<<12 |   3,	// 0x0F : dac_line_format2(15:12), dac_vstart_pos2(10:0)  
		10<<12 |   4,	// 0x10 : dac_line_format3(15:12), dac_vstart_pos3(10:0)  
		 0<<12 |   6,	// 0x11 : dac_line_format4(15:12), dac_vstart_pos4(10:0)  
		 0<<12 |  22,	// 0x12 : dac_line_format5(15:12), dac_vstart_pos5(10:0)  
		10<<12 | 311,	// 0x13 : dac_line_format6(15:12), dac_vstart_pos6(10:0)  
		 9<<12 | 313,	// 0x14 : dac_line_format7(15:12), dac_vstart_pos7(10:0)  
		 5<<12 | 314,	// 0x15 : dac_line_format8(15:12), dac_vstart_pos8(10:0)  
		 6<<12 | 316,	// 0x16 : dac_line_format9(15:12), dac_vstart_pos9(10:0)  
		10<<12 | 317,	// 0x17 : dac_line_format10(15:12), dac_vstart_pos10(10:0)  
		 8<<12 | 318,	// 0x18 : dac_line_format11(15:12), dac_vstart_pos11(10:0)
		 0<<12 | 319,	// 0x19 : dac_line_format12(15:12), dac_vstart_pos12(10:0)
		 0<<12 | 335,	// 0x1A : dac_line_format13(15:12), dac_vstart_pos13(10:0)
		10<<12 | 624,	// 0x1B : dac_line_format14(15:12), dac_vstart_pos14(10:0)
		10<<12 |   0,	// 0x1C : dac_line_format15(15:12), dac_vstart_pos15(10:0)
		10<<12 |   0,	// 0x1D : dac_line_format16(15:12), dac_vstart_pos16(10:0)
		 1<<15 | 717,	// 0x1E : dac_scale_g_en(15), dac_scale_g(9:0)            
		 1<<15 | 717,	// 0x1F : dac_scale_b_en(15), dac_scale_b(9:0)            
		 1<<15 | 717,	// 0x20 : dac_scale_r_en(15), dac_scale_r(9:0)            
		 1<<15 | 307,	// 0x21 : dac_clip_l_g_en(15), dac_clip_l_g(9:0)          
		 1<<15 | 1023,	// 0x22 : dac_clip_u_g_en(15), dac_clip_u_g(9:0)          
		 1<<15 | 153,	// 0x23 : dac_clip_l_br_en(15), dac_clip_l_br(9:0)        
		 1<<15 | 871},	// 0x24 : dac_clip_u_br_en(15), dac_clip_u_br(9:0)        
		 0				// dummy
	},
	{	// 720x480p60
	   { 76,			// 0x00 : dac_sync_level_a                                
		153,			// 0x01 : dac_sync_level_b                                
		229,			// 0x02 : dac_sync_level_c                                
		383,			// 0x03 : dac_sync_level_d                                
		460,			// 0x04 : dac_sync_level_e                                
		536,			// 0x05 : dac_sync_level_f                                
		614,			// 0x06 : dac_sync_level_g                                
		307,			// 0x07 : dac_blank_level_g                               
		307,			// 0x08 : dac_blank_level_br                              
		 32,			// 0x09 : dac_heq_pulse_size                              
		 64,			// 0x0A : dac_hsync_size                                  
		429,			// 0x0B : dac_htotal_half_size                            
		 48,			// 0x0C : dac_hfp_interval                                
		 90,			// 0x0D : dac_hbp_interval                                
		 0<<12 |   1,	// 0x0E : dac_line_format1(15:12), dac_vstart_pos1(10:0)  
		15<<12 |   7,	// 0x0F : dac_line_format2(15:12), dac_vstart_pos2(10:0)  
		 0<<12 |  13,	// 0x10 : dac_line_format3(15:12), dac_vstart_pos3(10:0)  
		 0<<12 |  43,	// 0x11 : dac_line_format4(15:12), dac_vstart_pos4(10:0)  
		 0<<12 | 523,	// 0x12 : dac_line_format5(15:12), dac_vstart_pos5(10:0)  
		 0<<12 |   0,	// 0x13 : dac_line_format6(15:12), dac_vstart_pos6(10:0)  
		 0<<12 |   0,	// 0x14 : dac_line_format7(15:12), dac_vstart_pos7(10:0)  
		 0<<12 |   0,	// 0x15 : dac_line_format8(15:12), dac_vstart_pos8(10:0)  
		 0<<12 |   0,	// 0x16 : dac_line_format9(15:12), dac_vstart_pos9(10:0)  
		 0<<12 |   0,	// 0x17 : dac_line_format10(15:12), dac_vstart_pos10(10:0)
		 0<<12 |   0,	// 0x18 : dac_line_format11(15:12), dac_vstart_pos11(10:0)
		 0<<12 |   0,	// 0x19 : dac_line_format12(15:12), dac_vstart_pos12(10:0)
		 0<<12 |   0,	// 0x1A : dac_line_format13(15:12), dac_vstart_pos13(10:0)
		 0<<12 |   0,	// 0x1B : dac_line_format14(15:12), dac_vstart_pos14(10:0)
		 0<<12 |   0,	// 0x1C : dac_line_format15(15:12), dac_vstart_pos15(10:0)
		 0<<12 |   0,	// 0x1D : dac_line_format16(15:12), dac_vstart_pos16(10:0)
		 1<<15 | 717,	// 0x1E : dac_scale_g_en(15), dac_scale_g(9:0)            
		 1<<15 | 717,	// 0x1F : dac_scale_b_en(15), dac_scale_b(9:0)            
		 1<<15 | 717,	// 0x20 : dac_scale_r_en(15), dac_scale_r(9:0)            
		 1<<15 | 307,	// 0x21 : dac_clip_l_g_en(15), dac_clip_l_g(9:0)          
		 1<<15 | 1023,	// 0x22 : dac_clip_u_g_en(15), dac_clip_u_g(9:0)          
		 1<<15 | 153,	// 0x23 : dac_clip_l_br_en(15), dac_clip_l_br(9:0)        
		 1<<15 | 871},	// 0x24 : dac_clip_u_br_en(15), dac_clip_u_br(9:0)        
		 0				// dummy
	},
	{	// 720x576p50
	   { 76,			// 0x00 : dac_sync_level_a                                
		153,			// 0x01 : dac_sync_level_b                                
		229,			// 0x02 : dac_sync_level_c                                
		383,			// 0x03 : dac_sync_level_d                                
		460,			// 0x04 : dac_sync_level_e                                
		536,			// 0x05 : dac_sync_level_f                                
		614,			// 0x06 : dac_sync_level_g                                
		307,			// 0x07 : dac_blank_level_g                               
		307,			// 0x08 : dac_blank_level_br                              
		 32,			// 0x09 : dac_heq_pulse_size                              
		 64,			// 0x0A : dac_hsync_size                                  
		432,			// 0x0B : dac_htotal_half_size                            
		 44,			// 0x0C : dac_hfp_interval                                
		100,			// 0x0D : dac_hbp_interval                                
		15<<12 |   1,	// 0x0E : dac_line_format1(15:12), dac_vstart_pos1(10:0)  
		 0<<12 |   6,	// 0x0F : dac_line_format2(15:12), dac_vstart_pos2(10:0)  
		 0<<12 |  45,	// 0x10 : dac_line_format3(15:12), dac_vstart_pos3(10:0)  
		 0<<12 | 621,	// 0x11 : dac_line_format4(15:12), dac_vstart_pos4(10:0)  
		 0<<12 |   0,	// 0x12 : dac_line_format5(15:12), dac_vstart_pos5(10:0)  
		 0<<12 |   0,	// 0x13 : dac_line_format6(15:12), dac_vstart_pos6(10:0)  
		 0<<12 |   0,	// 0x14 : dac_line_format7(15:12), dac_vstart_pos7(10:0)  
		 0<<12 |   0,	// 0x15 : dac_line_format8(15:12), dac_vstart_pos8(10:0)  
		 0<<12 |   0,	// 0x16 : dac_line_format9(15:12), dac_vstart_pos9(10:0)  
		 0<<12 |   0,	// 0x17 : dac_line_format10(15:12), dac_vstart_pos10(10:0)
		 0<<12 |   0,	// 0x18 : dac_line_format11(15:12), dac_vstart_pos11(10:0)
		 0<<12 |   0,	// 0x19 : dac_line_format12(15:12), dac_vstart_pos12(10:0)
		 0<<12 |   0,	// 0x1A : dac_line_format13(15:12), dac_vstart_pos13(10:0)
		 0<<12 |   0,	// 0x1B : dac_line_format14(15:12), dac_vstart_pos14(10:0)
		 0<<12 |   0,	// 0x1C : dac_line_format15(15:12), dac_vstart_pos15(10:0)
		 0<<12 |   0,	// 0x1D : dac_line_format16(15:12), dac_vstart_pos16(10:0)
		 1<<15 | 717,	// 0x1E : dac_scale_g_en(15), dac_scale_g(9:0)            
		 1<<15 | 717,	// 0x1F : dac_scale_b_en(15), dac_scale_b(9:0)            
		 1<<15 | 717,	// 0x20 : dac_scale_r_en(15), dac_scale_r(9:0)            
		 1<<15 | 307,	// 0x21 : dac_clip_l_g_en(15), dac_clip_l_g(9:0)          
		 1<<15 | 1023,	// 0x22 : dac_clip_u_g_en(15), dac_clip_u_g(9:0)          
		 1<<15 | 153,	// 0x23 : dac_clip_l_br_en(15), dac_clip_l_br(9:0)        
		 1<<15 | 871},	// 0x24 : dac_clip_u_br_en(15), dac_clip_u_br(9:0)        
		 0				// dummy
	},
	{	// 1280x720p60
	   { 76,			// 0x00 : dac_sync_level_a                                
		153,			// 0x01 : dac_sync_level_b                                
		229,			// 0x02 : dac_sync_level_c                                
		383,			// 0x03 : dac_sync_level_d                                
		460,			// 0x04 : dac_sync_level_e                                
		536,			// 0x05 : dac_sync_level_f                                
		614,			// 0x06 : dac_sync_level_g                                
		307,			// 0x07 : dac_blank_level_g                               
		307,			// 0x08 : dac_blank_level_br                              
		 20,			// 0x09 : dac_heq_pulse_size                              
		 41,			// 0x0A : dac_hsync_size                                  
		825,			// 0x0B : dac_htotal_half_size                            
		110,			// 0x0C : dac_hfp_interval                                
		260,			// 0x0D : dac_hbp_interval                                
		15<<12 |   1,	// 0x0E : dac_line_format1(15:12), dac_vstart_pos1(10:0)  
		 0<<12 |   6,	// 0x0F : dac_line_format2(15:12), dac_vstart_pos2(10:0)  
		 0<<12 |  26,	// 0x10 : dac_line_format3(15:12), dac_vstart_pos3(10:0)  
		 0<<12 | 746,	// 0x11 : dac_line_format4(15:12), dac_vstart_pos4(10:0)  
		 0<<12 |   0,	// 0x12 : dac_line_format5(15:12), dac_vstart_pos5(10:0)  
		 0<<12 |   0,	// 0x13 : dac_line_format6(15:12), dac_vstart_pos6(10:0)  
		 0<<12 |   0,	// 0x14 : dac_line_format7(15:12), dac_vstart_pos7(10:0)  
		 0<<12 |   0,	// 0x15 : dac_line_format8(15:12), dac_vstart_pos8(10:0)  
		 0<<12 |   0,	// 0x16 : dac_line_format9(15:12), dac_vstart_pos9(10:0)  
		 0<<12 |   0,	// 0x17 : dac_line_format10(15:12), dac_vstart_pos10(10:0)
		 0<<12 |   0,	// 0x18 : dac_line_format11(15:12), dac_vstart_pos11(10:0)
		 0<<12 |   0,	// 0x19 : dac_line_format12(15:12), dac_vstart_pos12(10:0)
		 0<<12 |   0,	// 0x1A : dac_line_format13(15:12), dac_vstart_pos13(10:0)
		 0<<12 |   0,	// 0x1B : dac_line_format14(15:12), dac_vstart_pos14(10:0)
		 0<<12 |   0,	// 0x1C : dac_line_format15(15:12), dac_vstart_pos15(10:0)
		 0<<12 |   0,	// 0x1D : dac_line_format16(15:12), dac_vstart_pos16(10:0)
		 1<<15 | 717,	// 0x1E : dac_scale_g_en(15), dac_scale_g(9:0)            
		 1<<15 | 717,	// 0x1F : dac_scale_b_en(15), dac_scale_b(9:0)            
		 1<<15 | 717,	// 0x20 : dac_scale_r_en(15), dac_scale_r(9:0)            
		 1<<15 | 307,	// 0x21 : dac_clip_l_g_en(15), dac_clip_l_g(9:0)          
		 1<<15 | 1023,	// 0x22 : dac_clip_u_g_en(15), dac_clip_u_g(9:0)          
		 1<<15 | 153,	// 0x23 : dac_clip_l_br_en(15), dac_clip_l_br(9:0)        
		 1<<15 | 871},	// 0x24 : dac_clip_u_br_en(15), dac_clip_u_br(9:0)        
		 0				// dummy
	},
	{	// 1280x720p50
	   { 76,			// 0x00 : dac_sync_level_a                                
		153,			// 0x01 : dac_sync_level_b                                
		229,			// 0x02 : dac_sync_level_c                                
		383,			// 0x03 : dac_sync_level_d                                
		460,			// 0x04 : dac_sync_level_e                                
		536,			// 0x05 : dac_sync_level_f                                
		614,			// 0x06 : dac_sync_level_g                                
		307,			// 0x07 : dac_blank_level_g                               
		307,			// 0x08 : dac_blank_level_br                              
		 20,			// 0x09 : dac_heq_pulse_size                              
		 41,			// 0x0A : dac_hsync_size                                  
		990,			// 0x0B : dac_htotal_half_size                            
		440,			// 0x0C : dac_hfp_interval                                
		260,			// 0x0D : dac_hbp_interval                                
		15<<12 |   1,	// 0x0E : dac_line_format1(15:12), dac_vstart_pos1(10:0)  
		 0<<12 |   6,	// 0x0F : dac_line_format2(15:12), dac_vstart_pos2(10:0)  
		 0<<12 |  26,	// 0x10 : dac_line_format3(15:12), dac_vstart_pos3(10:0)  
		 0<<12 | 746,	// 0x11 : dac_line_format4(15:12), dac_vstart_pos4(10:0)  
		 0<<12 |   0,	// 0x12 : dac_line_format5(15:12), dac_vstart_pos5(10:0)  
		 0<<12 |   0,	// 0x13 : dac_line_format6(15:12), dac_vstart_pos6(10:0)  
		 0<<12 |   0,	// 0x14 : dac_line_format7(15:12), dac_vstart_pos7(10:0)  
		 0<<12 |   0,	// 0x15 : dac_line_format8(15:12), dac_vstart_pos8(10:0)  
		 0<<12 |   0,	// 0x16 : dac_line_format9(15:12), dac_vstart_pos9(10:0)  
		 0<<12 |   0,	// 0x17 : dac_line_format10(15:12), dac_vstart_pos10(10:0)
		 0<<12 |   0,	// 0x18 : dac_line_format11(15:12), dac_vstart_pos11(10:0)
		 0<<12 |   0,	// 0x19 : dac_line_format12(15:12), dac_vstart_pos12(10:0)
		 0<<12 |   0,	// 0x1A : dac_line_format13(15:12), dac_vstart_pos13(10:0)
		 0<<12 |   0,	// 0x1B : dac_line_format14(15:12), dac_vstart_pos14(10:0)
		 0<<12 |   0,	// 0x1C : dac_line_format15(15:12), dac_vstart_pos15(10:0)
		 0<<12 |   0,	// 0x1D : dac_line_format16(15:12), dac_vstart_pos16(10:0)
		 1<<15 | 717,	// 0x1E : dac_scale_g_en(15), dac_scale_g(9:0)            
		 1<<15 | 717,	// 0x1F : dac_scale_b_en(15), dac_scale_b(9:0)            
		 1<<15 | 717,	// 0x20 : dac_scale_r_en(15), dac_scale_r(9:0)            
		 1<<15 | 307,	// 0x21 : dac_clip_l_g_en(15), dac_clip_l_g(9:0)          
		 1<<15 | 1023,	// 0x22 : dac_clip_u_g_en(15), dac_clip_u_g(9:0)          
		 1<<15 | 153,	// 0x23 : dac_clip_l_br_en(15), dac_clip_l_br(9:0)        
		 1<<15 | 871},	// 0x24 : dac_clip_u_br_en(15), dac_clip_u_br(9:0)        
		 0				// dummy
	},
	{	// 1920x1080i60
	   { 76,			// 0x00 : dac_sync_level_a                                
		153,			// 0x01 : dac_sync_level_b                                
		229,			// 0x02 : dac_sync_level_c                                
		383,			// 0x03 : dac_sync_level_d                                
		460,			// 0x04 : dac_sync_level_e                                
		536,			// 0x05 : dac_sync_level_f                                
		614,			// 0x06 : dac_sync_level_g                                
		307,			// 0x07 : dac_blank_level_g                               
		307,			// 0x08 : dac_blank_level_br                              
		 22,			// 0x09 : dac_heq_pulse_size                              
		 45,			// 0x0A : dac_hsync_size                                  
	   1100,			// 0x0B : dac_htotal_half_size                            
		 88,			// 0x0C : dac_hfp_interval                                
		133,			// 0x0D : dac_hbp_interval                                
		 5<<12 |   1,	// 0x0E : dac_line_format1(15:12), dac_vstart_pos1(10:0)  
		10<<12 |   6,	// 0x0F : dac_line_format2(15:12), dac_vstart_pos2(10:0)  
		 0<<12 |   7,	// 0x10 : dac_line_format3(15:12), dac_vstart_pos3(10:0)  
		 0<<12 |  21,	// 0x11 : dac_line_format4(15:12), dac_vstart_pos4(10:0)  
		 0<<12 | 561,	// 0x12 : dac_line_format5(15:12), dac_vstart_pos5(10:0)  
		 9<<12 | 563,	// 0x13 : dac_line_format6(15:12), dac_vstart_pos6(10:0)  
		 5<<12 | 564,	// 0x14 : dac_line_format7(15:12), dac_vstart_pos7(10:0)  
		 6<<12 | 568,	// 0x15 : dac_line_format8(15:12), dac_vstart_pos8(10:0)  
		 0<<12 | 569,	// 0x16 : dac_line_format9(15:12), dac_vstart_pos9(10:0)  
		 0<<12 | 584,	// 0x17 : dac_line_format10(15:12), dac_vstart_pos10(10:0)
		 0<<12 | 1124,	// 0x18 : dac_line_format11(15:12), dac_vstart_pos11(10:0)
		 0<<12 |   0,	// 0x19 : dac_line_format12(15:12), dac_vstart_pos12(10:0)
		 0<<12 |   0,	// 0x1A : dac_line_format13(15:12), dac_vstart_pos13(10:0)
		 0<<12 |   0,	// 0x1B : dac_line_format14(15:12), dac_vstart_pos14(10:0)
		 0<<12 |   0,	// 0x1C : dac_line_format15(15:12), dac_vstart_pos15(10:0)
		 0<<12 |   0,	// 0x1D : dac_line_format16(15:12), dac_vstart_pos16(10:0)
		 1<<15 | 717,	// 0x1E : dac_scale_g_en(15), dac_scale_g(9:0)            
		 1<<15 | 717,	// 0x1F : dac_scale_b_en(15), dac_scale_b(9:0)            
		 1<<15 | 717,	// 0x20 : dac_scale_r_en(15), dac_scale_r(9:0)            
		 1<<15 | 307,	// 0x21 : dac_clip_l_g_en(15), dac_clip_l_g(9:0)          
		 1<<15 | 1023,	// 0x22 : dac_clip_u_g_en(15), dac_clip_u_g(9:0)          
		 1<<15 | 153,	// 0x23 : dac_clip_l_br_en(15), dac_clip_l_br(9:0)        
		 1<<15 | 871},	// 0x24 : dac_clip_u_br_en(15), dac_clip_u_br(9:0)        
		 0				// dummy
	},
	{	// 1920x1080i50
	   { 76,			// 0x00 : dac_sync_level_a                                
		153,			// 0x01 : dac_sync_level_b                                
		229,			// 0x02 : dac_sync_level_c                                
		383,			// 0x03 : dac_sync_level_d                                
		460,			// 0x04 : dac_sync_level_e                                
		536,			// 0x05 : dac_sync_level_f                                
		614,			// 0x06 : dac_sync_level_g                                
		307,			// 0x07 : dac_blank_level_g                               
		307,			// 0x08 : dac_blank_level_br                              
		 22,			// 0x09 : dac_heq_pulse_size                              
		 45,			// 0x0A : dac_hsync_size                                  
	   1320,			// 0x0B : dac_htotal_half_size                            
		313,			// 0x0C : dac_hfp_interval                                
		133,			// 0x0D : dac_hbp_interval                                
		 5<<12 |   1,	// 0x0E : dac_line_format1(15:12), dac_vstart_pos1(10:0)  
		10<<12 |   6,	// 0x0F : dac_line_format2(15:12), dac_vstart_pos2(10:0)  
		 0<<12 |   7,	// 0x10 : dac_line_format3(15:12), dac_vstart_pos3(10:0)  
		 0<<12 |  21,	// 0x11 : dac_line_format4(15:12), dac_vstart_pos4(10:0)  
		 0<<12 | 561,	// 0x12 : dac_line_format5(15:12), dac_vstart_pos5(10:0)  
		 9<<12 | 563,	// 0x13 : dac_line_format6(15:12), dac_vstart_pos6(10:0)  
		 5<<12 | 564,	// 0x14 : dac_line_format7(15:12), dac_vstart_pos7(10:0)  
		 6<<12 | 568,	// 0x15 : dac_line_format8(15:12), dac_vstart_pos8(10:0)  
		 0<<12 | 569,	// 0x16 : dac_line_format9(15:12), dac_vstart_pos9(10:0)  
		 0<<12 | 584,	// 0x17 : dac_line_format10(15:12), dac_vstart_pos10(10:0)
		 0<<12 | 1124,	// 0x18 : dac_line_format11(15:12), dac_vstart_pos11(10:0)
		 0<<12 |   0,	// 0x19 : dac_line_format12(15:12), dac_vstart_pos12(10:0)
		 0<<12 |   0,	// 0x1A : dac_line_format13(15:12), dac_vstart_pos13(10:0)
		 0<<12 |   0,	// 0x1B : dac_line_format14(15:12), dac_vstart_pos14(10:0)
		 0<<12 |   0,	// 0x1C : dac_line_format15(15:12), dac_vstart_pos15(10:0)
		 0<<12 |   0,	// 0x1D : dac_line_format16(15:12), dac_vstart_pos16(10:0)
		 1<<15 | 717,	// 0x1E : dac_scale_g_en(15), dac_scale_g(9:0)            
		 1<<15 | 717,	// 0x1F : dac_scale_b_en(15), dac_scale_b(9:0)            
		 1<<15 | 717,	// 0x20 : dac_scale_r_en(15), dac_scale_r(9:0)            
		 1<<15 | 307,	// 0x21 : dac_clip_l_g_en(15), dac_clip_l_g(9:0)          
		 1<<15 | 1023,	// 0x22 : dac_clip_u_g_en(15), dac_clip_u_g(9:0)          
		 1<<15 | 153,	// 0x23 : dac_clip_l_br_en(15), dac_clip_l_br(9:0)        
		 1<<15 | 871},	// 0x24 : dac_clip_u_br_en(15), dac_clip_u_br(9:0)        
		 0				// dummy
	},
	{	// 1920x1080p60
	   { 76,			// 0x00 : dac_sync_level_a                                
		153,			// 0x01 : dac_sync_level_b                                
		229,			// 0x02 : dac_sync_level_c                                
		383,			// 0x03 : dac_sync_level_d                                
		460,			// 0x04 : dac_sync_level_e                                
		536,			// 0x05 : dac_sync_level_f                                
		614,			// 0x06 : dac_sync_level_g                                
		307,			// 0x07 : dac_blank_level_g                               
		307,			// 0x08 : dac_blank_level_br                              
		 22,			// 0x09 : dac_heq_pulse_size                              
		 45,			// 0x0A : dac_hsync_size                                  
	   1100,			// 0x0B : dac_htotal_half_size                            
		 88,			// 0x0C : dac_hfp_interval                                
		133,			// 0x0D : dac_hbp_interval                                
		15<<12 |   1,	// 0x0E : dac_line_format1(15:12), dac_vstart_pos1(10:0)  
		 0<<12 |   6,	// 0x0F : dac_line_format2(15:12), dac_vstart_pos2(10:0)  
		 0<<12 |  42,	// 0x10 : dac_line_format3(15:12), dac_vstart_pos3(10:0)  
		 0<<12 | 1122,	// 0x11 : dac_line_format4(15:12), dac_vstart_pos4(10:0)  
		 0<<12 |   0,	// 0x12 : dac_line_format5(15:12), dac_vstart_pos5(10:0)  
		 0<<12 |   0,	// 0x13 : dac_line_format6(15:12), dac_vstart_pos6(10:0)  
		 0<<12 |   0,	// 0x14 : dac_line_format7(15:12), dac_vstart_pos7(10:0)  
		 0<<12 |   0,	// 0x15 : dac_line_format8(15:12), dac_vstart_pos8(10:0)  
		 0<<12 |   0,	// 0x16 : dac_line_format9(15:12), dac_vstart_pos9(10:0)  
		 0<<12 |   0,	// 0x17 : dac_line_format10(15:12), dac_vstart_pos10(10:0)
		 0<<12 |   0,	// 0x18 : dac_line_format11(15:12), dac_vstart_pos11(10:0)
		 0<<12 |   0,	// 0x19 : dac_line_format12(15:12), dac_vstart_pos12(10:0)
		 0<<12 |   0,	// 0x1A : dac_line_format13(15:12), dac_vstart_pos13(10:0)
		 0<<12 |   0,	// 0x1B : dac_line_format14(15:12), dac_vstart_pos14(10:0)
		 0<<12 |   0,	// 0x1C : dac_line_format15(15:12), dac_vstart_pos15(10:0)
		 0<<12 |   0,	// 0x1D : dac_line_format16(15:12), dac_vstart_pos16(10:0)
		 1<<15 | 717,	// 0x1E : dac_scale_g_en(15), dac_scale_g(9:0)            
		 1<<15 | 717,	// 0x1F : dac_scale_b_en(15), dac_scale_b(9:0)            
		 1<<15 | 717,	// 0x20 : dac_scale_r_en(15), dac_scale_r(9:0)            
		 1<<15 | 307,	// 0x21 : dac_clip_l_g_en(15), dac_clip_l_g(9:0)          
		 1<<15 | 1023,	// 0x22 : dac_clip_u_g_en(15), dac_clip_u_g(9:0)          
		 1<<15 | 153,	// 0x23 : dac_clip_l_br_en(15), dac_clip_l_br(9:0)        
		 1<<15 | 871},	// 0x24 : dac_clip_u_br_en(15), dac_clip_u_br(9:0)        
		 0				// dummy
	},
	{	// 1920x1080p50
	   { 76,			// 0x00 : dac_sync_level_a                                
		153,			// 0x01 : dac_sync_level_b                                
		229,			// 0x02 : dac_sync_level_c                                
		383,			// 0x03 : dac_sync_level_d                                
		460,			// 0x04 : dac_sync_level_e                                
		536,			// 0x05 : dac_sync_level_f                                
		614,			// 0x06 : dac_sync_level_g                                
		307,			// 0x07 : dac_blank_level_g                               
		307,			// 0x08 : dac_blank_level_br                              
		 22,			// 0x09 : dac_heq_pulse_size                              
		 45,			// 0x0A : dac_hsync_size                                  
	   1320,			// 0x0B : dac_htotal_half_size                            
		528,			// 0x0C : dac_hfp_interval                                
		133,			// 0x0D : dac_hbp_interval                                
		15<<12 |   1,	// 0x0E : dac_line_format1(15:12), dac_vstart_pos1(10:0)  
		 0<<12 |   6,	// 0x0F : dac_line_format2(15:12), dac_vstart_pos2(10:0)  
		 0<<12 |  42,	// 0x10 : dac_line_format3(15:12), dac_vstart_pos3(10:0)  
		 0<<12 | 1122,	// 0x11 : dac_line_format4(15:12), dac_vstart_pos4(10:0)  
		 0<<12 |   0,	// 0x12 : dac_line_format5(15:12), dac_vstart_pos5(10:0)  
		 0<<12 |   0,	// 0x13 : dac_line_format6(15:12), dac_vstart_pos6(10:0)  
		 0<<12 |   0,	// 0x14 : dac_line_format7(15:12), dac_vstart_pos7(10:0)  
		 0<<12 |   0,	// 0x15 : dac_line_format8(15:12), dac_vstart_pos8(10:0)  
		 0<<12 |   0,	// 0x16 : dac_line_format9(15:12), dac_vstart_pos9(10:0)  
		 0<<12 |   0,	// 0x17 : dac_line_format10(15:12), dac_vstart_pos10(10:0)
		 0<<12 |   0,	// 0x18 : dac_line_format11(15:12), dac_vstart_pos11(10:0)
		 0<<12 |   0,	// 0x19 : dac_line_format12(15:12), dac_vstart_pos12(10:0)
		 0<<12 |   0,	// 0x1A : dac_line_format13(15:12), dac_vstart_pos13(10:0)
		 0<<12 |   0,	// 0x1B : dac_line_format14(15:12), dac_vstart_pos14(10:0)
		 0<<12 |   0,	// 0x1C : dac_line_format15(15:12), dac_vstart_pos15(10:0)
		 0<<12 |   0,	// 0x1D : dac_line_format16(15:12), dac_vstart_pos16(10:0)
		 1<<15 | 717,	// 0x1E : dac_scale_g_en(15), dac_scale_g(9:0)            
		 1<<15 | 717,	// 0x1F : dac_scale_b_en(15), dac_scale_b(9:0)            
		 1<<15 | 717,	// 0x20 : dac_scale_r_en(15), dac_scale_r(9:0)            
		 1<<15 | 307,	// 0x21 : dac_clip_l_g_en(15), dac_clip_l_g(9:0)          
		 1<<15 | 1023,	// 0x22 : dac_clip_u_g_en(15), dac_clip_u_g(9:0)          
		 1<<15 | 153,	// 0x23 : dac_clip_l_br_en(15), dac_clip_l_br(9:0)        
		 1<<15 | 871},	// 0x24 : dac_clip_u_br_en(15), dac_clip_u_br(9:0)        
		 0				// dummy
	}
};
	
// default value for srcVideo format
ROMDATA MDIN_SRCVIDEO_ATTB defMDINSrcVideo[]		= {
//	 Htot					                   H/V Polarity & ScanType                                       Hact  Vact
	{ 858, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_INTR,  720,  480},	// 720x480i60
	{ 864, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_INTR,  720,  576},	// 720x576i50
	{ 858, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG,  720,  480},	// 720x480p60
	{ 864, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG,  720,  576},	// 720x576p50
	{1650, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1280,  720},	// 1280x720p60
	{1980, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1280,  720},	// 1280x720p50
	{2200, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_INTR, 1920, 1080},	// 1920x1080i60
	{2640, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_INTR, 1920, 1080},	// 1920x1080i50
	{2200, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1920, 1080},	// 1920x1080p60
	{2640, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1920, 1080},	// 1920x1080p50

	{ 800, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG,  640,  480},	// 640x480p60
	{1056, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG,  800,  600},	// 800x600p60
	{1344, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG, 1024,  768},	// 1024x768p60
	{1688, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1280, 1024},	// 1280x1024p60

	{1664, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1280,  720},	// 1280x720pRGB
	{1800, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1280,  960},	// 1280x960p60
	{1792, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1360,  768},	// 1360x768p60
	{1904, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1440,  900},	// 1440x900p60

#if VGA_SOURCE_EXTENSION == 1
	{ 832, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG,  640,  480},	// 640x480p72
	{ 840, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG,  640,  480},	// 640x480p75
	{1024, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG,  800,  600},	// 800x600p56
	{1040, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG,  800,  600},	// 800x600p72
	{1056, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG,  800,  600},	// 800x600p75
	{1328, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG, 1024,  768},	// 1024x768p70
	{1312, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1024,  768},	// 1024x768p75
	{1600, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1152,  864},	// 1152x864p75
	{1680, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1280,  800},	// 1280x800p60
#endif
};

// default value for outVideo format
ROMDATA MDIN_OUTVIDEO_ATTB defMDINOutVideo[]		= {
//	 Htot					                   H/V Polarity & ScanType                                       Hact  Vact
	{ 858, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_INTR,  720,  480},	// 720x480i60
	{ 864, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_INTR,  720,  576},	// 720x576i50
	{ 858, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG,  720,  480},	// 720x480p60
	{ 864, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG,  720,  576},	// 720x576p50
	{1650, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1280,  720},	// 1280x720p60
	{1980, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1280,  720},	// 1280x720p50
	{2200, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_INTR, 1920, 1080},	// 1920x1080i60
	{2640, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_INTR, 1920, 1080},	// 1920x1080i50
	{2200, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1920, 1080},	// 1920x1080p60
	{2640, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1920, 1080},	// 1920x1080p50

	{ 800, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG,  640,  480},	// 640x480p60
	{ 840, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG,  640,  480},	// 640x480p75
	{1056, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG,  800,  600},	// 800x600p60
	{1056, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG,  800,  600},	// 800x600p75
	{1344, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG, 1024,  768},	// 1024x768p60
	{1312, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1024,  768},	// 1024x768p75
	{1688, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1280, 1024},	// 1280x1024p60
	{1688, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1280, 1024},	// 1280x1024p75

	{1792, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1360,  768},	// 1360x768p60
	{2160, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1600, 1200},	// 1600x1200p60

	{1904, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1440,  900},	// 1440x900p60
	{1936, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1440,  900},	// 1440x900p75
	{2240, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_NEGATIVE_HSYNC|MDIN_POSITIVE_VSYNC|MDIN_SCANTYPE_PROG, 1680, 1050},	// 1680x1050p60	

	{1840, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG, 1680, 1050},	// 1680x1050pRB	
	{2080, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG, 1920, 1080},	// 1920x1080pRB	
	{2080, MDIN_POSITIVE_HACT|MDIN_POSITIVE_VACT|MDIN_POSITIVE_HSYNC|MDIN_NEGATIVE_VSYNC|MDIN_SCANTYPE_PROG, 1920, 1200}	// 1920x1200pRB	
};

#if defined(SYSTEM_USE_MDIN340)||defined(SYSTEM_USE_MDIN380)
// default value for hdmi-Video format
ROMDATA MDIN_HTXVIDEO_FRMT defMDINHTXVideo[]		= {
//		MDIN-format (wide(0)-4:3, wide(1)-4:3/16:9, wide(2)-16:9)
//	  id1 id2 sub wide|rep    pixel   freq  Htot  Vtot     x    y   Hact  Vact                 BT656
	{{  6,  7, 0, (1<<4)|1}, { 2700,  6000, 1716,  525}, {238,  18,  720,  240}, {3, 124, 3, 114,  15,  38,  4,  429}}, // 1440x480i@60(6,7)
	{{ 21, 22, 0, (0<<4)|1}, { 2700,  5000, 1728,  625}, {264,  22,  720,  288}, {3, 126, 3, 138,  19,  24,  2,  432}}, // 1440x576i@50(21,22)
	{{  2,  3, 0, (1<<4)|0}, { 2700,  6000,  858,  525}, {122,  36,  720,  480}, {0,  62, 6,  60,  30,  16,  9,    0}}, // 720x480p@60(2,3)
	{{ 17, 18, 0, (1<<4)|0}, { 2700,  5000,  864,  625}, {132,  44,  720,  576}, {0,  64, 5,  68,  39,  12,  5,    0}}, // 720x576p@50(17,18)
	{{  4,  0, 0, (2<<4)|0}, { 7417,  6000, 1650,  750}, {260,  25, 1280,  720}, {0,  40, 5, 220,  20, 110,  5,    0}}, // 1280x720p@60(4)
	{{ 19,  0, 0, (2<<4)|0}, { 7425,  5000, 1980,  750}, {260,  25, 1280,  720}, {0,  40, 5, 220,  20, 440,  5,    0}}, // 1280x720p@50(19)
	{{  5,  0, 0, (2<<4)|0}, { 7417,  6000, 2200, 1125}, {192,  20, 1920,  540}, {3,  44, 5, 148,  15,  88,  2, 1100}}, // 1920x1080i@60(5)
	{{ 20,  0, 0, (2<<4)|0}, { 7425,  5000, 2640, 1125}, {192,  20, 1920,  540}, {3,  44, 5, 148,  15, 528,  2, 1320}}, // 1920x1080i@50(20)
	{{ 16,  0, 0, (2<<4)|0}, {14850,  6000, 2200, 1125}, {192,  41, 1920, 1080}, {0,  44, 5, 148,  36,  88,  4,    0}}, // 1920x1080p@60(16)
	{{ 31,  0, 0, (2<<4)|0}, {14850,  5000, 2640, 1125}, {192,  41, 1920, 1080}, {0,  44, 5, 148,  36, 528,  4,    0}}, // 1920x1080p@50(31)

	{{  1,  0, 0, (0<<4)|0}, { 2517,  6000,  800,  525}, {144,  35,  640,  480}, {0,  96, 2,  48,  33,  16, 10,    0}}, // 640x480@60(1)
	{{ 66,  0, 0, (0<<4)|0}, { 3150,  7500,  840,  500}, {184,  19,  640,  480}, {0,  64, 3, 120,  16,  16,  1,    0}}, // 640x480@75
	{{ 69,  0, 0, (0<<4)|0}, { 4000,  6032, 1056,  628}, {216,  27,  800,  600}, {0, 128, 4,  88,  23,  40,  1,    0}}, // 800x600@60
	{{ 71,  0, 0, (0<<4)|0}, { 4950,  7500, 1056,  625}, {240,  24,  800,  600}, {0,  80, 3, 160,  21,  16,  1,    0}}, // 800x600@75
	{{ 73,  0, 0, (0<<4)|0}, { 6500,  6000, 1344,  806}, {296,  35, 1024,  768}, {0, 136, 6, 160,  29,  24,  3,    0}}, // 1024x768@60
	{{ 75,  0, 0, (0<<4)|0}, { 7875,  7503, 1312,  800}, {272,  31, 1024,  768}, {0,  96, 3, 176,  28,  16,  1,    0}}, // 1024x768@75
	{{ 85,  0, 0, (0<<4)|0}, {10800,  6002, 1688, 1066}, {360,  41, 1280, 1024}, {0, 112, 3, 248,  38,  48,  1,    0}}, // 1280x1024@60
	{{ 86,  0, 0, (0<<4)|0}, {13500,  7503, 1688, 1066}, {392,  41, 1280, 1024}, {0, 144, 3, 248,  38,  16,  1,    0}}, // 1280x1024@75

	{{ 88,  0, 0, (2<<4)|0}, { 8550,  6002, 1792,  795}, {368,  24, 1360,  768}, {0, 112, 6, 256,  18,  64,  3,    0}}, // 1360x768@60
	{{ 78,  0, 0, (0<<4)|0}, {16200,  6000, 2160, 1250}, {496,  49, 1600, 1200}, {0, 192, 3, 304,  46,  64,  1,    0}}, // 1600x1200@60

	{{108,  0, 0, (2<<4)|0}, {10650,  5989, 1904,  934}, {384,  31, 1440,  900}, {0, 152, 6, 232,  25,  80,  3,    0}}, // 1440x900@60
	{{109,  0, 0, (2<<4)|0}, {13675,  7498, 1936,  942}, {400,  39, 1440,  900}, {0, 152, 6, 248,  33,  96,  3,    0}}, // 1440x900@75
	{{110,  0, 0, (2<<4)|0}, {14625,  5995, 2240, 1089}, {456,  36, 1680, 1050}, {0, 176, 6, 280,  30, 104,  3,    0}}, // 1680x1050@60

	{{111,  0, 0, (2<<4)|0}, {11900,  5988, 1840, 1080}, {112,  27, 1680, 1050}, {0,  32, 6,  80,  21,  48,  3,    0}}, // 1680x1050@RB
	{{112,  0, 0, (2<<4)|0}, {13850,  5993, 2080, 1111}, {112,  28, 1920, 1080}, {0,  32, 5,  80,  23,  48,  3,    0}}, // 1920x1080@RB
	{{113,  0, 0, (2<<4)|0}, {15400,  5995, 2080, 1235}, {112,  32, 1920, 1200}, {0,  32, 6,  80,  26,  48,  3,    0}}, // 1920x1200@RB

//		Video-format (60Hz)
//	  id1 id2 sub wide|rep    pixel   freq  Htot  Vtot     x    y   Hact  Vact                 BT656
	{{  8,  9, 1, (1<<4)|1}, { 2700,  6000, 1716,  262}, {238,  18, 1440,  240}, {0, 124, 3, 114,  15,  38,  4,    0}}, // 8,9(1) 1440 x 240p
	{{  8,  9, 2, (1<<4)|1}, { 2700,  6000, 1716,  263}, {238,  18, 1440,  240}, {0, 124, 3, 114,  15,  38,  4,    0}}, // 8,9(2) 1440 x 240p
	{{ 10, 11, 0, (1<<4)|1}, { 5400,  6000, 3432,  525}, {476,  18, 2880,  480}, {0, 248, 3, 228,  15,  76,  4, 1716}}, // 10,11 2880 x 480p
	{{ 12, 13, 1, (1<<4)|1}, { 5400,  6000, 3432,  262}, {476,  18, 2880,  240}, {0, 248, 3, 228,  15,  76,  4,    0}}, // 12,13(1) 2280 x 240p
	{{ 12, 13, 2, (1<<4)|1}, { 5400,  6000, 3432,  263}, {476,  18, 2880,  240}, {0, 248, 3, 228,  15,  76,  4,    0}}, // 12,13(2) 2280 x 240p
	{{ 14, 15, 0, (1<<4)|0}, { 5400,  6000, 1716,  525}, {244,  36, 1440,  480}, {0, 124, 6, 120,  30,  32,  9,    0}}, // 14, 15 1140 x 480p

//		Video-format (50Hz)
//	  id1 id2 sub wide|rep    pixel   freq  Htot  Vtot     x    y   Hact  Vact                 BT656
	{{ 23, 24, 1, (1<<4)|1}, { 2700,  5000, 1728,  312}, {264,  22, 1440,  288}, {0, 126, 3, 138,  19,  24,  2,    0}}, // 23,24(1) 1440 x 288p
	{{ 23, 24, 2, (1<<4)|1}, { 2700,  5000, 1728,  313}, {264,  22, 1440,  288}, {0, 126, 3, 138,  19,  24,  2,    0}}, // 23,24(2) 1440 x 288p
	{{ 23, 24, 3, (1<<4)|1}, { 2700,  5000, 1728,  314}, {264,  22, 1440,  288}, {0, 126, 3, 138,  19,  24,  2,    0}}, // 23,24(3) 1440 x 288p
	{{ 25, 26, 0, (1<<4)|1}, { 5400,  5000, 3456,  625}, {528,  22, 2880,  576}, {0, 252, 3, 276,  19,  48,  2, 1728}}, // 25, 26 2880 x 576p
	{{ 27, 28, 1, (1<<4)|1}, { 5400,  5000, 3456,  312}, {528,  22, 2880,  288}, {0, 252, 3, 276,  19,  48,  2,    0}}, // 27,28(1) 2880 x 288p
	{{ 27, 28, 2, (1<<4)|1}, { 5400,  5000, 3456,  313}, {528,  22, 2880,  288}, {0, 252, 3, 276,  19,  48,  3,    0}}, // 27,28(2) 2880 x 288p
	{{ 27, 28, 3, (1<<4)|1}, { 5400,  5000, 3456,  314}, {528,  22, 2880,  288}, {0, 252, 3, 276,  19,  48,  4,    0}}, // 27,28(3) 2880 x 288p
	{{ 29, 30, 0, (1<<4)|0}, { 5400,  5000, 1728,  625}, {264,  44, 1440,  576}, {0, 128, 5, 136,  39,  24,  5,    0}}, // 29,30 1440 x 576p

//		Video-format (etcHz)
//	  id1 id2 sub wide|rep    pixel   freq  Htot  Vtot     x    y   Hact  Vact                 BT656
	{{ 32,  0, 0, (2<<4)|0}, { 7417,  2400, 2750, 1125}, {192,  41, 1920, 1080}, {0,  44, 5, 148,  36, 638,  4,    0}}, // 32(2) 1920 x 1080p
	{{ 33,  0, 0, (2<<4)|0}, { 7425,  2500, 2640, 1125}, {192,  41, 1920, 1080}, {0,  44, 5, 148,  36, 528,  4,    0}}, // 33(3) 1920 x 1080p
	{{ 34,  0, 0, (2<<4)|0}, { 7417,  3000, 2200, 1125}, {192,  41, 1920, 1080}, {0,  44, 5, 148,  36, 528,  4,    0}}, // 34(4) 1920 x 1080p
	{{ 35, 36, 0, (1<<4)|0}, {10800,  5994, 3432,  525}, {488,  36, 2880,  480}, {0, 248, 6, 240,  30,  64, 10,    0}}, // 35, 36 2880 x 480p@59.94/60Hz
	{{ 37, 38, 0, (1<<4)|0}, {10800,  5000, 3456,  625}, {528,  44, 2880,  576}, {0, 256, 5, 272,  40,  48,  5,    0}}, // 37, 38 2880 x 576p@50Hz
	{{ 39,  0, 0, (2<<4)|0}, { 7200,  5000, 2304, 1250}, {352,  62, 1920, 1080}, {0, 168, 5, 184,  87,  32, 24,    0}}, // 39 1920 x 1080i@50Hz
	{{ 40,  0, 0, (2<<4)|0}, {14850, 10000, 2640, 1125}, {192,  20, 1920, 1080}, {0,  44, 5, 148,  15, 528,  2, 1320}}, // 40 1920 x 1080i@100Hz
	{{ 41,  0, 0, (2<<4)|0}, {14850, 10000, 1980,  750}, {260,  25, 1280,  720}, {0,  40, 5, 220,  20, 400,  5,    0}}, // 41 1280 x 720p@100Hz
	{{ 42, 43, 0, (1<<4)|0}, { 5400, 10000,  864,  144}, {132,  44,  720,  576}, {0,  64, 5,  68,  39,  12,  5,    0}}, // 42, 43, 720 x 576p@100Hz
	{{ 44, 45, 0, (1<<4)|1}, { 5400, 10000,  864,  625}, {264,  22, 1440,  576}, {0,  63, 3,  69,  19,  12,  2,  432}}, // 44, 45, 720 x 576i@100Hz, pix repl
	{{ 46,  0, 0, (2<<4)|0}, {14835, 11988, 2200, 1125}, {192,  20, 1920, 1080}, {0,  44, 5, 149,  15,  88,  2, 1100}}, // 46, 1920 x 1080i@119.88/120Hz
	{{ 47,  0, 0, (2<<4)|0}, {14835, 11988, 1650,  750}, {260,  25, 1280,  720}, {0,  40, 5, 220,  20, 110,  5, 1100}}, // 47, 1280 x 720p@119.88/120Hz
	{{ 48, 49, 0, (1<<4)|0}, { 5400, 11988,  858,  525}, {122,  36,  720,  480}, {0,  62, 6,  60,  30,  16, 10,    0}}, // 48, 49 720 x 480p@119.88/120Hz
	{{ 50, 51, 0, (1<<4)|1}, { 5400, 11988,  858,  525}, {119,  18,  720,  480}, {0,  62, 3,  57,  15,  19,  4,  491}}, // 50, 51 720 x 480i@119.88/120Hz
	{{ 52, 53, 0, (1<<4)|0}, {10800, 20000,  864,  625}, {132,  44,  720,  576}, {0,  64, 5,  68,  39,  12,  5,    0}}, // 52, 53, 720 x 576p@200Hz
	{{ 54, 55, 0, (1<<4)|1}, {10800, 20000,  864,  625}, {132,  22,  720,  576}, {0,  63, 3,  69,  19,  12,  2,  432}}, // 54, 55, 1440 x 720i @200Hz, pix repl
	{{ 56, 57, 0, (1<<4)|0}, {10800, 24000,  858,  525}, {122,  36,  720,  480}, {0,  62, 6,  60,  30,  16,  9,    0}}, // 56, 57, 720 x 480p @239.76/240Hz
	{{ 58, 59, 0, (1<<4)|1}, {10800, 24000,  858,  525}, {119,  18,  720,  480}, {0,  62, 3,  57,  15,  19,  4,  429}}, // 58, 59, 1440 x 480i @239.76/240Hz, pix repl

//		PC-format
//	  id1 id2 sub wide|rep    pixel   freq  Htot  Vtot     x    y   Hact  Vact                 BT656
	{{ 60,  0, 0, (2<<4)|0}, { 3150,  8508,  832,  445}, {160,  63,  640,  350}, {0,  64, 3,  96,  60,  32, 32,    0}}, // 640x350@85.08
	{{ 61,  0, 0, (2<<4)|0}, { 3150,  8508,  832,  445}, {160,  44,  640,  400}, {0,  64, 3,  96,  41,  32,  1,    0}}, // 640x400@85.08
	{{ 62,  0, 0, (2<<4)|0}, { 2700,  7008,  900,  449}, {  0,   0,  720,  400}, {0,   0, 0,   0,   0,   0,  0,    0}}, // 720x400@70.08
	{{ 63,  0, 0, (2<<4)|0}, { 3500,  8504,  936,  446}, { 20,  45,  720,  400}, {0,  72, 3, 108,  42,  36,  1,    0}}, // 720x400@85.04
	{{ 64,  0, 0, (0<<4)|0}, { 2517,  5994,  800,  525}, {144,  35,  640,  480}, {0,  96, 2,  48,  33,  16, 10,    0}}, // 640x480@59.94
	{{ 65,  0, 0, (0<<4)|0}, { 3150,  7281,  832,  520}, {144,  31,  640,  480}, {0,  40, 3, 128,  28, 128,  9,    0}}, // 640x480@72.80
	{{ 67,  0, 0, (0<<4)|0}, { 3600,  8500,  832,  509}, {168,  28,  640,  480}, {0,  56, 3, 128,  25,  24,  9,    0}}, // 640x480@85.00
	{{ 68,  0, 0, (0<<4)|0}, { 3600,  5625, 1024,  625}, {200,  24,  800,  600}, {0,  72, 2, 128,  22,  24,  1,    0}}, // 800x600@56.25
	{{ 70,  0, 0, (0<<4)|0}, { 5000,  7219, 1040,  666}, {184,  29,  800,  600}, {0, 120, 6,  64,  23,  56, 37,    0}}, // 800x600@72.19
	{{ 72,  0, 0, (0<<4)|0}, { 5625,  8506, 1048,  631}, {216,  30,  800,  600}, {0,  64, 3, 152,  27,  32,  1,    0}}, // 800x600@85.06
	{{ 74,  0, 0, (0<<4)|0}, { 7500,  7007, 1328,  806}, {280,  35, 1024,  768}, {0, 136, 6, 144,  19,  24,  3,    0}}, // 1024x768@70.07
	{{ 76,  0, 0, (0<<4)|0}, { 9450,  8500, 1376,  808}, {304,  39, 1024,  768}, {0,  96, 3, 208,  36,  48,  1,    0}}, // 1024x768@85
	{{ 77,  0, 0, (0<<4)|0}, {10800,  7500, 1600,  900}, {384,  35, 1152,  864}, {0, 128, 3, 256,  32,  64,  1,    0}}, // 1152x864@75
	{{ 79,  0, 0, (2<<4)|0}, { 6825,  6000, 1440,  790}, {112,  19, 1280,  768}, {0,  32, 7,  80,  12,  48,  3,    0}}, // 1280x768@59.95
	{{ 80,  0, 0, (2<<4)|0}, { 7950,  5987, 1664,  798}, {320,  27, 1280,  768}, {0, 128, 7, 192,  20,  64,  3,    0}}, // 1280x768@59.87
	{{ 81,  0, 0, (2<<4)|0}, {10220,  6029, 1696,  805}, {320,  27, 1280,  768}, {0, 128, 7, 208,  27,  80,  3,    0}}, // 1280x768@74.89
	{{ 82,  0, 0, (2<<4)|0}, {11750,  8484, 1712,  809}, {352,  38, 1280,  768}, {0, 136, 7, 216,  31,  80,  3,    0}}, // 1280x768@85
	{{ 83,  0, 0, (0<<4)|0}, {10800,  6000, 1800, 1000}, {424,  39, 1280,  960}, {0, 112, 3, 312,  36,  96,  1,    0}}, // 1280x960@60
	{{ 84,  0, 0, (0<<4)|0}, {14850,  8500, 1728, 1011}, {384,  50, 1280,  960}, {0, 160, 3, 224,  47,  64,  1,    0}}, // 1280x960@85
	{{ 87,  0, 0, (0<<4)|0}, {15750,  8502, 1728, 1072}, {384,  47, 1280, 1024}, {0, 160, 3, 224,   4,  64,  1,    0}}, // 1280x1024@85
	{{ 89,  0, 0, (0<<4)|0}, {10100,  5995, 1560, 1080}, {112,  27, 1400, 1050}, {0,  32, 4,  80,  23,  48,  3,    0}}, // 1400x105@59.95
	{{ 90,  0, 0, (0<<4)|0}, {12175,  5998, 1864, 1089}, {376,  36, 1400, 1050}, {0, 144, 4, 232,  32,  88,  3,    0}}, // 1400x105@59.98
	{{ 91,  0, 0, (0<<4)|0}, {15600,  7487, 1896, 1099}, {392,  46, 1400, 1050}, {0, 144, 4, 248,  22, 104,  3,    0}}, // 1400x105@74.87
	{{ 92,  0, 0, (0<<4)|0}, {17950,  8496, 1912, 1105}, {408,  52, 1400, 1050}, {0, 152, 4, 256,  48, 104,  3,    0}}, // 1400x105@84.96
	{{ 93,  0, 0, (0<<4)|0}, {17550,  6500, 2160, 1250}, {496,  49, 1600, 1200}, {0, 192, 3, 304,  46,  64,  1,    0}}, // 1600x1200@65
	{{ 94,  0, 0, (0<<4)|0}, {18900,  7000, 2160, 1250}, {496,  49, 1600, 1200}, {0, 192, 3, 304,  46,  64,  1,    0}}, // 1600x1200@70
	{{ 95,  0, 0, (0<<4)|0}, {20250,  7500, 2160, 1250}, {496,  49, 1600, 1200}, {0, 192, 3, 304,  46,  64,  1,    0}}, // 1600x1200@75
	{{ 96,  0, 0, (0<<4)|0}, {22950,  8500, 2160, 1250}, {496,  49, 1600, 1200}, {0, 192, 3, 304,  46,  64,  1,    0}}, // 1600x1200@85
	{{ 97,  0, 0, (0<<4)|0}, {20475,  6000, 2448, 1394}, {528,  49, 1792, 1344}, {0, 200, 3, 328,  46, 128,  1,    0}}, // 1792x1344@60
	{{ 98,  0, 0, (0<<4)|0}, {26100,  7500, 2456, 1417}, {568,  72, 1792, 1344}, {0, 216, 3, 352,  69,  96,  1,    0}}, // 1792x1344@74.997
	{{ 99,  0, 0, (0<<4)|0}, {21825,  6000, 2528, 1439}, {576,  46, 1856, 1392}, {0, 224, 3, 352,  43,  96,  1,    0}}, // 1856x1392@60
	{{100,  0, 0, (0<<4)|0}, {28800,  7500, 2560, 1500}, {576, 107, 1856, 1392}, {0, 224, 3, 352, 104, 128,  1,    0}}, // 1856x1392@75
	{{101,  0, 0, (2<<4)|0}, {15400,  5995, 2080, 1235}, {112,  32, 1920, 1200}, {0,  32, 6,  80,  26,  48,  3,    0}}, // 1920x1200@59.95
	{{102,  0, 0, (2<<4)|0}, {19325,  5988, 2592, 1245}, {536,  42, 1920, 1200}, {0, 200, 6, 336,  36, 136,  3,    0}}, // 1920x1200@59.88
	{{103,  0, 0, (2<<4)|0}, {24525,  7493, 2608, 1255}, {552,  52, 1920, 1200}, {0, 208, 6, 344,  46, 136,  3,    0}}, // 1920x1200@74.93
	{{104,  0, 0, (2<<4)|0}, {28125,  8493, 2624, 1262}, {560,  59, 1920, 1200}, {0, 208, 6, 352,  53, 144,  3,    0}}, // 1920x1200@84.93
	{{105,  0, 0, (0<<4)|0}, {23400,  6000, 2600, 1500}, {552,  59, 1920, 1440}, {0, 208, 3, 344,  56, 128,  1,    0}}, // 1920x1440@60
	{{106,  0, 0, (0<<4)|0}, {29700,  7500, 2640, 1500}, {576,  59, 1920, 1440}, {0, 224, 3, 352,  56, 144,  1,    0}}, // 1920x1440@75
	{{107,  0, 0, (0<<4)|0}, {29700,  7500, 2640, 1500}, {576,  59, 1920, 1440}, {0, 224, 3, 352,  56, 144,  1,    0}}, // 1920x1440@75
};
#endif	/* #if defined(SYSTEM_USE_MDIN340)||defined(SYSTEM_USE_MDIN380) */

// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------

/*  FILE_END_HERE */
