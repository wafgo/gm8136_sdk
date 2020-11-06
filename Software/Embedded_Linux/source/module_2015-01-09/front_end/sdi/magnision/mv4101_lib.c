/**
 * @file mv4101_lib.c
 * Magnision MV4101 Quad-Channel SDI Receiver Library
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.2 $
 * $Date: 2013/12/02 05:32:59 $
 *
 * ChangeLog:
 *  $Log: mv4101_lib.c,v $
 *  Revision 1.2  2013/12/02 05:32:59  jerson_l
 *  1. support vout select from different sdi input source
 *
 *  Revision 1.1.1.1  2013/10/16 07:15:25  jerson_l
 *  add magnision mv4101 sdi driver
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>

#include "magnision/mv4101.h"        ///< from module/include/front_end/sdi

#define MV4101_CH_ADDR_OFFSET       0x100

static unsigned short MV4101_RX_ANALOG_CFG[] = {
    0x0082, 0x6c28,
    0x008a, 0x0006,
    0x0088, 0x7777,
    0x0089, 0x5d01,
    0x0083, 0x2000,
    0x0083, 0x4231,
};

static unsigned short MV4101_RX_ANALOG_CFG_B[] = {
    0x0082, 0x6c28,
    0x008a, 0x12a6,
    0x0088, 0x777b,
    0x0089, 0x5d01,
    0x0083, 0x2000,
    0x0083, 0x4231,
};

static unsigned short MV4101_EQ_AB_MODE_A_CFG[] = {
    0x0083, 0x5215,
    0x0085, 0xef1f,
    0x0086, 0x36f1,
    0x0013, 0x0136,
    0x0013, 0x0116,
};

static unsigned short MV4101_EQ_AB_MODE_B_CFG[] = {
    0x0083, 0x5235,
    0x0085, 0x0000,
    0x0086, 0x3700,
    0x0013, 0x0136,
    0x0013, 0x0116,
};

static unsigned short MV4101_EQ_LINE_MODE_CFG_A[] = {
    0x0080, 0x0052,
    0x0080, 0x9050,
    0x0080, 0x9a50,
    0x0080, 0xfcc0,
    0x0080, 0xebc4,
    0x0080, 0xfbd4,
};

static unsigned short MV4101_EQ_LINE_MODE_CFG_A2[] = {
    0x0080, 0x0052,
    0x0080, 0x9050,
    0x0080, 0x9a50,
    0x0080, 0xdc80,
    0x0080, 0xfcc0,
    0x0080, 0xfbd4,
};

static unsigned short MV4101_EQ_LINE_MODE_CFG_B[] = {
    0x0080, 0x0052,
    0x0080, 0x9050,
    0x0080, 0xebc0,
    0x0080, 0xf7c0,
    0x0080, 0xebc4,
    0x0080, 0xfbd4,
};

static unsigned short MV4101_EQ_INPUT_CFG[]= {
    0x0027, 0x8000,
    0x0028, 0x0001,
    0x0090, 0x0052,
    0x0091, 0x9050,
    0x0092, 0x9a50,
    0x0093, 0xdc80,
    0x0094, 0xfcc0,
    0x0095, 0xebc0,
    0x0096, 0xe5c0,
    0x0097, 0xf4c0,
    0x0098, 0xfdc0,
    0x00b2, 0x00ff,
    0x00b4, 0x6c28,
    0x00b0, 0x03ff,
    0x00b0, 0x0859,
};

static unsigned short MV4101_EQ_INPUT_CFG_B2[]= {
    0x0028, 0x0001,
    0x0090, 0x0052,
    0x0091, 0x9050,
    0x0092, 0x9a50,
    0x0093, 0xfcc0,
    0x0094, 0xebc4,
    0x0095, 0xfbd4,
    0x0096, 0x0052,
    0x0097, 0x9050,
    0x0098, 0x9a50,
    0x0099, 0xfcc0,
    0x009a, 0xebc4,
    0x009b, 0xfbd4,
    0x00b2, 0x007f,
    0x00b8, 0x003f,
    0x003d, 0x03ff,
    0x00b4, 0x6c28,
    0x00b0, 0x0bff,
    0x00b0, 0x0bd9,
};

int mv4101_get_chip_type(int id)
{
    int chip_id;
    int type;

    if(id >= MV4101_DEV_MAX)
        return MV4101_CHIP_TYPE_UNKNOWN;

    chip_id = mv4101_reg_read(id, 0x024) & 0xf;

    switch(chip_id) {
        case 1:
        case 3:
            type = MV4101_CHIP_TYPE_A;
            break;
        case 7:
            type = MV4101_CHIP_TYPE_A2;
            break;
        case 2:
        case 6:
            type = MV4101_CHIP_TYPE_B;
            break;
        case 8:
            type = MV4101_CHIP_TYPE_B2;
            break;
        case 9:
            type = MV4101_CHIP_TYPE_C;
            break;
        default:
            type = MV4101_CHIP_TYPE_UNKNOWN;
            break;
    }

    return type;
}
EXPORT_SYMBOL(mv4101_get_chip_type);

int mv4101_init_chip(int id, MV4101_VOUT_FMT_T vout_fmt, MV4101_EQ_MODE_T eq_mode)
{
    int ret = 0;
    int i, j;
    int eq_auto_hard = 0;
    MV4101_CHIP_TYPE_T chip_type;

    if(id >= MV4101_DEV_MAX)
        return -1;

    chip_type = mv4101_get_chip_type(id);

    /* RX Analog Init */
    switch(chip_type) {
        case MV4101_CHIP_TYPE_A:
        case MV4101_CHIP_TYPE_A2:
        case MV4101_CHIP_TYPE_B2:
            for(i=0; i<MV4101_DEV_CH_MAX; i++) {
                for(j=0; j<ARRAY_SIZE(MV4101_RX_ANALOG_CFG); j+=2) {
                    mv4101_reg_write(id, MV4101_RX_ANALOG_CFG[j]+(MV4101_CH_ADDR_OFFSET*i), MV4101_RX_ANALOG_CFG[j+1]);
                }
            }
            break;

        case MV4101_CHIP_TYPE_B:
            for(i=0; i<MV4101_DEV_CH_MAX; i++) {
                for(j=0; j<ARRAY_SIZE(MV4101_RX_ANALOG_CFG_B); j+=2) {
                    mv4101_reg_write(id, MV4101_RX_ANALOG_CFG_B[j]+(MV4101_CH_ADDR_OFFSET*i), MV4101_RX_ANALOG_CFG_B[j+1]);
                }
            }
            break;
        default:
            printk("MV4101#%d Chip Type=%d not support!!\n", id, chip_type);
            ret = -1;
            goto exit;
    }

    if((chip_type >= MV4101_CHIP_TYPE_B) && (eq_mode == MV4101_EQ_MODE_HW_AUTO))
        eq_auto_hard = 1;

    /* Setup Video Output Format */
    if(vout_fmt == MV4101_VOUT_FMT_BT1120) {
        for(i=0; i<MV4101_DEV_CH_MAX; i++) {
            if(chip_type > MV4101_CHIP_TYPE_B)
                mv4101_reg_write(id, 0x0000+(MV4101_CH_ADDR_OFFSET*i), 0x78c0+(i*0x4));
            else
                mv4101_reg_write(id, 0x0000+(MV4101_CH_ADDR_OFFSET*i), 0x78c0);
        }
    }
    else {
        for(i=0; i<MV4101_DEV_CH_MAX; i++) {
            if(chip_type > MV4101_CHIP_TYPE_B)
                mv4101_reg_write(id, 0x0000+(MV4101_CH_ADDR_OFFSET*i), 0x78c1+(i*0x4));
            else
                mv4101_reg_write(id, 0x0000+(MV4101_CH_ADDR_OFFSET*i), 0x78c1);
        }
    }

    /* Init EQ Input */
    for(i=0; i<MV4101_DEV_CH_MAX; i++) {
        if(chip_type >= MV4101_CHIP_TYPE_B) {
            if(!eq_auto_hard) {
                /* Disable Hardware Auto Mode */
                mv4101_reg_write(id, 0x00b0+(MV4101_CH_ADDR_OFFSET*i), 0x0342);
            }
            else {
                if(chip_type == MV4101_CHIP_TYPE_B2) {
                    for(j=0; j<ARRAY_SIZE(MV4101_EQ_INPUT_CFG_B2); j+=2) {
                        mv4101_reg_write(id, MV4101_EQ_INPUT_CFG_B2[j]+(MV4101_CH_ADDR_OFFSET*i), MV4101_EQ_INPUT_CFG_B2[j+1]);
                    }
                }
                else {
                    for(j=0; j<ARRAY_SIZE(MV4101_EQ_INPUT_CFG); j+=2) {
                        mv4101_reg_write(id, MV4101_EQ_INPUT_CFG[j]+(MV4101_CH_ADDR_OFFSET*i), MV4101_EQ_INPUT_CFG[j+1]);
                    }
                }
            }
        }

        /* EQ Power Down */
        if(chip_type >= MV4101_CHIP_TYPE_B)
            mv4101_reg_write(id, 0x0080+(MV4101_CH_ADDR_OFFSET*i), 0x0001);
    }

    /* set EQ Mode */
    for(i=0; i<MV4101_DEV_CH_MAX; i++) {
        ret = mv4101_set_eq_mode(id, i, eq_mode);
        if(ret < 0)
            goto exit;
    }

