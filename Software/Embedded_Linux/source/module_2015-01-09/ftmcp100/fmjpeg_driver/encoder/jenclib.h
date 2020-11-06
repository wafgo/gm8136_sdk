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

#ifndef JENCLIB_H
#define JENCLIB_H
#ifdef LINUX
#include <linux/types.h>
#else
//#include "../common/type.h"
#endif
#include "../common/ftmcp.h"
#include "ioctl_je.h"
#include "../common/ftmcp_comm.h"

/***
 *  By using typedef to create a type name for DMA memory allocation function.
 *  And user can allocate memory on a specified alignment memory 
 *  by using the parameter align_size.
 *
 *  @param size is the bytes to allocate.
 *  @param align_size is the alignment value which must be power of 2.
 *  @param reserved_size is the specifed cache line.
 *  @param phy_ptr is used to return the physical address of allocated aligned memory block.
 *  @return return a void pointer to virtual address of the allocated memory block.
 *  @see FJPEG_ENC_PARAM
 *  @see DMA_FREE_PTR
 */
typedef void *(* DMA_MALLOC_PTR)(unsigned int size,unsigned char align_size,unsigned char reserved_size, void ** phy_ptr);
typedef void *(* MALLOC_PTR)(unsigned int size,unsigned char align_size,unsigned char reserved_size);

/**
 *  By using typedef to create a type name for DMA memory free function.
 *  And user can use this type of function to free a block of memory that
 *  was allocated by the DMA_MALLOC_PTR type of function.
 *
 *  @param virt_ptr is a pointer to the memory block with virtual address that was returned
 *                  previously by the type of DMA_MALLOC_PTR function.
 *  @param phy_ptr is a pointer to the memory block with physical address that was returned
 *                 previously by the type of DMA_MALLOC_PTR function.
 *  @return return void.
 *  @see FJPEG_ENC_PARAM
 *  @see DMA_MALLOC_PTR
 */
typedef void (* DMA_FREE_PTR)(void * virt_ptr, void * phy_ptr);
typedef void (* FREE_PTR)(void * virt_ptr);


#define CACHE_LINE 16
/*
 * First we include the configuration files that record how this
 * installation of the JPEG library is set up.  jconfig.h can be
 * generated automatically for many systems.  jmorecfg.h contains
 * manual configuration options that most people need not worry about.
 */

#ifndef JCONFIG_INCLUDED	/* in case jinclude.h already did */
#include "../common/jconfig.h"		/* widely used configuration options */
#endif
#include "../common/jmorecfg.h"		/* seldom changed options */

/* Various constants determining the sizes of things.
 * All of these are specified by the JPEG standard, so don't change them
 * if you want to be compatible.
 */

#define DCTSIZE		    8	/* The basic DCT block is 8x8 samples */
#define DCTSIZE2	    64	/* DCTSIZE squared; # of elements in a block */
#define NUM_QUANT_TBLS      4	/* Quantization tables are numbered 0..3 */
#define NUM_HUFF_TBLS       4	/* Huffman tables are numbered 0..3 */
#define NUM_ARITH_TBLS      16	/* Arith-coding tables are numbered 0..15 */
#define MAX_COMPS_IN_SCAN   4	/* JPEG limit on # of components in one scan */
#define MAX_SAMP_FACTOR     4	/* JPEG limit on sampling factors */

#define C_MAX_BLOCKS_IN_MCU   10 /* compressor's limit on blocks per MCU */
#ifndef D_MAX_BLOCKS_IN_MCU
#define D_MAX_BLOCKS_IN_MCU   10 /* decompressor's limit on blocks per MCU */
#endif
#include "jchuff.h"



/* Types for JPEG compression parameters and working tables. */
/* DCT coefficient quantization tables. */

typedef struct {
  /* This array gives the coefficient quantizers in natural array order
   * (not the zigzag order in which they are stored in a JPEG DQT marker).
   * CAUTION: IJG versions prior to v6a kept this array in zigzag order.
   */
////////  UINT16 quantval[DCTSIZE2];	/* quantization step for each coefficient */
  /* This field is used only during compression.  It's initialized FALSE when
   * the table is created, and set TRUE when it's been output to the file.
   * You could suppress output of a table by setting this to TRUE.
   * (See jpeg_suppress_tables for an example.)
   */
  boolean sent_table;		/* TRUE when table has been output */  
				//pwhsu:20031021 if it's FALSE, it will be write to FILE
} JQUANT_TBL;


