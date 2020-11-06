#ifndef _MP4E_PARAM_H_
#define _MP4E_PARAM_H_

#define RC_RESET_INIT_QUANT     0x01
#define RC_RESET_ALL_PARAM      0xFF

#define LOG_EMERGY  0
#define LOG_ERROR   1
#define LOG_WARNING 2
#define LOG_DEBUG   3
#define LOG_INFO    4
#define LOG_DIRECT  0x100

struct rc_init_param_t
{
    int chn;
    int rc_mode;
    int fbase;
    int fincr;
    unsigned int bitrate;
    /*
    int mb_count;
    int idr_period;
    int bframe;
    int ip_offset;
    int pb_offset;
    int qp_step;
    unsigned int rate_tolerance_fix;    // rate_tolerance = rate_tolerance_fix / 4096
    */
    int qp_constant;    
    int min_quant;
    int max_quant;
    unsigned int max_bitrate;
};

struct frame_info_t
{
    int avg_qp;
    int poc;
    //unsigned int satd;
    int slice_type;
};

struct rc_frame_info_t
{
    int force_qp;                   // input param
    int slice_type;                 // input/update param
    //unsigned int last_satd;         // input param
    
    //struct frame_info_t list[2];    // input param
    //struct frame_info_t cur_frame;  // input param

    unsigned int frame_size;        // out param
    int avg_qp;                     // out param
    //int avg_qp_act;                 // out param
};

/* rate control function */
struct rc_entity_t {
    int (*rc_init)(void **pptHandler, struct rc_init_param_t *rc_param);
    int (*rc_clear)(void *ptHandler);
    int (*rc_get_quant)(void *ptHandler, struct rc_frame_info_t *rc_data);
    int (*rc_update)(void *ptHandler, struct rc_frame_info_t *rc_data);
    //int (*rc_reset_param)(void *ptHandler, unsigned int inc_qp);
};

#endif
