
#ifndef _FMPEG4_DEC_ENITY_H_
#define _FMPEG4_DEC_ENITY_H_

#ifdef REF_POOL_MANAGEMENT
    #ifdef SHARED_POOL
        #include "shared_pool.h"
    #else
        #include "mem_pool.h"
    #endif
#endif

struct mp4d_data_t {
    int engine;
    int chn;
    
    int bitstream_size;
    int dst_fmt;
    /*  if 0, OUTPUT_FMT_CbYCrY
     *  if 4, OUTPUT_FMT_YUV    */
    int output_format;
    int dst_xy;
    int dst_bg_dim;
    int dst_dim;
    int yuv_width_threshold;
    int sub_yuv_ratio;
    
    int frame_width;
    int frame_height;
    int buf_width;
    int buf_height;

    int updated;    
    int	U_off;
	int V_off;
	int stop_job;

#ifdef REF_POOL_MANAGEMENT
    int res_pool_type;
    int re_reg_cnt;
    int over_spec_cnt;

    struct ref_buffer_info_t ref_buffer[2];
    struct ref_buffer_info_t *refer_buf;
    struct ref_buffer_info_t *recon_buf;
#endif

    void *handler;  // mpeg4 decoder local parameter
    unsigned int in_addr_va;
    unsigned int in_addr_pa;
    unsigned int in_size;
    unsigned int out_addr_va;
    unsigned int out_addr_pa;
    unsigned int out_size;

    void *data; // job_item
};


#endif
