/**
 * @file sen_mt9p031.c
 * Aptina MT9P031 sensor driver
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.10 $
 * $Date: 2014/10/30 00:40:46 $
 *
 * ChangeLog:
 *  $Log: sen_mt9p031.c,v $
 *  Revision 1.10  2014/10/30 00:40:46  jtwang
 *  *** empty log message ***
 *
 *  Revision 1.9  2014/10/29 08:01:32  jtwang
 *  update fps value if fps changed
 *
 *  Revision 1.8  2014/07/25 08:38:32  ricktsao
 *  no message
 *
 *  Revision 1.7  2014/06/06 11:05:11  jason_ha
 *  Add MT9P006 reg and check chip ID
 *
 *  Revision 1.6  2013/11/15 08:59:52  jason_ha
 *  add update_bayer_type
 *
 *  Revision 1.4  2013/11/07 12:22:53  ricktsao
 *  no message
 *
 *  Revision 1.3  2013/10/24 10:59:43  ricktsao
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
#define DEFAULT_IMAGE_WIDTH     1280
#define DEFAULT_IMAGE_HEIGHT    720
#define DEFAULT_XCLK            24000000

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
#define SENSOR_NAME         "MT9P031"
#define SENSOR_MAX_WIDTH    2592
#define SENSOR_MAX_HEIGHT   1952
#define ID_MT9P031          0x08
#define ID_MT9P006          0x11

static sensor_dev_t *g_psensor = NULL;

typedef struct sensor_info {
    bool is_init;
    u32 img_w;
    u32 img_h;
    int row_bin;
    int col_bin;
    int mirror;
    int flip;
    u32 hb;
    u32 t_overhead;     // unit is 1/10 us
    u32 t_row;          // unit is 1/10 us
} sensor_info_t;

typedef struct sensor_reg {
    u8  addr;
    u16 val;
} sensor_reg_t;

static sensor_reg_t mt9p031_sensor_init_regs[] = {
    {0x0A, 0x8600},
    {0x01, 0x01C1},
    {0x02, 0x026D},
    {0x03, 0x0400},
    {0x04, 0x0500},
    {0x05, 0x0200},     // H balnking
    {0x06, 0x0019},     // v blanking
    {0x07, 0x1F82},
    //{0x07, 0x1F8E},     // 96->72Mhz
    {0x08, 0x0000},     // (6) SHUTTER_WIDTH_HI
    {0x09, 0x01F3},     // (6) INTEG_TIME_REG
    {0x0C, 0x0000},     // (6) SHUTTER_DELAY_REG
    //{0x20, 0x1840},   // output Dark Row, Dark Column
    {0x35, 0x002F},     // Global Gain
};
#define NUM_OF_MT9P031_INIT_REGS (sizeof(mt9p031_sensor_init_regs) / sizeof(sensor_reg_t))


static sensor_reg_t mt9p006_sensor_init_regs[] = {
    {0x0A, 0x8600},
    {0x01, 0x01C1},
    {0x02, 0x026D},
    {0x03, 0x0400},
    {0x04, 0x0500},
    {0x05, 0x0200},     // H balnking
    {0x06, 0x0019},     // v blanking
    {0x07, 0x1F82},
    //{0x07, 0x1F8E},     // 96->72Mhz
    {0x08, 0x0000},     // (6) SHUTTER_WIDTH_HI
    {0x09, 0x01F3},     // (6) INTEG_TIME_REG
    {0x0C, 0x0000},     // (6) SHUTTER_DELAY_REG
    //{0x20, 0x1840},   // output Dark Row, Dark Column
    {0x35, 0x002F},     // Global Gain
    {0x4F, 0x0011},
    {0x29, 0x0481},
    {0x3E, 0x0087},
    {0x3F, 0x0007},
    {0x41, 0x0003},
    {0x48, 0x0018},
    {0x57, 0x0002},
    {0x2A, 0x7F72},
    {0x1E, 0x0006},
    {0x70, 0x0079},
    {0x71, 0x7800},
    {0x72, 0x7800},
    {0x73, 0x0300},
    {0x74, 0x0300},
    {0x75, 0x3C00},
    {0x76, 0x4E3D},
    {0x77, 0x4E3D},
    {0x78, 0x774F},
    {0x79, 0x7900},
    {0x7A, 0x7900},
    {0x7B, 0x7800},
    {0x7C, 0x7800},
    {0x7E, 0x7800},
    {0x7F, 0x7800},
};
#define NUM_OF_MT9P006_INIT_REGS (sizeof(mt9p006_sensor_init_regs) / sizeof(sensor_reg_t))

typedef struct gain_set {
    u8  analog;
    u8  digit;
    u16 gain_x; // UFIX 7.6
} gain_set_t;

static const gain_set_t gain_table[] = {
    { 8,   0,   64}, { 9,   0,   72}, {10,   0,   80}, {11,   0,   88},
    {12,   0,   96}, {13,   0,  104}, {14,   0,  112}, {15,   0,  120},
    {16,   0,  128}, {17,   0,  136}, {18,   0,  144}, {19,   0,  152},
    {20,   0,  160}, {21,   0,  168}, {22,   0,  176}, {23,   0,  184},
    {24,   0,  192}, {25,   0,  200}, {26,   0,  208}, {27,   0,  216},
    {28,   0,  224}, {29,   0,  232}, {30,   0,  240}, {31,   0,  248},
    {32,   0,  256}, {33,   0,  264}, {34,   0,  272}, {35,   0,  280},
    {36,   0,  288}, {37,   0,  296}, {38,   0,  304}, {39,   0,  312},
    {40,   0,  320}, {41,   0,  328}, {42,   0,  336}, {43,   0,  344},
    {44,   0,  352}, {45,   0,  360}, {46,   0,  368}, {47,   0,  376},
    {48,   0,  384}, {49,   0,  392}, {50,   0,  400}, {51,   0,  408},
    {52,   0,  416}, {53,   0,  424}, {54,   0,  432}, {55,   0,  440},
    {56,   0,  448}, {57,   0,  456}, {58,   0,  464}, {59,   0,  472},
    {60,   0,  480}, {61,   0,  488}, {62,   0,  496}, {63,   0,  504},
    {56,   1,  504}, {57,   1,  513}, {58,   1,  522}, {59,   1,  531},
    {60,   1,  540}, {61,   1,  549}, {62,   1,  558}, {63,   1,  567},
    {57,   2,  570}, {58,   2,  580}, {59,   2,  590}, {60,   2,  600},
    {61,   2,  610}, {62,   2,  620}, {63,   2,  630}, {58,   3,  638},
    {59,   3,  649}, {60,   3,  660}, {61,   3,  671}, {62,   3,  682},
    {63,   3,  693}, {58,   4,  696}, {59,   4,  708}, {60,   4,  720},
    {61,   4,  732}, {62,   4,  744}, {63,   4,  756}, {59,   5,  767},
    {60,   5,  780}, {61,   5,  793}, {62,   5,  806}, {63,   5,  819},
    {59,   6,  826}, {60,   6,  840}, {61,   6,  854}, {62,   6,  868},
    {63,   6,  882}, {59,   7,  885}, {60,   7,  900}, {61,   7,  915},
    {62,   7,  930}, {63,   7,  945}, {60,   8,  960}, {61,   8,  976},
    {62,   8,  992}, {63,   8, 1008}, {60,   9, 1020}, {61,   9, 1037},
    {62,   9, 1054}, {63,   9, 1071}, {60,  10, 1080}, {61,  10, 1098},
    {62,  10, 1116}, {63,  10, 1134}, {60,  11, 1140}, {61,  11, 1159},
    {62,  11, 1178}, {63,  11, 1197}, {60,  12, 1200}, {61,  12, 1220},
    {62,  12, 1240}, {63,  12, 1260}, {60,  13, 1260}, {61,  13, 1281},
    {62,  13, 1302}, {63,  13, 1323}, {61,  14, 1342}, {62,  14, 1364},
    {63,  14, 1386}, {61,  15, 1403}, {62,  15, 1426}, {63,  15, 1449},
    {61,  16, 1464}, {62,  16, 1488}, {63,  16, 1512}, {61,  17, 1525},
    {62,  17, 1550}, {63,  17, 1575}, {61,  18, 1586}, {62,  18, 1612},
    {63,  18, 1638}, {61,  19, 1647}, {62,  19, 1674}, {63,  19, 1701},
    {61,  20, 1708}, {62,  20, 1736}, {63,  20, 1764}, {61,  21, 1769},
    {62,  21, 1798}, {63,  21, 1827}, {61,  22, 1830}, {62,  22, 1860},
    {63,  22, 1890}, {61,  23, 1891}, {62,  23, 1922}, {63,  23, 1953},
    {62,  24, 1984}, {63,  24, 2016}, {62,  25, 2046}, {63,  25, 2079},
    {62,  26, 2108}, {63,  26, 2142}, {62,  27, 2170}, {63,  27, 2205},
    {62,  28, 2232}, {63,  28, 2268}, {62,  29, 2294}, {63,  29, 2331},
    {62,  30, 2356}, {63,  30, 2394}, {62,  31, 2418}, {63,  31, 2457},
    {62,  32, 2480}, {63,  32, 2520}, {62,  33, 2542}, {63,  33, 2583},
    {62,  34, 2604}, {63,  34, 2646}, {62,  35, 2666}, {63,  35, 2709},
    {62,  36, 2728}, {63,  36, 2772}, {62,  37, 2790}, {63,  37, 2835},
    {62,  38, 2852}, {63,  38, 2898}, {62,  39, 2914}, {63,  39, 2961},
    {62,  40, 2976}, {63,  40, 3024}, {62,  41, 3038}, {63,  41, 3087},
    {62,  42, 3100}, {63,  42, 3150}, {62,  43, 3162}, {63,  43, 3213},
    {62,  44, 3224}, {63,  44, 3276}, {62,  45, 3286}, {63,  45, 3339},
    {62,  46, 3348}, {63,  46, 3402}, {62,  47, 3410}, {63,  47, 3465},
    {62,  48, 3472}, {63,  48, 3528}, {62,  49, 3534}, {63,  49, 3591},
    {62,  50, 3596}, {63,  50, 3654}, {62,  51, 3658}, {63,  51, 3717},
    {62,  52, 3720}, {63,  52, 3780}, {62,  53, 3782}, {63,  53, 3843},
    {62,  54, 3844}, {63,  54, 3906}, {62,  55, 3906}, {63,  55, 3969},
    {63,  56, 4032}, {63,  57, 4095}, {63,  58, 4158}, {63,  59, 4221},
    {63,  60, 4284}, {63,  61, 4347}, {63,  62, 4410}, {63,  63, 4473},
    {63,  64, 4536}, {63,  65, 4599}, {63,  66, 4662}, {63,  67, 4725},
    {63,  68, 4788}, {63,  69, 4851}, {63,  70, 4914}, {63,  71, 4977},
    {63,  72, 5040}, {63,  73, 5103}, {63,  74, 5166}, {63,  75, 5229},
    {63,  76, 5292}, {63,  77, 5355}, {63,  78, 5418}, {63,  79, 5481},
    {63,  80, 5544}, {63,  81, 5607}, {63,  82, 5670}, {63,  83, 5733},
    {63,  84, 5796}, {63,  85, 5859}, {63,  86, 5922}, {63,  87, 5985},
    {63,  88, 6048}, {63,  89, 6111}, {63,  90, 6174}, {63,  91, 6237},
    {63,  92, 6300}, {63,  93, 6363}, {63,  94, 6426}, {63,  95, 6489},
    {63,  96, 6552}, {63,  97, 6615}, {63,  98, 6678}, {63,  99, 6741},
    {63, 100, 6804}, {63, 101, 6867}, {63, 102, 6930}, {63, 103, 6993},
    {63, 104, 7056}, {63, 105, 7119}, {63, 106, 7182}, {63, 107, 7245},
    {63, 108, 7308}, {63, 109, 7371}, {63, 110, 7434}, {63, 111, 7497},
    {63, 112, 7560}, {63, 113, 7623}, {63, 114, 7686}, {63, 115, 7749},
    {63, 116, 7812}, {63, 117, 7875}, {63, 118, 7938}, {63, 119, 8001},
    {63, 120, 8064}
};
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

//=============================================================================
// I2C
//=============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            (0x90 >> 1)
#include "i2c_common.c"

//=============================================================================
// internal functions
//=============================================================================
static int read_reg(u32 addr, u32 *pval)
{
    struct i2c_msg  msgs[2];
    unsigned char   tmp[1], tmp2[2];

    tmp[0]        = (u8)addr;
    msgs[0].addr  = I2C_ADDR;
    msgs[0].flags = 0;
    msgs[0].len   = 1;
    msgs[0].buf   = tmp;

    tmp2[0]       = 0;
    msgs[1].addr  = I2C_ADDR;
    msgs[1].flags = 1;
    msgs[1].len   = 2;
    msgs[1].buf   = tmp2;

    if (sensor_i2c_transfer(msgs, 2))
        return -1;

    *pval = (tmp2[0] << 8) | tmp2[1];
    return 0;
}

static int write_reg(u32 addr, u32 val)
{
    struct i2c_msg  msgs;
    unsigned char   buf[3];

    buf[0]     = (u8)addr;
    buf[1]     = (val >> 8) & 0xFF;
    buf[2]     = val & 0xFF;
    msgs.addr  = I2C_ADDR;
    msgs.flags = 0;
    msgs.len   = 3;
    msgs.buf   = buf;

    return sensor_i2c_transfer(&msgs, 1);
}

static void sync_changes(int on)
{
    u32 reg;

    read_reg(0x7, &reg);
    if (on)
        reg &=~0x1;
    else
        reg |= 0x1;
    write_reg(0x7, reg);
}

static u32 get_pclk(u32 xclk)
{
    u32 reg, pll_cfg, pll_cfg2, pclk;

    pclk = xclk;

    read_reg(0x10, &reg);
    if ((reg & 0x3) == 0x3) {
        read_reg(0x11, &pll_cfg);
        read_reg(0x12, &pll_cfg2);
        pclk *= ((pll_cfg >> 8) & 0xFF);
        pclk /= (((pll_cfg & 0xFF) + 1) * ((pll_cfg2 & 0x1F) + 1));
    }

    read_reg(0x07, &reg);
    if (reg & BIT2) {
        printk("pixel clock %d\n", (pclk * 3 / 4));
    } else {
        printk("pixel clock %d\n", pclk);
    }
    return pclk;
}

static void adjust_vblk(int fps)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int v_blk = 8;

    if (g_psensor->img_win.width > 1920 || g_psensor->img_win.height > 1080){
        if (fps > 13)
            fps = 13;
    }
    else{
        if (fps > 30)
            fps = 30;
    }
    /* Adjust Vertical Blanking to keep fps
     * (img_h + MAX(V_Blk, VB_min)) * tRow = tFrame (1/fps)
     * (img_h + V_Blk) * (2*1/pclk * MAX((img_w/2 + HB), (41 + 346 *(Row_Bin + 1) + 99)) = 1/fps
     * V_Blk = 1/fps/(2*1/pclk * MAX((img_w/2 + HB), (41 + 346*(Row_Bin + 1) + 99))) - img_h
     */
    v_blk = g_psensor->pclk / fps / 2 / MAX((pinfo->img_w/2 + pinfo->hb), (41 + 346*(pinfo->row_bin + 1) + 99)) - pinfo->img_h;
    v_blk = MAX(9, v_blk);
    write_reg(0x6, v_blk);
    g_psensor->fps = fps;
}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;
    u32 s_overhead; // shutter overhead
    u32 s_delay;    // shutter delay
    u32 img_w, img_h;
    int row_bin, col_bin;
    u32 hb, hb_blk, hb_min;

    const u32 HB_Min[4][4] = {
        { 450,  430, 000,  420},
        { 796,  776, 000,  766},
        { 000,  000, 000,  000},
        {1488, 1468, 000, 1458}
    };

    read_reg(0x22, &reg);
    row_bin = (reg >> 4) & 0x3;
    read_reg(0x23, &reg);
    col_bin = (reg >> 4) & 0x3;

    hb_min = HB_Min[row_bin][col_bin];
    read_reg(0x5, &hb_blk);
    hb = MAX(hb_blk, hb_min);
    read_reg(0x3, &reg);
    img_h = reg + 1;
    read_reg(0x4, &reg);
    img_w = reg + 1;
    if (row_bin)
        img_w /= (row_bin + 1);
    read_reg(0xC, &s_delay);

    pinfo->img_w   = img_w;
    pinfo->img_h   = img_h;
    pinfo->row_bin = row_bin;
    pinfo->col_bin = col_bin;
    pinfo->hb      = hb;

    /* tEXP = SW กั tROW กV SO กั 2 กั tPIXCLK
     * SO = 208 กั (Row_Bin + 1) + 98 + MIN(SD, SDmax) - 94
     * tROW = 2 กั tPIXCLK กั MAX(((W/2) + MAX(HB, HBMIN)), (41 + 346กั(Row_Bin+1) + 99))
     * SW = (n / PowerFreq + SO กั 2 / pclk) / tROW
     *    = (n / PowerFreq + SO กั 2 / pclk) / (2 / pclk กั (W/2 + HBMIN))
     *    = (pclk กั n / PowerFreq + SO กั 2) / (W + 2 กั HBMIN)
     */
    pinfo->t_row = (2 * MAX((img_w/2 + hb), (41 + 346*(row_bin + 1) + 99)) * 10000)
                    / (g_psensor->pclk / 1000);
    s_overhead = 208 * (row_bin + 1) + 98 + (s_delay + 1) - 94;
    pinfo->t_overhead = (2 * s_overhead * 10000) / (g_psensor->pclk / 1000);

    g_psensor->min_exp = (pinfo->t_row + 999) / 1000;
}

