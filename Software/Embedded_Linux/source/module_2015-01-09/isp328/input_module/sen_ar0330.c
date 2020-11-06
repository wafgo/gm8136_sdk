/**
 * @file sen_ar0330.c
 * Aptina AR0330 sensor driver
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
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

#define PFX "sen_ar0330"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     2048
#define DEFAULT_IMAGE_HEIGHT    1536
#define DEFAULT_XCLK            27000000
#define DEFAULT_CH_NUM       	2
#define DEFAULT_INTERFACE       IF_MIPI

static ushort ch_num = DEFAULT_CH_NUM;
module_param(ch_num, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_num, "channel number");

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

//=============================================================================
// global
//============================================================================
#define SENSOR_NAME         "AR0330"
#define SENSOR_MAX_WIDTH    2304
#define SENSOR_MAX_HEIGHT   1536
#define SENSOR_MAX_EXP      2000  //us


static sensor_dev_t *g_psensor = NULL;

#define DELAY_CODE      0xFFFF

typedef struct sensor_info {
    int is_init;
    int mirror;
    int flip;
    u32 max_exp_line;
    u32 ang_reg_1;      /* record reg(0x30b0) */
    u32 t_row;          /* unit: 0.01us */
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

static sensor_reg_t sensor_init_regs_3M[] = {
    //2304*1296
      //PLL_settings
    {0x301A, 0x0058},		//RESET_REGISTER = 88
    {0x302A, 0x0006},		//VT_PIX_CLK_DIV = 6
    {0x302C, 0x0002},		//VT_SYS_CLK_DIV = 2
    {0x302E, 0x0003},		//PRE_PLL_CLK_DIV = 3
    {0x3030, 0x0042},		//PLL_MULTIPLIER = 66, 30 fps
//    {0x3030, 0x0050},		//PLL_MULTIPLIER = 80, 36 fps test
    {0x3036, 0x000C},		//OP_PIX_CLK_DIV = 12
    {0x3038, 0x0001},		//OP_SYS_CLK_DIV = 1
    {0x31AC, 0x0C0C},		//DATA_FORMAT_BITS = 3084
    {0x301A, 0x005C},		//RESET_REGISTER = 92
      //MIPI Port Timing
    {0x31B0, 0x0024},
    {0x31B2, 0x000c},
    {0x31B4, 0x2643},
    {0x31B6, 0x114e},
    {0x31B8, 0x2048},
    {0x31BA, 0x0186},
    {0x31BC, 0x8005},
      //Timing_settings
    {0x31AE, 0x0202},		//SERIAL_FORMAT = 514
    {0x3002, 0x0082},		//Y_ADDR_START = 8
    {0x3004, 0x0006},		//X_ADDR_START = 6
    {0x3006, 0x0591},		//Y_ADDR_END = 1306
    {0x3008, 0x0905},		//X_ADDR_END = 2311
    {0x300A, 0x052a},		//FRAME_LENGTH_LINES = 1322
    {0x300C, 0x04e0},		//LINE_LENGTH_PCK = 1248

    {0x3012, 0x051b},		//COARSE_INTEGRATION_TIME = 1575
    {0x3014, 0x0000},		//FINE_INTEGRATION_TIME = 0
    {0x30A2, 0x0001},		//X_ODD_INC = 1
    {0x30A6, 0x0001},		//Y_ODD_INC = 1
    {0x308C, 0x0006},		//Y_ADDR_START_CB = 6
    {0x308A, 0x0006},		//X_ADDR_START_CB = 6
    {0x3090, 0x0605},		//Y_ADDR_END_CB = 1541
    {0x308E, 0x0905},		//X_ADDR_END_CB = 2309
    {0x30AA, 0x0642},		//FRAME_LENGTH_LINES_CB = 1602
    {0x303E, 0x04E0},		//LINE_LENGTH_PCK_CB = 1248
    {0x3016, 0x0641},		//COARSE_INTEGRATION_TIME_CB = 1601
    {0x3018, 0x0000},		//FINE_INTEGRATION_TIME_CB = 0
    {0x30AE, 0x0001},		//X_ODD_INC_CB = 1
    {0x30A8, 0x0001},		//Y_ODD_INC_CB = 1
    {0x3040, 0x0000},		//READ_MODE = 0
    {0x3042, 0x03b5},		//EXTRA_DELAY = 380
    {0x30BA, 0x002C},		//DIGITAL_CTRL = 44
    {0x3088, 0x80BA},		//SEQ_CTRL_PORT = 32954
    {0x3086, 0xE653},		//SEQ_DATA_PORT = 58963
};
#define NUM_OF_INIT_REGS_3M (sizeof(sensor_init_regs_3M) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_init_regs_3M_1536p[] = {
    //2048x1536 12 bit
      //PLL_settings
    {0x301A, 0x0058},		//RESET_REGISTER = 88
    {0x302A, 0x0006},		//VT_PIX_CLK_DIV = 6
    {0x302C, 0x0002},		//VT_SYS_CLK_DIV = 2
    {0x302E, 0x0003},		//PRE_PLL_CLK_DIV = 3
    {0x3030, 0x004e},		//PLL_MULTIPLIER = 66
    {0x3036, 0x000C},		//OP_PIX_CLK_DIV = 12
    {0x3038, 0x0001},		//OP_SYS_CLK_DIV = 1
    {0x31AC, 0x0C0C},		//DATA_FORMAT_BITS = 3084
    {0x301A, 0x005C},		//RESET_REGISTER = 92
      //MIPI Port Timing
    {0x31B0, 0x0024},
    {0x31B2, 0x000c},
    {0x31B4, 0x2643},
    {0x31B6, 0x114e},
    {0x31B8, 0x2048},
    {0x31BA, 0x0186},
    {0x31BC, 0x8005},
      //Timing_settings
    {0x31AE, 0x0202},		//SERIAL_FORMAT = 514
    {0x3002, 0x0006},		//Y_ADDR_START = 8
    {0x3004, 0x0008},		//X_ADDR_START = 6
    {0x3006, 0x0607},		//Y_ADDR_END = 1306
    {0x3008, 0x0907},		//X_ADDR_END = 2311
    {0x300A, 0x061a},		//FRAME_LENGTH_LINES = 1322
    {0x300C, 0x04e0},		//LINE_LENGTH_PCK = 1248
};
#define NUM_OF_INIT_REGS_3M_1536P (sizeof(sensor_init_regs_3M_1536p) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_init_regs_1080p[] = {      //1920*1080
      //PLL_settings
    {0x301A, 0x0058},       //RESET_REGISTER = 88
    {0x302A, 0x0006},       //VT_PIX_CLK_DIV = 6
    {0x302C, 0x0002},       //VT_SYS_CLK_DIV = 2
    {0x302E, 0x0002},       //PRE_PLL_CLK_DIV = 2
    {0x3030, 0x0038},       //PLL_MULTIPLIER = 56
    {0x3036, 0x000C},       //OP_PIX_CLK_DIV = 12
    {0x3038, 0x0001},       //OP_SYS_CLK_DIV = 1
    {0x31AC, 0x0C0C},       //DATA_FORMAT_BITS = 3084
    {0x301A, 0x005C},		//RESET_REGISTER = 92

      //MIPI Port Timing
    {0x31B0, 0x0036},
    {0x31B2, 0x0015},
    {0x31B4, 0x3D55},
    {0x31B6, 0x41D0},
    {0x31B8, 0x208A},
    {0x31BA, 0x0288},
    {0x31BC, 0x8007},

      //Timing_settings
    {0x31AE, 0x0202},		//SERIAL_FORMAT = 514
    {0x3002, 0x00EA},       //Y_ADDR_START = 234
    {0x3004, 0x00C6},       //X_ADDR_START = 198
    {0x3006, 0x0521},       //Y_ADDR_END = 1313
    {0x3008, 0x0845},       //X_ADDR_END = 2117
    {0x300A, 0x0461},       //FRAME_LENGTH_LINES = 1121
    {0x300C, 0x04E0},       //LINE_LENGTH_PCK = 1248

    {0x3064, 0x1902},
    {0x31be, 0x2003},

    {0x3012, 0x034D},       //COARSE_INTEGRATION_TIME = 845
    {0x3014, 0x0000},		//FINE_INTEGRATION_TIME = 0
    {0x30A2, 0x0001},		//X_ODD_INC = 1
    {0x30A6, 0x0001},		//Y_ODD_INC = 1
    {0x308C, 0x0006},		//Y_ADDR_START_CB = 6
    {0x308A, 0x0006},		//X_ADDR_START_CB = 6
    {0x3090, 0x0605},		//Y_ADDR_END_CB = 1541
    {0x308E, 0x0905},		//X_ADDR_END_CB = 2309
    {0x30AA, 0x0642},		//FRAME_LENGTH_LINES_CB = 1602
    {0x303E, 0x04E0},		//LINE_LENGTH_PCK_CB = 1248
    {0x3016, 0x0641},		//COARSE_INTEGRATION_TIME_CB = 1601
    {0x3018, 0x0000},		//FINE_INTEGRATION_TIME_CB = 0
    {0x30AE, 0x0001},		//X_ODD_INC_CB = 1
    {0x30A8, 0x0001},		//Y_ODD_INC_CB = 1
    {0x3040, 0x0000},       //READ_MODE = 0
    {0x3042, 0x0000},       //EXTRA_DELAY
    {0x30BA, 0x002C},		//DIGITAL_CTRL = 44
    {0x3088, 0x80BA},		//SEQ_CTRL_PORT = 32954
    {0x3086, 0xE653},		//SEQ_DATA_PORT = 58963
};

