/**
 * @file tw2868.c
 * VCAP300 TW2868 Input Device Driver
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
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
#include "intersil/tw2868.h"       ///< from module/include/front_end/decoder

#define DEV_MAX                 TW2868_DEV_MAX
#define DEV_VPORT_MAX           TW2868_DEV_VPORT_MAX            ///< VD1, VD2, VD3, VD4
#define DEV_CH_MAX              TW2868_DEV_CH_MAX
#define DEFAULT_TYPE            VCAP_INPUT_TYPE_DECODER
#define DEFAULT_SPEED           VCAP_INPUT_SPEED_I
#define DEFAULT_INTERFACE       VCAP_INPUT_INTF_BT656_INTERLACE
#define DEFAULT_NORM            VCAP_INPUT_NORM_NTSC
#define DEFAULT_DATA_RANGE      VCAP_INPUT_DATA_RANGE_256
#define DEFAULT_FIELD_ORDER     VCAP_INPUT_ORDER_ODD_FIRST
#define DEFAULT_CH_ID_MODE      VCAP_INPUT_CH_ID_EAV_SAV
#define DEFAULT_NTSC_TIMEOUT    66                               ///< 33(ms)*2(frame), 2 frame delay time
#define DEFAULT_PAL_TIMEOUT     80                               ///< 40(ms)*2(frame), 2 frame delay time

static ushort vport[DEV_MAX] = {0x0012, 0x0034, 0x0056, 0x0078};
module_param_array(vport, short, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vport, "VPort Link => VPortA[0:3], VPortB[3:7], VPortC[8:11], VPortD[12:15], 0:None, 1~8:X_CAP#0~7, 9:X_CAPCAS");

static int mode[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 2};
module_param_array(mode, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mode, "Video Output Mode=> 0:27MHz/36MHz, 1:54MHz/72MHz, 2:108MHz/144MHz");

static int norm[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = DEFAULT_NORM};
module_param_array(norm, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(norm, "Video Norm=> 0:PAL, 1:NTSC");

static int order[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = DEFAULT_FIELD_ORDER};
module_param_array(order, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(order, "Field Order=> 0:Anyone, 1:Odd First, 2:Even First");

static int inv_clk[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 0};
module_param_array(inv_clk, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "Invert Clock, 0:Disable, 1: Enable");

struct tw2868_dev_t {
    int                     index;                  ///< device index
    struct semaphore        lock;                   ///< device locker
    int                     norm;                   ///< device operation norm mode
    int                     vlos[DEV_CH_MAX];
    struct vcap_input_dev_t *port[DEV_VPORT_MAX];
};

static struct tw2868_dev_t    tw2868_dev[DEV_MAX];
static struct proc_dir_entry *tw2868_proc_root[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2868_proc_norm[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2868_proc_timeout[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = NULL};

/****************************************************************************************
 *  TW2868 VPort and X_CAP# Port Mapping for GM8283 Socket Board
 *
 *  TW2868#0(0x52) --->VD1 ---- [VIN#0,1,2,3] 108/144MHz ---> X_CAP#0
 *                 --->VD2 ---- [VIN#4,5,6,7] 108/144MHz ---> X_CAP#1
 *                 --->VD3 ---- X
 *                 --->VD4 ---- X
 *
 *****************************************************************************************/

static int tw2868_module_get(void)
{
    return try_module_get(THIS_MODULE);
}

static void tw2868_module_put(void)
{
    module_put(THIS_MODULE);
}

