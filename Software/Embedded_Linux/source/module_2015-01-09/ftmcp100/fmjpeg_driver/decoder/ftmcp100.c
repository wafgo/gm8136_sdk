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
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#endif

#include "../common/jinclude.h"
#include "ioctl_jd.h"
#include "jdeclib.h"

extern unsigned int ADDRESS_BASE;
extern int dump_n_frame;    // Tuba 20110720_0: add debug proc


#ifndef SEQ
int dma_cbycry(j_decompress_ptr dinfo)
{
	jpeg_decompress_struct_my *my = &dinfo->pub;
	jpeg_component_info *compptr = dinfo->comp_info;
	unsigned int * dma_cmd0 = (unsigned int *) (JD_DMA_CMD_OFF0 + ADDRESS_BASE);
	unsigned int * dma_cmd1 = (unsigned int *) (JD_DMA_CMD_OFF1 + ADDRESS_BASE);
	// support sample JDS_420 || JDS_422_0 || JDS_422_1
	int temp= (compptr->out_width*RGB_PIXEL_SIZE / 4)
															+ 1 - (RGB_PIXEL_SIZE * DCTSIZE*2 / 4);
	int temp1= (compptr->out_width*RGB_PIXEL_SIZE / 4)
															+ 1 - (RGB_PIXEL_SIZE * DCTSIZE*4 / 4);
	// check DMA limitation
	if ((int)my->outdata[0] & 0x7) {
		F_DEBUG("Output phy address 0x%x is not 8-byte align\n", (int)my->outdata[0]);
		return -1;
	}
	if ((compptr->out_width*RGB_PIXEL_SIZE) & 3) {
		F_DEBUG("2 times of output_width %d is not align to 4 bytes\n",
																							compptr->out_width);
		return -1;
	}
	if (my->comps_in_scan != dinfo->num_components) {
		F_DEBUG("not support this non-interleave sample %d for 422 output\n",
																									my->sample|16);
		return -1;
	}
	switch (my->sample) {
		case JDS_420:
			dma_cmd0[1] = mDmaLocMemAddr14b(RGB_OFF0);
			dma_cmd1[1] = mDmaLocMemAddr14b(RGB_OFF1);
			dma_cmd0[2] = 
			dma_cmd1[2] = (temp << 6) | (RGB_PIXEL_SIZE * PIXEL_Y / 8);
			dma_cmd0[3] =
			dma_cmd1[3] = 0x0910000|(RGB_PIXEL_SIZE * DCTSIZE2*4 / 4); // 16*16 pixel
			break;
		case JDS_422_0:
			dma_cmd0[1] =
				mDmaLoc2dWidth4b(2) |								// 2 blocks : left+right
				mDmaLoc2dOff8b(1-(RGB_PIXEL_SIZE * DCTSIZE2*2/4)) |	// jump to next 2 blocks
				mDmaLoc3dWidth4b(0) |
				mDmaLocMemAddr14b(RGB_OFF0);
			dma_cmd1[1] =
				mDmaLoc2dWidth4b(2) |								// 2 blocks : left+right
				mDmaLoc2dOff8b(1-(RGB_PIXEL_SIZE * DCTSIZE2*2/4)) |	// jump to next 2 blocks
				mDmaLoc3dWidth4b(0) |
				mDmaLocMemAddr14b(RGB_OFF1);
			dma_cmd0[2] = 
			dma_cmd1[2] =
				mDmaSysWidth6b(RGB_PIXEL_SIZE * DCTSIZE*4 / 8) |
				mDmaSysOff14b(temp1) |
				mDmaLocWidth4b(RGB_PIXEL_SIZE * DCTSIZE*2 / 4) |
				mDmaLocOff8b((RGB_PIXEL_SIZE * DCTSIZE2*2 / 4) + 1 - (RGB_PIXEL_SIZE * DCTSIZE*2 / 4));
			dma_cmd0[3] =
			dma_cmd1[3] = 0x0910000|(RGB_PIXEL_SIZE * DCTSIZE2*4 / 4) | // 16*16 pixel
										(2 << 18)	;	// local 3D
			break;
		case JDS_422_1:
			dma_cmd0[1] = mDmaLocMemAddr14b(RGB_OFF0);
			dma_cmd1[1] = mDmaLocMemAddr14b(RGB_OFF1);
			dma_cmd0[2] = 
			dma_cmd1[2] = (temp << 6) | (RGB_PIXEL_SIZE * DCTSIZE*2 / 8);
			dma_cmd0[3] =
			dma_cmd1[3] = 0x0910000|(RGB_PIXEL_SIZE* DCTSIZE2*2/4); // only 16*8 pixel
			break;
		default:
			F_DEBUG("not support sampling %d on CbYCrY Output\n", dinfo->pub.sample);
			return -1;
	}
	return 0;
}
#endif

