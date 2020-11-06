/*
 * (C) Copyright 2005
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <config.h>
#include <asm/io.h>

#include "lcd.h"

extern int hdmi_polling_thread(int);

#define LCD2_ENABLE	0	    //configurable
#define COLOR_MANAGEMENT /* Gamma table is enabled */


#define LCD_IDX_T   lcd_idx_t
#define TV_COMPOSITE_NTSC   OUTPUT_CCIR656_NTSC
#define TV_COMPOSITE_PAL    OUTPUT_CCIR656_PAL

struct ffb_timing_info {
	lcd_output_t otype;
    union {
        /* LCD */
        struct {
            u32 pixclock;
            u16 xres;
            u16 yres;
            u16 hsync_len;
            u16 left_margin;         //H Back Porch
            u16 right_margin;        //H Front Porch
            u16 vsync_len;
            u16 upper_margin;        //V Back Porch
            u16 lower_margin;        //V Front Porch
            u16 polarity;            //0x10C Polarity Control Parameter
            u16 panel_type;          //1: 8 bits per channel, 0: 6bits
            u16 panel_param;         //0x200 of LCDC
        } lcd;

        /* CCIR656 */
        struct {
            u32 pixclock;
            u16 xres;
            u16 yres;
            u16 field0_switch;
            u16 field1_switch;
            u16 hblank_len;
            u16 vblank0_len;
            u16 vblank1_len;
            u16 vblank2_len;
            u16 reserved1;
            u16 reserved2;
            u16 reserved3;
        } ccir656;
    } data;
};

struct ffb_timing_info timing_table[] = {
    {OUTPUT_CCIR656_NTSC, {{27000, 720, 480, 4, 266, 134, 22, 23, 0, 0, 0, 0}}},
    {OUTPUT_CCIR656_PAL, {{27000, 720, 576, 1, 313, 140, 22, 25, 2, 0, 0, 0}}},
    {OUTPUT_BT1120_720P, {{74250, 1280, 720, 1, 752, 362, 25, 0, 5, 0, 0, 0}}},
    {OUTPUT_BT1120_1080P, {{148500, 1920, 1080, 1, 1128, 272, 41, 0, 4, 0, 0, 0}}},
    {OUTPUT_VGA_1280x720, {{74250, 1280, 720, 40, 220, 110, 5, 20, 5, 0, 1, 0}}},
    {OUTPUT_VGA_1920x1080, {{148500, 1920, 1080, 44, 148, 88, 5, 36, 4, 0, 1, 0}}},
    {OUTPUT_VGA_1024x768, {{65000, 1024, 768, 136, 160, 24, 6, 29, 3, 3, 1, 0}}},
};

/*************************************************************************************
 *  LCD TIMING
 *************************************************************************************/
#define LCDHTIMING(HBP, HFP, HW, PL)  (((((HBP)-1)&0xFF)<<24)|((((HFP)-1)&0xFF)<<16)| \
				       ((((HW)-1)&0xFF)<<8)|(((((PL)>>4)-1)&0xFF)))
#define LCDVTIMING0(VFP, VW, LF)      (((VFP)&0xFF)<<24|(((VW)-1)&0x3F)<<16|(((LF)-1)&0xFFF))
#define LCDVTIMING1(VBP)              (((VBP)-1)&0xFF)
#define LCDCFG(PLA_PWR, PLA_DE, PLA_CLK, PLA_HS, PLA_VS) (((PLA_PWR)&0x01)<<4| \
							  ((PLA_DE)&0x01)<<3| \
							  ((PLA_CLK)&0x01)<<2| \
							  ((PLA_HS)&0x01)<<1| \
							  ((PLA_VS)&0x01))
#define LCDPARM(PLA_PWR, PLA_DE, PLA_CLK, PLA_HS, PLA_VS)
#define LCDHVTIMING_EX(HBP, HFP, HW, VFP, VW, VBP)  \
            ((((((HBP)-1)>>8)&0x3)<<8) | (((((HFP)-1)>>8)&0x3)<<4) | (((((HW)-1)>>8)&0x3)<<0) | \
            ((((VFP)>>8)&0x3)<<16) | (((((VW)-1)>>8)&0x3)<<12) | (((((VBP)-1)>>8)&0x3)<<20))

/*************************************************************************************
 *  CCIR656 TIMING
 *************************************************************************************/
#define CCIR656CYC(H,V)             (((V)&0xFFF)<<12|((H)&0xFFF))
#define CCIR656FIELDPLA(F1_0,F0_1)  (((F0_1)&0xFFF)<<12|((F1_0)&0xFFF))
#define CCIR656VBLANK01(VB0,VB1)    (((VB1)&0x3FF)<<12|((VB0)&0x3FF))
#define CCIR656VBLANK23(VB2,VB3)    (((VB3)&0x3FF)<<12|((VB2)&0x3FF))
#define CCIR656VACTIVE(VA0,VA1)     (((VA1)&0xFFF)<<12|((VA0)&0xFFF))
#define CCIR656HBLANK01(HB0,HB1)    (((HB1)&0x3FF)<<12|((HB0)&0x3FF))
#define CCIR656HBLANK2(HB2)         (((HB2)&0x3FF))
#define CCIR656HACTIVE(HA)          (((HA)&0xFFF))
#define CCIR656VBLANK45(VB4,VB5)    (((VB5)&0x3FF)<<12|((VB4)&0x3FF))
#define CCIR656VBI01(VBI_0, VBI_1)  (((VBI_1)&0x3FF)<<12|((VBI_0)&0x3FF))

#define CCIR656_OUTFMT_MASK      1
#define CCIR656_OUTFMT_INTERLACE 0
#define CCIR656_OUTFMT_PROGRESS  1

#define PMU_BASE            CONFIG_PMU_BASE
#define LCD0_BASE			CONFIG_LCDC_BASE
#define LCD2_BASE           CONFIG_LCDC2_BASE
#ifdef CONFIG_PLATFORM_GM8210
#define PCIE_PMU_BASE       CONFIG_8312_PMU_BASE
#define IOLINK0_TX          CONFIG_IOLINK_TX_0_BASE
#else
#define PCIE_PMU_BASE       0
#define IOLINK0_TX          0
#endif

