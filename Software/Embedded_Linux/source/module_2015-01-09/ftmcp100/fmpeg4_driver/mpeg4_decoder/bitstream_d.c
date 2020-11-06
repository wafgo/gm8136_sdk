#ifdef LINUX
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include "bitstream_d.h"
#include "../common/zigzag.h"
#include "../common/quant_matrix.h"
#include "local_mem_d.h"
#include "../common/define.h"
#include "../common/vpe_m.h"
#else
#include "bitstream_d.h"
#include "zigzag.h"
#include "quant_matrix.h"
#include "local_mem_d.h"
#include "define.h"
#include "vpe_m.h"
#endif

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
#define VIDOBJ_START_CODE_MASK		0x0000001f
#define VIDOBJLAY_START_CODE_MASK	0x0000000f
#define VIDO_SHORT_HEADER_MASK 0x000003ff
#define VIDO_SHORT_HEADER 0x8000
#define VIDO_SHORT_HEADER_END 0x3f
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

/*****************************************************************************
 * Routines
 ****************************************************************************/
#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
static uint32_t __inline
log2bin(uint32_t value)
{
/* Changed by Chenm001 */
	int n = 0;

	while (value) {
		value >>= 1;
		n++;
	}
	return n;
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

/*****************************************************************************
 * Functions
 ****************************************************************************/
#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) || defined(TWO_P_INT) )
bool 
bRead_video_packet_header(DECODER * dec,
//						int *fcode_forward,
//						int *fcode_backward,
						int32_t * mbnum)
{
	int tmp;
	int coding_type;
	uint32_t * table = (uint32_t *)(Mp4Base (dec) + TABLE_OUTPUT_OFF);

	tmp = table[0];
	// macroblock_number
	if (dec->bH263) {
		*mbnum = ((tmp  & 0xFFFF) >> 2) * dec->mb_width;
	} else {
		*mbnum = tmp  & 0xFFFF;
		//extent bit
		if (tmp & BIT24) {
			tmp = table[2];
			coding_type = tmp & 0xff;
			// hardware not support
			if (coding_type >= B_VOP) {
				#ifdef LINUX
				printk("not support B_VOP\n");
				#endif
				return -1;
				// ben mark @ 2005 1206 start
				// no need to save fcode_forward
				#if 0
				if (coding_type != I_VOP && *fcode_forward)
					*fcode_forward = (tmp >> 16) & 0xff;
				if (coding_type == B_VOP && *fcode_backward)
					*fcode_backward = 0;
				#endif
				// ben mark @ 2005 1206 stop
			}
		}
	}
	return 0;
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) || defined(TWO_P_INT) )



/*
decode headers
returns coding_type, or -1 if error
*/
#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
void 
bs_get_matrix(Bitstream * bs,
			  uint8_t * matrix)
{
	int i = 0;
	int last, value = 0;
	do {
		last = value;
		value = BitstreamGetBits(bs, 8);
		matrix[scan_tables[i++]] = value;
	}
	while (value != 0 && i < 64);
    i--;    /* fix little bug at coeff not full */

	while (i < 64) {
		matrix[scan_tables[i++]] = last;
	}
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )


