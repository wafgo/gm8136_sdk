/**
 * @file rn6318_lib.c
 * RichNex RN6318 8-CH 960H/D1 Video Decoders and Audio Codecs Library [Build-In RN6318x2]
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.6 $
 * $Date: 2014/07/24 02:26:55 $
 *
 * ChangeLog:
 *  $Log: rn6318_lib.c,v $
 *  Revision 1.6  2014/07/24 02:26:55  andy_hsu
 *  add playback volume control
 *
 *  Revision 1.5  2014/07/24 02:06:01  shawn_hu
 *  1. add IOCTL and proc node for set sharpness and video mode
 *
 *  Revision 1.4  2014/07/21 11:43:49  andy_hsu
 *  Fix rn631x codec's audio driver issue
 *
 *  Revision 1.3  2014/05/22 01:36:50  andy_hsu
 *  Fix rn6318 driver's playback issue.
 *
 *  Revision 1.2  2013/12/31 02:45:42  andy_hsu
 *  add rn6318 and rn6318 audio driver
 *
 *  Revision 1.1.1.1  2013/12/03 09:58:04  jerson_l
 *  add richnex decode driver
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>

#include "richnex/rn6318.h"         ///< from module/include/front_end/decoder

static RN6318_VMODE_T rn6318_vmode[RN6318_DEV_MAX];  ///< for record device video mode

static u8 playback_vol_map[]={0x01,0x03,0x05,0x07,0x09,0x0B,0x0D,0x0F,
                              0x11,0x13,0x15,0x17,0x19,0x1B,0x1D,0x1F};

static unsigned char RN6318_SYS_COMMON_CFG[] = {
    0x82, 0x00,
    0x84, 0x00,
    0x86, 0x00,
    0x88, 0x00,
    0xf2, 0x00,
    0xf4, 0x00,
    0xf6, 0x00,
    0xf8, 0x00,
    0xfa, 0x00,
    0xda, 0x07,
    0x96, 0x0f,
    0x97, 0x0f,
};

static unsigned char RN6318_VIDEO_COMMON_REV_A_CFG[] = {
    0x01, 0x00, ///< brightness (default: 00h)
    0x02, 0x80, ///< contrast   (default: 80h)
    0x03, 0x80, ///< saturation (default: 80h)
    0x04, 0x80, ///< hue        (default: 80h)
    0x05, 0x08,
    0x09, 0x88,
    0x0e, 0xb8,
    0x11, 0x03,
    0x1c, 0xff,
    0x1d, 0x00,
    0x2c, 0x60,
    0x2f, 0x14,
    0x31, 0xa5,
    0x32, 0xff,
    0x36, 0x31,
    0x37, 0x11,
    0x38, 0x7e,
    0x39, 0x04,
    0x4d, 0x13,
    0x61, 0x72,
    0x64, 0xff,
    0x65, 0x30,
    0x66, 0x00,
    0x67, 0xf0,
    0x57, 0x20,
    0x4f, 0x08,
    0x5c, 0x30,
    0x5d, 0x58,
    0x5e, 0x80,
    0x56, 0x03,
};

static unsigned char RN6318_VIDEO_COMMON_CFG[] = {
    0x01, 0x00, ///< brightness (default: 00h)
    0x02, 0x80, ///< contrast   (default: 80h)
    0x03, 0x80, ///< saturation (default: 80h)
    0x04, 0x80, ///< hue        (default: 80h)
    0x09, 0x88,
    0x0d, 0x30,
    0x0e, 0xb8,
    0x13, 0x16,
    0x17, 0x0f,
    0x2a, 0x8f,
    0x2c, 0x60,
    0x36, 0x30,
    0x37, 0x02,
    0x38, 0x77,
};
static unsigned char RN6318_AUDIO_COMMON_CFG[] = {
    0xff, 0x04, //< select audio codec register set
    0x7a, 0x04, //< MCLK = 256 * sample rate
    0x00, 0x09, //< sample rate 8k
    0x06, 0x00, //< ASCLK = AMCLK
    0x5a, 0xfb, //< audio channel 0,1,2 all on; ADC gain = -3.2dB
    0x5b, 0x55, //< audio ADC channel 0,1 gain = 6.27dB
    0x5c, 0x55, //< audio ADC channel 2,3 gain = 6.27dB
    0x5d, 0x7f, //< audio ADC channel 3,40 on; DAC output amplitude = 2.025Vpp
    0x69, 0x05, //< audio ADC channel 40 gain = 6.27dB
    0x5f, 0x38, 
    0x0d, 0x7f, //< sample period of rec/mix = 256
    0x0e, 0x0c, //< I2S_STD ; 16 audio sounds in ADATR (0x08 for single rn6318; 0x0c for double rn6318)
    0x30, 0x80, //< set default
    0x31, 0x88, //< set default
    0x0a, 0x80, //< set default
    0x0b, 0x08, //< set default
    0x08, 0x7f, //< sample period of playback = 256
    0x09, 0x70, //< for 2 rn6318 cascading (0x30 for single rn6318; 0x70 for double rn6318)
    0x32, 0x00, //< bug, set default
    0x61, 0x3f  //< bug, set default
#if 0
#if 0 //single 6318
    0x20, 0x03,
    0x21, 0x02,
    0x22, 0x01,
    0x23, 0x00,
    0x28, 0x0b, 
    0x29, 0x0a,
    0x2a, 0x09,
    0x2b, 0x08,
#else //cascade 6318
    0x20, 0x07,
    0x21, 0x06,
    0x22, 0x05,
    0x23, 0x04,
    0x24, 0x03,
    0x25, 0x02,
    0x26, 0x01,
    0x27, 0x00,
    0x28, 0x0f, 
    0x29, 0x0e,
    0x2a, 0x0d,
    0x2b, 0x0c,
    0x2c, 0x0b, 
    0x2d, 0x0a,
    0x2e, 0x09,
    0x2f, 0x08
#endif  
#endif
};

static u8 RN6318_AUDIO_SIN_TIMESEQ[RN6318_AUDIO_CH_MAX/2] = 
                                    {0x20,0x21,0x22,0x23,0x28,0x29,0x2A,0x2B};
static u8 RN6318_AUDIO_DUO_TIMESEQ[RN6318_AUDIO_CH_MAX] = 
                                      {0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
                                       0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F};

static u8 RN6318_AUDIO_SIN_CH_DATA_DEF[RN6318_AUDIO_CH_MAX/2] = 
                                    {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};

static u8 RN6318_AUDIO_SIN_CH_DATA_8287[RN6318_AUDIO_CH_MAX/2] = 
                                    {0x03,0x02,0x01,0x00,0x07,0x06,0x05,0x04};

static u8 RN6318_AUDIO_DUO_CH_DATA_DEF[RN6318_AUDIO_CH_MAX] = 
                                      {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                                       0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};

static u8 RN6318_AUDIO_DUO_CH_DATA_8287[RN6318_AUDIO_CH_MAX] = 
                                      {0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00,
                                       0x0F,0x0E,0x0D,0x0C,0x0B,0x0A,0x09,0x08};

static unsigned char RN6318_PAL_720H_VIDEO_CFG[] = {
    0x07, 0x22, ///< force in PAL decoding mode
    0x20, 0xa4,
    0x23, 0x17,
    0x24, 0x37,
    0x25, 0x18,
    0x26, 0x38,
    0x21, 0x46,
    0x27, 0x68,
    0x4a, 0x1d,
    0x28, 0xf2,
};

static unsigned char RN6318_NTSC_720H_VIDEO_CFG[] = {
    0x07, 0x23, ///< force in NTSC decoding mode
    0x20, 0xa4,
    0x23, 0x12,
    0x24, 0x06,
    0x25, 0x12,
    0x26, 0x05,
    0x21, 0x43,
    0x27, 0x68,
    0x4a, 0x1d,
    0x28, 0xf2,
};

static unsigned char RN6318_PAL_960H_VIDEO_CFG[] = {
    0x07, 0x22, ///< force in PAL decoding mode
    0x20, 0xa4,
    0x23, 0x17,
    0x24, 0x37,
    0x25, 0x18,
    0x26, 0x38,
    0x21, 0x46,
    0x27, 0xe0,
    0x4a, 0x1d,
    0x28, 0xf2,
};

static unsigned char RN6318_NTSC_960H_VIDEO_CFG[] = {
    0x07, 0x23, ///< force in NTSC decoding mode
    0x20, 0xa4,
    0x23, 0x12,
    0x24, 0x06,
    0x25, 0x12,
    0x26, 0x05,
    0x21, 0x43,
    0x27, 0xe0,
    0x4a, 0x1d,
    0x28, 0xf2,
};

static u8 *rn6318_sin_ch_data[] = {
    (u8 *)&RN6318_AUDIO_SIN_CH_DATA_8287, //GM8287
    (u8 *)&RN6318_AUDIO_SIN_CH_DATA_8287, //GM8286
    (u8 *)&RN6318_AUDIO_SIN_CH_DATA_8287, //GM8282
    (u8 *)&RN6318_AUDIO_SIN_CH_DATA_8287, //GM8210
    (u8 *)&RN6318_AUDIO_SIN_CH_DATA_8287 //GM8283
};

static u8 *rn6318_duo_ch_data[] = {
    (u8 *)&RN6318_AUDIO_DUO_CH_DATA_8287,
    (u8 *)&RN6318_AUDIO_DUO_CH_DATA_8287,
    (u8 *)&RN6318_AUDIO_DUO_CH_DATA_8287,
    (u8 *)&RN6318_AUDIO_DUO_CH_DATA_8287,
    (u8 *)&RN6318_AUDIO_DUO_CH_DATA_8287
};

static u8 *rn6318_sin_ch_data_def[] = {
    (u8 *)&RN6318_AUDIO_SIN_CH_DATA_DEF,
    (u8 *)&RN6318_AUDIO_SIN_CH_DATA_DEF,
    (u8 *)&RN6318_AUDIO_SIN_CH_DATA_DEF,
    (u8 *)&RN6318_AUDIO_SIN_CH_DATA_DEF,
    (u8 *)&RN6318_AUDIO_SIN_CH_DATA_DEF
};

static u8 *rn6318_duo_ch_data_def[] = {
    (u8 *)&RN6318_AUDIO_DUO_CH_DATA_DEF,
    (u8 *)&RN6318_AUDIO_DUO_CH_DATA_DEF,
    (u8 *)&RN6318_AUDIO_DUO_CH_DATA_DEF,
    (u8 *)&RN6318_AUDIO_DUO_CH_DATA_DEF,
    (u8 *)&RN6318_AUDIO_DUO_CH_DATA_DEF
};

static u8 *rn6318_time_seq=NULL, *rn6318_ch_data=NULL, *rn6318_ch_data_def=NULL;

static int rn6318_get_product_id(int id)
{
    int pid;

    if(id >= RN6318_DEV_MAX)
        return 0;

    pid = (((rn6318_i2c_read(id, 0, 0xfd)) | (rn6318_i2c_read(id, 0, 0xfe)<<8)) & 0xfff);

    return pid;
}

static int rn6318_comm_init(int id)
{
    int ret = 0;
    int dev_id;

    if(id >= RN6318_DEV_MAX)
        return -1;

    for(dev_id=0; dev_id<RN6318_DEV_BUILT_IN_DEV_MAX; dev_id++) {
        ret = rn6318_i2c_write(id, dev_id, 0x80, 0x31); if(ret < 0) goto exit;  ///< software reset
        mdelay(10);

        ret = rn6318_i2c_write(id, dev_id, 0xd2, 0x85); if(ret < 0) goto exit;
        ret = rn6318_i2c_write(id, dev_id, 0xd6, 0x37); if(ret < 0) goto exit;
        ret = rn6318_i2c_write(id, dev_id, 0x80, 0x36); if(ret < 0) goto exit;
        mdelay(10);

        ret = rn6318_i2c_write(id, dev_id, 0x80, 0x30); if(ret < 0) goto exit;
        mdelay(30);

        ret = rn6318_i2c_write_table(id, dev_id, RN6318_SYS_COMMON_CFG, ARRAY_SIZE(RN6318_SYS_COMMON_CFG));
        if(ret < 0)
            goto exit;
    }

exit:
    return ret;
}

int rn6318_video_set_mode(int id, RN6318_VMODE_T mode)
{
    int  ret;
    int  dev_id;
    int  ch, pid;
    char mode_str[32];

    if(id >= RN6318_DEV_MAX || mode >= RN6318_VMODE_MAX)
        return -1;

    if(rn6318_video_mode_support_check(id, mode) == 0) {
        printk("RN6318#%d driver not support video output mode(%d)!!\n", id, mode);
        return -1;
    }

    /* system common init */
    ret = rn6318_comm_init(id);
    if(ret < 0)
        goto exit;

    pid = rn6318_get_product_id(id);

    for(dev_id=0; dev_id<RN6318_DEV_BUILT_IN_DEV_MAX; dev_id++) {
        for(ch=0; ch<RN6318_DEV_CH_MAX; ch++) {
            ret = rn6318_i2c_write(id, dev_id, 0xff, ch);   ///< switch to the video decoder channel control page
            if(ret < 0)
                goto exit;

            /* video common init */
            if(pid == RN6318_DEV_REV_A_PID)
                ret = rn6318_i2c_write_table(id, dev_id, RN6318_VIDEO_COMMON_REV_A_CFG, ARRAY_SIZE(RN6318_VIDEO_COMMON_REV_A_CFG));
            else
                ret = rn6318_i2c_write_table(id, dev_id, RN6318_VIDEO_COMMON_CFG, ARRAY_SIZE(RN6318_VIDEO_COMMON_CFG));
            if(ret < 0)
                goto exit;

            /* video mode init */
            switch(mode) {
                case RN6318_VMODE_NTSC_720H_4CH:
                    ret = rn6318_i2c_write_table(id, dev_id, RN6318_NTSC_720H_VIDEO_CFG, ARRAY_SIZE(RN6318_NTSC_720H_VIDEO_CFG));
                    if(ret < 0)
                        goto exit;
                    sprintf(mode_str, "NTSC 720H 108MHz MODE");
                    break;
                case RN6318_VMODE_PAL_720H_4CH:
                    ret = rn6318_i2c_write_table(id, dev_id, RN6318_PAL_720H_VIDEO_CFG, ARRAY_SIZE(RN6318_PAL_720H_VIDEO_CFG));
                    if(ret < 0)
                        goto exit;
                    sprintf(mode_str, "PAL 720H 108MHz MODE");
                    break;
                case RN6318_VMODE_NTSC_960H_4CH:
                    ret = rn6318_i2c_write_table(id, dev_id, RN6318_NTSC_960H_VIDEO_CFG, ARRAY_SIZE(RN6318_NTSC_960H_VIDEO_CFG));
                    if(ret < 0)
                        goto exit;
                    sprintf(mode_str, "NTSC 960H 144MHz MODE");
                    break;
                case RN6318_VMODE_PAL_960H_4CH:
                    ret = rn6318_i2c_write_table(id, dev_id, RN6318_PAL_960H_VIDEO_CFG, ARRAY_SIZE(RN6318_PAL_960H_VIDEO_CFG));
                    if(ret < 0)
                        goto exit;
                    sprintf(mode_str, "PAL 960H 144MHz MODE");
                    break;
                default:
                    ret = -1;
                    goto exit;
            }

            /* video format init */
            switch(mode) {
                case RN6318_VMODE_NTSC_720H_4CH:
                case RN6318_VMODE_PAL_720H_4CH:
                    ret = rn6318_i2c_write(id, dev_id, 0x50, 0x00);   ///< output interface is BT-656
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0x63, 0x07);   ///< composite anti-aliasing filter control (Reg63 default: 09h)
                    if(ret < 0)                               ///< Reg63[3:1]: bandwidth selection, Reg63[0]: enable/disable
                        goto exit;
                    break;
                case RN6318_VMODE_NTSC_960H_4CH:
                case RN6318_VMODE_PAL_960H_4CH:
                    ret = rn6318_i2c_write(id, dev_id, 0x50, 0x01);   ///< output interface is BT-1302
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0x63, 0x09);   ///< composite anti-aliasing filter control (Reg63 default: 09h)
                    if(ret < 0)                               ///< Reg63[3:1]: bandwidth selection, Reg63[0]: enable/disable
                        goto exit;
                    break;
                default:
                    ret = -1;
                    goto exit;
            }

            /* video loss output black screen */
            ret = rn6318_i2c_write(id, dev_id, 0x1a, 0x83); ///< blue: 0x03, black: 0x83
            if(ret < 0)
                goto exit;

            /* embedded channel ID in SAV/EAV */
            ret = rn6318_i2c_write(id, dev_id, 0x3a, 0x20);
            if(ret < 0)
                goto exit;

            /* channel id assign */
            ret = rn6318_i2c_write(id, dev_id, 0x3f, (0x20|ch));
            if(ret < 0)
                goto exit;
        }

        /* video output port init */
        switch(mode) {
            case RN6318_VMODE_NTSC_720H_4CH:
            case RN6318_VMODE_PAL_720H_4CH:
                if(dev_id == 0) {
                    /* VPort3 */
                    ret = rn6318_i2c_write(id, dev_id, 0x94, 0x02); ///< configure VP3[7:0] in 4CH pixel interleaved
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0x95, 0x00); ///< 720H data stream on VP3[7:0]
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0x93, 0x31); ///< enable VP3[7:0] un-scaled image (1X base clock) output; Drv = 12mA
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0xF7, 0x02); ///< configure SCLK3A : base clock 27Mhz; output 4X base clock (108M)
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0xF6, 0xC3); ///< enable SCLK3A. Drv =12mA . Inv clck.
                    if(ret < 0)
                        goto exit;
                }
                else {
                    /* VPort1 */
                    ret = rn6318_i2c_write(id, dev_id, 0x8E, 0x02); ///< configure VP1[7:0] in 4CH pixel interleaved
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0x8F, 0x00); ///< 720H data stream on VP1[7:0]
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0x8D, 0x31); ///< enable VP1[7:0] un-scaled image (1X base clock) output; Drv = 12mA
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0x89, 0x02); ///< configure SCLK1B : base clock 27Mhz; output 4X base clock (108M)
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0x88, 0xC3); ///< enable SCLK1B. Drv =12mA . Inv clck.
                    if(ret < 0)
                        goto exit;
                }
                break;
            case RN6318_VMODE_NTSC_960H_4CH:
            case RN6318_VMODE_PAL_960H_4CH:
                if(dev_id == 0) {
                    /* VPORT3 */
                    ret = rn6318_i2c_write(id, dev_id, 0x94, 0x02); ///< configure VP3[7:0] in 4CH pixel interleaved
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0x95, 0x80); ///< 960H data stream on VP3[7:0]
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0x93, 0x31); ///< enable VP3[7:0] un-scaled image (1X base clock) output; Drv = 12mA
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0xF7, 0x0a); ///< configure SCLK3A : base clock 36Mhz; output 4X base clock (144M)
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0xF6, 0xC3); ///< enable SCLK3A. Drv =12mA . Inv clck.
                    if(ret < 0)
                        goto exit;
                }
                else {
                    /* VPORT1 */
                    ret = rn6318_i2c_write(id, dev_id, 0x8E, 0x02); ///< configure VP1[7:0] in 4CH pixel interleaved
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0x8F, 0x80); ///< 960H data stream on VP1[7:0]
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0x8D, 0x31); ///< enable VP1[7:0] un-scaled image (1X base clock) output; Drv = 12mA
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0x89, 0x0a); ///< configure SCLK1B : base clock 36Mhz; output 4X base clock (144M)
                    if(ret < 0)
                        goto exit;
                    ret = rn6318_i2c_write(id, dev_id, 0x88, 0xC3); ///< enable SCLK1B. Drv =12mA . Inv clck.
                    if(ret < 0)
                        goto exit;
                }
                break;
            default:
                ret = -1;
                goto exit;
        }

        if(dev_id == 0)
            ret = rn6318_i2c_write(id, dev_id, 0x81, 0x3f);         ///< turn-on all video decoders & audio adc
        else
            ret = rn6318_i2c_write(id, dev_id, 0x81, 0x1f);
        if(ret < 0)
            goto exit;
    }

    /* Update VMode */
    rn6318_vmode[id] = mode;

    printk("RN6318#%d ==> %s!!\n", id, mode_str);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_video_set_mode);

