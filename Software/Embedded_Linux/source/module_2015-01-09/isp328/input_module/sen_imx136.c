/**
 * @file sen_imx136.c
 * Sony IMX136 sensor driver
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

#define PFX "sen_imx136"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1920
#define DEFAULT_IMAGE_HEIGHT    1080
#define DEFAULT_XCLK            37125000

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

static uint ch_num = 2;
module_param(ch_num, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_num, "channel number");

static uint fps = 30;
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "frame per second");

static uint wdr = 0;
module_param(wdr, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(wdr, "WDR mode");

static ushort sensor_spi = 0;  //0:I2C, 1: SPI
module_param(sensor_spi, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_spi, "sensor SPI interface");

//=============================================================================
// global
//=============================================================================
#define SENSOR_NAME         "IMX136"
#define SENSOR_MAX_WIDTH    1920
#define SENSOR_MAX_HEIGHT   1200

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
static sensor_reg_t sensor_init_regs[] = {
        {0x3002, 0x01},   //Master Stop[0] : 1 as stop
        {DELAY_CODE, 500},
        {0x3000, 0x01},   // Stand by [0] : 1 as standby enable
        {DELAY_CODE, 500},
        {0x3001, 0x00},
        {0x3003, 0x00},
        {0x3004, 0x10},
        {0x3005, 0x01},   //12-bit
        {0x3006, 0x00},
        {0x3007, 0x10},    //0x10 : 1080P
        {0x3008, 0x10},
        {0x3009, 0x02},    //Frame Selection: 1 : 60 fps, 2 : 30fps
        {0x300a, 0x3c},
        {0x300b, 0x00},
        {0x300c, 0x00},
        {0x300d, 0x20},
        {0x300e, 0x01},
        {0x300f, 0x01},
        {0x3010, 0x01},
        {0x3011, 0x00},
        {0x3012, 0xf0},
        {0x3013, 0x00},
        {0x3014, 0x80},
        {0x3015, 0x00},
        {0x3016, 0x08},
        {0x3017, 0x00},

        {0x3018, 0x65},   //VMAX[ 7:0]
        {0x3019, 0x04},   //VMAX[ 15:8]
        {0x301a, 0x00},
        {0x301b, 0x98},
        {0x301c, 0x08},    //37.125M, HMAX= 0x0898, 60 fps

        {0x301d, 0x26},
        {0x301e, 0x02},
        {0x301f, 0x00},
        {0x3020, 0x00},
        {0x3021, 0x00},
        {0x3022, 0x00},
        {0x3038, 0x3c},
        {0x3039, 0x00},
        {0x303a, 0x50},
        {0x303b, 0x04},
        {0x303c, 0x00},
        {0x303d, 0x00},
        {0x303e, 0x9c},
        {0x303f, 0x07},
        {0x3040, 0x00},
        {0x3041, 0x00},
        {0x3042, 0x00},
        {0x3043, 0x00},
        {0x3044, 0xe1},     // [7..4]OPORTSEL: 0xE:SubLVDS 4ch , 0xD:SubLVDS 2ch
        {0x3045, 0x01},

        {0x3054, 0x63},
        {0x305b, 0x00},
        {0x305c, 0x30},
        {0x305d, 0x04},
        {0x305e, 0x30},
        {0x305f, 0x04},
        {0x310f, 0x0E},
        {0x3116, 0x02},
        {0x3236, 0x71},
        {0x3239, 0xF1},
        {0x3241, 0xF2},
        {0x3242, 0x21},
        {0x3243, 0x21},
        {0x3248, 0xF2},
        {0x3249, 0x21},
        {0x324A, 0x21},
        {0x3252, 0x01},
        {0x3254, 0xB1},

        {0x3000, 0x00},   // Stand by [0] : 1 as standby enable
        {DELAY_CODE, 500},
        {0x3002, 0x00},   //Master Stop[0] : 1 as stop
        {DELAY_CODE, 500},
        //Set XVS/XHS output
        {0x3046, 0x00},
        {0x3047, 0x08},
        {0x3048, 0x13},
        {0x3049, 0x0A},
        // Wait stability
        {DELAY_CODE, 500}
};
#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))

typedef struct _gain_set
{
    u16 ad_gain;
    u16 gain_x;     // UFIX7.6
} gain_set_t;

static gain_set_t gain_table[] =
{
    {0x000, 64  }, {0x001, 64  }, {0x002, 65  }, {0x003, 66  }, {0x004, 67  },
    {0x005, 67  }, {0x006, 68  }, {0x007, 69  }, {0x008, 70  }, {0x009, 70  },
    {0x00A, 71  }, {0x00B, 72  }, {0x00C, 73  }, {0x00D, 74  }, {0x00E, 75  },
    {0x00F, 76  }, {0x010, 76  }, {0x011, 77  }, {0x012, 78  }, {0x013, 79  },
    {0x014, 80  }, {0x015, 81  }, {0x016, 82  }, {0x017, 83  }, {0x018, 84  },
    {0x019, 85  }, {0x01A, 86  }, {0x01B, 87  }, {0x01C, 88  }, {0x01D, 89  },
    {0x01E, 90  }, {0x01F, 91  }, {0x020, 92  }, {0x021, 93  }, {0x022, 94  },
    {0x023, 95  }, {0x024, 96  }, {0x025, 97  }, {0x026, 99  }, {0x027, 100 },
    {0x028, 101 }, {0x029, 102 }, {0x02A, 103 }, {0x02B, 104 }, {0x02C, 106 },
    {0x02D, 107 }, {0x02E, 108 }, {0x02F, 109 }, {0x030, 111 }, {0x031, 112 },
    {0x032, 113 }, {0x033, 115 }, {0x034, 116 }, {0x035, 117 }, {0x036, 119 },
    {0x037, 120 }, {0x038, 121 }, {0x039, 123 }, {0x03A, 124 }, {0x03B, 126 },
    {0x03C, 127 }, {0x03D, 129 }, {0x03E, 130 }, {0x03F, 132 }, {0x040, 133 },
    {0x041, 135 }, {0x042, 136 }, {0x043, 138 }, {0x044, 140 }, {0x045, 141 },
    {0x046, 143 }, {0x047, 144 }, {0x048, 146 }, {0x049, 148 }, {0x04A, 150 },
    {0x04B, 151 }, {0x04C, 153 }, {0x04D, 155 }, {0x04E, 157 }, {0x04F, 158 },
    {0x050, 160 }, {0x051, 162 }, {0x052, 164 }, {0x053, 166 }, {0x054, 168 },
    {0x055, 170 }, {0x056, 172 }, {0x057, 174 }, {0x058, 176 }, {0x059, 178 },
    {0x05A, 180 }, {0x05B, 182 }, {0x05C, 184 }, {0x05D, 186 }, {0x05E, 188 },
    {0x05F, 191 }, {0x060, 193 }, {0x061, 195 }, {0x062, 197 }, {0x063, 200 },
    {0x064, 202 }, {0x065, 204 }, {0x066, 207 }, {0x067, 209 }, {0x068, 211 },
    {0x069, 214 }, {0x06A, 216 }, {0x06B, 219 }, {0x06C, 221 }, {0x06D, 224 },
    {0x06E, 227 }, {0x06F, 229 }, {0x070, 232 }, {0x071, 235 }, {0x072, 237 },
    {0x073, 240 }, {0x074, 243 }, {0x075, 246 }, {0x076, 248 }, {0x077, 251 },
    {0x078, 254 }, {0x079, 257 }, {0x07A, 260 }, {0x07B, 263 }, {0x07C, 266 },
    {0x07D, 269 }, {0x07E, 273 }, {0x07F, 276 }, {0x080, 279 }, {0x081, 282 },
    {0x082, 285 }, {0x083, 289 }, {0x084, 292 }, {0x085, 295 }, {0x086, 299 },
    {0x087, 302 }, {0x088, 306 }, {0x089, 309 }, {0x08A, 313 }, {0x08B, 317 },
    {0x08C, 320 }, {0x08D, 324 }, {0x08E, 328 }, {0x08F, 332 }, {0x090, 335 },
    {0x091, 339 }, {0x092, 343 }, {0x093, 347 }, {0x094, 351 }, {0x095, 355 },
    {0x096, 359 }, {0x097, 364 }, {0x098, 368 }, {0x099, 372 }, {0x09A, 376 },
    {0x09B, 381 }, {0x09C, 385 }, {0x09D, 390 }, {0x09E, 394 }, {0x09F, 399 },
    {0x0A0, 403 }, {0x0A1, 408 }, {0x0A2, 413 }, {0x0A3, 418 }, {0x0A4, 422 },
    {0x0A5, 427 }, {0x0A6, 432 }, {0x0A7, 437 }, {0x0A8, 442 }, {0x0A9, 447 },
    {0x0AA, 453 }, {0x0AB, 458 }, {0x0AC, 463 }, {0x0AD, 469 }, {0x0AE, 474 },
    {0x0AF, 479 }, {0x0B0, 485 }, {0x0B1, 491 }, {0x0B2, 496 }, {0x0B3, 502 },
    {0x0B4, 508 }, {0x0B5, 514 }, {0x0B6, 520 }, {0x0B7, 526 }, {0x0B8, 532 },
    {0x0B9, 538 }, {0x0BA, 544 }, {0x0BB, 551 }, {0x0BC, 557 }, {0x0BD, 563 },
    {0x0BE, 570 }, {0x0BF, 577 }, {0x0C0, 583 }, {0x0C1, 590 }, {0x0C2, 597 },
    {0x0C3, 604 }, {0x0C4, 611 }, {0x0C5, 618 }, {0x0C6, 625 }, {0x0C7, 632 },
    {0x0C8, 640 }, {0x0C9, 647 }, {0x0CA, 654 }, {0x0CB, 662 }, {0x0CC, 670 },
    {0x0CD, 677 }, {0x0CE, 685 }, {0x0CF, 693 }, {0x0D0, 701 }, {0x0D1, 709 },
    {0x0D2, 718 }, {0x0D3, 726 }, {0x0D4, 734 }, {0x0D5, 743 }, {0x0D6, 751 },
    {0x0D7, 760 }, {0x0D8, 769 }, {0x0D9, 778 }, {0x0DA, 787 }, {0x0DB, 796 },
    {0x0DC, 805 }, {0x0DD, 815 }, {0x0DE, 824 }, {0x0DF, 834 }, {0x0E0, 843 },
    {0x0E1, 853 }, {0x0E2, 863 }, {0x0E3, 873 }, {0x0E4, 883 }, {0x0E5, 893 },
    {0x0E6, 904 }, {0x0E7, 914 }, {0x0E8, 925 }, {0x0E9, 935 }, {0x0EA, 946 },
    {0x0EB, 957 }, {0x0EC, 968 }, {0x0ED, 979 }, {0x0EE, 991 }, {0x0EF, 1002},
    {0x0F0, 1014}, {0x0F1, 1026}, {0x0F2, 1037}, {0x0F3, 1049}, {0x0F4, 1062},
    {0x0F5, 1074}, {0x0F6, 1086}, {0x0F7, 1099}, {0x0F8, 1112}, {0x0F9, 1125},
    {0x0FA, 1138}, {0x0FB, 1151}, {0x0FC, 1164}, {0x0FD, 1178}, {0x0FE, 1191},
    {0x0FF, 1205}, {0x100, 1219}, {0x101, 1233}, {0x102, 1247}, {0x103, 1262},
    {0x104, 1276}, {0x105, 1291}, {0x106, 1306}, {0x107, 1321}, {0x108, 1337},
    {0x109, 1352}, {0x10A, 1368}, {0x10B, 1384}, {0x10C, 1400}, {0x10D, 1416},
    {0x10E, 1432}, {0x10F, 1449}, {0x110, 1466}, {0x111, 1483}, {0x112, 1500},
    {0x113, 1517}, {0x114, 1535}, {0x115, 1553}, {0x116, 1571}, {0x117, 1589},
    {0x118, 1607}, {0x119, 1626}, {0x11A, 1645}, {0x11B, 1664}, {0x11C, 1683},
    {0x11D, 1702}, {0x11E, 1722}, {0x11F, 1742}, {0x120, 1762}, {0x121, 1783},
    {0x122, 1803}, {0x123, 1824}, {0x124, 1845}, {0x125, 1867}, {0x126, 1888},
    {0x127, 1910}, {0x128, 1932}, {0x129, 1955}, {0x12A, 1977}, {0x12B, 2000},
    {0x12C, 2023}, {0x12D, 2047}, {0x12E, 2070}, {0x12F, 2094}, {0x130, 2119},
    {0x131, 2143}, {0x132, 2168}, {0x133, 2193}, {0x134, 2219}, {0x135, 2244},
    {0x136, 2270}, {0x137, 2297}, {0x138, 2323}, {0x139, 2350}, {0x13A, 2377},
    {0x13B, 2405}, {0x13C, 2433}, {0x13D, 2461}, {0x13E, 2489}, {0x13F, 2518},
    {0x140, 2547}, {0x141, 2577}, {0x142, 2607}, {0x143, 2637}, {0x144, 2667},
    {0x145, 2698}, {0x146, 2730}, {0x147, 2761}, {0x148, 2793}, {0x149, 2826},
    {0x14A, 2858}, {0x14B, 2891}, {0x14C, 2925}, {0x14D, 2959}, {0x14E, 2993},
    {0x14F, 3028}, {0x150, 3063}, {0x151, 3098}, {0x152, 3134}, {0x153, 3170},
    {0x154, 3207}, {0x155, 3244}, {0x156, 3282}, {0x157, 3320}, {0x158, 3358},
    {0x159, 3397}, {0x15A, 3437}, {0x15B, 3476}, {0x15C, 3517}, {0x15D, 3557},
    {0x15E, 3598}, {0x15F, 3640}, {0x160, 3682}, {0x161, 3725}, {0x162, 3768},
    {0x163, 3812}, {0x164, 3856}, {0x165, 3901}, {0x166, 3946}, {0x167, 3991},
    {0x168, 4038}, {0x169, 4084}, {0x16A, 4132}, {0x16B, 4180}, {0x16C, 4228},
    {0x16D, 4277}, {0x16E, 4326}, {0x16F, 4377}, {0x170, 4427}, {0x171, 4478},
    {0x172, 4530}, {0x173, 4583}, {0x174, 4636}, {0x175, 4690}, {0x176, 4744},
    {0x177, 4799}, {0x178, 4854}, {0x179, 4911}, {0x17A, 4967}, {0x17B, 5025},
    {0x17C, 5083}, {0x17D, 5142}, {0x17E, 5202}, {0x17F, 5262}, {0x180, 5323},
    {0x181, 5384}, {0x182, 5447}, {0x183, 5510}, {0x184, 5574}, {0x185, 5638},
    {0x186, 5704}, {0x187, 5770}, {0x188, 5836}, {0x189, 5904}, {0x18A, 5972},
    {0x18B, 6041}, {0x18C, 6111}, {0x18D, 6182}, {0x18E, 6254}, {0x18F, 6326},
    {0x190, 6400}, {0x191, 6474}, {0x192, 6549}, {0x193, 6624}, {0x194, 6701},
    {0x195, 6779}, {0x196, 6857}, {0x197, 6937}, {0x198, 7017}, {0x199, 7098},
    {0x19A, 7180}, {0x19B, 7264}, {0x19C, 7348}, {0x19D, 7433}, {0x19E, 7519},
    {0x19F, 7606}, {0x1A0, 7694}, {0x1A1, 7783}, {0x1A2, 7873}, {0x1A3, 7964},
    {0x1A4, 8057}
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

    if (g_psensor->img_win.height <= 720){
        pclk = 74250000;
    }
    else if (g_psensor->img_win.height <= 1080){
        pclk = 148500000;
    }
    else{
        pclk = 148500000;
    }

    isp_info("IMX136 Sensor: pclk(%d) XCLK(%d)\n", pclk, xclk);
    return pclk;
}

static void adjust_blanking(int fps)
{
	u32 vmax, reg;

    	sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (g_psensor->img_win.height <= 720)
        vmax = 750;
    else if (g_psensor->img_win.height <= 1080)
        vmax = 1125;
    else
        vmax = 1250;                    //1200

    if (g_psensor->img_win.height > 1080){
        if (fps > 54)
            fps = 54;
        if (fps > 27)
            pinfo->VMax = vmax*54/fps;
        else
            pinfo->VMax = vmax*27/fps;
    }
    else{
        if (fps > 60)
            fps = 60;
        if (fps > 30)
            pinfo->VMax = vmax*60/fps;
        else
            pinfo->VMax = vmax*30/fps;
    }
    g_psensor->fps = fps;

    write_reg(0x3018, (pinfo->VMax & 0xFF));
    write_reg(0x3019, ((pinfo->VMax >> 8) & 0xFF));
    write_reg(0x301a, ((pinfo->VMax >> 16) & 0x1));

    read_reg(0x3007, &reg);
    reg &= 0x0f;
    if (g_psensor->img_win.width <= 1280 && g_psensor->img_win.height <= 720) {
        write_reg(0x3007, 0x20 | reg);
        write_reg(0x3009, 0x01);
        write_reg(0x305d, 0x04);
        if (fps > 30){
            write_reg(0x301b, 0x78);
            write_reg(0x301c, 0x06);
            pinfo->HMax = 1656;
        }
        else{
            write_reg(0x301b, 0xe0);
            write_reg(0x301c, 0x0c);
            pinfo->HMax = 3296;
        }
    }
    else if (g_psensor->img_win.width <= 1920 && g_psensor->img_win.height <= 1080) {
        write_reg(0x3007, 0x10 | reg);
        write_reg(0x305d, 0x06);
        if (fps > 30){
            write_reg(0x301b, 0x98);
            write_reg(0x301c, 0x08);
            write_reg(0x3009, 0x01);
            pinfo->HMax = 2200;
        }
        else{
            write_reg(0x301b, 0x30);
            write_reg(0x301c, 0x11);
            write_reg(0x3009, 0x01);
            pinfo->HMax = 4400;
        }
    }
    else{   //1920x1200
        write_reg(0x3007, 0x00 | reg);
        write_reg(0x305d, 0x06);
        if (fps > 27){
            write_reg(0x301b, 0x98);
            write_reg(0x301c, 0x08);
            write_reg(0x3009, 0x01);
            pinfo->HMax = 2200;
        }
        else{
            write_reg(0x301b, 0x30);
            write_reg(0x301c, 0x11);
            write_reg(0x3009, 0x01);
            pinfo->HMax = 4400;
        }
    }
    isp_info("adjust_blanking, fps=%d\n", fps);
}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    // trow unit is 1/10 us
    pinfo->t_row = (pinfo->HMax * 10000) / (g_psensor->pclk / 1000);
    isp_info("t_row=%d\n", pinfo->t_row);
}

static int set_size(u32 width, u32 height)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;
    int ret = 0;
	isp_info("w=%d, h=%d\n", width, height);
    // check size
    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("Exceed maximum sensor resolution!\n");
        return -1;
    }

    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    g_psensor->pclk = get_pclk(g_psensor->xclk);
    adjust_blanking(g_psensor->fps);
    calculate_t_row();

    // update sensor information
    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;


    if (width<=1280 && height<=720) {
        g_psensor->out_win.width = 1312; //0x520
        g_psensor->out_win.height = 740; //0x2e4
        g_psensor->img_win.x = 20 ;
        g_psensor->img_win.y = 15 ;
        isp_info("720P,Sensor VMAX=0x%x(%d) , HMAX=0x%x(%d) \n",
            pinfo->VMax ,pinfo->VMax ,pinfo->HMax,pinfo->HMax );
    }
    else if (width<=1920 && height<=1080 ) {
        g_psensor->out_win.width = 1952; //0x7a0
        g_psensor->out_win.height = 1117; //0x45d
        g_psensor->img_win.x = 20 ;
        g_psensor->img_win.y = 25 ;
        isp_info("1080P,Sensor VMAX=0x%x(%d) , HMAX=0x%x(%d) \n",
            pinfo->VMax ,pinfo->VMax ,pinfo->HMax,pinfo->HMax );
    } else {    //1920x1200
        g_psensor->out_win.width = 1952;
        g_psensor->out_win.height = 1236;
        g_psensor->img_win.x = 20 ;
        g_psensor->img_win.y = 25 ;
        isp_info("WUXGA, Sensor VMAX=0x%x(%d) , HMAX=0x%x(%d) \n",
                pinfo->VMax ,pinfo->VMax ,pinfo->HMax,pinfo->HMax );
    }
    //Get Channel
    read_reg(0x3044, &reg);
    reg=reg >>4;
    switch (reg){
    case 0xe:
            isp_info("Sensor SubLVDS 4 ch \n");
            break;
    case 0xd:
            isp_info("Sensor SubLVDS 2 ch \n");
            break;
    default:
            isp_err("invalid LVDS channel number !\n");
    }
    return ret;
}

static u32 get_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h, reg_msb, shs1;
    u32 num_of_line;

    if (!g_psensor->curr_exp) {
        read_reg(0x3020, &reg_l);
        read_reg(0x3021, &reg_h);
        read_reg(0x3022, &reg_msb);
        shs1 = ((reg_msb & 0x1) << 16) | ((reg_h & 0xFF) << 8) | (reg_l & 0xFF);
        num_of_line = pinfo->VMax - shs1;
        g_psensor->curr_exp = (num_of_line * pinfo->t_row + 500) / 1000;
    }

    return g_psensor->curr_exp;
}

#define WDR_LONG_EXP_LINE_MIN    128
#define WDR_LONG_EXP_LINE_MAX    1120
//#define WDR_LONG_EXP_LINE_MIN    13
//#define WDR_LONG_EXP_LINE_MAX    1120

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 exp_line, shs1, shs2;
    int ret = 0;

    exp_line = MAX(1, (exp * 1000 + pinfo->t_row / 2 ) / pinfo->t_row);

    if (wdr){
        exp_line = MIN(WDR_LONG_EXP_LINE_MAX, MAX(exp_line, WDR_LONG_EXP_LINE_MIN));
        shs2 = pinfo->VMax - exp_line;
        shs1 = MIN(pinfo->VMax -1, pinfo->VMax - exp_line / 16);
        write_reg(0x3023, (shs2 & 0xFF));
        write_reg(0x3024, ((shs2 >> 8) & 0xFF));
        write_reg(0x3025, ((shs2 >> 16) & 0x1));
        write_reg(0x3020, (shs1 & 0xFF));
        write_reg(0x3021, ((shs1 >> 8) & 0xFF));
        write_reg(0x3022, ((shs1 >> 16) & 0x1));
    }
    else{
        if (exp_line <= pinfo->VMax) {
            shs1 = pinfo->VMax - exp_line;
            write_reg(0x3020, (shs1 & 0xFF));
            write_reg(0x3021, ((shs1 >> 8) & 0xFF));
            write_reg(0x3022, ((shs1 >> 16) & 0x1));
            if (pinfo->long_exp) {
                write_reg(0x3018, (pinfo->VMax & 0xFF));
                write_reg(0x3019, ((pinfo->VMax >> 8) & 0xFF));
                write_reg(0x301a, ((pinfo->VMax >> 16) & 0x1));
                pinfo->long_exp = 0;
            }
        }
        else {
            write_reg(0x3020, 0x0);
            write_reg(0x3021, 0x0);
            write_reg(0x3022, 0x0);
            write_reg(0x3018, (exp_line & 0xFF));
            write_reg(0x3019, ((exp_line >> 8) & 0xFF));
            write_reg(0x301a, ((exp_line >> 16) & 0x1));
            pinfo->long_exp = 1;
        }
    }
    g_psensor->curr_exp = MAX(1, (exp_line * pinfo->t_row + 500) / 1000);
    return ret;
}

static u32 get_gain(void)
{
    u32 reg_l, reg_h, i;

    if (!g_psensor->curr_gain) {
        read_reg(0x3014, &reg_l);
        read_reg(0x3015, &reg_h);
        reg_l = ((reg_h & 0x1) << 8) | (reg_l & 0xFF);
        for (i=0; i<NUM_OF_GAINSET; i++) {
            if (gain_table[i].ad_gain == reg_l)
                break;
        }
        if (i < NUM_OF_GAINSET)
            g_psensor->curr_gain = gain_table[i].gain_x;
    }

    return g_psensor->curr_gain;
}

static int set_gain(u32 gain)
{
    u32 reg_l, reg_h;
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

    reg_l = gain_table[i-1].ad_gain & 0xFF;
    reg_h = (gain_table[i-1].ad_gain >> 8) & 0x1;

    write_reg(0x3014, reg_l);
    write_reg(0x3015, reg_h);

    g_psensor->curr_gain = gain_table[i-1].gain_x;
    return ret;
}

static void update_bayer_type(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    if (pinfo->mirror) {
        if (pinfo->flip)
            g_psensor->bayer_type = BAYER_GR;
        else
            g_psensor->bayer_type = BAYER_BG;
    } else {
        if (pinfo->flip)
            g_psensor->bayer_type = BAYER_RG;
        else
            g_psensor->bayer_type = BAYER_GB;
    }
}


static int get_mirror(void)
{
	u32 reg;

	read_reg(0x3007, &reg);

	return reg & 0x02;
}

static int set_mirror(int enable)
{
    	sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
	u32 reg;

	pinfo->mirror = enable;
	read_reg(0x3007, &reg);
	if (enable)
		reg |= 0x02;
	else
		reg &= ~0x02;
	write_reg(0x3007, reg);

    	return 0;
}

static int get_flip(void)
{
	u32 reg;

	read_reg(0x3007, &reg);

	return reg & 0x01;
}

static int set_flip(int enable)
{
    	sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
	u32 reg;

	pinfo->flip = enable;
	read_reg(0x3007, &reg);
	if (enable)
		reg |= 0x01;
	else
		reg &= ~0x01;
	write_reg(0x3007, reg);

    	return 0;
}

static int set_fps(int fps)
{
    	adjust_blanking(fps);
    	g_psensor->fps = fps;

    	//isp_info("pdev->fps=%d\n",pdev->fps);

	return 0;
}

int cal_wdr_min_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int exp = MAX(1, (pinfo->t_row * WDR_LONG_EXP_LINE_MIN + 999) / 1000);
    return exp;
}

int cal_wdr_max_exp(void)
{
   sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
   int exp = MAX(1, pinfo->t_row * WDR_LONG_EXP_LINE_MAX / 1000);
    return exp;
}

static int set_wdr_en(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (enable){
        isp_info("enable wdr\n");
        wdr = 1;
        write_reg(0x300c, 0x02);
//        write_reg(0x300a, 0xf0);
        write_reg(0x300a, 0x3c);
        write_reg(0x300b, 0x00);
        write_reg(0x300f, 0x05);
        write_reg(0x3010, 0x00);
        write_reg(0x3012, 0x2d);

        write_reg(0x3065, 0x00);
        write_reg(0x3084, 0x10);
        write_reg(0x3086, 0x10);
        write_reg(0x30cf, 0xe1);
        write_reg(0x30d0, 0x30);
        write_reg(0x30d2, 0xc4);
        write_reg(0x30d3, 0x01);
        //16 times compression
        if (fps > 30){
            write_reg(0x3056, 0xc9);
//            write_reg(0x3057, 0x33);
            write_reg(0x3057, 0x64);
        }
        else{
            write_reg(0x3056, 0xc9);
            write_reg(0x3057, 0x64);
        }
        g_psensor->fmt = RAW12_WDR16;
        g_psensor->min_exp = cal_wdr_min_exp();
        //g_psensor->max_exp = cal_wdr_max_exp();
//        g_psensor->min_exp = 1;
        g_psensor->max_exp = 5000; // 0.5 sec

        if (pinfo->long_exp) {
            write_reg(0x3018, (pinfo->VMax & 0xFF));
            write_reg(0x3019, ((pinfo->VMax >> 8) & 0xFF));
            write_reg(0x301a, ((pinfo->VMax >> 16) & 0x1));
            pinfo->long_exp = 0;
        }
    }
    else{
        isp_info("disable wdr\n");
        wdr = 0;
        write_reg(0x300c, 0x00);
        write_reg(0x300a, 0x3c);
        write_reg(0x300b, 0x00);
        write_reg(0x300f, 0x01);
        write_reg(0x3010, 0x01);
        write_reg(0x3012, 0xf0);

        write_reg(0x3065, 0x20);
        write_reg(0x3084, 0x0c);
        write_reg(0x3086, 0x01);
        write_reg(0x30cf, 0xd1);
        write_reg(0x30d0, 0x1b);
        write_reg(0x30d2, 0x5f);
        write_reg(0x30d3, 0x00);

        g_psensor->fmt = RAW12;
        g_psensor->min_exp = 1;
        g_psensor->max_exp = 5000; // 0.5 sec
    }

    g_psensor->curr_exp  = MIN(g_psensor->max_exp, MAX(g_psensor->curr_exp, g_psensor->min_exp));
    set_exp(g_psensor->curr_exp);

    g_psensor->curr_gain  = MIN(g_psensor->max_gain, MAX(g_psensor->curr_gain, g_psensor->min_gain));
    set_gain(g_psensor->curr_gain);

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
    case ID_WDR_EN:
        set_wdr_en((int)arg);
        break;
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
    if (pinfo->is_init)
        return 0;

    isp_info("start initial...\n");
    // set initial registers
    for (i=0; i<NUM_OF_INIT_REGS; i++) {

         if (sensor_init_regs[i].addr == DELAY_CODE) {
//            mdelay(pInitTable[i].val);
         }
         else if(write_reg(sensor_init_regs[i].addr, sensor_init_regs[i].val) < 0) {
                isp_err("%s init fail!!", SENSOR_NAME);
                return -EINVAL;
         }
//         mdelay(30);
     }

    if (ch_num == 4)
        write_reg(0x3044, 0xe1);
    else
        write_reg(0x3044, 0xd1);    // 2 ch

    if (g_psensor->img_win.height > 1080) {
        if (fps > 27){
            write_reg(0x3009, 0x01);
            write_reg(0x301b, 0x98);
            write_reg(0x301c, 0x08);    //37.125M, HMAX= 2200, 60 fps
        }
    }
    else{
        if (fps > 30){
            if (g_psensor->img_win.width <= 1280 && g_psensor->img_win.height <= 720) {
                write_reg(0x301b, 0x78);
                write_reg(0x301c, 0x06);
            }
            else{
                write_reg(0x3009, 0x01);
                write_reg(0x301b, 0x98);
                write_reg(0x301c, 0x08);    //37.125M, HMAX= 2200, 60 fps
            }
        }
    }


    pinfo->mirror = mirror;
    pinfo->flip = flip;
    set_mirror(pinfo->mirror);
    set_flip(pinfo->flip);
    update_bayer_type();

    // set image size
    ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height);
    if (ret == 0)
        pinfo->is_init = 1;

    if (wdr)
        set_wdr_en(1);

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
static void imx136_deconstruct(void)
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

static int imx136_construct(u32 xclk, u16 width, u16 height)
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
    g_psensor->capability = SUPPORT_MIRROR | SUPPORT_FLIP | SUPPORT_WDR;
    g_psensor->xclk = xclk;
    g_psensor->fmt = RAW12;
    g_psensor->bayer_type = BAYER_GB;
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
    g_psensor->interface = IF_LVDS;
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

    if (sensor_spi){
        if ((ret = isp_api_spi_init()))
            return ret;
    }
    else{
        if ((ret = sensor_init_i2c_driver()) < 0)
            goto construct_err;
    }

    if ((ret = init()) < 0)
        goto construct_err;

    return 0;

construct_err:
    imx136_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init imx136_init(void)
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

    if ((ret = imx136_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
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

static void __exit imx136_exit(void)
{
    isp_unregister_sensor(g_psensor);
    imx136_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(imx136_init);
module_exit(imx136_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor IMX136");
MODULE_LICENSE("GPL");
