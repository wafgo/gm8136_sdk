/**
 * @file sen_ar0331.c
 * Aptina AR0331 sensor driver
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.31 $
 * $Date: 2015/01/06 09:20:10 $
 *
 * ChangeLog:
 *  $Log: sen_ar0331.c,v $
 *  Revision 1.31  2015/01/06 09:20:10  allenhsu
 *  Remove undefined gain table
 *
 *  Revision 1.29  2014/12/22 09:09:31  allenhsu
 *  #define WDR_EXP_LIMIT
 *
 *  Revision 1.28  2014/10/22 09:26:32  allenhsu
 *  Disable auto Vtxlo switching, for fix blooming issue
 *
 *  Revision 1.27  2014/09/11 06:30:17  allenhsu
 *  Fix hi-spi mode bright image bug.
 *
 *  Revision 1.26  2014/08/19 06:53:51  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.25  2014/08/08 04:20:53  allenhsu
 *  Revise gain table
 *
 *  Revision 1.24  2014/07/24 08:29:25  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.23  2014/07/17 08:25:13  allenhsu
 *  Avoid WDR flicker
 *
 *  Revision 1.20  2014/07/07 06:40:57  allenhsu
 *  Set default motion compensation to 2DMC mode
 *
 *  Revision 1.19  2014/07/05 02:51:21  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.18  2014/07/04 11:15:54  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.17  2014/06/25 06:52:28  jtwang
 *  set_size bug fixed
 *
 *  Revision 1.16  2014/06/12 05:38:25  jtwang
 *  remove Streaming-S support
 *
 *  Revision 1.15  2014/04/29 07:08:12  allenhsu
 *  set_gain() add blooming artifact workaround
 *
 *  Revision 1.14  2014/03/11 06:15:23  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.13  2014/01/03 08:12:56  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.12  2014/01/03 08:11:28  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.11  2014/01/03 07:10:10  jtwang
 *  fixed init fail issue
 *
 *  Revision 1.10  2014/01/02 10:55:38  jtwang
 *  add serial interface mode
 *
 *  Revision 1.9  2013/12/16 01:48:13  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.8  2013/12/13 06:00:20  allenhsu
 *  Disable ACACD noise filter in linear mode
 *
 *  Revision 1.7  2013/12/13 03:27:59  allenhsu
 *  Extend gain table to 127X
 *
 *  Revision 1.6  2013/12/12 04:29:16  allenhsu
 *  add module parameter [wdr]
 *
 *  Revision 1.5  2013/11/25 05:00:00  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.4  2013/11/13 06:39:06  ricktsao
 *  no message
 *
 *  Revision 1.3  2013/11/12 06:02:41  allenhsu
 *  Add sen_ar0331
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

#define PFX "sen_ar0331"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1920
#define DEFAULT_IMAGE_HEIGHT    1080
#define DEFAULT_XCLK            27000000
#define DEFAULT_CH_NUM       	4
#define DEFAULT_BITS      		12
#define DEFAULT_INTERFACE       IF_PARALLEL
#define DEFAULT_PROTOCOL		1
        // 0 : Streaming S 0x8000
        // 1 : Streaming SP 0x8404
        // 2 : Packetized 0x8400

static ushort ch_num = DEFAULT_CH_NUM;
module_param(ch_num, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_num, "channel number");

static ushort protocol = DEFAULT_PROTOCOL;
module_param( protocol, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(protocol, "HiSpi Streaming Protocol");

static ushort interface = DEFAULT_INTERFACE;
module_param( interface, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(interface, "interface type");

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

static uint wdr = 1;
module_param(wdr, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(wdr, "WDR mode");

//=============================================================================
// global
//============================================================================
#define SENSOR_NAME         "AR0331"
#define SENSOR_MAX_WIDTH    2048
#define SENSOR_MAX_HEIGHT   1536
#define SENSOR_MAX_EXP      2000  //us
//#define USE_DLO_MC
#define WDR_EXP_LIMIT


static sensor_dev_t *g_psensor = NULL;

#define DELAY_CODE      0xFFFF

typedef struct sensor_info {
    int is_init;
    int mirror;
    int flip;
    u32 max_exp_line;
    u32 ang_reg_1;      /* record reg(0x30b0) */
    u32 t_row;          /* unit: 0.01us */
} sensor_info_t;

typedef struct sensor_reg {
    u16 addr;
    u16 val;
} sensor_reg_t;

