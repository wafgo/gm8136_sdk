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
 * jcmarker.c
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to write JPEG datastream markers.
 */

#define JPEG_INTERNALS
#include "jenclib.h"
#include "../common/jinclude.h"

typedef enum {			/* JPEG marker codes */
  M_SOF0  = 0xc0,
  M_SOF1  = 0xc1,
  M_SOF2  = 0xc2,
  M_SOF3  = 0xc3,
  
  M_SOF5  = 0xc5,
  M_SOF6  = 0xc6,
  M_SOF7  = 0xc7,
  
  M_JPG   = 0xc8,
  M_SOF9  = 0xc9,
  M_SOF10 = 0xca,
  M_SOF11 = 0xcb,
  
  M_SOF13 = 0xcd,
  M_SOF14 = 0xce,
  M_SOF15 = 0xcf,
  
  M_DHT   = 0xc4,
  
  M_DAC   = 0xcc,
  
  M_RST0  = 0xd0,
  M_RST1  = 0xd1,
  M_RST2  = 0xd2,
  M_RST3  = 0xd3,
  M_RST4  = 0xd4,
  M_RST5  = 0xd5,
  M_RST6  = 0xd6,
  M_RST7  = 0xd7,
  
  M_SOI   = 0xd8,
  M_EOI   = 0xd9,
  M_SOS   = 0xda,
  M_DQT   = 0xdb,
  M_DNL   = 0xdc,
  M_DRI   = 0xdd,
  M_DHP   = 0xde,
  M_EXP   = 0xdf,
  
  M_APP0  = 0xe0,
  M_APP1  = 0xe1,
  M_APP2  = 0xe2,
  M_APP3  = 0xe3,
  M_APP4  = 0xe4,
  M_APP5  = 0xe5,
  M_APP6  = 0xe6,
  M_APP7  = 0xe7,
  M_APP8  = 0xe8,
  M_APP9  = 0xe9,
  M_APP10 = 0xea,
  M_APP11 = 0xeb,
  M_APP12 = 0xec,
  M_APP13 = 0xed,
  M_APP14 = 0xee,
  M_APP15 = 0xef,
  
  M_JPG0  = 0xf0,
  M_JPG13 = 0xfd,
  M_COM   = 0xfe,
  
  M_TEM   = 0x01,
  
  M_ERROR = 0x100
} JPEG_MARKER;

#ifdef TWO_P_INT
#define BASE_ADDRESS 0
#endif

/* Emit a marker code */
void __inline emit_marker (unsigned int base, JPEG_MARKER mark)
{
	BitstreamPutBits((0xFF00 | (int) mark),16, base);
}


static void
flush_bits (unsigned int base)
{
	unsigned char curbits;
	unsigned char curbits2;
	unsigned char	tmp;
	unsigned int buf1;
	unsigned char buf2;

	READ_VADR(base, curbits)
	READ_VLASTWORD(base, buf1)
	tmp = (((curbits-1) & 0xff) % 8);
	curbits2 = 7-tmp;
 
	/* fill any partial byte with ones */
	if (curbits2 != 0){			
		BitstreamPutBits((0x7F)>>tmp, curbits2, base);
		buf2 = ( buf1 >> (  ( 32-((curbits)&0x1F))&0x18 ) ) & 0xFF;
		switch(tmp){
			case 0:
				buf2 = (buf2 & 0x80) | 0x7f; 
				break;
			case 1:
				buf2 = (buf2 & 0xc0) | 0x3f; 
				break;
			case 2:
				buf2 = (buf2 & 0xe0) | 0x1f; 
				break;
			case 3:
				buf2 = (buf2 & 0xf0) | 0x0f; 
				break;
			case 4:
				buf2 = (buf2 & 0xf8) | 0x07; 
				break;
			case 5:
				buf2 = (buf2 & 0xfc) | 0x03; 
				break;
			case 6:
				buf2 = (buf2 & 0xfe) | 0x01; 
				break;
			default:
				buf2 = 0x0; 
				break;
		}
		if(buf2 == 0xff){
			FA526_DrainWriteBuffer();
			BitstreamPutBits(0, 8, base);
		}
	}
}
/*
 * Write datastream trailer.
 */
void
write_file_trailer1 (unsigned int base)
{
	flush_bits(base);
	emit_marker(base, M_EOI);
}

/*
 * Emit a restart marker & resynchronize predictions.
 */
