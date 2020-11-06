/**
 * @file nvp1118_drv.c
 * NextChip NVP1118 8-CH 720H Video Decoders and Audio Codecs Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.9 $
 * $Date: 2014/07/29 05:36:11 $
 *
 * ChangeLog:
 *  $Log: nvp1118_drv.c,v $
 *  Revision 1.9  2014/07/29 05:36:11  shawn_hu
 *  Add the module parameter of "ibus" to specify which I2C bus has the decoder attached.
 *
 *  Revision 1.8  2014/06/17 07:58:37  jerson_l
 *  1. add ioctl and proc node for sharpness and video mode switch
 *
 *  Revision 1.7  2014/02/24 13:02:03  jerson_l
 *  1. enable notify option on default
 *
 *  Revision 1.6  2014/02/13 05:37:31  ccsu
 *  [audio] add audio_chnum module parameter
 *
 *  Revision 1.5  2013/11/27 05:33:00  jerson_l
 *  1. support notify procedure
 *
 *  Revision 1.4  2013/10/11 10:27:13  jerson_l
 *  1. update platform gpio pin register api
 *
 *  Revision 1.3  2013/10/01 06:12:02  jerson_l
 *  1. switch to new channel mapping mechanism
 *
 *  Revision 1.2  2013/08/08 05:18:39  jerson_l
 *  1. add system board(1v1) vin and channel mapping comment
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
#include <mach/ftpmu010.h>

#include "platform.h"
#include "nextchip/nvp1118.h"        ///< from module/include/front_end/decoder

#define NVP1118_CLKIN               27000000
#define CH_IDENTIFY(id, vin, vp)    (((id)&0xf)|(((vin)&0xf)<<4)|(((vp)&0xf)<<8))
#define CH_IDENTIFY_ID(x)           ((x)&0xf)
#define CH_IDENTIFY_VIN(x)          (((x)>>4)&0xf)
#define CH_IDENTIFY_VPORT(x)        (((x)>>8)&0xf)

/* device number */
static int dev_num = NVP1118_DEV_MAX;
module_param(dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dev_num, "Device Number: 1~4");

/* device i2c bus */
static int ibus = 0;
module_param(ibus, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ibus, "Device I2C Bus: 0~4");

/* device i2c address */
static ushort iaddr[NVP1118_DEV_MAX] = {0x66, 0x64, 0x62, 0x60};
module_param_array(iaddr, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iaddr, "Device I2C Address");

/* device video mode */
static int vmode[NVP1118_DEV_MAX] = {[0 ... (NVP1118_DEV_MAX - 1)] = NVP1118_VMODE_NTSC_720H_4CH};
module_param_array(vmode, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vmode, "Video Mode");

/* clock port used */
static int clk_used = 0x3;
module_param(clk_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_used, "External Clock Port: BIT0:EXT_CLK0, BIT1:EXT_CLK1");

/* clock source */
static int clk_src = PLAT_EXTCLK_SRC_PLL3;
module_param(clk_src, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_src, "External Clock Source: 0:PLL1OUT1 1:PLL1OUT1/2 2:PLL4OUT2 3:PLL4OUT1/2 4:PLL3");

/* clock frequency */
static int clk_freq = NVP1118_CLKIN;   ///< 27MHz
module_param(clk_freq, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_freq, "External Clock Frequency(Hz)");

/* device use gpio as rstb pin */
static int rstb_used = PLAT_GPIO_ID_X_CAP_RST;
module_param(rstb_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rstb_used, "Use GPIO as RSTB Pin: 0:None, 1:X_CAP_RST");

/* audio sample rate */
static int sample_rate = AUDIO_SAMPLERATE_8K;
module_param(sample_rate, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sample_rate, "audio sample rate: 0:8k  1:16k");

/* audio sample size */
static int sample_size = AUDIO_BITS_16B;
module_param(sample_size, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sample_size, "audio sample size: 0:16bit  1:8bit");

/* init */
static int init = 1;
module_param(init, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init, "device init: 0:no-init  1:init");

