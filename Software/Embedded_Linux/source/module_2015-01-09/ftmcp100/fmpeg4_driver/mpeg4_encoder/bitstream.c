/**
 *  @file bitstream.c
 *  @brief The related functions for writing bitstream header are implemented in this
 *         file.
 *
 */
#ifdef LINUX
//#include "quant_matrix.h"
#include "bitstream.h"
#include "../common/zigzag.h"
#include "../common/quant_matrix.h"
#else
#include "quant_matrix.h"
#include "bitstream.h"
#include "zigzag.h"
#endif

/*****************************************************************************
 * inline Functions
 ****************************************************************************/

/* write custom quant matrix */
#ifdef TWO_P_INT
void
#else
void  __inline 
#endif
bs_put_matrix(volatile MP4_t * ptMP4, const uint8_t * matrix)
{
	int i, j;
	const int last = matrix[scan_tables[63]];

	for (j = 63; j > 0 && matrix[scan_tables[j - 1]] == last; j--);

	for (i = 0; i <= j; i++) {
		BitstreamPutBits(ptMP4, matrix[scan_tables[i]], 8);
	}

	if (j < 63) {
		BitstreamPutBits(ptMP4, 0, 8);
	}
}

/*****************************************************************************
 * Functions
 ****************************************************************************/
 
/*
decode headers
returns coding_type, or -1 if error
*/

#define VIDOBJ_START_CODE_MASK		0x0000001f
#define VIDOBJLAY_START_CODE_MASK	0x0000000f

//void BitstreamWriteShortHeader(MBParam * pParam,
void 
BitstreamWriteShortHeader(Encoder * pEnc,
						const FRAMEINFO * frame,
						int vop_coded)
{
	uint32_t a;
	volatile MP4_t * ptMP4 = pEnc->ptMP4;

	BitstreamPad(ptMP4);
	BitstreamPutBits(ptMP4, VIDO_SHORT_HEADER, 22);
	BitstreamPutBits(ptMP4, pEnc->u8Temporal_ref, 8);
	pEnc->u8Temporal_ref ++;
	WRITE_MARKER();

	for (a = 0; a < SOURCE_FMT_SZ; a ++) {
		if (pEnc->width == vop_width[a])
			break;
	}
	BitstreamPutBits(ptMP4, a + 1, 7);			//  source_format

//	BitstreamPutBits(3, 7);
//	BitstreamPutBit(0);			/// zero_bit
//	BitstreamPutBit(0);			//  split_screen_indicator
//	BitstreamPutBit(0);			//  document_camera_indicator
//	BitstreamPutBit(0);			//  full_picture_freeze_release
//	BitstreamPutBits(2, 3);			//  source_format

	BitstreamPutBit(ptMP4, vop_coded);  			//  picture_coding_type
	BitstreamPutBits(ptMP4, 0, 4);				//  four_reserved_zero_bits
	BitstreamPutBits(ptMP4, frame->quant, 5);	// quantizer
	BitstreamPutBits(ptMP4, 0, 2);
	//BitstreamPutBit(ptMP4, 0);				//  zero_bit
	//BitstreamPutBit(ptMP4, 0);				//  pei

}


