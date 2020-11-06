#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/platform/platform_io.h>
#include <mach/ftpmu010.h>
#include "ffb.h"
#include "proc.h"
#include "debug.h"
#include "dev.h"
#include "codec.h"
#include "lcd_def.h"
#include "platform.h"

/* index
 */
#define GM8126      0

#define SYS_CLK         (platform_cb->sys_clk_base)
#define MAX_SCALAR_CLK  (platform_cb->max_scaler_clk)
int osd_dec_line = 0;           /* how many lines need to be decreased due to bandwidth issue */

static int lcd_fd;

/*
 * GM8126 PMU registers
 */
static pmuReg_t pmu_reg_8126[] = {
    {0x38, (0x1 << 7) | (0x1 << 13), (0x1 << 7) | (0x1 << 13), 0x0, 0x0},
    {0x58, (0x3 << 2), 0x0, 0x0, 0x0},
    {0x64, 0xFFFFFFFF, 0x0, 0x0, 0x0},
    {0x74, (0xF << 8) | (0x3F << 20), (0xF << 8) | (0x3F << 20), 0x0, 0x0},
    {0x7C, 0xF, 0xF, 0x0, 0xF},
};

static pmuRegInfo_t pmu_reg_info_8126 = {
    "LCD_8126",
    ARRAY_SIZE(pmu_reg_8126),
    ATTR_TYPE_PLL2,
    &pmu_reg_8126[0]
};

/* Define the input resolution supporting list. Please fill the input resoluiton here.
 * If the resultion is not defined here, they will not be suppported!
 * Note: the array must be terminated with VIN_NONE.
 */
static VIN_RES_T res_support_list[] = {
    VIN_NTSC, VIN_PAL, VIN_640x480, VIN_800x600, VIN_1024x768, VIN_640x360, VIN_540x360, VIN_352x240, VIN_352x288, VIN_320x240, VIN_NONE
};

/* 
 * Local function declaration
 */
static int platform_check_input_resolution(VIN_RES_T resolution);
static unsigned int platform_pmu_calculate_clk(int lcd200_idx, u_long pixclk,
                                               unsigned int b_Use_Ext_Clk);
static int platform_tve100_config(int lcd_idx, int mode, int input);
static void platform_pmu_clock_gate_8126(int lcd_idx, int on_off, int b_Use_Ext_Clk);
static void platform_pmu_switch_pinmux_8126(void *ptr, int on);

/* 
 * Local variable declaration
 */
static unsigned int lcd_va_base[2];
static unsigned int TVE100_BASE = 0;

/* pmu callback functions
 */
static struct callback_s callback[] = {
    {
     .index = GM8126,
     .name = "GM8126",
     .init_fn = NULL,
     .exit_fn = NULL,
     .sys_clk_base = 30000000,  //30M
     .max_scaler_clk = (130 * 1000000UL),
     .switch_pinmux = platform_pmu_switch_pinmux_8126,
     .pmu_get_clk = platform_pmu_calculate_clk,
     .pmu_clock_gate = platform_pmu_clock_gate_8126,
     .tve_config = platform_tve100_config,
     .check_input_res = platform_check_input_resolution,
     }
};

/* callback funtion structure
 */
struct callback_s *platform_cb = NULL;

void platform_pmu_switch_pinmux_8126(void *ptr, int on)
{
    struct lcd200_dev_info *info = ptr;
    struct ffb_dev_info *fdinfo = (struct ffb_dev_info *)info;
    u32 tmp;

    if (on) {
        if (fdinfo->video_output_type >= VOT_CCIR656) {
            if ((info->CCIR656.Parameter & CCIR656_SYS_MASK) == CCIR656_SYS_HDTV) {
                //BT1120
                tmp =
                    ((0x2 << 0) | (0x2 << 2) | (0x2 << 4) | (0x2 << 6) | (0x2 << 8) | (0x2 << 10) |
                     (0x2 << 12) | (0x2 << 14) | (0x2 << 16) | (0x2 << 18) | (0x2 << 20) | (0x2 <<
                                                                                            22) |
                     (0x2 << 24) | (0x2 << 26) | (0x2 << 28) | (0x2 << 30));
                if (ftpmu010_write_reg(lcd_fd, 0x64, tmp, 0xFFFFFFFF) < 0)
                    panic("LCD: write register 0x64 fail");

                if (ftpmu010_write_reg(lcd_fd, 0x58, (0x3 << 2), (0x3 << 2)) < 0)
                    panic("LCD: write register 0x58 fail");
            } else {
                //BT656
            }
        } else {
            // RGB 16bit
        }
    } else {
        /* do nothing */
    }
}

