/**
 * @file mv4101_drv.c
 * Magnision MV4101 Quad-Channel SDI Receiver Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.5 $
 * $Date: 2014/07/29 05:36:14 $
 *
 * ChangeLog:
 *  $Log: mv4101_drv.c,v $
 *  Revision 1.5  2014/07/29 05:36:14  shawn_hu
 *  Add the module parameter of "ibus" to specify which I2C bus has the decoder attached.
 *
 *  Revision 1.4  2014/02/24 13:24:00  jerson_l
 *  1. enable notify option as default
 *
 *  Revision 1.3  2013/12/02 05:32:59  jerson_l
 *  1. support vout select from different sdi input source
 *
 *  Revision 1.2  2013/11/27 05:33:01  jerson_l
 *  1. support notify procedure
 *
 *  Revision 1.1.1.1  2013/10/16 07:15:25  jerson_l
 *  add magnision mv4101 sdi driver
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
#include "magnision/mv4101.h"           ///< from module/include/front_end/sdi

#define MV4101_CLKIN                    27000000
#define CH_IDENTIFY(id, sdi, vout)      (((id)&0xf)|(((sdi)&0xf)<<4)|(((vout)&0xf)<<8))
#define CH_IDENTIFY_ID(x)               ((x)&0xf)
#define CH_IDENTIFY_SDI(x)              (((x)>>4)&0xf)
#define CH_IDENTIFY_VOUT(x)             (((x)>>8)&0xf)

/* device number */
static int dev_num = 1;
module_param(dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dev_num, "Device Number: 1~4");

/* device i2c bus */
static int ibus = 0;
module_param(ibus, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ibus, "Device I2C Bus: 0~4");

/* device i2c address */
static ushort iaddr[MV4101_DEV_MAX] = {0x30, 0x32, 0x34, 0x36};
module_param_array(iaddr, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iaddr, "Device I2C Address");

/* clock port used */
static int clk_used = 0x3;
module_param(clk_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_used, "External Clock Port: BIT0:EXT_CLK0, BIT1:EXT_CLK1");

/* clock source */
static int clk_src = PLAT_EXTCLK_SRC_PLL3;
module_param(clk_src, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_src, "External Clock Source: 0:PLL1OUT1 1:PLL1OUT1/2 2:PLL4OUT2 3:PLL4OUT1/2 4:PLL3");

/* clock frequency */
static int clk_freq = MV4101_CLKIN;   ///< 27MHz
module_param(clk_freq, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_freq, "External Clock Frequency(Hz)");

static int vout_format[MV4101_DEV_MAX] = {[0 ... (MV4101_DEV_MAX - 1)] = MV4101_VOUT_FMT_BT656};
module_param_array(vout_format, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vout_format, "Video Output Format: 0:BT656, 1:BT1120");

