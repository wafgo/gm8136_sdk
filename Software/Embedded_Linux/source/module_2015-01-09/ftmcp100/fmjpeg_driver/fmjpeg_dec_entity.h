#ifndef _DECODE_ENTITY_H_
#define _DECODE_ENTITY_H_

#include "video_entity.h"

struct mjd_data_t {
    int engine;
    int chn;
    int bitstream_size;
    int dst_fmt;
    int output_format;
    int dst_xy;
    int dst_bg_dim;
    int dst_dim;
    int yuv_width_threshold;
    int sub_yuv_ratio;
/*
    int init_quant;
    int max_quant;
    int min_quant;
*/
    int src_interlace;
    int src_frame_rate;
/*
    int bitrate;
    int quant;
    int idr_interval;
    int b_frm_num;
    int slice_type;
*/
//    int update_parm;

    int updated;

    int output_422;
    int trans_422pack;

    int frame_width;
    int frame_height;
    int buf_width;
    int buf_height;

    int U_off;
    int V_off;

    int stop_job;
    
    // return value
    int NumofComponents;
    int output_width;
    int output_height;
    int sample;
    int err;

    unsigned int out_addr_va;
    unsigned int out_addr_pa;
    int out_size;
    unsigned int in_addr_va;
    unsigned int in_addr_pa;
    int in_size;
    void *handler;
    void *data;
};

int test_and_set_engine_busy(int engine);
int is_engine_busy(int engine);
int is_engine_idle(int engine);
void engine_work(struct mjd_data_t *decode_data);
 

#endif

