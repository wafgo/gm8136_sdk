#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/div64.h>
#include <linux/blkdev.h>
#include <linux/syscalls.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include "h264e_ratecontrol_210.h"
#ifdef VG_INTERFACE
    #include "log.h"
#endif

#ifdef VG_INTERFACE
    #define H264E_RC_PROC_DIR "videograph/favce_rc"
#else
    #define H264E_RC_PROC_DIR "favce_rc"
#endif
/*
* 0.1.1: first version
* 0.1.2: 1. fix bug: add re-encode update qp function
*        2. add proc to control rc mode (VBR &  ECBR)
*        3. add force qp (for re-encode)
* 1.1.2: change ratecontrol (use mcp210 rate control)
* 1.1.3: 1. fix bug: EVBR compensate (20140117)
*        2. avoid parameter overflow (20140117)
* 1.1.4: add level of bitrate converge (20140124)
* 1.1.5: adjust buffer period by frame rate (20140312)
* 1.1.6: handle divided by zero
* 1.1.7: dump current qp to proc/info (20140418)
* 1.1.8: 1. add proc to control ECBR delay factor
*        2. VBR min/max quant
*        3. handle key frame spend too much bitrate
* 1.1.9: new CBR: converge low bitrate (2014/08/14)
* 1.1.10: auto adjust ip offset when buffer overflow (2014/09/30)
* 1.1.11: fix bug: EVBR wrong check condition (2014/11/28)
*/

#define H264E_RC_VERISON    0x10100B00
#define H264E_RC_MAJOR      ((H264E_RC_VERISON>>28)&0x0F)    // huge change
#define H264E_RC_MINOR      ((H264E_RC_VERISON>>20)&0xFF)    // interface change 
#define H264E_RC_MINOR2     ((H264E_RC_VERISON>>8)&0x0FFF)   // functional modified or buf fixed
#define H264E_RC_BRANCH     (H264E_RC_VERISON&0xFF)          // branch for customer request

#define MAX_RC_LOG_CHN  128
#define DUMP_CUR_QP

#ifdef VG_INTERFACE
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

struct rc_bitrate_info_t
{
    unsigned int total_byte;
    unsigned int frame_cnt;
    unsigned int fr_base;
    unsigned int fr_incr;
    unsigned int i_frame_cnt;
};


/*************** proc ***************/
struct proc_dir_entry *rc_proc = NULL;
struct proc_dir_entry *rc_level_proc = NULL;
struct proc_dir_entry *rc_info_proc = NULL;
struct proc_dir_entry *rc_dbg_proc = NULL;
struct proc_dir_entry *rc_param_proc = NULL;

#ifdef ENABLE_MB_RC
    unsigned int rc_converge_method = 0;
    module_param(rc_converge_method, uint, S_IRUGO|S_IWUSR);
    MODULE_PARM_DESC(rc_converge_method, "Max channel number");
#endif

//struct rc_init_param_t log_rc_param[MAX_RC_LOG_CHN];
/*********** proc debug parameter ***********/
int h264e_rc_log_level = LOG_WARNING;
int buffer_period_factor = 3;
#ifdef ENABLE_MB_RC
    int learn_gop_period = 1;
    int ratio_base = 10;
    int keyframe_learn_rate = 4;
    int evbr_delay_factor = 10;
    int evbr_thd_factor = 0;
#endif
/********************************************/
static const char rc_mode_name[5][5] = {"", "CBR", "VBR", "ECBR", "EVBR"};
static struct rc_init_param_t log_rc_param[MAX_RC_LOG_CHN];
static struct rc_bitrate_info_t rc_bitrate[MAX_RC_LOG_CHN];
static int ecbr_delay_factor = RC_DELAY_FACTOR;
static int force_using_default_param = 0;
#ifdef ADJUST_IPQP_BY_BUFFER_SIZE
    int adjust_i_frame_num = 3;
    int min_ip_offset = -8;
#endif
#ifdef EVALUATE_H264_BITRATE
    static int bitrate_log_chn = -1;
    static int bitrate_period = 1;
    static int bitrate_measure_base = 1;    // 0: frame rate (sec), 1: gop
#endif

#ifdef DUMP_CUR_QP
    static int cur_qp[MAX_RC_LOG_CHN];
    static int key_frame_qp[MAX_RC_LOG_CHN];
    static int ip_ratio[MAX_RC_LOG_CHN];