static int dma_yuv_init(j_decompress_ptr dinfo)
{
	int ci;
	int comp_w_off;
	unsigned int local = CUR_B0;
	unsigned int * dma_cmd = (unsigned int *) (JD_DMA_CMD_OFF0 + ADDRESS_BASE);
	jpeg_component_info *cmpp;
	jpeg_decompress_struct_my *my = &dinfo->pub;

	if ((my->format == OUTPUT_420_YUV) &&
			((my->sample != JDS_422_1) && (my->sample != JDS_422_0) && (my->sample != JDS_420)) ){
		F_DEBUG("not support this sampling %d on 420 YUV Output\n", my->sample);
		return -1;
	}

	for (ci = 0, cmpp = dinfo->cur_comp_info[0]; ci < dinfo->pub.comps_in_scan; ci++, cmpp++) {
		unsigned int dma_1, dma_2, dma_3;
		int blk_sz = cmpp->hs_fact*cmpp->vs_fact * DCTSIZE2;
//printk("ci = %d, index = %d, id = %d\n", ci, cmpp->component_index, cmpp->component_id);
		// check DMA limitation
		if ((int)my->outdata[cmpp->component_index] & 0x7) {
			F_DEBUG("Output phy address[%d] 0x%x is not 8-byte align\n",
				cmpp->component_index, (int)my->outdata[cmpp->component_index]);
			return -1;
		}
		if (cmpp->out_width & 3) {
			F_DEBUG("component output_width %d is not align to 4 bytes\n", cmpp->out_width);
			return -1;
		}

//		comp_w_off = (dinfo->pub.MCUs_per_row -1) * cmpp->hs_fact*DCTSIZE;
		comp_w_off = cmpp->out_width -cmpp->hs_fact*DCTSIZE;
		if ((cmpp->component_index == 0) || (my->sample == JDS_420) ||
																			(my->format == OUTPUT_YUV)) {
			dma_1 = (8 << 28) |			// Loc3Dwidth, 8 lines in one block
						(((1 - (cmpp->hs_fact - 1)*DCTSIZE2/4) & 0xFF)<<20) |	// Loc2Doff
						(cmpp->hs_fact << 16) |													// Loc2Dwidth
						mDmaLocMemAddr14b (local);
			local += blk_sz;
		}
		else {
			// skip one line each 2 lines, JDS_422_0, JDS_422_1 at U/V component
			dma_1 = (8 << 28) |			// Loc3Dwidth, 8 lines in one block
						(((1 - ((cmpp->hs_fact - 1)*DCTSIZE2-DCTSIZE)/4) & 0xFF)<<20) |	// Loc2Doff
						(cmpp->hs_fact << 16) |													// Loc2Dwidth
						mDmaLocMemAddr14b (local);
			local += blk_sz;
			blk_sz >>= 1;
		}
		dma_2 = ((DCTSIZE2/4 + 1 - DCTSIZE/4)<<24) |		// LocOff
						((DCTSIZE/4)<<20)	| 									// LocWidth
						mDmaSysOff14b(comp_w_off/4+1) |			// SysOff
						mDmaSysWidth6b(cmpp->hs_fact*DCTSIZE/8);
		dma_3 = 
					(1 << 28) |			// Loc3Doff: 1, last line of blk1 to first line of blk2
					(0x9<<20) |		// local->sys, dma_en
					(0xd<<16) |		// sys:2d, local: 4d
					(blk_sz/4);

		if (ci != (dinfo->pub.comps_in_scan - 1))// not last one comp.
			dma_3 |=  ((0x1<<26) |	// mask interrupt
							(1 << 21));		// chain enable
#ifndef SEQ
		dma_cmd[1+JD_DMA_STRIDE] = dma_1 + mDmaLocMemAddr14b(STRIDE_MCU);
		dma_cmd[2+JD_DMA_STRIDE] = dma_2;
		dma_cmd[3+JD_DMA_STRIDE] = dma_3;
#endif
		dma_cmd[1] = dma_1;
		dma_cmd[2] = dma_2;
		dma_cmd[3] = dma_3;
		dma_cmd += 4;
	}
	return 0;
}
#if 0
int dma_yuv(j_decompress_ptr dinfo)
{
	jpeg_decompress_struct_my *my = &dinfo->pub;
	int ci;
	int bc;
	if ((my->format == OUTPUT_420_YUV) &&
						((my->sample != JDS_422_1) &&
						 (my->sample != JDS_422_0) &&
						 (my->sample != JDS_420)) ){
		F_DEBUG("not support this sampling %d on 420 YUV Output\n", my->sample);
		return -1;
	}
	for (ci = 0, bc = 0; ci < my->comps_in_scan; ci++)  {
		int temp;
		int bi;
   		jpeg_component_info *compptr = dinfo->cur_comp_info[ci];
		temp= (compptr->out_width/4) + 1 - (DCTSIZE / 4);
		// check DMA limitation
		if ((int)my->outdata[compptr->component_index] & 0x7) {
			F_DEBUG("Output phy address[%d] 0x%x is not 8-byte align\n",
				compptr->component_index, (int)my->outdata[compptr->component_index]);
			return -1;
		}
		if (compptr->out_width & 3) {
			F_DEBUG("component output_width %d is not align to 4 bytes\n", compptr->out_width);
			return -1;
		}
		for(bi=0;bi<(compptr->hs_fact * compptr->vs_fact);bi++)  {
			my->pLDMA[  1+bc*4] = mDmaLocMemAddr14b(CUR_B0+0x40*bc);
			my->pLDMA[41+bc*4] = mDmaLocMemAddr14b(CUR_B0+0x40*bc + Stride_MCU);
//			if ((ci == 0) || (dinfo->sample == JCS_yuv420)) {
//			if ((compptr->hs_fact != 0) || (my->sample == JDS_420) ||
			if ((compptr->component_index == 0) || (my->sample == JDS_420) ||
																			(my->format == OUTPUT_YUV)) {
				my->pLDMA[  2+bc*4] =
				my->pLDMA[42+bc*4] = (temp << 6) | (DCTSIZE/8);
				//chain enable , make it group 2 ,sync to MC done, length = 64B, 1D local
				my->pLDMA[  3+bc*4] =
				my->pLDMA[43+bc*4] = 0x4B12010;
			}
			else {
				// skip one line each 2 lines, JDS_422_0, JDS_422_1 at U/V component
				my->pLDMA[  2+bc*4] =
				my->pLDMA[42+bc*4] =
					mDmaSysWidth6b(DCTSIZE/8) |
					mDmaSysOff14b(temp) | 
					mDmaLocWidth4b(DCTSIZE/4) |
					mDmaLocOff8b((2 * DCTSIZE / 4) + 1 - (DCTSIZE / 4));
				//chain enable , make it group 2 ,sync to MC done, length = 32B, 2D local
				my->pLDMA[  3+bc*4] =
				my->pLDMA[43+bc*4] = 0x04B52008;
			}
			bc++;
		}
	}
	//disable chain & interrupt_mask at the end of blocks
	my->pLDMA[dinfo->blocks_in_MCU*4 - 1] =
	my->pLDMA[dinfo->blocks_in_MCU*4 - 1+40] =
//			0x00910000 | (my->pLDMA[dinfo->blocks_in_MCU*4 - 1] & 0x3FF);
			my->pLDMA[dinfo->blocks_in_MCU*4 - 1] & (~0x04200000);
	return 0;
}
#endif
int jd_dma_cmd_init (j_decompress_ptr dinfo)
{
	jpeg_decompress_struct_my *my = &dinfo->pub;
	int ci,bi;
	jpeg_component_info *compptr;
	unsigned int comp=0;
	unsigned int bc=0; // block counts

#ifdef SEQ
	if (my->format != OUTPUT_YUV) {
		printk ("Only support YUV output format (cur setting: %d) due to Seq. limitation\n", my->format);
		return -1;
	}
#endif
#ifdef BANK1_ONLY1280B		// not support DT
	if (my->format >= OUTPUT_CbYCrY) {
		printk ("Not support CbYCrY output format (cur setting: %d)\n", my->format);
		return -1;
	}
#endif
	switch (my->format) {
#ifndef SEQ
		case OUTPUT_CbYCrY:
			if (dma_cbycry(dinfo) < 0)
				return -1;
			break;
#endif
		case OUTPUT_YUV:
		case OUTPUT_420_YUV:
			if (dma_yuv_init (dinfo) < 0)
				return -1;
			break;
		default:
			F_DEBUG("not support output Format %d\n", my->format);
			return -1;
	}
  	
	bc = 0;
	for (ci = 0; ci < my->comps_in_scan; ci++)  {
    	compptr = dinfo->cur_comp_info[ci];
		if(ci<3) { // just for boundary safety
			comp |= ((compptr->ac_tbl_no << 3) |
						 (compptr->dc_tbl_no << 2) |
						 (compptr->quant_tbl_no << 0)) << (20 + 4 *ci);
		}
		for(bi=0;bi<(compptr->hs_fact * compptr->vs_fact);bi++) {
    	    comp |= ci << (bc<<1);
			bc++;
		}
    }
	//printk ("MCUTIR: 0x%08x\n", comp);
	SET_MCUTIR(ADDRESS_BASE, comp)
  	
if (0){
	int i;
	unsigned int * dma_cmd = (unsigned int *) (JD_DMA_CMD_OFF0+ ADDRESS_BASE);
	for (i = 0; i < 3; i ++) {
		printk ("dma chn = %d\n", i);
		printk ("0: %08x\n", dma_cmd[4*i+0]);
		printk ("1: %08x\n", dma_cmd[4*i+1]);
		printk ("2: %08x\n", dma_cmd[4*i+2]);
		printk ("3: %08x\n", dma_cmd[4*i+3]);
	}
}
	return 0;
}

