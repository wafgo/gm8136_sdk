/**
 * @file tw2868_lib.c
 * Intersil TW2868 8-CH D1 Video Decoders and Audio Codecs Library
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>

#include "intersil/tw2868.h"       ///< from module/include/front_end/decoder

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

static TW2868_VMODE_T tw2868_vmode[TW2868_DEV_MAX];  ///< for record device video mode

static int tw2868_write_table(int id, u8 addr, u8 *tbl_ptr, int tbl_cnt)
{
    int i, ret = 0;

    if(id >= TW2868_DEV_MAX)
        return -1;

    for(i=0; i<tbl_cnt; i++) {
        ret = tw2868_i2c_write(id, addr+i, tbl_ptr[i]);
        if(ret < 0)
            goto exit;
    }

exit:
    return ret;
}

static int tw2868_ntsc_common_init(int id)
{
    int ret;

    if(id >= TW2868_DEV_MAX)
        return -1;

    /* BANK 0 */
    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0
    /* Common */
    ret = tw2868_i2c_write(id, 0xFA, 0x40); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFB, 0x2F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFC, 0xFF); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xDB, 0xC1); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x73, 0x01); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x9C, 0xA0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x9E, 0x52); if(ret < 0) goto exit;  ///< NOVID
    ret = tw2868_i2c_write(id, 0xD2, 0x01); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xDD, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xDE, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE1, 0xC0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE2, 0xAA); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE3, 0xAA); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xF8, 0x64); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xF9, 0x11); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xAA, 0x00); if(ret < 0) goto exit;

    ret = tw2868_i2c_write(id, 0x5B, 0xFF); if(ret < 0) goto exit;  ///< Output Pin High Drive

    ret = tw2868_i2c_write(id, 0x60, 0x15); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x61, 0x13); if(ret < 0) goto exit;

    ret = tw2868_i2c_write(id, 0x9F, 0x33); if(ret < 0) goto exit;    

    ret = tw2868_i2c_write(id, 0x70, 0x08); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xF8, 0xC4); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xF9, 0x51); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xDF, 0x80); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x7F, 0x80); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x43, 0x08); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x4B, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x6B, 0x0F); if(ret < 0) goto exit; ///< CLKPO3/CLKNO3 off
    ret = tw2868_i2c_write(id, 0x6C, 0x0F); if(ret < 0) goto exit; ///< CLKPO4/CLKNO4 off
    ret = tw2868_i2c_write(id, 0xCF, 0x80); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xCF, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x89, 0x05); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x70, 0x08); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xD0, 0x88); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xD1, 0x88); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x7F, 0x80); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFC, 0xFF); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x73, 0x01); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFB, 0x3F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE1, 0xE0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE2, 0x22); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE3, 0x22); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x7E, 0xA2); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB3, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB4, 0x1C); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB5, 0x1D); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB6, 0x1A); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB7, 0x1B); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x75, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x76, 0x19); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x82, 0x80); if(ret < 0) goto exit;
    /* NTSC Common  */
    ret = tw2868_i2c_write(id, 0x08, 0x12); if(ret < 0) goto exit;  ///< VIN1
    ret = tw2868_i2c_write(id, 0x09, 0xF0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0A, 0x0A); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0E, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0F, 0x7F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x18, 0x12); if(ret < 0) goto exit;  ///< VIN2
    ret = tw2868_i2c_write(id, 0x19, 0xF0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1A, 0x0A); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1E, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1F, 0x7F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x28, 0x12); if(ret < 0) goto exit;  ///< VIN3
    ret = tw2868_i2c_write(id, 0x29, 0xF0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2A, 0x0A); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2E, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2F, 0x7F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x38, 0x12); if(ret < 0) goto exit;  ///< VIN4
    ret = tw2868_i2c_write(id, 0x39, 0xF0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3A, 0x0A); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3E, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3F, 0x7F); if(ret < 0) goto exit;
    /* BANK 1 */
    ret = tw2868_i2c_write(id, 0x40, 0x01); if(ret < 0) goto exit;  ///< set to page#1
    /* Common */
    ret = tw2868_i2c_write(id, 0xFC, 0xFF); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x73, 0x01); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xDD, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xDE, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE1, 0xC0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE2, 0xAA); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE3, 0xAA); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xAA, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x60, 0x55); if(ret < 0) goto exit;  ///< Video Output Mode
    ret = tw2868_i2c_write(id, 0x7F, 0x80); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xD0, 0x88); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xD1, 0x88); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x7F, 0x80); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFC, 0xFF); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x73, 0x01); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE1, 0xE0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE2, 0x22); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE3, 0x22); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x7E, 0xA2); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB3, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB4, 0x1C); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB5, 0x1D); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB6, 0x1A); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB7, 0x1B); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x75, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x76, 0x19); if(ret < 0) goto exit;
    /* NTSC Common  */
    ret = tw2868_i2c_write(id, 0x08, 0x12); if(ret < 0) goto exit;  ///< VIN5
    ret = tw2868_i2c_write(id, 0x09, 0xF0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0A, 0x0A); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0E, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0F, 0x7F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x18, 0x12); if(ret < 0) goto exit;  ///< VIN6
    ret = tw2868_i2c_write(id, 0x19, 0xF0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1A, 0x0A); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1E, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1F, 0x7F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x28, 0x12); if(ret < 0) goto exit;  ///< VIN7
    ret = tw2868_i2c_write(id, 0x29, 0xF0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2A, 0x0A); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2E, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2F, 0x7F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x38, 0x12); if(ret < 0) goto exit;  ///< VIN8
    ret = tw2868_i2c_write(id, 0x39, 0xF0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3A, 0x0A); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3E, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3F, 0x7F); if(ret < 0) goto exit;

    /* BANK 0 */
    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2868_i2c_write(id, 0x97, 0x85); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFB, 0x2F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFC, 0xFF); if(ret < 0) goto exit;

