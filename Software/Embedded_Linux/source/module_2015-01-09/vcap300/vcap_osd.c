/**
 * @file vcap_osd.c
 *  vcap300 osd control
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.20 $
 * $Date: 2014/11/27 01:50:06 $
 *
 * ChangeLog:
 *  $Log: vcap_osd.c,v $
 *  Revision 1.20  2014/11/27 01:50:06  jerson_l
 *  1. fix osd font index over range problem
 *
 *  Revision 1.19  2014/09/05 02:48:53  jerson_l
 *  1. support osd force frame mode for GM8136
 *  2. support osd edge smooth edge mode for GM8136
 *  3. support osd auto color change scheme for GM8136
 *
 *  Revision 1.18  2014/08/27 02:02:03  jerson_l
 *  1. support GM8136 platform VCAP300 feature
 *
 *  Revision 1.17  2014/08/15 03:02:07  jerson_l
 *  1. change alignment caculation mechanism for osd_mask window
 *
 *  Revision 1.16  2013/10/28 07:40:53  jerson_l
 *  1. fix osd set string fail problem
 *
 *  Revision 1.15  2013/08/05 08:21:25  jerson_l
 *  1. fix osd window background color incorrect problem
 *
 *  Revision 1.14  2013/07/23 05:42:05  jerson_l
 *  1. add channel range check in osd set window color api
 *
 *  Revision 1.13  2013/07/08 03:44:23  jerson_l
 *  1. move osd image border control to middleware property auto_border
 *
 *  Revision 1.12  2013/06/04 11:33:15  jerson_l
 *  1. support osd window as osd_mask window function
 *
 *  Revision 1.11  2013/05/10 05:35:32  jerson_l
 *  1. return osd align setting when get osd window config
 *
 *  Revision 1.10  2013/05/09 09:25:50  jerson_l
 *  1. reject osd font color change if fg_color the same as bg_color
 *
 *  Revision 1.9  2013/05/08 10:55:23  jerson_l
 *  1. change default osd font bitmap for new font type
 *
 *  Revision 1.8  2013/05/08 08:18:56  jerson_l
 *  1. add osd row space alignment
 *
 *  Revision 1.7  2013/04/11 03:13:44  jerson_l
 *  1. fixed set osd border cause osd window path switch problem
 *  2. fixed osd border color index limitation from 0~7 to 0~15
 *
 *  Revision 1.6  2013/03/26 02:21:01  jerson_l
 *  1. fix osd border parameter set to incorrect register
 *
 *  Revision 1.5  2013/01/02 07:34:58  jerson_l
 *  1. support osd/mark window auto align feature
 *
 *  Revision 1.4  2012/12/18 12:00:38  jerson_l
 *  1. remove parameter check for osd window steup
 *
 *  Revision 1.3  2012/12/11 10:01:27  jerson_l
 *  1. add osd control api
 *
 *  Revision 1.2  2012/10/24 06:47:51  jerson_l
 *
 *  switch file to unix format
 *
 *  Revision 1.1.1.1  2012/10/23 08:16:31  jerson_l
 *  import vcap300 driver
 *
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "bits.h"
#include "vcap_dev.h"
#include "vcap_osd.h"
#include "vcap_palette.h"
#include "vcap_dbg.h"

struct vcap_osd_font_data_t {
    int  ram_map[VCAP_OSD_CHAR_MAX];
    int  index_map[VCAP_OSD_CHAR_MAX];
    int  index[VCAP_OSD_CHAR_MAX];
    u8   fg_color;                      ///< default font foreground color
    u8   bg_color;                      ///< default font background color
    int  count;
};

static struct vcap_osd_font_data_t vcap_osd_data[VCAP_DEV_MAX];

static struct vcap_osd_char_t VCAP_OSD_Default_Char[] = {
    {
        .font=0x20,    //Char:[ ]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x21,    //Char:[!]
        .fbitmap={0x00,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x22,    //Char:["]
        .fbitmap={0x00,0x00,0x19,0x80,0x19,0x80,0x19,0x80,0x19,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x23,    //Char:[#]
        .fbitmap={0x00,0x00,0x19,0x80,0x19,0x80,0x19,0x80,0x19,0x80,0x7f,0xe0,0x7f,0xe0,0x19,0x80,0x19,0x80,0x7f,0xe0,0x7f,0xe0,0x19,0x80,0x19,0x80,0x19,0x80,0x19,0x80,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x24,    //Char:[$]
        .fbitmap={0x00,0x00,0x06,0x00,0x06,0x00,0x3f,0xe0,0x7f,0xe0,0x66,0x00,0x66,0x00,0x7f,0xc0,0x3f,0xe0,0x06,0xe0,0x06,0xe0,0x7f,0xe0,0x7f,0xc0,0x06,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x25,    //Char:[%]
        .fbitmap={0x00,0x00,0x78,0x60,0x78,0x60,0x78,0x60,0x78,0xe0,0x01,0xc0,0x03,0x80,0x07,0x00,0x0e,0x00,0x1c,0x00,0x38,0x00,0x71,0xe0,0x61,0xe0,0x61,0xe0,0x61,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x26,    //Char:[&]
        .fbitmap={0x00,0x00,0x3c,0x00,0x7e,0x00,0x6e,0x00,0x66,0x00,0x66,0x00,0x7e,0x00,0x3c,0x00,0x3c,0x00,0x7e,0x60,0x67,0xe0,0x63,0xc0,0x63,0xc0,0x7f,0xe0,0x3e,0x60,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x27,    //Char:[']
        .fbitmap={0x00,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x28,    //Char:[(]
        .fbitmap={0x00,0x00,0x03,0x80,0x07,0x80,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x07,0x80,0x03,0x80,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x29,    //Char:[)]
        .fbitmap={0x00,0x00,0x1c,0x00,0x1e,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x1e,0x00,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x2a,    //Char:[*]
        .fbitmap={0x00,0x00,0x19,0x80,0x1f,0x80,0x1f,0x80,0x0f,0x00,0x7f,0xe0,0x7f,0xe0,0x0f,0x00,0x1f,0x80,0x1f,0x80,0x19,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x2b,    //Char:[+]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x7f,0xe0,0x7f,0xe0,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x2c,    //Char:[,]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1c,0x00,0x18,0x00,0x00,0x00,}
    },
    {
        .font=0x2d,    //Char:[-]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0xe0,0x7f,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x2e,    //Char:[.]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x2f,    //Char:[/]
        .fbitmap={0x00,0x00,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0xe0,0x01,0xc0,0x03,0x80,0x07,0x00,0x0e,0x00,0x1c,0x00,0x38,0x00,0x70,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x30,    //Char:[0]
        .fbitmap={0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0xe0,0x61,0xe0,0x63,0xe0,0x67,0x60,0x6e,0x60,0x7c,0x60,0x78,0x60,0x70,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x31,    //Char:[1]
        .fbitmap={0x00,0x00,0x06,0x00,0x0e,0x00,0x1e,0x00,0x1e,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x1f,0x80,0x1f,0x80,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x32,    //Char:[2]
        .fbitmap={0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x00,0x60,0x00,0xe0,0x01,0xc0,0x03,0x80,0x07,0x00,0x0e,0x00,0x1c,0x00,0x38,0x00,0x7f,0xe0,0x7f,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x33,    //Char:[3]
        .fbitmap={0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x00,0x60,0x00,0x60,0x07,0xe0,0x07,0xe0,0x00,0x60,0x00,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x34,    //Char:[4]
        .fbitmap={0x00,0x00,0x01,0x80,0x01,0x80,0x61,0x80,0x61,0x80,0x61,0x80,0x61,0x80,0x61,0x80,0x61,0x80,0x7f,0xe0,0x7f,0xe0,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x35,    //Char:[5]
        .fbitmap={0x00,0x00,0x7f,0xe0,0x7f,0xe0,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x7f,0xc0,0x7f,0xe0,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x7f,0xe0,0x7f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x36,    //Char:[6]
        .fbitmap={0x00,0x00,0x3f,0x80,0x7f,0x80,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x7f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x37,    //Char:[7]
        .fbitmap={0x00,0x00,0xff,0xc0,0xff,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x01,0xc0,0x03,0x80,0x07,0x00,0x0e,0x00,0x1c,0x00,0x38,0x00,0x70,0x00,0xf0,0x00,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x38,    //Char:[8]
        .fbitmap={0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x39,    //Char:[9]
        .fbitmap={0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xe0,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x7f,0xe0,0x7f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x3a,    //Char:[:]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x3b,    //Char:[;]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1c,0x00,0x18,0x00,0x00,0x00,}
    },
    {
        .font=0x3c,    //Char:[<]
        .fbitmap={0x00,0x00,0x03,0x80,0x03,0x80,0x07,0x00,0x0e,0x00,0x1c,0x00,0x38,0x00,0x70,0x00,0x70,0x00,0x38,0x00,0x1c,0x00,0x0e,0x00,0x07,0x00,0x07,0x80,0x03,0x80,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x3d,    //Char:[=]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0xe0,0x7f,0xe0,0x00,0x00,0x00,0x00,0x7f,0xe0,0x7f,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x3e,    //Char:[>]
        .fbitmap={0x00,0x00,0x18,0x00,0x1c,0x00,0x0e,0x00,0x07,0x00,0x03,0x80,0x01,0xc0,0x00,0xe0,0x00,0xe0,0x01,0xc0,0x03,0x80,0x07,0x00,0x0e,0x00,0x1c,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x3f,    //Char:[?]
        .fbitmap={0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x00,0x60,0x00,0xe0,0x01,0xc0,0x03,0x80,0x07,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x40,    //Char:[@]
        .fbitmap={0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x63,0xe0,0x67,0xe0,0x66,0xe0,0x66,0xe0,0x67,0xe0,0x63,0xc0,0x60,0x00,0x60,0x00,0x7f,0xe0,0x3f,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x41,    //Char:[A]
        .fbitmap={0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x42,    //Char:[B]
        .fbitmap={0x00,0x00,0x7f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x7f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x43,    //Char:[C]
        .fbitmap={0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x44,    //Char:[D]
        .fbitmap={0x00,0x00,0x7f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x7f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x45,    //Char:[E]
        .fbitmap={0x00,0x00,0x7f,0xe0,0x7f,0xe0,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x7f,0x80,0x7f,0x80,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x7f,0xe0,0x7f,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x46,    //Char:[F]
        .fbitmap={0x00,0x00,0x7f,0xe0,0x7f,0xe0,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x7f,0x80,0x7f,0x80,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x47,    //Char:[G]
        .fbitmap={0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x00,0x60,0x00,0x61,0xe0,0x61,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x48,    //Char:[H]
        .fbitmap={0x00,0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x49,    //Char:[I]
        .fbitmap={0x00,0x00,0x1f,0x80,0x1f,0x80,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x1f,0x80,0x1f,0x80,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x4a,    //Char:[J]
        .fbitmap={0x00,0x00,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x4b,    //Char:[K]
        .fbitmap={0x00,0x00,0x60,0xe0,0x61,0xe0,0x61,0xc0,0x63,0x80,0x67,0x00,0x6e,0x00,0x7c,0x00,0x7c,0x00,0x7e,0x00,0x67,0x00,0x63,0x80,0x61,0xc0,0x61,0xe0,0x60,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x4c,    //Char:[L]
        .fbitmap={0x00,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x7f,0xe0,0x7f,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x4d,    //Char:[M]
        .fbitmap={0x00,0x00,0x60,0x60,0x70,0xe0,0x79,0xe0,0x7f,0xe0,0x6f,0x60,0x66,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x4e,    //Char:[N]
        .fbitmap={0x00,0x00,0x60,0x60,0x70,0x60,0x78,0x60,0x7c,0x60,0x6e,0x60,0x67,0x60,0x63,0xe0,0x61,0xe0,0x60,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x4f,    //Char:[O]
        .fbitmap={0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x50,    //Char:[P]
        .fbitmap={0x00,0x00,0x7f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x7f,0xc0,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x51,    //Char:[Q]
        .fbitmap={0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0xe0,0x67,0xe0,0x63,0xc0,0x63,0xc0,0x7f,0xe0,0x3e,0x60,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x52,    //Char:[R]
        .fbitmap={0x00,0x00,0x7f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x53,    //Char:[S]
        .fbitmap={0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x00,0x60,0x00,0x7f,0xc0,0x3f,0xe0,0x00,0x60,0x00,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x54,    //Char:[T]
        .fbitmap={0x00,0x00,0x7f,0xe0,0x7f,0xe0,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x55,    //Char:[U]
        .fbitmap={0x00,0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x56,    //Char:[V]
        .fbitmap={0x00,0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x70,0xe0,0x79,0xe0,0x3f,0xc0,0x1f,0x80,0x0f,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x57,    //Char:[W]
        .fbitmap={0x00,0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x60,0x66,0x60,0x66,0x60,0x66,0x60,0x66,0x60,0x6e,0xe0,0x7f,0xe0,0x3f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x58,    //Char:[X]
        .fbitmap={0x00,0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x70,0xe0,0x39,0xc0,0x1f,0x80,0x0f,0x00,0x0f,0x00,0x1f,0x80,0x39,0xc0,0x70,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x59,    //Char:[Y]
        .fbitmap={0x00,0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x70,0xe0,0x39,0xc0,0x1f,0x80,0x0f,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x5a,    //Char:[Z]
        .fbitmap={0x00,0x00,0x7f,0xe0,0x7f,0xe0,0x00,0x60,0x00,0xe0,0x01,0xc0,0x03,0x80,0x07,0x00,0x0e,0x00,0x1c,0x00,0x38,0x00,0x70,0x00,0x60,0x00,0x7f,0xe0,0x7f,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x5b,    //Char:[[]
        .fbitmap={0x00,0x00,0x07,0x80,0x07,0x80,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x07,0x80,0x07,0x80,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x5c,    //Char:[\]
        .fbitmap={0x00,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x70,0x00,0x38,0x00,0x1c,0x00,0x0e,0x00,0x07,0x00,0x03,0x80,0x01,0xc0,0x00,0xe0,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x5d,    //Char:[]]
        .fbitmap={0x00,0x00,0x1e,0x00,0x1e,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x1e,0x00,0x1e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x5e,    //Char:[^]
        .fbitmap={0x00,0x00,0x06,0x00,0x0f,0x00,0x1f,0x80,0x39,0xc0,0x70,0xe0,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x5f,    //Char:[_]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0xe0,0x7f,0xe0,0x00,0x00,}
    },
    {
        .font=0x60,    //Char:[`]
        .fbitmap={0x00,0x00,0x06,0x00,0x07,0x00,0x03,0x80,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x61,    //Char:[a]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,0xc0,0x1f,0xe0,0x00,0x60,0x00,0x60,0x3f,0xe0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x62,    //Char:[b]
        .fbitmap={0x00,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x7f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x7f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x63,    //Char:[c]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x00,0x60,0x00,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x64,    //Char:[d]
        .fbitmap={0x00,0x00,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x3f,0xe0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x65,    //Char:[e]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x7f,0xe0,0x7f,0xe0,0x60,0x00,0x60,0x00,0x7f,0x80,0x3f,0x80,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x66,    //Char:[f]
        .fbitmap={0x00,0x00,0x03,0xe0,0x07,0xe0,0x06,0x00,0x06,0x00,0x1f,0x80,0x1f,0x80,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x67,    //Char:[g]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3f,0xe0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xe0,0x00,0x60,0x00,0x60,0x1f,0xe0,0x1f,0xc0,0x00,0x00,}
    },
    {
        .font=0x68,    //Char:[h]
        .fbitmap={0x00,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x7f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x69,    //Char:[i]
        .fbitmap={0x00,0x00,0x06,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x1e,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x1f,0x80,0x1f,0x80,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x6a,    //Char:[j]
        .fbitmap={0x00,0x00,0x01,0x80,0x01,0x80,0x00,0x00,0x00,0x00,0x07,0x80,0x07,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x61,0x80,0x61,0x80,0x7f,0x80,0x3f,0x00,0x00,0x00,}
    },
    {
        .font=0x6b,    //Char:[k]
        .fbitmap={0x00,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x60,0x18,0xe0,0x19,0xc0,0x1b,0x80,0x1f,0x00,0x1f,0x00,0x1f,0x80,0x19,0xc0,0x18,0xe0,0x18,0x60,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x6c,    //Char:[l]
        .fbitmap={0x00,0x00,0x1e,0x00,0x1e,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x1f,0x80,0x1f,0x80,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x6d,    //Char:[m]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0xc0,0x7f,0xe0,0x6e,0xe0,0x66,0x60,0x66,0x60,0x66,0x60,0x66,0x60,0x66,0x60,0x66,0x60,0x66,0x60,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x6e,    //Char:[n]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x6f,    //Char:[o]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x70,    //Char:[p]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0xc0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x7f,0xc0,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x00,0x00,}
    },
    {
        .font=0x71,    //Char:[q]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3f,0xe0,0x7f,0xe0,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xe0,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x00,}
    },
    {
        .font=0x72,    //Char:[r]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,0xc0,0x1f,0xe0,0x18,0xe0,0x18,0x60,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x73,    //Char:[s]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3f,0xe0,0x7f,0xe0,0x60,0x00,0x60,0x00,0x7f,0xc0,0x3f,0xe0,0x00,0xe0,0x00,0x60,0x7f,0xe0,0x7f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x74,    //Char:[t]
        .fbitmap={0x00,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x1f,0xe0,0x1f,0xe0,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x07,0xe0,0x03,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x75,    //Char:[u]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x76,    //Char:[v]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x70,0xe0,0x79,0xe0,0x3f,0xc0,0x1f,0x80,0x0f,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x77,    //Char:[w]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x60,0x60,0x60,0x66,0x60,0x66,0x60,0x66,0x60,0x66,0x60,0x66,0x60,0x6e,0xe0,0x7f,0xe0,0x3f,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x78,    //Char:[x]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x60,0x70,0xe0,0x39,0xc0,0x1f,0x80,0x0f,0x00,0x0f,0x00,0x1f,0x80,0x39,0xc0,0x70,0xe0,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x79,    //Char:[y]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0xe0,0x3f,0xe0,0x00,0x60,0x00,0x60,0x1f,0xe0,0x1f,0xc0,0x00,0x00,}
    },
    {
        .font=0x7a,    //Char:[z]
        .fbitmap={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0xe0,0x7f,0xe0,0x01,0xc0,0x03,0x80,0x07,0x00,0x0e,0x00,0x1c,0x00,0x38,0x00,0x7f,0xe0,0x7f,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x7b,    //Char:[{]
        .fbitmap={0x00,0x00,0x03,0x80,0x07,0x80,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x1e,0x00,0x1e,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x07,0x80,0x03,0x80,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x7c,    //Char:[|]
        .fbitmap={0x00,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x7d,    //Char:[}]
        .fbitmap={0x00,0x00,0x1c,0x00,0x1e,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x07,0x80,0x07,0x80,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x1e,0x00,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
    {
        .font=0x7e,    //Char:[~]
        .fbitmap={0x00,0x00,0x3c,0x00,0x7e,0x00,0x6e,0x60,0x66,0xe0,0x07,0xe0,0x03,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}
    },
};

static inline int vcap_osd_get_index(struct vcap_dev_info_t *pdev_info, u16 font)
{
    struct vcap_osd_font_data_t *posd_d = &vcap_osd_data[pdev_info->index];

    if(posd_d->index_map[font%VCAP_OSD_CHAR_MAX]) {
        return posd_d->index[font%VCAP_OSD_CHAR_MAX];
    }
    return 0;
}

static inline int vcap_osd_set_palette_char(struct vcap_dev_info_t *pdev_info, int idx, u8 color)   ///< color: global palette index
{
    int i, row;
    u64 tmp;

    if(idx >= VCAP_OSD_PALETTE_CHAR_MAX) {
        vcap_err("vcap#%d palette_font_idx=%d over spec(MAX->%d)!\n", pdev_info->index, idx, VCAP_OSD_PALETTE_CHAR_MAX);
        return -1;
    }

    if(color >= VCAP_PALETTE_MAX) {
        vcap_err("vcap#%d palette_font_idx#%d color_idx=%d over range!\n", pdev_info->index, idx, color);
        return -1;
    }

    /* Update palette font to osd font ram */
    for(row=0; row<VCAP_OSD_FONT_ROW_SIZE; row++) {
        tmp = 0;
        for(i=0; i<VCAP_OSD_FONT_COL_SIZE; i++)
            tmp |= ((u64)color<<(i*4));

        VCAP_REG_WR64(pdev_info, VCAP_OSD_MEM_OFFSET(VCAP_OSD_CHAR_OFFSET(VCAP_OSD_PALETTE_CHAR_IDX(idx)) + row*8), tmp);
    }

    return 0;
}

