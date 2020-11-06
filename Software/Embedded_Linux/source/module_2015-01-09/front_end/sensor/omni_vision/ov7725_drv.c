/**
 * @file ov7725_drv.c
 * OmniVision OV7725 1MP Image CMOS Sensor Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.4 $
 * $Date: 2014/07/29 05:36:16 $
 *
 * ChangeLog:
 *  $Log: ov7725_drv.c,v $
 *  Revision 1.4  2014/07/29 05:36:16  shawn_hu
 *  Add the module parameter of "ibus" to specify which I2C bus has the decoder attached.
 *
 *  Revision 1.3  2013/11/11 05:45:43  jerson_l
 *  1. add ch_id module parameter for specify video channel index
 *
 *  Revision 1.2  2013/10/24 07:35:30  jerson_l
 *  1. update init config table
 *
 *  Revision 1.1.1.1  2013/10/24 02:51:52  jerson_l
 *  add sensor driver
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
#include "omni_vision/ov7725.h"        ///< from module/include/front_end/sensor

#define OV7725_CLKIN                   27000000

/* device number */
static int dev_num = 1;
module_param(dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dev_num, "Device Number: 1");

/* device i2c bus */
static int ibus = 0;
module_param(ibus, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ibus, "Device I2C Bus: 0~4");

/* device i2c address */
static ushort iaddr[OV7725_DEV_MAX] = {0x42};
module_param_array(iaddr, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iaddr, "Device I2C Address");

/* clock port used */
static int clk_used = 0x1;
module_param(clk_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_used, "External Clock Port: BIT0:EXT_CLK0, BIT1:EXT_CLK1");

/* clock source */
static int clk_src = PLAT_EXTCLK_SRC_PLL3;
module_param(clk_src, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_src, "External Clock Source: 0:PLL1OUT1 1:PLL1OUT1/2 2:PLL4OUT2 3:PLL4OUT1/2 4:PLL3");

/* clock frequency */
static int clk_freq = OV7725_CLKIN;
module_param(clk_freq, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_freq, "External Clock Frequency(Hz)");

/* device video mode */
static int vmode[OV7725_DEV_MAX] = {[0 ... (OV7725_DEV_MAX - 1)] = OV7725_VMODE_VGA};
module_param_array(vmode, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vmode, "Video Mode: 0:VGA");

/* device init */
static int init = 1;
module_param(init, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init, "device init: 0:no-init  1:init");

/* device use gpio as rstb pin */
static int rstb_used = PLAT_GPIO_ID_X_CAP_RST;
module_param(rstb_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rstb_used, "Use GPIO as RSTB Pin: 0:None, 1:X_CAP_RST");

/* device video channel index */
static int ch_id[OV7725_DEV_MAX]= {0};
module_param_array(ch_id, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_id, "Video Channel Index");

struct ov7725_dev_t {
    int                     index;                          ///< device index
    char                    name[16];                       ///< device name
    struct semaphore        lock;                           ///< device locker
    struct miscdevice       miscdev;                        ///< device node for ioctl access
    OV7725_VMODE_T          vmode;                          ///< device current video output mode
};

