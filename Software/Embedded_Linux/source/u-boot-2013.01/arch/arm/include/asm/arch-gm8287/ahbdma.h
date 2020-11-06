/*
 * (C) Copyright 2013 Faraday Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __AHBDMA_H
#define __AHBDMA_H

#define DMA_MAX			8
//DMA Source size
#define DMA_BURST_1			0x0L
#define DMA_BURST_4			0x1L
#define DMA_BURST_8			0x2L
#define DMA_BURST_128		0x6L
#define DMA_BURST_256		0x7L

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
//#define mDMACfgLLP4b(v)               ((v) << 16)
//#define mDMACfgBusy1b(v)      ((v) << 8)
#define mDMACfgDst5b(v)	    ((v) << 9)
#define mDMACfgSrc5b(v)	    ((v) << 3)
#define mDMACfgAbtDis1b(v)	((v) << 2)
#define mDMACfgErrDis1b(v)	((v) << 1)
#define mDMACfgTcDis1b(v)	((v) << 0)

// LLP Register
#define mDMALLPaddr30b(v)	((v) << 2)
#define mDMALLPmaster1b(v)	((v) << 0)

// DMA Data width
#define DMA_WIDTH_1		0x0L    // 1 Byte
#define DMA_WIDTH_2		0x1L    // 2 Bytes
#define DMA_WIDTH_4		0x2L    // 4 Bytes

// DMA Address control
#define DMA_ADD_INC		0x0L
#define DMA_ADD_DEC		0x1L
#define DMA_ADD_FIX		0x2L

#define mDmaCtrlSrcWidth_mask(ctrl)		(((ctrl) >> 11) & 3)
#define mDmaCtrlSel_mask(ctrl)		    (((ctrl) >> 2) & 1)
#define TIMEOUT_DMA				0x8000

#ifndef __ASSEMBLY__
typedef struct {
		unsigned int ctrl;
		unsigned int cfg;
		unsigned int *src;
		unsigned int *dst;
		unsigned int llp;
		unsigned int size;
		unsigned int reserved[2];
} CHANNELx;
	
typedef struct {
		unsigned int ints;			// 0x00
		unsigned int int_tc;
		unsigned int int_tc_clr;
		unsigned int int_err;
		unsigned int int_err_clr;		// 0x10
		unsigned int tc;
		unsigned int err;
		unsigned int ch_en;
		unsigned int ch_busy;			// 0x20
		unsigned int csr;
		unsigned int sync;			//0x28
		unsigned int reserved[53];
		CHANNELx ch[8];		//0x100
} ipDMA;

typedef struct {
		unsigned int src_addr;
		unsigned int dst_addr;
		unsigned int llp;
		unsigned int llp_ctrl;
		unsigned int size;
}DMA_LLPD;

int Dma_go_trigger(unsigned int * src, unsigned int * dst, unsigned int byte, DMA_LLPD * pllpd, unsigned int req, int real);
int Dma_wait_timeout(int hw_ch, unsigned int timeout);
int Dma_Set_BurstSz(unsigned int req, int burst);

#endif	/* __ASSEMBLY__ */
#endif	/* __AHBDMA_H */