static int tw2868_device_init(int id, int norm)
{
    int i, ret = 0;
    int width, height, frame_rate, pixel_clk;
    u32 timeout;
    struct vcap_input_dev_t *pinput;

    if((id >= DEV_MAX) || (id >= tw2868_get_device_num()))
        return -1;

    if(tw2868_dev[id].norm != norm) {
        switch(norm) {
            case VCAP_INPUT_NORM_PAL:
                width      = 704;
                height     = 576;
                frame_rate = 25;
                timeout    = DEFAULT_PAL_TIMEOUT;
                switch(mode[id]) {
                    case 0:  /* 27MHz */
                        pixel_clk = 27000000;
                        if(tw2868_video_get_mode(id) != TW2868_VMODE_PAL_720H_1CH)
                            ret = tw2868_video_set_mode(id, TW2868_VMODE_PAL_720H_1CH);
                        break;
                    case 1:  /* 54MHz ==> 27MHz Clock with 54MHz Data */
                        pixel_clk = 27000000;
                        if(tw2868_video_get_mode(id) != TW2868_VMODE_PAL_720H_2CH)
                            ret = tw2868_video_set_mode(id, TW2868_VMODE_PAL_720H_2CH);
                        break;
                    default: /* 108MHz */
                        pixel_clk = 108000000;
                        if(tw2868_video_get_mode(id) != TW2868_VMODE_PAL_720H_4CH)
                            ret = tw2868_video_set_mode(id, TW2868_VMODE_PAL_720H_4CH);
                        break;
                }
                break;
            default: /* NTSC */
                width      = 704;
                height     = 480;
                frame_rate = 30;
                timeout    = DEFAULT_NTSC_TIMEOUT;
                switch(mode[id]) {
                    case 0:  /* 27MHz */
                        pixel_clk = 27000000;
                        if(tw2868_video_get_mode(id) != TW2868_VMODE_NTSC_720H_1CH)
                            ret = tw2868_video_set_mode(id, TW2868_VMODE_NTSC_720H_1CH);
                        break;
                    case 1:  /* 54MHz ==> 27MHz Clock with 54MHz Data  */
                        pixel_clk = 27000000;
                        if(tw2868_video_get_mode(id) != TW2868_VMODE_NTSC_720H_2CH)
                            ret = tw2868_video_set_mode(id, TW2868_VMODE_NTSC_720H_2CH);
                        break;
                    default: /* 108MHz */
                        pixel_clk = 108000000;
                        if(tw2868_video_get_mode(id) != TW2868_VMODE_NTSC_720H_4CH)
                            ret = tw2868_video_set_mode(id, TW2868_VMODE_NTSC_720H_4CH);
                        break;
                }
                break;
        }
        if(ret < 0)
            goto end;

        /* Update device norm */
        tw2868_dev[id].norm = norm;
        for(i=0; i<DEV_VPORT_MAX; i++) {
            pinput = tw2868_dev[id].port[i];
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
        vcap_err("TW2868#%d init failed!\n", id);

    return ret;
}

static int tw2868_proc_norm_show(struct seq_file *sfile, void *v)
{
    struct tw2868_dev_t *pdev = (struct tw2868_dev_t *)sfile->private;
    char *norm_str[] = {"PAL", "NTSC"};

    down(&pdev->lock);

    seq_printf(sfile, "Norm => 0:%s, 1:%s\n", norm_str[0], norm_str[1]);
    seq_printf(sfile, "Current: %s\n", (pdev->norm>=0 && pdev->norm<=1) ? norm_str[pdev->norm] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static int tw2868_proc_timeout_show(struct seq_file *sfile, void *v)
{
    int i;
    u32 time_ms = 0;
    struct tw2868_dev_t *pdev = (struct tw2868_dev_t *)sfile->private;

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

static ssize_t tw2868_proc_norm_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, ret = 0;
    int  norm_value;
    struct vcap_input_dev_t *pinput;
    char value_str[8] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2868_dev_t *pdev = (struct tw2868_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &norm_value);

    down(&pdev->lock);

    /* check vport used or not? */
    for(i=0; i<DEV_VPORT_MAX; i++) {
        pinput = pdev->port[i];
        if(pinput && pinput->used) {
            vcap_err("TW2868#%d switch norm failed, device still in used!\n", pdev->index);
            ret = -EFAULT;
            goto end;
        }
    }

    if((norm_value <= 3) && (norm_value != norm[pdev->index])) {
        ret = tw2868_device_init(pdev->index, norm_value);
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

static ssize_t tw2868_proc_timeout_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, time_value;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2868_dev_t *pdev = (struct tw2868_dev_t *)sfile->private;

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

static int tw2868_proc_norm_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2868_proc_norm_show, PDE(inode)->data);
}

static int tw2868_proc_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2868_proc_timeout_show, PDE(inode)->data);
}

static struct file_operations tw2868_proc_norm_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2868_proc_norm_open,
    .write  = tw2868_proc_norm_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2868_proc_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2868_proc_timeout_open,
    .write  = tw2868_proc_timeout_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void tw2868_proc_remove(int id)
{
    if(tw2868_proc_root[id]) {
        if(tw2868_proc_timeout[id])
            vcap_input_proc_remove_entry(tw2868_proc_root[id], tw2868_proc_timeout[id]);

        if(tw2868_proc_norm[id])
            vcap_input_proc_remove_entry(tw2868_proc_root[id], tw2868_proc_norm[id]);

        vcap_input_proc_remove_entry(NULL, tw2868_proc_root[id]);
    }
}

static int tw2868_proc_init(struct tw2868_dev_t *pdev)
{
    int ret = 0;
    int id  = pdev->index;
    char name[32];

    /* root */
    sprintf(name, "tw2868.%d", id);
    tw2868_proc_root[id] = vcap_input_proc_create_entry(name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!tw2868_proc_root[id]) {
        vcap_err("create proc node '%s' failed!\n", name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2868_proc_root[id]->owner = THIS_MODULE;
#endif

    /* norm */
    tw2868_proc_norm[id] = vcap_input_proc_create_entry("norm", S_IRUGO|S_IXUGO, tw2868_proc_root[id]);
    if(!tw2868_proc_norm[id]) {
        vcap_err("create proc node '%s/norm' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    tw2868_proc_norm[id]->proc_fops = &tw2868_proc_norm_ops;
    tw2868_proc_norm[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2868_proc_norm[id]->owner     = THIS_MODULE;
#endif

    /* timeout */
    tw2868_proc_timeout[id] = vcap_input_proc_create_entry("timeout", S_IRUGO|S_IXUGO, tw2868_proc_root[id]);
    if(!tw2868_proc_timeout[id]) {
        vcap_err("create proc node '%s/timeout' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    tw2868_proc_timeout[id]->proc_fops = &tw2868_proc_timeout_ops;
    tw2868_proc_timeout[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2868_proc_timeout[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    tw2868_proc_remove(id);
    return ret;
}

static int tw2868_vlos_notify_handler(int id, int vin, int vlos)
{
    int i, j, vch;
    struct tw2868_dev_t *pdev = &tw2868_dev[id];

    if((id >= DEV_MAX) || (vin >= DEV_CH_MAX))
        return -1;

    down(&pdev->lock);

    if(pdev->vlos[vin] != vlos) {
        pdev->vlos[vin] = vlos;

        vch = tw2868_vin_to_vch(id, vin);
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

static int __init tw2868_input_init(void)
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
    memset(tw2868_dev, 0, sizeof(struct tw2868_dev_t)*DEV_MAX);

    /* Get TW2868 Device Number */
    dev_num = tw2868_get_device_num();

    for(i=0; i<DEV_MAX; i++) {
        tw2868_dev[i].index = i;
        tw2868_dev[i].norm  = -1;  ///< init value

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&tw2868_dev[i].lock, 1);
#else
        init_MUTEX(&tw2868_dev[i].lock);
#endif
        if(i >= dev_num)
            continue;

        if(vport[i]) {
            /* register input device */
            for(j=0; j<DEV_VPORT_MAX; j++) {
                if((vport[i]>>(4*j)) & 0xf) {   ///< VPort# Connect to X_CAP#
                    pinput = tw2868_dev[i].port[j] = kzalloc(sizeof(struct vcap_input_dev_t), GFP_KERNEL);
                    if(pinput == NULL) {
                        vcap_err("TW2868#%d.%d allocate vcap_input_dev_t failed\n", i, j);
                        ret = -ENOMEM;
                        goto err;
                    }

                    /* input device name */
                    snprintf(pinput->name, VCAP_INPUT_NAME_SIZE-1, "tw2868.%d.%d", i, j);

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
                    pinput->init        = tw2868_device_init;
                    pinput->norm.mode   = -1;                            ///< init value
                    pinput->module_get  = tw2868_module_get;
                    pinput->module_put  = tw2868_module_put;

                    switch(mode[i]) {
                        case 0:  /* 27MHz/36MHz */
                            pinput->mode     = VCAP_INPUT_MODE_BYPASS;
                            pinput->ch_id[0] = tw2868_get_vch_id(i, j, 0);
                            break;
                        case 1:  /* 54MHz/72MHz ==> 27MHz/36MHz Clock with 54MHz/72MHz Data */
                            pinput->mode     = VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE;
                            pinput->ch_id[0] = tw2868_get_vch_id(i, j, 0);
                            pinput->ch_id[2] = tw2868_get_vch_id(i, j, 1);
                            break;
                        default: /* 108MHz/144MHz */
                            pinput->mode     = VCAP_INPUT_MODE_4CH_BYTE_INTERLEAVE;
                            pinput->ch_id[0] = tw2868_get_vch_id(i, j, 0);
                            pinput->ch_id[1] = tw2868_get_vch_id(i, j, 1);
                            pinput->ch_id[2] = tw2868_get_vch_id(i, j, 2);
                            pinput->ch_id[3] = tw2868_get_vch_id(i, j, 3);
                            break;
                    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
                    sema_init(&pinput->sema_lock, 1);
#else
                    init_MUTEX(&pinput->sema_lock);
#endif
                    ret = vcap_input_device_register(pinput);
                    if(ret < 0) {
                        vcap_err("register TW2868#%d.%d input device failed\n", i, j);
                        goto err;
                    }
                }
            }

            /* device proc init */
            ret = tw2868_proc_init(&tw2868_dev[i]);
            if(ret < 0)
                goto err;

            /* device init */
            ret = tw2868_device_init(i, norm[i]);
            if(ret < 0)
                goto err;

            /* register tw2868 video loss notify handler */
            tw2868_notify_vlos_register(i, tw2868_vlos_notify_handler);

            vcap_info("Register TW2868#%d Input Device\n", i);
        }
    }

    return ret;

err:
    for(i=0; i<DEV_MAX; i++) {
        tw2868_notify_vlos_deregister(i);
        tw2868_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(tw2868_dev[i].port[j]) {
                vcap_input_device_unregister(tw2868_dev[i].port[j]);
                kfree(tw2868_dev[i].port[j]);
            }
        }
    }
    return ret;
}

static void __exit tw2868_input_exit(void)
{
    int i, j;

    for(i=0; i<DEV_MAX; i++) {
        tw2868_notify_vlos_deregister(i);
        tw2868_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(tw2868_dev[i].port[j]) {
                vcap_input_device_unregister(tw2868_dev[i].port[j]);
                kfree(tw2868_dev[i].port[j]);
            }
        }
    }
}

module_init(tw2868_input_init);
module_exit(tw2868_input_exit);

MODULE_DESCRIPTION("Grain Media Video Capture300 TW2868 Input Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
