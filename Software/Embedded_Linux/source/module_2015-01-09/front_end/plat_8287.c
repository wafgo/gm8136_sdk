/**
 * @file plat_8287.c
 * platform releated api for GM8287
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.21 $
 * $Date: 2014/12/22 08:06:00 $
 *
 * ChangeLog:
 *  $Log: plat_8287.c,v $
 *  Revision 1.21  2014/12/22 08:06:00  andy_hsu
 *  support GM828x SSP2 get clock from PLL5
 *
 *  Revision 1.20  2014/09/15 12:17:36  shawn_hu
 *  Add TechPoint TP2802(HDTVI) & Intersil TW2964(D1) hybrid frontend driver design for GM8286 EVB.
 *
 *  Revision 1.19  2014/07/14 09:52:30  jerson_l
 *  1. change VERSION definition to PLAT_COMMON_VERSION
 *
 *  Revision 1.18  2014/07/14 07:23:33  ccsu
 *  move version control define to platform.h
 *
 *  Revision 1.17  2014/07/14 06:48:17  ccsu
 *  add version control
 *
 *  Revision 1.16  2014/07/14 02:56:31  jerson_l
 *  1. fix ext_clk driving wrong bit mask problem
 *
 *  Revision 1.15  2014/07/14 02:44:00  jerson_l
 *  1. add ext_clk driving control api
 *
 *  Revision 1.14  2014/03/03 08:03:22  ccsu
 *  modify ssp2 main clock to 12.288Mhz
 *
 *  Revision 1.13  2014/01/27 05:31:32  ccsu
 *  modify ssp0~2 init_mask
 *
 *  Revision 1.12  2014/01/18 14:43:08  ccsu
 *  fix compiler error
 *
 *  Revision 1.11  2014/01/16 09:59:59  easonli
 *  *:[platform] add get_digital_codec_name()
 *
 *  Revision 1.10  2014/01/16 09:44:19  easonli
 *  *:[platform] add digital_codec_name check
 *
 *  Revision 1.9  2014/01/10 09:53:51  ccsu
 *  modify ssp2 clock source = 12Mhz
 *
 *  Revision 1.8  2013/12/03 16:54:26  easonli
 *  *:[frontend] add ssp2 scu setting for GM8287
 *
 *  Revision 1.7  2013/11/13 09:19:34  easonli
 *  *:[audio] add get codec name function and modified audio job member for av sync
 *
 *  Revision 1.6  2013/10/31 05:18:24  jerson_l
 *  1. add SSCG control api
 *
 *  Revision 1.5  2013/10/11 10:27:12  jerson_l
 *  1. update platform gpio pin register api
 *
 *  Revision 1.4  2013/09/16 08:53:11  ccsu
 *  add ssp0~2 pmu setting for audio
 *
 *  Revision 1.3  2013/09/03 08:14:10  easonli
 *  *:[front_end] fixed compiling error
 *
 *  Revision 1.2  2013/09/02 08:28:19  easonli
 *  *:[front_end] add audio function for GM8287
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

#define PLAT_EXTCLK_MAX             1       ///< EXT0_CLK
#define PLAT_X_CAP_RST_GPIO_PIN     0       ///< GPIO#0 as X_CAP_RST pin

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static DEFINE_SEMAPHORE(plat_lock);
#else
static DECLARE_MUTEX(plat_lock);
#endif

static audio_funs_t plat_audio_funs;

static int plat_pmu_fd = -1;
static int plat_gpio_init[PLAT_GPIO_ID_MAX] = {[0 ... (PLAT_GPIO_ID_MAX - 1)] = 0};

/*
 * GM8287 PMU registers for video decoder
 */
