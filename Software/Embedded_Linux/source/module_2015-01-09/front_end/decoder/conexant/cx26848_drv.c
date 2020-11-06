/**
 * @file cx26848_drv.c
 * Conexant CX26848 8-CH 960H Video/Audio Decoder with Integrated Audio DAC Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.7 $
 * $Date: 2014/07/29 05:36:08 $
 *
 * ChangeLog:
 *  $Log: cx26848_drv.c,v $
 *  Revision 1.7  2014/07/29 05:36:08  shawn_hu
 *  Add the module parameter of "ibus" to specify which I2C bus has the decoder attached.
 *
 *  Revision 1.6  2014/02/24 12:53:43  jerson_l
 *  1. modify 2CH mode to output dual edge format and correct channel mapping table
 *
 *  Revision 1.5  2013/11/27 05:32:59  jerson_l
 *  1. support notify procedure
 *
 *  Revision 1.4  2013/10/11 10:27:12  jerson_l
 *  1. update platform gpio pin register api
 *
 *  Revision 1.3  2013/10/01 06:12:00  jerson_l
 *  1. switch to new channel mapping mechanism
 *
 *  Revision 1.2  2013/09/16 11:03:23  jerson_l
 *  1. refine code
 *
 *  Revision 1.1.1.1  2013/08/09 01:43:10  gavinpe
 *  add cx26848 driver
 *
 *  Revision 1.1.1.1  2013/08/06 09:29:13  Gavin Peng
 *  add front_end device driver for CX26848
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
#include "conexant/cx26848.h"                       ///< from module/include/front_end/decoder
#include "lib_cx26848/Cx26848VidDecoder.h"

#define CX26848_CLKIN               24000000
#define CH_IDENTIFY(id, vin, vp)    (((id)&0xf)|(((vin)&0xf)<<4)|(((vp)&0xf)<<8))
#define CH_IDENTIFY_ID(x)           ((x)&0xf)
#define CH_IDENTIFY_VIN(x)          (((x)>>4)&0xf)
#define CH_IDENTIFY_VPORT(x)        (((x)>>8)&0xf)

/* device number */
static int dev_num = CX26848_DEV_MAX;
module_param(dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dev_num, "Device Number: 1~4");

/* device i2c bus */
static int ibus = 0;
module_param(ibus, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ibus, "Device I2C Bus: 0~4");

/* device i2c address */
static ushort iaddr[CX26848_DEV_MAX] = {0x8e, 0x8c, 0x8a, 0x88};
module_param_array(iaddr, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iaddr, "Device I2C Address");

/* clock port used */
static int clk_used = 0x3;
module_param(clk_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_used, "External Clock Port: BIT0:EXT_CLK0, BIT1:EXT_CLK1");

/* video input mode */
static ushort vmode[CX26848_DEV_MAX] = {[0 ... (CX26848_DEV_MAX - 1)] = CX26848_VMODE_NTSC_720H_4CH};
module_param_array(vmode, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vmode, "Video Mode");

/* clock source */
static int clk_src = PLAT_EXTCLK_SRC_PLL4OUT1_DIV2;
module_param(clk_src, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_src, "External Clock Source: 0:PLL1OUT1 1:PLL1OUT1/2 2:PLL4OUT2 3:PLL4OUT1/2 4:PLL3");

/* clock frequency */
static int clk_freq = CX26848_CLKIN;   ///< 24MHz
module_param(clk_freq, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_freq, "External Clock Frequency(Hz)");

/* device init */
static int init = 1;
module_param(init, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init, "device init: 0:no-init  1:init");

