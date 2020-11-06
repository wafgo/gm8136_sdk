/**
 * @file isp_lib.h
 * ISP driver library header file
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.40 $
 * $Date: 2014/12/09 04:07:51 $
 *
 * ChangeLog:
 *  $Log: isp_lib.h,v $
 *  Revision 1.40  2014/12/09 04:07:51  ricktsao
 *  add ISP_IOC_GET_CG_FORMAT / ISP_IOC_SET_CG_FORMAT
 *
 *  Revision 1.39  2014/11/27 09:00:21  ricktsao
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
 *  Revision 1.38  2014/10/22 07:26:21  ricktsao
 *  add ISP_IOC_SET_GAMMA_CURVE
 *
 *  Revision 1.37  2014/10/21 11:30:41  ricktsao
 *  modify ISP_IOC_GET_GAMMA_TYPE / ISP_IOC_SET_GAMMA_TYPE
 *
 *  Revision 1.36  2014/10/20 03:36:10  ricktsao
 *  modify gamma white clip function
 *
 *  Revision 1.35  2014/10/15 01:28:21  jtwang
 *  *** empty log message ***
 *
 *  Revision 1.34  2014/09/19 01:47:53  ricktsao
 *  no message
 *
 *  Revision 1.33  2014/09/09 07:45:36  ricktsao
 *  no message
 *
 *  Revision 1.32  2014/08/21 07:09:13  ricktsao
 *  no message
 *
 *  Revision 1.31  2014/08/19 06:54:39  ricktsao
 *  add IOCTL to get/set sensor analog gain and digital gain
 *
 *  Revision 1.30  2014/08/13 09:38:31  ricktsao
 *  add ISP_IOC_GET_ADJUST_CC_MATRIX/ISP_IOC_SET_ADJUST_CC_MATRIX
 *         ISP_IOC_GET_ADJUST_CV_MATRIX/ISP_IOC_SET_ADJUST_CV_MATRIX
 *
 *  Revision 1.29  2014/08/07 03:19:14  allenhsu
 *  Revise isp_dev_set_saturation, add USE_DMA_CMD
 *
 *  Revision 1.28  2014/08/01 11:30:33  ricktsao
 *  add image pipe control IOCTLs
 *
 *  Revision 1.27  2014/07/24 08:29:25  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.26  2014/07/21 02:11:55  ricktsao
 *  add auto gamma adjustment
 *
 *  Revision 1.25  2014/07/17 09:44:08  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.24  2014/07/17 05:38:46  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.23  2014/07/04 10:13:15  ricktsao
 *  support auto adjust black offset
 *
 *  Revision 1.22  2014/06/20 05:37:12  jtwang
 *  update fps calculation
 *
 *  Revision 1.21  2014/05/30 08:26:31  jtwang
 *  add get/set size to IOC
 *
 *  Revision 1.20  2014/05/06 06:19:58  ricktsao
 *  add ioctl for get and set cc matrix and cv matrix
 *
 *  Revision 1.19  2014/04/29 08:40:32  ricktsao
 *  no message
 *
 *  Revision 1.18  2014/04/24 11:39:42  ricktsao
 *  no message
 *
 *  Revision 1.17  2014/04/22 06:08:09  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.16  2014/04/15 05:03:02  ricktsao
 *  no message
 *
 *  Revision 1.15  2014/04/14 11:38:29  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.14  2014/04/07 08:26:50  ricktsao
 *  no message
 *
 *  Revision 1.13  2014/03/28 09:58:12  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.12  2014/03/26 06:24:06  ricktsao
 *  1. divide 3A algorithms from core driver.
 *
 *  Revision 1.11  2014/03/10 06:37:46  ricktsao
 *  no message
 *
 *  Revision 1.10  2014/03/06 03:38:56  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.9  2014/02/20 06:43:48  jtwang
 *  update mipi control
 *
 *  Revision 1.8  2014/01/21 11:32:54  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.7  2013/11/15 08:48:31  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.6  2013/11/07 12:22:59  ricktsao
 *  no message
 *
 *  Revision 1.5  2013/11/07 06:10:15  ricktsao
 *  no message
 *
 *  Revision 1.4  2013/11/06 09:35:24  allenhsu
 *  add ioctl for MRNR, 3DNR control
 *
 *  Revision 1.3  2013/11/05 09:21:21  ricktsao
 *  no message
 *
 *  Revision 1.2  2013/10/21 10:27:48  ricktsao
 *  no message
 *
 *  Revision 1.1  2013/09/26 02:06:48  ricktsao
 *  no message
 *
 */

