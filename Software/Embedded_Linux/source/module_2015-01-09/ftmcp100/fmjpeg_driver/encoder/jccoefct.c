#ifdef LINUX
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/hdreg.h>	/* HDIO_GETGEO */
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#endif
/*
 * jccoefct.c
 *
 * Copyright (C) 1994-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the coefficient buffer controller for compression.
 * This controller is the top level of the JPEG compressor proper.
 * The coefficient buffer lies between forward-DCT and entropy encoding steps.
 */

#define JPEG_INTERNALS
#include "jenclib.h"
#include "../common/jinclude.h"

#define CCATmp0 ((JE_DMA_CMD_OFF0&0xfffffff0) | 0x2)
#define CCATmp1 (((JE_DMA_CMD_OFF0 + 160)&0xfffffff0) | 0x2)
#define dmactrl (0xa<<20 | 0x1<<26)
#define mectrl ((1<<7) | (1<<6) | (1<<5) | (1<<4) | (1<<3) | (1<<1) | 1)
#define MCCTL_CONST ((1<<24) | (1 << 9) | (1 << 1))	//set encoding mode and JPEG_mode and INTRA

extern void fmcp100_resetrun(unsigned int base);
//#define POLLING
#ifdef TWO_P_EXT
extern int je_int;
#ifndef SUPPORT_VG
extern wait_queue_head_t mcp100_codec_queue;
void compress_YUV_block (j_compress_ptr cinfo)
{
	#ifdef POLLING
		#ifdef SEQ
		{
			int sr;
			do {
				int i;
				for (i=0; i<500; i++)
					;
				READ_CPSTS(cinfo->VirtBaseAddr, sr)
			} while((sr & (1L << 23)) == 0)
		}
		#else
		{
			Share_Node_JE *node = (Share_Node_JE *)(SHARE_MEM_ADDR+cinfo->VirtBaseAddr);
		
			while(node->jpgenc_done == 0) {
				int i;
				for (i=0; i<500; i++)
					;
			}
		}
		#endif
	#else
		int hw_time = 2;
		#ifdef FIE8150	// FPGA
			hw_time = 10;
		#endif
		wait_event_timeout(mcp100_codec_queue, je_int != IT_JPGE, msecs_to_jiffies(hw_time* 100));
	#endif
}
#endif

#ifdef SEQ
static void jc_dma_offset(j_compress_ptr cinfo)
{
	volatile MDMA *pmdma = MDMA1(cinfo->VirtBaseAddr);
	jpeg_compress_struct_my * my = &cinfo->pub;
	int ci;
	jpeg_component_info *cmpp;
	unsigned int addr;
	unsigned int * dma_cmd = (unsigned int *) (JE_DMA_CMD_OFF0 + cinfo->VirtBaseAddr);
	unsigned int mbx = my->roi_left_mb_x;
	unsigned int mby = my->roi_left_mb_y;
	switch ( my->u82D) {
		case 0:		// mp42D, must JCS_yuv420, 400
			dma_cmd[0] = (my->YUVAddr[0] + (mby*my->image_width + mbx*16)*16) | 7;// incre 256
			dma_cmd[4] = (my->YUVAddr[1] + (mby*my->image_width/2 + mbx*8)*8) | 5;// incre 64
			dma_cmd[8] = (my->YUVAddr[2] + (mby*my->image_width/2 + mbx*8)*8) | 5;// incre 64
			pmdma->RowOffYUV[0] = 256 + (my->image_width - cinfo->roi_w)*16;
			pmdma->RowOffYUV[1]= 64 + (my->image_width - cinfo->roi_w)*8/2;
			pmdma->RowOffYUV[2]= 64 + (my->image_width - cinfo->roi_w)*8/2;
			break;
		case 1: 	// sequential 1D
			for (ci = 0, cmpp = cinfo->cur_comp_info[0]; ci < my->comps_in_scan; ci++, cmpp ++) {
				addr = my->YUVAddr[ci] +
					(mby*my->MCUs_per_row*cmpp->vs_fact*8+ mbx)*cmpp->hs_fact*8;
				dma_cmd[4*ci] = addr | cmpp->hs_fact;
				pmdma->RowOffYUV[ci] = (my->MCUs_per_row * (cmpp->vs_fact*8 - 1) + 1 +
								(my->MCUs_per_row - (my->roi_right_mb_x - my->roi_left_mb_x))) * cmpp->hs_fact*8;
			}
			break;
		case 2:		// H264 2D, must JCS_yuv420, 400
			dma_cmd[0] = (my->YUVAddr[0] + (mby*my->image_width + mbx*16)*16) | 7;	// incre 512
			dma_cmd[4] = (my->YUVAddr[1] + (mby*my->image_width + mbx*16)*8) | 6;		// incre 256
			pmdma->RowOffYUV[0] = 256 + (my->image_width - cinfo->roi_w)*16;
			pmdma->RowOffYUV[1]= 128 + (my->image_width - cinfo->roi_w)*8/2*2;
			break;
		default:
			break;
	}
}
void compress_YUVsTri (j_compress_ptr cinfo)
{
	jc_dma_offset (cinfo);
	SET_MCCTL(cinfo->VirtBaseAddr, MCCTL_CONST)
	mSEQ_EXTMSK(cinfo->VirtBaseAddr) = 0xE00;		// Frame Done (0x200) FFD7 (0x400) sys_buf_full(0x800) interrupt mask	
	mSEQ_DIMEN(cinfo->VirtBaseAddr) = (cinfo->pub.sample << 26) |
						((cinfo->pub.roi_right_mb_y - cinfo->pub.roi_left_mb_y-1) << 13) |
						((cinfo->pub.roi_right_mb_x - cinfo->pub.roi_left_mb_x-1) << 0);
	mSEQ_RS_ITV(cinfo->VirtBaseAddr) = cinfo->pub.restart_interval;

	je_int = IT_JPGE;
	mSEQ_CTL(cinfo->VirtBaseAddr) = 2;		// start sequencer
	#ifdef EVALUATION_PERFORMANCE
		encframe.hw_start = get_counter();
	#endif
}

