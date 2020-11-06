/*
 * jdcoefct.c
 *
 * Copyright (C) 1994-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the coefficient buffer controller for decompression.
 * This controller is the top level of the JPEG decompressor proper.
 * The coefficient buffer lies between entropy decoding and inverse-DCT steps.
 *
 * In buffered-image mode, this controller is the interface between
 * input-oriented processing and output-oriented processing.
 * Also, the input side (only) is used when reading a file for transcoding.
 */

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
#include <linux/time.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#endif

#include "../common/jinclude.h"

#include "jdeclib.h"
#include "ioctl_jd.h"
#include "ftmcp100.h"
extern void fmcp100_resetrun(unsigned int base);
extern int dump_n_frame;


#ifdef EVALUATION_PERFORMANCE
#include "../common/performance.h"
extern FRAME_TIME decframe;
extern unsigned int get_counter(void);
#endif


#ifdef TWO_P_INT
#define ADDRESS_BASE 0
#endif

#ifdef TWO_P_EXT

void vld_err_meaning(int reg) {
	switch(reg)  {
		case 0x0: // no error
			break; 
		case 0x1:
			F_DEBUG("VLDSTS mode ERR\n");
			break;
		case 0x2:
			F_DEBUG("VLDSTS cbpy ERR\n");
			break;
		case 0x3:
			F_DEBUG("VLDSTS MVD ERR\n");
			break;
		case 0x4:
			F_DEBUG("VLDSTS DC ERR\n");
			break;
		case 0x5:
			F_DEBUG("VLDSTS AC ERR\n");
			break;
		case 0x6:
			F_DEBUG("VLDSTS vop ERR\n");
			break;
		case 0x0E:
			F_DEBUG("VLDSTS timeout ERR\n");	
			break;
		default : 
			F_DEBUG("VLDSTS other ERR\n");	
			break;
	}
}
//#define POLLING
#ifdef LINUX
	extern wait_queue_head_t mcp100_codec_queue;
	extern int jd_int;
