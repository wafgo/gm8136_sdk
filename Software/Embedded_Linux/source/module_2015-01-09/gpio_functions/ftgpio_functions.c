/**
 * @file gpio_special_use.c
 *  This source file is gpio special function driver
 * Copyright (C) 2011 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.2 $
 * $Date: 2012/07/31 07:32:45 $
 *
 * ChangeLog:
 *  $Log: ftgpio_functions.c,v $
 *  Revision 1.2  2012/07/31 07:32:45  easonli
 *  *:[GPIO] fixed bit mask bug
 *
 *  Revision 1.1  2012/07/10 06:45:02  easonli
 *  +:[GPIO] add external module for GPIO driver usage
 *
 *
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/bitops.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm-generic/gpio.h>
#include "ioctl_gpio.h"

#define GPIO_USE_NAME "ftgpio_function"

static dev_t dev_num;
static struct cdev gpio_cdev;
static struct class *gpio_class = NULL;
static u32 open_cnt = 0;
static DEFINE_SEMAPHORE(drv_sem);

//============================================
//      file operations
//============================================
static int gpio_open(struct inode *inode, struct file *filp)
{
    int ret = 0;

    down(&drv_sem);
    if (!open_cnt) {
        open_cnt++;
    } else {
        printk("request gpio functions fail\n");
        ret = -EINVAL;
    }
    up(&drv_sem);

    return ret;
}

static int gpio_release(struct inode *inode, struct file *filp)
{
    int ret = 0;

    down(&drv_sem);
    open_cnt--;
    up(&drv_sem);

    return ret;
}

static long gpio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    if (_IOC_TYPE(cmd) != GPIO_IOC_MAGIC)
        return -ENOTTY;
    if (_IOC_NR(cmd) > GPIO_IOC_MAXNR)
        return -ENOTTY;
    if (_IOC_DIR(cmd) & _IOC_READ)
        ret = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        ret = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (ret)
        return -EFAULT;

    switch (cmd) {
    case GPIO_SET_MULTI_PINS_OUT:
        {
            gpio_pin pin;
            int bit = 0;
            u32 pin_val = 0;
            if (copy_from_user(&pin, (void __user *)arg, sizeof(pin))) {
                ret = -EFAULT;
                break;
            }
            for_each_set_bit(bit, (const long unsigned int *)(&pin.port),
                             BITS_PER_BYTE * sizeof(pin.port)) {
                if ((ret = gpio_request(bit, GPIO_USE_NAME)) != 0) {
                    printk(KERN_ERR "%s: gpio_request %u failed\n", __func__, bit);
                    goto err;
                }
                pin_val =
                    (pin.value[BIT_WORD(bit)] & BIT_MASK(bit)) >> get_count_order(BIT_MASK(bit));
#if 0
                printk("\n");
                printk("%s: bit  = 0x%X\n", __func__, bit);
                printk("%s: pin.value[%d] = 0x%X\n", __func__, BIT_WORD(bit),
                       pin.value[BIT_WORD(bit)]);
                printk("%s: bit mask = 0x%X\n", __func__, BIT_MASK(bit));
                printk("%s: And Value = 0x%X\n", __func__,
                       pin.value[BIT_WORD(bit)] & BIT_MASK(bit));
                printk("%s: bit mask order = %d\n", __func__, get_count_order(BIT_MASK(bit)));
                printk("%s: pin(%d) value = 0x%X\n", __func__, bit, pin_val);
#endif
                if ((ret = gpio_direction_output(bit, pin_val)) != 0) {
                    printk(KERN_ERR "%s: gpio_direction_output %u failed\n", __func__, bit);
                    goto err;
                }
                gpio_free(bit);
            }
        }
        break;

    case GPIO_SET_MULTI_PINS_IN:
        {
            gpio_pin pin;
            int bit;
            if (copy_from_user(&pin, (void __user *)arg, sizeof(pin))) {
                ret = -EFAULT;
                break;
            }
            for_each_set_bit(bit, (const long unsigned int *)(&pin.port),
                             BITS_PER_BYTE * sizeof(pin.port)) {
                if ((ret = gpio_request(bit, GPIO_USE_NAME)) != 0) {
                    printk(KERN_ERR "%s: gpio_request %u failed\n", __func__, bit);
                    goto err;
                }
                if ((ret = gpio_direction_input(bit)) != 0) {
                    printk(KERN_ERR "%s: gpio_direction_input failed\n", __func__);
                    goto err;
                }
                gpio_free(bit);
            }
        }
        break;

    case GPIO_GET_MULTI_PINS_VALUE:
        {
            gpio_pin pin;
            u32 bit, value = 0;
            if (copy_from_user(&pin, (void __user *)arg, sizeof(pin))) {
                ret = -EFAULT;
                break;
            }
            for_each_set_bit(bit, (const long unsigned int *)(&pin.port),
                             BITS_PER_BYTE * sizeof(pin.port)) {
                if ((ret = gpio_request(bit, GPIO_USE_NAME)) != 0) {
                    printk(KERN_ERR "%s: gpio_request %u failed\n", __func__, bit);
                    goto err;
                }
                value = __gpio_get_value(bit);
                //printk("GPIO_GET_MULTI_PINS_VALUE: pin(%d) = 0x%X\n", bit, value>>get_count_order(BIT_MASK(bit)));
                (value != 0) ? (pin.value[BIT_WORD(bit)] |=
                                BIT_MASK(bit)) : (pin.value[BIT_WORD(bit)] &= ~BIT_MASK(bit));
                gpio_free(bit);
            }
            if (copy_to_user((void __user *)arg, &pin, sizeof(pin))) {
                ret = -EFAULT;
                break;
            }
        }
        break;

    default:
        ret = -ENOIOCTLCMD;
        printk(KERN_ERR "Does not support this command.(0x%x)", cmd);
        break;
    }
  err:
    return ret;
}

struct file_operations gpio_fops = {
    .owner = THIS_MODULE,
    .open = gpio_open,
    .release = gpio_release,
    .unlocked_ioctl = gpio_ioctl,
};

static int __init ftgpio_function_init(void)
{
    int ret = 0;
    ret = alloc_chrdev_region(&dev_num, 0, 1, GPIO_USE_NAME);
    if (unlikely(ret < 0)) {
        printk(KERN_ERR "%s:alloc_chrdev_region failed\n", __func__);
        goto err1;
    }
    cdev_init(&gpio_cdev, &gpio_fops);
    gpio_cdev.owner = THIS_MODULE;
    gpio_cdev.ops = &gpio_fops;
    ret = cdev_add(&gpio_cdev, dev_num, 1);
    if (unlikely(ret < 0)) {
        printk(KERN_ERR "%s:cdev_add failed\n", __func__);
        goto err3;
    }
    gpio_class = class_create(THIS_MODULE, GPIO_USE_NAME);
    if (IS_ERR(gpio_class)) {
        printk(KERN_ERR "%s:class_create failed\n", __func__);
        goto err2;
    }
    device_create(gpio_class, NULL, gpio_cdev.dev, NULL, GPIO_USE_NAME);

    printk(KERN_NOTICE "\nFTGPIO Functions Driver registered success\n");
  err1:
    return ret;
  err2:
    cdev_del(&gpio_cdev);
  err3:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit ftgpio_function_exit(void)
{
    unregister_chrdev_region(dev_num, 1);
    cdev_del(&gpio_cdev);
    device_destroy(gpio_class, dev_num);
    class_destroy(gpio_class);
}

module_init(ftgpio_function_init);
module_exit(ftgpio_function_exit);
MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("GPIO Function Driver");
MODULE_LICENSE("GPL");
