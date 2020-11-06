/**
 * @file tw9900.c
 * VCAP300 TW9900 Input Device Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.6 $
 * $Date: 2014/03/31 02:33:06 $
 *
 * ChangeLog:
 *  $Log: tw9900.c,v $
 *  Revision 1.6  2014/03/31 02:33:06  jerson_l
 *  1. remove un-used vcap_i2c.h header file include
 *
 *  Revision 1.5  2014/02/06 02:58:22  jerson_l
 *  1. modify version compare rule of input module driver
 *
 *  Revision 1.4  2014/02/06 02:14:05  jerson_l
 *  1. record channel vlos status when status notified
 *
 *  Revision 1.3  2013/11/27 05:38:47  jerson_l
 *  1. support status notify
 *
 *  Revision 1.2  2013/11/07 06:40:00  jerson_l
 *  1. support max_frame_rate infomation
 *
 *  Revision 1.1  2013/10/25 04:08:40  jerson_l
 *  1. support tw9900
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

#include "vcap_input.h"
#include "vcap_dbg.h"
#include "intersil/tw9900.h"    ///< from module/include/front_end/decoder

#define DEV_MAX                 TW9900_DEV_MAX
#define DEV_VPORT_MAX           TW9900_DEV_VPORT_MAX            ///< VD1
#define DEV_CH_MAX              TW9900_DEV_CH_MAX
#define DEFAULT_TYPE            VCAP_INPUT_TYPE_DECODER
#define DEFAULT_SPEED           VCAP_INPUT_SPEED_I
#define DEFAULT_INTERFACE       VCAP_INPUT_INTF_BT656_INTERLACE
#define DEFAULT_NORM            VCAP_INPUT_NORM_NTSC
#define DEFAULT_DATA_RANGE      VCAP_INPUT_DATA_RANGE_256
#define DEFAULT_FIELD_ORDER     VCAP_INPUT_ORDER_ODD_FIRST
#define DEFAULT_CH_ID_MODE      VCAP_INPUT_CH_ID_EAV_SAV
#define DEFAULT_NTSC_TIMEOUT    66                               ///< 33(ms)*2(frame), 2 frame delay time
#define DEFAULT_PAL_TIMEOUT     80                               ///< 40(ms)*2(frame), 2 frame delay time

static ushort vport[DEV_MAX] = {0x0001, 0x0002, 0x0003, 0x0004};
module_param_array(vport, short, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vport, "VPort Link => VPortA[0:3], VPortB[3:7], VPortC[8:11], VPortD[12:15], 0:None, 1~8:X_CAP#0~7, 9:X_CAPCAS");

static int mode[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 0};
module_param_array(mode, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mode, "Video Output Mode=> 0:27MHz");

static int norm[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = DEFAULT_NORM};
module_param_array(norm, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(norm, "Video Norm=> 0:PAL, 1:NTSC");

static int order[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = DEFAULT_FIELD_ORDER};
module_param_array(order, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(order, "Field Order=> 0:Anyone, 1:Odd First, 2:Even First");

static int inv_clk[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 1};
module_param_array(inv_clk, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "Invert Clock, 0:Disable, 1: Enable");

struct tw9900_dev_t {
    int                     index;                  ///< device index
    struct semaphore        lock;                   ///< device locker
    int                     norm;                   ///< device operation norm mode
    int                     vlos[DEV_CH_MAX];
    struct vcap_input_dev_t *port[DEV_VPORT_MAX];
};

static struct tw9900_dev_t    tw9900_dev[DEV_MAX];
static struct proc_dir_entry *tw9900_proc_root[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw9900_proc_norm[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw9900_proc_timeout[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = NULL};

static int tw9900_module_get(void)
{
    return try_module_get(THIS_MODULE);
}

static void tw9900_module_put(void)
{
    module_put(THIS_MODULE);
}

static int tw9900_device_init(int id, int norm)
{
    int i, ret = 0;
    int width, height, frame_rate, pixel_clk;
    u32 timeout;
    struct vcap_input_dev_t *pinput;

    if((id >= DEV_MAX) || (id >= tw9900_get_device_num()))
        return -1;

    if(tw9900_dev[id].norm != norm) {
        switch(norm) {
            case VCAP_INPUT_NORM_PAL:
                height     = 576;
                frame_rate = 25;
                timeout    = DEFAULT_PAL_TIMEOUT;
                switch(mode[id]) {
                    case 0:
                    default: /* 27MHz */
                        width     = 720;
                        pixel_clk = 27000000;
                        if(tw9900_video_get_mode(id) != TW9900_VMODE_PAL_720H_1CH)
                            ret = tw9900_video_set_mode(id, TW9900_VMODE_PAL_720H_1CH);
                        break;
                }
                break;
            default:    /* NTSC */
                height     = 480;
                frame_rate = 30;
                timeout    = DEFAULT_NTSC_TIMEOUT;
                switch(mode[id]) {
                    case 0:
                    default: /* 27MHz */
                        width     = 720;
                        pixel_clk = 27000000;
                        if(tw9900_video_get_mode(id) != TW9900_VMODE_NTSC_720H_1CH)
                            ret = tw9900_video_set_mode(id, TW9900_VMODE_NTSC_720H_1CH);
                        break;
                }
                break;
        }
        if(ret < 0)
            goto end;

        /* Update device norm */
        tw9900_dev[id].norm = norm;
        for(i=0; i<DEV_VPORT_MAX; i++) {
            pinput = tw9900_dev[id].port[i];
            if(pinput && (pinput->norm.mode != norm)) {
                pinput->norm.mode   = norm;
                pinput->norm.width  = width;
                pinput->norm.height = height;
                pinput->frame_rate  = pinput->max_frame_rate = frame_rate;
                pinput->pixel_clk   = pixel_clk;
                pinput->timeout_ms  = timeout;
            }
        }
    }