static int eq_mode[MV4101_DEV_MAX] = {[0 ... (MV4101_DEV_MAX - 1)] = MV4101_EQ_MODE_HW_AUTO};
module_param_array(eq_mode, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(eq_mode, "EQ Operation Mode");

/* device init */
static int init = 1;
module_param(init, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init, "device init: 0:no-init  1:init");

/* device use gpio as rstb pin */
static int rstb_used = PLAT_GPIO_ID_X_CAP_RST;
module_param(rstb_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rstb_used, "Use GPIO as RSTB Pin: 0:None, 1:X_CAP_RST");

/* device use gpio as spi pin */
static int spi_used[MV4101_DEV_MAX] = {[0 ... (MV4101_DEV_MAX - 1)] = 0};
module_param_array(spi_used, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(spi_used, "Use GPIO as SPI Pin: 0:None, 1:GPIO#47_48_51_52, 2:GPIO#27_28_49_50, 3:GPIO#49_50_27_28, 4:GPIO#25_26_27_28");

/* device channel mapping */
static int ch_map = 0;
module_param(ch_map, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_map, "channel mapping");

/* device notify */
static int notify = 1;
module_param(notify, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(notify, "Device Notify => 0:Disable 1:Enable");

/******************************************************************************
 GM8210/GM8287 EVB Channel and SDI Mapping Table (Socket Board, GM8210 System Board)

 VCH#0 ------> SDI#1.0 ---------> VOUT#1.0 -------> X_CAP#7
 VCH#1 ------> SDI#1.1 ---------> VOUT#1.1 -------> X_CAP#6
 VCH#2 ------> SDI#1.2 ---------> VOUT#1.2 -------> X_CAP#5
 VCH#3 ------> SDI#1.3 ---------> VOUT#1.3 -------> X_CAP#4
 VCH#4 ------> SDI#0.0 ---------> VOUT#0.0 -------> X_CAP#3
 VCH#5 ------> SDI#0.1 ---------> VOUT#0.1 -------> X_CAP#2
 VCH#6 ------> SDI#0.2 ---------> VOUT#0.2 -------> X_CAP#1
 VCH#7 ------> SDI#0.3 ---------> VOUT#0.3 -------> X_CAP#0

 ==============================================================================
 GM8287 EVB Channel and SDI Mapping Table (System Board)

 VCH#0 ------> SDI#0.0 ---------> VOUT#0.0 -------> X_CAP#0
 VCH#1 ------> SDI#0.1 ---------> VOUT#0.1 -------> X_CAP#1
 VCH#2 ------> SDI#0.2 ---------> VOUT#0.2 -------> X_CAP#2
 VCH#3 ------> SDI#0.3 ---------> VOUT#0.3 -------> X_CAP#3

*******************************************************************************/

struct mv4101_spi_pin_t {
    int init;
    int sclk;
    int cs;
    int sdin;
    int sdout;
};

struct mv4101_dev_t {
    int                     index;                          ///< device index
    char                    name[16];                       ///< device name
    struct semaphore        lock;                           ///< device locker
    struct miscdevice       miscdev;                        ///< device node for ioctl access
    struct mv4101_spi_pin_t spi_pin;                        ///< device spi gpio pin number
    int                     eq_mode[MV4101_DEV_CH_MAX];     ///< device channel current EQ mode
    int                     pre_plugin[MV4101_DEV_CH_MAX];  ///< device channel previous cable plugin status
    int                     cur_plugin[MV4101_DEV_CH_MAX];  ///< device channel current  cable plugin status

    int                     (*vfmt_notify)(int id, int ch, struct mv4101_video_fmt_t *vfmt);
    int                     (*vlos_notify)(int id, int ch, int vlos);
};

static struct i2c_client  *mv4101_i2c_client = NULL;
static struct task_struct *mv4101_wd = NULL;
static u32 MV4101_VCH_MAP[MV4101_DEV_MAX*MV4101_DEV_CH_MAX];

static struct mv4101_dev_t    mv4101_dev[MV4101_DEV_MAX];
static struct proc_dir_entry *mv4101_proc_root[MV4101_DEV_MAX]      = {[0 ... (MV4101_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *mv4101_proc_reg_read[MV4101_DEV_MAX]  = {[0 ... (MV4101_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *mv4101_proc_reg_write[MV4101_DEV_MAX] = {[0 ... (MV4101_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *mv4101_proc_status[MV4101_DEV_MAX]    = {[0 ... (MV4101_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *mv4101_proc_video_fmt[MV4101_DEV_MAX] = {[0 ... (MV4101_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *mv4101_proc_vout_sdi[MV4101_DEV_MAX]  = {[0 ... (MV4101_DEV_MAX - 1)] = NULL};

static u16 mv4101_reg_read_addr[MV4101_DEV_MAX]  = {[0 ... (MV4101_DEV_MAX - 1)] = 0};
static int mv4101_reg_read_count[MV4101_DEV_MAX] = {[0 ... (MV4101_DEV_MAX - 1)] = 0};

static struct mv4101_video_fmt_t mv4101_video_fmt_info[MV4101_VFMT_MAX] = {
    /* IDX  Active_Width     Active_Height   Total_W     Total_H     Pixel_Rate(HZ)  Bit_Width   Prog    Frame_Rate  Bit_Rate */
    {  0,   0,               0,              0,          0,          0,              0,          0,      0,          0       },
    {  1,   1920,            1080,           2200,       1125,       74250,          10,         0,      60,         1       },
    {  2,   1920,            1080,           2200,       1125,       74250,          10,         0,      60,         1       },
    {  3,   1920,            1080,           2640,       1125,       74250,          10,         0,      50,         1       },
    {  4,   1920,            1080,           2200,       1125,       74250,          10,         1,      30,         1       },
    {  5,   1920,            1080,           2200,       1125,       74250,          10,         1,      30,         1       },
    {  6,   1920,            1080,           2640,       1125,       74250,          10,         1,      25,         1       },
    {  7,   1920,            1080,           2750,       1125,       74250,          10,         1,      24,         1       },
    {  8,   1920,            1080,           2750,       1125,       74250,          10,         1,      24,         1       },
    {  9,   1280,            720,            1650,       750,        74250,          10,         1,      60,         1       },
    { 10,   1280,            720,            1650,       750,        74250,          10,         1,      60,         1       },
    { 11,   1280,            720,            1980,       750,        74250,          10,         1,      50,         1       },
    { 12,   1280,            720,            3300,       750,        74250,          10,         1,      30,         1       },
    { 13,   1280,            720,            3300,       750,        74250,          10,         1,      30,         1       },
    { 14,   1280,            720,            3960,       750,        74250,          10,         1,      25,         1       },
    { 15,   1280,            720,            4125,       750,        74250,          10,         1,      24,         1       },
    { 16,   1280,            720,            4125,       750,        74250,          10,         1,      24,         1       },
};

int mv4101_get_device_num(void)
{
    return dev_num;
}
EXPORT_SYMBOL(mv4101_get_device_num);

int mv4101_notify_vfmt_register(int id, int (*nt_func)(int, int, struct mv4101_video_fmt_t *))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&mv4101_dev[id].lock);

    mv4101_dev[id].vfmt_notify = nt_func;

    up(&mv4101_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(mv4101_notify_vfmt_register);

void mv4101_notify_vfmt_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&mv4101_dev[id].lock);

    mv4101_dev[id].vfmt_notify = NULL;

    up(&mv4101_dev[id].lock);
}
EXPORT_SYMBOL(mv4101_notify_vfmt_deregister);

int mv4101_notify_vlos_register(int id, int (*nt_func)(int, int, int))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&mv4101_dev[id].lock);

    mv4101_dev[id].vlos_notify = nt_func;

    up(&mv4101_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(mv4101_notify_vlos_register);

void mv4101_notify_vlos_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&mv4101_dev[id].lock);

    mv4101_dev[id].vlos_notify = NULL;

    up(&mv4101_dev[id].lock);
}
EXPORT_SYMBOL(mv4101_notify_vlos_deregister);

u32 mv4101_reg_read(int id, unsigned int reg)
{
    u16 read_data = 0;

    if(id >= dev_num)
        return 0;

    if(spi_used[id] == 0) { ///< use I2C interface
        u8 buf[2], data[2];
        struct i2c_msg msgs[2];
        struct i2c_adapter *adapter;

        if(!mv4101_i2c_client) {
            printk("MV4101 i2c_client not register!!\n");
            return -1;
        }

        adapter = to_i2c_adapter(mv4101_i2c_client->dev.parent);

        buf[0] = (reg>>8) & 0xff;   ///< Sub-Address is MSB first
        buf[1] = (reg & 0xff);

        msgs[0].addr  = iaddr[id]>>1;
        msgs[0].flags = 0; /* write */
        msgs[0].len   = 2;
        msgs[0].buf   = buf;

        msgs[1].addr  = (iaddr[id] + 1)>>1;
        msgs[1].flags = 1; /* read */
        msgs[1].len   = 2;
        msgs[1].buf   = (u8 *)data;

        if(i2c_transfer(adapter, msgs, 2) != 2) {
            printk("MV4101#%d i2c read failed!!\n", id);
        }

        read_data = (data[0]<<8) | data[1];
    }
    else {
        /*
         * prepare 16 bit command word output data format
         * (MSB)R/W RSV RSV RSV A11 A10 A9 A8 A7 A6 A5 A4 A3 A2 A1 A0
         *
         * [R/W]=> 0:write, 1:read
         *
         * 16 bit data word input data format
         * (MSB)D15 D14 D13 D12 D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0
         *
         */
        int i;
        u8  bit_val;

        /*active CS to low */
        plat_gpio_set_value(mv4101_dev[id].spi_pin.cs, 0);

        /* Write Address */
        for(i=15; i>=0; i--) {
            if(i == 15)
                bit_val = 1;
            else if(i >= 12)
                bit_val = 0;
            else
                bit_val = (reg>>i) & 0x01;

            /* active SCLK to low */
            plat_gpio_set_value(mv4101_dev[id].spi_pin.sclk, 0);

            /* set output data */
            plat_gpio_set_value(mv4101_dev[id].spi_pin.sdin, bit_val);

            ndelay(3);

            /* active SCLK to high to latch data */
            plat_gpio_set_value(mv4101_dev[id].spi_pin.sclk, 1);

            ndelay(3);
        }

        /* active SCLK to low */
        plat_gpio_set_value(mv4101_dev[id].spi_pin.sclk, 0);

        ndelay(150);

        /* Read Data */
        for(i=15; i>=0; i--) {
            /* active SCLK to high */
            plat_gpio_set_value(mv4101_dev[id].spi_pin.sclk, 1);

            if(plat_gpio_get_value(mv4101_dev[id].spi_pin.sdout))
                read_data |= (0x1<<i);

            ndelay(3);

            /* active SCLK to low */
            plat_gpio_set_value(mv4101_dev[id].spi_pin.sclk, 0);

            ndelay(3);
        }

        /* active SCLK to low */
        plat_gpio_set_value(mv4101_dev[id].spi_pin.sclk, 0);

        ndelay(40);

        /* active CS to high */
        plat_gpio_set_value(mv4101_dev[id].spi_pin.cs, 1);
    }

    return read_data;
}
EXPORT_SYMBOL(mv4101_reg_read);

int mv4101_reg_write(int id, unsigned int reg, unsigned int data)
{
    if(id >= dev_num)
        return 0;

    if(spi_used[id] == 0) { ///< use I2C interface
        u8  buf[4];
        struct i2c_msg msgs;
        struct i2c_adapter *adapter;

        if(!mv4101_i2c_client) {
            printk("MV4101 i2c_client not register!!\n");
            return -1;
        }

        adapter = to_i2c_adapter(mv4101_i2c_client->dev.parent);

        buf[0] = (reg>>8) & 0xff;   ///< Sub-Address is MSB first
        buf[1] = (reg & 0xff);
        buf[2] = (data>>8) & 0xff;  ///< Data is MSB first
        buf[3] = data & 0xff;

        msgs.addr  = iaddr[id]>>1;
        msgs.flags = 0;
        msgs.len   = 4;
        msgs.buf   = buf;

        if(i2c_transfer(adapter, &msgs, 1) != 1) {
            printk("MV4101#%d i2c write failed!!\n", id);
            return -1;
        }
    }
    else {
        int i;
        u8  bit_val;

        /*
         * prepare 16 bit command word output data format
         * (MSB)R/W RSV RSV RSV A11 A10 A9 A8 A7 A6 A5 A4 A3 A2 A1 A0
         *
         * [R/W]=> 0:write, 1:read
         *
         * prepare 16 bit data word output data format
         * (MSB)D15 D14 D13 D12 D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0
         *
         */

        /*active CS to low */
        plat_gpio_set_value(mv4101_dev[id].spi_pin.cs, 0);

        /* Write Address */
        for(i=15; i>=0; i--) {
            if(i >= 12)
                bit_val = 0;
            else
                bit_val = (reg>>i) & 0x01;

            /* active SCLK to low */
            plat_gpio_set_value(mv4101_dev[id].spi_pin.sclk, 0);

            /* set output data */
            plat_gpio_set_value(mv4101_dev[id].spi_pin.sdin, bit_val);

            ndelay(3);

            /* active SCLK to high to latch data */
            plat_gpio_set_value(mv4101_dev[id].spi_pin.sclk, 1);

            ndelay(3);
        }

        /* active SCLK to low */
        plat_gpio_set_value(mv4101_dev[id].spi_pin.sclk, 0);

        ndelay(40);

        /* Write Data */
        for(i=15; i>=0; i--) {
            bit_val = (data>>i) & 0x01;

            /* active SCLK to low */
            plat_gpio_set_value(mv4101_dev[id].spi_pin.sclk, 0);

            /* set output data */
            plat_gpio_set_value(mv4101_dev[id].spi_pin.sdin, bit_val);

            ndelay(3);

            /* active SCLK to high to latch data */
            plat_gpio_set_value(mv4101_dev[id].spi_pin.sclk, 1);

            ndelay(3);
        }

        /* active SCLK to low */
        plat_gpio_set_value(mv4101_dev[id].spi_pin.sclk, 0);

        ndelay(40);

        /* active CS to high */
        plat_gpio_set_value(mv4101_dev[id].spi_pin.cs, 1);
    }

    return 0;
}
EXPORT_SYMBOL(mv4101_reg_write);

static int mv4101_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    mv4101_i2c_client = client;
    return 0;
}

static int mv4101_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id mv4101_i2c_id[] = {
    { "mv4101_i2c", 0 },
    { }
};

static struct i2c_driver mv4101_i2c_driver = {
    .driver = {
        .name  = "MV4101_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = mv4101_i2c_probe,
    .remove   = mv4101_i2c_remove,
    .id_table = mv4101_i2c_id
};

static int mv4101_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&mv4101_i2c_driver);
    if(ret < 0) {
        printk("MV4101 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = iaddr[0]>>1;
    strlcpy(info.type, "mv4101_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("MV4101 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("MV4101 can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&mv4101_i2c_driver);
   return -ENODEV;
}

static void mv4101_i2c_unregister(void)
{
    i2c_unregister_device(mv4101_i2c_client);
    i2c_del_driver(&mv4101_i2c_driver);
    mv4101_i2c_client = NULL;
}

static void mv4101_deregister_spi_gpio_pin(int id)
{
    if(id >= dev_num)
        return;

    if(init && spi_used[id]) {
        /* free SCLK */
        if(mv4101_dev[id].spi_pin.sclk != PLAT_GPIO_ID_NONE) {
            plat_deregister_gpio_pin(mv4101_dev[id].spi_pin.sclk);
            mv4101_dev[id].spi_pin.sclk = PLAT_GPIO_ID_NONE;
        }

        /* free CS */
        if(mv4101_dev[id].spi_pin.cs != PLAT_GPIO_ID_NONE) {
            plat_deregister_gpio_pin(mv4101_dev[id].spi_pin.cs);
            mv4101_dev[id].spi_pin.cs = PLAT_GPIO_ID_NONE;
        }

        /* free SDIN */
        if(mv4101_dev[id].spi_pin.sdin != PLAT_GPIO_ID_NONE) {
            plat_deregister_gpio_pin(mv4101_dev[id].spi_pin.sdin);
            mv4101_dev[id].spi_pin.sdin = PLAT_GPIO_ID_NONE;
        }

        /* free SDOUT */
        if(mv4101_dev[id].spi_pin.sdout != PLAT_GPIO_ID_NONE) {
            plat_deregister_gpio_pin(mv4101_dev[id].spi_pin.sdout);
            mv4101_dev[id].spi_pin.sdout = PLAT_GPIO_ID_NONE;
        }
    }
    else {
        mv4101_dev[id].spi_pin.sclk  = PLAT_GPIO_ID_NONE;
        mv4101_dev[id].spi_pin.cs    = PLAT_GPIO_ID_NONE;
        mv4101_dev[id].spi_pin.sdin  = PLAT_GPIO_ID_NONE;
        mv4101_dev[id].spi_pin.sdout = PLAT_GPIO_ID_NONE;
    }
}

static int mv4101_register_spi_gpio_pin(int id)
{
    int ret = 0;
    int sclk_pin  = PLAT_GPIO_ID_NONE;
    int cs_pin    = PLAT_GPIO_ID_NONE;
    int sdin_pin  = PLAT_GPIO_ID_NONE;
    int sdout_pin = PLAT_GPIO_ID_NONE;

    if(id >= dev_num)
        return -1;

    switch(spi_used[id]) {
        case 0: /* None */
            break;
        case 1: /* GPIO#47,48,51,52 */
            sclk_pin  = PLAT_GPIO_ID_47;
            cs_pin    = PLAT_GPIO_ID_48;
            sdin_pin  = PLAT_GPIO_ID_51;
            sdout_pin = PLAT_GPIO_ID_52;
            break;
        case 2: /* GPIO#27,28,49,50 */
            sclk_pin  = PLAT_GPIO_ID_27;
            cs_pin    = PLAT_GPIO_ID_28;
            sdin_pin  = PLAT_GPIO_ID_49;
            sdout_pin = PLAT_GPIO_ID_50;
            break;
        case 3: /* GPIO#49,50,27,28 */
            sclk_pin  = PLAT_GPIO_ID_49;
            cs_pin    = PLAT_GPIO_ID_50;
            sdin_pin  = PLAT_GPIO_ID_27;
            sdout_pin = PLAT_GPIO_ID_28;
            break;
        case 4: /* GPIO#25,26,27,28 */
            sclk_pin  = PLAT_GPIO_ID_25;
            cs_pin    = PLAT_GPIO_ID_26;
            sdin_pin  = PLAT_GPIO_ID_27;
            sdout_pin = PLAT_GPIO_ID_28;
            break;
        default:
            printk("MV4101#%d spi gpio pin init failed(unknown spi_used=%d)!!\n", id, spi_used[id]);
            ret = -1;
            goto err;
    }

    if(init && spi_used[id]) {
        /* SCLK Config direction to output and active low */
        ret = plat_register_gpio_pin(sclk_pin, PLAT_GPIO_DIRECTION_OUT, 0);
        if(ret < 0)
            goto err;
        mv4101_dev[id].spi_pin.sclk = sclk_pin;

        /* CS Config direction to output and active high */
        ret = plat_register_gpio_pin(cs_pin, PLAT_GPIO_DIRECTION_OUT, 1);
        if(ret < 0)
            goto err;
        mv4101_dev[id].spi_pin.cs = cs_pin;

        /* SDIN Config direction to output and active low */
        ret = plat_register_gpio_pin(sdin_pin, PLAT_GPIO_DIRECTION_OUT, 0);
        if(ret < 0)
            goto err;
        mv4101_dev[id].spi_pin.sdin = sdin_pin;

        /* SDOUT Config direction to input */
        ret = plat_register_gpio_pin(sdout_pin, PLAT_GPIO_DIRECTION_IN, 0);
        if(ret < 0)
            goto err;
        mv4101_dev[id].spi_pin.sdout = sdout_pin;
    }
    else {
        mv4101_dev[id].spi_pin.sclk  = sclk_pin;
        mv4101_dev[id].spi_pin.cs    = cs_pin;
        mv4101_dev[id].spi_pin.sdin  = sdin_pin;
        mv4101_dev[id].spi_pin.sdout = sdout_pin;
    }

    return ret;

err:
    mv4101_deregister_spi_gpio_pin(id);
    return ret;
}

static int mv4101_create_ch_map(int map_id)
{
    int i;

    switch(map_id) {
        case 0: ///< for GM8210/GM8287 Socket Board, GM8210 System Board
            for(i=0; i<dev_num; i++) {
                MV4101_VCH_MAP[(i*MV4101_DEV_CH_MAX)+0] = CH_IDENTIFY(dev_num-i-1, 0, MV4101_DEV_VOUT_0);
                MV4101_VCH_MAP[(i*MV4101_DEV_CH_MAX)+1] = CH_IDENTIFY(dev_num-i-1, 1, MV4101_DEV_VOUT_1);
                MV4101_VCH_MAP[(i*MV4101_DEV_CH_MAX)+2] = CH_IDENTIFY(dev_num-i-1, 2, MV4101_DEV_VOUT_2);
                MV4101_VCH_MAP[(i*MV4101_DEV_CH_MAX)+3] = CH_IDENTIFY(dev_num-i-1, 3, MV4101_DEV_VOUT_3);
            }
            break;

        case 1: ///< for GM8287 System Board
        default:
            for(i=0; i<dev_num; i++) {
                MV4101_VCH_MAP[(i*MV4101_DEV_CH_MAX)+0] = CH_IDENTIFY(i, 0, MV4101_DEV_VOUT_0);
                MV4101_VCH_MAP[(i*MV4101_DEV_CH_MAX)+1] = CH_IDENTIFY(i, 1, MV4101_DEV_VOUT_1);
                MV4101_VCH_MAP[(i*MV4101_DEV_CH_MAX)+2] = CH_IDENTIFY(i, 2, MV4101_DEV_VOUT_2);
                MV4101_VCH_MAP[(i*MV4101_DEV_CH_MAX)+3] = CH_IDENTIFY(i, 3, MV4101_DEV_VOUT_3);
            }
            break;
    }

    return 0;
}

int mv4101_get_vout_format(int id)
{
    if(id >= dev_num)
        return MV4101_VOUT_FMT_BT656;
    else
        return vout_format[id];
}
EXPORT_SYMBOL(mv4101_get_vout_format);

int mv4101_get_vch_id(int id, MV4101_DEV_VOUT_T vout, int sdi_ch)
{
    int i;

    if(id >= MV4101_DEV_MAX || vout >= MV4101_DEV_VOUT_MAX || sdi_ch >= MV4101_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*MV4101_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(MV4101_VCH_MAP[i]) == id) &&
           (CH_IDENTIFY_VOUT(MV4101_VCH_MAP[i]) == vout) &&
           (CH_IDENTIFY_SDI(MV4101_VCH_MAP[i]) == sdi_ch)) {
            return i;
        }
    }

    return 0;
}
EXPORT_SYMBOL(mv4101_get_vch_id);

static ssize_t mv4101_proc_reg_read_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int r_cnt;
    u32 addr;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct mv4101_dev_t *pdev = (struct mv4101_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%x %d\n", &addr, &r_cnt);

    down(&pdev->lock);

    mv4101_reg_read_addr[pdev->index]  = (u16)(addr);
    mv4101_reg_read_count[pdev->index] = r_cnt;

    up(&pdev->lock);

    return count;
}

static ssize_t mv4101_proc_reg_write_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 reg, data;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct mv4101_dev_t *pdev = (struct mv4101_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%x %x\n", &reg, &data);

    down(&pdev->lock);

    mv4101_reg_write(pdev->index, reg, data);

    up(&pdev->lock);

    return count;
}

static ssize_t mv4101_proc_vout_sdi_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int vout_ch, sdi_ch;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct mv4101_dev_t *pdev = (struct mv4101_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &vout_ch, &sdi_ch);

    down(&pdev->lock);

    if((vout_ch >= 0 && vout_ch < MV4101_DEV_VOUT_MAX) && (sdi_ch >= 0 && sdi_ch < MV4101_DEV_CH_MAX)) {
        mv4101_set_vout_sdi_src(pdev->index, vout_ch, sdi_ch);
    }

    up(&pdev->lock);

    return count;
}

static int mv4101_proc_reg_read_show(struct seq_file *sfile, void *v)
{
    int i;
    unsigned int value;
    struct mv4101_dev_t *pdev = (struct mv4101_dev_t *)sfile->private;

    down(&pdev->lock);

    for(i=0; i<mv4101_reg_read_count[pdev->index]; i++) {
        value = mv4101_reg_read(pdev->index, mv4101_reg_read_addr[pdev->index]+i);
        seq_printf(sfile, "[MV4101#%d] 0x%04x => 0x%04x\n", pdev->index, mv4101_reg_read_addr[pdev->index]+i, (u32)(value & 0xffff));
    }
    seq_printf(sfile, "----------------------------------------------------------\n");
    seq_printf(sfile, "echo [reg] [count] to node to read data from address\n");

    up(&pdev->lock);

    return 0;
}

static int mv4101_proc_reg_write_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "echo [reg] [data] to node to write data to address\n");
    return 0;
}

static int mv4101_proc_status_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct mv4101_dev_t *pdev = (struct mv4101_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[MV4101#%d]\n", pdev->index);
    seq_printf(sfile, "-----------------------------\n");
    seq_printf(sfile, "SDI    VCH    LOS    EQ_MODE \n");
    seq_printf(sfile, "=============================\n");
    for(i=0; i<MV4101_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*MV4101_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(MV4101_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_SDI(MV4101_VCH_MAP[j]) == i)) {
                if(eq_mode[pdev->index] == MV4101_EQ_MODE_HW_AUTO)
                    seq_printf(sfile, "%-6d %-6d %-6s HW_AUTO\n", i, j, ((mv4101_status_get_video_loss(pdev->index, i) == 0) ? "NO" : "YES"));
                else if(eq_mode[pdev->index] == MV4101_EQ_MODE_SW_AUTO)
                    seq_printf(sfile, "%-6d %-6d %-6s SW_AUTO(%d)\n", i, j, ((mv4101_status_get_video_loss(pdev->index, i) == 0) ? "NO" : "YES"), pdev->eq_mode[i]);
                else
                    seq_printf(sfile, "%-6d %-6d %-6s %d\n", i, j, ((mv4101_status_get_video_loss(pdev->index, i) == 0) ? "NO" : "YES"), eq_mode[pdev->index]);
            }
        }
    }

    up(&pdev->lock);

    return 0;
}

static int mv4101_proc_video_fmt_show(struct seq_file *sfile, void *v)
{
    int i, j;
    int fmt_idx;
    struct mv4101_dev_t *pdev = (struct mv4101_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[MV4101#%d] Incoming Video Fromat\n", pdev->index);
    seq_printf(sfile, "===================================================================\n");
    seq_printf(sfile, "SDI   VCH   Width   Height  Pixel_Rate(KHz)  Prog/Inter   Frame_Rate\n");
    seq_printf(sfile, "===================================================================\n");

    for(i=0; i<MV4101_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*MV4101_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(MV4101_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_SDI(MV4101_VCH_MAP[j]) == i)) {
                fmt_idx = mv4101_status_get_video_format(pdev->index, i);
                if(((fmt_idx > MV4101_VFMT_UNKNOWN) && (fmt_idx < MV4101_VFMT_MAX))) {
                    seq_printf(sfile, "%-5d %-5d %-7d %-7d %-16d %-12s %-11d\n",
                               i,
                               j,
                               mv4101_video_fmt_info[fmt_idx].active_width,
                               mv4101_video_fmt_info[fmt_idx].active_height,
                               mv4101_video_fmt_info[fmt_idx].pixel_rate,
                               ((mv4101_video_fmt_info[fmt_idx].prog == 1) ? "Progressive" : "Interlace"),
                               mv4101_video_fmt_info[fmt_idx].frame_rate);
                }
                else {
                    seq_printf(sfile, "%-5d %-5d %-7s %-7s %-16s %-12s %-11s\n", i, j, "-", "-", "-", "-", "-");
                }
            }
        }
    }

    up(&pdev->lock);

    return 0;
}

static int mv4101_proc_vout_sdi_show(struct seq_file *sfile, void *v)
{
    int i;
    struct mv4101_dev_t *pdev = (struct mv4101_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[MV4101#%d]\n", pdev->index);
    seq_printf(sfile, "---------------------------------------------------------------------------\n");
    for(i=0; i<MV4101_DEV_CH_MAX; i++) {
        seq_printf(sfile, "VOUT_CH#%d: SDI_CH#%d\n", i, mv4101_get_vout_sdi_src(pdev->index, i));
    }
    seq_printf(sfile, "---------------------------------------------------------------------------\n");
    seq_printf(sfile, "echo [VOUT_CH] [SDI_CH] to node to control vout from which sdi input source\n");
    seq_printf(sfile, "[VOUT_CH]: 0~3\n");
    seq_printf(sfile, "[SDI_CH] : 0~3\n");

    up(&pdev->lock);

    return 0;
}

static int mv4101_proc_reg_read_open(struct inode *inode, struct file *file)
{
    return single_open(file, mv4101_proc_reg_read_show, PDE(inode)->data);
}

static int mv4101_proc_reg_write_open(struct inode *inode, struct file *file)
{
    return single_open(file, mv4101_proc_reg_write_show, PDE(inode)->data);
}

static int mv4101_proc_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, mv4101_proc_status_show, PDE(inode)->data);
}

static int mv4101_proc_video_fmt_open(struct inode *inode, struct file *file)
{
    return single_open(file, mv4101_proc_video_fmt_show, PDE(inode)->data);
}

static int mv4101_proc_vout_sdi_open(struct inode *inode, struct file *file)
{
    return single_open(file, mv4101_proc_vout_sdi_show, PDE(inode)->data);
}

static struct file_operations mv4101_proc_reg_read_ops = {
    .owner  = THIS_MODULE,
    .open   = mv4101_proc_reg_read_open,
    .write  = mv4101_proc_reg_read_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations mv4101_proc_reg_write_ops = {
    .owner  = THIS_MODULE,
    .open   = mv4101_proc_reg_write_open,
    .write  = mv4101_proc_reg_write_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations mv4101_proc_status_ops = {
    .owner  = THIS_MODULE,
    .open   = mv4101_proc_status_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations mv4101_proc_video_fmt_ops = {
    .owner  = THIS_MODULE,
    .open   = mv4101_proc_video_fmt_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations mv4101_proc_vout_sdi_ops = {
    .owner  = THIS_MODULE,
    .open   = mv4101_proc_vout_sdi_open,
    .write  = mv4101_proc_vout_sdi_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void mv4101_proc_remove(int id)
{
    if(id >= MV4101_DEV_MAX)
        return;

    if(mv4101_proc_root[id]) {
        if(mv4101_proc_reg_read[id]) {
            remove_proc_entry(mv4101_proc_reg_read[id]->name, mv4101_proc_root[id]);
            mv4101_proc_reg_read[id] = NULL;
        }

        if(mv4101_proc_reg_write[id]) {
            remove_proc_entry(mv4101_proc_reg_write[id]->name, mv4101_proc_root[id]);
            mv4101_proc_reg_write[id] = NULL;
        }

        if(mv4101_proc_status[id]) {
            remove_proc_entry(mv4101_proc_status[id]->name, mv4101_proc_root[id]);
            mv4101_proc_status[id] = NULL;
        }

        if(mv4101_proc_video_fmt[id]) {
            remove_proc_entry(mv4101_proc_video_fmt[id]->name, mv4101_proc_root[id]);
            mv4101_proc_video_fmt[id] = NULL;
        }

        if(mv4101_proc_vout_sdi[id]) {
            remove_proc_entry(mv4101_proc_vout_sdi[id]->name, mv4101_proc_root[id]);
            mv4101_proc_vout_sdi[id] = NULL;
        }

        remove_proc_entry(mv4101_proc_root[id]->name, NULL);
        mv4101_proc_root[id] = NULL;
    }
}

static int mv4101_proc_init(int id)
{
    int ret = 0;

    if(id >= MV4101_DEV_MAX)
        return -1;

    /* root */
    mv4101_proc_root[id] = create_proc_entry(mv4101_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!mv4101_proc_root[id]) {
        printk("create proc node '%s' failed!\n", mv4101_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    mv4101_proc_root[id]->owner = THIS_MODULE;
#endif

    /* register read */
    mv4101_proc_reg_read[id] = create_proc_entry("reg_read", S_IRUGO|S_IXUGO, mv4101_proc_root[id]);
    if(!mv4101_proc_reg_read[id]) {
        printk("create proc node '%s/read' failed!\n", mv4101_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    mv4101_proc_reg_read[id]->proc_fops = &mv4101_proc_reg_read_ops;
    mv4101_proc_reg_read[id]->data      = (void *)&mv4101_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    mv4101_proc_reg_read[id]->owner     = THIS_MODULE;
#endif

    /* register write */
    mv4101_proc_reg_write[id] = create_proc_entry("reg_write", S_IRUGO|S_IXUGO, mv4101_proc_root[id]);
    if(!mv4101_proc_reg_write[id]) {
        printk("create proc node '%s/write' failed!\n", mv4101_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    mv4101_proc_reg_write[id]->proc_fops = &mv4101_proc_reg_write_ops;
    mv4101_proc_reg_write[id]->data      = (void *)&mv4101_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    mv4101_proc_reg_write[id]->owner     = THIS_MODULE;
#endif

    /* status */
    mv4101_proc_status[id] = create_proc_entry("status", S_IRUGO|S_IXUGO, mv4101_proc_root[id]);
    if(!mv4101_proc_status[id]) {
        printk("create proc node '%s/status' failed!\n", mv4101_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    mv4101_proc_status[id]->proc_fops = &mv4101_proc_status_ops;
    mv4101_proc_status[id]->data      = (void *)&mv4101_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    mv4101_proc_status[id]->owner     = THIS_MODULE;
#endif

    /* video format */
    mv4101_proc_video_fmt[id] = create_proc_entry("video_format", S_IRUGO|S_IXUGO, mv4101_proc_root[id]);
    if(!mv4101_proc_video_fmt[id]) {
        printk("create proc node '%s/video_format' failed!\n", mv4101_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    mv4101_proc_video_fmt[id]->proc_fops = &mv4101_proc_video_fmt_ops;
    mv4101_proc_video_fmt[id]->data      = (void *)&mv4101_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    mv4101_proc_video_fmt[id]->owner     = THIS_MODULE;
#endif

    /* vout_sdi */
    mv4101_proc_vout_sdi[id] = create_proc_entry("vout_sdi", S_IRUGO|S_IXUGO, mv4101_proc_root[id]);
    if(!mv4101_proc_vout_sdi[id]) {
        printk("create proc node '%s/vout_sdi' failed!\n", mv4101_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    mv4101_proc_vout_sdi[id]->proc_fops = &mv4101_proc_vout_sdi_ops;
    mv4101_proc_vout_sdi[id]->data      = (void *)&mv4101_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    mv4101_proc_vout_sdi[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    mv4101_proc_remove(id);
    return ret;
}

static int mv4101_miscdev_open(struct inode *inode, struct file *file)
{
    int i, ret = 0;
    struct mv4101_dev_t *pdev = NULL;

    /* lookup device */
    for(i=0; i<dev_num; i++) {
        if(mv4101_dev[i].miscdev.minor == iminor(inode)) {
            pdev = &mv4101_dev[i];
            break;
        }
    }

    if(!pdev) {
        ret = -EINVAL;
        goto exit;
    }

    file->private_data = (void *)pdev;

exit:
    return ret;
}

static int mv4101_miscdev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long mv4101_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct mv4101_dev_t *pdev = (struct mv4101_dev_t *)file->private_data;

    down(&pdev->lock);

    if(_IOC_TYPE(cmd) != MV4101_IOC_MAGIC) {
        ret = -ENOIOCTLCMD;
        goto exit;
    }

    switch(cmd) {
        case MV4101_GET_NOVID:
            {
                struct mv4101_ioc_data_t ioc_data;
                int novid;

                if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_data.sdi_ch >= MV4101_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                novid = mv4101_status_get_video_loss(pdev->index, ioc_data.sdi_ch);

                ioc_data.data = novid;
                ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            }
            break;

        case MV4101_GET_VIDEO_FMT:
            {
                struct mv4101_ioc_vfmt_t ioc_vfmt;
                int fmt_idx = 0;

                if(copy_from_user(&ioc_vfmt, (void __user *)arg, sizeof(ioc_vfmt))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vfmt.sdi_ch >= MV4101_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                fmt_idx = mv4101_status_get_video_format(pdev->index, ioc_vfmt.sdi_ch);
                if((fmt_idx < 0) || (fmt_idx >= MV4101_VFMT_MAX))
                    fmt_idx = MV4101_VFMT_UNKNOWN;

                ioc_vfmt.active_width  = mv4101_video_fmt_info[fmt_idx].active_width;
                ioc_vfmt.active_height = mv4101_video_fmt_info[fmt_idx].active_height;
                ioc_vfmt.total_width   = mv4101_video_fmt_info[fmt_idx].total_width;
                ioc_vfmt.total_height  = mv4101_video_fmt_info[fmt_idx].total_height;
                ioc_vfmt.pixel_rate    = mv4101_video_fmt_info[fmt_idx].pixel_rate;
                ioc_vfmt.bit_width     = mv4101_video_fmt_info[fmt_idx].bit_width;
                ioc_vfmt.prog          = mv4101_video_fmt_info[fmt_idx].prog;
                ioc_vfmt.frame_rate    = mv4101_video_fmt_info[fmt_idx].frame_rate;
                ioc_vfmt.bit_rate      = mv4101_video_fmt_info[fmt_idx].bit_rate;
                ret = (copy_to_user((void __user *)arg, &ioc_vfmt, sizeof(ioc_vfmt))) ? (-EFAULT) : 0;
            }
            break;

        default:
            ret = -ENOIOCTLCMD;
            break;
    }

exit:
    up(&pdev->lock);
    return ret;
}

static struct file_operations mv4101_miscdev_fops = {
    .owner          = THIS_MODULE,
    .open           = mv4101_miscdev_open,
    .release        = mv4101_miscdev_release,
    .unlocked_ioctl = mv4101_miscdev_ioctl,
};

static int mv4101_miscdev_init(int id)
{
    int ret;

    if(id >= MV4101_DEV_MAX)
        return -1;

    /* clear */
    memset(&mv4101_dev[id].miscdev, 0, sizeof(struct miscdevice));

    /* create device node */
    mv4101_dev[id].miscdev.name  = mv4101_dev[id].name;
    mv4101_dev[id].miscdev.minor = MISC_DYNAMIC_MINOR;
    mv4101_dev[id].miscdev.fops  = (struct file_operations *)&mv4101_miscdev_fops;
    ret = misc_register(&mv4101_dev[id].miscdev);
    if(ret) {
        printk("create %s misc device node failed!\n", mv4101_dev[id].name);
        mv4101_dev[id].miscdev.name = 0;
        goto exit;
    }

exit:
    return ret;
}

static void mv4101_hw_reset(void)
{
    if(!rstb_used || !init)
        return;

    /* Active Low */
    plat_gpio_set_value(rstb_used, 0);

    mdelay(100);

    /* Active High and release reset */
    plat_gpio_set_value(rstb_used, 1);

    mdelay(100);
}

static int mv4101_device_init(int id)
{
    int i, ret = 0;

    if(id >= MV4101_DEV_MAX)
        return -1;

    if(!init)
        goto exit;

    /* video init */
    ret = mv4101_init_chip(id, vout_format[id], eq_mode[id]);
    if(ret < 0)
        goto exit;

    /* audio int */
    for(i=0; i<MV4101_DEV_CH_MAX; i++) {
        ret = mv4101_audio_set_bit_width(id, i, MV4101_AUDIO_WIDTH_16BIT);
        if(ret < 0)
            goto exit;

        ret = mv4101_audio_set_mute(id, i, MV4101_AUDIO_SOUND_MUTE_DEFAULT);
        if(ret < 0)
            goto exit;
    }

exit:
    printk("MV4101#%d Init...%s!\n", id, ((ret == 0) ? "OK" : "Fail"));

    return ret;
}

static int mv4101_watchdog_thread(void *data)
{
    int ret;
    int i, j, ch, mode, lock, start_mode, chip_type, fmt_idx, vout;
    struct mv4101_dev_t *pdev;

    do {
            /* check mv4101 status */
            for(i=0; i<dev_num; i++) {
                pdev = &mv4101_dev[i];

                down(&pdev->lock);

                chip_type = mv4101_get_chip_type(pdev->index);

                if((eq_mode[i] == MV4101_EQ_MODE_HW_AUTO) && (chip_type <= MV4101_CHIP_TYPE_A2))    ///< Rev A/A2 not support hardware auto
                    eq_mode[i] = MV4101_EQ_MODE_SW_AUTO;

                for(ch=0; ch<MV4101_DEV_CH_MAX; ch++) {
                    pdev->cur_plugin[ch] = (mv4101_status_get_video_loss(i, ch) == 0) ? 1 : 0;

                    if((pdev->cur_plugin[ch] == 1) && (pdev->pre_plugin[ch] == 0)) {        ///< cable plugged-in
                        goto sts_update;
                    }
                    else if((pdev->cur_plugin[ch] == 1) && (pdev->pre_plugin[ch] == 1)) {   ///< cable is connected
                        lock = mv4101_status_get_flywheel_lock(i, ch);
                        if(!lock && (eq_mode[i] == MV4101_EQ_MODE_SW_AUTO)) {
                            /* start search proper EQ mode */
                            if(pdev->eq_mode[ch] == 0)
                                start_mode = MV4101_EQ_MODE_A_LINE_0;
                            else
                                start_mode = pdev->eq_mode[ch];

                            /* start from previous mode */
                            for(mode=start_mode; mode<MV4101_EQ_MODE_MAX; mode++) {
                                ret = mv4101_set_eq_mode(i, ch, mode);
                                if(ret < 0)
                                    break;

                                for(j=0; j<5; j++) {
                                    msleep(2);
                                    lock = mv4101_status_get_flywheel_lock(i, ch);
                                    if(lock) {
                                        pdev->eq_mode[ch] = mode;
                                        goto check_vfmt;
                                    }
                                }
                            }

                            for(mode=MV4101_EQ_MODE_A_LINE_0; mode<start_mode; mode++) {
                                ret = mv4101_set_eq_mode(i, ch, mode);
                                if(ret < 0)
                                    break;

                                for(j=0; j<5; j++) {
                                    msleep(2);
                                    lock = mv4101_status_get_flywheel_lock(i, ch);
                                    if(lock) {
                                        pdev->eq_mode[ch] = mode;
                                        goto check_vfmt;
                                    }
                                }
                            }
                        }
check_vfmt:
                        if(lock) {
                            fmt_idx = mv4101_status_get_video_format(i, ch);

                            /* notify current video format */
                            if(notify && pdev->vfmt_notify && (fmt_idx > 0) && (fmt_idx < MV4101_VFMT_MAX)) {
                                for(vout=0; vout<MV4101_DEV_VOUT_MAX; vout++) {
                                    if(mv4101_get_vout_sdi_src(i, vout) == ch)
                                        pdev->vfmt_notify(i, vout, &mv4101_video_fmt_info[fmt_idx]);
                                }
                            }

                            /* notify current video on */
                            if(notify && pdev->vlos_notify && (fmt_idx > 0) && (fmt_idx < MV4101_VFMT_MAX)) {
                                for(vout=0; vout<MV4101_DEV_VOUT_MAX; vout++) {
                                    if(mv4101_get_vout_sdi_src(i, vout) == ch)
                                        pdev->vlos_notify(i, vout, 0);
                                }
                            }
                        }
                    }
                    else {  ///< cable not connected or is plugged-out
                        /* notify current video loss */
                        if(notify && pdev->vlos_notify) {
                            for(vout=0; vout<MV4101_DEV_VOUT_MAX; vout++) {
                                if(mv4101_get_vout_sdi_src(i, vout) == ch)
                                    pdev->vlos_notify(i, vout, 1);
                            }
                        }
                    }
sts_update:
                    pdev->pre_plugin[ch] = pdev->cur_plugin[ch];
                }

                up(&pdev->lock);
            }

            /* sleep 1 second */
            schedule_timeout_interruptible(msecs_to_jiffies(1000));

    } while (!kthread_should_stop());

    return 0;
}

static int __init mv4101_init(void)
{
    int i, ret;

    /* platfrom check */
    plat_identifier_check();

    /* check device number */
    if(dev_num > MV4101_DEV_MAX) {
        printk("MV4101 dev_num=%d invalid!!(Max=%d)\n", dev_num, MV4101_DEV_MAX);
        return -1;
    }

    /* check video output format */
    for(i=0; i<dev_num; i++) {
        if((vout_format[i] < 0) || (vout_format[i] >= MV4101_VOUT_FMT_MAX)) {
            printk("MV4101#%d vout_format=%d not support!!\n", i, vout_format[i]);
            return -1;
        }
    }

    /* register i2c client for contol mv4101 */
    ret = mv4101_i2c_register();
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

    /* create channel mapping table for different board */
    ret = mv4101_create_ch_map(ch_map);
    if(ret < 0)
        goto err;

    /* hardware reset for all device */
    mv4101_hw_reset();

    /* MV4101 init */
    for(i=0; i<dev_num; i++) {
        mv4101_dev[i].index = i;

        sprintf(mv4101_dev[i].name, "mv4101.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&mv4101_dev[i].lock, 1);
#else
        init_MUTEX(&mv4101_dev[i].lock);
#endif

        /*register gpio pin for device GSPI interface control */
        ret = mv4101_register_spi_gpio_pin(i);
        if(ret < 0)
            goto err;

        ret = mv4101_proc_init(i);
        if(ret < 0)
            goto err;

        ret = mv4101_miscdev_init(i);
        if(ret < 0)
            goto err;

        ret = mv4101_device_init(i);
        if(ret < 0)
            goto err;
    }

    if(init) {
        /* init mv4101 watchdog thread for check lock status */
        mv4101_wd = kthread_create(mv4101_watchdog_thread, NULL, "mv4101_wd");
        if(!IS_ERR(mv4101_wd))
            wake_up_process(mv4101_wd);
        else {
            printk("create mv4101 watchdog thread failed!!\n");
            mv4101_wd = 0;
            ret = -EFAULT;
            goto err;
        }
    }

    return 0;

err:
    if(mv4101_wd)
        kthread_stop(mv4101_wd);

    mv4101_i2c_unregister();

    for(i=0; i<MV4101_DEV_MAX; i++) {
        mv4101_deregister_spi_gpio_pin(i);

        if(mv4101_dev[i].miscdev.name)
            misc_deregister(&mv4101_dev[i].miscdev);

        mv4101_proc_remove(i);
    }

    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    return ret;
}

static void __exit mv4101_exit(void)
{
    int i;

    /* stop mv4101 watchdog thread */
    if(mv4101_wd)
        kthread_stop(mv4101_wd);

    mv4101_i2c_unregister();

    for(i=0; i<MV4101_DEV_MAX; i++) {
        /* free gpio pin */
        mv4101_deregister_spi_gpio_pin(i);

        /* remove device node */
        if(mv4101_dev[i].miscdev.name)
            misc_deregister(&mv4101_dev[i].miscdev);

        /* remove proc node */
        mv4101_proc_remove(i);
    }

    /* release gpio pin */
    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);
}

module_init(mv4101_init);
module_exit(mv4101_exit);

MODULE_DESCRIPTION("Grain Media MV4101 4CH SDI Receiver Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
