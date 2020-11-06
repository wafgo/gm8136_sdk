/**
 * @file tw2865_lib.c
 * Intersil TW2865 4-CH 720H Video Decoders and Audio Codecs Library
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.3 $
 * $Date: 2014/07/22 11:59:03 $
 *
 * ChangeLog:
 *  $Log: tw2865_lib.c,v $
 *  Revision 1.3  2014/07/22 11:59:03  shawn_hu
 *  Fix the bug when setting the sharpness for Intersil TW2XXX decoders.
 *
 *  Revision 1.2  2014/07/22 03:09:14  shawn_hu
 *  1. add IOCTL and proc node for set sharpness and video mode
 *
 *  Revision 1.1.1.1  2013/07/25 09:29:13  jerson_l
 *  add front_end device driver
 *
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>

#include "intersil/tw2865.h"       ///< from module/include/front_end/decoder

#define REG_OFFSET_BASE     0xD0
/* AB => BA */
#define SWAP_2CHANNEL(x)    do { (x) = (((x) & 0xF0) >> 4 | ((x) & 0x0F) << 4); } while(0);
/* ABCD => DCBA */
#define SWAP_4CHANNEL(x)    do { (x) = (((x) & 0xFF00) >> 8 | ((x) & 0x00FF) << 8); } while(0);
/* ABCDEFGH => HGFEDCBA */
#define SWAP_8CHANNEL(x)    do { (x) = (((x) & 0xFF000000) >> 24 | ((x) & 0x00FF0000) >> 8 | ((x) & 0x0000FF00) << 8 | ((x) & 0x000000FF) << 24); } while(0);
#define REG_NUMBER(x)		((x)-REG_OFFSET_BASE)

static unsigned char TW2865_NTSC[] = { // Force NTSC & disable shadow regs at 0x0E~0x0F 
    //   0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F
/*00*/   0x00,0xfd,0x65,0x21,0x80,0x80,0x00,0x02,0x17,0xf0,0x0f,0xd0,0x00,0x10,0x08,0x01,
/*10*/   0x00,0xfd,0x65,0x21,0x80,0x80,0x00,0x02,0x17,0xf0,0x0f,0xd0,0x00,0x10,0x08,0x01,
/*20*/   0x00,0xfd,0x65,0x21,0x80,0x80,0x00,0x02,0x17,0xf0,0x0f,0xd0,0x00,0x10,0x08,0x01,
/*30*/   0x00,0xfd,0x65,0x21,0x80,0x80,0x00,0x02,0x17,0xf0,0x0f,0xd0,0x00,0x10,0x08,0x01
};

static unsigned char TW2865_PAL[] = { // Force PAL & disable shadow regs at 0x0E~0x0F
    //   0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F
/*00*/   0x00,0xfd,0x65,0x21,0x80,0x80,0x00,0x12,0x17,0x20,0x0c,0xd0,0x00,0x00,0x09,0x02,
/*10*/   0x00,0xfd,0x65,0x21,0x80,0x80,0x00,0x12,0x17,0x20,0x0c,0xd0,0x00,0x00,0x09,0x02,
/*20*/   0x00,0xfd,0x65,0x21,0x80,0x80,0x00,0x12,0x17,0x20,0x0c,0xd0,0x00,0x00,0x09,0x02,
/*30*/   0x00,0xfd,0x65,0x21,0x80,0x80,0x00,0x12,0x17,0x20,0x0c,0xd0,0x00,0x00,0x09,0x02
};

static unsigned char tbl_pg0_sfr2[1][20] = {
    {
     0x33, 0x33, 0x01, 0x10,    //...           0xD0~0xD3
     0x54, 0x76, 0x98, 0x32,    //...           0xD4~0xD7
     0xBA, 0xDC, 0xFE, 0xC1,    //...           0xD8~0xDB
     0x0F, 0x88, 0x88, 0x40,    //...           0xDC~0xDF // change AOGAIN to 4, DF=80 -> 40
     0x10, 0xC0, 0xAA, 0xAA     //...           0xE0~0xE3
    }
};