#ifndef __ISP_LIB_H__
#define __ISP_LIB_H__

#include "isp_dev.h"

//=============================================================================
// ISP functions
//=============================================================================
extern void fp2str(char *str, s32 val, int fp_num, int decimal);
extern bool is_capture_en(isp_dev_info_t *pdev_info);
extern int  get_gamma_out(gamma_lut_t *pgm_lut, int i);

extern u32  isp_dev_read_reg(isp_dev_info_t *pdev_info, u32 offset);
extern void isp_dev_write_reg(isp_dev_info_t *pdev_info, u32 offset, u32 val);
extern void isp_dev_read_reg_list(isp_dev_info_t *pdev_info, reg_list_t *preg_list);
extern void isp_dev_write_reg_list(isp_dev_info_t *pdev_info, reg_list_t *preg_list);
extern u32  isp_dev_get_func0_sts(isp_dev_info_t *pdev_info);
extern void isp_dev_enable_func0(isp_dev_info_t *pdev_info, u32 mask);
extern void isp_dev_disable_func0(isp_dev_info_t *pdev_info, u32 mask);
extern u32  isp_dev_get_func1_sts(isp_dev_info_t *pdev_info);
extern void isp_dev_enable_func1(isp_dev_info_t *pdev_info, u32 mask);
extern void isp_dev_disable_func1(isp_dev_info_t *pdev_info, u32 mask);
extern u32  isp_dev_get_fun0(isp_dev_info_t *pdev_info);
extern u32  isp_dev_get_fun1(isp_dev_info_t *pdev_info);

extern void isp_dev_set_bayer_type(isp_dev_info_t *pdev_info, enum BAYER_TYPE type);
extern void isp_dev_set_active_window(isp_dev_info_t *pdev_info, win_rect_t *win);
extern void isp_dev_get_size(isp_dev_info_t *pdev_info, int *width, int*height);
extern int  isp_dev_set_size(isp_dev_info_t *pdev_info, int width, int height);
extern int  isp_dev_get_frame_size(isp_dev_info_t *pdev_info);