exit:
    return ret;
}
EXPORT_SYMBOL(mv4101_init_chip);

int mv4101_status_get_video_format(int id, int ch)
{
    u32 data;
    int fmt;

    if((id >= MV4101_DEV_MAX) || (ch >= MV4101_DEV_CH_MAX))
        return MV4101_VFMT_UNKNOWN;

    data = mv4101_reg_read(id, 0x0004+(MV4101_CH_ADDR_OFFSET*ch));

    switch((data>>8)&0x3f) {
        case 0x0c:
            fmt = MV4101_VFMT_1080I60;
            break;

        case 0x0d:
            fmt = MV4101_VFMT_1080I59;
            break;

        case 0x0e:
            fmt = MV4101_VFMT_1080I50;
            break;

        case 0x0f:
            fmt = MV4101_VFMT_1080P30;
            break;

        case 0x10:
            fmt = MV4101_VFMT_1080P29;
            break;

        case 0x11:
            fmt = MV4101_VFMT_1080P25;
            break;

        case 0x12:
            fmt = MV4101_VFMT_1080P24;
            break;

        case 0x13:
            fmt = MV4101_VFMT_1080P23;
            break;

        case 0x19:
            fmt = MV4101_VFMT_720P60;
            break;

        case 0x1a:
            fmt = MV4101_VFMT_720P59;
            break;

        case 0x1b:
            fmt = MV4101_VFMT_720P50;
            break;

        case 0x1c:
            fmt = MV4101_VFMT_720P30;
            break;

        case 0x1d:
            fmt = MV4101_VFMT_720P29;
            break;

        case 0x1e:
            fmt = MV4101_VFMT_720P25;
            break;

        case 0x1f:
            fmt = MV4101_VFMT_720P24;
            break;

        case 0x20:
            fmt = MV4101_VFMT_720P23;
            break;

        default:
            fmt = MV4101_VFMT_UNKNOWN;
            break;
    }

    return fmt;
}
EXPORT_SYMBOL(mv4101_status_get_video_format);

