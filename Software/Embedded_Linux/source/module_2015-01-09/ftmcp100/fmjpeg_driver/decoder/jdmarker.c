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
 * jdmarker.c
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to decode JPEG datastream markers.
 * Most of the complexity arises from our desire to support input
 * suspension: if not all of the data for a marker is available,
 * we must exit back to the application.  On resumption, we reprocess
 * the marker.
 */

#include "../common/jinclude.h"
#include "jdeclib.h"


/*
  Added by Leo
  Well, our zigzag order is shown as below :
  0   1   5   6   14  15  27  28
  2   4   7   13  16  26  29  42
  3   8   12  17  25  30  41  43
  9   11  18  24  31  40  44  53
  10  19  23  32  39  45  52  54
  20  22  33  38  46  51  55  60
  21  34  37  47  50  56  59  61
  35  36  48  49  57  58  62  63
  
  So, our zigzag order to natural order should be :
  0   1   8   16   9   2   3  10
  17  24  32  25  18  11   4   5
  12  19  26  33  40  48  41  34
  27  20  13   6   7  14  21  28
  35  42  49  56  57  50  43  36
  29  22  15  23  30  37  44  51
  58  59  52  45  38  31  39  46
  53  60  61  54  47  55  62  63
  
  Because the quantization table in Local Memory for FTMCP100 Media Coprocessor
  should be transposed according to hardware's algorithm. So we define another
  matrix for zigzag coef order to natural and transposed order to serve this purpose.
*/

/* zigzag coef order to natural and transposed order */
const int jpeg_natural_transpozed_order[DCTSIZE2] = {
  0,  8,  1, 2,  9,  16, 24, 17,
 10,  3,  4, 11, 18, 25, 32, 40,
 33, 26, 19, 12, 5,  6,  13, 20,
 27, 34, 41, 48, 56, 49, 42, 35,
 28, 21, 14,  7, 15, 22, 29, 36,
 43, 50, 57, 58, 51, 44, 37, 30,
 23, 31, 38, 45, 52, 59, 60, 53,
 46, 39, 47, 54, 61, 62, 55, 63
};

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

// read n bits from bitstream 
static unsigned int __inline BitstreamGetBits(unsigned int bitlen, unsigned int base)
{  
  unsigned int v;
  READ_BADR(base, v) 
  SET_BALR(base, bitlen)
//printk("read byte: 0x%x, %d\n", v, bitlen);
  return (v >> (32-bitlen));
}





/*
 * Routines to process JPEG markers.
 *
 * Entry condition: JPEG marker itself has been read and its code saved
 *   in dinfo->unread_marker; input restart point is just after the marker.
 *
 * Exit: if return TRUE, have read and processed any parameters, and have
 *   updated the restart point to point after the parameters.
 *   If return FALSE, was forced to suspend before reaching end of
 *   marker parameters; restart point has not been moved.  Same routine
 *   will be called again after application supplies more input data.
 *
 * This approach to suspension assumes that all of a marker's parameters
 * can fit into a single input bufferload.  This should hold for "normal"
 * markers.  Some COM/APPn markers might have large parameter segments
 * that might not fit.  If we are simply dropping such a marker, we use
 * skip_input_data to get past it, and thereby put the problem on the
 * source manager's shoulders.  If we are saving the marker's contents
 * into memory, we use a slightly different convention: when forced to
 * suspend, the marker processor updates the restart point to the end of
 * what it's consumed (ie, the end of the buffer) before returning FALSE.
 * On resumption, dinfo->unread_marker still contains the marker code,
 * but the data source will point to the next chunk of marker data.
 * The marker processor must retain internal state to deal with this.
 *
 * Note that we don't bother to avoid duplicate trace messages if a
 * suspension occurs within marker parameters.  Other side effects
 * require more care.
 */


