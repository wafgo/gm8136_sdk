/**
 * @file dh9901.c
 * VCAP300 DH9901 Input Device Driver
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.2 $
 * $Date: 2014/07/08 06:08:00 $
 *
 * ChangeLog:
 *  $Log: dh9901.c,v $
 *  Revision 1.2  2014/07/08 06:08:00  jerson_l
 *  1. set timeout to 40ms if timeout value less then 40
 *
 *  Revision 1.1  2014/07/03 02:34:52  jerson_l
 *  1. support dh9901
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
#include "dahua/dh9901.h"       ///< from module/include/front_end/hdcvi

#define DEV_MAX                 DH9901_DEV_MAX
#define DEV_VPORT_MAX           DH9901_DEV_VOUT_MAX    ///< VOUT#0, VOUT#1, VOUT#2, VOUT#3
#define DEFAULT_TYPE            VCAP_INPUT_TYPE_CVI
#define DEFAULT_NORM_WIDTH      1280
#define DEFAULT_NORM_HEIGHT     720
#define DEFAULT_FRAME_RATE      25
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

struct dh9901_dev_t {
    int                     index;                        ///< device index
    struct semaphore        lock;                         ///< device locker
    int                     vfmt[DEV_VPORT_MAX];          ///< record current video format index
    int                     vlos[DEV_VPORT_MAX];          ///< record current video loss status
    int                     frame_rate[DEV_VPORT_MAX];    ///< record current video frame rate

    struct vcap_input_dev_t *port[DEV_VPORT_MAX];
};

static struct dh9901_dev_t    dh9901_dev[DEV_MAX];
static struct proc_dir_entry *dh9901_proc_root[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_norm[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_timeout[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = NULL};

static int dh9901_module_get(void)
{
    return try_module_get(THIS_MODULE);
}

static void dh9901_module_put(void)
{
    module_put(THIS_MODULE);
}

static int dh9901_vfmt_notify_handler(int id, int ch, struct dh9901_video_fmt_t *vfmt)
{
    int fps_div, norm_switch = 0;
    struct vcap_input_dev_t *pinput;
    struct dh9901_dev_t     *pdev = &dh9901_dev[id];

    if((id >= DEV_MAX) || (ch >= DH9901_DEV_CH_MAX) || !vfmt)
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
                        pinput->interface = VCAP_INPUT_INTF_BT656_INTERLACE;
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
                    case VCAP_INPUT_INTF_BT656_INTERLACE:
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

            if((vfmt->width > 0) && (vfmt->width != pinput->norm.width)) {
                pinput->norm.width = vfmt->width;
                norm_switch++;
            }

            if((vfmt->height > 0) && (vfmt->height != pinput->norm.height)) {
                pinput->norm.height = vfmt->height;
                norm_switch++;
            }

            if(norm_switch) {
                pinput->norm.mode++;    ///< update mode to inform norm switched
                vcap_input_device_notify(pinput->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
            }

            /* check frame rate */
            if(((vfmt->frame_rate/fps_div) > 0) && (pdev->frame_rate[ch] != (vfmt->frame_rate/fps_div))) {
                pdev->frame_rate[ch] = pinput->frame_rate = pinput->max_frame_rate = vfmt->frame_rate/fps_div;
                pinput->timeout_ms = (1000/pinput->frame_rate)*2;   ///< base on current frame rate
                if(pinput->timeout_ms < 40)
                    pinput->timeout_ms = 40;
                vcap_input_device_notify(pinput->vi_idx, 0, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
            }
        }
    }

    up(&pdev->lock);

    return 0;
}

static int dh9901_vlos_notify_handler(int id, int ch, int vlos)
{
    struct vcap_input_dev_t *pinput;
    struct dh9901_dev_t *pdev = &dh9901_dev[id];

    if((id >= DEV_MAX) || (ch >= DH9901_DEV_CH_MAX))
        return -1;

    down(&pdev->lock);

    if(pdev->port[ch]) {
        pinput = pdev->port[ch];

        if(pdev->vlos[ch] != vlos) {
            pdev->vlos[ch] = pinput->ch_vlos[0] = vlos;

            /* notify video loss status change */
            vcap_input_device_notify((pdev->port[ch])->vi_idx, 0, ((vlos == 0) ? VCAP_INPUT_NOTIFY_SIGNAL_PRESENT : VCAP_INPUT_NOTIFY_SIGNAL_LOSS));
        }
    }

    up(&pdev->lock);

    return 0;
}