exit:
    return ret;
}

static int tw2868_pal_common_init(int id)
{
    int ret;

    if(id >= TW2868_DEV_MAX)
        return -1;

    /* BANK 0 */
    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0
    /* Common */
    ret = tw2868_i2c_write(id, 0xFA, 0x40); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFB, 0x2F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFC, 0xFF); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xDB, 0xC1); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x73, 0x01); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x9C, 0xA0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x9E, 0x52); if(ret < 0) goto exit;  ///< NOVID
    ret = tw2868_i2c_write(id, 0xD2, 0x01); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xDD, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xDE, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE1, 0xC0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE2, 0xAA); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE3, 0xAA); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xF8, 0x64); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xF9, 0x11); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xAA, 0x00); if(ret < 0) goto exit;

    ret = tw2868_i2c_write(id, 0x5B, 0xFF); if(ret < 0) goto exit;  ///< Output Pin High Drive

    ret = tw2868_i2c_write(id, 0x60, 0x15); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x61, 0x13); if(ret < 0) goto exit;

    ret = tw2868_i2c_write(id, 0x9F, 0x33); if(ret < 0) goto exit;    

    ret = tw2868_i2c_write(id, 0x70, 0x08); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xF8, 0xC4); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xF9, 0x51); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xDF, 0x80); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x7F, 0x80); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x43, 0x08); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x4B, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x6B, 0x0F); if(ret < 0) goto exit; ///< CLKPO3/CLKNO3 off
    ret = tw2868_i2c_write(id, 0x6C, 0x0F); if(ret < 0) goto exit; ///< CLKPO4/CLKNO4 off
    ret = tw2868_i2c_write(id, 0xCF, 0x80); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xCF, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x89, 0x05); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x70, 0x08); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xD0, 0x88); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xD1, 0x88); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x7F, 0x80); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFC, 0xFF); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x73, 0x01); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFB, 0x3F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE1, 0xE0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE2, 0x22); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE3, 0x22); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x7E, 0xA2); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB3, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB4, 0x1C); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB5, 0x1D); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB6, 0x1A); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB7, 0x1B); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x75, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x76, 0x19); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x82, 0x80); if(ret < 0) goto exit;
    /* PAL Common  */
    ret = tw2868_i2c_write(id, 0x08, 0x17); if(ret < 0) goto exit;  ///< VIN1
    ret = tw2868_i2c_write(id, 0x09, 0x20); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0A, 0x0C); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0E, 0x07); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0F, 0x7F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x18, 0x17); if(ret < 0) goto exit;  ///< VIN2
    ret = tw2868_i2c_write(id, 0x19, 0x20); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1A, 0x0C); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1E, 0x07); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1F, 0x7F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x28, 0x17); if(ret < 0) goto exit;  ///< VIN3
    ret = tw2868_i2c_write(id, 0x29, 0x20); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2A, 0x0C); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2E, 0x07); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2F, 0x7F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x38, 0x17); if(ret < 0) goto exit;  ///< VIN4
    ret = tw2868_i2c_write(id, 0x39, 0x20); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3A, 0x0C); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3E, 0x07); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3F, 0x7F); if(ret < 0) goto exit;

    /* BANK 1 */
    ret = tw2868_i2c_write(id, 0x40, 0x01); if(ret < 0) goto exit;  ///< set to page#1
    /* Common */
    ret = tw2868_i2c_write(id, 0xFC, 0xFF); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x73, 0x01); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xDD, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xDE, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE1, 0xC0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE2, 0xAA); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE3, 0xAA); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xAA, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x60, 0x55); if(ret < 0) goto exit;  ///< Video Output Mode
    ret = tw2868_i2c_write(id, 0x7F, 0x80); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xD0, 0x88); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xD1, 0x88); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x7F, 0x80); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFC, 0xFF); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x73, 0x01); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE1, 0xE0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE2, 0x22); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xE3, 0x22); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x7E, 0xA2); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB3, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB4, 0x1C); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB5, 0x1D); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB6, 0x1A); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xB7, 0x1B); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x75, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x76, 0x19); if(ret < 0) goto exit;
    /* PAL Common  */
    ret = tw2868_i2c_write(id, 0x08, 0x17); if(ret < 0) goto exit;  ///< VIN5
    ret = tw2868_i2c_write(id, 0x09, 0x20); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0A, 0x0C); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0E, 0x07); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x0F, 0x7F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x18, 0x17); if(ret < 0) goto exit;  ///< VIN6
    ret = tw2868_i2c_write(id, 0x19, 0x20); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1A, 0x0C); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1E, 0x07); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x1F, 0x7F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x28, 0x17); if(ret < 0) goto exit;  ///< VIN7
    ret = tw2868_i2c_write(id, 0x29, 0x20); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2A, 0x0C); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2E, 0x07); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x2F, 0x7F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x38, 0x17); if(ret < 0) goto exit;  ///< VIN8
    ret = tw2868_i2c_write(id, 0x39, 0x20); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3A, 0x0C); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3B, 0xD0); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3C, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3D, 0x00); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3E, 0x07); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0x3F, 0x7F); if(ret < 0) goto exit;
    /* BANK 0 */
    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2868_i2c_write(id, 0x97, 0xC5); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFB, 0x2F); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFC, 0xFF); if(ret < 0) goto exit;
    
