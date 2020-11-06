/**
 * @file isp.c
 * VCAP300 ISP Input Device Driver
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.7 $
 * $Date: 2014/06/18 06:28:47 $
 *
 * ChangeLog:
 *  $Log: isp.c,v $
 *  Revision 1.7  2014/06/18 06:28:47  jerson_l
 *  1. add polling_time proc node for swicth monitor thread sleep time
 *
 *  Revision 1.6  2014/03/31 01:45:23  jerson_l
 *  1. support dynamic switch resolution without stop channel
 *
 *  Revision 1.5  2014/02/06 02:58:22  jerson_l
 *  1. modify version compare rule of input module driver
 *
 *  Revision 1.4  2014/01/24 07:33:11  jerson_l
 *  1. support isp dynamic switch data range
 *
 *  Revision 1.3  2013/11/27 05:38:47  jerson_l
 *  1. support status notify
 *
 *  Revision 1.2  2013/11/07 06:39:59  jerson_l
 *  1. support max_frame_rate infomation
 *
 *  Revision 1.1  2013/10/28 10:01:29  jerson_l
 *  1. support ISP input module
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#include "vcap_input.h"
#include "vcap_dbg.h"
#include "isp_api.h"            ///< from module/include/isp320

#define DEFAULT_TYPE            VCAP_INPUT_TYPE_ISP
#define DEFAULT_INTERFACE       VCAP_INPUT_INTF_ISP
#define DEFAULT_NORM_WIDTH      1280
#define DEFAULT_NORM_HEIGHT     720
#define DEFAULT_FRAME_RATE      30
#define DEFAULT_MODE            VCAP_INPUT_MODE_BYPASS
#define DEFAULT_SPEED           VCAP_INPUT_SPEED_P
#define DEFAULT_ORDER           VCAP_INPUT_ORDER_ANYONE
#define DEFAULT_DATA_RANGE      VCAP_INPUT_DATA_RANGE_256
#define DEFAULT_TIMEOUT_MS      1000                          ///< 1000ms
#define DEFAULT_POLLING_TIME    100                           ///< 100ms
#define ISP_TIMEOUT_MS(x)       ((1000/(x))*3)

static int vi = 1;
module_param(vi, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vi, "VI Number");

static int width = -1;          ///< -1: means get width information from ISP driver
module_param(width, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(width, "Source Width");

static int height = -1;        ///< -1: means get height information from ISP driver
module_param(height, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(height, "Source Height");

static int range = DEFAULT_DATA_RANGE;
module_param(range, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(range, "Data Range=> 0:256 Level, 1:240 Level");

static int inv_clk = 0;
module_param(inv_clk, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "Invert Clock, 0:Disable, 1: Enable");

static int yc_swap = 0;
module_param(yc_swap, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(yc_swap, "0: None, 1:YC Swap, 2:CbCr Swap, 3:YC and CbCr Swap");

static int timeout = -1;        ///< -1 means base on frame rate to caculate
module_param(timeout, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(timeout, "Signal Timeout Threshold(ms)");

static int ch_id = -1;          ///< -1: means base on vi number
module_param(ch_id, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_id, "Video Channel Index");

struct isp_dev_t {
    int                     index;                  ///< device index
    struct semaphore        lock;                   ///< device locker
    int                     start;                  ///< device start, 0:stop 1:start
    int                     out_w;                  ///< device output image width
    int                     out_h;                  ///< device output image height
    int                     out_fps;                ///< device output frame rate
    int                     max_fps;                ///< device max outpout frame rate
    int                     polling_time;           ///< device monitor thread polling time(ms)
    struct vcap_input_dev_t *port;
    struct task_struct      *monitor;               ///< monitor resolution and frame rate
};

static struct isp_dev_t         isp_dev;
static struct proc_dir_entry   *isp_proc_root         = NULL;
static struct proc_dir_entry   *isp_proc_norm         = NULL;
static struct proc_dir_entry   *isp_proc_timeout      = NULL;
static struct proc_dir_entry   *isp_proc_data_range   = NULL;
static struct proc_dir_entry   *isp_proc_polling_time = NULL;

static int isp_module_get(void)
{
    return try_module_get(THIS_MODULE);
}

static void isp_module_put(void)
{
    module_put(THIS_MODULE);
}

static int isp_device_init(int id, int norm)
{
    int ret = 0;
    int out_w, out_h, out_fps, max_fps;
    struct isp_dev_t *pdev = &isp_dev;

    if(!pdev->start) {
        isp_info_t isp_info;

        ret = isp_api_start_isp();
        if(ret < 0) {
            printk("ISP start falied!!\n");
            goto exit;
        }
        pdev->start = 1;

        udelay(10);

        ret = isp_api_get_info(&isp_info);
        if(ret < 0)
            goto exit;

        /* get isp output resolution and frame rate */
        pdev->out_w   = isp_info.cap_width;
        pdev->out_h   = isp_info.cap_height;
        pdev->out_fps = isp_info.fps;
        pdev->max_fps = isp_info.max_fps;
    }

    if(norm != (pdev->port)->norm.mode) {
        if(width > 0)
            out_w = width;
        else
            out_w = (pdev->out_w > 0) ? pdev->out_w : DEFAULT_NORM_WIDTH;

        if(height > 0)
            out_h = height;
        else
            out_h = (pdev->out_h > 0) ? pdev->out_h : DEFAULT_NORM_HEIGHT;

        out_fps = (pdev->out_fps > 0) ? pdev->out_fps : DEFAULT_FRAME_RATE;

        max_fps = (pdev->max_fps > 0) ? pdev->max_fps : DEFAULT_FRAME_RATE;

        if((out_w != pdev->out_w) || (out_h != pdev->out_h))
            ret = isp_api_set_cap_size(out_w, out_h);

        if(ret < 0) {
            if((pdev->port)->norm.width <= 0)
                (pdev->port)->norm.width = out_w;

            if((pdev->port)->norm.height <= 0)
                (pdev->port)->norm.height = out_h;

            if((pdev->port)->frame_rate == 0)
                (pdev->port)->frame_rate = out_fps;

            if((pdev->port)->max_frame_rate == 0)
                (pdev->port)->max_frame_rate = max_fps;
        }
        else {
            pdev->out_w = out_w;
            pdev->out_h = out_h;

            (pdev->port)->norm.mode      = norm;
            (pdev->port)->norm.width     = out_w;
            (pdev->port)->norm.height    = out_h;
            (pdev->port)->frame_rate     = out_fps;
            (pdev->port)->max_frame_rate = max_fps;
        }
        (pdev->port)->timeout_ms = (timeout > 0) ? timeout : ISP_TIMEOUT_MS(out_fps);
    }

