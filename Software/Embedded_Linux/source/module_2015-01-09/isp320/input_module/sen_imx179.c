/**
 * @file sen_imx179.c
 * SONY IMX179 sensor driver
 *
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.6 $
 * $Date: 2014/11/10 01:17:44 $
 *
 * ChangeLog:
 *  $Log: sen_imx179.c,v $
 *  Revision 1.6  2014/11/10 01:17:44  jtwang
 *  *** empty log message ***
 *
 *  Revision 1.5  2014/09/05 07:19:57  jtwang
 *  fix ID_FPS property
 *
 *  Revision 1.4  2014/07/25 08:38:32  ricktsao
 *  no message
 *
 *  Revision 1.3  2014/06/19 01:47:52  jtwang
 *  update for new sdk
 *
 *  Revision 1.2  2014/03/07 03:30:07  jtwang
 *  *** empty log message ***
 *
 *  Revision 1.1  2013/12/10 09:20:05  jtwang
 *  *** empty log message ***
 *
 *  Revision 1.3  2013/08/20 07:03:56  kl_tang
 *  SubLVDS Version
 *
 *  Revision 1.2  2013/03/15 08:12:49  ricktsao
 *  no message
 *
 *  Revision 1.1  2013/03/15 08:02:10  ricktsao
 *  First version
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

#define PFX "sen_imx179"
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

//=============================================================================
// global
//=============================================================================
#define SENSOR_NAME         "IMX179"
#define SENSOR_MAX_WIDTH    3280
#define SENSOR_MAX_HEIGHT   2464

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
#define DELAY_CODE      0xFFFF

// All output 1920x1080, only resized by crop size
static sensor_reg_t sensor_1080P_init_regs[] = {
    {0x0100, 0x00},
    {0x0101, 0x00},
//    {0x0202, 0x09},
    {0x0202, 0x06},
    {0x0203, 0x9E},
    {0x0301, 0x05},
    {0x0303, 0x01},
    {0x0305, 0x06},
    {0x0309, 0x05},
    {0x030B, 0x01},
    {0x030C, 0x00},
#if 0
//    {0x030D, 0xA0},
    {0x030D, 0x50},
    {0x0340, 0x09},
    {0x0341, 0xA2},
    {0x0342, 0x0D},
    {0x0343, 0x84},
#else
    {0x030D, 0x6f},
    {0x0340, 0x07},
    {0x0341, 0x99},
    {0x0342, 0x0D},
    {0x0343, 0x60},
#endif
    {0x0344, 0x00},
    {0x0345, 0x00},
    {0x0346, 0x01},
    {0x0347, 0x34},
    {0x0348, 0x0C},
    {0x0349, 0xCF},
    {0x034A, 0x08},
    {0x034B, 0x6B},
#if 1
    {0x034C, 0x07},     //1942
    {0x034D, 0x96},
    {0x034E, 0x04},     //1094
    {0x034F, 0x46},
#else
    {0x034C, 0x07},     //1920
    {0x034D, 0x80},
    {0x034E, 0x04},     //1080
    {0x034F, 0x38},
#endif
    {0x0383, 0x01},
    {0x0387, 0x01},
    {0x0390, 0x00},
    {0x0401, 0x02},
    {0x0405, 0x1B},
    {0x3020, 0x10},
    {0x3041, 0x15},
    {0x3042, 0x87},
    {0x3089, 0x4F},
    {0x3309, 0x9A},
    {0x3344, 0x57},
    {0x3345, 0x1F},
    {0x3362, 0x0A},
    {0x3363, 0x0A},
    {0x3364, 0x00},
    {0x3368, 0x18},
    {0x3369, 0x00},
    {0x3370, 0x77},
    {0x3371, 0x2F},
    {0x3372, 0x4F},
    {0x3373, 0x2F},
    {0x3374, 0x2F},
    {0x3375, 0x37},
    {0x3376, 0x9F},
    {0x3377, 0x37},
    {0x33C8, 0x00},
    {0x33D4, 0x0C},
    {0x33D5, 0xD0},
    {0x33D6, 0x07},
    {0x33D7, 0x38},
    {0x4100, 0x0E},
    {0x4108, 0x01},
    {0x4109, 0x7C},
    {0x0100, 0x01},
};
#define NUM_OF_1080P_INIT_REGS (sizeof(sensor_1080P_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_1848P_init_regs[] = {
    {0x0100,0x00},
    {0x0101,0x00},
    {0x0202,0x09},
    {0x0203,0x9E},
    {0x0301,0x05},
    {0x0303,0x01},
    {0x0305,0x06},
    {0x0309,0x05},
    {0x030B,0x01},
    {0x030C,0x00},
//    {0x030D,0xA0},
//    {0x030D,0x7D},
    {0x030D,0x48},
    {0x0340,0x09},
    {0x0341,0xA2},
    {0x0342,0x0D},
    {0x0343,0x84},
    {0x0344,0x00},
    {0x0345,0x00},
    {0x0346,0x01},
//    {0x0347,0x34},
    {0x0347,0x24},
    {0x0348,0x0C},
    {0x0349,0xCF},
    {0x034A,0x08},
    {0x034B,0x6B},
    {0x034C,0x0C},
    {0x034D,0xD0},
    {0x034E,0x07},
    {0x034F,0x40},
    {0x0383,0x01},
    {0x0387,0x01},
    {0x0390,0x00},
    {0x0401,0x00},
    {0x0405,0x10},
    {0x3020,0x10},
    {0x3041,0x15},
    {0x3042,0x87},
    {0x3089,0x4F},
    {0x3309,0x9A},
    {0x3344,0x57},
    {0x3345,0x1F},
    {0x3362,0x0A},
    {0x3363,0x0A},
    {0x3364,0x00},
    {0x3368,0x18},
    {0x3369,0x00},
    {0x3370,0x77},
    {0x3371,0x2F},
    {0x3372,0x4F},
    {0x3373,0x2F},
    {0x3374,0x2F},
    {0x3375,0x37},
    {0x3376,0x9F},
    {0x3377,0x37},
    {0x33C8,0x00},
    {0x33D4,0x0C},
    {0x33D5,0xD0},
    {0x33D6,0x07},
    {0x33D7,0x38},
    {0x4100,0x0E},
    {0x4108,0x01},
    {0x4109,0x7C},
    {0x0100,0x01},
};
#define NUM_OF_1848P_INIT_REGS (sizeof(sensor_1848P_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_2464P_init_regs[] = {
    {0x0100,0x00},
    {0x0101,0x00},
    {0x0202,0x09},
    {0x0203,0x9E},
    {0x0301,0x05},
    {0x0303,0x01},
    {0x0305,0x06},
    {0x0309,0x05},
    {0x030B,0x01},
    {0x030C,0x00},
//    {0x030D,0xA0},
//    {0x030D,0x7D},
    {0x030D,0x4E},
    {0x0340,0x0A},
    {0x0341,0x20},
    {0x0342,0x0E},
    {0x0343,0x10},
    {0x0344,0x00},
    {0x0345,0x00},
    {0x0346,0x00},
    {0x0347,0x00},
    {0x0348,0x0C},
    {0x0349,0xCF},
    {0x034A,0x09},
    {0x034B,0xA3},
    {0x034C,0x0C},
    {0x034D,0xD0},
    {0x034E,0x09},
    {0x034F,0xA4},
    {0x0383,0x01},
    {0x0387,0x01},
    {0x0390,0x00},
    {0x0401,0x00},
    {0x0405,0x10},
    {0x3020,0x10},
    {0x3041,0x15},
    {0x3042,0x87},
    {0x3089,0x4F},
    {0x3309,0x9A},
    {0x3344,0x57},
    {0x3345,0x1F},
    {0x3362,0x0A},
    {0x3363,0x0A},
    {0x3364,0x00},
    {0x3368,0x18},
    {0x3369,0x00},
    {0x3370,0x77},
    {0x3371,0x2F},
    {0x3372,0x4F},
    {0x3373,0x2F},
    {0x3374,0x2F},
    {0x3375,0x37},
    {0x3376,0x9F},
    {0x3377,0x37},
    {0x33C8,0x00},
    {0x33D4,0x0C},
    {0x33D5,0xD0},
    {0x33D6,0x09},
    {0x33D7,0xA4},
    {0x4100,0x0E},
    {0x4108,0x01},
    {0x4109,0x7C},
    {0x0100,0x01},
};
#define NUM_OF_2464P_INIT_REGS (sizeof(sensor_2464P_init_regs) / sizeof(sensor_reg_t))

typedef struct _gain_set
{
    u16 ad_gain;
    u16 digital_gain;
    u16 gain_x;     // UFIX7.6
} gain_set_t;

static gain_set_t gain_table[] =
{
	{0x00, 0x100, 64},  {0x02, 0x100, 65},  {0x04, 0x100, 65},  {0x06, 0x100, 66},
	{0x08, 0x100, 66},  {0x0A, 0x100, 67},  {0x0C, 0x100, 67},  {0x0E, 0x100, 68},
	{0x10, 0x100, 68},  {0x12, 0x100, 69},  {0x14, 0x100, 69},  {0x16, 0x100, 70},
	{0x18, 0x100, 71},  {0x1A, 0x100, 71},  {0x1C, 0x100, 72},  {0x1E, 0x100, 72},
	{0x20, 0x100, 73},  {0x22, 0x100, 74},  {0x24, 0x100, 74},  {0x26, 0x100, 75},
	{0x28, 0x100, 76},  {0x2A, 0x100, 77},  {0x2C, 0x100, 77},  {0x2E, 0x100, 78},
	{0x30, 0x100, 79},  {0x32, 0x100, 80},  {0x34, 0x100, 80},  {0x36, 0x100, 81},
	{0x38, 0x100, 82},  {0x3A, 0x100, 83},  {0x3C, 0x100, 84},  {0x3E, 0x100, 84},
	{0x40, 0x100, 85},  {0x42, 0x100, 86},  {0x44, 0x100, 87},  {0x46, 0x100, 88},
	{0x48, 0x100, 89},  {0x4A, 0x100, 90},  {0x4C, 0x100, 91},  {0x4E, 0x100, 92},
	{0x50, 0x100, 93},  {0x52, 0x100, 94},  {0x54, 0x100, 95},  {0x56, 0x100, 96},
	{0x58, 0x100, 98},  {0x5A, 0x100, 99},  {0x5C, 0x100, 100}, {0x5E, 0x100, 101},
	{0x60, 0x100, 102}, {0x62, 0x100, 104}, {0x64, 0x100, 105}, {0x66, 0x100, 106},
	{0x68, 0x100, 108}, {0x6A, 0x100, 109}, {0x6C, 0x100, 111}, {0x6E, 0x100, 112},
	{0x70, 0x100, 114}, {0x72, 0x100, 115}, {0x74, 0x100, 117}, {0x76, 0x100, 119},
	{0x78, 0x100, 120}, {0x7A, 0x100, 122}, {0x7C, 0x100, 124}, {0x7E, 0x100, 126},
	{0x80, 0x100, 128}, {0x82, 0x100, 130}, {0x84, 0x100, 132}, {0x86, 0x100, 134},
	{0x88, 0x100, 137}, {0x8A, 0x100, 139}, {0x8C, 0x100, 141}, {0x8E, 0x100, 144},
	{0x90, 0x100, 146}, {0x92, 0x100, 149}, {0x94, 0x100, 152}, {0x96, 0x100, 155},
	{0x98, 0x100, 158}, {0x9A, 0x100, 161}, {0x9C, 0x100, 164}, {0x9E, 0x100, 167},
	{0xA0, 0x100, 171}, {0xA2, 0x100, 174}, {0xA4, 0x100, 178}, {0xA6, 0x100, 182},
	{0xA8, 0x100, 186}, {0xAA, 0x100, 191}, {0xAC, 0x100, 195}, {0xAE, 0x100, 200},
	{0xB0, 0x100, 205}, {0xB2, 0x100, 210}, {0xB4, 0x100, 216}, {0xB6, 0x100, 221},
	{0xB8, 0x100, 228}, {0xBA, 0x100, 234}, {0xBC, 0x100, 241}, {0xBE, 0x100, 248},
	{0xC0, 0x100, 256}, {0xC2, 0x100, 264}, {0xC4, 0x100, 273}, {0xC6, 0x100, 282},
	{0xC8, 0x100, 293}, {0xCA, 0x100, 303}, {0xCC, 0x100, 315}, {0xCE, 0x100, 328},
	{0xD0, 0x100, 341}, {0xD2, 0x100, 356}, {0xD4, 0x100, 372}, {0xD6, 0x100, 390},
	{0xD8, 0x100, 410}, {0xDA, 0x100, 431}, {0xDC, 0x100, 455}, {0xDE, 0x100, 482},
	{0xE0, 0x100, 512}, {0xE0, 0x110, 544}, {0xE0, 0x120, 576}, {0xE0, 0x130, 608},
	{0xE0, 0x140, 640}, {0xE0, 0x150, 672}, {0xE0, 0x160, 704}, {0xE0, 0x170, 736},
	{0xE0, 0x180, 768}, {0xE0, 0x190, 800}, {0xE0, 0x1A0, 832}, {0xE0, 0x1B0, 864},
	{0xE0, 0x1C0, 896}, {0xE0, 0x1D0, 928}, {0xE0, 0x1E0, 960}, {0xE0, 0x1F0, 992},
	{0xE0, 0x210, 1056},{0xE0, 0x220, 1088},{0xE0, 0x230, 1120},{0xE0, 0x240, 1152},
	{0xE0, 0x250, 1184},{0xE0, 0x260, 1216},{0xE0, 0x270, 1248},{0xE0, 0x280, 1280},
	{0xE0, 0x290, 1312},{0xE0, 0x2A0, 1344},{0xE0, 0x2B0, 1376},{0xE0, 0x2C0, 1408},
	{0xE0, 0x2D0, 1440},{0xE0, 0x2E0, 1472},{0xE0, 0x2F0, 1504},{0xE0, 0x310, 1568},
	{0xE0, 0x320, 1600},{0xE0, 0x330, 1632},{0xE0, 0x340, 1664},{0xE0, 0x350, 1696},
	{0xE0, 0x360, 1728},{0xE0, 0x370, 1760},{0xE0, 0x380, 1792},{0xE0, 0x390, 1824},
	{0xE0, 0x3A0, 1856},{0xE0, 0x3B0, 1888},{0xE0, 0x3C0, 1920},{0xE0, 0x3D0, 1952},
	{0xE0, 0x3E0, 1984},{0xE0, 0x3F0, 2016},{0xE0, 0x410, 2080},{0xE0, 0x420, 2112},
	{0xE0, 0x430, 2144},{0xE0, 0x440, 2176},{0xE0, 0x450, 2208},{0xE0, 0x460, 2240},
	{0xE0, 0x470, 2272},{0xE0, 0x480, 2304},{0xE0, 0x490, 2336},{0xE0, 0x4A0, 2368},
	{0xE0, 0x4B0, 2400},{0xE0, 0x4C0, 2432},{0xE0, 0x4D0, 2464},{0xE0, 0x4E0, 2496},
	{0xE0, 0x4F0, 2528},{0xE0, 0x510, 2592},{0xE0, 0x520, 2624},{0xE0, 0x530, 2656},
	{0xE0, 0x540, 2688},{0xE0, 0x550, 2720},{0xE0, 0x560, 2752},{0xE0, 0x570, 2784},
	{0xE0, 0x580, 2816},{0xE0, 0x590, 2848},{0xE0, 0x5A0, 2880},{0xE0, 0x5B0, 2912},
	{0xE0, 0x5C0, 2944},{0xE0, 0x5D0, 2976},{0xE0, 0x5E0, 3008},{0xE0, 0x5F0, 3040},
	{0xE0, 0x610, 3104},{0xE0, 0x620, 3136},{0xE0, 0x630, 3168},{0xE0, 0x640, 3200},
	{0xE0, 0x650, 3232},{0xE0, 0x660, 3264},{0xE0, 0x670, 3296},{0xE0, 0x680, 3328},
	{0xE0, 0x690, 3360},{0xE0, 0x6A0, 3392},{0xE0, 0x6B0, 3424},{0xE0, 0x6C0, 3456},
	{0xE0, 0x6D0, 3488},{0xE0, 0x6E0, 3520},{0xE0, 0x6F0, 3552},{0xE0, 0x710, 3616},
	{0xE0, 0x720, 3648},{0xE0, 0x730, 3680},{0xE0, 0x740, 3712},{0xE0, 0x750, 3744},
	{0xE0, 0x760, 3776},{0xE0, 0x770, 3808},{0xE0, 0x780, 3840},{0xE0, 0x790, 3872},
	{0xE0, 0x7A0, 3904},{0xE0, 0x7B0, 3936},{0xE0, 0x7C0, 3968},{0xE0, 0x7D0, 4000},
	{0xE0, 0x7E0, 4032},{0xE0, 0x7F0, 4064},{0xE0, 0x810, 4128},{0xE0, 0x820, 4160},
	{0xE0, 0x830, 4192},{0xE0, 0x840, 4224},{0xE0, 0x850, 4256},{0xE0, 0x860, 4288},
	{0xE0, 0x870, 4320},{0xE0, 0x880, 4352},{0xE0, 0x890, 4384},{0xE0, 0x8A0, 4416},
	{0xE0, 0x8B0, 4448},{0xE0, 0x8C0, 4480},{0xE0, 0x8D0, 4512},{0xE0, 0x8E0, 4544},
	{0xE0, 0x8F0, 4576},{0xE0, 0x910, 4640},{0xE0, 0x920, 4672},{0xE0, 0x930, 4704},
	{0xE0, 0x940, 4736},{0xE0, 0x950, 4768},{0xE0, 0x960, 4800},{0xE0, 0x970, 4832},
	{0xE0, 0x980, 4864},{0xE0, 0x990, 4896},{0xE0, 0x9A0, 4928},{0xE0, 0x9B0, 4960},
	{0xE0, 0x9C0, 4992},{0xE0, 0x9D0, 5024},{0xE0, 0x9E0, 5056},{0xE0, 0x9F0, 5088},
	{0xE0, 0xA10, 5152},{0xE0, 0xA20, 5184},{0xE0, 0xA30, 5216},{0xE0, 0xA40, 5248},
	{0xE0, 0xA50, 5280},{0xE0, 0xA60, 5312},{0xE0, 0xA70, 5344},{0xE0, 0xA80, 5376},
	{0xE0, 0xA90, 5408},{0xE0, 0xAA0, 5440},{0xE0, 0xAB0, 5472},{0xE0, 0xAC0, 5504},
	{0xE0, 0xAD0, 5536},{0xE0, 0xAE0, 5568},{0xE0, 0xAF0, 5600},{0xE0, 0xB10, 5664},
	{0xE0, 0xB20, 5696},{0xE0, 0xB30, 5728},{0xE0, 0xB40, 5760},{0xE0, 0xB50, 5792},
	{0xE0, 0xB60, 5824},{0xE0, 0xB70, 5856},{0xE0, 0xB80, 5888},{0xE0, 0xB90, 5920},
	{0xE0, 0xBA0, 5952},{0xE0, 0xBB0, 5984},{0xE0, 0xBC0, 6016},{0xE0, 0xBD0, 6048},
	{0xE0, 0xBE0, 6080},{0xE0, 0xBF0, 6112},{0xE0, 0xC10, 6176},{0xE0, 0xC20, 6208},
	{0xE0, 0xC30, 6240},{0xE0, 0xC40, 6272},{0xE0, 0xC50, 6304},{0xE0, 0xC60, 6336},
	{0xE0, 0xC70, 6368},{0xE0, 0xC80, 6400},{0xE0, 0xC90, 6432},{0xE0, 0xCA0, 6464},
	{0xE0, 0xCB0, 6496},{0xE0, 0xCC0, 6528},{0xE0, 0xCD0, 6560},{0xE0, 0xCE0, 6592},
	{0xE0, 0xCF0, 6624},{0xE0, 0xD10, 6688},{0xE0, 0xD20, 6720},{0xE0, 0xD30, 6752},
	{0xE0, 0xD40, 6784},{0xE0, 0xD50, 6816},{0xE0, 0xD60, 6848},{0xE0, 0xD70, 6880},
	{0xE0, 0xD80, 6912},{0xE0, 0xD90, 6944},{0xE0, 0xDA0, 6976},{0xE0, 0xDB0, 7008},
	{0xE0, 0xDC0, 7040},{0xE0, 0xDD0, 7072},{0xE0, 0xDE0, 7104},{0xE0, 0xDF0, 7136},
	{0xE0, 0xE10, 7200},{0xE0, 0xE20, 7232},{0xE0, 0xE30, 7264},{0xE0, 0xE40, 7296},
	{0xE0, 0xE50, 7328},{0xE0, 0xE60, 7360},{0xE0, 0xE70, 7392},{0xE0, 0xE80, 7424},
	{0xE0, 0xE90, 7456},{0xE0, 0xEA0, 7488},{0xE0, 0xEB0, 7520},{0xE0, 0xEC0, 7552},
	{0xE0, 0xED0, 7584},{0xE0, 0xEE0, 7616},{0xE0, 0xEF0, 7648},{0xE0, 0xF10, 7712},
	{0xE0, 0xF20, 7744},{0xE0, 0xF30, 7776},{0xE0, 0xF40, 7808},{0xE0, 0xF50, 7840},
	{0xE0, 0xF60, 7872},{0xE0, 0xF70, 7904},{0xE0, 0xF80, 7936},{0xE0, 0xF90, 7968},
	{0xE0, 0xFA0, 8000},{0xE0, 0xFB0, 8032},{0xE0, 0xFC0, 8064},{0xE0, 0xFD0, 8096},
	{0xE0, 0xFE0, 8128},{0xE0, 0xFF0, 8160},{0xE0, 0xFFF, 8190}
};
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

//=============================================================================
// I2C
//=============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            0x20 >> 1
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

    //printk("i2c_access 0x%02x w 0x%02x 0x%02x 0x%02x \n",
    //   I2C_ADDR, buf[0], buf[1], buf[2] );

    return sensor_i2c_transfer(&msgs, 1);
}

static u32 get_pclk(u32 xclk)
{
    u32 pclk, reg, pll_multiplier, pre_pll_clk_div, vt_sys_clk_div, vt_pix_clk_div;

    read_reg(0x30c, &reg);
    read_reg(0x30d, &pll_multiplier);
    pll_multiplier = pll_multiplier | (reg << 8);
    read_reg(0x305, &pre_pll_clk_div);
    read_reg(0x301, &vt_pix_clk_div);
    read_reg(0x303, &vt_sys_clk_div);
//    pclk = xclk * reg / 6 / (1 * 5) * 2;
    pclk = xclk / 100 * pll_multiplier * 2 / (pre_pll_clk_div * vt_sys_clk_div * vt_pix_clk_div)  * 100;
    isp_info("IMX179 Sensor: pclk(%d) XCLK(%d)\n", pclk, xclk);
    return pclk;
}

static void adjust_blanking(int fps)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 vmax, max_fps;

    if (g_psensor->img_win.height <= 1080){
        vmax = 0x799;
        max_fps = 30;
    }
    else if (g_psensor->img_win.height <= 1848){
        vmax = 0x9a2;
        max_fps = 15;
    }
    else{
        vmax = 0xa20;
        max_fps = 15;
    }
    if (fps > max_fps)
        fps = max_fps;
    pinfo->VMax = vmax*max_fps/fps;

    write_reg(0x341, (pinfo->VMax & 0xFF));
    write_reg(0x340, ((pinfo->VMax >> 8) & 0xFF));
    g_psensor->fps = fps;

    isp_info("adjust_blanking, fps=%d\n", fps);
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
    u32 reg_l, reg_h;
    int ret = 0;
	isp_info("w=%d, h=%d\n", width, height);
    // check size
    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("Exceed maximum sensor resolution!\n");
        return -1;
    }

    // get HMax and VMax
    read_reg(0x343, &reg_l);
    read_reg(0x342, &reg_h);

    pinfo->HMax = ( ((reg_h & 0xFF) << 8) | (reg_l & 0xFF) );

    read_reg(0x341, &reg_l);
    read_reg(0x340, &reg_h);
    pinfo->VMax = ((reg_h & 0xFF) << 8) | (reg_l & 0xFF);

    adjust_blanking(g_psensor->fps);
    calculate_t_row();

    // update sensor information
    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;


    if (width<=1920 && height<=1080 ) {
        g_psensor->out_win.width = 1942;
        g_psensor->out_win.height = 1094;
//        g_psensor->out_win.width = width;
//        g_psensor->out_win.height = height;
        g_psensor->img_win.x = 0;
        g_psensor->img_win.y = 0;

        isp_info("1080P,Sensor VMAX=0x%x(%d) , HMAX=0x%x(%d) \n",
            pinfo->VMax ,pinfo->VMax ,pinfo->HMax,pinfo->HMax );
    } else if (width<=3280 && height<=1848 ) {        //3280x1848
        g_psensor->out_win.width = 3280;
        g_psensor->out_win.height = 1848;
        g_psensor->img_win.x = 0;
        g_psensor->img_win.y = 0;
#if 0
        g_psensor->img_win.width = 1920;
        g_psensor->img_win.height = 1080;
#else
        g_psensor->out_win.width = width;
        g_psensor->out_win.height = height;
#endif

        isp_info("1848P,Sensor VMAX=0x%x(%d) , HMAX=0x%x(%d) \n",
            pinfo->VMax ,pinfo->VMax ,pinfo->HMax,pinfo->HMax );
    }
    else{
        g_psensor->out_win.width = 3280;
        g_psensor->out_win.height = 2464;
        g_psensor->img_win.x = 0;
        g_psensor->img_win.y = 0;
#if 0
        g_psensor->img_win.width = 3280;
        g_psensor->img_win.height = 2048;
#else
        g_psensor->out_win.width = width;
        g_psensor->out_win.height = height;
#endif

        isp_info("2464P,Sensor VMAX=0x%x(%d) , HMAX=0x%x(%d) \n",
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
        read_reg(0x203, &reg_l);
        read_reg(0x202, &reg_h);
        num_of_line = ((reg_h & 0xFF) << 8) | (reg_l & 0xFF);
        g_psensor->curr_exp = num_of_line * pinfo->t_row / 1000;
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 exp_line;
    int ret = 0;

    exp_line = MAX(1, exp * 1000 / pinfo->t_row);

    if (exp_line <= pinfo->VMax-4) {
        write_reg(0x203, (exp_line & 0xFF));
        write_reg(0x202, ((exp_line >> 8) & 0xFF));
        if (pinfo->long_exp) {
            write_reg(0x341, (pinfo->VMax & 0xFF));
            write_reg(0x340, ((pinfo->VMax >> 8) & 0xFF));
            pinfo->long_exp = 0;
        }
    } else {
        write_reg(0x203, (exp_line & 0xFF));
        write_reg(0x202, ((exp_line >> 8) & 0xFF));
        write_reg(0x341, ((exp_line+4) & 0xFF));
        write_reg(0x340, (((exp_line+4) >> 8) & 0xFF));
        pinfo->long_exp = 1;
    }

    g_psensor->curr_exp = MAX(1, exp_line * pinfo->t_row / 1000);

    return ret;
}

static u32 get_gain(void)
{
    u32 reg_l, reg_h0, reg_h1, i;

    if (!g_psensor->curr_gain) {
        read_reg(0x205, &reg_l);
        read_reg(0x20f, &reg_h0);
        read_reg(0x20e, &reg_h1);
        for (i=0; i<NUM_OF_GAINSET; i++) {
            if (gain_table[i].ad_gain == reg_l && gain_table[i].digital_gain == ((reg_h1 << 8) | reg_h0))
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
    write_reg(0x205, reg);
    reg = gain_table[i-1].digital_gain & 0xFF;
    write_reg(0x20f, reg);
    write_reg(0x211, reg);
    write_reg(0x213, reg);
    write_reg(0x215, reg);
    reg = (gain_table[i-1].digital_gain >> 8) & 0xFF;
    write_reg(0x20e, reg);
    write_reg(0x210, reg);
    write_reg(0x212, reg);
    write_reg(0x214, reg);

    g_psensor->curr_gain = gain_table[i-1].gain_x;

    return ret;
}

static void update_bayer_type(void)
{
#if 1
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
#endif
}


static int get_mirror(void)
{
	u32 reg;

	read_reg(0x101, &reg);

	return reg & 0x01;
}

static int set_mirror(int enable)
{
    	sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
	u32 reg;

	pinfo->mirror = enable;
	read_reg(0x101, &reg);
	if (enable)
		reg |= 0x01;
	else
		reg &= ~0x01;
	write_reg(0x101, reg);

    	return 0;
}

static int get_flip(void)
{
	u32 reg;

	read_reg(0x101, &reg);

	return reg & 0x02;
}

static int set_flip(int enable)
{
    	sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
	u32 reg;

	pinfo->flip = enable;
	read_reg(0x101, &reg);
	if (enable)
		reg |= 0x02;
	else
		reg &= ~0x02;
	write_reg(0x101, reg);

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
	 update_bayer_type();
        break;
    case ID_FLIP:
        set_flip((int)arg);
	 update_bayer_type();
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

    if (g_psensor->img_win.width<=1920 && g_psensor->img_win.height<=1080 ) {
        pInitTable=sensor_1080P_init_regs;
        InitTable_Num=NUM_OF_1080P_INIT_REGS;
    }
    else if (g_psensor->img_win.width<=3280 && g_psensor->img_win.height<=1848 ) {
        pInitTable=sensor_1848P_init_regs;
        InitTable_Num=NUM_OF_1848P_INIT_REGS;
    }
    else {  //3280x2464
        pInitTable=sensor_2464P_init_regs;
        InitTable_Num=NUM_OF_2464P_INIT_REGS;
    }

    isp_info("start initial...\n");    
    // set initial registers
    for (i=0; i<InitTable_Num; i++) {
         if(write_reg(pInitTable[i].addr, pInitTable[i].val) < 0) {
                isp_err("%s init fail!!", SENSOR_NAME);
                return -EINVAL;
         }
     }

    g_psensor->pclk = get_pclk(g_psensor->xclk);

	pinfo->mirror = mirror;
	pinfo->flip = flip;

//	set_mirror(pinfo->mirror);
//	set_flip(pinfo->flip);
//	update_bayer_type();

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
static void imx179_deconstruct(void)
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

static int imx179_construct(u32 xclk, u16 width, u16 height)
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
    g_psensor->min_gain = gain_table[0].gain_x;
    g_psensor->max_gain = gain_table[NUM_OF_GAINSET - 1].gain_x;
    g_psensor->exp_latency = 1;
    g_psensor->gain_latency = 0;
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

//    if ((ret = init()) < 0)
//        goto construct_err;

    return 0;

construct_err:
    imx179_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init imx179_init(void)
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

    if ((ret = imx179_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit imx179_exit(void)
{
    isp_unregister_sensor(g_psensor);
    imx179_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(imx179_init);
module_exit(imx179_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor IMX179");
MODULE_LICENSE("GPL");
