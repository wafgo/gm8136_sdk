/**
 * @file tw9900_drv.c
 * Intersil TW9900 1-CH D1 Video Decoders and Audio Codecs Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.6 $
 * $Date: 2014/08/26 08:14:59 $
 *
 * ChangeLog:
 *  $Log: tw9900_drv.c,v $
 *  Revision 1.6  2014/08/26 08:14:59  jerson_l
 *  1. correct notify api naming
 *
 *  Revision 1.5  2014/07/29 05:36:11  shawn_hu
 *  Add the module parameter of "ibus" to specify which I2C bus has the decoder attached.
 *
 *  Revision 1.4  2014/02/24 07:10:47  jerson_l
 *  1. enable notify on default
 *
 *  Revision 1.3  2013/11/27 05:32:59  jerson_l
 *  1. support notify procedure
 *
 *  Revision 1.2  2013/10/31 05:19:13  jerson_l
 *  1. add GPIO#59 as RSTB PIN support
 *
 *  Revision 1.1  2013/10/24 02:48:48  jerson_l
 *  1. support tw9900 decoder
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
#include "intersil/tw9900.h"        ///< from module/include/front_end/decoder

#define TW9900_CLKIN                27000000
#define CH_IDENTIFY(id, vin, vp)    (((id)&0xf)|(((vin)&0xf)<<4)|(((vp)&0xf)<<8))
#define CH_IDENTIFY_ID(x)           ((x)&0xf)
#define CH_IDENTIFY_VIN(x)          (((x)>>4)&0xf)
#define CH_IDENTIFY_VPORT(x)        (((x)>>8)&0xf)

/* device number */
static int dev_num = 1;
module_param(dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dev_num, "Device Number: 1~4");

/* device i2c bus */
static int ibus = 0;
module_param(ibus, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ibus, "Device I2C Bus: 0~4");

/* device i2c address */
static ushort iaddr[TW9900_DEV_MAX] = {0x88, 0x8a, 0x8c, 0x8e};
module_param_array(iaddr, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iaddr, "Device I2C Address");

/* device video mode */
static int vmode[TW9900_DEV_MAX] = {[0 ... (TW9900_DEV_MAX - 1)] = TW9900_VMODE_NTSC_720H_1CH};
module_param_array(vmode, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vmode, "Video Mode");

/* clock port used */
static int clk_used = 0x1;
module_param(clk_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_used, "External Clock Port: BIT0:EXT_CLK0, BIT1:EXT_CLK1");

/* clock source */
static int clk_src = PLAT_EXTCLK_SRC_PLL3;
module_param(clk_src, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_src, "External Clock Source: 0:PLL1OUT1 1:PLL1OUT1/2 2:PLL4OUT2 3:PLL4OUT1/2 4:PLL3");

/* clock frequency */
static int clk_freq = TW9900_CLKIN;
module_param(clk_freq, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_freq, "External Clock Frequency(Hz)");

/* clock SSCG */
static int clk_sscg = PLAT_EXTCLK_SSCG_DISABLE;
module_param(clk_sscg, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_sscg, "External Clock SSCG: 0:disable 1:MR0 2:MR1 3:MR2 4:MR3");

/* device use gpio as rstb pin */
static int rstb_used = PLAT_GPIO_ID_X_CAP_RST;
module_param(rstb_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rstb_used, "Use GPIO as RSTB Pin: 0:None, 1:X_CAP_RST, 2:GPIO_59");

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

struct tw9900_dev_t {
    int                 index;      ///< device index
    char                name[16];   ///< device name
    struct semaphore    lock;       ///< device locker
    struct miscdevice   miscdev;    ///< device node for ioctl access

    int                 (*vlos_notify)(int id, int vin, int vlos);
};

