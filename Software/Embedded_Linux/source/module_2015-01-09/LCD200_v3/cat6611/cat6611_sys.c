///*****************************************
//  Copyright (C) 2009-2014
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   >cat6611_sys.c<
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2009/08/24
//   @fileversion: CAT6611_SAMPLEINTERFACE_1.12
//******************************************/

///////////////////////////////////////////////////////////////////////////////
// This is the sample program for CAT6611 driver usage.
///////////////////////////////////////////////////////////////////////////////

#include "hdmitx.h"

#ifdef SUPPORT_SYNCEMB
int  support_syncemb = 1;
#else
int  support_syncemb = 0;
#endif

#ifdef SUPPORT_DEGEN
int support_degen = 1;
#else
int support_degen = 0;
#endif

////////////////////////////////////////////////////////////////////////////////
// input and default output color setting.
// to indicating this selection for color mode.
////////////////////////////////////////////////////////////////////////////////
#define HDMITX_INPUT_COLORMODE                F_MODE_RGB444
// #define HDMITX_INPUT_COLORMODE             F_MODE_YUV444 
//#define HDMITX_INPUT_COLORMODE             F_MODE_YUV422

#define HDMITX_INPUT_SIGNAL_TYPE 0  // for default
//#define HDMITX_INPUT_SIGNAL_TYPE T_MODE_SYNCEMB                    // for 16bit sync embedded, color is YUV422 only
//#define HDMITX_INPUT_SIGNAL_TYPE (T_MODE_SYNCEMB | T_MODE_CCIR656) // for 8bit sync embedded, color is YUV422 only
//#define HDMITX_INPUT_SIGNAL_TYPE T_MODE_INDDR                      // for DDR input
//#define HDMITX_INPUT_SIGNAL_TYPE T_MODE_DEGEN                      // for DE generating by input sync.
//#define HDMITX_INPUT_SIGNAL_TTYP (T_MODE_DEGEN| T_MODE_SYNCGEN)    // for DE and sync regenerating by input sync.

_IDATA BYTE bInputColorMode  = HDMITX_INPUT_COLORMODE ;
_IDATA BYTE bInputSignalType = HDMITX_INPUT_SIGNAL_TYPE ; 
_IDATA BYTE bOutputColorMode = F_MODE_RGB444 ; // default

////////////////////////////////////////////////////////////////////////////////
// Set Audio parameter for your product.
////////////////////////////////////////////////////////////////////////////////

// define the default input audio sample rate. If it is variance, need to modify in EnableAudioOutput calling.
#define INPUT_AUDIO_SAMPLE_FREQ                     48000
//#define INPUT_AUDIO_SAMPLE_FREQ                     32000
//#define INPUT_AUDIO_SAMPLE_FREQ                     44100
//#define INPUT_AUDIO_SAMPLE_FREQ                     96000
//#define INPUT_AUDIO_SAMPLE_FREQ                     192000

// define the default input audio channel. If it is variance, need to modify in EnableAudioOutput calling.
#define INPUT_AUDIO_CHANNEL                         2
// #define INPUT_AUDIO_CHANNEL                         6 // 6611 only, for 5.1 channel.
// #define INPUT_AUDIO_CHANNEL                         8 // 6611 only, for 7.1 channel.

// define the default input audio interface. If it is variance, need to modify in EnableAudioOutput calling.
#define INPUT_SPDIF_ENABLE                          FALSE // for I2S
// #define INPUT_SPDIF_ENABLE                       TRUE // for I2S

////////////////////////////////////////////////////////////////////////////////
// EDID
////////////////////////////////////////////////////////////////////////////////
static _XDATA unsigned char EDID_Buf[128] ;
static RX_CAP _XDATA RxCapability ;
static BOOL bChangeMode = FALSE ;
_XDATA AVI_InfoFrame AviInfo;
_XDATA Audio_InfoFrame AudioInfo ;

////////////////////////////////////////////////////////////////////////////////
// Program utility.
////////////////////////////////////////////////////////////////////////////////
BOOL ParseEDID(void) ;
static BOOL ParseCEAEDID(BYTE *pCEAEDID) ;
void ConfigAVIInfoFrame(void) ;
void ConfigAudioInfoFrm(void) ;

_IDATA BYTE iVideoModeSelect=0 ;

