/*
 * jdhuff.h
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains declarations for Huffman entropy decoding routines
 * that are shared between the sequential decoder (jdhuff.c) and the
 * progressive decoder (jdphuff.c).  No other modules need to see these.
 */



/* Derived data constructed for each Huffman table */

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

  //#ifdef HUFFTBL_BUILD
  struct huft *tl; // added by Leo, for building multi-level huffman table
  int bl; // added by Leo, the maximum bit length for the first huffman lookahead table  
  //#endif
} d_derived_tbl;



/*
 * Fetching the next N bits from the input stream is a time-critical operation
 * for the Huffman decoders.  We implement it with a combination of inline
 * macros and out-of-line subroutines.  Note that N (the number of bits
 * demanded at one time) never exceeds 15 for JPEG use.
 *
 * We read source bytes into get_buffer and dole out bits as needed.
 * If get_buffer already contains enough bits, they are fetched in-line
 * by the macros CHECK_BIT_BUFFER and GET_BITS.  When there aren't enough
 * bits, jpeg_fill_bit_buffer is called; it will attempt to fill get_buffer
 * as full as possible (not just to the number of bits needed; this
 * prefetching reduces the overhead cost of calling jpeg_fill_bit_buffer).
 * Note that jpeg_fill_bit_buffer may return FALSE to indicate suspension.
 * On TRUE return, jpeg_fill_bit_buffer guarantees that get_buffer contains
 * at least the requested number of bits --- dummy zeroes are inserted if
 * necessary.
 */

#ifdef MJPEG_MODE
extern void jpeg_reinit_huff_decoder (j_decompress_ptr cinfo);
#endif