extern void isp_dev_get_blc(isp_dev_info_t *pdev_info, blc_reg_t *pblc);
extern void isp_dev_set_blc(isp_dev_info_t *pdev_info, blc_reg_t *pblc);
extern void isp_dev_get_li_lut(isp_dev_info_t *pdev_info, li_lut_t *pli_lut);
extern void isp_dev_set_li_lut(isp_dev_info_t *pdev_info, li_lut_t *pli_lut);
extern void isp_dev_get_dpc_reg(isp_dev_info_t *pdev_info, dpc_reg_t *pdpc_reg);
extern void isp_dev_set_dpc_reg(isp_dev_info_t *pdev_info, dpc_reg_t *pdpc_reg);
extern void isp_dev_get_rdn_reg(isp_dev_info_t *pdev_info, rdn_reg_t *prdn_reg);
extern void isp_dev_set_rdn_reg(isp_dev_info_t *pdev_info, rdn_reg_t *prdn_reg);
extern void isp_dev_get_ctk_reg(isp_dev_info_t *pdev_info, ctk_reg_t *pctk_reg);
extern void isp_dev_set_ctk_reg(isp_dev_info_t *pdev_info, ctk_reg_t *pctk_reg);
extern void isp_dev_get_cdn_reg(isp_dev_info_t *pdev_info, cdn_reg_t *pcdn_reg);
extern void isp_dev_set_cdn_reg(isp_dev_info_t *pdev_info, cdn_reg_t *pcdn_reg);
extern void isp_dev_get_lsc_reg(isp_dev_info_t *pdev_info, lsc_reg_t *plsc);
extern void isp_dev_set_lsc_reg(isp_dev_info_t *pdev_info, lsc_reg_t *plsc);
extern u16  isp_dev_get_global_gain(isp_dev_info_t *pdev_info);
extern void isp_dev_set_global_gain(isp_dev_info_t *pdev_info, u32 gain);
extern int  isp_dev_get_color_gain_format(isp_dev_info_t *pdev_info);
extern void isp_dev_set_color_gain_format(isp_dev_info_t *pdev_info, enum CGAIN_FMT fmt);
extern u16  isp_dev_get_color_gain(isp_dev_info_t *pdev_info, enum CGAIN_TYPE type);
extern void isp_dev_set_color_gain(isp_dev_info_t *pdev_info, enum CGAIN_TYPE type, u32 gain);
extern void isp_dev_get_drc_bgwin(isp_dev_info_t *pdev_info, drc_bgwin_t *pdrc_bgwin);
extern void isp_dev_set_drc_bgwin(isp_dev_info_t *pdev_info, int imgw, int imgh, int bghno, int bgvno);
extern void isp_dev_get_drc_pre_gamma(isp_dev_info_t *pdev_info, drc_curve_t *pdrc_curve);
extern void isp_dev_set_drc_pre_gamma(isp_dev_info_t *pdev_info, drc_curve_t *pdrc_curve);
extern void isp_dev_get_drc_F_curve(isp_dev_info_t *pdev_info, drc_curve_t *pdrc_curve);
extern void isp_dev_set_drc_F_curve(isp_dev_info_t *pdev_info, drc_curve_t *pdrc_curve);
extern void isp_dev_get_drc_A_curve(isp_dev_info_t *pdev_info, drc_curve_t *pdrc_curve);
extern void isp_dev_set_drc_A_curve(isp_dev_info_t *pdev_info, drc_curve_t *pdrc_curve);
extern int  isp_dev_get_drc_strength(isp_dev_info_t *pdev_info);
extern void isp_dev_set_drc_strength(isp_dev_info_t *pdev_info, int lvl);
extern void isp_dev_set_drc_param(isp_dev_info_t *pdev_info, drc_param_t *pdrc_param);
extern void isp_dev_get_gamma_lut(isp_dev_info_t *pdev_info, gamma_lut_t *pgm_lut);
extern void isp_dev_set_gamma_lut(isp_dev_info_t *pdev_info, gamma_lut_t *pgm_lut);
extern void isp_dev_get_cc(isp_dev_info_t *pdev_info, s16 *mx);
extern void isp_dev_set_cc(isp_dev_info_t *pdev_info, s16 *mx);
extern void isp_dev_get_cv(isp_dev_info_t *pdev_info, s16 *mx);
extern void isp_dev_set_cv(isp_dev_info_t *pdev_info, s16 *mx);
extern void isp_dev_get_ia_hue(isp_dev_info_t *pdev_info, ia_hue_t *pia_hue);
extern void isp_dev_set_ia_hue(isp_dev_info_t *pdev_info, ia_hue_t *pia_hue);
extern void isp_dev_get_ia_saturation(isp_dev_info_t *pdev_info, ia_sat_t *pia_sat);
extern void isp_dev_set_ia_saturation(isp_dev_info_t *pdev_info, ia_sat_t *pia_sat);
extern void isp_dev_get_skin_cc(isp_dev_info_t *pdev_info, sk_reg_t *psk_reg);
extern void isp_dev_set_skin_cc(isp_dev_info_t *pdev_info, sk_reg_t *psk_reg);
extern void isp_dev_get_hbnr_reg(isp_dev_info_t *pdev_info, hbnr_reg_t *phbnr_reg);
extern void isp_dev_set_hbnr_reg(isp_dev_info_t *pdev_info, hbnr_reg_t *phbnr_reg);
extern void isp_dev_get_hbnr_lut(isp_dev_info_t *pdev_info, hbnr_lut_t *phbnr_lut);
extern void isp_dev_set_hbnr_lut(isp_dev_info_t *pdev_info, hbnr_lut_t *phbnr_lut);
extern void isp_dev_get_hbnr_antijaggy(isp_dev_info_t *pdev_info, hbnr_aj_t *phbnr_aj);
extern void isp_dev_set_hbnr_antijaggy(isp_dev_info_t *pdev_info, hbnr_aj_t *phbnr_aj);
extern int  isp_dev_get_sp_mask(isp_dev_info_t *pdev_info);
extern void isp_dev_set_sp_mask(isp_dev_info_t *pdev_info, int mask);
extern int  isp_dev_get_sp_strength(isp_dev_info_t *pdev_info);
extern void isp_dev_set_sp_strength(isp_dev_info_t *pdev_info, int lvl);
extern void isp_dev_get_sp_clip(isp_dev_info_t *pdev_info, sp_clip_t *psp_clip);
extern void isp_dev_set_sp_clip(isp_dev_info_t *pdev_info, sp_clip_t *psp_clip);
extern void isp_dev_get_sp_f_curve(isp_dev_info_t *pdev_info, sp_curve_t *psp_curve);
extern void isp_dev_set_sp_f_curve(isp_dev_info_t *pdev_info, sp_curve_t *psp_curve);
extern void isp_dev_get_cs_reg(isp_dev_info_t *pdev_info, cs_reg_t *pcs);
extern void isp_dev_set_cs_reg(isp_dev_info_t *pdev_info, cs_reg_t *pcs);
extern void isp_dev_get_hist_pre_gamma(isp_dev_info_t *pdev_info, hist_pre_gamma_t *pgamma);
extern void isp_dev_set_hist_pre_gamma(isp_dev_info_t *pdev_info, hist_pre_gamma_t *pgamma);
extern void isp_dev_set_hist_sta_cfg(isp_dev_info_t *pdev_info, hist_sta_cfg_t *psta_cfg);
extern void isp_dev_set_hist_win_cfg(isp_dev_info_t *pdev_info, hist_win_cfg_t *win_cfg);
extern void isp_dev_get_ae_pre_gamma(isp_dev_info_t *pdev_info, ae_pre_gamma_t *pgamma);
extern void isp_dev_set_ae_pre_gamma(isp_dev_info_t *pdev_info, ae_pre_gamma_t *pgamma);
extern void isp_dev_adjust_ae_pre_gamma(isp_dev_info_t *pdev_info);
extern void isp_dev_set_ae_sta_cfg(isp_dev_info_t *pdev_info, ae_sta_cfg_t *psta_cfg);
extern void isp_dev_set_ae_win_cfg(isp_dev_info_t *pdev_info, ae_win_cfg_t *win_cfg);
extern void isp_dev_set_awb_sta_cfg(isp_dev_info_t *pdev_info, awb_sta_cfg_t *psta_cfg);
extern void isp_dev_set_awb_win_cfg(isp_dev_info_t *pdev_info, awb_win_cfg_t *win_cfg);
extern void isp_dev_enable_af_sta(isp_dev_info_t *pdev_info);
extern void isp_dev_disable_af_sta(isp_dev_info_t *pdev_info);
extern void isp_dev_set_af_sta_cfg(isp_dev_info_t *pdev_info, af_sta_cfg_t *psta_cfg);
extern void isp_dev_set_af_win_cfg(isp_dev_info_t *pdev_info, af_win_cfg_t *win_cfg);
extern void isp_dev_get_af_pre_gamma(isp_dev_info_t *pdev_info, af_pre_gamma_t *pgamma);
extern void isp_dev_set_af_pre_gamma(isp_dev_info_t *pdev_info, af_pre_gamma_t *pgamma);