// HDR mode: Revision 2 sensor, INI0210_Rev2_DemoInitialzation_HDR_DLO.ini
static sensor_reg_t sensor_init_regs[] = {
    {0x301A, 0x0001 }, // RESET_REGISTER
    {DELAY_CODE, 200},
    {0x301A, 0x10D8 }, // RESET_REGISTER
    {DELAY_CODE, 200},  
    //
    {0x301A, 0x10D8},  // RESET_REGISTER
    {0x30B0, 0x0000},  // DIGITAL_TEST
    {0x30BA, 0x06EC},  // DIGITAL_CTRL
    {0x31AC, 0x100C},  // DATA_FORMAT_BITS 16/12
    // [PLL_settings - Parallel]
    {0x302A, 0x0008},  // VT_PIX_CLK_DIV
    {0x302C, 0x0001},  // VT_SYS_CLK_DIV
    {0x302E, 0x0002},  // PRE_PLL_CLK_DIV
    {0x3030, 0x002C},  // PLL_MULTIPLIER
    {0x3036, 0x000C},  // OP_PIX_CLK_DIV
    {0x3038, 0x0001},  // OP_SYS_CLK_DIV
    // [Parallel HDR 1080p30]
    {0x3002, 0x00E4},  // Y_ADDR_START
    {0x3004, 0x0042},  // X_ADDR_START
    {0x3006, 0x0523},  // Y_ADDR_END
    {0x3008, 0x07C9},  // X_ADDR_END
    {0x300A, 0x0465},  // FRAME_LENGTH_LINES
    {0x300C, 0x044C},  // LINE_LENGTH_PCK
    {0x3012, 0x03F4},  // COARSE_INTEGRATION_TIME
    {0x30A2, 0x0001},  // X_ODD_INC
    {0x30A6, 0x0001},  // Y_ODD_INC
    {0x3040, 0x0000},  // READ_MODE
    {0x31AE, 0x0301},  // SERIAL_FORMAT
    {0x305E, 0x0080},  // GLOBAL_GAIN
    {DELAY_CODE, 60},
#ifdef USE_DLO_MC
    // [DLO enabled]
    {0x3190, 0xE000},   // HDR_MC_CTRL4
#else
    {0x318C, 0xC001},   // HDR_MC_CTRL2
    {0x3198, 0x061E}, 	// HDR_MC_CTRL8
//    {0x301E, 0x0000}, 	// DATA_PEDESTAL
    {0x30FE, 0x0000}, 	// RESERVED_MFR_30FE
    {0x320A, 0x0000}, 	// ADACD_PEDESTAL
#endif
    //{0x30FE, 0x0000},  // RESERVED_MFR_30FE
    //{0x320A, 0x0000},  // ADACD_PEDESTAL
    {0x301A, 0x10D8},     // RESET_REGISTER

    // [Linear Mode Full Resolution]
    {0x3082, 0x0009},    // OPERATION_MODE_CTRL
    // [ALTM disabled]
    {0x301A, 0x10D8},     // RESET_REGISTER
    {0x2400, 0x0003},     // ALTM_CONTROL
    {0x2450, 0x0000},     // ALTM_OUT_PEDESTAL
    {0x301E, 0x00A8},     // DATA_PEDESTAL

    {0x301A, 0x10DC},     // RESET_REGISTER

    // [ADACD Enabled]
    {0x3202, 0x00CF},  // ADACD_NOISE_MODEL1
    {0x3206, 0x0A06},  // RESERVED_MFR_3206
    {0x3208, 0x1A12},  // RESERVED_MFR_3208
    {0x3200, 0x0002},  // ADACD_CONTROL
    // [HDR Mode Setup]
    {0x31E0, 0x0200},  // RESERVED_MFR_31E0
    {0x3060, 0x0006},  // ANALOG_GAIN
    {0x318A, 0x0E10},  // HDR_MC_CTRL1
    //
    {0x301A, 0x10DE},  // RESET_REGISTER
    // [Analog Settings]
    {0x3180, 0x8089},  // DELTA_DK_CONTROL
    {0x30F4, 0x4000},  // RESERVED_MFR_30F4
    {0x3ED4, 0x8F6C},  // RESERVED_MFR_3ED4
    {0x3ED6, 0x66CC},  // RESERVED_MFR_3ED6
    {0x3ED8, 0x8C42},  // RESERVED_MFR_3ED8
    {0x3EDA, 0x8899},  // RESERVED_MFR_3EDA
    {0x3EE6, 0x00F0},  // RESERVED_MFR_3EE6
    //
    {0x3ED2, 0x1F06},  // DAC_LD_6_7, [7]fix digital gain bug.
    {0x301A, 0x10DC},  // RESET_REGISTER
};

#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_init_regs_hispi[] = {
    {0x301A, 0x0001 }, // RESET_REGISTER
     {DELAY_CODE, 200},
     {0x301A, 0x0058 }, // RESET_REGISTER
     {DELAY_CODE, 200},
     //
     {0x301A, 0x0058},  // RESET_REGISTER
     {0x30B0, 0x0000},  // DIGITAL_TEST
     {0x30BA, 0x07EC},  // DIGITAL_CTRL
     {0x31AC, 0x0c0C},  // DATA_FORMAT_BITS
       
     {0x30A2, 0x0001},  // X_ODD_INC
     {0x30A6, 0x0001},  // Y_ODD_INC
     {0x3040, 0x0000},  // READ_MODE
    
     // [HDR Mode 16x]
     {0x3082, 0x0008},  // OPERATION_MODE_CTRL
     {0x305E, 0x0080},  // GLOBAL_GAIN
     {DELAY_CODE, 60},
     // [HDR Mode Setup]
     {0x3082, 0x0008},  // OPERATION_MODE_CTRL
     // [2D motion compensation OFF]
     {0x318C, 0x0000},  // HDR_MC_CTRL2
     {0x3190, 0x0000},  // HDR_MC_CTRL4
     {0x30FE, 0x0080},  // RESERVED_MFR_30FE
     {0x320A, 0x0080},  // ADACD_PEDESTAL
     // [DLO enabled]
     {0x3190, 0xE000},  // HDR_MC_CTRL4
     // [ALTM Enabled]
     {0x301A, 0x0058},  // RESET_REGISTER
     {0x2410, 0x0010},  // RESERVED_MFR_2410
     {0x2412, 0x0010},  // RESERVED_MFR_2412
     {0x2442, 0x0080},  // ALTM_CONTROL_KEY_K0
     {0x2444, 0x0000},  // ALTM_CONTROL_KEY_K01_LO
     {0x2446, 0x0004},  // ALTM_CONTROL_KEY_K01_HI
     {0x2440, 0x0002},  // ALTM_CONTROL_DAMPER
     {0x2450, 0x0000},  // ALTM_OUT_PEDESTAL
     {0x2438, 0x0010},  // ALTM_CONTROL_MIN_FACTOR
     {0x243A, 0x0020},  // ALTM_CONTROL_MAX_FACTOR
     {0x243C, 0x0000},  // ALTM_CONTROL_DARK_FLOOR
     {0x243E, 0x0200},  // ALTM_CONTROL_BRIGHT_FLOOR
     {0x31D0, 0x0000},  // COMPANDING
     {0x301E, 0x00A8},  // DATA_PEDESTAL
     //
     {0x2400, 0x0003},  // ALTM_CONTROL
     {0x301A, 0x005C},  // RESET_REGISTER
     // [ADACD Enabled]
     {0x3202, 0x00CF},  // ADACD_NOISE_MODEL1
     {0x3206, 0x0A06},  // RESERVED_MFR_3206
     {0x3208, 0x1A12},  // RESERVED_MFR_3208
     {0x3200, 0x0002},  // ADACD_CONTROL
     // [HDR Mode Setup]
     {0x31E0, 0x0200},  // RESERVED_MFR_31E0
     {0x3060, 0x0006},  // ANALOG_GAIN
     {0x318A, 0x0E10},  // HDR_MC_CTRL1
     // [Enable Embedded Data and Stats]
     {0x3064, 0x1882},  // SMIA_TEST
     //
     {0x301A, 0x005E},  // RESET_REGISTER
     // [Analog Settings]
     {0x3180, 0x8089},  // DELTA_DK_CONTROL
     {0x30F4, 0x4000},  // RESERVED_MFR_30F4
     {0x3ED4, 0x8F6C},  // RESERVED_MFR_3ED4
     {0x3ED6, 0x66CC},  // RESERVED_MFR_3ED6
     {0x3ED8, 0x8C42},  // RESERVED_MFR_3ED8
     {0x3EDA, 0x8899},  // RESERVED_MFR_3EDA
     {0x3EE6, 0x00F0},  // RESERVED_MFR_3EE6
     //
     {0x3ED2, 0x1F06},  // DAC_LD_6_7
     {0x301A, 0x005C},  // RESET_REGISTER
     {DELAY_CODE, 60}
};

