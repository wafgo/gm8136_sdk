/**
 * @file plat_a369.c
 * platform releated api for A369
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.4 $
 * $Date: 2014/07/14 02:44:00 $
 *
 * ChangeLog:
 *  $Log: plat_a369.c,v $
 *  Revision 1.4  2014/07/14 02:44:00  jerson_l
 *  1. add ext_clk driving control api
 *
 *  Revision 1.3  2013/10/31 05:18:24  jerson_l
 *  1. add SSCG control api
 *
 *  Revision 1.2  2013/10/11 10:27:12  jerson_l
 *  1. update platform gpio pin register api
 *
 *  Revision 1.1.1.1  2013/07/25 09:29:13  jerson_l
 *  add front_end device driver
 *
 *
 */
#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "platform.h"

int plat_extclk_onoff(int on)
{
    return 0;
}
EXPORT_SYMBOL(plat_extclk_onoff);

int plat_extclk_set_freq(int clk_id, u32 freq)
{
    return 0;
}
EXPORT_SYMBOL(plat_extclk_set_freq);

int plat_extclk_set_sscg(PLAT_EXTCLK_SSCG_T sscg_mr)
{
    return 0;
}
EXPORT_SYMBOL(plat_extclk_set_sscg);

int plat_extclk_set_driving(PLAT_EXTCLK_DRIVING_T driving)
{
    return 0;
}
EXPORT_SYMBOL(plat_extclk_set_driving);

int plat_extclk_set_src(PLAT_EXTCLK_SRC_T src)
{
    return 0;
}
EXPORT_SYMBOL(plat_extclk_set_src);

int plat_register_gpio_pin(PLAT_GPIO_ID_T gpio_id, PLAT_GPIO_DIRECTION_T direction, int out_value)
{
    return 0;
}
EXPORT_SYMBOL(plat_register_gpio_pin);

void plat_deregister_gpio_pin(PLAT_GPIO_ID_T gpio_id)
{
    return;
}
EXPORT_SYMBOL(plat_deregister_gpio_pin);

int plat_gpio_set_value(PLAT_GPIO_ID_T gpio_id, int value)
{
    return 0;
}
EXPORT_SYMBOL(plat_gpio_set_value);

int plat_gpio_get_value(PLAT_GPIO_ID_T gpio_id)
{
    return 0;
}
EXPORT_SYMBOL(plat_gpio_get_value);

void plat_identifier_check(void)
{
    return;
}
EXPORT_SYMBOL(plat_identifier_check);

static int __init plat_init(void)
{
    return 0;
}

static void __exit plat_exit(void)
{
    return;
}

module_init(plat_init);
module_exit(plat_exit);

MODULE_DESCRIPTION("Grain Media A369 Front-End Plaftorm Common Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
