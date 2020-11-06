/**
 * @file vcap_vg.c
 * VideoGraph interface for vcap300
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.52 $
 * $Date: 2014/12/22 03:46:04 $
 *
 * ChangeLog:
 *  $Log: vcap_vg.c,v $
 *  Revision 1.52  2014/12/22 03:46:04  jerson_l
 *  1. support videograph job property ID_DST2_BUF_OFFSET.
 *
 *  Revision 1.51  2014/11/28 08:52:17  jerson_l
 *  1. support frame 60I feature.
 *
 *  Revision 1.50  2014/11/27 01:49:22  jerson_l
 *  1. remove duplicate src_interlace property in property map
 *
 *  Revision 1.49  2014/11/11 05:37:24  jerson_l
 *  1. support group job pending mechanism
 *
 *  Revision 1.48  2014/11/06 07:14:57  jerson_l
 *  1. fix vcap_vg_putjob procedure semaphore not release problem when error exit
 *
 *  Revision 1.47  2014/09/05 05:56:27  jerson_l
 *  1. display callback event counter in info proc node for debugging
 *
 *  Revision 1.46  2014/09/03 08:15:33  jerson_l
 *  1. support to use kthread for videograph job callback
 *
 *  Revision 1.45  2014/05/14 06:23:48  jerson_l
 *  1. change job_item buffer allocate procedure to kmem_cache_alloc
 *
 *  Revision 1.44  2014/05/13 07:29:32  jerson_l
 *  1. fix vg callback scheduler not wake up problem
 *
 *  Revision 1.43  2014/05/13 03:59:47  jerson_l
 *  1. change default callback period to 0 ms
 *
 *  Revision 1.42  2014/04/02 03:40:06  jerson_l
 *  1. use gm_jiffies as timestamp value
 *
 *  Revision 1.41  2014/03/25 07:00:03  jerson_l
 *  1. add buf_clean proc node to control output property when job callback fail
 *
 *  Revision 1.40  2014/03/11 13:49:43  jerson_l
 *  1. support ID_BUF_CLEAN property
 *
 *  Revision 1.39  2014/01/24 09:58:26  schumy_c
 *  Use jiffies_64 as timestamp.
 *
 *  Revision 1.38  2014/01/24 09:53:28  schumy_c
 *  Use jiffies_64 as timestamp.
 *
 *  Revision 1.37  2014/01/24 09:38:41  schumy_c
 *  Use jiffies_64 as timestamp.
 *
 *  Revision 1.36  2014/01/24 04:08:46  jerson_l
 *  1. add tamper alarm notify support
 *
 *  Revision 1.35  2014/01/22 04:15:30  jerson_l
 *  1. remove panic when putjob fail
 *
 *  Revision 1.34  2014/01/14 07:43:18  jerson_l
 *  1. support skip sub-job in 2frames_2frames buffer format
 *
 *  Revision 1.33  2013/12/06 08:52:39  jerson_l
 *  1. add log message for job trigger fail
 *
 *  Revision 1.32  2013/12/04 02:34:29  jerson_l
 *  1. fix job root_time=0 problem
 *
 *  Revision 1.31  2013/11/29 03:47:03  jerson_l
 *  1. fix dst_fmt not display out problem
 *
 *  Revision 1.30  2013/11/20 08:57:56  jerson_l
 *  1. add new vg notify type
 *
 *  Revision 1.29  2013/11/06 01:39:47  jerson_l
 *  1. support 2Frame mode as progressive top/bottom format
 *
 *  Revision 1.28  2013/10/15 07:37:48  jerson_l
 *  1. support new job format TYPE_YUV422_FRAME_FRAME
 *
 *  Revision 1.27  2013/10/14 05:29:18  jerson_l
 *  1. support SRC_TYPE output property.
 *
 *  Revision 1.26  2013/10/01 06:15:09  jerson_l
 *  1. add buffer start address aligment check
 *
 *  Revision 1.25  2013/09/25 03:32:01  jerson_l
 *  1. move job callback timestamp to callback api
 *
 *  Revision 1.24  2013/09/17 02:18:10  jerson_l
 *  1. disable sub-frame auto_aspect_ratio
 *
 *  Revision 1.23  2013/09/13 07:13:09  jerson_l
 *  1. support frame_2frames job buffer type
 *
 *  Revision 1.22  2013/09/11 12:45:23  jerson_l
 *  1. add buffer boundary check to prevent image data write to illegal region
 *
 *  Revision 1.21  2013/09/05 02:34:02  jerson_l
 *  1. add dynamic kernel memoey allocate counter for debug monitor
 *
 *  Revision 1.20  2013/08/28 01:59:37  ivan
 *  update from 3frames to 4frames
 *
 *  Revision 1.19  2013/08/23 08:54:07  jerson_l
 *  1. support 2frame_frame buffer count to specify dst2 output buffer address and size
 *
 *  Revision 1.18  2013/08/13 07:33:35  jerson_l
 *  1. support DST2_XY, DST2_DIM, DST2_CROP_XY, DST2_CROP_DIM property
 *
 *  Revision 1.17  2013/07/08 03:45:57  jerson_l
 *  1. support auto_border property
 *  2. support 2frame_frame buffer format
 *
 *  Revision 1.16  2013/06/04 11:24:42  jerson_l
 *  1. support auto_aspect_ratio property
 *
 *  Revision 1.15  2013/05/27 05:52:30  jerson_l
 *  1. support dst_crop_xy, dst_crop_dim property
 *
 *  Revision 1.14  2013/05/07 02:43:42  jerson_l
 *  1. modify videograph property display layout
 *
 *  Revision 1.13  2013/04/08 12:12:06  jerson_l
 *  1. remove v_flip & h_flip property
 *
 *  Revision 1.12  2013/03/29 03:40:31  jerson_l
 *  1. move vg_postprocess and vg_set_out_property to callback scheduler to improve ISR latency
 *
 *  Revision 1.11  2013/03/18 05:43:08  jerson_l
 *  1. remove dma_ch property support
 *  2. add dvr_feature property for setup liveview/record capture dma channel
 *  3. add notify api for erro notfiy of capture
 *
 *  Revision 1.10  2013/03/11 08:06:44  jerson_l
 *  1. add cap_feature property, bit#0 for P2I
 *  2. fix frame rate contorl incorrect issue in 2 minors at the same engine
 *
 *  Revision 1.9  2013/01/02 07:29:57  jerson_l
 *  1. display job version into log message
 *
 *  Revision 1.8  2012/12/13 05:20:59  jerson_l
 *  1. fix vg semaphore resource protection problem
 *
 *  Revision 1.7  2012/12/11 09:27:19  jerson_l
 *  1. use semaphore to protect resource
 *  2. add error log for videograph panic debugging
 *
 *  Revision 1.6  2012/12/06 12:09:46  jerson_l
 *  1. add engine and minor job list critical section protection
 *
 *  Revision 1.5  2012/12/03 08:05:02  jerson_l
 *  1. modify engine/minor expression to match videograph definition
 *
 *  Revision 1.4  2012/11/19 08:30:45  jerson_l
 *  1. use semaphone to replace interrupt lock.
 *
 *  Revision 1.3  2012/11/01 06:23:11  waynewei
 *  1.sync with videograph_20121101
 *  2.fixed for build fail due to include path
 *
 *  Revision 1.2  2012/10/24 06:47:51  jerson_l
 *
 *  switch file to unix format
 *
 *  Revision 1.1.1.1  2012/10/23 08:16:31  jerson_l
 *  import vcap300 driver
 *
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <asm/uaccess.h>

#include "video_entity.h"
#include "vcap_vg.h"
#include "vcap_dbg.h"
#include "vcap_log.h"

#if (HZ == 1000)
    #define get_gm_jiffies()            (jiffies_64)
#else
    #include <mach/gm_jiffies.h>
    #define jiffies_to_msecs(time)      (time)
#endif

extern int vcap_dev_putjob(void *param);
extern int vcap_dev_stop(void *param, int ch, int path);
extern int vcap_dev_get_vi_mode(void *param, int ch);

#define VCAP_VG_CALLBACK_PERIOD     0   ///< 0 ms
#define VCAP_VG_UTILIZATION_PERIOD  5   ///< 5 seconds

struct vcap_vg_property_t {
    int  ch:8;
    int  id:24;
#define VCAP_VG_MAX_NAME    50
    char name[VCAP_VG_MAX_NAME];
};

struct vcap_vg_property_rec_t {
    int                     job_id;
#define VCAP_VG_MAX_RECORD  MAX_PROPERTYS
    struct video_property_t property[VCAP_VG_MAX_RECORD];
};

struct vcap_vg_perf_t {
    unsigned int prev_end;      ///< previous job finish time(jiffes)
    unsigned int times;         ///< job work time(jiffies)
    unsigned int util_start;
    unsigned int util_record;
};

struct vcap_vg_proc_t {
    struct proc_dir_entry   *root;
    struct proc_dir_entry   *info;
    struct proc_dir_entry   *cb_period;
    struct proc_dir_entry   *property;
    struct proc_dir_entry   *job;
    struct proc_dir_entry   *util;
    struct proc_dir_entry   *log_filter;
    struct proc_dir_entry   *log_level;
    struct proc_dir_entry   *buf_clean;
    struct proc_dir_entry   *group_info;
    unsigned int            engine;
    unsigned int            minor;
};

struct vcap_vg_info_t {
    int                             index;
    char                            name[32];               ///< vg info name
    spinlock_t                      lock;                   ///< vg lock
    struct semaphore                sema_lock;              ///< vg semaphore lock
    struct video_entity_t           entity;                 ///< video entity of videograph
    struct kmem_cache               *job_cache;             ///< job cache buffer handler
    struct workqueue_struct         *workq;                 ///< job callback workqueue
    struct delayed_work             callback_work;          ///< job callback work
    unsigned int                    callback_period;        ///< job callback period(tick)
    struct task_struct              *callback_thread;       ///< job callback thread
    wait_queue_head_t               callback_event_wait_q;  ///< job callback thread event wait queue
    atomic_t                        callback_wakeup_event;  ///< job callback thread wakeup event
    struct list_head                *engine_head;           ///< engine job list head
    struct list_head                *minor_head;            ///< minor job list head
    struct list_head                grp_pool_head;          ///< group pool head
    unsigned int                    *fd;                    ///< record fd
    unsigned int                    *sub_fd;                ///< record sub_fd
    struct vcap_vg_property_rec_t   *curr_property;         ///< property of current running job
    struct vcap_vg_perf_t           *perf;                  ///< performance statistic data
    unsigned int                    perf_util_period;       ///< performance utilization period(second)
    struct vcap_vg_proc_t           proc;                   ///< proc system
    int                             malloc_cnt;             ///< kernel memory alloc count
    int                             grp_pool_cnt;           ///< group pool count
    void                            *private;               ///< pointer to vcap_dev_info_t
};

static struct vcap_vg_info_t *vcap_vg_info[VCAP_VG_ENTITY_MAX] = {[0 ... (VCAP_VG_ENTITY_MAX - 1)] = NULL};

enum property_id {
    ID_MD_ENB=(MAX_WELL_KNOWN_ID_NUM+1), //start from 100
    ID_MD_XY_START,
    ID_MD_XY_SIZE,
    ID_MD_XY_NUM,
    ID_CAP_FEATURE,

    ID_COUNT,
    ID_MAX=(MAX_PROCESS_PROPERTYS+MAX_WELL_KNOWN_ID_NUM+1),
};

struct vcap_vg_property_t property_map[] = {
    {0, ID_SRC_FMT,            "src_fmt"},
    {0, ID_SRC_XY,             "src_xy"},
    {0, ID_SRC_BG_DIM,         "src_bg_dim"},
    {0, ID_SRC_DIM,            "src_dim"},
    {0, ID_SRC_INTERLACE,      "src_interlace"},        ///< 0:progress, 1:interlace
    {0, ID_SRC_TYPE,           "src_type"},             ///< 0:generic,  1:ISP
    {0, ID_DST_FMT,            "dst_fmt"},
    {0, ID_DST_XY,             "dst_xy"},
    {0, ID_DST_BG_DIM,         "dst_bg_dim"},
    {0, ID_DST_DIM,            "dst_dim"},
    {0, ID_DST_CROP_XY,        "dst_crop_xy"},
    {0, ID_DST_CROP_DIM,       "dst_crop_dim"},
    {0, ID_DST2_CROP_XY,       "dst2_crop_xy"},
    {0, ID_DST2_CROP_DIM,      "dst2_crop_dim"},
    {0, ID_DST2_FD,            "dst2_fd"},
    {0, ID_DST2_BG_DIM,        "dst2_bg_dim"},
    {0, ID_DST2_XY,            "dst2_xy"},
    {0, ID_DST2_DIM,           "dst2_dim"},
    {0, ID_DST2_BUF_OFFSET,    "dst2_buf_offset"},
    {0, ID_TARGET_FRAME_RATE,  "target_frame_rate"},    ///< decide frame rate by AP with target_frame_rate parameter
    {0, ID_FPS_RATIO,          "fps_ratio"},            ///< decide frame rate by AP with fps_ratio parameter
    {0, ID_RUNTIME_FRAME_RATE, "runtime_frame_rate"},   ///< target runtime frame rate
    {0, ID_SRC_FRAME_RATE,     "src_frame_rate"},       ///< source frame rate
    {0, ID_DVR_FEATURE,        "dvr_feature"},          ///< dvr feature 0:liveview 1:recording
    {0, ID_AUTO_ASPECT_RATIO,  "auto_aspect_ratio"},
    {0, ID_AUTO_BORDER,        "auto_border"},
    {0, ID_BUF_CLEAN,          "buf_clean"},
    {0, ID_SPEICIAL_FUNC,      "special_func"},         ///< [0]: 1frame_60I
    {0, ID_SOURCE_FIELD,       "source_field"},         ///< 0: top field, 1: bottom field

    {0, ID_CAP_FEATURE,        "cap_feature"},          ///< capture feature, bit[0]: P2I
    {0, ID_MD_ENB,             "md_enb"},               ///< MD 0: disable 1:enable
    {0, ID_MD_XY_START,        "md_xy_start"},          ///< MD X, Y start position [31:16]=> X start, [15:0]=> Y start
    {0, ID_MD_XY_SIZE,         "md_xy_size"},           ///< MD X, Y window size [31:16]=> X size, [15:0]=> Y size
    {0, ID_MD_XY_NUM,          "md_xy_num"},            ///< MD X, Y window number [31:16]=> X number, [15:0]=> Y number
};

/* log & debug message */
#define VCAP_VG_LOG_MAX_FILTER  5
#define VCAP_VG_LOG_ERROR       0
#define VCAP_VG_LOG_WARNING     1
#define VCAP_VG_LOG_QUIET       2