static int tw2865_write_table(int id, u8 addr, u8 *tbl_ptr, int tbl_cnt)
{
    int i, ret = 0;

    if(id >= TW2865_DEV_MAX)
        return -1;

    for(i=0; i<tbl_cnt; i++) {
        ret = tw2865_i2c_write(id, addr+i, tbl_ptr[i]);
        if(ret < 0)
            goto exit;
    }

exit:
    return ret;
}

static int tw2865_common_init(int id, TW2865_VFMT_T vfmt)
{
    int ret;

    if(id >= TW2865_DEV_MAX)
        return -1;

    ret = tw2865_i2c_write(id, 0xfc, 0xff); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x9c, 0x60); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xf9, 0x91); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xaf, 0x11); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xb0, 0x11); if(ret < 0) goto exit;

    if(vfmt == TW2865_VFMT_PAL) {
        ret = tw2865_i2c_write(id, 0x9d, 0x90); if(ret < 0) goto exit;
        ret = tw2865_i2c_write(id, 0x9e, 0x52); if(ret < 0) goto exit;
    }
    else {
        ret = tw2865_i2c_write(id, 0x9d, 0x8a); if(ret < 0) goto exit;
        ret = tw2865_i2c_write(id, 0x9e, 0x52); if(ret < 0) goto exit;
    }

exit:
    return ret;
}

static int tw2865_720h_1ch(int id, TW2865_VFMT_T vfmt)
{
    int ret = 0;

    if(id >= TW2865_DEV_MAX)
        return -1;

    /* soft-reset */
    ret = tw2865_i2c_write(id, 0x80, 0x3f);
    if(ret < 0)
        goto exit;

    /* inC27MHz_outD27MHz */
    ret = tw2865_i2c_write(id, 0x82, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xca, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xcb, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xfa, 0x40); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xfb, 0x6f); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6a, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6b, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6c, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6d, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x60, 0x15); if(ret < 0) goto exit;

    /* common init */
    ret = tw2865_common_init(id, vfmt); if(ret < 0) goto exit;

    if(vfmt == TW2865_VFMT_PAL)
        ret = tw2865_write_table(id, 0x00, TW2865_PAL, sizeof(TW2865_PAL));
    else
        ret = tw2865_write_table(id, 0x00, TW2865_NTSC, sizeof(TW2865_NTSC));
    if(ret < 0)
        goto exit;

    printk("TW2865#%d ==> %s 720H 27MHz MODE!!\n", id, ((vfmt == TW2865_VFMT_PAL) ? "PAL" : "NTSC"));

exit:
    return ret;
}

static int tw2865_720h_2ch(int id, TW2865_VFMT_T vfmt)
{
    int ret = 0;

    if(id >= TW2865_DEV_MAX)
        return -1;

    /* soft-reset */
    ret = tw2865_i2c_write(id, 0x80, 0x3f);
    if(ret < 0)
        goto exit;

    /* inC27MHz_outD54MHz */
    ret = tw2865_i2c_write(id, 0x82, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xca, 0x11); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xfa, 0x40); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xfb, 0x2f); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6a, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6b, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6c, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6d, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x60, 0x15); if(ret < 0) goto exit;

    /* common init */
    ret = tw2865_common_init(id, vfmt); if(ret < 0) goto exit;

    if(vfmt == TW2865_VFMT_PAL)
        ret = tw2865_write_table(id, 0x00, TW2865_PAL, sizeof(TW2865_PAL));
    else
        ret = tw2865_write_table(id, 0x00, TW2865_NTSC, sizeof(TW2865_NTSC));
    if(ret < 0)
        goto exit;

    printk("TW2865#%d ==> %s 720H 54MHz MODE!!\n", id, ((vfmt == TW2865_VFMT_PAL) ? "PAL" : "NTSC"));

exit:
    return ret;
}

