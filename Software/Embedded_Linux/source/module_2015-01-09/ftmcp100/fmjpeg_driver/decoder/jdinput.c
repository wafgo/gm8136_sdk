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
 * jdinput.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains input control logic for the JPEG decompressor.
 * These routines are concerned with controlling the decompressor's input
 * processing (marker reading and coefficient decoding).  The actual input
 * reading is done in jdmarker.c, jdhuff.c, and jdphuff.c.
 */

#include "../common/jinclude.h"
#include "jdeclib.h"



/*
 * Routines to calculate various quantities related to the size of the image.
 */

int dj_initial_setup (j_decompress_ptr dinfo)
/* Called once, when first SOS marker is reached */
{
	int ci;
	jpeg_component_info *compptr;
	jpeg_decompress_struct_my *my= & dinfo->pub;

	/* Compute maximum sampling factors; check factor validity */
	dinfo->pub.max_hs_fact = 1;
	dinfo->pub.max_vs_fact = 1;
	for (ci = 0, compptr = dinfo->comp_info; ci < dinfo->num_components;
    																				ci++, compptr++) {
		dinfo->pub.max_hs_fact = 
			MAX(dinfo->pub.max_hs_fact, compptr->hs_fact);
		dinfo->pub.max_vs_fact =
			MAX(dinfo->pub.max_vs_fact,compptr->vs_fact);
	}

	// automatic fit to image size
	my->output_width = 	dinfo->output_width_set;
	dinfo->output_height = 	dinfo->output_height_set;
	if (0 == my->output_width)
		my->output_width = dinfo->image_width;
	else if (my->output_width != dinfo->image_width) {
		printk ("JD Error: frame_width %d not equal to image width %d\n",
							my->output_width, dinfo->image_width);
		return -1;
	}

	if (0 == dinfo->output_height)
		dinfo->output_height = dinfo->image_height;
	else if (dinfo->output_height != dinfo->image_height) {
		printk ("JD Error: frame_height %d not equal to image height %d\n",
							dinfo->output_height, dinfo->image_height);
		return -1;
	}

	for (ci = 0, compptr = dinfo->comp_info; ci < dinfo->num_components;
																					ci++, compptr++) {
		/* Size in DCT blocks */
		compptr->width_in_blocks = (JDIMENSION) jdiv_round_up(
					(long) dinfo->image_width * (long) compptr->hs_fact,
					(long) (dinfo->pub.max_hs_fact * DCTSIZE));
		compptr->height_in_blocks = (JDIMENSION) jdiv_round_up(
					(long) dinfo->image_height * (long) compptr->vs_fact,
		    		(long) (dinfo->pub.max_vs_fact * DCTSIZE));
		compptr->out_width = my->output_width*compptr->hs_fact/my->max_hs_fact;
	}

	// check cropping
	dinfo->crop_mcu_x = dinfo->crop_x_set / (my->max_hs_fact*8);
	if (dinfo->crop_mcu_x*(my->max_hs_fact*8) != dinfo->crop_x_set)
		printk ("force crop_x from %d to %d due to Sampling\n", dinfo->crop_x_set, dinfo->crop_mcu_x*(my->max_hs_fact*8));
	dinfo->crop_mcu_y = dinfo->crop_y_set / (my->max_vs_fact*8);
	if (dinfo->crop_mcu_y*(my->max_vs_fact*8) != dinfo->crop_y_set)
		printk ("force crop_y from %d to %d due to Sampling\n", dinfo->crop_y_set, dinfo->crop_mcu_y*(my->max_vs_fact*8));

	// crop_xend mean: width in MCU unit
	dinfo->crop_mcu_xend = dinfo->crop_w_set / (my->max_hs_fact*8);
	if (dinfo->crop_mcu_xend*(my->max_hs_fact*8) != dinfo->crop_w_set) {
		printk ("force crop_w from %d to %d due to Sampling\n",
								dinfo->crop_w_set, dinfo->crop_mcu_xend*(my->max_hs_fact*8));
	}
	if (dinfo->crop_mcu_xend*(my->max_hs_fact*8) > my->output_width) {
	    printk("Fail \"crop_w, output_width\" (%d > %d)\n",
				dinfo->crop_mcu_xend*(my->max_hs_fact*8), my->output_width);
		return -1;
    }
	// crop_xend mean: x END MCU unit
	if (dinfo->crop_mcu_xend == 0) {
		dinfo->crop_mcu_xend = (unsigned int)jdiv_round_up(
			(long)dinfo->image_width,
			(long)(my->max_hs_fact*DCTSIZE));
		if ((dinfo->crop_mcu_xend*(my->max_hs_fact*DCTSIZE) != dinfo->image_width) &&
			 (dinfo->crop_mcu_xend*(my->max_hs_fact*DCTSIZE) != dinfo->buf_width_set)) {
			printk ("JD error: Not support such image width %d (mcu width %d) due to Sampling and buf_width %d\n",
								dinfo->image_width, my->max_hs_fact*8, dinfo->buf_width_set);
			return -1;
		}
	}
	else {
		dinfo->crop_mcu_xend += dinfo->crop_mcu_x;
		if (dinfo->crop_mcu_xend*(my->max_hs_fact*DCTSIZE) > dinfo->image_width) {
		    printk("Fail \"crop_x+ crop_w, width\" (%d > %d)\n",
								dinfo->crop_mcu_xend*(my->max_hs_fact*DCTSIZE), dinfo->image_width);
			return -1;
    	}
	}

	// crop_yend mean: height in MCU unit
	dinfo->crop_mcu_yend = dinfo->crop_h_set / (my->max_vs_fact*DCTSIZE);
	if (dinfo->crop_mcu_yend*(my->max_vs_fact*DCTSIZE) != dinfo->crop_h_set) {
		printk ("force crop_h from %d to %d\n", dinfo->crop_h_set, dinfo->crop_mcu_yend*(my->max_vs_fact*8));
	}
	if (dinfo->crop_mcu_yend*(my->max_vs_fact*DCTSIZE) > dinfo->output_height) {
	    printk("Fail \"crop_h, output_height\" (%d > %d)\n",
				dinfo->crop_mcu_yend*(my->max_vs_fact*DCTSIZE), dinfo->output_height);
		return -1;
    }
	// crop_yend mean: y END MCU unit
	if (dinfo->crop_mcu_yend == 0) {
		dinfo->crop_mcu_yend = (unsigned int)jdiv_round_up(
			(long)dinfo->image_height,
			(long)(my->max_vs_fact*DCTSIZE));
		if ((dinfo->crop_mcu_yend*(my->max_vs_fact*DCTSIZE) != dinfo->image_height) &&
			 (dinfo->crop_mcu_yend*(my->max_vs_fact*DCTSIZE) != dinfo->buf_height_set)) {
			printk ("Not support such image height %d (mcu height %d) due to Sampling and buf_height %d\n",
								dinfo->image_height, my->max_vs_fact*DCTSIZE, dinfo->buf_height_set);
			return -1;
		}
	}
	else {
		dinfo->crop_mcu_yend += dinfo->crop_mcu_y;
		if (dinfo->crop_mcu_yend*(my->max_vs_fact*DCTSIZE) > dinfo->image_height) {
		    printk("Fail \"crop_y + crop_h, height\" (%d > %d)\n",
					dinfo->crop_mcu_yend*(my->max_vs_fact*DCTSIZE),  dinfo->image_height);
			return -1;
    	}
	}
#if 0
printk("disp setting: output_width = %d, output_height = %d\n", my->output_width, dinfo->output_height);
printk("disp setting: set output_width = %d, output_height = %d\n", dinfo->output_width_set, dinfo->output_height_set);
printk("disp setting: ptFrame->img_width = %d, ptFrame->img_height = %d\n", dinfo->image_width, dinfo->image_height);
printk("disp setting: crop set %d %d %d %d\n"
					, dinfo->crop_x_set, dinfo->crop_y_set, dinfo->crop_w_set, dinfo->crop_h_set);
printk("disp setting: crop mcu %d %d %d %d\n"
					, dinfo->crop_mcu_x, dinfo->crop_mcu_y, dinfo->crop_mcu_xend, dinfo->crop_mcu_yend);
#endif
	return 0;
}


