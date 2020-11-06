/**
 * @file rn6314_drv.c
 * RichNex RN6314 4-CH D1 Video Decoders and Audio Codecs Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.7 $
 * $Date: 2014/07/29 05:36:13 $
 *
 * ChangeLog:
 *  $Log: rn6314_drv.c,v $
 *  Revision 1.7  2014/07/29 05:36:13  shawn_hu
 *  Add the module parameter of "ibus" to specify which I2C bus has the decoder attached.
 *
 *  Revision 1.6  2014/07/24 02:26:55  andy_hsu
 *  add playback volume control
 *
 *  Revision 1.5  2014/07/22 07:02:53  shawn_hu
 *  1. add IOCTL and proc node for set sharpness and video mode
 *
 *  Revision 1.4  2014/07/21 11:43:49  andy_hsu
 *  Fix rn631x codec's audio driver issue
 *
 *  Revision 1.3  2014/02/24 13:20:40  jerson_l
 *  1. fix proc node not remove problem when module removed
 *
 *  Revision 1.2  2013/12/31 02:45:41  andy_hsu
 *  add rn6314 and rn6318 audio driver
 *
 *  Revision 1.1.1.1  2013/12/03 09:58:04  jerson_l
 *  add richnex decode driver
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
#include "richnex/rn6314.h"         ///< from module/include/front_end/decoder

#define RN6314_CLKIN                27000000
#define CH_IDENTIFY(id, vin, vp)    (((id)&0xf)|(((vin)&0xf)<<4)|(((vp)&0xf)<<8))
#define CH_IDENTIFY_ID(x)           ((x)&0xf)
#define CH_IDENTIFY_VIN(x)          (((x)>>4)&0xf)
#define CH_IDENTIFY_VPORT(x)        (((x)>>8)&0xf)

/* device number */
static int dev_num = RN6314_DEV_MAX;
module_param(dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dev_num, "Device Number: 1~4");

/* device i2c bus */
static int ibus = 0;
module_param(ibus, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ibus, "Device I2C Bus: 0~4");

/* device i2c address */
static ushort iaddr[RN6314_DEV_MAX] = {0x58, 0x5a, 0x5c, 0x5e};
module_param_array(iaddr, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iaddr, "Device I2C Address");

/* device video mode */
static int vmode[RN6314_DEV_MAX] = {[0 ... (RN6314_DEV_MAX - 1)] = RN6314_VMODE_NTSC_720H_4CH};
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
static int clk_freq = RN6314_CLKIN;   ///< 27MHz
module_param(clk_freq, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_freq, "External Clock Frequency(Hz)");

/* device use gpio as rstb pin */
static int rstb_used = PLAT_GPIO_ID_X_CAP_RST;
module_param(rstb_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rstb_used, "Use GPIO as RSTB Pin: 0:None, 1:X_CAP_RST");

/* audio sample rate */
static int sample_rate = RN6314_AUDIO_SAMPLE_RATE_8K;
module_param(sample_rate, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sample_rate, "Audio Sample Rate: 0:8k  1:16k  2:32k  3:44k  4:48k");

