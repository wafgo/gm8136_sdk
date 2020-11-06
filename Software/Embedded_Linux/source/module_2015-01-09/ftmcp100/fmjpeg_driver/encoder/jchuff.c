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
 * jchuff.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains Huffman entropy encoding routines.
 *
 * Much of the complexity here has to do with supporting output suspension.
 * If the data destination module demands suspension, we want to be able to
 * back up to the start of the current MCU.  To do this, we copy state
 * variables into local working storage, and update them back to the
 * permanent JPEG objects only upon successful completion of an MCU.
 */

#define JPEG_INTERNALS
#include "../common/jinclude.h"
#include "jenclib.h"
#include "jchuff.h"		/* Declarations shared with jcphuff.c */
#include "huftbl_new.h"
#include "huftbl_default.h"

extern unsigned int dchuftblu0;
extern unsigned int dchuftblu1;
extern unsigned int achuftblu0;
extern unsigned int achuftblu1;

void start_pass_huff1 (j_compress_ptr cinfo)
{
	//huff_entropy_enc_ptr entropy = &cinfo->entropy;
	unsigned int *curtbl, *curtbl_1, *tab1, *tab2;
	int i;

	if(cinfo->hufftbl_no==1){	//default table
		//dc 0 huffman table
		//curtbl = huftbl0_dc;
		curtbl = (unsigned int *) HUFTBL0_DC(cinfo->VirtBaseAddr);		//8800 -- 8c00

      curtbl[0]=dchuftbl0[0];
	  curtbl[1]=dchuftbl0[1];
	  curtbl[2]=dchuftbl0[2];
	  curtbl[3]=dchuftbl0[3];
	  curtbl[4]=dchuftbl0[4];
	  curtbl[5]=dchuftbl0[5];
	  curtbl[6]=dchuftbl0[6];
	  curtbl[7]=dchuftbl0[7];
	  curtbl[8]=dchuftbl0[8];
	  curtbl[9]=dchuftbl0[9];
	  curtbl[10]=dchuftbl0[10];
	  curtbl[11]=dchuftbl0[11];
	  curtbl[12]=dchuftbl0[12];
	  curtbl[13]=dchuftbl0[13];
	  curtbl[14]=dchuftbl0[14];

	  //dc 1 huffman table
	  //curtbl = huftbl1_dc;	  
		curtbl = (unsigned int *) HUFTBL1_DC(cinfo->VirtBaseAddr);		//8c00 -- 9000
	  curtbl[0]=dchuftbl1[0];
	  curtbl[1]=dchuftbl1[1];
	  curtbl[2]=dchuftbl1[2];
	  curtbl[3]=dchuftbl1[3];
	  curtbl[4]=dchuftbl1[4];
	  curtbl[5]=dchuftbl1[5];
	  curtbl[6]=dchuftbl1[6];
	  curtbl[7]=dchuftbl1[7];
	  curtbl[8]=dchuftbl1[8];
	  curtbl[9]=dchuftbl1[9];
	  curtbl[10]=dchuftbl1[10];
	  curtbl[11]=dchuftbl1[11];
	  curtbl[12]=dchuftbl1[12];
	  curtbl[13]=dchuftbl1[13];
	  curtbl[14]=dchuftbl1[14];
	  //ac 0 huffman table
	  //ac 1 huffman table
	  //curtbl = huftbl0_ac;
		curtbl = (unsigned int *) HUFTBL0_AC(cinfo->VirtBaseAddr);		//8000 -- 8400
	  //curtbl_1 = huftbl1_ac;
		curtbl_1 = (unsigned int *) HUFTBL1_AC(cinfo->VirtBaseAddr);		//8400 -- 8800
	  tab1 = achuftbl0;
	  tab2 = achuftbl1;
	  for(i=256;i!=0;i--){
	      *curtbl++=*tab1++;
	      *curtbl_1++=*tab2++;
	  }
  } 
	else {
      //dc 0 huffman table
	  //dc 1 huffman table
	  //curtbl = huftbl0_dc;
	  curtbl = (unsigned int *) HUFTBL0_DC(cinfo->VirtBaseAddr);		//8800 -- 8c00
	  //curtbl_1 = huftbl1_dc;
		curtbl_1 = (unsigned int *) HUFTBL1_DC(cinfo->VirtBaseAddr);		//8c00 -- 9000
	  tab1 = dchuftbln0;
	  tab2 = dchuftbln1;
	  for(i=15;i!=0;i--){
	      *curtbl++=*tab1++;
	      *curtbl_1++=*tab2++;
	  }
	  //ac 0 huffman table
	  //curtbl = huftbl0_ac;
		curtbl = (unsigned int *) HUFTBL0_AC(cinfo->VirtBaseAddr);		//8000 -- 8400
	  //curtbl_1 = huftbl1_ac;
		curtbl_1 = (unsigned int *) HUFTBL1_AC(cinfo->VirtBaseAddr);		//8400 -- 8800
	  tab1 = achuftbln0;
	  tab2 = achuftbln1;
	  for(i=256;i!=0;i--){
	      *curtbl++=*tab1++;
	      *curtbl_1++=*tab2++;
	  }

  }
  /* Initialize restart stuff */
}
#if 0
/*
 * Compute the derived values for a Huffman table.
 * This routine also performs some validation checks on the table.
 *
 * Note this is also used by jcphuff.c.
 */

