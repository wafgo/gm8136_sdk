/**
 * @file ov7725.c
 * VCAP300 OV7725 Input Device Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.2 $
 * $Date: 2014/02/06 02:58:22 $
 *
 * ChangeLog:
 *  $Log: ov7725.c,v $
 *  Revision 1.2  2014/02/06 02:58:22  jerson_l
 *  1. modify version compare rule of input module driver
 *
 *  Revision 1.1  2013/11/11 05:50:19  jerson_l
 *  1. support ov7725
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#include "vcap_input.h"
#include "vcap_dbg.h"
#include "omni_vision/ov7725.h" ///< from module/include/front_end/sensor

#define DEV_MAX                 OV7725_DEV_MAX
#define DEFAULT_TYPE            VCAP_INPUT_TYPE_SENSOR
#define DEFAULT_INTERFACE       VCAP_INPUT_INTF_BT656_PROGRESSIVE
#define DEFAULT_NORM            OV7725_VMODE_VGA
#define DEFAULT_MODE            VCAP_INPUT_MODE_BYPASS
#define DEFAULT_SPEED           VCAP_INPUT_SPEED_P
#define DEFAULT_ORDER           VCAP_INPUT_ORDER_ANYONE
#define DEFAULT_DATA_RANGE      VCAP_INPUT_DATA_RANGE_256

static ushort vport[DEV_MAX] = {1};
module_param_array(vport, short, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vport, "VPort Link => 0:None, 1~8:X_CAP#0~7, 9:X_CAPCAS");

static int norm[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = DEFAULT_NORM};
module_param_array(norm, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(norm, "Video Norm=> 0:VGA");

static int inv_clk[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 0};
module_param_array(inv_clk, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "Invert Clock, 0:Disable, 1: Enable");

struct ov7725_dev_t {
    int                     index;                  ///< device index
    struct semaphore        lock;                   ///< device locker
    int                     norm;                   ///< device operation norm mode
    struct vcap_input_dev_t *port;
};

static struct ov7725_dev_t    ov7725_dev[DEV_MAX];
static struct proc_dir_entry *ov7725_proc_root[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *ov7725_proc_norm[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *ov7725_proc_timeout[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = NULL};

static int ov7725_module_get(void)
{
    return try_module_get(THIS_MODULE);
}

static void ov7725_module_put(void)
{
    module_put(THIS_MODULE);
}

static int ov7725_device_init(int id, int norm)
{
    int ret = 0;
    int width, height, frame_rate;
    u32 timeout;
    struct vcap_input_dev_t *pinput;

    if(id >= DEV_MAX)
        return -1;

    if(ov7725_dev[id].norm != norm) {
        ret = ov7725_video_set_mode(id, norm);
        if(ret < 0)
            goto exit;

        switch(norm) {
            case OV7725_VMODE_VGA:
            default:
                width      = 640;
                height     = 480;
                frame_rate = 30;
                break;
        }
        timeout = (1000/frame_rate)*2;

        ov7725_dev[id].norm = norm;

        /* Update device norm */
        pinput = ov7725_dev[id].port;
        if(pinput && (pinput->norm.mode != norm)) {
            pinput->norm.mode   = norm;
            pinput->norm.width  = width;
            pinput->norm.height = height;
            pinput->frame_rate  = pinput->max_frame_rate = frame_rate;
            pinput->timeout_ms  = timeout;
        }
    }

exit:
    return ret;
}