/* logo base */
#define FRAME_BUFFER_BASE	CONFIG_VIDEO_FB_BASE    //configurable

/* lcdc iobase */
static u32 lcd_io_base[3] = {LCD0_BASE, 0, LCD2_BASE};
static u32 lcd_frame_base[3] = {FRAME_BUFFER_BASE, 0, FRAME_BUFFER_BASE + (8 << 20)};  //5M, configurable
static int lcd_init_ok[3] = {0, 0, 0};

static lcd_output_t g_output[3] = {OUTPUT_VGA_1920x1080, 0, OUTPUT_CCIR656_NTSC};   //configurable
static lcd_vin_t    g_vin[3] = {VIN_1920x1080, 0, VIN_D1_NTSC};                     //configurable
static input_fmt_t  g_infmt[3] = {INPUT_FMT_RGB565, INPUT_FMT_RGB565, INPUT_FMT_RGB565};    //configurable

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif

#define printk	printf
#define LCD_IO_BASE(x)  lcd_io_base[(x)]
#define PIP1_FB_OFFSET  1920*1080*4             //configurable

int platform_tve100_config(int lcd_idx, int mode, int input);

int drv_video_init(void)
{
    return 0;
}

/*************************************************************************************
 *  LCD210 IO Read/Write
 *************************************************************************************/
#define ioread32        readl
#define iowrite32       writel

/*************************************************************************************
 *  LCD210 IO Read/Write
 *************************************************************************************/

typedef struct {
    int   hor_no_in;      // actual horizontal resoultion, note: don't decrease 1
    int   ver_no_in;      // actual vertical resoultion, note: don't decrease 1
    int   hor_no_out;     // actual horizontal resoultion = hor_no_out
    int   ver_no_out;     // actual vertical resoultion = ver_no_out
    int   hor_inter_mode;
    int   ver_inter_mode;
    int   g_enable;       // global enable/disable
} lcd210_scalar_t;

/*************************************************************************************
 *  LCD210 BODY
 *************************************************************************************/
struct ffb_timing_info	*get_timing(lcd_output_t out_idx)
{
	int	i, idx = out_idx & 0xFFFF;

	for (i = 0; i < ARRAY_SIZE(timing_table); i ++) {
		if (timing_table[i].otype == idx)
			return &timing_table[i];
	}

	printk("The timing table: %d doesn't exist! \n", out_idx);
	for (;;);

	return NULL;
}

void ftpmu010_write_reg(int offset, unsigned int val, unsigned int mask)
{
    unsigned int tmp, base = PMU_BASE + offset;

    if (val & ~mask)
        for (;;) {
            printf("ftpmu010_write_reg: offset: 0x%x, val: 0x%x mask: 0x%x \n", offset, val, mask);
        } //bug

    tmp = ioread32(base) & ~mask;
    tmp |= (val & mask);
    iowrite32(tmp, base);
}

void ftpmu010_pcie_write_reg(int offset, unsigned int val, unsigned int mask)
{
    unsigned int tmp, base = PCIE_PMU_BASE + offset;

    if (val & ~mask)
        for (;;) {
            printf("ftpmu010_pcie_write_reg: offset: 0x%x, val: 0x%x mask: 0x%x \n", offset, val, mask);
        } //bug

    tmp = ioread32(base) & ~mask;
    tmp |= (val & mask);
    iowrite32(tmp, base);
}

