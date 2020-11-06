/**
 * @file plat_8287.c
 *  vcap300 platform releated api for GM8287
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.5 $
 * $Date: 2014/05/22 06:14:47 $
 *
 * ChangeLog:
 *  $Log: plat_8287.c,v $
 *  Revision 1.5  2014/05/22 06:14:47  jerson_l
 *  1. add get platfrom chip version api.
 *
 *  Revision 1.4  2013/08/26 07:41:09  jerson_l
 *  1. fix cap3 source LCD0/LCD2 control bit definition
 *
 *  Revision 1.3  2013/08/26 02:21:07  jerson_l
 *  1. remove LCD control pin mux
 *
 *  Revision 1.2  2013/07/23 05:43:25  jerson_l
 *  1. add GM8287 pinmux control api
 *
 *  Revision 1.1  2013/01/15 09:33:34  jerson_l
 *  1. add GM8287 platfrom releated control api
 *
 */
#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "bits.h"
#include "vcap_dev.h"
#include "vcap_dbg.h"
#include "vcap_plat.h"

static int vcap_pmu_fd = 0;

/*
 * GM8287 PMU registers for capture
 */
static pmuReg_t vcap_pmu_reg_8287[] = {
    /*
     * Bus Control Register [offset=0x48]
     * ---------------------------------------------------------------
     * [19:16]cap_swap_reg        ==> 0:disable 1:enable, bit  swap
     * [25]   cap1_cap0_byte_swap ==> 0:disable 1:enable, byte swap
     * [26]   cap3_cap2_byte_swap ==> 0:disable 1:enable, byte swap
     */
    {
     .reg_off   = 0x48,
     .bits_mask = 0x060f0000,
     .lock_bits = 0x060f0000,
     .init_val  = 0,
     .init_mask = 0x060f0000,
    },

    /*
     * Multi-Function Port Setting Register 6 [offset=0x4c]
     * ----------------------------------------------------------------
     *  [19:16]cap_clkin_polarity     ==> 0:positive 1:negative
     *  [20]   cap2_src_sel           ==> 0:X_CAP#2, 1:LCD2
     *  [21]   cap3_lcd_src_sel       ==> 0:LCD2,    1:LCD0
     */
    {
     .reg_off   = 0x4c,
     .bits_mask = 0x003f0000,
     .lock_bits = 0x003f0000,
     .init_val  = 0,
     .init_mask = 0x003f0000,
    },

    /*
     * Multi-Function Port Setting Register 0 [offset=0x50]
     * ----------------------------------------------------------------
     *  [4:3]cap1_pin ==> 0:CAP1, 1:PLL1/PLL3/PLL4/PLL5 div32 out, 2:PWM[5]/PWM[6]/SD, 3:LCD
     *  [6:5]cap2_pin ==> 0:CAP2, 1:GPIO56_63,                     2:LCD
     *  [8:7]cap3_pin ==> 0:CAP3, 1:LCD,                           2:LCD
     */
    {
     .reg_off   = 0x50,
     .bits_mask = 0x000001f8,
     .lock_bits = 0,
     .init_val  = 0,
     .init_mask = 0,        ///< don't init
    },

    /*
     * Schmitt Trigger Control Register [offset=0x68]
     * -----------------------------------------------------------------------
     *  [0]cap_clk_schm ==> 0:disable schmitt trigger 1:enable schmitt trigger
     */
    {
     .reg_off   = 0x68,
     .bits_mask = BIT0,
     .lock_bits = BIT0,
     .init_val  = BIT0,
     .init_mask = BIT0,
    },

    /*
     *  Capture and Display Setting Register [offset=0x7c]
     * --------------------------------------------------------------------
     *  [16]   cap0_cap1_mode_sel ==> 0:bt656 1:bt1120
     *  [17]   cap2_cap3_mode_sel ==> 0:bt656 1:bt1120
     *  [26]   cap3_src_sel       ==> 0:cap2/cap3, 1: lcd0/lcd2
     */
    {
     .reg_off   = 0x7c,
     .bits_mask = 0x04030000,
     .lock_bits = 0x04030000,
     .init_val  = 0x00000000,
     .init_mask = 0x04030000,
    },

    /*
     * IP Software Reset Control Register[offset=0xa0]
     * --------------------------------------------------------------------------------
     * [0]vcap300_rst_n ==> capture in module(aclk) reset, 0:reset active 1:reset in-active(power on default)
     */
    {
     .reg_off   = 0xa0,
     .bits_mask = BIT0,
     .lock_bits = BIT0,
     .init_val  = 0,
     .init_mask = 0,        ///< don't init
    },

    /*
     * AXI Module Clock Off Control Register[offset=0xb0]
     * --------------------------------------------------------------------------------
     * [27]cap300aoff ==> 0:capture module axi clock on 1: capture module axi clock off(power on default)
     */
    {
     .reg_off   = 0xb0,
     .bits_mask = BIT27,
     .lock_bits = BIT27,
     .init_val  = 0,
     .init_mask = BIT27,
    }
};