static struct i2c_client     *ov7725_i2c_client = NULL;
static struct ov7725_dev_t    ov7725_dev[OV7725_DEV_MAX];
static struct proc_dir_entry *ov7725_proc_root[OV7725_DEV_MAX]  = {[0 ... (OV7725_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *ov7725_proc_vmode[OV7725_DEV_MAX] = {[0 ... (OV7725_DEV_MAX - 1)] = NULL};

static u8 OV7725_VGA_CFG[] = {
    0x12, 0x80,
    0x12, 0x20,
    0x0c, 0xc0,
    0x3d, 0x03,
    0x17, 0x22,
    0x18, 0xa4,
    0x19, 0x04,
    0x1a, 0xf4,
    0x32, 0x02,
    0x29, 0xa0,
    0x2c, 0xf4,
    0x2a, 0x02,
    0x11, 0x01,  ///< 00/01/03/07 for 60/30/15/7.5fps
    0x42, 0x7f,
    0x4d, 0x09,
    0x63, 0xe0,
    0x64, 0xff,
    0x65, 0x20,
    0x66, 0x00,
    0x67, 0x48,
    0x13, 0xf0,
    0x0d, 0x41,  ///< 51/61/71 for different AEC/AGC window
    0x0f, 0xc5,
    0x14, 0x11,
    0x22, 0x7f,  ///< ff/7f/3f/1f for 60/30/15/7.5fps
    0x23, 0x03,  ///< 01/03/07/0f for 60/30/15/7.5fps
    0x24, 0x40,
    0x25, 0x30,
    0x26, 0xa1,
    0x2b, 0x00,  ///< 00/9e for 60/50Hz
    0x6b, 0xaa,
    0x13, 0xff,
    0x90, 0x05,
    0x91, 0x01,
    0x92, 0x05,
    0x93, 0x00,
    0x94, 0x78,
    0x95, 0x64,
    0x96, 0x14,
    0x97, 0x12,
    0x98, 0x72,
    0x99, 0x84,
    0x9a, 0x1e,
    0x9b, 0x08,
    0x9c, 0x20,
    0x9e, 0x00,
    0x9f, 0x00,
    0xa6, 0x04,
    0x7e, 0x0c,
    0x7f, 0x16,
    0x80, 0x2a,
    0x81, 0x4e,
    0x82, 0x61,
    0x83, 0x6f,
    0x84, 0x7b,
    0x85, 0x86,
    0x86, 0x8e,
    0x87, 0x97,
    0x88, 0xa4,
    0x89, 0xaf,
    0x8a, 0xc5,
    0x8b, 0xd7,
    0x8c, 0xe8,
    0x8d, 0x20,
};

static int ov7725_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    ov7725_i2c_client = client;
    return 0;
}

static int ov7725_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id ov7725_i2c_id[] = {
    { "ov7725_i2c", 0 },
    { }
};

static struct i2c_driver ov7725_i2c_driver = {
    .driver = {
        .name  = "OV7725_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = ov7725_i2c_probe,
    .remove   = ov7725_i2c_remove,
    .id_table = ov7725_i2c_id
};

static int ov7725_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&ov7725_i2c_driver);
    if(ret < 0) {
        printk("OV7725 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = iaddr[0]>>1;
    strlcpy(info.type, "ov7725_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("OV7725 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("OV7725 can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&ov7725_i2c_driver);
   return -ENODEV;
}

static void ov7725_i2c_unregister(void)
{
    i2c_unregister_device(ov7725_i2c_client);
    i2c_del_driver(&ov7725_i2c_driver);
    ov7725_i2c_client = NULL;
}

static int ov7725_proc_vmode_show(struct seq_file *sfile, void *v)
{
    int i, vmode;
    struct ov7725_dev_t *pdev = (struct ov7725_dev_t *)sfile->private;
    char *vmode_str[] = {"VGA_640x480"};

    down(&pdev->lock);

    seq_printf(sfile, "\n[OV7725#%d]\n", pdev->index);
    for(i=0; i<OV7725_VMODE_MAX; i++) {
        seq_printf(sfile, "%02d: %s\n", i, vmode_str[i]);
    }
    seq_printf(sfile, "----------------------------------\n");

    vmode = ov7725_video_get_mode(pdev->index);
    seq_printf(sfile, "Current==> %s\n\n", (vmode >= 0 && vmode < OV7725_VMODE_MAX) ? vmode_str[vmode] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static int ov7725_proc_vmode_open(struct inode *inode, struct file *file)
{
    return single_open(file, ov7725_proc_vmode_show, PDE(inode)->data);
}

static struct file_operations ov7725_proc_vmode_ops = {
    .owner  = THIS_MODULE,
    .open   = ov7725_proc_vmode_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void ov7725_proc_remove(int id)
{
    if(id >= OV7725_DEV_MAX)
        return;

    if(ov7725_proc_root[id]) {
        if(ov7725_proc_vmode[id]) {
            remove_proc_entry(ov7725_proc_vmode[id]->name, ov7725_proc_root[id]);
            ov7725_proc_vmode[id] = NULL;
        }

        remove_proc_entry(ov7725_proc_root[id]->name, NULL);
        ov7725_proc_root[id] = NULL;
    }
}

static int ov7725_proc_init(int id)
{
    int ret = 0;

    if(id >= OV7725_DEV_MAX)
        return -1;

    /* root */
    ov7725_proc_root[id] = create_proc_entry(ov7725_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!ov7725_proc_root[id]) {
        printk("create proc node '%s' failed!\n", ov7725_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    ov7725_proc_root[id]->owner = THIS_MODULE;
#endif

    /* vmode */
    ov7725_proc_vmode[id] = create_proc_entry("vmode", S_IRUGO|S_IXUGO, ov7725_proc_root[id]);
    if(!ov7725_proc_vmode[id]) {
        printk("create proc node '%s/vmode' failed!\n", ov7725_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    ov7725_proc_vmode[id]->proc_fops = &ov7725_proc_vmode_ops;
    ov7725_proc_vmode[id]->data      = (void *)&ov7725_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    ov7725_proc_vmode[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    ov7725_proc_remove(id);
    return ret;
}

int ov7725_get_device_num(void)
{
    return dev_num;
}
EXPORT_SYMBOL(ov7725_get_device_num);

int ov7725_get_vch_id(int id)
{
    if(id >= dev_num)
        return 0;

    return ch_id[id];
}
EXPORT_SYMBOL(ov7725_get_vch_id);

int ov7725_i2c_write(u8 id, u8 reg, u8 data)
{
    u8  buf[2];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return -1;

    if(!ov7725_i2c_client) {
        printk("OV7725 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(ov7725_i2c_client->dev.parent);

    buf[0] = reg;
    buf[1] = data;

    msgs.addr  = iaddr[id]>>1;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("OV7725#%d i2c write failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(ov7725_i2c_write);

int ov7725_i2c_read(u8 id, u8 reg)
{
    u8  data = 0;
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if(id >= dev_num)
        return 0;

    if(!ov7725_i2c_client) {
        printk("OV7725 i2c_client not register!!\n");
        return 0;
    }

    adapter = to_i2c_adapter(ov7725_i2c_client->dev.parent);

    msgs[0].addr  = iaddr[id]>>1;
    msgs[0].flags = 0; /* write */
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    msgs[1].addr  = (iaddr[id] + 1)>>1;
    msgs[1].flags = 1; /* read */
    msgs[1].len   = 1;
    msgs[1].buf   = &data;

    if(i2c_transfer(adapter, msgs, 2) != 2) {
        printk("OV7725#%d i2c read failed!!\n", id);
        return -1;
    }

    return data;
}
EXPORT_SYMBOL(ov7725_i2c_read);

int ov7725_video_get_mode(int id)
{
    if(id >= dev_num)
        return OV7725_VMODE_MAX;

    return ov7725_dev[id].vmode;
}
EXPORT_SYMBOL(ov7725_video_get_mode);

int ov7725_video_set_mode(int id, OV7725_VMODE_T v_mode)
{
    int i, ret = 0;

    if((id >= dev_num) || (v_mode >= OV7725_VMODE_MAX))
        return -1;

    down(&ov7725_dev[id].lock);

    if(v_mode != ov7725_dev[id].vmode) {
        switch(v_mode) {
            case OV7725_VMODE_VGA:
                for(i=0; i<(ARRAY_SIZE(OV7725_VGA_CFG)/2); i++) {
                    ret = ov7725_i2c_write(id, OV7725_VGA_CFG[i*2], OV7725_VGA_CFG[(i*2)+1]);
                    if(ret < 0)
                        goto exit;
                    udelay(10);
                }
                break;
            default:
                break;
        }
        ov7725_dev[id].vmode = v_mode;
    }

exit:
    up(&ov7725_dev[id].lock);

    return ret;
}
EXPORT_SYMBOL(ov7725_video_set_mode);

static void ov7725_hw_reset(void)
{
    if(!rstb_used || !init)
        return;

    /* Active Low */
    plat_gpio_set_value(rstb_used, 0);

    udelay(10);

    /* Active High and release reset */
    plat_gpio_set_value(rstb_used, 1);

    udelay(10);
}

static int ov7725_device_init(int id)
{
    int ret = 0;

    if(id >= OV7725_DEV_MAX)
        return -1;

    if(!init) {
        ov7725_dev[id].vmode = vmode[id];
        goto exit;
    }

    ret = ov7725_video_set_mode(id, vmode[id]);
    if(ret < 0)
        goto exit;

exit:
    printk("OV7725#%d Init...%s!\n", id, ((ret == 0) ? "OK" : "Fail"));

    return ret;
}

static int __init ov7725_init(void)
{
    int i, ret;

    /* platfrom check */
    plat_identifier_check();

    /* check device number */
    if(dev_num > OV7725_DEV_MAX) {
        printk("OV7725 dev_num=%d invalid!!(Max=%d)\n", dev_num, OV7725_DEV_MAX);
        return -1;
    }

    /* register i2c client */
    ret = ov7725_i2c_register();
    if(ret < 0)
        return -1;

    /* register gpio pin for device rstb pin */
    if(rstb_used && init) {
        ret = plat_register_gpio_pin(rstb_used, PLAT_GPIO_DIRECTION_OUT, 0);
        if(ret < 0)
            goto err;
    }

    /* setup platfrom external clock */
    if((init == 1) && (clk_used >= 0)) {
        if(clk_used) {
            ret = plat_extclk_set_src(clk_src);
            if(ret < 0)
                goto err;

            if(clk_used & 0x1) {
                ret = plat_extclk_set_freq(0, clk_freq);
                if(ret < 0)
                    goto err;
            }

            if(clk_used & 0x2) {
                ret = plat_extclk_set_freq(1, clk_freq);
                if(ret < 0)
                    goto err;
            }

            ret = plat_extclk_onoff(1);
            if(ret < 0)
                goto err;

        }
        else {
            /* use external oscillator. disable SoC external clock output */
            ret = plat_extclk_onoff(0);
            if(ret < 0)
                goto err;
        }
    }

    /* hardware reset for all device */
    ov7725_hw_reset();

    /* OV7725 init */
    for(i=0; i<dev_num; i++) {
        ov7725_dev[i].index = i;
        ov7725_dev[i].vmode = -1;   ///< init value

        sprintf(ov7725_dev[i].name, "ov7725.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&ov7725_dev[i].lock, 1);
#else
        init_MUTEX(&ov7725_dev[i].lock);
#endif
        ret = ov7725_proc_init(i);
        if(ret < 0)
            goto err;

        ret = ov7725_device_init(i);
        if(ret < 0)
            goto err;
    }

    return 0;

err:
    ov7725_i2c_unregister();

    for(i=0; i<OV7725_DEV_MAX; i++) {
        if(ov7725_dev[i].miscdev.name)
            misc_deregister(&ov7725_dev[i].miscdev);

        ov7725_proc_remove(i);
    }

    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    return ret;
}

static void __exit ov7725_exit(void)
{
    int i;

    ov7725_i2c_unregister();

    for(i=0; i<OV7725_DEV_MAX; i++) {
        /* remove device node */
        if(ov7725_dev[i].miscdev.name)
            misc_deregister(&ov7725_dev[i].miscdev);

        /* remove proc node */
        ov7725_proc_remove(i);
    }

    /* release gpio pin */
    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);
}

module_init(ov7725_init);
module_exit(ov7725_exit);

MODULE_DESCRIPTION("Grain Media OV7725 CMOS Sensor Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
