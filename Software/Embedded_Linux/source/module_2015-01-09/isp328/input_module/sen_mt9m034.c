/**
 * @file sen_mt9m034.c
 * Panasonic MT9M034/AR0130 sensor driver
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

static uint binning = 0;
module_param(binning, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(binning, "sensor binning");

static uint fps = 30;
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "sensor frame rate");

static uint inv_clk = 0;
module_param(inv_clk, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "invert clock, 0:Disable, 1: Enable");

static uint wdr = 0;
module_param(wdr, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(wdr, "WDR mode");

//=============================================================================
// global
//=============================================================================
#define SENSOR_NAME         "MT9M034"
#define SENSOR_MAX_WIDTH    1288
#define SENSOR_MAX_HEIGHT   968
#define ID_AR0130           0x2402
#define ID_MT9M034          0x2400

//#define WDR_EXP_LIMIT
//#define USE_DLO_MC

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
    u32 t_row;          // unit is 1/10 us
    u32 ang_reg_1;
    u32 dcg_reg;
} sensor_info_t;

typedef struct sensor_reg {
    u16  addr;
    u16 val;
} sensor_reg_t;

#define DELAY_CODE      0xFFFF

static sensor_reg_t ar0130_init_regs[] = {
    //All output 1280x960, only resized by crop size
    {0x301A, 0x0001},   // RESET_REGISTER
    {DELAY_CODE, 200},
    {0x301A, 0x10D8},   // RESET_REGISTER
    {DELAY_CODE, 200},

    //[A-1000ERS Rev3 linear Only sequencer load December 16 2010]
    //HiDY sequencer CR 31644
    {0x3088, 0x8000},   // SEQ_CTRL_PORT

    {0x3086, 0x0225},   // SEQ_DATA_PORT
    {0x3086, 0x5050},   // SEQ_DATA_PORT
    {0x3086, 0x2D26},   // SEQ_DATA_PORT
    {0x3086, 0x0828},   // SEQ_DATA_PORT
    {0x3086, 0x0D17},   // SEQ_DATA_PORT
    {0x3086, 0x0926},   // SEQ_DATA_PORT
    {0x3086, 0x0028},   // SEQ_DATA_PORT
    {0x3086, 0x0526},   // SEQ_DATA_PORT
    {0x3086, 0xA728},   // SEQ_DATA_PORT
    {0x3086, 0x0725},   // SEQ_DATA_PORT
    {0x3086, 0x8080},   // SEQ_DATA_PORT
    {0x3086, 0x2917},   // SEQ_DATA_PORT
    {0x3086, 0x0525},   // SEQ_DATA_PORT
    {0x3086, 0x0040},   // SEQ_DATA_PORT
    {0x3086, 0x2702},   // SEQ_DATA_PORT
    {0x3086, 0x1616},   // SEQ_DATA_PORT
    {0x3086, 0x2706},   // SEQ_DATA_PORT
    {0x3086, 0x1736},   // SEQ_DATA_PORT
    {0x3086, 0x26A6},   // SEQ_DATA_PORT
    {0x3086, 0x1703},   // SEQ_DATA_PORT
    {0x3086, 0x26A4},   // SEQ_DATA_PORT
    {0x3086, 0x171F},   // SEQ_DATA_PORT
    {0x3086, 0x2805},   // SEQ_DATA_PORT
    {0x3086, 0x2620},   // SEQ_DATA_PORT
    {0x3086, 0x2804},   // SEQ_DATA_PORT
    {0x3086, 0x2520},   // SEQ_DATA_PORT
    {0x3086, 0x2027},   // SEQ_DATA_PORT
    {0x3086, 0x0017},   // SEQ_DATA_PORT
    {0x3086, 0x1E25},   // SEQ_DATA_PORT
    {0x3086, 0x0020},   // SEQ_DATA_PORT
    {0x3086, 0x2117},   // SEQ_DATA_PORT
    {0x3086, 0x1028},   // SEQ_DATA_PORT
    {0x3086, 0x051B},   // SEQ_DATA_PORT
    {0x3086, 0x1703},   // SEQ_DATA_PORT
    {0x3086, 0x2706},   // SEQ_DATA_PORT
    {0x3086, 0x1703},   // SEQ_DATA_PORT
    {0x3086, 0x1747},   // SEQ_DATA_PORT
    {0x3086, 0x2660},   // SEQ_DATA_PORT
    {0x3086, 0x17AE},   // SEQ_DATA_PORT
    {0x3086, 0x2500},   // SEQ_DATA_PORT
    {0x3086, 0x9027},   // SEQ_DATA_PORT
    {0x3086, 0x0026},   // SEQ_DATA_PORT
    {0x3086, 0x1828},   // SEQ_DATA_PORT
    {0x3086, 0x002E},   // SEQ_DATA_PORT
    {0x3086, 0x2A28},   // SEQ_DATA_PORT
    {0x3086, 0x081E},   // SEQ_DATA_PORT
    {0x3086, 0x0831},   // SEQ_DATA_PORT
    {0x3086, 0x1440},   // SEQ_DATA_PORT
    {0x3086, 0x4014},   // SEQ_DATA_PORT
    {0x3086, 0x2020},   // SEQ_DATA_PORT
    {0x3086, 0x1410},   // SEQ_DATA_PORT
    {0x3086, 0x1034},   // SEQ_DATA_PORT
    {0x3086, 0x1400},   // SEQ_DATA_PORT
    {0x3086, 0x1014},   // SEQ_DATA_PORT
    {0x3086, 0x0020},   // SEQ_DATA_PORT
    {0x3086, 0x1400},   // SEQ_DATA_PORT
    {0x3086, 0x4013},   // SEQ_DATA_PORT
    {0x3086, 0x1802},   // SEQ_DATA_PORT
    {0x3086, 0x1470},   // SEQ_DATA_PORT
    {0x3086, 0x7004},   // SEQ_DATA_PORT
    {0x3086, 0x1470},   // SEQ_DATA_PORT
    {0x3086, 0x7003},   // SEQ_DATA_PORT
    {0x3086, 0x1470},   // SEQ_DATA_PORT
    {0x3086, 0x7017},   // SEQ_DATA_PORT
    {0x3086, 0x2002},   // SEQ_DATA_PORT
    {0x3086, 0x1400},   // SEQ_DATA_PORT
    {0x3086, 0x2002},   // SEQ_DATA_PORT
    {0x3086, 0x1400},   // SEQ_DATA_PORT
    {0x3086, 0x5004},   // SEQ_DATA_PORT
    {0x3086, 0x1400},   // SEQ_DATA_PORT
    {0x3086, 0x2004},   // SEQ_DATA_PORT
    {0x3086, 0x1400},   // SEQ_DATA_PORT
    {0x3086, 0x5022},   // SEQ_DATA_PORT
    {0x3086, 0x0314},   // SEQ_DATA_PORT
    {0x3086, 0x0020},   // SEQ_DATA_PORT
    {0x3086, 0x0314},   // SEQ_DATA_PORT
    {0x3086, 0x0050},   // SEQ_DATA_PORT
    {0x3086, 0x2C2C},   // SEQ_DATA_PORT
    {0x3086, 0x2C2C},   // SEQ_DATA_PORT
    {0x309E, 0x0000},   // set start address for linear seq

    {DELAY_CODE, 200},

    // [Linear Mode Full Resolution]
    {0x301A, 0x10D8},   // RESET_REGISTER
    {0x3082, 0x0029},   // OPERATION_MODE_CTRL

    // [A-1000ERS Rev3 Optimized settings]
    {0x301E, 0x00C8},   // DATA_PEDESTAL
    {0x3EDA, 0x0F03},
    {0x3EDE, 0xC005},
    {0x3ED8, 0x09EF},
    {0x3EE2, 0xA46B},
    {0x3EE0, 0x047D},
    {0x3EDC, 0x0070},
    {0x3044, 0x0400},
    {0x3EE6, 0x4303},
    {0x3EE4, 0xD308},
    {0x3ED6, 0x00BD},
    {0x3EE6, 0x8303},

    {0x30E4, 0x6372},   // RESERVED_MFR_30E4
    {0x30E2, 0x7253},   // RESERVED_MFR_30E2
    {0x30E0, 0x5470},   // RESERVED_MFR_30E0
    {0x30E6, 0xC4CC},   // RESERVED_MFR_30E6
    {0x30E8, 0x8050},   // RESERVED_MFR_30E8

    // [Column Retriggering at start up]
    {0x30B0, 0x1300},   // DIGITAL_TEST
    {0x30D4, 0xE007},   // COLUMN_CORRECTION
    {0x30BA, 0x0008},   // RESERVED_MFR_30BA
    {0x301A, 0x10DC},   // RESET_REGISTER
    {DELAY_CODE, 200},
    {0x301A, 0x10D8},   // RESET_REGISTER
    {DELAY_CODE, 200},

    {0x3012, 0x02A0},   // COARSE_INTEGRATION_TIME

    // [Full Resolution 45FPS Setup]
    {0x3032, 0x0000},   // DIGITAL_BINNING

    {0x3002, 0x0002},   // Y_ADDR_START
    {0x3004, 0x0001},   // X_ADDR_START
    {0x3006, 0x03C5},   // Y_ADDR_END
    {0x3008, 0x0503},   // X_ADDR_END
    {0x300A, 0x03DE},   // FRAME_LENGTH_LINES
    {0x300C, 0x0672},   // LINE_LENGTH_PCK

    {0x301A, 0x10D8},   // RESET_REGISTER

    // [Enable Parallel Mode]
    {0x31D0, 0x0001},   // HDR_COMP

    // [PLL Enabled 27Mhz to 74.25Mhz]
    {0x302C, 0x0002},   // VT_SYS_CLK_DIV (P1)
    {0x302A, 0x0004},   // VT_PIX_CLK_DIV (P2)
    {0x302E, 0x0002},   // PRE_PLL_CLK_DIV (N)
    {0x3030, 0x002C},   // PLL_MULTIPLIER (M)
    {0x30B0, 0x1300},   // DIGITAL_TEST

    {DELAY_CODE, 100},

    // [Enable Embedded Data and Stats]
    {0x3064, 0x1802},   // EMBEDDED_DATA_CTRL disable

    // [Enable AE and Load Optimized Settings For Linear Mode]
    {0x3100, 0x001A},   // AE_CTRL_REG  ,disable AE ----Allen
    {0x305E, 0x0020},
    {0x301A, 0x10DC},   // RESET_REGISTER

    {0x30C6, 0xf1c9},
    {0x30C8, 0x2005},
    {0x30Ca, 0x09a0},
    {0x30CC, 0x0802},
};
#define NUM_OF_AR0130_INIT_REGS (sizeof(ar0130_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t mt9m034_init_regs[] = {
    //All output 1280x960, only resized by crop size
    {0x301A, 0x0001},   // RESET_REGISTER
    {DELAY_CODE, 200},
    {0x301A, 0x10D8},   // RESET_REGISTER
    {DELAY_CODE, 200},

    //[A-1000ERS Rev3 linear Only sequencer load December 16 2010]
    //HiDY sequencer CR 31644
    {0x3088, 0x8000},     // SEQ_CTRL_PORT

    {0x3086, 0x0025},     // SEQ_DATA_PORT
    {0x3086, 0x5050},     // SEQ_DATA_PORT
    {0x3086, 0x2D26},     // SEQ_DATA_PORT
    {0x3086, 0x0828},     // SEQ_DATA_PORT
    {0x3086, 0x0D17},     // SEQ_DATA_PORT
    {0x3086, 0x0926},     // SEQ_DATA_PORT
    {0x3086, 0x0028},     // SEQ_DATA_PORT
    {0x3086, 0x0526},     // SEQ_DATA_PORT
    {0x3086, 0xA728},     // SEQ_DATA_PORT
    {0x3086, 0x0725},     // SEQ_DATA_PORT
    {0x3086, 0x8080},     // SEQ_DATA_PORT
    {0x3086, 0x2925},     // SEQ_DATA_PORT
    {0x3086, 0x0040},     // SEQ_DATA_PORT
    {0x3086, 0x2702},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x2706},     // SEQ_DATA_PORT
    {0x3086, 0x1F17},     // SEQ_DATA_PORT
    {0x3086, 0x3626},     // SEQ_DATA_PORT
    {0x3086, 0xA617},     // SEQ_DATA_PORT
    {0x3086, 0x0326},     // SEQ_DATA_PORT
    {0x3086, 0xA417},     // SEQ_DATA_PORT
    {0x3086, 0x1F28},     // SEQ_DATA_PORT
    {0x3086, 0x0526},     // SEQ_DATA_PORT
    {0x3086, 0x2028},     // SEQ_DATA_PORT
    {0x3086, 0x0425},     // SEQ_DATA_PORT
    {0x3086, 0x2020},     // SEQ_DATA_PORT
    {0x3086, 0x2700},     // SEQ_DATA_PORT
    {0x3086, 0x171D},     // SEQ_DATA_PORT
    {0x3086, 0x2500},     // SEQ_DATA_PORT
    {0x3086, 0x2017},     // SEQ_DATA_PORT
    {0x3086, 0x1219},     // SEQ_DATA_PORT
    {0x3086, 0x1703},     // SEQ_DATA_PORT
    {0x3086, 0x2706},     // SEQ_DATA_PORT
    {0x3086, 0x1728},     // SEQ_DATA_PORT
    {0x3086, 0x2805},     // SEQ_DATA_PORT
    {0x3086, 0x171A},     // SEQ_DATA_PORT
    {0x3086, 0x2660},     // SEQ_DATA_PORT
    {0x3086, 0x175A},     // SEQ_DATA_PORT
    {0x3086, 0x2317},     // SEQ_DATA_PORT
    {0x3086, 0x1122},     // SEQ_DATA_PORT
    {0x3086, 0x1741},     // SEQ_DATA_PORT
    {0x3086, 0x2500},     // SEQ_DATA_PORT
    {0x3086, 0x9027},     // SEQ_DATA_PORT
    {0x3086, 0x0026},     // SEQ_DATA_PORT
    {0x3086, 0x1828},     // SEQ_DATA_PORT
    {0x3086, 0x002E},     // SEQ_DATA_PORT
    {0x3086, 0x2A28},     // SEQ_DATA_PORT
    {0x3086, 0x081C},     // SEQ_DATA_PORT
    {0x3086, 0x1470},     // SEQ_DATA_PORT
    {0x3086, 0x7003},     // SEQ_DATA_PORT
    {0x3086, 0x1470},     // SEQ_DATA_PORT
    {0x3086, 0x7004},     // SEQ_DATA_PORT
    {0x3086, 0x1470},     // SEQ_DATA_PORT
    {0x3086, 0x7005},     // SEQ_DATA_PORT
    {0x3086, 0x1470},     // SEQ_DATA_PORT
    {0x3086, 0x7009},     // SEQ_DATA_PORT
    {0x3086, 0x170C},     // SEQ_DATA_PORT
    {0x3086, 0x0014},     // SEQ_DATA_PORT
    {0x3086, 0x0020},     // SEQ_DATA_PORT
    {0x3086, 0x0014},     // SEQ_DATA_PORT
    {0x3086, 0x0050},     // SEQ_DATA_PORT
    {0x3086, 0x0314},     // SEQ_DATA_PORT
    {0x3086, 0x0020},     // SEQ_DATA_PORT
    {0x3086, 0x0314},     // SEQ_DATA_PORT
    {0x3086, 0x0050},     // SEQ_DATA_PORT
    {0x3086, 0x0414},     // SEQ_DATA_PORT
    {0x3086, 0x0020},     // SEQ_DATA_PORT
    {0x3086, 0x0414},     // SEQ_DATA_PORT
    {0x3086, 0x0050},     // SEQ_DATA_PORT
    {0x3086, 0x0514},     // SEQ_DATA_PORT
    {0x3086, 0x0020},     // SEQ_DATA_PORT
    {0x3086, 0x2405},     // SEQ_DATA_PORT
    {0x3086, 0x1400},     // SEQ_DATA_PORT
    {0x3086, 0x5001},     // SEQ_DATA_PORT
    {0x3086, 0x2550},     // SEQ_DATA_PORT
    {0x3086, 0x502D},     // SEQ_DATA_PORT
    {0x3086, 0x2608},     // SEQ_DATA_PORT
    {0x3086, 0x280D},     // SEQ_DATA_PORT
    {0x3086, 0x1709},     // SEQ_DATA_PORT
    {0x3086, 0x2600},     // SEQ_DATA_PORT
    {0x3086, 0x2805},     // SEQ_DATA_PORT
    {0x3086, 0x26A7},     // SEQ_DATA_PORT
    {0x3086, 0x2807},     // SEQ_DATA_PORT
    {0x3086, 0x2580},     // SEQ_DATA_PORT
    {0x3086, 0x8029},     // SEQ_DATA_PORT
    {0x3086, 0x2500},     // SEQ_DATA_PORT
    {0x3086, 0x4027},     // SEQ_DATA_PORT
    {0x3086, 0x0216},     // SEQ_DATA_PORT
    {0x3086, 0x1627},     // SEQ_DATA_PORT
    {0x3086, 0x0620},     // SEQ_DATA_PORT
    {0x3086, 0x1736},     // SEQ_DATA_PORT
    {0x3086, 0x26A6},     // SEQ_DATA_PORT
    {0x3086, 0x1703},     // SEQ_DATA_PORT
    {0x3086, 0x26A4},     // SEQ_DATA_PORT
    {0x3086, 0x171F},     // SEQ_DATA_PORT
    {0x3086, 0x2805},     // SEQ_DATA_PORT
    {0x3086, 0x2620},     // SEQ_DATA_PORT
    {0x3086, 0x2804},     // SEQ_DATA_PORT
    {0x3086, 0x2520},     // SEQ_DATA_PORT
    {0x3086, 0x2027},     // SEQ_DATA_PORT
    {0x3086, 0x0017},     // SEQ_DATA_PORT
    {0x3086, 0x1D25},     // SEQ_DATA_PORT
    {0x3086, 0x0020},     // SEQ_DATA_PORT
    {0x3086, 0x1712},     // SEQ_DATA_PORT
    {0x3086, 0x1A17},     // SEQ_DATA_PORT
    {0x3086, 0x0327},     // SEQ_DATA_PORT
    {0x3086, 0x0617},     // SEQ_DATA_PORT
    {0x3086, 0x2828},     // SEQ_DATA_PORT
    {0x3086, 0x0517},     // SEQ_DATA_PORT
    {0x3086, 0x1A26},     // SEQ_DATA_PORT
    {0x3086, 0x6017},     // SEQ_DATA_PORT
    {0x3086, 0xAE25},     // SEQ_DATA_PORT
    {0x3086, 0x0090},     // SEQ_DATA_PORT
    {0x3086, 0x2700},     // SEQ_DATA_PORT
    {0x3086, 0x2618},     // SEQ_DATA_PORT
    {0x3086, 0x2800},     // SEQ_DATA_PORT
    {0x3086, 0x2E2A},     // SEQ_DATA_PORT
    {0x3086, 0x2808},     // SEQ_DATA_PORT
    {0x3086, 0x1D05},     // SEQ_DATA_PORT
    {0x3086, 0x1470},     // SEQ_DATA_PORT
    {0x3086, 0x7009},     // SEQ_DATA_PORT
    {0x3086, 0x1720},     // SEQ_DATA_PORT
    {0x3086, 0x1400},     // SEQ_DATA_PORT
    {0x3086, 0x2024},     // SEQ_DATA_PORT
    {0x3086, 0x1400},     // SEQ_DATA_PORT
    {0x3086, 0x5002},     // SEQ_DATA_PORT
    {0x3086, 0x2550},     // SEQ_DATA_PORT
    {0x3086, 0x502D},     // SEQ_DATA_PORT
    {0x3086, 0x2608},     // SEQ_DATA_PORT
    {0x3086, 0x280D},     // SEQ_DATA_PORT
    {0x3086, 0x1709},     // SEQ_DATA_PORT
    {0x3086, 0x2600},     // SEQ_DATA_PORT
    {0x3086, 0x2805},     // SEQ_DATA_PORT
    {0x3086, 0x26A7},     // SEQ_DATA_PORT
    {0x3086, 0x2807},     // SEQ_DATA_PORT
    {0x3086, 0x2580},     // SEQ_DATA_PORT
    {0x3086, 0x8029},     // SEQ_DATA_PORT
    {0x3086, 0x2500},     // SEQ_DATA_PORT
    {0x3086, 0x4027},     // SEQ_DATA_PORT
    {0x3086, 0x0216},     // SEQ_DATA_PORT
    {0x3086, 0x1627},     // SEQ_DATA_PORT
    {0x3086, 0x0617},     // SEQ_DATA_PORT
    {0x3086, 0x3626},     // SEQ_DATA_PORT
    {0x3086, 0xA617},     // SEQ_DATA_PORT
    {0x3086, 0x0326},     // SEQ_DATA_PORT
    {0x3086, 0xA417},     // SEQ_DATA_PORT
    {0x3086, 0x1F28},     // SEQ_DATA_PORT
    {0x3086, 0x0526},     // SEQ_DATA_PORT
    {0x3086, 0x2028},     // SEQ_DATA_PORT
    {0x3086, 0x0425},     // SEQ_DATA_PORT
    {0x3086, 0x2020},     // SEQ_DATA_PORT
    {0x3086, 0x2700},     // SEQ_DATA_PORT
    {0x3086, 0x171D},     // SEQ_DATA_PORT
    {0x3086, 0x2500},     // SEQ_DATA_PORT
    {0x3086, 0x2021},     // SEQ_DATA_PORT
    {0x3086, 0x1712},     // SEQ_DATA_PORT
    {0x3086, 0x1B17},     // SEQ_DATA_PORT
    {0x3086, 0x0327},     // SEQ_DATA_PORT
    {0x3086, 0x0617},     // SEQ_DATA_PORT
    {0x3086, 0x2828},     // SEQ_DATA_PORT
    {0x3086, 0x0517},     // SEQ_DATA_PORT
    {0x3086, 0x1A26},     // SEQ_DATA_PORT
    {0x3086, 0x6017},     // SEQ_DATA_PORT
    {0x3086, 0xAE25},     // SEQ_DATA_PORT
    {0x3086, 0x0090},     // SEQ_DATA_PORT
    {0x3086, 0x2700},     // SEQ_DATA_PORT
    {0x3086, 0x2618},     // SEQ_DATA_PORT
    {0x3086, 0x2800},     // SEQ_DATA_PORT
    {0x3086, 0x2E2A},     // SEQ_DATA_PORT
    {0x3086, 0x2808},     // SEQ_DATA_PORT
    {0x3086, 0x1E17},     // SEQ_DATA_PORT
    {0x3086, 0x0A05},     // SEQ_DATA_PORT
    {0x3086, 0x1470},     // SEQ_DATA_PORT
    {0x3086, 0x7009},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x1400},     // SEQ_DATA_PORT
    {0x3086, 0x2024},     // SEQ_DATA_PORT
    {0x3086, 0x1400},     // SEQ_DATA_PORT
    {0x3086, 0x502B},     // SEQ_DATA_PORT
    {0x3086, 0x302C},     // SEQ_DATA_PORT
    {0x3086, 0x2C2C},     // SEQ_DATA_PORT
    {0x3086, 0x2C00},     // SEQ_DATA_PORT
    {0x3086, 0x0225},     // SEQ_DATA_PORT
    {0x3086, 0x5050},     // SEQ_DATA_PORT
    {0x3086, 0x2D26},     // SEQ_DATA_PORT
    {0x3086, 0x0828},     // SEQ_DATA_PORT
    {0x3086, 0x0D17},     // SEQ_DATA_PORT
    {0x3086, 0x0926},     // SEQ_DATA_PORT
    {0x3086, 0x0028},     // SEQ_DATA_PORT
    {0x3086, 0x0526},     // SEQ_DATA_PORT
    {0x3086, 0xA728},     // SEQ_DATA_PORT
    {0x3086, 0x0725},     // SEQ_DATA_PORT
    {0x3086, 0x8080},     // SEQ_DATA_PORT
    {0x3086, 0x2917},     // SEQ_DATA_PORT
    {0x3086, 0x0525},     // SEQ_DATA_PORT
    {0x3086, 0x0040},     // SEQ_DATA_PORT
    {0x3086, 0x2702},     // SEQ_DATA_PORT
    {0x3086, 0x1616},     // SEQ_DATA_PORT
    {0x3086, 0x2706},     // SEQ_DATA_PORT
    {0x3086, 0x1736},     // SEQ_DATA_PORT
    {0x3086, 0x26A6},     // SEQ_DATA_PORT
    {0x3086, 0x1703},     // SEQ_DATA_PORT
    {0x3086, 0x26A4},     // SEQ_DATA_PORT
    {0x3086, 0x171F},     // SEQ_DATA_PORT
    {0x3086, 0x2805},     // SEQ_DATA_PORT
    {0x3086, 0x2620},     // SEQ_DATA_PORT
    {0x3086, 0x2804},     // SEQ_DATA_PORT
    {0x3086, 0x2520},     // SEQ_DATA_PORT
    {0x3086, 0x2027},     // SEQ_DATA_PORT
    {0x3086, 0x0017},     // SEQ_DATA_PORT
    {0x3086, 0x1E25},     // SEQ_DATA_PORT
    {0x3086, 0x0020},     // SEQ_DATA_PORT
    {0x3086, 0x2117},     // SEQ_DATA_PORT
    {0x3086, 0x121B},     // SEQ_DATA_PORT
    {0x3086, 0x1703},     // SEQ_DATA_PORT
    {0x3086, 0x2706},     // SEQ_DATA_PORT
    {0x3086, 0x1728},     // SEQ_DATA_PORT
    {0x3086, 0x2805},     // SEQ_DATA_PORT
    {0x3086, 0x171A},     // SEQ_DATA_PORT
    {0x3086, 0x2660},     // SEQ_DATA_PORT
    {0x3086, 0x17AE},     // SEQ_DATA_PORT
    {0x3086, 0x2500},     // SEQ_DATA_PORT
    {0x3086, 0x9027},     // SEQ_DATA_PORT
    {0x3086, 0x0026},     // SEQ_DATA_PORT
    {0x3086, 0x1828},     // SEQ_DATA_PORT
    {0x3086, 0x002E},     // SEQ_DATA_PORT
    {0x3086, 0x2A28},     // SEQ_DATA_PORT
    {0x3086, 0x081E},     // SEQ_DATA_PORT
    {0x3086, 0x0831},     // SEQ_DATA_PORT
    {0x3086, 0x1440},     // SEQ_DATA_PORT
    {0x3086, 0x4014},     // SEQ_DATA_PORT
    {0x3086, 0x2020},     // SEQ_DATA_PORT
    {0x3086, 0x1410},     // SEQ_DATA_PORT
    {0x3086, 0x1034},     // SEQ_DATA_PORT
    {0x3086, 0x1400},     // SEQ_DATA_PORT
    {0x3086, 0x1014},     // SEQ_DATA_PORT
    {0x3086, 0x0020},     // SEQ_DATA_PORT
    {0x3086, 0x1400},     // SEQ_DATA_PORT
    {0x3086, 0x4013},     // SEQ_DATA_PORT
    {0x3086, 0x1802},     // SEQ_DATA_PORT
    {0x3086, 0x1470},     // SEQ_DATA_PORT
    {0x3086, 0x7004},     // SEQ_DATA_PORT
    {0x3086, 0x1470},     // SEQ_DATA_PORT
    {0x3086, 0x7003},     // SEQ_DATA_PORT
    {0x3086, 0x1470},     // SEQ_DATA_PORT
    {0x3086, 0x7017},     // SEQ_DATA_PORT
    {0x3086, 0x2002},     // SEQ_DATA_PORT
    {0x3086, 0x1400},     // SEQ_DATA_PORT
    {0x3086, 0x2002},     // SEQ_DATA_PORT
    {0x3086, 0x1400},     // SEQ_DATA_PORT
    {0x3086, 0x5004},     // SEQ_DATA_PORT
    {0x3086, 0x1400},     // SEQ_DATA_PORT
    {0x3086, 0x2004},     // SEQ_DATA_PORT
    {0x3086, 0x1400},     // SEQ_DATA_PORT
    {0x3086, 0x5022},     // SEQ_DATA_PORT
    {0x3086, 0x0314},     // SEQ_DATA_PORT
    {0x3086, 0x0020},     // SEQ_DATA_PORT
    {0x3086, 0x0314},     // SEQ_DATA_PORT
    {0x3086, 0x0050},     // SEQ_DATA_PORT
    {0x3086, 0x2C2C},     // SEQ_DATA_PORT
    {0x3086, 0x2C2C},     // SEQ_DATA_PORT
    {0x309E, 0x018A},     // RESERVED_MFR_309E

    {DELAY_CODE, 200},

    // [Linear Mode Full Resolution]
    {0x301A, 0x10D8},    // RESET_REGISTER
    {0x3082, 0x0029},    // OPERATION_MODE_CTRL
    // [A-1000ERS Rev3 Optimized settings]
    {0x301E, 0x00C8},     // DATA_PEDESTAL
    {0x3EDA, 0x0F03},     // RESERVED_MFR_3EDA
    {0x3EDE, 0xC005},     // RESERVED_MFR_3EDE
    {0x3ED8, 0x01EF},     // RESERVED_MFR_3ED8
    {0x3EE2, 0xA46B},     // RESERVED_MFR_3EE2
    {0x3EE0, 0x047D},     // RESERVED_MFR_3EE0
    {0x3EDC, 0x0070},     // RESERVED_MFR_3EDC
    {0x3044, 0x0404},     // DARK_CONTROL
    {0x3EE6, 0x4303},     // RESERVED_MFR_3EE6
    {0x3EE4, 0xD308},     // DAC_LD_24_25
    {0x3ED6, 0x00BD},     // RESERVED_MFR_3ED6
    {0x3EE6, 0x8303},     // RESERVED_MFR_3EE6
    {0x30E4, 0x6372},     // RESERVED_MFR_30E4
    {0x30E2, 0x7253},     // RESERVED_MFR_30E2
    {0x30E0, 0x5470},     // RESERVED_MFR_30E0
    {0x30E6, 0xC4CC},     // RESERVED_MFR_30E6
    {0x30E8, 0x8050},     // RESERVED_MFR_30E8

    // [Column Retriggering at start up]
    {0x30B0, 0x1300},   // DIGITAL_TEST
    {0x30D4, 0xE007},   // COLUMN_CORRECTION
    {0x30BA, 0x0008},   // RESERVED_MFR_30BA
    {0x301A, 0x10DC},   // RESET_REGISTER
    {DELAY_CODE, 200},
    {0x301A, 0x10D8},   // RESET_REGISTER
    {DELAY_CODE, 200},

    {0x3012, 0x02A0},   // COARSE_INTEGRATION_TIME

    // [Full Resolution 45FPS Setup]
    {0x3032, 0x0000},   // DIGITAL_BINNING

    {0x3002, 0x0002},   // Y_ADDR_START
    {0x3004, 0x0001},   // X_ADDR_START
    {0x3006, 0x03C5},   // Y_ADDR_END
    {0x3008, 0x0503},   // X_ADDR_END
    {0x300A, 0x03DE},   // FRAME_LENGTH_LINES
    {0x300C, 0x0672},   // LINE_LENGTH_PCK

    {0x301A, 0x10D8},   // RESET_REGISTER

    // [Enable Parallel Mode]
    {0x31D0, 0x0001},   // HDR_COMP

    {0x302C, 0x0002},   // VT_SYS_CLK_DIV (P1)
    {0x302A, 0x0004},   // VT_PIX_CLK_DIV (P2)
    {0x302E, 0x0003},   // PRE_PLL_CLK_DIV (N)
    {0x3030, 0x002C},   // PLL_MULTIPLIER (M)
    {0x30B0, 0x1300},  // DIGITAL_TEST

    {DELAY_CODE, 100},

    // [Enable Embedded Data and Stats]
    {0x3064, 0x1802},   // EMBEDDED_DATA_CTRL disable
    // [Enable AE and Load Optimized Settings For Linear Mode]
    {0x3100, 0x001A},   // AE_CTRL_REG  ,disable AE ----Allen
    {0x305E, 0x0020},
    {0x301A, 0x10DC},   // RESET_REGISTER

};
#define NUM_OF_MT9M034_INIT_REGS (sizeof(mt9m034_init_regs) / sizeof(sensor_reg_t))

typedef struct _gain_set
{
    unsigned char DCG;
    unsigned char analog;
    unsigned short digit;
    int gain_x; //need divided by 100
} gain_set_t;

#define DCG_LO      0
#define DCG_HI      1

static gain_set_t gain_table[] =
{
    {DCG_LO, 0 ,32 , 64   }, {DCG_LO, 0 ,33 , 66   }, {DCG_LO, 0 ,34 , 68   },
    {DCG_LO, 0 ,35 , 70   }, {DCG_LO, 0 ,36 , 72   }, {DCG_LO, 0 ,37 , 74   },
    {DCG_LO, 0 ,38 , 76   }, {DCG_LO, 0 ,39 , 78   }, {DCG_LO, 0 ,40 , 80   },
    {DCG_LO, 0 ,42 , 84   }, {DCG_LO, 0 ,44 , 88   }, {DCG_LO, 0 ,46 , 92   },
    {DCG_LO, 0 ,48 , 96   }, {DCG_LO, 0 ,50 , 100  }, {DCG_LO, 0 ,52 , 104  },
    {DCG_LO, 0 ,54 , 108  }, {DCG_LO, 0 ,56 , 112  }, {DCG_LO, 0 ,58 , 116  },
    {DCG_LO, 0 ,60 , 120  }, {DCG_LO, 0 ,52 , 124  }, {DCG_LO, 1 ,32 , 128  }, 
    {DCG_LO, 1 ,33 , 131  },
    {DCG_LO, 1 ,35 , 140  }, {DCG_LO, 1 ,36 , 144  }, {DCG_LO, 1 ,38 , 152  },
    {DCG_LO, 1 ,40 , 160  }, {DCG_LO, 1 ,41 , 163  }, {DCG_LO, 1 ,43 , 172  },
    {DCG_HI, 0 ,32 , 179  }, {DCG_HI, 0 ,33 , 184  }, {DCG_HI, 0 ,34 , 190  },
    {DCG_HI, 0 ,35 , 195  }, {DCG_HI, 0 ,36 , 201  }, {DCG_HI, 0 ,37 , 207  },
    {DCG_HI, 0 ,38 , 213  }, {DCG_HI, 0 ,40 , 224  }, {DCG_HI, 0 ,41 , 229  },
    {DCG_HI, 0 ,42 , 235  }, {DCG_HI, 0 ,43 , 240  }, {DCG_HI, 0 ,44 , 246  },
    {DCG_LO, 2 ,32 , 256  }, {DCG_LO, 2 ,32 , 256  }, {DCG_LO, 2 ,33 , 264  },
    {DCG_LO, 2 ,34 , 272  }, {DCG_LO, 2 ,35 , 280  }, {DCG_LO, 2 ,36 , 288  },
    {DCG_LO, 2 ,36 , 288  }, {DCG_LO, 2 ,37 , 296  }, {DCG_LO, 2 ,38 , 304  },
    {DCG_LO, 2 ,39 , 312  }, {DCG_LO, 2 ,40 , 320  }, {DCG_LO, 2 ,40 , 320  },
    {DCG_LO, 2 ,41 , 328  }, {DCG_LO, 2 ,42 , 336  }, {DCG_LO, 2 ,43 , 344  },
    {DCG_LO, 2 ,44 , 352  }, {DCG_HI, 1 ,32 , 358  }, {DCG_HI, 1 ,33 , 369  },
    {DCG_HI, 1 ,34 , 380  }, {DCG_HI, 1 ,35 , 392  }, {DCG_HI, 1 ,36 , 403  },
    {DCG_HI, 1 ,37 , 414  }, {DCG_HI, 1 ,38 , 425  }, {DCG_HI, 1 ,39 , 437  },
    {DCG_HI, 1 ,40 , 448  }, {DCG_HI, 1 ,41 , 459  }, {DCG_HI, 1 ,42 , 470  },
    {DCG_HI, 1 ,43 , 481  }, {DCG_HI, 1 ,44 , 492  }, {DCG_HI, 1 ,45 , 504  },
    {DCG_LO, 3 ,32 , 512  }, {DCG_LO, 3 ,32 , 512  }, {DCG_LO, 3 ,33 , 528  },
    {DCG_LO, 3 ,33 , 528  }, {DCG_LO, 3 ,34 , 544  }, {DCG_LO, 3 ,34 , 544  },
    {DCG_LO, 3 ,35 , 560  }, {DCG_LO, 3 ,36 , 576  }, {DCG_LO, 3 ,36 , 576  },
    {DCG_LO, 3 ,36 , 576  }, {DCG_LO, 3 ,37 , 592  }, {DCG_LO, 3 ,38 , 608  },
    {DCG_LO, 3 ,38 , 608  }, {DCG_LO, 3 ,39 , 624  }, {DCG_LO, 3 ,40 , 640  },
    {DCG_LO, 3 ,40 , 640  }, {DCG_LO, 3 ,41 , 656  }, {DCG_LO, 3 ,41 , 656  },
    {DCG_LO, 3 ,42 , 672  }, {DCG_LO, 3 ,42 , 672  }, {DCG_LO, 3 ,43 , 688  },
    {DCG_LO, 3 ,43 , 688  }, {DCG_LO, 3 ,44 , 704  }, {DCG_LO, 3 ,44 , 704  },
    {DCG_HI, 2 ,32 , 716  }, {DCG_HI, 2 ,32 , 716  }, {DCG_HI, 2 ,32 , 716  },
    {DCG_HI, 2 ,33 , 739  }, {DCG_HI, 2 ,33 , 739  }, {DCG_HI, 2 ,34 , 761  },
    {DCG_HI, 2 ,34 , 761  }, {DCG_HI, 2 ,35 , 784  }, {DCG_HI, 2 ,35 , 784  },
    {DCG_HI, 2 ,36 , 806  }, {DCG_HI, 2 ,36 , 806  }, {DCG_HI, 2 ,37 , 828  },
    {DCG_HI, 2 ,38 , 851  }, {DCG_HI, 2 ,38 , 851  }, {DCG_HI, 2 ,39 , 873  },
    {DCG_HI, 2 ,39 , 873  }, {DCG_HI, 2 ,40 , 896  }, {DCG_HI, 2 ,40 , 896  },
    {DCG_HI, 2 ,41 , 918  }, {DCG_HI, 2 ,41 , 918  }, {DCG_HI, 2 ,42 , 940  },
    {DCG_HI, 2 ,42 , 940  }, {DCG_HI, 2 ,42 , 940  }, {DCG_HI, 2 ,43 , 963  },
    {DCG_HI, 2 ,43 , 963  }, {DCG_HI, 2 ,44 , 985  }, {DCG_HI, 2 ,44 , 985  },
    {DCG_HI, 2 ,45 , 1008 }, {DCG_HI, 2 ,45 , 1008 }, {DCG_HI, 2 ,46 , 1030 },
    {DCG_HI, 2 ,47 , 1052 }, {DCG_HI, 2 ,48 , 1075 }, {DCG_HI, 2 ,49 , 1097 },
    {DCG_HI, 2 ,51 , 1142 }, {DCG_HI, 2 ,52 , 1164 }, {DCG_HI, 2 ,53 , 1187 },
    {DCG_HI, 2 ,54 , 1209 }, {DCG_HI, 2 ,55 , 1232 }, {DCG_HI, 2 ,56 , 1254 },
    {DCG_HI, 2 ,57 , 1276 }, {DCG_HI, 2 ,58 , 1299 }, {DCG_HI, 2 ,59 , 1321 },
    {DCG_HI, 2 ,60 , 1344 }, {DCG_HI, 2 ,61 , 1366 }, {DCG_HI, 2 ,62 , 1388 },
    {DCG_HI, 2 ,63 , 1411 }, {DCG_HI, 3 ,32 , 1433 }, {DCG_HI, 3 ,33 , 1478 },
    {DCG_HI, 3 ,34 , 1523 }, {DCG_HI, 3 ,35 , 1568 }, {DCG_HI, 3 ,36 , 1612 },
    {DCG_HI, 3 ,37 , 1657 }, {DCG_HI, 3 ,39 , 1747 }, {DCG_HI, 3 ,40 , 1792 },
    {DCG_HI, 3 ,41 , 1836 }, {DCG_HI, 3 ,43 , 1926 }, {DCG_HI, 3 ,45 , 2016 },
    {DCG_HI, 3 ,46 , 2060 }, {DCG_HI, 3 ,47 , 2105 }, {DCG_HI, 3 ,48 , 2150 },
    {DCG_HI, 3 ,49 , 2195 }, {DCG_HI, 3 ,50 , 2240 }, {DCG_HI, 3 ,51 , 2284 },
    {DCG_HI, 3 ,53 , 2374 }, {DCG_HI, 3 ,54 , 2419 }, {DCG_HI, 3 ,55 , 2464 },
    {DCG_HI, 3 ,56 , 2508 }, {DCG_HI, 3 ,57 , 2553 }, {DCG_HI, 3 ,58 , 2598 },
    {DCG_HI, 3 ,59 , 2643 }, {DCG_HI, 3 ,60 , 2688 }, {DCG_HI, 3 ,61 , 2732 },
    {DCG_HI, 3 ,63 , 2822 }, {DCG_HI, 3 ,64 , 2867 }, {DCG_HI, 3 ,65 , 2912 },
    {DCG_HI, 3 ,66 , 2956 }, {DCG_HI, 3 ,67 , 3001 }, {DCG_HI, 3 ,68 , 3046 },
    {DCG_HI, 3 ,69 , 3091 }, {DCG_HI, 3 ,70 , 3136 }, {DCG_HI, 3 ,71 , 3180 },
    {DCG_HI, 3 ,73 , 3270 }, {DCG_HI, 3 ,74 , 3315 }, {DCG_HI, 3 ,75 , 3360 },
    {DCG_HI, 3 ,76 , 3404 }, {DCG_HI, 3 ,77 , 3449 }, {DCG_HI, 3 ,78 , 3494 },
    {DCG_HI, 3 ,79 , 3539 }, {DCG_HI, 3 ,80 , 3584 }, {DCG_HI, 3 ,81 , 3628 },
    {DCG_HI, 3 ,82 , 3673 }, {DCG_HI, 3 ,83 , 3718 }, {DCG_HI, 3 ,84 , 3763 },
    {DCG_HI, 3 ,85 , 3808 }, {DCG_HI, 3 ,86 , 3852 }, {DCG_HI, 3 ,87 , 3897 },
    {DCG_HI, 3 ,88 , 3942 }, {DCG_HI, 3 ,89 , 3987 }, {DCG_HI, 3 ,90 , 4032 },
    {DCG_HI, 3 ,91 , 4076 }, {DCG_HI, 3 ,92 , 4121 }, {DCG_HI, 3 ,93 , 4166 },
    {DCG_HI, 3 ,93 , 4166 }, {DCG_HI, 3 ,94 , 4211 }, {DCG_HI, 3 ,96 , 4300 },
    {DCG_HI, 3 ,98 , 4390 }, {DCG_HI, 3 ,99 , 4435 }, {DCG_HI, 3 ,100, 4480 },
    {DCG_HI, 3 ,101, 4524 }, {DCG_HI, 3 ,102, 4569 }, {DCG_HI, 3 ,103, 4614 },
    {DCG_HI, 3 ,104, 4659 }, {DCG_HI, 3 ,105, 4704 }, {DCG_HI, 3 ,106, 4748 },
    {DCG_HI, 3 ,107, 4793 }, {DCG_HI, 3 ,108, 4838 }, {DCG_HI, 3 ,109, 4883 },
    {DCG_HI, 3 ,110, 4928 }, {DCG_HI, 3 ,111, 4972 }, {DCG_HI, 3 ,112, 5017 },
    {DCG_HI, 3 ,113, 5062 }, {DCG_HI, 3 ,114, 5107 }, {DCG_HI, 3 ,115, 5152 },
    {DCG_HI, 3 ,116, 5196 }, {DCG_HI, 3 ,117, 5241 }, {DCG_HI, 3 ,118, 5286 },
    {DCG_HI, 3 ,119, 5331 }, {DCG_HI, 3 ,120, 5376 }, {DCG_HI, 3 ,121, 5420 },
    {DCG_HI, 3 ,122, 5465 }, {DCG_HI, 3 ,123, 5510 }, {DCG_HI, 3 ,124, 5555 },
    {DCG_HI, 3 ,125, 5600 }, {DCG_HI, 3 ,126, 5644 }, {DCG_HI, 3 ,127, 5689 },
    {DCG_HI, 3 ,128, 5734 }, {DCG_HI, 3 ,129, 5779 }, {DCG_HI, 3 ,130, 5824 },
    {DCG_HI, 3 ,131, 5868 }, {DCG_HI, 3 ,132, 5913 }, {DCG_HI, 3 ,133, 5958 },
    {DCG_HI, 3 ,134, 6003 }, {DCG_HI, 3 ,135, 6048 }, {DCG_HI, 3 ,136, 6092 },
    {DCG_HI, 3 ,137, 6137 }, {DCG_HI, 3 ,138, 6182 }, {DCG_HI, 3 ,139, 6227 },
    {DCG_HI, 3 ,140, 6272 }, {DCG_HI, 3 ,141, 6316 }, {DCG_HI, 3 ,142, 6361 },
    {DCG_HI, 3 ,143, 6406 }, {DCG_HI, 3 ,144, 6451 }, {DCG_HI, 3 ,145, 6496 },
    {DCG_HI, 3 ,146, 6540 }, {DCG_HI, 3 ,147, 6585 }, {DCG_HI, 3 ,148, 6630 },
    {DCG_HI, 3 ,149, 6675 }, {DCG_HI, 3 ,150, 6720 }, {DCG_HI, 3 ,151, 6764 },
    {DCG_HI, 3 ,152, 6809 }, {DCG_HI, 3 ,153, 6854 }, {DCG_HI, 3 ,154, 6899 },
    {DCG_HI, 3 ,155, 6944 }, {DCG_HI, 3 ,156, 6988 }, {DCG_HI, 3 ,157, 7033 },
    {DCG_HI, 3 ,158, 7078 }, {DCG_HI, 3 ,159, 7123 }, {DCG_HI, 3 ,160, 7168 },
    {DCG_HI, 3 ,161, 7212 }, {DCG_HI, 3 ,162, 7257 }, {DCG_HI, 3 ,163, 7302 },
    {DCG_HI, 3 ,164, 7347 }, {DCG_HI, 3 ,165, 7392 }, {DCG_HI, 3 ,166, 7436 },
    {DCG_HI, 3 ,167, 7481 }, {DCG_HI, 3 ,168, 7526 }, {DCG_HI, 3 ,169, 7571 },
    {DCG_HI, 3 ,170, 7616 }, {DCG_HI, 3 ,171, 7660 }, {DCG_HI, 3 ,172, 7705 },
    {DCG_HI, 3 ,173, 7750 }, {DCG_HI, 3 ,174, 7795 }, {DCG_HI, 3 ,175, 7840 },
    {DCG_HI, 3 ,176, 7884 }, {DCG_HI, 3 ,177, 7929 }, {DCG_HI, 3 ,178, 7974 },
    {DCG_HI, 3 ,179, 8019 }, {DCG_HI, 3 ,180, 8064 }, {DCG_HI, 3 ,181, 8108 },
    {DCG_HI, 3 ,182, 8153 }, {DCG_HI, 3 ,183, 8198 }, {DCG_HI, 3 ,184, 8243 },
    {DCG_HI, 3 ,185, 8288 }, {DCG_HI, 3 ,186, 8332 }, {DCG_HI, 3 ,187, 8377 },
    {DCG_HI, 3 ,188, 8422 }, {DCG_HI, 3 ,189, 8467 }, {DCG_HI, 3 ,190, 8512 },
    {DCG_HI, 3 ,191, 8556 }, {DCG_HI, 3 ,192, 8601 }, {DCG_HI, 3 ,193, 8646 },
    {DCG_HI, 3 ,194, 8691 }, {DCG_HI, 3 ,195, 8736 }, {DCG_HI, 3 ,196, 8780 },
    {DCG_HI, 3 ,197, 8825 }, {DCG_HI, 3 ,198, 8870 }, {DCG_HI, 3 ,199, 8915 },
    {DCG_HI, 3 ,200, 8960 }, {DCG_HI, 3 ,201, 9004 }, {DCG_HI, 3 ,202, 9049 },
    {DCG_HI, 3 ,203, 9094 }, {DCG_HI, 3 ,204, 9139 }, {DCG_HI, 3 ,205, 9184 },
    {DCG_HI, 3 ,206, 9228 }, {DCG_HI, 3 ,207, 9273 }, {DCG_HI, 3 ,208, 9318 },
    {DCG_HI, 3 ,209, 9363 }, {DCG_HI, 3 ,210, 9408 }, {DCG_HI, 3 ,211, 9452 },
    {DCG_HI, 3 ,212, 9497 }, {DCG_HI, 3 ,213, 9542 }, {DCG_HI, 3 ,214, 9587 },
    {DCG_HI, 3 ,215, 9632 }, {DCG_HI, 3 ,216, 9676 }, {DCG_HI, 3 ,217, 9721 },
    {DCG_HI, 3 ,218, 9766 }, {DCG_HI, 3 ,219, 9811 }, {DCG_HI, 3 ,220, 9856 },
    {DCG_HI, 3 ,221, 9900 }, {DCG_HI, 3 ,222, 9945 }, {DCG_HI, 3 ,223, 9990 },
    {DCG_HI, 3 ,224, 10035}, {DCG_HI, 3 ,225, 10080}, {DCG_HI, 3 ,226, 10124},
    {DCG_HI, 3 ,227, 10169}, {DCG_HI, 3 ,228, 10214}, {DCG_HI, 3 ,229, 10259},
    {DCG_HI, 3 ,230, 10304}, {DCG_HI, 3 ,231, 10348}, {DCG_HI, 3 ,232, 10393},
    {DCG_HI, 3 ,233, 10438}, {DCG_HI, 3 ,234, 10483}, {DCG_HI, 3 ,235, 10528},
    {DCG_HI, 3 ,236, 10572}, {DCG_HI, 3 ,237, 10617}, {DCG_HI, 3 ,238, 10662},
    {DCG_HI, 3 ,239, 10707}, {DCG_HI, 3 ,240, 10752}, {DCG_HI, 3 ,241, 10796},
    {DCG_HI, 3 ,242, 10841}, {DCG_HI, 3 ,243, 10886}, {DCG_HI, 3 ,244, 10931},
    {DCG_HI, 3 ,245, 10976}, {DCG_HI, 3 ,246, 11020}, {DCG_HI, 3 ,247, 11065},
    {DCG_HI, 3 ,248, 11110}, {DCG_HI, 3 ,249, 11155}, {DCG_HI, 3 ,250, 11200},
    {DCG_HI, 3 ,251, 11244}, {DCG_HI, 3 ,252, 11289}, {DCG_HI, 3 ,253, 11334},
    {DCG_HI, 3 ,254, 11379}, {DCG_HI, 3 ,255, 11424},
};
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

//=============================================================================
// I2C
//=============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            (0x20 >> 1)
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
    unsigned char   buf[4];

    buf[0]     = (addr >> 8) & 0xFF;
    buf[1]     = addr & 0xFF;
    buf[2]     = (val >> 8) & 0xFF;
    buf[3]     = val & 0xFF;
    msgs.addr  = I2C_ADDR;
    msgs.flags = 0;
    msgs.len   = 4;
    msgs.buf   = buf;

    return sensor_i2c_transfer(&msgs, 1);
}


static u32 get_pclk(u32 xclk)
{
    u32 vt_pix_clk_div, vt_sys_clk_div, pre_pll_clk_div, pll_multiplier;
    u32 pclk;
    u32 digital_test;
    read_reg(0x302a, &vt_pix_clk_div);   //P2
    read_reg(0x302c, &vt_sys_clk_div);   //P1
    read_reg(0x302e, &pre_pll_clk_div);  //N
    read_reg(0x3030, &pll_multiplier);   //M
    read_reg(0x30B0, &digital_test);     //digital_test[14] , bypass all
    if(digital_test & BIT14){
        pclk = xclk;
    }
    else{
        pclk = ((xclk /1000) * pll_multiplier / vt_pix_clk_div / vt_sys_clk_div / pre_pll_clk_div) * 1000;
    }
    printk("pclk(%d) XCLK(%d)\n", pclk, xclk);
    return pclk;
}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    u32 num_of_pclk;

    g_psensor->pclk = get_pclk(g_psensor->xclk);

    read_reg(0X300C, &num_of_pclk);

    pinfo->t_row = num_of_pclk * 10000 / (g_psensor->pclk / 1000);

    //isp_dbg("t_row=%d pclk=%d\n",pinfo->t_row,g_psensor->pclk);
}

static void adjust_vblk(int fps)
{
    //sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    //int tFrame, tRow;  //frame total time, row time
    int Vblk, Hblk; //V blanking lines, H blanking
    int P1 = 6, P2 = 6;  //frame start/end blanking
    int img_w, img_h;
    u32 x_start, x_end, y_start, y_end;
    int line_w, frame_h;

   //ADJ PLL
   if(g_psensor->img_win.height > 720)
        write_reg(0X3030, 0x59);
   else
        write_reg(0X3030, 0x2e);

    g_psensor->pclk = get_pclk(g_psensor->xclk);

    read_reg(0x3008, &x_end);
    read_reg(0x3004, &x_start);
    read_reg(0x3006, &y_end);
    read_reg(0x3002, &y_start);
    read_reg(0x300C, &line_w);

    img_w = x_end - x_start + 1;
    img_h = y_end - y_start + 1;
    Hblk = line_w - img_w;
    //tFrame = 1/fps
    //=> ((Vblk+img_h)*line_w - Hblk + P1 + P2) * 1 / pclk = 1/fps
    //=> (Vblk+img_h)*line_w = pclk / fps + Hblk - P1 - P2
    //=> Vblk = (pclk / fps + Hblk - 12) / line_w - img_h;
    Vblk = ((g_psensor->pclk / fps) + Hblk - P1 - P2) / line_w - img_h;
    if(Vblk < 26)  //min. V blanking
        Vblk = 26;
    frame_h = img_h + Vblk;

    write_reg(0x300A, (u32)frame_h);

    calculate_t_row();

    g_psensor->fps = fps;
}


static int set_size(u32 width, u32 height)
{
    int ret = 0;
    u32 x_start, x_end, y_start, y_end;
    int sen_width, sen_height;
    // check size
    if ((width * (1 + binning) > SENSOR_MAX_WIDTH) || (height * (1 + binning) > SENSOR_MAX_HEIGHT)) {
        isp_err("Exceed maximum sensor resolution!\n");
        return -1;
    }
    if(binning){
        sen_width = width * 2;
        sen_height = height * 2;
        x_start = (0 + (SENSOR_MAX_WIDTH - sen_width)/2) & ~BIT0;
        y_start = (2 + (SENSOR_MAX_HEIGHT - sen_height)/2) & ~BIT0;
        x_end = (sen_width - 1 + x_start + 2) | BIT0;
        y_end = (sen_height - 1 + y_start + 2) | BIT0;
        write_reg(0X3032, 0x2);
    }
    else{
        x_start = (0 + (SENSOR_MAX_WIDTH - width)/2) & ~BIT0;
        y_start = (2 + (SENSOR_MAX_HEIGHT - height)/2) & ~BIT0;
        x_end = (width - 1 + x_start + 4) | BIT0;
        y_end = (height - 1 + y_start + 4) | BIT0;
        write_reg(0X3032, 0x00);
    }
    write_reg(0X3006, y_end);
    write_reg(0x3008, x_end);
    write_reg(0X3002, y_start);
    write_reg(0X3004, x_start);

    // update sensor information
    g_psensor->out_win.x = x_start;
    g_psensor->out_win.y = y_start;
    g_psensor->out_win.width = (x_end - x_start + 1) / (1 + binning);
    g_psensor->out_win.height = (y_end - y_start + 1) / (1 + binning);

    g_psensor->img_win.x = 0;
    g_psensor->img_win.y = 0;
    g_psensor->img_win.width = width ;
    g_psensor->img_win.height = height;

    return ret;
}

static u32 get_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 num_of_line_pclk;

    if (!g_psensor->curr_exp) {
        read_reg(0x3012, &num_of_line_pclk);
        g_psensor->curr_exp = num_of_line_pclk * pinfo->t_row / 1000;
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 exp_line;

    exp_line = MAX(1, exp * 1000 / pinfo->t_row);

    write_reg(0x3012, exp_line);

    g_psensor->curr_exp = MAX(1, exp_line * pinfo->t_row / 1000);

    return ret;
}

static u32 get_gain(void)
{
    u32 ang_x, dig_x, dcg_x;
    u32 reg_dig, reg_ang, reg_dcg;
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (g_psensor->curr_gain==0) {

        read_reg(0x305E, &reg_dig);
        read_reg(0x30B0, &reg_ang);
        read_reg(0x3100, &reg_dcg);

        dig_x = (reg_dig>>5 & 0x7)*GAIN_1X +(reg_dig&0x1f)*GAIN_1X / 32;
        dcg_x = ((reg_dcg >> 2) & 0x01) * 28 * GAIN_1X / 10;
        ang_x = 1<<((reg_ang>>4) & 0x03);
        g_psensor->curr_gain = (dig_x * ang_x * dcg_x) >> GAIN_SHIFT;

        // update analog gain everytime
        pinfo->ang_reg_1 = reg_ang;
        pinfo->dcg_reg = reg_dcg;

    }
    return g_psensor->curr_gain;

}

static int set_gain(u32 gain)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 ang_x_1, dig_x, dcg_x;
    u32 ang_reg_1, dcg_reg;
    int i;

    if (gain > gain_table[NUM_OF_GAINSET - 1].gain_x)
        gain = gain_table[NUM_OF_GAINSET - 1].gain_x;
    else if(gain < gain_table[0].gain_x)
        gain = gain_table[0].gain_x;

    // search most suitable gain into gain table
    for (i=0; i<NUM_OF_GAINSET; i++) {
        if (gain_table[i].gain_x > gain)
            break;
    }

    ang_x_1 = (gain_table[i-1].analog<<4) & 0x30; // shift to bit4(0x30B0)
    dig_x = gain_table[i-1].digit;
    dcg_x = gain_table[i-1].DCG;

    ang_reg_1 = (pinfo->ang_reg_1 & 0xffcf)|ang_x_1;
    dcg_reg = (pinfo->dcg_reg & 0xfffb) | ((dcg_x & 0x01) << 2);
    write_reg(0x305E, dig_x);
    write_reg(0x30B0, ang_reg_1);
    write_reg(0x3100, dcg_reg);

    //isp_dbg("[0x%x][0x%x],%d\n",ang_x_1,dig_x, gain_table[i-1].gain_x);

    g_psensor->curr_gain = gain_table[i-1].gain_x;

    return ret;

}

static void update_bayer_type(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (pinfo->mirror) {
        if (pinfo->flip)
            g_psensor->bayer_type = BAYER_RG;
        else
            g_psensor->bayer_type = BAYER_RG;
    } else {
        if (pinfo->flip)
            g_psensor->bayer_type = BAYER_GR;
        else
            g_psensor->bayer_type = BAYER_GR;
    }
}

static int get_mirror(void)
{
    u32 reg;

    read_reg(0x3040, &reg);
    return (reg & BIT14) ? 1 : 0;

}

static int set_mirror(int enable)
{
    u32 reg;
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    read_reg(0x3040, &reg);
    if (enable)
        reg |= BIT14;
    else
        reg &=~BIT14;
    pinfo->mirror = enable;
    update_bayer_type();

    // [Column Correction ReTriggering]
    write_reg(0x301a, 0x10d8);
    write_reg(0x30d4, 0x4007);
    mdelay(200);
    write_reg(0x301a, 0x10dc);
    mdelay(200);
    write_reg(0x301a, 0x10d8);
    write_reg(0x30d4, 0xc007);
    mdelay(200);
    write_reg(0x301a, 0x10dc);

    write_reg(0x3040, reg);
    return 0;

}

static int get_flip(void)
{
    u32 reg;

    read_reg(0x3040, &reg);
    return (reg & BIT15) ? 1 : 0;

}

static int set_flip(int enable)
{
    u32 reg;
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    read_reg(0x3040, &reg);
    if (enable)
        reg |= BIT15;
    else
        reg &=~BIT15;
    write_reg(0x3040, reg);
    pinfo->flip = enable;
    update_bayer_type();

    return 0;

}

#ifdef WDR_EXP_LIMIT
static int cal_wdr_min_exp(void)
{
//    exp_line = MAX(1, exp * 1000 / pinfo->t_row);
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    int exp = MAX(1, pinfo->t_row * 128 / 1000); //128=0.5*(T1/T2)*(T2/T3)
    return exp;
}

static int cal_wdr_max_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 frame_len;
    int lines, exp;
    read_reg(0x300A, &frame_len); //frame_length_lines
    lines = MIN(42 * 16, frame_len - 45);
    exp = MAX(1, pinfo->t_row * lines / 1000);
    return exp;
}
#endif

static int set_wdr_en(int enable)
{
    u32 reg;

    // check ID
    read_reg(0x3000, &reg);
    if (reg != ID_MT9M034)
        return -EFAULT;

    read_reg(0x3082, &reg);
    //set to power down mode
    write_reg(0x301A, 0x10D8);   // RESET_REGISTER
    msleep(100);
    if(enable){
        reg &= ~BIT0;
        g_psensor->fmt = RAW12_WDR20;
        wdr = 1;
        //[HDR mode]
        write_reg(0x318A, 0x0FA0);       // Important, Reduce SNR lost
        write_reg(0x318E, 0x0380);       // enable smooth function

        write_reg(0x3082, 0x0028);       // OPERATION_MODE_CTRL  //16x16
        write_reg(0x318C, 0x4040);       // [14] enable 2D MC
#ifdef USE_DLO_MC
        write_reg(0x3190, 0x6BA0);     // [13] enable DLO Motion compensation, will disable 2D MC
        write_reg(0x3194, 0xD48);
#else
        write_reg(0x3190, 0x0BA0);     // [13] enable DLO Motion compensation
#endif

#ifdef WDR_EXP_LIMIT
        g_psensor->min_exp = cal_wdr_min_exp();
        g_psensor->max_exp = cal_wdr_max_exp(); // 0.5 sec
#else
        //g_psensor->min_exp = cal_wdr_min_exp() * 27 /74;
        g_psensor->min_exp = 2;
        g_psensor->max_exp = 5000; // 0.5 sec
#endif
    }
    else{
        wdr = 0;
        reg |= BIT0;
        g_psensor->fmt = RAW12;
        //[Linear mode]
        write_reg(0x3082, 0x0029);    // OPERATION_MODE_CTRL
        g_psensor->min_exp = 1;
        g_psensor->max_exp = 5000; // 0.5 sec
    }

    //set to streaming mode
    write_reg(0x301A, 0x10DC);   // RESET_REGISTER

    return 0;
}

static int get_wdr_en(void)
{
    int enable = (g_psensor->fmt & WDR_MASK)?1:0;
    return enable;

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
    case ID_WDR_EN:
        *(int*)arg = get_wdr_en();
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
    case ID_WDR_EN:
        ret = set_wdr_en((int)arg);
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
static void set_xclk(void)
{
#define CRYSTAL_CLOCK   24000000
#define CHIP_CLOCK      27000000

    if(g_psensor->xclk == CHIP_CLOCK){
        // [PLL Enabled 27Mhz to 74.25Mhz]
        write_reg(0x302A, 0x04);
        write_reg(0x302C, 0x02);
        write_reg(0x302E, 0x04);
        write_reg(0x3030, 0x2e);
        mdelay(100);
    }
    else if(g_psensor->xclk == CRYSTAL_CLOCK){
        // [PLL Enabled 24Mhz to 74.25Mhz]
        write_reg(0x302A, 0x04);
        write_reg(0x302C, 0x02);
        write_reg(0x302E, 0x04);
        write_reg(0x3030, 0x34);
        mdelay(100);
    }
    else{
        isp_err("Input XCLK=%d, set XCLK=%d\n",g_psensor->xclk,CHIP_CLOCK);
    }

#undef CRYSTAL_CLOCK
#undef CHIP_CLOCK
}

static int init(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    sensor_reg_t *sensor_init_regs;
    int num_of_init_regs;
    u32 id;
    int ret = 0;
    int i;

    if (pinfo->is_init)
        return 0;

    read_reg(0x3000, &id);
    if (id == ID_MT9M034) {
        printk("[MT9M034]\n");
        sensor_init_regs = mt9m034_init_regs;
        num_of_init_regs = NUM_OF_MT9M034_INIT_REGS;
        snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, "MT9M034");
    } else {
        printk("[AR0130]\n");
        sensor_init_regs = ar0130_init_regs;
        num_of_init_regs = NUM_OF_AR0130_INIT_REGS;
        snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, "AR0130");
    }

    for (i=0; i<num_of_init_regs; i++) {
        if(sensor_init_regs[i].addr == DELAY_CODE){
            mdelay(sensor_init_regs[i].val);
        }
        else if(write_reg(sensor_init_regs[i].addr, sensor_init_regs[i].val) < 0) {
            isp_err("%s init fail!!\n", SENSOR_NAME);
            return -EINVAL;
        }
    }

    set_xclk(); // set xclk
    g_psensor->pclk = get_pclk(g_psensor->xclk);
    printk("pclk(%d) XCLK(%d)\n", g_psensor->pclk, g_psensor->xclk);

    // set mirror and flip status
    set_mirror(mirror);
    set_flip(flip);
    // set image size
    ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height);
    if (ret == 0)
        pinfo->is_init = 1;
    //Must be after set_size()
    adjust_vblk(g_psensor->fps);

    // initial exposure and gain
    set_exp(g_psensor->min_exp);
    set_gain(g_psensor->min_gain);

    // get current exposure and gain setting
    get_exp();
    get_gain();

    if (id == ID_AR0130)
        return ret;

    set_wdr_en(wdr);
    if (wdr)
        printk("<< WDR mode >>\n");
    else
        printk("<< Linear mode >>\n");

    return ret;
}

//=============================================================================
// external functions
//=============================================================================
static void mt9m034_deconstruct(void)
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

static int mt9m034_construct(u32 xclk, u16 width, u16 height)
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
    //g_psensor->max_gain = 11424;
    g_psensor->exp_latency = 1;
    g_psensor->gain_latency = 1;
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

    //if ((ret = init()) < 0)
    //    goto construct_err;

    return 0;

construct_err:
    mt9m034_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init mt9m034_init(void)
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

    if ((ret = mt9m034_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit mt9m034_exit(void)
{
    isp_unregister_sensor(g_psensor);
    mt9m034_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(mt9m034_init);
module_exit(mt9m034_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor MT9M034");
MODULE_LICENSE("GPL");
