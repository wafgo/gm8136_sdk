/**
 * @file nvp1104_lib.c
 * NextChip NVP1104 4-CH 720H Video Decoders and Audio Codecs Library
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.2 $
 * $Date: 2014/07/14 03:21:19 $
 *
 * ChangeLog:
 *  $Log: nvp1104_lib.c,v $
 *  Revision 1.2  2014/07/14 03:21:19  jerson_l
 *  1. support dynamic switch video mode
 *  2. support setup sharpness
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

#include "nextchip/nvp1104.h"        ///< from module/include/front_end/decoder

static unsigned char NVP1104_PAL[] = {
    //  0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F
/*00*/  0x00,0x00,0x00,0x00,0x00,0x10,0x32,0x10,0x32,0x10,0x32,0x10,0x32,0x00,0x00,0x00,
/*10*/  0x00,0xef,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*20*/  0xbd,0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x07,0x10,0xac,0x00,0x00,0x10,0x79,0x06,
/*30*/  0xbd,0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x07,0x10,0xac,0x00,0x00,0x10,0x04,0x11,
/*40*/  0xbd,0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x07,0x10,0xac,0x00,0x00,0x10,0x04,0x22,
/*50*/  0xbd,0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x07,0x10,0xac,0x00,0x00,0x10,0x04,0x33,
/*60*/  0xd0,0x80,0x40,0x7c,0x9f,0x00,0x20,0x40,0x80,0x50,0x38,0x0f,0x0c,0x01,0x15,0x0a,
/*70*/  0x89,0x23,0x88,0x04,0x2a,0xcc,0xf0,0x2F,0x57,0x43,0x10,0x88,0x01,0x63,0x01,0x00,
/*80*/  0x80,0x00,0x00,0x81,0x01,0x00,0x00,0x00,0x00,0x20,0x04,0x2e,0x00,0x30,0xd8,0x01,
/*90*/  0x06,0x06,0x11,0xb9,0xb2,0x05,0x00,0x28,0x50,0x01,0xa8,0x13,0x03,0x22,0x00,0x00,
/*A0*/  0x00,0x00,0x22,0x88,0x88,0x84,0x33,0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0x04,
/*B0*/  0x00,0x88,0x88,0x88,0x14,0x0f,0xaa,0xaa,0x02,0x00,0x80,0x00,0x00,0xc9,0x0F,0x18,
/*C0*/  0x13,0x13,0x13,0x13,0x00,0x00,0x71,0x71,0x71,0x71,0x1c,0x1c,0x1c,0x1c,0x87,0x87,
/*D0*/  0x87,0x87,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x10,0x80,0x10,0x00,0x00,
/*E0*/  0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x10,0x24,0x01,0x4B,0x00,0x00,0x00,0x00,
/*F0*/  0x00,0x00,0xa0,0x04,0x00,0x00,0x80,0x00,0x7B,0x20,0x0f,0x80,0x80,0x49,0x37,0x00
};

static unsigned char NVP1104_NTSC[] = {
    //  0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F
/*00*/  0x00,0x00,0x00,0x00,0x00,0x10,0x32,0x10,0x32,0x10,0x32,0x10,0x32,0x00,0x00,0x00,
/*10*/  0x00,0xef,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*20*/  0x60,0x00,0x00,0x02,0x9f,0x00,0x00,0x00,0x06,0x10,0x95,0x00,0x00,0x8b,0x10,0x00,
/*30*/  0x60,0x00,0x00,0x02,0x9f,0x00,0x00,0x00,0x06,0x10,0x95,0x00,0x00,0x8b,0x10,0x11,
/*40*/  0x60,0x00,0x00,0x02,0x9f,0x00,0x00,0x00,0x06,0x10,0x95,0x00,0x00,0x8b,0x10,0x22,
/*50*/  0x60,0x00,0x00,0x02,0x9f,0x00,0x00,0x00,0x06,0x10,0x95,0x00,0x00,0x8b,0x10,0x33,
/*60*/  0xd0,0x80,0x40,0x7c,0x9f,0x00,0x20,0x40,0x80,0x50,0x38,0x0f,0x0c,0x01,0x15,0x0a,
/*70*/  0x80,0x23,0x88,0x04,0x2a,0xcc,0xf0,0x2F,0x57,0x43,0x10,0x88,0x82,0x63,0x01,0x00,
/*80*/  0x80,0x00,0x00,0x81,0x01,0x00,0x00,0x00,0x00,0x20,0x04,0x2e,0x00,0x30,0xb8,0x01,
/*90*/  0x06,0x06,0x11,0xb9,0xb2,0x05,0x00,0x28,0x50,0x51,0xb5,0x13,0x03,0x22,0xff,0x00,
/*A0*/  0x00,0x00,0x22,0x88,0x88,0x84,0x33,0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0x04,
/*B0*/  0x00,0x88,0x88,0x88,0x14,0x0f,0xaa,0xaa,0x02,0x00,0x80,0x00,0x00,0xc9,0x0F,0x18,
/*C0*/  0x13,0x13,0x13,0x13,0x00,0x00,0x71,0x71,0x71,0x71,0x1c,0x1c,0x1c,0x1c,0x87,0x87,
/*D0*/  0x87,0x87,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x10,0x80,0x10,0x00,0x00,
/*E0*/  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x11,0x01,0x40,0x00,0x00,0x00,0x00,
/*F0*/  0x00,0x00,0xa0,0x04,0x00,0x00,0x80,0x00,0x69,0x20,0x0f,0x80,0x80,0x49,0x37,0x00
};