/*
	write vol header
*/
void 
BitstreamWriteVolHeader(const Encoder * pEnc,
						const FRAMEINFO * frame)
{
	volatile MP4_t * ptMP4 = pEnc->ptMP4;
//	unsigned int base, incr;
	//unsigned int max_log;
  #ifdef GENERATE_VISUAL_OBJECT_SEQUENCE_SYNTAX
    BitstreamPad(ptMP4);

  	/* Write the VOS header */    
	//BitstreamPutBits(ptMP4, VISOBJSEQ_START_CODE, 32);
	BitstreamPutBits(ptMP4, VUDOBJ_CODE, 24);
	BitstreamPutBits(ptMP4, VISOBJSEQ_START_CODE, 8);
	BitstreamPutBits(ptMP4, 0x03, 8);/* fix it to simple profile level 3 */ 	/* profile_and_level_indication */

   	/* visual_object_start_code */
	BitstreamPad(ptMP4);
	//BitstreamPutBits(ptMP4, VISOBJ_START_CODE, 32);
	BitstreamPutBits(ptMP4, VUDOBJ_CODE, 24);
	BitstreamPutBits(ptMP4, VISOBJ_START_CODE, 8);
	BitstreamPutBits(ptMP4, 0, 1);		/* is_visual_object_identifier */

	/* Video type */
	BitstreamPutBits(ptMP4, VISOBJ_TYPE_VIDEO, 4);		/* visual_object_type */
	BitstreamPutBit(ptMP4, 0); /* video_signal_type */

    /* video object_start_code & vo_id */
	BitstreamPadAlways(ptMP4); /* next_start_code() */
	BitstreamPutBits(ptMP4, VO_START_CODE, 27);
	BitstreamPutBits(ptMP4, 0, 5);

  #else  // #ifdef GENERATE_VISUAL_OBJECT_SEQUENCE_SYNTAX

	// video object_start_code & vo_id
	BitstreamPad(ptMP4);
	BitstreamPutBits(ptMP4, VO_START_CODE, 27);
	BitstreamPutBits(ptMP4, 0, 5);
	
  #endif // #ifdef GENERATE_VISUAL_OBJECT_SEQUENCE_SYNTAX

	// video_object_layer_start_code & vol_id
	//BitstreamPutBits(ptMP4, VOL_START_CODE, 28);
	BitstreamPutBits(ptMP4, VUDOBJ_CODE, 24);
	BitstreamPutBits(ptMP4, 2, 4);
	
#if 1 //ivan for simple object type
	BitstreamPutBits(ptMP4, 2, 14);
    //BitstreamPutBits(ptMP4, 0, 4);
    //BitstreamPutBits(ptMP4, 0, 1); 
    //BitstreamPutBits(ptMP4, 1, 8); //object type is simple object
    //BitstreamPutBits(ptMP4, 0, 1);
#else
	BitstreamPutBits(ptMP4, 0, 14);
#endif
//	BitstreamPutBits(bs, 0, 4);
//	BitstreamPutBit(bs, 0);		// random_accessible_vol
//	BitstreamPutBits(bs, 0, 8);	// video_object_type_indication
//	BitstreamPutBit(bs, 0);		// is_object_layer_identified (0=not given)

// Tuba 20120316 start: force syntax equal to 8120
    if (!pEnc->forceSyntaxSameAs8120)
	    BitstreamPutBits(ptMP4, 216, 11);
    else
        BitstreamPutBits(ptMP4, 8, 7);
// Tuba 20120316 end
//	BitstreamPutBits(bs, 1, 4);	// aspect_ratio_info (1=1:1)
//	BitstreamPutBit(bs, 1);	// vol_control_parameters
//	BitstreamPutBits(bs, 1, 2);	// chroma_format 1="4:2:0"
//	BitstreamPutBit(bs, 1);	// low_delay
//	BitstreamPutBit(bs, 0);	// vbv_parameters (0=not given)
//	BitstreamPutBits(bs, 0, 2);	// video_object_layer_shape (0=rectangular)

	WRITE_MARKER();
	
	/*
	 * time_increment_resolution; ignored by current decore versions
	 *  eg. 2fps     base=2       inc=1
	 *      25fps    base=25      inc=1
	 *      29.97fps base=30000   inc=1001
	 */
	BitstreamPutBits(ptMP4, pEnc->fbase, 16);

	WRITE_MARKER();

// Tuba 20120316 start: force syntax equal to 8120
    if (!pEnc->forceSyntaxSameAs8120) {
    #if 0
    	BitstreamPutBit(ptMP4, 0);		// fixed_vop_rate = 0
    #else
    	BitstreamPutBit(ptMP4, 1);		// fixed_vop_rate = 1
    	
    	// The following line was the bug of xvid version 0.9 and was fixed
    	// on 07-15-2005.We should send the (pParam->fbase-1) value to 
    	// calculate the minimum number of bits representing the range.
        //	BitstreamPutBits(ptMP4, pEnc->fincr, MAX(log2bin(pEnc->fbase),1));	// fixed_vop_time_increment
    	BitstreamPutBits(ptMP4, pEnc->fincr, MAX(log2bin(pEnc->fbase-1),1));	// fixed_vop_time_increment
    #endif
    }
    else
        BitstreamPutBit(ptMP4, 0);		// fixed_vop_rate = 0
        
	WRITE_MARKER();
#ifndef ENABLE_VARIOUS_RESOLUTION
	BitstreamPutBits(ptMP4, pEnc->roi_width, 13);	// roi width
	WRITE_MARKER();
	BitstreamPutBits(ptMP4, pEnc->roi_height, 13);	// roi height
#else
    BitstreamPutBits(ptMP4, pEnc->roi_width-pEnc->cropping_width, 13);	// roi width
	WRITE_MARKER();
    BitstreamPutBits(ptMP4, pEnc->roi_height-pEnc->cropping_height, 13);    // Tuba 20110614_0: add crop height to encode 1080p
#endif
	WRITE_MARKER();

#if 0	
#if 1
	BitstreamPutBit(ptMP4, 0);	// interlace
#else
	BitstreamPutBit(ptMP4, frame->global_flags & Gm_INTERLACING);	// interlace
#endif

	BitstreamPutBits(ptMP4, 4, 3);
//	BitstreamPutBit(bs, 1);		// obmc_disable (overlapped block motion compensation)
//	BitstreamPutBit(bs, 0);		// sprite_enable
//	BitstreamPutBit(bs, 0);		// not_in_bit
#else
	BitstreamPutBits(ptMP4, 4, 4);
#endif

	// bMp4_quant   0=h.263  1=mpeg4(quantizer tables)
	BitstreamPutBit(ptMP4, pEnc->bMp4_quant);

	if (pEnc->bMp4_quant) {
		BitstreamPutBit(ptMP4, pEnc->custom_intra_matrix);	// load_intra_quant_mat
		if (pEnc->custom_intra_matrix) {
			bs_put_matrix(ptMP4, pEnc->u8intra_matrix);
		}

		BitstreamPutBit(ptMP4, pEnc->custom_inter_matrix);	// load_inter_quant_mat
		if (pEnc->custom_inter_matrix) {
			bs_put_matrix(ptMP4, pEnc->u8inter_matrix);
		}
	}

	if (pEnc->bResyncMarker)
		BitstreamPutBits(ptMP4, 8, 4);
	else
		BitstreamPutBits(ptMP4, 12, 4);

//	BitstreamPutBit(bs, 1);		// complexity_estimation_disable
//	BitstreamPutBit(bs, 1);		// resync_marker_disable
//	BitstreamPutBit(bs, 0);		// data_partitioned
//	BitstreamPutBit(bs, 0);		// scalability
}



