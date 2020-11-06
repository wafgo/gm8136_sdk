/**
 * @file ftrtc011.c
 * FTRTC011 Real Time Clock Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.1.1.1 $
 * $Date: 2013/10/30 06:28:14 $
 *
 * ChangeLog:
 *  $Log: ftrtc011.c,v $
 *  Revision 1.1.1.1  2013/10/30 06:28:14  jerson_l
 *  add ftrtc011 driver
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <mach/platform/platform_io.h>
#include <asm/io.h>

#include "platform.h"

#define DRIVER_NAME "ftrtc011"

static int clk_src = PLAT_RTC_CLK_SRC_OSC;
module_param(clk_src, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_src, "RTC Clock Source => 0:OSC 1:PLL3");

/******************************************************************************
 * register definitions
 *****************************************************************************/
#define FTRTC011_OFFSET_SEC             0x00
#define FTRTC011_OFFSET_MIN             0x04
#define FTRTC011_OFFSET_HOUR            0x08
#define FTRTC011_OFFSET_DAY             0x0c
#define FTRTC011_OFFSET_ALARM_SEC       0x10
#define FTRTC011_OFFSET_ALARM_MIN       0x14
#define FTRTC011_OFFSET_ALARM_HOUR      0x18
#define FTRTC011_OFFSET_CR              0x20
#define FTRTC011_OFFSET_WSEC            0x24
#define FTRTC011_OFFSET_WMIN            0x28
#define FTRTC011_OFFSET_WHOUR           0x2c
#define FTRTC011_OFFSET_WDAY            0x30
#define FTRTC011_OFFSET_INTR_STATE      0x34
#define FTRTC011_OFFSET_REVISION        0x3c
#define FTRTC011_OFFSET_RWSTATUS        0x40
#define FTRTC011_OFFSET_CURRENT         0x44
#define FTRTC011_OFFSET_SLEEPTIME       0x48

/*
 * RTC Control Register
 */
#define FTRTC011_CR_ENABLE              (1 << 0)
#define FTRTC011_CR_INTERRUPT_SEC       (1 << 1)    /* enable interrupt per second */
#define FTRTC011_CR_INTERRUPT_MIN       (1 << 2)    /* enable interrupt per minute */
#define FTRTC011_CR_INTERRUPT_HR        (1 << 3)    /* enable interrupt per hour   */
#define FTRTC011_CR_INTERRUPT_DAY       (1 << 4)    /* enable interrupt per day    */
#define FTRTC011_CR_ALARM_INTERRUPT     (1 << 5)
#define FTRTC011_CR_COUNTER_LOAD        (1 << 6)
#define FTRTC011_CR_REFRESH             (1 << 7)

/*
 * IntrState
 */
#define FTRTC011_INTR_STATE_SEC         (1 << 0)
#define FTRTC011_INTR_STATE_MIN         (1 << 1)
#define FTRTC011_INTR_STATE_HOUR        (1 << 2)
#define FTRTC011_INTR_STATE_DAY         (1 << 3)
#define FTRTC011_INTR_STATE_ALARM       (1 << 4)

/******************************************************************************
 * FTRTC011 private data
 *****************************************************************************/
struct ftrtc011 {
    spinlock_t lock;
    struct platform_device  *pdev;
    struct rtc_device       *rtc;
    void __iomem            *base;
    int                     irq;
};

/******************************************************************************
 * internal functions
 *****************************************************************************/
static unsigned long ftrtc011_get_uptime(struct ftrtc011 *ftrtc011)
{
    unsigned long sec, min, hour, day;

    sec  = inl(ftrtc011->base + FTRTC011_OFFSET_SEC);
    min  = inl(ftrtc011->base + FTRTC011_OFFSET_MIN);
    hour = inl(ftrtc011->base + FTRTC011_OFFSET_HOUR);
    day  = inl(ftrtc011->base + FTRTC011_OFFSET_DAY);

    return (sec + min * 60 + hour * 60 * 60 + day * 24 * 60 * 60);
}

static void ftrtc011_set_uptime(struct ftrtc011 *ftrtc011, unsigned long time)
{
    unsigned long sec, min, hour, day;
    unsigned int cr;

    sec  = time % 60;
    min  = (time / 60) % 60;
    hour = (time / 60 / 60) % 24;
    day  = time / 60 / 60 / 24;

    outl(sec,  ftrtc011->base + FTRTC011_OFFSET_WSEC);
    outl(min,  ftrtc011->base + FTRTC011_OFFSET_WMIN);
    outl(hour, ftrtc011->base + FTRTC011_OFFSET_WHOUR);
    outl(day,  ftrtc011->base + FTRTC011_OFFSET_WDAY);

    /* trigger refresh */
    cr = inl(ftrtc011->base + FTRTC011_OFFSET_CR);
    cr |= FTRTC011_CR_COUNTER_LOAD;
    outl(cr, ftrtc011->base + FTRTC011_OFFSET_CR);
}

