/**
 * @file ftssp010-gm8210.c
 * Grain Media's SSP PMU settings of GM8210 series SOC
 *
 * $Author: mars_ch $
 * $Id: ftssp010-gm8210.c,v 1.4 2011/05/16 06:55:19 mars_ch Exp $
 *
 * $Log: ftssp010-gm8210.c,v $
 * Revision 1.4  2011/05/16 06:55:19  mars_ch
 * *: use PMU API interface for gm8210 and adda300
 *
 * Revision 1.3  2010/09/29 02:13:05  mars_ch
 * *: add file header
 *
 * Revision 1.2  2010/09/28 03:34:15  mars_ch
 * *: refine coding style
 *
 * Revision 1.1  2010/08/02 02:10:41  mars_ch
 * *: add GM8210 SSP setting
 *
 */
#include <linux/version.h>      /* kernel versioning macros      */
#include <linux/module.h>       /* kernel modules                */
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <mach/platform/pmu.h>
#include <mach/platform/platform_io.h>
#include <mach/ftpmu010.h>
#include <mach/ftpmu010_pcie.h>
#include <mach/ftdmac020.h>
#include <linux/synclink.h>
#include "ftssp010-ssp.h"
#include "debug.h"

#define CLK_SRC_12000K  1
#define CLK_SRC_12288K  0
#define SSP3_CLK_SRC_24000K  0

#define OSCH_CLK (12000000)
static unsigned int Main_CLK = OSCH_CLK;
static int pmu_fd = -1, pmu_fd_cnt = 0, pcie_pmu_fd = -1, pcie_pmu_fd_cnt = 0;
#define DMA_UNKNOWN_CH  0x7F
/* Local function declaration */


#define FTSSP010_SETUP_PLATFORM_RESOURCE(_x)    \
static struct resource ftssp010_##_x##_resource[] = {\
        {\
                .start  = SSP_FTSSP010_##_x##_PA_BASE,\
                .end    = SSP_FTSSP010_##_x##_PA_LIMIT,\
                .flags  = IORESOURCE_MEM\
        },\
        {\
                .start  = SSP_FTSSP010_##_x##_IRQ,\
                .end    = SSP_FTSSP010_##_x##_IRQ,\
                .flags  = IORESOURCE_IRQ,\
        },\
}

static void ftssp010_release_device(struct device *dev) {}

#define FTSSP0101_SETUP_PLATFORM_DEVICE(_x)\
static struct platform_device ftssp010_##_x##_device = {\
        .dev            = {.release = ftssp010_release_device},\
        .name           = "ftssp010",\
        .id             = _x,\
        .num_resources  = ARRAY_SIZE(ftssp010_##_x##_resource),\
        .resource       = ftssp010_##_x##_resource\
}

/* define the resources of SSP0-5 */
FTSSP010_SETUP_PLATFORM_RESOURCE(0);
FTSSP010_SETUP_PLATFORM_RESOURCE(1);
FTSSP010_SETUP_PLATFORM_RESOURCE(2);
FTSSP010_SETUP_PLATFORM_RESOURCE(3);
FTSSP010_SETUP_PLATFORM_RESOURCE(4);
FTSSP010_SETUP_PLATFORM_RESOURCE(5);
//GM8312, which is 0 and 1
FTSSP010_SETUP_PLATFORM_RESOURCE(6);
FTSSP010_SETUP_PLATFORM_RESOURCE(7);

/* define the platform device of SSP0-5 */
FTSSP0101_SETUP_PLATFORM_DEVICE(0);
FTSSP0101_SETUP_PLATFORM_DEVICE(1);
FTSSP0101_SETUP_PLATFORM_DEVICE(2);
FTSSP0101_SETUP_PLATFORM_DEVICE(3);
FTSSP0101_SETUP_PLATFORM_DEVICE(4);
FTSSP0101_SETUP_PLATFORM_DEVICE(5);
//GM8312, which is 0 and 1
FTSSP0101_SETUP_PLATFORM_DEVICE(6);
FTSSP0101_SETUP_PLATFORM_DEVICE(7);