static void
dj_per_scan_setup (j_decompress_ptr dinfo)
/* Do computations that are needed before processing a JPEG scan */
/* dinfo->comps_in_scan and dinfo->cur_comp_info[] were set from SOS marker */
{
	int ci, mcublks;
	jpeg_component_info *compptr;

	if (dinfo->pub.comps_in_scan == 1) {
		/* Noninterleaved (single-component) scan */
		compptr = dinfo->cur_comp_info[0];
		/* Overall image size in MCUs */
		dinfo->pub.MCUs_per_row = compptr->width_in_blocks;
		/* Compute number of fully interleaved MCU rows. */
		dinfo->pub.total_iMCU_rows = compptr->height_in_blocks;
		dinfo->pub.crop_x = dinfo->crop_mcu_x*compptr->hs_fact;
		dinfo->pub.crop_y = dinfo->crop_mcu_y*compptr->vs_fact;
		dinfo->pub.crop_xend = dinfo->crop_mcu_xend*compptr->hs_fact;
		dinfo->pub.crop_yend = dinfo->crop_mcu_yend*compptr->vs_fact;

		compptr->hs_fact = 1;
		compptr->vs_fact = 1;
		dinfo->blocks_in_MCU = 1;
		dinfo->MCU_membership[0] = 0;

		// change max_h/v_samp_factor
		dinfo->pub.max_hs_fact = 1;
		dinfo->pub.max_vs_fact = 1;

	}
	else {
		/* Interleaved (multi-component) scan */
		/* Overall image size in MCUs */
		dinfo->pub.MCUs_per_row = (JDIMENSION)
			jdiv_round_up((long) dinfo->image_width,  (long) (dinfo->pub.max_hs_fact*DCTSIZE));
		/* Compute number of fully interleaved MCU rows. */
		dinfo->pub.total_iMCU_rows = (JDIMENSION)
			jdiv_round_up((long) dinfo->image_height, (long) (dinfo->pub.max_vs_fact*DCTSIZE));
		dinfo->pub.crop_x = dinfo->crop_mcu_x;
		dinfo->pub.crop_y = dinfo->crop_mcu_y;
		dinfo->pub.crop_xend = dinfo->crop_mcu_xend;
		dinfo->pub.crop_yend = dinfo->crop_mcu_yend;
		dinfo->blocks_in_MCU = 0;
		for (ci = 0; ci < dinfo->pub.comps_in_scan; ci++) {
			compptr = dinfo->cur_comp_info[ci];
			/* Prepare array describing MCU composition */
			mcublks = compptr->hs_fact * compptr->vs_fact;
			while (mcublks-- > 0) {
				dinfo->MCU_membership[dinfo->blocks_in_MCU++] = ci;
			}
		}
	}
}