int lcd210_pinmux_init(LCD_IDX_T lcd_idx, lcd_output_t out_idx)
{
    static int  pmu_init_done[3] = {0, 0, 0};
    struct ffb_timing_info *timing;
    unsigned int tmp, pll3 = 594000000, pll1_div2 = 594000000;
    int ret = -1;

    if (pmu_init_done[lcd_idx])
        return 0;

    pmu_init_done[lcd_idx] = 1;

#ifdef CONFIG_PLATFORM_GM8287
    /* [10:4] : pll3ns (CKOUT = FREF*NS/2), default value is 0x63 */
    timing = get_timing(out_idx);
    if (lcd_idx == LCD0_ID) {
        if ((timing->data.lcd.pixclock == 74250) || (timing->data.lcd.pixclock == 148500)) {
            ftpmu010_write_reg(0x34, 99 << 4, 0x7F << 4);   /* CKOUT = FREF*NS/2 */
            pll3 = 594000000;
            ret = 0;
        } else if (timing->data.lcd.pixclock == 65000) {
            ftpmu010_write_reg(0x34, 108 << 4, 0x7F << 4);
            pll3 = 648000000;
            ret = 0;
        }
        /* panel0pixclk_sel choose PLL3 */
        ftpmu010_write_reg(0x28, 0x8 << 8, 0xF << 8);
        ftpmu010_write_reg(0x7C, 0x0, (0x1F << 21));
        /* aximclkoff */
        ftpmu010_write_reg(0xB0, (0x0 << 18), (0x1 << 18));

        /* calculate scaler div */
        tmp = (((pll3 + 100000000) / 200000000) - 1) & 0x1F;
        ftpmu010_write_reg(0x78, tmp << 24, 0x1F << 24);
        /* calculate pixel div, pixel clock: 0x78[17-12] : panel0pixclk_pvalue */
        tmp = (((pll3 + (timing->data.lcd.pixclock * 1000 >> 1)) / (timing->data.lcd.pixclock * 1000)) - 1) & 0x3F;
        ftpmu010_write_reg(0x78, tmp << 12, 0x3F << 12);

        /* gate clock */
        ftpmu010_write_reg(0xb0, 0, 0x1 << 18);
        ftpmu010_write_reg(0xb8, 0, 0x1 << 31);
    }
    if (lcd_idx == LCD2_ID) {
        #define ADC_WRAP_1_PA_BASE	0x98F00000
        unsigned int div = 12000000;

        /* panel2pixclk_sel choose extclk_sel: 01-pll1out1_div2, 10-pll4out2/pll4out1_div2(reg28[3]), 11-pll3 */
        ftpmu010_write_reg(0x28, 0x3 << 12, 0x3 << 12);
        /* aximclkoff */
        ftpmu010_write_reg(0xB0, (0x0 << 16), (0x1 << 16));

        /* calculate pixel div, pixel clock: 0x74[29-24] : panel2pixclk_pvalue */
        tmp = (((pll3 + (timing->data.ccir656.pixclock * 1000 >> 1)) / (timing->data.ccir656.pixclock * 1000)) - 1) & 0x3F;
        ftpmu010_write_reg(0x74, tmp << 24, 0x3F << 24);

        printf("tmp = %d \n", tmp);

        /* lcd2 gate clock */
        ftpmu010_write_reg(0xb8, 0, 0x1 << 29);
        /* tve gate clock */
        ftpmu010_write_reg(0xb4, 0, 0x1 << 31);

        tmp = ioread32(ADC_WRAP_1_PA_BASE);
        tmp &= ~((0xFF << 16) | 0x1F);
        tmp |= ((0xA << 16) | (0x7 << 3) | 0x01); /* bit5: ADC normal operating mode */
        iowrite32(tmp, ADC_WRAP_1_PA_BASE);
        div = div / (760*1000);
        if (div * 760*1000 != 12000000)
            div += 1;   //can't over 760K
        ftpmu010_write_reg(0x60, div, 0x3F);
        /* sar adc */
        ftpmu010_write_reg(0xE0, 0x7 << 20 | 0x0 << 16 | 0x7 << 5, 0xF << 20 | 0x1 << 16 | 0xF << 5);
        ftpmu010_write_reg(0x7C, 0x0 << 21, 0x1F << 21);

        ret = 0;
    }
#endif /* CONFIG_PLATFORM_GM8287 */

#ifdef CONFIG_PLATFORM_GM8210
    /* [10:4] : pll3ns (CKOUT = FREF*NS/2), default value is 0x63 */
    timing = get_timing(out_idx);

    if (lcd_idx == LCD0_ID) {
        if ((timing->data.lcd.pixclock == 74250) || (timing->data.lcd.pixclock == 148500)) {
            ftpmu010_write_reg(0x34, 99 << 4, 0x7F << 4);   /* CKOUT = FREF*NS/2 */
            pll3 = 594000000;
            ret = 0;
        } else if (timing->data.lcd.pixclock == 65000) {
            ftpmu010_write_reg(0x34, 108 << 4, 0x7F << 4);
            pll3 = 648000000;
            ret = 0;
        }

        /* panel0pixclk_sel choose PLL3 */
        ftpmu010_write_reg(0x28, 0x8 << 8, 0xF << 8);
        ftpmu010_write_reg(0x38, (0x3 << 29) | (0x3 << 25) | (0x3 << 21) | (0x3 << 17), (0xFFFF << 16));
        ftpmu010_write_reg(0x7C, (0x3 << 11) | (0x3 << 8), (0x1F << 26) | (0x3F << 8) | (0x1 << 4) | 0x3);
        /* aximclkoff */
        ftpmu010_write_reg(0xB0, (0x0 << 18), (0x1 << 18));

        /* calculate scaler div */
        tmp = (((pll3 + 100000000) / 200000000) - 1) & 0x1F;
        ftpmu010_write_reg(0x78, tmp << 24, 0x1F << 24);
        /* calculate pixel div, pixel clock: 0x78[17-12] : panel0pixclk_pvalue */
        tmp = (((pll3 + (timing->data.lcd.pixclock * 1000 >> 1)) / (timing->data.lcd.pixclock * 1000)) - 1) & 0x3F;
        ftpmu010_write_reg(0x78, tmp << 12, 0x3F << 12);

        /* gate clock */
        ftpmu010_write_reg(0xb0, 0, 0x1 << 18);
        ftpmu010_write_reg(0xb8, 0, 0x1 << 31);

        /* iolink0 */
        tmp = ioread32(IOLINK0_TX) | 0x1;
        iowrite32(tmp, IOLINK0_TX);

        /* pci-e
         */
        /* GM8312 HDMI_HCLK_OFF, 0: on, TVE_HCLK_OFF */
        ftpmu010_pcie_write_reg(0x30, 0x0, (0x1 << 7) | (0x1 << 9));
        ftpmu010_pcie_write_reg(0x44, (0x0 << 21) | (0x0 << 19) | (0x0 << 16) | (0x1 << 13) | 0x924, (0x3F << 16) | (0x3 << 13) | 0xFFF),
        ftpmu010_pcie_write_reg(0x88, 0x97, 0xFF);
    }

    if (lcd_idx == LCD2_ID) {
        #define IOLINK_TX_2_PA_BASE         0x9BD00000

        /* panel2pixclk_sel choose extclk_sel: 01-pll1out1_div2 */
        ftpmu010_write_reg(0x28, 0x1 << 12, 0x3 << 12);
        ftpmu010_write_reg(0x7C, 0x0 << 6, 0x1 << 6);
        /* aximclkoff */
        ftpmu010_write_reg(0xB0, (0x0 << 16), (0x1 << 16));

        /* calculate pixel div, pixel clock: 0x74[29-24] : panel2pixclk_pvalue */
        tmp = (((pll1_div2 + (timing->data.ccir656.pixclock * 1000 >> 1)) / (timing->data.ccir656.pixclock * 1000)) - 1) & 0x3F;
        ftpmu010_write_reg(0x74, tmp << 24, 0x3F << 24);

        /* gate clock */
        ftpmu010_write_reg(0xb8, 0, 0x1 << 29);
        /* iolink2 */
        tmp = ioread32(IOLINK_TX_2_PA_BASE) | 0x1;
        iowrite32(tmp, IOLINK_TX_2_PA_BASE);

        ret = 0;
    }
#endif /* CONFIG_PLATFORM_GM8210 */

#if defined(CONFIG_PLATFORM_GM8136) || defined(CONFIG_PLATFORM_GM8139)
    if (1) {
        extern uint u32PMU_ReadPLL3CLK(void);
        #define ADDA_WRAP_PA_BASE	0x90B00000
        u32 tmp_mask, tmp, max_scaler_clk = 1485 * 100000UL, div;

        timing = get_timing(out_idx);

        /* LCD scaler CLK, 1'b0-lcd_scarclk_cntp; 1'b1-lcd_pixclk_cntp
         * LCD CLK, 1'b0-PLL3 (594 or 540MHz); 1'b1-PLL2 (600MHz)
         */
        ftpmu010_write_reg(0x28, 0x0 << 8, 0x3 << 8);
        /* [13:8] LCD scaler clock divided value, [5:0] LCD pixel clock divided value */
        ftpmu010_write_reg(0x78, 0x0, 0x0);
#ifdef CONFIG_PLATFORM_GM8136
        /* TV_PCLK: clock inverse */
        ftpmu010_write_reg(0x7C, 0x0, 0x0);
#else
        ftpmu010_write_reg(0x7C, 0x1 << 26, 0x1 << 26);
#endif
        /* AXIMCLKOFF: LCD Gate, GPENC */
        ftpmu010_write_reg(0xB0, 0x0 << 12, 0x1 << 12);
        /* TVE  (reg) */
        ftpmu010_write_reg(0xB4, 0, (0x1 << 20));
        /* adda_wrapper */
        ftpmu010_write_reg(0xB8, 0x0, 0x1 << 6);
        /* APBMCLKOFF: LCD Gate */
        ftpmu010_write_reg(0xBC, 0x0 << 15, 0x7 << 15);
        ftpmu010_write_reg(0xB0, 0x0 << 15, 0x1 << 15); /* AXIMCLKOFF: LCD Gate */

        /* platform_pmu_calculate_clk */
        tmp = ioread32(CONFIG_PMU_BASE + 0xB0);
        ftpmu010_write_reg(0xB0, 0x0 << 15, 0x1 << 15);
        /* scaler clock cntp */
        tmp = u32PMU_ReadPLL3CLK();
        tmp += (max_scaler_clk >> 1);  /* take near value */
        div = tmp / max_scaler_clk - 1;
        ftpmu010_write_reg(0x78, div << 8, 0x3F << 8);

        /* Pixel clock cntp */
        tmp = u32PMU_ReadPLL3CLK();
        tmp += (timing->data.lcd.pixclock * 1000 >> 1); /* take near value */
        div = tmp / (timing->data.lcd.pixclock * 1000) - 1;
        ftpmu010_write_reg(0x78, div, 0x3F);

#ifdef CONFIG_PLATFORM_GM8136
        /* pinmux */
        if (out_idx < OUTPUT_VESA) {
            /* TV, CEA standard */
            tmp_mask = 0xFFFFFFFF;
            tmp = 0xFFFFFFFF;   /* 2b11 means X_TV_DATA[x] for pin 0-16*/
            /* 0x5C, 0x64: Digital PAD, TV DATA 0-7,  LC_DATA0-7*/
            ftpmu010_write_reg(0x5C, tmp, tmp_mask);
            /* X_TV_PCLK */
            tmp_mask = 0x3 << 8;
            tmp = 0x3 << 8;
            ftpmu010_write_reg(0x64, tmp, tmp_mask);
        } else {
            /* VGA standard */
            tmp_mask = 0xFFFFFFFF;
            tmp = 0xAAAAAAAA;   /* 2b10 means X_LC_DATA[x] */
            /* 0x5C, 0x64: Digital PAD, TV DATA 0-7,  LC_DATA0-7*/
            ftpmu010_write_reg(0x5C, tmp, tmp_mask);

            /* X_LC_PCLK */
            tmp_mask = 0x3 << 8;
            tmp = 0x3 << 8;
            ftpmu010_write_reg(0x64, tmp, tmp_mask);

            /* X_LC_VS, X_LC_HS */
            tmp_mask = (0x3 << 6) | 0x3;
            tmp = (0x2 << 6) | 0x2;
            ftpmu010_write_reg(0x64, tmp, tmp_mask);
        }
#else
        /* GM8139 */
        /* pinmux */
        if (out_idx < OUTPUT_VESA) {
            /* TV, CEA standard */
            /* BT1120 */
            tmp_mask = 0xFFFFFFFF;
            tmp = (0x4 << 28) | (0x4 << 24) | (0x4 << 20) | (0x4 << 16) | (0x4 << 12) | (0x4 << 8) | (0x4 << 4) | 0x4;
            ftpmu010_write_reg(0x5C, tmp, tmp_mask);
            ftpmu010_write_reg(0x60, tmp, tmp_mask);
            /* LC_PCLK, TV_PCLK */
            tmp_mask = 0xF << 8;
            tmp = 0x4 << 8;
            ftpmu010_write_reg(0x64, tmp, tmp_mask);
        } else {
            /* VGA standard */
            /* RGB888 */
            tmp_mask = 0xFFFFFFFF;
            tmp = (0x3 << 28) | (0x3 << 24) | (0x3 << 20) | (0x3 << 16) | (0x3 << 12) | (0x3 << 8) | (0x3 << 4) | 0x3;
            ftpmu010_write_reg(0x5C, tmp, tmp_mask);
            ftpmu010_write_reg(0x60, tmp, tmp_mask);
            /* LC_PCLK */
            tmp_mask = (0xF << 8) | (0xF << 2);
            tmp = (0x3 << 8) | (0x3 << 4) | ( 0x3 << 2);
            ftpmu010_write_reg(0x64, tmp, tmp_mask);
        }
#endif /* CONFIG_PLATFORM_GM8139 */
    }

    /* turn on ADDA wrapper
     */
    if (g_output[lcd_idx] == OUTPUT_CCIR656_NTSC || g_output[lcd_idx] == OUTPUT_CCIR656_PAL) {
        /* 1. video_dac_stby low */
        tmp = ioread32(ADDA_WRAP_PA_BASE + 0x14);
        tmp &= ~(0x1 << 3);
        iowrite32(tmp, ADDA_WRAP_PA_BASE + 0x14);
        /* 2. PD, PSW_PD, ISO_ENABLE */
        tmp &= ~(0x7);
        iowrite32(tmp, ADDA_WRAP_PA_BASE + 0x14);
    }

    ret = 0;
#endif /* CONFIG_PLATFORM_GM8136/9 */

    return ret;
}