#define NUM_OF_INIT_REGS_HISPI (sizeof(sensor_init_regs_hispi) / sizeof(sensor_reg_t))

typedef struct _gain_set {
    u8 coarse;
    u8 fine;
    u16 dig_x;
    u32 gain_x;
} gain_set_t;

static const gain_set_t gain_table_li[] = {
#if 1    
    {0,	 6, 128,  79},
    {0,	 7, 128,  82},
    {0,	 8, 128,  86},
    {0,	 9, 128,  89},
    {0,	10, 128,  93},
    {0,	11, 128,  97},
    {0,	12, 128, 102},
    {0,	13, 128, 108},
    {0,	14, 128, 114},
    {0,	15, 128, 120},
    {1,	 0, 128, 128},
    {1, 2,  128, 137},
    {1, 4,  128, 146},
    {1, 6,  128, 158},
    {1, 8,  128, 171},
    {1, 10, 128, 186},
    {1, 12, 128, 205},
    {1, 14, 128, 228},
    {2, 0,  128, 256},
    {2, 1,  128, 264},
    {2, 2,  128, 274},
    {2, 4,  128, 292},
    {2, 6,  128, 315},
    {2, 8,  128, 342},
    {2, 10, 128, 371},
    {2, 12, 128, 410},
    {2, 14, 128, 456},
    {2, 15, 128, 481},
    {3, 0,  128, 512},
#else    
    {0, 6,  128,  79  },
    {0, 7,  128,  82  },
    {0, 8,  128,  86  },
    {0, 9,  128,  89  },
    {0, 10, 128,  93  },
    {0, 11, 128,  97  },
    {0, 12, 128,  102 },
    {0, 13, 128,  108 },
    {0, 14, 128,  114 },
    {0, 15, 128,  120 },
    {1, 0,  128,  128 },
    {1, 2,  128,  137 },
    {1, 4,  128,  146 },
    {1, 6,  128,  158 },
    {1, 8,  128,  171 },
    {1, 10, 128,  186 },
    {1, 12, 128,  205 },
    {1, 14, 128,  228 },
    {2, 0,  128,  256 },
    {2, 4,  128,  292 },
    {2, 8,  128,  342 },
    {2, 12, 128,  410 },
    {3, 0,  128,  512 },
    {3, 0,  132,  528 },
    {3, 0,  136,  544 },
    {3, 0,  140,  560 },
    {3, 0,  144,  576 },
    {3, 0,  148,  592 },
    {3, 0,  152,  608 },
    {3, 0,  156,  624 },
    {3, 0,  160,  640 },
    {3, 0,  164,  656 },
    {3, 0,  168,  672 },
    {3, 0,  172,  688 },
    {3, 0,  176,  704 },
    {3, 0,  180,  720 },
    {3, 0,  184,  736 },
    {3, 0,  188,  752 },
    {3, 0,  192,  768 },
    {3, 0,  196,  784 },
    {3, 0,  200,  800 },
    {3, 0,  204,  816 },
    {3, 0,  208,  832 },
    {3, 0,  212,  848 },
    {3, 0,  216,  864 },
    {3, 0,  220,  880 },
    {3, 0,  224,  896 },
    {3, 0,  228,  912 },
    {3, 0,  232,  928 },
    {3, 0,  236,  944 },
    {3, 0,  240,  960 },
    {3, 0,  244,  976 },
    {3, 0,  248,  992 },
    {3, 0,  252,  1008},
    {3, 0,  256,  1024},
    {3, 0,  264,  1056},
    {3, 0,  272,  1088},
    {3, 0,  280,  1120},
    {3, 0,  288,  1152},
    {3, 0,  296,  1184},
    {3, 0,  304,  1216},
    {3, 0,  312,  1248},
    {3, 0,  320,  1280},
    {3, 0,  328,  1312},
    {3, 0,  336,  1344},
    {3, 0,  344,  1376},
    {3, 0,  352,  1408},
    {3, 0,  360,  1440},
    {3, 0,  368,  1472},
    {3, 0,  376,  1504},
    {3, 0,  384,  1536},
    {3, 0,  392,  1568},
    {3, 0,  400,  1600},
    {3, 0,  408,  1632},
    {3, 0,  416,  1664},
    {3, 0,  424,  1696},
    {3, 0,  432,  1728},
    {3, 0,  440,  1760},
    {3, 0,  448,  1792},
    {3, 0,  456,  1824},
    {3, 0,  464,  1856},
    {3, 0,  472,  1888},
    {3, 0,  480,  1920},
    {3, 0,  488,  1952},
    {3, 0,  496,  1984},
    {3, 0,  504,  2016},
    {3, 0,  512,  2048},
    {3, 0,  528,  2112},
    {3, 0,  544,  2176},
    {3, 0,  560,  2240},
    {3, 0,  576,  2304},
    {3, 0,  592,  2368},
    {3, 0,  608,  2432},
    {3, 0,  624,  2496},
    {3, 0,  640,  2560},
    {3, 0,  656,  2624},
    {3, 0,  672,  2688},
    {3, 0,  688,  2752},
    {3, 0,  704,  2816},
    {3, 0,  720,  2880},
    {3, 0,  736,  2944},
    {3, 0,  752,  3008},
    {3, 0,  768,  3072},
    {3, 0,  784,  3136},
    {3, 0,  800,  3200},
    {3, 0,  816,  3264},
    {3, 0,  832,  3328},
    {3, 0,  848,  3392},
    {3, 0,  864,  3456},
    {3, 0,  880,  3520},
    {3, 0,  896,  3584},
    {3, 0,  912,  3648},
    {3, 0,  928,  3712},
    {3, 0,  944,  3776},
    {3, 0,  960,  3840},
    {3, 0,  976,  3904},
    {3, 0,  992,  3968},
    {3, 0,  1008, 4032},
    {3, 0,  1024, 4096},
    {3, 0,  1040, 4160},
    {3, 0,  1056, 4224},
    {3, 0,  1072, 4288},
    {3, 0,  1088, 4352},
    {3, 0,  1104, 4416},
    {3, 0,  1120, 4480},
    {3, 0,  1136, 4544},
    {3, 0,  1152, 4608},
    {3, 0,  1168, 4672},
    {3, 0,  1184, 4736},
    {3, 0,  1200, 4800},
    {3, 0,  1216, 4864},
    {3, 0,  1232, 4928},
    {3, 0,  1248, 4992},
    {3, 0,  1264, 5056},
    {3, 0,  1280, 5120},
    {3, 0,  1296, 5184},
    {3, 0,  1312, 5248},
    {3, 0,  1328, 5312},
    {3, 0,  1344, 5376},
    {3, 0,  1360, 5440},
    {3, 0,  1376, 5504},
    {3, 0,  1392, 5568},
    {3, 0,  1408, 5632},
    {3, 0,  1424, 5696},
    {3, 0,  1440, 5760},
    {3, 0,  1456, 5824},
    {3, 0,  1472, 5888},
    {3, 0,  1488, 5952},
    {3, 0,  1504, 6016},
    {3, 0,  1520, 6080},
    {3, 0,  1536, 6144},
    {3, 0,  1552, 6208},
    {3, 0,  1568, 6272},
    {3, 0,  1584, 6336},
    {3, 0,  1600, 6400},
    {3, 0,  1616, 6464},
    {3, 0,  1632, 6528},
    {3, 0,  1648, 6592},
    {3, 0,  1664, 6656},
    {3, 0,  1680, 6720},
    {3, 0,  1696, 6784},
    {3, 0,  1712, 6848},
    {3, 0,  1728, 6912},
    {3, 0,  1744, 6976},
    {3, 0,  1760, 7040},
    {3, 0,  1776, 7104},
    {3, 0,  1792, 7168},
    {3, 0,  1808, 7232},
    {3, 0,  1824, 7296},
    {3, 0,  1840, 7360},
    {3, 0,  1856, 7424},
    {3, 0,  1872, 7488},
    {3, 0,  1888, 7552},
    {3, 0,  1904, 7616},
    {3, 0,  1920, 7680},
    {3, 0,  1936, 7744},
    {3, 0,  1952, 7808},
    {3, 0,  1968, 7872},
    {3, 0,  1984, 7936},
    {3, 0,  2000, 8000},
    {3, 0,  2016, 8064},
    {3, 0,  2032, 8128},
    {3, 0,  2047, 8188},  
#endif    
};
#define NUM_OF_GAINSET_LI (sizeof(gain_table_li) / sizeof(gain_set_t))