_XDATA ULONG VideoPixelClock ; 
_XDATA BYTE VIC ; // 480p60
_XDATA BYTE pixelrep ; // no pixelrepeating
_XDATA HDMI_Aspec aspec ;
_XDATA HDMI_Colorimetry Colorimetry ;

BOOL bHDMIMode, bAudioEnable ;
#ifdef SUPPORT_DEGEN
MODE_ID ModeId ;
#endif


////////////////////////////////////////////////////////////////////////////////
// Function Body.
////////////////////////////////////////////////////////////////////////////////

void HDMITX_ChangeDisplayOption(HDMI_Video_Type VideoMode, HDMI_OutputColorMode OutputColorMode) ;
void HDMITX_SetOutput(void) ;
void HDMITX_DevLoopProc(void) ;
void HDMITX_TrunOnHDCP(void) ;
void HDMITX_TrunOffHDCP(void) ;

void
HDMITX_SetOutput(void)
{
    
    // if bInputSignalType is not fixed but changed by input mode or other condition, 
    // set here.
    
    // bInputSignalType = ... ;

#ifdef HARRY_CHANGE
    /* Note: if SUPPORT_SYNCEMB is defined, we don't need to have DE generation
     * Only in RGB mode, we just need to do DE generation due to lack of DE from GM LCD controller
     */
    if (support_syncemb)
    {
        #ifdef SUPPORT_SYNCEMB
        ProgramSyncEmbeddedVideoMode(VIC, bInputSignalType) ; // inf CCIR656 input
        #endif
    }
    else
    {
        if (support_degen)
        {
            #ifdef SUPPORT_DEGEN
            ProgramDEGenModeByID( ModeId, bInputSignalType);
            #endif
        }
    }
#else   
    
    #ifdef SUPPORT_SYNCEMB
    ProgramSyncEmbeddedVideoMode(VIC, bInputSignalType) ; // inf CCIR656 input
    #endif

    #ifdef SUPPORT_DEGEN
    ProgramDEGenModeByID( ModeId, bInputSignalType) ;
    #endif
#endif

    EnableVideoOutput(VideoPixelClock>80000000,bInputColorMode, bInputSignalType, bOutputColorMode,bHDMIMode) ;
    
    if( bHDMIMode )
    {
        ConfigAVIInfoFrame() ;
        DelayMS(100) ;   
#ifdef SUPPORT_HDCP                 
        EnableHDCP(TRUE) ;
#endif        
		if( bAudioEnable )
		{
		    SetNonPCMAudio(0) ; // For LPCM audio. If AC3 or other compressed audio, please set the parameter as 1
            EnableAudioOutput(VideoPixelClock*(pixelrep+1),INPUT_AUDIO_SAMPLE_FREQ, INPUT_AUDIO_CHANNEL, INPUT_SPDIF_ENABLE);
            ConfigAudioInfoFrm() ;
		}
    }
    SetAVMute(FALSE) ;
    bChangeMode = FALSE ;    
}

void
HDMITX_DevLoopProc(void)
{
    BYTE HPD, HPDChange ;

    CheckHDMITX(&HPD,&HPDChange) ;
    
    //printk("HPD = %x, HPDChange = %x \n", HPD, HPDChange);
    
    if( HPDChange )
    {
        if( HPD )
        {
            ParseEDID() ;
            bAudioEnable = FALSE ; // default disable audio 
            bOutputColorMode &= ~F_MODE_CLRMOD_MASK ; // reset the output color mode into RGB
			if( RxCapability.ValidHDMI )
			{
				bHDMIMode = TRUE ;

#if 0				
				if(RxCapability.VideoMode & (1<<6))
#endif				
				{
					bAudioEnable = TRUE ;
				}
				
				if( RxCapability.VideoMode & (1<<5))
				{
					bOutputColorMode &= ~F_MODE_CLRMOD_MASK ;
					bOutputColorMode |= F_MODE_YUV444;
				}
				else if (RxCapability.VideoMode & (1<<4))
				{
					bOutputColorMode &= ~F_MODE_CLRMOD_MASK ;
					bOutputColorMode |= F_MODE_YUV422 ;
				}
			}
			else
			{
				bHDMIMode = FALSE ;
				bAudioEnable = FALSE ;
			}

            HDMITX_SetOutput() ;
            
        }
        else
        {
            // unplug mode, ...
            DisableAudioOutput() ;
            DisableVideoOutput() ;
        }
    }
    else // no stable but need to process mode change procedure
    {
        if(bChangeMode && HPD)
        {
            HDMITX_SetOutput() ;
        }
    }
}


