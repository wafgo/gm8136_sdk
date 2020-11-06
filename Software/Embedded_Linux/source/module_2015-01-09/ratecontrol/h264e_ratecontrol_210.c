#include <linux/kernel.h>
#include <asm/div64.h>
#include <linux/blkdev.h>
#include <linux/syscalls.h>
#include <linux/vmalloc.h>
#include "h264e_ratecontrol_210.h"

/*
	H264 rate control fixed-point version:
	0.9:  Init. version
	0.91: Add EVBR/ECBR mode
	0.92: 1. Fix bug for internal variable overrun
	             when (target-rate = 4Mbps, fps=30, overrun after 23.7 hrs)
	             when (target-rate = 2Mbps, fps=30, overrun after 47.2 hrs)
	             when (target-rate = 2Mbps, fps=15, overrun after 94.4 hrs) ...
	      2. Add debug_define (DEBUG_DUMP) to dump debug message to files
	0.93: Add VBR mode
	0.94: fix bug of long term quality drop to eliminate error integration at ECBR mode
	0.95: Keep original initial quant to EVBR
	0.96: EVBR compensate
*/
//#define DEBUG
//#define DEBUG_IF
//#define DEBUG_DUMP

#ifdef DEBUG
	#define RC_DEBUG(args...) printk(args)
#else 
	#define RC_DEBUG(args...)
#endif 
#ifdef DEBUG_IF
	#define RCIF_DEBUG(args...) printk(args)
#else 
	#define RCIF_DEBUG(args...)
#endif 
#ifdef DEBUG_DUMP
	#include "rc_proc.c"
#endif

#if 0
    #define rc_debug(fmt, args...)  printk(fmt, ## args);
#else
    #define rc_debug(args...)
#endif

