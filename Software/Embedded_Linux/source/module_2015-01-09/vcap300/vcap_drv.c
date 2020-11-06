/**
 * @file vcap_drv.c
 *  vcap300 driver.
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.10 $
 * $Date: 2014/11/27 01:54:55 $
 *
 * ChangeLog:
 *  $Log: vcap_drv.c,v $
 *  Revision 1.10  2014/11/27 01:54:55  jerson_l
 *  1. modify panic stop procedure
 *
 *  Revision 1.9  2014/10/30 01:47:32  jerson_l
 *  1. support vi 2ch dual edge hybrid channel mode.
 *  2. support grab channel horizontal blanking data.
 *
 *  Revision 1.8  2014/07/24 05:49:55  jerson_l
 *  1. remove vcap i2c register
 *
 *  Revision 1.7  2014/04/18 01:44:11  jerson_l
 *  1. modify channel timeout detect procedure without using kernel timer
 *
 *  Revision 1.6  2014/03/06 04:01:58  jerson_l
 *  1. add ext_irq_src module parameter to support extra ll_done interrupt source
 *
 *  Revision 1.5  2013/08/13 07:32:20  jerson_l
 *  1. remove PANIC string keyword
 *
 *  Revision 1.4  2013/01/15 09:37:34  jerson_l
 *  1. add platfrom pmu config control
 *
 *  Revision 1.3  2012/12/11 09:23:37  jerson_l
 *  1. add log system panic handler for dump videograph information
 *
 *  Revision 1.2  2012/10/24 06:47:51  jerson_l
 *
 *  switch file to unix format
 *
 *  Revision 1.1.1.1  2012/10/23 08:16:31  jerson_l
 *  import vcap300 driver
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>

#include "bits.h"
#include "vcap_drv.h"
#include "vcap_proc.h"
#include "vcap_vg.h"
#include "vcap_dbg.h"
#include "vcap_log.h"
#include "vcap_plat.h"

static const char VCAP_NAME[] ={"VCAP300"};

/*
 * External Function Prototype
 */
extern int  vcap_input_interface_register(void);
extern void vcap_input_interface_unregister(void);
#ifdef VCAP_I2C_SUPPORT
extern int  vcap_i2c_register(void);
extern void vcap_i2c_unregister(void);
#endif
extern int  vcap_lli_construct(struct vcap_dev_info_t *pdev_info, int index, u32 addr_base, int irq, const char *sname);

static struct proc_dir_entry *vcap_drv_proc_version = NULL;

static int vcap_drv_proc_version_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;

    if(off > 0)
        goto end;

    len += sprintf(page+len, "Version: %s\n", VCAP_MODULE_VERSION);
    *eof = 1;

end:
    return len;
}

static int vcap_drv_proc_init(void)
{
    int ret = 0;

    vcap_drv_proc_version = vcap_proc_create_entry("version", S_IRUGO|S_IXUGO, NULL);
    if(!vcap_drv_proc_version) {
        vcap_err("create proc node 'version' failed!\n");
        ret = -EINVAL;
        goto end;
    }
    vcap_drv_proc_version->read_proc = &vcap_drv_proc_version_read;

end:
    return ret;
}

static void vcap_drv_proc_remove(void)
{
    if(vcap_drv_proc_version)
        vcap_proc_remove_entry(NULL, vcap_drv_proc_version);
}

static int vcap_drv_panic_handler(int data)
{
    int i;
    struct vcap_dev_info_t *pdev_info;

    vcap_log("<<< PANIC!! Processing Start >>>\n");

    for(i=0; i<VCAP_DEV_MAX; i++) {
        pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(i);

        if(pdev_info) {
            if(pdev_info->fatal_stop)
                pdev_info->fatal_stop(pdev_info);
        }
    }

    vcap_log("<<< PANIC!! Processing End >>>\n");

    return 0;
}

static int vcap_drv_panic_printout_handler(int data)
{
    vcap_log("<<< PrintOut Start >>>\n");

    vcap_vg_panic_dump_job();

    vcap_log("<<< PrintOut End >>>\n");

    return 0;
}

