/**
 * @file es8328_drv.c
 * EVEREST ES8328 2-CH Audio Codecs Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
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
#include <mach/ftpmu010.h>

#include "platform.h"
#include "everest/es8328.h"       ///< from module/include/front_end/decoder


/* device number */
static int dev_num = ES8328_DEV_MAX;
module_param(dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dev_num, "Device Number: 1~4");

/* device i2c bus */
static int ibus = 0;
module_param(ibus, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ibus, "Device I2C Bus: 0~4");

/* device i2c address */
static ushort iaddr[ES8328_DEV_MAX] = {0x20, 0x22};
module_param_array(iaddr, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iaddr, "Device I2C Address");

/* audio sample rate */
static int sample_rate = AUDIO_SAMPLERATE_8K;
module_param(sample_rate, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sample_rate, "audio sample rate: 0:8k  1:16k");

/* audio sample size */
static int sample_size = AUDIO_BITS_16B;
module_param(sample_size, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sample_size, "audio sample size: 0:16bit  1:8bit");

/* init */
static int init = 1;
module_param(init, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init, "device init: 0:no-init  1:init");

/* device channel mapping */
static int ch_map = 0;
module_param(ch_map, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_map, "channel mapping");

/* audio channel number */
int audio_chnum = 2;
module_param(audio_chnum, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(audio_chnum, "audio channel number");

/******************************************************************************
 GM8210/GM8287 EVB Channel and VIN Mapping Table (Socket Board)

 VCH#0 ------> VIN#0 -----------------> VPortA -------> X_CAP#1
 VCH#1 ------> VIN#1 ---------|
 VCH#2 ------> VIN#2 ---------|
 VCH#3 ------> VIN#3 ---------|
 VCH#4 ------> VIN#4 -----------------> VPortB -------> X_CAP#0
 VCH#5 ------> VIN#5 ---------|
 VCH#6 ------> VIN#6 ---------|
 VCH#7 ------> VIN#7 ---------|

 ==============================================================================
 GM8210 EVB Channel and VIN Mapping Table (System Board 1V1)

 VCH#7 ------> VIN#0 -----------------> VPortA -------> X_CAP#0
 VCH#6 ------> VIN#1 ---------|
 VCH#5 ------> VIN#2 ---------|
 VCH#4 ------> VIN#3 ---------|
 VCH#3 ------> VIN#4 -----------------> VPortB -------> X_CAP#1
 VCH#2 ------> VIN#5 ---------|
 VCH#1 ------> VIN#6 ---------|
 VCH#0 ------> VIN#7 ---------|

 ==============================================================================
 GM8287 EVB Channel and VIN Mapping Table (System Board)
 GM8283 EVB Channel and VIN Mapping Table (Socket Board)

 VCH#0 ------> VIN#0 -----------------> VPortA -------> X_CAP#0
 VCH#1 ------> VIN#1 ---------|
 VCH#2 ------> VIN#2 ---------|
 VCH#3 ------> VIN#3 ---------|
 VCH#4 ------> VIN#4 -----------------> VPortB -------> X_CAP#1
 VCH#5 ------> VIN#5 ---------|
 VCH#6 ------> VIN#6 ---------|
 VCH#7 ------> VIN#7 ---------|

*******************************************************************************/

struct es8328_dev_t {
    int                 index;      ///< device index
    char                name[16];   ///< device name
    struct semaphore    lock;       ///< device locker
    struct miscdevice   miscdev;    ///< device node for ioctl access

    int                 (*vlos_notify)(int id, int vin, int vlos);
};

static struct es8328_dev_t  es8328_dev[ES8328_DEV_MAX];
static struct proc_dir_entry *es8328_proc_root[ES8328_DEV_MAX]     = {[0 ... (ES8328_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *es8328_proc_volume[ES8328_DEV_MAX]   = {[0 ... (ES8328_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *es8328_proc_output_ch[ES8328_DEV_MAX]= {[0 ... (ES8328_DEV_MAX - 1)] = NULL};

static audio_funs_t es8328_audio_funs;
static struct task_struct *es8328_wd         = NULL;
static struct i2c_client  *es8328_i2c_client = NULL;

static int es8328_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    es8328_i2c_client = client;
    return 0;
}

static int es8328_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id es8328_i2c_id[] = {
    { "es8328_i2c", 0 },
    { }
};

static struct i2c_driver es8328_i2c_driver = {
    .driver = {
        .name  = "ES8328_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = es8328_i2c_probe,
    .remove   = es8328_i2c_remove,
    .id_table = es8328_i2c_id
};

static int es8328_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&es8328_i2c_driver);
    if(ret < 0) {
        printk("ES8328 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = iaddr[0]>>1;
    strlcpy(info.type, "es8328_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("ES8328 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("ES8328 can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&es8328_i2c_driver);
   return -ENODEV;
}

static void es8328_i2c_unregister(void)
{
    i2c_unregister_device(es8328_i2c_client);
    i2c_del_driver(&es8328_i2c_driver);
    es8328_i2c_client = NULL;
}

/* return value, 0: success, others: failed */
static int es8328_sound_switch(int ssp_idx, int chan_num, bool is_on)
{
    const u32 max_audio_chan_cnt   = 2;
    u32 temp, decoder_idx = 0;

    if (chan_num >= max_audio_chan_cnt) {
        printk(KERN_WARNING "%s: Not implement yet\n", __func__);
        return -EACCES;
    }

    temp = chan_num;

    is_on ? es8328_audio_set_output_ch(decoder_idx, chan_num) : es8328_audio_set_output_ch(decoder_idx, 0x10);

    return 0;
}

int es8328_i2c_write(u8 id, u8 reg, u8 data)
{
    u8 buf[2];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return -1;

    if(!es8328_i2c_client) {
        printk("ES8328 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(es8328_i2c_client->dev.parent);

    buf[0]     = reg;
    buf[1]     = data;
    msgs.addr  = iaddr[id]>>1;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("ES8328#%d i2c write failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(es8328_i2c_write);

u8 es8328_i2c_read(u8 id, u8 reg)
{
    u8 reg_data = 0;
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return 0;

    if(!es8328_i2c_client) {
        printk("ES8328 i2c_client not register!!\n");
        return 0;
    }

    adapter = to_i2c_adapter(es8328_i2c_client->dev.parent);

    msgs[0].addr  = iaddr[id]>>1;
    msgs[0].flags = 0; /* write */
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    msgs[1].addr  = (iaddr[id] + 1)>>1;
    msgs[1].flags = 1; /* read */
    msgs[1].len   = 1;
    msgs[1].buf   = &reg_data;

    if(i2c_transfer(adapter, msgs, 2) != 2)
        printk("ES8328#%d i2c read failed!!\n", id);

    return reg_data;
}
EXPORT_SYMBOL(es8328_i2c_read);

int es8328_i2c_array_write(u8 id, u8 addr, u8 *parray, u32 array_cnt)
{
#define ES8328_I2C_ARRAY_WRITE_MAX     256
    int i, num = 0;
    u8  buf[ES8328_I2C_ARRAY_WRITE_MAX+1];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if(id >= dev_num || !parray || array_cnt > ES8328_I2C_ARRAY_WRITE_MAX)
        return -1;

    if(!es8328_i2c_client) {
        printk("ES8328 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(es8328_i2c_client->dev.parent);

    buf[num++] = addr;
    for(i=0; i<array_cnt; i++) {
        buf[num++] = parray[i];
    }

    msgs.addr  = iaddr[id]>>1;
    msgs.flags = 0;
    msgs.len   = num;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("ES8328#%d i2c write failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(es8328_i2c_array_write);

int es8328_get_device_num(void)
{
    return dev_num;
}
EXPORT_SYMBOL(es8328_get_device_num);


static int es8328_proc_volume_show(struct seq_file *sfile, void *v)
{
    int aogain;
    struct es8328_dev_t *pdev = (struct es8328_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[ES8328#%d]\n", pdev->index);
    aogain = es8328_audio_get_mute(pdev->index);
    seq_printf(sfile, "Volume[0x0~0xf] = %d\n", aogain);

    up(&pdev->lock);

    return 0;
}

static int es8328_proc_output_ch_show(struct seq_file *sfile, void *v)
{
    int ch;
    struct es8328_dev_t *pdev = (struct es8328_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[ES8328#%d]\n", pdev->index);

    ch = es8328_audio_get_output_ch(pdev->index);

    if (ch == 0 || ch == 1)
        seq_printf(sfile, "Current[0x0~0x1]==> %d\n\n", ch);
    if (ch == 16)
        seq_printf(sfile, "Playback Channel\n");

    up(&pdev->lock);

    return 0;
}


static ssize_t es8328_proc_volume_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 volume;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct es8328_dev_t *pdev = (struct es8328_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &volume);

    down(&pdev->lock);

    es8328_audio_set_volume(pdev->index, volume);

    up(&pdev->lock);

    return count;
}

static ssize_t es8328_proc_output_ch_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 ch;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct es8328_dev_t *pdev = (struct es8328_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &ch);

    down(&pdev->lock);

    es8328_audio_set_output_ch(pdev->index, ch);

    up(&pdev->lock);

    return count;
}

static int es8328_proc_volume_open(struct inode *inode, struct file *file)
{
    return single_open(file, es8328_proc_volume_show, PDE(inode)->data);
}

static int es8328_proc_output_ch_open(struct inode *inode, struct file *file)
{
    return single_open(file, es8328_proc_output_ch_show, PDE(inode)->data);
}

static struct file_operations es8328_proc_volume_ops = {
    .owner  = THIS_MODULE,
    .open   = es8328_proc_volume_open,
    .write  = es8328_proc_volume_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations es8328_proc_output_ch_ops = {
    .owner  = THIS_MODULE,
    .open   = es8328_proc_output_ch_open,
    .write  = es8328_proc_output_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void es8328_proc_remove(int id)
{
    if(id >= ES8328_DEV_MAX)
        return;

    if(es8328_proc_root[id]) {
        if(es8328_proc_volume[id]) {
            remove_proc_entry(es8328_proc_volume[id]->name, es8328_proc_root[id]);
            es8328_proc_volume[id] = NULL;
        }

        if(es8328_proc_output_ch[id]) {
            remove_proc_entry(es8328_proc_output_ch[id]->name, es8328_proc_root[id]);
            es8328_proc_output_ch[id] = NULL;
        }

        remove_proc_entry(es8328_proc_root[id]->name, NULL);
        es8328_proc_root[id] = NULL;
    }
}

static int es8328_proc_init(int id)
{
    int ret = 0;

    if(id >= ES8328_DEV_MAX)
        return -1;

    /* root */
    es8328_proc_root[id] = create_proc_entry(es8328_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!es8328_proc_root[id]) {
        printk("create proc node '%s' failed!\n", es8328_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    es8328_proc_root[id]->owner = THIS_MODULE;
#endif

    /* volume */
    es8328_proc_volume[id] = create_proc_entry("volume", S_IRUGO|S_IXUGO, es8328_proc_root[id]);
    if(!es8328_proc_volume[id]) {
        printk("create proc node '%s/volume' failed!\n", es8328_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    es8328_proc_volume[id]->proc_fops = &es8328_proc_volume_ops;
    es8328_proc_volume[id]->data      = (void *)&es8328_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    es8328_proc_volume[id]->owner     = THIS_MODULE;
#endif

    /* output channel */
    es8328_proc_output_ch[id] = create_proc_entry("output_ch", S_IRUGO|S_IXUGO, es8328_proc_root[id]);
    if(!es8328_proc_output_ch[id]) {
        printk("create proc node '%s/output_ch' failed!\n", es8328_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    es8328_proc_output_ch[id]->proc_fops = &es8328_proc_output_ch_ops;
    es8328_proc_output_ch[id]->data      = (void *)&es8328_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    es8328_proc_output_ch[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    es8328_proc_remove(id);
    return ret;
}

static int es8328_miscdev_open(struct inode *inode, struct file *file)
{
    int i, ret = 0;
    struct es8328_dev_t *pdev = NULL;

    /* lookup device */
    for(i=0; i<dev_num; i++) {
        if(es8328_dev[i].miscdev.minor == iminor(inode)) {
            pdev = &es8328_dev[i];
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

static int es8328_miscdev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long es8328_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int tmp, ret = 0;
    //struct es8328_ioc_data ioc_data;
    struct es8328_dev_t *pdev = (struct es8328_dev_t *)file->private_data;

    down(&pdev->lock);

    if(_IOC_TYPE(cmd) != ES8328_IOC_MAGIC) {
        ret = -ENOIOCTLCMD;
        goto exit;
    }

    switch(cmd) {
        case ES8328_GET_VOL:
            tmp = es8328_audio_get_mute(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case ES8328_SET_VOL:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = es8328_audio_set_volume(pdev->index, tmp);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case ES8328_GET_OUT_CH:
            tmp = es8328_audio_get_output_ch(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case ES8328_SET_OUT_CH:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }
            printk("tmp %d\n", tmp);
            ret = es8328_audio_set_output_ch(pdev->index, tmp);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
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

static struct file_operations es8328_miscdev_fops = {
    .owner          = THIS_MODULE,
    .open           = es8328_miscdev_open,
    .release        = es8328_miscdev_release,
    .unlocked_ioctl = es8328_miscdev_ioctl,
};

static int es8328_miscdev_init(int id)
{
    int ret;

    if(id >= ES8328_DEV_MAX)
        return -1;

    /* clear */
    memset(&es8328_dev[id].miscdev, 0, sizeof(struct miscdevice));

    /* create device node */
    es8328_dev[id].miscdev.name  = es8328_dev[id].name;
    es8328_dev[id].miscdev.minor = MISC_DYNAMIC_MINOR;
    es8328_dev[id].miscdev.fops  = (struct file_operations *)&es8328_miscdev_fops;
    ret = misc_register(&es8328_dev[id].miscdev);
    if(ret) {
        printk("create %s misc device node failed!\n", es8328_dev[id].name);
        es8328_dev[id].miscdev.name = 0;
        goto exit;
    }

exit:
    return ret;
}

static int es8328_device_init(int id)
{
    int ret = 0;

    if (!init)
        return 0;

    if(id >= ES8328_DEV_MAX)
        return -1;

    /*====================== audio init ========================= */
    ret = es8328_audio_set_mode(id, sample_size, sample_rate);
    if(ret < 0)
        goto exit;

exit:
    return ret;
}


static int __init es8328_init(void)
{
    int i, ret;

    /* platfrom check */
    plat_identifier_check();

    /* check device number */
    if(dev_num > ES8328_DEV_MAX) {
        printk("ES8328 dev_num=%d invalid!!(Max=%d)\n", dev_num, ES8328_DEV_MAX);
        return -1;
    }

    /* register i2c client for contol es8328 */
    ret = es8328_i2c_register();
    if(ret < 0)
        return -1;

    /* ES8328 init */
    for(i=0; i<dev_num; i++) {
        es8328_dev[i].index = i;

        sprintf(es8328_dev[i].name, "es8328.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&es8328_dev[i].lock, 1);
#else
        init_MUTEX(&es8328_dev[i].lock);
#endif
        ret = es8328_proc_init(i);
        if(ret < 0)
            goto err;

        ret = es8328_miscdev_init(i);
        if(ret < 0)
            goto err;

        ret = es8328_device_init(i);
        if(ret < 0)
            goto err;
    }

    /* register audio function for platform used */
    es8328_audio_funs.sound_switch = es8328_sound_switch;
    es8328_audio_funs.codec_name   = es8328_dev[0].name;
    plat_audio_register_function(&es8328_audio_funs);

    return 0;

err:
    if(es8328_wd)
        kthread_stop(es8328_wd);

    es8328_i2c_unregister();
    for(i=0; i<ES8328_DEV_MAX; i++) {
        if(es8328_dev[i].miscdev.name)
            misc_deregister(&es8328_dev[i].miscdev);

        es8328_proc_remove(i);
    }



    return ret;
}

static void __exit es8328_exit(void)
{
    int i;

    /* stop es8328 watchdog thread */
    if(es8328_wd)
        kthread_stop(es8328_wd);

    es8328_i2c_unregister();
    for(i=0; i<ES8328_DEV_MAX; i++) {
        /* remove device node */
        if(es8328_dev[i].miscdev.name)
            misc_deregister(&es8328_dev[i].miscdev);

        /* remove proc node */
        es8328_proc_remove(i);
    }

    plat_audio_deregister_function();
}

module_init(es8328_init);
module_exit(es8328_exit);

MODULE_DESCRIPTION("Grain Media ES8328 Video Decoders and Audio Codecs Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