/* Huffman coding tables. */

typedef struct {
  /* These two fields directly represent the contents of a JPEG DHT marker */
  UINT8 bits[17];		/* bits[k] = # of symbols with codes of */
				/* length k bits; bits[0] is unused */
  UINT8 huffval[256];		/* The symbols, in order of incr code length */
  /* This field is used only during compression.  It's initialized FALSE when
   * the table is created, and set TRUE when it's been output to the file.
   * You could suppress output of a table by setting this to TRUE.
   * (See jpeg_suppress_tables for an example.)
   */
  boolean sent_table;		/* TRUE when table has been output */
} JHUFF_TBL;


/* Basic info about one component (color channel). */

typedef struct {
  /* These values are fixed over the whole image. */
  /* For compression, they must be supplied by parameter setup; */
  /* for decompression, they are read from the SOF marker. */
  int component_id;		/* identifier for this component (0..255) */
  int component_index;		/* its index in SOF or cinfo->comp_info[] */
  int hs_fact;		/* horizontal sampling factor (1..4) */
  int vs_fact;		/* vertical sampling factor (1..4) */
  int quant_tbl_no;		/* quantization table selector (0..3) */
  /* These values may vary between scans. */
  /* For compression, they must be supplied by parameter setup; */
  /* for decompression, they are read from the SOS marker. */
  /* The decompressor output side may not use these variables. */
  int dc_tbl_no;		/* DC entropy table selector (0..3) */
  int ac_tbl_no;		/* AC entropy table selector (0..3) */
} jpeg_component_info;

/* Master record for a compression instance */
typedef struct {
	unsigned int bs_phy; // ben add
	JDIMENSION image_width;	/* input image width */
	unsigned int restart_interval; /* MCUs per restart, or 0 for no restart */
	int comps_in_scan;		/* # of JPEG components in this scan */
	JDIMENSION MCUs_per_row;	/* # of MCUs across the image */
	unsigned int   *dev_buffer_phy;
	unsigned int u32MotionDetection;
	unsigned char u82D;
	unsigned int roi_left_mb_x;
	unsigned int roi_left_mb_y;
	unsigned int roi_right_mb_x;
	unsigned int roi_right_mb_y;
	unsigned int sample;
	unsigned int YUVAddr[3];
} jpeg_compress_struct_my;

struct jpeg_compress_struct {
	jpeg_compress_struct_my pub;
  JDIMENSION image_height;	/* input image height */
  int num_components;		/* # of color components in JPEG image */
  jpeg_component_info * comp_info;
  jpeg_component_info * cur_comp_info[MAX_COMPS_IN_SCAN];
  /* comp_info[i] describes component that appears i'th in SOF */
  //JQUANT_TBL * quant_tbl_ptrs[NUM_QUANT_TBLS];
	boolean qtbl_sent_table[NUM_QUANT_TBLS];		/* TRUE when q table has been output */  
  /* ptrs to coefficient quantization tables, or NULL if not defined */
  JHUFF_TBL * dc_huff_tbl_ptrs[NUM_HUFF_TBLS];
  JHUFF_TBL * ac_huff_tbl_ptrs[NUM_HUFF_TBLS];

  unsigned int DRI_emmit;
  boolean write_JFIF_header;	/* should a JFIF marker be written? */
  int max_hs_fact;	/* largest h_samp_factor */
  int max_vs_fact;	/* largest v_samp_factor */

	huff_entropy_encoder entropy;