void emit_restart (unsigned int base, int restart_num)
{
	flush_bits(base);
	emit_marker(base, JPEG_RST0 + restart_num);
}

#if (defined(TWO_P_EXT) || defined(ONE_P_EXT))
/*
 * Routines to write specific marker types.
 */
extern const int jpeg_natural_order[]; /* zigzag coef order to natural order */

static int
emit_dqt (j_compress_ptr cinfo, int index)
/* Emit a DQT marker */
/* Returns the precision used (0 = 8bits, 1 = 16bits) for baseline checking */
{
	int prec;
	int i;

	prec = 0;
//printk ("emit_dqt: %d sent_table = %d\n", index, qtbl->sent_table);
	if (! cinfo->qtbl_sent_table[index]) {
    // tuba 20110701_2 start: add wait dma free
        {
            unsigned int reg_temp;
            do {
                READ_VLDSTS(BASE_ADDRESS, reg_temp)
                //printk("wait quant table %d\n", reg_temp);
            } while(!(reg_temp&1024));
        }
    // tuba 20110701_2 end

		emit_marker(cinfo->VirtBaseAddr, M_DQT);
		BitstreamPutBits(prec ? DCTSIZE2*2 + 1 + 2 : DCTSIZE2 + 1 + 2,16, cinfo->VirtBaseAddr);			//Lq
		BitstreamPutBits(index + (prec<<4),8, cinfo->VirtBaseAddr);
		for (i = 0; i < 8; i++) {
			int j = i*8;
			int qval0 = (rinfo.Inter_quant[index][jpeg_natural_order[j]]) & 0xFF;
			int qval1 = (rinfo.Inter_quant[index][jpeg_natural_order[j+1]]) & 0xFF;
			BitstreamPutBits( ((qval0<<8)|qval1), 16 , cinfo->VirtBaseAddr);

			qval0 = (rinfo.Inter_quant[index][jpeg_natural_order[j+2]]) & 0xFF;
			qval1 = (rinfo.Inter_quant[index][jpeg_natural_order[j+3]]) & 0xFF;
			BitstreamPutBits( ((qval0<<8)|qval1), 16 , cinfo->VirtBaseAddr);

			qval0 = (rinfo.Inter_quant[index][jpeg_natural_order[j+4]]) & 0xFF;
			qval1 = (rinfo.Inter_quant[index][jpeg_natural_order[j+5]]) & 0xFF;
			BitstreamPutBits( ((qval0<<8)|qval1), 16, cinfo->VirtBaseAddr);

			qval0 = (rinfo.Inter_quant[index][jpeg_natural_order[j+6]]) & 0xFF;
			qval1 = (rinfo.Inter_quant[index][jpeg_natural_order[j+7]]) & 0xFF;
			BitstreamPutBits( ((qval0<<8)|qval1), 16, cinfo->VirtBaseAddr );
		}
		cinfo->qtbl_sent_table[index] = TRUE;
	}
	return prec;
}


static void
emit_dht (j_compress_ptr cinfo, int index, boolean is_ac)
/* Emit a DHT marker */
{
  JHUFF_TBL * htbl;
  int length, i;
  
  if (is_ac) {
    htbl = cinfo->ac_huff_tbl_ptrs[index];
    index += 0x10;		/* output index has AC bit set */
  } else {
    htbl = cinfo->dc_huff_tbl_ptrs[index];
  }

  if (! htbl->sent_table) {
// tuba 20110701_2 start: add wait dma free
    {
        unsigned int reg_temp;
        do {
            READ_VLDSTS(BASE_ADDRESS, reg_temp)
            //printk("wait huf table %d\n", reg_temp);
        } while(!(reg_temp&1024));
    }
// tuba 20110701_2 end

    emit_marker(cinfo->VirtBaseAddr, M_DHT);			//DHT
    
    length = 0;
    for (i = 1; i <= 16; i++)
      length += htbl->bits[i];          
    BitstreamPutBits(length + 2 + 1 + 16,16, cinfo->VirtBaseAddr);	//Lh  (huffman table definition length)
    BitstreamPutBits(index,8, cinfo->VirtBaseAddr);
    for (i = 1; i <= 16; i++)
	  BitstreamPutBits(htbl->bits[i],8, cinfo->VirtBaseAddr);
    for (i = 0; i < length; i++)
	  BitstreamPutBits(htbl->huffval[i],8, cinfo->VirtBaseAddr);
    htbl->sent_table = TRUE;
  }
}



