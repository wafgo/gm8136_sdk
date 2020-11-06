/**
 * @file plat_8139.c
 *  vcap300 platform releated api for GM8139
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.6 $
 * $Date: 2014/08/13 12:14:07 $
 *
 * ChangeLog:
 *  $Log: plat_8139.c,v $
 *  Revision 1.6  2014/08/13 12:14:07  jerson_l
 *  1. change bt1120 and bt601_16bit to use BAYER_CLK as VI#0 clock source
 *
 *  Revision 1.5  2014/05/22 06:14:47  jerson_l
 *  1. add get platfrom chip version api.
 *
 *  Revision 1.4  2013/11/26 12:01:17  jerson_l
 *  1. correct SoC BGA package id definition
 *
 *  Revision 1.3  2013/10/31 05:52:23  jerson_l
 *  1. check SoC package id to setup different pin of pixel clock input
 *
 *  Revision 1.2  2013/10/23 09:40:34  jerson_l
 *  1. modify pinmux config
 *
 *  Revision 1.1  2013/10/14 04:02:51  jerson_l
 *  1. support GM8139 platform
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

#define GM8139_SOC_PKG_ID_BGA256            0x6     ///< have CAP0_CLK and BAYER_CLK pin, others only have BAYER_CLK pin
#define GM8139_SOC_VI_16BIT_USE_BAYER_CLK   1

static int vcap_pmu_fd = 0;

/*
 * GM8139 PMU registers for capture
 */
static pmuReg_t vcap_pmu_reg_8139[] = {
    /*
     * Multi-Function Port Setting Register 0 [offset=0x50]
     * -------------------------------------------------
     * [14:15]bayer_clk  ==> 0: None,      1: CAP0_CLK,    2: BAYER_CLK
     * [16:17]dp3        ==> 0: None,      1: CAP_HS,      2: BAYER_HS
     * [18:19]dn3        ==> 0: None,      1: CAP_VS,      2: BAYER_VS
     * [20:21]dp1        ==> 0: None,      1: CAP_HI8_D7,  2: BAYER_D11
     * [22:23]dn1        ==> 0: None,      1: CAP_HI8_D6,  2: BAYER_D10
     * [24:25]ckp        ==> 0: None,      1: CAP_HI8_D5,  2: BAYER_D9
     * [26:27]ckn        ==> 0: None,      1: CAP_HI8_D4,  2: BAYER_D8
     * [28:29]dp0        ==> 0: None,      1: CAP_HI8_D3,  2: BAYER_D7
     * [30:31]dn0        ==> 0: None,      1: CAP_HI8_D2,  2: BAYER_D6
     */
    {
     .reg_off   = 0x50,
     .bits_mask = 0xffffc000,
     .lock_bits = 0x00000000,   ///< default don't lock these bit, ISP module may use these pin mux
     .init_val  = 0,
     .init_mask = 0,            ///< don't init
    },

    /*
     * Multi-Function Port Setting Register 1 [offset=0x54]
     * -------------------------------------------------
     * [0:1]  dp2      ==> 0: None,       1: CAP_HI8_D1, 2: BAYER_D5
     * [2:3]  dn2      ==> 0: None,       1: CAP_HI8_D0, 2: BAYER_D4
     * [4:5]  cap0_clk ==> 0: GPIO_0[9],  1: CAP0_CLK,   2: None
     * [6:7]  cap0_d7  ==> 0: GPIO_0[10], 1: CAP_LO8_D7, 2: Bayer_D3
     * [8:9]  cap0_d6  ==> 0: GPIO_0[11], 1: CAP_LO8_D6, 2: Bayer_D2
     * [10:11]cap0_d5  ==> 0: GPIO_0[12], 1: CAP_LO8_D5, 2: Bayer_D1
     * [12:13]cap0_d4  ==> 0: GPIO_0[13], 1: CAP_LO8_D4, 2: Bayer_D0
     * [14:15]cap0_d3  ==> 0: GPIO_0[14], 1: CAP_LO8_D3, 2: I2C_SCL, 3:SSP1_FS
     * [16:17]cap0_d2  ==> 0: GPIO_0[15], 1: CAP_LO8_D2, 2: I2C_SDA, 3:SSP1_RXD
     * [18:19]cap0_d1  ==> 0: GPIO_0[16], 1: CAP_LO8_D1, 2: None,    3:SSP1_TXD
     * [20:21]cap0_d0  ==> 0: GPIO_0[17], 1: CAP_LO8_D0, 2: None     3:SSP1_SCLK
     */
    {
     .reg_off   = 0x54,
     .bits_mask = 0x003fffff,
     .lock_bits = 0x00000000,   ///< default don't lock these bit, ISP module may use these pin mux
     .init_val  = 0,
     .init_mask = 0,            ///< don't init
    },

    /*
     * System Control Register [offset=0x7c]
     * -------------------------------------------------
     * [8:10] cap0_data_swap ==> 1: cap0 low_8 bit swap, 2: cap0 high_8 bit swap, 4: cap0 high/low byte swap
     * [12]   cap0_clk_inv   ==> 0: disable 1: enable
     */
    {
     .reg_off   = 0x7c,
     .bits_mask = 0x00001700,
     .lock_bits = 0x00001700,
     .init_val  = 0,
     .init_mask = 0x00001700,
    },

    /*
     * IP Software Reset Control Register [offset=0xa0]
     * -------------------------------------------------
     * [0]vcap300_rst_n ==> capture in module(aclk) reset, 0:reset active 1:reset in-active(power on default)
     */
    {
     .reg_off   = 0xa0,
     .bits_mask = BIT0,
     .lock_bits = BIT0,
     .init_val  = 0,
     .init_mask = 0,            ///< don't init
    },

    /*
     * AXI Module Clock Off Control Register [offset=0xb0]
     * -------------------------------------------------
     * [17]cap300aoff ==> 0:capture module axi clock on 1: capture module axi clock off(power on default)
     */
    {
     .reg_off   = 0xb0,
     .bits_mask = BIT17,
     .lock_bits = BIT17,
     .init_val  = 0,
     .init_mask = BIT17,
    }
};