  //  store the md information of MBs, i.e., deviation, for N interval
  unsigned int   *dev_buffer_virt;
  int   dev_flag;		// index the first frame
  unsigned int u32JPEGMode;
  /// The function pointer to user-defined DMA memory allocation function.
  DMA_MALLOC_PTR pfnDmaMalloc;  /**< This variable contains the function pointer to the user-defined 
                                 *   DMA malloc function since under OS environment, our hardware device
                                 *   may need the physical address instead of virtual address.
                                 *
                                 *   @see pfnDmaFree
                                 *   @see DMA_MALLOC_PTR_dec
                                 *   @see DMA_FREE_PTR_dec
                                 */
  /// The function pointer to user-defined DMA free allocation function.
  DMA_FREE_PTR   pfnDmaFree;    /**< This variable contains the function pointer to the user-defined 
                                 *   DMA free function since under OS environment, our hardware device
                                 *   may need the physical address instead of virtual address.
                                 *
                                 *   @see pfnDmaFree
                                 *   @see DMA_MALLOC_PTR_dec
                                 *   @see DMA_FREE_PTR_dec
                                 */
  
   MALLOC_PTR 	pfnMalloc;
   FREE_PTR  	pfnFree;								 
   unsigned char u32CacheAlign;
   int qtbl_idx;
   int htbl_idx;
   void *adctbl[0x20];
   int adc_idx;                              
   //for ROI data structure 
   // the flag to indicate whether the ROI encoding is enabled or not
   int      roi_enable;
   // ROI's (region of interest) upper-left corner coordinate(x,y) of captured frame.
   unsigned int roi_x;
   unsigned int roi_y;
   unsigned int roi_w;
   unsigned int roi_h;
   unsigned int quality;
   unsigned int qtbl_no;
   unsigned int hufftbl_no;
   unsigned int *luma_qtbl;
   unsigned int *chroma_qtbl;
	 // IP virtual base address
	unsigned int VirtBaseAddr;
	unsigned int u32BaseAddr_wrapper;
	unsigned int crop_width;    //Tuba 20130311 satrt: add crop size to encode varibale size
    unsigned int crop_height;   //Tuba 20130311 satrt: add crop size to encode varibale size
};
typedef struct jpeg_compress_struct * j_compress_ptr;

struct Compress_recon{
	//Quant table
	UINT16 Inter_quant[4][DCTSIZE2]; //quant table
	unsigned int QUTTBL[4][DCTSIZE2];		//pwhsu++:20040922
};

extern struct Compress_recon rinfo; 
typedef struct
{
	unsigned int *SMaddr;	//0
	unsigned int *LMaddr;
	unsigned int BlkWidth;
	unsigned int Control;
	unsigned int CCA;			// 0x10
	unsigned int Status;
	unsigned int CCA2;
	unsigned int GRPC;
	unsigned int GRPS;		// 0x20
	unsigned int ABFCtrl;     // 0x24   Auto-buffer control
	unsigned int THRESHOLD;	// 0x28   DMA threshold value
	unsigned int AUTOINT;		// 0x2C   auto send interrupt even last command is skipped or disabled
	unsigned int BsBufSz;		// 0x30, bitstream buffer size
	unsigned int RowOffYUV[3];
} MDMA;

	typedef struct {
		volatile CODEC_COMMAND command;
		jpeg_compress_struct_my pub;
		jpeg_component_info comp_info[MAX_COMPS_IN_SCAN];
		jpeg_component_info *cur_comp_info[MAX_COMPS_IN_SCAN];
		unsigned int bitslen;				//total bitstream length
		unsigned int jpgenc_done;			//a signal for communication
	} Share_Node_JE;



extern unsigned int BASE_ADDRESS;

/* write n bits to bitstream */

static void __inline
BitstreamPutBits(unsigned int value,unsigned int size, unsigned int base)
{
//	static int count = 0;
//	count += size;
//	printk("put 0x%x %d bits, cnt = %x\n", value, size, count);
	SET_BADR(base, value)
	SET_BALR(base, size)
}


	  

//pwhsu++:20040909
#define mDMA3bOffset(value) (((unsigned int)(value))&0xf << 28)			//local mem 4d block offset
#define mDMAenb(value)		(((unsigned int)(value))&0x1 << 23)			//DMA start data transfer
#define mDMAchain(value)	(((unsigned int)(value))&0x1 << 21)			//DMA enable chain transfer
#define mDMALmtype(value)	(((unsigned int)(value))&0x3 << 18)			//DMA local mem data type
#define	mDMASmtype(value)	(((unsigned int)(value))&0x3 << 16)			//DMA sys mem data type
#define	mDMATransLen(vlaue)	((unsigned int)(value))&0xfff				//DMA transfer length