int rn6318_video_get_mode(int id)
{
    if(id >= RN6318_DEV_MAX)
        return -1;

    return rn6318_vmode[id];
}
EXPORT_SYMBOL(rn6318_video_get_mode);

int rn6318_video_mode_support_check(int id, RN6318_VMODE_T mode)
{
    if(id >= RN6318_DEV_MAX)
        return 0;

    switch(mode) {
        case RN6318_VMODE_NTSC_720H_4CH:
        case RN6318_VMODE_NTSC_960H_4CH:
        case RN6318_VMODE_PAL_720H_4CH:
        case RN6318_VMODE_PAL_960H_4CH:
            return 1;
        default:
            return 0;
    }

    return 0;
}
EXPORT_SYMBOL(rn6318_video_mode_support_check);

int rn6318_video_set_brightness(int id, int ch, u8 value)
{
    int ret;

    if(id >= RN6318_DEV_MAX || ch >= RN6318_DEV_CH_MAX)
        return -1;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0xff, RN6318_BUILD_IN_DEV_CH(ch));   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0x01, value);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_video_set_brightness);

int rn6318_video_get_brightness(int id, int ch)
{
    int ret;

    if(id >= RN6318_DEV_MAX || ch >= RN6318_DEV_CH_MAX)
        return -1;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0xff, RN6318_BUILD_IN_DEV_CH(ch));   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6318_i2c_read(id, RN6318_BUILD_IN_DEV_ID(ch), 0x01);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_video_get_brightness);

