#include "header.h"
#include "bitstream.h"
#include "decoder_vg.h"

#ifdef PARSING_RESOLUTION
static int __inline log2bin(unsigned int value)
{
    int n = 0;

    while (value) {
        value >>= 1;
        n++;
	}
    return n;
}

static int skip_marker(Bitstream *stream)
{
    int length = read_bits(stream, 16) - 2;

    if (length > 0) {
        while (length--) {
            skip_bits(stream, 8);
            if (stream->cur_byte > (stream->streamBuffer + stream->totalLength))
                return -1;
        }
    }
    return 0;
}

static int parsing_jpeg_header(Bitstream *stream, HeaderInfo *hdr)
{
    int marker;

    for (;;) {
        do {
            marker = read_bits(stream, 8);
            if (stream->cur_byte > (stream->streamBuffer + stream->totalLength))
                return -1;
        } while (marker == 0xFF);
        switch (marker) {
        case SOF0:
        case SOF1:
            // get jpeg info
            skip_bits(stream, 16);  // frame header length
            skip_bits(stream, 8);   // sample precision
            hdr->height = read_bits(stream, 16);
            hdr->width = read_bits(stream, 16);
            hdr->resolution_exist = 1;
            return 0;
        default:
            skip_marker(stream);
            break;
        }
        if (stream->cur_byte > (stream->streamBuffer + stream->totalLength))
            return -1;
    }
    return 0;
}

