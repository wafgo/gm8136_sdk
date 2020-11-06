/**
 * @file cx25930.c
 * VCAP300 CX25930 Input Device Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.6 $
 * $Date: 2014/02/10 03:47:39 $
 *
 * ChangeLog:
 *  $Log: cx25930.c,v $
 *  Revision 1.6  2014/02/10 03:47:39  jerson_l
 *  1. fix frame_rate incorrect problem when 720P@60 from video loss to video present
 *
 *  Revision 1.5  2014/02/06 02:58:22  jerson_l
 *  1. modify version compare rule of input module driver
 *
 *  Revision 1.4  2014/02/06 02:14:04  jerson_l
 *  1. record channel vlos status when status notified
 *
 *  Revision 1.3  2014/01/22 04:10:05  jerson_l
 *  1. switch frame rate to 30fps when video loss
 *
 *  Revision 1.2  2013/11/27 05:38:47  jerson_l
 *  1. support status notify
 *
 *  Revision 1.1  2013/11/20 09:01:06  jerson_l
 *  1. support cx25930
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
#include "conexant/cx25930.h"   ///< from module/include/front_end/sdi

#define DEV_MAX                 CX25930_DEV_MAX
#define DEV_VPORT_MAX           CX25930_DEV_VOUT_MAX    ///< VOUT#0, VOUT#1, VOUT#2, VOUT#3
#define DEFAULT_TYPE            VCAP_INPUT_TYPE_SDI
#define DEFAULT_NORM_WIDTH      1920
#define DEFAULT_NORM_HEIGHT     1080
#define DEFAULT_FRAME_RATE      30
#define DEFAULT_MODE            VCAP_INPUT_MODE_BYPASS
#define DEFAULT_SPEED           VCAP_INPUT_SPEED_P
#define DEFAULT_ORDER           VCAP_INPUT_ORDER_ANYONE
#define DEFAULT_DATA_RANGE      VCAP_INPUT_DATA_RANGE_256
#define DEFAULT_TIMEOUT         ((1000/DEFAULT_FRAME_RATE)*2)

static ushort vport[DEV_MAX] = {0x1234, 0x5678, 0, 0};
module_param_array(vport, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vport, "VPort Link => VOUT0[0:3], VOUT1[3:7], VOUT2[8:11], VOUT3[12:15], 0:None, 1~8:X_CAP#0~7, 9:X_CAPCAS");

static int yc_swap[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 0x0000};
module_param_array(yc_swap, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(yc_swap, "0: None, 1:YC Swap, 2:CbCr Swap, 3:YC and CbCr Swap");

static int inv_clk[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 0x0000};
module_param_array(inv_clk, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "Invert Clock, 0:Disable, 1: Enable");

struct cx25930_dev_t {
    int                     index;                        ///< device index
    struct semaphore        lock;                         ///< device locker
    int                     vfmt[DEV_VPORT_MAX];          ///< record current video format index
    int                     vlos[DEV_VPORT_MAX];          ///< record current video loss status
    int                     frame_rate[DEV_VPORT_MAX];    ///< record current video frame rate
    int                     user_timeout[DEV_VPORT_MAX];  ///< record user specify channel timeout second
    struct vcap_input_dev_t *port[DEV_VPORT_MAX];
};

static struct cx25930_dev_t   cx25930_dev[DEV_MAX];
static struct proc_dir_entry *cx25930_proc_root[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx25930_proc_norm[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *cx25930_proc_timeout[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = NULL};

static int cx25930_module_get(void)
{
    return try_module_get(THIS_MODULE);
}

static void cx25930_module_put(void)
{
    module_put(THIS_MODULE);
}

static int cx25930_vfmt_notify_handler(int id, int ch, struct cx25930_video_fmt_t *vfmt)
{
    int fps_div, norm_switch = 0;
    struct vcap_input_dev_t *pinput;
    struct cx25930_dev_t    *pdev = &cx25930_dev[id];

    if((id >= DEV_MAX) || (ch >= CX25930_DEV_CH_MAX) || !vfmt)
        return -1;

    down(&pdev->lock);

    if(pdev->port[ch]) {
        pinput = pdev->port[ch];

        /* check video format */
        if(pdev->vfmt[ch] != vfmt->fmt_idx) {
            pdev->vfmt[ch] = vfmt->fmt_idx;
            if(vfmt->prog == 0) {   ///< interlace
                fps_div = 2;
                switch(pinput->interface) {
                    case VCAP_INPUT_INTF_SDI8BIT_PROGRESSIVE:
                        pinput->interface = VCAP_INPUT_INTF_SDI8BIT_INTERLACE;
                        norm_switch++;
                        break;
                    case VCAP_INPUT_INTF_BT1120_PROGRESSIVE:
                        pinput->interface = VCAP_INPUT_INTF_BT1120_INTERLACE;
                        norm_switch++;
                        break;
                    default:
                        break;
                }

                if(pinput->speed != VCAP_INPUT_SPEED_I) {
                    pinput->speed       = VCAP_INPUT_SPEED_I;
                    pinput->field_order = VCAP_INPUT_ORDER_ODD_FIRST;
                }
            }
            else {
                fps_div = 1;
                switch(pinput->interface) {
                    case VCAP_INPUT_INTF_SDI8BIT_INTERLACE:
                        pinput->interface = VCAP_INPUT_INTF_SDI8BIT_PROGRESSIVE;
                        norm_switch++;
                        break;
                    case VCAP_INPUT_INTF_BT1120_INTERLACE:
                        pinput->interface = VCAP_INPUT_INTF_BT1120_PROGRESSIVE;
                        norm_switch++;
                        break;
                    default:
                        break;
                }

                if(vfmt->frame_rate == 50 || vfmt->frame_rate == 60) {
                    if(pinput->speed != VCAP_INPUT_SPEED_2P) {
                        pinput->speed       = VCAP_INPUT_SPEED_2P;
                        pinput->field_order = VCAP_INPUT_ORDER_ANYONE;
                    }
                }
                else {
                    if(pinput->speed != VCAP_INPUT_SPEED_P) {
                        pinput->speed       = VCAP_INPUT_SPEED_P;
                        pinput->field_order = VCAP_INPUT_ORDER_ANYONE;
                    }
                }
            }

            if(pinput->pixel_clk != (vfmt->pixel_rate*1000))
                pinput->pixel_clk = vfmt->pixel_rate*1000;      ///< Hz

            if((vfmt->active_width > 0) && (vfmt->active_width != pinput->norm.width)) {
                pinput->norm.width = vfmt->active_width;
                norm_switch++;
            }

            if((vfmt->active_height > 0) && (vfmt->active_height != pinput->norm.height)) {
                pinput->norm.height = vfmt->active_height;
                norm_switch++;
            }

            if(norm_switch) {
                pinput->norm.mode++;    ///< update mode to inform norm switched
                vcap_input_device_notify(pinput->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
            }

            /* check frame rate */
            if(((vfmt->frame_rate/fps_div) > 0) && (pdev->frame_rate[ch] != (vfmt->frame_rate/fps_div))) {
                pdev->frame_rate[ch] = pinput->frame_rate = pinput->max_frame_rate = vfmt->frame_rate/fps_div;
                if(pdev->user_timeout[ch] <= 0)
                    pinput->timeout_ms = (1000/pinput->frame_rate)*2;   ///< base on current frame rate
                vcap_input_device_notify(pinput->vi_idx, 0, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
            }
        }
    }

    up(&pdev->lock);

    return 0;
}

