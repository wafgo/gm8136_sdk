/**
 * @file sen_ar0835.c
 * Aptina AR0835 sensor driver
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

#define PFX "sen_ar0835"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     3264
#define DEFAULT_IMAGE_HEIGHT    2448
#define DEFAULT_XCLK            27000000
#define DEFAULT_CH_NUM       	4

static ushort ch_num = DEFAULT_CH_NUM;
module_param(ch_num, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_num, "channel number");

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
#define SENSOR_NAME         "AR0835"
#define SENSOR_MAX_WIDTH    3264
#define SENSOR_MAX_HEIGHT   2448
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
} sensor_info_t;

typedef struct sensor_reg {
    u16 addr;
    u16 val;
} sensor_reg_t;

static sensor_reg_t sensor_init_regs[] = {
    //[Initialize for Camera]
    //XMCLK=24000000
    {0x301A, 0x0019}, 	// RESET_REGISTER
    {DELAY_CODE, 300},

    //Default_3P
    {0x3042, 0x0000}, 	// DARK_CONTROL2
    {0x30C0, 0x1810}, 	// CALIB_CONTROL
    {0x30C8, 0x0018}, 	// CALIB_DAC
    {0x30D2, 0x0000}, 	// CRM_CONTROL
    {0x30D6, 0x2200}, 	// COLUMN_CORRECTION2
    {0x30DA, 0x0080}, 	// COLUMN_CORRECTION_CLIP2
    {0x30DC, 0x0080}, 	// COLUMN_CORRECTION_CLIP3
    {0x316A, 0x8800}, 	// DAC_RSTLO_BLCLO
    {0x316C, 0x8200}, 	// DAC_TXLO
    {0x3172, 0x0286}, 	// ANALOG_CONTROL2
    {0x3174, 0x8000}, 	// ANALOG_CONTROL3
    {0x317C, 0xE103}, 	// ANALOG_CONTROL7
    {0x31E0, 0x0741}, 	// PIX_DEF_ID
    {0x3ECC, 0x0056}, 	// DAC_LD_0_1
    {0x3EDE, 0x6664}, 	// DAC_LD_18_19
    {0x3EE0, 0x26D5}, 	// DAC_LD_20_21
    {0x3EE6, 0xB10C}, 	// DAC_LD_26_27
    {0x3EE8, 0x6E79}, 	// DAC_LD_28_29
    {0x3EFE, 0x77CC}, 	// DAC_LD_TXLO
    {0x31E6, 0x0000}, 	// PIX_DEF_ID_2

    //Configure HiSpi 10 bit
    {0x31AE, 0x0304}, 	// SERIAL_FORMAT
    {0x31BE, 0xC003}, 	// MIPI_CONFIG //For VDD_SLVS=1.2v
    {0x31C6, 0x8404},	// Streaming-SP, LSB first.
    {0x0112, 0x0A0A}, 	// CCP_DATA_FORMAT

    //2DDC 2p7
    {0x3F00, 0x0051}, 	// BM_T0
    {0x3F02, 0x00A2}, 	// BM_T1
    {0x3F04, 0x0002}, 	// NOISE_GAIN_THRESHOLD0
    {0x3F06, 0x0004}, 	// NOISE_GAIN_THRESHOLD1
    {0x3F08, 0x0008}, 	// NOISE_GAIN_THRESHOLD2
    {0x3F0A, 0x0702}, 	// NOISE_FLOOR10
    {0x3F0C, 0x0707}, 	// NOISE_FLOOR32
    {0x3F10, 0x0505}, 	// SINGLE_K_FACTOR0
    {0x3F12, 0x0303}, 	// SINGLE_K_FACTOR1
    {0x3F14, 0x0101}, 	// SINGLE_K_FACTOR2
    {0x3F16, 0x0103}, 	// CROSSFACTOR0
    {0x3F18, 0x0114}, 	// CROSSFACTOR1
    {0x3F1A, 0x0112}, 	// CROSSFACTOR2
    {0x3F40, 0x1D1D}, 	// COUPLE_K_FACTOR0
    {0x3F42, 0x1D1D}, 	// COUPLE_K_FACTOR1
    {0x3F44, 0x1D1D}, 	// COUPLE_K_FACTOR2
    {0x3F1C, 0x0014}, 	// SINGLE_MAXFACTOR
    {0x3F1E, 0x001E}, 	// NOISE_COEF
    {0x301A, 0x0218}, 	// RESET_REGISTER

    //Sequencer_v13p13
    {0x3D00, 0x0471},
    {0x3D02, 0xC9FF},
    {0x3D04, 0xFFFF},
    {0x3D06, 0xFFFF},
    {0x3D08, 0x6F40},
    {0x3D0A, 0x140E},
    {0x3D0C, 0x23C2},
    {0x3D0E, 0x4120},
    {0x3D10, 0x3054},
    {0x3D12, 0x8042},
    {0x3D14, 0x00C0},
    {0x3D16, 0x8357},
    {0x3D18, 0x8464},
    {0x3D1A, 0x6455},
    {0x3D1C, 0x8023},
    {0x3D1E, 0x0065},
    {0x3D20, 0x6582},
    {0x3D22, 0x00C0},
    {0x3D24, 0x6E80},
    {0x3D26, 0x5051},
    {0x3D28, 0x8342},
    {0x3D2A, 0x8358},
    {0x3D2C, 0x6E80},
    {0x3D2E, 0x5F87},
    {0x3D30, 0x6382},
    {0x3D32, 0x5B82},
    {0x3D34, 0x5980},
    {0x3D36, 0x5A5E},
    {0x3D38, 0xBD59},
    {0x3D3A, 0x599D},
    {0x3D3C, 0x6C80},
    {0x3D3E, 0x6DA3},
    {0x3D40, 0x5080},
    {0x3D42, 0x5182},
    {0x3D44, 0x5880},
    {0x3D46, 0x6683},
    {0x3D48, 0x6464},
    {0x3D4A, 0x8030},
    {0x3D4C, 0x50DC},
    {0x3D4E, 0x6A83},
    {0x3D50, 0x6BAA},
    {0x3D52, 0x3094},
    {0x3D54, 0x6784},
    {0x3D56, 0x6565},
    {0x3D58, 0x814D},
    {0x3D5A, 0x686A},
    {0x3D5C, 0xAC06},
    {0x3D5E, 0x088D},
    {0x3D60, 0x4596},
    {0x3D62, 0x4585},
    {0x3D64, 0x6A83},
    {0x3D66, 0x6B06},
    {0x3D68, 0x08A9},
    {0x3D6A, 0x3090},
    {0x3D6C, 0x6764},
    {0x3D6E, 0x6489},
    {0x3D70, 0x6565},
    {0x3D72, 0x8158},
    {0x3D74, 0x8810},
    {0x3D76, 0xC0B1},
    {0x3D78, 0x5E96},
    {0x3D7A, 0x5382},
    {0x3D7C, 0x5E52},
    {0x3D7E, 0x6680},
    {0x3D80, 0x5883},
    {0x3D82, 0x6464},
    {0x3D84, 0x805B},
    {0x3D86, 0x815A},
    {0x3D88, 0x1D0C},
    {0x3D8A, 0x8055},
    {0x3D8C, 0x3060},
    {0x3D8E, 0x4182},
    {0x3D90, 0x42B2},
    {0x3D92, 0x4280},
    {0x3D94, 0x4081},
    {0x3D96, 0x4089},
    {0x3D98, 0x06C0},
    {0x3D9A, 0x4180},
    {0x3D9C, 0x4285},
    {0x3D9E, 0x4483},
    {0x3DA0, 0x4382},
    {0x3DA2, 0x6A83},
    {0x3DA4, 0x6B8D},
    {0x3DA6, 0x4383},
    {0x3DA8, 0x4481},
    {0x3DAA, 0x4185},
    {0x3DAC, 0x06C0},
    {0x3DAE, 0x8C30},
    {0x3DB0, 0xA467},
    {0x3DB2, 0x8142},
    {0x3DB4, 0x8265},
    {0x3DB6, 0x6581},
    {0x3DB8, 0x696A},
    {0x3DBA, 0x9640},
    {0x3DBC, 0x8240},
    {0x3DBE, 0x8906},
    {0x3DC0, 0xC041},
    {0x3DC2, 0x8042},
    {0x3DC4, 0x8544},
    {0x3DC6, 0x8343},
    {0x3DC8, 0x9243},
    {0x3DCA, 0x8344},
    {0x3DCC, 0x8541},
    {0x3DCE, 0x8106},
    {0x3DD0, 0xC081},
    {0x3DD2, 0x6A83},
    {0x3DD4, 0x6B82},
    {0x3DD6, 0x42A0},
    {0x3DD8, 0x4084},
    {0x3DDA, 0x38A8},
    {0x3DDC, 0x3300},
    {0x3DDE, 0x2830},
    {0x3DE0, 0x7000},
    {0x3DE2, 0x6F40},
    {0x3DE4, 0x140E},
    {0x3DE6, 0x23C2},
    {0x3DE8, 0x4182},
    {0x3DEA, 0x4200},
    {0x3DEC, 0xC05D},
    {0x3DEE, 0x805A},
    {0x3DF0, 0x8057},
    {0x3DF2, 0x8464},
    {0x3DF4, 0x8055},
    {0x3DF6, 0x8664},
    {0x3DF8, 0x8065},
    {0x3DFA, 0x8865},
    {0x3DFC, 0x8254},
    {0x3DFE, 0x8058},
    {0x3E00, 0x8000},
    {0x3E02, 0xC086},
    {0x3E04, 0x4282},
    {0x3E06, 0x1030},
    {0x3E08, 0x9C5C},
    {0x3E0A, 0x806E},
    {0x3E0C, 0x865B},
    {0x3E0E, 0x8063},
    {0x3E10, 0x9E59},
    {0x3E12, 0x8C5E},
    {0x3E14, 0x8A6C},
    {0x3E16, 0x806D},
    {0x3E18, 0x815F},
    {0x3E1A, 0x6061},
    {0x3E1C, 0x8810},
    {0x3E1E, 0x3066},
    {0x3E20, 0x836E},
    {0x3E22, 0x8064},
    {0x3E24, 0x8764},
    {0x3E26, 0x3050},
    {0x3E28, 0xD36A},
    {0x3E2A, 0x6BAD},
    {0x3E2C, 0x3094},
    {0x3E2E, 0x6784},
    {0x3E30, 0x6582},
    {0x3E32, 0x4D83},
    {0x3E34, 0x6530},
    {0x3E36, 0x50A7},
    {0x3E38, 0x4306},
    {0x3E3A, 0x008D},
    {0x3E3C, 0x459A},
    {0x3E3E, 0x6A6B},
    {0x3E40, 0x4585},
    {0x3E42, 0x0600},
    {0x3E44, 0x8143},
    {0x3E46, 0x8A6F},
    {0x3E48, 0x9630},
    {0x3E4A, 0x9067},
    {0x3E4C, 0x6488},
    {0x3E4E, 0x6480},
    {0x3E50, 0x6582},
    {0x3E52, 0x10C0},
    {0x3E54, 0x8465},
    {0x3E56, 0xEF10},
    {0x3E58, 0xC066},
    {0x3E5A, 0x8564},
    {0x3E5C, 0x8117},
    {0x3E5E, 0x0080},
    {0x3E60, 0x200D},
    {0x3E62, 0x8018},
    {0x3E64, 0x0C80},
    {0x3E66, 0x6430},
    {0x3E68, 0x6041},
    {0x3E6A, 0x8242},
    {0x3E6C, 0xB242},
    {0x3E6E, 0x8040},
    {0x3E70, 0x8240},
    {0x3E72, 0x4C45},
    {0x3E74, 0x926A},
    {0x3E76, 0x6B9B},
    {0x3E78, 0x4581},
    {0x3E7A, 0x4C40},
    {0x3E7C, 0x8C30},
    {0x3E7E, 0xA467},
    {0x3E80, 0x8565},
    {0x3E82, 0x8765},
    {0x3E84, 0x3060},
    {0x3E86, 0xD36A},
    {0x3E88, 0x6BAC},
    {0x3E8A, 0x6C32},
    {0x3E8C, 0xA880},
    {0x3E8E, 0x2830},
    {0x3E90, 0x7000},
    {0x3E92, 0x8040},
    {0x3E94, 0x4CBD},
    {0x3E96, 0x000E},
    {0x3E98, 0xBE44},
    {0x3E9A, 0x8844},
    {0x3E9C, 0xBC78},
    {0x3E9E, 0x0900},
    {0x3EA0, 0x8904},
    {0x3EA2, 0x8080},
    {0x3EA4, 0x0240},
    {0x3EA6, 0x8609},
    {0x3EA8, 0x008E},
    {0x3EAA, 0x0900},
    {0x3EAC, 0x8002},
    {0x3EAE, 0x4080},
    {0x3EB0, 0x0480},
    {0x3EB2, 0x887D},
    {0x3EB4, 0xA086},
    {0x3EB6, 0x0900},
    {0x3EB8, 0x877A},
    {0x3EBA, 0x000E},
    {0x3EBC, 0xC379},
    {0x3EBE, 0x4C40},
    {0x3EC0, 0xBF70},
    {0x3EC2, 0x0000},
    {0x3EC4, 0x0000},
    {0x3EC6, 0x0000},
    {0x3EC8, 0x0000},
    {0x3ECA, 0x0000},

    //Setting_V7
    {0x301A, 0x0218}, 	// RESET_REGISTER
    {0x30D4, 0x3030}, 	// COLUMN_CORRECTION
    {0x3EE4, 0x3548}, 	// DAC_LD_24_25
    {0x3F20, 0x0209}, 	// GTH_CONTROL
    {0x30EE, 0x0340}, 	// DARK_CONTROL3
    {0x3F2C, 0x2210}, 	// GTH_THRES_RTN
    {0x3180, 0xB080}, 	// FINEDIGCORR_CONTROL
    {0x3F38, 0x44A8}, 	// GTH_THRES_FDOC
    {0x3ED4, 0x6ACC}, 	// DAC_LD_8_9
    {0x3ED8, 0x7488}, 	// DAC_LD_12_13
    {0x316E, 0x8100}, 	// DAC_ECL
    {0x3EFA, 0xA656}, 	// DAC_LD_ECL
    {0x3ED0, 0xC86A}, 	// DAC_LD_4_5
    {0x3ED2, 0x66A6}, 	// DAC_LD_6_7
    {0x3EEA, 0x08B9}, 	// DAC_LD_30_31
    {0x3EDA, 0x77CB}, 	// DAC_LD_14_15
};
#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_init_regs_8M[] = {
    //[8M 3264x2448 15fps]
    //337.5Mbps/lane
    {0x301A, 0x0218}, 	// RESET_REGISTER
    {DELAY_CODE, 300},
    {0x31AE, 0x0304}, 	// SERIAL_FORMAT
    {0x3F3A, 0xFF03}, 	// ANALOG_CONTROL8
    {0x0300, 0x0006}, 	// VT_PIX_CLK_DIV
    {0x0302, 0x0001}, 	// VT_SYS_CLK_DIV
    {0x0304, 0x0002}, 	// PRE_PLL_CLK_DIV
    {0x0306, 0x0032},	// PLL_MULTIPLIER
    {0x0308, 0x000A}, 	// OP_PIX_CLK_DIV
    {0x030A, 0x0002},	// OP_SYS_CLK_DIV
    {0x3016, 0x0111}, 	// ROW_SPEED
    {0x3064, 0x5800}, 	// SMIA_TEST
    {0x3002, 0x0008}, 	// Y_ADDR_START_
//    {0x3006, 0x0997}, 	// Y_ADDR_END_
    {0x3006, 0x0999}, 	// Y_ADDR_END_
    {0x3004, 0x0008}, 	// X_ADDR_START_
    {0x3008, 0x0CC7}, 	// X_ADDR_END_
    {0x3040, 0x4041}, 	// READ_MODE
    {0x300C, 0x165D}, 	// LINE_LENGTH_PCK_
    {0x300A, 0x0A3C}, 	// FRAME_LENGTH_LINE_
//    {0x3012, 0x0A3C}, 	// COARSE_INTEGRATION_TIME_
    {0x0400, 0x0000}, 	// SCALING_MODE
    {0x0402, 0x0000}, 	// SPATIAL_SAMPLING
    {0x0404, 0x0010}, 	// SCALE_M
    {0x306E, 0x9080}, 	// DATA_PATH_SELECT
    {0x0408, 0x1010}, 	// SECOND_RESIDUAL
    {0x040A, 0x0210}, 	// SECOND_CROP
    {0x034C, 0x0CC0}, 	// X_OUTPUT_SIZE
//    {0x034E, 0x0990}, 	// Y_OUTPUT_SIZE
    {0x034E, 0x0992}, 	// Y_OUTPUT_SIZE
    {0x3120, 0x0031}, 	// GAIN_DITHER_CONTROL
    {0x301A, 0x001C}, 	// RESET_REGISTER
    //STATE = Master Clock, 225000000
};
#define NUM_OF_INIT_REGS_8M (sizeof(sensor_init_regs_8M) / sizeof(sensor_reg_t))
static sensor_reg_t sensor_init_regs_5M[] = {
    //[27M:5M 2560x1920 from 3264x2448 xy:16/20]
    //2611x1958
    //675Mbps/lane
    //XMCLK=27000000
    {0x301A, 0x0218}, 	// RESET_REGISTER
    {DELAY_CODE, 300},
    {0x31AE, 0x0304}, 	// SERIAL_FORMAT
    {0x3F3A, 0xFF03}, 	// ANALOG_CONTROL8
    {0x0300, 0x0003}, 	// VT_PIX_CLK_DIV
    {0x0302, 0x0001}, 	// VT_SYS_CLK_DIV
    {0x0304, 0x0003}, 	// PRE_PLL_CLK_DIV
//    {0x0306, 0x0032}, 	// PLL_MULTIPLIER
    {0x0306, 0x002E}, 	// PLL_MULTIPLIER
    {0x0308, 0x000A}, 	// OP_PIX_CLK_DIV
    {0x030A, 0x0001}, 	// OP_SYS_CLK_DIV
    {0x3016, 0x0111}, 	// ROW_SPEED
    {0x3064, 0x5800}, 	// SMIA_TEST

    {0x3002, 0x0008}, 		// Y_ADDR_START_
    {0x3006, 0x0997}, 	// Y_ADDR_END_
    {0x3004, 0x0008},		// X_ADDR_START_
    {0x3008, 0x0CC7},		// X_ADDR_END_
    {0x3040, 0x4041}, 	// READ_MODE
    {0x300C, 4540},		// LINE_LENGTH_PCK_
//    {0x300A, 0x0EEE},	 	// FRAME_LENGTH_LINE_
//    {0x3012, 0x0EEE}, 	// COARSE_INTEGRATION_TIME_
    {0x300A, 2532},	 	// FRAME_LENGTH_LINE_
//    {0x3012, 2532}, 	// COARSE_INTEGRATION_TIME_
    {0x0400, 0x0002}, 	// SCALING_MODE
    {0x0402, 0x0000}, 	// SPATIAL_SAMPLING
    {0x0404, 0x0014},	// SCALE_M
    {0x306E, 0x9090}, 	// DATA_PATH_SELECT
    {0x0408, 0x0C0E},	// SECOND_RESIDUAL
    {0x040A, 0x018C}, 	// SECOND_CROP
    {0x034C, 0x0A00}, 	// X_OUTPUT_SIZE
    {0x034E, 0x0780},	 	// Y_OUTPUT_SIZE
    {0x3120, 0x0031}, 	// GAIN_DITHER_CONTROL
    {0x301A, 0x021C}, 	// RESET_REGISTER
    //STATE = Master Clock, 450000000
};
#define NUM_OF_INIT_REGS_5M (sizeof(sensor_init_regs_5M) / sizeof(sensor_reg_t))


static sensor_reg_t sensor_init_regs_4M[] = {
    //[27M:4M 2688x1520 from 3264x1836 xy:16/19]
    //2748x1546
    //675Mbps/lane
    //XMCLK=27000000
    {0x301A, 0x0218}, 	// RESET_REGISTER
    {DELAY_CODE, 300},
    {0x31AE, 0x0304}, 	// SERIAL_FORMAT
    {0x3F3A, 0xFF03}, 	// ANALOG_CONTROL8
    {0x0300, 0x0003}, 	// VT_PIX_CLK_DIV
    {0x0302, 0x0001}, 	// VT_SYS_CLK_DIV
    {0x0304, 0x0003}, 	// PRE_PLL_CLK_DIV
//    {0x0306, 0x0032}, 	// PLL_MULTIPLIER
    {0x0306, 0x002D}, 	// PLL_MULTIPLIER
    {0x0308, 0x000A}, 	// OP_PIX_CLK_DIV
    {0x030A, 0x0001}, 	// OP_SYS_CLK_DIV
    {0x3016, 0x0111}, 	// ROW_SPEED
    {0x3064, 0x5800}, 	// SMIA_TEST

    {0x3002, 0x013A},		// Y_ADDR_START_
    {0x3006, 0x0865}, 	// Y_ADDR_END_
    {0x3004, 0x0008},		// X_ADDR_START_
    {0x3008, 0x0CC7},		// X_ADDR_END_
    {0x3040, 0x4041}, 	// READ_MODE
    {0x300C, 4760},		// LINE_LENGTH_PCK_
//    {0x300A, 0x0B70},	// FRAME_LENGTH_LINE_
//    {0x3012, 0x0B70}, 	// COARSE_INTEGRATION_TIME_
    {0x300A, 1890},	// FRAME_LENGTH_LINE_
//    {0x3012, 1890}, 	// COARSE_INTEGRATION_TIME_
    {0x0400, 0x0002}, 	// SCALING_MODE
    {0x0402, 0x0000}, 	// SPATIAL_SAMPLING
    {0x0404, 0x0013},	// SCALE_M
    {0x306E, 0x9090}, 	// DATA_PATH_SELECT
    {0x0408, 0x0B0C},	// SECOND_RESIDUAL
    {0x040A, 0x018C}, 	// SECOND_CROP
    {0x034C, 0x0A80},		// X_OUTPUT_SIZE
    {0x034E, 0x05F0}, 	// Y_OUTPUT_SIZE
    {0x3120, 0x0031}, 	// GAIN_DITHER_CONTROL
    {0x301A, 0x021C}, 	// RESET_REGISTER
    //STATE = Master Clock, 450000000
};
#define NUM_OF_INIT_REGS_4M (sizeof(sensor_init_regs_4M) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_init_regs_3M[] = {
    //[27M:3M 2304x1296 from 3264x1836 xy:16/22]
    //2373x1335
    //675Mbps/lane
    //XMCLK=27000000
    {0x301A, 0x0218}, 	// RESET_REGISTER
    {DELAY_CODE, 300},
    {0x31AE, 0x0304}, 	// SERIAL_FORMAT
    {0x3F3A, 0xFF03}, 	// ANALOG_CONTROL8
    {0x0300, 0x0003}, 	// VT_PIX_CLK_DIV
    {0x0302, 0x0001}, 	// VT_SYS_CLK_DIV
    {0x0304, 0x0002}, 	// PRE_PLL_CLK_DIV
    {0x0306, 0x0023}, 	// PLL_MULTIPLIER
    {0x0308, 0x000A}, 	// OP_PIX_CLK_DIV
    {0x030A, 0x0001}, 	// OP_SYS_CLK_DIV
    {0x3016, 0x0111}, 	// ROW_SPEED
    {0x3064, 0x5800}, 	// SMIA_TEST

    {0x3002, 0x013A},		// Y_ADDR_START_
    {0x3006, 0x0865}, 	// Y_ADDR_END_
    {0x3004, 0x0008},		// X_ADDR_START_
    {0x3008, 0x0CC7},		// X_ADDR_END_
    {0x3040, 0x4041}, 	// READ_MODE
    {0x300C, 4120}, 	// LINE_LENGTH_PCK_
    {0x300A, 1910},	// FRAME_LENGTH_LINE_
//    {0x3012, 1910}, 	// COARSE_INTEGRATION_TIME_
    {0x0400, 0x0002}, 	// SCALING_MODE
    {0x0402, 0x0000}, 	// SPATIAL_SAMPLING
    {0x0404, 0x0016},	// SCALE_M
    {0x306E, 0x9090}, 	// DATA_PATH_SELECT
    {0x0408, 0x080B},	// SECOND_RESIDUAL
    {0x040A, 0x016B}, 	// SECOND_CROP
    {0x034C, 0x0900},		// X_OUTPUT_SIZE
    {0x034E, 0x0510}, 	// Y_OUTPUT_SIZE
    {0x3120, 0x0031}, 	// GAIN_DITHER_CONTROL
    {0x301A, 0x021C}, 	// RESET_REGISTER
    //STATE = Master Clock, 450000000
};
#define NUM_OF_INIT_REGS_3M (sizeof(sensor_init_regs_3M) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_init_regs_3M_1536p[] = {
    //[27M:3M 2048x1536 from 3264x2448 xy:16/25]
    //2088x1566
    //675Mbps/lane
    //XMCLK=27000000
    {0x301A, 0x0218}, 	// RESET_REGISTER
    {DELAY_CODE, 300},
    {0x31AE, 0x0304}, 	// SERIAL_FORMAT
    {0x3F3A, 0xFF03}, 	// ANALOG_CONTROL8
    {0x0300, 0x0003}, 	// VT_PIX_CLK_DIV
    {0x0302, 0x0001}, 	// VT_SYS_CLK_DIV
    {0x0304, 0x0002}, 	// PRE_PLL_CLK_DIV
    {0x0306, 0x002B}, 	// PLL_MULTIPLIER
    {0x0308, 0x000A}, 	// OP_PIX_CLK_DIV
    {0x030A, 0x0001}, 	// OP_SYS_CLK_DIV
    {0x3016, 0x0111}, 	// ROW_SPEED
    {0x3064, 0x5800}, 	// SMIA_TEST

    {0x3002, 0x0008}, 		// Y_ADDR_START_
    {0x3006, 0x0997}, 		// Y_ADDR_END_
    {0x3004, 0x0008},		// X_ADDR_START_
    {0x3008, 0x0CC7},		// X_ADDR_END_
    {0x3040, 0x4041}, 	// READ_MODE
    {0x300C, 3800},	 	// LINE_LENGTH_PCK_
//    {0x300A, 0x0A4C},	 	// FRAME_LENGTH_LINE_
//    {0x3012, 0x0A4C},	 	// COARSE_INTEGRATION_TIME_
    {0x300A, 2546},	 	// FRAME_LENGTH_LINE_
//    {0x3012, 2546},	 	// COARSE_INTEGRATION_TIME_
    {0x0400, 0x0002}, 	// SCALING_MODE
    {0x0402, 0x0000}, 	// SPATIAL_SAMPLING
    {0x0404, 0x0019},	// SCALE_M
    {0x306E, 0x9090}, 	// DATA_PATH_SELECT
    {0x0408, 0x1601},	// SECOND_RESIDUAL
    {0x040A, 0x012A}, 	// SECOND_CROP
    {0x034C, 0x0800}, 		// X_OUTPUT_SIZE
    {0x034E, 0x0600},	 	// Y_OUTPUT_SIZE
    {0x3120, 0x0031}, 	// GAIN_DITHER_CONTROL
    {0x301A, 0x021C}, 	// RESET_REGISTER
    //STATE = Master Clock, 450000000
};
#define NUM_OF_INIT_REGS_3M_1536P (sizeof(sensor_init_regs_3M_1536p) / sizeof(sensor_reg_t))
static sensor_reg_t sensor_init_regs_1080p[] = {      //1920*1080
    //[27M:1080p60 1920x1080 from 3136x1764 xy:16/26  vt=414 op=621]
    //1929x1085
    //621Mbps/lane
    //XMCLK=27000000
    {0x301A, 0x0218},     // RESET_REGISTER
    {DELAY_CODE, 300},
    {0x31AE, 0x0304},     // SERIAL_FORMAT
    {0x3F3A, 0xFF03},     // ANALOG_CONTROL8
    {0x0300, 0x0003},      // VT_PIX_CLK_DIV
    {0x0302, 0x0001},      // VT_SYS_CLK_DIV
    {0x0304, 0x0002},      // PRE_PLL_CLK_DIV
    {0x0306, 0x002E},      // PLL_MULTIPLIER
    {0x0308, 0x000A},     // OP_PIX_CLK_DIV
    {0x030A, 0x0001},     // OP_SYS_CLK_DIV
    {0x3016, 0x0111},      // ROW_SPEED
    {0x3064, 0x5800},      // SMIA_TEST
    {0x3002, 350   },        // Y_ADDR_START_
    {0x3006, 2113  },       // Y_ADDR_END_
    {0x3004, 72    },         // X_ADDR_START_
    {0x3008, 3207  },        // X_ADDR_END_
    {0x3040, 0x4041},      // READ_MODE
    {0x300C, 3800  },        // LINE_LENGTH_PCK_
    {0x300A, 1815  },        // FRAME_LENGTH_LINE_
//    {0x3012, 1815  },        // COARSE_INTEGRATION_TIME_
    {0x0400, 0x0002},      // SCALING_MODE
    {0x0402, 0x0000},      // SPATIAL_SAMPLING
    {0x0404, 0x001A},     // SCALE_M
    {0x306E, 0x9090},      // DATA_PATH_SELECT
    {0x0408, 0x0C11},     // SECOND_RESIDUAL
    {0x040A, 0x0129},     // SECOND_CROP
    {0x034C, 1920  },        // X_OUTPUT_SIZE
    {0x034E, 1080  },        // Y_OUTPUT_SIZE
    {0x3120, 0x0031},      // GAIN_DITHER_CONTROL
    {0x301A, 0x021C},     // RESET_REGISTER
    //STATE = Master Clock, 414000000
};

#define NUM_OF_INIT_REGS_1080P (sizeof(sensor_init_regs_1080p) / sizeof(sensor_reg_t))


typedef struct _gain_set {
    u8 colamp;
    u8 adcg;
    u16 dig_x;
    u32 gain_x;
} gain_set_t;

static const gain_set_t gain_table[] = {
    {0, 0,  128,   64}, {0, 0,  136,   68}, {0, 0,  144,   72},
    {0, 0,  152,   76}, {0, 0,  160,   80}, {0, 0,  168,   84},
    {0, 0,  176,   88}, {0, 0,  184,   92}, {0, 0,  192,   96},
    {0, 0,  200,  100}, {0, 0,  208,  104}, {0, 0,  216,  108},
    {0, 0,  224,  112}, {0, 0,  232,  116}, {0, 0,  240,  120},
    {0, 0,  248,  124}, {1, 0,  128,  128},
    {1, 0,  136,  136}, {1, 0,  144,  144}, {1, 0,  152,  152},
    {1, 0,  160,  160}, {1, 0,  168,  168}, {1, 0,  176,  176},
    {1, 0,  184,  184}, {2, 0,  128,  192}, {2, 0,  133,  199},
    {2, 0,  138,  207}, {2, 0,  144,  216}, {2, 0,  149,  223},
    {2, 0,  154,  231}, {2, 0,  160,  240}, {2, 0,  165,  247},
    {3, 0,  128,  256}, {3, 0,  132,  264}, {3, 0,  136,  272},
    {3, 0,  140,  280}, {3, 0,  144,  288}, {3, 0,  148,  296},
    {3, 0,  152,  304}, {3, 0,  156,  312}, {3, 0,  160,  320},
    {3, 0,  164,  328}, {3, 0,  168,  336}, {3, 0,  172,  344},
    {3, 0,  176,  352}, {3, 0,  180,  360}, {3, 0,  184,  368},
    {3, 0,  188,  376}, {2, 1,  128,  384}, {2, 1,  132,  396},
    {2, 1,  136,  408}, {2, 1,  140,  420}, {2, 1,  144,  432},
    {2, 1,  148,  444}, {2, 1,  152,  456}, {2, 1,  156,  468},
    {2, 1,  160,  480}, {2, 1,  164,  492}, {2, 1,  168,  504},
    {3, 1,  128,  512}, {3, 1,  144,  576}, {3, 1,  160,  640},
    {3, 1,  176,  704}, {3, 1,  192,  768}, {3, 1,  208,  832},
    {3, 1,  224,  896}, {3, 1,  240,  960}, {3, 1,  256, 1024},
    {3, 1,  272, 1088}, {3, 1,  288, 1152}, {3, 1,  304, 1216},
    {3, 1,  320, 1280}, {3, 1,  336, 1344}, {3, 1,  352, 1408},
    {3, 1,  368, 1472}, {3, 1,  384, 1536}, {3, 1,  400, 1600},
    {3, 1,  416, 1664}, {3, 1,  432, 1728}, {3, 1,  448, 1792},
    {3, 1,  464, 1856}, {3, 1,  480, 1920}, {3, 1,  496, 1984},
    {3, 1,  512, 2048}, {3, 1,  528, 2112}, {3, 1,  544, 2176},
    {3, 1,  560, 2240}, {3, 1,  576, 2304}, {3, 1,  592, 2368},
    {3, 1,  608, 2432}, {3, 1,  624, 2496}, {3, 1,  640, 2560},
    {3, 1,  656, 2624}, {3, 1,  672, 2688}, {3, 1,  688, 2752},
    {3, 1,  704, 2816}, {3, 1,  720, 2880}, {3, 1,  736, 2944},
    {3, 1,  752, 3008}, {3, 1,  768, 3072}, {3, 1,  784, 3136},
    {3, 1,  800, 3200}, {3, 1,  816, 3264}, {3, 1,  832, 3328},
    {3, 1,  848, 3392}, {3, 1,  864, 3456}, {3, 1,  880, 3520},
    {3, 1,  896, 3584}, {3, 1,  912, 3648}, {3, 1,  928, 3712},
    {3, 1,  944, 3776}, {3, 1,  960, 3840}, {3, 1,  976, 3904},
    {3, 1,  992, 3968}, {3, 1, 1008, 4032}, {3, 1, 1024, 4096},
    {3, 1, 1040, 4160}, {3, 1, 1056, 4224}, {3, 1, 1072, 4288},
    {3, 1, 1088, 4352}, {3, 1, 1104, 4416}, {3, 1, 1120, 4480},
    {3, 1, 1136, 4544}, {3, 1, 1152, 4608}, {3, 1, 1168, 4672},
    {3, 1, 1184, 4736}, {3, 1, 1200, 4800}, {3, 1, 1216, 4864},
    {3, 1, 1232, 4928}, {3, 1, 1248, 4992}, {3, 1, 1264, 5056},
    {3, 1, 1280, 5120}, {3, 1, 1296, 5184}, {3, 1, 1312, 5248},
    {3, 1, 1328, 5312}, {3, 1, 1344, 5376}, {3, 1, 1360, 5440},
    {3, 1, 1376, 5504}, {3, 1, 1392, 5568}, {3, 1, 1408, 5632},
    {3, 1, 1424, 5696}, {3, 1, 1440, 5760}, {3, 1, 1456, 5824},
    {3, 1, 1472, 5888}, {3, 1, 1488, 5952}, {3, 1, 1504, 6016},
    {3, 1, 1520, 6080}, {3, 1, 1536, 6144}, {3, 1, 1552, 6208},
    {3, 1, 1568, 6272}, {3, 1, 1584, 6336}, {3, 1, 1600, 6400},
    {3, 1, 1616, 6464}, {3, 1, 1632, 6528}, {3, 1, 1648, 6592},
    {3, 1, 1664, 6656}, {3, 1, 1680, 6720}, {3, 1, 1696, 6784},
    {3, 1, 1712, 6848}, {3, 1, 1728, 6912}, {3, 1, 1744, 6976},
    {3, 1, 1760, 7040}, {3, 1, 1776, 7104}, {3, 1, 1792, 7168},
    {3, 1, 1808, 7232}, {3, 1, 1824, 7296}, {3, 1, 1840, 7360},
    {3, 1, 1856, 7424}, {3, 1, 1872, 7488}, {3, 1, 1888, 7552},
    {3, 1, 1904, 7616}, {3, 1, 1920, 7680}, {3, 1, 1936, 7744},
    {3, 1, 1952, 7808}, {3, 1, 1968, 7872}, {3, 1, 1984, 7936},
    {3, 1, 2000, 8000}, {3, 1, 2016, 8064}, {3, 1, 2032, 8128},
    {3, 1, 2047, 8188},
};
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

#define MIN(x,y) ((x)<(y) ? (x) : (y))
#define MAX(x,y) ((x)>(y) ? (x) : (y))

//============================================================================
// I2C
//============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            (0x6C >> 1)
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
    u32 pclk;

    read_reg(0x0300, &vt_pix_clk_div);
    read_reg(0x0302, &vt_sys_clk_div);
    read_reg(0x0304, &pre_pll_clk_div);
    read_reg(0x0306, &pll_multiplier);

    pclk = xclk * pll_multiplier / (pre_pll_clk_div * vt_sys_clk_div * vt_pix_clk_div);

    pclk *= 2;
    isp_info("pclk(%d) XCLK(%d)\n", pclk, xclk);
    return pclk;
}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 num_of_pclk;
    u32 line_length_pck;
    g_psensor->pclk = get_pclk(g_psensor->xclk);

    read_reg(0X300C, &line_length_pck);
//    num_of_pclk = line_length_pck * 2;
    num_of_pclk = line_length_pck;

    //720p: pinfo->t_row = num_of_pclk * 1000000 / g_psensor->pclk;
    //pinfo->t_row = num_of_pclk * 2 * 1000000 / g_psensor->pclk;
    // The t_row need to be consided fractional digit and round-off
    pinfo->t_row = num_of_pclk * 10000 / (g_psensor->pclk/1000) ;

    printk("t_row=%d pclk=%d\n",pinfo->t_row,g_psensor->pclk);
}

static void adjust_vblk(int fps)
{
    int frame_h, max_fps;

    if (g_psensor->img_win.width <= 1920 && g_psensor->img_win.height <= 1080){
        frame_h = 1815;
        max_fps =60;
    }
    else if (g_psensor->img_win.width <= 2304 && g_psensor->img_win.height <= 1296){
        frame_h = 1910;
        max_fps = 40;
    }
    else if (g_psensor->img_win.width <= 2048 && g_psensor->img_win.height <= 1536){
        frame_h = 2546;
        max_fps = 40;
    }
    else if (g_psensor->img_win.width <= 2688 && g_psensor->img_win.height <= 1520){
        frame_h = 1890;
        max_fps = 30;
    }
    else if (g_psensor->img_win.width <= 2560 && g_psensor->img_win.height <= 1920){
            frame_h = 2532;
        max_fps =24;
    }
    else{
        frame_h = 0xa3c;
        max_fps = 15;
    }
    if (fps > max_fps)
        fps = max_fps;

    frame_h = frame_h * max_fps / fps;

    g_psensor->fps = fps;
    write_reg(0x300A, (u32)frame_h);

    isp_info("fps=%d\n", g_psensor->fps);
    calculate_t_row();
}

static void update_bayer_type(void);
static int set_mirror(int enable);
static int set_flip(int enable);
static int set_exp(u32 exp);
static int set_gain(u32 gain);

static int set_size(u32 width, u32 height)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    int i, reg;
    sensor_reg_t* pInitTable;
    u32 InitTable_Num;

    // check size
    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("Exceed maximum sensor resolution!\n");
        return -1;
    }

    if (width <= 1920 && height <= 1080){
        pInitTable=sensor_init_regs_1080p;
        InitTable_Num=NUM_OF_INIT_REGS_1080P;
    }
    else if (width <= 2048 && height <= 1536){
        pInitTable=sensor_init_regs_3M_1536p;
        InitTable_Num=NUM_OF_INIT_REGS_3M_1536P;
    }
    else if (width <= 2304 && height <= 1296){
        pInitTable=sensor_init_regs_3M;
        InitTable_Num=NUM_OF_INIT_REGS_3M;
    }
    else if (width <= 2688 && height <= 1520){
        pInitTable=sensor_init_regs_4M;
        InitTable_Num=NUM_OF_INIT_REGS_4M;
    }
    else if (width <= 2560 &&height <= 1920){
        pInitTable=sensor_init_regs_5M;
        InitTable_Num=NUM_OF_INIT_REGS_5M;
    }
    else{
        pInitTable=sensor_init_regs_8M;
        InitTable_Num=NUM_OF_INIT_REGS_8M;
    }

    for (i=0; i<InitTable_Num; i++) {
        if(pInitTable[i].addr == DELAY_CODE){
//            mdelay(pInitTable[i].val);
        }
        else if(write_reg(pInitTable[i].addr, pInitTable[i].val) < 0) {
            isp_err("%s size init fail!!\n", SENSOR_NAME);
            return -EINVAL;
        }
        if(pInitTable[i].addr == 0x301a && (!(pInitTable[i].val & 0x04)) ){
            while(1){
                read_reg(0x303c, &reg);
                if (reg & 0x02)
                    break;
                mdelay(10);
            }
            isp_info("sensor is standby\n");
        }
    }

    set_mirror(pinfo->mirror);
    set_flip(pinfo->flip);
    update_bayer_type();


    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

//    g_psensor->pclk = get_pclk(g_psensor->xclk);
//    calculate_t_row();
    adjust_vblk(g_psensor->fps);

    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    g_psensor->out_win.width = width;
    g_psensor->out_win.height = height;

    set_exp(g_psensor->curr_exp);
    set_gain(g_psensor->curr_gain);

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
        read_reg(0x305E, &ang_x_1);
        dig_x = (ang_x_1 & 0xFFE0) >> 5;
        ana_coarse = (ang_x_1 & 0x03);
        ana_fine = (ang_x_1 & 0x04) >> 2;

        for (i=0; i<NUM_OF_GAINSET; i++) {
            if ((gain_table[i].colamp == ana_coarse)&&(gain_table[i].adcg == ana_fine)
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

    ang_reg_1 = (gain_table[i].dig_x << 5) | (gain_table[i].colamp & 0x03) | ((gain_table[i].adcg & 0x01) << 2);


    write_reg(0x305E, ang_reg_1);


    g_psensor->curr_gain = (gain_table[i].gain_x);
//    printk("gain=%d\n", g_psensor->curr_gain);

    return ret;
}

static void update_bayer_type(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (pinfo->flip){
        if (pinfo->mirror)
            g_psensor->bayer_type = BAYER_GB;
        else
            g_psensor->bayer_type = BAYER_BG;
    }
    else{
        if (pinfo->mirror)
            g_psensor->bayer_type = BAYER_RG;
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
    sensor_reg_t* pInitTable;
    u32 InitTable_Num;

    if (pinfo->is_init)
        return 0;

    pInitTable=sensor_init_regs;
    InitTable_Num=NUM_OF_INIT_REGS;

    for (i=0; i<InitTable_Num; i++) {
        if(pInitTable[i].addr == DELAY_CODE){
            mdelay(pInitTable[i].val);
        }
        else if(write_reg(pInitTable[i].addr, pInitTable[i].val) < 0) {
            isp_err("%s init fail!!\n", SENSOR_NAME);
            return -EINVAL;
        }
    }

    if (ch_num == 2){
        write_reg(0x31AE, 0x0302);
    }
    isp_info("ch num = %d\n", ch_num);

    pinfo->mirror = mirror;
    pinfo->flip = flip;

    // set image size
    ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height);
    if (ret == 0)
        pinfo->is_init = 1;

    // update sensor information
//    adjust_vblk(g_psensor->fps);

//    g_psensor->pclk = get_pclk(g_psensor->xclk);

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
static void ar0835_deconstruct(void)
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

static int ar0835_construct(u32 xclk, u16 width, u16 height)
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
    g_psensor->gain_latency = 1;

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
    g_psensor->interface = IF_HiSPi;
    g_psensor->num_of_lane = ch_num;
    g_psensor->protocol = Streaming_SP;

    if ((ret = sensor_init_i2c_driver()) < 0)
        goto construct_err;

    if ((ret = init()) < 0)
        goto construct_err;
    isp_info("AR0835 in_w=%d, in_h=%d, out_w=%d, out_h=%d\n", g_psensor->img_win.width, g_psensor->img_win.height,
    g_psensor->out_win.width, g_psensor->out_win.height);

    return 0;

construct_err:
    ar0835_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init ar0835_init(void)
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

    if ((ret = ar0835_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit ar0835_exit(void)
{
    isp_unregister_sensor(g_psensor);
    ar0835_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(ar0835_init);
module_exit(ar0835_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor AR0835");
MODULE_LICENSE("GPL");