static int parsing_mpeg4_header(Bitstream *stream, HeaderInfo *hdr)
{
    unsigned int start_code;
    int visual_object_verid;
    
    hdr->resolution_exist = 0;
    do {
        start_code = show_bits(stream, 32);
        if (start_code == VISOBJSEQ_START_CODE) {
            int profile;
            skip_bits(stream, 32);          // visual_object_sequence_start_code
            profile = read_bits(stream, 8); // profile_and_level_indication
        }
        else if (start_code == VISOBJSEQ_STOP_CODE) {
            skip_bits(stream, 32);
        }
        else if (start_code == VISOBJ_START_CODE) {
            skip_bits(stream, 32);
            if (read_bits(stream, 1)) {
                visual_object_verid = read_bits(stream, 4);
                skip_bits(stream, 3);
            }
            if (read_bits(stream, 4) != VISOBJ_TYPE_VIDEO)
                return -1;
            if (read_bits(stream, 1)) {         /* video_signal_type */
                skip_bits(stream, 3);           /* video_format */
                skip_bits(stream, 1);           /* video_range */
                if (read_bits(stream, 1)) {     /* color_description */
                    skip_bits(stream, 8);       /* color_primaries */
                    skip_bits(stream, 8);       /* transfer_characteristics */
                    skip_bits(stream, 8);       /* matrix_coefficients */
                }
            }
        }
        else if ((start_code & ~VIDOBJ_START_CODE_MASK) == VIDOBJ_START_CODE) {
            skip_bits(stream, 32);
        }
        else if ((start_code & ~VIDOBJLAY_START_CODE_MASK) == VIDOBJLAY_START_CODE) {
            int video_object_type;
            int video_object_layer_verid;
            int video_object_layer_shape;
            int vop_time_increment_resolution;

            skip_bits(stream, 32);                      /* video_object_layer_start_code */
			skip_bits(stream, 1);                       /* random_accessible_vol */
            video_object_type = read_bits(stream, 8);   /* video_object_type_indication */
            if (read_bits(stream, 1)) {                 /* is_object_layer_identifier */
                video_object_layer_verid = read_bits(stream, 4);
                skip_bits(stream, 3);
            }
            else 
                video_object_layer_verid = 1;
            if (read_bits(stream, 4) == VIDOBJLAY_AR_EXTPAR) {  /* aspect_ratio_info */
                skip_bits(stream, 8);                   /* par_width */
                skip_bits(stream, 8);                   /* par_height */
            }
            if (read_bits(stream, 1)) {                 /* vol_control_parameters */
                skip_bits(stream, 2);                   /* chroma_format */
                skip_bits(stream, 1);                   /* low_delay */
                if (read_bits(stream, 1)) {             /* vbv_parameters */
                    skip_bits(stream, 15);              /* first_half_bit_rate */
                    skip_bits(stream, 1);               /* marker_bit */
                    skip_bits(stream, 15);              /* latter_half_bit_rate */
                    skip_bits(stream, 1);               /* marker bit */
                    skip_bits(stream, 15);              /* first_half_vbv_buffer_size */
                    skip_bits(stream, 1);               /* marker bit */
                    skip_bits(stream, 3);               /* latter_half_vbv_buffer_size */
                    skip_bits(stream, 11);              /* first _half_vbv_occupancy */
                    skip_bits(stream, 1);               /* marker bit */
                    skip_bits(stream, 15);              /* latter_half_vbv_occupancy */
                    skip_bits(stream, 1);               /* marker bit */
                }
            }
            video_object_layer_shape = read_bits(stream, 2);
            if (video_object_layer_shape == VIDOBJLAY_SHAPE_GRAYSCALE && video_object_layer_verid != 0x01)
                skip_bits(stream, 4);                   /* video_object_layer_shape_extension */
            skip_bits(stream, 1);                       /* marker bit */
            vop_time_increment_resolution = read_bits(stream, 16);  /* vop_time_increment_resolution */
            skip_bits(stream, 1);                       /* marker bit */
            if (read_bits(stream, 1))                   /* fixed_vop_rate */
                skip_bits(stream, iMax(log2bin(vop_time_increment_resolution), 1)); /* fixed_vop_time_increment */
            if (video_object_layer_shape != VIDOBJLAY_SHAPE_BINARY_ONLY) {
                if (video_object_layer_shape == VIDOBJLAY_SHAPE_RECTANGULAR) {
                    skip_bits(stream, 1);
                    hdr->width = read_bits(stream, 13);
                    skip_bits(stream, 1);
                    hdr->height = read_bits(stream, 13);
                    //skip_bits(stream, 1);
                    hdr->resolution_exist = 1;
                    return 1;
                }
            }

        }
        else if (start_code == GRPOFVOP_START_CODE) {
            skip_bits(stream, 32);          /* group_of_vop_start_code */ 
            skip_bits(stream, 18);          /* time_code */
            skip_bits(stream, 1);           /* closed_gov */
			skip_bits(stream, 1);	        /* broken_link */
        }
        else if (start_code == VOP_START_CODE) {    // VideoObjectPlane
            return 0;
        }
        else if (start_code == USERDATA_START_CODE) {
            skip_bits(stream, 32);
        }
        if (stream->cur_byte > (stream->streamBuffer + stream->totalLength))
            return -1;
    } while (1);

    return 0;
}

