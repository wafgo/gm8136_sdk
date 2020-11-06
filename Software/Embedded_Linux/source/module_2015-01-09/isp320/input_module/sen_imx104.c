/**
 * @file sen_imx104.c
 * SONY IMX104 sensor driver
 *
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.5 $
 * $Date: 2014/07/25 08:38:32 $
 *
 * ChangeLog:
 *  $Log: sen_imx104.c,v $
 *  Revision 1.5  2014/07/25 08:38:32  ricktsao
 *  no message
 *
 *  Revision 1.4  2014/03/07 03:28:08  jtwang
 *  modify get_gain()
 *
 *  Revision 1.3  2013/12/10 09:19:06  jtwang
 *  *** empty log message ***
 *
 *  Revision 1.1  2013/11/20 02:13:59  jtwang
 *  add serial interface sensor modules
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

#define PFX "sen_imx104"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1280
#define DEFAULT_IMAGE_HEIGHT    1024
#define DEFAULT_XCLK            37125000

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

static uint ch_num = 4;
module_param(ch_num, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_num, "channel number");

static uint fps = 30;
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "frame per second");

//=============================================================================
// global
//=============================================================================
#define SENSOR_NAME         "IMX104"
#define SENSOR_MAX_WIDTH    1280
#define SENSOR_MAX_HEIGHT   1024

static sensor_dev_t *g_psensor = NULL;

typedef struct sensor_info {
    bool is_init;
    int row_bin;
    int col_bin;
    int mirror;
    int flip;
    u32 t_row;          // unit is 1/10 us
    int HMax;
    int VMax;
    int long_exp;
} sensor_info_t;

typedef struct sensor_reg {
    u16 addr;
    u16 val;
} sensor_reg_t;

static sensor_reg_t sensor_720P_init_regs[] = {
    {0x3002, 0x01},
    {0x3000, 0x01},
    {0x3005 ,0x01},         // 12bits
    {0x3006 ,0x00},         // All-pixel scan
    {0x3007 ,0x10},         // 720p HD mode
    {0x3009 ,0x02},         // 30 fps
    {0x3018 ,0xee},         // VMAX
    {0x3019 ,0x02},
    {0x301a ,0x00},
    {0x301b ,0xc8},         // HMAX
    {0x301c ,0x19},
    {0x3044, 0xe1},
    {0x305b ,0x00},
    {0x305d ,0x00},
    {0x305f ,0x00},
    {0x3000, 0x00},
    {0x3002, 0x00},
};
#define NUM_OF_720P_INIT_REGS (sizeof(sensor_720P_init_regs) / sizeof(sensor_reg_t))


static sensor_reg_t sensor_1024P_init_regs[] = {
    {0x3002, 0x01},
    {0x3000, 0x01},
    {0x3005 ,0x01},         // 12bits
    {0x3006 ,0x00},         // All-pixel scan
    {0x3007 ,0x00},         // All-pixel mode
    {0x3009 ,0x02},         //30 fps
    {0x3018 ,0x4c},         // VMAX
    {0x3019 ,0x04},
    {0x301a ,0x00},
    {0x301b ,0xa0},         // HMAX, 0x1194 => 0x11a0, should be 16x
    {0x301c ,0x11},
    {0x3044, 0xe1},         // 4ch
    {0x305b ,0x00},
    {0x305d ,0x00},
    {0x305f ,0x00},
    {0x3000, 0x00},
    {0x3002, 0x00},
};
#define NUM_OF_1024P_INIT_REGS (sizeof(sensor_1024P_init_regs) / sizeof(sensor_reg_t))

typedef struct _gain_set
{
    u16 ad_gain;
    u16 gain_x;     // UFIX7.6
} gain_set_t;

static gain_set_t gain_table[] =
{
{0x00,   64},{0x01,   66},{0x02,   68},{0x03,   70},{0x04,   73},
{0x05,   76},{0x06,   78},{0x07,   81},{0x08,   84},{0x09,   87},
{0x0A,   90},{0x0B,   93},{0x0C,   96},{0x0D,  100},{0x0E,  103},
{0x0F,  107},{0x10,  111},{0x11,  115},{0x12,  119},{0x13,  123},
{0x14,  127},{0x15,  132},{0x16,  136},{0x17,  141},{0x18,  146},
{0x19,  151},{0x1A,  157},{0x1B,  162},{0x1C,  168},{0x1D,  174},
{0x1E,  180},{0x1F,  186},{0x20,  193},{0x21,  200},{0x22,  207},
{0x23,  214},{0x24,  221},{0x25,  229},{0x26,  237},{0x27,  246},
{0x28,  254},{0x29,  263},{0x2A,  273},{0x2B,  282},{0x2C,  292},
{0x2D,  302},{0x2E,  313},{0x2F,  324},{0x30,  335},{0x31,  347},
{0x32,  359},{0x33,  372},{0x34,  385},{0x35,  399},{0x36,  413},
{0x37,  427},{0x38,  442},{0x39,  458},{0x3A,  474},{0x3B,  491},
{0x3C,  508},{0x3D,  526},{0x3E,  544},{0x3F,  563},{0x40,  583},
{0x41,  604},{0x42,  625},{0x43,  647},{0x44,  670},{0x45,  693},
{0x46,  718},{0x47,  743},{0x48,  769},{0x49,  796},{0x4A,  824},
{0x4B,  853},{0x4C,  883},{0x4D,  914},{0x4E,  946},{0x4F,  979},
{0x50, 1014},{0x51, 1049},{0x52, 1086},{0x53, 1125},{0x54, 1164},
{0x55, 1205},{0x56, 1247},{0x57, 1291},{0x58, 1337},{0x59, 1384},
{0x5A, 1432},{0x5B, 1483},{0x5C, 1535},{0x5D, 1589},{0x5E, 1645},
{0x5F, 1702},{0x60, 1762},{0x61, 1824},{0x62, 1888},{0x63, 1955},
{0x64, 2023},{0x65, 2094},{0x66, 2168},{0x67, 2244},{0x68, 2323},
{0x69, 2405},{0x6A, 2489},{0x6B, 2577},{0x6C, 2667},{0x6D, 2761},
{0x6E, 2858},{0x6F, 2959},{0x70, 3063},{0x71, 3170},{0x72, 3282},
{0x73, 3397},{0x74, 3517},{0x75, 3640},{0x76, 3768},{0x77, 3901},
{0x78, 4038},{0x79, 4180},{0x7A, 4326},{0x7B, 4478},{0x7C, 4636},
{0x7D, 4799},{0x7E, 4967},{0x7F, 5142},{0x80, 5323},{0x81, 5510},
{0x82, 5704},{0x83, 5904},{0x84, 6111},{0x85, 6326},{0x86, 6549},
{0x87, 6779},{0x88, 7017},{0x89, 7264},{0x8A, 7519},{0x8B, 7783},
{0x8C, 8057},

};
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

//=============================================================================
// I2C
//=============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            0x34 >> 1
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

    if (fps > 30)
        pclk = 99000000;
    else
        pclk = 49500000;

    isp_info("IMX104 Sensor: pclk(%d) XCLK(%d)\n", pclk, xclk);
    return pclk;
}

static void adjust_blanking(int fps)
{
    u32 vmax;

    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (g_psensor->img_win.height <= 720)
        vmax = 750;
    else
        vmax = 1100;
    if (fps > 30)
        pinfo->VMax = vmax*60/fps;
    else
        pinfo->VMax = vmax*30/fps;


    write_reg(0x3018, (pinfo->VMax & 0xFF));
    write_reg(0x3019, ((pinfo->VMax >> 8) & 0xFF));
    write_reg(0x301a, ((pinfo->VMax >> 16) & 0x1));

    isp_info("fps=%d\n", fps);
}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    // trow unit is 1/10 us
    pinfo->t_row = (pinfo->HMax * 10000) / (g_psensor->pclk / 1000);

}

static int set_size(u32 width, u32 height)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h, reg_msb;
    int ret = 0;

    // check size
    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("Exceed maximum sensor resolution!\n");
        return -1;
    }

    // get HMax and VMax
    read_reg(0x301b, &reg_l);
    read_reg(0x301c, &reg_h);

    pinfo->HMax = ( ((reg_h & 0xFF) << 8) | (reg_l & 0xFF) );

    read_reg(0x3018, &reg_l);
    read_reg(0x3019, &reg_h);
    read_reg(0x301a, &reg_msb);
    pinfo->VMax = ((reg_msb & 0x1) << 16) | ((reg_h & 0xFF) << 8) | (reg_l & 0xFF);


    // update sensor information
    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    if (width<=1280 && height<=720) {
        g_psensor->out_win.width = 1312;    //0x520
        g_psensor->out_win.height = 741;    //0x2e5
        g_psensor->img_win.x = 20;
        g_psensor->img_win.y = 15;
    }
    else {  //1280x1024
        g_psensor->out_win.width = 1312;    //0x520
        g_psensor->out_win.height = 1069;   //0x42d
        g_psensor->img_win.x = 20;
        g_psensor->img_win.y = 25;
    }

    adjust_blanking(g_psensor->fps);
    calculate_t_row();

    isp_info("Sensor : %dx%d, LVDS : %d ch\n", width, height, ch_num);

    return ret;
}

static u32 get_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h, reg_msb, shs1;
    u32 num_of_line;

    if (!g_psensor->curr_exp) {
        read_reg(0x3020, &reg_l);
        read_reg(0x3021, &reg_h);
        read_reg(0x3022, &reg_msb);
        shs1 = ((reg_msb & 0x1) << 16) | ((reg_h & 0xFF) << 8) | (reg_l & 0xFF);
        num_of_line = pinfo->VMax - shs1;
        g_psensor->curr_exp = num_of_line * pinfo->t_row / 1000;
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 exp_line, shs1;
    int ret = 0;

    exp_line = MAX(1, exp * 1000 / pinfo->t_row);

    if (exp_line <= pinfo->VMax) {
        shs1 = pinfo->VMax - exp_line;
        write_reg(0x3020, (shs1 & 0xFF));
        write_reg(0x3021, ((shs1 >> 8) & 0xFF));
        write_reg(0x3022, ((shs1 >> 16) & 0x1));
        if (pinfo->long_exp) {
            write_reg(0x3018, (pinfo->VMax & 0xFF));
            write_reg(0x3019, ((pinfo->VMax >> 8) & 0xFF));
            write_reg(0x301a, ((pinfo->VMax >> 16) & 0x1));
            pinfo->long_exp = 0;
        }
    } else {
        write_reg(0x3020, 0x0);
        write_reg(0x3021, 0x0);
        write_reg(0x3022, 0x0);
        write_reg(0x3018, (exp_line & 0xFF));
        write_reg(0x3019, ((exp_line >> 8) & 0xFF));
        write_reg(0x301a, ((exp_line >> 16) & 0x1));
        pinfo->long_exp = 1;
    }

    g_psensor->curr_exp = MAX(1, exp_line * pinfo->t_row / 1000);
    return ret;
}

static u32 get_gain(void)
{
    u32 reg, i;

    if (!g_psensor->curr_gain) {
        read_reg(0x3014, &reg);
        for (i=0; i<NUM_OF_GAINSET; i++) {
            if (gain_table[i].ad_gain == reg)
                break;
        }
        if (i < NUM_OF_GAINSET)
            g_psensor->curr_gain = gain_table[i].gain_x;
    }

    return g_psensor->curr_gain;
}

static int set_gain(u32 gain)
{
    u32 reg;
    int ret = 0, i;

    if (gain > gain_table[NUM_OF_GAINSET - 1].gain_x)
        gain = gain_table[NUM_OF_GAINSET - 1].gain_x;
    else if(gain < gain_table[0].gain_x)
        gain = gain_table[0].gain_x;

    // search most suitable gain into gain table
    for (i=0; i<NUM_OF_GAINSET; i++) {
        if (gain_table[i].gain_x > gain)
            break;
    }

    reg = gain_table[i-1].ad_gain & 0xFF;

    write_reg(0x3014, reg);

    g_psensor->curr_gain = gain_table[i-1].gain_x;
    return ret;
}

static int get_mirror(void)
{
    u32 reg;

    read_reg(0x3007, &reg);

    return reg & 0x02;
}

static int set_mirror(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    pinfo->mirror = enable;
    read_reg(0x3007, &reg);
    if (enable)
        reg |= 0x02;
    else
        reg &= ~0x02;
    write_reg(0x3007, reg);

    return 0;
}

static int get_flip(void)
{
    u32 reg;

    read_reg(0x3007, &reg);

    return reg & 0x01;
}

static int set_flip(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    pinfo->flip = enable;
    read_reg(0x3007, &reg);
    if (enable)
        reg |= 0x01;
    else
        reg &= ~0x01;
    write_reg(0x3007, reg);

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

    if (g_psensor->img_win.width <= 1280 && g_psensor->img_win.height <= 720) {
        pInitTable=sensor_720P_init_regs;
        InitTable_Num=NUM_OF_720P_INIT_REGS;
    }
    else if (g_psensor->img_win.width <= 1280 && g_psensor->img_win.height <= 1024) {
        pInitTable=sensor_1024P_init_regs;
        InitTable_Num=NUM_OF_1024P_INIT_REGS;
    }
    else{
        isp_info("invalid resolution...\n");
        return -EINVAL;
    }
//    isp_info("I2C_ADDR=%x\n", I2C_ADDR);
    isp_info("start initial...\n");
    // set initial registers
    for (i=0; i<InitTable_Num; i++) {

        if (write_reg(pInitTable[i].addr, pInitTable[i].val) < 0){
            isp_err("%s init fail!!", SENSOR_NAME);
            return -EINVAL;
        }
    }
    g_psensor->pclk = get_pclk(g_psensor->xclk);

    if (ch_num == 4)
        write_reg(0x3044, 0xe1);
    else if (ch_num == 2)
        write_reg(0x3044, 0xd1);
    else
        write_reg(0x3044, 0x61);    // 1 ch

    if (fps > 30){
        if (g_psensor->img_win.width <= 1280 && g_psensor->img_win.height <= 720) {
            write_reg(0x3009, 0x01);
            write_reg(0x301b, 0xe4);
            write_reg(0x301c, 0x0c);
        }
        else{
            write_reg(0x3009, 0x01);
            write_reg(0x301b, 0xd0);    // 0x8ca => 0x8d0, should be 16x
            write_reg(0x301c, 0x08);
        }
    }

    set_mirror(mirror);
    set_flip(flip);

    // set image size
    ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height);
    if (ret == 0)
        pinfo->is_init = 1;

    // initial exposure and gain
    set_exp(g_psensor->min_exp);
    set_gain(g_psensor->min_gain);

    // get current exposure and gain setting
    get_exp();
    get_gain();

    return ret;
}

//=============================================================================
// external functions
//=============================================================================
static void imx104_deconstruct(void)
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

static int imx104_construct(u32 xclk, u16 width, u16 height)
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
    g_psensor->fmt = RAW12;
    g_psensor->bayer_type = BAYER_RG;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_HIGH;
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
    g_psensor->interface = IF_LVDS;
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
    imx104_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init imx104_init(void)
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

    if ((ret = imx104_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit imx104_exit(void)
{
    isp_unregister_sensor(g_psensor);
    imx104_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(imx104_init);
module_exit(imx104_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor IMX104");
MODULE_LICENSE("GPL");
