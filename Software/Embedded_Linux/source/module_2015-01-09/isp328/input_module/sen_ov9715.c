/**
 * @file sen_ov9715.c
 * OmniVision OV9710/OV9712/OV9715 sensor driver
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

static uint fps = 30;
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "sensor frame rate");

static uint inv_clk = 0;
module_param(inv_clk, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "invert clock, 0:Disable, 1: Enable");

//=============================================================================
// global
//=============================================================================
#define SENSOR_NAME         "OV9715"
#define SENSOR_MAX_WIDTH    1280
#define SENSOR_MAX_HEIGHT   800

static sensor_dev_t *g_psensor = NULL;

typedef struct sensor_info {
    bool is_init;
    int mirror;
    int flip;
    u32 sw_lines_max;
    int htp;
    u32 t_row;          // unit is 1/10 us
    u32 curr_exp_us;
} sensor_info_t;

typedef struct sensor_reg {
    u8 addr;
    u8 val;
} sensor_reg_t;

static sensor_reg_t sensor_init_regs[] = {
    {0x12, 0x80},   // reset

    // core Settings
    {0x1e, 0x07},
    {0x5f, 0x18},
    {0x69, 0x04},
    {0x65, 0x2a},
    {0x68, 0x0a},
    {0x39, 0x28},
    {0x4d, 0x90},
    {0xc1, 0x80},
    {0x0c, 0x30},
    {0x6d, 0x02},

    // DSP
    {0x96, 0x21},   //[5]ebabke AWB gain

    // Resolution and Format
    {0x12, 0x00},   // reset end
    {0x3b, 0x00},
    {0x97, 0x80},
    {0x17, 0x25},
    {0x18, 0xA2},
    {0x19, 0x00},
    {0x1a, 0xCA},
    {0x03, 0x0B},
    {0x32, 0x07},
    {0x98, 0x00},
    {0x99, 0x00},
    {0x9a, 0x00},
    {0x57, 0x00},
    {0x58, 0xC8},
    {0x59, 0xa0},
    {0x4c, 0x13},
    {0x4b, 0x36},
    {0x3d, 0x3c},
    {0x3e, 0x03},
    {0xbd, 0xa0},
    {0xbe, 0xc8},
    {0x04, 0xC8},

    // clock
    {0x5c, 0x52},
    {0x5d, 0x00},
    {0x11, 0x01},
    {0x2a, 0x98},
    {0x2b, 0x06},
    {0x2d, 0x00},
    {0x2e, 0x00},
    {0x63, 0x04},
    // General
    {0x13, 0x00},
    {0x38, 0x00},
    {0xb6, 0x08},
};
#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))

typedef struct gain_set {
    u8  mult2;
    u8  step;
    u8  awb_gain;
    u16 total_gain;     // UFIX 7.6
} gain_set_t;

static const gain_set_t gain_table[] = {
    { 0,  0,  64,   64}, { 0,  1,  64,   68}, { 0,  2,  64,   72}, { 0,  3,  64,   76}, { 0,  4,  64,   80},
    { 0,  5,  64,   84}, { 0,  6,  64,   88}, { 0,  7,  64,   92}, { 0,  8,  64,   96}, { 0,  9,  64,  100},
    { 0, 10,  64,  104}, { 0, 11,  64,  108}, { 0, 12,  64,  112}, { 0, 13,  64,  116}, { 0, 14,  64,  120},
    { 0, 15,  64,  124}, { 1,  0,  64,  128}, { 1,  1,  64,  136}, { 1,  2,  64,  144}, { 1,  3,  64,  152},
    { 1,  4,  64,  160}, { 1,  5,  64,  168}, { 1,  6,  64,  176}, { 1,  7,  64,  184}, { 1,  8,  64,  192},
    { 1,  9,  64,  200}, { 1, 10,  64,  208}, { 1, 11,  64,  216}, { 1, 12,  64,  224}, { 1, 13,  64,  232},
    { 1, 14,  64,  240}, { 1, 15,  64,  248}, { 3,  0,  64,  256}, { 3,  1,  64,  272}, { 3,  2,  64,  288},
    { 3,  3,  64,  304}, { 3,  4,  64,  320}, { 3,  5,  64,  336}, { 3,  6,  64,  352}, { 3,  7,  64,  368},
    { 3,  8,  64,  384}, { 3,  9,  64,  400}, { 3, 10,  64,  416}, { 3, 11,  64,  432}, { 3, 12,  64,  448},
    { 3, 13,  64,  464}, { 3, 14,  64,  480}, { 3, 15,  64,  496}, { 7,  0,  64,  512}, { 7,  1,  64,  544},
    { 7,  2,  64,  576}, { 7,  3,  64,  608}, { 7,  4,  64,  640}, { 7,  5,  64,  672}, { 7,  6,  64,  704},
    { 7,  7,  64,  736}, { 7,  8,  64,  768}, { 7,  9,  64,  800}, { 7, 10,  64,  832}, { 7, 11,  64,  864},
    { 7, 12,  64,  896}, { 7, 13,  64,  928}, { 7, 14,  64,  960}, { 7, 15,  64,  992}, {15,  0,  64, 1024},
    {15,  1,  64, 1088}, {15,  2,  64, 1152}, {15,  3,  64, 1216}, {15,  4,  64, 1280}, {15,  5,  64, 1344},
    {15,  6,  64, 1408}, {15,  7,  64, 1472}, {15,  8,  64, 1536}, {15,  9,  64, 1600}, {15, 10,  64, 1664},
    {15, 11,  64, 1728}, {15, 12,  64, 1792}, {15, 13,  64, 1856}, {15, 14,  64, 1920}, {15, 15,  64, 1984},
/*
    {15, 15,  68, 2108}, {15, 15,  72, 2232}, {15, 15,  76, 2356}, {15, 15,  80, 2480}, {15, 15,  84, 2604},
    {15, 15,  88, 2728}, {15, 15,  92, 2852}, {15, 15,  96, 2976}, {15, 15, 100, 3100}, {15, 15, 104, 3224},
    {15, 15, 108, 3348}, {15, 15, 112, 3472}, {15, 15, 116, 3596}, {15, 15, 120, 3720}, {15, 15, 124, 3844},
    {15, 15, 128, 3968}, {15, 15, 132, 4092}, {15, 15, 136, 4216}, {15, 15, 140, 4340}, {15, 15, 144, 4464},
    {15, 15, 148, 4588}, {15, 15, 152, 4712}, {15, 15, 156, 4836}, {15, 15, 160, 4960}, {15, 15, 164, 5084},
    {15, 15, 168, 5208}, {15, 15, 172, 5332}, {15, 15, 176, 5456}, {15, 15, 180, 5580}, {15, 15, 184, 5704},
    {15, 15, 188, 5828}, {15, 15, 192, 5952}, {15, 15, 196, 6076}, {15, 15, 200, 6200}, {15, 15, 204, 6324},
    {15, 15, 208, 6448}, {15, 15, 212, 6572}, {15, 15, 216, 6696}, {15, 15, 220, 6820}, {15, 15, 224, 6944},
    {15, 15, 228, 7068}, {15, 15, 232, 7192}, {15, 15, 236, 7316}, {15, 15, 240, 7440}, {15, 15, 244, 7564},
    {15, 15, 248, 7688}, {15, 15, 252, 7812}, {15, 15, 255, 7905},
*/    
};
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

