#ifndef _DH9901_LIB_H_
#define _DH9901_LIB_H_

/*************************************************************************************
 *  DH9901 Type Definition
 *************************************************************************************/
typedef unsigned char  DH_U8;
typedef unsigned short DH_U16;
typedef unsigned int   DH_U32;

typedef char  DH_S8;
typedef short DH_S16;
typedef int   DH_S32;

typedef int   DH_BOOL;

/*************************************************************************************
 *  DH9901 Definition
 *************************************************************************************/
#define DH_CHNS_PER_DH9901          4
#define MAX_DH9901_NUM_PER_CPU      4

typedef struct dh_video_position_s {
    DH_S32 sHorizonOffset;
    DH_S32 sVerticalOffset;
    DH_S32 res[2];
} DH_VIDEO_POSITION_S;

typedef enum dh_set_mode_e {
    DH_SET_MODE_DEFAULT = 0,
    DH_SET_MODE_USER = 1,
    DH_SET_MODE_NONE,
} DH_SET_MODE_E;

typedef struct dh_video_color_s {
    DH_U8 ucBrightness;
    DH_U8 ucContrast;
    DH_U8 ucSaturation;
    DH_U8 ucHue;
    DH_U8 ucGain;
    DH_U8 ucWhiteBalance;
    DH_U8 ucSharpness;
    DH_U8 reserved[1];
} DH_VIDEO_COLOR_S;

typedef enum dh_hd_video_format_e {
    DH_HD_720P_25HZ = 0,
    DH_HD_720P_30HZ,
    DH_HD_720P_50HZ,
    DH_HD_720P_60HZ,
    DH_HD_1080P_25HZ,
    DH_HD_1080P_30HZ,
    DH_HD_DEFAULT,
} DH_HD_VIDEO_FORMAT_E;

typedef struct dh_video_status_s {
    DH_U8 ucVideoLost;
    DH_U8 ucVideoFormat;
    DH_U8 ucVideoReportFormat;
    DH_U8 reserved[1];
} DH_VIDEO_STATUS_S;

typedef struct dh_dh9901_audio_connect_mode_s {
    DH_BOOL ucCascade;
    DH_U8   ucCascadeNum;
    DH_U8   ucCascadeStage;
} DH_AUDIO_CONNECT_MODE_S;

typedef enum dh_audio_samplerate_e {
    DH_AUDIO_SAMPLERATE_8k  = 0,
    DH_AUDIO_SAMPLERATE_16K = 1,
} DH_AUDIO_SAMPLERATE_E;

typedef enum dh_audio_encclk_mode_e {
    DH_AUDIO_ENCCLK_MODE_SLAVE  = 0,
    DH_AUDIO_ENCCLK_MODE_MASTER = 1,
} DH_AUDIO_ENCCLK_MODE_E;

typedef enum dh_audio_sync_mode_e {
    DH_AUDIO_SYNC_MODE_I2S = 0,
    DH_AUDIO_SYNC_MODE_DSP = 1,
} DH_AUDIO_SYNC_MODE_E;

typedef enum dh_audio_encclk_sel_e {
    DH_AUDIO_ENCCLK_MODE_108M = 0,
    DH_AUDIO_ENCCLK_MODE_54M  = 1,
    DH_AUDIO_ENCCLK_MODE_27M  = 2,
} DH_AUDIO_ENCCLK_SEL_E;

typedef enum dh_cable_type_e {
    DH_CABLE_TYPE_COAXIAL = 0,
    DH_CABLE_TYPE_UTP_10OHM,
    DH_CABLE_TYPE_UTP_17OHM,
    DH_CABLE_TYPE_UTP_25OHM,
    DH_CABLE_TYPE_UTP_35OHM,
    DH_CABLE_TYPE_BUT
} DH_CABLE_TYPE_E;

typedef struct dh_dh9901_audio_attr_s {
    DH_BOOL bAudioEnable;
    DH_AUDIO_CONNECT_MODE_S stAudioConnectMode;
    DH_SET_MODE_E enSetAudioMode;
    DH_AUDIO_SAMPLERATE_E enAudioSampleRate;
    DH_AUDIO_ENCCLK_MODE_E enAudioEncClkMode;
    DH_AUDIO_SYNC_MODE_E enAudioSyncMode;
    DH_AUDIO_ENCCLK_SEL_E enAudioEncClkSel;
    DH_U8 reserved[5];
} DH_DH9901_AUDIO_ATTR_S;