#define NUM_OF_INIT_REGS_1080P (sizeof(sensor_init_regs_1080p) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_init_regs_3M_parallel[] = {
    //PLL_settings
    //STATE = Master Clock, 98357147
    {0x301A, 0x10D8},		//RESET_REGISTER = 4312
    {0x302A, 0x0006},		//VT_PIX_CLK_DIV = 6
    {0x302C, 0x0001},		//VT_SYS_CLK_DIV = 1
    {0x302E, 0x0007},		//PRE_PLL_CLK_DIV = 7
    {0x3030, 0x0099},		//PLL_MULTIPLIER = 153
    {0x3036, 0x000C},		//OP_PIX_CLK_DIV = 12
    {0x3038, 0x0001},		//OP_SYS_CLK_DIV = 1
    {0x31AC, 0x0C0C},		//DATA_FORMAT_BITS = 3084
    {0x31AE, 0x0301},		//SERIAL_FORMAT = 769
    //Timing_settings
    {0x3002, 0x007E},		//Y_ADDR_START = 126
    {0x3004, 0x0006},		//X_ADDR_START = 6
    {0x3006, 0x058D},		//Y_ADDR_END = 1421
    {0x3008, 0x0905},		//X_ADDR_END = 2309
    {0x300A, 0x0521},		//FRAME_LENGTH_LINES = 1313
    {0x300C, 0x04E0},		//LINE_LENGTH_PCK = 1248
    {0x3012, 0x0520},		//COARSE_INTEGRATION_TIME = 1312
    {0x3014, 0x0000},		//FINE_INTEGRATION_TIME = 0
    {0x30A2, 0x0001},		//X_ODD_INC = 1
    {0x30A6, 0x0001},		//Y_ODD_INC = 1
    {0x308C, 0x0006},		//Y_ADDR_START_CB = 6
    {0x308A, 0x0006},		//X_ADDR_START_CB = 6
    {0x3090, 0x0605},		//Y_ADDR_END_CB = 1541
    {0x308E, 0x0905},		//X_ADDR_END_CB = 2309
    {0x30AA, 0x1EC9},		//FRAME_LENGTH_LINES_CB = 7881
    {0x303E, 0x04E0},		//LINE_LENGTH_PCK_CB = 1248
    {0x3016, 0x1EC8},		//COARSE_INTEGRATION_TIME_CB = 7880
    {0x3018, 0x0000},		//FINE_INTEGRATION_TIME_CB = 0
    {0x30AE, 0x0001},		//X_ODD_INC_CB = 1
    {0x30A8, 0x0001},		//Y_ODD_INC_CB = 1
    {0x3040, 0x0000},		//READ_MODE = 0
    {0x3042, 0x0295},		//EXTRA_DELAY = 661
    {0x30BA, 0x002C},		//DIGITAL_CTRL = 44
    //Recommended Configuration
    {0x31E0, 0x0303},
    {0x3064, 0x1802},
    {0x3ED2, 0x0146},
    {0x3ED4, 0x8F6C},
    {0x3ED6, 0x66CC},
    {0x3ED8, 0x8C42},
    {0x3EDA, 0x88BC},
    {0x3EDC, 0xAA63},
    {0x305E, 0x00A0},
    {0x301A, 0x10D8},		//Disable Streaming
    // [Sequencer Normal Length]
    {0x301A, 0x10D8},
    {DELAY_CODE, 100},
    {0x3088, 0x80BA},
    {0x3086, 0xE653},
    {0x301A, 0x10DC},		//Enable Streaming
};
#define NUM_OF_INIT_REGS_3M_PARALLEL (sizeof(sensor_init_regs_3M_parallel) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_init_regs_3M_1536p_parallel[] = {
    //PLL_settings
    //STATE = Master Clock, 98357147
    {0x301A, 0x10D8},		//RESET_REGISTER = 4312
    {0x302A, 0x0006},		//VT_PIX_CLK_DIV = 6
    {0x302C, 0x0001},		//VT_SYS_CLK_DIV = 1
    {0x302E, 0x0007},		//PRE_PLL_CLK_DIV = 7
    {0x3030, 0x0099},		//PLL_MULTIPLIER = 153
    {0x3036, 0x000C},		//OP_PIX_CLK_DIV = 12
    {0x3038, 0x0001},		//OP_SYS_CLK_DIV = 1
    {0x31AC, 0x0C0C},		//DATA_FORMAT_BITS = 3084
    {0x31AE, 0x0301},		//SERIAL_FORMAT = 769
    //Timing_settings
    {0x3002, 0x0006},		//Y_ADDR_START = 6
    {0x3004, 0x0006},		//X_ADDR_START = 134
    {0x3006, 0x0605},		//Y_ADDR_END = 1541
    {0x3008, 0x08f5},		//X_ADDR_END = 2181
    {0x300A, 0x062F},		//FRAME_LENGTH_LINES = 1583
    {0x300C, 0x04DA},		//LINE_LENGTH_PCK = 1242
    {0x3012, 0x0317},		//COARSE_INTEGRATION_TIME = 791
    {0x3014, 0x0000},		//FINE_INTEGRATION_TIME = 0
    {0x30A2, 0x0001},		//X_ODD_INC = 1
    {0x30A6, 0x0001},		//Y_ODD_INC = 1
    {0x308C, 0x0006},		//Y_ADDR_START_CB = 6
    {0x308A, 0x0006},		//X_ADDR_START_CB = 6
    {0x3090, 0x0605},		//Y_ADDR_END_CB = 1541
    {0x308E, 0x0905},		//X_ADDR_END_CB = 2309
    {0x30AA, 0x1EC9},		//FRAME_LENGTH_LINES_CB = 7881
    {0x303E, 0x04E0},		//LINE_LENGTH_PCK_CB = 1248
    {0x3016, 0x1EC8},		//COARSE_INTEGRATION_TIME_CB = 7880
    {0x3018, 0x0000},		//FINE_INTEGRATION_TIME_CB = 0
    {0x30AE, 0x0001},		//X_ODD_INC_CB = 1
    {0x30A8, 0x0001},		//Y_ODD_INC_CB = 1
    {0x3040, 0x0000},		//READ_MODE = 0
    {0x3042, 0x0420},		//EXTRA_DELAY = 1056
    {0x30BA, 0x002C},		//DIGITAL_CTRL = 44
    //Recommended Configuration
    {0x31E0, 0x0303},
    {0x3064, 0x1802},
    {0x3ED2, 0x0146},
    {0x3ED4, 0x8F6C},
    {0x3ED6, 0x66CC},
    {0x3ED8, 0x8C42},
    {0x3EDA, 0x88BC},
    {0x3EDC, 0xAA63},
    {0x305E, 0x00A0},
    //{0x301A, 0x025C},		//Enable Streaming
    {0x301A, 0x10D8},		//Disable Streaming
    // [Sequencer Normal Length]
    {0x301A, 0x10D8},
    {DELAY_CODE, 100},
    {0x3088, 0x80BA},
    {0x3086, 0xE653},
    {0x301A, 0x10DC},		//Enable Streaming
};
#define NUM_OF_INIT_REGS_3M_1536P_PARALLEL (sizeof(sensor_init_regs_3M_1536p_parallel) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_init_regs_1080p_parallel[] = {
    //PLL_settings
    //STATE = Master Clock, 82000000
    {0x301A, 0x10D8},		//RESET_REGISTER = 4312
    {0x302A, 0x0006},		//VT_PIX_CLK_DIV = 6
    {0x302C, 0x0001},		//VT_SYS_CLK_DIV = 1
    {0x302E, 0x0009},		//PRE_PLL_CLK_DIV = 9
    {0x3030, 0x00A4},		//PLL_MULTIPLIER = 164
    {0x3036, 0x000C},		//OP_PIX_CLK_DIV = 12
    {0x3038, 0x0001},		//OP_SYS_CLK_DIV = 1
    {0x31AC, 0x0C0C},		//DATA_FORMAT_BITS = 3084
    {0x31AE, 0x0301},		//SERIAL_FORMAT = 769
    //Timing_settings
    {0x3002, 0x00EA},		//Y_ADDR_START = 234
    {0x3004, 0x0006},		//X_ADDR_START = 198
    {0x3006, 0x0521},		//Y_ADDR_END = 1313
    {0x3008, 0x08f5},		//X_ADDR_END = 2117
    {0x300A, 0x044C},		//FRAME_LENGTH_LINES = 1100
    {0x300C, 0x04DA},		//LINE_LENGTH_PCK = 1242
    {0x3012, 0x03DE},		//COARSE_INTEGRATION_TIME = 990
    {0x3014, 0x0000},		//FINE_INTEGRATION_TIME = 0
    {0x30A2, 0x0001},		//X_ODD_INC = 1
    {0x30A6, 0x0001},		//Y_ODD_INC = 1
    {0x308C, 0x0006},		//Y_ADDR_START_CB = 6
    {0x308A, 0x0006},		//X_ADDR_START_CB = 6
    {0x3090, 0x0605},		//Y_ADDR_END_CB = 1541
    {0x308E, 0x0905},		//X_ADDR_END_CB = 2309
    {0x30AA, 0x19AA},		//FRAME_LENGTH_LINES_CB = 6570
    {0x303E, 0x04E0},		//LINE_LENGTH_PCK_CB = 1248
    {0x3016, 0x19A9},		//COARSE_INTEGRATION_TIME_CB = 6569
    {0x3018, 0x0000},		//FINE_INTEGRATION_TIME_CB = 0
    {0x30AE, 0x0001},		//X_ODD_INC_CB = 1
    {0x30A8, 0x0001},		//Y_ODD_INC_CB = 1
    {0x3040, 0x0000},		//READ_MODE = 0
    {0x3042, 0x01D2},		//EXTRA_DELAY = 466
    {0x30BA, 0x002C},		//DIGITAL_CTRL = 44
