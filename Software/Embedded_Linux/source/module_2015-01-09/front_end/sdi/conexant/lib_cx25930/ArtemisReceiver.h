/*+++ *******************************************************************\
*
*  Copyright and Disclaimer:
*
*     ---------------------------------------------------------------
*     This software is provided "AS IS" without warranty of any kind,
*     either expressed or implied, including but not limited to the
*     implied warranties of noninfringement, merchantability and/or
*     fitness for a particular purpose.
*     ---------------------------------------------------------------
*
*     Copyright (c) 2013 Conexant Systems, Inc.
*     All rights reserved.
*
\******************************************************************* ---*/
#ifndef _ARTEMIS_RECEIVER_H_
#define _ARTEMIS_RECEIVER_H_

#include "Comm.h"
#include "CxApiDefines.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define CX25930_VER_MAJOR       2
#define CX25930_VER_MINOR       0
#define CX25930_VER_REL         118
#define CX25930_VERSION_CODE    KERNEL_VERSION(CX25930_VER_MAJOR, CX25930_VER_MINOR, CX25930_VER_REL)

#define ARTM_RATE_SD            0
#define ARTM_RATE_HD            1
#define ARTM_RATE_GEN3          2

#define ARTM_VOUT_FMT_BT656     0
#define ARTM_VOUT_FMT_BT1120    1

#define ARTM_CHIP_REV_11ZP      0x0001
#define ARTM_CHIP_REV_11ZP2     0x0002

struct artm_re_info_t {
    unsigned int    input_rate;
    unsigned int    pre_plugin;
    unsigned int    cur_plugin;
    unsigned int    value_up;
    unsigned int    value_down;
    unsigned int    cdr_ph_up;
    unsigned int    settled;
    unsigned int    force_blue;
};

// EXTRACTION_MODE
typedef enum {
    CX25930_AUTO_DETECT             = 0, // (default)
    CX25930_AUTO_DETECT_MONO        = 0x1,
    CX25930_AUTO_DETECT_STEREO      = 0x2,
    CX25930_SPECIFIC_CHANNEL_MONO   = 0x4,
    CX25930_SPECIFIC_CHANNEL_STEREO = 0x5,
} CX25930_EXTRACTION_MODE;

// EXTENDED_WS_FORMAT
typedef enum {
    CX25930_EXTENDED_I2S_FORMAT = 0x0,
    CX25930_DSP_FORMAT          = 0x1,
    CX25930_EXTENDED_DSP_FORMAT = 0x2,
} CX25930_EXTENDED_WS_FORMAT;

// EXTENDED_MODE
typedef enum {
    CX25930_STANDARD_I2S     = 0,
    CX25930_EXTENDED_I2S_4CH = 1,
    CX25930_EXTENDED_I2S_8CH = 2,
} CX25930_EXTENDED_MODE;

// SLOT_WIDTH
typedef enum {
    CX25930_BIT_8   = 0,
    CX25930_BIT_16  = 1,
    CX25930_BIT_24  = 2,
} CX25930_SLOT_WIDTH;

// SAMPLE_RATE
typedef enum {
    CX25930_KHZ48    = 0,
    CX25930_KHZ44PT1 = 1,
    CX25930_KHZ32    = 2,
    CX25930_KHZ16    = 3,
    CX25930_KHZ8     = 4,
} CX25930_SAMPLE_RATE;

// ENABLE_I2S
typedef enum {
    CX25930_ENABLE_0 = 1,
    CX25930_ENABLE_1 = 2,
    CX25930_ENABLE_2 = 4,
    CX25930_ENABLE_3 = 8,
} CX25930_ENABLE_I2S;

typedef struct {
    CX25930_EXTRACTION_MODE     extraction_mode;
    CX25930_EXTENDED_WS_FORMAT  extended_ws_format;
    CX25930_EXTENDED_MODE       extended_mode;
    CX25930_SLOT_WIDTH          slot_width;
    CX25930_SAMPLE_RATE         sample_rate;
    CX25930_ENABLE_I2S          enable_i2s;
    int                         master_mode;
    int                         channel;
} CX25930_AUDIO_EXTRACTION;

