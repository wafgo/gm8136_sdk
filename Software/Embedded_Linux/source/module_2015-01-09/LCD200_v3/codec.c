#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include "ffb.h"
#include "dev.h"
#include "ffb_api_compat.h"
#include "codec.h"

/*
 * Main structure defintion
 */
struct _codec_db_ {
    struct semaphore sema;
    codec_setup_t *pLcd[LCD_IP_NUM];    /* the device used by LCD */
    struct list_head codec_list;        /* codec driver list */
};

/*
 * Local variables declaration
 */
static struct _codec_db_ g_codec_device;

/*
 * MACRO definition
 */
#define DEVICE_LIST         g_codec_device.codec_list
#define SEMA_DOWN           down(&g_codec_device.sema)
#define SEMA_UP             up(&g_codec_device.sema)
#define DRIVER_UNAVAIL      -1
#define DEVICE_UNAVAIL      -1

/* -----------------------------------------------------------------------
 * codec driver register structure and functions
 * -----------------------------------------------------------------------
 */

/* return 0 for success, -1 for fail
 */
int codec_pip_new_device(unsigned char lcd_idx, codec_setup_t * new_setup_dev)
{
    codec_setup_t *setup_dev;
    int ret = 0;

    SEMA_DOWN;

    if (g_codec_device.pLcd[lcd_idx]) {
        ret = -1;
        goto exit;
    }

    /* search all devices and check if it was reagistered by another PIP. 
     */
    list_for_each_entry(setup_dev, &DEVICE_LIST, list) {
        if (setup_dev->device.output_type != new_setup_dev->device.output_type)
            continue;

        if (setup_dev->driver.codec_id != -1)
            printk("Error: This device(codecID=%d) is unsing by another PIP device! \n",
                   setup_dev->device.output_type);

        ret = -1;
        goto exit;
    }

    g_codec_device.pLcd[lcd_idx] = new_setup_dev;

    /* If the device was created by codec, update device info. */
    list_for_each_entry(setup_dev, &DEVICE_LIST, list) {
        if (setup_dev->driver.codec_id != new_setup_dev->device.output_type)
            continue;

        /* update device data */
        memcpy(&setup_dev->device, &new_setup_dev->device, sizeof(codec_device_t));

        /* proc function
         */
        if (setup_dev->driver.enc_proc_init) {
            ret = setup_dev->driver.enc_proc_init();
            if (ret < 0)
                goto exit;
        }

        /* configuration func */
        if (setup_dev->driver.setting)
            setup_dev->driver.setting(setup_dev->device.tunnel_data);

        goto exit1;
    }

    /* create a new device */
    setup_dev = kzalloc(sizeof(codec_setup_t), GFP_KERNEL);
    if (setup_dev) {
        memcpy(&setup_dev->device, &new_setup_dev->device, sizeof(codec_device_t));
        setup_dev->driver.codec_id = DRIVER_UNAVAIL;    //wait driver

        /* add to codec link list */
        INIT_LIST_HEAD(&setup_dev->list);
        list_add_tail(&setup_dev->list, &DEVICE_LIST);
    }

  exit1:
    try_module_get(THIS_MODULE);

  exit:
    SEMA_UP;
    return ret;
}

/* return 0 for success, -1 for fail
 */
int codec_pip_remove_device(unsigned char lcd_idx, codec_setup_t * old_device)
{
    codec_setup_t *setup_dev, *ne_dev;
    int ret = -1;

    if (!g_codec_device.pLcd[lcd_idx])
        return -1;

    SEMA_DOWN;

    list_for_each_entry_safe(setup_dev, ne_dev, &DEVICE_LIST, list) {
        if (setup_dev->device.output_type != old_device->device.output_type)
            continue;

        ret = 0;
        /* Found it.
         * If the system is running this driver, we need to unload it. 
         */
        if (setup_dev->driver.codec_id == setup_dev->device.output_type) {
            /* remove proc. */
            if (setup_dev->driver.enc_proc_remove)
                setup_dev->driver.enc_proc_remove();

            /* remove driver */
            if (setup_dev->driver.remove)
                setup_dev->driver.remove(setup_dev->driver.private);
        }

        /* marked this device is removed */
        setup_dev->device.output_type = DEVICE_UNAVAIL;

        if ((setup_dev->device.output_type == DEVICE_UNAVAIL)
            && (setup_dev->driver.codec_id == DRIVER_UNAVAIL)) {
            /* remove this node */
            list_del_init(&setup_dev->list);
            kfree(setup_dev);
        }
    }

    if (!ret) {
        g_codec_device.pLcd[lcd_idx] = NULL;
        module_put(THIS_MODULE);
    }

    SEMA_UP;

    return ret;
}