int vcap_osd_set_char_4bits(struct vcap_dev_info_t *pdev_info, struct vcap_osd_char_t *osd_char, u8 bg_color, u8 fg_color)
{
    int    i, row, font_ram_index;
    int    replace = 0;
    u64    tmp;
    struct vcap_osd_font_data_t *posd_d = &vcap_osd_data[pdev_info->index];

    /* check osd font index */
    if(osd_char->font >= VCAP_OSD_CHAR_MAX) {
        vcap_err("vcap#%d font index(0x%x) over range!\n", pdev_info->index, osd_char->font);
        return -1;
    }

    /* check font exist or not ? */
    if(posd_d->index_map[osd_char->font]) {
        font_ram_index = posd_d->index[osd_char->font];
        replace = 1;
    }
    else {
        /*
         * Inserted font not exist, we need to check current font ram space
         * have free space or not?
         */
        if(posd_d->count >= VCAP_OSD_CHAR_MAX) {
            vcap_err("vcap#%d osd font ram is full!\n", pdev_info->index);
            return -1;
        }
        else {
            for(font_ram_index=0; font_ram_index<VCAP_OSD_CHAR_MAX; font_ram_index++) {
                if(!posd_d->ram_map[font_ram_index])
                    break;
            }
        }
    }

    /* Font ram space not available */
    if(font_ram_index >= VCAP_OSD_CHAR_MAX) {
        vcap_err("vcap#%d osd font database error!\n", pdev_info->index);
        return -1;
    }

    /* Update font to osd font ram */
    for(row=0; row<VCAP_OSD_FONT_ROW_SIZE; row++) {
        tmp = 0;
        for(i=0; i<8; i++) {
            if(osd_char->fbitmap[row*2] & (0x1<<i))
                tmp |= ((u64)fg_color<<((i*4)+16));
            else
                tmp |= ((u64)bg_color<<((i*4)+16));
        }

        for(i=0; i<4; i++) {
            if(osd_char->fbitmap[(row*2)+1] & (0x1<<(i+4)))
                tmp |= ((u64)fg_color<<(i*4));
            else
                tmp |= ((u64)bg_color<<(i*4));
        }
        VCAP_REG_WR64(pdev_info, VCAP_OSD_MEM_OFFSET(VCAP_OSD_CHAR_OFFSET(font_ram_index) + row*8), tmp);
    }

    /* Update osd font database */
    if(!replace) {
        posd_d->ram_map[font_ram_index]   = 1;
        posd_d->index_map[osd_char->font] = 1;
        posd_d->index[osd_char->font]     = font_ram_index;
        posd_d->count++;
    }

    return 0;
}

