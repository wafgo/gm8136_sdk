/**
 * @file tp2802_lib.c
 * TechPoint TP2802 4CH HD-TVI Video Decoder Library
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>

#include "techpoint/tp2802.h"   ///< from module/include/front_end/hdtvi

static struct tp2802_video_fmt_t tp2802_video_fmt_info[TP2802_VFMT_MAX] = {
    /* Idx                    Width   Height  Prog    FPS */
    {  TP2802_VFMT_720P60,    1280,   720,    1,      60  },  ///< 720P @60
    {  TP2802_VFMT_720P50,    1280,   720,    1,      50  },  ///< 720P @50
    {  TP2802_VFMT_1080P30,   1920,   1080,   1,      30  },  ///< 1080P@30
    {  TP2802_VFMT_1080P25,   1920,   1080,   1,      25  },  ///< 1080P@25
    {  TP2802_VFMT_720P30,    1280,   720,    1,      30  },  ///< 720P @30
    {  TP2802_VFMT_720P25,    1280,   720,    1,      25  },  ///< 720P @25
};

static const unsigned char TP2802_720P60[] = {
    0x15, 0x12,
    0x16, 0x16,
    0x17, 0x00,
    0x18, 0x19,
    0x19, 0xd0,
    0x1a, 0x25,
    0x1c, 0x06,
    0x1d, 0x72
};

static const unsigned char TP2802_720P50[] = {
    0x15, 0x12,
    0x16, 0x16,
    0x17, 0x00,
    0x18, 0x19,
    0x19, 0xd0,
    0x1a, 0x25,
    0x1c, 0x07,
    0x1d, 0xbc
};

static const unsigned char TP2802_720P30[] = {
    0x15, 0x12,
    0x16, 0x16,
    0x17, 0x00,
    0x18, 0x19,
    0x19, 0xd0,
    0x1a, 0x25,
    0x1c, 0x0c,
    0x1d, 0xe4
};

static const unsigned char TP2802_720P25[] = {
    0x15, 0x12,
    0x16, 0x16,
    0x17, 0x00,
    0x18, 0x19,
    0x19, 0xd0,
    0x1a, 0x25,
    0x1c, 0x0f,
    0x1d, 0x78
};

static const unsigned char TP2802_1080P30[] = {
    0x15, 0x02,
    0x16, 0xc6,
    0x17, 0x80,
    0x18, 0x29,
    0x19, 0x38,
    0x1a, 0x47,
    0x1c, 0x08,
    0x1d, 0x98
};

static const unsigned char TP2802_1080P25[] = {
    0x15, 0x02,
    0x16, 0xc6,
    0x17, 0x80,
    0x18, 0x29,
    0x19, 0x38,
    0x1a, 0x47,
    0x1c, 0x0a,
    0x1d, 0x50
};

static const unsigned char TP2802_COMM[] = {
    0x07, 0x0f,
    0x08, 0xe0,
    0x2f, 0x08,
    0x30, 0x48,
    0x31, 0xba,
    0x32, 0x2e,
    0x33, 0x90,
    0x38, 0xc0,
    0x39, 0xca,
    0x3a, 0x0a,
    0x3d, 0x20,
    0x3e, 0x02,
    0x42, 0x00,
    0x43, 0x12,
    0x44, 0x07,
    0x45, 0x49,
    0x4b, 0x10,
    0x4c, 0x32,
    0x4d, 0x0f,
    0x4e, 0xff,
    0x4f, 0x0f,
    0x50, 0x00,
    0x51, 0x0a,
    0x52, 0x0b,
    0x53, 0x1f,
    0x54, 0xfa,
    0x55, 0x27,
    0x7e, 0x2f,
    0x7f, 0x00,
    0x80, 0x07,
    0x81, 0x08,
    0x82, 0x04,
    0x83, 0x00,
    0x84, 0x00,
    0x85, 0x60,
    0x86, 0x10,
    0x87, 0x06,
    0x88, 0xbe,
    0x89, 0x39,
    0x8a, 0xa7,
    0xb3, 0xfa,
    0xb4, 0x1c,
    0xfc, 0x00,
    0xfd, 0x00
};