static int cx25930_vlos_notify_handler(int id, int ch, int vlos)
{
    struct vcap_input_dev_t *pinput;
    struct cx25930_dev_t *pdev = &cx25930_dev[id];

    if((id >= DEV_MAX) || (ch >= CX25930_DEV_CH_MAX))
        return -1;

    down(&pdev->lock);

    if(pdev->port[ch]) {
        pinput = pdev->port[ch];

        if(pdev->vlos[ch] != vlos) {
            pdev->vlos[ch] = pinput->ch_vlos[0] = vlos;

            /* clear video format index when video loss */
            if(vlos)
                pdev->vfmt[ch] = 0;

            /* notify video loss status change */
            vcap_input_device_notify(pinput->vi_idx, 0, ((vlos == 0) ? VCAP_INPUT_NOTIFY_SIGNAL_PRESENT : VCAP_INPUT_NOTIFY_SIGNAL_LOSS));

            /* frame rate will switch to 30fps when video loss */
            if((pdev->frame_rate[ch] != DEFAULT_FRAME_RATE) && vlos) {
                pdev->frame_rate[ch] = pinput->frame_rate = pinput->max_frame_rate = DEFAULT_FRAME_RATE;
                if(pdev->user_timeout[ch] <= 0)
                    pinput->timeout_ms = (1000/pinput->frame_rate)*2;   ///< base on current frame rate
                vcap_input_device_notify(pinput->vi_idx, 0, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
            }
        }
    }

    up(&pdev->lock);

    return 0;
}

static int cx25930_device_init(int id, int norm)
{
    int i, ret = 0;
    struct vcap_input_dev_t *pinput;

    if((id >= DEV_MAX) || (id >= cx25930_get_device_num()))
        return -1;

    /* Update input device norm */
    for(i=0; i<DEV_VPORT_MAX; i++) {
        pinput = cx25930_dev[id].port[i];
        if(pinput && (pinput->norm.mode != norm)) {
            pinput->norm.mode   = norm;
            pinput->norm.width  = DEFAULT_NORM_WIDTH;
            pinput->norm.height = DEFAULT_NORM_HEIGHT;
            pinput->frame_rate  = pinput->max_frame_rate = DEFAULT_FRAME_RATE;
            pinput->timeout_ms  = DEFAULT_TIMEOUT;
        }
    }

    return ret;
}

static int cx25930_proc_norm_show(struct seq_file *sfile, void *v)
{
    int i;
    struct cx25930_dev_t *pdev = (struct cx25930_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[CX25930.%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "SDI   VCH   Width   Height\n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<CX25930_DEV_CH_MAX; i++) {
        if(pdev->port[i]) {
            seq_printf(sfile, "%-5d %-5d %-7d %-7d\n",
                       i,
                       cx25930_get_vch_id(pdev->index, i, i),
                       (pdev->port[i])->norm.width,
                       (pdev->port[i])->norm.height);
        }
        else
            seq_printf(sfile, "%-5s %-5s %-7s %-7s\n", "-", "-", "-", "-");
    }
    seq_printf(sfile, "\n");

    up(&pdev->lock);

    return 0;
}

static int cx25930_proc_timeout_show(struct seq_file *sfile, void *v)
{
    int i;
    struct cx25930_dev_t *pdev = (struct cx25930_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[CX25930.%d]\n", pdev->index);
    seq_printf(sfile, "-------------------\n");
    seq_printf(sfile, "SDI   VCH   Timeout\n");
    seq_printf(sfile, "-------------------\n");
    for(i=0; i<CX25930_DEV_CH_MAX; i++) {
        if(pdev->port[i]) {
            seq_printf(sfile, "%-5d %-5d %-7d\n",
                       i,
                       cx25930_get_vch_id(pdev->index, i, i),
                       (pdev->port[i])->timeout_ms);
        }
        else
            seq_printf(sfile, "%-5s %-5s %-7s\n", "-", "-", "-");
    }
    seq_printf(sfile, "\n");

    up(&pdev->lock);

    return 0;
}

static ssize_t cx25930_proc_timeout_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, ret = 0;
    int  sdi_ch, time_value;
    char value_str[64] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct cx25930_dev_t *pdev = (struct cx25930_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &sdi_ch, &time_value);

    down(&pdev->lock);

    if(sdi_ch >= 0) {
        for(i=0; i<CX25930_DEV_CH_MAX; i++) {
            if((sdi_ch == i) || (sdi_ch >= CX25930_DEV_CH_MAX)) {
                if(pdev->port[i] && ((pdev->port[i])->timeout_ms != time_value)) {
                    if(time_value > 0)
                        pdev->user_timeout[i] = (pdev->port[i])->timeout_ms = time_value;
                    else {
                        pdev->user_timeout[i] = 0;
                        (pdev->port[i])->timeout_ms = (1000/(pdev->port[i])->frame_rate)*2; ///< base on current frame rate
                    }
                }
            }
        }
    }

    up(&pdev->lock);

    if(ret)
        return ret;

    return count;
}

static int cx25930_proc_norm_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx25930_proc_norm_show, PDE(inode)->data);
}

