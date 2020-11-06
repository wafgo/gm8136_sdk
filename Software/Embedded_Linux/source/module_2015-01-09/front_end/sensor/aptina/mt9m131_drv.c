/**
 * @file mt9m131_drv.c
 * Aptina MT9M131 1MP Image CMOS Sensor Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.4 $
 * $Date: 2014/07/29 05:36:14 $
 *
 * ChangeLog:
 *  $Log: mt9m131_drv.c,v $
 *  Revision 1.4  2014/07/29 05:36:14  shawn_hu
 *  Add the module parameter of "ibus" to specify which I2C bus has the decoder attached.
 *
 *  Revision 1.3  2013/11/11 05:45:42  jerson_l
 *  1. add ch_id module parameter for specify video channel index
 *
 *  Revision 1.2  2013/10/24 05:57:08  jerson_l
 *  1. support QSXGA and SXGA video mode
 *  2. keep fix frame rate
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
#include "aptina/mt9m131.h"            ///< from module/include/front_end/sensor

#define MT9M131_CLKIN                  27000000

/* device number */
static int dev_num = 1;
module_param(dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dev_num, "Device Number: 1");

/* device i2c bus */
static int ibus = 0;
module_param(ibus, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ibus, "Device I2C Bus: 0~4");

/* device i2c address */
static ushort iaddr[MT9M131_DEV_MAX] = {0xBA, 0x90};
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
static int clk_freq = MT9M131_CLKIN;
module_param(clk_freq, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_freq, "External Clock Frequency(Hz)");

/* device video mode */
static int vmode[MT9M131_DEV_MAX] = {[0 ... (MT9M131_DEV_MAX - 1)] = MT9M131_VMODE_VGA};
module_param_array(vmode, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vmode, "Video Mode: 0:VGA 1:QSXGA 2:SXGA");

/* device init */
static int init = 1;
module_param(init, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init, "device init: 0:no-init  1:init");

/* device use gpio as rstb pin */
static int rstb_used = PLAT_GPIO_ID_X_CAP_RST;
module_param(rstb_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rstb_used, "Use GPIO as RSTB Pin: 0:None, 1:X_CAP_RST");

/* device video channel index */
static int ch_id[MT9M131_DEV_MAX]= {0, 1};
module_param_array(ch_id, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_id, "Video Channel Index");

struct mt9m131_dev_t {
    int                     index;                          ///< device index
    char                    name[16];                       ///< device name
    struct semaphore        lock;                           ///< device locker
    struct miscdevice       miscdev;                        ///< device node for ioctl access
    MT9M131_VMODE_T         vmode;                          ///< device current video output mode
};