/* notify */
static int notify = 1;
module_param(notify, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(notify, "Device Notify => 0:Disable 1:Enable");

/* device channel mapping */
static int ch_map = 0;
module_param(ch_map, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_map, "channel mapping");

/* audio channel number */
int audio_chnum = 8;
module_param(audio_chnum, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(audio_chnum, "audio channel number");

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

struct nvp1118_dev_t {
    int                 index;      ///< device index
    char                name[16];   ///< device name
    struct semaphore    lock;       ///< device locker
    struct miscdevice   miscdev;    ///< device node for ioctl access

    int                 (*vlos_notify)(int id, int vin, int vlos);
    int                 (*vfmt_notify)(int id, int vmode);
};

static struct nvp1118_dev_t   nvp1118_dev[NVP1118_DEV_MAX];
static struct proc_dir_entry *nvp1118_proc_root[NVP1118_DEV_MAX]     = {[0 ... (NVP1118_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *nvp1118_proc_vmode[NVP1118_DEV_MAX]    = {[0 ... (NVP1118_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *nvp1118_proc_status[NVP1118_DEV_MAX]   = {[0 ... (NVP1118_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *nvp1118_proc_bri[NVP1118_DEV_MAX]      = {[0 ... (NVP1118_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *nvp1118_proc_con[NVP1118_DEV_MAX]      = {[0 ... (NVP1118_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *nvp1118_proc_sat[NVP1118_DEV_MAX]      = {[0 ... (NVP1118_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *nvp1118_proc_hue[NVP1118_DEV_MAX]      = {[0 ... (NVP1118_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *nvp1118_proc_sharp[NVP1118_DEV_MAX]    = {[0 ... (NVP1118_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *nvp1118_proc_volume[NVP1118_DEV_MAX]   = {[0 ... (NVP1118_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *nvp1118_proc_output_ch[NVP1118_DEV_MAX]= {[0 ... (NVP1118_DEV_MAX - 1)] = NULL};

static audio_funs_t nvp1118_audio_funs;
static struct i2c_client  *nvp1118_i2c_client = NULL;
static struct task_struct *nvp1118_wd         = NULL;
static u32 NVP1118_VCH_MAP[NVP1118_DEV_MAX*NVP1118_DEV_CH_MAX];

static int nvp1118_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    nvp1118_i2c_client = client;
    return 0;
}

static int nvp1118_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id nvp1118_i2c_id[] = {
    { "nvp1118_i2c", 0 },
    { }
};

static struct i2c_driver nvp1118_i2c_driver = {
    .driver = {
        .name  = "NVP1118_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = nvp1118_i2c_probe,
    .remove   = nvp1118_i2c_remove,
    .id_table = nvp1118_i2c_id
};

static int nvp1118_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&nvp1118_i2c_driver);
    if(ret < 0) {
        printk("NVP1118 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = iaddr[0]>>1;
    strlcpy(info.type, "nvp1118_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("NVP1118 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("NVP1118 can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&nvp1118_i2c_driver);
   return -ENODEV;
}

static void nvp1118_i2c_unregister(void)
{
    i2c_unregister_device(nvp1118_i2c_client);
    i2c_del_driver(&nvp1118_i2c_driver);
    nvp1118_i2c_client = NULL;
}

static int nvp1118_create_ch_map(int map_id)
{
    int i;

    switch(map_id) {
        case 0: ///< for GM8210/GM8287 Socket Board
            for(i=0; i<dev_num; i++) {
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+0] = CH_IDENTIFY(dev_num-i-1, 0, NVP1118_DEV_VPORT_A);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+1] = CH_IDENTIFY(dev_num-i-1, 1, NVP1118_DEV_VPORT_A);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+2] = CH_IDENTIFY(dev_num-i-1, 2, NVP1118_DEV_VPORT_A);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+3] = CH_IDENTIFY(dev_num-i-1, 3, NVP1118_DEV_VPORT_A);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+4] = CH_IDENTIFY(dev_num-i-1, 4, NVP1118_DEV_VPORT_B);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+5] = CH_IDENTIFY(dev_num-i-1, 5, NVP1118_DEV_VPORT_B);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+6] = CH_IDENTIFY(dev_num-i-1, 6, NVP1118_DEV_VPORT_B);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+7] = CH_IDENTIFY(dev_num-i-1, 7, NVP1118_DEV_VPORT_B);
            }
            break;

        case 1: ///< for GM8210 System Board
            for(i=0; i<dev_num; i++) {
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+7] = CH_IDENTIFY(i, 0, NVP1118_DEV_VPORT_A);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+6] = CH_IDENTIFY(i, 1, NVP1118_DEV_VPORT_A);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+5] = CH_IDENTIFY(i, 2, NVP1118_DEV_VPORT_A);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+4] = CH_IDENTIFY(i, 3, NVP1118_DEV_VPORT_A);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+3] = CH_IDENTIFY(i, 4, NVP1118_DEV_VPORT_B);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+2] = CH_IDENTIFY(i, 5, NVP1118_DEV_VPORT_B);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+1] = CH_IDENTIFY(i, 6, NVP1118_DEV_VPORT_B);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+0] = CH_IDENTIFY(i, 7, NVP1118_DEV_VPORT_B);
            }
            break;

        case 2: ///< for GM8287 System Board, GM8283 Socket Board
        default:
            for(i=0; i<dev_num; i++) {
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+0] = CH_IDENTIFY(i, 0, NVP1118_DEV_VPORT_A);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+1] = CH_IDENTIFY(i, 1, NVP1118_DEV_VPORT_A);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+2] = CH_IDENTIFY(i, 2, NVP1118_DEV_VPORT_A);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+3] = CH_IDENTIFY(i, 3, NVP1118_DEV_VPORT_A);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+4] = CH_IDENTIFY(i, 4, NVP1118_DEV_VPORT_B);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+5] = CH_IDENTIFY(i, 5, NVP1118_DEV_VPORT_B);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+6] = CH_IDENTIFY(i, 6, NVP1118_DEV_VPORT_B);
                NVP1118_VCH_MAP[(i*NVP1118_DEV_CH_MAX)+7] = CH_IDENTIFY(i, 7, NVP1118_DEV_VPORT_B);
            }
            break;
    }

    return 0;
}

int nvp1118_vin_to_ch(int id, int vin_idx)
{
    int i;

    if(id >= NVP1118_DEV_MAX || vin_idx >= NVP1118_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*NVP1118_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(NVP1118_VCH_MAP[i]) == id) && (CH_IDENTIFY_VIN(NVP1118_VCH_MAP[i]) == vin_idx))
            return (i%NVP1118_DEV_CH_MAX);
    }

    return 0;
}
EXPORT_SYMBOL(nvp1118_vin_to_ch);

int nvp1118_ch_to_vin(int id, int ch_idx)
{
    int i;

    if(id >= NVP1118_DEV_MAX || ch_idx >= NVP1118_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*NVP1118_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(NVP1118_VCH_MAP[i]) == id) && ((i%NVP1118_DEV_CH_MAX) == ch_idx))
            return CH_IDENTIFY_VIN(NVP1118_VCH_MAP[i]);
    }

    return 0;
}
EXPORT_SYMBOL(nvp1118_ch_to_vin);

int nvp1118_get_vch_id(int id, NVP1118_DEV_VPORT_T vport, int vport_seq)
{
    int i;

    if(id >= NVP1118_DEV_MAX || vport >= NVP1118_DEV_VPORT_MAX || vport_seq >= 4)
        return 0;

    for(i=0; i<(dev_num*NVP1118_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(NVP1118_VCH_MAP[i]) == id) &&
           (CH_IDENTIFY_VPORT(NVP1118_VCH_MAP[i]) == vport) &&
           ((CH_IDENTIFY_VIN(NVP1118_VCH_MAP[i])%4) == vport_seq)) {
            return i;
        }
    }

    return 0;
}
EXPORT_SYMBOL(nvp1118_get_vch_id);

int nvp1118_vin_to_vch(int id, int vin_idx)
{
    int i;

    if(id >= NVP1118_DEV_MAX || vin_idx >= NVP1118_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*NVP1118_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(NVP1118_VCH_MAP[i]) == id) && (CH_IDENTIFY_VIN(NVP1118_VCH_MAP[i]) == vin_idx))
            return i;
    }

    return 0;
}
EXPORT_SYMBOL(nvp1118_vin_to_vch);

int nvp1118_notify_vlos_register(int id, int (*nt_func)(int, int, int))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&nvp1118_dev[id].lock);

    nvp1118_dev[id].vlos_notify = nt_func;

    up(&nvp1118_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(nvp1118_notify_vlos_register);

void nvp1118_notify_vlos_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&nvp1118_dev[id].lock);

    nvp1118_dev[id].vlos_notify = NULL;

    up(&nvp1118_dev[id].lock);
}
EXPORT_SYMBOL(nvp1118_notify_vlos_deregister);