void clear_screen(unsigned char *fb_buf, lcd_output_t out_idx, input_fmt_t fmt)
{
    int     i, byte_per_pixel;
	struct ffb_timing_info *timing;

	timing = get_timing(out_idx);

    if (fmt == INPUT_FMT_YUV422) {
        unsigned int *pbuf = (unsigned int *)fb_buf;

        for (i = 0; i < (timing->data.ccir656.xres * timing->data.ccir656.yres * 2) >> 2; i ++)
            pbuf[i] = 0x10801080;
    } else {
        unsigned char *pbuf = fb_buf;

        byte_per_pixel = (fmt == INPUT_FMT_RGB888) ? 4 : 2;
        memset(pbuf, 0, timing->data.ccir656.xres * timing->data.ccir656.yres * byte_per_pixel);
    }
}

int lcd210_set_framebase(lcd_idx_t lcd_id, u32 framebase)
{
	u32	tmp, pip1_framebase = framebase + PIP1_FB_OFFSET;

    iowrite32(framebase, LCD_IO_BASE(lcd_id) + 0x18);
    iowrite32(pip1_framebase, LCD_IO_BASE(lcd_id) + 0x24);

	tmp = ioread32(LCD_IO_BASE(lcd_id) + 0x4);
	tmp |= (0x1 << 16); /* AddrUpdate */
	iowrite32(tmp, LCD_IO_BASE(lcd_id) + 0x4);

    return 0;
}