int mv4101_status_get_flywheel_lock(int id, int ch)
{
    u32 tmp, mask;
    int lock = 0;

    if((id >= MV4101_DEV_MAX) || (ch >= MV4101_DEV_CH_MAX))
        return 0;

    /* unmask flywheel lock interrupt mask */
    mask = mv4101_reg_read(id, 0x0023+(MV4101_CH_ADDR_OFFSET*ch));
    tmp = mask & (~(0x1<<4));
    mv4101_reg_write(id, 0x0023+(MV4101_CH_ADDR_OFFSET*ch), tmp);

    /* clear flywheel lock interrupt status */
    mv4101_reg_write(id, 0x0022+(MV4101_CH_ADDR_OFFSET*ch), 0x10);

    ndelay(1);

    /* read flywheel lock status */
    tmp = mv4101_reg_read(id, 0x0022+(MV4101_CH_ADDR_OFFSET*ch));
    if(tmp & 0x10)
        lock = 1;

    /* restore mask config */
    mv4101_reg_write(id, 0x0023+(MV4101_CH_ADDR_OFFSET*ch), mask);

    return lock;
}
EXPORT_SYMBOL(mv4101_status_get_flywheel_lock);

int mv4101_status_get_video_loss(int id, int ch)
{
    int lock;

    if((id >= MV4101_DEV_MAX) || (ch >= MV4101_DEV_CH_MAX))
        return 0;

    lock = mv4101_status_get_flywheel_lock(id, ch);

    return ((lock == 0) ? 1 : 0);
}
EXPORT_SYMBOL(mv4101_status_get_video_loss);