static struct tw9900_dev_t   tw9900_dev[TW9900_DEV_MAX];
static struct proc_dir_entry *tw9900_proc_root[TW9900_DEV_MAX]   = {[0 ... (TW9900_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw9900_proc_vmode[TW9900_DEV_MAX]  = {[0 ... (TW9900_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw9900_proc_status[TW9900_DEV_MAX] = {[0 ... (TW9900_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw9900_proc_bri[TW9900_DEV_MAX]    = {[0 ... (TW9900_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw9900_proc_con[TW9900_DEV_MAX]    = {[0 ... (TW9900_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw9900_proc_sat_u[TW9900_DEV_MAX]  = {[0 ... (TW9900_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw9900_proc_sat_v[TW9900_DEV_MAX]  = {[0 ... (TW9900_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw9900_proc_hue[TW9900_DEV_MAX]    = {[0 ... (TW9900_DEV_MAX - 1)] = NULL};

static struct i2c_client  *tw9900_i2c_client = NULL;
static struct task_struct *tw9900_wd         = NULL;
static u32 TW9900_VCH_MAP[TW9900_DEV_MAX*TW9900_DEV_CH_MAX];

static int tw9900_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    tw9900_i2c_client = client;
    return 0;
}

static int tw9900_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id tw9900_i2c_id[] = {
    { "tw9900_i2c", 0 },
    { }
};

static struct i2c_driver tw9900_i2c_driver = {
    .driver = {
        .name  = "TW9900_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = tw9900_i2c_probe,
    .remove   = tw9900_i2c_remove,
    .id_table = tw9900_i2c_id
};

static int tw9900_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&tw9900_i2c_driver);
    if(ret < 0) {
        printk("TW9900 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = iaddr[0]>>1;
    strlcpy(info.type, "tw9900_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("TW9900 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("TW9900 can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&tw9900_i2c_driver);
   return -ENODEV;
}

static void tw9900_i2c_unregister(void)
{
    i2c_unregister_device(tw9900_i2c_client);
    i2c_del_driver(&tw9900_i2c_driver);
    tw9900_i2c_client = NULL;
}

static int tw9900_create_ch_map(int map_id)
{
    int i;

    switch(map_id) {
        case 0:     ///< General, 1CH Mode
        default:
            for(i=0; i<dev_num; i++) {
                TW9900_VCH_MAP[i] = CH_IDENTIFY(i, 0, TW9900_DEV_VPORT_VD1);
            }
            break;
    }

    return 0;
}

int tw9900_get_vch_id(int id, TW9900_DEV_VPORT_T vport)
{
    int i;

    if(id >= TW9900_DEV_MAX)
        return 0;

    for(i=0; i<(dev_num*TW9900_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(TW9900_VCH_MAP[i]) == id) && (CH_IDENTIFY_VPORT(TW9900_VCH_MAP[i]) == vport)) {
            return i;
        }
    }

    return 0;
}
EXPORT_SYMBOL(tw9900_get_vch_id);

int tw9900_vin_to_vch(int id, int vin_idx)
{
    int i;

    if(id >= TW9900_DEV_MAX || vin_idx >= TW9900_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*TW9900_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(TW9900_VCH_MAP[i]) == id) && (CH_IDENTIFY_VIN(TW9900_VCH_MAP[i]) == vin_idx))
            return i;
    }

    return 0;
}
EXPORT_SYMBOL(tw9900_vin_to_vch);

int tw9900_notify_vlos_register(int id, int (*nt_func)(int, int, int))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&tw9900_dev[id].lock);

    tw9900_dev[id].vlos_notify = nt_func;

    up(&tw9900_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(tw9900_notify_vlos_register);

void tw9900_notify_vlos_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&tw9900_dev[id].lock);

    tw9900_dev[id].vlos_notify = NULL;

    up(&tw9900_dev[id].lock);
}
EXPORT_SYMBOL(tw9900_notify_vlos_deregister);

int tw9900_i2c_write(u8 id, u8 reg, u8 data)
{
    u8 buf[2];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return -1;

    if(!tw9900_i2c_client) {
        printk("TW9900 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(tw9900_i2c_client->dev.parent);

    buf[0]     = reg;
    buf[1]     = data;
    msgs.addr  = iaddr[id]>>1;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("TW9900#%d i2c write failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(tw9900_i2c_write);

u8 tw9900_i2c_read(u8 id, u8 reg)
{
    u8 reg_data = 0;
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return 0;

    if(!tw9900_i2c_client) {
        printk("TW9900 i2c_client not register!!\n");
        return 0;
    }

    adapter = to_i2c_adapter(tw9900_i2c_client->dev.parent);

    msgs[0].addr  = iaddr[id]>>1;
    msgs[0].flags = 0; /* write */
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    msgs[1].addr  = (iaddr[id] + 1)>>1;
    msgs[1].flags = 1; /* read */
    msgs[1].len   = 1;
    msgs[1].buf   = &reg_data;

    if(i2c_transfer(adapter, msgs, 2) != 2)
        printk("TW9900#%d i2c read failed!!\n", id);

    return reg_data;
}
EXPORT_SYMBOL(tw9900_i2c_read);

int tw9900_get_device_num(void)
{
    return dev_num;
}
EXPORT_SYMBOL(tw9900_get_device_num);

static int tw9900_proc_vmode_show(struct seq_file *sfile, void *v)
{
    int i, vmode;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;
    char *vmode_str[] = {"NTSC_720H_1CH", "PAL_720H_1CH"};

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW9900#%d]\n", pdev->index);
    for(i=0; i<TW9900_VMODE_MAX; i++) {
        if(tw9900_video_mode_support_check(pdev->index, i))
            seq_printf(sfile, "%02d: %s\n", i, vmode_str[i]);
    }
    seq_printf(sfile, "----------------------------------\n");

    vmode = tw9900_video_get_mode(pdev->index);
    seq_printf(sfile, "Current==> %s\n\n",(vmode >= 0 && vmode < TW9900_VMODE_MAX) ? vmode_str[vmode] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static int tw9900_proc_status_show(struct seq_file *sfile, void *v)
{
    int i, j, novid;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW9900#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    NOVID     \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<TW9900_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW9900_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TW9900_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TW9900_VCH_MAP[j]) == i)) {
                novid = tw9900_status_get_novid(pdev->index);
                seq_printf(sfile, "%-7d %-7d %s\n", i, j, (novid != 0) ? "Video_Loss" : "Video_On");
            }
        }
    }
    seq_printf(sfile, "\n");

    up(&pdev->lock);

    return 0;
}

static int tw9900_proc_bri_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW9900#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    BRIGHTNESS\n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<TW9900_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW9900_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TW9900_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TW9900_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw9900_video_get_brightness(pdev->index));
            }
        }
    }
    seq_printf(sfile, "\nBrightness[-128 ~ +127] ==> 0x01=+1, 0x7f=+127, 0x80=-128, 0xff=-1\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw9900_proc_con_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW9900#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    CONTRAST  \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<TW9900_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW9900_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TW9900_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TW9900_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw9900_video_get_contrast(pdev->index));
            }
        }
    }
    seq_printf(sfile, "\nContrast[0%% ~ 255%%] ==> 0x00=0%%, 0xff=255%%\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw9900_proc_sat_u_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW9900#%d]\n", pdev->index);
    seq_printf(sfile, "----------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    SATURATION_U\n");
    seq_printf(sfile, "----------------------------\n");
    for(i=0; i<TW9900_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW9900_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TW9900_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TW9900_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw9900_video_get_saturation_u(pdev->index));
            }
        }
    }
    seq_printf(sfile, "\nSaturation_U[0%% ~ 200%%] ==> 0x80=100%%\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw9900_proc_sat_v_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW9900#%d]\n", pdev->index);
    seq_printf(sfile, "----------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    SATURATION_V\n");
    seq_printf(sfile, "----------------------------\n");
    for(i=0; i<TW9900_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW9900_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TW9900_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TW9900_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw9900_video_get_saturation_v(pdev->index));
            }
        }
    }
    seq_printf(sfile, "\nSaturation_V[0%% ~ 200%%] ==> 0x80=100%%\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw9900_proc_hue_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW9900#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    HUE       \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<TW9900_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW9900_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TW9900_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TW9900_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw9900_video_get_hue(pdev->index));
            }
        }
    }
    seq_printf(sfile, "\nHue[0x00 ~ 0xff] ==> 0x00=0, 0x7f=90, 0x80=-90\n\n");

    up(&pdev->lock);

    return 0;
}

static ssize_t tw9900_proc_bri_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32  bri;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%x\n", &bri);

    down(&pdev->lock);

    tw9900_video_set_brightness(pdev->index, (u8)bri);

    up(&pdev->lock);

    return count;
}

static ssize_t tw9900_proc_con_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 con;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%x\n", &con);

    down(&pdev->lock);

    tw9900_video_set_contrast(pdev->index, (u8)con);

    up(&pdev->lock);

    return count;
}

static ssize_t tw9900_proc_sat_u_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 sat;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%x\n", &sat);

    down(&pdev->lock);

    tw9900_video_set_saturation_u(pdev->index, (u8)sat);

    up(&pdev->lock);

    return count;
}

static ssize_t tw9900_proc_sat_v_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 sat;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%x\n", &sat);

    down(&pdev->lock);

    tw9900_video_set_saturation_v(pdev->index, (u8)sat);

    up(&pdev->lock);

    return count;
}

static ssize_t tw9900_proc_hue_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 hue;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%x\n", &hue);

    down(&pdev->lock);

    tw9900_video_set_hue(pdev->index, (u8)hue);

    up(&pdev->lock);

    return count;
}

static int tw9900_proc_vmode_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw9900_proc_vmode_show, PDE(inode)->data);
}

