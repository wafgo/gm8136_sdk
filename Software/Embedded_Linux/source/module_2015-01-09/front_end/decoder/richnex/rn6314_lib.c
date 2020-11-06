/**
 * @file rn6314_lib.c
 * RichNex RN6314 4-CH 960H/D1 Video Decoders and Audio Codecs Library
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.6 $
 * $Date: 2014/07/24 02:26:55 $
 *
 * ChangeLog:
 *  $Log: rn6314_lib.c,v $
 *  Revision 1.6  2014/07/24 02:26:55  andy_hsu
 *  add playback volume control
 *
 *  Revision 1.5  2014/07/22 12:03:19  shawn_hu
 *  Fix the bug when setting the sharpness of Richnex RN6314 decoder.
 *
 *  Revision 1.4  2014/07/22 07:02:53  shawn_hu
 *  1. add IOCTL and proc node for set sharpness and video mode
 *
 *  Revision 1.3  2014/07/21 11:43:49  andy_hsu
 *  Fix rn631x codec's audio driver issue
 *
 *  Revision 1.2  2013/12/31 02:45:42  andy_hsu
 *  add rn6314 and rn6318 audio driver
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

#include "richnex/rn6314.h"        ///< from module/include/front_end/decoder

static u8 playback_vol_map[]={0x01,0x03,0x05,0x07,0x09,0x0B,0x0D,0x0F,
                              0x11,0x13,0x15,0x17,0x19,0x1B,0x1D,0x1F};

static unsigned char RN6314_SYS_COMMON_CFG[] = {
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

static unsigned char RN6314_VIDEO_COMMON_REV_A_CFG[] = {
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

static unsigned char RN6314_VIDEO_COMMON_CFG[] = {
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

static unsigned char RN6314_AUDIO_COMMON_CFG[] = {
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
    0x0e, 0x08, //< I2S_STD ; 8 audio sounds in ADATR
    0x30, 0x80, //< set default
    0x31, 0x88, //< set default
    0x0a, 0x80, //< set default
    0x0b, 0x08, //< set default
    0x08, 0x7f, //< sample period of playback = 256
    0x09, 0x30, //< for 2 rn6314 cascading
#if 0
    0x20, 0x03,
    0x21, 0x02,
    0x22, 0x01,
    0x23, 0x00,
    0x28, 0x07, 
    0x29, 0x06,
    0x2a, 0x05,
    0x2b, 0x04
#endif    
};

// for single rn6134's audio channel data mapping
static unsigned char RN6314_AUDIO_SIN_CH_MAP[] = {
    0x20, 0x01,
    0x21, 0x00,
    0x28, 0x03,
    0x29, 0x02
};

// for duo rn6134's audio channel data mapping
static unsigned char RN6314_AUDIO_DUO_CH_MAP[] = {
    0x20, 0x03,
    0x21, 0x02,
    0x22, 0x01,
    0x23, 0x00,
    0x28, 0x07,
    0x29, 0x06,
    0x2A, 0x05,
    0x2B, 0x04
};

static unsigned char RN6314_PAL_720H_VIDEO_CFG[] = {
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

static unsigned char RN6314_NTSC_720H_VIDEO_CFG[] = {
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

static unsigned char RN6314_PAL_960H_VIDEO_CFG[] = {
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

static unsigned char RN6314_NTSC_960H_VIDEO_CFG[] = {
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

static int rn6314_get_product_id(int id)
{
    int pid;

    if(id >= RN6314_DEV_MAX)
        return 0;

    pid = (((rn6314_i2c_read(id, 0xfd)) | (rn6314_i2c_read(id, 0xfe)<<8)) & 0xfff);

    return pid;
}

static int rn6314_comm_init(int id)
{
    int ret = 0;

    if(id >= RN6314_DEV_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0x80, 0x31); if(ret < 0) goto exit;  ///< software reset
    mdelay(10);

    ret = rn6314_i2c_write(id, 0xd2, 0x85); if(ret < 0) goto exit;
    ret = rn6314_i2c_write(id, 0xd6, 0x37); if(ret < 0) goto exit;
    ret = rn6314_i2c_write(id, 0x80, 0x36); if(ret < 0) goto exit;
    mdelay(10);

    ret = rn6314_i2c_write(id, 0x80, 0x30); if(ret < 0) goto exit;
    mdelay(30);

    ret = rn6314_i2c_write_table(id, RN6314_SYS_COMMON_CFG, ARRAY_SIZE(RN6314_SYS_COMMON_CFG));
    if(ret < 0)
        goto exit;

exit:
    return ret;
}

int rn6314_video_set_mode(int id, RN6314_VMODE_T mode)
{
    int ret;
    int ch, pid;
    char mode_str[32];

    if(id >= RN6314_DEV_MAX || mode >= RN6314_VMODE_MAX)
        return -1;

    if(rn6314_video_mode_support_check(id, mode) == 0) {
        printk("RN6314#%d driver not support video output mode(%d)!!\n", id, mode);
        return -1;
    }

    /* system common init */
    ret = rn6314_comm_init(id);
    if(ret < 0)
        goto exit;

    pid = rn6314_get_product_id(id);

    for(ch=0; ch<RN6314_DEV_CH_MAX; ch++) {
        ret = rn6314_i2c_write(id, 0xff, ch);   ///< switch to the video decoder channel control page
        if(ret < 0)
            goto exit;

        /* video common init */
        if(pid == RN6314_DEV_REV_A_PID)
            ret = rn6314_i2c_write_table(id, RN6314_VIDEO_COMMON_REV_A_CFG, ARRAY_SIZE(RN6314_VIDEO_COMMON_REV_A_CFG));
        else
            ret = rn6314_i2c_write_table(id, RN6314_VIDEO_COMMON_CFG, ARRAY_SIZE(RN6314_VIDEO_COMMON_CFG));
        if(ret < 0)
            goto exit;

        /* video mode init */
        switch(mode) {
            case RN6314_VMODE_NTSC_720H_4CH:
                ret = rn6314_i2c_write_table(id, RN6314_NTSC_720H_VIDEO_CFG, ARRAY_SIZE(RN6314_NTSC_720H_VIDEO_CFG));
                if(ret < 0)
                    goto exit;
                sprintf(mode_str, "NTSC 720H 108MHz MODE");
                break;
            case RN6314_VMODE_PAL_720H_4CH:
                ret = rn6314_i2c_write_table(id, RN6314_PAL_720H_VIDEO_CFG, ARRAY_SIZE(RN6314_PAL_720H_VIDEO_CFG));
                if(ret < 0)
                    goto exit;
                sprintf(mode_str, "PAL 720H 108MHz MODE");
                break;
            case RN6314_VMODE_NTSC_960H_4CH:
                ret = rn6314_i2c_write_table(id, RN6314_NTSC_960H_VIDEO_CFG, ARRAY_SIZE(RN6314_NTSC_960H_VIDEO_CFG));
                if(ret < 0)
                    goto exit;
                sprintf(mode_str, "NTSC 960H 144MHz MODE");
                break;
            case RN6314_VMODE_PAL_960H_4CH:
                ret = rn6314_i2c_write_table(id, RN6314_PAL_960H_VIDEO_CFG, ARRAY_SIZE(RN6314_PAL_960H_VIDEO_CFG));
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
            case RN6314_VMODE_NTSC_720H_4CH:
            case RN6314_VMODE_PAL_720H_4CH:
                ret = rn6314_i2c_write(id, 0x50, 0x00);   ///< output interface is BT-656
                if(ret < 0)
                    goto exit;
                ret = rn6314_i2c_write(id, 0x63, 0x07);   ///< composite anti-aliasing filter control (Reg63 default: 09h)
                if(ret < 0)                               ///< Reg63[3:1]: bandwidth selection, Reg63[0]: enable/disable
                    goto exit;
                break;
            case RN6314_VMODE_NTSC_960H_4CH:
            case RN6314_VMODE_PAL_960H_4CH:
                ret = rn6314_i2c_write(id, 0x50, 0x01);   ///< output interface is BT-1302
                if(ret < 0)
                    goto exit;
                ret = rn6314_i2c_write(id, 0x63, 0x09);   ///< composite anti-aliasing filter control (Reg63 default: 09h)
                if(ret < 0)                               ///< Reg63[3:1]: bandwidth selection, Reg63[0]: enable/disable
                    goto exit;
                break;
            default:
                ret = -1;
                goto exit;
        }

        /* video loss output black screen */
        ret = rn6314_i2c_write(id, 0x1a, 0x83); ///< blue: 0x03, black: 0x83
        if(ret < 0)
            goto exit;

        /* embedded channel ID in SAV/EAV */
        ret = rn6314_i2c_write(id, 0x3a, 0x20);
        if(ret < 0)
            goto exit;

        /* channel id assign */
        ret = rn6314_i2c_write(id, 0x3f, (0x20|ch));
        if(ret < 0)
            goto exit;
    }

    /* video output port init */
    switch(mode) {
        case RN6314_VMODE_NTSC_720H_4CH:
        case RN6314_VMODE_PAL_720H_4CH:
            /* VPort1 */
            ret = rn6314_i2c_write(id, 0x8E, 0x02); ///< configure VP1[7:0] in 4CH pixel interleaved
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0x8F, 0x00); ///< 720H data stream on VP1[7:0]
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0x8D, 0x31); ///< enable VP1[7:0] un-scaled image (1X base clock) output; Drv = 12mA
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0x89, 0x02); ///< configure SCLK1B : base clock 27Mhz; output 4X base clock (108M)
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0x88, 0xC3); ///< enable SCLK1B. Drv =12mA . Inv clck.
            if(ret < 0)
                goto exit;

            /* VPort3 */
            ret = rn6314_i2c_write(id, 0x94, 0x02); ///< configure VP3[7:0] in 4CH pixel interleaved
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0x95, 0x00); ///< 720H data stream on VP3[7:0]
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0x93, 0x31); ///< enable VP3[7:0] un-scaled image (1X base clock) output; Drv = 12mA
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0xF7, 0x02); ///< configure SCLK3A : base clock 27Mhz; output 4X base clock (108M)
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0xF6, 0xC3); ///< enable SCLK3A. Drv =12mA . Inv clck.
            if(ret < 0)
                goto exit;
            break;
        case RN6314_VMODE_NTSC_960H_4CH:
        case RN6314_VMODE_PAL_960H_4CH:
            /* VPORT1 */
            ret = rn6314_i2c_write(id, 0x8E, 0x02); ///< configure VP1[7:0] in 4CH pixel interleaved
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0x8F, 0x80); ///< 960H data stream on VP1[7:0]
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0x8D, 0x31); ///< enable VP1[7:0] un-scaled image (1X base clock) output; Drv = 12mA
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0x89, 0x0a); ///< configure SCLK1B : base clock 36Mhz; output 4X base clock (144M)
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0x88, 0xC3); ///< enable SCLK1B. Drv =12mA . Inv clck.
            if(ret < 0)
                goto exit;

            /* VPORT3 */
            ret = rn6314_i2c_write(id, 0x94, 0x02); ///< configure VP3[7:0] in 4CH pixel interleaved
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0x95, 0x80); ///< 960H data stream on VP3[7:0]
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0x93, 0x31); ///< enable VP3[7:0] un-scaled image (1X base clock) output; Drv = 12mA
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0xF7, 0x0a); ///< configure SCLK3A : base clock 36Mhz; output 4X base clock (144M)
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0xF6, 0xC3); ///< enable SCLK3A. Drv =12mA . Inv clck.
            if(ret < 0)
                goto exit;
            break;
        default:
            ret = -1;
            goto exit;
    }

    ret = rn6314_i2c_write(id, 0x81, 0x3f);         ///< turn-on all video decoders & audio adc
    if(ret < 0)
        goto exit;

    printk("RN6314#%d ==> %s!!\n", id, mode_str);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_video_set_mode);