exit:
    return ret;
}

static int isp_proc_norm_show(struct seq_file *sfile, void *v)
{
    struct isp_dev_t *pdev = (struct isp_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "ISP_Out: %d x %d @%dFPS\n", pdev->out_w, pdev->out_h, pdev->out_fps);
    seq_printf(sfile, "Norm   : %d x %d\n", (pdev->port)->norm.width, (pdev->port)->norm.height);

    up(&pdev->lock);

    return 0;
}

static int isp_proc_timeout_show(struct seq_file *sfile, void *v)
{
    struct isp_dev_t *pdev = (struct isp_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "Timeout Threshold: %d (ms)\n", (pdev->port)->timeout_ms);

    up(&pdev->lock);

    return 0;
}

static int isp_proc_data_range_show(struct seq_file *sfile, void *v)
{
    struct isp_dev_t *pdev = (struct isp_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "0: 256_Level 1: 240_Level\n");
    seq_printf(sfile, "Data Range: %s\n", (((pdev->port)->data_range == VCAP_INPUT_DATA_RANGE_240) ? "240_Level" : "256_Level"));

    up(&pdev->lock);

    return 0;
}

static int isp_proc_polling_time_show(struct seq_file *sfile, void *v)
{
    struct isp_dev_t *pdev = (struct isp_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "Polling Time: %d (ms)\n", pdev->polling_time);

    up(&pdev->lock);

    return 0;
}