/* Add a codec driver to the device
 * return NULL indicates the node was registered by other codec already
 */
static codec_setup_t *codec_driver_add(codec_driver_t * driver)
{
    codec_setup_t *setup_dev;

    SEMA_DOWN;

    /* check if registered driver already */
    list_for_each_entry(setup_dev, &DEVICE_LIST, list) {
        if (setup_dev->driver.codec_id == DRIVER_UNAVAIL)
            continue;

        if (setup_dev->driver.codec_id != driver->codec_id)
            continue;

        printk("This codec driver was already registered by %s!\n", setup_dev->driver.name);
        setup_dev = NULL;

        goto exit;
    }

    /* If this device was registered by PIP, update the driver */
    list_for_each_entry(setup_dev, &DEVICE_LIST, list) {
        if (setup_dev->device.output_type != driver->codec_id)
            continue;

        /* update driver */
        memcpy(&setup_dev->driver, driver, sizeof(codec_driver_t));
        goto exit;
    }

    /* new a new device */
    setup_dev = kzalloc(sizeof(codec_setup_t), GFP_KERNEL);
    if (setup_dev) {
        /* update the driver */
        memcpy(&setup_dev->driver, driver, sizeof(codec_driver_t));
        setup_dev->device.output_type = DEVICE_UNAVAIL; /* wait pip device */

        /* add to codec link list */
        INIT_LIST_HEAD(&setup_dev->list);
        list_add_tail(&setup_dev->list, &DEVICE_LIST);
    }

  exit:
    SEMA_UP;

    return setup_dev;
}

/* 
 * register the codec driver. The probe function will be called immediately while registering
 * return value: 0 for success, < 0 for fail
 */
int codec_driver_register(codec_driver_t * driver)
{
    int ret = 0;
    codec_setup_t *setup_dev;

    /* sanity check
     */
    if (driver == NULL)
        return -1;

    if (!strlen(driver->name) || strlen(driver->name) > NAME_LEN)
        return -1;

    /* hook the driver to the device */
    setup_dev = codec_driver_add(driver);
    if (setup_dev == NULL)
        return -1;

    /* initialization */
    if (driver->probe) {
        ret = driver->probe(driver->private);
        if (ret < 0)
            return ret;
    }

    /* if the pip had registered the device already */
    if (setup_dev->device.output_type != DEVICE_UNAVAIL) {
        if (driver->enc_proc_init) {
            ret = driver->enc_proc_init();
            if (ret < 0) {
                setup_dev->driver.codec_id = DRIVER_UNAVAIL;
                return ret;
            }
        }

        if (driver->setting)
            driver->setting(setup_dev->device.tunnel_data);
    }

    printk("Register codec %s. \n", setup_dev->driver.name);

    try_module_get(THIS_MODULE);
    return 0;
}

/* 
 * de-register the codec driver.  
 */