static struct i2c_client     *mt9m131_i2c_client = NULL;
static struct mt9m131_dev_t   mt9m131_dev[MT9M131_DEV_MAX];
static struct proc_dir_entry *mt9m131_proc_root[MT9M131_DEV_MAX]  = {[0 ... (MT9M131_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *mt9m131_proc_vmode[MT9M131_DEV_MAX] = {[0 ... (MT9M131_DEV_MAX - 1)] = NULL};

static u16 MT9M131_COMMON_CFG[] = {
    0xF0, 0x0000,   ///< BYTEWISE_ADDR_REG
    0x0D, 0x0009,
    0x0D, 0x0008,
    0x20, 0x0303,   ///< READ_MODE_B, bit0: Mirror rows , bit 1: Mirror columms
    0x21, 0x000C,   ///< READ_MODE_A, scaling 2x
    0x2B, 0x00BE,   ///< GREEN1_GAIN_REG
    0x2C, 0x00FD,   ///< BLUE_GAIN_REG
    0x2E, 0x00BE,   ///< GREEN2_GAIN_REG
    0x30, 0x0C2A,   ///< ROW_NOISE_CONTROL_REG
    0x34, 0xC039,   ///< RESERVED_SENSOR_34

    0xF0, 0x0001,   ///< BYTEWISE_ADDR_REG
    0x05, 0x000C,   ///< APERTURE_GAIN
    0x06, 0x708e,   ///< Mode Control
    0x25, 0x002D,   ///< AWB_SPEED_SATURATION
    0x34, 0x0000,   ///< LUMA_OFFSET
    0x35, 0xFF00,   ///< CLIPPING_LIM_OUT_LUMA
    0x3a, 0x0a00,   ///< Y/CB/CR
    0x80, 0x0007,   ///< LENS_CORRECT_CONTROL
    0x81, 0xE10D,   ///< LENS_ADJ_VERT_RED_0
    0x82, 0xEEF3,   ///< LENS_ADJ_VERT_RED_1_2
    0x83, 0xFFF8,   ///< LENS_ADJ_VERT_RED_3_4
    0x84, 0xF00A,   ///< LENS_ADJ_VERT_GREEN_0
    0x85, 0xF1F4,   ///< LENS_ADJ_VERT_GREEN_1_2
    0x86, 0xFDF9,   ///< LENS_ADJ_VERT_GREEN_3_4
    0x87, 0xF10B,   ///< LENS_ADJ_VERT_BLUE_0
    0x88, 0xF1F4,   ///< LENS_ADJ_VERT_BLUE_1_2
    0x89, 0xFBF9,   ///< LENS_ADJ_VERT_BLUE_3_4
    0x8A, 0xD715,   ///< LENS_ADJ_HORIZ_RED_0
    0x8B, 0xEEEC,   ///< LENS_ADJ_HORIZ_RED_1_2
    0x8C, 0xF7EE,   ///< LENS_ADJ_HORIZ_RED_3_4
    0x8D, 0x00FF,   ///< LENS_ADJ_HORIZ_RED_5
    0x8E, 0xE80F,   ///< LENS_ADJ_HORIZ_GREEN_0
    0x8F, 0xF1F5,   ///< LENS_ADJ_HORIZ_GREEN_1_2
    0x90, 0xF7F2,   ///< LENS_ADJ_HORIZ_GREEN_3_4
    0x91, 0x00FE,   ///< LENS_ADJ_HORIZ_GREEN_5
    0x92, 0xE60E,   ///< LENS_ADJ_HORIZ_BLUE_0
    0x93, 0xF5F5,   ///< LENS_ADJ_HORIZ_BLUE_1_2
    0x94, 0xF7F3,   ///< LENS_ADJ_HORIZ_BLUE_3_4
    0x95, 0x00FE,   ///< LENS_ADJ_HORIZ_BLUE_5
    0x9B, 0x0a00,   ///< FORMAT_OUTPUT_CONTROL2B(8)
    0x3A, 0x0a00,   ///< FORMAT_OUTPUT_CONTROL2A(8)
    0xB6, 0x0E06,   ///< LENS_ADJ_VERT_RED_5_6
    0xB7, 0x311B,   ///< LENS_ADJ_VERT_RED_7_8
    0xB8, 0x0B06,   ///< LENS_ADJ_VERT_GREEN_5_6
    0xB9, 0x2016,   ///< LENS_ADJ_VERT_GREEN_7_8
    0xBA, 0x0A04,   ///< LENS_ADJ_VERT_BLUE_5_6
    0xBB, 0x2212,   ///< LENS_ADJ_VERT_BLUE_7_8
    0xBC, 0x0D09,   ///< LENS_ADJ_HORIZ_RED_6_7
    0xBD, 0x1C18,   ///< LENS_ADJ_HORIZ_RED_8_9
    0xBE, 0x003E,   ///< LENS_ADJ_HORIZ_RED_10
    0xBF, 0x0B08,   ///< LENS_ADJ_HORIZ_GREEN_6_7
    0xC0, 0x1314,   ///< LENS_ADJ_HORIZ_GREEN_8_9
    0xC1, 0x0026,   ///< LENS_ADJ_HORIZ_GREEN_10
    0xC2, 0x0908,   ///< LENS_ADJ_HORIZ_BLUE_6_7
    0xC3, 0x1210,   ///< LENS_ADJ_HORIZ_BLUE_8_9
    0xC4, 0x0026,   ///< LENS_ADJ_HORIZ_BLUE_10

    0xF0, 0x0002,   ///< BYTEWISE_ADDR_REG
    0x20, 0xC814,   ///< (1)LUM_LIMITS_WB_STATS
    0x22, 0xD060,   ///< AWB_RED_LIMIT
    0x23, 0xD840,   ///< AWB_BLUE_LIMIT
    0x24, 0x4000,   ///< (1)MATRIX_ADJ_LIMITS
    0x28, 0xEF05,   ///< AWB_ADVANCED_CONTROL_REG
    0x2B, 0x6818,   ///< (1)H_BOUNDS_AE_WIN_C
    0x2C, 0x6226,   ///< (1)V_BOUNDS_AE_WIN_C
    0x2E, 0x0C50,   ///< AE_PRECISION_TARGET
    0x5C, 0x140F,   ///< SEARCH_FLICKER_60
    0x5D, 0x1914,   ///< SEARCH_FLICKER_50
    0x5E, 0x4962,   ///< RATIO_BASE_REG
    0x5F, 0x7A5B,   ///< RATIO_DELTA_REG
};

static u16 MT9M131_VGA_CFG[]= { /* 640x480 */
    0xF0, 0x0000, 	///< BYTEWISE_ADDR_REG
    0x03, 0x0400, 	///< ROW_WINDOW_SIZE_REG   height=>1024
    0x04, 0x0500, 	///< ROW_WINDOW_SIZE_REG   width =>1280

    0xF0, 0x0001, 	///< BYTEWISE_ADDR_REG
    0xA9, 0x0400, 	///< VERT_ZOOM_RESIZE_A   /* 1024 */
    0xAA, 0x01E0, 	///< VERT_SIZE_RESIZE_A   /* 480  */

    0xF0, 0x0002,   ///< BYTEWISE_ADDR_REG
    0xC8, 0x8800,   ///< CONTEXT_CONTROL, switch to Context A 640x480
    0x37, 0x0040,   ///< AE_GAIN_ZONE_LIM @ 4, keep VGA fixed output 30fps.
};

static u16 MT9M131_QSXGA_CFG[] = { /* 640x512 */
    0xF0, 0x0000, 	///< BYTEWISE_ADDR_REG
    0x03, 0x0400, 	///< ROW_WINDOW_SIZE_REG   height=>1024

    0xF0, 0x0001, 	///< BYTEWISE_ADDR_REG
    0xA9, 0x0400, 	///< VERT_ZOOM_RESIZE_A    /* 1024 */
    0xAA, 0x0200, 	///< VERT_SIZE_RESIZE_A    /* 512  */

    0xF0, 0x0002,   ///< BYTEWISE_ADDR_REG
    0xC8, 0x8800,   ///< CONTEXT_CONTROL, switch to Context A 640x512
    0x37, 0x0040,   ///< AE_GAIN_ZONE_LIM @ 4, keep QSXGA fixed output 30fps.
};

static u16 MT9M131_SXGA_CFG[] = { /* 1280x1024 */
    0xF0, 0x0000, 	///< BYTEWISE_ADDR_REG
    0x03, 0x0400, 	///< ROW_WINDOW_SIZE_REG  height=>1024

    0xF0, 0x0001, 	///< BYTEWISE_ADDR_REG
    0xA3, 0x0400, 	///< VERT_ZOOM_RESIZE_B    /* 1024 */
    0xA4, 0x0400, 	///< VERT_SIZE_RESIZE_B    /* 1024 */

    0xF0, 0x0002,   ///< BYTEWISE_ADDR_REG
    0xC8, 0x9f0b,   ///< CONTEXT_CONTROL, switch to Context B 1280x1024
    0x37, 0x0040,   ///< AE_GAIN_ZONE_LIM @ 7, keep SXGA fixed output 15fps.
};


static int mt9m131_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    mt9m131_i2c_client = client;
    return 0;
}

static int mt9m131_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id mt9m131_i2c_id[] = {
    { "mt9m131_i2c", 0 },
    { }
};

static struct i2c_driver mt9m131_i2c_driver = {
    .driver = {
        .name  = "MT9M131_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = mt9m131_i2c_probe,
    .remove   = mt9m131_i2c_remove,
    .id_table = mt9m131_i2c_id
};

static int mt9m131_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&mt9m131_i2c_driver);
    if(ret < 0) {
        printk("MT9M131 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = iaddr[0]>>1;
    strlcpy(info.type, "mt9m131_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("MT9M131 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("MT9M131 can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&mt9m131_i2c_driver);
   return -ENODEV;
}

static void mt9m131_i2c_unregister(void)
{
    i2c_unregister_device(mt9m131_i2c_client);
    i2c_del_driver(&mt9m131_i2c_driver);
    mt9m131_i2c_client = NULL;
}

static int mt9m131_proc_vmode_show(struct seq_file *sfile, void *v)
{
    int i, vmode;
    struct mt9m131_dev_t *pdev = (struct mt9m131_dev_t *)sfile->private;
    char *vmode_str[] = {"VGA_640x480", "QSXGA_640x512", "SXGA_1280x1024"};

    down(&pdev->lock);

    seq_printf(sfile, "\n[MT9M131#%d]\n", pdev->index);
    for(i=0; i<MT9M131_VMODE_MAX; i++) {
        seq_printf(sfile, "%02d: %s\n", i, vmode_str[i]);
    }
    seq_printf(sfile, "----------------------------------\n");

    vmode = mt9m131_video_get_mode(pdev->index);
    seq_printf(sfile, "Current==> %s\n\n", (vmode >= 0 && vmode < MT9M131_VMODE_MAX) ? vmode_str[vmode] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static int mt9m131_proc_vmode_open(struct inode *inode, struct file *file)
{
    return single_open(file, mt9m131_proc_vmode_show, PDE(inode)->data);
}

static struct file_operations mt9m131_proc_vmode_ops = {
    .owner  = THIS_MODULE,
    .open   = mt9m131_proc_vmode_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void mt9m131_proc_remove(int id)
{
    if(id >= MT9M131_DEV_MAX)
        return;

    if(mt9m131_proc_root[id]) {
        if(mt9m131_proc_vmode[id]) {
            remove_proc_entry(mt9m131_proc_vmode[id]->name, mt9m131_proc_root[id]);
            mt9m131_proc_vmode[id] = NULL;
        }

        remove_proc_entry(mt9m131_proc_root[id]->name, NULL);
        mt9m131_proc_root[id] = NULL;
    }
}

static int mt9m131_proc_init(int id)
{
    int ret = 0;

    if(id >= MT9M131_DEV_MAX)
        return -1;

    /* root */
    mt9m131_proc_root[id] = create_proc_entry(mt9m131_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!mt9m131_proc_root[id]) {
        printk("create proc node '%s' failed!\n", mt9m131_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    mt9m131_proc_root[id]->owner = THIS_MODULE;
#endif

    /* vmode */
    mt9m131_proc_vmode[id] = create_proc_entry("vmode", S_IRUGO|S_IXUGO, mt9m131_proc_root[id]);
    if(!mt9m131_proc_vmode[id]) {
        printk("create proc node '%s/vmode' failed!\n", mt9m131_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    mt9m131_proc_vmode[id]->proc_fops = &mt9m131_proc_vmode_ops;
    mt9m131_proc_vmode[id]->data      = (void *)&mt9m131_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    mt9m131_proc_vmode[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    mt9m131_proc_remove(id);
    return ret;
}

int mt9m131_get_device_num(void)
{
    return dev_num;
}
EXPORT_SYMBOL(mt9m131_get_device_num);

int mt9m131_get_vch_id(int id)
{
    if(id >= dev_num)
        return 0;

    return ch_id[id];
}
EXPORT_SYMBOL(mt9m131_get_vch_id);

int mt9m131_i2c_write(u8 id, u8 reg, u16 data)
{
    u8  buf[3];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return -1;

    if(!mt9m131_i2c_client) {
        printk("MT9M131 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(mt9m131_i2c_client->dev.parent);

    buf[0] = reg;
    buf[1] = (data>>8) & 0xff;  ///< Data is MSB first
    buf[2] = data & 0xff;

    msgs.addr  = iaddr[id]>>1;
    msgs.flags = 0;
    msgs.len   = 3;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("MT9M131#%d i2c write failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(mt9m131_i2c_write);

int mt9m131_i2c_read(u8 id, u8 reg)
{
    u8  buf[1];
    u16 data = 0;
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return 0;

    if(!mt9m131_i2c_client) {
        printk("MT9M131 i2c_client not register!!\n");
        return 0;
    }

    adapter = to_i2c_adapter(mt9m131_i2c_client->dev.parent);

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
        printk("MT9M131#%d i2c read failed!!\n", id);
        return -1;
    }

    return data;
}
EXPORT_SYMBOL(mt9m131_i2c_read);

int mt9m131_video_get_mode(int id)
{
    if(id >= dev_num)
        return MT9M131_VMODE_MAX;

    return mt9m131_dev[id].vmode;
}
EXPORT_SYMBOL(mt9m131_video_get_mode);

int mt9m131_video_set_mode(int id, MT9M131_VMODE_T v_mode)
{
    int i, ret = 0;

    if((id >= dev_num) || (v_mode >= MT9M131_VMODE_MAX))
        return -1;

    down(&mt9m131_dev[id].lock);

    if(v_mode != mt9m131_dev[id].vmode) {
        switch(v_mode) {
            case MT9M131_VMODE_QSXGA:
                for(i=0; i<(ARRAY_SIZE(MT9M131_COMMON_CFG)/2); i++) {
                    ret = mt9m131_i2c_write(id, (u8)MT9M131_COMMON_CFG[i*2], MT9M131_COMMON_CFG[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                for(i=0; i<(ARRAY_SIZE(MT9M131_QSXGA_CFG)/2); i++) {
                    ret = mt9m131_i2c_write(id, (u8)MT9M131_QSXGA_CFG[i*2], MT9M131_QSXGA_CFG[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }
                break;
            case MT9M131_VMODE_SXGA:
                for(i=0; i<(ARRAY_SIZE(MT9M131_COMMON_CFG)/2); i++) {
                    ret = mt9m131_i2c_write(id, (u8)MT9M131_COMMON_CFG[i*2], MT9M131_COMMON_CFG[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                for(i=0; i<(ARRAY_SIZE(MT9M131_SXGA_CFG)/2); i++) {
                    ret = mt9m131_i2c_write(id, (u8)MT9M131_SXGA_CFG[i*2], MT9M131_SXGA_CFG[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }
                break;
            case MT9M131_VMODE_VGA:
            default:
                for(i=0; i<(ARRAY_SIZE(MT9M131_COMMON_CFG)/2); i++) {
                    ret = mt9m131_i2c_write(id, (u8)MT9M131_COMMON_CFG[i*2], MT9M131_COMMON_CFG[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }

                for(i=0; i<(ARRAY_SIZE(MT9M131_VGA_CFG)/2); i++) {
                    ret = mt9m131_i2c_write(id, (u8)MT9M131_VGA_CFG[i*2], MT9M131_VGA_CFG[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }
                break;
        }
        mt9m131_dev[id].vmode = v_mode;
    }

exit:
    up(&mt9m131_dev[id].lock);

    return ret;
}
EXPORT_SYMBOL(mt9m131_video_set_mode);

static void mt9m131_hw_reset(void)
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

static int mt9m131_device_init(int id)
{
    int ret = 0;

    if(id >= MT9M131_DEV_MAX)
        return -1;

    if(!init) {
        mt9m131_dev[id].vmode = vmode[id];
        goto exit;
    }

    ret = mt9m131_video_set_mode(id, vmode[id]);
    if(ret < 0)
        goto exit;

exit:
    printk("MT9M131#%d Init...%s!\n", id, ((ret == 0) ? "OK" : "Fail"));

    return ret;
}

static int __init mt9m131_init(void)
{
    int i, ret;

    /* platfrom check */
    plat_identifier_check();

    /* check device number */
    if(dev_num > MT9M131_DEV_MAX) {
        printk("MT9M131 dev_num=%d invalid!!(Max=%d)\n", dev_num, MT9M131_DEV_MAX);
        return -1;
    }

    /* register i2c client */
    ret = mt9m131_i2c_register();
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
    mt9m131_hw_reset();

    /* MT9M131 init */
    for(i=0; i<dev_num; i++) {
        mt9m131_dev[i].index = i;
        mt9m131_dev[i].vmode = -1;   ///< init value

        sprintf(mt9m131_dev[i].name, "mt9m131.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&mt9m131_dev[i].lock, 1);
#else
        init_MUTEX(&mt9m131_dev[i].lock);
#endif
        ret = mt9m131_proc_init(i);
        if(ret < 0)
            goto err;

        ret = mt9m131_device_init(i);
        if(ret < 0)
            goto err;
    }

    return 0;

err:
    mt9m131_i2c_unregister();

    for(i=0; i<MT9M131_DEV_MAX; i++) {
        if(mt9m131_dev[i].miscdev.name)
            misc_deregister(&mt9m131_dev[i].miscdev);

        mt9m131_proc_remove(i);
    }

    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    return ret;
}

static void __exit mt9m131_exit(void)
{
    int i;

    mt9m131_i2c_unregister();

    for(i=0; i<MT9M131_DEV_MAX; i++) {
        /* remove device node */
        if(mt9m131_dev[i].miscdev.name)
            misc_deregister(&mt9m131_dev[i].miscdev);

        /* remove proc node */
        mt9m131_proc_remove(i);
    }

    /* release gpio pin */
    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);
}

module_init(mt9m131_init);
module_exit(mt9m131_exit);

MODULE_DESCRIPTION("Grain Media MT9M131 CMOS Sensor Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