static const gain_set_t gain_table_wdr[] = {
#if 1
    {0,	 6, 128,  79},
    {0,	 7, 128,  82},
    {0,	 8, 128,  86},
    {0,	 9, 128,  89},
    {0,	10, 128,  93},
    {0,	11, 128,  97},
    {0,	12, 128, 102},
    {0,	13, 128, 108},
    {0,	14, 128, 114},
    {0,	15, 128, 120},
    {1,	 0, 128, 128},  
    {1,  2, 128, 137},
    {1,  4, 128, 146},
    {1,  6, 128, 158},
    {1,  8, 128, 171},
    {1, 10, 128, 186},
    {1, 12, 128, 205},
    {1, 14, 128, 228},
    {2,  0, 128, 256},
    {2,  1, 128, 264},
    {2,  2, 128, 274},
    {2,  4, 128, 292},
    {2,  6, 128, 315},
    {2,  8, 128, 342},
    {2, 10, 128, 371},
    {2, 12, 128, 410},
    {2, 14, 128, 456},
    {2, 15, 128, 481},
    {3,  0, 128, 512},
#else
    {0,	 6,	 128,   79},
    {0,	 6,	 136,   83},
    {0,	 6,	 142,   86},
    {0,	 6,	 148,   89},
    {0,	 6,	 152,   91},
    {0,	 6,	 160,   98},
    {0,	 6,	 192,  118},
    {0,	 6,	 224,  137},
    {0,	 6,	 256,  157},
    {0,	 6,	 288,  177},
    {0,	 6,	 320,  196},
    {0,	 6,	 352,  216},
    {0,	 6,	 384,  236},
    {0,	 6,	 416,  256},
    {0,	 6,	 448,  275},
    {0,	 6,	 480,  295},
    {0,	 6,	 520,  320},
    {0,	 6,	 546,  336},
    {0,	 6,	 572,  352},
    {0,	 6,	 598,  368},
    {0,	 6,	 624,  384},
    {0,	 6,	 650,  400},
    {0,	 6,	 676,  416},
    {0,	 6,	 702,  432},
    {0,	 6,	 728,  448},
    {0,	 6,	 754,  464},
    {0,	 7,	 750,  480},
    {0,	 7,	 775,  496},
    {0,	 7,	 800,  512},
    {0,	 7,	 825,  528},
    {0,	 7,	 850,  544},
    {0,	 7,	 875,  560},
    {0,	 7,	 900,  576},
    {0,	 7,	 925,  592},
    {0,	 7,	 950,  608},
    {0,	 7,	 975,  624},
    {0,	 7,	1000,  640},
    {0,	 8,	 984,  655},
    {0,	 8,	1008,  671},
    {0,	 8,	1032,  687},
    {0,	 8,	1056,  703},
    {0,	 8,	1080,  719},
    {0,	 8,	1104,  735},
    {0,	 8,	1128,  751},
    {0,	 8,	1151,  767},
    {0,	 9,	1126,  783},
    {0,	 9,	1149,  799},
    {0,	 9,	1172,  815},
    {0,	 9,	1195,  831},
    {0,	 9,	1218,  847},
    {0,	 9,	1241,  863},
    {0,	 9,	1264,  879},
    {0,	10,	1230,  894},
    {0,	10,	1252,  910},
    {0,	10,	1274,  926},
    {0,	10,	1296,  942},
    {0,	10,	1318,  958},
    {0,	10,	1340,  974},
    {0,	10,	1362,  990},
    {0,	10,	1384, 1006},
    {0,	10,	1406, 1022},
    {0,	11,	1363, 1038},
    {0,	11,	1384, 1054},
    {0,	11,	1405, 1070},
    {0,	12,	1358, 1086},
    {0,	12,	1378, 1102},
    {0,	12,	1398, 1118},
    {0,	12,	1418, 1134},
    {0,	12,	1438, 1150},
    {0,	12,	1458, 1166},
    {0,	12,	1478, 1182},
    {0,	12,	1498, 1198},
    {0,	12,	1518, 1214},
    {0,	13,	1461, 1230},
    {0,	13,	1480, 1246},
    {0,	13,	1499, 1262},
    {0,	13,	1518, 1278},
    {0,	14,	1456, 1294},
    {0,	14,	1474, 1310},
    {0,	14,	1492, 1326},
    {0,	14,	1510, 1342},
    {0,	14,	1528, 1358},
    {0,	15,	1461, 1374},
    {0,	15,	1478, 1390},
    {0,	15,	1495, 1406},
    {0,	15,	1512, 1422},
    {0,	15,	1529, 1438},
    {1,	 0,	1455, 1455},
    {1,	 0,	1471, 1471},
    {1,	 0,	1487, 1487},
    {1,	 0,	1503, 1503},
    {1,	 0,	1519, 1519},
    {1,	 0,	1535, 1535},
    {1,	 0,	1551, 1551},
    {1,	 0,	1567, 1567},
    {1,	 0,	1583, 1583},
    {1,	 0,	1599, 1599},
    {1,	 0,	1615, 1615},
    {1,	 0,	1631, 1631},
    {1,	 0,	1647, 1647},
    {1,	 0,	1663, 1663},
    {1,	 2,	1574, 1678},
    {1,	 2,	1589, 1694},
    {1,	 2,	1604, 1710},
    {1,	 2,	1619, 1726},
    {1,	 2,	1634, 1742},
    {1,	 2,	1649, 1758},
    {1,	 4,	1553, 1774},
    {1,	 4,	1567, 1790},
    {1,	 4,	1581, 1806},
    {1,	 4,	1595, 1822},
    {1,	 4,	1609, 1838},
    {1,	 4,	1623, 1854},
    {1,	 4,	1637, 1870},
    {1,	 4,	1651, 1886},
    {1,	 6,	1546, 1902},
    {1,	 6,	1559, 1918},
    {1,	 6,	1572, 1934},
    {1,	 6,	1585, 1950},
    {1,	 6,	1598, 1966},
    {1,	 6,	1611, 1982},
    {1,	 6,	1624, 1998},
    {1,	 6,	1637, 2014},
    {1,	 6,	1650, 2030},
    {1,	 6,	1663, 2046},
    {1,	 8,	1547, 2065},
    {1,	 8,	1675, 2236},
    {1,	 8,	1803, 2407},
    {1,	 8,	1931, 2577},
    {1,	 8,	2047, 2732},
    {1,	10,	2047, 2978},
    {1,	12,	2047, 3275},
    {1,	14,	2047, 3643},
    {2,	 0,	2047, 4094},
    {2,  4, 2047, 4667},
    {2,  8, 2047, 5645},
    {2, 12, 2047, 6560},
#endif    
};
#define NUM_OF_GAINSET_WDR (sizeof(gain_table_wdr) / sizeof(gain_set_t))