//=============================================================================
// I2C
//=============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            (0x60 >> 1)
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
    u32 reg, pre_div, pll_m, pll_div, sys_div, pclk;
    bool pll_bypass;

    read_reg(0x5D, &reg);
    pll_bypass = reg & (1 << 6);

    if (pll_bypass) {
        read_reg(0x5C, &reg);
        reg = (reg >> 5) & 0x03;
        if (reg <= 1)
            pre_div = 1;
        else if (reg == 2)
            pre_div = 2;
        else
            pre_div = 4;
        pclk = xclk / pre_div;
    } else {
        read_reg(0x5C, &reg);
        //pre-divider
        pre_div = (reg >> 5) & 0x03;
        if (pre_div <= 1)
            pre_div = 1;
        else if (pre_div == 2)
            pre_div = 2;
        else
            pre_div = 4;
        //pll multiplier
        pll_m = 32 - (reg & 0x1F);
        //pll divider
        read_reg(0x5D, &reg);
        pll_div = ((reg >> 2) & 0x03) + 1;
        //system divider
        read_reg(0x11, &reg);
        sys_div = ((reg & 0x3F) + 1) * 2;
        pclk = xclk / pre_div * pll_m / pll_div / sys_div;
    }

    printk("pixel clock %d\n", pclk);
    return pclk;
}

static void adjust_htp(int fps)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h;

    read_reg(0x3E, &reg_h);
    read_reg(0x3D, &reg_l);
    pinfo->sw_lines_max = ((reg_h << 8) | reg_l);

    pinfo->htp = g_psensor->pclk / fps / pinfo->sw_lines_max;
    reg_h = (pinfo->htp >> 8) & 0xFF;
    reg_l = pinfo->htp & 0xFF;
    write_reg(0x2B, reg_h);
    write_reg(0x2A, reg_l);

    pinfo->t_row = (pinfo->htp * 10000) / (g_psensor->pclk / 1000);
    g_psensor->min_exp = (pinfo->t_row + 999) / 1000;
}