void
HDMITX_ChangeDisplayOption(HDMI_Video_Type OutputVideoTiming, HDMI_OutputColorMode OutputColorMode)
{
   //HDMI_Video_Type  t=HDMI_480i60_16x9;
    switch(OutputVideoTiming)
	{
    case HDMI_640x480p60:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_640x480p60;
    #endif //SUPPORT_DEGEN

        VIC = 1 ;
        VideoPixelClock = 25000000 ;
        pixelrep = 0 ;
        aspec = HDMI_4x3 ;
        Colorimetry = HDMI_ITU601 ;
        break ;
    case HDMI_480p60:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_720x480p60;
    #endif //SUPPORT_DEGEN

        VIC = 2 ;
        VideoPixelClock = 27000000 ;
        pixelrep = 0 ;
        aspec = HDMI_4x3 ;
        Colorimetry = HDMI_ITU601 ;
        break ;
    case HDMI_480p60_16x9:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_720x480p60;
    #endif //SUPPORT_DEGEN

        VIC = 3 ;
        VideoPixelClock = 27000000 ;
        pixelrep = 0 ;
        aspec = HDMI_16x9 ;
        Colorimetry = HDMI_ITU601 ;
        break ;
    case HDMI_720p60:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_1280x720p60;
    #endif //SUPPORT_DEGEN

        VIC = 4 ;
        VideoPixelClock = 74250000 ;
        pixelrep = 0 ;
        aspec = HDMI_16x9 ;
        Colorimetry = HDMI_ITU709 ;
        break ;
    case HDMI_1080i60:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_1920x1080i60;
    #endif //SUPPORT_DEGEN

        VIC = 5 ;
        VideoPixelClock = 74250000 ;
        pixelrep = 0 ;
        aspec = HDMI_16x9 ;
        Colorimetry = HDMI_ITU709 ;
        break ;
    case HDMI_480i60:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_720x480i60;
    #endif //SUPPORT_DEGEN

        VIC = 6 ;
        VideoPixelClock = 13500000 ;
        pixelrep = 1 ;
        aspec = HDMI_4x3 ;
        Colorimetry = HDMI_ITU601 ;
        break ;
    case HDMI_480i60_16x9:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_720x480i60;
    #endif //SUPPORT_DEGEN

        VIC = 7 ;
        VideoPixelClock = 13500000 ;
        pixelrep = 1 ;
        aspec = HDMI_16x9 ;
        Colorimetry = HDMI_ITU601 ;
        break ;
    case HDMI_1080p60:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_1920x1080p60;
    #endif //SUPPORT_DEGEN

        VIC = 16 ;
        VideoPixelClock = 148500000 ;
        pixelrep = 0 ;
        aspec = HDMI_16x9 ;
        Colorimetry = HDMI_ITU709 ;
        break ;
    case HDMI_576p50:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_720x576p50;
    #endif //SUPPORT_DEGEN

        VIC = 17 ;
        VideoPixelClock = 27000000 ;
        pixelrep = 0 ;
        aspec = HDMI_4x3 ;
        Colorimetry = HDMI_ITU601 ;
        break ;
    case HDMI_576p50_16x9:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_720x576p50;
    #endif //SUPPORT_DEGEN

        VIC = 18 ;
        VideoPixelClock = 27000000 ;
        pixelrep = 0 ;
        aspec = HDMI_16x9 ;
        Colorimetry = HDMI_ITU601 ;
        break ;
    case HDMI_720p50:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_1280x720p50;
    #endif //SUPPORT_DEGEN

        VIC = 19 ;
        VideoPixelClock = 74250000 ;
        pixelrep = 0 ;
        aspec = HDMI_16x9 ;
        Colorimetry = HDMI_ITU709 ;
        break ;
    case HDMI_1080i50:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_1920x1080i50;
    #endif //SUPPORT_DEGEN

        VIC = 20 ;
        VideoPixelClock = 74250000 ;
        pixelrep = 0 ;
        aspec = HDMI_16x9 ;
        Colorimetry = HDMI_ITU709 ;
        break ;
    case HDMI_576i50:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_720x576i50;
    #endif //SUPPORT_DEGEN

        VIC = 21 ;
        VideoPixelClock = 13500000 ;
        pixelrep = 1 ;
        aspec = HDMI_4x3 ;
        Colorimetry = HDMI_ITU601 ;
        break ;
    case HDMI_576i50_16x9:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_720x576i50;
    #endif //SUPPORT_DEGEN

        VIC = 22 ;
        VideoPixelClock = 13500000 ;
        pixelrep = 1 ;
        aspec = HDMI_16x9 ;
        Colorimetry = HDMI_ITU601 ;
        break ;
    case HDMI_1080p50:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_1920x1080p50;
    #endif //SUPPORT_DEGEN

        VIC = 31 ;
        VideoPixelClock = 148500000 ;
        pixelrep = 0 ;
        aspec = HDMI_16x9 ;
        Colorimetry = HDMI_ITU709 ;
        break ;
    case HDMI_1080p24:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_1920x1080p24;
    #endif //SUPPORT_DEGEN

        VIC = 32 ;
        VideoPixelClock = 74250000 ;
        pixelrep = 0 ;
        aspec = HDMI_16x9 ;
        Colorimetry = HDMI_ITU709 ;
        break ;
    case HDMI_1080p25:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_1920x1080p25;
    #endif //SUPPORT_DEGEN

        VIC = 33 ;
        VideoPixelClock = 74250000 ;
        pixelrep = 0 ;
        aspec = HDMI_16x9 ;
        Colorimetry = HDMI_ITU709 ;
        break ;
    case HDMI_1080p30:
    #ifdef SUPPORT_DEGEN
        ModeId = CEA_1920x1080p30;
    #endif //SUPPORT_DEGEN

        VIC = 34 ;
        VideoPixelClock = 74250000 ;
        pixelrep = 0 ;
        aspec = HDMI_16x9 ;
        Colorimetry = HDMI_ITU709 ;
        break ;
    case HDMI_1024x768p60:
    #ifdef SUPPORT_DEGEN    
        ModeId = VESA_1024x768p60;
    #endif
        VIC = 36;
        VideoPixelClock = 65000000 ;
        pixelrep = 0 ;
        aspec = 0 ;
        Colorimetry = 0;
        break;
    case HDMI_1280x1024p60:
    #ifdef SUPPORT_DEGEN    
        ModeId = VESA_1280x1024p60;
    #endif
        VIC = 37;
        VideoPixelClock = 108000000 ;
        pixelrep = 0 ;
        aspec = 0 ;
        Colorimetry = 0;
        break;
    case HDMI_1680x1050p60:
    #ifdef SUPPORT_DEGEN    
        ModeId = VESA_1680x1050p60;
    #endif
        VIC = 38;
        VideoPixelClock = 146250000;
        pixelrep = 0 ;
        aspec = 0 ;
        Colorimetry = 0;
        break;
    
    case HDMI_1600x1200p60:
    #ifdef SUPPORT_DEGEN    
        ModeId = VESA_1600x1200p60;
    #endif
        VIC = 39;
        VideoPixelClock = 162000000;
        pixelrep = 0 ;
        aspec = 0 ;
        Colorimetry = 0;
        break;
    case HDMI_1440x900p60:
    #ifdef SUPPORT_DEGEN    
        ModeId = VESA_1440x900p60;
    #endif
        VIC = 40;
        VideoPixelClock = 106500000;
        pixelrep = 0 ;
        aspec = 0 ;
        Colorimetry = 0;
        break;
    default:
        bChangeMode = FALSE ;                
        return ;
    }

    switch(OutputColorMode)
    {
    case HDMI_YUV444:
        bOutputColorMode = F_MODE_YUV444 ;
        break ;
    case HDMI_YUV422:

        bOutputColorMode = F_MODE_YUV422 ;
        break ;
    case HDMI_RGB444:
        bOutputColorMode = F_MODE_RGB444 ;
        break ;

    default:
        bOutputColorMode = F_MODE_RGB444 ;
        break ;
    }

    if( Colorimetry == HDMI_ITU709 )
    {
        bInputColorMode |= F_VIDMODE_ITU709 ;
    }
    else
    {
        bInputColorMode &= ~F_VIDMODE_ITU709 ;
    }
    
    if( Colorimetry != HDMI_640x480p60)
    {
        bInputColorMode |= F_VIDMODE_16_235 ;
    }
    else
    {
        bInputColorMode &= ~F_VIDMODE_16_235 ;
    }

    bChangeMode = TRUE ;
}


