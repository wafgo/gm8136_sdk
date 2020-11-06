#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/platform/board.h>
#include <mach/platform/platform_io.h>
#include <linux/delay.h>
#include "ffb.h"
#include "proc.h"
#include "debug.h"
#include "dev.h"
#include "codec.h"
#include "lcd_def.h"
#include "platform.h"
#include <mach/ftpmu010.h>
#include <mach/fmem.h>

#define ALL_USE_PLL3

/* index
 */
#define GM8287      0
#define DAC_ALWAYS_ON
#define SYS_CLK         (platform_cb->sys_clk_base)
#define MAX_SCALAR_CLK  (platform_cb->max_scaler_clk)
#define HZ_TO_MHZ(x)    ((x) / 1000000)

static int lcd_fd = 0, lcd_srcsel = 0;
static volatile unsigned int TVE100_BASE = 0;
static volatile unsigned int SAR_ADC1_BASE = 0;
static u32 lcd_va_base[LCD_MAX_ID], lcd_pa_base[LCD_MAX_ID];
static u32 dac_onoff[LCD_IP_NUM] = {LCD_SCREEN_ON, LCD_SCREEN_ON, LCD_SCREEN_ON}; /* 1 for ON, 0 for OFF */
static struct proc_dir_entry *lcd_srcsel_proc = NULL;  /* analog proc */
static struct proc_dir_entry *cvbs_loading_proc = NULL;  /* cvbs loading proc */
static struct task_struct *cvbs_thread = NULL;
static volatile int cvbs_thread_runing = 0, tv_dac_on = 0;

/* Define the input resolution supporting list. Please fill the input resoluiton here.
 * If the resultion is not defined here, they will not be suppported!
 * Note: the array must be terminated with VIN_NONE.
 */
static VIN_RES_T res_support_list[LCD_IP_NUM][30] = {
    /* LCD0 */
    {VIN_NTSC, VIN_PAL, VIN_640x480, VIN_800x600, VIN_1024x768, VIN_1280x800, VIN_1280x960, VIN_720P,
    VIN_1280x1024, VIN_1680x1050, VIN_1440x960, VIN_1920x1080, VIN_1280x1080, VIN_1920x1152, VIN_NONE},
    /* LCD1 */
    {VIN_NONE},
    /* LCD2 */
    {VIN_NTSC, VIN_PAL, VIN_NONE},
};


/* If the clock not listed here, use default value */
struct lcd_mn {
    int M;
    u32 pixel_clk;
} lcd_mn_pixel[] = {
    {-1,  27000000},    //D1
    {108, 65000000},    //1024x768
    {108, 108000000},   //1280x1024
    {99, 74250000},     //720P
    {99, 148500000},    //1080P, and it is default value as well, placed in the end of the array.
};

static u32  compress_pa_ipbase[LCD_IP_NUM] = {GRAPHIC_ENC_PA_BASE, 0, GRAPHIC_ENC_PA_BASE};
static u32  decompress_pa_ipbase[LCD_IP_NUM] = {GRAPHIC_DEC_0_PA_BASE, 0, GRAPHIC_DEC_2_PA_BASE};

/*
 * GM8287 PMU registers
 */