static int ov7725_proc_norm_show(struct seq_file *sfile, void *v)
{
    struct ov7725_dev_t *pdev = (struct ov7725_dev_t *)sfile->private;
    char *norm_str[] = {"VGA_640x480"};

    down(&pdev->lock);

    seq_printf(sfile, "Norm => 0:%s\n", norm_str[0]);
    seq_printf(sfile, "Current: %s\n", (pdev->norm == 0) ? norm_str[pdev->norm] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static int ov7725_proc_timeout_show(struct seq_file *sfile, void *v)
{
    struct ov7725_dev_t *pdev = (struct ov7725_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "Timeout Threshold: %d (ms)\n", (pdev->port)->timeout_ms);

    up(&pdev->lock);

    return 0;
}

static ssize_t ov7725_proc_norm_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ret = 0;
    int  norm_value;
    struct vcap_input_dev_t *pinput;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct ov7725_dev_t *pdev = (struct ov7725_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &norm_value);

    down(&pdev->lock);

    /* check port used or not? */
    pinput = pdev->port;
    if(pinput && pinput->used) {
        vcap_err("OV7725#%d switch norm failed, device still in used!\n", pdev->index);
        ret = -EFAULT;
        goto end;
    }

    if((norm_value < OV7725_VMODE_MAX) && (norm_value != pdev->norm)) {
        ret = ov7725_device_init(pdev->index, norm_value);
        if(ret < 0) {
            ret = -EFAULT;
            goto end;
        }
    }

end:
    up(&pdev->lock);

    if(ret)
        return ret;

    return count;
}

static ssize_t ov7725_proc_timeout_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ret = 0;
    int  time_value;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct ov7725_dev_t *pdev = (struct ov7725_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &time_value);

    down(&pdev->lock);

    if(time_value > 0) {
        if(pdev->port && ((pdev->port)->timeout_ms != time_value))
            (pdev->port)->timeout_ms = time_value;
    }

    up(&pdev->lock);

    if(ret)
        return ret;

    return count;
}

static int ov7725_proc_norm_open(struct inode *inode, struct file *file)
{
    return single_open(file, ov7725_proc_norm_show, PDE(inode)->data);
}

static int ov7725_proc_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, ov7725_proc_timeout_show, PDE(inode)->data);
}

