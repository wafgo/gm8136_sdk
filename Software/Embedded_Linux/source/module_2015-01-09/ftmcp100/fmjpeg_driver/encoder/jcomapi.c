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
 * jcomapi.c
 *
 * Copyright (C) 1994-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains application interface routines that are used for both
 * compression and decompression.
 */

#define JPEG_INTERNALS
#include "../common/jinclude.h"
#include "jenclib.h"





JHUFF_TBL *
jpeg_alloc_huff_table_enc (j_compress_ptr cinfo)
{
  JHUFF_TBL *tbl;

  tbl = (JHUFF_TBL *) cinfo->pfnMalloc(sizeof(JHUFF_TBL),
  							cinfo->u32CacheAlign,
  							cinfo->u32CacheAlign);
  if ( !tbl) {
		F_DEBUG("can't allocate JHUFF_TBL\n");
  }else{
  		//D_DEBUG("allocate JHUFF_TBL %d\n", cinfo->qtbl_idx);
		cinfo->htbl_idx++;
  }
  tbl->sent_table = FALSE;	/* make sure this is false in any new table */

  return tbl;
  
 }