#endif
#ifdef SEQ
static void jd_dma_init(j_decompress_ptr dinfo)
{
	int ci;
	int comp_w_off;
	unsigned int local = CUR_B0;
	unsigned int * dma_cmd = (unsigned int *) (JD_DMA_CMD_OFF0 + ADDRESS_BASE);
	jpeg_component_info *cmpp;
	for (ci = 0, cmpp = dinfo->cur_comp_info[0]; ci < dinfo->pub.comps_in_scan; ci++, cmpp++) {
		unsigned int dma_1, dma_2, dma_3;
		int blk_sz = cmpp->hs_fact*cmpp->vs_fact;
		comp_w_off = (dinfo->pub.MCUs_per_row -1) * cmpp->hs_fact*DCTSIZE;
		dma_1 = (8 << 28) |			// Loc3Dwidth, 8 lines in one block
						(((1 - (cmpp->hs_fact - 1)*DCTSIZE2/4) & 0xFF)<<20) |	// Loc2Doff
						(cmpp->hs_fact << 16) |										// Loc2Dwidth
						mDmaLocMemAddr14b (local);
		dma_2 = ((DCTSIZE2/4 + 1 - DCTSIZE/4)<<24) |	// LocOff
						((DCTSIZE/4)<<20)	| 									// LocWidth
						mDmaSysOff14b(comp_w_off/4+1) |				// SysOff
						mDmaSysWidth6b(cmpp->hs_fact*DCTSIZE/8);
		blk_sz *= DCTSIZE2;
		dma_cmd[1] = dma_1;
		dma_cmd[2] = dma_2;
		dma_3 = 
					(1 << 28) |			// Loc3Doff: 1, last line of blk1 to first line of blk2
					(0x9<<20) |		// local->sys, dma_en
					(0xd<<16) |		// sys:2d, local: 4d
					(blk_sz/4);
		if (ci != (dinfo->pub.comps_in_scan - 1))// not last one comp.
			dma_3 |=  ((0x1<<26) |	// mask interrupt
							(1 << 21));		// chain enable
		dma_cmd[3] = dma_3;
		dma_cmd += 4;
		local += blk_sz;
	}
}
static void jd_dma_offset(j_decompress_ptr dinfo)
{
	volatile MDMA *pmdma = MDMA1(ADDRESS_BASE);
	jpeg_decompress_struct_my * my = &dinfo->pub;
	int ci;
	jpeg_component_info *cmpp;
	unsigned int addr;
	unsigned int mbx = my->crop_x;
	unsigned int mby = my->crop_y;
	unsigned int * dma_cmd = (unsigned int *) (JD_DMA_CMD_OFF0 + ADDRESS_BASE);

	for (ci = 0, cmpp = dinfo->cur_comp_info[0]; ci < my->comps_in_scan; ci++, cmpp ++) {
//printk("%d: outdata:0x%x\n", ci, (int)my->outdata[ci]);
		addr = (unsigned int )my->outdata[ci] +
			(mby*my->MCUs_per_row*cmpp->vs_fact*8+ mbx)*cmpp->hs_fact*8;
		dma_cmd[4*ci] = addr | cmpp->hs_fact;
		pmdma->RowOffYUV[ci] = (my->MCUs_per_row * (cmpp->vs_fact*8 - 1) + 1 +
						(my->MCUs_per_row - (my->crop_xend - my->crop_x))) * cmpp->hs_fact*8;
	}
}
#ifdef SUPPORT_VG
int decompress_YUVsTri (j_decompress_ptr dinfo)
{
	int sr;
	int sample_alias = dinfo->pub.sample;
	if (sample_alias == JDS_400)
		sample_alias = JDS_420;

	READ_CPSTS(ADDRESS_BASE, sr)
	if ((sr & (1L << 23)) == 0) {
		printk("seqencer still working. but do encode again.\n");
		return -1;
	}
	jd_dma_init (dinfo);
	jd_dma_offset (dinfo);
	mSEQ_DIMEN(ADDRESS_BASE) = (sample_alias << 26) |
							((dinfo->pub.crop_yend - dinfo->pub.crop_y-1) << 13) |
							((dinfo->pub.crop_xend - dinfo->pub.crop_x-1) << 0);
	mSEQ_RS_ITV(ADDRESS_BASE) = dinfo->pub.restart_interval;
	jd_int = IT_JPGD;
	mSEQ_EXTMSK(ADDRESS_BASE) = 0x3200;		// Frame Done (0x200) VLDerr (0x2000) sys_buf_empty(0x1000) interrupt mask	

    if (dump_n_frame)                                       // Tuba 20110720_0: add debug proc
        printk("allocate memory %d\n", dinfo->mem_cost);    // Tuba 20110720_0: add debug proc
	mSEQ_CTL(ADDRESS_BASE) = 2;		// start sequencer
	return 0;
}
int decompress_YUVsEnd(j_decompress_ptr dinfo)
{
	if (jd_int & (1L << 29)) {
		printk("[jpeg_dec]: VLD error (%08x)\n", jd_int);
        // Tuba 20110714_0 start: add vld error handle
        fmcp100_resetrun(dinfo->VirtBaseAddr);
		return -1;
		//return 0;			// not report error to caller
		// Tuba 20110714_0 end
	}
	if (jd_int & (1L << 28)){
		printk("[jpeg_dec]: bs not enought\n");
		fmcp100_resetrun(dinfo->VirtBaseAddr);
		return -1;
	}
	if (jd_int & (1L << 25))	// frame done
		return 0;
	else {
		printk("[jpeg_dec]: time out, 0x%08x, 0x%x\n", mSEQ_INTSTS (ADDRESS_BASE), mSEQ_CTL(ADDRESS_BASE));
		return -1;
	}
}
#else   // else of "#ifdef SUPPORT_VG"
extern void FJpegDec_fill_bs(void *dec_handle, void * src_init, int bs_sz_init);
int decompress_YUVs (j_decompress_ptr dinfo)
{
	int sr;
	int sample_alias = dinfo->pub.sample;
	if (sample_alias == JDS_400)
		sample_alias = JDS_420;
	READ_CPSTS(ADDRESS_BASE, sr)
	if ((sr & (1L << 23)) == 0) {
		printk("seqencer still working. but do encode again.\n");
		return -1;
	}
	jd_dma_init (dinfo);
	jd_dma_offset (dinfo);
	mSEQ_DIMEN(ADDRESS_BASE) = (sample_alias << 26) |
							((dinfo->pub.crop_yend - dinfo->pub.crop_y-1) << 13) |
							((dinfo->pub.crop_xend - dinfo->pub.crop_x-1) << 0);
	mSEQ_RS_ITV(ADDRESS_BASE) = dinfo->pub.restart_interval;

	#if !defined (LINUX) || defined(POLLING)
	{
		mSEQ_CTL(ADDRESS_BASE) = 2;		// start sequencer
		do {
			int i;
			for (i=0; i<500; i++)
				;
			READ_CPSTS(ADDRESS_BASE, sr)
		} while((sr & (1L << 23)) == 0)
	}
	#elif 0
		mSEQ_CTL(ADDRESS_BASE) = 2;		// start sequencer
 		// check interrupt status
	 	do {
			int aa;
			volatile MDMA *pmdma = MDMA1(ADDRESS_BASE);
       		READ_CPSTS(ADDRESS_BASE, sr)
//printk("sts: 0x%08x\n", sr);
			if ( sr & 0x800000 ) { 					// FRAME_DONE, 23
//printk("FRAME_DONE\n");
				SET_INT_FLAG(ADDRESS_BASE, 0x800000)				// clear FRAME_DONE interrupt flag
				break;
			}
			else if ( sr & 0x4000000 ) {				// SYS BUFFER EMPTY, 26
				SET_INT_FLAG(ADDRESS_BASE, 0x10000000)			// clear SYS_BUF_EMPTY interrupt flag
//				READ_ABADR(ADDRESS_BASE, aa)
//printk("jpeg buffer empty, %x 0x%x\n", (int)dinfo->pu8BitstreamAddr+ off_jd, aa);
//				SET_ABADR(ADDRESS_BASE, dinfo->pu8BitstreamAddr+ off_jd)
				dj_fill_bs(dinfo, 0, 0);
			}
			else if ( sr & 0x8000000 ) {				// VLD ERR, 27
				SET_INT_FLAG(ADDRESS_BASE, 0x20000000)			// clear VLD_ERROR interrupt flag
				printk("VLD error\n");
				break;
			}
	} while(!(sr&0x800000));		
	#else
		jd_int = IT_JPGD;
		mSEQ_EXTMSK(ADDRESS_BASE) = 0x3200;		// Frame Done (0x200) VLDerr (0x2000) sys_buf_empty(0x1000) interrupt mask	
		mSEQ_CTL(ADDRESS_BASE) = 2;		// start sequencer
		#ifdef EVALUATION_PERFORMANCE
       		encframe.hw_start = get_counter();
		#endif
		do {
			int hw_time = 2;
			#ifdef FIE8150	// FPGA
				hw_time = 10;
			#endif
			wait_event_timeout(mcp100_codec_queue, jd_int != IT_JPGD, msecs_to_jiffies(hw_time * 100));
			if (jd_int & (1L << 29)) {
				printk("VLD error\n");
				return 0;			// not report error to caller
			}
			if (jd_int & (1L << 28))
				FJpegDec_fill_bs((void*)dinfo, (void *)0, 0);
			if (jd_int & (1L << 25))	// frame done
				break;
			else if ((jd_int & (1L << 28)) == 0){
				printk("jpeg decoder time out, 0x%08x, 0x%x\n", mSEQ_INTSTS (ADDRESS_BASE), mSEQ_CTL(ADDRESS_BASE));
				return -1;
			}
			jd_int = IT_JPGD;
		} while (jd_int == IT_JPGD);
	#endif
	return 0;
}
#endif  // end of "#ifdef SUPPORT_VG"
#else   // else of "#ifdef SEQ"
int decompress_YUVeTri (j_decompress_ptr dinfo)
{
	int ci;
	Share_Node_JD *node = (Share_Node_JD *)(SHARE_MEM_ADDR+ADDRESS_BASE);

	// wait embedded CPU ready
	if( (node->command & 0x0F) != WAIT_FOR_COMMAND) {
		printk ("decompress_YUVeTri is not issued due to mcp100 is busy\n");
		return -1;
	}

	#if !(defined(GM8120) || defined(GM8180) || defined(GM8185))
	mSEQ_EXTMSK(ADDRESS_BASE) = 0x1000;		// sys_buf_empty(0x1000) interrupt mask
	#endif
	node->pub = dinfo->pub;
	for(ci=0;ci<dinfo->pub.comps_in_scan;ci++)
		node->comp_info[ci] = *dinfo->cur_comp_info[ci];

	node->internal_cpu_command=1; // to instruct the internal CPU to do interleaved decoding
	node->external_cpu_response=0;

	jd_int = IT_JPGD;
	node->command = DECODE_JPEG;		// trigger MCP100
	return 0;
}
int decompress_YUVeEnd (j_decompress_ptr dinfo)
{
	if (jd_int == (1L << 31))		// embedded CPU frame done
		return 0;
	else if (jd_int & (1L << 28)){
		printk("[jpeg_dec]: bs not enought\n");
		fmcp100_resetrun(dinfo->VirtBaseAddr);
	}
	else {
		printk("[jpeg_dec]: unknown error 0x%08x\n", jd_int);
	}
	return -1;
}

#ifndef SUPPORT_VG
int decompress_YUVe (j_decompress_ptr dinfo)
{
	if (decompress_YUVeTri (dinfo) < 0)
		return -1;
	#if !defined (LINUX) | defined(POLLING)
	{
		// to wait for internal CPU's response
		int ci = 0;
		Share_Node_JD *node = (Share_Node_JD *)(SHARE_MEM_ADDR+ADDRESS_BASE);
		while(node->external_cpu_response!=1) {
			int i;
			for(i=0;i<100; i++)
				;
			if (++ci >= 100)
				printk ("0x%x\n", node->pub.monitor);
			if (ci == 200)
				return -1;
		}
	}
	#else
		#ifdef EVALUATION_PERFORMANCE
			decframe.hw_start = get_counter();
		#endif
		{
			int hw_time = 2;
			#ifdef FIE8150	// FPGA
				hw_time = 10;
			#endif
			wait_event_timeout(mcp100_codec_queue, jd_int != IT_JPGD, msecs_to_jiffies(hw_time * 100));
			if (decompress_YUVeEnd (dinfo) < 0)
				return -1;
		}
	#endif
	return 0;
}
#endif	// end of "#ifndef SUPPORT_VG"      !SUPPORT_VG
#endif	// end of "#ifdef SEQ"                      SEQ
#endif	// end if "#ifdef TWO_P_EXT"            TWO_P_EXT

