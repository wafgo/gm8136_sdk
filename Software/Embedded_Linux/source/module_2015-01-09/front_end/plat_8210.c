/**
 * @file plat_8210.c
 * platform releated api for GM8210
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.20 $
 * $Date: 2014/08/05 10:47:13 $
 *
 * ChangeLog:
 *  $Log: plat_8210.c,v $
 *  Revision 1.20  2014/08/05 10:47:13  shawn_hu
 *  1. Update the sources for HD/SD hybrid frontend driver design (only for 8210).
 *  2. Please attach the DH9901/TW2964 daughterboard to GM8210 socket J77.
 *
 *  Revision 1.19  2014/07/14 09:52:30  jerson_l
 *  1. change VERSION definition to PLAT_COMMON_VERSION
 *
 *  Revision 1.18  2014/07/14 07:23:15  ccsu
 *  move version control define to platform.h
 *
 *  Revision 1.17  2014/07/14 06:45:26  ccsu
 *  add version control
 *
 *  Revision 1.16  2014/07/14 02:56:31  jerson_l
 *  1. fix ext_clk driving wrong bit mask problem
 *
 *  Revision 1.15  2014/07/14 02:44:00  jerson_l
 *  1. add ext_clk driving control api
 *
 *  Revision 1.14  2014/07/10 09:23:58  easonli
 *  *:[frontend] modify ssp3 mclk to 24MHz bcuz 7500 use 24MHz to count divisor of SSP3
 *
 *  Revision 1.13  2014/06/20 05:56:56  ccsu
 *  *:[front end] add 48.03k sample rate support for SSP3
 *
 *  Revision 1.12  2014/02/10 02:10:14  ccsu
 *  remove i2s 4 & 5 setting
 *
 *  Revision 1.11  2014/01/27 06:49:12  ccsu
 *  enable i2s_4 & i2s_5 pinmux
 *
 *  Revision 1.10  2014/01/27 05:26:09  ccsu
 *  modify ssp0~5 init_mask at 0xb8 & 0xbc
 *
 *  Revision 1.9  2014/01/16 09:59:59  easonli
 *  *:[platform] add get_digital_codec_name()
 *
 *  Revision 1.8  2014/01/16 09:44:19  easonli
 *  *:[platform] add digital_codec_name check
 *
 *  Revision 1.7  2013/12/16 03:04:16  ccsu
 *  add i2s driving capacity to 8mA cause when i2s is master/cx20811 is slave, i2s driving not enough that cx20811 can't receive data
 *
 *  Revision 1.6  2013/11/13 09:19:34  easonli
 *  *:[audio] add get codec name function and modified audio job member for av sync
 *
 *  Revision 1.5  2013/10/31 05:18:24  jerson_l
 *  1. add SSCG control api
 *
 *  Revision 1.4  2013/10/11 10:27:12  jerson_l
 *  1. update platform gpio pin register api
 *
 *  Revision 1.3  2013/09/16 08:52:54  ccsu
 *  add ssp0~5 pmu setting for audio
 *
 *  Revision 1.2  2013/08/23 10:04:03  easonli
 *  *:[front_end] add audio hook function for nvp1918
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
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <asm/gpio.h>
#include <mach/ftpmu010.h>
#include <mach/fmem.h>

#include "platform.h"

#define PLAT_EXTCLK_MAX             2       ///< EXT0_CLK, EXT1_CLK
#define PLAT_X_CAP_RST_GPIO_PIN     33      ///< GPIO#33 as X_CAP_RST pin

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static DEFINE_SEMAPHORE(plat_lock);
#else
static DECLARE_MUTEX(plat_lock);
#endif

static audio_funs_t plat_audio_funs;

static int plat_pmu_fd = -1;
static int plat_gpio_init[PLAT_GPIO_ID_MAX] = {[0 ... (PLAT_GPIO_ID_MAX - 1)] = 0};

/*
 * GM8210 PMU registers for video decoder
 */