static void ftrtc011_get_alarm_tm(struct ftrtc011 *ftrtc011, struct rtc_time *tm)
{
    unsigned long time;
    unsigned long sec  = inl(ftrtc011->base + FTRTC011_OFFSET_ALARM_SEC);
    unsigned long min  = inl(ftrtc011->base + FTRTC011_OFFSET_ALARM_MIN);
    unsigned long hour = inl(ftrtc011->base + FTRTC011_OFFSET_ALARM_HOUR);
    unsigned long day  = inl(ftrtc011->base + FTRTC011_OFFSET_DAY);

    /* alarm time */
    time = sec + min * 60 + hour * 60 * 60 + day * 24 * 60 * 60;

    rtc_time_to_tm(time, tm);
}

static void ftrtc011_set_alarm_tm(struct ftrtc011 *ftrtc011, struct rtc_time *tm)
{
    outl(tm->tm_sec,  ftrtc011->base + FTRTC011_OFFSET_ALARM_SEC);
    outl(tm->tm_min,  ftrtc011->base + FTRTC011_OFFSET_ALARM_MIN);
    outl(tm->tm_hour, ftrtc011->base + FTRTC011_OFFSET_ALARM_HOUR);
}

static void ftrtc011_enable_alarm_interrupt(struct ftrtc011 *ftrtc011)
{
    unsigned int cr;

    cr = inl(ftrtc011->base + FTRTC011_OFFSET_CR);
    cr |= FTRTC011_CR_ALARM_INTERRUPT;
    outl(cr, ftrtc011->base + FTRTC011_OFFSET_CR);
}

static void ftrtc011_disable_alarm_interrupt(struct ftrtc011 *ftrtc011)
{
    unsigned int cr;

    cr = inl(ftrtc011->base + FTRTC011_OFFSET_CR);
    cr &= ~FTRTC011_CR_ALARM_INTERRUPT;
    outl(cr, ftrtc011->base + FTRTC011_OFFSET_CR);
}

static int ftrtc011_enable(struct ftrtc011 *ftrtc011)
{
    int ret = 0;
    int check_cnt = 5;
    int rtc_busy  = 1;

    /* check restore/refersh status when rtc power-on */
    do {
            if((inl(ftrtc011->base + FTRTC011_OFFSET_RWSTATUS) & 0x100) == 0) {
                rtc_busy = 0;
                break;
            }
            mdelay(100);
    } while(check_cnt--);

    if(!rtc_busy) {
        outl(FTRTC011_CR_ENABLE | FTRTC011_CR_REFRESH | FTRTC011_CR_INTERRUPT_SEC, ftrtc011->base + FTRTC011_OFFSET_CR);
    }
    else {
        dev_err(&ftrtc011->pdev->dev, "rtc enable failed!!(refresh busy)\n");
        ret = -EINVAL;
    }

    return ret;
}

static int ftrtc011_disable(struct ftrtc011 *ftrtc011)
{
    outl(0, ftrtc011->base + FTRTC011_OFFSET_CR);
    return 0;
}

static irqreturn_t ftrtc011_useless_handler(int irq, void *dev_id)
{
    struct ftrtc011 *ftrtc011 = dev_id;

    outl(0x20, ftrtc011->base + FTRTC011_OFFSET_INTR_STATE);

    return IRQ_HANDLED;
}

/******************************************************************************
 * alarm interrupt handler
 *****************************************************************************/
static irqreturn_t ftrtc011_ai_handler(int irq, void *dev_id)
{
    struct ftrtc011 *ftrtc011 = dev_id;
    struct rtc_device *rtc    = ftrtc011->rtc;

    spin_lock(&ftrtc011->lock);

    rtc_update_irq(rtc, 1, RTC_AF|RTC_IRQF);

    spin_unlock(&ftrtc011->lock);

    outl(FTRTC011_INTR_STATE_ALARM, ftrtc011->base + FTRTC011_OFFSET_INTR_STATE);

    return IRQ_HANDLED;
}

/******************************************************************************
 * periodic interrupt handler
 *****************************************************************************/