/* offset, bits_mask, lock_bits, init_val, init_mask */
static pmuReg_t pmu_reg_8287[] = {
    /* panel0pixclk_sel choose PLL3, panel1pixclk_sel choose pll1out1_div1, LCD2 chooses pll1out1_div1.
     * [13:12] decides the clock tree of LCDC2.
     */
#ifdef ALL_USE_PLL3
    {0x28, 0xBF << 8, 0xBF << 8, (0x0 << 15) | (0x0 << 12) | (0x8 << 8), 0xBF << 8},
#else
    {0x28, 0xBF << 8, 0xBF << 8, (0x0 << 15) | (0x0 << 12) | (0x8 << 8), 0xBF << 8},
#endif
    /* [10:4] : pll3ns (CKOUT = FREF*NS/2), default value is 0x63 */
    {0x34, (0x7F << 4) | 0x1, (0x7F << 4), 0x0, 0x0},

    /* lcd([27]-slew rate, 1-slow, [26:25] 00-4mA, 01-8-mA, 10-12mA, 11-16mA, [24] reserved) */
    {0x40, 0xF << 24, 0xF << 24, 0x2 << 25, 0xF << 24},

    /* BT1120, [5:6]: CAP2/GPIO/LCD, [7:8]: CAP3/LCD
     *      [10:13]: GMAC Pinmux
     */
    {0x50, (0xF << 10) | (0xF << 5), 0x0, 0x0, 0x0},
    /* SAR-ADC1 bit 5:0 pvalue */
    {0x60, 0x3F, 0x3F, 00, 00},
    /* lcd2pixclk_pvalue (lcd2pixclk = after reg28[13:12] setection/([29:24]+1))
     * select adc1_wrapper clock source 12Mhz
     */
    {0x74, (0x1 << 31) | (0x3F << 24), (0x1 << 31) | (0x3F << 24), 1 << 31, 0x1 << 31},
    /* lcd0/1 pixel pvalue and scaler pvalue*/
    {0x78, (0x1F << 24) | (0x3F << 12), (0x1F << 24) | (0x3F << 12), 0, 0},

    /* LCD source selection [25:21]
     * VCAP300 lcdc_source_sel, x0000 - lcdc0 lcdout, x001x - lcdc2 lcdout, x0100 - lcdc0 1120/656out, x011x - lcdc2 1120/656out
     * default: x0000 - lcdc0 lcdout.
     * bit25: clock inverse. hide from datasheet.
     */
    {0x7C, (0x1F << 21), (0x1F << 21), 0x0, (0x1F << 21)},
    /* IPs reset control by register : active low */
    {0xA0, (0x1 << 23), (0x1 << 23), (0x1 << 23), (0x1 << 23)},
    /* aximclkoff : 1-gating clock for lcd/0/1/2 and graphic encoder */
    {0xB0, (0x1 << 23) | (0x5 << 16), (0x1 << 23) | (0x5 << 16), (0x0 << 23) | (0x0 << 16), (0x1 << 23) | (0x5 << 16)},
    /* TVE hclk */
    {0xB4, 0x1 << 31, 0x1 << 31, 0x0 << 31, 0x1 << 31},
    /* apbmclkoff0 for lcd0/2, graphic decoder 0/2 */
    {0xB8, (0x2D << 26), (0x2D << 26), (0x0 << 26), (0x2D << 26)},
    /* rlc encoder */
    {0xBC, (0x1 << 5), (0x1 << 5), (0x0 << 5), (0x1 << 5)},
    /* set CVBS power-down by default */
    {0xE0, 0xF << 20 | 0x1 << 16 | 0xF << 5 | 0x1, 0xF << 20 | 0x1 << 16 | 0xF << 5 | 0x1, 0x7 << 20 | 0x1 << 16 | 0x7 << 5, 0xF << 20 | 0x1 << 16 | 0xF << 5}

};

static pmuRegInfo_t pmu_reg_info_8287 = {
    "LCD_8287",
    ARRAY_SIZE(pmu_reg_8287),
    ATTR_TYPE_NONE, //will decide later
    &pmu_reg_8287[0]
};


/*
 * Local function declaration
 */
void platform_pmu_switch_pinmux_gm8287(void *ptr, int on);
unsigned int platform_pmu_calculate_clk(int lcd200_idx, u_long pixclk, unsigned int b_Use_Ext_Clk);
void platform_pmu_clock_gate(int lcd_idx, int on_off, int use_ext_clk);
int platform_tve100_config(int lcd_idx, int mode, int input);
int platform_check_input_resolution(int lcd_idx, VIN_RES_T resolution);
int platfrom_8287_init(void);
void platfrom_8287_exit(void);
static int cvbs_polling_thread(void *private);
/*
 * Local variable declaration
 */