int mv4101_set_eq_mode(int id, int ch, MV4101_EQ_MODE_T mode)
{
    int i, ret = 0;
    int ab_mode_cfg_size;
    u16 *pline_mode_cfg, *pab_mode_cfg;

    if((id >= MV4101_DEV_MAX) || (ch >= MV4101_DEV_CH_MAX) || mode >= MV4101_EQ_MODE_MAX)
        return -1;

    if(mode == MV4101_EQ_MODE_SW_AUTO || mode == MV4101_EQ_MODE_HW_AUTO)
        return 0;

    switch(mv4101_get_chip_type(id)) {
        case MV4101_CHIP_TYPE_A:
            pline_mode_cfg = MV4101_EQ_LINE_MODE_CFG_A;
            break;
        case MV4101_CHIP_TYPE_A2:
            pline_mode_cfg = MV4101_EQ_LINE_MODE_CFG_A2;
            break;
        case MV4101_CHIP_TYPE_B:
        default:
            pline_mode_cfg = MV4101_EQ_LINE_MODE_CFG_B;
            break;
    }

    if(mode >= MV4101_EQ_MODE_B_LINE_0) {
        pab_mode_cfg     = MV4101_EQ_AB_MODE_B_CFG;
        ab_mode_cfg_size = ARRAY_SIZE(MV4101_EQ_AB_MODE_B_CFG);
    }
    else {
        pab_mode_cfg     = MV4101_EQ_AB_MODE_A_CFG;
        ab_mode_cfg_size = ARRAY_SIZE(MV4101_EQ_AB_MODE_A_CFG);
    }

    /* set EQ AB Mode */
    for(i=0; i<ab_mode_cfg_size; i+=2)
        mv4101_reg_write(id, pab_mode_cfg[i]+(MV4101_CH_ADDR_OFFSET*ch), pab_mode_cfg[i+1]);

    mdelay(3);

    /* set flywheel reset */
    mv4101_reg_write(id, 0x0014+(MV4101_CH_ADDR_OFFSET*ch), 0x2600);

    /* set EQ line mode */
    if(pline_mode_cfg) {
        switch(mode) {
            case MV4101_EQ_MODE_A_LINE_1:
                mv4101_reg_write(id, pline_mode_cfg[2]+(MV4101_CH_ADDR_OFFSET*ch), pline_mode_cfg[3]);
                break;
            case MV4101_EQ_MODE_A_LINE_2:
                mv4101_reg_write(id, pline_mode_cfg[4]+(MV4101_CH_ADDR_OFFSET*ch), pline_mode_cfg[5]);
                break;
            case MV4101_EQ_MODE_A_LINE_3:
                mv4101_reg_write(id, pline_mode_cfg[6]+(MV4101_CH_ADDR_OFFSET*ch), pline_mode_cfg[7]);
                break;
            case MV4101_EQ_MODE_A_LINE_4:
                mv4101_reg_write(id, pline_mode_cfg[8]+(MV4101_CH_ADDR_OFFSET*ch), pline_mode_cfg[9]);
                break;
            case MV4101_EQ_MODE_A_LINE_5:
                mv4101_reg_write(id, pline_mode_cfg[10]+(MV4101_CH_ADDR_OFFSET*ch), pline_mode_cfg[11]);
                break;
            case MV4101_EQ_MODE_B_LINE_0:
                mv4101_reg_write(id, pline_mode_cfg[0]+(MV4101_CH_ADDR_OFFSET*ch), pline_mode_cfg[1]);
                break;
            case MV4101_EQ_MODE_B_LINE_1:
                mv4101_reg_write(id, pline_mode_cfg[2]+(MV4101_CH_ADDR_OFFSET*ch), pline_mode_cfg[3]);
                break;
            case MV4101_EQ_MODE_B_LINE_2:
                mv4101_reg_write(id, pline_mode_cfg[4]+(MV4101_CH_ADDR_OFFSET*ch), pline_mode_cfg[5]);
                break;
            case MV4101_EQ_MODE_B_LINE_3:
                mv4101_reg_write(id, pline_mode_cfg[6]+(MV4101_CH_ADDR_OFFSET*ch), pline_mode_cfg[7]);
                break;
            case MV4101_EQ_MODE_B_LINE_4:
                mv4101_reg_write(id, pline_mode_cfg[8]+(MV4101_CH_ADDR_OFFSET*ch), pline_mode_cfg[9]);
                break;
            case MV4101_EQ_MODE_B_LINE_5:
                mv4101_reg_write(id, pline_mode_cfg[10]+(MV4101_CH_ADDR_OFFSET*ch), pline_mode_cfg[11]);
                break;
            default:
            case MV4101_EQ_MODE_A_LINE_0:
                mv4101_reg_write(id, pline_mode_cfg[0]+(MV4101_CH_ADDR_OFFSET*ch), pline_mode_cfg[1]);
                break;
        }
    }

    /* clear flywheel reset */
    mv4101_reg_write(id, 0x0014+(MV4101_CH_ADDR_OFFSET*ch), 0x2200);

    return ret;
}
EXPORT_SYMBOL(mv4101_set_eq_mode);