unsigned int compress_YUVsEnd (j_compress_ptr cinfo)
{
	if (je_int & (1L << 26))
		printk("[jpeg_enc]:FFD7 meet\n");
	if (je_int & (1L << 25)) 	// frame done
		return mSEQ_BS_LEN(cinfo->VirtBaseAddr);
	if (je_int & (1L << 27)) {
		//printk("[jpeg_enc]: buffer full\n");
		// system address setting
		// system buffer size setting
		fmcp100_resetrun (cinfo->VirtBaseAddr);
		return -1;
	}
	else {
		printk("[jpeg_enc]: time out, 0x%08x, 0x%x\n", mSEQ_INTSTS (cinfo->VirtBaseAddr), mSEQ_CTL(cinfo->VirtBaseAddr));
		return 0;
	}
}

#if 0
//#ifndef SUPPORT_VG
unsigned int compress_YUVs (j_compress_ptr cinfo)
{
	compress_YUVsTri(cinfo);

	#ifdef POLLING
		{
			int sr;
			do {
				int i;
				for (i=0; i<500; i++)
					;
				READ_CPSTS(cinfo->VirtBaseAddr, sr)
			} while((sr & (1L << 23)) == 0)
		}
		return mSEQ_BS_LEN(cinfo->VirtBaseAddr);
	#else
		wait_event_timeout(mcp100_codec_queue, je_int != IT_JPGE, msecs_to_jiffies(200));
		return compress_YUVsEnd(cinfo);
	#endif
}
#endif
#else
int compress_YUVeTri (j_compress_ptr cinfo)
{
	int ci;
	Share_Node_JE *node = (Share_Node_JE *)(SHARE_MEM_ADDR+cinfo->VirtBaseAddr);

	#if !(defined(GM8120) || defined(GM8180) || defined(GM8185))
	mSEQ_EXTMSK(cinfo->VirtBaseAddr) = 0x800;		// sys_buf_full(0x800) interrupt mask
	#endif
	// check embedded CPU ready
	if ( (node->command & 0x0F) != WAIT_FOR_COMMAND) {
		printk ("compress_YUVeTri is not issued due to mcp100 is busy\n");
		return -1;
	}
	node->pub = cinfo->pub;
	for(ci=0;ci<cinfo->pub.comps_in_scan;ci++) {
		node->comp_info[ci] = *cinfo->cur_comp_info[ci];
	}

	node->jpgenc_done = 0;
	je_int = IT_JPGE;
	node->command = ENCODE_JPEG;	    // trigger MCP100
	#ifdef EVALUATION_PERFORMANCE
		encframe.hw_start = get_counter();
	#endif
	return 0;
}
unsigned int compress_YUVeEnd (j_compress_ptr cinfo)
{
	Share_Node_JE *node = (Share_Node_JE *)(SHARE_MEM_ADDR+cinfo->VirtBaseAddr);
	if (je_int & (1L << 31))	// embedded CPU frame done
		return node->bitslen;
	else if (je_int & (1L << 27)) {
		//printk("[jpeg_enc]: buffer full\n");
		// 1. system address setting
		// 2. system buffer size setting -> hw resume
		fmcp100_resetrun (cinfo->VirtBaseAddr);
		return -1;
	}
	else {
		printk("[jpeg_enc]: time out, je_int = 0x%x\n", je_int);
		return 0;
	}
}
#if 0
//#ifndef SUPPORT_VG
unsigned int compress_YUVe (j_compress_ptr cinfo)
{
	if (compress_YUVeTri (cinfo) < 0)
		return 0;		// fail

	#ifdef POLLING
		while(node->jpgenc_done == 0) {
			int i;
			for (i=0; i<500; i++)
				;
		}
	#else
		wait_event_timeout(mcp100_codec_queue, je_int != IT_JPGE, msecs_to_jiffies(200));
	#endif
	return compress_YUVeEnd(cinfo);
}
#endif