int rn6318_video_set_contrast(int id, int ch, u8 value)
{
    int ret;

    if(id >= RN6318_DEV_MAX || ch >= RN6318_DEV_CH_MAX)
        return -1;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0xff, RN6318_BUILD_IN_DEV_CH(ch));   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0x02, value);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_video_set_contrast);

int rn6318_video_get_contrast(int id, int ch)
{
    int ret;

    if(id >= RN6318_DEV_MAX || ch >= RN6318_DEV_CH_MAX)
        return -1;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0xff, RN6318_BUILD_IN_DEV_CH(ch));   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6318_i2c_read(id, RN6318_BUILD_IN_DEV_ID(ch), 0x02);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_video_get_contrast);

int rn6318_video_set_saturation(int id, int ch, u8 value)
{
    int ret;

    if(id >= RN6318_DEV_MAX || ch >= RN6318_DEV_CH_MAX)
        return -1;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0xff, RN6318_BUILD_IN_DEV_CH(ch));   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0x03, value);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_video_set_saturation);

int rn6318_video_get_saturation(int id, int ch)
{
    int ret;

    if(id >= RN6318_DEV_MAX || ch >= RN6318_DEV_CH_MAX)
        return -1;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0xff, RN6318_BUILD_IN_DEV_CH(ch));   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6318_i2c_read(id, RN6318_BUILD_IN_DEV_ID(ch), 0x03);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_video_get_saturation);

