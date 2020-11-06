#include <linux/kernel.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/platform/spec.h>
#include <mach/platform/platform_io.h>
#include "ffb.h"
#include "proc.h"
#include "debug.h"
#include "dev.h"
#include "codec.h"
#include "lcd_def.h"
#include "platform.h"
#include <mach/ftpmu010.h>

/* index
 */
#define GM8181      0
#define GM8181T     1

#define SYS_CLK         (platform_cb->sys_clk_base)
#define MAX_SCALAR_CLK  (platform_cb->max_scaler_clk)
#define HZ_TO_MHZ(x)    ((x)/1000000)

int osd_dec_line = 1;           /* how many lines need to be decreased due to bandwidth issue */
static int lcd_fd = 0;
static unsigned int TVE100_BASE = 0;
/*
 * GM8181
 */
static pmuReg_t pmu_reg_8181[] = {
    {0x28, (0x1 << 14) | (0x1 << 8), (0x1 << 14) | (0x1 << 8), (0x1 << 14),
     (0x1 << 14) | (0x1 << 8)},
    {0x38, (0x1 << 13) | (0x1 << 10) | (0x1 << 7), (0x1 << 13) | (0x1 << 10) | (0x1 << 7), 0, 0},
    {0x44, (0xF << 16), (0x7 << 17), (0x1 << 17), (0xF << 16)},
    {0x50, (0x7 << 5) | (0x1 << 4), (0x7 << 5), 0x0, 0x0},
    {0x6c, (0x7 << 13), (0x7 << 13), 0x0, 0x0},
    {0x70, (0x1F << 10) | (0x1F << 5), (0x1F << 10) | (0x1F << 5), 0, 0},
    {0x74, (0xF << 12), (0xF << 12), 0, 0},
};

static pmuRegInfo_t pmu_reg_info_8181 = {
    "LCD_8181",
    ARRAY_SIZE(pmu_reg_8181),
    ATTR_TYPE_PLL3,
    &pmu_reg_8181[0]
};

/*
 * GM8181T
 */
static pmuReg_t pmu_reg_8181t[] = {
    {0x28, (0x7 << 18) | (0x1 << 14) | (0x1 << 8) | (0x1 << 6), (0x7 << 18) | (0x1 << 14) | (0x1 << 8), 0, 0}, //remove bit6 because it also used by capture decoder
    {0x38, (0x1 << 13) | (0x1 << 10) | (0x1 << 7), (0x1 << 13) | (0x1 << 10) | (0x1 << 7), 0, 0},
    {0x3C, (0x3 << 28), (0x3 << 28), 0, 0},
    {0x44, (0xF << 16), (0x7 << 17), (0x1 << 17), (0xF << 16)},
    {0x50, (0x7 << 25) | (0x7 << 5) | (0x1 << 4) | (0x1 << 3),
    (0x7 << 25) | (0x7 << 5) | (0x1 << 4) | (0x1 << 3), 0x0, 0x0},
    {0x6c, (0x7 << 13), (0x7 << 13), 0x0, 0x0},
    {0x70, (0x1F << 10) | (0x1F << 5), (0x1F << 10) | (0x1F << 5), 0, 0},
    {0x74, (0x1F << 21) | (0x1F << 26) | (0xF << 12), (0x1F << 21) | (0x1F << 26) | (0xF << 12), 0, 0},
};

static pmuRegInfo_t pmu_reg_info_8181t = {
    "LCD_8181T",
    ARRAY_SIZE(pmu_reg_8181t),
    ATTR_TYPE_PLL3,
    &pmu_reg_8181t[0]
};

/* Define the input resolution supporting list. Please fill the input resoluiton here.
 * If the resultion is not defined here, they will not be suppported!
 * Note: the array must be terminated with VIN_NONE.
 */
static VIN_RES_T res_support_list[] = {
    VIN_NTSC, VIN_PAL, VIN_640x480, VIN_800x600, VIN_1024x768, VIN_1280x800, VIN_NONE
};

/* 
 * Local function declaration
 */
unsigned int platform_get_pll3(void); 
static void platform_pmu_switch_pinmux_2(int lcd_idx, OUT_TARGET_T target);
static int platform_check_input_resolution(VIN_RES_T resolution);
static unsigned int platform_pmu_calculate_clk(int lcd200_idx, u_long pixclk,
                                               unsigned int b_Use_Ext_Clk);