static pmuRegInfo_t vcap_pmu_reg_info_8287 = {
    "VCAP_8287",
    ARRAY_SIZE(vcap_pmu_reg_8287),
    ATTR_TYPE_AXI,                      ///< capture clock source from AXI Clock
    &vcap_pmu_reg_8287[0]
};

int vcap_plat_pmu_init(void)
{
    int fd;

    fd = ftpmu010_register_reg(&vcap_pmu_reg_info_8287);
    if(fd < 0) {
        vcap_err("register pmu failed!!\n");
        return -1;
    }

    vcap_pmu_fd = fd;

    return 0;
}

void vcap_plat_pmu_exit(void)
{
    if(vcap_pmu_fd) {
        ftpmu010_deregister_reg(vcap_pmu_fd);
        vcap_pmu_fd = 0;
    }
}

u32 vcap_plat_chip_version(void)
{
    return ftpmu010_get_attr(ATTR_TYPE_CHIPVER);
}

int vcap_plat_cap_data_swap(int cap_id, int bit_swap, int byte_swap)
{
    int ret = 0;
    u32 val = 0;
    u32 bit_mask;

    if(!vcap_pmu_fd || cap_id >= VCAP_VI_MAX) {
        ret = -1;
        goto err;
    }

    bit_mask = (0x1<<(16+cap_id)) | (0x1<<(25+(cap_id/2)));

    /* X_CAP# bit swap */
    if(bit_swap & BIT0)
        val |= (0x1<<(16+cap_id));

    /* X_CAP# byte swap */
    if(byte_swap)
        val |= (0x1<<(25+(cap_id/2)));

    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x48, val, bit_mask);

err:
    if(ret < 0)
        vcap_err("cap#%d set data swap failed!!\n", cap_id);

    return ret;
}

int vcap_plat_cap_data_duplicate(int cap_id, int dup_on)
{
    return 0;
}

int vcap_plat_cap_clk_invert(int cap_id, int clk_invert)
{
    int ret = 0;
    u32 val = 0;
    u32 bit_mask;

    if(!vcap_pmu_fd || cap_id >= VCAP_VI_MAX) {
        ret = -1;
        goto err;
    }

    /* X_CAP# pixel clock invert */
    bit_mask = (0x1<<(16+cap_id));
    if(clk_invert)
        val = (0x1<<(16+cap_id));
    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x4c, val, bit_mask);

err:
    if(ret < 0)
        vcap_err("cap#%d set pixel clock invert failed!!\n", cap_id);

    return ret;
}

