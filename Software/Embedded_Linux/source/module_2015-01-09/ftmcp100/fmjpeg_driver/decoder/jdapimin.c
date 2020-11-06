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
 * jdapimin.c
 *
 * Copyright (C) 1994-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains application interface code for the decompression half
 * of the JPEG library.  These are the "minimum" API routines that may be
 * needed in either the normal full-decompression case or the
 * transcoding-only case.
 *
 * Most of the routines intended to be called directly by an application
 * are in this file or in jdapistd.c.  But also see jcomapi.c for routines
 * shared by compression and decompression, and jdtrans.c for the transcoding
 * case.
 */

#include "../common/jinclude.h"
#include "jdeclib.h"

//void jpeg_abort_dec (j_common_ptr dinfo);
//void jpeg_destroy_dec (j_common_ptr dinfo);

#ifdef MJPEG_MODE
static const UINT8 bits_dc_luma[17] =
{ /* 0-base */ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
static const UINT8 val_dc_luma[] =
{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

static const UINT8 bits_dc_chroma[17] =
{ /* 0-base */ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
static const UINT8 val_dc_chroma[] =
{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

static const UINT8 bits_ac_luma[17] =
{ /* 0-base */ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
static const UINT8 val_ac_luma[] =
{ 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
  0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
  0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
  0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
  0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
  0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
  0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
  0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
  0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
  0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
  0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
  0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
  0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
  0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
  0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
  0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
  0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
  0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
  0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
  0xf9, 0xfa 
};

static const UINT8 bits_ac_chroma[17] =
{ /* 0-base */ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };

static const UINT8 val_ac_chroma[] =
{ 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
  0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
  0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
  0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
  0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
  0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
  0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
  0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
  0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
  0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
  0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
  0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
  0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
  0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
  0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
  0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
  0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
  0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
  0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
  0xf9, 0xfa 
};
#endif

//extern int show_warnning_msg;   //  Tuba 20110701_3: add proc to control warning message
int show_warnning_msg = 0;


/*
 * Destruction of a JPEG decompression object
 */

void
jpeg_destroy_decompress (j_decompress_ptr dinfo)
{
	int tblno=0;
	if ( dinfo->adc_idx ) {
		for(tblno = 0; tblno < (NUM_ADC_TBLS) ; tblno++) {
			if ( dinfo->adctbl[tblno] ) {
				dinfo->pfnFree(dinfo->adctbl[tblno] );
				dinfo->adctbl[tblno]=0;
				dinfo->adc_idx--;
			}
		}
		if (  dinfo->adc_idx ) {
			F_DEBUG("Destory adc_idx %d err\n", dinfo->adc_idx);
		}
	}

	for(tblno = 0; tblno < NUM_HUFF_TBLS; tblno++) {
		if ( dinfo->dc_huff_tbl_ptrs[tblno] ) {
			dinfo->pfnFree(dinfo->dc_huff_tbl_ptrs[tblno] );
			dinfo->dc_huff_tbl_ptrs[tblno]=0;
		}	
		if ( dinfo->ac_huff_tbl_ptrs[tblno] ) {
			dinfo->pfnFree(dinfo->ac_huff_tbl_ptrs[tblno] );
			dinfo->ac_huff_tbl_ptrs[tblno]=0;
		}
	}

	if ( dinfo->huftq_idx ) {
		for(tblno = 0; tblno < (NUM_HUFQ_TBLS); tblno++) {
			if ( dinfo->huftq[tblno] ) {
				dinfo->pfnFree(dinfo->huftq[tblno] );
				dinfo->huftq[tblno]=0;
				dinfo->huftq_idx--;
			}	
  		}
		if ( dinfo->huftq_idx ){
  			F_DEBUG("Destory huftq_idx %d err\n", dinfo->huftq_idx);
  		}
	}

}


/*
 * Abort processing of a JPEG decompression operation,
 * but don't destroy the object itself.
 */

#ifdef MJPEG_MODE  
static void
set_huff_tbl(j_decompress_ptr dinfo, JHUFF_TBL **htblptr, const UINT8 *bits_table, const UINT8 *val_table)
{
//printk ("set_huff_tbl START ====\n");	 
  	if (*htblptr == NULL)  {
		*htblptr = (JHUFF_TBL *) dinfo->pfnMalloc (sizeof(JHUFF_TBL),
  					dinfo->u32CacheAlign,
  					dinfo->u32CacheAlign);
  		if( !*htblptr ) {
  			F_DEBUG("can't allocate  JHUFF_TBL\n");
  		}
    	MEMCOPY((*htblptr)->bits, bits_table, SIZEOF((*htblptr)->bits));
    	MEMCOPY((*htblptr)->huffval, val_table, SIZEOF((*htblptr)->huffval));
  	}
}
#endif


/*
 * Set default decompression parameters.
 */

void
default_decompress_parms (j_decompress_ptr dinfo)
{
	//D_DEBUG("default_decompress_parms START ====\n");	 
	/* Guess the input colorspace, and set output colorspace accordingly. */
	/* (Wish JPEG committee had provided a real way to specify this...) */
	/* Note application may override our guesses. */

	switch (dinfo->num_components) {
		case 1:
    		dinfo->jpeg_color_space = JCS_GRAYSCALE;
    		break;
  		case 3:
   			if (dinfo->saw_JFIF_marker)
   				dinfo->jpeg_color_space = JCS_YCbCr; /* JFIF implies YCbCr */
			else if (dinfo->saw_Adobe_marker) {
      			switch (dinfo->Adobe_transform) {
      				case 0:
						dinfo->jpeg_color_space = JCS_RGB;
						break;
      				case 1:
						dinfo->jpeg_color_space = JCS_YCbCr;
						break;
      				default:
						dinfo->jpeg_color_space = JCS_YCbCr; /* assume it's YCbCr */
						break;
      			}
    		}
			else {
      			/* Saw no special markers, try to guess from the component IDs */
      			int cid0 = dinfo->comp_info[0].component_id;
      			int cid1 = dinfo->comp_info[1].component_id;
      			int cid2 = dinfo->comp_info[2].component_id;
   				if (cid0 == 1 && cid1 == 2 && cid2 == 3)
					dinfo->jpeg_color_space = JCS_YCbCr; /* assume JFIF w/out marker */
      			else if (cid0 == 'R' && cid1 == 'G' && cid2 == 'B')
					dinfo->jpeg_color_space = JCS_RGB; /* ASCII 'R', 'G', 'B' */
      			else
					dinfo->jpeg_color_space = JCS_YCbCr; /* assume it's YCbCr */
    		}
   			/* Always guess RGB is proper output colorspace. */
   			break;
  		case 4:
   			if (dinfo->saw_Adobe_marker) {
   				switch (dinfo->Adobe_transform) {
   					case 0:
						dinfo->jpeg_color_space = JCS_CMYK;
						break;
    				case 2:
						dinfo->jpeg_color_space = JCS_YCCK;
						break;
    				default:
						dinfo->jpeg_color_space = JCS_YCCK; /* assume it's YCCK */
						break;
    			}
    		}
			else	/* No special markers, assume straight CMYK. */
   				dinfo->jpeg_color_space = JCS_CMYK;
   			break;
  		default:
   			dinfo->jpeg_color_space = JCS_UNKNOWN;
   			break;
	}
//printk ("color space = %d\n", dinfo->jpeg_color_space);
	#ifdef MJPEG_MODE
//	#if 0
	set_huff_tbl(dinfo, &dinfo->dc_huff_tbl_ptrs[0], bits_dc_luma, val_dc_luma);
	set_huff_tbl(dinfo, &dinfo->dc_huff_tbl_ptrs[1], bits_dc_chroma, val_dc_chroma);
	set_huff_tbl(dinfo, &dinfo->ac_huff_tbl_ptrs[0], bits_ac_luma, val_ac_luma);
	set_huff_tbl(dinfo, &dinfo->ac_huff_tbl_ptrs[1], bits_ac_chroma, val_ac_chroma);
  	#endif
}


void jd_bs_align(void)
{
	unsigned int reg;
	READ_VADR(ADDRESS_BASE, reg)
	reg &= 7;
	if (reg > 0)
		SET_BALR(ADDRESS_BASE, 8-reg)
}
extern int read_markers (j_decompress_ptr dinfo);

int jpeg_read_header (j_decompress_ptr dinfo)
{
	// before finding marker, make sure byte aligment		
	jd_bs_align ();
	return read_markers (dinfo);
}

#if 1
int jpeg_finish_decompress (j_decompress_ptr dinfo, int check_EOI)
{
	int ret = 0;

	// check EOI
	if (check_EOI) {
		unsigned int reg, reg1;
		// before finding EOI marker, make sure byte aligment		
		jd_bs_align ();
		READ_BADR(ADDRESS_BASE, reg);
		// next two bytes is 0xFFD9
		if((reg&0xFFFF0000) != 0xFFD90000) {
            if (show_warnning_msg) {    //  Tuba 20110701_3: add proc to control warning message
    			unsigned int asadr, bitlen;
    			READ_VADR(ADDRESS_BASE, reg1);
    			printk("port 0x%08x, bit: 0x%08x\n", reg, reg1&7);
    			READ_ABADR(ADDRESS_BASE, asadr);
    			READ_BALR(ADDRESS_BASE, bitlen);
    			printk ("add_f: %08x\n", asadr - 256 + (bitlen & 0x3f) - 0xc);
    			printk("not FOUND EOI\n");
            }
			//ret = -1;
		}
		else
			SET_BALR(ADDRESS_BASE, 16);
	}
	#ifdef JPG_VLD_ERR_WORKARROUND
	{
  		unsigned int vldsts;
		SET_VOP0(ADDRESS_BASE, 0x1)
		SET_VLDCTL(ADDRESS_BASE, 0x40) // mp4 dec go
		SET_MCCTL(ADDRESS_BASE, 0x20)	// dec_go

		// wait for  VLD done & AB clean
		do {
   			READ_VLDSTS(ADDRESS_BASE, vldsts)
   		} while((vldsts&((1<<0) | (1<<10))) != ((1<<0) | (1<<10)));
	}
	#endif
#if defined(SEQ)
	mSEQ_EXTMSK(ADDRESS_BASE) = 0;		// no interrupt
#endif
	SET_VLDCTL(ADDRESS_BASE, 1<<5)  //Autobuffer Stop
	return ret;
}
#else
boolean
jpeg_finish_decompress (j_decompress_ptr dinfo, int must_end)
{
	int count;
  	unsigned int vldsts;
	unsigned int cnt;
	unsigned badr;

 	//20070130 tckuo add -->
 	cnt=0;
 	do {
    	READ_VLDSTS(ADDRESS_BASE, vldsts)
		cnt++;
		if ( cnt > JPEG_SYNC_TIMEOUT) {
			F_DEBUG("VLD Sync Timeout\n");
			break;
		}
  	} while(!(vldsts&1024));
	// 20070130 tckuo add --<

	{
		unsigned int vadr, balr, last_bit;

		// before finding EOI marker, make sure byte aligment		
		unsigned int reg;
		READ_VADR(ADDRESS_BASE, reg)
		reg &= 7;
		if (reg > 0)
			SET_BALR(ADDRESS_BASE, 8-reg)

		READ_BADR(ADDRESS_BASE, badr)  
		// next two bytes is 0xFFD9
//printk("badr 0x%08x\n", badr);
		if((badr&0xFFFF0000) == 0xFFD90000)     
			SET_BALR(ADDRESS_BASE, 8)

		// workaround with MP4 err MB data ->>
		//READ_BADR(ADDRESS_BASE, badr)
		// next word is 0xFFFF65BE
		// shift 14 becaome 0xFFD9...  
		if ( (badr & 0xFFFFF000) == 0xFFFF6000)
			SET_BALR(ADDRESS_BASE, 14)
		// next word is 0xFF65BE
		//READ_BADR(ADDRESS_BASE, badr)
		// shift 6 becaome 0xFFD9...  
		if ( (badr & 0xFFFF0000)== 0xFF650000)
			SET_BALR(ADDRESS_BASE, 6)
		// workaround with MP4 err MB data -<<    
	}

	if (must_end == 0)
		return TRUE;
	/* Read until EOI */
	count = 0;
	while (1) {
		int vadr;
		READ_BADR(ADDRESS_BASE, badr) 
		SET_BALR(ADDRESS_BASE, 8)
		if ( (badr & 0xFF000000) == 0xD9000000)
			break;
		if (1) {
			READ_VADR(ADDRESS_BASE, vadr);
			printk("port 0x%08x 0x%08x\n", badr, vadr);
		}
		if (++count >= 5) {
			if (1){
				unsigned int asadr, bitlen;
				READ_ABADR(ADDRESS_BASE, asadr);
				READ_BALR(ADDRESS_BASE, bitlen);
				printk ("add_f: %08x\n", asadr - 256 + (bitlen & 0x3f) - 0xc);
			}
			printk("not FOUND EOI\n");
			return FALSE;
		}
	}
	READ_BADR(ADDRESS_BASE, badr)
	if ( badr != 0x6f800000) {
		F_DEBUG("current BADR 0x%08x\n", badr);
       	return FALSE;
	}
	{
		unsigned int vop0_tmp;
		unsigned int vldctl_tmp;
		unsigned int mcctl_tmp;
	    // This is a softwareworkaround method.
	    // It is use to fix the QTbl 1 issue.
	    // When the element of qtbl is always 1 and the next decoding format
	    // is mpeg4, the AC error will happen in mp4 decoding.
	    // This bug can be fixed by padding 0x6370 into jpg bitstream and re-enable
	    // vld go before autobuffer stop. The two operations will make the decoding engine
	    // recorvery.
	    // The workaround method can be used in 8120, 8150 , 8180 and 8185.
	    // There is another method to fixe this bug in 8180. We can fix it by software reset in 8180.
	    // The software reset is only reset DMA controler in 8120, so it is not work for this bug on 8120.
	    READ_BADR(ADDRESS_BASE, badr)
		//F_DEBUG("workaround BADR 0x%08x\n", badr);
		
		//SET_VLDCTL(ADDRESS_BASE, 0x20)  //Autobuffer Stop
		//SET_VLDCTL(ADDRESS_BASE, 0x58)  //Autobuffer start
        		
		READ_BADR(ADDRESS_BASE, badr)
		if ( badr != 0x6f800000 ) {
//			F_DEBUG("MP4 err MB workaround err size %d\n", ptFrame->buf_size);
			F_DEBUG("MP4 err MB workaround err size\n");
			F_DEBUG("current BADR 0x%08x\n", badr);
			return FALSE;
	    }

        READ_VOP0(ADDRESS_BASE, vop0_tmp)
		READ_VLDCTL(ADDRESS_BASE, vldctl_tmp) // mp4 dec go
		READ_MCCTL(ADDRESS_BASE, mcctl_tmp)
		
		SET_VOP0(ADDRESS_BASE, 0x1)
		SET_VLDCTL(ADDRESS_BASE, 0x40) // mp4 dec go
		SET_MCCTL(ADDRESS_BASE, 0x20)

		cnt=0;
		do {
   			READ_CPSTS(ADDRESS_BASE, vldsts)
			cnt++;
			if ( cnt > JPEG_SYNC_TIMEOUT)
				break;	
   		} while(!(vldsts&(1<<15)));    // VLD done
		if ( cnt > JPEG_SYNC_TIMEOUT) {
			F_DEBUG("VLD Done Sync Timeout\n");
		}
		
        cnt=0;
   		do {
   			READ_VLDSTS(ADDRESS_BASE, vldsts)
			cnt++;
			if ( cnt > JPEG_SYNC_TIMEOUT)
			break;	
			//F_DEBUG("vldsts 0x%08x\n", vldsts);	
   		} while(!(vldsts&(1<<10))); // ABClean       
		if ( cnt > JPEG_SYNC_TIMEOUT) {
			F_DEBUG("ABClean Sync Timeout\n");
		}
		SET_VOP0(ADDRESS_BASE, vop0_tmp)
		SET_VLDCTL(ADDRESS_BASE, vldctl_tmp) // mp4 dec go
		SET_MCCTL(ADDRESS_BASE, mcctl_tmp)
  	}
	return TRUE;
}
#endif
