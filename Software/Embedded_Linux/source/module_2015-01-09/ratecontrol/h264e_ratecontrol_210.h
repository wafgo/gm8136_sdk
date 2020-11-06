//------------------------------------------
//   frame layer Rate control header file
//				Richard Wang		
//				03/16/2006
//------------------------------------------
#ifndef _H264F_RATECONRTROL_
#define _H264F_RATECONRTROL_

#include "favce_param.h"
#ifdef VG_INTERFACE
    #include "common.h"
#endif

// function
//#define FAST_CONVERGE
//#define COMPENSATE_BITRATE
#define NEW_EVBR
#define EVALUATE_H264_BITRATE
#define GOP_BASE_CONVERGE
#ifdef GM8136
    #define ADJUST_IPQP_BY_BUFFER_SIZE
#endif

#define RC_MODULE_NAME "RC"

#define RC_CLIP3(val, low, high)    ((val)>(high)?(high):((val)<(low)?(low):(val)))
#define RC_MAX(val0, val1)          ((val0)>(val1)?(val0):(val1))
#define LOG_EMERGY  0
#define LOG_ERROR   1
#define LOG_WARNING 2
#define LOG_DEBUG   3
#define LOG_INFO    4
#define LOG_DIRECT  0x100

#ifndef VG_INTERFACE
#define EM_VRC_CBR  1
#define EM_VRC_VBR  2
#define EM_VRC_ECBR 3
#define EM_VRC_EVBR 4

#define EM_SLICE_TYPE_P     0
#define EM_SLICE_TYPE_B     1
#define EM_SLICE_TYPE_I     2
#endif
//typedef long long int64_t;
//typedef unsigned long long uint64_t;
#define rc_MAX_QUANT  52
#define rc_MIN_QUANT  0

#define shift	17
#define shift1	7
#define scale  (1<<shift)
#define scale_2  (scale<<1)
#define scale_0_1  (long) (scale*0.1)
#define scale_internal  (1<<shift1)

typedef struct
{
	unsigned short frames;
	int64_t total_size_fix;
	int64_t target_rate1;		// target_rate1 = original_target_bitrate * fincr
	int64_t avg_framesize_fix;
	int64_t target_framesize_fix;
} H264FRC_statics;

typedef struct
{
	short rtn_quant;
	short init_quant;
	unsigned int fincr;			// framerate = fbase/fincr
	unsigned int fbase;		// framerate = fbase/fincr
	short max_quant;
	short min_quant;
	int64_t quant_error_fix[rc_MAX_QUANT];
	int64_t sequence_quality_fix;
	int averaging_period;
	int reaction_delay_factor;
	int buffer;
	int reaction_delay_max;		// max frame count will the rate-control stay when away from the target bitrate
	int state_fcount; 	// count up when keep at max_bitrate or target_bitrate
	int mustTarget; 		// = 1: must approach to target_bitrate
	H264FRC_statics normal;
	H264FRC_statics max;
	int qp_total_n;
	int qp_no_n;
	int64_t fsz_th;
	int64_t fsz_devation;
	int64_t framesbb;		// just for debug
	int rc_mode;		// 0: CBR, only target bitrate
							// 1: ECBR,
							//				usually stays at target bitrate,
							//				produce large bitrate when big motion, but never more than max bitrate
							// 2: VBR,
							//				usuall stays at init. qp,
							//				may be decrease quality (qp up) when hit max bitrate
	H264FRC_statics * rc_prev; 	//prevous rc
	int chn;
	int not_dis_intra_time;
	
	int64_t fsz_evbr;
    int evbr_overflow_state;    // Tuba 20140813: remember evbr overflow state or normal state
	int fsz_cnt;
    int ip_offset;  // Tuba 20131206
#ifdef GOP_BASE_CONVERGE
    int gop_size;
    int64_t gop_total_bit;
    int64_t gop_pred_bit;
    int64_t gop_bitrate;
    int64_t i_frame_size;
    int64_t p_frame_size;
    int64_t i_frame_cnt;
    int64_t p_frame_cnt;
    int64_t i_pred_size;
    int64_t p_pred_size;
    int ip_ratio;
    int64_t prev_gop_overflow;
    int64_t evbr_overflow_thd;
#endif
#ifdef ADJUST_IPQP_BY_BUFFER_SIZE
    int cur_slice_type;
    int over_i_buffer_cnt;
#endif
}
H264FRateControl;

