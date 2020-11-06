/**
 * @file isp_dev.h
 * ISP device layer header file
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.28 $
 * $Date: 2014/11/27 09:00:21 $
 *
 * ChangeLog:
 *  $Log: isp_dev.h,v $
 *  Revision 1.28  2014/11/27 09:00:21  ricktsao
 *  add ISP_IOC_GET_MRNR_EN
 *         ISP_IOC_GET_MRNR_PARAM
 *         ISP_IOC_GET_TMNR_EN
 *         ISP_IOC_GET_TMNR_EDG_EN
 *         ISP_IOC_GET_TMNR_LEARN_EN
 *         ISP_IOC_GET_TMNR_PARAM
 *         ISP_IOC_GET_TMNR_EDG_TH
 *         ISP_IOC_GET_SP_MASK / ISP_IOC_SET_SP_MASK
 *         ISP_IOC_GET_MRNR_OPERATOR / ISP_IOC_SET_MRNR_OPERATOR
 *
 *  Revision 1.27  2014/10/23 10:39:48  ricktsao
 *  fix unalignment program with load/store
 *
 *  Revision 1.26  2014/10/21 11:30:41  ricktsao
 *  modify ISP_IOC_GET_GAMMA_TYPE / ISP_IOC_SET_GAMMA_TYPE
 *
 *  Revision 1.25  2014/10/16 08:18:21  ricktsao
 *  add gamma white clip
 *
 *  Revision 1.24  2014/09/26 05:20:36  ricktsao
 *  use kthread instead of workqueue
 *
 *  Revision 1.23  2014/09/09 07:45:35  ricktsao
 *  no message
 *
 *  Revision 1.22  2014/08/01 11:30:32  ricktsao
 *  add image pipe control IOCTLs
 *
 *  Revision 1.21  2014/07/24 08:29:25  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.20  2014/07/21 02:11:55  ricktsao
 *  add auto gamma adjustment
 *
 *  Revision 1.19  2014/07/04 10:13:15  ricktsao
 *  support auto adjust black offset
 *
 *  Revision 1.18  2014/06/11 07:12:11  ricktsao
 *  add err_panic module parameter
 *
 *  Revision 1.17  2014/04/22 08:15:43  ricktsao
 *  no message
 *
 *  Revision 1.16  2014/04/22 06:08:09  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.15  2014/04/15 05:03:02  ricktsao
 *  no message
 *
 *  Revision 1.14  2014/04/14 11:38:29  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.13  2014/04/07 08:26:49  ricktsao
 *  no message
 *
 *  Revision 1.12  2014/03/26 06:24:06  ricktsao
 *  1. divide 3A algorithms from core driver.
 *
 *  Revision 1.11  2014/01/21 11:32:54  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.10  2013/12/10 06:51:50  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.9  2013/11/15 10:17:03  ricktsao
 *  no message
 *
 *  Revision 1.8  2013/11/15 08:48:31  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.7  2013/11/14 03:17:56  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.6  2013/11/13 09:48:02  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.5  2013/11/07 06:10:15  ricktsao
 *  no message
 *
 *  Revision 1.4  2013/11/05 09:21:21  ricktsao
 *  no message
 *
 *  Revision 1.3  2013/10/23 06:41:30  ricktsao
 *  add isp_plat_extclk_set_freq() and isp_plat_extclk_onoff()
 *
 *  Revision 1.2  2013/10/21 10:27:48  ricktsao
 *  no message
 *
 *  Revision 1.1  2013/09/26 02:06:48  ricktsao
 *  no message
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
    u32 hispi_vbase;
    int hispi_irq;
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
    win_rect_t active_win;
    enum OUT_FMT out_fmt;
    enum RESET_STS reset_sts;

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
    dma_buf_t hist_sta_buf;
    dma_buf_t ae_sta_buf;
    dma_buf_t awb_sta_buf;
    dma_buf_t af_sta_buf;
    int using_awb_seg_buf_idx;
    int finish_awb_seg_buf_idx;
    dma_buf_t awb_seg_sta_buf[2];

    // time sharing histogram
    bool tss_hist_en;
    int tss_hist_idx;
    hist_sta_cfg_t tss_hist_sta_cfg[2];
    hist_pre_gamma_t tss_hist_pre_gamma[2];

    // stauts counter
    int idma_err_cnt;
    int odma_err_cnt;
    int sbuf_overrun_cnt;
    int ibuf_overrun_cnt;
    int reset_cnt;

    // frame rate
    u32 frame_cnt;
    u32 prev_tick;
    int fps;

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
    int drc_strength;
    enum DR_MODE dr_mode;

    // black level adjustment
    bool adjust_blc;

    // noise reduction adjustment
    bool adjust_nr;
    int nl_raw;
    int nl_ctk;
    int nl_ci;
    int nl_hbl;
    int nl_hbc;
    int sp_lvl;
    int saturation_lvl;
    mrnr_op_t curr_mrnr_op;

    // gamma
    bool adjust_gamma;
    int curr_gamma_type;
    gamma_lut_t gamma_curve[NUM_OF_GAMMA_TYPE];
    gamma_lut_t curr_gamma;
    bool gamma_white_clip;
    int white_clip_value;

    int curr_tmnr_y_var;
    int curr_tmnr_cb_var;
    int curr_tmnr_cr_var;
    int curr_tmnr_edge_th;
    // color correction adjustment
    bool adjust_cc;
    u32 rb_ratio;
    int c_temp;
    s16 cc_mx[9];
    s16 cv_mx[9];

    // local variable
    reg_list_t reg_list;

    // DMA command info
    dma_cmd_info_t idma_info;
    dma_cmd_info_t odma_info;
    bool dma_in_error;
    bool dma_out_error;
    bool sbuf_ovf_error;
    u32 ioctl_buf_4k[1024];

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
    u32 event_mask;
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
extern int  isp_dev_construct(isp_dev_info_t *pdev_info, u32 vstart[4], u32 vend[4], int irq[4]);
extern void isp_dev_deconstruct(isp_dev_info_t *pdev_info);
extern int  isp_dev_init(isp_dev_info_t *pdev_info);
extern int  isp_dev_start(isp_dev_info_t *pdev_info);
extern int  isp_dev_stop(isp_dev_info_t *pdev_info);
extern void isp_dev_reset_recovery(isp_dev_info_t *pdev_info);

#endif /* __ISP_DEV_H__ */