#define DIFF(x,y)   ((x) > (y) ? (x) - (y) : (y) - (x))


/* pmu callback functions
 */
static struct callback_s callback[] = {
    {
         .index = GM8287,
         .name = "GM8287",
         .init_fn = platfrom_8287_init,
         .exit_fn = platfrom_8287_exit,
         .sys_clk_base = 12000000,  //12M
         .max_scaler_clk = (200 * 1000000UL),       //130M
         .switch_pinmux = platform_pmu_switch_pinmux_gm8287,
         .pmu_get_clk = platform_pmu_calculate_clk,
         .pmu_clock_gate = platform_pmu_clock_gate,
         .tve_config = platform_tve100_config,
         .check_input_res = platform_check_input_resolution,
     }
};

/* callback funtion structure
 */
struct callback_s *platform_cb = NULL;

void platform_pmu_switch_pinmux_gm8287(void *ptr, int on)
{
    return;
}

/* pixclk: Standard VESA clock (kHz)
 */
unsigned int platform_pmu_calculate_clk(int lcd200_idx, u_long pixclk, unsigned int b_Use_Ext_Clk)
{
    unsigned int div, pll, i = 0;
    unsigned int diff, tolerance;
    unsigned int scar_clk = platform_cb->max_scaler_clk;

    /* [28:24] : lcdscarclk_pvalue (lcdscarclk=pll3/([28:24]+1)) */
    div = (ftpmu010_clock_divisor2(ATTR_TYPE_PLL3, scar_clk, 1) - 1) & 0x1F;
    ftpmu010_write_reg(lcd_fd, 0x78, div << 24, 0x1F << 24);

    /* pixel clock:
     * 0x78[17-12] : panel0pixclk_pvalue
     * 0x74[29:24] : panel2pixclk_pvalue
     */
#ifdef ALL_USE_PLL3
    /* All LCD shares PLL3 */
    for (i = 0; i < ARRAY_SIZE(lcd_mn_pixel); i ++) {
        if (lcd_mn_pixel[i].pixel_clk == (pixclk * 1000))
            break;
    }
    if (i == ARRAY_SIZE(lcd_mn_pixel)) {
        printk("Warning!!!!!!!!!!! The pixel clock %d is not supported! \n", (u32)(pixclk * 1000));
        i = ARRAY_SIZE(lcd_mn_pixel) - 1;   //use the last one as the default value
    }

    /* update the M value */
    if (lcd_mn_pixel[i].M != -1) {
        ftpmu010_write_reg(lcd_fd, 0x34, (lcd_mn_pixel[i].M & 0x7F) << 4, 0x7F << 4);
        ftpmu010_reload_attr(ATTR_TYPE_PLL3);   //refresh PLL3
    }

    switch (lcd200_idx) {
      case LCD_ID:
        /* [17:12] : lcd0pixclk_pvalue (lcd0pixclk = after reg28[11:8] setection/([17:12]+1)) */
        div = (ftpmu010_clock_divisor2(ATTR_TYPE_PLL3, pixclk * 1000, 1) - 1) & 0x3F;
        ftpmu010_write_reg(lcd_fd, 0x78, div << 12, 0x3F << 12);
        pll = ftpmu010_get_attr(ATTR_TYPE_PLL3);
        break;

      case LCD2_ID:
        /* choose PLL3 */
        ftpmu010_write_reg(lcd_fd, 0x28, 0x3 << 12, 0x3 << 12);

        /* [29:24] : lcd2pixclk_pvalue (lcd2pixclk = after reg28[13:12] setection/([29:24]+1)) */
        div = (ftpmu010_clock_divisor2(ATTR_TYPE_PLL3, pixclk * 1000, 1) - 1) & 0x3F;
        ftpmu010_write_reg(lcd_fd, 0x74, div << 24, 0x3F << 24);
        pll = ftpmu010_get_attr(ATTR_TYPE_PLL3);
        break;

      default:
        panic("%s, worng lcd idx %d \n", __func__, lcd200_idx);
        break;
    }
#else
    switch (lcd200_idx) {
      case LCD_ID:
        for (i = 0; i < ARRAY_SIZE(lcd_mn_pixel); i ++) {
            if (lcd_mn_pixel[i].pixel_clk == (pixclk * 1000))
                break;
        }
        if (i == ARRAY_SIZE(lcd_mn_pixel))
            i = ARRAY_SIZE(lcd_mn_pixel) - 1;   //use the last one as the default value
        /* update the M value */
        ftpmu010_write_reg(lcd_fd, 0x34, (lcd_mn_pixel[i].M & 0x7F) << 4, 0x7F << 4);
        ftpmu010_reload_attr(ATTR_TYPE_PLL3);   //refresh PLL3

        /* [17:12] : lcd0pixclk_pvalue (lcd0pixclk = after reg28[11:8] setection/([17:12]+1)) */
        div = (ftpmu010_clock_divisor2(ATTR_TYPE_PLL3, pixclk * 1000, 1) - 1) & 0x3F;
        ftpmu010_write_reg(lcd_fd, 0x78, div << 12, 0x3F << 12);
        pll = ftpmu010_get_attr(ATTR_TYPE_PLL3);
        break;

      case LCD2_ID:
        /* [29:24] : lcd2pixclk_pvalue (lcd2pixclk = after reg28[13:12] setection/([29:24]+1)) */
        div = (ftpmu010_clock_divisor2(ATTR_TYPE_PLL1, pixclk * 1000, 1) - 1) & 0x3F;
        ftpmu010_write_reg(lcd_fd, 0x74, div << 24, 0x3F << 24);
        pll = ftpmu010_get_attr(ATTR_TYPE_PLL1);
        break;
      default:
        panic("%s, worng lcd idx %d \n", __func__, lcd200_idx);
        break;
    }
#endif /* ALL_USE_PLL3 */

    diff = DIFF(pixclk * 1000, pll / (div + 1));
    tolerance = diff / pixclk;  /* comment: (diff * 1000) / (pixclk * 1000); */
    if (tolerance > 5) {        /* if the tolerance is larger than 0.5%, dump error message! */
        while (i < 50) {
            printk("The standard clock is %d.%dMHZ, but gets %dMHZ. Please correct PLL!\n",
                   (u32) (pixclk / 1000), (u32) ((100 * (pixclk % 1000)) / 1000), pll / 1000000);
            i++;
        }
    }

    return pll / (div + 1);
}