static int nvp1104_common_init(int id, NVP1104_VFMT_T vfmt)
{
    int ret;

    if(id >= NVP1104_DEV_MAX)
        return -1;

    switch(vfmt) {
        case NVP1104_VFMT_PAL:
            ret = nvp1104_i2c_array_write(id, 0x00, NVP1104_PAL, sizeof(NVP1104_PAL));
            break;
        default:
            ret = nvp1104_i2c_array_write(id, 0x00, NVP1104_NTSC, sizeof(NVP1104_NTSC));
            break;
    }

    return ret;
}

static int nvp1104_720h_1ch(int id, NVP1104_VFMT_T vfmt, NVP1104_OSC_CLKIN_T clkin)
{
    int ret = 0;

    if(id >= NVP1104_DEV_MAX)
        return -1;

    ret = nvp1104_common_init(id, vfmt);
    if(ret < 0)
        goto exit;

    switch(clkin) {
        case NVP1104_OSC_CLKIN_108MHZ:
            ret = nvp1104_i2c_write(id, 0x2e, 0x00); if(ret < 0) goto exit;   ///< 27MHz Channel 1 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x3e, 0x00); if(ret < 0) goto exit;   ///< 27MHz Channel 2 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x4e, 0x00); if(ret < 0) goto exit;   ///< 27MHz Channel 3 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x5e, 0x00); if(ret < 0) goto exit;   ///< 27MHz Channel 4 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x2f, 0x00); if(ret < 0) goto exit;   ///< Channel 1 output 27MHz, Channel ID:0
            ret = nvp1104_i2c_write(id, 0x3f, 0x11); if(ret < 0) goto exit;   ///< Channel 2 output 27MHz, Channel ID:1
            ret = nvp1104_i2c_write(id, 0x4f, 0x22); if(ret < 0) goto exit;   ///< Channel 3 output 27MHz, Channel ID:2
            ret = nvp1104_i2c_write(id, 0x5f, 0x33); if(ret < 0) goto exit;   ///< Channel 4 output 27MHz, Channel ID:3
            ret = nvp1104_i2c_write(id, 0x83, 0x81); if(ret < 0) goto exit;   ///< No Video Background Color => Black(75%), Y(001~254), Cb(001~254), Cr(001~254)
            ret = nvp1104_i2c_write(id, 0x84, 0x01); if(ret < 0) goto exit;   ///< Channel ID in EAV/SAV
            ret = nvp1104_i2c_write(id, 0xec, 0xc0); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf1, 0x00); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf2, 0x20); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf3, 0x04); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf6, 0x80); if(ret < 0) goto exit;   ///< using 108MHz oscillator
            ret = nvp1104_i2c_write(id, 0xfa, 0x00); if(ret < 0) goto exit;
            break;
        default:
            ret = nvp1104_i2c_write(id, 0x2e, 0x40); if(ret < 0) goto exit;   ///< 27MHz Channel 1 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x3e, 0x40); if(ret < 0) goto exit;   ///< 27MHz Channel 2 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x4e, 0x40); if(ret < 0) goto exit;   ///< 27MHz Channel 3 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x5e, 0x40); if(ret < 0) goto exit;   ///< 27MHz Channel 4 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x2f, 0x00); if(ret < 0) goto exit;   ///< Channel 1 output 27MHz, Channel ID:0
            ret = nvp1104_i2c_write(id, 0x3f, 0x11); if(ret < 0) goto exit;   ///< Channel 2 output 27MHz, Channel ID:1
            ret = nvp1104_i2c_write(id, 0x4f, 0x22); if(ret < 0) goto exit;   ///< Channel 3 output 27MHz, Channel ID:2
            ret = nvp1104_i2c_write(id, 0x5f, 0x33); if(ret < 0) goto exit;   ///< Channel 4 output 27MHz, Channel ID:3
            ret = nvp1104_i2c_write(id, 0x83, 0x81); if(ret < 0) goto exit;   ///< No Video Background Color => Black(75%), Y(001~254), Cb(001~254), Cr(001~254)
            ret = nvp1104_i2c_write(id, 0x84, 0x01); if(ret < 0) goto exit;   ///< Channel ID in EAV/SAV
            ret = nvp1104_i2c_write(id, 0xec, 0xc0); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf1, 0x04); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf2, 0x40); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf3, 0x04); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf6, 0x00); if(ret < 0) goto exit;   ///< using 54MHz oscillator
            ret = nvp1104_i2c_write(id, 0xfa, 0x0f); if(ret < 0) goto exit;
            break;
    }

    printk("NVP1104#%d ==> %s 720H 27MHz MODE!!\n", id, ((vfmt == NVP1104_VFMT_PAL) ? "PAL" : "NTSC"));

exit:
    return ret;
}