static int dh9901_device_init(int id, int norm)
{
    int i, ret = 0;
    struct vcap_input_dev_t *pinput;

    if((id >= DEV_MAX) || (id >= dh9901_get_device_num()))
        return -1;

    /* Update input device norm */
    for(i=0; i<DEV_VPORT_MAX; i++) {
        pinput = dh9901_dev[id].port[i];
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

static int dh9901_proc_norm_show(struct seq_file *sfile, void *v)
{
    int i;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9901.%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "CVI   VCH   Width   Height\n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<DH9901_DEV_CH_MAX; i++) {
        if(pdev->port[i]) {
            seq_printf(sfile, "%-5d %-5d %-7d %-7d\n",
                       i,
                       dh9901_get_vch_id(pdev->index, i, i),
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

static int dh9901_proc_timeout_show(struct seq_file *sfile, void *v)
{
    int i;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9901.%d]\n", pdev->index);
    seq_printf(sfile, "-------------------\n");
    seq_printf(sfile, "CVI   VCH   Timeout\n");
    seq_printf(sfile, "-------------------\n");
    for(i=0; i<DH9901_DEV_CH_MAX; i++) {
        if(pdev->port[i]) {
            seq_printf(sfile, "%-5d %-5d %-7d\n",
                       i,
                       dh9901_get_vch_id(pdev->index, i, i),
                       (pdev->port[i])->timeout_ms);
        }
        else
            seq_printf(sfile, "%-5s %-5s %-7s\n", "-", "-", "-");
    }
    seq_printf(sfile, "\n");

    up(&pdev->lock);

    return 0;
}

static int dh9901_proc_norm_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_norm_show, PDE(inode)->data);
}

static int dh9901_proc_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_timeout_show, PDE(inode)->data);
}

static struct file_operations dh9901_proc_norm_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_norm_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_timeout_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void dh9901_proc_remove(int id)
{
    if(dh9901_proc_root[id]) {
        if(dh9901_proc_norm[id])
            vcap_input_proc_remove_entry(dh9901_proc_root[id], dh9901_proc_norm[id]);

        if(dh9901_proc_timeout[id])
            vcap_input_proc_remove_entry(dh9901_proc_root[id], dh9901_proc_timeout[id]);

        vcap_input_proc_remove_entry(NULL, dh9901_proc_root[id]);
    }
}