/*
 * get LCD frame base
 */
void lcd210_get_framebase(unsigned int *pFrameBuffer, lcd_idx_t lcd_idx)
{
    u32 ip_io_base = LCD_IO_BASE(lcd_idx);

    *pFrameBuffer = *(unsigned int *)(ip_io_base + 0x18);
}

void lcd210_control_en(u32 base, int state)
{
    u32		value;

    value = ioread32(base);
    value &= ~0x1;
    if (state)
        value |= 0x1;
    iowrite32(value, base);

    // clear interrupt status
    iowrite32(0xf, base + 0xC);
    return;
}

void lcd210_set_color_management(u32 io_base)
{
    int           i,  j;
    unsigned char GammaTable[3][256];
    u32           value;
    /* linear */
    for (i = 0; i < 256; i++)
        GammaTable[0][i] = GammaTable[1][i] = GammaTable[2][i] = i;

    /* Gamma table */
    for( j = 0; j < 3; j++)
    {
        for (i=0; i < 256/4; i++)
        {
            value = GammaTable[j][i*4+0] | GammaTable[j][i*4+1] << 8 | GammaTable[j][i*4+2] << 16 | GammaTable[j][i*4+3] << 24;
            iowrite32(value, io_base + 0x600 + (j*0x100) + i*4);
        }
    }

    iowrite32(0x2000, io_base + 0x400);	// saturation
    iowrite32(0x2000, io_base + 0x404);	// hue
    iowrite32(0x00,   io_base + 0x408);	// sharpness, 0x00422008
    iowrite32(0x40000,io_base + 0x40C);	// contrast

    return;
}

int lcd210_scalar(unsigned int lcd_io_base, lcd210_scalar_t *scalar)
{
    u32 scal_hor_num = 0, scal_ver_num = 0, tmp;
    int fir_sel = 0, sec_bypass_mode = 1;
    int lcd_enable;

    if (scalar->hor_no_in > 2047 || scalar->ver_no_in > 2047 || scalar->hor_no_out > 4097 || scalar->ver_no_out > 4097)
        return -1;

	if ((scalar->hor_no_out >> 1) >= scalar->hor_no_in) {
		printk("hor out can't be larger than twice of hor_no_in \n");
		return -1;
	}

	if ((scalar->ver_no_out >> 1) >= scalar->ver_no_in) {
		printk("vertical out can't be larger than twice of ver_no_in \n");
		return -1;
	}

	if ((scalar->hor_no_in == scalar->hor_no_out) && (scalar->ver_no_in == scalar->ver_no_out))
		return 0;	//do nothing

    /* Process first stage parameter
     */
    if ((scalar->hor_no_out >= scalar->hor_no_in) && (scalar->ver_no_out >= scalar->ver_no_in))
    {
        // scaling up, bypass the first stage
        fir_sel = 0;
    }

    /* 1st state is bypass, fill in the coefficients of the 2nd-stage scalar
     */
    sec_bypass_mode = 0;    // 1 for bypass

    /* hor scaling up */
    if (scalar->hor_no_out >= scalar->hor_no_in)
        scal_hor_num = (scalar->hor_no_in + 1) * 256 / scalar->hor_no_out;
    else /* scaling down */
        scal_hor_num = (((scalar->hor_no_in + 1) % scalar->hor_no_out) * 256) / scalar->hor_no_out;

    /* ver scaling up */
    if (scalar->ver_no_out >= scalar->ver_no_in)
        scal_ver_num = (scalar->ver_no_in + 1) * 256 / scalar->ver_no_out;
    else /* scaling down */
        scal_ver_num = (((scalar->ver_no_in + 1) % scalar->ver_no_out) * 256) / scalar->ver_no_out;

    /* Hor_no_in */
    tmp = scalar->hor_no_in & 0xfff;
    iowrite32(tmp-1, lcd_io_base + 0x1100);    // minus 1

    /* Ver_no_in */
    tmp = scalar->ver_no_in & 0xfff;
    iowrite32(tmp-1, lcd_io_base + 0x1104);    // minus 1

    /* Hor_no_out */
    tmp = scalar->hor_no_out & 0xfff;
    iowrite32(tmp, lcd_io_base + 0x1108);

    /* Hor_no_out */
    tmp = scalar->ver_no_out & 0xfff;
    iowrite32(tmp, lcd_io_base + 0x110C);

    /* Miscellaneous Control Register */
    tmp = (fir_sel & 0x7) << 6 | (scalar->hor_inter_mode & 0x3) << 3 | (scalar->ver_inter_mode & 0x3) << 1 | sec_bypass_mode;
    iowrite32(tmp, lcd_io_base + 0x1110);

    /* Scalar Resolution Parameters */
    tmp = (scal_hor_num & 0xff) << 8 | (scal_ver_num & 0xff);
    iowrite32(tmp, lcd_io_base + 0x112c);

    tmp = ioread32(lcd_io_base + 0x0);
    lcd_enable = (tmp & 0x1) ? 1 : 0;
    tmp &= (~(0x1 << 5));
    if (scalar->g_enable)
        tmp |= (0x1 << 5);
    tmp &= ~0x1;    //disable lcd
    iowrite32(tmp, lcd_io_base + 0x0);

    if (lcd_enable)
        iowrite32(tmp | 0x1, lcd_io_base + 0x0);
	return 0;
}