static ssize_t isp_proc_norm_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ret = 0;
    int  norm_w, norm_h;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct isp_dev_t *pdev = (struct isp_dev_t *)sfile->private;
    struct vcap_input_dev_t *pinput = pdev->port;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &norm_w, &norm_h);

    down(&pdev->lock);

    if(norm_w != width || norm_h != height) {
        if(norm_w > 0)
            width = norm_w;
        if(norm_h > 0)
            height = norm_h;
        isp_device_init(pdev->index, pinput->norm.mode+1);  ///< update norm value to inform driver to switch VI source

        /* issue notify */
        vcap_input_device_notify(pinput->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
    }

    up(&pdev->lock);

    if(ret)
        return ret;

    return count;
}

static ssize_t isp_proc_timeout_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ret = 0;
    int  time_ms;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct isp_dev_t *pdev = (struct isp_dev_t *)sfile->private;
    struct vcap_input_dev_t *pinput = pdev->port;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &time_ms);

    down(&pdev->lock);

    if(time_ms != pinput->timeout_ms) {
        if(time_ms < 0) {
            timeout = -1;
            if(pinput->frame_rate)
                pinput->timeout_ms = ISP_TIMEOUT_MS(pinput->frame_rate);
            else
                pinput->timeout_ms = ISP_TIMEOUT_MS(DEFAULT_FRAME_RATE);
        }
        else
            timeout = pinput->timeout_ms = time_ms;
    }

    up(&pdev->lock);

    if(ret)
        return ret;

    return count;
}

static ssize_t isp_proc_data_range_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ret = 0;
    int  range;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct isp_dev_t *pdev = (struct isp_dev_t *)sfile->private;
    struct vcap_input_dev_t *pinput = pdev->port;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &range);

    down(&pdev->lock);

    if(range != pinput->data_range) {
        if((range == VCAP_INPUT_DATA_RANGE_256) || (range == VCAP_INPUT_DATA_RANGE_240))
            pinput->data_range = range;
    }

    up(&pdev->lock);

    if(ret)
        return ret;

    return count;
}

static ssize_t isp_proc_polling_time_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  time_ms;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct isp_dev_t *pdev = (struct isp_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &time_ms);

    down(&pdev->lock);

    if((time_ms > 0) && (time_ms != pdev->polling_time)) {
        pdev->polling_time = time_ms;
    }

    up(&pdev->lock);

    return count;
}

static int isp_proc_norm_open(struct inode *inode, struct file *file)
{
    return single_open(file, isp_proc_norm_show, PDE(inode)->data);
}

static int isp_proc_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, isp_proc_timeout_show, PDE(inode)->data);
}

static int isp_proc_data_range_open(struct inode *inode, struct file *file)
{
    return single_open(file, isp_proc_data_range_show, PDE(inode)->data);
}

static int isp_proc_polling_time_open(struct inode *inode, struct file *file)
{
    return single_open(file, isp_proc_polling_time_show, PDE(inode)->data);
}