static void
emit_dri (j_compress_ptr cinfo)
/* Emit a DRI marker */
{
	emit_marker(cinfo->VirtBaseAddr, M_DRI);
	BitstreamPutBits(4, 16, cinfo->VirtBaseAddr);	/* fixed length */
	BitstreamPutBits((int) cinfo->pub.restart_interval, 16, cinfo->VirtBaseAddr);
}


static void
emit_sof (j_compress_ptr cinfo, JPEG_MARKER code)
/* Emit a SOF marker */
{
	int ci;
	jpeg_component_info *compptr;
	emit_marker(cinfo->VirtBaseAddr, code);		//SOFn
	BitstreamPutBits(3 * cinfo->num_components + 2 + 5 + 1, 16, cinfo->VirtBaseAddr);  /* length */ //Lf
	/* Make sure image isn't bigger than SOF field can handle */
	BitstreamPutBits(BITS_IN_JSAMPLE, 8, cinfo->VirtBaseAddr);
	// Tuba 20130311 satrt: add crop size to encode varibale size
	/*
	BitstreamPutBits((int) cinfo->roi_h, 16, cinfo->VirtBaseAddr);
	BitstreamPutBits((int) cinfo->roi_w, 16, cinfo->VirtBaseAddr);
	*/
	BitstreamPutBits((int) (cinfo->roi_h - cinfo->crop_height), 16, cinfo->VirtBaseAddr);
	BitstreamPutBits((int) (cinfo->roi_w - cinfo->crop_width), 16, cinfo->VirtBaseAddr);
    // Tuba 20130311 end

	BitstreamPutBits(cinfo->num_components, 8, cinfo->VirtBaseAddr);
	for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components; ci++, compptr++) {
		BitstreamPutBits(compptr->component_id, 8, cinfo->VirtBaseAddr);
		BitstreamPutBits((compptr->hs_fact << 4) + compptr->vs_fact, 8, cinfo->VirtBaseAddr);
		BitstreamPutBits(compptr->quant_tbl_no, 8, cinfo->VirtBaseAddr);
	}
}


static void
emit_sos (j_compress_ptr cinfo)
/* Emit a SOS marker */
{
	int i, td, ta;
	jpeg_component_info *compptr;
	emit_marker(cinfo->VirtBaseAddr, M_SOS);	//SOS
	BitstreamPutBits(2 * cinfo->pub.comps_in_scan + 2 + 1 + 3, 16, cinfo->VirtBaseAddr);  //Ls
	BitstreamPutBits(cinfo->pub.comps_in_scan, 8, cinfo->VirtBaseAddr);	//Ns
	for (i = 0; i < cinfo->pub.comps_in_scan; i++) {
		compptr = cinfo->cur_comp_info[i];
		BitstreamPutBits(compptr->component_id, 8, cinfo->VirtBaseAddr);
		td = compptr->dc_tbl_no;
		ta = compptr->ac_tbl_no;
		BitstreamPutBits((td << 4) + ta, 8, cinfo->VirtBaseAddr);
	}
	BitstreamPutBits((0 <<8)|(DCTSIZE2-1), 16, cinfo->VirtBaseAddr);
	BitstreamPutBits(0, 8, cinfo->VirtBaseAddr);		//In baseline  //Ah Al
}


