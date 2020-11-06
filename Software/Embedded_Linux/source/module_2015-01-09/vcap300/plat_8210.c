/**
 * @file plat_8210.c
 *  vcap300 platform releated api for GM8210
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.5 $
 * $Date: 2014/05/22 06:14:47 $
 *
 * ChangeLog:
 *  $Log: plat_8210.c,v $
 *  Revision 1.5  2014/05/22 06:14:47  jerson_l
 *  1. add get platfrom chip version api.
 *
 *  Revision 1.4  2014/01/20 02:52:08  jerson_l
 *  1. fix incorrect comment message
 *
 *  Revision 1.3  2013/10/14 04:04:04  jerson_l
 *  1. modify data swap definition
 *
 *  Revision 1.2  2013/02/07 12:31:24  jerson_l
 *  1. fixed cascade port no byte swap issue
 *
 *  Revision 1.1  2013/01/15 09:32:53  jerson_l
 *  1. add GM8210 platform releated control api.
 *
 */
#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <mach/ftpmu010.h>

#include "bits.h"
#include "vcap_dev.h"
#include "vcap_dbg.h"
#include "vcap_plat.h"

static int vcap_pmu_fd = 0;

/*
 * GM8210 PMU registers for capture
 */
static pmuReg_t vcap_pmu_reg_8210[] = {
    /*
     * Bus Control Register[offset=0x48]
     * -------------------------------------------------
     * [16:23]cap_swap_reg        ==> 0:disable 1:enable, bit  swap
     * [24]   cas_swap_reg        ==> 0:disable 1:enable, bit  swap
     * [25]   cap1_cap0_byte_swap ==> 0:disable 1:enable, byte swap
     * [26]   cap3_cap2_byte_swap ==> 0:disable 1:enable, byte swap
     * [27]   cap5_cap4_byte_swap ==> 0:disable 1:enable, byte swap
     * [28]   cap7_cap6_byte_swap ==> 0:disable 1:enable, byte swap
     */
    {
     .reg_off   = 0x48,
     .bits_mask = 0x1fff0000,
     .lock_bits = 0x1fff0000,
     .init_val  = 0,
     .init_mask = 0x1fff0000,
    },

    /*
     * Clock In Polarity and Multi-Function Port Setting Register 6[offset=0x4c]
     * -------------------------------------------------------------------------
     *  [0]    cap4_cap0_dup      ==> 0:normal   1:data duplicate
     *  [1]    cap5_cap1_dup      ==> 0:normal   1:data duplicate
     *  [2]    cap6_cap2_dup      ==> 0:normal   1:data duplicate
     *  [3]    cap7_cap3_dup      ==> 0:normal   1:data duplicate
     *  [16:23]cap_clkin_polarity ==> 0:positive 1:negative
     *  [24]   cas_clkin_polarity ==> 0:positive 1:negative
     */
    {
     .reg_off   = 0x4c,
     .bits_mask = 0x01ff000f,
     .lock_bits = 0x01ff000f,
     .init_val  = 0,
     .init_mask = 0x01ff000f,
    },

    /*
     * Schmitt Trigger Control Register[offset=0x68]
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
     * Divider Setting Register 2(offset=0x74)
     * ------------------------------------------------------------------------
     * [20]cap4_lcd1_mux ==> 0:cap4 in on X_CAP4 port 1:LCD1 out to X_CAP4 port
     * [21]cap5_lcd1_mux ==> 0:cap5 in on X_CAP5 port 1:LCD1 out to X_CAP5 port
     * [22]cap6_lcd1_mux ==> 0:cap6 in on X_CAP6 port 1:LCD1 out to X_CAP6 port
     * [23]cap7_lcd1_mux ==> 0:cap7 in on X_CAP7 port 1:LCD1 out to X_CAP7 port
     */
    {
     .reg_off   = 0x74,
     .bits_mask = 0x00f00000,
     .lock_bits = 0,             ///< default don't lock these bit, lcd module may use these pin mux
     .init_val  = 0,
     .init_mask = 0x00f00000,
    },

    /*
     *  Capture and Display Setting Register[offset=0x7c]
     * --------------------------------------------------------------------
     *  [16]   cap0_cap1_mode_sel ==> 0:bt656 1:bt1120
     *  [17]   cap2_cap3_mode_sel ==> 0:bt656 1:bt1120
     *  [18]   cap4_cap5_mode_sel ==> 0:bt656 1:bt1120
     *  [19]   cap6_cap7_mode_sel ==> 0:bt656 1:bt1120
     *  [20]   vcap_cap7_src_sel  ==> 0:cap7  1:cascade
     *  [25:21]vcap_lcdc_src_sel  ==> x0000:lcd0_out  ==> cascade port internal source select
     *                                x0001:lcd1_out
     *                                x001x:lcd2_out
     *                                x0100:lcd0 1120/656out
     *                                x0101:lcd1 1120/656out
     *                                x011x:lcd2 1120/656out
     *                                x1xxx:T_CAPCAS in  ==> cascade port source from external(power on default)
     *                                00xxx:lcd clock to vcap positive edge
     *                                10xxx:lcd clock to vcap negative edge
     */
    {
     .reg_off   = 0x7c,
     .bits_mask = 0x03ff0000,
     .lock_bits = 0x03ff0000,
     .init_val  = 0x01000000,
     .init_mask = 0x03ff0000,
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
    },
};