static u32 vcap_vg_log_level = VCAP_VG_LOG_ERROR;
static u32 vcap_vg_buf_clean = 1;                   ///< output buf_clean property when job status fail, 0:disable 1:enable
static int vcap_vg_grp_run_mode = VCAP_VG_GRP_RUN_MODE_NORMAL;   ///< group job running mode
static int vcap_vg_grp_run_ver_thres = 0;                        ///< old version job count threshold
static int vcap_vg_log_inc_filter[VCAP_VG_LOG_MAX_FILTER] = {[0 ... (VCAP_VG_LOG_MAX_FILTER - 1)] = -1};
static int vcap_vg_log_exc_filter[VCAP_VG_LOG_MAX_FILTER] = {[0 ... (VCAP_VG_LOG_MAX_FILTER - 1)] = -1};

static inline int is_print(int engine, int minor)
{
    int i;

    if (vcap_vg_log_inc_filter[0] >= 0) {
        for (i=0; i<VCAP_VG_LOG_MAX_FILTER; i++)
            if (vcap_vg_log_inc_filter[i] == ENTITY_FD(0, engine, minor))
                return 1;
    }

    if (vcap_vg_log_exc_filter[0] >= 0) {
        for (i=0; i<VCAP_VG_LOG_MAX_FILTER; i++)
            if (vcap_vg_log_exc_filter[i] == ENTITY_FD(0, engine, minor))
                return 0;
    }
    return 1;
}

#define VCAP_VG_LOG(level, engine, minor, fmt, args...) do {                                                                        \
                                                            if((vcap_vg_log_level >= level) && is_print((int)engine, (int)minor)) { \
                                                                vcap_log(fmt, ## args);                                             \
                                                                if(level == VCAP_VG_LOG_ERROR)                                      \
                                                                    vcap_err(fmt, ## args);                                         \
                                                                else if(level == VCAP_VG_LOG_WARNING)                               \
                                                                    vcap_warn(fmt, ## args);                                        \
                                                            }                                                                       \
                                                        } while(0)

static void *vcap_vg_job_item_alloc(struct vcap_vg_info_t *pvg_info)
{
    void *job_item;

    if(in_interrupt() || in_atomic())
        job_item = kmem_cache_alloc(pvg_info->job_cache, GFP_ATOMIC);
    else
        job_item = kmem_cache_alloc(pvg_info->job_cache, GFP_KERNEL);

    if(job_item)
        pvg_info->malloc_cnt++;

    return job_item;
}

static void vcap_vg_job_item_free(struct vcap_vg_info_t *pvg_info, void *job)
{
    kmem_cache_free(pvg_info->job_cache, job);
    pvg_info->malloc_cnt--;
}

static void *vcap_vg_group_pool_alloc(struct vcap_vg_info_t *pvg_info)
{
    void *grp_pool;

    if(in_interrupt() || in_atomic())
        grp_pool = kmem_cache_alloc(pvg_info->job_cache, GFP_ATOMIC);
    else
        grp_pool = kmem_cache_alloc(pvg_info->job_cache, GFP_KERNEL);

    if(grp_pool) {
        pvg_info->grp_pool_cnt++;
        pvg_info->malloc_cnt++;
    }

    return grp_pool;
}

static void vcap_vg_group_pool_free(struct vcap_vg_info_t *pvg_info, void *grp_pool)
{
    kmem_cache_free(pvg_info->job_cache, grp_pool);
    pvg_info->grp_pool_cnt--;
    pvg_info->malloc_cnt--;
}

static void *vcap_vg_group_pool_lookup_alloc(struct vcap_vg_info_t *pvg_info, int grp_id, int grp_ver)
{
    struct vcap_vg_grp_pool_t *grp_pool = NULL;

    /* lookup group pool */
    if(!list_empty(&pvg_info->grp_pool_head)) {
        struct vcap_vg_grp_pool_t *curr_pool, *next_pool;
        list_for_each_entry_safe(curr_pool, next_pool, &pvg_info->grp_pool_head, pool_list) {
            if(curr_pool->id == grp_id) {
                grp_pool = curr_pool;
                break;
            }
        }
    }

    /* allocate new group pool */
    if(!grp_pool) {
        grp_pool = vcap_vg_group_pool_alloc(pvg_info);
        if(grp_pool) {
            grp_pool->id  = grp_id;
            grp_pool->ver = grp_ver;
            INIT_LIST_HEAD(&grp_pool->job_head);
            INIT_LIST_HEAD(&grp_pool->pool_list);
            list_add_tail(&grp_pool->pool_list, &pvg_info->grp_pool_head);
        }
    }

    return grp_pool;
}

static int vcap_vg_group_pool_get_run_ver_cnt(struct vcap_vg_grp_pool_t *grp_pool)
{
    int job_cnt = 0;
    struct vcap_vg_job_item_t *curr_job, *next_job;

    list_for_each_entry_safe(curr_job, next_job, &grp_pool->job_head, group_list) {
        if(curr_job->job_ver == grp_pool->ver)
            job_cnt++;
        else
            break;
    }

    return job_cnt;
}

static void vcap_vg_group_pool_job_check(struct vcap_vg_info_t *pvg_info)
{
    int new_ver;
    struct vcap_vg_grp_pool_t *curr_pool, *next_pool;
    struct vcap_vg_job_item_t *curr_job, *next_job, *job_item, *job_item_next;
    unsigned long flags;

    if(list_empty(&pvg_info->grp_pool_head))
        return;

    /* lookup all group pool */
    list_for_each_entry_safe(curr_pool, next_pool, &pvg_info->grp_pool_head, pool_list) {
        /* delete and free empty pool */
        if(list_empty(&curr_pool->job_head)) {
            list_del(&curr_pool->pool_list);
            vcap_vg_group_pool_free(pvg_info, curr_pool);
            continue;
        }

        /* check run version job count */
        if((vcap_vg_grp_run_mode == VCAP_VG_GRP_RUN_MODE_WAIT_OLD_VER_DONE) &&
           (vcap_vg_group_pool_get_run_ver_cnt(curr_pool) > vcap_vg_grp_run_ver_thres)) {
            continue;
        }

        spin_lock_irqsave(&pvg_info->lock, flags);  ///< to protect job status change, prevent content switch before all job status change

        /* check job status in current pool */
        if(vcap_vg_grp_run_mode == VCAP_VG_GRP_RUN_MODE_NORMAL) {
            /* switch all keep job to standby job */
            list_for_each_entry_safe(curr_job, next_job, &curr_pool->job_head, group_list) {
                if(curr_job->status == VCAP_VG_JOB_STATUS_KEEP) {
                    if(list_empty(&curr_job->m_job_head)) {
                        if(curr_job->status == VCAP_VG_JOB_STATUS_KEEP)
                            curr_job->status = VCAP_VG_JOB_STATUS_STANDBY;

                        if(curr_job->subjob && ((curr_job->subjob)->status == VCAP_VG_JOB_STATUS_KEEP))
                            (curr_job->subjob)->status = VCAP_VG_JOB_STATUS_STANDBY;
                    }
                    else {
                        list_for_each_entry_safe(job_item, job_item_next, &curr_job->m_job_head, m_job_list) {
                            if(job_item->status == VCAP_VG_JOB_STATUS_KEEP)
                                job_item->status = VCAP_VG_JOB_STATUS_STANDBY;

                            if(job_item->subjob && ((job_item->subjob)->status == VCAP_VG_JOB_STATUS_KEEP))
                                (job_item ->subjob)->status = VCAP_VG_JOB_STATUS_STANDBY;
                        }
                    }
                }

                /* update pool run version */
                if(curr_pool->ver != curr_job->job_ver)
                    curr_pool->ver = curr_job->job_ver;     ///< switch group pool job running version
            }
        }
        else {
            new_ver = 0;
            list_for_each_entry_safe(curr_job, next_job, &curr_pool->job_head, group_list) {
                if(curr_job->status == VCAP_VG_JOB_STATUS_KEEP) {
                    if((new_ver == 0) && (curr_job->job_ver != curr_pool->ver)) {
                        curr_pool->ver = curr_job->job_ver; ///< switch group pool job running version
                        new_ver++;
                        if(vcap_vg_log_level)
                            vcap_log("Group#%d run version switch to %d\n", curr_pool->id, curr_pool->ver);
                    }

                    if(curr_job->job_ver != curr_pool->ver)
                        break;

                    if(list_empty(&curr_job->m_job_head)) {
                        if(curr_job->status == VCAP_VG_JOB_STATUS_KEEP)
                            curr_job->status = VCAP_VG_JOB_STATUS_STANDBY;

                        if(curr_job->subjob && ((curr_job->subjob)->status == VCAP_VG_JOB_STATUS_KEEP))
                            (curr_job->subjob)->status = VCAP_VG_JOB_STATUS_STANDBY;
                    }
                    else {
                        list_for_each_entry_safe(job_item, job_item_next, &curr_job->m_job_head, m_job_list) {
                            if(job_item->status == VCAP_VG_JOB_STATUS_KEEP)
                                job_item->status = VCAP_VG_JOB_STATUS_STANDBY;

                            if(job_item->subjob && ((job_item->subjob)->status == VCAP_VG_JOB_STATUS_KEEP))
                                (job_item ->subjob)->status = VCAP_VG_JOB_STATUS_STANDBY;
                        }
                    }
                }
                else
                    break;  ///< go to next pool
            }
        }

        spin_unlock_irqrestore(&pvg_info->lock, flags);
    }
}

static struct vcap_vg_info_t *vcap_vg_get_info(struct video_entity_t *entity)
{
    int i;

    if(entity) {
        for(i=0; i<VCAP_VG_ENTITY_MAX; i++) {
            if(entity == &vcap_vg_info[i]->entity)
                return vcap_vg_info[i];
        }
    }
    return NULL;
}

static int vcap_vg_proc_info_show(struct seq_file *sfile, void *v)
{
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;

    down(&pvg_info->sema_lock);

    seq_printf(sfile, "=== VG Information ===\n");

    seq_printf(sfile, "VG_Malloc_Count  : %d\n", pvg_info->malloc_cnt);
#ifdef CFG_VG_CALLBACK_USE_KTHREAD
    seq_printf(sfile, "VG_CB_Event_Count: %d\n", atomic_read(&pvg_info->callback_wakeup_event));
#endif

    up(&pvg_info->sema_lock);

    return 0;
}

static int vcap_vg_proc_cb_show(struct seq_file *sfile, void *v)
{
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;

    down(&pvg_info->sema_lock);

    seq_printf(sfile, "Callback Period = %d (ms)\n", pvg_info->callback_period);

    up(&pvg_info->sema_lock);

    return 0;
}

static int vcap_vg_proc_property_show(struct seq_file *sfile, void *v)
{
    int i, j, engine, minor;
    int id, value, idx;
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;
    int disp_ch   = VCAP_VG_FD_ENGINE_CH(pvg_info->proc.engine);
    int disp_path = VCAP_VG_FD_MINOR_PATH(pvg_info->proc.minor);

    down(&pvg_info->sema_lock);

    seq_printf(sfile, "\n%s engine#%d minor#%d system ticks(0x%x)\n", pvg_info->entity.name,
               pvg_info->proc.engine, pvg_info->proc.minor, (int)jiffies&0xffff);
    seq_printf(sfile, "-----------------------------------------------------------\n");

    for(engine=0; engine<pvg_info->entity.engines; engine++) {
        if((disp_ch == engine) || (disp_ch >= pvg_info->entity.engines)) {
            for(minor=0; minor<pvg_info->entity.minors; minor++) {
                if((disp_path == minor) || (disp_path >= pvg_info->entity.minors)) {
                    idx = VCAP_VG_MAKE_IDX(pvg_info, engine, minor);

                    seq_printf(sfile, "\n%s engine#%d minor#%d job_id:%d\n",
                               pvg_info->entity.name,
                               VCAP_VG_FD_ENGINE(vcap_dev_get_vi_mode(pvg_info->private, engine), engine),
                               VCAP_VG_FD_MINOR(0, minor),
                               pvg_info->curr_property[idx].job_id);
                    seq_printf(sfile, "=============================================================\n");
                    seq_printf(sfile, "ID   Name(string)          Value(hex)\n");

                    i = 0;
                    do {
                        id = pvg_info->curr_property[idx].property[i].id;
                        if(id == ID_NULL)
                            break;

                        value = pvg_info->curr_property[idx].property[i].value;
                        for(j=0; j<ARRAY_SIZE(property_map); j++) {
                            if(property_map[j].id == id) {
                                seq_printf(sfile, "%-3d  %-20s  %08x\n", id, property_map[j].name, value);
                                break;
                            }
                        }
                        i++;
                    } while(i < VCAP_VG_MAX_RECORD);
                    seq_printf(sfile, "=============================================================\n");
                }
            }
        }
    }

    up(&pvg_info->sema_lock);

    return 0;
}

static int vcap_vg_proc_job_show(struct seq_file *sfile, void *v)
{
    int engine, minor, idx;
    struct vcap_vg_job_item_t *job_item, *next_item;
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;
    char *st_string[]={"  ", "STANDBY", "ONGOING", "FINISH", "FAIL"};
    int disp_ch   = VCAP_VG_FD_ENGINE_CH(pvg_info->proc.engine);
    int disp_path = VCAP_VG_FD_MINOR_PATH(pvg_info->proc.minor);

    down(&pvg_info->sema_lock);

    seq_printf(sfile, "\n%s engine#%d minor#%d system ticks(0x%x)\n", pvg_info->entity.name,
               pvg_info->proc.engine, pvg_info->proc.minor, (int)jiffies&0xffff);
    seq_printf(sfile, "--------------------------------------------------------------\n");

    seq_printf(sfile, "Engine  Minor  Job_ID         Status   Puttime  Start   End\n");
    seq_printf(sfile, "==============================================================\n");
    for(engine=0; engine<pvg_info->entity.engines; engine++) {
        if((disp_ch == engine) || (disp_ch >= pvg_info->entity.engines)) {
            for(minor=0; minor<pvg_info->entity.minors; minor++) {
                if((disp_path == minor) || (disp_path >= pvg_info->entity.minors)) {
                    idx = VCAP_VG_MAKE_IDX(pvg_info, engine, minor);

                    list_for_each_entry_safe(job_item, next_item, &pvg_info->minor_head[idx], minor_list) {
                        seq_printf(sfile, "%-7d %-6d %-1s%-3s%-10d %-8s 0x%04x   0x%04x  0x%04x\n",
                                   VCAP_VG_FD_ENGINE(vcap_dev_get_vi_mode(pvg_info->private, engine), engine),
                                   VCAP_VG_FD_MINOR(0, minor),
                                   job_item->need_callback == 0 ? " " : "*",
                                   job_item->type == VCAP_VG_JOB_TYPE_MULTI ? "(M)" : "(S)",
                                   job_item->job_id,
                                   st_string[job_item->status],
                                   job_item->puttime&0xffff,
                                   job_item->starttime&0xffff,
                                   job_item->finishtime&0xffff);
                    }
                }
            }
        }
    }

    up(&pvg_info->sema_lock);

    return 0;
}

static int vcap_vg_proc_util_show(struct seq_file *sfile, void *v)
{
    int idx, engine, minor;
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;

    down(&pvg_info->sema_lock);

    for(engine=0; engine<pvg_info->entity.engines; engine++) {
        for(minor=0; minor<pvg_info->entity.minors; minor++) {
            idx = VCAP_VG_MAKE_IDX(pvg_info, engine, minor);
            if(pvg_info->perf[idx].util_start == 0)
                break;

            if(jiffies > pvg_info->perf[idx].util_start) {
                seq_printf(sfile, "\nEngine%d-Minor%d HW Utilization Period=%d(sec) Utilization=%d\n",
                           VCAP_VG_FD_ENGINE(vcap_dev_get_vi_mode(pvg_info->private, engine), engine),
                           VCAP_VG_FD_MINOR(0, minor),
                           pvg_info->perf_util_period,
                           pvg_info->perf[idx].util_record);
            }
        }
    }

    up(&pvg_info->sema_lock);

    return 0;
}

static int vcap_vg_proc_log_filter_show(struct seq_file *sfile, void *v)
{
    int i;
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;

    down(&pvg_info->sema_lock);

    seq_printf(sfile, "\nUsage:\necho <0:exclude 1:include 2:clear> <engine> <minor> > filter\n");

    seq_printf(sfile, "\nDriver log Include:\n");
    for(i=0; i<VCAP_VG_LOG_MAX_FILTER; i++) {
        if(vcap_vg_log_inc_filter[i] >= 0) {
            seq_printf(sfile, "{%d,%d}, ",
                       ENTITY_FD_ENGINE(vcap_vg_log_inc_filter[i]),
                       ENTITY_FD_MINOR(vcap_vg_log_inc_filter[i]));
        }
    }

    seq_printf(sfile, "\nDriver log Exclude:\n");
    for(i=0; i<VCAP_VG_LOG_MAX_FILTER; i++) {
        if(vcap_vg_log_exc_filter[i] >= 0) {
            seq_printf(sfile, "{%d,%d}, ",
                       ENTITY_FD_ENGINE(vcap_vg_log_exc_filter[i]),
                       ENTITY_FD_MINOR(vcap_vg_log_exc_filter[i]));
        }
    }
    seq_printf(sfile, "\n");

    up(&pvg_info->sema_lock);

    return 0;
}

static int vcap_vg_proc_log_level_show(struct seq_file *sfile, void *v)
{
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;

    down(&pvg_info->sema_lock);

    seq_printf(sfile, "\nLog level = %d (0:emerge 1:error 2:debug)\n", vcap_vg_log_level);

    up(&pvg_info->sema_lock);

    return 0;
}

static int vcap_vg_proc_buf_clean_show(struct seq_file *sfile, void *v)
{
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;

    down(&pvg_info->sema_lock);

    seq_printf(sfile, "\nOutput buf_clean property = %d (0:disable 1:enable)\n", vcap_vg_buf_clean);

    up(&pvg_info->sema_lock);

    return 0;
}

static int vcap_vg_proc_group_info_show(struct seq_file *sfile, void *v)
{
    int job_cnt;
    struct vcap_vg_grp_pool_t *curr_pool, *next_pool;
    struct vcap_vg_job_item_t *curr_job, *next_job;
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;
    char *grp_mode_str[] = {"Normal", "Wait_Old_Version_Done"};
    char *job_sts_str[]  = {"None", "STANDBY", "ONGOING", "FINISH", "FAIL", "KEEP"};

    down(&pvg_info->sema_lock);

    seq_printf(sfile, "Group Run Mode             : %s\n", grp_mode_str[vcap_vg_grp_run_mode]);
    seq_printf(sfile, "Group Run Version Threshold: %d\n", vcap_vg_grp_run_ver_thres);
    seq_printf(sfile, "Group Pool Count           : %d\n", pvg_info->grp_pool_cnt);
    seq_printf(sfile, "=====================================================================\n");

    list_for_each_entry_safe(curr_pool, next_pool, &pvg_info->grp_pool_head, pool_list) {
        seq_printf(sfile, "[Group#%d-Ver:%d]\n", curr_pool->id, curr_pool->ver);
        job_cnt = 0;
        list_for_each_entry_safe(curr_job, next_job, &curr_pool->job_head, group_list) {
            job_cnt++;
            seq_printf(sfile, "%d[V:%d,%s]%s",
                       curr_job->job_id,
                       curr_job->job_ver,
                       ((curr_job->status > 0 && curr_job->status <= VCAP_VG_JOB_STATUS_KEEP) ? job_sts_str[curr_job->status] : job_sts_str[0]),
                       ((job_cnt%4 == 0) ? "\n" : " "));
        }
        seq_printf(sfile, "%s---------------------------------------------------------------------\n", (((job_cnt%4) != 0) ? "\n" : ""));
    }

    seq_printf(sfile, "=====================================================================\n");
    seq_printf(sfile, "echo [mode] [thres] to setup group run mode and run version threshold\n");
    seq_printf(sfile, "mode  => 0: normal 1:wait_old_ver_done\n");

    up(&pvg_info->sema_lock);

    return 0;
}

static int vcap_vg_proc_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_vg_proc_info_show, PDE(inode)->data);
}

