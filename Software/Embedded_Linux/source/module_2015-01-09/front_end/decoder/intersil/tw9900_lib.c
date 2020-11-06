/**
 * @file tw9900_lib.c
 * Intersil TW9900 1-CH 720H Video Decoders and Audio Codecs Library
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.2 $
 * $Date: 2013/10/31 05:20:27 $
 *
 * ChangeLog:
 *  $Log: tw9900_lib.c,v $
 *  Revision 1.2  2013/10/31 05:20:27  jerson_l
 *  1. setup video loss keep frame rate as 30fps in NTSC mode
 *
 *  Revision 1.1  2013/10/24 02:48:48  jerson_l
 *  1. support tw9900 decoder
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>

#include "intersil/tw9900.h"       ///< from module/include/front_end/decoder

static TW9900_VMODE_T tw9900_vmode[TW9900_DEV_MAX];  ///< for record device video mode

static unsigned char TW9900_NTSC[] = {
    0x06, 0x00,
    0x1a, 0x4f,
    0x02, 0x40,
    0x03, 0xa2,
    0x05, 0x01,
    0x07, 0x02,
    0x08, 0x13,
    0x09, 0xf2,
    0x0a, 0x13,
    0x19, 0x47,
    0x1a, 0x0f,
    0x1b, 0xc0,
    0x29, 0x03,
    0x33, 0x85,     ///< video loss force 30fps
    0x55, 0x00,
    0x6b, 0x26,
    0x6c, 0x36,
    0x6d, 0xF0,
    0x6e, 0x28,
    0x1a, 0x0f,
    0x06, 0x80,
};

static unsigned char TW9900_PAL[] = {
    0x06, 0x00,
    0x1a, 0x4f,
    0x02, 0x40,
    0x03, 0xa2,
    0x05, 0x01,
    0x07, 0x12,
    0x08, 0x19,
    0x09, 0x22,
    0x0a, 0x13,
    0x19, 0x57,
    0x1a, 0x0f,
    0x1b, 0xc0,
    0x29, 0x03,
    0x33, 0xc5,     ///< video loss force 25fps
    0x55, 0x00,
    0x6b, 0x26,
    0x6c, 0x36,
    0x6d, 0xF0,
    0x6e, 0x38,
    0x1a, 0x0f,
    0x06, 0x80,
};

static int tw9900_720h(int id, TW9900_VFMT_T vfmt)
{
    int i;
    int ret = 0;

    if(id >= TW9900_DEV_MAX)
        return -1;

    if(vfmt == TW9900_VFMT_PAL) {
        for(i=0; i<ARRAY_SIZE(TW9900_PAL)/2; i++) {
            ret = tw9900_i2c_write(id, TW9900_PAL[i*2], TW9900_PAL[(i*2)+1]);
            if(ret < 0)
                goto exit;
        }
    }
    else {
        for(i=0; i<ARRAY_SIZE(TW9900_NTSC)/2; i++) {
            ret = tw9900_i2c_write(id, TW9900_NTSC[i*2], TW9900_NTSC[(i*2)+1]);
            if(ret < 0)
                goto exit;
        }
    }

    printk("TW9900#%d ==> %s 720H 27MHz MODE!!\n", id, ((vfmt == TW9900_VFMT_PAL) ? "PAL" : "NTSC"));

exit:
    return ret;
}

int tw9900_video_set_mode(int id, TW9900_VMODE_T mode)
{
    int ret;

    if(id >= TW9900_DEV_MAX || mode >= TW9900_VMODE_MAX)
        return -1;

    switch(mode) {
        case TW9900_VMODE_NTSC_720H_1CH:
            ret = tw9900_720h(id, TW9900_VFMT_NTSC);
            break;
        case TW9900_VMODE_PAL_720H_1CH:
            ret = tw9900_720h(id, TW9900_VFMT_PAL);
            break;
        default:
            ret = -1;
            printk("TW9900#%d driver not support video output mode(%d)!!\n", id, mode);
            break;
    }

    /* Update VMode */
    if(ret == 0)
        tw9900_vmode[id] = mode;

    return ret;
}
EXPORT_SYMBOL(tw9900_video_set_mode);