typedef struct
{
    unsigned int rc_mode;
    unsigned int bitrate;
	unsigned int fbase;
	unsigned int fincr;
	unsigned int max_quant;
	unsigned int min_quant;
	unsigned int init_quant;
	unsigned int max_bitrate;
	unsigned int reaction_delay;
} H264E_RC_INIT_PARAM;

typedef struct
{
    unsigned int quant;
    unsigned int frame_size;
	unsigned int keyframe;
} H264E_RC_UPDATE;

typedef struct
{
    unsigned int quant;    
} H264E_RC_INFO;

#define RC_DELAY_FACTOR         	8//4
#define RC_AVERAGING_PERIOD	100
#define RC_BUFFER_SIZE				100

#define quality_const (double)((double)2/(double)rc_MAX_QUANT)
#define quality_fix_const (scale_2/rc_MAX_QUANT)
int H264FRateControlInit(H264FRateControl * rate_control,
				unsigned int target_rate,
				unsigned int reaction_delay_factor,
				unsigned int averaging_period,
				unsigned int buffer,
				unsigned int fincr,
				unsigned int fbase,
				int max_quant,
				int min_quant,
				unsigned int initq,
				unsigned int target_rate_max, 		// in bps
							// 1. CBR:	when (target_rate != 0) && (target_rate_max == 0)
							//		only target bitrate
							// 2: ECBR:	when (target_rate != 0) && (target_rate_max > target_rate)
							//		usually stays at target bitrate,
							//		produce large bitrate when big motion, but never more than max bitrate
							// 3: VBR:	when (target_rate_max != 0) && (target_rate == 0)
							//		usuall stays at init. qp,
							//		may be decrease quality (qp up) when hit max bitrate
				unsigned int reaction_delay_max);
							// max frame count will the rate-control stay when away from the target bitrate
							// only valid at ECBR

int H264FRateControlReInit(H264FRateControl * rate_control,
				unsigned int target_rate,
				unsigned int fincr,
				unsigned int fbase,
				int max_quant,
				int min_quant,
				unsigned int target_rate_max, 		// in bps
							// 1. CBR:	when (target_rate != 0) && (target_rate_max == 0)
							//		only target bitrate
							// 2: ECBR: when (target_rate != 0) && (target_rate_max > target_rate)
							//		usually stays at target bitrate,
							//		produce large bitrate when big motion, but never more than max bitrate
							// 3: VBR:	when (target_rate_max != 0) && (target_rate == 0)
							//		usuall stays at init. qp,
							//		may be decrease quality (qp up) when hit max bitrate
				unsigned int reaction_delay_max);
							// max frame count will the rate-control stay when away from the target bitrate
							// only valid at ECBR
				
void H264FRateControlUpdate(H264FRateControl *rate_control,
				  short quant,
				  int frame_size,
				  int keyframe,
				  int frame_qp,
				  int avg_qp);	// caller using rate_control->rtn_quant, it's recommanded qp value for next time use
				  
void H264FRateControlEnd(void);

#ifdef GOP_BASE_CONVERGE
void H264FGOPRateControlInit(H264FRateControl*rate_control, 
                unsigned int target_bitrate,
                unsigned int idr_period,
                unsigned int fincr,
                unsigned int fbase);
unsigned int H264RateControlGetPredSize(H264FRateControl *rate_control, int keyframe);
#endif
/*
int ratecontrol_init(int chn, void *rc_param, int reinit);
int ratecontrol_start(int chn, void *rc_info);
int ratecontrol_end(int chn, void *rc_info);
int ratecontrol_exit(int chn);
*/
#endif
