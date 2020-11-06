/**
 * @file plat_8136.c
 * platform releated api for GM8136
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.1 $
 * $Date: 2014/08/05 03:13:07 $
 *
 * ChangeLog:
 *  $Log: plat_8136.c,v $
 *  Revision 1.1  2014/08/05 03:13:07  jerson_l
 *  1. support GM8136 RTC
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <mach/ftpmu010.h>

#include "platform.h"

#define PLAT_RTC_CLK_HZ     32768

static int rtc_pmu_fd = -1;

/*
 * GM8136 PMU registers for RTC
 */
static pmuReg_t rtc_pmu_reg_8136[] = {
    /*
     * IP Main Clock Select Setting Register [offset=0x28]
     * ----------------------------------------------------
     * [0] RTC_CLK ==> 0: External OSC(32.768MHz) 1: ADDA_CLK/2(6MHz)
     */
    {
     .reg_off   = 0x28,
     .bits_mask = 0x00000001,
     .lock_bits = 0x00000001,
     .init_val  = 0,            ///< keep default value
     .init_mask = 0,
    },

    /*
     * SDC/MAC/UART/RTC x bit count [offset=0x70]
     * ----------------------------------------------------
     * [31:24] RTC_CLK_DIV
     */
    {
     .reg_off   = 0x70,
     .bits_mask = (0xff<<24),
     .lock_bits = (0xff<<24),
     .init_val  = 0,            ///< keep default value
     .init_mask = 0,
    },

    /*
     * APBMCLKoff                   [offset=0xB8]
     * ----------------------------------------------------
     * [14] APB_RTC_CLK_GATE ==> 0: disable 1: gating clock
     */
    {
     .reg_off   = 0xb8,
     .bits_mask = (0x1<<14),
     .lock_bits = (0x1<<14),
     .init_val  = 0,            ///< init to disable clcok gate
     .init_mask = (0x1<<14),
    }
};

static pmuRegInfo_t rtc_pmu_reg_info_8136 = {
    "RTC_8136",
    ARRAY_SIZE(rtc_pmu_reg_8136),
    ATTR_TYPE_NONE,
    &rtc_pmu_reg_8136[0]
};

int rtc_plat_pmu_init(void)
{
    int ret = 0;
    int fd;

    /* register pmu register for register access */
    fd = ftpmu010_register_reg(&rtc_pmu_reg_info_8136);
    if(fd < 0) {
        printk("register pmu register failed!!\n");
        ret = -1;
        goto exit;
    }

    rtc_pmu_fd = fd;

exit:
    return ret;
}
EXPORT_SYMBOL(rtc_plat_pmu_init);

void rtc_plat_pmu_exit(void)
{
    /* deregister pmu register */
    if(rtc_pmu_fd >= 0) {
        ftpmu010_deregister_reg(rtc_pmu_fd);
        rtc_pmu_fd = -1;
    }
}
EXPORT_SYMBOL(rtc_plat_pmu_exit);

int rtc_plat_rtc_clk_gate(int onoff)
{
    int ret;

    if(rtc_pmu_fd < 0)
        return -1;

    if(onoff)
        ret = ftpmu010_write_reg(rtc_pmu_fd, 0xb8, (0x1<<14), (0x1<<14));
    else
        ret = ftpmu010_write_reg(rtc_pmu_fd, 0xb8, 0, (0x1<<14));

    return ret;
}
EXPORT_SYMBOL(rtc_plat_rtc_clk_gate);

int rtc_plat_rtc_clk_set_src(PLAT_RTC_CLK_SRC_T clk_src)
{
    int ret = 0;
    u32 adda_clk;
    u32 div;

    if(rtc_pmu_fd < 0)
        return -1;

    switch(clk_src) {
        case PLAT_RTC_CLK_SRC_PLL3:
            /* get ADDA clock out frequency */
            adda_clk = PLL3_CLK_IN/(((ftpmu010_read_reg(0x74)>>24)&0x3f)+1);    ///< should be 12MHz
            div      = (adda_clk/2)/PLAT_RTC_CLK_HZ;
            if(div)
                div--;

            /* set RTC clock divide value */
            ret = ftpmu010_write_reg(rtc_pmu_fd, 0x70, (div<<24), (0xff<<24));
            if(ret < 0)
                goto exit;

            /* switch clock source */
            ret = ftpmu010_write_reg(rtc_pmu_fd, 0x28, 0x1, 0x1);
            if(ret < 0)
                goto exit;

            printk("RTC_CLK output ==> %uHz\n", (adda_clk/2)/(div+1));
            break;
        case PLAT_RTC_CLK_SRC_OSC:
        default:
            ret = ftpmu010_write_reg(rtc_pmu_fd, 0x28, 0, 0x1);
            if(ret < 0)
                goto exit;
            break;
    }

exit:
    return ret;
}
EXPORT_SYMBOL(rtc_plat_rtc_clk_set_src);