static int dh9901_proc_init(struct dh9901_dev_t *pdev)
{
    int ret = 0;
    int id  = pdev->index;
    char name[32];

    /* root */
    sprintf(name, "dh9901.%d", id);
    dh9901_proc_root[id] = vcap_input_proc_create_entry(name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!dh9901_proc_root[id]) {
        vcap_err("create proc node '%s' failed!\n", name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_root[id]->owner = THIS_MODULE;
#endif

    /* norm */
    dh9901_proc_norm[id] = vcap_input_proc_create_entry("norm", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_norm[id]) {
        vcap_err("create proc node '%s/norm' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_norm[id]->proc_fops = &dh9901_proc_norm_ops;
    dh9901_proc_norm[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_norm[id]->owner     = THIS_MODULE;
#endif

    /* timeout */
    dh9901_proc_timeout[id] = vcap_input_proc_create_entry("timeout", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_timeout[id]) {
        vcap_err("create proc node '%s/timeout' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_timeout[id]->proc_fops = &dh9901_proc_timeout_ops;
    dh9901_proc_timeout[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_timeout[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    dh9901_proc_remove(id);
    return ret;
}

static int __init dh9901_input_init(void)
{
    int i, j;
    int dev_num;
    struct vcap_input_dev_t *pinput;
    int ret = 0;

    if(vcap_input_get_version() < VCAP_INPUT_VERSION) {
        vcap_err("Input driver version(%08x) is not compatibility with vcap300_common.ko(%08x)!\n",
                 vcap_input_get_version(), VCAP_INPUT_VERSION);
        return -EFAULT;
    }

    /* clear buffer */
    memset(dh9901_dev, 0, sizeof(struct dh9901_dev_t)*DEV_MAX);

    /* Get DH9901 Device Number */
    dev_num = dh9901_get_device_num();

    for(i=0; i<DEV_MAX; i++) {
        dh9901_dev[i].index = i;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&dh9901_dev[i].lock, 1);
#else
        init_MUTEX(&dh9901_dev[i].lock);
#endif
        if(i >= dev_num)
            continue;

        if(vport[i]) {
            /* register input device */
            for(j=0; j<DEV_VPORT_MAX; j++) {
                if((vport[i]>>(4*j)) & 0xf) {   ///< VPort# Connect to X_CAP#
                    pinput = dh9901_dev[i].port[j] = kzalloc(sizeof(struct vcap_input_dev_t), GFP_KERNEL);
                    if(pinput == NULL) {
                        vcap_err("DH9901#%d.%d allocate vcap_input_dev_t failed\n", i, j);
                        ret = -ENOMEM;
                        goto err;
                    }

                    /* input device name */
                    snprintf(pinput->name, VCAP_INPUT_NAME_SIZE-1, "dh9901.%d.%d", i, j);

                    /* input device parameter setup */
                    pinput->index       = (i*DEV_VPORT_MAX) + j;
                    pinput->vi_idx      = ((vport[i]>>(4*j)) & 0xf) - 1; ///< VI#
                    pinput->vi_src      = ((vport[i]>>(4*j)) & 0xf) - 1; ///< X_CAP#
                    pinput->type        = DEFAULT_TYPE;
                    pinput->speed       = DEFAULT_SPEED;
                    pinput->interface   = (dh9901_get_vout_format(i) == DH9901_VOUT_FORMAT_BT1120) ? VCAP_INPUT_INTF_BT1120_PROGRESSIVE : VCAP_INPUT_INTF_SDI8BIT_PROGRESSIVE;
                    pinput->field_order = DEFAULT_ORDER;
                    pinput->data_range  = DEFAULT_DATA_RANGE;
                    pinput->mode        = DEFAULT_MODE;
                    pinput->yc_swap     = (yc_swap[i]>>(4*j)) & 0x1;
                    pinput->data_swap   = 0;
                    pinput->ch_id_mode  = 0;
                    pinput->inv_clk     = (inv_clk[i]>>(4*j)) & 0x1;
                    pinput->init        = dh9901_device_init;
                    pinput->module_get  = dh9901_module_get;
                    pinput->module_put  = dh9901_module_put;
                    pinput->norm.mode   = -1;                            ///< init value
                    pinput->ch_id[0]    = dh9901_get_vch_id(i, j, j);


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
                    sema_init(&pinput->sema_lock, 1);
#else
                    init_MUTEX(&pinput->sema_lock);
#endif
                    ret = vcap_input_device_register(pinput);
                    if(ret < 0) {
                        vcap_err("register DH9901#%d.%d input device failed\n", i, j);
                        goto err;
                    }
                }
            }

            /* device proc init */
            ret = dh9901_proc_init(&dh9901_dev[i]);
            if(ret < 0)
                goto err;

            /* device init */
            ret = dh9901_device_init(i, 0);
            if(ret < 0)
                goto err;

            /* register dh9901 video loss notify handler */
            dh9901_notify_vlos_register(i, dh9901_vlos_notify_handler);

            /* register dh9901 video foramt notify handler */
            dh9901_notify_vfmt_register(i, dh9901_vfmt_notify_handler);

            vcap_info("Register DH9901#%d Input Device\n", i);
        }
    }

    return ret;

err:
    for(i=0; i<DEV_MAX; i++) {
        dh9901_notify_vlos_deregister(i);
        dh9901_notify_vfmt_deregister(i);
        dh9901_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(dh9901_dev[i].port[j]) {
                vcap_input_device_unregister(dh9901_dev[i].port[j]);
                kfree(dh9901_dev[i].port[j]);
            }
        }
    }
    return ret;
}

static void __exit dh9901_input_exit(void)
{
    int i, j;

    for(i=0; i<DEV_MAX; i++) {
        dh9901_notify_vlos_deregister(i);
        dh9901_notify_vfmt_deregister(i);
        dh9901_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(dh9901_dev[i].port[j]) {
                vcap_input_device_unregister(dh9901_dev[i].port[j]);
                kfree(dh9901_dev[i].port[j]);
            }
        }
    }
}

module_init(dh9901_input_init);
module_exit(dh9901_input_exit);

MODULE_DESCRIPTION("Grain Media Video Capture300 DH9901 Input Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