int nvp1118_notify_vfmt_register(int id, int (*nt_func)(int, int))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&nvp1118_dev[id].lock);

    nvp1118_dev[id].vfmt_notify = nt_func;

    up(&nvp1118_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(nvp1118_notify_vfmt_register);

void nvp1118_notify_vfmt_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&nvp1118_dev[id].lock);

    nvp1118_dev[id].vfmt_notify = NULL;

    up(&nvp1118_dev[id].lock);
}
EXPORT_SYMBOL(nvp1118_notify_vfmt_deregister);

/* return value, 0: success, others: failed */
static int nvp1118_sound_switch(int ssp_idx, int chan_num, bool is_on)
{
    const u32 single_decoder_chan_cnt  = 8;
    const u32 cascade_decoder_chan_cnt = 16;
    const u32 max_audio_chan_cnt   = 32;
    const u32 chip_id = ftpmu010_get_attr(ATTR_TYPE_CHIPVER) >> 16;
    u32 temp, vin_idx, decoder_idx = 0;

    switch (chip_id) {
        case 0x8287:
            decoder_idx = 0;
            break;
        default:
            decoder_idx = 0;
            break;
    }

    if (chan_num >= max_audio_chan_cnt) {
        printk(KERN_WARNING "%s: Not implement yet\n", __func__);
        return -EACCES;
    }

    temp = chan_num;
    if (chan_num >= cascade_decoder_chan_cnt && chan_num < max_audio_chan_cnt) {
        chan_num -= cascade_decoder_chan_cnt;
        decoder_idx = 2;
    }
    if (chan_num >= single_decoder_chan_cnt)
        chan_num -= single_decoder_chan_cnt;
    vin_idx = nvp1118_ch_to_vin(decoder_idx, chan_num);

    if (temp >= single_decoder_chan_cnt)
        vin_idx += single_decoder_chan_cnt;
    is_on ? nvp1118_audio_set_output_ch(decoder_idx, vin_idx) : nvp1118_audio_set_output_ch(decoder_idx, 0x10);

    return 0;
}