#endif

extern int rc_register(struct rc_entity_t *entity);
extern int rc_deregister(void);

static struct rc_init_param_t default_rc_param = 
{
    rc_mode:        EM_VRC_CBR,
    fbase:          30,
    fincr:          1,
    bitrate:        2000,
    idr_period:     30,
    /*
    mb_count:       1350,
    idr_period:     30,
    bframe:         0,
    ip_offset:      2,
    pb_offset:      2,
    qp_step:        1,
    rate_tolerance_fix: 409,
    */
    ip_offset:      2,
    qp_constant:    26,
    min_quant:      1,
    max_quant:      51,
    max_bitrate:    2000
};

#ifdef EVALUATE_H264_BITRATE
static int evaluation_bitrate(int chn, unsigned frame_size)
{
    if (bitrate_log_chn != chn)
        return 0;

    rc_bitrate[chn].total_byte += frame_size;
    rc_bitrate[chn].frame_cnt++;
    //printk("total %d, frame %d, base %d, incr %d\n", rc_bitrate[chn].total_byte, rc_bitrate[chn].frame_cnt, rc_bitrate[chn].fr_base, rc_bitrate[chn].fr_incr);
    if (rc_bitrate[chn].fr_incr > 0 && rc_bitrate[chn].fr_base > 0) {
        if (bitrate_period > 0 && rc_bitrate[chn].frame_cnt > (bitrate_period * rc_bitrate[chn].fr_base / rc_bitrate[chn].fr_incr)) {
            // evaluate bitrate
            printk("{chn%d} bitrate\t%d\n", chn, rc_bitrate[chn].total_byte * 8 / bitrate_period);
            rc_bitrate[chn].frame_cnt = 0;
            rc_bitrate[chn].total_byte = 0;
        }
    }
    return 0;
}

static int evaluation_gop_bitrate(int chn, unsigned frame_size, int keyframe)
{
    if (bitrate_log_chn != chn)
        return 0;

    if (keyframe) {
        if (rc_bitrate[chn].i_frame_cnt >= bitrate_period) {
            printk("{chn%d} gop bitrate\t%d\n", chn, rc_bitrate[chn].total_byte*8);
            rc_bitrate[chn].frame_cnt = 0;
            rc_bitrate[chn].total_byte = 0;
            rc_bitrate[chn].i_frame_cnt = 0;
        }
        rc_bitrate[chn].i_frame_cnt++;
    }
    rc_bitrate[chn].total_byte += frame_size;
    rc_bitrate[chn].frame_cnt++;
    return 0;
}
#endif

static int division_algorithm(int a, int b)
{
    if (0 == a || 0 == b)
        return 0;
    while (a > 0 && b > 0) {
        if (a > b)
            a = a%b;
        else
            b = b%a;
    }
    if (0 == a)
        return b;
    if (0 == b)
        return a;
    return 0;
}

