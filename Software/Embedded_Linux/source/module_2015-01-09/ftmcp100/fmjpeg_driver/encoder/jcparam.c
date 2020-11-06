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
 * jcparam.c
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains optional default-setting code for the JPEG compressor.
 * Applications do not have to use this file, but those that don't use it
 * must know a lot more about the innards of the JPEG code.
 */

#define JPEG_INTERNALS
#include "../common/jinclude.h"
#include "jenclib.h"

JHUFF_TBL * jpeg_alloc_huff_table_enc (j_compress_ptr cinfo);

#define FIX1(X) (1L << 16) / (X) + 1
/*
 * Quantization table setup routines
 */

void
jpeg_add_quant_table (j_compress_ptr cinfo, int which_tbl,
		      const unsigned int *basic_table,
		      int scale_factor)
/* Define a quantization table equal to the basic_table times
 * a scale factor (given as a percentage).
 * If force_baseline is TRUE, the computed quantization table entries
 * are limited to 1..255 for JPEG baseline compatibility.
 */
{
  int i,j;
  long temp;
  UINT16 *out_tab;
  unsigned int *out_tab_L;
  int invtemp;
  unsigned int qout;

  //for (i = 0; i < DCTSIZE2; i++) {
  out_tab = rinfo.Inter_quant[which_tbl];
  out_tab_L = rinfo.QUTTBL[which_tbl];
  for(i=0;i<8;i++){
	  for(j=0;j<8;j++){
		temp = ((long) *basic_table++ * scale_factor + 50L) / 100L;
		/* limit the values to the valid range */
		if (temp <= 0L) temp = 1L;
		if (temp > 255L) temp = 255L;		/* limit to baseline range if requested */
		*out_tab++ = (UINT16) temp;
		invtemp = FIX1((UINT16) temp);
		qout = (unsigned int) ( ((invtemp & 0x1ffff)<<8) | temp );
		*out_tab_L = qout;		//pwhsu++:20040922
		out_tab_L += 8;
	  }
	  out_tab_L -= 63;
  }
  /* Initialize sent_table FALSE so table will be written to JPEG file. */
 cinfo->qtbl_sent_table[which_tbl] = FALSE;
  //D_DEBUG("jpeg_add_quant_table FINISH ====\n");	
}

/* These are the sample quantization tables given in JPEG spec section K.1.
   * The spec says that the values given produce "good" quality, and
   * when divided by 2, "very good" quality.
   */
const unsigned int luma_qtbl_1[DCTSIZE2] = {
        16,  11,  10,  16,  24,  40,  51,  61,
        12,  12,  14,  19,  26,  58,  60,  55,
        14,  13,  16,  24,  40,  57,  69,  56,
        14,  17,  22,  29,  51,  87,  80,  62,
        18,  22,  37,  56,  68, 109, 103,  77,
        24,  35,  55,  64,  81, 104, 113,  92,
        49,  64,  78,  87, 103, 121, 120, 101,
        72,  92,  95,  98, 112, 100, 103,  99
};
const unsigned int chroma_qtbl_1[DCTSIZE2] = {
        17,  18,  24,  47,  99,  99,  99,  99,
        18,  21,  26,  66,  99,  99,  99,  99,
        24,  26,  56,  99,  99,  99,  99,  99,
        47,  66,  99,  99,  99,  99,  99,  99,
        99,  99,  99,  99,  99,  99,  99,  99,
        99,  99,  99,  99,  99,  99,  99,  99,
        99,  99,  99,  99,  99,  99,  99,  99,
        99,  99,  99,  99,  99,  99,  99,  99
};
const unsigned int luma_qtbl_2[DCTSIZE2] = {
        8,  5,  20, 25, 30, 27, 14, 25,
        5,  12, 30, 29, 7,  11, 43, 27,
        8,  6,  13, 6,  8,  40, 17, 32,
        6,  9,  8,  7,  31, 12, 40, 60,
        7,  12, 28, 9,  38, 52, 51, 60,
        20, 34, 11, 51, 56, 43, 50, 56,
        28, 18, 54, 46, 39, 36, 49, 50,
        28, 34, 24, 32, 46, 47, 51, 49
};
const unsigned int chroma_qtbl_2[DCTSIZE2] = {
        8,  9,  49, 49, 49, 49, 49, 49,
        12, 49, 49, 49, 12, 49, 49, 49,
        23, 9,  49, 13, 33, 49, 49, 49,
        10, 33, 28, 23, 49, 49, 49, 49,
        13, 49, 49, 49, 49, 49, 49, 49,
        49, 49, 49, 49, 49, 49, 49, 49,
        49, 49, 49, 49, 49, 49, 49, 49,
        49, 49, 49, 49, 49, 49, 49, 49
};