#ifdef VG_INTERFACE
    #include "log.h"
    #define DEBUG_M(level, fmt, args...) { \
        if (h264e_rc_log_level == LOG_DIRECT) \
            printk(fmt, ## args); \
        else if (h264e_rc_log_level >= level) \
            printm(RC_MODULE_NAME, fmt, ## args); }
    #define rc_wrn(fmt, args...) { \
            printk(fmt, ## args); \
            printm(RC_MODULE_NAME, fmt, ## args); }
    #define rc_err(fmt, args...) { \
            printk(fmt, ## args); \
            printm(RC_MODULE_NAME, fmt, ## args); }
#else
    #define DEBUG_M(level, fmt, args...) { \
        if (h264e_rc_log_level >= level) \
            printk(fmt, ## args); }
    #define rc_wrn(fmt, args...) { \
        printk(fmt, ## args); }
    #define rc_err(fmt, args...) { \
        printk(fmt, ## args); }
#endif


//int gEVBR_Bitrate_compenstae_frame = 32;
#define EVBR_BR_COMPENSATE_FRAME 32

extern int h264e_rc_log_level;
#ifdef ENABLE_MB_RC
    extern int rc_converge_method;
    extern int learn_gop_period;
    extern int keyframe_learn_rate;
    extern int ratio_base;
    extern int evbr_delay_factor;
    extern int evbr_thd_factor;
#endif

static uint64_t division64_64(uint64_t num, uint64_t den)
{
	uint64_t r = num;
	unsigned int N = 64, q = 0;
    if (0 == den)
        return num;
	do{
		N--;
		if ((r >> N) >= den){
			r -= (den << N);
			q += (1 << N);
		}
	}
	while (N);
	return q;
}

//-------------------------------------------------------------------------

static int64_t division(int64_t a_num, int64_t b_den){
	unsigned int b_hi, sign;
	unsigned int b_lo;
	uint64_t acc;
    if (0 == b_den)
        return a_num;
	sign = (a_num < 0) ^ (b_den < 0);
	if (a_num < 0)
		a_num =  - a_num;
	if (b_den < 0)
		b_den =  - b_den;
	b_hi = b_den >> 32;
	if (b_hi == 0){
		b_lo = b_den &0xffffffff;
//		acc = (uint64_t)a_num / b_lo; //	divide 64bits by 32bits
		do_div(*(uint64_t*)&a_num, b_lo); //	divide 64bits by 32bits
		acc = a_num; 								//	get the divide result from a_num
		if (sign)
			acc =  - acc;
	}
	else{
		acc = division64_64((uint64_t)a_num, (uint64_t)b_den);
		if (sign)
			acc =  - acc;
		return acc;
	}
	return acc;
}

static void h264fRcInit(H264FRC_statics * rc_st,
	unsigned int target_rate,
	unsigned int fincr,
	unsigned int fbase)
{
	int64_t tmp;
	rc_st->frames = 0;
	rc_st->total_size_fix = 0;
	rc_st->target_rate1 = (int64_t)target_rate * (int64_t)fincr;
	tmp = division(rc_st->target_rate1, fbase);
	if (tmp <= 0)
		tmp = 1;
	rc_st->avg_framesize_fix =
	rc_st->target_framesize_fix = tmp;
}

static int h264f_mode(unsigned int target_rate, unsigned int target_rate_max)
{
	if (target_rate == 0) {
		if (target_rate_max == 0) {
			printk ("H264FRateControlInit setting error: target_rate_max = 0 and target_rate = 0\n");
			return -1;
		}
		return 2;		// EVBR mode
	}
	else if (target_rate_max == 0)
		return 0;	// CBR mode
	else if (target_rate_max < target_rate) {
		printk ("H264FRateControlInit setting error: target_rate_max (%d) must be higher than or equal to target_rate (%d)\n", target_rate_max, target_rate);
		return -1;
	}
	else
		return 1;	// ECBR mode
}
#define CBR()       (rate_control->rc_mode == 0)
#define ECBR()		(rate_control->rc_mode == 1)
#define EVBR()		(rate_control->rc_mode == 2)
#define VBR()		(rate_control->rc_mode == 3)
//-------------------------------------------------------------------------
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
					unsigned int target_rate_max,
					unsigned int reaction_delay_max)
{
	int i;
	/*
	if (h264rc_ver_print) {
		h264rc_ver_print = 0;
		printk("\nH264 rate control version: %s\n", H264F_RC_VERSION);
	}
	*/
#ifdef DEBUG_DUMP
	rate_control->chn = rcdump_init();
#endif
	RC_DEBUG("H264FRateControlInit max_quant:%d  min_quant:%d initq:%d target_rate:%d\n",max_quant,min_quant,initq,target_rate);
	RC_DEBUG("H264FRateControlInit target_rate_max:%d  reaction_delay_max:%d\n",target_rate_max,reaction_delay_max);
    RC_DEBUG("fincr:%d  fbase:%d  \n\n", fincr, fbase);
	if ((max_quant < min_quant) || (initq < min_quant) || (max_quant < initq)) {
		printk ("rc_init error, max_quant = %d, min_quant = %d, initq = %d\n", max_quant, min_quant, initq);
		return -1;
	}
	else if (max_quant == min_quant)
		rate_control->rc_mode = 3;// VBR
	else {
		rate_control->rc_mode = h264f_mode(target_rate, target_rate_max);
		if (rate_control->rc_mode < 0)
			return -1;

		if ((0 == fincr) || (0 == fbase)) {
			printk ("rc_init error, fincr = %d, fbase = %d\n", fincr, fbase);
			return -1;
		}
		h264fRcInit (&rate_control->normal, target_rate, fincr, fbase);
		h264fRcInit (&rate_control->max, target_rate_max, fincr, fbase);
		RC_DEBUG("target_framesize_fix %d\n", (int)rate_control->normal.target_framesize_fix);
		RC_DEBUG("target_framesize_fix_max %d\n", (int)rate_control->max.target_framesize_fix);
	}
	rate_control->framesbb= 0;
	rate_control->fincr = fincr;
	rate_control->fbase = fbase;
	rate_control->init_quant =
	rate_control->rtn_quant = initq;
	rate_control->max_quant = (short)max_quant;
	rate_control->min_quant = (short)min_quant;
	for (i = 0; i < rc_MAX_QUANT; ++i)
		rate_control->quant_error_fix[i] = 0;
	rate_control->sequence_quality_fix = division(scale_2, rate_control->rtn_quant);
	rate_control->reaction_delay_factor = (int)reaction_delay_factor;
	rate_control->averaging_period = (int)averaging_period;
	rate_control->buffer = (int)buffer;
	if (ECBR()) {
		if (rate_control->max.target_framesize_fix > (rate_control->normal.target_framesize_fix * 2))
			rate_control->fsz_th = rate_control->normal.target_framesize_fix;
		else
			rate_control->fsz_th = rate_control->max.target_framesize_fix - rate_control->normal.target_framesize_fix;
		rate_control->mustTarget = 1;
		rate_control->state_fcount = 0;
		rate_control->reaction_delay_max = reaction_delay_max;
		rate_control->qp_total_n = 0;
		rate_control->qp_no_n = 0;
		rate_control->fsz_devation = rate_control->fsz_th/2;
	}
	rate_control->rc_prev = NULL;
    rate_control->evbr_overflow_state = 0;
    // Tuba 20140107 start: EVBR bitrate compensate
	if (EVBR()) {
	    rate_control->fsz_evbr = 0;
	    rate_control->fsz_cnt = 0;
	}
    // Tuba 20140107 end
#ifdef ADJUST_IPQP_BY_BUFFER_SIZE
    rate_control->over_i_buffer_cnt = 0;
#endif
	return 0;
}

//-------------------------------------------------------------------------


int H264FRateControlReInit(H264FRateControl * rate_control,
					unsigned int target_rate,
					unsigned int fincr,
					unsigned int fbase,
					int max_quant,
					int min_quant,
					unsigned int target_rate_max,
					unsigned int reaction_delay_max)
{
	int64_t total_size_fix, total_size_fix_max;
	int init_quant;     // Tuba 20110805_0: keep original initial quant when EVBR
	// keep deviation
	total_size_fix = rate_control->normal.total_size_fix -
			division(rate_control->normal.target_rate1 * rate_control->normal.frames, rate_control->fbase);
	total_size_fix_max = rate_control->max.total_size_fix -
			division(rate_control->max.target_rate1 * rate_control->max.frames, rate_control->fbase);
    // Tuba 20110805_0 start: keep original initial quant when EVBR
    if (EVBR()) 
        init_quant = rate_control->init_quant;
    else
        init_quant = rate_control->rtn_quant;
    // Tuba 20110805_0 end
	if (H264FRateControlInit (rate_control,
					target_rate,
					rate_control->reaction_delay_factor,
					rate_control->averaging_period,
					rate_control->buffer,
					fincr,
					fbase,
					max_quant,
					min_quant,
					init_quant,
					target_rate_max,
					reaction_delay_max)	< 0) {
		printk ("H264FRateControlReInit fail\n");
		return -1;
	}
	// keep deviation
	rate_control->normal.total_size_fix = total_size_fix;
	rate_control->max.total_size_fix = total_size_fix_max;
	return 0;
}

#ifdef GOP_BASE_CONVERGE
void H264FGOPRateControlInit(H264FRateControl*rate_control, 
                unsigned int target_bitrate,
                unsigned int idr_period,
                unsigned int fincr,
                unsigned int fbase)
{
    rate_control->gop_total_bit = 0;
    rate_control->gop_pred_bit = 0;
    rate_control->gop_bitrate = target_bitrate * idr_period * fincr;
    rate_control->gop_bitrate = division(rate_control->gop_bitrate, (int64_t)fbase);
    rate_control->gop_size = idr_period;
    rate_control->i_frame_size = 
    rate_control->p_frame_size = 0;
    rate_control->i_frame_cnt = 
    rate_control->p_frame_cnt = 0;
    rate_control->i_pred_size = 
    rate_control->p_pred_size = 0;
    rate_control->ip_ratio = 0;
    rate_control->prev_gop_overflow = 0;
    if (evbr_thd_factor > 0)
        rate_control->evbr_overflow_thd = -division(rate_control->gop_bitrate, (int64_t)evbr_thd_factor);
    else
        rate_control->evbr_overflow_thd = 0;
}

static int64_t gop_current_frame_overflow(H264FRateControl *rc, unsigned int frame_size, int keyframe)
{
    if (rc->ip_ratio) {
        if (keyframe)
            return frame_size - rc->i_pred_size;
        else
            return frame_size - rc->p_pred_size;
    }
    return 0;
}

static int64_t gop_bitrate_overflow(H264FRateControl *rc, unsigned int frame_size, int keyframe)
{
    int64_t overflow = 0;

    // measure overflow
    if (rc->ip_ratio) {
        if (keyframe) {
            rc->prev_gop_overflow = rc->gop_total_bit - rc->gop_pred_bit;
            rc->gop_total_bit = frame_size;
            rc->gop_pred_bit = rc->i_pred_size;
        }
        else {
            rc->gop_pred_bit += rc->p_pred_size;
            rc->gop_total_bit += frame_size;
        }
        overflow = rc->gop_total_bit - rc->gop_pred_bit;
    }
    return overflow;
}

static void evaluate_gop_ip_ratio(H264FRateControl *rc, unsigned int frame_size, int keyframe)
{
    unsigned int i_size, p_size;
    unsigned int factor;

    if (keyframe) {
        if (rc->i_frame_cnt >= learn_gop_period) {
            i_size = division(rc->i_frame_size, rc->i_frame_cnt);
            p_size = division(rc->p_frame_size, rc->p_frame_cnt);
            if (i_size <= p_size || 0 == p_size)
                rc->ip_ratio = ratio_base;
            else {
                rc->ip_ratio = i_size*ratio_base/p_size;
            }
            // i/p pred size
            factor = rc->ip_ratio + (rc->gop_size - 1)*ratio_base;
            rc->p_pred_size = division(rc->gop_bitrate*ratio_base, factor);
            rc->i_pred_size = division(rc->ip_ratio * rc->p_pred_size, ratio_base);
        }
        if (rc->i_frame_cnt >= keyframe_learn_rate) {
            rc->i_frame_size -= division(rc->i_frame_size, keyframe_learn_rate);
            rc->i_frame_size += frame_size;
        }
        else {
            rc->i_frame_size += frame_size;
            rc->i_frame_cnt++;
        }
    }
    else {
        if (rc->p_frame_cnt >= 32) {
            rc->p_frame_size -= division(rc->p_frame_size, 32);
            rc->p_frame_size += frame_size;
        }
        else {
            rc->p_frame_size += frame_size;
            rc->p_frame_cnt++;
        }
    }
}

unsigned int H264RateControlGetPredSize(H264FRateControl *rate_control, int keyframe)
{
#if 0
    if (!((EVBR() && rate_control->evbr_overflow_state) || CBR()))
        return 0;
    if (keyframe)
        return rate_control->i_pred_size;
    else
        return rate_control->p_pred_size;
#else
    if (EVBR()) {
        if (rate_control->evbr_overflow_state) {
            if (keyframe) {
                int64_t overflow = rate_control->gop_total_bit - rate_control->gop_pred_bit;
                if (overflow > 0)
                    return RC_MAX(rate_control->p_pred_size, rate_control->i_pred_size-overflow);
                else
                    return rate_control->i_pred_size;
            }
            else
                return rate_control->p_pred_size;
        }
    }
    else if (CBR()) {
        if (keyframe)
            return rate_control->i_pred_size;
        else
            return rate_control->p_pred_size;
    }
#endif
    return 0;
}
#endif

#define QP_AVG_T 128
void H264FRateControlUpdate(H264FRateControl *rate_control,
					short quant,
					int frame_size,
					int keyframe,
					int frame_qp,
				    int avg_qp)
{
	int64_t deviation_fix;
	int64_t overflow_fix, averaging_period_fix, reaction_delay_factor_fix;
	int64_t quality_scale_fix, base_quality_fix, target_quality_fix;
	int rtn_quant = quant;
	int64_t fsz_delta;
	int64_t fsz_delta_max;
#ifdef GOP_BASE_CONVERGE
    int64_t gop_overflow;
#endif
	H264FRC_statics * rc_st = NULL;
	int near_target = 0;		// 0: Not near and not far rate
										// 1: near target rate
										// -1: far away target rate
										// -2: strongly far away target rate

	rate_control->framesbb++;
	if (VBR ())
		goto limit_rtn;
	//RCIF_DEBUG("H264FRateControlUpdate quant:%d frame_size:%d keyframe:%d \n",quant,frame_size,keyframe);
	frame_size <<= 3;
	fsz_delta = frame_size - rate_control->normal.target_framesize_fix;
	fsz_delta_max = frame_size - rate_control->max.target_framesize_fix;
#ifdef GOP_BASE_CONVERGE
    gop_overflow = gop_bitrate_overflow(rate_control, frame_size, keyframe);
    evaluate_gop_ip_ratio(rate_control, frame_size, keyframe);
#endif
	// add max_target_bitrate, start
	if (ECBR()) {
		int qp_avg;
		if (fsz_delta < rate_control->fsz_th /2) {
			if (rate_control->qp_no_n < QP_AVG_T) {
				rate_control->qp_no_n += 4;
				rate_control->qp_total_n += quant*4;
			}
			else {
				qp_avg = (rate_control->qp_total_n + (QP_AVG_T/2))/QP_AVG_T;
				// speed up falling down
				if (quant < qp_avg) {
					rate_control->qp_total_n -= 4*qp_avg;
					rate_control->qp_total_n += 4*quant;
				}
				else {
					rate_control->qp_total_n -= qp_avg;
					rate_control->qp_total_n += quant;
				}
			}
		}

		if (rate_control->qp_no_n == QP_AVG_T) {
			qp_avg = (rate_control->qp_total_n + (QP_AVG_T/2))/QP_AVG_T;
			if ((rate_control->rc_prev == &rate_control->normal) && (keyframe == 0)) {
				if ((quant == (qp_avg + 1)) || (quant == (qp_avg - 1))) {
					int delta = (fsz_delta>=0) ? fsz_delta: -fsz_delta;
					rate_control->fsz_devation -= rate_control->fsz_devation/32;
					rate_control->fsz_devation += delta/32;
					//printk ("%d rate_control->fsz_devation = %d %d\n", (int)rate_control->framesbb,  (int)rate_control->fsz_devation, (int)rate_control->fsz_th);
				}
			}
		}
		else
			qp_avg = 0;
		if (rate_control->mustTarget == 1) {
			rc_st = &rate_control->normal;					// cbr at normal target_bitrate
			if (++ rate_control->state_fcount >= rate_control->reaction_delay_max) {
				rate_control->mustTarget = 0;
				rate_control->state_fcount = 0;
			}
		}
		else if (keyframe && rate_control->rc_prev)
			rc_st = rate_control->rc_prev;
		else {
			if (qp_avg) {
				if ((fsz_delta > 0 ) && (quant > qp_avg)) {
					if (rate_control->rc_prev != &rate_control->normal) near_target = -2; // far away target strongly
					else near_target = -1; // far away target
				}
				else if ((quant <= (qp_avg + 1)))
					near_target = 1;			// near target
				RC_DEBUG("%d+", qp_avg);
			}

			if ( ((near_target == 1) && (fsz_delta > (rate_control->fsz_devation * 2))) ||
				((near_target == 0) && (fsz_delta > (rate_control->fsz_devation*3/2))) ||//8))) ||
				((near_target == -1) && (fsz_delta > (rate_control->fsz_devation))) ||//4))) ||
				(near_target == -2)) {
				if (++ rate_control->state_fcount >= rate_control->reaction_delay_max) {
					// big motion is too long to stand
					rate_control->mustTarget = 1;
					rate_control->state_fcount = 0;
				}
				if ((fsz_delta_max < 0) || keyframe) {
					if ((near_target < 0) && (quant > qp_avg)) rtn_quant --;
					else if ((near_target > 0) && (quant < qp_avg)) rtn_quant ++;
					rc_st = NULL;												// garentee quality with big motion
				}
				else {
					rc_st = &rate_control->max;								// cbr at max target_bitrate
					fsz_delta = fsz_delta_max;
				}
			}
			else {
				rc_st = &rate_control->normal;					// cbr at normal target_bitrate
				if (rate_control->state_fcount > 2) rate_control->state_fcount -= 2;
				else rate_control->state_fcount = 0;
			}
		}
		// for next time
		if (rc_st == NULL) {
			RC_DEBUG("F%d: %d\t%d\t%d",
				(rate_control->rc_prev != &rate_control->normal)? -1: 0,
				rate_control->mustTarget,
				rate_control->state_fcount,
				(int)rate_control->framesbb);
			RC_DEBUG("\t%d\t%d\t%d\tnt%d", quant, frame_size, keyframe, near_target);
			// reset avg_frame_size
			if (rate_control->rc_prev)
				rate_control->rc_prev->avg_framesize_fix -= rate_control->rc_prev->avg_framesize_fix/2;
			goto limit_rtn;
		}
	}
	else if (EVBR()) {
    // Tuba 20140107 start: EVBR bitrate compensate
    #if 0
		fsz_delta = fsz_delta_max;
		if (fsz_delta < 0) {
			RC_DEBUG("F%d: \t%d\t%d\t%d\t", rate_control->init_quant, quant, frame_size, keyframe);
			if (quant > rate_control->init_quant) rtn_quant --;
			else if (quant < rate_control->init_quant) rtn_quant ++;
			goto limit_rtn; 											// forward init. quant if not exceed over max bitrate
		}
		rc_st = &rate_control->max; 							// cbr at max bitrate
    #else
        #ifdef GOP_BASE_CONVERGE
        if (rc_converge_method) {
        #ifdef NEW_EVBR
            int64_t cur_overflow = gop_current_frame_overflow(rate_control, frame_size, keyframe);
            DEBUG_M(LOG_DEBUG, "EVBR: over %lld, thd %lld, init qp %d, qp %d\n", 
                gop_overflow, rate_control->evbr_overflow_thd, rate_control->init_quant, avg_qp);
            // Tuba 20141128: change condition to >=, because first time qp is the same as init quant
            if (gop_overflow > rate_control->evbr_overflow_thd && avg_qp >= rate_control->init_quant)
                rate_control->evbr_overflow_state = evbr_delay_factor;
            else {
                if (rate_control->evbr_overflow_state > 0)
                    rate_control->evbr_overflow_state--;
            }
            if (rate_control->evbr_overflow_state) {
                rc_st = &rate_control->max; 							// cbr at max bitrate
            }
            else {
                if (quant > rate_control->init_quant && cur_overflow < 0) rtn_quant --;
      			else if (quant < rate_control->init_quant) rtn_quant ++;
                goto limit_rtn;
            }
        #else   // NEW_EVBR
            int64_t cur_overflow = gop_current_frame_overflow(rate_control, frame_size, keyframe);
            if (gop_overflow > rate_control->evbr_overflow_thd && avg_qp >= rate_control->init_quant) {
    		    rc_st = &rate_control->max; 							// cbr at max bitrate
    		    //rate_control->evbr_overflow_state = 1;
    		    rate_control->evbr_overflow_state = evbr_delay_factor;
            }
    		else {
                //rate_control->evbr_overflow_state = 0;
                if (rate_control->evbr_overflow_state > 0)
                    rate_control->evbr_overflow_state--;
                if (quant > rate_control->init_quant && cur_overflow < 0) rtn_quant --;
      			else if (quant < rate_control->init_quant) rtn_quant ++;
                goto limit_rtn;
            }
        #endif  // NEW_EVBR
        }
        else {
    	    fsz_delta = fsz_delta_max;
    		if (rate_control->fsz_cnt < EVBR_BR_COMPENSATE_FRAME) {
    		    rate_control->fsz_evbr += fsz_delta_max;
    		    rate_control->fsz_cnt++;
    		    goto limit_rtn;
    		}
    		else {
    		    rate_control->fsz_evbr = rate_control->fsz_evbr - rate_control->fsz_evbr/EVBR_BR_COMPENSATE_FRAME + fsz_delta_max;
    		    if (rate_control->fsz_evbr < 0) {
           			if (quant > rate_control->init_quant) rtn_quant --;
        			else if (quant < rate_control->init_quant) rtn_quant ++;
    	    		goto limit_rtn; 											// forward init. quant if not exceed over max bitrate
    		    }
    		    rc_st = &rate_control->max; 							// cbr at max bitrate
    		}
        }
        #else   // GOP_BASE_CONVERGE
        fsz_delta = fsz_delta_max;
		if (rate_control->fsz_cnt < EVBR_BR_COMPENSATE_FRAME) {
		    rate_control->fsz_evbr += fsz_delta_max;
		    rate_control->fsz_cnt++;
		    goto limit_rtn;
		}
		else {
		    rate_control->fsz_evbr = rate_control->fsz_evbr - rate_control->fsz_evbr/EVBR_BR_COMPENSATE_FRAME + fsz_delta_max;
		    if (rate_control->fsz_evbr < 0) {
       			if (quant > rate_control->init_quant) rtn_quant --;
    			else if (quant < rate_control->init_quant) rtn_quant ++;
	    		goto limit_rtn; 											// forward init. quant if not exceed over max bitrate
		    }
		    rc_st = &rate_control->max; 							// cbr at max bitrate
		}
        #endif  // GOP_BASE_CONVERGE
    #endif
	// Tube 20140107 end
	}
	else
		rc_st = &rate_control->normal; 						// cbr at target bitrate
	// add max_target_bitrate, stop
	RC_DEBUG("%s%d: %d\t%d\t%d", (rc_st == &rate_control->max)? "H": "L",
		(rate_control->rc_prev != &rate_control->normal)? -1: 0,
		rate_control->mustTarget,
		rate_control->state_fcount,
		(int)rate_control->framesbb);
	RC_DEBUG("\t%d\t%d\t%d", quant, frame_size, keyframe);

	deviation_fix = rc_st->total_size_fix -
			(division(rc_st->target_rate1 * rc_st->frames, rate_control->fbase));

	if ((quant == rate_control->min_quant) && (fsz_delta < 0))			// stop less the deviation when meet min_q 
		goto skip_integrate_err;
	else if (fsz_delta > 0) {
		if (quant == rate_control->max_quant)			// stop larger the deviation when meet max_q
			goto skip_integrate_err;
		else if (deviation_fix > (rc_st->target_framesize_fix * rate_control->buffer)) {
			// stop larger the deviation when meet the largestdeviation, usually occur at ECBR mode
			goto skip_integrate_err;
		}
	}

	rc_st->frames++;
	rc_st->total_size_fix += frame_size;

	deviation_fix += fsz_delta;
	// adjust offset
	if (rc_st->frames >= rate_control->fbase) {
		rc_st->total_size_fix -= rc_st->target_rate1;
		rc_st->frames -= rate_control->fbase;
	}
skip_integrate_err:

	RC_DEBUG("\t%d\t%d\t%d", (int)rc_st->frames, (int)rc_st->total_size_fix, (int)deviation_fix);

	if (rate_control->rtn_quant >= 2) {
		averaging_period_fix = rate_control->averaging_period;
		rate_control->sequence_quality_fix -=
			division(rate_control->sequence_quality_fix, averaging_period_fix);
		rate_control->sequence_quality_fix +=
			division(scale_2, (rate_control->rtn_quant * averaging_period_fix));
		if (rate_control->sequence_quality_fix < scale_0_1)
			rate_control->sequence_quality_fix = scale_0_1;
		if (!keyframe){
			reaction_delay_factor_fix = rate_control->reaction_delay_factor;
			rc_st->avg_framesize_fix -=
				division(rc_st->avg_framesize_fix, reaction_delay_factor_fix);
			rc_st->avg_framesize_fix += division(frame_size, reaction_delay_factor_fix);
		}
	}
    // Tuba 20140117 start: avoid parameter overflow
#if 0
	quality_scale_fix =
		division(scale * rc_st->target_framesize_fix * rc_st->target_framesize_fix,
						rc_st->avg_framesize_fix * rc_st->avg_framesize_fix);
#else
    quality_scale_fix =
        division(division(scale * rc_st->target_framesize_fix, rc_st->avg_framesize_fix) * rc_st->target_framesize_fix, 
                    rc_st->avg_framesize_fix);
#endif
    // Tuba 20140117 end
	base_quality_fix = rate_control->sequence_quality_fix;
	if (quality_scale_fix >= scale) {
		//int64_t xxx = division((int64_t)scale*(int64_t)scale, quality_scale_fix);
		//base_quality_fix = scale - (scale - base_quality_fix)*division(scale, quality_scale_fix);
		base_quality_fix = (int64_t)scale*(int64_t)scale - (scale - base_quality_fix)*division((int64_t)scale*(int64_t)scale, quality_scale_fix);
	}
	else{
		//base_quality = 0.06452 + (base_quality - 0.06452) * quality_scale;
		//base_quality_fix = quality_fix_const + division(((base_quality_fix - quality_fix_const) * quality_scale_fix),scale);
		base_quality_fix = (int64_t)quality_fix_const*(int64_t)scale + ((base_quality_fix - quality_fix_const) * quality_scale_fix);
	}
	base_quality_fix /= scale;
	overflow_fix = -division(deviation_fix, rate_control->buffer);
	
	RC_DEBUG("\t%d\t%d\t%d",
			(int)rate_control->sequence_quality_fix,
			(int)rc_st->avg_framesize_fix, (int)quality_scale_fix);
	/*
	RC_DEBUG("\t%.3f\t%d\t%.3f",
			(float)rate_control->sequence_quality_fix/scale,
			(int)rate_control->avg_framesize_fix,
			(float)quality_scale_fix/scale);*/
	/*
	target_quality =
	base_quality + (base_quality -
	0.06452) * overflow / rate_control->target_framesize;*/
	target_quality_fix =
		base_quality_fix + division((base_quality_fix - quality_fix_const) * overflow_fix, rc_st->target_framesize_fix);
		
	RC_DEBUG("\t%d", (int)base_quality_fix);
	RC_DEBUG("\t%d", (int)target_quality_fix);
	RC_DEBUG("\t%d", quality_fix_const);
	/*
	RC_DEBUG("\t%.3f", (float)base_quality_fix/scale);
	RC_DEBUG("\t%.5f", (float)target_quality_fix/scale);
	RC_DEBUG("\t%f", (float)quality_fix_const/scale);*/
	/*
	if (target_quality > 2.0)
	target_quality = 2.0;
	else if (target_quality < 0.06452)
	target_quality = 0.06452;*/
	if (target_quality_fix > scale_2)
		target_quality_fix = scale_2;
	else if (target_quality_fix < quality_fix_const)
		target_quality_fix = quality_fix_const;

	rtn_quant = (int) division(scale_2, target_quality_fix);

	if (rtn_quant < rc_MAX_QUANT) {
		rate_control->quant_error_fix[rtn_quant] +=
			(division((int64_t)scale_2*(int64_t)scale, target_quality_fix)) - (rtn_quant*scale);
		if (rate_control->quant_error_fix[rtn_quant] >= scale) {
			rate_control->quant_error_fix[rtn_quant] -= scale;
			rtn_quant++;
		}
	}
	RC_DEBUG("\t%d", rtn_quant);
	{
		int q_delta_p = 0;
		int q_delta_m = 0;

    #ifdef ENABLE_MB_RC
        if (1 == rc_converge_method) {
            int over_cond = 0;
            if (keyframe) {
                if (rate_control->prev_gop_overflow < 0 && -rate_control->prev_gop_overflow > division(rate_control->gop_bitrate, 10)) {
                    q_delta_p = -1;
                    q_delta_m = -1;
                    over_cond = 1;
                }
                if (rate_control->prev_gop_overflow > 0 && rate_control->prev_gop_overflow > division(rate_control->gop_bitrate, 10)) {
                    q_delta_p = 1;
                    q_delta_m = 1;
                    over_cond = 1;
                }                    
            }
            if (0 == over_cond) {
                if (rate_control->ip_ratio) {
                    if (gop_overflow < 0)   // underflow
                        q_delta_m = -1;
                    else
                        q_delta_p = 1;
                    if (frame_qp > avg_qp) {
                        q_delta_p = 0;
                        //q_delta_m = -1;
                    }
                    if (frame_qp < avg_qp) {
                        //q_delta_p = 1;
                        q_delta_m = 0;
                    }
                }
                else {
                    if (fsz_delta < 0)
                        q_delta_m = -1;
                    else
                        q_delta_p = 1;
                }
            }
        }
        else {
            if (fsz_delta < 0)
                q_delta_m = -1;
            else
                q_delta_p = 1;
        }
    #else   // ENABLE_MB_RC
        
		if (fsz_delta < 0){
            q_delta_m =  -1;
		}
		else if (fsz_delta > 0){
            q_delta_p = 1;
		}
    #endif  // ENABLE_MB_RC
		if (rtn_quant > rate_control->rtn_quant + q_delta_p)
			rtn_quant = rate_control->rtn_quant + q_delta_p;
		else if (rtn_quant < rate_control->rtn_quant + q_delta_m)
			rtn_quant = rate_control->rtn_quant + q_delta_m;
	}
limit_rtn:
	// save rc_st as prevous
	rate_control->rc_prev = rc_st;
	if (rtn_quant > rate_control->max_quant)
		rtn_quant = rate_control->max_quant;
	else if (rtn_quant < rate_control->min_quant)
		rtn_quant = rate_control->min_quant;
	RC_DEBUG("\t%d\n", rtn_quant);

	//RCIF_DEBUG("H264FRateControlUpdate Done rtn_quant:%d \n",rtn_quant);
	rate_control->rtn_quant = rtn_quant;
}