int vcap_plat_cap_pin_mux(int cap_id, VCAP_PLAT_CAP_PINMUX_T cap_pin)
{
    int ret = 0;
    u32 val;
    u32 bit_mask;

    if(!vcap_pmu_fd || cap_id >= VCAP_VI_MAX || cap_pin >= VCAP_PLAT_CAP_PINMUX_MAX) {
        ret = -1;
        goto err;
    }

    switch(cap_pin) {
        case VCAP_PLAT_CAP_PINMUX_BT656_IN:
            /* pin mux config as input mode, and lock these bit */
            if(cap_id >= 1) {
                bit_mask = 0x3<<(3+(2*(cap_id-1)));

                /* check pmu bit lock or not? */
                if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {
                    ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                    if(ret < 0) {
                        vcap_err("pmu reg#0x50 bit lock failed!!\n");
                        goto err;
                    }
                }

                /* set X_CAP# as input */
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x50, 0, bit_mask);
                if(ret < 0)
                    goto err;

                /* set cap#2 source from X_CAP#2 */
                if(cap_id == 2) {
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x4c, 0, BIT20);
                    if(ret < 0)
                        goto err;
                }
            }

            /* pin mux config as bt656 mode */
            bit_mask = 0x1<<(16+(cap_id/2));
            if(cap_id == VCAP_RGB_VI_IDX)
                bit_mask |= BIT26;   ///< cap3 source from X_CAP#2/X_CAP#3
            ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, 0, bit_mask);
            if(ret < 0)
                goto err;
            break;
        case VCAP_PLAT_CAP_PINMUX_BT1120_IN:
            /* pin mux config as input mode, and lock these bit */
            if((cap_id/2) == 0)
                bit_mask = 0x3<<3;  ///< BT1120 data from X_CAP#0 and X_CAP#1
            else
                bit_mask = 0xf<<5;  ///< BT1120 data from X_CAP#2 and X_CAP#3

            /* check pmu bit lock or not? */
            if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {
                ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                if(ret < 0) {
                    vcap_err("pmu reg#0x50 bit lock failed!!\n");
                    goto err;
                }
            }

            /* set X_CAP# as input */
            ret = ftpmu010_write_reg(vcap_pmu_fd, 0x50, 0, bit_mask);
            if(ret < 0)
                goto err;

            /* set cap#2 source from X_CAP#2 */
            if(cap_id == 2) {
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x4c, 0, BIT20);
                if(ret < 0)
                    goto err;
            }

            /* pin mux config as bt1120 mode */
            bit_mask = val = 0x1<<(16+(cap_id/2));
            if(cap_id == VCAP_RGB_VI_IDX)
                bit_mask |= BIT26;   ///< cap3 source from X_CAP#2/X_CAP#3
            ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);
            if(ret < 0)
                goto err;
            break;
        case VCAP_PLAT_CAP_PINMUX_LCD0_RGB_IN:
            if(cap_id == VCAP_RGB_VI_IDX) {
                /* set cap#3 source from LCD0 */
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x4c, BIT21, BIT21);
                if(ret < 0)
                    goto err;

                /* set cap#3 source from LCD0/LCD2 */
                bit_mask = val = BIT26;
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);
                if(ret < 0)
                    goto err;
            }
            else {
                vcap_err("X_CAP#%d not support LCD#0 RGB pin mux mode\n", cap_id);
                ret = -1;
                goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_LCD2_RGB_IN:
            if(cap_id == VCAP_RGB_VI_IDX) {
                /* set cap#3 source from LCD2 */
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x4c, 0, BIT21);
                if(ret < 0)
                    goto err;

                /* set cap#3 source from LCD0/LCD2 */
                bit_mask = val = BIT26;
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);
                if(ret < 0)
                    goto err;
            }
            else {
                vcap_err("X_CAP#%d not support LCD#2 RGB pin mux mode\n", cap_id);
                ret = -1;
                goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_LCD2_BT656_1120_IN:
            if(cap_id == 2) {
                /* set cap#2 source from LCD2 */
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x4c, BIT20, BIT20);
                if(ret < 0)
                    goto err;
            }
            else {
                vcap_err("X_CAP#%d not support LCD#2 BT656/1120 pin mux mode\n", cap_id);
                ret = -1;
                goto err;
            }
            break;
        default:
            ret = -1;
            break;
    }

err:
    if(ret < 0)
        vcap_err("cap#%d pin mux config failed!!\n", cap_id);

    return ret;
}

int vcap_plat_cap_schmitt_trigger_onoff(int on)
{
    int ret;

    if(on)
        ret = ftpmu010_write_reg(vcap_pmu_fd, 0x68, BIT0, BIT0);
    else
        ret = ftpmu010_write_reg(vcap_pmu_fd, 0x68, 0, BIT0);

    if(ret < 0)
        vcap_err("%s capture clock schmitt trigger failed!!\n", ((on == 0) ? "disable" : "enable"));

    return ret;
}