// copied from GZIP source codes
/*
   Huffman code decoding is performed using a multi-level table lookup.
   The fastest way to decode is to simply build a lookup table whose
   size is determined by the longest code.  However, the time it takes
   to build this table can also be a factor if the data being decoded
   is not very long.  The most common codes are necessarily the
   shortest codes, so those codes dominate the decoding time, and hence
   the speed.  The idea is you can have a shorter table that decodes the
   shorter, more probable codes, and then point to subsidiary tables for
   the longer codes.  The time it costs to decode the longer codes is
   then traded against the time it takes to make longer tables.

   This results of this trade are in the variables lbits and dbits
   below.  lbits is the number of bits the first level table for literal/
   length codes can decode in one step, and dbits is the same thing for
   the distance codes.  Subsequent tables are also less than or equal to
   those sizes.  These values may be adjusted either when all of the
   codes are shorter than that, in which case the longest code length in
   bits is used, or when the shortest code is *longer* than the requested
   table size, in which case the length of the shortest code in bits is
   used.

   There are two different values for the two tables, since they code a
   different number of possibilities each.  The literal/length table
   codes 286 possible values, or in a flat code, a little over eight
   bits.  The distance table codes 30 possible values, or a little less
   than five bits, flat.  The optimum values for speed end up being
   about one bit more than those, so lbits is 8+1 and dbits is 5+1.
   The optimum values may differ though from machine to machine, and
   possibly even between compilers.  Your mileage may vary.
 */