int rn6314_video_get_mode(int id)
{
    int ret;
    int is_ntsc;
    int is_960h;

    if(id >= RN6314_DEV_MAX)
        return -1;

    /* get ch#0 current register setting for determine device running mode */
    ret = rn6314_i2c_write(id, 0xff, 0x0);   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = (rn6314_i2c_read(id, 0x07) & 0x3);
    if(ret == 0x3)
        is_ntsc = 1;
    else if(ret == 0x2)
        is_ntsc = 0;
    else {
        printk("Can't get current video mode (NTSC or PAL) of RN6314#%d!!\n", id);
        return -1;
    }

    is_960h = ((rn6314_i2c_read(id, 0x50)&0x01) == 0x01) ? 1 : 0;

    if(is_ntsc) {
        if(is_960h)
            ret = RN6314_VMODE_NTSC_960H_4CH;
        else
            ret = RN6314_VMODE_NTSC_720H_4CH;
    }
    else {
        if(is_960h)
            ret = RN6314_VMODE_PAL_960H_4CH;
        else
            ret = RN6314_VMODE_PAL_720H_4CH;
    }

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_video_get_mode);

int rn6314_video_mode_support_check(int id, RN6314_VMODE_T mode)
{
    if(id >= RN6314_DEV_MAX)
        return 0;

    switch(mode) {
        case RN6314_VMODE_NTSC_720H_4CH:
        case RN6314_VMODE_NTSC_960H_4CH:
        case RN6314_VMODE_PAL_720H_4CH:
        case RN6314_VMODE_PAL_960H_4CH:
            return 1;
        default:
            return 0;
    }

    return 0;
}
EXPORT_SYMBOL(rn6314_video_mode_support_check);