//Recommended Configuration
    {0x31E0, 0x0303},
    {0x3064, 0x1802},
    {0x3ED2, 0x0146},
    {0x3ED4, 0x8F6C},
    {0x3ED6, 0x66CC},
    {0x3ED8, 0x8C42},
    {0x3EDA, 0x88BC},
    {0x3EDC, 0xAA63},
    {0x305E, 0x00A0},
    {0x301A, 0x10D8},		//Disable Streaming
// [Sequencer Normal Length]
    {0x301A, 0x10D8},
    {DELAY_CODE, 100},
    {0x3088, 0x80BA},
    {0x3086, 0xE653},
    {0x301A, 0x10DC},		//Enable Streaming
};
#define NUM_OF_INIT_REGS_1080P_PARALLEL (sizeof(sensor_init_regs_1080p_parallel) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_init_regs_720p_parallel[] = {
    //PLL_settings
    //STATE = Master Clock, 64000000
    {0x301A, 0x10D8},		//RESET_REGISTER = 4312
    {0x302A, 0x0006},		//VT_PIX_CLK_DIV = 6
    {0x302C, 0x0001},		//VT_SYS_CLK_DIV = 1
    {0x302E, 0x0009},		//PRE_PLL_CLK_DIV = 9
    {0x3030, 0x0080},		//PLL_MULTIPLIER = 128
    {0x3036, 0x000C},		//OP_PIX_CLK_DIV = 12
    {0x3038, 0x0001},		//OP_SYS_CLK_DIV = 1
    {0x31AC, 0x0C0C},		//DATA_FORMAT_BITS = 3084
    {0x31AE, 0x0301},		//SERIAL_FORMAT = 769
    //Timing_settings
    {0x3002, 0x019E},		//Y_ADDR_START = 414
    {0x3004, 0x0206},		//X_ADDR_START = 518
    {0x3006, 0x046D},		//Y_ADDR_END = 1133
    {0x3008, 0x0705},		//X_ADDR_END = 1797
    {0x300A, 0x035A},		//FRAME_LENGTH_LINES = 858
    {0x300C, 0x04DA},		//LINE_LENGTH_PCK = 1242
    {0x3012, 0x0304},		//COARSE_INTEGRATION_TIME = 772
    {0x3014, 0x0000},		//FINE_INTEGRATION_TIME = 0
    {0x30A2, 0x0001},		//X_ODD_INC = 1
    {0x30A6, 0x0001},		//Y_ODD_INC = 1
    {0x308C, 0x0006},		//Y_ADDR_START_CB = 6
    {0x308A, 0x0006},		//X_ADDR_START_CB = 6
    {0x3090, 0x0605},		//Y_ADDR_END_CB = 1541
    {0x308E, 0x0905},		//X_ADDR_END_CB = 2309
    {0x30AA, 0x1408},		//FRAME_LENGTH_LINES_CB = 5128
    {0x303E, 0x04E0},		//LINE_LENGTH_PCK_CB = 1248
    {0x3016, 0x1407},		//COARSE_INTEGRATION_TIME_CB = 5127
    {0x3018, 0x0000},		//FINE_INTEGRATION_TIME_CB = 0
    {0x30AE, 0x0001},		//X_ODD_INC_CB = 1
    {0x30A8, 0x0001},		//Y_ODD_INC_CB = 1
    {0x3040, 0x0000},		//READ_MODE = 0
    {0x3042, 0x0406},		//EXTRA_DELAY = 1030
    {0x30BA, 0x002C},		//DIGITAL_CTRL = 44
    //Recommended Configuration
    {0x31E0, 0x0303},
    {0x3064, 0x1802},
    {0x3ED2, 0x0146},
    {0x3ED4, 0x8F6C},
    {0x3ED6, 0x66CC},
    {0x3ED8, 0x8C42},
    {0x3EDA, 0x88BC},
    {0x3EDC, 0xAA63},
    {0x305E, 0x00A0},
    {0x301A, 0x10D8},		//Disable Streaming
    // [Sequencer Normal Length]
    {0x301A, 0x10D8},
    {DELAY_CODE, 100},
    {0x3088, 0x80BA},
    {0x3086, 0xE653},
    {0x301A, 0x10DC},		//Enable Streaming
};
#define NUM_OF_INIT_REGS_720P_PARALLEL (sizeof(sensor_init_regs_720p_parallel) / sizeof(sensor_reg_t))