static pmuReg_t plat_pmu_reg_8287[] = {
    /*
     * Module Clock Selecting Register[offset=0x28]
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
     .lock_bits = 0,
     .init_val  = 0x15<<20,
     .init_mask = 0x3f<<20,
    },

    /*
     * Driving Capacity [offset=0x40]
     * -------------------------------------------------
     * [20:23]ext_clk => [23]slow rate 1:slow [21:22]0:4mA, 1:8mA, 2:12mA, 3:16mA
     */
    {
     .reg_off   = 0x40,
     .bits_mask = 0x00f00000,
     .lock_bits = 0x0,
     .init_val  = 0x0,
     .init_mask = 0x0,
    },

    /*
     * Divider Setting Register 3[offset=0x78]
     * -------------------------------------------------
     * [11:6]extclk_pvalue ==> reg28[13:12]/(extclk_pvalue + 1), default: 0
     */
    {
     .reg_off   = 0x78,
     .bits_mask = 0x1f<<6,
     .lock_bits = 0x0,
     .init_val  = 0x0,
     .init_mask = 0x0,
    },

    /*
     * Divider Setting Register 2[offset=0x74]
     * -------------------------------------------------
     * [17:12]ssp2clk_pvalue ==> ssp2clk = PCIE pipeclk / (ssp2clk_pvalue + 1)
     */
    {
     .reg_off   = 0x74,
     .bits_mask = 0x3F<<12,
     .lock_bits = 0x0,
     .init_val  = 0x0, //0x9<<12,
     .init_mask = 0x0 //0x3F<<12
    },

    /*
     * AXI Module Clock Off Control Register[offset=0xb0]
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
     * SSP clock value [offset=0x60]
     * ---------------------------------------------------------
     * [13:12] : select 12.288MHz source for analog die
     *    1x : 12MHz crystal in
     *    01 : 12.288MHz crystal in
     *    00 : ssp6144_clk ((pll4out1, pll4out2 or pll4out3) / ([24:22]+1)/([21:17]+1))
     * SSP 6.144MHz clock pvalue
     * [16] : gating clock control, 1-gating, 0-un-gating
     * [21:17] : ssp6144_clk_cnt5_pvalue
     * [24:22] : ssp6144_clk_cnt3_pvalue
     * [26:25] : ssp6144_clk_sel, 00-PLL4 CKOUT1, 01-PLL4 CKOUT2, 1x-PLL4 CKOUT3
     */
     {
     .reg_off   = 0x60,
     .bits_mask = (0x3 << 12) | (0x1 << 16) | (0x1f << 17) | (0xf << 22) | (0x3 << 25),
     .lock_bits = 0,
     .init_val  = (0x0 << 12) | (0x0 << 16) | (0x9 << 17) | (0x2 << 22) | (0x1 << 25),
     .init_mask = (0x3 << 12) | (0x1 << 16) | (0x1f << 17) | (0xf << 22) | (0x3 << 25),
    },

    /*
     * SSP0~2 pclk & sclk [offset=0xb8]
     * ---------------------------------------------------------
     * [6]  ssp0 pclk & sclk
     * [7]  ssp1 pclk & sclk
     * [8]  ssp2 pclk & sclk
     */
     {
     .reg_off   = 0xb8,
     .bits_mask = 0x7<<6,
     .lock_bits = 0,
     .init_val  = 0x0,
     .init_mask = 0x7<<6,
    },
    /*
     * Multi-function Port Setting Register 3 [offset=0x5c]
     * ---------------------------------------------------------
     * [23:22]: 00 as GPIO[9:8]
     * [19:18]: 00 as GPIO[16:15]
     */
    {
     .reg_off   = 0x5c,
     .bits_mask = 0xCC0000,
     .lock_bits = 0x0,
     .init_val  = 0x0,
     .init_mask = 0x0,
    }
};

static pmuRegInfo_t plat_pmu_reg_info_8287 = {
    "FE_8287",
    ARRAY_SIZE(plat_pmu_reg_8287),
    ATTR_TYPE_NONE,
    &plat_pmu_reg_8287[0]
};

typedef struct pvalue {
    int bit17_21;
    int bit22_24;
} pvalue_t;

/*
 * find out pvalue->bit17_21 & pvalue->bit22_24 to get ssp2 clock
 * ssp2 clock = ((pll4out1, pll4out2 or pll4out3) / ([24:22]+1)/([21:17]+1))
 * div = (pll4out1, pll4out2 or pll4out3) / ssp2 clock
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

/*  SSP2 playback to HDMI, sample rate = 48k, sample size = 16 bits, bit clock = 3072000, SSP2 MAIN CLOCK must = 24MHz
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
 *  SSP2 choose clk_12288 as main clock, clk_12288 form ssp6144_clk,
 */
