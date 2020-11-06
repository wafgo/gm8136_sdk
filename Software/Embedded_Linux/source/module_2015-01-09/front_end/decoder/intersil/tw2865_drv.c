/**
 * @file tw2865_drv.c
 * Intersil TW2865 4-CH D1 Video Decoders and Audio Codecs Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.6 $
 * $Date: 2014/07/29 05:36:10 $
 *
 * ChangeLog:
 *  $Log: tw2865_drv.c,v $
 *  Revision 1.6  2014/07/29 05:36:10  shawn_hu
 *  Add the module parameter of "ibus" to specify which I2C bus has the decoder attached.
 *
 *  Revision 1.5  2014/07/22 03:09:14  shawn_hu
 *  1. add IOCTL and proc node for set sharpness and video mode
 *
 *  Revision 1.4  2014/02/24 07:05:51  jerson_l
 *  1. correct channel mapping for general 1/2/4CH
 *
 *  Revision 1.3  2013/11/27 05:32:59  jerson_l
 *  1. support notify procedure
 *
 *  Revision 1.2  2013/10/01 06:12:00  jerson_l
 *  1. switch to new channel mapping mechanism
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
#include <asm/uaccess.h>

#include "platform.h"
#include "intersil/tw2865.h"        ///< from module/include/front_end/decoder

#define TW2865_CLKIN                27000000
#define CH_IDENTIFY(id, vin, vp)    (((id)&0xf)|(((vin)&0xf)<<4)|(((vp)&0xf)<<8))
#define CH_IDENTIFY_ID(x)           ((x)&0xf)
#define CH_IDENTIFY_VIN(x)          (((x)>>4)&0xf)
#define CH_IDENTIFY_VPORT(x)        (((x)>>8)&0xf)

/* device number */
static int dev_num = TW2865_DEV_MAX;
module_param(dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dev_num, "Device Number: 1~4");

/* device i2c bus */
static int ibus = 0;
module_param(ibus, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ibus, "Device I2C Bus: 0~4");

/* device i2c address */
static ushort iaddr[TW2865_DEV_MAX] = {0x50, 0x52, 0x54, 0x56};
module_param_array(iaddr, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iaddr, "Device I2C Address");

/* device video mode */
static int vmode[TW2865_DEV_MAX] = {[0 ... (TW2865_DEV_MAX - 1)] = TW2865_VMODE_NTSC_720H_4CH};
module_param_array(vmode, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vmode, "Video Mode");

/* clock port used */
static int clk_used = 0x3;
module_param(clk_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_used, "External Clock Port: BIT0:EXT_CLK0, BIT1:EXT_CLK1");

/* clock source */
static int clk_src = PLAT_EXTCLK_SRC_PLL1OUT1;
module_param(clk_src, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_src, "External Clock Source: 0:PLL1OUT1 1:PLL1OUT1/2 2:PLL4OUT2 3:PLL4OUT1/2 4:PLL3");

/* clock frequency */
static int clk_freq = TW2865_CLKIN;   ///< 27MHz
module_param(clk_freq, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_freq, "External Clock Frequency(Hz)");

/* audio sample rate */
static int sample_rate = TW2865_AUDIO_SAMPLERATE_8K;
module_param(sample_rate, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sample_rate, "audio sample rate: 0:8k  1:16k  2:32k 3:44.1k 4:48k");

/* audio sample size */
static int sample_size = TW2865_AUDIO_BITS_16B;
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

/******************************************************************************
 VCH#0 ------> VIN#0 -----------------> VD1 -------> X_CAP#0
 VCH#1 ------> VIN#1 ---------|
 VCH#2 ------> VIN#2 ---------|
 VCH#3 ------> VIN#3 ---------|

*******************************************************************************/

struct tw2865_dev_t {
    int                 index;      ///< device index
    char                name[16];   ///< device name
    struct semaphore    lock;       ///< device locker
    struct miscdevice   miscdev;    ///< device node for ioctl access

    int                 (*vlos_notify)(int id, int vin, int vlos);
    int                 (*vfmt_notify)(int id, int vmode);
};

