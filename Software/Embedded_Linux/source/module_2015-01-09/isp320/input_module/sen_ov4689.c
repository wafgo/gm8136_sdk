/**
 * @file sen_ov4689.c
 * OmniVision OV4689 sensor driver
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.7 $
 * $Date: 2014/08/25 07:30:22 $
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

#define PFX "sen_ov4689"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1920
#define DEFAULT_IMAGE_HEIGHT    1080
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

static uint interface = IF_MIPI;
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
#define SENSOR_NAME         "OV4689"
#define SENSOR_MAX_WIDTH    2688
#define SENSOR_MAX_HEIGHT   1520


static sensor_dev_t *g_psensor = NULL;

typedef struct sensor_info {
    bool is_init;
    u32 img_w;
    u32 img_h;
    int ae_en;
    int awb_en;
    u32 sw_lines_max;
    u32 lines_max;
    u32 prev_sw_res;
    u32 prev_sw_reg;
    u32 t_row;          // unit: 1/10 us
    int htp;
    int mirror;
    int flip;
} sensor_info_t;

typedef struct sensor_reg {
    u16 addr;
    u8  val;
} sensor_reg_t;

static sensor_reg_t sensor_1080p_init_regs[] = {
    {0x0103, 0x01},
    {0x3638, 0x00},
    {0x0300, 0x00},
    {0x0302, 0x2a},
    {0x0303, 0x00},
    {0x0304, 0x03},
    {0x030b, 0x00},
    {0x030d, 0x1b},
    {0x030e, 0x04},
    {0x030f, 0x01},
    {0x0312, 0x01},
    {0x031e, 0x00},
    {0x3000, 0x20},
    {0x3002, 0x00},
    {0x3018, 0x72},
    {0x3020, 0x93},
    {0x3021, 0x03},
    {0x3022, 0x01},
    {0x3031, 0x0a},
    {0x303f, 0x0c},
    {0x3305, 0xf1},
    {0x3307, 0x04},
    {0x3309, 0x29},
    {0x3500, 0x00},
    {0x3501, 0x60},
    {0x3502, 0x00},
    {0x3503, 0x04},
    {0x3504, 0x00},
    {0x3505, 0x00},
    {0x3506, 0x00},
    {0x3507, 0x00},
    {0x3508, 0x00},
    {0x3509, 0x80},
    {0x350a, 0x00},
    {0x350b, 0x00},
    {0x350c, 0x00},
    {0x350d, 0x00},
    {0x350e, 0x00},
    {0x350f, 0x80},
    {0x3510, 0x00},
    {0x3511, 0x00},
    {0x3512, 0x00},
    {0x3513, 0x00},
    {0x3514, 0x00},
    {0x3515, 0x80},
    {0x3516, 0x00},
    {0x3517, 0x00},
    {0x3518, 0x00},
    {0x3519, 0x00},
    {0x351a, 0x00},
    {0x351b, 0x80},
    {0x351c, 0x00},
    {0x351d, 0x00},
    {0x351e, 0x00},
    {0x351f, 0x00},
    {0x3520, 0x00},
    {0x3521, 0x80},
    {0x3522, 0x08},
    {0x3524, 0x08},
    {0x3526, 0x08},
    {0x3528, 0x08},
    {0x352a, 0x08},
    {0x3602, 0x00},
    {0x3603, 0x40},
    {0x3604, 0x02},
    {0x3605, 0x00},
    {0x3606, 0x00},
    {0x3607, 0x00},
    {0x3609, 0x12},
    {0x360a, 0x40},
    {0x360c, 0x08},
    {0x360f, 0xe5},
    {0x3608, 0x8f},
    {0x3611, 0x00},
    {0x3613, 0xf7},
    {0x3616, 0x58},
    {0x3619, 0x99},
    {0x361b, 0x60},
    {0x361c, 0x7a},
    {0x361e, 0x79},
    {0x361f, 0x02},
    {0x3632, 0x00},
    {0x3633, 0x10},
    {0x3634, 0x10},
    {0x3635, 0x10},
    {0x3636, 0x15},
    {0x3646, 0x86},
    {0x364a, 0x0b},
    {0x3700, 0x17},
    {0x3701, 0x22},
    {0x3703, 0x10},
    {0x370a, 0x37},
    {0x3705, 0x00},
    {0x3706, 0x63},
    {0x3709, 0x3c},
    {0x370b, 0x01},
    {0x370c, 0x30},
    {0x3710, 0x24},
    {0x3711, 0x0c},
    {0x3716, 0x00},
    {0x3720, 0x28},
    {0x3729, 0x7b},
    {0x372a, 0x84},
    {0x372b, 0xbd},
    {0x372c, 0xbc},
    {0x372e, 0x52},
    {0x373c, 0x0e},
    {0x373e, 0x33},
    {0x3743, 0x10},
    {0x3744, 0x88},
    {0x3745, 0xc0},
    {0x374a, 0x43},
    {0x374c, 0x00},
    {0x374e, 0x23},
    {0x3751, 0x7b},
    {0x3752, 0x84},
    {0x3753, 0xbd},
    {0x3754, 0xbc},
    {0x3756, 0x52},
    {0x375c, 0x00},
    {0x3760, 0x00},
    {0x3761, 0x00},
    {0x3762, 0x00},
    {0x3763, 0x00},
    {0x3764, 0x00},
    {0x3767, 0x04},
    {0x3768, 0x04},
    {0x3769, 0x08},
    {0x376a, 0x08},
    {0x376b, 0x20},
    {0x376c, 0x00},
    {0x376d, 0x00},
    {0x376e, 0x00},
    {0x3773, 0x00},
    {0x3774, 0x51},
    {0x3776, 0xbd},
    {0x3777, 0xbd},
    {0x3781, 0x18},
    {0x3783, 0x25},
    {0x3798, 0x1b},
    {0x3800, 0x00},
    {0x3801, 0x08},
    {0x3802, 0x00},
    {0x3803, 0x04},
    {0x3804, 0x0a},
    {0x3805, 0x97},
    {0x3806, 0x05},
    {0x3807, 0xfb},
    {0x3808, 0x0a},
    {0x3809, 0x80},
    {0x380a, 0x05},
    {0x380b, 0xf0},
    {0x380c, 0x03},
    {0x380d, 0x5c},
    {0x380e, 0x06},
    {0x380f, 0x12},
    {0x3810, 0x00},
    {0x3811, 0x08},
    {0x3812, 0x00},
    {0x3813, 0x04},
    {0x3814, 0x01},
    {0x3815, 0x01},
    {0x3819, 0x01},
    {0x3820, 0x00},
    {0x3821, 0x06},
    {0x3829, 0x00},
    {0x382a, 0x01},
    {0x382b, 0x01},
    {0x382d, 0x7f},
    {0x3830, 0x04},
    {0x3836, 0x01},
    {0x3837, 0x00},
    {0x3841, 0x02},
    {0x3846, 0x08},
    {0x3847, 0x07},
    {0x3d85, 0x36},
    {0x3d8c, 0x71},
    {0x3d8d, 0xcb},
    {0x3f0a, 0x00},
    {0x4000, 0x71},//BLC
    {0x4001, 0x40},
    {0x4002, 0x04},
    {0x4003, 0x14},
    {0x400e, 0x00},
    {0x4011, 0x00},
    {0x401a, 0x00},
    {0x401b, 0x00},
    {0x401c, 0x00},
    {0x401d, 0x00},
    {0x401f, 0x00},
    {0x4020, 0x00},
    {0x4021, 0x10},
    {0x4022, 0x07},
    {0x4023, 0xcf},
    {0x4024, 0x09},
    {0x4025, 0x60},
    {0x4026, 0x09},
    {0x4027, 0x6f},
    {0x4028, 0x00},
    {0x4029, 0x02},
    {0x402a, 0x06},
    {0x402b, 0x04},
    {0x402c, 0x02},
    {0x402d, 0x02},
    {0x402e, 0x0e},
    {0x402f, 0x04},
    {0x4302, 0xff},
    {0x4303, 0xff},
    {0x4304, 0x00},
    {0x4305, 0x00},
    {0x4306, 0x00},
    {0x4308, 0x02},
    {0x4500, 0x6c},
    {0x4501, 0xc4},
    {0x4502, 0x40},
    {0x4503, 0x01},
    {0x4601, 0xA7},
    {0x4800, 0x04},
    {0x4813, 0x08},
    {0x481f, 0x40},
    {0x4829, 0x78},
    {0x4837, 0x10},
    {0x4b00, 0x2a},
    {0x4b0d, 0x00},
    {0x4d00, 0x04},
    {0x4d01, 0x42},
    {0x4d02, 0xd1},
    {0x4d03, 0x93},
    {0x4d04, 0xf5},
    {0x4d05, 0xc1},
    {0x5000, 0xf3},
    {0x5001, 0x11},
    {0x5004, 0x00},
    {0x500a, 0x00},
    {0x500b, 0x00},
    {0x5032, 0x00},
    {0x5040, 0x00},
    {0x5050, 0x0c},
    {0x5500, 0x00},
    {0x5501, 0x10},
    {0x5502, 0x01},
    {0x5503, 0x0f},
    {0x8000, 0x00},
    {0x8001, 0x00},
    {0x8002, 0x00},
    {0x8003, 0x00},
    {0x8004, 0x00},
    {0x8005, 0x00},
    {0x8006, 0x00},
    {0x8007, 0x00},
    {0x8008, 0x00},
    {0x3638, 0x00},
    {0x0100, 0x01},

    //Stream on
    {0x0100, 0x01},

    //Stream off
    {0x0100, 0x00},

    {0x0100, 0x00},

    {0x0300, 0x00},
    {0x0302, 0x14},//540Mbps
    {0x4837, 0x19},

    {0x3632, 0x00},
    {0x376b, 0x20},
    {0x3820, 0x00},//flip
    {0x3821, 0x00},//mirror
    {0x382a, 0x01},
    {0x3830, 0x04},
    {0x3836, 0x01},
    {0x4001, 0x40},
    {0x4502, 0x40},
    {0x5050, 0x0c},

    {0x3800, 0x00},
    {0x3801, 0x08},
    {0x3802, 0x00},
    {0x3803, 0x04},
    {0x3804, 0x0A},
    {0x3805, 0x97},
    {0x3806, 0x05},
    {0x3807, 0xFB},
    {0x3808, 0x0A},
    {0x3809, 0x80},
    {0x380A, 0x05},
    {0x380B, 0xF2},//V_OUT_SIZE
    {0x380C, 0x0a},
    {0x380D, 0x2e},
    {0x380E, 0x06},
    {0x380F, 0x12},
    {0x3810, 0x00},
    {0x3811, 0x08},
    {0x3812, 0x00},
    {0x3813, 0x04},

    {0x3814, 0x01},
    {0x3815, 0x01},

    {0x4020, 0x00},
    {0x4021, 0x10},
    {0x4022, 0x09},
    {0x4023, 0x13},
    {0x4024, 0x0A},
    {0x4025, 0x40},
    {0x4026, 0x0A},
    {0x4027, 0x50},
    {0x4600, 0x00},
    {0x4601, 0xA7},

    {0x0100, 0x01}
};
#define NUM_OF_1080P_INIT_REGS (sizeof(sensor_1080p_init_regs) / sizeof(sensor_reg_t))


typedef struct gain_set {
    u8  gain_mul2;
    u8  gain_step;
    u16 digtal_gain;
    u32 total_gain; // UFIX 7.6
} gain_set_t;

static const gain_set_t gain_table[] = {
    {0x0, 0x80, 2048 , 64   }, {0x0, 0x81, 2048 , 65   }, {0x0, 0x83, 2048 , 66   },
    {0x0, 0x85, 2048 , 67   }, {0x0, 0x87, 2048 , 68   }, {0x0, 0x89, 2048 , 69   },
    {0x0, 0x8B, 2048 , 70   }, {0x0, 0x8D, 2048 , 71   }, {0x0, 0x8F, 2048 , 72   },
    {0x0, 0x91, 2048 , 73   }, {0x0, 0x93, 2048 , 74   }, {0x0, 0x95, 2048 , 75   },
    {0x0, 0x97, 2048 , 76   }, {0x0, 0x99, 2048 , 77   }, {0x0, 0x9B, 2048 , 78   },
    {0x0, 0x9D, 2048 , 79   }, {0x0, 0x9F, 2048 , 80   }, {0x0, 0xA1, 2048 , 81   },
    {0x0, 0xA3, 2048 , 82   }, {0x0, 0xA5, 2048 , 83   }, {0x0, 0xA7, 2048 , 84   },
    {0x0, 0xA9, 2048 , 85   }, {0x0, 0xAB, 2048 , 86   }, {0x0, 0xAD, 2048 , 87   },
    {0x0, 0xAF, 2048 , 88   }, {0x0, 0xB1, 2048 , 89   }, {0x0, 0xB3, 2048 , 90   },
    {0x0, 0xB5, 2048 , 91   }, {0x0, 0xB7, 2048 , 92   }, {0x0, 0xB9, 2048 , 93   },
    {0x0, 0xBB, 2048 , 94   }, {0x0, 0xBD, 2048 , 95   }, {0x0, 0xBF, 2048 , 96   },
    {0x0, 0xC1, 2048 , 97   }, {0x0, 0xC3, 2048 , 98   }, {0x0, 0xC5, 2048 , 99   },
    {0x0, 0xC7, 2048 , 100  }, {0x0, 0xC9, 2048 , 101  }, {0x0, 0xCB, 2048 , 102  },
    {0x0, 0xCD, 2048 , 103  }, {0x0, 0xCF, 2048 , 104  }, {0x0, 0xD1, 2048 , 105  },
    {0x0, 0xD3, 2048 , 106  }, {0x0, 0xD5, 2048 , 107  }, {0x0, 0xD7, 2048 , 108  },
    {0x0, 0xD9, 2048 , 109  }, {0x0, 0xDB, 2048 , 110  }, {0x0, 0xDD, 2048 , 111  },
    {0x0, 0xDF, 2048 , 112  }, {0x0, 0xE1, 2048 , 113  }, {0x0, 0xE3, 2048 , 114  },
    {0x0, 0xE5, 2048 , 115  }, {0x0, 0xE7, 2048 , 116  }, {0x0, 0xE9, 2048 , 117  },
    {0x0, 0xEB, 2048 , 118  }, {0x0, 0xED, 2048 , 119  }, {0x0, 0xEF, 2048 , 120  },
    {0x0, 0xF1, 2048 , 121  }, {0x0, 0xF3, 2048 , 122  }, {0x0, 0xF5, 2048 , 123  },
    {0x0, 0xF7, 2048 , 124  }, {0x0, 0xF9, 2048 , 125  }, {0x0, 0xFB, 2048 , 126  },
    {0x0, 0xFD, 2048 , 127  }, {0x0, 0xFF, 2048 , 128  }, {0x1, 0x79, 2048 , 129  },
    {0x1, 0x7A, 2048 , 130  }, {0x1, 0x7B, 2048 , 131  }, {0x1, 0x7C, 2048 , 132  },
    {0x1, 0x7D, 2048 , 133  }, {0x1, 0x7E, 2048 , 134  }, {0x1, 0x7F, 2048 , 135  },
    {0x1, 0x80, 2048 , 136  }, {0x1, 0x81, 2048 , 137  }, {0x1, 0x82, 2048 , 138  },
    {0x1, 0x83, 2048 , 139  }, {0x1, 0x84, 2048 , 140  }, {0x1, 0x85, 2048 , 141  },
    {0x1, 0x86, 2048 , 142  }, {0x1, 0x87, 2048 , 143  }, {0x1, 0x88, 2048 , 144  },
    {0x1, 0x89, 2048 , 145  }, {0x1, 0x8A, 2048 , 146  }, {0x1, 0x8B, 2048 , 147  },
    {0x1, 0x8C, 2048 , 148  }, {0x1, 0x8D, 2048 , 149  }, {0x1, 0x8E, 2048 , 150  },
    {0x1, 0x8F, 2048 , 151  }, {0x1, 0x90, 2048 , 152  }, {0x1, 0x91, 2048 , 153  },
    {0x1, 0x92, 2048 , 154  }, {0x1, 0x93, 2048 , 155  }, {0x1, 0x94, 2048 , 156  },
    {0x1, 0x95, 2048 , 157  }, {0x1, 0x96, 2048 , 158  }, {0x1, 0x97, 2048 , 159  },
    {0x1, 0x98, 2048 , 160  }, {0x1, 0x99, 2048 , 161  }, {0x1, 0x9A, 2048 , 162  },
    {0x1, 0x9B, 2048 , 163  }, {0x1, 0x9C, 2048 , 164  }, {0x1, 0x9D, 2048 , 165  },
    {0x1, 0x9E, 2048 , 166  }, {0x1, 0x9F, 2048 , 167  }, {0x1, 0xA0, 2048 , 168  },
    {0x1, 0xA1, 2048 , 169  }, {0x1, 0xA2, 2048 , 170  }, {0x1, 0xA3, 2048 , 171  },
    {0x1, 0xA4, 2048 , 172  }, {0x1, 0xA5, 2048 , 173  }, {0x1, 0xA6, 2048 , 174  },
    {0x1, 0xA7, 2048 , 175  }, {0x1, 0xA8, 2048 , 176  }, {0x1, 0xA9, 2048 , 177  },
    {0x1, 0xAA, 2048 , 178  }, {0x1, 0xAB, 2048 , 179  }, {0x1, 0xAC, 2048 , 180  },
    {0x1, 0xAD, 2048 , 181  }, {0x1, 0xAE, 2048 , 182  }, {0x1, 0xAF, 2048 , 183  },
    {0x1, 0xB0, 2048 , 184  }, {0x1, 0xB1, 2048 , 185  }, {0x1, 0xB2, 2048 , 186  },
    {0x1, 0xB3, 2048 , 187  }, {0x1, 0xB4, 2048 , 188  }, {0x1, 0xB5, 2048 , 189  },
    {0x1, 0xB6, 2048 , 190  }, {0x1, 0xB7, 2048 , 191  }, {0x1, 0xB8, 2048 , 192  },
    {0x1, 0xB9, 2048 , 193  }, {0x1, 0xBA, 2048 , 194  }, {0x1, 0xBB, 2048 , 195  },
    {0x1, 0xBC, 2048 , 196  }, {0x1, 0xBD, 2048 , 197  }, {0x1, 0xBE, 2048 , 198  },
    {0x1, 0xBF, 2048 , 199  }, {0x1, 0xC0, 2048 , 200  }, {0x1, 0xC1, 2048 , 201  },
    {0x1, 0xC2, 2048 , 202  }, {0x1, 0xC3, 2048 , 203  }, {0x1, 0xC4, 2048 , 204  },
    {0x1, 0xC5, 2048 , 205  }, {0x1, 0xC6, 2048 , 206  }, {0x1, 0xC7, 2048 , 207  },
    {0x1, 0xC8, 2048 , 208  }, {0x1, 0xC9, 2048 , 209  }, {0x1, 0xCA, 2048 , 210  },
    {0x1, 0xCB, 2048 , 211  }, {0x1, 0xCC, 2048 , 212  }, {0x1, 0xCD, 2048 , 213  },
    {0x1, 0xCE, 2048 , 214  }, {0x1, 0xCF, 2048 , 215  }, {0x1, 0xD0, 2048 , 216  },
    {0x1, 0xD1, 2048 , 217  }, {0x1, 0xD2, 2048 , 218  }, {0x1, 0xD3, 2048 , 219  },
    {0x1, 0xD4, 2048 , 220  }, {0x1, 0xD5, 2048 , 221  }, {0x1, 0xD6, 2048 , 222  },
    {0x1, 0xD7, 2048 , 223  }, {0x1, 0xD8, 2048 , 224  }, {0x1, 0xD9, 2048 , 225  },
    {0x1, 0xDA, 2048 , 226  }, {0x1, 0xDB, 2048 , 227  }, {0x1, 0xDC, 2048 , 228  },
    {0x1, 0xDD, 2048 , 229  }, {0x1, 0xDE, 2048 , 230  }, {0x1, 0xDF, 2048 , 231  },
    {0x1, 0xE0, 2048 , 232  }, {0x1, 0xE1, 2048 , 233  }, {0x1, 0xE2, 2048 , 234  },
    {0x1, 0xE3, 2048 , 235  }, {0x1, 0xE4, 2048 , 236  }, {0x1, 0xE5, 2048 , 237  },
    {0x1, 0xE6, 2048 , 238  }, {0x1, 0xE7, 2048 , 239  }, {0x1, 0xE8, 2048 , 240  },
    {0x1, 0xE9, 2048 , 241  }, {0x1, 0xEA, 2048 , 242  }, {0x1, 0xEB, 2048 , 243  },
    {0x1, 0xEC, 2048 , 244  }, {0x1, 0xED, 2048 , 245  }, {0x1, 0xEE, 2048 , 246  },
    {0x1, 0xEF, 2048 , 247  }, {0x1, 0xF0, 2048 , 248  }, {0x1, 0xF1, 2048 , 249  },
    {0x1, 0xF2, 2048 , 250  }, {0x1, 0xF3, 2048 , 251  }, {0x1, 0xF4, 2048 , 252  },
    {0x1, 0xF5, 2048 , 253  }, {0x1, 0xF6, 2048 , 254  }, {0x1, 0xF7, 2048 , 255  },
    {0x3, 0x74, 2048 , 256  }, {0x3, 0x75, 2048 , 258  }, {0x3, 0x76, 2048 , 260  },
    {0x3, 0x77, 2048 , 262  }, {0x3, 0x78, 2048 , 264  }, {0x3, 0x79, 2048 , 266  },
    {0x3, 0x7A, 2048 , 268  }, {0x3, 0x7B, 2048 , 270  }, {0x3, 0x7C, 2048 , 272  },
    {0x3, 0x7D, 2048 , 274  }, {0x3, 0x7E, 2048 , 276  }, {0x3, 0x7F, 2048 , 278  },
    {0x3, 0x80, 2048 , 280  }, {0x3, 0x81, 2048 , 282  }, {0x3, 0x82, 2048 , 284  },
    {0x3, 0x83, 2048 , 286  }, {0x3, 0x84, 2048 , 288  }, {0x3, 0x85, 2048 , 290  },
    {0x3, 0x86, 2048 , 292  }, {0x3, 0x87, 2048 , 294  }, {0x3, 0x88, 2048 , 296  },
    {0x3, 0x89, 2048 , 298  }, {0x3, 0x8A, 2048 , 300  }, {0x3, 0x8B, 2048 , 302  },
    {0x3, 0x8C, 2048 , 304  }, {0x3, 0x8D, 2048 , 306  }, {0x3, 0x8E, 2048 , 308  },
    {0x3, 0x8F, 2048 , 310  }, {0x3, 0x90, 2048 , 312  }, {0x3, 0x91, 2048 , 314  },
    {0x3, 0x92, 2048 , 316  }, {0x3, 0x93, 2048 , 318  }, {0x3, 0x94, 2048 , 320  },
    {0x3, 0x95, 2048 , 322  }, {0x3, 0x96, 2048 , 324  }, {0x3, 0x97, 2048 , 326  },
    {0x3, 0x98, 2048 , 328  }, {0x3, 0x99, 2048 , 330  }, {0x3, 0x9A, 2048 , 332  },
    {0x3, 0x9B, 2048 , 334  }, {0x3, 0x9C, 2048 , 336  }, {0x3, 0x9D, 2048 , 338  },
    {0x3, 0x9E, 2048 , 340  }, {0x3, 0x9F, 2048 , 342  }, {0x3, 0xA0, 2048 , 344  },
    {0x3, 0xA1, 2048 , 346  }, {0x3, 0xA2, 2048 , 348  }, {0x3, 0xA3, 2048 , 350  },
    {0x3, 0xA4, 2048 , 352  }, {0x3, 0xA5, 2048 , 354  }, {0x3, 0xA6, 2048 , 356  },
    {0x3, 0xA7, 2048 , 358  }, {0x3, 0xA8, 2048 , 360  }, {0x3, 0xA9, 2048 , 362  },
    {0x3, 0xAA, 2048 , 364  }, {0x3, 0xAB, 2048 , 366  }, {0x3, 0xAC, 2048 , 368  },
    {0x3, 0xAD, 2048 , 370  }, {0x3, 0xAE, 2048 , 372  }, {0x3, 0xAF, 2048 , 374  },
    {0x3, 0xB0, 2048 , 376  }, {0x3, 0xB1, 2048 , 378  }, {0x3, 0xB2, 2048 , 380  },
    {0x3, 0xB3, 2048 , 382  }, {0x3, 0xB4, 2048 , 384  }, {0x3, 0xB5, 2048 , 386  },
    {0x3, 0xB6, 2048 , 388  }, {0x3, 0xB7, 2048 , 390  }, {0x3, 0xB8, 2048 , 392  },
    {0x3, 0xB9, 2048 , 394  }, {0x3, 0xBA, 2048 , 396  }, {0x3, 0xBB, 2048 , 398  },
    {0x3, 0xBC, 2048 , 400  }, {0x3, 0xBD, 2048 , 402  }, {0x3, 0xBE, 2048 , 404  },
    {0x3, 0xBF, 2048 , 406  }, {0x3, 0xC0, 2048 , 408  }, {0x3, 0xC1, 2048 , 410  },
    {0x3, 0xC2, 2048 , 412  }, {0x3, 0xC3, 2048 , 414  }, {0x3, 0xC4, 2048 , 416  },
    {0x3, 0xC5, 2048 , 418  }, {0x3, 0xC6, 2048 , 420  }, {0x3, 0xC7, 2048 , 422  },
    {0x3, 0xC8, 2048 , 424  }, {0x3, 0xC9, 2048 , 426  }, {0x3, 0xCA, 2048 , 428  },
    {0x3, 0xCB, 2048 , 430  }, {0x3, 0xCC, 2048 , 432  }, {0x3, 0xCD, 2048 , 434  },
    {0x3, 0xCE, 2048 , 436  }, {0x3, 0xCF, 2048 , 438  }, {0x3, 0xD0, 2048 , 440  },
    {0x3, 0xD1, 2048 , 442  }, {0x3, 0xD2, 2048 , 444  }, {0x3, 0xD3, 2048 , 446  },
    {0x3, 0xD4, 2048 , 448  }, {0x3, 0xD5, 2048 , 450  }, {0x3, 0xD6, 2048 , 452  },
    {0x3, 0xD7, 2048 , 454  }, {0x3, 0xD8, 2048 , 456  }, {0x3, 0xD9, 2048 , 458  },
    {0x3, 0xDA, 2048 , 460  }, {0x3, 0xDB, 2048 , 462  }, {0x3, 0xDC, 2048 , 464  },
    {0x3, 0xDD, 2048 , 466  }, {0x3, 0xDE, 2048 , 468  }, {0x3, 0xDF, 2048 , 470  },
    {0x3, 0xE0, 2048 , 472  }, {0x3, 0xE1, 2048 , 474  }, {0x3, 0xE2, 2048 , 476  },
    {0x3, 0xE3, 2048 , 478  }, {0x3, 0xE4, 2048 , 480  }, {0x3, 0xE5, 2048 , 482  },
    {0x3, 0xE6, 2048 , 484  }, {0x3, 0xE7, 2048 , 486  }, {0x3, 0xE8, 2048 , 488  },
    {0x3, 0xE9, 2048 , 490  }, {0x3, 0xEA, 2048 , 492  }, {0x3, 0xEB, 2048 , 494  },
    {0x3, 0xEC, 2048 , 496  }, {0x3, 0xED, 2048 , 498  }, {0x3, 0xEE, 2048 , 500  },
    {0x3, 0xEF, 2048 , 502  }, {0x3, 0xF0, 2048 , 504  }, {0x3, 0xF1, 2048 , 506  },
    {0x3, 0xF2, 2048 , 508  }, {0x3, 0xF3, 2048 , 510  }, {0x7, 0x78, 2048 , 512  },
    {0x7, 0x79, 2048 , 516  }, {0x7, 0x7A, 2048 , 520  }, {0x7, 0x7B, 2048 , 524  },
    {0x7, 0x7C, 2048 , 528  }, {0x7, 0x7D, 2048 , 532  }, {0x7, 0x7E, 2048 , 536  },
    {0x7, 0x7F, 2048 , 540  }, {0x7, 0x80, 2048 , 544  }, {0x7, 0x81, 2048 , 548  },
    {0x7, 0x82, 2048 , 552  }, {0x7, 0x83, 2048 , 556  }, {0x7, 0x84, 2048 , 560  },
    {0x7, 0x85, 2048 , 564  }, {0x7, 0x86, 2048 , 568  }, {0x7, 0x87, 2048 , 572  },
    {0x7, 0x88, 2048 , 576  }, {0x7, 0x89, 2048 , 580  }, {0x7, 0x8A, 2048 , 584  },
    {0x7, 0x8B, 2048 , 588  }, {0x7, 0x8C, 2048 , 592  }, {0x7, 0x8D, 2048 , 596  },
    {0x7, 0x8E, 2048 , 600  }, {0x7, 0x8F, 2048 , 604  }, {0x7, 0x90, 2048 , 608  },
    {0x7, 0x91, 2048 , 612  }, {0x7, 0x92, 2048 , 616  }, {0x7, 0x93, 2048 , 620  },
    {0x7, 0x94, 2048 , 624  }, {0x7, 0x95, 2048 , 628  }, {0x7, 0x96, 2048 , 632  },
    {0x7, 0x97, 2048 , 636  }, {0x7, 0x98, 2048 , 640  }, {0x7, 0x99, 2048 , 644  },
    {0x7, 0x9A, 2048 , 648  }, {0x7, 0x9B, 2048 , 652  }, {0x7, 0x9C, 2048 , 656  },
    {0x7, 0x9D, 2048 , 660  }, {0x7, 0x9E, 2048 , 664  }, {0x7, 0x9F, 2048 , 668  },
    {0x7, 0xA0, 2048 , 672  }, {0x7, 0xA1, 2048 , 676  }, {0x7, 0xA2, 2048 , 680  },
    {0x7, 0xA3, 2048 , 684  }, {0x7, 0xA4, 2048 , 688  }, {0x7, 0xA5, 2048 , 692  },
    {0x7, 0xA6, 2048 , 696  }, {0x7, 0xA7, 2048 , 700  }, {0x7, 0xA8, 2048 , 704  },
    {0x7, 0xA9, 2048 , 708  }, {0x7, 0xAA, 2048 , 712  }, {0x7, 0xAB, 2048 , 716  },
    {0x7, 0xAC, 2048 , 720  }, {0x7, 0xAD, 2048 , 724  }, {0x7, 0xAE, 2048 , 728  },
    {0x7, 0xAF, 2048 , 732  }, {0x7, 0xB0, 2048 , 736  }, {0x7, 0xB1, 2048 , 740  },
    {0x7, 0xB2, 2048 , 744  }, {0x7, 0xB3, 2048 , 748  }, {0x7, 0xB4, 2048 , 752  },
    {0x7, 0xB5, 2048 , 756  }, {0x7, 0xB6, 2048 , 760  }, {0x7, 0xB7, 2048 , 764  },
    {0x7, 0xB8, 2048 , 768  }, {0x7, 0xB9, 2048 , 772  }, {0x7, 0xBA, 2048 , 776  },
    {0x7, 0xBB, 2048 , 780  }, {0x7, 0xBC, 2048 , 784  }, {0x7, 0xBD, 2048 , 788  },
    {0x7, 0xBE, 2048 , 792  }, {0x7, 0xBF, 2048 , 796  }, {0x7, 0xC0, 2048 , 800  },
    {0x7, 0xC1, 2048 , 804  }, {0x7, 0xC2, 2048 , 808  }, {0x7, 0xC3, 2048 , 812  },
    {0x7, 0xC4, 2048 , 816  }, {0x7, 0xC5, 2048 , 820  }, {0x7, 0xC6, 2048 , 824  },
    {0x7, 0xC7, 2048 , 828  }, {0x7, 0xC8, 2048 , 832  }, {0x7, 0xC9, 2048 , 836  },
    {0x7, 0xCA, 2048 , 840  }, {0x7, 0xCB, 2048 , 844  }, {0x7, 0xCC, 2048 , 848  },
    {0x7, 0xCD, 2048 , 852  }, {0x7, 0xCE, 2048 , 856  }, {0x7, 0xCF, 2048 , 860  },
    {0x7, 0xD0, 2048 , 864  }, {0x7, 0xD1, 2048 , 868  }, {0x7, 0xD2, 2048 , 872  },
    {0x7, 0xD3, 2048 , 876  }, {0x7, 0xD4, 2048 , 880  }, {0x7, 0xD5, 2048 , 884  },
    {0x7, 0xD6, 2048 , 888  }, {0x7, 0xD7, 2048 , 892  }, {0x7, 0xD8, 2048 , 896  },
    {0x7, 0xD9, 2048 , 900  }, {0x7, 0xDA, 2048 , 904  }, {0x7, 0xDB, 2048 , 908  },
    {0x7, 0xDC, 2048 , 912  }, {0x7, 0xDD, 2048 , 916  }, {0x7, 0xDE, 2048 , 920  },
    {0x7, 0xDF, 2048 , 924  }, {0x7, 0xE0, 2048 , 928  }, {0x7, 0xE1, 2048 , 932  },
    {0x7, 0xE2, 2048 , 936  }, {0x7, 0xE3, 2048 , 940  }, {0x7, 0xE4, 2048 , 944  },
    {0x7, 0xE5, 2048 , 948  }, {0x7, 0xE6, 2048 , 952  }, {0x7, 0xE7, 2048 , 956  },
    {0x7, 0xE8, 2048 , 960  }, {0x7, 0xE9, 2048 , 964  }, {0x7, 0xEA, 2048 , 968  },
    {0x7, 0xEB, 2048 , 972  }, {0x7, 0xEC, 2048 , 976  }, {0x7, 0xED, 2048 , 980  },
    {0x7, 0xEE, 2048 , 984  }, {0x7, 0xEF, 2048 , 988  }, {0x7, 0xF0, 2048 , 992  },
#if 0    
    {0x7, 0xF0, 2368 , 1147 }, {0x7, 0xF0, 2688 , 1302 }, {0x7, 0xF0, 3008 , 1457 },
    {0x7, 0xF0, 3328 , 1612 }, {0x7, 0xF0, 3648 , 1767 }, {0x7, 0xF0, 3968 , 1922 },
    {0x7, 0xF0, 4288 , 2077 }, {0x7, 0xF0, 4608 , 2232 }, {0x7, 0xF0, 4928 , 2387 },
    {0x7, 0xF0, 5248 , 2542 }, {0x7, 0xF0, 5568 , 2697 }, {0x7, 0xF0, 5888 , 2852 },
    {0x7, 0xF0, 6208 , 3007 }, {0x7, 0xF0, 6528 , 3162 }, {0x7, 0xF0, 6848 , 3317 },
    {0x7, 0xF0, 7168 , 3472 }, {0x7, 0xF0, 7488 , 3627 }, {0x7, 0xF0, 7808 , 3782 },
    {0x7, 0xF0, 8128 , 3937 }, {0x7, 0xF0, 8448 , 4092 }, {0x7, 0xF0, 8768 , 4247 },
    {0x7, 0xF0, 9088 , 4402 }, {0x7, 0xF0, 9408 , 4557 }, {0x7, 0xF0, 9728 , 4712 },
    {0x7, 0xF0, 10048, 4867 }, {0x7, 0xF0, 10368, 5022 }, {0x7, 0xF0, 10688, 5177 },
    {0x7, 0xF0, 11008, 5332 }, {0x7, 0xF0, 11328, 5487 }, {0x7, 0xF0, 11648, 5642 },
    {0x7, 0xF0, 11968, 5797 }, {0x7, 0xF0, 12288, 5952 }, {0x7, 0xF0, 12608, 6107 },
    {0x7, 0xF0, 12928, 6262 }, {0x7, 0xF0, 13248, 6417 }, {0x7, 0xF0, 13568, 6572 },
    {0x7, 0xF0, 13888, 6727 }, {0x7, 0xF0, 14208, 6882 }, {0x7, 0xF0, 14528, 7037 },
    {0x7, 0xF0, 14848, 7192 }, {0x7, 0xF0, 15168, 7347 }, {0x7, 0xF0, 15488, 7502 },
    {0x7, 0xF0, 15808, 7657 }, {0x7, 0xF0, 16128, 7812 }, {0x7, 0xF0, 16448, 7967 },
    {0x7, 0xF0, 16768, 8122 }, {0x7, 0xF0, 17088, 8277 }, {0x7, 0xF0, 17408, 8432 },
    {0x7, 0xF0, 17728, 8587 }, {0x7, 0xF0, 18048, 8742 }, {0x7, 0xF0, 18368, 8897 },
    {0x7, 0xF0, 18688, 9052 }, {0x7, 0xF0, 19008, 9207 }, {0x7, 0xF0, 19328, 9362 },
    {0x7, 0xF0, 19648, 9517 }, {0x7, 0xF0, 19968, 9672 }, {0x7, 0xF0, 20288, 9827 },
    {0x7, 0xF0, 20608, 9982 }, {0x7, 0xF0, 20928, 10137}, {0x7, 0xF0, 21248, 10292},
    {0x7, 0xF0, 21568, 10447}, {0x7, 0xF0, 21888, 10602}, {0x7, 0xF0, 22208, 10757},
    {0x7, 0xF0, 22528, 10912}, {0x7, 0xF0, 22848, 11067}, {0x7, 0xF0, 23168, 11222},
    {0x7, 0xF0, 23488, 11377}, {0x7, 0xF0, 23808, 11532}, {0x7, 0xF0, 24128, 11687},
    {0x7, 0xF0, 24448, 11842}, {0x7, 0xF0, 24768, 11997}, {0x7, 0xF0, 25088, 12152},
    {0x7, 0xF0, 25408, 12307}, {0x7, 0xF0, 25728, 12462}, {0x7, 0xF0, 26048, 12617},
    {0x7, 0xF0, 26368, 12772}, {0x7, 0xF0, 26688, 12927}, {0x7, 0xF0, 27008, 13082},
    {0x7, 0xF0, 27328, 13237}, {0x7, 0xF0, 27648, 13392}, {0x7, 0xF0, 27968, 13547},
    {0x7, 0xF0, 28288, 13702}, {0x7, 0xF0, 28608, 13857}, {0x7, 0xF0, 28928, 14012},
    {0x7, 0xF0, 29248, 14167}, {0x7, 0xF0, 29568, 14322}, {0x7, 0xF0, 29888, 14477},
    {0x7, 0xF0, 30208, 14632}, {0x7, 0xF0, 30528, 14787}, {0x7, 0xF0, 30848, 14942},
    {0x7, 0xF0, 31168, 15097}, {0x7, 0xF0, 31488, 15252}, {0x7, 0xF0, 31808, 15407},
    {0x7, 0xF0, 32128, 15562}, {0x7, 0xF0, 32448, 15717}, {0x7, 0xF0, 32767, 15872},
#endif    
};
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

//=============================================================================
// I2C
//=============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            (0x6C >> 1)
#include "i2c_common.c"

//=============================================================================
// internal functions
//=============================================================================
static int read_reg(u32 addr, u32 *pval)
{
    struct i2c_msg  msgs[2];
    unsigned char   tmp[2], tmp2[2];

    tmp[0]        = (addr >> 8) & 0xFF;
    tmp[1]        = addr & 0xFF;
    msgs[0].addr  = I2C_ADDR;
    msgs[0].flags = 0;
    msgs[0].len   = 2;
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
    unsigned char   buf[3];

    buf[0]     = (addr >> 8) & 0xFF;
    buf[1]     = addr & 0xFF;
    buf[2]     = val & 0xFF;
    msgs.addr  = I2C_ADDR;
    msgs.flags = 0;
    msgs.len   = 3;
    msgs.buf   = buf;

    return sensor_i2c_transfer(&msgs, 1);
}

static u32 get_pclk(u32 xclk)
{
    u32 pixclk;

    pixclk = 121500000;
    printk("********OV4689 Pixel Clock %d*********\n", pixclk);
    return pixclk;
}

static void adjust_blanking(int fps)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    pinfo->sw_lines_max = pinfo->lines_max*30/fps - 8;
    isp_info("lines_max=%d, sw_lines_max=%d\n", pinfo->lines_max, pinfo->sw_lines_max);
    write_reg(0x380f, ((pinfo->sw_lines_max+8) & 0xFF));
    write_reg(0x380e, (((pinfo->sw_lines_max+8) >> 8) & 0xFF));

    isp_info("adjust_blanking, fps=%d\n", fps);

}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h, reg_msb;
    u32 hts;

    read_reg(0x380C, &reg_h);
    read_reg(0x380D, &reg_l);
    hts = ((reg_h & 0x1F) << 8) | (reg_l & 0xFF);
    printk("Current HTS = %d\n", (u32)hts);
    pinfo->t_row = (hts * 10000) / (g_psensor->pclk / 1000);
    g_psensor->min_exp = (pinfo->t_row + 500) / 1000;

    read_reg(0x380E, &reg_h);
    read_reg(0x380F, &reg_l);

    pinfo->sw_lines_max = (((reg_h & 0xF) << 8) | (reg_l & 0xFF)) - 8;

    read_reg(0x3502, &reg_l);
    read_reg(0x3501, &reg_h);
    read_reg(0x3500, &reg_msb);
    pinfo->prev_sw_reg = ((reg_msb & 0xFF) << 16) | ((reg_h & 0xFF) << 8) | (reg_l & 0xFF);

	isp_info("t_row=%d, pinfo->sw_lines_max=%d\n", pinfo->t_row, pinfo->sw_lines_max);
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
    if (g_psensor->interface == IF_MIPI){
        g_psensor->out_win.width = SENSOR_MAX_WIDTH;
        g_psensor->out_win.height = SENSOR_MAX_HEIGHT;
    }
    else{
        g_psensor->out_win.width = SENSOR_MAX_WIDTH;
        g_psensor->out_win.height = SENSOR_MAX_HEIGHT;
    }
    g_psensor->img_win.x = (((SENSOR_MAX_WIDTH - width) >> 1)) &~BIT0;
    g_psensor->img_win.y = (((SENSOR_MAX_HEIGHT - height) >> 1)) &~BIT0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    return ret;
}

static u32 get_exp(void)
{

    u32 lines, sw_t, sw_m, sw_b;

    if (!g_psensor->curr_exp) {
        sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
        read_reg(0x3500, &sw_t);
        read_reg(0x3501, &sw_m);
        read_reg(0x3502, &sw_b);
        lines = ((sw_t & 0xF) << 12)|((sw_m & 0xFF) << 4)|((sw_b & 0xFF) >> 4);
        //printk("lines = %d\n",lines);
        g_psensor->curr_exp = (lines * pinfo->t_row + 500) / 1000;
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 lines, tmp;

    lines = ((exp * 1000) + pinfo->t_row /2 ) / pinfo->t_row;

    if (lines == 0)
        lines = 1;

    tmp = lines << 4;

    if (tmp != pinfo->prev_sw_reg) {
        write_reg(0x3502, (tmp & 0xFF));
        write_reg(0x3501, (tmp >> 8) & 0xFF);
        write_reg(0x3500, (tmp >> 16) & 0xFF);
        pinfo->prev_sw_reg = tmp;
    }

    g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;

    return ret;
}

static u32 get_gain(void)
{
    u32 reg;
    u32 Anlg_h = 0, Anlg_l = 0, gain_total = 0, reg_h = 0, reg_l = 0, digtal_gain = 0, Analog_gain = 0;

    if (g_psensor->curr_gain == 0) {
        read_reg(0x3508, &reg);
        Anlg_h = reg;
        read_reg(0x3509, &reg);
        Anlg_l = reg;
        //global digital gain
        read_reg(0x352a, &reg_h);//high byte
        read_reg(0x352b, &reg_l);//low byte

        digtal_gain = ((reg_h << 8) & 0xFF00) | (reg_l & 0xFF);

        if(Anlg_h == 0){
            Analog_gain = Anlg_l / 128;
            gain_total = Analog_gain * 64;
        }
        if(Anlg_h == 1){
            Analog_gain = (Anlg_l+8)/64;
            gain_total = Analog_gain * 64;
        }
        if(Anlg_h == 3){
            Analog_gain = (Anlg_l+12)/32;
            gain_total = Analog_gain * 64;
        }
        if(Anlg_h == 7){
            Analog_gain = (Anlg_l+8)/16;
            gain_total = Analog_gain * 64;
            if((Anlg_h == 7) && (Anlg_l == 0xF0)){
                gain_total = (Analog_gain * digtal_gain / 2048)*64;
            }
        }
        g_psensor->curr_gain = gain_total;
    }
    return g_psensor->curr_gain;
}

static int set_gain(u32 gain)
{
    int ret = 0;
    int i = 0;
    u8 gain_mul2 = 0, gain_step = 0, reg_h = 0, reg_l = 0;
    u16 digtal_gain = 0;

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

    gain_mul2 = gain_table[i-1].gain_mul2;//high byte
    gain_step = gain_table[i-1].gain_step;//low byte
    digtal_gain = gain_table[i-1].digtal_gain;

    //Digtal gain
    reg_l = digtal_gain & 0xFF;//[7:0]
    reg_h = (digtal_gain >> 8) & 0x7F;//[14:8]

    //Analog gain
    write_reg(0x3508, gain_mul2);
    write_reg(0x3509, gain_step);
    //global digital gain
    write_reg(0x352a, reg_h);//high byte
    write_reg(0x352b, reg_l);//low byte

    g_psensor->curr_gain = gain_table[i-1].total_gain;

    return ret;
}

static int get_mirror(void)
{
    u32 reg;

    read_reg(0x3821, &reg);
    return (reg & BIT2) ? 1 : 0;
}

static int set_mirror(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_mir;

    read_reg(0x3821, &reg_mir);

    if (enable) {
        write_reg(0x3821, (reg_mir | BIT2 | BIT1));
    } else {
        write_reg(0x3821, (reg_mir & ~(BIT2 | BIT1)));
    }
    pinfo->mirror = enable;

    return 0;
}

static int get_flip(void)
{
    u32 reg;

    read_reg(0x3820, &reg);
    return (reg & BIT2) ? 1 : 0;
}

static int set_flip(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_flp;


    read_reg(0x3820, &reg_flp);

    if (enable) {
        write_reg(0x3820, (reg_flp | BIT2 | BIT1));
    } else {
        write_reg(0x3820, (reg_flp & ~(BIT2 | BIT1)));
    }
    pinfo->flip = enable;

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
    u32 readtmp;
    sensor_reg_t* pInitTable;
    u32 InitTable_Num;

    if (pinfo->is_init)
        return 0;

    if ((g_psensor->img_win.width <= 1920) && (g_psensor->img_win.height <= 1080)) {
		pInitTable = sensor_1080p_init_regs;
        InitTable_Num = NUM_OF_1080P_INIT_REGS;
	}

    // write initial register value
    isp_info("init %s\n", SENSOR_NAME);
    for (i=0; i<InitTable_Num; i++) {
        if (write_reg(pInitTable[i].addr, pInitTable[i].val) < 0) {
            isp_err("failed to initial %s!\n", SENSOR_NAME);
            return -EINVAL;
        }
//        mdelay(1);
    }

    mdelay(300);      //wait register update

    read_reg(0x380e, &readtmp);
    read_reg(0x380f, &(pinfo->lines_max));
    pinfo->lines_max |= readtmp << 8;
    isp_info("lines_max=%d\n", pinfo->lines_max);
          
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

    // get current exposure and gain setting
    get_exp();
    get_gain();

    pinfo->is_init = true;

    return ret;
}

//=============================================================================
// external functions
//=============================================================================
static void ov4689_deconstruct(void)
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

static int ov4689_construct(u32 xclk, u16 width, u16 height)
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
    g_psensor->bayer_type = BAYER_BG;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_HIGH;
    g_psensor->inv_clk = inv_clk;
    g_psensor->img_win.x = 0;
    g_psensor->img_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;
    g_psensor->min_exp = 1;
    g_psensor->max_exp = 5000; // 0.5 sec
    g_psensor->min_gain = gain_table[0].total_gain;
    g_psensor->max_gain = gain_table[NUM_OF_GAINSET - 1].total_gain;
	//g_psensor->max_gain = 4096;//For better image quality.
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
    ov4689_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init ov4689_init(void)
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

    if ((ret = ov4689_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit ov4689_exit(void)
{
    isp_unregister_sensor(g_psensor);
    ov4689_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(ov4689_init);
module_exit(ov4689_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor OV4689");
MODULE_LICENSE("GPL");