static pmuRegInfo_t vcap_pmu_reg_info_8139 = {
    "VCAP_8139",
    ARRAY_SIZE(vcap_pmu_reg_8139),
    ATTR_TYPE_AXI,                      ///< capture clock source from AXI Clock
    &vcap_pmu_reg_8139[0]
};

int vcap_plat_pmu_init(void)
{
    int fd;

    fd = ftpmu010_register_reg(&vcap_pmu_reg_info_8139);
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

    if(cap_id == 0) {   ///< only X_CAP#0 support data swap
        bit_mask = BIT8 | BIT9 | BIT10;

        /* X_CAP#0 low_8 bit swap */
        if(bit_swap & BIT0)
            val |= BIT8;

        /* X_CAP#0 high_8 bit swap */
        if(bit_swap & BIT1)
            val |= BIT9;

        /* X_CAP#0 byte swap */
        if(byte_swap)
            val |= BIT10;

        ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);
    }

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

    if(cap_id == 0) {   ///< only X_CAP#0 support clock invert
        bit_mask = BIT12;

        if(clk_invert)
            val = BIT12;

        ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);
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
    u32 pkg_id = 0;

    if(!vcap_pmu_fd || cap_id >= VCAP_VI_MAX || cap_pin >= VCAP_PLAT_CAP_PINMUX_MAX) {
        ret = -1;
        goto err;
    }

    /* Get GM8139 Package ID */
    pkg_id = (ftpmu010_read_reg(0x00)>>8) & 0x7;

    switch(cap_pin) {
        case VCAP_PLAT_CAP_PINMUX_BT656_IN:
            if(cap_id != VCAP_CASCADE_VI_NUM) {
                if(pkg_id != GM8139_SOC_PKG_ID_BGA256) {                            ///< not BGA256 only have BAYER_CLK input pin
                    bit_mask = 0x0000c000;
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {  ///< use BAYER_CLK as clock pin
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x50 bit lock failed!!\n");
                            goto err;
                        }

                        val = 0x00004000;
                        ret = ftpmu010_write_reg(vcap_pmu_fd, 0x50, val, bit_mask);
                        if(ret < 0)
                            goto err;
                    }

                    /* check pmu bit lock or not? */
                    bit_mask = 0x003fffc0;
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x54, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x54, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x54 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP#0 as input */
                    val = 0x00155540;
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x54, val, bit_mask);
                    if(ret < 0)
                        goto err;
                }
                else {
                    /* check pmu bit lock or not? */
                    bit_mask = 0x003ffff0;  ///< use CAP0_CLK as clock pin
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x54, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x54, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x54 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP#0 as input */
                    val = 0x00155550;
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x54, val, bit_mask);
                    if(ret < 0)
                        goto err;
                }
            }
            else {
                vcap_err("X_CAP#%d not support BT656 pin mux mode\n", cap_id);
                ret = -1;
                goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_BT1120_IN:
            if(cap_id != VCAP_CASCADE_VI_NUM) {
                if((pkg_id != GM8139_SOC_PKG_ID_BGA256) || GM8139_SOC_VI_16BIT_USE_BAYER_CLK) {
                    /* check pmu bit lock or not? */
                    bit_mask = 0x003fffcf;
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x54, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x54, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x54 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP#0 as input */
                    val = 0x00155545;
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x54, val, bit_mask);
                    if(ret < 0)
                        goto err;

                    /* check pmu bit lock or not? */
                    bit_mask = 0xfff0c000;
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x50 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP#1 as input */
                    val = 0x55504000;
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x50, val, bit_mask);
                    if(ret < 0)
                        goto err;
                }
                else {
                    /* check pmu bit lock or not? */
                    bit_mask = 0x003fffff;
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x54, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x54, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x54 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP#0 as input */
                    val = 0x00155555;
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x54, val, bit_mask);
                    if(ret < 0)
                        goto err;

                    /* check pmu bit lock or not? */
                    bit_mask = 0xfff00000;
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x50 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP#1 as input */
                    val = 0x55500000;
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x50, val, bit_mask);
                    if(ret < 0)
                        goto err;
                }
            }
            else {
                vcap_err("X_CAP#%d not support BT1120 pin mux mode\n", cap_id);
                ret = -1;
                goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_BT601_8BIT_IN:
            if(cap_id != VCAP_CASCADE_VI_NUM) {
                if(pkg_id != GM8139_SOC_PKG_ID_BGA256) {
                    /* check pmu bit lock or not? */
                    bit_mask = 0x003fffc0;
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x54, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x54, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x54 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP#0 as input */
                    val = 0x00155540;
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x54, val, bit_mask);
                    if(ret < 0)
                        goto err;

                    /* check pmu bit lock or not? */
                    bit_mask = 0x000fc000;
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x50 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP_HS/X_CAP_VS as input */
                    val = 0x00054000;
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x50, val, bit_mask);
                    if(ret < 0)
                        goto err;
                }
                else {
                    /* check pmu bit lock or not? */
                    bit_mask = 0x003ffff0;
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x54, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x54, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x54 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP#0 as input */
                    val = 0x00155550;
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x54, val, bit_mask);
                    if(ret < 0)
                        goto err;

                    /* check pmu bit lock or not? */
                    bit_mask = 0x000f0000;
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x50 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP_HS/X_CAP_VS as input */
                    val = 0x00050000;
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x50, val, bit_mask);
                    if(ret < 0)
                        goto err;
                }
            }
            else {
                vcap_err("X_CAP#%d not support BT601_8BIT pin mux mode\n", cap_id);
                ret = -1;
                goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_BT601_16BIT_IN:
            if(cap_id != VCAP_CASCADE_VI_NUM) {
                if((pkg_id != GM8139_SOC_PKG_ID_BGA256) || GM8139_SOC_VI_16BIT_USE_BAYER_CLK) {
                    /* check pmu bit lock or not? */
                    bit_mask = 0x003fffcf;
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x54, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x54, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x54 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP#0 as input */
                    val = 0x00155545;
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x54, val, bit_mask);
                    if(ret < 0)
                        goto err;

                    /* check pmu bit lock or not? */
                    bit_mask = 0xffffc000;
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x50 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP#1 as input */
                    val = 0x55554000;
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x50, val, bit_mask);
                    if(ret < 0)
                        goto err;
                }
                else {
                    /* check pmu bit lock or not? */
                    bit_mask = 0x003fffff;
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x54, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x54, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x54 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP#0 as input */
                    val = 0x00155555;
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x54, val, bit_mask);
                    if(ret < 0)
                        goto err;

                    /* check pmu bit lock or not? */
                    bit_mask = 0xffff0000;
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x50 bit lock failed!!\n");
                            goto err;
                        }
                    }

                    /* set X_CAP#1 as input */
                    val = 0x55550000;
                    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x50, val, bit_mask);
                    if(ret < 0)
                        goto err;
                }
            }
            else {
                vcap_err("X_CAP#%d not support BT601_16BIT pin mux mode\n", cap_id);
                ret = -1;
                goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_ISP:
            /* the isp driver will touch all related pinmux */
            if(cap_id != VCAP_CASCADE_VI_NUM) {
                vcap_err("X_CAP#%d not support ISP pin mux mode\n", cap_id);
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
    return 0;
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
        ret = ftpmu010_write_reg(vcap_pmu_fd, 0xb0, 0, BIT17);
    else
        ret = ftpmu010_write_reg(vcap_pmu_fd, 0xb0, BIT17, BIT17);

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
        case VCAP_PLAT_VI_CAP_SRC_XCAP0:   ///< x_cap#0 can only pinmux as vi#0 source
            if(cap_id != 0) {
                ret = -1;
                goto exit;
            }

            switch(cfg->cap_fmt) {
                case VCAP_PLAT_VI_CAP_FMT_BT656:
                    ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_BT656_IN);
                    break;
                case VCAP_PLAT_VI_CAP_FMT_BT1120:
                    ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_BT1120_IN);
                    break;
                case VCAP_PLAT_VI_CAP_FMT_BT601_8BIT:
                    ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_BT601_8BIT_IN);
                    break;
                case VCAP_PLAT_VI_CAP_FMT_BT601_16BIT:
                    ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_BT601_16BIT_IN);
                    break;
                default:
                    ret = -1;
                    break;
            }
            if(ret < 0)
                goto exit;
            break;
        case VCAP_PLAT_VI_CAP_SRC_ISP:
            if((cap_id != VCAP_CASCADE_VI_NUM) || (cfg->cap_fmt != VCAP_PLAT_VI_CAP_FMT_ISP)) {
                ret = - 1;
                goto exit;
            }

            ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_ISP);
            if(ret < 0)
                goto exit;
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
