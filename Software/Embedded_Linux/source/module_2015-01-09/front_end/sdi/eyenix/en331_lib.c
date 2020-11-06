/**
 * @file en331_lib.c
 * Eyenix EN331 1CH HD-SDI Video Decoder Library
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.2 $
 * $Date: 2014/12/25 08:27:40 $
 *
 * ChangeLog:
 *  $Log: en331_lib.c,v $
 *  Revision 1.2  2014/12/25 08:27:40  shawn_hu
 *  Fix the problem of no black image output after power-on.
 *
 *  Revision 1.1  2014/10/15 13:53:02  shawn_hu
 *  Add Eyenix EN331 1-CH SDI video decoder support.
 *  1. Support EX-SDI(270Mbps), HD-SDI(1.485Gbps) data rates.
 *  2. Support pattern generation (black image) output when no video input.
 *  3. Support auto-switch between EX-SDI, HD-SDI, and pattern generation output.
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>

#include "eyenix/en331.h"   ///< from module/include/front_end/sdi

static struct en331_video_fmt_t en331_video_fmt_info[EN331_VFMT_MAX] = {
    /* Idx                    Width   Height  Prog    FPS */
    {  EN331_VFMT_1080P24,   1920,   1080,   1,      24  },  ///< 1080P@24
    {  EN331_VFMT_1080P25,   1920,   1080,   1,      25  },  ///< 1080P@25
    {  EN331_VFMT_1080P30,   1920,   1080,   1,      30  },  ///< 1080P@30
    {  EN331_VFMT_1080I50,   1920,   1080,   0,      50  },  ///< 1080I@50
    {  EN331_VFMT_1080I60,   1920,   1080,   0,      60  },  ///< 1080I@60
    {  EN331_VFMT_720P24,    1280,   720,    1,      24  },  ///< 720P @24
    {  EN331_VFMT_720P25,    1280,   720,    1,      25  },  ///< 720P @25
    {  EN331_VFMT_720P30,    1280,   720,    1,      30  },  ///< 720P @30
    {  EN331_VFMT_720P50,    1280,   720,    1,      50  },  ///< 720P @50
    {  EN331_VFMT_720P60,    1280,   720,    1,      60  },  ///< 720P @60
};

static u32 EN331_COMM[] = { // initial table
    0x14,  0x0,       // Start of PLL activation
    0x17,  0x0,
    0x14,  0x2,
    0x14,  0x3,
    0x17,  0x2,
    0x17,  0x3,
    0x14,  0x0,
    0x17,  0x0,
    0x14,  0x2,
    0x14,  0x3,
    0x17,  0x2,
    0x17,  0x3,      // End of PLL activation
    0x64,  0x0,
    0x6A,  0x10,
    0x6F,  0x435E,
    0x74,  0x980,    // 10bit mode
    0x80,  0x0,
    0x81,  0x0,
    0x83,  0x88,
    0x9B,  0x0,
    0xAA,  0x1,
    0xAC,  0x140,
    0x11,  0xA0,
    0x12,  0x1BC,
    0x13,  0x0,
    0x15,  0xF032,
    0x16,  0x1376E,
    0x21,  0xF1,     // Set TCLK_SEL to rpck_cnt[0], or the input detection will not work
    0x22,  0x0,
    0x23,  0x32000,  // 10bit output
    0x24,  0x3704B3,
    0x27,  0x0,
    0x29,  0x0,
    0x2B,  0x0,
    0x62,  0x20001,
    0x6B,  0x488F,
    0x71,  0x8080,   // For output black image
    0x74,  0x980,    // 10bit mode
    0x78,  0x0,
    0xA2,  0x0,
    0x14,  0x0,
    0x17,  0x0,
    0x14,  0x2,
    0x14,  0x3,
    0x17,  0x2,
    0x17,  0x3,
};

static int en331_pre_output_mode[EN331_DEV_MAX] = {-1, -1, -1, -1}; // Previous output mode: -1 -> Unknown
static int en331_cur_output_mode[EN331_DEV_MAX] = {0, 0, 0, 0}; // Current output mode: 0 -> black image, 1 -> EX-SDI, 2 -> HD-SDI

int en331_status_get_video_loss(int id, int ch)
{
    int vlos = 1;

    if(id >= EN331_DEV_MAX || ch >= EN331_DEV_CH_MAX)
        return 1;

    vlos = (en331_i2c_read(id, 0x302) & 0x2) ? 1 : 0;
    return vlos;
}
EXPORT_SYMBOL(en331_status_get_video_loss);

