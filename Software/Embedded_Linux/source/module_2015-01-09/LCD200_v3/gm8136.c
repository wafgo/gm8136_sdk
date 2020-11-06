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
#define GM8136      0

#define SYS_CLK         (platform_cb->sys_clk_base)
#define MAX_SCALAR_CLK  (platform_cb->max_scaler_clk)

static int lcd_fd;

static u32  compress_pa_ipbase[LCD_IP_NUM] = {GRAPHIC_ENC_PA_BASE};
static u32  decompress_pa_ipbase[LCD_IP_NUM] = {GRAPHIC_DEC_0_PA_BASE};
static u32 dac_onoff[LCD_IP_NUM] = {LCD_SCREEN_ON}; /* 1 for ON, 0 for OFF */

/*
 * GM8136 PMU registers
 */
static pmuReg_t pmu_reg_8136[] = {
    /* LCD scaler CLK, 1'b0-lcd_scarclk_cntp; 1'b1-lcd_pixclk_cntp
     * LCD CLK, 1'b0-PLL3 (594MHz); 1'b1-PLL2 (600MHz)
     */
    {0x28, (0x3 << 8), (0x3 << 8), 0x0 << 8, 0x3 << 8},
    /* Digital PAD, TV DATA 0-7,  LC_DATA0-7*/
    {0x5C, 0xFFFFFFFF, 0xFFFF, 0x0, 0x0},
    {0x64, (0xF << 6) | 0x3, (0xF << 6) | 0x3, 0x0, 0x0},

    /* [13:8] LCD scaler clock divided value, [5:0] LCD pixel clock divided value */
    {0x78, (0x3F << 8) | 0x3F, (0x3F << 8) | 0x3F, 0x0, 0x0},
    /* TV_PCLK: clock inverse */
    {0x7C, 0x1 << 26, 0x1 << 26, 0, 0},
    /* RESET, rlc_dec and FTLCDC210(aclk), rlc_enc(aclk) */
    //{0xA0, 0x3 << 9, 0x3 << 9, 0x0 << 9, 0x3 << 9},
    /* AXIMCLKOFF: LCD Gate, GPENC */
    {0xB0, (0x1 << 15) | (0x1 << 12), (0x1 << 15) | (0x1 << 12), 0x0 << 12, 0x1 << 12},
    /* TVE  (reg) */
    {0xB4, (0x1 << 20), (0x1 << 20), 0, (0x1 << 20)},
    /* adda_wrapper */
    {0xB8, 0x1 << 6, 0, 0x0, 0x1 << 6},
    /* APBMCLKOFF: LCD Gate */
    {0xBC, (0x7 << 15), (0x7 << 15), 0x0 << 15, 0x7 << 15},
};

static pmuRegInfo_t pmu_reg_info_8136 = {
    "LCD_8136",
    ARRAY_SIZE(pmu_reg_8136),
    ATTR_TYPE_PLL3,
    &pmu_reg_8136[0]
};

/* Define the input resolution supporting list. Please fill the input resoluiton here.
 * If the resultion is not defined here, they will not be suppported!
 * Note: the array must be terminated with VIN_NONE.
 */
/* Define the input resolution supporting list. Please fill the input resoluiton here.
 * If the resultion is not defined here, they will not be suppported!
 * Note: the array must be terminated with VIN_NONE.
 */
static VIN_RES_T res_support_list[LCD_IP_NUM][30] = {
    /* LCD0 */
#if 1
    {VIN_NTSC, VIN_PAL, VIN_NONE},
#else
    {VIN_NTSC, VIN_PAL, VIN_640x480, VIN_800x600, VIN_1024x768, VIN_1280x800, VIN_1280x960, VIN_720P,
    VIN_1280x1024, VIN_1920x1080, VIN_NONE},
#endif
};

/*
 * Local function declaration
 */
int platform_check_input_resolution(int lcd_idx, VIN_RES_T resolution);
static unsigned int platform_pmu_calculate_clk(int lcd200_idx, u_long pixclk,
                                               unsigned int b_Use_Ext_Clk);
static int platform_tve100_config(int lcd_idx, int mode, int input);
static void platform_pmu_clock_gate_8136(int lcd_idx, int on_off, int b_Use_Ext_Clk);
static void platform_pmu_switch_pinmux_8136(void *ptr, int on);

/*
 * Local variable declaration
 */
static unsigned int lcd_va_base[LCD_IP_NUM], lcd_pa_base[LCD_IP_NUM];
static unsigned int TVE100_BASE = 0, ADDA_WRAPPER_BASE = 0;

/* pmu callback functions
 */