static int platform_tve100_config(int lcd_idx, int mode, int input);
static void platform_pmu_clock_gate_8181(int lcd_idx, int on_off, int use_ext_clk);

/* 
 * Local variable declaration
 */
/* define default target */
static OUT_TARGET_T output_target[2] = { OUT_TARGET_VGA, OUT_TARGET_CVBS };
static int lcd_pin_init[2] = { 0, 0 };
static unsigned int lcd_va_base[2], lcd_pa_base[2];

#define DIFF(x,y)   ((x) > (y) ? (x) - (y) : (y) - (x))

#include "gm8181t.c"

/* pmu callback functions
 */
static struct callback_s callback[] = {
    {
     .index = GM8181,
     .name = "GM8181",
     .init_fn = NULL,
     .exit_fn = NULL,
     .sys_clk_base = 12000000,  //12M
     .max_scaler_clk = (130 * 1000000UL),       //130M
     .switch_pinmux = platform_pmu_switch_pinmux_8181,
     .pmu_get_clk = platform_pmu_calculate_clk,
     .pmu_clock_gate = platform_pmu_clock_gate_8181,
     .tve_config = platform_tve100_config,
     .check_input_res = platform_check_input_resolution,
     },
    {
     .index = GM8181T,
     .name = "GM8181T",
     .init_fn = platfrom_8181t_init,
     .exit_fn = platfrom_8181t_exit,
     .sys_clk_base = 12000000,  //12M
     .max_scaler_clk = (2205 * 100000UL),       //220.5M
     .switch_pinmux = platform_pmu_switch_pinmux_8181t,
     .pmu_get_clk = platform_pmu_calculate_clk,
     .pmu_clock_gate = platform_pmu_clock_gate_8181t,
     .tve_config = platform_tve100_config_8181t,
     .check_input_res = platform_check_input_resolution,
     }
};

/* callback funtion structure
 */
struct callback_s *platform_cb = NULL;

void platform_pmu_switch_pinmux_8181(void *ptr, int on)
{
    struct lcd200_dev_info *info = ptr;
    struct ffb_dev_info *fdinfo = (struct ffb_dev_info *)info;
    int is_8182 = 0, cap_bit;
    u32 tmp, tmp_mask, tmp2, tmp2_mask, tmp3, tmp3_mask;

    is_8182 = (ftpmu010_read_reg(0x0) & 0x40) ? 1 : 0;
    cap_bit = (is_8182 == 1) ? 4 : 3;   /* 8181:cap3, 8182:cap4 as output */

    /* 0x28 config tveout_en to output mode */
    ftpmu010_write_reg(lcd_fd, 0x28, (0x1 << 14), (0x1 << 14));

    if (on) {
        /* 0x50 */
        tmp_mask = 0;
        tmp_mask |= 0x1 << cap_bit;
        tmp_mask |= (0x7 << 5); //maskout tv16_lcd24_mux, bit 5-7
        tmp = 0;

        if (fdinfo->video_output_type >= VOT_CCIR656) {
            if ((info->CCIR656.Parameter & CCIR656_SYS_MASK) == CCIR656_SYS_HDTV) {
                //BT1120
                // 000: X_TV_CLK and X_TV_DATA[15:0] are BT656/BT1120
            } else {
                //BT656
                // 010: CT656 DATA[7:0] are output
                //tmp |= (2 << 5);
            }

            /* VGA chip */
            tmp2_mask = (0x1 << 13);
            tmp2_mask |= (0x1 << 14);
            tmp2_mask |= (0x1 << 15);
            tmp2 = 0;
            tmp2 |= (0x0 << 13);        /* vga_vs_he_oe output enable, 0:disabled */
            tmp2 |= (0x1 << 14);        /* Power-down control of the VGA DAC, 1: Power down. */
            tmp2 |= (0x1 << 15);        /* Standby mode control, 1: Standby mode. */
            ftpmu010_write_reg(lcd_fd, 0x6C, tmp2, tmp2_mask);
        } else {
            // RGB 16bit
            // 000: X_TV_CLK and X_TV_DATA[15:0] are LCD RGB[15:0] data output

            switch (codec_get_output_type(info->lcd200_idx)) {
            case OUTPUT_TYPE_CAT6611_1024x768_24:
                tmp |= (0x1 << cap_bit);        /* cap3_lcd24_mux or cap4_lcd24_mux, output mode */
                break;
            default:
                break;
            }

            tmp |= (0x1 << 5);
            ftpmu010_write_reg(lcd_fd, 0x50, tmp, tmp_mask);

            /* VGA chip */
            tmp2_mask = (0x1 << 13);
            tmp2_mask |= (0x1 << 14);
            tmp2_mask |= (0x1 << 15);
            tmp2 = 0;
            tmp2 |= (0x1 << 13);        /* vga_vs_he_oe output enable, 0:disabled */
            tmp2 |= (0x0 << 14);        /* Power-down control of the VGA DAC, 1: Power down. */
            tmp2 |= (0x0 << 15);        /* Standby mode control, 1: Standby mode. */
            ftpmu010_write_reg(lcd_fd, 0x6C, tmp2, tmp2_mask);
        }

        ftpmu010_write_reg(lcd_fd, 0x50, tmp, tmp_mask);
    } else {
        /* do nothing */
    }

    /* driving capbility */
    tmp3_mask = (0xF << 16);
    tmp3 = (0x1 << 17);         //8mA
    ftpmu010_write_reg(lcd_fd, 0x44, tmp3, tmp3_mask);
}