static int vcap_vg_proc_cb_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_vg_proc_cb_show, PDE(inode)->data);
}

static int vcap_vg_proc_property_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_vg_proc_property_show, PDE(inode)->data);
}

static int vcap_vg_proc_job_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_vg_proc_job_show, PDE(inode)->data);
}

static int vcap_vg_proc_util_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_vg_proc_util_show, PDE(inode)->data);
}

static int vcap_vg_proc_log_filter_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_vg_proc_log_filter_show, PDE(inode)->data);
}

static int vcap_vg_proc_log_level_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_vg_proc_log_level_show, PDE(inode)->data);
}

static int vcap_vg_proc_buf_clean_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_vg_proc_buf_clean_show, PDE(inode)->data);
}

static int vcap_vg_proc_group_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_vg_proc_group_info_show, PDE(inode)->data);
}

static ssize_t vcap_vg_proc_cb_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned int cb_period;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%u\n", &cb_period);

    down(&pvg_info->sema_lock);

    pvg_info->callback_period = cb_period;

    up(&pvg_info->sema_lock);

    return count;
}

static ssize_t vcap_vg_proc_index_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned int engine, minor;
    char value_str[64] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%u %u\n", &engine, &minor);

    down(&pvg_info->sema_lock);

    pvg_info->proc.engine = engine;
    pvg_info->proc.minor  = minor;

    up(&pvg_info->sema_lock);

    return count;
}

static ssize_t vcap_vg_proc_util_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned int util_period;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%u\n", &util_period);

    down(&pvg_info->sema_lock);

    pvg_info->perf_util_period = util_period;

    up(&pvg_info->sema_lock);

    return count;
}

static ssize_t vcap_vg_proc_log_filter_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i;
    unsigned int mode, engine, minor;
    char value_str[64] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%u %u %u\n", &mode, &engine, &minor);

    down(&pvg_info->sema_lock);

    if(mode == 0) { //exclude
        for(i=0; i<VCAP_VG_LOG_MAX_FILTER; i++) {
            if (vcap_vg_log_exc_filter[i] == -1) {
                vcap_vg_log_exc_filter[i] = ENTITY_FD(0, engine, minor);
                break;
            }
        }
    }
    else if(mode == 1) {
        for(i=0; i<VCAP_VG_LOG_MAX_FILTER; i++) {
            if(vcap_vg_log_inc_filter[i] == -1) {
                vcap_vg_log_inc_filter[i] = ENTITY_FD(0, engine, minor);
                break;
            }
        }
    }
    else {  /* clear filter */
        for(i=0; i<VCAP_VG_LOG_MAX_FILTER; i++) {
            vcap_vg_log_exc_filter[i] = vcap_vg_log_inc_filter[i] = -1;
        }
    }

    up(&pvg_info->sema_lock);

    return count;
}

static ssize_t vcap_vg_proc_log_level_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  log_level;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &log_level);

    down(&pvg_info->sema_lock);

    vcap_vg_log_level = log_level;

    up(&pvg_info->sema_lock);

    return count;
}

static ssize_t vcap_vg_proc_buf_clean_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  buf_clean;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &buf_clean);

    down(&pvg_info->sema_lock);

    vcap_vg_buf_clean = buf_clean;

    up(&pvg_info->sema_lock);

    return count;
}

static ssize_t vcap_vg_proc_group_info_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  run_mode, ver_thres = 0, pool_check = 0;
    char value_str[32] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &run_mode, &ver_thres);

    down(&pvg_info->sema_lock);

    if((vcap_vg_grp_run_mode != run_mode) && (run_mode == VCAP_VG_GRP_RUN_MODE_NORMAL || run_mode == VCAP_VG_GRP_RUN_MODE_WAIT_OLD_VER_DONE)) {
        if((vcap_vg_grp_run_mode == VCAP_VG_GRP_RUN_MODE_WAIT_OLD_VER_DONE) && (run_mode == VCAP_VG_GRP_RUN_MODE_NORMAL))
            pool_check++;   ///< group pool job status recheck

        vcap_vg_grp_run_mode = run_mode;
    }

    if(vcap_vg_grp_run_ver_thres != ver_thres) {
        if(vcap_vg_grp_run_mode == VCAP_VG_GRP_RUN_MODE_WAIT_OLD_VER_DONE)
            pool_check++;   ///< group pool job status recheck

        vcap_vg_grp_run_ver_thres = ver_thres;
    }

    if(pool_check)
        vcap_vg_group_pool_job_check(pvg_info);

    up(&pvg_info->sema_lock);

    return count;
}

static struct file_operations vcap_vg_proc_info_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_vg_proc_info_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_vg_proc_cb_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_vg_proc_cb_open,
    .write  = vcap_vg_proc_cb_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_vg_proc_property_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_vg_proc_property_open,
    .write  = vcap_vg_proc_index_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_vg_proc_job_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_vg_proc_job_open,
    .write  = vcap_vg_proc_index_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_vg_proc_util_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_vg_proc_util_open,
    .write  = vcap_vg_proc_util_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_vg_proc_log_filter_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_vg_proc_log_filter_open,
    .write  = vcap_vg_proc_log_filter_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_vg_proc_log_level_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_vg_proc_log_level_open,
    .write  = vcap_vg_proc_log_level_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_vg_proc_buf_clean_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_vg_proc_buf_clean_open,
    .write  = vcap_vg_proc_buf_clean_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_vg_proc_group_info_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_vg_proc_group_info_open,
    .write  = vcap_vg_proc_group_info_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void vcap_vg_proc_remove(int id)
{
    struct vcap_vg_info_t *pvg_info;

    if(id >= VCAP_VG_ENTITY_MAX)
        return;

    pvg_info = vcap_vg_info[id];

    if(pvg_info->proc.root) {
        if(pvg_info->proc.group_info)
            remove_proc_entry((pvg_info->proc.group_info)->name, pvg_info->proc.root);

        if(pvg_info->proc.buf_clean)
            remove_proc_entry((pvg_info->proc.buf_clean)->name, pvg_info->proc.root);

        if(pvg_info->proc.log_filter)
            remove_proc_entry((pvg_info->proc.log_filter)->name, pvg_info->proc.root);

        if(pvg_info->proc.log_level)
            remove_proc_entry((pvg_info->proc.log_level)->name, pvg_info->proc.root);

        if(pvg_info->proc.util)
            remove_proc_entry((pvg_info->proc.util)->name, pvg_info->proc.root);

        if(pvg_info->proc.job)
            remove_proc_entry((pvg_info->proc.job)->name, pvg_info->proc.root);

        if(pvg_info->proc.property)
            remove_proc_entry((pvg_info->proc.property)->name, pvg_info->proc.root);

        if(pvg_info->proc.cb_period)
            remove_proc_entry((pvg_info->proc.cb_period)->name, pvg_info->proc.root);

        if(pvg_info->proc.info)
            remove_proc_entry((pvg_info->proc.info)->name, pvg_info->proc.root);

        remove_proc_entry((pvg_info->proc.root)->name, (pvg_info->proc.root)->parent);
    }
}

