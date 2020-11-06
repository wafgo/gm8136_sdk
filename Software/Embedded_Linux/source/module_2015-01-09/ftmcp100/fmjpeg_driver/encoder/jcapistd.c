/*
 * jcapistd.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains application interface code for the compression half
 * of the JPEG library.  These are the "standard" API routines that are
 * used in the normal full-compression case.  They are not used by a
 * transcoding-only application.  Note that if an application links in
 * jpeg_start_compress, it will end up linking in the entire compressor.
 * We thus must separate this file from jcapimin.c to avoid linking the
 * whole compression library into a transcoder.
 */

#include <linux/string.h>
#define JPEG_INTERNALS
#include "jenclib.h"
#include "../common/jinclude.h"


static void jpeg_suppress_tables (j_compress_ptr cinfo, boolean quant_tbl_suppress,boolean huffman_tbl_suppress)
{
	int i;
	JHUFF_TBL * htbl;
	for (i = 0; i < NUM_QUANT_TBLS; i++) {
		cinfo->qtbl_sent_table[i] = quant_tbl_suppress;
		//printk ("jpeg_suppress_tables: %d sent_table = %d\n", i, qtbl->sent_table);
	}
	for (i = 0; i < NUM_HUFF_TBLS; i++) {
		if ((htbl = cinfo->dc_huff_tbl_ptrs[i]) != NULL)
			htbl->sent_table = huffman_tbl_suppress;      
		if ((htbl = cinfo->ac_huff_tbl_ptrs[i]) != NULL)
			htbl->sent_table = huffman_tbl_suppress;
	}
}



static void jc_dma_init(j_compress_ptr cinfo)
{
	int ci;
	int comp_w_off;
	unsigned int local = CUR_B0;
	unsigned int * dma_cmd = (unsigned int *) (JE_DMA_CMD_OFF0 + cinfo->VirtBaseAddr);
	// only support interleaving
	for (ci = 0; ci < cinfo->pub.comps_in_scan; ci++) {
		unsigned int dma_1, dma_2, dma_3;
		jpeg_component_info *compptr = cinfo->cur_comp_info[ci];
		int blk_sz = compptr->hs_fact*compptr->vs_fact;
		switch (cinfo->pub.u82D) {
			case 0:		// mp4-2d, for 420 & 400
				dma_1 = (8 << 28) |			// Loc3Dwidth, 8 lines in one block
								(((1 - (compptr->hs_fact - 1)*DCTSIZE2/4) & 0xFF)<<20) |	// Loc2Doff
								(compptr->hs_fact << 16) |										// Loc2Dwidth
								mDMALmaddr (local);
				dma_2 = ((DCTSIZE2/4 + 1 - DCTSIZE/4)<<24) |	// LocOff
								((DCTSIZE/4)<<20)	|								// LocWidth
								mDMASOffset(1) |									// SysOff
								mDMASWidth(compptr->hs_fact*DCTSIZE/8);
				break;
			case 2: 	// 264-2d, for 420 & 400
				if (ci == 0)
					blk_sz = 4;
				else {	// ci == 1
					blk_sz = 2; // 2	blks for u/v
					ci ++;			// skip next v
				}
				dma_1 =  (8 << 28) |			// Loc3Dwidth, 8 lines in one block
								(((1 - (2 - 1)*DCTSIZE2/4) & 0xFF)<<20) | // Loc2Doff
								(2 << 16) | 									// Loc2Dwidth, always 2 blks
								mDMALmaddr (local);
				dma_2 = ((DCTSIZE2/4 + 1 - DCTSIZE/4)<<24) |	// LocOff
								((DCTSIZE/4)<<20) | 							// LocWidth
								mDMASOffset(1) |									// SysOff
								mDMASWidth(2*DCTSIZE/8);					// system width always 16
				break;
			default:		// seq-1d if no DMAWRP
								// seq-1d 420/422 if DMAWRP
				comp_w_off = (cinfo->pub.MCUs_per_row -1) * compptr->hs_fact*DCTSIZE;
				dma_1 = (8 << 28) |			// Loc3Dwidth, 8 lines in one block
								(((1 - (compptr->hs_fact - 1)*DCTSIZE2/4) & 0xFF)<<20) |	// Loc2Doff
								(compptr->hs_fact << 16) |										// Loc2Dwidth
								mDMALmaddr (local);
				dma_2 = ((DCTSIZE2/4 + 1 - DCTSIZE/4)<<24) |	// LocOff
								((DCTSIZE/4)<<20)	| 									// LocWidth
								mDMASOffset(comp_w_off/4+1) |				// SysOff
								mDMASWidth(compptr->hs_fact*DCTSIZE/8);
				break;

		}
		blk_sz *= DCTSIZE2;
		dma_cmd[1] = dma_1;
		dma_cmd[41] = dma_1 + mDMALmaddr(STRIDE_MCU);
		dma_cmd[2] =
		dma_cmd[42] = dma_2;
		dma_3 = 
					(1 << 28) |			// Loc3Doff: 1, last line of blk1 to first line of blk2
					(0x8<<20) |		// sys->local, dma_en
					(0xd<<16) |		// sys:2d, local: 4d
					(blk_sz/4);
		if (ci != (cinfo->pub.comps_in_scan - 1))// not last one comp.
			dma_3 |=  ((0x1<<26) |	// mask interrupt
							(1 << 21));		// chain enable
		dma_cmd[3] =
		dma_cmd[43] = dma_3;
		//printk ("%d: blk 0x%x,\n", ci, blk_sz);
		//printk ("%d: 1 0x%08x,\n", ci, dma_cmd[1]);
		//printk ("%d: 2 0x%08x,\n", ci, dma_cmd[2]);
		//printk ("%d: 3 0x%08x,\n", ci, dma_cmd[3]);
		dma_cmd += 4;
		local += blk_sz;
	}
}

