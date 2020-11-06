#ifndef _FMPEG4_ENC_ENTITY_H_
#define _FMPEG4_ENC_ENTITY_H_

#include "video_entity.h"
#ifdef REF_POOL_MANAGEMENT
    #ifdef SHARED_POOL
        #include "shared_pool.h"
    #else
        #include "mem_pool.h"
    #endif
#endif

#define MP4E_REINIT        0xFF
#define MP4E_RC_REINIT     0x01
#define MP4E_GOP_REINIT    0x02
//#define MP4E_ROI_REINIT    0x04
#define MP4E_FORCE_INTRA   0x10

#define SUPPORT_2FRAME

struct mp4e_data_t {
    int engine;
    int chn;

    //int max_width;
    //int max_height;
    int frame_width;
    int frame_height;
    unsigned int src_fmt;
        /* input format
         * 0: MP4 2D, 420p,
		 * 1: sequencial 1D 420p,
		 * 2: H264 2D, 420p
		 * 3: sequential 1D 420, one case of u82D=1, (only support when DMAWRP is configured)
		 * 4: sequential 1D 422, (only support when DMAWRP is configured)   */
    unsigned int input_format;
    unsigned int src_xy;
    unsigned int src_bg_dim;
    unsigned int src_dim;

    unsigned int src_interlace;
    unsigned int di_result;


    unsigned int idr_period;

    int short_header;
        /* 0: disable short header.
         * 1: enable short header.  */
    int enable_4mv;
        /* 0: disable 4MV mode and use 1MV (1 motion vector) mode instead.
         * 1: enable 4MV mode and select 4MV mode.  */
    int h263_quant;
        /* 0: select MPEG4 quantization method.
         * 1: select H.263 quantization method.
         * 2: select MPEG4 user defined quant tbl "quant.txt"   */
    int resync_marker;
        /* 0: disable resync marker.
         * 1: enable resync marker. */
    int ac_disable;

    // rate control
    unsigned int rc_mode;
    unsigned int src_frame_rate;
    unsigned int fps_ratio;
    unsigned int init_quant;
    unsigned int max_quant;
    unsigned int min_quant;
    unsigned int bitrate;
    unsigned int max_bitrate;

    int motion_enable;

    int updated;
    //int reinit;
    void *rc_handler;
    void *handler;
    int stop_job;

#ifdef REF_POOL_MANAGEMENT
    int res_pool_type;
    int re_reg_cnt;
    int over_spec_cnt;

    struct ref_buffer_info_t ref_buffer[2];
    struct ref_buffer_info_t *refer_buf;
    struct ref_buffer_info_t *recon_buf;
#endif

    int quant;
    unsigned int U_off;
    unsigned int V_off;
    unsigned int mbinfo_length;

    unsigned int bitstream_size; // output
    unsigned int bitstream_offset;
    unsigned int slice_type;

    unsigned int in_addr_va;
    unsigned int in_addr_pa;
    unsigned int in_size;
    unsigned int out_addr_va;
    unsigned int out_addr_pa;
    unsigned int out_size;
    
    void *data;
#ifdef SUPPORT_2FRAME
    unsigned int src_buf_offset;
    unsigned int src_bg_frame_size;
    int frame_buf_idx;
#endif
};


#endif
