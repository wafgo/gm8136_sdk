/**
 * @file isp_middleware.h
 * ISP driver middleware header file
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 */

#ifndef __ISP_MIDDLEWARE_H__
#define __ISP_MIDDLEWARE_H__

#include "isp_lib.h"

//=============================================================================
// user preference functions
//=============================================================================
extern int  isp_get_brightness(isp_dev_info_t *pdev_info);
extern void isp_set_brightness(isp_dev_info_t *pdev_info, int lvl);
extern int  isp_get_contrast(isp_dev_info_t *pdev_info);
extern void isp_set_contrast(isp_dev_info_t *pdev_info, int lvl);
extern int  isp_get_hue(isp_dev_info_t *pdev_info);
extern void isp_set_hue(isp_dev_info_t *pdev_info, int lvl);
extern int  isp_get_saturation(isp_dev_info_t *pdev_info);
extern void isp_set_saturation(isp_dev_info_t *pdev_info, int lvl);
extern int  isp_get_denoise(isp_dev_info_t *pdev_info);
extern void isp_set_denoise(isp_dev_info_t *pdev_info, int lvl);
extern int  isp_get_sharpness(isp_dev_info_t *pdev_info);
extern void isp_set_sharpness(isp_dev_info_t *pdev_info, int lvl);
extern int  isp_get_gamma_type(isp_dev_info_t *pdev_info);
extern void isp_set_gamma_type(isp_dev_info_t *pdev_info, int type);
extern int  isp_get_dr_mode(isp_dev_info_t *pdev_info);
extern int  isp_set_dr_mode(isp_dev_info_t *pdev_info, int mode);
extern void isp_set_gamma_curve(isp_dev_info_t *pdev_info, u8 *curve);

//=============================================================================
// tuning functions
//=============================================================================
extern void isp_adjust_dpc_level(isp_dev_info_t *pdev_info, int lvl);
extern void isp_adjust_rdn_level(isp_dev_info_t *pdev_info, int lvl);
extern void isp_adjust_ctk_level(isp_dev_info_t *pdev_info, int lvl);
extern void isp_adjust_cdn_level(isp_dev_info_t *pdev_info, int lvl);
extern void isp_adjust_sat_level(isp_dev_info_t *pdev_info, int lvl);
extern void isp_adjust_sp_level(isp_dev_info_t *pdev_info, int lvl);
extern void isp_adjust_sp_nr_level(isp_dev_info_t *pdev_info, int lvl);
extern void isp_adjust_sp_clip_level(isp_dev_info_t *pdev_info, int lvl);
extern void isp_adjust_tmnr_level(isp_dev_info_t *pdev_info, int lvl);
extern void isp_adjust_tmnr_K(isp_dev_info_t *pdev_info, int K);

//=============================================================================
// auto adjustment functions
//=============================================================================
extern void isp_auto_adjustment(isp_dev_info_t *pdev_info);
extern void isp_get_auto_adj_param(isp_dev_info_t *pdev_info, adj_param_t *padj_param);
extern void isp_set_auto_adj_param(isp_dev_info_t *pdev_info, adj_param_t *padj_param);
extern void isp_get_adjust_cv_matrix(isp_dev_info_t *pdev_info, adj_matrix_t *padj_matrix);
extern void isp_set_adjust_cv_matrix(isp_dev_info_t *pdev_info, adj_matrix_t *padj_matrix);
extern void isp_get_adjust_cc_matrix(isp_dev_info_t *pdev_info, adj_matrix_t *padj_matrix);
extern void isp_set_adjust_cc_matrix(isp_dev_info_t *pdev_info, adj_matrix_t *padj_matrix);

#endif /* __ISP_MIDDLEWARE_H__ */
