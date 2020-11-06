/**
 * @file isp_hal.h
 * ISP hardware abstract layer header file
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.6 $
 * $Date: 2014/12/09 04:07:50 $
 *
 * ChangeLog:
 *  $Log: isp_hal.h,v $
 *  Revision 1.6  2014/12/09 04:07:50  ricktsao
 *  add ISP_IOC_GET_CG_FORMAT / ISP_IOC_SET_CG_FORMAT
 *
 *  Revision 1.5  2014/08/21 07:09:13  ricktsao
 *  no message
 *
 *  Revision 1.4  2014/08/01 11:30:32  ricktsao
 *  add image pipe control IOCTLs
 *
 *  Revision 1.3  2014/03/10 06:37:46  ricktsao
 *  no message
 *
 *  Revision 1.2  2013/11/05 09:21:21  ricktsao
 *  no message
 *
 *  Revision 1.1  2013/09/26 02:06:48  ricktsao
 *  no message
 *
 */

#ifndef __ISP_HAL_H__
#define __ISP_HAL_H__

#include "isp_common.h"

//=============================================================================
// external functions
//=============================================================================
#define isp_hal_read_reg(vbase, offset)         ioread32((vbase)+(offset))
#define isp_hal_write_reg(vbase, offset, val)   iowrite32((val), (vbase)+(offset))

extern void isp_hal_init(u32 vbase);
extern void isp_hal_sw_reset(u32 vbase);
extern void isp_hal_disable_dma(u32 vbase);

extern void isp_hal_enable_func0(u32 vbase, u32 mask);
extern void isp_hal_disable_func0(u32 vbase, u32 mask);
extern void isp_hal_enable_func1(u32 vbase, u32 mask);
extern void isp_hal_disable_func1(u32 vbase, u32 mask);

extern u32  isp_hal_get_intr0_sts(u32 vbase);
extern void isp_hal_clr_intr0_sts(u32 vbase, u32 intr_sts);
extern u32  isp_hal_get_intr0_mask(u32 vbase);
extern void isp_hal_mask_intr0(u32 vbase, u32 mask);
extern void isp_hal_unmask_intr0(u32 vbase, u32 mask);

extern u32  isp_hal_get_intr1_sts(u32 vbase);
extern void isp_hal_clr_intr1_sts(u32 vbase, u32 intr_sts);
extern u32  isp_hal_get_intr1_mask(u32 vbase);
extern void isp_hal_mask_intr1(u32 vbase, u32 mask);
extern void isp_hal_unmask_intr1(u32 vbase, u32 mask);

extern void isp_hal_set_frame_buf_base(u32 vbase, u32 addr);
extern u32  isp_hal_get_awb_seg_sta_buf_base(u32 vbase);
extern void isp_hal_set_awb_seg_sta_buf_base(u32 vbase, u32 addr);
extern u32  isp_hal_get_idma_cmd_base(u32 vbase);
extern void isp_hal_set_idma_cmd_base(u32 vbase, u32 addr);
extern u32  isp_hal_get_odma_cmd_base(u32 vbase);
extern void isp_hal_set_odma_cmd_base(u32 vbase, u32 addr);
extern void isp_hal_set_idma_auto_clr(u32 vbase, bool auto_clr);
extern void isp_hal_set_odma_auto_clr(u32 vbase, bool auto_clr);
extern void isp_hal_trigger_idma_cmd(u32 vbase);
extern void isp_hal_trigger_odma_cmd(u32 vbase);

extern void isp_hal_set_polarity(u32 vbase, enum POLARITY hs_pol, enum POLARITY vs_pol);
extern void isp_hal_set_src_format(u32 vbase, enum SRC_FMT fmt);
extern enum OUT_FMT isp_hal_get_out_format(u32 vbase);
extern void isp_hal_set_out_format(u32 vbase, enum OUT_FMT fmt);
extern void isp_hal_set_bayer_type(u32 vbase, enum BAYER_TYPE type);
extern void isp_hal_set_active_window(u32 vbase, win_rect_t *win);

