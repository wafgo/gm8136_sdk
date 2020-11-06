#ifndef _FMJPEG_ENC_ENTITY_H_
#define _FMJPEG_ENC_ENTITY_H_

#include "video_entity.h"

#define MJE_REINIT      0x01
#define MJE_RC_UPDATE   0x02

#define SUPPORT_2FRAME

struct mje_data_t {
    int engine;
    int chn;
    unsigned int sample;
                    /* OUTPUT_YUV=0         // for 111, 211, 222, 333, 420, 422, 444
					 * OUTPUT_420_YUV=1	    // for 211, 422 output 420YUV   
					 * OUTPUT_CbYCrY=2      // for 211, 420, 422 output CbYCrY
					 * OUTPUT_RGB555=3
					 * OUTPUT_RGB565=4
					 * OUTPUT_RGB888=5  */
    unsigned int restart_interval;
    unsigned int src_fmt;
                    /* 0: MP4 2D, 420p,
					 * 1: sequencial 1D 420p,
					 * 2: H264 2D, 420p
    				 * 3: sequential 1D 420, one case of u82D=1, (only support when DMAWRP is configured)
					 * 4: sequential 1D 422, (only support when DMAWRP is configured)   */
    unsigned int input_format;
    unsigned int src_xy;
    unsigned int src_bg_dim;
    unsigned int src_dim;
    //unsigned int snapshot_cnt;
    unsigned int image_quality;
    //int snap_updated;
    int updated;

    int reinit;
    void *handler;
    int stop_job;

    unsigned int bitstream_size; // output
    unsigned int check_sum;

    unsigned int in_addr_va;
    unsigned int in_addr_pa;
    unsigned int in_size;
    unsigned int out_addr_va;
    unsigned int out_addr_pa;
    unsigned int out_size;
    
    unsigned int U_off;
    unsigned int V_off;
    void *data;
#ifdef SUPPORT_2FRAME
    unsigned int src_buf_offset;
    unsigned int src_bg_frame_size;
    int frame_buf_idx;
#endif
#ifdef SUPPORT_MJE_RC
    void *rc_handler;
    int rc_mode;
    int init_quant;
    int max_quant;
    int min_quant;
    int bitrate;
    int max_bitrate;
    unsigned int src_frame_rate;
    unsigned int fps_ratio;
    int enable_rc;
    int cur_quality;
#endif
};


#endif
