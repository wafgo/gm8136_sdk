/**
 * @file plat_8136.c
 *  vcap300 platform releated api for GM8136
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.1 $
 * $Date: 2014/08/27 02:02:03 $
 *
 * ChangeLog:
 *  $Log: plat_8136.c,v $
 *  Revision 1.1  2014/08/27 02:02:03  jerson_l
 *  1. support GM8136 platform VCAP300 feature
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

/********************************************************************************************************
 GM8136 PIN MUX Configuration Table
 --------------------------------------------------------------------------------------------------------
|             | Bayer 12 | Bayer 10 | Bayer 8 | BT656 + BT656 | BT1120 | BT601 16 | MIPI | MIPI + BT656 |
 --------------------------------------------------------------------------------------------------------
| X_Bayer_CLK |   CLK    |   CLK    |   CLK   |   D656_1_CLK  |   CLK  |    CLK   |      |  D656_0_CLK  |
| X_Bayer_HS  |   HS     |   HS     |   HS    |   D656_0_CLK  |        |    HS    | DN0  |  DN0         |
| X_Bayer_VS  |   VS     |   VS     |   VS    |               |        |    VS    | DP0  |  DP0         |
| X_Bayer_D11 |   D11    |   D9     |   D7    |   D656_1_D7   |   Y7   |    Y7    | CKN  |  CKN         |
| X_Bayer_D10 |   D10    |   D8     |   D6    |   D656_1_D6   |   Y6   |    Y6    | CKP  |  CKP         |
| X_Bayer_D9  |   D9     |   D7     |   D5    |   D656_1_D5   |   Y5   |    Y5    | DN1  |  DN1         |
| X_Bayer_D8  |   D8     |   D6     |   D4    |   D656_1_D4   |   Y4   |    Y4    | DP1  |  DP1         |
| X_Bayer_D7  |   D7     |   D5     |   D3    |   D656_1_D3   |   Y3   |    Y3    |      |              |
| X_Bayer_D6  |   D6     |   D4     |   D2    |   D656_1_D2   |   Y2   |    Y2    |      |              |
| X_Bayer_D5  |   D5     |   D3     |   D1    |   D656_1_D1   |   Y1   |    Y1    |      |              |
| X_Bayer_D4  |   D4     |   D2     |   D0    |   D656_1_D0   |   Y0   |    Y0    |      |              |
=========================================================================================================
| X_CAP0_D7   |   D3     |   D1     |         |   D656_0_D7   |   C7   |    C7    |      |  D656_0_D7   |
| X_CAP0_D6   |   D2     |   D0     |         |   D656_0_D6   |   C6   |    C6    |      |  D656_0_D6   |
| X_CAP0_D5   |   D1     |          |         |   D656_0_D5   |   C5   |    C5    |      |  D656_0_D5   |
| X_CAP0_D4   |   D0     |          |         |   D656_0_D4   |   C4   |    C4    |      |  D656_0_D4   |
| X_CAP0_D3   |          |          |         |   D656_0_D3   |   C3   |    C3    |      |  D656_0_D3   |
| X_CAP0_D2   |          |          |         |   D656_0_D2   |   C2   |    C2    |      |  D656_0_D2   |
| X_CAP0_D1   |          |          |         |   D656_0_D1   |   C1   |    C1    |      |  D656_0_D1   |
| X_CAP0_D0   |          |          |         |   D656_0_D0   |   C0   |    C0    |      |  D656_0_D0   |
---------------------------------------------------------------------------------------------------------
********************************************************************************************************/

/*
 * GM8136 PMU registers for capture
 */