extern int  isp_dev_load_cfg(isp_dev_info_t *pdev_info);
extern void isp_dev_wait_frame_start(isp_dev_info_t *pdev_info);
extern void isp_dev_wait_frame_end(isp_dev_info_t *pdev_info);
extern u32  isp_dev_wait_sta_ready(isp_dev_info_t *pdev_info);
extern u32  isp_dev_get_sta_ready_flag(isp_dev_info_t *pdev_info);
extern u32  isp_dev_fps_timer_init(isp_dev_info_t *pdev_info);
extern u32  isp_dev_fps_timer_enable(isp_dev_info_t *pdev_info, int enable);

//=============================================================================
// time sharing histogram
//=============================================================================
extern void isp_dev_set_tss_hist_en(isp_dev_info_t *pdev_info, bool enable);
extern void isp_dev_set_tss_hist_pre_gamma(isp_dev_info_t *pdev_info, int index, hist_pre_gamma_t *pgamma);
extern void isp_dev_set_tss_hist_sta_cfg(isp_dev_info_t *pdev_info, int index, hist_sta_cfg_t *psta_cfg);

//=============================================================================
// serial sensor interface
//=============================================================================
extern u32  isp_dev_get_s_clk(isp_dev_info_t *pdev_info);
extern void isp_dev_init_dphy(isp_dev_info_t *pdev_info);
extern void isp_dev_set_dphy_settle_count(isp_dev_info_t *pdev_info);
extern void isp_dev_init_mipi(isp_dev_info_t *pdev_info);
extern void isp_dev_init_hispi(isp_dev_info_t *pdev_info);
extern void isp_dev_set_mipi_src_size(isp_dev_info_t *pdev_info, int width, int height);
extern void isp_dev_set_hispi_src_size(isp_dev_info_t *pdev_info, int width, int height);
extern void isp_dev_init_lvds(isp_dev_info_t *pdev_info);
extern void isp_dev_set_lvds_src_size(isp_dev_info_t *pdev_info, int width, int height);