static pmuReg_t plat_pmu_reg_8210[] = {
    /*
     * Module Clock Selecting Register [offset=0x28]
     * -------------------------------------------------
     * [3]    extclkfpll4_sel ==> 0:pll4out1_div2(default) 1:pll4out2_div1
     * [13:12]extclk_sel      ==> 0:pll1out1 1:pll1out1_div2(default) 2:pll4out2/pll4out1_div2(reg28[3]) 3:pll3
     * [14]   extclk_en       ==> 0:disable 1:enable(default)
     * [25:24] ssp0_clk : 0: clk_12288 1:12MHz 2/3: pcie divided by cntp
     * [23:22] ssp1_clk : 0: clk_12288 1:12MHz 2/3: pcie divided by cntp
     * [21:20] ssp2_clk : 0: clk_12288 1:12MHz 2/3: pcie divided by cntp
     */
    {
     .reg_off   = 0x28,
     .bits_mask = (0x1<<14) | (0x3<<12) | (0x1<<3) | (0x3f<<20),
     .lock_bits = 0x0,
     .init_val  = 0x15<<20,
     .init_mask = 0x3f<<20,
    },

    /*
     * Clock In Polarity and Multi-Function Port Setting Register 6 [offset=0x4c]
     * -------------------------------------------------
     * [4]ext1_lcd1_mux ==> 0:ext_clk_1(default) 1:LCDC pixel clock
     */
    {
     .reg_off   = 0x4c,
     .bits_mask = 0x1<<4,
     .lock_bits = 0x0,
     .init_val  = 0x0,
     .init_mask = 0x0,
    },

    /*
     * Divider Setting Register 3 [offset=0x78]
     * -------------------------------------------------
     * [11:6]extclk_pvalue ==> reg28[13:12]/(extclk_pvalue + 1), default: 0
     * [31:30] ssp5_clk : 0: clk_12288 1:12MHz 2/3: pcie divided by cntp
     */
    {
     .reg_off   = 0x78,
     .bits_mask = (0x1f<<6) | (0x3<<30),
     .lock_bits = 0x0,
     .init_val  = 0x1<<30,
     .init_mask = 0x3<<30,
    },

    /*
     * PCIe PHY Control Register 0 [offset=0x84]
     * -------------------------------------------------
     * [21:16]ext1clk_pvalue ==> reg28[13:12]/(ext1clk_pvalue + 1), default: 0
     */
    {
     .reg_off   = 0x84,
     .bits_mask = 0x1f<<16,
     .lock_bits = 0x0,
     .init_val  = 0x0,
     .init_mask = 0x0,
    },

    /*
     * AXI Module Clock Off Control Register [offset=0xb0]
     * -------------------------------------------------
     * [6]extclkaoff ==> 0:EXT_CLK clock on 1:EXT_CLK clock off(default)
     */
    {
     .reg_off   = 0xb0,
     .bits_mask = 0x1<<6,
     .lock_bits = 0x0,
     .init_val  = 0x0,
     .init_mask = 0x0,
    },

    /*
     * Multi-function Port Setting Register 3 [offset=0x5c]
     * ---------------------------------------------------------
     * [26]smc_gpio_mux ==> 0:smc_csn[2] 1:gpio[33] as X_CAP_RST
     */
    {
     .reg_off   = 0x5c,
     .bits_mask = 0x1<<26,
     .lock_bits = 0x0,
     .init_val  = 0x0,
     .init_mask = 0x0,
    },

    /*
     * SSP clock value [offset=0x60]
     * ---------------------------------------------------------
     *  [12] : 1 - clk_12288 = 12.288MHz, 0 - clk_12288 = ssp6144_clk
     *  [13] : 1 - analog die clock = 12MHz, 0 - analog die clock = clk_12288
     *   Note : ssp6144_clk = ((pll4out1, pll4out2 or pll4out3) / ([24:22]+1)/([21:17]+1))
     *             clk_12288 is SSP and analog die clock's candidate
     *  [16] : gating clock control, 1-gating, 0-un-gating
     *  [21:17] : ssp6144_clk_cnt5_pvalue
     *  [24:22] : ssp6144_clk_cnt3_pvalue
     *  [26:25] : ssp6144_clk_sel, 00-PLL4 CKOUT1, 01-PLL4 CKOUT2, 1x-PLL4 CKOUT3
     */
     {
     .reg_off   = 0x60,
     .bits_mask = (0x3 << 12) | (0x3FF << 17) | (0x1 << 16),
     .lock_bits = 0,
     .init_val  = 0x0,
     .init_mask = (0x3 << 12) | (0x3FF << 17) | (0x1 << 16),
    },

    /*
     * SSP3 select 12M as the source [offset=0x70]
     * ---------------------------------------------------------
     * [31:30] ssp3_clk : 0: clk_12288 1:12MHz 2/3: pcie divided by cntp
     */
     {
     .reg_off   = 0x70,
     .bits_mask = 0x3<<30,
     .lock_bits = 0x0,
     .init_val  = 0x1<<30,
     .init_mask = 0x3<<30,
    },

    /*
     * SSP4 select 12M as the source [offset=0x74]
     * ---------------------------------------------------------
     * [31:30] ssp4_clk : 0: clk_12288 1:12MHz 2/3: pcie divided by cntp
     */
     {
     .reg_off   = 0x74,
     .bits_mask = 0x3<<30,
     .lock_bits = 0x0,
     .init_val  = 0x1<<30,
     .init_mask = 0x3<<30,
    },

    /*
     * SSP0~3 pclk & sclk [offset=0xb8]
     * ---------------------------------------------------------
     * [6]  ssp0 pclk & sclk
     * [7]  ssp1 pclk & sclk
     * [8]  ssp2 pclk & sclk
     * [22] ssp3 pclk & sclk
     */
     {
     .reg_off   = 0xb8,
     .bits_mask = (0x1<<22) | (0x7<<6),
     .lock_bits = (0x0),
     .init_val  = 0x0,
     .init_mask = (0x1<<22) | (0x7<<6),
    },

    /*
     * SSP4~5 pclk & sclk [offset=0xbc]
     * ---------------------------------------------------------
     * [18]  ssp4 pclk & sclk
     * [19]  ssp5 pclk & sclk
     */
     {
     .reg_off   = 0xbc,
     .bits_mask = 0x3 << 18,
     .lock_bits = 0x0,
     .init_val  = 0x0,
     .init_mask = 0x3 << 18,
    },

    /*
     * Multi-function Port Setting Register 0 [offset=0x50]
     * -------------------------------------------------
     * [23:22]i2c_gpio_i2s_mux ==> 00: {i2c1_sda, i2c1_scl}, 01: gpio[48:47], 10: {i2s5_fs,  i2s5_sclk}
     * [25:24]i2c_gpio_i2s_mux ==> 00: {i2c2_sda, i2c2_scl}, 01: gpio[50:49], 10: {i2s4_rxd, i2s4_txd}
     * [27:26]i2c_gpio_i2s_mux ==> 00: {i2c3_sda, i2c3_scl}, 01: gpio[28:27], 10: {i2s4_fs,  i2s4_sclk}
     * [29:28]i2c_gpio_i2s_mux ==> 00: {i2c4_sda, i2c4_scl}, 01: gpio[52:51], 10: {i2s5_rxd, i2s5_txd}
     * [31] gpio_gspi_mux ==> 0: gpio[7:4], 1: gspi_cen[7:4]
     */
    {
     .reg_off   = 0x50,
     .bits_mask = (0x3<<22) | (0x3<<24) | (0x3<<26) | (0x3<<28) | (0x1<<31),
     .lock_bits = 0x0,
     .init_val  = 0x0,
     .init_mask = 0x0,
    },

    /*
     * Driving Capacity [offset=0x44]
     * modify i2s driving capacity to 8mA cause when i2s is master/cx20811 is slave, i2s driving is not enough that cx20811 can't receive data
     */
    {
     .reg_off   = 0x44,
     .bits_mask = 0x3<<9,
     .lock_bits = 0x0,
     .init_val  = 0x1<<9,
     .init_mask = 0x3<<9,
    }
};