static struct tw2865_dev_t   tw2865_dev[TW2865_DEV_MAX];
static struct proc_dir_entry *tw2865_proc_root[TW2865_DEV_MAX]   = {[0 ... (TW2865_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2865_proc_vmode[TW2865_DEV_MAX]  = {[0 ... (TW2865_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2865_proc_status[TW2865_DEV_MAX] = {[0 ... (TW2865_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2865_proc_bri[TW2865_DEV_MAX]    = {[0 ... (TW2865_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2865_proc_con[TW2865_DEV_MAX]    = {[0 ... (TW2865_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2865_proc_sat_u[TW2865_DEV_MAX]  = {[0 ... (TW2865_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2865_proc_sat_v[TW2865_DEV_MAX]  = {[0 ... (TW2865_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2865_proc_hue[TW2865_DEV_MAX]    = {[0 ... (TW2865_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2865_proc_sharp[TW2865_DEV_MAX]  = {[0 ... (TW2865_DEV_MAX - 1)] = NULL};

static struct i2c_client  *tw2865_i2c_client = NULL;
static struct task_struct *tw2865_wd         = NULL;
static u32 TW2865_VCH_MAP[TW2865_DEV_MAX*TW2865_DEV_CH_MAX];

static int tw2865_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    tw2865_i2c_client = client;
    return 0;
}

static int tw2865_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id tw2865_i2c_id[] = {
    { "tw2865_i2c", 0 },
    { }
};

static struct i2c_driver tw2865_i2c_driver = {
    .driver = {
        .name  = "TW2865_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = tw2865_i2c_probe,
    .remove   = tw2865_i2c_remove,
    .id_table = tw2865_i2c_id
};

static int tw2865_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&tw2865_i2c_driver);
    if(ret < 0) {
        printk("TW2865 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = iaddr[0]>>1;
    strlcpy(info.type, "tw2865_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("TW2865 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("TW2865 can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&tw2865_i2c_driver);
   return -ENODEV;
}

static void tw2865_i2c_unregister(void)
{
    i2c_unregister_device(tw2865_i2c_client);
    i2c_del_driver(&tw2865_i2c_driver);
    tw2865_i2c_client = NULL;
}

static int tw2865_create_ch_map(int map_id)
{
    int i;

    switch(map_id) {
        case 0: ///< General, 1CH Mode
            for(i=0; i<dev_num; i++) {
                TW2865_VCH_MAP[(i*TW2865_DEV_VPORT_MAX)+0] = CH_IDENTIFY(i, 0, TW2865_DEV_VPORT_VD1);
                TW2865_VCH_MAP[(i*TW2865_DEV_VPORT_MAX)+1] = CH_IDENTIFY(i, 1, TW2865_DEV_VPORT_VD2);
                TW2865_VCH_MAP[(i*TW2865_DEV_VPORT_MAX)+2] = CH_IDENTIFY(i, 2, TW2865_DEV_VPORT_VD3);
                TW2865_VCH_MAP[(i*TW2865_DEV_VPORT_MAX)+3] = CH_IDENTIFY(i, 3, TW2865_DEV_VPORT_VD4);
            }
            break;
        case 1: ///< General, 2CH Mode
            for(i=0; i<dev_num; i++) {
                TW2865_VCH_MAP[(i*TW2865_DEV_VPORT_MAX)+0] = CH_IDENTIFY(i, 0, TW2865_DEV_VPORT_VD1);
                TW2865_VCH_MAP[(i*TW2865_DEV_VPORT_MAX)+1] = CH_IDENTIFY(i, 1, TW2865_DEV_VPORT_VD1);
                TW2865_VCH_MAP[(i*TW2865_DEV_VPORT_MAX)+2] = CH_IDENTIFY(i, 2, TW2865_DEV_VPORT_VD2);
                TW2865_VCH_MAP[(i*TW2865_DEV_VPORT_MAX)+3] = CH_IDENTIFY(i, 3, TW2865_DEV_VPORT_VD2);
            }
            break;

        case 2: ///< General, 4CH Mode
        default:
            for(i=0; i<dev_num; i++) {
                TW2865_VCH_MAP[(i*TW2865_DEV_CH_MAX)+0] = CH_IDENTIFY(i, 0, TW2865_DEV_VPORT_VD1);
                TW2865_VCH_MAP[(i*TW2865_DEV_CH_MAX)+1] = CH_IDENTIFY(i, 1, TW2865_DEV_VPORT_VD1);
                TW2865_VCH_MAP[(i*TW2865_DEV_CH_MAX)+2] = CH_IDENTIFY(i, 2, TW2865_DEV_VPORT_VD1);
                TW2865_VCH_MAP[(i*TW2865_DEV_CH_MAX)+3] = CH_IDENTIFY(i, 3, TW2865_DEV_VPORT_VD1);
            }
            break;
    }

    return 0;
}

int tw2865_get_vch_id(int id, TW2865_DEV_VPORT_T vport, int vport_seq)
{
    int i;
    int vport_chnum;

    if(id >= TW2865_DEV_MAX || vport >= TW2865_DEV_VPORT_MAX || vport_seq >= 4)
        return 0;

    /* get vport max channel number */
    switch(vmode[id]) {
        case TW2865_VMODE_NTSC_720H_1CH:
        case TW2865_VMODE_PAL_720H_1CH:
            vport_chnum = 1;
            break;
        case TW2865_VMODE_NTSC_720H_2CH:
        case TW2865_VMODE_PAL_720H_2CH:
            vport_chnum = 2;
            break;
        case TW2865_VMODE_NTSC_720H_4CH:
        case TW2865_VMODE_PAL_720H_4CH:
        default:
            vport_chnum = 4;
            break;
    }

    for(i=0; i<(dev_num*TW2865_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(TW2865_VCH_MAP[i]) == id) &&
           (CH_IDENTIFY_VPORT(TW2865_VCH_MAP[i]) == vport) &&
           ((CH_IDENTIFY_VIN(TW2865_VCH_MAP[i])%vport_chnum) == vport_seq)) {
            return i;
        }
    }

    return 0;
}
EXPORT_SYMBOL(tw2865_get_vch_id);

int tw2865_vin_to_vch(int id, int vin_idx)
{
    int i;

    if(id >= TW2865_DEV_MAX || vin_idx >= TW2865_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*TW2865_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(TW2865_VCH_MAP[i]) == id) && (CH_IDENTIFY_VIN(TW2865_VCH_MAP[i]) == vin_idx))
            return i;
    }

    return 0;
}
EXPORT_SYMBOL(tw2865_vin_to_vch);

int tw2865_notify_vlos_register(int id, int (*nt_func)(int, int, int))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&tw2865_dev[id].lock);

    tw2865_dev[id].vlos_notify = nt_func;

    up(&tw2865_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(tw2865_notify_vlos_register);

void tw2865_notify_vlos_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&tw2865_dev[id].lock);

    tw2865_dev[id].vlos_notify = NULL;

    up(&tw2865_dev[id].lock);
}
EXPORT_SYMBOL(tw2865_notify_vlos_deregister);

int tw2865_notify_vfmt_register(int id, int (*nt_func)(int, int))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&tw2865_dev[id].lock);

    tw2865_dev[id].vfmt_notify = nt_func;

    up(&tw2865_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(tw2865_notify_vfmt_register);

void tw2865_notify_vfmt_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&tw2865_dev[id].lock);

    tw2865_dev[id].vfmt_notify = NULL;

    up(&tw2865_dev[id].lock);
}
EXPORT_SYMBOL(tw2865_notify_vfmt_deregister);

int tw2865_i2c_write(u8 id, u8 reg, u8 data)
{
    u8 buf[2];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return -1;

    if(!tw2865_i2c_client) {
        printk("TW2865 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(tw2865_i2c_client->dev.parent);

    buf[0]     = reg;
    buf[1]     = data;
    msgs.addr  = iaddr[id]>>1;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("TW2865#%d i2c write failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(tw2865_i2c_write);

u8 tw2865_i2c_read(u8 id, u8 reg)
{
    u8 reg_data = 0;
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return 0;

    if(!tw2865_i2c_client) {
        printk("TW2865 i2c_client not register!!\n");
        return 0;
    }

    adapter = to_i2c_adapter(tw2865_i2c_client->dev.parent);

    msgs[0].addr  = iaddr[id]>>1;
    msgs[0].flags = 0; /* write */
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    msgs[1].addr  = (iaddr[id] + 1)>>1;
    msgs[1].flags = 1; /* read */
    msgs[1].len   = 1;
    msgs[1].buf   = &reg_data;

    if(i2c_transfer(adapter, msgs, 2) != 2)
        printk("TW2865#%d i2c read failed!!\n", id);

    return reg_data;
}
EXPORT_SYMBOL(tw2865_i2c_read);

int tw2865_get_device_num(void)
{
    return dev_num;
}
EXPORT_SYMBOL(tw2865_get_device_num);

static int tw2865_proc_vmode_show(struct seq_file *sfile, void *v)
{
    int i, vmode;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;
    char *vmode_str[] = {"NTSC_720H_1CH", "NTSC_720H_2CH", "NTSC_720H_4CH", "NTSC_CIF_4CH",
                         "PAL_720H_1CH" , "PAL_720H_2CH" , "PAL_720H_4CH" , "PAL_CIF_4CH" };

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2865#%d]\n", pdev->index);
    for(i=0; i<TW2865_VMODE_MAX; i++) {
        if(tw2865_video_mode_support_check(pdev->index, i))
            seq_printf(sfile, "%02d: %s\n", i, vmode_str[i]);
    }
    seq_printf(sfile, "----------------------------------\n");

    vmode = tw2865_video_get_mode(pdev->index);
    seq_printf(sfile, "Current==> %s\n\n",(vmode >= 0 && vmode < TW2865_VMODE_MAX) ? vmode_str[vmode] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static ssize_t tw2865_proc_vmode_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int vmode;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &vmode);

    down(&pdev->lock);
    if(vmode != tw2865_video_get_mode(pdev->index))
        tw2865_video_set_mode(pdev->index, vmode);

    up(&pdev->lock);

    return count;
}

static int tw2865_proc_status_show(struct seq_file *sfile, void *v)
{
    int i, j, novid;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2865#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    NOVID     \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<TW2865_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW2865_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TW2865_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TW2865_VCH_MAP[j]) == i)) {
                novid = tw2865_status_get_novid(pdev->index, i);
                seq_printf(sfile, "%-7d %-7d %s\n", i, j, (novid != 0) ? "Video_Loss" : "Video_On");
            }
        }
    }
    seq_printf(sfile, "\n");

    up(&pdev->lock);

    return 0;
}

static int tw2865_proc_bri_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2865#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    BRIGHTNESS\n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<TW2865_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW2865_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TW2865_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TW2865_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw2865_video_get_brightness(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nBrightness[-128 ~ +127] ==> 0x01=+1, 0x7f=+127, 0x80=-128, 0xff=-1\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw2865_proc_con_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2865#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    CONTRAST  \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<TW2865_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW2865_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TW2865_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TW2865_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw2865_video_get_contrast(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nContrast[0%% ~ 255%%] ==> 0x00=0%%, 0xff=255%%\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw2865_proc_sat_u_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2865#%d]\n", pdev->index);
    seq_printf(sfile, "----------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    SATURATION_U\n");
    seq_printf(sfile, "----------------------------\n");
    for(i=0; i<TW2865_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW2865_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TW2865_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TW2865_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw2865_video_get_saturation_u(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nSaturation_U[0%% ~ 200%%] ==> 0x80=100%%\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw2865_proc_sat_v_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2865#%d]\n", pdev->index);
    seq_printf(sfile, "----------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    SATURATION_V\n");
    seq_printf(sfile, "----------------------------\n");
    for(i=0; i<TW2865_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW2865_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TW2865_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TW2865_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw2865_video_get_saturation_v(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nSaturation_V[0%% ~ 200%%] ==> 0x80=100%%\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw2865_proc_hue_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2865#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    HUE       \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<TW2865_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW2865_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TW2865_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TW2865_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw2865_video_get_hue(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nHue[0x00 ~ 0xff] ==> 0x00=0, 0x7f=90, 0x80=-90\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw2865_proc_sharp_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2865#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    SHARPNESS \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<TW2865_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW2865_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TW2865_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TW2865_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%x\n", i, j, tw2865_video_get_sharpness(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nSharpness[0x0 ~ 0xf] - (16 levels) ==> 0x0:no effect, 0x1~0xf:sharpness enhancement ('0xf' being the strongest)\n\n");

    up(&pdev->lock);

    return 0;
}

static ssize_t tw2865_proc_bri_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, vin;
    u32  bri;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &bri);

    down(&pdev->lock);

    for(i=0; i<TW2865_DEV_CH_MAX; i++) {
        if(i == vin || vin >= TW2865_DEV_CH_MAX) {
            tw2865_video_set_brightness(pdev->index, i, (u8)bri);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t tw2865_proc_con_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 con;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &con);

    down(&pdev->lock);

    for(i=0; i<TW2865_DEV_CH_MAX; i++) {
        if(i == vin || vin >= TW2865_DEV_CH_MAX) {
            tw2865_video_set_contrast(pdev->index, i, (u8)con);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t tw2865_proc_sat_u_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 sat;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &sat);

    down(&pdev->lock);

    for(i=0; i<TW2865_DEV_CH_MAX; i++) {
        if(i == vin || vin >= TW2865_DEV_CH_MAX) {
            tw2865_video_set_saturation_u(pdev->index, i, (u8)sat);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t tw2865_proc_sat_v_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 sat;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &sat);

    down(&pdev->lock);

    for(i=0; i<TW2865_DEV_CH_MAX; i++) {
        if(i == vin || vin >= TW2865_DEV_CH_MAX) {
            tw2865_video_set_saturation_v(pdev->index, i, (u8)sat);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t tw2865_proc_hue_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 hue;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &hue);

    down(&pdev->lock);

    for(i=0; i<TW2865_DEV_CH_MAX; i++) {
        if(i == vin || vin >= TW2865_DEV_CH_MAX) {
            tw2865_video_set_hue(pdev->index, i, (u8)hue);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t tw2865_proc_sharp_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 sharp;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &sharp);

    down(&pdev->lock);

    for(i=0; i<TW2865_DEV_CH_MAX; i++) {
        if(i == vin || vin >= TW2865_DEV_CH_MAX) {
            tw2865_video_set_sharpness(pdev->index, i, (u8)sharp);
        }
    }

    up(&pdev->lock);

    return count;
}

static int tw2865_proc_vmode_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2865_proc_vmode_show, PDE(inode)->data);
}

static int tw2865_proc_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2865_proc_status_show, PDE(inode)->data);
}

static int tw2865_proc_bri_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2865_proc_bri_show, PDE(inode)->data);
}

static int tw2865_proc_con_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2865_proc_con_show, PDE(inode)->data);
}

static int tw2865_proc_sat_u_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2865_proc_sat_u_show, PDE(inode)->data);
}

static int tw2865_proc_sat_v_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2865_proc_sat_v_show, PDE(inode)->data);
}

static int tw2865_proc_hue_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2865_proc_hue_show, PDE(inode)->data);
}

static int tw2865_proc_sharp_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2865_proc_sharp_show, PDE(inode)->data);
}

static struct file_operations tw2865_proc_vmode_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2865_proc_vmode_open,
    .write   = tw2865_proc_vmode_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2865_proc_status_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2865_proc_status_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2865_proc_bri_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2865_proc_bri_open,
    .write  = tw2865_proc_bri_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2865_proc_con_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2865_proc_con_open,
    .write  = tw2865_proc_con_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2865_proc_sat_u_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2865_proc_sat_u_open,
    .write  = tw2865_proc_sat_u_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2865_proc_sat_v_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2865_proc_sat_v_open,
    .write  = tw2865_proc_sat_v_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2865_proc_hue_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2865_proc_hue_open,
    .write  = tw2865_proc_hue_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2865_proc_sharp_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2865_proc_sharp_open,
    .write  = tw2865_proc_sharp_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void tw2865_proc_remove(int id)
{
    if(id >= TW2865_DEV_MAX)
        return;

    if(tw2865_proc_root[id]) {
        if(tw2865_proc_vmode[id]) {
            remove_proc_entry(tw2865_proc_vmode[id]->name, tw2865_proc_root[id]);
            tw2865_proc_vmode[id] = NULL;
        }

        if(tw2865_proc_status[id]) {
            remove_proc_entry(tw2865_proc_status[id]->name, tw2865_proc_root[id]);
            tw2865_proc_status[id] = NULL;
        }

        if(tw2865_proc_bri[id]) {
            remove_proc_entry(tw2865_proc_bri[id]->name, tw2865_proc_root[id]);
            tw2865_proc_bri[id] = NULL;
        }

        if(tw2865_proc_con[id]) {
            remove_proc_entry(tw2865_proc_con[id]->name, tw2865_proc_root[id]);
            tw2865_proc_con[id] = NULL;
        }

        if(tw2865_proc_sat_u[id]) {
            remove_proc_entry(tw2865_proc_sat_u[id]->name, tw2865_proc_root[id]);
            tw2865_proc_sat_u[id] = NULL;
        }

        if(tw2865_proc_sat_v[id]) {
            remove_proc_entry(tw2865_proc_sat_v[id]->name, tw2865_proc_root[id]);
            tw2865_proc_sat_v[id] = NULL;
        }

        if(tw2865_proc_hue[id]) {
            remove_proc_entry(tw2865_proc_hue[id]->name, tw2865_proc_root[id]);
            tw2865_proc_hue[id] = NULL;
        }

        if(tw2865_proc_sharp[id]) {
            remove_proc_entry(tw2865_proc_sharp[id]->name, tw2865_proc_root[id]);
            tw2865_proc_sharp[id] = NULL;
        }

        remove_proc_entry(tw2865_proc_root[id]->name, NULL);
        tw2865_proc_root[id] = NULL;
    }
}

static int tw2865_proc_init(int id)
{
    int ret = 0;

    if(id >= TW2865_DEV_MAX)
        return -1;

    /* root */
    tw2865_proc_root[id] = create_proc_entry(tw2865_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!tw2865_proc_root[id]) {
        printk("create proc node '%s' failed!\n", tw2865_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2865_proc_root[id]->owner = THIS_MODULE;
#endif

    /* vmode */
    tw2865_proc_vmode[id] = create_proc_entry("vmode", S_IRUGO|S_IXUGO, tw2865_proc_root[id]);
    if(!tw2865_proc_vmode[id]) {
        printk("create proc node '%s/vmode' failed!\n", tw2865_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2865_proc_vmode[id]->proc_fops = &tw2865_proc_vmode_ops;
    tw2865_proc_vmode[id]->data      = (void *)&tw2865_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2865_proc_vmode[id]->owner     = THIS_MODULE;
#endif

    /* status */
    tw2865_proc_status[id] = create_proc_entry("status", S_IRUGO|S_IXUGO, tw2865_proc_root[id]);
    if(!tw2865_proc_status[id]) {
        printk("create proc node '%s/status' failed!\n", tw2865_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2865_proc_status[id]->proc_fops = &tw2865_proc_status_ops;
    tw2865_proc_status[id]->data      = (void *)&tw2865_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2865_proc_status[id]->owner     = THIS_MODULE;
#endif

    /* brightness */
    tw2865_proc_bri[id] = create_proc_entry("brightness", S_IRUGO|S_IXUGO, tw2865_proc_root[id]);
    if(!tw2865_proc_bri[id]) {
        printk("create proc node '%s/brightness' failed!\n", tw2865_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2865_proc_bri[id]->proc_fops = &tw2865_proc_bri_ops;
    tw2865_proc_bri[id]->data      = (void *)&tw2865_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2865_proc_bri[id]->owner     = THIS_MODULE;
#endif

    /* contrast */
    tw2865_proc_con[id] = create_proc_entry("contrast", S_IRUGO|S_IXUGO, tw2865_proc_root[id]);
    if(!tw2865_proc_con[id]) {
        printk("create proc node '%s/contrast' failed!\n", tw2865_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2865_proc_con[id]->proc_fops = &tw2865_proc_con_ops;
    tw2865_proc_con[id]->data      = (void *)&tw2865_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2865_proc_con[id]->owner     = THIS_MODULE;
#endif

    /* saturation u */
    tw2865_proc_sat_u[id] = create_proc_entry("saturation_u", S_IRUGO|S_IXUGO, tw2865_proc_root[id]);
    if(!tw2865_proc_sat_u[id]) {
        printk("create proc node '%s/saturation_u' failed!\n", tw2865_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2865_proc_sat_u[id]->proc_fops = &tw2865_proc_sat_u_ops;
    tw2865_proc_sat_u[id]->data      = (void *)&tw2865_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2865_proc_sat_u[id]->owner     = THIS_MODULE;
#endif

    /* saturation v */
    tw2865_proc_sat_v[id] = create_proc_entry("saturation_v", S_IRUGO|S_IXUGO, tw2865_proc_root[id]);
    if(!tw2865_proc_sat_v[id]) {
        printk("create proc node '%s/saturation_v' failed!\n", tw2865_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2865_proc_sat_v[id]->proc_fops = &tw2865_proc_sat_v_ops;
    tw2865_proc_sat_v[id]->data      = (void *)&tw2865_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2865_proc_sat_v[id]->owner     = THIS_MODULE;
#endif

    /* hue */
    tw2865_proc_hue[id] = create_proc_entry("hue", S_IRUGO|S_IXUGO, tw2865_proc_root[id]);
    if(!tw2865_proc_hue[id]) {
        printk("create proc node '%s/hue' failed!\n", tw2865_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2865_proc_hue[id]->proc_fops = &tw2865_proc_hue_ops;
    tw2865_proc_hue[id]->data      = (void *)&tw2865_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2865_proc_hue[id]->owner     = THIS_MODULE;
#endif

    /* sharpness */
    tw2865_proc_sharp[id] = create_proc_entry("sharpness", S_IRUGO|S_IXUGO, tw2865_proc_root[id]);
    if(!tw2865_proc_sharp[id]) {
        printk("create proc node '%s/sharpness' failed!\n", tw2865_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2865_proc_sharp[id]->proc_fops = &tw2865_proc_sharp_ops;
    tw2865_proc_sharp[id]->data      = (void *)&tw2865_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2865_proc_sharp[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    tw2865_proc_remove(id);
    return ret;
}

static int tw2865_miscdev_open(struct inode *inode, struct file *file)
{
    int i, ret = 0;
    struct tw2865_dev_t *pdev = NULL;

    /* lookup device */
    for(i=0; i<dev_num; i++) {
        if(tw2865_dev[i].miscdev.minor == iminor(inode)) {
            pdev = &tw2865_dev[i];
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

static int tw2865_miscdev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long tw2865_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int tmp, ret = 0;
    struct tw2865_ioc_data ioc_data;
    struct tw2865_dev_t *pdev = (struct tw2865_dev_t *)file->private_data;

    down(&pdev->lock);

    if(_IOC_TYPE(cmd) != TW2865_IOC_MAGIC) {
        ret = -ENOIOCTLCMD;
        goto exit;
    }

    switch(cmd) {
        case TW2865_GET_NOVID:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = tw2865_status_get_novid(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case TW2865_GET_MODE:
            tmp = tw2865_video_get_mode(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }
            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case TW2865_SET_MODE:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw2865_video_set_mode(pdev->index, tmp);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW2865_GET_CONTRAST:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = tw2865_video_get_contrast(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case TW2865_SET_CONTRAST:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw2865_video_set_contrast(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW2865_GET_BRIGHTNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = tw2865_video_get_brightness(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case TW2865_SET_BRIGHTNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw2865_video_set_brightness(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW2865_GET_SATURATION_U:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = tw2865_video_get_saturation_u(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case TW2865_SET_SATURATION_U:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw2865_video_set_saturation_u(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW2865_GET_SATURATION_V:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = tw2865_video_get_saturation_v(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case TW2865_SET_SATURATION_V:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw2865_video_set_saturation_v(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW2865_GET_HUE:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = tw2865_video_get_hue(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case TW2865_SET_HUE:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw2865_video_set_hue(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW2865_GET_SHARPNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = tw2865_video_get_sharpness(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case TW2865_SET_SHARPNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw2865_video_set_sharpness(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
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

static struct file_operations tw2865_miscdev_fops = {
    .owner          = THIS_MODULE,
    .open           = tw2865_miscdev_open,
    .release        = tw2865_miscdev_release,
    .unlocked_ioctl = tw2865_miscdev_ioctl,
};

static int tw2865_miscdev_init(int id)
{
    int ret;

    if(id >= TW2865_DEV_MAX)
        return -1;

    /* clear */
    memset(&tw2865_dev[id].miscdev, 0, sizeof(struct miscdevice));

    /* create device node */
    tw2865_dev[id].miscdev.name  = tw2865_dev[id].name;
    tw2865_dev[id].miscdev.minor = MISC_DYNAMIC_MINOR;
    tw2865_dev[id].miscdev.fops  = (struct file_operations *)&tw2865_miscdev_fops;
    ret = misc_register(&tw2865_dev[id].miscdev);
    if(ret) {
        printk("create %s misc device node failed!\n", tw2865_dev[id].name);
        tw2865_dev[id].miscdev.name = 0;
        goto exit;
    }

exit:
    return ret;
}

static int tw2865_device_init(int id)
{
    int ret = 0;

    if(id >= TW2865_DEV_MAX)
        return -1;

    if(!init)
        return 0;

    /*====================== video init ========================= */
    ret = tw2865_video_set_mode(id, vmode[id]);
    if(ret < 0)
        goto exit;

    /*====================== audio init ========================= */
	ret = tw2865_audio_set_mode(id, vmode[id], sample_size, sample_rate);
    if(ret < 0)
        goto exit;

exit:
    return ret;
}

static int tw2865_watchdog_thread(void *data)
{
    int i, vin, novid, vmode;
    struct tw2865_dev_t *pdev;

    do {
            /* check tw2865 video status */
            for(i=0; i<dev_num; i++) {
                pdev = &tw2865_dev[i];

                down(&pdev->lock);

                /* video signal check */
                if(pdev->vlos_notify) {
                    for(vin=0; vin<TW2865_DEV_CH_MAX; vin++) {
                        novid = tw2865_status_get_novid(pdev->index, vin);
                        if(novid)
                            pdev->vlos_notify(i, vin, 1);
                        else
                            pdev->vlos_notify(i, vin, 0);
                    }
                }

                /* video format check */
                if(pdev->vfmt_notify) {
                    vmode = tw2865_video_get_mode(pdev->index);
                    if((vmode >= 0) && (vmode < TW2865_VMODE_MAX))
                        pdev->vfmt_notify(i, vmode);
                }

                up(&pdev->lock);
            }

            /* sleep 1 second */
            schedule_timeout_interruptible(msecs_to_jiffies(1000));

    } while (!kthread_should_stop());

    return 0;
}

static int __init tw2865_init(void)
{
    int i, ret;

    /* platfrom check */
    plat_identifier_check();

    /* check device number */
    if(dev_num > TW2865_DEV_MAX) {
        printk("TW2865 dev_num=%d invalid!!(Max=%d)\n", dev_num, TW2865_DEV_MAX);
        return -1;
    }

    /* register i2c client for contol tw2865 */
    ret = tw2865_i2c_register();
    if(ret < 0)
        return -1;

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
    ret = tw2865_create_ch_map(ch_map);
    if(ret < 0)
        goto err;

    /* TW2865 init */
    for(i=0; i<dev_num; i++) {
        tw2865_dev[i].index = i;

        sprintf(tw2865_dev[i].name, "tw2865.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&tw2865_dev[i].lock, 1);
#else
        init_MUTEX(&tw2865_dev[i].lock);
#endif
        ret = tw2865_proc_init(i);
        if(ret < 0)
            goto err;

        ret = tw2865_miscdev_init(i);
        if(ret < 0)
            goto err;

        ret = tw2865_device_init(i);
        if(ret < 0)
            goto err;
    }

    if(init && notify) {
        /* init tw2865 watchdog thread for monitor video status */
        tw2865_wd = kthread_create(tw2865_watchdog_thread, NULL, "tw2865_wd");
        if(!IS_ERR(tw2865_wd))
            wake_up_process(tw2865_wd);
        else {
            printk("create tw2865 watchdog thread failed!!\n");
            tw2865_wd = 0;
            ret = -EFAULT;
            goto err;
        }
    }

    return 0;

err:
    if(tw2865_wd)
        kthread_stop(tw2865_wd);

    tw2865_i2c_unregister();
    for(i=0; i<TW2865_DEV_MAX; i++) {
        if(tw2865_dev[i].miscdev.name)
            misc_deregister(&tw2865_dev[i].miscdev);

        tw2865_proc_remove(i);
    }

    return ret;
}

static void __exit tw2865_exit(void)
{
    int i;

    /* stop tw2865 watchdog thread */
    if(tw2865_wd)
        kthread_stop(tw2865_wd);

    tw2865_i2c_unregister();
    for(i=0; i<TW2865_DEV_MAX; i++) {
        /* remove device node */
        if(tw2865_dev[i].miscdev.name)
            misc_deregister(&tw2865_dev[i].miscdev);

        /* remove proc node */
        tw2865_proc_remove(i);
    }
}

module_init(tw2865_init);
module_exit(tw2865_exit);

MODULE_DESCRIPTION("Grain Media TW2865 Video Decoders and Audio Codecs Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
