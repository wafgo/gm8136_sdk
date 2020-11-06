/**
 *  \file jpeglib.h
 *  \brief The JPEG Library exported data and function declarations.
 *
 *  This file contains the IJG JPEG library's exported data and function declarations.
 *
 */
 
/*
 * jpeglib.h
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file defines the application interface for the JPEG library.
 * Most applications using the library need only include this file,
 * and perhaps jerror.h if they want to know the exact error codes.
 */

#ifndef JDECLIB_H
#define JDECLIB_H

	#ifndef JCONFIG_INCLUDED	/* in case jinclude.h already did */
		#include "../common/jconfig.h"		/* widely used configuration options */
	#endif
	#include "../common/jmorecfg.h"		/* seldom changed options */
//	#include "../common/dma.h"
	#include "../common/ftmcp_comm.h"
typedef void *(* DMA_MALLOC_PTR)(unsigned int size,unsigned char align_size,unsigned char reserved_size, void ** phy_ptr);
typedef void *(* MALLOC_PTR)(unsigned int size,unsigned char align_size,unsigned char reserved_size);
typedef void (* DMA_FREE_PTR)(void * virt_ptr, void * phy_ptr);
typedef void (* FREE_PTR)(void * virt_ptr);
#define CACHE_LINE 16
#define DCTSIZE		    8	/* The basic DCT block is 8x8 samples */
#define DCTSIZE2	    64	/* DCTSIZE squared; # of elements in a block */
#define NUM_QUANT_TBLS      4	/* Quantization tables are numbered 0..3 */
#define NUM_HUFF_TBLS	    4
#define NUM_ADC_TBLS 	NUM_HUFF_TBLS*2
#define NUM_HUFQ_TBLS	NUM_HUFF_TBLS*5
#define NUM_ARITH_TBLS      16	/* Arith-coding tables are numbered 0..15 */
#define MAX_COMPS_IN_SCAN   4	/* JPEG limit on # of components in one scan */
#define MAX_SAMP_FACTOR     4	/* JPEG limit on sampling factors */

#define C_MAX_BLOCKS_IN_MCU   10 /* compressor's limit on blocks per MCU */
#ifndef D_MAX_BLOCKS_IN_MCU
#define D_MAX_BLOCKS_IN_MCU   10 /* decompressor's limit on blocks per MCU */
#endif


/* Basic info about one component (color channel). */
typedef struct {
	int component_id;		/* identifier for this component (0..255) */
	int component_index;		/* its index in SOF or dinfo->comp_info[] */
	int hs_fact;		/* horizontal sampling factor (1..4) */
	int vs_fact;		/* vertical sampling factor (1..4) */
	int quant_tbl_no;		/* quantization table selector (0..3) */
	int dc_tbl_no;		/* DC entropy table selector (0..3) */
	int ac_tbl_no;		/* AC entropy table selector (0..3) */

	JDIMENSION width_in_blocks;
	JDIMENSION height_in_blocks;
	unsigned int out_width;	// frame buffer width for this component

	unsigned int crop_x;		// cropping area x-axis start point, unit: MCU
	unsigned int crop_y;		// cropping area y-axis start point, unit: MCU
	unsigned int crop_xend;		// cropping area width, unit: MCU,
	unsigned int crop_yend;		// cropping area height, unit: MCU,
} jpeg_component_info;

/* Known color spaces. */
typedef enum {
	JCS_UNKNOWN,		/* error/unspecified */
	JCS_GRAYSCALE,		/* monochrome */
	JCS_RGB,		/* red/green/blue */
	JCS_YCbCr,		/* Y/Cb/Cr (also known as YUV) */
	JCS_CMYK,		/* C/M/Y/K */
	JCS_YCCK		/* Y/Cb/Cr/K */
} J_COLOR_SPACE;

