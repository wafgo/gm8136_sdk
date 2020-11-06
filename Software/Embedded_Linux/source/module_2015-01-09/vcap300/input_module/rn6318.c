/**
 * @file rn6318.c
 * VCAP300 RN6318 Input Device Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.6 $
 * $Date: 2014/07/24 02:06:01 $
 *
 * ChangeLog:
 *  $Log: rn6318.c,v $
 *  Revision 1.6  2014/07/24 02:06:01  shawn_hu
 *  1. add IOCTL and proc node for set sharpness and video mode
 *
 *  Revision 1.5  2014/03/31 02:33:06  jerson_l
 *  1. remove un-used vcap_i2c.h header file include
 *
 *  Revision 1.4  2014/03/11 14:13:04  jerson_l
 *  1. add data_swap module parameter
 *
 *  Revision 1.3  2014/02/06 02:58:22  jerson_l
 *  1. modify version compare rule of input module driver
 *
 *  Revision 1.2  2014/02/06 02:14:05  jerson_l
 *  1. record channel vlos status when status notified
 *
 *  Revision 1.1  2013/12/03 10:02:46  jerson_l
 *  1. support richnex rn6318
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
#include "richnex/rn6318.h"     ///< from module/include/av_decoder

#define DEV_MAX                 RN6318_DEV_MAX
#define DEV_VPORT_MAX           RN6318_DEV_VPORT_MAX            ///< VPORT_1, VPORT_3
#define DEV_CH_MAX              RN6318_DEV_CH_MAX
#define DEFAULT_TYPE            VCAP_INPUT_TYPE_DECODER
#define DEFAULT_SPEED           VCAP_INPUT_SPEED_I
#define DEFAULT_INTERFACE       VCAP_INPUT_INTF_BT656_INTERLACE
#define DEFAULT_NORM            VCAP_INPUT_NORM_NTSC
#define DEFAULT_DATA_RANGE      VCAP_INPUT_DATA_RANGE_256
#define DEFAULT_FIELD_ORDER     VCAP_INPUT_ORDER_ODD_FIRST
#define DEFAULT_CH_ID_MODE      VCAP_INPUT_CH_ID_EAV_SAV
#define DEFAULT_NTSC_TIMEOUT    66                               ///< 33(ms)*2(frame), 2 frame delay time
#define DEFAULT_PAL_TIMEOUT     80                               ///< 40(ms)*2(frame), 2 frame delay time

static ushort vport[DEV_MAX] = {0x0012, 0x0034};
module_param_array(vport, short, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vport, "VPort Link => VPort1[0:3], VPort3[3:7], 0:None, 1~8:X_CAP#0~7, 9:X_CAPCAS");

static int mode[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 2};
module_param_array(mode, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mode, "Video Output Mode=> 0:1CH, 1:2CH, 2:4CH");

static int norm[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = DEFAULT_NORM};
module_param_array(norm, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(norm, "Video Norm=> 0:PAL, 1:NTSC, 2:960H_PAL, 3:960H_NTSC");

static int order[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = DEFAULT_FIELD_ORDER};
module_param_array(order, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(order, "Field Order=> 0:Anyone, 1:Odd First, 2:Even First");

static int inv_clk[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 0};
module_param_array(inv_clk, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "Invert Clock, 0:Disable, 1: Enable");

static int data_swap[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 0};
module_param_array(data_swap, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(data_swap, "0: None, 1:Lo8Bit, 2:Byte, 3:Lo8Bit+Byte");

struct rn6318_dev_t {
    int                     index;                  ///< device index
    struct semaphore        lock;                   ///< device locker
    int                     norm;                   ///< device operation norm mode
    int                     vmode;                  ///< device operation video mode
    int                     vlos[DEV_CH_MAX];
    struct vcap_input_dev_t *port[DEV_VPORT_MAX];
};

static struct rn6318_dev_t    rn6318_dev[DEV_MAX];
static struct proc_dir_entry *rn6318_proc_root[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6318_proc_norm[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *rn6318_proc_timeout[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = NULL};

/****************************************************************************************
 *  RN6318 VPort and X_CAP# Port Mapping for GM8210/GM8287 Socket Board
 *
 *  RN6318#0(0x58) --->VPort1 ---- [VIN#4,5,6,7] 108/144MHz ---> X_CAP#3
 *                 --->VPort3 ---- [VIN#0,1,2,3] 108/144MHz ---> X_CAP#2
 *
 *  RN6318#1(0x5c) --->VPort1 ---- [VIN#4,5,6,7] 108/144MHz ---> X_CAP#1
 *                 --->VPort3 ---- [VIN#0,1,2,3] 108/144MHz ---> X_CAP#0
 *
 *****************************************************************************************/

