//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2010  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:	APP340.C
// Description 		:
// Ref. Docment		: 
// Revision History 	:

//--------------------------------------------------------------------------------------------------------------------------
// You must make drive functions for I2C read & I2C write.
//--------------------------------------------------------------------------------------------------------------------------
// static BYTE MDINI2C_Write(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes)
// static BYTE MDINI2C_Read(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes)

//--------------------------------------------------------------------------------------------------------------------------
// You must make drive functions for delay (usec and msec).
//--------------------------------------------------------------------------------------------------------------------------
// MDIN_ERROR_t MDINDLY_10uSec(WORD delay) -- 10usec unit
// MDIN_ERROR_t MDINDLY_mSec(WORD delay) -- 1msec unit

//--------------------------------------------------------------------------------------------------------------------------
// You must make drive functions for debug (ex. printf).
//--------------------------------------------------------------------------------------------------------------------------
// void UARTprintf(const char *pcString, ...)

// ----------------------------------------------------------------------
// Include files
// ----------------------------------------------------------------------
#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h>
#ifndef		__MDIN3xx_H__
#include	 "mdin3xx.h"
#endif


// ----------------------------------------------------------------------
// Static Global Data section variables
// ----------------------------------------------------------------------
#if 1 /* Harry */
MDIN_VIDEO_INFO		stVideo;
#else
static MDIN_VIDEO_INFO		stVideo;
#endif
static MDIN_INTER_WINDOW	stInterWND;

// ----------------------------------------------------------------------
// External Variable 
// ----------------------------------------------------------------------
int video_output_type = VIDOUT_1920x1080p60;
int video_input_vidsrc = VIDSRC_1920x1080i60;
int video_vidsrc_mode = MDIN_SRC_EMB422_8; //MDIN_SRC_MUX656_8

// ----------------------------------------------------------------------
// Static Prototype Functions
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static functions
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Exported functions
// ----------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------

/* There MUST be only one IN_DATA_MAP_x defined. User should set these flags based on board layout. */
#define IN_DATA_MAP_0           1 //GM board
#define IN_DATA_MAP_3           0