static void
get_soi (j_decompress_ptr dinfo)
/* Process an SOI marker */
{

////////  if (dinfo->marker->saw_SOI)
////////    ERREXIT(dinfo, JERR_SOI_DUPLICATE);

  /* Reset all parameters that are defined to be reset by SOI */
  dinfo->pub.restart_interval = 0;

  /* Set initial assumptions for colorspace etc */

  dinfo->jpeg_color_space = JCS_UNKNOWN;
  dinfo->saw_JFIF_marker = FALSE;
  dinfo->JFIF_major_version = 1; /* set default JFIF APP0 values */
  dinfo->JFIF_minor_version = 1;
  dinfo->density_unit = 0;
  dinfo->X_density = 1;
  dinfo->Y_density = 1;
  dinfo->saw_Adobe_marker = FALSE;
  dinfo->Adobe_transform = 0;

  dinfo->saw_SOI = TRUE;
}


static int
get_sof (j_decompress_ptr dinfo)
/* Process a SOFn marker */
{
	INT32 length;
	int c, ci;
	jpeg_component_info * compptr;

	length = BitstreamGetBits(16, ADDRESS_BASE);
	// read  & skip data precision
	BitstreamGetBits(8, ADDRESS_BASE);
	dinfo->image_height = BitstreamGetBits(16, ADDRESS_BASE);
	dinfo->image_width = BitstreamGetBits(16, ADDRESS_BASE);
	dinfo->num_components = BitstreamGetBits(8, ADDRESS_BASE);
	length -= 8;

	if (dinfo->num_components >= 4) {
		printk ("can NOT support %d components in JPEG file\n", dinfo->num_components);
		return -1;
	}
   for (ci = 0, compptr = dinfo->comp_info; ci < dinfo->num_components;    ci++, compptr++) {		
    	compptr->component_index = ci;		
 		compptr->component_id = BitstreamGetBits(8, ADDRESS_BASE);
		c = BitstreamGetBits(8, ADDRESS_BASE);
    	compptr->hs_fact = (c >> 4) & 15; 	
    	compptr->vs_fact = (c     ) & 15;		
 		compptr->quant_tbl_no = BitstreamGetBits(8, ADDRESS_BASE);
	}
	dinfo->saw_SOF = TRUE;
	return 0;
}


static int
get_sos (j_decompress_ptr dinfo)
{
  	INT32 length;
  	int i, ci, n, c, cc;
  	jpeg_component_info * compptr;
  
	 if (! dinfo->saw_SOF) {
		printk("SOF not found, but SOS\n");
		return -1;
	 }

	length = BitstreamGetBits(16, ADDRESS_BASE);
  	/* Number of components */
	n = BitstreamGetBits(8, ADDRESS_BASE);

	if (n >= MAX_COMPS_IN_SCAN) {
		printk ("over max comps %d\n", n);
		return -1;
	}

  	dinfo->pub.comps_in_scan = n;

  	/* Collect the component-spec parameters */

  	for (i = 0; i < n; i++) {
  		cc= BitstreamGetBits(8, ADDRESS_BASE);
		c = BitstreamGetBits(8, ADDRESS_BASE);

   		for (ci = 0, compptr = dinfo->comp_info; ci < dinfo->num_components;	ci++, compptr++) {
			if (cc == compptr->component_id)
				goto id_found;
    	}
		printk ("JPEG: error SOS\n");
		return -1;
id_found:
   		dinfo->cur_comp_info[i] = compptr;
   		compptr->dc_tbl_no = c >> 4;
   		compptr->ac_tbl_no = c & 0x0F;
  	}

  	/* skip the additional scan parameters Ss, Se, Ah/Al. */
	BitstreamGetBits(24, ADDRESS_BASE);
  	return 0;
}


#define get_dac(dinfo)  skip_variable(dinfo)
#define process_COM(dinfo)  skip_variable(dinfo)
#define process_APP_0_14(dinfo, v)  get_interesting_appn(dinfo, v)
#define process_APP_other(dinfo)  skip_variable(dinfo)