static int set_size(u32 width, u32 height)
{
    int ret = 0;
    u32 regW, regH, regX, regY;

    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("%dx%d exceeds maximum resolution %dx%d!\n",
            width, height, SENSOR_MAX_WIDTH, SENSOR_MAX_HEIGHT);
        return -1;
    }

    regW = (width - 1 + 1) | BIT0;
    regH = (height - 1 + 1) | BIT0;
    regX = (16 + (SENSOR_MAX_WIDTH - width)/2) & ~BIT0;
    regY = (54 + (SENSOR_MAX_HEIGHT - height)/2) & ~BIT0;

    sync_changes(0);    // set synchronize changes flag

    write_reg(0x03, regH);
    write_reg(0x04, regW);
    write_reg(0x01, regY);
    write_reg(0x02, regX);

    // tricky: add H blanking time in 5M resolution
    if (width == SENSOR_MAX_WIDTH)
        write_reg(0x05, 0x2A0);
    else
        write_reg(0x05, 0x200);

    calculate_t_row();
    adjust_vblk(g_psensor->fps);

    sync_changes(1);    // synchronize changes

    g_psensor->out_win.x = regX;
    g_psensor->out_win.y = regY;
    g_psensor->out_win.width = regW + 1;
    g_psensor->out_win.height = regH + 1;

    g_psensor->img_win.x = 0;
    g_psensor->img_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    return ret;
}