static void CreateMDIN340VideoInstance(void)
{
	WORD nID = 0;

	while (nID!=0x85) {
	    MDIN3xx_GetChipID(&nID);	// get chip-id
	    printk("nID : 0x%x \n", nID);
	}
	
	MDIN3xx_EnableMainDisplay(OFF);		// set main display off
	MDIN3xx_SetMemoryConfig();			// initialize DDR memory

	MDIN3xx_SetVCLKPLLSource(MDIN_PLL_SOURCE_XTAL);	// set PLL source
	MDIN3xx_EnableClockDrive(MDIN_CLK_DRV_ALL, ON);
#if IN_DATA_MAP_0 /* Harry */
    MDIN3xx_SetInDataMapMode(MDIN_IN_DATA24_MAP0);	// set in-data map
#endif
#if IN_DATA_MAP_3
	MDIN3xx_SetInDataMapMode(MDIN_IN_DATA24_MAP3);	// set in-data map
#endif

	// setup enhancement
	MDIN3xx_SetFrontNRFilterCoef(NULL);		// set default frontNR filter coef
	MDINAUX_SetFrontNRFilterCoef(NULL);		// set default frontNR filter coef
	MDIN3xx_SetPeakingFilterCoef(NULL);		// set default peaking filter coef
	MDIN3xx_SetColorEnFilterCoef(NULL);		// set default color enhancer coef
	MDIN3xx_SetBlockNRFilterCoef(NULL);		// set default blockNR filter coef
	MDIN3xx_SetMosquitFilterCoef(NULL);		// set default mosquit filter coef
	MDIN3xx_SetSkinTonFilterCoef(NULL);		// set default skinton filter coef

	MDIN3xx_EnableLTI(OFF);					// set LTI off
	MDIN3xx_EnableCTI(OFF);					// set CTI off
	MDIN3xx_SetPeakingFilterLevel(0);		// set peaking gain
	MDIN3xx_EnablePeakingFilter(ON);		// set peaking on

	MDIN3xx_EnableFrontNRFilter(OFF);		// set frontNR off
	MDIN3xx_EnableBWExtension(OFF);			// set B/W extension off

	MDIN3xx_SetIPCBlock();		// initialize IPC block (3DNR gain is 37)

	memset((PBYTE)&stVideo, 0, sizeof(MDIN_VIDEO_INFO));

	MDIN3xx_SetMFCHYFilterCoef(&stVideo, NULL);	// set default MFC filters
	MDIN3xx_SetMFCHCFilterCoef(&stVideo, NULL);
	MDIN3xx_SetMFCVYFilterCoef(&stVideo, NULL);
	MDIN3xx_SetMFCVCFilterCoef(&stVideo, NULL);

	// set aux display OFF
	stVideo.dspFLAG = MDIN_AUX_DISPLAY_OFF | MDIN_AUX_FREEZE_ON;

	// set video path (main/aux/dac/enc)
	stVideo.srcPATH = PATH_MAIN_A_AUX_B;	// set main is A, aux is B, input port is A
	stVideo.dacPATH = DAC_PATH_MAIN_OUT;	// set main only

	// if you need to front format conversion then set size.
//	stVideo.ffcH_m  = 1440;

	// define video format of PORTA-INPUT

#if 1
    stVideo.stSRC_a.frmt = video_input_vidsrc; //VIDSRC_1920x1080i60;
    stVideo.stSRC_a.mode = video_vidsrc_mode; //MDIN_SRC_EMB422_8;
#else	
	stVideo.stSRC_a.frmt = VIDSRC_720x480i60;
	stVideo.stSRC_a.mode = MDIN_SRC_MUX656_8;
#endif	
	stVideo.stSRC_a.fine = MDIN_FIELDID_INPUT | MDIN_LOW_IS_TOPFLD;

	// define video format of MAIN-OUTPUT
#if 1
	stVideo.stOUT_m.frmt = VIDOUT_1920x1080p60;
	stVideo.stOUT_m.mode = MDIN_OUT_RGB444_8;
#else
    stVideo.stOUT_m.frmt = VIDOUT_1024x768p60;
	stVideo.stOUT_m.mode = MDIN_OUT_RGB444_8;   //VGA monitor
#endif	
	stVideo.stOUT_m.fine = MDIN_SYNC_FREERUN;	// set main outsync free-run

	stVideo.stOUT_m.brightness = 128;			// set main picture factor
	stVideo.stOUT_m.contrast = 128;
	stVideo.stOUT_m.saturation = 128;
	stVideo.stOUT_m.hue = 128;

#if RGB_GAIN_OFFSET_TUNE == 1
	stVideo.stOUT_m.r_gain = 128;				// set main gain/offset
	stVideo.stOUT_m.g_gain = 128;
	stVideo.stOUT_m.b_gain = 128;
	stVideo.stOUT_m.r_offset = 128;
	stVideo.stOUT_m.g_offset = 128;
	stVideo.stOUT_m.b_offset = 128;
#endif

	// define video format of IPC-block
	stVideo.stIPC_m.mode = MDIN_DEINT_ADAPTIVE;
	stVideo.stIPC_m.film = MDIN_DEINT_FILM_ALL;
	stVideo.stIPC_m.gain = 37;
	stVideo.stIPC_m.fine = MDIN_DEINT_3DNR_ON | MDIN_DEINT_CCS_ON;

	// define map of frame buffer
	stVideo.stMAP_m.frmt = MDIN_MAP_AUX_ON_NR_ON;	// when MDIN_DEINT_3DNR_ON
//	stVideo.stMAP_m.frmt = MDIN_MAP_AUX_ON_NR_OFF;	// when MDIN_DEINT_3DNR_OFF

	// define video format of PORTB-INPUT

#if 1
    stVideo.stSRC_b.frmt = video_input_vidsrc; //VIDSRC_1920x1080i60;
    stVideo.stSRC_b.mode = video_vidsrc_mode; //MDIN_SRC_EMB422_8;
#else	
	stVideo.stSRC_b.frmt = VIDSRC_720x480i60;
	stVideo.stSRC_b.mode = MDIN_SRC_MUX656_8;
#endif	
	stVideo.stSRC_b.fine = MDIN_FIELDID_INPUT | MDIN_LOW_IS_TOPFLD;

	// define video format of AUX-OUTPUT
#if 1
	stVideo.stOUT_x.frmt = VIDOUT_1920x1080p60;
#else
    stVideo.stOUT_x.frmt = VIDOUT_1024x768p60;
#endif	
	stVideo.stOUT_x.mode = MDIN_OUT_RGB444_8;
	stVideo.stOUT_x.fine = MDIN_SYNC_FREERUN;	// set aux outsync free-run

	stVideo.stOUT_x.brightness = 128;			// set aux picture factor
	stVideo.stOUT_x.contrast = 128;
	stVideo.stOUT_x.saturation = 128;
	stVideo.stOUT_x.hue = 128;

#if RGB_GAIN_OFFSET_TUNE == 1
	stVideo.stOUT_x.r_gain = 128;				// set aux gain/offset
	stVideo.stOUT_x.g_gain = 128;
	stVideo.stOUT_x.b_gain = 128;
	stVideo.stOUT_x.r_offset = 128;
	stVideo.stOUT_x.g_offset = 128;
	stVideo.stOUT_x.b_offset = 128;
#endif

	// define video format of HDMI-OUTPUT
	stVideo.stVID_h.mode  = HDMI_OUT_RGB444_8;
	stVideo.stVID_h.fine  = HDMI_CLK_EDGE_RISE;

	stVideo.stAUD_h.frmt  = AUDIO_INPUT_I2S_0;						// audio input format
	stVideo.stAUD_h.freq  = AUDIO_MCLK_256Fs | AUDIO_FREQ_48kHz;	// sampling frequency
#if 1 /* Harry, change audio to 16 bits. It only provides 16/20/24 bits */	
    stVideo.stAUD_h.fine  = AUDIO_MAX20B_MINUS4 | AUDIO_SD_JUST_LEFT | AUDIO_WS_POLAR_HIGH |
							AUDIO_SCK_EDGE_RISE | AUDIO_SD_MSB_FIRST | AUDIO_SD_1ST_SHIFT;
#else
	stVideo.stAUD_h.fine  = AUDIO_MAX24B_MINUS0 | AUDIO_SD_JUST_LEFT | AUDIO_WS_POLAR_HIGH |
							AUDIO_SCK_EDGE_RISE | AUDIO_SD_MSB_FIRST | AUDIO_SD_1ST_SHIFT;
#endif							
	MDINHTX_SetHDMIBlock(&stVideo);		// initialize HDMI block

	// define window for inter-area
	stInterWND.lx = 315;	stInterWND.rx = 405;
	stInterWND.ly = 90;		stInterWND.ry = 150;
	MDIN3xx_SetDeintInterWND(&stInterWND, MDIN_INTER_BLOCK0);
	MDIN3xx_EnableDeintInterWND(MDIN_INTER_BLOCK0, OFF);

	stVideo.exeFLAG = MDIN_UPDATE_MAINFMT;	// execution of video process
}