static int tw2865_720h_4ch(int id, TW2865_VFMT_T vfmt)
{
    int ret = 0;

    if(id >= TW2865_DEV_MAX)
        return -1;

    /* soft-reset */
    ret = tw2865_i2c_write(id, 0x80, 0x3f);
    if(ret < 0)
        goto exit;

    /* inC27MHz_outD108MHz */
    ret = tw2865_i2c_write(id, 0x82, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xca, 0x02); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xcb, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xfa, 0x4a); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xfb, 0x2f); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6a, 0x0a); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6b, 0x0a); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6c, 0x0a); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6d, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x60, 0x15); if(ret < 0) goto exit;

    /* common init */
    ret = tw2865_common_init(id, vfmt); if(ret < 0) goto exit;

    if(vfmt == TW2865_VFMT_PAL)
        ret = tw2865_write_table(id, 0x00, TW2865_PAL, sizeof(TW2865_PAL));
    else
        ret = tw2865_write_table(id, 0x00, TW2865_NTSC, sizeof(TW2865_NTSC));
    if(ret < 0)
        goto exit;

    printk("TW2865#%d ==> %s 720H 108MHz MODE!!\n", id, ((vfmt == TW2865_VFMT_PAL) ? "PAL" : "NTSC"));

exit:
    return ret;
}

static int tw2865_cif_4ch(int id, TW2865_VFMT_T vfmt)
{
    int ret = 0;

    if(id >= TW2865_DEV_MAX)
        return -1;

    /* soft-reset */
    ret = tw2865_i2c_write(id, 0x80, 0x3f);
    if(ret < 0)
        goto exit;

    /* inC27MHz_outD54MHz_4CIF */
    ret = tw2865_i2c_write(id, 0x82, 0x20); if(ret < 0) goto exit;  ///< 54MHz clock output rising edge
    ret = tw2865_i2c_write(id, 0xca, 0x55); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xcb, 0x0f); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xfa, 0x45); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0xfb, 0x2f); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6a, 0x05); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6b, 0x05); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6c, 0x05); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x6d, 0x00); if(ret < 0) goto exit;
    ret = tw2865_i2c_write(id, 0x60, 0x15); if(ret < 0) goto exit;

    /* common init */
    ret = tw2865_common_init(id, vfmt); if(ret < 0) goto exit;

    if(vfmt == TW2865_VFMT_PAL)
        ret = tw2865_write_table(id, 0x00, TW2865_PAL, sizeof(TW2865_PAL));
    else
        ret = tw2865_write_table(id, 0x00, TW2865_NTSC, sizeof(TW2865_NTSC));
    if(ret < 0)
        goto exit;

    printk("TW2865#%d ==> %s 4CIF 54MHz MODE!!\n", id, ((vfmt == TW2865_VFMT_PAL) ? "PAL" : "NTSC"));

exit:
    return ret;
}

int tw2865_video_set_mode(int id, TW2865_VMODE_T mode)
{
    int ret;

    if(id >= TW2865_DEV_MAX || mode >= TW2865_VMODE_MAX)
        return -1;

    switch(mode) {
        case TW2865_VMODE_NTSC_720H_1CH:
            ret = tw2865_720h_1ch(id, TW2865_VFMT_NTSC);
            break;
        case TW2865_VMODE_NTSC_720H_2CH:
            ret = tw2865_720h_2ch(id, TW2865_VFMT_NTSC);
            break;
        case TW2865_VMODE_NTSC_720H_4CH:
            ret = tw2865_720h_4ch(id, TW2865_VFMT_NTSC);
            break;
        case TW2865_VMODE_NTSC_CIF_4CH:
            ret = tw2865_cif_4ch(id, TW2865_VFMT_NTSC);
            break;
        case TW2865_VMODE_PAL_720H_1CH:
            ret = tw2865_720h_1ch(id, TW2865_VFMT_PAL);
            break;
        case TW2865_VMODE_PAL_720H_2CH:
            ret = tw2865_720h_2ch(id, TW2865_VFMT_PAL);
            break;
        case TW2865_VMODE_PAL_720H_4CH:
            ret = tw2865_720h_4ch(id, TW2865_VFMT_PAL);
            break;
        case TW2865_VMODE_PAL_CIF_4CH:
            ret = tw2865_cif_4ch(id, TW2865_VFMT_PAL);
            break;
        default:
            ret = -1;
            printk("TW2865#%d driver not support video output mode(%d)!!\n", id, mode);
            break;
    }

    return ret;
}
EXPORT_SYMBOL(tw2865_video_set_mode);