int vcap_osd_set_char(struct vcap_dev_info_t *pdev_info, struct vcap_osd_char_t *osd_char)
{
    int    row, font_ram_index;
    int    replace = 0;
    u64    tmp;
    struct vcap_osd_font_data_t *posd_d = &vcap_osd_data[pdev_info->index];

    /* check osd font index */
    if(osd_char->font >= VCAP_OSD_CHAR_MAX) {
        vcap_err("vcap#%d font index(0x%x) over range!\n", pdev_info->index, osd_char->font);
        return -1;
    }

    /* check font exist or not ? */
    if(posd_d->index_map[osd_char->font]) {
        font_ram_index = posd_d->index[osd_char->font];
        replace = 1;
    }
    else {
        /*
         * Inserted font not exist, we need to check current font ram space
         * have free space or not?
         */
        if(posd_d->count >= VCAP_OSD_CHAR_MAX) {
            vcap_err("vcap#%d osd font ram is full!\n", pdev_info->index);
            return -1;
        }
        else {
            for(font_ram_index=0; font_ram_index<VCAP_OSD_CHAR_MAX; font_ram_index++) {
                if(!posd_d->ram_map[font_ram_index])
                    break;
            }
        }
    }

    /* Font ram space not available */
    if(font_ram_index >= VCAP_OSD_CHAR_MAX) {
        vcap_err("vcap#%d osd font database error!\n", pdev_info->index);
        return -1;
    }

    /* Update font to osd font ram */
    for(row=0; row<VCAP_OSD_FONT_ROW_SIZE; row++) {
        tmp = ((osd_char->fbitmap[(row*2)+1]>>4) & 0xf) | ((osd_char->fbitmap[row*2] & 0xff)<<4);
        VCAP_REG_WR64(pdev_info, VCAP_OSD_MEM_OFFSET(VCAP_OSD_CHAR_OFFSET(font_ram_index) + row*8), tmp);
    }

    /* Update osd font database */
    if(!replace) {
        posd_d->ram_map[font_ram_index]   = 1;
        posd_d->index_map[osd_char->font] = 1;
        posd_d->index[osd_char->font]     = font_ram_index;
        posd_d->count++;
    }

    return 0;
}

int vcap_osd_remove_char(struct vcap_dev_info_t *pdev_info, int font)   ///< -1 for remove all
{
    int i;
    struct vcap_osd_font_data_t *posd_d = &vcap_osd_data[pdev_info->index];

    if(font >= VCAP_OSD_CHAR_MAX) {
        vcap_err("vcap#%d font index(0x%x) over range!\n", pdev_info->index, font);
        return -1;
    }

    if(font < 0) {  ///< remove all osd character in database
        for(i=0; i<VCAP_OSD_CHAR_MAX; i++) {
            posd_d->ram_map[i]   = 0;
            posd_d->index_map[i] = 0;
            posd_d->index[i]     = 0;
            posd_d->count        = 0;
        }
    }
    else {
        if(posd_d->index_map[font]) {
            posd_d->ram_map[posd_d->index[font]] = 0;
            posd_d->index_map[font]              = 0;
            posd_d->index[font]                  = 0;
            posd_d->count--;
        }
    }

    return 0;
}

int vcap_osd_set_disp_string(struct vcap_dev_info_t *pdev_info, u32 offset, u16 *font_data, u16 font_num)
{
    int i;

    if((offset + font_num) > VCAP_OSD_DISP_MAX)
        return -1;

    /* Update osd display ram */
    for(i=0; i<font_num; i++) {
        VCAP_REG_WR(pdev_info, VCAP_OSD_DISP_MEM_OFFSET((offset+i)<<3), vcap_osd_get_index(pdev_info, font_data[i]));
    }

    return 0;
}

int vcap_osd_init(struct vcap_dev_info_t *pdev_info)
{
    int i;
    int ret = 0;
    int fontcount = 0;
    struct vcap_osd_font_data_t *posd_d = &vcap_osd_data[pdev_info->index];

    memset(posd_d, 0, sizeof(struct vcap_osd_font_data_t));
    posd_d->bg_color = 0;   ///< white
    posd_d->fg_color = 1;   ///< black

    /* caculate default OSD font character count */
    fontcount = sizeof(VCAP_OSD_Default_Char)/sizeof(struct vcap_osd_char_t);

    /* check default osd character count */
    if(fontcount > VCAP_OSD_CHAR_MAX)
        fontcount = VCAP_OSD_CHAR_MAX;

    /* Load default osd character to osd font ram */
    for(i=0; i<fontcount; i++) {
        if(PLAT_OSD_PIXEL_BITS == 4)
            ret = vcap_osd_set_char_4bits(pdev_info, &VCAP_OSD_Default_Char[i], posd_d->bg_color, posd_d->fg_color);
        else
            ret = vcap_osd_set_char(pdev_info, &VCAP_OSD_Default_Char[i]);
        if(ret < 0) {
            vcap_err("vcap#%d add osd character#%d failed!!\n", pdev_info->index, i);
            goto exit;
        }
    }

    /* load osd_mask palette character, for osd_mask used, support on GM8210 */
    if(PLAT_OSD_PIXEL_BITS == 4) {
        /* load osd_mask palette character to osd font ram */
        for(i=0; i<VCAP_OSD_PALETTE_CHAR_MAX; i++) {
            ret = vcap_osd_set_palette_char(pdev_info, i, i);
            if(ret < 0) {
                vcap_err("vcap#%d add osd_mask palette character#%d failed!!\n", pdev_info->index, i);
                goto exit;
            }

            /* load osd_mask palette display index to display ram */
            VCAP_REG_WR(pdev_info, VCAP_OSD_DISP_MEM_OFFSET(VCAP_OSD_PALETTE_DISP_IDX(i)<<3), VCAP_OSD_PALETTE_CHAR_IDX(i));
        }
    }

exit:
    return ret;
}

int vcap_osd_set_priority(struct vcap_dev_info_t *pdev_info, int ch, VCAP_OSD_PRIORITY_T priority)
{
    u32 tmp;

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)));
    if(priority == VCAP_OSD_PRIORITY_OSD_ON_MARK)
        tmp |= BIT0;
    else
        tmp &= ~BIT0;
    VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)), tmp);

    return 0;
}