static int nvp1104_720h_2ch(int id, NVP1104_VFMT_T vfmt, NVP1104_OSC_CLKIN_T clkin)
{
    int ret = 0;

    if(id >= NVP1104_DEV_MAX || vfmt >= NVP1104_VFMT_MAX || clkin >= NVP1104_OSC_CLKIN_MAX)
        return -1;

    ret = nvp1104_common_init(id, vfmt);
    if(ret < 0)
        goto exit;

    switch(clkin) {
        case NVP1104_OSC_CLKIN_108MHZ:
            ret = nvp1104_i2c_write(id, 0x2e, 0x00); if(ret < 0) goto exit;    ///< 27MHz Channel 1 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x3e, 0x00); if(ret < 0) goto exit;    ///< 27MHz Channel 2 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x4e, 0x00); if(ret < 0) goto exit;    ///< 27MHz Channel 3 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x5e, 0x00); if(ret < 0) goto exit;    ///< 27MHz Channel 4 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x2f, 0x04); if(ret < 0) goto exit;    ///< 54MHz time-mixed CCIR656 data of ch 1,2
            ret = nvp1104_i2c_write(id, 0x3f, 0x15); if(ret < 0) goto exit;    ///< 54MHz time-mixed CCIR656 data of ch 3,4
            ret = nvp1104_i2c_write(id, 0x4f, 0x24); if(ret < 0) goto exit;    ///< 54MHz time-mixed CCIR656 data of ch 1,2
            ret = nvp1104_i2c_write(id, 0x5f, 0x35); if(ret < 0) goto exit;    ///< 54MHz time-mixed CCIR656 data of ch 3,4
            ret = nvp1104_i2c_write(id, 0x83, 0x81); if(ret < 0) goto exit;    ///< No Video Background Color => Black(75%), Y(001~254), Cb(001~254), Cr(001~254)
            ret = nvp1104_i2c_write(id, 0x84, 0x01); if(ret < 0) goto exit;    ///< Channel ID in EAV/SAV
            ret = nvp1104_i2c_write(id, 0xec, 0xc0); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf1, 0x00); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf2, 0x20); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf3, 0x04); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf6, 0x80); if(ret < 0) goto exit;    ///< using 108MHz oscillator
            ret = nvp1104_i2c_write(id, 0xfa, 0x00); if(ret < 0) goto exit;
            break;
        default:
            ret = nvp1104_i2c_write(id, 0x2e, 0x40); if(ret < 0) goto exit;    ///< 54MHz Channel 1 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x3e, 0x40); if(ret < 0) goto exit;    ///< 54MHz Channel 2 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x4e, 0x40); if(ret < 0) goto exit;    ///< 54MHz Channel 3 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x5e, 0x40); if(ret < 0) goto exit;    ///< 54MHz Channel 4 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x2f, 0x04); if(ret < 0) goto exit;    ///< 54MHz time-mixed CCIR656 data of ch 1,2, only this port can output 54MHz 2D1
            ret = nvp1104_i2c_write(id, 0x3f, 0x15); if(ret < 0) goto exit;    ///< 54MHz time-mixed CCIR656 data of ch 3,4, only this port can output 54MHz 2D1
            ret = nvp1104_i2c_write(id, 0x4f, 0x24); if(ret < 0) goto exit;    ///< 54MHz time-mixed CCIR656 data of ch 1,2
            ret = nvp1104_i2c_write(id, 0x5f, 0x35); if(ret < 0) goto exit;    ///< 54MHz time-mixed CCIR656 data of ch 3,4
            ret = nvp1104_i2c_write(id, 0x83, 0x81); if(ret < 0) goto exit;    ///< No Video Background Color => Black(75%), Y(001~254), Cb(001~254), Cr(001~254)
            ret = nvp1104_i2c_write(id, 0x84, 0x01); if(ret < 0) goto exit;    ///< Channel ID in EAV/SAV
            ret = nvp1104_i2c_write(id, 0xec, 0xc0); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf1, 0x04); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf2, 0x40); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf3, 0x04); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf6, 0x00); if(ret < 0) goto exit;    ///< using 54MHz oscillator
            ret = nvp1104_i2c_write(id, 0xfa, 0x0f); if(ret < 0) goto exit;
            break;
    }

    printk("NVP1104#%d ==> %s 720H 54MHz MODE!!\n", id, ((vfmt == NVP1104_VFMT_PAL) ? "PAL" : "NTSC"));

exit:
    return ret;
}