static pmuPcieReg_t reg_array_ssp_pcie[] = {
    /* reg_off,  bit_masks,       lock_bits,  init_val, init_mask */
    { 0x30,      0x3 << 16,       0x3 << 16,  0,        0},
    { 0x34,      0x3 << 17,       0x3 << 17,  0,        0x3 << 17},
    { 0x48,      0x3 << 2,        0x3 << 2,   0x3 << 2, 0x3 << 2 },   //12M as the source
};

static pmuPcieRegInfo_t ssp_pcie_clk_info = {
    "ssp_pcie_clk",
    ARRAY_SIZE(reg_array_ssp_pcie),
    ATTR_TYPE_PCIE_NONE,
    reg_array_ssp_pcie,
};

static pmuReg_t reg_array_ssp[] = {
/* reg_off,  bit_masks,   lock_bits,     init_val,   init_mask */
#if CLK_SRC_12000K
    {0x28,  0x3F << 20,  0x3F << 20,    0x15 << 20,    0x3F << 20}, //SSP0-2 select 12M as the source
    {0x70,  0x3 << 30,   0x3 << 30,     0x1 << 30,    0x3 << 30},  //SSP3 selects 12M as the source
    {0x74,  0x3 << 30,   0x3 << 30,     0x1 << 30,    0x3 << 30},  //SSP4 selects 12M as the source
    {0x78,  0x3 << 30,   0x3 << 30,     0x1 << 30,    0x3 << 30},  //SSP5 selects 12M as the source
    {0xb8, (0x1 << 22) | (0x7 << 6),   (0x1 << 22) | (0x7 << 6), 0x0, 0x0}, //SSP0-3 pclk & sclk
    {0xbc,  0x3 << 18,   0x3 << 18,     0x0,          0x0},        //SSP4-5 pclk & sclk
#elif CLK_SRC_12288K  //CLK SRC 12288K
    {0x08,  0x1 << 16,   0x1 << 16,     0x0 << 16,    0x1 << 16},  //enable OSCH_1 as 12.288M
    {0x28,  0x3F << 20,  0x3F << 20,    0x0 << 20,    0x3F << 20}, //SSP0-2 select 12.288M as the source
    {0x60,  0x3 << 12,   0x3 << 12,     0x1 << 12,    0x3 << 12},  //SSP clock source as 12.288M
    {0x70,  0x3 << 30,   0x3 << 30,     0x0 << 30,    0x3 << 30},  //SSP3 selects 12M.288 as the source
    {0x74,  0x3 << 30,   0x3 << 30,     0x0 << 30,    0x3 << 30},  //SSP4 selects 12M.288 as the source
    {0x78,  0x3 << 30,   0x3 << 30,     0x0 << 30,    0x3 << 30},  //SSP5 selects 12M.288 as the source
    {0xb8, (0x1 << 22) | (0x7 << 6),   (0x1 << 22) | (0x7 << 6), 0x0, 0x0}, //SSP0-3 pclk & sclk
    {0xbc,  0x3 << 18,   0x3 << 18,     0x0,          0x0},        //SSP4-5 pclk & sclk
#else   // SSP0~2/4~5 as 12MHz, SSP3 as 24MHz
    {0x28,  0x3F << 20,  0x3F << 20,    0x15 << 20,    0x3F << 20}, //SSP0-2 select 12M as the source
    {0x60,  ((0x3 << 12) | (0x3FF << 17) | (0x1 << 16)),   (0x3 << 12) | (0x3FF << 17) | (0x1 << 16),  0x0,  (0x3 << 12) | (0x3FF << 17) | (0x1 << 16)},
    {0x70,  0x3 << 30,   0x3 << 30,     0x0 << 30,    0x3 << 30},  //SSP3 selects clk_12288 as the source
    {0x74,  0x3 << 30,   0x3 << 30,     0x1 << 30,    0x3 << 30},  //SSP4 selects 12M as the source
    {0x78,  0x3 << 30,   0x3 << 30,     0x1 << 30,    0x3 << 30},  //SSP5 selects 12M as the source
    {0xb8, (0x1 << 22) | (0x7 << 6),   (0x1 << 22) | (0x7 << 6), 0x0, 0x0}, //SSP0-3 pclk & sclk
    {0xbc,  0x3 << 18,   0x3 << 18,     0x0,          0x0},        //SSP4-5 pclk & sclk
#endif
};