void pmu_cal_clk(void)
{
    unsigned int pll4_out1, div;
    unsigned int pll4_out2, pll4_out3;
    unsigned int ssp2_clk = 12288000;
    pvalue_t pvalue;
    //int pll4_clkout1 = 0, pll4_clkout2 = 0, pll4_clkout3 = 0;
    int pll4_clkout3 = 0;
    int bit25_26 = 0, flag = 0;
    int ret = 0;

    unsigned int pll5_out;

    //return;   //GM8287 ssp2's mclk don't need to be 24Mhz, use default 12Mhz is fine

    memset(&pvalue, 0x0, sizeof(pvalue_t));

    pll4_out1 = ftpmu010_get_attr(ATTR_TYPE_PLL4);   //hz
    pll4_out2 = pll4_out1 * 2 / 3;   // pll4_out1 / 1.5
    pll4_out3 = pll4_out1 * 2 / 5;   // pll4_out1 / 2.5

    pll5_out = ftpmu010_get_attr(ATTR_TYPE_PLL5);
    
#if 0
    if (pll4_out1 != 768000000) {
        ssp2_clk = 24000000;
    div = ftpmu010_clock_divisor2(ATTR_TYPE_PLL4, ssp2_clk, 1);
    /* get pvalue from pll4_out1 */
    flag = ssp_get_pvalue(&pvalue, div);
    if (flag == 1)
        pll4_clkout1 = 1;
    /* if flag == 0, get pvalue from pll4_out2 */
    if (flag == 0) {
        div = pll4_out2 / ssp2_clk;
        flag = ssp_get_pvalue(&pvalue, div);
        if (flag == 1)
            pll4_clkout2 = 1;
    }
    /* if flag == 0, get pvalue from pll4_out3 */
    if (flag == 0) {
        div = pll4_out3 / ssp2_clk;
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
#endif
    if (pll4_out1 == 768000000) {
        ssp2_clk = 12288000;
        div = pll4_out3 / ssp2_clk;
        flag = ssp_get_pvalue(&pvalue, div);
        if (flag == 1)
            pll4_clkout3 = 1;

        if (pll4_clkout3)
            bit25_26 = 2;
    }
    else if (pll5_out == 750000000) {
        ret = ftpmu010_write_reg(plat_pmu_fd, 0x28, (0x2 << 20), (0x3 << 20));
        if (ret < 0)
            panic("%s, register ssp2 clock on failed!\n", __func__);

        // set 0x74[17:12] as 60(decimal)
        ret = ftpmu010_write_reg(plat_pmu_fd, 0x74, (0x3C << 12), (0x3F << 12));
        if (ret < 0)
            panic("%s, set reg 0x74 failed!\n", __func__);

        return;
    }
    else
        return;   //GM8287 ssp2's mclk don't need to be 24Mhz, use default 12Mhz is fine

    ret = ftpmu010_write_reg(plat_pmu_fd, 0x60, ((0x2 << 12) | (0x0 << 16) | ((pvalue.bit17_21 - 1) << 17) | ((pvalue.bit22_24 - 1) << 22) | (bit25_26 << 25)), ((0x3 << 12) | (0x3FF << 17) | (0x1 << 16)));
    if (ret < 0)
        panic("%s, register ssp2 pvalue fail\n", __func__);
    // SSP2 clk_sel = clk_12288
    ret = ftpmu010_write_reg(plat_pmu_fd, 0x28, 0x0 << 20, 0x3 << 20);
    if (ret < 0)
        panic("%s, register ssp2 clock on fail\n", __func__);
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
    ret = ftpmu010_write_reg(plat_pmu_fd, 0x78, (div&0x1f)<<6, 0x1f<<6);

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
            /* X_CAP_RST GPIO pin request */
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
        case PLAT_GPIO_ID_8:
        case PLAT_GPIO_ID_9:
            /* check pmu bit lock or not? */
            bit_mask = 0x3<<22; // [23:22]
            if(ftpmu010_bits_is_locked(plat_pmu_fd, 0x5C, bit_mask) < 0) {
                ret = ftpmu010_add_lockbits(plat_pmu_fd, 0x5C, bit_mask);
                if(ret < 0) {
                    printk("pmu reg#0x5C bit lock failed!!\n");
                    goto err;
                }
            }

            /* set pin as GPIO */
            val = 0x0; // [23:22]: 00 as GPIO[9:8]
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x5C, val, bit_mask);
            if(ret < 0)
                goto err;

            if(gpio_id == PLAT_GPIO_ID_8) {
                if(gpio_request(8, "gpio_8") < 0) {
                    printk("gpio#8 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(8);
                else
                    gpio_direction_output(8, out_value);
            }
            else { // gpio_id == PLAT_GPIO_ID_9
                if(gpio_request(9, "gpio_9") < 0) {
                    printk("gpio#9 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(9);
                else
                    gpio_direction_output(9, out_value);
            }

            plat_gpio_init[gpio_id] = 1;
            break;
        case PLAT_GPIO_ID_15:
        case PLAT_GPIO_ID_16:
            /* check pmu bit lock or not? */
            bit_mask = 0x3<<18; // [19:18]
            if(ftpmu010_bits_is_locked(plat_pmu_fd, 0x5C, bit_mask) < 0) {
                ret = ftpmu010_add_lockbits(plat_pmu_fd, 0x5C, bit_mask);
                if(ret < 0) {
                    printk("pmu reg#0x5C bit lock failed!!\n");
                    goto err;
                }
            }

            /* set pin as GPIO */
            val = 0x0; // [19:18]: 00 as GPIO[16:15]
            ret = ftpmu010_write_reg(plat_pmu_fd, 0x5C, val, bit_mask);
            if(ret < 0)
                goto err;

            /* GPIO pin request */
            if(gpio_id == PLAT_GPIO_ID_15) {
                if(gpio_request(15, "gpio_15") < 0) {
                    printk("gpio#15 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(15);
                else
                    gpio_direction_output(15, out_value);
            }
            else { // gpio_id == PLAT_GPIO_ID_16
                if(gpio_request(16, "gpio_16") < 0) {
                    printk("gpio#16 request failed!\n");
                    ret = -1;
                    goto err;
                }

                /* Config direction */
                if(direction == PLAT_GPIO_DIRECTION_IN)
                    gpio_direction_input(16);
                else
                    gpio_direction_output(16, out_value);
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
        case PLAT_GPIO_ID_8:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(8);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_9:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(9);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_15:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(15);
                plat_gpio_init[gpio_id] = 0;
            }
            break;
        case PLAT_GPIO_ID_16:
            if(plat_gpio_init[gpio_id]) {
                /* free gpio */
                gpio_free(16);
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
                printk("gpio set value failed!(un-register gpio_id:%d)\n", gpio_id);
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_8:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(8, value);
            else {
                printk("gpio#8 set value failed!(un-register gpio_8)\n");
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_9:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(9, value);
            else {
                printk("gpio#9 set value failed!(un-register gpio_9)\n");
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_15:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(15, value);
            else {
                printk("gpio#15 set value failed!(un-register gpio_15)\n");
                ret = -1;
            }
            break;
        case PLAT_GPIO_ID_16:
            if(plat_gpio_init[gpio_id])
                gpio_set_value(16, value);
            else {
                printk("gpio#16 set value failed!(un-register gpio_16)\n");
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
        case PLAT_GPIO_ID_8:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(8);
            break;
        case PLAT_GPIO_ID_9:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(9);
            break;
        case PLAT_GPIO_ID_15:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(15);
            break;
        case PLAT_GPIO_ID_16:
            if(plat_gpio_init[gpio_id])
                ret = gpio_get_value(16);
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
    fd = ftpmu010_register_reg(&plat_pmu_reg_info_8287);
    if(fd < 0) {
        printk("register pmu register failed!!\n");
        ret = -1;
        goto exit;
    }

    plat_pmu_fd = fd;

    // calculate ssp2 clock
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
            case PLAT_GPIO_ID_8:
                if(plat_gpio_init[i])
                    gpio_free(8);
                break;
            case PLAT_GPIO_ID_9:
                if(plat_gpio_init[i])
                    gpio_free(9);
                break;
            case PLAT_GPIO_ID_15:
                if(plat_gpio_init[i])
                    gpio_free(15);
                break;
            case PLAT_GPIO_ID_16:
                if(plat_gpio_init[i])
                    gpio_free(16);
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

MODULE_DESCRIPTION("Grain Media GM8287 Front-End Plaftorm Common Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