typedef struct _gain_set {
    u8 coarse;
    u8 fine;
    u16 dig_x;
    u32 gain_x;
} gain_set_t;

static const gain_set_t gain_table[] = {
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
    {2, 1,  128,  264 },
    {2, 2,  128,  274 },
    {2, 4,  128,  292 },
    {2, 6,  128,  315 },
    {2, 8,  128,  342 },
    {2, 10, 128,  371 },
    {2, 12, 128,  410 },
    {2, 14, 128,  456 },
    {2, 15, 128,  481 },
    {3, 0,  128,  512 },
#if 0
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
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

static const gain_set_t a_gain_table[] = {
    {0,  0, 0, 64 }, 
    {0,  1, 0, 66 }, 
    {0,  2, 0, 68 }, 
    {0,  3, 0, 70 }, 
    {0,  4, 0, 73 }, 
    {0,  5, 0, 76 }, 
    {0,  6, 0, 79 }, 
    {0,  7, 0, 82 }, 
    {0,  8, 0, 85 }, 
    {0,  9, 0, 89 }, 
    {0, 10, 0, 93 }, 
    {0, 11, 0, 97 }, 
    {0, 12, 0, 102},
    {0, 13, 0, 108},
    {0, 14, 0, 114},
    {0, 15, 0, 120},
    {1,  0, 0, 128},
    {1,  2, 0, 136},
    {1,  4, 0, 147},
    {1,  6, 0, 157},
    {1,  8, 0, 171},
    {1, 10, 0, 186},
    {1, 12, 0, 205},
    {1, 14, 0, 228},
    {2,  0, 0, 256},
    {2,  4, 0, 292},
    {2,  8, 0, 341},
    {2, 12, 0, 410},
    {3,  0, 0, 512},
};
#define NUM_OF_A_GAINSET (sizeof(a_gain_table) / sizeof(gain_set_t))


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

//    isp_info("write_reg 0x%4x 0x%4x\n", addr, val);
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

    if (g_psensor->interface == IF_MIPI){
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
//        pclk /= vt_sys_clk_div;
        pclk *= ch_num;
        pclk /= bits;
    }
    else{
        pclk = xclk / vt_pix_clk_div;
        pclk *= pll_multiplier;
        pclk /= vt_sys_clk_div;
        pclk /= pre_pll_clk_div;
    }

    isp_info("pclk(%d) XCLK(%d)\n", pclk, xclk);
    return pclk;
}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 num_of_pclk;
    u32 line_length_pck;

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
    int line_w, frame_h, max_fps;

    if (g_psensor->interface == IF_PARALLEL){
        if (g_psensor->img_win.width <= 2304 && g_psensor->img_win.height <= 1296)
            max_fps = 30;
        else
            max_fps = 25;
    }
    else if (g_psensor->img_win.width <= 1920 && g_psensor->img_win.height <= 1080)
        max_fps = 46;
    else
        max_fps = 30;
    if (fps > max_fps)
        fps = max_fps;

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

    if(Vblk < 12)  //min. V blanking
        Vblk = 12;
    frame_h = img_h + Vblk;

    isp_info("fps=%d, Vblk = %d\n", fps, Vblk);

    write_reg(0x300A, (u32)frame_h);

    calculate_t_row();

    g_psensor->fps = fps;
}

