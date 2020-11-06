/**
 * @file tw2968_lib.c
 * Intersil TW2968 8-CH 960H/D1 Video Decoders and Audio Codecs Library
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.10 $
 * $Date: 2014/09/11 09:23:19 $
 *
 * ChangeLog:
 *  $Log: tw2968_lib.c,v $
 *  Revision 1.10  2014/09/11 09:23:19  jerson_l
 *  1. add delay for system reset
 *
 *  Revision 1.9  2014/07/29 02:08:36  jerson_l
 *  1. fix pixel clock not stable problem when do system reset
 *
 *  Revision 1.8  2014/07/22 11:59:03  shawn_hu
 *  Fix the bug when setting the sharpness for Intersil TW2XXX decoders.
 *
 *  Revision 1.7  2014/07/18 05:57:25  shawn_hu
 *  1. add IOCTL and proc node for set sharpness and video mode
 *
 *  Revision 1.6  2014/07/17 09:26:22  ccsu
 *  support tw2968 AIN51 function
 *
 *  Revision 1.5  2014/01/09 09:41:00  ccsu
 *  support tw2968 8 channel audio & 16 channel audio
 *
 *  Revision 1.4  2013/10/01 06:12:00  jerson_l
 *  1. switch to new channel mapping mechanism
 *
 *  Revision 1.3  2013/08/08 05:16:53  jerson_l
 *  1. support system board(1v1) channel mapping
 *
 *  Revision 1.2  2013/07/31 09:00:52  jerson_l
 *  1. fix 4ch mode vin_chid video/audio channel mapping issue
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

#include "intersil/tw2968.h"       ///< from module/include/front_end/decoder

extern int audio_chnum;

#define REG_OFFSET_BASE     0xD0
/* AB => BA */
#define SWAP_2CHANNEL(x)    do { (x) = (((x) & 0xF0) >> 4 | ((x) & 0x0F) << 4); } while(0);
/* ABCD => DCBA */
#define SWAP_4CHANNEL(x)    do { (x) = (((x) & 0xFF00) >> 8 | ((x) & 0x00FF) << 8); } while(0);
/* ABCDEFGH => HGFEDCBA */
#define SWAP_8CHANNEL(x)    do { (x) = (((x) & 0xFF000000) >> 24 | ((x) & 0x00FF0000) >> 8 | ((x) & 0x0000FF00) << 8 | ((x) & 0x000000FF) << 24); } while(0);
#define REG_NUMBER(x)		((x)-REG_OFFSET_BASE)

static unsigned char tbl_pg0_sfr2[1][20] = {
    {0x33, 0x33, 0x01, 0x10,    //...           0xD0~0xD3
     0x54, 0x76, 0x98, 0x32,    //...           0xD4~0xD7
     0xBA, 0xDC, 0xFE, 0xC1,    //...           0xD8~0xDB
     0x0F, 0x88, 0x88, 0x40,    //...           0xDC~0xDF // change AOGAIN to 4, DF=80 -> 40
     0x10, 0xC0, 0xAA, 0xAA     //...           0xE0~0xE3
     }
};

static int tw2968_write_table(int id, u8 addr, u8 *tbl_ptr, int tbl_cnt)
{
    int i, ret = 0;

    if(id >= TW2968_DEV_MAX)
        return -1;

    for(i=0; i<tbl_cnt; i++) {
        ret = tw2968_i2c_write(id, addr+i, tbl_ptr[i]);
        if(ret < 0)
            goto exit;
    }

exit:
    return ret;
}

static int tw2968_ntsc_common_init(int id)
{
    int ret;

    if(id >= TW2968_DEV_MAX)
        return -1;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0
    ret = tw2968_i2c_write(id, 0x08, 0x14); if(ret < 0) goto exit;  ///< VIN1
    ret = tw2968_i2c_write(id, 0x0A, 0x10); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x0E, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x18, 0x14); if(ret < 0) goto exit;  ///< VIN2
    ret = tw2968_i2c_write(id, 0x1A, 0x10); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x1E, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x28, 0x14); if(ret < 0) goto exit;  ///< VIN3
    ret = tw2968_i2c_write(id, 0x2A, 0x10); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x2E, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x38, 0x14); if(ret < 0) goto exit;  ///< VIN4
    ret = tw2968_i2c_write(id, 0x3A, 0x10); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x3E, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x68, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x69, 0xF0); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6A, 0xF0); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6B, 0xF0); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6C, 0xF0); if(ret < 0) goto exit;

    /* BANK 1 */
    ret = tw2968_i2c_write(id, 0x40, 0x01); if(ret < 0) goto exit;  ///< set to page#1
    ret = tw2968_i2c_write(id, 0x08, 0x14); if(ret < 0) goto exit;  ///< VIN5
    ret = tw2968_i2c_write(id, 0x0A, 0x10); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x0E, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x18, 0x14); if(ret < 0) goto exit;  ///< VIN6
    ret = tw2968_i2c_write(id, 0x1A, 0x10); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x1E, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x28, 0x14); if(ret < 0) goto exit;  ///< VIN7
    ret = tw2968_i2c_write(id, 0x2A, 0x10); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x2E, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x38, 0x14); if(ret < 0) goto exit;  ///< VIN8
    ret = tw2968_i2c_write(id, 0x3A, 0x10); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x3E, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x68, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x69, 0xF0); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6A, 0xF0); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6B, 0xF0); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6C, 0xF0); if(ret < 0) goto exit;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2968_i2c_write(id, 0x97, 0x85); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xFB, 0x2F); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x5B, 0x03); if(ret < 0) goto exit;  ///< 27MHz XTI
    ret = tw2968_i2c_write(id, 0xFC, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xDB, 0xC1); if(ret < 0) goto exit;  ///< audio
    ret = tw2968_i2c_write(id, 0xD2, 0x02); if(ret < 0) goto exit;  ///< audio
    ret = tw2968_i2c_write(id, 0xCF, 0x80); if(ret < 0) goto exit;  ///< audio
    ret = tw2968_i2c_write(id, 0xE0, 0x1f); if(ret < 0) goto exit;  ///< audio

exit:
    return ret;
}