#define mDMA3bWidth(value)	(((unsigned int)(value))&0xf  << 28)			//local mem 4d block width
#define mDMA2bOffset(value)	(((unsigned int)(value))&0xff << 20)			//local mem 3d block offset
#define mDMA2bWidth(value)	(((unsigned int)(value))&0xf  << 16)			//local mem 3d block width
#ifdef GM8120
#define	mDMALmaddr(value) (((unsigned int)(value)) & 0xFFFC)							//local mem base addr
#else
#define	mDMALmaddr(value) (((unsigned int)(value)>>1) & 0xFFFC)					//local mem base addr
#endif
#define	mDMAIncrIdx(value)	((unsigned int)(vlaue))&0x3						//local mem incr index

#define mDMAbOffset(value)	(((unsigned int)(value))&0xff  <<24)				//local mem 2d block offset
#define mDMAbWidth(value)	(((unsigned int)(value))&0xf <<20)				//local mem 2d block width
#define mDMASOffset(value)	(((unsigned int)(value) & 0x3FFF) <<6)					//sys mem block offset
#define mDMASWidth(value)	((unsigned int)(value)&0x3f)					//sys mem block width, 8bytes

#define	DMA_DATA_SEQ	0
#define	DMA_DATA_2D		1
#define	DMA_DATA_3D		2
#define	DMA_DATA_4D		3

#define	DMA_COM_SYS		0
#define	DMA_COM_LOC		2
		
#define TRUE			1
#define FALSE			0



extern void emit_restart (unsigned int base, int restart_num);



/* Standard data source and destination managers: stdio streams. */
/* Caller is responsible for opening the file before and closing after. */

/* Default parameter setup for compression */
extern void jpeg_set_defaults (j_compress_ptr cinfo);
/* Compression parameter setup aids */
extern void jpeg_set_quality (j_compress_ptr cinfo);
//extern void jpeg_set_linear_quality (j_compress_ptr cinfo,
//					  int scale_factor);
extern void jpeg_add_quant_table (j_compress_ptr cinfo, int which_tbl,
				       const unsigned int *basic_table,
				       int scale_factor);

/* Main entry points for compression */
extern void jpeg_start_compress (j_compress_ptr cinfo,boolean write_quant_tables,boolean write_huffman_tables);


extern void start_pass_huff1 (j_compress_ptr cinfo);
extern void write_file_header1 (j_compress_ptr cinfo);
extern void write_frame_header1 (j_compress_ptr cinfo);
extern void write_file_trailer1 (unsigned int base);
extern void write_scan_header1 (j_compress_ptr cinfo);
extern void write_marker_header1 (j_compress_ptr cinfo, int marker, unsigned int datalen);


/* Write a special marker.  See libjpeg.doc concerning safe usage. */
extern void jpeg_write_marker(j_compress_ptr cinfo, int marker,
	     const JOCTET * dataptr, unsigned int datalen);
/* Same, but piecemeal. */

/* Alternate compression function: just write an abbreviated table file */
/* Return value is one of: */
#define JPEG_SUSPENDED		0 /* Suspended due to lack of input data */
#define JPEG_HEADER_OK		1 /* Found valid image datastream */
#define JPEG_HEADER_TABLES_ONLY	2 /* Found valid table-specs-only datastream */
/* If you pass require_image = TRUE (normal case), you need not check for
 * a TABLES_ONLY return code; an abbreviated file will cause an error exit.
 * JPEG_SUSPENDED is only possible if you use a data source module that can
 * give a suspension return (the stdio source module doesn't).
 */

/* Return value is one of: */
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


/*
 * The JPEG library modules define JPEG_INTERNALS before including this file.
 * The internal structure declarations are read only when that is true.
 * Applications using the library should not include jpegint.h, but may wish
 * to include jerror.h.
 */

#ifdef JPEG_INTERNALS
#include "../common/encint.h"
#include "../common/jerror.h" 	/* fetch error codes too */
#endif

#endif
/* JPEGLIB_H */
