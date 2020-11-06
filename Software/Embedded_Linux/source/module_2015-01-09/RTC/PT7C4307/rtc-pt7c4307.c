/**
 * @file rtc-pt7c4307.c
 * PT7C4307 Real-Time Clock driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.2 $
 * $Date: 2013/06/05 02:56:28 $
 *
 * ChangeLog:
 *  $Log: rtc-pt7c4307.c,v $
 *  Revision 1.2  2013/06/05 02:56:28  jerson_l
 *  1. fix rtc hour display value incorrect problem
 *
 *  Revision 1.1.1.1  2013/04/18 05:46:59  jerson_l
 *  add RTC driver
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/rtc.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/bcd.h>

/* Register map */
/* rtc section */
#define PT7C_REG_SC         0x00
#define PT7C_REG_MN         0x01
#define PT7C_REG_HR         0x02
#define PT7C_REG_HR_MIL     (1<<6) /* 24h/12h mode */
#define PT7C_REG_HR_PM      (1<<5) /* PM/AM bit in 12h mode */
#define PT7C_REG_DW         0x03
#define PT7C_REG_DT         0x04
#define PT7C_REG_MO         0x05
#define PT7C_REG_YR         0x06

#define RTC_SECTION_LEN     7

/* i2c configuration */
#define PT7C_I2C_ADDR       0xd0

static struct i2c_client *rtc_i2c_client = NULL;
static unsigned long rtc_status;
static volatile unsigned long rtc_irq_data;
static unsigned long rtc_freq = 1;
static unsigned AIE_stat = 0;

static struct fasync_struct *rtc_async_queue;
static DECLARE_WAIT_QUEUE_HEAD(rtc_wait);

static struct proc_dir_entry *rtc_pt7c4307_proc_rtc = NULL;

extern spinlock_t rtc_lock;

/* block read */
static int i2c_read_regs(u8 reg, u8 buf[], unsigned len)
{
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if(!rtc_i2c_client) {
        printk("rtc_i2c_client not register\n");
        return -1;
    }

    adapter = to_i2c_adapter(rtc_i2c_client->dev.parent);

    buf[0] = reg;
    msgs[0].addr  = PT7C_I2C_ADDR>>1;
    msgs[0].flags = 0;  //normal write
    msgs[0].len   = 1;
    msgs[0].buf   = buf;

    msgs[1].addr  = PT7C_I2C_ADDR>>1;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len   = len+1;
    msgs[1].buf   = buf;

    if(unlikely(i2c_transfer(adapter, msgs, 2) != 2)) {
        return -1;
    }

    return 0;
}

/* block write */
static int i2c_set_regs(u8 reg, u8 const buf[], unsigned len)
{
    u8 i2c_buf[7];
    struct i2c_msg msgs[1];
    struct i2c_adapter *adapter;

    if(!rtc_i2c_client) {
        printk("rtc_i2c_client not register\n");
        return -1;
    }

    adapter = to_i2c_adapter(rtc_i2c_client->dev.parent);

    i2c_buf[0] = reg;
    memcpy(&i2c_buf[1], &buf[0], len);

    msgs[0].addr  = PT7C_I2C_ADDR>>1;
    msgs[0].flags = 0;
    msgs[0].len   = len + 1;
    msgs[0].buf   = i2c_buf;

    if(unlikely(i2c_transfer(adapter, msgs, 1) != 1)) {
        return -1;
    }

    return 0;
}

static int set_time(struct rtc_time const *tm)
{
    int sr;
    u8 regs[RTC_SECTION_LEN] = { 0, };

    regs[PT7C_REG_SC] = bin2bcd(tm->tm_sec);
    regs[PT7C_REG_MN] = bin2bcd(tm->tm_min);
    regs[PT7C_REG_HR] = bin2bcd(tm->tm_hour);

    regs[PT7C_REG_DT] = bin2bcd(tm->tm_mday);
    regs[PT7C_REG_MO] = bin2bcd(tm->tm_mon + 1);
    regs[PT7C_REG_YR] = bin2bcd(tm->tm_year - 100);

    regs[PT7C_REG_DW] = bin2bcd(tm->tm_wday & 7);

    /* write RTC registers */
    sr = i2c_set_regs(0, regs, RTC_SECTION_LEN);
    if (sr < 0) {
        printk("%s: writing RTC section failed\n", __FUNCTION__);
        return sr;
    }

    return 0;
}