/* this function will be called in initialization state or module removed */
void platform_pmu_clock_gate(int lcd_idx, int on_off, int use_ext_clk)
{
    u32 bit, mask, ofs = 0, apb_ofs = 0;

    if (use_ext_clk)    {}

    switch (lcd_idx) {
      case LCD_ID:
        ofs = 18;
        apb_ofs = 31;
        break;

      case LCD2_ID:
        ofs = 16;
        apb_ofs = 29;
        break;
      default:
        panic("%s, wrong lcd idx %d \n", __func__, lcd_idx);
        break;
    }

    if (on_off) {
        bit = (0x0 << ofs);
        mask = (0x1 << ofs);
        ftpmu010_write_reg(lcd_fd, 0xB0, bit, mask);   //turn on gate clock

        bit = (0x0 << apb_ofs);
        mask = (0x1 << apb_ofs);
        ftpmu010_write_reg(lcd_fd, 0xB8, bit, mask);   //turn on apb clock
    } else {
        bit = (0x1 << ofs);
        mask = (0x1 << ofs);
        ftpmu010_write_reg(lcd_fd, 0xB0, bit, mask);   //turn off gate clock

        bit = (0x1 << apb_ofs);
        mask = (0x1 << apb_ofs);
        ftpmu010_write_reg(lcd_fd, 0xB8, bit, mask);   //turn off apb clock
    }

    if ((on_off == 0) && (lcd_idx == LCD2_ID)) {
        tv_dac_on = 0;
    }

    return;
}