typedef enum {
	  OUTPUT_YUV=0,   		// for 111, 211, 222, 333, 420, 422, 444
	  OUTPUT_420_YUV=1,	       	// for 211, 420, 422 output 420YUV   
	  OUTPUT_CbYCrY=2,             // for 211, 420, 422 output CbYCrY
	  OUTPUT_RGB555=3,
	  OUTPUT_RGB565=4,
	  OUTPUT_RGB888=5
} OUTPUT_FMT;

typedef struct{
	unsigned int crop_x;		// cropping area x-axis start point, unit: MCU(if interleave)/blk(if noninterleave)
	unsigned int crop_y;		// cropping area y-axis start point, unit: MCU(if interleave)/blk(if noninterleave)
	unsigned int crop_xend;		// cropping area width, unit: MCU(if interleave)/blk(if noninterleave)
	unsigned int crop_yend;		// cropping area height, unit: MCU(if interleave)/blk(if noninterleave)
	unsigned int sample;
	unsigned int format; /**< This value is use to define output format
                                 	* OUTPUT_YUV=0,   		// for 111, 211, 222, 333, 420, 422, 444
						  			* OUTPUT_420_YUV=1,	       	// for 211, 422 output 420YUV   
									* OUTPUT_CbYCrY=2,            // for 211, 420, 422 output CbYCrY
									* OUTPUT_RGB555=3,
									* OUTPUT_RGB565=4,
									* OUTPUT_RGB888=5
									*/
	unsigned int restart_interval; /* MCUs per restart interval, or 0 for no restart */
	int comps_in_scan;		/* # of JPEG components in this scan */
	JDIMENSION total_iMCU_rows;	/* # of iMCU rows in image */
	JDIMENSION MCUs_per_row;	/* # of MCUs across the image */
	unsigned char *outdata[3]; // the output YUV 8-byte aligned buffer (pyhsical address)
//	unsigned int *pLDMA; // DMA command buffer's begin address in local memory

	int max_hs_fact;	/* largest h_samp_factor */
	int max_vs_fact;	/* largest v_samp_factor */
	JDIMENSION output_width;			/* frame buffer width */
	unsigned int bs_phy_end;	// codec bitstream buffer end phy
	int monitor;
	int monitor1;
} jpeg_decompress_struct_my;

#if (defined(ONE_P_EXT) || defined(TWO_P_EXT))
	#ifdef LINUX
		#include <linux/types.h>
	#else
		#include "../common/type.h"
	#endif