static irqreturn_t ftrtc011_pi_handler(int irq, void *dev_id)
{
    struct ftrtc011 *ftrtc011 = dev_id;
    struct rtc_device *rtc    = ftrtc011->rtc;

    spin_lock(&ftrtc011->lock);

    rtc_update_irq(rtc, 1, RTC_UF|RTC_IRQF);

    spin_unlock(&ftrtc011->lock);

    outl(FTRTC011_INTR_STATE_SEC | 0x20, ftrtc011->base + FTRTC011_OFFSET_INTR_STATE);

    return IRQ_HANDLED;
}

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
static irqreturn_t ftrtc011_irq_handler(int irq, void *dev_id)
{
    struct ftrtc011 *ftrtc011 = dev_id;
    u32 IntrStatus = 0;

    IntrStatus = inl(ftrtc011->base + FTRTC011_OFFSET_INTR_STATE);

    if (IntrStatus & (FTRTC011_INTR_STATE_SEC | FTRTC011_INTR_STATE_MIN | FTRTC011_INTR_STATE_HOUR | FTRTC011_INTR_STATE_DAY)) {
        ftrtc011_pi_handler(irq, dev_id);
    }
    else if (IntrStatus & FTRTC011_INTR_STATE_ALARM) {
        ftrtc011_ai_handler(irq, dev_id);
    }
    else {
        ftrtc011_useless_handler(irq, dev_id);
    }
    return IRQ_HANDLED;
}

static int ftrtc011_read_time(struct device *dev, struct rtc_time *tm)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct ftrtc011 *ftrtc011    = platform_get_drvdata(pdev);
    unsigned long time;
    unsigned long flags = 0;

    spin_lock_irqsave(&ftrtc011->lock, flags);

    time = ftrtc011_get_uptime(ftrtc011);
    rtc_time_to_tm(time, tm);

    spin_unlock_irqrestore(&ftrtc011->lock, flags);

    return 0;
}

static int ftrtc011_set_time(struct device *dev, struct rtc_time *tm)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct ftrtc011 *ftrtc011    = platform_get_drvdata(pdev);
    unsigned long new_time;
    int ret = 0;
    unsigned long flags = 0;

    spin_lock_irqsave(&ftrtc011->lock, flags);

    ret = rtc_tm_to_time(tm, &new_time);
    if (ret == 0)
        ftrtc011_set_uptime(ftrtc011, new_time);

    spin_unlock_irqrestore(&ftrtc011->lock, flags);

    return ret;
}

static int ftrtc011_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct ftrtc011 *ftrtc011    = platform_get_drvdata(pdev);

    ftrtc011_get_alarm_tm(ftrtc011, &alrm->time);

    return 0;
}

static int ftrtc011_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct ftrtc011 *ftrtc011    = platform_get_drvdata(pdev);
    unsigned long flags = 0;

    spin_lock_irqsave(&ftrtc011->lock, flags);

    ftrtc011_set_alarm_tm(ftrtc011, &alrm->time);

    spin_unlock_irqrestore(&ftrtc011->lock, flags);

    return 0;
}

static int ftrtc011_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ftrtc011 *ftrtc011    = platform_get_drvdata(pdev);
	unsigned long flags = 0;

	spin_lock_irqsave(&ftrtc011->lock, flags);

	if(enabled)
	    ftrtc011_enable_alarm_interrupt(ftrtc011);
	else
	    ftrtc011_disable_alarm_interrupt(ftrtc011);

	spin_unlock_irqrestore(&ftrtc011->lock, flags);

	return 0;
}

