/**
 * @file isp_lib.h
 * ISP driver library header file
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
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

extern void isp_dev_set_bayer_type(isp_dev_info_t *pdev_info, enum BAYER_TYPE type);
extern void isp_dev_get_active_window(isp_dev_info_t *pdev_info, win_rect_t *win);
extern void isp_dev_set_active_window(isp_dev_info_t *pdev_info, win_rect_t *win);
extern void isp_dev_get_size(isp_dev_info_t *pdev_info, int *width, int*height);
extern int  isp_dev_set_size(isp_dev_info_t *pdev_info, int width, int height);
extern int  isp_dev_get_frame_size(isp_dev_info_t *pdev_info);

extern u32  isp_dev_get_func0_sts(isp_dev_info_t *pdev_info);
extern void isp_dev_enable_func0(isp_dev_info_t *pdev_info, u32 mask);
extern void isp_dev_disable_func0(isp_dev_info_t *pdev_info, u32 mask);
extern u32  isp_dev_get_func1_sts(isp_dev_info_t *pdev_info);
extern void isp_dev_enable_func1(isp_dev_info_t *pdev_info, u32 mask);
extern void isp_dev_disable_func1(isp_dev_info_t *pdev_info, u32 mask);

extern void isp_dev_get_dpcd(isp_dev_info_t *pdev_info, dcpd_reg_t *pdpcd);
extern void isp_dev_set_dcpd(isp_dev_info_t *pdev_info, dcpd_reg_t *pdpcd);
extern void isp_dev_get_blc(isp_dev_info_t *pdev_info, blc_reg_t *pblc);
extern void isp_dev_set_blc(isp_dev_info_t *pdev_info, blc_reg_t *pblc);
extern void isp_dev_get_dpc(isp_dev_info_t *pdev_info, dpc_reg_t *pdpc);
extern void isp_dev_set_dpc(isp_dev_info_t *pdev_info, dpc_reg_t *pdpc);
extern void isp_dev_get_rdn(isp_dev_info_t *pdev_info, rdn_reg_t *prdn);
extern void isp_dev_set_rdn(isp_dev_info_t *pdev_info, rdn_reg_t *prdn);
extern void isp_dev_get_ctk(isp_dev_info_t *pdev_info, ctk_reg_t *pctk);
extern void isp_dev_set_ctk(isp_dev_info_t *pdev_info, ctk_reg_t *pctk);
extern void isp_dev_get_lsc(isp_dev_info_t *pdev_info, lsc_reg_t *plsc);
extern void isp_dev_set_lsc(isp_dev_info_t *pdev_info, lsc_reg_t *plsc);
extern u16  isp_dev_get_global_gain(isp_dev_info_t *pdev_info);
extern void isp_dev_set_global_gain(isp_dev_info_t *pdev_info, u32 gain);
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
extern void isp_dev_get_ce_cfg(isp_dev_info_t *pdev_info, ce_cfg_t *pce_cfg);
extern void isp_dev_set_ce_cfg(isp_dev_info_t *pdev_info, ce_cfg_t *pce_cfg);
extern u16  isp_dev_get_ce_strength(isp_dev_info_t *pdev_info);
extern void isp_dev_set_ce_strength(isp_dev_info_t *pdev_info, u16 strength);
extern void isp_dev_get_ce_histogram(isp_dev_info_t *pdev_info, ce_hist_t *pce_hist);
extern void isp_dev_get_ce_tone_curve(isp_dev_info_t *pdev_info, ce_curve_t *pce_curve);
extern void isp_dev_set_ce_tone_curve(isp_dev_info_t *pdev_info, ce_curve_t *pce_curve);
extern void isp_dev_get_ci_cfg(isp_dev_info_t *pdev_info, ci_reg_t *pci_reg);
extern void isp_dev_set_ci_cfg(isp_dev_info_t *pdev_info, ci_reg_t *pci_reg);
extern void isp_dev_get_cdn(isp_dev_info_t *pdev_info, cdn_reg_t *pcdn);
extern void isp_dev_set_cdn(isp_dev_info_t *pdev_info, cdn_reg_t *pcdn);
extern void isp_dev_get_cc(isp_dev_info_t *pdev_info, s16 *mx);
extern void isp_dev_set_cc(isp_dev_info_t *pdev_info, s16 *mx);
extern void isp_dev_get_cv(isp_dev_info_t *pdev_info, s16 *mx);
extern void isp_dev_set_cv(isp_dev_info_t *pdev_info, s16 *mx);
extern int  isp_dev_get_ia_brightness(isp_dev_info_t *pdev_info);
extern void isp_dev_set_ia_brightness(isp_dev_info_t *pdev_info, int lvl);
extern int  isp_dev_get_ia_contrast(isp_dev_info_t *pdev_info);
extern void isp_dev_set_ia_contrast(isp_dev_info_t *pdev_info, int lvl);
extern void isp_dev_get_ia_hue(isp_dev_info_t *pdev_info, ia_hue_t *pia_hue);
extern void isp_dev_set_ia_hue(isp_dev_info_t *pdev_info, ia_hue_t *pia_hue);
extern void isp_dev_get_ia_saturation(isp_dev_info_t *pdev_info, ia_sat_t *pia_sat);
extern void isp_dev_set_ia_saturation(isp_dev_info_t *pdev_info, ia_sat_t *pia_sat);
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

//=============================================================================
// frame rate counter
//=============================================================================
extern void isp_dev_init_fps_timer(isp_dev_info_t *pdev_info);
extern void isp_dev_remove_fps_timer(isp_dev_info_t *pdev_info);
extern void isp_dev_start_fps_timer(isp_dev_info_t *pdev_info);
extern void isp_dev_stop_fps_timer(isp_dev_info_t *pdev_info);

//=============================================================================
// serial sensor interface
//=============================================================================
extern u32  isp_dev_get_s_clk(isp_dev_info_t *pdev_info);
extern void isp_dev_init_dphy(isp_dev_info_t *pdev_info);
extern void isp_dev_set_dphy_settle_count(isp_dev_info_t *pdev_info);
extern void isp_dev_init_mipi(isp_dev_info_t *pdev_info);
extern void isp_dev_set_mipi_src_size(isp_dev_info_t *pdev_info, int width, int height);
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
// 3DNR functions
//=============================================================================
extern bool isp_dev_get_mrnr_en(isp_dev_info_t *pdev_info);
extern void isp_dev_set_mrnr_en(isp_dev_info_t *pdev_info, bool to_set);
extern void isp_dev_get_mrnr_param(isp_dev_info_t *pdev_info, mrnr_param_t *pmrnr_param);
extern void isp_dev_set_mrnr_param(isp_dev_info_t *pdev_info, mrnr_param_t *pmrnr_param);
extern void isp_dev_get_mrnr_operator(isp_dev_info_t *pdev_info, mrnr_op_t *pmrnr_op);
extern void isp_dev_set_mrnr_operator(isp_dev_info_t *pdev_info, mrnr_op_t *pmrnr_op);

extern bool isp_dev_get_tmnr_en(isp_dev_info_t *pdev_info);
extern void isp_dev_set_tmnr_en(isp_dev_info_t *pdev_info, bool to_set);
extern void isp_dev_set_tmnr_learn_en(isp_dev_info_t *pdev_info, bool tnr_learn_en);
extern void isp_dev_get_tmnr_param(isp_dev_info_t *pdev_info, tmnr_param_t *ptmnr_param);
extern void isp_dev_set_tmnr_param(isp_dev_info_t *pdev_info, tmnr_param_t *ptmnr_param);
extern void isp_dev_set_tmnr_NF(isp_dev_info_t *pdev_info, int NF);
extern void isp_dev_set_tmnr_var_offset(isp_dev_info_t *pdev_info, int var_offset);
extern void isp_dev_set_tmnr_auto_recover_en(isp_dev_info_t *pdev_info, bool to_set);
extern void isp_dev_set_tmnr_rapid_recover_en(isp_dev_info_t *pdev_info, bool to_set);
extern void isp_dev_set_tmnr_Motion_th(isp_dev_info_t *pdev_info,
                                       int Motion_top_lft_th,int Motion_top_nrm_th,
                                       int Motion_top_rgt_th,int Motion_nrm_lft_th,
                                       int Motion_nrm_nrm_th,int Motion_nrm_rgt_th,
                                       int Motion_bot_lft_th,int Motion_bot_nrm_th,
                                       int Motion_bot_rgt_th);

extern bool isp_dev_get_sp_en(isp_dev_info_t *pdev_info);
extern void isp_dev_set_sp_en(isp_dev_info_t *pdev_info, bool to_set);
extern void isp_dev_get_sp_param(isp_dev_info_t *pdev_info, sp_param_t *psp_param);
extern void isp_dev_set_sp_param(isp_dev_info_t *pdev_info, sp_param_t *psp_param);
extern void isp_dev_set_sp_hpf_en(isp_dev_info_t *pdev_info, int idx, bool to_set);
extern void isp_dev_set_sp_hpf_use_lpf(isp_dev_info_t *pdev_info, int idx, bool to_set);
extern void isp_dev_set_sp_nl_gain_en(isp_dev_info_t *pdev_info, bool to_set);
extern void isp_dev_set_sp_edg_wt_en(isp_dev_info_t *pdev_info, bool to_set);
extern void isp_dev_set_sp_gau_coeff(isp_dev_info_t *pdev_info, s16 *coeff, u8 shift);
extern void isp_dev_set_sp_hpf_coeff(isp_dev_info_t *pdev_info, int idx, s16 *coeff, u8 shift);
extern void isp_dev_set_sp_hpf_gain(isp_dev_info_t *pdev_info, int idx, u16 gain);
extern void isp_dev_set_sp_hpf_coring_th(isp_dev_info_t *pdev_info, int idx, u8 coring_th);
extern void isp_dev_set_sp_hpf_bright_clip(isp_dev_info_t *pdev_info, int idx, u8 bright_clip);
extern void isp_dev_set_sp_hpf_dark_clip(isp_dev_info_t *pdev_info, int idx, u8 dark_clip);
extern void isp_dev_set_sp_hpf_peak_gain(isp_dev_info_t *pdev_info, int idx, u8 peak_gain);
extern void isp_dev_set_sp_rec_bright_clip(isp_dev_info_t *pdev_info, u8 bright_clip);
extern void isp_dev_set_sp_rec_dark_clip(isp_dev_info_t *pdev_info, u8 dark_clip);
extern void isp_dev_set_sp_rec_peak_gain(isp_dev_info_t *pdev_info, u8 peak_gain);
extern void isp_dev_set_sp_nl_gain_curve(isp_dev_info_t *pdev_info, sp_curve_t *pcurve);
extern void isp_dev_set_sp_edg_wt_curve(isp_dev_info_t *pdev_info, sp_curve_t *pcurve);

#endif /* __ISP_LIB_H__ */