#define MIN(x,y) ((x)<(y) ? (x) : (y))
#define MAX(x,y) ((x)>(y) ? (x) : (y))

//============================================================================
// I2C
//============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            (0x20 >> 1)
#include "i2c_common.c"

//============================================================================
// internal functions
//============================================================================
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
    u32 pclk, bits;

    read_reg(0x302a, &vt_pix_clk_div);
    read_reg(0x302c, &vt_sys_clk_div);
    read_reg(0x302e, &pre_pll_clk_div);
    read_reg(0x3030, &pll_multiplier);

    if (g_psensor->interface == IF_HiSPi){
        switch(g_psensor->fmt){
            case RAW12_WDR16:
            case RAW12:
                bits=12;
                break;
            case RAW10:
                bits=10;
                break;
            default:
                bits=14;
                break;                
        }
        pclk = xclk / pre_pll_clk_div;
        pclk *= pll_multiplier;
        pclk /= vt_sys_clk_div;
        pclk *= ch_num; 
        pclk /= bits;
    }
    else{
        pclk = xclk / vt_pix_clk_div;
        pclk *= pll_multiplier;
        pclk /= vt_sys_clk_div;
        pclk /= pre_pll_clk_div;
    }

    printk("pclk(%d) XCLK(%d)\n", pclk, xclk);
    return pclk;
}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 num_of_pclk;
    u32 line_length_pck;
    g_psensor->pclk = get_pclk(g_psensor->xclk);

    read_reg(0X300C, &line_length_pck);
    num_of_pclk = line_length_pck * 2;

    //720p: pinfo->t_row = num_of_pclk * 1000000 / g_psensor->pclk;
    //pinfo->t_row = num_of_pclk * 2 * 1000000 / g_psensor->pclk;
    // The t_row need to be consided fractional digit and round-off
    pinfo->t_row = num_of_pclk * 10000 / (g_psensor->pclk/1000) ;

    //printk("t_row=%d pclk=%d\n",pinfo->t_row,g_psensor->pclk);
}

static void adjust_vblk(int fps)
{
    //sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    //int tFrame, tRow;  //frame total time, row time
    int Vblk; //V blanking lines, H blanking
    int img_w, img_h;
    u32 x_start, x_end, y_start, y_end, extra_delay;
    int line_w, frame_h;

    read_reg(0x3008, &x_end);
    read_reg(0x3004, &x_start);
    read_reg(0x3006, &y_end);
    read_reg(0x3002, &y_start);
    read_reg(0x3042, &extra_delay);
    read_reg(0x300C, &line_w);

    img_w = x_end - x_start + 1;
    img_h = y_end - y_start + 1;

    //tFrame = 1/fps
    //=> ((Vblk+img_h)*line_w + extra_delay) * 1 / pclk = 1/fps
    //=> (Vblk+img_h)*line_w = pclk / fps - extra_delay
    //=> Vblk = (pclk / fps - 0) / line_w - img_h;
    Vblk = ((g_psensor->pclk / fps) - extra_delay) / (line_w*2) - img_h;

    if(Vblk < 32)  //min. V blanking
        Vblk = 32;
    frame_h = img_h + Vblk;

    //printk("vblk=%d, Vblk2 = %d\n", Vblk);

    write_reg(0x300A, (u32)frame_h);

    calculate_t_row();

    g_psensor->fps = fps;
}