static struct rtc_class_ops ftrtc011_ops = {
    .read_time        = ftrtc011_read_time,
    .set_time         = ftrtc011_set_time,
    .read_alarm       = ftrtc011_read_alarm,
    .set_alarm        = ftrtc011_set_alarm,
    .alarm_irq_enable = ftrtc011_alarm_irq_enable,
};

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftrtc011_probe(struct platform_device *pdev)
{
    int    ret, irq;
    struct ftrtc011   *ftrtc011;
    struct resource   *res;
    struct rtc_device *rtc;

    if ((ftrtc011 = kzalloc(sizeof(*ftrtc011), GFP_KERNEL)) == NULL) {
        return -ENOMEM;
    }

    spin_lock_init(&ftrtc011->lock);

    if ((res = platform_get_resource(pdev, IORESOURCE_MEM, 0)) == 0) {
        ret = -EINVAL;
        dev_err(&pdev->dev, "get IORESOURCE_MEM failed\n");
        goto err_dealloc;
    }

    if ((irq = platform_get_irq(pdev, 0)) < 0) {
        ret = -EINVAL;
        dev_err(&pdev->dev, "get IORESOURCE_IRQ failed\n");
        goto err_dealloc;
    }

    if ((ftrtc011->base = ioremap_nocache(res->start, res->end - res->start)) == NULL) {
        ret = -ENOMEM;
        dev_err(&pdev->dev, "ioremap failed\n");
        goto err_dealloc;
    }

    platform_set_drvdata(pdev, ftrtc011);

    ftrtc011->pdev = pdev;

    /* disable RTC */
    ftrtc011_disable(ftrtc011);

    ret = request_irq(irq, ftrtc011_irq_handler, IRQF_SHARED, DRIVER_NAME, ftrtc011);
    if (ret < 0) {
        dev_err(&pdev->dev, "IRQ %d request failed!!\n", irq);
        goto err_unmap;
    }
    ftrtc011->irq = irq;

    rtc = rtc_device_register(pdev->name, &pdev->dev, &ftrtc011_ops, THIS_MODULE);
    if (IS_ERR(rtc)) {
        ret = PTR_ERR(rtc);
        dev_err(&pdev->dev, "register rtc device failed\n");
        goto err_irq;
    }
    ftrtc011->rtc = rtc;

    /* Enable RTC */
    ret = ftrtc011_enable(ftrtc011);
    if(ret < 0)
        goto err_dev;

    return 0;

err_dev:
    rtc_device_unregister(ftrtc011->rtc);

err_irq:
    free_irq(ftrtc011->irq, ftrtc011);

err_unmap:
    iounmap(ftrtc011->base);

err_dealloc:
    kfree(ftrtc011);

    return ret;
}

static int ftrtc011_remove(struct platform_device *pdev)
{
    struct ftrtc011 *ftrtc011 = platform_get_drvdata(pdev);

    /* disable RTC */
    ftrtc011_disable(ftrtc011);

    if(ftrtc011->irq >= 0)
        free_irq(ftrtc011->irq, ftrtc011);

    if(ftrtc011->rtc)
        rtc_device_unregister(ftrtc011->rtc);

    if(ftrtc011->base)
        iounmap((void __iomem *)ftrtc011->base);

    kfree(ftrtc011);

    return 0;
}

static struct platform_driver ftrtc011_driver = {
    .probe  = ftrtc011_probe,
    .remove = ftrtc011_remove,
    .driver = {
                .name = DRIVER_NAME,
              },
};

static struct resource ftrtc011_resource[] = {
    [0] = {
           .start = RTC_FTRTC_PA_BASE,
           .end   = RTC_FTRTC_PA_LIMIT,
           .flags = IORESOURCE_MEM,
          },
    [1] = {
           .start = RTC_FTRTC_IRQ,      ///< RTC_INT
           .end   = RTC_FTRTC_IRQ,
           .flags = IORESOURCE_IRQ,
          },
};

static void ftrtc011_device_release(struct device *dev)
{
    return;
}

static u64 ftrtc011_dmamask = ~(u32) 0;
static struct platform_device ftrtc011_device = {
    .name          = DRIVER_NAME,
    .id            = 0,
    .num_resources = ARRAY_SIZE(ftrtc011_resource),
    .resource      = ftrtc011_resource,
    .dev = {
            .dma_mask          = &ftrtc011_dmamask,
            .coherent_dma_mask = 0xFFFFFFFF,
            .release           = ftrtc011_device_release,
           },
};

static int __init ftrtc011_init(void)
{
    int ret = 0;

    /* register pmu register */
    ret = rtc_plat_pmu_init();
    if (ret < 0)
        return -1;

    /* setup rtc clock source */
    ret = rtc_plat_rtc_clk_set_src(clk_src);
    if(ret < 0)
        goto fail0;

    /* register rtc platfrom driver */
    ret = platform_driver_register(&ftrtc011_driver);
    if (ret < 0) {
        printk("register RTC platform driver failed\n");
        goto fail1;
    }

    /* register rtc platfrom device */
    ret = platform_device_register(&ftrtc011_device);
    if (ret < 0) {
        printk("register RTC platform device failed\n");
        goto fail2;
    }

    return ret;

fail2:
    platform_driver_unregister(&ftrtc011_driver);

fail1:
    /* gating RTC clk for power saving */
    rtc_plat_rtc_clk_gate(1);

fail0:
    /* release pmu register */
    rtc_plat_pmu_exit();

    return ret;
}

static void __exit ftrtc011_exit(void)
{
    platform_device_unregister(&ftrtc011_device);
    platform_driver_unregister(&ftrtc011_driver);

    /* gating RTC clk for power saving, must ensure the rtc is disable */
    rtc_plat_rtc_clk_gate(1);

    rtc_plat_pmu_exit();
}

module_init(ftrtc011_init);
module_exit(ftrtc011_exit);

MODULE_DESCRIPTION("Grain Media FTRTC011 RealTime Clock Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