static int vcap_vg_proc_init(char *name, struct vcap_vg_info_t *pvg_info)
{
    int  ret = 0;
    char root_name[64];

    /* proc root */
    snprintf(root_name, sizeof(root_name), "videograph/%s", name);
    pvg_info->proc.root = create_proc_entry(root_name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(pvg_info->proc.root == NULL) {
        vcap_err("error to create %s proc, please insert log.ko first\n", root_name);
        ret = -EFAULT;
        goto err;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    (pvg_info->proc.root)->owner = THIS_MODULE;
#endif

    /* info */
    pvg_info->proc.info = create_proc_entry("info", S_IRUGO|S_IXUGO, pvg_info->proc.root);
    if(pvg_info->proc.info == NULL) {
        vcap_err("error to create %s/info proc\n", root_name);
        ret = -EFAULT;
        goto err;
    }
    (pvg_info->proc.info)->proc_fops  = &vcap_vg_proc_info_ops;
    (pvg_info->proc.info)->data       = (void *)pvg_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    (pvg_info->proc.info)->owner      = THIS_MODULE;
#endif

    /* callback period */
    pvg_info->proc.cb_period = create_proc_entry("callback_period", S_IRUGO|S_IXUGO, pvg_info->proc.root);
    if(pvg_info->proc.cb_period == NULL) {
        vcap_err("error to create %s/callback_period proc\n", root_name);
        ret = -EFAULT;
        goto err;
    }
    (pvg_info->proc.cb_period)->proc_fops  = &vcap_vg_proc_cb_ops;
    (pvg_info->proc.cb_period)->data       = (void *)pvg_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    (pvg_info->proc.cb_period)->owner      = THIS_MODULE;
#endif

    /* property */
    pvg_info->proc.property = create_proc_entry("property", S_IRUGO|S_IXUGO, pvg_info->proc.root);
    if(pvg_info->proc.property == NULL) {
        vcap_err("error to create %s/property proc\n", root_name);
        ret = -EFAULT;
        goto err;
    }
    (pvg_info->proc.property)->proc_fops  = &vcap_vg_proc_property_ops;
    (pvg_info->proc.property)->data       = (void *)pvg_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    (pvg_info->proc.property)->owner      = THIS_MODULE;
#endif

    /* job */
    pvg_info->proc.job = create_proc_entry("job", S_IRUGO|S_IXUGO, pvg_info->proc.root);
    if(pvg_info->proc.job == NULL) {
        vcap_err("error to create %s/job proc\n", root_name);
        ret = -EFAULT;
        goto err;
    }
    (pvg_info->proc.job)->proc_fops  = &vcap_vg_proc_job_ops;
    (pvg_info->proc.job)->data       = (void *)pvg_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    (pvg_info->proc.job)->owner      = THIS_MODULE;
#endif

    /* utilization */
    pvg_info->proc.util = create_proc_entry("utilization", S_IRUGO|S_IXUGO, pvg_info->proc.root);
    if(pvg_info->proc.util == NULL) {
        vcap_err("error to create %s/utilization proc\n", root_name);
        ret = -EFAULT;
        goto err;
    }
    (pvg_info->proc.util)->proc_fops  = &vcap_vg_proc_util_ops;
    (pvg_info->proc.util)->data       = (void *)pvg_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    (pvg_info->proc.util)->owner      = THIS_MODULE;
#endif

    /* log filter */
    pvg_info->proc.log_filter = create_proc_entry("filter", S_IRUGO|S_IXUGO, pvg_info->proc.root);
    if(pvg_info->proc.log_filter == NULL) {
        vcap_err("error to create %s/filter proc\n", root_name);
        return -EFAULT;
        goto err;
    }
    (pvg_info->proc.log_filter)->proc_fops  = &vcap_vg_proc_log_filter_ops;
    (pvg_info->proc.log_filter)->data       = (void *)pvg_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    (pvg_info->proc.log_filter)->owner      = THIS_MODULE;
#endif

    /* log level */
    pvg_info->proc.log_level = create_proc_entry("level", S_IRUGO|S_IXUGO, pvg_info->proc.root);
    if(pvg_info->proc.log_level == NULL) {
        vcap_err("error to create %s/level proc\n", root_name);
        return -EFAULT;
        goto err;
    }
    (pvg_info->proc.log_level)->proc_fops  = &vcap_vg_proc_log_level_ops;
    (pvg_info->proc.log_level)->data       = (void *)pvg_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    (pvg_info->proc.log_level)->owner      = THIS_MODULE;
#endif

    /* buf_clean */
    pvg_info->proc.buf_clean = create_proc_entry("buf_clean", S_IRUGO|S_IXUGO, pvg_info->proc.root);
    if(pvg_info->proc.buf_clean == NULL) {
        vcap_err("error to create %s/buf_clean proc\n", root_name);
        return -EFAULT;
        goto err;
    }
    (pvg_info->proc.buf_clean)->proc_fops  = &vcap_vg_proc_buf_clean_ops;
    (pvg_info->proc.buf_clean)->data       = (void *)pvg_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    (pvg_info->proc.buf_clean)->owner      = THIS_MODULE;
#endif

    /* group_info */
    pvg_info->proc.group_info = create_proc_entry("group_info", S_IRUGO|S_IXUGO, pvg_info->proc.root);
    if(pvg_info->proc.group_info == NULL) {
        vcap_err("error to create %s/group_info proc\n", root_name);
        ret = -EFAULT;
        goto err;
    }
    (pvg_info->proc.group_info)->proc_fops  = &vcap_vg_proc_group_info_ops;
    (pvg_info->proc.group_info)->data       = (void *)pvg_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    (pvg_info->proc.group_info)->owner      = THIS_MODULE;
#endif

err:
    if(ret < 0)
        vcap_vg_proc_remove(pvg_info->index);

    return ret;
}

static inline int vcap_vg_preprocess(void *param, void *priv)
{
    struct vcap_vg_job_item_t *job_item = (struct vcap_vg_job_item_t *)param;
    return video_preprocess(job_item->job, priv);
}

static inline int vcap_vg_postprocess(void *parm, void *priv)
{
    int ret = 0;
    struct vcap_vg_job_item_t *job_item=(struct vcap_vg_job_item_t *)parm;

    if(VCAP_VG_FD_ENGINE_VIMODE(job_item->engine) == VCAP_VG_VIMODE_SPLIT) {    ///< postprocess all split job
        struct vcap_vg_job_item_t *curr_item, *next_item;
        list_for_each_entry_safe(curr_item, next_item, &job_item->m_job_head, m_job_list) {
            ret = video_postprocess(curr_item->job, priv);
            if(ret < 0)
                goto exit;
        }
    }
    else
        ret = video_postprocess(job_item->job, priv);

exit:
    return ret;
}

static inline int vcap_vg_parse_in_property(struct vcap_vg_job_item_t *job_item)
{
    int idx, i = 0;
    struct video_entity_job_t  *job   = (struct video_entity_job_t *)job_item->job;
    struct vcap_vg_job_param_t *param = &job_item->param;
    struct video_property_t *property = job->in_prop;
    struct vcap_vg_info_t   *pvg_info = (struct vcap_vg_info_t *)job_item->info;

    /* check output buffer */
    if(!job->out_buf.buf[0].addr_pa) {
        VCAP_VG_LOG(VCAP_VG_LOG_ERROR, job_item->engine, job_item->minor,
                    "job#%d out_buf[0] is NULL!(fd: 0x%08x)\n", job_item->job_id, job->fd);
        return -1;
    }

    param->ch          = VCAP_VG_FD_ENGINE_CH(job_item->engine);
    param->split_ch    = VCAP_VG_FD_MINOR_SPLITCH(job_item->minor);
    param->path        = VCAP_VG_FD_MINOR_PATH(job_item->minor);
    param->addr_va[0]  = job->out_buf.buf[0].addr_va;
    param->addr_pa[0]  = job->out_buf.buf[0].addr_pa;
    param->out_size[0] = job->out_buf.buf[0].size;
    param->addr_va[1]  = job->out_buf.buf[1].addr_va;
    param->addr_pa[1]  = job->out_buf.buf[1].addr_pa;
    param->out_size[1] = job->out_buf.buf[1].size;
    param->data        = (void *)job_item;

    idx = VCAP_VG_MAKE_IDX(pvg_info, param->ch, param->path);

    while(property[i].id!=0) {
        switch(property[i].id) {
            case ID_SRC_FMT:
                param->src_fmt = property[i].value;
                break;
            case ID_SRC_XY:
                param->src_xy = property[i].value;
                break;
            case ID_SRC_BG_DIM:
                param->src_bg_dim = property[i].value;
                break;
            case ID_SRC_DIM:
                param->src_dim = property[i].value;
                break;
            case ID_SRC_INTERLACE:
                param->src_interlace = property[i].value;
                break;
            case ID_DST_FMT:
                param->job_fmt = property[i].value;
                if(param->job_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
                    param->dst_fmt = TYPE_YUV422_2FRAMES;
                else if((param->job_fmt == TYPE_YUV422_FRAME_2FRAMES) || (param->job_fmt == TYPE_YUV422_FRAME_FRAME))
                    param->dst_fmt = TYPE_YUV422_FRAME;
                else
                    param->dst_fmt = param->job_fmt;
                break;
            case ID_DST_XY:
                param->dst_xy = property[i].value;
                break;
            case ID_DST_BG_DIM:
                param->dst_bg_dim = property[i].value;
                break;
            case ID_DST_DIM:
                param->dst_dim = property[i].value;
                break;
            case ID_DST_CROP_XY:
                param->dst_crop_xy = property[i].value;
                break;
            case ID_DST_CROP_DIM:
                param->dst_crop_dim = property[i].value;
                break;
            case ID_DST2_CROP_XY:
                param->dst2_crop_xy = property[i].value;
                break;
            case ID_DST2_CROP_DIM:
                param->dst2_crop_dim = property[i].value;
                break;
            case ID_DST2_FD:
                param->dst2_fd = property[i].value;
                break;
            case ID_DST2_BG_DIM:
                param->dst2_bg_dim = property[i].value;
                break;
            case ID_DST2_XY:
                param->dst2_xy = property[i].value;
                break;
            case ID_DST2_DIM:
                param->dst2_dim = property[i].value;
                break;
            case ID_DST2_BUF_OFFSET:
                param->dst2_buf_offset = property[i].value;
                break;
            case ID_TARGET_FRAME_RATE:
                param->target_frame_rate = property[i].value;
                break;
            case ID_FPS_RATIO:
                param->fps_ratio = property[i].value;
                break;
            case ID_RUNTIME_FRAME_RATE:
                param->runtime_frame_rate = property[i].value;
                break;
            case ID_SRC_FRAME_RATE:
                param->src_frame_rate = property[i].value;
                break;
            case ID_DVR_FEATURE:
                switch(property[i].value) {
                    case 0: /* liveview  ==> use DMA#0 */
                    case 1: /* recording ==> use DMA#1 */
                        param->dma_ch = property[i].value;
                        break;
                }
                break;
            case ID_AUTO_ASPECT_RATIO:
                param->auto_aspect_ratio = (property[i].value & 0xffff);    ///< [31:16]=>palette index, [15:0]=>enable/disable
                break;
            case ID_AUTO_BORDER:
                param->auto_border = property[i].value; ///< (palette_idx << 16 | border_width << 8 | enable)
                break;
            case ID_CAP_FEATURE:
                /* P2I */
                if(property[i].value & VCAP_VG_PROP_CAP_FEATURE_P2I_BIT)
                    param->p2i = 1;
                else
                    param->p2i = 0;

                if(property[i].value & VCAP_VG_PROP_CAP_FEATURE_PROG_2FRM_BIT)
                    param->prog_2frm = 1;
                else
                    param->prog_2frm = 0;
                break;
            case ID_MD_ENB:
                param->md_enb = property[i].value;
                break;
            case ID_MD_XY_START:
                param->md_xy_start = property[i].value;
                break;
            case ID_MD_XY_SIZE:
                param->md_xy_size = property[i].value;
                break;
            case ID_MD_XY_NUM:
                param->md_xy_num = property[i].value;
                break;
            case ID_SPEICIAL_FUNC:
                if(property[i].value & VCAP_VG_PROP_SPEC_FUN_FRM_60I_BIT)
                    param->frm_2frm_field = 1;
                else
                    param->frm_2frm_field = 0;
                break;

            default:
                break;
        }
        i++;
    }

    /* check property */
    if(param->frm_2frm_field && ((param->job_fmt != TYPE_YUV422_FRAME) && (param->job_fmt != TYPE_YUV422_FRAME_FRAME))) {
        VCAP_VG_LOG(VCAP_VG_LOG_ERROR, job_item->engine, job_item->minor,
                    "job#%d buf_type:0x%04x not support frame_60I feature!(fd: 0x%08x)\n",
                    job_item->job_id, param->job_fmt, job->fd);
        return -1;
    }

    return 0;
}

static inline int vcap_vg_check_buf_boundary(struct vcap_vg_job_item_t *job_item)
{
    int ret = 0;
    u32 bg_size, bg_w, bg_h, bg_x, bg_y;
    u32 dst_w, dst_h;
    struct video_entity_job_t *job = (struct video_entity_job_t *)job_item->job;
    struct vcap_vg_job_param_t *param = &job_item->param;

    bg_size = param->out_size[0];
    bg_w    = EM_PARAM_WIDTH(param->dst_bg_dim);
    bg_h    = EM_PARAM_HEIGHT(param->dst_bg_dim);
    bg_x    = EM_PARAM_X(param->dst_xy);
    bg_y    = EM_PARAM_Y(param->dst_xy);

    if(param->dst_crop_dim) {
        dst_w = EM_PARAM_WIDTH(param->dst_crop_dim);
        dst_h = EM_PARAM_HEIGHT(param->dst_crop_dim);
    }
    else {
        dst_w = EM_PARAM_WIDTH(param->dst_dim);
        dst_h = EM_PARAM_HEIGHT(param->dst_dim);
    }

    switch(param->dst_fmt) {
        case TYPE_YUV422_FIELDS:
        case TYPE_YUV422_FRAME:
        case TYPE_YUV422_2FRAMES:
            if((param->dst_fmt == TYPE_YUV422_2FRAMES) ||
               ((param->dst_fmt == TYPE_YUV422_FRAME) && ((param->job_fmt == TYPE_YUV422_2FRAMES_2FRAMES) || (param->job_fmt == TYPE_YUV422_FRAME_2FRAMES)) && !param->dst2_fd)) {  ///< subjob
                bg_size >>= 1;
            }

            if((param->addr_va[0] & 0x7) || (param->addr_pa[0] & 0x7)) {    ///< buffer address must multiple of 8 for DMA aglinment
                ret = -1;
                VCAP_VG_LOG(VCAP_VG_LOG_ERROR, job_item->engine, job_item->minor,
                            "job#%d buffer address not multiple of 8!(fd: 0x%08x) => j_fmt:0x%04x fmt:0x%04x va:0x%08x pa:0x%08x size:%u\n",
                            job->id, job->fd, param->job_fmt, param->dst_fmt,
                            param->addr_va[0], param->addr_pa[0], param->out_size[0]);
                goto exit;
            }

            if(((bg_x + dst_w) > bg_w) ||
               ((bg_y + dst_h) > bg_h) ||
               (((bg_w*(bg_y + dst_h - 1) + (bg_x + dst_w))*2) > bg_size)) {
                ret = -1;
                VCAP_VG_LOG(VCAP_VG_LOG_ERROR, job_item->engine, job_item->minor,
                            "job#%d buffer boundary check failed!(fd: 0x%08x) => j_fmt:0x%04x fmt:0x%04x va:0x%08x pa:0x%08x size:%u bg_xy(%u, %u) bg_wh(%u, %u) dst_wh(%u, %u)\n",
                            job->id, job->fd, param->job_fmt, param->dst_fmt,
                            param->addr_va[0], param->addr_pa[0], param->out_size[0],
                            bg_x, bg_y, bg_w, bg_h,
                            dst_w, dst_h);
                goto exit;
            }
            break;

        default:
            ret = -1;
            VCAP_VG_LOG(VCAP_VG_LOG_ERROR, job_item->engine, job_item->minor,
                    "job#%d buffer format=0x%x not support!!(fd: 0x%08x, job_fmt:0x%04x)\n", job->id, param->dst_fmt, job->fd, param->job_fmt);
            goto exit;
    }

exit:
    return ret;
}

static inline void *vcap_vg_alloc_subjob(struct vcap_vg_job_item_t *job_item, int job_fmt)
{
    int ret = 0;
    int dst_bg_w, dst_bg_h;
    int dst_w, dst_h;
    int dst2_engine, dst2_minor;
    int dst2_x, dst2_y, dst2_w, dst2_h, dst2_bg_w, dst2_bg_h;
    int dst2_crop_x, dst2_crop_y, dst2_crop_w, dst2_crop_h;
    int dst2_auto_border;
    struct video_entity_job_t *job = (struct video_entity_job_t *)job_item->job;
    struct vcap_vg_job_item_t *job_item2 = NULL;
    struct vcap_vg_info_t     *pvg_info  = (struct vcap_vg_info_t *)job_item->info;

    dst2_engine = ENTITY_FD_ENGINE(job_item->param.dst2_fd);
    dst2_minor  = ENTITY_FD_MINOR(job_item->param.dst2_fd);

    /* check dst2 engine and minor */
    if((job_item->engine != dst2_engine) || (job_item->minor == dst2_minor)) {
        VCAP_VG_LOG(VCAP_VG_LOG_ERROR, job_item->engine, job_item->minor,
                    "job#%d(subjob) property dst2_fd:0x%08x invalid!(fd: 0x%08x)\n", job->id, job_item->param.dst2_fd, job->fd);
        ret = -1;
        goto err;
    }

    /* check dst2 frame buffer offset specify or not? */
    if(!job_item->param.dst2_buf_offset) {
        /* check job output buffer count */
        if(job->out_buf.count < 1) {
	    	VCAP_VG_LOG(VCAP_VG_LOG_ERROR, job_item->engine, job_item->minor,
                        "job#%d out buffer count=%d invalid\n", job->id, job->out_buf.count);
            ret = -1;
            goto err;
        }

        /* check dst2 frame buffer */
        if((job->out_buf.count == 2) && (!job->out_buf.buf[1].addr_pa)) {
            VCAP_VG_LOG(VCAP_VG_LOG_ERROR, job_item->engine, job_item->minor,
                        "job#%d(subjob) dst2 out_buf is NULL!(fd: 0x%08x)\n", job->id, job->fd);
            ret = -1;
            goto err;
        }
    }

    dst_bg_w  = EM_PARAM_WIDTH(job_item->param.dst_bg_dim);
    dst_bg_h  = EM_PARAM_HEIGHT(job_item->param.dst_bg_dim);
    dst2_bg_w = EM_PARAM_WIDTH(job_item->param.dst2_bg_dim);
    dst2_bg_h = EM_PARAM_HEIGHT(job_item->param.dst2_bg_dim);
    dst_w     = EM_PARAM_WIDTH(job_item->param.dst_dim);
    dst_h     = EM_PARAM_HEIGHT(job_item->param.dst_dim);

    /* check dst and dst2 background dimension */
    if(!dst_bg_w || !dst_bg_h || !dst2_bg_w || !dst2_bg_h) {
        VCAP_VG_LOG(VCAP_VG_LOG_ERROR, job_item->engine, job_item->minor,
                    "error to put job#%d(subjob) fd(0x%08x), bg_dim incorrect(dst_bg_dim:0x%08x, dst2_bg_dim:0x%08x)!\n",
                    job->id,
                    job->fd,
                    job_item->param.dst_bg_dim,
                    job_item->param.dst2_bg_dim);
        ret = -1;
        goto err;
    }

    if(job_item->param.dst2_dim) {  ///< specify dst2 position and dimension from middleware
        dst2_x      = EM_PARAM_X(job_item->param.dst2_xy);
        dst2_y      = EM_PARAM_Y(job_item->param.dst2_xy);
        dst2_w      = EM_PARAM_WIDTH(job_item->param.dst2_dim);
        dst2_h      = EM_PARAM_HEIGHT(job_item->param.dst2_dim);
        dst2_crop_x = EM_PARAM_X(job_item->param.dst2_crop_xy);
        dst2_crop_y = EM_PARAM_Y(job_item->param.dst2_crop_xy);
        dst2_crop_w = EM_PARAM_WIDTH(job_item->param.dst2_crop_dim);
        dst2_crop_h = EM_PARAM_HEIGHT(job_item->param.dst2_crop_dim);
    }
    else {  ///< caculate dst2 position and size base on dst background dimension
        dst2_x      = (dst2_bg_w*EM_PARAM_X(job_item->param.dst_xy))/dst_bg_w;
        dst2_y      = (dst2_bg_h*EM_PARAM_Y(job_item->param.dst_xy))/dst_bg_h;
        dst2_w      = (dst2_bg_w*EM_PARAM_WIDTH(job_item->param.dst_dim))/dst_bg_w;
        dst2_h      = (dst2_bg_h*EM_PARAM_HEIGHT(job_item->param.dst_dim))/dst_bg_h;
        dst2_crop_x = (dst2_bg_w*EM_PARAM_X(job_item->param.dst_crop_xy))/dst_bg_w;
        dst2_crop_y = (dst2_bg_h*EM_PARAM_Y(job_item->param.dst_crop_xy))/dst_bg_h;
        dst2_crop_w = (dst2_bg_w*EM_PARAM_WIDTH(job_item->param.dst_crop_dim))/dst_bg_w;
        dst2_crop_h = (dst2_bg_w*EM_PARAM_HEIGHT(job_item->param.dst_crop_dim))/dst_bg_w;
    }

    /* dst2 border width adjustment base on dst and dst2 dimension */
    dst2_auto_border = job_item->param.auto_border;
    if(dst2_auto_border) {
        if((dst_w >= (dst2_w*2)) || (dst_h >= (dst2_h*2))) {
            dst2_auto_border = ((((job_item->param.auto_border>>8)&0xff)>>1)<<8) | (job_item->param.auto_border&(~(0xff<<8)));
        }
        else if((dst2_w >= (dst_w*2)) || (dst2_h >= (dst_h*2))) {
            dst2_auto_border = (((((job_item->param.auto_border>>8)&0xff)<<1)&0xff)<<8) | (job_item->param.auto_border&(~(0xff<<8)));
        }
    }

    /* allocate dst2 job */
    job_item2 = vcap_vg_job_item_alloc(pvg_info);
    if(!job_item2) {
        VCAP_VG_LOG(VCAP_VG_LOG_ERROR, job_item->engine, job_item->minor,
                    "error to put job#%d(subjob) fd(0x%08x), no free memory to allocate dst2 job!\n", job->id, job->fd);
        ret = -1;
        goto err;
    }
    memset(job_item2, 0, sizeof(struct vcap_vg_job_item_t));

    /* prepare config of job_item#2 */
    INIT_LIST_HEAD(&job_item2->engine_list);
    INIT_LIST_HEAD(&job_item2->minor_list);
    INIT_LIST_HEAD(&job_item2->m_job_head);
    INIT_LIST_HEAD(&job_item2->m_job_list);
    INIT_LIST_HEAD(&job_item2->group_list);

    job_item2->info          = job_item->info;
    job_item2->engine        = dst2_engine;
    job_item2->minor         = dst2_minor;
    job_item2->job_id        = job_item->job_id;
    job_item2->job_ver       = job_item->job_ver;
    job_item2->job           = job_item->job;
    job_item2->root_job      = (void *)job_item;
    job_item2->type          = VCAP_VG_JOB_TYPE_NORMAL;
    job_item2->status        = job_item->status;
    job_item2->private       = job_item->private;
    job_item2->timestamp     = 0;
    job_item2->puttime       = jiffies;
    job_item2->starttime     = 0;
    job_item2->finishtime    = 0;
    job_item2->need_callback = 0;
    job_item2->subjob        = NULL;

    memcpy(&job_item2->param, &job_item->param, sizeof(struct vcap_vg_job_param_t));

    if(!job_item->param.dst2_buf_offset) {
        if(job->out_buf.count == 1) {
            switch(job->out_buf.btype) {    ///< dst buffer type
                case TYPE_YUV422_2FRAMES_2FRAMES:
                    job_item2->param.addr_va[0]  = job->out_buf.buf[0].addr_va + (job->out_buf.buf[0].width*job->out_buf.buf[0].height*2*2);
                    job_item2->param.addr_pa[0]  = job->out_buf.buf[0].addr_pa + (job->out_buf.buf[0].width*job->out_buf.buf[0].height*2*2);
                    job_item2->param.out_size[0] = job->out_buf.buf[0].size    - (job->out_buf.buf[0].width*job->out_buf.buf[0].height*2*2);
                    break;
                default:
                    job_item2->param.addr_va[0]  = job->out_buf.buf[0].addr_va + (job->out_buf.buf[0].width*job->out_buf.buf[0].height*2);
                    job_item2->param.addr_pa[0]  = job->out_buf.buf[0].addr_pa + (job->out_buf.buf[0].width*job->out_buf.buf[0].height*2);
                    job_item2->param.out_size[0] = job->out_buf.buf[0].size    - (job->out_buf.buf[0].width*job->out_buf.buf[0].height*2);
                    break;
            }
        }
        else {
            job_item2->param.addr_va[0]  = job->out_buf.buf[1].addr_va;
            job_item2->param.addr_pa[0]  = job->out_buf.buf[1].addr_pa;
            job_item2->param.out_size[0] = job->out_buf.buf[1].size;
        }
    }
    else {
        switch(job->out_buf.btype) {    ///< dst buffer type
            case TYPE_YUV422_2FRAMES_2FRAMES:
                job_item2->param.addr_va[0]  = job->out_buf.buf[0].addr_va + job_item->param.dst2_buf_offset;
                job_item2->param.addr_pa[0]  = job->out_buf.buf[0].addr_pa + job_item->param.dst2_buf_offset;
                job_item2->param.out_size[0] = dst2_bg_w*dst2_bg_h*2*2;
                break;
            default:
                job_item2->param.addr_va[0]  = job->out_buf.buf[0].addr_va + job_item->param.dst2_buf_offset;
                job_item2->param.addr_pa[0]  = job->out_buf.buf[0].addr_pa + job_item->param.dst2_buf_offset;
                job_item2->param.out_size[0] = dst2_bg_w*dst2_bg_h*2;
                break;
        }
    }

    job_item2->param.path              = VCAP_VG_FD_MINOR_PATH(dst2_minor);
    job_item2->param.data              = (void *)job_item2;
    job_item2->param.dst_fmt           = job_fmt;
    job_item2->param.dst_bg_dim        = job_item->param.dst2_bg_dim;
    job_item2->param.dst_xy            = EM_PARAM_XY(dst2_x, dst2_y);
    job_item2->param.dst_dim           = EM_PARAM_DIM(dst2_w, dst2_h);
    job_item2->param.dst_crop_xy       = EM_PARAM_XY(dst2_crop_x, dst2_crop_y);
    job_item2->param.dst_crop_dim      = EM_PARAM_DIM(dst2_crop_w, dst2_crop_h);
    job_item2->param.md_enb            = 0;
    job_item2->param.dst2_fd           = 0;
    job_item2->param.auto_aspect_ratio = 0;                 ///< disable auto aspect ratio in sub-frame
    job_item2->param.auto_border       = dst2_auto_border;

    /* check buffer boundary */
    if(vcap_vg_check_buf_boundary(job_item2) < 0) {
        vcap_vg_job_item_free(pvg_info, job_item2);
        job_item2 = NULL;
    }

err:
    return (void *)job_item2;
}

static inline int vcap_vg_putjob2engine(struct vcap_vg_job_item_t *job_item, int add2eng)
{
    int idx, ret = 0;
    struct vcap_vg_job_item_t *curr_item, *next_item, *job_item2;
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)job_item->info;

    /* parse job input property */
    ret = vcap_vg_parse_in_property(job_item);
    if(ret < 0)
        goto err;

    /* check buffer boundary */
    ret = vcap_vg_check_buf_boundary(job_item);
    if(ret < 0)
        goto err;

    vcap_vg_preprocess(job_item, 0);

    if(add2eng) {
        /*
         * check subjob request,
         * allocate and put subjob to engine, the subjob duplicated from main job
         */
        switch(job_item->param.job_fmt) {
            case TYPE_YUV422_2FRAMES_2FRAMES:
            case TYPE_YUV422_FRAME_2FRAMES:
                if(job_item->param.dst2_fd != VCAP_VG_FD_SKIPJOB_TAG) {
                    if(VCAP_VG_FD_ENGINE_VIMODE(job_item->engine) == VCAP_VG_VIMODE_SPLIT) {
                        int err = 0;

                        list_for_each_entry_safe(curr_item, next_item, &job_item->m_job_head, m_job_list) {
                            curr_item->subjob = vcap_vg_alloc_subjob(curr_item, TYPE_YUV422_FRAME);
                            if(!curr_item->subjob) {
                                err++;
                                break;
                            }
                        }

                        if(err) {   ///< free all subjob
                            list_for_each_entry_safe(curr_item, next_item, &job_item->m_job_head, m_job_list) {
                                if(curr_item->subjob) {
                                    vcap_vg_job_item_free(pvg_info, curr_item->subjob);
                                    curr_item->subjob = NULL;
                                }
                            }
                        }
                        else {
                            if(job_item->subjob) {
                                list_for_each_entry_safe(curr_item, next_item, &job_item->m_job_head, m_job_list) {
                                    if(curr_item->subjob) {
                                        curr_item->type               = VCAP_VG_JOB_TYPE_MULTI;
                                        (curr_item->subjob)->root_job = job_item->subjob;
                                        list_add_tail(&(curr_item->subjob)->m_job_list, &(job_item->subjob)->m_job_head);
                                    }
                                }
                            }
                        }
                    }
                    else
                        job_item->subjob = vcap_vg_alloc_subjob(job_item, TYPE_YUV422_FRAME);
                }
                break;
            case TYPE_YUV422_FRAME_FRAME:   ///< only do main job
            default:
                break;
        }

        /* put main job to engine */
        ret = vcap_dev_putjob((void *)job_item);
        if(ret == 0) {
            if(job_item->root_job) {
                VCAP_VG_LOG(VCAP_VG_LOG_QUIET, job_item->engine, job_item->minor,
                            "put job#%d to engine, ver:%d engine:%d minor:%d root#%d => fmt:0x%04x va:0x%08x pa:0x%08x size:%u\n",
                            job_item->job_id,
                            job_item->job_ver,
                            job_item->engine,
                            job_item->minor,
                            ((struct vcap_vg_job_item_t *)job_item->root_job)->job_id,
                            job_item->param.dst_fmt,
                            job_item->param.addr_va[0],
                            job_item->param.addr_pa[0],
                            job_item->param.out_size[0]);
            }
            else {
                VCAP_VG_LOG(VCAP_VG_LOG_QUIET, job_item->engine, job_item->minor,
                            "put job#%d to engine, ver:%d engine:%d minor:%d => fmt:0x%04x va:0x%08x pa:0x%08x size:%u\n",
                            job_item->job_id,
                            job_item->job_ver,
                            job_item->engine,
                            job_item->minor,
                            job_item->param.dst_fmt,
                            job_item->param.addr_va[0],
                            job_item->param.addr_pa[0],
                            job_item->param.out_size[0]);
            }
        }
        else
            goto free_subjob;

        idx = VCAP_VG_MAKE_IDX(pvg_info, job_item->param.ch, job_item->param.path);

        /* put subjob to engine */
        if(job_item->subjob) {
            if(vcap_dev_putjob((void *)job_item->subjob) != 0)
                goto free_subjob;

            /* update sub_fd */
            if(pvg_info->sub_fd[idx] != job_item->param.dst2_fd)
                pvg_info->sub_fd[idx] = job_item->param.dst2_fd;

            job_item2 = (struct vcap_vg_job_item_t *)job_item->subjob;
            VCAP_VG_LOG(VCAP_VG_LOG_QUIET, job_item2->engine, job_item2->minor,
                        "put job#%d(subjob) to engine, ver:%d engine:%d minor:%d root#%d => fmt:0x%04x va:0x%08x pa:0x%08x size:%u\n",
                        job_item2->job_id,
                        job_item2->job_ver,
                        job_item2->engine,
                        job_item2->minor,
                        ((struct vcap_vg_job_item_t *)job_item2->root_job)->job_id,
                        job_item2->param.dst_fmt,
                        job_item2->param.addr_va[0],
                        job_item2->param.addr_pa[0],
                        job_item2->param.out_size[0]);
        }
        else {
            if(pvg_info->sub_fd[idx] != 0)
                pvg_info->sub_fd[idx] = 0;  ///< clear sub_fd if normal job coming
        }
    }

err:
    return ret;

free_subjob:
    if(job_item->subjob) {
        if(VCAP_VG_FD_ENGINE_VIMODE(job_item->engine) == VCAP_VG_VIMODE_SPLIT) {
            list_for_each_entry_safe(curr_item, next_item, &job_item->m_job_head, m_job_list) {
                if(((u32)curr_item->subjob) == ((u32)job_item->subjob)) ///< subjob root
                    continue;
                else {
                    if(curr_item->subjob) {
                        list_del(&((curr_item->subjob)->m_job_list));
                        vcap_vg_job_item_free(pvg_info, curr_item->subjob);
                        curr_item->subjob = NULL;
                    }
                }
            }
        }
        vcap_vg_job_item_free(pvg_info, job_item->subjob);
        job_item->subjob = NULL;
    }

    return ret;
}

static int vcap_vg_putjob(void *param)
{
    int    idx, ret, split_sub_fail = 0, split_multi = 0;
    int    engine, minor;
    struct video_entity_job_t *job = (struct video_entity_job_t *)param;
    struct video_entity_job_t *next_job = NULL;
    int    multi_jobs = 0, job_cnt = 0;
    struct vcap_vg_job_item_t *job_item = NULL, *root_job_item = NULL;
    struct vcap_vg_grp_pool_t *grp_pool = NULL;
    struct vcap_vg_info_t *pvg_info;

    pvg_info = vcap_vg_get_info(job->entity);
    if(pvg_info == NULL) {
        VCAP_VG_LOG(VCAP_VG_LOG_ERROR, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd),
                    "%s: invalid entity\n", __FUNCTION__);
        goto exit;
    }

    down(&pvg_info->sema_lock);

    /* check group job running mode */
    if(vcap_vg_grp_run_mode == VCAP_VG_GRP_RUN_MODE_WAIT_OLD_VER_DONE) {
        if(job->group_id && (VCAP_VG_FD_ENGINE_VIMODE(ENTITY_FD_ENGINE(job->fd)) != VCAP_VG_VIMODE_SPLIT)) {
            /* lookup or allocate group pool */
            grp_pool = (struct vcap_vg_grp_pool_t *)vcap_vg_group_pool_lookup_alloc(pvg_info, job->group_id, job->ver);
            if(!grp_pool) {
                job->status    = JOB_STATUS_FAIL;
                job->root_time = get_gm_jiffies();
                VCAP_VG_LOG(VCAP_VG_LOG_ERROR, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd),
                            "job#%d fd(0x%08x) allocate job group pool failed\n", job->id, job->fd);
                goto end;
            }
        }
    }

    do {
        /* get engine and minor */
        engine = ENTITY_FD_ENGINE(job->fd);
        minor  = ENTITY_FD_MINOR(job->fd);

        /* check job fd */
        if((VCAP_VG_FD_ENGINE_CH(engine) >= pvg_info->entity.engines) ||
           (VCAP_VG_FD_MINOR_PATH(minor) >= pvg_info->entity.minors)) {
            job->status    = JOB_STATUS_FAIL;
            job->root_time = get_gm_jiffies();
            next_job       = job->next_job;
            VCAP_VG_LOG(VCAP_VG_LOG_ERROR, engine, minor, "job#%d fd(0x%08x) invalid\n", job->id, job->fd);
            if(root_job_item)
                continue;
            else
                goto end;
        }

        /* allocate job */
        job_item = vcap_vg_job_item_alloc(pvg_info);
        if(!job_item) {
            job->status    = JOB_STATUS_FAIL;
            job->root_time = get_gm_jiffies();
            next_job       = job->next_job;
            VCAP_VG_LOG(VCAP_VG_LOG_ERROR, engine, minor, "error to put job#%d fd(0x%08x), no free memory!\n", job->id, job->fd);
            if(root_job_item)
                continue;
            else
                goto end;
        }
        memset(job_item, 0, sizeof(struct vcap_vg_job_item_t));

        job_item->engine     = engine;
        job_item->minor      = minor;
        job_item->job        = job;
        job_item->job_id     = job->id;
        job_item->job_ver    = job->ver;
        job_item->private    = (void *)pvg_info->private;
        job_item->info       = (void *)pvg_info;
        job_item->timestamp  = 0;
        job_item->puttime    = jiffies;
        job_item->starttime  = 0;
        job_item->finishtime = 0;

        /* set job status */
        if((grp_pool) &&
           (grp_pool->ver != job->ver) &&
           (vcap_vg_group_pool_get_run_ver_cnt(grp_pool) > vcap_vg_grp_run_ver_thres)) {
            /* group job version change, check not finish old version job count to decide job status */
            job_item->status = VCAP_VG_JOB_STATUS_KEEP;
        }
        else {
            job_item->status = VCAP_VG_JOB_STATUS_STANDBY;
        }

        INIT_LIST_HEAD(&job_item->engine_list);
        INIT_LIST_HEAD(&job_item->minor_list);
        INIT_LIST_HEAD(&job_item->m_job_head);
        INIT_LIST_HEAD(&job_item->m_job_list);
        INIT_LIST_HEAD(&job_item->group_list);

        if((next_job=job->next_job) != NULL) {
            multi_jobs++;
            if(multi_jobs == 1) {   ///< record root job
                root_job_item = job_item;
                job_item->need_callback = 1;
            }
        }

        if(multi_jobs) {            ///< has multi jobs
            job_item->root_job = (void *)root_job_item;
            job_item->type     = VCAP_VG_JOB_TYPE_MULTI;
            if(VCAP_VG_FD_ENGINE_VIMODE(engine) == VCAP_VG_VIMODE_SPLIT) {
                split_multi    = 1;
                split_sub_fail = 0;
            }
        } else {
            job_item->root_job      = root_job_item = (void *)job_item;
            job_item->type          = VCAP_VG_JOB_TYPE_NORMAL;
            job_item->need_callback = 1;
        }

        if(split_multi) {   ///< split channel multi-job, link job to m_job_head and only put root job to engine
            if(((u32)job_item) != ((u32)root_job_item)) {
                ret = vcap_vg_putjob2engine(job_item, 0);
                if(ret < 0) {
                    job_item->status    = VCAP_VG_JOB_STATUS_FAIL;
                    job_item->timestamp = get_gm_jiffies();
                    split_sub_fail++;
                }
            }

            list_add_tail(&job_item->m_job_list, &root_job_item->m_job_head);

            if(next_job == NULL) { ///< put root job to engine
                struct vcap_vg_job_item_t *curr_item, *next_item;

                if(split_sub_fail) {
                    job_item->status    = VCAP_VG_JOB_STATUS_FAIL;
                    job_item->timestamp = get_gm_jiffies();
                }
                else {
                    ret = vcap_vg_putjob2engine(root_job_item, 1);
                    if(ret < 0) {
                        job_item->status    = VCAP_VG_JOB_STATUS_FAIL;
                        job_item->timestamp = get_gm_jiffies();
                    }
                }

                list_for_each_entry_safe(curr_item, next_item, &root_job_item->m_job_head, m_job_list) {
                    list_add_tail(&curr_item->engine_list, &pvg_info->engine_head[VCAP_VG_FD_ENGINE_CH(curr_item->engine)]);
                    idx = VCAP_VG_MAKE_IDX(pvg_info, VCAP_VG_FD_ENGINE_CH(curr_item->engine), VCAP_VG_FD_MINOR_PATH(curr_item->minor));
                    list_add_tail(&curr_item->minor_list, &pvg_info->minor_head[idx]);
                }

                /* update fd */
                idx = VCAP_VG_MAKE_IDX(pvg_info, VCAP_VG_FD_ENGINE_CH(root_job_item->engine), VCAP_VG_FD_MINOR_PATH(root_job_item->minor));
                if(pvg_info->fd[idx] != ((struct video_entity_job_t *)(root_job_item->job))->fd)
                    pvg_info->fd[idx] = ((struct video_entity_job_t *)(root_job_item->job))->fd;

                job_cnt++;
            }
        }
        else {
            ret = vcap_vg_putjob2engine(job_item, 1);
            if(ret < 0) {
                job_item->status    = VCAP_VG_JOB_STATUS_FAIL;
                job_item->timestamp = get_gm_jiffies();
            }

            /* link job to root job m_job list head */
            if(multi_jobs)
                list_add_tail(&job_item->m_job_list, &root_job_item->m_job_head);

            /* link job to engine and minor job list head */
            list_add_tail(&job_item->engine_list, &pvg_info->engine_head[VCAP_VG_FD_ENGINE_CH(job_item->engine)]);
            idx = VCAP_VG_MAKE_IDX(pvg_info, VCAP_VG_FD_ENGINE_CH(job_item->engine), VCAP_VG_FD_MINOR_PATH(job_item->minor));
            list_add_tail(&job_item->minor_list, &pvg_info->minor_head[idx]);

            /* link root job to group pool list head */
            if(grp_pool && (job_item == root_job_item))
                list_add_tail(&job_item->group_list, &grp_pool->job_head);

            /* update fd */
            if(pvg_info->fd[idx] != ((struct video_entity_job_t *)(job_item->job))->fd)
                pvg_info->fd[idx] = ((struct video_entity_job_t *)(job_item->job))->fd;

            job_cnt++;
        }
    } while((job=next_job));

end:
    up(&pvg_info->sema_lock);

exit:
    if(job_cnt)
        return JOB_STATUS_ONGOING;
    else
        return JOB_STATUS_FAIL;
}