int rn6314_video_set_brightness(int id, int ch, u8 value)
{
    int ret;

    if(id >= RN6314_DEV_MAX || ch >= RN6314_DEV_CH_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, ch);   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6314_i2c_write(id, 0x01, value);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_video_set_brightness);

int rn6314_video_get_brightness(int id, int ch)
{
    int ret;

    if(id >= RN6314_DEV_MAX || ch >= RN6314_DEV_CH_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, ch);   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6314_i2c_read(id, 0x01);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_video_get_brightness);

int rn6314_video_set_contrast(int id, int ch, u8 value)
{
    int ret;

    if(id >= RN6314_DEV_MAX || ch >= RN6314_DEV_CH_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, ch);   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6314_i2c_write(id, 0x02, value);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_video_set_contrast);

int rn6314_video_get_contrast(int id, int ch)
{
    int ret;

    if(id >= RN6314_DEV_MAX || ch >= RN6314_DEV_CH_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, ch);   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6314_i2c_read(id, 0x02);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_video_get_contrast);

int rn6314_video_set_saturation(int id, int ch, u8 value)
{
    int ret;

    if(id >= RN6314_DEV_MAX || ch >= RN6314_DEV_CH_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, ch);   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6314_i2c_write(id, 0x03, value);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_video_set_saturation);