unsigned int platform_get_pll3(void)
{
    return ftpmu010_get_attr(ATTR_TYPE_PLL3);
}

/* pixclk: Standard VESA clock (kHz)
 */
unsigned int platform_pmu_calculate_clk(int lcd200_idx, u_long pixclk, unsigned int b_Use_Ext_Clk)
{
    u32 tmp, tmp_mask, LCDen;
    unsigned int div, sclar_div;
    unsigned int diff, tolerance, v1, v2, v3;
    int org_val = 0;

    if (b_Use_Ext_Clk) {
    }

    pixclk *= 1000;             //change to Mhz 

    div = ftpmu010_clock_divisor(lcd_fd, pixclk, 1) - 1;
    if (!div)
        div = 1;

    /* calculate scalar clock */
    sclar_div = ftpmu010_clock_divisor(lcd_fd, platform_cb->max_scaler_clk, 0);
    if (sclar_div > div)
        sclar_div = div;

    /* 
     * check if the clock divisor is changed 
     */
    if (lcd200_idx == LCD_ID) {
        u32 tmp, tmp2;

        tmp = ftpmu010_read_reg(0x70);
        tmp2 = (tmp >> 10) & 0x1F;      //scaler clock
        if (tmp2 == sclar_div) {
            tmp2 = (tmp >> 5) & 0x1F;
            if (tmp2 == div)
                goto exit;      /* do nothing */
        }
    } else if (lcd200_idx == LCD2_ID) {
        u32 tmp, tmp2;

        tmp = ftpmu010_read_reg(0x74);
        tmp2 = (tmp >> 26) & 0x1F;      //scaler clock
        if (tmp2 == sclar_div) {
            tmp2 = (tmp >> 21) & 0x1F;
            if (tmp2 == div)    /* just check TVE2_PVALUE */
                goto exit;      /* do nothing */
        }
    }

    v1 = platform_get_pll3() / (div + 1);
    v1 = HZ_TO_MHZ(v1);         //to Mhz
    v2 = platform_get_pll3() / (div + 1);
    v2 = ((v2 % 1000000) * 100) / 1000000;
    v3 = platform_get_pll3() / (sclar_div + 1);
    v3 = HZ_TO_MHZ(v3);         //to Mhz
    printk("LCD%d: Standard: %d.%dMHZ \n", lcd200_idx, (u32) HZ_TO_MHZ(pixclk),
           (u32) ((100 * (pixclk % 1000)) / 1000));
    printk("LCD%d: PVALUE = %d, pixel clock = %d.%dMHZ, scalar clock = %dMHZ \n", lcd200_idx, div,
           v1, v2, v3);

    diff = DIFF(pixclk, platform_get_pll3() / (div + 1));
    tolerance = diff / pixclk;  /* comment: (diff * 1000) / (pixclk * 1000); */
    if (tolerance > 5) {        /* if the tolerance is larger than 0.5%, dump error message! */
        int i = 0;

        v1 = platform_get_pll3() / (div + 1);
        v1 = HZ_TO_MHZ(v1);     //to Mhz
        v2 = platform_get_pll3() / (div + 1);
        v2 = ((v2 % 1000000) * 100) / 1000000;

        while (i < 50) {
            printk("The standard clock is %d.%dMHZ, but gets %d.%dMHZ. Please correct PLL3!\n",
                   (u32) HZ_TO_MHZ(pixclk), (u32) ((100 * (pixclk % 1000)) / 1000), v1, v2);
            i++;
        }
    }

    /* change the frequency, the steps will be:
     * 1) release the abh bus lock
     * 2) disable LCD controller
     * 3) turn off the gating clock
     */
    if (lcd200_idx == LCD_ID) {
        LCDen = ioread32(lcd_va_base[0]);
        if (LCDen & 0x1) {
            u32 temp;

            /* release bus */
            org_val = platform_lock_ahbbus(lcd200_idx, 0);
            /* disable LCD controller */
            temp = LCDen & (~0x1);
            iowrite32(temp, lcd_va_base[0]);
            /* disable gating clock */
            platform_cb->pmu_clock_gate(lcd200_idx, 0, 0);
        }

        /* 0x70 */
        tmp = 0;
        tmp_mask = (0x1F << 10) | (0x1F << 5);  // maskout 14-10, 9-5        
        tmp |= ((sclar_div << 10) | (div << 5));
        ftpmu010_write_reg(lcd_fd, 0x70, tmp, tmp_mask);

        /* rollback the orginal state */
        if (LCDen & 0x1) {
            /* enable gating clock */
            platform_cb->pmu_clock_gate(lcd200_idx, 1, 0);

            /* enable LCD controller */
            iowrite32(LCDen, lcd_va_base[0]);

            /* lock bus ? */
            platform_lock_ahbbus(lcd200_idx, org_val);
        }
    } else if (lcd200_idx == LCD2_ID) {
        LCDen = ioread32(lcd_va_base[1]);
        if (LCDen & 0x1) {
            u32 temp;

            /* release bus */
            org_val = platform_lock_ahbbus(lcd200_idx, 0);
            /* disable LCD controller */
            temp = LCDen & (~0x1);
            iowrite32(temp, lcd_va_base[1]);
            /* disable gating clock */
            platform_cb->pmu_clock_gate(lcd200_idx, 0, 0);
        }

        /* 0x74 */
        tmp = 0;
        tmp_mask = (0x1F << 26) | (0x1F << 21); // maskout 30-26, 25-21
        tmp |= ((sclar_div << 26) | (div << 21));
        ftpmu010_write_reg(lcd_fd, 0x74, tmp, tmp_mask);

        /* rollback the orginal state */
        if (LCDen & 0x1) {
            /* enable gating clock */
            platform_cb->pmu_clock_gate(lcd200_idx, 1, 0);

            /* enable LCD controller */
            iowrite32(LCDen, lcd_va_base[1]);

            /* lock bus ? */
            platform_lock_ahbbus(lcd200_idx, org_val);
        }
    }
  exit:
    return platform_get_pll3() / (div + 1);
}

