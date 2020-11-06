/**
 * @file sen_ps1210.c
 * Pixelplus PS1210 sensor driver
 *
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
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

#define PFX "sen_ps1210"
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

static uint ch_num = 4;
module_param(ch_num, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_num, "channel number");

static uint fps = 30;
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "frame per second");

static uint interface = IF_LVDS_PANASONIC;
module_param(interface, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(interface, "interface type");

//=============================================================================
// global
//=============================================================================
#define SENSOR_NAME         "PS1210"
#define SENSOR_MAX_WIDTH    1932
#define SENSOR_MAX_HEIGHT   1092

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
    u32 min_a_gain;
    u32 max_a_gain;
    u32 min_d_gain;
    u32 max_d_gain;
    u32 curr_a_gain;
    u32 curr_d_gain;
} sensor_info_t;

typedef struct sensor_reg {
    u16 addr;
    u16 val;
} sensor_reg_t;
#define DELAY_CODE      0xFFFF

static sensor_reg_t sensor_1080P_init_regs[] = {
    {0x03,0x00},
    {0x29,0x98},
    {0x03,0x00},
    {0x05,0x03},
    {0x06,0x08},
    {0x07,0x97},
    {0x14,0x00},
    {0x15,0x0B},
    {0x25,0x10},
    {0x33,0x01},
    {0x34,0x02},
    {0x36,0xC8},
    {0x38,0x48},
    {0x3A,0x22},
    {0x41,0x21},
    {0x42,0x04},
    {0x40,0x10},
    {DELAY_CODE,50},
    {0x40,0x00},
    {0x03,0x01},
    {0x26,0x03},
    {0x62,0x96},
    {0x03,0x01},
    {0xC0,0x04},
    {0xC1,0x5F},
    {0xC2,0x00},
    {0xC3,0x00},
    {0xC4,0x40},
//    {0xb6,0x88},
//    {0x03,0x02},
//    {0x1D,0x3F},
//    {0x1F,0x42},
//    {0x03,0x01},
};
#define NUM_OF_1080P_INIT_REGS (sizeof(sensor_1080P_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_720P_init_regs[] = {
    {0x03,0x00},
    {0x05,0x03},
    {0x06,0x08},
    {0x07,0x97},
    {0x0C,0x01},
    {0x0D,0x4D},
    {0x0E,0x00},
    {0x0F,0xC4},
    {0x10,0x06},
    {0x11,0x4C},
    {0x12,0x03},
    {0x13,0x91},
    {0x25,0x00},
    {0x26,0xC3},
    {0x29,0x98},
    {0x33,0x01},
    {0x34,0x02},
    {0x41,0x0B},
    {0x42,0x04},
    {0x03,0x00},
    {0x40,0x10},
    {0x40,0x00},
    {0x03,0x01},
    {0x26,0x03}
};
#define NUM_OF_720P_INIT_REGS (sizeof(sensor_720P_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_1080P_lvds_init_regs[] = {
    {0x03,0x00},
    {0x05,0x03},
    {0x06,0x08},
    {0x07,0x97},
    {0x25,0x09},
    {0x26,0x8B},
    {0x33,0x01},
    {0x34,0x02},
    {0x36,0xC8},
    {0x38,0x48},
    {0x3A,0x22},
    {0x40,0x10},
    {0x03,0x01},
    {0x19,0xC3},
    {0x03,0x01},
    {0x26,0x03},
    {0x03,0x03},
    {0x05,0x04},
    {0x06,0x04},
    {0x07,0x00},
    {0x08,0x00},
    {0x13,0x03},
    {0x03,0x00},
    {0x40,0x00},
    {0x03,0x03},
    {0x04,0x02},
    {DELAY_CODE,10},
    {0x04,0x42},
#if 0    
    {0x03,0x01},
    {0xb6,0x88},
    {0x03,0x02},
    {0x1D,0x3F},
    {0x1F,0x42},
    {0x03,0x01},
#endif    
};
#define NUM_OF_1080P_LVDS_INIT_REGS (sizeof(sensor_1080P_lvds_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_720P_lvds_init_regs[] = {
    {0x03,0x00},
    {0x05,0x03},
    {0x06,0x08},
    {0x07,0x97},
    {0x0C,0x01},
    {0x0D,0x4D},
    {0x0E,0x00},
    {0x0F,0xC4},
    {0x10,0x06},
    {0x11,0x4C},
    {0x12,0x03},
    {0x13,0x91},
    {0x25,0x00},
    {0x26,0xC3},
    {0x29,0x98},
    {0x33,0x01},
    {0x34,0x02},
    {0x41,0x0B},
    {0x42,0x04},
    {0x03,0x00},
    {0x40,0x10},
    {0x40,0x00},
    {0x03,0x01},
    {0x26,0x03}
};
#define NUM_OF_720P_LVDS_INIT_REGS (sizeof(sensor_720P_lvds_init_regs) / sizeof(sensor_reg_t))

typedef struct _gain_set
{
    u16 ad_gain;
    u16 gain_x;     // UFIX7.6
} gain_set_t;

static gain_set_t gain_table[] =
{
    {  0,   64},  
    {  1,   68},  
    {  2,   72},  
    {  3,   76},  
    {  4,   80},  
    {  5,   84},  
    {  6,   88},  
    {  7,   92},  
    {  8,   96},  
    {  9,  100}, 
    { 10,  104}, 
    { 11,  108}, 
    { 12,  112}, 
    { 13,  116}, 
    { 14,  120}, 
    { 15,  124}, 
    { 16,  128}, 
    { 17,  136}, 
    { 18,  144}, 
    { 19,  152}, 
    { 20,  160}, 
    { 21,  168}, 
    { 22,  176}, 
    { 23,  184}, 
    { 24,  192}, 
    { 25,  200}, 
    { 26,  208}, 
    { 27,  216}, 
    { 28,  224}, 
    { 29,  232}, 
    { 30,  240}, 
    { 31,  248}, 
    { 32,  256}, 
    { 33,  272}, 
    { 34,  288}, 
    { 35,  304}, 
    { 36,  320}, 
    { 37,  336}, 
    { 38,  352}, 
    { 39,  368}, 
    { 40,  384}, 
    { 41,  400}, 
    { 42,  416}, 
    { 43,  432}, 
    { 44,  448}, 
    { 45,  464}, 
    { 46,  480}, 
    { 47,  496}, 
    { 48,  512}, 
    { 49,  544}, 
    { 50,  576}, 
    { 51,  608}, 
    { 52,  640}, 
    { 53,  672}, 
    { 54,  704}, 
    { 55,  736}, 
    { 56,  768}, 
    { 57,  800}, 
    { 58,  832}, 
    { 59,  864}, 
    { 60,  896}, 
    { 61,  928}, 
    { 62,  960}, 
    { 63,  992}, 
    { 64, 1024},
    { 65, 1088},
    { 66, 1152},
    { 67, 1216},
    { 68, 1280},
    { 69, 1344},
    { 70, 1408},
    { 71, 1472},
    { 72, 1536},
    { 73, 1600},
    { 74, 1664},
    { 75, 1728},
    { 76, 1792},
    { 77, 1856},
    { 78, 1920},
    { 79, 1984},
    { 80, 2048},
    { 81, 2176},
    { 82, 2304},
    { 83, 2432},
    { 84, 2560},
    { 85, 2688},
    { 86, 2816},
    { 87, 2944},
    { 88, 3072},
    { 89, 3200},
    { 90, 3328},
    { 91, 3456},
    { 92, 3584},
    { 93, 3712},
    { 94, 3840},
    { 95, 3968},
    { 96, 4096},
    { 97, 4352},
    { 98, 4608},
    { 99, 4864},
    {100, 5120},
    {101, 5376},
    {102, 5632},
    {103, 5888},
    {104, 6144},
    {105, 6400},
    {106, 6656},
    {107, 6912},
    {108, 7168},
    {109, 7424},
    {110, 7680},
    {111, 7936},
};
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

//=============================================================================
// I2C
//=============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            0xEE >> 1
#include "i2c_common.c"

//=============================================================================
// internal functions
//=============================================================================
static int read_reg(u32 addr, u32 *pval)
{
    struct i2c_msg  msgs[2];
    unsigned char   tmp[2], tmp2[2];


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
    unsigned char   buf[3];


    buf[0]     = addr & 0xFF;
    buf[1]     = val & 0xFF;
    msgs.addr  = I2C_ADDR;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = buf;

    //printk("write_reg 0x%02x 0x%02x\n", buf[0], buf[1]);

    return sensor_i2c_transfer(&msgs, 1);
}

static u32 get_pclk(u32 xclk)
{
    u32 pclk;

    if (g_psensor->img_win.height <= 1080){
        pclk = 74250000;
    }
    else{
        pclk = 72000000;
    }
    isp_info("pclk(%d) XCLK(%d)\n", pclk, xclk);

    return pclk;
}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    g_psensor->pclk = get_pclk(g_psensor->xclk);

    // trow unit is 1/10 us
    pinfo->t_row = (pinfo->HMax * 10000) / (g_psensor->pclk / 1000);
}

static void adjust_blanking(int fps)
{
    u32 vmax;

    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    vmax = 1124;
    if (fps > 30)
        fps = 30;
    if (fps < 5)
        fps = 5;
    
    pinfo->VMax = vmax*30/fps;

	write_reg(0x03, 0x00);
    write_reg(0x09, (pinfo->VMax & 0xFF));
    write_reg(0x08, ((pinfo->VMax >> 8) & 0xFF));
    write_reg(0x03, 0x01);

    calculate_t_row();

    isp_info("adjust_blanking, fps=%d\n", fps);
}

static void update_bayer_type(void);
static int set_mirror(int enable);
static int set_flip(int enable);

static int set_size(u32 width, u32 height)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    sensor_reg_t* pInitTable;
    u32 InitTable_Num;
    int i, ret = 0;
	isp_info("w=%d, h=%d\n", width, height);
    // check size
    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("Exceed maximum sensor resolution!\n");
        return -1;
    }

    pinfo->HMax = 2199;
    pinfo->VMax = 1124;
    if (interface == IF_PARALLEL){
        if (width <= 1280 && height<= 720 ) {
            pInitTable=sensor_720P_init_regs;
            InitTable_Num=NUM_OF_720P_INIT_REGS;
        }
        else{
            pInitTable=sensor_1080P_init_regs;
            InitTable_Num=NUM_OF_1080P_INIT_REGS;
        }
    }
    else{
        if (width <= 1280 && height<= 720 ) {
            pInitTable=sensor_720P_lvds_init_regs;
            InitTable_Num=NUM_OF_720P_LVDS_INIT_REGS;
        }
        else{
            pInitTable=sensor_1080P_lvds_init_regs;
            InitTable_Num=NUM_OF_1080P_LVDS_INIT_REGS;
        }
    }    

    isp_info("start initial...\n");

    // set initial registers
    for (i=0; i<InitTable_Num; i++) {

         if (pInitTable[i].addr == DELAY_CODE) {
            mdelay(pInitTable[i].val);
         }
         else if(write_reg(pInitTable[i].addr, pInitTable[i].val) < 0) {
                isp_err("%s init fail!!", SENSOR_NAME);
                return -EINVAL;
         }
     }

    set_mirror(pinfo->mirror);
    set_flip(pinfo->flip);
    update_bayer_type();

    // update sensor information
    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    adjust_blanking(g_psensor->fps);

    if (width<=1280 && height<=720 ) {
        g_psensor->out_win.width = 1280;
        g_psensor->out_win.height = 720;
        g_psensor->img_win.x = (1280 - width) / 2;
        g_psensor->img_win.y = (720 - height) / 2;
        isp_info("1080P,Sensor VMAX=0x%x(%d) , HMAX=0x%x(%d) \n",
            pinfo->VMax ,pinfo->VMax ,pinfo->HMax,pinfo->HMax );
    } else {
        g_psensor->out_win.width = 1920;
        g_psensor->out_win.height = 1080;
        g_psensor->img_win.x = (1920 - width) / 2;
        g_psensor->img_win.y = 0 + (1080 - height) / 2;
        isp_info("WUXGA, Sensor VMAX=0x%x(%d) , HMAX=0x%x(%d) \n",
                pinfo->VMax ,pinfo->VMax ,pinfo->HMax,pinfo->HMax );
    }
    return ret;
}

static u32 get_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h;
    u32 num_of_line;

    if (!g_psensor->curr_exp) {
        read_reg(0xc1, &reg_l);
        read_reg(0xc0, &reg_h);
        num_of_line = (((reg_h & 0xFF) << 8) | (reg_l & 0xFF))+1;
        g_psensor->curr_exp = (num_of_line * pinfo->t_row + 500) / 1000;
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 exp_line;
    int ret = 0;

//    isp_info("set_exp %d\n", exp);
    exp_line = MAX(2, (exp * 1000 + pinfo->t_row / 2 ) / pinfo->t_row);

    if (exp_line < pinfo->VMax-3){
        if (pinfo->long_exp){
	        write_reg(0x03, 0x00);
            write_reg(0x09, (pinfo->VMax & 0xFF));
            write_reg(0x08, ((pinfo->VMax >> 8) & 0xFF));
	        write_reg(0x03, 0x01);
            pinfo->long_exp = 0;
        }
    }
    else{
        write_reg(0x03, 0x00);
        write_reg(0x09, (exp_line & 0xFF));
        write_reg(0x08, ((exp_line >> 8) & 0xFF));
        write_reg(0x03, 0x01);
        pinfo->long_exp = 1;
    }
    write_reg(0xc1, ((exp_line-3) & 0xFF));
    write_reg(0xc0, (((exp_line-3) >> 8) & 0xFF));
        
    g_psensor->curr_exp = MAX(1, (exp_line * pinfo->t_row + 500) / 1000);

    return ret;
}

static u32 get_gain(void)
{
    u32 reg, i;

    if (!g_psensor->curr_gain) {
        read_reg(0xc3, &reg);
        for (i=0; i<NUM_OF_GAINSET; i++) {
            if (gain_table[i].ad_gain == reg)
                break;
        }
        if (i < NUM_OF_GAINSET)
            g_psensor->curr_gain = gain_table[i].gain_x;
    }

    return g_psensor->curr_gain;
}

static u32 get_a_gain(void);
static u32 get_d_gain(void);

static int set_gain(u32 gain)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
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

    reg = gain_table[i-1].ad_gain;

    write_reg(0xc3, reg);

    g_psensor->curr_gain = gain_table[i-1].gain_x;

    pinfo->curr_a_gain = 0;
    get_a_gain();
    pinfo->curr_d_gain = 0;
    get_d_gain();

    return ret;
}

static u32 get_a_gain(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    pinfo->curr_a_gain = get_gain();
    return pinfo->curr_a_gain;

}

static int set_a_gain(u32 gain)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (gain > pinfo->max_a_gain)
        gain = pinfo->max_a_gain;
    pinfo->curr_a_gain = set_gain(gain);
    return pinfo->curr_a_gain;
}
static u32 get_d_gain(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    if (pinfo->curr_d_gain == 0) {        
        read_reg(0xc4, &reg);   //  max 3.984x
        pinfo->curr_d_gain = reg;
    }
    return pinfo->curr_d_gain;
}

static int set_d_gain(u32 gain)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (gain > pinfo->max_d_gain)
        gain = pinfo->max_d_gain;
    write_reg(0xc4, gain);
    pinfo->curr_d_gain = gain;

    return pinfo->curr_d_gain;
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

	write_reg(0x03, 0x00);
	read_reg(0x05, &reg);
	write_reg(0x03, 0x01);

	return reg & 0x01;
}

static int set_mirror(int enable)
{
    	sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
	u32 reg;

	pinfo->mirror = enable;
	write_reg(0x03, 0x00);
	read_reg(0x05, &reg);
    if (pinfo->flip)
		reg |= 0x02;
	else
		reg &= ~0x02;
	if (enable)
		reg |= 0x01;
	else
		reg &= ~0x01;
	write_reg(0x05, reg);
	write_reg(0x03, 0x01);

   	return 0;
}

static int get_flip(void)
{
	u32 reg;

	write_reg(0x03, 0x00);
	read_reg(0x05, &reg);
	write_reg(0x03, 0x01);

	return reg & 0x02;
}

static int set_flip(int enable)
{
    	sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
	u32 reg;

	pinfo->flip = enable;
	write_reg(0x03, 0x00);
	read_reg(0x05, &reg);
    if (pinfo->mirror)
		reg |= 0x01;
    else
		reg &= ~0x01;
    if (enable)
		reg |= 0x02;
	else
		reg &= ~0x02;
	write_reg(0x05, reg);
	write_reg(0x03, 0x01);

   	return 0;
}

static int set_fps(int fps)
{
    	adjust_blanking(fps);
    	g_psensor->fps = fps;

    	//isp_info("pdev->fps=%d\n",pdev->fps);

	return 0;
}

static int get_property(enum SENSOR_CMD cmd, unsigned long arg)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
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
        case ID_A_GAIN:
            *(int*)arg = get_a_gain();
            break;
        case ID_MIN_A_GAIN:
            *(int*)arg = pinfo->min_a_gain;
            break;
        case ID_MAX_A_GAIN:
            *(int*)arg = pinfo->max_a_gain;
            break;
        case ID_D_GAIN:
            *(int*)arg = get_d_gain();
            break;
        case ID_MIN_D_GAIN:
            *(int*)arg = pinfo->min_d_gain;
            break;
        case ID_MAX_D_GAIN:
            *(int*)arg = pinfo->max_d_gain;
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
	     update_bayer_type();
            break;
        case ID_FLIP:
            set_flip((int)arg);
	     update_bayer_type();
            break;
        case ID_FPS:
            set_fps((int)arg);
            break;
        case ID_A_GAIN:
            ret = set_a_gain((int)arg);
            break;
        case ID_D_GAIN:
            ret = set_d_gain((int)arg);
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
    int ret = 0;

    if (pinfo->is_init)
        return 0;

    isp_info("PS1210 init\n");
    if (interface == IF_PARALLEL)
        isp_info("Interface : Parallel\n");
    else    
        isp_info("Interface : LVDS\n");
    pinfo->mirror = mirror;
    pinfo->flip = flip;

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
static void ps1210_deconstruct(void)
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

static int ps1210_construct(u32 xclk, u16 width, u16 height)
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
    g_psensor->bayer_type = BAYER_GB;
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
    g_psensor->exp_latency = 0;
    g_psensor->gain_latency = 0;
    g_psensor->fps = fps;
    g_psensor->interface = interface;
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
    pinfo->min_a_gain = gain_table[0].gain_x;
    pinfo->max_a_gain = gain_table[NUM_OF_GAINSET - 1].gain_x;
    pinfo->curr_a_gain = 0;
    pinfo->min_d_gain = 64;
    pinfo->max_d_gain = 255;
    pinfo->curr_d_gain = 0;

    if ((ret = sensor_init_i2c_driver()) < 0)
        goto construct_err;

    if ((ret = init()) < 0)
        goto construct_err;

    return 0;

construct_err:
    ps1210_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init ps1210_init(void)
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

    if ((ret = ps1210_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit ps1210_exit(void)
{
    isp_unregister_sensor(g_psensor);
    ps1210_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(ps1210_init);
module_exit(ps1210_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor PS1210");
MODULE_LICENSE("GPL");