/*
 * mode: VOT_NTSC, VOT_PAL
 * input: 2 : 656in
 *        6 : colorbar output for test
 */
int platform_tve100_config(int lcd_idx, int mode, int input)
{
    u32 tmp;

    if (TVE100_BASE == 0) {
        TVE100_BASE = (unsigned int)ioremap_nocache(TVE_FTTVE100_PA_BASE, TVE_FTTVE100_PA_SIZE);
        if (!TVE100_BASE) {
            printk("LCD: No virtual memory! \n");
            return -1;
        }
    }

    /* only applies to LCD2_ID */
    if (lcd_idx != LCD2_ID)
        return 0;

    /* only support PAL & NTSC */
    if ((mode != VOT_NTSC) && (mode != VOT_PAL))
        return -EINVAL;

    if ((input != 2) && (input != 6))   /* color bar */
        return -EINVAL;

    //lcd disable case
    if (!(ioread32(lcd_va_base[lcd_idx]) & 0x1))
        return 0;

    /* 0x0 */
    tmp = (mode == VOT_NTSC) ? 0x0 : 0x2;
    iowrite32(tmp, TVE100_BASE + 0x0);

    /* 0x4 */
    iowrite32(input, TVE100_BASE + 0x4);

    /* 0x8, 0xc */
    iowrite32(0, TVE100_BASE + 0x8);

    /* bit 0: power down, bit 1: standby */
    iowrite32(0, TVE100_BASE + 0xc);

    tv_dac_on = 1;

    return 0;
}

/* Check the input resolution is supported or not
 * 0 for yes, -1 for No
 */
int platform_check_input_resolution(int lcd_idx, VIN_RES_T resolution)
{
    int i, bFound = 0;

    for (i = 0; (res_support_list[lcd_idx][i] != VIN_NONE); i++) {
        if (res_support_list[lcd_idx][i] == resolution) {
            bFound = 1;
            break;
        }
    }

    return (bFound == 1) ? 0 : -1;
}

/* 1 for lock ahb bus, 0 for release abhbus
 * return value: original value
 */
int platform_lock_ahbbus(int lcd_idx, int on)
{
    if (lcd_idx || on)  {}

    return 0;
}

/*
 * Platform init function. This platform dependent.
 */
int platform_pmu_init(void)
{
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;
    int i, type = GM8287;

    //compile check
    fmem_get_identifier(&pci_id, &cpu_id);

    for (i = 0; i < ARRAY_SIZE(callback); i++) {
        if (callback[i].index == type) {
            platform_cb = &callback[i];
            printk("\nLCD platform: Hook %s driver.\n", platform_cb->name);
        }
    }
    if (platform_cb == NULL) {
        panic("Error in platform_pmu_init(), can't find match table");
        return -1;
   }

    lcd_fd = ftpmu010_register_reg(&pmu_reg_info_8287);
    if (lcd_fd < 0)
        panic("lcd register pmu fail!");

    if (platform_cb->init_fn)
        platform_cb->init_fn();

    /* TV DAC */
    TVE100_BASE = (unsigned int)ioremap_nocache(TVE_FTTVE100_PA_BASE, TVE_FTTVE100_PA_SIZE);
    if (!TVE100_BASE) {
        printk("LCD: No virtual memory! \n");
        return -1;
    }

    cvbs_thread = kthread_create(cvbs_polling_thread, NULL, "dac thread");
    if (IS_ERR(cvbs_thread)){
        printk("LCD: Error in creating kernel thread! \n");
        return -1;
    }

    wake_up_process(cvbs_thread);


    return 0;
}

