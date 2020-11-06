#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/div64.h>
#include <linux/blkdev.h>
#include <linux/syscalls.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include "mp4e_ratecontrol.h"
#ifdef VG_INTERFACE
    #include "log.h"
#endif

#ifdef VG_INTERFACE
    #define MP4E_RC_PROC_DIR "videograph/mp4e_rc"
#else
    #define MP4E_RC_PROC_DIR "mp4e_rc"
#endif
/*
* 0.0.1: first version
* 0.0.2: update log message
* 0.0.3: 1. fix bug: EVBR compensate (20140117)
*        2. avoid parameter overflow (20140117)
* 0.0.4: add level of bitrate converge (20140124)
* 0.0.5: adjust buffer period by frame rate (20140312)
* 0.0.6: handle divided by zero
* 0.0.7: dump current qp to proc/info
* 0.0.8: add proc to control ECBR delay factor
*/

#define MP4E_RC_VERISON     0x00000800
#define MP4E_RC_MAJOR       ((MP4E_RC_VERISON>>28)&0x0F)    // huge change
#define MP4E_RC_MINOR       ((MP4E_RC_VERISON>>20)&0xFF)    // interface change 
#define MP4E_RC_MINOR2      ((MP4E_RC_VERISON>>8)&0x0FFF)   // functional modified or buf fixed
#define MP4E_RC_BRANCH      (MP4E_RC_VERISON&0xFF)          // branch for customer request

#define MAX_RC_LOG_CHN  128
#define DUMP_CUR_QP