void
jpeg_make_c_derived_tbl (j_compress_ptr cinfo, boolean isDC, int tblno,
			 c_derived_tbl ** pdtbl)
{
  JHUFF_TBL *htbl;
  c_derived_tbl *dtbl;
  int p, i, l, lastp, si, maxsymbol;
  char huffsize[257];
  unsigned int huffcode[257];
  unsigned int code;
  //int k1; //pwhsu++:20031022
  unsigned int *curtbl;		//pwhsu++:20040107
  unsigned int codesize;	//pwhsu++:20040107

  D_DEBUG("jpeg_make_c_derived_tbl START =====\n");
  /* Note that huffsize[] and huffcode[] are filled in code-length order,
   * paralleling the order of the symbols themselves in htbl->huffval[].
   */

  /* Find the input Huffman table */
  htbl =
    isDC ? cinfo->dc_huff_tbl_ptrs[tblno] : cinfo->ac_huff_tbl_ptrs[tblno];

  /* Allocate a workspace if we haven't already done so. */
  if (*pdtbl == NULL) {
    *pdtbl = (c_derived_tbl *) cinfo->pfnMalloc(sizeof(c_derived_tbl),
							 cinfo->u32CacheAlign,
							 cinfo->u32CacheAlign);
	if ( !(*pdtbl)) {
		F_DEBUG("can't allocate c_derived_tbl\n");
	}else{
		cinfo->adctbl[cinfo->adc_idx++]  = *pdtbl;	// TCKUO add
		//D_DEBUG("allocate c_derived_tbl %d\n", cinfo->adc_idx);
	
	}
  }	
  dtbl = *pdtbl;
  
  /* Figure C.1: make table of Huffman code length for each symbol */
  p = 0;
  for (l = 1; l <= 16; l++) {
    i = (int) htbl->bits[l];
    while (i--)
      huffsize[p++] = (char) l;
  }
  huffsize[p] = 0;
  lastp = p;
  
  /* Figure C.2: generate the codes themselves */
  /* We also validate that the counts represent a legal Huffman code tree. */

  code = 0;
  si = huffsize[0];
  p = 0;
  while (huffsize[p]) {
    while (((int) huffsize[p]) == si) {
      huffcode[p++] = code;
      code++;
    }
    /* code is now 1 more than the last code used for codelength si; but
     * it must still fit in si bits, since no code is allowed to be all ones.
     */
    code <<= 1;
    si++;
  }
  
  /* Figure C.3: generate encoding tables */
  /* These are code and size indexed by symbol value */

  /* Set all codeless symbols to have code length 0;
   * this lets us detect duplicate VAL entries here, and later
   * allows emit_bits to detect any attempt to emit such symbols.
   */
  /* This is also a convenient place to check for out-of-range
   * and duplicated VAL entries.  We allow 0..255 for AC symbols
   * but only 0..15 for DC.  (We could constrain them further
   * based on data depth and mode, but this seems enough.)
   */
  maxsymbol = isDC ? 15 : 255;

  //pwhsu++:20040107
  if(isDC){
	  curtbl = tblno ? huftbl1_dc : huftbl0_dc;
  }else{
	  curtbl = tblno ? huftbl1_ac : huftbl0_ac;
  }


  for (p = 0; p < lastp; p++) {
    	i = htbl->huffval[p];
	codesize = (huffcode[p]<< (5 + i%16) ) | (huffsize[p]+i%16);
#ifdef AHB_interface
	//__asm{
	//	STR	codesize, [curtbl+i]
	//}
	curtbl[i]= codesize;
#else		
	curtbl[i] = codesize;
#endif

  }
   F_DEBUG("jpeg_make_c_derived_tbl FINISH =====\n");
}
#endif