int vcap_osd_get_priority(struct vcap_dev_info_t *pdev_info, int ch, VCAP_OSD_PRIORITY_T *priority)
{
    u32 tmp;

    if(!priority) {
        vcap_err("[%d] ch#%d get osd priority failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)));
    *priority = tmp & BIT0;

    return 0;
}

int vcap_osd_set_font_smooth_param(struct vcap_dev_info_t *pdev_info, int ch, struct vcap_osd_font_smooth_param_t *param)
{
    u32 tmp;

    if(!param || param->level >= VCAP_OSD_FONT_SMOOTH_LEVEL_MAX) {
        vcap_err("[%d] ch#%d set osd font smooth parameter failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)));
    tmp &= ~(0x3<<1);
    if(param->enable)
        tmp |= (param->level<<2);
    else
        tmp |= (BIT1 | (param->level<<2));
    VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)), tmp);

    return 0;
}

int vcap_osd_get_font_smooth_param(struct vcap_dev_info_t *pdev_info, int ch, struct vcap_osd_font_smooth_param_t *param)
{
    u32 tmp;

    if(!param) {
        vcap_err("[%d] ch#%d get osd font smooth parameter failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)));
    param->enable = ((tmp>>1) & 0x1) ? 0 : 1;
    param->level  = (tmp>>2) & 0x1;

    return 0;
}

int vcap_osd_set_font_marquee_param(struct vcap_dev_info_t *pdev_info, int ch, struct vcap_osd_marquee_param_t *param)
{
    u32 tmp;
    unsigned long flags;

    if(!param || param->length >= VCAP_OSD_MARQUEE_LENGTH_MAX || param->speed >= 4) {
        vcap_err("[%d] ch#%d set osd font marquee parameter failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.osd_comm.marquee_speed = (u8)param->speed;  ///< lli tasklet will apply this config and sync to current

        /* update marquee_length driectly */
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)));
        tmp &= ~(0xf<<4);
        tmp |= (param->length<<4);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)), tmp);
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)));
        tmp &= ~(0x3f<<4);
        tmp |= ((param->length<<4) | (param->speed<<8));
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)), tmp);
        pdev_info->ch[ch].param.osd_comm.marquee_speed = pdev_info->ch[ch].temp_param.osd_comm.marquee_speed = (u8)param->speed;
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_osd_get_font_marquee_param(struct vcap_dev_info_t *pdev_info, int ch, struct vcap_osd_marquee_param_t *param)
{
    u32 tmp;

    if(!param) {
        vcap_err("[%d] ch#%d get osd font marquee parameter failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)));
    param->length = (tmp>>4) & 0xf;
    param->speed  = (tmp>>8) & 0x3;

    return 0;
}

int vcap_osd_set_font_marquee_mode(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_OSD_MARQUEE_MODE_T mode)
{
    int ret = 0;
    u32 tmp;
    unsigned long flags;

    if(win_idx >= VCAP_OSD_WIN_MAX || mode >= VCAP_OSD_MARQUEE_MODE_MAX) {
        vcap_err("[%d] ch#%d set osd window#%d font marquee mode failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(pdev_info->ch[ch].temp_param.osd[win_idx].win_type == VCAP_OSD_WIN_TYPE_MASK) {
        vcap_err("[%d] ch#%d set osd window#%d font marquee mode failed!(window type is osd_mask)\n", pdev_info->index, ch, win_idx);
        ret = -1;
        goto exit;
    }

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.osd[win_idx].marquee_mode = mode;  ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x0c + (0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x0c + (0x10*win_idx)))));
        tmp &= ~(0x3<<22);
        tmp |= (mode<<22);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x0c + (0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x0c + (0x10*win_idx)))), tmp);

        pdev_info->ch[ch].temp_param.osd[win_idx].marquee_mode = pdev_info->ch[ch].param.osd[win_idx].marquee_mode = mode;
    }

exit:
    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return ret;
}

int vcap_osd_get_font_marquee_mode(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_OSD_MARQUEE_MODE_T *mode)
{
    u32 tmp;

    if(win_idx >= VCAP_OSD_WIN_MAX || !mode) {
        vcap_err("[%d] ch#%d get osd font marquee mode failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x0c + (0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x0c + (0x10*win_idx)))));
    *mode = (tmp>>22) & 0x3;

    return 0;
}

int vcap_osd_set_font_edge_mode(struct vcap_dev_info_t *pdev_info, int ch, VCAP_OSD_FONT_EDGE_MODE_T mode)
{
#ifdef PLAT_OSD_EDGE_SMOOTH
    u32 tmp;
    unsigned long flags;

    if(mode >= VCAP_OSD_FONT_EDGE_MODE_MAX) {
        vcap_err("[%d] ch#%d set osd font edge mode failed!(mode=%d not support)\n", pdev_info->index, ch, mode);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.osd_comm.font_edge_mode = mode;      ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)));
        tmp &= ~BIT16;
        if(mode)
            tmp |= BIT16;
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)), tmp);
        pdev_info->ch[ch].param.osd_comm.font_edge_mode = pdev_info->ch[ch].temp_param.osd_comm.font_edge_mode = mode;
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
#else
    vcap_err("[%d] ch#%d not support osd edge mode!\n", pdev_info->index, ch);
    return -1;
#endif
}

int vcap_osd_get_font_edge_mode(struct vcap_dev_info_t *pdev_info, int ch, VCAP_OSD_FONT_EDGE_MODE_T *mode)
{
    u32 tmp;

    if(!mode) {
        vcap_err("[%d] ch#%d get osd edge mode failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

#ifdef PLAT_OSD_EDGE_SMOOTH
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)));
#else
    tmp = 0;
#endif

    *mode = (tmp>>16) & 0x1;

    return 0;
}

int vcap_osd_set_img_border_color(struct vcap_dev_info_t *pdev_info, int ch, int color)
{
    u32 tmp;
    unsigned long flags;

    if(color > 0xf) {
        vcap_err("[%d] ch#%d set osd image border color failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.osd_comm.border_color = (u8)color;      ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)));
        tmp &= ~(0xf<<12);
        tmp |= (color<<12);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)), tmp);
        pdev_info->ch[ch].param.osd_comm.border_color = pdev_info->ch[ch].temp_param.osd_comm.border_color = (u8)color;
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_osd_get_img_border_color(struct vcap_dev_info_t *pdev_info, int ch, int *color)
{
    u32 tmp;

    if(!color) {
        vcap_err("[%d] ch#%d get osd image border color failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00)));
    *color = (tmp>>12) & 0xf;

    return 0;
}

int vcap_osd_set_img_border_width(struct vcap_dev_info_t *pdev_info, int ch, int path, int width)
{
    u32 tmp;
    unsigned long flags;

    if(width > VCAP_OSD_BORDER_WIDTH_MAX) {
        vcap_err("[%d] ch#%d set osd image border width failed!(width=%d Max:%d)\n", pdev_info->index, ch, width, VCAP_OSD_BORDER_WIDTH_MAX);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x00) : VCAP_CH_SUBCH_OFFSET(ch, 0x00)));

    if(tmp & (0x2<<(path*0x8))) {  ///< check path active or not?
        pdev_info->ch[ch].temp_param.tc[path].border_w = width;      ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_TC_OFFSET(0x4 + (path*0x8)) : VCAP_CH_TC_OFFSET(ch, (0x4 + (path*0x8)))));
        tmp &= ~((0x7<<13) | BIT29);
        tmp |= ((VCAP_OSD_BORDER_PIXEL == 2) ? (((width&0x7)<<13) | (((width>>3)&0x1)<<29)) : ((width&0x7)<<13));
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_TC_OFFSET(0x4 + (path*0x8)) : VCAP_CH_TC_OFFSET(ch, (0x4 + (path*0x8)))), tmp);
        pdev_info->ch[ch].param.tc[path].border_w = pdev_info->ch[ch].temp_param.tc[path].border_w = width;
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_osd_get_img_border_width(struct vcap_dev_info_t *pdev_info, int ch, int path, int *width)
{
    u32 tmp;

    if(!width) {
        vcap_err("[%d] ch#%d get osd image border width failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_TC_OFFSET(0x4 + (path*0x8)) : VCAP_CH_TC_OFFSET(ch, (0x4 + (path*0x8)))));
    *width = (VCAP_OSD_BORDER_PIXEL == 2) ? (((tmp>>13) & 0x7) | (((tmp>>29)&0x1)<<3)) : ((tmp>>13) & 0x7);

    return 0;
}

int vcap_osd_img_border_enable(struct vcap_dev_info_t *pdev_info, int ch, int path)
{
    u32 tmp;
    unsigned long flags;

    if(path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] ch#%d-p%d enable osd image border failed!(invalid parameter)\n", pdev_info->index, ch, path);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.comm.border_en |= (0x1<<path);      ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)));
        tmp |= (0x1<<(12+path));
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)), tmp);
        pdev_info->ch[ch].temp_param.comm.border_en |= (0x1<<path);
        pdev_info->ch[ch].param.comm.border_en      |= (0x1<<path);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_osd_img_border_disable(struct vcap_dev_info_t *pdev_info, int ch, int path)
{
    u32 tmp;
    unsigned long flags;

    if(path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] ch#%d-p%d disable osd image border failed!(invalid parameter)\n", pdev_info->index, ch, path);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.comm.border_en &= ~(0x1<<path);      ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)));
        tmp &= ~(0x1<<(12+path));
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)), tmp);
        pdev_info->ch[ch].temp_param.comm.border_en &= ~(0x1<<path);
        pdev_info->ch[ch].param.comm.border_en &= ~(0x1<<path);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_osd_frame_mode_enable(struct vcap_dev_info_t *pdev_info, int ch, int path)
{
#ifdef PLAT_OSD_FRAME_MODE
    u32 tmp;
    unsigned long flags;

    if(path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] ch#%d-p%d enable osd frame mode failed!(invalid parameter)\n", pdev_info->index, ch, path);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.comm.osd_frm_mode |= (0x1<<path);      ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)));
        tmp |= (0x1<<(16+path));
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)), tmp);
        pdev_info->ch[ch].temp_param.comm.osd_frm_mode |= (0x1<<path);
        pdev_info->ch[ch].param.comm.osd_frm_mode      |= (0x1<<path);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
#else
    vcap_err("[%d] ch#%d-p%d not support osd frame mode!\n", pdev_info->index, ch, path);
    return -1;
#endif
}

int vcap_osd_frame_mode_disable(struct vcap_dev_info_t *pdev_info, int ch, int path)
{
#ifdef PLAT_OSD_FRAME_MODE
    u32 tmp;
    unsigned long flags;

    if(path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] ch#%d-p%d disable osd frame mode failed!(invalid parameter)\n", pdev_info->index, ch, path);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.comm.osd_frm_mode &= ~(0x1<<path);      ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)));
        tmp &= ~(0x1<<(16+path));
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)), tmp);
        pdev_info->ch[ch].temp_param.comm.osd_frm_mode &= ~(0x1<<path);
        pdev_info->ch[ch].param.comm.osd_frm_mode &= ~(0x1<<path);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