int rn6318_video_set_hue(int id, int ch, u8 value)
{
    int ret;

    if(id >= RN6318_DEV_MAX || ch >= RN6318_DEV_CH_MAX)
        return -1;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0xff, RN6318_BUILD_IN_DEV_CH(ch));   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0x04, value);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_video_set_hue);

int rn6318_video_get_hue(int id, int ch)
{
    int ret;

    if(id >= RN6318_DEV_MAX || ch >= RN6318_DEV_CH_MAX)
        return -1;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0xff, RN6318_BUILD_IN_DEV_CH(ch));   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6318_i2c_read(id, RN6318_BUILD_IN_DEV_ID(ch), 0x04);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_video_get_hue);

int rn6318_video_set_sharpness(int id, int ch, u8 value)
{
    int ret;

    if(id >= RN6318_DEV_MAX || ch >= RN6318_DEV_CH_MAX)
        return -1;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0xff, RN6318_BUILD_IN_DEV_CH(ch));   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0x05, value);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_video_set_sharpness);

int rn6318_video_get_sharpness(int id, int ch)
{
    int ret;

    if(id >= RN6318_DEV_MAX || ch >= RN6318_DEV_CH_MAX)
        return -1;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0xff, RN6318_BUILD_IN_DEV_CH(ch));   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6318_i2c_read(id, RN6318_BUILD_IN_DEV_ID(ch), 0x05);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_video_get_sharpness);

