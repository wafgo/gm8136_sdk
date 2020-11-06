/**
 * @file cx25930_drv.c
 * Conexant CX25930 4CH SDI Receiver with four integrated Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.21 $
 * $Date: 2014/07/29 05:36:14 $
 *
 * ChangeLog:
 *  $Log: cx25930_drv.c,v $
 *  Revision 1.21  2014/07/29 05:36:14  shawn_hu
 *  Add the module parameter of "ibus" to specify which I2C bus has the decoder attached.
 *
 *  Revision 1.20  2014/04/28 09:10:16  jerson_l
 *  1. update driver library version to V2.0.118
 *
 *  Revision 1.19  2014/02/24 13:23:01  jerson_l
 *  1. enable notify option as default
 *
 *  Revision 1.18  2014/01/17 13:55:58  ccsu
 *  [audio] add hook digital_codec_name
 *
 *  Revision 1.17  2013/12/05 08:57:38  jerson_l
 *  1. update cx25930 driver from conexant release v2.100.102
 *
 *  Revision 1.16  2013/12/04 10:06:35  jerson_l
 *  1. update cx25930 driver library from conexant release v2.99.102
 *
 *  Revision 1.15  2013/11/27 05:33:01  jerson_l
 *  1. support notify procedure
 *
 *  Revision 1.14  2013/11/22 09:13:33  jerson_l
 *  1. modify module parameter "notify" definition
 *
 *  Revision 1.13  2013/11/22 08:42:06  jerson_l
 *  1. support cx25930-11zp2
 *
 *  Revision 1.12  2013/11/20 09:47:44  ccsu
 *  modify audio default sample rate as 8k
 *
 *  Revision 1.11  2013/11/20 09:03:22  easonli
 *  *:[conexant] add audio functions
 *
 *  Revision 1.10  2013/11/20 08:48:06  jerson_l
 *  1. add video format and novid notify
 *
 *  Revision 1.9  2013/11/18 12:12:03  ccsu
 *  rm cx20811
 *
 *  Revision 1.8  2013/11/18 07:45:42  ccsu
 *  add cx20811 ad/da sample rate, sample size module parameter
 *
 *  Revision 1.7  2013/11/15 08:30:04  ccsu
 *  add audio adda cx20811
 *
 *  Revision 1.6  2013/11/06 07:28:29  jerson_l
 *  1. update cx25930 driver library
 *
 *  Revision 1.5  2013/10/16 02:12:49  jerson_l
 *  1. disable cx25930 monitor watchdog in master cpu
 *
 *  Revision 1.4  2013/10/15 05:14:43  jerson_l
 *  1. support RevID=0x0002 IC
 *
 *  Revision 1.3  2013/10/11 09:53:13  jerson_l
 *  1. add novid and video format ioctl command
 *  2. add ch_map module parameter for control different EVB channel mapping
 *
 *  Revision 1.2  2013/09/02 06:34:32  jerson_l
 *  1. add watchdog thread to monitor cable plug-in/out or video format switch
 *
 *  Revision 1.1.1.1  2013/07/25 09:29:13  jerson_l
 *  add front_end device driver
 *
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
#include "conexant/cx25930.h"                        ///< from module/include/front_end/sdi
#include "lib_cx25930/ArtemisReceiver.h"

#define CX25930_CLKIN                   27000000
#define CX25930_I2C_ARRAY_RW_MAX        768
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
static ushort iaddr[CX25930_DEV_MAX] = {0x88, 0x8a, 0x8c, 0x8e};
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
static int clk_freq = CX25930_CLKIN;   ///< 27MHz
module_param(clk_freq, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_freq, "External Clock Frequency(Hz)");

/* device init */
static int init = 1;
module_param(init, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init, "device init: 0:no-init  1:init");

/* device use gpio as rstb pin */
static int rstb_used = PLAT_GPIO_ID_X_CAP_RST;
module_param(rstb_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rstb_used, "Use GPIO as RSTB Pin: 0:None, 1:X_CAP_RST");

