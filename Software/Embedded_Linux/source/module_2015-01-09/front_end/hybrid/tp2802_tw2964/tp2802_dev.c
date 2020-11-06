/**
 * @file tp2802_drv.c
 * TechPoint TP2802 4CH HD-TVI Video Decoder Driver (for hybrid design)
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.2 $
 * $Date: 2014/10/21 12:42:09 $
 *
 * ChangeLog:
 *  $Log: tp2802_dev.c,v $
 *  Revision 1.2  2014/10/21 12:42:09  shawn_hu
 *  1. Set HDTVI default output format to 720P30.
 *  2. Add error handling for TP2802 I2C read (tp2802_i2c_read_ext()), and it will retry 3 times when read failed.
 *
 *  Revision 1.1  2014/09/15 12:17:35  shawn_hu
 *  Add TechPoint TP2802(HDTVI) & Intersil TW2964(D1) hybrid frontend driver design for GM8286 EVB.
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
#include "techpoint/tp2802.h"                       ///< from module/include/front_end/hdtvi
#include "tp2802_dev.h"
#include "tp2802_tw2964_hybrid_drv.h"

/*************************************************************************************
 *  Initial values of the following structures come from tp2802_tw2964_hybrid.c
 *************************************************************************************/
static int dev_num;
static ushort iaddr[TP2802_DEV_MAX];
static int vout_format[TP2802_DEV_MAX];
static struct tp2802_dev_t* tp2802_dev;
static int init;
static u32 TP2802_TW2964_VCH_MAP[TP2802_DEV_MAX*TP2802_DEV_CH_MAX];

/*************************************************************************************
 *  Original structure
 *************************************************************************************/
