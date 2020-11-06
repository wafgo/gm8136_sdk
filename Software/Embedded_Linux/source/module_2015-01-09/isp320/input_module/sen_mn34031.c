/**
 * @file sen_mn34031.c
 * Panasonic MN34031 sensor driver
 *
 * Copyright (C) 2010 GM Corp. (http://www.grain-media.com)
 *
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

#define PFX "sen_mn34031"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"
#include "spi_common.h"

#define FP6(f)  ((u32)(f*(1<<6)+0.5))
//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1280
#define DEFAULT_IMAGE_HEIGHT    960
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

// global
//============================================================================
#define SENSOR_NAME         "MN34031"
#define SENSOR_MAX_WIDTH    1280
#define SENSOR_MAX_HEIGHT   1024

#define H_CYCLE 1688
#define V_CYCLE 1066
#define H_VALID 1376
#define V_VALID 1052
#define ALPHA   1   // 0.64(H)

static sensor_dev_t *g_psensor = NULL;

typedef struct sensor_info {
    int is_init;
    u32 t_row;          // unit: 1/10 us
    int VBLK;
    int vert_cycle;
} sensor_info_t;

typedef struct sensor_reg {
    u16 addr;
    u16 val;
} sensor_reg_t;

static sensor_reg_t sensor_init_regs[] = {
    {0x0000, 0x0003},
    {0x0003, 0x8220},
    {0x0008, 0x0008},
    {0x0001, 0x0030},   // 54MHz
    {0x0002, 0x0002},
    {0x0004, 0x0546},
    {0x0000, 0x0013},

    // waiting PLL stable
    {0xFFFF, 0x0000},

    // Reset release of Voltage up/down convert circuit
    {0x0005, 0x0000},

    // waiting for the stablizing Voltage up/down convert circuit
    {0xFFFF, 0x0000},

    // initialize
    {0x0020, 0x0080},
    {0x0021, 0x000a},
    {0x0022, 0x0000},
    {0x0023, 0x000F},
    {0x0024, 0x0000},
    {0x0025, 0x0000},
    {0x0026, 0x0000},
    {0x0027, 0x0000},
    {0x0028, 0x0008},
    {0x0029, 0x0000},
    {0x0030, 0x5420},
    {0x0031, 0x111A},
    {0x0032, 0x0460},
    {0x0033, 0x0000},
    {0x0034, 0x0000},
    {0x0035, 0x20E0},
    {0x0036, 0x0400},
    {0x0037, 0x0000},
    {0x003E, 0x014E},
    {0x003F, 0x0000},
    {0x0040, 0x0000},
    {0x0041, 0x0200},
    {0x0042, 0x0000},
    {0x0043, 0x7057},
    {0x0046, 0x0000},
    {0x004C, 0x0000},
    {0x004E, 0x0FFF},
    {0x004F, 0x0000},
    {0x0050, 0x0FFE},
    {0x0057, 0x0000},
    {0x005C, 0x0038},
    {0x0060, 0x0000},
    {0x0061, 0x0200},
    {0x0062, 0x0000},
    {0x0063, 0x7057},
    {0x0066, 0x0000},
    {0x006C, 0x0000},
    {0x006D, 0x0000},
    {0x006E, 0x0FFF},
    {0x006F, 0x0000},
    {0x0070, 0x0FFE},
    {0x0077, 0x0000},
    {0x007C, 0x0038},
    {0x0080, 0x000C},
    {0x0081, 0xFFFF},
    {0x0082, 0x0000},
    {0x0083, 0x0000},
    {0x0084, 0x0000},
    {0x0085, 0x0004},
    {0x0086, 0x1032},
    {0x0087, 0x0000},
    {0x0088, 0x0001},
    {0x0089, 0x0000},
    {0x0090, 0x0012},
    {0x0091, 0x0013},
    {0x0092, 0x0010},
    {0x0093, 0x0011},
    {0x0094, 0x0016},
    {0x0095, 0x0017},
    {0x0096, 0x0014},
    {0x0097, 0x0015},
    {0x0098, 0x0002},
    {0x0099, 0x0003},
    {0x009A, 0x0000},
    {0x009B, 0x0001},
    {0x009C, 0x0006},
    {0x009D, 0x0007},
    {0x009E, 0x0004},
    {0x009F, 0x0005},
    {0x00A0, 0x0000},
    {0x00A1, 0x034b},
    {0x00A2, 0x0000},
    {0x00A3, 0x0000},
    {0x00A4, 0x0000},
    {0x00A9, 0x0000},
    {0x00C0, 0x1155},
    {0x00C1, 0xC5D5},
    {0x00C2, 0x93D5},
    {0x00C3, 0x01FC},
    {0x00C4, 0xF041},
    {0x00C5, 0x7FC4},
    {0x00C6, 0x0F54},
    {0x00C7, 0x0051},
    {0x00C8, 0x1541},
    {0x00C9, 0x0200},
    {0x00CA, 0x0100},
    {0x00CB, 0x0000},
    {0x00CC, 0x0000},
    {0x00CD, 0x0000},
    {0x00CE, 0x0000},
    {0x00D1, 0x041B},
    {0x00D2, 0x0000},
    {0x00D3, 0x041B},

    // TG setting
    {0x0100, 0x0697},
    {0x0101, 0x0697},
    {0x0102, 0x0697},
    {0x0103, 0x01A5},
    {0x0104, 0x0000},
    {0x0105, 0x0000},
    {0x0106, 0x0000},
    {0x0107, 0x0000},
    {0x0108, 0x0008},
    {0x0109, 0x0009},
    {0x010A, 0x0696},
    {0x010B, 0x0008},
    {0x010C, 0x0000},
    {0x010D, 0x0008},
    {0x010E, 0x0009},
    {0x010F, 0x0696},
    {0x0110, 0x0000},
    {0x0111, 0x0003},
    {0x0112, 0x0009},
    {0x0113, 0x0696},
    {0x0114, 0x0330},
    {0x0115, 0x0333},
    {0x0116, 0x033F},
    {0x0117, 0x05EF},
    {0x0118, 0x0000},
    {0x0119, 0x0005},
    {0x011A, 0x001F},
    {0x011B, 0x0000},
    {0x011C, 0x0005},
    {0x011D, 0x0013},
    {0x011E, 0x001D},
    {0x011F, 0x001F},
    {0x0120, 0x0000},
    {0x0121, 0x0007},
    {0x0122, 0x0006},
    {0x0123, 0x0000},
    {0x0124, 0x033F},
    {0x0125, 0x05F9},
    {0x0126, 0x0002},
    {0x0127, 0x068F},
    {0x0130, 0x0000},
    {0x0131, 0x0000},
    {0x0132, 0x0000},
    {0x0133, 0x0000},
    {0x0134, 0x0000},
    {0x0135, 0x0605},
    {0x0136, 0x0341},
    {0x0137, 0x0000},
    {0x0138, 0x0000},
    {0x0139, 0x0000},
    {0x013A, 0x0000},
    {0x0140, 0x0009},
    {0x0141, 0x00D0},
    {0x0142, 0x0007},
    {0x0143, 0x030D},
    {0x0144, 0x0008},
    {0x0145, 0x00CF},
    {0x0146, 0x0006},
    {0x0147, 0x030C},
    {0x0148, 0x0000},
    {0x0149, 0x0000},
    {0x014A, 0x0008},
    {0x014B, 0x0006},
    {0x014C, 0x0000},
    {0x014D, 0x0002},
    {0x014E, 0x0002},
    {0x014F, 0x0007},
    {0x0150, 0x0008},
    {0x0151, 0x0006},
    {0x0152, 0x0009},
    {0x0160, 0x0001},
    {0x0161, 0x0004},
    {0x0162, 0x01A5},
    {0x0163, 0x0001},
    {0x0164, 0x003B},
    {0x0165, 0x004B},
    {0x0166, 0x0053},
    {0x0167, 0x0197},
    {0x0168, 0x01A4},
    {0x0169, 0x01A5},
    {0x016A, 0x0000},
    {0x016B, 0x0000},
    {0x0170, 0x0327},
    {0x0171, 0x0335},
    {0x0172, 0x0000},
    {0x0173, 0x0000},
    {0x0174, 0x0000},
    {0x0175, 0x0000},
    {0x0176, 0x0000},
    {0x0177, 0x0000},
    {0x0178, 0x0000},
    {0x0179, 0x0000},
    {0x017C, 0x0000},
    {0x017D, 0x0000},
    {0x017E, 0x0000},
    {0x017F, 0x0000},
    {0x0190, 0x0007},
    {0x0191, 0x0181},
    {0x0192, 0x0047},
    {0x0193, 0x0049},
    {0x0194, 0x004A},
    {0x0195, 0x004B},
    {0x0196, 0x004F},//HBLK_H
    {0x0197, 0x0002},//HBLK_L
    {0x0198, 0x0002},
    {0x0199, 0x0002},
    {0x019A, 0x004F},
    {0x019B, 0x0002},
    {0x01A0, 0x0429},//Vertical line
    {0x01A1, 0x0000},
    {0x01A2, 0x0429},
    {0x01A3, 0x041A},
    {0x01A4, 0x0429},
    {0x01A5, 0x041A},
    {0x01A6, 0x0000},
    {0x01A7, 0x041B},
    {0x01A8, 0x0001},
    {0x01A9, 0x0009},
    {0x01AA, 0x0008},
    {0x01AB, 0x0009},
    {0x01AC, 0x0000},
    {0x01AD, 0x041C},
    {0x01AE, 0x0000},
    {0x01AF, 0x0000},
    {0x01B0, 0x0000},
    {0x01B1, 0x041C},
    {0x01B2, 0x0000},
    {0x01B3, 0x0000},
    {0x01C0, 0x0010},
    {0x01C1, 0x0417},
    {0x01C2, 0x0010},
    {0x01C3, 0x0417},
    {0x01C4, 0x0000},
    {0x01C5, 0x0000},
    {0x01C6, 0x0000},
    {0x01C7, 0x0000},
    {0x01C8, 0x000F},
    {0x01D0, 0x0000},
    {0x01D1, 0x0000},
    {0x01D2, 0x0000},
    {0x01D3, 0x0000},
    {0x01E0, 0x000A},
    {0x01E1, 0x0023},
    {0x01E2, 0x0023},
    {0x01E3, 0x0023},
    {0x01E4, 0x000E},
    {0x01E5, 0x000E},
    {0x01E6, 0x000E},
    {0x01E7, 0x0028},
    {0x01E8, 0x006B},
    {0x01E9, 0x006B},
    {0x01EA, 0x006B},
    {0x01EB, 0x0087},
    {0x01EC, 0x00CF},
    {0x01ED, 0x00D8},
    {0x01EE, 0x0106},
    {0x01EF, 0x0316},
    {0x01F0, 0x0003},
    {0x01F1, 0x001F},
    {0x01F2, 0x00B3},
    {0x01F3, 0x01BC},

    // release TG reset
    {0x0000, 0x0113},
    // setting of clock frequency dividing and parallel output
    {0x0003, 0x8260}
};
#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))

typedef struct gain_set {
    u16 analog;
    u16 digit;
    u32 gain_x;
} gain_set_t;

static const gain_set_t gain_table[] = {
    {0x0080, 0x0a, FP6(  1.1140)}, {0x0080, 0x0c, FP6(  1.1383)}, {0x0080,  0x0e, FP6(  1.1631)},
    {0x0080, 0x10, FP6(  1.1885)}, {0x0080, 0x12, FP6(  1.2144)}, {0x0080,  0x14, FP6(  1.2409)},
    {0x0080, 0x16, FP6(  1.2680)}, {0x0080, 0x18, FP6(  1.2957)}, {0x0080,  0x1a, FP6(  1.3240)},
    {0x0080, 0x1c, FP6(  1.3529)}, {0x0080, 0x1e, FP6(  1.3824)}, {0x0080,  0x20, FP6(  1.4125)},
    {0x0080, 0x22, FP6(  1.4434)}, {0x0080, 0x24, FP6(  1.4749)}, {0x0080,  0x26, FP6(  1.5070)},
    {0x0080, 0x28, FP6(  1.5399)}, {0x0080, 0x2a, FP6(  1.5735)}, {0x0080,  0x2c, FP6(  1.6079)},
    {0x0080, 0x2e, FP6(  1.6430)}, {0x0080, 0x30, FP6(  1.6788)}, {0x0080,  0x32, FP6(  1.7154)},
    {0x0080, 0x34, FP6(  1.7529)}, {0x0080, 0x36, FP6(  1.7911)}, {0x0080,  0x38, FP6(  1.8302)},
    {0x0080, 0x3a, FP6(  1.8701)}, {0x0080, 0x3c, FP6(  1.9110)}, {0x0080,  0x3e, FP6(  1.9527)},
    {0x0080, 0x40, FP6(  1.9953)}, {0x0080, 0x42, FP6(  2.0388)}, {0x0080,  0x44, FP6(  2.0833)},
    {0x0080, 0x46, FP6(  2.1288)}, {0x0080, 0x48, FP6(  2.1752)}, {0x2080,  0x0a, FP6(  2.2074)},
    {0x2080, 0x0c, FP6(  2.2555)}, {0x2080, 0x0e, FP6(  2.3048)}, {0x2080,  0x10, FP6(  2.3550)},
    {0x2080, 0x12, FP6(  2.4064)}, {0x2080, 0x14, FP6(  2.4590)}, {0x2080,  0x16, FP6(  2.5126)},
    {0x2080, 0x18, FP6(  2.5674)}, {0x2080, 0x1a, FP6(  2.6235)}, {0x2080,  0x1c, FP6(  2.6807)},
    {0x2080, 0x1e, FP6(  2.7392)}, {0x2080, 0x20, FP6(  2.7990)}, {0x2080,  0x22, FP6(  2.8601)},
    {0x2080, 0x24, FP6(  2.9225)}, {0x2080, 0x26, FP6(  2.9862)}, {0x2080,  0x28, FP6(  3.0514)},
    {0x2080, 0x2a, FP6(  3.1180)}, {0x2080, 0x2c, FP6(  3.1860)}, {0x2080,  0x2e, FP6(  3.2556)},
    {0x2080, 0x30, FP6(  3.3266)}, {0x2080, 0x32, FP6(  3.3992)}, {0x2080,  0x34, FP6(  3.4734)},
    {0x2080, 0x36, FP6(  3.5492)}, {0x2080, 0x38, FP6(  3.6266)}, {0x2080,  0x3a, FP6(  3.7057)},
    {0x2080, 0x3c, FP6(  3.7866)}, {0x2080, 0x3e, FP6(  3.8692)}, {0x2080,  0x40, FP6(  3.9537)},
    {0x2080, 0x42, FP6(  4.0399)}, {0x2080, 0x44, FP6(  4.1281)}, {0x2080,  0x46, FP6(  4.2182)},
    {0x2080, 0x48, FP6(  4.3102)}, {0x6080, 0x0a, FP6(  4.3439)}, {0x6080,  0x0c, FP6(  4.4386)},
    {0x6080, 0x0e, FP6(  4.5355)}, {0x6080, 0x10, FP6(  4.6345)}, {0x6080,  0x12, FP6(  4.7356)},
    {0x6080, 0x14, FP6(  4.8389)}, {0x6080, 0x16, FP6(  4.9445)}, {0x6080,  0x18, FP6(  5.0524)},
    {0x6080, 0x1a, FP6(  5.1627)}, {0x6080, 0x1c, FP6(  5.2753)}, {0x6080,  0x1e, FP6(  5.3904)},
    {0x6080, 0x20, FP6(  5.5081)}, {0x6080, 0x22, FP6(  5.6283)}, {0x6080,  0x24, FP6(  5.7511)},
    {0x6080, 0x26, FP6(  5.8766)}, {0x6080, 0x28, FP6(  6.0048)}, {0x6080,  0x2a, FP6(  6.1359)},
    {0x6080, 0x2c, FP6(  6.2697)}, {0x6080, 0x2e, FP6(  6.4066)}, {0x6080,  0x30, FP6(  6.5464)},
    {0x6080, 0x32, FP6(  6.6892)}, {0x6080, 0x34, FP6(  6.8352)}, {0x6080,  0x36, FP6(  6.9843)},
    {0x6080, 0x38, FP6(  7.1367)}, {0x6080, 0x3a, FP6(  7.2925)}, {0x6080,  0x3c, FP6(  7.4516)},
    {0x6080, 0x3e, FP6(  7.6142)}, {0x6080, 0x40, FP6(  7.7804)}, {0x6080,  0x42, FP6(  7.9501)},
    {0x6080, 0x44, FP6(  8.1236)}, {0x6080, 0x46, FP6(  8.3009)}, {0xe080,  0x0a, FP6(  8.4504)},
    {0xe080, 0x0c, FP6(  8.6348)}, {0xe080, 0x0e, FP6(  8.8232)}, {0xe080,  0x10, FP6(  9.0157)},
    {0xe080, 0x12, FP6(  9.2124)}, {0xe080, 0x14, FP6(  9.4135)}, {0xe080,  0x16, FP6(  9.6189)},
    {0xe080, 0x18, FP6(  9.8288)}, {0xe080, 0x1a, FP6( 10.0433)}, {0xe080,  0x1c, FP6( 10.2624)},
    {0xe080, 0x1e, FP6( 10.4864)}, {0xe080, 0x20, FP6( 10.7152)}, {0xe080,  0x22, FP6( 10.9490)},
    {0xe080, 0x24, FP6( 11.1879)}, {0xe080, 0x26, FP6( 11.4321)}, {0xe080,  0x28, FP6( 11.6815)},
    {0xe080, 0x2a, FP6( 11.9364)}, {0xe080, 0x2c, FP6( 12.1969)}, {0xe080,  0x2e, FP6( 12.4631)},
    {0xe080, 0x30, FP6( 12.7350)}, {0xe080, 0x32, FP6( 13.0129)}, {0xe080,  0x34, FP6( 13.2969)},
    {0xe080, 0x36, FP6( 13.5870)}, {0xe080, 0x38, FP6( 13.8835)}, {0xe080,  0x3a, FP6( 14.1865)},
    {0xe080, 0x3c, FP6( 14.4961)}, {0xe080, 0x3e, FP6( 14.8124)}, {0xe080,  0x40, FP6( 15.1356)},
    {0xe080, 0x42, FP6( 15.4659)}, {0xe080, 0x44, FP6( 15.8034)}, {0xe080,  0x46, FP6( 16.1482)},
    {0xe080, 0x48, FP6( 16.5006)}, {0xe080, 0x4a, FP6( 16.8607)}, {0xe080,  0x4c, FP6( 17.2286)},
    {0xe080, 0x4e, FP6( 17.6046)}, {0xe080, 0x50, FP6( 17.9887)}, {0xe080,  0x52, FP6( 18.3812)},
    {0xe080, 0x54, FP6( 18.7824)}, {0xe080, 0x56, FP6( 19.1922)}, {0xe080,  0x58, FP6( 19.6110)},
    {0xe080, 0x5a, FP6( 20.0390)}, {0xe080, 0x5c, FP6( 20.4762)}, {0xe080,  0x5e, FP6( 20.9231)},
    {0xe080, 0x60, FP6( 21.3796)}, {0xe080, 0x62, FP6( 21.8462)}, {0xe080,  0x64, FP6( 22.3229)},
    {0xe080, 0x66, FP6( 22.8100)}, {0xe080, 0x68, FP6( 23.3077)}, {0xe080,  0x6a, FP6( 23.8163)},
    {0xe080, 0x6c, FP6( 24.3360)}, {0xe080, 0x6e, FP6( 24.8671)}, {0xe080,  0x70, FP6( 25.4097)},
    {0xe080, 0x72, FP6( 25.9642)}, {0xe080, 0x74, FP6( 26.5308)}, {0xe080,  0x76, FP6( 27.1097)},
    {0xe080, 0x78, FP6( 27.7013)}, {0xe080, 0x7a, FP6( 28.3058)}, {0xe080,  0x7c, FP6( 28.9234)},
    {0xe080, 0x7e, FP6( 29.5546)}, {0xe080, 0x80, FP6( 30.1995)}, {0xe080,  0x82, FP6( 30.8585)},
    {0xe080, 0x84, FP6( 31.5319)}, {0xe080, 0x86, FP6( 32.2200)}, {0xe080,  0x88, FP6( 32.9230)},
    {0xe080, 0x8a, FP6( 33.6415)}, {0xe080, 0x8c, FP6( 34.3756)}, {0xe080,  0x8e, FP6( 35.1257)},
    {0xe080, 0x90, FP6( 35.8922)}, {0xe080, 0x92, FP6( 36.6754)}, {0xe080,  0x94, FP6( 37.4757)},
    {0xe080, 0x96, FP6( 38.2935)}, {0xe080, 0x98, FP6( 39.1291)}, {0xe080,  0x9a, FP6( 39.9830)},
    {0xe080, 0x9c, FP6( 40.8555)}, {0xe080, 0x9e, FP6( 41.7470)}, {0xe080,  0xa0, FP6( 42.6580)},
    {0xe080, 0xa2, FP6( 43.5888)}, {0xe080, 0xa4, FP6( 44.5400)}, {0xe080,  0xa6, FP6( 45.5119)},
    {0xe080, 0xa8, FP6( 46.5050)}, {0xe080, 0xaa, FP6( 47.5198)}, {0xe080,  0xac, FP6( 48.5568)},
    {0xe080, 0xae, FP6( 49.6164)}, {0xe080, 0xb0, FP6( 50.6991)}, {0xe080,  0xb2, FP6( 51.8054)},
    {0xe080, 0xb4, FP6( 52.9359)}, {0xe080, 0xb6, FP6( 54.0910)}, {0xe080,  0xb8, FP6( 55.2713)},
    {0xe080, 0xba, FP6( 56.4774)}, {0xe080, 0xbc, FP6( 57.7099)}, {0xe080,  0xbe, FP6( 58.9692)},
    {0xe080, 0xc0, FP6( 60.2560)}, {0xe080, 0xc2, FP6( 61.5708)}, {0xe080,  0xc4, FP6( 62.9144)},
    {0xe080, 0xc6, FP6( 64.2873)}, {0xe080, 0xc8, FP6( 65.6901)}, {0xe080,  0xca, FP6( 67.1236)},
    {0xe080, 0xcc, FP6( 68.5883)}, {0xe080, 0xce, FP6( 70.0850)}, {0xe080,  0xd0, FP6( 71.6143)},
    {0xe080, 0xd2, FP6( 73.1771)}, {0xe080, 0xd4, FP6( 74.7739)}, {0xe080,  0xd6, FP6( 76.4056)},
    {0xe080, 0xd8, FP6( 78.0728)}, {0xe080, 0xda, FP6( 79.7765)}, {0xe080,  0xdc, FP6( 81.5173)},
    {0xe080, 0xde, FP6( 83.2962)}, {0xe080, 0xe0, FP6( 85.1138)}, {0xe080,  0xe2, FP6( 86.9711)},
    {0xe080, 0xe4, FP6( 88.8689)}, {0xe080, 0xe6, FP6( 90.8082)}, {0xe080,  0xe8, FP6( 92.7897)},
    {0xe080, 0xea, FP6( 94.8146)}, {0xe080, 0xec, FP6( 96.8835)}, {0xe080,  0xee, FP6( 98.9977)},
    {0xe080, 0xf0, FP6(101.1579)}, {0xe080, 0xf2, FP6(103.3654)}, {0xe080,  0xf4, FP6(105.6209)},
    {0xe080, 0xf6, FP6(107.9257)}, {0xe080, 0xf8, FP6(110.2808)}, {0xe080,  0xfa, FP6(112.6873)},
    {0xe080, 0xfc, FP6(115.1463)}, {0xe080, 0xfe, FP6(117.6590)}, {0xe080, 0x100, FP6(120.2264)},
};
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))
#define AGAIN_LV0_INDEX 0
#define AGAIN_LV1_INDEX 32
#define AGAIN_LV2_INDEX 64
#define AGAIN_LV3_INDEX 95

static u16 g_sensor_reg[0x10];

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

static int read_reg(u32 addr, u32 *pval)
{
    int i;
    u32 tmp;

    if (addr < 0x10) {
        *pval = (u32)g_sensor_reg[addr];
        return 0;
    }

    _delay();
    SPI_CS_CLR();
    _delay();

    // address phase
    tmp = BIT15 | (addr & 0x7FFF);
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
    for (i=0; i<16; i++) {
        SPI_CLK_CLR();
        _delay();
        SPI_CLK_SET();
        _delay();
        if (SPI_DI_READ())
            *pval |= (1 << i);
    }
    SPI_CS_SET();

    return 0;
}

static int write_reg(u32 addr, u32 val)
{
    int i;
    u32 tmp;

    _delay();
    SPI_CS_CLR();
    _delay();

    // address phase & data phase
    tmp = (val << 16) | (addr & 0x7FFF);
    for (i=0; i<32; i++) {
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

    if (addr < 0x10)
        g_sensor_reg[addr] = val;
    return 0;
}

static u32 get_pclk(u32 xclk)
{
    u32 pclk, pll_m, tg_div;

    read_reg(0x01, &pll_m);
    read_reg(0x08, &tg_div);
    pclk = (xclk / 3) * pll_m / tg_div;
    printk("pixel clock %d\n", pclk);

    return pclk;
}

static int set_size(u32 width, u32 height)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;

    // check size
    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("Exceed maximum sensor resolution!\n");
        return -1;
    }

    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    g_psensor->out_win.width = SENSOR_MAX_WIDTH;
    g_psensor->out_win.height = SENSOR_MAX_HEIGHT;

    g_psensor->img_win.x = 86 + (((SENSOR_MAX_WIDTH - width) >> 1) &~BIT0);
    g_psensor->img_win.y = 20 + (((SENSOR_MAX_HEIGHT - height) >> 1) &~BIT0);
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    g_psensor->pclk = get_pclk(g_psensor->xclk);
    pinfo->t_row = (H_CYCLE * 10000) / (g_psensor->pclk / 1000);
    g_psensor->min_exp = (pinfo->t_row + 500) / 1000;
    //printk("min_exp = %d, pinfo->t_row = %d\n", g_psensor->min_exp, pinfo->t_row);

    return ret;
}

static u32 get_exp(void)
{
    u32 lines, shtpos, tg_lcf;

    if (!g_psensor->curr_exp) {
        sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
        read_reg(0xA1, &shtpos);
        read_reg(0xA3, &tg_lcf);
        // exposure = r_tg_lcf_v(V) + {1(V) - 2(H) - shtpos(H) + alpha(H)}
        lines = (tg_lcf * pinfo->vert_cycle) + pinfo->vert_cycle - 2 - shtpos + ALPHA;
        g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;
    }
    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 lines, tg_lcf, shutpos;

    // exposure = r_tg_lcf_v(V) + {1(V) - 2(H) - shtpos(H) + alpha(H)}
    lines = ((exp * 1000)+pinfo->t_row/2) / pinfo->t_row;

    if (lines == 0)
        lines = 1;

    // tricky: to keep 30fps
    if ((lines > (pinfo->vert_cycle - 1)) && ((lines - pinfo->vert_cycle) < 10)){
        lines = pinfo->vert_cycle - 1;
        //printk("lines=%d,vert_cycle=%d\n",lines, pinfo->vert_cycle);
    }
    if (lines >= pinfo->vert_cycle) {
        tg_lcf = lines / pinfo->vert_cycle;
        lines -= (tg_lcf * pinfo->vert_cycle);
    } else {
        tg_lcf = 0;
    }
    shutpos = (pinfo->vert_cycle - 2 + ALPHA) - lines;
    write_reg(0xA3, tg_lcf);
    write_reg(0xA1, shutpos);

    lines += (tg_lcf * pinfo->vert_cycle);
    g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;

    //printk("lines=%d,tg_lcf=%d,shutpos=%d, pinfo->t_row=%d, exp =%d\n",lines, tg_lcf, shutpos, pinfo->t_row, exp);

    return ret;
}

static u32 get_gain(void)
{
    u32 a_reg, d_reg;
    int idx = 0;

    if (g_psensor->curr_gain == 0){
        read_reg(0x20, &a_reg);
        read_reg(0x21, &d_reg);

        switch (a_reg) {
            case 0x0080:
                idx = AGAIN_LV0_INDEX;
                break;
            case 0x2080:
                idx = AGAIN_LV1_INDEX;
                break;
            case 0x6080:
                idx = AGAIN_LV2_INDEX;
                break;
            case 0xE080:
                idx = AGAIN_LV3_INDEX;
                break;
        }
        idx += ((d_reg - 0xa) >> 1);
        g_psensor->curr_gain = gain_table[idx].gain_x;
    }
    //printk("g_psensor->curr_gain = %d\n", g_psensor->curr_gain);

    return g_psensor->curr_gain;
}

static int set_gain(u32 gain)
{
    int ret = 0;
    u32 ang_x, dig_x;
    int i;

    // search most suitable gain into gain table
    ang_x = gain_table[0].analog;
    dig_x = gain_table[0].digit;

    for (i=0; i<NUM_OF_GAINSET; i++) {
        if (gain_table[i].gain_x > gain)
            break;
    }
    if (i > 0) {
        ang_x = gain_table[i-1].analog;
        dig_x = gain_table[i-1].digit;
    }

    write_reg(0x20, ang_x);
    write_reg(0x21, dig_x);
    g_psensor->curr_gain = gain_table[i-1].gain_x;

    return ret;
}

static int get_mirror(void)
{
    return 0;
}

static int set_mirror(int enable)
{
    return 0;
}

static int get_flip(void)
{
    return 0;
}

static int set_flip(int enable)
{
    return 0;
}

static void adjust_vblk(int fps)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if((fps >= 8) && (fps < 31)){
        pinfo->VBLK = (g_psensor->pclk / fps / H_CYCLE) - 1;
	    pinfo->vert_cycle = pinfo->VBLK;
        write_reg(0x1A0, pinfo->VBLK);
        write_reg(0x2A0, pinfo->VBLK);
        //printk("t_row = %d, min_exposure =%d, pclk=%d, VBLK=%d\n", pinfo->t_row, g_psensor->min_exp, g_psensor->pclk, pinfo->VBLK);
    }

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
    int ret = 0;
    int i;

    if (pinfo->is_init)
        return 0;

    for (i=0; i<NUM_OF_INIT_REGS; i++) {
        if (sensor_init_regs[i].addr == 0xFFFF) {
            mdelay(30);
        } else {
            if (write_reg(sensor_init_regs[i].addr, sensor_init_regs[i].val) < 0) {
                isp_err("%s init fail!!", SENSOR_NAME);
                return -EINVAL;
            }
        }
    }

    pinfo->vert_cycle = V_CYCLE;

    //printk("pinfo->vert_cycle = %d\n", pinfo->vert_cycle);

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

//============================================================================
// external functions
//=============================================================================
static void mn34031_deconstruct(void)
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

static int mn34031_construct(u32 xclk, u16 width, u16 height)
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

    // allocate ae private data
    pinfo = kzalloc(sizeof(sensor_info_t), GFP_KERNEL);
    if (!pinfo) {
        isp_err("failed to allocate private data!\n");
        return -ENOMEM;
    }

    g_psensor->private = pinfo;

    snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, SENSOR_NAME);
    g_psensor->capability = SUPPORT_MIRROR | SUPPORT_FLIP;
    g_psensor->xclk = xclk;
    g_psensor->fmt = RAW12;
    g_psensor->bayer_type = BAYER_GR;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_HIGH;
    g_psensor->inv_clk = inv_clk;
    g_psensor->img_win.x = 86;
    g_psensor->img_win.y = 20;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;
    g_psensor->min_exp = 1;
    g_psensor->max_exp = 5000; // 0.5 sec
    g_psensor->min_gain = FP6(1.1140);
    g_psensor->max_gain = FP6(120.2264);
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
    mn34031_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init mn34031_init(void)
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

    if ((ret = mn34031_construct(sensor_xclk, sensor_w, sensor_h)) < 0){
        printk("mn34031_construct Error \n");
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

static void __exit mn34031_exit(void)
{
    isp_unregister_sensor(g_psensor);
    mn34031_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(mn34031_init);
module_exit(mn34031_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor mn34031");
MODULE_LICENSE("GPL");