static pmuRegInfo_t ssp_clk_info = {
    "ssp_clk",
    ARRAY_SIZE(reg_array_ssp),
    ATTR_TYPE_NONE,
    reg_array_ssp,
};

typedef struct pvalue {
    int bit17_21;
    int bit22_24;
} pvalue_t;

int ssp_get_pvalue(pvalue_t *pvalue, int div)
{
    int i = 0;
    int flag = 0;

    for (i = 1; i <= 7; i++) {
        if (div % i == 0) {
            pvalue->bit17_21 = div / i;
            if (pvalue->bit17_21 > 32)
                continue;
            else {
                pvalue->bit22_24 = i;
                flag = 1;
                break;
            }
        }
    }

    return flag;
}

/*  SSP3 playback to HDMI, sample rate = 48k, sample size = 16 bits, bit clock = 3072000, SSP3 MAIN CLOCK must = 24MHz
 *  pmu 0x60 [13:12] : select 12.288MHz source for analog die and clk_12288
 *  [12] : 1 - clk_12288 = 12.288MHz, 0 - clk_12288 = ssp6144_clk
 *  [13] : 1 - analog die clock = 12MHz, 0 - analog die clock = clk_12288
 *   Note : ssp6144_clk = ((pll4out1, pll4out2 or pll4out3) / ([24:22]+1)/([21:17]+1))
 *             clk_12288 is SSP and analog die clock's candidate
 *  [16] : gating clock control, 1-gating, 0-un-gating
 *  [21:17] : ssp6144_clk_cnt5_pvalue
 *  [24:22] : ssp6144_clk_cnt3_pvalue
 *  [26:25] : ssp6144_clk_sel, 00-PLL4 CKOUT1, 01-PLL4 CKOUT2, 1x-PLL4 CKOUT3
 *
 *  SSP3 choose clk_12288 as main clock, clk_12288 form ssp6144_clk,
 */
void pmu_cal_clk(ftssp010_card_t *card)
{
    unsigned int pll4_out1, div;
    unsigned int pll4_out2, pll4_out3;
    unsigned int ssp3_clk = 24000000;
    pvalue_t pvalue;
    int pll4_clkout1 = 0, pll4_clkout2 = 0, pll4_clkout3 = 0;
    int bit25_26 = 0, flag = 0;
    int pmu_fd = card->pmu_fd;

    memset(&pvalue, 0x0, sizeof(pvalue_t));

    pll4_out1 = ftpmu010_get_attr(ATTR_TYPE_PLL4);   //hz
    pll4_out2 = pll4_out1 * 2 / 3;   // pll4_out1 / 1.5
    pll4_out3 = pll4_out1 * 2 / 5;   // pll4_out1 / 2.5

    div = ftpmu010_clock_divisor2(ATTR_TYPE_PLL4, ssp3_clk, 1);

    flag = ssp_get_pvalue(&pvalue, div);
    if (flag == 1)
        pll4_clkout1 = 1;

    if (flag == 0) {
        div = pll4_out2 / Main_CLK;
        flag = ssp_get_pvalue(&pvalue, div);
        if (flag == 1)
            pll4_clkout2 = 1;
    }

    if (flag == 0) {
        div = pll4_out3 / Main_CLK;
        flag = ssp_get_pvalue(&pvalue, div);
        if (flag == 1)
            pll4_clkout3 = 1;
    }

    if (pll4_clkout1)
        bit25_26 = 0;
    if (pll4_clkout2)
        bit25_26 = 1;
    if (pll4_clkout3)
        bit25_26 = 2;

    if (ftpmu010_write_reg(pmu_fd, 0x60, ((0x2 << 12) | (0x0 << 16) | ((pvalue.bit17_21 - 1) << 17) | ((pvalue.bit22_24 - 1) << 22) | (bit25_26 << 25)), ((0x3 << 12) | (0x3FF << 17) | (0x1 << 16))))
        panic("%s, register ssp3 pvalue fail\n", __func__);
    // SSP3 clk_sel = clk_12288
    if (ftpmu010_write_reg(pmu_fd, 0x70, 0x0 << 30, 0x3 << 30))
        panic("%s, register ssp3 clock on fail\n", __func__);
}

