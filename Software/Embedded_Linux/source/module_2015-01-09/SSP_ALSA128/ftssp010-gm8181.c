#include <asm/arch/platform/spec.h>
#include <linux/dma-mapping.h>
#include "ftssp010-ssp.h"
#include "debug.h"

//#define OSCH_CLK (12288000)
#define OSCH_CLK (12000000)

//#define SSP_USE_PLL3

static unsigned int Main_CLK = OSCH_CLK;

/*
 * IP base array. Sorted by SSP0, SSP1, ....
 */
static dma_addr_t ip_base_array[] = {I2S_PCIE_FTSSP010_0_PA_BASE, I2S_PCIE_FTSSP010_1_PA_BASE};
static int        ip_irq_array[] = {I2S_PCIE_FTSSP010_0_IRQ, I2S_PCIE_FTSSP010_1_IRQ};
/*
 * get the relative clock.
 * Input parameters:
 *      cardno, framerate, table
 * Output parameters:
 *      sspclk, bclk
 * Return value:
 *      0 for success, otherwise for fail.
 */
static int ftssp010_get_clks(int cardno, u32 framerate, ftssp_clock_table_t *clk_tab, u32 *sspclk, u32 *bclk)
{
    int ret = 0;
    int i;

	if (cardno) {}

    for (i = 0; i < clk_tab->number; i++) {
        if (framerate == clk_tab->fs_table[i])
            break;
    }

    if (i == clk_tab->number) {
        err("freq (%d) not support, exit", framerate);
        return -EINVAL;
    }

    if (clk_tab->bclk_table)
        *bclk = clk_tab->bclk_table[i];
    else
        *bclk = 0;

    *sspclk = Main_CLK;

    return ret;
}

static int ftssp010_hardware_init(int cardno)
{
    int ret = 0;
    u32 tmp;
    unsigned long div;

    tmp = readl(PCIPMU_FTPMU010_VA_BASE + 0x30);
    //  SSP0CLK SSP1CLK on
    tmp &= ~(0x3<<16);
    //  AD/DA MCLK on
    tmp &= ~(0x3<<14);
    writel(tmp, PCIPMU_FTPMU010_VA_BASE + 0x30);
    
    tmp = readl(PCIPMU_FTPMU010_VA_BASE + 0x34);
    //  I2S0/1 APB clk on
    tmp &= ~(0x3<<17);
    writel(tmp, PCIPMU_FTPMU010_VA_BASE + 0x34);

#if 0
#ifdef SSP_USE_PLL3
    tmp = readl(PCIPMU_FTPMU010_VA_BASE + 0x28);
    if (cardno == 0)
        tmp |= 0x1 << 15;

    if (cardno == 1)
        tmp |= 0x1 << 16;

    if (cardno == 2) {
        tmp |= 0x1 << 17;
    }

    writel(tmp, PCIPMU_FTPMU010_VA_BASE + 0x28);
#else
    /* select clock source */
    tmp = readl(PCIPMU_FTPMU010_VA_BASE + 0x28);
    tmp &= ~(0x7 << 15);        /* 0: select OSCH1 */
    writel(tmp, PCIPMU_FTPMU010_VA_BASE + 0x28);
#endif

    /* turn on the clock, APBMCLKOFF */
    tmp = readl(PCIPMU_FTPMU010_VA_BASE + 0x3C);
    if (cardno == 0) {
        /* SSP0 */
        tmp &= ~(0x1 << 6);
    } else if (cardno == 1) {
        /* SSP1 */
        tmp &= ~(0x1 << 7);
    } else {
        /* SSP2 */
        tmp &= ~(0x1 << 8);
    }
    writel(tmp, PCIPMU_FTPMU010_VA_BASE + 0x3C);

    /* SSP pins muxed with GPIO */
    tmp = readl(PCIPMU_FTPMU010_VA_BASE + 0x50);
    if (cardno == 1) {
        tmp &= ~(0x1 << 11);
    } else if (cardno == 2) {
        tmp &= ~(0x1 << 12);
    }
    writel(tmp, PCIPMU_FTPMU010_VA_BASE + 0x50);

    tmp = readl(PCIPMU_FTPMU010_VA_BASE + 0x78);
    tmp &= 0xffff8000;          /* bit 0-14 */
    div = PLL3_CLK_IN;
    //SSP_CLK should be much higher than the possible speed of bclk to judge the level correctly
    //we assume the future bclk always under 48000*256
    do_div(div, 48000 * 256 * 6);       //48K FS
    div -= 1;
    tmp |= div | (div << 5) | (div << 10);
    printk("SSP%d gets divisor %d, SSP clock = %d \n", cardno, div,  (u32)(PLL3_CLK_IN / (div + 1)));

#ifdef SSP_USE_PLL3
    Main_CLK = PLL3_CLK_IN / (div + 1);
#endif

    /* NOTE: IP limitation!! SSP working clk can not exceed 100MHz!! */
    if (Main_CLK > 100000000) {
        printk("IP limitation!! SSP working clk can not exceed 100MHz!! \n");
        return -1;
    }

    writel(tmp, PCIPMU_FTPMU010_VA_BASE + 0x78);
#endif
    return ret;
}

/*
 * Follows AHB DMA REQ/ACK Mapping Table
 */
int ftssp010_get_dmach(int card_no, u32 * rx_ch, u32 * tx_ch)
{
    int ret = -1;

    if ((rx_ch == NULL) || (tx_ch == NULL))
        return -1;

    *rx_ch = 0;
    *tx_ch = 0;

    /* only supports two SSPs */
    if (card_no == 0) {
        *rx_ch = 0;             /* AHB DMA REQ/ACK Mapping Table */
        *tx_ch = 1;
        ret = 0;
    } else if (card_no == 1) {
        *rx_ch = 2;
        *tx_ch = 3;
        ret = 0;
    } else if (card_no == 2) {
        *rx_ch = 4;
        *tx_ch = 5;
        ret = 0;
    } else
        printk("Error in %s \n", __FUNCTION__);

    return ret;
}

ftssp_hardware_platform_t ftssp_hw_platform = {
    .paddr_array = ip_base_array,
    .irq_array = ip_irq_array,
    .array_num = ARRAY_SIZE(ip_base_array),
    .platform_init = ftssp010_hardware_init,
    .platform_get_clks = ftssp010_get_clks,
    .platform_get_dmach = ftssp010_get_dmach,
};