typedef struct
{
	int x;			// current mb x
	int y;			// current mb y
	int toggle;		// indicate which table will be used;
	int jump;		// indicate need to re-set dma_setting;
} MCU_s;
 //set JPEG_mode and DEC_GO and INTRA
#define START_VLD ((1 << 9) | (1 << 5) | (1<<1))
//set JPEG_mode and DMC_GO and INTRA
#define START_DQ_MC ((1 << 9) | (1<<10) | (1 << 1))
//set DT_enable and DT_GO 
#define START_DT ((1 << 9) | (1<<17) | (1<<18) | (1 << 1))

#if (defined(ONE_P_EXT) || (defined(TWO_P_INT)))
/* 
 * Data swapping function for CbYCrY output format. These function are
 * only support input bitstream source on yuv 211, 420 and 422 mode.
 * 
 */
static void JPG2MP4_4Y(unsigned int *src)
{
	unsigned int tmp0, tmp1;
	//MCB_SWAP(src, 2 , 4);
	tmp0 = *(src+2);
	tmp1 = *(src+3);
	*(src+2)= *(src+4);
	*(src+3)= *(src+5);
	*(src+4)= tmp0;
	*(src+5)=tmp1;
	//MCB_SWAP(src, 2 , 8);
	tmp0 = *(src+2);
	tmp1 = *(src+3);
	*(src+2) = *(src+8);
	*(src+3) = *(src+9);
	*(src+8) = tmp0;
	*(src+9) = tmp1;
	//MCB_SWAP(src, 2 , 16);
	tmp0 = *(src+2);
	tmp1 = *(src+3);
	*(src+2) = *(src+16);
	*(src+3) = *(src+17);
	*(src+16) = tmp0;
	*(src+17) =tmp1;
	//MCB_SWAP(src, 6 , 12);
	tmp0 = *(src+6);
	tmp1 = *(src+7);
	*(src+6) = *(src+12);
	*(src+7) = *(src+13);
	*(src+12) = tmp0;
	*(src+13) =tmp1;
	//MCB_SWAP(src, 6 , 24);
	tmp0 = *(src+6);
	tmp1 = *(src+7);
	*(src+6) = *(src+24);
	*(src+7) = *(src+25);
	*(src+24) = tmp0;
	*(src+25) = tmp1;
	//MCB_SWAP(src, 6 , 18);
	tmp0 = *(src+6);
	tmp1 = *(src+7);
	*(src+6) = *(src+18);
	*(src+7) = *(src+19);
	*(src+18) = tmp0;
	*(src+19) = tmp1;
	//MCB_SWAP(src, 10, 20);
	tmp0 = *(src+10);
	tmp1 = *(src+11);
	*(src+10) = *(src+20);
	*(src+11) = *(src+21);
	*(src+20) = tmp0;
	*(src+21) = tmp1;
	//MCB_SWAP(src, 14, 28);
	tmp0 = *(src+14);
	tmp1 = *(src+15);
	*(src+14)= *(src+28);
	*(src+15)= *(src+29);
	*(src+28) = tmp0;
	*(src+29) = tmp1;
	//MCB_SWAP(src, 14, 26);
	tmp0 = *(src+14);
	tmp1 = *(src+15);
	*(src+14) = *(src+26);
	*(src+15) = *(src+27);
	*(src+26) = tmp0;
	*(src+27) = tmp1;
	//MCB_SWAP(src, 14, 22);		
	tmp0 = *(src+14);
	tmp1 = *(src+15);
	*(src+14) = *(src+22);
	*(src+15) = *(src+23);
	*(src+22) = tmp0;
	*(src+23) = tmp1;
	//MCB_SWAP(src, 34 , 36);
	tmp0 = *(src+34);
	tmp1 = *(src+35);
	*(src+34)= *(src+36);
	*(src+35)= *(src+37);
	*(src+36)= tmp0;
	*(src+37)= tmp1;
	//MCB_SWAP(src, 34 , 40);
	tmp0 = *(src+34);
	tmp1 = *(src+35);
	*(src+34)= *(src+40);
	*(src+35)= *(src+41);
	*(src+40)= tmp0;
	*(src+41)= tmp1;
	//MCB_SWAP(src, 34 , 48);
	tmp0 = *(src+34);
	tmp1 = *(src+35);
	*(src+34)= *(src+48);
	*(src+35)= *(src+49);
	*(src+48)= tmp0;
	*(src+49)= tmp1;
	//MCB_SWAP(src, 38 , 44);
	tmp0 = *(src+38);
	tmp1 = *(src+39);
	*(src+38)= *(src+44);
	*(src+39)= *(src+45);
	*(src+44)= tmp0;
	*(src+45)= tmp1;
	//MCB_SWAP(src, 38 , 56);
	tmp0 = *(src+38);
	tmp1 = *(src+39);
	*(src+38)= *(src+56);
	*(src+39)= *(src+57);
	*(src+56)= tmp0;
	*(src+57)= tmp1;
	//MCB_SWAP(src, 38 , 50);
	tmp0 = *(src+38);
	tmp1 = *(src+39);
	*(src+38)= *(src+50);
	*(src+39)= *(src+51);
	*(src+50)= tmp0;
	*(src+51)= tmp1;
	//MCB_SWAP(src, 42, 52);
	tmp0 = *(src+42);
	tmp1 = *(src+43);
	*(src+42)= *(src+52);
	*(src+43)= *(src+53);
	*(src+52)= tmp0;
	*(src+53)= tmp1;
	//MCB_SWAP(src, 46, 60);
	tmp0 = *(src+46);
	tmp1 = *(src+47);
	*(src+46)= *(src+60);
	*(src+47)= *(src+61);
	*(src+60)= tmp0;
	*(src+61)= tmp1;
	//MCB_SWAP(src, 46, 58);
	tmp0 = *(src+46);
	tmp1 = *(src+47);
	*(src+46)= *(src+58);
	*(src+47)= *(src+59);
	*(src+58)= tmp0;
	*(src+59)= tmp1;
	//MCB_SWAP(src, 46, 54);
	tmp0 = *(src+46);
	tmp1 = *(src+47);
	*(src+46)= *(src+54);
	*(src+47)= *(src+55);
	*(src+54)= tmp0;
	*(src+55)= tmp1;
	return;
}