#endif	// SEQ

#else		//  (defined(ONE_P_EXT) || (defined(TWO_P_INT)))
#ifdef TWO_P_INT
#define BASE_ADDRESS 0
#endif

void
mcudma_each(jpeg_compress_struct_my * my, unsigned int mbx, unsigned int mby,
			unsigned int * dma_cmd, jpeg_component_info * cur_comp_info[MAX_COMPS_IN_SCAN])
{
	int ci;
	jpeg_component_info *compptr;
	unsigned int addr;
	switch ( my->u82D) {
		case 0:		// mp42D, must JCS_yuv420, 400
			dma_cmd[0] = (my->YUVAddr[0] + (mby*my->image_width + mbx*2*8)*2*8) | 7;	// incre 512
			dma_cmd[4] = (my->YUVAddr[1] + (mby*my->image_width/2 + mbx*1*8)*1*8) | 5;	// incre 128
			dma_cmd[8] = (my->YUVAddr[2] + (mby*my->image_width/2 + mbx*1*8)*1*8) | 5;	// incre 128
			break;
		case 2:		// H264 2D, must JCS_yuv420, 400
			dma_cmd[0] = (my->YUVAddr[0] + (mby*my->image_width + mbx*2*8)*2*8) | 7;	// incre 512
			dma_cmd[4] =	(my->YUVAddr[1] + (mby*my->image_width + mbx*2*8)*1*8) | 6;		// incre 256
			break;
		default:		// sequential 1D
			for (ci = 0, compptr = cur_comp_info[0]; ci < my->comps_in_scan; ci++, compptr ++) {
				addr = my->YUVAddr[ci] +
					(mby*my->MCUs_per_row*compptr->vs_fact*8+ mbx)*compptr->hs_fact*8;
				dma_cmd[4*ci] = addr | compptr->hs_fact;
		}
	}
}

/*
 * Finish JPEG compression.
 *
 * If a multipass operating mode was selected, this may do a great deal of
 * work including most of the actual output.
 */
