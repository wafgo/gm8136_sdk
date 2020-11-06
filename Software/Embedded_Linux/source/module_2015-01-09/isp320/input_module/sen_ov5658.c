/**
 * @file sen_ov5658.c
 * OmniVision OV5658 sensor driver
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <asm/io.h>

#define PFX "sen_ov5658"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     2592
#define DEFAULT_IMAGE_HEIGHT    1944
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
#define SENSOR_NAME         "OV5658"
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
    {0x0103, 0x01},
    {0x3210, 0x43},
    {0x3001, 0x0e},
    {0x3002, 0xc0},
    {0x3011, 0x41},
    {0x3012, 0x0a},
    {0x3013, 0x50},
    {0x3015, 0x09},
    {0x3018, 0xf0},
    {0x3021, 0x40},
    {0x3500, 0x00},
    {0x3501, 0x7b},
    {0x3502, 0x00},
    {0x3503, 0x07},
    {0x3505, 0x00},
    {0x3506, 0x00},
    {0x3507, 0x02},
    {0x3508, 0x00},
    {0x3509, 0x10},
    {0x350a, 0x00},
    {0x350b, 0x80},
    {0x3600, 0x4b},
    {0x3602, 0x3c},
    {0x3605, 0x14},
    {0x3606, 0x09},
    {0x3612, 0x04},
    {0x3613, 0x66},
    {0x3614, 0x39},
    {0x3615, 0x33},
    {0x3616, 0x46},
    {0x361a, 0x0a},
    {0x361c, 0x76},
    {0x3620, 0x40},
    {0x3640, 0x03},
    {0x3641, 0x60},
    {0x3642, 0x00},
    {0x3643, 0x90},
    {0x3660, 0x04},
    {0x3665, 0x00},
    {0x3666, 0x20},
    {0x3667, 0x00},
    {0x366a, 0x80},
    {0x3680, 0xe0},
    {0x3692, 0x80},
    {0x3700, 0x42},
    {0x3701, 0x14},
    {0x3702, 0xe8},
    {0x3703, 0x20},
    {0x3704, 0x5e},
    {0x3705, 0x02},
    {0x3708, 0xe3},
    {0x3709, 0xc3},
    {0x370a, 0x00},
    {0x370b, 0x20},
    {0x370c, 0x0c},
    {0x370d, 0x11},
    {0x370e, 0x00},
    {0x370f, 0x40},
    {0x3710, 0x00},
    {0x3715, 0x09},
    {0x371a, 0x04},
    {0x371b, 0x05},
    {0x371c, 0x01},
    {0x371e, 0xa1},
    {0x371f, 0x18},
    {0x3721, 0x00},
    {0x3726, 0x00},
    {0x372a, 0x01},
    {0x3730, 0x10},
    {0x3738, 0x22},
    {0x3739, 0xe5},
    {0x373a, 0x50},
    {0x373b, 0x02},
    {0x373c, 0x2c},
    {0x373f, 0x02},
    {0x3740, 0x42},
    {0x3741, 0x02},
    {0x3743, 0x01},
    {0x3744, 0x02},
    {0x3747, 0x00},
    {0x3754, 0xc0},
    {0x3755, 0x07},
    {0x3756, 0x1a},
    {0x3759, 0x0f},
    {0x375c, 0x04},
    {0x3767, 0x00},
    {0x376b, 0x44},
    {0x377b, 0x44},
    {0x377c, 0x30},
    {0x377e, 0x30},
    {0x377f, 0x08},
    {0x3781, 0x0c},
    {0x3785, 0x1e},
    {0x378f, 0xf5},
    {0x3791, 0xb0},
    {0x379c, 0x0c},
    {0x379d, 0x20},
    {0x379e, 0x00},
    {0x3796, 0x72},
    {0x379a, 0x07},
    {0x379b, 0xb0},
    {0x37c5, 0x00},
    {0x37c6, 0x00},
    {0x37c7, 0x00},
    {0x37c9, 0x00},
    {0x37ca, 0x00},
    {0x37cb, 0x00},
    {0x37cc, 0x00},
    {0x37cd, 0x00},
    {0x37ce, 0x01},
    {0x37cf, 0x00},
    {0x37d1, 0x00},
    {0x37de, 0x00},
    {0x37df, 0x00},
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x0a},
    {0x3805, 0x3f},
    {0x3806, 0x07},
    {0x3807, 0xa3},
    {0x3808, 0x0a},
    {0x3809, 0x20},
    {0x380a, 0x07},
    {0x380b, 0x98},
    {0x380c, 0x0c},
    {0x380d, 0x98},
    {0x380e, 0x07},
    {0x380f, 0xc0},
    {0x3810, 0x00},
    {0x3811, 0x10},
    {0x3812, 0x00},
    {0x3813, 0x06},
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3820, 0x10},
    {0x3821, 0x1e},
    {0x3823, 0x00},
    {0x3824, 0x00},
    {0x3825, 0x00},
    {0x3826, 0x00},
    {0x3827, 0x00},
    {0x3829, 0x0b},
    {0x382a, 0x04},
    {0x382c, 0x34},
    {0x382d, 0x00},
    {0x3a04, 0x06},
    {0x3a05, 0x14},
    {0x3a06, 0x00},
    {0x3a07, 0xfe},
    {0x3b00, 0x00},
    {0x3b02, 0x00},
    {0x3b03, 0x00},
    {0x3b04, 0x00},
    {0x3b05, 0x00},
    {0x4000, 0x18},
    {0x4001, 0x04},
    {0x4002, 0x45},
    {0x4004, 0x04},
    {0x4005, 0x18},
    {0x4006, 0x20},
    {0x4007, 0x98},
    {0x4008, 0x24},
    {0x4009, 0x10},
    {0x400c, 0x00},
    {0x400d, 0x00},
    {0x404e, 0x37},
    {0x404f, 0x8f},
    {0x4058, 0x00},
    {0x4100, 0x50},
    {0x4101, 0x34},
    {0x4102, 0x34},
    {0x4104, 0xde},
    {0x4300, 0xff},
    {0x4307, 0x30},
    {0x4311, 0x04},
    {0x4315, 0x01},
    {0x4501, 0x08},
    {0x4502, 0x08},
    {0x4816, 0x52},
    {0x481f, 0x30},
    {0x4826, 0x28},
    {0x4837, 0x0d},
    {0x4a00, 0xaa},
    {0x4a02, 0x00},
    {0x4a03, 0x01},
    {0x4a05, 0x40},
    {0x4a0a, 0x88},
    {0x5000, 0x06},
    {0x5001, 0x01},
    {0x5002, 0x00},
    {0x5003, 0x20},
    {0x5013, 0x00},
    {0x501f, 0x00},
    {0x5043, 0x48},
    {0x5780, 0x1c},
    {0x5786, 0x20},
    {0x5788, 0x18},
    {0x578a, 0x04},
    {0x578b, 0x02},
    {0x578c, 0x02},
    {0x578e, 0x06},
    {0x578f, 0x02},
    {0x5790, 0x02},
    {0x5791, 0xff},
    {0x5e00, 0x00},
    {0x5e10, 0x0c},
    {0x0100, 0x01},
    {0x5800, 0x22},
    {0x5801, 0x11},
    {0x5802, 0x0d},
    {0x5803, 0x0d},
    {0x5804, 0x12},
    {0x5805, 0x26},
    {0x5806, 0x09},
    {0x5807, 0x07},
    {0x5808, 0x05},
    {0x5809, 0x05},
    {0x580a, 0x07},
    {0x580b, 0x0a},
    {0x580c, 0x07},
    {0x580d, 0x02},
    {0x580e, 0x00},
    {0x580f, 0x00},
    {0x5810, 0x03},
    {0x5811, 0x07},
    {0x5812, 0x06},
    {0x5813, 0x02},
    {0x5814, 0x00},
    {0x5815, 0x00},
    {0x5816, 0x03},
    {0x5817, 0x07},
    {0x5818, 0x09},
    {0x5819, 0x05},
    {0x581a, 0x04},
    {0x581b, 0x04},
    {0x581c, 0x07},
    {0x581d, 0x09},
    {0x581e, 0x1d},
    {0x581f, 0x0e},
    {0x5820, 0x0b},
    {0x5821, 0x0b},
    {0x5822, 0x0f},
    {0x5823, 0x1e},
    {0x5824, 0x59},
    {0x5825, 0x46},
    {0x5826, 0x37},
    {0x5827, 0x36},
    {0x5828, 0x45},
    {0x5829, 0x39},
    {0x582a, 0x46},
    {0x582b, 0x44},
    {0x582c, 0x45},
    {0x582d, 0x28},
    {0x582e, 0x38},
    {0x582f, 0x52},
    {0x5830, 0x60},
    {0x5831, 0x51},
    {0x5832, 0x26},
    {0x5833, 0x38},
    {0x5834, 0x43},
    {0x5835, 0x42},
    {0x5836, 0x34},
    {0x5837, 0x18},
    {0x5838, 0x05},
    {0x5839, 0x27},
    {0x583a, 0x27},
    {0x583b, 0x27},
    {0x583c, 0x0a},
    {0x583d, 0xbf},
    {0x5842, 0x01},
    {0x5843, 0x2b},
    {0x5844, 0x01},
    {0x5845, 0x92},
    {0x5846, 0x01},
    {0x5847, 0x8f},
    {0x5848, 0x01},
    {0x5849, 0x0c},
};
#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_5M_sub_init_regs[] = {
    //;;;; key settings for 27Mhz input clock, sclk=97.2Mhz, MIPI_CLK=243Mhz, 30fps
    {0x3501, 0x7b},
    {0x3502, 0x00},
    {0x3600, 0x6c},
    {0x3601, 0x23},
    {0x3604, 0x24},
    {0x3708, 0xe3},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x0a},
    {0x3805, 0x3f},
    {0x3806, 0x07},
    {0x3807, 0xa3},
    {0x3808, 0x0a},
    {0x3809, 0x20},
    {0x380a, 0x07},
    {0x380b, 0x98},
    {0x380c, 0x0c},
    {0x380d, 0xc0},
    {0x380e, 0x07},
    {0x380f, 0xc0},
    {0x3810, 0x00},
    {0x3811, 0x10},
    {0x3812, 0x00},
    {0x3813, 0x06},
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3820, 0x10},
    {0x3821, 0x1e},
    {0x4502, 0x08},
    {0x4816, 0x52},
    {0x481f, 0x30},
    {0x4826, 0x28},
    {0x4837, 0x10},
    {0x5002, 0x00},
};
#define NUM_OF_5M_SUB_INIT_REGS (sizeof(sensor_5M_sub_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_3M_1536p_sub_init_regs[] = {
    //;;;; key settings for 27Mhz input clock, sclk=97.2Mhz, MIPI_CLK=243Mhz, 30fps
    {0x3501, 0x7b},
    {0x3502, 0x00},
    {0x3600, 0x6c},
    {0x3601, 0x23},
    {0x3604, 0x24},
    {0x3708, 0xe3},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x0a},
    {0x3805, 0x3f},
    {0x3806, 0x07},
    {0x3807, 0xa3},
    {0x3808, 0x08},
    {0x3809, 0x00},
    {0x380a, 0x06},
    {0x380b, 0x00},
    {0x380c, 0x0c},
    {0x380d, 0x98},
    {0x380e, 0x07},
    {0x380f, 0xd8},
    //;;;; key settings for output size 2048x1536, and scaling
    {0x3810, 0x00},
    {0x3811, 0x10},
    {0x3812, 0x00},
    {0x3813, 0x06},
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3820, 0x10},
    {0x3821, 0x1e},
    {0x4502, 0x08},
    {0x4816, 0x52},
    {0x481f, 0x30},
    {0x4826, 0x28},
    {0x4837, 0x10},
    {0x5002, 0x80},
};
#define NUM_OF_3M_1536P_SUB_INIT_REGS (sizeof(sensor_3M_1536p_sub_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_3M_sub_init_regs[] = {
    //;;;; key settings for 27Mhz input clock, sclk=97.2Mhz, MIPI_CLK=243Mhz, 30fps
    //;;;; key settings for output size 2304x1296, cropping(2592x1944->2592x1458) and scaling
    {0x3501, 0x7b},
    {0x3502, 0x00},
    {0x3600, 0x6c},
    {0x3601, 0x23},
    {0x3604, 0x24},
    {0x3708, 0xe3},
    {0x3802, 0x00},
    {0x3803, 0xf2},
    {0x3804, 0x0a},
    {0x3805, 0x3f},
    {0x3806, 0x06},
    {0x3807, 0xb1},
    {0x3808, 0x09},
    {0x3809, 0x00},
    {0x380a, 0x05},
    {0x380b, 0x10},
    {0x380c, 0x0c},
    {0x380d, 0x98},
    {0x380e, 0x07},
    {0x380f, 0xd8},
    {0x3810, 0x00},
    {0x3811, 0x10},
    {0x3812, 0x00},
    {0x3813, 0x06},
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3820, 0x10},
    {0x3821, 0x1e},
    {0x4502, 0x08},
    {0x4816, 0x52},
    {0x481f, 0x30},
    {0x4826, 0x28},
    {0x4837, 0x10},
    {0x5002, 0x80},
};
#define NUM_OF_3M_SUB_INIT_REGS (sizeof(sensor_3M_sub_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_1080p_sub_init_regs[] = {
    //;;;; key settings for 27Mhz input clock, sclk=97.2Mhz, MIPI_CLK=243Mhz, 30fps
    //;;;; key settings for output size 1920x1080, cropping(2592x1944->2592x1458) and scaling
    {0x3501, 0x7b},
    {0x3502, 0x00},
    {0x3600, 0x6c},
    {0x3601, 0x23},
    {0x3604, 0x24},
    {0x3708, 0xe3},
    {0x3802, 0x00},
    {0x3803, 0xf2},
    {0x3804, 0x0a},
    {0x3805, 0x3f},
    {0x3806, 0x06},
    {0x3807, 0xb1},
    {0x3808, 0x07},
    {0x3809, 0x80},
    {0x380a, 0x04},
    {0x380b, 0x38},
    {0x380c, 0x0c},
    {0x380d, 0x98},
    {0x380e, 0x07},
    {0x380f, 0xd8},
    {0x3810, 0x00},
    {0x3811, 0x10},
    {0x3812, 0x00},
    {0x3813, 0x06},
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3820, 0x10},
    {0x3821, 0x1e},
    {0x4502, 0x08},
    {0x4816, 0x52},
    {0x481f, 0x30},
    {0x4826, 0x28},
    {0x4837, 0x10},
    {0x5002, 0x80},
};
#define NUM_OF_1080P_SUB_INIT_REGS (sizeof(sensor_1080p_sub_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_720p_sub_init_regs[] = {
    //;;;; key settings for 27Mhz input clock, sclk=97.2Mhz, MIPI_CLK=243Mhz, 60fps
    {0x3501, 0x2e},
    {0x3502, 0x80},
    {0x3600, 0x6c},
    {0x3601, 0x23},
    {0x3604, 0x24},
    {0x3708, 0xe6},
    {0x3802, 0x00},
    {0x3803, 0xf6},
    {0x3804, 0x0a},
    {0x3805, 0x3f},
    {0x3806, 0x06},
    {0x3807, 0xad},
    {0x3808, 0x05},
    {0x3809, 0x00},
    {0x380a, 0x02},
    {0x380b, 0xd0},
    {0x380c, 0x10},
    {0x380d, 0x70},
    {0x380e, 0x03},
    {0x380f, 0x00},
    {0x3810, 0x00},
    {0x3811, 0x07},
    {0x3812, 0x00},
    {0x3813, 0x02},
    {0x3814, 0x31},
    {0x3815, 0x31},
    {0x3820, 0x11},
    {0x3821, 0x1f},
    {0x4502, 0x48},
    {0x4816, 0x52},
    {0x481f, 0x30},
    {0x4826, 0x38},
    {0x4837, 0x10},
    {0x5002, 0x00},
};
#define NUM_OF_720P_SUB_INIT_REGS (sizeof(sensor_720p_sub_init_regs) / sizeof(sensor_reg_t))

typedef struct gain_set {
    u8  gain_analog;
    u32 gain_x; // UFIX 7.6
} gain_set_t;

static const gain_set_t gain_table[] = {
    {0x10, 64}, {0x11, 68}, {0x12, 72}, {0x13, 76}, {0x14, 80}, 
    {0x15, 84}, {0x16, 88}, {0x17, 92}, {0x18, 96}, {0x19, 100},
    {0x1A, 104},{0x1B, 108},{0x1C, 112},{0x1D, 116},{0x1E, 120},
    {0x1F, 124},{0x20, 128},{0x21, 132},{0x22, 136},{0x23, 140},
    {0x24, 144},{0x25, 148},{0x26, 152},{0x27, 156},{0x28, 160},
    {0x29, 164},{0x2A, 168},{0x2B, 172},{0x2C, 176},{0x2D, 180},
    {0x2E, 184},{0x2F, 188},{0x30, 192},{0x31, 196},{0x32, 200},
    {0x33, 204},{0x34, 208},{0x35, 212},{0x36, 216},{0x37, 220},
    {0x38, 224},{0x39, 228},{0x3A, 232},{0x3B, 236},{0x3C, 240},
    {0x3D, 244},{0x3E, 248},{0x3F, 252},{0x40, 256},{0x41, 260},
    {0x42, 264},{0x43, 268},{0x44, 272},{0x45, 276},{0x46, 280},
    {0x47, 284},{0x48, 288},{0x49, 292},{0x4A, 296},{0x4B, 300},
    {0x4C, 304},{0x4D, 308},{0x4E, 312},{0x4F, 316},{0x50, 320},
    {0x51, 324},{0x52, 328},{0x53, 332},{0x54, 336},{0x55, 340},
    {0x56, 344},{0x57, 348},{0x58, 352},{0x59, 356},{0x5A, 360},
    {0x5B, 364},{0x5C, 368},{0x5D, 372},{0x5E, 376},{0x5F, 380},
    {0x60, 384},{0x61, 388},{0x62, 392},{0x63, 396},{0x64, 400},
    {0x65, 404},{0x66, 408},{0x67, 412},{0x68, 416},{0x69, 420},
    {0x6A, 424},{0x6B, 428},{0x6C, 432},{0x6D, 436},{0x6E, 440},
    {0x6F, 444},{0x70, 448},{0x71, 452},{0x72, 456},{0x73, 460},
    {0x74, 464},{0x75, 468},{0x76, 472},{0x77, 476},{0x78, 480},
    {0x79, 484},{0x7A, 488},{0x7B, 492},{0x7C, 496},{0x7D, 500},
    {0x7E, 504},{0x7F, 508},{0x80, 512},{0x81, 516},{0x82, 520},
    {0x83, 524},{0x84, 528},{0x85, 532},{0x86, 536},{0x87, 540},
    {0x88, 544},{0x89, 548},{0x8A, 552},{0x8B, 556},{0x8C, 560},
    {0x8D, 564},{0x8E, 568},{0x8F, 572},{0x90, 576},{0x91, 580},
    {0x92, 584},{0x93, 588},{0x94, 592},{0x95, 596},{0x96, 600},
    {0x97, 604},{0x98, 608},{0x99, 612},{0x9A, 616},{0x9B, 620},
    {0x9C, 624},{0x9D, 628},{0x9E, 632},{0x9F, 636},{0xA0, 640},
    {0xA1, 644},{0xA2, 648},{0xA3, 652},{0xA4, 656},{0xA5, 660},
    {0xA6, 664},{0xA7, 668},{0xA8, 672},{0xA9, 676},{0xAA, 680},
    {0xAB, 684},{0xAC, 688},{0xAD, 692},{0xAE, 696},{0xAF, 700},
    {0xB0, 704},{0xB1, 708},{0xB2, 712},{0xB3, 716},{0xB4, 720},
    {0xB5, 724},{0xB6, 728},{0xB7, 732},{0xB8, 736},{0xB9, 740},
    {0xBA, 744},{0xBB, 748},{0xBC, 752},{0xBD, 756},{0xBE, 760},
    {0xBF, 764},{0xC0, 768},{0xC1, 772},{0xC2, 776},{0xC3, 780},
    {0xC4, 784},{0xC5, 788},{0xC6, 792},{0xC7, 796},{0xC8, 800},
    {0xC9, 804},{0xCA, 808},{0xCB, 812},{0xCC, 816},{0xCD, 820},
    {0xCE, 824},{0xCF, 828},{0xD0, 832},{0xD1, 836},{0xD2, 840},
    {0xD3, 844},{0xD4, 848},{0xD5, 852},{0xD6, 856},{0xD7, 860},
    {0xD8, 864},{0xD9, 868},{0xDA, 872},{0xDB, 876},{0xDC, 880},
    {0xDD, 884},{0xDE, 888},{0xDF, 892},{0xE0, 896},{0xE1, 900},
    {0xE2, 904},{0xE3, 908},{0xE4, 912},{0xE5, 916},{0xE6, 920},
    {0xE7, 924},{0xE8, 928},{0xE9, 932},{0xEA, 936},{0xEB, 940},
    {0xEC, 944},{0xED, 948},{0xEE, 952},{0xEF, 956},{0xF0, 960},
    {0xF1, 964},{0xF2, 968},{0xF3, 972},{0xF4, 976},{0xF5, 980},
    {0xF6, 984},{0xF7, 988},{0xF8, 992},
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
    u32 reg, pixclk, mul;
    u8  reg_div_r;
    u8  pre_div, div_s, div_r;

    read_reg(0x3604, &reg);
    mul = reg;
    read_reg(0x3605, &reg);
    div_s = (reg & 0xf) + 1;
    pre_div = (reg >> 4) & 0x07;    // 0~7 => 1, 1, 2, 3, 4, 1, 1, 1
    if (pre_div < 1)
        pre_div = 1;
    if (pre_div > 4)
        pre_div = 1;
    
    read_reg(0x3606, &reg);
    reg_div_r = (reg >> 4) & 0x3;

    switch (reg_div_r) {
        case 0:
        case 1:
            div_r = 2;     // Divide by 1
            break;
        case 2:
            div_r = 4;     // Divide by 2
            break;
        case 3:
            div_r = 5;     // Divide by 2.5
            break;
    }
 
    pixclk = (xclk / pre_div * mul / div_s) * 2 / div_r / 2;
    pixclk = pixclk << 1;   // x2 channel

    isp_info("********OV5658 Pixel Clock %d*********\n", pixclk);
    return pixclk;
}

static void adjust_blanking(int fps)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if ((g_psensor->img_win.width <= 1280) && (g_psensor->img_win.height <= 720)){
        if (fps > 60)
            fps = 60;
        pinfo->lines_max = 0x300;
        pinfo->sw_lines_max = pinfo->lines_max*60/fps - 6;
    }
    else if ((g_psensor->img_win.width <= 1920) && (g_psensor->img_win.height <= 1080)){
        if (fps > 30)
            fps = 30;
        pinfo->lines_max = 0x7d8;
        pinfo->sw_lines_max = pinfo->lines_max*30/fps - 6;
    }
    else if ((g_psensor->img_win.width <= 2304) && (g_psensor->img_win.height <= 1296)){
        if (fps > 30)
            fps = 30;
        pinfo->lines_max = 0x7d8;
        pinfo->sw_lines_max = pinfo->lines_max*30/fps - 6;
    }
    else if ((g_psensor->img_win.width <= 2048) && (g_psensor->img_win.height <= 1536)){
        if (fps > 30)
            fps = 30;
        pinfo->lines_max = 0x7d8;
        pinfo->sw_lines_max = pinfo->lines_max*30/fps - 6;
    }
    else{   //2592x1944
        if (fps > 30)
            fps = 30;
        pinfo->lines_max = 0x7c0;
        pinfo->sw_lines_max = pinfo->lines_max*30/fps - 6;
    }
    write_reg(0x380f, ((pinfo->sw_lines_max+6) & 0xFF));
    write_reg(0x380e, (((pinfo->sw_lines_max+6) >> 8) & 0xFF));

    g_psensor->fps = fps;
    isp_info("adjust_blanking, fps=%d\n", g_psensor->fps);

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
    }
    else if ((g_psensor->img_win.width <= 1920) && (g_psensor->img_win.height <= 1080)) {
        pInitTable = sensor_1080p_sub_init_regs;
        InitTable_Num = NUM_OF_1080P_SUB_INIT_REGS;
    }
    else if ((g_psensor->img_win.width <= 2304) && (g_psensor->img_win.height <= 1296)) {
        pInitTable = sensor_3M_sub_init_regs;
        InitTable_Num = NUM_OF_3M_SUB_INIT_REGS;
    }
    else if ((g_psensor->img_win.width <= 2048) && (g_psensor->img_win.height <= 1536)) {
        pInitTable = sensor_3M_1536p_sub_init_regs;
        InitTable_Num = NUM_OF_3M_1536P_SUB_INIT_REGS;
    }
    else{
        pInitTable = sensor_5M_sub_init_regs;
        InitTable_Num = NUM_OF_5M_SUB_INIT_REGS;
    }
    // write initial register value
    for (i=0; i<InitTable_Num; i++) {
        if (write_reg(pInitTable[i].addr, pInitTable[i].val) < 0) {
            isp_err("failed to sub initial %s!\n", SENSOR_NAME);
            return -EINVAL;
        }
    }

    set_mirror(pinfo->mirror);
    set_flip(pinfo->flip);

    g_psensor->pclk = get_pclk(g_psensor->xclk);
    adjust_blanking(g_psensor->fps);
    calculate_t_row();

    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    g_psensor->out_win.width = width;
    g_psensor->out_win.height = height;
    g_psensor->img_win.x = ((g_psensor->out_win.width-width)/2) & 0xfffe;
    g_psensor->img_win.y = ((g_psensor->out_win.height-height)/2) & 0xfffe;
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
    u32 reg, i;

    if (g_psensor->curr_gain == 0) {
        read_reg(0x350B, &reg);
        for (i=0; i<NUM_OF_GAINSET; i++) {
            if (gain_table[i].gain_analog == reg)
                break;
        }
        if (i < NUM_OF_GAINSET)
            g_psensor->curr_gain = gain_table[i].gain_x;
    }

    return g_psensor->curr_gain;
}

static int set_gain(u32 gain)
{
    int ret = 0;
    int i = 0;

    // search most suitable gain into gain table
    if (gain > gain_table[NUM_OF_GAINSET - 1].gain_x)
        gain = gain_table[NUM_OF_GAINSET - 1].gain_x;
    else if (gain < gain_table[0].gain_x)
        gain = gain_table[0].gain_x;
	else{
	    for (i=0; i<NUM_OF_GAINSET; i++) {
    	    if (gain_table[i].gain_x > gain)
        	    break;
    	}
	}
    write_reg(0x350B, gain_table[i-1].gain_analog);
    g_psensor->curr_gain = gain_table[i-1].gain_x;

    return ret;
}

static int get_mirror(void)
{
    u32 reg;

    read_reg(0x3821, &reg);
    return (reg & BIT1) ? 1 : 0;
}

static int set_mirror(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    read_reg(0x3821, &reg);
    if (enable) {
        write_reg(0x3821, reg | 0x06);
    } else {
        write_reg(0x3821, reg & ~0x06);
    }
    pinfo->mirror = enable;

    return 0;
}

static int get_flip(void)
{
    u32 reg;

    read_reg(0x3820, &reg);
    return (reg & BIT1) ? 1 : 0;
}

static int set_flip(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    read_reg(0x3820, &reg);
    if (enable) 
        write_reg(0x3820, reg | 0x42);
    else
        write_reg(0x3820, reg & ~0x42);
    
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
    sensor_reg_t* pInitTable;
    u32 InitTable_Num;

    if (pinfo->is_init)
        return 0;

    pInitTable = sensor_init_regs;
    InitTable_Num = NUM_OF_INIT_REGS;

    // write initial register value
    for (i=0; i<InitTable_Num; i++) {
        if (write_reg(pInitTable[i].addr, pInitTable[i].val) < 0) {
            isp_err("failed to initial %s!\n", SENSOR_NAME);
            return -EINVAL;
        }
    }

    pinfo->mirror = mirror;
    pinfo->flip = flip;

    // set image size
    if ((ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height)) < 0)
        return ret;

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
static void ov5658_deconstruct(void)
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

static int ov5658_construct(u32 xclk, u16 width, u16 height)
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
    g_psensor->min_gain = gain_table[0].gain_x;
    g_psensor->max_gain = gain_table[NUM_OF_GAINSET - 1].gain_x;
    g_psensor->exp_latency = 0;
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

    return 0;

construct_err:
    ov5658_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init ov5658_init(void)
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

    if ((ret = ov5658_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit ov5658_exit(void)
{
    isp_unregister_sensor(g_psensor);
    ov5658_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(ov5658_init);
module_exit(ov5658_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor OV5658");
MODULE_LICENSE("GPL");