void
ConfigAVIInfoFrame()
{
//     AVI_InfoFrame AviInfo;

    AviInfo.pktbyte.AVI_HB[0] = AVI_INFOFRAME_TYPE|0x80 ; 
    AviInfo.pktbyte.AVI_HB[1] = AVI_INFOFRAME_VER ; 
    AviInfo.pktbyte.AVI_HB[2] = AVI_INFOFRAME_LEN ; 
    
//     memset(&AviInfo.info,0,sizeof(AviInfo.info)) ;
// 
//     AviInfo.info.ActiveFmtInfoPresent = 1 ;
//     AviInfo.info.Scan = 0 ; // no ucData.
// 
//     AviInfo.info.BarInfo = 0 ; // no bar ucData valid
// 
//     if ( AviInfo.info.BarInfo )
//     {
//         // should give valid bar ucData.
//         AviInfo.info.Ln_End_Top = 0 ;
//         AviInfo.info.Ln_Start_Bottom = 0 ;
//         AviInfo.info.Pix_End_Left = 0 ;
//         AviInfo.info.Pix_Start_Right = 0 ;
//     }
// 
// 
    switch(bOutputColorMode)
    {
    case F_MODE_YUV444:
        // AviInfo.info.ColorMode = 2 ;
        AviInfo.pktbyte.AVI_DB[0] = (2<<5)|(1<<4) ;
        break ;
    case F_MODE_YUV422:
        // AviInfo.info.ColorMode = 1 ;
        AviInfo.pktbyte.AVI_DB[0] = (1<<5)|(1<<4) ;
        break ;
    case F_MODE_RGB444:
    default:
        // AviInfo.info.ColorMode = 0 ;
        AviInfo.pktbyte.AVI_DB[0] = (0<<5)|(1<<4) ;
        break ;
    }
// 
//     AviInfo.info.ActiveFormatAspectRatio = 8 ; // same as picture aspect ratio
    AviInfo.pktbyte.AVI_DB[1] = 8 ;
//     AviInfo.info.PictureAspectRatio = (aspec != HDMI_16x9)?1:2 ; // 4x3
    if(VIC==0)
        AviInfo.pktbyte.AVI_DB[1] &= 0xC0;
    else
        AviInfo.pktbyte.AVI_DB[1] |= (aspec != HDMI_16x9)?(1<<4):(2<<4) ; // 4:3 or 16:9
//     AviInfo.info.Colorimetry = (Colorimetry != HDMI_ITU709) ? 1:2 ; // ITU601

    AviInfo.pktbyte.AVI_DB[1] |= (Colorimetry != HDMI_ITU709)?(1<<6):(2<<6) ; // 4:3 or 16:9
//     AviInfo.info.Scaling = 0 ;
    AviInfo.pktbyte.AVI_DB[2] = 0 ;
//     AviInfo.info.VIC = VIC ;
    if (VIC > 34)   VIC = 0;
    AviInfo.pktbyte.AVI_DB[3] = VIC ;
//     AviInfo.info.PixelRepetition = pixelrep;
    AviInfo.pktbyte.AVI_DB[4] =  pixelrep & 3 ;
    AviInfo.pktbyte.AVI_DB[5] = 0 ;
    AviInfo.pktbyte.AVI_DB[6] = 0 ;
    AviInfo.pktbyte.AVI_DB[7] = 0 ;
    AviInfo.pktbyte.AVI_DB[8] = 0 ;
    AviInfo.pktbyte.AVI_DB[9] = 0 ;
    AviInfo.pktbyte.AVI_DB[10] = 0 ;
    AviInfo.pktbyte.AVI_DB[11] = 0 ;
    AviInfo.pktbyte.AVI_DB[12] = 0 ;

    EnableAVIInfoFrame(TRUE, (unsigned char *)&AviInfo) ;
}