int rn6314_video_get_saturation(int id, int ch)
{
    int ret;

    if(id >= RN6314_DEV_MAX || ch >= RN6314_DEV_CH_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, ch);   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6314_i2c_read(id, 0x03);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_video_get_saturation);

int rn6314_video_set_hue(int id, int ch, u8 value)
{
    int ret;

    if(id >= RN6314_DEV_MAX || ch >= RN6314_DEV_CH_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, ch);   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6314_i2c_write(id, 0x04, value);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_video_set_hue);

int rn6314_video_get_hue(int id, int ch)
{
    int ret;

    if(id >= RN6314_DEV_MAX || ch >= RN6314_DEV_CH_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, ch);   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6314_i2c_read(id, 0x04);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_video_get_hue);

int rn6314_video_set_sharpness(int id, int ch, u8 value)
{
    int ret;

    if(id >= RN6314_DEV_MAX || ch >= RN6314_DEV_CH_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, ch);   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6314_i2c_write(id, 0x05, value);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_video_set_sharpness);

int rn6314_video_get_sharpness(int id, int ch)
{
    int ret;

    if(id >= RN6314_DEV_MAX || ch >= RN6314_DEV_CH_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, ch);   ///< switch to the video decoder channel control page
    if(ret < 0)
         goto exit;

    ret = rn6314_i2c_read(id, 0x05);

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_video_get_sharpness);

int rn6314_status_get_novid(int id, int ch)
{
    int ret;

    if(id >= RN6314_DEV_MAX || ch >= RN6314_DEV_CH_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, ch);   ///< switch to the video decoder channel control page
    if(ret < 0)
        goto exit;

    ret = (rn6314_i2c_read(id, 0x00)>>4) & 0x01;

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_status_get_novid);