static void JPG2MP4_2Y(unsigned int *src)
{
	unsigned int tmp0, tmp1;
	
	//MCB_SWAP(src, 2 , 4);
	tmp0 = *(src+2);
	tmp1 = *(src+3);
	*(src+2)= *(src+4);
	*(src+3)= *(src+5);
	*(src+4)= tmp0;
	*(src+5)=tmp1;
	//MCB_SWAP(src, 2 , 8);
	tmp0 = *(src+2);
	tmp1 = *(src+3);
	*(src+2) = *(src+8);
	*(src+3) = *(src+9);
	*(src+8) = tmp0;
	*(src+9) = tmp1;
	//MCB_SWAP(src, 2 , 16);
	tmp0 = *(src+2);
	tmp1 = *(src+3);
	*(src+2) = *(src+16);
	*(src+3) = *(src+17);
	*(src+16) = tmp0;
	*(src+17) =tmp1;
	//MCB_SWAP(src, 6 , 12);
	tmp0 = *(src+6);
	tmp1 = *(src+7);
	*(src+6) = *(src+12);
	*(src+7) = *(src+13);
	*(src+12) = tmp0;
	*(src+13) =tmp1;
	//MCB_SWAP(src, 6 , 24);
	tmp0 = *(src+6);
	tmp1 = *(src+7);
	*(src+6) = *(src+24);
	*(src+7) = *(src+25);
	*(src+24) = tmp0;
	*(src+25) = tmp1;
	//MCB_SWAP(src, 6 , 18);
	tmp0 = *(src+6);
	tmp1 = *(src+7);
	*(src+6) = *(src+18);
	*(src+7) = *(src+19);
	*(src+18) = tmp0;
	*(src+19) = tmp1;
	//MCB_SWAP(src, 10, 20);
	tmp0 = *(src+10);
	tmp1 = *(src+11);
	*(src+10) = *(src+20);
	*(src+11) = *(src+21);
	*(src+20) = tmp0;
	*(src+21) = tmp1;
	//MCB_SWAP(src, 14, 28);
	tmp0 = *(src+14);
	tmp1 = *(src+15);
	*(src+14)= *(src+28);
	*(src+15)= *(src+29);
	*(src+28) = tmp0;
	*(src+29) = tmp1;
	//MCB_SWAP(src, 14, 26);
	tmp0 = *(src+14);
	tmp1 = *(src+15);
	*(src+14) = *(src+26);
	*(src+15) = *(src+27);
	*(src+26) = tmp0;
	*(src+27) = tmp1;
	//MCB_SWAP(src, 14, 22);		
	tmp0 = *(src+14);
	tmp1 = *(src+15);
	*(src+14) = *(src+22);
	*(src+15) = *(src+23);
	*(src+22) = tmp0;
	*(src+23) = tmp1;
	return;
}

static void JPG2MP4_2U2V(unsigned int *des, unsigned int *src)
{
	//MCB_SWAPUV(des,  0, src, 0);
  	*des = *src;
	*(des+1) = *(src+1);
	//MCB_SWAPUV(des,  2, src, 4);
	*(des+2) = *(src+4);
	*(des+3) = *(src+5);
	//MCB_SWAPUV(des,  4, src, 8);
	*(des+4) = *(src+8);
	*(des+5) = *(src+9);
	//MCB_SWAPUV(des,  6, src, 12);
	*(des+6) = *(src+12);
	*(des+7) = *(src+13);
	//MCB_SWAPUV(des,  8, src, 16);
	*(des+8) = *(src+16);
	*(des+9) = *(src+17);
	//MCB_SWAPUV(des,  10, src, 20);
	*(des+10) = *(src+20);
	*(des+11) = *(src+21);
	//MCB_SWAPUV(des,  12, src, 24);
	*(des+12) = *(src+24);
	*(des+13) = *(src+25);
	//MCB_SWAPUV(des,  14, src, 28);
	*(des+14) = *(src+28);
	*(des+15) = *(src+29);
	//MCB_SWAPUV(des,  16, src, 32);
	*(des+16) = *(src+32);
	*(des+17) = *(src+33);
	//MCB_SWAPUV(des,  18, src, 36);
	*(des+18) = *(src+36);
	*(des+19) = *(src+37);
	//MCB_SWAPUV(des,  20, src, 40);
	*(des+20) = *(src+40);
	*(des+21) = *(src+41);
	//MCB_SWAPUV(des,  22, src, 44);
	*(des+22) = *(src+44);
	*(des+23) = *(src+45);
	//MCB_SWAPUV(des,  24, src, 48);
	*(des+24) = *(src+48);
	*(des+25) = *(src+49);
	//MCB_SWAPUV(des,  26, src, 52);
	*(des+26) = *(src+52);
	*(des+27) = *(src+53);
	//MCB_SWAPUV(des,  28, src, 56);
	*(des+28) = *(src+56);
	*(des+29) = *(src+57);
	//MCB_SWAPUV(des,  30, src, 60);
	*(des+30) = *(src+60);
	*(des+31) = *(src+61);
	return;
}

static void JPG2MP4_1U1V(unsigned int *des, unsigned int *src)
{
	//MCB_SWAPUV(des, 0x0, src, 0x0);
	//des[0]    = src[0];
	//des[0x1]= src[0x1];
	*des = *src;
	*(des+1) = *(src+1);
	//MCB_SWAPUV(des, 0x2, src, 0x4);
	//des[0x2] = src[0x4];
	//des[0x3] = src[0x5];
	*(des+2) = *(src+4);
	*(des+3) = *(src+5);
	//MCB_SWAPUV(des, 0x4, src, 0x8);
	//des[0x4] = src[0x8];
	//des[0x5] = src[0x9];
	*(des+4) = *(src+8);
	*(des+5) = *(src+9);
	//MCB_SWAPUV(des, 0x6, src, 0xC);	
	//des[0x6] = src[0xC];
	//des[0x7] = src[0xD];
	*(des+6) = *(src+0xC);
	*(des+7) = *(src+0xD);
	//F_DEBUG(" %x %x %x %x %x %x %x %x\n",  des[0], des[1], des[2], des[3], des[4] ,des[5] ,des[6], des[7]);
	return;
}

void dma_line_422 (jpeg_decompress_struct_my * my,int mcu_x, int mcu_y, unsigned int *pLDMA, int jump)
{
	unsigned char *mp;

	if(mcu_x==0 || mcu_x==1 || jump) {
		mp=my->outdata[0]+
			(mcu_y*my->max_vs_fact*DCTSIZE*my->output_width*RGB_PIXEL_SIZE) +
			(mcu_x *my->max_hs_fact*DCTSIZE* RGB_PIXEL_SIZE);
//printk ("y: %d, addr: 0x%08x\n", mcu_y, (int)mp);
		if ( (my->sample == JDS_420) || ( my->sample == JDS_422_1) )
			pLDMA[0] = (unsigned int) mp | DMA_INCS_64;
		else
			pLDMA[0] = (unsigned int) mp | DMA_INCS_128;
	}
}
void dma_line_yuv (jpeg_decompress_struct_my * my,int mcu_x, int mcu_y,
	unsigned int *pLDMA, jpeg_component_info * cci_simple[4], int jump)
{
	int ci;
	unsigned int addr;
	jpeg_component_info *cmpp;
	if(mcu_x==0 || mcu_x==1 || jump) {
		//support OUTPUT_YUV & all sample
		//support OUTPUT_420_YUV & (JDS_422_0 ||JDS_422_1 || JDS_420)
		for (ci = 0, cmpp = cci_simple[0]; ci < my->comps_in_scan; ci++, cmpp ++) {
			if ((my->format == OUTPUT_420_YUV) && (my->sample != JDS_420) &&
																						(cmpp->component_index != 0))
				addr = (unsigned int )my->outdata[cmpp->component_index] +
					mcu_y*cmpp->out_width*cmpp->vs_fact*8/2+ mcu_x*cmpp->hs_fact*8;
			else
				addr = (unsigned int )my->outdata[cmpp->component_index] +
					mcu_y*cmpp->out_width*cmpp->vs_fact*8+ mcu_x*cmpp->hs_fact*8;

			pLDMA[4*ci] = addr | cmpp->hs_fact;
		}
	}
}