////////////////////////////////////////////////////////////////////////////////
// Function: ConfigAudioInfoFrm
// Parameter: NumChannel, number from 1 to 8
// Return: ER_SUCCESS for successfull.
// Remark: Evaluate. The speakerplacement is only for reference.
//         For production, the caller of SetAudioInfoFrame should program
//         Speaker placement by actual status.
// Side-Effect:
////////////////////////////////////////////////////////////////////////////////

void
ConfigAudioInfoFrm()
{
    int i ;
    ErrorF("ConfigAudioInfoFrm(%d)\n",2) ;
//    memset(&AudioInfo,0,sizeof(Audio_InfoFrame)) ;
//    
//    AudioInfo.info.Type = AUDIO_INFOFRAME_TYPE ;
//    AudioInfo.info.Ver = 1 ;
//    AudioInfo.info.Len = AUDIO_INFOFRAME_LEN ;

    AudioInfo.pktbyte.AUD_HB[0] = AUDIO_INFOFRAME_TYPE ;
    AudioInfo.pktbyte.AUD_HB[1] = 1 ;
    AudioInfo.pktbyte.AUD_HB[2] = AUDIO_INFOFRAME_LEN ;
//    
//    // 6611 did not accept this, cannot set anything.
//    // AudioInfo.info.AudioCodingType = 1 ; // IEC60958 PCM 
//    AudioInfo.info.AudioChannelCount = 2 - 1 ;
    AudioInfo.pktbyte.AUD_DB[0] = 1 ;
    for( i = 1 ;i < AUDIO_INFOFRAME_LEN ; i++ )
    {
        AudioInfo.pktbyte.AUD_DB[i] = 0 ;
    }
//
//    /* 
//    CAT6611 does not need to fill the sample size information in audio info frame.
//    ignore the part.
//    */
// 
//    AudioInfo.info.SpeakerPlacement = 0x00 ;   //                     FR FL
//    AudioInfo.info.LevelShiftValue = 0 ;    
//    AudioInfo.info.DM_INH = 0 ;    
//
    EnableAudioInfoFrame(TRUE, (unsigned char *)&AudioInfo) ;
}

