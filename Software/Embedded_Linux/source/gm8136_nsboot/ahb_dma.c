/*
 *      dma.c
 *
 *      Simple DMA driver. (NAND/SPI-NOR to SDRAM only)
 *
 *      Written by:
 *
 *              Copyright (c) 2010 GM, ALL Rights Reserve
 */
#include "board.h"
#include "ahb_dma.h"

// Control Register
#define mDMACtrlTc1b(v)				((v) << 31)
#define mDMACtrlDMAFFTH3b(p)		((p) << 24)
#define mDMACtrlChpri2b(p)			((p) << 22)
#define mDMACtrlCache1b(v)			((v) << 21)
#define mDMACtrlBuf1b(v)			((v) << 20)
#define mDMACtrlPri1b(v)			((v) << 19)
#define mDMACtrlSsz3b(type)			((type) << 16)
#define mDMACtrlAbt1b(v)			((v) << 15)
#define mDMACtrlSwid3b(width)		((width) << 11)
#define mDMACtrlDwid3b(width)		((width) << 8)
#define mDMACtrlHW1b(v)				((v) << 7)
#define mDMACtrlSaddr2b(type) 		((type) << 5)
#define mDMACtrlDaddr2b(type) 		((type) << 3)
#define mDMACtrlSsel1b(v) 			((v) << 2)
#define mDMACtrlDsel1b(v) 			((v) << 1)
#define mDMACtrlEn1b(enable)		((enable) << 0)

// Configuration Register
//#define mDMACfgLLP4b(v)		((v) << 16)
//#define mDMACfgBusy1b(v)	((v) << 8)
#define mDMACfgDst5b(v)	    ((v) << 9)
#define mDMACfgSrc5b(v)	    ((v) << 3)
#define mDMACfgAbtDis1b(v)	((v) << 2)
#define mDMACfgErrDis1b(v)	((v) << 1)
#define mDMACfgTcDis1b(v)	((v) << 0)

// LLP Register
#define mDMALLPaddr30b(v)	((v) << 2)
#define mDMALLPmaster1b(v)	((v) << 0)

// DMA Data width
#define DMA_WIDTH_1		0x0L		// 1 Byte
#define DMA_WIDTH_2		0x1L		// 2 Bytes
#define DMA_WIDTH_4		0x2L		// 4 Bytes

// DMA Address control
#define DMA_ADD_INC		0x0L
#define DMA_ADD_DEC		0x1L
#define DMA_ADD_FIX		0x2L

#define mDmaCtrlSrcWidth_mask(ctrl)		(((ctrl) >> 11) & 3)
#define mDmaCtrlSel_mask(ctrl)		    (((ctrl) >> 2) & 1)
#define TIMEOUT_DMA				0x8000