int mv4101_get_vout_sdi_src(int id, MV4101_DEV_VOUT_T vout)
{
    int ret = 0;

    if((id >= MV4101_DEV_MAX) || (vout >= MV4101_DEV_VOUT_MAX))
        return 0;

    if(mv4101_get_chip_type(id) > MV4101_CHIP_TYPE_B)
        ret = (mv4101_reg_read(id, 0x0000+(MV4101_CH_ADDR_OFFSET*vout))>>2)&0x3;
    else
        ret = vout;

    return ret;
}
EXPORT_SYMBOL(mv4101_get_vout_sdi_src);

int mv4101_set_vout_sdi_src(int id, MV4101_DEV_VOUT_T vout, int sdi_ch)
{
    int ret = 0;
    u32 tmp;

    if((id >= MV4101_DEV_MAX) || (vout >= MV4101_DEV_VOUT_MAX) || (sdi_ch >= MV4101_DEV_CH_MAX))
        return -1;

    if(mv4101_get_chip_type(id) > MV4101_CHIP_TYPE_B) {
        tmp = mv4101_reg_read(id, 0x0000+(MV4101_CH_ADDR_OFFSET*vout));
        tmp &= ~(0x3<<2);
        tmp |= (sdi_ch<<2);
        ret = mv4101_reg_write(id, 0x0000+(MV4101_CH_ADDR_OFFSET*vout), tmp);
    }

    return ret;
}
EXPORT_SYMBOL(mv4101_set_vout_sdi_src);