static int vcap_drv_probe_lli(struct platform_device *pdev)
{
    int ret = 0;
    int dev_irq;
    struct resource *dev_res;
    struct vcap_dev_drv_info_t *pdev_drv_info;
    struct vcap_drv_info_t *pdrv_info;
    struct vcap_dev_info_t *pdev_info;

    /* get device irq */
    dev_irq = platform_get_irq(pdev, 0);
    if (dev_irq < 0) {
        vcap_err("vcap#%d IRQ resource does not exit!\n", pdev->id);
        ret = -EINVAL;
        goto end;
    }

    /* get device resource */
    dev_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(dev_res == NULL) {
        vcap_err("vcap#%d memory resource does not exit!\n", pdev->id);
        ret = -EINVAL;
        goto end;
    }

    /* allocate memory for record device driver info */
    pdev_drv_info = kzalloc(sizeof(struct vcap_dev_drv_info_t), GFP_KERNEL);
    if(pdev_drv_info == NULL) {
        vcap_err("vcap#%d allocate dev_drv_info failed!\n", pdev->id);
        ret = -ENOMEM;
        goto end;
    }

    pdrv_info          = &pdev_drv_info->drv_info;
    pdev_info          = &pdrv_info->dev_info;
    pdev_info->m_param = (struct vcap_dev_module_param_t *)pdev->dev.platform_data;
    snprintf(pdev_drv_info->name, VCAP_DRV_NAME_SIZE, "vcap%d", pdev->id);

    /* LLI driver construct */
    ret = vcap_lli_construct(pdev_info, pdev->id, dev_res->start, dev_irq, pdev_drv_info->name);
    if(ret < 0) {
        vcap_err("vcap#%d Link List mode initiation faild!\n", pdev->id);
        goto err_cons;
    }

    /* device proc */
    ret = vcap_dev_proc_init(pdev->id, pdev_drv_info->name, (void *)pdev_info);
    if(ret < 0) {
        goto err_proc;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
    sema_init(&pdrv_info->lock, 1);
#else
    init_MUTEX(&pdrv_info->lock);
#endif

    platform_set_drvdata(pdev, pdev_drv_info);

    vcap_dev_set_dev_info(pdev->id, (void *)pdev_info);

    /* videograph init */
    ret = vcap_vg_init(pdev->id, pdev_drv_info->name, VCAP_CHANNEL_MAX, VCAP_SCALER_MAX, (void *)pdev_info);
    if(ret < 0)
        goto err_vg;

    /* init vcap device */
    pdev_info->init(pdev_info);

end:
    return ret;

err_vg:
    vcap_dev_proc_remove(pdev->id);

err_proc:
    if(pdev_info->deconstruct)
        pdev_info->deconstruct(pdev_info);

err_cons:
    if(pdev_drv_info)
        kfree(pdev_drv_info);

    return ret;
}

static int vcap_drv_remove(struct platform_device *pdev)
{
    int ret = 0;
    struct vcap_dev_drv_info_t *pdev_drv_info;
    struct vcap_dev_info_t     *pdev_info;

    pdev_drv_info = platform_get_drvdata(pdev);

    if(pdev_drv_info) {
        pdev_info = &pdev_drv_info->drv_info.dev_info;
        vcap_dev_proc_remove(pdev_info->index);
        vcap_vg_close(pdev_info->index);
        pdev_info->deconstruct(pdev_info);
        kfree(pdev_drv_info);
    }

    return ret;
}

static struct platform_driver vcap_drv_lli = {
    .probe  = vcap_drv_probe_lli,
    .remove =  __devexit_p(vcap_drv_remove),
    .driver = {
            .name = "vcap_lli",
            .owner = THIS_MODULE,
        },
};

static int __init vcap_drv_init(void)
{
    int ret;

    vcap_info("%s Version: %s\n", VCAP_NAME, VCAP_MODULE_VERSION);

    /* register vcap_lli driver */
    ret = platform_driver_register(&vcap_drv_lli);
    if(ret) {
        vcap_err("register vcap_lli driver failed!\n");
        return ret;
    }

    /* register pmu register for capture pinmux control */
    ret = vcap_plat_pmu_init();
    if(ret < 0)
        goto err_pmu;

    /* register proc */
    ret = vcap_proc_register("vcap300");
    if(ret < 0) {
        vcap_err("register vcap proc failed!\n");
        goto err_proc;
    }

    /* proc init */
    ret = vcap_drv_proc_init();
    if(ret < 0)
        goto err_proc_init;

#ifdef VCAP_I2C_SUPPORT
    /* register i2c interface for input device */
    ret = vcap_i2c_register();
    if(ret < 0) {
        vcap_err("register i2c interface driver failed\n");
        goto err_i2c;
    }
#endif

    /* register input interface */
    ret = vcap_input_interface_register();
    if(ret < 0) {
        vcap_err("register vcap input interface failed!\n");
        goto err_input;
    }

    /* register log system */
    ret = register_panic_notifier(vcap_drv_panic_handler);
    if(ret < 0) {
        vcap_err("register panic notifier for log system failed!\n");
        goto end;
    }

    ret = register_printout_notifier(vcap_drv_panic_printout_handler);
    if(ret < 0) {
        vcap_err("register printout notifier for log system failed!\n");
        goto end;
    }

    return 0;

err_input:
#ifdef VCAP_I2C_SUPPORT
    vcap_i2c_unregister();

err_i2c:
#endif
    vcap_drv_proc_remove();

err_proc_init:
    vcap_proc_unregister();

err_proc:
    vcap_plat_pmu_exit();

err_pmu:
    platform_driver_unregister(&vcap_drv_lli);

end:
    return ret;
}

static void __exit vcap_drv_exit(void)
{
    vcap_input_interface_unregister();
#ifdef VCAP_I2C_SUPPORT
    vcap_i2c_unregister();
#endif
    vcap_drv_proc_remove();
    vcap_proc_unregister();
    vcap_plat_pmu_exit();
    platform_driver_unregister(&vcap_drv_lli);
}

module_init(vcap_drv_init);
module_exit(vcap_drv_exit);

MODULE_VERSION(VCAP_MODULE_VERSION);
MODULE_DESCRIPTION("Grain Media Video Capture300 Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