unsigned int Platform_Get_FB_AcceID(LCD_IDX_T lcd_idx , int fb_id)
{
    if(lcd_idx == LCD_ID && fb_id == 1)
        return 0x54736930;

    return FB_ACCEL_NONE;
}

int platform_set_dac_onoff(int lcd_idx, int status)
{
    if (lcd_idx < 0 || lcd_idx >= LCD_IP_NUM)
        return -1;

    if (dac_onoff[lcd_idx] == status)
        return 0;

    switch (lcd_idx) {
      case LCD_ID:
        if (status == LCD_SCREEN_ON)
            ftpmu010_write_reg(lcd_fd, 0xE0, 0, 0x1); /* on */
        else
            ftpmu010_write_reg(lcd_fd, 0xE0, 1, 0x1); /* off */
        break;
      case LCD2_ID:
        if (status == LCD_SCREEN_OFF)
            ftpmu010_write_reg(lcd_fd, 0xE0, 0x1 << 16, 0x1 << 16); /* off */
        break;
      default:
        return 0;
        break;
    }

    dac_onoff[lcd_idx] = status;
    return 0;
}
EXPORT_SYMBOL(platform_set_dac_onoff);

int platform_get_dac_onoff(int lcd_idx, int *status)
{
    if (lcd_idx < 0 || lcd_idx >= LCD_IP_NUM)
        return -1;

    *status = dac_onoff[lcd_idx];

    return 0;
}
EXPORT_SYMBOL(platform_get_dac_onoff);

/* Work Flow:
 * Set 0x9900_0074 bit[31:30] = 2'b00, select adcl_wrapper clock source 12Mhz
 * Set 0x9900_00E0 bit[16] = 1, set CVBS power-down
 * Set 0x9900_0000 bit[4:0], open auto_detect & ADC_XLSEL[0] = 1, SEL = 2'b11
 * while (1)    read 0x98f0_0008, read ADC_Data
 * If (ADC_Data > ADC_THDH)
 *      set 0x9900_00E0 bit[16] = 1'b1, set CVBS power-down ==> with LOAD
 * If (ADC_Data < ADC_THDL)
 *      SET 0x9900_00E0 bit[16] = 1'b0, disable CVBS power-down ==> without LOAD
 * Note:
 *  ADC_THDH, ADC_THDL value should be decided by reading the real value from ADC_Data with LOAD
 *          or without LOAD.
 */
static int cvbs_polling_thread(void *private)
{
    u32 value, div = 12000000;

    /* SAR-ADC */
    SAR_ADC1_BASE = (unsigned int)ioremap_nocache(ADC_WRAP_1_PA_BASE, ADC_WRAP_1_PA_SIZE);
    if (SAR_ADC1_BASE == 0)
        panic("%s, lcd ioremap fail! \n", __func__);

    cvbs_thread_runing = 1;

    /* do basic init */
    value = ioread32(SAR_ADC1_BASE);
    value &= ~((0xFF << 16) | 0x1F);
    value |= ((0xA << 16) | (0x7 << 3) | 0x01); /* bit5: ADC normal operating mode */
    iowrite32(value, SAR_ADC1_BASE);
    do_div(div, 760*1000);    //760K
    if (div * 760*1000 != 12000000)
        div += 1;   //can't over 760K
    ftpmu010_write_reg(lcd_fd, 0x60, div, 0x3F);

    do {
#ifdef DAC_ALWAYS_ON
        if (tv_dac_on) {
            ftpmu010_write_reg(lcd_fd, 0xE0, 0x0 << 16, 0x1 << 16); /* on */
        }
#else
        if (tv_dac_on) {
            u32 value, dac_off;

            if (dac_onoff[LCD2_ID] == LCD_SCREEN_OFF) {
                ftpmu010_write_reg(lcd_fd, 0xE0, 0x1 << 16, 0x1 << 16); /* off */
                msleep(1000);
                continue;
            }

            dac_off = (ftpmu010_read_reg(0xE0) >> 16) & 0x1;    /* 1 means CVBS power down */

            value = ioread32(SAR_ADC1_BASE + 0x8);
            if (dac_off) {
                /* dac off, 0 ~ 0x45 */
                if (value > 0x20) {
                    /* no loading */
                } else {
                    /* has loading */
                    ftpmu010_write_reg(lcd_fd, 0xE0, 0x0 << 16, 0x1 << 16);
                }
            } else {
                /* dac on, 0x40 ~ 0xC0 */
                if (value > 0x80) {
                    /* no loading */
                    ftpmu010_write_reg(lcd_fd, 0xE0, 0x1 << 16, 0x1 << 16); /* off */
                } else {
                    /* has loading */
                }
            }
        }
#endif
        msleep(1000);
    }while(!kthread_should_stop());

    cvbs_thread_runing = 0;

    return 0;
}


