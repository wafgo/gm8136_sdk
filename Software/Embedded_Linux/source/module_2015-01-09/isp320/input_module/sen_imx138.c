/**
 * @file sen_imx138.c
 * Sony IMX138 sensor driver
 *
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.9 $
 * $Date: 2014/10/16 08:57:38 $
 *
 * ChangeLog:
 *  $Log: sen_imx138.c,v $
 *  Revision 1.9  2014/10/16 08:57:38  jason_ha
 *  Add LVDS Interface
 *
 *  Revision 1.8  2014/07/25 08:38:32  ricktsao
 *  no message
 *
 *  Revision 1.7  2014/07/22 09:31:43  jason_ha
 *  *** empty log message ***
 *
 *  Revision 1.5  2014/03/04 09:09:20  jason_ha
 *  Blocklevel -> 0xF0
 *
 *  Revision 1.4  2013/12/09 11:23:41  jason_ha
 *  Fixed min_exp
 *
 *  Revision 1.3  2013/11/28 03:46:21  jason_ha
 *  *** empty log message ***
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

#define PFX "sen_imx138"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"

//#define sensor_37125
#define sensor_54


//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1280
#define DEFAULT_IMAGE_HEIGHT    720
#define DEFAULT_INTERFACE       IF_PARALLEL

#ifdef sensor_37125
#define DEFAULT_XCLK            37125000
#endif

#ifdef sensor_54
#define DEFAULT_XCLK            54000000
#endif

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

static ushort sensor_spi = 1;  //0:I2C, 1: SPI
module_param(sensor_spi, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_spi, "sensor SPI interface");

static ushort interface = DEFAULT_INTERFACE;
module_param( interface, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(interface, "interface type");

//=============================================================================
// global
//=============================================================================
#define SENSOR_NAME         "IMX138"
#define SENSOR_MAX_WIDTH    1280
#define SENSOR_MAX_HEIGHT   1024
#define ID_IMX138           0x0
#define ID_IMX238           0x40

static sensor_dev_t *g_psensor = NULL;

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
    u16 addr;
    u16 val;
} sensor_reg_t;
#define DELAY_CODE      0xFFFF


static sensor_reg_t imx138_init_regs[] = {

#ifdef sensor_37125
    {0x3000, 0x01},      // Standby 0: Operating    1: Standby

    {0x3005, 0x01},      // AD conversion bits  0: 10bit   1:12 bit

    {0x3006, 0x00},      // Drive mode setting  00h: All-pix scan mode   33h: Vertical 1/2 subsampling readout mode

    {0x3007, 0x00},      // Window mode setting

    {0x3009, 0x02},      // Frame rate setting  0: 120 fps  1: 60 fps  2: 30fps
    //37.125MHz
    {0x3018, 0x4C},     // Vmax Lsb

    {0x3019, 0x04},      // Vmax Msb

    {0x301B, 0x94},     // Hmax Lsb

    {0x301C, 0x11},     // Hmax Msb

    {0x300A, 0xF0},     // Black Level Adjustment, 10 bits 0x3c,  12bits: 0xF0(240), 0x78(120)

    {0x3017, 0x01},      // Set to "01h"

    {0x301D, 0xFF},    // Set to "FFh"

    {0x301E, 0x01},      // Set to "01h"

    {0x3044, 0x01},      // output bit setting 0: 10bit  1: 12bit     Output system 0: Parallel CMOS  1: Parallel LVDS...

    {0x3046, 0x01},      // Vsync line output, polarity

    {0x3047, 0x0A},     // Sync code H V

    {0x3048, 0xC0},     // DOM[6] & DCKM Output enable

    {0x3049, 0x0A},     // XVS& XHSOUTSEL

    {0x3054, 0x63},     // Set to "63h"

    {0x305B, 0x00},     // INCK setting 1  0: 37.125 MHz or 74.25 MHz   1: 27 MHz or 54 MHz

    {0x305D, 0x00},     // INCK setting 2  0: Input 27 MHz or 37.125 MHz   1: Input 54 MHz or 74.25 MHz

    {0x30A1, 0x45},     // Set to "45h"

    {0x30BF, 0x1F},     // Set to "1Fh"

    {0x3000, 0x00},     // Standby 0: Operating    1: Standby

    {0x3002, 0x00}     // master mode operation 0: Start    1: Stop
#endif

#ifdef sensor_54
    {0x3000, 0x01},      // Standby 0: Operating    1: Standby

    {0x3005, 0x01},      // AD conversion bits  0: 10bit   1:12 bit

    {0x3006, 0x00},      // Drive mode setting  00h: All-pix scan mode   33h: Vertical 1/2 subsampling readout mode

    {0x3007, 0x00},      // Window mode setting

    {0x3009, 0x02},      // Frame rate setting  0: 120 fps  1: 60 fps  2: 30fps

    //54MHZ
    {0x3018, 0x44},     // Vmax Lsb

    {0x3019, 0x04},      // Vmax Msb

    {0x301B, 0xE4},     // Hmax Lsb

    {0x301C, 0x0C},     // Hmax Msb

    {0x300A, 0xF0},     // Black Level Adjustment, 10 bits 0x3c,  12bits: 0xF0(240), 0x78(120)

    {0x3017, 0x01},      // Set to "01h"

    {0x301D, 0xFF},    // Set to "FFh"

    {0x301E, 0x01},      // Set to "01h"

    {0x3044, 0x01},      // output bit setting 0: 10bit  1: 12bit     Output system 0: Parallel CMOS  1: Parallel LVDS...

    {0x3046, 0x01},      // Vsync line output, polarity

    {0x3047, 0x0A},     // Sync code H V

    {0x3048, 0x00},     // DOM[6] & DCKM Output enable

    {0x3049, 0x0A},     // XVS& XHSOUTSEL

    {0x3054, 0x63},     // Set to "63h"

    {0x305B, 0x01},     // INCK setting 1  0: 37.125 MHz or 74.25 MHz   1: 27 MHz or 54 MHz Bit[0]

    {0x305D, 0x10},     // INCK setting 2  0: Input 27 MHz or 37.125 MHz   1: Input 54 MHz or 74.25 MHz Bit[4]

    {0x305F, 0x10},     // INCK setting 3  0: Input 27 MHz   1: Input 54 MHz Bit[4]

    {0x30A1, 0x45},     // Set to "45h"

    {0x30BF, 0x1F},     // Set to "1Fh"

    {0x3000, 0x00},     // Standby 0: Operating    1: Standby

    {0x3002, 0x00}     // master mode operation 0: Start    1: Stop
#endif

};
#define NUM_OF_IMX138_INIT_REGS (sizeof(imx138_init_regs) / sizeof(sensor_reg_t))


static sensor_reg_t imx238_init_regs[] = {

#ifdef sensor_37125
    {0x3000, 0x01},      // Standby 0: Operating    1: Standby

    {0x3005, 0x01},      // AD conversion bits  0: 10bit   1:12 bit

    {0x3006, 0x00},      // Drive mode setting  00h: All-pix scan mode   33h: Vertical 1/2 subsampling readout mode

    {0x3007, 0x00},      // Window mode setting

    {0x3009, 0x02},      // Frame rate setting  0: 120 fps  1: 60 fps  2: 30fps
    //37.125MHz
    {0x3018, 0x4C},     // Vmax Lsb

    {0x3019, 0x04},      // Vmax Msb

    {0x301B, 0x94},     // Hmax Lsb

    {0x301C, 0x11},     // Hmax Msb

    {0x300A, 0xF0},     // Black Level Adjustment, 10 bits 0x3c,  12bits: 0xF0(240), 0x78(120)

    {0x3017, 0x01},      // Set to "01h"

    {0x301D, 0xFF},    // Set to "FFh"

    {0x301E, 0x01},      // Set to "01h"

    {0x3044, 0x01},      // output bit setting 0: 10bit  1: 12bit     Output system 0: Parallel CMOS  1: Parallel LVDS...

    {0x3046, 0x01},      // Vsync line output, polarity

    {0x3047, 0x0A},     // Sync code H V

    {0x3048, 0xC0},     // DOM[6] & DCKM Output enable

    {0x3049, 0x0A},     // XVS& XHSOUTSEL

    {0x3054, 0x63},     // Set to "63h"

    {0x305B, 0x00},     // INCK setting 1  0: 37.125 MHz or 74.25 MHz   1: 27 MHz or 54 MHz

    {0x305D, 0x00},     // INCK setting 2  0: Input 27 MHz or 37.125 MHz   1: Input 54 MHz or 74.25 MHz

    {0x30A1, 0x45},     // Set to "45h"

    {0x30BF, 0x1F},     // Set to "1Fh"

    {0x3123, 0x07},     //For IMX238 add
    {0x3126, 0xDF},
    {0x3147, 0x87},
    {0x3203, 0xCD},
    {0x3207, 0x4B},
    {0x3209, 0xE9},
    {0x3213, 0x1B},
    {0x3215, 0xED},
    {0x3216, 0x01},
    {0x3218, 0x09},
    {0x321A, 0x19},
    {0x321B, 0xA1},
    {0x321C, 0x11},
    {0x3227, 0x00},
    {0x3228, 0x05},
    {0x3229, 0xEC},
    {0x322A, 0x40},
    {0x322B, 0x11},
    {0x322D, 0x22},
    {0x322E, 0x00},
    {0x322F, 0x05},
    {0x3231, 0xEC},
    {0x3232, 0x40},
    {0x3233, 0x11},
    {0x3235, 0x23},
    {0x3236, 0xB0},
    {0x3237, 0x04},
    {0x3239, 0x24},
    {0x323A, 0x30},
    {0x323B, 0x04},
    {0x323C, 0xED},
    {0x323D, 0xC0},
    {0x323E, 0x10},
    {0x3240, 0x44},
    {0x3241, 0xA0},
    {0x3242, 0x04},
    {0x3243, 0x0D},
    {0x3244, 0x31},
    {0x3245, 0x11},
    {0x3247, 0xEC},
    {0x3248, 0xD0},
    {0x3249, 0x1D},
    {0x3252, 0xFF},
    {0x3253, 0xFF},
    {0x3254, 0xFF},
    {0x3255, 0x02},
    {0x3256, 0x54},
    {0x3257, 0x60},
    {0x3258, 0x1F},
    {0x325A, 0xA9},
    {0x325B, 0x50},
    {0x325C, 0x0A},
    {0x325D, 0x16},
    {0x325E, 0xA1},
    {0x325F, 0x0E},
    {0x3261, 0x8D},
    {0x3266, 0xB0},
    {0x3267, 0x09},
    {0x326A, 0x20},
    {0x326B, 0x0A},
    {0x326E, 0x20},
    {0x326F, 0x0A},
    {0x3272, 0x20},
    {0x3273, 0x0A},
    {0x3275, 0xEC},
    {0x327D, 0xA5},
    {0x327E, 0x20},
    {0x327F, 0x0A},
    {0x3281, 0xEF},
    {0x3282, 0xC0},
    {0x3283, 0x0E},
    {0x3285, 0xF6},
    {0x328A, 0x60},
    {0x328B, 0x1F},
    {0x328D, 0xBB},
    {0x328E, 0x90},
    {0x328F, 0x0D},
    {0x3290, 0x39},
    {0x3291, 0xC1},
    {0x3292, 0x1D},
    {0x3294, 0xC9},
    {0x3295, 0x70},
    {0x3296, 0x0E},
    {0x3297, 0x47},
    {0x3298, 0xA1},
    {0x3299, 0x1E},
    {0x329B, 0xC5},
    {0x329C, 0xB0},
    {0x329D, 0x0E},
    {0x329E, 0x43},
    {0x329F, 0xE1},
    {0x32A0, 0x1E},
    {0x32A2, 0xBB},
    {0x32A3, 0x10},
    {0x32A4, 0x0C},
    {0x32A6, 0xB3},
    {0x32A7, 0x30},
    {0x32A8, 0x0A},
    {0x32A9, 0x29},
    {0x32AA, 0x91},
    {0x32AB, 0x11},
    {0x32AD, 0xB4},
    {0x32AE, 0x40},
    {0x32AF, 0x0A},
    {0x32B0, 0x2A},
    {0x32B1, 0xA1},
    {0x32B2, 0x11},
    {0x32B4, 0xAB},
    {0x32B5, 0xB0},
    {0x32B6, 0x0B},
    {0x32B7, 0x21},
    {0x32B8, 0x11},
    {0x32B9, 0x13},
    {0x32BB, 0xAC},
    {0x32BC, 0xC0},
    {0x32BD, 0x0B},
    {0x32BE, 0x22},
    {0x32BF, 0x21},
    {0x32C0, 0x13},
    {0x32C2, 0xAD},
    {0x32C3, 0x10},
    {0x32C4, 0x0B},
    {0x32C5, 0x23},
    {0x32C6, 0x71},
    {0x32C7, 0x12},
    {0x32C9, 0xB5},
    {0x32CA, 0x90},
    {0x32CB, 0x0B},
    {0x32CC, 0x2B},
    {0x32CD, 0xF1},
    {0x32CE, 0x12},
    {0x32D0, 0xBB},
    {0x32D1, 0x10},
    {0x32D2, 0x0C},
    {0x32D4, 0xE7},
    {0x32D5, 0x90},
    {0x32D6, 0x0E},
    {0x32D8, 0x45},
    {0x32D9, 0x11},
    {0x32DA, 0x1F},
    {0x32EB, 0xA4},
    {0x32EC, 0x60},
    {0x32ED, 0x1F},

    {0x3000, 0x00},     // Standby 0: Operating    1: Standby

    {0x3002, 0x00},     // master mode operation 0: Start    1: Stop

#endif

#ifdef sensor_54
    {0x3000, 0x01},      // Standby 0: Operating    1: Standby

    {0x3005, 0x01},      // AD conversion bits  0: 10bit   1:12 bit

    {0x3006, 0x00},      // Drive mode setting  00h: All-pix scan mode   33h: Vertical 1/2 subsampling readout mode

    {0x3007, 0x00},      // Window mode setting

    {0x3009, 0x02},      // Frame rate setting  0: 120 fps  1: 60 fps  2: 30fps

    //54MHZ
    {0x3018, 0x44},     // Vmax Lsb

    {0x3019, 0x04},      // Vmax Msb

    {0x301B, 0xE4},     // Hmax Lsb

    {0x301C, 0x0C},     // Hmax Msb

    {0x300A, 0xF0},     // Black Level Adjustment, 10 bits 0x3c,  12bits: 0xF0(240), 0x78(120)

    {0x3017, 0x01},      // Set to "01h"

    {0x301D, 0xFF},    // Set to "FFh"

    {0x301E, 0x01},      // Set to "01h"

    {0x3044, 0x01},      // output bit setting 0: 10bit  1: 12bit     Output system 0: Parallel CMOS  1: Parallel LVDS...

    {0x3046, 0x01},      // Vsync line output, polarity

    {0x3047, 0x0A},     // Sync code H V

    {0x3048, 0x00},     // DOM[6] & DCKM Output enable

    {0x3049, 0x0A},     // XVS& XHSOUTSEL

    {0x3054, 0x63},     // Set to "63h"

    {0x305B, 0x01},     // INCK setting 1  0: 37.125 MHz or 74.25 MHz   1: 27 MHz or 54 MHz Bit[0]

    {0x305D, 0x10},     // INCK setting 2  0: Input 27 MHz or 37.125 MHz   1: Input 54 MHz or 74.25 MHz Bit[4]

    {0x305F, 0x10},     // INCK setting 3  0: Input 27 MHz   1: Input 54 MHz Bit[4]

    {0x30A1, 0x45},     // Set to "45h"

    {0x30BF, 0x1F},     // Set to "1Fh"

    {0x3123, 0x07},     //For IMX238 add
    {0x3126, 0xDF},
    {0x3147, 0x87},
    {0x3203, 0xCD},
    {0x3207, 0x4B},
    {0x3209, 0xE9},
    {0x3213, 0x1B},
    {0x3215, 0xED},
    {0x3216, 0x01},
    {0x3218, 0x09},
    {0x321A, 0x19},
    {0x321B, 0xA1},
    {0x321C, 0x11},
    {0x3227, 0x00},
    {0x3228, 0x05},
    {0x3229, 0xEC},
    {0x322A, 0x40},
    {0x322B, 0x11},
    {0x322D, 0x22},
    {0x322E, 0x00},
    {0x322F, 0x05},
    {0x3231, 0xEC},
    {0x3232, 0x40},
    {0x3233, 0x11},
    {0x3235, 0x23},
    {0x3236, 0xB0},
    {0x3237, 0x04},
    {0x3239, 0x24},
    {0x323A, 0x30},
    {0x323B, 0x04},
    {0x323C, 0xED},
    {0x323D, 0xC0},
    {0x323E, 0x10},
    {0x3240, 0x44},
    {0x3241, 0xA0},
    {0x3242, 0x04},
    {0x3243, 0x0D},
    {0x3244, 0x31},
    {0x3245, 0x11},
    {0x3247, 0xEC},
    {0x3248, 0xD0},
    {0x3249, 0x1D},
    {0x3252, 0xFF},
    {0x3253, 0xFF},
    {0x3254, 0xFF},
    {0x3255, 0x02},
    {0x3256, 0x54},
    {0x3257, 0x60},
    {0x3258, 0x1F},
    {0x325A, 0xA9},
    {0x325B, 0x50},
    {0x325C, 0x0A},
    {0x325D, 0x16},
    {0x325E, 0xA1},
    {0x325F, 0x0E},
    {0x3261, 0x8D},
    {0x3266, 0xB0},
    {0x3267, 0x09},
    {0x326A, 0x20},
    {0x326B, 0x0A},
    {0x326E, 0x20},
    {0x326F, 0x0A},
    {0x3272, 0x20},
    {0x3273, 0x0A},
    {0x3275, 0xEC},
    {0x327D, 0xA5},
    {0x327E, 0x20},
    {0x327F, 0x0A},
    {0x3281, 0xEF},
    {0x3282, 0xC0},
    {0x3283, 0x0E},
    {0x3285, 0xF6},
    {0x328A, 0x60},
    {0x328B, 0x1F},
    {0x328D, 0xBB},
    {0x328E, 0x90},
    {0x328F, 0x0D},
    {0x3290, 0x39},
    {0x3291, 0xC1},
    {0x3292, 0x1D},
    {0x3294, 0xC9},
    {0x3295, 0x70},
    {0x3296, 0x0E},
    {0x3297, 0x47},
    {0x3298, 0xA1},
    {0x3299, 0x1E},
    {0x329B, 0xC5},
    {0x329C, 0xB0},
    {0x329D, 0x0E},
    {0x329E, 0x43},
    {0x329F, 0xE1},
    {0x32A0, 0x1E},
    {0x32A2, 0xBB},
    {0x32A3, 0x10},
    {0x32A4, 0x0C},
    {0x32A6, 0xB3},
    {0x32A7, 0x30},
    {0x32A8, 0x0A},
    {0x32A9, 0x29},
    {0x32AA, 0x91},
    {0x32AB, 0x11},
    {0x32AD, 0xB4},
    {0x32AE, 0x40},
    {0x32AF, 0x0A},
    {0x32B0, 0x2A},
    {0x32B1, 0xA1},
    {0x32B2, 0x11},
    {0x32B4, 0xAB},
    {0x32B5, 0xB0},
    {0x32B6, 0x0B},
    {0x32B7, 0x21},
    {0x32B8, 0x11},
    {0x32B9, 0x13},
    {0x32BB, 0xAC},
    {0x32BC, 0xC0},
    {0x32BD, 0x0B},
    {0x32BE, 0x22},
    {0x32BF, 0x21},
    {0x32C0, 0x13},
    {0x32C2, 0xAD},
    {0x32C3, 0x10},
    {0x32C4, 0x0B},
    {0x32C5, 0x23},
    {0x32C6, 0x71},
    {0x32C7, 0x12},
    {0x32C9, 0xB5},
    {0x32CA, 0x90},
    {0x32CB, 0x0B},
    {0x32CC, 0x2B},
    {0x32CD, 0xF1},
    {0x32CE, 0x12},
    {0x32D0, 0xBB},
    {0x32D1, 0x10},
    {0x32D2, 0x0C},
    {0x32D4, 0xE7},
    {0x32D5, 0x90},
    {0x32D6, 0x0E},
    {0x32D8, 0x45},
    {0x32D9, 0x11},
    {0x32DA, 0x1F},
    {0x32EB, 0xA4},
    {0x32EC, 0x60},
    {0x32ED, 0x1F},

    {0x3000, 0x00},     // Standby 0: Operating    1: Standby

    {0x3002, 0x00}      // master mode operation 0: Start    1: Stop

#endif

};
#define NUM_OF_IMX238_INIT_REGS (sizeof(imx238_init_regs) / sizeof(sensor_reg_t))

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
    {0x55,  1206}, {0x56,  1248}, {0x57,  1292},  {0x58,  1337},  {0x59,  1384},
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

#include "spi_common.h"
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

//=============================================================================
// I2C
//=============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            0x34 >> 1
#include "i2c_common.c"

//=============================================================================
// internal functions
//============================================================================
static void _delay(void)
{
    int i;
    for (i=0; i<0x1f; i++);
}

static int read_reg(u32 addr, u32 *pval)
{
    u32 id, temp;
    int i;
    struct i2c_msg  msgs[2];
    unsigned char   tmp[2], tmp2[2];

    if (sensor_spi){
        _delay();
        SPI_CS_CLR();
        _delay();

        id = ((addr >> 8) & 0x0f) + 2;
        // address phase
        temp = ((addr & 0xff) << 8) | (0x80 | id);
        for (i=0; i<16; i++) {
            SPI_CLK_CLR();
            _delay();
            if (temp & BIT0)
                SPI_DO_SET();
            else
                SPI_DO_CLR();
            temp >>= 1;
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
    else{
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
}

static int write_reg(u32 addr, u32 val)
{
    u32 id, temp;
    int i;
    struct i2c_msg  msgs;
    unsigned char   buf[3];

    if (sensor_spi){
        _delay();
        SPI_CS_CLR();
        _delay();

        id = ((addr >> 8) & 0x0f) + 2;
        // address phase & data phase
        temp = (val << 16) | ((addr & 0xff) << 8) | id;
        for (i=0; i<24; i++) {
            SPI_CLK_CLR();
            _delay();
            if (temp & BIT0)
                SPI_DO_SET();
            else
                SPI_DO_CLR();
            temp >>= 1;
            SPI_CLK_SET();
            _delay();
        }
        SPI_CS_SET();

        return 0;
    }
    else{
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
}

static u32 get_pclk(u32 xclk)
{
    u32 pclk;

#ifdef sensor_54
    pclk = 54000000;
#endif

#ifdef sensor_37125
    pclk = 74250000;
#endif

    printk("pclk = %d\n", pclk);
    return pclk;
}

static void adjust_vblk(int fps)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h;

    if((fps >= 5) && (fps < 31)){
        write_reg(0x3000, 0x01);// STANDBY on
        _delay();
        pinfo->htp = g_psensor->pclk / fps / pinfo->VMax * 2;
        reg_h = (pinfo->htp >> 8) & 0xFF;
        reg_l = pinfo->htp & 0xFF;
        write_reg(0x301b, reg_l);// HMAX_L
        write_reg(0x301c, reg_h);// HMAX_H
        _delay();
        write_reg(0x3000, 0x00); // STANDBY off
        pinfo->t_row = (pinfo->htp * 5000) / (g_psensor->pclk / 1000);
        g_psensor->min_exp = (pinfo->t_row + 500) / 1000;
    }
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
    g_psensor->out_win.width = 1312;
    g_psensor->out_win.height = 1069;

    g_psensor->img_win.x = (20 + ((1280 - width) >> 1)) &~BIT0;
    g_psensor->img_win.y = (33 + ((1024 - height) >> 1)) &~BIT0;

    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    read_reg(0x301b, &val_l);
    read_reg(0x301c, &val_h);
    pinfo->HMax = (val_h << 8) | val_l;

    read_reg(0x3018, &val_l);
    read_reg(0x3019, &val_h);
    pinfo->VMax = (val_h << 8) | val_l;

    g_psensor->pclk = get_pclk(g_psensor->xclk);
    pinfo->t_row = (pinfo->HMax * 5000) / (g_psensor->pclk / 1000);
    g_psensor->min_exp = (pinfo->t_row + 500) / 1000;

    return ret;
}

static u32 get_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 val_l, val_h;
    u32 lines, vmax, shs1;

    if (!g_psensor->curr_exp) {
        read_reg(0x3018, &val_l);
        read_reg(0x3019, &val_h);
        vmax = (val_h << 8) | val_l;

        read_reg(0x3020, &val_l);
        read_reg(0x3021, &val_h);
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
        write_reg(0x3018, (lines & 0xFF));
        write_reg(0x3019, ((lines >> 8) & 0xFF));
        write_reg(0x3020, 0x0);
        write_reg(0x3021, 0x0);
    } else {
        write_reg(0x3018, (pinfo->VMax & 0xFF));
        write_reg(0x3019, ((pinfo->VMax >> 8) & 0xFF));
        shs1 = pinfo->VMax - lines;
        write_reg(0x3020, (shs1 & 0xFF));
        write_reg(0x3021, ((shs1 >> 8) & 0xFF));
    }

    g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;

    return ret;
}

static u32 get_gain(void)
{
    u32 ad_gain;

    if (g_psensor->curr_gain == 0) {
        read_reg(0x3014, &ad_gain);
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

    write_reg(0x3014, ad_gain);

    g_psensor->curr_gain = gain_table[i-1].gain_x;

    return ret;
}

static int get_mirror(void)
{
    u32 reg;

    read_reg(0x3007, &reg);

    return (reg & BIT1) ? 1 : 0;
}

static int set_mirror(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    read_reg(0x3007, &reg);
    if (enable)
        reg |= BIT1;
    else
        reg &=~BIT1;
    write_reg(0x3007, reg);
    pinfo->mirror = enable;

    return 0;

}

static int get_flip(void)
{
    u32 reg;

    read_reg(0x3007, &reg);

    return (reg & BIT0) ? 1 : 0;
}

static int set_flip(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;

    read_reg(0x3007, &reg);
    if (enable)
        reg |= BIT0;
    else
        reg &=~BIT0;
    write_reg(0x3007, reg);
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
    sensor_reg_t *sensor_init_regs;
    int num_of_init_regs;
    int ret = 0;
    int i;
    u32 id=0xFF;

    if (pinfo->is_init)
        return 0;

    // set initial registers
    //Check Chip ID
    read_reg(0x31CA, &id);

    if(id == ID_IMX138){
        sensor_init_regs = imx138_init_regs;
        num_of_init_regs = NUM_OF_IMX138_INIT_REGS;
        printk("IMX138\n");
        snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, "IMX138");
    }else{        
        sensor_init_regs = imx238_init_regs; 
        num_of_init_regs = NUM_OF_IMX238_INIT_REGS;
         printk("IMX238\n");
        snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, "IMX238");
    }
    
    for (i=0; i<num_of_init_regs; i++) {
        if (write_reg(sensor_init_regs[i].addr, sensor_init_regs[i].val) < 0) {
            isp_err("%s init fail!!", SENSOR_NAME);
            return -EINVAL;
        }        
        mdelay(1);
    }     
    
    if(g_psensor->interface == IF_LVDS){
        isp_info("Sensor Interface : LVDS 4 ch\n");
        write_reg(0x3000, 0x01);// STANDBY on 
        write_reg(0x3002, 0x01);// master mode operation 0: Start    1: Stop  
        write_reg(0x3009, 0x01);// Frame rate setting  0: 120 fps  1: 60 fps  2: 30fps
        write_reg(0x3044, 0xE1);// output bit setting 0: 10bit  1: 12bit     Output system 0: Parallel CMOS  1: Parallel LVDS...
        write_reg(0x3000, 0x00);// Standby 0: Operating    1: Standby
        write_reg(0x3002, 0x00);// master mode operation 0: Start    1: Stop    	
    }

    //g_psensor->pclk = get_pclk(g_psensor->xclk);    

    pinfo->mirror = mirror;
    pinfo->flip = flip;
    set_mirror(pinfo->mirror);
    set_flip(pinfo->flip);

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
static void imx138_deconstruct(void)
{
    if ((g_psensor) && (g_psensor->private)) {
        kfree(g_psensor->private);
    }

    if (sensor_spi)
        isp_api_spi_exit();
    else
        sensor_remove_i2c_driver();

    // turn off EXT_CLK
    isp_api_extclk_onoff(0);
    // release CAP_RST
    isp_api_cap_rst_exit();
}

static int imx138_construct(u32 xclk, u16 width, u16 height)
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
    g_psensor->interface = interface;
    g_psensor->num_of_lane = 4;
    g_psensor->protocol = 0;
    g_psensor->fmt = RAW12;
    g_psensor->bayer_type = BAYER_GB;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_HIGH;
    g_psensor->inv_clk = inv_clk;
    g_psensor->img_win.x = 142;
    g_psensor->img_win.y = 24;
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

    if (sensor_spi){
        if ((ret = isp_api_spi_init()))
            return ret;
    }
    else{
        if ((ret = sensor_init_i2c_driver()) < 0)
            goto construct_err;
    }

    if (g_psensor->interface != IF_LVDS){
        if ((ret = init()) < 0)
            goto construct_err;
        isp_info("IMX1238 in_w=%d, in_h=%d, out_w=%d, out_h=%d\n", g_psensor->img_win.width, g_psensor->img_win.height,
        g_psensor->out_win.width, g_psensor->out_win.height);      
    }
    
    return 0;

construct_err:
    imx138_deconstruct();
    return ret;

}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init imx138_init(void)
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

    if ((ret = imx138_construct(sensor_xclk, sensor_w, sensor_h)) < 0){
        printk("imx138_construct Error \n");
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

static void __exit imx138_exit(void)
{
    isp_unregister_sensor(g_psensor);
    imx138_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(imx138_init);
module_exit(imx138_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor IMX138");
MODULE_LICENSE("GPL");