#define BIT0 (1L<<0)
#define BIT1 (1L<<1)
#define BIT_RGB (1L<<2)
#define BIT1_422 (1L<<3)
#define BIT2_422 (1L<<4)

// only GM8181 and after will report Restart_marker,
// for compatibility define the followings will stop checking
#define STOP_CHECK_RST_MARKER

int decompress_YUV (jpeg_decompress_struct_my * my, jpeg_component_info * cci_simple[4])
{
	int pipe= 0;
	int i = -1;
	MCU_s mm[4];
	MCU_s * m_vld = &mm[4-1], *m_dq = 0, *m_dt=0, *m_dma=0;
	int restart2go;
	int next_restart_num = 0;
	int dma_jump = 0;
	unsigned int cpsts_reg,vldsts_reg;
	unsigned int dzar_qar;
	volatile MDMA *pmdma = MDMA1(ADDRESS_BASE);
	if (my->restart_interval == 0)
		restart2go = 0x7FFFFFFF;		// max pos. value, never reach to 0
	else
		restart2go = my->restart_interval + 1;	// for 1st time

	m_vld->x = -1;
	m_vld->y = 0;
	while (1) {

		// DMA stage
		if(pipe & BIT_RGB) {
			pipe &= ~BIT_RGB;
			FA526_DrainWriteBuffer();
			// wait DMA done
			while(!(pmdma->Status & 0x1))
				;
		}
		// DT stage if CbyCry
		if (pipe & BIT2_422) {
			pipe &= ~BIT2_422;
			if ((m_dt->x >= my->crop_x) && (m_dt->x < my->crop_xend) &&
			  	(m_dt->y >= my->crop_y) && (m_dt->y < my->crop_yend)) {
			  	unsigned int * dma_cmd = (unsigned int *) (ADDRESS_BASE +
							((m_dt->toggle == 0)? JD_DMA_CMD_OFF0:
																	 JD_DMA_CMD_OFF1));
				dma_line_422(my, m_dt->x-my->crop_x, m_dt->y-my->crop_y,
										dma_cmd, m_dt->jump | dma_jump);
				pipe |= BIT_RGB;
			}
			// check DT done
			do { 
				READ_CPSTS(ADDRESS_BASE, cpsts_reg)
			} while(!(cpsts_reg&0x04000));
			if (pipe & BIT_RGB) {
		  		// Dont care SMaddr,LMaddr and BlkWidth DMA register, since
				pmdma->CCA = (m_dt->toggle==0)? (JD_DMA_CMD_OFF0 | 0x02):
																				(JD_DMA_CMD_OFF1 | 0x02);
				pmdma->Control =  (0x04A00000); //start DMA from local memory
			}
		}

		// DQ-MC stage
		if(pipe & BIT1_422) {
			unsigned int base = m_dq->toggle * STRIDE_MCU+ ADDRESS_BASE;

			pipe &= ~BIT1_422;
			pipe |= BIT2_422;
			FA526_DrainWriteBuffer();

			// Don't remove DQ_MC sync
			do {
				READ_CPSTS(ADDRESS_BASE, cpsts_reg)		
			} while(!(cpsts_reg&0x02));	
				
			if ( my->sample == JDS_420 ) {
				JPG2MP4_4Y((unsigned int *)(CUR_B0+base));
			}
			else if ( my->sample == JDS_422_0 ) {
				JPG2MP4_4Y((unsigned int *)(CUR_B0 + base));
				JPG2MP4_2U2V((unsigned int *)(CUR_B4 + base), (unsigned int *)(CUR_B4 + base));
			}
			else if ( my->sample == JDS_422_1) {
				JPG2MP4_1U1V((unsigned int *)(CUR_B4 + base), (unsigned int *)(CUR_B2 + base));
				JPG2MP4_1U1V((unsigned int *)(CUR_B5 + base), (unsigned int *)(CUR_B3 + base));
				JPG2MP4_2Y((unsigned int *)(CUR_B0 + base));
			}

			// set DT src and dst base address
			SET_MECADDR(ADDRESS_BASE, (m_dq->toggle==0)? CUR_B0: CUR1_B0)
			// DT destination base address, must 32-bit aligned
			SET_MCCADDR(ADDRESS_BASE, (m_dq->toggle == 0)? RGB_OFF0: RGB_OFF1)
			// start DT Engine , to set the MC Control Register's DT_EN and DT_GO field
			SET_MCCTL(ADDRESS_BASE, START_DT) 
		}

		// DQ-MC stage
		if(pipe & BIT1) {
			pipe &= ~BIT1;
			//FA526_DrainWriteBuffer();
			if ((m_dq->x >= my->crop_x) && (m_dq->x < my->crop_xend) &&
			  	(m_dq->y >= my->crop_y) && (m_dq->y < my->crop_yend)) {
			  	unsigned int * dma_cmd = (unsigned int *) (ADDRESS_BASE +
							((m_dq->toggle == 0)? JD_DMA_CMD_OFF0:
																	JD_DMA_CMD_OFF1));
				dma_line_yuv(my, m_dq->x-my->crop_x, m_dq->y-my->crop_y,
							dma_cmd, cci_simple, m_dq->jump | dma_jump);
				pipe |= BIT_RGB;
			}
			// if the DQ_MC stage is active, we need to check whether the DQ_MC is done or not
			do {
				READ_CPSTS(ADDRESS_BASE, cpsts_reg)
			} while(!(cpsts_reg&0x02));
			if (pipe & BIT_RGB) {
				// don't set the SMaddr,LMaddr and BlkWidth DMA register
				pmdma->CCA = (m_dq->toggle==0)? (JD_DMA_CMD_OFF0 | 0x02):
																				(JD_DMA_CMD_OFF1 | 0x02);
				pmdma->Control =  (0x04A00000); //start DMA from local memory
			}
		}

		// start VLD-DZ Engine and set the VLD-DZ Di-zigzag scan buffer
		if(pipe & BIT0)  {
			int rst_off = 0;
			pipe &= ~BIT0;
			if (my->format == OUTPUT_CbYCrY)
				pipe |= BIT1_422;
			else
				pipe |= BIT1;
			while (1) {
				int vld_err = 0;
				FA526_DrainWriteBuffer(); 	
				// check VLD is done or not
				do {	
					READ_VLDSTS(ADDRESS_BASE, vldsts_reg)
				} while(!(vldsts_reg&0x01));
				if (vldsts_reg  & 0xF000) {
					#ifdef LINUX
						printk ("VLD error 0x%x (%d %d)\n", (vldsts_reg  & 0xF000)>>12, m_vld->x, m_vld->y);
					#endif
					//printk ("my->restart_interval %d, restart2go = %d\n", my->restart_interval, restart2go);
					vld_err = 1;
					if (my->restart_interval == 0) {
						vld_err = 2;		// un-recovery error
					}
					else {
						//printk ("restart2go %d %d %d\n", restart2go, m_vld->x, m_vld->y);
						// force to search next restart-markder
						while (restart2go-- != 0) {
							if (++ m_vld->x >= my->MCUs_per_row) {
								m_vld->y ++;
								m_vld->x = 0;
							}
						}
						restart2go = 0;
					}
					m_vld->jump = 1;
				}
				if (restart2go != 0)
					break;
				if (vld_err) {
					// set the command for JPEG search re-sync marker before decoding
					//printk ("search restart %x\n", ((vldsts_reg >> 16) & 0xFF));
					SET_VLDCTL(ADDRESS_BASE, (1L << 6) | 9)
				}
				else {
					vldsts_reg = ((vldsts_reg >> 16) & 0xFF) -0xd0;	// take 0xFFDx off
					#ifdef STOP_CHECK_RST_MARKER
					vldsts_reg = next_restart_num;								// always hit
					#endif
					if(vldsts_reg == next_restart_num) {
						//printk ("hit %d,", next_restart_num);
						break;									// already get correct restart marker, out of this while loop
					}
					//printk ("restart not equal (%d %d)\n", vldsts_reg, next_restart_num);
					if (vldsts_reg < 8) {
						int mcu_cnt;
						if (vldsts_reg == ((next_restart_num+1) & 7))
							rst_off = 1;
						else if (vldsts_reg == ((next_restart_num+2) & 7))
							rst_off = 2;
						else if (vldsts_reg == ((next_restart_num-1) & 7))
							rst_off = -1;
						else if (vldsts_reg == ((next_restart_num-2) & 7))
							rst_off = -2;
						if (rst_off) {
							// caculate new mcu count
							mcu_cnt = rst_off * my->restart_interval+ m_vld->y*my->MCUs_per_row +m_vld->x;
							if ((mcu_cnt < (my->total_iMCU_rows*my->MCUs_per_row)) &&
								(mcu_cnt >= 0) ) {
								// modify to correct restart marker, out of this while loop
								m_vld->x = mcu_cnt%my->MCUs_per_row;
								m_vld->y = mcu_cnt/my->MCUs_per_row;
								m_vld->jump = 1;
								//printk ("rst jump to %d(%d %d)\n", rst_off, m_vld->x, m_vld->y);
								break;
							}
						}
					}
				}
				{
					unsigned int asadr, bitlen;
					READ_ABADR(ADDRESS_BASE, asadr);
					READ_BALR(ADDRESS_BASE, bitlen);
					asadr = asadr - 256 + (bitlen & 0x3f) - 0xc;
					//printk ("sync to 0x%08x\n", asadr);
					if (asadr >= my->bs_phy_end) {
						//printk ("bs err to end\n");
						vld_err = 2;
					}
				}
				if (vld_err == 2) {		// un-recovery error
					m_vld->y = my->total_iMCU_rows;	// end this frame;
					pipe &= ~(BIT1 | BIT1_422);				// no more next stage
					break;
				}
				// search for next restart marker
				SET_MCCTL(ADDRESS_BASE, START_VLD)
			}

			if (restart2go == 0) {
				next_restart_num += rst_off + 1;
				next_restart_num &= 7;
				restart2go = my->restart_interval;
				SET_PYDCR(ADDRESS_BASE, 0)
				SET_PUVDCR(ADDRESS_BASE, 0)
			}
			// assign QAR register to the input buffer of De-Quantization & MC Engine
			READ_QAR(ADDRESS_BASE, dzar_qar)
			dzar_qar = (((m_vld->toggle == 0)? BUFFER_0: BUFFER_1)&0x0000ffff) |
																												(dzar_qar & 0x0ffff0000);
			// set the Quantization Local Buffer Address
			SET_QAR(ADDRESS_BASE, dzar_qar)
			// start DQ-MC Engine
			SET_MCIADDR(ADDRESS_BASE, CUR_B0 + m_vld->toggle* STRIDE_MCU)         
			SET_MCCTL(ADDRESS_BASE, START_DQ_MC)
		}

		// pipe mcu pointer
		i ++;
		//printk("my->format = %d\n", my->format);
		if (my->format == OUTPUT_CbYCrY) {
			m_dma = m_dt;
			m_dt = m_dq;
		}
		else
			m_dma = m_dq;
		dma_jump =  m_dma? m_dma->jump: 0;
		m_dq = m_vld;
		m_vld = &mm[i%4];
		m_vld->x = m_dq->x+1;
		m_vld->y = m_dq->y;
		if (m_vld->x >= my->MCUs_per_row) {
			m_vld->y ++;
			m_vld->x = 0;
		}
		m_vld->jump = 0;
		m_vld->toggle = i&1;

		if (m_vld->y < my->total_iMCU_rows) {
			pipe |= BIT0;
			if (--restart2go == 0) {
				// set the command for JPEG search re-sync marker before decoding
				SET_VLDCTL(ADDRESS_BASE, (1L << 6) | 9)
			}
			else {
				// set the command for JPEG search rnormal decoding
				SET_VLDCTL(ADDRESS_BASE, (1L << 6) | 8)
			}
			// set output buffer for VLD-DZ Engine by setting DZAR register
			READ_QAR(ADDRESS_BASE, dzar_qar)
			dzar_qar = (dzar_qar & 0x0000ffff) |
							(((m_vld->toggle == 0)? BUFFER_0: BUFFER_1) << 16);
			SET_QAR(ADDRESS_BASE, dzar_qar) // set the de-zigzag Scan Buffer Address
			SET_MCCTL(ADDRESS_BASE, START_VLD)
		}
//my->monitor1 = (my->total_iMCU_rows << 16) | my->MCUs_per_row;
//printk ("i %d %d %d 0x%x\n", i, m_vld->x, m_vld->y, pipe);
		if (pipe == 0)
			break;
	}
//my->monitor = (m_vld->y << 24) |(m_vld->x << 8) | i;
	return 0;
}
#endif
#ifdef SUPPORT_VG_422T
#define BIT_IN		(1L<<0)
#define BIT_DT		(1L<<1)
#define BIT_OUT	(1L<<2)
static void dma_init_422T(int y_w)
{
	uint32_t * dma_cmd[2];
	dma_cmd[0] = (uint32_t *)(ADDRESS_BASE + DMACMD_422_0);
	dma_cmd[1] = (uint32_t *)(ADDRESS_BASE + DMACMD_422_1);
 	// init dma parameter
	dma_cmd[0][0 + 1] = mDmaLocMemAddr14b (IN_420_0);
	dma_cmd[0][4 + 1] = mDmaLocMemAddr14b (IN_420_0+0x100);
	dma_cmd[0][8 + 1] = mDmaLocMemAddr14b(IN_420_0+0x140);

	dma_cmd[1][0 + 1] = mDmaLocMemAddr14b (IN_420_1);
	dma_cmd[1][4 + 1] = mDmaLocMemAddr14b(IN_420_1+0x100);
	dma_cmd[1][8 + 1] = mDmaLocMemAddr14b(IN_420_1+0x140);

	dma_cmd[0][0 + 2] =
	dma_cmd[1][0 + 2] =
		mDmaSysOff14b(y_w/4+1 - DCTSIZE*2/4) | 			// SysOff
		mDmaSysWidth6b(2*DCTSIZE/8);
	dma_cmd[0][4 + 2] =
	dma_cmd[0][8 + 2] =
	dma_cmd[1][4 + 2] =
	dma_cmd[1][8 + 2] =
		mDmaSysOff14b(y_w/2/4+1 - DCTSIZE/4) | 		// SysOff
		mDmaSysWidth6b(DCTSIZE/8);
	dma_cmd[0][0 + 3] =
	dma_cmd[1][0 + 3] =
		(1 << 28) | 	//Loc3Doff: 1, last line of blk1 to first line of blk2
		(1 << 26) |	//mask interrupt
		(1 << 23) | 	//dma_en
		(1 << 21) | 	//chain enable
		(0 << 20) | 	//sys->local
		(0 << 18) | 	//local: seq
		(1 << 16) | 	//sys:2d
		(0x100/4);
	dma_cmd[0][4 + 3] =
	dma_cmd[1][4 + 3] =
		(1 << 26) |	//mask interrupt
		(1 << 23) | 	//dma_en
		(1 << 21) | 	//chain enable
		(0 << 20) | 	//sys->local
		(0 << 18) | 	//local: seq
		(1 << 16) | 	//sys:2d
		(0x40/4);
	dma_cmd[0][8 + 3] =
	dma_cmd[1][8 + 3] =
		(1 << 23) | 	//dma_en
		(0 << 20) | 	//sys->local
		(0 << 18) | 	//local: seq
		(1 << 16) | 	//sys:2d
		(0x40/4);

	// output 422 package
	dma_cmd[0][12 + 1] = mDmaLocMemAddr14b(OUT_422);
	dma_cmd[0][12 + 2] =
		mDmaSysOff14b(y_w*2/4+1 - DCTSIZE*4/4) | 		// SysOff
		mDmaSysWidth6b(DCTSIZE*4/8);
	dma_cmd[0][12 + 3] =
		(1 << 23) | 	//dma_en
		(1 << 20) | 	//local->sys
		(0 << 18) | 	//local: seq
		(1 << 16) | 	//sys:2d
		(0x200/4);
}
static void dma_in_422T(int tgl, int mbx, unsigned int in_y, unsigned int in_u, unsigned int in_v, int yh_offset)
{
	uint32_t * dma_cmd = (uint32_t *)(ADDRESS_BASE + ((tgl)? DMACMD_422_1: DMACMD_422_0));
	if ( (mbx == 0) || (mbx == 1) ) {
		dma_cmd[0] = (in_y + mbx*16 + yh_offset) | DMA_INCS_32;
		dma_cmd[4] = (in_u + mbx*8 + yh_offset/4) | DMA_INCS_16;
		dma_cmd[8] = (in_v + mbx*8 + yh_offset/4) | DMA_INCS_16;
	}
}