static int pmu_init(ftssp010_card_t *card)
{
    if (!card) {
        printk("In %s: card is NULL.\n", __func__);
        return 0;
    }

    /* totally we have 7 ssps */
    if ((card->cardno < 0) || (card->cardno > 7)) {
        printk("In %s: cardno(%d) is not reasonable.\n", __func__, card->cardno);
        panic("Worong SSP cardno!!\n");
    }

    /* gm8210 */
    if (card->cardno <= 5) {
        if (pmu_fd < 0)
            pmu_fd = ftpmu010_register_reg(&ssp_clk_info);
        card->pmu_fd = pmu_fd;
        if (card->pmu_fd < 0) {
            printk("In %s: card(%d) register pmu setting failed\n", __func__, card->cardno);
            goto register_fail;
        }
        pmu_fd_cnt ++;
    } else if ((card->cardno == 6) || (card->cardno == 7)) {
        /* GM8312 */
        if (pcie_pmu_fd < 0)
            pcie_pmu_fd = ftpmu010_pcie_register_reg(&ssp_pcie_clk_info);
        card->pmu_fd = pcie_pmu_fd;
        if (card->pmu_fd < 0) {
            printk("In %s: card(%d) register pmu setting failed\n", __func__, card->cardno);
            goto register_fail;
        }
        pcie_pmu_fd_cnt ++;
    }
#if SSP3_CLK_SRC_24000K
    if (card->cardno == 3) {
        pmu_cal_clk(card);
    }
#endif
    return 0;

register_fail:
    panic("%s fail! \n", __func__);

    return -1;
}

static int pmu_deinit(ftssp010_card_t *card)
{
    if (card->cardno <= 5) {/* GM8310 */
        pmu_fd_cnt --;
        if (!pmu_fd_cnt)
            ftpmu010_deregister_reg(card->pmu_fd);
    }
    else { /* GM8312 */
        pcie_pmu_fd_cnt --;
        if (!pcie_pmu_fd_cnt)
            ftpmu010_pcie_deregister_reg(card->pmu_fd);
    }

    return 0;
}

/*
 * Follows AHB DMA REQ/ACK routing Table
 */
static int ftssp010_get_dma_ch(int card_no, u32 *rx_ch, u32 *tx_ch)
{
    if (unlikely((rx_ch == NULL) || (tx_ch == NULL))) {
        printk("%s fails: rx_ch == NULL || tx_ch == NULL\n", __func__);
        return -EINVAL;
    }

    /* the following table is for AHB DMA */
    switch (card_no) {
      case 0:
        *rx_ch = 10;
        *tx_ch = 9;
        break;
      case 1:
        *rx_ch = 12;
        *tx_ch = 11;
        break;
      case 2:
       *rx_ch = 14;
        *tx_ch = 13;
        break;
      case 3:
        *rx_ch = 4;
        *tx_ch = 3;
        break;
      case 4:
        *rx_ch = 6;
        *tx_ch = DMA_UNKNOWN_CH;    //only rx, don't support tx
        break;
      case 5:
        *rx_ch = 7;
        *tx_ch = 15;
        break;
      case 6:   //GM8312 AHB-DMA
        *rx_ch = 0;
        *tx_ch = 1;
        break;
      case 7:   //GM8312 AHB-DMA
        *rx_ch = 2;
        *tx_ch = 3;
        break;
      default:
        panic("%s, invalid value: %d \n", __func__, card_no);
        break;
    }

    return 0;
}