static int
get_dht (j_decompress_ptr dinfo)
/* Process a DHT marker */
{
  	INT32 length;
  	UINT8 bits[17];
  	UINT8 huffval[256];
  	int i, index, count;
	int table;
  	JHUFF_TBL **htblptr;

	length = BitstreamGetBits(16, ADDRESS_BASE) - 2;
  	while (length > 16) {
		index = BitstreamGetBits(8, ADDRESS_BASE);
		table = index >> 4;
		index &= 0x0F;
		if (index >= 2 || table >= 2) {
			printk ("JPEG: DHT index error\n");
			return -1;
		}
		if (table)		/* AC table definition */
			htblptr = &dinfo->ac_huff_tbl_ptrs[index];
		else				/* DC table definition */
			htblptr = &dinfo->dc_huff_tbl_ptrs[index];

   		bits[0] = 0;
   		count = 0;
   		for (i = 1; i <= 16; i++) {
 			bits[i] = BitstreamGetBits(8, ADDRESS_BASE);
  			count += bits[i];
   		}
   		length -= 1 + 16;
		if (length < count || count > 256) {
			printk ("JPEG: DHT length error\n");
			return -1;
		}

   		for (i = 0; i < count; i++)
			huffval[i] = BitstreamGetBits(8, ADDRESS_BASE);

    	length -= count;
#if 1
    	if (*htblptr == NULL) {
      		 *htblptr = (JHUFF_TBL *) dinfo->pfnMalloc (sizeof(JHUFF_TBL),
  				dinfo->u32CacheAlign, dinfo->u32CacheAlign);
  			if( !*htblptr ) {
  				F_DEBUG("can't allocate  JHUFF_TBL\n");
				return -2;		// system error
  			}
    	}
#endif
    	MEMCOPY((*htblptr)->bits, bits, SIZEOF((*htblptr)->bits));
    	MEMCOPY((*htblptr)->huffval, huffval, SIZEOF((*htblptr)->huffval));
  	}
	return 0;
}

static void
pass_dht (j_decompress_ptr dinfo)
/* Bypass process a DHT marker */
{
	INT32 length;

	length = BitstreamGetBits(16, ADDRESS_BASE);
	length -= 2;

	for ( ;length>1; length-= 2)
		BitstreamGetBits(16, ADDRESS_BASE);

	if (length)
		BitstreamGetBits(8, ADDRESS_BASE);

	//dinfo->marker->saw_DHT = TRUE;
}

static int
get_dqt (j_decompress_ptr dinfo)
// Process a DQT marker
{
  //FTMCP100_CODEC *pCodec=(FTMCP100_CODEC *)dinfo->pCodec;
  unsigned int *pqtbl=0; // added by Leo
  
  INT32 length;
  int n, i, prec;

	length = BitstreamGetBits(16, ADDRESS_BASE) - 2;

	while (length > 0) {
		n = BitstreamGetBits(8, ADDRESS_BASE);
    	prec = n >> 4;
    	n &= 0x0F;

		switch(n) {
			case 0: pqtbl = (unsigned int *)QTBL0(ADDRESS_BASE); break;
			case 1: pqtbl = (unsigned int *)QTBL1(ADDRESS_BASE); break;
			case 2: pqtbl = (unsigned int *)QTBL2(ADDRESS_BASE); break;
			case 3: pqtbl = (unsigned int *)QTBL3(ADDRESS_BASE); break;
			default:
				printk ("Tq of DQT %d over Max 3\n", n);
				return -1;
		}

		for (i = 0; i < DCTSIZE2; i++) {	//    in baseline, prec always equal to 0
			pqtbl[jpeg_natural_transpozed_order[i]] = BitstreamGetBits(8, ADDRESS_BASE);
		}
		length -= DCTSIZE2+1;
	}
	return 0;
}


static void
get_dri (j_decompress_ptr dinfo)
/* Process a DRI marker */
{
  INT32 length;

	length = BitstreamGetBits(16, ADDRESS_BASE);

	//if (length != 4)
	//	ERREXIT(dinfo, JERR_BAD_LENGTH);

	dinfo->pub.restart_interval  = BitstreamGetBits(16, ADDRESS_BASE);
}


/*
 * Routines for processing APPn and COM markers.
 * These are either saved in memory or discarded, per application request.
 * APP0 and APP14 are specially checked to see if they are
 * JFIF and Adobe markers, respectively.
 */

#define APP0_DATA_LEN	14	/* Length of interesting data in APP0 */
#define APP14_DATA_LEN	12	/* Length of interesting data in APP14 */
#define APPN_DATA_LEN	14	/* Must be the largest of the above!! */


