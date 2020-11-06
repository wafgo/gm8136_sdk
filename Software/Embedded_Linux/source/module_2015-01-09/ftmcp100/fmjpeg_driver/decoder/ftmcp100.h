#ifndef FTMCP100_H
#define FTMCP100_H

#include "../common/ftmcp.h"

// local start address
#ifdef GM8120
	#define mDmaLocMemAddr14b(v) ((unsigned int)v)
#else
//	#define mDmaLocMemAddr14b(v) ( (((((unsigned int)v)|0x10000)>>1)&0x0fffffffc) | (((unsigned int)v)&0x03) )  
	#define mDmaLocMemAddr14b(v)  ((((unsigned int)v)>>1)&0x0fffffffc)
#endif
#define mDmaLoc2dWidth4b(v)	(((uint32_t)(v)) << 16)	// Local memory 2D width (words)
#define mDmaLoc2dOff8b(v)		((((uint32_t)(v) & 0xFF)) << 20)	// Local memory 2D offset (words)
#define mDmaLoc3dWidth4b(v)	(((uint32_t)(v)) << 28)			// Local memory 3D width (words)

// DMA_width
#define mDmaSysWidth6b(v)		((uint32_t)(v))				// System block width (1unit: 8 bytes)
#define mDmaSysOff14b(v)		(((uint32_t)(v) & 0x3FFF) << 6)	// System memory data line offset
#define mDmaLocWidth4b(v)		(((uint32_t)(v)) << 20)			// Local block width (words)
#define mDmaLocOff8b(v)		(((uint32_t)(v) & 0xFF) << 24)	// Local memory data line offset


//---------------------------------------------------------------------------
// DMA Control Registers Structure Definition
//---------------------------------------------------------------------------
// well, using the strcutre to do register access is possibly platform specific
// we improve it later once we are doing the abstraction of register access.
  typedef struct {
	unsigned int *SMaddr;	// 0x00
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
#if (defined(ONE_P_EXT) || defined(TWO_P_EXT))
extern int jd_dma_cmd_init (j_decompress_ptr dinfo);
extern void ftmcp100_set_mcu_dma_noninterleaved_params (j_decompress_ptr dinfo);
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//#ifdef HUFFTBL_BUILD
// copied from GZIP source codes
/* Huffman code lookup table entry--this entry is four bytes for machines
   that have 16-bit pointers (e.g. PC's in the small or medium model).
   Valid extra bits are 0..13.  e == 15 is EOB (end of block), e == 16
   means that v is a literal, 16 < e < 32 means that v is a pointer to
   the next table, which codes e - 16 bits, and lastly e == 99 indicates
   an unused code.  If a code with e == 99 is looked up, this implies an
   error in the data. */
struct huft {
  //unsigned char e;                /* number of extra bits or operation */
  //unsigned char b;                /* number of bits in this code or subcode */
  //union {
  //  unsigned short n;              /* literal, length base, or distance base */
  //  struct huft *t;     /* pointer to next level of table */
  //} v;

  // leo's fields  
  struct huft *nextpt; /* pointer to next level of table */
  unsigned int tblsize; /* the size of table, indicating the number of entry */
  unsigned int tbl:1; /* indicate it's table entry or data entry */
  unsigned int valid:1; /* indicate it is empty entry or valid data entry */
  unsigned int codelen:3; /* indicate the code length of codeword that current entry represents */
  union {
    unsigned int value:8;  /* indicate the value if it is a data entry */
    unsigned int offset:10; /* indicate the offset of next table */
  } data;
};

/* If BMAX needs to be larger than 16, then h and x[] should be ulg. */
#define BMAX 16         /* maximum bit length of any code (16 for explode) */
#define N_MAX 256       /* maximum number of codes in any set */

extern int huft_build(j_decompress_ptr dinfo,unsigned *b,unsigned n,unsigned s,unsigned int *v,unsigned int *ct,unsigned short *d, unsigned short *e, struct huft **t, int *m);


/*
 *   To transform the original Huffman table in JPEG header to a Huffman Multi-Level Table
 *   for hardware VLD engine and store the transformed Huffman Multi-Level Table to 
 *   FTMCP100 Media Coprocessor's local memory or to a local file.
 *   And we also set the first lookahead bit length for each huffman table (DC0,DC1,AC0,AC1)
 *   by using VOP0 register in FTMC100 Media Coprocessor.
 *
 */
extern void ftmcp100_store_multilevel_huffman_table (j_decompress_ptr dinfo);

#endif
#endif