static u32 get_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 lines, sw_u, sw_l;

    if (!g_psensor->curr_exp) {
        read_reg(0x8, &sw_u);
        read_reg(0x9, &sw_l);
        lines = (sw_u << 5) + sw_l;
        g_psensor->curr_exp = ((lines * pinfo->t_row) - pinfo->t_overhead + 500) / 1000;
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 lines;
    u16 sw_l, sw_u;;

    // tEXP = SW กั tROW กV SO กั 2 กั tPIXCLK
    // SW = (tEXP - (SO กั 2 กั tPIXCLK)) / tROW
    lines = ((exp * 1000) + pinfo->t_overhead) / pinfo->t_row;
    if (lines == 0)
        lines = 1;

    if (lines > 0xFFFF) {
        sw_u = (lines >> 5);
        sw_l = (lines & 0x1F);
    } else {
        sw_u = 0;
        sw_l = lines;
    }

    write_reg(0x08, sw_u);
    write_reg(0x09, sw_l);
    g_psensor->curr_exp = ((lines * pinfo->t_row) - pinfo->t_overhead + 500) / 1000;

    return ret;
}

static u32 get_gain(void)
{
    u32 reg;
    u32 analogx8, mul, digitalx64;

    if (g_psensor->curr_gain == 0) {
        read_reg(0x35, &reg);
        analogx8 = reg & 0x3F;
        mul = ((reg>>6)&0x01) + 1;
        digitalx64 = GAIN_1X + (((reg&0x7F00) >> 8) * GAIN_1X >> 3); //GAIN_1X*(1+((reg&0x7f00)>>8)/8)

        g_psensor->curr_gain = (analogx8 * mul * digitalx64) >> 3;
    }

    return g_psensor->curr_gain;
}