int tp2802_video_get_video_output_format(int id, int ch, struct tp2802_video_fmt_t *vfmt)
{
    int ret = 0;
    int fmt_idx;
    int npxl_value;

    if(id >= TP2802_DEV_MAX || ch >= TP2802_DEV_CH_MAX || !vfmt)
        return -1;

    ret = tp2802_i2c_write(id, 0x40, ch);   ///< Switch to Channel Control Page
    if(ret < 0)
        goto exit;

    npxl_value = (tp2802_i2c_read(id, 0x1c)<<8) | tp2802_i2c_read(id, 0x1d);

    switch(npxl_value) {
        case 0x0672:
            fmt_idx = TP2802_VFMT_720P60;
            break;
        case 0x07BC:
            fmt_idx = TP2802_VFMT_720P50;
            break;
        case 0x0898:
            fmt_idx = TP2802_VFMT_1080P30;
            break;
        case 0x0A50:
            fmt_idx = TP2802_VFMT_1080P25;
            break;
        case 0x0CE4:
            fmt_idx = TP2802_VFMT_720P30;
            break;
        case 0x0F78:
            fmt_idx = TP2802_VFMT_720P25;
            break;
        default:
            ret = -1;
            goto exit;
    }

    vfmt->fmt_idx    = tp2802_video_fmt_info[fmt_idx].fmt_idx;
    vfmt->width      = tp2802_video_fmt_info[fmt_idx].width;
    vfmt->height     = tp2802_video_fmt_info[fmt_idx].height;
    vfmt->prog       = tp2802_video_fmt_info[fmt_idx].prog;
    vfmt->frame_rate = tp2802_video_fmt_info[fmt_idx].frame_rate;

exit:
    return ret;
}
EXPORT_SYMBOL(tp2802_video_get_video_output_format);

int tp2802_video_set_video_output_format(int id, int ch, TP2802_VFMT_T vfmt, TP2802_VOUT_FORMAT_T vout_fmt)
{
    int i, ret;

    if(id >= TP2802_DEV_MAX || ch > TP2802_DEV_CH_MAX || vfmt >= TP2802_VFMT_MAX || vout_fmt >= TP2802_VOUT_FORMAT_MAX)
        return -1;

    ret = tp2802_i2c_write(id, 0x40, ch);   ///< Switch to Channel Control Page, 4 for all channel
    if(ret < 0)
        goto exit;

    if(vout_fmt == TP2802_VOUT_FORMAT_BT1120) {
        if(vfmt == TP2802_VFMT_1080P30 || vfmt == TP2802_VFMT_1080P25)
            ret = tp2802_i2c_write(id, 0x02, 0x00);
        else
            ret = tp2802_i2c_write(id, 0x02, 0x02);
    }
    else {
        if(vfmt == TP2802_VFMT_1080P30 || vfmt == TP2802_VFMT_1080P25)
            ret = tp2802_i2c_write(id, 0x02, 0xc0);
        else
            ret = tp2802_i2c_write(id, 0x02, 0xc2);
    }
    if(ret < 0)
        goto exit;

    switch(vfmt) {
        case TP2802_VFMT_720P60:
            for(i=0; i<ARRAY_SIZE(TP2802_720P60); i+=2) {
                ret = tp2802_i2c_write(id, TP2802_720P60[i], TP2802_720P60[i+1]);
                if(ret < 0)
                    goto exit;
            }
            break;
        case TP2802_VFMT_720P50:
            for(i=0; i<ARRAY_SIZE(TP2802_720P50); i+=2) {
                ret = tp2802_i2c_write(id, TP2802_720P50[i], TP2802_720P50[i+1]);
                if(ret < 0)
                    goto exit;
            }
            break;
        case TP2802_VFMT_1080P30:
            for(i=0; i<ARRAY_SIZE(TP2802_1080P30); i+=2) {
                ret = tp2802_i2c_write(id, TP2802_1080P30[i], TP2802_1080P30[i+1]);
                if(ret < 0)
                    goto exit;
            }
            break;
        case TP2802_VFMT_1080P25:
            for(i=0; i<ARRAY_SIZE(TP2802_1080P25); i+=2) {
                ret = tp2802_i2c_write(id, TP2802_1080P25[i], TP2802_1080P25[i+1]);
                if(ret < 0)
                    goto exit;
            }
            break;
        case TP2802_VFMT_720P30:
            for(i=0; i<ARRAY_SIZE(TP2802_720P30); i+=2) {
                ret = tp2802_i2c_write(id, TP2802_720P30[i], TP2802_720P30[i+1]);
                if(ret < 0)
                    goto exit;
            }
            break;
        case TP2802_VFMT_720P25:
            for(i=0; i<ARRAY_SIZE(TP2802_720P25); i+=2) {
                ret = tp2802_i2c_write(id, TP2802_720P25[i], TP2802_720P25[i+1]);
                if(ret < 0)
                    goto exit;
            }
            break;
        default:
            ret = -1;
            break;
    }

exit:
    return ret;
}
EXPORT_SYMBOL(tp2802_video_set_video_output_format);