int tw9900_video_get_mode(int id)
{
    if(id >= TW9900_DEV_MAX)
        return -1;

    return tw9900_vmode[id];
}
EXPORT_SYMBOL(tw9900_video_get_mode);

int tw9900_video_mode_support_check(int id, TW9900_VMODE_T mode)
{
    if(id >= TW9900_DEV_MAX)
        return 0;

    switch(mode) {
        case TW9900_VMODE_NTSC_720H_1CH:
        case TW9900_VMODE_PAL_720H_1CH:
            return 1;
        default:
            return 0;
    }

    return 0;
}
EXPORT_SYMBOL(tw9900_video_mode_support_check);

int tw9900_video_set_contrast(int id, u8 value)
{
    int ret;

    if(id >= TW9900_DEV_MAX)
        return -1;

    ret = tw9900_i2c_write(id, 0x11, value);

    return ret;
}
EXPORT_SYMBOL(tw9900_video_set_contrast);

int tw9900_video_get_contrast(int id)
{
    int ret;

    if(id >= TW9900_DEV_MAX)
        return -1;

    ret = tw9900_i2c_read(id, 0x11);

    return ret;
}
EXPORT_SYMBOL(tw9900_video_get_contrast);

int tw9900_video_set_brightness(int id, u8 value)
{
    int ret;

    if(id >= TW9900_DEV_MAX)
        return -1;

    ret = tw9900_i2c_write(id, 0x10, value);

    return ret;
}
EXPORT_SYMBOL(tw9900_video_set_brightness);

int tw9900_video_get_brightness(int id)
{
    int ret;

    if(id >= TW9900_DEV_MAX)
        return -1;

    ret = tw9900_i2c_read(id, 0x10);

    return ret;
}
EXPORT_SYMBOL(tw9900_video_get_brightness);

int tw9900_video_set_saturation_u(int id, u8 value)
{
    int ret;

    if(id >= TW9900_DEV_MAX)
        return -1;

    ret = tw9900_i2c_write(id, 0x13, value);

    return ret;
}
EXPORT_SYMBOL(tw9900_video_set_saturation_u);

int tw9900_video_get_saturation_u(int id)
{
    int ret;

    if(id >= TW9900_DEV_MAX)
        return -1;

    ret = tw9900_i2c_read(id, 0x13);

    return ret;
}
EXPORT_SYMBOL(tw9900_video_get_saturation_u);

int tw9900_video_set_saturation_v(int id, u8 value)
{
    int ret;

    if(id >= TW9900_DEV_MAX)
        return -1;

    ret = tw9900_i2c_write(id, 0x14, value);

    return ret;
}
EXPORT_SYMBOL(tw9900_video_set_saturation_v);

int tw9900_video_get_saturation_v(int id)
{
    int ret;

    if(id >= TW9900_DEV_MAX)
        return -1;

    ret = tw9900_i2c_read(id, 0x14);

    return ret;
}
EXPORT_SYMBOL(tw9900_video_get_saturation_v);

int tw9900_video_set_hue(int id, u8 value)
{
    int ret;

    if(id >= TW9900_DEV_MAX)
        return -1;

    ret = tw9900_i2c_write(id, 0x15, value);

    return ret;
}
EXPORT_SYMBOL(tw9900_video_set_hue);

int tw9900_video_get_hue(int id)
{
    int ret;

    if(id >= TW9900_DEV_MAX)
        return -1;

    ret = tw9900_i2c_read(id, 0x15);

    return ret;
}
EXPORT_SYMBOL(tw9900_video_get_hue);

int tw9900_status_get_novid(int id)
{
    int ret;

    if(id >= TW9900_DEV_MAX)
        return -1;

    ret = (tw9900_i2c_read(id, 0x01)>>7) & 0x1;

    return ret;
}
EXPORT_SYMBOL(tw9900_status_get_novid);