static int set_gain(u32 gain)
{
    int ret = 0;
    u32 ang_multi = 0;
    u32 ang_x, dig_x;
    u32 reg;
    int i;

    if (gain > gain_table[NUM_OF_GAINSET - 1].gain_x)
        gain = gain_table[NUM_OF_GAINSET - 1].gain_x;
    else if (gain < gain_table[0].gain_x)
        gain = gain_table[0].gain_x;

    // search most suitable gain into gain table
    for (i=0; i<NUM_OF_GAINSET; i++) {
        if (gain_table[i].gain_x > gain)
            break;
    }
    ang_x = gain_table[i-1].analog;
    dig_x = gain_table[i-1].digit;

    reg = (dig_x << 8) | (ang_multi << 6) | ang_x;
    write_reg(0x35, reg);
    g_psensor->curr_gain = ((ang_x) * (ang_multi + 1) * (64 + (dig_x << 3))) >> 3;

    return ret;
}

static void update_bayer_type(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (pinfo->mirror) {
        if (pinfo->flip)
            g_psensor->bayer_type = BAYER_GB;
        else
            g_psensor->bayer_type = BAYER_RG;
    } else {
        if (pinfo->flip)
            g_psensor->bayer_type = BAYER_BG;
        else
            g_psensor->bayer_type = BAYER_GR;
    }
}