static int tw9900_proc_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw9900_proc_status_show, PDE(inode)->data);
}

static int tw9900_proc_bri_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw9900_proc_bri_show, PDE(inode)->data);
}

static int tw9900_proc_con_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw9900_proc_con_show, PDE(inode)->data);
}

static int tw9900_proc_sat_u_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw9900_proc_sat_u_show, PDE(inode)->data);
}

static int tw9900_proc_sat_v_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw9900_proc_sat_v_show, PDE(inode)->data);
}

static int tw9900_proc_hue_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw9900_proc_hue_show, PDE(inode)->data);
}

static struct file_operations tw9900_proc_vmode_ops = {
    .owner  = THIS_MODULE,
    .open   = tw9900_proc_vmode_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw9900_proc_status_ops = {
    .owner  = THIS_MODULE,
    .open   = tw9900_proc_status_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw9900_proc_bri_ops = {
    .owner  = THIS_MODULE,
    .open   = tw9900_proc_bri_open,
    .write  = tw9900_proc_bri_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw9900_proc_con_ops = {
    .owner  = THIS_MODULE,
    .open   = tw9900_proc_con_open,
    .write  = tw9900_proc_con_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw9900_proc_sat_u_ops = {
    .owner  = THIS_MODULE,
    .open   = tw9900_proc_sat_u_open,
    .write  = tw9900_proc_sat_u_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw9900_proc_sat_v_ops = {
    .owner  = THIS_MODULE,
    .open   = tw9900_proc_sat_v_open,
    .write  = tw9900_proc_sat_v_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw9900_proc_hue_ops = {
    .owner  = THIS_MODULE,
    .open   = tw9900_proc_hue_open,
    .write  = tw9900_proc_hue_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void tw9900_proc_remove(int id)
{
    if(id >= TW9900_DEV_MAX)
        return;

    if(tw9900_proc_root[id]) {
        if(tw9900_proc_vmode[id]) {
            remove_proc_entry(tw9900_proc_vmode[id]->name, tw9900_proc_root[id]);
            tw9900_proc_vmode[id] = NULL;
        }

        if(tw9900_proc_status[id]) {
            remove_proc_entry(tw9900_proc_status[id]->name, tw9900_proc_root[id]);
            tw9900_proc_status[id] = NULL;
        }

        if(tw9900_proc_bri[id]) {
            remove_proc_entry(tw9900_proc_bri[id]->name, tw9900_proc_root[id]);
            tw9900_proc_bri[id] = NULL;
        }

        if(tw9900_proc_con[id]) {
            remove_proc_entry(tw9900_proc_con[id]->name, tw9900_proc_root[id]);
            tw9900_proc_con[id] = NULL;
        }

        if(tw9900_proc_sat_u[id]) {
            remove_proc_entry(tw9900_proc_sat_u[id]->name, tw9900_proc_root[id]);
            tw9900_proc_sat_u[id] = NULL;
        }

        if(tw9900_proc_sat_v[id]) {
            remove_proc_entry(tw9900_proc_sat_v[id]->name, tw9900_proc_root[id]);
            tw9900_proc_sat_v[id] = NULL;
        }

        if(tw9900_proc_hue[id]) {
            remove_proc_entry(tw9900_proc_hue[id]->name, tw9900_proc_root[id]);
            tw9900_proc_hue[id] = NULL;
        }

        remove_proc_entry(tw9900_proc_root[id]->name, NULL);
        tw9900_proc_root[id] = NULL;
    }
}

static int tw9900_proc_init(int id)
{
    int ret = 0;

    if(id >= TW9900_DEV_MAX)
        return -1;

    /* root */
    tw9900_proc_root[id] = create_proc_entry(tw9900_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!tw9900_proc_root[id]) {
        printk("create proc node '%s' failed!\n", tw9900_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw9900_proc_root[id]->owner = THIS_MODULE;
#endif

    /* vmode */
    tw9900_proc_vmode[id] = create_proc_entry("vmode", S_IRUGO|S_IXUGO, tw9900_proc_root[id]);
    if(!tw9900_proc_vmode[id]) {
        printk("create proc node '%s/vmode' failed!\n", tw9900_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw9900_proc_vmode[id]->proc_fops = &tw9900_proc_vmode_ops;
    tw9900_proc_vmode[id]->data      = (void *)&tw9900_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw9900_proc_vmode[id]->owner     = THIS_MODULE;
#endif

    /* status */
    tw9900_proc_status[id] = create_proc_entry("status", S_IRUGO|S_IXUGO, tw9900_proc_root[id]);
    if(!tw9900_proc_status[id]) {
        printk("create proc node '%s/status' failed!\n", tw9900_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw9900_proc_status[id]->proc_fops = &tw9900_proc_status_ops;
    tw9900_proc_status[id]->data      = (void *)&tw9900_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw9900_proc_status[id]->owner     = THIS_MODULE;
#endif

    /* brightness */
    tw9900_proc_bri[id] = create_proc_entry("brightness", S_IRUGO|S_IXUGO, tw9900_proc_root[id]);
    if(!tw9900_proc_bri[id]) {
        printk("create proc node '%s/brightness' failed!\n", tw9900_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw9900_proc_bri[id]->proc_fops = &tw9900_proc_bri_ops;
    tw9900_proc_bri[id]->data      = (void *)&tw9900_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw9900_proc_bri[id]->owner     = THIS_MODULE;
#endif

    /* contrast */
    tw9900_proc_con[id] = create_proc_entry("contrast", S_IRUGO|S_IXUGO, tw9900_proc_root[id]);
    if(!tw9900_proc_con[id]) {
        printk("create proc node '%s/contrast' failed!\n", tw9900_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw9900_proc_con[id]->proc_fops = &tw9900_proc_con_ops;
    tw9900_proc_con[id]->data      = (void *)&tw9900_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw9900_proc_con[id]->owner     = THIS_MODULE;
#endif

    /* saturation u */
    tw9900_proc_sat_u[id] = create_proc_entry("saturation_u", S_IRUGO|S_IXUGO, tw9900_proc_root[id]);
    if(!tw9900_proc_sat_u[id]) {
        printk("create proc node '%s/saturation_u' failed!\n", tw9900_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw9900_proc_sat_u[id]->proc_fops = &tw9900_proc_sat_u_ops;
    tw9900_proc_sat_u[id]->data      = (void *)&tw9900_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw9900_proc_sat_u[id]->owner     = THIS_MODULE;
#endif

    /* saturation v */
    tw9900_proc_sat_v[id] = create_proc_entry("saturation_v", S_IRUGO|S_IXUGO, tw9900_proc_root[id]);
    if(!tw9900_proc_sat_v[id]) {
        printk("create proc node '%s/saturation_v' failed!\n", tw9900_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw9900_proc_sat_v[id]->proc_fops = &tw9900_proc_sat_v_ops;
    tw9900_proc_sat_v[id]->data      = (void *)&tw9900_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw9900_proc_sat_v[id]->owner     = THIS_MODULE;
#endif

    /* hue */
    tw9900_proc_hue[id] = create_proc_entry("hue", S_IRUGO|S_IXUGO, tw9900_proc_root[id]);
    if(!tw9900_proc_hue[id]) {
        printk("create proc node '%s/hue' failed!\n", tw9900_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw9900_proc_hue[id]->proc_fops = &tw9900_proc_hue_ops;
    tw9900_proc_hue[id]->data      = (void *)&tw9900_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw9900_proc_hue[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    tw9900_proc_remove(id);
    return ret;
}

static int tw9900_miscdev_open(struct inode *inode, struct file *file)
{
    int i, ret = 0;
    struct tw9900_dev_t *pdev = NULL;

    /* lookup device */
    for(i=0; i<dev_num; i++) {
        if(tw9900_dev[i].miscdev.minor == iminor(inode)) {
            pdev = &tw9900_dev[i];
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

static int tw9900_miscdev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long tw9900_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int tmp, ret = 0;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)file->private_data;

    down(&pdev->lock);

    if(_IOC_TYPE(cmd) != TW9900_IOC_MAGIC) {
        ret = -ENOIOCTLCMD;
        goto exit;
    }

    switch(cmd) {
        case TW9900_GET_NOVID:
            tmp = tw9900_status_get_novid(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }
            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case TW9900_GET_MODE:
            tmp = tw9900_video_get_mode(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }
            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case TW9900_GET_CONTRAST:
            tmp = tw9900_video_get_contrast(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }
            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case TW9900_SET_CONTRAST:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw9900_video_set_contrast(pdev->index, (u8)tmp);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW9900_GET_BRIGHTNESS:
            tmp = tw9900_video_get_brightness(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }
            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case TW9900_SET_BRIGHTNESS:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw9900_video_set_brightness(pdev->index, (u8)tmp);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW9900_GET_SATURATION_U:
            tmp = tw9900_video_get_saturation_u(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case TW9900_SET_SATURATION_U:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw9900_video_set_saturation_u(pdev->index, (u8)tmp);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW9900_GET_SATURATION_V:
            tmp = tw9900_video_get_saturation_v(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case TW9900_SET_SATURATION_V:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw9900_video_set_saturation_v(pdev->index, (u8)tmp);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW9900_GET_HUE:
            tmp = tw9900_video_get_hue(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case TW9900_SET_HUE:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw9900_video_set_hue(pdev->index, (u8)tmp);
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

static struct file_operations tw9900_miscdev_fops = {
    .owner          = THIS_MODULE,
    .open           = tw9900_miscdev_open,
    .release        = tw9900_miscdev_release,
    .unlocked_ioctl = tw9900_miscdev_ioctl,
};

static int tw9900_miscdev_init(int id)
{
    int ret;

    if(id >= TW9900_DEV_MAX)
        return -1;

    /* clear */
    memset(&tw9900_dev[id].miscdev, 0, sizeof(struct miscdevice));

    /* create device node */
    tw9900_dev[id].miscdev.name  = tw9900_dev[id].name;
    tw9900_dev[id].miscdev.minor = MISC_DYNAMIC_MINOR;
    tw9900_dev[id].miscdev.fops  = (struct file_operations *)&tw9900_miscdev_fops;
    ret = misc_register(&tw9900_dev[id].miscdev);
    if(ret) {
        printk("create %s misc device node failed!\n", tw9900_dev[id].name);
        tw9900_dev[id].miscdev.name = 0;
        goto exit;
    }

exit:
    return ret;
}

static void tw9900_hw_reset(void)
{
    if(!rstb_used || !init)
        return;

    /* Active Low */
    plat_gpio_set_value(rstb_used, 0);

    udelay(100);

    /* Active High and release reset */
    plat_gpio_set_value(rstb_used, 1);
}

static int tw9900_device_init(int id)
{
    int ret = 0;

    if(id >= TW9900_DEV_MAX)
        return -1;

    if(!init)
        return 0;

    /*====================== video init ========================= */
    ret = tw9900_video_set_mode(id, vmode[id]);
    if(ret < 0)
        goto exit;

    /*====================== audio init ========================= */

exit:
    return ret;
}

static int tw9900_watchdog_thread(void *data)
{
    int i, novid;
    struct tw9900_dev_t *pdev;

    do {
            /* check tw9900 video status */
            for(i=0; i<dev_num; i++) {
                pdev = &tw9900_dev[i];

                down(&pdev->lock);

                if(pdev->vlos_notify) {
                    novid = tw9900_status_get_novid(pdev->index);
                    if(novid)
                        pdev->vlos_notify(i, 0, 1);
                    else
                        pdev->vlos_notify(i, 0, 0);
                }

                up(&pdev->lock);
            }

            /* sleep 1 second */
            schedule_timeout_interruptible(msecs_to_jiffies(1000));

    } while (!kthread_should_stop());

    return 0;
}

static int __init tw9900_init(void)
{
    int i, ret;

    /* platfrom check */
    plat_identifier_check();

    /* check device number */
    if(dev_num > TW9900_DEV_MAX) {
        printk("TW9900 dev_num=%d invalid!!(Max=%d)\n", dev_num, TW9900_DEV_MAX);
        return -1;
    }

    /* map rstb pin to gpio number */
    if(rstb_used == 2)
        rstb_used = PLAT_GPIO_ID_59;

    /* register i2c client for contol tw9900 */
    ret = tw9900_i2c_register();
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

            ret = plat_extclk_set_sscg(clk_sscg);
            if(ret < 0)
                goto err;

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
    ret = tw9900_create_ch_map(ch_map);
    if(ret < 0)
        goto err;

    /* hardware reset for all device */
    tw9900_hw_reset();

    /* TW9900 init */
    for(i=0; i<dev_num; i++) {
        tw9900_dev[i].index = i;

        sprintf(tw9900_dev[i].name, "tw9900.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&tw9900_dev[i].lock, 1);
#else
        init_MUTEX(&tw9900_dev[i].lock);
#endif
        ret = tw9900_proc_init(i);
        if(ret < 0)
            goto err;

        ret = tw9900_miscdev_init(i);
        if(ret < 0)
            goto err;

        ret = tw9900_device_init(i);
        if(ret < 0)
            goto err;
    }

    if(init && notify) {
        /* init tw9900 watchdog thread for monitor video status */
        tw9900_wd = kthread_create(tw9900_watchdog_thread, NULL, "tw9900_wd");
        if(!IS_ERR(tw9900_wd))
            wake_up_process(tw9900_wd);
        else {
            printk("create tw9900 watchdog thread failed!!\n");
            tw9900_wd = 0;
            ret = -EFAULT;
            goto err;
        }
    }

    return 0;

err:
    if(tw9900_wd)
        kthread_stop(tw9900_wd);

    tw9900_i2c_unregister();
    for(i=0; i<TW9900_DEV_MAX; i++) {
        if(tw9900_dev[i].miscdev.name)
            misc_deregister(&tw9900_dev[i].miscdev);

        tw9900_proc_remove(i);
    }

    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    return ret;
}

static void __exit tw9900_exit(void)
{
    int i;

    /* stop tw9900 watchdog thread */
    if(tw9900_wd)
        kthread_stop(tw9900_wd);

    tw9900_i2c_unregister();
    for(i=0; i<TW9900_DEV_MAX; i++) {
        /* remove device node */
        if(tw9900_dev[i].miscdev.name)
            misc_deregister(&tw9900_dev[i].miscdev);

        /* remove proc node */
        tw9900_proc_remove(i);
    }

    /* release gpio pin */
    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);
}

module_init(tw9900_init);
module_exit(tw9900_exit);

MODULE_DESCRIPTION("Grain Media TW9900 Video Decoders and Audio Codecs Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