#else
    vcap_err("[%d] ch#%d-p%d not support osd frame mode!\n", pdev_info->index, ch, path);
    return -1;
#endif
}

int vcap_osd_set_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_win_t *win)
{
    u32 tmp;
    unsigned long flags;
    u16 x_start, y_start, x_end, y_end, width, height;
    struct vcap_osd_font_data_t *posd_d = &vcap_osd_data[pdev_info->index];

    if(!win || win_idx >= VCAP_OSD_WIN_MAX || win->path >= VCAP_SCALER_MAX || win->align >= VCAP_ALIGN_MAX) {
        vcap_err("[%d] ch#%d setup osd window#%d failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    x_start = ALIGN_4(win->x_start);
    y_start = win->y_start;
    width   = ALIGN_4(win->width);
    height  = win->height;
    x_end = ((x_start + width)  > 0) ? (x_start + width  - 1) : 0;
    y_end = ((y_start + height) > 0) ? (y_start + height - 1) : 0;

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        spin_lock_irqsave(&pdev_info->lock, flags);

        /* window type parameter */
        pdev_info->ch[ch].temp_param.osd[win_idx].win_type = VCAP_OSD_WIN_TYPE_FONT;

        /* window auto align parameter */
        pdev_info->ch[ch].temp_param.osd[win_idx].align_type     = win->align;
        pdev_info->ch[ch].temp_param.osd[win_idx].align_x_offset = x_start;
        pdev_info->ch[ch].temp_param.osd[win_idx].align_y_offset = y_start;
        pdev_info->ch[ch].temp_param.osd[win_idx].width          = width;
        pdev_info->ch[ch].temp_param.osd[win_idx].height         = height;

        /*
         * x_start, y_start, x_end, y_end, will be re-caculate in lli tasklet if auto align enable.
         * default set osd window to none align position.
         */
        pdev_info->ch[ch].temp_param.osd[win_idx].x_start  = x_start & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].y_start  = y_start & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].x_end    = x_end   & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].y_end    = y_end   & 0xfff;

        pdev_info->ch[ch].temp_param.osd[win_idx].col_sp   = win->col_sp   & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].h_wd_num = win->h_wd_num & 0x3f;
        pdev_info->ch[ch].temp_param.osd[win_idx].v_wd_num = win->v_wd_num & 0x3f;
        pdev_info->ch[ch].temp_param.osd[win_idx].path     = win->path;

        if(PLAT_OSD_PIXEL_BITS == 4) {  ///< GM8210
            pdev_info->ch[ch].temp_param.osd[win_idx].wd_addr  = win->wd_addr & 0xffff;
            pdev_info->ch[ch].temp_param.osd[win_idx].row_sp   = ALIGN_4(win->row_sp) & 0xfff;
            pdev_info->ch[ch].temp_param.osd[win_idx].bg_color = posd_d->bg_color & 0xf;
        }
        else {
            pdev_info->ch[ch].temp_param.osd[win_idx].wd_addr = win->wd_addr & 0x3fff;
            pdev_info->ch[ch].temp_param.osd[win_idx].row_sp  = (win->row_sp>>2) & 0x3ff;
        }

        spin_unlock_irqrestore(&pdev_info->lock, flags);
    }
    else {
        /* word start address, row space */
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x08+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x08+(0x10*win_idx)))));
        if(PLAT_OSD_PIXEL_BITS == 4) {  ///< GM8210
            tmp &= ~0xfffffff;
            tmp |= (win->wd_addr & 0xffff) | ((ALIGN_4(win->row_sp) & 0xfff)<<16);
        }
        else {
            tmp &= ~0xffc3fff;
            tmp |= (win->wd_addr & 0x3fff) | (((win->row_sp>>2) & 0x3ff)<<18);
        }
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x08+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x08+(0x10*win_idx)))), tmp);

        /* column space, horizontal word number, vertical word number, task select */
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x0c+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x0c+(0x10*win_idx)))));
        if(PLAT_OSD_PIXEL_BITS == 4) {  ///< GM8210
            tmp &= ~0xff3fffff;
            tmp |= (win->col_sp & 0xfff)          |
                   ((posd_d->bg_color & 0xf)<<12) |
                   ((win->h_wd_num & 0x3f)<<16)   |
                   ((win->v_wd_num & 0x3f)<<24)   |
                   ((win->path & 0x3)<<30);
        }
        else {
            tmp &= ~0xff3f0fff;
            tmp |= (win->col_sp & 0xfff) | ((win->h_wd_num & 0x3f)<<16) | ((win->v_wd_num & 0x3f)<<24) | ((win->path & 0x3)<<30);
        }
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x0c+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x0c+(0x10*win_idx)))), tmp);

        /* x position and size */
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x10+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x10+(0x10*win_idx)))));
        tmp &= ~0x0fff0fff;
        tmp |= (x_start & 0xfff) | ((x_end & 0xfff)<<16);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x10+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x10+(0x10*win_idx)))), tmp);

        /* y position and size */
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))));
        tmp &= ~0x0fff0fff;
        tmp |= (y_start & 0xfff) | ((y_end & 0xfff)<<16);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))), tmp);

        spin_lock_irqsave(&pdev_info->lock, flags);

        /* window type parameter */
        pdev_info->ch[ch].temp_param.osd[win_idx].win_type = pdev_info->ch[ch].param.osd[win_idx].win_type = VCAP_OSD_WIN_TYPE_FONT;

        /* window auto align parameter */
        pdev_info->ch[ch].temp_param.osd[win_idx].align_type     = pdev_info->ch[ch].param.osd[win_idx].align_type     = win->align;
        pdev_info->ch[ch].temp_param.osd[win_idx].align_x_offset = pdev_info->ch[ch].param.osd[win_idx].align_x_offset = x_start;
        pdev_info->ch[ch].temp_param.osd[win_idx].align_y_offset = pdev_info->ch[ch].param.osd[win_idx].align_y_offset = y_start;
        pdev_info->ch[ch].temp_param.osd[win_idx].width          = pdev_info->ch[ch].param.osd[win_idx].width          = width;
        pdev_info->ch[ch].temp_param.osd[win_idx].height         = pdev_info->ch[ch].param.osd[win_idx].height         = height;

        /*
         * x_start, y_start, x_end, y_end, will be re-caculate in lli tasklet if auto align enable.
         * default set osd window to none align position.
         */
        pdev_info->ch[ch].temp_param.osd[win_idx].x_start  = pdev_info->ch[ch].param.osd[win_idx].x_start  = x_start & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].y_start  = pdev_info->ch[ch].param.osd[win_idx].y_start  = y_start & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].x_end    = pdev_info->ch[ch].param.osd[win_idx].x_end    = x_end   & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].y_end    = pdev_info->ch[ch].param.osd[win_idx].y_end    = y_end   & 0xfff;

        pdev_info->ch[ch].temp_param.osd[win_idx].col_sp   = pdev_info->ch[ch].param.osd[win_idx].col_sp   = win->col_sp   & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].h_wd_num = pdev_info->ch[ch].param.osd[win_idx].h_wd_num = win->h_wd_num & 0x3f;
        pdev_info->ch[ch].temp_param.osd[win_idx].v_wd_num = pdev_info->ch[ch].param.osd[win_idx].v_wd_num = win->v_wd_num & 0x3f;
        pdev_info->ch[ch].temp_param.osd[win_idx].path     = pdev_info->ch[ch].param.osd[win_idx].path     = win->path;

        if(PLAT_OSD_PIXEL_BITS == 4) {  ///< GM8210
            pdev_info->ch[ch].temp_param.osd[win_idx].wd_addr  = pdev_info->ch[ch].param.osd[win_idx].wd_addr  = win->wd_addr & 0xffff;
            pdev_info->ch[ch].temp_param.osd[win_idx].row_sp   = pdev_info->ch[ch].param.osd[win_idx].row_sp   = ALIGN_4(win->row_sp)  & 0xfff;
            pdev_info->ch[ch].temp_param.osd[win_idx].bg_color = pdev_info->ch[ch].param.osd[win_idx].bg_color = posd_d->bg_color & 0xf;
        }
        else {
            pdev_info->ch[ch].temp_param.osd[win_idx].wd_addr = pdev_info->ch[ch].param.osd[win_idx].wd_addr = win->wd_addr & 0x3fff;
            pdev_info->ch[ch].temp_param.osd[win_idx].row_sp  = pdev_info->ch[ch].param.osd[win_idx].row_sp  = (win->row_sp>>2) & 0x3ff;
        }

        spin_unlock_irqrestore(&pdev_info->lock, flags);
    }

    return 0;
}

int vcap_osd_get_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_win_t *win)
{
    u32 tmp;

    if(!win || win_idx >= VCAP_OSD_WIN_MAX) {
        vcap_err("[%d] ch#%d get osd window#%d failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    /* word start address, row space */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x08+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x08+(0x10*win_idx)))));
    if(PLAT_OSD_PIXEL_BITS == 4) {  ///< GM8210
        win->wd_addr = tmp & 0xffff;
        win->row_sp  = (tmp>>16) & 0xfff;
    }
    else {
        win->wd_addr = tmp & 0x3fff;
        win->row_sp  = ((tmp>>18) & 0x3ff)<<2;
    }

    /* column space, horizontal word number, vertical word number, task select */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x0c+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x0c+(0x10*win_idx)))));
    win->col_sp   = tmp & 0xfff;
    win->h_wd_num = (tmp>>16) & 0x3f;
    win->v_wd_num = (tmp>>24) & 0x3f;
    win->path     = (tmp>>30) & 0x3;

    /* x position and size */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x10+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x10+(0x10*win_idx)))));
    win->x_start = tmp & 0xfff;
    if((tmp>>16) & 0xfff)
        win->width = (((tmp>>16) & 0xfff) + 1) - win->x_start;
    else
        win->width = 0;

    /* y position and size */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))));
    win->y_start = (tmp & 0xfff);

    if((tmp>>16) & 0xfff)
        win->height = (((tmp>>16) & 0xfff) + 1) - win->y_start;
    else
        win->height = 0;

    /* align type */
    win->align = pdev_info->ch[ch].param.osd[win_idx].align_type;

    return 0;
}