static int nvp1104_720h_4ch(int id, NVP1104_VFMT_T vfmt, NVP1104_OSC_CLKIN_T clkin)
{
    int ret = 0;

    if(id >= NVP1104_DEV_MAX || vfmt >= NVP1104_VFMT_MAX || clkin >= NVP1104_OSC_CLKIN_MAX)
        return -1;

    ret = nvp1104_common_init(id, vfmt);
    if(ret < 0)
        goto exit;

    switch(clkin) {
        case NVP1104_OSC_CLKIN_108MHZ:
            ret = nvp1104_i2c_write(id, 0x2e, 0x60); if(ret < 0) goto exit;    ///< 108MHz Channel 2 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x3e, 0x60); if(ret < 0) goto exit;    ///< 108MHz Channel 3 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x4e, 0x60); if(ret < 0) goto exit;    ///< 108MHz Channel 4 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x5e, 0x60); if(ret < 0) goto exit;    ///< 108MHz time-mixed CCIR656 data of ch 1,2,3,4
            ret = nvp1104_i2c_write(id, 0x2f, 0x06); if(ret < 0) goto exit;    ///< 108MHz time-mixed CCIR656 data of ch 1,2,3,4
            ret = nvp1104_i2c_write(id, 0x3f, 0x16); if(ret < 0) goto exit;    ///< 108MHz time-mixed CCIR656 data of ch 1,2,3,4
            ret = nvp1104_i2c_write(id, 0x4f, 0x26); if(ret < 0) goto exit;    ///< 108MHz time-mixed CCIR656 data of ch 1,2,3,4
            ret = nvp1104_i2c_write(id, 0x5f, 0x36); if(ret < 0) goto exit;    ///< No Video Background Color => Black(75%), Y(001~254), Cb(001~254), Cr(001~254)
            ret = nvp1104_i2c_write(id, 0x83, 0x81); if(ret < 0) goto exit;    ///< Channel ID in EAV/SAV
            ret = nvp1104_i2c_write(id, 0x84, 0x01); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xec, 0xc0); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf1, 0x00); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf2, 0x20); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf3, 0x04); if(ret < 0) goto exit;    ///< using 108MHz oscillator
            ret = nvp1104_i2c_write(id, 0xf6, 0x80); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xfa, 0x00); if(ret < 0) goto exit;
            break;
        default:
            ret = nvp1104_i2c_write(id, 0x2e, 0x60); if(ret < 0) goto exit;    ///< 108MHz Channel 1 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x3e, 0x60); if(ret < 0) goto exit;    ///< 108MHz Channel 2 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x4e, 0x60); if(ret < 0) goto exit;    ///< 108MHz Channel 3 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x5e, 0x60); if(ret < 0) goto exit;    ///< 108MHz Channel 4 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x2f, 0x06); if(ret < 0) goto exit;    ///< 108MHz time-mixed CCIR656 data of ch 1,2,3,4
            ret = nvp1104_i2c_write(id, 0x3f, 0x16); if(ret < 0) goto exit;    ///< 108MHz time-mixed CCIR656 data of ch 1,2,3,4, only this port can output 108MHz 4D1
            ret = nvp1104_i2c_write(id, 0x4f, 0x26); if(ret < 0) goto exit;    ///< 108MHz time-mixed CCIR656 data of ch 1,2,3,4
            ret = nvp1104_i2c_write(id, 0x5f, 0x36); if(ret < 0) goto exit;    ///< 108MHz time-mixed CCIR656 data of ch 1,2,3,4
            ret = nvp1104_i2c_write(id, 0x83, 0x81); if(ret < 0) goto exit;    ///< No Video Background Color => Black(75%), Y(001~254), Cb(001~254), Cr(001~254)
            ret = nvp1104_i2c_write(id, 0x84, 0x01); if(ret < 0) goto exit;    ///< Channel ID in EAV/SAV
            ret = nvp1104_i2c_write(id, 0xec, 0xc0); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf1, 0x04); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf2, 0x40); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf3, 0x04); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf6, 0x00); if(ret < 0) goto exit;    ///< using 54MHz oscillator
            ret = nvp1104_i2c_write(id, 0xfa, 0x0f); if(ret < 0) goto exit;
            break;
    }

    printk("NVP1104#%d ==> %s 720H 108MHz MODE!!\n", id, ((vfmt == NVP1104_VFMT_PAL) ? "PAL" : "NTSC"));

exit:
    return ret;
}

static int nvp1104_cif_4ch(int id, NVP1104_VFMT_T vfmt, NVP1104_OSC_CLKIN_T clkin)
{
    int ret = 0;

    if(id >= NVP1104_DEV_MAX || vfmt >= NVP1104_VFMT_MAX || clkin >= NVP1104_OSC_CLKIN_MAX)
        return -1;

    ret = nvp1104_common_init(id, vfmt);
    if(ret < 0)
        goto exit;

    switch(clkin) {
        case NVP1104_OSC_CLKIN_108MHZ:
            ret = nvp1104_i2c_write(id, 0x2e, 0x40); if(ret < 0) goto exit;    ///< 54MHz Channel 1 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x3e, 0x40); if(ret < 0) goto exit;    ///< 54MHz Channel 2 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x4e, 0x40); if(ret < 0) goto exit;    ///< 54MHz Channel 3 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x5e, 0x40); if(ret < 0) goto exit;    ///< 54MHz Channel 4 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x2f, 0x07); if(ret < 0) goto exit;    ///< 54MHz time-mixed CCIR656 data of ch 1,2,3,4
            ret = nvp1104_i2c_write(id, 0x3f, 0x17); if(ret < 0) goto exit;    ///< 54MHz time-mixed CCIR656 data of ch 1,2,3,4
            ret = nvp1104_i2c_write(id, 0x4f, 0x27); if(ret < 0) goto exit;    ///< 54MHz time-mixed CCIR656 data of ch 1,2,3,4
            ret = nvp1104_i2c_write(id, 0x5f, 0x37); if(ret < 0) goto exit;    ///< 54MHz time-mixed CCIR656 data of ch 1,2,3,4
            ret = nvp1104_i2c_write(id, 0x83, 0x81); if(ret < 0) goto exit;    ///< No Video Background Color => Black(75%), Y(001~254), Cb(001~254), Cr(001~254)
            ret = nvp1104_i2c_write(id, 0x84, 0x01); if(ret < 0) goto exit;    ///< Channel ID in EAV/SAV
            ret = nvp1104_i2c_write(id, 0xec, 0xc0); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf1, 0x00); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf2, 0x20); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf3, 0x04); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf6, 0x80); if(ret < 0) goto exit;    ///< using 108MHz oscillator
            ret = nvp1104_i2c_write(id, 0xfa, 0x00); if(ret < 0) goto exit;
            break;
        default:
            ret = nvp1104_i2c_write(id, 0x2e, 0x74); if(ret < 0) goto exit;    ///< 54MHz Channel 1 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x3e, 0x74); if(ret < 0) goto exit;    ///< 54MHz Channel 2 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x4e, 0x74); if(ret < 0) goto exit;    ///< 54MHz Channel 3 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x5e, 0x74); if(ret < 0) goto exit;    ///< 54MHz Channel 4 clock delay 0 ns
            ret = nvp1104_i2c_write(id, 0x2f, 0x07); if(ret < 0) goto exit;    ///< 54MHz time-mixed CIF CCIR656 data of ch 1,2,3,4
            ret = nvp1104_i2c_write(id, 0x3f, 0x17); if(ret < 0) goto exit;    ///< 54MHz time-mixed CIF CCIR656 data of ch 1,2,3,4, only this port can output 4CIF
            ret = nvp1104_i2c_write(id, 0x4f, 0x27); if(ret < 0) goto exit;    ///< 54MHz time-mixed CIF CCIR656 data of ch 1,2,3,4
            ret = nvp1104_i2c_write(id, 0x5f, 0x37); if(ret < 0) goto exit;    ///< 54MHz time-mixed CIF CCIR656 data of ch 1,2,3,4
            ret = nvp1104_i2c_write(id, 0x83, 0x81); if(ret < 0) goto exit;    ///< No Video Background Color => Black(75%), Y(001~254), Cb(001~254), Cr(001~254)
            ret = nvp1104_i2c_write(id, 0x84, 0x01); if(ret < 0) goto exit;    ///< Channel ID in EAV/SAV
            ret = nvp1104_i2c_write(id, 0xec, 0xc0); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf1, 0x04); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf2, 0x50); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf3, 0x04); if(ret < 0) goto exit;
            ret = nvp1104_i2c_write(id, 0xf6, 0x00); if(ret < 0) goto exit;    ///< using 54MHz oscillator
            ret = nvp1104_i2c_write(id, 0xfa, 0x0f); if(ret < 0) goto exit;
            break;
    }

    printk("NVP1104#%d ==> %s 4CIF 54MHz MODE!!\n", id, ((vfmt == NVP1104_VFMT_PAL) ? "PAL" : "NTSC"));

