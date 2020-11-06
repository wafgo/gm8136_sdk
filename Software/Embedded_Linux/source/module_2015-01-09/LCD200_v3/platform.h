#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include "lcd_def.h"

/*
 * Define LCD output target to VGA DAC, ....
 */
typedef enum
{
    OUT_TARGET_DISABLE = 1,
    OUT_TARGET_CVBS,        /* analog pad */
    OUT_TARGET_VGA,         /* analog(inside) */
    OUT_TARGET_LCD24,       /* digital pad */
    OUT_TARGET_BT1120       /* digital pad */
} OUT_TARGET_T;


/*
 * Function Declarations
 */
int platform_lock_ahbbus(int lcd_idx, int on);
int platform_pmu_init(void);
int platform_pmu_exit(void);
u32 platform_get_compress_ipbase(int lcd_idx);
u32 platform_get_decompress_ipbase(int lcd_idx);
int platform_set_dac_onoff(int lcd_idx, int status);
int platform_get_dac_onoff(int lcd_idx, int *status);

/* Macro
 */
struct callback_s
{
    int   index;
    char  name[20];
    int   (*init_fn)(void);
    void  (*exit_fn)(void);
    u32   sys_clk_base;
    u32   max_scaler_clk;
    void  (*switch_pinmux)(void *info, int on);
    u32   (*pmu_get_clk)(int lcd200_idx, u_long pixclk, unsigned int b_Use_Ext_Clk);
    void  (*pmu_clock_gate)(int lcd_idx, int on_off, int use_ext_clk);
    int   (*tve_config)(int lcd_idx, int mode, int input);
    int   (*check_input_res)(int lcd_idx, VIN_RES_T resolution);
    void  *private_data;
};

extern struct callback_s *platform_cb;

#define platform_pmu_switch_pinmux  platform_cb->switch_pinmux
#define platform_pmu_get_clk        platform_cb->pmu_get_clk
#define platform_pmu_clock          platform_cb->pmu_clock_gate
#define tve100_config               platform_cb->tve_config
#define platform_check_input_res    platform_cb->check_input_res

void platform_set_lcd_vbase(int idx, unsigned int va);
void platform_set_lcd_pbase(int idx, unsigned int pa);
void platform_get_lcd_pbase(int idx, unsigned int *pa);

int platform_get_osdline(void);

#endif /* __PLATFORM_H__ */