int vcap_osd_win_enable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx)
{
    u32 tmp;
    unsigned long flags;
    struct vcap_osd_font_data_t *posd_d = &vcap_osd_data[pdev_info->index];

    if(win_idx >= VCAP_OSD_WIN_MAX) {
        vcap_err("[%d] ch#%d enable osd window#%d failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        /*
         * set global font background as osd window background color in GM8210
         */
        if(PLAT_OSD_PIXEL_BITS == 4) {
            if(pdev_info->ch[ch].temp_param.osd[win_idx].win_type == VCAP_OSD_WIN_TYPE_FONT) {
                pdev_info->ch[ch].temp_param.osd[win_idx].bg_color = posd_d->bg_color;
            }
        }

        pdev_info->ch[ch].temp_param.comm.osd_en |= (0x1<<win_idx);      ///< lli tasklet will apply this config and sync to current
    }
    else {
        /*
         * set global font background color as osd window background color in GM8210
         */
        if(PLAT_OSD_PIXEL_BITS == 4) {
            if(pdev_info->ch[ch].temp_param.osd[win_idx].win_type == VCAP_OSD_WIN_TYPE_FONT) {
                tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x0c + (0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x0c + (0x10*win_idx)))));
                tmp &= ~(0xf<<12);
                tmp |= (posd_d->bg_color<<12);
                VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x0c + (0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x0c + (0x10*win_idx)))), tmp);
                pdev_info->ch[ch].param.osd[win_idx].bg_color = pdev_info->ch[ch].temp_param.osd[win_idx].bg_color = posd_d->bg_color;
            }
        }

        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)));
        tmp |= (0x1<<win_idx);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)), tmp);
        pdev_info->ch[ch].temp_param.comm.osd_en |= (0x1<<win_idx);
        pdev_info->ch[ch].param.comm.osd_en      |= (0x1<<win_idx);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_osd_win_disable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx)
{
    u32 tmp;
    unsigned long flags;

    if(win_idx >= VCAP_OSD_WIN_MAX) {
        vcap_err("[%d] ch#%d disable osd window#%d failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.comm.osd_en &= ~(0x1<<win_idx);      ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)));
        tmp &= ~(0x1<<win_idx);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)), tmp);
        pdev_info->ch[ch].temp_param.comm.osd_en &= ~(0x1<<win_idx);
        pdev_info->ch[ch].param.comm.osd_en      &= ~(0x1<<win_idx);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_osd_set_font_zoom(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_OSD_FONT_ZOOM_T zoom)
{
    int ret = 0;
    u32 tmp;
    unsigned long flags;

    if(win_idx >= VCAP_OSD_WIN_MAX || zoom >= VCAP_OSD_FONT_ZOOM_MAX) {
        vcap_err("[%d] ch#%d set osd window#%d font zoom failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(pdev_info->ch[ch].temp_param.osd[win_idx].win_type == VCAP_OSD_WIN_TYPE_MASK) {
        vcap_err("[%d] ch#%d set osd window#%d font zoom failed!(window type is osd_mask)\n", pdev_info->index, ch, win_idx);
        ret = -1;
        goto exit;
    }

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.osd[win_idx].zoom = zoom;  ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x08+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x08+(0x10*win_idx)))));
        tmp &= ~(0xf<<28);
        tmp |= (zoom<<28);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x08+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x08+(0x10*win_idx)))), tmp);
        pdev_info->ch[ch].temp_param.osd[win_idx].zoom = pdev_info->ch[ch].param.osd[win_idx].zoom = zoom;
    }

exit:
    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return ret;
}

int vcap_osd_get_font_zoom(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_OSD_FONT_ZOOM_T *zoom)
{
    u32 tmp;

    if(!zoom || win_idx >= VCAP_OSD_WIN_MAX) {
        vcap_err("[%d] ch#%d get osd window#%d font zoom failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x08+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x08+(0x10*win_idx)))));
    *zoom = (tmp>>28) & 0xf;

    return 0;
}

int vcap_osd_set_alpha(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_alpha_t *alpha)
{
    int ret = 0;
    u32 tmp;
    unsigned long flags;

    if((!alpha) ||
        (win_idx >= VCAP_OSD_WIN_MAX) ||
        (alpha->font >= VCAP_OSD_ALPHA_MAX) ||
        (alpha->background >= VCAP_OSD_ALPHA_MAX)) {
        vcap_err("[%d] ch#%d set osd window#%d alpha failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(pdev_info->ch[ch].temp_param.osd[win_idx].win_type == VCAP_OSD_WIN_TYPE_MASK) {
        vcap_err("[%d] ch#%d set osd window#%d alpha failed!(window type is osd_mask)\n", pdev_info->index, ch, win_idx);
        ret = -1;
        goto exit;
    }

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.osd[win_idx].font_alpha = alpha->font;  ///< lli tasklet will apply this config and sync to current
        pdev_info->ch[ch].temp_param.osd[win_idx].bg_alpha   = alpha->background;
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))));
        tmp &= ~0x70007000;
        tmp |= ((alpha->font<<12) | (alpha->background<<28));
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))), tmp);

        pdev_info->ch[ch].temp_param.osd[win_idx].font_alpha = pdev_info->ch[ch].param.osd[win_idx].font_alpha = alpha->font;
        pdev_info->ch[ch].temp_param.osd[win_idx].bg_alpha   = pdev_info->ch[ch].param.osd[win_idx].bg_alpha   = alpha->background;
    }

exit:
    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return ret;
}

int vcap_osd_get_alpha(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_alpha_t *alpha)
{
    u32 tmp;

    if(!alpha || win_idx >= VCAP_OSD_WIN_MAX) {
        vcap_err("[%d] ch#%d get osd window#%d alpha failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))));
    alpha->font       = (tmp>>12) & 0x7;
    alpha->background = (tmp>>28) & 0x7;

    return 0;
}

int vcap_osd_set_border_param(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_border_param_t *param)
{
    int ret = 0;
    u32 tmp;
    unsigned long flags;

    if(!param || win_idx >= VCAP_OSD_WIN_MAX || param->width > VCAP_OSD_BORDER_WIDTH_MAX || param->color > 0xf || param->type >= VCAP_OSD_BORDER_TYPE_MAX) {
        vcap_err("[%d] ch#%d set osd window#%d border failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(pdev_info->ch[ch].temp_param.osd[win_idx].win_type == VCAP_OSD_WIN_TYPE_MASK) {
        vcap_err("[%d] ch#%d set osd window#%d border failed!(window type is osd_mask)\n", pdev_info->index, ch, win_idx);
        ret = -1;
        goto exit;
    }

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.osd[win_idx].border_width = param->width;  ///< lli tasklet will apply this config and sync to current
        pdev_info->ch[ch].temp_param.osd[win_idx].border_type  = param->type;
        pdev_info->ch[ch].temp_param.osd[win_idx].border_color = param->color;
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x10+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x10+(0x10*win_idx)))));
        tmp &= ~0xf000f000;
        tmp |= ((param->width&0x7)<<12) | (param->type<<15) | (param->color<<28);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x10+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x10+(0x10*win_idx)))), tmp);

        if(VCAP_OSD_BORDER_PIXEL == 2) {
            tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))));
            tmp &= ~BIT15;
            tmp |= (((param->width>>3)&0x1)<<15);
            VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))), tmp);
        }

        pdev_info->ch[ch].temp_param.osd[win_idx].border_width = pdev_info->ch[ch].param.osd[win_idx].border_width = param->width;
        pdev_info->ch[ch].temp_param.osd[win_idx].border_type  = pdev_info->ch[ch].param.osd[win_idx].border_type  = param->type;
        pdev_info->ch[ch].temp_param.osd[win_idx].border_color = pdev_info->ch[ch].param.osd[win_idx].border_color = param->color;
    }

exit:
    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return ret;
}

int vcap_osd_get_border_param(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_border_param_t *param)
{
    u32 tmp;

    if(!param || win_idx >= VCAP_OSD_WIN_MAX) {
        vcap_err("[%d] ch#%d get osd window#%d border failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x10+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x10+(0x10*win_idx)))));
    param->type  = (tmp>>15) & 0x1;
    param->color = (tmp>>28) & 0xf;
    param->width = (tmp>>12) & 0x7;
    if(VCAP_OSD_BORDER_PIXEL == 2) {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))));
        param->width |= (((tmp>>15)&0x1)<<3);
    }

    return 0;
}

int vcap_osd_border_enable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx)
{
    int ret = 0;
    u32 tmp;
    unsigned long flags;

    if(win_idx >= VCAP_OSD_WIN_MAX) {
        vcap_err("[%d] ch#%d enable osd window#%d border failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(pdev_info->ch[ch].temp_param.osd[win_idx].win_type == VCAP_OSD_WIN_TYPE_MASK) {
        vcap_err("[%d] ch#%d enable osd window#%d border failed!(window type is osd_mask)\n", pdev_info->index, ch, win_idx);
        ret = -1;
        goto exit;
    }

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.osd[win_idx].border_en = 1;  ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))));
        tmp |= BIT31;
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))), tmp);
        pdev_info->ch[ch].temp_param.osd[win_idx].border_en = pdev_info->ch[ch].param.osd[win_idx].border_en = 1;
    }

exit:
    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return ret;
}