int jpeg_finish_compress (jpeg_compress_struct_my * my)
{
	unsigned int bitslen;
	int i;
	int x, y, smaddr, reg_temp;

	/* byte align & Write EOI, do final cleanup */
	write_file_trailer1(BASE_ADDRESS);
	FA526_DrainWriteBuffer();
	// wait for auto-buffer-clean
	do {
		READ_VLDSTS(BASE_ADDRESS, reg_temp)
	} while(!(reg_temp&1024));

	//Read the system mem pointer and local mem pointer
	READ_ABADR(BASE_ADDRESS, smaddr)			//Read system memory pointer (64 byte)
	READ_BALR (BASE_ADDRESS, x)			//Read local memory pointer (words)
	READ_VADR (BASE_ADDRESS, y)			    //Read local memory pointer (bits)      

	x = x & 0x3f;
	y = y & 0xff;
	y = y/8 + x;

	smaddr -= my->bs_phy;		//total bytes in system memory
	bitslen = y + smaddr;
	for(i=32;i!=0;i--)
		BitstreamPutBits(0x0, 16, BASE_ADDRESS);

	FA526_DrainWriteBuffer();
	// wait for auto-buffer-clean
	do {
		READ_VLDSTS(BASE_ADDRESS, reg_temp)
	} while(!(reg_temp&1024));
	SET_VLDCTL(BASE_ADDRESS, 1<<5)		//ABF stop 
	return bitslen;
}

#define BIT_DMA (1L<<0)
#define BIT_VLC_MC (1L<<1)
unsigned int compress_YUV (jpeg_compress_struct_my *my,  jpeg_component_info * jci[MAX_COMPS_IN_SCAN])
{
	unsigned int sreg;
	unsigned int pipe = 0;
	volatile MDMA *pmdma = MDMA1(BASE_ADDRESS);
	int mbx = my->roi_left_mb_x-1;
	int mby = my->roi_left_mb_y;
	int mbpos = -1;
	int restart2go;
	int next_restart_num = 0;

	if (my->restart_interval == 0)
		restart2go = 0x7FFFFFFF;		// max pos. value, never reach to 0
	else
		restart2go = my->restart_interval + 1;	// for 1st time
	while (1) {
		if (pipe & BIT_VLC_MC) {
			pipe &= ~BIT_VLC_MC;
			// wait for MC & VLC done
			do {
				READ_CPSTS(BASE_ADDRESS, sreg)
			} while( !( (sreg & 32) && (sreg & 2))  ); // sync VLC_DONE and MC_DONE
		}

		if (pipe & BIT_DMA) {
			pipe &= ~BIT_DMA;
			pipe |= BIT_VLC_MC;
			// wait for DMA
			while((pmdma->Status & 0x1) == 0)
				;
		}

		if (pipe & BIT_VLC_MC) {
			// start ME
#if 0
			if (my->u32MotionDetection==1) {
				unsigned int cal_reg;
				SET_MECADDR(BASE_ADDRESS, (mbpos & 1)?CUR_B0+STRIDE_MCU: CUR_B0)
				SET_MECTL(BASE_ADDRESS, mectrl)
				FA526_DrainWriteBuffer();
				do {
           	  		READ_CPSTS(BASE_ADDRESS, sreg)		
           		} while(!(sreg&8));   //sync ME-DONE
				READ_CURDEV(BASE_ADDRESS, cal_reg)
				my->dev_buffer_virt[mbpos] = cal_reg;
			}
#endif
			if (--restart2go == 0) {
				restart2go = my->restart_interval;
				emit_restart(BASE_ADDRESS, (next_restart_num++) & 7);
				SET_PYDCR(BASE_ADDRESS, 0)
				SET_PUVDCR(BASE_ADDRESS, 0)
			}

			SET_MCCADDR(BASE_ADDRESS, (mbpos & 1)?CUR_B0+STRIDE_MCU: CUR_B0)
			SET_MCCTL(BASE_ADDRESS, MCCTL_CONST | 1)		// with MC_GO
		}

		++ mbpos;
		if (++ mbx == my->roi_right_mb_x) {
			mbx = my->roi_left_mb_x;
			++ mby;
		}
		if (mby < my->roi_right_mb_y) {
			pipe |= BIT_DMA;
			if ((mbx == my->roi_left_mb_x) ||
				(mbx == (my->roi_left_mb_x+1))) {
				unsigned int * dma_cmd = (unsigned int *) (JE_DMA_CMD_OFF0 + BASE_ADDRESS);
				mcudma_each(my, mbx, mby, &dma_cmd[(mbpos & 1)*40], jci);
			}
			pmdma->CCA = (mbpos & 1)? CCATmp1: CCATmp0;
			pmdma->Control =  dmactrl;		//start DMA	
		}
		if (pipe == 0)
			break;
	}
	return jpeg_finish_compress(my);
}
#endif