static int tw2968_pal_common_init(int id)
{
    int ret;

    if(id >= TW2968_DEV_MAX)
        return -1;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0
    ret = tw2968_i2c_write(id, 0x08, 0x18); if(ret < 0) goto exit;  ///< VIN1
    ret = tw2968_i2c_write(id, 0x0A, 0x11); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x0E, 0x01); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x18, 0x18); if(ret < 0) goto exit;  ///< VIN2
    ret = tw2968_i2c_write(id, 0x1A, 0x11); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x1E, 0x01); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x28, 0x18); if(ret < 0) goto exit;  ///< VIN3
    ret = tw2968_i2c_write(id, 0x2A, 0x11); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x2E, 0x01); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x38, 0x18); if(ret < 0) goto exit;  ///< VIN4
    ret = tw2968_i2c_write(id, 0x3A, 0x11); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x3E, 0x01); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x68, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x69, 0xE6); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6A, 0xE6); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6B, 0xE6); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6C, 0xE6); if(ret < 0) goto exit;

    /* BANK 1 */
    ret = tw2968_i2c_write(id, 0x40, 0x01); if(ret < 0) goto exit;  ///< set to page#1
    ret = tw2968_i2c_write(id, 0x08, 0x18); if(ret < 0) goto exit;  ///< VIN5
    ret = tw2968_i2c_write(id, 0x0A, 0x11); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x0E, 0x01); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x18, 0x18); if(ret < 0) goto exit;  ///< VIN6
    ret = tw2968_i2c_write(id, 0x1A, 0x11); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x1E, 0x01); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x28, 0x18); if(ret < 0) goto exit;  ///< VIN7
    ret = tw2968_i2c_write(id, 0x2A, 0x11); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x2E, 0x01); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x38, 0x18); if(ret < 0) goto exit;  ///< VIN8
    ret = tw2968_i2c_write(id, 0x3A, 0x11); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x3E, 0x01); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x68, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x69, 0xE6); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6A, 0xE6); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6B, 0xE6); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6C, 0xE6); if(ret < 0) goto exit;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2968_i2c_write(id, 0x97, 0xC5); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xFB, 0x2F); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x5B, 0x03); if(ret < 0) goto exit;  ///< 27MHz XTI
    ret = tw2968_i2c_write(id, 0xFC, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xDB, 0xC1); if(ret < 0) goto exit;  ///< audio
    ret = tw2968_i2c_write(id, 0xD2, 0x02); if(ret < 0) goto exit;  ///< audio
    ret = tw2968_i2c_write(id, 0xCF, 0x80); if(ret < 0) goto exit;  ///< audio
    ret = tw2968_i2c_write(id, 0xE0, 0x1f); if(ret < 0) goto exit;  ///< audio

exit:
    return ret;
}