static int read_time(struct rtc_time *tm)
{
    int sr;
    u8 regs[RTC_SECTION_LEN] = { 0, };
    sr = i2c_read_regs(0, regs, RTC_SECTION_LEN);
    if (sr < 0) {
        printk("%s: reading RTC section failed\n", __FUNCTION__);
        return sr;
    }
    tm->tm_sec = bcd2bin(regs[PT7C_REG_SC]);
    tm->tm_min = bcd2bin(regs[PT7C_REG_MN]);
    { /* HR field has a more complex interpretation */
        const u8 _hr = regs[PT7C_REG_HR];
        if ((_hr & PT7C_REG_HR_MIL) == 0){ /* 24h format */
            tm->tm_hour = bcd2bin(_hr & 0x3f);
        }
        else { // 12h format
            tm->tm_hour = bcd2bin(_hr & 0x1f);
            if (_hr & PT7C_REG_HR_PM) /* PM flag set */
                tm->tm_hour += 12;
        }
    }

    tm->tm_mday = bcd2bin(regs[PT7C_REG_DT]);
    tm->tm_mon  = bcd2bin(regs[PT7C_REG_MO]) - 1; /* rtc starts at 1 */
    tm->tm_year = bcd2bin(regs[PT7C_REG_YR]) + 100;
    tm->tm_wday = bcd2bin(regs[PT7C_REG_DW]);

    return 0;
}

static int rtc_open(struct inode *inode, struct file *file)
{
    if(test_and_set_bit (1, &rtc_status)){
        return -EBUSY;
    }
    rtc_irq_data = 0;
    return 0;
}

static int rtc_release(struct inode *inode, struct file *file)
{
    rtc_status = 0;
    return 0;
}

static int rtc_fasync(int fd, struct file *filp, int on)
{
    return fasync_helper(fd, filp, on, &rtc_async_queue);
}

static unsigned int rtc_poll(struct file *file, poll_table *wait)
{
    poll_wait(file, &rtc_wait, wait);
    return (rtc_irq_data) ? 0 : POLLIN | POLLRDNORM;
}

static loff_t rtc_llseek(struct file *file, loff_t offset, int origin)
{
    return -ESPIPE;
}

ssize_t rtc_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    DECLARE_WAITQUEUE(wait, current);
    unsigned long data;
    ssize_t retval;

    if(count < sizeof(unsigned long)){
        return -EINVAL;
    }

    add_wait_queue(&rtc_wait, &wait);
    set_current_state(TASK_INTERRUPTIBLE);
    for (;;) {
        spin_lock_irq (&rtc_lock);
        data = rtc_irq_data;
        if (data != 0) {
            rtc_irq_data = 0;
            break;
        }
        spin_unlock_irq (&rtc_lock);

        if (file->f_flags & O_NONBLOCK) {
            retval = -EAGAIN;
            goto out;
        }

        if (signal_pending(current)) {
            retval = -ERESTARTSYS;
            goto out;
        }

        schedule();
    }

    spin_unlock_irq (&rtc_lock);

    data -= 0x100;  /* the first IRQ wasn't actually missed */

    retval = put_user(data, (unsigned long *)buf);
    if(!retval){
        retval = sizeof(unsigned long);
    }
out:
    set_current_state(TASK_RUNNING);
    remove_wait_queue(&rtc_wait, &wait);

    return retval;
}

static long rtc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct rtc_time tm;

    switch(cmd){
        case RTC_AIE_OFF:
            printk("Not Support\n");
            return 0;
        case RTC_AIE_ON:
            printk("Not Support\n");
            return 0;

        case RTC_ALM_READ:
            printk("Not Support\n");
            return 0;

        case RTC_ALM_SET:
            printk("Not Support\n");
            return 0;

        case RTC_RD_TIME:
            read_time(&tm);
            break;

        case RTC_SET_TIME:
            {
                if(!capable(CAP_SYS_TIME)){
                    return -EACCES;
                }

                if(copy_from_user(&tm, (struct rtc_time*)arg, sizeof (tm))){
                    return -EFAULT;
                }

                set_time(&tm);
            }
            return 0;

        case RTC_IRQP_READ:
            return put_user(rtc_freq, (unsigned long *)arg);

        case RTC_IRQP_SET:
            if(arg != 1){
                 return -EINVAL;
            }

            return 0;

        case RTC_EPOCH_READ:
            return put_user (1970, (unsigned long *)arg);

        default:
            return -EINVAL;
    }

    return copy_to_user ((void *)arg, &tm, sizeof (tm)) ? -EFAULT : 0;
}