/*
  write vop header

  NOTE: doesnt handle bother with time_base & time_inc
  time_base = n seconds since last resync (eg. last iframe)
  time_inc = nth of a second since last resync
  (decoder uses these values to determine precise time since last resync)
*/
void 
BitstreamWriteVopHeader(const Encoder * pEnc,
						const FRAMEINFO * frame,
						int vop_coded)
{
	volatile MP4_t * ptMP4 = pEnc->ptMP4;

	BitstreamPad(ptMP4);
	BitstreamPutBits(ptMP4, VOP_START_CODE0, 16);
	BitstreamPutBits(ptMP4, VOP_START_CODE1, 16);

	BitstreamPutBits(ptMP4, frame->coding_type, 2);

	// time_base = 0  write n x PutBit(1), PutBit(0)
    if(pEnc->module_time_base==-1)
    {
    	BitstreamPutBits(ptMP4, (1 << (frame->seconds + 1)) - 2, frame->seconds + 1);
//    	printk("frame->seconds=%d ticks=%d\n",frame->seconds,frame->ticks);
    }
    else
    {
        BitstreamPutBits(ptMP4, (1 << (pEnc->module_time_base + 1)) - 2, pEnc->module_time_base + 1);
//        printk("pEnc->module_time_base=%d\n",pEnc->module_time_base);
    }

	WRITE_MARKER();

	// time_increment: value=nth_of_sec, nbits = log2(resolution)
	//BitstreamPutBits(ptMP4, frame->ticks, log2bin(pEnc->fbase));
//	BitstreamPutBits(ptMP4, frame->ticks, log2bin(pEnc->fbase-1));
//	BitstreamPutBits(ptMP4, frame->ticks, MAX(log2bin(pEnc->fbase-1), 1));
	BitstreamPutBits(ptMP4, frame->ticks, MAX(log2bin(pEnc->fbase), 1));
	
	WRITE_MARKER();

	if (!vop_coded) {
		BitstreamPutBit(ptMP4, 0);
		return;
	}

	BitstreamPutBit(ptMP4, 1);		// vop_coded

	if (frame->coding_type == P_VOP)
		BitstreamPutBit(ptMP4, frame->bRounding_type);
	
	
	//BitstreamPutBits(ptMP4, 0, 3);	// intra_dc_vlc_threshold
	//BitstreamPutBits(ptMP4, frame->quant, 5);	// quantizer
	BitstreamPutBits(ptMP4, frame->quant, 8);

	if (frame->coding_type != I_VOP)
		BitstreamPutBits(ptMP4, FCODE, 3);	// forward_fixed_code
#if 0
	if (frame->coding_type == B_VOP)
		BitstreamPutBits(ptMP4, frame->bcode, 3);	// backward_fixed_code
#endif

}