typedef struct dh_dh9901_video_attr_s {
    DH_SET_MODE_E enSetVideoMode;
    DH_U8 ucChnSequence[DH_CHNS_PER_DH9901];
    DH_U8 ucVideoOutMux;
    DH_U8 reserved[7];
} DH_DH9901_VIDEO_ATTR_S;

typedef struct dh_dh9901_ptz485attr_s {
    DH_SET_MODE_E enSetVideoMode;
    DH_U8 ucProtocolLen;
    DH_U8 reserved[3];
} DH_DH9901_PTZ485_ATTR_S;

typedef struct dh_auto_detect_attr_s {
    DH_SET_MODE_E enDetectMode;
    DH_U16 ucDetectPeriod;
    DH_U8 reserved[2];
} DH_AUTO_DETECT_ATTR_S;

typedef struct dh_dh9901_attr_s {
    DH_U8 ucChipAddr;
    DH_DH9901_VIDEO_ATTR_S stVideoAttr;
    DH_DH9901_AUDIO_ATTR_S stAuioAttr;
    DH_DH9901_PTZ485_ATTR_S stPtz485Attr;
    DH_U32 reserved[4];
} DH_DH9901_ATTR_S;

typedef struct dh_dh9901_init_attr_s {
    DH_U8 ucAdCount;
    DH_DH9901_ATTR_S stDh9901Attr[MAX_DH9901_NUM_PER_CPU];
    DH_AUTO_DETECT_ATTR_S stDetectAttr;
    DH_S32 (* Dh9901_WriteByte)(DH_U8 ucChipAddr, DH_U8 ucRegAddr, DH_U8 ucRegValue);
    DH_U8  (* Dh9901_ReadByte)(DH_U8 ucChipAddr, DH_U8 ucRegAddr);
    DH_U32 reserved[4];
} DH_DH9901_INIT_ATTR_S;

/*************************************************************************************
 *  DH9901 API Export Function Prototype
 *************************************************************************************/
DH_S32 DH9901_API_Init(DH_DH9901_INIT_ATTR_S *pDh9901InitAttrs);
DH_S32 DH9901_API_DeInit(void);
DH_S32 DH9901_API_SetColor(DH_U8 ucChipIndex, DH_U8 ucChn, DH_VIDEO_COLOR_S *pVideoColor, DH_SET_MODE_E enColorMode);
DH_S32 DH9901_API_GetColor(DH_U8 ucChipIndex, DH_U8 ucChn, DH_VIDEO_COLOR_S *pVideoColor, DH_SET_MODE_E enColorMode);
DH_S32 DH9901_API_ClearEq(DH_U8 ucChipIndex, DH_U8 ucChn);
DH_S32 DH9901_API_Contrl485Enable(DH_U8 ucChipIndex, DH_U8 ucChn, DH_BOOL bEnable);
DH_S32 DH9901_API_Send485Buffer(DH_U8 ucChipIndex, DH_U8 ucChn, DH_U8 *ucBuf, DH_U8 ucLenth);
DH_S32 DH9901_API_SetVideoPosition(DH_U8 ucChipIndex, DH_U8 ucChn, DH_VIDEO_POSITION_S *pVideoPos);
DH_S32 DH9901_API_GetVideoPosition (DH_U8 ucChipIndex, DH_U8 ucChn, DH_VIDEO_POSITION_S *pVideoPos);
DH_S32 DH9901_API_SetAudioInVolume(DH_U8 ucChipIndex, DH_U8 ucChn, DH_U8 ucVolume);
DH_S32 DH9901_API_SetCableType(DH_U8 ucChipIndex, DH_U8 ucChn, DH_CABLE_TYPE_E enCableType);
DH_S32 DH9901_API_GetVideoStatus(DH_U8 ucChipIndex, DH_U8 ucChn, DH_VIDEO_STATUS_S *pVideoStatus);
DH_S32 DH9901_API_ReadReg(DH_U8 ucChipIndex, DH_U8 ucPage, DH_U8 ucRegAddr, DH_U8 *pucValue);
DH_S32 DH9901_API_WriteReg(DH_U8 ucChipIndex, DH_U8 ucPage, DH_U8 ucRegAddr, DH_U8 *pucValue);

#endif  /* _DH9901_LIB_H_ */