//--------------------------------------------------------------------------------------------------------------------------
void main(void)
{
	CreateMDIN340VideoInstance();				// initialize MDIN-340

	while (TRUE) {
		if (stVideo.exeFLAG) {					// check change video formats
			MDIN3xx_EnableMainDisplay(OFF);
			MDIN3xx_VideoProcess(&stVideo);		// mdin3xx main video process
			MDIN3xx_EnableMainDisplay(ON);
		}
		MDINHTX_CtrlHandler(&stVideo);			// hdmi-tx control handler
	}
}

//-----------GM porting ----------------------------------------------------------------------------
#include <linux/kthread.h>
int mdin340_is_runing = 0;

/* Main thread
 */
int mdin340_working_thread(void *private)
{
    if (private)    {}
    
    CreateMDIN340VideoInstance();				// initialize MDIN-340. in_data_map_mode MDIN_IN_DATA24_MAP0
    mdin340_is_runing = 1;
    stVideo.exeFLAG = 1;    //force update
    
    while (!kthread_should_stop()) {
        
        if (stVideo.stOUT_m.frmt != video_output_type) {
            stVideo.stOUT_m.frmt = video_output_type;
            stVideo.stOUT_x.frmt = video_output_type;
            stVideo.exeFLAG = 1;
            
        }

        if (stVideo.stSRC_a.frmt != video_input_vidsrc) {
            stVideo.stSRC_a.frmt = video_input_vidsrc;
            stVideo.stSRC_b.frmt = video_input_vidsrc;
            stVideo.stSRC_b.mode = video_vidsrc_mode;
            stVideo.exeFLAG = 1;
            
        }
        
		if (stVideo.exeFLAG) {					// check change video formats
			MDIN3xx_EnableMainDisplay(OFF);
			MDIN3xx_VideoProcess(&stVideo);		// mdin3xx main video process
			MDIN3xx_EnableMainDisplay(ON);
		}
		MDINHTX_CtrlHandler(&stVideo);			// hdmi-tx control handler
	}
	
	mdin340_is_runing = 0;
	return 0;
}

/*  FILE_END_HERE */