static void
examine_app0 (j_decompress_ptr dinfo, JOCTET * data,
	      unsigned int datalen, INT32 remaining)
/* Examine first few bytes from an APP0.
 * Take appropriate action if it is a JFIF marker.
 * datalen is # of bytes at data[], remaining is length of rest of marker data.
 */
{
	INT32 totallen = (INT32) datalen + remaining;

	if (datalen >= APP0_DATA_LEN &&
		GETJOCTET(data[0]) == 0x4A && GETJOCTET(data[1]) == 0x46 &&
		GETJOCTET(data[2]) == 0x49 && GETJOCTET(data[3]) == 0x46 &&
		GETJOCTET(data[4]) == 0) {
		/* Found JFIF APP0 marker: save info */
		dinfo->saw_JFIF_marker = TRUE;
		dinfo->JFIF_major_version = GETJOCTET(data[5]);
		dinfo->JFIF_minor_version = GETJOCTET(data[6]);
		dinfo->density_unit = GETJOCTET(data[7]);
		dinfo->X_density = (GETJOCTET(data[8]) << 8) + GETJOCTET(data[9]);
		dinfo->Y_density = (GETJOCTET(data[10]) << 8) + GETJOCTET(data[11]);
		/* Check version.
		* Major version must be 1, anything else signals an incompatible change.
		* (We used to treat this as an error, but now it's a nonfatal warning,
		* because some bozo at Hijaak couldn't read the spec.)
		* Minor version should be 0..2, but process anyway if newer.
		*/
		if (dinfo->JFIF_major_version != 1) {
			printk ("dinfo->JFIF_major_version, dinfo->JFIF_minor_version = %d, %d\n",
											dinfo->JFIF_major_version, dinfo->JFIF_minor_version);
		}
		/* Validate thumbnail dimensions and issue appropriate messages */
		if (GETJOCTET(data[12]) | GETJOCTET(data[13]))
			TRACEMS2(dinfo, 1, JTRC_JFIF_THUMBNAIL, GETJOCTET(data[12]), GETJOCTET(data[13]));
		totallen -= APP0_DATA_LEN;
		if (totallen != ((INT32)GETJOCTET(data[12]) * (INT32)GETJOCTET(data[13]) * (INT32) 3))
			TRACEMS1(dinfo, 1, JTRC_JFIF_BADTHUMBNAILSIZE, (int) totallen);
	}
	else if (datalen >= 6 &&
		GETJOCTET(data[0]) == 0x4A && GETJOCTET(data[1]) == 0x46 &&
		GETJOCTET(data[2]) == 0x58 && GETJOCTET(data[3]) == 0x58 &&
		GETJOCTET(data[4]) == 0) {
		/* Found JFIF "JFXX" extension APP0 marker */
		/* The library doesn't actually do anything with these,
		* but we try to produce a helpful trace message.
		*/
		switch (GETJOCTET(data[5])) {
			case 0x10:
				TRACEMS1(dinfo, 1, JTRC_THUMB_JPEG, (int) totallen);
				break;
			case 0x11:
				TRACEMS1(dinfo, 1, JTRC_THUMB_PALETTE, (int) totallen);
				break;
			case 0x13:
				TRACEMS1(dinfo, 1, JTRC_THUMB_RGB, (int) totallen);
				break;
			default:
				TRACEMS2(dinfo, 1, JTRC_JFIF_EXTENSION,
				GETJOCTET(data[5]), (int) totallen);
				break;
		}
	}
	else {
		/* Start of APP0 does not match "JFIF" or "JFXX", or too short */
		TRACEMS1(dinfo, 1, JTRC_APP0, (int) totallen);
	}
}


/* Examine first few bytes from an APP14.
 * Take appropriate action if it is an Adobe marker.
 * datalen is # of bytes at data[], remaining is length of rest of marker data.
 */
