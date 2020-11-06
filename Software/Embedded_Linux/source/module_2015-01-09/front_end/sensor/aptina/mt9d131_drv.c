/**
 * @file mt9d131_drv.c
 * Aptina MT9D131 2MP Image CMOS Sensor Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.4 $
 * $Date: 2014/07/29 05:36:14 $
 *
 * ChangeLog:
 *  $Log: mt9d131_drv.c,v $
 *  Revision 1.4  2014/07/29 05:36:14  shawn_hu
 *  Add the module parameter of "ibus" to specify which I2C bus has the decoder attached.
 *
 *  Revision 1.3  2013/11/11 05:45:42  jerson_l
 *  1. add ch_id module parameter for specify video channel index
 *
 *  Revision 1.2  2013/10/24 03:35:18  jerson_l
 *  1. update sensor setup delay time
 *
 *  Revision 1.1.1.1  2013/10/24 02:51:52  jerson_l
 *  add sensor driver
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#include "platform.h"
#include "aptina/mt9d131.h"            ///< from module/include/front_end/sensor

#define MT9D131_CLKIN                  27000000

/* device number */
static int dev_num = 1;
module_param(dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dev_num, "Device Number: 1");

/* device i2c bus */
static int ibus = 0;
module_param(ibus, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ibus, "Device I2C Bus: 0~4");

/* device i2c address */
static ushort iaddr[MT9D131_DEV_MAX] = {0x90};
module_param_array(iaddr, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iaddr, "Device I2C Address");

/* clock port used */
static int clk_used = 0x1;
module_param(clk_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_used, "External Clock Port: BIT0:EXT_CLK0, BIT1:EXT_CLK1");

/* clock source */
static int clk_src = PLAT_EXTCLK_SRC_PLL3;
module_param(clk_src, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_src, "External Clock Source: 0:PLL1OUT1 1:PLL1OUT1/2 2:PLL4OUT2 3:PLL4OUT1/2 4:PLL3");

/* clock frequency */
static int clk_freq = MT9D131_CLKIN;
module_param(clk_freq, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_freq, "External Clock Frequency(Hz)");

/* device video mode */
static int vmode[MT9D131_DEV_MAX] = {[0 ... (MT9D131_DEV_MAX - 1)] = MT9D131_VMODE_HD720};
module_param_array(vmode, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vmode, "Video Mode: 0:SVGA 1:HD720 2:WXGA 3:UXGA");

/* device init */
static int init = 1;
module_param(init, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init, "device init: 0:no-init  1:init");

/* device use gpio as rstb pin */
static int rstb_used = PLAT_GPIO_ID_X_CAP_RST;
module_param(rstb_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rstb_used, "Use GPIO as RSTB Pin: 0:None, 1:X_CAP_RST");

/* device video channel index */
static int ch_id[MT9D131_DEV_MAX]= {0};
module_param_array(ch_id, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_id, "Video Channel Index");

struct mt9d131_dev_t {
    int                     index;                          ///< device index
    char                    name[16];                       ///< device name
    struct semaphore        lock;                           ///< device locker
    struct miscdevice       miscdev;                        ///< device node for ioctl access
    MT9D131_VMODE_T         vmode;                          ///< device current video output mode
};

