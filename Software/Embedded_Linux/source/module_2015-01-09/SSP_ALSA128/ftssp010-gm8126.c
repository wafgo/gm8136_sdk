#include <linux/dma-mapping.h>
#include "ftssp010-ssp.h"
#include "debug.h"

#define OSCH_CLK (12288000*2)

static unsigned int Main_CLK = OSCH_CLK;

//mclk: the SSP_CLK
int pmu_set_mclk(int cardno, u32 base, u32 speed, u32 * mclk, u32 * bclk,
                 ftssp_clock_table_t * clk_tab)
{
    int ret = 0;
    u8 i = 0;

    /* gm8126 should only use SSP1 and SSP2 as audio interface */
    if (unlikely((cardno != 1) && (cardno != 2))) {
        printk("%s fails: pmu_set_mclk not OK\n", __FUNCTION__);
        return -EINVAL;
    }

    for (i = 0; i < clk_tab->number; i++) {
        if (speed == clk_tab->fs_table[i]) {
            break;
        }
    }

    if (i == clk_tab->number) {
        err("freq (%d) not support, exit", speed);
        return -EINVAL;
    }

    if (clk_tab->bclk_table) {
        *bclk = clk_tab->bclk_table[i];
    } else {
        *bclk = 0;
    }

    *mclk = Main_CLK;

    return ret;
}

int ftssp010_dma_pmu_setting(int cardno, u32 base, u32 dma_ch, u32 dir)
{
    int ret = 0;
    u32 tmp = 0;
    unsigned long div = 0;

    /* gm8126 should only use SSP1 and SSP2 as audio interface */
    if (unlikely((cardno != 1) && (cardno != 2))) {
        printk("%s fails: pmu_set_mclk not OK\n", __FUNCTION__);
        return -EINVAL;
    }

    /* Divider setting 2 */
    tmp = readl(PMU_FTPMU010_VA_BASE + 0x78);
    if (cardno == 1) {
        tmp &= 0xfffff03f;      /* bit 6-11 */
    } else if (cardno == 2) {
        tmp &= 0xfffC0fff;      /* bit 12-17 */
    }
    div = PLL2_CLK_IN;
    if (cardno == 1) {
        do_div(div, 12000000);
    } else {
        do_div(div, 12288000);
    }
    div -= 1;
    if (cardno == 1) {
        tmp |= (div << 6);
    } else if (cardno == 2) {
        tmp |= (div << 12);
    }
    //printk("SSP%d gets divisor %d, SSP clock = %u \n", cardno, div, PLL2_CLK_IN/(div+1));

    Main_CLK = PLL2_CLK_IN / (div + 1);
    /* NOTE: IP limitation!! SSP working clk can not exceed 100MHz!! */
    if (Main_CLK > 100000000) {
        printk("IP limitation!! SSP working clk can not exceed 100MHz!! \n");
        return -1;
    }

    writel(tmp, PMU_FTPMU010_VA_BASE + 0x78);

    //pin mux when SSP1 selected
    tmp = readl(PMU_FTPMU010_VA_BASE + 0x60);
    if (cardno == 1) {
        tmp |= 0x249;
    }
    writel(tmp, PMU_FTPMU010_VA_BASE + 0x60);

    /* turn on SSP2 clock, APBMCLKOFF */
    tmp = readl(PMU_FTPMU010_VA_BASE + 0x3C);
    if (cardno == 1) {
        tmp &= ~(1 << 6);
    } else if (cardno == 2) {
        tmp &= ~(1 << 7);
    }
    writel(tmp, PMU_FTPMU010_VA_BASE + 0x3C);

    return ret;
}

/*
 * Follows AHB DMA REQ/ACK Mapping Table
 */
int ftssp010_get_dma_ch(int card_no, u32 * rx_ch, u32 * tx_ch)
{
    if (unlikely((rx_ch == NULL) || (tx_ch == NULL))) {
        printk("%s fails: rx_ch == NULL || tx_ch == NULL\n", __FUNCTION__);
        return -EINVAL;
    }

    /* gm8126 should only use SSP1 and SSP2 as audio interface */
    if (unlikely((card_no != 1) && (card_no != 2))) {
        printk("%s fails: pmu_set_mclk not OK\n", __FUNCTION__);
        return -EINVAL;
    }

    if (card_no == 1) {
        *rx_ch = 2;
        *tx_ch = 3;
    } else if (card_no == 2) {
        *rx_ch = 4;
        *tx_ch = 5;
    } else {
        printk("%s fails: doesn't support such cardno\n", __FUNCTION__);
        return -1;
    }

    return 0;
}

ftssp_hardware_platform_t ftssp_hw_platform = {
    .rates = (SNDRV_PCM_RATE_8000_48000 | SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_96000),
    .rate_min = 8000,
    .rate_max = 96000,
    .pmu_set_mclk = pmu_set_mclk,
    .pmu_set_dma = ftssp010_dma_pmu_setting,
    .pmu_get_dma = ftssp010_get_dma_ch,
};
