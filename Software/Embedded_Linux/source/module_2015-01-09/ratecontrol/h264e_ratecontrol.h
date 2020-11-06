#ifndef _H264E_RATECONTROL_H_
#define _H264E_RATECONTROL_H_

#include "favce_param.h"
#ifdef VG_INTERFACE
    #include "common.h"
#endif

#define MODULE_NAME "RC"

#define SLICE_TYPE_P    0
#define SLICE_TYPE_B    1
#define SLICE_TYPE_I    2

#define RC_ABS(a)           ((a)>0 ? (a) : -(a))
#define RC_CLIP3(val, low, high)    ((val)>(high)?(high):((val)<(low)?(low):(val)))
#define RC_MAX(a,b)         ((a)>(b)?(a):(b))
#define RC_MIN(a,b)         ((a)<(b)?(a):(b))
#define IsIFrame(a)         (SLICE_TYPE_I == a)
#define iDiff(a,b)          ((a)>(b)?((a)-(b)):((b)-(a)))

#define MAX_BUFFERED_BUDGET         32
#define MB_LEVEL_RC_BOOST_FRAME_NUM 0

//#define CBR_DECAY               0.5

/******************************************
 * user definition to enable or disable
 ******************************************/
#define USE_SHORT_TERN_CPLX
#define FIX_SHORT_TERM_CPLX
#define SHORT_TERM_LEARNING_RATE    4
#define MAX_SHORTTERM_CNT_TABLE     100

#define SUPPORT_EVBR

#define NORMAL_STATE    0x00
#define BUFFER_STATE    0x01
#define LIMIT_STATE     0x02

#define LOG_EMERGY  0
#define LOG_ERROR   1
#define LOG_WARNING 2
//#define LOG_MIDDLE  3
#define LOG_DEBUG   3
#define LOG_INFO    4
#define LOG_DIRECT  0x100

#ifndef VG_INTERFACE
enum mm_vrc_mode_t {
    EM_VRC_CBR = 1,
    EM_VRC_VBR,
    EM_VRC_ECBR,
    EM_VRC_EVBR
};
#endif

struct rc_private_data_t
{
    int rc_mode;
    int b_abr;
    //float fps;
    unsigned int fbase;
    unsigned int fincr;
    
    int qcompress;
    unsigned int bitrate;
    unsigned int max_bitrate;
    //unsigned long long bitrate;
    unsigned int abr_buffer;
    unsigned int budget_buffer;
    //float rate_tolerance;
    unsigned int rate_tolerance; // float: rate_tolerance / SCALE_BASE
    unsigned int mb_count;
    unsigned int idr_period;
    int last_non_b_pict_type;
    unsigned int cbr_decay;
    
    //int b_variable_mbqp;
    //int mb_var_qp;
    
    //double accum_p_norm;
    //double accum_p_qp;
    unsigned int accum_p_norm;
    //unsigned int accum_p_qp;
    unsigned long long accum_p_qp;
    //unsigned int cplxr_sum;
    unsigned long long cplxr_sum;
    unsigned int wanted_bits_window;

    //unsigned long long short_term_cplxsum;
    //unsigned int short_term_cplxcount;
    unsigned int short_term_cplxsum;
    unsigned int short_term_cplxcount;

    unsigned int ip_offset;
    unsigned int pb_offset;
    unsigned int ip_factor;
    unsigned int pb_factor;
    int qp_constant[3];
    unsigned int lstep;
    unsigned int last_qscale;
    unsigned int rate_factor_constant;

    unsigned int last_qscale_for[3];
    int min_quant;
    int max_quant;
    unsigned int min_qscale[3];
    unsigned int max_qscale[3];
#ifdef SUPPORT_EVBR
    unsigned int qscale_constant[3];
#endif
    
    int bframes;
    unsigned int total_bits;

    unsigned int last_satd;
    unsigned int last_rceq;
    
    int avg_qp;
    int avg_qp_act;
    unsigned int qp;
    
    unsigned int bframe_bits;
    unsigned int frame_size_pred;
    int encoded_frame;

    int state;
};

int rc_init_fix (struct rc_private_data_t *rc, struct rc_init_param_t *param);
int rc_start_fix(struct rc_private_data_t *rc, struct rc_frame_info_t *data);
int rc_end_fix  (struct rc_private_data_t *rc, struct rc_frame_info_t *data);
int rc_exit_fix (struct rc_private_data_t *rc);
int rc_increase_qp_fix(struct rc_private_data_t *rc, unsigned int inc_qp);

#endif
