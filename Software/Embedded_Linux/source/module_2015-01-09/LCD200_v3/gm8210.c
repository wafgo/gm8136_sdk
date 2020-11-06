#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/platform/board.h>
#include <mach/platform/platform_io.h>
#include "ffb.h"
#include "proc.h"
#include "debug.h"
#include "dev.h"
#include "codec.h"
#include "lcd_def.h"
#include "platform.h"
#include <mach/ftpmu010.h>
#include <mach/ftpmu010_pcie.h>
#include <mach/fmem.h>

#define ALL_USE_PLL3

/* index
 */
#define GM8210      0

#define SYS_CLK         (platform_cb->sys_clk_base)
#define MAX_SCALAR_CLK  (platform_cb->max_scaler_clk)
#define HZ_TO_MHZ(x)    ((x) / 1000000)

static int lcd_fd = 0, pcie_lcd_fd = 0;
static unsigned int TVE100_BASE = 0;
static u32 lcd_va_base[LCD_MAX_ID], lcd_pa_base[LCD_MAX_ID];
static u32 dac_onoff[LCD_IP_NUM] = {LCD_SCREEN_OFF, LCD_SCREEN_OFF, LCD_SCREEN_OFF}; /* 1 for ON, 0 for OFF */

static struct proc_dir_entry *vga_proc = NULL;  /* analog proc */

static LCD_IDX_T  iolink1_vga = LCD1_ID;

/* Define the input resolution supporting list. Please fill the input resoluiton here.
 * If the resultion is not defined here, they will not be suppported!
 * Note: the array must be terminated with VIN_NONE.
 */