/* device notify */
static int notify = 1;
module_param(notify, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(notify, "Device Notify => 0:Disable 1:Enable");

/* device use gpio as rstb pin */
static int rstb_used = PLAT_GPIO_ID_X_CAP_RST;
module_param(rstb_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rstb_used, "Use GPIO as RSTB Pin: 0:None, 1:X_CAP_RST");

/* device channl mapping */
static int ch_map = 0;
module_param(ch_map, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_map, "channel mapping");

/******************************************************************************
 GM8210/GM8287 EVB Channel and VIN Mapping Table (Socket Board)

 VCH#0 ------> VIN#0 -----------------> VPortA -------> X_CAP#1
 VCH#1 ------> VIN#1 ---------|
 VCH#2 ------> VIN#2 ---------|
 VCH#3 ------> VIN#3 ---------|
 VCH#4 ------> VIN#4 -----------------> VPortB -------> X_CAP#0
 VCH#5 ------> VIN#5 ---------|
 VCH#6 ------> VIN#6 ---------|
 VCH#7 ------> VIN#7 ---------|

 ==============================================================================
 GM8210 EVB Channel and VIN Mapping Table (System Board 1V1)

 VCH#7 ------> VIN#0 -----------------> VPortA -------> X_CAP#0
 VCH#6 ------> VIN#1 ---------|
 VCH#5 ------> VIN#2 ---------|
 VCH#4 ------> VIN#3 ---------|
 VCH#3 ------> VIN#4 -----------------> VPortB -------> X_CAP#1
 VCH#2 ------> VIN#5 ---------|
 VCH#1 ------> VIN#6 ---------|
 VCH#0 ------> VIN#7 ---------|

 ==============================================================================
 GM8287 EVB Channel and VIN Mapping Table (System Board)
 GM8283 EVB Channel and VIN Mapping Table (Socket Board)

 VCH#0 ------> VIN#0 -----------------> VPortA -------> X_CAP#0
 VCH#1 ------> VIN#1 ---------|
 VCH#2 ------> VIN#2 ---------|
 VCH#3 ------> VIN#3 ---------|
 VCH#4 ------> VIN#4 -----------------> VPortB -------> X_CAP#1
 VCH#5 ------> VIN#5 ---------|
 VCH#6 ------> VIN#6 ---------|
 VCH#7 ------> VIN#7 ---------|

*******************************************************************************/

struct cx26848_dev_t {
    int                 index;      ///< device index
    char                name[16];   ///< device name
    struct semaphore    lock;       ///< device locker
    struct miscdevice   miscdev;    ///< device node for ioctl access
    CX_COMMUNICATION    cx_intf;    ///< conexant common interface
    CX_COLOR_CTRL       color_ctrl; ///< image color setting

    int                 (*vlos_notify)(int id, int vin, int vlos);
};

static struct cx26848_dev_t   cx26848_dev[CX26848_DEV_MAX];
static struct proc_dir_entry *cx26848_proc_root[CX26848_DEV_MAX]      = {[0 ... (CX26848_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx26848_proc_vmode[CX26848_DEV_MAX]     = {[0 ... (CX26848_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx26848_proc_status[CX26848_DEV_MAX]    = {[0 ... (CX26848_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx26848_proc_bri[CX26848_DEV_MAX]       = {[0 ... (CX26848_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx26848_proc_con[CX26848_DEV_MAX]       = {[0 ... (CX26848_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx26848_proc_sat[CX26848_DEV_MAX]       = {[0 ... (CX26848_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx26848_proc_hue[CX26848_DEV_MAX]       = {[0 ... (CX26848_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx26848_proc_volume[CX26848_DEV_MAX]    = {[0 ... (CX26848_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx26848_proc_output_ch[CX26848_DEV_MAX] = {[0 ... (CX26848_DEV_MAX - 1)] = NULL};

static struct i2c_client  *cx26848_i2c_client = NULL;
static struct task_struct *cx26848_wd         = NULL;
static u32 CX26848_VCH_MAP[CX26848_DEV_MAX*CX26848_DEV_CH_MAX];

/* Video Output Port Channel Selection */
static unsigned long cx26848_vport_channel_sel_1ch[] = {
    0, 0, 0, 0, ///< VPort_A
    1, 1, 1, 1, ///< VPort_B
    2, 2, 2, 2, ///< VPort_C
    3, 3, 3, 3  ///< VPort_D
};

static unsigned long cx26848_vport_channel_sel_2ch[] = {
    0, 1, 0, 1, ///< VPort_A
    2, 3, 2, 3, ///< VPort_B
    4, 5, 4, 5, ///< VPort_C
    6, 7, 6, 7  ///< VPort_D
};

static unsigned long cx26848_vport_channel_sel_4ch[] = {
    0, 1, 2, 3, ///< VPort_A
    4, 5, 6, 7, ///< VPort_B
    0, 1, 2, 3, ///< VPort_C
    4, 5, 6, 7  ///< VPort_D
};

static int cx26848_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    cx26848_i2c_client = client;
    return 0;
}

static int cx26848_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id cx26848_i2c_id[] = {
    { "cx26848_i2c", 0 },
    { }
};

static struct i2c_driver cx26848_i2c_driver = {
    .driver = {
        .name  = "CX26848_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = cx26848_i2c_probe,
    .remove   = cx26848_i2c_remove,
    .id_table = cx26848_i2c_id
};

static int cx26848_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&cx26848_i2c_driver);
    if(ret < 0) {
        printk("CX26848 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = iaddr[0]>>1;
    strlcpy(info.type, "cx26848_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("CX26848 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("CX26848 can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&cx26848_i2c_driver);
   return -ENODEV;
}

static void cx26848_i2c_unregister(void)
{
    i2c_unregister_device(cx26848_i2c_client);
    i2c_del_driver(&cx26848_i2c_driver);
    cx26848_i2c_client = NULL;
}

static int cx26848_create_ch_map(int map_id)
{
    int i;

    switch(map_id) {
        case 0: ///< for GM8210/GM8287 Socket Board, 4CH Mode
            for(i=0; i<dev_num; i++) {
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+0] = CH_IDENTIFY(dev_num-i-1, 0, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+1] = CH_IDENTIFY(dev_num-i-1, 1, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+2] = CH_IDENTIFY(dev_num-i-1, 2, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+3] = CH_IDENTIFY(dev_num-i-1, 3, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+4] = CH_IDENTIFY(dev_num-i-1, 4, CX26848_DEV_VPORT_B);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+5] = CH_IDENTIFY(dev_num-i-1, 5, CX26848_DEV_VPORT_B);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+6] = CH_IDENTIFY(dev_num-i-1, 6, CX26848_DEV_VPORT_B);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+7] = CH_IDENTIFY(dev_num-i-1, 7, CX26848_DEV_VPORT_B);
            }
            break;

        case 1: ///< for GM8210 System Board, 4CH Mode
            for(i=0; i<dev_num; i++) {
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+7] = CH_IDENTIFY(i, 0, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+6] = CH_IDENTIFY(i, 1, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+5] = CH_IDENTIFY(i, 2, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+4] = CH_IDENTIFY(i, 3, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+3] = CH_IDENTIFY(i, 4, CX26848_DEV_VPORT_B);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+2] = CH_IDENTIFY(i, 5, CX26848_DEV_VPORT_B);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+1] = CH_IDENTIFY(i, 6, CX26848_DEV_VPORT_B);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+0] = CH_IDENTIFY(i, 7, CX26848_DEV_VPORT_B);
            }
            break;

        case 3: ///< General, 1CH Mode
            for(i=0; i<dev_num; i++) {
                CX26848_VCH_MAP[(i*CX26848_DEV_VPORT_MAX)+0] = CH_IDENTIFY(i, 0, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_VPORT_MAX)+1] = CH_IDENTIFY(i, 1, CX26848_DEV_VPORT_B);
                CX26848_VCH_MAP[(i*CX26848_DEV_VPORT_MAX)+2] = CH_IDENTIFY(i, 2, CX26848_DEV_VPORT_C);
                CX26848_VCH_MAP[(i*CX26848_DEV_VPORT_MAX)+3] = CH_IDENTIFY(i, 3, CX26848_DEV_VPORT_D);
            }
            break;

        case 4: ///< General, 2CH Mode
            for(i=0; i<dev_num; i++) {
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+0] = CH_IDENTIFY(i, 0, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+1] = CH_IDENTIFY(i, 1, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+2] = CH_IDENTIFY(i, 2, CX26848_DEV_VPORT_B);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+3] = CH_IDENTIFY(i, 3, CX26848_DEV_VPORT_B);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+4] = CH_IDENTIFY(i, 4, CX26848_DEV_VPORT_C);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+5] = CH_IDENTIFY(i, 5, CX26848_DEV_VPORT_C);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+6] = CH_IDENTIFY(i, 6, CX26848_DEV_VPORT_D);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+7] = CH_IDENTIFY(i, 7, CX26848_DEV_VPORT_D);
            }
            break;

        case 2: ///< for GM8287 System Board, GM8283 Socket Board, 4CH Mode
        default:
            for(i=0; i<dev_num; i++) {
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+0] = CH_IDENTIFY(i, 0, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+1] = CH_IDENTIFY(i, 1, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+2] = CH_IDENTIFY(i, 2, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+3] = CH_IDENTIFY(i, 3, CX26848_DEV_VPORT_A);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+4] = CH_IDENTIFY(i, 4, CX26848_DEV_VPORT_B);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+5] = CH_IDENTIFY(i, 5, CX26848_DEV_VPORT_B);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+6] = CH_IDENTIFY(i, 6, CX26848_DEV_VPORT_B);
                CX26848_VCH_MAP[(i*CX26848_DEV_CH_MAX)+7] = CH_IDENTIFY(i, 7, CX26848_DEV_VPORT_B);
            }
            break;
    }

    return 0;
}

int cx26848_get_vch_id(int id, CX26848_DEV_VPORT_T vport, int vport_seq)
{
    int i;
    int vport_chnum;

    if(id >= CX26848_DEV_MAX || vport >= CX26848_DEV_VPORT_MAX || vport_seq >= 4)
        return 0;

    /* get vport max channel number */
    switch(vmode[id]) {
        case CX26848_VMODE_NTSC_720H_1CH:
        case CX26848_VMODE_PAL_720H_1CH:
        case CX26848_VMODE_NTSC_960H_1CH:
        case CX26848_VMODE_PAL_960H_1CH:
            vport_chnum = 1;
            break;
        case CX26848_VMODE_NTSC_720H_2CH:
        case CX26848_VMODE_PAL_720H_2CH:
        case CX26848_VMODE_NTSC_960H_2CH:
        case CX26848_VMODE_PAL_960H_2CH:
            vport_chnum = 2;
            break;
        case CX26848_VMODE_NTSC_720H_4CH:
        case CX26848_VMODE_PAL_720H_4CH:
        case CX26848_VMODE_NTSC_960H_4CH:
        case CX26848_VMODE_PAL_960H_4CH:
        default:
            vport_chnum = 4;
            break;
    }

    for(i=0; i<(dev_num*CX26848_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(CX26848_VCH_MAP[i]) == id) &&
           (CH_IDENTIFY_VPORT(CX26848_VCH_MAP[i]) == vport) &&
           ((CH_IDENTIFY_VIN(CX26848_VCH_MAP[i])%vport_chnum) == vport_seq)) {
            return i;
        }
    }

    return 0;
}
EXPORT_SYMBOL(cx26848_get_vch_id);

int cx26848_video_get_mode(u8 id)
{
    if(id >= CX26848_DEV_MAX)
            return -1;

    return vmode[id];
}
EXPORT_SYMBOL(cx26848_video_get_mode);

int cx26848_vin_to_vch(int id, int vin_idx)
{
    int i;

    if(id >= CX26848_DEV_MAX || vin_idx >= CX26848_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*CX26848_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(CX26848_VCH_MAP[i]) == id) && (CH_IDENTIFY_VIN(CX26848_VCH_MAP[i]) == vin_idx))
            return i;
    }

    return 0;
}
EXPORT_SYMBOL(cx26848_vin_to_vch);

int cx26848_notify_vlos_register(int id, int (*nt_func)(int, int, int))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&cx26848_dev[id].lock);

    cx26848_dev[id].vlos_notify = nt_func;

    up(&cx26848_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(cx26848_notify_vlos_register);

void cx26848_notify_vlos_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&cx26848_dev[id].lock);

    cx26848_dev[id].vlos_notify = NULL;

    up(&cx26848_dev[id].lock);
}
EXPORT_SYMBOL(cx26848_notify_vlos_deregister);

int cx26848_video_set_mode(u8 id, CX26848_VMODE_T v_mode)
{
    int i, ret = FALSE;
    unsigned long size, output_rate;
    unsigned char num_channel;
    int video_std[CX26848_DEV_CH_MAX];
    int video_res[CX26848_DEV_CH_MAX];
    unsigned long *src_sel;
    char vmode_str[32] = {0};

    if(id >= dev_num)
        return -1;

    /* Video Standard and Resolution */
    switch(v_mode) {
        case CX26848_VMODE_NTSC_720H_1CH:
        case CX26848_VMODE_PAL_720H_1CH:
            num_channel = 1;
            size        = 720;
            output_rate = OUTPUT_RATE_27_MHZ;
            src_sel     = (unsigned long *)cx26848_vport_channel_sel_1ch;
            for(i=0; i<CX26848_DEV_CH_MAX; i++) {
                video_std[i] = (v_mode == CX26848_VMODE_NTSC_720H_1CH) ? NTSC_VIDEO_STD : PAL_VIDEO_STD;
                video_res[i] = VIDEO_EXTEND_D1;
            }
            sprintf(vmode_str, "%s 720H 27MHz", ((v_mode==CX26848_VMODE_NTSC_720H_1CH) ? "NTSC" : "PAL"));
            break;
        case CX26848_VMODE_NTSC_720H_2CH:
        case CX26848_VMODE_PAL_720H_2CH:
            num_channel = 2;
            size        = 720;
            output_rate = OUTPUT_RATE_27_MHZ;
            src_sel     = (unsigned long *)cx26848_vport_channel_sel_2ch;
            for(i=0; i<CX26848_DEV_CH_MAX; i++) {
                video_std[i] = (v_mode == CX26848_VMODE_NTSC_720H_2CH) ? NTSC_VIDEO_STD : PAL_VIDEO_STD;
                video_res[i] = VIDEO_EXTEND_D1;
            }
            sprintf(vmode_str, "%s 720H 54MHz", ((v_mode==CX26848_VMODE_NTSC_720H_2CH) ? "NTSC" : "PAL"));
            break;
        case CX26848_VMODE_NTSC_720H_4CH:
        case CX26848_VMODE_PAL_720H_4CH:
            num_channel = 4;
            size        = 720;
            output_rate = OUTPUT_RATE_108_MHZ;
            src_sel     = (unsigned long *)cx26848_vport_channel_sel_4ch;
            for(i=0; i<CX26848_DEV_CH_MAX; i++) {
                video_std[i] = (v_mode == CX26848_VMODE_NTSC_720H_4CH) ? NTSC_VIDEO_STD : PAL_VIDEO_STD;
                video_res[i] = VIDEO_EXTEND_D1;
            }
            sprintf(vmode_str, "%s 720H 108MHz", ((v_mode==CX26848_VMODE_NTSC_720H_4CH) ? "NTSC" : "PAL"));
            break;
        case CX26848_VMODE_NTSC_960H_1CH:
        case CX26848_VMODE_PAL_960H_1CH:
            num_channel = 1;
            size        = 960;
            output_rate = OUTPUT_RATE_27_MHZ;
            src_sel     = (unsigned long *)cx26848_vport_channel_sel_1ch;
            for(i=0; i<CX26848_DEV_CH_MAX; i++) {
                video_std[i] = (v_mode == CX26848_VMODE_NTSC_960H_1CH) ? NTSC_EFFIO_STD : PAL_EFFIO_STD;
                video_res[i] = VIDEO_EXTEND_960H;
            }
            sprintf(vmode_str, "%s 960H 36MHz", ((v_mode==CX26848_VMODE_NTSC_960H_1CH) ? "NTSC" : "PAL"));
            break;
        case CX26848_VMODE_NTSC_960H_2CH:
        case CX26848_VMODE_PAL_960H_2CH:
            num_channel = 2;
            size        = 960;
            output_rate = OUTPUT_RATE_27_MHZ;
            src_sel     = (unsigned long *)cx26848_vport_channel_sel_2ch;
            for(i=0; i<CX26848_DEV_CH_MAX; i++) {
                video_std[i] = (v_mode == CX26848_VMODE_NTSC_960H_2CH) ? NTSC_EFFIO_STD : PAL_EFFIO_STD;
                video_res[i] = VIDEO_EXTEND_960H;
            }
            sprintf(vmode_str, "%s 960H 72MHz", ((v_mode==CX26848_VMODE_NTSC_960H_2CH) ? "NTSC" : "PAL"));
            break;
        case CX26848_VMODE_NTSC_960H_4CH:
        case CX26848_VMODE_PAL_960H_4CH:
            num_channel = 4;
            size        = 960;
            output_rate = OUTPUT_RATE_144_MHZ;
            src_sel     = (unsigned long *)cx26848_vport_channel_sel_4ch;
            for(i=0; i<CX26848_DEV_CH_MAX; i++) {
                video_std[i] = (v_mode == CX26848_VMODE_NTSC_960H_4CH) ? NTSC_EFFIO_STD : PAL_EFFIO_STD;
                video_res[i] = VIDEO_EXTEND_960H;
            }
            sprintf(vmode_str, "%s 960H 144MHz", ((v_mode==CX26848_VMODE_NTSC_960H_4CH) ? "NTSC" : "PAL"));
            break;
         default:
            printk("CX26848#%d vmode=%d not support!\n", id, v_mode);
            return -1;
    }

    down(&cx26848_dev[id].lock);

    /* Software Reset */
    CX2684x_Reset(&cx26848_dev[id].cx_intf);

    ret = CX2684x_initialize(&cx26848_dev[id].cx_intf, video_std);
    if(ret != TRUE)
        goto exit;

    ret = CX2684x_setupByteInterleaveOutput(&cx26848_dev[id].cx_intf,
                                            num_channel,
                                            size,
                                            output_rate,
                                            CX26848_DEV_VPORT_A,
                                            src_sel,
                                            0,
                                            (unsigned long *)video_std,
                                            (unsigned long *)video_res);
    if(ret != TRUE)
        goto exit;

    ret = CX2684x_setupByteInterleaveOutput(&cx26848_dev[id].cx_intf,
                                            num_channel,
                                            size,
                                            output_rate,
                                            CX26848_DEV_VPORT_B,
                                            src_sel,
                                            0,
                                            (unsigned long *)video_std,
                                            (unsigned long *)video_res);
    if(ret != TRUE)
        goto exit;

    ret = CX2684x_setupByteInterleaveOutput(&cx26848_dev[id].cx_intf,
                                            num_channel,
                                            size,
                                            output_rate,
                                            CX26848_DEV_VPORT_C,
                                            src_sel,
                                            0,
                                            (unsigned long *)video_std,
                                            (unsigned long *)video_res);
    if(ret != TRUE)
        goto exit;

    ret = CX2684x_setupByteInterleaveOutput(&cx26848_dev[id].cx_intf,
                                            num_channel,
                                            size,
                                            output_rate,
                                            CX26848_DEV_VPORT_D,
                                            src_sel,
                                            0,
                                            (unsigned long *)video_std,
                                            (unsigned long *)video_res);
    if(ret != TRUE)
        goto exit;

    if(v_mode != vmode[id])
        vmode[id] = v_mode;

    /* Default Image Color Config */
    for(i=0; i<CX26848_DEV_CH_MAX; i++) {
        cx26848_dev[id].color_ctrl._brightness[i] = ((video_std[i] == NTSC_VIDEO_STD) || (video_std[i] == NTSC_EFFIO_STD)) ? BRIGHTNESS_NTSC_DEFAULT : BRIGHTNESS_DEFAULT;
        cx26848_dev[id].color_ctrl._contrast[i]   = CONTRAST_DEFAULT;
        cx26848_dev[id].color_ctrl._hue[i]        = HUE_DEFAULT;
        cx26848_dev[id].color_ctrl._saturation[i] = SATURATION_DEFAULT;
        cx26848_dev[id].color_ctrl._sharpness[i]  = SHARPNESS_DEFAULT;
    }

    printk("CX26848#%d ==> %s MODE!!\n", id, vmode_str);

exit:
    up(&cx26848_dev[id].lock);

    return ((ret == TRUE) ? 0 : -1);

}
EXPORT_SYMBOL(cx26848_video_set_mode);

void *cx26848_get_cxcomm_handle(u8 id)
{
    if(id >= CX26848_DEV_CH_MAX)
        return NULL;

    return ((void *)&cx26848_dev[id].cx_intf);
}
EXPORT_SYMBOL(cx26848_get_cxcomm_handle);

int cx26848_get_device_num(void)
{
    return dev_num;
}
EXPORT_SYMBOL(cx26848_get_device_num);

int cx26848_i2c_write(u8 id, u16 reg, u32 data, int size)
{
    int i;
    u8  buf[6];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if((id >= dev_num) || (size <= 0) || (size > 4))
        return -1;

    if(!cx26848_i2c_client) {
        printk("CX26848 i2c_client not register!!\n");
        return -1;
    }
    adapter = to_i2c_adapter(cx26848_i2c_client->dev.parent);

    buf[0] = (reg>>8) & 0xff;   ///< Sub-Address is MSB first
    buf[1] = (reg & 0xff);

    for(i=0; i<size; i++)       ///< Data is LSB first
        buf[i+2] = (data>>(8*i))&0xff;

    msgs.addr  = iaddr[id]>>1;
    msgs.flags = 0;
    msgs.len   = size + 2;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("CX26848#%d i2c write failed!!\n", id);
        return -1;
    }
    return 0;
}
EXPORT_SYMBOL(cx26848_i2c_write);

int cx26848_i2c_read(u8 id, u16 reg, void *data, int size)
{
    u8 buf[2];
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if((id >= dev_num) || (!data) || (size <= 0) || (size > 4))
        return -1;

    if(!cx26848_i2c_client) {
        printk("CX26848 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(cx26848_i2c_client->dev.parent);

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
        printk("CX26848#%d i2c read failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(cx26848_i2c_read);

int cx26848_ch_to_vin(int id, int ch_idx)
{
    int i;

    if(id >= CX26848_DEV_MAX || ch_idx >= CX26848_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*CX26848_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(CX26848_VCH_MAP[i]) == id) && ((i%CX26848_DEV_CH_MAX) == ch_idx))
            return CH_IDENTIFY_VIN(CX26848_VCH_MAP[i]);
    }

    return 0;
}
EXPORT_SYMBOL(cx26848_ch_to_vin);

int cx26848_vin_to_ch(int id, int vin_idx)
{
    int i;

    if(id >= CX26848_DEV_MAX || vin_idx >= CX26848_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*CX26848_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(CX26848_VCH_MAP[i]) == id) && (CH_IDENTIFY_VIN(CX26848_VCH_MAP[i]) == vin_idx))
            return (i%CX26848_DEV_CH_MAX);
    }

    return 0;
}
EXPORT_SYMBOL(cx26848_vin_to_ch);

int cx26848_video_mode_support_check(int id, CX26848_VMODE_T mode)
{
    if(id >= CX26848_DEV_MAX)
        return 0;

    switch(mode) {
            case CX26848_VMODE_NTSC_720H_1CH:
            case CX26848_VMODE_NTSC_720H_2CH:
            case CX26848_VMODE_NTSC_720H_4CH:
            case CX26848_VMODE_NTSC_960H_1CH:
            case CX26848_VMODE_NTSC_960H_2CH:
            case CX26848_VMODE_NTSC_960H_4CH:
            case CX26848_VMODE_PAL_720H_1CH:
            case CX26848_VMODE_PAL_720H_2CH:
            case CX26848_VMODE_PAL_720H_4CH:
            case CX26848_VMODE_PAL_960H_1CH:
            case CX26848_VMODE_PAL_960H_2CH:
            case CX26848_VMODE_PAL_960H_4CH:
            return 1;
        default:
            return 0;
    }

    return 0;
}
EXPORT_SYMBOL(cx26848_video_mode_support_check);

int cx26848_video_get_contrast(int id, int ch)
{
    int con;

    if(id >= CX26848_DEV_MAX || ch >= CX26848_DEV_CH_MAX)
        return -1;

    down(&cx26848_dev[id].lock);

    con = cx26848_dev[id].color_ctrl._contrast[ch];

    up(&cx26848_dev[id].lock);

    return con;
}
EXPORT_SYMBOL(cx26848_video_get_contrast);

int cx26848_video_set_contrast(int id, int ch, int value)
{
    int ret;

    if(id >= CX26848_DEV_MAX || ch >= CX26848_DEV_CH_MAX)
        return -1;

    down(&cx26848_dev[id].lock);

    ret = CX2684x_setContrast(&cx26848_dev[id].cx_intf, value, ch);
    if(ret != TRUE)
        goto exit;

    cx26848_dev[id].color_ctrl._contrast[ch] = value;

exit:
    up(&cx26848_dev[id].lock);

    return ((ret == TRUE) ? 0 : -1);
}
EXPORT_SYMBOL(cx26848_video_set_contrast);

int cx26848_video_get_brightness(int id, int ch)
{
    int bri;

    if(id >= CX26848_DEV_MAX || ch >= CX26848_DEV_CH_MAX)
        return -1;

    down(&cx26848_dev[id].lock);

    bri = cx26848_dev[id].color_ctrl._brightness[ch];

    up(&cx26848_dev[id].lock);

    return bri;
}
EXPORT_SYMBOL(cx26848_video_get_brightness);

int cx26848_video_set_brightness(int id, int ch, int value)
{
    int ret;

    if(id >= CX26848_DEV_MAX || ch >= CX26848_DEV_CH_MAX)
        return -1;

    down(&cx26848_dev[id].lock);

    ret = CX2684x_setBrightness(&cx26848_dev[id].cx_intf, value, ch);
    if(ret != TRUE)
        goto exit;

    cx26848_dev[id].color_ctrl._brightness[ch] = value;

exit:
    up(&cx26848_dev[id].lock);

    return ((ret == TRUE) ? 0 : -1);
}
EXPORT_SYMBOL(cx26848_video_set_brightness);

int cx26848_video_get_saturation(int id, int ch)
{
    int sat;

    if(id >= CX26848_DEV_MAX || ch >= CX26848_DEV_CH_MAX)
        return -1;

    down(&cx26848_dev[id].lock);

    sat = cx26848_dev[id].color_ctrl._saturation[ch];

    up(&cx26848_dev[id].lock);

    return sat;
}
EXPORT_SYMBOL(cx26848_video_get_saturation);

int cx26848_video_set_saturation(int id, int ch, int value)
{
    int ret;

    if(id >= CX26848_DEV_MAX || ch >= CX26848_DEV_CH_MAX)
        return -1;

    down(&cx26848_dev[id].lock);

    ret = CX2684x_setSaturation(&cx26848_dev[id].cx_intf, value, ch);
    if(ret != TRUE)
        goto exit;

    cx26848_dev[id].color_ctrl._saturation[ch] = value;

exit:
    up(&cx26848_dev[id].lock);

    return ((ret == TRUE) ? 0 : -1);
}
EXPORT_SYMBOL(cx26848_video_set_saturation);

int cx26848_video_get_hue(int id, int ch)
{
    int hue;

    if(id >= CX26848_DEV_MAX || ch >= CX26848_DEV_CH_MAX)
        return -1;

    down(&cx26848_dev[id].lock);

    hue = cx26848_dev[id].color_ctrl._hue[ch];

    up(&cx26848_dev[id].lock);

    return hue;
}
EXPORT_SYMBOL(cx26848_video_get_hue);

int cx26848_video_set_hue(int id, int ch, int value)
{
    int ret;

    if(id >= CX26848_DEV_MAX || ch >= CX26848_DEV_CH_MAX)
        return -1;

    down(&cx26848_dev[id].lock);

    ret = CX2684x_setHue(&cx26848_dev[id].cx_intf, value, ch);
    if(ret != TRUE)
        goto exit;

    cx26848_dev[id].color_ctrl._hue[ch] = value;

exit:
    up(&cx26848_dev[id].lock);

    return ((ret == TRUE) ? 0 : -1);
}
EXPORT_SYMBOL(cx26848_video_set_hue);

static unsigned int cx26848_cxcomm_i2c_write(unsigned id, unsigned reg, int value, int size)
{
    int ret;

    if((size != 1) && (size != 2) && (size != 4))
        return FALSE;

    ret = cx26848_i2c_write((u8)id, (u16)reg, value, size);

    return ((ret < 0) ? FALSE : TRUE);
}

static unsigned int cx26848_cxcomm_i2c_read(unsigned char id, unsigned short reg, void *value, int size)
{
    int ret;

    if((size != 1) && (size != 2) && (size != 4))
        return FALSE;

    ret = cx26848_i2c_read((u8)id, (u16)reg, value, size);

    return ((ret < 0) ? FALSE : TRUE);
}

static void cx26848_cxcomm_sleep(unsigned long usec)
{
    udelay(usec);
}

static int cx26848_cxcomm_register(int id)
{
    if(id >= CX26848_DEV_MAX)
        return -1;

    cx26848_dev[id].cx_intf.i2c_addr = (u8)id;
    cx26848_dev[id].cx_intf.p_handle = &cx26848_dev[id].cx_intf;
    cx26848_dev[id].cx_intf.write    = cx26848_cxcomm_i2c_write;
    cx26848_dev[id].cx_intf.read     = cx26848_cxcomm_i2c_read;
    cx26848_dev[id].cx_intf.sleep    = cx26848_cxcomm_sleep;

    return 0;
}

static int cx26848_miscdev_open(struct inode *inode, struct file *file)
{
    int i, ret = 0;
    struct cx26848_dev_t *pdev = NULL;

    /* lookup device */
    for(i=0; i<dev_num; i++) {
        if(cx26848_dev[i].miscdev.minor == iminor(inode)) {
            pdev = &cx26848_dev[i];
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

static int cx26848_miscdev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long cx26848_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int tmp, ret = 0;
    struct cx26848_ioc_data ioc_data;
    struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)file->private_data;

    down(&pdev->lock);

    if(_IOC_TYPE(cmd) != CX26848_IOC_MAGIC) {
        ret = -ENOIOCTLCMD;
        goto exit;
    }

    switch(cmd) {
        case CX26848_GET_NOVID:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                    ret = -EFAULT;
                    goto exit;
            }

            if(ioc_data.ch >= CX26848_DEV_CH_MAX) {
                ret = -EFAULT;
                goto exit;
            }

            ret = CX2684x_getVideoLostStatus(&pdev->cx_intf, ioc_data.ch, &ioc_data.data);
            if(ret != TRUE) {
                ret = -EFAULT;
                goto exit;
            }
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case CX26848_GET_MODE:
            tmp = cx26848_video_get_mode(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }
            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case CX26848_GET_CONTRAST:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                    ret = -EFAULT;
                    goto exit;
            }

            if(ioc_data.ch >= CX26848_DEV_CH_MAX) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = pdev->color_ctrl._contrast[ioc_data.ch];
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case CX26848_SET_CONTRAST:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            if(ioc_data.ch >= CX26848_DEV_CH_MAX) {
                ret = -EFAULT;
                goto exit;
            }

            ret = CX2684x_setContrast(&pdev->cx_intf, ioc_data.data, ioc_data.ch);
            if(ret != TRUE) {
                ret = -EFAULT;
                goto exit;
            }
            pdev->color_ctrl._contrast[ioc_data.ch] = ioc_data.data;
            break;

        case CX26848_GET_BRIGHTNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            if(ioc_data.ch >= CX26848_DEV_CH_MAX) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = pdev->color_ctrl._brightness[ioc_data.ch];
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case CX26848_SET_BRIGHTNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            if(ioc_data.ch >= CX26848_DEV_CH_MAX) {
                ret = -EFAULT;
                goto exit;
            }

            ret = CX2684x_setBrightness(&pdev->cx_intf, ioc_data.data, ioc_data.ch);
            if(ret !=  TRUE) {
                ret = -EFAULT;
                goto exit;
            }
            pdev->color_ctrl._brightness[ioc_data.ch] = ioc_data.data;
            break;

        case CX26848_GET_SATURATION:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            if(ioc_data.ch >= CX26848_DEV_CH_MAX) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = pdev->color_ctrl._saturation[ioc_data.ch];
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case CX26848_SET_SATURATION:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            if(ioc_data.ch >= CX26848_DEV_CH_MAX) {
                ret = -EFAULT;
                goto exit;
            }

            ret = CX2684x_setSaturation(&pdev->cx_intf, ioc_data.data, ioc_data.ch);
            if(ret !=  TRUE) {
                ret = -EFAULT;
                goto exit;
            }
            pdev->color_ctrl._saturation[ioc_data.ch] = ioc_data.data;
            break;

        case CX26848_GET_HUE:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            if(ioc_data.ch >= CX26848_DEV_CH_MAX) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = pdev->color_ctrl._hue[ioc_data.ch];
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case CX26848_SET_HUE:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            if(ioc_data.ch >= CX26848_DEV_CH_MAX) {
                ret = -EFAULT;
                goto exit;
            }

            ret = CX2684x_setHue(&pdev->cx_intf, ioc_data.data, ioc_data.ch);
            if(ret != TRUE) {
                ret = -EFAULT;
                goto exit;
            }
            pdev->color_ctrl._hue[ioc_data.ch] = ioc_data.data;
            break;

        case CX26848_GET_VOL:
            tmp = CX2684x_getDACVolume(&cx26848_dev[pdev->index].cx_intf);
            if(tmp < 0) {
                    ret = -EFAULT;
                    goto exit;
            }

            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case CX26848_SET_VOL:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                    ret = -EFAULT;
                    goto exit;
            }

            ret = CX2684x_setDACVolume(&pdev->cx_intf, tmp);
            if(ret != TRUE) {
                ret = -EFAULT;
                goto exit;
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

static int cx26848_proc_vmode_show(struct seq_file *sfile, void *v)
{
    int i, vmode;
    struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)sfile->private;
    char *vmode_str[] = {"NTSC_720H_1CH", "NTSC_720H_2CH", "NTSC_720H_4CH",
                         "NTSC_960H_1CH", "NTSC_960H_2CH", "NTSC_960H_4CH",
                         "PAL_720H_1CH" , "PAL_720H_2CH" , "PAL_720H_4CH" ,
                         "PAL_960H_1CH" , "PAL_960H_2CH" , "PAL_960H_4CH" };

    down(&pdev->lock);

    seq_printf(sfile, "\n[CX26848#%d]\n", pdev->index);
    for(i=0; i<CX26848_VMODE_MAX; i++) {
        if(cx26848_video_mode_support_check(pdev->index, i))
            seq_printf(sfile, "%02d: %s\n", i, vmode_str[i]);
    }
    seq_printf(sfile, "----------------------------------\n");

    vmode = cx26848_video_get_mode(pdev->index);
    seq_printf(sfile, "Current==> %s\n\n",(vmode >= 0 && vmode < CX26848_VMODE_MAX) ? vmode_str[vmode] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static int cx26848_proc_status_show(struct seq_file *sfile, void *v)
{
    int i, j, video_los= 0;
    struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[CX26848#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    NOVID     \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<CX26848_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*CX26848_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(CX26848_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(CX26848_VCH_MAP[j]) == i)) {
                CX2684x_getVideoLostStatus(&cx26848_dev[pdev->index].cx_intf, i, &video_los);
                seq_printf(sfile, "%-7d %-7d %s\n", i, j, (video_los == 1) ? "Video_Loss" : "Video_On");
            }
        }
    }
    seq_printf(sfile, "\n");

    up(&pdev->lock);

    return 0;
}

static int cx26848_proc_bri_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[CX26848#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    BRIGHTNESS\n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<CX26848_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*CX26848_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(CX26848_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(CX26848_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d %d\n", i, j, pdev->color_ctrl._brightness[i]);
            }
        }
    }
    seq_printf(sfile, "\nBrightness[0 ~ 10000] \n\n");

    up(&pdev->lock);

    return 0;
}