int vcap_osd_border_disable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx)
{
    u32 tmp;
    unsigned long flags;

    if(win_idx >= VCAP_OSD_WIN_MAX) {
        vcap_err("[%d] ch#%d disable osd window#%d border failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.osd[win_idx].border_en = 0;  ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))));
        tmp &= ~BIT31;
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))), tmp);
        pdev_info->ch[ch].temp_param.osd[win_idx].border_en = pdev_info->ch[ch].param.osd[win_idx].border_en = 0;
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_osd_set_font_color(struct vcap_dev_info_t *pdev_info, struct vcap_osd_color_t *color)
{
    if(PLAT_OSD_PIXEL_BITS == 4) {
        int i, row, col;
        u64 tmp1, tmp2;
        struct vcap_osd_font_data_t *posd_d = &vcap_osd_data[pdev_info->index];

        if(!color || color->fg_color >= VCAP_PALETTE_MAX || color->bg_color >= VCAP_PALETTE_MAX) {
            vcap_err("[%d] set osd font color failed!(invalid parameter)\n", pdev_info->index);
            return -1;
        }

        /* check foreground and background color */
        if(color->fg_color == color->bg_color) {
            vcap_err("[%d] set osd font color failed!(foreground and background color must be different)\n", pdev_info->index);
            return -1;
        }

        if((posd_d->fg_color != color->fg_color) || (posd_d->bg_color != color->bg_color)) {
            /* switch all font color in osd font ram */
            for(i=0; i<VCAP_OSD_CHAR_MAX; i++)  {
                if(posd_d->ram_map[i]) {
                    for(row=0; row<VCAP_OSD_FONT_ROW_SIZE; row++) {
                        tmp1 = VCAP_REG_RD64(pdev_info, VCAP_OSD_MEM_OFFSET(VCAP_OSD_CHAR_OFFSET(i) + row*8));
                        tmp2 = 0;
                        for(col=0; col<VCAP_OSD_FONT_COL_SIZE; col++) {
                            if((((u64)tmp1>>(col*4)) & 0xf) == posd_d->fg_color)
                                tmp2 |= ((u64)color->fg_color<<(col*4));
                            else
                                tmp2 |= ((u64)color->bg_color<<(col*4));
                        }
                        VCAP_REG_WR64(pdev_info, VCAP_OSD_MEM_OFFSET(VCAP_OSD_CHAR_OFFSET(i) + row*8), tmp2);
                    }
                }
            }

            /* update osd font ram foreground and background color */
            posd_d->fg_color = color->fg_color;
            posd_d->bg_color = color->bg_color;
        }

        return 0;
    }
    else {
        vcap_err("[%d] set osd font color failed!(not support)\n", pdev_info->index);
        return -1;
    }
}

int vcap_osd_get_font_color(struct vcap_dev_info_t *pdev_info, struct vcap_osd_color_t *color)
{
    if(PLAT_OSD_PIXEL_BITS == 4) {  ///< GM8210
        if(!color) {
            vcap_err("[%d] get osd font color failed!(invalid parameter)\n", pdev_info->index);
            return -1;
        }

        color->fg_color = vcap_osd_data[pdev_info->index].fg_color;
        color->bg_color = vcap_osd_data[pdev_info->index].bg_color;

        return 0;
    }
    else {
        vcap_err("[%d] get osd font color failed!(not support)\n", pdev_info->index);
        return -1;
    }
}

int vcap_osd_set_win_color(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_color_t *color)
{
    int ret = 0;
    u32 tmp;
    unsigned long flags;

    if(!color || (ch >= VCAP_CHANNEL_MAX) || (win_idx >= VCAP_OSD_WIN_MAX) || (color->fg_color >= VCAP_PALETTE_MAX) || (color->bg_color >= VCAP_PALETTE_MAX)) {
        vcap_err("[%d] ch#%d set osd window#%d color failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(pdev_info->ch[ch].temp_param.osd[win_idx].win_type == VCAP_OSD_WIN_TYPE_MASK) {
        vcap_err("[%d] ch#%d set osd window#%d color failed!(window type is osd_mask)\n", pdev_info->index, ch, win_idx);
        ret = -1;
        goto exit;
    }

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.osd[win_idx].bg_color = color->bg_color;  ///< lli tasklet will apply this config and sync to current

        if(PLAT_OSD_PIXEL_BITS != 4)
            pdev_info->ch[ch].temp_param.osd[win_idx].fg_color = color->fg_color;
    }
    else {
        /* background color */
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x0c + (0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x0c + (0x10*win_idx)))));
        tmp &= ~(0xf<<12);
        tmp |= (color->bg_color<<12);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x0c + (0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x0c + (0x10*win_idx)))), tmp);
        pdev_info->ch[ch].temp_param.osd[win_idx].bg_color = pdev_info->ch[ch].param.osd[win_idx].bg_color = color->bg_color;

        /* foreground color */
        if(PLAT_OSD_PIXEL_BITS != 4) {
            tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x08 + (0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x08 + (0x10*win_idx)))));
            tmp &= ~(0xf<<14);
            tmp |= (color->fg_color<<14);
            VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x08 + (0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x08 + (0x10*win_idx)))), tmp);
            pdev_info->ch[ch].temp_param.osd[win_idx].fg_color = pdev_info->ch[ch].param.osd[win_idx].fg_color = color->fg_color;
        }
    }

exit:
    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return ret;
}

int vcap_osd_get_win_color(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_color_t *color)
{
    u32 tmp;

    if(!color || win_idx >= VCAP_OSD_WIN_MAX) {
        vcap_err("[%d] ch#%d set osd window#%d color failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    /* background color */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x0c + (0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x0c + (0x10*win_idx)))));
    color->bg_color = (tmp>>12) & 0xf;

    /* foreground color */
    if(PLAT_OSD_PIXEL_BITS == 4) {  ///< GM8210
        color->fg_color = vcap_osd_data[pdev_info->index].fg_color;
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x08 + (0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x08 + (0x10*win_idx)))));
        color->fg_color = (tmp>>14) & 0xf;
    }

    return 0;
}

int vcap_osd_mask_set_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_mask_win_t *win)
{
    u32 tmp;
    unsigned long flags;
    u8  alpha;
    u16 x_start, y_start, x_end, y_end, width, height;

    if((!win) ||
       (win_idx >= VCAP_OSD_WIN_MAX)  ||
       (win->path >= VCAP_SCALER_MAX) ||
       (win->align >= VCAP_ALIGN_MAX) ||
       (win->color >= VCAP_OSD_MASK_PALETTE_MAX)) {
        vcap_err("[%d] ch#%d setup osd_mask window#%d failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    x_start = ALIGN_4(win->x_start);
    y_start = win->y_start;
    width   = (win->width%4) ? (ALIGN_4(win->width) + 4) : ALIGN_4(win->width); ///< round up to multiple of 4
    height  = win->height;
    x_end = ((x_start + width)  > 0) ? (x_start + width  - 1) : 0;
    y_end = ((y_start + height) > 0) ? (y_start + height - 1) : 0;

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        spin_lock_irqsave(&pdev_info->lock, flags);

        /* window type parameter */
        pdev_info->ch[ch].temp_param.osd[win_idx].win_type = VCAP_OSD_WIN_TYPE_MASK;

        /* window auto align parameter */
        pdev_info->ch[ch].temp_param.osd[win_idx].align_type     = win->align;
        pdev_info->ch[ch].temp_param.osd[win_idx].align_x_offset = x_start;
        pdev_info->ch[ch].temp_param.osd[win_idx].align_y_offset = y_start;
        pdev_info->ch[ch].temp_param.osd[win_idx].width          = width;
        pdev_info->ch[ch].temp_param.osd[win_idx].height         = height;

        /*
         * x_start, y_start, x_end, y_end, will be re-caculate in lli tasklet if auto align enable.
         * default set osd window to none align position.
         */
        pdev_info->ch[ch].temp_param.osd[win_idx].path         = win->path;
        pdev_info->ch[ch].temp_param.osd[win_idx].x_start      = x_start & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].y_start      = y_start & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].x_end        = x_end   & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].y_end        = y_end   & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].bg_color     = pdev_info->ch[ch].temp_param.osd[win_idx].fg_color = win->color;
        pdev_info->ch[ch].temp_param.osd[win_idx].font_alpha   = pdev_info->ch[ch].temp_param.osd[win_idx].bg_alpha;
        pdev_info->ch[ch].temp_param.osd[win_idx].wd_addr      = (PLAT_OSD_PIXEL_BITS == 4) ? VCAP_OSD_PALETTE_DISP_IDX(win->color) : 0;

        pdev_info->ch[ch].temp_param.osd[win_idx].col_sp       = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].row_sp       = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].h_wd_num     = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].v_wd_num     = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].zoom         = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].border_en    = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].border_type  = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].border_width = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].border_color = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].marquee_mode = 0;

        spin_unlock_irqrestore(&pdev_info->lock, flags);
    }
    else {
        /* word start address, font color */
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x08+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x08+(0x10*win_idx)))));
        tmp &= ~0xffffffff;
        if(PLAT_OSD_PIXEL_BITS == 4) {  ///< GM8210
            tmp |= (VCAP_OSD_PALETTE_DISP_IDX(win->color) & 0xffff);
        }
        else {
            tmp |= ((win->color & 0xf)<<14);
        }
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x08+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x08+(0x10*win_idx)))), tmp);

        /* background color, task select */
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x0c+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x0c+(0x10*win_idx)))));
        tmp &= ~0xffffffff;
        tmp |= ((win->color & 0xf)<<12) | ((win->path & 0x3)<<30);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x0c+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x0c+(0x10*win_idx)))), tmp);

        /* x position and size */
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x10+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x10+(0x10*win_idx)))));
        tmp &= ~0xffffffff;
        tmp |= (x_start & 0xfff) | ((x_end & 0xfff)<<16);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x10+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x10+(0x10*win_idx)))), tmp);

        /* y position and size */
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))));
        alpha = (tmp>>28)&0x7;  ///< keep bg_alpha value
        tmp &= ~0xffffffff;
        tmp |= (y_start & 0xfff) | (alpha<<12) | ((y_end & 0xfff)<<16) | (alpha<<28);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))), tmp);

        spin_lock_irqsave(&pdev_info->lock, flags);

        /* window type parameter */
        pdev_info->ch[ch].temp_param.osd[win_idx].win_type = pdev_info->ch[ch].param.osd[win_idx].win_type = VCAP_OSD_WIN_TYPE_MASK;

        /* window auto align parameter */
        pdev_info->ch[ch].temp_param.osd[win_idx].align_type     = pdev_info->ch[ch].param.osd[win_idx].align_type     = win->align;
        pdev_info->ch[ch].temp_param.osd[win_idx].align_x_offset = pdev_info->ch[ch].param.osd[win_idx].align_x_offset = x_start;
        pdev_info->ch[ch].temp_param.osd[win_idx].align_y_offset = pdev_info->ch[ch].param.osd[win_idx].align_y_offset = y_start;
        pdev_info->ch[ch].temp_param.osd[win_idx].width          = pdev_info->ch[ch].param.osd[win_idx].width          = width;
        pdev_info->ch[ch].temp_param.osd[win_idx].height         = pdev_info->ch[ch].param.osd[win_idx].height         = height;

        /*
         * x_start, y_start, x_end, y_end, will be re-caculate in lli tasklet if auto align enable.
         * default set osd window to none align position.
         */
        pdev_info->ch[ch].temp_param.osd[win_idx].path         = pdev_info->ch[ch].param.osd[win_idx].path         = win->path;
        pdev_info->ch[ch].temp_param.osd[win_idx].x_start      = pdev_info->ch[ch].param.osd[win_idx].x_start      = x_start & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].y_start      = pdev_info->ch[ch].param.osd[win_idx].y_start      = y_start & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].x_end        = pdev_info->ch[ch].param.osd[win_idx].x_end        = x_end   & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].y_end        = pdev_info->ch[ch].param.osd[win_idx].y_end        = y_end   & 0xfff;
        pdev_info->ch[ch].temp_param.osd[win_idx].font_alpha   = pdev_info->ch[ch].param.osd[win_idx].font_alpha   = alpha;
        pdev_info->ch[ch].temp_param.osd[win_idx].bg_alpha     = pdev_info->ch[ch].param.osd[win_idx].bg_alpha     = alpha;
        pdev_info->ch[ch].temp_param.osd[win_idx].bg_color     = pdev_info->ch[ch].param.osd[win_idx].bg_color     = win->color & 0xf;
        pdev_info->ch[ch].temp_param.osd[win_idx].fg_color     = pdev_info->ch[ch].param.osd[win_idx].fg_color     = win->color & 0xf;
        pdev_info->ch[ch].temp_param.osd[win_idx].wd_addr      = pdev_info->ch[ch].param.osd[win_idx].wd_addr      = (PLAT_OSD_PIXEL_BITS == 4) ? VCAP_OSD_PALETTE_DISP_IDX(win->color) : 0;

        pdev_info->ch[ch].temp_param.osd[win_idx].col_sp       = pdev_info->ch[ch].param.osd[win_idx].col_sp       = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].row_sp       = pdev_info->ch[ch].param.osd[win_idx].row_sp       = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].h_wd_num     = pdev_info->ch[ch].param.osd[win_idx].h_wd_num     = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].v_wd_num     = pdev_info->ch[ch].param.osd[win_idx].v_wd_num     = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].zoom         = pdev_info->ch[ch].param.osd[win_idx].zoom         = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].border_en    = pdev_info->ch[ch].param.osd[win_idx].border_en    = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].border_type  = pdev_info->ch[ch].param.osd[win_idx].border_type  = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].border_width = pdev_info->ch[ch].param.osd[win_idx].border_width = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].border_color = pdev_info->ch[ch].param.osd[win_idx].border_color = 0;
        pdev_info->ch[ch].temp_param.osd[win_idx].marquee_mode = pdev_info->ch[ch].param.osd[win_idx].marquee_mode = 0;

        spin_unlock_irqrestore(&pdev_info->lock, flags);
    }

    return 0;
}