/*
							<-  mb_in	     	 -><- mb_dt	   		->
<-00st stage	    -><-  1st stage      -><- 2nd stage 	->
----------------------------cycle start -----------------------------------------------
							2)goDMA_In
														4)goDT(D)
														5)preDmaOut(D)
							3)waitDMA_In
~~change to next mb~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														6)waitDT(D-x)
														7)goDMA_Out
														8)waitDMA_Out
								(I-D)
1)preDmaIn(x-I)
----------------------------cycle end --------------------------------------------------
*/
int Yuv2Cbycry (jpeg_decompress_struct_my * my, unsigned int in_y, unsigned int in_u, unsigned int in_v, unsigned int out_yuv)
{
	int pipe= 0;
	int i = -1;
	MCU_s mm[2];
	MCU_s * m_in = &mm[2-1], *m_dt = 0;
	unsigned int cpsts_reg;
	volatile MDMA *pmdma = MDMA1(ADDRESS_BASE);

	dma_init_422T((my->crop_xend-my->crop_x) * 16);
	m_in->x = -1;
	m_in->y = 0;
	while (1) {
//		2)goDMA_In
		if(pipe & BIT_IN) {
			// Dont care SMaddr,LMaddr and BlkWidth DMA register
			pmdma->CCA = ((m_in->toggle==0)? DMACMD_422_0: DMACMD_422_1) | 0x02;
			pmdma->Control =	(0x04A00000); //start DMA from local memory
		}
//		5)goDT(D)
		if(pipe & BIT_DT) {
			// set DT src and dst base address
			SET_MECADDR(ADDRESS_BASE, (m_dt->toggle == 0)? IN_420_0: IN_420_1)
			// DT destination base address, must 32-bit aligned
			SET_MCCADDR(ADDRESS_BASE, OUT_422)
			// start DT Engine , to set the MC Control Register's DT_EN and DT_GO field
			SET_MCCTL(ADDRESS_BASE, START_DT) 
		}

//																6)preDmaOut(D)
		if (pipe & BIT_DT) {
			uint32_t * dma_cmd = (uint32_t *)(ADDRESS_BASE + DMACMD_422_OUT);
			if (m_dt->x == 0)
				dma_cmd[0] = (out_yuv + (m_dt->y - my->crop_y)*(my->crop_xend - my->crop_x)*512) | DMA_INCS_32;
		}

//									4)waitDMA_In
		if(pipe & (BIT_IN | BIT_OUT)) {
			FA526_DrainWriteBuffer();
			// wait DMA done
			while(!(pmdma->Status & 0x1))
				;
		}
//																							(O-x)
		if(pipe & BIT_OUT)
			pipe &= ~BIT_OUT;
//		~~change to next mb~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// pipe mcu pointer
		i ++;
		m_dt = m_in;
		m_in = &mm[i&1];
		m_in->x = m_dt->x+1;
		m_in->y = m_dt->y;
		if (m_in->x >= (my->crop_xend - my->crop_x)) {
			m_in->y ++;
			//printk("y %d\n", m_in->y);
			m_in->x = 0;
		}
		m_in->toggle = i&1;
//																6)waitDT(D-x)
//																7)goDMA_Out
//																8)waitDMA_Out
		if (pipe & BIT_DT) {
			pipe &= ~BIT_DT;
			// check DT done
			do { 
				READ_CPSTS(ADDRESS_BASE, cpsts_reg)
			} while(!(cpsts_reg&0x04000));
			// Dont care SMaddr,LMaddr and BlkWidth DMA register
			pmdma->CCA = DMACMD_422_OUT | 0x02;
			pmdma->Control =	(0x04A00000); //start DMA from local memory
			// wait DMA done
			while(!(pmdma->Status & 0x1))
					;
		}
//									(I-D)
		if(pipe & BIT_IN) {
			pipe &= ~BIT_IN;
			pipe |= BIT_DT;
		}
//		1)preDmaIn(x-I)
		if (m_in->y < (my->crop_yend - my->crop_y)) {
			pipe |= BIT_IN;
			dma_in_422T(m_in->toggle, m_in->x, in_y, in_u, in_v, (i - m_in->x)*256);
		}

		if (pipe == 0)
			break;
	}
	return 0;
}

