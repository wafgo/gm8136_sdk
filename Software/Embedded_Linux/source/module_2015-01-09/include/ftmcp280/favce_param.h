
#ifndef _FAVCE_PARAM_H_
#define _FAVCE_PARAM_H_

#define RC_RESET_INIT_QUANT     0x01
#define RC_RESET_ALL_PARAM      0xFF

//#define GET_RC_INFO
//#define EVALUATE_FRAME_SIZE
#define ENABLE_MB_RC

struct rc_init_param_t
{
    int chn;
    int rc_mode;
    int fbase;
    int fincr;
    unsigned int bitrate;
    int mb_count;
    //int mb_var_qp;
    
    int idr_period;
    int bframe;
    int ip_offset;
    int pb_offset;
    int qp_constant;
    int qp_step;
    
    int min_quant;
    int max_quant;

    //double rate_tolerance;
    unsigned int rate_tolerance_fix;    // rate_tolerance = rate_tolerance_fix / 4096
    unsigned int max_bitrate;
    int bu_rc_enable;
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
    unsigned int last_satd;         // input param
    
    struct frame_info_t list[2];    // input param
    struct frame_info_t cur_frame;  // input param
    //unsigned int frame_num;
    //int idr_period;

    unsigned int frame_size;        // out param
    int avg_qp;                     // out param
    int avg_qp_act;                 // out param
    //int last_b_frame;

    //unsigned int frame_size_estimate;
};

/* rate control function */
struct rc_entity_t {
    int (*rc_init)(void **pptHandler, struct rc_init_param_t *rc_param);
    int (*rc_clear)(void *ptHandler);
    int (*rc_get_quant)(void *ptHandler, struct rc_frame_info_t *rc_data);
    int (*rc_update)(void *ptHandler, struct rc_frame_info_t *rc_data);
    int (*rc_reset_param)(void *ptHandler, unsigned int inc_qp);
#ifdef GET_RC_INFO
    struct rc_init_param_t *(*rc_get_param)(int chn);
#endif
};

#endif