//=============================================================================
// sensor functions
//=============================================================================
extern int  isp_dev_read_sensor_reg(isp_dev_info_t *pdev_info, u32 addr, u32 *pval);
extern int  isp_dev_write_sensor_reg(isp_dev_info_t *pdev_info, u32 addr, u32 val);
extern void isp_dev_get_sensor_size(isp_dev_info_t *pdev_info, int *pwidth, int *pheight);
extern int  isp_dev_set_sensor_size(isp_dev_info_t *pdev_info, int width, int hight);
extern u32  isp_dev_get_sensor_exp(isp_dev_info_t *pdev_info);
extern int  isp_dev_set_sensor_exp(isp_dev_info_t *pdev_info, u32 exp);
extern u32  isp_dev_get_sensor_exp_us(isp_dev_info_t *pdev_info);
extern int  isp_dev_set_sensor_exp_us(isp_dev_info_t *pdev_info, u32 exp);
extern u32  isp_dev_get_sensor_gain(isp_dev_info_t *pdev_info);
extern int  isp_dev_set_sensor_gain(isp_dev_info_t *pdev_info, u32 gain);
extern bool isp_dev_get_sensor_mirror(isp_dev_info_t *pdev_info);
extern int  isp_dev_set_sensor_mirror(isp_dev_info_t *pdev_info, bool to_set);
extern bool isp_dev_get_sensor_flip(isp_dev_info_t *pdev_info);
extern int  isp_dev_set_sensor_flip(isp_dev_info_t *pdev_info, bool to_set);
extern bool isp_dev_get_sensor_ae_en(isp_dev_info_t *pdev_info);
extern int  isp_dev_set_sensor_ae_en(isp_dev_info_t *pdev_info, bool to_set);
extern bool isp_dev_get_sensor_awb_en(isp_dev_info_t *pdev_info);
extern int  isp_dev_set_sensor_awb_en(isp_dev_info_t *pdev_info, bool to_set);
extern int  isp_dev_get_sensor_fps(isp_dev_info_t *pdev_info);
extern int  isp_dev_set_sensor_fps(isp_dev_info_t *pdev_info, int fps);
extern u32  isp_dev_get_sensor_a_gain(isp_dev_info_t *pdev_info);
extern int  isp_dev_set_sensor_a_gain(isp_dev_info_t *pdev_info, u32 gain);
extern u32  isp_dev_get_sensor_min_a_gain(isp_dev_info_t *pdev_info);
extern u32  isp_dev_get_sensor_max_a_gain(isp_dev_info_t *pdev_info);
extern u32  isp_dev_get_sensor_d_gain(isp_dev_info_t *pdev_info);
extern int  isp_dev_set_sensor_d_gain(isp_dev_info_t *pdev_info, u32 gain);
extern u32  isp_dev_get_sensor_min_d_gain(isp_dev_info_t *pdev_info);
extern u32  isp_dev_get_sensor_max_d_gain(isp_dev_info_t *pdev_info);

