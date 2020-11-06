/**
 * @file isp_dev.h
 * ISP device layer header file
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 */

#ifndef __ISP_DEV_H__
#define __ISP_DEV_H__

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/mempool.h>
#include <asm/io.h>
#include "isp_common.h"
#include "isp_job.h"
#include "isp_input_inf.h"
#include "isp_alg_inf.h"

//=============================================================================
// struct & flag definition
//=============================================================================
#define FRAME_EVENT_TIMOUT      2000    // ms
#define STA_READY_TIMOUT        2000    // ms
#define FPS_TIMER_INTERVAL      3       // sec

enum RESET_STS {
    ISP_RESET_DONE = 0,
    ISP_RESETING,
};

#ifdef USE_KTHREAD
#define KTHREAD_EVENT_TIME_OUT  600     // ms

enum {
    EVENT_AE   = BIT0,
    EVENT_AWB  = BIT1,
    EVENT_AF   = BIT2,
    EVENT_STOP = BIT31,
};
#endif

typedef struct isp_dev_info {
    u32 vbase;
    int irq;
    u32 mipi_vbase;
    int mipi_irq;
    u32 lvds_vbase;
    int lvds_irq;

    // input device interfaces
    sensor_dev_t *sensor;
    lens_dev_t *lens;
    iris_dev_t *iris;

    // algorithm module interfaces
    alg_module_t *alg_ae;
    alg_module_t *alg_awb;
    alg_module_t *alg_af;

    // attributes
    enum ISP_STS status;
    enum RESET_STS reset_sts;
    win_rect_t active_win;
    enum OUT_FMT out_fmt;

    // config file
    char cfg_path[MAX_PATH_NAME_SZ];
    cfg_info_t cfg_info;

    // statictis ready flag for ioctl
    u32 sta_ready_flag;

    // statistic configuration
    hist_sta_cfg_t hist_sta_cfg;
    hist_win_cfg_t hist_win_cfg;
    ae_sta_cfg_t ae_sta_cfg;
    ae_win_cfg_t ae_win_cfg;
    awb_sta_cfg_t awb_sta_cfg;
    awb_win_cfg_t awb_win_cfg;
    af_sta_cfg_t af_sta_cfg;
    af_win_cfg_t af_win_cfg;

    // frame and statistic buffers
    dma_buf_t dma_cmd_buf[2];
    dma_buf_t ce_hist_buf;
    dma_buf_t hist_sta_buf;
    dma_buf_t ae_sta_buf;
    dma_buf_t awb_sta_buf;
    dma_buf_t af_sta_buf;
    int using_awb_seg_buf_idx;
    int finish_awb_seg_buf_idx;
    dma_buf_t awb_seg_sta_buf[2];

    // error stauts counter
    int idma_err_cnt;
    int odma_err_cnt;
    int sbuf_overrun_cnt;
    int ibuf_overrun_cnt;
    int reset_cnt;

    // frame rate
    struct timer_list fps_timer;
    u32 frame_cnt;
    u32 prev_tick;
    int fps;
    u32 frame_no;

    // DRC background window
    int drc_bghno;
    int drc_bgvno;

    // user preference
    int brightness;
    int contrast;
    int hue;
    int saturation;
    int denoise;
    int sharpness;
    int gamma_type;
    int dr_mode;

    // auto adjustments
    bool adjust_blc_en;
    bool adjust_nr_en;
    bool adjust_gamma_en;
    bool adjust_ce_en;
    bool adjust_sat_en;
    bool adjust_tmnr_K;

    // adjustment variables
    int rdn_lvl;
    int ctk_lvl;
    int dpc_lvl;
    int cdn_lvl;
    int sat_lvl;
    int ce_lvl;
    int sp_lvl;
    int sp_nr_lvl;
    int sp_clip_lvl;

    // gamma white clip
    bool gamma_white_clip;
    int white_clip_value;

    // cc matrix
    cc_matrix_t curr_cc_matrix;

    // local variable
    mrnr_op_t curr_mrnr_op;
    u32 ioctl_buf_4k[1024];

    // DMA command info
    dma_cmd_info_t idma_info;
    dma_cmd_info_t odma_info;
    bool dma_in_error;
    bool dma_out_error;
    bool sbuf_ovf_error;

    // job list mechanism
    job_pool_t job_pool;
    struct list_head job_head;

    // debug mode
    int dbg_mode;
    int err_panic;

    // work queues
    struct work_struct reset_work;
    struct work_struct adjust_work;

#ifdef USE_KTHREAD
    // event thread
    u32 event_flag;
    wait_queue_head_t event_queue;
    struct task_struct *ae_thread;
    struct task_struct *awb_thread;
    struct task_struct *af_thread;
#else
    struct work_struct ae_work;
    struct workqueue_struct *ae_wq;
    struct work_struct awb_work;
    struct workqueue_struct *awb_wq;
    struct work_struct af_work;
    struct workqueue_struct *af_wq;
#endif

    // event waitqueue
    wait_queue_head_t fmstart_wait;
    wait_queue_head_t fmend_wait;
    wait_queue_head_t sta_ready_wait;

    // synchronization mechanism
    spinlock_t dev_lock;
    struct semaphore dev_mutex;
} isp_dev_info_t;

//=============================================================================
// extern functions
//=============================================================================
extern int  isp_dev_construct(isp_dev_info_t *pdev_info, u32 vstart[3], u32 vend[3], int irq[3]);
extern void isp_dev_deconstruct(isp_dev_info_t *pdev_info);
extern int  isp_dev_init(isp_dev_info_t *pdev_info);
extern int  isp_dev_start(isp_dev_info_t *pdev_info);
extern int  isp_dev_stop(isp_dev_info_t *pdev_info);
extern void isp_dev_reset_recovery(isp_dev_info_t *pdev_info);

#endif /* __ISP_DEV_H__ */