unsigned int platform_get_pll2(int out_pixel_clk)
{
    unsigned int tmp;

    tmp = ftpmu010_get_attr(ATTR_TYPE_PLL2);

    return tmp;
}

#define DIFF(x,y)   ((x) > (y) ? (x) - (y) : (y) - (x))

/* pixclk: Standard VESA clock (kHz)
 */
unsigned int platform_pmu_calculate_clk(int lcd200_idx, u_long pixclk, unsigned int b_Use_Ext_Clk)
{
    unsigned int pll2, div, sclar_div;
    unsigned int diff, tolerance, v1, v2, v3, org_val;
    u32 LCDen, tmp, tmp2, tmp_msk;

    if (b_Use_Ext_Clk) {
    }

    pll2 = platform_get_pll2(pixclk) / 1000;    //chage to Khz
    div = ((pll2 + pixclk / 2) / pixclk) - 1;
    if (!div)
        div = 1;

    /* calculate scalar clock */
    sclar_div = platform_get_pll2(pixclk);
    do_div(sclar_div, MAX_SCALAR_CLK);
    if (sclar_div > div)
        sclar_div = div;

    /* 
     * check if the clock divisor is changed 
     */
    tmp = ftpmu010_read_reg(0x74);
    tmp2 = (tmp >> 8) & 0xF;    //scaler clock
    if (tmp2 == sclar_div) {
        tmp2 = (tmp >> 20) & 0x3F;
        if (tmp2 == div)
            goto exit;          /* do nothing */
    }

    v1 = platform_get_pll2(pixclk) / (div + 1);
    v1 /= 1000000;              //to Mhz
    v2 = platform_get_pll2(pixclk) / (div + 1);
    v2 = ((v2 % 1000000) * 100) / 1000000;
    v3 = platform_get_pll2(pixclk) / (sclar_div + 1);
    v3 /= 1000000;              //to Mhz
    printk("LCD%d: Standard: %d.%dMHZ \n", lcd200_idx, (u32) (pixclk / 1000),
           (u32) ((100 * (pixclk % 1000)) / 1000));
    printk("LCD%d: PVALUE = %d, pixel clock = %d.%dMHZ, scalar clock = %dMHZ \n", lcd200_idx, div,
           v1, v2, v3);

    diff = DIFF(pixclk * 1000, platform_get_pll2(pixclk) / (div + 1));
    tolerance = diff / pixclk;  /* comment: (diff * 1000) / (pixclk * 1000); */
    if (tolerance > 5) {        /* if the tolerance is larger than 0.5%, dump error message! */
        int i = 0;

        v1 = platform_get_pll2(pixclk) / (div + 1);
        v1 /= 1000000;          //to Mhz
        v2 = platform_get_pll2(pixclk) / (div + 1);
        v2 = ((v2 % 1000000) * 100) / 1000000;

        while (i < 50) {
            printk("The standard clock is %d.%dMHZ, but gets %d.%dMHZ. Please correct pll2!\n",
                   (u32) (pixclk / 1000), (u32) ((100 * (pixclk % 1000)) / 1000), v1, v2);
            i++;
        }
    }

    /* change the frequency, the steps will be:
     * 1) release the abh bus lock
     * 2) disable LCD controller
     * 3) turn off the gating clock
     */
    LCDen = ioread32(lcd_va_base[lcd200_idx]);
    if (LCDen & 0x1) {
        u32 temp;

        /* release bus */
        org_val = platform_lock_ahbbus(lcd200_idx, 0);
        /* disable LCD controller */
        temp = LCDen & (~0x1);
        iowrite32(temp, lcd_va_base[lcd200_idx]);
        /* disable gating clock */
        platform_cb->pmu_clock_gate(lcd200_idx, 0, 0);
    }

    tmp = ftpmu010_read_reg(0x74);
    tmp_msk = (0xF << 8) | (0x3F << 20);
    tmp = (sclar_div << 8) | (div << 20);
    ftpmu010_write_reg(lcd_fd, 0x74, tmp, tmp_msk);

    /* rollback the orginal state */
    if (LCDen & 0x1) {
        /* enable gating clock */
        platform_cb->pmu_clock_gate(lcd200_idx, 1, 0);

        /* enable LCD controller */
        iowrite32(LCDen, lcd_va_base[lcd200_idx]);

        /* lock bus ? */
        platform_lock_ahbbus(lcd200_idx, org_val);
    }

  exit:
    return platform_get_pll2(pixclk) / (div + 1);
}