static int ftssp010_driver_probe(struct platform_device *pdev)
{
    struct resource *res = NULL;
    ftssp010_card_t *card = platform_get_drvdata(pdev);
    int irq = -1, ret = -1;
    u32 rx_ch, tx_ch;

    if (card->cardno > 7)
        panic("%s, invalid card%d \n", __func__, card->cardno);

    card->dmac_pbase = (card->cardno <= 5) ? DMAC_FTDMAC020_PA_BASE : DMAC_FTDMAC020_1_PA_BASE;
    card->dmac_vbase = (u32)ioremap_nocache(card->dmac_pbase, PAGE_SIZE);
    if (unlikely(!card->dmac_vbase))
        panic("%s, no virtual memory! \n", __func__);

    //get the SSP controller's memory I/O, this will be pyhsical address
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (unlikely(res == NULL)) {
        printk("%s fail: can't get resource\n", __func__);
        ret = -ENXIO;
        goto err_final;
    }

    card->pbase = res->start;
    card->mem_len = res->end - res->start + 1;

    if (unlikely((irq = platform_get_irq(pdev, 0)) < 0)) {
        printk("%s fail: can't get irq\n", __func__);
        ret = irq;
        goto err_final;
    }

    card->irq = irq;
    ftssp010_get_dma_ch(card->cardno, &rx_ch, &tx_ch);

    /* request a capture dma channel */
    if (rx_ch != DMA_UNKNOWN_CH) {
        dmaengine_t  *dmaengine = &card->capture.dmaengine;

        dma_cap_zero(dmaengine->cap_mask);
        dma_cap_set(DMA_SLAVE, dmaengine->cap_mask);
        if (card->cardno <= 5) {
            dmaengine->dma_slave_config.id = 0;    //local AHB-DMA
            dmaengine->dma_slave_config.src_sel = FTDMA020_AHBMASTER_0;
            dmaengine->dma_slave_config.dst_sel = FTDMA020_AHBMASTER_1;
        } else {
            dmaengine->dma_slave_config.id = 1;    //PCI-E AHB-DMA
            dmaengine->dma_slave_config.src_sel = FTDMA020_AHBMASTER_0; //only one master port
            dmaengine->dma_slave_config.dst_sel = FTDMA020_AHBMASTER_0;
        }

        dmaengine->dma_slave_config.handshake = rx_ch;   //hardware handshake pin
        dmaengine->dma_chan = dma_request_channel(dmaengine->cap_mask, ftdmac020_chan_filter,
                                            (void *)&dmaengine->dma_slave_config);
        if (!dmaengine->dma_chan) {
            printk("%s, No RX DMA channel for cardno:%d \n", __func__, card->cardno);
            ret = -ENOMEM;
            goto err_final;
        }
    }

    /* request a playback dma channel */
    if (tx_ch != DMA_UNKNOWN_CH) {
        dmaengine_t  *dmaengine = &card->playback.dmaengine;

        dma_cap_zero(dmaengine->cap_mask);
        dma_cap_set(DMA_SLAVE, dmaengine->cap_mask);
        if (card->cardno <= 5) {
            dmaengine->dma_slave_config.id = 0;    //local AHB-DMA
            dmaengine->dma_slave_config.src_sel = FTDMA020_AHBMASTER_1;
            dmaengine->dma_slave_config.dst_sel = FTDMA020_AHBMASTER_0;
        } else {
            dmaengine->dma_slave_config.id = 1;    //PCI-E AHB-DMA
            dmaengine->dma_slave_config.src_sel = FTDMA020_AHBMASTER_0; //only one master port
            dmaengine->dma_slave_config.dst_sel = FTDMA020_AHBMASTER_0;
        }

        dmaengine->dma_slave_config.handshake = tx_ch;   //hardware handshake pin
        dmaengine->dma_chan = dma_request_channel(dmaengine->cap_mask, ftdmac020_chan_filter,
                                            (void *)&dmaengine->dma_slave_config);
        if (!dmaengine->dma_chan) {
            printk("%s, No TX DMA channel for cardno:%d \n", __func__, card->cardno);
            ret = -ENOMEM;
            /* release rx channel */
            if (card->capture.dmaengine.dma_chan != NULL) {
                dma_release_channel(card->capture.dmaengine.dma_chan);
                card->capture.dmaengine.dma_chan = NULL;
            }
            goto err_final;
        }
    }

    //optional, but this is better for future debug
    if (!request_mem_region(res->start, (res->end - res->start + 1), dev_name(&pdev->dev))) {
        printk("%s fail: could not reserve memory region\n", __func__);
        ret = -ENOMEM;
        goto err_final;
    }

    //map pyhsical address to virtual address
    card->vbase = (u32) ioremap_nocache(res->start, (res->end - res->start + 1));
    if (unlikely(card->vbase == 0)) {
        printk("%s fail: counld not do ioremap\n", __func__);
        ret = -ENOMEM;
        goto err_ioremap;
    }

    /* init pmu */
    if (pmu_init(card) < 0)
        panic("%s, cardno:%d register pmu fail! \n", __func__, card->cardno);

    printk("SSP-128bit cardno = %d, pbase = 0x%x, vbase = 0x%x, irq = %d\n", card->cardno,
                                                        card->pbase, card->vbase, card->irq);
    return 0;

  err_ioremap:
    release_mem_region(card->pbase, card->mem_len);

  err_final:
    __iounmap((void *)card->dmac_vbase);
    card->dmac_vbase = 0;

    return ret;
}