static pmuRegInfo_t plat_pmu_reg_info_8210 = {
    "FE_8210",
    ARRAY_SIZE(plat_pmu_reg_8210),
    ATTR_TYPE_NONE,
    &plat_pmu_reg_8210[0]
};

typedef struct pvalue {
    int bit17_21;
    int bit22_24;
} pvalue_t;

/*
 * find out pvalue->bit17_21 & pvalue->bit22_24 to get ssp3 clock
 * ssp3 clock = ((pll4out1, pll4out2 or pll4out3) / ([24:22]+1)/([21:17]+1))
 * div = (pll4out1, pll4out2 or pll4out3) / ssp3 clock
 * rule : div = pvalue->bit17_21 * pvalue->bit22_24
 * pvalue->bit17_21 : 1~31, pvalue->bit22_24 : 1~7
 * pvalue->bit17_21 & pvalue->bit22_24 must at least = 1
 * return flag : 0 --> can't not get pvalue, 1 --> got pvalue
 */
int ssp_get_pvalue(pvalue_t *pvalue, int div)
{
    int i = 0;
    int flag = 0;
    /* pvalue->bit22_24 : 1~7 */
    for (i = 1; i <= 7; i++) {
        if (div % i == 0) {
            pvalue->bit17_21 = div / i;
            if (pvalue->bit17_21 >= 32) // pvalue->bit17_21 : 1~31
                continue;

            pvalue->bit22_24 = i;
            flag = 1;
            break;
        }
    }
    /* flag : 0 --> can't not get pvalue, 1 --> got pvalue */
    return flag;
}

/*  SSP3 playback to HDMI, sample rate = 48k, sample size = 16 bits, bit clock = 3072000, SSP3 MAIN CLOCK must = 24MHz
 *  pmu 0x60 [13:12] : select 12.288MHz source for analog die and clk_12288
 *  [12] : 1 - clk_12288 = 12.288MHz, 0 - clk_12288 = ssp6144_clk
 *  [13] : 1 - analog die clock = 12MHz, 0 - analog die clock = clk_12288
 *   Note : ssp6144_clk = ((pll4out1, pll4out2 or pll4out3) / ([24:22]+1)/([21:17]+1))
 *             clk_12288 is SSP and analog die clock's candidate
 *  [16] : gating clock control, 1-gating, 0-un-gating
 *  [21:17] : ssp6144_clk_cnt5_pvalue
 *  [24:22] : ssp6144_clk_cnt3_pvalue
 *  [26:25] : ssp6144_clk_sel, 00-PLL4 CKOUT1, 01-PLL4 CKOUT2, 1x-PLL4 CKOUT3
 *
 *  SSP3 choose clk_12288 as main clock, clk_12288 form ssp6144_clk,
 */