static int vcap_vg_stop(void *param, int engine, int minor)
{
    int idx, ret = 0;
    int ch, path;
    struct vcap_vg_info_t *pvg_info;
    struct video_entity_t *entity = (struct video_entity_t *)param;

    pvg_info = vcap_vg_get_info(entity);
    if(pvg_info == NULL) {
        VCAP_VG_LOG(VCAP_VG_LOG_ERROR, engine, minor, "%s: invalid entity\n", __FUNCTION__);
        ret = -1;
        goto end;
    }

    ch   = VCAP_VG_FD_ENGINE_CH(engine);
    path = VCAP_VG_FD_MINOR_PATH(minor);

    if(ch >= pvg_info->entity.engines || path >= pvg_info->entity.minors) {
        VCAP_VG_LOG(VCAP_VG_LOG_ERROR, engine, minor, "%s: invalid engine:%d minor:%d\n", __FUNCTION__, engine, minor);
        ret = -1;
        goto end;
    }

    idx = VCAP_VG_MAKE_IDX(pvg_info, ch, path);

    /* stop sub-path if man-path ever put subjob to sub-path */
    if(pvg_info->sub_fd[idx] != 0)
        vcap_dev_stop(pvg_info->private, ch, VCAP_VG_FD_MINOR_PATH(ENTITY_FD_MINOR(pvg_info->sub_fd[idx])));

    ret = vcap_dev_stop(pvg_info->private, ch, path);
    if(ret < 0)
        goto end;

    /* clear fd and sub_fd */
    down(&pvg_info->sema_lock);
    pvg_info->fd[idx]     = 0;
    pvg_info->sub_fd[idx] = 0;
    up(&pvg_info->sema_lock);

end:
    if(ret < 0) {
        VCAP_VG_LOG(VCAP_VG_LOG_ERROR, engine, minor, "engine:%d minor:%d stop failed!\n", engine, minor);
    }
    return ret;
}