/*
 * First we include the configuration files that record how this
 * installation of the JPEG library is set up.  jconfig.h can be
 * generated automatically for many systems.  jmorecfg.h contains
 * manual configuration options that most people need not worry about.
 */
	// Sync status timeout
	#define JPEG_SYNC_TIMEOUT 0x30000
	#ifndef j_compress_ptr
		/* Huffman coding tables. */
		typedef struct {
			/* These two fields directly represent the contents of a JPEG DHT marker */
			UINT8 bits[17];		/* bits[k] = # of symbols with codes of */
										/* length k bits; bits[0] is unused */
			UINT8 huffval[256];		/* The symbols, in order of incr code length */
		} JHUFF_TBL;
	#endif

	#define HUFF_LOOKAHEAD	8	/* # of bits of lookahead */
	typedef struct {
		/* Basic tables: (element [0] of each array is unused) */
		INT32 maxcode[18];		/* largest code of length k (-1 if none) */
		/* (maxcode[17] is a sentinel to ensure jpeg_huff_decode terminates) */
		INT32 valoffset[17];		/* huffval[] offset for codes of length k */
		/* valoffset[k] = huffval[] index of 1st symbol of code length k, less
		 * the smallest code of length k; so given a code of length k, the
		 * corresponding symbol is huffval[code + valoffset[k]]
		 */
		/* Link to public Huffman table (needed only in jpeg_huff_decode) */
		JHUFF_TBL *pub;
		  /* Lookahead tables: indexed by the next HUFF_LOOKAHEAD bits of
		   * the input data stream.  If the next Huffman code is no more
		   * than HUFF_LOOKAHEAD bits long, we can obtain its length and
		   * the corresponding symbol directly from these tables.
		   */
		int look_nbits[1<<HUFF_LOOKAHEAD]; /* # bits, or 0 if too long */
		UINT8 look_sym[1<<HUFF_LOOKAHEAD]; /* symbol, or unused */

		struct huft *tl; // added by Leo, for building multi-level huffman table
		int bl; // added by Leo, the maximum bit length for the first huffman lookahead table  
	} d_derived_tbl;

	struct huff_entropy_decoder {
		/* Pointers to derived tables (these workspaces have image lifespan) */
		d_derived_tbl * dc_derived_tbls[NUM_HUFF_TBLS];
		d_derived_tbl * ac_derived_tbls[NUM_HUFF_TBLS];
	};
	typedef struct huff_entropy_decoder * huff_entropy_ptr;


	/* Master record for a decompression instance */
	struct jpeg_decompress_struct {
		jpeg_decompress_struct_my pub;
		unsigned int crop_mcu_x;		// cropping area x-axis start point, unit: MCU
		unsigned int crop_mcu_y;		// cropping area y-axis start point, unit: MCU
		unsigned int crop_mcu_xend;		// cropping area x-axis end point, unit: MCU
		unsigned int crop_mcu_yend;		// cropping area y-axis end point, unit: MCU
		MALLOC_PTR pfnMalloc;
		FREE_PTR pfnFree;
		unsigned int u32CacheAlign;
		//unsigned int static_jpg;

		JDIMENSION image_width;	/* nominal image width (from SOF marker) */
		JDIMENSION image_height;	/* nominal image height */
		int num_components;		/* # of color components in JPEG image */
		J_COLOR_SPACE jpeg_color_space; /* colorspace of JPEG image */

		JDIMENSION output_height;			/* image height */
		JDIMENSION output_width_set;		/* image width setting*/
		JDIMENSION output_height_set;	/* image height setting*/
		UINT16 buf_width_set;			/* image buffer width setting*/
		UINT16 buf_height_set;			/* image buffer height setting*/

		JHUFF_TBL * dc_huff_tbl_ptrs[NUM_HUFF_TBLS];
		JHUFF_TBL * ac_huff_tbl_ptrs[NUM_HUFF_TBLS];

		/* These parameters are never carried across datastreams, since they
		* are given in SOF/SOS markers or defined to be reset by SOI.
		*/
		jpeg_component_info * comp_info;
		jpeg_component_info * cur_comp_info[MAX_COMPS_IN_SCAN];
		/* comp_info[i] describes component that appears i'th in SOF */

		/* These fields record data obtained from optional markers recognized by
		* the JPEG library.
		*/
		boolean saw_JFIF_marker;	/* TRUE iff a JFIF APP0 marker was found */
		/* Data copied from JFIF marker; only valid if saw_JFIF_marker is TRUE: */
		UINT8 JFIF_major_version;	/* JFIF version number */
		UINT8 JFIF_minor_version;
		UINT8 density_unit;		/* JFIF code for pixel size units */
		UINT16 X_density;		/* Horizontal pixel density */
		UINT16 Y_density;		/* Vertical pixel density */
		boolean saw_Adobe_marker;	/* TRUE iff an Adobe APP14 marker was found */
		UINT8 Adobe_transform;	/* Color transform code from Adobe marker */

		int blocks_in_MCU;		/* # of DCT blocks per MCU */
		int MCU_membership[D_MAX_BLOCKS_IN_MCU];
		void *huftq[NUM_HUFQ_TBLS];
		int huftq_idx;
		void *adctbl[NUM_ADC_TBLS];
		int adc_idx;

		huff_entropy_ptr entropy;
		unsigned int HeaderPass;
		boolean saw_SOI;		/* found SOI? */
		boolean saw_SOF;		/* found SOF? */

		unsigned int crop_x_set;		// cropping area x-axis start point setting, must be multiple of Y component width
		unsigned int crop_y_set;		// cropping area y-axis start point setting, must be multiple of Y component heigth
		unsigned int crop_w_set;		// cropping area width, unit: pixel setting, must be multiple of Y component width
		unsigned int crop_h_set;		// cropping area height, unit: pixel setting,  must be multiple of Y component height
		 // IP virtual base address
		unsigned int VirtBaseAddr;
		unsigned char *bs_virt;		// codec bitstream buffer virt
		unsigned int bs_phy;			// codec bitstream buffer phy
		unsigned int bs_len;			// codec bitstream buffer length
		// user space bs buffer
		unsigned char *user_bs_virt;		// codec bitstream buffer virt
		unsigned int user_bs_len;			// codec bitstream buffer length
		
		unsigned int ci;					// current component no

        unsigned int mem_cost;  // Tuba 20110721_0: memeory cost
	};
	typedef struct jpeg_decompress_struct * j_decompress_ptr;


	/* jpeg_read_header()'s return value is one of: */
	#define JPEG_SUSPENDED		0 /* Suspended due to lack of input data */
	#define JPEG_HEADER_OK		1 /* Found valid image datastream */