u32 platform_get_compress_ipbase(int lcd_idx)
{
    if (lcd_idx >= ARRAY_SIZE(compress_pa_ipbase))
        panic("%s, invalid index %d! \n", __func__, lcd_idx);

    return compress_pa_ipbase[lcd_idx];
}

u32 platform_get_decompress_ipbase(int lcd_idx)
{
    if (lcd_idx >= ARRAY_SIZE(decompress_pa_ipbase))
        panic("%s, invalid index %d! \n", __func__, lcd_idx);

    return decompress_pa_ipbase[lcd_idx];
}
/*
 * Platform exit function. This platform dependent.
 */
int platform_pmu_exit(void)
{
    if (platform_cb->exit_fn)
        platform_cb->exit_fn();

    __iounmap((void *)TVE100_BASE);
    __iounmap((void *)SAR_ADC1_BASE);

    ftpmu010_deregister_reg(lcd_fd);

    if (cvbs_thread)
        kthread_stop(cvbs_thread);

    while (cvbs_thread_runing == 1)
        msleep(1);

    return 0;
}

void platform_set_lcd_vbase(int idx, unsigned int va)
{
    if (idx < LCD_MAX_ID)
        lcd_va_base[idx] = va;
    else
        panic("LCD: too small array size(idx: %d)! \n", idx);
}

void platform_set_lcd_pbase(int idx, unsigned int pa)
{
    if (idx < LCD_MAX_ID)
        lcd_pa_base[idx] = pa;
    else
        panic("LCD: too small array size(idx: %d)! \n", idx);
}

void platform_get_lcd_pbase(int idx, unsigned int *pa)
{
    if (idx < LCD_MAX_ID)
        *pa = lcd_pa_base[idx];
    else
        panic("LCD: too small array size(idx: %d)! \n", idx);
}

int platform_get_osdline(void)
{
    return 0;
}

/*
 * VGA proc function
 */
static int proc_read_srcsel(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(page, "LCD source_sel, 0: LCD0 RGB out, 1: LCD2 RGB out,  \n");
	len += sprintf(page, "                2: LCD0 BT1120/656out(CAP123), 3: LCD0 BT1120/656out(GMAC) \n");
	len += sprintf(page, "                4: LCD2 BT1120/656out(CAP123), 5: LCD2 BT1120/656out(GMAC) \n");

	len += sprintf(page + len, "current value = %d\n", lcd_srcsel);
	*eof = 1;		//end of file
	*start = page + off;
	len = len - off;
	return len;
}