static INT32U ahbdma_cmd[4][2] =
{
	// DMA_MEM2NAND		((1UL << 8))
	{
		(mDMACtrlTc1b(0) |
		 mDMACtrlDMAFFTH3b(0) |
		 mDMACtrlChpri2b(0) |
		 mDMACtrlCache1b(0) |
		 mDMACtrlBuf1b(0) |
		 mDMACtrlPri1b(0) |
		 mDMACtrlSsz3b(DMA_BURST_4) | /* default value */
		 mDMACtrlAbt1b(0) |
		 mDMACtrlSwid3b(DMA_WIDTH_4) |
		 mDMACtrlDwid3b(DMA_WIDTH_4) |
		 mDMACtrlHW1b(1) |
		 mDMACtrlSaddr2b(DMA_ADD_INC) |     //Memory is the source
		 mDMACtrlDaddr2b(DMA_ADD_FIX) |     //NAND is the destination
		 mDMACtrlSsel1b(1) |                //AHB Master 1 is the source
		 mDMACtrlDsel1b(0) |                //AHB Master 0 is the destination
		 mDMACtrlEn1b(1)),

		(mDMACfgDst5b(0x10 | REQ_NANDRX) |   //bit13, 12-9, NANDC:9
		 mDMACfgSrc5b(0)    |
		 mDMACfgAbtDis1b(0) |
		 mDMACfgErrDis1b(0) |
		 mDMACfgTcDis1b(0))
	},	
	// DMA_NAND2MEM		((2UL << 8))
	{
		(mDMACtrlTc1b(0) |
		 mDMACtrlDMAFFTH3b(0) |
		 mDMACtrlChpri2b(0) |
		 mDMACtrlCache1b(0) |
		 mDMACtrlBuf1b(0) |
		 mDMACtrlPri1b(0) |
		 mDMACtrlSsz3b(DMA_BURST_4) |
		 mDMACtrlAbt1b(0) |
		 mDMACtrlSwid3b(DMA_WIDTH_4) |
		 mDMACtrlDwid3b(DMA_WIDTH_4) |
		 mDMACtrlHW1b(1) |
		 mDMACtrlSaddr2b(DMA_ADD_FIX) |
		 mDMACtrlDaddr2b(DMA_ADD_INC) |
		 mDMACtrlSsel1b(0) |                //AHB Master 0 is the source
		 mDMACtrlDsel1b(1) |                //AHB Master 1 is the destination
		 mDMACtrlEn1b(1)),

		(mDMACfgDst5b(0)    |
		 mDMACfgSrc5b(0x10 | REQ_NANDTX) |
		 mDMACfgAbtDis1b(0) |
		 mDMACfgErrDis1b(0) |
		 mDMACfgTcDis1b(0))
	},
	
	/*
	 * NOR configuration
	 */
	// DMA_MEM2NOR		((3UL << 8))
	{
		(mDMACtrlTc1b(0) |                  //TC_MSK
		 mDMACtrlDMAFFTH3b(0) |             //DMA FIFO threshold value
		 mDMACtrlChpri2b(0) |               //Channel priority
		 mDMACtrlCache1b(0) |               //PROTO3
		 mDMACtrlBuf1b(0) |                 //PROTO2    
		 mDMACtrlPri1b(0) |                 //PROTO1
		 mDMACtrlSsz3b(DMA_BURST_8) |       //Source burst size selection
		 mDMACtrlAbt1b(0) |                 //ABT    
		 mDMACtrlSwid3b(DMA_WIDTH_4) |      //SRC_WIDTH
		 mDMACtrlDwid3b(DMA_WIDTH_4) |      //DST_WIDTH
		 mDMACtrlHW1b(1) |                  //Hardware Handshake
		 mDMACtrlSaddr2b(DMA_ADD_INC) |     //Memory is the source
		 mDMACtrlDaddr2b(DMA_ADD_FIX) |     //NOR is the destination
		 mDMACtrlSsel1b(1) |                //AHB Master 1 is the source
		 mDMACtrlDsel1b(0) |                //AHB Master 0 is the destination
		 mDMACtrlEn1b(1)),                  //Channel Enable

		(mDMACfgDst5b(0x10 | REQ_SPI020RX) |               //bit13, 12-9; SSP0_TX:1
		 mDMACfgSrc5b(0)    |               //Memory
		 mDMACfgAbtDis1b(0) |               //Channel abort interrupt mask. 0 for not mask
		 mDMACfgErrDis1b(0) |               //Channel error interrupt mask. 0 for not mask
		 mDMACfgTcDis1b(0))                 //Channel terminal count interrupt mask. 0 for not mask
	},	
	// DMA_NOR2MEM		((4UL << 8))
	{
		(mDMACtrlTc1b(0) |                  //TC_MSK
		 mDMACtrlDMAFFTH3b(0) |             //DMA FIFO threshold value
		 mDMACtrlChpri2b(0) |               //Channel priority
		 mDMACtrlCache1b(0) |               //PROTO3
		 mDMACtrlBuf1b(0) |                 //PROTO2
		 mDMACtrlPri1b(0) |                 //PROTO1
		 mDMACtrlSsz3b(DMA_BURST_8) |       //Source burst size selection
		 mDMACtrlAbt1b(0) |                 //ABT 
		 mDMACtrlSwid3b(DMA_WIDTH_4) |      //SRC_WIDTH
		 mDMACtrlDwid3b(DMA_WIDTH_4) |      //DST_WIDTH
		 mDMACtrlHW1b(1) |                  //Hardware Handshake
		 mDMACtrlSaddr2b(DMA_ADD_FIX) |     //Memory is the source
		 mDMACtrlDaddr2b(DMA_ADD_INC) |     //NOR is the destination
		 mDMACtrlSsel1b(0) |                //AHB Master 0 is the source
		 mDMACtrlDsel1b(1) |                //AHB Master 1 is the destination
		 mDMACtrlEn1b(1)),                  //Channel Enable

		(mDMACfgDst5b(0)    |               //Memory
		 mDMACfgSrc5b(0x10 | REQ_SPI020TX) |               //bit13, 12-9; SSP0_RX:0
		 mDMACfgAbtDis1b(0) |               //Channel abort interrupt mask. 0 for not mask
		 mDMACfgErrDis1b(0) |               //Channel error interrupt mask. 0 for not mask
		 mDMACfgTcDis1b(0))                 //Channel terminal count interrupt mask. 0 for not mask
	}	
};