/* this function will be called in initialization state or module removed */
void platform_pmu_clock_gate_8126(int lcd_idx, int on_off, int use_ext_clk)
{
    u32 tmp, tmp_msk;
    
    if (use_ext_clk) {}
    
    if (on_off) {
        /* ON 
         */
        tmp_msk = (0x1 << 7) | (0x1 << 13);
        tmp = 0;
        /* turn on LCD global clock */
        tmp &= ~(0x1 << 7);     // lcd global clock/scaler clock
        tmp &= ~(0x1 << 13);    // ct656 clock
        ftpmu010_write_reg(lcd_fd, 0x38, tmp, tmp_msk);
    } else {
        /* OFF
         */
        tmp_msk = (0x1 << 7);
        tmp = 0;
        /* turn off LCD global clock */
        tmp |= (0x1 << 7);
        ftpmu010_write_reg(lcd_fd, 0x38, tmp, tmp_msk);
    }
}

/* 
 * mode: VOT_NTSC, VOT_PAL
 * input: 2 : 656in
 *        6 : colorbar output for test
 */
int platform_tve100_config(int lcd_idx, int mode, int input)
{
    u32 tmp, base = TVE100_BASE;

    if (lcd_idx) {
    }

    /* only support PAL & NTSC */
    if ((mode != VOT_NTSC) && (mode != VOT_PAL))
        return -EINVAL;

    if ((input != 2) && (input != 6))   /* color bar */
        return -EINVAL;

    /* check LCD is enabled or not */
    tmp = ioread32(lcd_va_base[lcd_idx]);
    if (!(tmp & 0x1))
        return -EINVAL;

    /* TVE DAC PMU */
    ftpmu010_write_reg(lcd_fd, 0x7C, 0x0, 0xF);

    /* 0x0 */ /* bit2: 1: Black level is 0 IRE (Black level is the same as blanking level). */    
    tmp = (mode == VOT_NTSC) ? 0x0 : (0x2 | 0x4);
    iowrite32(tmp, base + 0x0);

    /* 0x4 */
    iowrite32(input, base + 0x4);

    /* 0x8, 0xc */
    iowrite32(0, base + 0x8);

    /* bit 0: power down, bit 1: standby */
    iowrite32(0, base + 0xc);

    return 0;
}

/* Check the input resolution is supported or not
 * 0 for yes, -1 for No
 */
int platform_check_input_resolution(VIN_RES_T resolution)
{
    int i, bFound = 0;

    for (i = 0; (res_support_list[i] != VIN_NONE); i++) {
        if (res_support_list[i] == resolution) {
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
    if (lcd_idx || on) {
    }

    return 0;
}

/*
 * Platform init function. This platform dependent.
 */
int platform_pmu_init(void)
{
    int i;
    u32 type;

    type = GM8126;

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

    if (platform_cb->init_fn)
        platform_cb->init_fn();

    lcd_fd = ftpmu010_register_reg(&pmu_reg_info_8126);
    if (lcd_fd < 0)
        panic("lcd register pmu fail!");

    TVE100_BASE = (unsigned int)ioremap_nocache(TVE_FTTVE100_PA_BASE, TVE_FTTVE100_PA_SIZE);
    if (!TVE100_BASE) {
        printk("LCD: No virtual memory! \n");
        return -1;
    }

    return 0;
}

/*
 * Platform exit function. This platform dependent.
 */
int platform_pmu_exit(void)
{
    __iounmap((void *)TVE100_BASE);

    if (platform_cb->exit_fn)
        platform_cb->exit_fn();
    
    /* de-register pmu */
    ftpmu010_deregister_reg(lcd_fd);
    
    return 0;
}

void platform_set_lcdbase(int idx, unsigned int va)
{
    lcd_va_base[idx] = va;
}

int platform_get_osdline(void)
{
    return osd_dec_line;
}

EXPORT_SYMBOL(platform_cb);
EXPORT_SYMBOL(platform_lock_ahbbus);
EXPORT_SYMBOL(platform_set_lcdbase);
EXPORT_SYMBOL(platform_get_osdline);