void HDMITX_TrunOnHDCP()
{
#ifdef SUPPORT_HDCP    
    EnableHDCP(TRUE) ;
#endif    
}

void HDMITX_TrunOffHDCP()
{
#ifdef SUPPORT_HDCP    
    EnableHDCP(FALSE) ;
#endif    
}


/////////////////////////////////////////////////////////////////////
// ParseEDID()
// Check EDID check sum and EDID 1.3 extended segment.
/////////////////////////////////////////////////////////////////////

BOOL
ParseEDID()
{
    // collect the EDID ucdata of segment 0
    BYTE CheckSum ;
    BYTE BlockCount ;
    BOOL err = FALSE;
    BOOL bValidCEA = FALSE ;
    int i ;

    RxCapability.ValidCEA = FALSE ;
	
    GetEDIDData(0, EDID_Buf);


    for( i = 0, CheckSum = 0 ; i < 128 ; i++ )
    {
        CheckSum += EDID_Buf[i] ; CheckSum &= 0xFF ;
    }
	
			//Eep_Write(0x80, 0x80, EDID_Buf) ;
	if( CheckSum != 0 )
	{
		return FALSE ;
	}
	
	if( EDID_Buf[0] != 0x00 ||
	    EDID_Buf[1] != 0xFF ||
	    EDID_Buf[2] != 0xFF ||
	    EDID_Buf[3] != 0xFF ||
	    EDID_Buf[4] != 0xFF ||
	    EDID_Buf[5] != 0xFF ||
	    EDID_Buf[6] != 0xFF ||
	    EDID_Buf[7] != 0x00)
    {
        return FALSE ;
    }


    BlockCount = EDID_Buf[0x7E] ;

    if( BlockCount == 0 )
    {
        return TRUE ; // do nothing.
    }
    else if ( BlockCount > 4 )
    {
        BlockCount = 4 ;
    }
        	
     // read all segment for test
    for( i = 1 ; i <= BlockCount ; i++ )
    {
        err = GetEDIDData(i, EDID_Buf) ;

        if( err )
        {  
           if( !bValidCEA && EDID_Buf[0] == 0x2 && EDID_Buf[1] == 0x3 )
            {
                err = ParseCEAEDID(EDID_Buf) ;
                if( err )
                {
 
				    if(RxCapability.IEEEOUI==0x0c03)
				    {
				    	RxCapability.ValidHDMI = TRUE ;
				    	bValidCEA = TRUE ;
					}
				    else
				    {
				    	RxCapability.ValidHDMI = FALSE ;
				    }
				                   
                }
            }
        }
    }

    return err ;

}