int rn6318_status_get_novid(int id, int ch)
{
    int ret;

    if(id >= RN6318_DEV_MAX || ch >= RN6318_DEV_CH_MAX)
        return -1;

    ret = rn6318_i2c_write(id, RN6318_BUILD_IN_DEV_ID(ch), 0xff, RN6318_BUILD_IN_DEV_CH(ch));   ///< switch to the video decoder channel control page
    if(ret < 0)
        goto exit;

    ret = (rn6318_i2c_read(id, RN6318_BUILD_IN_DEV_ID(ch), 0x00)>>4) & 0x01;

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_status_get_novid);

int rn6318_audio_set_mode(u8 id, RN6318_AUDIO_SAMPLE_SIZE_T size, RN6318_AUDIO_SAMPLE_RATE_T rate)
{
    int idx, ret = 0;
    int dev_id;
    struct rn6318_audio_parameter ap; //audio parameter

    if (id >= RN6318_DEV_MAX)
        return -1;

    if (rn6318_audio_get_param(&ap)<0)
        goto exit;

    printk("%s(%d) id:%d sample_szie:%d sample_rate:%d chip_num:%d channel_num:%d chip_id:0x%x\n",
           __FUNCTION__,__LINE__,id,size,rate,ap.chip_num,ap.channel_num,ap.chip_id);
    
    rn6318_time_seq = ((ap.channel_num)==8)?((u8 *)&RN6318_AUDIO_SIN_TIMESEQ):((u8 *)&RN6318_AUDIO_DUO_TIMESEQ);
    rn6318_ch_data = ((ap.channel_num)==8)?((u8 *)rn6318_sin_ch_data[ap.chip_id]):((u8 *)rn6318_duo_ch_data[ap.chip_id]);
    rn6318_ch_data_def = ((ap.channel_num)==8)?((u8 *)rn6318_sin_ch_data_def[ap.chip_id]):
                                               ((u8 *)rn6318_duo_ch_data_def[ap.chip_id]);

    for (dev_id=0; dev_id<RN6318_DEV_BUILT_IN_DEV_MAX; dev_id++) {
        if ((ret=rn6318_i2c_write_table(id, dev_id, RN6318_AUDIO_COMMON_CFG, ARRAY_SIZE(RN6318_AUDIO_COMMON_CFG))) < 0)
            goto exit;

        for (idx=0; idx<(ap.channel_num); idx++) {
            //printk("rn6318 i2c write : %d.%d 0x%02x 0x%02x\n",id,dev_id,rn6318_time_seq[idx],rn6318_ch_data[idx]);
            if (rn6318_i2c_write(id,dev_id,rn6318_time_seq[idx],rn6318_ch_data[idx])<0)
                goto exit;
        }

        switch (id) {
            case 0:
                ret = rn6318_i2c_write(id, dev_id, 0x07, (dev_id==0)?(0x01):(0x00));     ///< enable audio playback interface in 16bit slave mode
                if(ret < 0)
                    goto exit;

                ret = rn6318_i2c_write(id, dev_id, 0x0c, (dev_id==0)?(0x71):(0x01));     ///< enable ADATR pin , 16bit master mode
                if(ret < 0)
                    goto exit;

                ret = rn6318_i2c_write(id, dev_id, 0x4e, 
                    (dev_id==0)?(0x08):((ap.channel_num==8)?(0x07):(0x0d))); ///< audio cascade first stage

                if(ret < 0)
                    goto exit;

                ret = rn6318_i2c_write(id, dev_id, 0x67, (dev_id==0)?(0x00):(0x40)); ///< set playback channel position
                if(ret < 0)
                    goto exit;
                    
                ret = rn6318_i2c_write(id, dev_id, 0x76, 0x14);     ///< DAC source is from mixer0
                if(ret < 0)
                    goto exit;
            
                if (ap.channel_num==8) {
                    ret = rn6318_i2c_write(id, dev_id, 0x0e, 0x08);
                    if(ret < 0)
                        goto exit;

                    ret = rn6318_i2c_write(id, dev_id, 0x09, 0x30);
                    if(ret < 0)
                        goto exit;
                }
            
                break;
            case 1:
                ret = rn6318_i2c_write(id, dev_id, 0x67, (dev_id==0)?(0x00):(0x40));
                if(ret < 0)
                    goto exit;

                if(ap.chip_num == 2)
                    ret = rn6318_i2c_write(id, dev_id, 0x4E, (dev_id==0)?(0x0e):(0x07));
                else
                    ret = rn6318_i2c_write(id, dev_id, 0x4E, 0x0d);
                if(ret < 0)
                    goto exit;

                ret = rn6318_i2c_write(id, dev_id, 0x07, 0x00);
                if(ret < 0)
                    goto exit;

                ret = rn6318_i2c_write(id, dev_id, 0x0c, 0x01);
                if(ret < 0)
                    goto exit;
                    
                break;
#if 0                
            case 2:
                ret = rn6318_i2c_write(id, dev_id, 0x67, 0x00);
                if(ret < 0)
                    goto exit;
                ret = rn6318_i2c_write(id, dev_id, 0x4E, 0x0E);      ///< audio cascade third stage
                if(ret < 0)
                    goto exit;
                break;
            case 3:
                ret = rn6318_i2c_write(id, dev_id, 0x67, 0x40);
                if(ret < 0)
                    goto exit;
                ret = rn6318_i2c_write(id, dev_id, 0x4E, 0x07);      ///< audio cascade third stage
                if(ret < 0)
                    goto exit;
                break;
#endif
            default:
                break;
        }
    }

    if (id==0) {
        rn6318_audio_set_playback_channel(id,0);
        rn6318_audio_set_bypass_channel(id,0xff);
    }

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_audio_set_mode);