static pmuRegInfo_t vcap_pmu_reg_info_8210 = {
    "VCAP_8210",
    ARRAY_SIZE(vcap_pmu_reg_8210),
    ATTR_TYPE_AXI,                      ///< capture clock source from AXI Clock
    &vcap_pmu_reg_8210[0]
};

int vcap_plat_pmu_init(void)
{
    int fd;

    fd = ftpmu010_register_reg(&vcap_pmu_reg_info_8210);
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

    if(cap_id == VCAP_CASCADE_VI_NUM)
        bit_mask = (0x1<<(16+cap_id));
    else
        bit_mask = (0x1<<(16+cap_id)) | (0x1<<(25+(cap_id/2)));

    /* X_CAP# bit swap */
    if(bit_swap & BIT0)
        val |= (0x1<<(16+cap_id));

    /* X_CAP# byte swap */
    if(byte_swap && (cap_id != VCAP_CASCADE_VI_NUM))
        val |= (0x1<<(25+(cap_id/2)));

    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x48, val, bit_mask);

err:
    if(ret < 0)
        vcap_err("cap#%d set data swap failed!!\n", cap_id);

    return ret;
}

int vcap_plat_cap_data_duplicate(int cap_id, int dup_on)
{
    int ret = 0;
    u32 val = 0;
    u32 bit_mask;

    if(!vcap_pmu_fd || cap_id >= VCAP_VI_MAX) {
        ret = -1;
        goto err;
    }

    /* X_CAP#4,5,6,7 input data duplicate */
    if(cap_id >= 4 && cap_id <= 7) {
        bit_mask = (0x1<<(cap_id%4));
        if(dup_on)
            val |= (0x1<<(cap_id%4));

        ret = ftpmu010_write_reg(vcap_pmu_fd, 0x4c, val, bit_mask);
    }

err:
    if(ret < 0)
        vcap_err("cap#%d set data duplicate to cap#%d failed!!\n", cap_id, (cap_id%4));

    return ret;
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

    if(cap_id != VCAP_CASCADE_VI_NUM) {
        /* X_CAP# pixel clock invert */
        bit_mask = (0x1<<(16+cap_id));
        if(clk_invert)
            val = (0x1<<(16+cap_id));

        ret = ftpmu010_write_reg(vcap_pmu_fd, 0x4c, val, bit_mask);
    }
    else {
        u32 tmp = ftpmu010_read_reg(0x7c);

        if(tmp & (0x8<<21)) {   ///< cascade source from CAPCAS
            bit_mask = BIT24;
            if(clk_invert)
                val = BIT24;

            ret = ftpmu010_write_reg(vcap_pmu_fd, 0x4c, val, bit_mask);
            if(ret < 0)
                goto err;

            bit_mask = BIT25;
            ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, 0, bit_mask);
        }
        else {  ///< cascade source from internal LCD
            bit_mask = BIT24;
            ret = ftpmu010_write_reg(vcap_pmu_fd, 0x4c, 0, bit_mask);
            if(ret < 0)
                goto err;

            bit_mask = BIT25;
            if(clk_invert)
                val = BIT25;

            ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);
        }
    }

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
            if(cap_id != VCAP_CASCADE_VI_NUM) {
                /* pin mux config as input mode, and lock these bit */
                if(cap_id >= 4) {
                    bit_mask = 0x1<<(20+(cap_id%4));

                    /* check pmu bit lock or not? */
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x74, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x74, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x74 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP# as input */
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x74, 0, bit_mask);
                    if(ret < 0)
                        goto err;
                }

                /* pin mux config as bt656 mode */
                bit_mask = 0x1<<(16+(cap_id/2));
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, 0, bit_mask);
                if(ret < 0)
                    goto err;
            }
            else {
                vcap_err("X_CAPCAS not support BT656 pin mux mode\n");
                ret = -1;
                goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_BT1120_IN:
            if(cap_id != VCAP_CASCADE_VI_NUM) {
                /* pin mux config as input mode, and lock these bit */
                if(cap_id >= 4) {
                    bit_mask = 0x3<<(20+(((cap_id-4)/2)*2));

                    /* check pmu bit lock or not? */
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x74, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x74, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x74 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP# as input */
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x74, 0, bit_mask);
                    if(ret < 0)
                        goto err;
                }

                /* pin mux config as bt1120 mode */
                bit_mask = val = 0x1<<(16+(cap_id/2));
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);
                if(ret < 0)
                    goto err;
            }
            else {
                vcap_err("X_CAPCAS not support BT1120 pin mux mode\n");
                ret = -1;
                goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_CASCADE_IN:
            if(cap_id == 7) {
                /* pin mux config as input mode, and lock these bit */
                bit_mask = BIT23;

                /* check pmu bit lock or not? */
                if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x74, bit_mask) < 0) {
                    ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x74, bit_mask);
                    if(ret < 0) {
                        vcap_err("pmu reg#0x74 bit lock failed!!\n");
                        goto err;
                    }
                }

                /* set X_CAP#7 as input */
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x74, 0, bit_mask);
                if(ret < 0)
                    goto err;

                /* set X_CAP#7 source from CASCAP */
                bit_mask = BIT20 | BIT19;
                val      = BIT20;
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);
                if(ret < 0)
                    goto err;
            }
            else if(cap_id == VCAP_CASCADE_VI_NUM) {
                /* set X_CAPCAS source from CAPCAS */
                bit_mask = (0xf<<21);
                val      = (0x8<<21);
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);
                if(ret < 0)
                    goto err;
            }
            else {
                vcap_err("X_CAP#%d not support CAPCAS pin mux mode\n", cap_id);
                ret = -1;
                goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_LCD0_RGB_IN:
            if(cap_id == VCAP_CASCADE_VI_NUM) {
                /* set X_CAPCAS source from LCD0 RGB */
                bit_mask = (0xf<<21);
                val      = 0;
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
        case VCAP_PLAT_CAP_PINMUX_LCD1_RGB_IN:
            if(cap_id == VCAP_CASCADE_VI_NUM) {
                /* set X_CAPCAS source from LCD1 RGB */
                bit_mask = (0xf<<21);
                val      = (0x1<<21);
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);
                if(ret < 0)
                    goto err;
            }
            else {
                vcap_err("X_CAP#%d not support LCD#1 RGB pin mux mode\n", cap_id);
                ret = -1;
                goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_LCD2_RGB_IN:
            if(cap_id == VCAP_CASCADE_VI_NUM) {
                /* set X_CAPCAS source from LCD2 RGB */
                bit_mask = (0xf<<21);
                val      = (0x2<<21);
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
        case VCAP_PLAT_CAP_PINMUX_LCD0_BT656_1120_IN:
            if(cap_id == VCAP_CASCADE_VI_NUM) {
                /* set X_CAPCAS source from LCD0 BT656/1120 */
                bit_mask = (0xf<<21);
                val      = (0x4<<21);
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);
                if(ret < 0)
                    goto err;
            }
            else {
                vcap_err("X_CAP#%d not support LCD#0 BT656/1120 pin mux mode\n", cap_id);
                ret = -1;
                goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_LCD1_BT656_1120_IN:
            if(cap_id == VCAP_CASCADE_VI_NUM) {
                /* set X_CAPCAS source from LCD1 BT656/1120 */
                bit_mask = (0xf<<21);
                val      = (0x5<<21);
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);
                if(ret < 0)
                    goto err;
            }
            else {
                vcap_err("X_CAP#%d not support LCD#1 BT656/1120 pin mux mode\n", cap_id);
                ret = -1;
                goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_LCD2_BT656_1120_IN:
            if(cap_id == VCAP_CASCADE_VI_NUM) {
                /* set X_CAPCAS source from LCD2 BT656/1120 */
                bit_mask = (0xf<<21);
                val      = (0x6<<21);
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);
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

    if(vi == VCAP_CASCADE_VI_NUM) {
        /* switch pin mux */
        switch(cfg->cap_src) {
            case VCAP_PLAT_VI_CAP_SRC_XCAPCAS:
                ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_CASCADE_IN);
                if(ret < 0)
                    goto exit;
                break;
            case VCAP_PLAT_VI_CAP_SRC_LCD0:
            case VCAP_PLAT_VI_CAP_SRC_LCD1:
            case VCAP_PLAT_VI_CAP_SRC_LCD2:
                if(cfg->cap_fmt == VCAP_PLAT_VI_CAP_FMT_RGB888)
                    ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_LCD0_RGB_IN + (cfg->cap_src - VCAP_PLAT_VI_CAP_SRC_LCD0));
                else
                    ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_LCD0_BT656_1120_IN + (cfg->cap_src - VCAP_PLAT_VI_CAP_SRC_LCD0));
                if(ret < 0)
                    goto exit;
                break;
            default:
                ret = -1;
                goto exit;
        }
    }
    else {
        if(cfg->cap_fmt == VCAP_PLAT_VI_CAP_FMT_RGB888) {   ///< only cascade support RGB888 format
            ret = -1;
            goto exit;
        }

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
            case VCAP_PLAT_VI_CAP_SRC_XCAP4:    ///< x_cap#4 can only pinmux as vi#0, vi#4 source
            case VCAP_PLAT_VI_CAP_SRC_XCAP5:    ///< x_cap#5 can only pinmux as vi#1, vi#5 source
            case VCAP_PLAT_VI_CAP_SRC_XCAP6:    ///< x_cap#6 can only pinmux as vi#2, vi#6 source
            case VCAP_PLAT_VI_CAP_SRC_XCAP7:    ///< x_cap#7 can only pinmux as vi#3, vi#7 source
                if((vi != cfg->cap_src) && (vi != (cfg->cap_src%4))) {
                    ret = -1;
                    goto exit;
                }

                /* data duplicate */
                if(vi == cfg->cap_src) {
                    ret = vcap_plat_cap_data_duplicate(cap_id, 0);  ///< disable x_cap#4~7 duplicate to vi#0~3
                }
                else {
                    cap_id = cfg->cap_src;
                    ret = vcap_plat_cap_data_duplicate(cap_id, 1);  ///< enable  x_cap#4~7 duplicate to vi#0~3
                }
                if(ret < 0)
                    goto exit;

                /* switch pin mux */
                if(cfg->cap_fmt == VCAP_PLAT_VI_CAP_FMT_BT1120)
                    ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_BT1120_IN);
                else
                    ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_BT656_IN);
                if(ret < 0)
                    goto exit;
                break;
            case VCAP_PLAT_VI_CAP_SRC_XCAPCAS:  ///< x_capcas can link to vi#7, cascade
                if(vi != 7) {
                    ret = -1;
                    goto exit;
                }

                ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_CASCADE_IN);
                if(ret < 0)
                    goto exit;

                cap_id = VCAP_CASCADE_VI_NUM;
                break;
            default:
                ret = -1;
                goto exit;
        }
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