/* the function is for PROGRSSIVE INPUT AND PROGRESSIVE OUT */
void lcd210_set_CEA_standard(int out_idx, u32 lcd_io_base, int bt1120)
{
	u32 value, tmp, h_cycle, v_cycle;
    struct ffb_timing_info *timing;

	timing = get_timing(out_idx);

	if (bt1120)
		value = (bt1120 << 3) | 0x3;    //Input image format is Progress and out is progressive too.
	else
		value = 0x2; /* tve clock phase is 1 */
    iowrite32(value, lcd_io_base + 0x204);

    /* TV Field Polarity Parameters */
    value = CCIR656FIELDPLA(timing->data.ccir656.field0_switch, timing->data.ccir656.field1_switch);
    iowrite32(value, lcd_io_base + 0x20C);

    /* TV Vertical Blank Parameters */
    value = CCIR656VBLANK01(timing->data.ccir656.vblank0_len, timing->data.ccir656.vblank1_len);
    iowrite32(value, lcd_io_base + 0x210);

    /* TV Vertical Blank Parameters */
    value = CCIR656VBLANK23(timing->data.ccir656.vblank2_len, 0);    /* blank3 is zero */
    iowrite32(value, lcd_io_base + 0x214);

    /* TV Vertical Active Parameters */
    tmp = timing->data.ccir656.yres;
    if (bt1120 == 0) { /* CCIR656 */
        tmp = tmp >> 1;
        value = CCIR656VACTIVE(tmp, tmp);
    } else {
        /* progressive */
        value = CCIR656VACTIVE(tmp, 0);
    }
    iowrite32(value, lcd_io_base + 0x218);

    /* TV Horizontal Blank Parameters, H_Blk0/1 */
    h_cycle = timing->data.ccir656.xres + timing->data.ccir656.hblank_len;
    v_cycle = timing->data.ccir656.yres + timing->data.ccir656.vblank0_len +
                        timing->data.ccir656.vblank1_len + timing->data.ccir656.vblank2_len;
    if (bt1120 == 0) /* CCIR656 */
        value = CCIR656HBLANK01(timing->data.ccir656.hblank_len << 1, 0);
    else
        value = CCIR656HBLANK01(timing->data.ccir656.hblank_len, 0);
    iowrite32(value, lcd_io_base + 0x21C);

	/* TV Horizontal Blank Parameters, H_Blk2 */
    value = CCIR656HBLANK2(0);
    iowrite32(value, lcd_io_base + 0x220);

	/* TV Horizontal Active Parameters */
	if (bt1120 == 0) /* CCIR656 */
		value = CCIR656HACTIVE(timing->data.ccir656.xres << 1);
	else
		value = CCIR656HACTIVE(timing->data.ccir656.xres);
	iowrite32(value, lcd_io_base + 0x224);

	/* TV Vertical Blank Parameters */
	value = CCIR656VBLANK45(0, 0);
	iowrite32(value, lcd_io_base + 0x228);

	/* TV Vertical Blank Parameters */
    value = CCIR656VBI01(0, 0);
    iowrite32(value, lcd_io_base + 0x22C);

	if (bt1120 == 0) /* CCIR656 */
		h_cycle <<= 1; /* double */
	value = CCIR656CYC(h_cycle + 8, v_cycle);
	iowrite32(value, lcd_io_base + 0x208);

	/* LCD Horizontal Timing control */
	value = LCDHTIMING(0, 0, 0, timing->data.ccir656.xres);
	iowrite32(value, lcd_io_base + 0x100);

	/* LCD Vertical Timing control */
    value = LCDVTIMING0(0, 0, timing->data.ccir656.yres);
    iowrite32(value, lcd_io_base + 0x104);

	return;
}

void lcd210_set_VESA_standard(int out_idx, u32 lcd_io_base)
{
    struct ffb_timing_info *timing;
    u32 value;

    timing = get_timing(out_idx);

    /* LCD Horizaontal Timing Control Parameter */
    value = LCDHTIMING(timing->data.lcd.left_margin, timing->data.lcd.right_margin,
                        timing->data.lcd.hsync_len, timing->data.lcd.xres);
    iowrite32(value, lcd_io_base + 0x100);

    /* LCD Vertical Timing Control Parameter */
    value = LCDVTIMING0(timing->data.lcd.lower_margin, timing->data.lcd.vsync_len,
                        timing->data.lcd.yres);
    iowrite32(value, lcd_io_base + 0x104);

    /* LCD Vertical Back Porch Parameter */
    value = LCDVTIMING1(timing->data.lcd.upper_margin); /* VBP */
    iowrite32(value, lcd_io_base + 0x108);

    /* LCD Polarity Control Parameter */
    value = timing->data.lcd.polarity;
    iowrite32(value, lcd_io_base + 0x10C);

    /* LCD Serial panel Pixel Parameter */
    value = timing->data.lcd.panel_param;
    iowrite32(value, lcd_io_base + 0x200);

    /* PanelType */
    value = ioread32(lcd_io_base + 0x4);
    value &= ~(0x1 << 11);
    if (timing->data.lcd.panel_type == 1)
        value |= (0x1 << 11);

#if defined(CONFIG_PLATFORM_GM8136) || defined(CONFIG_PLATFORM_GM8139)
    value &= ~(0x1 << 11);  /* 6bits per channel with 1 18-bit panel interface */
#endif

    iowrite32(value, lcd_io_base + 0x4);

    return;
}