static inline int rn6314_audio_param_sanity_check(int dev_num, int audio_chnum)
{
    int ret=0;

    if ((dev_num<1)||(dev_num>2)) {
        printk("[RN6134_AU_DRV] error device number: %d\n",dev_num);
        ret=-1;
        goto out;
    }

    if ((audio_chnum!=RN6314_DEV_CH_MAX)&&(audio_chnum!=RN6314_AUDIO_CH_MAX)) {
        printk("[RN6134_AU_DRV] error audio channel number: %d\n",audio_chnum);
        ret=-1;
        goto out;
    }

    if ((audio_chnum%RN6314_DEV_CH_MAX)!=0) {
        printk("[RN6134_AU_DRV] error audio channel number: %d\n",audio_chnum);
        ret=-1;    
        goto out;
    }
    
    if ((dev_num==1)&&(audio_chnum>RN6314_DEV_CH_MAX)) {
        printk("[RN6134_AU_DRV] error audio channel number: %d\n",audio_chnum);
        ret=-1;
        goto out;
    }

    if ((dev_num==2)&&(audio_chnum>RN6314_AUDIO_CH_MAX)) {
        printk("[RN6134_AU_DRV] error audio channel number: %d\n",audio_chnum);
        ret=-1;
        goto out;
    }

out:
    return ret;
}

int rn6314_audio_set_mode(u8 id, RN6314_AUDIO_SAMPLE_SIZE_T size, RN6314_AUDIO_SAMPLE_RATE_T rate)
{
    int ret = 0;
    int chip_num = rn6314_audio_get_chip_num();
    int au_ch_num = rn6314_audio_get_channel_number();

    if(id >= RN6314_DEV_MAX)
        return -1;

    if (rn6314_audio_param_sanity_check(chip_num,au_ch_num)<0)
        return -1;

    ret = rn6314_i2c_write_table(id, RN6314_AUDIO_COMMON_CFG, ARRAY_SIZE(RN6314_AUDIO_COMMON_CFG));
    if(ret < 0)
        goto exit;

    // audio channel data mapping config
    ret = rn6314_i2c_write_table(id, (au_ch_num==RN6314_DEV_CH_MAX)?(RN6314_AUDIO_SIN_CH_MAP):(RN6314_AUDIO_DUO_CH_MAP),
          (au_ch_num==RN6314_DEV_CH_MAX)?(ARRAY_SIZE(RN6314_AUDIO_SIN_CH_MAP)):(ARRAY_SIZE(RN6314_AUDIO_DUO_CH_MAP)));
    if (ret < 0)
        goto exit;

    switch(id) {
        case 0:
            ret = rn6314_i2c_write(id, 0x07, 0x01);     ///< enable audio playback interface in 16bit slave mode
            if(ret < 0)
                goto exit;

            ret = rn6314_i2c_write(id, 0x0c, 0x71);     ///< enable ADATR pin , 16bit master mode
            if(ret < 0)
                goto exit;

            if (au_ch_num==RN6314_DEV_CH_MAX) ///< for 4D1  machine
                ret = rn6314_i2c_write(id, 0x4e, 0x00); ///< no cascade
            else
                ret = rn6314_i2c_write(id, 0x4e, 0x08); ///< audio cascade first stage
            if(ret < 0)
                goto exit;

            ret = rn6314_i2c_write(id, 0x67, 0x00);
            if(ret < 0)
                goto exit;

            ret = rn6314_i2c_write(id, 0x76, 0x14);     ///< DAC source is from mixer0
            if(ret < 0)
                goto exit;

            // The case that have only one rn6314
            if (au_ch_num==RN6314_DEV_CH_MAX) {
                ret = rn6314_i2c_write(id, 0x0e, 0x04);
                if (ret<0)
                    goto exit;
                
                ret = rn6314_i2c_write(id, 0x09, 0x10);
                if (ret<0)
                    goto exit;
            }

            break;
        case 1:
            ret = rn6314_i2c_write(id, 0x67, 0x00);
            if(ret < 0)
                goto exit;

            if(au_ch_num==RN6314_AUDIO_CH_MAX) ///< for 8D1  machine when used one rn6318a
                ret = rn6314_i2c_write(id, 0x4E, 0x07);  ///< audio cascade second stage
            else
                ret = rn6314_i2c_write(id, 0x4E, 0x0d);  ///< audio cascade second stage
            if(ret < 0)
                goto exit;

            ret = rn6314_i2c_write(id, 0x0c, 0x01);
            if(ret < 0)
                goto exit;
            break;
        case 2:
            ret = rn6314_i2c_write(id, 0x67, 0x00);
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0x4E, 0x0E);      ///< audio cascade third stage
            if(ret < 0)
                goto exit;
            break;
        case 3:
            ret = rn6314_i2c_write(id, 0x67, 0x40);
            if(ret < 0)
                goto exit;
            ret = rn6314_i2c_write(id, 0x4E, 0x07);      ///< audio cascade fourth stage
            if(ret < 0)
                goto exit;
            break;
    }

    if (id==0) {
    rn6314_audio_set_playback_channel(id,0);
        //rn6314_audio_set_bypass_channel(id,1);
    }