int ratecontrol_init(void **pptHandler, struct rc_init_param_t *rc_param)
{
    H264FRateControl *rc;
    int ret = 0;
    int min_quant, max_quant;
	// Tuba 20140313: adjust buffer period by frame rate
    unsigned int buffer_size = RC_BUFFER_SIZE;
    int fps_gcd;

    /* 1: EM_VRC_CBR, 2: EM_VRC_VBR, 3: EM_VRC_ECBR, 4: EM_VRC_EVBR */
    // check parameter value
    if (rc_param->rc_mode < EM_VRC_CBR || rc_param->rc_mode > EM_VRC_EVBR || force_using_default_param) {
        if (!force_using_default_param)
            rc_err("h264e rc_mode(%d) out of range, force default setting\n", rc_param->rc_mode);
        memcpy(rc_param, &default_rc_param, sizeof(struct rc_init_param_t));
    }
    if (0 == rc_param->fbase || 0 == rc_param->fincr) {
        rc_err("invalid frame rate = %d / %d, force 30 fps\n", rc_param->fbase, rc_param->fincr);
        rc_param->fbase = 30;
        rc_param->fincr = 1;
    }

    fps_gcd = division_algorithm(rc_param->fbase, rc_param->fincr);
    if (fps_gcd > 1) {
        rc_param->fbase /= fps_gcd;
        rc_param->fincr /= fps_gcd;
    }

    if (buffer_period_factor) { // fast converge when lower frame rate
        if (rc_param->fbase / rc_param->fincr < 25)
            buffer_size = rc_param->fbase * buffer_period_factor / rc_param->fincr;
    }

    /* CBR: bitrate != 0 && max_bitrate = 0
     * ECBR: bitrate != && max_bitrate != 0
     * EVBR: bitrate = 0 && max_bitrate != 0
     * VBR: init quant = max quant = min quant  */
    if (0 == rc_param->min_quant)
        rc_param->min_quant = 1;
    if (0 == rc_param->max_quant)
        rc_param->max_quant = 51;
    rc_param->min_quant = RC_CLIP3(rc_param->min_quant, 1, 51);
    rc_param->max_quant = RC_CLIP3(rc_param->max_quant, 1, 51);

    if (rc_param->max_quant < rc_param->min_quant) {
        rc_wrn("max_quant(%d) must be larger than min_quant(%d), force default value\n", rc_param->max_quant, rc_param->min_quant);
        rc_param->min_quant = 1;
        rc_param->max_quant = 51;
    }
    
    if (rc_param->qp_constant < rc_param->min_quant || rc_param->qp_constant > rc_param->max_quant) {
        rc_wrn("init quant(%d) is out of min/max quant, force middle value %d\n", 
			rc_param->qp_constant, (rc_param->min_quant + rc_param->max_quant)/2);
        rc_param->qp_constant = (rc_param->min_quant + rc_param->max_quant)/2;
    }
    min_quant = rc_param->min_quant;
    max_quant = rc_param->max_quant;
    switch (rc_param->rc_mode) {
    case EM_VRC_CBR:
        if (0 == rc_param->bitrate)
            rc_param->bitrate = rc_param->max_bitrate;
        if (0 == rc_param->bitrate) {
            rc_wrn("CBR bitarte can not be zero, force default value 2M\n");
            rc_param->bitrate = 2000;
        }
        rc_param->max_bitrate = 0;
        break;
    case EM_VRC_VBR:
        //rc_param->min_quant = rc_param->max_quant = rc_param->qp_constant;
        // Tuba 20140428: VBR handle original min/max quant
        min_quant = max_quant = rc_param->qp_constant;
        rc_param->bitrate = rc_param->max_bitrate = 0;
        break;
    case EM_VRC_ECBR:
        if (0 == rc_param->bitrate || 0 == rc_param->max_bitrate) {
            rc_wrn("ECBR bitrate and max bitrate can not be zero, force default value 2M\n");
            rc_param->bitrate = rc_param->max_bitrate = 2000;
        }
        if (rc_param->bitrate > rc_param->max_bitrate) {
            rc_wrn("ECBR max bitrate can not less than bitrate, force max bitrate the same as bitrate\n");
            rc_param->max_bitrate = rc_param->bitrate;
        }
        break;
    case EM_VRC_EVBR:
        if (0 == rc_param->max_bitrate)
            rc_param->max_bitrate = rc_param->bitrate;
        if (0 == rc_param->max_bitrate) {
            rc_wrn("EVBR max bitarte can not be zero, force default value 2M\n");
            rc_param->max_bitrate = 2000;
        }
        rc_param->bitrate = 0;
        break;
    }

    DEBUG_M(LOG_INFO, "[mode %d] bitrate: %d, fps: %d/%d, qp: %d (%d, %d)\n", rc_param->rc_mode, rc_param->bitrate,
        rc_param->fbase, rc_param->fincr, rc_param->qp_constant, rc_param->max_quant, rc_param->min_quant);

    if (*pptHandler)
        rc = *pptHandler;
    else
        rc = kmalloc(sizeof(H264FRateControl), GFP_ATOMIC);

    ret = H264FRateControlInit(rc, 
        rc_param->bitrate*1000,
        ecbr_delay_factor,
        RC_AVERAGING_PERIOD,
        buffer_size,
        rc_param->fincr,
        rc_param->fbase,
        max_quant,
        min_quant,
        rc_param->qp_constant,
        rc_param->max_bitrate*1000,
        3);

#ifdef GOP_BASE_CONVERGE
    if (EM_VRC_CBR == rc_param->rc_mode)
        H264FGOPRateControlInit(rc, rc_param->bitrate*1000, rc_param->idr_period, rc_param->fincr, rc_param->fbase);
    else if (EM_VRC_EVBR == rc_param->rc_mode)
        H264FGOPRateControlInit(rc, rc_param->max_bitrate*1000, rc_param->idr_period, rc_param->fincr, rc_param->fbase);
#endif

    if (ret < 0) {
        rc_err("h264e rate control init fail\n");
        goto rc_init_fail;
    }
    *pptHandler = rc;
    rc->chn = rc_param->chn;
    rc->ip_offset = rc_param->ip_offset;
    rc_bitrate[rc->chn].frame_cnt = rc_bitrate[rc->chn].total_byte = rc_bitrate[rc->chn].i_frame_cnt = 0;
    rc_bitrate[rc->chn].fr_base = rc_param->fbase;
    rc_bitrate[rc->chn].fr_incr = rc_param->fincr;
    //DEBUG_M(LOG_INFO, "{chn%d} frame rate %d / %d\n", rc->chn, rc_bitrate[rc->chn].fr_base, rc_bitrate[rc->chn].fr_incr);
    if (rc_param->chn < MAX_RC_LOG_CHN)
        memcpy((void *)(&log_rc_param[rc_param->chn]), rc_param, sizeof(struct rc_init_param_t));
    DEBUG_M(LOG_DEBUG, "{chn%d} rc %s, fps: %d/%d, br: %d/%d, qp: %d (%d ~ %d)\n", rc->chn,
        rc_mode_name[rc_param->rc_mode], rc_param->fbase, rc_param->fincr, rc_param->bitrate, rc_param->max_bitrate, 
        rc_param->qp_constant, rc_param->min_quant, rc_param->max_quant);
    return 0;
rc_init_fail:
    if (rc)
        kfree(rc);
    *pptHandler = NULL;
    return ret;
}