int mv4101_audio_set_bit_width(int id, int ch, MV4101_AUDIO_BIT_WIDTH_T bit_width)
{
    int ret = 0;

    if((id >= MV4101_DEV_MAX) || (ch >= MV4101_DEV_CH_MAX))
        return -1;

    if(mv4101_get_chip_type(id) <= MV4101_CHIP_TYPE_A2) ///< A/A2 Rev not support
        return 0;

    switch(bit_width) {
        case MV4101_AUDIO_WIDTH_16BIT:
            ret = mv4101_reg_write(id, 0x0046+(MV4101_CH_ADDR_OFFSET*ch), 0x0203);
            break;
        case MV4101_AUDIO_WIDTH_20BIT:
            ret = mv4101_reg_write(id, 0x0046+(MV4101_CH_ADDR_OFFSET*ch), 0x0103);
            break;
        case MV4101_AUDIO_WIDTH_24BIT:
            ret = mv4101_reg_write(id, 0x0046+(MV4101_CH_ADDR_OFFSET*ch), 0x0003);
            break;
        case MV4101_AUDIO_WIDTH_BIT_AUTO:
            ret = mv4101_reg_write(id, 0x0046+(MV4101_CH_ADDR_OFFSET*ch), 0x0303);
            break;
        default:
            return -1;
    }

    return ret;
}
EXPORT_SYMBOL(mv4101_audio_set_bit_width);

int mv4101_audio_get_rate(int id, int ch)
{
    int rate, value;

    if((id >= MV4101_DEV_MAX) || (ch >= MV4101_DEV_CH_MAX))
        return 0;

    if(mv4101_get_chip_type(id) <= MV4101_CHIP_TYPE_A2) ///< A/A2 Rev not support
        return 0;

    value = mv4101_reg_read(id, 0x004b+(MV4101_CH_ADDR_OFFSET*ch));
    value = (value & 0xf)>>1;
    switch(value) {
        case 1:
            rate = MV4101_AUDIO_RATE_44100;
            break;
        case 2:
            rate = MV4101_AUDIO_RATE_32000;
            break;
        case 0:
        default:
            rate = MV4101_AUDIO_RATE_48000;
            break;
    }

    return rate;
}
EXPORT_SYMBOL(mv4101_audio_get_rate);

int mv4101_audio_set_mute(int id, int ch, MV4101_AUDIO_MUTE_T mute)
{
    int ret = 0;

    if((id >= MV4101_DEV_MAX) || (ch >= MV4101_DEV_CH_MAX))
        return -1;

    if(mv4101_get_chip_type(id) <= MV4101_CHIP_TYPE_A2) ///< A/A2 Rev not support
        return 0;

    switch(mute) {
        case MV4101_AUDIO_SOUND_MUTE_L:
            ret = mv4101_reg_write(id, 0x0040+(MV4101_CH_ADDR_OFFSET*ch), 0xc800);
            break;
        case MV4101_AUDIO_SOUND_MUTE_R:
            ret = mv4101_reg_write(id, 0x0040+(MV4101_CH_ADDR_OFFSET*ch), 0xc400);
            break;
        case MV4101_AUDIO_SOUND_MUTE_ALL:
            ret = mv4101_reg_write(id, 0x0040+(MV4101_CH_ADDR_OFFSET*ch), 0xd000);
            break;
        case MV4101_AUDIO_SOUND_MUTE_DEFAULT:
            ret = mv4101_reg_write(id, 0x0040+(MV4101_CH_ADDR_OFFSET*ch), 0xc000);
            break;
        default:
            return -1;
    }

    return ret;
}
EXPORT_SYMBOL(mv4101_audio_set_mute);
