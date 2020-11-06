#ifndef _VCAP_PLAT_H_
#define _VCAP_PLAT_H_

/*************************************************************************************
 *  VCAP Platform Definition
 *************************************************************************************/
typedef enum {
    VCAP_PLAT_CAP_PINMUX_BT656_IN = 0,          ///< source from X_CAP#n, bt656(8bit + pclk) input
    VCAP_PLAT_CAP_PINMUX_BT1120_IN,             ///< source from X_CAP#n+X_CAP#n+1, bt1120(16bit + pclk) input
    VCAP_PLAT_CAP_PINMUX_CASCADE_IN,            ///< source from X_CAPCAS, only for CAPCAS and CAP#7
    VCAP_PLAT_CAP_PINMUX_LCD0_RGB_IN,           ///< source from internal LCD0_RGB(24bit + de/pclk/vs/hs), only for CAPCAS
    VCAP_PLAT_CAP_PINMUX_LCD1_RGB_IN,           ///< source from internal LCD1_RGB(24bit + de/pclk/vs/hs), only for CAPCAS
    VCAP_PLAT_CAP_PINMUX_LCD2_RGB_IN,           ///< source from internal LCD2_RGB(24bit + de/pclk/vs/hs), only for CAPCAS
    VCAP_PLAT_CAP_PINMUX_LCD0_BT656_1120_IN,    ///< source from internal LCD0_BT656/1120(8/16bit + pclk), only for CAPCAS
    VCAP_PLAT_CAP_PINMUX_LCD1_BT656_1120_IN,    ///< source from internal LCD1_BT656/1120(8/16bit + pclk), only for CAPCAS
    VCAP_PLAT_CAP_PINMUX_LCD2_BT656_1120_IN,    ///< source from internal LCD2_BT656/1120(8/16bit + pclk), only for CAPCAS
    VCAP_PLAT_CAP_PINMUX_BT601_8BIT_IN,         ///< source from X_CAP#n, bt601(8bit + h_sync + v_sync + pclk) input
    VCAP_PLAT_CAP_PINMUX_BT601_16BIT_IN,        ///< source from X_CAP#n, bt601(16bit + h_sync + v_sync + pclk) input
    VCAP_PLAT_CAP_PINMUX_ISP,                   ///< source from internal isp
    VCAP_PLAT_CAP_PINMUX_MAX
} VCAP_PLAT_CAP_PINMUX_T;

typedef enum {
    VCAP_PLAT_VI_CAP_SRC_XCAP0 = 0,
    VCAP_PLAT_VI_CAP_SRC_XCAP1,
    VCAP_PLAT_VI_CAP_SRC_XCAP2,
    VCAP_PLAT_VI_CAP_SRC_XCAP3,
    VCAP_PLAT_VI_CAP_SRC_XCAP4,
    VCAP_PLAT_VI_CAP_SRC_XCAP5,
    VCAP_PLAT_VI_CAP_SRC_XCAP6,
    VCAP_PLAT_VI_CAP_SRC_XCAP7,
    VCAP_PLAT_VI_CAP_SRC_XCAPCAS,
    VCAP_PLAT_VI_CAP_SRC_LCD0,
    VCAP_PLAT_VI_CAP_SRC_LCD1,
    VCAP_PLAT_VI_CAP_SRC_LCD2,
    VCAP_PLAT_VI_CAP_SRC_ISP,
    VCAP_PLAT_VI_CAP_SRC_MAX
} VCAP_PLAT_VI_CAP_SRC_T;

typedef enum {
    VCAP_PLAT_VI_CAP_FMT_BT656,
    VCAP_PLAT_VI_CAP_FMT_BT1120,
    VCAP_PLAT_VI_CAP_FMT_RGB888,
    VCAP_PLAT_VI_CAP_FMT_BT601_8BIT,
    VCAP_PLAT_VI_CAP_FMT_BT601_16BIT,
    VCAP_PLAT_VI_CAP_FMT_ISP,
    VCAP_PLAT_VI_CAP_FMT_MAX
} VCAP_PLAT_VI_CAP_FMT_T;

struct vcap_plat_cap_port_cfg_t {
    VCAP_PLAT_VI_CAP_SRC_T  cap_src;
    VCAP_PLAT_VI_CAP_FMT_T  cap_fmt;
    u8                      inv_clk;
    u8                      bit_swap;
    u8                      byte_swap;
};

/*************************************************************************************
 *  Public Function Prototype
 *************************************************************************************/
void vcap_plat_pmu_exit(void);
int  vcap_plat_pmu_init(void);
u32  vcap_plat_chip_version(void);
int  vcap_plat_cap_schmitt_trigger_onoff(int on);
int  vcap_plat_cap_sw_reset_onoff(int on);
int  vcap_plat_cap_axiclk_onoff(int on);
int  vcap_plat_cap_data_swap(int cap_id, int bit_swap, int byte_swap);
int  vcap_plat_cap_data_duplicate(int cap_id, int dup_on);
int  vcap_plat_cap_clk_invert(int cap_id, int clk_invert);
int  vcap_plat_cap_pin_mux(int cap_id, VCAP_PLAT_CAP_PINMUX_T cap_pin);
int  vcap_plat_vi_capture_port_config(int vi, struct vcap_plat_cap_port_cfg_t *cfg);

#endif  /* _VCAP_PLAT_H_ */