//	#define JPEG_HEADER_TABLES_ONLY	2 /* Found valid table-specs-only datastream */
	#define JPEG_SYSERR		3		 // system error: malloc error

	extern int jpeg_read_header (j_decompress_ptr dinfo);
	extern boolean jpeg_finish_decompress (j_decompress_ptr dinfo, int check_EOI);
	extern void jpeg_destroy_decompress (j_decompress_ptr dinfo);

	/* #define JPEG_SUSPENDED	0    Suspended due to lack of input data */
	#define JPEG_REACHED_SOS	1 /* Reached start of new scan */
	#define JPEG_REACHED_EOI	2 /* Reached end of image */
	#define JPEG_ROW_COMPLETED	3 /* Completed one iMCU row */
	#define JPEG_SCAN_COMPLETED	4 /* Completed last iMCU row of a scan */

	/* These marker codes are exported since applications and data source modules
	 * are likely to want to use them.
	 */

	#define JPEG_RST0	0xD0	/* RST0 marker code */
	#define JPEG_EOI	0xD9	/* EOI marker code */
	#define JPEG_APP0	0xE0	/* APP0 marker code */
	#define JPEG_COM	0xFE	/* COM marker code */

	#include "../common/decint.h"		/* fetch private declarations */
	#include "../common/jerror.h"		/* fetch error codes too */
	#include "ftmcp100.h"
	extern unsigned int ADDRESS_BASE;
	extern unsigned int Stride_MCU;
#else
	#define Stride_MCU STRIDE_MCU
#endif

	typedef struct {
		volatile CODEC_COMMAND command;
		jpeg_decompress_struct_my pub;
		jpeg_component_info comp_info[MAX_COMPS_IN_SCAN];
		jpeg_component_info *cur_comp_info[MAX_COMPS_IN_SCAN];
		// it's the command to instruct the internal CPU to do something useful for external CPU
		int internal_cpu_command; // 0 : to instruct internal CPU to stop and exit decoding
                            // 1 : to instruct internal CPU to do interleaved decoding
                            // 2 : to instruct internal CPU to do non-interleaved decoding for the very first time.
                            // 3 : to instruct internal CPU to do next non-interleaved decoding.
		// it's the response from internal CPU after executing the command instructed by external CPU
		int external_cpu_response; // 0 : normal state , when internal_cpu_command was set to do decoding, the external_cpu_response will be reset to 0
                             // 1 : decoding done for interleaved decoding
                             // 2 : decoding done for non-interleaved decoding

	} Share_Node_JD;

#endif
/* JPEGLIB_H */