int lcd210_pip_enable(int out_idx, u32 lcd_io_base, int pip_mode)
{
    struct ffb_timing_info *timing;
    u32 value;

    if (pip_mode > 1) /* only pip1 */
        return -1;

    if (pip_mode == 0)
        return 0;

    timing = get_timing(out_idx);

    /* set blending to 128 */
    value = (128 << 8) | 128;
    iowrite32(value, lcd_io_base + 0x300);

    /* Pip sub-picture1 Dimension Parameter */
    if (timing->otype >= OUTPUT_VESA) {
        value = (timing->data.lcd.xres << 16) | timing->data.lcd.yres;
        iowrite32(value, lcd_io_base + 0x308);
    } else {
        value = (timing->data.ccir656.xres << 16) | timing->data.ccir656.yres;
        iowrite32(value, lcd_io_base + 0x308);
    }

    /* PiP sub-picture2 Position Parameter */
    iowrite32(0, lcd_io_base + 0x30C);

    /* PiP Image Priority Parameter */
    value = 0x24;   //just copy from 8181
    iowrite32(value, lcd_io_base + 0x314);

    /* Image0: YUV422, Image1: RGB565 */
    value = (0x4 << 4) | (0x1 << 3) | 4;
    iowrite32(value, lcd_io_base + 0x318);

    /* yuv422 when pip is disabled */
    value = ioread32(lcd_io_base + 0x0);
    value |= (0x1 << 3);
    iowrite32(value, lcd_io_base + 0x0);

    /* Image Format2 Parameter */
    iowrite32(0, lcd_io_base + 0x31C);
    /* update pip */
    iowrite32(0x1 << 28, lcd_io_base + 0x304);

    value = ioread32(lcd_io_base + 0x0);
    value &= ~(0xF << 8);   //BlendEn, PiPEn
    value |= (pip_mode << 10);
    if (pip_mode)
        value |= (0x1 << 8);    //BlendEn, for global Alpha Blending
    iowrite32(value, lcd_io_base + 0x0);

    return 0;
}

/*
 * lcd_id: 0/1/2
 * vin: input resolution. Currently it is ingored.
 * lcd_output: combination of lcd_output_t
 */
static int lcd210_init(lcd_idx_t lcd_id, lcd_vin_t vin, u32 lcd_output, input_fmt_t in_fmt)
{
    lcd210_scalar_t scaler;
    struct ffb_timing_info *timing;
    int out_progressive = 0, out_idx = lcd_output & 0xFFFF;
	u32	ip_io_base = LCD_IO_BASE(lcd_id);
	u32	value, CEA = 0;
	int pip_mode = 0;

    if (lcd_id >= ARRAY_SIZE(lcd_io_base))
       return -1;

    if (lcd_init_ok[lcd_id] == 1)
        return 0;

    timing = get_timing(out_idx);

    lcd_init_ok[lcd_id] = 1;

    lcd210_control_en(ip_io_base, 0);
    /* clear the screen */
#if 0
	clear_screen((unsigned char *)lcd_frame_base[lcd_id], timing->otype, in_fmt);
	if (pip_mode)
	    clear_screen((unsigned char *)(lcd_frame_base[lcd_id] + PIP1_FB_OFFSET), timing->otype, INPUT_FMT_RGB565);
#endif
	/* set the frame base */
    lcd210_set_framebase(lcd_id, lcd_frame_base[lcd_id]);

#ifdef COLOR_MANAGEMENT
	/* set color management */
	lcd210_set_color_management(ip_io_base);
#endif

	switch (timing->otype) {
	  case OUTPUT_CCIR656_NTSC:
	  case OUTPUT_CCIR656_PAL:
	    out_progressive = 0;
		CEA = 1;
		lcd210_set_CEA_standard(out_idx, ip_io_base, out_progressive);
		break;
	  case OUTPUT_BT1120_720P:
	  case OUTPUT_BT1120_1080P:
		out_progressive = 1;
		CEA = 1;
		lcd210_set_CEA_standard(out_idx, ip_io_base, out_progressive);
		break;
	  case OUTPUT_VGA_1280x720:
	  case OUTPUT_VGA_1920x1080:
	  case OUTPUT_VGA_1024x768:
		out_progressive = 1;
		CEA = 0;
		lcd210_set_VESA_standard(out_idx, ip_io_base);
		break;
	  default:
	    printf("error in lcd210 init! \n");
	    for (;;);   /* error */
	    break;
	};

	/* set LCD Panel Pixel Parameters */
	value = ioread32(ip_io_base + 0x4);
	value &= ~(0x7);
	if (in_fmt == INPUT_FMT_YUV422) {
		value |= 0x4;
	}
	else if (in_fmt == INPUT_FMT_RGB565) {
		value |= 0x4;
		value |= (0x0 << 7); /* RGB565 input */
	} else if (in_fmt == INPUT_FMT_RGB888) {
		value |= 0x5;	/* 24bpp */
	} else
		value |= 0x3; /* 8bpp */

	value &= ~(0x3 << 9);
	if (out_progressive)
		value |= (0x1 << 9);    /* back porch */
	else
		value |= (0x2 << 9);    /* active image */
	value |= (0x1 << 16);	/* Address update */
	iowrite32(value, ip_io_base + 0x4);

	/* interrupt mask */
    value = (0x1 << 3) | 0x1;
    iowrite32(value, ip_io_base + 0x8);

	value = (0x3 << 16); /* AddrSyn_En and axi 64bit */
	if (CEA == 1)
		value |= (0x1 << 13);	//TV enable
	else
	    value |= (0x1 << 1);    //LCDon
	if (in_fmt == INPUT_FMT_YUV422)
		value |= (0x1 << 3);
	iowrite32(value, ip_io_base + 0x0); /* note: lcdc is still off */

    lcd210_pip_enable(out_idx, ip_io_base, pip_mode);

	lcd210_control_en(ip_io_base, 1);

    g_vin[lcd_id] = vin;
    g_output[lcd_id] = lcd_output;

    /* process scaler */
    memset(&scaler, 0, sizeof(lcd210_scalar_t));
    switch (g_output[lcd_id]) {
      case OUTPUT_CCIR656_NTSC:
        scaler.hor_no_out = 720;
        scaler.ver_no_out = 480;
        break;
      case OUTPUT_CCIR656_PAL:
        scaler.hor_no_out = 720;
        scaler.ver_no_out = 576;
        break;
      case OUTPUT_BT1120_720P:
      case OUTPUT_VGA_1280x720:
        scaler.hor_no_out = 1280;
        scaler.ver_no_out = 720;
        break;
      case OUTPUT_BT1120_1080P:
      case OUTPUT_VGA_1920x1080:
        scaler.hor_no_out = 1920;
        scaler.ver_no_out = 1080;
        break;
      case OUTPUT_VGA_1024x768:
        scaler.hor_no_out = 1024;
        scaler.ver_no_out = 768;
        break;
      default:
        for (;;);
        break;
    };

    switch (g_vin[lcd_id]) {
      case VIN_D1_NTSC:
        scaler.hor_no_in = 720;
        scaler.ver_no_in = 480;
        break;
      case VIN_D1_PAL:
        scaler.hor_no_in = 720;
        scaler.ver_no_in = 576;
        break;
      case VIN_1280x720:
        scaler.hor_no_in = 1280;
        scaler.ver_no_in = 720;
        break;
      case VIN_1920x1080:
        scaler.hor_no_in = 1920;
        scaler.ver_no_in = 1080;
        break;
      case VIN_1024x768:
        scaler.hor_no_in = 1024;
        scaler.ver_no_in = 768;
        break;
      default:
        for(;;) {}
        break;
    }

    if ((scaler.hor_no_out != scaler.hor_no_in) || (scaler.ver_no_out != scaler.ver_no_in)) {
        scaler.g_enable = 1;
        lcd210_scalar(LCD_IO_BASE(lcd_id), &scaler);
    }

	return 0;
}