/* this function will be called in initialization state or module removed */
void platform_pmu_clock_gate_8181(int lcd_idx, int on_off, int use_ext_clk)
{
    u32 tmp, tmp_mask;

    if (on_off) {
        /* ON 
         */
        tmp = 0;
        tmp_mask = (0x1 << 7) | (0x1 << 10) | (0x1 << 13);
        tmp |= (0x0 << 7);      // lcd global clock(hclk)
        tmp |= (0x0 << 10);     // pixel clock
        tmp |= (0x0 << 13);     // ct656 clock 
        ftpmu010_write_reg(lcd_fd, 0x38, tmp, tmp_mask);
    } else {
        /* OFF
         */
        tmp = 0;
        tmp_mask = (0x1 << 7) | (0x1 << 10) | (0x1 << 13);
        tmp |= (0x1 << 7);      // lcd global clock(hclk)
        tmp |= (0x1 << 10);     // pixel clock
        tmp |= (0x1 << 13);     // ct656 clock 
        ftpmu010_write_reg(lcd_fd, 0x38, tmp, tmp_mask);

        /* Power-down VGA chip */
        tmp_mask = (0x1 << 14);
        tmp = (0x1 << 14);      /* Power-down control of the VGA DAC, 1: Power down. */
        ftpmu010_write_reg(lcd_fd, 0x6C, tmp, tmp_mask);
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

    /* tve100 needs lcd pixel clock, otherwise will hang up */
    tmp = ftpmu010_read_reg(0x38);
    if (tmp & (0x1 << 7))
        return -EINVAL;

    /* only support PAL & NTSC */
    if ((mode != VOT_NTSC) && (mode != VOT_PAL))
        return -EINVAL;

    if ((input != 2) && (input != 6))   /* color bar */
        return -EINVAL;

    /* check LCD is enabled or not */
    tmp = ioread32(lcd_va_base[0]);
    if (!(tmp & 0x1))
        return -EINVAL;
        
    /* 0x0 */
    tmp = (mode == VOT_NTSC) ? 0x0 : 0x2;
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
    u32 base, value;
    static int lock_bus[4] = { -1, -1, -1, -1 };
    int org_val = lock_bus[lcd_idx];

    if ((on != 0) && (on != 1))
        return org_val;         /* do nothing */

    if (on == lock_bus[lcd_idx])
        return org_val;

    lock_bus[lcd_idx] = on;

    base = (lcd_idx == LCD_ID) ? lcd_va_base[0] : lcd_va_base[1];

    value = ioread32(base + 0x50);
    value &= ~(0x1 << 9);       //bit9:lock ahb bus
    value |= (on << 9);
    iowrite32(value, base + 0x50);

    /* disable */
    if (on == 0) {
        int delay = (20 * HZ) / 1000;

        if (delay == 0)
            delay = 20;

        /* we must wait a moment to have controller to send the commands without AHBLCOK */
        set_current_state(TASK_UNINTERRUPTIBLE);
        schedule_timeout(delay);
    }

    return org_val;
}

/*
 * Platform init function. This platform dependent.
 */
int platform_pmu_init(void)
{
    int i;
    u32 tmp, type = -1;

    tmp = (ftpmu010_read_reg(0x0) >> 8) & 0xF0;
    switch (tmp) {
    case 0x10:
        type = GM8181;
        osd_dec_line = 1;
        break;
    case 0x20:
        type = GM8181T;
        osd_dec_line = 2;
        break;
    default:
        break;
    }

    if (type == GM8181) {
        lcd_fd = ftpmu010_register_reg(&pmu_reg_info_8181);
        if (lcd_fd < 0) {
            printk("LCD: %s fail \n", __FUNCTION__);
            return -1;
        }
    } else if (type == GM8181T) {
        lcd_fd = ftpmu010_register_reg(&pmu_reg_info_8181t);
        if (lcd_fd < 0) {
            printk("LCD: %s fail \n", __FUNCTION__);
            return -1;
        }
    }

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
    
    TVE100_BASE = (unsigned int)ioremap_nocache(TVE_FTTVE100_PA_BASE, TVE_FTTVE100_PA_SIZE);
    if (!TVE100_BASE) {
        printk("LCD: No virtual memory! \n");
        return -1;
    }
    
    if (platform_cb->init_fn)
        platform_cb->init_fn();

    return 0;
}

/*
 * Platform exit function. This platform dependent.
 */
int platform_pmu_exit(void)
{    
    if (platform_cb->exit_fn)
        platform_cb->exit_fn();
    
    __iounmap((void *)TVE100_BASE);
    
    return 0;
}

void platform_set_lcd_vbase(int idx, unsigned int va)
{
    lcd_va_base[idx] = va;
}

void platform_set_lcd_pbase(int idx, unsigned int pa)
{
    lcd_pa_base[idx] = pa;
}

int platform_get_osdline(void)
{
    return osd_dec_line;
}

EXPORT_SYMBOL(platform_cb);
EXPORT_SYMBOL(platform_lock_ahbbus);
EXPORT_SYMBOL(platform_set_lcd_vbase);
EXPORT_SYMBOL(platform_set_lcd_pbase);
EXPORT_SYMBOL(platform_get_osdline);
