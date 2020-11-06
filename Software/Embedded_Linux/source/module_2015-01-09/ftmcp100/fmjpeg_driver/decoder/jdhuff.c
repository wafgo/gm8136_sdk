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
 * jdhuff.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains Huffman entropy decoding routines.
 *
 * Much of the complexity here has to do with supporting input suspension.
 * If the data source module demands suspension, we want to be able to back
 * up to the start of the current MCU.  To do this, we copy state variables
 * into local working storage, and update them back to the permanent
 * storage only upon successful completion of an MCU.
 */

#include "../common/jinclude.h"
#include "jdeclib.h"
//#include "jdhuff.h"		/* Declarations shared with jdphuff.c */


/*
 * Compute the derived values for a Huffman table.
 * This routine also performs some validation checks on the table.
 *
 * Note this is also used by jdphuff.c.
 */

extern int dump_n_frame;    // Tuba 20110720_0: add debug proc
static char huffsize[257];          // Tuba 20130205: reduce size of local variable
static unsigned int huffcode[257];  // Tuba 20130205: reduce size of local variable
static unsigned int huffsize_1[257];    // Tuba 20130205: reduce size of local variable
static unsigned int huffvalue_1[257];   // Tuba 20130205: reduce size of local variable
#if TUBA_LIMIT_TABLE_SIZE   // tuba 20110721_0: add limit to huffman tbale
int
#else
void
#endif
jpeg_make_d_derived_tbl (j_decompress_ptr dinfo, boolean isDC, int tblno,d_derived_tbl ** pdtbl)
{
  	JHUFF_TBL *htbl=NULL;
  	d_derived_tbl *dtbl=NULL;
  	d_derived_tbl *dtbltmp=NULL;
  	int p, i, l, si, numsymbols;
  	int lookbits, ctr;
  	//char huffsize[257];         // Tuba 20130205: reduce size of local variable
  	//unsigned int huffcode[257]; // Tuba 20130205: reduce size of local variable
  	unsigned int code;
	//boolean isdc=isDC;

  
  	/* Note that huffsize[] and huffcode[] are filled in code-length order,
   	  * paralleling the order of the symbols themselves in htbl->huffval[].
   	  */
	
	/* Find the input Huffman table */
  	if (tblno < 0 || tblno >= NUM_HUFF_TBLS) {
		////////    ERREXIT1(dinfo, JERR_NO_HUFF_TABLE, tblno);
		D_DEBUG("jpeg_make_d_derived_tbl : tblno %d ERROR\n", tblno);
  	}
	htbl =  isDC ? dinfo->dc_huff_tbl_ptrs[tblno] : dinfo->ac_huff_tbl_ptrs[tblno];
  	if (htbl == NULL) {
		////////    ERREXIT1(dinfo, JERR_NO_HUFF_TABLE, tblno);
		D_DEBUG(" jpeg_make_d_derived_tbl: %s_htbl[ %d] 0x%x\n", 
			isDC ? "DC":"AC", 
			tblno,
			isDC ? (unsigned int)&dinfo->dc_huff_tbl_ptrs[tblno]: (unsigned int)&dinfo->ac_huff_tbl_ptrs[tblno]);
  	}
	/* Allocate a workspace if we haven't already done so. */
  	if (*pdtbl == NULL)        {
		dtbltmp  = (d_derived_tbl *) dinfo->pfnMalloc (sizeof(d_derived_tbl),
								dinfo->u32CacheAlign,	dinfo->u32CacheAlign); 
		if(dtbltmp  == NULL) {
	  		F_DEBUG("can't allocate pdtbl\n");
		} 
		dinfo->adctbl[dinfo->adc_idx++]  = dtbltmp;	// TCKUO add

        dinfo->mem_cost += sizeof(d_derived_tbl);     // Tuba 20110721_0: memeory cost
//        if (dump_n_frame)                                               // Tuba 20110720_0: add debug proc
//            printk("allocate derived table %d\n", dinfo->adc_idx-1);    // Tuba 20110720_0: add debug proc
		((d_derived_tbl *)(dtbltmp ))->tl=NULL;
      	((d_derived_tbl *)(dtbltmp ))->bl=isDC?6:8;
		*pdtbl = dtbltmp;
 	} 
  	dtbl = *pdtbl;
  	dtbl->pub = htbl;		/* fill in back link */

	/* Figure C.1: make table of Huffman code length for each symbol */
	p = 0;
	for (l = 1; l <= 16; l++) {
		i = (int) ( htbl->bits[l] );
		if (i < 0 || p + i > 256) {	/* protect against table overrun */
			////////      ERREXIT(dinfo, JERR_BAD_HUFF_TABLE);
			D_DEBUG("table overrun\n");
    	}
		while (i--) {
      		huffsize[p++] = (char) l;
    	}
	}

	huffsize[p] = 0;
	numsymbols = p;
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
    	if (((INT32) code) >= (((INT32) 1) << si)) {
		////////      ERREXIT(dinfo, JERR_BAD_HUFF_TABLE);
    	}
    	code <<= 1;
    	si++;
  	}
	/* Figure F.15: generate decoding tables for bit-sequential decoding */
  	p = 0;
  	for (l = 1; l <= 16; l++) {
    	if (htbl->bits[l]) {
      		/* valoffset[l] = huffval[] index of 1st symbol of code length l,
       		  * minus the minimum code of length l
       		  */
      		dtbl->valoffset[l] = (INT32) p - (INT32) huffcode[p];
      		p += htbl->bits[l];
      		dtbl->maxcode[l] = huffcode[p-1]; /* maximum code of length l */
    	} else {
      		dtbl->maxcode[l] = -1;	/* -1 if no codes of this length */
    	}
  	}
	dtbl->maxcode[17] = 0xFFFFFL; /* ensures jpeg_huff_decode terminates */

  	/* Compute lookahead tables to speed up decoding.
  	  * First we set all the table entries to 0, indicating "too long";
 	  * then we iterate through the Huffman codes that are short enough and
 	  * fill in all the entries that correspond to bit sequences starting
 	  * with that code.
 	  */
 	MEMZERO(dtbl->look_nbits, SIZEOF(dtbl->look_nbits));
 	p = 0;
	
  	for (l = 1; l <= HUFF_LOOKAHEAD; l++) {
    	for (i = 1; i <= (int) htbl->bits[l]; i++, p++) {
      		/* l = current code's length, p = its index in huffcode[] & huffval[]. */
      		/* Generate left-justified code followed by all possible bit sequences */
      		lookbits = huffcode[p] << (HUFF_LOOKAHEAD-l);
      		for (ctr = 1 << (HUFF_LOOKAHEAD-l); ctr > 0; ctr--) {
				dtbl->look_nbits[lookbits] = l;
				dtbl->look_sym[lookbits] = htbl->huffval[p];
				lookbits++;
      		}
    	}
  	}
	/* Validate symbols as being reasonable.
	* For AC tables, we make no check, but accept all byte values 0..255.
	* For DC tables, we require the symbols to be in range 0..15.
	* (Tighter bounds could be applied depending on the data depth and mode,
	* but this is sufficient to ensure safe decoding.)
	*/
  	if (isDC) {
    	for (i = 0; i < numsymbols; i++) {
			int sym = htbl->huffval[i];
			if (sym < 0 || sym > 15) {
				////////	ERREXIT(dinfo, JERR_BAD_HUFF_TABLE);
      		}
   		}
  	}
     	//#ifdef HUFFTBL_BUILD
  	// added by Leo for building multi-level huffman table by using huff_build() function
  	if(!dtbl->tl)  {	/* if the multiple-level huffman table is not created before */
		//unsigned int huffsize_1[257];   // Tuba 20130205: reduce size of local variable
      	//unsigned int huffvalue_1[257];  // Tuba 20130205: reduce size of local variable
      	dtbl->bl = isDC ? 6:8;  // for DC Table, we set the fist Huffman LookAhead Table Maximum Bit Length as 6
        								// as for AC Table, we set the fist Huffman LookAhead Table Maximum Bit Length as 8 
		for(p = 0; p < 257; p++) 
	  		huffsize_1[p]=huffsize[p];
      		for(p = 0; p < numsymbols; p++) 
	  			huffvalue_1[p]=htbl->huffval[p];
      		if (huft_build(dinfo,huffsize_1, numsymbols, 256,huffvalue_1,huffcode,NULL, NULL, &(dtbl->tl), &dtbl->bl)!= 0)        	{
          		// error condition...
          		D_DEBUG("huft_build  error condition...\n");
            #if TUBA_LIMIT_TABLE_SIZE   // tuba 20110721_0: add limit to huffman tbale
                return -1;
            #endif
        	}
	}
