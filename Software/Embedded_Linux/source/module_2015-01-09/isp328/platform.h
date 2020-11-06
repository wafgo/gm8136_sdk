/**
 * @file platform.h
 * Platform layer header file
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

//=============================================================================
// struct & flag definition
//=============================================================================
enum BAYER_SRC {
    SRC_MIPI = 0,
    SRC_HISPI,
    SRC_LVDS,
    SRC_PARALLEL,
};

// MIPI clock source
#define CLK_DPHY_BYTECLK    0
#define CLK_EXT_CLK         1

//=============================================================================
// external functions
//=============================================================================
extern unsigned int isp_dev_get_platf_ver(void);

extern int  isp_plat_pmu_init(void);
extern void isp_plat_pmu_exit(void);

extern int  isp_plat_init_pinmux(int mode);
extern int  isp_plat_extclk_set_freq(u32 freq);
extern int  isp_plat_extclk_onoff(int on);
extern int  isp_plat_dphy_clk_init(void);
extern int  isp_plat_dphy_clk_onoff(int on);

extern int  isp_plat_ispclk_onoff(int on);
extern int  isp_plat_ispclk_set_pwm(int period);
extern int  isp_plat_adjust_ispclk(u32 pclk);

extern int  isp_plat_isp_sw_reset_onoff(int on);
extern int  isp_plat_dphy_sw_reset_onoff(int on);
extern int  isp_plat_mipi_sw_reset_onoff(int on);
extern int  isp_plat_hispi_sw_reset_onoff(int on);
extern int  isp_plat_lvds_sw_reset_onoff(int on);

extern int  isp_plat_isp_aximclk_onoff(int on);
extern int  isp_plat_isp_ahbmclk_onoff(int on);
extern int  isp_plat_mipi_apbmclk_onoff(int on);
extern int  isp_plat_hispi_apbmclk_onoff(int on);
extern int  isp_plat_lvds_apbmclk_onoff(int on);

extern int  isp_plat_isp_pixclk_onoff(int on);
extern int  isp_plat_mipi_pixclk_onoff(int on);
extern int  isp_plat_hispi_pixclk_onoff(int on);
extern int  isp_plat_lvds_pixclk_onoff(int on);

extern int  isp_plat_dphy_csi_clk_select(int clk_src);
extern int  isp_plat_bayer_clk_invert(int clk_invert);
extern int  isp_plat_sensor_vs_invert(int clk_invert);
extern int  isp_plat_sensor_hs_invert(int clk_invert);
extern int  isp_plat_bayer_data_swap(int swap);
extern int  isp_plat_select_bayer_source(enum BAYER_SRC bayer_src);
extern int  isp_plat_bayer_14b_in_onoff(int on);

//=============================================================================
// sensor related functions
//=============================================================================
extern int  isp_plat_cap_rst_init(void);
extern void isp_plat_cap_rst_exit(void);
extern void isp_plat_cap_rst_set_value(int value);

extern int  isp_plat_spi_init(void);
extern void isp_plat_spi_exit(void);
extern void isp_plat_spi_clk_set_value(int value);
extern int  isp_plat_spi_di_get_value(void);
extern void isp_plat_spi_do_set_value(int value);
extern void isp_plat_spi_cs_set_value(int value);

#endif /* __PLATFORM_H__ */
