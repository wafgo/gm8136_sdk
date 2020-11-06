/**
 * @file sen_ov2715.c
 * OmniVision OV2715 sensor driver
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.16 $
 * $Date: 2014/10/29 07:45:23 $
 *
 * ChangeLog:
 *  $Log: sen_ov2715.c,v $
 *  Revision 1.16  2014/10/29 07:45:23  jtwang
 *  fixed MIPI mode 15 fps fail issue
 *
 *  Revision 1.15  2014/09/05 10:12:52  jason_ha
 *  Add msleep fix mirror/flip bug
 *
 *  Revision 1.14  2014/08/04 06:30:41  jtwang
 *  *** empty log message ***
 *
 *  Revision 1.13  2014/07/28 01:14:14  jtwang
 *  modify y start offset for flip
 *
 *  Revision 1.12  2014/07/25 08:38:32  ricktsao
 *  no message
 *
 *  Revision 1.11  2014/07/17 05:37:40  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.10  2014/04/24 06:31:37  jtwang
 *  1. fix black image when enable both mirror and flip at sensor init
 *  2. add MIPI mode
 *
 *  Revision 1.9  2013/11/28 08:43:18  jason_ha
 *  *** empty log message ***
 *
 *  Revision 1.8  2013/11/07 12:22:53  ricktsao
 *  no message
 *
 *  Revision 1.7  2013/10/28 05:43:14  ricktsao
 *  no message
 *
 *  Revision 1.6  2013/10/26 03:31:15  ricktsao
 *  no message
 *
 *  Revision 1.5  2013/10/25 10:32:25  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.4  2013/10/23 10:31:04  ricktsao
 *  add CAP_RST and ISP SPI api functions
 *
 *  Revision 1.3  2013/10/23 06:41:30  ricktsao
 *  add isp_plat_extclk_set_freq() and isp_plat_extclk_onoff()
 *
 *  Revision 1.2  2013/10/21 10:27:48  ricktsao
 *  no message
 *
 *  Revision 1.1  2013/09/26 02:06:49  ricktsao
 *  no message
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
#include "isp_input_inf.h"
#include "isp_api.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1920
#define DEFAULT_IMAGE_HEIGHT    1080
#define DEFAULT_XCLK            27000000
#define DEFAULT_INTERFACE       IF_PARALLEL

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

static uint fps = 30;
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "sensor frame rate");

static uint inv_clk = 0;
module_param(inv_clk, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "invert clock, 0:Disable, 1: Enable");

static ushort interface = DEFAULT_INTERFACE;
module_param( interface, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(interface, "interface type");


//=============================================================================
// global
//=============================================================================
#define SENSOR_NAME         "OV2715"
#define SENSOR_MAX_WIDTH    1920
#define SENSOR_MAX_HEIGHT   1088

static sensor_dev_t *g_psensor = NULL;

typedef struct sensor_info {
    bool is_init;
    u32 img_w;
    u32 img_h;
    int mirror;
    int flip;
    u32 sw_lines_max;
    u32 prev_sw_res;
    u32 prev_sw_reg;
    u32 t_row;          // unit is 1/10 us
} sensor_info_t;

typedef struct sensor_reg {
    u16 addr;
    u8  val;
} sensor_reg_t;

static sensor_reg_t sensor_init_regs[] = {
    {0x3103, 0x93},
    {0x3008, 0x82},
    {0x3017, 0x7f},
    {0x3018, 0xfc},
    {0x3706, 0x61},
    {0x3712, 0x0c},
    {0x3630, 0x6d},
    {0x3801, 0xb4},
    {0x3621, 0x04},
    {0x3604, 0x60},
    {0x3603, 0xa7},
    {0x3631, 0x26},
    {0x3600, 0x04},
    {0x3620, 0x37},
    {0x3623, 0x00},
    {0x3702, 0x9e},
    {0x3703, 0x74},
    {0x3704, 0x10},
    {0x370d, 0x0f},
    {0x3713, 0x8b},
    {0x3714, 0x74},
    {0x3710, 0x9e},
    {0x3801, 0xc4},
    {0x3803, 0x09},
    {0x3605, 0x05},
    {0x3606, 0x12},
    {0x302d, 0x90},
    {0x370b, 0x40},
    {0x3716, 0x31},
    {0x380d, 0x74},
    {0x5181, 0x20},
    {0x518f, 0x00},
    {0x4301, 0xff},
    {0x4303, 0x00},
    {0x3a00, 0x78},
    {0x300f, 0x88},
    {0x3011, 0x28},
    {0x3a1a, 0x06},
    {0x3a18, 0x00},
    {0x3a19, 0x7a},
    {0x3a13, 0x54},
    {0x382e, 0x0f},
    {0x381a, 0x1a},
    {0x5688, 0x03},
    {0x5684, 0x07},
    {0x5685, 0xa0},
    {0x5686, 0x04},
    {0x5687, 0x43},
    {0x3a0f, 0x40},
    {0x3a10, 0x38},
    {0x3a1b, 0x48},
    {0x3a1e, 0x30},
    {0x3a11, 0x90},
    {0x3a1f, 0x10},
    {0x3010, 0x00},
    {0x382e, 0x0f},
    {0x381a, 0x1a},
    {0x3621, 0x14},
    {0x3818, 0xe0},
    {0x3503, 0x07},
    {0x3406, 0x01},
    {0x4000, 0x01},
    {0x5000, 0x59},
    {0x5001, 0x4E},
    {0x5002, 0xE0},
    {0x4708, 0x00},
    {0x350C, 0x00},
    {0x350D, 0x00},

    // initial exposure time
    {0x3500, 0x00},
    {0x3501, 0x00},
    {0x3502, 0x10},
    // initial gain
    {0x350A, 0x00},
    {0x350B, 0x00},
};
#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_mipi_init_regs[] = {
    //MIPI
    {0x3103, 0x93},
    {0x3008, 0x82},
    {0x3017, 0x7f},
    {0x3018, 0xfc},
    {0x3706, 0x61},
    {0x3712, 0x0c},
    {0x3630, 0x6d},
    {0x3801, 0xb4},
    {0x3621, 0x04},
    {0x3604, 0x60},
    {0x3603, 0xa7},
    {0x3631, 0x26},
    {0x3600, 0x04},
    {0x3620, 0x37},
    {0x3623, 0x00},
    {0x3702, 0x9e},
    {0x3703, 0x5c},
    {0x3704, 0x40},
    {0x370d, 0x0f},
    {0x3713, 0x9f},
    {0x3714, 0x4c},
    {0x3710, 0x9e},
    {0x3801, 0xc4},
    {0x3605, 0x05},
    {0x3606, 0x3f},
    {0x302d, 0x90},
    {0x370b, 0x40},
    {0x3716, 0x31},
    {0x3707, 0x52},
	{0x380d, 0x74},
    {0x5181, 0x20},
    {0x518f, 0x00},
    {0x4301, 0xff},
    {0x4303, 0x00},
    {0x3a00, 0x78},
    {0x300f, 0x88},
    {0x3011, 0x28},
    {0x3a1a, 0x06},
    {0x3a18, 0x00},
    {0x3a19, 0x7a},
    {0x3a13, 0x54},
    {0x382e, 0x0f},
    {0x381a, 0x1a},
    {0x401d, 0x02},
    {0x5688, 0x03},
    {0x5684, 0x07},
    {0x5685, 0xa0},
    {0x5686, 0x04},
    {0x5687, 0x43},
    {0x3011, 0x0a},
    {0x300f, 0x8a},
    {0x3017, 0x00},
    {0x3018, 0x00},
    {0x300e, 0x04},
    {0x4801, 0x0f},
    {0x300f, 0xc3},
    {0x3a0f, 0x40},
    {0x3a10, 0x38},
    {0x3a1b, 0x48},
    {0x3a1e, 0x30},
    {0x3a11, 0x90},
    {0x3a1f, 0x10},

    {0x3820, 0x00},
    {0x3821, 0x00},
    {0x381c, 0x00},
    {0x381d, 0x02},
    {0x381e, 0x04},
    {0x381f, 0x44},
    {0x3800, 0x01},
    {0x3801, 0xC4},
    {0x3804, 0x07},
    {0x3805, 0x88},
    {0x3802, 0x00},
    {0x3803, 0x0A},
    {0x3806, 0x04},
    {0x3807, 0x40},
    {0x3808, 0x07},
    {0x3809, 0x88},
    {0x380a, 0x04},
    {0x380b, 0x40},
    {0x380c, 0x0a},
    {0x380d, 0x9d},
    {0x380e, 0x04},
    {0x380f, 0x50},
    {0x3810, 0x08},
    {0x3811, 0x02},

    {0x3621, 0x04},
    {0x3622, 0x08},
    {0x3818, 0x80},

    //AVG windows
    {0x5688, 0x03},
    {0x5684, 0x07},
    {0x5685, 0x88},
    {0x5686, 0x04},
    {0x5687, 0x40},

    {0x3a08, 0x14},
    {0x3a09, 0xB3},
    {0x3a0a, 0x11},
    {0x3a0b, 0x40},
    {0x3a0e, 0x03},
    {0x3a0d, 0x04},

    {0x401c, 0x08},

    // off 3A
    {0x3503, 0x07}, // off aec/agc
    {0x5001, 0x4e}, // off awb
    {0x5000, 0x5f}, // off lenc
};
#define NUM_OF_MIPI_INIT_REGS (sizeof(sensor_mipi_init_regs) / sizeof(sensor_reg_t))

typedef struct gain_set {
    u8  mult2;
    u8  step;
    u8  substep;
    u16 total_gain;     // UFIX 7.6
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
    u32 reg, pclk;
    u8  pre_div, mult45, div1to2p5, divp, divs;
    const u8 pre_div_lut[8] = {8, 12, 16, 20, 24, 32, 48, 64}; // precision is UFIX5.3
    const u8 mult45_lut[4] = {1, 1, 4, 5};
    const u8 div1to2p5_lut[4] = {4, 4, 8, 10}; // recision is UFIX6.2

    read_reg(0x3012, &reg);
    pre_div = pre_div_lut[reg & 0x7];

    read_reg(0x300f, &reg);
    mult45 = mult45_lut[reg & 0x3];
    div1to2p5 = div1to2p5_lut[(reg >> 6) & 0x3];

    read_reg(0x3011, &reg);
    divp = reg & 0x3F;

    read_reg(0x3010, &reg);
    divs = ((reg >> 4) & 0xF) + 1;

    pclk = ((((((xclk << 3) / pre_div) * mult45 * divp) / divs) << 2) / div1to2p5) >> 2;
    printk("pixel clock %d\n", pclk);
    return pclk;
}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h, reg_msb;
    u32 hts;

    read_reg(0x380C, &reg_h);
    read_reg(0x380D, &reg_l);
    hts = ((reg_h & 0xF) << 8) | (reg_l & 0xFF);
    pinfo->t_row = (hts * 10000) / (g_psensor->pclk / 1000);
    isp_info("hts=%d, t_row=%d\n", hts, pinfo->t_row);
    g_psensor->min_exp = (pinfo->t_row + 999) / 1000;

    read_reg(0x380E, &reg_h);
    read_reg(0x380F, &reg_l);
    pinfo->sw_lines_max = (((reg_h & 0xF) << 8) | (reg_l & 0xFF)) - 6;

    read_reg(0x350C, &reg_h);
    read_reg(0x350D, &reg_l);
    pinfo->prev_sw_res = ((reg_h & 0xFF) << 8) | (reg_l & 0xFF);

    read_reg(0x3502, &reg_l);
    read_reg(0x3501, &reg_h);
    read_reg(0x3500, &reg_msb);
    pinfo->prev_sw_reg = ((reg_msb & 0xFF) << 16) | ((reg_h & 0xFF) << 8) | (reg_l & 0xFF);
}

static void adjust_blk(int fps)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h;
    int htp;

    if (g_psensor->interface != IF_MIPI){
        // adjust PCLK
        if (g_psensor->xclk == 24000000) {

            if (fps >= 20) {
                write_reg(0x3011, 0x28);
                write_reg(0x3010, 0x00);
                g_psensor->pclk = (g_psensor->xclk/15) * 1 * 40 * 1 / 2 / 4 * 10;   //80MHz
            }
            else if ((fps < 20) && (fps >= 10)) {
                write_reg(0x3011, 0x28);
                write_reg(0x3010, 0x10);
                g_psensor->pclk = (g_psensor->xclk/15/2) * 1 * 40 * 1 / 2 / 4 * 10;  //40MHz
            }
            else {
                write_reg(0x3011, 0x28);
                write_reg(0x3010, 0x30);
                g_psensor->pclk = (g_psensor->xclk/15/4) * 1 * 40 * 1 / 2 / 4 * 10;  //20MHz
            }
        } else if (g_psensor->xclk == 27000000) {
            if (fps >= 20) {
                write_reg(0x3011, 0x24);
                write_reg(0x3010, 0x00);
                g_psensor->pclk = (g_psensor->xclk/15) * 1 * 36 * 1 / 2 / 4 * 10;    //81MHz
            }
            else if((fps < 20) && (fps >= 10)) {
                write_reg(0x3011, 0x24);
                write_reg(0x3010, 0x10);
                g_psensor->pclk = (g_psensor->xclk/15/2) * 1 * 36 * 1 / 2 / 4 * 10;  //40.5MHz
            }
            else {
                write_reg(0x3011, 0x24);
                write_reg(0x3010, 0x30);
                g_psensor->pclk = (g_psensor->xclk/15/4) * 1 * 36 * 1 / 2 / 4 * 10;  //20.25MHz
            }
        }
        // adjust blanking
        read_reg(0x380E, &reg_h);
        read_reg(0x380F, &reg_l);
        pinfo->sw_lines_max = (((reg_h & 0xF) << 8) | (reg_l & 0xFF)) - 6;

        // pclk = vts * total_line * fps
        htp = g_psensor->pclk / fps / (pinfo->sw_lines_max + 6);
        reg_h = (htp >> 8) & 0xFF;
        reg_l = htp & 0xFF;
        write_reg(0x380C, reg_h);
        write_reg(0x380D, reg_l);
        //printk("htp = %d, pdev->pclk= %d, min_exposure =%d, pinfo->sw_lines_max=%d\n",
        //        htp, g_psensor->pclk, fps, pinfo->sw_lines_max);

    }
    else{
        reg_l = 1104 * 30 / fps; 
        write_reg(0x380e, reg_l >> 8);
        write_reg(0x380f, reg_l & 0xff);
        isp_info("adjust_blk : %d fps\n", fps);
    }

    calculate_t_row();
    g_psensor->fps = fps;
}

static int set_size(u32 width, u32 height)
{
    int ret = 0;
    u32 reg_l, reg_h;

    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("%dx%d exceeds maximum resolution %dx%d!\n",
            width, height, SENSOR_MAX_WIDTH, SENSOR_MAX_HEIGHT);
        return -1;
    }

    if (g_psensor->interface != IF_MIPI){
        if ((width <= 1280) && (height <= 720)) {
            // sensor output 1288 x 728
            write_reg(0x3800, 0x01);    // horizontal start H
            write_reg(0x3801, 0xc8);    // horizontal start L
            write_reg(0x3802, 0x00);    // vertical start H
            write_reg(0x3803, 0x09);    // vertical start L
            write_reg(0x3804, 0x05);    // horizontal width H
            write_reg(0x3805, 0x08);    // horizontal width L
            write_reg(0x3806, 0x02);    // vertical height H
            write_reg(0x3807, 0xd8);    // vertical height L
            write_reg(0x3808, 0x05);    // DVP horizontal width H
            write_reg(0x3809, 0x08);    // DVP horizontal width L
            write_reg(0x380a, 0x02);    // DVP vertical height H
            write_reg(0x380b, 0xd8);    // DVP vertical height L
            write_reg(0x380c, 0x07);    // total horizontal size H
            write_reg(0x380d, 0x04);    // total horizontal size L
            write_reg(0x380e, 0x02);    // total vertical size H
            write_reg(0x380f, 0xe8);    // total vertical size L
            write_reg(0x3810, 0x08);    // [4:0]horizontal offset min=8
            write_reg(0x3811, 0x02);    // [4:0]vertical offset min=2
            write_reg(0x381c, 0x10);    // [3:0] crop vertical start H
            write_reg(0x381d, 0xbe);    // crop vertical start L (must be even)
            write_reg(0x381e, 0x02);    // [3:0] crop vertical size H
            write_reg(0x381f, 0xdc);    // crop vertical size L
            write_reg(0x3820, 0x09);    // crop horizontal start
            write_reg(0x3821, 0x29);    // crop horizontal size
            write_reg(0x5684, 0x05);    // AVG vertical start position H
            write_reg(0x5685, 0x02);    // AVG vertical start position L
            write_reg(0x5686, 0x02);    // AVG vertical end position H
            write_reg(0x5687, 0xd2);    // AVG vertical end position L
        } else {
            // sensor output 1928 x 1088
            write_reg(0x3800, 0x01);    // horizontal start H
            write_reg(0x3801, 0xc4);    // horizontal start L
            write_reg(0x3802, 0x00);    // vertical start H
            write_reg(0x3803, 0x09);    // vertical start L
            write_reg(0x3804, 0x07);    // horizontal width H
            write_reg(0x3805, 0x88);    // horizontal width L
            write_reg(0x3806, 0x04);    // vertical height H
            write_reg(0x3807, 0x40);    // vertical height L
            write_reg(0x3808, 0x07);    // DVP horizontal width H
            write_reg(0x3809, 0x88);    // DVP horizontal width L
            write_reg(0x380a, 0x04);    // DVP vertical height H
            write_reg(0x380b, 0x40);    // DVP vertical height L
            write_reg(0x380c, 0x09);    // total horizontal size H
            write_reg(0x380d, 0xa4);    // total horizontal size L
            write_reg(0x380e, 0x04);    // total vertical size H
            write_reg(0x380f, 0x50);    // total vertical size L
            write_reg(0x3810, 0x08);    // [4:0]horizontal offset min=8
            write_reg(0x3811, 0x02);    // [4:0]vertical offset min=2
            write_reg(0x381c, 0x00);    // [3:0] crop vertical start H
            write_reg(0x381d, 0x02);    // crop vertical start L (must be even)
            write_reg(0x381e, 0x04);    // [3:0] crop vertical size H
            write_reg(0x381f, 0x44);    // crop vertical size L
            write_reg(0x3820, 0x00);    // crop horizontal start
            write_reg(0x3821, 0x00);    // crop horizontal size
            write_reg(0x5684, 0x07);    // AVG vertical start position H
            write_reg(0x5685, 0x88);    // AVG vertical start position L
            write_reg(0x5686, 0x04);    // AVG vertical end position H
            write_reg(0x5687, 0x40);    // AVG vertical end position L
        }
    }

    adjust_blk(g_psensor->fps);

    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    read_reg(0x3804, &reg_h);
    read_reg(0x3805, &reg_l);
    g_psensor->out_win.width = ((reg_h & 0xF) << 8) | (reg_l & 0xFF);;
    read_reg(0x380a, &reg_h);
    read_reg(0x380b, &reg_l);
    g_psensor->out_win.height = ((reg_h & 0xF) << 8) | (reg_l & 0xFF);;

    g_psensor->img_win.x = 0;
    g_psensor->img_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    return ret;
}

static u32 get_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 lines, sw_t, sw_m, sw_b;

    if (!g_psensor->curr_exp) {
        read_reg(0x3500, &sw_t);
        read_reg(0x3501, &sw_m);
        read_reg(0x3502, &sw_b);
        lines = ((sw_t & 0xF) << 12)|((sw_m & 0xFF) << 4)|((sw_b & 0xFF) >> 4);
        g_psensor->curr_exp = (lines * pinfo->t_row + 500) / 1000;
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 lines, sw_res, tmp;

    lines = (exp * 1000) / pinfo->t_row;
    if (lines == 0)
        lines = 1;

    if (lines > pinfo->sw_lines_max) {
        if (lines <= (pinfo->sw_lines_max + 6)) {
            sw_res = 0;
            lines = pinfo->sw_lines_max;
        } else {
            sw_res = lines - pinfo->sw_lines_max;//6
        }
    } else {
        sw_res = 0;
    }
    tmp = lines << 4;

    if (sw_res != pinfo->prev_sw_res) {
        write_reg(0x350C, (sw_res >> 8));
        write_reg(0x350D, (sw_res & 0xFF));
        pinfo->prev_sw_res = sw_res;
    }

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
        g_psensor->curr_gain = gain_base + ((gain_base * mstep) >> 4);
    }

    return g_psensor->curr_gain;
}

static int set_gain(u32 gain)
{
    u32 gain_base;
    u8 mult2 = 0, step = 0, substep = 0;
    u8 m7, m6, m5, m4;
    int ret = 0, i;

    // search most suitable gain into gain table
    if (gain > gain_table[NUM_OF_GAINSET - 1].total_gain)
        gain = gain_table[NUM_OF_GAINSET - 1].total_gain;
    else if (gain < gain_table[0].total_gain)
        gain = gain_table[0].total_gain;

    for (i=0; i<NUM_OF_GAINSET; i++) {
        if (gain_table[i].total_gain > gain)
            break;
    }
    mult2 = gain_table[i-1].mult2;
    step = gain_table[i-1].step & 0xF;
    substep = gain_table[i-1].substep;

    write_reg(0x3212, 0x00);    // Enable Group0
    write_reg(0x350A, mult2);
    write_reg(0x350B, ((u32)step << 4) | substep);
    write_reg(0x3212, 0x10);    // End Group0
    write_reg(0x3212, 0xA0);    // Launch Group0

    m7 = (step >> 3) & 0x01;
    m6 = (step >> 2) & 0x01;
    m5 = (step >> 1) & 0x01;
    m4 = step & 0x01;

    gain_base = ((((((u32)mult2+1) << m7) << m6) << m5) << m4) << GAIN_SHIFT;
    g_psensor->curr_gain = gain_base + ((gain_base * substep) >> 4);
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
    u32 reg;

    read_reg(0x3818, &reg);

    write_reg(0x3212, 0x01);    // Enable Group1
    if (pinfo->flip){
            write_reg(0x3803, 0x9);
        reg |= BIT5;
    }
    else{
        reg &=~BIT5;
        write_reg(0x3803, 0xa);
    }

    if (enable) {
        write_reg(0x3621, 0x14);
        write_reg(0x3818, (reg | BIT6));
    } else {
        write_reg(0x3621, 0x04);
        write_reg(0x3818, (reg &~BIT6));
    }
    write_reg(0x3212, 0x11);    // End Group1
    write_reg(0x3212, 0xA1);    // Launch Group1
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

    write_reg(0x3212, 0x02);    // Enable Group2

    if (pinfo->mirror){
        reg |= BIT6;
        write_reg(0x3621, 0x14);
    }
    else{
        reg &=~BIT6;
        write_reg(0x3621, 0x04);
    }

    if (enable) {
            write_reg(0x3803, 0x9);
        write_reg(0x3818, (reg | BIT5));
    } else {
        write_reg(0x3803, 0xa);
        write_reg(0x3818, (reg &~BIT5));
    }
    write_reg(0x3212, 0x12);    // End Group2
    write_reg(0x3212, 0xA2);    // Launch Group2

    pinfo->flip = enable;

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
        adjust_blk((int)arg);
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

    if (pinfo->is_init)
        return 0;

    if (g_psensor->interface != IF_MIPI){
        isp_info("Sensor Interface : PARALLEL\n");
       // write initial register value
        for (i=0; i<NUM_OF_INIT_REGS; i++) {
            if (write_reg(sensor_init_regs[i].addr, sensor_init_regs[i].val) < 0) {
                isp_err("failed to initial %s!\n", SENSOR_NAME);
                return -EINVAL;
            }
            mdelay(1);
        }
    }
    else{
        isp_info("Sensor Interface : MIPI\n");
        // write initial register value
        for (i=0; i<NUM_OF_MIPI_INIT_REGS; i++) {
            if (write_reg(sensor_mipi_init_regs[i].addr, sensor_mipi_init_regs[i].val) < 0) {
                isp_err("failed to initial %s!\n", SENSOR_NAME);
                return -EINVAL;
            }
//            mdelay(1);
        }
    }
    if (g_psensor->interface != IF_MIPI){
        if (g_psensor->xclk == 27000000) {
            write_reg(0x3011, 0x24);
            write_reg(0x380c, 0x09);
            write_reg(0x380d, 0x8d);
            msleep(10);
        }
    }

    if (g_psensor->interface == IF_MIPI){

        if (g_psensor->xclk != 27000000)
            isp_err("MIPI Interface only support 27MHz input clock\n");
    }
    // get pixel clock
    g_psensor->pclk = get_pclk(g_psensor->xclk);

    // set image size
    if ((ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height)) < 0)
        return ret;

    // set mirror and flip
    set_mirror(mirror);
    set_flip(flip);
    
    msleep(50);
    
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
static void ov2715_deconstruct(void)
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

static int ov2715_construct(u32 xclk, u16 width, u16 height)
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
    g_psensor->num_of_lane = 1;
    g_psensor->protocol = 0;
    if (g_psensor->interface == IF_MIPI)
        g_psensor->fmt = RAW10;
    else
        g_psensor->fmt = RAW12;
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

    if (g_psensor->interface == IF_MIPI){
	    // no frame if init this sensor after mipi inited
    	if ((ret = init()) < 0)
        	goto construct_err;
	}

    return 0;

construct_err:
    ov2715_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init ov2715_init(void)
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

    if ((ret = ov2715_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit ov2715_exit(void)
{
    isp_unregister_sensor(g_psensor);
    ov2715_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(ov2715_init);
module_exit(ov2715_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor OV2715");
MODULE_LICENSE("GPL");