int tw2865_video_get_mode(int id)
{
    int ret;
    int is_ntsc;

    if(id >= TW2865_DEV_MAX)
        return -1;

    /* get ch#0 current register setting for determine device running mode */
    ret = (tw2865_i2c_read(id, 0x0E)&0x70)>>4;
    if(ret == 0x0) /* Force NTSC/PAL mode in TW2865_NTSC/PAL */
        is_ntsc = 1;
    else if(ret == 0x1)
        is_ntsc = 0;
    else {
        printk("Can't get current video mode (NTSC or PAL) of TW2865#%d!!\n", id);
        return -1;
    }

    if(is_ntsc) {
        ret = TW2865_VMODE_NTSC_720H_4CH;
    }
    else {
        ret = TW2865_VMODE_PAL_720H_4CH;
    }

    return ret;
}
EXPORT_SYMBOL(tw2865_video_get_mode);

int tw2865_video_mode_support_check(int id, TW2865_VMODE_T mode)
{
    if(id >= TW2865_DEV_MAX)
        return 0;

    switch(mode) {
        case TW2865_VMODE_NTSC_720H_1CH:
        case TW2865_VMODE_NTSC_720H_2CH:
        case TW2865_VMODE_NTSC_720H_4CH:
        case TW2865_VMODE_NTSC_CIF_4CH:
        case TW2865_VMODE_PAL_720H_1CH:
        case TW2865_VMODE_PAL_720H_2CH:
        case TW2865_VMODE_PAL_720H_4CH:
        case TW2865_VMODE_PAL_CIF_4CH:
            return 1;
        default:
            return 0;
    }

    return 0;
}
EXPORT_SYMBOL(tw2865_video_mode_support_check);

int tw2865_video_set_contrast(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2865_DEV_MAX || ch >= TW2865_DEV_CH_MAX)
        return -1;

    ret = tw2865_i2c_write(id, 0x02+(0x10*ch), value);

    return ret;
}
EXPORT_SYMBOL(tw2865_video_set_contrast);

int tw2865_video_get_contrast(int id, int ch)
{
    int ret;

    if(id >= TW2865_DEV_MAX || ch >= TW2865_DEV_CH_MAX)
        return -1;

    ret = tw2865_i2c_read(id, 0x02+(0x10*ch));

    return ret;
}
EXPORT_SYMBOL(tw2865_video_get_contrast);

int tw2865_video_set_brightness(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2865_DEV_MAX || ch >= TW2865_DEV_CH_MAX)
        return -1;

    ret = tw2865_i2c_write(id, 0x01+(0x10*ch), value);

    return ret;
}
EXPORT_SYMBOL(tw2865_video_set_brightness);

int tw2865_video_get_brightness(int id, int ch)
{
    int ret;

    if(id >= TW2865_DEV_MAX || ch >= TW2865_DEV_CH_MAX)
        return -1;

    ret = tw2865_i2c_read(id, 0x01+(0x10*ch));

    return ret;
}
EXPORT_SYMBOL(tw2865_video_get_brightness);

int tw2865_video_set_saturation_u(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2865_DEV_MAX || ch >= TW2865_DEV_CH_MAX)
        return -1;

    ret = tw2865_i2c_write(id, 0x04+(0x10*ch), value);

    return ret;
}
EXPORT_SYMBOL(tw2865_video_set_saturation_u);