end:
    if(ret < 0)
        vcap_err("TW9900#%d init failed!\n", id);

    return ret;
}

static int tw9900_proc_norm_show(struct seq_file *sfile, void *v)
{
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;
    char *norm_str[] = {"PAL", "NTSC"};

    down(&pdev->lock);

    seq_printf(sfile, "Norm => 0:%s, 1:%s\n", norm_str[0], norm_str[1]);
    seq_printf(sfile, "Current: %s\n", (pdev->norm>=0 && pdev->norm<=1) ? norm_str[pdev->norm] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static int tw9900_proc_timeout_show(struct seq_file *sfile, void *v)
{
    int i;
    u32 time_ms = 0;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;

    down(&pdev->lock);

    for(i=0; i<DEV_VPORT_MAX; i++) {
        if(pdev->port[i]) {
            time_ms = (pdev->port[i])->timeout_ms;
            break;
        }
    }
    seq_printf(sfile, "Timeout Threshold: %u (ms)\n", time_ms);

    up(&pdev->lock);

    return 0;
}

static ssize_t tw9900_proc_norm_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, ret = 0;
    int  norm_value;
    struct vcap_input_dev_t *pinput;
    char value_str[8] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &norm_value);

    down(&pdev->lock);

    /* check vport used or not? */
    for(i=0; i<DEV_VPORT_MAX; i++) {
        pinput = pdev->port[i];
        if(pinput && pinput->used) {
            vcap_err("TW9900#%d switch norm failed, device still in used!\n", pdev->index);
            ret = -EFAULT;
            goto end;
        }
    }

    if((norm_value <= 1) && (norm_value != norm[pdev->index])) {
        ret = tw9900_device_init(pdev->index, norm_value);
        if(ret < 0) {
            ret = -EFAULT;
            goto end;
        }
        norm[pdev->index] = norm_value;
    }

end:
    up(&pdev->lock);

    if(ret)
        return ret;

    return count;
}

static ssize_t tw9900_proc_timeout_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, time_value;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw9900_dev_t *pdev = (struct tw9900_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &time_value);

    down(&pdev->lock);

    if(time_value > 0) {
        for(i=0; i<DEV_VPORT_MAX; i++) {
            if(pdev->port[i] && ((pdev->port[i])->timeout_ms != time_value))
                (pdev->port[i])->timeout_ms = time_value;
        }
    }

    up(&pdev->lock);

    return count;
}