static struct callback_s callback[] = {
    {
     .index = GM8136,
     .name = "GM8136",
     .init_fn = NULL,
     .exit_fn = NULL,
     .sys_clk_base = 30000000,
     .max_scaler_clk = (1485 * 100000UL),   /* 148.5 Mhz */
     .switch_pinmux = platform_pmu_switch_pinmux_8136,
     .pmu_get_clk = platform_pmu_calculate_clk,
     .pmu_clock_gate = platform_pmu_clock_gate_8136,
     .tve_config = platform_tve100_config,
     .check_input_res = platform_check_input_resolution,
     }
};

/* callback funtion structure
 */
struct callback_s *platform_cb = NULL;

void platform_pmu_switch_pinmux_8136(void *ptr, int on)
{
    struct lcd200_dev_info  *info = ptr;
	struct ffb_dev_info *fdinfo = (struct ffb_dev_info*)info;
    u32     tmp, tmp_mask;

    if (on) {
        if (fdinfo->video_output_type >= VOT_CCIR656) {
             /* TV */
            if ((info->CCIR656.Parameter & CCIR656_SYS_MASK) == CCIR656_SYS_HDTV) {
                /* BT1120 */
                tmp_mask = 0xFFFFFFFF;
                tmp = 0xFFFFFFFF;   /* 2b11 means X_TV_DATA[x] for pin 0-16*/
                if (ftpmu010_write_reg(lcd_fd, 0x5C, tmp, tmp_mask))
                    panic("%s, error 0 \n", __func__);

                /* X_TV_PCLK */
                tmp_mask = 0x3 << 8;
                tmp = 0x3 << 8;
                if (ftpmu010_write_reg(lcd_fd, 0x64, tmp, tmp_mask))
                    panic("%s, error 1 \n", __func__);
            }
#if 0
            else {
                /* DIGITAL OUT CCIR656 */
                tmp_mask = 0xFFFF;
                tmp = 0xFFFF;   /* 2b11 means X_TV_DATA[x] for pin 0-16*/
                if (ftpmu010_write_reg(lcd_fd, 0x5C, tmp, tmp_mask))
                    panic("%s, error 3 \n", __func__);

                /* X_TV_PCLK */
                tmp_mask = 0x3 << 8;
                tmp = 0x3 << 8;
                if (ftpmu010_write_reg(lcd_fd, 0x64, tmp, tmp_mask))
                    panic("%s, error 4 \n", __func__);
            }
#endif
        } else {
            /* RGB888 */
            tmp_mask = 0xFFFFFFFF;
            tmp = 0xAAAAAAAA;   /* 2b10 means X_LC_DATA[x] */
            if (ftpmu010_write_reg(lcd_fd, 0x5C, tmp, tmp_mask))
                panic("%s, error 0 \n", __func__);

            /* X_LC_PCLK */
            tmp_mask = 0x3 << 8;
            tmp = 0x3 << 8;
            if (ftpmu010_write_reg(lcd_fd, 0x64, tmp, tmp_mask))
                panic("%s, error 1 \n", __func__);

            /* X_LC_VS, X_LC_HS */
            tmp_mask = (0x3 << 6) | 0x3;
            tmp = (0x2 << 6) | 0x2;
            if (ftpmu010_write_reg(lcd_fd, 0x64, tmp, tmp_mask))
                panic("%s, error 2 \n", __func__);
        }
    }
}

#define DIFF(x,y)   ((x) > (y) ? (x) - (y) : (y) - (x))

/* pixclk: Standard VESA clock (kHz)
 */
unsigned int platform_pmu_calculate_clk(int lcd200_idx, u_long pixclk, unsigned int b_Use_Ext_Clk)
{
    int div;
    unsigned int pll, i = 0;
    unsigned int diff, tolerance;
    unsigned int org_val, tmp, scar_clk = platform_cb->max_scaler_clk;

    /* off the axi clk */
    tmp = ftpmu010_read_reg(0xB0);
    org_val = tmp & (0x1 << 15);
    ftpmu010_write_reg(lcd_fd, 0xB0, 0, 0x1 << 15);

    /* scaler clock cntp */
    div = ftpmu010_clock_divisor(lcd_fd, scar_clk, 1) - 1;
    if (div <= 0)
        panic("%s, bug in scaler div:%d ! \n", __func__, div);
    ftpmu010_write_reg(lcd_fd, 0x78, div << 8, 0x3F << 8);

    /* Pixel clock */
    div = ftpmu010_clock_divisor(lcd_fd, pixclk * 1000, 1) - 1;
    if (div <= 0)
        panic("%s, bug in lcd div:%d ! \n", __func__, div);
    ftpmu010_write_reg(lcd_fd, 0x78, div, 0x3F);

    pll = ftpmu010_get_attr(ATTR_TYPE_PLL3);
    diff = DIFF(pixclk * 1000, pll / (div + 1));
    tolerance = diff / pixclk;  /* comment: (diff * 1000) / (pixclk * 1000); */
    if (tolerance > 5) {        /* if the tolerance is larger than 0.5%, dump error message! */
        while (i < 50) {
            printk("The standard clock is %d.%dMHZ, but gets %dMHZ. Please correct PLL!\n",
                   (u32) (pixclk / 1000), (u32) ((100 * (pixclk % 1000)) / 1000), pll / 1000000);
            i++;
        }
    }

    ftpmu010_write_reg(lcd_fd, 0xB0, org_val, 0x1 << 15);

    return pll / (div + 1);
}