int ratecontrol_exit(void *ptHandler)
{
    if (ptHandler)
        kfree(ptHandler);
    return 0;
}

int ratecontrol_get_quant(void *ptHandler, struct rc_frame_info_t *rc_data)
{
    H264FRateControl *rc = NULL;
    int qp = 26;
    int min_quant, max_quant;

    if (ptHandler) {
        rc = (H264FRateControl *)ptHandler;
        if (EM_VRC_VBR == log_rc_param[rc->chn].rc_mode) {
            min_quant = log_rc_param[rc->chn].min_quant;
            max_quant = log_rc_param[rc->chn].max_quant;
        }
        else {
            min_quant = rc->min_quant;
            max_quant = rc->max_quant;
        }
        if (rc_data->force_qp > 0)
            qp = RC_CLIP3(rc_data->force_qp, min_quant, max_quant);
        else {
            if (EM_SLICE_TYPE_I == rc_data->slice_type) {
                qp = RC_CLIP3(rc->rtn_quant - rc->ip_offset, min_quant, max_quant);
                if (rc && rc->chn < MAX_RC_LOG_CHN)
                    key_frame_qp[rc->chn] = qp;
            }
            else
                qp = rc->rtn_quant;
        }
    #ifdef ENABLE_MB_RC
        if (rc_converge_method > 0) {
            rc_data->frame_size = H264RateControlGetPredSize(rc, (EM_SLICE_TYPE_I == rc_data->slice_type)? 1: 0);
        }
        else
            rc_data->frame_size = 0;
    #endif
    #ifdef ADJUST_IPQP_BY_BUFFER_SIZE
        rc->cur_slice_type = rc_data->slice_type;
    #endif
    }
    else {
        rc_err("rate control is not initialized\n");
    }
#ifdef DUMP_CUR_QP
    if (rc && rc->chn < MAX_RC_LOG_CHN) {
        cur_qp[rc->chn] = qp;
        ip_ratio[rc->chn] = rc->ip_ratio;
    }
#endif    
    return qp;
}