int rn6318_audio_get_sample_size(int id)
{
    u8 tmp;

    if (id >= RN6318_DEV_MAX)
        return -1;

    if ((tmp=rn6318_i2c_read(id,0,0x0c))<0)
        return -1;

    tmp &= 0x02;
    tmp >>= 1;

    return (tmp==0)?(RN6318_AUDIO_SAMPLE_SIZE_16B):(RN6318_AUDIO_SAMPLE_SIZE_8B);
    
}
EXPORT_SYMBOL(rn6318_audio_get_sample_size);

int rn6318_audio_set_sample_size(int id, RN6318_AUDIO_SAMPLE_SIZE_T size)
{
    int ret = 0;
    int dev_id;
    u8  tmp;

    if(id >= RN6318_DEV_MAX || size >= RN6318_AUDIO_BITWIDTH_MAX)
        return -1;

    for(dev_id=0; dev_id<RN6318_DEV_BUILT_IN_DEV_MAX; dev_id++) {
        ret = rn6318_i2c_write(id, dev_id, 0xff, 0x04); ///< switch to the audio control page
        if(ret < 0)
            goto exit;

        tmp = rn6318_i2c_read(id, dev_id, 0x0c);
        tmp &= 0xfd;
        tmp |= (size<<1);
        ret = rn6318_i2c_write(id, dev_id, 0x0c, tmp);
        if(ret < 0)
            goto exit;
    }

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_audio_set_sample_size);

int rn6318_audio_get_sample_rate(int id)
{
    u8 val;
    int sr;

    if (id >= RN6318_DEV_MAX)
        return -1;

    if ((val=rn6318_i2c_read(id,0,0x00))<0)
        return -1;

    val&=0x07;

    switch (val) {
        case 0x1:
            sr=RN6318_AUDIO_SAMPLE_RATE_8K;
            break;
        case 0x2:
            sr=RN6318_AUDIO_SAMPLE_RATE_16K;
            break;
        case 0x3:
            sr=RN6318_AUDIO_SAMPLE_RATE_32K;
            break;
        case 0x4:
            sr=RN6318_AUDIO_SAMPLE_RATE_44K;
            break;
        case 0x5:
            sr=RN6318_AUDIO_SAMPLE_RATE_48K;
            break;
        default:
            sr=-1;
            break;
    }

    return sr;
    
}
EXPORT_SYMBOL(rn6318_audio_get_sample_rate);

int rn6318_audio_set_sample_rate(int id, RN6318_AUDIO_SAMPLE_RATE_T rate)
{
    int ret = 0;
    int tmp, val, dev_id;

    if(id >= RN6318_DEV_MAX || rate >= RN6318_AUDIO_SAMPLE_RATE_MAX)
        return -1;

    switch(rate) {
        case RN6318_AUDIO_SAMPLE_RATE_8K:
            val = 0x01;
            break;
        case RN6318_AUDIO_SAMPLE_RATE_16K:
            val = 0x02;
            break;
        case RN6318_AUDIO_SAMPLE_RATE_32K:
            val = 0x03;
            break;
        case RN6318_AUDIO_SAMPLE_RATE_44K:
            val = 0x04;
            break;
        case RN6318_AUDIO_SAMPLE_RATE_48K:
            val = 0x05;
            break;
        default:
            return -1;
    }

    for(dev_id=0; dev_id<RN6318_DEV_BUILT_IN_DEV_MAX; dev_id++) {
        ret = rn6318_i2c_write(id, dev_id, 0xff, 0x04); ///< switch to the audio control page
        if(ret < 0)
            goto exit;

        ret = rn6318_i2c_write(id, dev_id, 0x7a, 0x04); ///< 256 clock edges for a sample period, mask silent channel for audio mixer
        if(ret < 0)
            goto exit;

        tmp = rn6318_i2c_read(id, dev_id, 0x00);
        tmp &= ~0x07;
        tmp |= val;
        tmp |= (0x1 << 3);
        ret = rn6318_i2c_write(id, dev_id, 0x00, tmp); ///< free clock running. sample rate samplerate.
        if(ret < 0)
            goto exit;

        ret = rn6318_i2c_write(id, dev_id, 0x06, 0x00); ///< AUIDO_SDIV = 0 -> fASCLK = fAMCLK/1, fAMCLK = 256X samplerateKHZ
        if(ret < 0)
            goto exit;
    }

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_audio_set_sample_rate);