static int tw2968_ntsc_960h_4ch(int id)
{
    int ret;

    if(id >= TW2968_DEV_MAX)
        return -1;

    ret = tw2968_ntsc_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2968_i2c_write(id, 0xE7, 0xAA); if(ret < 0) goto exit;  ///< 960H 144MHz VD1/VD2/VD3/VD4

    ret = tw2968_i2c_write(id, 0x63, 0x10); if(ret < 0) goto exit;  ///< CH2NUM, CH1NUM
    ret = tw2968_i2c_write(id, 0x64, 0x32); if(ret < 0) goto exit;  ///< CH4NUM, CH3NUM
    ret = tw2968_i2c_write(id, 0x65, 0x54); if(ret < 0) goto exit;  ///< CH6NUM, CH5NUM
    ret = tw2968_i2c_write(id, 0x66, 0x76); if(ret < 0) goto exit;  ///< CH8NUM, CH7NUM

    ret = tw2968_i2c_write(id, 0xE8, 0x10); if(ret < 0) goto exit;  ///< VD1
    ret = tw2968_i2c_write(id, 0xE9, 0x32); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xEA, 0x54); if(ret < 0) goto exit;  ///< VD2
    ret = tw2968_i2c_write(id, 0xEB, 0x76); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xEC, 0x10); if(ret < 0) goto exit;  ///< VD3
    ret = tw2968_i2c_write(id, 0xED, 0x32); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xEE, 0x54); if(ret < 0) goto exit;  ///< VD4
    ret = tw2968_i2c_write(id, 0xEF, 0x76); if(ret < 0) goto exit;

    ret = tw2968_i2c_write(id, 0xCC, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xCD, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xF9, 0x03); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xFA, 0x4A); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6F, 0xF0); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9E, 0x52); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9F, 0x00); if(ret < 0) goto exit;  ///< Clock Output Delay 0ns

    /* Disable -> Enable PLL for system reset VD clock not stable problem */
    ret = tw2968_i2c_write(id, 0x61, 0xFF); if(ret < 0) goto exit;
    udelay(5);
    ret = tw2968_i2c_write(id, 0x61, 0x1F); if(ret < 0) goto exit;

    printk("TW2968#%d ==> NTSC 960H 144MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2968_ntsc_720h_4ch(int id)
{
    int ret;

    if(id >= TW2968_DEV_MAX)
        return -1;

    ret = tw2968_ntsc_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2968_i2c_write(id, 0xE7, 0xAA); if(ret < 0) goto exit;  ///< 720H 108MHz VD1/VD2/VD3/VD4

    ret = tw2968_i2c_write(id, 0x63, 0x10); if(ret < 0) goto exit;  ///< CH2NUM, CH1NUM
    ret = tw2968_i2c_write(id, 0x64, 0x32); if(ret < 0) goto exit;  ///< CH4NUM, CH3NUM
    ret = tw2968_i2c_write(id, 0x65, 0x54); if(ret < 0) goto exit;  ///< CH6NUM, CH5NUM
    ret = tw2968_i2c_write(id, 0x66, 0x76); if(ret < 0) goto exit;  ///< CH8NUM, CH7NUM

    ret = tw2968_i2c_write(id, 0xE8, 0x10); if(ret < 0) goto exit;  ///< VD1
    ret = tw2968_i2c_write(id, 0xE9, 0x32); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xEA, 0x54); if(ret < 0) goto exit;  ///< VD2
    ret = tw2968_i2c_write(id, 0xEB, 0x76); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xEC, 0x10); if(ret < 0) goto exit;  ///< VD3
    ret = tw2968_i2c_write(id, 0xED, 0x32); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xEE, 0x54); if(ret < 0) goto exit;  ///< VD4
    ret = tw2968_i2c_write(id, 0xEF, 0x76); if(ret < 0) goto exit;

    ret = tw2968_i2c_write(id, 0xEF, 0x32); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xCC, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xCD, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xF9, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xFA, 0x4A); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6F, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9E, 0x52); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9F, 0x77); if(ret < 0) goto exit;  ///< Clock Output Delay 3ns

    /* Disable -> Enable PLL for system reset VD clock not stable problem */
    ret = tw2968_i2c_write(id, 0x61, 0xFF); if(ret < 0) goto exit;
    udelay(5);
    ret = tw2968_i2c_write(id, 0x61, 0x1F); if(ret < 0) goto exit;

    printk("TW2968#%d ==> NTSC 720H 108MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2968_pal_960h_4ch(int id)
{
    int ret;

    if(id >= TW2968_DEV_MAX)
        return -1;

    ret = tw2968_pal_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2968_i2c_write(id, 0xE7, 0xAA); if(ret < 0) goto exit;  ///< 960H 144MHz VD1/VD2/VD3/VD4

    ret = tw2968_i2c_write(id, 0x63, 0x10); if(ret < 0) goto exit;  ///< CH2NUM, CH1NUM
    ret = tw2968_i2c_write(id, 0x64, 0x32); if(ret < 0) goto exit;  ///< CH4NUM, CH3NUM
    ret = tw2968_i2c_write(id, 0x65, 0x54); if(ret < 0) goto exit;  ///< CH6NUM, CH5NUM
    ret = tw2968_i2c_write(id, 0x66, 0x76); if(ret < 0) goto exit;  ///< CH8NUM, CH7NUM

    ret = tw2968_i2c_write(id, 0xE8, 0x10); if(ret < 0) goto exit;  ///< VD1
    ret = tw2968_i2c_write(id, 0xE9, 0x32); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xEA, 0x54); if(ret < 0) goto exit;  ///< VD2
    ret = tw2968_i2c_write(id, 0xEB, 0x76); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xEC, 0x10); if(ret < 0) goto exit;  ///< VD3
    ret = tw2968_i2c_write(id, 0xED, 0x32); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xEE, 0x54); if(ret < 0) goto exit;  ///< VD4
    ret = tw2968_i2c_write(id, 0xEF, 0x76); if(ret < 0) goto exit;

    ret = tw2968_i2c_write(id, 0xCC, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xCD, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xF9, 0x03); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xFA, 0x4A); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6F, 0xF0); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9E, 0x52); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9F, 0x00); if(ret < 0) goto exit;  ///< Clock Output Delay 0ns

    /* Disable -> Enable PLL for system reset VD clock not stable problem */
    ret = tw2968_i2c_write(id, 0x61, 0xFF); if(ret < 0) goto exit;
    udelay(5);
    ret = tw2968_i2c_write(id, 0x61, 0x1F); if(ret < 0) goto exit;

    printk("TW2968#%d ==> PAL 960H 144MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2968_pal_720h_4ch(int id)
{
    int ret;

    if(id >= TW2968_DEV_MAX)
        return -1;

    ret = tw2968_pal_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2968_i2c_write(id, 0xE7, 0xAA); if(ret < 0) goto exit;  ///< 720H 108MHz VD1/VD2/VD3/VD4

    ret = tw2968_i2c_write(id, 0x63, 0x10); if(ret < 0) goto exit;  ///< CH2NUM, CH1NUM
    ret = tw2968_i2c_write(id, 0x64, 0x32); if(ret < 0) goto exit;  ///< CH4NUM, CH3NUM
    ret = tw2968_i2c_write(id, 0x65, 0x54); if(ret < 0) goto exit;  ///< CH6NUM, CH5NUM
    ret = tw2968_i2c_write(id, 0x66, 0x76); if(ret < 0) goto exit;  ///< CH8NUM, CH7NUM

    ret = tw2968_i2c_write(id, 0xE8, 0x10); if(ret < 0) goto exit;  ///< VD1
    ret = tw2968_i2c_write(id, 0xE9, 0x32); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xEA, 0x54); if(ret < 0) goto exit;  ///< VD2
    ret = tw2968_i2c_write(id, 0xEB, 0x76); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xEC, 0x10); if(ret < 0) goto exit;  ///< VD3
    ret = tw2968_i2c_write(id, 0xED, 0x32); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xEE, 0x54); if(ret < 0) goto exit;  ///< VD4
    ret = tw2968_i2c_write(id, 0xEF, 0x76); if(ret < 0) goto exit;

    ret = tw2968_i2c_write(id, 0xCC, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xCD, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xF9, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xFA, 0x4A); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6F, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9E, 0x52); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9F, 0xff); if(ret < 0) goto exit;  ///< Clock Output Delay 4ns

    /* Disable -> Enable PLL for system reset VD clock not stable problem */
    ret = tw2968_i2c_write(id, 0x61, 0xFF); if(ret < 0) goto exit;
    udelay(5);
    ret = tw2968_i2c_write(id, 0x61, 0x1F); if(ret < 0) goto exit;

    printk("TW2968#%d ==> PAL 720H 108MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2968_ntsc_960h_2ch(int id)
{
    int ret;

    if(id >= TW2968_DEV_MAX)
        return -1;

    ret = tw2968_ntsc_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2968_i2c_write(id, 0xE7, 0x55); if(ret < 0) goto exit;  ///< 960H 72MHz VD1/VD2/VD3/VD4

    ret = tw2968_i2c_write(id, 0x63, 0x10); if(ret < 0) goto exit;  ///< CH2NUM, CH1NUM
    ret = tw2968_i2c_write(id, 0x64, 0x32); if(ret < 0) goto exit;  ///< CH4NUM, CH3NUM
    ret = tw2968_i2c_write(id, 0x65, 0x54); if(ret < 0) goto exit;  ///< CH6NUM, CH5NUM
    ret = tw2968_i2c_write(id, 0x66, 0x76); if(ret < 0) goto exit;  ///< CH8NUM, CH7NUM

    ret = tw2968_i2c_write(id, 0xE8, 0x10); if(ret < 0) goto exit;  ///< VD1
    ret = tw2968_i2c_write(id, 0xEA, 0x32); if(ret < 0) goto exit;  ///< VD2
    ret = tw2968_i2c_write(id, 0xEC, 0x54); if(ret < 0) goto exit;  ///< VD3
    ret = tw2968_i2c_write(id, 0xEE, 0x76); if(ret < 0) goto exit;  ///< VD4

    ret = tw2968_i2c_write(id, 0xCC, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xCD, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xF9, 0x03); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xFA, 0x40); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6F, 0xF0); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9F, 0x00); if(ret < 0) goto exit;  ///< Clock Output Delay 0ns

    /* Disable -> Enable PLL for system reset VD clock not stable problem */
    ret = tw2968_i2c_write(id, 0x61, 0xFF); if(ret < 0) goto exit;
    udelay(5);
    ret = tw2968_i2c_write(id, 0x61, 0x1F); if(ret < 0) goto exit;

    printk("TW2968#%d ==> NTSC 960H 72MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2968_ntsc_720h_2ch(int id)
{
    int ret;

    if(id >= TW2968_DEV_MAX)
        return -1;

    ret = tw2968_ntsc_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2968_i2c_write(id, 0xE7, 0x55); if(ret < 0) goto exit;  ///< 720H 54MHz VD1/VD2/VD3/VD4

    ret = tw2968_i2c_write(id, 0x63, 0x10); if(ret < 0) goto exit;  ///< CH2NUM, CH1NUM
    ret = tw2968_i2c_write(id, 0x64, 0x32); if(ret < 0) goto exit;  ///< CH4NUM, CH3NUM
    ret = tw2968_i2c_write(id, 0x65, 0x54); if(ret < 0) goto exit;  ///< CH6NUM, CH5NUM
    ret = tw2968_i2c_write(id, 0x66, 0x76); if(ret < 0) goto exit;  ///< CH8NUM, CH7NUM

    ret = tw2968_i2c_write(id, 0xE8, 0x10); if(ret < 0) goto exit;  ///< VD1
    ret = tw2968_i2c_write(id, 0xEA, 0x32); if(ret < 0) goto exit;  ///< VD2
    ret = tw2968_i2c_write(id, 0xEC, 0x54); if(ret < 0) goto exit;  ///< VD3
    ret = tw2968_i2c_write(id, 0xEE, 0x76); if(ret < 0) goto exit;  ///< VD4

    ret = tw2968_i2c_write(id, 0xCC, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xCD, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xF9, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xFA, 0x40); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6F, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9F, 0x00); if(ret < 0) goto exit;  ///< Clock Output Delay 0ns

    /* Disable -> Enable PLL for system reset VD clock not stable problem */
    ret = tw2968_i2c_write(id, 0x61, 0xFF); if(ret < 0) goto exit;
    udelay(5);
    ret = tw2968_i2c_write(id, 0x61, 0x1F); if(ret < 0) goto exit;

    printk("TW2968#%d ==> NTSC 720H 54MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2968_pal_960h_2ch(int id)
{
    int ret;

    if(id >= TW2968_DEV_MAX)
        return -1;

    ret = tw2968_pal_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2968_i2c_write(id, 0xE7, 0x55); if(ret < 0) goto exit;  ///< 960H 72MHz VD1/VD2/VD3/VD4

    ret = tw2968_i2c_write(id, 0x63, 0x10); if(ret < 0) goto exit;  ///< CH2NUM, CH1NUM
    ret = tw2968_i2c_write(id, 0x64, 0x32); if(ret < 0) goto exit;  ///< CH4NUM, CH3NUM
    ret = tw2968_i2c_write(id, 0x65, 0x54); if(ret < 0) goto exit;  ///< CH6NUM, CH5NUM
    ret = tw2968_i2c_write(id, 0x66, 0x76); if(ret < 0) goto exit;  ///< CH8NUM, CH7NUM

    ret = tw2968_i2c_write(id, 0xE8, 0x10); if(ret < 0) goto exit;  ///< VD1
    ret = tw2968_i2c_write(id, 0xEA, 0x32); if(ret < 0) goto exit;  ///< VD2
    ret = tw2968_i2c_write(id, 0xEC, 0x54); if(ret < 0) goto exit;  ///< VD3
    ret = tw2968_i2c_write(id, 0xEE, 0x76); if(ret < 0) goto exit;  ///< VD4

    ret = tw2968_i2c_write(id, 0xCC, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xCD, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xF9, 0x03); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xFA, 0x40); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6F, 0xF0); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9F, 0x00); if(ret < 0) goto exit;  ///< Clock Output Delay 0ns

    /* Disable -> Enable PLL for system reset VD clock not stable problem */
    ret = tw2968_i2c_write(id, 0x61, 0xFF); if(ret < 0) goto exit;
    udelay(5);
    ret = tw2968_i2c_write(id, 0x61, 0x1F); if(ret < 0) goto exit;

    printk("TW2968#%d ==> PAL 960H 72MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2968_pal_720h_2ch(int id)
{
    int ret;

    if(id >= TW2968_DEV_MAX)
        return -1;

    ret = tw2968_pal_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2968_i2c_write(id, 0xE7, 0x55); if(ret < 0) goto exit;  ///< 720H 54MHz VD1/VD2/VD3/VD4

    ret = tw2968_i2c_write(id, 0x63, 0x10); if(ret < 0) goto exit;  ///< CH2NUM, CH1NUM
    ret = tw2968_i2c_write(id, 0x64, 0x32); if(ret < 0) goto exit;  ///< CH4NUM, CH3NUM
    ret = tw2968_i2c_write(id, 0x65, 0x54); if(ret < 0) goto exit;  ///< CH6NUM, CH5NUM
    ret = tw2968_i2c_write(id, 0x66, 0x76); if(ret < 0) goto exit;  ///< CH8NUM, CH7NUM

    ret = tw2968_i2c_write(id, 0xE8, 0x10); if(ret < 0) goto exit;  ///< VD1
    ret = tw2968_i2c_write(id, 0xEA, 0x32); if(ret < 0) goto exit;  ///< VD2
    ret = tw2968_i2c_write(id, 0xEC, 0x54); if(ret < 0) goto exit;  ///< VD3
    ret = tw2968_i2c_write(id, 0xEE, 0x76); if(ret < 0) goto exit;  ///< VD4

    ret = tw2968_i2c_write(id, 0xCC, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xCD, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xF9, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xFA, 0x40); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6F, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9F, 0x00); if(ret < 0) goto exit;  ///< Clock Output Delay 0ns

    /* Disable -> Enable PLL for system reset VD clock not stable problem */
    ret = tw2968_i2c_write(id, 0x61, 0xFF); if(ret < 0) goto exit;
    udelay(5);
    ret = tw2968_i2c_write(id, 0x61, 0x1F); if(ret < 0) goto exit;

    printk("TW2968#%d ==> PAL 720H 54MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2968_ntsc_960h_1ch(int id)
{
    int ret;

    if(id >= TW2968_DEV_MAX)
        return -1;

    ret = tw2968_ntsc_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2968_i2c_write(id, 0xE7, 0x00); if(ret < 0) goto exit;  ///< 960H 36MHz VD1/VD2/VD3/VD4

    ret = tw2968_i2c_write(id, 0x63, 0x10); if(ret < 0) goto exit;  ///< CH2NUM, CH1NUM
    ret = tw2968_i2c_write(id, 0x64, 0x32); if(ret < 0) goto exit;  ///< CH4NUM, CH3NUM
    ret = tw2968_i2c_write(id, 0x65, 0x54); if(ret < 0) goto exit;  ///< CH6NUM, CH5NUM
    ret = tw2968_i2c_write(id, 0x66, 0x76); if(ret < 0) goto exit;  ///< CH8NUM, CH7NUM

    ret = tw2968_i2c_write(id, 0xE8, 0x00); if(ret < 0) goto exit;  ///< VD1
    ret = tw2968_i2c_write(id, 0xEA, 0x11); if(ret < 0) goto exit;  ///< VD2
    ret = tw2968_i2c_write(id, 0xEC, 0x22); if(ret < 0) goto exit;  ///< VD3
    ret = tw2968_i2c_write(id, 0xEE, 0x33); if(ret < 0) goto exit;  ///< VD4

    ret = tw2968_i2c_write(id, 0xCC, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xCD, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xF9, 0x03); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xFA, 0x40); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6F, 0xF0); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9F, 0x00); if(ret < 0) goto exit;  ///< Clock Output Delay 0ns

    /* Disable -> Enable PLL for system reset VD clock not stable problem */
    ret = tw2968_i2c_write(id, 0x61, 0xFF); if(ret < 0) goto exit;
    udelay(5);
    ret = tw2968_i2c_write(id, 0x61, 0x1F); if(ret < 0) goto exit;

    printk("TW2968#%d ==> NTSC 960H 36MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2968_pal_960h_1ch(int id)
{
    int ret;

    if(id >= TW2968_DEV_MAX)
        return -1;

    ret = tw2968_pal_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2968_i2c_write(id, 0xE7, 0x00); if(ret < 0) goto exit;  ///< 960H 36MHz VD1/VD2/VD3/VD4

    ret = tw2968_i2c_write(id, 0x63, 0x10); if(ret < 0) goto exit;  ///< CH2NUM, CH1NUM
    ret = tw2968_i2c_write(id, 0x64, 0x32); if(ret < 0) goto exit;  ///< CH4NUM, CH3NUM
    ret = tw2968_i2c_write(id, 0x65, 0x54); if(ret < 0) goto exit;  ///< CH6NUM, CH5NUM
    ret = tw2968_i2c_write(id, 0x66, 0x76); if(ret < 0) goto exit;  ///< CH8NUM, CH7NUM

    ret = tw2968_i2c_write(id, 0xE8, 0x00); if(ret < 0) goto exit;  ///< VD1
    ret = tw2968_i2c_write(id, 0xEA, 0x11); if(ret < 0) goto exit;  ///< VD2
    ret = tw2968_i2c_write(id, 0xEC, 0x22); if(ret < 0) goto exit;  ///< VD3
    ret = tw2968_i2c_write(id, 0xEE, 0x33); if(ret < 0) goto exit;  ///< VD4

    ret = tw2968_i2c_write(id, 0xCC, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xCD, 0xFF); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xF9, 0x03); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xFA, 0x40); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6F, 0xF0); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9F, 0x00); if(ret < 0) goto exit;  ///< Clock Output Delay 0ns

    /* Disable -> Enable PLL for system reset VD clock not stable problem */
    ret = tw2968_i2c_write(id, 0x61, 0xFF); if(ret < 0) goto exit;
    udelay(5);
    ret = tw2968_i2c_write(id, 0x61, 0x1F); if(ret < 0) goto exit;

    printk("TW2968#%d ==> PAL 960H 36MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2968_ntsc_720h_1ch(int id)
{
    int ret;

    if(id >= TW2968_DEV_MAX)
        return -1;

    ret = tw2968_ntsc_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2968_i2c_write(id, 0xE7, 0x00); if(ret < 0) goto exit;  ///< 720H 27MHz VD1/VD2/VD3/VD4

    ret = tw2968_i2c_write(id, 0x63, 0x10); if(ret < 0) goto exit;  ///< CH2NUM, CH1NUM
    ret = tw2968_i2c_write(id, 0x64, 0x32); if(ret < 0) goto exit;  ///< CH4NUM, CH3NUM
    ret = tw2968_i2c_write(id, 0x65, 0x54); if(ret < 0) goto exit;  ///< CH6NUM, CH5NUM
    ret = tw2968_i2c_write(id, 0x66, 0x76); if(ret < 0) goto exit;  ///< CH8NUM, CH7NUM

    ret = tw2968_i2c_write(id, 0xE8, 0x00); if(ret < 0) goto exit;  ///< VD1
    ret = tw2968_i2c_write(id, 0xEA, 0x11); if(ret < 0) goto exit;  ///< VD2
    ret = tw2968_i2c_write(id, 0xEC, 0x22); if(ret < 0) goto exit;  ///< VD3
    ret = tw2968_i2c_write(id, 0xEE, 0x33); if(ret < 0) goto exit;  ///< VD4

    ret = tw2968_i2c_write(id, 0xCC, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xCD, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xF9, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xFA, 0x40); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6F, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9F, 0x00); if(ret < 0) goto exit;  ///< Clock Output Delay 0ns

    /* Disable -> Enable PLL for system reset VD clock not stable problem */
    ret = tw2968_i2c_write(id, 0x61, 0xFF); if(ret < 0) goto exit;
    udelay(5);
    ret = tw2968_i2c_write(id, 0x61, 0x1F); if(ret < 0) goto exit;

    printk("TW2968#%d ==> NTSC 720H 27MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2968_pal_720h_1ch(int id)
{
    int ret;

    if(id >= TW2968_DEV_MAX)
        return -1;

    ret = tw2968_pal_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2968_i2c_write(id, 0xE7, 0x00); if(ret < 0) goto exit;  ///< 720H 27MHz VD1/VD2/VD3/VD4

    ret = tw2968_i2c_write(id, 0x63, 0x10); if(ret < 0) goto exit;  ///< CH2NUM, CH1NUM
    ret = tw2968_i2c_write(id, 0x64, 0x32); if(ret < 0) goto exit;  ///< CH4NUM, CH3NUM
    ret = tw2968_i2c_write(id, 0x65, 0x54); if(ret < 0) goto exit;  ///< CH6NUM, CH5NUM
    ret = tw2968_i2c_write(id, 0x66, 0x76); if(ret < 0) goto exit;  ///< CH8NUM, CH7NUM

    ret = tw2968_i2c_write(id, 0xE8, 0x00); if(ret < 0) goto exit;  ///< VD1
    ret = tw2968_i2c_write(id, 0xEA, 0x11); if(ret < 0) goto exit;  ///< VD2
    ret = tw2968_i2c_write(id, 0xEC, 0x22); if(ret < 0) goto exit;  ///< VD3
    ret = tw2968_i2c_write(id, 0xEE, 0x33); if(ret < 0) goto exit;  ///< VD4

    ret = tw2968_i2c_write(id, 0xCC, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xCD, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xF9, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xFA, 0x40); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x6F, 0x00); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0x9F, 0x00); if(ret < 0) goto exit;  ///< Clock Output Delay 0ns

    /* Disable -> Enable PLL for system reset VD clock not stable problem */
    ret = tw2968_i2c_write(id, 0x61, 0xFF); if(ret < 0) goto exit;
    udelay(5);
    ret = tw2968_i2c_write(id, 0x61, 0x1F); if(ret < 0) goto exit;

    printk("TW2968#%d ==> PAL 720H 27MHz MODE!!\n", id);

exit:
    return ret;
}