int nvp1118_i2c_write(u8 id, u8 reg, u8 data)
{
    u8 buf[2];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return -1;

    if(!nvp1118_i2c_client) {
        printk("NVP1118 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(nvp1118_i2c_client->dev.parent);

    buf[0]     = reg;
    buf[1]     = data;
    msgs.addr  = iaddr[id]>>1;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("NVP1118#%d i2c write failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(nvp1118_i2c_write);

u8 nvp1118_i2c_read(u8 id, u8 reg)
{
    u8 reg_data = 0;
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return 0;

    if(!nvp1118_i2c_client) {
        printk("NVP1118 i2c_client not register!!\n");
        return 0;
    }

    adapter = to_i2c_adapter(nvp1118_i2c_client->dev.parent);

    msgs[0].addr  = iaddr[id]>>1;
    msgs[0].flags = 0; /* write */
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    msgs[1].addr  = (iaddr[id] + 1)>>1;
    msgs[1].flags = 1; /* read */
    msgs[1].len   = 1;
    msgs[1].buf   = &reg_data;

    if(i2c_transfer(adapter, msgs, 2) != 2)
        printk("NVP1118#%d i2c read failed!!\n", id);

    return reg_data;
}
EXPORT_SYMBOL(nvp1118_i2c_read);

int nvp1118_i2c_array_write(u8 id, u8 addr, u8 *parray, u32 array_cnt)
{
#define NVP1118_I2C_ARRAY_WRITE_MAX     256
    int i, num = 0;
    u8  buf[NVP1118_I2C_ARRAY_WRITE_MAX+1];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if(id >= dev_num || !parray || array_cnt > NVP1118_I2C_ARRAY_WRITE_MAX)
        return -1;

    if(!nvp1118_i2c_client) {
        printk("NVP1118 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(nvp1118_i2c_client->dev.parent);

    buf[num++] = addr;
    for(i=0; i<array_cnt; i++) {
        buf[num++] = parray[i];
    }

    msgs.addr  = iaddr[id]>>1;
    msgs.flags = 0;
    msgs.len   = num;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("NVP1118#%d i2c write failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(nvp1118_i2c_array_write);

int nvp1118_get_device_num(void)
{
    return dev_num;
}
EXPORT_SYMBOL(nvp1118_get_device_num);

static int nvp1118_proc_vmode_show(struct seq_file *sfile, void *v)
{
    int i, vmode;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;
    char *vmode_str[] = {"NTSC_720H_1CH", "NTSC_720H_2CH", "NTSC_720H_4CH",
                         "PAL_720H_1CH" , "PAL_720H_2CH" , "PAL_720H_4CH" };

    down(&pdev->lock);

    seq_printf(sfile, "\n[NVP1118#%d]\n", pdev->index);
    for(i=0; i<NVP1118_VMODE_MAX; i++) {
        if(nvp1118_video_mode_support_check(pdev->index, i))
            seq_printf(sfile, "%02d: %s\n", i, vmode_str[i]);
    }
    seq_printf(sfile, "----------------------------------\n");

    vmode = nvp1118_video_get_mode(pdev->index);
    seq_printf(sfile, "Current==> %s\n\n",(vmode >= 0 && vmode < NVP1118_VMODE_MAX) ? vmode_str[vmode] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static int nvp1118_proc_status_show(struct seq_file *sfile, void *v)
{
    int i, j, novid;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    down(&pdev->lock);

    novid = nvp1118_status_get_novid(pdev->index);
    seq_printf(sfile, "\n[NVP1118#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    NOVID     \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<NVP1118_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*NVP1118_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(NVP1118_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(NVP1118_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d %s\n", i, j, (novid & (0x1<<i)) ? "Video_Loss" : "Video_On");
            }
        }
    }
    seq_printf(sfile, "\n");

    up(&pdev->lock);

    return 0;
}

static int nvp1118_proc_bri_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[NVP1118#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    BRIGHTNESS\n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<NVP1118_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*NVP1118_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(NVP1118_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(NVP1118_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, nvp1118_video_get_brightness(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nBrightness[-128 ~ +127] ==> 0x01=+1, 0x7f=+127, 0x80=-128, 0xff=-1\n\n");

    up(&pdev->lock);

    return 0;
}

static int nvp1118_proc_con_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[NVP1118#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    CONTRAST  \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<NVP1118_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*NVP1118_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(NVP1118_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(NVP1118_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, nvp1118_video_get_contrast(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nContrast[0x00 ~ 0xff] ==> 0x00=x0, 0x40=x0.5, 0x80=x1, 0xff=x2\n\n");

    up(&pdev->lock);

    return 0;
}

static int nvp1118_proc_sat_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[NVP1118#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    SATURATION\n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<NVP1118_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*NVP1118_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(NVP1118_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(NVP1118_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, nvp1118_video_get_saturation(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nSaturation[0x00 ~ 0xff] ==> 0x00=x0, 0x80=x1, 0xc0=x1.5, 0xff=x2\n\n");

    up(&pdev->lock);

    return 0;
}

static int nvp1118_proc_hue_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[NVP1118#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    HUE       \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<NVP1118_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*NVP1118_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(NVP1118_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(NVP1118_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, nvp1118_video_get_hue(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nHue[0x00 ~ 0xff] ==> 0x00=0, 0x40=90, 0x80=180, 0xff=360\n\n");

    up(&pdev->lock);

    return 0;
}

static int nvp1118_proc_sharp_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[NVP1118#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    SHARPNESS \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<NVP1118_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*NVP1118_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(NVP1118_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(NVP1118_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, nvp1118_video_get_sharpness(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nH_Sharpness[0x0 ~ 0xf] - Bit[7:4] ==> 0x0:x0, 0x4:x0.5, 0x8:x1, 0xf:x2");
    seq_printf(sfile, "\nV_Sharpness[0x0 ~ 0xf] - Bit[3:0] ==> 0x0:x1, 0x4:x2,   0x8:x3, 0xf:x4\n\n");

    up(&pdev->lock);

    return 0;
}

static int nvp1118_proc_volume_show(struct seq_file *sfile, void *v)
{
    int aogain;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[NVP1118#%d]\n", pdev->index);
    aogain = nvp1118_audio_get_mute(pdev->index);
    seq_printf(sfile, "Volume[0x0~0xf] = %d\n", aogain);

    up(&pdev->lock);

    return 0;
}

static int nvp1118_proc_output_ch_show(struct seq_file *sfile, void *v)
{
    int ch, vin_idx;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;
    static char *output_ch_str[] = {"CH 1", "CH 2", "CH 3", "CH 4", "CH 5", "CH 6",
                                    "CH 7", "CH 8", "CH 9", "CH 10", "CH 11", "CH 12",
                                    "CH 13", "CH 14", "CH 15", "CH 16", "FIRST PLAYBACK AUDIO",
                                    "SECOND PLAYBACK AUDIO", "THIRD PLAYBACK AUDIO",
                                    "FOURTH PLAYBACK AUDIO", "MIC input 1", "MIC input 2",
                                    "MIC input 3", "MIC input 4", "Mixed audio", "no audio output"};

    down(&pdev->lock);

    seq_printf(sfile, "\n[NVP1118#%d]\n", pdev->index);

    vin_idx = nvp1118_audio_get_output_ch(pdev->index);

    if (vin_idx >= 0 && vin_idx <= 7)
        ch = nvp1118_vin_to_ch(pdev->index, vin_idx);
    else if (vin_idx >= 8 && vin_idx <= 15) {
        vin_idx -= 8;
        ch = nvp1118_vin_to_ch(pdev->index, vin_idx);
        ch += 8;
    }
    else
        ch = vin_idx;

    seq_printf(sfile, "Current[0x0~0x18]==> %s\n\n", output_ch_str[ch]);

    up(&pdev->lock);

    return 0;
}

static ssize_t nvp1118_proc_vmode_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int vmode;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &vmode);

    down(&pdev->lock);

    if(vmode != nvp1118_video_get_mode(pdev->index))
        nvp1118_video_set_mode(pdev->index, vmode);

    up(&pdev->lock);

    return count;
}

static ssize_t nvp1118_proc_bri_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, vin;
    u32  bri;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &bri);

    down(&pdev->lock);

    for(i=0; i<NVP1118_DEV_CH_MAX; i++) {
        if(i == vin || vin >= NVP1118_DEV_CH_MAX) {
            nvp1118_video_set_brightness(pdev->index, i, (u8)bri);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t nvp1118_proc_con_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 con;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &con);

    down(&pdev->lock);

    for(i=0; i<NVP1118_DEV_CH_MAX; i++) {
        if(i == vin || vin >= NVP1118_DEV_CH_MAX) {
            nvp1118_video_set_contrast(pdev->index, i, (u8)con);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t nvp1118_proc_sat_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 sat;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &sat);

    down(&pdev->lock);

    for(i=0; i<NVP1118_DEV_CH_MAX; i++) {
        if(i == vin || vin >= NVP1118_DEV_CH_MAX) {
            nvp1118_video_set_saturation(pdev->index, i, (u8)sat);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t nvp1118_proc_hue_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 hue;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &hue);

    down(&pdev->lock);

    for(i=0; i<NVP1118_DEV_CH_MAX; i++) {
        if(i == vin || vin >= NVP1118_DEV_CH_MAX) {
            nvp1118_video_set_hue(pdev->index, i, (u8)hue);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t nvp1118_proc_sharp_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 sharp;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &sharp);

    down(&pdev->lock);

    for(i=0; i<NVP1118_DEV_CH_MAX; i++) {
        if(i == vin || vin >= NVP1118_DEV_CH_MAX) {
            nvp1118_video_set_sharpness(pdev->index, i, (u8)sharp);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t nvp1118_proc_volume_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 volume;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &volume);

    down(&pdev->lock);

    nvp1118_audio_set_volume(pdev->index, volume);

    up(&pdev->lock);

    return count;
}

static ssize_t nvp1118_proc_output_ch_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 ch, vin_idx, temp;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &ch);

    temp = ch;
    if (ch >= 16)
        vin_idx = ch;

    if (ch < 16) {
        if (ch >= 8)
            ch -= 8;
        vin_idx = nvp1118_ch_to_vin(pdev->index, ch);
        if (temp >= 8)
            vin_idx += 8;
    }

    down(&pdev->lock);

    nvp1118_audio_set_output_ch(pdev->index, vin_idx);

    up(&pdev->lock);

    return count;
}

static int nvp1118_proc_vmode_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvp1118_proc_vmode_show, PDE(inode)->data);
}

static int nvp1118_proc_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvp1118_proc_status_show, PDE(inode)->data);
}

static int nvp1118_proc_bri_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvp1118_proc_bri_show, PDE(inode)->data);
}

static int nvp1118_proc_con_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvp1118_proc_con_show, PDE(inode)->data);
}