#if 1
int rn6318_audio_get_volume(int id, u8 ch)
{
    int ret=0, dev_id, channel;
    u8 tmp=0;

    if (id>=RN6318_DEV_MAX)
        return -1;

    dev_id = (ch<(RN6318_DEV_CH_MAX/2))?(0):(1);    
    channel = (ch<(RN6318_DEV_CH_MAX/2))?(ch):(ch-4);

    ret = rn6318_i2c_write(id, dev_id, 0xff, 0x04);         ///< switch to the audio control page
    if (ret<0)
        return -1;

    tmp = rn6318_i2c_read(id, dev_id, 0x5B+(channel/2));     ///< 0x5B:CH0/CH1, 0x5C:CH2/CH3
    tmp &= 0x0f<<(4*(channel%2));
    tmp >>= 4*(channel%2);

    return tmp;
}
EXPORT_SYMBOL(rn6318_audio_get_volume);

int rn6318_audio_set_volume(int id, u8 ch, u8 volume)
{
    int ret=0, dev_id, channel;
    u8 tmp=0;

    if (id>=RN6318_DEV_MAX)
        return -1;

    dev_id = (ch<(RN6318_DEV_CH_MAX/2))?(0):(1);    
    channel = (ch<(RN6318_DEV_CH_MAX/2))?(ch):(ch-4);

    ret = rn6318_i2c_write(id, dev_id, 0xff, 0x04);         ///< switch to the audio control page
    if (ret<0)
        return -1;

    tmp = rn6318_i2c_read(id, dev_id, 0x5B+(channel/2));     ///< 0x71:CH0/CH1, 0x72:CH2/CH3
    tmp &= ~(0xf<<(4*(channel%2)));
    tmp |= (volume<<(4*(channel%2)));
    ret = rn6318_i2c_write(id, dev_id, 0x5B+(channel/2), tmp);

    return ret;
}
EXPORT_SYMBOL(rn6318_audio_set_volume);

int rn6318_audio_get_playback_volume(int id)
{
    int ret=0,idx, dev_id=0;
    u8 tmp=0;

    if (id>=RN6318_DEV_MAX)
        return -1;

    if ((ret=rn6318_i2c_write(id, dev_id, 0xff, 0x04))<0)  ///< switch to the audio control page
        return -1;

    tmp = rn6318_i2c_read(id, dev_id, 0x5D);
    tmp &= 0x1f;

    for (idx=0; idx<RN6318_PLAYBACK_VOL_LEVEL; idx++)
        if (tmp==playback_vol_map[idx])
            break;

    return idx;
}
EXPORT_SYMBOL(rn6318_audio_get_playback_volume);

int rn6318_audio_set_playback_volume(int id, u8 volume)
{
    int ret, dev_id;
    u8 tmp;

    if (id>=RN6318_DEV_MAX || volume>0x0f)
        return -1;

    for (dev_id=0; dev_id<RN6318_DEV_BUILT_IN_DEV_MAX; dev_id++) {
        if ((ret=rn6318_i2c_write(id, dev_id, 0xff, 0x04))<0) ///< switch to the audio control page
            goto exit;

        tmp = rn6318_i2c_read(id, dev_id, 0x5D);
        tmp &= ~0x1f;
        tmp |= playback_vol_map[volume];
        
        if ((ret=rn6318_i2c_write(id, dev_id, 0x5D, tmp))<0)
            goto exit;
    }

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_audio_set_playback_volume);

#else
int rn6318_audio_get_volume(int id, u8 ch)
{
    int ret=0, dev_id=0;
    u8 tmp=0;

    if(id >= RN6318_DEV_MAX)
        return -1;

    ret = rn6318_i2c_write(id, dev_id, 0xff, 0x04);         ///< switch to the audio control page
    if(ret < 0)
        return -1;

    if (ch<4) {
        tmp = rn6318_i2c_read(id, dev_id, 0x71+(ch/2));     ///< 0x71:CH0/CH1, 0x72:CH2/CH3
        tmp &= 0x0f<<(4*(ch%2));
        tmp >>= 4*(ch%2);
    } else if (ch<6) {
        tmp = rn6318_i2c_read(id, dev_id, 0x73);            ///< PA0/PAB
        tmp &= 0xf<<(4*(ch%4));
        tmp >>= 4*(ch%4);
    } else if (ch<8) {
        tmp = rn6318_i2c_read(id, dev_id, 0x74);            ///< MIX/MIX40
        tmp &= 0xf<<(4*(ch%6));
        tmp >>= 4*(ch%6);
    }

    return tmp;

}
EXPORT_SYMBOL(rn6318_audio_get_volume);

int rn6318_audio_set_volume(int id, u8 ch, u8 volume)
{
    int ret = 0;
    int dev_id;
    u8  tmp;

    if(id >= RN6318_DEV_MAX || volume > 0x0f)
        return -1;

    for(dev_id=0; dev_id<RN6318_DEV_BUILT_IN_DEV_MAX; dev_id++) {
        ret = rn6318_i2c_write(id, dev_id, 0xff, 0x04);         ///< switch to the audio control page
        if(ret < 0)
            goto exit;

        if(ch < 4) {
            tmp = rn6318_i2c_read(id, dev_id, 0x71+(ch/2));     ///< 0x71:CH0/CH1, 0x72:CH2/CH3
            tmp &= ~(0xf<<(4*(ch%2)));
            tmp |= (volume<<(4*(ch%2)));
            ret = rn6318_i2c_write(id, dev_id, 0x71+(ch/2), tmp);
            if(ret < 0)
                goto exit;
        }
        else if(ch < 6) {
            tmp = rn6318_i2c_read(id, dev_id, 0x73);            ///< PA0/PAB
            tmp &= ~(0xf<<(4*(ch%4)));
            tmp |= (volume<<(4*(ch%4)));
            ret = rn6318_i2c_write(id, dev_id, 0x73, tmp);
            if(ret < 0)
                goto exit;
        }
        else if(ch < 8) {
            tmp = rn6318_i2c_read(id, dev_id, 0x74);            ///< MIX/MIX40
            tmp &= ~(0xf<<(4*(ch%6)));
            tmp |= (volume<<(4*(ch%6)));
            ret = rn6318_i2c_write(id, dev_id, 0x74, tmp);
            if(ret < 0)
                goto exit;
        }
    }

exit:
    return ret;
}
EXPORT_SYMBOL(rn6318_audio_set_volume);
#endif