int tw2865_video_get_saturation_u(int id, int ch)
{
    int ret;

    if(id >= TW2865_DEV_MAX || ch >= TW2865_DEV_CH_MAX)
        return -1;

    ret = tw2865_i2c_read(id, 0x04+(0x10*ch));

    return ret;
}
EXPORT_SYMBOL(tw2865_video_get_saturation_u);

int tw2865_video_set_saturation_v(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2865_DEV_MAX || ch >= TW2865_DEV_CH_MAX)
        return -1;

    ret = tw2865_i2c_write(id, 0x05+(0x10*ch), value);

    return ret;
}
EXPORT_SYMBOL(tw2865_video_set_saturation_v);

int tw2865_video_get_saturation_v(int id, int ch)
{
    int ret;

    if(id >= TW2865_DEV_MAX || ch >= TW2865_DEV_CH_MAX)
        return -1;

    ret = tw2865_i2c_read(id, 0x05+(0x10*ch));

    return ret;
}
EXPORT_SYMBOL(tw2865_video_get_saturation_v);

int tw2865_video_set_hue(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2865_DEV_MAX || ch >= TW2865_DEV_CH_MAX)
        return -1;

    ret = tw2865_i2c_write(id, 0x06+(0x10*ch), value);

    return ret;
}
EXPORT_SYMBOL(tw2865_video_set_hue);

int tw2865_video_get_hue(int id, int ch)
{
    int ret;

    if(id >= TW2865_DEV_MAX || ch >= TW2865_DEV_CH_MAX)
        return -1;

    ret = tw2865_i2c_read(id, 0x06+(0x10*ch));

    return ret;
}
EXPORT_SYMBOL(tw2865_video_get_hue);

int tw2865_video_set_sharpness(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2865_DEV_MAX || ch >= TW2865_DEV_CH_MAX)
        return -1;

    ret = (tw2865_i2c_read(id, 0x03+(0x10*ch)) & 0xF0);
    value = ret | (value & 0x0F);
    ret = tw2865_i2c_write(id, 0x03+(0x10*ch), value);

    return ret;
}
EXPORT_SYMBOL(tw2865_video_set_sharpness);

int tw2865_video_get_sharpness(int id, int ch)
{
    int ret;

    if(id >= TW2865_DEV_MAX || ch >= TW2865_DEV_CH_MAX)
        return -1;

    ret = tw2865_i2c_read(id, 0x03+(0x10*ch)) & 0x0F;

    return ret;
}
EXPORT_SYMBOL(tw2865_video_get_sharpness);

int tw2865_status_get_novid(int id, int ch)
{
    int ret;

    if(id >= TW2865_DEV_MAX || ch >= TW2865_DEV_CH_MAX)
        return -1;

    ret = (tw2865_i2c_read(id, 0x00+(0x10*ch))>>7) & 0x1;

    return ret;
}
EXPORT_SYMBOL(tw2865_status_get_novid);

int tw2865_swap_channels(int id, int num_of_ch)
{
    int reg, side, side_ch, ret=0;
    unsigned char tmp_memory[20];

    unsigned short *value16;
    unsigned int *value32;

    /* one codec consists of 4 channels */
    if (num_of_ch % 4)
        return -1;

    side_ch = num_of_ch / 2;    /* for each side */

    memcpy(tmp_memory, tbl_pg0_sfr2, 20 * sizeof(unsigned char));

    /* step 1, swap two registers. AB => BA */
    for (reg = REG_NUMBER(0xd3); reg <= REG_NUMBER(0xd7); reg++)
        SWAP_2CHANNEL(tmp_memory[reg]);

    /* step 2, re-arrange the mapping
     */
    /* right and left side */
    for (side = 0; side < 2; side++) {
        switch (side_ch) {
        case 4:            /* SDL = sample size * 4 = 32/64 bits */
            value16 = (unsigned short *)(&tmp_memory[REG_NUMBER(0xd3) + side * 4]); /* next will be 0xd7 */
            SWAP_4CHANNEL(*value16);
            break;
        case 8:            /* SDL = sample size * 8 = 64/128 bits */
            value32 = (unsigned int *)(&tmp_memory[REG_NUMBER(0xd3) + side * 4]);
            SWAP_8CHANNEL(*value32);
            break;
        }
    }

    /* update the register to codec */
    ret = tw2865_write_table(id, 0xD0, tmp_memory, sizeof(tbl_pg0_sfr2));

    return ret;
}