static struct file_operations rtc_fops = {
    owner         : THIS_MODULE,
    llseek        : rtc_llseek,
    read          : rtc_read,
    poll          : rtc_poll,
    unlocked_ioctl: rtc_ioctl,
    open          : rtc_open,
    release       : rtc_release,
    fasync        : rtc_fasync,
};

static struct miscdevice ftrtc010rtc_miscdev = {
    RTC_MINOR,
    "rtc",
    &rtc_fops
};

static int rtc_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    char *p = page;
    int len;
    struct rtc_time tm;

    read_time(&tm);

    p += sprintf(p, "rtc_time\t: %02d:%02d:%02d\n"
                    "rtc_date\t: %04d-%02d-%02d\n"
                    "rtc_epoch\t: %04d\n",
                    tm.tm_hour, tm.tm_min, tm.tm_sec,
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 2000);
    p += sprintf(p, "alrm_time\t: Not Support\n"
                    "alrm_date\t: Not Support\n");
    p += sprintf(p, "alarm_IRQ\t: %s\n", AIE_stat ? "yes" : "no" );

    len = (p - page) - off;
    if (len < 0){
        len = 0;
    }

    *eof = (len <= count) ? 1 : 0;
    *start = page + off;

    return len;
}

static int __devinit pt7c4307_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    rtc_i2c_client = client;
    return 0;
}

static int __devexit pt7c4307_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id pt7c4307_id[] = {
    { "rtc-pt7c4307", 0 },
    { }
};

static struct i2c_driver pt7c4307_driver = {
    .driver = {
        .name   = "rtc-pt7c4307",
        .owner  = THIS_MODULE,
    },
    .probe      = pt7c4307_probe,
    .remove     = __devexit_p(pt7c4307_remove),
    .id_table   = pt7c4307_id
};

static struct i2c_board_info pt7c4307_device = {
    .type       = "rtc-pt7c4307",
    .addr       = (PT7C_I2C_ADDR>>1)
};

static int __init pt7c4307_init(void)
{
    int ret = 0;
    struct i2c_adapter *adapter;
    struct i2c_client  *client = NULL;

    /* create rtc misc device node */
    ret = misc_register(&ftrtc010rtc_miscdev);
    if(ret < 0) {
        printk("%s fail: misc_register not OK.\n", __FUNCTION__);
        return ret;
    }

    /* add i2c driver */
    ret = i2c_add_driver(&pt7c4307_driver);
    if(ret < 0) {
        printk("%s fail: i2c_add_driver not OK.\n", __FUNCTION__);
        goto i2c_drverr;
    }

    adapter = i2c_get_adapter(0);   ///< I2C BUS#0
    if(!adapter) {
        printk("%s fail: i2c_get_adapter failed.\n", __FUNCTION__);
        ret = -ENODEV;
        goto i2c_deverr;
    }

    /* register rtc i2c device */
    client = i2c_new_device(adapter, &pt7c4307_device);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("%s fail: i2c_new_device not OK.\n", __FUNCTION__);
        ret = -ENXIO;
        goto i2c_deverr;
    }

    /* create rtc proc node */
    rtc_pt7c4307_proc_rtc = create_proc_read_entry("driver/rtc", 0, 0, rtc_read_proc, NULL);
    if(!rtc_pt7c4307_proc_rtc) {
        printk("%s fail: create_proc_read_entry not OK.\n", __FUNCTION__);
        ret = -EAGAIN;
        goto proc_err;
    }

    printk("RTC-PT7C4307 init...OK\n");

    return 0;

proc_err:
    if(client)
        i2c_unregister_device(client);

i2c_deverr:
    i2c_del_driver(&pt7c4307_driver);

i2c_drverr:
    misc_deregister(&ftrtc010rtc_miscdev);

    return ret;
}

static void __exit pt7c4307_exit(void)
{
    if(rtc_pt7c4307_proc_rtc)
        remove_proc_entry ("driver/rtc", NULL);

    if(rtc_i2c_client) {
        i2c_unregister_device(rtc_i2c_client);
        i2c_del_driver(&pt7c4307_driver);
    }

    misc_deregister (&ftrtc010rtc_miscdev);
}

module_init(pt7c4307_init);
module_exit(pt7c4307_exit);

MODULE_DESCRIPTION("Grain Media PT7C4307 RTC Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