static int set_size(u32 width, u32 height)
{
    int ret = 0;
    u32 x_start, x_end, y_start, y_end;
    // check size
    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("Exceed maximum sensor resolution!\n");
        return -1;
    }

    x_start = (6 + (SENSOR_MAX_WIDTH - width)/2) & ~BIT0;
    y_start = (6+ (SENSOR_MAX_HEIGHT - height)/2) & ~BIT0;
    x_end = (width - 1 + x_start) | BIT0;
    y_end = (height - 1 + y_start) | BIT0;
  
    write_reg(0X3006, y_end);
    write_reg(0x3008, x_end);
    write_reg(0X3002, y_start);
    write_reg(0X3004, x_start);
        

    // tricky:3M resolution
    if (width == 2032){
        write_reg(0x300A, 0x0ADE);
        write_reg(0x300C, 0x0458);
    }

    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    calculate_t_row();

    // update sensor information
    g_psensor->out_win.x = x_start;
    g_psensor->out_win.y = y_start;
    g_psensor->out_win.width = x_end - x_start + 1;
    g_psensor->out_win.height = y_end - y_start + 1;

    return ret;
}

static u32 get_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 num_of_line_pclk;

    if (!g_psensor->curr_exp) {
        read_reg(0x3012, &num_of_line_pclk);
        //read_reg(0x3014, &minus_pixel);
        g_psensor->curr_exp = (num_of_line_pclk * pinfo->t_row)/1000 ;
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 exp_line;

    // The t_row need to be consided fractional digit and round-off
    exp_line = MAX(1, ((exp * 1000)+pinfo->t_row/2)/pinfo->t_row);

    write_reg(0x3012, exp_line);
    // The t_row need to be consided fractional digit
    g_psensor->curr_exp = MAX(1, (exp_line * pinfo->t_row)/1000);

    return ret;
}

static u32 get_gain(void)
{
    u32 ang_x_1, dig_x;
    int ana_coarse, ana_fine;
    int i;
    int NUM_OF_GAINSET;
    gain_set_t *gain_table;
    
    if(wdr){
        NUM_OF_GAINSET = NUM_OF_GAINSET_WDR;
        gain_table = (gain_set_t *)gain_table_wdr;
    }
    else{
        NUM_OF_GAINSET = NUM_OF_GAINSET_LI;
        gain_table = (gain_set_t *)gain_table_li;
    }
    if (g_psensor->curr_gain==0) {
        //read_reg(0x3082, &exp_mode); //HDR mode or Combilinear mode
        read_reg(0x305E, &dig_x);
        read_reg(0x3060, &ang_x_1);
        dig_x = (dig_x & 0x07FF);
        ana_coarse = (ang_x_1 & 0x30)>>4;
        ana_fine = (ang_x_1 & 0x0F);

        for (i=0; i<NUM_OF_GAINSET; i++) {
            if ((gain_table[i].coarse == ana_coarse)&&(gain_table[i].fine == ana_fine)
                 && (gain_table[i].dig_x == dig_x))
                break;
        }
        if(i==NUM_OF_GAINSET) i--;
        //g_psensor->curr_gain = ((dig_x>>7 & 0xF)+((dig_x&0x7F)>>7))*(analog_gain_table[i].gain_x);
        g_psensor->curr_gain = (gain_table[i].gain_x);
    }
    return g_psensor->curr_gain;

}

/* analog canot frame sync but digital can do it, so this function canot occupy too long */
static int set_gain(u32 gain)
{
    u32 dig_x;
    u32 ang_reg_1;
    int i;
    int ret = 0;
    int NUM_OF_GAINSET = NUM_OF_GAINSET_LI;
    gain_set_t *gain_table;

    if(wdr){
        NUM_OF_GAINSET = NUM_OF_GAINSET_WDR;
        gain_table = (gain_set_t *)gain_table_wdr;
    }
    else{
        NUM_OF_GAINSET = NUM_OF_GAINSET_LI;
        gain_table = (gain_set_t *)gain_table_li;
    }

    if (gain > gain_table[NUM_OF_GAINSET - 1].gain_x)
        gain = gain_table[NUM_OF_GAINSET - 1].gain_x;

    if (gain < gain_table[0].gain_x)
        gain = gain_table[0].gain_x;
    // search most suitable gain into gain table

    for (i=0; i<NUM_OF_GAINSET; i++) {
        if (gain_table[i].gain_x > gain)
            break;
    }
   
    if (i > 0) i--;
//    ang_x_1 = gain_table[i].gain_x;
    dig_x =gain_table[i].dig_x;
    ang_reg_1 = (gain_table[i].coarse << 4) | (gain_table[i].fine & 0xF);

    write_reg(0x305E, dig_x);
    write_reg(0x3060, ang_reg_1);
    //
    if(wdr){
        if(ang_reg_1 <= 6)
            write_reg(0x3198, 0x7F1E);
        else if(ang_reg_1 <= 7)
            write_reg(0x3198, 0x7F3A);
        else if(ang_reg_1 <= 8)
            write_reg(0x3198, 0x7F3B);
        else if(ang_reg_1 <= 9)
            write_reg(0x3198, 0x7F3D);
        else if(ang_reg_1 <= 10)
            write_reg(0x3198, 0x7F3E);
        else if(ang_reg_1 <= 11)
            write_reg(0x3198, 0x7F3F);
        else if(ang_reg_1 <= 12)
            write_reg(0x3198, 0x7F41);
        else if(ang_reg_1 <= 13)
            write_reg(0x3198, 0x7F43);
        else if(ang_reg_1 <= 14)
            write_reg(0x3198, 0x7F45);
        else if(ang_reg_1 <= 15)
            write_reg(0x3198, 0x7F46);
        else if(ang_reg_1 <= 16)
            write_reg(0x3198, 0x7F49);
        else if(ang_reg_1 <= 18)
            write_reg(0x3198, 0x7F4B);
        else if(ang_reg_1 <= 20)
            write_reg(0x3198, 0x7F4E);
        else if(ang_reg_1 <= 22)
            write_reg(0x3198, 0x7F51);
        else if(ang_reg_1 <= 24)
            write_reg(0x3198, 0x7F54);
        else if(ang_reg_1 <= 26)
            write_reg(0x3198, 0x7F58);
        else if(ang_reg_1 <= 28)
            write_reg(0x3198, 0x7F5c);
        else if(ang_reg_1 <= 30)
            write_reg(0x3198, 0x7F61);
        else if(ang_reg_1 <= 32)
            write_reg(0x3198, 0x7F67);
        else if(ang_reg_1 <= 36)
            write_reg(0x3198, 0x7F6E);
        else if(ang_reg_1 <= 40)
            write_reg(0x3198, 0x7F77);
        else if(ang_reg_1 <= 44)
            write_reg(0x3198, 0x7F82);
        else
            write_reg(0x3198, 0x7F92);
    }
    //Important : Blooming artifact workaround

    if(ang_reg_1 < 22){//(gain_table[i].gain_x < 158){
        write_reg(0x3ED4, 0x8F6C);
        write_reg(0x3ED6, 0x6666);
        write_reg(0x3ED8, 0x8642);         
    }
    else if(ang_reg_1 >= 30){//(gain_table[i].gain_x > 224)
        write_reg(0x3ED4, 0x8FCC);
        write_reg(0x3ED6, 0xCCCC); 
        write_reg(0x3ED8, 0x8C42);         
    }
    else{
        write_reg(0x3ED4, 0x8F9C);
        write_reg(0x3ED6, 0x9999);         
        write_reg(0x3ED8, 0x8942);         
    }

    g_psensor->curr_gain = (gain_table[i].gain_x);
    //printk("gain=%d\n", g_psensor->curr_gain);

    return ret;
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
    write_reg(0x3040, reg);
    pinfo->mirror = enable;

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

    return 0;
}