/*
 * Initialize the input modules to read a scan of compressed data.
 * The first call to this is done by jdmaster.c after initializing
 * the entire decompressor (during jpeg_start_decompress).
 * Subsequent calls come from consume_markers, below.
 */
#if TUBA_LIMIT_TABLE_SIZE   // tuba 20110721_0: add limit to huffman tbale
extern int start_pass_huff_decoder (j_decompress_ptr dinfo);
#else
extern void start_pass_huff_decoder (j_decompress_ptr dinfo);
#endif

#if TUBA_LIMIT_TABLE_SIZE   // tuba 20110721_0: add limit to huffman tbale
int
#else
void
#endif
dj_setup_per_scan (j_decompress_ptr dinfo)
{
  	/* Prepare to scan data & restart markers */
	SET_PYDCR(ADDRESS_BASE, 0)  // set JPEG Previous Y DC Value Register to zero
	SET_PUVDCR(ADDRESS_BASE, 0) // set JPEG Previous UV DC Value Register to zero

  	dj_per_scan_setup(dinfo);
#if TUBA_LIMIT_TABLE_SIZE   // tuba 20110721_0: add limit to huffman tbale
  	if (start_pass_huff_decoder(dinfo) < 0)
        return -1;
    return 0;
#else
    start_pass_huff_decoder(dinfo);
#endif
}


