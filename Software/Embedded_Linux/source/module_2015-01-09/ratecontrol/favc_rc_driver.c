#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/div64.h>
#include <linux/blkdev.h>
#include <linux/syscalls.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include "h264e_ratecontrol.h"
#ifdef VG_INTERFACE
    #include "log.h"
#endif

#ifdef VG_INTERFACE
    #define H264E_RC_PROC_DIR "videograph/favce_rc"
#else
    #define H264E_RC_PROC_DIR "favce_rc"
#endif

/*
0.0.1: first version
0.0.2: update frame rate computing to handle value overflow (2013/05/13)
0.0.3: check divide by zero (2013/05/24)
0.0.4: update EVBR parameter (use max bitrate)
0.0.5: add increasing qp function to handle bitstream overflow
0.0.6: modify input parameter when parameter is illegal (2013/07/22)
0.0.7: update debug message (2013/08/05)
*/

#define H264E_RC_VERSION    0x00000700
#define H264E_RC_MAJOR      ((H264E_RC_VERSION>>28)&0x0F)
#define H264E_RC_MINOR      ((H264E_RC_VERSION>>20)&0xFF)
#define H264E_RC_MINOR2     ((H264E_RC_VERSION>>8)&0x0FFF)
#define H264E_RC_BRANCH     (H264E_RC_VERSION&0xFF)
//#define iClip3(val, low, high)  ((val)>(high)?(high):((val)<(low)?(low):(val)))
#define MAX_RC_LOG_CHN  128