static int vcap_vg_queryid(void *param, char *str)
{
    int i;
    struct vcap_vg_info_t *pvg_info;
    struct video_entity_t *entity = (struct video_entity_t *)param;

    pvg_info = vcap_vg_get_info(entity);
    if(pvg_info == NULL) {
        vcap_err("%s: invalid entity\n", __FUNCTION__);
        return -1;
    }

    for(i=0; i<ARRAY_SIZE(property_map); i++) {
        if(strcmp(property_map[i].name, str) == 0) {
            return property_map[i].id;
        }
    }
    vcap_err("%s: error to find property name %s\n", __FUNCTION__, str);
    return -1;
}

static int vcap_vg_querystr(void *param, int id, char *string)
{
    int i;
    struct vcap_vg_info_t *pvg_info;
    struct video_entity_t *entity = (struct video_entity_t *)param;

    pvg_info = vcap_vg_get_info(entity);
    if(pvg_info == NULL) {
        vcap_err("%s: invalid entity\n", __FUNCTION__);
        return -1;
    }

    for(i=0; i<ARRAY_SIZE(property_map); i++) {
        if(property_map[i].id == id) {
            memcpy(string, property_map[i].name, sizeof(property_map[i].name));
            return 0;
        }
    }
    vcap_err("%s: Error to find id %d\n", __FUNCTION__, id);
    return -1;
}

static int vcap_vg_getproperty(void *param, int engine, int minor, char *string)
{
    int i, id, idx, value = -1;
    int ch, path;
    struct vcap_vg_info_t *pvg_info;
    struct video_entity_t *entity = (struct video_entity_t *)param;

    pvg_info = vcap_vg_get_info(entity);
    if(pvg_info == NULL) {
        VCAP_VG_LOG(VCAP_VG_LOG_ERROR, engine, minor, "%s: invalid entity\n", __FUNCTION__);
        goto end;
    }

    ch   = VCAP_VG_FD_ENGINE_CH(engine);
    path = VCAP_VG_FD_MINOR_PATH(minor);

    if (ch > pvg_info->entity.engines) {
        VCAP_VG_LOG(VCAP_VG_LOG_ERROR, engine, minor, "%s: engine#%d invalid(ch#%d > MAX#%d)\n", __FUNCTION__, engine, ch, pvg_info->entity.engines);
        goto end;
    }

    if (path > pvg_info->entity.minors) {
        VCAP_VG_LOG(VCAP_VG_LOG_ERROR, engine, minor, "%s: minor#%d invalid(path#%d > MAX#%d)\n", __FUNCTION__, minor, path, pvg_info->entity.minors);
        goto end;
    }

    idx = VCAP_VG_MAKE_IDX(pvg_info, ch, path);

    /* query property id */
    id = vcap_vg_queryid(param, string);

    /* get property value of current running job */
    for(i=0; i<VCAP_VG_MAX_RECORD; i++) {
        if(pvg_info->curr_property[idx].property[i].id == 0) ///< end of property
            break;

        if(pvg_info->curr_property[idx].property[i].id == id) {
            value = pvg_info->curr_property[idx].property[i].value;
            break;
        }
    }

end:
    return value;
}

static inline int vcap_vg_set_out_property(struct vcap_vg_job_item_t *job_item)
{
    int i = 0;
    struct video_entity_job_t  *job      = (struct video_entity_job_t *)job_item->job;
    struct video_property_t    *property = job->out_prop;
    struct vcap_vg_job_param_t *param    = &job_item->param;

    property[i].id    = ID_SRC_BG_DIM;
    property[i].value = param->src_bg_dim;
    i++;

    property[i].id    = ID_SRC_INTERLACE;
    property[i].value = param->src_interlace;
    i++;

    property[i].id    = ID_SRC_TYPE;
    property[i].value = param->src_type;
    i++;

    property[i].id    = ID_FPS_RATIO;
    property[i].value = param->fps_ratio;
    i++;

    property[i].id    = ID_RUNTIME_FRAME_RATE;
    property[i].value = param->runtime_frame_rate;
    i++;

    property[i].id    = ID_SRC_FRAME_RATE;
    property[i].value = param->src_frame_rate;
    i++;

    if(param->frm_2frm_field) {
        property[i].id    = ID_SOURCE_FIELD;
        property[i].value = param->src_field;
        i++;
    }

    if((job_item->status == VCAP_VG_JOB_STATUS_FAIL) && vcap_vg_buf_clean) {
        property[i].id    = ID_BUF_CLEAN;
        property[i].value = 1;
        i++;
    }

    property[i].id    = ID_NULL;
    property[i].value = EM_PROPERTY_NULL_VALUE;
    i++;

    return 0;
}