static void
jpeg_set_linear_quality (j_compress_ptr cinfo, int scale_factor)
/* Set or change the 'quality' (quantization) setting, using default tables
 * and a straight percentage-scaling quality scale.  In most cases it's better
 * to use jpeg_set_quality (below); this entry point is provided for
 * applications that insist on a linear percentage scaling.
 */
{
    static const unsigned int *std_luminance_quant_tbl;
    static const unsigned int *std_chrominance_quant_tbl;
    //D_DEBUG("jpeg_set_linear_quality START ====\n");	
    switch (cinfo->qtbl_no){
        case 1:
	    std_luminance_quant_tbl = luma_qtbl_1;
	    std_chrominance_quant_tbl = chroma_qtbl_1; 
        break;
        case 2:
	    std_luminance_quant_tbl = luma_qtbl_2;
	    std_chrominance_quant_tbl = chroma_qtbl_2; 
        break;
        case 3: //user defined table
        //printk("Quant Table 3\n");
        std_luminance_quant_tbl = cinfo->luma_qtbl;
	    std_chrominance_quant_tbl = cinfo->chroma_qtbl; 
        break;
        default:
        std_luminance_quant_tbl = luma_qtbl_2;
	    std_chrominance_quant_tbl = chroma_qtbl_2;
        break;
    }
    /* Set up two quantization tables using the specified scaling */
    jpeg_add_quant_table(cinfo, 0, std_luminance_quant_tbl, scale_factor);
    jpeg_add_quant_table(cinfo, 1, std_chrominance_quant_tbl, scale_factor);
    //D_DEBUG("jpeg_set_linear_quality FINISH ====\n");		
	
}


static int
jpeg_quality_scaling (int quality)
/* Convert a user-specified quality rating to a percentage scaling factor
 * for an underlying quantization table, using our recommended scaling curve.
 * The input 'quality' factor should be 0 (terrible) to 100 (very good).
 */
{
  /* Safety limit on quality factor.  Convert 0 to 1 to avoid zero divide. */
  if (quality <= 0) quality = 1;
  if (quality > 100) quality = 100;

  /* The basic table is used as-is (scaling 100) for a quality of 50.
   * Qualities 50..100 are converted to scaling percentage 200 - 2*Q;
   * note that at Q=100 the scaling is 0, which will cause jpeg_add_quant_table
   * to make all the table entries 1 (hence, minimum quantization loss).
   * Qualities 1..50 are converted to scaling percentage 5000/Q.
   */
  if (quality < 50)
    quality = 5000 / quality;
  else
    quality = 200 - quality*2;

  return quality;
}


void
jpeg_set_quality (j_compress_ptr cinfo)
/* Set or change the 'quality' (quantization) setting, using default tables.
 * This is the standard quality-adjusting entry point for typical user
 * interfaces; only those who want detailed control over quantization tables
 * would use the preceding three routines directly.
 */
{
	int quality;
	/* Convert user 0-100 rating to percentage scaling */
	quality = jpeg_quality_scaling(cinfo->quality);
	/* Set up standard quality tables */
	jpeg_set_linear_quality(cinfo, quality);
}


/*
 * Huffman table setup routines
 */