void pmu_cal_clk(void)
{
    unsigned int pll4_out1, div;
    unsigned int pll4_out2, pll4_out3;
    unsigned int ssp3_clk = 12000000;//24000000;
    pvalue_t pvalue;
    int pll4_clkout1 = 0, pll4_clkout2 = 0, pll4_clkout3 = 0;
    int bit25_26 = 0, flag = 0;
    int ret = 0;

    memset(&pvalue, 0x0, sizeof(pvalue_t));

    pll4_out1 = ftpmu010_get_attr(ATTR_TYPE_PLL4);   //hz
    pll4_out2 = pll4_out1 * 2 / 3;   // pll4_out1 / 1.5
    pll4_out3 = pll4_out1 * 2 / 5;   // pll4_out1 / 2.5

    if (pll4_out1 == 996000000) {   // ssp main clock from 996mHZ, 996/(3*27) = 12.296 mHZ
        bit25_26 = 0;               // ssp3 as master , bclk = 12.296 mHZ/4 = 3.074 mHZ
        pvalue.bit17_21 = 27;       // 3.074 mHZ/64 = 48.03 kHZ
        pvalue.bit22_24 = 3;
    }
    else {
        ssp3_clk = 24000000; /* this setting is matched for 7500 */
        div = ftpmu010_clock_divisor2(ATTR_TYPE_PLL4, ssp3_clk, 1);
        /* get pvalue from pll4_out1 */
        flag = ssp_get_pvalue(&pvalue, div);
        if (flag == 1)
            pll4_clkout1 = 1;
        /* if flag == 0, get pvalue from pll4_out2 */
        if (flag == 0) {
            div = pll4_out2 / ssp3_clk;
            flag = ssp_get_pvalue(&pvalue, div);
            if (flag == 1)
                pll4_clkout2 = 1;
        }
        /* if flag == 0, get pvalue from pll4_out3 */
        if (flag == 0) {
            div = pll4_out3 / ssp3_clk;
            flag = ssp_get_pvalue(&pvalue, div);
            if (flag == 1)
                pll4_clkout3 = 1;
        }
        /* get ssp6144_clk_sel from pll4_clkout1/pll4_out2/pll4_out3 */
        if (pll4_clkout1)
            bit25_26 = 0;
        if (pll4_clkout2)
            bit25_26 = 1;
        if (pll4_clkout3)
            bit25_26 = 2;
    }

    ret = ftpmu010_write_reg(plat_pmu_fd, 0x60, ((0x2 << 12) | (0x0 << 16) | ((pvalue.bit17_21 - 1) << 17) | ((pvalue.bit22_24 - 1) << 22) | (bit25_26 << 25)), ((0x3 << 12) | (0x3FF << 17) | (0x1 << 16)));
    if (ret < 0)
        panic("%s, register ssp3 pvalue fail\n", __func__);

    // SSP3 clk_sel = clk_12288
    ret = ftpmu010_write_reg(plat_pmu_fd, 0x70, 0x0 << 30, 0x3 << 30);
    if (ret < 0)
        panic("%s, register ssp3 clock on fail\n", __func__);

}

int plat_extclk_onoff(int on)
{
    int ret = 0;

    if(plat_pmu_fd < 0)
        return -1;

    down(&plat_lock);

    if(on) {
        /* enable AXI external clock gate */
        ret = ftpmu010_write_reg(plat_pmu_fd, 0xb0, 0, 0x1<<6);
        if(ret < 0)
            goto exit;

        /* enable external clock output */
        ret = ftpmu010_write_reg(plat_pmu_fd, 0x28, 0x1<<14, 0x1<<14);
        if(ret < 0)
            goto exit;
    }
    else {
        /* keep AXI exteranl clock gate config */

        /* disable external clock output */
        ret = ftpmu010_write_reg(plat_pmu_fd, 0x28, 0, 0x1<<14);
        if(ret < 0)
            goto exit;
    }

exit:
    if(ret < 0)
        printk("%s EXT_CLK output failed!!\n", ((on == 0) ? "disable" : "enable"));

    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_extclk_onoff);