int tw2968_video_set_mode(int id, TW2968_VMODE_T mode)
{
    int ret;

    if(id >= TW2968_DEV_MAX || mode >= TW2968_VMODE_MAX)
        return -1;

    switch(mode) {
        case TW2968_VMODE_NTSC_720H_1CH:
            ret = tw2968_ntsc_720h_1ch(id);
            break;
        case TW2968_VMODE_NTSC_720H_2CH:
            ret = tw2968_ntsc_720h_2ch(id);
            break;
        case TW2968_VMODE_NTSC_720H_4CH:
            ret = tw2968_ntsc_720h_4ch(id);
            break;
        case TW2968_VMODE_NTSC_960H_1CH:
            ret = tw2968_ntsc_960h_1ch(id);
            break;
        case TW2968_VMODE_NTSC_960H_2CH:
            ret = tw2968_ntsc_960h_2ch(id);
            break;
        case TW2968_VMODE_NTSC_960H_4CH:
            ret = tw2968_ntsc_960h_4ch(id);
            break;

        case TW2968_VMODE_PAL_720H_1CH:
            ret = tw2968_pal_720h_1ch(id);
            break;
        case TW2968_VMODE_PAL_720H_2CH:
            ret = tw2968_pal_720h_2ch(id);
            break;
        case TW2968_VMODE_PAL_720H_4CH:
            ret = tw2968_pal_720h_4ch(id);
            break;
        case TW2968_VMODE_PAL_960H_1CH:
            ret = tw2968_pal_960h_1ch(id);
            break;
        case TW2968_VMODE_PAL_960H_2CH:
            ret = tw2968_pal_960h_2ch(id);
            break;
        case TW2968_VMODE_PAL_960H_4CH:
            ret = tw2968_pal_960h_4ch(id);
            break;
        default:
            ret = -1;
            printk("TW2968#%d driver not support video output mode(%d)!!\n", id, mode);
            break;
    }

    return ret;
}
EXPORT_SYMBOL(tw2968_video_set_mode);