static int get_mirror(void)
{
    u32 reg;

    read_reg(0x20, &reg);
    return (reg & BIT14) ? 1 : 0;
}

static int set_mirror(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg, col_start;

    read_reg(0x02, &col_start);
    read_reg(0x20, &reg);
    if (enable) {
        reg |= BIT14;
        col_start -= 2;
    } else {
        reg &=~BIT14;
        col_start += 2;
    }
    write_reg(0x20, reg);
    write_reg(0x02, col_start);

    pinfo->mirror = enable;
    update_bayer_type();

    return 0;
}

static int get_flip(void)
{
    u32 reg;

    read_reg(0x20, &reg);
    return (reg & BIT15) ? 1 : 0;
}

static int set_flip(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    read_reg(0x20, &reg);
    if (enable)
        reg |= BIT15;
    else
        reg &=~BIT15;
    write_reg(0x20, reg);

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
        adjust_vblk((int)arg);
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
    sensor_reg_t *sensor_init_regs;
    int num_of_init_regs;
    int ret = 0;
    int i;
    u32 ID;

    if (pinfo->is_init)
        return 0;

    // soft reset
    write_reg(0x0D, 0x01);
    mdelay(10);
    write_reg(0x0D, 0x00);
    mdelay(10);

    // set pixel clock
    write_reg(0x10, 0x0051);
    mdelay(10);
    if (g_psensor->xclk == 27000000) {
        write_reg(0x11, 0x2002);
        write_reg(0x12, 0x0002);
    } else if (g_psensor->xclk == 24000000) {
        write_reg(0x11, 0x1801);
        write_reg(0x12, 0x0002);
    } else {
        isp_err("XCLK %d Hz is not support!\n", g_psensor->xclk);
        return -EINVAL;
    }
    mdelay(10);
    write_reg(0x10, 0x0053);

    read_reg(0xFD, &ID);
    ID = (ID & 0x0FE0) >> 5;
    //printk("ID=0x%x\n",ID);
    if (ID == ID_MT9P031) {
        printk("[MT9P031]\n");
        sensor_init_regs = mt9p031_sensor_init_regs;
        num_of_init_regs = NUM_OF_MT9P031_INIT_REGS;
        snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, "MT9P031");
    } else {
        printk("[MT9P006]\n");
        sensor_init_regs = mt9p006_sensor_init_regs;
        num_of_init_regs = NUM_OF_MT9P006_INIT_REGS;
        snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, "MT9P006");
    }

    // write initial register value
    for (i=0; i<num_of_init_regs; i++) {
        if (write_reg(sensor_init_regs[i].addr, sensor_init_regs[i].val) < 0) {
            isp_err("failed to initial %s!\n", SENSOR_NAME);
            return -EINVAL;
        }
        mdelay(1);
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
static void mt9p031_deconstruct(void)
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

static int mt9p031_construct(u32 xclk, u16 width, u16 height)
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
    g_psensor->bayer_type = BAYER_GR;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_HIGH;
    g_psensor->inv_clk = inv_clk;
    g_psensor->img_win.x = 0;
    g_psensor->img_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;
    g_psensor->min_exp = 1;
    g_psensor->max_exp = 5000; // 0.5 sec
    g_psensor->min_gain = gain_table[0].gain_x;
    g_psensor->max_gain = gain_table[NUM_OF_GAINSET - 1].gain_x;
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
#if 0
    if ((ret = init()) < 0)
        goto construct_err;
#endif
    return 0;

construct_err:
    mt9p031_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init mt9p031_init(void)
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

    if ((ret = mt9p031_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit mt9p031_exit(void)
{
    isp_unregister_sensor(g_psensor);
    mt9p031_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(mt9p031_init);
module_exit(mt9p031_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor MT9P031");
MODULE_LICENSE("GPL");