int ratecontrol_update(void *ptHandler, struct rc_frame_info_t *rc_data)
{
    H264FRateControl *rc = (H264FRateControl *)ptHandler;
    if (NULL == rc)
        rc_err("rate control is not initialized\n");
#if 0
    H264FRateControlUpdate(rc, 
        RC_CLIP3(rc->rtn_quant+(rc_data->avg_qp_act - rc_data->avg_qp), rc->min_quant, rc->max_quant),
        rc_data->frame_size,
        (EM_SLICE_TYPE_I == rc_data->slice_type ? 1 : 0),
        rc_data->avg_qp, rc_data->avg_qp_act);
#else
    H264FRateControlUpdate(rc, 
        rc->rtn_quant, 
        rc_data->frame_size,
        (EM_SLICE_TYPE_I == rc_data->slice_type ? 1 : 0),
        rc_data->avg_qp, rc_data->avg_qp_act);
#endif
#ifdef EVALUATE_H264_BITRATE
    if (0 == bitrate_measure_base)
        evaluation_bitrate(rc->chn, rc_data->frame_size);
    else if (1 == bitrate_measure_base)
        evaluation_gop_bitrate(rc->chn, rc_data->frame_size, (EM_SLICE_TYPE_I == rc_data->slice_type ? 1 : 0));
#endif
#ifdef ADJUST_IPQP_BY_BUFFER_SIZE
    if (EM_SLICE_TYPE_I == rc_data->slice_type) {
        if (rc->over_i_buffer_cnt)
            rc->over_i_buffer_cnt--;
    }
#endif

    return 0;
}

int ratecontrol_reset_param(void *ptHandler, unsigned int inc_qp)
{
    H264FRateControl *rc;
    int qp = 26;
    if (ptHandler) {
        rc = (H264FRateControl *)ptHandler;
        qp = RC_CLIP3(rc->rtn_quant + inc_qp, rc->min_quant, rc->max_quant);
        rc->rtn_quant = qp;     // Tuba 20140930: force qp updated
    #ifdef ADJUST_IPQP_BY_BUFFER_SIZE
        if (EM_SLICE_TYPE_I == rc->cur_slice_type) {
            rc->over_i_buffer_cnt+=2;
            if (adjust_i_frame_num > 0 && rc->over_i_buffer_cnt > 2*adjust_i_frame_num && rc->ip_offset >= min_ip_offset) {
                rc->ip_offset--;
                rc_wrn("continuous i frame overflow, force ip offset %d\n", rc->ip_offset);
                //rc->over_i_buffer_cnt = 0;
            }
        }
    #endif
    }
    else
        printk("reset: rate control is not initialization\n");
    return qp;
}

static int h264e_rc_msg(char *str)
{
    int len = 0;
    if (NULL == str) {
        printk("H264 rate control");
    #ifdef VG_INTERFACE
        printk("(VG)");
    #endif
        printk(" version: %d.%d.%d", H264E_RC_MAJOR, H264E_RC_MINOR, H264E_RC_MINOR2);
        if (H264E_RC_BRANCH)
            printk(".%d", H264E_RC_BRANCH);
        printk(", built @ %s %s\n", __DATE__, __TIME__);
    }
    else {
        len += sprintf(str+len, "H264 rate control");
    #ifdef VG_INTERFACE
        len += sprintf(str+len, "(VG)");
    #endif
        len += sprintf(str+len, " version: %d.%d.%d", H264E_RC_MAJOR, H264E_RC_MINOR, H264E_RC_MINOR2);
        if (H264E_RC_BRANCH)
            len += sprintf(str+len, ".%d", H264E_RC_BRANCH);
        len += sprintf(str+len, ", built @ %s %s\n", __DATE__, __TIME__);
    }
    return len;
}

// ========================= proc =========================

static int proc_rc_log_level_read(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    
    len += sprintf(page+len, "Log level = %d (%d: emergy, %d: error, %d: warning, %d: info, %d: debug)\n", 
        h264e_rc_log_level, LOG_EMERGY, LOG_ERROR, LOG_WARNING, LOG_INFO, LOG_DEBUG);
    *eof = 1;
    return len;
}

static int proc_rc_log_level_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int level;
    sscanf(buffer, "%d", &level);
    h264e_rc_log_level = level;
    printk("\nLog level = %d (%d: emergy, %d: error, %d: warning, %d: info, %d: debug)\n", 
        h264e_rc_log_level, LOG_EMERGY, LOG_ERROR, LOG_WARNING, LOG_INFO, LOG_DEBUG);
    return count;
}