exit:
    return ret;
}

static int tw2868_ntsc_720h_4ch(int id)
{
    int ret;

    if(id >= TW2868_DEV_MAX)
        return -1;

    ret = tw2868_ntsc_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 1 */
    ret = tw2868_i2c_write(id, 0x40, 0x01); if(ret < 0) goto exit;  ///< set to page1
    ret = tw2868_i2c_write(id, 0x60, 0xAA); if(ret < 0) goto exit;  ///< 720H 108MHz VD1/VD2/VD3/VD4

    ret = tw2868_i2c_write(id, 0x61, 0x10); if(ret < 0) goto exit;  ///< VD1
    ret = tw2868_i2c_write(id, 0x62, 0x32); if(ret < 0) goto exit;  ///< VD1
    ret = tw2868_i2c_write(id, 0x63, 0x54); if(ret < 0) goto exit;  ///< VD2
    ret = tw2868_i2c_write(id, 0x64, 0x76); if(ret < 0) goto exit;  ///< VD2
    ret = tw2868_i2c_write(id, 0x65, 0x10); if(ret < 0) goto exit;  ///< VD3
    ret = tw2868_i2c_write(id, 0x66, 0x32); if(ret < 0) goto exit;  ///< VD3
    ret = tw2868_i2c_write(id, 0x67, 0x54); if(ret < 0) goto exit;  ///< VD4
    ret = tw2868_i2c_write(id, 0x68, 0x76); if(ret < 0) goto exit;  ///< VD4

    /* BANK 0 */
    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2868_i2c_write(id, 0x08, 0x14); if(ret < 0) goto exit;  ///< VDELAY_LO for VIN1
    ret = tw2868_i2c_write(id, 0x18, 0x14); if(ret < 0) goto exit;  ///< VDELAY_LO for VIN2
    ret = tw2868_i2c_write(id, 0x28, 0x14); if(ret < 0) goto exit;  ///< VDELAY_LO for VIN3
    ret = tw2868_i2c_write(id, 0x38, 0x14); if(ret < 0) goto exit;  ///< VDELAY_LO for VIN4
    ret = tw2868_i2c_write(id, 0xFA, 0x4A); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFB, 0x2F); if(ret < 0) goto exit;

    printk("TW2868#%d ==> NTSC 720H 108MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2868_pal_720h_4ch(int id)
{
    int ret;

    if(id >= TW2868_DEV_MAX)
        return -1;

    ret = tw2868_pal_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 1 */
    ret = tw2868_i2c_write(id, 0x40, 0x01); if(ret < 0) goto exit;  ///< set to page1
    ret = tw2868_i2c_write(id, 0x60, 0xAA); if(ret < 0) goto exit;  ///< 720H 108MHz VD1/VD2/VD3/VD4

    ret = tw2868_i2c_write(id, 0x61, 0x10); if(ret < 0) goto exit;  ///< VD1
    ret = tw2868_i2c_write(id, 0x62, 0x32); if(ret < 0) goto exit;  ///< VD1
    ret = tw2868_i2c_write(id, 0x63, 0x54); if(ret < 0) goto exit;  ///< VD2
    ret = tw2868_i2c_write(id, 0x64, 0x76); if(ret < 0) goto exit;  ///< VD2
    ret = tw2868_i2c_write(id, 0x65, 0x10); if(ret < 0) goto exit;  ///< VD3
    ret = tw2868_i2c_write(id, 0x66, 0x32); if(ret < 0) goto exit;  ///< VD3
    ret = tw2868_i2c_write(id, 0x67, 0x54); if(ret < 0) goto exit;  ///< VD4
    ret = tw2868_i2c_write(id, 0x68, 0x76); if(ret < 0) goto exit;  ///< VD4

    /* BANK 0 */
    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2868_i2c_write(id, 0xFA, 0x4A); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFB, 0x2F); if(ret < 0) goto exit;

    printk("TW2868#%d ==> PAL 720H 108MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2868_ntsc_720h_2ch(int id)
{
    int ret;

    if(id >= TW2868_DEV_MAX)
        return -1;

    ret = tw2868_ntsc_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 1 */
    ret = tw2868_i2c_write(id, 0x40, 0x01); if(ret < 0) goto exit;  ///< set to page1
    ret = tw2868_i2c_write(id, 0x60, 0x55); if(ret < 0) goto exit;  ///< 720H 54MHz VD1/VD2/VD3/VD4

    ret = tw2868_i2c_write(id, 0x61, 0x10); if(ret < 0) goto exit;  ///< VD1
    ret = tw2868_i2c_write(id, 0x62, 0x32); if(ret < 0) goto exit;  ///< VD1
    ret = tw2868_i2c_write(id, 0x63, 0x32); if(ret < 0) goto exit;  ///< VD2
    ret = tw2868_i2c_write(id, 0x64, 0x54); if(ret < 0) goto exit;  ///< VD2
    ret = tw2868_i2c_write(id, 0x65, 0x54); if(ret < 0) goto exit;  ///< VD3
    ret = tw2868_i2c_write(id, 0x66, 0x76); if(ret < 0) goto exit;  ///< VD3
    ret = tw2868_i2c_write(id, 0x67, 0x76); if(ret < 0) goto exit;  ///< VD4
    ret = tw2868_i2c_write(id, 0x68, 0x10); if(ret < 0) goto exit;  ///< VD4

    /* BANK 0 */
    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2868_i2c_write(id, 0xFA, 0x40); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFB, 0x2F); if(ret < 0) goto exit;

    printk("TW2868#%d ==> NTSC 720H 54MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2868_pal_720h_2ch(int id)
{
    int ret;

    if(id >= TW2868_DEV_MAX)
        return -1;

    ret = tw2868_pal_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 1 */
    ret = tw2868_i2c_write(id, 0x40, 0x01); if(ret < 0) goto exit;  ///< set to page1
    ret = tw2868_i2c_write(id, 0x60, 0x55); if(ret < 0) goto exit;  ///< 720H 54MHz VD1/VD2/VD3/VD4

    ret = tw2868_i2c_write(id, 0x61, 0x10); if(ret < 0) goto exit;  ///< VD1
    ret = tw2868_i2c_write(id, 0x62, 0x32); if(ret < 0) goto exit;  ///< VD1
    ret = tw2868_i2c_write(id, 0x63, 0x32); if(ret < 0) goto exit;  ///< VD2
    ret = tw2868_i2c_write(id, 0x64, 0x54); if(ret < 0) goto exit;  ///< VD2
    ret = tw2868_i2c_write(id, 0x65, 0x54); if(ret < 0) goto exit;  ///< VD3
    ret = tw2868_i2c_write(id, 0x66, 0x76); if(ret < 0) goto exit;  ///< VD3
    ret = tw2868_i2c_write(id, 0x67, 0x76); if(ret < 0) goto exit;  ///< VD4
    ret = tw2868_i2c_write(id, 0x68, 0x10); if(ret < 0) goto exit;  ///< VD4

    /* BANK 0 */
    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2868_i2c_write(id, 0xFA, 0x40); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFB, 0x2F); if(ret < 0) goto exit;

    printk("TW2868#%d ==> PAL 720H 54MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2868_ntsc_720h_1ch(int id)
{
    int ret;

    if(id >= TW2868_DEV_MAX)
        return -1;

    ret = tw2868_ntsc_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 1 */
    ret = tw2868_i2c_write(id, 0x40, 0x01); if(ret < 0) goto exit;  ///< set to page1
    ret = tw2868_i2c_write(id, 0x60, 0x00); if(ret < 0) goto exit;  ///< 720H 27MHz VD1/VD2/VD3/VD4

    ret = tw2868_i2c_write(id, 0x61, 0x00); if(ret < 0) goto exit;  ///< VD1
    ret = tw2868_i2c_write(id, 0x62, 0x00); if(ret < 0) goto exit;  ///< VD1
    ret = tw2868_i2c_write(id, 0x63, 0x11); if(ret < 0) goto exit;  ///< VD2
    ret = tw2868_i2c_write(id, 0x64, 0x11); if(ret < 0) goto exit;  ///< VD2
    ret = tw2868_i2c_write(id, 0x65, 0x22); if(ret < 0) goto exit;  ///< VD3
    ret = tw2868_i2c_write(id, 0x66, 0x22); if(ret < 0) goto exit;  ///< VD3
    ret = tw2868_i2c_write(id, 0x67, 0x33); if(ret < 0) goto exit;  ///< VD4
    ret = tw2868_i2c_write(id, 0x68, 0x33); if(ret < 0) goto exit;  ///< VD4

    /* BANK 0 */
    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2868_i2c_write(id, 0xFA, 0x40); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFB, 0x2F); if(ret < 0) goto exit;

    printk("TW2868#%d ==> NTSC 720H 27MHz MODE!!\n", id);

exit:
    return ret;
}

static int tw2868_pal_720h_1ch(int id)
{
    int ret;

    if(id >= TW2868_DEV_MAX)
        return -1;

    ret = tw2868_pal_common_init(id);
    if(ret < 0)
        goto exit;

    /* BANK 1 */
    ret = tw2868_i2c_write(id, 0x40, 0x01); if(ret < 0) goto exit;  ///< set to page1
    ret = tw2868_i2c_write(id, 0x60, 0x00); if(ret < 0) goto exit;  ///< 720H 27MHz VD1/VD2/VD3/VD4

    ret = tw2868_i2c_write(id, 0x61, 0x00); if(ret < 0) goto exit;  ///< VD1
    ret = tw2868_i2c_write(id, 0x62, 0x00); if(ret < 0) goto exit;  ///< VD1
    ret = tw2868_i2c_write(id, 0x63, 0x11); if(ret < 0) goto exit;  ///< VD2
    ret = tw2868_i2c_write(id, 0x64, 0x11); if(ret < 0) goto exit;  ///< VD2
    ret = tw2868_i2c_write(id, 0x65, 0x22); if(ret < 0) goto exit;  ///< VD3
    ret = tw2868_i2c_write(id, 0x66, 0x22); if(ret < 0) goto exit;  ///< VD3
    ret = tw2868_i2c_write(id, 0x67, 0x33); if(ret < 0) goto exit;  ///< VD4
    ret = tw2868_i2c_write(id, 0x68, 0x33); if(ret < 0) goto exit;  ///< VD4

    /* BANK 0 */
    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page0
    ret = tw2868_i2c_write(id, 0xFA, 0x40); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xFB, 0x2F); if(ret < 0) goto exit;

    printk("TW2868#%d ==> PAL 720H 27MHz MODE!!\n", id);

exit:
    return ret;
}

int tw2868_video_set_mode(int id, TW2868_VMODE_T mode)
{
    int ret;

    if(id >= TW2868_DEV_MAX || mode >= TW2868_VMODE_MAX)
        return -1;

    switch(mode) {
        case TW2868_VMODE_NTSC_720H_1CH:
            ret = tw2868_ntsc_720h_1ch(id);
            break;
        case TW2868_VMODE_NTSC_720H_2CH:
            ret = tw2868_ntsc_720h_2ch(id);
            break;
        case TW2868_VMODE_NTSC_720H_4CH:
            ret = tw2868_ntsc_720h_4ch(id);
            break;

        case TW2868_VMODE_PAL_720H_1CH:
            ret = tw2868_pal_720h_1ch(id);
            break;
        case TW2868_VMODE_PAL_720H_2CH:
            ret = tw2868_pal_720h_2ch(id);
            break;
        case TW2868_VMODE_PAL_720H_4CH:
            ret = tw2868_pal_720h_4ch(id);
            break;

        default:
            ret = -1;
            printk("TW2868#%d driver not support video output mode(%d)!!\n", id, mode);
            break;
    }

    /* Update VMode */
    if(ret == 0)
        tw2868_vmode[id] = mode;

    return ret;
}
EXPORT_SYMBOL(tw2868_video_set_mode);

int tw2868_video_get_mode(int id)
{
    if(id >= TW2868_DEV_MAX)
        return -1;

    return tw2868_vmode[id];
}
EXPORT_SYMBOL(tw2868_video_get_mode);

int tw2868_video_mode_support_check(int id, TW2868_VMODE_T mode)
{
    if(id >= TW2868_DEV_MAX)
        return 0;

    switch(mode) {
        case TW2868_VMODE_NTSC_720H_1CH:
        case TW2868_VMODE_NTSC_720H_2CH:
        case TW2868_VMODE_NTSC_720H_4CH:
        case TW2868_VMODE_PAL_720H_1CH:
        case TW2868_VMODE_PAL_720H_2CH:
        case TW2868_VMODE_PAL_720H_4CH:
            return 1;
        default:
            return 0;
    }

    return 0;
}
EXPORT_SYMBOL(tw2868_video_mode_support_check);

int tw2868_video_set_contrast(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2868_DEV_MAX || ch >= TW2868_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2868_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2868_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2868_i2c_write(id, 0x02+(0x10*(ch%4)), value);

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_video_set_contrast);

int tw2868_video_get_contrast(int id, int ch)
{
    int ret;

    if(id >= TW2868_DEV_MAX || ch >= TW2868_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2868_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2868_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2868_i2c_read(id, 0x02+(0x10*(ch%4)));

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_video_get_contrast);

int tw2868_video_set_brightness(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2868_DEV_MAX || ch >= TW2868_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2868_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2868_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2868_i2c_write(id, 0x01+(0x10*(ch%4)), value);

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_video_set_brightness);

int tw2868_video_get_brightness(int id, int ch)
{
    int ret = 0;

    if(id >= TW2868_DEV_MAX || ch >= TW2868_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2868_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2868_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2868_i2c_read(id, 0x01+(0x10*(ch%4)));

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_video_get_brightness);

int tw2868_video_set_saturation_u(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2868_DEV_MAX || ch >= TW2868_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2868_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2868_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2868_i2c_write(id, 0x04+(0x10*(ch%4)), value);

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_video_set_saturation_u);

int tw2868_video_get_saturation_u(int id, int ch)
{
    int ret;

    if(id >= TW2868_DEV_MAX || ch >= TW2868_DEV_CH_MAX)
        return -1;


    if(ch < 4) {
        /* BANK 0 */
        ret = tw2868_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2868_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2868_i2c_read(id, 0x04+(0x10*(ch%4)));

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_video_get_saturation_u);

int tw2868_video_set_saturation_v(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2868_DEV_MAX || ch >= TW2868_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2868_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2868_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2868_i2c_write(id, 0x05+(0x10*(ch%4)), value);

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_video_set_saturation_v);

int tw2868_video_get_saturation_v(int id, int ch)
{
    int ret;

    if(id >= TW2868_DEV_MAX || ch >= TW2868_DEV_CH_MAX)
        return -1;


    if(ch < 4) {
        /* BANK 0 */
        ret = tw2868_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2868_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2868_i2c_read(id, 0x05+(0x10*(ch%4)));

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_video_get_saturation_v);

int tw2868_video_set_hue(int id, int ch, u8 value)
{
    int ret;

    if(id >= TW2868_DEV_MAX || ch >= TW2868_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2868_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2868_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2868_i2c_write(id, 0x06+(0x10*(ch%4)), value);

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_video_set_hue);

int tw2868_video_get_hue(int id, int ch)
{
    int ret;

    if(id >= TW2868_DEV_MAX || ch >= TW2868_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2868_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;
    }
    else {
        /* BANK 1 */
        ret = tw2868_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;
    }

    ret = tw2868_i2c_read(id, 0x06+(0x10*(ch%4)));

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_video_get_hue);

int tw2868_status_get_novid(int id, int ch)
{
    int ret;

    if(id >= TW2868_DEV_MAX || ch >= TW2868_DEV_CH_MAX)
        return -1;

    if(ch < 4) {
        /* BANK 0 */
        ret = tw2868_i2c_write(id, 0x40, 0x00);
        if(ret < 0)
            goto exit;

        ret = (tw2868_i2c_read(id, 0x10*ch)>>7) & 0x1;
    }
    else {
        /* BANK 1 */
        ret = tw2868_i2c_write(id, 0x40, 0x01);
        if(ret < 0)
            goto exit;

        ret = (tw2868_i2c_read(id, 0x10*(ch-4))>>7) & 0x1;
    }

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_status_get_novid);

int tw2868_audio_set_mute(int id, int on)
{
    int data;
    int ret;
    /* set DAOGAIN: BIT4 DAORATIO = 1, DAOGAIN = DAOGAIN/64, BIT4 DAORATIO = 0, DAOGAIN = DAOGAIN
       set DAORATIO = 1 ==> DAOGAIN = DAOGAIN/64, SET DAOGAIN = 0 ==> 0/64 = 0 */
    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0
    data = tw2868_i2c_read(id, 0x72);
    data &= 0xe0;
    data |= 0x10;
    ret = tw2868_i2c_write(id, 0x72, data); if(ret < 0) goto exit;     ///< DAOGAIN mute

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_audio_set_mute);

int tw2868_audio_get_mute(int id)
{
    int data, mute;
    int ret;

    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0
    data = tw2868_i2c_read(id, 0x72);
    if ((data & 0x10) == 0x10)
        mute = 1;
    else
        mute = 0;

    return mute;

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_audio_get_mute);

int tw2868_audio_get_volume(int id)
{
    int aogain;
    int ret;

    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0

    aogain = tw2868_i2c_read(id, 0x72);
    if ((aogain & 0x10) == 0x10)
        aogain = 0;
    else
        aogain &= 0xf;

    return aogain;

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_audio_get_volume);

int tw2868_audio_set_volume(int id, int volume)
{
    int data;
    int ret;

    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0
    data = tw2868_i2c_read(id, 0x72);
    data &= 0xE0;
    data |= volume;
    ret = tw2868_i2c_write(id, 0x72, data); if(ret < 0) goto exit;     ///< DAOGAIN setting

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_audio_set_volume);

int tw2868_audio_get_output_ch(int id)
{
    int ch;
    int ret;

    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0

    ch = tw2868_i2c_read(id, 0xe0);
    ch &= 0x1f;

    return ch;

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_audio_get_output_ch);

int tw2868_audio_set_output_ch(int id, int ch)
{
    int data;
    int ret;

    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0

    data = tw2868_i2c_read(id, 0xe0);
    data &= 0xe0;
    data |= ch;
    ret = tw2868_i2c_write(id, 0xe0, data); if(ret < 0) goto exit;              ///< MIX_OUTSEL

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_audio_set_output_ch);

int tw2868_swap_channels(int id, int num_of_ch)
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
    ret = tw2868_write_table(id, 0xD0, tmp_memory, sizeof(tbl_pg0_sfr2));

    return ret;
}

/* set sample rate */
int tw2868_audio_set_sample_rate(int id, TW2868_SAMPLERATE_T samplerate)
{
    int ret;
    int data;

    if (samplerate > TW2868_AUDIO_SAMPLERATE_48K) {
        printk("sample rate exceed tw2868 fs table \n");
        return -1;
    }

    data = tw2868_i2c_read(id, 0x70);
    data &= ~0xF;
    data |= 0x1 << 3;        /* AFAUTO */

    switch (samplerate) {
    default:
    case TW2868_AUDIO_SAMPLERATE_8K:
        data |= 0;
        break;
    case TW2868_AUDIO_SAMPLERATE_16K:
        data |= 1;
        break;
    case TW2868_AUDIO_SAMPLERATE_32K:
        data |= 2;
        break;
    case TW2868_AUDIO_SAMPLERATE_44K:
        data |= 3;
        break;
    case TW2868_AUDIO_SAMPLERATE_48K:
        data |= 4;
        break;
    }

    ret = tw2868_i2c_write(id, 0x70, data);

    return ret;
}

int tw2868_audio_set_sample_size(int id, TW2868_SAMPLESIZE_T samplesize)
{
    uint8_t data = 0;
    int ret = 0;

    if ((samplesize != TW2868_AUDIO_BITS_8B) && (samplesize != TW2868_AUDIO_BITS_16B)) {
        printk("%s fails: due to wrong sampling size = %d\n", __FUNCTION__, samplesize);
        return -1;
    }

    data = tw2868_i2c_read(id, 0xDF);
    data = 0x80;
    ret = tw2868_i2c_write(id, 0xDF, data); if(ret < 0) goto exit;

    data = tw2868_i2c_read(id, 0xDB);
    if (samplesize == TW2868_AUDIO_BITS_8B) {
        data = data | 0x04;
    } else if (samplesize == TW2868_AUDIO_BITS_16B) {
        data = data & (~0x04);
    }
    ret = tw2868_i2c_write(id, 0xDB, data); if(ret < 0) goto exit;

exit:
    return ret;
}

static int tw2868_audio_set_master_chan_seq(int id, char *RSEQ)
{
    u16 data;
    int ret;

    data = RSEQ[0];
    ret = tw2868_i2c_write(id, 0xD3, data); if(ret < 0) goto exit;
    data = RSEQ[1];
    ret = tw2868_i2c_write(id, 0xD4, data); if(ret < 0) goto exit;
    data = RSEQ[2];
    ret = tw2868_i2c_write(id, 0xD5, data); if(ret < 0) goto exit;
    data = RSEQ[3];
    ret = tw2868_i2c_write(id, 0xD6, data); if(ret < 0) goto exit;
    data = RSEQ[4];
    ret = tw2868_i2c_write(id, 0xD7, data); if(ret < 0) goto exit;
    data = RSEQ[5];
    ret = tw2868_i2c_write(id, 0xD8, data); if(ret < 0) goto exit;
    data = RSEQ[6];
    ret = tw2868_i2c_write(id, 0xD9, data); if(ret < 0) goto exit;
    data = RSEQ[7];
    ret = tw2868_i2c_write(id, 0xDA, data); if(ret < 0) goto exit;
exit:
    return ret;
}

int tw2868_audio_init(int id, int total_channels)
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
    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;      ///< set to page#0
    if (cascade == 1)
    ret = tw2868_i2c_write(id, 0xCF, 0x80); if(ret < 0) goto exit;      ///< CASCADE MODE

    data = tw2868_i2c_read(id, 0xFB);                                   ///< define audio detection mode
    data |= 0xc;
    ret = tw2868_i2c_write(id, 0xFB, data); if(ret < 0) goto exit;

    data = tw2868_i2c_read(id, 0xFC);                                   ///< enable audio detection AIN1234
    data |= 0xf0;
    ret = tw2868_i2c_write(id, 0xFC, data); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xDB, 0xC1); if(ret < 0) goto exit;      ///< master control: MASTER MODE

    if (audio_chnum == 8)
        ret = tw2868_i2c_write(id, 0xD2, 0x02); if(ret < 0) goto exit;  ///< 8 channel record, first stage playback
    if (audio_chnum == 16)
        ret = tw2868_i2c_write(id, 0xD2, 0x03); if(ret < 0) goto exit;  ///< 16 channel record, first stage playback

    ret = tw2868_i2c_write(id, 0xE0, 0x10); if(ret < 0) goto exit;      ///< MIX_OUTSEL = playback first stage
    ret = tw2868_i2c_write(id, 0x70, 0x88); if(ret < 0) goto exit;      ///< AFAUTO=1, sample rate = 8k

    data = tw2868_i2c_read(id, 0x89);                                   ///< AFS384=0/AIN5MD=0
    data &= 0xf0;
    tw2868_i2c_write(id, 0x89, data); if(ret < 0) goto exit;
    // if sample rate < 32k, bit6 = 1, sampel rate >= 32k, bit6 = 0
    data = tw2868_i2c_read(id, 0x71);                                   ///< MASCKMD = 1
    data |= (1 << 6);
    ret = tw2868_i2c_write(id, 0x71, data); if(ret < 0) goto exit;

    ret = tw2868_i2c_write(id, 0xDC, 0x20); if(ret < 0) goto exit;      ///< mix mute control AIN1/2/3/4 not muted

    for(i = 0; i < 4; i++) {
        j = i * 2;
        ch = tw2868_vin_to_ch(id, j);
        ch1 = tw2868_vin_to_ch(id, j+1);
        RSEQ[i] = (ch1 << 4 | ch);

        ch = tw2868_vin_to_ch(id + 1, j);
        ch1 = tw2868_vin_to_ch(id + 1, j+1);
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
        ret = tw2868_audio_set_master_chan_seq(id, RSEQ);
    // when cascade, slave configure should as same as master
    if (cascade == 1) {
        if ((id == 0) || (id == 2)) {
            ret = tw2868_audio_set_master_chan_seq(id, RSEQ);
            ret = tw2868_audio_set_master_chan_seq(id + 1, RSEQ);
        }
    }

    /* BANK 1 */
    ret = tw2868_i2c_write(id, 0x40, 0x01); if(ret < 0) goto exit;  ///< set to page#1
    data = tw2868_i2c_read(id, 0xFC);                               ///< enable audio detection AIN5678
    data |= 0xf0;
    ret = tw2868_i2c_write(id, 0xFC, data); if(ret < 0) goto exit;
    ret = tw2868_i2c_write(id, 0xDC, 0x20); if(ret < 0) goto exit;  ///< mix mute control AIN5/6/7/8 not muted

    ret = tw2868_i2c_write(id, 0x40, 0x00); if(ret < 0) goto exit;  ///< set to page#0

exit:
    return ret;
}

int tw2868_audio_set_mode(int id, TW2868_VMODE_T mode, TW2868_SAMPLESIZE_T samplesize, TW2868_SAMPLERATE_T samplerate)
{
    int ch_num = 0;
    int ret;

    ch_num = audio_chnum;
    if(ch_num < 0){
        return -1;
    }

    /* tw2868 audio initialize */
    ret = tw2868_audio_init(id, ch_num); if(ret < 0) goto exit;

    /* set audio sample rate */
    ret = tw2868_audio_set_sample_rate(id, samplerate); if(ret < 0) goto exit;

    /* set audio sample size */
    ret = tw2868_audio_set_sample_size(id, samplesize); if(ret < 0) goto exit;

exit:
    return ret;
}
EXPORT_SYMBOL(tw2868_audio_set_mode);