exit:
    return ret;
}

int nvp1104_video_set_mode(int id, NVP1104_VMODE_T mode, NVP1104_OSC_CLKIN_T clkin)
{
    int ret;

    if(id >= NVP1104_DEV_MAX || mode >= NVP1104_VMODE_MAX)
        return -1;

    switch(mode) {
        case NVP1104_VMODE_NTSC_720H_1CH:
            ret = nvp1104_720h_1ch(id, NVP1104_VFMT_NTSC, clkin);
            break;
        case NVP1104_VMODE_NTSC_720H_2CH:
            ret = nvp1104_720h_2ch(id, NVP1104_VFMT_NTSC, clkin);
            break;
        case NVP1104_VMODE_NTSC_720H_4CH:
            ret = nvp1104_720h_4ch(id, NVP1104_VFMT_NTSC, clkin);
            break;
        case NVP1104_VMODE_NTSC_CIF_4CH:
            ret = nvp1104_cif_4ch(id, NVP1104_VFMT_NTSC, clkin);
            break;
        case NVP1104_VMODE_PAL_720H_1CH:
            ret = nvp1104_720h_1ch(id, NVP1104_VFMT_PAL, clkin);
            break;
        case NVP1104_VMODE_PAL_720H_2CH:
            ret = nvp1104_720h_2ch(id, NVP1104_VFMT_PAL, clkin);
            break;
        case NVP1104_VMODE_PAL_720H_4CH:
            ret = nvp1104_720h_4ch(id, NVP1104_VFMT_PAL, clkin);
            break;
        case NVP1104_VMODE_PAL_CIF_4CH:
            ret = nvp1104_cif_4ch(id, NVP1104_VFMT_PAL, clkin);
            break;
        default:
            ret = -1;
            printk("NVP1104#%d driver not support video output mode(%d)!!\n", id, mode);
            break;
    }

    return ret;
}
EXPORT_SYMBOL(nvp1104_video_set_mode);

int nvp1104_video_get_mode(int id)
{
    int ret;
    int is_ntsc;

    if(id >= NVP1104_DEV_MAX)
        return -1;

    /* get ch#0 current register setting for determine device running mode */
    ret = nvp1104_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;
    is_ntsc = (((nvp1104_i2c_read(id, 0x20)>>6)&0x03) == 0x01) ? 1 : 0;

    switch(nvp1104_i2c_read(id, 0x2f)) {    ///< ch_out_sel
        case 0 ... 3:
            if(is_ntsc)
                ret = NVP1104_VMODE_NTSC_720H_1CH;
            else
                ret = NVP1104_VMODE_PAL_720H_1CH;
            break;
        case 4 ... 5:
            if(is_ntsc)
                ret = NVP1104_VMODE_NTSC_720H_2CH;
            else
                ret = NVP1104_VMODE_PAL_720H_2CH;
            break;
        case 7:
            if(is_ntsc)
                ret = NVP1104_VMODE_NTSC_CIF_4CH;
            else
                ret = NVP1104_VMODE_PAL_CIF_4CH;
            break;
        default:
            if(is_ntsc)
                ret = NVP1104_VMODE_NTSC_720H_4CH;
            else
                ret = NVP1104_VMODE_PAL_720H_4CH;
            break;
    }

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_video_get_mode);

