/**
 * @file nvp1118.c
 * VCAP300 NVP1118 Input Device Driver
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.14 $
 * $Date: 2014/07/15 08:00:29 $
 *
 * ChangeLog:
 *  $Log: nvp1118.c,v $
 *  Revision 1.14  2014/07/15 08:00:29  jerson_l
 *  1. modify mode module parameter help message
 *
 *  Revision 1.13  2014/06/17 08:00:07  jerson_l
 *  1. support video format change notify
 *
 *  Revision 1.12  2014/03/31 02:33:06  jerson_l
 *  1. remove un-used vcap_i2c.h header file include
 *
 *  Revision 1.11  2014/02/06 02:58:22  jerson_l
 *  1. modify version compare rule of input module driver
 *
 *  Revision 1.10  2014/02/06 02:14:05  jerson_l
 *  1. record channel vlos status when status notified
 *
 *  Revision 1.9  2013/11/27 05:38:47  jerson_l
 *  1. support status notify
 *
 *  Revision 1.8  2013/11/07 06:40:00  jerson_l
 *  1. support max_frame_rate infomation
 *
 *  Revision 1.7  2013/10/01 06:18:19  jerson_l
 *  1. support new channel mapping mechanism
 *
 *  Revision 1.6  2013/09/12 11:39:27  jerson_l
 *  1. switch default order to odd field first
 *
 *  Revision 1.5  2013/08/23 09:18:07  jerson_l
 *  1. support device timeout control proc node
 *
 *  Revision 1.4  2013/07/11 03:36:22  jerson_l
 *  1. modify video_set_mode control api
 *
 *  Revision 1.3  2013/03/19 06:08:56  jerson_l
 *  1. modify default field order to even field first
 *
 *  Revision 1.2  2013/02/07 12:33:14  jerson_l
 *  1. correct GM8210 EVB port mapping
 *
 *  Revision 1.1  2013/01/31 05:41:46  jerson_l
 *  1. add nvp1118 decoder support
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
#include "nextchip/nvp1118.h"       ///< from module/include/front_end/decoder

#define DEV_MAX                 NVP1118_DEV_MAX
#define DEV_VPORT_MAX           NVP1118_DEV_VPORT_MAX            ///< VPORT_A, VPORT_B, VPORT_C, VPORT_D
#define DEV_CH_MAX              NVP1118_DEV_CH_MAX
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
MODULE_PARM_DESC(mode, "Video Output Mode=> 0:1CH, 1:2CH, 2:4CH");

static int norm[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = DEFAULT_NORM};
module_param_array(norm, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(norm, "Video Norm=> 0:PAL, 1:NTSC");

static int order[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = DEFAULT_FIELD_ORDER};
module_param_array(order, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(order, "Field Order=> 0:Anyone, 1:Odd First, 2:Even First");

static int inv_clk[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 0};
module_param_array(inv_clk, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "Invert Clock, 0:Disable, 1: Enable");

struct nvp1118_dev_t {
    int                     index;                  ///< device index
    struct semaphore        lock;                   ///< device locker
    int                     norm;                   ///< device operation norm mode
    int                     vmode;                  ///< device operation video mode
    int                     vlos[DEV_CH_MAX];
    struct vcap_input_dev_t *port[DEV_VPORT_MAX];
};

static struct nvp1118_dev_t   nvp1118_dev[DEV_MAX];
static struct proc_dir_entry *nvp1118_proc_root[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *nvp1118_proc_norm[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *nvp1118_proc_timeout[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = NULL};

/****************************************************************************************
 *  NVP1118 VPort and X_CAP# Port Mapping for GM8210/GM8287 Socket Board
 *
 *  NVP1118#0(0x66) --->VPortA ---- [VIN#0,1,2,3] 108MHz ---> X_CAP#1
 *                  --->VPortB ---- [VIN#4,5,6,7] 108MHz ---> X_CAP#0
 *                  --->VPortC ---- X
 *                  --->VPortD ---- X
 *
 *  NVP1118#1(0x64) --->VPortA ---- [VIN#0,1,2,3] 108MHz ---> X_CAP#3
 *                  --->VPortB ---- [VIN#4,5,6,7] 108MHz ---> X_CAP#2
 *                  --->VPortC ---- X
 *                  --->VPortD ---- X
 *
 *  NVP1118#2(0x62) --->VPortA ---- [VIN#0,1,2,3] 108MHz ---> X_CAP#5
 *                  --->VPortB ---- [VIN#4,5,6,7] 108MHz ---> X_CAP#4
 *                  --->VPortC ---- X
 *                  --->VPortD ---- X
 *
 *  NVP1118#3(0x60) --->VPortA ---- [VIN#0,1,2,3] 108MHz ---> X_CAP#7
 *                  --->VPortB ---- [VIN#4,5,6,7] 108MHz ---> X_CAP#6
 *                  --->VPortC ---- X
 *                  --->VPortD ---- X
 *========================================================================================
 *  NVP1118 VPort and X_CAP# Port Mapping for GM8210 System Board
 *
 *  NVP1118#0(0x66) --->VPortA ---- [VIN#0,1,2,3] 108MHz ---> X_CAP#0
 *                  --->VPortB ---- [VIN#4,5,6,7] 108MHz ---> X_CAP#1
 *                  --->VPortC ---- X
 *                  --->VPortD ---- X
 *
 *  NVP1118#1(0x64) --->VPortA ---- [VIN#0,1,2,3] 108MHz ---> X_CAP#2
 *                  --->VPortB ---- [VIN#4,5,6,7] 108MHz ---> X_CAP#3
 *                  --->VPortC ---- X
 *                  --->VPortD ---- X
 *
 *  NVP1118#2(0x62) --->VPortA ---- [VIN#0,1,2,3] 108MHz ---> X_CAP#4
 *                  --->VPortB ---- [VIN#4,5,6,7] 108MHz ---> X_CAP#5
 *                  --->VPortC ---- X
 *                  --->VPortD ---- X
 *
 *  NVP1118#3(0x60) --->VPortA ---- [VIN#0,1,2,3] 108MHz ---> X_CAP#6
 *                  --->VPortB ---- [VIN#4,5,6,7] 108MHz ---> X_CAP#7
 *                  --->VPortC ---- X
 *                  --->VPortD ---- X
 *****************************************************************************************/