int plat_extclk_set_freq(int clk_id, u32 freq)
{
    int ret = 0;
    u32 tmp;
    u32 src_clk;
    u32 div;

    if(plat_pmu_fd < 0)
        return -1;

    if(clk_id >= PLAT_EXTCLK_MAX || freq == 0)
        return -1;

    down(&plat_lock);

    /* get external clock source */
    tmp = ftpmu010_read_reg(0x28);
    switch((tmp>>12)&0x3) {
        case 1: /* PLL1OUT1_DIV2 */
            div = ftpmu010_clock_divisor2(ATTR_TYPE_PLL1, freq, 1)/2;
            src_clk = PLL1_CLK_IN/2;
            break;
        case 2: /* PLL4OUT2 or PLL4OUT1_DIV2 */
            div = ftpmu010_clock_divisor2(ATTR_TYPE_PLL4, freq, 1);
            if(((tmp>>3)&0x1) == 0) {
                div /= 2;
                src_clk = PLL4_CLK_IN/2;
            }
            else
                src_clk = PLL4_CLK_IN;
            break;
        case 3: /* PLL3 */
            div = ftpmu010_clock_divisor2(ATTR_TYPE_PLL3, freq, 1);
            src_clk = PLL3_CLK_IN;
            break;
        default: /* PLL1OUT1 */
            div = ftpmu010_clock_divisor2(ATTR_TYPE_PLL1, freq, 1);
            src_clk = PLL1_CLK_IN;
            break;
    }

    if(div)
        div--;

    /* set external clock pvalue */
    if(clk_id == 1) {
        /* swicth ext1_lcd1_mux to ext_clk_1 output */
        ret = ftpmu010_write_reg(plat_pmu_fd, 0x4c, 0, 0x1<<4);
        if(ret < 0)
            goto exit;

        ret = ftpmu010_write_reg(plat_pmu_fd, 0x84, (div&0x1f)<<16, 0x1f<<16);
    }
    else
        ret = ftpmu010_write_reg(plat_pmu_fd, 0x78, (div&0x1f)<<6, 0x1f<<6);

exit:
    if(ret < 0)
        printk("set platform EXT_CLK#%d frequency(%uHz) failed!!\n", clk_id, freq);
    else
        printk("EXT_CLK#%d output ==> %uHz\n", clk_id, src_clk/(div+1));

    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_extclk_set_freq);

int plat_extclk_set_sscg(PLAT_EXTCLK_SSCG_T sscg_mr)
{
    return 0;
}
EXPORT_SYMBOL(plat_extclk_set_sscg);

int plat_extclk_set_driving(PLAT_EXTCLK_DRIVING_T driving)
{
    int ret = 0;

    if(plat_pmu_fd < 0 || driving >= PLAT_EXTCLK_DRIVING_MAX)
        return -1;

    down(&plat_lock);

    /* external clock driving setting */
    ret = ftpmu010_write_reg(plat_pmu_fd, 0x40, (driving&0x3)<<21, 0x3<<21);
    if(ret < 0)
        goto exit;

exit:
    if(ret < 0)
        printk("EXT_CLK driving setup failed!!\n");

    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_extclk_set_driving);

int plat_audio_register_function(audio_funs_t *func)
{
    if (func->codec_name == NULL && func->digital_codec_name == NULL) {
        printk(KERN_ERR "%s: codec_name is NULL\n", __func__);
        return -EFAULT;
    }
    if (func->codec_name)
        plat_audio_funs.codec_name = func->codec_name;
    if (func->digital_codec_name)
        plat_audio_funs.digital_codec_name = func->digital_codec_name;
    if (func->sound_switch == NULL) {
        printk(KERN_ERR "%s: sound_switch is NULL\n", __func__);
        return -EFAULT;
    }
    plat_audio_funs.sound_switch = func->sound_switch;
    return 0;
}
EXPORT_SYMBOL(plat_audio_register_function);

void plat_audio_deregister_function(void)
{
    plat_audio_funs.codec_name = NULL;
    plat_audio_funs.digital_codec_name = NULL;
    plat_audio_funs.sound_switch = NULL;
}
EXPORT_SYMBOL(plat_audio_deregister_function);

int plat_audio_sound_switch(int ssp_idx, int chan_num, bool is_on)
{
    if (plat_audio_funs.sound_switch) {
        return plat_audio_funs.sound_switch(ssp_idx, chan_num, is_on);
    } else {
        printk(KERN_WARNING"%s: No audio codec is registered\n", __func__);
        return 0;
    }
}
EXPORT_SYMBOL(plat_audio_sound_switch);

const char *plat_audio_get_codec_name(void)
{
    return plat_audio_funs.codec_name;
}
EXPORT_SYMBOL(plat_audio_get_codec_name);

const char *plat_audio_get_digital_codec_name(void)
{
    return plat_audio_funs.digital_codec_name;
}
EXPORT_SYMBOL(plat_audio_get_digital_codec_name);

int plat_extclk_set_src(PLAT_EXTCLK_SRC_T src)
{
    int ret = 0;

    if(plat_pmu_fd < 0)
        return -1;

    down(&plat_lock);

    switch(src) {
        case PLAT_EXTCLK_SRC_PLL1OUT1_DIV2:
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x28, (0x1<<12), (0x3<<12));
            break;
        case PLAT_EXTCLK_SRC_PLL4OUT2:
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x28, (0x2<<12)|(0x1<<3), (0x3<<12)|(0x1<<3));
            break;
        case PLAT_EXTCLK_SRC_PLL4OUT1_DIV2:
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x28, (0x2<<12), (0x3<<12)|(0x1<<3));
            break;
        case PLAT_EXTCLK_SRC_PLL3:
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x28, (0x3<<12), (0x3<<12));
            break;
        case PLAT_EXTCLK_SRC_PLL1OUT1:
        default:
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x28, 0, (0x3<<12));
            break;
    }

    if(ret < 0)
        printk("set platform external clock source failed!!\n");

    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_extclk_set_src);