static struct video_entity_ops_t vcap_vg_ops = {
    putjob:         &vcap_vg_putjob,
    stop:           &vcap_vg_stop,
    queryid:        &vcap_vg_queryid,
    querystr:       &vcap_vg_querystr,
    getproperty:    &vcap_vg_getproperty,
    register_clock: NULL,                   ///< not used in vcap entity
};

void vcap_vg_callback_scheduler(void *data) ///< for workqueue data should be work_struct, for kthread data should be vcap_vg_info_t
{
    int i, j, idx;
    struct video_entity_job_t *job;
    struct vcap_vg_job_item_t *job_item, *job_item_next, *root_job_item;
#ifdef CFG_VG_CALLBACK_USE_KTHREAD
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)data;
#else
    struct work_struct    *work     = (struct work_struct *)data;
    struct delayed_work   *dwork    = container_of(work, struct delayed_work, work);
    struct vcap_vg_info_t *pvg_info = container_of(dwork, struct vcap_vg_info_t, callback_work);
#endif

    down(&pvg_info->sema_lock);

    for(i=0; i<pvg_info->entity.engines; i++) {
        for(j=0; j<pvg_info->entity.minors; j++) {
            idx = VCAP_VG_MAKE_IDX(pvg_info, i, j);
            list_for_each_entry_safe(job_item, job_item_next, &pvg_info->minor_head[idx], minor_list) {
                job            = (struct video_entity_job_t *)job_item->job;
                root_job_item  = (struct vcap_vg_job_item_t *)job_item->root_job;   ///< root_job should be pointer the job_item in single job type

                if(job_item->status == VCAP_VG_JOB_STATUS_FINISH) {
                    /* check subjob status */
                    if(job_item->subjob) {
                        if((job_item->subjob)->status == VCAP_VG_JOB_STATUS_FINISH) {
                            vcap_vg_job_item_free(pvg_info, job_item->subjob);     ///< free subjob item
                        }
                        else if((job_item->subjob)->status == VCAP_VG_JOB_STATUS_FAIL)
                            goto fail_job;                      ///< main job finish but subjob fail
                        else
                            break;                              ///< subjob not ready, go to check next minor
                    }
                    else if((job_item->param.job_fmt == TYPE_YUV422_2FRAMES_2FRAMES) || (job_item->param.job_fmt == TYPE_YUV422_FRAME_2FRAMES)) {
                        if(job_item->param.dst2_fd != VCAP_VG_FD_SKIPJOB_TAG)
                            goto fail_job;                          ///< main job finish but no subjob
                    }

                    vcap_vg_set_out_property(job_item);
                    vcap_vg_postprocess(job_item, 0);
                    job->status    = JOB_STATUS_FINISH;
                    job->root_time = job_item->timestamp;
                    list_del(&job_item->engine_list);
                    list_del(&job_item->minor_list);

                    if(job_item->type == VCAP_VG_JOB_TYPE_MULTI) {
                        list_del(&job_item->m_job_list);
                        if(list_empty(&root_job_item->m_job_head)) {    ///< all job done of multi-job
                            if(root_job_item->need_callback) {
                                job = (struct video_entity_job_t *)root_job_item->job;
                                job->callback(job);
                                VCAP_VG_LOG(VCAP_VG_LOG_QUIET, root_job_item->engine, root_job_item->minor,
                                            "multi-job#%d callback finish, ver:%d root_engine:%d root_minor:%d\n",
                                            root_job_item->job_id,
                                            root_job_item->job_ver,
                                            root_job_item->engine,
                                            root_job_item->minor);
                            }
                        }
                        if((u32)job_item != (u32)root_job_item) {         ///< root job will be free after all multi-job done
                            vcap_vg_job_item_free(pvg_info, job_item);
                        }
                    }
                    else {
                        if(job_item->need_callback) {
                            job->callback(job);
                            VCAP_VG_LOG(VCAP_VG_LOG_QUIET, job_item->engine, job_item->minor,
                                        "job#%d callback finish, ver:%d engine:%d minor:%d\n",
                                        job_item->job_id,
                                        job_item->job_ver,
                                        job_item->engine,
                                        job_item->minor);
                        }
                    }
                    if(root_job_item && list_empty(&root_job_item->m_job_head)) {
                        if(!list_empty(&root_job_item->group_list))   ///< remove root job from group pool list
                            list_del(&root_job_item->group_list);
                        vcap_vg_job_item_free(pvg_info, root_job_item);
                    }

                    continue;
                } else if(job_item->status == VCAP_VG_JOB_STATUS_FAIL) {
fail_job:
                    /* check subjob status */
                    if(job_item->subjob) {
                        if(((job_item->subjob)->status == VCAP_VG_JOB_STATUS_FINISH) || ((job_item->subjob)->status == VCAP_VG_JOB_STATUS_FAIL)) {
                            vcap_vg_job_item_free(pvg_info, job_item->subjob); ///< free subjob item
                        }
                        else
                            break;                          ///< subjob not ready, go to check next minor
                    }

                    vcap_vg_set_out_property(job_item);
                    job->status    = JOB_STATUS_FAIL;
                    job->root_time = job_item->timestamp;
                    list_del(&job_item->engine_list);
                    list_del(&job_item->minor_list);

                    if(job_item->type == VCAP_VG_JOB_TYPE_MULTI) {
                        list_del(&job_item->m_job_list);
                        if(list_empty(&root_job_item->m_job_head)) {    ///< all job done of multi-job
                            if(root_job_item->need_callback) {
                                job = (struct video_entity_job_t *)root_job_item->job;
                                job->callback(job);
                                VCAP_VG_LOG(VCAP_VG_LOG_QUIET, root_job_item->engine, root_job_item->minor,
                                            "multi-job#%d callback fail, ver:%d root_engine:%d root_minor:%d\n",
                                            root_job_item->job_id,
                                            root_job_item->job_ver,
                                            root_job_item->engine,
                                            root_job_item->minor);
                            }
                        }
                        if((u32)job_item != (u32)root_job_item) {         ///< root job will be free after all multi-job done
                            vcap_vg_job_item_free(pvg_info, job_item);
                        }
                    }
                    else {
                        if(job_item->need_callback) {
                            job->callback(job);
                            VCAP_VG_LOG(VCAP_VG_LOG_QUIET, job_item->engine, job_item->minor,
                                        "job#%d callback fail, ver:%d engine:%d minor:%d\n",
                                        job_item->job_id,
                                        job_item->job_ver,
                                        job_item->engine,
                                        job_item->minor);
                        }
                    }
                    if(root_job_item && list_empty(&root_job_item->m_job_head)) {
                        if(!list_empty(&root_job_item->group_list))   ///< remove root job from group pool list
                            list_del(&root_job_item->group_list);
                        vcap_vg_job_item_free(pvg_info, root_job_item);
                    }

                    continue;
                }
                break;
            }
        }
    }

    vcap_vg_group_pool_job_check(pvg_info);

    up(&pvg_info->sema_lock);
}

#ifdef CFG_VG_CALLBACK_USE_KTHREAD
static inline void vcap_vg_callback_wakeup(struct vcap_vg_info_t *pvg_info)
{
    atomic_inc(&pvg_info->callback_wakeup_event);
    wake_up(&pvg_info->callback_event_wait_q);
}

static int vcap_vg_callback_thread(void *data)
{
    int status;
    struct vcap_vg_info_t *pvg_info = (struct vcap_vg_info_t *)data;

    do {
            status = wait_event_timeout(pvg_info->callback_event_wait_q, atomic_read(&pvg_info->callback_wakeup_event), msecs_to_jiffies(1000));
            if(status == 0)
                continue;   ///< event wait timeout

            atomic_dec(&pvg_info->callback_wakeup_event);

            /* callback process */
            vcap_vg_callback_scheduler(pvg_info);

    } while(!kthread_should_stop());

    return 0;
}
#endif

int vcap_vg_init(int id, char *name, int engine_num, int minor_num, void *private_data)
{
    int i;
    int ret = 0;

    if(id >= VCAP_VG_ENTITY_MAX || engine_num <= 0 || minor_num <= 0)
        return -1;

    /* allocate vcap_vg_info */
    vcap_vg_info[id] = kzalloc(sizeof(struct vcap_vg_info_t), GFP_KERNEL);
    if(vcap_vg_info[id] == NULL) {
        vcap_err("allocate vcap_vg_info failed!\n");
        ret = -ENOMEM;
        goto err;
    }

    /* assign vg info name */
    sprintf(vcap_vg_info[id]->name, "vcap_vg%d", id);

    /* allocate engine job list */
    vcap_vg_info[id]->engine_head = kzalloc(sizeof(struct list_head)*engine_num, GFP_KERNEL);
    if(vcap_vg_info[id]->engine_head == NULL) {
        vcap_err("allocate engine job list failed!\n");
        ret = -ENOMEM;
        goto err;
    }

    /* allocate minor job list */
    vcap_vg_info[id]->minor_head = kzalloc(sizeof(struct list_head)*engine_num*minor_num, GFP_KERNEL);
    if(vcap_vg_info[id]->minor_head == NULL) {
        vcap_err("allocate minor job list failed!\n");
        ret = -ENOMEM;
        goto err;
    }

    /* allocate fd */
    vcap_vg_info[id]->fd = kzalloc(sizeof(unsigned int)*engine_num*minor_num, GFP_KERNEL);
    if(vcap_vg_info[id]->fd == NULL) {
        vcap_err("allocate fd buffer failed!\n");
        ret = -ENOMEM;
        goto err;
    }

    /* allocate sub_fd */
    vcap_vg_info[id]->sub_fd = kzalloc(sizeof(unsigned int)*engine_num*minor_num, GFP_KERNEL);
    if(vcap_vg_info[id]->sub_fd == NULL) {
        vcap_err("allocate sub_fd buffer failed!\n");
        ret = -ENOMEM;
        goto err;
    }

    /* allocate record property */
    vcap_vg_info[id]->curr_property = kzalloc(sizeof(struct vcap_vg_property_rec_t)*engine_num*minor_num, GFP_KERNEL);
    if(vcap_vg_info[id]->curr_property == NULL) {
        vcap_err("allocate record property buffer failed!\n");
        ret = -ENOMEM;
        goto err;
    }

    /* allocate performance */
    vcap_vg_info[id]->perf = kzalloc(sizeof(struct vcap_vg_perf_t)*engine_num*minor_num, GFP_KERNEL);
    if(vcap_vg_info[id]->perf == NULL) {
        vcap_err("allocate performance buffer failed!\n");
        ret = -ENOMEM;
        goto err;
    }

    /* create job cache buffer */
    vcap_vg_info[id]->job_cache = kmem_cache_create(vcap_vg_info[id]->name, sizeof(struct vcap_vg_job_item_t), 0, 0, NULL);
    if(!vcap_vg_info[id]->job_cache) {
        vcap_err("allocate job cache buffer failed!\n");
        ret = -ENOMEM;
        goto err;
    }

    vcap_vg_info[id]->index   = id;
    vcap_vg_info[id]->private = private_data;

    for(i=0; i<engine_num; i++)
        INIT_LIST_HEAD(&vcap_vg_info[id]->engine_head[i]);

    for(i=0; i<engine_num*minor_num; i++)
        INIT_LIST_HEAD(&vcap_vg_info[id]->minor_head[i]);

    INIT_LIST_HEAD(&vcap_vg_info[id]->grp_pool_head);

    vcap_vg_info[id]->callback_period  = VCAP_VG_CALLBACK_PERIOD;
    vcap_vg_info[id]->perf_util_period = VCAP_VG_UTILIZATION_PERIOD;

    /* init lock */
    spin_lock_init(&vcap_vg_info[id]->lock);

    /* init semaphore lock */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
    sema_init(&vcap_vg_info[id]->sema_lock, 1);
#else
    init_MUTEX(&vcap_vg_info[id]->sema_lock);
#endif

#ifdef CFG_VG_CALLBACK_USE_KTHREAD
    init_waitqueue_head(&vcap_vg_info[id]->callback_event_wait_q);
    atomic_set(&vcap_vg_info[id]->callback_wakeup_event , 0);
    vcap_vg_info[id]->callback_thread = kthread_create(vcap_vg_callback_thread, (void *)vcap_vg_info[id], "vcap_vg_cb");
    if(IS_ERR(vcap_vg_info[id]->callback_thread)) {
        vcap_vg_info[id]->callback_thread = 0;
        ret = -EFAULT;
        goto err;
    }
    wake_up_process(vcap_vg_info[id]->callback_thread);
#else
    /* init work */
    INIT_DELAYED_WORK(&vcap_vg_info[id]->callback_work, 0);

    /* create vcap work queue */
    vcap_vg_info[id]->workq = create_workqueue(name);
    if(vcap_vg_info[id]->workq == NULL) {
        vcap_err("error to create %s workqueue\n", name);
        ret = -EFAULT;
        goto err;
    }
#endif

    /* init proc */
    ret = vcap_vg_proc_init(name, vcap_vg_info[id]);
    if(ret < 0) {
        vcap_err(" %s videograph proc node init failed!\n", name);
        goto err;
    }

    /* register vcap entity to videograph */
    vcap_vg_info[id]->entity.type    = TYPE_CAPTURE;
    vcap_vg_info[id]->entity.engines = engine_num;      ///< capture channel max number
    vcap_vg_info[id]->entity.minors  = minor_num;       ///< capture path max number
    vcap_vg_info[id]->entity.ops     = &vcap_vg_ops;
    strlcpy(vcap_vg_info[id]->entity.name, name, MAX_NAME_LEN);
    ret = video_entity_register(&vcap_vg_info[id]->entity);
    if(ret < 0) {
        vcap_err("register %s entity to videograph failed!\n", name);
        goto err;
    }

    return ret;

err:
    if(vcap_vg_info[id]) {
#ifdef CFG_VG_CALLBACK_USE_KTHREAD
        if(vcap_vg_info[id]->callback_thread)
            kthread_stop(vcap_vg_info[id]->callback_thread);
#else
        if(vcap_vg_info[id]->workq)
            destroy_workqueue(vcap_vg_info[id]->workq);
#endif

        if(vcap_vg_info[id]->job_cache)
            kmem_cache_destroy(vcap_vg_info[id]->job_cache);

        if(vcap_vg_info[id]->perf)
            kfree(vcap_vg_info[id]->perf);

        if(vcap_vg_info[id]->curr_property)
            kfree(vcap_vg_info[id]->curr_property);

        if(vcap_vg_info[id]->engine_head)
            kfree(vcap_vg_info[id]->engine_head);

        if(vcap_vg_info[id]->minor_head)
            kfree(vcap_vg_info[id]->minor_head);

        if(vcap_vg_info[id]->sub_fd)
            kfree(vcap_vg_info[id]->sub_fd);

        if(vcap_vg_info[id]->fd)
            kfree(vcap_vg_info[id]->fd);

        vcap_vg_proc_remove(id);

        kfree(vcap_vg_info[id]);
    }
    return ret;
}