static int ftssp010_driver_remove(struct platform_device *pdev)
{
    ftssp010_card_t *card = platform_get_drvdata(pdev);

    __iounmap((void *)card->dmac_vbase);
    card->dmac_vbase = 0;

    /* de-init pmu */
    pmu_deinit(card);

    return 0;
}

/* this driver will service SSP0, SSP1 and SSP2 */
static struct platform_driver ftssp010_driver = {
    .probe = ftssp010_driver_probe,
    .remove = ftssp010_driver_remove,
    .driver = {
               .name = "ftssp010",
               .owner = THIS_MODULE,
               },
};

static int ftssp010_device_bind_driver_data(struct platform_device *pdev, ftssp010_card_t * card)
{
    /* drv data */
    platform_set_drvdata(pdev, card);

    return platform_device_register(pdev);
}

int ftssp010_init_platform_device(ftssp010_card_t * card)
{
    if (card->cardno == 0) {
        return ftssp010_device_bind_driver_data(&ftssp010_0_device, card);
    } else if (card->cardno == 1) {
        return ftssp010_device_bind_driver_data(&ftssp010_1_device, card);
    } else if (card->cardno == 2) {
        return ftssp010_device_bind_driver_data(&ftssp010_2_device, card);
    } else if (card->cardno == 3) {
        return ftssp010_device_bind_driver_data(&ftssp010_3_device, card);
    } else if (card->cardno == 4) {
        return ftssp010_device_bind_driver_data(&ftssp010_4_device, card);
    } else if (card->cardno == 5) {
        return ftssp010_device_bind_driver_data(&ftssp010_5_device, card);
    } else if (card->cardno == 6) {
        //GM8312
        return ftssp010_device_bind_driver_data(&ftssp010_6_device, card);
    } else if (card->cardno == 7) {
        //GM8312
        return ftssp010_device_bind_driver_data(&ftssp010_7_device, card);
    }

    return -1;
}

static void ftssp010_release_resource(ftssp010_card_t *card)
{
    iounmap((void *)card->vbase);
    release_mem_region(card->pbase, card->mem_len);

    if (card->capture.dmaengine.dma_chan) {
        dma_release_channel(card->capture.dmaengine.dma_chan);
        card->capture.dmaengine.dma_chan = NULL;
    }

    if (card->playback.dmaengine.dma_chan) {
        dma_release_channel(card->playback.dmaengine.dma_chan);
        card->playback.dmaengine.dma_chan = NULL;
    }

    return;
}