int nvp1104_video_mode_support_check(int id, NVP1104_VMODE_T mode)
{
    if(id >= NVP1104_DEV_MAX)
        return 0;

    switch(mode) {
        case NVP1104_VMODE_NTSC_720H_1CH:
        case NVP1104_VMODE_NTSC_720H_2CH:
        case NVP1104_VMODE_NTSC_720H_4CH:
        case NVP1104_VMODE_NTSC_CIF_4CH:
        case NVP1104_VMODE_PAL_720H_1CH:
        case NVP1104_VMODE_PAL_720H_2CH:
        case NVP1104_VMODE_PAL_720H_4CH:
        case NVP1104_VMODE_PAL_CIF_4CH:
            return 1;
        default:
            return 0;
    }

    return 0;
}
EXPORT_SYMBOL(nvp1104_video_mode_support_check);

int nvp1104_video_set_contrast(int id, int ch, u8 value)
{
    int ret;

    if(id >= NVP1104_DEV_MAX || ch >= NVP1104_DEV_CH_MAX)
        return -1;

    ret = nvp1104_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1104_i2c_write(id, 0x22+(0x10*ch), value);

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_video_set_contrast);

int nvp1104_video_get_contrast(int id, int ch)
{
    int ret;

    if(id >= NVP1104_DEV_MAX || ch >= NVP1104_DEV_CH_MAX)
        return -1;

    ret = nvp1104_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1104_i2c_read(id, 0x22+(0x10*ch));

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_video_get_contrast);

int nvp1104_video_set_brightness(int id, int ch, u8 value)
{
    int ret;

    if(id >= NVP1104_DEV_MAX || ch >= NVP1104_DEV_CH_MAX)
        return -1;

    ret = nvp1104_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1104_i2c_write(id, 0x21+(0x10*ch), value);

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_video_set_brightness);

int nvp1104_video_get_brightness(int id, int ch)
{
    int ret;

    if(id >= NVP1104_DEV_MAX || ch >= NVP1104_DEV_CH_MAX)
        return -1;

    ret = nvp1104_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1104_i2c_read(id, 0x21+(0x10*ch));

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_video_get_brightness);

int nvp1104_video_set_saturation(int id, int ch, u8 value)
{
    int ret;

    if(id >= NVP1104_DEV_MAX || ch >= NVP1104_DEV_CH_MAX)
        return -1;

    ret = nvp1104_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1104_i2c_write(id, 0x24+(0x10*ch), value);

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_video_set_saturation);

int nvp1104_video_get_saturation(int id, int ch)
{
    int ret;

    if(id >= NVP1104_DEV_MAX || ch >= NVP1104_DEV_CH_MAX)
        return -1;

    ret = nvp1104_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1104_i2c_read(id, 0x24+(0x10*ch));

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_video_get_saturation);

int nvp1104_video_set_hue(int id, int ch, u8 value)
{
    int ret;

    if(id >= NVP1104_DEV_MAX || ch >= NVP1104_DEV_CH_MAX)
        return -1;

    ret = nvp1104_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1104_i2c_write(id, 0x23+(0x10*ch), value);

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_video_set_hue);

int nvp1104_video_get_hue(int id, int ch)
{
    int ret;

    if(id >= NVP1104_DEV_MAX || ch >= NVP1104_DEV_CH_MAX)
        return -1;

    ret = nvp1104_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1104_i2c_read(id, 0x23+(0x10*ch));

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_video_get_hue);

int nvp1104_video_set_sharpness(int id, u8 value)
{
    int ret;

    if(id >= NVP1104_DEV_MAX)
        return -1;

    ret = nvp1104_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1104_i2c_write(id, 0x7b, value);

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_video_set_sharpness);

int nvp1104_video_get_sharpness(int id)
{
    int ret;

    if(id >= NVP1104_DEV_MAX)
        return -1;

    ret = nvp1104_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = nvp1104_i2c_read(id, 0x7b);

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_video_get_sharpness);

int nvp1104_status_get_novid(int id)
{
    int ret;

    if(id >= NVP1104_DEV_MAX)
        return -1;

    ret = nvp1104_i2c_write(id, 0xFF, 0x00);
    if(ret < 0)
        goto exit;

    ret = (nvp1104_i2c_read(id, 0x00)>>4) & 0xf;

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_status_get_novid);