static void
examine_app14 (j_decompress_ptr dinfo, JOCTET * data,
												unsigned int datalen, INT32 remaining)
{
	unsigned int version, flags0, flags1;

	if (datalen >= APP14_DATA_LEN &&
		GETJOCTET(data[0]) == 0x41 && GETJOCTET(data[1]) == 0x64 &&
		GETJOCTET(data[2]) == 0x6F && GETJOCTET(data[3]) == 0x62 &&
		GETJOCTET(data[4]) == 0x65) {
    	/* Found Adobe APP14 marker */
		version = (GETJOCTET(data[5]) << 8) + GETJOCTET(data[6]);
		flags0 = (GETJOCTET(data[7]) << 8) + GETJOCTET(data[8]);
		flags1 = (GETJOCTET(data[9]) << 8) + GETJOCTET(data[10]);
		dinfo->Adobe_transform = GETJOCTET(data[11]);
		dinfo->saw_Adobe_marker = TRUE;
	} else {
		/* Start of APP14 does not match "Adobe", or too short */
		TRACEMS1(dinfo, 1, JTRC_APP14, (int) (datalen + remaining));
	}
}

static void
skip_input_data (j_decompress_ptr dinfo, long num_bytes)
{
	while(num_bytes--)
		BitstreamGetBits(8, ADDRESS_BASE);
}

static void
get_interesting_appn (j_decompress_ptr dinfo, int unread_marker)
/* Process an APP0 or APP14 marker without saving it */
{
  INT32 length;
  JOCTET b[APPN_DATA_LEN];
  unsigned int i, numtoread;

	length = BitstreamGetBits(16, ADDRESS_BASE);
  length -= 2;

  /* get the interesting part of the marker data */
  if (length >= APPN_DATA_LEN)
    numtoread = APPN_DATA_LEN;
  else if (length > 0)
    numtoread = (unsigned int) length;
  else
    numtoread = 0;
  for (i = 0; i < numtoread; i++)
	b[i] = BitstreamGetBits(8, ADDRESS_BASE);

  length -= numtoread;

  /* process it */
	switch (unread_marker) {
		case M_APP0:
		examine_app0(dinfo, b, numtoread, length);
		break;
	default:
		examine_app14(dinfo, b, numtoread, length);
		break;
	}

  /* skip any remaining data -- could be lots */
  if (length > 0)
    skip_input_data (dinfo, (long) length);
}


static void
skip_variable (j_decompress_ptr dinfo)
/* Skip over an unknown or uninteresting variable-length marker */
{
  INT32 length;

	length = BitstreamGetBits(16, ADDRESS_BASE);
  length -= 2;
  
  TRACEMS2(dinfo, 1, JTRC_MISC_MARKER, dinfo->unread_marker, (int) length);

  if (length > 0)
    skip_input_data(dinfo, (long) length);
}


/*
 * Read markers until SOS or EOI.
 *
 * Returns same codes as are defined for jpeg_consume_input:
 * JPEG_SUSPENDED, JPEG_REACHED_SOS, or JPEG_REACHED_EOI.
 */