static int proc_rc_info_read(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0, i;

    len += h264e_rc_msg(page+len);

#ifdef DUMP_CUR_QP
    len += sprintf(page+len, " chn   mode         fps          bitrate    max.br    init.q  min.q  max.q   qp   I.qp  ip.r\n");
    len += sprintf(page+len, "=====  ======  ===============  =========  =========  ======  =====  =====  ====  ====  ====\n");
#else
    len += sprintf(page+len, " chn   mode         fps          bitrate    max.br    init.q  min.q  max.q\n");
    len += sprintf(page+len, "=====  ======  ===============  =========  =========  ======  =====  =====\n");
#endif
    for (i = 0; i < MAX_RC_LOG_CHN; i++) {
        if (log_rc_param[i].rc_mode >= EM_VRC_CBR && log_rc_param[i].rc_mode <= EM_VRC_EVBR) {
        #ifdef DUMP_CUR_QP
            len += sprintf(page+len, " %3d    %4s   %7d/%-7d  %7d    %7d      %2d      %2d     %2d    %2d    %2d   %2d.%d\n",
                i, rc_mode_name[log_rc_param[i].rc_mode], log_rc_param[i].fbase, log_rc_param[i].fincr, 
                log_rc_param[i].bitrate, log_rc_param[i].max_bitrate, 
                log_rc_param[i].qp_constant, log_rc_param[i].min_quant, log_rc_param[i].max_quant, 
                cur_qp[i], key_frame_qp[i], ip_ratio[i]/ratio_base, ip_ratio[i]%ratio_base);
        #else
            len += sprintf(page+len, " %3d    %4s   %7d/%-7d  %7d    %7d      %2d      %2d     %2d\n",
                i, rc_mode_name[log_rc_param[i].rc_mode], log_rc_param[i].fbase, log_rc_param[i].fincr, 
                log_rc_param[i].bitrate, log_rc_param[i].max_bitrate, 
                log_rc_param[i].qp_constant, log_rc_param[i].min_quant, log_rc_param[i].max_quant);
        #endif
        }
    }
    *eof = 0;
    return len;
}

static int proc_rc_info_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    return count;
}

static int proc_rc_dbg_read(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;

#ifdef EVALUATE_H264_BITRATE
    len += sprintf(page+len, " 1: channel of dump bitrate, -1 not dump (%d)\n", bitrate_log_chn);
    len += sprintf(page+len, " 2: bitrate period (%d)\n", bitrate_period);
    len += sprintf(page+len, " 3: evaluate bitrate 0: time base, 1: gop base (%d)\n", bitrate_measure_base);
#endif
    len += sprintf(page+len, " 4: buffer period factor (%d)\n", buffer_period_factor);
    len += sprintf(page+len, " 5: ecbr delay factor (%d)\n", ecbr_delay_factor);
    len += sprintf(page+len, " 6: force using default param (%d)\n", force_using_default_param);
#ifdef ENABLE_MB_RC
    len += sprintf(page+len, "10: converge method (%d)\n", rc_converge_method);
    len += sprintf(page+len, "11: ratio learn gop number (%d)\n", learn_gop_period);
    len += sprintf(page+len, "12: ratio base (%d)\n", ratio_base);
    len += sprintf(page+len, "13: evbr delay factor (%d)\n", evbr_delay_factor);
    len += sprintf(page+len, "14: evbr near factor (%d)\n", evbr_thd_factor);
    len += sprintf(page+len, "15: i frame learning rate (%d)\n", keyframe_learn_rate);
#endif
#ifdef ADJUST_IPQP_BY_BUFFER_SIZE
    len += sprintf(page+len, "20: max cnt of i frame buffer overflow, 0: disable (%d)\n", adjust_i_frame_num);
    len += sprintf(page+len, "21: min ip offset (%d)\n", min_ip_offset);
#endif
    *eof = 0;
    return len;
}

