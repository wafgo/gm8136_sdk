/**
 * @file sen_gc1004.c
 * GC1004 sensor driver
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <asm/io.h>

#define PFX "sen_gc1004"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1280
#define DEFAULT_IMAGE_HEIGHT    720
#define DEFAULT_XCLK            27000000

static ushort sensor_w = DEFAULT_IMAGE_WIDTH;
module_param(sensor_w, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_w, "sensor image width");

static ushort sensor_h = DEFAULT_IMAGE_HEIGHT;
module_param(sensor_h, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_h, "sensor image height");

static uint sensor_xclk = DEFAULT_XCLK;
module_param(sensor_xclk, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_xclk, "sensor external clock frequency");

static uint mirror = 0;
module_param(mirror, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mirror, "sensor mirror function");

static uint flip = 0;
module_param(flip, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(flip, "sensor flip function");

static uint interface = IF_PARALLEL;
module_param(interface, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(interface, "interface type");

static uint ch_num = 4;
module_param(ch_num, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_num, "channel number");

static uint fps = 30;
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "sensor frame rate");

static uint inv_clk = 0;
module_param(inv_clk, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "invert clock, 0:Disable, 1: Enable");

//=============================================================================
// global
//=============================================================================
#define SENSOR_NAME         "GC1004"
#define SENSOR_MAX_WIDTH    1296
#define SENSOR_MAX_HEIGHT   742

static sensor_dev_t *g_psensor = NULL;

typedef struct sensor_info {
    bool is_init;
    u32 img_w;
    u32 img_h;
    u32 Hb;
    u32 t_row;          // unit: 1/10 us
    u32 VB;
    u32 Sh_delay;
    int win_width;
    int win_height;
    int mirror;
    int flip;
} sensor_info_t;

typedef struct sensor_reg {
    u16 addr;
    u8  val;
} sensor_reg_t;

static sensor_reg_t sensor_720p_init_regs[] = {
    /////////////////////////////////////////////////////
    //////////////////////   SYS   //////////////////////
    /////////////////////////////////////////////////////
    {0xfe ,0x80},
    {0xfe ,0x80},
    {0xfe ,0x80},
    {0xf2 ,0x00},//sync_pad_io_ebi
    {0xf6 ,0x00}, //up down
    {0xfc ,0xc6},
    {0xf7 ,0xb9}, //pll enable
    {0xf8 ,0x03}, //Pll mode 2
    {0xf9 ,0x2e}, //[0] pll enable
    {0xfa ,0x00}, //div
    {0xfe ,0x00},
    /////////////////////////////////////////////////////
    ////////////////   ANALOG & CISCTL   ////////////////
    /////////////////////////////////////////////////////
    {0x03 ,0x03},
    {0x04 ,0x00},
    {0x05 ,0x01},
    {0x06 ,0x80},
    {0x07 ,0x00},//vb[12:8]
    {0x08 ,0x10},//vb[7:0]
    {0x0d ,0x02}, //win_height[9:8]
    {0x0e ,0xe8},//win_height[7:0]
    {0x0f ,0x05}, //win_width[10:8]
    {0x10 ,0x10}, //win_width[7:0]
    {0x11 ,0x00},
    {0x12 ,0x0c}, //sh_delay//write big can weaken dark current not work
    {0x17 ,0x14}, //01//14//[0]mirror [1]flip
    {0x18 ,0x0a}, //sdark off
    {0x19 ,0x06},
    {0x1a ,0x09},
    {0x1b ,0x4f},
    {0x1c ,0x21},
    {0x1d ,0xe0},//f0
    {0x1e ,0x7c}, //fe The dark stripes
    {0x1f ,0x08}, //08//comv_r
    {0x20 ,0xa5}, //A vertical bar on the right
    {0x21 ,0x6f}, //2f//20//rsg
    {0x22 ,0xb0},
    {0x23 ,0x32}, //38
    {0x24 ,0x2f}, //PAD drive
    {0x2a ,0x00},
    {0x2c ,0xc8},
    {0x2d ,0x0f},
    {0x2e ,0x40},
    {0x2f ,0x1f}, //exp not work
    {0x25 ,0xc0},
    /////////////////////////////////////////////////////
    //////////////////////   ISP   //////////////////////
    /////////////////////////////////////////////////////
    {0xfe ,0x00},
    {0x8a ,0x00},
    {0x8c ,0x02},
    {0x8e ,0x02}, //luma value not normal
    {0x90 ,0x01},
    {0x94 ,0x02},
    {0x95 ,0x02},
    {0x96 ,0xd0},
    {0x97 ,0x05},
    {0x98 ,0x00},
    /////////////////////////////////////////////////////
    //////////////////////	 MIPI	/////////////////////
    /////////////////////////////////////////////////////
    {0xfe ,0x03},
    {0x01 ,0x00},
    {0x02 ,0x00},
    {0x03 ,0x00},
    {0x06 ,0x00},
    {0x10 ,0x00},
    {0x15 ,0x00},
    /////////////////////////////////////////////////////
    //////////////////////	 BLK	/////////////////////
    /////////////////////////////////////////////////////
    {0xfe ,0x00},
    {0x18 ,0x02},
    {0x1a ,0x01},
    {0x40 ,0x23},
    {0x5e ,0x00},
    {0x66 ,0x20},
    /////////////////////////////////////////////////////
    ////////////////////// Dark SUN /////////////////////
    /////////////////////////////////////////////////////
     //dark sun
    {0xfe ,0x02},
    {0x49 ,0x23},
    {0xa4 ,0x00},
    {0xfe ,0x00},
    /////////////////////////////////////////////////////
    //////////////////////	 Gain	/////////////////////
    /////////////////////////////////////////////////////
    {0xb0 ,0x50},
    {0xb3 ,0x40},
    {0xb4 ,0x40},
    {0xb5 ,0x40},
    {0xfe ,0x00},
    {0x1c ,0x41},
    {0xfe ,0x00},
    /////////////////////////////////////////////////////
    //////////////////////   pad enable   ///////////////
    /////////////////////////////////////////////////////
    {0xfe ,0x00},
    {0xf2 ,0x0f}

};
#define NUM_OF_720P_INIT_REGS (sizeof(sensor_720p_init_regs) / sizeof(sensor_reg_t))

typedef struct gain_set {
    u8  Analog_gain;
    u8  Pre_gain_h;
    u8  Pre_gain_l;
    u16 Golbal_gain;
    u32 total_gain; // UFIX 7.6
} gain_set_t;

static const gain_set_t gain_table[] = {
    {0x00, 0x1, 0x00, 0x40, 64  }, {0x00, 0x1, 0x01, 0x40, 65  }, {0x00, 0x1, 0x02, 0x40, 66  },
    {0x00, 0x1, 0x03, 0x40, 67  }, {0x00, 0x1, 0x04, 0x40, 68  }, {0x00, 0x1, 0x05, 0x40, 69  },
    {0x00, 0x1, 0x06, 0x40, 70  }, {0x00, 0x1, 0x07, 0x40, 71  }, {0x00, 0x1, 0x08, 0x40, 72  },
    {0x00, 0x1, 0x09, 0x40, 73  }, {0x00, 0x1, 0x0A, 0x40, 74  }, {0x00, 0x1, 0x0B, 0x40, 75  },
    {0x00, 0x1, 0x0C, 0x40, 76  }, {0x00, 0x1, 0x0D, 0x40, 77  }, {0x00, 0x1, 0x0E, 0x40, 78  },
    {0x00, 0x1, 0x0F, 0x40, 79  }, {0x00, 0x1, 0x10, 0x40, 80  }, {0x00, 0x1, 0x11, 0x40, 81  },
    {0x00, 0x1, 0x12, 0x40, 82  }, {0x00, 0x1, 0x13, 0x40, 83  }, {0x00, 0x1, 0x14, 0x40, 84  },
    {0x00, 0x1, 0x15, 0x40, 85  }, {0x00, 0x1, 0x16, 0x40, 86  }, {0x00, 0x1, 0x17, 0x40, 87  },
    {0x00, 0x1, 0x18, 0x40, 88  }, {0x00, 0x1, 0x19, 0x40, 89  }, {0x01, 0x1, 0x00, 0x40, 90  },
    {0x01, 0x1, 0x02, 0x40, 92  }, {0x01, 0x1, 0x03, 0x40, 94  }, {0x01, 0x1, 0x04, 0x40, 95  },
    {0x01, 0x1, 0x05, 0x40, 97  }, {0x01, 0x1, 0x06, 0x40, 98  }, {0x01, 0x1, 0x07, 0x40, 99  },
    {0x01, 0x1, 0x08, 0x40, 101 }, {0x01, 0x1, 0x09, 0x40, 102 }, {0x01, 0x1, 0x0A, 0x40, 104 },
    {0x01, 0x1, 0x0B, 0x40, 105 }, {0x01, 0x1, 0x0C, 0x40, 106 }, {0x01, 0x1, 0x0D, 0x40, 108 },
    {0x01, 0x1, 0x0E, 0x40, 109 }, {0x01, 0x1, 0x0F, 0x40, 111 }, {0x01, 0x1, 0x10, 0x40, 112 },
    {0x01, 0x1, 0x11, 0x40, 113 }, {0x01, 0x1, 0x12, 0x40, 115 }, {0x02, 0x1, 0x00, 0x40, 115 },
    {0x02, 0x1, 0x01, 0x40, 117 }, {0x02, 0x1, 0x02, 0x40, 119 }, {0x02, 0x1, 0x03, 0x40, 121 },
    {0x02, 0x1, 0x04, 0x40, 122 }, {0x02, 0x1, 0x05, 0x40, 124 }, {0x02, 0x1, 0x06, 0x40, 126 },
    {0x02, 0x1, 0x07, 0x40, 128 }, {0x02, 0x1, 0x08, 0x40, 130 }, {0x02, 0x1, 0x09, 0x40, 131 },
    {0x02, 0x1, 0x0A, 0x40, 133 }, {0x02, 0x1, 0x0B, 0x40, 135 }, {0x02, 0x1, 0x0C, 0x40, 137 },
    {0x02, 0x1, 0x0D, 0x40, 139 }, {0x02, 0x1, 0x0E, 0x40, 140 }, {0x02, 0x1, 0x0F, 0x40, 142 },
    {0x02, 0x1, 0x10, 0x40, 144 }, {0x02, 0x1, 0x11, 0x40, 146 }, {0x02, 0x1, 0x12, 0x40, 148 },
    {0x02, 0x1, 0x13, 0x40, 149 }, {0x02, 0x1, 0x14, 0x40, 151 }, {0x02, 0x1, 0x15, 0x40, 153 },
    {0x02, 0x1, 0x16, 0x40, 155 }, {0x02, 0x1, 0x17, 0x40, 157 }, {0x02, 0x1, 0x18, 0x40, 158 },
    {0x02, 0x1, 0x19, 0x40, 160 }, {0x02, 0x1, 0x1A, 0x40, 162 }, {0x02, 0x1, 0x1B, 0x40, 164 },
    {0x02, 0x1, 0x1C, 0x40, 166 }, {0x03, 0x1, 0x00, 0x40, 166 }, {0x03, 0x1, 0x01, 0x40, 169 },
    {0x03, 0x1, 0x02, 0x40, 172 }, {0x03, 0x1, 0x03, 0x40, 174 }, {0x03, 0x1, 0x04, 0x40, 177 },
    {0x03, 0x1, 0x05, 0x40, 179 }, {0x03, 0x1, 0x06, 0x40, 182 }, {0x03, 0x1, 0x07, 0x40, 185 },
    {0x03, 0x1, 0x08, 0x40, 187 }, {0x03, 0x1, 0x09, 0x40, 190 }, {0x03, 0x1, 0x0A, 0x40, 192 },
    {0x03, 0x1, 0x0B, 0x40, 195 }, {0x03, 0x1, 0x0C, 0x40, 198 }, {0x03, 0x1, 0x0D, 0x40, 200 },
    {0x03, 0x1, 0x0E, 0x40, 203 }, {0x03, 0x1, 0x0F, 0x40, 205 }, {0x03, 0x1, 0x10, 0x40, 208 },
    {0x03, 0x1, 0x11, 0x40, 211 }, {0x03, 0x1, 0x12, 0x40, 213 }, {0x03, 0x1, 0x13, 0x40, 216 },
    {0x04, 0x1, 0x00, 0x40, 218 }, {0x04, 0x1, 0x01, 0x40, 221 }, {0x04, 0x1, 0x02, 0x40, 224 },
    {0x04, 0x1, 0x03, 0x40, 228 }, {0x04, 0x1, 0x04, 0x40, 231 }, {0x04, 0x1, 0x05, 0x40, 235 },
    {0x04, 0x1, 0x06, 0x40, 238 }, {0x04, 0x1, 0x07, 0x40, 241 }, {0x04, 0x1, 0x08, 0x40, 245 },
    {0x04, 0x1, 0x09, 0x40, 248 }, {0x04, 0x1, 0x0A, 0x40, 252 }, {0x04, 0x1, 0x0B, 0x40, 255 },
    {0x04, 0x1, 0x0C, 0x40, 258 }, {0x04, 0x1, 0x0D, 0x40, 262 }, {0x04, 0x1, 0x0E, 0x40, 265 },
    {0x04, 0x1, 0x0F, 0x40, 269 }, {0x04, 0x1, 0x10, 0x40, 272 }, {0x04, 0x1, 0x11, 0x40, 275 },
    {0x04, 0x1, 0x12, 0x40, 279 }, {0x04, 0x1, 0x13, 0x40, 282 }, {0x04, 0x1, 0x14, 0x40, 286 },
    {0x04, 0x1, 0x15, 0x40, 289 }, {0x04, 0x1, 0x16, 0x40, 292 }, {0x04, 0x1, 0x17, 0x40, 296 },
    {0x04, 0x1, 0x18, 0x40, 299 }, {0x05, 0x1, 0x00, 0x40, 301 }, {0x05, 0x1, 0x01, 0x40, 306 },
    {0x05, 0x1, 0x02, 0x40, 310 }, {0x05, 0x1, 0x03, 0x40, 315 }, {0x05, 0x1, 0x04, 0x40, 320 },
    {0x05, 0x1, 0x05, 0x40, 324 }, {0x05, 0x1, 0x06, 0x40, 329 }, {0x05, 0x1, 0x07, 0x40, 334 },
    {0x05, 0x1, 0x08, 0x40, 338 }, {0x05, 0x1, 0x09, 0x40, 343 }, {0x05, 0x1, 0x0A, 0x40, 348 },
    {0x05, 0x1, 0x0B, 0x40, 353 }, {0x05, 0x1, 0x0C, 0x40, 357 }, {0x05, 0x1, 0x0D, 0x40, 362 },
    {0x05, 0x1, 0x0E, 0x40, 367 }, {0x05, 0x1, 0x0F, 0x40, 371 }, {0x05, 0x1, 0x10, 0x40, 376 },
    {0x05, 0x1, 0x11, 0x40, 381 }, {0x05, 0x1, 0x12, 0x40, 385 }, {0x05, 0x1, 0x13, 0x40, 390 },
    {0x05, 0x1, 0x14, 0x40, 395 }, {0x05, 0x1, 0x15, 0x40, 400 }, {0x05, 0x1, 0x16, 0x40, 404 },
    {0x05, 0x1, 0x17, 0x40, 409 }, {0x05, 0x1, 0x18, 0x40, 414 }, {0x05, 0x1, 0x19, 0x40, 418 },
    {0x05, 0x1, 0x1A, 0x40, 423 }, {0x05, 0x1, 0x1B, 0x40, 428 }, {0x05, 0x1, 0x1C, 0x40, 432 },
    {0x06, 0x1, 0x00, 0x40, 438 }, {0x06, 0x1, 0x01, 0x40, 445 }, {0x06, 0x1, 0x02, 0x40, 451 },
    {0x06, 0x1, 0x03, 0x40, 458 }, {0x06, 0x1, 0x04, 0x40, 465 }, {0x06, 0x1, 0x05, 0x40, 472 },
    {0x06, 0x1, 0x06, 0x40, 479 }, {0x06, 0x1, 0x07, 0x40, 486 }, {0x06, 0x1, 0x08, 0x40, 492 },
    {0x06, 0x1, 0x09, 0x40, 499 }, {0x06, 0x1, 0x0A, 0x40, 506 }, {0x06, 0x1, 0x0B, 0x40, 513 },
    {0x06, 0x1, 0x0C, 0x40, 520 }, {0x06, 0x1, 0x0D, 0x40, 527 }, {0x06, 0x1, 0x0E, 0x40, 534 },
    {0x06, 0x1, 0x0F, 0x40, 540 }, {0x06, 0x1, 0x10, 0x40, 547 }, {0x06, 0x1, 0x11, 0x40, 554 },
    {0x06, 0x1, 0x12, 0x40, 561 }, {0x06, 0x1, 0x13, 0x40, 568 }, {0x06, 0x1, 0x14, 0x40, 575 },
    {0x06, 0x1, 0x15, 0x40, 581 }, {0x06, 0x1, 0x16, 0x40, 588 }, {0x06, 0x1, 0x17, 0x40, 595 },
    {0x07, 0x1, 0x00, 0x40, 602 }, {0x07, 0x1, 0x01, 0x40, 611 }, {0x07, 0x1, 0x02, 0x40, 620 },
    {0x07, 0x1, 0x03, 0x40, 630 }, {0x07, 0x1, 0x04, 0x40, 639 }, {0x07, 0x1, 0x05, 0x40, 649 },
    {0x07, 0x1, 0x06, 0x40, 658 }, {0x07, 0x1, 0x07, 0x40, 667 }, {0x07, 0x1, 0x08, 0x40, 677 },
    {0x07, 0x1, 0x09, 0x40, 686 }, {0x07, 0x1, 0x0A, 0x40, 696 }, {0x07, 0x1, 0x0B, 0x40, 705 },
    {0x07, 0x1, 0x0C, 0x40, 714 }, {0x07, 0x1, 0x0D, 0x40, 724 }, {0x07, 0x1, 0x0E, 0x40, 733 },
    {0x07, 0x1, 0x0F, 0x40, 743 }, {0x07, 0x1, 0x10, 0x40, 752 }, {0x07, 0x1, 0x11, 0x40, 761 },
    {0x07, 0x1, 0x12, 0x40, 771 }, {0x07, 0x1, 0x13, 0x40, 780 }, {0x07, 0x1, 0x14, 0x40, 790 },
    {0x07, 0x1, 0x15, 0x40, 799 }, {0x07, 0x1, 0x16, 0x40, 808 }, {0x07, 0x1, 0x17, 0x40, 818 },
    {0x07, 0x1, 0x18, 0x40, 827 }, {0x07, 0x1, 0x19, 0x40, 837 }, {0x08, 0x1, 0x00, 0x40, 845 },
    {0x08, 0x1, 0x01, 0x40, 858 }, {0x08, 0x1, 0x02, 0x40, 871 }, {0x08, 0x1, 0x03, 0x40, 884 },
    {0x08, 0x1, 0x04, 0x40, 898 }, {0x08, 0x1, 0x05, 0x40, 911 }, {0x08, 0x1, 0x06, 0x40, 924 },
    {0x08, 0x1, 0x07, 0x40, 937 }, {0x08, 0x1, 0x08, 0x40, 950 }, {0x08, 0x1, 0x09, 0x40, 964 },
    {0x08, 0x1, 0x0A, 0x40, 977 }, {0x08, 0x1, 0x0B, 0x40, 990 }, {0x08, 0x1, 0x0C, 0x40, 1003},
    {0x08, 0x1, 0x0D, 0x40, 1016}, {0x08, 0x1, 0x0E, 0x40, 1030}, {0x08, 0x1, 0x0E, 0x48, 1158},
    {0x08, 0x1, 0x0E, 0x50, 1287}, {0x08, 0x1, 0x0E, 0x58, 1416}, {0x08, 0x1, 0x0E, 0x60, 1544},
    {0x08, 0x1, 0x0E, 0x68, 1673}, {0x08, 0x1, 0x0E, 0x70, 1802}, {0x08, 0x1, 0x0E, 0x78, 1931},
    {0x08, 0x1, 0x0E, 0x80, 2059}, {0x08, 0x1, 0x0E, 0x90, 2317}, {0x08, 0x1, 0x0E, 0xA0, 2574},
    {0x08, 0x1, 0x0E, 0xB0, 2831}, {0x08, 0x1, 0x0E, 0xC0, 3089}, {0x08, 0x1, 0x0E, 0xD0, 3346},
    {0x08, 0x1, 0x0E, 0xE0, 3604}, {0x08, 0x1, 0x0E, 0xF0, 3861}, {0x08, 0x1, 0x0E, 0xFF, 4102},
};
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

//=============================================================================
// I2C
//=============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            (0x78 >> 1)
#include "i2c_common.c"

//=============================================================================
// internal functions
//=============================================================================
static int read_reg(u32 addr, u32 *pval)
{
    struct i2c_msg  msgs[2];
    unsigned char   tmp[1], tmp2[1];

    tmp[0]        = addr & 0xFF;
    msgs[0].addr  = I2C_ADDR;
    msgs[0].flags = 0;
    msgs[0].len   = 1;
    msgs[0].buf   = tmp;

    tmp2[0]       = 0;
    msgs[1].addr  = I2C_ADDR;
    msgs[1].flags = 1;
    msgs[1].len   = 1;
    msgs[1].buf   = tmp2;

    if (sensor_i2c_transfer(msgs, 2))
        return -1;

    *pval = tmp2[0];
    return 0;
}

static int write_reg(u32 addr, u32 val)
{
    struct i2c_msg  msgs;
    unsigned char   buf[2];

    buf[0]     = addr & 0xFF;
    buf[1]     = val & 0xFF;
    msgs.addr  = I2C_ADDR;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = buf;

    return sensor_i2c_transfer(&msgs, 1);
}


static u32 get_pclk(u32 xclk)
{
    u32 pixclk;

    if(xclk == 24000000){
        pixclk = 48000000;
        printk("24M\n");
    }
    else if(xclk == 27000000){
        pixclk = 54000000;
        printk("27M\n");
    }

    printk("********GC1004 Pixel Clock %d*********\n", pixclk);
    return pixclk;
}

static void calculate_t_row(void)
{

    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h;

    read_reg(0x05, &reg_h);//[11:8]
    read_reg(0x06, &reg_l);//[7:0]
    pinfo->Hb = ((reg_h & 0x0F) << 8) | (reg_l & 0xFF);
    printk("Current Hb = %d\n", (u32)pinfo->Hb);
    read_reg(0x11, &reg_h);//[9:8]
    read_reg(0x12, &reg_l);//[7:0]
    pinfo->Sh_delay = ((reg_h & 0x03) << 8) | (reg_l & 0xFF);
    read_reg(0x0f, &reg_h);//[10:8]
    read_reg(0x10, &reg_l);//[7:0]
    pinfo->win_width = ((reg_h & 0x07) << 8) | (reg_l & 0xFF);
    read_reg(0x0d ,&reg_h), //win_height[9:8]
    read_reg(0x0e ,&reg_l),//win_height[7:0]
    pinfo->win_height = ((reg_h & 0x03) << 8) | (reg_l & 0xFF);

    pinfo->t_row = ((pinfo->Hb + pinfo->Sh_delay + pinfo->win_width/2 + 4) * 2 * 10000) / (g_psensor->pclk / 1000);

    g_psensor->min_exp = (pinfo->t_row + 500) / 1000;

    //isp_info("t_row=%d\n", pinfo->t_row);

}

static void adjust_blanking(int fps)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int tmp;

    tmp = (10000000 / (fps * pinfo->t_row));

    pinfo->VB = tmp - pinfo->win_height - 16;

    //isp_info("pinfo->VB = %d\n", pinfo->VB);
    write_reg(0x08, (pinfo->VB & 0xFF));//vb[7:0]
    write_reg(0x07, ((pinfo->VB >> 8) & 0x1F));//vb[12:8]

    //isp_info("adjust_blanking, fps=%d, t_row = %d, win_height = %d, tmp =%d\n", fps, pinfo->t_row, pinfo->win_height, tmp);
}

static int set_size(u32 width, u32 height)
{

    int ret = 0;

    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("%dx%d exceeds maximum resolution %dx%d!\n",
            width, height, SENSOR_MAX_WIDTH, SENSOR_MAX_HEIGHT);
        return -1;
    }

    calculate_t_row();

    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    g_psensor->out_win.width = width;
    g_psensor->out_win.height = height;

    g_psensor->img_win.x = 0;
    g_psensor->img_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    //isp_info("width = %d height= %d\n",g_psensor->out_win.width, g_psensor->out_win.height);

    return ret;

}

static u32 get_exp(void)
{

    u32 exp_lines, reg_h, reg_l;

    if (!g_psensor->curr_exp) {
        sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
        read_reg(0x04, &reg_l);//[7:0]
        read_reg(0x03, &reg_h);//[12:8]
        exp_lines = ((reg_h << 8) & 0x1F) |  (reg_l & 0xFF);
        //printk("exp_lines = %d\n", exp_lines);
        g_psensor->curr_exp = (exp_lines * pinfo->t_row + 500) / 1000;
    }

    return g_psensor->curr_exp;

}

static int set_exp(u32 exp)
{

    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 exp_lines;

    exp_lines = ((exp * 1000) + pinfo->t_row /2 ) / pinfo->t_row;

    //printk("lines=%d\n",exp_lines);

    if (exp_lines < 1)
        exp_lines = 1;

    if (exp_lines > 8191)
        exp_lines = 8191;


    //Update shutter
    write_reg(0x04, (exp_lines & 0xFF));//[7:0]
    write_reg(0x03, (exp_lines >> 8) & 0x1F);//[12:8]

    g_psensor->curr_exp = ((exp_lines * pinfo->t_row) + 500) / 1000;

    //printk("curr_exp = %d, t_row=%d, exp_lines=%d\n",g_psensor->curr_exp, pinfo->t_row, exp_lines);

    return ret;

}

static u32 get_gain(void)
{
    u32 reg;

    u32 Anlg = 0,  Pre_gain_h = 0, Pre_gain_l = 0, P_gain = 0,  gain_total = 0, Golbal_gain = 0, Analog_gain = 0;;

    if (g_psensor->curr_gain == 0) {
        //Analog gain
        read_reg(0xb6, &reg);
        Anlg = reg;
        //Pre-gain
        read_reg(0xb1, &reg); //[3:0]
        Pre_gain_h = reg;
        read_reg(0xb2, &reg); //[7:2]
        Pre_gain_l = reg;
        P_gain = Pre_gain_l >> 2;
        //global digital gain
        read_reg(0xb0, &reg);
        Golbal_gain = reg;

        if(Anlg == 0){
            Analog_gain = 1 * (Pre_gain_h+(P_gain/64));
            gain_total = Analog_gain * Golbal_gain;
        }
        if(Anlg == 1){
            Analog_gain = 14 * (Pre_gain_h+(P_gain/64)) / 10;
            gain_total = Analog_gain * Golbal_gain;
        }

        if(Anlg == 2){
            Analog_gain = 18 * (Pre_gain_h+(P_gain/64)) / 10;
            gain_total = Analog_gain * Golbal_gain;
        }
         if(Anlg == 3){
            Analog_gain = 26 * (Pre_gain_h+(P_gain/64)) / 10;
            gain_total = Analog_gain * Golbal_gain;
        }
        if(Anlg == 4){
            Analog_gain = 34 * (Pre_gain_h+(P_gain/64)) / 10;
            gain_total = Analog_gain * Golbal_gain;
        }
        if(Anlg == 5){
            Analog_gain = 47 * (Pre_gain_h+(P_gain/64)) / 10;
            gain_total = Analog_gain * Golbal_gain;
        }
        if(Anlg == 6){
            Analog_gain = 684 * (Pre_gain_h+(P_gain/64)) / 10;
            gain_total = Analog_gain * Golbal_gain;
        }
        if(Anlg == 7){
            Analog_gain = 94 * (Pre_gain_h+(P_gain/64)) / 10;
            gain_total = Analog_gain * Golbal_gain;
        }
        if(Anlg == 8){
            Analog_gain = 132 * (Pre_gain_h+(P_gain/64)) / 10;
            gain_total = Analog_gain * Golbal_gain;
        }

        g_psensor->curr_gain = gain_total;
        //printk("get_curr_gain = %d\n",g_psensor->curr_gain);
    }

    return g_psensor->curr_gain;

}

static int set_gain(u32 gain)
{

    int ret = 0;
    int i = 0;
    u8 Analog_gain = 0, Pre_gain_h = 0, Pre_gain_l = 0, Golbal_gain = 0;

    // search most suitable gain into gain table
    if (gain > gain_table[NUM_OF_GAINSET - 1].total_gain)
        gain = gain_table[NUM_OF_GAINSET - 1].total_gain;
    else if (gain < gain_table[0].total_gain)
        gain = gain_table[0].total_gain;
	else{
	    for (i=0; i<NUM_OF_GAINSET; i++) {
    	    if (gain_table[i].total_gain > gain)
        	    break;
    	    }
	}

    Analog_gain = gain_table[i-1].Analog_gain;
    Pre_gain_h = gain_table[i-1].Pre_gain_h;
    Pre_gain_l = gain_table[i-1].Pre_gain_l;
    Golbal_gain = gain_table[i-1].Golbal_gain;

    //Analog gain
    write_reg(0xb6, Analog_gain);
    //Pre-gain
    write_reg(0xb1, Pre_gain_h); //[3:0]
    write_reg(0xb2, (Pre_gain_l <<2) & 0xfc); //[7:2]
    //global digital gain
    write_reg(0xb0, Golbal_gain);

    g_psensor->curr_gain = gain_table[i-1].total_gain;

    //printk("g_psensor->curr_gain = %d\n",g_psensor->curr_gain);

    return ret;

}

static void update_bayer_type(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (pinfo->mirror) {
        if (pinfo->flip)
            g_psensor->bayer_type = BAYER_BG;
        else
            g_psensor->bayer_type = BAYER_GR;
    } else {
        if (pinfo->flip)
            g_psensor->bayer_type = BAYER_GB;
        else
            g_psensor->bayer_type = BAYER_RG;
    }
}

static int get_mirror(void)
{
    u32 reg;

    read_reg(0x17, &reg);
    return (reg & BIT0) ? 1 : 0;
}

static int set_mirror(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    read_reg(0x17, &reg);
    if (pinfo->flip)
        reg |= BIT1;
    else
        reg &=~BIT1;

    if (enable) {
        write_reg(0x17, (reg | BIT0));
    } else {
        write_reg(0x17, (reg &~BIT0));
    }
    pinfo->mirror = enable;
    update_bayer_type();

    return 0;
}

static int get_flip(void)
{
    u32 reg;

    read_reg(0x17, &reg);
    return (reg & BIT1) ? 1 : 0;
}

static int set_flip(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    read_reg(0x17, &reg);
    if (pinfo->mirror)
        reg |= BIT0;
    else
        reg &=~BIT0;

    if (enable) {
        write_reg(0x17, (reg | BIT1));
    } else {
        write_reg(0x17, (reg &~BIT1));
    }
    pinfo->flip = enable;
    update_bayer_type();

    return 0;
}

static int set_fps(int fps)
{
    adjust_blanking(fps);

    g_psensor->fps = fps;

    return 0;
}

static int get_property(enum SENSOR_CMD cmd, unsigned long arg)
{
    int ret = 0;

    switch (cmd) {
        case ID_MIRROR:
            *(int*)arg = get_mirror();
            break;

    case ID_FLIP:
        *(int*)arg = get_flip();
        break;

    case ID_FPS:
        *(int*)arg = g_psensor->fps;
        break;

        default:
            ret = -EFAULT;
            break;
    }

    return ret;
}

static int set_property(enum SENSOR_CMD cmd, unsigned long arg)
{
    int ret = 0;

    switch (cmd) {
        case ID_MIRROR:
            set_mirror((int)arg);
            break;

        case ID_FLIP:
            set_flip((int)arg);
            break;

        case ID_FPS:
            set_fps((int)arg);
            break;
        default:
            ret = -EFAULT;
            break;
    }

    return ret;
}

static int init(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0, i;
    sensor_reg_t* pInitTable;
    u32 InitTable_Num;

    if (pinfo->is_init)
        return 0;

    if ((g_psensor->img_win.width <= 1280) && (g_psensor->img_win.height <= 720)) {
        pInitTable = sensor_720p_init_regs;
        InitTable_Num = NUM_OF_720P_INIT_REGS;
        //printk("1\n");
    }

    // write initial register value
    isp_info("init %s\n", SENSOR_NAME);
    for (i=0; i<InitTable_Num; i++) {
        if (write_reg(pInitTable[i].addr, pInitTable[i].val) < 0) {
            isp_err("failed to initial %s!\n", SENSOR_NAME);
            return -EINVAL;
        }
        //printk("i=%d\n",i);
    }

    // get pixel clock
    g_psensor->pclk = get_pclk(g_psensor->xclk);

    // set image size
    if ((ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height)) < 0)
        return ret;

    set_fps(g_psensor->fps);
    // set mirror and flip
    set_mirror(mirror);
    set_flip(flip);

    // initial exposure and gain
    set_exp(g_psensor->min_exp);
    set_gain(g_psensor->min_gain);

    // get current shutter_width and gain setting
    get_exp();
    get_gain();

    pinfo->is_init = true;

    return ret;
}

//=============================================================================
// external functions
//=============================================================================
static void GC1004_deconstruct(void)
{
    if ((g_psensor) && (g_psensor->private)) {
        kfree(g_psensor->private);
    }

    sensor_remove_i2c_driver();
    // turn off EXT_CLK
    isp_api_extclk_onoff(0);
    // release CAP_RST
    isp_api_cap_rst_exit();
}

static int GC1004_construct(u32 xclk, u16 width, u16 height)
{
    sensor_info_t *pinfo = NULL;
    int ret = 0;

    // initial CAP_RST
    ret = isp_api_cap_rst_init();
    if (ret < 0)
        return ret;

    // clear CAP_RST
    isp_api_cap_rst_set_value(0);

    // set EXT_CLK frequency and turn on it.
    ret = isp_api_extclk_set_freq(xclk);
    if (ret < 0)
        return ret;
    ret = isp_api_extclk_onoff(1);
    if (ret < 0)
        return ret;
    mdelay(50);

    // set CAP_RST
    isp_api_cap_rst_set_value(1);
    mdelay(50);

    pinfo = kzalloc(sizeof(sensor_info_t), GFP_KERNEL);
    if (!pinfo) {
        isp_err("failed to allocate private data!\n");
        return -ENOMEM;
    }

    g_psensor->private = pinfo;
    snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, SENSOR_NAME);
    g_psensor->capability = SUPPORT_MIRROR | SUPPORT_FLIP;
    g_psensor->xclk = xclk;
    g_psensor->interface = interface;
    g_psensor->num_of_lane = ch_num;
    g_psensor->protocol = 0;
    g_psensor->fmt = RAW10;
    g_psensor->bayer_type = BAYER_RG;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_LOW;
    g_psensor->inv_clk = inv_clk;
    g_psensor->img_win.x = 0;
    g_psensor->img_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;
    g_psensor->min_exp = 1;
    g_psensor->max_exp = 5000; // 0.5 sec
    g_psensor->min_gain = gain_table[0].total_gain;
    g_psensor->max_gain = gain_table[NUM_OF_GAINSET - 1].total_gain;
    g_psensor->exp_latency = 1;
    g_psensor->gain_latency = 1;
    g_psensor->fps = fps;
    g_psensor->fn_init = init;
    g_psensor->fn_read_reg = read_reg;
    g_psensor->fn_write_reg = write_reg;
    g_psensor->fn_set_size = set_size;
    g_psensor->fn_get_exp = get_exp;
    g_psensor->fn_set_exp = set_exp;
    g_psensor->fn_get_gain = get_gain;
    g_psensor->fn_set_gain = set_gain;
    g_psensor->fn_get_property = get_property;
    g_psensor->fn_set_property = set_property;

    if ((ret = sensor_init_i2c_driver()) < 0)
        goto construct_err;
	if (g_psensor->interface == IF_PARALLEL){
    	if ((ret = init()) < 0)
        	goto construct_err;
	}

    return 0;

construct_err:
    GC1004_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init GC1004_init(void)
{
    int ret = 0;

    // check ISP driver version
    if (isp_get_input_inf_ver() != ISP_INPUT_INF_VER) {
        isp_err("input module version(%x) is not equal to fisp320.ko(%x)!\n",
             ISP_INPUT_INF_VER, isp_get_input_inf_ver());
        ret = -EINVAL;
        goto init_err1;
    }


    g_psensor = kzalloc(sizeof(sensor_dev_t), GFP_KERNEL);
    if (!g_psensor) {
        ret = -ENODEV;
        goto init_err1;
    }

    if ((ret = GC1004_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
        goto init_err2;

    // register sensor device to ISP driver
    if ((ret = isp_register_sensor(g_psensor)) < 0)
        goto init_err2;

    return ret;

init_err2:
    kfree(g_psensor);

init_err1:
    return ret;
}

static void __exit GC1004_exit(void)
{
    isp_unregister_sensor(g_psensor);
    GC1004_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(GC1004_init);
module_exit(GC1004_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor GC1004");
MODULE_LICENSE("GPL");