int en331_status_get_video_input_format(int id, int ch, struct en331_video_fmt_t *vfmt)
{
    int ret = 0;
    u32 rsel_det0, h_width, v_height, ex_width, ex_inter;
    int fmt_idx;

    if(id >= EN331_DEV_MAX || ch >= EN331_DEV_CH_MAX || !vfmt || en331_status_get_video_loss(id, ch))
        return -1;

    rsel_det0 = en331_i2c_read(id, 0x123) & 0xC000;  // 0x123[15:14], EX 1.5G or 3G mode
    if(rsel_det0) { // EX-SDI mode
        en331_cur_output_mode[id] = 1;
        ex_width = en331_i2c_read(id, 0x134) & 0xFFF; // 0x134[11:0]
        ex_inter = en331_i2c_read(id, 0x134) & 0x2000 ? 1 : 0; // 0x134[13]

        if(ex_inter == 0) {
            if(ex_width == 0xF77)
                fmt_idx = EN331_VFMT_720P25; //EN331_VFMT_720P25
            else if(ex_width == 0xCE3)
                fmt_idx = EN331_VFMT_720P30; //EN331_VFMT_720P30
            else if(ex_width == 0x7BB)
                fmt_idx = EN331_VFMT_720P50; //EN331_VFMT_720P50
            else if(ex_width == 0x671)
                fmt_idx = EN331_VFMT_720P60; //EN331_VFMT_720P60
            else if(ex_width == 0xABD)
                fmt_idx = EN331_VFMT_1080P24; //EN331_VFMT_1080P24
            else if(ex_width == 0xAF4)
                fmt_idx = EN331_VFMT_1080P25; //EN331_VFMT_1080P25
            else if(ex_width == 0x897)
                fmt_idx = EN331_VFMT_1080P30; //EN331_VFMT_1080P30
            else { //Unknown video input format
                ret = -1;
                goto exit;
            }
        }
        else { // ex_inter == 1
            if(ex_width == 0xAF4)
                fmt_idx = EN331_VFMT_1080I50; //EN331_VFMT_1080I50
            else if(ex_width == 0x897)
                fmt_idx = EN331_VFMT_1080I60; //EN331_VFMT_1080I60
            /* Caution!!! 1080 50P (or 60P) can't be differentiated from 25p(30p) in EX-SDI */
            /*else if(ex_width == 0xAF4)
                fmt_idx = EN331_VFMT_1080P50; //EN331_VFMT_1080P50
            else if(ex_width == 0x897)
                fmt_idx = EN331_VFMT_1080P60; //EN331_VFMT_1080P60*/
            else { //Unknown video input format
                ret = -1;
                goto exit;
            }
        }
    }
    else { // HD-SDI mode
        en331_cur_output_mode[id] = 2;
        h_width = en331_i2c_read(id, 0x14C);
        v_height = en331_i2c_read(id, 0x14D);

        if(h_width == 0x101C)
            fmt_idx = EN331_VFMT_720P24; //EN331_VFMT_720P24
        else if(h_width == 0xF77)
            fmt_idx = EN331_VFMT_720P25; //EN331_VFMT_720P25
        else if(h_width == 0xCE3)
            fmt_idx = EN331_VFMT_720P30; //EN331_VFMT_720P30
        else if(h_width == 0x7BB)
            fmt_idx = EN331_VFMT_720P50; //EN331_VFMT_720P50
        else if(h_width == 0x671)
            fmt_idx = EN331_VFMT_720P60; //EN331_VFMT_720P60
        else if(h_width == 0xABD)
            fmt_idx = EN331_VFMT_1080P24; //EN331_VFMT_1080P24
        else if(h_width == 0xAF4) {
            if(v_height == 0x465)
                fmt_idx = EN331_VFMT_1080P25; //EN331_VFMT_1080P25
            else // v_height == 232 or 233
                fmt_idx = EN331_VFMT_1080I50; //EN331_VFMT_1080I50
        }
        else if(h_width == 0x897) {
            if(v_height == 0x465)
               fmt_idx = EN331_VFMT_1080P30; //EN331_VFMT_1080P30
            else // v_height == 232 or 233
               fmt_idx = EN331_VFMT_1080I60; //EN331_VFMT_1080I60
        }
        else { //Unknown video input format
            ret = -1;
            goto exit;
        }
    }

    vfmt->fmt_idx    = en331_video_fmt_info[fmt_idx].fmt_idx;
    vfmt->width      = en331_video_fmt_info[fmt_idx].width;
    vfmt->height     = en331_video_fmt_info[fmt_idx].height;
    vfmt->prog       = en331_video_fmt_info[fmt_idx].prog;
    vfmt->frame_rate = en331_video_fmt_info[fmt_idx].frame_rate;

exit:
    return ret;
}
EXPORT_SYMBOL(en331_status_get_video_input_format);

