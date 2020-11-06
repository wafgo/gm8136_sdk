/*
 *      dma.c
 *
 *      Simple DMA driver. (NAND/SPI-NOR to SDRAM only)
 *
 *      Written by:
 *
 *              Copyright (c) 2010 GM, ALL Rights Reserve
 */
#include <config.h>
#include <common.h>
#include <asm/io.h>

extern unsigned int ahbdma_cmd[5][2];

void Dma_init(unsigned int sync)
{
    volatile ipDMA *ptDMA = (volatile ipDMA *)CONFIG_DMAC_BASE;
    // config master 1 as little endian
    // config master 0 as little endian
    ptDMA->csr = BIT0;          // dma enable
    ptDMA->sync = sync;
}

int Dma_go_trigger(unsigned int * src, unsigned int * dst, unsigned int byte, DMA_LLPD * pllpd, unsigned int req, int real)
{
    volatile ipDMA *ptDMA = (volatile ipDMA *)CONFIG_DMAC_BASE;
    unsigned int ctrl;
    volatile CHANNELx *ch;
    int hw_ch;               // hardware channel no

    if (byte >= 4096 * 1024)    //4M
        return -1;

    hw_ch = req & 0xFF;

    if (req > DMA_MAX)
        return -1;

    // clear interrupt
    ptDMA->int_tc_clr = 1UL << hw_ch;
    ptDMA->int_err_clr = (1UL << hw_ch) | (1UL << (hw_ch + 16));
    ch = &ptDMA->ch[hw_ch];
    ch->cfg = ahbdma_cmd[req][1];
    ch->src = src;
    ch->dst = dst;

    ctrl = ahbdma_cmd[req][0];
    if (!real)                  //dummy dma
    {
        ctrl &= ~(0xF << 3);
        ctrl |= mDMACtrlSaddr2b(DMA_ADD_FIX);
        ctrl |= mDMACtrlDaddr2b(DMA_ADD_FIX);
    }

    ch->size = byte >> mDmaCtrlSrcWidth_mask(ctrl);

    if (pllpd) {
        ch->llp = ((unsigned int) pllpd) | mDmaCtrlSel_mask(ctrl);
        ctrl |= 0x80000000;
    } else {
        ch->llp = 0;            // end this chain
    }

    ch->ctrl = ctrl;

    return hw_ch;
}

///////////////////////////////////////////////////////////////////////////////
//              eBGDWaitFinish()
//              Description:
//                      1. Waitting for DMA finish its job
//              input:  BGD-channel-no, polling time
//              output: BGD_RET_NO_ERROR, BGD_RET_ERROR, BGD_RET_TIMEOUT
///////////////////////////////////////////////////////////////////////////////
int Dma_wait_timeout(int hw_ch, unsigned int timeout)
{
    volatile ipDMA *ptDMA = (volatile ipDMA *)CONFIG_DMAC_BASE;
    unsigned int count = timeout;
    unsigned int bit = BIT0 << hw_ch;

    // waitting for dma finish
    while (1) {
        // finish
        if (ptDMA->tc & bit) {
            return 0;
        }
        // error/abort
        if (ptDMA->err & (bit | bit << 8)) {
            ptDMA->int_err_clr = (bit | bit << 8);
            break;
        }
        if (timeout) {
            if ((--count) == 0)
                break;
        }
    };
    // stop dma
    ptDMA->ch[hw_ch].ctrl &= BIT0;
    printf("Dma_wait_timeout\n");
    return -1;
}

/* req: DMA_MEM2MEM / DMA_MEM2NAND ....
 * burst: DMA_BURST_1 ~ DMA_BURST_256
 * return:
 *  >= 0 for success
 */
int Dma_Set_BurstSz(unsigned int req, int burst)
{
    unsigned int val;

    //bit[15-8] is software channel
    req >>= 8;
    if (req > DMA_MAX)
        return -1;

    if ((burst < DMA_BURST_1) || (burst > DMA_BURST_256))
        return -1;

    val = ahbdma_cmd[req][0];
    val &= ~(0x7 << 16);
    val |= (burst << 16);
    ahbdma_cmd[req][0] = val;

    return 0;
}