#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
int 
BitstreamReadHeaders(Bitstream * bs,
					 DECODER_ext * dec,
					 uint32_t * rounding,
					 uint32_t * quant,
					 uint32_t * fcode_forward,
					 uint32_t * fcode_backward,
					 uint32_t * intra_dc_threshold_bit)
{
	uint32_t vol_ver_id;
	uint32_t coding_type;
	uint32_t start_code;
	uint32_t time_incr = 0;
	int32_t time_increment = 0;
	do {
		BitstreamByteAlign(bs);
		start_code = BitstreamShowBits(bs, 32);
    #ifndef FPGA
		mVpe_Indicator(0x93000000 | (start_code >> 16));  // 20130220 Tuba: remove
		mVpe_Indicator(0x93000000 | (start_code & 0xFFFF));   // 20130220 Tuba: remove
    #endif
		if (start_code == VISOBJSEQ_START_CODE) {

			int profile;
			BitstreamSkip(bs, 32);	/* visual_object_sequence_start_code */
			profile = BitstreamGetBits(bs, 8);	/* profile_and_level_indication */
        #ifdef LINUX
            if((profile==1)||(profile==2)||(profile==3)||
               (profile==0xf0)||(profile==0xf1)||(profile==0xf2)||
               (profile==0xf3)||(profile==0xf4)||(profile==0xf5))
                ;//simple profile or advanced simple profile
			else
			    printk("Fail Support MPEG4 format: profile=0x%x (simple profile only)!\n",profile);
        #endif
		} else if (start_code == VISOBJSEQ_STOP_CODE) {
			BitstreamSkip(bs, 32);	/* visual_object_sequence_stop_code */
		} else if (start_code == VISOBJ_START_CODE) {
			BitstreamSkip(bs, 32);	/* visual_object_start_code */
			if (BitstreamGetBit(bs))	/* is_visual_object_identified */
			{
				vol_ver_id = BitstreamGetBits(bs, 4);	/* visual_object_ver_id */
				BitstreamSkip(bs, 3);	/* visual_object_priority */
			} else {
				vol_ver_id = 1;
			}

			if (BitstreamShowBits(bs, 4) != VISOBJ_TYPE_VIDEO)	/* visual_object_type */
			{
				return -1;
			}
			BitstreamSkip(bs, 4);

			/* video_signal_type */

			if (BitstreamGetBit(bs))	/* video_signal_type */
			{
				BitstreamSkip(bs, 3);	/* video_format */
				BitstreamSkip(bs, 1);	/* video_range */
				if (BitstreamGetBit(bs))	/* color_description */
				{
					BitstreamSkip(bs, 8);	/* color_primaries */
					BitstreamSkip(bs, 8);	/* transfer_characteristics */
					BitstreamSkip(bs, 8);	/* matrix_coefficients */
				}
			}
		} else if ((start_code & ~VIDOBJ_START_CODE_MASK) == VIDOBJ_START_CODE) {
			BitstreamSkip(bs, 32);	/* video_object_start_code */
		} else if ((start_code & ~VIDO_SHORT_HEADER_MASK) == VIDO_SHORT_HEADER) {
			int pei, psupp = 0;
			uint32_t source_format;
			uint32_t width, height;		// ben add

			dec->bH263 = 1;
			// set default quantization method: H263
			dec->bMp4_quant = H263_QUANT;
			*fcode_forward = 1;
			// dec->quarterpel = 0;
			BitstreamSkip(bs, 22);
			BitstreamSkip(bs, 8);
			READ_MARKER();
#if 1
			ZERO_BIT ();						// zero_bit
			// 1. split_screen_indicator
			// 2. document_camera_indicator
			// 3. full_picture_freeze_release
			BitstreamSkip (bs, 3);
			source_format = BitstreamGetBits(bs,3) - 1;			//  source_format
			if (source_format >= SOURCE_FMT_SZ)
				return -1;
			coding_type = BitstreamGetBit(bs);					//  picture_coding_type
			// four_reserved_zero_bits
			BitstreamSkip (bs, 4);
			*quant = BitstreamGetBits(bs,5);		// quant
			ZERO_BIT ();						// zero_bit
#else
			temp = BitstreamGetBit(bs);							//  zero_bit
			dec->split_screen_indicator = BitstreamGetBit(bs);		//  split_screen_indicator
			dec->document_camera_indicator = BitstreamGetBit(bs);	//  document_camera_indicator
			dec->full_picture_freeze_release = BitstreamGetBit(bs);	//  full_picture_freeze_release
			dec->source_format = BitstreamGetBits(bs,3);			//  source_format
			coding_type = BitstreamGetBit(bs);					//  picture_coding_type
			temp = BitstreamGetBits(bs,4);						//  four_reserved_zero_bits
			*quant = BitstreamGetBits(bs,5);						// quant
			temp = BitstreamGetBit(bs);							// zero_bit
#endif
			do {
				pei = BitstreamGetBit(bs);
				if (pei == 1)
					psupp=BitstreamGetBits(bs,8);
			} while (pei == 1);

			width = vop_width[source_format];
			height = vop_height[source_format];
			//ben modify, start
			/* for auto set width & height */
			if (dec->width == 0)	
				dec->width = width;
			if (dec->height == 0)
				dec->height = height;
			if (width != dec->width || height != dec->height) {
				#ifdef LINUX
				printk("auto set width and height fail on short header\n");
				#endif
				return -1;
			}
			//ben modify, stop
			return coding_type;
		}
		else if ((start_code & ~VIDOBJLAY_START_CODE_MASK) == VIDOBJLAY_START_CODE) {
			BitstreamSkip(bs, 32);	/* video_object_layer_start_code */
			BitstreamSkip(bs, 1);	/* random_accessible_vol */

			dec->bH263 = 0;
			/* video_object_type_indication */
			if (BitstreamShowBits(bs, 8) != VIDOBJLAY_TYPE_SIMPLE && BitstreamShowBits(bs, 8) != VIDOBJLAY_TYPE_CORE && BitstreamShowBits(bs, 8) != VIDOBJLAY_TYPE_MAIN && BitstreamShowBits(bs, 8) != 0)	/* BUGGY DIVX */
			{
				#ifdef LINUX
				printk("BUGGY DIVX fail\n");
				#endif
				return -1;
			}
			BitstreamSkip(bs, 8);


			if (BitstreamGetBit(bs))	/* is_object_layer_identifier */
			{
				vol_ver_id = BitstreamGetBits(bs, 4);	/* video_object_layer_verid */
				BitstreamSkip(bs, 3);	/* video_object_layer_priority */
			} else {
				vol_ver_id = 1;
			}

			if (BitstreamGetBits(bs, 4) == VIDOBJLAY_AR_EXTPAR)	/* aspect_ratio_info */
			{
				BitstreamSkip(bs, 8);	/* par_width */
				BitstreamSkip(bs, 8);	/* par_height */
			}

			if (BitstreamGetBit(bs))	/* vol_control_parameters */
			{
				#if 1
				// 1. chroma_format: 2 bit
				// 2. low_delay: 1 bit
				BitstreamSkip(bs, 3);
				#else
				BitstreamSkip(bs, 2);	/* chroma_format */
				dec->low_delay = (uint8_t)BitstreamGetBit(bs);	/* low_delay */
				#endif
				if (BitstreamGetBit(bs))	/* vbv_parameters */
				{
					BitstreamSkip(bs, 15);	/* first_half_bitrate */
					READ_MARKER();
					BitstreamSkip(bs, 15);	/* latter_half_bitrate */
					READ_MARKER();
					BitstreamSkip(bs, 15);	/* first_half_vbv_buffer_size */
					READ_MARKER();
					BitstreamSkip(bs, 3);	/* latter_half_vbv_buffer_size */
					BitstreamSkip(bs, 11);	/* first_half_vbv_occupancy */
					READ_MARKER();
					BitstreamSkip(bs, 15);	/* latter_half_vbv_occupancy */
					READ_MARKER();

				}
			}

			dec->shape = BitstreamGetBits(bs, 2);	/* video_object_layer_shape */

			if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE && vol_ver_id != 1) {
				BitstreamSkip(bs, 4);	/* video_object_layer_shape_extension */
			}

			READ_MARKER();

/* *************************** for decode B-frame time *********************** */
			dec->time_increment_resolution = BitstreamGetBits(bs, 16);	/* vop_time_increment_resolution */

/*			dec->time_increment_resolution--; */

			if (dec->time_increment_resolution > 0) {
				dec->time_inc_bits = MAX (log2bin(dec->time_increment_resolution-1), 1);
			} else {
				/* dec->time_inc_bits = 0; */
				/* for "old" xvid compatibility, set time_inc_bits = 1 */
				dec->time_inc_bits = 1;
			}

			READ_MARKER();

			if (BitstreamGetBit(bs))	/* fixed_vop_rate */
			{
				BitstreamSkip(bs, dec->time_inc_bits);	/* fixed_vop_time_increment */
			}

			if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY) {

				if (dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR) {
					uint32_t width, height;

					READ_MARKER();
					width = BitstreamGetBits(bs, 13);	/* video_object_layer_width */
					READ_MARKER();
					height = BitstreamGetBits(bs, 13);	/* video_object_layer_height */
					READ_MARKER();
                    // Tuba 20140306: align to 16
                    width = ((width+15)>>4)<<4;
                    height = ((height+15)>>4)<<4;
//ben modify, start
					/* for auto set width & height */
					if (dec->width == 0)	
						dec->width = width;
					if (dec->height == 0)
						dec->height = height;
					if (width != dec->width || height != dec->height){
						#ifdef LINUX
						printk("auto set width and height fail \n");
						#endif
						return -1;
					}
//ben modify, stop
				}

				dec->interlacing = BitstreamGetBit(bs);
				if (dec->interlacing != 0) {
					#ifdef LINUX
					printk("interlacing not support fail \n");
					#endif
					return -1; 		// not support interlacing now
				}

				if (!BitstreamGetBit(bs))	/* obmc_disable */
				{
					/* TODO */
					/* fucking divx4.02 has this enabled */
				}

				if (BitstreamGetBits(bs, (vol_ver_id == 1 ? 1 : 2)))	/* sprite_enable */
				{	
					#ifdef LINUX
					printk("sprite_enable support fail \n");
					#endif
					return -1;
				}

				if (vol_ver_id != 1 &&
					dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR) {
					BitstreamSkip(bs, 1);	/* sadct_disable */
				}

				if (BitstreamGetBit(bs))	/* not_8_bit */
				{
					dec->quant_bits = BitstreamGetBits(bs, 4);	/* quant_precision */
					BitstreamSkip(bs, 4);	/* bits_per_pixel */
				} else {
					dec->quant_bits = 5;
				}

				if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE) {
					BitstreamSkip(bs, 1);	/* no_gray_quant_update */
					BitstreamSkip(bs, 1);	/* composition_method */
					BitstreamSkip(bs, 1);	/* linear_composition */
				}

				dec->bMp4_quant = BitstreamGetBit(bs);	/* bMp4_quant */

				if (dec->bMp4_quant) {
					if (BitstreamGetBit(bs))	/* load_intra_quant_mat */
					{
						uint8_t matrix[64];
						bs_get_matrix(bs, matrix);
						init_quant_matrix (matrix,
							dec->u8intra_matrix,
							dec->u32intra_matrix_fix1);
					} else {
//printk("<2,default=0x%x dec->u32intra_matrix_fix1=0x%x(dec=0x%x,&dec=0x%x)>\n",get_default_intra_matrix(),dec->u32intra_matrix_fix1,dec,&dec);

						init_quant_matrix (get_default_intra_matrix(),
							dec->u8intra_matrix,
							dec->u32intra_matrix_fix1);
					}
					if (BitstreamGetBit(bs))	/* load_inter_quant_mat */
					{
						uint8_t matrix[64];
						
						bs_get_matrix(bs, matrix);
						init_quant_matrix (matrix,
							dec->u8inter_matrix,
							dec->u32inter_matrix_fix1);
					} else {
						init_quant_matrix (get_default_inter_matrix(),
							dec->u8inter_matrix,
							dec->u32inter_matrix_fix1);
					}
					if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE) {
						#ifdef LINUX
						printk("VIDOBJLAY_SHAPE_GRAYSCALE support fail \n");
						#endif
						return -1;
					}

				}


				if (vol_ver_id != 1)
					BitstreamSkip(bs, 1);	/* quarter_sample */
				if (!BitstreamGetBit(bs))	/* complexity_estimation_disable */
				{
					#ifdef LINUX
					printk("complexity_estimation_disable support fail \n");
					#endif
					return -1;
				}
				BitstreamSkip(bs, 1);	/* resync_marker_disable */
				dec->data_partitioned = BitstreamGetBits(bs,1);

				if (dec->data_partitioned)	/* data_partitioned */
				{
					dec->reversible_vlc = BitstreamGetBits(bs,1);
				}

				if (vol_ver_id != 1) {
					if (BitstreamGetBit(bs))	/* newpred_enable */
					{
						BitstreamSkip(bs, 2);	/* requested_upstream_message_type */
						BitstreamSkip(bs, 1);	/* newpred_segment_type */
					}
					if (BitstreamGetBit(bs))	/* reduced_resolution_vop_enable */
					{
						#ifdef LINUX
						printk("Fail Support MPEG4 format: Not Support reduced_resolution_vop_enable\n");
						#endif
						return -1;
					}
				}

				if ((dec->scalability = (int8_t)BitstreamGetBit(bs)) != 0)	/* scalability */
				{
					#ifdef LINUX
					printk("Fail Support MPEG4 format: Not Support scalability\n");
					#endif
					return -1;
				}
			} else				/* dec->shape == BINARY_ONLY */
			{
				if (vol_ver_id != 1) {
					if (BitstreamGetBit(bs))	/* scalability */
					{
						#ifdef LINUX
						printk("Fail Support MPEG4 format: Not Support scalability on BINARY_ONLY\n");
						#endif
						return -1;
					}
				}
				BitstreamSkip(bs, 1);	/* resync_marker_disable */
			}
		}
		else if (start_code == GRPOFVOP_START_CODE) {

			BitstreamSkip(bs, 32);
			{
				int hours, minutes, seconds;

				hours = BitstreamGetBits(bs, 5);
				minutes = BitstreamGetBits(bs, 6);
				READ_MARKER();
				seconds = BitstreamGetBits(bs, 6);
				
			}
			BitstreamSkip(bs, 1);	/* closed_gov */
			BitstreamSkip(bs, 1);	/* broken_link */

		} else if (start_code == VOP_START_CODE) {

			BitstreamSkip(bs, 32);	/* vop_start_code */
			coding_type = BitstreamGetBits(bs, 2);	/* vop_coding_type */
//			dec->picture_coding_type = coding_type;
//**********************NOTE*****************************************
//			coding_type = coding_type & 0x1;
			// only support I_VOP and P_VOP
			if (coding_type > P_VOP)
			{
				#ifdef LINUX
				printk("Fail Support MPEG4 format: Not Support B-VOP!\n");
				#endif
				return -1;
            }

/* *************************** for decode B-frame time *********************** */
			while (BitstreamGetBit(bs) != 0)	/* time_base */
				time_incr++;

			READ_MARKER();

			if (dec->time_inc_bits) {
				time_increment = (BitstreamGetBits(bs, dec->time_inc_bits));	/* vop_time_increment */
			} 

			if (coding_type != B_VOP) {
				dec->last_time_base = dec->time_base;
				dec->time_base += time_incr;
				dec->time = time_increment;

/*					dec->time_base * time_increment_resolution +
					time_increment;
*/
				if (dec->time_increment_resolution!= 0)
					dec->time_pp = (uint32_t) 
						(dec->time_increment_resolution + dec->time - dec->last_non_b_time) %
						dec->time_increment_resolution;
				dec->last_non_b_time = dec->time;
			} else {
				dec->time = time_increment; 
/*
					(dec->last_time_base +
					 time_incr) * dec->time_increment_resolution + time_increment; 
*/
				dec->time_bp = (uint32_t) 
					(dec->time_increment_resolution + dec->last_non_b_time - dec->time) %
					dec->time_increment_resolution;
			}

			READ_MARKER();

			if (!BitstreamGetBit(bs))	/* vop_coded */
			{
				return N_VOP;
			}

			/* fix a little bug by MinChen <chenm002@163.com> */
			if ((dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY) &&
				(coding_type == P_VOP)) {
				*rounding = BitstreamGetBit(bs);	/* rounding_type */
			}

			if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR) {
				uint32_t width, height;
				uint32_t horiz_mc_ref, vert_mc_ref;

				width = BitstreamGetBits(bs, 13);
				READ_MARKER();
				height = BitstreamGetBits(bs, 13);
				READ_MARKER();
				horiz_mc_ref = BitstreamGetBits(bs, 13);
				READ_MARKER();
				vert_mc_ref = BitstreamGetBits(bs, 13);
				READ_MARKER();

				BitstreamSkip(bs, 1);	/* change_conv_ratio_disable */
				if (BitstreamGetBit(bs))	/* vop_constant_alpha */
				{
					BitstreamSkip(bs, 8);	/* vop_constant_alpha_value */
				}
			}


			if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY) {
				/* intra_dc_vlc_threshold */
				* intra_dc_threshold_bit= BitstreamGetBits(bs, 3);
//				* intra_dc_threshold_bit= BitstreamShowBits(bs, 3);
//				*intra_dc_threshold = intra_dc_threshold_table[BitstreamGetBits(bs, 3)];

				if (dec->interlacing) {
					BitstreamSkip(bs, 2);	/* top_field_first, alternate_vertical_scan */
				}
			}

			if ((*quant = BitstreamGetBits(bs, dec->quant_bits)) < 1)	/* vop_quant */
				*quant = 1;

			if (coding_type != I_VOP) {
				*fcode_forward = BitstreamGetBits(bs, 3);	/* fcode_forward */

//**********************NOTE*****************************************
				if (*fcode_forward < 1) *fcode_forward = 1;
			}

			if (coding_type == B_VOP) {
				*fcode_backward = BitstreamGetBits(bs, 3);	/* fcode_backward */
			}

			if (!dec->scalability) {
				if ((dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR) &&
					(coding_type != I_VOP)) {
					BitstreamSkip(bs, 1);	/* vop_shape_coding_type */
				}
			}
			return coding_type;

		} else if (start_code == USERDATA_START_CODE) {
			BitstreamSkip(bs, 32);	/* user_data_start_code */
		} else					/* start_code == ? */
		{
			if (BitstreamShowBits(bs, 24) == 0x000001) {
			}
			BitstreamSkip(bs, 8);
		}
	}
	while ((BitstreamPos(bs) >> 3) < bs->length);

	//return -1;					/* ignore it */
	return 0;					/* ignore it */
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

