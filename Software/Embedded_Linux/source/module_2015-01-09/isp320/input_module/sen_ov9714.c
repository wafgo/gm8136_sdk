/**
 * @file sen_ov9714.c
 * OmniVision ov9714 sensor driver
 *
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.8 $
 * $Date: 2014/11/27 11:28:06 $
 *
 * ChangeLog:
 *  $Log: sen_ov9714.c,v $
 *  Revision 1.8  2014/11/27 11:28:06  jason_ha
 *  FIXED Long exp bug
 *
 *  Revision 1.6  2014/07/25 08:38:32  ricktsao
 *  no message
 *
 *  Revision 1.5  2014/07/22 09:24:50  pshung
 *  *** empty log message ***
 *
 *  Revision 1.4  2014/07/15 07:36:52  pshung
 *  1. fix for t_row
 *
 *  Revision 1.3  2014/07/04 11:15:54  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.2  2014/06/12 03:58:03  pshung
 *  1. Remove dummy code and mark Group registers for flicker issue
 *
 *  Revision 1.1  2014/06/11 08:36:24  pshung
 *  Rename to OV9714 and support 60 fps
 *
 *  Revision 1.3  2014/04/30 03:29:55  jtwang
 *  MIPI driver
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

#define PFX "sen_ov9714"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1280
#define DEFAULT_IMAGE_HEIGHT    800
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

static uint ch_num = 2;
module_param(ch_num, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_num, "channel number");

static uint fps = 30;
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "frame per second");
//=============================================================================
// global
//=============================================================================
#define SENSOR_NAME         "ov9714"
#define SENSOR_MAX_WIDTH    1280
#define SENSOR_MAX_HEIGHT   800
#define DELAY_CODE          0xffff


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
    u32 sclk;
} sensor_info_t;

typedef struct sensor_reg {
    u16 addr;
    u8  val;
} sensor_reg_t;
//MIPI Sensor  1280x800 , 10 RAWs , 2 LANE
static sensor_reg_t sensor_init_regs[] = {
    {0x0100,0x00},
    {0x0103,0x01},
    {0x0100,0x00},
    {0x0100,0x00},
    {0x0100,0x00},
    {0x0100,0x00},
    {0x0101,0x00},
    {0x3001,0x01},
    {0x3712,0x7a},
    {0x3714,0x90},
    {0x3729,0x04},
    {0x3731,0x14},
    {0x3624,0x77},
    {0x3621,0x44},
    {0x3623,0xcc},
    {0x371e,0x30},
    {0x3601,0x44},
    {0x3603,0x63},
    {0x3635,0x05},
    {0x3632,0xa7},
    {0x3634,0x57},
    {0x370b,0x01},
    {0x3711,0x0e},
    {0x0340,0x03},
    {0x0341,0x40},
    {0x0342,0x06},
    {0x0343,0x3e},
    {0x0344,0x00},
    {0x0345,0x00},
    {0x0346,0x00},
    {0x0347,0x00},
    {0x0348,0x05},
    {0x0349,0x1f},
    {0x034a,0x03},
    {0x034b,0x2f},
    {0x034c,0x05},
    {0x034d,0x00},
    {0x034e,0x03},
    {0x034f,0x20},
    {0x0381,0x01},
    {0x0383,0x01},
    {0x0385,0x01},
    {0x0387,0x01},
    {0x3002,0x09},
    {0x3012,0x20},
    {0x3013,0x12},
    {0x3014,0x84},
    {0x301f,0x83},
    {0x3020,0x02},
    {0x3024,0x20},
    {0x3090,0x01},
    {0x3091,0x05},
    {0x3092,0x03},
    {0x3093,0x01},
    {0x3094,0x02},
    {0x309a,0x01},
    {0x309b,0x01},
    {0x309c,0x01},
    {0x30a3,0x19},
    {0x30a4,0x01},
    {0x30a5,0x03},
    {0x30b3,0x28},
    {0x30b5,0x04},
    {0x30b6,0x02},
    {0x30b7,0x02},
    {0x30b8,0x01},
    {0x3103,0x00},
    {0x3200,0x00},
    {0x3201,0x03},
    {0x3202,0x06},
    {0x3203,0x08},
    {0x3208,0x00},
    {0x3209,0x00},
    {0x320a,0x00},
    {0x320b,0x01},
    {0x3500,0x00},
    {0x3501,0x20},
    {0x3502,0x00},
    {0x3503,0x07},
    {0x3504,0x00},
    {0x3505,0x20},
    {0x3506,0x00},
    {0x3507,0x00},
    {0x3508,0x20},
    {0x3509,0x00},
    {0x350a,0x00},
    {0x350b,0x10},
    {0x350c,0x00},
    {0x350d,0x10},
    {0x350f,0x10},
    {0x3628,0x12},
    {0x3633,0x14},
    {0x3641,0x83},
    {0x3643,0x14},
    {0x3660,0x00},
    {0x3662,0x11},
    {0x3667,0x00},
    {0x370d,0xc0},
    {0x370e,0x80},
    {0x370a,0x31},
    {0x3714,0x80},
    {0x371f,0xa0},
    {0x3730,0x86},
    {0x3732,0x14},
    {0x3811,0x10},
    {0x3813,0x08},
    {0x3820,0x80},
    {0x3821,0x00},
    {0x382a,0x00},
    {0x3832,0x80},
    {0x3902,0x08},
    {0x3903,0x00},
    {0x3912,0x80},
    {0x3913,0x00},
    {0x3907,0x80},
    {0x3903,0x00},
    {0x3b00,0x00},
    {0x3b02,0x00},
    {0x3b03,0x00},
    {0x3b04,0x00},
    {0x3b40,0x00},
    {0x3b41,0x00},
    {0x3b42,0x00},
    {0x3b43,0x00},
    {0x3b44,0x00},
    {0x3b45,0x00},
    {0x3b46,0x00},
    {0x3b47,0x00},
    {0x3b48,0x00},
    {0x3b49,0x00},
    {0x3b4a,0x00},
    {0x3b4b,0x00},
    {0x3b4c,0x00},
    {0x3b4d,0x00},
    {0x3b4e,0x00},
    {0x3e00,0x00},
    {0x3e01,0x00},
    {0x3e02,0x0f},
    {0x3e03,0xdb},
    {0x3e06,0x03},
    {0x3e07,0x20},
    {0x3f04,0xd0},
    {0x3f05,0x00},
    {0x3f00,0x03},
    {0x3f11,0x06},
    {0x3f13,0x20},
    {0x4140,0x98},
    {0x4141,0x41},
    {0x4143,0x03},
    {0x4144,0x54},
    {0x4240,0x02},
    {0x4300,0xff},
    {0x4301,0x00},
    {0x4307,0x30},
    {0x4315,0x00},
    {0x4316,0x30},
    {0x4500,0xa2},
    {0x4501,0x14},
    {0x4502,0x60},
    {0x4580,0x00},
    {0x4583,0x83},
    {0x4826,0x32},
    {0x4837,0x10},
    {0x4602,0xf2},
    {0x4142,0x24},
    {0x4001,0x06},
    {0x4002,0x45},
    {0x4004,0x04},
    {0x4005,0x40},
    {0x5000,0xff},
    {0x5001,0x1f},
    {0x5003,0x00},
    {0x500a,0x85},
    {0x5080,0x00},
    {0x5091,0xa0},
    {0x5300,0x05},

//;30FPS KEY
    {0x3094,0x01},
    {0x30b3,0x14},
    {0x4837,0x21},
//;
    {0x0100,0x01},
//;
    {0x3501,0x33},
    {0x3502,0xc0},
    {0x3a02,0x40},
    {0x3a03,0x30},
    {0x3a04,0x03},
    {0x3a05,0x3c}
};


#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))



typedef struct gain_set {
    u8  mult2;
    u8  step;
    u8  substep;
    u32 total_gain;     // UFIX 7.6
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
    u32 pclk;
    u32 pre_div, pll_m, pll_div, sys_div;
    u32 pll_pix_div[4] = {6,8,10,12};


    read_reg(0x30b8, &pre_div);
    read_reg(0x30b3, &pll_m);
    read_reg(0x30b6, &pll_div);
    read_reg(0x30b5, &sys_div);

    pre_div &= 0x7;
    pll_m &= 0xFF;
    pll_div &= 0xF;
    sys_div &= 0x7;


    pclk = xclk / pre_div * pll_m / pll_div / pll_pix_div[sys_div];
    isp_dbg("xclk=%d, PCLK= %d\n", xclk ,pclk);
    return pclk;
}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h, reg_msb;
    u32 hts;

    read_reg(0x0342, &reg_h);
    read_reg(0x0343, &reg_l);
    hts = ((reg_h & 0xF) << 8) | (reg_l & 0xFF);
    
    pinfo->t_row = (hts * 10000) / (pinfo->sclk / 1000);

    g_psensor->min_exp = (pinfo->t_row + 999) / 1000;

    read_reg(0x0340, &reg_h);
    read_reg(0x0341, &reg_l);
    pinfo->sw_lines_max = (((reg_h & 0xF) << 8) | (reg_l & 0xFF));//2715:1104-6

    read_reg(0x3502, &reg_l);
    read_reg(0x3501, &reg_h);
    read_reg(0x3500, &reg_msb);
    pinfo->prev_sw_reg = ((reg_msb & 0xFF) << 16) | ((reg_h & 0xFF) << 8) | (reg_l & 0xFF);

    isp_info("hts=%d, t_row=%d min_exp=%d sw_lines_max=%d prev_sw_reg=%d\n", hts, pinfo->t_row, g_psensor->min_exp, pinfo->sw_lines_max, pinfo->prev_sw_reg);

}

static void adjust_blk(int fps)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_h, reg_l;
    u32 hts, vts;
    u32 pll2_prediv, pll2_multiplier, pll2_divs, pll2_seld5;
    u32 scale;
    u32 sheld5[4] = {10, 10, 20, 25};

    read_reg(0x0342, &reg_h);
    read_reg(0x0343, &reg_l);
    hts = ((reg_h & 0xF) << 8) | (reg_l & 0xFF);

    read_reg(0x0340, &reg_h);
    read_reg(0x0341, &reg_l);
    vts = ((reg_h & 0xF) << 8) | (reg_l & 0xFF);

    read_reg(0x3090, &pll2_prediv);
    read_reg(0x3091, &pll2_multiplier);
    read_reg(0x3092, &pll2_divs);
    read_reg(0x3093, &pll2_seld5);
    pll2_prediv &= 0x7;
    pll2_multiplier &= 0x3F;
    pll2_divs &= 0x1F;
    pll2_seld5 &= 0x3;

    if( fps > 30){
        scale = 2;
        write_reg(0x3094,0x02);
        write_reg(0x30b3,0x028);
        //write_reg(0x4837,0x010);//Not work, fps->0
    }
    else{
        scale = 1;
        write_reg(0x3094,0x01);
        write_reg(0x30b3,0x014);
        //write_reg(0x30b3,0x0f); // PLL_MUL,  0x0f= 45M , 0x10=48M,  0x14= 60M
        //write_reg(0x4837,0x021);;//Not work, fps->0
    }

    pinfo->sclk = g_psensor->xclk * pll2_multiplier / pll2_prediv / pll2_divs / sheld5[pll2_seld5]*10*scale;

    vts= pinfo->sclk/fps/hts;

    write_reg(0x0340, (vts >> 8) & 0xF);
    write_reg(0x0341, vts & 0xFF);

    isp_dbg("hts=%d, vts=%d, fps=%d", hts, vts, fps);

    calculate_t_row();
    g_psensor->fps = fps;
}



static int set_size(u32 width, u32 height)
{
    int ret = 0;

    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("%dx%d exceeds maximum resolution %dx%d!\n",
            width, height, SENSOR_MAX_WIDTH, SENSOR_MAX_HEIGHT);
        return -1;
    }

    if (g_psensor->interface == IF_MIPI){
        isp_info("Sensor Interface : MIPI\n");
        isp_info("Sensor Resolution : %d x %d\n", g_psensor->img_win.width, g_psensor->img_win.height);
        {
            write_reg(0x0340, 0x03);
            write_reg(0x0341, 0x40);
            write_reg(0x0342, 0x06);
            write_reg(0x0343, 0x3e);
            write_reg(0x034c, 0x05);
            write_reg(0x034d, 0x00);
            write_reg(0x034e, 0x03);
            write_reg(0x034f, 0x20);

            write_reg(0x0383, 0x01);
            write_reg(0x0387, 0x01);

            write_reg(0x370e, 0x80);
            write_reg(0x370a, 0x31);
            write_reg(0x3811, 0x10);
            write_reg(0x3813, 0x08);
            write_reg(0x3821, 0x00);
            write_reg(0x4144, 0x54);
            g_psensor->out_win.x = 0;
            g_psensor->out_win.y = 0;
            g_psensor->out_win.width = SENSOR_MAX_WIDTH;
            g_psensor->out_win.height = SENSOR_MAX_HEIGHT;
        }
    }
    g_psensor->pclk = get_pclk(g_psensor->xclk);
    adjust_blk(g_psensor->fps);

    g_psensor->img_win.x = (g_psensor->out_win.width-width)/2;
    g_psensor->img_win.y = (g_psensor->out_win.height-height)/2;
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
        //printk("get_exp lines=%d  t_row=%d\n",lines, pinfo->t_row);
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    int max_exp_line;
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 lines,sw_res,tmp;

    lines = ((exp * 1000) + pinfo->t_row /2 ) / pinfo->t_row;
    
    if (lines == 0)
        lines = 1;

    max_exp_line = pinfo->sw_lines_max - 4;

    if (lines > max_exp_line) {
        sw_res = lines - max_exp_line;
        tmp = max_exp_line;        
    } else {
        tmp = lines;        
    }

    tmp = lines << 4;

    isp_dbg("tmp = %d, pinfo->prev_sw_reg=%d\n",tmp, pinfo->prev_sw_reg);      

    if (lines > max_exp_line) {
        write_reg(0x0340, (lines >> 8) & 0xFF);
        write_reg(0x0341, (lines & 0xFF));                
        isp_dbg("lines = %d\n",lines); 
    }
    if (tmp != pinfo->prev_sw_reg){        
        write_reg(0x3502, (tmp & 0xFF));
        write_reg(0x3501, (tmp >> 8) & 0xFF);
        write_reg(0x3500, (tmp >> 16) & 0xFF);
        pinfo->prev_sw_reg = tmp;       
        isp_dbg("pinfo->prev_sw_reg = %d\n",pinfo->prev_sw_reg); 
    }  

    g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;

    isp_dbg("set_exp lines=%d  t_row=%d sw_lines_max=%d exp=%d\n",lines, pinfo->t_row, pinfo->sw_lines_max,g_psensor->curr_exp);

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
    u8 mult2 = 0, step = 0, substep = 0;
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

    write_reg(0x350A, mult2);
    write_reg(0x350B, ((u32)step << 4) | substep);

    g_psensor->curr_gain = gain_table[i-1].total_gain;

    return ret;
}

static void update_bayer_type(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (pinfo->flip){
        if (pinfo->mirror)
            g_psensor->bayer_type = BAYER_RG;
        else
            g_psensor->bayer_type = BAYER_GR;
    }
    else{
        if (pinfo->mirror)
            g_psensor->bayer_type = BAYER_GB;
        else
            g_psensor->bayer_type = BAYER_BG;
    }

    //printk ("===========flip=%d mirror=%d bayer=%d===========\n", pinfo->mirror, pinfo->flip, g_psensor->bayer_type);

}

static int get_mirror(void)
{
    u32 reg;

    read_reg(0x0101, &reg);
    return (reg & BIT0) ? 1 : 0;
}

static int set_mirror(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    read_reg(0x0101, &reg);
    if (pinfo->flip)
        reg |= BIT1;
    else
        reg &=~BIT1;

    
    if (enable) {
        write_reg(0x0101, (reg | BIT0));
    } else {
        write_reg(0x0101, (reg &~BIT0));
    }
    
    pinfo->mirror = enable;
    update_bayer_type();

    return 0;
}

static int get_flip(void)
{
    u32 reg;

    read_reg(0x0101, &reg);
    return (reg & BIT1) ? 1 : 0;
}

static int set_flip(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    read_reg(0x0101, &reg);
    if (pinfo->mirror)
        reg |= BIT0;
    else
        reg &=~BIT0;

   
    if (enable) {
        write_reg(0x0101, (reg | BIT1));
    } else {
        write_reg(0x0101, (reg &~BIT1));
    }
    
    pinfo->flip = enable;
    update_bayer_type();

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

    // write initial register value
    for (i=0; i<NUM_OF_INIT_REGS; i++) {
        if (sensor_init_regs[i].addr == DELAY_CODE) {
                    msleep(sensor_init_regs[i].val);
                }
        else if (write_reg(sensor_init_regs[i].addr, sensor_init_regs[i].val) < 0) {
            isp_err("failed to initial %s!", SENSOR_NAME);
            return -EINVAL;
        }
        //mdelay(10);      
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

    // get current exposure and gain setting
    get_exp();
    get_gain();

    pinfo->is_init = true;

    return ret;
}

//=============================================================================
// external functions
//=============================================================================
static void ov9714_deconstruct(void)
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

static int ov9714_construct(u32 xclk, u16 width, u16 height)
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
        isp_err("failed to allocate private data!");
        return -ENOMEM;
    }

    g_psensor->private = pinfo;
    snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, SENSOR_NAME);
    g_psensor->capability = SUPPORT_MIRROR | SUPPORT_FLIP;
    g_psensor->xclk = xclk;
    g_psensor->fmt = RAW10;
    g_psensor->bayer_type = BAYER_RG;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_HIGH;
    g_psensor->img_win.x = 0;
    g_psensor->img_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;
    g_psensor->min_exp = 1;
    g_psensor->max_exp = 5000; // 0.5 sec
    g_psensor->min_gain = gain_table[0].total_gain;
    //g_psensor->max_gain = gain_table[NUM_OF_GAINSET - 1].total_gain;
    g_psensor->max_gain = 1024;//16X
    g_psensor->exp_latency = 1;
    g_psensor->gain_latency = 1;
    g_psensor->fps = fps;
    g_psensor->interface = IF_MIPI;
    g_psensor->num_of_lane = ch_num;
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
    ov9714_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init ov9714_init(void)
{
    int ret = 0;

    // check ISP driver version
    if (isp_get_input_inf_ver() != ISP_INPUT_INF_VER) {
        isp_err("input module version(%x) is not equal to fisp320.ko(%x)!",
             ISP_INPUT_INF_VER, isp_get_input_inf_ver());
        ret = -EINVAL;
        goto init_err1;
    }

    g_psensor = kzalloc(sizeof(sensor_dev_t), GFP_KERNEL);
    if (!g_psensor) {
        ret = -ENODEV;
        goto init_err1;
    }

    if ((ret = ov9714_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit ov9714_exit(void)
{
    isp_unregister_sensor(g_psensor);
    ov9714_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(ov9714_init);
module_exit(ov9714_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor ov9714");
MODULE_LICENSE("GPL");