static struct file_operations isp_proc_norm_ops = {
    .owner  = THIS_MODULE,
    .open   = isp_proc_norm_open,
    .write  = isp_proc_norm_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations isp_proc_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = isp_proc_timeout_open,
    .write  = isp_proc_timeout_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations isp_proc_data_range_ops = {
    .owner  = THIS_MODULE,
    .open   = isp_proc_data_range_open,
    .write  = isp_proc_data_range_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations isp_proc_polling_time_ops = {
    .owner  = THIS_MODULE,
    .open   = isp_proc_polling_time_open,
    .write  = isp_proc_polling_time_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void isp_proc_remove(void)
{
    if(isp_proc_root) {
        if(isp_proc_polling_time)
            vcap_input_proc_remove_entry(isp_proc_root, isp_proc_polling_time);

        if(isp_proc_data_range)
            vcap_input_proc_remove_entry(isp_proc_root, isp_proc_data_range);

        if(isp_proc_timeout)
            vcap_input_proc_remove_entry(isp_proc_root, isp_proc_timeout);

        if(isp_proc_norm)
            vcap_input_proc_remove_entry(isp_proc_root, isp_proc_norm);

        vcap_input_proc_remove_entry(NULL, isp_proc_root);
    }
}

static int isp_proc_init(struct isp_dev_t *pdev)
{
    int ret = 0;

    /* root */
    isp_proc_root = vcap_input_proc_create_entry((pdev->port)->name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!isp_proc_root) {
        vcap_err("create proc node '%s' failed!\n", (pdev->port)->name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    isp_proc_root->owner = THIS_MODULE;
#endif

    /* norm */
    isp_proc_norm = vcap_input_proc_create_entry("norm", S_IRUGO|S_IXUGO, isp_proc_root);
    if(!isp_proc_norm) {
        vcap_err("create proc node '%s/norm' failed!\n", (pdev->port)->name);
        ret = -EINVAL;
        goto err;
    }
    isp_proc_norm->proc_fops = &isp_proc_norm_ops;
    isp_proc_norm->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    isp_proc_norm->owner     = THIS_MODULE;
#endif

    /* timeout */
    isp_proc_timeout = vcap_input_proc_create_entry("timeout", S_IRUGO|S_IXUGO, isp_proc_root);
    if(!isp_proc_timeout) {
        vcap_err("create proc node '%s/timeout' failed!\n", (pdev->port)->name);
        ret = -EINVAL;
        goto err;
    }
    isp_proc_timeout->proc_fops = &isp_proc_timeout_ops;
    isp_proc_timeout->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    isp_proc_timeout->owner     = THIS_MODULE;
#endif

    /* data range */
    isp_proc_data_range = vcap_input_proc_create_entry("data_range", S_IRUGO|S_IXUGO, isp_proc_root);
    if(!isp_proc_data_range) {
        vcap_err("create proc node '%s/data_range' failed!\n", (pdev->port)->name);
        ret = -EINVAL;
        goto err;
    }
    isp_proc_data_range->proc_fops = &isp_proc_data_range_ops;
    isp_proc_data_range->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    isp_proc_data_range->owner     = THIS_MODULE;
#endif

    /* polling time */
    isp_proc_polling_time = vcap_input_proc_create_entry("polling_time", S_IRUGO|S_IXUGO, isp_proc_root);
    if(!isp_proc_polling_time) {
        vcap_err("create proc node '%s/polling_time' failed!\n", (pdev->port)->name);
        ret = -EINVAL;
        goto err;
    }
    isp_proc_polling_time->proc_fops = &isp_proc_polling_time_ops;
    isp_proc_polling_time->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    isp_proc_polling_time->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    isp_proc_remove();
    return ret;
}

static int isp_monitor_thread(void *data)
{
    int ret;
    isp_info_t isp_info;
    struct isp_dev_t *pdev = &isp_dev;

    do {
            down(&pdev->lock);

            ret = isp_api_get_info(&isp_info);
            if(ret == 0) {
                pdev->out_w   = isp_info.cap_width;
                pdev->out_h   = isp_info.cap_height;
                pdev->out_fps = isp_info.fps;
                pdev->max_fps = isp_info.max_fps;

                /* check resolution */
                if((pdev->out_w != (pdev->port)->norm.width) || (pdev->out_h != (pdev->port)->norm.height)) {
                    isp_device_init(pdev->index, (pdev->port)->norm.mode+1);

                    /* issue notify */
                    vcap_input_device_notify((pdev->port)->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                }

                /* check frame rate */
                if((pdev->out_fps > 0) && (pdev->out_fps != (pdev->port)->frame_rate)) {
                    (pdev->port)->frame_rate = pdev->out_fps;
                    (pdev->port)->timeout_ms = ISP_TIMEOUT_MS(pdev->out_fps);

                    if(pdev->max_fps)
                        (pdev->port)->max_frame_rate = pdev->max_fps;

                    /* issue notify */
                    vcap_input_device_notify((pdev->port)->vi_idx, 0, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                }
            }

            up(&pdev->lock);

            /* sleep */
            schedule_timeout_interruptible(msecs_to_jiffies(pdev->polling_time));

    } while (!kthread_should_stop());

    return 0;
}

static int __init isp_input_init(void)
{
    int ret = 0;
    struct vcap_input_dev_t *pinput;

    if(vcap_input_get_version() != VCAP_INPUT_VERSION) {
        vcap_err("Input driver version(%08x) is not compatibility with vcap300_common.ko(%08x)!\n",
                 vcap_input_get_version(), VCAP_INPUT_VERSION);
        return -EFAULT;
    }

    /* clear buffer */
    memset(&isp_dev, 0, sizeof(struct isp_dev_t));

    isp_dev.index = 0;
    isp_dev.polling_time = DEFAULT_POLLING_TIME;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
    sema_init(&isp_dev.lock, 1);
#else
    init_MUTEX(&isp_dev.lock);
#endif

    pinput = isp_dev.port = kzalloc(sizeof(struct vcap_input_dev_t), GFP_KERNEL);
    if(pinput == NULL) {
        vcap_err("ISP allocate vcap_input_dev_t failed\n");
        ret = -ENOMEM;
        goto err;
    }

    /* input device name */
    snprintf(pinput->name, VCAP_INPUT_NAME_SIZE-1, "isp");

    /* device parameter setup */
    pinput->index        = 0;
    pinput->vi_idx       = vi;
    pinput->vi_src       = VCAP_INPUT_VI_SRC_ISP;
    pinput->type         = DEFAULT_TYPE;
    pinput->speed        = DEFAULT_SPEED;
    pinput->interface    = DEFAULT_INTERFACE;
    pinput->mode         = DEFAULT_MODE;
    pinput->field_order  = DEFAULT_ORDER;
    pinput->data_range   = range;
    pinput->yc_swap      = yc_swap;
    pinput->data_swap    = 0;
    pinput->inv_clk      = inv_clk;
    pinput->init         = isp_device_init;
    pinput->module_get   = isp_module_get;
    pinput->module_put   = isp_module_put;
    pinput->norm.mode    = -1;                   ///< init value
    pinput->ch_id[0]     = (ch_id < 0) ? (vi*VCAP_INPUT_DEV_CH_MAX) : ch_id;

    /* init semaphore lock */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
    sema_init(&pinput->sema_lock, 1);
#else
    init_MUTEX(&pinput->sema_lock);
#endif

    ret = vcap_input_device_register(pinput);
    if(ret < 0) {
        vcap_err("register vcap ISP input device failed\n");
        goto err;
    }

    ret = isp_proc_init(&isp_dev);
    if(ret < 0)
        goto err;

    /* device init */
    ret = isp_device_init(isp_dev.index, 0);
    if(ret < 0)
        goto err;

    /* init isp monitor thread */
    isp_dev.monitor = kthread_create(isp_monitor_thread, NULL, "isp_mon");
    if(!IS_ERR(isp_dev.monitor))
        wake_up_process(isp_dev.monitor);
    else {
        printk("create isp monitor thread failed!!\n");
        isp_dev.monitor = 0;
        ret = -EFAULT;
        goto err;
    }

    vcap_info("Register ISP Input Device\n");

    return ret;

err:
    if(isp_dev.monitor)
        kthread_stop(isp_dev.monitor);

    isp_proc_remove();

    if(isp_dev.port) {
        if(isp_dev.start) {
            isp_api_stop_isp();
            isp_dev.start = 0;
        }

        vcap_input_device_unregister(isp_dev.port);
        kfree(isp_dev.port);
    }

    return ret;
}

static void __exit isp_input_exit(void)
{
    if(isp_dev.monitor)
        kthread_stop(isp_dev.monitor);

    isp_proc_remove();

    if(isp_dev.port) {
        if(isp_dev.start) {
            isp_api_stop_isp();
            isp_dev.start = 0;
        }

        vcap_input_device_unregister(isp_dev.port);
        kfree(isp_dev.port);
    }
}

module_init(isp_input_init);
module_exit(isp_input_exit);

MODULE_DESCRIPTION("Grain Media Video Capture300 ISP Input Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