#ifdef WDR_EXP_LIMIT
static int cal_wdr_min_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    //minimum exp line = 0.5*(T1/T2)=8
    int exp = MAX(1, pinfo->t_row * 8 / 1000);
    return exp;
}

static int cal_wdr_max_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 frame_len;
    int lines, exp;
    read_reg(0x300A, &frame_len); //frame_length_lines
    //maximum exp line = min(70*T1/T2, frame_len - 71)
    lines = MIN(70 * 16, frame_len - 71);
    exp = MAX(1, pinfo->t_row * lines / 1000);
    return exp;
}
#endif

static int set_wdr_en(int enable)
{
    u32 reg;
    read_reg(0x3082, &reg);
    //set to power down mode
    if (g_psensor->interface == IF_HiSPi)
        write_reg(0x301A, 0x0058);   // RESET_REGISTER
    else
        write_reg(0x301A, 0x10D8);   // RESET_REGISTER
    msleep(100);
 
    if(enable){
        wdr = 1;
#ifdef USE_DLO_MC
        // [DLO enabled]
        write_reg(0x3190, 0xE000);   // HDR_MC_CTRL4
        write_reg(0x301E, 0x00A8);
#else
        write_reg(0x3190, 0x0000);
        write_reg(0x318C, 0xC001);   // HDR_MC_CTRL2
        write_reg(0x3198, 0x061E); 	// HDR_MC_CTRL8
//        write_reg(0x301E, 0x0000); 	// DATA_PEDESTAL
        write_reg(0x30FE, 0x0000); 	// RESERVED_MFR_30FE
        write_reg(0x320A, 0x0000); 	// ADACD_PEDESTAL
#endif
        // [ADACD Enabled]
        write_reg(0x3202, 0x00CF);  // ADACD_NOISE_MODEL1
        write_reg(0x3206, 0x0A06);  // RESERVED_MFR_3206
        write_reg(0x3208, 0x1A12);  // RESERVED_MFR_3208
        write_reg(0x3200, 0x0002);  // ADACD_CONTROL
        // [HDR Mode Setup]
        write_reg(0x31D0, 0x0001);     // Enable data companding
        write_reg(0x3082, 0x0008);     // OPERATION_MODE_CTRL  //16x16
        // [Enable Embedded Data and Stats]
        write_reg(0x3064, 0x1882);  // SMIA_TEST           
#ifdef WDR_EXP_LIMIT
        g_psensor->min_exp = cal_wdr_min_exp();
        g_psensor->max_exp = cal_wdr_max_exp(); // 0.5 sec
        //g_psensor->max_exp = 400; // 1/25 sec        
#else
        g_psensor->min_exp = 1;
        g_psensor->max_exp = 400; // 1/25 sec
#endif
        g_psensor->max_gain = gain_table_wdr[NUM_OF_GAINSET_WDR-1].gain_x;
        g_psensor->fmt = RAW12_WDR16;
    }
    else{
        wdr = 0;
        // [Linear Mode Full Resolution]
        write_reg(0x3082, 0x0009);    // OPERATION_MODE_CTRL
        // [2D motion compensation OFF]
        write_reg(0x318C, 0x0000);
        write_reg(0x3190, 0x0000);
        write_reg(0x30FE, 0x0080);
        write_reg(0x320A, 0x0080);
        // [ALTM Bypassed]
           
        // [ADACD Disabled]
        write_reg(0x3200, 0x0000); 
        // [Companding Disabled]
        write_reg(0x31D0, 0x0000);
        write_reg(0x31E0, 0x0200);
        write_reg(0x3060, 0x0006);
        // SMIA_TEST
        write_reg(0x3064, 0x1882);        
        g_psensor->min_exp = 1;
        g_psensor->max_exp = 5000; // 0.5 sec
        g_psensor->max_gain = gain_table_li[NUM_OF_GAINSET_LI-1].gain_x;
        g_psensor->fmt = RAW12;
    }
    set_gain(g_psensor->curr_gain);
    
    //set to streaming mode
    if (g_psensor->interface == IF_HiSPi){
        write_reg(0x301A, 0x005C);   // REET_REGISTER
        isp_info("HiSPi LANE %d \n", ch_num);
	}
    else        
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
        set_wdr_en((int)arg);
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

static void set_pclk(void)
{
#define CRYSTAL_CLOCK   24000000
#define CHIP_CLOCK      27000000

    if (g_psensor->interface == IF_HiSPi){
        if (g_psensor->xclk == CHIP_CLOCK){
            // 4 lane
            write_reg(0x302A, 0x0006);
            write_reg(0x302C, 0x0001);
            write_reg(0x302E, 0x0004);
            if (g_psensor->img_win.width > 1920 && g_psensor->img_win.height > 1080){
                if (g_psensor->fps > 15)
                    write_reg(0x3030, 0x0042);
                else
                    write_reg(0x3030, 0x0021);
            }
            else{
                if (g_psensor->fps > 30)
                    write_reg(0x3030, 0x0042);
                else
                    write_reg(0x3030, 0x0021);
            }
            
            write_reg(0x31AE, 0x0304);
        }
        else{
            isp_err("HiSpi interface only support 27MHz input clock\n");
        }
    }
    else{
        if (g_psensor->xclk == CHIP_CLOCK){
            /* [PLL Enabled 27Mhz to 74.25Mhz] */
            write_reg(0x302C, 0x02);
            write_reg(0x302A, 0x04);
            write_reg(0x302E, 0x02);
            write_reg(0x3030, 0x2c);
            mdelay(100);
        } else if (g_psensor->xclk == CRYSTAL_CLOCK) {
            /* [PLL Enabled 24Mhz to 74.25Mhz] */
            write_reg(0x302C, 0x01);
            write_reg(0x302A, 0x08);
            write_reg(0x302E, 0x08);
            write_reg(0x3030, 0xc6);
            mdelay(100);
        } else{
            isp_info("Input XCLK=%d, set XCLK=%d\n",g_psensor->xclk,CHIP_CLOCK);
        }
    }
#undef CRYSTAL_CLOCK
#undef CHIP_CLOCK
}


static void set_protocol(void)
{
    u32 tmp;
    // Package protocol
    if (g_psensor->protocol == Streaming_SP){
        ;
        tmp = 0x8404;
        isp_info("protocol : Streaming_SP\n");
    }
    else{   // 0x8400
        g_psensor->protocol = Packetized_SP;
        tmp = 0x8400;
        isp_info("protocol : Packetized_SP\n");
    }
    if( write_reg( 0x31c6 , tmp) < 0 ) {
         isp_err("%s set protocol fail!!", SENSOR_NAME);
    }
}
static int init(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    int i, tempver;
    sensor_reg_t* pInitTable;
    u32 InitTable_Num;

    if (pinfo->is_init)
        return 0;

    read_reg(0x300E, &tempver);
    printk("sensor ver:%d\n", tempver);

    if (g_psensor->interface == IF_HiSPi){
        pInitTable=sensor_init_regs_hispi;
        InitTable_Num=NUM_OF_INIT_REGS_HISPI;
    }
    else{
        pInitTable=sensor_init_regs;
        InitTable_Num=NUM_OF_INIT_REGS;
    }
    for (i=0; i<InitTable_Num; i++) {
       if(pInitTable[i].addr == DELAY_CODE){
            mdelay(pInitTable[i].val);
        }
        else if(write_reg(pInitTable[i].addr, pInitTable[i].val) < 0) {
            isp_err("%s init fail!!\n", SENSOR_NAME);
            return -EINVAL;
        }
    }

    write_reg(0x3202, 0x3FF);
    write_reg(0x2438, 0x08);
    write_reg(0x2420, 0x00);  //denoise for edge of object
    write_reg(0x2440, 0x40);  //reduce black shadow in low lux region
    write_reg(0x3064, 0x1882); //disable embedded data in the first two rows

    set_pclk(); /* set pixel clock */
    if (g_psensor->interface == IF_HiSPi){
        set_protocol();
    }
    g_psensor->pclk = get_pclk(g_psensor->xclk);

    // get mirror and flip status
    pinfo->mirror = get_mirror();
    pinfo->flip = get_flip();

    // set image size
    ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height);
    if (ret == 0)
        pinfo->is_init = 1;

    // update sensor information
    adjust_vblk(g_psensor->fps);

    // initial exposure and gain
    set_exp(g_psensor->min_exp);
    set_gain(g_psensor->min_gain);

    // get current shutter_width and gain setting
    get_exp();
    get_gain();
    
    set_wdr_en(wdr);

    return ret;
}