#endif

#ifdef SUPPORT_VG_422P
#define BIT_P_IN        (1L<<0)
#define BIT_P_DT        (1L<<1)
#define BIT_P_OUT       (1L<<2)
static void dma_init_422P(int y_w)
{
	uint32_t * dma_cmd[2];
	dma_cmd[0] = (uint32_t *)(ADDRESS_BASE + DMACMD_422_0);
	dma_cmd[1] = (uint32_t *)(ADDRESS_BASE + DMACMD_422_1);
 	// init dma parameter
	dma_cmd[0][0 + 1] = mDmaLocMemAddr14b (IN_420_0);
	dma_cmd[0][4 + 1] = mDmaLocMemAddr14b (IN_420_0+0x100);
	dma_cmd[0][8 + 1] = mDmaLocMemAddr14b(IN_420_0+0x140);

	dma_cmd[1][0 + 1] = mDmaLocMemAddr14b (IN_420_1);
	dma_cmd[1][4 + 1] = mDmaLocMemAddr14b(IN_420_1+0x100);
	dma_cmd[1][8 + 1] = mDmaLocMemAddr14b(IN_420_1+0x140);

	dma_cmd[0][0 + 2] =
	dma_cmd[1][0 + 2] =
		mDmaSysOff14b(y_w/4+1 - DCTSIZE*2/4) | 			// SysOff
		mDmaSysWidth6b(2*DCTSIZE/8);
	dma_cmd[0][4 + 2] =
	dma_cmd[0][8 + 2] =
	dma_cmd[1][4 + 2] =
	dma_cmd[1][8 + 2] =
		mDmaSysOff14b(y_w/4+1 - DCTSIZE/4) | 		// SysOff
		mDmaSysWidth6b(DCTSIZE/8);
	dma_cmd[0][0 + 3] =
	dma_cmd[1][0 + 3] =
		(1 << 28) | 	//Loc3Doff: 1, last line of blk1 to first line of blk2
		(1 << 26) |	//mask interrupt
		(1 << 23) | 	//dma_en
		(1 << 21) | 	//chain enable
		(0 << 20) | 	//sys->local
		(0 << 18) | 	//local: seq
		(1 << 16) | 	//sys:2d
		(0x100/4);
	dma_cmd[0][4 + 3] =
	dma_cmd[1][4 + 3] =
		(1 << 26) |	//mask interrupt
		(1 << 23) | 	//dma_en
		(1 << 21) | 	//chain enable
		(0 << 20) | 	//sys->local
		(0 << 18) | 	//local: seq
		(1 << 16) | 	//sys:2d
		(0x40/4);
	dma_cmd[0][8 + 3] =
	dma_cmd[1][8 + 3] =
		(1 << 23) | 	//dma_en
		(0 << 20) | 	//sys->local
		(0 << 18) | 	//local: seq
		(1 << 16) | 	//sys:2d
		(0x40/4);

	// output 422 package
	dma_cmd[0][12 + 1] = mDmaLocMemAddr14b(OUT_422);
	dma_cmd[0][12 + 2] =
		mDmaSysOff14b(y_w*2/4+1 - DCTSIZE*4/4) | 		// SysOff
		mDmaSysWidth6b(DCTSIZE*4/8);
	dma_cmd[0][12 + 3] =
		(1 << 23) | 	//dma_en
		(1 << 20) | 	//local->sys
		(0 << 18) | 	//local: seq
		(1 << 16) | 	//sys:2d
		(0x200/4);
}
static void dma_in_422P(int tgl, int mbx, unsigned int in_y, unsigned int in_u, unsigned int in_v, int yh_offset)
{
	uint32_t * dma_cmd = (uint32_t *)(ADDRESS_BASE + ((tgl)? DMACMD_422_1: DMACMD_422_0));
	if ( (mbx == 0) || (mbx == 1) ) {
		dma_cmd[0] = (in_y + mbx*16 + yh_offset) | DMA_INCS_32;
		dma_cmd[4] = (in_u + mbx*8 + yh_offset/2) | DMA_INCS_16;
		dma_cmd[8] = (in_v + mbx*8 + yh_offset/2) | DMA_INCS_16;
	}
}