int plat_register_gpio_pin(PLAT_GPIO_ID_T gpio_id, PLAT_GPIO_DIRECTION_T direction, int out_value)
{
    int ret = 0;
    u32 val, bit_mask;

    if(plat_pmu_fd < 0)
        return -1;

    down(&plat_lock);

    switch(gpio_id) {
        case PLAT_GPIO_ID_NONE:
            break;
        case PLAT_GPIO_ID_X_CAP_RST:
            /* check pmu bit lock or not? */
            bit_mask = 0x1<<26;
            if(ftpmu010_bits_is_locked(plat_pmu_fd, 0x5c, bit_mask) < 0) {
                ret = ftpmu010_add_lockbits(plat_pmu_fd, 0x5c, bit_mask);
                if(ret < 0) {
                    printk("pmu reg#0x5c bit lock failed!!\n");
                    goto err;
                }
            }

            /* set pin as GPIO */
            val = 0x1<<26;
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x5c, val, bit_mask);
            if(ret < 0)
                goto err;

            /* GPIO pin request */
            if(gpio_request(PLAT_X_CAP_RST_GPIO_PIN, "x_cap_rst") < 0) {
                printk("X_CAP_RST gpio request failed!\n");
                ret = -1;
                goto err;
            }

            /* Config direction */
            if(direction == PLAT_GPIO_DIRECTION_IN)
                gpio_direction_input(PLAT_X_CAP_RST_GPIO_PIN);
            else
                gpio_direction_output(PLAT_X_CAP_RST_GPIO_PIN, out_value);

            plat_gpio_init[gpio_id] = 1;
            break;
        case PLAT_GPIO_ID_4:
        case PLAT_GPIO_ID_5:
        case PLAT_GPIO_ID_6:
        case PLAT_GPIO_ID_7:
            /* check pmu bit lock or not? */
            bit_mask = 0x1<<31;
            if(ftpmu010_bits_is_locked(plat_pmu_fd, 0x50, bit_mask) < 0) {
                ret = ftpmu010_add_lockbits(plat_pmu_fd, 0x50, bit_mask);
                if(ret < 0) {
                    printk("pmu reg#0x50 bit lock failed!!\n");
                    goto err;
                }
            }

            /* set pin as GPIO */
            val = 0x0; // [31]: 0 as GPIO[7:4]
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x50, val, bit_mask);
            if(ret < 0)
                goto err;

            /* GPIO pin request */
            if(gpio_id == PLAT_GPIO_ID_4) {
                if(gpio_request(4, "gpio_4") < 0) {
                    printk("gpio#4 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(4);
                else
                    gpio_direction_output(4, out_value);
            }
            else if(gpio_id == PLAT_GPIO_ID_5) {
                if(gpio_request(5, "gpio_5") < 0) {
                    printk("gpio#5 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(5);
                else
                    gpio_direction_output(5, out_value);
            }
            else if(gpio_id == PLAT_GPIO_ID_6) {
                if(gpio_request(6, "gpio_6") < 0) {
                    printk("gpio#6 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(6);
                else
                    gpio_direction_output(6, out_value);
            }
            else { // gpio_id == PLAT_GPIO_ID_7
                if(gpio_request(7, "gpio_7") < 0) {
                    printk("gpio#7 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(7);
                else
                    gpio_direction_output(7, out_value);
            }

            plat_gpio_init[gpio_id] = 1;
            break;
        case PLAT_GPIO_ID_27:
        case PLAT_GPIO_ID_28:
            /* check pmu bit lock or not? */
            bit_mask = 0x3<<26;
            if(ftpmu010_bits_is_locked(plat_pmu_fd, 0x50, bit_mask) < 0) {
                ret = ftpmu010_add_lockbits(plat_pmu_fd, 0x50, bit_mask);
                if(ret < 0) {
                    printk("pmu reg#0x50 bit lock failed!!\n");
                    goto err;
                }
            }

            /* set pin as GPIO */
            val = 0x1<<26;
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x50, val, bit_mask);
            if(ret < 0)
                goto err;

            /* GPIO pin request */
            if(gpio_id == PLAT_GPIO_ID_27) {
                if(gpio_request(27, "gpio_27") < 0) {
                    printk("gpio#27 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(27);
                else
                    gpio_direction_output(27, out_value);
            }
            else {
                if(gpio_request(28, "gpio_28") < 0) {
                    printk("gpio#28 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(28);
                else
                    gpio_direction_output(28, out_value);
            }

            plat_gpio_init[gpio_id] = 1;
            break;
        case PLAT_GPIO_ID_47:
        case PLAT_GPIO_ID_48:
            /* check pmu bit lock or not? */
            bit_mask = 0x3<<22;
            if(ftpmu010_bits_is_locked(plat_pmu_fd, 0x50, bit_mask) < 0) {
                ret = ftpmu010_add_lockbits(plat_pmu_fd, 0x50, bit_mask);
                if(ret < 0) {
                    printk("pmu reg#0x50 bit lock failed!!\n");
                    goto err;
                }
            }

            /* set pin as GPIO */
            val = 0x1<<22;
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x50, val, bit_mask);
            if(ret < 0)
                goto err;

            /* GPIO pin request */
            if(gpio_id == PLAT_GPIO_ID_47) {
                if(gpio_request(47, "gpio_47") < 0) {
                    printk("gpio#47 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(47);
                else
                    gpio_direction_output(47, out_value);
            }
            else {
                if(gpio_request(48, "gpio_48") < 0) {
                    printk("gpio#48 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(48);
                else
                    gpio_direction_output(48, out_value);
            }

            plat_gpio_init[gpio_id] = 1;
            break;
        case PLAT_GPIO_ID_49:
        case PLAT_GPIO_ID_50:
            /* check pmu bit lock or not? */
            bit_mask = 0x3<<24;
            if(ftpmu010_bits_is_locked(plat_pmu_fd, 0x50, bit_mask) < 0) {
                ret = ftpmu010_add_lockbits(plat_pmu_fd, 0x50, bit_mask);
                if(ret < 0) {
                    printk("pmu reg#0x50 bit lock failed!!\n");
                    goto err;
                }
            }

            /* set pin as GPIO */
            val = 0x1<<24;
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x50, val, bit_mask);
            if(ret < 0)
                goto err;

            /* GPIO pin request */
            if(gpio_id == PLAT_GPIO_ID_49) {
                if(gpio_request(49, "gpio_49") < 0) {
                    printk("gpio#49 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(49);
                else
                    gpio_direction_output(49, out_value);
            }
            else {
                if(gpio_request(50, "gpio_50") < 0) {
                    printk("gpio#50 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(50);
                else
                    gpio_direction_output(50, out_value);
            }

            plat_gpio_init[gpio_id] = 1;
            break;
        case PLAT_GPIO_ID_51:
        case PLAT_GPIO_ID_52:
            /* check pmu bit lock or not? */
            bit_mask = 0x3<<28;
            if(ftpmu010_bits_is_locked(plat_pmu_fd, 0x50, bit_mask) < 0) {
                ret = ftpmu010_add_lockbits(plat_pmu_fd, 0x50, bit_mask);
                if(ret < 0) {
                    printk("pmu reg#0x50 bit lock failed!!\n");
                    goto err;
                }
            }

            /* set pin as GPIO */
            val = 0x1<<28;
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x50, val, bit_mask);
            if(ret < 0)
                goto err;

            /* GPIO pin request */
            if(gpio_id == PLAT_GPIO_ID_51) {
                if(gpio_request(51, "gpio_51") < 0) {
                    printk("gpio#51 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(51);
                else
                    gpio_direction_output(51, out_value);
            }
            else {
                if(gpio_request(52, "gpio_52") < 0) {
                    printk("gpio#52 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(52);
                else
                    gpio_direction_output(52, out_value);
            }

            plat_gpio_init[gpio_id] = 1;
            break;
        default:
            printk("register gpio pin failed!(unknown gpio_id:%d)\n", gpio_id);
            ret = -1;
            goto err;
    }

err:
    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_register_gpio_pin);

void plat_deregister_gpio_pin(PLAT_GPIO_ID_T gpio_id)
{
    down(&plat_lock);

    switch(gpio_id) {
        case PLAT_GPIO_ID_X_CAP_RST:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(PLAT_X_CAP_RST_GPIO_PIN);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_4:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(4);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_5:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(5);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_6:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(6);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_7:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(7);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_27:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(27);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_28:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(28);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_47:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(47);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_48:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(48);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_49:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(49);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_50:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(50);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_51:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(51);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_52:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(52);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        default:
            break;
    }

    up(&plat_lock);
}
EXPORT_SYMBOL(plat_deregister_gpio_pin);

int plat_gpio_set_value(PLAT_GPIO_ID_T gpio_id, int value)
{
    int ret = 0;

    down(&plat_lock);

    switch(gpio_id) {
        case PLAT_GPIO_ID_NONE:
            break;
        case PLAT_GPIO_ID_X_CAP_RST:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(PLAT_X_CAP_RST_GPIO_PIN, value);
            else {
                printk("gpio x_cap_rst set value failed!(un-register gpio_%d)\n", PLAT_X_CAP_RST_GPIO_PIN);
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_4:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(4, value);
            else {
                printk("gpio#4 set value failed!(un-register gpio_4)\n");
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_5:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(5, value);
            else {
                printk("gpio#5 set value failed!(un-register gpio_5)\n");
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_6:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(6, value);
            else {
                printk("gpio#6 set value failed!(un-register gpio_6)\n");
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_7:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(7, value);
            else {
                printk("gpio#7 set value failed!(un-register gpio_7)\n");
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_27:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(27, value);
            else {
                printk("gpio#27 set value failed!(un-register gpio_27)\n");
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_28:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(28, value);
            else {
                printk("gpio#28 set value failed!(un-register gpio_28)\n");
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_47:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(47, value);
            else {
                printk("gpio#47 set value failed!(un-register gpio_47)\n");
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_48:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(48, value);
            else {
                printk("gpio#48 set value failed!(un-register gpio_48)\n");
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_49:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(49, value);
            else {
                printk("gpio#49 set value failed!(un-register gpio_49)\n");
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_50:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(50, value);
            else {
                printk("gpio#50 set value failed!(un-register gpio_50)\n");
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_51:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(51, value);
            else {
                printk("gpio#51 set value failed!(un-register gpio_51)\n");
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_52:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(52, value);
            else {
                printk("gpio#52 set value failed!(un-register gpio_52)\n");
                ret = -1;
            }
            break;
        default:
            printk("gpio set value failed!(unknown gpio_id:%d)\n", gpio_id);
            ret = -1;
            break;
    }

    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_gpio_set_value);

int plat_gpio_get_value(PLAT_GPIO_ID_T gpio_id)
{
    int ret = 0;

    down(&plat_lock);

    switch(gpio_id) {
        case PLAT_GPIO_ID_X_CAP_RST:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(PLAT_X_CAP_RST_GPIO_PIN);
            break;
        case PLAT_GPIO_ID_4:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(4);
            break;
        case PLAT_GPIO_ID_5:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(5);
            break;
        case PLAT_GPIO_ID_6:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(6);
            break;
        case PLAT_GPIO_ID_7:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(7);
            break;
        case PLAT_GPIO_ID_27:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(27);
            break;
        case PLAT_GPIO_ID_28:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(28);
            break;
        case PLAT_GPIO_ID_47:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(47);
            break;
        case PLAT_GPIO_ID_48:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(48);
            break;
        case PLAT_GPIO_ID_49:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(49);
            break;
        case PLAT_GPIO_ID_50:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(50);
            break;
        case PLAT_GPIO_ID_51:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(51);
            break;
        case PLAT_GPIO_ID_52:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(52);
            break;
        default:
            break;
    }

    up(&plat_lock);

    return ret;
}
EXPORT_SYMBOL(plat_gpio_get_value);

void plat_identifier_check(void)
{
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    fmem_get_identifier(&pci_id, &cpu_id);  ///< compile check, panic if check fail
}
EXPORT_SYMBOL(plat_identifier_check);

static int __init plat_init(void)
{
    int ret = 0;
    int fd;

    printk("fe_common version %d\n", PLAT_COMMON_VERSION);

    /* register pmu register for register access */
    fd = ftpmu010_register_reg(&plat_pmu_reg_info_8210);
    if(fd < 0) {
        printk("register pmu register failed!!\n");
        ret = -1;
        goto exit;
    }

    plat_pmu_fd = fd;

    // calculate ssp3 clock
    pmu_cal_clk();

exit:
    return ret;
}

static void __exit plat_exit(void)
{
    int i;

    /* free gpio pin */
    for(i=0; i<PLAT_GPIO_ID_MAX; i++) {
        switch(i) {
            case PLAT_GPIO_ID_X_CAP_RST:
                if(plat_gpio_init[i])
                    gpio_free(PLAT_X_CAP_RST_GPIO_PIN);
                break;
            case PLAT_GPIO_ID_4:
                if(plat_gpio_init[i])
                    gpio_free(4);
                break;
            case PLAT_GPIO_ID_5:
                if(plat_gpio_init[i])
                    gpio_free(5);
                break;
            case PLAT_GPIO_ID_6:
                if(plat_gpio_init[i])
                    gpio_free(6);
                break;
            case PLAT_GPIO_ID_7:
                if(plat_gpio_init[i])
                    gpio_free(7);
                break;
            case PLAT_GPIO_ID_27:
                if(plat_gpio_init[i])
                    gpio_free(27);
                break;
            case PLAT_GPIO_ID_28:
                if(plat_gpio_init[i])
                    gpio_free(28);
                break;
            case PLAT_GPIO_ID_47:
                if(plat_gpio_init[i])
                    gpio_free(47);
                break;
            case PLAT_GPIO_ID_48:
                if(plat_gpio_init[i])
                    gpio_free(48);
                break;
            case PLAT_GPIO_ID_49:
                if(plat_gpio_init[i])
                    gpio_free(49);
                break;
            case PLAT_GPIO_ID_50:
                if(plat_gpio_init[i])
                    gpio_free(50);
                break;
            case PLAT_GPIO_ID_51:
                if(plat_gpio_init[i])
                    gpio_free(51);
                break;
            case PLAT_GPIO_ID_52:
                if(plat_gpio_init[i])
                    gpio_free(52);
                break;
            default:
                break;
        }
    }

    /* deregister pmu register */
    if(plat_pmu_fd >= 0)
        ftpmu010_deregister_reg(plat_pmu_fd);
}

module_init(plat_init);
module_exit(plat_exit);

MODULE_DESCRIPTION("Grain Media GM8210 Front-End Plaftorm Common Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