void vcap_vg_close(int id)
{
    if(vcap_vg_info[id]) {
#ifdef CFG_VG_CALLBACK_USE_KTHREAD
        /* stop callback thread */
        if(vcap_vg_info[id]->callback_thread)
            kthread_stop(vcap_vg_info[id]->callback_thread);
#else
        /* cancel all pending work */
        cancel_delayed_work_sync(&vcap_vg_info[id]->callback_work);

        /* destroy workqueue */
        if(vcap_vg_info[id]->workq)
            destroy_workqueue(vcap_vg_info[id]->workq);
#endif

        /* free job cache buffer */
        if(vcap_vg_info[id]->job_cache)
            kmem_cache_destroy(vcap_vg_info[id]->job_cache);

        /* free performance */
        if(vcap_vg_info[id]->perf)
            kfree(vcap_vg_info[id]->perf);

        /* free record property */
        if(vcap_vg_info[id]->curr_property)
            kfree(vcap_vg_info[id]->curr_property);

        /* free engine head */
        if(vcap_vg_info[id]->engine_head)
            kfree(vcap_vg_info[id]->engine_head);

        /* free minor head */
        if(vcap_vg_info[id]->minor_head)
            kfree(vcap_vg_info[id]->minor_head);

        /* free fd buffer */
        if(vcap_vg_info[id]->fd)
            kfree(vcap_vg_info[id]->fd);

        /* remove proc */
        vcap_vg_proc_remove(id);

        /*un-register vcap entity from videograph */
        video_entity_deregister(&vcap_vg_info[id]->entity);

        /* check allocated memory count */
        if(vcap_vg_info[id]->malloc_cnt)
            vcap_warn("allocated memory buffer not all free(%d)\n", vcap_vg_info[id]->malloc_cnt);

        kfree(vcap_vg_info[id]);
    }
}

static inline void vcap_vg_record_property(struct vcap_vg_job_item_t *job_item)
{
    int idx, i = 0;
    struct video_entity_job_t *job      = (struct video_entity_job_t *)job_item->job;
    struct video_property_t   *property = job->in_prop;
    struct vcap_vg_info_t     *pvg_info = (struct vcap_vg_info_t *)job_item->info;

    idx = VCAP_VG_MAKE_IDX(pvg_info, job_item->param.ch, job_item->param.path);

    pvg_info->curr_property[idx].job_id = job->id;
    while((property[i].id != 0) && (i < VCAP_VG_MAX_RECORD)) {
        memcpy(&pvg_info->curr_property[idx].property[i], &property[i], sizeof(struct video_property_t));
        i++;
    }

    if(i < VCAP_VG_MAX_RECORD)
        pvg_info->curr_property[idx].property[i].id = 0;  ///< end of property
}

void vcap_vg_mark_engine_start(void *param)
{
    int idx;
    struct vcap_vg_job_item_t *job_item = (struct vcap_vg_job_item_t *)param;
    struct vcap_vg_info_t     *pvg_info = (struct vcap_vg_info_t *)job_item->info;
    int engine, minor;
    unsigned long flags;

    engine = VCAP_VG_FD_ENGINE_CH(job_item->engine);
    minor  = VCAP_VG_FD_MINOR_PATH(job_item->minor);
    idx    = VCAP_VG_MAKE_IDX(pvg_info, engine, minor);

    spin_lock_irqsave(&pvg_info->lock, flags);  ///< this api may used in interrupt, should use spin lock to protect resource

    /* record property of current job for query */
    vcap_vg_record_property(job_item);

    if(job_item->starttime != 0)
        VCAP_VG_LOG(VCAP_VG_LOG_WARNING, engine, minor, "%s: warning to nested use engine%d-minor%d!\n", __FUNCTION__, engine, minor);

    job_item->starttime = jiffies;

    if(VCAP_VG_FD_ENGINE_VIMODE(job_item->engine) == VCAP_VG_VIMODE_SPLIT) {
        struct vcap_vg_job_item_t *curr_item, *next_item;
        list_for_each_entry_safe(curr_item, next_item, &job_item->m_job_head, m_job_list) {
            curr_item->starttime = job_item->starttime;
        }
    }

    if(pvg_info->perf[idx].util_start == 0) {
        pvg_info->perf[idx].util_start = job_item->starttime;
        pvg_info->perf[idx].times      = 0;
        pvg_info->perf[idx].prev_end   = job_item->starttime;
    }

    spin_unlock_irqrestore(&pvg_info->lock, flags);
}

void vcap_vg_mark_engine_finish(void *param)
{
    int idx;
    int engine, minor;
    struct vcap_vg_job_item_t *job_item = (struct vcap_vg_job_item_t *)param;
    struct vcap_vg_info_t     *pvg_info = (struct vcap_vg_info_t *)job_item->info;
    unsigned long flags;

    engine = VCAP_VG_FD_ENGINE_CH(job_item->engine);
    minor  = VCAP_VG_FD_MINOR_PATH(job_item->minor);
    idx    = VCAP_VG_MAKE_IDX(pvg_info, engine, minor);

    spin_lock_irqsave(&pvg_info->lock, flags);   ///< this api may used in interrupt, should use spin lock to protect resource

    if(job_item->finishtime != 0)
        VCAP_VG_LOG(VCAP_VG_LOG_WARNING, engine, minor, "%s: warning to nested use engine%d-minor%d!\n", __FUNCTION__, engine, minor);

    job_item->starttime          = pvg_info->perf[idx].prev_end;    ///< link-list mode, the start time should be the end time of previous job
    job_item->finishtime         = jiffies;
    pvg_info->perf[idx].prev_end = job_item->finishtime;

    if(VCAP_VG_FD_ENGINE_VIMODE(job_item->engine) == VCAP_VG_VIMODE_SPLIT) {
        struct vcap_vg_job_item_t *curr_item, *next_item;
        list_for_each_entry_safe(curr_item, next_item, &job_item->m_job_head, m_job_list) {
            curr_item->starttime  = job_item->starttime;
            curr_item->finishtime = job_item->finishtime;
        }
    }

    if(job_item->finishtime > job_item->starttime)
        pvg_info->perf[idx].times += (job_item->finishtime - job_item->starttime);

    if(pvg_info->perf[idx].util_start <= job_item->finishtime) {
        if(job_item->finishtime - pvg_info->perf[idx].util_start >= pvg_info->perf_util_period*HZ) {
            unsigned int utilization;
            utilization=(unsigned int)((100*pvg_info->perf[idx].times)/(job_item->finishtime-pvg_info->perf[idx].util_start));
            if(utilization)
                pvg_info->perf[idx].util_record = utilization;
            pvg_info->perf[idx].util_start = 0;
            pvg_info->perf[idx].times      = 0;
        }
    }
    else {
        pvg_info->perf[idx].util_start = 0;
        pvg_info->perf[idx].times      = 0;
    }

    spin_unlock_irqrestore(&pvg_info->lock, flags);
}

void vcap_vg_trigger_callback_finish(void *param)
{
    unsigned long flags;
    struct vcap_vg_job_item_t *job_item = (struct vcap_vg_job_item_t *)param;
    struct vcap_vg_info_t     *pvg_info = (struct vcap_vg_info_t *)job_item->info;

    spin_lock_irqsave(&pvg_info->lock, flags);   ///< this api may used in interrupt, should use spin lock to protect resource

    job_item->status    = VCAP_VG_JOB_STATUS_FINISH;
    job_item->timestamp = get_gm_jiffies();

    if(VCAP_VG_FD_ENGINE_VIMODE(job_item->engine) == VCAP_VG_VIMODE_SPLIT) {    ///< update all split job status
        struct vcap_vg_job_item_t *curr_item, *next_item;
        list_for_each_entry_safe(curr_item, next_item, &job_item->m_job_head, m_job_list) {
            curr_item->status    = job_item->status;
            curr_item->timestamp = job_item->timestamp;
        }
    }

#ifdef CFG_VG_CALLBACK_USE_KTHREAD
    vcap_vg_callback_wakeup(pvg_info);
#else
    PREPARE_DELAYED_WORK(&pvg_info->callback_work, (void *)vcap_vg_callback_scheduler);
    queue_delayed_work(pvg_info->workq, &pvg_info->callback_work, msecs_to_jiffies(pvg_info->callback_period));
#endif

    VCAP_VG_LOG(VCAP_VG_LOG_QUIET, job_item->engine, job_item->minor,
                "job#%d trigger finish, ver:%d engine:%d minor:%d\n",
                job_item->job_id,
                job_item->job_ver,
                job_item->engine,
                job_item->minor);

    spin_unlock_irqrestore(&pvg_info->lock, flags);
}

void vcap_vg_trigger_callback_fail(void *param)
{
    unsigned long flags;
    struct vcap_vg_job_item_t *job_item = (struct vcap_vg_job_item_t *)param;
    struct vcap_vg_info_t     *pvg_info = (struct vcap_vg_info_t *)job_item->info;

    spin_lock_irqsave(&pvg_info->lock, flags);   ///< this api may used in interrupt, should use spin lock to protect resource

    job_item->status          = VCAP_VG_JOB_STATUS_FAIL;
    job_item->timestamp       = get_gm_jiffies();

    if(VCAP_VG_FD_ENGINE_VIMODE(job_item->engine) == VCAP_VG_VIMODE_SPLIT) {    ///< update all split job status
        struct vcap_vg_job_item_t *curr_item, *next_item;
        list_for_each_entry_safe(curr_item, next_item, &job_item->m_job_head, m_job_list) {
            curr_item->status          = job_item->status;
            curr_item->timestamp       = job_item->timestamp;
        }
    }

#ifdef CFG_VG_CALLBACK_USE_KTHREAD
    vcap_vg_callback_wakeup(pvg_info);
#else
    PREPARE_DELAYED_WORK(&pvg_info->callback_work, (void *)vcap_vg_callback_scheduler);
    queue_delayed_work(pvg_info->workq, &pvg_info->callback_work, msecs_to_jiffies(pvg_info->callback_period));
#endif

    VCAP_VG_LOG(VCAP_VG_LOG_QUIET, job_item->engine, job_item->minor,
                "job#%d trigger fail, ver:%d engine:%d minor:%d\n",
                job_item->job_id,
                job_item->job_ver,
                job_item->engine,
                job_item->minor);

    spin_unlock_irqrestore(&pvg_info->lock, flags);
}

void vcap_vg_panic_dump_job(void)
{
    int i, engine, minor, idx;
    unsigned long flags;
    struct vcap_vg_info_t *pvg_info;
    struct vcap_vg_job_item_t *job_item, *next_item;
    char *type_msg[]   = {"S", "M"};
    char *status_msg[] = {"STANDBY", "ONGOING", "FINISH", "FAIL", "KEEP"};

    for(i=0; i<VCAP_VG_ENTITY_MAX; i++) {
        pvg_info = vcap_vg_info[i];
        if(!pvg_info)
            continue;

        spin_lock_irqsave(&pvg_info->lock, flags);

        for(engine=0; engine<pvg_info->entity.engines; engine++) {
            for(minor=0; minor<pvg_info->entity.minors; minor++) {
                idx = VCAP_VG_MAKE_IDX(pvg_info, engine, minor);
                list_for_each_entry_safe(job_item, next_item, &pvg_info->minor_head[idx], minor_list) {
                    vcap_log("ch#%d-p%d pending job#%d engine:%d minor:%d type:%s status:%s call:%d root:%d\n",
                              engine, minor,
                              job_item->job_id,
                              job_item->engine,
                              job_item->minor,
                              type_msg[job_item->type],
                              status_msg[job_item->status],
                              job_item->need_callback,
                              ((struct vcap_vg_job_item_t *)job_item->root_job)->job_id);
                }
            }

        }

        spin_unlock_irqrestore(&pvg_info->lock, flags);
    }
}

int vcap_vg_notify(int id, int ch, int path, VCAP_VG_NOTIFY_T ntype)    ///< path = -1 means lookup an active path
{
    unsigned long flags;
    int i, idx = -1, ret = 0;
    struct vcap_vg_info_t *pvg_info;

    if(id >= VCAP_VG_ENTITY_MAX)
        return -1;

    pvg_info = vcap_vg_info[id];
    if(!pvg_info || ch >= pvg_info->entity.engines || path >= pvg_info->entity.minors)
        return -1;

    spin_lock_irqsave(&pvg_info->lock, flags);   ///< this api may used in interrupt, should use spin lock to protect resource

    if(path < 0) {  ///< lookup an active path
        for(i=0; i<pvg_info->entity.minors; i++) {
            idx = VCAP_VG_MAKE_IDX(pvg_info, ch, i);
            if(pvg_info->fd[idx])
                break;
        }
    }
    else
        idx = VCAP_VG_MAKE_IDX(pvg_info, ch, path);

    if((idx >= 0) && pvg_info->fd[idx]) {
        switch(ntype) {
            case VCAP_VG_NOTIFY_NO_JOB:
                ret = video_entity_notify(&pvg_info->entity, pvg_info->fd[idx], VG_NO_NEW_JOB, 1);
                break;
            case VCAP_VG_NOTIFY_HW_DELAY:
                ret = video_entity_notify(&pvg_info->entity, pvg_info->fd[idx], VG_HW_DELAYED, 1);
                break;
            case VCAP_VG_NOTIFY_SIGNAL_LOSS:
                ret = video_entity_notify(&pvg_info->entity, pvg_info->fd[idx], VG_SIGNAL_LOSS, 1);
                break;
            case VCAP_VG_NOTIFY_SIGNAL_PRESENT:
                ret = video_entity_notify(&pvg_info->entity, pvg_info->fd[idx], VG_SIGNAL_PRESENT, 1);
                break;
            case VCAP_VG_NOTIFY_FRAMERATE_CHANGE:
                ret = video_entity_notify(&pvg_info->entity, pvg_info->fd[idx], VG_FRAMERATE_CHANGE, 1);
                break;
            case VCAP_VG_NOTIFY_HW_CONFIG_CHANGE:
                ret = video_entity_notify(&pvg_info->entity, pvg_info->fd[idx], VG_HW_CONFIG_CHANGE, 1);
                break;
            case VCAP_VG_NOTIFY_TAMPER_ALARM:
                ret = video_entity_notify(&pvg_info->entity, pvg_info->fd[idx], VG_TAMPER_ALARM, 1);
                break;
            case VCAP_VG_NOTIFY_TAMPER_ALARM_RELEASE:
                ret = video_entity_notify(&pvg_info->entity, pvg_info->fd[idx], VG_TAMPER_ALARM_RELEASE, 1);
                break;
            default:
                ret = -1;
                break;
        }
    }
    else
        ret = -1;

    spin_unlock_irqrestore(&pvg_info->lock, flags);

    return ret;
}