static VIN_RES_T res_support_list[LCD_IP_NUM][30] = {
    /* LCD0 */
    {VIN_NTSC, VIN_PAL, VIN_640x480, VIN_800x600, VIN_1024x768, VIN_1280x800, VIN_1280x960, VIN_720P,
    VIN_1280x1024, VIN_1680x1050, VIN_1920x1080, VIN_1440x960, VIN_1280x1080, VIN_1920x1152, VIN_NONE},

    /* LCD1 */
    {VIN_NTSC, VIN_PAL, VIN_640x480, VIN_800x600, VIN_1024x768, VIN_1280x800, VIN_1280x960, VIN_720P,
    VIN_1280x1024, VIN_1680x1050, VIN_1920x1080, VIN_1440x960, VIN_1280x1080, VIN_1920x1152, VIN_NONE},

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

static u32  compress_pa_ipbase[LCD_IP_NUM] = {GRAPHIC_ENC_PA_BASE, GRAPHIC_ENC_PA_BASE, GRAPHIC_ENC_PA_BASE};
static u32  decompress_pa_ipbase[LCD_IP_NUM] = {GRAPHIC_DEC_0_PA_BASE, GRAPHIC_DEC_1_PA_BASE, GRAPHIC_DEC_2_PA_BASE};

/*
 * GM8210 PMU registers
 */
/* offset, bits_mask, lock_bits, init_val, init_mask */
static pmuReg_t pmu_reg_8210[] = {
    /* panel0pixclk_sel choose PLL3, panel1pixclk_sel choose pll1out1_div1, LCD2 chooses pll1out1_div2.
     * [13:12] decides the clock tree of LCDC2.
     */
#ifdef ALL_USE_PLL3
    {0x28, 0xBFF << 4, 0xBFF << 4, (0x0 << 15) | (0x1 << 12) | (0x8 << 8) | (0x0 << 4), 0xBFF << 4},
#else
    {0x28, 0xBFF << 4, 0xBFF << 4, (0x0 << 15) | (0x0 << 12) | (0x8 << 8) | (0x0 << 4), 0xBFF << 4},
#endif
    /* [10:4] : pll3ns (CKOUT = FREF*NS/2), default value is 0x63 */
    {0x34, (0x7F << 4) | 0x1, (0x7F << 4), 0x0, 0x0},
    /* Driving Capacity of IOLINK1 and IOLINK2( [3]-slew rate, 1-slow, [2:1] 00-4mA, 01-8-mA, 10-12mA, 11-16mA, [0] reserved) */
    {0x38, (0xFFFF << 16), (0xFFFF << 16), (0x3 << 29) | (0x3 << 25) | (0x3 << 21) | (0x3 << 17), (0xFFFF << 16)},
    /* lcd2pixclk_pvalue (lcd2pixclk = after reg28[13:12] setection/([29:24]+1))
     * lcd1 output pinmux with capture4-7 of bit20-23
     */
    {0x74, (0x3F << 24) | (0xF << 20), 0x3F << 24, 0, 0},
    /* lcd0/1 pixel pvalue and scaler pvalue*/
    {0x78, (0x1FFFF << 12), (0x1FFFF << 12), 0, 0},
    /* [1:0] : IOLINK_1 input select, 00 : LCDC1 LCD out, 01 : LCDC1 1120/656 out, 10 : LCDC0 LCD out, 11 : LCDC0 1120/656 out
     * [26] : iolink_tx_0 input select, 0 : LCDC0 LCD out, 1:LCDC0 1120/656 out
     * [27] : iolink_tx_0 bypassL, [28] : iolink_tx_0 bypassH. [29] : iolink_tx_1 bypassL, [30] : iolink_tx_1 bypassH
     * [6:4] :iolink2, iolink1, iolink0  output enable, 0: enable, 1: disabled
     * [10:8] : iolink clk driving capacity, [10] slew rate 1-slow, [9:8] 00-4mA, 01-8mA, 10-12mA, 11-16mA
     * [13:11] : iolink data driving capacity, [13] slew rate 1-slow, [12:11] 00-4mA, 01-8mA, 10-12mA, 11-16mA
     */
    {0x7C, (0x1F << 26) | (0x3F << 8) | (0x7 << 4) | 0x3, (0x1 << 26) | (0x3F << 8) | (0x7 << 4) | 0x3, (0x3 << 8) | (0x7 << 4) | (0x3 << 11), (0x1F << 26) | (0x3F << 8) | (0x7 << 4) | 0x3},
    /* aximclkoff : 1-gating clock for lcd/0/1/2 and graphic encoder */
    {0xB0, (0x1 << 23) | (0x7 << 16), (0x1 << 23) | (0x7 << 16), (0x0 << 23) | (0x0 << 16), (0x1 << 23) | (0x7 << 16)},
    /* apbmclkoff0 for lcd0/1/2, graphic decoder 0/1/2 */
    {0xB8, (0x3F << 26), (0x3F << 26), (0x0 << 26), (0x3F << 26)},
    /* rlc encoder */
    {0xBC, (0x1 << 5), (0x1 << 5), (0x0 << 5), (0x1 << 5)},
};

static pmuRegInfo_t pmu_reg_info_8210 = {
    "LCD_8210",
    ARRAY_SIZE(pmu_reg_8210),
    ATTR_TYPE_NONE, //will decide later
    &pmu_reg_8210[0]
};

#ifdef CONFIG_GM8312
/*
 * GM8312 PMU registers
 */
/* offset, bits_mask, lock_bits, init_val, init_mask */
static pmuPcieReg_t pmu_reg_8312[] = {
    /* GM8312 HDMI_HCLK_OFF, 0: on, TVE_HCLK_OFF */
    {0x30, (0x1 << 7) | (0x1 << 9), (0x1 << 7) | (0x1 << 9), 0x0, (0x1 << 7) | (0x1 << 9)},
    /* GM8312 VIDEO_CTRL0 */
    {0x40, (0x3 << 5) | (0xFF << 24), (0x3 << 5) | (0xFF << 24), 0x0, 0x0},
    /* GM8312 VIDEO_CTRL1, 2 VGA DAC and CVBS on */ /* 0: 4ma, 1:8ma, 2:12ma, 3:16ma.  HS,VS output pad driving control
     * R,G,B is swaped due to bug in GM8310.
     */
    {0x44, 0x3FFFFF, 0x3FFFFF, (0x1 << 21) | (0x1 << 19) | (0x0 << 16) | (0x1 << 13) | 0x924, (0x3F << 16) | (0x3 << 13) | 0xFFF},
    /* HDMI_CTRL */
    {0x88, 0xFF, 0xFF, 0x97, 0xFF}
};

static pmuPcieRegInfo_t pmu_reg_info_8312 = {
    "LCD_8312",
    ARRAY_SIZE(pmu_reg_8312),
    ATTR_TYPE_PCIE_NONE,
    &pmu_reg_8312[0]
};
#endif /* CONFIG_GM8312 */

/*
 * Local function declaration
 */
void platform_pmu_switch_pinmux_gm8210(void *ptr, int on);
unsigned int platform_pmu_calculate_clk(int lcd200_idx, u_long pixclk, unsigned int b_Use_Ext_Clk);
void platform_pmu_clock_gate(int lcd_idx, int on_off, int use_ext_clk);
int platform_tve100_config(int lcd_idx, int mode, int input);
int platform_check_input_resolution(int lcd_idx, VIN_RES_T resolution);
int platfrom_8210_init(void);
void platfrom_8210_exit(void);

/*
 * Local variable declaration
 */

#define DIFF(x,y)   ((x) > (y) ? (x) - (y) : (y) - (x))


/* pmu callback functions
 */
static struct callback_s callback[] = {
    {
         .index = GM8210,
         .name = "GM8210",
         .init_fn = platfrom_8210_init,
         .exit_fn = platfrom_8210_exit,
         .sys_clk_base = 12000000,  //12M
         .max_scaler_clk = (200 * 1000000UL),       //130M
         .switch_pinmux = platform_pmu_switch_pinmux_gm8210,
         .pmu_get_clk = platform_pmu_calculate_clk,
         .pmu_clock_gate = platform_pmu_clock_gate,
         .tve_config = platform_tve100_config,
         .check_input_res = platform_check_input_resolution,
     }
};

/* callback funtion structure
 */
struct callback_s *platform_cb = NULL;

unsigned int Platform_Get_FB_AcceID(LCD_IDX_T lcd_idx , int fb_id)
{
    if(lcd_idx|fb_id){}

    return FB_ACCEL_NONE;
}


void platform_pmu_switch_pinmux_gm8210(void *ptr, int on)
{
    struct lcd200_dev_info *info = ptr;
    struct ffb_dev_info *fdinfo = (struct ffb_dev_info *)info;
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    if (!on)
        return; //do nothing

    fmem_get_identifier(&pci_id, &cpu_id);

    switch (info->lcd200_idx) {
      case LCD_ID:
        /* [1:0] : IOLINK_1 input select, 00 : LCDC1 LCD out, 01 : LCDC1 1120/656 out, 10 : LCDC0 LCD out, 11 : LCDC0 1120/656 out
         * [26] : iolink_tx_0 input select, 0 : LCDC0 LCD out, 1:LCDC0 1120/656 out
         */
        if (fdinfo->video_output_type >= VOT_CCIR656) {
            /* 656 or BT1120, clk 4ma, [30]:iolink_tx_1 bypassH, 1 */
            ftpmu010_write_reg(lcd_fd, 0x7C, 0x17 << 26 | 0x2 << 11 | 0x3, 0x1F << 26 | 0x3F << 8 | 0x3);
#ifdef CONFIG_GM8312
            /* HDMI and VGA dac0 source selection.0: from dual edge iolink_rx_0.
             * 1: from single edge input, RGB888 format. 2: from single edge input, BT1120 format.
             */
            if (pci_id == FMEM_PCI_HOST)
                ftpmu010_pcie_write_reg(pcie_lcd_fd, 0x40, 0x2 << 24, 0x3 << 24);
#endif
        } else {
            // RGB
            ftpmu010_write_reg(lcd_fd, 0x7C, 0x1B << 8, 0x1F << 26 | 0x3F << 8 | 0x3);

#ifdef CONFIG_GM8312
            /* HDMI and VGA dac0 source selection.0: from dual edge iolink_rx_0.
             * 1: from single edge input, RGB888 format. 2: from single edge input, BT1120 format.
             */
            if (pci_id == FMEM_PCI_HOST)
                ftpmu010_pcie_write_reg(pcie_lcd_fd, 0x40, 0x0 << 24, 0x3 << 24);
#endif
        }
        break;

      case LCD1_ID:
        /*
         * [1:0] : IOLINK_1 input select, 00 : LCDC1 LCD out, 01 : LCDC1 1120/656 out,
         *    10 : LCDC0 LCD out, 11 : LCDC0 1120/656 out
         */
        switch (iolink1_vga) {
          case LCD_ID:
            /* VGA DAC content is from LCDC0 */
            if (fdinfo->video_output_type >= VOT_CCIR656)
                ftpmu010_write_reg(lcd_fd, 0x7C, 0x3, 0x3);
            else //rgb
                ftpmu010_write_reg(lcd_fd, 0x7C, 0x2, 0x3);
            break;

          case LCD1_ID:
            /* VGA DAC content is from LCDC1 */
            if (fdinfo->video_output_type >= VOT_CCIR656)
                ftpmu010_write_reg(lcd_fd, 0x7C, 0x1, 0x3);
            else //rgb
                ftpmu010_write_reg(lcd_fd, 0x7C, 0x0, 0x3);

            break;

          default:
            panic("%s, wrong source %d \n", __func__, iolink1_vga);
            break;
          }
        break;
    }

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
     * 0x78[23-18] : panel1pixclk_pvalue
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

      case LCD1_ID:
        /* choose PLL3 */
        ftpmu010_write_reg(lcd_fd, 0x28, 0x8 << 4, 0xF << 4);

        /* [23:18] : lcd1pixclk_pvalue (lcd1pixclk = after reg28[7:4] setection/([23:18]+1)) */
        div = (ftpmu010_clock_divisor2(ATTR_TYPE_PLL3, pixclk * 1000, 1) - 1) & 0x3F;
        ftpmu010_write_reg(lcd_fd, 0x78, div << 18, 0x3F << 18);
        pll = ftpmu010_get_attr(ATTR_TYPE_PLL3);
        break;

      case LCD2_ID:
        /* [29:24] : lcd2pixclk_pvalue (lcd2pixclk = after reg28[13:12] setection/([29:24]+1)) */
        div = ftpmu010_clock_divisor2(ATTR_TYPE_PLL1, pixclk * 1000, 1) / 2;
        div = (div - 1) & 0x3F;
        ftpmu010_write_reg(lcd_fd, 0x74, div << 24, 0x3F << 24);
        pll = ftpmu010_get_attr(ATTR_TYPE_PLL1) / 2;
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

      case LCD1_ID:
        /* [23:18] : lcd1pixclk_pvalue (lcd1pixclk = after reg28[7:4] setection/([23:18]+1)) */
        div = (ftpmu010_clock_divisor2(ATTR_TYPE_PLL1, pixclk * 1000, 1) - 1) & 0x3F;
        ftpmu010_write_reg(lcd_fd, 0x78, div << 18, 0x3F << 18);
        pll = ftpmu010_get_attr(ATTR_TYPE_PLL1);

        /* output pinmux with capture4-7, 1 for LCD, 0 for capture */
        ftpmu010_write_reg(lcd_fd, 0x74, 0xF << 20, 0xF << 20);
        break;

      case LCD2_ID:
        /* [29:24] : lcd2pixclk_pvalue (lcd2pixclk = after reg28[13:12] setection/([29:24]+1)) */
        div = (ftpmu010_clock_divisor2(ATTR_TYPE_PLL1, pixclk * 1000, 1) - 1) & 0x3F;
        ftpmu010_write_reg(lcd_fd, 0x74, div << 24, 0x3F << 24);
        pll = ftpmu010_get_attr(ATTR_TYPE_PLL1)/2;
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
    u32 bit, mask, ofs = 0, apb_ofs = 0, iolink_ofs;
    u32 iolink_base;

    if (use_ext_clk)    {}

    switch (lcd_idx) {
      case LCD_ID:
        ofs = 18;
        apb_ofs = 31;
        iolink_ofs = 4;
        iolink_base = (u32)ioremap_nocache(IOLINK_TX_0_PA_BASE, IOLINK_TX_0_PA_SIZE);
        break;
      case LCD1_ID:
        ofs = 17;
        apb_ofs = 30;
        iolink_ofs = 5;
        iolink_base = (u32)ioremap_nocache(IOLINK_TX_0_PA_BASE, IOLINK_TX_0_PA_SIZE);
        break;
      case LCD2_ID:
        ofs = 16;
        apb_ofs = 29;
        iolink_ofs = 6;
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
        ftpmu010_write_reg(lcd_fd, 0x7C, 0x0 << iolink_ofs, 0x1 << iolink_ofs);
    } else {
        bit = (0x1 << ofs);
        mask = (0x1 << ofs);
        ftpmu010_write_reg(lcd_fd, 0xB0, bit, mask);   //turn off gate clock

        bit = (0x1 << apb_ofs);
        mask = (0x1 << apb_ofs);
        ftpmu010_write_reg(lcd_fd, 0xB8, bit, mask);   //turn off apb clock
        ftpmu010_write_reg(lcd_fd, 0x7C, 0x1 << iolink_ofs, 0x1 << iolink_ofs);
    }

    platform_set_dac_onoff(lcd_idx, (on_off == 1) ? LCD_SCREEN_ON : LCD_SCREEN_OFF);

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
    int i, type = GM8210;
    u32 tmp, iolink_base;

    if (pcie_lcd_fd) {}

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

    lcd_fd = ftpmu010_register_reg(&pmu_reg_info_8210);
    if (lcd_fd < 0)
        panic("lcd register pmu fail!");

    /* iolink0 */
    iolink_base = (u32)ioremap_nocache(IOLINK_TX_0_PA_BASE, IOLINK_TX_0_PA_SIZE);
    if (iolink_base == 0) panic("%s, no virtual memory! \n", __func__);
    tmp = ioread32(iolink_base) | 0x1;
    iowrite32(tmp, iolink_base);
    __iounmap((void *)iolink_base);

    /* iolink1 */
    iolink_base = (u32)ioremap_nocache(IOLINK_TX_1_PA_BASE, IOLINK_TX_1_PA_SIZE);
    if (iolink_base == 0) panic("%s, no virtual memory! \n", __func__);
    tmp = ioread32(iolink_base) | 0x1;
    iowrite32(tmp, iolink_base);
    __iounmap((void *)iolink_base);

    /* iolink2 */
    iolink_base = (u32)ioremap_nocache(IOLINK_TX_2_PA_BASE, IOLINK_TX_2_PA_SIZE);
    if (iolink_base == 0) panic("%s, no virtual memory! \n", __func__);
    tmp = ioread32(iolink_base) | 0x1;
    iowrite32(tmp, iolink_base);
    __iounmap((void *)iolink_base);

#ifdef CONFIG_GM8312
    if (pci_id == FMEM_PCI_HOST) {
        pcie_lcd_fd = ftpmu010_pcie_register_reg(&pmu_reg_info_8312);
        if (pcie_lcd_fd < 0)
            panic("lcd register pcie_pmu fail!");
    }
#endif /* CONFIG_GM8312 */

    if (platform_cb->init_fn)
        platform_cb->init_fn();

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
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    fmem_get_identifier(&pci_id, &cpu_id);

    if (platform_cb->exit_fn)
        platform_cb->exit_fn();

    __iounmap((void *)TVE100_BASE);

    ftpmu010_deregister_reg(lcd_fd);

#ifdef CONFIG_GM8312
    if (pci_id == FMEM_PCI_HOST)
        ftpmu010_deregister_reg(pcie_lcd_fd);
#endif

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
static int proc_read_vga(char *page, char **start, off_t off, int count,
			  int *eof, void *data)
{
	int len = 0;

	len += sprintf(page, "0:LCD0, 1:LCD1 \n");
	len += sprintf(page + len, "current value = %d\n", iolink1_vga);
	*eof = 1;		//end of file
	*start = page + off;
	len = len - off;
	return len;
}

static int proc_write_vga(struct file *file, const char *buffer,
			   unsigned long count, void *data)
{
	int len = count;
	unsigned char value[20];
	uint tmp, regVal;

	if (copy_from_user(value, buffer, len))
		return 0;
	value[len] = '\0';
	sscanf(value, "%u\n", &tmp);

	if ((tmp != LCD_ID) && (tmp != LCD1_ID)) {
	    printk("Invalid value! \n");
	    return count;
	}

	/* [1:0] : IOLINK_1 input select, 00 : LCDC1 LCD out, 01 : LCDC1 1120/656 out,
	 *         10 : LCDC0 LCD out, 11 : LCDC0 1120/656 out
     */
	if (iolink1_vga == tmp)
	    return count;

	iolink1_vga = tmp;

	regVal = (iolink1_vga == LCD_ID) ? 0x2 : 0x0;
    /* update to SCU */
    ftpmu010_write_reg(lcd_fd, 0x7C, regVal, 0x3);

	return count;
}

int platform_set_dac_onoff(int lcd_idx, int status)
{
    u32 mask, value;
    if (lcd_idx < 0 || lcd_idx >= LCD_IP_NUM)
        return -1;

    if (dac_onoff[lcd_idx] == status)
        return 0;

    switch (lcd_idx) {
      case LCD_ID:
        mask = 1 << 21;
        value = 1 << 21;
        break;
      case LCD1_ID:
        mask = 1 << 19;
        value = 1 << 19;
        break;
      case LCD2_ID:
        mask = 1 << 17;
        value = 1 << 17;
        break;
      default:
        return 0;
        break;
    }

    if (status == LCD_SCREEN_ON)
        value = 0;

#ifdef CONFIG_GM8312
    {
        fmem_pci_id_t   pci_id;
        fmem_cpu_id_t   cpu_id;

        fmem_get_identifier(&pci_id, &cpu_id);

        /* HDMI and VGA dac0 source selection.0: from dual edge iolink_rx_0.
         * 1: from single edge input, RGB888 format. 2: from single edge input, BT1120 format.
         */
        if (pci_id == FMEM_PCI_HOST)
            ftpmu010_pcie_write_reg(pcie_lcd_fd, 0x44, value, mask);
    }
#endif
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

/* Init function
 */
int platfrom_8210_init(void)
{
    int ret = 0;

    /* VGA
	 */
    vga_proc = ffb_create_proc_entry("iolink1_vga", S_IRUGO | S_IXUGO, flcd_common_proc);
    if (vga_proc == NULL)
    {
        panic("Fail to create VGA proc!\n");
		ret = -EINVAL;
		goto exit;
	}
	vga_proc->read_proc = (read_proc_t *) proc_read_vga;
	vga_proc->write_proc = (write_proc_t *) proc_write_vga;

exit:
    return ret;
}

void platfrom_8210_exit(void)
{
    if (vga_proc)
        ffb_remove_proc_entry(flcd_common_proc, vga_proc);

    vga_proc = NULL;
}

EXPORT_SYMBOL(platform_cb);
EXPORT_SYMBOL(platform_lock_ahbbus);
EXPORT_SYMBOL(platform_set_lcd_vbase);
EXPORT_SYMBOL(platform_set_lcd_pbase);
EXPORT_SYMBOL(platform_get_lcd_pbase);
EXPORT_SYMBOL(platform_get_osdline);
EXPORT_SYMBOL(platform_get_compress_ipbase);
EXPORT_SYMBOL(platform_get_decompress_ipbase);