int
read_markers (j_decompress_ptr dinfo)
{
	int c1, marker;
	int count_00 = 0;
	for (;;) {
		if (! dinfo->saw_SOI) {
			// first_marker;
			c1 = BitstreamGetBits(8, ADDRESS_BASE);
			marker = BitstreamGetBits(8, ADDRESS_BASE);
			if (c1 != 0xFF || marker != (int) M_SOI) {
				printk ("first mark is not SOI 0x%02x, 0x%02x\n", c1, marker);
				return JPEG_SUSPENDED;
			}
		}
    	else {
			// next_marker
			for (;;) {
				// Skip any non-FF bytes.
				do {
					c1 = BitstreamGetBits(8, ADDRESS_BASE);
					if (c1 == 0) {
						if (++count_00 == 5) {
							printk ("hit padding bitstream\n");
							return JPEG_SUSPENDED;
						}
					}
					else
						count_00 = 0;
				} while (c1 != 0xFF);
				// This loop swallows any duplicate FF bytes.  Extra FFs are legal as
				do {
					marker = BitstreamGetBits(8, ADDRESS_BASE);
				} while (marker == 0xFF);
				if (marker != 0)
					break;			/* found a valid marker, exit loop */
			}
    	}
 //     	printk ("marker: 0x%x\n", marker);
    	switch (marker) {
    		case M_SOI:
      			get_soi(dinfo);
      			break;
    		case M_SOF0:		/* Baseline */
    		case M_SOF1:		/* Extended sequential, Huffman */
      			if (get_sof(dinfo) < 0)
					return JPEG_SUSPENDED;
      			break;
			case M_SOF2:		/* Progressive, Huffman */
				//      ERREXIT(dinfo, JERR_NO_PROGRESSIVE); // added by Leo in order to warn the user that we did not supprt progressive mode
				return JPEG_SUSPENDED;
			case M_SOF9:		/* Extended sequential, arithmetic */
				return JPEG_SUSPENDED;
			case M_SOF10:		/* Progressive, arithmetic */
				//      ERREXIT(dinfo, JERR_NO_PROGRESSIVE); // added by Leo in order to warn the user that we did not supprt progressive mode      
				return JPEG_SUSPENDED;
    		/* Currently unsupported SOFn types */
    		case M_SOF3:		/* Lossless, Huffman */
    		case M_SOF5:		/* Differential sequential, Huffman */
    		case M_SOF6:		/* Differential progressive, Huffman */
    		case M_SOF7:		/* Differential lossless, Huffman */
    		case M_JPG:			/* Reserved for JPEG extensions */
    		case M_SOF11:		/* Lossless, arithmetic */
    		case M_SOF13:		/* Differential sequential, arithmetic */
    		case M_SOF14:		/* Differential progressive, arithmetic */
    		case M_SOF15:		/* Differential lossless, arithmetic */
			//      ERREXIT1(dinfo, JERR_SOF_UNSUPPORTED, marker);
      			break;
    		case M_SOS:
      			if (get_sos(dinfo) < 0)
	    			return JPEG_SUSPENDED;
      			return JPEG_REACHED_SOS;
    		case M_EOI:
      			return JPEG_REACHED_EOI;
    		case M_DAC:
      			get_dac(dinfo);
				break;
    		case M_DHT:
				if(dinfo->HeaderPass)
					pass_dht(dinfo);
				else {
					switch (get_dht(dinfo)) {
						case -1:
							return JPEG_SUSPENDED;
						case -2:
							return JPEG_SYSERR;
						default:
							break;
					}
				}
      			break;
    		case M_DQT:
      			if (get_dqt(dinfo) < 0)
					return JPEG_SUSPENDED;
      			break;
    		case M_DRI:
      			get_dri(dinfo);
      			break;
    		case M_APP0:
   			case M_APP14:
   				process_APP_0_14 (dinfo, marker);
   				break;
    		case M_APP1:
    		case M_APP2:
    		case M_APP3:
   			case M_APP4:
   			case M_APP5:
  			case M_APP6:
   			case M_APP7:
   			case M_APP8:
   			case M_APP9:
   			case M_APP10:
   			case M_APP11:
   			case M_APP12:
   			case M_APP13:
   			case M_APP15:
   				process_APP_other(dinfo);
   				break;
   			case M_COM:
   				process_COM (dinfo);
   				break;
			case M_RST0:		/* these are all parameterless */
			case M_RST1:
   			case M_RST2:
   			case M_RST3:
   			case M_RST4:
   			case M_RST5:
   			case M_RST6:
   			case M_RST7:
   			case M_TEM:
			//D_DEBUG("read_markers [ M_TEM ]\n");	
   				TRACEMS1(dinfo, 1, JTRC_PARMLESS_MARKER, marker);
				break;
//				return JPEG_SUSPENDED;
   			case M_DNL:			/* Ignore DNL ... perhaps the wrong thing */
   				skip_variable(dinfo);
    			break;
			default:			/* must be DHP, EXP, JPGn, or RESn */
   				 /* For now, we treat the reserved markers as fatal errors since they are
					* likely to be used to signal incompatible JPEG Part 3 extensions.
					* Once the JPEG 3 version-number marker is well defined, this code
					* ought to change!
   				    */
				//      ERREXIT1(dinfo, JERR_UNKNOWN_MARKER, marker);
				//return JPEG_SUSPENDED;
				break;
   		}
  	}
}