#ifdef VG_INTERFACE
    #define DEBUG_M(level, fmt, args...) { \
        if (mp4_rc_log_level == LOG_DIRECT) \
            printk(fmt, ## args); \
        else if (mp4_rc_log_level >= level) \
            printm(RC_MODULE_NAME, fmt, ## args); }
    #define rc_wrn(fmt, args...) { \
            printk(fmt, ## args); \
            printm(RC_MODULE_NAME, fmt, ## args); }
    #define rc_err(fmt, args...) { \
            printk(fmt, ## args); \
            printm(RC_MODULE_NAME, fmt, ## args); }
#else
    #define DEBUG_M(level, fmt, args...) { \
        if (mp4_rc_log_level >= level) \
            printk(fmt, ## args); }
    #define rc_wrn(fmt, args...) { \
        printk(fmt, ## args); }
    #define rc_err(fmt, args...) { \
        printk(fmt, ## args); }
#endif

#define EVALUATE_BITRATE
#ifdef EVALUATE_BITRATE
struct rc_bitrate_info_t
{
    unsigned int total_byte;
    unsigned int frame_cnt;
    unsigned int fr_base;
    unsigned int fr_incr;
};
#endif

/*************** proc ***************/
struct proc_dir_entry *rc_proc = NULL;
struct proc_dir_entry *rc_level_proc = NULL;
struct proc_dir_entry *rc_info_proc = NULL;
struct proc_dir_entry *rc_dbg_proc = NULL;

int mp4_rc_log_level = LOG_WARNING;
int under_converge_level = 0;
int over_converge_level = 0;
int buffer_period_factor = 3;
static const char rc_mode_name[5][5] = {"", "CBR", "VBR", "ECBR", "EVBR"};
static struct rc_init_param_t log_rc_param[MAX_RC_LOG_CHN];
#ifdef EVALUATE_BITRATE
static struct rc_bitrate_info_t rc_bitrate[MAX_RC_LOG_CHN];
static int bitrate_log_chn = -1;
static int bitrate_period = 5;
#endif
static int ecbr_delay_factor = RC_DELAY_FACTOR;

#ifdef DUMP_CUR_QP
    static int cur_qp[MAX_RC_LOG_CHN];
#endif

extern int mp4e_rc_register(struct rc_entity_t *entity);
extern int mp4e_rc_deregister(void);

static const struct rc_init_param_t default_rc_param = 
{
    rc_mode:        EM_VRC_CBR,
    fbase:          30,
    fincr:          1,
    bitrate:        2000,
    /*
    mb_count:       1350,
    idr_period:     30,
    bframe:         0,
    ip_offset:      2,
    pb_offset:      2,
    qp_step:        1,
    rate_tolerance_fix: 409,
    */
    qp_constant:    10,
    min_quant:      1,
    max_quant:      31,
    max_bitrate:    2000
};

#ifdef EVALUATE_BITRATE
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
            printk("{chn%d} bitrate %d\n", chn, rc_bitrate[chn].total_byte * 8 / bitrate_period);
            rc_bitrate[chn].frame_cnt = 0;
            rc_bitrate[chn].total_byte = 0;
        }
    }
    return 0;
}
#endif

int mp4e_ratecontrol_init(void **pptHandler, struct rc_init_param_t *rc_param)
{
    MP4FRateControl *rc;
    int ret = 0;
    unsigned int buffer_size = RC_BUFFER_SIZE;

    /* 1: EM_VRC_CBR, 2: EM_VRC_VBR, 3: EM_VRC_ECBR, 4: EM_VRC_EVBR */
    // check parameter value
    if (rc_param->rc_mode < EM_VRC_CBR || rc_param->rc_mode > EM_VRC_EVBR) {
        rc_err("mp4e rc_mode(%d) out of range, force default setting\n", rc_param->rc_mode);
        memcpy(rc_param, &default_rc_param, sizeof(struct rc_init_param_t));
    }
    if (0 == rc_param->fbase || 0 == rc_param->fincr) {
        rc_err("invalid frame rate = %d / %d, force 30 fps\n", rc_param->fbase, rc_param->fincr);
        rc_param->fbase = 30;
        rc_param->fincr = 1;
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
        rc_param->max_quant = 31;
    rc_param->min_quant = RC_CLIP3(rc_param->min_quant, 1, 31);
    rc_param->max_quant = RC_CLIP3(rc_param->max_quant, 1, 31);

    if (rc_param->max_quant < rc_param->min_quant) {
        rc_wrn("max_quant(%d) must be larger than min_quant(%d), force default value\n", rc_param->max_quant, rc_param->min_quant);
        rc_param->min_quant = 1;
        rc_param->max_quant = 31;
    }
    
    if (rc_param->qp_constant < rc_param->min_quant || rc_param->qp_constant > rc_param->max_quant) {
        rc_wrn("init quant(%d) is out of min/max quant, force middle value %d\n", 
			rc_param->qp_constant, (rc_param->min_quant + rc_param->max_quant)/2);
        rc_param->qp_constant = (rc_param->min_quant + rc_param->max_quant)/2;
    }

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
        rc_param->min_quant = rc_param->max_quant = rc_param->qp_constant;
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
        rc = kmalloc(sizeof(MP4FRateControl), GFP_ATOMIC);

    ret = MP4FRateControlInit(rc, 
        rc_param->bitrate,
        ecbr_delay_factor,
        RC_AVERAGING_PERIOD,
        buffer_size,
        rc_param->fincr,
        rc_param->fbase,
        rc_param->max_quant,
        rc_param->min_quant,
        rc_param->qp_constant,
        rc_param->max_bitrate,
        3);

    if (ret < 0) {
        rc_err("mp4e rate control init fail\n");
        goto rc_init_fail;
    }
    *pptHandler = rc;
	rc->ch = rc_param->chn; 
#ifdef EVALUATE_BITRATE
    rc_bitrate[rc->ch].frame_cnt = rc_bitrate[rc->ch].total_byte = 0;
    rc_bitrate[rc->ch].fr_base = rc_param->fbase;
    rc_bitrate[rc->ch].fr_incr = rc_param->fincr;
#endif
    if (rc_param->chn < MAX_RC_LOG_CHN)
        memcpy((void *)(&log_rc_param[rc_param->chn]), rc_param, sizeof(struct rc_init_param_t));
    DEBUG_M(LOG_DEBUG, "{chn%d} rc %s, fps: %d/%d, br: %d/%d, qp: %d (%d ~ %d)\n", rc->ch,
        rc_mode_name[rc_param->rc_mode], rc_param->fbase, rc_param->fincr, rc_param->bitrate, rc_param->max_bitrate, 
        rc_param->qp_constant, rc_param->min_quant, rc_param->max_quant);
    return 0;
rc_init_fail:
    if (rc)
        kfree(rc);
    *pptHandler = NULL;
    return ret;
}

int mp4e_ratecontrol_exit(void *ptHandler)
{
    if (ptHandler)
        kfree(ptHandler);
    return 0;
}

int mp4e_ratecontrol_get_quant(void *ptHandler, struct rc_frame_info_t *rc_data)
{
    MP4FRateControl *rc = NULL;
    int qp = 10;
    if (ptHandler) {
        rc = (MP4FRateControl *)ptHandler;
        qp = rc->rtn_quant;
    }
    else {
        rc_err("rate control is not initialized\n");
    }
#ifdef DUMP_CUR_QP
    if (rc && rc->ch < MAX_RC_LOG_CHN)
        cur_qp[rc->ch] = qp;
#endif    
    return qp;
}

int mp4e_ratecontrol_update(void *ptHandler, struct rc_frame_info_t *rc_data)
{
    MP4FRateControl *rc = (MP4FRateControl *)ptHandler;
    if (NULL == rc)
        rc_err("rate control is not initialized\n");
    MP4FRateControlUpdate(rc, 
        rc_data->avg_qp, 
        rc_data->frame_size,
        (EM_SLICE_TYPE_I == rc_data->slice_type ? 1 : 0));
#ifdef EVALUATE_BITRATE
    evaluation_bitrate(rc->ch, rc_data->frame_size);
#endif
    return 0;
}

// ========================= proc =========================

static int proc_rc_log_level_read(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    
    len += sprintf(page+len, "Log level = %d (%d: emergy, %d: error, %d: warning, %d: info, %d: debug)\n", 
        mp4_rc_log_level, LOG_EMERGY, LOG_ERROR, LOG_WARNING, LOG_INFO, LOG_DEBUG);
    *eof = 1;
    return len;
}

static int proc_rc_log_level_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int level;
    sscanf(buffer, "%d", &level);
    mp4_rc_log_level = level;
    printk("\nLog level = %d (%d: emergy, %d: error, %d: warning, %d: info, %d: debug)\n", 
        mp4_rc_log_level, LOG_EMERGY, LOG_ERROR, LOG_WARNING, LOG_INFO, LOG_DEBUG);
    return count;
}

static int proc_rc_info_read(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0, i;
    
    if (MP4E_RC_BRANCH)
        len += sprintf(page+len, "MPEG4 rate control version: %d.%d.%d.%d\n", MP4E_RC_MAJOR, MP4E_RC_MINOR, MP4E_RC_MINOR2, MP4E_RC_BRANCH);
    else
        len += sprintf(page+len, "MPEG4 rate control version: %d.%d.%d\n", MP4E_RC_MAJOR, MP4E_RC_MINOR, MP4E_RC_MINOR2);

#ifdef DUMP_CUR_QP
    len += sprintf(page+len, " chn   mode         fps          bitrate    max.br    init.q  min.q  max.q   qp\n");
    len += sprintf(page+len, "=====  ======  ===============  =========  =========  ======  =====  =====  ====\n");
#else
    len += sprintf(page+len, " chn   mode         fps          bitrate    max.br    init.q  min.q  max.q\n");
    len += sprintf(page+len, "=====  ======  ===============  =========  =========  ======  =====  =====\n");
#endif
    for (i = 0; i < MAX_RC_LOG_CHN; i++) {
        if (log_rc_param[i].rc_mode >= EM_VRC_CBR && log_rc_param[i].rc_mode <= EM_VRC_EVBR) {
        #ifdef DUMP_CUR_QP
            len += sprintf(page+len, " %3d    %4s   %7d/%-7d  %7d    %7d      %2d      %2d     %2d    %d\n",
                i, rc_mode_name[log_rc_param[i].rc_mode], log_rc_param[i].fbase, log_rc_param[i].fincr, 
                log_rc_param[i].bitrate, log_rc_param[i].max_bitrate, 
                log_rc_param[i].qp_constant, log_rc_param[i].min_quant, log_rc_param[i].max_quant, cur_qp[i]);
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

#ifdef EVALUATE_BITRATE
    len += sprintf(page+len, " 1: channel of dump bitrate, -1 not dump (%d)\n", bitrate_log_chn);
    len += sprintf(page+len, " 2: bitrate period (%d)\n", bitrate_period);
#endif
    len += sprintf(page+len, " 3: ecbr delay factor (%d)\n", ecbr_delay_factor);
    len += sprintf(page+len, "10: overflow converge level (%d) (0~5)\n", over_converge_level);
    len += sprintf(page+len, "11: underflow converge level (%d) (0~5)\n", under_converge_level);
    len += sprintf(page+len, "12: buffer period factor (%d)\n", buffer_period_factor);

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
#ifdef EVALUATE_BITRATE
    case 1:
        if (value < 0 || value >= MAX_RC_LOG_CHN)
            bitrate_log_chn = -1;
        else {
            bitrate_log_chn = value;
            rc_bitrate[bitrate_log_chn].total_byte = 0;
            rc_bitrate[bitrate_log_chn].frame_cnt = 0;
        }
        break;
    case 2:
        if (value > 0)
            bitrate_period = value;
        break;
#endif
    case 3:
        if (value > 0)
            ecbr_delay_factor = value;
        else
            ecbr_delay_factor = RC_DELAY_FACTOR;
        break;
    case 10:
        if (value >= 0 && value <= 5)
            over_converge_level = value;
        break;
    case 11:
        if (value >= 0 && value <= 5)
            under_converge_level = value;
        break;
    case 12:
        if (value >= 0 && value < 10)
            buffer_period_factor = value;
        break;
    default:
        break;
    }

    return count;
}

static void mp4e_rc_clear_proc(void)
{
    if (rc_info_proc)
        remove_proc_entry(rc_info_proc->name, rc_proc);
    if (rc_level_proc)
        remove_proc_entry(rc_level_proc->name, rc_proc);
    if (rc_dbg_proc)
        remove_proc_entry(rc_dbg_proc->name, rc_proc);
    if (rc_proc)
        remove_proc_entry(MP4E_RC_PROC_DIR, NULL);
}

static int mp4e_rc_init_proc(void)
{
    rc_proc = create_proc_entry(MP4E_RC_PROC_DIR, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (NULL == rc_proc) {
        printk("Error to create rc proc\n");
        goto rc_init_proc_exit;
    }

    rc_level_proc = create_proc_entry("level", S_IRUGO | S_IXUGO, rc_proc);
    if (NULL == rc_level_proc) {
        printk("Error to create %s/level proc\n", MP4E_RC_PROC_DIR);
        goto rc_init_proc_exit;
    }
    rc_level_proc->read_proc = (read_proc_t *)proc_rc_log_level_read;
    rc_level_proc->write_proc = (write_proc_t *)proc_rc_log_level_write;
    
    rc_info_proc = create_proc_entry("info", S_IRUGO | S_IXUGO, rc_proc);
    if (NULL == rc_info_proc) {
        printk("Error to create %s/info proc\n", MP4E_RC_PROC_DIR);
        goto rc_init_proc_exit;
    }
    rc_info_proc->read_proc = (read_proc_t *)proc_rc_info_read;
    rc_info_proc->write_proc = (write_proc_t *)proc_rc_info_write;

    rc_dbg_proc = create_proc_entry("dbg", S_IRUGO | S_IXUGO, rc_proc);
    if (NULL == rc_dbg_proc) {
        printk("Error to create %s/dbg proc\n", MP4E_RC_PROC_DIR);
        goto rc_init_proc_exit;
    }
    rc_dbg_proc->read_proc = (read_proc_t *)proc_rc_dbg_read;
    rc_dbg_proc->write_proc = (write_proc_t *)proc_rc_dbg_write;
    return 0;
    
rc_init_proc_exit:
    mp4e_rc_clear_proc();
    return -EFAULT;
}


struct rc_entity_t mp4e_rc_entity = {
    rc_init:        &mp4e_ratecontrol_init,
    rc_clear:       &mp4e_ratecontrol_exit,
    rc_get_quant:   &mp4e_ratecontrol_get_quant,
    rc_update:      &mp4e_ratecontrol_update,
};

static int __init mp4e_rc_init(void)
{
    mp4e_rc_register(&mp4e_rc_entity);
    mp4e_rc_init_proc();
    printk("MPEG4 rate control version: %d.%d.%d", MP4E_RC_MAJOR, MP4E_RC_MINOR, MP4E_RC_MINOR2);
    if (MP4E_RC_BRANCH)
        printk(".%d", MP4E_RC_BRANCH);
    printk(", built @ %s %s\n", __DATE__, __TIME__);
    memset((void *)log_rc_param, 0, sizeof(log_rc_param));
#ifdef EVALUATE_BITRATE
    memset((void *)rc_bitrate, 0, sizeof(rc_bitrate));
#endif
    return 0;
}

static void __exit mp4e_rc_clearnup(void)
{
    mp4e_rc_deregister();
    mp4e_rc_clear_proc();
}

module_init(mp4e_rc_init);
module_exit(mp4e_rc_clearnup);

MODULE_AUTHOR("Grain Media Corp.");
MODULE_LICENSE("GPL");