int tw2968_video_get_mode(int id)
{
    int ret;
    int is_ntsc;
    int is_960h;

    if(id >= TW2968_DEV_MAX)
        return -1;

    /* get ch#0 current register setting for determine device running mode */
    ret = tw2968_i2c_write(id, 0x40, 0x00);
    if(ret < 0)
        goto exit;
    ret = (tw2968_i2c_read(id, 0x0E)&0x70)>>4;
    if((ret == 0x0) || (ret == 0x3))
        is_ntsc = 1;
    else if ((ret == 0x1) || (ret == 0x4) || (ret == 0x5) || (ret == 0x6))
        is_ntsc = 0;
    else {
        printk("Can't get current video mode (NTSC or PAL) of TW2968#%d!!\n", id);
        return -1;
    }

    is_960h = ((tw2968_i2c_read(id, 0xCC)&0x01) == 0x01) ? 1 : 0;

    if(is_ntsc) {
        if(is_960h)
            ret = TW2968_VMODE_NTSC_960H_4CH;
        else
            ret = TW2968_VMODE_NTSC_720H_4CH;
    }
    else {
        if(is_960h)
            ret = TW2968_VMODE_PAL_960H_4CH;
        else
            ret = TW2968_VMODE_PAL_720H_4CH;
    }

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_video_get_mode);