extern void ARTM_enableXtalOutput(const CX_COMMUNICATION *p_comm);

extern int ARTM_initialize(const CX_COMMUNICATION *p_comm, int length, int rate, int vout_fmt);

extern void ARTM_force_bluescreen(const CX_COMMUNICATION *p_comm, int ch_num, int rate);

extern void ARTM_enableBT1120(const CX_COMMUNICATION *p_comm);

extern void ARTM_enableBT656(const CX_COMMUNICATION *p_comm);

extern void ARTM_setCableDistance(const CX_COMMUNICATION *p_comm, int ch_num, int length);

extern void ARTM_disableEnableRE(const CX_COMMUNICATION *p_comm, int ch_num);

extern void ARTM_remove_forced_bluescreen(const CX_COMMUNICATION *p_comm, int ch_num);

extern void ARTM_checkRasterEngineStatus_11ZP(const CX_COMMUNICATION *p_comm, struct artm_re_info_t *info, int ch_num);
extern int  ARTM_checkRasterEngineStatus(const CX_COMMUNICATION *p_comm, struct artm_re_info_t *info, int ch_num);

extern void ARTM_resetRE(const CX_COMMUNICATION *p_comm, int ch_num);

extern void ARTM_EQ_Check(const CX_COMMUNICATION *p_comm, int ch_num, int rate);

extern void ARTM_RasterEngineEnable(const CX_COMMUNICATION *p_comm, int ch_num);
extern void ARTM_RasterEngineDisable(const CX_COMMUNICATION *p_comm, int ch_num);

extern int
ARTM_writeRegister(const CX_COMMUNICATION *p_comm,
                   const unsigned long registerAddress,
                   const unsigned long value);

extern int
ARTM_readRegister(const CX_COMMUNICATION *p_comm,
                  const unsigned long registerAddress,
                  unsigned long *p_value);

extern int ARTM_PowerDownTxChannel(const CX_COMMUNICATION *p_comm, int ch_num);

extern int ARTM_EQLoopback(const CX_COMMUNICATION *p_comm, int in_ch_num, int out_ch_num);

extern int ARTM_CDRLoopback(const CX_COMMUNICATION *p_comm, int in_ch_num, int out_ch_num);

extern int ARTM_BT1120Loopback(const CX_COMMUNICATION *p_comm, int in_ch_num, int out_ch_num, int data_rate);

extern int ARTM_XREnable(const CX_COMMUNICATION *p_comm, int ch_num, int dB);

extern int ARTM_getVideoFormat(const CX_COMMUNICATION *p_comm, int ch_num, int* p_format);

extern int ARTM_isCablePlugin(const CX_COMMUNICATION *p_comm, int ch_num);

extern int ARTM_Enable_GPIOC_MCLK(const CX_COMMUNICATION *p_comm);

extern int ARTM_AudioInit(const CX_COMMUNICATION *p_comm, CX25930_AUDIO_EXTRACTION *aud_extract);

extern int ARTM_AudioInitCh(const CX_COMMUNICATION *p_comm, int ch_num);

extern int ARTM_PinRemap_I2S(const CX_COMMUNICATION *p_comm, int ch_num,
                             unsigned char MCLK, unsigned char WS,
                             unsigned char SCK, unsigned char I2S_SD);

extern int ARTM_get_Chip_RevID(const CX_COMMUNICATION *p_comm);

extern int ARTM_ReadLOS(const CX_COMMUNICATION *p_comm, int ch_num);

extern void ARTM_LockToFractionalFrames(const CX_COMMUNICATION *p_comm, int ch_num);

extern int ARTM_RasterEngine_Format_Detected(const CX_COMMUNICATION *p_comm, int ch_num);

extern int ARTM_BurstTransfer_Ctrl(const CX_COMMUNICATION *p_comm, int enable, unsigned char inc_amount, unsigned short burst_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // _ARTEMIS_RECEIVER_H_