static int rn6318_module_get(void)
{
    return try_module_get(THIS_MODULE);
}

static void rn6318_module_put(void)
{
    module_put(THIS_MODULE);
}

static int rn6318_device_init(int id, int norm)
{
    int i, ret = 0;
    int vmode, width, height, frame_rate, pixel_clk;
    u32 timeout;
    struct vcap_input_dev_t *pinput;

    if((id >= DEV_MAX) || (id >= rn6318_get_device_num()))
        return -1;

    if(rn6318_dev[id].norm != norm) {
        switch(norm) {
            case VCAP_INPUT_NORM_PAL:
                width      = 720;
                height     = 576;
                frame_rate = 25;
                timeout    = DEFAULT_PAL_TIMEOUT;
                switch(mode[id]) {
                    case 0:  /* 27MHz */
                        vmode     = RN6318_VMODE_PAL_720H_1CH;
                        pixel_clk = 27000000;
                        break;
                    case 1:  /* 54MHz ==> 27MHz Clock with 54MHz Data */
                        vmode     = RN6318_VMODE_PAL_720H_2CH;
                        pixel_clk = 27000000;
                        break;
                    default: /* 108MHz */
                        vmode     = RN6318_VMODE_PAL_720H_4CH;
                        pixel_clk = 108000000;
                        break;
                }
                break;
            case VCAP_INPUT_NORM_PAL_960H:
                width      = 960;
                height     = 576;
                frame_rate = 25;
                timeout    = DEFAULT_PAL_TIMEOUT;
                switch(mode[id]) {
                    case 0:  /* 36MHz */
                        vmode     = RN6318_VMODE_PAL_960H_1CH;
                        pixel_clk = 36000000;
                        break;
                    case 1:  /* 72MHz ==> 36MHz Clock with 72MHz Data  */
                        vmode     = RN6318_VMODE_PAL_960H_2CH;
                        pixel_clk = 36000000;
                        break;
                    default: /* 144MHz */
                        vmode     = RN6318_VMODE_PAL_960H_4CH;
                        pixel_clk = 144000000;
                        break;
                }
                break;
            case VCAP_INPUT_NORM_NTSC_960H:
                width      = 960;
                height     = 480;
                frame_rate = 30;
                timeout    = DEFAULT_NTSC_TIMEOUT;
                switch(mode[id]) {
                    case 0:  /* 36MHz */
                        vmode     = RN6318_VMODE_NTSC_960H_1CH;
                        pixel_clk = 36000000;
                        break;
                    case 1:  /* 72MHz ==> 36MHz Clock with 72MHz Data  */
                        vmode     = RN6318_VMODE_NTSC_960H_2CH;
                        pixel_clk = 36000000;
                        break;
                    default: /* 144MHz */
                        vmode     = RN6318_VMODE_NTSC_960H_4CH;
                        pixel_clk = 144000000;
                        break;
                }
                break;
            default: /* NTSC */
                width      = 720;
                height     = 480;
                frame_rate = 30;
                timeout    = DEFAULT_NTSC_TIMEOUT;
                switch(mode[id]) {
                    case 0:  /* 27MHz */
                        vmode     = RN6318_VMODE_NTSC_720H_1CH;
                        pixel_clk = 27000000;
                        break;
                    case 1:  /* 54MHz ==> 27MHz Clock with 54MHz Data  */
                        vmode     = RN6318_VMODE_NTSC_720H_2CH;
                        pixel_clk = 27000000;
                        break;
                    default: /* 108MHz */
                        vmode     = RN6318_VMODE_NTSC_720H_4CH;
                        pixel_clk = 108000000;
                        break;
                }
                break;
        }

        /* Update device norm */
        rn6318_dev[id].norm = norm;
        rn6318_dev[id].vmode = vmode;
        for(i=0; i<DEV_VPORT_MAX; i++) {
            pinput = rn6318_dev[id].port[i];
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

    if(ret < 0)
        vcap_err("RN6318#%d init failed!\n", id);

    return ret;
}

static int rn6318_proc_norm_show(struct seq_file *sfile, void *v)
{
    struct rn6318_dev_t *pdev = (struct rn6318_dev_t *)sfile->private;
    char *norm_str[] = {"PAL", "NTSC", "960H_PAL", "960H_NTSC"};

    down(&pdev->lock);

    seq_printf(sfile, "Norm => 0:%s, 1:%s, 2:%s, 3:%s\n", norm_str[0], norm_str[1], norm_str[2], norm_str[3]);
    seq_printf(sfile, "Current: %s\n", (pdev->norm>=0 && pdev->norm<=3) ? norm_str[pdev->norm] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static int rn6318_proc_timeout_show(struct seq_file *sfile, void *v)
{
    int i;
    u32 time_ms = 0;
    struct rn6318_dev_t *pdev = (struct rn6318_dev_t *)sfile->private;

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

static ssize_t rn6318_proc_norm_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ret = 0;
    int  norm_value;
    char value_str[8] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct rn6318_dev_t *pdev = (struct rn6318_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &norm_value);

    down(&pdev->lock);

    if((norm_value <= 3) && (norm_value != pdev->norm)) {
        ret = rn6318_device_init(pdev->index, norm_value);
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

static ssize_t rn6318_proc_timeout_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, time_value;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct rn6318_dev_t *pdev = (struct rn6318_dev_t *)sfile->private;

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

static int rn6318_proc_norm_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6318_proc_norm_show, PDE(inode)->data);
}

static int rn6318_proc_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, rn6318_proc_timeout_show, PDE(inode)->data);
}

static struct file_operations rn6318_proc_norm_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6318_proc_norm_open,
    .write  = rn6318_proc_norm_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations rn6318_proc_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = rn6318_proc_timeout_open,
    .write  = rn6318_proc_timeout_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void rn6318_proc_remove(int id)
{
    if(rn6318_proc_root[id]) {
        if(rn6318_proc_timeout[id])
            vcap_input_proc_remove_entry(rn6318_proc_root[id], rn6318_proc_timeout[id]);

        if(rn6318_proc_norm[id])
            vcap_input_proc_remove_entry(rn6318_proc_root[id], rn6318_proc_norm[id]);

        vcap_input_proc_remove_entry(NULL, rn6318_proc_root[id]);
    }
}

static int rn6318_proc_init(struct rn6318_dev_t *pdev)
{
    int ret = 0;
    int id  = pdev->index;
    char name[32];

    /* root */
    sprintf(name, "rn6318.%d", id);
    rn6318_proc_root[id] = vcap_input_proc_create_entry(name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!rn6318_proc_root[id]) {
        vcap_err("create proc node '%s' failed!\n", name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6318_proc_root[id]->owner = THIS_MODULE;
#endif

    /* norm */
    rn6318_proc_norm[id] = vcap_input_proc_create_entry("norm", S_IRUGO|S_IXUGO, rn6318_proc_root[id]);
    if(!rn6318_proc_norm[id]) {
        vcap_err("create proc node '%s/norm' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    rn6318_proc_norm[id]->proc_fops = &rn6318_proc_norm_ops;
    rn6318_proc_norm[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6318_proc_norm[id]->owner     = THIS_MODULE;
#endif

    /* timeout */
    rn6318_proc_timeout[id] = vcap_input_proc_create_entry("timeout", S_IRUGO|S_IXUGO, rn6318_proc_root[id]);
    if(!rn6318_proc_timeout[id]) {
        vcap_err("create proc node '%s/timeout' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    rn6318_proc_timeout[id]->proc_fops = &rn6318_proc_timeout_ops;
    rn6318_proc_timeout[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    rn6318_proc_timeout[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    rn6318_proc_remove(id);
    return ret;
}

static int rn6318_vlos_notify_handler(int id, int vin, int vlos)
{
    int i, j, vch;
    struct rn6318_dev_t *pdev = &rn6318_dev[id];

    if((id >= DEV_MAX) || (vin >= DEV_CH_MAX))
        return -1;

    down(&pdev->lock);

    if(pdev->vlos[vin] != vlos) {
        pdev->vlos[vin] = vlos;

        vch = rn6318_vin_to_vch(id, vin);
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

static int rn6318_vfmt_notify_handler(int id, int vmode)
{
    int i, input_norm, ret;
    int port_norm[DEV_VPORT_MAX], frame_rate[DEV_VPORT_MAX];
    struct rn6318_dev_t *pdev = &rn6318_dev[id];

    if(id >= DEV_MAX)
        return -1;

    down(&pdev->lock);

    if(pdev->vmode != vmode) {
        switch(vmode) {
            case RN6318_VMODE_NTSC_720H_1CH:
            case RN6318_VMODE_NTSC_720H_2CH:
            case RN6318_VMODE_NTSC_720H_4CH:
                input_norm = VCAP_INPUT_NORM_NTSC;
                break;

            case RN6318_VMODE_NTSC_960H_1CH:
            case RN6318_VMODE_NTSC_960H_2CH:
            case RN6318_VMODE_NTSC_960H_4CH:
                input_norm = VCAP_INPUT_NORM_NTSC_960H;
                break;

            case RN6318_VMODE_PAL_720H_1CH:
            case RN6318_VMODE_PAL_720H_2CH:
            case RN6318_VMODE_PAL_720H_4CH:
                input_norm = VCAP_INPUT_NORM_PAL;
                break;
            case RN6318_VMODE_PAL_960H_1CH:
            case RN6318_VMODE_PAL_960H_2CH:
            case RN6318_VMODE_PAL_960H_4CH:
                input_norm = VCAP_INPUT_NORM_PAL_960H;
                break;
            default:
                goto exit;
        }

        /* store current norm and frame rate value for notify compare */
        for(i=0; i<DEV_VPORT_MAX; i++) {
            if(!pdev->port[i])
                continue;

            port_norm[i]  = (pdev->port[i])->norm.mode;
            frame_rate[i] = (pdev->port[i])->frame_rate;
        }

        ret = rn6318_device_init(id, input_norm);
        if(ret == 0) {
            for(i=0; i<DEV_VPORT_MAX; i++) {
                if(!pdev->port[i])
                    continue;

                switch((pdev->port[i])->mode) {
                    case VCAP_INPUT_MODE_BYPASS:
                        if(port_norm[i] != (pdev->port[i])->norm.mode) {
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                        }

                        if(frame_rate[i] != (pdev->port[i])->frame_rate) {
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 0, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                        }
                        break;
                    case VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE:
                        if(port_norm[i] != (pdev->port[i])->norm.mode) {
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 2, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                        }

                        if(frame_rate[i] != (pdev->port[i])->frame_rate) {
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 0, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 2, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                        }
                        break;
                    case VCAP_INPUT_MODE_4CH_BYTE_INTERLEAVE:
                        if(port_norm[i] != (pdev->port[i])->norm.mode) {
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 1, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 2, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 3, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                        }

                        if(frame_rate[i] != (pdev->port[i])->frame_rate) {
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 0, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 1, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 2, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 3, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }

exit:
    up(&pdev->lock);

    return 0;
}

static int __init rn6318_input_init(void)
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
    memset(rn6318_dev, 0, sizeof(struct rn6318_dev_t)*DEV_MAX);

    /* Get RN6318 Device Number */
    dev_num = rn6318_get_device_num();

    for(i=0; i<DEV_MAX; i++) {
        rn6318_dev[i].index = i;
        rn6318_dev[i].norm  = -1;  ///< init value

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&rn6318_dev[i].lock, 1);
#else
        init_MUTEX(&rn6318_dev[i].lock);
#endif
        if(i >= dev_num)
            continue;

        if(vport[i]) {
            /* register input device */
            for(j=0; j<DEV_VPORT_MAX; j++) {
                if((vport[i]>>(4*j)) & 0xf) {   ///< VPort# Connect to X_CAP#
                    pinput = rn6318_dev[i].port[j] = kzalloc(sizeof(struct vcap_input_dev_t), GFP_KERNEL);
                    if(pinput == NULL) {
                        vcap_err("RN6318#%d.%d allocate vcap_input_dev_t failed\n", i, j);
                        ret = -ENOMEM;
                        goto err;
                    }

                    /* input device name */
                    snprintf(pinput->name, VCAP_INPUT_NAME_SIZE-1, "rn6318.%d.%d", i, j);

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
                    pinput->data_swap   = data_swap[i];
                    pinput->ch_id_mode  = DEFAULT_CH_ID_MODE;
                    pinput->inv_clk     = inv_clk[i];
                    pinput->init        = rn6318_device_init;
                    pinput->norm.mode   = -1;                            ///< init value
                    pinput->module_get  = rn6318_module_get;
                    pinput->module_put  = rn6318_module_put;

                    switch(mode[i]) {
                        case 0:  /* 27/36MHz */
                            pinput->mode     = VCAP_INPUT_MODE_BYPASS;
                            pinput->ch_id[0] = rn6318_get_vch_id(i, j, 0);
                            break;
                        case 1:  /* 54/72MHz ==> 27/36MHz Clock with 54/72MHz Data */
                            pinput->mode     = VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE;
                            pinput->ch_id[0] = rn6318_get_vch_id(i, j, 0);
                            pinput->ch_id[2] = rn6318_get_vch_id(i, j, 1);
                            break;
                        default: /* 108/144MHz */
                            pinput->mode     = VCAP_INPUT_MODE_4CH_BYTE_INTERLEAVE;
                            pinput->ch_id[0] = rn6318_get_vch_id(i, j, 0);
                            pinput->ch_id[1] = rn6318_get_vch_id(i, j, 1);
                            pinput->ch_id[2] = rn6318_get_vch_id(i, j, 2);
                            pinput->ch_id[3] = rn6318_get_vch_id(i, j, 3);
                            break;
                    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
                    sema_init(&pinput->sema_lock, 1);
#else
                    init_MUTEX(&pinput->sema_lock);
#endif
                    ret = vcap_input_device_register(pinput);
                    if(ret < 0) {
                        vcap_err("register RN6318#%d.%d input device failed\n", i, j);
                        goto err;
                    }
                }
            }

            /* device proc init */
            ret = rn6318_proc_init(&rn6318_dev[i]);
            if(ret < 0)
                goto err;

            /* device init */
            ret = rn6318_device_init(i, norm[i]);
            if(ret < 0)
                goto err;

            /* register rn6318 video loss notify handler */
            rn6318_notify_vlos_register(i, rn6318_vlos_notify_handler);

            /* register rn6318 video format notify handler */
            rn6318_notify_vfmt_register(i, rn6318_vfmt_notify_handler);

            vcap_info("Register RN6318#%d Input Device\n", i);
        }
    }

    return ret;

err:
    for(i=0; i<DEV_MAX; i++) {
        rn6318_notify_vlos_deregister(i);
        rn6318_notify_vfmt_deregister(i);
        rn6318_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(rn6318_dev[i].port[j]) {
                vcap_input_device_unregister(rn6318_dev[i].port[j]);
                kfree(rn6318_dev[i].port[j]);
            }
        }
    }
    return ret;
}

static void __exit rn6318_input_exit(void)
{
    int i, j;

    for(i=0; i<DEV_MAX; i++) {
        rn6318_notify_vlos_deregister(i);
        rn6318_notify_vfmt_deregister(i);
        rn6318_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(rn6318_dev[i].port[j]) {
                vcap_input_device_unregister(rn6318_dev[i].port[j]);
                kfree(rn6318_dev[i].port[j]);
            }
        }
    }
}

module_init(rn6318_input_init);
module_exit(rn6318_input_exit);

MODULE_DESCRIPTION("Grain Media Video Capture300 RN6318 Input Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