int tw2968_video_mode_support_check(int id, TW2968_VMODE_T mode)
{
    if(id >= TW2968_DEV_MAX)
        return 0;

    switch(mode) {
        case TW2968_VMODE_NTSC_720H_1CH:
        case TW2968_VMODE_NTSC_720H_2CH:
        case TW2968_VMODE_NTSC_720H_4CH:
        case TW2968_VMODE_NTSC_960H_1CH:
        case TW2968_VMODE_NTSC_960H_2CH:
        case TW2968_VMODE_NTSC_960H_4CH:
        case TW2968_VMODE_PAL_720H_1CH:
        case TW2968_VMODE_PAL_720H_2CH:
        case TW2968_VMODE_PAL_720H_4CH:
        case TW2968_VMODE_PAL_960H_1CH:
        case TW2968_VMODE_PAL_960H_2CH:
        case TW2968_VMODE_PAL_960H_4CH:
            return 1;
        default:
            return 0;
    }

    return 0;
}
EXPORT_SYMBOL(tw2968_video_mode_support_check);

int tw2968_video_set_contrast(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2968_DEV_MAX || ch >= TW2968_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2968_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2968_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2968_i2c_write(id, 0x02+(0x10*(ch%4)), value);

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_video_set_contrast);

int tw2968_video_get_contrast(int id, int ch)
{
    int ret;

    if(id >= TW2968_DEV_MAX || ch >= TW2968_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2968_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2968_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2968_i2c_read(id, 0x02+(0x10*(ch%4)));

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_video_get_contrast);

int tw2968_video_set_brightness(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2968_DEV_MAX || ch >= TW2968_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2968_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2968_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2968_i2c_write(id, 0x01+(0x10*(ch%4)), value);

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_video_set_brightness);

int tw2968_video_get_brightness(int id, int ch)
{
    int ret = 0;

    if(id >= TW2968_DEV_MAX || ch >= TW2968_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2968_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2968_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2968_i2c_read(id, 0x01+(0x10*(ch%4)));

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_video_get_brightness);

int tw2968_video_set_saturation_u(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2968_DEV_MAX || ch >= TW2968_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2968_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2968_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2968_i2c_write(id, 0x04+(0x10*(ch%4)), value);

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_video_set_saturation_u);

int tw2968_video_get_saturation_u(int id, int ch)
{
    int ret;

    if(id >= TW2968_DEV_MAX || ch >= TW2968_DEV_CH_MAX)
        return -1;


    if(ch < 4) {
        /* BANK 0 */
        ret = tw2968_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2968_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2968_i2c_read(id, 0x04+(0x10*(ch%4)));

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_video_get_saturation_u);

int tw2968_video_set_saturation_v(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2968_DEV_MAX || ch >= TW2968_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2968_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2968_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2968_i2c_write(id, 0x05+(0x10*(ch%4)), value);

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_video_set_saturation_v);

int tw2968_video_get_saturation_v(int id, int ch)
{
    int ret;

    if(id >= TW2968_DEV_MAX || ch >= TW2968_DEV_CH_MAX)
        return -1;


    if(ch < 4) {
        /* BANK 0 */
        ret = tw2968_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2968_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2968_i2c_read(id, 0x05+(0x10*(ch%4)));

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_video_get_saturation_v);

int tw2968_video_set_hue(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2968_DEV_MAX || ch >= TW2968_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2968_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2968_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2968_i2c_write(id, 0x06+(0x10*(ch%4)), value);

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_video_set_hue);

int tw2968_video_get_hue(int id, int ch)
{
    int ret;

    if(id >= TW2968_DEV_MAX || ch >= TW2968_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2968_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2968_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2968_i2c_read(id, 0x06+(0x10*(ch%4)));

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_video_get_hue);

int tw2968_video_set_sharpness(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2968_DEV_MAX || ch >= TW2968_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2968_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2968_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = (tw2968_i2c_read(id, 0x03+(0x10*(ch%4))) & 0xF0);
    value = ret | (value & 0x0F);
    ret = tw2968_i2c_write(id, 0x03+(0x10*(ch%4)), value);

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_video_set_sharpness);

int tw2968_video_get_sharpness(int id, int ch)
{
    int ret;

    if(id >= TW2968_DEV_MAX || ch >= TW2968_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2968_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2968_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2968_i2c_read(id, 0x03+(0x10*(ch%4))) & 0x0F;

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_video_get_sharpness);

int tw2968_status_get_novid(int id, int ch)
{
    int ret;

    if(id >= TW2968_DEV_MAX || ch >= TW2968_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2968_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;

        ret = (tw2968_i2c_read(id, 0x10*ch)>>7) & 0x1;
    }
    else {
        /* BANK 1 */
        ret = tw2968_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;

        ret = (tw2968_i2c_read(id, 0x10*(ch-4))>>7) & 0x1;
    }

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_status_get_novid);

int tw2968_audio_set_mute(int id, int on)
{
    int data;
    int ret;
    /* set DAOGAIN: BIT4 DAORATIO = 1, DAOGAIN = DAOGAIN/64, BIT4 DAORATIO = 0, DAOGAIN = DAOGAIN
       set DAORATIO = 1 ==> DAOGAIN = DAOGAIN/64, SET DAOGAIN = 0 ==> 0/64 = 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0
    data = tw2968_i2c_read(id, 0x72);
    data &= 0xe0;
    data |= 0x10;
    ret = tw2968_i2c_write(id, 0x72, data); if(ret < 0) goto exit;     ///< DAOGAIN mute

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_audio_set_mute);

int tw2968_audio_get_mute(int id)
{
    int data, mute;
    int ret;

    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0
    data = tw2968_i2c_read(id, 0x72);
    if ((data & 0x10) == 0x10)
        mute = 1;
    else
        mute = 0;

    return mute;

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_audio_get_mute);

int tw2968_audio_get_volume(int id)
{
    int aogain;
    int ret;

    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0

    aogain = tw2968_i2c_read(id, 0x72);
    if ((aogain & 0x10) == 0x10)
        aogain = 0;
    else
        aogain &= 0xf;

    return aogain;

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_audio_get_volume);

int tw2968_audio_set_volume(int id, int volume)
{
    int data;
    int ret;

    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0
    data = tw2968_i2c_read(id, 0x72);
    data &= 0xE0;
    data |= volume;
    ret = tw2968_i2c_write(id, 0x72, data); if(ret < 0) goto exit;     ///< DAOGAIN setting

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_audio_set_volume);

int tw2968_audio_get_output_ch(int id)
{
    int ch;
    int ret;

    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0

    ch = tw2968_i2c_read(id, 0xe0);
    ch &= 0x1f;

    return ch;

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_audio_get_output_ch);

int tw2968_audio_set_output_ch(int id, int ch)
{
    int data;
    int ret;

    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0

    data = tw2968_i2c_read(id, 0xe0);
    data &= 0xe0;
    data |= ch;
    ret = tw2968_i2c_write(id, 0xe0, data); if(ret < 0) goto exit;              ///< MIX_OUTSEL

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_audio_set_output_ch);

int tw2968_swap_channels(int id, int num_of_ch)
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
            printk("value32 %x\n", *value32);
            //printk("value %x\n", &tmp_memory[REG_NUMBER(0xd3)]+side*4);
            SWAP_8CHANNEL(*value32);
            printk("1 value32 %x\n", *value32);
            break;
        }
    }

    /* update the register to codec */
    ret = tw2968_write_table(id, 0xD0, tmp_memory, sizeof(tbl_pg0_sfr2));

    return ret;
}

/* set sample rate */
int tw2968_audio_set_sample_rate(int id, TW2968_SAMPLERATE_T samplerate)
{
    int ret;
    int data;

    if (samplerate > TW2968_AUDIO_SAMPLERATE_48K) {
        printk("sample rate exceed tw2968 fs table \n");
        return -1;
    }

    data = tw2968_i2c_read(id, 0x70);
    data &= ~0xF;
    data |= 0x1 << 3;        /* AFAUTO */

    switch (samplerate) {
    default:
    case TW2968_AUDIO_SAMPLERATE_8K:
        data |= 0;
        break;
    case TW2968_AUDIO_SAMPLERATE_16K:
        data |= 1;
        break;
    case TW2968_AUDIO_SAMPLERATE_32K:
        data |= 2;
        break;
    case TW2968_AUDIO_SAMPLERATE_44K:
        data |= 3;
        break;
    case TW2968_AUDIO_SAMPLERATE_48K:
        data |= 4;
        break;
    }

    ret = tw2968_i2c_write(id, 0x70, data);

    return ret;
}

int tw2968_audio_set_sample_size(int id, TW2968_SAMPLESIZE_T samplesize)
{
    uint8_t data = 0;
    int ret = 0;

    if ((samplesize != TW2968_AUDIO_BITS_8B) && (samplesize != TW2968_AUDIO_BITS_16B)) {
        printk("%s fails: due to wrong sampling size = %d\n", __FUNCTION__, samplesize);
        return -1;
    }

    data = tw2968_i2c_read(id, 0xDF);
    //data |= 0xF0;
    data = 0x80;
    ret = tw2968_i2c_write(id, 0xDF, data); if(ret < 0) goto exit;

    data = tw2968_i2c_read(id, 0xDB);
    if (samplesize == TW2968_AUDIO_BITS_8B) {
        data = data | 0x04;
    } else if (samplesize == TW2968_AUDIO_BITS_16B) {
        data = data & (~0x04);
    }
    ret = tw2968_i2c_write(id, 0xDB, data); if(ret < 0) goto exit;

exit:
    return ret;
}

static int tw2968_audio_set_master_chan_seq(int id, char *RSEQ)
{
    u16 data;
    int ret;

    data = RSEQ[0];
    ret = tw2968_i2c_write(id, 0xD3, data); if(ret < 0) goto exit;
    data = RSEQ[1];
    ret = tw2968_i2c_write(id, 0xD4, data); if(ret < 0) goto exit;
    data = RSEQ[2];
    ret = tw2968_i2c_write(id, 0xD5, data); if(ret < 0) goto exit;
    data = RSEQ[3];
    ret = tw2968_i2c_write(id, 0xD6, data); if(ret < 0) goto exit;
    data = RSEQ[4];
    ret = tw2968_i2c_write(id, 0xD7, data); if(ret < 0) goto exit;
    data = RSEQ[5];
    ret = tw2968_i2c_write(id, 0xD8, data); if(ret < 0) goto exit;
    data = RSEQ[6];
    ret = tw2968_i2c_write(id, 0xD9, data); if(ret < 0) goto exit;
    data = RSEQ[7];
    ret = tw2968_i2c_write(id, 0xDA, data); if(ret < 0) goto exit;
exit:
    return ret;
}
#define SUPPORT_AUXIN51     0
int tw2968_audio_init(int id, int total_channels)
{
    int ret =0;
    int data = 0;
    int cascade = 0;
    int i, j, ch, ch1;
    char RSEQ[8] = {0};
    u32 *temp;
    u16 *temp1;

    if (audio_chnum == 16)
        cascade = 1;

    /* BANK 0 */
    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;      ///< set to page#0

    if (cascade == 1)
    ret = tw2968_i2c_write(id, 0xCF, 0x80); if(ret < 0) goto exit;      ///< CASCADE MODE

    data = tw2968_i2c_read(id, 0xFB);                                   ///< define audio detection mode
    data |= 0xc;
    ret = tw2968_i2c_write(id, 0xFB, data); if(ret < 0) goto exit;

    data = tw2968_i2c_read(id, 0xFC);                                   ///< enable audio detection AIN1234
    data |= 0xf0;
    ret = tw2968_i2c_write(id, 0xFC, data); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xDB, 0xC1); if(ret < 0) goto exit;      ///< master control: MASTER MODE

    if (audio_chnum == 8)
        ret = tw2968_i2c_write(id, 0xD2, 0x02); if(ret < 0) goto exit;  ///< 8 channel record, first stage playback
    if (audio_chnum == 16)
        ret = tw2968_i2c_write(id, 0xD2, 0x03); if(ret < 0) goto exit;  ///< 16 channel record, first stage playback

    ret = tw2968_i2c_write(id, 0xE0, 0x10); if(ret < 0) goto exit;      ///< MIX_OUTSEL = playback first stage
    ret = tw2968_i2c_write(id, 0x70, 0x88); if(ret < 0) goto exit;      ///< AFAUTO=1, sample rate = 8k

    data = tw2968_i2c_read(id, 0x89);                                   ///< AFS384=0/AIN5MD=0
    data &= 0xf0;
    tw2968_i2c_write(id, 0x89, data); if(ret < 0) goto exit;

    data = tw2968_i2c_read(id, 0x71);                                   ///< MASCKMD = 1
    data |= (1 << 6);
    ret = tw2968_i2c_write(id, 0x71, data); if(ret < 0) goto exit;

    ret = tw2968_i2c_write(id, 0xDC, 0x20); if(ret < 0) goto exit;      ///< mix mute control AIN1/2/3/4 not muted

    for(i = 0; i < 4; i++) {
        j = i * 2;
        ch = tw2968_vin_to_ch(id, j);
        ch1 = tw2968_vin_to_ch(id, j+1);
        RSEQ[i] = (ch1 << 4 | ch);

        ch = tw2968_vin_to_ch(id + 1, j);
        ch1 = tw2968_vin_to_ch(id + 1, j+1);
        ch += 8;
        ch1 += 8;
        RSEQ[i+4] = (ch1 << 4 | ch);
    }

    for (i = 0; i < 8; i++)
        SWAP_2CHANNEL(RSEQ[i]);

    if (cascade == 1) {
        for (i = 0; i < 2; i++) {
            j = i*4;
            temp = (u32 *)&RSEQ[j];
            SWAP_8CHANNEL(*temp);
        }
    }
    else {
        for (i = 0; i < 4; i++) {
            j = i * 2;
            temp1 = (u16 *)&RSEQ[j];
            SWAP_4CHANNEL(*temp1);
        }

        RSEQ[4] = RSEQ[2];
        RSEQ[5] = RSEQ[3];
        RSEQ[2] = 0x89;
        RSEQ[3] = 0xab;
        RSEQ[6] = 0xcd;
        RSEQ[7] = 0xef;
    }

    if (cascade == 0)
        ret = tw2968_audio_set_master_chan_seq(id, RSEQ);
    // when cascade, slave configure should as same as master
    if (cascade == 1) {
        if ((id == 0) || (id == 2)) {
            ret = tw2968_audio_set_master_chan_seq(id, RSEQ);
            ret = tw2968_audio_set_master_chan_seq(id + 1, RSEQ);
        }
    }

    /* if support AIN51 --> tw2968 is 20(10+10) channels mode
     * but ssp010 only support 128 bits (8 channels)
     * set 0x73 = 0x04 to change record output format to
     * ch0/ch1/ch2/ch3/AIN51/ch4/ch5/ch6/ch7/AIN52 to grab AIN51 data, but CH7&AIN52 still can't grab.
     * if 0x73 = 0x00 the record output format is :
     * ch0/ch1/ch2/ch3/ch4/ch5/ch6/ch7/AIN51/AIN52 --> can't grab AIN51&AIN52 because ssp010 only get 128 bits
     */
    if (SUPPORT_AUXIN51) {
        ret = tw2968_i2c_write(id, 0xD2, 0x03); if(ret < 0) goto exit;      ///< set to 16 channel
        ret = tw2968_i2c_write(id, 0x89, 0x04); if(ret < 0) goto exit;      ///< AIN5MD = 1, set to 320 fs mode
        ret = tw2968_i2c_write(id, 0x73, 0x04); if(ret < 0) goto exit;      ///< set AIN51FROM to dat0/dat1/dat2/dat3/dat51/dat4/dat5/dat6/dat7/dat52
        ret = tw2968_i2c_write(id, 0x7d, 0xe4); if(ret < 0) goto exit;      ///< set AIN51 output data position to dat51
        ret = tw2968_i2c_write(id, 0x7e, 0x23); if(ret < 0) goto exit;      ///< output AIN5 record data on ADATR

        // output channel sequence L : CH0/CH1/CH2/AIN51/CH3/CH4/CH5/CH6/ (8+AIN51)
        //                         R : CH7/CH8/CH9/AIN53/CH10/CH11/CH12/CH13/
        ret = tw2968_i2c_write(id, 0xD3, 0x56); if(ret < 0) goto exit;
        ret = tw2968_i2c_write(id, 0xD4, 0x34); if(ret < 0) goto exit;
        ret = tw2968_i2c_write(id, 0xD5, 0x12); if(ret < 0) goto exit;
        ret = tw2968_i2c_write(id, 0xD6, 0xf0); if(ret < 0) goto exit;
        ret = tw2968_i2c_write(id, 0xD7, 0xcd); if(ret < 0) goto exit;
        ret = tw2968_i2c_write(id, 0xD8, 0xab); if(ret < 0) goto exit;
        ret = tw2968_i2c_write(id, 0xD9, 0x89); if(ret < 0) goto exit;
        ret = tw2968_i2c_write(id, 0xDa, 0xf7); if(ret < 0) goto exit;
    }

    /* BANK 1 */
    ret = tw2968_i2c_write(id, 0x40, 0x01); if(ret < 0) goto exit;  ///< set to page#1
    data = tw2968_i2c_read(id, 0xFC);                               ///< enable audio detection AIN5678
    data |= 0xf0;
    ret = tw2968_i2c_write(id, 0xFC, data); if(ret < 0) goto exit;
    ret = tw2968_i2c_write(id, 0xDC, 0x20); if(ret < 0) goto exit;  ///< mix mute control AIN5/6/7/8 not muted

    ret = tw2968_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0
exit:
    return ret;
}

int tw2968_audio_set_mode(int id, TW2968_VMODE_T mode, TW2968_SAMPLESIZE_T samplesize, TW2968_SAMPLERATE_T samplerate)
{
    int ch_num = 0;
    int ret;

    ch_num = audio_chnum;
    if(ch_num < 0){
        return -1;
    }

    /* tw2968 audio initialize */
    ret = tw2968_audio_init(id, ch_num); if(ret < 0) goto exit;

    /* set audio sample rate */
    ret = tw2968_audio_set_sample_rate(id, samplerate); if(ret < 0) goto exit;

    /* set audio sample size */
    ret = tw2968_audio_set_sample_size(id, samplesize); if(ret < 0) goto exit;

exit:
    return ret;
}
EXPORT_SYMBOL(tw2968_audio_set_mode);