static int cx26848_proc_con_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[CX26848#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    CONTRAST  \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<CX26848_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*CX26848_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(CX26848_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(CX26848_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d %d\n", i, j, pdev->color_ctrl._contrast[i]);
            }
        }
    }
    seq_printf(sfile, "\nContrast[0 ~ 10000] \n\n");

    up(&pdev->lock);

    return 0;
}

static int cx26848_proc_sat_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[CX26848#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    SATURATION\n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<CX26848_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*CX26848_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(CX26848_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(CX26848_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d %d\n", i, j, pdev->color_ctrl._saturation[i]);
            }
        }
    }
    seq_printf(sfile, "\nSaturation[0 ~ 10000] \n\n");

    up(&pdev->lock);

    return 0;
}

static int cx26848_proc_hue_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[CX26848#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    HUE       \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<CX26848_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*CX26848_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(CX26848_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(CX26848_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d %d\n", i, j, pdev->color_ctrl._hue[i]);
            }
        }
    }
    seq_printf(sfile, "\nHue[0 ~ 10000] \n\n");

    up(&pdev->lock);

    return 0;
}

static int cx26848_proc_volume_show(struct seq_file *sfile, void *v)
{
    int volume;
    struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[CX26848#%d]\n", pdev->index);
    volume = (int)CX2684x_getDACVolume(&cx26848_dev[pdev->index].cx_intf);
    seq_printf(sfile, "Volume[0 ~ 63] = %d\n", volume);

    up(&pdev->lock);

    return 0;
}

static int cx26848_proc_output_ch_show(struct seq_file *sfile, void *v)
{
    struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[CX26848#%d]\n", pdev->index);

    up(&pdev->lock);

    return 0;
}

static ssize_t cx26848_proc_bri_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, ret;
    int bri, vin;
    char value_str[32] = {'\0'};
    struct seq_file *sfile     = (struct seq_file *)file->private_data;
    struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &vin, &bri);

    down(&pdev->lock);

    for(i=0; i<CX26848_DEV_CH_MAX; i++) {
        if(i == vin || vin >= CX26848_DEV_CH_MAX) {
            if(bri != cx26848_dev[pdev->index].color_ctrl._brightness[i]) {
                ret = CX2684x_setBrightness(&cx26848_dev[pdev->index].cx_intf, bri, i);
                if(ret == TRUE)
                    cx26848_dev[pdev->index].color_ctrl._brightness[i] = bri;
            }
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t cx26848_proc_con_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, ret;
    int con, vin;
    char value_str[32] = {'\0'};
    struct seq_file *sfile     = (struct seq_file *)file->private_data;
    struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &vin, &con);

    down(&pdev->lock);

    for(i=0; i<CX26848_DEV_CH_MAX; i++) {
        if(i == vin || vin >= CX26848_DEV_CH_MAX) {
            if(con != cx26848_dev[pdev->index].color_ctrl._contrast[i]) {
                ret = CX2684x_setContrast(&cx26848_dev[pdev->index].cx_intf, con, i);
                if(ret == TRUE)
                    cx26848_dev[pdev->index].color_ctrl._contrast[i] = con;
            }
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t cx26848_proc_sat_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, ret;
    int sat, vin;
    char value_str[32] = {'\0'};
    struct seq_file *sfile     = (struct seq_file *)file->private_data;
    struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &vin, &sat);

    down(&pdev->lock);

    for(i=0; i<CX26848_DEV_CH_MAX; i++) {
        if(i == vin || vin >= CX26848_DEV_CH_MAX) {
            if(sat != cx26848_dev[pdev->index].color_ctrl._saturation[i]) {
                ret = CX2684x_setSaturation(&cx26848_dev[pdev->index].cx_intf, sat, i);
                if(ret == TRUE)
                    cx26848_dev[pdev->index].color_ctrl._saturation[i] = sat;
            }
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t cx26848_proc_hue_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, ret;
    int hue, vin;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &vin, &hue);

    down(&pdev->lock);

    for(i=0; i<CX26848_DEV_CH_MAX; i++) {
        if(i == vin || vin >= CX26848_DEV_CH_MAX) {
            if(hue != cx26848_dev[pdev->index].color_ctrl._hue[i]) {
                ret = CX2684x_setHue(&cx26848_dev[pdev->index].cx_intf, hue, i);
                if(ret == TRUE)
                    cx26848_dev[pdev->index].color_ctrl._hue[i] = hue;
            }
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t cx26848_proc_volume_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32  volume;
    char value_str[32] = {'\0'};
    struct seq_file *sfile     = (struct seq_file *)file->private_data;
    struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &volume);

    down(&pdev->lock);

    CX2684x_setDACVolume(&cx26848_dev[pdev->index].cx_intf, volume);

    up(&pdev->lock);

    return count;
}

static ssize_t cx26848_proc_output_ch_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 ch;
    char value_str[32] = {'\0'};
  //struct seq_file *sfile     = (struct seq_file *)file->private_data;
  //struct cx26848_dev_t *pdev = (struct cx26848_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &ch);

    return count;
}

static int cx26848_proc_vmode_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx26848_proc_vmode_show, PDE(inode)->data);
}

static int cx26848_proc_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx26848_proc_status_show, PDE(inode)->data);
}