static int proc_rc_dbg_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = count;
    int cmd_idx, value;    
    char str[80];
    //int i;
    if (copy_from_user(str, buffer, len))
        return 0;
    str[len] = '\0';
    sscanf(str, "%d %d\n", &cmd_idx, &value);
    switch (cmd_idx) {
#ifdef EVALUATE_H264_BITRATE
    case 1:
        if (value < 0 || value >= MAX_RC_LOG_CHN)
            bitrate_log_chn = -1;
        else {
            bitrate_log_chn = value;
            rc_bitrate[bitrate_log_chn].total_byte = 0;
            rc_bitrate[bitrate_log_chn].frame_cnt = 0;
            rc_bitrate[bitrate_log_chn].i_frame_cnt = 0;
        }
        break;
    case 2:
        if (value > 0)
            bitrate_period = value;
        break;
    case 3:
        if (0 == value || 1 == value)
            bitrate_measure_base = value;
        break;
#endif
    case 4:
        if (value >= 0 && value < 10)
            buffer_period_factor = value;
        break;
    case 5:
        if (value > 0)
            ecbr_delay_factor = value;
        else
            ecbr_delay_factor = RC_DELAY_FACTOR;
        break;
    case 6:
        if (1 == value)
            force_using_default_param = 1;
        else
            force_using_default_param = 0;
        break;
#ifdef ENABLE_MB_RC
    case 10:
        if (0 == value || 1 == value)
            rc_converge_method = value;
        break;
    case 11:
        if (value > 0)
            learn_gop_period = value;
        else
            learn_gop_period = 1;
        break;
    case 12:
        if (value > 0)
            ratio_base = value;
        else
            ratio_base = 1;
        break;
    case 13:
        if (value > 1)
            evbr_delay_factor = value;
        break;
    case 14:
        if (value >= 0 && value < 100)
            evbr_thd_factor = value;
        break;
    case 15:
        if (value > 0)
            keyframe_learn_rate = value;
        break;
#endif
#ifdef ADJUST_IPQP_BY_BUFFER_SIZE
    case 20:
        if (value > 0)
            adjust_i_frame_num = value;
        else
            adjust_i_frame_num = 0;
        break;
    case 21:
        min_ip_offset = value;
        break;
#endif
    default:
        break;
    }

    return count;
}

static int proc_rc_param_read(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;

    len += sprintf(page+len, "usage: echo [id] [value] > /proc/%s/param\n", H264E_RC_PROC_DIR);
    len += sprintf(page+len, "default rc parameter\n");
    len += sprintf(page+len, "id  param name    value\n");
    len += sprintf(page+len, "==  ============  =====\n");
    len += sprintf(page+len, "0   rc mode       %d (%d: CBR, %d: VBR, %d: ECBR, %d EVBR)\n", 
        default_rc_param.rc_mode, EM_VRC_CBR, EM_VRC_VBR, EM_VRC_ECBR, EM_VRC_EVBR);
    len += sprintf(page+len, "1   fps base      %d\n", default_rc_param.fbase);
    len += sprintf(page+len, "2   fps incr      %d (fps = base / incr)\n", default_rc_param.fincr);
    len += sprintf(page+len, "3   bitrate       %d\n", default_rc_param.bitrate);
    len += sprintf(page+len, "4   max bitrate   %d\n", default_rc_param.max_bitrate);
    len += sprintf(page+len, "5   init quant    %d\n", default_rc_param.qp_constant);
    len += sprintf(page+len, "6   min quant     %d\n", default_rc_param.min_quant);
    len += sprintf(page+len, "7   max quant     %d\n", default_rc_param.max_quant);
    len += sprintf(page+len, "8   ip qp offset  %d\n", default_rc_param.ip_offset);
    len += sprintf(page+len, "9   gop           %d\n", default_rc_param.idr_period);

    *eof = 0;
    return len;
}

static int proc_rc_param_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = count;
    int cmd_idx, value;    
    char str[80];
    if (copy_from_user(str, buffer, len))
        return 0;
    str[len] = '\0';
    sscanf(str, "%d %d\n", &cmd_idx, &value);
    switch (cmd_idx) {
    case 0:
        if (value >= EM_VRC_CBR && value <= EM_VRC_EVBR)
            default_rc_param.rc_mode = value;
        break;
    case 1:
        if (value > 0)
            default_rc_param.fbase = value;
        break;
    case 2:
        if (value > 0)
            default_rc_param.fincr = value;
        break;
    case 3:
        if (value > 0)
            default_rc_param.bitrate = value;
        break;
    case 4:
        if (value > 0)
            default_rc_param.max_bitrate = value;
        break;
    case 5:
        if (value > 0 && value <= 51)
            default_rc_param.qp_constant = value;
        break;
    case 6:
        if (value > 0 && value <= 51)
            default_rc_param.min_quant = value;
        break;
    case 7:
        if (value > 0 && value <= 51)
            default_rc_param.max_quant = value;
        break;
    case 8:
        if (value >= -12 && value < 12)
            default_rc_param.ip_offset = value;
        break;
    case 9:
        if (value > 0)
            default_rc_param.idr_period = value;
        break;
    default:
        break;
    }

    return count;
}

