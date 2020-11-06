/**
 * @file sen_ov16820.c
 * OmniVision ov16820 sensor driver
 *
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.6 $
 * $Date: 2014/03/07 03:30:58 $
 *
 * ChangeLog:
 *  $Log: sen_ov16820.c,v $
 *  Revision 1.6  2014/03/07 03:30:58  jtwang
 *  add 3M pixels resolution
 *
 *  Revision 1.5  2013/12/10 09:19:07  jtwang
 *  *** empty log message ***
 *
 *  Revision 1.3  2013/11/20 02:14:00  jtwang
 *  add serial interface sensor modules
 *
 *  Revision 1.1  2013/08/20 07:04:52  kl_tang
 *  *** empty log message ***
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

#define PFX "sen_ov16820"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1920    
#define DEFAULT_IMAGE_HEIGHT    1080
#define DEFAULT_XCLK            27000000
#define DEFAULT_INTERFACE   IF_MIPI
#define DEFAULT_CH_NUM       4


static ushort interface = DEFAULT_INTERFACE;
module_param(interface, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(interface, "interface type");

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

static uint fps = 18;       //for 4096x2048 test
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "frame per second");

//=============================================================================
// global
//=============================================================================
#define SENSOR_NAME         "ov16820"
#define SENSOR_MAX_WIDTH    4608
#define SENSOR_MAX_HEIGHT   3456

#define FPGA
#define DELAY_CODE          0xffff


static sensor_dev_t *g_psensor = NULL;

typedef struct sensor_info {
    bool is_init;
    u32 img_w;
    u32 img_h;
    int mirror;
    int flip;
    u32 vts;
    u32 hts;
    u32 prev_sw_reg;
    u32 t_row;          // unit is 1/10 us
} sensor_info_t;

typedef struct sensor_reg {
    u16 addr;
    u8  val;
} sensor_reg_t;

// Modified from sensor_4608x3456_4LANE_init_regs
static sensor_reg_t sensor_4608x3456_init_regs[] = {
    {0x0100,0x00},       
    {DELAY_CODE,100},  //delay 100 ms
    {0x0103,0x01},
    {DELAY_CODE,100},  //delay 100 ms

    {0x0300,0x02},
    {0x0302,0x64},
    {0x0305,0x04},
    {0x0306,0x00},
    {0x030b,0x02},
    {0x030c,0x24},
    {0x030e,0x00},
    {0x0313,0x02},
    {0x0314,0x14},
    {0x031f,0x00},
    {0x3018,0x72},     //            MIPI 4-LANE = 0x72 , 2LANE=0x32
    {0x3106,0x61},     //   
    {0x3022,0x01},
    {0x3031,0x0a},	//10-bit
    {0x3032,0x80},
    {0x3500,0x00},
    {0x3501,0x06},     //           Exp.  : old 0x45
    {0x3502,0x00},
    {0x3508,0x00},
    {0x3509,0x80},

    {0x3603, 0x00},
    {0x3604, 0x00},
    {0x360a, 0x00},
    {0x360b, 0x02},
    {0x360c, 0x12},
    {0x360d, 0x00},
    {0x3614, 0x77},
    {0x3616, 0x30},
    {0x3631, 0x60},
    {0x3700, 0x30},
    {0x3701, 0x08},
    {0x3702, 0x11},
    {0x3703, 0x20},
    {0x3704, 0x08},
    {0x3705, 0x00},
    {0x3706, 0x84},
    {0x3708, 0x20},
    {0x3709, 0x3c},
    {0x370a, 0x01},
    {0x370b, 0x5d},
    {0x370c, 0x03},
    {0x370e, 0x20},
    {0x370f, 0x05},
    {0x3710, 0x20},
    {0x3711, 0x20},
    {0x3714, 0x31},
    {0x3719, 0x13},
    {0x371b, 0x03},
    {0x371d, 0x03},
    {0x371e, 0x09},
    {0x371f, 0x17},
    {0x3720, 0x0b},
    {0x3721, 0x18},
    {0x3722, 0x0b},
    {0x3723, 0x18},
    {0x3724, 0x04},
    {0x3725, 0x04},
    {0x3726, 0x02},
    {0x3727, 0x02},
    {0x3728, 0x02},
    {0x3729, 0x02},
    {0x372a, 0x25},
    {0x372b, 0x65},
    {0x372c, 0x55},
    {0x372d, 0x65},
    {0x372e, 0x53},
    {0x372f, 0x33},
    {0x3730, 0x33},
    {0x3731, 0x33},
    {0x3732, 0x03},
    {0x3734, 0x10},
    {0x3739, 0x03},
    {0x373a, 0x20},
    {0x373b, 0x0c},
    {0x373c, 0x1c},
    {0x373e, 0x0b},
    {0x373f, 0x80},

    {0x3f08,0x20},

    {0x3800,0x00},
    {0x3801,0x20},
    {0x3802,0x00},
    {0x3803,0x0e},
    {0x3804,0x12},
    {0x3805,0x3f},
    {0x3806,0x0d},
    {0x3807,0x93},
    {0x3808,0x12},
    {0x3809,0x00},
    {0x380a,0x0d},
    {0x380b,0x80},
    {0x380c,0x05},
    {0x380d,0xf8},   //HTS
    {0x380e,0x0d},
    {0x380f,0xa2},   //VTS
    {0x3811,0x0f},
    {0x3813,0x02},
    {0x3814,0x01},
    {0x3815,0x01},
    {0x3820,0x00},
    {0x3821,0x06},
    {0x3829,0x00},
    {0x382a,0x01},
    {0x382b,0x01},
    {0x3830,0x08},

    {0x3f00, 0x02},
    
    {0x4001, 0x83},
    {0x400e, 0x00},
    {0x4011, 0x00},
    {0x4012, 0x00},
    {0x4200, 0x08},
    {0x4302, 0x7f},
    {0x4303, 0xff},
    {0x4304, 0x00},
    {0x4305, 0x00},
    {0x4501, 0x30},
    {0x4603, 0x20},
    {0x4b00, 0x22},
    {0x4903, 0x00},
    {0x5000, 0x7f},
    {0x5001, 0x01},
    {0x5004, 0x00},
    {0x5013, 0x20},
    {0x5051, 0x00},
    {0x5500, 0x01},
    {0x5501, 0x00},
    {0x5502, 0x07},
    {0x5503, 0xff},
    {0x5505, 0x6c},
    {0x5509, 0x02},
    {0x5780, 0xfc},
    {0x5781, 0xff},
    {0x5787, 0x40},
    {0x5788, 0x08},
    {0x578a, 0x02},
    {0x578b, 0x01},
    {0x578c, 0x01},
    {0x578e, 0x02},
    {0x578f, 0x01},
    {0x5790, 0x01},
    {0x5792, 0x00},
    {0x5980, 0x00},
    {0x5981, 0x21},
    {0x5982, 0x00},
    {0x5983, 0x00},
    {0x5984, 0x00},
    {0x5985, 0x00},
    {0x5986, 0x00},
    {0x5987, 0x00},
    {0x5988, 0x00},

    {0x0100,0x01},       
    {DELAY_CODE,100}  //delay 100 ms
};
#define NUM_OF_4608x3456_INIT_REGS (sizeof(sensor_4608x3456_init_regs) / sizeof(sensor_reg_t))

// Modified from sensor_4096x2048_4LANE_init_regs
static sensor_reg_t sensor_4096x2048_init_regs[] = {
    {0x0100,0x00},       
    {DELAY_CODE,100},  //delay 100 ms
    {0x0103,0x01},
    {DELAY_CODE,100},  //delay 100 ms
    {0x0302,0x6c},
    {0x0305,0x04},
    {0x0306,0x00},
    {0x030c,0x6c},
    {0x030e,0x00},
    {0x3018,0x72},     //            MIPI 4-LANE = 0x72 , 2LANE=0x32
    {0x3106,0x61},     //   
    {0x3022,0x01},
    {0x3031,0x0a},	//10-bit
//    {0x3031,0x0c},		//12-bit
    {0x3032,0x80},
    {0x3500,0x00},
    {0x3501,0x06},     //           Exp.  : old 0x45
    {0x3502,0x00},
//    {0x3508,0x0c},
//    {0x3509,0xff},
    {0x3508,0x00},
    {0x3509,0x80},
    {0x3603, 0x00},
    {0x3604, 0x00},
    {0x360a, 0x00},
    {0x360b, 0x02},
    {0x360c, 0x12},
    {0x360d, 0x00},
    {0x3614, 0x77},
    {0x3616, 0x30},
    {0x3631, 0x60},
    {0x3700, 0x30},
    {0x3701, 0x08},
    {0x3702, 0x11},
    {0x3703, 0x20},
    {0x3704, 0x08},
    {0x3705, 0x00},
    {0x3706, 0x84},
    {0x3708, 0x20},
    {0x3709, 0x3c},
    {0x370a, 0x01},
    {0x370b, 0x5d},
    {0x370c, 0x03},
    {0x370e, 0x20},
    {0x370f, 0x05},
    {0x3710, 0x20},
    {0x3711, 0x20},
    {0x3714, 0x31},
    {0x3719, 0x13},
    {0x371b, 0x03},
    {0x371d, 0x03},
    {0x371e, 0x09},
    {0x371f, 0x17},
    {0x3720, 0x0b},
    {0x3721, 0x18},
    {0x3722, 0x0b},
    {0x3723, 0x18},
    {0x3724, 0x04},
    {0x3725, 0x04},
    {0x3726, 0x02},
    {0x3727, 0x02},
    {0x3728, 0x02},
    {0x3729, 0x02},
    {0x372a, 0x25},
    {0x372b, 0x65},
    {0x372c, 0x55},
    {0x372d, 0x65},
    {0x372e, 0x53},
    {0x372f, 0x33},
    {0x3730, 0x33},
    {0x3731, 0x33},
    {0x3732, 0x03},
    {0x3734, 0x10},
    {0x3739, 0x03},
    {0x373a, 0x20},
    {0x373b, 0x0c},
    {0x373c, 0x1c},
    {0x373e, 0x0b},
    {0x373f, 0x80},

    {0x3f08,0x20},
    {0x3800,0x01},
    {0x3801,0x20},
    {0x3802,0x02},
    {0x3803,0xce},
    {0x3804,0x11},
    {0x3805,0x3f},
    {0x3806,0x0a},
    {0x3807,0xd3},
    {0x3808,0x10},
    {0x3809,0x00},
    {0x380a,0x08},
//    {0x380b,0x00},
    {0x380b,0x10},
    {0x380c,0x05},
    {0x380d,0xf8},   //HTS
    // 11 fps
//    {0x380e,0x0d},
//    {0x380f,0xa2},   //VTS
    // 18 fps
    {0x380e,0x08},
    {0x380f,0x72},   //VTS
    {0x3811,0x0f},
    {0x3813,0x02},
    {0x3814,0x01},
    {0x3815,0x01},
    {0x3820,0x00},
    {0x3821,0x06},
    {0x3829,0x00},
    {0x382a,0x01},
    {0x382b,0x01},
    {0x3830,0x08},
    {0x3f00, 0x02},
    
    {0x4001, 0x83},
    {0x400e, 0x00},
    {0x4011, 0x00},
    {0x4012, 0x00},
    {0x4200, 0x08},
    {0x4302, 0x7f},
    {0x4303, 0xff},
    {0x4304, 0x00},
    {0x4305, 0x00},
    {0x4501, 0x30},
    {0x4603, 0x20},
    {0x4b00, 0x22},
    {0x4903, 0x00},
    {0x5000, 0x7f},
    {0x5001, 0x01},
    {0x5004, 0x00},
    {0x5013, 0x20},
    {0x5051, 0x00},
    {0x5500, 0x01},
    {0x5501, 0x00},
    {0x5502, 0x07},
    {0x5503, 0xff},
    {0x5505, 0x6c},
    {0x5509, 0x02},
    {0x5780, 0xfc},
    {0x5781, 0xff},
    {0x5787, 0x40},
    {0x5788, 0x08},
    {0x578a, 0x02},
    {0x578b, 0x01},
    {0x578c, 0x01},
    {0x578e, 0x02},
    {0x578f, 0x01},
    {0x5790, 0x01},
    {0x5792, 0x00},
    {0x5980, 0x00},
    {0x5981, 0x21},
    {0x5982, 0x00},
    {0x5983, 0x00},
    {0x5984, 0x00},
    {0x5985, 0x00},
    {0x5986, 0x00},
    {0x5987, 0x00},
    {0x5988, 0x00},

    {0x0100,0x01},       
    {DELAY_CODE,100}  //delay 100 ms
};
#define NUM_OF_4096x2048_INIT_REGS (sizeof(sensor_4096x2048_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_1536P_4LANE_init_regs[] = {
    {0x0100,0x00},       
    {DELAY_CODE,100},  //delay 100 ms
    {0x0103,0x01},
    {DELAY_CODE,100},  //delay 100 ms
#if 1
    //30fps
    {0x0302,0x86},
    {0x0305,0x04},
    {0x0306,0x00},
    {0x030c,0x30},
#else
    //34fps
    {0x0302,0x9c},
    {0x0305,0x04},
    {0x0306,0x00},
    {0x030c,0x37},
#endif
    {0x030e,0x00},
    {0x3018,0x72},     //            MIPI 4-LANE = 0x72 , 2LANE=0x32
    {0x3106,0x61},     //   
    {0x3022,0x01},
    {0x3031,0x0a},	    //10-bit
//    {0x3031,0x0c},    //12-bit
    {0x3032,0x80},
    {0x3500,0x00},
    {0x3501,0x06},     //           Exp.  : old 0x45
    {0x3502,0x00},
//    {0x3508,0x0c},
//    {0x3509,0xff},
    {0x3508,0x00},
    {0x3509,0x80},
    {0x3603, 0x00},
    {0x3604, 0x00},
    {0x360a, 0x00},
    {0x360b, 0x02},
    {0x360c, 0x12},
    {0x360d, 0x00},
    {0x3614, 0x77},
    {0x3616, 0x30},
    {0x3631, 0x60},
    {0x3700, 0x30},
    {0x3701, 0x08},
    {0x3702, 0x11},
    {0x3703, 0x20},
    {0x3704, 0x08},
    {0x3705, 0x00},
    {0x3706, 0x84},
    {0x3708, 0x20},
    {0x3709, 0x3c},
    {0x370a, 0x01},
    {0x370b, 0x5d},
    {0x370c, 0x03},
    {0x370e, 0x20},
    {0x370f, 0x05},
    {0x3710, 0x20},
    {0x3711, 0x20},
    {0x3714, 0x31},
    {0x3719, 0x13},
    {0x371b, 0x03},
    {0x371d, 0x03},
    {0x371e, 0x09},
    {0x371f, 0x17},
    {0x3720, 0x0b},
    {0x3721, 0x18},
    {0x3722, 0x0b},
    {0x3723, 0x18},
    {0x3724, 0x04},
    {0x3725, 0x04},
    {0x3726, 0x02},
    {0x3727, 0x02},
    {0x3728, 0x02},
    {0x3729, 0x02},
    {0x372a, 0x25},
    {0x372b, 0x65},
    {0x372c, 0x55},
    {0x372d, 0x65},
    {0x372e, 0x53},
    {0x372f, 0x33},
    {0x3730, 0x33},
    {0x3731, 0x33},
    {0x3732, 0x03},
    {0x3734, 0x10},
    {0x3739, 0x03},
    {0x373a, 0x20},
    {0x373b, 0x0c},
    {0x373c, 0x1c},
    {0x373e, 0x0b},
    {0x373f, 0x80},

    {0x3f08,0x20},
    {0x3800,0x01},
    {0x3801,0x20},
    {0x3802,0x02},
    {0x3803,0xce},
    {0x3804,0x11},
    {0x3805,0x3f},
    {0x3806,0x0a},
    {0x3807,0xd3},
#if 0    
    {0x3808,0x08},
    {0x3809,0x00},
    {0x380a,0x06},
//    {0x380b,0x00},
    {0x380b,0x10},
    {0x380c,0x05},
    {0x380d,0xf8},   //HTS
    // 11 fps
//    {0x380e,0x0d},
//    {0x380f,0xa2},   //VTS
    // 18 fps
    {0x380e,0x08},
    {0x380f,0x72},   //VTS
#else
    {0x3808,0x08},
    {0x3809,0x00},
    {0x380a,0x06},
//    {0x380b,0x00},
//    {0x380b,0x10},
    {0x380b,0x01},  //AE statistic fail
    {0x380c,0x05},
    {0x380d,0xf8},   //HTS
    {0x380e,0x06},
    {0x380f,0x22},   //VTS
#endif
    {0x3811,0x0f},
    {0x3813,0x02},
    {0x3814,0x01},
    {0x3815,0x01},
    {0x3820,0x00},
    {0x3821,0x06},
    {0x3829,0x00},
    {0x382a,0x01},
    {0x382b,0x01},
    {0x3830,0x08},
    {0x3f00, 0x02},
    
    {0x4001, 0x83},
    {0x400e, 0x00},
    {0x4011, 0x00},
    {0x4012, 0x00},
    {0x4200, 0x08},
    {0x4302, 0x7f},
    {0x4303, 0xff},
    {0x4304, 0x00},
    {0x4305, 0x00},
    {0x4501, 0x30},
    {0x4603, 0x20},
    {0x4b00, 0x22},
    {0x4903, 0x00},
    {0x5000, 0x7f},
    {0x5001, 0x01},
    {0x5004, 0x00},
    {0x5013, 0x20},
    {0x5051, 0x00},
    {0x5500, 0x01},
    {0x5501, 0x00},
    {0x5502, 0x07},
    {0x5503, 0xff},
    {0x5505, 0x6c},
    {0x5509, 0x02},
    {0x5780, 0xfc},
    {0x5781, 0xff},
    {0x5787, 0x40},
    {0x5788, 0x08},
    {0x578a, 0x02},
    {0x578b, 0x01},
    {0x578c, 0x01},
    {0x578e, 0x02},
    {0x578f, 0x01},
    {0x5790, 0x01},
    {0x5792, 0x00},
    {0x5980, 0x00},
    {0x5981, 0x21},
    {0x5982, 0x00},
    {0x5983, 0x00},
    {0x5984, 0x00},
    {0x5985, 0x00},
    {0x5986, 0x00},
    {0x5987, 0x00},
    {0x5988, 0x00},

    {0x0100,0x01},       
    {DELAY_CODE,100}  //delay 100 ms
};
#define NUM_OF_1536P_4LANE_INIT_REGS (sizeof(sensor_1536P_4LANE_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_1080P_4LANE_init_regs[] = {
    {0x0100,0x00},       
    {DELAY_CODE,100},  //delay 100 ms
    {0x0103,0x01},
    {DELAY_CODE,100},  //delay 100 ms  

    {0x0300,0x02},
    {0x0302,0x6c},      
//    {0x0302,0x34},      
    {0x0305,0x04},
    {0x0306,0x00},
    {0x030b,0x02},
//    {0x030c,0x3d},      
    {0x030c,0x36},  //for 27MHz input      
//    {0x030c,0x3c},      
    {0x030e,0x00},
    {0x0313,0x02},
    {0x0314,0x14},
    {0x031f,0x00},

    {0x3018,0x72},     // 4-LANE = 0x72 , 2LANE=0x32
    {0x3106,0x61},     // 0x01
//    {0x3106,0x21},     // 0x01
    {0x3022,0x01},
    {0x3031,0x0a},
    {0x3032,0x80},
    {0x3500,0x00},
    {0x3501,0x06},    // old 0x45
    {0x3502,0x00},
    {0x3508,0x0c},
    {0x3509,0xff},
    {0x3601,0x88},
    {0x3602,0x00},
    {0x3603,0x05},
    {0x3604,0x02},
    {0x3605,0x50},
    {0x3606,0x00},
    {0x3607,0x2b},
    {0x3608,0x12},
    {0x3609,0x00},
    {0x360a,0x00},
    {0x360b,0x02},
    {0x360c,0x10},
    {0x360d,0x05},
    {0x3612,0x20},
    {0x3613,0x88},
    {0x3614,0x77},
    {0x3615,0xf4},
    {0x3616,0x30},
    {0x3617,0x00},
    {0x3618,0x20},
    {0x3619,0x00},
    {0x361a,0x10},
    {0x361d,0x00},
    {0x361e,0x00},
    {0x3631,0x60},
    {0x364a,0x06},
    {0x3700,0x40},
    {0x3701,0x08},
    {0x3702,0x11},
    {0x3703,0x20},
    {0x3704,0x08},
    {0x3705,0x00},
    {0x3706,0x84},
    {0x3707,0x08},
    {0x3708,0x20},
    {0x3709,0x3c},
    {0x370a,0x01},
    {0x370b,0x5d},
    {0x370c,0x03},
    {0x370e,0x20},
    {0x370f,0x05},
    {0x3710,0x20},
    {0x3711,0x0e},
    {0x3714,0x31},
    {0x3718,0x75},
    {0x3719,0x13},
    {0x371a,0x55},
    {0x371b,0x03},
    {0x371c,0x55},
    {0x371d,0x03},
    {0x371e,0x09},
    {0x371f,0x16},
    {0x3720,0x0b},
    {0x3721,0x18},
    {0x3722,0x0b},
    {0x3723,0x18},
    {0x3724,0x04},
    {0x3725,0x04},
    {0x3726,0x02},
    {0x3727,0x02},
    {0x3728,0x02},
    {0x3729,0x02},
    {0x372a,0x05},
    {0x372b,0x65},
    {0x372c,0x55},
    {0x372d,0x65},
    {0x372e,0x53},
    {0x372f,0x33},
    {0x3730,0x33},
    {0x3731,0x33},
    {0x3732,0x03},
    {0x3733,0x80},
    {0x3734,0x10},
    {0x3739,0x03},
    {0x373a,0x20},
    {0x373b,0x0c},
    {0x373c,0x1c},
    {0x373e,0x18},
    {0x3760,0x05},
    {0x3761,0x00},
    {0x3762,0x05},
    {0x3763,0x00},
    {0x3764,0x05},
    {0x3765,0x00},
    {0x3f08,0x20},

    {0x3800,0x01},
    {0x3801,0x80},
    {0x3802,0x02},
    {0x3803,0x84},
    {0x3804,0x10},
    {0x3805,0xbf},
    {0x3806,0x0b},
    {0x3807,0x0f},
    {0x3808,0x07},
    {0x3809,0x80},
    {0x380a,0x04},
    {0x380b,0x3a},
    {0x380c,0x04},
    {0x380d,0xb6},   //HTS
    {0x380e,0x04},
//    {0x380f,0x52},   //VTS
    {0x380f,0x5f},   //VTS
    {0x3811,0x17},
    {0x3813,0x02},
    {0x3814,0x03},
    {0x3815,0x01},
    {0x3820,0x01},
    {0x3821,0x06},
    {0x3829,0x02},
    {0x382a,0x03},
    {0x382b,0x01},
    {0x3830,0x08},
    
    {0x3f00,0x02},
    {0x4001,0x83},
    {0x4002,0x02},
    {0x4003,0x04},
    {0x400e,0x00},
    {0x4011,0x00},
    {0x4012,0x00},
    {0x4200,0x08},
    {0x4302,0x7f},
    {0x4303,0xff},
    {0x4304,0x00},
    {0x4305,0x00},
    {0x4501,0x30},
    {0x4603,0x10},
    {0x4837,0x14},
    {0x4b00,0x22},
    {0x4903,0x00},
    {0x5000,0x5F},  //DPC
    {0x5001,0x01},
    {0x5004,0x00},
    {0x5013,0x20},
    {0x5051,0x00},
    {0x5980,0x00},
    {0x5981,0x00},
    {0x5982,0x00},
    {0x5983,0x00},
    {0x5984,0x00},
    {0x5985,0x00},
    {0x5986,0x00},
    {0x5987,0x00},
    {0x5988,0x00},
    {0x0100,0x01},       
    {DELAY_CODE,100}  //delay 100 ms
};
#define NUM_OF_1080P_4LANE_INIT_REGS (sizeof(sensor_1080P_4LANE_init_regs) / sizeof(sensor_reg_t))

//----------------720 P ---------------------------
static sensor_reg_t sensor_720P_4LANE_init_regs[] = {
    {0x0100,0x00},       
    {DELAY_CODE,100},  //delay 100 ms
    {0x0103,0x01},
    {DELAY_CODE,100},  //delay 100 ms

    {0x0302,0x68},      // 45 fps @210MHz serial clk
//    {0x0302,0x34},      // 24 fps @105MHz serial clk
    {0x0305,0x04},
    {0x0306,0x00},
    {0x030e,0x01},
    {0x030c,0x3c},      // 45 fps @210MHz serial clk
//    {0x030c,0x20},      // 24 fps @105MHz serial clk
    
    {0x3018,0x72},     //            MIPI 4-LANE = 0x72 , 2LANE=0x32
    {0x3106,0x61},     //   
    {0x3022,0x01},
    {0x3031,0x0a},	//10-bit
    {0x3032,0x80},

    {0x3500,0x00},
    {0x3501,0x06},     //           Exp.  : old 0x45
    {0x3502,0x00},
    {0x3508,0x0c},
    {0x3509,0xff},
    {0x3601,0x88},
    {0x3602,0x00},
    {0x3603,0x05},
    {0x3604,0x02},
    {0x3605,0x50},
    {0x3606,0x00},
    {0x3607,0x2b},
    {0x3608,0x12},
    {0x3609,0x00},
    {0x360a,0x00},
    {0x360b,0x02},
    {0x360c,0x10},
    {0x360d,0x05},
    {0x3612,0x20},
    {0x3613,0x88},
    {0x3614,0x77},
    {0x3615,0xf4},
    {0x3616,0x30},
    {0x3617,0x00},
    {0x3618,0x20},
    {0x3619,0x00},
    {0x361a,0x10},
    {0x361d,0x00},
    {0x361e,0x00},
    {0x3631,0x60},
    {0x364a,0x06},
    {0x3700,0x40},
    {0x3701,0x08},
    {0x3702,0x11},
    {0x3703,0x20},
    {0x3704,0x08},
    {0x3705,0x00},
    {0x3706,0x84},
    {0x3707,0x08},
    {0x3708,0x20},
    {0x3709,0x3c},
    {0x370a,0x01},
    {0x370b,0x5d},
    {0x370c,0x03},
    {0x370e,0x20},
    {0x370f,0x05},
    {0x3710,0x20},
    {0x3711,0x0e},
    {0x3714,0x31},
    {0x3718,0x75},
    {0x3719,0x13},
    {0x371a,0x55},
    {0x371b,0x03},
    {0x371c,0x55},
    {0x371d,0x03},
    {0x371e,0x09},
    {0x371f,0x16},
    {0x3720,0x0b},
    {0x3721,0x18},
    {0x3722,0x0b},
    {0x3723,0x18},
    {0x3724,0x04},
    {0x3725,0x04},
    {0x3726,0x02},
    {0x3727,0x02},
    {0x3728,0x02},
    {0x3729,0x02},
    {0x372a,0x05},
    {0x372b,0x65},
    {0x372c,0x55},
    {0x372d,0x65},
    {0x372e,0x53},
    {0x372f,0x33},
    {0x3730,0x33},
    {0x3731,0x33},
    {0x3732,0x03},
    {0x3733,0x80},
    {0x3734,0x10},
    {0x3739,0x03},
    {0x373a,0x20},
    {0x373b,0x0c},
    {0x373c,0x1c},
    {0x373e,0x18},
    {0x3760,0x05},
    {0x3761,0x00},
    {0x3762,0x05},
    {0x3763,0x00},
    {0x3764,0x05},
    {0x3765,0x00},
    {0x3f08,0x20},

    {0x3800,0x01},
    {0x3801,0x80},
    {0x3802,0x02},
    {0x3803,0x94},
    {0x3804,0x10},
    {0x3805,0xbf},
    {0x3806,0x0b},
    {0x3807,0x0f},
    {0x3808,0x05},
    {0x3809,0x00},
    {0x380a,0x02},
    {0x380b,0xd2},
    {0x380c,0x04},
    {0x380d,0xb6},   //HTS
    {0x380e,0x02},
    {0x380f,0xe8},   //VTS

    {0x3811,0x17},
    {0x3813,0x02},
    {0x3814,0x03},
    {0x3815,0x01},
    {0x3820,0x01},
    {0x3821,0x06},
    {0x3829,0x02},
    {0x382a,0x03},
    {0x382b,0x01},
    {0x3830,0x08},

    {0x3f00,0x02},
    {0x4001,0x83},
    {0x4002,0x02},
    {0x4003,0x04},
    {0x400e,0x00},
    {0x4011,0x00},
    {0x4012,0x00},
    {0x4200,0x08},
    {0x4302,0x7f},
    {0x4303,0xff},
    {0x4304,0x00},
    {0x4305,0x00},
    {0x4501,0x30},
    {0x4603,0x10},
    {0x4837,0x14},
    {0x4b00,0x22},
    {0x4903,0x00},

    {0x5000,0x5F},       //DPC
    {0x5001,0x01},
    {0x5004,0x00},
    {0x5013,0x20},
    {0x5051,0x00},
    {0x5980,0x00},
    {0x5981,0x00},
    {0x5982,0x00},
    {0x5983,0x00},
    {0x5984,0x00},
    {0x5985,0x00},
    {0x5986,0x00},
    {0x5987,0x00},
    {0x5988,0x00},

    {0x0100,0x01},       
    {DELAY_CODE,100}  //delay 100 ms
};
#define NUM_OF_720P_4LANE_INIT_REGS (sizeof(sensor_720P_4LANE_init_regs) / sizeof(sensor_reg_t))

typedef struct gain_set {
	u8 digtal;
	u8 analog1;
	u8 analog0;
	u8 fine;
	u32 total_gain;     // UFIX 7.6
} gain_set_t;

static const gain_set_t gain_table[] = {
{0,	0,	0,	128,	  64},	// 1
{0,	0,	0,	136,	  68},	//1.0625 
{0,	0,	0,	144,	  72},	//1.125  
{0,	0,	0,	152,	  76},	//1.1875 
{0,	0,	0,	160,	  80},	//1.25   
{0,	0,	0,	168,	  84},	//1.3125 
{0,	0,	0,	176,	  88},	//1.375  
{0,	0,	0,	184,	  92},	//1.4375 
{0,	0,	0,	192,	  96},	//1.5    
{0,	0,	0,	200,	 100},	//1.5625 
{0,	0,	0,	208,	 104},	//1.625  
{0,	0,	0,	216,	 108},	//1.6875 
{0,	0,	0,	224,	 112},	//1.75   
{0,	0,	0,	232,	 116},	//1.8125 
{0,	0,	0,	240,	 120},	//1.875  
{0,	0,	0,	248,	 124},	//1.9375 
{0,	1,	0,	128,	 128},	// 2
{0,	1,	0,	136,	 136},	//2.125  
{0,	1,	0,	144,	 144},	//2.25   
{0,	1,	0,	152,	 152},	//2.375  
{0,	1,	0,	160,	 160},	//2.5    
{0,	1,	0,	168,	 168},	//2.625  
{0,	1,	0,	176,	 176},	//2.75   
{0,	1,	0,	184,	 184},	//2.875  
{0,	1,	0,	192,	 192},	// 3
{0,	1,	0,	200,	 200},	//3.125  
{0,	1,	0,	208,	 208},	//3.25   
{0,	1,	0,	216,	 216},	//3.375  
{0,	1,	0,	224,	 224},	//3.5    
{0,	1,	0,	232,	 232},	//3.625  
{0,	1,	0,	240,	 240},	//3.75   
{0,	1,	0,	248,	 248},	//3.875  
{0,	2,	0,	128,	 256},	// 4      
{0,	2,	0,	136,	 272},	//4.25   
{0,	2,	0,	144,	 288},	//4.5    
{0,	2,	0,	152,	 304},	//4.75   
{0,	2,	0,	160,	 320},	//5      
{0,	2,	0,	168,	 336},	//5.25   
{0,	2,	0,	176,	 352},	//5.5    
{0,	2,	0,	184,	 368},	//5.75   
{0,	2,	0,	192,	 384},	//6      
{0,	2,	0,	200,	 400},	//6.25   
{0,	2,	0,	208,	 416},	//6.5    
{0,	2,	0,	216,	 432},	//6.75   
{0,	2,	0,	224,	 448},	//7      
{0,	2,	0,	232,	 464},	//7.25   
{0,	2,	0,	240,	 480},	//7.5    
{0,	2,	0,	248,	 496},	//7.75   
{0,	3,	0,	128,	 512},	//8      
{0,	3,	0,	136,	 544},	//8.5    
{0,	3,	0,	144,	 576},	//9      
{0,	3,	0,	152,	 608},	//9.5    
{0,	3,	0,	160,	 640},	//10     
{0,	3,	0,	168,	 672},	//10.5   
{0,	3,	0,	176,	 704},	//11     
{0,	3,	0,	184,	 736},	//11.5   
{0,	3,	0,	192,	 768},	//12     
{0,	3,	0,	200,	 800},	//12.5   
{0,	3,	0,	208,	 832},	//13     
{0,	3,	0,	216,	 864},	//13.5   
{0,	3,	0,	224,	 896},	//14     
{0,	3,	0,	232,	 928},	//14.5   
{0,	3,	0,	240,	 960},	//15     
{0,	3,	0,	248,	 992},	//15.5
#if 0
{0,	3,	1,	128,	1024},	//16     
{0,	3,	1,	136,	1088},	//17     
{0,	3,	1,	144,	1152},	//18     
{0,	3,	1,	152,	1216},	//19     
{0,	3,	1,	160,	1280},	//20     
{0,	3,	1,	168,	1344},	//21     
{0,	3,	1,	176,	1408},	//22     
{0,	3,	1,	184,	1472},	//23     
{0,	3,	1,	192,	1536},	//24     
{0,	3,	1,	200,	1600},	//25     
{0,	3,	1,	208,	1664},	//26     
{0,	3,	1,	216,	1728},	//27     
{0,	3,	1,	224,	1792},	//28     
{0,	3,	1,	232,	1856},	//29     
{0,	3,	1,	240,	1920},	//30     
{0,	3,	1,	248,	1984},	//31     
{0,	3,	1,	255,	1984},	//31.975     
#else
{1,	3,	0,	128,	1024},	//16     
{1,	3,	0,	136,	1088},	//17     
{1,	3,	0,	144,	1152},	//18     
{1,	3,	0,	152,	1216},	//19     
{1,	3,	0,	160,	1280},	//20     
{1,	3,	0,	168,	1344},	//21     
{1,	3,	0,	176,	1408},	//22     
{1,	3,	0,	184,	1472},	//23     
{1,	3,	0,	192,	1536},	//24     
{1,	3,	0,	200,	1600},	//25     
{1,	3,	0,	208,	1664},	//26     
{1,	3,	0,	216,	1728},	//27     
{1,	3,	0,	224,	1792},	//28     
{1,	3,	0,	232,	1856},	//29     
{1,	3,	0,	240,	1920},	//30     
{1,	3,	0,	248,	1984},	//31     
{1,	3,	0,	255,	1984},	//31.975     
#endif
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
#if 0
    u32 pclk,sysclk;
    u32 reg_l, reg_h;
    u32 pre_div, mul, R_div,R_DIVR_S,R_Seld5_s,Div2_4, pll2_sclk;
    u32 Div_op_sys, Div_op_pix, Div_op_2lane, DIV2;
//    pclk=40000000;
    read_reg(0x0300, &pre_div);    
    pre_div=(pre_div & 0x03)+1;
    
    read_reg(0x0302, &mul);
    mul=mul & 0xff ; 
    
    read_reg(0x0305, &Div_op_sys);    
    Div_op_sys &= 0x07;
    if (Div_op_sys < 4)				// 0/1/2/3/4/5/6/7 => 1/1/1/1/2/4/6/8
        Div_op_sys = 1;
    else
        Div_op_sys = (Div_op_sys-3) * 2;	

    read_reg(0x0306, &Div_op_pix);    
    Div_op_pix=(Div_op_pix & 0x03)+4;	//0/1/2/3 => 4/5/6/8
	if (Div_op_pix == 7)
		Div_op_pix++;

    read_reg(0x0307, &Div_op_2lane);    
    Div_op_2lane=(Div_op_2lane & 0x1)+1;

    read_reg(0x3020, &DIV2);    
    DIV2=((DIV2 & 0x8) >> 3)+1;

    pclk = xclk / (pre_div * Div_op_sys * Div_op_pix * Div_op_2lane * DIV2) * mul;

    isp_info("pre_div=%d, Div_op_sys=%d, Div_op_pix=%d, Div_op_2lane=%d, DIV2=%d, mul=%d\n",
            pre_div, Div_op_sys, Div_op_pix, Div_op_2lane, DIV2, mul);
#if 0	
	//calculate sclk
    read_reg(0x380C, &reg_h);
    read_reg(0x380D, &reg_l);
    g_psensor->hts = ((reg_h & 0x1F) << 8) | (reg_l & 0xFF);
    read_reg(0x380E, &reg_h);
    read_reg(0x380F, &reg_l);
    g_psensor->vts = ((reg_h & 0x1F) << 8) | (reg_l & 0xFF);

    read_reg(0x030b, &pre_div);    
    pre_div=(pre_div & 0x03)+1;
    
    read_reg(0x030c, &mul);
    mul=mul & 0x3f ; 
    
    read_reg(0x030d, &R_div);    
    R_div=(R_div & 0x01)+1;

    read_reg(0x030e, &R_DIVR_S);    
    R_DIVR_S=(R_DIVR_S & 0x0f)+1;

    read_reg(0x030f, &R_Seld5_s);    
    R_Seld5_s=R_Seld5_s & 0x3;
    switch (R_Seld5_s){
        case   0:
                R_Seld5_s=10;
                break;
        case   1:
                R_Seld5_s=10;
                break;
        case   2:
                R_Seld5_s=20;
                break;        
        case   3:
                R_Seld5_s=25;
                break;
    }
    pll2_sclk=xclk / pre_div * mul * R_div;
    pll2_sclk=pll2_sclk/ R_DIVR_S ;
    pll2_sclk=(pll2_sclk*10) / R_Seld5_s;    

    read_reg(0x3106, &Div2_4);    
    Div2_4= Div2_4 >>4;
    if (Div2_4==0)
        Div2_4=1;

    // sysclk 
    sysclk=pll2_sclk / Div2_4;
    isp_info("Get Pclk: Frame Rate=%d , Sysclk=%d M(Hz) ,  HTS= 0x%x , VTS=0x%x \n", (sysclk*100/hts/vts) , (sysclk/1000000), hts , vts);    
#endif
    isp_info("pixel clock=%d Hz\n", pclk);
    return pclk;
#else
    u32 pclk;

    if (g_psensor->img_win.width <= 1280)
        pclk = 160000000;
    else if (g_psensor->img_win.width <= 1920)
        pclk = 160000000;
    else if (g_psensor->img_win.width <= 4096)
        pclk = 160000000;

    return pclk;
#endif
}

static void adjust_blanking(int fps)
{
    isp_info("adjust_blanking, fps=%d\n", fps);
}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    pinfo->t_row = (pinfo->hts * 10000) / ( g_psensor->pclk / 1000);
}

static int set_size(u32 width, u32 height)
{

   sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 reg_l, reg_h;
   

    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("%dx%d exceeds maximum resolution %dx%d!\n",
            width, height, SENSOR_MAX_WIDTH, SENSOR_MAX_HEIGHT);
        return -1;
    }
    
    g_psensor->min_exp  = (pinfo->t_row + 500) / 1000;
    
    read_reg(0x380E, &reg_h);
    read_reg(0x380F, &reg_l);
    pinfo->vts = ((reg_h & 0xF) << 8) | (reg_l & 0xFF);
    
    read_reg(0x380C, &reg_h);
    read_reg(0x380D, &reg_l);
    pinfo->hts = (((reg_h & 0xFF) << 8) | (reg_l & 0xFF)) * 3;
    
    adjust_blanking(g_psensor->fps);
    calculate_t_row();
    
    isp_info("hts = %d vts=%d trow=%d\n",pinfo->hts, pinfo->vts,pinfo->t_row);
    
    // update sensor information
    g_psensor->out_win.width = width; // SENSOR_MAX_WIDTH;
    g_psensor->out_win.height = height; //SENSOR_MAX_HEIGHT;
    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    
    if (width == 4608 && height == 3456){
        g_psensor->img_win.x = (width -1920) / 2;
        g_psensor->img_win.y = (height -1080) / 2;
        g_psensor->img_win.width = 1920;
        g_psensor->img_win.height = 1080;
    }
    else{
        g_psensor->img_win.x = 0;
        g_psensor->img_win.y = 0;
        g_psensor->img_win.width = width;
        g_psensor->img_win.height = height;
    }

    return ret;
}


static u32 get_exp(void)
{
    u32 lines, sw_t, sw_m, sw_b;
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (!g_psensor->curr_exp) {
       
       
        read_reg(0x3500, &sw_t);
        read_reg(0x3501, &sw_m);
        read_reg(0x3502, &sw_b);
       
        lines = ((sw_t & 0xF) << 12)|((sw_m & 0xFF) << 4)|((sw_b & 0xFF) >> 4);
        //isp_info("lines = %d\n",lines);
        g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;
    }
//	isp_info("get_exp=%d\n", g_psensor->curr_exp);

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
//    sensor_info_t *pinfo = (sensor_info_t *)pdev->private;
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 lines, tmp;

    lines = ((exp * 1000) + pinfo->t_row /2 ) / pinfo->t_row;
//    lines = ((lines>0x7ea)?0x7ea:lines);

    if (lines == 0)
        lines = 1;

//    if (lines > pinfo->vts)
//        sw_res = lines - pinfo->vts;
//    else
//        sw_res = 0;

    tmp = lines << 4;

//    pinfo->hts = 0;

//    //write_reg(0x3212, 0x00);    // Enable Group0
//    if (sw_res != pinfo->hts) {
//        write_reg(0x350C, (sw_res >> 8));
//        write_reg(0x350D, (sw_res & 0xFF));
//        pinfo->hts = sw_res;
//    }

    if (tmp != pinfo->prev_sw_reg) {
        write_reg(0x3502, (tmp & 0xFF));
        write_reg(0x3501, (tmp >> 8) & 0xFF);
        write_reg(0x3500, (tmp >> 16) & 0xFF);
        pinfo->prev_sw_reg = tmp;
//        isp_info("set_exp = %x\n",tmp);
    }

    //write_reg(0x3212, 0x10);    // End Group0
    //write_reg(0x3212, 0xA0);    // Launch Group0

    g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;

	isp_info("set_exp=%d %d\n", exp, g_psensor->curr_exp);

    return ret;
}

static u32 get_gain(void)
{
	int i,a0,a1,d,f;

	if (g_psensor->curr_gain==0) {              
		read_reg(0x3508, &d);
		read_reg(0x3509, &f);
		a1 = (d & 0x0c) >> 2;
		a0 = (d & 0x03);
		d = (d & 0x30) >> 4;
		f = (f & 0xFF);

		for (i=0; i<NUM_OF_GAINSET; i++) {
			if ((gain_table[i].digtal == d)&&(gain_table[i].analog1 == a1)
					&& (gain_table[i].analog0 == a0)&&(gain_table[i].fine ==f))
			break;
		}
       
		g_psensor->curr_gain = (gain_table[i].total_gain);
	}
//	isp_info("get_gain=%d\n", g_psensor->curr_gain);
	return g_psensor->curr_gain;   
}

static int set_gain(u32 gain)
{
	int ret = 0;
	u32 i,d,f;
   
	if (gain > gain_table[NUM_OF_GAINSET - 1].total_gain)
		gain = gain_table[NUM_OF_GAINSET - 1].total_gain;

	if (gain < gain_table[0].total_gain)
		gain = gain_table[0].total_gain;

	// search most suitable gain into gain table
	for (i=0; i<NUM_OF_GAINSET; i++) {
		if (gain_table[i].total_gain > gain)
			break;
	}
    	if (i > 0) i--;
    
    	d = (gain_table[i].digtal << 4) | (gain_table[i].analog1 << 2) | gain_table[i].analog0;
    	f = gain_table[i].fine;

   
	write_reg(0x3508, d);
	write_reg(0x3509, f);
    

	g_psensor->curr_gain = (gain_table[i].total_gain);

//	isp_info("set_gain=%d %d\n", gain, g_psensor->curr_gain);
   
    return ret;
}

static int set_fps(int fps)
{
    u32 val[18]={0x35a2,0x24a2,0x1da2,0x14a2,0x0da2,
                        0x17a2,0x15a2,0x13a2,0x10a2,0x0ea2,
                        0x0da2,0x0c72,0x0ba2,0x0aa2,0x09b2,
                        0x0952,0x08b2,0x0872};

    if (g_psensor->out_win.width == 4096 && g_psensor->out_win.height== 2048){
        if (fps <= 18 && fps >= 1){
            if (fps >= 6){
                write_reg(0x302, 0x6c);
                write_reg(0x30c, 0x6c);
                write_reg(0x30e, 0x00);
            }
            else{
                write_reg(0x302, 0x3c);
                write_reg(0x30c, 0x6c);
                write_reg(0x30e, 0x01);
            }
            write_reg(0x380e, val[fps-1]>>8);
            write_reg(0x380f, val[fps-1]);
            isp_info("set_fps %d\n", fps);
        }
        else{
            isp_info("set_fps,  fps=1~18\n");
        }
    }
    return 0;
}

static int get_mirror(void)
{
	u32 reg;
    
      read_reg(0x3821, &reg);
      return ((reg & 0x06) == 6) ? 1 : 0;

}

static int set_mirror(int enable)
{
	u32 reg;
	
	read_reg(0x3821, &reg);
	if (enable)
		reg |= 0x06;
	else
		reg &= ~0x06;
	write_reg(0x3821, reg);
	
	read_reg(0x3811, &reg);
	if (enable)
		reg |= 0x01;
	else
		reg &= ~0x01;
	write_reg(0x3811, reg);
	
	return 0;

}

static int get_flip(void)
{
	u32 reg;
    
      read_reg(0x3820, &reg);
      return ((reg & 0x06) == 6) ? 1 : 0;
}

static int set_flip(int enable)
{
	u32 reg;
	
	read_reg(0x3820, &reg);
	if (enable)
		reg |= 0x06;
	else
		reg &= ~0x06;
	write_reg(0x3820, reg);

	read_reg(0x3813, &reg);
	if (enable)
		reg |= 0x01;
	else
		reg &= ~0x01;
	write_reg(0x3813, reg);
	
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
        
    case ID_FPS:
        set_fps((int)arg);
        break;
        
//    case ID_Sensor_Mode:        
//        set_sensor_mode((int)arg);        
//        break;   

    default:
        ret = -EFAULT;
        break;
    }

    return ret;
}


//
static int SenRes_init(void){
    int ret=0, i;
    u32 readtmp;
    sensor_reg_t* pInitTable;
    u32 InitTable_Num;

    if (g_psensor->img_win.width <= 1280 && g_psensor->img_win.height <= 720){
        pInitTable=sensor_720P_4LANE_init_regs;
        InitTable_Num=NUM_OF_720P_4LANE_INIT_REGS;
    }
    else if (g_psensor->img_win.width <= 1920 && g_psensor->img_win.height <= 1080){
        pInitTable=sensor_1080P_4LANE_init_regs;
        InitTable_Num=NUM_OF_1080P_4LANE_INIT_REGS;
    }
    else if (g_psensor->img_win.width <= 2048 && g_psensor->img_win.height <= 1536){
        pInitTable=sensor_1536P_4LANE_init_regs;
        InitTable_Num=NUM_OF_1536P_4LANE_INIT_REGS;
    }
    else if (g_psensor->img_win.width <= 4096 && g_psensor->img_win.height <= 2048){
        pInitTable=sensor_4096x2048_init_regs;
        InitTable_Num=NUM_OF_4096x2048_INIT_REGS;
    }
    else{   //4608x3456
    pInitTable=sensor_4608x3456_init_regs;
    InitTable_Num=NUM_OF_4608x3456_INIT_REGS;
    }
        
    isp_info("start OV16820 initial...\n");    
     // set initial registers
        for (i=0; i<InitTable_Num; i++) {    
             if (pInitTable[i].addr == DELAY_CODE) {
//                mdelay(pInitTable[i].val);
             }          
             else if(write_reg(pInitTable[i].addr, pInitTable[i].val) < 0) {
                    isp_err("%s init fail!!", SENSOR_NAME);
                    return -EINVAL;
             } 
//             mdelay(10);        
         }    

    if (interface == IF_LVDS){
        write_reg(0x306, 0x01);
        write_reg(0x3022, 0x09);
        write_reg(0x4303, 0x78);
        write_reg(0x4305, 0x80);
        write_reg(0x4b00, 0x2b);
        isp_info("interfacce : LVDS\n");
    }
    else{
        isp_info("interfacce : MIPI\n");
    }
    if (g_psensor->img_win.width <= 1280 && g_psensor->img_win.height <= 720){
        if (ch_num == 2){
            write_reg(0x302, 0x68);
            write_reg(0x30c, 0x3c);
            write_reg(0x30e, 0x01);
            write_reg(0x3018, 0x32);                
        }
        else if (ch_num == 1){
            write_reg(0x302, 0x68);
            write_reg(0x30c, 0x3c);
            write_reg(0x30e, 0x02);
            write_reg(0x3018, 0x12);
        }
    }
    else if (g_psensor->img_win.width <= 1920 && g_psensor->img_win.height <= 1080){
        if (ch_num == 2){
#if 0            
            //60 fps, MIPI 
            write_reg(0x302, 0xa0);
            write_reg(0x30c, 0x38);
            write_reg(0x30e, 0x00);
#else            
            //30 fps 
            write_reg(0x302, 0x6c);
            write_reg(0x30c, 0x35);
            write_reg(0x30e, 0x01);
#endif            
            write_reg(0x3018, 0x32);
        }
        else if (ch_num == 1){
#if 0            
            // 40 fps, MIPI, 450 MHz s-clk
            write_reg(0x302, 0xc4);
            write_reg(0x30c, 0x24);
            write_reg(0x30e, 0x00);
#else            
            //24 fps
            write_reg(0x302, 0x74);
            write_reg(0x30c, 0x3f);
            write_reg(0x30e, 0x02);
#endif            
            write_reg(0x3018, 0x12);
        }
    }
    if (g_psensor->img_win.width == 2048 && g_psensor->img_win.height == 1536){
        if (ch_num == 2){
            write_reg(0x3018, 0x32);                
        }
        else if (ch_num == 1){
            write_reg(0x3018, 0x12);
        }
    }
    
    if (interface == IF_LVDS)
        isp_info("%d Channel\n", ch_num);
    else
        isp_info("%d Lane\n", ch_num);   
    
    //enable digital gain
    read_reg(0x5004, &readtmp);
    readtmp |= 0x80;
    write_reg(0x5004, readtmp);

    return ret;
}

// initial
static int init(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;

    if (pinfo->is_init)
        return 0;

    //Sensor init
    SenRes_init();

    // get pixel clock
    g_psensor->pclk = get_pclk(g_psensor->xclk);

    // set image size
    if ((ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height)) < 0)
        return ret;
    set_fps(g_psensor->fps);
    // set mirror and flip
//    isp_info ("set mirror and flip %d %d\n", mirror, flip);
//    set_mirror(mirror);
//    set_flip(flip);

    // get current shutter_width and gain setting
    ///g_psensor->curr_exp = get_exp();
   // g_psensor->curr_gain = get_gain();   

    pinfo->is_init = true;

    
    return ret;
}

//=============================================================================
// external functions
//=============================================================================
static void ov16820_deconstruct(void)
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

static int ov16820_construct(u32 xclk, u16 width, u16 height)
{
    sensor_info_t *pinfo = NULL;
    int ret = 0;
    
//    isp_info("entry ov16820_construct@@Hight \n");

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
    g_psensor->bayer_type = BAYER_BG;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_HIGH;
    g_psensor->img_win.x = 0;
    g_psensor->img_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;
    g_psensor->min_exp = 1;
    g_psensor->max_exp = 5000; // 0.5 sec
    g_psensor->min_gain = gain_table[0].total_gain;
    g_psensor->max_gain = gain_table[NUM_OF_GAINSET - 1].total_gain;
    g_psensor->exp_latency = 1;
    g_psensor->gain_latency = 1;
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

    if ((ret = sensor_init_i2c_driver()) < 0)
        goto construct_err;

    if (interface == IF_LVDS){
        if ((ret = init()) < 0)
            goto construct_err;
    }
    return 0;

construct_err:
    ov16820_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init ov16820_init(void)
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

    if ((ret = ov16820_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit ov16820_exit(void)
{
    isp_unregister_sensor(g_psensor);
    ov16820_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(ov16820_init);
module_exit(ov16820_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor ov16820");
MODULE_LICENSE("GPL");
