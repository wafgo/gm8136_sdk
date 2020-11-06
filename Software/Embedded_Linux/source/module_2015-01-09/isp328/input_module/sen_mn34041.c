/**
 * @file sen_mn34041.c
 * Panasonic MN34041 sensor driver
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

#define PFX "sen_mn34041"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"
#include "spi_common.h"

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

static uint flip = 0;
module_param(flip, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(flip, "sensor flip function");

static uint ch_num = 4;
module_param(ch_num, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_num, "channel number");

static uint fps = 30;
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "channel number");

//=============================================================================
// global
//=============================================================================
#define SENSOR_NAME         "mn34041"
#define SENSOR_MAX_WIDTH    1944
#define SENSOR_MAX_HEIGHT   1092

#define H_CYCLE 2400
#define V_CYCLE 1125
#define H_VALID 2016
#define V_VALID 1108
#define ALPHA   1   // 0.64(H)

int vert_cycle = V_CYCLE;

#define FPGA
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
    u32 ang_reg_1;
    u32 dcg_reg;
} sensor_info_t;

typedef struct sensor_reg {
    u16  addr;
    u16 val;
} sensor_reg_t;

static sensor_reg_t sensor_init_regs[] = {
    {0x0001,0x001B},  // 81M/Pixel for 30 fps
//    {0x0001,0x0036},  // 162M/Pixel for 60 fps
    {0x0002,0x0002},
    {0x0004,0x01c2},
    {0x0000,0x0003},
    {0x0003,0x0209},
    {0x0006,0x442a},
    {0x0005,0x0000},
    {0xFFFF,0x03e8}, //delay 10ms
    {0x0000,0x0013},

    {0x0036,0x0021},
    {0x0037,0x0300},
    {0x0020,0x0080},
    {0x0021,0x0080},
    {0x0022,0x0000},
    {0x0023,0x0000},
    {0x0024,0x0010},
    {0x0025,0x0011},
    {0x0026,0x0012},
    {0x0027,0x0013},
    {0x0028,0x0000},
    {0x0030,0x0424},
    {0x0031,0x110A},
    {0x0032,0x7450},
    {0x0033,0x0000},
    {0x0034,0x0000},
    {0x0035,0x0281},
    {0x0038,0x0001},
    {0x0039,0x0210},
    {0x003a,0x0333},
    {0x003b,0x0111},
    {0x003f,0x0000},
    {0x0040,0x0004},
    {0x0041,0x0200},
    {0x0042,0x0100},
    {0x0043,0x0070},
    {0x0044,0x0000},
    {0x0045,0x0000},
    {0x0046,0x0000},
    {0x0047,0x0010},
    {0x0048,0x0000},
    {0x0049,0x0002},
    {0x004a,0x0ffe},
    {0x004b,0x0004},
    {0x0056,0x0000},
    {0x0057,0x1fff},
    {0x0058,0x2000},
    {0x0059,0x0000},
    {0x005a,0x0011},
    {0x0070,0x2084},
    {0x0071,0xffff},
    {0x0072,0x0000},
    {0x0073,0x0000},
    {0x0074,0x0000},
    {0x0075,0x0004},
    {0x0076,0x0230},
    {0x0077,0x0541},
    {0x0078,0x0001},
    {0x0079,0x0011},
    {0x0080,0x0002},
    {0x0081,0x0003},
    {0x0082,0x0000},
    {0x0083,0x0001},
    {0x0084,0x0012},
    {0x0085,0x0013},
    {0x0086,0x0010},
    {0x0087,0x0011},
    {0x0088,0x000a},
    {0x0089,0x000b},
    {0x008a,0x0008},
    {0x008b,0x0009},
    {0x008c,0x0006},
    {0x008d,0x0007},
    {0x008e,0x0004},
    {0x008f,0x0005},
    {0x0090,0x0016},
    {0x0091,0x0017},
    {0x0092,0x0014},
    {0x0093,0x0015},
    {0x0094,0x001a},
    {0x0095,0x001b},
    {0x0096,0x0018},
    {0x0097,0x0019},
    {0x00a0,0x3000},
    {0x00a1,0x0000},
    {0x00a2,0x0002},
    {0x00a3,0x0000},
    {0x00a4,0x0000},
    {0x00a5,0x0000},
    {0x00a6,0x0000},
    {0x00a7,0x0000},
    {0x00a8,0x000f},
    {0x00a9,0x0022},
    {0x00c0,0x5540},
    {0x00c1,0x5fd5},
    {0x00c2,0xf757},
    {0x00c3,0xdf5f},
    {0x00c4,0x208a},
    {0x00c5,0x0071},
    {0x00c6,0x0557},
    {0x00c7,0x0000},
    {0x00ca,0x0080},
    {0x00cb,0x0000},
    {0x00cc,0x0000},
    {0x00cd,0x0000},
    {0x00ce,0x0000},
    {0x0100,0x03a8},
    {0x0101,0x03a8},
    {0x0102,0x02c0},
    {0x0103,0x037a},
    {0x0104,0x002b},
    {0x0105,0x00de},
    {0x0106,0x00fa},
    {0x0170,0x0002},
    {0x0171,0x000d},
    {0x0172,0x0007},
    {0x0173,0x0004},
    {0x0174,0x002a},
    {0x0175,0x0062},
    {0x0176,0x0079},
    {0x0177,0x0326},
    {0x0178,0x0003},
    {0x0179,0x0033},
    {0x017a,0x0360},
    {0x017b,0x0002},
    {0x017c,0x000d},
    {0x0190,0x0000},
    {0x0191,0x0000},
    {0x0192,0x0000},
    {0x0193,0x0000},
    {0x0194,0x0000},
    {0x0195,0x0000},
    {0x0196,0x0000},
    {0x0197,0x01ba},
    {0x0198,0xb060},
    {0x0199,0x7c0a},
    {0x019a,0x0000},
    {0x019b,0x0313},
    {0x019c,0x0b08},
    {0x019d,0x3906},
    {0x019e,0x0000},
    {0x01a0,0x0464},
    {0x01a1,0x0000},
    {0x01a2,0x0000},
    {0x01a3,0x0464},
    {0x01a4,0x0000},
    {0x01a5,0x0453},
    {0x01a6,0x0000},
    {0x01a7,0x0464},
    {0x01a8,0x0000},
    {0x01a9,0x0454},
    {0x01aa,0x0000},
    {0x01ab,0x0000},
    {0x01ac,0x0000},
    {0x01ad,0x0454},
    {0x01ae,0x0000},
    {0x01af,0x0000},
    {0x01b0,0x0000},
    {0x01b1,0x0454},
    {0x01b2,0x0000},
    {0x01b3,0x0000},
    {0x01b4,0x0000},
    {0x01b5,0x0454},
    {0x01b6,0x0000},
    {0x01b7,0x0000},
    {0x01b8,0x0000},
    {0x01b9,0x0453},
    {0x01ba,0x0000},
    {0x01bb,0x0000},
    {0x01bc,0x0000},
    {0x01bd,0x0453},
    {0x01be,0x0000},
    {0x01c4,0x0000},
    {0x01c5,0x0000},
    {0x01c6,0x0011},
    {0x0200,0x03a8},
    {0x0201,0x03a8},
    {0x0202,0x02c0},
    {0x0203,0x037a},
    {0x0204,0x002b},
    {0x0205,0x00de},
    {0x0206,0x00fa},
    {0x0270,0x0002},
    {0x0271,0x000d},
    {0x0272,0x0007},
    {0x0273,0x0004},
    {0x0274,0x002a},
    {0x0275,0x0062},
    {0x0276,0x0079},
    {0x0277,0x0326},
    {0x0278,0x0003},
    {0x0279,0x0033},
    {0x027a,0x0360},
    {0x027b,0x0002},
    {0x027c,0x000d},
    {0x0290,0x0000},
    {0x0291,0x0000},
    {0x0292,0x0000},
    {0x0293,0x0000},
    {0x0294,0x0000},
    {0x0295,0x0000},
    {0x0296,0x0000},
    {0x0297,0x01ba},
    {0x0298,0xb060},
    {0x0299,0x7c0a},
    {0x029a,0x0000},
    {0x029b,0x0313},
    {0x029c,0x0b08},
    {0x029d,0x3906},
    {0x029e,0x0000},
    {0x02a0,0x0464},
    {0x02a1,0x0000},
    {0x02a2,0x0000},
    {0x02a3,0x0464},
    {0x02a4,0x0000},
    {0x02a5,0x0453},
    {0x02a6,0x0000},
    {0x02a7,0x0464},
    {0x02a8,0x0000},
    {0x02a9,0x0454},
    {0x02aa,0x0000},
    {0x02ab,0x0000},
    {0x02ac,0x0000},
    {0x02ad,0x0454},
    {0x02ae,0x0000},
    {0x02af,0x0000},
    {0x02b0,0x0000},
    {0x02b1,0x0454},
    {0x02b2,0x0000},
    {0x02b3,0x0000},
    {0x02b4,0x0000},
    {0x02b5,0x0454},
    {0x02b6,0x0000},
    {0x02b7,0x0000},
    {0x02b8,0x0000},
    {0x02b9,0x0453},
    {0x02ba,0x0000},
    {0x02bb,0x0000},
    {0x02bc,0x0000},
    {0x02bd,0x0453},
    {0x02be,0x0000},
    {0x02c4,0x0000},
    {0x02c5,0x0000},
    {0x02c6,0x0011},
    {0x0108,0x0000},
    {0x0109,0x000f},
    {0x010a,0x0009},
    {0x010b,0x0000},
    {0x010c,0x0016},
    {0x010d,0x0000},
    {0x010e,0x000f},
    {0x010f,0x0000},
    {0x0110,0x0009},
    {0x0111,0x0000},
    {0x0112,0x0016},
    {0x0113,0x0003},
    {0x0114,0x000a},
    {0x0115,0x0000},
    {0x0116,0x0009},
    {0x0117,0x0000},
    {0x0118,0x0016},
    {0x0119,0x0018},
    {0x011a,0x0017},
    {0x011b,0x0000},
    {0x011c,0x0002},
    {0x011d,0x0009},
    {0x011e,0x0012},
    {0x011f,0x0001},
    {0x0120,0x003a},
    {0x0121,0x0000},
    {0x0122,0x0000},
    {0x0123,0x0000},
    {0x0124,0x0011},
    {0x0125,0x0000},
    {0x0126,0x0003},
    {0x0127,0x0003},
    {0x0128,0x0000},
    {0x0129,0x0010},
    {0x012a,0x0000},
    {0x012b,0x0003},
    {0x012c,0x0000},
    {0x012d,0x0011},
    {0x012e,0x0000},
    {0x012f,0x0009},
    {0x0130,0x0009},
    {0x0131,0x0012},
    {0x0132,0x0000},
    {0x0133,0x0000},
    {0x0134,0x0009},
    {0x0135,0x0009},
    {0x0136,0x0012},
    {0x0137,0x0006},
    {0x0138,0x0000},
    {0x0139,0x0010},
    {0x013a,0x0000},
    {0x0140,0x0020},
    {0x0141,0x0036},
    {0x0142,0x0000},
    {0x0143,0x0001},
    {0x0144,0x0003},
    {0x0145,0x0000},
    {0x0146,0x0000},
    {0x0150,0x0011},
    {0x0151,0x0001},
    {0x0152,0x0001},
    {0x0153,0x0001},
    {0x0154,0x0010},
    {0x0155,0x0000},
    {0x0156,0x0003},
    {0x0157,0x0000},
    {0x0158,0x0013},
    {0x0159,0x0000},
    {0x015a,0x0000},
    {0x015b,0x0008},
    {0x015c,0x0000},
    {0x015d,0x0004},
    {0x015e,0x0005},
    {0x015f,0x000a},
    {0x0160,0x0006},
    {0x0161,0x000f},
    {0x0162,0x000d},
    {0x0163,0x0004},
    {0x0164,0x0002},
    {0x0165,0x0002},
    {0x0166,0x0000},
    {0x0167,0x0010},
    {0x0168,0x0009},
    {0x0169,0x0003},
    {0x016a,0x0000},
    {0x016b,0x0000},
    {0x0180,0x0017},
    {0x0181,0x00c5},
    {0x0182,0x0000},
    {0x0183,0x0000},
    {0x0184,0x00fa},
    {0x0185,0x00a5},
    {0x0186,0x01ef},
    {0x0187,0x03d9},
    {0x0188,0x01fb},
    {0x0189,0x035f},
    {0x018a,0x0000},
    {0x018b,0x0000},
    {0x018c,0x0000},
    {0x018d,0x0000},
    {0x01d0,0x0000},
    {0x01d1,0x0000},
    {0x01d2,0x0000},
    {0x01d3,0x0000},
    {0x01d4,0x0000},
    {0x01d5,0x0000},
    {0x01d6,0x0000},
    {0x01d7,0x0000},
    {0x01d8,0x0006},
    {0x01d9,0x0005},
    {0x01da,0x0001},
    {0x01db,0x0006},
    {0x01dc,0x0001},
    {0x01dd,0x0007},
    {0x01de,0x0001},
    {0x01df,0x0002},
    {0x01e0,0x0001},
    {0x01e1,0x0001},
    {0x01e2,0x00c9},
    {0x01e3,0x8000},
    {0x01e4,0x003e},
    {0x01e5,0x0015},
    {0x01e6,0x003e},
    {0x01e7,0x00c8},
    {0x01e8,0x0043},
    {0x01e9,0x00a9},
    {0x01ea,0x00a9},
    {0x01eb,0x00a9},
    {0x01ec,0x00fb},
    {0x01ed,0x00b0},
    {0x01ee,0x00b9},
    {0x01ef,0x01bb},
    {0x01f0,0x02ec},
    {0x01f1,0x02ec},
    {0x01f2,0x02ec},
    {0x01f3,0x01bd},
    {0x01f4,0x034a},
    {0x01f5,0x03d8},
    {0x01f6,0x03fc},
    {0xFFFF,0x0014}, //dealy 30ms
    {0x0000,0x0113}

};

#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))

typedef struct gain_set {
    u16 analog;
    u16 digit;
    u32 gain_x;
} gain_set_t;


static const gain_set_t gain_table[] = {
    {0x0080,0x81,64},
    {0x0080,0x82,65},
    {0x0080,0x83,66},
    {0x0080,0x84,67},
    {0x0080,0x86,68},
    {0x0080,0x87,69},
    {0x0080,0x88,70},
    {0x0080,0x8A,71},
    {0x0080,0x8B,72},
    {0x0080,0x8C,73},
    {0x0080,0x8E,74},
    {0x0080,0x8F,75},
    {0x0080,0x90,76},
    {0x0080,0x91,77},
    {0x0080,0x93,78},
    {0x0080,0x94,79},
    {0x0080,0x95,80},
    {0x0080,0x96,81},
    {0x0080,0x97,82},
    {0x0080,0x98,83},
    {0x0080,0x99,84},
    {0x0080,0x9B,85},
    {0x0080,0x9C,86},
    {0x0080,0x9D,87},
    {0x0080,0x9E,88},
    {0x0080,0x9F,89},
    {0x0080,0xA0,90},
    {0x0080,0xA1,91},
    {0x0080,0xA2,92},
    {0x0080,0xA3,93},
    {0x0080,0xA4,94},
    {0x0080,0xA5,95},
    {0x0080,0xA6,96},
    {0x0080,0xA7,97},
    {0x0080,0xA8,98},
    {0x0080,0xA9,99},
    {0x0080,0xAA,100},
    {0x0080,0xAB,101},
    {0x0080,0xAC,103},
    {0x0080,0xAD,104},
    {0x0080,0xAE,105},
    {0x0080,0xAF,106},
    {0x0080,0xB0,107},
    {0x0080,0xB1,108},
    {0x0080,0xB2,109},
    {0x0080,0xB3,111},
    {0x0080,0xB4,112},
    {0x0080,0xB5,113},
    {0x0080,0xB6,114},
    {0x0080,0xB7,115},
    {0x0080,0xB8,117},
    {0x0080,0xB9,118},
    {0x0080,0xBA,119},
    {0x0080,0xBB,121},
    {0x0080,0xBC,122},
    {0x0080,0xBD,123},
    {0x0080,0xBE,125},
    {0x0080,0xBF,126},
    {0x0080,0xC0,127},
    {0x8080,0x81,129},
    {0x8080,0x82,130},
    {0x8080,0x83,131},
    {0x8080,0x84,133},
    {0x8080,0x85,134},
    {0x8080,0x86,136},
    {0x8080,0x87,137},
    {0x8080,0x88,139},
    {0x8080,0x89,140},
    {0x8080,0x8A,142},
    {0x8080,0x8B,143},
    {0x8080,0x8C,145},
    {0x8080,0x8D,146},
    {0x8080,0x8E,148},
    {0x8080,0x8F,150},
    {0x8080,0x90,151},
    {0x8080,0x91,153},
    {0x8080,0x92,155},
    {0x8080,0x93,156},
    {0x8080,0x94,158},
    {0x8080,0x95,160},
    {0x8080,0x96,161},
    {0x8080,0x98,165},
    {0x8080,0x9A,168},
    {0x8080,0x9C,172},
    {0x8080,0x9E,176},
    {0x8080,0xA0,180},
    {0x8080,0xA2,184},
    {0x8080,0xA4,188},
    {0x8080,0xA6,192},
    {0x8080,0xA8,196},
    {0x8080,0xAA,200},
    {0x8080,0xAC,205},
    {0x8080,0xAE,209},
    {0x8080,0xB0,214},
    {0x8080,0xB2,218},
    {0x8080,0xB4,223},
    {0x8080,0xB6,228},
    {0x8080,0xB8,233},
    {0x8080,0xBA,238},
    {0x8080,0xBC,243},
    {0x8080,0xBE,248},
    {0x8080,0xC0,254},
    {0xc080,0x81,257},
    {0xc080,0x84,265},
    {0xc080,0x87,274},
    {0xc080,0x8A,283},
    {0xc080,0x8D,292},
    {0xc080,0x90,302},
    {0xc080,0x93,312},
    {0xc080,0x96,322},
    {0xc080,0x99,333},
    {0xc080,0x9C,344},
    {0xc080,0x9F,355},
    {0xc080,0xA2,366},
    {0xc080,0xA5,379},
    {0xc080,0xA8,391},
    {0xc080,0xAB,404},
    {0xc080,0xAE,417},
    {0xc080,0xB1,431},
    {0xc080,0xB4,445},
    {0xc080,0xB7,460},
    {0xc080,0xBA,475},
    {0xc080,0xBD,490},
    {0xc080,0xC0,507},
    {0xc0c0,0x82,518},
    {0xc0c0,0x86,541},
    {0xc0c0,0x8A,564},
    {0xc0c0,0x8E,589},
    {0xc0c0,0x92,615},
    {0xc0c0,0x96,642},
    {0xc0c0,0x9A,671},
    {0xc0c0,0x9E,700},
    {0xc0c0,0xA2,731},
    {0xc0c0,0xA6,763},
    {0xc0c0,0xAA,797},
    {0xc0c0,0xAE,832},
    {0xc0c0,0xB2,869},
    {0xc0c0,0xB6,907},
    {0xc0c0,0xBA,947},
    {0xc0c0,0xBE,989},
    {0xc0c0,0xC2,1033},
    {0xc0c0,0xC6,1078},
    {0xc0c0,0xCA,1126},
    {0xc0c0,0xCE,1176},
    {0xc0c0,0xD2,1228},
    {0xc0c0,0xD6,1282},
    {0xc0c0,0xDA,1338},
    {0xc0c0,0xDE,1397},
    {0xc0c0,0xE2,1459},
    {0xc0c0,0xE6,1523},
    {0xc0c0,0xEA,1591},
    {0xc0c0,0xEE,1661},
    {0xc0c0,0xF2,1734},
    {0xc0c0,0xF6,1811},
    {0xc0c0,0xFA,1890},
    {0xc0c0,0x100,2017},
};

#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

#define MIN(x,y) ((x)<(y) ? (x) : (y))
#define MAX(x,y) ((x)>(y) ? (x) : (y))

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
    for (i=0; i<0x3f; i++);
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

   //  isp_info ("echo w sen_reg 0x%x 0x%x >/proc/isp0/command \n",
   //         addr,val);
    return 0;
}

static u32 get_pclk(u32 xclk)
{
    u32 pclk, pll_m, pll_div;

    read_reg(0x01, &pll_m);
    read_reg(0x02, &pll_div);
//    pclk = (xclk / 3) * pll_m /3;
	//12-bit, 2ch, 2 port
	pclk = xclk * pll_m / (pll_div+1) / 12 * 2 * 2;
    isp_info("LVDS Pixel clock %d\n", pclk);

    return pclk;
}

static void adjust_blanking(int fps)
{
    vert_cycle = g_psensor->pclk / (H_CYCLE * fps);
    isp_info("fps=%d, vert_cycle=%d\n", fps, vert_cycle);
    write_reg(0x1A0, vert_cycle-1);
    write_reg(0x1A3, vert_cycle-1);
    write_reg(0x1A7, vert_cycle-1);
    write_reg(0x2A0, vert_cycle-1);
    write_reg(0x2A3, vert_cycle-1);
    write_reg(0x2A7, vert_cycle-1);

}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

//    u32 num_of_pclk;

//    g_psensor->pclk = get_pclk(g_psensor->xclk);


    pinfo->t_row = H_CYCLE * 10000 / (g_psensor->pclk / 1000);

    isp_info("t_row=%d pclk=%d\n",pinfo->t_row,g_psensor->pclk);
}

static int set_size(u32 width, u32 height)
{
    int ret = 0;
//    u32 x_start, x_end, y_start, y_end;
    // check size
    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("Exceed maximum sensor resolution!\n");
        return -1;
    }

    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;

    g_psensor->out_win.width = 2016;    //0x7da
    g_psensor->out_win.height = 1108;   //0x454

    adjust_blanking(g_psensor->fps);
    calculate_t_row();

    if (width <=1280 && height <= 720){
        g_psensor->img_win.x = 84 + (1920-1280) / 2;
        g_psensor->img_win.y = 20 + (1080-720) / 2;
    }
    else{
        g_psensor->img_win.x = 84;
        g_psensor->img_win.y = 20;
    }
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    return ret;
}

static u32 get_exp(void)
{
    u32 lines, shtpos, tg_lcf;

    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (!g_psensor->curr_exp) {

        read_reg(0xA1, &shtpos);
        read_reg(0xA2, &tg_lcf);
        shtpos |= (tg_lcf << 16) & 0x10000;
        read_reg(0xA5, &tg_lcf);

        // exposure = r_tg_lcf_v(V) + {1(V) - 3(H) - shtpos(H) + alpha(H)}
        lines = (tg_lcf * vert_cycle) + vert_cycle - 3 - shtpos + ALPHA;
        g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 lines, tg_lcf, shutpos;

//	isp_info("set_exposure %d\n", exp);
//	pdev->curr_exposure = us;

    lines = ((exp * 1000)+pinfo->t_row/2) / pinfo->t_row;
    lines=MAX(1, lines);

//    isp_info("Before:exp=%u,t_row=%d,  lines=%d \n",exp,pinfo->t_row,  lines);


    // tricky: to keep 30fps
    if ((lines > (vert_cycle-1)) && ((lines - vert_cycle) < 10))
        lines = vert_cycle - 1;



    if (lines >= vert_cycle) {
        tg_lcf = lines / vert_cycle;
        lines -= (tg_lcf * vert_cycle);
    } else {
        tg_lcf = 0;
    }


    // exposure = r_tg_lcf_v(V) + {1(V) - 3(H) - shtpos(H) + alpha(H)
    shutpos = (vert_cycle - 3 + ALPHA) - lines;
    shutpos=MAX(1, shutpos);

    write_reg(0xA5, tg_lcf);
    write_reg(0xA1, shutpos&0xFFFF);
    write_reg(0xA2, shutpos >> 16);
    lines += (tg_lcf * vert_cycle);
    g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;

//    isp_info("tg_lcf =%d, shutpos=%d ,lines=%d ,curr_exp=%d \n",tg_lcf,shutpos, lines ,  g_psensor->curr_exp );
    return ret;
}

static u32 get_gain(void)
{
    if (g_psensor->curr_gain == 0)
       g_psensor->curr_gain = gain_table[0].gain_x;

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
//    isp_info("set_gain %d, %d, 0x%x, 0x%x, %d\n", gain, i-1, ang_x, dig_x, g_psensor->curr_gain);

    return ret;
}

static void update_bayer_type(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (pinfo->flip)
        g_psensor->bayer_type = BAYER_BG;
    else
        g_psensor->bayer_type = BAYER_GR;
}

static int get_flip(void)
{
    u32 reg;

    read_reg(0x00a0, &reg);

    return (reg >> 15) & 0x8000;
}

static int set_flip(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    pinfo->flip = enable;
    read_reg(0x00a0, &reg);
    if (enable)
        reg |= 0x8000;
    else
        reg &= ~0x8000;
    write_reg(0x00a0, reg);

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
    int ret = 0;

    switch (cmd) {
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
    int ret = 0;
    int i;
//    u32 readtmp;
    u32 get_reg;

    if (pinfo->is_init)
        return 0;

    for (i=0; i<NUM_OF_INIT_REGS; i++) {
        if (sensor_init_regs[i].addr == 0xFFFF) {
            mdelay(sensor_init_regs[i].val);
        } else {
            if (write_reg(sensor_init_regs[i].addr, sensor_init_regs[i].val) < 0) {
                isp_err("%s init fail!!", SENSOR_NAME);
                return -EINVAL;
            }

            //reading
            if (sensor_init_regs[i].addr >= 0x10){
                read_reg( sensor_init_regs[i].addr        , &get_reg );
                if (get_reg != sensor_init_regs[i].val)
                    isp_err("0x%x =0x%x(0x%x)",sensor_init_regs[i].addr ,get_reg , sensor_init_regs[i].val );
             }

            //break loop
             // if (sensor_init_regs[i].addr == 0x20 )
             //    i = NUM_OF_INIT_REGS;
//         mdelay(10);

        }
    }

    if (fps > 30)
        write_reg(0x0001, 0x0036);

    g_psensor->pclk = get_pclk(g_psensor->xclk);

    // set image size
    ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height);
    if (ret == 0)
        pinfo->is_init = 1;
    //Must be after set_size()
     //adjust_vblk(g_psensor->fps);

    // set flip
    set_flip(flip);
    update_bayer_type();

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
static void mn34041_deconstruct(void)
{
    if ((g_psensor) && (g_psensor->private)) {
        kfree(g_psensor->private);
    }

    isp_api_spi_exit();
    //sensor_remove_i2c_driver();
    // turn off EXT_CLK
    isp_api_extclk_onoff(0);
    // release CAP_RST
    isp_api_cap_rst_exit();
}

static int mn34041_construct(u32 xclk, u16 width, u16 height)
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
    g_psensor->capability = SUPPORT_FLIP;
    g_psensor->xclk = xclk;
    g_psensor->fmt = RAW12;
    g_psensor->bayer_type = BAYER_GR;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_HIGH;
    g_psensor->img_win.x = 84;
    g_psensor->img_win.y = 20;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;
    g_psensor->min_exp = 100;
    g_psensor->max_exp = 5000; // 0.5 sec
    g_psensor->min_gain = gain_table[0].gain_x;
    g_psensor->max_gain = gain_table[NUM_OF_GAINSET - 1].gain_x;
    //g_psensor->max_gain = 11424;
    g_psensor->exp_latency = 1;
    g_psensor->gain_latency = 0;
    g_psensor->fps = fps;
    g_psensor->num_of_lane = ch_num;
    g_psensor->interface = IF_LVDS_PANASONIC;
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

   // if ((ret = sensor_init_i2c_driver()) < 0)
    //    goto construct_err;

    ret = isp_api_spi_init();
    if (ret)
        return ret;

    if ((ret = init()) < 0)
        goto construct_err;

    return 0;

construct_err:
    mn34041_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init mn34041_init(void)
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

    if ((ret = mn34041_construct(sensor_xclk, sensor_w, sensor_h)) < 0){
        isp_err("mn34041_construct Error \n");
        goto init_err2;
        }
    // register sensor device to ISP driver

    if ((ret = isp_register_sensor(g_psensor)) < 0){
        isp_err("isp_register_sensor Error \n");
        goto init_err2;
     }

    return ret;

init_err2:
    kfree(g_psensor);

init_err1:
    return ret;
}

static void __exit mn34041_exit(void)
{
    isp_unregister_sensor(g_psensor);
    mn34041_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(mn34041_init);
module_exit(mn34041_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor mn34041");
MODULE_LICENSE("GPL");
