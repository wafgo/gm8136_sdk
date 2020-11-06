/**
 * @file isp_job.h
 * ISP job header file
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 */

#ifndef __ISP_JOB_H__
#define __ISP_JOB_H__

#include <linux/types.h>
#include <linux/list.h>
#include "isp_common.h"

//=============================================================================
// struct & flag definition
//=============================================================================
#define NUM_OF_JOB_ENTRIES      1024
typedef struct job_info {
    bool used;
    u32 id;
    struct list_head list;
    int header_entry;
    int cur_entry;
    u32 table[NUM_OF_JOB_ENTRIES];
} job_info_t;

#define NUM_OF_JOBS_IN_POOL     12
typedef struct job_pool {
    job_info_t *elements[NUM_OF_JOBS_IN_POOL];
    spinlock_t lock;
    int free_jobs;
} job_pool_t;

#define ALLOC_JOB_RETRY_CNT     3

//=============================================================================
// external functions
//=============================================================================
extern void isp_dev_init_job_pool(job_pool_t *pjob_pool);
extern void isp_dev_free_job_pool(job_pool_t *pjob_pool);

extern job_info_t* isp_dev_alloc_job(job_pool_t *pjob_pool);
extern void isp_dev_free_job(job_pool_t *pjob_pool, job_info_t *pjob);

extern void fill_single(job_info_t *pjob, u32 ofst, u32 value);
extern void fill_bunch(job_info_t *pjob, u32 ofst, u32 *value, int len);

extern void isp_job_fill_reg_list(job_info_t *pjob, reg_list_t *preg_list);
extern void isp_job_fill_lsc_reg(job_info_t *pjob, lsc_reg_t *plsc, int width, int height);
extern void isp_job_fill_global_gain(job_info_t *pjob, u32 gain);
extern void isp_job_fill_drc_bgwin(job_info_t *pjob, drc_bgwin_t *pdrc_bgwin);
extern void isp_job_fill_drc_pre_gamma(job_info_t *pjob, u16 *index, u32 *value);
extern void isp_job_fill_drc_F_table(job_info_t *pjob, u16 *index, u32 *value);
extern void isp_job_fill_drc_A_table(job_info_t *pjob, u16 *index, u32 *value);
extern void isp_job_fill_gamma(job_info_t *pjob, u16 *index, u8 *value);
extern void isp_job_fill_cv(job_info_t *pjob, s16 *mx);
extern void isp_job_fill_hue_sat(job_info_t *pjob, u32 *hue_sat);

extern void isp_job_fill_hist_pre_gamma(job_info_t *pjob, hist_pre_gamma_t *pgamma);
extern void isp_job_fill_hist_sta_cfg(job_info_t *pjob, hist_sta_cfg_t *psta_cfg);
extern void isp_job_fill_hist_win_cfg(job_info_t *pjob, win_rect_t *win);
extern void isp_job_fill_ae_pre_gamma(job_info_t *pjob, ae_pre_gamma_t *pgamma);
extern void isp_job_fill_ae_sta_cfg(job_info_t *pjob, ae_sta_cfg_t *psta_cfg);
extern void isp_job_fill_ae_win_cfg(job_info_t *pjob, int index, win_rect_t *win);
extern void isp_job_fill_awb_sta_cfg(job_info_t *pjob, awb_sta_cfg_t *psta_cfg);
extern void isp_job_fill_awb_win_cfg(job_info_t *pjob, win_rect_t *win);
extern void isp_job_fill_af_pre_gamma(job_info_t *pjob, af_pre_gamma_t *pgamma);
extern void isp_job_fill_af_sta_cfg(job_info_t *pjob, af_sta_cfg_t *psta_cfg);
extern void isp_job_fill_af_win_cfg(job_info_t *pjob, int index, win_rect_t *win);

#endif /* __ISP_JOB_H__ */