exit:
    return ret;
    
}
EXPORT_SYMBOL(rn6314_audio_set_mode);

int rn6314_audio_get_sample_size(int id)
{
    u8 tmp;

    if (id >= RN6314_DEV_MAX)
        return -1;

    if ((tmp=rn6314_i2c_read(id,0x0c))<0)
        return -1;

    tmp &= 0x02;
    tmp >>= 1;

    return (tmp==0)?(RN6314_AUDIO_SAMPLE_SIZE_16B):(RN6314_AUDIO_SAMPLE_SIZE_8B);
    
}
EXPORT_SYMBOL(rn6314_audio_get_sample_size);

int rn6314_audio_set_sample_size(int id, RN6314_AUDIO_SAMPLE_SIZE_T size)
{
    int ret;
    u8  tmp;

    if(id >= RN6314_DEV_MAX || size >= RN6314_AUDIO_BITWIDTH_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, 0x04); ///< switch to the audio control page
    if(ret < 0)
        goto exit;

    tmp = rn6314_i2c_read(id, 0x0c);
    tmp &= 0xfd;
    tmp |= (size<<1);

    ret = rn6314_i2c_write(id, 0x0c, tmp);
    if(ret < 0)
        goto exit;
exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_audio_set_sample_size);

int rn6314_audio_get_sample_rate(int id)
{
    u8 val;
    int sr;

    if (id >= RN6314_DEV_MAX)
        return -1;

    if ((val=rn6314_i2c_read(id,0x00))<0)
        return -1;

    val&=0x07;

    switch (val) {
        case 0x1:
            sr=RN6314_AUDIO_SAMPLE_RATE_8K;
            break;
        case 0x2:
            sr=RN6314_AUDIO_SAMPLE_RATE_16K;
            break;
        case 0x3:
            sr=RN6314_AUDIO_SAMPLE_RATE_32K;
            break;
        case 0x4:
            sr=RN6314_AUDIO_SAMPLE_RATE_44K;
            break;
        case 0x5:
            sr=RN6314_AUDIO_SAMPLE_RATE_48K;
            break;
        default:
            sr=-1;
            break;
    }

    return sr;
    
}
EXPORT_SYMBOL(rn6314_audio_get_sample_rate);

int rn6314_audio_set_sample_rate(int id, RN6314_AUDIO_SAMPLE_RATE_T rate)
{
    int tmp, val, ret;

    if(id >= RN6314_DEV_MAX || rate >= RN6314_AUDIO_SAMPLE_RATE_MAX)
        return -1;

    switch(rate) {
        case RN6314_AUDIO_SAMPLE_RATE_8K:
            val = 0x01;
            break;
        case RN6314_AUDIO_SAMPLE_RATE_16K:
            val = 0x02;
            break;
        case RN6314_AUDIO_SAMPLE_RATE_32K:
            val = 0x03;
            break;
        case RN6314_AUDIO_SAMPLE_RATE_44K:
            val = 0x04;
            break;
        case RN6314_AUDIO_SAMPLE_RATE_48K:
            val = 0x05;
            break;
        default:
            return -1;
    }

    ret = rn6314_i2c_write(id, 0xff, 0x04); ///< switch to the audio control page
    if(ret < 0)
        goto exit;

    ret = rn6314_i2c_write(id, 0x7a, 0x04); ///< 256 clock edges for a sample period, mask silent channel for audio mixer
    if(ret < 0)
        goto exit;

    tmp = rn6314_i2c_read(id, 0x00);
    tmp &= ~0x07;
    tmp |= val;
    tmp |= (0x1 << 3);
    ret = rn6314_i2c_write(id,  0x00, tmp); ///< free clock running. sample rate samplerate.
    if(ret < 0)
        goto exit;

    ret = rn6314_i2c_write(id, 0x06, 0x00); ///< AUIDO_SDIV = 0 -> fASCLK = fAMCLK/1, fAMCLK = 256X samplerateKHZ
    if(ret < 0)
        goto exit;

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_audio_set_sample_rate);