static int cx25930_proc_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, cx25930_proc_timeout_show, PDE(inode)->data);
}

static struct file_operations cx25930_proc_norm_ops = {
    .owner  = THIS_MODULE,
    .open   = cx25930_proc_norm_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations cx25930_proc_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = cx25930_proc_timeout_open,
    .write  = cx25930_proc_timeout_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void cx25930_proc_remove(int id)
{
    if(cx25930_proc_root[id]) {
        if(cx25930_proc_norm[id])
            vcap_input_proc_remove_entry(cx25930_proc_root[id], cx25930_proc_norm[id]);

        if(cx25930_proc_timeout[id])
            vcap_input_proc_remove_entry(cx25930_proc_root[id], cx25930_proc_timeout[id]);

        vcap_input_proc_remove_entry(NULL, cx25930_proc_root[id]);
    }
}

static int cx25930_proc_init(struct cx25930_dev_t *pdev)
{
    int ret = 0;
    int id  = pdev->index;
    char name[32];

    /* root */
    sprintf(name, "cx25930.%d", id);
    cx25930_proc_root[id] = vcap_input_proc_create_entry(name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!cx25930_proc_root[id]) {
        vcap_err("create proc node '%s' failed!\n", name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx25930_proc_root[id]->owner = THIS_MODULE;
#endif

    /* norm */
    cx25930_proc_norm[id] = vcap_input_proc_create_entry("norm", S_IRUGO|S_IXUGO, cx25930_proc_root[id]);
    if(!cx25930_proc_norm[id]) {
        vcap_err("create proc node '%s/norm' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    cx25930_proc_norm[id]->proc_fops = &cx25930_proc_norm_ops;
    cx25930_proc_norm[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx25930_proc_norm[id]->owner     = THIS_MODULE;
#endif

    /* timeout */
    cx25930_proc_timeout[id] = vcap_input_proc_create_entry("timeout", S_IRUGO|S_IXUGO, cx25930_proc_root[id]);
    if(!cx25930_proc_timeout[id]) {
        vcap_err("create proc node '%s/timeout' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    cx25930_proc_timeout[id]->proc_fops = &cx25930_proc_timeout_ops;
    cx25930_proc_timeout[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    cx25930_proc_timeout[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    cx25930_proc_remove(id);
    return ret;
}

static int __init cx25930_input_init(void)
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
    memset(cx25930_dev, 0, sizeof(struct cx25930_dev_t)*DEV_MAX);

    /* Get CX25930 Device Number */
    dev_num = cx25930_get_device_num();

    for(i=0; i<DEV_MAX; i++) {
        cx25930_dev[i].index = i;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&cx25930_dev[i].lock, 1);
#else
        init_MUTEX(&cx25930_dev[i].lock);
#endif
        if(i >= dev_num)
            continue;

        if(vport[i]) {
            /* register input device */
            for(j=0; j<DEV_VPORT_MAX; j++) {
                if((vport[i]>>(4*j)) & 0xf) {   ///< VPort# Connect to X_CAP#
                    pinput = cx25930_dev[i].port[j] = kzalloc(sizeof(struct vcap_input_dev_t), GFP_KERNEL);
                    if(pinput == NULL) {
                        vcap_err("CX25930#%d.%d allocate vcap_input_dev_t failed\n", i, j);
                        ret = -ENOMEM;
                        goto err;
                    }

                    /* input device name */
                    snprintf(pinput->name, VCAP_INPUT_NAME_SIZE-1, "cx25930.%d.%d", i, j);

                    /* input device parameter setup */
                    pinput->index       = (i*DEV_VPORT_MAX) + j;
                    pinput->vi_idx      = ((vport[i]>>(4*j)) & 0xf) - 1; ///< VI#
                    pinput->vi_src      = ((vport[i]>>(4*j)) & 0xf) - 1; ///< X_CAP#
                    pinput->type        = DEFAULT_TYPE;
                    pinput->speed       = DEFAULT_SPEED;
                    pinput->interface   = (cx25930_get_vout_format(i) == CX25930_VOUT_FORMAT_BT1120) ? VCAP_INPUT_INTF_BT1120_PROGRESSIVE : VCAP_INPUT_INTF_SDI8BIT_PROGRESSIVE;
                    pinput->field_order = DEFAULT_ORDER;
                    pinput->data_range  = DEFAULT_DATA_RANGE;
                    pinput->mode        = DEFAULT_MODE;
                    pinput->yc_swap     = (yc_swap[i]>>(4*j)) & 0x1;
                    pinput->data_swap   = 0;
                    pinput->ch_id_mode  = 0;
                    pinput->inv_clk     = (inv_clk[i]>>(4*j)) & 0x1;
                    pinput->init        = cx25930_device_init;
                    pinput->module_get  = cx25930_module_get;
                    pinput->module_put  = cx25930_module_put;
                    pinput->norm.mode   = -1;                            ///< init value
                    pinput->ch_id[0]    = cx25930_get_vch_id(i, j, j);


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
                    sema_init(&pinput->sema_lock, 1);
#else
                    init_MUTEX(&pinput->sema_lock);
#endif
                    ret = vcap_input_device_register(pinput);
                    if(ret < 0) {
                        vcap_err("register CX25930#%d.%d input device failed\n", i, j);
                        goto err;
                    }
                }
            }

            /* device proc init */
            ret = cx25930_proc_init(&cx25930_dev[i]);
            if(ret < 0)
                goto err;

            /* device init */
            ret = cx25930_device_init(i, 0);
            if(ret < 0)
                goto err;

            /* register cx25930 video loss notify handler */
            cx25930_notify_vlos_register(i, cx25930_vlos_notify_handler);

            /* register cx25930 video foramt notify handler */
            cx25930_notify_vfmt_register(i, cx25930_vfmt_notify_handler);

            vcap_info("Register CX25930#%d Input Device\n", i);
        }
    }

    return ret;

err:
    for(i=0; i<DEV_MAX; i++) {
        cx25930_notify_vlos_deregister(i);
        cx25930_notify_vfmt_deregister(i);
        cx25930_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(cx25930_dev[i].port[j]) {
                vcap_input_device_unregister(cx25930_dev[i].port[j]);
                kfree(cx25930_dev[i].port[j]);
            }
        }
    }
    return ret;
}

static void __exit cx25930_input_exit(void)
{
    int i, j;

    for(i=0; i<DEV_MAX; i++) {
        cx25930_notify_vlos_deregister(i);
        cx25930_notify_vfmt_deregister(i);
        cx25930_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(cx25930_dev[i].port[j]) {
                vcap_input_device_unregister(cx25930_dev[i].port[j]);
                kfree(cx25930_dev[i].port[j]);
            }
        }
    }
}

module_init(cx25930_input_init);
module_exit(cx25930_input_exit);

MODULE_DESCRIPTION("Grain Media Video Capture300 CX25930 Input Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