static int parsing_h264_header(Bitstream *stream, HeaderInfo *header)
{
    unsigned int tmp, nal_type, profile, idc, num;
    unsigned int mb_width, mb_height, frame_mbs_only_flag;
    unsigned int i;

    header->resolution_exist = 0;
    while (1) {
        tmp = show_bits(stream, 32);
        while (tmp != 0x00000001 && (tmp>>8) != 0x000001) {
            read_bits(stream, 8);
            tmp = show_bits(stream, 32);
        }
        tmp = read_bits(stream, 32);
        if (tmp == 0x00000001) {
            tmp = read_bits(stream, 8);
            nal_type = tmp&0x1F;
        }
        else 
            nal_type = tmp&0x1F;

        switch (nal_type) {
        case NALU_TYPE_SLICE:
        case NALU_TYPE_IDR:
            return 0;
            break;
        case NALU_TYPE_SEI:
            break;
        case NALU_TYPE_SPS:
            // read SPS
            profile = read_bits(stream, 8);
            read_bits(stream, 8);           /* cs_flag */
            read_bits(stream, 8);           /* level_idc */
            read_ue(stream);                /* seq_parameter_set_id */

            if (profile == FREXT_HP || profile == FREXT_Hi10P 
                || profile == FREXT_Hi422 || profile == FREXT_Hi444){
                if (read_ue(stream) == 3)   /* chroma_format_idc */
                    read_bits(stream, 1);   /* residual_colour_transform_flag */
                read_ue(stream);            /* bit_depth_luma_minus8 */
		        read_ue(stream);            /* bit_depth_chroma_minus8 */
		        read_bits(stream, 1);       /* qpprime_y_zero_transform_bypass_flag */
		
                if (read_bits(stream, 1)) { /* seq_scaling_matrix_present_flag */
                    // scaling matrix
                }
            }
            read_ue(stream);                /* log2_max_frame_num_minus4 */
            idc = read_ue(stream);          /* pic_order_cnt_type */
            if (idc == 0)
                read_ue(stream);            /* log2_max_pic_order_cnt_lsb_minus4 */
            else if (idc == 1) {
                read_bits(stream, 1);       /* delta_pic_order_always_zero_flag */
                read_se(stream);            /* offset_for_non_ref_pic */
                read_se(stream);            /* offset_for_top_to_bottom_field */
                num = read_ue(stream);      /* num_ref_frames_in_pic_order_cnt_cycle */
                for (i = 0; i<num; i++)
                    read_se(stream);        /* offset_for_ref_frame */
            }
            read_ue(stream);                /* num_ref_frames */
            read_bits(stream, 1);           /* gaps_in_frame_num_value_allowed_flag */
            mb_width = read_ue(stream) + 1; /* pic_width_in_mbs_minus1 */
            mb_height = read_ue(stream) + 1;    /*pic_height_in_map_units_minus1 */
            frame_mbs_only_flag = read_bits(stream, 1); /* frame_mbs_only_flag */
            header->width = mb_width*16;
            header->height = (2 - frame_mbs_only_flag)*mb_height*16;
            header->resolution_exist = 1;
            return 1;
            break;
        case NALU_TYPE_PPS:
            break;
        case NALU_TYPE_EOSEQ:
            break;
        case NALU_TYPE_EOSTREAM:
            break;
        default:
            break;
        }
        if (stream->cur_byte > (stream->streamBuffer + stream->totalLength))
            return -1;
    }

    return 0;
}
#endif
int parsing_header(Bitstream *stream, HeaderInfo *header)
{
    unsigned int tmp;
    int i, len;

    byte_align(stream);
    len = iMin(MAX_PARSING_BYTE, stream->totalLength);
    tmp = show_bits(stream, 32);
    for (i = 0; i<MAX_PARSING_BYTE; i++) {
        if (((tmp>>16)&0xFFFF) == 0xFFD8) {     // JPEG SOI
            read_bits(stream, 16);
        #ifdef PARSING_RESOLUTION
            if (parsing_jpeg_header(stream, header) < 0)
                return -1;
        #endif
            return TYPE_JPEG;
        }
        if (tmp == 0x00000001) {             // H264 start code
        #ifdef PARSING_RESOLUTION
            if (parsing_h264_header(stream, header) < 0)
                return -1;
        #endif
            return TYPE_H264;
        }
        if ((tmp>>8) == 0x000001) {         // MPEG4 start code or H264 short code
            if ((tmp & ~VIDOBJ_START_CODE_MASK) == VIDOBJ_START_CODE ||
                tmp == VIDOBJLAY_START_CODE || tmp == VOP_START_CODE) {
            #ifdef PARSING_RESOLUTION
                if (parsing_mpeg4_header(stream, header) < 0)
                    return -1;
            #endif
                return TYPE_MPEG4;
            }
            else {
            #ifdef PARSING_RESOLUTION
                if (parsing_h264_header(stream, header) < 0)
                    return -1;
            #endif
                return TYPE_H264;
            }
        }
        read_bits(stream, 8);
        tmp = show_bits(stream, 32);
    }

    return -1;
}