//int huft_build(b, n, s, d, e, t, m)
//unsigned *b;            /* code lengths in bits (all assumed <= BMAX) */
//unsigned n;             /* number of codes (assumed <= N_MAX) */
//unsigned s;             /* number of simple-valued codes (0..s-1) */
//unsigned *v;            /* added by Leo, the array of huffman value */
//unsigned *ct;           /* added by Leo, the table of huffman codeword */
//ush *d;                 /* list of base values for non-simple codes */
//ush *e;                 /* list of extra bits for non-simple codes */
//struct huft **t;        /* result: starting table */
//int *m;                 /* maximum lookup bits, returns actual */
int huft_build(j_decompress_ptr dinfo,unsigned *b,unsigned n,unsigned s,unsigned int *v,unsigned *ct,unsigned short *d, unsigned short *e, struct huft **t, int *m)
/* Given a list of code lengths and a maximum table size, make a set of
   tables to decode that set of codes.  Return zero on success, one if
   the given code set is incomplete (the tables are still built in this
   case), two if the input is invalid (all zero length codes or an
   oversubscribed set of lengths), and three if not enough memory. */
{
	unsigned a;                   /* counter for codes of length k */
	unsigned c[BMAX+1];           /* bit length count table */
	unsigned f;                   /* i repeats in table every f entries */
	int g;                        /* maximum code length */
	int h;                        /* table level */
	register unsigned i;          /* counter, current code */
	register unsigned j;          /* counter */
	register int k;               /* number of bits in current code */
	int l;                        /* bits per table (returned in m) */
	register unsigned *p;         /* pointer into c[], b[], or v[] */
	register struct huft *q=0;      /* points to current table */

	struct huft r;                /* table entry for structure assignment */
	struct huft *u[BMAX];         /* table stack */
	register int w;               /* bits before this table == (l * h) */
	unsigned x[BMAX+1];           /* bit offsets, then code stack */
	unsigned *xp;                 /* pointer into x */
	unsigned z;                   /* number of entries in current table */

	unsigned int ii,cc=0; // added by Leo for another counter
	unsigned int mask; // added by Leo for mask

  unsigned int hufts=0; // add by Leo, number of valid struct huft entry allocated so far
  unsigned int hufts_stk[BMAX];  // added by Leo, start offset of each table's stack

	/* Generate counts for each bit length */
	//memzero(c, sizeof(c));
	memset ((char *)(c), 0, sizeof(c));

	p = b;  i = n;
	do {
		c[*p]++;                    /* assume all entries <= BMAX */
		p++;                      /* Can't combine with above line (Solaris bug) */
	} while (--i);
	if (c[0] == n) {               /* null input--all zero length codes */
    	*t = (struct huft *)NULL;
		*m = 0;
		return 0;
	}

	/* Find minimum and maximum length, bound *m by those */
	l = *m;
	for (j = 1; j <= BMAX; j++)
		if (c[j])
			break;
	k = j;                        /* minimum code length */
	if ((unsigned)l < j)
		l = j;
	for (i = BMAX; i; i--)
		if (c[i])
			break;
	g = i;                        /* maximum code length */
	if ((unsigned)l > i)
		l = i;
	*m = l;

	/* Adjust last length count to fill out codes, if needed */
	//for (y = 1 << j; j < i; j++, y <<= 1)
	//if ((y -= c[j]) < 0)
		//return 2;                 /* bad input: more codes than bits */
	//if ((y -= c[i]) < 0)
		//return 2;
	//c[i] += y;

	/* Generate starting offsets into the value table for each length */
	x[1] = j = 0;
	p = c + 1;  xp = x + 2;
	while (--i) {                 /* note that i == g from above */
		*xp++ = (j += *p++);
	}

	/* Make a table of values in order of bit lengths */
	//p = b;  i = 0;
	//do {
		//if ((j = *p++) != 0)
			//v[x[j]++] = i;
	//} while (++i < n);

	/* Generate the Huffman codes and for each, make the table entries */
	x[0] = i = 0;                 /* first Huffman code is zero */
	i=*ct; // added by Leo
  
	p = v;                        /* grab values in bit order */
	h = -1;                       /* no tables yet--level -1 */
	w = -l;                       /* bits decoded == (l * h) */
	//u[0] = (struct huft *)NULL;   /* just to keep compilers happy *///TCKUO modify
	//q = (struct huft *)NULL;      /* ditto */ //TCKUO modify
	z = 0;                        /* ditto */

	/* go through the bit lengths (k already is bits in shortest code) */
	for (; k <= g; k++) {
		a = c[k];
		while (a--) {
			/* here i is the Huffman code of length k bits for value *p */
			/* make tables up to required level */
			while (k > w + l) {
				h++;
				w += l;                 /* previous table always l bits */

				/* compute minimum size table less than or equal to l bits */        
				z = (z = g - w) > (unsigned)l ? l : z;  /* upper limit on table size */
				if ((f = 1 << (j = k - w)) > a + 1)  {   /* try a k-w bit table */
					/* too few codes for k-w bit table */
					f -= a + 1;           /* deduct codes from patterns left */
					xp = c + k;
					while (++j < z) {       /* try smaller tables up to z bits */
						if ((f <<= 1) <= *++xp)
							break;            /* enough codes to use up j bits */
						f -= *xp;           /* else deduct codes from patterns */
					}
				}

				j=(j>z)?z:j;   // added by Leo for limiting the maximum upper limit for working around ConvGrid.jpg and huffman table2 jpg test patterns         
				z = 1 << j;             /* table entries for j-bit table */
				//z = 1 << ((j>z)?z:j);             /* table entries for j-bit table */ // modified by Leo
            #if TUBA_LIMIT_TABLE_SIZE   // tuba 20110721_0: add limit to huffman tbale
                if (z > 256) {
                    printk("table too large %d\n", (1<<z));
                    return -1;
                }
            #endif
				/* allocate and link in new table */
				q=(struct huft *) dinfo->pfnMalloc((z + 1)*sizeof(struct huft), dinfo->u32CacheAlign, dinfo->u32CacheAlign);
				if (q == NULL) {
					F_DEBUG("can't allocate huft\n");
					return 3;             /* not enough memory */
				}
				dinfo->huftq[dinfo->huftq_idx++] = q;
                dinfo->mem_cost += (z + 1)*sizeof(struct huft);     // Tuba 20110721_0: memeory cost
//                if (dump_n_frame)                                                           // Tuba 20110720_0: add debug proc
//                    printk("allocate huff entity(%d) size %d\n", dinfo->huftq_idx-1, z+1);  // Tuba 20110720_0: add debug proc
				// added by Leo , we clean the allocated memory
				//MEMZERO(q,(z + 1)*sizeof(struct huft)); let's don't clean this memory for hardware simulation purpose

				*t = (struct huft *)(q + 1);             /* link to list for huft_free() */
				*(t = &(q->nextpt)) = (struct huft *)NULL;

				q->tblsize=z; // record the size of table in halfword size
				//u[h] = ++q;   // table starts after link
				hufts=(hufts%z)?(hufts+z-(hufts%z)):hufts;
				hufts_stk[h]=hufts;
				q->data.offset=hufts; // record the offset of this table
				hufts += z;
				u[h] = ++q;   // table starts after link
				/* connect to last table, if there is one */
				if (h) {
					x[h] = i>> (k-w);             /* save pattern for backing up */
					// added by Leo
					r.tbl=1;
					r.valid=1;		  
					r.codelen=j-1;          
					r.data.offset=hufts_stk[h];
					//j = i >> (w - l);     /* (get around Turbo C bug) */
					//u[h-1][j] = r;        /* connect to last table */
					ii = (i&(((1<<l)-1)<<(k-w))) >> (k-w);
					u[h-1][ii] = r;        /* connect to last table */		  
				}
			}			//	end of while (k > w + l)

			r.tbl=0;
			r.valid=1;
			r.codelen=(k-w)-1;      
			r.data.value=*p++;      
			/* fill code-like entries with r */
			//f = 1 << (k - w);
			//for (j = i >> w; j < z; j += f)
			//  q[j] = r;
			f = (j-(k-w));
			mask = (1<<(k-w))-1;  // to be used as mask
			i = (i&mask) << f;
			for (ii = 0 ; ii < (unsigned)(1<<f); ii++,i++)
				q[i] = r;

			/* backwards increment the k-bit code i */
			//for (j = 1 << (k - 1); i & j; j >>= 1)
			//  i ^= j;
			//i ^= j;
			i=*(++ct); // modified by Leo. get next huffman code
			cc++;
			/* backup over finished tables */
			//while ((i & ((1 << w) - 1)) != x[h])
			f=b[cc]-w; // get the next symbol's code length
			while ( ((i&(((1<<w)-1)<<f))>>f) != x[h] ) {
				h--;                    /* don't need to update q */
				w -= l;
			}
		}	//	end of while (a--)
	}     // 	end of for loop

	/* Return true (1) if we were given an incomplete table */
	//return y != 0 && g != 1;  
	return 0; // modified by Leo
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