static struct file_operations ov7725_proc_norm_ops = {
    .owner  = THIS_MODULE,
    .open   = ov7725_proc_norm_open,
    .write  = ov7725_proc_norm_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations ov7725_proc_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = ov7725_proc_timeout_open,
    .write  = ov7725_proc_timeout_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void ov7725_proc_remove(int id)
{
    if(ov7725_proc_root[id]) {
        if(ov7725_proc_timeout[id])
            vcap_input_proc_remove_entry(ov7725_proc_root[id], ov7725_proc_timeout[id]);

        if(ov7725_proc_norm[id])
            vcap_input_proc_remove_entry(ov7725_proc_root[id], ov7725_proc_norm[id]);

        vcap_input_proc_remove_entry(NULL, ov7725_proc_root[id]);
    }
}

static int ov7725_proc_init(struct ov7725_dev_t *pdev)
{
    int ret = 0;
    int id  = pdev->index;

    /* root */
    ov7725_proc_root[id] = vcap_input_proc_create_entry((pdev->port)->name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!ov7725_proc_root[id]) {
        vcap_err("create proc node '%s' failed!\n", (pdev->port)->name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    ov7725_proc_root[id]->owner = THIS_MODULE;
#endif

    /* norm */
    ov7725_proc_norm[id] = vcap_input_proc_create_entry("norm", S_IRUGO|S_IXUGO, ov7725_proc_root[id]);
    if(!ov7725_proc_norm[id]) {
        vcap_err("create proc node '%s/norm' failed!\n", (pdev->port)->name);
        ret = -EINVAL;
        goto err;
    }
    ov7725_proc_norm[id]->proc_fops = &ov7725_proc_norm_ops;
    ov7725_proc_norm[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    ov7725_proc_norm[id]->owner     = THIS_MODULE;
#endif

    /* timeout */
    ov7725_proc_timeout[id] = vcap_input_proc_create_entry("timeout", S_IRUGO|S_IXUGO, ov7725_proc_root[id]);
    if(!ov7725_proc_timeout[id]) {
        vcap_err("create proc node '%s/timeout' failed!\n", (pdev->port)->name);
        ret = -EINVAL;
        goto err;
    }
    ov7725_proc_timeout[id]->proc_fops = &ov7725_proc_timeout_ops;
    ov7725_proc_timeout[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    ov7725_proc_timeout[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    ov7725_proc_remove(id);
    return ret;
}

static int __init ov7725_input_init(void)
{
    int i;
    int dev_num;
    struct vcap_input_dev_t *pinput;
    int ret = 0;

    if(vcap_input_get_version() != VCAP_INPUT_VERSION) {
        vcap_err("Input driver version(%08x) is not compatibility with vcap300_common.ko(%08x)!\n",
                 vcap_input_get_version(), VCAP_INPUT_VERSION);
        return -EFAULT;
    }

    /* clear buffer */
    memset(ov7725_dev, 0, sizeof(struct ov7725_dev_t)*DEV_MAX);

    /* Get OV7725 Device Number */
    dev_num = ov7725_get_device_num();

    for(i=0; i<DEV_MAX; i++) {
        ov7725_dev[i].index = i;
        ov7725_dev[i].norm  = -1;  ///< init value

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&ov7725_dev[i].lock, 1);
#else
        init_MUTEX(&ov7725_dev[i].lock);
#endif
        if(i >= dev_num)
            continue;

        if(vport[i]) {
            /* register input device */
            pinput = ov7725_dev[i].port = kzalloc(sizeof(struct vcap_input_dev_t), GFP_KERNEL);
            if(pinput == NULL) {
                vcap_err("OV7725#%d allocate vcap_input_dev_t failed\n", i);
                ret = -ENOMEM;
                goto err;
            }

            /* input device name */
            snprintf(pinput->name, VCAP_INPUT_NAME_SIZE-1, "ov7725.%d", i);

            /* input device parameter setup */
            pinput->index       = i;
            pinput->vi_idx      = vport[i] - 1;         ///< VI#
            pinput->vi_src      = vport[i] - 1;         ///< X_CAP#
            pinput->type        = DEFAULT_TYPE;
            pinput->speed       = DEFAULT_SPEED;
            pinput->interface   = DEFAULT_INTERFACE;
            pinput->field_order = DEFAULT_ORDER;
            pinput->data_range  = DEFAULT_DATA_RANGE;
            pinput->yc_swap     = 0;
            pinput->data_swap   = 0;
            pinput->ch_id_mode  = 0;
            pinput->inv_clk     = inv_clk[i];
            pinput->init        = ov7725_device_init;
            pinput->mode        = DEFAULT_MODE;
            pinput->ch_id[0]    = ov7725_get_vch_id(i);
            pinput->module_get  = ov7725_module_get;
            pinput->module_put  = ov7725_module_put;
            pinput->norm.mode   = -1;                   ///< init value

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
            sema_init(&pinput->sema_lock, 1);
#else
            init_MUTEX(&pinput->sema_lock);
#endif
            ret = vcap_input_device_register(pinput);
            if(ret < 0) {
                vcap_err("register OV7725#%d input device failed\n", i);
                goto err;
            }

            /* device proc init */
            ret = ov7725_proc_init(&ov7725_dev[i]);
            if(ret < 0)
                goto err;

            /* device init */
            ret = ov7725_device_init(i, norm[i]);
            if(ret < 0)
                goto err;

            vcap_info("Register OV7725#%d Input Device\n", i);
        }
    }

    return ret;

err:
    for(i=0; i<DEV_MAX; i++) {
        ov7725_proc_remove(i);
        if(ov7725_dev[i].port) {
            vcap_input_device_unregister(ov7725_dev[i].port);
            kfree(ov7725_dev[i].port);
        }
    }
    return ret;
}

static void __exit ov7725_input_exit(void)
{
    int i;

    for(i=0; i<DEV_MAX; i++) {
        ov7725_proc_remove(i);
        if(ov7725_dev[i].port) {
            vcap_input_device_unregister(ov7725_dev[i].port);
            kfree(ov7725_dev[i].port);
        }
    }
}

module_init(ov7725_input_init);
module_exit(ov7725_input_exit);

MODULE_DESCRIPTION("Grain Media Video Capture300 OV7725 Input Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