static int cx26848_proc_bri_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx26848_proc_bri_show, PDE(inode)->data);
}

static int cx26848_proc_con_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx26848_proc_con_show, PDE(inode)->data);
}

static int cx26848_proc_sat_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx26848_proc_sat_show, PDE(inode)->data);
}

static int cx26848_proc_hue_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx26848_proc_hue_show, PDE(inode)->data);
}

static int cx26848_proc_volume_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx26848_proc_volume_show, PDE(inode)->data);
}

static int cx26848_proc_output_ch_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx26848_proc_output_ch_show, PDE(inode)->data);
}

static struct file_operations cx26848_miscdev_fops = {
    .owner          = THIS_MODULE,
    .open           = cx26848_miscdev_open,
    .release        = cx26848_miscdev_release,
    .unlocked_ioctl = cx26848_miscdev_ioctl,
};

static struct file_operations cx26848_proc_vmode_ops = {
    .owner  = THIS_MODULE,
    .open   = cx26848_proc_vmode_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations cx26848_proc_status_ops = {
    .owner  = THIS_MODULE,
    .open   = cx26848_proc_status_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations cx26848_proc_bri_ops = {
    .owner  = THIS_MODULE,
    .open   = cx26848_proc_bri_open,
    .write  = cx26848_proc_bri_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations cx26848_proc_con_ops = {
    .owner  = THIS_MODULE,
    .open   = cx26848_proc_con_open,
    .write  = cx26848_proc_con_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations cx26848_proc_sat_ops = {
    .owner  = THIS_MODULE,
    .open   = cx26848_proc_sat_open,
    .write  = cx26848_proc_sat_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations cx26848_proc_hue_ops = {
    .owner  = THIS_MODULE,
    .open   = cx26848_proc_hue_open,
    .write  = cx26848_proc_hue_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations cx26848_proc_volume_ops = {
    .owner  = THIS_MODULE,
    .open   = cx26848_proc_volume_open,
    .write  = cx26848_proc_volume_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations cx26848_proc_output_ch_ops = {
    .owner  = THIS_MODULE,
    .open   = cx26848_proc_output_ch_open,
    .write  = cx26848_proc_output_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void cx26848_proc_remove(int id)
{
    if(id >= CX26848_DEV_MAX)
        return;

    if(cx26848_proc_root[id]) {
        if(cx26848_proc_vmode[id]) {
            remove_proc_entry(cx26848_proc_vmode[id]->name, cx26848_proc_root[id]);
            cx26848_proc_vmode[id] = NULL;
        }

        if(cx26848_proc_status[id]) {
            remove_proc_entry(cx26848_proc_status[id]->name, cx26848_proc_root[id]);
            cx26848_proc_status[id] = NULL;
        }

        if(cx26848_proc_bri[id]) {
            remove_proc_entry(cx26848_proc_bri[id]->name, cx26848_proc_root[id]);
            cx26848_proc_bri[id] = NULL;
        }

        if(cx26848_proc_con[id]) {
            remove_proc_entry(cx26848_proc_con[id]->name, cx26848_proc_root[id]);
            cx26848_proc_con[id] = NULL;
        }

        if(cx26848_proc_sat[id]) {
            remove_proc_entry(cx26848_proc_sat[id]->name, cx26848_proc_root[id]);
            cx26848_proc_sat[id] = NULL;
        }

        if(cx26848_proc_hue[id]) {
            remove_proc_entry(cx26848_proc_hue[id]->name, cx26848_proc_root[id]);
            cx26848_proc_hue[id] = NULL;
        }

        if(cx26848_proc_volume[id]) {
            remove_proc_entry(cx26848_proc_volume[id]->name, cx26848_proc_root[id]);
            cx26848_proc_volume[id] = NULL;
        }

        if(cx26848_proc_output_ch[id]) {
            remove_proc_entry(cx26848_proc_output_ch[id]->name, cx26848_proc_root[id]);
            cx26848_proc_output_ch[id] = NULL;
        }

        remove_proc_entry(cx26848_proc_root[id]->name, NULL);
        cx26848_proc_root[id] = NULL;
    }
}

static int cx26848_proc_init(int id)
{
    int ret = 0;

    if(id >= CX26848_DEV_MAX)
        return -1;

    /* root */
    cx26848_proc_root[id] = create_proc_entry(cx26848_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!cx26848_proc_root[id]) {
        printk("create proc node '%s' failed!\n", cx26848_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx26848_proc_root[id]->owner = THIS_MODULE;
#endif

    /* vmode */
    cx26848_proc_vmode[id] = create_proc_entry("vmode", S_IRUGO|S_IXUGO, cx26848_proc_root[id]);
    if(!cx26848_proc_vmode[id]) {
        printk("create proc node '%s/vmode' failed!\n", cx26848_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    cx26848_proc_vmode[id]->proc_fops = &cx26848_proc_vmode_ops;
    cx26848_proc_vmode[id]->data      = (void *)&cx26848_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx26848_proc_vmode[id]->owner     = THIS_MODULE;
#endif

    /* status */
    cx26848_proc_status[id] = create_proc_entry("status", S_IRUGO|S_IXUGO, cx26848_proc_root[id]);
    if(!cx26848_proc_status[id]) {
        printk("create proc node '%s/status' failed!\n", cx26848_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    cx26848_proc_status[id]->proc_fops = &cx26848_proc_status_ops;
    cx26848_proc_status[id]->data      = (void *)&cx26848_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx26848_proc_status[id]->owner     = THIS_MODULE;
#endif

    /* brightness */
    cx26848_proc_bri[id] = create_proc_entry("brightness", S_IRUGO|S_IXUGO, cx26848_proc_root[id]);
    if(!cx26848_proc_bri[id]) {
        printk("create proc node '%s/brightness' failed!\n", cx26848_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    cx26848_proc_bri[id]->proc_fops = &cx26848_proc_bri_ops;
    cx26848_proc_bri[id]->data      = (void *)&cx26848_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx26848_proc_bri[id]->owner     = THIS_MODULE;
#endif

    /* contrast */
    cx26848_proc_con[id] = create_proc_entry("contrast", S_IRUGO|S_IXUGO, cx26848_proc_root[id]);
    if(!cx26848_proc_con[id]) {
        printk("create proc node '%s/contrast' failed!\n", cx26848_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    cx26848_proc_con[id]->proc_fops = &cx26848_proc_con_ops;
    cx26848_proc_con[id]->data      = (void *)&cx26848_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx26848_proc_con[id]->owner     = THIS_MODULE;
#endif

    /* saturation */
    cx26848_proc_sat[id] = create_proc_entry("saturation", S_IRUGO|S_IXUGO, cx26848_proc_root[id]);
    if(!cx26848_proc_sat[id]) {
        printk("create proc node '%s/saturation' failed!\n", cx26848_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    cx26848_proc_sat[id]->proc_fops = &cx26848_proc_sat_ops;
    cx26848_proc_sat[id]->data      = (void *)&cx26848_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx26848_proc_sat[id]->owner     = THIS_MODULE;
#endif

    /* hue */
    cx26848_proc_hue[id] = create_proc_entry("hue", S_IRUGO|S_IXUGO, cx26848_proc_root[id]);
    if(!cx26848_proc_hue[id]) {
        printk("create proc node '%s/hue' failed!\n", cx26848_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    cx26848_proc_hue[id]->proc_fops = &cx26848_proc_hue_ops;
    cx26848_proc_hue[id]->data      = (void *)&cx26848_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx26848_proc_hue[id]->owner     = THIS_MODULE;
#endif

    /* volume */
    cx26848_proc_volume[id] = create_proc_entry("volume", S_IRUGO|S_IXUGO, cx26848_proc_root[id]);
    if(!cx26848_proc_volume[id]) {
        printk("create proc node '%s/volume' failed!\n", cx26848_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    cx26848_proc_volume[id]->proc_fops = &cx26848_proc_volume_ops;
    cx26848_proc_volume[id]->data      = (void *)&cx26848_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx26848_proc_volume[id]->owner     = THIS_MODULE;
#endif

    /* output channel */
    cx26848_proc_output_ch[id] = create_proc_entry("output_ch", S_IRUGO|S_IXUGO, cx26848_proc_root[id]);
    if(!cx26848_proc_output_ch[id]) {
        printk("create proc node '%s/output_ch' failed!\n", cx26848_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    cx26848_proc_output_ch[id]->proc_fops = &cx26848_proc_output_ch_ops;
    cx26848_proc_output_ch[id]->data      = (void *)&cx26848_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx26848_proc_output_ch[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    cx26848_proc_remove(id);
    return ret;
}

static int cx26848_miscdev_init(int id)
{
    int ret;

    if(id >= CX26848_DEV_MAX)
        return -1;

    /* clear */
    memset(&cx26848_dev[id].miscdev, 0, sizeof(struct miscdevice));

    /* create device node */
    cx26848_dev[id].miscdev.name  = cx26848_dev[id].name;
    cx26848_dev[id].miscdev.minor = MISC_DYNAMIC_MINOR;
    cx26848_dev[id].miscdev.fops  = (struct file_operations *)&cx26848_miscdev_fops;
    ret = misc_register(&cx26848_dev[id].miscdev);
    if(ret) {
        printk("create %s misc device node failed!\n", cx26848_dev[id].name);
        cx26848_dev[id].miscdev.name = 0;
        goto exit;
    }

exit:
    return ret;
}

static void cx26848_hw_reset(void)
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

static int cx26848_device_init(int id)
{
    int ret = 0;

    if(!init)
        return 0;

    if(id >= CX26848_DEV_MAX)
        return -1;

    /*====================== video init ========================= */
    ret = cx26848_video_set_mode(id, vmode[id]);
    if(ret < 0)
        goto exit;

    /*====================== audio init ========================= */

exit:
    return ret;
}

static int cx26848_watchdog_thread(void *data)
{
    int i, vin, video_los;
    struct cx26848_dev_t *pdev;

    do {
            /* check cx26848 video status */
            for(i=0; i<dev_num; i++) {
                pdev = &cx26848_dev[i];

                down(&pdev->lock);

                if(pdev->vlos_notify) {
                    for(vin=0; vin<CX26848_DEV_CH_MAX; vin++) {
                        CX2684x_getVideoLostStatus(&pdev->cx_intf, vin, &video_los);
                        if(video_los)
                            pdev->vlos_notify(i, vin, 1);
                        else
                            pdev->vlos_notify(i, vin, 0);
                    }
                }

                up(&pdev->lock);
            }

            /* sleep 1 second */
            schedule_timeout_interruptible(msecs_to_jiffies(1000));

    } while (!kthread_should_stop());

    return 0;
}

static int __init cx26848_init(void)
{
    int i, ret;

    /* platfrom check */
    plat_identifier_check();

    /* check device number */
    if(dev_num > CX26848_DEV_MAX) {
        printk("CX26848 dev_num=%d invalid!!(Max=%d)\n", dev_num, CX26848_DEV_MAX);
        return -1;
    }

    /* register i2c client for contol cx26848 */
    ret = cx26848_i2c_register();
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

    /* create channel mapping table for specific board */
    ret = cx26848_create_ch_map(ch_map);
    if(ret < 0)
        goto err;

    /* hardware reset for all device */
    cx26848_hw_reset();

    /* CX26848 init */
    for(i=0; i<dev_num; i++) {
        cx26848_dev[i].index = i;

        sprintf(cx26848_dev[i].name, "cx26848.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&cx26848_dev[i].lock, 1);
#else
        init_MUTEX(&cx26848_dev[i].lock);
#endif
        ret = cx26848_cxcomm_register(i);
        if(ret < 0)
            goto err;

        ret = cx26848_miscdev_init(i);
        if(ret < 0)
            goto err;

        ret = cx26848_proc_init(i);
        if(ret < 0)
            goto err;

        ret = cx26848_device_init(i);
        if(ret < 0)
            goto err;
    }

    if(init && notify) {
        /* init cx26848 watchdog thread for monitor video status */
        cx26848_wd = kthread_create(cx26848_watchdog_thread, NULL, "cx26848_wd");
        if(!IS_ERR(cx26848_wd))
            wake_up_process(cx26848_wd);
        else {
            printk("create cx26848 watchdog thread failed!!\n");
            cx26848_wd = 0;
            ret = -EFAULT;
            goto err;
        }
    }

    return 0;

err:
    if(cx26848_wd)
        kthread_stop(cx26848_wd);

    cx26848_i2c_unregister();

    for(i=0; i<CX26848_DEV_MAX; i++) {
        if(cx26848_dev[i].miscdev.name)
            misc_deregister(&cx26848_dev[i].miscdev);
        cx26848_proc_remove(i);
    }

    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    return ret;
}

static void __exit cx26848_exit(void)
{
    int i;

    /* stop cx26848 watchdog thread */
    if(cx26848_wd)
        kthread_stop(cx26848_wd);

    cx26848_i2c_unregister();

    for(i=0; i<CX26848_DEV_MAX; i++) {
        /* Remove misc node */
        if(cx26848_dev[i].miscdev.name)
            misc_deregister(&cx26848_dev[i].miscdev);

        /* remove proc node */
        cx26848_proc_remove(i);
    }

    /* release gpio pin */
    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);
}

module_init(cx26848_init);
module_exit(cx26848_exit);

MODULE_DESCRIPTION("Grain Media CX26848 Video Decoders and Audio Codecs Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