int nvp1104_audio_init(int id, unsigned char ch_num, int cascade)
{
    int ret;
    unsigned char banksave;
    unsigned char samplerate = AUDIO_SAMPLERATE_8K;
    unsigned char bitrates   = AUDIO_BITS_16B;

    if (ch_num == 8)
        bitrates = AUDIO_BITS_8B;
    else if (ch_num == 4)
        bitrates = AUDIO_BITS_16B;
    else if (ch_num == 2)
        bitrates = AUDIO_BITS_16B;
    else {
        printk("NVP1104 just support 2/4/8/16 channel number\n");
        return -1;
    }

    banksave = nvp1104_i2c_read(id, 0xff);      ///< save bank

    ret = nvp1104_i2c_write(id, 0xff, 0x10); if(ret < 0) goto exit;     ///< select bank 0
    ret = nvp1104_i2c_write(id, 0x52, 0x00); if(ret < 0) goto exit;     ///< Audio No MIXed out - Gain 0 (AOGAIN=0)
    ret = nvp1104_i2c_write(id, 0x53, 0x19); if(ret < 0) goto exit;    ///< Audio No MIXed out - No audio Output (MIX_OUTSEL=25)
    ret = nvp1104_i2c_write(id, 0x31, 0x88); if(ret < 0) goto exit;     ///< Audio input gain init - Gain 1.0
    ret = nvp1104_i2c_write(id, 0x32, 0x88); if(ret < 0) goto exit;
    ret = nvp1104_i2c_write(id, 0x33, 0x88); if(ret < 0) goto exit;
    ret = nvp1104_i2c_write(id, 0x34, 0x88); if(ret < 0) goto exit;

    if (cascade) {              // cascade mode
        if ((id == 0)||(id == 2)) {
            ret = nvp1104_i2c_write(id, 0x36, 0x1A); if(ret < 0) goto exit;  ///< RM cascade first stage
            ret = nvp1104_i2c_write(id, 0x37, 0x80 | (samplerate << 3) | (bitrates << 2)); if(ret < 0) goto exit;    ///< RM master
            ret = nvp1104_i2c_write(id, 0x43, 0x00 | (samplerate << 3) | (bitrates << 2)); if(ret < 0) goto exit;    ///< PB master
        }
        else {
            ret = nvp1104_i2c_write(id, 0x36, 0x19); if(ret < 0) goto exit;  ///< RM cascade last stage
            ret = nvp1104_i2c_write(id, 0x37, 0x00 | (samplerate << 3) | (bitrates << 2)); if(ret < 0) goto exit;    ///< RM master
            ret = nvp1104_i2c_write(id, 0x43, 0x00 | (samplerate << 3) | (bitrates << 2)); if(ret < 0) goto exit;    ///< PB slave
        }
    } else {
        ret = nvp1104_i2c_write(id, 0x36, 0x1B); if(ret < 0) goto exit;      ///< RM single operation (CHIP_STAGE=3)
        ret = nvp1104_i2c_write(id, 0x37, 0x80 | (samplerate << 3) | (bitrates << 2)); if(ret < 0) goto exit;        ///< RM master
        ret = nvp1104_i2c_write(id, 0x43, 0x00 | (samplerate << 3) | (bitrates << 2)); if(ret < 0) goto exit;        ///< PB slave
    }

    if (2 == ch_num) {
        ret = nvp1104_i2c_write(id, 0x38, 0x00); if(ret < 0) goto exit;      ///< RM channel number 2
    } else if (4 == ch_num) {
        ret = nvp1104_i2c_write(id, 0x38, 0x01); if(ret < 0) goto exit;      ///< RM channel number 4
    } else if (8 == ch_num) {
        ret = nvp1104_i2c_write(id, 0x38, 0x02); if(ret < 0) goto exit;      ///< RM channel number 8
    }

    // L: 4/2/3/1/11/9/12/10
    ret = nvp1104_i2c_write(id, 0x3A, 0x13); if(ret < 0) goto exit;
    ret = nvp1104_i2c_write(id, 0x3B, 0x02); if(ret < 0) goto exit;
    ret = nvp1104_i2c_write(id, 0x3C, 0x8A); if(ret < 0) goto exit;
    ret = nvp1104_i2c_write(id, 0x3D, 0x9B); if(ret < 0) goto exit;

    // R: 8/6/7/5/15/13/16/15
    ret = nvp1104_i2c_write(id, 0x3E, 0x57); if(ret < 0) goto exit;
    ret = nvp1104_i2c_write(id, 0x3F, 0x46); if(ret < 0) goto exit;
    ret = nvp1104_i2c_write(id, 0x40, 0xCE); if(ret < 0) goto exit;
    ret = nvp1104_i2c_write(id, 0x41, 0xDF); if(ret < 0) goto exit;

    ret = nvp1104_i2c_write(id, 0x53, 0x20 | 0x10); if(ret < 0) goto exit;   ///< PB select - (MIX_DERATIO=1, MIX_OUTSEL=16)
    ret = nvp1104_i2c_write(id, 0x52, 0x80); if(ret < 0) goto exit;          ///< Audio No MIXed out - Gain 1.0 (AOGAIN=8)
    ret = nvp1104_i2c_write(id, 0x44, 0x00); if(ret < 0) goto exit;          ///< playback select channel - channel 0 (PB_SEL=0)

    ret = nvp1104_i2c_write(id, 0xff, banksave); if(ret < 0) goto exit;      ///< restore bank setting

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_audio_init);

int nvp1104_audio_set_sample_rate(int id, NVP1104_SAMPLERATE_T rate)
{
    int ret;
    unsigned char banksave, data, umask;
    unsigned char samplerate = AUDIO_SAMPLERATE_8K;

    switch(rate) {
        case AUDIO_SAMPLERATE_8K:
            samplerate = AUDIO_SAMPLERATE_8K;
            break;
        case AUDIO_SAMPLERATE_16K:
            samplerate = AUDIO_SAMPLERATE_16K;
            break;
        default:
            printk("NVP1104 only support 8/16 KHz\n");
            return -1;
    }

    banksave = nvp1104_i2c_read(id, 0xff);      ///< save bank

    ret = nvp1104_i2c_write(id, 0xff, 0x10); if(ret < 0) goto exit;    ///< select bank 0

    data = nvp1104_i2c_read(id, 0x37);
    umask = (unsigned char)(AUDIO_SAMPLERATE_16K << 3);
    data &= ~umask;
    data |= (samplerate << 3);
    ret = nvp1104_i2c_write(id, 0x37, data); if(ret < 0) goto exit;

    data = nvp1104_i2c_read(id, 0x43);
    umask = (unsigned char)(AUDIO_SAMPLERATE_16K << 3);
    data &= ~umask;
    data |= (samplerate << 3);
    ret = nvp1104_i2c_write(id, 0x43, data); if(ret < 0) goto exit;

    ret = nvp1104_i2c_write(id, 0xff, banksave); if(ret < 0) goto exit; ///< restore bank setting

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_audio_set_sample_rate);