static int proc_write_srcsel(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int len = count;
	unsigned char value[20];
	uint tmp;

	if (copy_from_user(value, buffer, len))
		return 0;
	value[len] = '\0';
	sscanf(value, "%u\n", &tmp);

    switch (tmp) {
      case 0:
        /* LCD0 outputs RGB */
        ftpmu010_write_reg(lcd_fd, 0x7C, 0x0 << 21, 0x1F << 21);
        break;
      case 1:
        /* LCD2 outputs RGB */
        ftpmu010_write_reg(lcd_fd, 0x7C, 0x2 << 21, 0x1F << 21);
        break;
      case 2:
        /* LCD0 BT1120/656out(CAP123) */
        ftpmu010_write_reg(lcd_fd, 0x7C, 0x4 << 21, 0x1F << 21);
        ftpmu010_write_reg(lcd_fd, 0x50, 0xA << 5, 0xF << 5);
        break;
      case 3:
        /* LCD0 BT1120/656out(GMAC) */
        ftpmu010_write_reg(lcd_fd, 0x7C, 0x4 << 21, 0x1F << 21);
        ftpmu010_write_reg(lcd_fd, 0x50, 0xA << 10, 0xF << 10);
        break;
      case 4:
        /* LCD2 BT1120/656out(CAP123) */
        ftpmu010_write_reg(lcd_fd, 0x7C, 0x6 << 21, 0x1F << 21);
        ftpmu010_write_reg(lcd_fd, 0x50, 0xA << 5, 0xF << 5);
        break;
      case 5:
        /* LCD2 BT1120/656out(GMAC) */
        ftpmu010_write_reg(lcd_fd, 0x7C, 0x6 << 21, 0x1F << 21);
        ftpmu010_write_reg(lcd_fd, 0x50, 0xA << 10, 0xF << 10);
        break;
      default:
        printk("Invalid value! \n");
        break;
    }

	return count;
}

/*
 * CVBS load polling status
 */
static int proc_read_cvbs_loading(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    u32 tmp;
	int len = 0;

    tmp = (ftpmu010_read_reg(0xE0) >> 16) & 0x1;    /* 1 means CVBS power down */
    if (tmp)
        len += sprintf(page, "cvbs loading off \n");
    else
        len += sprintf(page, "cvbs loading on \n");

	*eof = 1;		//end of file
	*start = page + off;
	len = len - off;

	return len;
}

/* Init function
 */
int platfrom_8287_init(void)
{
    int ret = 0;

    /* BT1120
	 */
    lcd_srcsel_proc = ffb_create_proc_entry("lcd_srcsel", S_IRUGO | S_IXUGO, flcd_common_proc);
    if (lcd_srcsel_proc == NULL) {
        panic("Fail to create lcd_srcsel proc!\n");
		ret = -EINVAL;
		goto exit;
	}
	lcd_srcsel_proc->read_proc = (read_proc_t *) proc_read_srcsel;
	lcd_srcsel_proc->write_proc = (write_proc_t *) proc_write_srcsel;

	/* cvbs loading */
	cvbs_loading_proc = ffb_create_proc_entry("cvbs_loading", S_IRUGO | S_IXUGO, flcd_common_proc);
    if (cvbs_loading_proc == NULL) {
        panic("Fail to create cvbs_loading proc!\n");
		ret = -EINVAL;
		goto exit;
	}
	cvbs_loading_proc->read_proc = (read_proc_t *) proc_read_cvbs_loading;

exit:
    return ret;
}

void platfrom_8287_exit(void)
{
    if (lcd_srcsel_proc)
        ffb_remove_proc_entry(flcd_common_proc, lcd_srcsel_proc);
    lcd_srcsel_proc = NULL;

    if (cvbs_loading_proc)
        ffb_remove_proc_entry(flcd_common_proc, cvbs_loading_proc);
    cvbs_loading_proc = NULL;
}

EXPORT_SYMBOL(platform_cb);
EXPORT_SYMBOL(platform_lock_ahbbus);
EXPORT_SYMBOL(platform_set_lcd_vbase);
EXPORT_SYMBOL(platform_set_lcd_pbase);
EXPORT_SYMBOL(platform_get_lcd_pbase);
EXPORT_SYMBOL(platform_get_osdline);
EXPORT_SYMBOL(platform_get_compress_ipbase);
EXPORT_SYMBOL(platform_get_decompress_ipbase);
