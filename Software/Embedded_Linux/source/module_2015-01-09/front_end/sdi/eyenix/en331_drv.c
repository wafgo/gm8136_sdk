/**
 * @file en331_drv.c
 * Eyenix EN331 1CH HD-SDI Video Decoder Driver
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.1 $
 * $Date: 2014/10/15 13:53:02 $
 *
 * ChangeLog:
 *  $Log: en331_drv.c,v $
 *  Revision 1.1  2014/10/15 13:53:02  shawn_hu
 *  Add Eyenix EN331 1-CH SDI video decoder support.
 *  1. Support EX-SDI(270Mbps), HD-SDI(1.485Gbps) data rates.
 *  2. Support pattern generation (black image) output when no video input.
 *  3. Support auto-switch between EX-SDI, HD-SDI, and pattern generation output.
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
#include "eyenix/en331.h"                       ///< from module/include/front_end/hdsdi

#define EN331_CLKIN                    27000000     ///< Master source clock of device
#define CH_IDENTIFY(id, vin, vout)     (((id)&0xf)|(((vin)&0xf)<<4)|(((vout)&0xf)<<8))
#define CH_IDENTIFY_ID(x)              ((x)&0xf)
#define CH_IDENTIFY_VIN(x)             (((x)>>4)&0xf)
#define CH_IDENTIFY_VOUT(x)            (((x)>>8)&0xf)

/* device number */
static int dev_num = 1;
module_param(dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dev_num, "Device Number: 1~4");

/* device i2c bus */
static int ibus = 0;
module_param(ibus, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ibus, "Device I2C Bus: 0~4");

/* device i2c address */
static ushort iaddr[EN331_DEV_MAX] = {0x84, 0x8C, 0x94, 0x9C};
module_param_array(iaddr, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iaddr, "Device I2C Address");

/* clock port used */
static int clk_used = 0x3;
module_param(clk_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_used, "External Clock Port: BIT0:EXT_CLK0, BIT1:EXT_CLK1");

/* clock source */
static int clk_src = PLAT_EXTCLK_SRC_PLL1OUT1_DIV2;
module_param(clk_src, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_src, "External Clock Source: 0:PLL1OUT1 1:PLL1OUT1/2 2:PLL4OUT2 3:PLL4OUT1/2 4:PLL3");

/* clock frequency */
static int clk_freq = EN331_CLKIN;   ///< 27MHz
module_param(clk_freq, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_freq, "External Clock Frequency(Hz)");

/* device init */
static int init = 1;
module_param(init, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init, "device init: 0:no-init  1:init");

/* device use gpio as rstb pin */
static int rstb_used = 1;
module_param(rstb_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rstb_used, "Use GPIO as RSTB Pin: 0:None, 1:X_CAP_RST");

static int vout_format[EN331_DEV_MAX] = {[0 ... (EN331_DEV_MAX - 1)] = EN331_VOUT_FORMAT_BT656};
module_param_array(vout_format, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vout_format, "Video Output Format: 0:BT656, 1:BT1120");

/* device channel mapping */
static int ch_map = 0;
module_param(ch_map, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_map, "channel mapping");

