/**
 * @file sen_imx076.c
 * Sony IMX076 sensor driver
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <asm/io.h>

#define PFX "sen_imx076"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"
#include "spi_common.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1280
#define DEFAULT_IMAGE_HEIGHT    720
#define DEFAULT_XCLK            54000000

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

static uint inv_clk = 1;
module_param(inv_clk, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "invert clock, 0:Disable, 1: Enable");

//=============================================================================
// global
//============================================================================
#define SENSOR_NAME         "IMX076"
#define SENSOR_MAX_WIDTH    1500
#define SENSOR_MAX_HEIGHT   1000
static sensor_dev_t *g_psensor = NULL;
#define H_CYCLE 1650
#define V_CYCLE 1090

typedef struct sensor_info {
    int is_init;
    u32 HMax;
    u32 VMax;
    u32 t_row;          // unit: 1/10 us
    int htp;
    int mirror;
    int flip;
} sensor_info_t;

typedef struct sensor_reg {
    u8 id;
    u16 addr;
    u16 val;
} sensor_reg_t;

static sensor_reg_t sensor_init_regs[] = {
    {0x02, 0x00, 0x00},     //Standby, Invalid Register Write
    {0x02, 0x01, 0x03},     // WinMode[6:4], Flip[0]
    {0x02, 0x02, 0x00},     // All-pixel out
    //{0x02, 0x02, 0x01},     // HD720p
    {0x02, 0x03, 0x72},     // HMax
    {0x02, 0x04, 0x06},     // HMax
    {0x02, 0x05, 0x42},     // VMax
    {0x02, 0x06, 0x04},     // VMax
    //{0x02, 0x05, 0xEE},     // VMax
    //{0x02, 0x06, 0x02},     // VMax
    {0x02, 0x07, 0x00},     // Storage Time (Low Speed Shutter)
    {0x02, 0x08, 0x00},     // Storage Time (Low Speed Shutter)
    {0x02, 0x09, 0x00},     // SHS1
    {0x02, 0x0A, 0x00},     // SHS1
    {0x02, 0x0B, 0x00},
    {0x02, 0x0C, 0x00},
    {0x02, 0x0D, 0x00},
    {0x02, 0x0E, 0x00},
    {0x02, 0x0F, 0x00},
    {0x02, 0x10, 0x00},
    //{0x02, 0x11, 0x88},
    {0x02, 0x11, 0x81},
    //{0x02, 0x12, 0x81},     // AD Bit = 10 bit
    {0x02, 0x12, 0x83},     // AD Bit = 12 bit
    {0x02, 0x13, 0x00},
    {0x02, 0x14, 0x00},     // Window Cropping
    {0x02, 0x15, 0x00},     // Window Cropping
    {0x02, 0x16, 0x00},
    {0x02, 0x17, 0x00},
    {0x02, 0x18, 0x58},     // Window Crop Size (H)
    {0x02, 0x19, 0x05},     // Window Crop Size (H)
    {0x02, 0x1A, 0x19},     // Window Crop Size (V)
    {0x02, 0x1B, 0x04},     // Window Crop Size (V)
    {0x02, 0x1C, 0x50},
    {0x02, 0x1D, 0x00},
    {0x02, 0x1E, 0x00},     // Gain Setting
    {0x02, 0x1F, 0x31},
    {0x02, 0x20, 0x3C},     // Black level offset (LSB)
    {0x02, 0x21, 0x00},     // Black level offset (MSB) / XHS
    {0x02, 0x22, 0x00},
    {0x02, 0x23, 0x08},
    {0x02, 0x24, 0x30},
    {0x02, 0x25, 0x00},
    {0x02, 0x26, 0x80},
    {0x02, 0x27, 0x21},
    {0x02, 0x28, 0x34},
    {0x02, 0x29, 0x63},
    {0x02, 0x2A, 0x00},
    {0x02, 0x2B, 0x00},
    {0x02, 0x2C, 0x00},     // XMSTA
    {0x02, 0x2D, 0x42},     // DCK phase delay / BITSEL
    {0x02, 0x2E, 0x00},
    {0x02, 0x2F, 0x02},
    {0x02, 0x30, 0x30},
    {0x02, 0x31, 0x20},
    {0x02, 0x32, 0x00},
    {0x02, 0x33, 0x14},
    {0x02, 0x34, 0x20},
    {0x02, 0x35, 0x60},     // SCPRD
    {0x02, 0x36, 0x00},
    {0x02, 0x37, 0x23},
    {0x02, 0x38, 0x01},
    {0x02, 0x39, 0x00},
    {0x02, 0x3A, 0xA8},
    {0x02, 0x3B, 0xE0},
    {0x02, 0x3C, 0x06},     // VNDMY

    {0x02, 0x40, 0x43},

    {0x02, 0x75, 0x00},
    {0x02, 0x76, 0x00},

    {0x03, 0x07, 0x0A},
    {0x03, 0x0A, 0x35},
    {0x03, 0x0B, 0x3B},
    {0x03, 0x12, 0x0C},

    {0x03, 0x16, 0x77},
    {0x03, 0x17, 0x40},

    {0x03, 0x28, 0x00},

    {0x03, 0x73, 0x28},

    {0x03, 0x7D, 0x84},
    {0x03, 0x7E, 0x30},

    {0x03, 0x85, 0x60},
    {0x03, 0x86, 0xC2},
    {0x03, 0x87, 0x15},
    {0x03, 0x88, 0xFF},
    {0x03, 0x89, 0x58},
    {0x03, 0x8A, 0xCB},
    {0x03, 0x8B, 0x28},
    {0x03, 0x8C, 0x72},
    {0x03, 0x8D, 0xA1},
    {0x03, 0x8F, 0xBB}
};
#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))

typedef struct gain_set {
    u8  ad_reg;
    u32 gain_x;
} gain_set_t;

static const gain_set_t gain_table[] = {
    {0x0,	 64},  {0x1,     66}, {0x2,	    69},  {0x3,     71},  {0x4,     73},
    {0x5,	 76},  {0x6,     79}, {0x7,	    82},  {0x8,     84},  {0x9,     87},
    {0xA,	 90},  {0xB,     94}, {0xC,	    97},  {0xD,    100},  {0xE,    104},
    {0xF,	107},  {0x10,	111}, {0x11,   115},  {0x12,   119},  {0x13,   123},
    {0x14,	128},  {0x15,	132}, {0x16,   137},  {0x17,   142},  {0x18,   147},
    {0x19,	152},  {0x1A,	157}, {0x1B,   163},  {0x1C,   168},  {0x1D,   174},
    {0x1E,	180},  {0x1F,	187}, {0x20,   193},  {0x21,   200},  {0x22,   207},
    {0x23,	214},  {0x24,	222}, {0x25,   230},  {0x26,   238},  {0x27,   246},
    {0x28,	255},  {0x29,	264}, {0x2A,   273},  {0x2B,   283},  {0x2C,   293},
    {0x2D,	303},  {0x2E,	313}, {0x2F,   324},  {0x30,   336},  {0x31,   348},
    {0x32,	360},  {0x33,	373}, {0x34,   386},  {0x35,   399},  {0x36,   413},
    {0x37,	428},  {0x38,	443}, {0x39,   458},  {0x3A,   474},  {0x3B,   491},
    {0x3C,	508},  {0x3D,	526}, {0x3E,   545},  {0x3F,   564},  {0x40,   584},
    {0x41,	604},  {0x42,	625}, {0x43,   647},  {0x44,   670},  {0x45,   694},
    {0x46,	718},  {0x47,	743}, {0x48,   769},  {0x49,   796},  {0x4A,   824},
    {0x4B,	853},  {0x4C,	883}, {0x4D,   914},  {0x4E,   947},  {0x4F,   980},
    {0x50,  1014}, {0x51,  1050}, {0x52,  1087},  {0x53,  1125},  {0x54,  1165},
    {0x55,  1206}, {0x56,  1248}, {0x57,  1292},  {0x58,  1337},  {0x59,  0764},
    {0x5A,  1433}, {0x5B,  1483}, {0x5C,  1535},  {0x5D,  1589},  {0x5E,  1645},
    {0x5F,  1703}, {0x60,  1763}, {0x61,  1825},  {0x62,  1889},  {0x63,  1955},
    {0x64,  2024}, {0x65,  2095}, {0x66,  2169},  {0x67,  2245},  {0x68,  2324},
    {0x69,  2405}, {0x6A,  2490}, {0x6B,  2577},  {0x6C,  2668},  {0x6D,  2762},
    {0x6E,  2859}, {0x6F,  2959}, {0x70,  3063},  {0x71,  3171},  {0x72,  3282},
    {0x73,  3398}, {0x74,  3517}, {0x75,  3641},  {0x76,  3769},  {0x77,  3901},
    {0x78,  4038}, {0x79,  4180}, {0x7A,  4327},  {0x7B,  4479},  {0x7C,  4636},
    {0x7D,  4799}, {0x7E,  4968}, {0x7F,  5143},  {0x80,  5323},  {0x81,  5510},
    {0x82,  5704}, {0x83,  5904}, {0x84,  6112},  {0x85,  6327},  {0x86,  6549},
    {0x87,  6779}, {0x88,  7017}, {0x89,  7264},  {0x8A,  7519},  {0x8B,  7784},
    {0x8C,  8057},
};

#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

//============================================================================
// SPI
//============================================================================
#define SPI_CLK_SET()   isp_api_spi_clk_set_value(1)
#define SPI_CLK_CLR()   isp_api_spi_clk_set_value(0)
#define SPI_DI_READ()   isp_api_spi_di_get_value()
#define SPI_DO_SET()    isp_api_spi_do_set_value(1)
#define SPI_DO_CLR()    isp_api_spi_do_set_value(0)
#define SPI_CS_SET()    isp_api_spi_cs_set_value(1)
#define SPI_CS_CLR()    isp_api_spi_cs_set_value(0)

//============================================================================
// internal functions
//============================================================================
static void _delay(void)
{
    int i;
    for (i=0; i<0x1f; i++);
}

static int imx076_read_reg(u8 id, u8 addr, u32 *pval)
{
    int i;
    u32 tmp;

    _delay();
    SPI_CS_CLR();
    _delay();

    // address phase
    tmp = (addr << 8) | (0x80 | id);
    for (i=0; i<16; i++) {
        SPI_CLK_CLR();
        _delay();
        if (tmp & BIT0)
            SPI_DO_SET();
        else
            SPI_DO_CLR();
        tmp >>= 1;
        SPI_CLK_SET();
        _delay();
    }

    // data phase
    *pval = 0;
    for (i=0; i<8; i++) {
        SPI_CLK_CLR();
        _delay();
        SPI_CLK_SET();
        _delay();
        if (SPI_DI_READ() & BIT0)
            *pval |= (1 << i);
    }
    SPI_CS_SET();

    return 0;
}

static int imx076_write_reg(u8 id, u8 addr, u32 val)
{
    int i;
    u32 tmp;

    _delay();
    SPI_CS_CLR();
    _delay();

    // address phase & data phase
    tmp = (val << 16) | (addr << 8) | id;
    for (i=0; i<24; i++) {
        SPI_CLK_CLR();
        _delay();
        if (tmp & BIT0)
            SPI_DO_SET();
        else
            SPI_DO_CLR();
        tmp >>= 1;
        SPI_CLK_SET();
        _delay();
    }
    SPI_CS_SET();

    return 0;
}

static int read_reg(u32 addr, u32 *pval)
{
    u8 id, r_addr;

    id = (addr >> 8) & 0xFF;
    r_addr = addr & 0xFF;
    return imx076_read_reg(id, r_addr, pval);
}

static int write_reg(u32 addr, u32 val)
{
    u8 id, r_addr;

    id = (addr >> 8) & 0xFF;
    r_addr = addr & 0xFF;
    return imx076_write_reg(id, r_addr, val);
}

static u32 get_pclk(u32 xclk)
{
    u32 pclk = 54000000;

    printk("**imx076 pclk** = %d\n", pclk);
    return pclk;
}


static int set_size(u32 width, u32 height)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 val_l, val_h;

    if ((width > 1280) || (height > 1024))
        return -EINVAL;

    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    g_psensor->out_win.width = H_CYCLE;
    g_psensor->out_win.height = V_CYCLE;

    g_psensor->img_win.x = (150 + ((1280 - width) >> 1)) &~BIT0;
    g_psensor->img_win.y = (44 + ((1024 - height) >> 1)) &~BIT0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    imx076_read_reg(0x2, 0x03, &val_l);
    imx076_read_reg(0x2, 0x04, &val_h);
    pinfo->HMax = (val_h << 8) | val_l;

    imx076_read_reg(0x2, 0x05, &val_l);
    imx076_read_reg(0x2, 0x06, &val_h);
    pinfo->VMax = (val_h << 8) | val_l;

    g_psensor->pclk = get_pclk(g_psensor->xclk);
    pinfo->t_row = (pinfo->HMax * 10000) / (g_psensor->pclk / 1000);
    g_psensor->min_exp = (pinfo->t_row + 500) / 1000;

    return ret;
}

static u32 get_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 val_l, val_h;
    u32 lines, vmax, shs1;

    if (!g_psensor->curr_exp) {
        imx076_read_reg(0x2, 0x05, &val_l);
        imx076_read_reg(0x2, 0x06, &val_h);
        vmax = (val_h << 8) | val_l;

        imx076_read_reg(0x2, 0x09, &val_l);
        imx076_read_reg(0x2, 0x0A, &val_h);
        shs1 = (val_h << 8) | val_l;

        // exposure = (VMAX - SHS1 - 0.3)H, skip 0.3H
        lines = vmax - shs1;
        g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 lines, shs1;

    lines = ((exp * 1000)+pinfo->t_row/2) / pinfo->t_row;

    if (lines == 0)
        lines = 1;

    if (lines > pinfo->VMax) {
        imx076_write_reg(0x2, 0x05, (lines & 0xFF));
        imx076_write_reg(0x2, 0x06, ((lines >> 8) & 0xFF));
        imx076_write_reg(0x2, 0x09, 0x0);
        imx076_write_reg(0x2, 0x0A, 0x0);
    } else {
        imx076_write_reg(0x2, 0x05, (pinfo->VMax & 0xFF));
        imx076_write_reg(0x2, 0x06, ((pinfo->VMax >> 8) & 0xFF));
        shs1 = pinfo->VMax - lines;
        imx076_write_reg(0x2, 0x09, (shs1 & 0xFF));
        imx076_write_reg(0x2, 0x0A, ((shs1 >> 8) & 0xFF));
	}
    g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;

    return ret;
}

static u32 get_gain(void)
{
    u32 ad_gain;

    if (g_psensor->curr_gain == 0) {

        imx076_read_reg(0x2, 0x1E, &ad_gain);
        g_psensor->curr_gain = gain_table[ad_gain].gain_x;
    }

    return g_psensor->curr_gain;
}

static int set_gain(u32 gain)
{
    int ret = 0, i;
    u32 ad_gain;

    // search most suitable gain into gain table
    ad_gain = gain_table[0].ad_reg;

    for (i=0; i<NUM_OF_GAINSET; i++) {
        if (gain_table[i].gain_x > gain)
            break;
    }
    if (i > 0)
        ad_gain = gain_table[i-1].ad_reg;

    imx076_write_reg(0x2, 0x1E, ad_gain);
    g_psensor->curr_gain = gain_table[i-1].gain_x;

    return ret;
}

static int get_mirror(void)
{
    u32 reg;

    imx076_read_reg(0x2, 0x01, &reg);
    return (reg & BIT1) ? 1 : 0;
}

static int set_mirror(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    imx076_read_reg(0x2, 0x01, &reg);
    if (enable)
        reg |= BIT1;
    else
        reg &=~BIT1;
    imx076_write_reg(0x2, 0x01, reg);
    pinfo->mirror = enable;

    return 0;
}

static int get_flip(void)
{
    u32 reg;

    imx076_read_reg(0x2, 0x01, &reg);
    return (reg & BIT0) ? 1 : 0;
}

static int set_flip(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    imx076_read_reg(0x2, 0x01, &reg);
    if (enable)
        reg |= BIT0;
    else
        reg &=~BIT0;
    imx076_write_reg(0x2, 0x01, reg);
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
    int i;

    if (pinfo->is_init)
        return 0;

    for (i=0; i<NUM_OF_INIT_REGS; i++) {
        if (imx076_write_reg(sensor_init_regs[i].id, sensor_init_regs[i].addr, sensor_init_regs[i].val) < 0) {
            isp_err("%s init fail!!", SENSOR_NAME);
            return -EINVAL;
        }
    }

    // get mirror and flip status
    pinfo->mirror = get_mirror();
    pinfo->flip = get_flip();

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

    pinfo->is_init = true;

    return ret;
}

//=============================================================================
// external functions
//=============================================================================
static void imx076_deconstruct(void)
{
    if ((g_psensor) && (g_psensor->private)) {
        kfree(g_psensor->private);
    }

    isp_api_spi_exit();

    // turn off EXT_CLK
    isp_api_extclk_onoff(0);
    // release CAP_RST
    isp_api_cap_rst_exit();
}

static int imx076_construct(u32 xclk, u16 width, u16 height)
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
        isp_err("failed to allocate private data!");
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
    g_psensor->img_win.x = 150;
    g_psensor->img_win.y = 150;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;
    g_psensor->min_exp = 1;
    g_psensor->max_exp = 5000; // 0.5 sec
    g_psensor->min_gain = 64;
    g_psensor->max_gain = 8057;
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


    ret = isp_api_spi_init();
    if (ret)
        return ret;

    if ((ret = init()) < 0)
        goto construct_err;

    return 0;

construct_err:
    imx076_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init imx076_init(void)
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

    if ((ret = imx076_construct(sensor_xclk, sensor_w, sensor_h)) < 0){
        printk("imx076_construct Error \n");
        goto init_err2;
        }

    // register sensor device to ISP driver
    if ((ret = isp_register_sensor(g_psensor)) < 0){
        printk("isp_register_sensor Error \n");
        goto init_err2;
     }

    return ret;

init_err2:
    kfree(g_psensor);

init_err1:
    return ret;
}

static void __exit imx076_exit(void)
{
    isp_unregister_sensor(g_psensor);
    imx076_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(imx076_init);
module_exit(imx076_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor IMX076");
MODULE_LICENSE("GPL");