static int set_size(u32 width, u32 height)
{
    int ret = 0;

    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("%dx%d exceeds maximum resolution %dx%d!\n",
            width, height, SENSOR_MAX_WIDTH, SENSOR_MAX_HEIGHT);
        return -1;
    }

    adjust_htp(g_psensor->fps);

    // update sensor information
    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    g_psensor->out_win.width = SENSOR_MAX_WIDTH;
    g_psensor->out_win.height = SENSOR_MAX_HEIGHT;
    g_psensor->img_win.x = ((SENSOR_MAX_WIDTH - width) >> 1) &~BIT0;
    g_psensor->img_win.y = ((SENSOR_MAX_HEIGHT - height) >> 1) &~BIT0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    return ret;
}

static u32 get_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 lines, sw_u, sw_l;

    if (!g_psensor->curr_exp) {
        read_reg(0x16, &sw_u);
        read_reg(0x10, &sw_l);

        lines = (sw_u << 8) | sw_l;
        g_psensor->curr_exp = (lines * pinfo->t_row + 500) / 1000;  // unit : 1/10 ms
        pinfo->curr_exp_us = (lines * pinfo->t_row + 5) / 10;       // unit : us
    }

    return g_psensor->curr_exp;
}

static void set_exp_line(u32 lines)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 tmp, max_exp_line, dummy_line;

    max_exp_line = pinfo->sw_lines_max - 7;
    if (lines > max_exp_line) {
        tmp = max_exp_line;
        dummy_line = lines - max_exp_line;
    } else {
        tmp = lines;
        dummy_line = 0;
    }

    { // group exposure related register to avoid I2C command be interrupted
        struct i2c_msg  msgs[4];
        unsigned char   buf[4][2];
        int i;

        //write_reg(0x16, (tmp >> 8) & 0xFF);
        buf[0][0] = 0x16;
        buf[0][1] = (tmp >> 8) & 0xFF;

        //write_reg(0x10, tmp & 0xFF);
        buf[1][0] = 0x10;
        buf[1][1] = tmp & 0xFF;

        //write_reg(0x2E, (dummy_line >> 8) & 0xFF);
        buf[2][0] = 0x2E;
        buf[2][1] = (dummy_line >> 8) & 0xFF;

        //write_reg(0x2D, dummy_line & 0xFF);
        buf[3][0] = 0x2D;
        buf[3][1] = dummy_line & 0xFF;

        for (i=0; i<4; i++) {
            msgs[i].addr  = I2C_ADDR;
            msgs[i].flags = 0;
            msgs[i].len   = 2;
            msgs[i].buf   = &buf[i][0];
        }

        sensor_i2c_transfer(msgs, 4);
    }
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 lines;

    lines = ((exp * 1000) + pinfo->t_row /2 ) / pinfo->t_row;
    if (lines == 0)
        lines = 1;

    set_exp_line(lines);
    g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;    // unit : 1/10 ms
    pinfo->curr_exp_us = (lines * pinfo->t_row + 5) / 10;           // unit : us
    return 0;
}

static u32 get_exp_us(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 lines, sw_u, sw_l;

    if (!pinfo->curr_exp_us) {
        read_reg(0x16, &sw_u);
        read_reg(0x10, &sw_l);

        lines = (sw_u << 8) | sw_l;
        g_psensor->curr_exp = (lines * pinfo->t_row + 500) / 1000;  // unit : 1/10 ms
        pinfo->curr_exp_us = (lines * pinfo->t_row + 5) / 10;       // unit : us
    }

    return pinfo->curr_exp_us;
}

static int set_exp_us(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 lines;

    lines = ((exp * 10) + pinfo->t_row /2 ) / pinfo->t_row;
    if (lines == 0)
        lines = 1;

    set_exp_line(lines);
    g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;    // unit : 1/10 ms
    pinfo->curr_exp_us = (lines * pinfo->t_row + 5) / 10;           // unit : us
    return 0;
}

static u32 get_gain(void)
{
    u32 gain_base, reg;
    u8 m0, m1, m2, m3;

    if (g_psensor->curr_gain == 0) {
        read_reg(0x0, &reg);
        m0 = (reg >> 4) & 0x01;
        m1 = (reg >> 5) & 0x01;
        m2 = (reg >> 6) & 0x01;
        m3 = (reg >> 7) & 0x01;

        gain_base = (((((u32)1 << m0) << m1) << m2) << m3) << GAIN_SHIFT;
        g_psensor->curr_gain = gain_base + ((gain_base * (reg & 0xF)) >> 4);
        read_reg(0x96, &reg);
        if(reg & BIT5){
            read_reg(0x05, &reg);
            g_psensor->curr_gain = (g_psensor->curr_gain * reg) >> 6;
        }
    }

    return g_psensor->curr_gain;
}