//============================================================================
// external functions
//============================================================================
static void ar0331_deconstruct(void)
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

static int ar0331_construct(u32 xclk, u16 width, u16 height)
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
    g_psensor->capability = SUPPORT_MIRROR | SUPPORT_FLIP | SUPPORT_WDR;
    g_psensor->xclk = xclk;

    if(wdr)
        g_psensor->fmt = RAW12_WDR16;
    else
        g_psensor->fmt = RAW12;

	g_psensor->bayer_type = BAYER_GR;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_HIGH;

    g_psensor->img_win.x = 0;
    g_psensor->img_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    g_psensor->min_exp = 1;
    g_psensor->max_exp = 5000; // 0.5 sec
    if(wdr){
        g_psensor->min_gain = gain_table_wdr[0].gain_x;
        g_psensor->max_gain = gain_table_wdr[NUM_OF_GAINSET_WDR - 1].gain_x;
    }
    else{
        g_psensor->min_gain = gain_table_li[0].gain_x;
        g_psensor->max_gain = gain_table_li[NUM_OF_GAINSET_LI - 1].gain_x;
    }
    g_psensor->exp_latency = 2;
    g_psensor->gain_latency = 2;

    g_psensor->fn_read_reg = read_reg;
    g_psensor->fn_write_reg = write_reg;
    g_psensor->fn_init = init;
    g_psensor->fn_set_size = set_size;
    g_psensor->fn_get_exp = get_exp;
    g_psensor->fn_set_exp = set_exp;
    g_psensor->fn_get_gain = get_gain;
    g_psensor->fn_set_gain = set_gain;
    g_psensor->fps = fps;
    g_psensor->fn_get_property = get_property;
    g_psensor->fn_set_property = set_property;
    g_psensor->interface = interface;
    g_psensor->num_of_lane = ch_num;
    g_psensor->protocol = protocol;

    if ((ret = sensor_init_i2c_driver()) < 0)
        goto construct_err;

    if ((ret = init()) < 0)
        goto construct_err;
        printk("AR0331 in_w=%d, in_h=%d, out_w=%d, out_h=%d\n", g_psensor->img_win.width, g_psensor->img_win.height,
        g_psensor->out_win.width, g_psensor->out_win.height);

    return 0;

construct_err:
    ar0331_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init ar0331_init(void)
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

    if ((ret = ar0331_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit ar0331_exit(void)
{
    isp_unregister_sensor(g_psensor);
    ar0331_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(ar0331_init);
module_exit(ar0331_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor AR0331");
MODULE_LICENSE("GPL");