static int set_mirror(int enable);
static int set_flip(int enable);

static int set_size(u32 width, u32 height)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    sensor_reg_t* pInitTable;
    u32 InitTable_Num;
    int i, ret = 0;

    // check size
    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("Exceed maximum sensor resolution!\n");
        return -1;
    }

    if (g_psensor->interface == IF_MIPI){
        isp_info("Interface : MIPI");
        if (width <= 1920 && height <= 1080){
            pInitTable=sensor_init_regs_1080p;
            InitTable_Num=NUM_OF_INIT_REGS_1080P;
            g_psensor->out_win.width = 1920;
            g_psensor->out_win.height = 1080;
        }
        else if (width <= 2304 && height <= 1296){
            pInitTable=sensor_init_regs_3M;
            InitTable_Num=NUM_OF_INIT_REGS_3M;
            g_psensor->out_win.width = 2304;
            g_psensor->out_win.height = 1296;
        }
        else {
            pInitTable=sensor_init_regs_3M_1536p;
            InitTable_Num=NUM_OF_INIT_REGS_3M_1536P;
            g_psensor->out_win.width = 2304;
            g_psensor->out_win.height = 1536;
        }
    }
    else{
        isp_info("Interface : Parallel");
        if (width <= 1280 && height <= 720){
            pInitTable=sensor_init_regs_720p_parallel;
            InitTable_Num=NUM_OF_INIT_REGS_720P_PARALLEL;
            g_psensor->out_win.width = 1280;
            g_psensor->out_win.height = 720;
        }
        else if (width <= 1920 && height <= 1080){
            pInitTable=sensor_init_regs_1080p_parallel;
            InitTable_Num=NUM_OF_INIT_REGS_1080P_PARALLEL;
            g_psensor->out_win.width = 2288;
            g_psensor->out_win.height = 1080;
        }
        else if (width <= 2304 && height <= 1296){
            pInitTable=sensor_init_regs_3M_parallel;
            InitTable_Num=NUM_OF_INIT_REGS_3M_PARALLEL;
            g_psensor->out_win.width = 2304;
            g_psensor->out_win.height = 1296;
        }
        else {
            pInitTable=sensor_init_regs_3M_1536p_parallel;
            InitTable_Num=NUM_OF_INIT_REGS_3M_1536P_PARALLEL;
            g_psensor->out_win.width = 2288;
            g_psensor->out_win.height = 1536;
        }
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
    if (g_psensor->interface == IF_MIPI){
        if (ch_num == 1){
            write_reg(0x302C, 0x0004);
            write_reg(0x31AE, 0x0201);
        }
        isp_info("ch num = %d\n", ch_num);
    }

    g_psensor->img_win.x = ((g_psensor->out_win.width-width)/2) & 0xfffe;
    g_psensor->img_win.y = ((g_psensor->out_win.height-height)/2) & 0xfffe;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    g_psensor->pclk = get_pclk(g_psensor->xclk);
    adjust_vblk(g_psensor->fps);

    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;

    set_mirror(pinfo->mirror);
    set_flip(pinfo->flip);

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
static u32 get_a_gain(void);
static u32 get_d_gain(void);

static int set_gain(u32 gain)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 dig_x;
    u32 ang_reg_1;
    int i;
    int ret = 0;

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
    //Important : Blooming artifact workaround
    if(ang_reg_1 < 22){  //Not 16, to avoid green side effect in dark region
        write_reg(0x3ED4, 0x8F6C);
        write_reg(0x3ED6, 0x6666);
        write_reg(0x3ED8, 0x8642);
    }
    else if(ang_reg_1 > 30){
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
    pinfo->curr_a_gain = 0;
    get_a_gain();
    pinfo->curr_d_gain = 0;
    get_d_gain();
//    printk("gain=%d\n", g_psensor->curr_gain);

    return ret;
}

static u32 get_a_gain(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 ang_x_1;
    int ana_coarse, ana_fine;
    int i;
    
    if (pinfo->curr_a_gain==0) {
        read_reg(0x3060, &ang_x_1);
        ana_coarse = (ang_x_1 & 0x30)>>4;
        ana_fine = (ang_x_1 & 0x0F);

        for (i=0; i<NUM_OF_A_GAINSET; i++) {
            if ((a_gain_table[i].coarse == ana_coarse)&&(a_gain_table[i].fine == ana_fine))
                break;
        }
        if(i==NUM_OF_A_GAINSET) i--;
        pinfo->curr_a_gain = a_gain_table[i].gain_x;
    }
    return pinfo->curr_a_gain;

}

/* analog canot frame sync but digital can do it, so this function canot occupy too long */
static int set_a_gain(u32 gain)
{
    u32 ang_reg_1;
    int i;
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (gain > a_gain_table[NUM_OF_A_GAINSET - 1].gain_x)
        gain = a_gain_table[NUM_OF_A_GAINSET - 1].gain_x;

    if (gain < a_gain_table[0].gain_x)
        gain = a_gain_table[0].gain_x;
    
    // search most suitable gain into gain table
    for (i=0; i<NUM_OF_A_GAINSET; i++) {
        if (a_gain_table[i].gain_x > gain)
            break;
    }
    if (i > 0) i--;
    ang_reg_1 = (a_gain_table[i].coarse << 4) | (a_gain_table[i].fine & 0xF);
    
    write_reg(0x3060, ang_reg_1);
#if 0    
    //Important : Blooming artifact workaround
    if(ang_reg_1 < 22){  //Not 16, to avoid green side effect in dark region
        write_reg(0x3ED4, 0x8F6C);
        write_reg(0x3ED6, 0x6666);
        write_reg(0x3ED8, 0x8642);         
    }
    else if(ang_reg_1 > 30){
        write_reg(0x3ED4, 0x8FCC);
        write_reg(0x3ED6, 0xCCCC); 
        write_reg(0x3ED8, 0x8C42);         
    }
    else{
        write_reg(0x3ED4, 0x8F9C);
        write_reg(0x3ED6, 0x9999);         
        write_reg(0x3ED8, 0x8942);         
    }
#endif   
    pinfo->curr_a_gain = (a_gain_table[i].gain_x);

    return pinfo->curr_a_gain;
}

static u32 get_d_gain(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 dig_x;
    
    if (pinfo->curr_d_gain==0) {
        read_reg(0x305E, &dig_x);
        dig_x = (dig_x & 0x07FF);
        pinfo->curr_d_gain = dig_x * 64 / 128;
    }
    return pinfo->curr_d_gain;

}

static int set_d_gain(u32 gain)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 dig_x;

    if (gain > pinfo->max_d_gain)
        gain = pinfo->max_d_gain;
    if (gain < pinfo->min_d_gain)
        gain = pinfo->min_d_gain;

    dig_x = (gain * 128 / 64) & 0x7ff;

    write_reg(0x305E, dig_x);
   
    pinfo->curr_d_gain = gain;

    return pinfo->curr_d_gain;
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
        break;
    case ID_FLIP:
        set_flip((int)arg);
        break;
    case ID_FPS:
        adjust_vblk((int)arg);
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

    pinfo->mirror = mirror;
    pinfo->flip = flip;

    // set image size
    ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height);
    if (ret == 0)
        pinfo->is_init = 1;

    // initial exposure and gain
    set_exp(g_psensor->min_exp);
    set_gain(g_psensor->min_gain);

    // get current shutter_width and gain setting
    get_exp();
    get_gain();

    return ret;
}