int flcd210_set_patterngen(lcd_idx_t lcd_idx, int on)
{
    u32	value, ip_io_base = LCD_IO_BASE(lcd_idx);

    if (lcd_idx >= ARRAY_SIZE(lcd_io_base))
        return -1;

    value = ioread32(ip_io_base);
    if (on) {
        value |= (0x1 << 14);
    } else {
        value &= ~(0x1 << 14);
    }

    iowrite32(value, ip_io_base);
    return 0;
}

int platform_tve100_config(int lcd_idx, int mode, int input)
{
    u32 tmp;

    /* only support PAL & NTSC */
    if ((mode != TV_COMPOSITE_NTSC) && (mode != TV_COMPOSITE_PAL))
        return -1;

    if ((input != 2) && (input != 6))   /* color bar */
        return -1;

    //lcd disable case
    if (!(ioread32(lcd_io_base[lcd_idx]) & 0x1))
        return 0;

    /* 0x0 */
    tmp = (mode == TV_COMPOSITE_NTSC) ? 0x0 : 0x2;
    iowrite32(tmp, CONFIG_FTTVE100_BASE + 0x0);

    /* 0x4 */
    iowrite32(input, CONFIG_FTTVE100_BASE + 0x4);

    /* 0x8, 0xc */
    iowrite32(0, CONFIG_FTTVE100_BASE + 0x8);

    /* bit 0: power down, bit 1: standby */
    iowrite32(0, CONFIG_FTTVE100_BASE + 0xc);

    return 0;
}

/* entry point
 */
int flcd_main(LCD_IDX_T lcd_idx, lcd_vin_t vin, lcd_output_t output)
{
    int ret;

    ret = lcd210_pinmux_init(lcd_idx, g_output[lcd_idx]);
    if (ret < 0)
        return -1;

    /* default value */
    ret = lcd210_init(lcd_idx, vin, output, INPUT_FMT_RGB565);
    if (!ret) {
        /* enable pattern generation */
        flcd210_set_patterngen(lcd_idx, 1);
    }

    return ret;
}

/* this function is portable for the customer */
int do_bootlogo (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int hdmi = 0;

    if (argc == 2) {
        char *p = argv[1];
        if (strcmp("hdmi", p)) {
            printf("Usage: bootlogo {hdmi} \n");
            return -1;
        }
        hdmi = 1;
    }

    flcd_main(LCD0_ID, g_vin[LCD0_ID], g_output[LCD0_ID]);
    /* this only for lcd0 */
    if (hdmi) {
        hdmi_polling_thread(g_output[0]);
    }

    if (g_output[LCD0_ID] == OUTPUT_CCIR656_NTSC || g_output[LCD0_ID] == OUTPUT_CCIR656_PAL) {
        platform_tve100_config(LCD0_ID, g_output[LCD0_ID], 2);
    }

#if LCD2_ENABLE
    flcd_main(LCD2_ID, g_vin[LCD2_ID], g_output[LCD2_ID]);
    platform_tve100_config(LCD2_ID, g_output[LCD2_ID], 2);
#endif
    return 0;
}

U_BOOT_CMD(
	bootlogo,	2,	1,	do_bootlogo,
	"show lcd bootlogo",
	"no argument means VGA output only, \n"
	"{hdmi} - VGA and HDMI output simulataneously"
);