static pmuReg_t vcap_pmu_reg_8136[] = {
    /*
     * Multi-Function Port Setting Register 0 [offset=0x50]
     * -----------------------------------------------------------------------------------
     * [15:14]bayer_clk  ==> 0: GPIO_0[9]   1: CAP0_CLK  2: BAYER_CLK/CAP1_CLK
     * [17:16]bayer_hs   ==> 0: CAP0_CLK    1: CAP_HS    2: BAYER_HS            3: DN0
     * [19:18]bayer_vs   ==> 0: None        1: CAP_VS    2: BAYER_VS            3: DP0
     * [21:20]bayer_d11  ==> 0: None        1: CAP0_D15  2: BAYER_D11/CAP1_D7   3: CKN
     * [23:22]bayer_d10  ==> 0: None        1: CAP0_D14  2: BAYER_D10/CAP1_D6   3: CKP
     * [25:24]bayer_d9   ==> 0: GPIO_0[7]   1: CAP0_D13  2: BAYER_D9 /CAP1_D5   3: DN1
     * [27:26]bayer_d8   ==> 0: GPIO_0[8]   1: CAP0_D12  2: BAYER_D8 /CAP1_D4   3: DP1
     * [29:28]bayer_d7   ==> 0: GPIO_0[20]  1: CAP0_D11  2: BAYER_D7 /CAP1_D3   3: SD1_CD
     * [31:30]bayer_d6   ==> 0: GPIO_0[21]  1: CAP0_D10  2: BAYER_D6 /CAP1_D2   3: SD1_D1
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
     * -----------------------------------------------------------------------------------
     * [1:0]  bayer_d5 ==> 0: GPIO_0[22]  1: CAP0_D9  2: BAYER_D5/CAP1_D1  3: SD1_D0
     * [3:2]  bayer_d4 ==> 0: GPIO_0[23]  1: CAP0_D8  2: BAYER_D4/CAP1_D0  3: SD1_CLK
     * [7:6]  cap0_d7  ==> 0: GPIO_0[10]  1: CAP0_D7  2: Bayer_D3          3: SD1_CMD_RSP
     * [9:8]  cap0_d6  ==> 0: GPIO_0[11]  1: CAP0_D6  2: Bayer_D2          3: SD1_D3
     * [11:10]cap0_d5  ==> 0: GPIO_0[12]  1: CAP0_D5  2: Bayer_D1          3: SD1_D2
     * [13:12]cap0_d4  ==> 0: GPIO_0[13]  1: CAP0_D4  2: Bayer_D0
     * [15:14]cap0_d3  ==> 0: GPIO_0[14]  1: CAP0_D3  2: I2C_SCL           3: SSP1_FS
     * [17:16]cap0_d2  ==> 0: GPIO_0[15]  1: CAP0_D2  2: I2C_SDA           3: SSP1_RXD
     * [19:18]cap0_d1  ==> 0: GPIO_0[16]  1: CAP0_D1  2: None              3: SSP1_TXD
     * [21:20]cap0_d0  ==> 0: GPIO_0[17]  1: CAP0_D0  2: None              3: SSP1_SCLK
     */
    {
     .reg_off   = 0x54,
     .bits_mask = 0x003fffcf,
     .lock_bits = 0x00000000,   ///< default don't lock these bit, ISP module may use these pin mux
     .init_val  = 0,
     .init_mask = 0,            ///< don't init
    },

    /*
     * System Control Register [offset=0x7c]
     * --------------------------------------
     * [10:8] cap0_data_swap ==> 1: cap0 low_8 bit swap, 2: cap0 high_8 bit swap, 4: cap0 high/low byte swap
     * [12]   cap0_clk_inv   ==> 0: disable 1: enable
     * [19]   cap1_data_swap ==> 0: none 1: bit swap
     * [22]   Sensor mode    ==> 0: serial  1: parallel
     * [23]   isp_cap1_clk   ==> 0: isp     1: cap1
     * [24]   bayer_clk_inv  ==> 0: disable 1: enable
     */
    {
     .reg_off   = 0x7c,
     .bits_mask = 0x01C81700,
     .lock_bits = 0x00081700,   ///< default don't lock bit24/23/22, ISP module may use these pin mux
     .init_val  = 0,
     .init_mask = 0x00081700,
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
     * ----------------------------------------------------
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

static pmuRegInfo_t vcap_pmu_reg_info_8136 = {
    "VCAP_8136",
    ARRAY_SIZE(vcap_pmu_reg_8136),
    ATTR_TYPE_AXI,                      ///< capture clock source from AXI Clock
    &vcap_pmu_reg_8136[0]
};

int vcap_plat_pmu_init(void)
{
    int fd;

    fd = ftpmu010_register_reg(&vcap_pmu_reg_info_8136);
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

    if(cap_id == 0) {
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
    }
    else {
        bit_mask = BIT19;

        /* X_CAP#1 low_8 bit swap */
        if(bit_swap & BIT0)
            val |= BIT19;
    }

    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);

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

    if(cap_id == 0) {
        bit_mask = BIT12;

        if(clk_invert)
            val = BIT12;
    }
    else {
        bit_mask = BIT24;

        if(clk_invert)
            val = BIT24;
    }

    ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);

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
            if(cap_id == 0) {
                /* check pmu bit lock or not? */
                bit_mask = 0x00030000;      ///< Use X_CAP0_CLK as CAP0 clock source
                if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {
                    ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                    if(ret < 0) {
                        vcap_err("pmu reg#0x50 bit lock failed!!\n");
                        goto err;
                    }
                    val = 0x00000000;
                }
                else {
                    bit_mask = 0x0000c000;  ///< Use X_BAYER_CLK as CAP0 clock source
                    if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {
                        ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                        if(ret < 0) {
                            vcap_err("pmu reg#0x50 bit lock failed!!\n");
                            goto err;
                        }
                    }
                    val = 0x00004000;
                }
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x50, val, bit_mask);
                if(ret < 0)
                    goto err;

                /* check pmu bit lock or not? */
                bit_mask = 0x003fffc0;  ///< Use X_CAP0_D7 ~ X_CAP0_D0
                if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x54, bit_mask) < 0) {
                    ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x54, bit_mask);
                    if(ret < 0) {
                        vcap_err("pmu reg#0x54 bit lock failed!!\n");
                        goto err;
                    }
                }
                val = 0x00155540;
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x54, val, bit_mask);
                if(ret < 0)
                    goto err;
            }
            else {
                /* check pmu bit lock or not? */
                bit_mask = 0xfff0c000;  ///< Use X_BAYER_D11 ~ X_BAYER_D6 as CAP1 data, X_BAYER_CLK as CAP1 clock source
                if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {
                    ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                    if(ret < 0) {
                        vcap_err("pmu reg#0x50 bit lock failed!!\n");
                        goto err;
                    }
                }
                val = 0xaaa08000;
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x50, val, bit_mask);
                if(ret < 0)
                    goto err;

                /* check pmu bit lock or not? */
                bit_mask = 0x0000000f;  ///< Use X_BAYER_D5 ~ X_BAYER_D4 as CAP1 data
                if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x54, bit_mask) < 0) {
                    ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x54, bit_mask);
                    if(ret < 0) {
                        vcap_err("pmu reg#0x54 bit lock failed!!\n");
                        goto err;
                    }
                }
                val = 0x0000000a;
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x54, val, bit_mask);
                if(ret < 0)
                    goto err;

                /* check pmu bit lock or not? */
                bit_mask = 0x00c00000;
                if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x7c, bit_mask) < 0) {
                    ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x7c, bit_mask);
                    if(ret < 0) {
                        vcap_err("pmu reg#0x7c bit lock failed!!\n");
                        goto err;
                    }
                }
                val = 0x00c00000;
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x7c, val, bit_mask);
                if(ret < 0)
                    goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_BT1120_IN:
            if(cap_id != VCAP_CASCADE_VI_NUM) {
                /* check pmu bit lock or not? */
                bit_mask = 0xfff0c000;  ///< Use X_BAYER_D11 ~ X_BAYER_D6 as CAP0 data, X_BAYER_CLK as CAP0 clock source
                if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {
                    ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                    if(ret < 0) {
                        vcap_err("pmu reg#0x50 bit lock failed!!\n");
                        goto err;
                    }
                }
                val = 0x55504000;
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x50, val, bit_mask);
                if(ret < 0)
                    goto err;

                /* check pmu bit lock or not? */
                bit_mask = 0x003fffcf;  ///< Use X_BAYER_D5, X_BAYER_D4, X_CAP0_D7 ~ X_CAP0_D0 as CAP0 data
                if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x54, bit_mask) < 0) {
                    ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x54, bit_mask);
                    if(ret < 0) {
                        vcap_err("pmu reg#0x54 bit lock failed!!\n");
                        goto err;
                    }
                }
                val = 0x00155545;
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x54, val, bit_mask);
                if(ret < 0)
                    goto err;
            }
            else {
                vcap_err("X_CAP#%d not support BT1120 pin mux mode\n", cap_id);
                ret = -1;
                goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_BT601_8BIT_IN:
            if(cap_id != VCAP_CASCADE_VI_NUM) {
                /* check pmu bit lock or not? */
                bit_mask = 0x000fc000;  ///< Use X_BAYER_CLK as CAP0 clock source, X_BAYER_HS as CAP0 HSync, X_BAYER_VS as CAP0 VSync
                if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {
                    ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                    if(ret < 0) {
                        vcap_err("pmu reg#0x50 bit lock failed!!\n");
                        goto err;
                    }
                }
                val = 0x00054000;
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x50, val, bit_mask);
                if(ret < 0)
                    goto err;

                /* check pmu bit lock or not? */
                bit_mask = 0x003fffc0;  ///< Use X_CAP0_D7 ~ X_CAP0_D0
                if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x54, bit_mask) < 0) {
                    ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x54, bit_mask);
                    if(ret < 0) {
                        vcap_err("pmu reg#0x54 bit lock failed!!\n");
                        goto err;
                    }
                }
                val = 0x00155540;
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x54, val, bit_mask);
                if(ret < 0)
                    goto err;
            }
            else {
                vcap_err("X_CAP#%d not support BT601_8BIT pin mux mode\n", cap_id);
                ret = -1;
                goto err;
            }
            break;
        case VCAP_PLAT_CAP_PINMUX_BT601_16BIT_IN:
            if(cap_id != VCAP_CASCADE_VI_NUM) {
                /* check pmu bit lock or not? */
                bit_mask = 0xffffc000;  ///< Use X_BAYER_D11 ~ X_BAYER_D6 as CAP0 data, X_BAYER_CLK as CAP0 clock source, X_BAYER_HS as CAP0 HSync, X_BAYER_VS as CAP0 VSync
                if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x50, bit_mask) < 0) {
                    ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x50, bit_mask);
                    if(ret < 0) {
                        vcap_err("pmu reg#0x50 bit lock failed!!\n");
                        goto err;
                    }
                }
                val = 0x55554000;
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x50, val, bit_mask);
                if(ret < 0)
                    goto err;

                /* check pmu bit lock or not? */
                bit_mask = 0x003fffcf;  ///< Use X_BAYER_D5, X_BAYER_D4, X_CAP0_D7 ~ X_CAP0_D0 as CAP0 data
                if(ftpmu010_bits_is_locked(vcap_pmu_fd, 0x54, bit_mask) < 0) {
                    ret = ftpmu010_add_lockbits(vcap_pmu_fd, 0x54, bit_mask);
                    if(ret < 0) {
                        vcap_err("pmu reg#0x54 bit lock failed!!\n");
                        goto err;
                    }
                }
                val = 0x00155545;
                ret = ftpmu010_write_reg(vcap_pmu_fd, 0x54, val, bit_mask);
                if(ret < 0)
                    goto err;
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
        case VCAP_PLAT_VI_CAP_SRC_XCAP0:
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
        case VCAP_PLAT_VI_CAP_SRC_XCAP1:
            if((cap_id != VCAP_CASCADE_VI_NUM) || (cfg->cap_fmt != VCAP_PLAT_CAP_PINMUX_BT656_IN)) {
                ret = -1;
                goto exit;
            }

            ret = vcap_plat_cap_pin_mux(cap_id, VCAP_PLAT_CAP_PINMUX_BT656_IN);
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