static void h264e_rc_clear_proc(void)
{
    if (rc_param_proc)
        remove_proc_entry(rc_param_proc->name, rc_proc);
    if (rc_dbg_proc)
        remove_proc_entry(rc_dbg_proc->name, rc_proc);
    if (rc_info_proc)
        remove_proc_entry(rc_info_proc->name, rc_proc);
    if (rc_level_proc)
        remove_proc_entry(rc_level_proc->name, rc_proc);
    if (rc_proc)
        remove_proc_entry(H264E_RC_PROC_DIR, NULL);
}

static int h264e_rc_init_proc(void)
{
    rc_proc = create_proc_entry(H264E_RC_PROC_DIR, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (NULL == rc_proc) {
        printk("Error to create rc proc\n");
        goto rc_init_proc_exit;
    }

    rc_level_proc = create_proc_entry("level", S_IRUGO | S_IXUGO, rc_proc);
    if (NULL == rc_level_proc) {
        printk("Error to create %s/level proc\n", H264E_RC_PROC_DIR);
        goto rc_init_proc_exit;
    }
    rc_level_proc->read_proc = (read_proc_t *)proc_rc_log_level_read;
    rc_level_proc->write_proc = (write_proc_t *)proc_rc_log_level_write;
    
    rc_info_proc = create_proc_entry("info", S_IRUGO | S_IXUGO, rc_proc);
    if (NULL == rc_info_proc) {
        printk("Error to create %s/info proc\n", H264E_RC_PROC_DIR);
        goto rc_init_proc_exit;
    }
    rc_info_proc->read_proc = (read_proc_t *)proc_rc_info_read;
    rc_info_proc->write_proc = (write_proc_t *)proc_rc_info_write;

    rc_dbg_proc = create_proc_entry("dbg", S_IRUGO | S_IXUGO, rc_proc);
    if (NULL == rc_dbg_proc) {
        printk("Error to create %s/dbg proc\n", H264E_RC_PROC_DIR);
        goto rc_init_proc_exit;
    }
    rc_dbg_proc->read_proc = (read_proc_t *)proc_rc_dbg_read;
    rc_dbg_proc->write_proc = (write_proc_t *)proc_rc_dbg_write;

    rc_param_proc = create_proc_entry("param", S_IRUGO | S_IXUGO, rc_proc);
    if (NULL == rc_param_proc) {
        printk("Error to create %s/param proc\n", H264E_RC_PROC_DIR);
        goto rc_init_proc_exit;
    }
    rc_param_proc->read_proc = (read_proc_t *)proc_rc_param_read;
    rc_param_proc->write_proc = (write_proc_t *)proc_rc_param_write;
    return 0;
    
rc_init_proc_exit:
    h264e_rc_clear_proc();
    return -EFAULT;
}


struct rc_entity_t favce_rc_entity = {
    rc_init:        &ratecontrol_init,
    rc_clear:       &ratecontrol_exit,
    rc_get_quant:   &ratecontrol_get_quant,
    rc_update:      &ratecontrol_update,
    rc_reset_param: &ratecontrol_reset_param
};

static int __init h264e_rc_init(void)
{
    rc_register(&favce_rc_entity);
    h264e_rc_init_proc();
    h264e_rc_msg(NULL);
    memset((void *)log_rc_param, 0, sizeof(log_rc_param));
    memset((void *)rc_bitrate, 0, sizeof(rc_bitrate));
    return 0;
}

static void __exit h264e_rc_clearnup(void)
{
    rc_deregister();
    h264e_rc_clear_proc();
}

module_init(h264e_rc_init);
module_exit(h264e_rc_clearnup);

MODULE_AUTHOR("Grain Media Corp.");
MODULE_LICENSE("GPL");