#if TUBA_LIMIT_TABLE_SIZE   // tuba 20110721_0: add limit to huffman tbale
    return 0;
#endif
}

static unsigned int scode_value;
#define STROE_SCODE( val ) scode_value=val;	
#define LOAD_SCODE( val ) val=scode_value;

/*
 * Initialize for a Huffman-compressed scan.
 */
#if TUBA_LIMIT_TABLE_SIZE   // tuba 20110721_0: add limit to huffman tbale
int
#else
void
#endif
start_pass_huff_decoder (j_decompress_ptr dinfo)
{
	huff_entropy_ptr entropy = dinfo->entropy;
	int ci, dctbl, actbl;
	jpeg_component_info * compptr;

	if (dinfo->HeaderPass) {
		unsigned int scode;
		LOAD_SCODE(scode)
		SET_MCUBR(ADDRESS_BASE, (unsigned int)dinfo->blocks_in_MCU)
		SET_SCODE(ADDRESS_BASE, scode)	
    #if TUBA_LIMIT_TABLE_SIZE   // tuba 20110721_0: add limit to huffman tbale
		return 0;       // tuba 20110721_0: add limit to huffman tbale
    #else
        return;
    #endif
	}

	for (ci = 0; ci < dinfo->pub.comps_in_scan; ci++) {
		compptr = dinfo->cur_comp_info[ci];
		dctbl = compptr->dc_tbl_no;
		actbl = compptr->ac_tbl_no;
	
		/* Compute derived values for Huffman tables */
    #if TUBA_LIMIT_TABLE_SIZE   // tuba 20110721_0: add limit to huffman tbale
		if (jpeg_make_d_derived_tbl(dinfo, TRUE, dctbl,   & entropy->dc_derived_tbls[dctbl]) < 0)
            return -1;
		if (jpeg_make_d_derived_tbl(dinfo, FALSE, actbl,  & entropy->ac_derived_tbls[actbl]) < 0)
            return -1;
    #else
        jpeg_make_d_derived_tbl(dinfo, TRUE, dctbl,   & entropy->dc_derived_tbls[dctbl]);
        jpeg_make_d_derived_tbl(dinfo, FALSE, actbl,  & entropy->ac_derived_tbls[actbl]);
    #endif
    }
    return 0;
}