static struct i2c_client     *mt9d131_i2c_client = NULL;
static struct mt9d131_dev_t    mt9d131_dev[MT9D131_DEV_MAX];
static struct proc_dir_entry *mt9d131_proc_root[MT9D131_DEV_MAX]  = {[0 ... (MT9D131_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *mt9d131_proc_vmode[MT9D131_DEV_MAX] = {[0 ... (MT9D131_DEV_MAX - 1)] = NULL};

static u16 MT9D131_COMMON_CFG[] = {
    0xf0, 0x0000,
    0x65, 0xA000,   ///< CLOCK_ENABLING
    0xf0, 0x0001,
    0xC3, 0x0501,   ///< MCU_BOOT_MODE
    0xf0, 0x0000,
    0x0D, 0x0021,   ///< RESET_REG
    0x0D, 0x0000,   ///< RESET_REG
};

static u16 MT9D131_SVGA_CFG_1[] = {
    0x66, 0x4C0C,   ///< PLL Control 1 = 19468
    0x67, 0x0500,   ///< PLL Control 2 = 1280
    0x65, 0xA000,   ///< Clock CNTRL: PLL ON = 40960
    0x65, 0x2000,   ///< Clock CNTRL: USE PLL = 8192
};

static u16 MT9D131_SVGA_CFG_2[] = {
    0x21, 0x8400,   ///< READ_MODE_A
    0xf0, 0x0001,   ///< page 1
    0xC6, 0x270B,   ///< MCU_ADDRESS [MODE_CONFIG]
    0xC8, 0x0030,   ///< MCU_DATA_0
    0xC6, 0x2774,   ///< MCU_ADDRESS [MODE_FIFO_CONF1_B]
    0xC8, 0x0501,   ///< MCU_DATA_0
    0xC6, 0x2772,   ///< MCU_ADDRESS
    0xC8, 0x0027,   ///< MCU_DATA_0
    0xf0, 0x0000,   ///< page 0
    0x07, 0x00FE,   ///< HORZ_BLANK_A
    0x08, 0x000B,   ///< VERT_BLANK_A
    0x0B, 0x0000,   ///< EXTRA_DELAY_REG  // 0x03BB
    0xf0, 0x0001,   ///< page 1
    0xC6, 0x2703,   ///< MCU_ADDRESS [MODE_OUTPUT_WIDTH_A]
    0xC8, 0x0320,   ///< MCU_DATA_0
    0xC6, 0x2705,   ///< MCU_ADDRESS [MODE_OUTPUT_HEIGHT_A]
    0xC8, 0x0258,   ///< MCU_DATA_0
    0xC6, 0x270F,   ///< MCU_ADDRESS [MODE_SENSOR_ROW_START_A]
    0xC8, 0x001C,   ///< MCU_DATA_0
    0xC6, 0x2711,   ///< MCU_ADDRESS [MODE_SENSOR_COL_START_A]
    0xC8, 0x003C,   ///< MCU_DATA_0
    0xC6, 0x2713,   ///< MCU_ADDRESS [MODE_SENSOR_ROW_HEIGHT_A]
    0xC8, 0x04B0,   ///< MCU_DATA_0
    0xC6, 0x2715,   ///< MCU_ADDRESS [MODE_SENSOR_COL_WIDTH_A]
    0xC8, 0x0640,   ///< MCU_DATA_0
    0xC6, 0x2727,   ///< MCU_ADDRESS [MODE_CROP_X0_A]
    0xC8, 0x0000,   ///< MCU_DATA_0
    0xC6, 0x2729,   ///< MCU_ADDRESS [MODE_CROP_X1_A]
    0xC8, 0x0320,   ///< MCU_DATA_0
    0xC6, 0x272B,   ///< MCU_ADDRESS [MODE_CROP_Y0_A]
    0xC8, 0x0000,   ///< MCU_DATA_0
    0xC6, 0x272D,   ///< MCU_ADDRESS [MODE_CROP_Y1_A]
    0xC8, 0x0258,   ///< MCU_DATA_0
    0xC6, 0xA206,   ///< MCU_ADDRESS [AE_TARGET]
    0xC8, 0x004A,   ///< MCU_DATA_0
    0xC6, 0xA20E,   ///< MCU_ADDRESS [AE_MAX_INDEX]
    0xC8, 0x0004,   ///< MCU_DATA_0
    0xC6, 0x222F,   ///< MCU_ADDRESS [AE_R9_STEP]
    0xC8, 0x009C,   ///< MCU_DATA_0
    0xC6, 0x2411,   ///< MCU_ADDRESS [FD_R9_STEP60]
    0xC8, 0x009C,   ///< MCU_DATA_0
    0xC6, 0x2413,   ///< MCU_ADDRESS [FD_R9_STEP50]
    0xC8, 0x00BC,   ///< MCU_DATA_0
    0xC6, 0xA408,   ///< MCU_ADDRESS [FD_SEARCH_F1_50]
    0xC8, 0x0024,   ///< MCU_DATA_0
    0xC6, 0xA409,   ///< MCU_ADDRESS [FD_SEARCH_F2_50]
    0xC8, 0x0026,   ///< MCU_DATA_0
    0xC6, 0xA40A,   ///< MCU_ADDRESS [FD_SEARCH_F1_60]
    0xC8, 0x001E,   ///< MCU_DATA_0
    0xC6, 0xA40B,   ///< MCU_ADDRESS [FD_SEARCH_F2_60]
    0xC8, 0x0020,   ///< MCU_DATA_0
};

static u16 MT9D131_SVGA_CFG_3[] = {
    0xC6, 0xA103,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x0006,   ///< MCU_DATA_0
};

static u16 MT9D131_SVGA_CFG_4[] = {
    0xC6, 0xA103,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x0005,   ///< MCU_DATA_0
};

static u16 MT9D131_SVGA_CFG_5[] = {
    0xC6, 0xA120,   ///< MCU_ADDRESS [SEQ_CAP_MODE]
    0xC8, 0x0000,   ///< MCU_DATA_0
    0xC6, 0xA103,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x0001,   ///< MCU_DATA_0
};

static u16 MT9D131_SVGA_CFG_6[] = {
    0xf0, 0x0001,   ///< Page 1
    0xC6, 0xA102,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x000f,   ///< MCU_DATA_0
    0xf0, 0x0000,   ///< Page 0
    0x20, 0x0303,   ///< Mirror Row
};

static u16 MT9D131_HD720_CFG_1[] = {
    0x66, 0x4C0C,   ///< PLL_REG
    0x67, 0x0500,   ///< PLL2_REG
    0x65, 0xA000,   ///< CLOCK_ENABLING
    0x65, 0x2000,   ///< CLOCK_ENABLING
};

static u16 MT9D131_HD720_CFG_2[] = {
    0x20, 0x0300,   ///< READ_MODE_B
    0xf0, 0x0001,   ///< page 1
    0xC6, 0x270B,   ///< MCU_ADDRESS [MODE_CONFIG]
    0xC8, 0x0030,   ///< MCU_DATA_0
    0xC6, 0x2774,   ///< MCU_ADDRESS [MODE_FIFO_CONF1_B]
    0xC8, 0x0501,   ///< MCU_DATA_0
    0xC6, 0x2772,   ///< MCU_ADDRESS
    0xC8, 0x0027,   ///< MCU_DATA_0
    0xf0, 0x0000,   ///< page 0
    0x05, 0x011E,   ///< HORZ_BLANK_B
    0x06, 0x0064,   ///< VERT_BLANK_B, david
    0x0B, 0x0000,   ///< EXTRA_DELAY_REG
    0xf0, 0x0001,   ///< page 1
    0xC6, 0x2707,   ///< MCU_ADDRESS [MODE_OUTPUT_WIDTH_B]
    0xC8, 0x0500,   ///< MCU_DATA_0
    0xC6, 0x2709,   ///< MCU_ADDRESS [MODE_OUTPUT_HEIGHT_B]
    0xC8, 0x02D0,   ///< MCU_DATA_0
    0xC6, 0x271B,   ///< MCU_ADDRESS [MODE_SENSOR_ROW_START_B]
    0xC8, 0x001C,   ///< MCU_DATA_0
    0xC6, 0x271D,   ///< MCU_ADDRESS [MODE_SENSOR_COL_START_B]
    0xC8, 0x003C,   ///< MCU_DATA_0
    0xC6, 0x271F,   ///< MCU_ADDRESS [MODE_SENSOR_ROW_HEIGHT_B]
    0xC8, 0x02D0,   ///< MCU_DATA_0
    0xC6, 0x2721,   ///< MCU_ADDRESS [MODE_SENSOR_COL_WIDTH_B]
    0xC8, 0x0500,   ///< MCU_DATA_0
    0xC6, 0x2735,   ///< MCU_ADDRESS [MODE_CROP_X0_B]
    0xC8, 0x0000,   ///< MCU_DATA_0
    0xC6, 0x2737,   ///< MCU_ADDRESS [MODE_CROP_X1_B]
    0xC8, 0x0500,   ///< MCU_DATA_0
    0xC6, 0x2739,   ///< MCU_ADDRESS [MODE_CROP_Y0_B]
    0xC8, 0x0000,   ///< MCU_DATA_0
    0xC6, 0x273B,   ///< MCU_ADDRESS [MODE_CROP_Y1_B]
    0xC8, 0x02D0,   ///< MCU_DATA_0
    0xC6, 0xA206,   ///< MCU_ADDRESS [AE_TARGET]
    0xC8, 0x004A,   ///< MCU_DATA_0
    0xC6, 0xA20E,   ///< MCU_ADDRESS [AE_MAX_INDEX]
    0xC8, 0x0004,   ///< MCU_DATA_0  //0x04 --> 0x02
    0xC6, 0x222F,   ///< MCU_ADDRESS [AE_R9_STEP]
    0xC8, 0x00D3,   ///< MCU_DATA_0, david
    0xC6, 0x2411,   ///< MCU_ADDRESS [FD_R9_STEP60]
    0xC8, 0x00D3,   ///< MCU_DATA_0, david
    0xC6, 0x2413,   ///< MCU_ADDRESS [FD_R9_STEP50]
    0xC8, 0x00FD,   ///< MCU_DATA_0, david
    0xC6, 0xA408,   ///< MCU_ADDRESS [FD_SEARCH_F1_50]
    0xC8, 0x0031,   ///< MCU_DATA_0
    0xC6, 0xA409,   ///< MCU_ADDRESS [FD_SEARCH_F2_50]
    0xC8, 0x0033,   ///< MCU_DATA_0
    0xC6, 0xA40A,   ///< MCU_ADDRESS [FD_SEARCH_F1_60]
    0xC8, 0x0029,   ///< MCU_DATA_0
    0xC6, 0xA40B,   ///< MCU_ADDRESS [FD_SEARCH_F2_60]
    0xC8, 0x002B,   ///< MCU_DATA_0
};

static u16 MT9D131_HD720_CFG_3[] = {
    0xC6, 0xA103,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x0006,   ///< MCU_DATA_0
};

static u16 MT9D131_HD720_CFG_4[] = {
    0xC6, 0xA103,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x0005,   ///< MCU_DATA_0
};

static u16 MT9D131_HD720_CFG_5[] = {
    0xC6, 0xA120,   ///< MCU_ADDRESS [SEQ_CAP_MODE]
    0xC8, 0x0002,   ///< MCU_DATA_0
    0xC6, 0xA103,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x0002,   ///< MCU_DATA_0
};

static u16 MT9D131_HD720_CFG_6[] = {
    0xf0, 0x0001,   ///< Page 1
    0xC6, 0xA102,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x000f,   ///< MCU_DATA_0
    0xf0, 0x0000,   ///< Page 0
    0x20, 0x0303,   ///< Mirror Row
};

static u16 MT9D131_WXGA_CFG_1[] = {
    0x66, 0x4C0C,   ///< PLL Control 1 = 19468
    0x67, 0x0500,   ///< PLL Control 2 = 1280
    0x65, 0xA000,   ///< Clock CNTRL: PLL ON = 40960
    0x65, 0x2000,   ///< Clock CNTRL: USE PLL = 8192
};

static u16 MT9D131_WXGA_CFG_2[] = {
    0x20, 0x0300,   ///< READ_MODE_B
    0xf0, 0x0001,   ///< page 1
    0xC6, 0x270B,   ///< MCU_ADDRESS [MODE_CONFIG]
    0xC8, 0x0030,   ///< MCU_DATA_0
    0xC6, 0x2774,   ///< MCU_ADDRESS [MODE_FIFO_CONF1_B]
    0xC8, 0x0501,   ///< MCU_DATA_0
    0xC6, 0x2772,   ///< MCU_ADDRESS
    0xC8, 0x0027,   ///< MCU_DATA_0
    0xf0, 0x0000,   ///< page 0
    0x05, 0x011E,   ///< HORZ_BLANK_B
    0x06, 0x0014,   ///< VERT_BLANK_B
    0x0B, 0x0000,   ///< EXTRA_DELAY_REG
    0xf0, 0x0001,   ///< page 1
    0xC6, 0x2707,   ///< MCU_ADDRESS [MODE_OUTPUT_WIDTH_B]
    0xC8, 0x0500,   ///< MCU_DATA_0
    0xC6, 0x2709,   ///< MCU_ADDRESS [MODE_OUTPUT_HEIGHT_B]
    0xC8, 0x0320,   ///< MCU_DATA_0
    0xC6, 0x271B,   ///< MCU_ADDRESS [MODE_SENSOR_ROW_START_B]
    0xC8, 0x001C,   ///< MCU_DATA_0
    0xC6, 0x271D,   ///< MCU_ADDRESS [MODE_SENSOR_COL_START_B]
    0xC8, 0x003C,   ///< MCU_DATA_0
    0xC6, 0x271F,   ///< MCU_ADDRESS [MODE_SENSOR_ROW_HEIGHT_B]
    0xC8, 0x0320,   ///< MCU_DATA_0
    0xC6, 0x2721,   ///< MCU_ADDRESS [MODE_SENSOR_COL_WIDTH_B]
    0xC8, 0x0500,   ///< MCU_DATA_0
    0xC6, 0x2735,   ///< MCU_ADDRESS [MODE_CROP_X0_B]
    0xC8, 0x0000,   ///< MCU_DATA_0
    0xC6, 0x2737,   ///< MCU_ADDRESS [MODE_CROP_X1_B]
    0xC8, 0x0500,   ///< MCU_DATA_0
    0xC6, 0x2739,   ///< MCU_ADDRESS [MODE_CROP_Y0_B]
    0xC8, 0x0000,   ///< MCU_DATA_0
    0xC6, 0x273B,   ///< MCU_ADDRESS [MODE_CROP_Y1_B]
    0xC8, 0x0320,   ///< MCU_DATA_0
    0xC6, 0xA206,   ///< MCU_ADDRESS [AE_TARGET]
    0xC8, 0x004A,   ///< MCU_DATA_0
    0xC6, 0xA20E,   ///< MCU_ADDRESS [AE_MAX_INDEX]
    0xC8, 0x0004,   ///< MCU_DATA_0
    0xC6, 0x222F,   ///< MCU_ADDRESS [AE_R9_STEP]
    0xC8, 0x00D3,   ///< MCU_DATA_0
    0xC6, 0x2411,   ///< MCU_ADDRESS [FD_R9_STEP60]
    0xC8, 0x00D3,   ///< MCU_DATA_0
    0xC6, 0x2413,   ///< MCU_ADDRESS [FD_R9_STEP50]
    0xC8, 0x00FD,   ///< MCU_DATA_0
    0xC6, 0xA408,   ///< MCU_ADDRESS [FD_SEARCH_F1_50]
    0xC8, 0x0031,   ///< MCU_DATA_0
    0xC6, 0xA409,   ///< MCU_ADDRESS [FD_SEARCH_F2_50]
    0xC8, 0x0033,   ///< MCU_DATA_0
    0xC6, 0xA40A,   ///< MCU_ADDRESS [FD_SEARCH_F1_60]
    0xC8, 0x0029,   ///< MCU_DATA_0
    0xC6, 0xA40B,   ///< MCU_ADDRESS [FD_SEARCH_F2_60]
    0xC8, 0x002B,   ///< MCU_DATA_0
};

static u16 MT9D131_WXGA_CFG_3[] = {
    0xC6, 0xA103,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x0006,   ///< MCU_DATA_0
};

static u16 MT9D131_WXGA_CFG_4[] = {
    0xC6, 0xA103,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x0005,   ///< MCU_DATA_0
};

static u16 MT9D131_WXGA_CFG_5[] = {
    0xC6, 0xA120,   ///< MCU_ADDRESS [SEQ_CAP_MODE]
    0xC8, 0x0002,   ///< MCU_DATA_0
    0xC6, 0xA103,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x0002,   ///< MCU_DATA_0
};

static u16 MT9D131_WXGA_CFG_6[] = {
    0xf0, 0x0001,   ///< Page 1
    0xC6, 0xA102,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x000f,   ///< MCU_DATA_0
    0xf0, 0x0000,   ///< Page 0
    0x20, 0x0303,   ///< Mirror Row
};

static u16 MT9D131_UXGA_CFG_1[] = {
    0x66, 0x2E08,   ///< PLL_REG
    0x67, 0x0500,   ///< PLL2_REG
    0x65, 0xA000,   ///< CLOCK_ENABLING
    0x65, 0x2000,   ///< CLOCK_ENABLING
};

static u16 MT9D131_UXGA_CFG_2[] = {
    0x20, 0x0300,   ///< READ_MODE_B
    0xf0, 0x0001,   ///< page 1
    0xC6, 0x270B,   ///< MCU_ADDRESS [MODE_CONFIG]
    0xC8, 0x0030,   ///< MCU_DATA_0
    0xC6, 0x2774,   ///< MCU_ADDRESS [MODE_FIFO_CONF1_B]
    0xC8, 0x0501,   ///< MCU_DATA_0
    0xC6, 0x2772,   ///< MCU_ADDRESS
    0xC8, 0x0027,   ///< MCU_DATA_0
    0xf0, 0x0000,   ///< page 0
    0x05, 0x011E,   ///< HORZ_BLANK_B
    0x06, 0x000B,   ///< VERT_BLANK_B
    0x0B, 0x0000,   ///< EXTRA_DELAY_REG
    0xf0, 0x0001,   ///< page 1
    0xC6, 0x2707,   ///< MCU_ADDRESS [MODE_OUTPUT_WIDTH_B]
    0xC8, 0x0640,   ///< MCU_DATA_0
    0xC6, 0x2709,   ///< MCU_ADDRESS [MODE_OUTPUT_HEIGHT_B]
    0xC8, 0x04B0,   ///< MCU_DATA_0
    0xC6, 0x271B,   ///< MCU_ADDRESS [MODE_SENSOR_ROW_START_B]
    0xC8, 0x001C,   ///< MCU_DATA_0
    0xC6, 0x271D,   ///< MCU_ADDRESS [MODE_SENSOR_COL_START_B]
    0xC8, 0x003C,   ///< MCU_DATA_0
    0xC6, 0x271F,   ///< MCU_ADDRESS [MODE_SENSOR_ROW_HEIGHT_B]
    0xC8, 0x04B0,   ///< MCU_DATA_0
    0xC6, 0x2721,   ///< MCU_ADDRESS [MODE_SENSOR_COL_WIDTH_B]
    0xC8, 0x0640,   ///< MCU_DATA_0
    0xC6, 0x2735,   ///< MCU_ADDRESS [MODE_CROP_X0_B]
    0xC8, 0x0000,   ///< MCU_DATA_0
    0xC6, 0x2737,   ///< MCU_ADDRESS [MODE_CROP_X1_B]
    0xC8, 0x0640,   ///< MCU_DATA_0
    0xC6, 0x2739,   ///< MCU_ADDRESS [MODE_CROP_Y0_B]
    0xC8, 0x0000,   ///< MCU_DATA_0
    0xC6, 0x273B,   ///< MCU_ADDRESS [MODE_CROP_Y1_B]
    0xC8, 0x04B0,   ///< MCU_DATA_0
    0xC6, 0xA206,   ///< MCU_ADDRESS [AE_TARGET]
    0xC8, 0x004A,   ///< MCU_DATA_0
    0xC6, 0xA20E,   ///< MCU_ADDRESS [AE_MAX_INDEX]
    0xC8, 0x0018,   ///< MCU_DATA_0
    0xC6, 0x222F,   ///< MCU_ADDRESS [AE_R9_STEP]
    0xC8, 0x0097,   ///< MCU_DATA_0
    0xC6, 0x2411,   ///< MCU_ADDRESS [FD_R9_STEP60]
    0xC8, 0x0097,   ///< MCU_DATA_0
    0xC6, 0x2413,   ///< MCU_ADDRESS [FD_R9_STEP50]
    0xC8, 0x00B6,   ///< MCU_DATA_0
    0xC6, 0xA408,   ///< MCU_ADDRESS [FD_SEARCH_F1_50]
    0xC8, 0x0023,   ///< MCU_DATA_0
    0xC6, 0xA409,   ///< MCU_ADDRESS [FD_SEARCH_F2_50]
    0xC8, 0x0025,   ///< MCU_DATA_0
    0xC6, 0xA40A,   ///< MCU_ADDRESS [FD_SEARCH_F1_60]
    0xC8, 0x001D,   ///< MCU_DATA_0
    0xC6, 0xA40B,   ///< MCU_ADDRESS [FD_SEARCH_F2_60]
    0xC8, 0x001F,   ///< MCU_DATA_0
};

static u16 MT9D131_UXGA_CFG_3[] = {
    0xC6, 0xA103,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x0006,   ///< MCU_DATA_0
};

static u16 MT9D131_UXGA_CFG_4[] = {
    0xC6, 0xA103,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x0005,   ///< MCU_DATA_0
};

static u16 MT9D131_UXGA_CFG_5[] = {
    0xC6, 0xA120,   ///< MCU_ADDRESS [SEQ_CAP_MODE]
    0xC8, 0x0002,   ///< MCU_DATA_0
    0xC6, 0xA103,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x0002,   ///< MCU_DATA_0
};

static u16 MT9D131_UXGA_CFG_6[] = {
    0xf0, 0x0001,   ///< Page 1
    0xC6, 0xA102,   ///< MCU_ADDRESS [SEQ_CMD]
    0xC8, 0x000f,   ///< MCU_DATA_0
    0xf0, 0x0000,   ///< Page 0
    0x20, 0x0303,   ///< Mirror Row
};

static int mt9d131_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    mt9d131_i2c_client = client;
    return 0;
}

static int mt9d131_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id mt9d131_i2c_id[] = {
    { "mt9d131_i2c", 0 },
    { }
};

static struct i2c_driver mt9d131_i2c_driver = {
    .driver = {
        .name  = "MT9D131_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = mt9d131_i2c_probe,
    .remove   = mt9d131_i2c_remove,
    .id_table = mt9d131_i2c_id
};

static int mt9d131_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&mt9d131_i2c_driver);
    if(ret < 0) {
        printk("MT9D131 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = iaddr[0]>>1;
    strlcpy(info.type, "mt9d131_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("MT9D131 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("MT9D131 can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&mt9d131_i2c_driver);
   return -ENODEV;
}

static void mt9d131_i2c_unregister(void)
{
    i2c_unregister_device(mt9d131_i2c_client);
    i2c_del_driver(&mt9d131_i2c_driver);
    mt9d131_i2c_client = NULL;
}

static int mt9d131_proc_vmode_show(struct seq_file *sfile, void *v)
{
    int i, vmode;
    struct mt9d131_dev_t *pdev = (struct mt9d131_dev_t *)sfile->private;
    char *vmode_str[] = {"SVGA_800x600", "HD720_1280x720", "WXGA_1280x800", "UXGA_1600x1200"};

    down(&pdev->lock);

    seq_printf(sfile, "\n[MT9D131#%d]\n", pdev->index);
    for(i=0; i<MT9D131_VMODE_MAX; i++) {
        seq_printf(sfile, "%02d: %s\n", i, vmode_str[i]);
    }
    seq_printf(sfile, "----------------------------------\n");

    vmode = mt9d131_video_get_mode(pdev->index);
    seq_printf(sfile, "Current==> %s\n\n", (vmode >= 0 && vmode < MT9D131_VMODE_MAX) ? vmode_str[vmode] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static int mt9d131_proc_vmode_open(struct inode *inode, struct file *file)
{
    return single_open(file, mt9d131_proc_vmode_show, PDE(inode)->data);
}

static struct file_operations mt9d131_proc_vmode_ops = {
    .owner  = THIS_MODULE,
    .open   = mt9d131_proc_vmode_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void mt9d131_proc_remove(int id)
{
    if(id >= MT9D131_DEV_MAX)
        return;

    if(mt9d131_proc_root[id]) {
        if(mt9d131_proc_vmode[id]) {
            remove_proc_entry(mt9d131_proc_vmode[id]->name, mt9d131_proc_root[id]);
            mt9d131_proc_vmode[id] = NULL;
        }

        remove_proc_entry(mt9d131_proc_root[id]->name, NULL);
        mt9d131_proc_root[id] = NULL;
    }
}

static int mt9d131_proc_init(int id)
{
    int ret = 0;

    if(id >= MT9D131_DEV_MAX)
        return -1;

    /* root */
    mt9d131_proc_root[id] = create_proc_entry(mt9d131_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!mt9d131_proc_root[id]) {
        printk("create proc node '%s' failed!\n", mt9d131_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    mt9d131_proc_root[id]->owner = THIS_MODULE;
#endif

    /* vmode */
    mt9d131_proc_vmode[id] = create_proc_entry("vmode", S_IRUGO|S_IXUGO, mt9d131_proc_root[id]);
    if(!mt9d131_proc_vmode[id]) {
        printk("create proc node '%s/vmode' failed!\n", mt9d131_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    mt9d131_proc_vmode[id]->proc_fops = &mt9d131_proc_vmode_ops;
    mt9d131_proc_vmode[id]->data      = (void *)&mt9d131_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    mt9d131_proc_vmode[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    mt9d131_proc_remove(id);
    return ret;
}

int mt9d131_get_device_num(void)
{
    return dev_num;
}
EXPORT_SYMBOL(mt9d131_get_device_num);

int mt9d131_get_vch_id(int id)
{
    if(id >= dev_num)
        return 0;

    return ch_id[id];
}
EXPORT_SYMBOL(mt9d131_get_vch_id);

int mt9d131_i2c_write(u8 id, u8 reg, u16 data)
{
    u8  buf[3];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return -1;

    if(!mt9d131_i2c_client) {
        printk("MT9D131 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(mt9d131_i2c_client->dev.parent);

    buf[0] = reg;
    buf[1] = (data>>8) & 0xff;
    buf[2] = data & 0xff;

    msgs.addr  = iaddr[id]>>1;
    msgs.flags = 0;
    msgs.len   = 3;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("MT9D131#%d i2c write failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(mt9d131_i2c_write);

int mt9d131_i2c_read(u8 id, u8 reg)
{
    u8  buf[1];
    u16 data = 0;
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return 0;

    if(!mt9d131_i2c_client) {
        printk("MT9D131 i2c_client not register!!\n");
        return 0;
    }

    adapter = to_i2c_adapter(mt9d131_i2c_client->dev.parent);

    buf[0] = reg;

    msgs[0].addr  = iaddr[id]>>1;
    msgs[0].flags = 0; /* write */
    msgs[0].len   = 1;
    msgs[0].buf   = buf;

    msgs[1].addr  = (iaddr[id] + 1)>>1;
    msgs[1].flags = 1; /* read */
    msgs[1].len   = 2;
    msgs[1].buf   = (u8 *)&data;

    if(i2c_transfer(adapter, msgs, 2) != 2) {
        printk("MT9D131#%d i2c read failed!!\n", id);
        return -1;
    }

    data = ((data&0xff)<<8) | ((data>>8)&0xff); ///< swap LSB and MSB

    return data;
}
EXPORT_SYMBOL(mt9d131_i2c_read);

int mt9d131_video_get_mode(int id)
{
    if(id >= dev_num)
        return MT9D131_VMODE_MAX;

    return mt9d131_dev[id].vmode;
}
EXPORT_SYMBOL(mt9d131_video_get_mode);

int mt9d131_video_set_mode(int id, MT9D131_VMODE_T v_mode)
{
    int i, ret = 0;
    int delay_ms = 50;

    if((id >= dev_num) || (v_mode >= MT9D131_VMODE_MAX))
        return -1;

    down(&mt9d131_dev[id].lock);

    if(v_mode != mt9d131_dev[id].vmode) {
        switch(v_mode) {
            case MT9D131_VMODE_SVGA:
                for(i=0; i<(ARRAY_SIZE(MT9D131_COMMON_CFG)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_COMMON_CFG[i*2], MT9D131_COMMON_CFG[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms);

                for(i=0; i<(ARRAY_SIZE(MT9D131_SVGA_CFG_1)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_SVGA_CFG_1[i*2], MT9D131_SVGA_CFG_1[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms);

                for(i=0; i<(ARRAY_SIZE(MT9D131_SVGA_CFG_2)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_SVGA_CFG_2[i*2], MT9D131_SVGA_CFG_2[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms);

                for(i=0; i<(ARRAY_SIZE(MT9D131_SVGA_CFG_3)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_SVGA_CFG_3[i*2], MT9D131_SVGA_CFG_3[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms*2);

                for(i=0; i<(ARRAY_SIZE(MT9D131_SVGA_CFG_4)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_SVGA_CFG_4[i*2], MT9D131_SVGA_CFG_4[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms*2);

                for(i=0; i<(ARRAY_SIZE(MT9D131_SVGA_CFG_5)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_SVGA_CFG_5[i*2], MT9D131_SVGA_CFG_5[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms*2);

                for(i=0; i<(ARRAY_SIZE(MT9D131_SVGA_CFG_6)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_SVGA_CFG_6[i*2], MT9D131_SVGA_CFG_6[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }
                break;
            case MT9D131_VMODE_HD720:
                for(i=0; i<(ARRAY_SIZE(MT9D131_COMMON_CFG)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_COMMON_CFG[i*2], MT9D131_COMMON_CFG[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms);

                for(i=0; i<(ARRAY_SIZE(MT9D131_HD720_CFG_1)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_HD720_CFG_1[i*2], MT9D131_HD720_CFG_1[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms);

                for(i=0; i<(ARRAY_SIZE(MT9D131_HD720_CFG_2)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_HD720_CFG_2[i*2], MT9D131_HD720_CFG_2[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms);

                for(i=0; i<(ARRAY_SIZE(MT9D131_HD720_CFG_3)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_HD720_CFG_3[i*2], MT9D131_HD720_CFG_3[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms*2);

                for(i=0; i<(ARRAY_SIZE(MT9D131_HD720_CFG_4)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_HD720_CFG_4[i*2], MT9D131_HD720_CFG_4[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms*2);

                for(i=0; i<(ARRAY_SIZE(MT9D131_HD720_CFG_5)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_HD720_CFG_5[i*2], MT9D131_HD720_CFG_5[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms*2);

                for(i=0; i<(ARRAY_SIZE(MT9D131_HD720_CFG_6)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_HD720_CFG_6[i*2], MT9D131_HD720_CFG_6[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }
                break;
            case MT9D131_VMODE_WXGA:
                for(i=0; i<(ARRAY_SIZE(MT9D131_COMMON_CFG)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_COMMON_CFG[i*2], MT9D131_COMMON_CFG[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms);

                for(i=0; i<(ARRAY_SIZE(MT9D131_WXGA_CFG_1)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_WXGA_CFG_1[i*2], MT9D131_WXGA_CFG_1[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms);

                for(i=0; i<(ARRAY_SIZE(MT9D131_WXGA_CFG_2)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_WXGA_CFG_2[i*2], MT9D131_WXGA_CFG_2[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms);

                for(i=0; i<(ARRAY_SIZE(MT9D131_WXGA_CFG_3)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_WXGA_CFG_3[i*2], MT9D131_WXGA_CFG_3[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms*2);

                for(i=0; i<(ARRAY_SIZE(MT9D131_WXGA_CFG_4)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_WXGA_CFG_4[i*2], MT9D131_WXGA_CFG_4[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms*2);

                for(i=0; i<(ARRAY_SIZE(MT9D131_WXGA_CFG_5)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_WXGA_CFG_5[i*2], MT9D131_WXGA_CFG_5[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms*2);

                for(i=0; i<(ARRAY_SIZE(MT9D131_WXGA_CFG_6)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_WXGA_CFG_6[i*2], MT9D131_WXGA_CFG_6[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }
                break;
            case MT9D131_VMODE_UXGA:
                for(i=0; i<(ARRAY_SIZE(MT9D131_COMMON_CFG)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_COMMON_CFG[i*2], MT9D131_COMMON_CFG[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(30);
                }

                mdelay(delay_ms);

                for(i=0; i<(ARRAY_SIZE(MT9D131_UXGA_CFG_1)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_UXGA_CFG_1[i*2], MT9D131_UXGA_CFG_1[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms);

                for(i=0; i<(ARRAY_SIZE(MT9D131_UXGA_CFG_2)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_UXGA_CFG_2[i*2], MT9D131_UXGA_CFG_2[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms);

                for(i=0; i<(ARRAY_SIZE(MT9D131_UXGA_CFG_3)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_UXGA_CFG_3[i*2], MT9D131_UXGA_CFG_3[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms*2);

                for(i=0; i<(ARRAY_SIZE(MT9D131_UXGA_CFG_4)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_UXGA_CFG_4[i*2], MT9D131_UXGA_CFG_4[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms*2);

                for(i=0; i<(ARRAY_SIZE(MT9D131_UXGA_CFG_5)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_UXGA_CFG_5[i*2], MT9D131_UXGA_CFG_5[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                mdelay(delay_ms*2);

                for(i=0; i<(ARRAY_SIZE(MT9D131_UXGA_CFG_6)/2); i++) {
                    ret = mt9d131_i2c_write(id, (u8)MT9D131_UXGA_CFG_6[i*2], MT9D131_UXGA_CFG_6[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }
                break;
            default:
                break;
        }
        mt9d131_dev[id].vmode = v_mode;
    }

exit:
    up(&mt9d131_dev[id].lock);

    return ret;
}
EXPORT_SYMBOL(mt9d131_video_set_mode);

static void mt9d131_hw_reset(void)
{
    if(!rstb_used || !init)
        return;

    /* Active Low */
    plat_gpio_set_value(rstb_used, 0);

    udelay(10);

    /* Active High and release reset */
    plat_gpio_set_value(rstb_used, 1);

    udelay(10);
}

static int mt9d131_device_init(int id)
{
    int ret = 0;

    if(id >= MT9D131_DEV_MAX)
        return -1;

    if(!init) {
        mt9d131_dev[id].vmode = vmode[id];
        goto exit;
    }

    ret = mt9d131_video_set_mode(id, vmode[id]);
    if(ret < 0)
        goto exit;

exit:
    printk("MT9D131#%d Init...%s!\n", id, ((ret == 0) ? "OK" : "Fail"));

    return ret;
}

static int __init mt9d131_init(void)
{
    int i, ret;

    /* platfrom check */
    plat_identifier_check();

    /* check device number */
    if(dev_num > MT9D131_DEV_MAX) {
        printk("MT9D131 dev_num=%d invalid!!(Max=%d)\n", dev_num, MT9D131_DEV_MAX);
        return -1;
    }

    /* register i2c client */
    ret = mt9d131_i2c_register();
    if(ret < 0)
        return -1;

    /* register gpio pin for device rstb pin */
    if(rstb_used && init) {
        ret = plat_register_gpio_pin(rstb_used, PLAT_GPIO_DIRECTION_OUT, 0);
        if(ret < 0)
            goto err;
    }

    /* setup platfrom external clock */
    if((init == 1) && (clk_used >= 0)) {
        if(clk_used) {
            ret = plat_extclk_set_src(clk_src);
            if(ret < 0)
                goto err;

            if(clk_used & 0x1) {
                ret = plat_extclk_set_freq(0, clk_freq);
                if(ret < 0)
                    goto err;
            }

            if(clk_used & 0x2) {
                ret = plat_extclk_set_freq(1, clk_freq);
                if(ret < 0)
                    goto err;
            }

            ret = plat_extclk_onoff(1);
            if(ret < 0)
                goto err;

        }
        else {
            /* use external oscillator. disable SoC external clock output */
            ret = plat_extclk_onoff(0);
            if(ret < 0)
                goto err;
        }
    }

    /* hardware reset for all device */
    mt9d131_hw_reset();

    /* MT9D131 init */
    for(i=0; i<dev_num; i++) {
        mt9d131_dev[i].index = i;
        mt9d131_dev[i].vmode = -1;   ///< init value

        sprintf(mt9d131_dev[i].name, "mt9d131.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&mt9d131_dev[i].lock, 1);
#else
        init_MUTEX(&mt9d131_dev[i].lock);
#endif
        ret = mt9d131_proc_init(i);
        if(ret < 0)
            goto err;

        ret = mt9d131_device_init(i);
        if(ret < 0)
            goto err;
    }

    return 0;

err:
    mt9d131_i2c_unregister();

    for(i=0; i<MT9D131_DEV_MAX; i++) {
        if(mt9d131_dev[i].miscdev.name)
            misc_deregister(&mt9d131_dev[i].miscdev);

        mt9d131_proc_remove(i);
    }

    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    return ret;
}

static void __exit mt9d131_exit(void)
{
    int i;

    mt9d131_i2c_unregister();

    for(i=0; i<MT9D131_DEV_MAX; i++) {
        /* remove device node */
        if(mt9d131_dev[i].miscdev.name)
            misc_deregister(&mt9d131_dev[i].miscdev);

        /* remove proc node */
        mt9d131_proc_remove(i);
    }

    /* release gpio pin */
    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);
}

module_init(mt9d131_init);
module_exit(mt9d131_exit);

MODULE_DESCRIPTION("Grain Media MT9D131 CMOS Sensor Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