static int nvp1118_module_get(void)
{
    return try_module_get(THIS_MODULE);
}

static void nvp1118_module_put(void)
{
    module_put(THIS_MODULE);
}

static int nvp1118_device_init(int id, int norm)
{
    int i, ret = 0;
    int vmode, width, height, frame_rate, pixel_clk;
    u32 timeout;
    struct vcap_input_dev_t *pinput;

    if((id >= DEV_MAX) || (id >= nvp1118_get_device_num()))
        return -1;

    if(nvp1118_dev[id].norm != norm) {
        switch(norm) {
            case VCAP_INPUT_NORM_PAL:
                width      = 720;
                height     = 576;
                frame_rate = 25;
                timeout    = DEFAULT_PAL_TIMEOUT;
                switch(mode[id]) {
                    case 0:  /* 27MHz */
                        vmode     = NVP1118_VMODE_PAL_720H_1CH;
                        pixel_clk = 27000000;
                        break;
                    case 1:  /* 54MHz ==> 27MHz Clock with 54MHz Data */
                        vmode     = NVP1118_VMODE_PAL_720H_2CH;
                        pixel_clk = 27000000;
                        break;
                    default: /* 108MHz */
                        vmode     = NVP1118_VMODE_PAL_720H_4CH;
                        pixel_clk = 108000000;
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
                        vmode     = NVP1118_VMODE_NTSC_720H_1CH;
                        pixel_clk = 27000000;
                        break;
                    case 1:  /* 54MHz ==> 27MHz Clock with 54MHz Data */
                        vmode     = NVP1118_VMODE_NTSC_720H_2CH;
                        pixel_clk = 27000000;
                        break;
                    default: /* 108MHz */
                        vmode     = NVP1118_VMODE_NTSC_720H_4CH;
                        pixel_clk = 108000000;
                        break;
                }
                break;
        }
        if(ret < 0)
            goto end;

        /* Update device norm */
        nvp1118_dev[id].norm  = norm;
        nvp1118_dev[id].vmode = vmode;
        for(i=0; i<DEV_VPORT_MAX; i++) {
            pinput = nvp1118_dev[id].port[i];
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
        vcap_err("NVP1118#%d init failed!\n", id);

    return ret;
}

static int nvp1118_proc_norm_show(struct seq_file *sfile, void *v)
{
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;
    char *norm_str[] = {"PAL", "NTSC"};

    down(&pdev->lock);

    seq_printf(sfile, "Norm => 0:%s, 1:%s\n", norm_str[0], norm_str[1]);
    seq_printf(sfile, "Current: %s\n", (pdev->norm>=0 && pdev->norm<=1) ? norm_str[pdev->norm] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static int nvp1118_proc_timeout_show(struct seq_file *sfile, void *v)
{
    int i;
    u32 time_ms = 0;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

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

static ssize_t nvp1118_proc_norm_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ret = 0;
    int  norm_value;
    char value_str[8] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &norm_value);

    down(&pdev->lock);

    if((norm_value <= 1) && (norm_value != norm[pdev->index])) {
        ret = nvp1118_device_init(pdev->index, norm_value);
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

static ssize_t nvp1118_proc_timeout_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, time_value;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct nvp1118_dev_t *pdev = (struct nvp1118_dev_t *)sfile->private;

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

static int nvp1118_proc_norm_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvp1118_proc_norm_show, PDE(inode)->data);
}

static int nvp1118_proc_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvp1118_proc_timeout_show, PDE(inode)->data);
}

static struct file_operations nvp1118_proc_norm_ops = {
    .owner  = THIS_MODULE,
    .open   = nvp1118_proc_norm_open,
    .write  = nvp1118_proc_norm_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations nvp1118_proc_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = nvp1118_proc_timeout_open,
    .write  = nvp1118_proc_timeout_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void nvp1118_proc_remove(int id)
{
    if(nvp1118_proc_root[id]) {
        if(nvp1118_proc_timeout[id])
            vcap_input_proc_remove_entry(nvp1118_proc_root[id], nvp1118_proc_timeout[id]);

        if(nvp1118_proc_norm[id])
            vcap_input_proc_remove_entry(nvp1118_proc_root[id], nvp1118_proc_norm[id]);

        vcap_input_proc_remove_entry(NULL, nvp1118_proc_root[id]);
    }
}

static int nvp1118_proc_init(struct nvp1118_dev_t *pdev)
{
    int ret = 0;
    int id  = pdev->index;
    char name[32];

    /* root */
    sprintf(name, "nvp1118.%d", id);
    nvp1118_proc_root[id] = vcap_input_proc_create_entry(name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!nvp1118_proc_root[id]) {
        vcap_err("create proc node '%s' failed!\n", name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp1118_proc_root[id]->owner = THIS_MODULE;
#endif

    /* norm */
    nvp1118_proc_norm[id] = vcap_input_proc_create_entry("norm", S_IRUGO|S_IXUGO, nvp1118_proc_root[id]);
    if(!nvp1118_proc_norm[id]) {
        vcap_err("create proc node '%s/norm' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    nvp1118_proc_norm[id]->proc_fops = &nvp1118_proc_norm_ops;
    nvp1118_proc_norm[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp1118_proc_norm[id]->owner     = THIS_MODULE;
#endif

    /* timeout */
    nvp1118_proc_timeout[id] = vcap_input_proc_create_entry("timeout", S_IRUGO|S_IXUGO, nvp1118_proc_root[id]);
    if(!nvp1118_proc_timeout[id]) {
        vcap_err("create proc node '%s/timeout' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    nvp1118_proc_timeout[id]->proc_fops = &nvp1118_proc_timeout_ops;
    nvp1118_proc_timeout[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp1118_proc_timeout[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    nvp1118_proc_remove(id);
    return ret;
}

static int nvp1118_vlos_notify_handler(int id, int vin, int vlos)
{
    int i, j, vch;
    struct nvp1118_dev_t *pdev = &nvp1118_dev[id];

    if((id >= DEV_MAX) || (vin >= DEV_CH_MAX))
        return -1;

    down(&pdev->lock);

    if(pdev->vlos[vin] != vlos) {
        pdev->vlos[vin] = vlos;

        vch = nvp1118_vin_to_vch(id, vin);
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

static int nvp1118_vfmt_notify_handler(int id, int vmode)
{
    int i, input_norm, ret;
    int port_norm[DEV_VPORT_MAX], frame_rate[DEV_VPORT_MAX];
    struct nvp1118_dev_t *pdev = &nvp1118_dev[id];

    if(id >= DEV_MAX)
        return -1;

    down(&pdev->lock);

    if(pdev->vmode != vmode) {
        switch(vmode) {
            case NVP1118_VMODE_NTSC_720H_1CH:
            case NVP1118_VMODE_NTSC_720H_2CH:
            case NVP1118_VMODE_NTSC_720H_4CH:
                input_norm = VCAP_INPUT_NORM_NTSC;
                break;
            case NVP1118_VMODE_PAL_720H_1CH:
            case NVP1118_VMODE_PAL_720H_2CH:
            case NVP1118_VMODE_PAL_720H_4CH:
                input_norm = VCAP_INPUT_NORM_PAL;
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

        ret = nvp1118_device_init(id, input_norm);
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

static int __init nvp1118_input_init(void)
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
    memset(nvp1118_dev, 0, sizeof(struct nvp1118_dev_t)*DEV_MAX);

    /* Get NVP1118 Device Number */
    dev_num = nvp1118_get_device_num();

    for(i=0; i<DEV_MAX; i++) {
        nvp1118_dev[i].index = i;
        nvp1118_dev[i].norm  = -1;  ///< init value

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&nvp1118_dev[i].lock, 1);
#else
        init_MUTEX(&nvp1118_dev[i].lock);
#endif
        if(i >= dev_num)
            continue;

        if(vport[i]) {
            /* register input device */
            for(j=0; j<DEV_VPORT_MAX; j++) {
                if((vport[i]>>(4*j)) & 0xf) {   ///< VPort# Connect to X_CAP#
                    pinput = nvp1118_dev[i].port[j] = kzalloc(sizeof(struct vcap_input_dev_t), GFP_KERNEL);
                    if(pinput == NULL) {
                        vcap_err("NVP1118#%d.%d allocate vcap_input_dev_t failed\n", i, j);
                        ret = -ENOMEM;
                        goto err;
                    }

                    /* input device name */
                    snprintf(pinput->name, VCAP_INPUT_NAME_SIZE-1, "nvp1118.%d.%d", i, j);

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
                    pinput->init        = nvp1118_device_init;
                    pinput->norm.mode   = -1;                            ///< init value
                    pinput->module_get  = nvp1118_module_get;
                    pinput->module_put  = nvp1118_module_put;

                    switch(mode[i]) {
                        case 0:  /* 27MHz */
                            pinput->mode     = VCAP_INPUT_MODE_BYPASS;
                            pinput->ch_id[0] = nvp1118_get_vch_id(i, j, 0);
                            break;
                        case 1:  /* 54MHz ==> 27MHz Clock with 54MHz Data */
                            pinput->mode     = VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE;
                            pinput->ch_id[0] = nvp1118_get_vch_id(i, j, 0);
                            pinput->ch_id[2] = nvp1118_get_vch_id(i, j, 1);
                            break;
                        default: /* 108MHz */
                            pinput->mode     = VCAP_INPUT_MODE_4CH_BYTE_INTERLEAVE;
                            pinput->ch_id[0] = nvp1118_get_vch_id(i, j, 0);
                            pinput->ch_id[1] = nvp1118_get_vch_id(i, j, 1);
                            pinput->ch_id[2] = nvp1118_get_vch_id(i, j, 2);
                            pinput->ch_id[3] = nvp1118_get_vch_id(i, j, 3);
                            break;
                    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
                    sema_init(&pinput->sema_lock, 1);
#else
                    init_MUTEX(&pinput->sema_lock);
#endif
                    ret = vcap_input_device_register(pinput);
                    if(ret < 0) {
                        vcap_err("register NVP1118#%d.%d input device failed\n", i, j);
                        goto err;
                    }
                }
            }

            /* device proc init */
            ret = nvp1118_proc_init(&nvp1118_dev[i]);
            if(ret < 0)
                goto err;

            /* device init */
            ret = nvp1118_device_init(i, norm[i]);
            if(ret < 0)
                goto err;

            /* register nvp1118 video loss notify handler */
            nvp1118_notify_vlos_register(i, nvp1118_vlos_notify_handler);

            /* register nvp1118 video format notify handler */
            nvp1118_notify_vfmt_register(i, nvp1118_vfmt_notify_handler);

            vcap_info("Register NVP1118#%d Input Device\n", i);
        }
    }

    return ret;

err:
    for(i=0; i<DEV_MAX; i++) {
        nvp1118_notify_vlos_deregister(i);
        nvp1118_notify_vfmt_deregister(i);
        nvp1118_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(nvp1118_dev[i].port[j]) {
                vcap_input_device_unregister(nvp1118_dev[i].port[j]);
                kfree(nvp1118_dev[i].port[j]);
            }
        }
    }
    return ret;
}

static void __exit nvp1118_input_exit(void)
{
    int i, j;

    for(i=0; i<DEV_MAX; i++) {
        nvp1118_notify_vlos_deregister(i);
        nvp1118_notify_vfmt_deregister(i);
        nvp1118_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(nvp1118_dev[i].port[j]) {
                vcap_input_device_unregister(nvp1118_dev[i].port[j]);
                kfree(nvp1118_dev[i].port[j]);
            }
        }
    }
}

module_init(nvp1118_input_init);
module_exit(nvp1118_input_exit);

MODULE_DESCRIPTION("Grain Media Video Capture300 NVP1118 Input Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