/* notify */
static int notify = 1;
module_param(notify, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(notify, "Device Notify => 0:Disable 1:Enable");

/******************************************************************************

GM EVB TBD...

 GM8287 EVB Channel and EN331 Mapping Table

 VCH#0 ------> EN331#0(VIN0) ---------> VOUT#0 -------> X_CAP#0
 VCH#1 ------> EN331#1(VIN0) ---------> VOUT#0 -------> X_CAP#1
 VCH#2 ------> EN331#2(VIN0) ---------> VOUT#0 -------> X_CAP#2
 VCH#3 ------> EN331#3(VIN0) ---------> VOUT#0 -------> X_CAP#3

*******************************************************************************/

struct en331_dev_t {
    int               index;                          ///< device index
    char              name[16];                       ///< device name
    struct semaphore  lock;                           ///< device locker
    struct miscdevice miscdev;                        ///< device node for ioctl access

    int               pre_plugin[EN331_DEV_CH_MAX];  ///< device channel previous cable plugin status
    int               cur_plugin[EN331_DEV_CH_MAX];  ///< device channel current  cable plugin status

    int               (*vfmt_notify)(int id, int ch, struct en331_video_fmt_t *vfmt);
    int               (*vlos_notify)(int id, int ch, int vlos);
};

static struct en331_dev_t    en331_dev[EN331_DEV_MAX];
static struct proc_dir_entry *en331_proc_root[EN331_DEV_MAX]      = {[0 ... (EN331_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *en331_proc_status[EN331_DEV_MAX]    = {[0 ... (EN331_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *en331_proc_video_fmt[EN331_DEV_MAX] = {[0 ... (EN331_DEV_MAX - 1)] = NULL};

static struct i2c_client  *en331_i2c_client = NULL;
static struct task_struct *en331_wd         = NULL;
static u32 EN331_VCH_MAP[EN331_DEV_MAX*EN331_DEV_CH_MAX];

static int en331_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    en331_i2c_client = client;
    return 0;
}

static int en331_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id en331_i2c_id[] = {
    { "en331_i2c", 0 },
    { }
};

static struct i2c_driver en331_i2c_driver = {
    .driver = {
        .name  = "EN331_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = en331_i2c_probe,
    .remove   = en331_i2c_remove,
    .id_table = en331_i2c_id
};

static int en331_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&en331_i2c_driver);
    if(ret < 0) {
        printk("EN331 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = iaddr[0]>>1;
    strlcpy(info.type, "en331_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("EN331 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("EN331 can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&en331_i2c_driver);
   return -ENODEV;
}

static void en331_i2c_unregister(void)
{
    i2c_unregister_device(en331_i2c_client);
    i2c_del_driver(&en331_i2c_driver);
    en331_i2c_client = NULL;
}

static int en331_create_ch_map(int map_id)
{
    int i;

    switch(map_id) {
        case 1: ///< for GM8210/GM8287 Socket Board ... TBD
            for(i=0; i<dev_num; i++) {
                EN331_VCH_MAP[(i*EN331_DEV_CH_MAX)+0] = CH_IDENTIFY(dev_num-i-1, 0, EN331_DEV_VOUT0);
            }
            break;

        case 0: ///< for GM8210/GM8287 System Board ... TBD
        default:
            for(i=0; i<dev_num; i++) {
                EN331_VCH_MAP[(i*EN331_DEV_CH_MAX)+0] = CH_IDENTIFY(i, 0, EN331_DEV_VOUT0);
            }
            break;
    }

    return 0;
}

int en331_get_vch_id(int id, EN331_DEV_VOUT_T vout, int vin_ch)
{
    int i;

    if(id >= dev_num || vout >= EN331_DEV_VOUT_MAX || vin_ch >= EN331_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*EN331_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(EN331_VCH_MAP[i]) == id) &&
           (CH_IDENTIFY_VOUT(EN331_VCH_MAP[i]) == vout) &&
           (CH_IDENTIFY_VIN(EN331_VCH_MAP[i]) == vin_ch)) {
            return i;
        }
    }

    return 0;
}
EXPORT_SYMBOL(en331_get_vch_id);

int en331_get_vout_format(int id)
{
    if(id >= dev_num)
        return EN331_VOUT_FORMAT_BT656;
    else
        return vout_format[id];
}
EXPORT_SYMBOL(en331_get_vout_format);

int en331_get_device_num(void)
{
    return dev_num;
}
EXPORT_SYMBOL(en331_get_device_num);

int en331_notify_vfmt_register(int id, int (*nt_func)(int, int, struct en331_video_fmt_t *))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&en331_dev[id].lock);

    en331_dev[id].vfmt_notify = nt_func;

    up(&en331_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(en331_notify_vfmt_register);

void en331_notify_vfmt_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&en331_dev[id].lock);

    en331_dev[id].vfmt_notify = NULL;

    up(&en331_dev[id].lock);
}
EXPORT_SYMBOL(en331_notify_vfmt_deregister);

int en331_notify_vlos_register(int id, int (*nt_func)(int, int, int))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&en331_dev[id].lock);

    en331_dev[id].vlos_notify = nt_func;

    up(&en331_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(en331_notify_vlos_register);

void en331_notify_vlos_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&en331_dev[id].lock);

    en331_dev[id].vlos_notify = NULL;

    up(&en331_dev[id].lock);
}
EXPORT_SYMBOL(en331_notify_vlos_deregister);

int en331_i2c_write(u8 id, u32 addr, u32 data)
{
    int i;
    u8 buf[2], tmp;
    u32 internal_addr;
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return -1;

    if(!en331_i2c_client) {
        printk("EN331 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(en331_i2c_client->dev.parent);

    internal_addr = addr << 2;   // internal address (x4)
    tmp = internal_addr >> 8;    // page data
    internal_addr &= 0x000000FF; // register address

    buf[0]     = 0x0; // page register
    buf[1]     = tmp;
    msgs.addr  = iaddr[id]>>1;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("EN331#%d i2c write failed!!\n", id);
        return -1;
    }
    for(i=0; i < 4; i++) { // data is a 32-bit (4 * 8-bit)

        tmp = (data & (0x000000FF  << (8*i))) >> (8*i); // register data

        buf[0]     = internal_addr + i; // register address
        buf[1]     = tmp;
        msgs.addr  = iaddr[id]>>1;
        msgs.flags = 0;
        msgs.len   = 2;
        msgs.buf   = buf;

        if(i2c_transfer(adapter, &msgs, 1) != 1) {
            printk("EN331#%d i2c write failed!!\n", id);
            return -1;
        }
    }
    return 0;
}
EXPORT_SYMBOL(en331_i2c_write);

u32 en331_i2c_read(u8 id, u32 addr)
{
    int i;
    u8 reg_data = 0, buf[2];
    u32 internal_addr, data = 0;
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return 0;

    if(!en331_i2c_client) {
        printk("EN331 i2c_client not register!!\n");
        return 0;
    }

    adapter = to_i2c_adapter(en331_i2c_client->dev.parent);

    internal_addr = addr << 2;     // internal address (x4)
    reg_data = internal_addr >> 8; // page data
    internal_addr &= 0x000000FF;   // register address

    buf[0]        = 0x0; // page register
    buf[1]        = reg_data;
    msgs[0].addr  = iaddr[id]>>1;
    msgs[0].flags = 0;
    msgs[0].len   = 2;
    msgs[0].buf   = buf;

    if(i2c_transfer(adapter, &msgs[0], 1) != 1)
        printk("EN331#%d i2c write failed!!\n", id);

    for(i=0; i < 4; i++) {
        
        buf[0] = internal_addr; // register address

        msgs[0].addr  = iaddr[id]>>1;
        msgs[0].flags = 0; /* write */
        msgs[0].len   = 1;
        msgs[0].buf   = buf;

        msgs[1].addr  = (iaddr[id] + 1)>>1;
        msgs[1].flags = 1; /* read */
        msgs[1].len   = 1;
        msgs[1].buf   = &reg_data;

        if(i2c_transfer(adapter, msgs, 2) != 2)
            printk("EN331#%d i2c read failed!!\n", id);

        internal_addr++;
        data |= reg_data << (8*i);
    }
    return data;
}
EXPORT_SYMBOL(en331_i2c_read);

int en331_i2c_write_table(u8 id, u32 *ptable, u32 size)
{
    int i, j;
    u8 buf[2], data;
    u32 internal_addr;
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if(id >= dev_num || !ptable || (size < 2) || (size%2 != 0))
        return -1;

    if(!en331_i2c_client) {
        printk("EN331 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(en331_i2c_client->dev.parent);

    for(i=0; i<size; i+=2) { // iterate the table: (addr, data) pair

        internal_addr = ptable[i] << 2; // internal address (x4)
        data = internal_addr >> 8;      // page data
        internal_addr &= 0x000000FF;    // register address

        buf[0]     = 0x0; // page register
        buf[1]     = data;
        msgs.addr  = iaddr[id]>>1;
        msgs.flags = 0;
        msgs.len   = 2;
        msgs.buf   = buf;
        
        if(i2c_transfer(adapter, &msgs, 1) != 1) {
            printk("EN331#%d i2c write table failed!!\n", id);
            return -1;
        }

        for(j=0; j < 4; j++) { // ptable[i+1] is a 32-bit (4 * 8-bit) data

            data = (ptable[i+1] & (0x000000FF  << (8*j))) >> (8*j); // register data

            buf[0]     = internal_addr + j; // register address
            buf[1]     = data;
            msgs.addr  = iaddr[id]>>1;
            msgs.flags = 0;
            msgs.len   = 2;
            msgs.buf   = buf;

            if(i2c_transfer(adapter, &msgs, 1) != 1) {
                printk("EN331#%d i2c write table failed!!\n", id);
                return -1;
            }
        }
    }
    return 0;
}
EXPORT_SYMBOL(en331_i2c_write_table);

static int en331_proc_status_show(struct seq_file *sfile, void *v)
{
    int i, j;
    int vlos;
    struct en331_dev_t *pdev = (struct en331_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[EN331#%d]\n", pdev->index);
    seq_printf(sfile, "------------------\n");
    seq_printf(sfile, "VIN    VCH    LOS \n");
    seq_printf(sfile, "==================\n");
    for(i=0; i<EN331_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*EN331_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(EN331_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(EN331_VCH_MAP[j]) == i)) {
                vlos = en331_status_get_video_loss(pdev->index, i);
                seq_printf(sfile, "%-6d %-6d %-7s\n", i, j, ((vlos == 0) ? "NO" : "YES"));
            }
        }
    }

    up(&pdev->lock);

    return 0;
}

static int en331_proc_video_fmt_show(struct seq_file *sfile, void *v)
{
    int i, j, ret;
    struct en331_video_fmt_t vfmt;
    struct en331_dev_t *pdev = (struct en331_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[EN331#%d] Incoming Video Fromat\n", pdev->index);
    seq_printf(sfile, "====================================================\n");
    seq_printf(sfile, "VIN   VCH   Width   Height   Prog/Inter   Frame_Rate\n");
    seq_printf(sfile, "====================================================\n");

    for(i=0; i<EN331_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*EN331_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(EN331_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(EN331_VCH_MAP[j]) == i)) {
                ret = en331_status_get_video_input_format(pdev->index, i, &vfmt);
                if((ret == 0) && ((vfmt.fmt_idx >= 0) && (vfmt.fmt_idx < EN331_VFMT_MAX))) {
                    seq_printf(sfile, "%-5d %-5d %-7d %-8d %-12s %-11d\n",
                               i,
                               j,
                               vfmt.width,
                               vfmt.height,
                               ((vfmt.prog == 1) ? "Progressive" : "Interlace"),
                               vfmt.frame_rate);
                }
                else {
                    seq_printf(sfile, "%-5d %-5d %-7s %-8s %-12s %-11s\n", i, j, "-", "-", "-", "-");
                }
            }
        }
    }

    up(&pdev->lock);

    return 0;
}

static int en331_proc_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, en331_proc_status_show, PDE(inode)->data);
}

static int en331_proc_video_fmt_open(struct inode *inode, struct file *file)
{
    return single_open(file, en331_proc_video_fmt_show, PDE(inode)->data);
}

static struct file_operations en331_proc_status_ops = {
    .owner  = THIS_MODULE,
    .open   = en331_proc_status_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations en331_proc_video_fmt_ops = {
    .owner  = THIS_MODULE,
    .open   = en331_proc_video_fmt_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void en331_proc_remove(int id)
{
    if(id >= EN331_DEV_MAX)
        return;

    if(en331_proc_root[id]) {
        if(en331_proc_status[id]) {
            remove_proc_entry(en331_proc_status[id]->name, en331_proc_root[id]);
            en331_proc_status[id] = NULL;
        }

        if(en331_proc_video_fmt[id]) {
            remove_proc_entry(en331_proc_video_fmt[id]->name, en331_proc_root[id]);
            en331_proc_video_fmt[id] = NULL;
        }

        remove_proc_entry(en331_proc_root[id]->name, NULL);
        en331_proc_root[id] = NULL;
    }
}

static int en331_proc_init(int id)
{
    int ret = 0;

    if(id >= EN331_DEV_MAX)
        return -1;

    /* root */
    en331_proc_root[id] = create_proc_entry(en331_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!en331_proc_root[id]) {
        printk("create proc node '%s' failed!\n", en331_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    en331_proc_root[id]->owner = THIS_MODULE;
#endif

    /* status */
    en331_proc_status[id] = create_proc_entry("status", S_IRUGO|S_IXUGO, en331_proc_root[id]);
    if(!en331_proc_status[id]) {
        printk("create proc node '%s/status' failed!\n", en331_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    en331_proc_status[id]->proc_fops = &en331_proc_status_ops;
    en331_proc_status[id]->data      = (void *)&en331_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    en331_proc_status[id]->owner     = THIS_MODULE;
#endif

    /* frame format */
    en331_proc_video_fmt[id] = create_proc_entry("video_format", S_IRUGO|S_IXUGO, en331_proc_root[id]);
    if(!en331_proc_video_fmt[id]) {
        printk("create proc node '%s/video_format' failed!\n", en331_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    en331_proc_video_fmt[id]->proc_fops = &en331_proc_video_fmt_ops;
    en331_proc_video_fmt[id]->data      = (void *)&en331_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    en331_proc_video_fmt[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    en331_proc_remove(id);
    return ret;
}

static int en331_miscdev_open(struct inode *inode, struct file *file)
{
    int i, ret = 0;
    struct en331_dev_t *pdev = NULL;

    /* lookup device */
    for(i=0; i<dev_num; i++) {
        if(en331_dev[i].miscdev.minor == iminor(inode)) {
            pdev = &en331_dev[i];
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

static int en331_miscdev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long en331_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct en331_dev_t *pdev = (struct en331_dev_t *)file->private_data;

    down(&pdev->lock);

    if(_IOC_TYPE(cmd) != EN331_IOC_MAGIC) {
        ret = -ENOIOCTLCMD;
        goto exit;
    }

    switch(cmd) {
        case EN331_GET_NOVID:
            {
                struct en331_ioc_data_t ioc_data;

                if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_data.vin_ch >= EN331_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                ioc_data.data = en331_status_get_video_loss(pdev->index, ioc_data.vin_ch);

                ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            }
            break;

        case EN331_GET_VIDEO_FMT:
            {
                struct en331_ioc_vfmt_t ioc_vfmt;
                struct en331_video_fmt_t vfmt;

                if(copy_from_user(&ioc_vfmt, (void __user *)arg, sizeof(ioc_vfmt))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vfmt.vin_ch >= EN331_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                ret = en331_status_get_video_input_format(pdev->index, ioc_vfmt.vin_ch, &vfmt);
                if(ret < 0) {
                    ret = -EFAULT;
                    goto exit;
                }

                ioc_vfmt.width      = vfmt.width;
                ioc_vfmt.height     = vfmt.height;
                ioc_vfmt.prog       = vfmt.prog;
                ioc_vfmt.frame_rate = vfmt.frame_rate;

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

static struct file_operations en331_miscdev_fops = {
    .owner          = THIS_MODULE,
    .open           = en331_miscdev_open,
    .release        = en331_miscdev_release,
    .unlocked_ioctl = en331_miscdev_ioctl,
};

static int en331_miscdev_init(int id)
{
    int ret;

    if(id >= EN331_DEV_MAX)
        return -1;

    /* clear */
    memset(&en331_dev[id].miscdev, 0, sizeof(struct miscdevice));

    /* create device node */
    en331_dev[id].miscdev.name  = en331_dev[id].name;
    en331_dev[id].miscdev.minor = MISC_DYNAMIC_MINOR;
    en331_dev[id].miscdev.fops  = (struct file_operations *)&en331_miscdev_fops;
    ret = misc_register(&en331_dev[id].miscdev);
    if(ret) {
        printk("create %s misc device node failed!\n", en331_dev[id].name);
        en331_dev[id].miscdev.name = 0;
        goto exit;
    }

exit:
    return ret;
}

static void en331_hw_reset(void)
{
    if(!rstb_used || !init)
        return;

    /* Active Low */
    plat_gpio_set_value(rstb_used, 0);

    udelay(100);
    /* Active High and release reset */
    plat_gpio_set_value(rstb_used, 1);

    udelay(100);
}

static int en331_device_init(int id)
{
    int ret;

    if(id >= EN331_DEV_MAX)
        return -1;

    if(!init)
        goto exit;

    /* video init */
    ret = en331_video_init(id, vout_format[id]);
    if(ret < 0)
        goto exit;

    /* audio init */
    /* ToDo */

exit:
    printk("EN331#%d Init...%s!\n", id, ((ret == 0) ? "OK" : "Fail"));

    return ret;
}

static int en331_watchdog_thread(void *data)
{
    int i, ch, ret;
    struct en331_dev_t *pdev;
    struct en331_video_fmt_t vin_fmt;

    do {
            /* check en331 channel status */
            for(i=0; i<dev_num; i++) {
                pdev = &en331_dev[i];

                down(&pdev->lock);

                for(ch=0; ch<EN331_DEV_CH_MAX; ch++) {
                    pdev->cur_plugin[ch] = (en331_status_get_video_loss(i, ch) == 0) ? 1 : 0;

                    if((pdev->cur_plugin[ch] == 1) && (pdev->pre_plugin[ch] == 0)) {        ///< cable plugged-in
                        goto sts_update;
                    }
                    else if((pdev->cur_plugin[ch] == 1) && (pdev->pre_plugin[ch] == 1)) {   ///< cable is connected
                            /* notify current video present */
                            if(notify && pdev->vlos_notify)
                                pdev->vlos_notify(i, ch, 0);

                            /* restore normal output setting */
                            en331_output_normal(i);

                            /* get input camera video fromat */
                            ret = en331_status_get_video_input_format(i, ch, &vin_fmt);
                            if(ret < 0)
                                goto sts_update;

                            /* notify current video format */
                            if(notify && pdev->vfmt_notify && (vin_fmt.fmt_idx >= EN331_VFMT_1080P24) && (vin_fmt.fmt_idx < EN331_VFMT_MAX))
                                pdev->vfmt_notify(i, ch, &vin_fmt);
                    }
                    else {  ///< cable is plugged-out
                        /* notify current video loss */
                        if(notify && pdev->vlos_notify)
                            pdev->vlos_notify(i, ch, 1);
                        /* notify current video format */
                        if(notify && pdev->vfmt_notify) {
                            /* set output format to 720P30 */
                            vin_fmt.fmt_idx = EN331_VFMT_720P30;
                            vin_fmt.width = 1280;
                            vin_fmt.height = 720;
                            vin_fmt.prog = 1;
                            vin_fmt.frame_rate = 30;

                            pdev->vfmt_notify(i, ch, &vin_fmt);
                            en331_output_black_image(i);
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

static int __init en331_init(void)
{
    int i, ret;

    /* platfrom check */
    plat_identifier_check();

    /* check device number */
    if(dev_num > EN331_DEV_MAX) {
        printk("EN331 dev_num=%d invalid!!(Max=%d)\n", dev_num, EN331_DEV_MAX);
        return -1;
    }

    /* check video output format */
    for(i=0; i<dev_num; i++) {
        if((vout_format[i] < 0) || (vout_format[i] >= EN331_VOUT_FORMAT_MAX)) {
            printk("EN331#%d vout_format=%d not support!!\n", i, vout_format[i]);
            return -1;
        }
    }

    /* register i2c client for contol en331 */
    ret = en331_i2c_register();
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

    /* create channel mapping table for different EVB */
    ret = en331_create_ch_map(ch_map);
    if(ret < 0)
        goto err;

    /* hardware reset for all device */
    en331_hw_reset();

    /* EN331 init */
    for(i=0; i<dev_num; i++) {
        en331_dev[i].index = i;

        sprintf(en331_dev[i].name, "en331.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&en331_dev[i].lock, 1);
#else
        init_MUTEX(&en331_dev[i].lock);
#endif
        ret = en331_proc_init(i);
        if(ret < 0)
            goto err;

        ret = en331_miscdev_init(i);
        if(ret < 0)
            goto err;

        ret = en331_device_init(i);
        if(ret < 0)
            goto err;
    }

    if(init) {
        /* init en331 watchdog thread for check video status */
        en331_wd = kthread_create(en331_watchdog_thread, NULL, "en331_wd");
        if(!IS_ERR(en331_wd))
            wake_up_process(en331_wd);
        else {
            printk("create en331 watchdog thread failed!!\n");
            en331_wd = 0;
            ret = -EFAULT;
            goto err;
        }
    }

    return 0;

err:
    if(en331_wd)
        kthread_stop(en331_wd);

    en331_i2c_unregister();
    for(i=0; i<EN331_DEV_MAX; i++) {
        if(en331_dev[i].miscdev.name)
            misc_deregister(&en331_dev[i].miscdev);

        en331_proc_remove(i);
    }

    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    return ret;
}

static void __exit en331_exit(void)
{
    int i;

    /* stop en331 watchdog thread */
    if(en331_wd)
        kthread_stop(en331_wd);

    en331_i2c_unregister();

    for(i=0; i<EN331_DEV_MAX; i++) {
        /* remove device node */
        if(en331_dev[i].miscdev.name)
            misc_deregister(&en331_dev[i].miscdev);

        /* remove proc node */
        en331_proc_remove(i);
    }

    /* release gpio pin */
    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);
}

module_init(en331_init);
module_exit(en331_exit);

MODULE_DESCRIPTION("Grain Media EN331 1CH HD-SDI Video Decoder Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