#ifdef VG_INTERFACE
#define DEBUG_M(level, fmt, args...) { \
    if (log_level == LOG_DIRECT) \
        printk(fmt, ## args); \
    else if (log_level >= level) \
        printm(MODULE_NAME, fmt, ## args); }
#define rc_err(fmt, args...) { \
        printk(fmt, ## args); \
        printm(MODULE_NAME, fmt, ## args); }
#else
#define DEBUG_M(level, fmt, args...) { \
    if (log_level >= level) \
        printk(fmt, ## args); }
#define rc_err(fmt, args...) { \
    printk(fmt, ## args); }
#endif

struct proc_dir_entry *rc_proc = NULL;
struct proc_dir_entry *rc_level_proc = NULL;
struct proc_dir_entry *rc_info_proc = NULL;
struct proc_dir_entry *rc_dbg_proc = NULL;
//struct proc_dir_entry *favc_rc_proc = NULL;

// control parameter
static int dump_qp_value = 0;
unsigned int overflow_bitrate_bound = 4506;     // 1.1
unsigned int underflow_bitrate_bound = 3686;    // 0.9
unsigned int rate_tolerance = 409;  // 0.1

// Tuba 20131026: log rc parameter
struct rc_init_param_t log_rc_param[MAX_RC_LOG_CHN];
//struct rc_init_param_t *p_rc_param;
static const char rc_mode_name[5][5] = {"", "CBR", "VBR", "ECBR", "EVBR"};

extern int rc_register(struct rc_entity_t *entity);
extern int rc_deregister(void);
extern int log_level;

static const struct rc_init_param_t default_rc_param = 
{
    rc_mode:        EM_VRC_CBR,
    fbase:          30,
    fincr:          1,
    bitrate:        2000,
    mb_count:       1350,
    idr_period:     30,
    bframe:         0,
    ip_offset:      2,
    pb_offset:      2,
    qp_constant:    26,
    qp_step:        1,
    min_quant:      14,
    max_quant:      51,
    rate_tolerance_fix: 409,
    max_bitrate:    2000
};

int favce_ratecontrol_init(void **pptHandler, struct rc_init_param_t *rc_param)
{
    struct rc_private_data_t *rc = NULL;
    
    // check parameter value
    if (rc_param->rc_mode < EM_VRC_CBR || rc_param->rc_mode > EM_VRC_EVBR) {
        rc_err("h264e rc_mode(%d) out of range, force default setting\n", rc_param->rc_mode);
        memcpy(rc_param, &default_rc_param, sizeof(struct rc_init_param_t));
        //goto favce_rc_init_fail;
    }
    if (0 == rc_param->fbase || 0 == rc_param->fincr) {
        rc_err("invalid frame rate = %d / %d, force 30 fps\n", rc_param->fbase, rc_param->fincr);
        rc_param->fbase = 30;
        rc_param->fincr = 1;
    }
    if (rc_param->rc_mode == EM_VRC_ECBR)
        rc_param->rc_mode = EM_VRC_CBR;
    if (rc_param->rc_mode == EM_VRC_EVBR) {
        if (0 == rc_param->max_bitrate) {
            if (0 == rc_param->bitrate) {
                rc_err("EVBR, but not set max bitrate, force 2M\n");
                rc_param->max_bitrate = rc_param->bitrate = 2000;
            }
            else {
                rc_err("EVBR not set max bitrate, use target bitrate to be max bitrate\n");
                rc_param->max_bitrate = rc_param->bitrate;
            }
        }
        else
            rc_param->bitrate = rc_param->max_bitrate;
    }
    if (rc_param->rc_mode != EM_VRC_VBR && 0 == rc_param->bitrate) {
        rc_err("CBR bitrate can not be zero, force 2M\n");
        //rc_param->rc_mode = EM_VRC_VBR;
        rc_param->bitrate = 2000;
    }
    rc_param->min_quant = RC_CLIP3(rc_param->min_quant, 1, 51);
    rc_param->max_quant = RC_CLIP3(rc_param->max_quant, 1, 51);
    
    if (rc_param->max_quant < rc_param->min_quant) {
        rc_err("max_quant(%d) must be larger than min_quant(%d), force default value\n", rc_param->max_quant, rc_param->min_quant);
        rc_param->min_quant = 14;
        rc_param->max_quant = 51;
        //goto favce_rc_init_fail;
    }
    
    if (rc_param->qp_constant < rc_param->min_quant || rc_param->qp_constant > rc_param->max_quant) {
        rc_err("init quant(%d) is out of min/max quant, force middle value %d\n", 
			rc_param->qp_constant, (rc_param->min_quant + rc_param->max_quant)/2);
        rc_param->qp_constant = (rc_param->min_quant + rc_param->max_quant)/2;
        //goto favce_rc_init_fail;
    }
    rc_param->rate_tolerance_fix = rate_tolerance;

    DEBUG_M(LOG_INFO, "[mode %d] bitrate: %d, fps: %d/%d, qp: %d (%d, %d)\n", rc_param->rc_mode, rc_param->bitrate,
        rc_param->fbase, rc_param->fincr, rc_param->qp_constant, rc_param->max_quant, rc_param->min_quant);

    if (*pptHandler)
        rc = *pptHandler;
    else
        rc = kmalloc(sizeof(struct rc_private_data_t), GFP_ATOMIC);
    
    if (rc_init_fix(rc, rc_param) < 0) {
        rc_err("h264e rate control init fail\n");
        goto favce_rc_init_fail;
    }
    *pptHandler = rc;
    // Tuba 20131026: log rc parameter
    if (rc_param->chn < MAX_RC_LOG_CHN)
        memcpy((void *)(&log_rc_param[rc_param->chn]), rc_param, sizeof(struct rc_init_param_t));
    return 0;
favce_rc_init_fail:
    if (rc)
        kfree(rc);
    *pptHandler = NULL;
    return -1;
}

int favce_ratecontrol_exit(void *ptHandler)
{
    rc_exit_fix(ptHandler);
    if (ptHandler)
        kfree(ptHandler);
    return 0;
}

int favce_ratecontrol_get_quant(void *ptHandler, struct rc_frame_info_t *rc_data)
{
    int qp;
    if (NULL == ptHandler) {
        printk("get_quant: rate control is not initialization\n");
        return -1;
    }
    qp = rc_start_fix((struct rc_private_data_t *)ptHandler, rc_data);
    if (dump_qp_value)
        printk("qp = %d\n", qp);
    return qp;
    //return rc_start_fix((struct rc_private_data_t *)ptHandler, rc_data);
}

int favce_ratecontrol_update(void *ptHandler, struct rc_frame_info_t *rc_data)
{
    if (NULL == ptHandler)  {
        printk("update: rate control is not initialization\n");
        return -1;
    }
    return rc_end_fix((struct rc_private_data_t *)ptHandler, rc_data);
}

int favce_ratecontrol_reset_param(void *ptHandler, unsigned int inc_qp)
{
    if (NULL == ptHandler) {
        printk("reset: rate control is not initialization\n");
        return -1;
    }
    return rc_increase_qp_fix((struct rc_private_data_t *)ptHandler, inc_qp);
}


// ========================= proc =========================
static int proc_log_level_read(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    
    len += sprintf(page+len, "Log level = %d (%d: emergy, %d: error, %d: warning, %d: info, %d: debug)\n", 
        log_level, LOG_EMERGY, LOG_ERROR, LOG_WARNING, LOG_INFO, LOG_DEBUG);
    *eof = 1;
    return len;
}

static int proc_log_level_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int level;
    sscanf(buffer, "%d", &level);
    log_level = level;
    printk("\nLog level = %d (%d: emergy, %d: error, %d: warning, %d: info, %d: debug)\n", 
        log_level, LOG_EMERGY, LOG_ERROR, LOG_WARNING, LOG_INFO, LOG_DEBUG);
    return count;
}

static int proc_info_read(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0, i;
    
    if (H264E_RC_BRANCH)
        len += sprintf(page+len, "H264 rate control version: %d.%d.%d.%d\n", H264E_RC_MAJOR, H264E_RC_MINOR, H264E_RC_MINOR2, H264E_RC_BRANCH);
    else
        len += sprintf(page+len, "H264 rate control version: %d.%d.%d\n", H264E_RC_MAJOR, H264E_RC_MINOR, H264E_RC_MINOR2);

    len += sprintf(page+len, " mode         fps         bitrate   init.q  min.q  max.q\n");
    len += sprintf(page+len, "======  ===============  =========  ======  =====  =====\n");
    for (i = 0; i < MAX_RC_LOG_CHN; i++) {
        if (log_rc_param[i].rc_mode >= EM_VRC_CBR && log_rc_param[i].rc_mode <= EM_VRC_EVBR) {
            len += sprintf(page+len, " %4s   %7d/%-7d  %7d      %2d      %2d     %2d\n",
                rc_mode_name[log_rc_param[i].rc_mode], log_rc_param[i].fbase, log_rc_param[i].fincr, log_rc_param[i].bitrate, 
                log_rc_param[i].qp_constant, log_rc_param[i].min_quant, log_rc_param[i].max_quant);
        }
    }
    *eof = 0;
    return len;
}

static int proc_info_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    return count;
}

static int proc_dbg_read(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;

    len += sprintf(page+len, "usage: \"echo [id] [valule] > /proc/videograph/favce_rc/dbg\"\n");
    len += sprintf(page+len, "id  value     readme\n");
    len += sprintf(page+len, "==  =====  ==============\n");
    len += sprintf(page+len, " 1   %3d   1: dump qp value\n", dump_qp_value);
    len += sprintf(page+len, "10  %4d   overflow bitarte bound\n", overflow_bitrate_bound);
    len += sprintf(page+len, "11  %4d   underflow bitrate bound\n", underflow_bitrate_bound);
    len += sprintf(page+len, "12  %4d   rate tolerance\n", rate_tolerance);
    *eof = 0;
    return len;
}

static int proc_dbg_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int cmd_idx, value;
    sscanf(buffer, "%d %d", &cmd_idx, &value);
    switch (cmd_idx) {
    case 1:
        if (1 == value)
            dump_qp_value = 1;
        else
            dump_qp_value = 0;
        break;
    case 10:
        if (value > 4096)
            overflow_bitrate_bound = value;
        else
            printk("overflow bitrate bound must be larger than 4096\n");
        break;
    case 11:
        if (value < 4096)
            underflow_bitrate_bound = value;
        else
            printk("underflow bitrate bound must be lower than 4096\n");        
        break;
    case 12:
        if (value > 41)
            rate_tolerance = value;
        break;
    default:
        break;
    }            
    return count;
}


static void rc_clear_proc(void)
{
    if (rc_info_proc)
        remove_proc_entry(rc_info_proc->name, rc_proc);
    if (rc_level_proc)
        remove_proc_entry(rc_level_proc->name, rc_proc);
    if (rc_dbg_proc)
        remove_proc_entry(rc_dbg_proc->name, rc_proc);
    if (rc_proc)
        remove_proc_entry(H264E_RC_PROC_DIR, NULL);
}

static int rc_init_proc(void)
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
    rc_level_proc->read_proc = (read_proc_t *)proc_log_level_read;
    rc_level_proc->write_proc = (write_proc_t *)proc_log_level_write;
    
    rc_info_proc = create_proc_entry("info", S_IRUGO | S_IXUGO, rc_proc);
    if (NULL == rc_info_proc) {
        printk("Error to create %s/info proc\n", H264E_RC_PROC_DIR);
        goto rc_init_proc_exit;
    }
    rc_info_proc->read_proc = (read_proc_t *)proc_info_read;
    rc_info_proc->write_proc = (write_proc_t *)proc_info_write;

    rc_dbg_proc = create_proc_entry("dbg", S_IRUGO | S_IXUGO, rc_proc);
    if (NULL == rc_dbg_proc) {
        printk("Error to create %s/dbg proc\n", H264E_RC_PROC_DIR);
        goto rc_init_proc_exit;
    }
    rc_dbg_proc->read_proc = (read_proc_t *)proc_dbg_read;
    rc_dbg_proc->write_proc = (write_proc_t *)proc_dbg_write;
    
    return 0;
    
rc_init_proc_exit:
    rc_clear_proc();
    return -EFAULT;
}

struct rc_entity_t favc_rc_entity = {
    rc_init:        &favce_ratecontrol_init,
    rc_clear:       &favce_ratecontrol_exit,
    rc_get_quant:   &favce_ratecontrol_get_quant,
    rc_update:      &favce_ratecontrol_update,
    rc_reset_param: &favce_ratecontrol_reset_param
};


static int __init h264e_rc_init(void)
{
    rc_register(&favc_rc_entity);
    rc_init_proc();
    printk("H264 rate control version: %d.%d.%d", H264E_RC_MAJOR, H264E_RC_MINOR, H264E_RC_MINOR2);
    if (H264E_RC_BRANCH)
        printk(".%d", H264E_RC_BRANCH);
    printk(", built @ %s %s\n", __DATE__, __TIME__);

    memset((void *)log_rc_param, 0, sizeof(log_rc_param));
    //memset(p_rc_param, 0, sizeof(p_rc_param));
    return 0;
}

static void __exit h264e_rc_clearnup(void)
{
    rc_deregister();
    rc_clear_proc();
}

module_init(h264e_rc_init);
module_exit(h264e_rc_clearnup);

MODULE_AUTHOR("Grain Media Corp.");
MODULE_LICENSE("GPL");