//============================================================================
// external functions
//============================================================================
static void ar0330_deconstruct(void)
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

static int ar0330_construct(u32 xclk, u16 width, u16 height)
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
    g_psensor->min_gain = gain_table[0].gain_x;
    g_psensor->max_gain = MIN(8000, gain_table[NUM_OF_GAINSET - 1].gain_x);
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
    pinfo->min_a_gain = a_gain_table[0].gain_x;
    pinfo->max_a_gain = a_gain_table[NUM_OF_A_GAINSET - 1].gain_x;
    pinfo->curr_a_gain = 0;
    pinfo->min_d_gain = 64;
    pinfo->max_d_gain = 0x7FF*64/128;
    pinfo->curr_d_gain = 0;

    if ((ret = sensor_init_i2c_driver()) < 0)
        goto construct_err;

    if (g_psensor->interface != IF_MIPI){
        if ((ret = init()) < 0)
            goto construct_err;
        isp_info("AR0330 in_w=%d, in_h=%d, out_w=%d, out_h=%d\n", g_psensor->img_win.width, g_psensor->img_win.height,
        g_psensor->out_win.width, g_psensor->out_win.height);
    }
    return 0;

construct_err:
    ar0330_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init ar0330_init(void)
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

    if ((ret = ar0330_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit ar0330_exit(void)
{
    isp_unregister_sensor(g_psensor);
    ar0330_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(ar0330_init);
module_exit(ar0330_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor AR0330");
MODULE_LICENSE("GPL");