void Dma_init(INT32U sync)
{
	volatile ipDMA *ptDMA = (volatile ipDMA *)REG_DMA_BASE;
	// config master 1 as little endian
	// config master 0 as little endian
	ptDMA->csr = BIT0;	// dma enable
	ptDMA->sync = sync;
}

INT32S Dma_go_trigger(INT32U * src, INT32U *dst, INT32U byte, DMA_LLPD *pllpd,INT32U req, int real)
{
	volatile ipDMA *ptDMA = (volatile ipDMA *)REG_DMA_BASE;
	INT32U ctrl;
	volatile CHANNELx * ch;
	INT32S hw_ch;	// hardware channel no

	if (byte >= 4096*1024)  //4M
		return -1;
	// req[7:0]: hardware channel no
	hw_ch = req & 0xFF;
	// req[15:8]: request no
	req >>= 8;
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
	if (!real)  //dummy dma
	{
	    ctrl &= ~(0xF << 3);
	    ctrl |= mDMACtrlSaddr2b(DMA_ADD_FIX);
		ctrl |= mDMACtrlDaddr2b(DMA_ADD_FIX);
	}
		
	ch->size = byte >> mDmaCtrlSrcWidth_mask (ctrl);
            
	if(pllpd) {
		ch->llp = ((INT32U)pllpd)|mDmaCtrlSel_mask(ctrl);
		ctrl |= 0x80000000;
	}
	else {
		ch->llp = 0;	// end this chain
	}

	ch->ctrl= ctrl;
   
	return hw_ch;
}

///////////////////////////////////////////////////////////////////////////////
//		eBGDWaitFinish()
//		Description:
//			1. Waitting for DMA finish its job
//		input:	BGD-channel-no, polling time
//		output:	BGD_RET_NO_ERROR, BGD_RET_ERROR, BGD_RET_TIMEOUT
///////////////////////////////////////////////////////////////////////////////
INT32S Dma_wait_timeout(INT32S hw_ch, INT32U timeout)
{
	volatile ipDMA *ptDMA = (volatile ipDMA *)REG_DMA_BASE;
	INT32U count = timeout;
	INT32U bit = BIT0 << hw_ch;
	
	// waitting for dma finish
	while(1) {
		// finish
		if (ptDMA->tc  & bit) {
			return 0;
		}
	
		// error/abort
		if (ptDMA->err  & (bit | bit << 8)) {
			ptDMA->int_err_clr = (bit | bit << 8);
			break;
		}
		if (timeout) {
			if ((-- count) == 0)
				break;
		}
	};
	// stop dma
	ptDMA->ch[hw_ch].ctrl &= BIT0;
	return -1;
}

/* req: DMA_MEM2MEM / DMA_MEM2NAND ....
 * burst: DMA_BURST_1 ~ DMA_BURST_256
 * return:
 *  >= 0 for success
 */
int Dma_Set_BurstSz(INT32U req, int burst)
{
	INT32U  val;
	
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