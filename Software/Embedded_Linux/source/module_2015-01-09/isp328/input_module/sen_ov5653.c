/**
 * @file sen_ov5653.c
 * OmniVision OV5653 sensor driver
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
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

#define PFX "sen_ov5653"
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

static uint interface = IF_PARALLEL;
module_param(interface, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(interface, "interface type");

static uint ch_num = 2;
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
#define SENSOR_NAME         "OV5653"
#define SENSOR_MAX_WIDTH    2592
#define SENSOR_MAX_HEIGHT   1944


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

static sensor_reg_t sensor_init_regs[] = {
    {0x3008,0x82},
    {0x3008,0x42},
    {0x3103,0x93},
    {0x3b07,0x0c},
    {0x3017,0xff},
    {0x3018,0xfc},
    {0x3706,0x41},
    {0x3703,0xe6},
    {0x3613,0x44},
    {0x3630,0x22},
    {0x3605,0x04},
    {0x3606,0x3f},
    {0x3712,0x13},
    {0x370e,0x00},
    {0x370b,0x40},
    {0x3600,0x54},
    {0x3601,0x05},
    {0x3713,0x22},
    {0x3714,0x27},
    {0x3631,0x22},
    {0x3612,0x1a},
    {0x3604,0x40},
    {0x3705,0xda},
    {0x370a,0x80},
    {0x370c,0x00},
    {0x3710,0x28},
    {0x3702,0x3a},
    {0x3704,0x18},
    {0x3a18,0x00},
    {0x3a19,0xf8},
    {0x3a00,0x38},
    {0x3800,0x02},
    {0x3801,0x54},
    {0x3803,0x0c},
    {0x3804,0x0a},      //
    {0x3805,0x20},      //
    {0x3806,0x07},      //
    {0x3807,0xa0},      //IN-Vsize
    {0x3808,0x0a},      //
    {0x3809,0x20},      //
    {0x380a,0x07},      //
    {0x380b,0xa0},      //OUT-Vsize
    {0x380c,0x0c},
    {0x380d,0xb4},
    {0x380e,0x07},
    {0x380f,0xb0},
    {0x381c,0x20},      //
    {0x381d,0x0a},      //
    {0x381e,0x01},      //
    {0x381f,0x20},      //
    {0x3820,0x00},      //
    {0x3821,0x00},      //
    {0x3824,0x01},      //H-Start
    {0x3825,0xb4},      //H-Start
    {0x3830,0x50},
    {0x3a08,0x12},
    {0x3a09,0x70},
    {0x3a0a,0x0f},
    {0x3a0b,0x60},
    {0x3a0d,0x06},
    {0x3a0e,0x06},
    {0x3a13,0x54},
    {0x3815,0x82},
    {0x5059,0x80},
    {0x3615,0x52},
    {0x505a,0x0a},
    {0x505b,0x2e},
    {0x3a1a,0x06},
    //;disable AGC/AEC
    {0x3503,0x17},
    {0x3500,0x00},
    {0x3501,0x10},
    {0x3502,0x00},
    {0x350a,0x00},
    {0x350b,0x10},
    //;disable AWB
    {0x3406,0x01},      //
    {0x3400,0x04},      //
    {0x3401,0x00},      //
    {0x3402,0x04},      //
    {0x3403,0x00},      //
    {0x3404,0x04},      //
    {0x3623,0x01},
    {0x3633,0x24},
    {0x3c01,0x34},
    {0x3c04,0x28},
    {0x3c05,0x98},
    {0x3c07,0x07},
    {0x3c09,0xc2},
    {0x4000,0x05},
    {0x401d,0x28},
    {0x4001,0x02},
    {0x401c,0x46},
    {0x5046,0x01},
    {0x3810,0x40},
    {0x3836,0x41},
    {0x505f,0x04},
    {0x5000,0x00},
    {0x5001,0x00},
    {0x5002,0x00},
    {0x503d,0x00},
    {0x5901,0x00},
    {0x585a,0x01},
    {0x585b,0x2c},
    {0x585c,0x01},
    {0x585d,0x93},
    {0x585e,0x01},
    {0x585f,0x90},
    {0x5860,0x01},
    {0x5861,0x0d},
    {0x5180,0xc0},
    {0x5184,0x00},
    {0x470a,0x00},
    {0x470b,0x00},
    {0x470c,0x00},
    {0x300f,0x8e},
    {0x3603,0xa7},
    {0x3632,0x55},
    {0x3620,0x56},
    {0x3621,0x2f},
    {0x381a,0x3c},
    {0x3818,0xc0},
    {0x3631,0x36},
    {0x3632,0x5f},
    {0x3711,0x24},
    {0x401f,0x03},
    {0x3008,0x02},
};
#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_5M_sub_init_regs[] = {
    {0x3703,0xe6},
    {0x3613,0x44},
    {0x370d,0x04},
    {0x3713,0x22},
    {0x3714,0x27},
    {0x3705,0xda},
    {0x370a,0x80},
    {0x3801,0x54},
    {0x3804,0x0a},
    {0x3805,0x20},
    {0x3806,0x07},
    {0x3807,0xa0},
    {0x3808,0x0a},
    {0x3809,0x20},
    {0x380a,0x07},
    {0x380b,0xa0},
    {0x380c,0x0c},
    {0x380d,0xb4},
    {0x380e,0x07},
    {0x380f,0xb0},
    {0x381c,0x20},
    {0x381d,0x0a},
    {0x381e,0x01},
    {0x381f,0x20},
    {0x3820,0x00},
    {0x3821,0x00},
    {0x3824,0x01},      //H-Start
    {0x3825,0xb4},      //H-Start
    {0x3a08,0x12},
    {0x3a09,0x70},
    {0x3a0a,0x0f},
    {0x3a0b,0x60},
    {0x3a0d,0x06},
    {0x3a0e,0x06},
    {0x3815,0x82},
    {0x3501,0x7c},  //
    {0x3502,0x40},  //
    {0x350b,0x11},  //
    {0x401c,0x46},
    {0x3621,0x3f},
    {0x381a,0x3c},
};
#define NUM_OF_5M_SUB_INIT_REGS (sizeof(sensor_5M_sub_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_3M_sub_init_regs[] = {
    {0x3703,0xe6},
    {0x3613,0x44},
    {0x370d,0x04},
    {0x3713,0x22},
    {0x3714,0x27},
    {0x3705,0xda},
    {0x370a,0x80},
    {0x3801,0x94},
    {0x3804,0x08},
    {0x3805,0x00},
    {0x3806,0x06},
    {0x3807,0x00},//IN-Vsize, 1536
    {0x3808,0x08},
    {0x3809,0x00},
    {0x380a,0x06},
    {0x380b,0x00},//OUT-Vsize, 1536
    {0x380c,0x10},
    {0x380d,0x1b},//HTS
    {0x380e,0x06},//VTS
    {0x380f,0x10},//
    {0x381c,0x30},
    {0x381d,0xde},
    {0x381e,0x06},
    {0x381f,0x20},
    {0x3820,0x04},
    {0x3821,0x1a},
    {0x3824,0x22},//H-Start
    {0x3825,0xac},//H-Start
    {0x3a08,0x16},
    {0x3a09,0x48},
    {0x3a0a,0x12},
    {0x3a0b,0x90},
    {0x3a0d,0x03},
    {0x3a0e,0x03},
    {0x3815,0x82},
    {0x3501,0x7c},  //
    {0x3502,0x40},  //
    {0x350b,0x11},  //
    {0x401c,0x46},
    {0x3621,0x3f},
    {0x381a,0x1c},
};
#define NUM_OF_3M_SUB_INIT_REGS (sizeof(sensor_3M_sub_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_1080p_sub_init_regs[] = {
    {0x3703,0xe6},
    {0x3613,0x44},
    {0x370d,0x04},
    {0x3713,0x22},
    {0x3714,0x27},
    {0x3705,0xda},
    {0x370a,0x80},
    {0x3801,0x22},
    {0x3804,0x07},
    {0x3805,0x80},
    {0x3806,0x04},
    {0x3807,0x38},
    {0x3808,0x07},
    {0x3809,0x80},
    {0x380a,0x04},
    {0x380b,0x38},
    {0x380c,0x0a},
    {0x380d,0x6c},
    {0x380e,0x04},
    {0x380f,0x48},
    {0x381c,0x31},
    {0x381d,0xc2},
    {0x381e,0x04},
    {0x381f,0x58},
    {0x3820,0x05},
    {0x3821,0x18},
    {0x3824,0x22},//H-Start
    {0x3825,0x94},//H-Start
    {0x3a08,0x16},
    {0x3a09,0x48},
    {0x3a0a,0x12},
    {0x3a0b,0x90},
    {0x3a0d,0x03},
    {0x3a0e,0x03},
    {0x3815,0x82},
    {0x3501,0x7c},  //
    {0x3502,0x40},  //
    {0x350b,0x11},  //
    {0x401c,0x46},
    {0x3621,0x3f},
    {0x381a,0x1c},
};
#define NUM_OF_1080P_SUB_INIT_REGS (sizeof(sensor_1080p_sub_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_720p_sub_init_regs[] = {
    {0x3703,0x9a},
    {0x3613,0xc4},
    {0x370d,0x42},
    {0x3713,0x92},
    {0x3714,0x17},
    {0x3705,0xdb},
    {0x370a,0x81},
    {0x3801,0x54},
    {0x3804,0x05},
    {0x3805,0x00},
    {0x3806,0x02},
    {0x3807,0xd0},
    {0x3808,0x05},
    {0x3809,0x00},
    {0x380a,0x02},
    {0x380b,0xd0},
    {0x380c,0x0a},
    {0x380d,0x84},
    {0x380e,0x04},
    {0x380f,0xa4},
    {0x381c,0x10},
    {0x381d,0x82},
    {0x381e,0x05},
    {0x381f,0xc0},
    {0x3820,0x00},
    {0x3821,0x20},
    {0x3824,0x23},
    {0x3825,0x2c},
    {0x3a08,0x1b},
    {0x3a09,0xc0},
    {0x3a0a,0x17},
    {0x3a0b,0x20},
    {0x3a0d,0x01},
    {0x3a0e,0x01},
    {0x3815,0x81},
    {0x3501,0x10},  //
    {0x3502,0x00},  //
    {0x350b,0x10},  //
    {0x401c,0x42},
    {0x3621,0xaf},
    {0x381a,0x00}, //
};
#define NUM_OF_720P_SUB_INIT_REGS (sizeof(sensor_720p_sub_init_regs) / sizeof(sensor_reg_t))

typedef struct gain_set {
    u8  gain_mul2;
    u8  gain_step;
    u8  gain_substep;
    u32 total_gain; // UFIX 7.6
} gain_set_t;

static const gain_set_t gain_table[] = {
    {0,  0,  0,   64}, {0,  0,  1,   68}, {0,  0,  2,   72}, {0,  0,  3,   76},
    {0,  0,  4,   80}, {0,  0,  5,   84}, {0,  0,  6,   88}, {0,  0,  7,   92},
    {0,  0,  8,   96}, {0,  0,  9,  100}, {0,  0, 10,  104}, {0,  0, 11,  108},
    {0,  0, 12,  112}, {0,  0, 13,  116}, {0,  0, 14,  120}, {0,  0, 15,  124},
    {0,  1,  0,  128}, {0,  1,  1,  136}, {0,  1,  2,  144}, {0,  1,  3,  152},
    {0,  1,  4,  160}, {0,  1,  5,  168}, {0,  1,  6,  176}, {0,  1,  7,  184},
    {0,  1,  8,  192}, {0,  1,  9,  200}, {0,  1, 10,  208}, {0,  1, 11,  216},
    {0,  1, 12,  224}, {0,  1, 13,  232}, {0,  1, 14,  240}, {0,  1, 15,  248},
    {0,  3,  0,  256}, {0,  3,  1,  272}, {0,  3,  2,  288}, {0,  3,  3,  304},
    {0,  3,  4,  320}, {0,  3,  5,  336}, {0,  3,  6,  352}, {0,  3,  7,  368},
    {0,  3,  8,  384}, {0,  3,  9,  400}, {0,  3, 10,  416}, {0,  3, 11,  432},
    {0,  3, 12,  448}, {0,  3, 13,  464}, {0,  3, 14,  480}, {0,  3, 15,  496},
    {0,  7,  0,  512}, {0,  7,  1,  544}, {0,  7,  2,  576}, {0,  7,  3,  608},
    {0,  7,  4,  640}, {0,  7,  5,  672}, {0,  7,  6,  704}, {0,  7,  7,  736},
    {0,  7,  8,  768}, {0,  7,  9,  800}, {0,  7, 10,  832}, {0,  7, 11,  864},
    {0,  7, 12,  896}, {0,  7, 13,  928}, {0,  7, 14,  960}, {0,  7, 15,  992},
    {0, 15,  0, 1024}, {0, 15,  1, 1088}, {0, 15,  2, 1152}, {0, 15,  3, 1216},
    {0, 15,  4, 1280}, {0, 15,  5, 1344}, {0, 15,  6, 1408}, {0, 15,  7, 1472},
    {0, 15,  8, 1536}, {0, 15,  9, 1600}, {0, 15, 10, 1664}, {0, 15, 11, 1728},
    {0, 15, 12, 1792}, {0, 15, 13, 1856}, {0, 15, 14, 1920}, {0, 15, 15, 1984},
    {1, 15,  0, 2048}, {1, 15,  1, 2176}, {1, 15,  2, 2304}, {1, 15,  3, 2432},
    {1, 15,  4, 2560}, {1, 15,  5, 2688}, {1, 15,  6, 2816}, {1, 15,  7, 2944},
    {1, 15,  8, 3072}, {1, 15,  9, 3200}, {1, 15, 10, 3328}, {1, 15, 11, 3456},
    {1, 15, 12, 3584}, {1, 15, 13, 3712}, {1, 15, 14, 3840}, {1, 15, 15, 3968}

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
    u32 reg, pixclk;
    u8  reg_pre_div, reg_muldiv45, reg_divp, reg_divs;
    u8  pre_div, mul_45, div_125;

    read_reg(0x300F, &reg);
    reg_muldiv45 = reg & 0xFF;
    read_reg(0x3010, &reg);
    reg_divs = ((reg & 0xFF) >> 4) & 0xF;
    read_reg(0x3011, &reg);
    reg_divp = reg & 0x3F;
    read_reg(0x3012, &reg);
    reg_pre_div = reg & 0x7;

    switch (reg_pre_div) {
        case 0:
            pre_div = 8;    // 1 << 3
            break;
        case 1:
            pre_div = 12;   // 1.5 << 3
            break;
        case 2:
            pre_div = 16;   // 2 << 3
            break;
        case 3:
            pre_div = 20;   // 2.5 << 3
            break;
        case 4:
            pre_div = 24;   // 3 << 3
            break;
        case 5:
            pre_div = 32;   // 4 << 3
            break;
        case 6:
            pre_div = 48;   // 6 << 3
            break;
        case 7:
            pre_div = 64;   // 8 << 3
            break;
    }

    switch ((reg_muldiv45 >> 6) & 0x03) {
        case 0:
            mul_45 = 1;     // Bypass
            break;
        case 1:
            mul_45 = 1;     // Divide by 1
            break;
        case 2:
            mul_45 = 4;     // Divide by 4
            break;
        case 3:
            mul_45 = 5;     // Divide by 5
            break;
    }

    switch(reg_muldiv45 & 0x03) {
        case 0:
            div_125 = 4;    // Bypass
            break;
        case 1:
            div_125 = 4;    // Divide by 1
            break;
        case 2:
            div_125 = 8;    // Divide by 2
            break;
        case 3:
            div_125 = 10;   // Divide by 2.5
            break;
    }

    pixclk = (xclk << 3) / pre_div;
    pixclk = pixclk * mul_45;
    pixclk = pixclk * reg_divp;
    pixclk = (pixclk << 2) / div_125;
    pixclk = pixclk >> 2;

    isp_info("********OV5653 Pixel Clock %d*********\n", pixclk);
    return pixclk;
}

static void adjust_blanking(int fps)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h;
    u32 pixclk_tmp = 24000000;

    if (g_psensor->interface == IF_MIPI){
        if (g_psensor->xclk == 27000000){

            if ((g_psensor->img_win.width <= 1280) && (g_psensor->img_win.height <= 720)){
                if (fps > 60)
                    fps = 60;
                if (fps > 30){
                    write_reg(0x380c ,0x08);
                    write_reg(0x380d ,0x80);
                    pinfo->lines_max = 0x2e8;
                    pinfo->sw_lines_max = pinfo->lines_max*60/fps - 6;
                }
                else{
                    write_reg(0x380c ,0x0a);
                    write_reg(0x380d ,0x84);
                    pinfo->lines_max = 0x4a4;
                    pinfo->sw_lines_max = pinfo->lines_max*30/fps - 6;
                }
            }
            else if ((g_psensor->img_win.width <= 1920) && (g_psensor->img_win.height <= 1080)){
                if (fps > 30)
                    fps = 30;
                    pinfo->lines_max = 0x448;
                    pinfo->sw_lines_max = pinfo->lines_max*30/fps - 6;
            }
            else if ((g_psensor->img_win.width <= 2048) && (g_psensor->img_win.height <= 1536)){
                if (fps > 15)
                    fps = 15;
                    pinfo->lines_max = 0x610;
                    pinfo->sw_lines_max = pinfo->lines_max*15/fps - 6;
            }
            else{
                if (fps > 15)
                    fps = 15;
                pinfo->lines_max = 0x7b0;
                pinfo->sw_lines_max = pinfo->lines_max*15/fps - 6;
            }
            isp_info("lines_max=%d, sw_lines_max=%d\n", pinfo->lines_max, pinfo->sw_lines_max);
            write_reg(0x380f, ((pinfo->sw_lines_max+6) & 0xFF));
            write_reg(0x380e, (((pinfo->sw_lines_max+6) >> 8) & 0xFF));

            isp_info("adjust_blanking, fps=%d\n", fps);
        }
        else{
            isp_err("MIPI interface only support 27MHz input clock\n");
        }
    }
    else{
        if (g_psensor->xclk == 24000000){
            if(fps >= 15){
                write_reg(0x3011, 0x10);
                write_reg(0x3010, 0x10);
                pixclk_tmp = (g_psensor->xclk/8) * 4 * 8;//96MHz
            }
            else if((fps < 15) && (fps >= 6)){
                write_reg(0x3011, 0x10);
                write_reg(0x3010, 0x30);
                pixclk_tmp = (g_psensor->xclk/8) * 2 * 8;//48MHz
            }
            else{
                write_reg(0x3011, 0x10);
                write_reg(0x3010, 0x70);
                pixclk_tmp = (g_psensor->xclk/8) * 1 * 8;;//24MHz
            }
            isp_info("24M\n");
        }

        if (g_psensor->xclk == 27000000){
            if(fps >= 10){
                write_reg(0x3010, 0x10);
				write_reg(0x3011, 0x0D);
                pixclk_tmp = 87750000;//87.75MHz
            }
            else if((fps < 15) && (fps >= 6)){
                write_reg(0x3010, 0x30);
                write_reg(0x3011, 0x0D);
                pixclk_tmp = 43875000;//43.875MHz
            }
            else{
                write_reg(0x3010, 0x70);
				write_reg(0x3011, 0x0D);
                pixclk_tmp = 27000000;//27MHz
            }
            isp_info("27M\n");
        }

        if ((g_psensor->img_win.width <= 1280) && (g_psensor->img_win.height <= 720)) {
            if (fps > 30) {
                //(0x872 x 0x2e4)/pclk = 60fps
                write_reg(0x380c ,0x08);
                write_reg(0x380d ,0x72);
                write_reg(0x380e ,0x02);
                write_reg(0x380f ,0xe4);
            }
        }

        g_psensor->pclk = pixclk_tmp;

        //isp_info("g_psensor->pclk=%d,fps=%d,g_psensor->xclk=%d\n",g_psensor->pclk,fps,g_psensor->xclk);

        // pclk = vts * total_line * fps
        pinfo->htp = g_psensor->pclk / fps / (pinfo->sw_lines_max + 6);
        //isp_info("htp = %d, g_psensor->pclk= %d, min_exposure =%d, pinfo->sw_lines_max=%d\n", pinfo->htp, g_psensor->pclk, fps, pinfo->sw_lines_max);
        reg_h = (pinfo->htp >> 8) & 0xFF;
        reg_l = pinfo->htp & 0xFF;
        write_reg(0x380c, reg_h);
        write_reg(0x380d, reg_l);
        pinfo->t_row = (pinfo->htp * 10000) / (g_psensor->pclk / 1000);
        g_psensor->min_exp = (pinfo->t_row + 500) / 1000;
    }
}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h;
    u32 hts;

    read_reg(0x380C, &reg_h);
    read_reg(0x380D, &reg_l);
    hts = ((reg_h & 0x1F) << 8) | (reg_l & 0xFF);
    isp_info("Current HTS = %d\n", (u32)hts);
    pinfo->t_row = (hts * 10000) / (g_psensor->pclk / 1000);
    g_psensor->min_exp = (pinfo->t_row + 500) / 1000;

    read_reg(0x380E, &reg_h);
    read_reg(0x380F, &reg_l);
    pinfo->sw_lines_max = (((reg_h & 0xF) << 8) | (reg_l & 0xFF)) - 6;

    read_reg(0x350C, &reg_h);
    read_reg(0x350D, &reg_l);
    pinfo->prev_sw_res = ((reg_h & 0xFF) << 8) | (reg_l & 0xFF);

	isp_info("t_row=%d\n", pinfo->t_row);
}

static int set_exp(u32 exp);
static int set_gain(u32 gain);
static int set_mirror(int enable);
static int set_flip(int enable);

static int set_size(u32 width, u32 height)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0, i = 0;
    sensor_reg_t* pInitTable;
    u32 InitTable_Num;
    u32 reg_h, reg_l, reg_msb;
    u32 readtmp;


    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("%dx%d exceeds maximum resolution %dx%d!\n",
            width, height, SENSOR_MAX_WIDTH, SENSOR_MAX_HEIGHT);
        return -1;
    }

    g_psensor->img_win.width = width;

    g_psensor->img_win.height = height;

    if ((g_psensor->img_win.width <= 1280) && (g_psensor->img_win.height <= 720)){
        pInitTable = sensor_720p_sub_init_regs;
        InitTable_Num = NUM_OF_720P_SUB_INIT_REGS;
        if (g_psensor->fps > 60)
            g_psensor->fps = 60;
        isp_info("720P\n");
    }
    else if ((g_psensor->img_win.width <= 1920) && (g_psensor->img_win.height <= 1080)) {
        pInitTable = sensor_1080p_sub_init_regs;
        InitTable_Num = NUM_OF_1080P_SUB_INIT_REGS;
        if (g_psensor->fps > 30)
            g_psensor->fps = 30;
        isp_info("1080P\n");
    }
    else if ((g_psensor->img_win.width <= 2048) && (g_psensor->img_win.height <= 1536)) {
        pInitTable = sensor_3M_sub_init_regs;
        InitTable_Num = NUM_OF_3M_SUB_INIT_REGS;
        if (g_psensor->fps > 15)
            g_psensor->fps = 15;
        isp_info("3M\n");
    }
    else{
        pInitTable = sensor_5M_sub_init_regs;
        InitTable_Num = NUM_OF_5M_SUB_INIT_REGS;
        if (g_psensor->interface == IF_PARALLEL){
            if (g_psensor->fps > 13)
                g_psensor->fps = 13;
        }
        else{
            if (g_psensor->fps > 15)
                g_psensor->fps = 15;
        }
        isp_info("5M\n");
    }

    // write initial register value
    for (i=0; i<InitTable_Num; i++) {
        if (write_reg(pInitTable[i].addr, pInitTable[i].val) < 0) {
            isp_err("failed to initial %s!\n", SENSOR_NAME);
            return -EINVAL;
        }
    }

    if (g_psensor->interface == IF_MIPI){
        isp_info("Sensor Interface : MIPI\n");
        isp_info("Sensor Resolution : %d x %d\n", g_psensor->img_win.width, g_psensor->img_win.height);

        if ((g_psensor->img_win.width <= 1280) && (g_psensor->img_win.height <= 720)) {
            write_reg(0x3803, 0x0c);
            write_reg(0x3807, 0xd8);
            write_reg(0x380b, 0xd8);

            if (g_psensor->fps > 30){
                write_reg(0x380c ,0x08);
                write_reg(0x380d ,0x80);
                write_reg(0x380e ,0x02);
                write_reg(0x380f ,0xe8);
            }
        }
        else if ((g_psensor->img_win.width <= 1920) && (g_psensor->img_win.height <= 1080)) {
            write_reg(0x3803, 0x0c);
            write_reg(0x3807, 0x50);
            write_reg(0x380b, 0x50);
            write_reg(0x380c ,0x0b);
            write_reg(0x380d ,0x70);
            write_reg(0x380e ,0x04);
            write_reg(0x380f ,0x50);
            // adjust crop position
            write_reg(0x381d ,0x52);
            write_reg(0x3820 ,0x03);
            write_reg(0x3825 ,0x74);
        }
        else if ((g_psensor->img_win.width <= 2048) && (g_psensor->img_win.height <= 1536)) {
            write_reg(0x3803, 0x0c);
            write_reg(0x3807, 0x00);
            write_reg(0x380b, 0x00);
            // adjust crop position
            write_reg(0x381d ,0xde);
            write_reg(0x3820 ,0x04);
            write_reg(0x3825 ,0xac);
        }
        else{   //5M
            write_reg(0x3803, 0x0a);
            write_reg(0x3807, 0xa0);
            write_reg(0x380b, 0xa0);
            write_reg(0x381d ,0x0a);
            write_reg(0x3820 ,0x00);
            write_reg(0x3825 ,0xb4);
        }

        mdelay(300);      //wait register update

        read_reg(0x3818, &readtmp);
        if ((g_psensor->img_win.width <= 1280) && (g_psensor->img_win.height <= 720))
            write_reg(0x3818, readtmp | BIT0);
        else
            write_reg(0x3818, readtmp & (~BIT0));

        set_mirror(pinfo->mirror);
        set_flip(pinfo->flip);

        write_reg(0x3827, 0x0b);
    }
    else{
        read_reg(0x3502, &reg_l);
        read_reg(0x3501, &reg_h);
        read_reg(0x3500, &reg_msb);
        pinfo->prev_sw_reg = ((reg_msb & 0xF) << 16) | ((reg_h & 0xFF) << 8) | (reg_l & 0xFF);

        set_mirror(pinfo->mirror);
        set_flip(pinfo->flip);

        write_reg(0x3827, 0x0b);

        mdelay(300);      //wait register update
    }

    g_psensor->pclk = get_pclk(g_psensor->xclk);
    adjust_blanking(g_psensor->fps);
    calculate_t_row();

    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    if (g_psensor->interface == IF_MIPI){
        g_psensor->out_win.width = width;
        g_psensor->out_win.height = height;
    }
    else{
        g_psensor->out_win.width = SENSOR_MAX_WIDTH;
        g_psensor->out_win.height = SENSOR_MAX_HEIGHT;
    }
    g_psensor->img_win.x = 0;
    g_psensor->img_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;
    isp_info("set_size end %dx%d\n", width, height);

    pinfo->prev_sw_res = 0;
    pinfo->prev_sw_reg = 0;
    set_exp(g_psensor->curr_exp);
    set_gain(g_psensor->curr_gain);

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
        //isp_info("lines = %d\n",lines);
        g_psensor->curr_exp = (lines * pinfo->t_row + 500) / 1000;
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 lines, sw_res, tmp;

    lines = ((exp * 1000) + pinfo->t_row /2 ) / pinfo->t_row;
//    lines = ((lines>0x7ea)?0x7ea:lines);
    if (lines == 0)
        lines = 1;

    if (lines > pinfo->sw_lines_max)
        sw_res = lines - pinfo->sw_lines_max;
    else
        sw_res = 0;
    tmp = lines << 4;

    //isp_info("lines = %d, sw_res = %d\n",lines,sw_res);

    //write_reg(0x3212, 0x00);    // Enable Group0

    if (sw_res != pinfo->prev_sw_res) {
        write_reg(0x350C, (sw_res >> 8));
        write_reg(0x350D, (sw_res & 0xFF));
        pinfo->prev_sw_res = sw_res;
        //isp_info("sw_res = %d\n",sw_res);
    }

    if (tmp != pinfo->prev_sw_reg) {
        write_reg(0x3502, (tmp & 0xFF));
        write_reg(0x3501, (tmp >> 8) & 0xFF);
        write_reg(0x3500, (tmp >> 16) & 0xFF);
        pinfo->prev_sw_reg = tmp;
        //isp_info("tmpreg_exp = %d\n",tmp);
    }

    //write_reg(0x3212, 0x10);    // End Group0
    //write_reg(0x3212, 0xA0);    // Launch Group0

    pinfo->prev_sw_res = sw_res;

    g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;

    //isp_info("g_psensor->curr_exp = %d\n",g_psensor->curr_exp);

    return ret;
}

static u32 get_gain(void)
{
    u32 gain_base, reg;
    u8 m4, m5, m6, m7, mstep, mult2;

    if (g_psensor->curr_gain == 0) {
        read_reg(0x350A, &reg);
        mult2 = reg & 0x01;
        read_reg(0x350B, &reg);
        mstep = reg & 0xF;
        m7 = (reg >> 7) & 0x01;
        m6 = (reg >> 6) & 0x01;
        m5 = (reg >> 5) & 0x01;
        m4 = (reg >> 4) & 0x01;

        gain_base = ((((((u32)mult2+1) << m7) << m6) << m5) << m4) << GAIN_SHIFT;
        g_psensor->curr_gain = gain_base + ((gain_base*(u32)(mstep&0xF))>>4);
    }

    //isp_info("g_psensor->curr_gain = %d\n",g_psensor->curr_gain);
    return g_psensor->curr_gain;
}

static int set_gain(u32 gain)
{
    int ret = 0;
    int i = 0;
    u32 gain_base;
    u8 gain_mul2 = 0, gain_step = 0, gain_substep = 0;
    u8 m7, m6, m5, m4;

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
    gain_mul2 = gain_table[i-1].gain_mul2;
    gain_step = gain_table[i-1].gain_step & 0xF;
    gain_substep = gain_table[i-1].gain_substep;

   // write_reg(0x3212, 0x00);    // Enable Group0
    write_reg(0x350A, gain_mul2);
    write_reg(0x350B, ((u32)gain_step << 4) | gain_substep);
   // write_reg(0x3212, 0x10);    // End Group0
   // write_reg(0x3212, 0xA0);    // Launch Group0

    m7 = (gain_step >> 3) & 0x01;
    m6 = (gain_step >> 2) & 0x01;
    m5 = (gain_step >> 1) & 0x01;
    m4 = gain_step & 0x01;

    gain_base = (((((u32)(gain_mul2+1) << m7) << m6) << m5) << m4) << GAIN_SHIFT;
    g_psensor->curr_gain = gain_base + ((gain_base*(u32)(gain_substep&0xF)) >> 4);

    return ret;
}

static int get_mirror(void)
{
    u32 reg;

    read_reg(0x3818, &reg);
    return (reg & BIT6) ? 1 : 0;
}

static int set_mirror(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg, tmp;

    read_reg(0x3818, &reg);
    read_reg(0x3621, &tmp);

    if (enable) {
        write_reg(0x3818, (reg | BIT6));
        write_reg(0x3621, (tmp & 0xEF));
        write_reg(0x505a, 0x0a);
        write_reg(0x505b, 0x2E);
    } else {
        write_reg(0x3818, (reg &~BIT6));
        write_reg(0x3621, (tmp | 0x10));
        write_reg(0x505a, 0x00);
        write_reg(0x505b, 0x12);
    }
    pinfo->mirror = enable;

    return 0;
}

static int get_flip(void)
{
    u32 reg;

    read_reg(0x3818, &reg);
    return (reg & BIT5) ? 1 : 0;
}

static int set_flip(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    read_reg(0x3818, &reg);

    if (enable) {
       //read_reg(0x3827, &tmp);
       // write_reg(0x3827, (tmp | 0x01)); //odd
        write_reg(0x3818, (reg | BIT5));
    } else {
       //read_reg(0x3803, &tmp);
       //write_reg(0x3803, (tmp & 0xFE)); //even
        write_reg(0x3818, (reg &~BIT5));
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
    u32 reg_h, reg_l, reg_msb;
    sensor_reg_t* pInitTable;
    u32 InitTable_Num;

    if (pinfo->is_init)
        return 0;

        pInitTable = sensor_init_regs;
        InitTable_Num = NUM_OF_INIT_REGS;

    // write initial register value
            isp_info("init %s\n", SENSOR_NAME);
    for (i=0; i<InitTable_Num; i++) {
        if (write_reg(pInitTable[i].addr, pInitTable[i].val) < 0) {
            isp_err("failed to initial %s!\n", SENSOR_NAME);
            return -EINVAL;
        }
//        mdelay(1);
    }

    if (g_psensor->interface == IF_MIPI){
        isp_info("Sensor Interface : MIPI\n");
        isp_info("Sensor Resolution : %d x %d\n", g_psensor->img_win.width, g_psensor->img_win.height);
        write_reg(0x3011, 0x12);
        write_reg(0x3007, 0x3b);
        write_reg(0x3010, 0x10);
        write_reg(0x4801, 0x0f);
        write_reg(0x3003, 0x03);
        write_reg(0x300e, 0x0c);
        write_reg(0x4803, 0x50);
        write_reg(0x4800, 0x04);
        write_reg(0x300f, 0x8f);
        write_reg(0x3003, 0x01);
    }
    else{
        read_reg(0x3502, &reg_l);
    	 read_reg(0x3501, &reg_h);
	 read_reg(0x3500, &reg_msb);
    	 pinfo->prev_sw_reg = ((reg_msb & 0xF) << 16) | ((reg_h & 0xFF) << 8) | (reg_l & 0xFF);

        write_reg(0x3827, 0x0b);

        mdelay(300);      //wait register update
    }

    pinfo->mirror = mirror;
    pinfo->flip = flip;

    // set image size
    if ((ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height)) < 0)
        return ret;

    set_fps(g_psensor->fps);

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
static void ov5653_deconstruct(void)
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

static int ov5653_construct(u32 xclk, u16 width, u16 height)
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
    g_psensor->exp_latency = 1;
    g_psensor->gain_latency = 0;
    if ((width > 2048) && (height > 1536) && (fps > 13) && (g_psensor->interface == IF_PARALLEL))
        fps = 13;
    else if ((width > 1920) && (height > 1080) && (fps > 15))
        fps = 15;
    else if (width > 1280 && height > 720 && fps > 30)
        fps = 30;
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
    ov5653_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init ov5653_init(void)
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

    if ((ret = ov5653_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit ov5653_exit(void)
{
    isp_unregister_sensor(g_psensor);
    ov5653_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(ov5653_init);
module_exit(ov5653_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor OV5653");
MODULE_LICENSE("GPL");