int rn6318_audio_get_playback_channel(void)
{
    u8 val;
    int ch;
    struct rn6318_audio_parameter ap; //audio parameter

    if (rn6318_audio_get_param(&ap)<0)
        goto err;

    if ((val=rn6318_i2c_read(0,0,0x70))<0)
        goto err;

    if (val!=0xEF) {
        //printk("%s currently is not under playback mode!\n",__FUNCTION__);
        goto err;
    }

    if ((val=rn6318_i2c_read(0,0,0x67))<0)
        goto err;

    val&=0x1F;

    for (ch=0; ch<(ap.channel_num); ch++) {
        if (rn6318_ch_data_def[ch]==val)
            break;
    }

    return (ch>=(ap.channel_num))?(-1):(ch);
    
err:
    return -1;    
}
EXPORT_SYMBOL(rn6318_audio_get_playback_channel);

int rn6318_audio_set_playback_channel(int chipID, int pbCH)
{
    int devID=0;
    int ret=-1, setDefault=0;
    u8 val=0;
    struct rn6318_audio_parameter ap;
    
#if 0    
    u8 ch2seq[RN6318_AUDIO_CH_MAX]=
#if 0 // single 6318   
                                    {0x03,0x02,0x01,0x00,0x0B,0x0A,0x09,0x08};
#else // cascade 6318
                                    {0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00,
                                     0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08};
#endif
#endif

    if (rn6318_audio_get_param(&ap)<0)
        goto err;

    if (chipID >= RN6318_DEV_MAX)
        goto err;

    if ((pbCH<0)||(pbCH>=(ap.channel_num))) {
        printk("%s invalid pbCH(%d), set default!\n",__FUNCTION__,pbCH);
        setDefault=1;
    }

    //for (devID=0; devID<RN6318_DEV_BUILT_IN_DEV_MAX; devID++) {
        // Switch to audio control register set
        if ((ret=rn6318_i2c_write(chipID,devID,0xFF,0x04))<0)
            goto err;

        //Feed playback 0A data to mixer
        if ((ret=rn6318_i2c_write(chipID,devID,0x70,0xEF))<0)
            goto err;

        //Select time sequence slot
        val = rn6318_i2c_read(chipID,devID,0x67);
        val &= ~0x1f;
        val |= (setDefault==1)?(rn6318_ch_data_def[0]):(rn6318_ch_data_def[pbCH]);
        if ((ret=rn6318_i2c_write(chipID,devID,0x67,val))<0)
            goto err;
    //}

    return ret;
err:
    printk("%s(%d) err(%d)\n",__FUNCTION__,__LINE__,ret);
    return ret;
}
EXPORT_SYMBOL(rn6318_audio_set_playback_channel);

int rn6318_audio_get_bypass_channel(void)
{
    u8 val;

    if ((val=rn6318_i2c_read(0,0,0x70))<0)
        goto err;

    if (val==0xef) {
        //printk("currently is not under bypass mode!\n");
        goto err;
    }

    if ((val&0x0f)!=0x0f) {
        val=((~val)&0x0f)/2;
    } else { //extra mixer
        val=rn6318_i2c_read(0,0,0x75);
        val&=0x1F;        
    }

    return val;

err:
    return -1;    
}
EXPORT_SYMBOL(rn6318_audio_get_bypass_channel);

int rn6318_audio_set_bypass_channel(int chipID, int bpCH)
{
    int devID=0;
    int ret=-1, setDefault=0;

    if (chipID >= RN6318_DEV_MAX)
        goto err;

    if (((bpCH<0)||(bpCH>=RN6318_AUDIO_CH_MAX))&&(bpCH!=0xFF)) {
        printk("%s invalid bpCH(%d), set default!\n",__FUNCTION__,bpCH);
        setDefault=1;
    }

    //for (devID=0; devID<RN6318_DEV_BUILT_IN_DEV_MAX; devID++) {
        // Switch to audio control register set
        if ((ret=rn6318_i2c_write(chipID,devID,0xFF,0x04))<0)
            goto err;

        switch (bpCH) {
            case 0 ... 3:
                if ((ret=rn6318_i2c_write(chipID,devID,0x70,(u8)(~(1<<bpCH))))<0)
                    goto err;
                break;
            case 4 ... 15:
                if ((ret=rn6318_i2c_write(chipID,devID,0x70,0xBF))<0)
                    goto err;
                if ((ret=rn6318_i2c_write(chipID,devID,0x75,bpCH))<0)
                    goto err;
                break;
            #if 0
            case 8:     ///< ch40 linein
                if ((ret=rn6318_i2c_write(chipID,devID,0x70,(u8)(~0x80)))<0)
                    goto err;
                break;
            #endif            
            case 0xFF:    ///< 0xFF: playback 0A
                if((ret=rn6318_i2c_write(chipID,devID,0x70,(u8)(~0x10)))<0)
                    goto err;
                break;
            default:
                ret = -1;
                goto err;
        }
    //}
        
    return ret;
err:
    printk("%s(%d) err(%d)\n",__FUNCTION__,__LINE__,ret);
    return ret;
}
EXPORT_SYMBOL(rn6318_audio_set_bypass_channel);