void ftssp010_deinit_platform_device(ftssp010_card_t * card)
{
    printk("deinit SOUNDCARD#%d\n", card->cardno);

    ftssp010_release_resource(card);
    if (card->cardno == 0) {
        return platform_device_unregister(&ftssp010_0_device);
    } else if (card->cardno == 1) {
        return platform_device_unregister(&ftssp010_1_device);
    } else if (card->cardno == 2) {
        return platform_device_unregister(&ftssp010_2_device);
    } else if (card->cardno == 3) {
        return platform_device_unregister(&ftssp010_3_device);
    } else if (card->cardno == 4) {
        return platform_device_unregister(&ftssp010_4_device);
    } else if (card->cardno == 5) {
        return platform_device_unregister(&ftssp010_5_device);
    } else if (card->cardno == 6) {
        //GM8312
        return platform_device_unregister(&ftssp010_6_device);
    } else if (card->cardno == 7) {
        //GM8312
        return platform_device_unregister(&ftssp010_7_device);
    }
}

int ftssp010_init_platform_driver(void)
{
    return platform_driver_register(&ftssp010_driver);
}

void ftssp0101_deinit_platform_driver(void)
{
    platform_driver_unregister(&ftssp010_driver);
}

//mclk: the SSP_CLK
static int pmu_set_mclk(ftssp010_card_t *card, u32 speed, u32 *mclk, u32 *bclk,
                                                        ftssp_clock_table_t *clk_tab)
{
    int ret = 0, i;

    for (i = 0; i < clk_tab->number; i ++) {
        if (speed == clk_tab->fs_table[i]) {
            break;
        }
    }

    if (i == clk_tab->number) {
        err("freq (%d) not support, exit", speed);
        return -EINVAL;
    }

    if (clk_tab->bclk_table == NULL)
        panic("%s, error: The bclk table is null! \n", __func__);

    *bclk = clk_tab->bclk_table[i];
    *mclk = Main_CLK;
#if SSP3_CLK_SRC_24000K
    if (card->cardno == 3)
        *mclk = 24000000;
#endif
    return ret;
}

static int ftssp010_pmu_setting(ftssp010_card_t *card)
{
    int ret = 0, cardno = card->cardno;
    u32 mask = 0;
    int pmu_fd = card->pmu_fd;

    switch (cardno) {
      case 0:
      case 1:
      case 2:
        mask = 0x1 << (cardno + 6); //bit 6-8
        if (ftpmu010_write_reg(pmu_fd, 0xB8, 0, mask)) //pclk & sclk
            panic("%s, error in cardno %d \n", __func__, cardno);
        break;
 	  case 3:
        if (ftpmu010_write_reg(pmu_fd, 0xB8, 0, 0x1 << 22))
            panic("%s, error in cardno %d \n", __func__, cardno);
        break;
      case 4:
      case 5:
        mask = 0x1 << (cardno + 18); //bit 18-19
        if (ftpmu010_write_reg(pmu_fd, 0xBC, 0, mask))
            panic("%s, error in cardno %d \n", __func__, cardno);
        break;
      case 6:
      case 7:
        mask = (cardno == 6) ? (0x1 << 17) : (0x1 << 16);
        if (ftpmu010_pcie_write_reg(pmu_fd, 0x30, 0, mask))
            panic("%s, error in cardno %d \n", __func__, cardno);
        break;
      default:
        panic("%s, error in cardno %d \n", __func__, cardno);
        break;
    }

    return ret;
}

ftssp_hardware_platform_t ftssp_hw_platform = {
    .rates = SNDRV_PCM_RATE_8000_48000,
    .rate_min = 8000,
    .rate_max = 96000,
    .pmu_set_mclk = pmu_set_mclk,
    .pmu_set_pmu = ftssp010_pmu_setting,
};

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("GM SSP-ALSA driver");
MODULE_LICENSE("GPL");