/* this function will be called in initialization state or module removed */
void platform_pmu_clock_gate_8136(int lcd_idx, int on_off, int use_ext_clk)
{
    if (on_off) {
        /* ON
         */
        ftpmu010_write_reg(lcd_fd, 0xB0, 0x0 << 15, 0x1 << 15); /* AXIMCLKOFF: LCD Gate */
    } else {
        /* OFF
         */
        ftpmu010_write_reg(lcd_fd, 0xB0, 0x1 << 15, 0x1 << 15); /* AXIMCLKOFF: LCD Gate */
    }
}

/*
 * mode: VOT_NTSC, VOT_PAL
 * input: 2 : 656in
 *        6 : colorbar output for test
 */
int platform_tve100_config(int lcd_idx, int mode, int input)
{
    u32 tmp;

    if ((mode != VOT_NTSC) && (mode != VOT_PAL)) {
        printk("%s, error 1 \n", __func__);
        return -EINVAL;
    }

    if ((input != 2) && (input != 6)) {  /* color bar */
        printk("%s, error 2 \n", __func__);
        return -EINVAL;
    }

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

#if 0/*SARADC will take care it*/
    /* turn on ADDA wrapper
     */
    /* 1. video_dac_stby low */
    tmp = ioread32(ADDA_WRAPPER_BASE + 0x14);
    tmp &= ~(0x1 << 3);
    iowrite32(tmp, ADDA_WRAPPER_BASE + 0x14);
    /* 2. PD, PSW_PD, ISO_ENABLE */
    tmp &= ~(0x7);
    iowrite32(tmp, ADDA_WRAPPER_BASE + 0x14);
#endif

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

    type = GM8136;

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

    lcd_fd = ftpmu010_register_reg(&pmu_reg_info_8136);
    if (lcd_fd < 0)
        panic("lcd register pmu fail!");

    TVE100_BASE = (unsigned int)ioremap_nocache(TVE_FTTVE100_PA_BASE, TVE_FTTVE100_PA_SIZE);
    if (!TVE100_BASE) {
        printk("LCD: No virtual memory1! \n");
        return -1;
    }

    ADDA_WRAPPER_BASE = (unsigned int)ioremap_nocache(ADDA_WRAP_PA_BASE, ADDA_WRAP_PA_SIZE);
    if (!ADDA_WRAPPER_BASE) {
        printk("LCD: No virtual memory2! \n");
        return -1;
    }

    return 0;
}

unsigned int Platform_Get_FB_AcceID(LCD_IDX_T lcd_idx , int fb_id)
{
    if (lcd_idx == LCD_ID && fb_id == 1)
        return 0x54736930;

    return FB_ACCEL_NONE;
}


int platform_set_dac_onoff(int lcd_idx, int status)
{
    if (lcd_idx < 0 || lcd_idx >= LCD_IP_NUM)
        return -1;

    /* no implement */
    return -1;
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
    __iounmap((void *)TVE100_BASE);
    TVE100_BASE = 0;
    __iounmap((void *)ADDA_WRAPPER_BASE);
    ADDA_WRAPPER_BASE = 0;

    if (platform_cb->exit_fn)
        platform_cb->exit_fn();

    /* de-register pmu */
    ftpmu010_deregister_reg(lcd_fd);

    return 0;
}

void platform_set_lcd_vbase(int idx, unsigned int va)
{
    if (idx >= LCD_IP_NUM)
        panic("%s \n", __func__);

    lcd_va_base[idx] = va;
}

void platform_set_lcd_pbase(int idx, unsigned int pa)
{
    if (idx >= LCD_IP_NUM)
        panic("%s \n", __func__);

    lcd_pa_base[idx] = pa;
}

int platform_get_osdline(void)
{
    return 0;
}

EXPORT_SYMBOL(platform_cb);
EXPORT_SYMBOL(platform_lock_ahbbus);
EXPORT_SYMBOL(platform_set_lcd_vbase);
EXPORT_SYMBOL(platform_set_lcd_pbase);
EXPORT_SYMBOL(platform_get_osdline);
EXPORT_SYMBOL(platform_get_compress_ipbase);
EXPORT_SYMBOL(platform_get_decompress_ipbase);