int codec_driver_deregister(codec_driver_t * driver)
{
    codec_setup_t *setup_dev, *ne_dev;
    int ret = -1;

    if (driver->codec_id == DRIVER_UNAVAIL)
        return -1;

    SEMA_DOWN;

    list_for_each_entry_safe(setup_dev, ne_dev, &DEVICE_LIST, list) {
        if (setup_dev->driver.codec_id != driver->codec_id)
            continue;

        /* If the system is running this driver, we need to unload it */
        if (setup_dev->device.output_type == driver->codec_id) {
            /* remove proc. */
            if (setup_dev->driver.enc_proc_remove)
                setup_dev->driver.enc_proc_remove();

            /* remove driver */
            if (setup_dev->driver.remove)
                setup_dev->driver.remove(setup_dev->driver.private);
        }

        /* marked this driver is removed */
        setup_dev->driver.codec_id = DRIVER_UNAVAIL;
        printk("Deregister codec %s.\n", setup_dev->driver.name);

        if ((setup_dev->device.output_type == DEVICE_UNAVAIL)
            && (setup_dev->driver.codec_id == DRIVER_UNAVAIL)) {
            /* remove this node */
            list_del_init(&setup_dev->list);
            kfree(setup_dev);
        }

        module_put(THIS_MODULE);
        ret = 0;
    }

    SEMA_UP;

    return ret;
}

/* ---------------------------------------------------------------------------------
 * Proc function
 * ---------------------------------------------------------------------------------
 */
/* 
 * dump the cat6611/6612 register
 */
void codec_dump_database(void)
{
    codec_setup_t *setup_dev;

    printk("Codec registeration information: \n");

    SEMA_DOWN;

    /* If this device was registered by PIP, update the driver */
    list_for_each_entry(setup_dev, &DEVICE_LIST, list) {
        printk("Device otuput_type = %d \n", setup_dev->device.output_type);
        printk("Driver codec ID = %d \n", setup_dev->driver.codec_id);
        if (setup_dev->driver.codec_id != DRIVER_UNAVAIL)
            printk("Driver name = %s \n", setup_dev->driver.name);

        printk("\n");
    }
    SEMA_UP;
}

/* Get the system output_type
 */
OUTPUT_TYPE_T codec_get_output_type(int lcd_idx)
{
    OUTPUT_TYPE_T ret = -1;

    if (g_codec_device.pLcd[lcd_idx])
        ret = g_codec_device.pLcd[lcd_idx]->device.output_type;

    return ret;
}

/*
 * Proc functions to show driver registeration information.
 */
static struct proc_dir_entry *codec_driver_proc = NULL;

static int proc_read_codec_reginfo(char *page, char **start, off_t off, int count,
                                   int *eof, void *data)
{
    int len = 0;

    codec_dump_database();

    *eof = 1;                   //end of file
    *start = page + off;
    len = len - off;
    return len;
}

/*
 * Entry point of this module
 */
void __init codec_middleware_init(void)
{
    memset(&g_codec_device, 0, sizeof(g_codec_device));

    /* init semaphore */
    sema_init(&g_codec_device.sema, 1);
    /* init codec driver list */
    INIT_LIST_HEAD(&g_codec_device.codec_list);

    /* 
     * create proc
     */
    codec_driver_proc =
        ffb_create_proc_entry("codec_reg_info", S_IRUGO | S_IXUGO, flcd_common_proc);
    if (codec_driver_proc == NULL)
        panic("Fail to create codec_reg_info proc!\n");

    codec_driver_proc->read_proc = (read_proc_t *) proc_read_codec_reginfo;
    codec_driver_proc->write_proc = NULL;    
}

void codec_middle_exit(void)
{
    codec_setup_t *setup_dev, *ne_dev;

    list_for_each_entry_safe(setup_dev, ne_dev, &DEVICE_LIST, list) {
        /* remove this node */
        list_del_init(&setup_dev->list);
        kfree(setup_dev);
    }

    /* Remove proc.
     */
    if (codec_driver_proc)
        ffb_remove_proc_entry(flcd_common_proc, codec_driver_proc);
    codec_driver_proc = NULL;
}

EXPORT_SYMBOL(codec_driver_register);
EXPORT_SYMBOL(codec_driver_deregister);
EXPORT_SYMBOL(codec_pip_new_device);
EXPORT_SYMBOL(codec_pip_remove_device);
EXPORT_SYMBOL(codec_dump_database);
EXPORT_SYMBOL(codec_get_output_type);

MODULE_DESCRIPTION("GM codec database");
MODULE_AUTHOR("GM Technology Corp.");
MODULE_LICENSE("GPL");