static int tw9900_proc_norm_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw9900_proc_norm_show, PDE(inode)->data);
}

static int tw9900_proc_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw9900_proc_timeout_show, PDE(inode)->data);
}

static struct file_operations tw9900_proc_norm_ops = {
    .owner  = THIS_MODULE,
    .open   = tw9900_proc_norm_open,
    .write  = tw9900_proc_norm_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw9900_proc_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = tw9900_proc_timeout_open,
    .write  = tw9900_proc_timeout_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void tw9900_proc_remove(int id)
{
    if(tw9900_proc_root[id]) {
        if(tw9900_proc_timeout[id])
            vcap_input_proc_remove_entry(tw9900_proc_root[id], tw9900_proc_timeout[id]);

        if(tw9900_proc_norm[id])
            vcap_input_proc_remove_entry(tw9900_proc_root[id], tw9900_proc_norm[id]);

        vcap_input_proc_remove_entry(NULL, tw9900_proc_root[id]);
    }
}

static int tw9900_proc_init(struct tw9900_dev_t *pdev)
{
    int ret = 0;
    int id  = pdev->index;
    char name[32];

    /* root */
    sprintf(name, "tw9900.%d", id);
    tw9900_proc_root[id] = vcap_input_proc_create_entry(name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!tw9900_proc_root[id]) {
        vcap_err("create proc node '%s' failed!\n", name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw9900_proc_root[id]->owner = THIS_MODULE;
#endif

    /* norm */
    tw9900_proc_norm[id] = vcap_input_proc_create_entry("norm", S_IRUGO|S_IXUGO, tw9900_proc_root[id]);
    if(!tw9900_proc_norm[id]) {
        vcap_err("create proc node '%s/norm' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    tw9900_proc_norm[id]->proc_fops = &tw9900_proc_norm_ops;
    tw9900_proc_norm[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw9900_proc_norm[id]->owner     = THIS_MODULE;
#endif

    /* timeout */
    tw9900_proc_timeout[id] = vcap_input_proc_create_entry("timeout", S_IRUGO|S_IXUGO, tw9900_proc_root[id]);
    if(!tw9900_proc_timeout[id]) {
        vcap_err("create proc node '%s/timeout' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    tw9900_proc_timeout[id]->proc_fops = &tw9900_proc_timeout_ops;
    tw9900_proc_timeout[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw9900_proc_timeout[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    tw9900_proc_remove(id);
    return ret;
}

static int tw9900_vlos_notify_handler(int id, int vin, int vlos)
{
    int i, j, vch;
    struct tw9900_dev_t *pdev = &tw9900_dev[id];

    if((id >= DEV_MAX) || (vin >= DEV_CH_MAX))
        return -1;

    down(&pdev->lock);

    if(pdev->vlos[vin] != vlos) {
        pdev->vlos[vin] = vlos;

        vch = tw9900_vin_to_vch(id, vin);
        for(i=0; i<DEV_VPORT_MAX; i++) {
            if(!pdev->port[i])
                continue;

            for(j=0; j<VCAP_INPUT_DEV_CH_MAX; j++) {
                if((pdev->port[i])->ch_id[j] == vch) {
                    (pdev->port[i])->ch_vlos[j] = vlos;
                    vcap_input_device_notify((pdev->port[i])->vi_idx, j, ((vlos == 0) ? VCAP_INPUT_NOTIFY_SIGNAL_PRESENT : VCAP_INPUT_NOTIFY_SIGNAL_LOSS));
                    goto exit;
                }
            }
        }
    }

exit:
    up(&pdev->lock);

    return 0;
}

static int __init tw9900_input_init(void)
{
    int i, j;
    int dev_num;
    struct vcap_input_dev_t *pinput;
    int ret = 0;

    if(vcap_input_get_version() != VCAP_INPUT_VERSION) {
        vcap_err("Input driver version(%08x) is not compatibility with vcap300_common.ko(%08x)!\n",
                 vcap_input_get_version(), VCAP_INPUT_VERSION);
        return -EFAULT;
    }

    /* clear buffer */
    memset(tw9900_dev, 0, sizeof(struct tw9900_dev_t)*DEV_MAX);

    /* Get TW9900 Device Number */
    dev_num = tw9900_get_device_num();

    for(i=0; i<DEV_MAX; i++) {
        tw9900_dev[i].index = i;
        tw9900_dev[i].norm  = -1;  ///< init value

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&tw9900_dev[i].lock, 1);
#else
        init_MUTEX(&tw9900_dev[i].lock);
#endif
        if(i >= dev_num)
            continue;

        if(vport[i]) {
            /* register input device */
            for(j=0; j<DEV_VPORT_MAX; j++) {
                if((vport[i]>>(4*j)) & 0xf) {   ///< VPort# Connect to X_CAP#
                    pinput = tw9900_dev[i].port[j] = kzalloc(sizeof(struct vcap_input_dev_t), GFP_KERNEL);
                    if(pinput == NULL) {
                        vcap_err("TW9900#%d.%d allocate vcap_input_dev_t failed\n", i, j);
                        ret = -ENOMEM;
                        goto err;
                    }

                    /* input device name */
                    snprintf(pinput->name, VCAP_INPUT_NAME_SIZE-1, "tw9900.%d.%d", i, j);

                    /* input device parameter setup */
                    pinput->index       = (i*DEV_VPORT_MAX) + j;
                    pinput->vi_idx      = ((vport[i]>>(4*j)) & 0xf) - 1; ///< VI#
                    pinput->vi_src      = ((vport[i]>>(4*j)) & 0xf) - 1; ///< X_CAP#
                    pinput->type        = DEFAULT_TYPE;
                    pinput->speed       = DEFAULT_SPEED;
                    pinput->interface   = DEFAULT_INTERFACE;
                    pinput->field_order = order[i];
                    pinput->data_range  = DEFAULT_DATA_RANGE;
                    pinput->yc_swap     = 0;
                    pinput->data_swap   = 0;
                    pinput->ch_id_mode  = DEFAULT_CH_ID_MODE;
                    pinput->inv_clk     = inv_clk[i];
                    pinput->init        = tw9900_device_init;
                    pinput->norm.mode   = -1;                            ///< init value
                    pinput->module_get  = tw9900_module_get;
                    pinput->module_put  = tw9900_module_put;

                    switch(mode[i]) {
                        case 0: /* 27MHz */
                        default:
                            pinput->mode     = VCAP_INPUT_MODE_BYPASS;
                            pinput->ch_id[0] = tw9900_get_vch_id(i, j);
                            break;
                    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
                    sema_init(&pinput->sema_lock, 1);
#else
                    init_MUTEX(&pinput->sema_lock);
#endif
                    ret = vcap_input_device_register(pinput);
                    if(ret < 0) {
                        vcap_err("register TW9900#%d.%d input device failed\n", i, j);
                        goto err;
                    }
                }
            }

            /* device proc init */
            ret = tw9900_proc_init(&tw9900_dev[i]);
            if(ret < 0)
                goto err;

            /* device init */
            ret = tw9900_device_init(i, norm[i]);
            if(ret < 0)
                goto err;

            /* register tw9900 video loss notify handler */
            tw9900_notify_vlos_register(i, tw9900_vlos_notify_handler);

            vcap_info("Register TW9900#%d Input Device\n", i);
        }
    }

    return ret;

err:
    for(i=0; i<DEV_MAX; i++) {
        tw9900_notify_vlos_deregister(i);
        tw9900_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(tw9900_dev[i].port[j]) {
                vcap_input_device_unregister(tw9900_dev[i].port[j]);
                kfree(tw9900_dev[i].port[j]);
            }
        }
    }
    return ret;
}

static void __exit tw9900_input_exit(void)
{
    int i, j;

    for(i=0; i<DEV_MAX; i++) {
        tw9900_notify_vlos_deregister(i);
        tw9900_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(tw9900_dev[i].port[j]) {
                vcap_input_device_unregister(tw9900_dev[i].port[j]);
                kfree(tw9900_dev[i].port[j]);
            }
        }
    }
}

module_init(tw9900_input_init);
module_exit(tw9900_input_exit);

MODULE_DESCRIPTION("Grain Media Video Capture300 TW9900 Input Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