int rn6314_audio_get_volume(int id, u8 ch)
{
    int ret=0;
    u8 tmp=0;

    if(id >= RN6314_DEV_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, 0x04);         ///< switch to the audio control page
    if(ret < 0)
        return -1;

    if (ch<4) {
        tmp = rn6314_i2c_read(id, 0x5B+(ch/2));     ///< 0x5B:CH0/CH1, 0x5C:CH2/CH3
        tmp &= 0x0f<<(4*(ch%2));
        tmp >>= 4*(ch%2);
    } 
#if 0
    else if (ch<6) {
        tmp = rn6314_i2c_read(id, 0x73);            ///< PA0/PAB
        tmp &= 0xf<<(4*(ch%4));
        tmp >>= 4*(ch%4);
    } else if (ch<8) {
        tmp = rn6314_i2c_read(id, 0x74);            ///< MIX/MIX40
        tmp &= 0xf<<(4*(ch%6));
        tmp >>= 4*(ch%6);
    }
#endif

    return tmp;

}
EXPORT_SYMBOL(rn6314_audio_get_volume);

int rn6314_audio_set_volume(int id, u8 ch, u8 volume)
{
    int ret;
    u8  tmp;

    if(id >= RN6314_DEV_MAX || volume > 0x0f)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, 0x04);         ///< switch to the audio control page
    if(ret < 0)
        goto exit;

    if(ch < 4) {
        tmp = rn6314_i2c_read(id, 0x5B+(ch/2));     ///< 0x5B:CH0/CH1, 0x5C:CH2/CH3
        tmp &= ~(0xf<<(4*(ch%2)));
        tmp |= (volume<<(4*(ch%2)));
        ret = rn6314_i2c_write(id, 0x5B+(ch/2), tmp);
        if(ret < 0)
            goto exit;
    }
#if 0    
    else if(ch < 6) {
        tmp = rn6314_i2c_read(id, 0x73);            ///< PA0/PAB
        tmp &= ~(0xf<<(4*(ch%4)));
        tmp |= (volume<<(4*(ch%4)));
        ret = rn6314_i2c_write(id, 0x73, tmp);
        if(ret < 0)
            goto exit;
    }
    else if(ch < 8) {
        tmp = rn6314_i2c_read(id, 0x74);            ///< MIX/MIX40
        tmp &= ~(0xf<<(4*(ch%6)));
        tmp |= (volume<<(4*(ch%6)));
        ret = rn6314_i2c_write(id, 0x74, tmp);
        if(ret < 0)
            goto exit;
    }
#endif

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_audio_set_volume);

int rn6314_audio_get_playback_volume(int id)
{
    int ret=0,idx;
    u8 tmp=0;

    if (id >= RN6314_DEV_MAX)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, 0x04);         ///< switch to the audio control page
    if (ret<0)
        return -1;

    tmp = rn6314_i2c_read(id, 0x5D);
    tmp &= 0x1f;

    for (idx=0; idx<RN6314_PLAYBACK_VOL_LEVEL; idx++)
        if (tmp==playback_vol_map[idx])
            break;

    return idx;
}
EXPORT_SYMBOL(rn6314_audio_get_playback_volume);

int rn6314_audio_set_playback_volume(int id, u8 volume)
{
    int ret;
    u8  tmp;

    if (id>=RN6314_DEV_MAX || volume>0x0f)
        return -1;

    ret = rn6314_i2c_write(id, 0xff, 0x04);         ///< switch to the audio control page
    if (ret < 0)
        goto exit;

    tmp = rn6314_i2c_read(id, 0x5D);
    tmp &= ~0x1f;
    tmp |= playback_vol_map[volume];
    ret = rn6314_i2c_write(id, 0x5D, tmp);
    if(ret < 0)
        goto exit;

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_audio_set_playback_volume);

int rn6314_audio_get_playback_channel(void)
{
    u8 val;
    u8 ch2seq[RN6314_AUDIO_CH_MAX]={0x03,0x02,0x01,0x00,0x07,0x06,0x05,0x04};
    int ch;

    if ((val=rn6314_i2c_read(0,0x70))<0)
        goto err;

    if (val!=0xEF) {
        //printk("%s currently is not under playback mode!\n",__FUNCTION__);
        goto err;
    }

    if ((val=rn6314_i2c_read(0,0x67))<0)
        goto err;

    val&=0x1F;

    for (ch=0; ch<RN6314_AUDIO_CH_MAX; ch++) {
        if (ch2seq[ch]==val)
            break;
    }

    return (ch>=RN6314_AUDIO_CH_MAX)?(-1):(ch);
    
err:
    return -1;    
}
EXPORT_SYMBOL(rn6314_audio_get_playback_channel);