int vcap_osd_mask_get_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_mask_win_t *win)
{
    u32 tmp;

    if(!win || win_idx >= VCAP_OSD_WIN_MAX) {
        vcap_err("[%d] ch#%d get osd_mask window#%d failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    /* bg_color, task select */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x0c+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x0c+(0x10*win_idx)))));
    win->color = (tmp>>12) & 0xf;
    win->path  = (tmp>>30) & 0x3;

    /* x position and size */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x10+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x10+(0x10*win_idx)))));
    win->x_start = tmp & 0xfff;
    if((tmp>>16) & 0xfff)
        win->width = (((tmp>>16) & 0xfff) + 1) - win->x_start;
    else
        win->width = 0;

    /* y position and size */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))));
    win->y_start = (tmp & 0xfff);
    if((tmp>>16) & 0xfff)
        win->height = (((tmp>>16) & 0xfff) + 1) - win->y_start;
    else
        win->height = 0;

    /* align type */
    win->align = pdev_info->ch[ch].param.osd[win_idx].align_type;

    return 0;
}

int vcap_osd_mask_set_alpha(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_OSD_ALPHA_T alpha)
{
    int ret = 0;
    u32 tmp;
    unsigned long flags;

    if((win_idx >= VCAP_OSD_WIN_MAX) || (alpha >= VCAP_OSD_ALPHA_MAX)) {
        vcap_err("[%d] ch#%d set osd_mask window#%d alpha failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(pdev_info->ch[ch].temp_param.osd[win_idx].win_type == VCAP_OSD_WIN_TYPE_FONT) {
        vcap_err("[%d] ch#%d set osd_mask window#%d alpha failed!(window type is osd_font)\n", pdev_info->index, ch, win_idx);
        ret = -1;
        goto exit;
    }

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.osd[win_idx].font_alpha = alpha;  ///< lli tasklet will apply this config and sync to current
        pdev_info->ch[ch].temp_param.osd[win_idx].bg_alpha   = alpha;
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))));
        tmp &= ~0x70007000;
        tmp |= ((alpha<<12) | (alpha<<28));
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))), tmp);

        pdev_info->ch[ch].temp_param.osd[win_idx].font_alpha = pdev_info->ch[ch].param.osd[win_idx].font_alpha = alpha;
        pdev_info->ch[ch].temp_param.osd[win_idx].bg_alpha   = pdev_info->ch[ch].param.osd[win_idx].bg_alpha   = alpha;
    }

exit:
    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return ret;
}

int vcap_osd_mask_get_alpha(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_OSD_ALPHA_T *alpha)
{
    u32 tmp;

    if(!alpha || win_idx >= VCAP_OSD_WIN_MAX) {
        vcap_err("[%d] ch#%d get osd_mask window#%d alpha failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x14+(0x10*win_idx))) : VCAP_CH_OSD_OFFSET(ch, (0x14+(0x10*win_idx)))));
    *alpha = (tmp>>28) & 0x7;

    return 0;
}

int vcap_osd_accs_win_enable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx)
{
#ifdef PLAT_OSD_COLOR_SCHEME
    int ret = 0;
    u32 tmp;
    unsigned long flags;

    if(win_idx >= VCAP_OSD_WIN_MAX) {
        vcap_err("[%d] ch#%d enable osd window#%d auto color change scheme failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(pdev_info->ch[ch].temp_param.osd[win_idx].win_type == VCAP_OSD_WIN_TYPE_MASK) {
        vcap_err("[%d] ch#%d enable osd window#%d auto color change scheme failed!(window type is osd_mask)\n", pdev_info->index, ch, win_idx);
        ret = -1;
        goto exit;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x0c) : VCAP_CH_SUBCH_OFFSET(ch, 0x0c)));
    tmp |= (0x1<<win_idx);
    VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x0c) : VCAP_CH_SUBCH_OFFSET(ch, 0x0c)), tmp);
    pdev_info->ch[ch].temp_param.comm.accs_win_en |= (0x1<<win_idx);
    pdev_info->ch[ch].param.comm.accs_win_en      |= (0x1<<win_idx);

exit:
    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return ret;
#else
    vcap_err("[%d] ch#%d not support auto color change scheme!\n", pdev_info->index, ch);
    return -1;
#endif
}

int vcap_osd_accs_win_disable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx)
{
#ifdef PLAT_OSD_COLOR_SCHEME
    int ret = 0;
    u32 tmp;
    unsigned long flags;

    if(win_idx >= VCAP_OSD_WIN_MAX) {
        vcap_err("[%d] ch#%d enable osd window#%d auto color change scheme failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(pdev_info->ch[ch].temp_param.osd[win_idx].win_type == VCAP_OSD_WIN_TYPE_MASK) {
        vcap_err("[%d] ch#%d enable osd window#%d auto color change scheme failed!(window type is osd_mask)\n", pdev_info->index, ch, win_idx);
        ret = -1;
        goto exit;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x0c) : VCAP_CH_SUBCH_OFFSET(ch, 0x0c)));
    tmp &= ~(0x1<<win_idx);
    VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x0c) : VCAP_CH_SUBCH_OFFSET(ch, 0x0c)), tmp);
    pdev_info->ch[ch].temp_param.comm.accs_win_en &= ~(0x1<<win_idx);
    pdev_info->ch[ch].param.comm.accs_win_en      &= ~(0x1<<win_idx);

exit:
    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return ret;
#else
    vcap_err("[%d] ch#%d not support auto color change scheme!\n", pdev_info->index, ch);
    return -1;
#endif
}

int vcap_osd_set_accs_data_thres(struct vcap_dev_info_t *pdev_info, int ch, int thres)
{
#ifdef PLAT_OSD_COLOR_SCHEME
    u32 tmp;
    unsigned long flags;

    if(thres > VCAP_OSD_ACCS_DATA_THRES_MAX) {
        vcap_err("[%d] ch#%d set osd accs data threshold failed!(%d > MAX=%d)\n", pdev_info->index, ch, thres, VCAP_OSD_ACCS_DATA_THRES_MAX);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.comm.accs_data_thres = thres;      ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)));
        tmp &= ~(0xf<<20);
        tmp |= (thres<<20);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)), tmp);
        pdev_info->ch[ch].temp_param.comm.accs_data_thres = thres;
        pdev_info->ch[ch].param.comm.accs_data_thres      = thres;
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
#else
    vcap_err("[%d] ch#%d not support to set osd accs data threshold!\n", pdev_info->index, ch);
    return -1;
#endif
}

int vcap_osd_get_accs_data_thres(struct vcap_dev_info_t *pdev_info, int ch, int *thres)
{
    u32 tmp;

    if(!thres) {
        vcap_err("[%d] ch#%d get osd accs data threshold!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

#ifdef PLAT_OSD_COLOR_SCHEME
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)));
#else
    tmp = 0;
#endif

    *thres = (tmp>>20) & 0xf;

    return 0;
}