/* Set up the scan parameters for the current scan */
static void select_scan_parameters (j_compress_ptr cinfo)
{
	int ci;
	// only support interleaving
	/* Prepare for single sequential-JPEG scan containing all components */
	cinfo->pub.comps_in_scan = cinfo->num_components;
	for (ci = 0; ci < cinfo->num_components; ci++)
		cinfo->cur_comp_info[ci] = &cinfo->comp_info[ci];
}

/* Do computations that are needed before processing a JPEG scan */
/* cinfo->comps_in_scan and cinfo->cur_comp_info[] are already set */
static void per_scan_setup (j_compress_ptr cinfo)
{
	int ci, mcublks;
	jpeg_component_info *compptr;
	unsigned int mcublkn = 0;
	unsigned int Comp_member = 0;
	unsigned int tbidx;
	unsigned short tbidx0=0;
	unsigned short tbidx1=0;
	unsigned short tbidx2=0;
	unsigned int imginfo;

	cinfo->pub.MCUs_per_row = (JDIMENSION) jdiv_round_up(
			(long) cinfo->pub.image_width, (long) (cinfo->max_hs_fact*DCTSIZE));
	if (cinfo->pub.comps_in_scan == 1) {
		/* Noninterleaved (single-component) scan */
		compptr = cinfo->cur_comp_info[0];
		/* Prepare array describing MCU composition */
		mcublkn = 1;
		tbidx0 = compptr->ac_tbl_no;
		tbidx0 = (tbidx0<<1) | compptr->dc_tbl_no;
		tbidx0 = (tbidx0<<2) | compptr->quant_tbl_no;
		tbidx0 = tbidx0 & 0xf;
		imginfo = (tbidx0<<20);
	}
	else {
		/* Interleaved (multi-component) scan */
		for (ci = 0; ci < cinfo->pub.comps_in_scan; ci++) {
			compptr = cinfo->cur_comp_info[ci];
			/* Prepare array describing MCU composition */
			mcublks = compptr->hs_fact * compptr->vs_fact;

			if ( ci==0 ){				//pwhsu++:20040108
				tbidx0 = compptr->ac_tbl_no;
				tbidx0 = (tbidx0<<1) | compptr->dc_tbl_no;
				tbidx0 = (tbidx0<<2) | compptr->quant_tbl_no;
				tbidx0 = tbidx0 & 0xf;
			}
			else if ( ci==1 ){
				tbidx1 = compptr->ac_tbl_no;
				tbidx1 = (tbidx1<<1) | compptr->dc_tbl_no;
				tbidx1 = (tbidx1<<2) | compptr->quant_tbl_no;
				tbidx1 = tbidx1 & 0xf;
			}
			else if ( ci==2 ){
				tbidx2 = compptr->ac_tbl_no;
				tbidx2 = (tbidx2<<1) | compptr->dc_tbl_no;
				tbidx2 = (tbidx2<<2) | compptr->quant_tbl_no;
				tbidx2 = tbidx2 & 0xf;
			}

			while (mcublks-- > 0) {
				Comp_member |= ci << 2*mcublkn;
				mcublkn ++;
			}
		}
		tbidx = (tbidx2<<8) | (tbidx1<<4) | tbidx0;	//pwhsu++:20040108
		imginfo = (tbidx<<20) | (Comp_member&0xfffff);
	}
	SET_MCUBR(cinfo->VirtBaseAddr, mcublkn)
	SET_MCUTIR(cinfo->VirtBaseAddr, imginfo)
}


void jpeg_start_compress (j_compress_ptr cinfo, boolean write_quant_tables,boolean write_huffman_tables)
{
	cinfo->DRI_emmit = 0;
	jpeg_suppress_tables(cinfo, !write_quant_tables,!write_huffman_tables);	

	/* Write the datastream header (SOI) immediately.
	* Frame and scan headers are postponed till later.
	* This lets application insert special markers after the SOI.
	*/
	write_file_header1(cinfo);

	// each scan
	select_scan_parameters(cinfo);
	per_scan_setup(cinfo);
	start_pass_huff1(cinfo);

	write_frame_header1(cinfo);
	write_scan_header1(cinfo);
	jc_dma_init (cinfo);
	memcpy ((unsigned int *) QTBL0(cinfo->VirtBaseAddr), &rinfo.QUTTBL[0][0], DCTSIZE2*4);
	memcpy ((unsigned int *) QTBL1(cinfo->VirtBaseAddr), &rinfo.QUTTBL[1][0], DCTSIZE2*4);
	memcpy ((unsigned int *) QTBL2(cinfo->VirtBaseAddr), &rinfo.QUTTBL[2][0], DCTSIZE2*4);
	memcpy ((unsigned int *) QTBL3(cinfo->VirtBaseAddr), &rinfo.QUTTBL[3][0], DCTSIZE2*4);
}