static BOOL
ParseCEAEDID(BYTE *pCEAEDID)
{
    BYTE offset,End ;
    BYTE count ;
    BYTE tag ;
    int i ;

    if( pCEAEDID[0] != 0x02 || pCEAEDID[1] != 0x03 ) return ER_SUCCESS ; // not a CEA BLOCK.
    End = pCEAEDID[2]  ; // CEA description.
    RxCapability.VideoMode = pCEAEDID[3] ;

	RxCapability.VDOModeCount = 0 ;
    RxCapability.idxNativeVDOMode = 0xff ;
    
    for( offset = 4 ; offset < End ; )
    {
        tag = pCEAEDID[offset] >> 5 ;
        count = pCEAEDID[offset] & 0x1f ;
        switch( tag )
        {

        case 0x02: // Video Data Block ;
            //RxCapability.VDOModeCount = 0 ;
            offset ++ ;
            for( i = 0,RxCapability.idxNativeVDOMode = 0xff ; i < count ; i++, offset++ )
            {
            	BYTE VIC ;
            	VIC = pCEAEDID[offset] & (~0x80) ;
            	// if( FindModeTableEntryByVIC(VIC) != -1 )
            	{
	                RxCapability.VDOMode[RxCapability.VDOModeCount] = VIC ;
	                if( pCEAEDID[offset] & 0x80 )
	                {
	                    RxCapability.idxNativeVDOMode = (BYTE)RxCapability.VDOModeCount ;
	                    iVideoModeSelect = RxCapability.VDOModeCount ;
	                }

	                RxCapability.VDOModeCount++ ;
            	}
            }
            break ;

        case 0x03: // Vendor Specific Data Block ;
            offset ++ ;
            RxCapability.IEEEOUI = (ULONG)pCEAEDID[offset+2] ;
            RxCapability.IEEEOUI <<= 8 ;
            RxCapability.IEEEOUI += (ULONG)pCEAEDID[offset+1] ;
            RxCapability.IEEEOUI <<= 8 ;
            RxCapability.IEEEOUI += (ULONG)pCEAEDID[offset] ;
            offset += count ; // ignore the remaind.

            break ;
#ifdef PARSE_AUDIO_BLOCK
        case 0x01: // Audio Data Block ;
            RxCapability.AUDDesCount = count/3 ;
            offset++ ;
            for( i = 0 ; i < RxCapability.AUDDesCount ; i++ )
            {
                RxCapability.AUDDes[i].uc[0] = pCEAEDID[offset++] ;
                RxCapability.AUDDes[i].uc[1] = pCEAEDID[offset++] ;
                RxCapability.AUDDes[i].uc[2] = pCEAEDID[offset++] ;
            }

            break ;

        case 0x04: // Speaker Data Block ;
            offset ++ ;
            RxCapability.SpeakerAllocBlk.uc[0] = pCEAEDID[offset] ;
            RxCapability.SpeakerAllocBlk.uc[1] = pCEAEDID[offset+1] ;
            RxCapability.SpeakerAllocBlk.uc[2] = pCEAEDID[offset+2] ;
            offset += 3 ;
            break ;
#endif

// FUTHUR_USING            
#if 0 // reserved but not enabled for easy porting.
        case 0x05: // VESA Data Block ;
            offset += count+1 ;
            break ;
        case 0x07: // Extended Data Block ;
            offset += count+1 ; //ignore
            break ;
#endif            
        default:
            offset += count+1 ; // ignore
        }
    }
    RxCapability.ValidCEA = TRUE ;
    return TRUE ;
}

/*******************************************************************
 * GM added this function
 *******************************************************************
 */
#ifdef HARRY_CHANGE 
/* Input parameter: 
 * bRGB: 1 for RGB888 input. 0 for YUV422 Embedded Sync.
 */ 
void CAT6611_Config_SyncMode(int bRGB)
{
    if (bRGB)
    {
        support_syncemb = 0;
        support_degen = 1;
        /* because we don't have DE signal */
        bInputSignalType = T_MODE_DEGEN;
        bInputColorMode = F_MODE_RGB444;
    }
    else
    {
        support_syncemb = 1;
        support_degen = 0;
        bInputSignalType = T_MODE_SYNCEMB;    //default
        bInputColorMode = F_MODE_YUV422;
    }
    //HDMITX_SetOutput();
}
#endif /* HARRY_CHANGE */