static JHUFF_TBL *
add_huff_table (j_compress_ptr cinfo,JHUFF_TBL *htblptr, const UINT8 *bits, const UINT8 *val)
/* Define a Huffman table */
{
  int nsymbols, len;

  //D_DEBUG("add_huff_table START ======\n");
  if (htblptr == NULL) { 
  	htblptr = (JHUFF_TBL *)jpeg_alloc_huff_table_enc(cinfo );
  }
  
  /* Copy the number-of-symbols-of-each-code-length counts */
  MEMCOPY((htblptr)->bits, bits, SIZEOF((htblptr)->bits));
  /* Validate the counts.  We do this here mainly so we can copy the right
   * number of symbols from the val[] array, without risking marching off
   * the end of memory.  jchuff.c will do a more thorough test later.
   */
  nsymbols = 0;
  for (len = 1; len <= 16; len++)
	nsymbols += bits[len];
  MEMCOPY((htblptr)->huffval, val, nsymbols * SIZEOF(UINT8));
  /* Initialize sent_table FALSE so table will be written to JPEG file. */
  (htblptr)->sent_table = FALSE;
  return htblptr;
}


static void
std_huff_tables (j_compress_ptr cinfo)
/* Set up the standard Huffman tables (cf. JPEG standard section K.3) */
/* IMPORTANT: these are only valid for 8-bit data precision! */
{

  static const UINT8* bits_dc_luminance;
  static const UINT8* val_dc_luminance;
  static const UINT8* bits_dc_chrominance;
  static const UINT8* val_dc_chrominance;
  static const UINT8* bits_ac_luminance;
  static const UINT8* val_ac_luminance;
  static const UINT8* bits_ac_chrominance;
  static const UINT8* val_ac_chrominance;
  static const UINT8 bits_dc_lum1[17] =
    { /* 0-base */ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
  static const UINT8 val_dc_lum1[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  static const UINT8 bits_dc_chrom1[17] =
    { /* 0-base */ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  static const UINT8 val_dc_chrom1[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  static const UINT8 bits_ac_lum1[17] =
    { /* 0-base */ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
  static const UINT8 val_ac_lum1[] =
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
      0xf9, 0xfa };
  static const UINT8 bits_ac_chrom1[17] =
    { /* 0-base */ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
  static const UINT8 val_ac_chrom1[] =
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
      0xf9, 0xfa };
  const UINT8 bits_dc_lum2[17] =
		{ /* 0-base */ 0, 0, 1, 2, 3, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
  const UINT8 bits_dc_chrom2[17] =
		{ /* 0-base */ 0, 0, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  const UINT8 bits_ac_lum2[17] =
		{ /* 0-base */ 0, 0, 1, 2, 2, 4, 2, 4, 3, 5, 5, 4, 4, 1, 0, 5, 0x78 };
  const UINT8 bits_ac_chrom2[17] =
		{ /* 0-base */ 0, 0, 0, 3, 3, 3, 4, 3, 4, 6, 6, 4, 4, 0, 2, 1, 0x77 };
  const UINT8 val_dc_lum2[] =
		{ 0, 1, 2, 4, 3, 5, 6, 7, 8, 9, 10, 11 };
  const UINT8 val_dc_chrom2[] =
		{ 0, 1, 2, 3, 4, 5, 6, 8, 7, 9, 10, 11 };
  const UINT8 val_ac_lum2[] =
		{ 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
		  0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
		  0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
		  0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
		  0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
		  0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
		  0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	      0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
		  0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
		  0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
		  0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
		  0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
		  0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
		  0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
		  0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
		  0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
		  0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
		  0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
		  0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
		  0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
		  0xf9, 0xfa };

   const UINT8 val_ac_chrom2[] =
		{ 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
		  0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
		  0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
		  0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
		  0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
		  0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
		  0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
		  0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
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
		  0xf9, 0xfa };

  if(cinfo->hufftbl_no == 1){
	bits_dc_luminance	=	bits_dc_lum1;
	val_dc_luminance	=	val_dc_lum1;
	bits_dc_chrominance =	bits_dc_chrom1;
	val_dc_chrominance	=	val_dc_chrom1;
	bits_ac_luminance	=	bits_ac_lum1;
	val_ac_luminance	=	val_ac_lum1;
	bits_ac_chrominance	=	bits_ac_chrom1;
	val_ac_chrominance	=	val_ac_chrom1;
  }else  {
    bits_dc_luminance	=	bits_dc_lum2;
	val_dc_luminance	=	val_dc_lum2;
	bits_dc_chrominance =	bits_dc_chrom2;
	val_dc_chrominance	=	val_dc_chrom2;
	bits_ac_luminance	=	bits_ac_lum2;
	val_ac_luminance	=	val_ac_lum2;
	bits_ac_chrominance	=	bits_ac_chrom2;
	val_ac_chrominance	=	val_ac_chrom2;
  }
	

  cinfo->dc_huff_tbl_ptrs[0] = add_huff_table(cinfo, cinfo->dc_huff_tbl_ptrs[0],
		 bits_dc_luminance, val_dc_luminance);
  
  	
  cinfo->ac_huff_tbl_ptrs[0]  = add_huff_table(cinfo, cinfo->ac_huff_tbl_ptrs[0],
		 bits_ac_luminance, val_ac_luminance);
  
     
  cinfo->dc_huff_tbl_ptrs[1]  = add_huff_table(cinfo, cinfo->dc_huff_tbl_ptrs[1],
		 bits_dc_chrominance, val_dc_chrominance);
  
   
  cinfo->ac_huff_tbl_ptrs[1]  = add_huff_table(cinfo, cinfo->ac_huff_tbl_ptrs[1],
		 bits_ac_chrominance, val_ac_chrominance);
}


static unsigned char comp_list[][3][6] =
{
	// id, h, v, q_tbl_no, dc_tbl_no, ac_tbl_no
	// JCS_yuv420
	{{1, 2, 2, 0, 0, 0},
	  {2, 1, 1, 1, 1, 1},
	  {3, 1, 1, 1, 1, 1}},
	// JCS_yuv422
	{{1, 4, 1, 0, 0, 0},
	  {2, 2, 1, 1, 1, 1},
	  {3, 2, 1, 1, 1, 1}},
	// JCS_yuv211
	{{1, 2, 1, 0, 0, 0},
	  {2, 1, 1, 1, 1, 1},
	  {3, 1, 1, 1, 1, 1}},
	// JCS_yuv333
	{{1, 3, 1, 0, 0, 0},
	  {2, 3, 1, 1, 1, 1},
	  {3, 3, 1, 1, 1, 1}},
	// JCS_yuv222
	{{1, 2, 1, 0, 0, 0},
	  {2, 2, 1, 1, 1, 1},
	  {3, 2, 1, 1, 1, 1}},
	// JCS_yuv111
	{{1, 1, 1, 0, 0, 0},
	  {2, 1, 1, 1, 1, 1},
	  {3, 1, 1, 1, 1, 1}},
	// JCS_yuv400
	{{1, 1, 1, 0, 0, 0},
	  {0, 0, 0, 0, 0, 0},
	  {0, 0, 0, 0, 0, 0}}
};
//pwhsu:20031013 set samp_factor and quantixation table 
void jpeg_set_sampling (j_compress_ptr cinfo)
{
	jpeg_component_info * compptr;
	int ci;
	int comp_cnt= 0;
	for (ci = 0; ci < 3; ci ++) {
		unsigned char * comp_info = &comp_list[cinfo->pub.sample][ci][0];
		compptr = &cinfo->comp_info[ci];
		compptr->component_id = comp_info[0];
		compptr->hs_fact = comp_info[1];
		compptr->vs_fact = comp_info[2];
		compptr->quant_tbl_no = comp_info[3];
		compptr->dc_tbl_no = comp_info[4];
		compptr->ac_tbl_no = comp_info[5];
		if (compptr->hs_fact != 0)
			 comp_cnt ++;
	}
	 cinfo->num_components  = comp_cnt;
#if 0
  cinfo->write_JFIF_header = FALSE; /* No marker for non-JFIF colorspaces */
  cinfo->write_JFIF_header = TRUE; /* Write a JFIF marker */
#endif
  /* JFIF specifies component IDs 1,2,3 */
  /* We default to 2x2 subsamples of chrominance */
  cinfo->write_JFIF_header = TRUE; /* Write a JFIF marker */
}

/*
 * Default parameter setup for compression.
 *
 * Applications that don't choose to use this routine must do their
 * own setup of all these parameters.  Alternately, you can call this
 * to establish defaults and then alter parameters selectively.  This
 * is the recommended approach since, if we add any new parameters,
 * your code will still work (they'll be set to reasonable defaults).
 */

void jpeg_set_defaults (j_compress_ptr cinfo)
{
	std_huff_tables(cinfo);
	jpeg_set_sampling(cinfo);
}