//#ifdef HUFFTBL_BUILD
void ftmcp100_store_multilevel_huffman_table (j_decompress_ptr dinfo)
{
  //FTMCP100_CODEC *pCodec=(FTMCP100_CODEC *)dinfo->pCodec;
  d_derived_tbl *dtbl[4];
  unsigned int tbladdr[4];
  int di;
  huff_entropy_ptr entropy = (huff_entropy_ptr) dinfo->entropy;  
//  unsigned int mcubr_reg=0; // the MCUBR register
  unsigned int scode_reg=0; // to store the Huffman First LookAhead Table Bitlength
  
  // I got non-constant initialiser error message if I use array initialization
  // to initlaize the following variables in ARM Compiler.. So I use kind of 
  // silly way to initialze the following array of variables....*sigh*...
  dtbl[0]=(d_derived_tbl *) entropy->ac_derived_tbls[0]; // AC0
  dtbl[1]=(d_derived_tbl *) entropy->ac_derived_tbls[1]; // AC1
  dtbl[2]=(d_derived_tbl *) entropy->dc_derived_tbls[0]; // DC0
  dtbl[3]=(d_derived_tbl *) entropy->dc_derived_tbls[1]; // DC1
  tbladdr[0]=(unsigned int)HUFTBL0_AC_DEC(ADDRESS_BASE);
  tbladdr[1]=(unsigned int)HUFTBL1_AC_DEC(ADDRESS_BASE);
  tbladdr[2]=(unsigned int)HUFTBL0_DC_DEC(ADDRESS_BASE);
  tbladdr[3]=(unsigned int)HUFTBL1_DC_DEC(ADDRESS_BASE);  
	                      
  for(di=0;di<4;di++) {
      register struct huft *t; // the header entry for table  
      register struct huft *p; // the first data entry for table
      unsigned int c; // used as counter
      unsigned int e=0; // current relative word address
      unsigned int v1,v2; // the encoded value for each entry to be output
      //unsigned int offset; // the offset in word address
      unsigned int twsize; // the table size in word
      d_derived_tbl *d=dtbl[di];
      unsigned int *s=(unsigned int *)tbladdr[di]; // the base address for each table
      //unsigned int *qq;
      if(d) { // if the table is not empty
          p=d->tl; // the first data entry
          while (p != (struct huft *)NULL) {
            t=p-1;
            //offset=(t->data.offset)>>1; // to get this table's starting word address by shift right the t->data.offset
            e=(t->data.offset)>>1;
            twsize=(t->tblsize)>>1; // to get the table size in word
            for(c=0;c<twsize;c++,e++) // the t->tblsize is in half-word size
            {
                    v1=(p->tbl<<15)|(p->valid<<14)|(p->codelen<<11);
                    // it's table entry or data entry
                    v1|=((p->tbl)? p->data.offset:p->data.value);

                    p++; // advance to next half-word entry

                    v2=(p->tbl<<15)|(p->valid<<14)|(p->codelen<<11);
                    // it's table entry or data entry
                    v2|=((p->tbl)? p->data.offset:p->data.value);

                    v2=v1<<16 | v2;                    
                    s[e] = v2;
                    p++; // advance to next half-word entry
                //  }
              }
            p=t->nextpt;
		  } 
		}
	}

	// after storing the Huffman Multi-Level Table into the Local Memory, we starts to 
	// store the Huffamn First LookAhead Bit Length for each table in the register SCODE  
	//scode_reg|=( (dtbl[2]->bl? (dtbl[2]->bl-1):0)     | (dtbl[0]->bl? (dtbl[0]->bl-1)<<4:0)
	//            |(dtbl[3]->bl? (dtbl[3]->bl-1)<<8:0)  | (dtbl[1]->bl? (dtbl[1]->bl-1)<<12:0) );
	scode_reg|=(  ((dtbl[2])?(dtbl[2]->bl? (dtbl[2]->bl-1):0):0)     | ((dtbl[0])?(dtbl[0]->bl? (dtbl[0]->bl-1)<<4:0):0)
              | ((dtbl[3])?(dtbl[3]->bl? (dtbl[3]->bl-1)<<8:0):0)  | ((dtbl[1])?(dtbl[1]->bl? (dtbl[1]->bl-1)<<12:0):0) );
	SET_MCUBR(ADDRESS_BASE, (unsigned int)dinfo->blocks_in_MCU)
	SET_SCODE(ADDRESS_BASE, scode_reg)
}
//#endif