/* audio sample size */
static int sample_size = RN6314_AUDIO_SAMPLE_SIZE_8B;
module_param(sample_size, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sample_size, "Audio Sample Size: 0:16bit  1:8bit");

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
static int audio_chnum = 8; //default is cascade
module_param(audio_chnum, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(audio_chnum, "audio channel number: 4 or 8(default)");

/******************************************************************************
 GM8210/GM8287 EVB Channel and VIN Mapping Table (Socket Board)

 VCH#0 ------> VIN#0 -----------------> VPort1/3 -------> X_CAP#1/#0
 VCH#1 ------> VIN#1 ---------|
 VCH#2 ------> VIN#2 ---------|
 VCH#3 ------> VIN#3 ---------|

*******************************************************************************/

struct rn6314_dev_t {
    int                 index;      ///< device index
    char                name[16];   ///< device name
    struct semaphore    lock;       ///< device locker
    struct miscdevice   miscdev;    ///< device node for ioctl access

    int                 (*vlos_notify)(int id, int vin, int vlos);
    int                 (*vfmt_notify)(int id, int vmode);
};

static struct rn6314_dev_t    rn6314_dev[RN6314_DEV_MAX];
static struct proc_dir_entry *rn6314_proc_root[RN6314_DEV_MAX]     = {[0 ... (RN6314_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6314_proc_vmode[RN6314_DEV_MAX]    = {[0 ... (RN6314_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6314_proc_status[RN6314_DEV_MAX]   = {[0 ... (RN6314_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6314_proc_bri[RN6314_DEV_MAX]      = {[0 ... (RN6314_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6314_proc_con[RN6314_DEV_MAX]      = {[0 ... (RN6314_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6314_proc_sat[RN6314_DEV_MAX]      = {[0 ... (RN6314_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6314_proc_hue[RN6314_DEV_MAX]      = {[0 ... (RN6314_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6314_proc_sharp[RN6314_DEV_MAX]       = {[0 ... (RN6314_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6314_proc_sr[RN6314_DEV_MAX]       = {[0 ... (RN6314_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6314_proc_ss[RN6314_DEV_MAX]       = {[0 ... (RN6314_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6314_proc_pb[RN6314_DEV_MAX]       = {[0 ... (RN6314_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6314_proc_bp[RN6314_DEV_MAX]       = {[0 ... (RN6314_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6314_proc_vol[RN6314_DEV_MAX]      = {[0 ... (RN6314_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6314_proc_playback_vol[RN6314_DEV_MAX]      = {[0 ... (RN6314_DEV_MAX - 1)] = NULL};

static struct task_struct *rn6314_wd         = NULL;
static struct i2c_client  *rn6314_i2c_client = NULL;
static u32 RN6314_VCH_MAP[RN6314_DEV_MAX*RN6314_DEV_CH_MAX];
static audio_funs_t rn6314_audio_funs;

static int rn6314_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    rn6314_i2c_client = client;
    return 0;
}

static int rn6314_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id rn6314_i2c_id[] = {
    { "rn6314_i2c", 0 },
    { }
};

static struct i2c_driver rn6314_i2c_driver = {
    .driver = {
        .name  = "RN6314_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = rn6314_i2c_probe,
    .remove   = rn6314_i2c_remove,
    .id_table = rn6314_i2c_id
};

static int rn6314_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&rn6314_i2c_driver);
    if(ret < 0) {
        printk("RN6314 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = iaddr[0]>>1;
    strlcpy(info.type, "rn6314_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("RN6314 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("RN6314 can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&rn6314_i2c_driver);
   return -ENODEV;
}

static void rn6314_i2c_unregister(void)
{
    i2c_unregister_device(rn6314_i2c_client);
    i2c_del_driver(&rn6314_i2c_driver);
    rn6314_i2c_client = NULL;
}

static int rn6314_create_ch_map(int map_id)
{
    int i;

    switch(map_id) {
        case 0: ///< for GM8210/GM8287 Socket Board
        default:
            for(i=0; i<dev_num; i++) {
                RN6314_VCH_MAP[(i*RN6314_DEV_CH_MAX)+0] = CH_IDENTIFY(i, 0, RN6314_DEV_VPORT_3);
                RN6314_VCH_MAP[(i*RN6314_DEV_CH_MAX)+1] = CH_IDENTIFY(i, 1, RN6314_DEV_VPORT_3);
                RN6314_VCH_MAP[(i*RN6314_DEV_CH_MAX)+2] = CH_IDENTIFY(i, 2, RN6314_DEV_VPORT_3);
                RN6314_VCH_MAP[(i*RN6314_DEV_CH_MAX)+3] = CH_IDENTIFY(i, 3, RN6314_DEV_VPORT_3);
            }
            break;
    }

    return 0;
}

int rn6314_vin_to_ch(int id, int vin_idx)
{
    int i;

    if(id >= RN6314_DEV_MAX || vin_idx >= RN6314_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*RN6314_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(RN6314_VCH_MAP[i]) == id) && (CH_IDENTIFY_VIN(RN6314_VCH_MAP[i]) == vin_idx))
            return (i%RN6314_DEV_CH_MAX);
    }

    return 0;
}
EXPORT_SYMBOL(rn6314_vin_to_ch);

int rn6314_vin_to_vch(int id, int vin_idx)
{
    int i;

    if(id >= RN6314_DEV_MAX || vin_idx >= RN6314_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*RN6314_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(RN6314_VCH_MAP[i]) == id) && (CH_IDENTIFY_VIN(RN6314_VCH_MAP[i]) == vin_idx))
            return i;
    }

    return 0;
}
EXPORT_SYMBOL(rn6314_vin_to_vch);

int rn6314_get_vch_id(int id, RN6314_DEV_VPORT_T vport, int vport_seq)
{
    int i;

    if(id >= RN6314_DEV_MAX || vport >= RN6314_DEV_VPORT_MAX || vport_seq >= 4)
        return 0;

    for(i=0; i<(dev_num*RN6314_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(RN6314_VCH_MAP[i]) == id) &&
           (CH_IDENTIFY_VPORT(RN6314_VCH_MAP[i]) == vport) &&
           ((CH_IDENTIFY_VIN(RN6314_VCH_MAP[i])%4) == vport_seq)) {
            return i;
        }
    }

    return 0;
}
EXPORT_SYMBOL(rn6314_get_vch_id);

int rn6314_notify_vlos_register(int id, int (*nt_func)(int, int, int))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&rn6314_dev[id].lock);

    rn6314_dev[id].vlos_notify = nt_func;

    up(&rn6314_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(rn6314_notify_vlos_register);

void rn6314_notify_vlos_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&rn6314_dev[id].lock);

    rn6314_dev[id].vlos_notify = NULL;

    up(&rn6314_dev[id].lock);
}
EXPORT_SYMBOL(rn6314_notify_vlos_deregister);

int rn6314_notify_vfmt_register(int id, int (*nt_func)(int, int))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&rn6314_dev[id].lock);

    rn6314_dev[id].vfmt_notify = nt_func;

    up(&rn6314_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(rn6314_notify_vfmt_register);

void rn6314_notify_vfmt_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&rn6314_dev[id].lock);

    rn6314_dev[id].vfmt_notify = NULL;

    up(&rn6314_dev[id].lock);
}
EXPORT_SYMBOL(rn6314_notify_vfmt_deregister);

int rn6314_i2c_write(u8 id, u8 reg, u8 data)
{
    u8 buf[2];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return -1;

    if(!rn6314_i2c_client) {
        printk("RN6314 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(rn6314_i2c_client->dev.parent);

    buf[0]     = reg;
    buf[1]     = data;
    msgs.addr  = iaddr[id]>>1;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("RN6314#%d i2c write failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(rn6314_i2c_write);

u8 rn6314_i2c_read(u8 id, u8 reg)
{
    u8 reg_data = 0;
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return 0;

    if(!rn6314_i2c_client) {
        printk("RN6314 i2c_client not register!!\n");
        return 0;
    }

    adapter = to_i2c_adapter(rn6314_i2c_client->dev.parent);

    msgs[0].addr  = iaddr[id]>>1;
    msgs[0].flags = 0; /* write */
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    msgs[1].addr  = (iaddr[id] + 1)>>1;
    msgs[1].flags = 1; /* read */
    msgs[1].len   = 1;
    msgs[1].buf   = &reg_data;

    if(i2c_transfer(adapter, msgs, 2) != 2)
        printk("RN6314#%d i2c read failed!!\n", id);

    return reg_data;
}
EXPORT_SYMBOL(rn6314_i2c_read);

int rn6314_i2c_write_table(u8 id, u8 *ptable, int size)
{
    int i;
    int ret = 0;

    if(id >= dev_num || !ptable || (size < 2) || (size%2 != 0))
        return -1;

    for(i=0; i<size; i+=2) {
        ret = rn6314_i2c_write(id, ptable[i], ptable[i+1]);
        if(ret < 0)
            goto exit;
    }

exit:
    return ret;
}
EXPORT_SYMBOL(rn6314_i2c_write_table);

int rn6314_get_device_num(void)
{
    return dev_num;
}
EXPORT_SYMBOL(rn6314_get_device_num);

static int rn6314_proc_vmode_show(struct seq_file *sfile, void *v)
{
    int i, vmode;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;
    char *vmode_str[] = {"NTSC_720H_1CH", "NTSC_720H_2CH", "NTSC_720H_4CH",
                         "NTSC_960H_1CH", "NTSC_960H_2CH", "NTSC_960H_4CH",
                         "PAL_720H_1CH" , "PAL_720H_2CH" , "PAL_720H_4CH",
                         "PAL_960H_1CH" , "PAL_960H_2CH" , "PAL_960H_4CH"};

    down(&pdev->lock);

    seq_printf(sfile, "\n[RN6314#%d]\n", pdev->index);
    for(i=0; i<RN6314_VMODE_MAX; i++) {
        if(rn6314_video_mode_support_check(pdev->index, i))
            seq_printf(sfile, "%02d: %s\n", i, vmode_str[i]);
    }
    seq_printf(sfile, "----------------------------------\n");

    vmode = rn6314_video_get_mode(pdev->index);
    seq_printf(sfile, "Current==> %s\n\n",(vmode >= 0 && vmode < RN6314_VMODE_MAX) ? vmode_str[vmode] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static ssize_t rn6314_proc_vmode_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int vmode;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &vmode);

    down(&pdev->lock);

    if(vmode != rn6314_video_get_mode(pdev->index))
        rn6314_video_set_mode(pdev->index, vmode);

    up(&pdev->lock);

    return count;
}

static int rn6314_proc_status_show(struct seq_file *sfile, void *v)
{
    int i, j, novid;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[RN6314#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    NOVID     \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<RN6314_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*RN6314_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(RN6314_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(RN6314_VCH_MAP[j]) == i)) {
                novid = rn6314_status_get_novid(pdev->index, i);
                seq_printf(sfile, "%-7d %-7d %s\n", i, j, (novid == 1) ? "Video_Loss" : "Video_On");
            }
        }
    }
    seq_printf(sfile, "\n");

    up(&pdev->lock);

    return 0;
}

static int rn6314_proc_bri_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[RN6314#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    BRIGHTNESS\n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<RN6314_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*RN6314_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(RN6314_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(RN6314_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, rn6314_video_get_brightness(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nBrightness[-128 ~ +127] ==> 0x01=+1, 0x7f=+127, 0x80=-128, 0xff=-1\n\n");

    up(&pdev->lock);

    return 0;
}

static int rn6314_proc_con_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[RN6314#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    CONTRAST  \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<RN6314_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*RN6314_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(RN6314_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(RN6314_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, rn6314_video_get_contrast(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nContrast[0x00 ~ 0xff]\n\n");

    up(&pdev->lock);

    return 0;
}

static int rn6314_proc_sat_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[RN6314#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    SATURATION\n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<RN6314_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*RN6314_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(RN6314_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(RN6314_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, rn6314_video_get_saturation(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nSaturation[0x00 ~ 0xff]\n\n");

    up(&pdev->lock);

    return 0;
}

static int rn6314_proc_hue_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[RN6314#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    HUE       \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<RN6314_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*RN6314_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(RN6314_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(RN6314_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, rn6314_video_get_hue(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nHue[0x00 ~ 0xff]\n\n");

    up(&pdev->lock);

    return 0;
}

static int rn6314_proc_sharp_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[RN6314#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    SHARPNESS \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<RN6314_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*RN6314_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(RN6314_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(RN6314_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, rn6314_video_get_sharpness(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nSharpness ==> [7]:peaking frequency selection ==> 0:low freq. 1:high freq.");
    seq_printf(sfile, "\nSharpness ==> [6:0]:gain ==> 0:disable 0x01~0x7F:amplitude\n\n");

    up(&pdev->lock);

    return 0;
}

static int rn6314_proc_sample_rate_show(struct seq_file *sfile, void *v)
{
    int sample_rate;
    int sr[]={8,16,32,44,48};
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    down(&pdev->lock);

    sample_rate=rn6314_audio_get_sample_rate(pdev->index);
    seq_printf(sfile, "\n[RN6314#%d]\n", pdev->index);
    seq_printf(sfile, "Current Audio Sample Rate: %d KBps\n\n", (sample_rate>=0)?(sr[sample_rate]):(-1));

    up(&pdev->lock);

    return 0;
}

static int rn6314_proc_sample_size_show(struct seq_file *sfile, void *v)
{
    int sample_size;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    down(&pdev->lock);

    sample_size=rn6314_audio_get_sample_size(pdev->index);
    seq_printf(sfile, "\n[RN6314#%d]\n", pdev->index);
    seq_printf(sfile, "Current Audio Sample Size: %d Bit\n\n", (sample_size==0)?(16):(8));

    up(&pdev->lock);

    return 0;
}

static int rn6314_proc_playback_ch_show(struct seq_file *sfile, void *v)
{
    int pb_ch;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    down(&pdev->lock);

    pb_ch=rn6314_audio_get_playback_channel();
    seq_printf(sfile, "\n[RN6314#%d]\n", pdev->index);
    if (pb_ch>=0)
        seq_printf(sfile, "Current Audio Playback Channel: %d\n\n", pb_ch);
    else
        seq_printf(sfile, "Current is not under playback mode!\n\n");
    up(&pdev->lock);

    return 0;
}

static int rn6314_proc_bypass_ch_show(struct seq_file *sfile, void *v)
{
    int bp_ch;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    down(&pdev->lock);

    bp_ch=rn6314_audio_get_bypass_channel();
    seq_printf(sfile, "\n[RN6314#%d]\n", pdev->index);
    if (bp_ch>=0)
        seq_printf(sfile, "Current Audio Playback Channel: %d\n\n", bp_ch);
    else
        seq_printf(sfile, "Current is not under bypass mode!\n\n");
    up(&pdev->lock);

    return 0;
}

static int rn6314_proc_vol_show(struct seq_file *sfile, void *v)
{
    int ch, tmp;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[RN6314#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "AIN#    ACH#    VOL       \n");
    seq_printf(sfile, "--------------------------\n");

    if (rn6314_i2c_write(pdev->index, 0xff, 0x04) < 0)
        return -1;

    for (ch=0; ch<RN6314_DEV_CH_MAX; ch++) {
        int displayCH=0;
        if (ch<4) {
            tmp = rn6314_i2c_read(pdev->index, 0x5B+(ch/2));     ///< 0x71:CH0/CH1, 0x72:CH2/CH3
            tmp &= 0x0f<<(4*(ch%2));
            tmp >>= 4*(ch%2);
        } 
#if 0
        else if (ch<6) {
            tmp = rn6314_i2c_read(pdev->index, 0x73);            ///< PA0/PAB
            tmp &= 0xf<<(4*(ch%4));
            tmp >>= 4*(ch%4);
        } else if (ch<8) {
            tmp = rn6314_i2c_read(pdev->index, 0x74);            ///< MIX/MIX40
            tmp &= 0xf<<(4*(ch%6));
            tmp >>= 4*(ch%6);
        }
#endif
        displayCH = ((pdev->index)==0)?(ch):(ch+4);
        seq_printf(sfile, "%-8d%-8d0x%X\n",displayCH,displayCH,tmp);
    }
    seq_printf(sfile, "\nVolume[0x0 ~ 0xF]\n\n");

    up(&pdev->lock);

    return 0;
}

static int rn6314_proc_playback_vol_show(struct seq_file *sfile, void *v)
{
    int tmp;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    down(&pdev->lock);

    tmp = rn6314_audio_get_playback_volume(pdev->index);

    seq_printf(sfile, "\n[RN6314#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "Playback Volume: 0x%X     \n", tmp);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "Volume[0x0 ~ 0xF]\n\n");

    up(&pdev->lock);

    return 0;
}

static ssize_t rn6314_proc_bri_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, vin;
    u32  bri;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &bri);

    down(&pdev->lock);

    for(i=0; i<RN6314_DEV_CH_MAX; i++) {
        if(i == vin || vin >= RN6314_DEV_CH_MAX) {
            rn6314_video_set_brightness(pdev->index, i, (u8)bri);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t rn6314_proc_con_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 con;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &con);

    down(&pdev->lock);

    for(i=0; i<RN6314_DEV_CH_MAX; i++) {
        if(i == vin || vin >= RN6314_DEV_CH_MAX) {
            rn6314_video_set_contrast(pdev->index, i, (u8)con);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t rn6314_proc_sat_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 sat;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &sat);

    down(&pdev->lock);

    for(i=0; i<RN6314_DEV_CH_MAX; i++) {
        if(i == vin || vin >= RN6314_DEV_CH_MAX) {
            rn6314_video_set_saturation(pdev->index, i, (u8)sat);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t rn6314_proc_hue_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 hue;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &hue);

    down(&pdev->lock);

    for(i=0; i<RN6314_DEV_CH_MAX; i++) {
        if(i == vin || vin >= RN6314_DEV_CH_MAX) {
            rn6314_video_set_hue(pdev->index, i, (u8)hue);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t rn6314_proc_sharp_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 sharp;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &sharp);

    down(&pdev->lock);

    for(i=0; i<RN6314_DEV_CH_MAX; i++) {
        if(i == vin || vin >= RN6314_DEV_CH_MAX) {
            rn6314_video_set_sharpness(pdev->index, i, (u8)sharp);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t rn6314_proc_sample_rate_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int val=0, sr=0;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &val);

    switch (val) {
        case 8000:
            sr=RN6314_AUDIO_SAMPLE_RATE_8K;
            break;
        case 16000:
            sr=RN6314_AUDIO_SAMPLE_RATE_16K;
            break;
        case 32000:
            sr=RN6314_AUDIO_SAMPLE_RATE_32K;
            break;
        case 44000:
            sr=RN6314_AUDIO_SAMPLE_RATE_44K;
            break;
        case 48000:
            sr=RN6314_AUDIO_SAMPLE_RATE_48K;
            break;
        default:
            printk("\nInvalid sample rate %d\n\n",val);
            return -EFAULT;
    }

    down(&pdev->lock);

    rn6314_audio_set_sample_rate(pdev->index, sr);
    //printk("\nAudio sample rate change to %d KBps success!\n\n",(val/1000));

    up(&pdev->lock);

    return count;
}

static ssize_t rn6314_proc_sample_size_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int val=0, ss=0;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &val);

    if ((val!=8)&&(val!=16)) {
        printk("\nInvalid sample size %d bit!\n\n",val);
        return -EFAULT;
    }

    ss=(val==8)?(RN6314_AUDIO_SAMPLE_SIZE_8B):(RN6314_AUDIO_SAMPLE_SIZE_16B);

    down(&pdev->lock);

    rn6314_audio_set_sample_size(pdev->index,ss);
    //printk("\nAudio sample size change to %d bit success!\n\n",val);

    up(&pdev->lock);

    return count;

}

static ssize_t rn6314_proc_playback_ch_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int ch=0;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &ch);

    if ((ch<0)||(ch>0xf)) {
        printk("\nInvalid channel %X!\n\n",ch);
        return -EFAULT;
    }

    rn6314_audio_set_playback_channel(pdev->index,ch);

    return count;

}

static ssize_t rn6314_proc_bypass_ch_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int ch=0;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &ch);

    if (((ch<0)||(ch>0xf))&&(ch!=0xff)) {
        printk("\nInvalid channel %X!\n\n",ch);
        return -EFAULT;
    }

    rn6314_audio_set_bypass_channel(pdev->index,ch);

    return count;

}

static ssize_t rn6314_proc_vol_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int ch,vol,channel;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &ch, &vol); //ch=4~7 if chip ID=1

    channel = ((pdev->index)==0)?(ch):(ch-4);

    if ((channel<0)||(channel>=RN6314_DEV_CH_MAX)) {
        printk("\nInvalid channel %X!\n\n",((pdev->index)==0)?(channel):(channel+4));
        return -EFAULT;
    }

    if ((vol<0)||(vol>0xf)) {
        printk("\nInvalid volume setting %X!\n\n",vol);
        return -EFAULT;
    }

    rn6314_audio_set_volume(pdev->index,channel,vol);

    return count;

}

static ssize_t rn6314_proc_playback_vol_write(struct file *file, 
                        const char __user *buffer, size_t count, loff_t *ppos)
{
    int vol;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)sfile->private;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%x\n", &vol);

    if ((vol<0)||(vol>0xf)) {
        printk("\nInvalid volume setting %X!\n\n",vol);
        return -EFAULT;
    }

    rn6314_audio_set_playback_volume(pdev->index,vol);

    return count;

}

static int rn6314_proc_vmode_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6314_proc_vmode_show, PDE(inode)->data);
}

static int rn6314_proc_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6314_proc_status_show, PDE(inode)->data);
}

static int rn6314_proc_bri_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6314_proc_bri_show, PDE(inode)->data);
}

static int rn6314_proc_con_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6314_proc_con_show, PDE(inode)->data);
}

static int rn6314_proc_sat_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6314_proc_sat_show, PDE(inode)->data);
}

static int rn6314_proc_hue_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6314_proc_hue_show, PDE(inode)->data);
}

static int rn6314_proc_sharp_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6314_proc_sharp_show, PDE(inode)->data);
}

static int rn6314_proc_sample_rate_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6314_proc_sample_rate_show, PDE(inode)->data);
}

static int rn6314_proc_sample_size_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6314_proc_sample_size_show, PDE(inode)->data);
}

static int rn6314_proc_playback_ch_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6314_proc_playback_ch_show, PDE(inode)->data);
}

static int rn6314_proc_bypass_ch_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6314_proc_bypass_ch_show, PDE(inode)->data);
}

static int rn6314_proc_vol_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6314_proc_vol_show, PDE(inode)->data);
}

static int rn6314_proc_playback_vol_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6314_proc_playback_vol_show, PDE(inode)->data);
}

static struct file_operations rn6314_proc_vmode_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6314_proc_vmode_open,
    .write  = rn6314_proc_vmode_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations rn6314_proc_status_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6314_proc_status_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations rn6314_proc_bri_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6314_proc_bri_open,
    .write  = rn6314_proc_bri_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations rn6314_proc_con_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6314_proc_con_open,
    .write  = rn6314_proc_con_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations rn6314_proc_sat_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6314_proc_sat_open,
    .write  = rn6314_proc_sat_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations rn6314_proc_hue_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6314_proc_hue_open,
    .write  = rn6314_proc_hue_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations rn6314_proc_sharp_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6314_proc_sharp_open,
    .write  = rn6314_proc_sharp_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations rn6314_proc_sample_rate_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6314_proc_sample_rate_open,
    .write  = rn6314_proc_sample_rate_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations rn6314_proc_sample_size_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6314_proc_sample_size_open,
    .write  = rn6314_proc_sample_size_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations rn6314_proc_playback_ch_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6314_proc_playback_ch_open,
    .write  = rn6314_proc_playback_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations rn6314_proc_bypass_ch_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6314_proc_bypass_ch_open,
    .write  = rn6314_proc_bypass_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations rn6314_proc_vol_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6314_proc_vol_open,
    .write  = rn6314_proc_vol_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations rn6314_proc_playback_vol_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6314_proc_playback_vol_open,
    .write  = rn6314_proc_playback_vol_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void rn6314_proc_remove(int id)
{
    if(id >= RN6314_DEV_MAX)
        return;

    if(rn6314_proc_root[id]) {
        if(rn6314_proc_vmode[id]) {
            remove_proc_entry(rn6314_proc_vmode[id]->name, rn6314_proc_root[id]);
            rn6314_proc_vmode[id] = NULL;
        }

        if(rn6314_proc_status[id]) {
            remove_proc_entry(rn6314_proc_status[id]->name, rn6314_proc_root[id]);
            rn6314_proc_status[id] = NULL;
        }

        if(rn6314_proc_bri[id]) {
            remove_proc_entry(rn6314_proc_bri[id]->name, rn6314_proc_root[id]);
            rn6314_proc_bri[id] = NULL;
        }

        if(rn6314_proc_con[id]) {
            remove_proc_entry(rn6314_proc_con[id]->name, rn6314_proc_root[id]);
            rn6314_proc_con[id] = NULL;
        }

        if(rn6314_proc_sat[id]) {
            remove_proc_entry(rn6314_proc_sat[id]->name, rn6314_proc_root[id]);
            rn6314_proc_sat[id] = NULL;
        }

        if(rn6314_proc_hue[id]) {
            remove_proc_entry(rn6314_proc_hue[id]->name, rn6314_proc_root[id]);
            rn6314_proc_hue[id] = NULL;
        }

        if(rn6314_proc_sharp[id]) {
            remove_proc_entry(rn6314_proc_sharp[id]->name, rn6314_proc_root[id]);
            rn6314_proc_sharp[id] = NULL;
        }

        if(rn6314_proc_sr[id]) {
            remove_proc_entry(rn6314_proc_sr[id]->name, rn6314_proc_root[id]);
            rn6314_proc_sr[id] = NULL;
        }

        if(rn6314_proc_ss[id]) {
            remove_proc_entry(rn6314_proc_ss[id]->name, rn6314_proc_root[id]);
            rn6314_proc_ss[id] = NULL;
        }

        if(rn6314_proc_pb[id]) {
            remove_proc_entry(rn6314_proc_pb[id]->name, rn6314_proc_root[id]);
            rn6314_proc_pb[id] = NULL;
        }

        if(rn6314_proc_bp[id]) {
            remove_proc_entry(rn6314_proc_bp[id]->name, rn6314_proc_root[id]);
            rn6314_proc_bp[id] = NULL;
        }

        if(rn6314_proc_vol[id]) {
            remove_proc_entry(rn6314_proc_vol[id]->name, rn6314_proc_root[id]);
            rn6314_proc_vol[id] = NULL;
        }

        if(rn6314_proc_playback_vol[id]) {
            remove_proc_entry(rn6314_proc_playback_vol[id]->name, rn6314_proc_root[id]);
            rn6314_proc_playback_vol[id] = NULL;
        }

        remove_proc_entry(rn6314_proc_root[id]->name, NULL);
        rn6314_proc_root[id] = NULL;
    }
}

static int rn6314_proc_init(int id)
{
    int ret = 0;

    if(id >= RN6314_DEV_MAX)
        return -1;

    /* root */
    rn6314_proc_root[id] = create_proc_entry(rn6314_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!rn6314_proc_root[id]) {
        printk("create proc node '%s' failed!\n", rn6314_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6314_proc_root[id]->owner = THIS_MODULE;
#endif

    /* vmode */
    rn6314_proc_vmode[id] = create_proc_entry("vmode", S_IRUGO|S_IXUGO, rn6314_proc_root[id]);
    if(!rn6314_proc_vmode[id]) {
        printk("create proc node '%s/vmode' failed!\n", rn6314_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    rn6314_proc_vmode[id]->proc_fops = &rn6314_proc_vmode_ops;
    rn6314_proc_vmode[id]->data      = (void *)&rn6314_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6314_proc_vmode[id]->owner     = THIS_MODULE;
#endif

    /* status */
    rn6314_proc_status[id] = create_proc_entry("status", S_IRUGO|S_IXUGO, rn6314_proc_root[id]);
    if(!rn6314_proc_status[id]) {
        printk("create proc node '%s/status' failed!\n", rn6314_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    rn6314_proc_status[id]->proc_fops = &rn6314_proc_status_ops;
    rn6314_proc_status[id]->data      = (void *)&rn6314_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6314_proc_status[id]->owner     = THIS_MODULE;
#endif

    /* brightness */
    rn6314_proc_bri[id] = create_proc_entry("brightness", S_IRUGO|S_IXUGO, rn6314_proc_root[id]);
    if(!rn6314_proc_bri[id]) {
        printk("create proc node '%s/brightness' failed!\n", rn6314_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    rn6314_proc_bri[id]->proc_fops = &rn6314_proc_bri_ops;
    rn6314_proc_bri[id]->data      = (void *)&rn6314_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6314_proc_bri[id]->owner     = THIS_MODULE;
#endif

    /* contrast */
    rn6314_proc_con[id] = create_proc_entry("contrast", S_IRUGO|S_IXUGO, rn6314_proc_root[id]);
    if(!rn6314_proc_con[id]) {
        printk("create proc node '%s/contrast' failed!\n", rn6314_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    rn6314_proc_con[id]->proc_fops = &rn6314_proc_con_ops;
    rn6314_proc_con[id]->data      = (void *)&rn6314_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6314_proc_con[id]->owner     = THIS_MODULE;
#endif

    /* saturation */
    rn6314_proc_sat[id] = create_proc_entry("saturation", S_IRUGO|S_IXUGO, rn6314_proc_root[id]);
    if(!rn6314_proc_sat[id]) {
        printk("create proc node '%s/saturation' failed!\n", rn6314_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    rn6314_proc_sat[id]->proc_fops = &rn6314_proc_sat_ops;
    rn6314_proc_sat[id]->data      = (void *)&rn6314_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6314_proc_sat[id]->owner     = THIS_MODULE;
#endif

    /* hue */
    rn6314_proc_hue[id] = create_proc_entry("hue", S_IRUGO|S_IXUGO, rn6314_proc_root[id]);
    if(!rn6314_proc_hue[id]) {
        printk("create proc node '%s/hue' failed!\n", rn6314_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    rn6314_proc_hue[id]->proc_fops = &rn6314_proc_hue_ops;
    rn6314_proc_hue[id]->data      = (void *)&rn6314_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6314_proc_hue[id]->owner     = THIS_MODULE;
#endif

    /* sharpness */
    rn6314_proc_sharp[id] = create_proc_entry("sharpness", S_IRUGO|S_IXUGO, rn6314_proc_root[id]);
    if(!rn6314_proc_sharp[id]) {
        printk("create proc node '%s/sharpness' failed!\n", rn6314_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    rn6314_proc_sharp[id]->proc_fops = &rn6314_proc_sharp_ops;
    rn6314_proc_sharp[id]->data      = (void *)&rn6314_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6314_proc_sharp[id]->owner     = THIS_MODULE;
#endif

    /* sample rate */
    rn6314_proc_sr[id] = create_proc_entry("sample_rate", S_IRUGO|S_IXUGO, rn6314_proc_root[id]);
    if(!rn6314_proc_sr[id]) {
        printk("create proc node '%s/sample_rate' failed!\n", rn6314_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    rn6314_proc_sr[id]->proc_fops = &rn6314_proc_sample_rate_ops;
    rn6314_proc_sr[id]->data      = (void *)&rn6314_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6314_proc_sr[id]->owner     = THIS_MODULE;
#endif

    /* sample size */
    rn6314_proc_ss[id] = create_proc_entry("sample_size", S_IRUGO|S_IXUGO, rn6314_proc_root[id]);
    if(!rn6314_proc_ss[id]) {
        printk("create proc node '%s/sample_size' failed!\n", rn6314_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    rn6314_proc_ss[id]->proc_fops = &rn6314_proc_sample_size_ops;
    rn6314_proc_ss[id]->data      = (void *)&rn6314_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6314_proc_ss[id]->owner     = THIS_MODULE;
#endif

    /* playback channel */
    rn6314_proc_pb[id] = create_proc_entry("playback_channel", S_IRUGO|S_IXUGO, rn6314_proc_root[id]);
    if(!rn6314_proc_pb[id]) {
        printk("create proc node '%s/playback_channel' failed!\n", rn6314_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    rn6314_proc_pb[id]->proc_fops = &rn6314_proc_playback_ch_ops;
    rn6314_proc_pb[id]->data      = (void *)&rn6314_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6314_proc_pb[id]->owner     = THIS_MODULE;
#endif

    /* bypass channel */
    rn6314_proc_bp[id] = create_proc_entry("bypass_channel", S_IRUGO|S_IXUGO, rn6314_proc_root[id]);
    if(!rn6314_proc_bp[id]) {
        printk("create proc node '%s/bypass_channel' failed!\n", rn6314_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    rn6314_proc_bp[id]->proc_fops = &rn6314_proc_bypass_ch_ops;
    rn6314_proc_bp[id]->data      = (void *)&rn6314_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6314_proc_bp[id]->owner     = THIS_MODULE;
#endif

    /* audio volume */
    rn6314_proc_vol[id] = create_proc_entry("audio_volume", S_IRUGO|S_IXUGO, rn6314_proc_root[id]);
    if(!rn6314_proc_vol[id]) {
        printk("create proc node '%s/audio_volume' failed!\n", rn6314_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    rn6314_proc_vol[id]->proc_fops = &rn6314_proc_vol_ops;
    rn6314_proc_vol[id]->data      = (void *)&rn6314_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6314_proc_vol[id]->owner     = THIS_MODULE;
#endif

    /* audio playback volume */
    rn6314_proc_playback_vol[id] = create_proc_entry("audio_playback_volume", S_IRUGO|S_IXUGO, rn6314_proc_root[id]);
    if(!rn6314_proc_playback_vol[id]) {
        printk("create proc node '%s/audio_playback_volume' failed!\n", rn6314_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    rn6314_proc_playback_vol[id]->proc_fops = &rn6314_proc_playback_vol_ops;
    rn6314_proc_playback_vol[id]->data      = (void *)&rn6314_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6314_proc_playback_vol[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    rn6314_proc_remove(id);
    return ret;
}

static int rn6314_miscdev_open(struct inode *inode, struct file *file)
{
    int i, ret = 0;
    struct rn6314_dev_t *pdev = NULL;

    /* lookup device */
    for(i=0; i<dev_num; i++) {
        if(rn6314_dev[i].miscdev.minor == iminor(inode)) {
            pdev = &rn6314_dev[i];
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

static int rn6314_miscdev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long rn6314_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int tmp, ret = 0;
    struct rn6314_ioc_data ioc_data;
    struct rn6314_dev_t *pdev = (struct rn6314_dev_t *)file->private_data;

    down(&pdev->lock);

    if(_IOC_TYPE(cmd) != RN6314_IOC_MAGIC) {
        ret = -ENOIOCTLCMD;
        goto exit;
    }

    switch(cmd) {
        case RN6314_GET_NOVID:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            if(ioc_data.ch >= RN6314_DEV_CH_MAX) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = rn6314_status_get_novid(pdev->index, ioc_data.ch);
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case RN6314_GET_MODE:
            tmp = rn6314_video_get_mode(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }
            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case RN6314_SET_MODE:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = rn6314_video_set_mode(pdev->index, tmp);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case RN6314_GET_CONTRAST:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = rn6314_video_get_contrast(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case RN6314_SET_CONTRAST:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = rn6314_video_set_contrast(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case RN6314_GET_BRIGHTNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = rn6314_video_get_brightness(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case RN6314_SET_BRIGHTNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = rn6314_video_set_brightness(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case RN6314_GET_SATURATION:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = rn6314_video_get_saturation(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case RN6314_SET_SATURATION:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = rn6314_video_set_saturation(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case RN6314_GET_HUE:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = rn6314_video_get_hue(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case RN6314_SET_HUE:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = rn6314_video_set_hue(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case RN6314_GET_SHARPNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = rn6314_video_get_sharpness(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case RN6314_SET_SHARPNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = rn6314_video_set_sharpness(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case RN6314_GET_SAMPLE_RATE:
        {
            int val,sr[RN6314_AUDIO_SAMPLE_RATE_MAX]={8000,16000,32000,44100,48000};
            if((tmp=rn6314_audio_get_sample_rate(pdev->index))<0) {
                ret = -EFAULT;
                goto exit;
            }
            val=sr[tmp];
            ret = (copy_to_user((void __user *)arg, &val, sizeof(int))) ? (-EFAULT) : 0;
            break;
        }
        case RN6314_SET_SAMPLE_RATE:
        {
            int i;
            int sr[RN6314_AUDIO_SAMPLE_RATE_MAX]={8000,16000,32000,44100,48000};
            if (copy_from_user(&tmp, (void __user *)arg, sizeof(int))) {
                ret = -EFAULT;
                goto exit;
            }

            for (i=0; i<RN6314_AUDIO_SAMPLE_RATE_MAX; i++)
                if (sr[i]==tmp)
                    break;

            if (i>=RN6314_AUDIO_SAMPLE_RATE_MAX) {
                ret=-EFAULT;
                goto exit;
            }

            if (rn6314_audio_set_sample_rate(pdev->index,i)<0) {
                ret=-EFAULT;
                goto exit;
            }

            break;
        }
        case RN6314_GET_SAMPLE_SIZE:
        {
            int ss;
            if((tmp=rn6314_audio_get_sample_size(pdev->index))<0) {
                ret = -EFAULT;
                goto exit;
            }
            ss=(tmp==RN6314_AUDIO_SAMPLE_SIZE_16B)?(16):(8);
            ret = (copy_to_user((void __user *)arg, &ss, sizeof(int))) ? (-EFAULT) : 0;
            break;
        }
        case RN6314_SET_SAMPLE_SIZE:
            if (copy_from_user(&tmp, (void __user *)arg, sizeof(int))) {
                ret = -EFAULT;
                goto exit;
            }
            if (rn6314_audio_set_sample_rate(pdev->index,(tmp==8)?(RN6314_AUDIO_SAMPLE_SIZE_8B):(RN6314_AUDIO_SAMPLE_SIZE_16B))<0) {
                ret=-EFAULT;
                goto exit;
            }
            break;

        case RN6314_GET_VOLUME:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = rn6314_audio_get_volume(pdev->index,ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case RN6314_SET_VOLUME:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }
            if (rn6314_audio_set_volume(pdev->index,ioc_data.ch,(u8)ioc_data.data)<0) {
                ret=-EFAULT;
                goto exit;
            }
            break;

        case RN6314_GET_PLAYBACK_CH:
            if ((tmp=rn6314_audio_get_playback_channel())<0) {
                ret=-EFAULT;
                goto exit;
            }
            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(int))) ? (-EFAULT) : 0;
            break;

        case RN6314_SET_PLAYBACK_CH:
            if (copy_from_user(&tmp, (void __user *)arg, sizeof(int))) {
                ret = -EFAULT;
                goto exit;
            }
            if (rn6314_audio_set_playback_channel(pdev->index,tmp)<0) {
                ret=-EFAULT;
                goto exit;
            }
            break;

        case RN6314_GET_BYPASS_CH:
            if ((tmp=rn6314_audio_get_bypass_channel())<0) {
                ret=-EFAULT;
                goto exit;
            }
            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(int))) ? (-EFAULT) : 0;
            break;

        case RN6314_SET_BYPASS_CH:
            if (copy_from_user(&tmp, (void __user *)arg, sizeof(int))) {
                ret = -EFAULT;
                goto exit;
            }
            if (rn6314_audio_set_bypass_channel(pdev->index,tmp)<0) {
                ret=-EFAULT;
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

static struct file_operations rn6314_miscdev_fops = {
    .owner          = THIS_MODULE,
    .open           = rn6314_miscdev_open,
    .release        = rn6314_miscdev_release,
    .unlocked_ioctl = rn6314_miscdev_ioctl,
};

static int rn6314_miscdev_init(int id)
{
    int ret;

    if(id >= RN6314_DEV_MAX)
        return -1;

    /* clear */
    memset(&rn6314_dev[id].miscdev, 0, sizeof(struct miscdevice));

    /* create device node */
    rn6314_dev[id].miscdev.name  = rn6314_dev[id].name;
    rn6314_dev[id].miscdev.minor = MISC_DYNAMIC_MINOR;
    rn6314_dev[id].miscdev.fops  = (struct file_operations *)&rn6314_miscdev_fops;
    ret = misc_register(&rn6314_dev[id].miscdev);
    if(ret) {
        printk("create %s misc device node failed!\n", rn6314_dev[id].name);
        rn6314_dev[id].miscdev.name = 0;
        goto exit;
    }

exit:
    return ret;
}

static void rn6314_hw_reset(void)
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

static int rn6314_device_init(int id)
{
    int ret = 0;

    if (!init)
        return 0;

    if(id >= RN6314_DEV_MAX)
        return -1;

    /*====================== video init ========================= */
    ret = rn6314_video_set_mode(id, vmode[id]);
    if(ret < 0)
        goto exit;

    /*====================== audio init ========================= */
    ret = rn6314_audio_set_mode(id, sample_size, sample_rate);
    if(ret < 0)
        goto exit;

exit:
    return ret;
}

static int rn6314_watchdog_thread(void *data)
{
    int i, vin, novid, vmode;
    struct rn6314_dev_t *pdev;

    do {
            /* check rn6314 video status */
            for(i=0; i<dev_num; i++) {
                pdev = &rn6314_dev[i];

                down(&pdev->lock);

                /* video signal check */
                if(pdev->vlos_notify) {
                    for(vin=0; vin<RN6314_DEV_CH_MAX; vin++) {
                        novid = rn6314_status_get_novid(pdev->index, vin);
                        if(novid)
                            pdev->vlos_notify(i, vin, 1);
                        else
                            pdev->vlos_notify(i, vin, 0);
                    }
                }

                /* video format check */
                if(pdev->vfmt_notify) {
                    vmode = rn6314_video_get_mode(pdev->index);
                    if((vmode >= 0) && (vmode < RN6314_VMODE_MAX))
                        pdev->vfmt_notify(i, vmode);
                }

                up(&pdev->lock);
            }

            /* sleep 1 second */
            schedule_timeout_interruptible(msecs_to_jiffies(1000));

    } while (!kthread_should_stop());

    return 0;
}

static int rn6314_sound_switch(int ssp_idx, int chan_num, bool is_on)
{
    printk("%s(%d) ssp_idx:%d channel:%d action:%s\n",__FUNCTION__,__LINE__,
            ssp_idx,chan_num,(is_on)?("on"):("off"));

    goto out; // The sound switch function has been deprecated by audio_drv

    if (chan_num>=RN6314_AUDIO_CH_MAX)
        return -1;

    rn6314_audio_set_bypass_channel(0,chan_num);

out:
    return 0;
}

static int __init rn6314_init(void)
{
    int i, ret;

    /* platfrom check */
    plat_identifier_check();

    /* check device number */
    if(dev_num > RN6314_DEV_MAX) {
        printk("RN6314 dev_num=%d invalid!!(Max=%d)\n", dev_num, RN6314_DEV_MAX);
        return -1;
    }

    /* register i2c client for control rn6314 */
    ret = rn6314_i2c_register();
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
    ret = rn6314_create_ch_map(ch_map);
    if(ret < 0)
        goto err;

    /* hardware reset for all device */
    rn6314_hw_reset();

    /* RN6314 init */
    for(i=0; i<dev_num; i++) {
        rn6314_dev[i].index = i;

        sprintf(rn6314_dev[i].name, "rn6314.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&rn6314_dev[i].lock, 1);
#else
        init_MUTEX(&rn6314_dev[i].lock);
#endif
        ret = rn6314_proc_init(i);
        if(ret < 0)
            goto err;

        ret = rn6314_miscdev_init(i);
        if(ret < 0)
            goto err;

        ret = rn6314_device_init(i);
        if(ret < 0)
            goto err;
    }

    if(init && notify) {
        /* init rn6314 watchdog thread for monitor video status */
        rn6314_wd = kthread_create(rn6314_watchdog_thread, NULL, "rn6314_wd");
        if(!IS_ERR(rn6314_wd))
            wake_up_process(rn6314_wd);
        else {
            printk("create rn6314 watchdog thread failed!!\n");
            rn6314_wd = 0;
            ret = -EFAULT;
            goto err;
        }
    }

    /* register audio function for platform used */
    memset(&rn6314_audio_funs,0,sizeof(audio_funs_t));
    rn6314_audio_funs.sound_switch = rn6314_sound_switch;
    rn6314_audio_funs.codec_name   = rn6314_dev[0].name;
    plat_audio_register_function(&rn6314_audio_funs);

    return 0;

err:
    if(rn6314_wd)
        kthread_stop(rn6314_wd);

    rn6314_i2c_unregister();
    for(i=0; i<RN6314_DEV_MAX; i++) {
        if(rn6314_dev[i].miscdev.name)
            misc_deregister(&rn6314_dev[i].miscdev);

        rn6314_proc_remove(i);
    }

    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    return ret;
}

static void __exit rn6314_exit(void)
{
    int i;

    /* stop rn6314 watchdog thread */
    if(rn6314_wd)
        kthread_stop(rn6314_wd);

    rn6314_i2c_unregister();
    for(i=0; i<RN6314_DEV_MAX; i++) {
        /* remove device node */
        if(rn6314_dev[i].miscdev.name)
            misc_deregister(&rn6314_dev[i].miscdev);

        /* remove proc node */
        rn6314_proc_remove(i);
    }

    /* release gpio pin */
    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);
}

int rn6314_audio_get_chip_num(void)
{
    return dev_num;
}

int rn6314_audio_get_channel_number(void)
{
    return audio_chnum;
}

module_init(rn6314_init);
module_exit(rn6314_exit);

MODULE_DESCRIPTION("Grain Media RN6314 Video Decoders and Audio Codecs Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