extern void isp_hal_get_wdr_knee(u32 vbase, u8 *bit_width, s32 *knee_x, s32 *knee_y, s32 *slope);
extern void isp_hal_set_wdr_knee(u32 vbase, u8 bit_width, s32 *knee_x, s32 *knee_y, s32 *slope);
extern void isp_hal_get_ob(u32 vbase, s16 *ob);
extern void isp_hal_set_ob(u32 vbase, s16 *ob);
extern void isp_hal_get_li_lut(u32 vbase, enum RGB_CH ch, u8 *lut);
extern void isp_hal_set_li_lut(u32 vbase, enum RGB_CH ch, u8 *lut);
extern u8   isp_hal_get_dpc_mode(u32 vbase);
extern void isp_hal_set_dpc_mode(u32 vbase, u8 mode);
extern u8   isp_hal_get_dpc_rvc_mode(u32 vbase);
extern void isp_hal_set_dpc_rvc_mode(u32 vbase, u8 mode);
extern u8   isp_hal_get_dpc_dpcn(u32 vbase);
extern void isp_hal_set_dpc_dpcn(u32 vbase, u8 dpcn);
extern void isp_hal_get_dpc_th(u32 vbase, u16 *th);
extern void isp_hal_set_dpc_th(u32 vbase, u16 *th);
extern void isp_hal_get_rdn_th(u32 vbase, u32 *th);
extern void isp_hal_set_rdn_th(u32 vbase, u32 *th);
extern void isp_hal_get_rdn_w(u32 vbase, u8 *w);
extern void isp_hal_set_rdn_w(u32 vbase, u8 *w);
extern void isp_hal_get_ctk_th(u32 vbase, u32 *th);
extern void isp_hal_set_ctk_th(u32 vbase, u32 *th);
extern void isp_hal_get_ctk_w(u32 vbase, u8 *w);
extern void isp_hal_set_ctk_w(u32 vbase, u8 *w);
extern u8   isp_hal_get_ctk_gbgr(u32 vbase);
extern void isp_hal_set_ctk_gbgr(u32 vbase, u8 gbgr);
extern u8   isp_hal_get_lsc_segrds(u32 vbase);
extern void isp_hal_set_lsc_segrds(u32 vbase, u8 segrds);
extern void isp_hal_get_lsc_center(u32 vbase, enum RAW_CH ch, win_pos_t *pctr);
extern void isp_hal_set_lsc_center(u32 vbase, enum RAW_CH ch, win_pos_t *pctr);
extern void isp_hal_get_lsc_maxtan(u32 vbase, enum RAW_CH ch, s16 *maxtan);
extern void isp_hal_set_lsc_maxtan(u32 vbase, enum RAW_CH ch, s16 *maxtan);
extern void isp_hal_get_lsc_rdsparam(u32 vbase, enum RAW_CH ch, u16 *rdsparam);
extern void isp_hal_set_lsc_rdsparam(u32 vbase, enum RAW_CH ch, u16 *rdsparam);
extern u8   isp_hal_get_lsc_adjustn(u32 vbase);
extern void isp_hal_set_lsc_adjustn(u32 vbase, u8 adjustn);
extern void isp_hal_get_lsc_veparam(u32 vbase, s16 *veparam);
extern void isp_hal_set_lsc_veparam(u32 vbase, s16 *veparam);
extern u16  isp_hal_get_global_gain(u32 vbase);
extern void isp_hal_set_global_gain(u32 vbase, u16 gain);
extern int  isp_hal_get_color_gain_format(u32 vbase);
extern void isp_hal_set_color_gain_format(u32 vbase, enum CGAIN_FMT fmt);
extern u16  isp_hal_get_color_gain(u32 vbase, enum CGAIN_TYPE type);
extern void isp_hal_set_color_gain(u32 vbase, enum CGAIN_TYPE type, u16 gain);
extern void isp_hal_get_drc_bgwin_off_skip(u32 vbase, u16 *h_off, u16 *v_off, u8 *h_skip, u8 *v_skip);
extern void isp_hal_set_drc_bgwin_off_skip(u32 vbase, u16 h_off, u16 v_off, u8 h_skip, u8 v_skip);
extern void isp_hal_get_drc_bgwin_cfg(u32 vbase, u8 *h_no, u8 *v_no, u8 *w_shift, u8 *h_shift);
extern void isp_hal_set_drc_bgwin_cfg(u32 vbase, u8 h_no, u8 v_no, u8 w_shift, u8 h_shift);
extern void isp_hal_set_drc_int_varth(u32 vbase, u32 varth);
extern u32  isp_hal_get_drc_int_varth(u32 vbase);
extern void isp_hal_get_drc_pre_gamma(u32 vbase, u16 *index, u32 *value);
extern void isp_hal_set_drc_pre_gamma(u32 vbase, u16 *index, u32 *value);
extern void isp_hal_get_drc_F_table(u32 vbase, u16 *index, u32 *value);
extern void isp_hal_set_drc_F_table(u32 vbase, u16 *index, u32 *value);
extern void isp_hal_get_drc_A_table(u32 vbase, u16 *index, u32 *value);
extern void isp_hal_set_drc_A_table(u32 vbase, u16 *index, u32 *value);
extern void isp_hal_set_drc_qcoeff(u32 vbase, u16 coef0, u16 coef1, u16 coef2);
extern u16  isp_hal_get_drc_strength(u32 vbase);
extern void isp_hal_set_drc_strength(u32 vbase, u16 strength);
extern void isp_hal_set_drc_intensity_mode(u32 vbase,u8 local_max, u8 blend);
extern void isp_hal_get_gamma(u32 vbase, u16 *index, u8 *value);
extern void isp_hal_set_gamma(u32 vbase, u16 *index, u8 *value);
extern void isp_hal_get_bi_th(u32 vbase, u8 *pt1, u8 *pt2);
extern void isp_hal_set_bi_th(u32 vbase, u8 t1, u8 t2);
extern void isp_hal_set_bi_mode(u32 vbase, u8 mode);
extern void isp_hal_get_cdn_th(u32 vbase, u8 *th);
extern void isp_hal_set_cdn_th(u32 vbase, u8 *th);
extern void isp_hal_get_cdn_w(u32 vbase, u8 *w);
extern void isp_hal_set_cdn_w(u32 vbase, u8 *w);
extern void isp_hal_get_cc(u32 vbase, s16 *mx);
extern void isp_hal_set_cc(u32 vbase, s16 *mx);
extern void isp_hal_get_cv(u32 vbase, s16 *mx);
extern void isp_hal_set_cv(u32 vbase, s16 *mx);
extern s8   isp_hal_get_ia_brightness(u32 vbase);
extern void isp_hal_set_ia_brightness(u32 vbase, s8 lvl);
extern u8   isp_hal_get_ia_contrast(u32 vbase);
extern void isp_hal_set_ia_contrast(u32 vbase, u8 lvl);
extern u8   isp_hal_get_ia_contrast_mode(u32 vbase);
extern void isp_hal_set_ia_contrast_mode(u32 vbase, u8 mode);
extern void isp_hal_get_ia_hue(u32 vbase, s8 *hue);
extern void isp_hal_set_ia_hue(u32 vbase, s8 *hue);
extern void isp_hal_get_ia_saturation(u32 vbase, u8 *sat);
extern void isp_hal_set_ia_saturation(u32 vbase, u8 *sat);
extern void isp_hal_get_skin_cc(u32 vbase, sk_reg_t *psk_reg);
extern void isp_hal_set_skin_cc(u32 vbase, sk_reg_t *psk_reg);
extern u16  isp_hal_get_hbnr_edge_th(u32 vbase);
extern void isp_hal_set_hbnr_edge_th(u32 vbase, u16 th);
extern u8   isp_hal_get_hbnr_luma_filter(u32 vbase);
extern void isp_hal_set_hbnr_luma_filter(u32 vbase, enum HB_FILTER_TYPE type);
extern u8   isp_hal_get_hbnr_chroma_filter(u32 vbase);
extern void isp_hal_set_hbnr_chroma_filter(u32 vbase, enum HB_FILTER_TYPE type);
extern u16  isp_hal_get_hbnr_aj_edge_th(u32 vbase);
extern void isp_hal_set_hbnr_aj_edge_th(u32 vbase, u16 th);
extern void isp_hal_get_hbnr_lutl(u32 vbase, u8 *lut);
extern void isp_hal_set_hbnr_lutl(u32 vbase, u8 *lut);
extern void isp_hal_get_hbnr_lutc(u32 vbase, u8 *lut);
extern void isp_hal_set_hbnr_lutc(u32 vbase, u8 *lut);
extern int  isp_hal_get_sp_mask(u32 vbase);
extern void isp_hal_set_sp_mask(u32 vbase, int mask);
extern u8   isp_hal_get_sp_strength(u32 vbase);
extern void isp_hal_set_sp_strength(u32 vbase, u8 lvl);
extern u8   isp_hal_get_sp_th(u32 vbase);
extern void isp_hal_set_sp_th(u32 vbase, u8 th);
extern void isp_hal_get_sp_clip(u32 vbase, u8 *pclip0, u8 *pclip1);
extern void isp_hal_set_sp_clip(u32 vbase, u8 clip0, u8 clip1);
extern u8   isp_hal_get_sp_reduce(u32 vbase);
extern void isp_hal_set_sp_reduce(u32 vbase, u8 reduce);
extern void isp_hal_get_sp_F(u32 vbase, u8 *F);
extern void isp_hal_set_sp_F(u32 vbase, u8 *F);
extern void isp_hal_get_cs_curve(u32 vbase, u8 *in, u16 *out, u16 *slope);
extern void isp_hal_set_cs_curve(u32 vbase, u8 *in, u16 *out, u16 *slope);
extern void isp_hal_get_cs_th(u32 vbase, u16 *pcb_th, u16 *pcr_th);
extern void isp_hal_set_cs_th(u32 vbase, u16 cb_th, u16 cr_th);
extern void isp_hal_get_hist_pre_gamma(u32 vbase, hist_pre_gamma_t *pgamma);
extern void isp_hal_set_hist_pre_gamma(u32 vbase, hist_pre_gamma_t *pgamma);
extern void isp_hal_set_hist_sta_cfg(u32 vbase, hist_sta_cfg_t *psta_cfg);
extern void isp_hal_set_hist_win_cfg(u32 vbase, win_rect_t *win);
extern void isp_hal_trigger_histogram(u32 vbase);
extern void isp_hal_get_ae_pre_gamma(u32 vbase, ae_pre_gamma_t *pgamma);
extern void isp_hal_set_ae_pre_gamma(u32 vbase, ae_pre_gamma_t *pgamma);
extern void isp_hal_set_ae_sta_cfg(u32 vbase, ae_sta_cfg_t *psta_cfg);
extern void isp_hal_set_ae_win_cfg(u32 vbase, int index, win_rect_t *win);
extern void isp_hal_set_awb_sta_cfg(u32 vbase, awb_sta_cfg_t *psta_cfg);
extern void isp_hal_set_awb_win_cfg(u32 vbase, win_rect_t *win);
extern void isp_hal_get_af_pre_gamma(u32 vbase, af_pre_gamma_t *pgamma);
extern void isp_hal_set_af_pre_gamma(u32 vbase, af_pre_gamma_t *pgamma);
extern void isp_hal_set_af_sta_cfg(u32 vbase, af_sta_cfg_t *psta_cfg);
extern void isp_hal_set_af_win_cfg(u32 vbase, int index, win_rect_t *win);

#endif /* __ISP_HAL_H__ */