static void
emit_jfif_app0 (j_compress_ptr cinfo)
/* Emit a JFIF-compliant APP0 marker */
{
  /*
   * Length of APP0 block	(2 bytes)
   * Block ID			(4 bytes - ASCII "JFIF")
   * Zero byte			(1 byte to terminate the ID string)
   * Version Major, Minor	(2 bytes - major first)
   * Units			(1 byte - 0x00 = none, 0x01 = inch, 0x02 = cm)
   * Xdpu			(2 bytes - dots per unit horizontal)
   * Ydpu			(2 bytes - dots per unit vertical)
   * Thumbnail X size		(1 byte)
   * Thumbnail Y size		(1 byte)
   */
  
  emit_marker(cinfo->VirtBaseAddr, M_APP0);
  
  BitstreamPutBits(2 + 4 + 1 + 2 + 1 + 2 + 2 + 1 + 1, 16, cinfo->VirtBaseAddr);	/* length */
  //emit_2bytes(cinfo, 2 + 4 + 1 + 2 + 1 + 2 + 2 + 1 + 1); /* length */

  BitstreamPutBits(0x4A46, 16, cinfo->VirtBaseAddr);
  BitstreamPutBits(0x4946, 16, cinfo->VirtBaseAddr);
////////  BitstreamPutBits(cinfo->JFIF_major_version, 16,BASE_ADDRESS);
  BitstreamPutBits(1, 16, cinfo->VirtBaseAddr);
  //BitstreamPutBits(cinfo->JFIF_minor_version, 8,BASE_ADDRESS);
  //BitstreamPutBits(cinfo->density_unit, 8,BASE_ADDRESS);		/* Pixel size information */
////////  BitstreamPutBits( (cinfo->JFIF_minor_version<<8)| cinfo->density_unit, 16,BASE_ADDRESS);		/* Pixel size information */
  BitstreamPutBits( (1<<8)| 0, 16, cinfo->VirtBaseAddr);		/* Pixel size information */
////////  BitstreamPutBits((int) cinfo->X_density, 16,BASE_ADDRESS);
////////  BitstreamPutBits((int) cinfo->Y_density, 16,BASE_ADDRESS);
  BitstreamPutBits( 1, 16, cinfo->VirtBaseAddr);
  BitstreamPutBits( 1, 16, cinfo->VirtBaseAddr);
  //BitstreamPutBits(0, 8,BASE_ADDRESS);	
  //BitstreamPutBits(0, 8,BASE_ADDRESS);
  BitstreamPutBits(0, 16, cinfo->VirtBaseAddr);	/* No thumbnail image */
}



/*
 * Write datastream header.
 * This consists of an SOI and optional APPn markers.
 * We recommend use of the JFIF marker, but not the Adobe marker,
 * when using YCbCr or grayscale data.  The JFIF marker should NOT
 * be used for any other JPEG colorspace.  The Adobe marker is helpful
 * to distinguish RGB, CMYK, and YCCK colorspaces.
 * Note that an application can write additional header markers after
 * jpeg_start_compress returns.
 */


void 
write_file_header1 (j_compress_ptr cinfo)
{
	emit_marker(cinfo->VirtBaseAddr, M_SOI);	/* first the SOI */
	if (cinfo->write_JFIF_header)	/* next an optional JFIF APP0 */
		emit_jfif_app0(cinfo);
}



void
write_frame_header1 (j_compress_ptr cinfo)
{
	int ci, prec;
	boolean is_baseline;
	jpeg_component_info *compptr;
  
	/* Emit DQT for each quantization table.
	* Note that emit_dqt() suppresses any duplicate tables.
	*/
	prec = 0;
	for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components; ci++, compptr++) {
		prec += emit_dqt(cinfo, compptr->quant_tbl_no);
	}

	/* now prec is nonzero iff there are any 16-bit quant tables. */
	is_baseline = TRUE;
	for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components; ci++, compptr++) {
		if (compptr->dc_tbl_no > 1 || compptr->ac_tbl_no > 1)
			is_baseline = FALSE;
	}
	/* Emit the proper SOF marker */
	if (is_baseline)		//pwhsu:20031015 execute this one	
		emit_sof(cinfo, M_SOF0);	/* SOF code for baseline implementation */		
	else
		emit_sof(cinfo, M_SOF1);	/* SOF code for non-baseline Huffman file */
}

/*
 * Write scan header.
 * This consists of DHT or DAC markers, optional DRI, and SOS.
 * Compressed data will be written following the SOS.
 */


void
write_scan_header1 (j_compress_ptr cinfo)
{
	int i;
	jpeg_component_info *compptr;
	/* Emit Huffman tables.
	* Note that emit_dht() suppresses any duplicate tables.
	*/
	for (i = 0; i < cinfo->pub.comps_in_scan; i++) {
		compptr = cinfo->cur_comp_info[i];
		//pwhsu:20031106   Execute this one
		/* Sequential mode: need both DC and AC tables */
		emit_dht(cinfo, compptr->dc_tbl_no, FALSE);
		emit_dht(cinfo, compptr->ac_tbl_no, TRUE);
	}
	/* Emit DRI if required --- note that DRI value could change for each scan.
	* We avoid wasting space with unnecessary DRIs, however.
	*/
	if (cinfo->DRI_emmit == 0) {
		emit_dri(cinfo);
		cinfo->DRI_emmit = 1;
	}
  	emit_sos(cinfo);
}

#endif