static struct proc_dir_entry *tp2802_proc_root[TP2802_DEV_MAX]      = {[0 ... (TP2802_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tp2802_proc_status[TP2802_DEV_MAX]    = {[0 ... (TP2802_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tp2802_proc_video_fmt[TP2802_DEV_MAX] = {[0 ... (TP2802_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tp2802_proc_vout_fmt[TP2802_DEV_MAX]  = {[0 ... (TP2802_DEV_MAX - 1)] = NULL};

/*************************************************************************************
 *  Revised for hybrid design
 *************************************************************************************/
int tp2802_i2c_write(u8 id, u8 reg, u8 data)
{
    return tp2802_tw2964_i2c_write(iaddr[id], reg, data);
}
EXPORT_SYMBOL(tp2802_i2c_write);

u8 tp2802_i2c_read(u8 id, u8 reg)
{
    return tp2802_tw2964_i2c_read(iaddr[id], reg);
}
EXPORT_SYMBOL(tp2802_i2c_read);

int tp2802_i2c_read_ext(u8 id, u8 reg)
{
    return tp2802_tw2964_i2c_read_ext(iaddr[id], reg);
}
EXPORT_SYMBOL(tp2802_i2c_read_ext);

void tp2802_set_params(tp2802_params_p params_p)
{
    int i = 0;

    dev_num = params_p->tp2802_dev_num;
    for(i=0; i<dev_num; i++) {
        iaddr[i] = *(params_p->tp2802_iaddr + i);
    }
    for(i=0; i<TP2802_DEV_MAX; i++) {
        vout_format[i] = *(params_p->tp2802_vout_format + i);
    }
    tp2802_dev = params_p->tp2802_dev;
    init = params_p->init;
    for(i=0; i<TP2802_DEV_MAX*TP2802_DEV_CH_MAX; i++) {
        TP2802_TW2964_VCH_MAP[i] = *(params_p->TP2802_TW2964_VCH_MAP + i);
    }
}   

/*************************************************************************************
 *  Keep as original design (only XXXX_VCH_MAP was updated)
 *************************************************************************************/
int tp2802_get_vch_id(int id, TP2802_DEV_VOUT_T vout, int vin_ch)
{
    int i;

    if(id >= dev_num || vout >= TP2802_DEV_VOUT_MAX || vin_ch >= TP2802_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*TP2802_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[i]) == id) &&
           (CH_IDENTIFY_VOUT(TP2802_TW2964_VCH_MAP[i]) == vout) &&
           (CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[i]) == vin_ch)) {
            return i;
        }
    }

    return 0;
}
EXPORT_SYMBOL(tp2802_get_vch_id);

int tp2802_get_vout_format(int id)
{
    if(id >= dev_num)
        return TP2802_VOUT_FORMAT_BT656;
    else
        return vout_format[id];
}
EXPORT_SYMBOL(tp2802_get_vout_format);

int tp2802_notify_vfmt_register(int id, int (*nt_func)(int, int, struct tp2802_video_fmt_t *))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&tp2802_dev[id].lock);

    tp2802_dev[id].vfmt_notify = nt_func;

    up(&tp2802_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(tp2802_notify_vfmt_register);

void tp2802_notify_vfmt_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&tp2802_dev[id].lock);

    tp2802_dev[id].vfmt_notify = NULL;

    up(&tp2802_dev[id].lock);
}
EXPORT_SYMBOL(tp2802_notify_vfmt_deregister);

int tp2802_notify_vlos_register(int id, int (*nt_func)(int, int, int))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&tp2802_dev[id].lock);

    tp2802_dev[id].vlos_notify = nt_func;

    up(&tp2802_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(tp2802_notify_vlos_register);

void tp2802_notify_vlos_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&tp2802_dev[id].lock);

    tp2802_dev[id].vlos_notify = NULL;

    up(&tp2802_dev[id].lock);
}
EXPORT_SYMBOL(tp2802_notify_vlos_deregister);

static int tp2802_proc_status_show(struct seq_file *sfile, void *v)
{
    int i, j;
    int vlos;
    struct tp2802_dev_t *pdev = (struct tp2802_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[TP2802#%d]\n", pdev->index);
    seq_printf(sfile, "------------------\n");
    seq_printf(sfile, "VIN    VCH    LOS \n");
    seq_printf(sfile, "==================\n");
    for(i=0; i<TP2802_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TP2802_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[j]) == i)) {
                vlos = tp2802_status_get_video_loss(pdev->index, i);
                seq_printf(sfile, "%-6d %-6d %-7s\n", i, j, ((vlos == 0) ? "NO" : "YES"));
            }
        }
    }

    up(&pdev->lock);

    return 0;
}