int en331_i2c_test(int id) {
    int ret, i;
    u32 result;

    for(i = 0 ; i < 100; i++) {
        ret = en331_i2c_write(id, 0x19, 0xffffffff);
        if(ret < 0)
            return ret;
        result = en331_i2c_read(id, 0x19);
        if(result != 0xffffffff) {
            printk("EN331#%d I2C test failed!!!\n", id);
            return -1;
        }
        ret = en331_i2c_write(id, 0x19, 0x0); // clear to 0x0
        if(ret < 0)
            return ret;
    }

    printk("EN331#%d I2C test pass!!!\n", id);
    return 0;
}

int en331_video_init(int id, EN331_VOUT_FORMAT_T vout_fmt)
{
    int ret;

    if(id >= EN331_DEV_MAX || vout_fmt >= EN331_VOUT_FORMAT_MAX)
        return -1;

    /* Video Common */
    if(vout_fmt == EN331_VOUT_FORMAT_BT1120) {
        ret = -1;
        printk("EN331#%d driver doesn't support BT1120 vout format!\n", id);
        goto exit;
    }
    else {
        ret = en331_i2c_write_table(id, &EN331_COMM[0], ARRAY_SIZE(EN331_COMM));
        if(ret < 0)
            goto exit;
    }
    /* Uncomment below line to perform EN331 I2C read/write test */
    //ret = en331_i2c_test(id);

exit:
    return ret;
}
EXPORT_SYMBOL(en331_video_init);

int en331_output_black_image(int id) // Output black image in 720P30 format
{
    int ret = 0;

    en331_cur_output_mode[id] = 0; // Set current output mode to black image

    if(en331_pre_output_mode[id] != en331_cur_output_mode[id]) {
        ret = en331_i2c_write(id, 0x12, 0x13C);
        if(ret < 0)
            goto exit;
        ret = en331_i2c_write(id, 0x13, 0x1033);
        if(ret < 0)
            goto exit;
        ret = en331_i2c_write(id, 0x15, 0xF5E);
        if(ret < 0)
            goto exit;
        ret = en331_i2c_write(id, 0x16, 0x360F);
        if(ret < 0)
            goto exit;
        ret = en331_i2c_write(id, 0x21, 0xF4);
        if(ret < 0)
            goto exit;
        ret = en331_i2c_write(id, 0x23, 0x12000);
        if(ret < 0)
            goto exit;
        ret = en331_i2c_write(id, 0x24, 0x3504B1);
        if(ret < 0)
            goto exit;
        ret = en331_i2c_write(id, 0x2B, 0x11000);
        if(ret < 0)
            goto exit;
        ret = en331_i2c_write(id, 0x72, 0x2810C);
        if(ret < 0)
            goto exit;

        en331_pre_output_mode[id] = en331_cur_output_mode[id]; 
    }
exit:
    return ret;
}
EXPORT_SYMBOL(en331_output_black_image);

int en331_output_normal(int id)
{
    int ret = 0;

    if(en331_pre_output_mode[id] != en331_cur_output_mode[id]) {
        if(en331_cur_output_mode[id] == 1) { // Settings for EX-SDI mode
            ret = en331_i2c_write(id, 0x12, 0x38);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x13, 0x1033);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x15, 0xf5e);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x16, 0x11722);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x21, 0xF3);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x23, 0x10000);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x24, 0x150471);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x27, 0x100);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x29, 0x4);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x2B, 0x43000);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x62, 0x21080);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x6B, 0x140f0);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x72, 0x20205);
            if(ret < 0)
                goto exit;
        }else { // Settings for HD-SDI mode, en331_cur_output_mode[id] == 2
            ret = en331_i2c_write(id, 0x12, 0x1BC);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x13, 0x0);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x15, 0x4F52);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x16, 0x1376E);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x21, 0xF1);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x23, 0x32000);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x24, 0x3704B3);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x27, 0x0);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x29, 0x0);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x2B, 0x0);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x62, 0x20001);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x6B, 0x488F);
            if(ret < 0)
                goto exit;
            ret = en331_i2c_write(id, 0x72, 0x20205);
            if(ret < 0)
                goto exit;
        }
        en331_pre_output_mode[id] = en331_cur_output_mode[id];
    }
exit:
    return ret;
}
EXPORT_SYMBOL(en331_output_normal);