int tp2802_status_get_video_loss(int id, int ch)
{
    int ret;
    int vlos = 1;

    if(id >= TP2802_DEV_MAX || ch >= TP2802_DEV_CH_MAX)
        return 1;

    ret = tp2802_i2c_write(id, 0x40, ch);   ///< Switch to Channel Control Page
    if(ret < 0)
        goto exit;

    vlos = (tp2802_i2c_read(id, 0x01)>>7) & 0x1;

exit:
    return vlos;
}
EXPORT_SYMBOL(tp2802_status_get_video_loss);

int tp2802_status_get_video_input_format(int id, int ch, struct tp2802_video_fmt_t *vfmt)
{
    int ret = 0;
    u8  status;
    int fmt_idx;

    if(id >= TP2802_DEV_MAX || ch >= TP2802_DEV_CH_MAX || !vfmt)
        return -1;

    ret = tp2802_i2c_write(id, 0x40, ch);   ///< Switch to Channel Control Page
    if(ret < 0)
        goto exit;

    status = tp2802_i2c_read(id, 0x04);
    if(status < 0x30) {
        fmt_idx = tp2802_i2c_read(id, 0x03) & 0x7;
        if(fmt_idx >= TP2802_VFMT_MAX) {
            ret = -1;
            goto exit;
        }
    }
    else {
        ret = -1;
        goto exit;
    }

    vfmt->fmt_idx    = tp2802_video_fmt_info[fmt_idx].fmt_idx;
    vfmt->width      = tp2802_video_fmt_info[fmt_idx].width;
    vfmt->height     = tp2802_video_fmt_info[fmt_idx].height;
    vfmt->prog       = tp2802_video_fmt_info[fmt_idx].prog;
    vfmt->frame_rate = tp2802_video_fmt_info[fmt_idx].frame_rate;

exit:
    return ret;
}
EXPORT_SYMBOL(tp2802_status_get_video_input_format);

int tp2802_video_init(int id, TP2802_VOUT_FORMAT_T vout_fmt)
{
    int i, ret;

    if(id >= TP2802_DEV_MAX || vout_fmt >= TP2802_VOUT_FORMAT_MAX)
        return -1;

    ret = tp2802_i2c_write(id, 0x40, 0x04);   ///< Switch to write all page
    if(ret < 0)
        goto exit;

    /* Video Common */
    if(vout_fmt == TP2802_VOUT_FORMAT_BT1120) {
        ret = -1;
        printk("TP2802#%d driver not support BT1120 vout format!\n", id);
        goto exit;
    }
    else {
        for(i=0; i<ARRAY_SIZE(TP2802_COMM); i+=2) {
            ret = tp2802_i2c_write(id, TP2802_COMM[i], TP2802_COMM[i+1]);
            if(ret < 0)
                goto exit;
        }
    }

    /* Video Standard */
    ret = tp2802_video_set_video_output_format(id, 4, TP2802_VFMT_720P30, vout_fmt);   ///< ch=4 for config all channel
    if(ret < 0)
        goto exit;

exit:
    return ret;
}
EXPORT_SYMBOL(tp2802_video_init);