static int tp2802_proc_video_fmt_show(struct seq_file *sfile, void *v)
{
    int i, j, ret;
    struct tp2802_video_fmt_t vfmt;
    struct tp2802_dev_t *pdev = (struct tp2802_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[TP2802#%d] Incoming Video Fromat\n", pdev->index);
    seq_printf(sfile, "====================================================\n");
    seq_printf(sfile, "VIN   VCH   Width   Height   Prog/Inter   Frame_Rate\n");
    seq_printf(sfile, "====================================================\n");

    for(i=0; i<TP2802_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TP2802_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[j]) == i)) {
                ret = tp2802_status_get_video_input_format(pdev->index, i, &vfmt);
                if((ret == 0) && ((vfmt.fmt_idx >= 0) && (vfmt.fmt_idx < TP2802_VFMT_MAX))) {
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

static int tp2802_proc_vout_fmt_show(struct seq_file *sfile, void *v)
{
    int i, j, ret;
    struct tp2802_video_fmt_t vout_fmt;
    struct tp2802_dev_t *pdev = (struct tp2802_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[TP2802#%d] Video Output Fromat\n", pdev->index);
    seq_printf(sfile, "=====================================================\n");
    seq_printf(sfile, "VOUT   VCH   Width   Height   Prog/Inter   Frame_Rate\n");
    seq_printf(sfile, "=====================================================\n");

    for(i=0; i<TP2802_DEV_VOUT_MAX; i++) {
        for(j=0; j<(dev_num*TP2802_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VOUT(TP2802_TW2964_VCH_MAP[j]) == i)) {
                ret = tp2802_video_get_video_output_format(pdev->index, CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[j]), &vout_fmt);
                if((ret == 0) && ((vout_fmt.fmt_idx >= TP2802_VFMT_720P60) && (vout_fmt.fmt_idx < TP2802_VFMT_MAX))) {
                    seq_printf(sfile, "%-6d %-5d %-7d %-8d %-12s %-11d\n",
                               i,
                               j,
                               vout_fmt.width,
                               vout_fmt.height,
                               ((vout_fmt.prog == 1) ? "Progressive" : "Interlace"),
                               vout_fmt.frame_rate);
                }
                else {
                    seq_printf(sfile, "%-6d %-5d %-7s %-8s %-12s %-11s\n", i, j, "-", "-", "-", "-");
                }
            }
        }
    }

    up(&pdev->lock);

    return 0;
}

static int tp2802_proc_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, tp2802_proc_status_show, PDE(inode)->data);
}

static int tp2802_proc_video_fmt_open(struct inode *inode, struct file *file)
{
    return single_open(file, tp2802_proc_video_fmt_show, PDE(inode)->data);
}

static int tp2802_proc_vout_fmt_open(struct inode *inode, struct file *file)
{
    return single_open(file, tp2802_proc_vout_fmt_show, PDE(inode)->data);
}

static struct file_operations tp2802_proc_status_ops = {
    .owner  = THIS_MODULE,
    .open   = tp2802_proc_status_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tp2802_proc_video_fmt_ops = {
    .owner  = THIS_MODULE,
    .open   = tp2802_proc_video_fmt_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tp2802_proc_vout_fmt_ops = {
    .owner  = THIS_MODULE,
    .open   = tp2802_proc_vout_fmt_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

void tp2802_proc_remove(int id)
{
    if(id >= TP2802_DEV_MAX)
        return;

    if(tp2802_proc_root[id]) {
        if(tp2802_proc_status[id]) {
            remove_proc_entry(tp2802_proc_status[id]->name, tp2802_proc_root[id]);
            tp2802_proc_status[id] = NULL;
        }

        if(tp2802_proc_video_fmt[id]) {
            remove_proc_entry(tp2802_proc_video_fmt[id]->name, tp2802_proc_root[id]);
            tp2802_proc_video_fmt[id] = NULL;
        }

        if(tp2802_proc_vout_fmt[id]) {
            remove_proc_entry(tp2802_proc_vout_fmt[id]->name, tp2802_proc_root[id]);
            tp2802_proc_vout_fmt[id] = NULL;
        }

        remove_proc_entry(tp2802_proc_root[id]->name, NULL);
        tp2802_proc_root[id] = NULL;
    }
}

int tp2802_proc_init(int id)
{
    int ret = 0;

    if(id >= TP2802_DEV_MAX)
        return -1;

    /* root */
    tp2802_proc_root[id] = create_proc_entry(tp2802_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!tp2802_proc_root[id]) {
        printk("create proc node '%s' failed!\n", tp2802_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tp2802_proc_root[id]->owner = THIS_MODULE;
#endif

    /* status */
    tp2802_proc_status[id] = create_proc_entry("status", S_IRUGO|S_IXUGO, tp2802_proc_root[id]);
    if(!tp2802_proc_status[id]) {
        printk("create proc node '%s/status' failed!\n", tp2802_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tp2802_proc_status[id]->proc_fops = &tp2802_proc_status_ops;
    tp2802_proc_status[id]->data      = (void *)&tp2802_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tp2802_proc_status[id]->owner     = THIS_MODULE;
#endif

    /* frame format */
    tp2802_proc_video_fmt[id] = create_proc_entry("video_format", S_IRUGO|S_IXUGO, tp2802_proc_root[id]);
    if(!tp2802_proc_video_fmt[id]) {
        printk("create proc node '%s/video_format' failed!\n", tp2802_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tp2802_proc_video_fmt[id]->proc_fops = &tp2802_proc_video_fmt_ops;
    tp2802_proc_video_fmt[id]->data      = (void *)&tp2802_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tp2802_proc_video_fmt[id]->owner     = THIS_MODULE;
#endif

    /* video output format */
    tp2802_proc_vout_fmt[id] = create_proc_entry("vout_format", S_IRUGO|S_IXUGO, tp2802_proc_root[id]);
    if(!tp2802_proc_vout_fmt[id]) {
        printk("create proc node '%s/vout_format' failed!\n", tp2802_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tp2802_proc_vout_fmt[id]->proc_fops = &tp2802_proc_vout_fmt_ops;
    tp2802_proc_vout_fmt[id]->data      = (void *)&tp2802_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tp2802_proc_vout_fmt[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    tp2802_proc_remove(id);
    return ret;
}

static int tp2802_miscdev_open(struct inode *inode, struct file *file)
{
    int i, ret = 0;
    struct tp2802_dev_t *pdev = NULL;

    /* lookup device */
    for(i=0; i<dev_num; i++) {
        if(tp2802_dev[i].miscdev.minor == iminor(inode)) {
            pdev = &tp2802_dev[i];
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

static int tp2802_miscdev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long tp2802_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct tp2802_dev_t *pdev = (struct tp2802_dev_t *)file->private_data;

    down(&pdev->lock);

    if(_IOC_TYPE(cmd) != TP2802_IOC_MAGIC) {
        ret = -ENOIOCTLCMD;
        goto exit;
    }

    switch(cmd) {
        case TP2802_GET_NOVID:
            {
                struct tp2802_ioc_data_t ioc_data;

                if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_data.vin_ch >= TP2802_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                ioc_data.data = tp2802_status_get_video_loss(pdev->index, ioc_data.vin_ch);

                ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            }
            break;

        case TP2802_GET_VIDEO_FMT:
            {
                struct tp2802_ioc_vfmt_t ioc_vfmt;
                struct tp2802_video_fmt_t vfmt;

                if(copy_from_user(&ioc_vfmt, (void __user *)arg, sizeof(ioc_vfmt))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vfmt.vin_ch >= TP2802_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                ret = tp2802_status_get_video_input_format(pdev->index, ioc_vfmt.vin_ch, &vfmt);
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

static struct file_operations tp2802_miscdev_fops = {
    .owner          = THIS_MODULE,
    .open           = tp2802_miscdev_open,
    .release        = tp2802_miscdev_release,
    .unlocked_ioctl = tp2802_miscdev_ioctl,
};

int tp2802_miscdev_init(int id)
{
    int ret;

    if(id >= TP2802_DEV_MAX)
        return -1;

    /* clear */
    memset(&tp2802_dev[id].miscdev, 0, sizeof(struct miscdevice));

    /* create device node */
    tp2802_dev[id].miscdev.name  = tp2802_dev[id].name;
    tp2802_dev[id].miscdev.minor = MISC_DYNAMIC_MINOR;
    tp2802_dev[id].miscdev.fops  = (struct file_operations *)&tp2802_miscdev_fops;
    ret = misc_register(&tp2802_dev[id].miscdev);
    if(ret) {
        printk("create %s misc device node failed!\n", tp2802_dev[id].name);
        tp2802_dev[id].miscdev.name = 0;
        goto exit;
    }

exit:
    return ret;
}

int tp2802_device_init(int id)
{
    int ret = 0;

    if(id >= TP2802_DEV_MAX)
        return -1;

    if(!init)
        goto exit;

    /* video init */
    ret = tp2802_video_init(id, vout_format[id]);
    if(ret < 0)
        goto exit;

    /* audio init */
    /* ToDo */

exit:
    printk("TP2802#%d Init...%s!\n", id, ((ret == 0) ? "OK" : "Fail"));

    return ret;
}