/* input data rate */
static int input_rate[CX25930_DEV_MAX] = {[0 ... (CX25930_DEV_MAX - 1)] = CX25930_INPUT_RATE_HD};
module_param_array(input_rate, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(input_rate, "Input Data Rate: 0:SD, 1:HD, 2:GEN3");

static int vout_format[CX25930_DEV_MAX] = {[0 ... (CX25930_DEV_MAX - 1)] = CX25930_VOUT_FORMAT_BT656};
module_param_array(vout_format, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vout_format, "Video Output Format: 0:BT656, 1:BT1120");

/* cable lenght */
static int cab_len[CX25930_DEV_MAX] = {[0 ... (CX25930_DEV_MAX - 1)] = CX25930_CAB_LEN_0_50M_AUTO};
module_param_array(cab_len, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(cab_len, "Cable Length: 0:0~50M(Auto), 1:75~100M, 2:125~150M");

/* device channel mapping */
static int ch_map = 0;
module_param(ch_map, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_map, "channel mapping");

/* audio sample rate */
static int sample_rate = CX25930_KHZ8;
module_param(sample_rate, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sample_rate, "audio sample rate: 0:48k  1:44.k 2:32k 3:16k 4:8k");

/* audio sample size */
static int sample_size = CX25930_BIT_16;
module_param(sample_size, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sample_size, "audio sample size: 0:8bit  1:16bit 2:24bit");

/* notify */
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

struct cx25930_dev_t {
    int                     index;                          ///< device index
    char                    name[16];                       ///< device name
    struct semaphore        lock;                           ///< device locker
    struct miscdevice       miscdev;                        ///< device node for ioctl access
    CX_COMMUNICATION        cx_intf;                        ///< conexant common interface
    struct artm_re_info_t   re_info[CX25930_DEV_CH_MAX];    ///< device channel raster engine info record

    int                     (*vfmt_notify)(int id, int ch, struct cx25930_video_fmt_t *vfmt);
    int                     (*vlos_notify)(int id, int ch, int vlos);
};

static struct cx25930_dev_t  cx25930_dev[CX25930_DEV_MAX];
static struct proc_dir_entry *cx25930_proc_root[CX25930_DEV_MAX]      = {[0 ... (CX25930_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx25930_proc_reg_read[CX25930_DEV_MAX]  = {[0 ... (CX25930_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx25930_proc_reg_write[CX25930_DEV_MAX] = {[0 ... (CX25930_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx25930_proc_status[CX25930_DEV_MAX]    = {[0 ... (CX25930_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx25930_proc_loopback[CX25930_DEV_MAX]  = {[0 ... (CX25930_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx25930_proc_video_fmt[CX25930_DEV_MAX] = {[0 ... (CX25930_DEV_MAX - 1)] = NULL};

static u16 cx25930_read_reg_addr[CX25930_DEV_MAX]  = {[0 ... (CX25930_DEV_MAX - 1)] = 0};
static int cx25930_read_reg_count[CX25930_DEV_MAX] = {[0 ... (CX25930_DEV_MAX - 1)] = 0};

static struct i2c_client  *cx25930_i2c_client = NULL;
static struct task_struct *cx25930_wd         = NULL;
static u32 CX25930_VCH_MAP[CX25930_DEV_MAX*CX25930_DEV_CH_MAX];

static struct cx25930_video_fmt_t cx25930_video_fmt_info[CX25930_VIDEO_FMT_IDX_MAX] = {
    /* IDX  Active_W     Active_H    Total_W     Total_H     Pixel_Rate(KHz) Bit_Width   Prog    Frame_Rate  Bit_Rate */
    {  0,   0,           0,          0,          0,          0,              0,          0,      0,          0       },
    {  1,   720,         480,        858,        525,        13500,          10,         0,      60,         0       },
    {  2,   720,         576,        864,        625,        13500,          10,         0,      50,         0       },
    {  3,   1280,        720,        1650,       750,        74250,          10,         1,      60,         1       },
    {  4,   1280,        720,        1980,       750,        74250,          10,         1,      50,         1       },
    {  5,   1280,        720,        3330,       750,        74250,          10,         1,      30,         1       },
    {  6,   1280,        720,        3960,       750,        74250,          10,         1,      25,         1       },
    {  7,   1280,        720,        4125,       750,        74250,          10,         1,      24,         1       },
    {  8,   1920,        1080,       2200,       1125,       74250,          10,         0,      60,         1       },
    {  9,   1920,        1080,       2640,       1125,       74250,          10,         0,      50,         1       },
    { 10,   1920,        1080,       2200,       1125,       74250,          10,         1,      30,         1       },
    { 11,   1920,        1080,       2640,       1125,       74250,          10,         1,      25,         1       },
    { 12,   1920,        1080,       2750,       1125,       74250,          10,         1,      24,         1       },
    { 13,   1280,        720,        1650,       750,        74250,          10,         1,      60,         1       },
    { 14,   1280,        720,        3300,       750,        74250,          10,         1,      30,         1       },
    { 15,   1280,        720,        4125,       750,        74250,          10,         1,      24,         1       },
    { 16,   1920,        1080,       2200,       1125,       74250,          10,         0,      60,         1       },
    { 17,   1920,        1080,       2200,       1125,       74250,          10,         1,      30,         1       },
    { 18,   1920,        1080,       2750,       1125,       74250,          10,         1,      24,         1       },
    { 19,   1920,        1080,       2200,       1125,       148500,         10,         1,      60,         2       },
    { 20,   1920,        1080,       2640,       1125,       148500,         10,         1,      50,         2       },
    { 21,   1920,        1080,       2200,       1125,       148500,         10,         1,      60,         2       },
};

static audio_funs_t cx25930_audio_funs;

static int cx25930_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    cx25930_i2c_client = client;
    return 0;
}

static int cx25930_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id cx25930_i2c_id[] = {
    { "cx25930_i2c", 0 },
    { }
};

static struct i2c_driver cx25930_i2c_driver = {
    .driver = {
        .name  = "CX25930_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = cx25930_i2c_probe,
    .remove   = cx25930_i2c_remove,
    .id_table = cx25930_i2c_id
};

static int cx25930_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&cx25930_i2c_driver);
    if(ret < 0) {
        printk("CX25930 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = iaddr[0]>>1;
    strlcpy(info.type, "cx25930_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("CX25930 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("CX25930 can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&cx25930_i2c_driver);
   return -ENODEV;
}

static void cx25930_i2c_unregister(void)
{
    i2c_unregister_device(cx25930_i2c_client);
    i2c_del_driver(&cx25930_i2c_driver);
    cx25930_i2c_client = NULL;
}

static int cx25930_create_ch_map(int map_id)
{
    int i;

    switch(map_id) {
        case 0: ///< for GM8210/GM8287 Socket Board, GM8210 System Board
            for(i=0; i<dev_num; i++) {
                CX25930_VCH_MAP[(i*CX25930_DEV_CH_MAX)+0] = CH_IDENTIFY(dev_num-i-1, 0, CX25930_DEV_VOUT_0);
                CX25930_VCH_MAP[(i*CX25930_DEV_CH_MAX)+1] = CH_IDENTIFY(dev_num-i-1, 1, CX25930_DEV_VOUT_1);
                CX25930_VCH_MAP[(i*CX25930_DEV_CH_MAX)+2] = CH_IDENTIFY(dev_num-i-1, 2, CX25930_DEV_VOUT_2);
                CX25930_VCH_MAP[(i*CX25930_DEV_CH_MAX)+3] = CH_IDENTIFY(dev_num-i-1, 3, CX25930_DEV_VOUT_3);
            }
            break;

        case 1: ///< for GM8287 System Board
        default:
            for(i=0; i<dev_num; i++) {
                CX25930_VCH_MAP[(i*CX25930_DEV_CH_MAX)+0] = CH_IDENTIFY(i, 0, CX25930_DEV_VOUT_0);
                CX25930_VCH_MAP[(i*CX25930_DEV_CH_MAX)+1] = CH_IDENTIFY(i, 1, CX25930_DEV_VOUT_1);
                CX25930_VCH_MAP[(i*CX25930_DEV_CH_MAX)+2] = CH_IDENTIFY(i, 2, CX25930_DEV_VOUT_2);
                CX25930_VCH_MAP[(i*CX25930_DEV_CH_MAX)+3] = CH_IDENTIFY(i, 3, CX25930_DEV_VOUT_3);
            }
            break;
    }

    return 0;
}

int cx25930_get_vch_id(int id, CX25930_DEV_VOUT_T vout, int sdi_ch)
{
    int i;

    if(id >= dev_num || vout >= CX25930_DEV_VOUT_MAX || sdi_ch >= CX25930_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*CX25930_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(CX25930_VCH_MAP[i]) == id) &&
           (CH_IDENTIFY_VOUT(CX25930_VCH_MAP[i]) == vout) &&
           (CH_IDENTIFY_SDI(CX25930_VCH_MAP[i]) == sdi_ch)) {
            return i;
        }
    }

    return 0;
}
EXPORT_SYMBOL(cx25930_get_vch_id);

int cx25930_get_vout_format(int id)
{
    if(id >= dev_num)
        return CX25930_VOUT_FORMAT_BT656;
    else
        return vout_format[id];
}
EXPORT_SYMBOL(cx25930_get_vout_format);

int cx25930_status_get_video_format(int id, int ch, struct cx25930_video_fmt_t *vfmt)
{
    int ret = TRUE;
    int fmt_idx = 0;
    struct cx25930_dev_t *pdev = &cx25930_dev[id];

    if(id >= dev_num || ch >= CX25930_DEV_CH_MAX || !vfmt)
        return -1;

    down(&pdev->lock);

    ret &= ARTM_getVideoFormat(&pdev->cx_intf, ch, &fmt_idx);
    if((ret == TRUE) && ((fmt_idx >= 0) && (fmt_idx < CX25930_VIDEO_FMT_IDX_MAX))) {
        memcpy(vfmt, &cx25930_video_fmt_info[fmt_idx], sizeof(struct cx25930_video_fmt_t));
        ret = 0;
    }
    else
        ret = -1;

    up(&pdev->lock);

    return ret;
}
EXPORT_SYMBOL(cx25930_status_get_video_format);

int cx25930_status_get_video_loss(int id, int ch)
{
    int ret;
    struct cx25930_dev_t *pdev = &cx25930_dev[id];

    if(id >= dev_num || ch >= CX25930_DEV_CH_MAX)
        return 1;

    down(&pdev->lock);

    ret = ARTM_ReadLOS(&pdev->cx_intf, ch);

    up(&pdev->lock);

    return ret;
}
EXPORT_SYMBOL(cx25930_status_get_video_loss);

void *cx25930_get_cxcomm_handle(u8 id)
{
    if(id >= CX25930_DEV_MAX)
        return NULL;

    return ((void *)&cx25930_dev[id].cx_intf);
}
EXPORT_SYMBOL(cx25930_get_cxcomm_handle);

int cx25930_get_device_num(void)
{
    return dev_num;
}
EXPORT_SYMBOL(cx25930_get_device_num);

int cx25930_notify_vfmt_register(int id, int (*nt_func)(int, int, struct cx25930_video_fmt_t *))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&cx25930_dev[id].lock);

    cx25930_dev[id].vfmt_notify = nt_func;

    up(&cx25930_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(cx25930_notify_vfmt_register);

void cx25930_notify_vfmt_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&cx25930_dev[id].lock);

    cx25930_dev[id].vfmt_notify = NULL;

    up(&cx25930_dev[id].lock);
}
EXPORT_SYMBOL(cx25930_notify_vfmt_deregister);

int cx25930_notify_vlos_register(int id, int (*nt_func)(int, int, int))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&cx25930_dev[id].lock);

    cx25930_dev[id].vlos_notify = nt_func;

    up(&cx25930_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(cx25930_notify_vlos_register);

void cx25930_notify_vlos_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&cx25930_dev[id].lock);

    cx25930_dev[id].vlos_notify = NULL;

    up(&cx25930_dev[id].lock);
}
EXPORT_SYMBOL(cx25930_notify_vlos_deregister);

static int cx25930_sound_switch(int ssp_idx, int chan_num, bool is_on)
{
    printk(KERN_WARNING "%s: No implementation\n", __func__);
    return 0;
}

int cx25930_i2c_write(u8 id, u16 reg, u32 data, int size)
{
    int i;
    u8  buf[6];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if((id >= dev_num) || (size <= 0) || (size > 4))
        return -1;

    if(!cx25930_i2c_client) {
        printk("CX25930 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(cx25930_i2c_client->dev.parent);

    buf[0] = (reg>>8) & 0xff;   ///< Sub-Address is MSB first
    buf[1] = (reg & 0xff);

    for(i=0; i<size; i++)       ///< Data is LSB first
        buf[i+2] = (data>>(8*i))&0xff;

    msgs.addr  = iaddr[id]>>1;
    msgs.flags = 0;
    msgs.len   = size + 2;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("CX25930#%d i2c write failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(cx25930_i2c_write);

int cx25930_i2c_read(u8 id, u16 reg, void *data, int size)
{
    u8 buf[2];
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if((id >= dev_num) || (!data) || (size <= 0) || (size > 4))
        return -1;

    if(!cx25930_i2c_client) {
        printk("CX25930 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(cx25930_i2c_client->dev.parent);

    buf[0] = (reg>>8) & 0xff;   ///< Sub-Address is MSB first
    buf[1] = (reg & 0xff);

    msgs[0].addr  = iaddr[id]>>1;
    msgs[0].flags = 0; /* write */
    msgs[0].len   = 2;
    msgs[0].buf   = buf;

    msgs[1].addr  = (iaddr[id] + 1)>>1;
    msgs[1].flags = 1; /* read */
    msgs[1].len   = size;
    msgs[1].buf   = (u8 *)data;

    if(i2c_transfer(adapter, msgs, 2) != 2) {
        printk("CX25930#%d i2c read failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(cx25930_i2c_read);

int cx25930_i2c_array_write(u8 id, u16 reg, void *parray, int array_size)
{
    int ret = 0;
    int i, num = 0;
    u8  buf[CX25930_I2C_ARRAY_RW_MAX+2];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if((id >= dev_num) || !parray || (array_size <= 0) || (array_size > CX25930_I2C_ARRAY_RW_MAX) || (array_size%2 != 0))
        return -1;

    if(!cx25930_i2c_client) {
        printk("CX25930 i2c_client not register!!\n");
        return -1;
    }

    /* Enable CX25930 Burst Transfer */
    if(ARTM_BurstTransfer_Ctrl(&cx25930_dev[id].cx_intf, 1 , 2, array_size) != TRUE)
        return -1;

    adapter = to_i2c_adapter(cx25930_i2c_client->dev.parent);

    buf[num++] = (reg>>8) & 0xff;   ///< Sub-Address is MSB first
    buf[num++] = (reg & 0xff);
    for(i=0; i<array_size; i++) {
        buf[num++] = *(((u8 *)parray)+i);
    }

    msgs.addr  = iaddr[id]>>1;
    msgs.flags = 0;
    msgs.len   = num;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("CX25930#%d i2c array write failed!!\n", id);
        ret = -1;
        goto exit;
    }

exit:
    ARTM_BurstTransfer_Ctrl(&cx25930_dev[id].cx_intf, 0 , 0, 0);    ///< Disable CX25930 Burst Transfer

    return ret;
}
EXPORT_SYMBOL(cx25930_i2c_array_write);

int cx25930_i2c_array_read(u8 id, u16 reg, void *parray, int array_size)
{
    int ret = 0;
    u8  buf[2];
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if((id >= dev_num) || !parray || (array_size <= 0) || (array_size > CX25930_I2C_ARRAY_RW_MAX) || (array_size%2 != 0))
        return -1;

    if(!cx25930_i2c_client) {
        printk("CX25930 i2c_client not register!!\n");
        return -1;
    }

    /* Enable CX25930 Burst Transfer */
    if(ARTM_BurstTransfer_Ctrl(&cx25930_dev[id].cx_intf, 1 , 2, array_size) != TRUE)
        return -1;

    adapter = to_i2c_adapter(cx25930_i2c_client->dev.parent);

    buf[0] = (reg>>8) & 0xff;   ///< Sub-Address is MSB first
    buf[1] = (reg & 0xff);

    msgs[0].addr  = iaddr[id]>>1;
    msgs[0].flags = 0; /* write */
    msgs[0].len   = 2;
    msgs[0].buf   = buf;

    msgs[1].addr  = (iaddr[id] + 1)>>1;
    msgs[1].flags = 1; /* read */
    msgs[1].len   = array_size;
    msgs[1].buf   = (u8 *)parray;

    if(i2c_transfer(adapter, msgs, 2) != 2) {
        printk("CX25930#%d i2c array read failed!!\n", id);
        ret = -1;
        goto exit;
    }

exit:
    ARTM_BurstTransfer_Ctrl(&cx25930_dev[id].cx_intf, 0 , 0, 0);    ///< Disable Burst Transfer

    return ret;
}
EXPORT_SYMBOL(cx25930_i2c_array_read);

static unsigned int cx25930_cxcomm_i2c_write(unsigned id, unsigned reg, int value, int size)
{
    int ret;

    if((size != 1) && (size != 2) && (size != 4))
        return FALSE;

    ret = cx25930_i2c_write((u8)id, (u16)reg, value, size);

    return ((ret < 0) ? FALSE : TRUE);
}

static unsigned int cx25930_cxcomm_i2c_read(unsigned char id, unsigned short reg, void *value, int size)
{
    int ret;

    if((size != 1) && (size != 2) && (size != 4))
        return FALSE;

    ret = cx25930_i2c_read((u8)id, (u16)reg, value, size);

    return ((ret < 0) ? FALSE : TRUE);
}

static unsigned int cx25930_cxcomm_i2c_burst_write(unsigned id, unsigned reg, void *parray, int size)
{
    int ret;

    if(size <= CX25930_I2C_ARRAY_RW_MAX) {
        ret = cx25930_i2c_array_write((u8)id, (u16)reg, parray, size);
    }
    else {
        int i;
        u8  *ptr = (u8 *)parray;

        for(i=0; i<(size/CX25930_I2C_ARRAY_RW_MAX); i++) {
            ret = cx25930_i2c_array_write((u8)id, (u16)(reg+(i*CX25930_I2C_ARRAY_RW_MAX)), &ptr[i*CX25930_I2C_ARRAY_RW_MAX], CX25930_I2C_ARRAY_RW_MAX);
            if(ret < 0)
                break;
        }

        if(!ret && (size > (i*CX25930_I2C_ARRAY_RW_MAX)))
            ret = cx25930_i2c_array_write((u8)id, (u16)(reg+(i*CX25930_I2C_ARRAY_RW_MAX)), &ptr[i*CX25930_I2C_ARRAY_RW_MAX], (size - (i*CX25930_I2C_ARRAY_RW_MAX)));
    }

    return ((ret < 0) ? FALSE : TRUE);
}

static unsigned int cx25930_cxcomm_i2c_burst_read(unsigned id, unsigned reg, void *p_value, int size)
{
    int ret;

    if(size <= CX25930_I2C_ARRAY_RW_MAX) {
        ret = cx25930_i2c_array_read((u8)id, (u16)reg, p_value, size);
    }
    else {
        int i;
        u8  *ptr = (u8 *)p_value;

        for(i=0; i<(size/CX25930_I2C_ARRAY_RW_MAX); i++) {
            ret = cx25930_i2c_array_read((u8)id, (u16)(reg+(i*CX25930_I2C_ARRAY_RW_MAX)), &ptr[i*CX25930_I2C_ARRAY_RW_MAX], CX25930_I2C_ARRAY_RW_MAX);
            if(ret < 0)
                break;
        }

        if(!ret && (size > (i*CX25930_I2C_ARRAY_RW_MAX)))
            ret = cx25930_i2c_array_read((u8)id, (u16)(reg+(i*CX25930_I2C_ARRAY_RW_MAX)), &ptr[i*CX25930_I2C_ARRAY_RW_MAX], (size - (i*CX25930_I2C_ARRAY_RW_MAX)));
    }

    return ((ret < 0) ? FALSE : TRUE);
}

static void cx25930_cxcomm_sleep(unsigned long msec)
{
    if(msec <= 50)
        msleep(1);
    else
        msleep(msec/50);
}

static ssize_t cx25930_proc_reg_read_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int r_cnt;
    u32 reg;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct cx25930_dev_t *pdev = (struct cx25930_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%x %d\n", &reg, &r_cnt);

    down(&pdev->lock);

    cx25930_read_reg_addr[pdev->index]  = (u16)((reg>>1)<<1);     ///< align 2
    cx25930_read_reg_count[pdev->index] = r_cnt;

    up(&pdev->lock);

    return count;
}

static ssize_t cx25930_proc_reg_write_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 reg, data;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct cx25930_dev_t *pdev = (struct cx25930_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%x %x\n", &reg, &data);

    down(&pdev->lock);

    if(reg%2 == 0) {    ///< register must align 2
        ARTM_writeRegister(&pdev->cx_intf, reg, data);
    }

    up(&pdev->lock);

    return count;
}

static ssize_t cx25930_proc_loopback_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int in_ch, out_ch, lbk_mode;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct cx25930_dev_t *pdev = (struct cx25930_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d %d\n", &out_ch, &in_ch, &lbk_mode);

    down(&pdev->lock);

    if((in_ch >= 0 && in_ch < CX25930_DEV_CH_MAX) && (out_ch >= 0 && out_ch < CX25930_DEV_CH_MAX)) {
        switch(lbk_mode) {
            case CX25930_LBK_OFF:
                ARTM_PowerDownTxChannel(&pdev->cx_intf, out_ch);
                break;
            case CX25930_LBK_EQ:
                ARTM_EQLoopback(&pdev->cx_intf, in_ch, out_ch);
                break;
            case CX25930_LBK_CDR:
                ARTM_CDRLoopback(&pdev->cx_intf, in_ch, out_ch);
                break;
        }
    }

    up(&pdev->lock);

    return count;
}

static int cx25930_proc_reg_read_show(struct seq_file *sfile, void *v)
{
    int i, ret;
    unsigned long value;
    struct cx25930_dev_t *pdev = (struct cx25930_dev_t *)sfile->private;

    down(&pdev->lock);

    if(cx25930_read_reg_count[pdev->index] >= 16) { ///< use burst read
        int n_size;
        u8  buf[CX25930_I2C_ARRAY_RW_MAX];
        u16 read_reg  = cx25930_read_reg_addr[pdev->index];
        int read_size = cx25930_read_reg_count[pdev->index]*2;

        while(read_size > 0) {
            if(read_size >= CX25930_I2C_ARRAY_RW_MAX)
                n_size = CX25930_I2C_ARRAY_RW_MAX;
            else
                n_size = read_size;

            ret = cx25930_i2c_array_read(pdev->index, read_reg, buf, n_size);
            if(ret < 0) {
                seq_printf(sfile, "[CX25930#%d] burst read 0x%04x...failed\n", pdev->index, read_reg);
                break;
            }
            else {
                for(i=0; i<n_size; i+=2)
                    seq_printf(sfile, "[CX25930#%d] 0x%04x => 0x%04x\n", pdev->index, (read_reg+i), *(((u16 *)buf)+(i/2)));
            }

            read_reg  += n_size;
            read_size -= n_size;
        }
    }
    else {
        for(i=0; i<cx25930_read_reg_count[pdev->index]; i++) {
            ret = ARTM_readRegister(&pdev->cx_intf, cx25930_read_reg_addr[pdev->index]+(2*i), &value);
            if(ret != TRUE) {
                seq_printf(sfile, "[CX25930#%d] read 0x%04x...failed\n", pdev->index, cx25930_read_reg_addr[pdev->index]+(2*i));
                break;
            }
            else
                seq_printf(sfile, "[CX25930#%d] 0x%04x => 0x%04x\n", pdev->index, cx25930_read_reg_addr[pdev->index]+(2*i), (u32)(value & 0xffff));
        }
    }
    seq_printf(sfile, "----------------------------------------------------------\n");
    seq_printf(sfile, "echo [reg] [read_count] to node to read data from register\n");

    up(&pdev->lock);

    return 0;
}

static int cx25930_proc_status_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct cx25930_dev_t *pdev = (struct cx25930_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[CX25930#%d]\n", pdev->index);
    seq_printf(sfile, "------------------\n");
    seq_printf(sfile, "SDI    VCH    LOS \n");
    seq_printf(sfile, "==================\n");
    for(i=0; i<CX25930_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*CX25930_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(CX25930_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_SDI(CX25930_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-6d %-6d %-7s\n", i, j, ((ARTM_ReadLOS(&pdev->cx_intf, i) == 0) ? "NO" : "YES"));
            }
        }
    }

    up(&pdev->lock);

    return 0;
}

static int cx25930_proc_reg_write_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "echo [reg] [data] to node to write data to register\n");
    return 0;
}

static int cx25930_proc_loopback_show(struct seq_file *sfile, void *v)
{
    int i;
    int ret = TRUE;
    unsigned long value[3];
    struct cx25930_dev_t *pdev = (struct cx25930_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[CX25930#%d]\n", pdev->index);
    seq_printf(sfile, "-----------------------------------------------------------------------------\n");

    for(i=0; i<CX25930_DEV_CH_MAX; i++) {
        ret &= ARTM_readRegister(&pdev->cx_intf, 0x2980+(i*0x10), &value[0]);               ///< AMS_CH0_TXREG1,  TX_Power Up/Down
        ret &= ARTM_readRegister(&pdev->cx_intf, 0x2984+(i*0x10), &value[1]);               ///< AMS_CH0_TXREG3,  Loopback Source
        ret &= ARTM_readRegister(&pdev->cx_intf, 0x2a2c+((value[1]&0x3)*0x80), &value[2]);  ///< AMS_CH0_RXREG23, Loopback Mode
        if(ret == TRUE) {
            if(value[0] & (0x1<<14))
                seq_printf(sfile, "SDO_CH#%d: TX Power Down\n", i);
            else
                seq_printf(sfile, "SDO_CH#%d: SDI_CH#%d %s Loopback\n", i, (int)(value[1]&0x3), ((((value[2]>>4)&0xf) == 8) ? "EQ " : "CDR"));
        }
        else
            seq_printf(sfile, "SDO_CH#%d: Unknown\n", i);
    }

    seq_printf(sfile, "-----------------------------------------------------------------------------\n");
    seq_printf(sfile, "echo [SDO_CH] [SDI_CH] [LBK_MODE] to node to control channel loopback function\n");
    seq_printf(sfile, "[SDO_CH]  : 0~3\n");
    seq_printf(sfile, "[SDI_CH]  : 0~3\n");
    seq_printf(sfile, "[LBK_MODE]: 0: OFF, 1: EQ_LBK, 2: CDR_LBK\n");

    up(&pdev->lock);

    return 0;
}

static int cx25930_proc_video_fmt_show(struct seq_file *sfile, void *v)
{
    int i, j;
    int ret = TRUE;
    int fmt_idx;
    struct cx25930_dev_t *pdev = (struct cx25930_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[CX25930#%d] Incoming Video Fromat\n", pdev->index);
    seq_printf(sfile, "===================================================================\n");
    seq_printf(sfile, "SDI   VCH   Width   Height  Pixel_Rate(KHz)  Prog/Inter   Frame_Rate\n");
    seq_printf(sfile, "===================================================================\n");

    for(i=0; i<CX25930_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*CX25930_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(CX25930_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_SDI(CX25930_VCH_MAP[j]) == i)) {
                ret &= ARTM_getVideoFormat(&pdev->cx_intf, i, &fmt_idx);
                if((ret == TRUE) && ((fmt_idx >= 1) && (fmt_idx < CX25930_VIDEO_FMT_IDX_MAX))) {
                    seq_printf(sfile, "%-5d %-5d %-7d %-7d %-16d %-12s %-11d\n",
                               i,
                               j,
                               cx25930_video_fmt_info[fmt_idx].active_width,
                               cx25930_video_fmt_info[fmt_idx].active_height,
                               cx25930_video_fmt_info[fmt_idx].pixel_rate,
                               ((cx25930_video_fmt_info[fmt_idx].prog == 1) ? "Progressive" : "Interlace"),
                               cx25930_video_fmt_info[fmt_idx].frame_rate);
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

static int cx25930_proc_reg_read_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx25930_proc_reg_read_show, PDE(inode)->data);
}

static int cx25930_proc_reg_write_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx25930_proc_reg_write_show, PDE(inode)->data);
}

static int cx25930_proc_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx25930_proc_status_show, PDE(inode)->data);
}

static int cx25930_proc_loopback_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx25930_proc_loopback_show, PDE(inode)->data);
}

static int cx25930_proc_video_fmt_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx25930_proc_video_fmt_show, PDE(inode)->data);
}

static struct file_operations cx25930_proc_reg_read_ops = {
    .owner  = THIS_MODULE,
    .open   = cx25930_proc_reg_read_open,
    .write  = cx25930_proc_reg_read_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations cx25930_proc_reg_write_ops = {
    .owner  = THIS_MODULE,
    .open   = cx25930_proc_reg_write_open,
    .write  = cx25930_proc_reg_write_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations cx25930_proc_status_ops = {
    .owner  = THIS_MODULE,
    .open   = cx25930_proc_status_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations cx25930_proc_loopback_ops = {
    .owner  = THIS_MODULE,
    .open   = cx25930_proc_loopback_open,
    .write  = cx25930_proc_loopback_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations cx25930_proc_video_fmt_ops = {
    .owner  = THIS_MODULE,
    .open   = cx25930_proc_video_fmt_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static int cx25930_cxcomm_register(int id)
{
    if(id >= CX25930_DEV_MAX)
        return -1;

    cx25930_dev[id].cx_intf.i2c_addr    = (u8)id;
    cx25930_dev[id].cx_intf.p_handle    = &cx25930_dev[id].cx_intf;
    cx25930_dev[id].cx_intf.write       = cx25930_cxcomm_i2c_write;
    cx25930_dev[id].cx_intf.read        = cx25930_cxcomm_i2c_read;
    cx25930_dev[id].cx_intf.burst_write = cx25930_cxcomm_i2c_burst_write;
    cx25930_dev[id].cx_intf.burst_read  = cx25930_cxcomm_i2c_burst_read;
    cx25930_dev[id].cx_intf.sleep       = cx25930_cxcomm_sleep;

    return 0;
}

static void cx25930_proc_remove(int id)
{
    if(id >= CX25930_DEV_MAX)
        return;

    if(cx25930_proc_root[id]) {
        if(cx25930_proc_reg_read[id]) {
            remove_proc_entry(cx25930_proc_reg_read[id]->name, cx25930_proc_root[id]);
            cx25930_proc_reg_read[id] = NULL;
        }

        if(cx25930_proc_reg_write[id]) {
            remove_proc_entry(cx25930_proc_reg_write[id]->name, cx25930_proc_root[id]);
            cx25930_proc_reg_write[id] = NULL;
        }

        if(cx25930_proc_status[id]) {
            remove_proc_entry(cx25930_proc_status[id]->name, cx25930_proc_root[id]);
            cx25930_proc_status[id] = NULL;
        }

        if(cx25930_proc_loopback[id]) {
            remove_proc_entry(cx25930_proc_loopback[id]->name, cx25930_proc_root[id]);
            cx25930_proc_loopback[id] = NULL;
        }

        if(cx25930_proc_video_fmt[id]) {
            remove_proc_entry(cx25930_proc_video_fmt[id]->name, cx25930_proc_root[id]);
            cx25930_proc_video_fmt[id] = NULL;
        }

        remove_proc_entry(cx25930_proc_root[id]->name, NULL);
        cx25930_proc_root[id] = NULL;
    }
}

static int cx25930_proc_init(int id)
{
    int ret = 0;

    if(id >= CX25930_DEV_MAX)
        return -1;

    /* root */
    cx25930_proc_root[id] = create_proc_entry(cx25930_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!cx25930_proc_root[id]) {
        printk("create proc node '%s' failed!\n", cx25930_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx25930_proc_root[id]->owner = THIS_MODULE;
#endif

    /* reg_read */
    cx25930_proc_reg_read[id] = create_proc_entry("reg_read", S_IRUGO|S_IXUGO, cx25930_proc_root[id]);
    if(!cx25930_proc_reg_read[id]) {
        printk("create proc node '%s/reg_read' failed!\n", cx25930_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    cx25930_proc_reg_read[id]->proc_fops = &cx25930_proc_reg_read_ops;
    cx25930_proc_reg_read[id]->data      = (void *)&cx25930_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx25930_proc_reg_read[id]->owner     = THIS_MODULE;
#endif

    /* reg_write */
    cx25930_proc_reg_write[id] = create_proc_entry("reg_write", S_IRUGO|S_IXUGO, cx25930_proc_root[id]);
    if(!cx25930_proc_reg_write[id]) {
        printk("create proc node '%s/reg_write' failed!\n", cx25930_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    cx25930_proc_reg_write[id]->proc_fops = &cx25930_proc_reg_write_ops;
    cx25930_proc_reg_write[id]->data      = (void *)&cx25930_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx25930_proc_reg_write[id]->owner     = THIS_MODULE;
#endif

    /* status */
    cx25930_proc_status[id] = create_proc_entry("status", S_IRUGO|S_IXUGO, cx25930_proc_root[id]);
    if(!cx25930_proc_status[id]) {
        printk("create proc node '%s/status' failed!\n", cx25930_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    cx25930_proc_status[id]->proc_fops = &cx25930_proc_status_ops;
    cx25930_proc_status[id]->data      = (void *)&cx25930_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx25930_proc_status[id]->owner     = THIS_MODULE;
#endif

    /* loopback */
    cx25930_proc_loopback[id] = create_proc_entry("loopback", S_IRUGO|S_IXUGO, cx25930_proc_root[id]);
    if(!cx25930_proc_loopback[id]) {
        printk("create proc node '%s/loopback' failed!\n", cx25930_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    cx25930_proc_loopback[id]->proc_fops = &cx25930_proc_loopback_ops;
    cx25930_proc_loopback[id]->data      = (void *)&cx25930_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx25930_proc_loopback[id]->owner     = THIS_MODULE;
#endif

    /* frame format */
    cx25930_proc_video_fmt[id] = create_proc_entry("video_format", S_IRUGO|S_IXUGO, cx25930_proc_root[id]);
    if(!cx25930_proc_video_fmt[id]) {
        printk("create proc node '%s/video_format' failed!\n", cx25930_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    cx25930_proc_video_fmt[id]->proc_fops = &cx25930_proc_video_fmt_ops;
    cx25930_proc_video_fmt[id]->data      = (void *)&cx25930_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx25930_proc_video_fmt[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    cx25930_proc_remove(id);
    return ret;
}

static int cx25930_miscdev_open(struct inode *inode, struct file *file)
{
    int i, ret = 0;
    struct cx25930_dev_t *pdev = NULL;

    /* lookup device */
    for(i=0; i<dev_num; i++) {
        if(cx25930_dev[i].miscdev.minor == iminor(inode)) {
            pdev = &cx25930_dev[i];
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

static int cx25930_miscdev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long cx25930_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct cx25930_dev_t *pdev = (struct cx25930_dev_t *)file->private_data;

    down(&pdev->lock);

    if(_IOC_TYPE(cmd) != CX25930_IOC_MAGIC) {
        ret = -ENOIOCTLCMD;
        goto exit;
    }

    switch(cmd) {
        case CX25930_GET_NOVID:
            {
                struct cx25930_ioc_data_t ioc_data;
                int novid;

                if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_data.sdi_ch >= CX25930_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                novid = ARTM_ReadLOS(&pdev->cx_intf, ioc_data.sdi_ch);

                ioc_data.data = novid;
                ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            }
            break;

        case CX25930_GET_VIDEO_FMT:
            {
                struct cx25930_ioc_vfmt_t ioc_vfmt;
                int fmt_idx = 0;

                if(copy_from_user(&ioc_vfmt, (void __user *)arg, sizeof(ioc_vfmt))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vfmt.sdi_ch >= CX25930_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                ARTM_getVideoFormat(&pdev->cx_intf, ioc_vfmt.sdi_ch, &fmt_idx);
                if((fmt_idx < 0) || (fmt_idx >= CX25930_VIDEO_FMT_IDX_MAX))
                    fmt_idx = 0;    ///< unknown format

                ioc_vfmt.active_width  = cx25930_video_fmt_info[fmt_idx].active_width;
                ioc_vfmt.active_height = cx25930_video_fmt_info[fmt_idx].active_height;
                ioc_vfmt.total_width   = cx25930_video_fmt_info[fmt_idx].total_width;
                ioc_vfmt.total_height  = cx25930_video_fmt_info[fmt_idx].total_height;
                ioc_vfmt.pixel_rate    = cx25930_video_fmt_info[fmt_idx].pixel_rate;
                ioc_vfmt.bit_width     = cx25930_video_fmt_info[fmt_idx].bit_width;
                ioc_vfmt.prog          = cx25930_video_fmt_info[fmt_idx].prog;
                ioc_vfmt.frame_rate    = cx25930_video_fmt_info[fmt_idx].frame_rate;
                ioc_vfmt.bit_rate      = cx25930_video_fmt_info[fmt_idx].bit_rate;
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

static struct file_operations cx25930_miscdev_fops = {
    .owner          = THIS_MODULE,
    .open           = cx25930_miscdev_open,
    .release        = cx25930_miscdev_release,
    .unlocked_ioctl = cx25930_miscdev_ioctl,
};

static int cx25930_miscdev_init(int id)
{
    int ret;

    if(id >= CX25930_DEV_MAX)
        return -1;

    /* clear */
    memset(&cx25930_dev[id].miscdev, 0, sizeof(struct miscdevice));

    /* create device node */
    cx25930_dev[id].miscdev.name  = cx25930_dev[id].name;
    cx25930_dev[id].miscdev.minor = MISC_DYNAMIC_MINOR;
    cx25930_dev[id].miscdev.fops  = (struct file_operations *)&cx25930_miscdev_fops;
    ret = misc_register(&cx25930_dev[id].miscdev);
    if(ret) {
        printk("create %s misc device node failed!\n", cx25930_dev[id].name);
        cx25930_dev[id].miscdev.name = 0;
        goto exit;
    }

exit:
    return ret;
}

static void cx25930_hw_reset(void)
{
    if(!rstb_used || !init)
        return;

    /* Active Low */
    plat_gpio_set_value(rstb_used, 0);

    udelay(100);

    /* Active High and release reset */
    plat_gpio_set_value(rstb_used, 1);

    udelay(200);
}

static int cx25930_device_init(int id)
{
    int i;
    int ret = TRUE;

    if(id >= CX25930_DEV_MAX)
        return -1;

    if(!init)
        goto exit;

    ARTM_enableXtalOutput(&cx25930_dev[id].cx_intf);

    /* video init */
    ret &= ARTM_initialize(&cx25930_dev[id].cx_intf, cab_len[id], input_rate[id], vout_format[id]);
    if(ret != TRUE)
        goto exit;

    for(i=0; i<CX25930_DEV_CH_MAX; i++) {
        /* clear all info */
        memset(&cx25930_dev[id].re_info[i], 0, sizeof(struct artm_re_info_t));

        cx25930_dev[id].re_info[i].input_rate = input_rate[id];
        cx25930_dev[id].re_info[i].cdr_ph_up  = 1;
    }

    /* audio init */
    if(ARTM_get_Chip_RevID(&cx25930_dev[id].cx_intf) != ARTM_CHIP_REV_11ZP) {
        CX25930_AUDIO_EXTRACTION aud_extract;

        aud_extract.extraction_mode     = CX25930_AUTO_DETECT;
        aud_extract.extended_ws_format  = CX25930_EXTENDED_I2S_FORMAT;
        aud_extract.extended_mode       = CX25930_EXTENDED_I2S_4CH;
        aud_extract.slot_width          = sample_size;
        aud_extract.sample_rate         = sample_rate;
        aud_extract.enable_i2s          = CX25930_ENABLE_0;
        aud_extract.master_mode         = 1;
        aud_extract.channel             = 0;

        ret &= ARTM_AudioInit(&cx25930_dev[id].cx_intf, &aud_extract);
        if(ret != TRUE)
            goto exit;

        ret &= ARTM_PinRemap_I2S(&cx25930_dev[id].cx_intf, 0, 1, 1, 1, 1);
        if(ret != TRUE)
            goto exit;

        WriteBitField(&cx25930_dev[id].cx_intf, 0x1024 , 0, 0, 0);
    }

exit:
    printk("CX25930#%d Init...%s!\n", id, ((ret == TRUE) ? "OK" : "Fail"));

    return ((ret == TRUE) ? 0 : -1);
}

static int cx25930_watchdog_thread(void *data)
{
    int i, ch, fmt_idx, ret;
    struct cx25930_dev_t *pdev;
    int chip_rev;

    do {
            /* check cx25930 status */
            for(i=0; i<dev_num; i++) {
                pdev = &cx25930_dev[i];

                down(&pdev->lock);

                chip_rev = ARTM_get_Chip_RevID(&pdev->cx_intf);

                for(ch=0; ch<CX25930_DEV_CH_MAX; ch++) {
                    pdev->re_info[ch].cur_plugin  = (ARTM_ReadLOS(&pdev->cx_intf, ch) == 0) ? 1 : 0;
                    if((pdev->re_info[ch].cur_plugin == 1) && (pdev->re_info[ch].pre_plugin == 0)) {        ///< cable plugged-in
                        if(chip_rev == ARTM_CHIP_REV_11ZP) {
                            ARTM_checkRasterEngineStatus_11ZP(&pdev->cx_intf, &pdev->re_info[ch], ch);
                            ARTM_resetRE(&pdev->cx_intf, ch);

                            if(pdev->re_info[ch].force_blue) {
                                ARTM_remove_forced_bluescreen(&pdev->cx_intf, ch);
                                pdev->re_info[ch].force_blue = 0;
                            }
                        }
                        else {
                            ARTM_RasterEngineDisable(&pdev->cx_intf, ch);
                            ARTM_EQ_Check(&pdev->cx_intf, ch, pdev->re_info[ch].input_rate);
                            ARTM_RasterEngineEnable(&pdev->cx_intf, ch);
                            cx25930_cxcomm_sleep(200);
                            if(ARTM_checkRasterEngineStatus(&pdev->cx_intf, &pdev->re_info[ch], ch) == 1) { ///< lock
                    	        ARTM_remove_forced_bluescreen(&pdev->cx_intf, ch);
                    	        ARTM_AudioInitCh(&pdev->cx_intf, ch);
                    	        pdev->re_info[ch].force_blue = 0;
                    	    }
                        }
                    }
                    else if((pdev->re_info[ch].cur_plugin == 1) && (pdev->re_info[ch].pre_plugin == 1)) {   ///< cable is connected
                        if(chip_rev == ARTM_CHIP_REV_11ZP) {
                            ARTM_checkRasterEngineStatus_11ZP(&pdev->cx_intf, &pdev->re_info[ch], ch);
                        }
                        else {
                            if(ARTM_checkRasterEngineStatus(&pdev->cx_intf, &pdev->re_info[ch], ch) == 1) { ///< lock
                                if(pdev->re_info[ch].force_blue) {
                                    ARTM_remove_forced_bluescreen(&pdev->cx_intf, ch);
                                    ARTM_AudioInitCh(&pdev->cx_intf, ch);
                                    pdev->re_info[ch].force_blue = 0;
                                }
                            }
                            else {
                                if(pdev->re_info[ch].force_blue == 0) {
                                    ARTM_force_bluescreen(&pdev->cx_intf, ch, pdev->re_info[ch].input_rate);
                                    pdev->re_info[ch].force_blue = 1;
                                }
                            }
                        }

                        ret = ARTM_getVideoFormat(&pdev->cx_intf, ch, &fmt_idx);
                        if(ret == TRUE) {
                            if((fmt_idx == 0) && (chip_rev == ARTM_CHIP_REV_11ZP))
                                ARTM_resetRE(&pdev->cx_intf, ch);

                            /* notify current video format */
                            if(notify && pdev->vfmt_notify && (fmt_idx > 0) && (fmt_idx < CX25930_VIDEO_FMT_IDX_MAX))
                                pdev->vfmt_notify(i, ch, &cx25930_video_fmt_info[fmt_idx]);
                        }

                        /* notify current video on */
                        if(notify && pdev->vlos_notify && (fmt_idx > 0) && (fmt_idx < CX25930_VIDEO_FMT_IDX_MAX))
                            pdev->vlos_notify(i, ch, 0);
                    }
                    else {  ///< cable not connected or is plugged-out
                        if((pdev->re_info[ch].force_blue == 0) || (ARTM_RasterEngine_Format_Detected(&pdev->cx_intf, ch) != 0)) {
                            ARTM_force_bluescreen(&pdev->cx_intf, ch, pdev->re_info[ch].input_rate);
                            pdev->re_info[ch].force_blue = 1;
                        }

                        /* notify current video loss */
                        if(notify && pdev->vlos_notify)
                            pdev->vlos_notify(i, ch, 1);
                    }

                    pdev->re_info[ch].pre_plugin = pdev->re_info[ch].cur_plugin;
                }

                up(&pdev->lock);
            }

            /* sleep 1 second */
            schedule_timeout_interruptible(msecs_to_jiffies(1000));

    } while (!kthread_should_stop());

    return 0;
}

static int __init cx25930_init(void)
{
    int i, ret;

    /* platfrom check */
    plat_identifier_check();

    /* check device number */
    if(dev_num > CX25930_DEV_MAX) {
        printk("CX25930 dev_num=%d invalid!!(Max=%d)\n", dev_num, CX25930_DEV_MAX);
        return -1;
    }

    /* check input data rate */
    for(i=0; i<dev_num; i++) {
        if((input_rate[i] < 0) || (input_rate[i] >= CX25930_INPUT_RATE_GEN3)) {
            printk("CX25930#%d input_rate=%d not support!!\n", i, input_rate[i]);
            return -1;
        }
    }

    /* check video output format */
    for(i=0; i<dev_num; i++) {
        if((vout_format[i] < 0) || (vout_format[i] >= CX25930_VOUT_FORMAT_MAX)) {
            printk("CX25930#%d vout_format=%d not support!!\n", i, vout_format[i]);
            return -1;
        }
    }

    /* check cable length */
    for(i=0; i<dev_num; i++) {
        if((cab_len[i] < 0) || (cab_len[i] >= CX25930_CAB_LEN_MAX))
            cab_len[i] = 0;
    }

    /* register i2c client for contol cx25930 */
    ret = cx25930_i2c_register();
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
    ret = cx25930_create_ch_map(ch_map);
    if(ret < 0)
        goto err;

    /* hardware reset for all device */
    cx25930_hw_reset();

    /* Display driver library version */
    printk("CX25930 ==> V%d.%d.%d\n", CX25930_VER_MAJOR, CX25930_VER_MINOR, CX25930_VER_REL);

    /* CX25930 init */
    for(i=0; i<dev_num; i++) {
        cx25930_dev[i].index = i;

        sprintf(cx25930_dev[i].name, "cx25930.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&cx25930_dev[i].lock, 1);
#else
        init_MUTEX(&cx25930_dev[i].lock);
#endif
        ret = cx25930_cxcomm_register(i);
        if(ret < 0)
            goto err;

        ret = cx25930_proc_init(i);
        if(ret < 0)
            goto err;

        ret = cx25930_miscdev_init(i);
        if(ret < 0)
            goto err;

        ret = cx25930_device_init(i);
        if(ret < 0)
            goto err;
    }

    if(init) {
        /* init cx25930 watchdog thread for check raster engine status */
        cx25930_wd = kthread_create(cx25930_watchdog_thread, NULL, "cx25930_wd");
        if(!IS_ERR(cx25930_wd))
            wake_up_process(cx25930_wd);
        else {
            printk("create cx25930 watchdog thread failed!!\n");
            cx25930_wd = 0;
            ret = -EFAULT;
            goto err;
        }
    }

    /* register audio function for platform used */
    cx25930_audio_funs.sound_switch       = cx25930_sound_switch;
    cx25930_audio_funs.digital_codec_name = cx25930_dev[0].name;
    plat_audio_register_function(&cx25930_audio_funs);

    return 0;

err:
    if(cx25930_wd)
        kthread_stop(cx25930_wd);

    cx25930_i2c_unregister();
    for(i=0; i<CX25930_DEV_MAX; i++) {
        if(cx25930_dev[i].miscdev.name)
            misc_deregister(&cx25930_dev[i].miscdev);

        cx25930_proc_remove(i);
    }

    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    return ret;
}

static void __exit cx25930_exit(void)
{
    int i;

    /* stop cx25930 watchdog thread */
    if(cx25930_wd)
        kthread_stop(cx25930_wd);

    cx25930_i2c_unregister();

    for(i=0; i<CX25930_DEV_MAX; i++) {
        /* remove device node */
        if(cx25930_dev[i].miscdev.name)
            misc_deregister(&cx25930_dev[i].miscdev);

        /* remove proc node */
        cx25930_proc_remove(i);
    }

    /* release gpio pin */
    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    plat_audio_deregister_function();
}

module_init(cx25930_init);
module_exit(cx25930_exit);

MODULE_DESCRIPTION("Grain Media CX25930 4CH SDI Receiver Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