int YuvPackedCbycry (jpeg_decompress_struct_my * my, unsigned int in_y, unsigned int in_u, unsigned int in_v, unsigned int out_yuv)
{
    int pipe= 0;
    int i = -1;
    MCU_s mm[2];
    MCU_s * m_in = &mm[2-1], *m_dt = 0;
    unsigned int cpsts_reg;
    volatile MDMA *pmdma = MDMA1(ADDRESS_BASE);

    dma_init_422P((my->crop_xend-my->crop_x) * 16);
    m_in->x = -1;
    m_in->y = 0;

//    printm("mb rows: %x\n", my->total_iMCU_rows);

    //printk("int buf (%08x, %08x, %08x), out buffer %08x\n", in_y, in_u, in_v, out_yuv);
    while (1) {
//      2)goDMA_In
        if(pipe & BIT_P_IN) {
            // Dont care SMaddr,LMaddr and BlkWidth DMA register
            pmdma->CCA = ((m_in->toggle==0)? DMACMD_422_0: DMACMD_422_1) | 0x02;
            pmdma->Control =    (0x04A00000); //start DMA from local memory
        }
//      5)goDT(D)
        if(pipe & BIT_P_DT) {
            // set DT src and dst base address
            SET_MECADDR(ADDRESS_BASE, (m_dt->toggle == 0)? IN_420_0: IN_420_1)
            // DT destination base address, must 32-bit aligned
            SET_MCCADDR(ADDRESS_BASE, OUT_422)
            // start DT Engine , to set the MC Control Register's DT_EN and DT_GO field
            SET_MCCTL(ADDRESS_BASE, START_DT) 
        }

//                                                              6)preDmaOut(D)
        if (pipe & BIT_P_DT) {
            uint32_t * dma_cmd = (uint32_t *)(ADDRESS_BASE + DMACMD_422_OUT);
            if (m_dt->x == 0)
                dma_cmd[0] = (out_yuv + (m_dt->y - my->crop_y)*(my->crop_xend - my->crop_x)*512) | DMA_INCS_32;
        }

//                                  4)waitDMA_In
        if(pipe & (BIT_P_IN | BIT_P_OUT)) {
            FA526_DrainWriteBuffer();
            // wait DMA done
            while(!(pmdma->Status & 0x1))
                ;
        }
//                                                                                          (O-x)
        if(pipe & BIT_P_OUT)
            pipe &= ~BIT_P_OUT;
//      ~~change to next mb~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // pipe mcu pointer
        i ++;
        m_dt = m_in;
        m_in = &mm[i&1];
        m_in->x = m_dt->x+1;
        m_in->y = m_dt->y;
        if (m_in->x >= (my->crop_xend - my->crop_x)) {
            m_in->y ++;
            //printk("y %d\n", m_in->y);
            m_in->x = 0;
        }
        m_in->toggle = i&1;
//                                                              6)waitDT(D-x)
//                                                              7)goDMA_Out
//                                                              8)waitDMA_Out
        if (pipe & BIT_P_DT) {
            pipe &= ~BIT_P_DT;
            // check DT done
            do { 
                READ_CPSTS(ADDRESS_BASE, cpsts_reg)
            } while(!(cpsts_reg&0x04000));
            // Dont care SMaddr,LMaddr and BlkWidth DMA register
            pmdma->CCA = DMACMD_422_OUT | 0x02;
            pmdma->Control =    (0x04A00000); //start DMA from local memory
            // wait DMA done
            while(!(pmdma->Status & 0x1))
                    ;
        }
//                                  (I-D)
        if(pipe & BIT_P_IN) {
            pipe &= ~BIT_P_IN;
            pipe |= BIT_P_DT;
        }
//      1)preDmaIn(x-I)
        //if (m_in->y < (my->crop_yend - my->crop_y)) {
        if (m_in->y < (my->total_iMCU_rows+1)/2) {  // because mb is 16x8
            pipe |= BIT_P_IN;
            dma_in_422P(m_in->toggle, m_in->x, in_y, in_u, in_v, (i - m_in->x)*256);
        }

        if (pipe == 0)
            break;
    }

    return 0;

}
#endif