static int set_gain(u32 gain)
{
    u32 reg;
    u8 mult2 = 0, step = 0;
    u8 m3, m2, m1, m0;
    u8 awb_gain = 0x40;
    int i;

    if (gain > gain_table[NUM_OF_GAINSET - 1].total_gain)
        gain = gain_table[NUM_OF_GAINSET - 1].total_gain;
    else if(gain < gain_table[0].total_gain)
        gain = gain_table[0].total_gain;

    // search most suitable gain into gain table
    for (i=0; i<NUM_OF_GAINSET; i++) {
        if (gain_table[i].total_gain > gain)
            break;
    }

    if (i > 0) i--;

    mult2 = gain_table[i].mult2;
    step = gain_table[i].step;
    awb_gain = gain_table[i].awb_gain;

    reg = (mult2 << 4) | step;
    write_reg(0x00, reg);

    m0 = mult2 & 0x01;
	m1 = (mult2 >> 1) & 0x01;
    m2 = (mult2 >> 2) & 0x01;
    m3 = (mult2 >> 3) & 0x01;

    write_reg(0x05, awb_gain);  //G gain
    write_reg(0x02, awb_gain);  //R gain
    write_reg(0x01, awb_gain);  //B gain

    g_psensor->curr_gain = gain_table[i-1].total_gain;

    return 0;
}

static int get_mirror(void)
{
    u32 reg;

    read_reg(0x04, &reg);
    return (reg & BIT7) ? 1 : 0;
}

static int set_mirror(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    read_reg(0x4, &reg);
    if (pinfo->flip)
        reg |= BIT6;
    else
        reg &=~BIT6;

    if (enable)
        write_reg(0x4, (reg | BIT7));
    else
        write_reg(0x4, (reg &~BIT7));

    pinfo->mirror = enable;
    return 0;
}

static int get_flip(void)
{
    u32 reg;

    read_reg(0x04, &reg);
    return (reg & BIT6) ? 1 : 0;
}

static int set_flip(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    read_reg(0x4, &reg);
    if (pinfo->mirror)
        reg |= BIT7;
    else
        reg &=~BIT7;

    if (enable) {
        write_reg(0x4, (reg | BIT6));
        write_reg(0x3, 0xB);
        write_reg(0x19, 0x0);
    } else {
        write_reg(0x4, (reg &~BIT6));
        write_reg(0x3, 0xA);
        write_reg(0x19, 0x1);
    }
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

    case ID_EXP_US:
        *(u32*)arg = get_exp_us();
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

    case ID_EXP_US:
        set_exp_us((u32)arg);
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

    // write initial register value
    for (i=0; i<NUM_OF_INIT_REGS; i++) {
        if (write_reg(sensor_init_regs[i].addr, sensor_init_regs[i].val) < 0) {
            isp_err("failed to initial %s!\n", SENSOR_NAME);
            return -EINVAL;
        }
        mdelay(1);
    }

    // adjust PLL setting
    if (g_psensor->xclk == 12000000) {
        // (12MHz/1) * 14 / 1 / 2 / 2 = 42Mhz
        write_reg(0x5c, 0x32);
    } else if (g_psensor->xclk == 24000000) {
        // (24MHz/2) * 14 / 1 / 2 / 2 = 42Mhz
        write_reg(0x5c, 0x52);
    } else if (g_psensor->xclk == 27000000) {
        // (27MHz/4) * 25 / 1 / 2 / 2 = 42.1875Mhz
        write_reg(0x5c, 0x67);
    } else {
        isp_err("sensor input clock frequency not support!\n");
        return -EINVAL;
    }

    // get pixel clock
    g_psensor->pclk = get_pclk(g_psensor->xclk);

    // set image size
    if ((ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height)) < 0)
        return ret;

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
static void ov9715_deconstruct(void)
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

static int ov9715_construct(u32 xclk, u16 width, u16 height)
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

    pinfo = kzalloc(sizeof(sensor_info_t), GFP_KERNEL);
    if (!pinfo) {
        isp_err("failed to allocate private data!\n");
        return -ENOMEM;
    }

    g_psensor->private = pinfo;
    snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, SENSOR_NAME);
    g_psensor->capability = SUPPORT_MIRROR | SUPPORT_FLIP;
    g_psensor->xclk = xclk;
    g_psensor->interface = IF_PARALLEL;
    g_psensor->num_of_lane = 0;
    g_psensor->protocol = 0;
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

    if ((ret = init()) < 0)
        goto construct_err;

    return 0;

construct_err:
    ov9715_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init ov9715_init(void)
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

    if ((ret = ov9715_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit ov9715_exit(void)
{
    isp_unregister_sensor(g_psensor);
    ov9715_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(ov9715_init);
module_exit(ov9715_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor OV9715");
MODULE_LICENSE("GPL");