int vcap_plat_cap_sw_reset_onoff(int on)
{
    int ret;

    if(!vcap_pmu_fd)
        return -1;

    if(on)
        ret = ftpmu010_write_reg(vcap_pmu_fd, 0xa0, 0, BIT0);
    else
        ret = ftpmu010_write_reg(vcap_pmu_fd, 0xa0, BIT0, BIT0);

    return ret;
}

int vcap_plat_cap_axiclk_onoff(int on)
{
    int ret;

    if(!vcap_pmu_fd)
        return -1;

    if(on)
        ret = ftpmu010_write_reg(vcap_pmu_fd, 0xb0, 0, BIT27);
    else
        ret = ftpmu010_write_reg(vcap_pmu_fd, 0xb0, BIT27, BIT27);

    return ret;
}

int vcap_plat_vi_capture_port_config(int vi, struct vcap_plat_cap_port_cfg_t *cfg)
{
    int ret = 0;
    int cap_id;

    if(vi >= VCAP_VI_MAX || !cfg)
        return -1;

    if(cfg->cap_src >= VCAP_PLAT_VI_CAP_SRC_MAX || cfg->cap_fmt >= VCAP_PLAT_VI_CAP_FMT_MAX)
        return -1;

    cap_id = vi;

    switch(cfg->cap_src) {
        case VCAP_PLAT_VI_CAP_SRC_XCAP0:    ///< x_cap#0 can only pinmux as vi#0 source
        case VCAP_PLAT_VI_CAP_SRC_XCAP1:    ///< x_cap#1 can only pinmux as vi#1 source
        case VCAP_PLAT_VI_CAP_SRC_XCAP2:    ///< x_cap#2 can only pinmux as vi#2 source
        case VCAP_PLAT_VI_CAP_SRC_XCAP3:    ///< x_cap#3 can only pinmux as vi#3 source
            if(vi != cfg->cap_src) {
                ret = -1;
                goto exit;
            }

            /* switch pin mux */
            if(cfg->cap_fmt == VCAP_PLAT_VI_CAP_FMT_BT1120)
                ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_BT1120_IN);
            else
                ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_BT656_IN);
            if(ret < 0)
                goto exit;
            break;
        case VCAP_PLAT_VI_CAP_SRC_LCD0:
            if((cap_id == VCAP_RGB_VI_IDX) && (cfg->cap_fmt == VCAP_PLAT_VI_CAP_FMT_RGB888)) { ///< only support LCD#0 RGB In
                ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_LCD0_RGB_IN);
                if(ret < 0)
                    goto exit;
            }
            else {
                ret = -1;
                goto exit;
            }
            break;
        case VCAP_PLAT_VI_CAP_SRC_LCD2:
            if(cap_id == VCAP_RGB_VI_IDX) { ///< only support LCD#2 RGB In
                if(cfg->cap_fmt == VCAP_PLAT_VI_CAP_FMT_RGB888)
                    ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_LCD2_RGB_IN);
                else
                    ret = -1;
                if(ret < 0)
                    goto exit;
            }
            else if(cap_id == 2) {  ///< only support LCD#2 BT656/1120 In
                if(cfg->cap_fmt == VCAP_PLAT_VI_CAP_FMT_RGB888) {
                    ret = -1;
                    goto exit;
                }
                else
                    ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_LCD2_BT656_1120_IN);
                if(ret < 0)
                    goto exit;
            }
            else {
                ret = -1;
                goto exit;
            }
            break;
        default:
            ret = -1;
            goto exit;
    }

    /* X_CAP# data swap */
    ret = vcap_plat_cap_data_swap(cap_id, cfg->bit_swap, cfg->byte_swap);
    if(ret < 0)
        goto exit;

    /* X_CAP# clock invert */
    ret = vcap_plat_cap_clk_invert(cap_id, cfg->inv_clk);
    if(ret < 0)
        goto exit;

exit:
    if(ret < 0) {
        vcap_err("vi#%d capture port config failed!!\n", vi);
    }

    return ret;
}