int nvp1104_audio_set_sample_size(int id, NVP1104_SAMPLESIZE_T size)
{
    int ret;
    unsigned char banksave, data, umask;
    unsigned char samplesize = AUDIO_BITS_8B;

    switch(size) {
        case AUDIO_BITS_16B:
            samplesize = AUDIO_BITS_16B;
            break;
        case AUDIO_BITS_8B:
            samplesize = AUDIO_BITS_8B;
            break;
        default:
            printk("NVP1104 only support 8/16 bitrates\n");
            return -1;
    }

    banksave = nvp1104_i2c_read(id, 0xff);      ///< save bank

    ret = nvp1104_i2c_write(id, 0xff, 0x10); if(ret < 0) goto exit;  ///< select bank 0

    data = nvp1104_i2c_read(id, 0x37);
    umask = (unsigned char)(AUDIO_BITS_8B << 2);
    data &= ~umask;
    data |= (samplesize << 2);
    ret = nvp1104_i2c_write(id, 0x37, data); if(ret < 0) goto exit;

    data = nvp1104_i2c_read(id, 0x43);
    umask = (unsigned char)(AUDIO_BITS_8B << 2);
    data &= ~umask;
    data |= (samplesize << 2);
    ret = nvp1104_i2c_write(id, 0x43, data); if(ret < 0) goto exit;

    ret = nvp1104_i2c_write(id, 0xff, banksave); if(ret < 0) goto exit;      ///< restore bank setting

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_audio_set_sample_size);

int nvp1104_audio_set_mute(int id, int on)
{
    int ret;
    unsigned char banksave;

    banksave = nvp1104_i2c_read(id, 0xff);      ///< save bank

    ret = nvp1104_i2c_write(id, 0xff, 0x10); if(ret < 0) goto exit;  ///< select bank 0

    ret = nvp1104_i2c_write(id, 0x52, on ? 0x00 : 0x80); if(ret < 0) goto exit;      ///< Audio No MIXed out - Gain 0|1 (AOGAIN=0|1)

    ret = nvp1104_i2c_write(id, 0xff, banksave); if(ret < 0) goto exit;             ///< restore bank setting

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_audio_set_mute);

int nvp1104_audio_set_play_ch(int id, unsigned char ch)
{
    int ret;
    unsigned char banksave;

    banksave = nvp1104_i2c_read(id, 0xff);      ///< save bank

    ret = nvp1104_i2c_write(id, 0xff, 0x10); if(ret < 0) goto exit;         ///< select bank 0

    ret = nvp1104_i2c_write(id, 0x44, (ch & 0x0F)); if(ret < 0) goto exit;

    ret = nvp1104_i2c_write(id, 0xff, banksave); if(ret < 0) goto exit;     ///< restore bank setting

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_audio_set_play_ch);

static int nvp1104_audio_get_chnum(NVP1104_VMODE_T mode)
{
    int ch_num;

    switch(mode) {
        case NVP1104_VMODE_NTSC_720H_4CH:
        case NVP1104_VMODE_NTSC_CIF_4CH:
        case NVP1104_VMODE_PAL_720H_4CH:
        case NVP1104_VMODE_PAL_CIF_4CH:
            ch_num = 8;
            break;
        case NVP1104_VMODE_NTSC_720H_2CH:
        case NVP1104_VMODE_PAL_720H_2CH:
            ch_num = 4;
            break;
        case NVP1104_VMODE_NTSC_720H_1CH:
        case NVP1104_VMODE_PAL_720H_1CH:
            ch_num = 2;
            break;
        default:    /* error */
            ch_num = -1;
            break;
    }

    return ch_num;
}

int nvp1104_audio_set_mode(int id, NVP1104_VMODE_T mode, NVP1104_SAMPLESIZE_T samplesize, NVP1104_SAMPLERATE_T samplerate)
{
    int ch_num;
    int ret;

    ch_num = nvp1104_audio_get_chnum(mode);
    if(ch_num < 0)
        return -1;

    /* nvp1104 audio initialize */
    ret = nvp1104_audio_init(id, ch_num, 1); if(ret < 0) goto exit;

    /* set audio sample rate */
    ret = nvp1104_audio_set_sample_rate(id, samplerate); if(ret < 0) goto exit;

    /* set audio sample size */
    ret = nvp1104_audio_set_sample_size(id, samplesize); if(ret < 0) goto exit;

    /* set nvp1918 AOGAIN mute */
    ret = nvp1104_audio_set_mute(id, 1); if(ret < 0) goto exit;

exit:
    return ret;
}
EXPORT_SYMBOL(nvp1104_audio_set_mode);