int rn6314_audio_set_playback_channel(int chipID, int pbCH)
{
    int ret=-1, setDefault=0;
#if 0
    u8 ch2seq[RN6314_AUDIO_CH_MAX]={0x03,0x02,0x01,0x00,0x07,0x06,0x05,0x04}; // cascade rn6314
    u8 ch2seq[RN6314_DEV_CH_MAX]={0x01,0x00,0x03,0x02}; // single rn6314
#else
    u8 ch2seq[RN6314_AUDIO_CH_MAX]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07}; // for coordinate with audio_drv
#endif
    u8 val=0;

    if (chipID >= RN6314_DEV_MAX)
        goto err;

    if ((pbCH<0)||(pbCH>=RN6314_AUDIO_CH_MAX)) {
        printk("%s invalid pbCH(%d), set default!\n",__FUNCTION__,pbCH);
        setDefault=1;
    }

    // Switch to audio control register set
    if ((ret=rn6314_i2c_write(chipID,0xFF,0x04))<0)
        goto err;

    // Feed playback 0A data to mixer
    if ((ret=rn6314_i2c_write(chipID,0x70,0xEF))<0)
        goto err;

    // Select time sequence slot
    val = ~(0x1f) & rn6314_i2c_read(chipID,0x67);
    val |= (setDefault==1)?(ch2seq[0]):(ch2seq[pbCH]);
    if ((ret=rn6314_i2c_write(chipID,0x67,val))<0)
        goto err;

    return ret;
err:
    printk("%s(%d) err(%d)\n",__FUNCTION__,__LINE__,ret);
    return ret;
}
EXPORT_SYMBOL(rn6314_audio_set_playback_channel);

int rn6314_audio_get_bypass_channel(void)
{
    u8 val;

    if ((val=rn6314_i2c_read(0,0x70))<0)
        goto err;

    if (val==0xef) {
        //printk("currently is not under bypass mode!\n");
        goto err;
    }

    if ((val&0x0f)!=0x0f) {
        val=((~val)&0x0f)/2;
    } else { //extra mixer
        val=rn6314_i2c_read(0,0x75);
        val&=0x1F;        
    }

    return val;

err:
    return -1;    
}
EXPORT_SYMBOL(rn6314_audio_get_bypass_channel);

int rn6314_audio_set_bypass_channel(int chipID, int bpCH)
{
    int ret=-1, setDefault=0;

    if (chipID >= RN6314_DEV_MAX)
        goto err;

    if ((bpCH<0)||(bpCH>=RN6314_AUDIO_CH_MAX)) {
        printk("%s invalid bpCH(%d), set default!\n",__FUNCTION__,bpCH);
        setDefault=1;
    }

    // Switch to audio control register set
    if ((ret=rn6314_i2c_write(chipID,0xFF,0x04))<0)
        goto err;

    switch (bpCH) {
        case 0 ... 3:
            if ((ret=rn6314_i2c_write(chipID,0x70,(u8)(~(1<<(bpCH)))))<0)
                goto err;
            break;
        case 4 ... 7:
            if ((ret=rn6314_i2c_write(chipID,0x70,0xBF))<0)
                goto err;
            if ((ret=rn6314_i2c_write(chipID,0x75,(bpCH)))<0)
                goto err;
            break;
        case 8:     ///< ch40 linein
            if ((ret=rn6314_i2c_write(chipID,0x70,(u8)(~0x80)))<0)
                goto err;
            break;
        case 0xFF:    ///< 0xFF: playback 0A
            if((ret=rn6314_i2c_write(chipID,0x70,(u8)(~0x10)))<0)
                goto err;
            break;
        default:
            ret = -1;
            goto err;
    }
        
    return ret;
err:
    printk("%s(%d) err(%d)\n",__FUNCTION__,__LINE__,ret);
    return ret;
}
EXPORT_SYMBOL(rn6314_audio_set_bypass_channel);