//=============================================================================
// user preference functions
//=============================================================================
extern int  isp_dev_get_brightness(isp_dev_info_t *pdev_info);
extern void isp_dev_set_brightness(isp_dev_info_t *pdev_info, int lvl);
extern int  isp_dev_get_contrast(isp_dev_info_t *pdev_info);
extern void isp_dev_set_contrast(isp_dev_info_t *pdev_info, int lvl);
extern int  isp_dev_get_hue(isp_dev_info_t *pdev_info);
extern void isp_dev_set_hue(isp_dev_info_t *pdev_info, int lvl);
extern int  isp_dev_get_saturation(isp_dev_info_t *pdev_info);
extern void isp_dev_set_saturation(isp_dev_info_t *pdev_info, int lvl);
extern int  isp_dev_get_denoise(isp_dev_info_t *pdev_info);
extern void isp_dev_set_denoise(isp_dev_info_t *pdev_info, int lvl);
extern int  isp_dev_get_sharpness(isp_dev_info_t *pdev_info);
extern void isp_dev_set_sharpness(isp_dev_info_t *pdev_info, int lvl);
extern enum DR_MODE isp_dev_get_dr_mode(isp_dev_info_t *pdev_info);
extern int  isp_dev_set_dr_mode(isp_dev_info_t *pdev_info, enum DR_MODE mode);
extern int  isp_dev_get_gamma_type(isp_dev_info_t *pdev_info);
extern void isp_dev_set_gamma_type(isp_dev_info_t *pdev_info, int type);
extern void isp_dev_set_gamma_curve(isp_dev_info_t *pdev_info, u8 *curve);
extern void isp_dev_get_auto_adj_param(isp_dev_info_t *pdev_info, adj_param_t *padj_param);
extern void isp_dev_set_auto_adj_param(isp_dev_info_t *pdev_info, adj_param_t *padj_param);
extern void isp_dev_get_adjust_cc_matrix(isp_dev_info_t *pdev_info, adj_matrix_t *padj_matrix);
extern void isp_dev_set_adjust_cc_matrix(isp_dev_info_t *pdev_info, adj_matrix_t *padj_matrix);
extern void isp_dev_get_adjust_cv_matrix(isp_dev_info_t *pdev_info, adj_matrix_t *padj_matrix);
extern void isp_dev_set_adjust_cv_matrix(isp_dev_info_t *pdev_info, adj_matrix_t *padj_matrix);

//=============================================================================
// auto adjustment mechanism
//=============================================================================
extern void isp_adjust_nr_level(isp_dev_info_t *pdev_info);
extern void isp_adjust_color_matrix(isp_dev_info_t *pdev_info);
extern void isp_adjust_blc(isp_dev_info_t *pdev_info);
extern void isp_adjust_gamma(isp_dev_info_t *pdev_info);

//=============================================================================
// 3DNR adjustment mechanism
//=============================================================================
extern bool isp_get_mrnr_en(void);
extern void isp_set_mrnr_en(bool enable);
extern void isp_get_mrnr_param(mrnr_param_t *pMRNRth);
extern void isp_set_mrnr_param(mrnr_param_t *pMRNRth);
extern void isp_get_mrnr_operator(isp_dev_info_t *pdev_info, mrnr_op_t *pmrnr_op);
extern void isp_set_mrnr_operator(isp_dev_info_t *pdev_info, mrnr_op_t *pmrnr_op);
extern bool isp_get_tmnr_en(void);
extern void isp_set_tmnr_en(bool temporal_en);
extern bool isp_get_tmnr_edg_en(void);
extern void isp_set_tmnr_edg_en(bool tnr_edg_en);
extern bool isp_get_tmnr_learn_en(void);
extern void isp_set_tmnr_learn_en(bool tnr_learn_en);
extern void isp_get_tmnr_param(int *pY_var, int *pCb_var, int *pCr_var);
extern void isp_set_tmnr_param(int Y_var, int Cb_var, int Cr_var);
extern int  isp_get_tmnr_edge_th(void);
extern void isp_set_tmnr_edge_th(int edge_th);
extern int  isp_get_tmnr_his_factor(void);
extern void isp_set_tmnr_his_factor(u8 his);
extern int  isp_get_tmnr_learn_rate(void);
extern void isp_set_tmnr_learn_rate(u8 rate);


#endif /* __ISP_LIB_H__ */