/* set sample rate */
int tw2865_audio_set_sample_rate(int id, TW2865_SAMPLERATE_T samplerate)
{
    int ret;
    int data;

    if (samplerate > TW2865_AUDIO_SAMPLERATE_48K) {
        printk("sample rate exceed tw2865 fs table \n");
        return -1;
    }

    data = tw2865_i2c_read(id, 0x70);
    data &= ~0xF;
    data |= 0x1 << 3;        /* AFAUTO */

    switch (samplerate) {
    default:
    case TW2865_AUDIO_SAMPLERATE_8K:
        data |= 0;
        break;
    case TW2865_AUDIO_SAMPLERATE_16K:
        data |= 1;
        break;
    case TW2865_AUDIO_SAMPLERATE_32K:
        data |= 2;
        break;
    case TW2865_AUDIO_SAMPLERATE_44K:
        data |= 3;
        break;
    case TW2865_AUDIO_SAMPLERATE_48K:
        data |= 4;
        break;
    }

    ret = tw2865_i2c_write(id, 0x70, data);

    return ret;
}

int tw2865_audio_set_sample_size(int id, TW2865_SAMPLESIZE_T samplesize)
{
    uint8_t data = 0;
    int ret = 0;

    if ((samplesize != TW2865_AUDIO_BITS_8B) && (samplesize != TW2865_AUDIO_BITS_16B)) {
        printk("%s fails: due to wrong sampling size = %d\n", __FUNCTION__, samplesize);
        return -1;
    }

    data = tw2865_i2c_read(id, 0xDF);
    data |= 0xF0;
    ret = tw2865_i2c_write(id, 0xDF, data); if(ret < 0) goto exit;

    data = tw2865_i2c_read(id, 0xDB);
    if (samplesize == TW2865_AUDIO_BITS_8B) {
        data = data | 0x04;
    } else if (samplesize == TW2865_AUDIO_BITS_16B) {
        data = data & (~0x04);
    }
    ret = tw2865_i2c_write(id, 0xDB, data); if(ret < 0) goto exit;

exit:
    return ret;
}

int tw2865_audio_init(int id, int total_channels)
{
    int ret =0;

#ifndef SSP_SLAVE_MODE
    tbl_pg0_sfr2[0][0xDB - REG_OFFSET_BASE] = 0xC2;     // TW2865 as the slave of I2S bus, 0xC1 if master.
#endif

    /* 4ch 16bit */
    ret = tw2865_swap_channels(id, total_channels); if(ret < 0) goto exit;

    /* default 8k fs*/
    ret = tw2865_audio_set_sample_rate(id, TW2865_AUDIO_SAMPLERATE_8K);

exit:
    return ret;
}

static int tw2865_audio_get_chnum(TW2865_VMODE_T mode)
{
    int ch_num = 0;

    ch_num = 4;

    return ch_num;
}

int tw2865_audio_set_mode(int id, TW2865_VMODE_T mode, TW2865_SAMPLESIZE_T samplesize, TW2865_SAMPLERATE_T samplerate)
{
    int ch_num = 0;
    int ret = 0;

    ch_num = tw2865_audio_get_chnum(mode);
    if(ch_num < 0){
        ret = -1;
        goto exit;
    }
    /* tw2865 audio initialize */
    ret = tw2865_audio_init(id, ch_num); if(ret < 0) goto exit;

    /* set audio sample rate */
    ret = tw2865_audio_set_sample_rate(id, samplerate); if(ret < 0) goto exit;

    /* set audio sample size */
    ret = tw2865_audio_set_sample_size(id, samplesize); if(ret < 0) goto exit;

exit:
    return ret;
}
EXPORT_SYMBOL(tw2865_audio_set_mode);