static int nvp1118_proc_sat_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvp1118_proc_sat_show, PDE(inode)->data);
}

static int nvp1118_proc_hue_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvp1118_proc_hue_show, PDE(inode)->data);
}

static int nvp1118_proc_sharp_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvp1118_proc_sharp_show, PDE(inode)->data);
}

static int nvp1118_proc_volume_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvp1118_proc_volume_show, PDE(inode)->data);
}

static int nvp1118_proc_output_ch_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvp1118_proc_output_ch_show, PDE(inode)->data);
}

static struct file_operations nvp1118_proc_vmode_ops = {
    .owner  = THIS_MODULE,
    .open   = nvp1118_proc_vmode_open,
    .write  = nvp1118_proc_vmode_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations nvp1118_proc_status_ops = {
    .owner  = THIS_MODULE,
    .open   = nvp1118_proc_status_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations nvp1118_proc_bri_ops = {
    .owner  = THIS_MODULE,
    .open   = nvp1118_proc_bri_open,
    .write  = nvp1118_proc_bri_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations nvp1118_proc_con_ops = {
    .owner  = THIS_MODULE,
    .open   = nvp1118_proc_con_open,
    .write  = nvp1118_proc_con_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations nvp1118_proc_sat_ops = {
    .owner  = THIS_MODULE,
    .open   = nvp1118_proc_sat_open,
    .write  = nvp1118_proc_sat_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations nvp1118_proc_hue_ops = {
    .owner  = THIS_MODULE,
    .open   = nvp1118_proc_hue_open,
    .write  = nvp1118_proc_hue_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations nvp1118_proc_sharp_ops = {
    .owner  = THIS_MODULE,
    .open   = nvp1118_proc_sharp_open,
    .write  = nvp1118_proc_sharp_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations nvp1118_proc_volume_ops = {
    .owner  = THIS_MODULE,
    .open   = nvp1118_proc_volume_open,
    .write  = nvp1118_proc_volume_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations nvp1118_proc_output_ch_ops = {
    .owner  = THIS_MODULE,
    .open   = nvp1118_proc_output_ch_open,
    .write  = nvp1118_proc_output_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void nvp1118_proc_remove(int id)
{
    if(id >= NVP1118_DEV_MAX)
        return;

    if(nvp1118_proc_root[id]) {
        if(nvp1118_proc_vmode[id]) {
            remove_proc_entry(nvp1118_proc_vmode[id]->name, nvp1118_proc_root[id]);
            nvp1118_proc_vmode[id] = NULL;
        }

        if(nvp1118_proc_status[id]) {
            remove_proc_entry(nvp1118_proc_status[id]->name, nvp1118_proc_root[id]);
            nvp1118_proc_status[id] = NULL;
        }

        if(nvp1118_proc_bri[id]) {
            remove_proc_entry(nvp1118_proc_bri[id]->name, nvp1118_proc_root[id]);
            nvp1118_proc_bri[id] = NULL;
        }

        if(nvp1118_proc_con[id]) {
            remove_proc_entry(nvp1118_proc_con[id]->name, nvp1118_proc_root[id]);
            nvp1118_proc_con[id] = NULL;
        }

        if(nvp1118_proc_sat[id]) {
            remove_proc_entry(nvp1118_proc_sat[id]->name, nvp1118_proc_root[id]);
            nvp1118_proc_sat[id] = NULL;
        }

        if(nvp1118_proc_hue[id]) {
            remove_proc_entry(nvp1118_proc_hue[id]->name, nvp1118_proc_root[id]);
            nvp1118_proc_hue[id] = NULL;
        }

        if(nvp1118_proc_sharp[id]) {
            remove_proc_entry(nvp1118_proc_sharp[id]->name, nvp1118_proc_root[id]);
            nvp1118_proc_sharp[id] = NULL;
        }

        if(nvp1118_proc_volume[id]) {
            remove_proc_entry(nvp1118_proc_volume[id]->name, nvp1118_proc_root[id]);
            nvp1118_proc_volume[id] = NULL;
        }

        if(nvp1118_proc_output_ch[id]) {
            remove_proc_entry(nvp1118_proc_output_ch[id]->name, nvp1118_proc_root[id]);
            nvp1118_proc_output_ch[id] = NULL;
        }

        remove_proc_entry(nvp1118_proc_root[id]->name, NULL);
        nvp1118_proc_root[id] = NULL;
    }
}

static int nvp1118_proc_init(int id)
{
    int ret = 0;

    if(id >= NVP1118_DEV_MAX)
        return -1;

    /* root */
    nvp1118_proc_root[id] = create_proc_entry(nvp1118_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!nvp1118_proc_root[id]) {
        printk("create proc node '%s' failed!\n", nvp1118_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp1118_proc_root[id]->owner = THIS_MODULE;
#endif

    /* vmode */
    nvp1118_proc_vmode[id] = create_proc_entry("vmode", S_IRUGO|S_IXUGO, nvp1118_proc_root[id]);
    if(!nvp1118_proc_vmode[id]) {
        printk("create proc node '%s/vmode' failed!\n", nvp1118_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    nvp1118_proc_vmode[id]->proc_fops = &nvp1118_proc_vmode_ops;
    nvp1118_proc_vmode[id]->data      = (void *)&nvp1118_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp1118_proc_vmode[id]->owner     = THIS_MODULE;
#endif

    /* status */
    nvp1118_proc_status[id] = create_proc_entry("status", S_IRUGO|S_IXUGO, nvp1118_proc_root[id]);
    if(!nvp1118_proc_status[id]) {
        printk("create proc node '%s/status' failed!\n", nvp1118_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    nvp1118_proc_status[id]->proc_fops = &nvp1118_proc_status_ops;
    nvp1118_proc_status[id]->data      = (void *)&nvp1118_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp1118_proc_status[id]->owner     = THIS_MODULE;
#endif

    /* brightness */
    nvp1118_proc_bri[id] = create_proc_entry("brightness", S_IRUGO|S_IXUGO, nvp1118_proc_root[id]);
    if(!nvp1118_proc_bri[id]) {
        printk("create proc node '%s/brightness' failed!\n", nvp1118_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    nvp1118_proc_bri[id]->proc_fops = &nvp1118_proc_bri_ops;
    nvp1118_proc_bri[id]->data      = (void *)&nvp1118_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp1118_proc_bri[id]->owner     = THIS_MODULE;
#endif

    /* contrast */
    nvp1118_proc_con[id] = create_proc_entry("contrast", S_IRUGO|S_IXUGO, nvp1118_proc_root[id]);
    if(!nvp1118_proc_con[id]) {
        printk("create proc node '%s/contrast' failed!\n", nvp1118_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    nvp1118_proc_con[id]->proc_fops = &nvp1118_proc_con_ops;
    nvp1118_proc_con[id]->data      = (void *)&nvp1118_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp1118_proc_con[id]->owner     = THIS_MODULE;
#endif

    /* saturation */
    nvp1118_proc_sat[id] = create_proc_entry("saturation", S_IRUGO|S_IXUGO, nvp1118_proc_root[id]);
    if(!nvp1118_proc_sat[id]) {
        printk("create proc node '%s/saturation' failed!\n", nvp1118_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    nvp1118_proc_sat[id]->proc_fops = &nvp1118_proc_sat_ops;
    nvp1118_proc_sat[id]->data      = (void *)&nvp1118_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp1118_proc_sat[id]->owner     = THIS_MODULE;
#endif

    /* hue */
    nvp1118_proc_hue[id] = create_proc_entry("hue", S_IRUGO|S_IXUGO, nvp1118_proc_root[id]);
    if(!nvp1118_proc_hue[id]) {
        printk("create proc node '%s/hue' failed!\n", nvp1118_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    nvp1118_proc_hue[id]->proc_fops = &nvp1118_proc_hue_ops;
    nvp1118_proc_hue[id]->data      = (void *)&nvp1118_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp1118_proc_hue[id]->owner     = THIS_MODULE;
#endif

    /* sharpness */
    nvp1118_proc_sharp[id] = create_proc_entry("sharpness", S_IRUGO|S_IXUGO, nvp1118_proc_root[id]);
    if(!nvp1118_proc_sharp[id]) {
        printk("create proc node '%s/sharpness' failed!\n", nvp1118_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    nvp1118_proc_sharp[id]->proc_fops = &nvp1118_proc_sharp_ops;
    nvp1118_proc_sharp[id]->data      = (void *)&nvp1118_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp1118_proc_sharp[id]->owner     = THIS_MODULE;
#endif

    /* volume */
    nvp1118_proc_volume[id] = create_proc_entry("volume", S_IRUGO|S_IXUGO, nvp1118_proc_root[id]);
    if(!nvp1118_proc_volume[id]) {
        printk("create proc node '%s/volume' failed!\n", nvp1118_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    nvp1118_proc_volume[id]->proc_fops = &nvp1118_proc_volume_ops;
    nvp1118_proc_volume[id]->data      = (void *)&nvp1118_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp1118_proc_volume[id]->owner     = THIS_MODULE;
#endif

    /* output channel */
    nvp1118_proc_output_ch[id] = create_proc_entry("output_ch", S_IRUGO|S_IXUGO, nvp1118_proc_root[id]);
    if(!nvp1118_proc_output_ch[id]) {
        printk("create proc node '%s/output_ch' failed!\n", nvp1118_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    nvp1118_proc_output_ch[id]->proc_fops = &nvp1118_proc_output_ch_ops;
    nvp1118_proc_output_ch[id]->data      = (void *)&nvp1118_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp1118_proc_output_ch[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    nvp1118_proc_remove(id);
    return ret;
}

static int nvp1118_miscdev_open(struct inode *inode, struct file *file)
{
    int i, ret = 0;
    struct nvp1118_dev_t *pdev = NULL;

    /* lookup device */
    for(i=0; i<dev_num; i++) {
        if(nvp1118_dev[i].miscdev.minor == iminor(inode)) {
            pdev = &nvp1118_dev[i];
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

static int nvp1118_miscdev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long nvp1118_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int tmp, vin_idx, ch, ret = 0;
    struct nvp1118_ioc_data ioc_data;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)file->private_data;

    down(&pdev->lock);

    if(_IOC_TYPE(cmd) != NVP1118_IOC_MAGIC) {
        ret = -ENOIOCTLCMD;
        goto exit;
    }

    switch(cmd) {
        case NVP1118_GET_NOVID:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            if(ioc_data.ch >= NVP1118_DEV_CH_MAX) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = nvp1118_status_get_novid(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = (tmp>>ioc_data.ch) & 0x1;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case NVP1118_GET_MODE:
            tmp = nvp1118_video_get_mode(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }
            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case NVP1118_SET_MODE:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = nvp1118_video_set_mode(pdev->index, tmp);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case NVP1118_GET_CONTRAST:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = nvp1118_video_get_contrast(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case NVP1118_SET_CONTRAST:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = nvp1118_video_set_contrast(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case NVP1118_GET_BRIGHTNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = nvp1118_video_get_brightness(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case NVP1118_SET_BRIGHTNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = nvp1118_video_set_brightness(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case NVP1118_GET_SATURATION:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = nvp1118_video_get_saturation(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case NVP1118_SET_SATURATION:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = nvp1118_video_set_saturation(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case NVP1118_GET_HUE:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = nvp1118_video_get_hue(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case NVP1118_SET_HUE:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = nvp1118_video_set_hue(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case NVP1118_GET_SHARPNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = nvp1118_video_get_sharpness(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case NVP1118_SET_SHARPNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = nvp1118_video_set_sharpness(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case NVP1118_GET_VOL:
            tmp = nvp1118_audio_get_mute(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case NVP1118_SET_VOL:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = nvp1118_audio_set_volume(pdev->index, tmp);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case NVP1118_GET_OUT_CH:
            tmp = nvp1118_audio_get_output_ch(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            vin_idx = tmp;
            if (vin_idx >= 0 && vin_idx <= 7)
                ch = nvp1118_vin_to_ch(pdev->index, vin_idx);
            else if (vin_idx >= 8 && vin_idx <= 15) {
                vin_idx -= 8;
                ch = nvp1118_vin_to_ch(pdev->index, vin_idx);
                ch += 8;
            }
            else
                ch = vin_idx;

            tmp = ch;

            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case NVP1118_SET_OUT_CH:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }

            ch = tmp;
            if (tmp >= 16)
                vin_idx = tmp;

            if (tmp < 16) {
                if (tmp >= 8)
                    tmp -= 8;
                vin_idx = nvp1118_ch_to_vin(pdev->index, tmp);
                if (ch >= 8)
                    vin_idx += 8;
            }
            tmp = vin_idx;

            ret = nvp1118_audio_set_output_ch(pdev->index, tmp);
            if(tmp < 0) {
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

static struct file_operations nvp1118_miscdev_fops = {
    .owner          = THIS_MODULE,
    .open           = nvp1118_miscdev_open,
    .release        = nvp1118_miscdev_release,
    .unlocked_ioctl = nvp1118_miscdev_ioctl,
};

static int nvp1118_miscdev_init(int id)
{
    int ret;

    if(id >= NVP1118_DEV_MAX)
        return -1;

    /* clear */
    memset(&nvp1118_dev[id].miscdev, 0, sizeof(struct miscdevice));

    /* create device node */
    nvp1118_dev[id].miscdev.name  = nvp1118_dev[id].name;
    nvp1118_dev[id].miscdev.minor = MISC_DYNAMIC_MINOR;
    nvp1118_dev[id].miscdev.fops  = (struct file_operations *)&nvp1118_miscdev_fops;
    ret = misc_register(&nvp1118_dev[id].miscdev);
    if(ret) {
        printk("create %s misc device node failed!\n", nvp1118_dev[id].name);
        nvp1118_dev[id].miscdev.name = 0;
        goto exit;
    }

exit:
    return ret;
}

static void nvp1118_hw_reset(void)
{
    if(!rstb_used || !init)
        return;

    /* Active Low */
    plat_gpio_set_value(rstb_used, 0);

    udelay(10);

    /* Active High and release reset */
    plat_gpio_set_value(rstb_used, 1);
}

static int nvp1118_device_init(int id)
{
    int ret = 0;

    if (!init)
        return 0;

    if(id >= NVP1118_DEV_MAX)
        return -1;

    /*====================== video init ========================= */
    ret = nvp1118_video_set_mode(id, vmode[id]);
    if(ret < 0)
        goto exit;

    /*====================== audio init ========================= */
    ret = nvp1118_audio_set_mode(id, vmode[(id>>1)<<1], sample_rate, sample_size);
    if(ret < 0)
        goto exit;

exit:
    return ret;
}

static int nvp1118_watchdog_thread(void *data)
{
    int i, vin, novid, vmode;
    struct nvp1118_dev_t *pdev;

    do {
            /* check nvp1118 video status */
            for(i=0; i<dev_num; i++) {
                pdev = &nvp1118_dev[i];

                down(&pdev->lock);

                /* video signal check */
                if(pdev->vlos_notify) {
                    novid = nvp1118_status_get_novid(pdev->index);
                    for(vin=0; vin<NVP1118_DEV_CH_MAX; vin++) {
                        if(novid & (0x1<<vin))
                            pdev->vlos_notify(i, vin, 1);
                        else
                            pdev->vlos_notify(i, vin, 0);
                    }
                }

                /* video format check */
                if(pdev->vfmt_notify) {
                    vmode = nvp1118_video_get_mode(pdev->index);
                    if((vmode >= 0) && (vmode < NVP1118_VMODE_MAX))
                        pdev->vfmt_notify(i, vmode);
                }

                up(&pdev->lock);
            }

            /* sleep 1 second */
            schedule_timeout_interruptible(msecs_to_jiffies(1000));

    } while (!kthread_should_stop());

    return 0;
}

static int __init nvp1118_init(void)
{
    int i, ret;

    /* platfrom check */
    plat_identifier_check();

    /* check device number */
    if(dev_num > NVP1118_DEV_MAX) {
        printk("NVP1118 dev_num=%d invalid!!(Max=%d)\n", dev_num, NVP1118_DEV_MAX);
        return -1;
    }

    /* register i2c client for contol nvp1118 */
    ret = nvp1118_i2c_register();
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
    ret = nvp1118_create_ch_map(ch_map);
    if(ret < 0)
        goto err;

    /* hardware reset for all device */
    nvp1118_hw_reset();

    /* NVP1118 init */
    for(i=0; i<dev_num; i++) {
        nvp1118_dev[i].index = i;

        sprintf(nvp1118_dev[i].name, "nvp1118.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&nvp1118_dev[i].lock, 1);
#else
        init_MUTEX(&nvp1118_dev[i].lock);
#endif
        ret = nvp1118_proc_init(i);
        if(ret < 0)
            goto err;

        ret = nvp1118_miscdev_init(i);
        if(ret < 0)
            goto err;

        ret = nvp1118_device_init(i);
        if(ret < 0)
            goto err;
    }

    if(init && notify) {
        /* init nvp1118 watchdog thread for monitor video status */
        nvp1118_wd = kthread_create(nvp1118_watchdog_thread, NULL, "nvp1118_wd");
        if(!IS_ERR(nvp1118_wd))
            wake_up_process(nvp1118_wd);
        else {
            printk("create nvp1118 watchdog thread failed!!\n");
            nvp1118_wd = 0;
            ret = -EFAULT;
            goto err;
        }
    }

    /* register audio function for platform used */
    nvp1118_audio_funs.sound_switch = nvp1118_sound_switch;
    nvp1118_audio_funs.codec_name   = nvp1118_dev[0].name;
    plat_audio_register_function(&nvp1118_audio_funs);

    return 0;

err:
    if(nvp1118_wd)
        kthread_stop(nvp1118_wd);

    nvp1118_i2c_unregister();
    for(i=0; i<NVP1118_DEV_MAX; i++) {
        if(nvp1118_dev[i].miscdev.name)
            misc_deregister(&nvp1118_dev[i].miscdev);

        nvp1118_proc_remove(i);
    }

    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    return ret;
}

static void __exit nvp1118_exit(void)
{
    int i;

    /* stop nvp1118 watchdog thread */
    if(nvp1118_wd)
        kthread_stop(nvp1118_wd);

    nvp1118_i2c_unregister();

    for(i=0; i<NVP1118_DEV_MAX; i++) {
        /* remove device node */
        if(nvp1118_dev[i].miscdev.name)
            misc_deregister(&nvp1118_dev[i].miscdev);

        /* remove proc node */
        nvp1118_proc_remove(i);
    }

    /* release gpio pin */
    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);
}

module_init(nvp1118_init);
module_exit(nvp1118_exit);

MODULE_DESCRIPTION("Grain Media NVP1118 Video Decoders and Audio Codecs Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
