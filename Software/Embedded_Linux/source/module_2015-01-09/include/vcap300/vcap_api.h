/** @file vcap_api.h
 *   This file provides the vcap300 driver related function calls or structures
 *  Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 */

#ifndef _VCAP_API_H_
#define _VCAP_API_H_

#include <linux/types.h>
#include <osd_dispatch/osd_api.h>        ///< from module/include/osd_dispatch

/*************************************************************************************
 * VCAP API Interface Version
 *************************************************************************************/
#define VCAP_API_MAJOR_VER         0
#define VCAP_API_MINOR_VER         2
#define VCAP_API_BETA_VER          1
#define VCAP_API_VERSION           (((VCAP_API_MAJOR_VER)<<16)|((VCAP_API_MINOR_VER)<<8)|(VCAP_API_BETA_VER))  /* 0.1.0 */

/*************************************************************************************
 * VCAP API FCS Parmaeter Definition
 *************************************************************************************/
typedef enum {
    VCAP_API_FCS_PARAM_LV0_THRED = 0,   ///< Threshold for Y bandwidth + C bandwidth. value => 0 ~ 0xfff
    VCAP_API_FCS_PARAM_LV1_THRED,       ///< Threshold for first element of Y bandwidth. value => 0 ~ 0xff
    VCAP_API_FCS_PARAM_LV2_THRED,       ///< Threshold for Y bandwidth. value => 0 ~ 0xff
    VCAP_API_FCS_PARAM_LV3_THRED,       ///< Threshold for C bandwidth. value => 0 ~ 0xff
    VCAP_API_FCS_PARAM_LV4_THRED,       ///< Threshold for C level. value => 0 ~ 0xff
    VCAP_API_FCS_PARAM_GREY_THRED,      ///< Threshold for local grey area decision. value => 0 ~ 0xfff
    VCAP_API_FCS_PARAM_MAX
} VCAP_API_FCS_PARAM_T;

/*************************************************************************************
 * VCAP API Sharpness Parmaeter Definition
 *************************************************************************************/
typedef enum {
    VCAP_API_SHARP_PARAM_ADAPTIVE_ENB = 0,  ///< Adaptive sharpness(adapt amount and level only). value => 0:disable, 1:enable
    VCAP_API_SHARP_PARAM_RADIUS,            ///< Sharpness radius. value => 0 ~ 0x7, 0: means bypass
    VCAP_API_SHARP_PARAM_AMOUNT,            ///< Sharpness amount. value => 0 ~ 0x3f
    VCAP_API_SHARP_PARAM_THRED,             ///< Sharpness dn level. value => 0 ~ 0x3f
    VCAP_API_SHARP_PARAM_ADAPTIVE_START,    ///< Sharpness adaptive starting strength. value => 0 ~ 0x3f
    VCAP_API_SHARP_PARAM_ADAPTIVE_STEP,     ///< Sharpness adaptive step. value => 0 ~ 0x1f
    VCAP_API_SHARP_PARAM_MAX
} VCAP_API_SHARP_PARAM_T;

/*************************************************************************************
 * VCAP API DeNoise Parmaeter Definition
 *************************************************************************************/
typedef enum {
    VCAP_API_DN_PARAM_GEOMATRIC = 0,        ///< 1D DN strength according to distance. value => 0 ~ 0x7, 0: means bypass
    VCAP_API_DN_PARAM_SIMILARITY,           ///< 1D DN strength according to differance. value => 0 ~ 0x7 0: means bypass
    VCAP_API_DN_PARAM_ADAPTIVE_ENB,         ///< Adaptive denoise (adpat similarity str only). value => 0:disable, 1:enable
    VCAP_API_DN_PARAM_ADAPTIVE_STEP,        ///< Denoise adaptive step size. value => 4, 8, 16, 32, 64, 128, 256
    VCAP_API_DN_PARAM_MAX,
} VCAP_API_DN_PARAM_T;

/*************************************************************************************
 * VCAP API MD Parmaeter Definition
 *************************************************************************************/
struct vcap_api_md_region_t {
    u8   enable;                        ///< MD Enable, 0:Disable, 1:Enable
    u16  x_start;                       ///< MD region x start position
    u16  y_start;                       ///< MD region y start position
    u8   x_num;                         ///< MD region x number, 1~128
    u8   y_num;                         ///< MD region y number, 1~128
    u8   x_size;                        ///< MD region x size, 16, 20, 24, 28, 32
    u8   y_size;                        ///< MD region y size, 2~32
    u8   interlace;                     ///< MD interlace, 0: frame, 1: field
};

struct vcap_api_md_ch_info_t {
    u8   enable;                        ///< MD Enable, 0:Disable, 1:Enable
    u16  x_start;                       ///< MD region x start position
    u16  y_start;                       ///< MD region y start position
    u8   x_num;                         ///< MD region x number, 1~128
    u8   y_num;                         ///< MD region y number, 1~128
    u8   x_size;                        ///< MD region x size, 16, 20, 24, 28, 32
    u8   y_size;                        ///< MD region y size, 2~32
    u8   interlace;                     ///< MD interlace, 0: frame, 1: field
    u32  event_va;                      ///< channel event buffer virtual  address
    u32  event_pa;                      ///< channel event buffer physical address
    u32  event_size;                    ///< channel event buffer size
};

struct vcap_api_md_ch_tamper_t {
    int md_enb;                         ///< md 0:disable 1:enable
    int alarm;                          ///< current tamper alarm, 0:alarm 1:alarm release
    int sensitive_b_th;                 ///< tamper detection sensitive black threshold, 1 ~ 100, 0:for disable
    int sensitive_h_th;                 ///< tamper detection sensitive homogeneous threshold, 1 ~ 100, 0:for disable
    int histogram_idx;                  ///< tamper detection histogram index, 1 ~ 255
};

#define VCAP_API_MD_ALL_INFO_CH_MAX     33                          ///< 32 channel + 1 Cascade

struct vcap_api_md_all_info_t {
    u32 dev_event_va;                                               ///< device event buffer start virtual  address
    u32 dev_event_pa;                                               ///< device event buffer start physical address
    u32 dev_event_size;                                             ///< device event buffer size
    struct vcap_api_md_ch_info_t ch[VCAP_API_MD_ALL_INFO_CH_MAX];   ///< channel information
};

struct vcap_api_md_all_tamper_t {
    struct vcap_api_md_ch_tamper_t ch[VCAP_API_MD_ALL_INFO_CH_MAX]; ///< channel information
};

typedef enum {
    VCAP_API_MD_PARAM_ALPHA = 0,            ///< Speed of update, if the time interval you want to average over is T set alpha=1/T. value => 0 ~ 0xffff
    VCAP_API_MD_PARAM_TBG,                  ///< Threshold when the component becomes significant enough to be included into the background model. value => 0 ~ 0x1fff
    VCAP_API_MD_PARAM_INIT_VAL,             ///< Initial value for the mixture of Gaussians. value => 0 ~ 0xff
    VCAP_API_MD_PARAM_TB,                   ///< Threshold on the squared Mahalan. dist. to decide if it is well described by the background model or not. value => 0 ~ 0xf
    VCAP_API_MD_PARAM_SIGMA,                ///< Initial standard deviation. it will influence the speed of adaptation. value => 0 ~ 0x1f
    VCAP_API_MD_PARAM_PRUNE,                ///< Prune = -(alpha * CT)/256. value = > 0 ~ 0xf
    VCAP_API_MD_PARAM_TAU,                  ///< Shadow threshold. value => 0 ~ 0xff
    VCAP_API_MD_PARAM_ALPHA_ACCURACY,       ///< Alpha * Accuracy(8191). value => 0 ~ 0xfffffff
    VCAP_API_MD_PARAM_TG,                   ///< Threshold on the squared Mahalan. dist. to decide when a sample is close to the existing components. value => 0 ~ 0xf
    VCAP_API_MD_PARAM_DXDY,                 ///< CU to MD 1/(dx * dy). value => 0 ~ 0xffff
    VCAP_API_MD_PARAM_ONE_MIN_ALPHA,        ///< 2^15- alpha. value => 0 ~ 0x7fff
    VCAP_API_MD_PARAM_SHADOW,               ///< Shadow detection function. value => 0: disable, 1: enable
    VCAP_API_MD_PARAM_TAMPER_SENSITIVE_B,   ///< Tamper detection sensitive black threshold, 0 ~ 100, 0: for disable
    VCAP_API_MD_PARAM_TAMPER_SENSITIVE_H,   ///< Tamper detection sensitive homogeneous threshold, 0 ~ 100, 0: for disable
    VCAP_API_MD_PARAM_TAMPER_HISTOGRAM,     ///< Tamper detection histogram index, 1 ~ 255
    VCAP_API_MD_PARAM_MAX
} VCAP_API_MD_PARAM_T;

typedef enum {
     VCAP_API_MD_EVENT_FOREGROUND = 0,
     VCAP_API_MD_EVENT_BACKGROUND,
     VCAP_API_MD_EVENT_SHADOW
} VCAP_API_MD_EVENT_T;

/*************************************************************************************
 * VCAP API Flip Parmaeter Definition
 *************************************************************************************/
typedef enum {
     VCAP_API_FLIP_NONE = 0,        ///< disable channel flip
     VCAP_API_FLIP_V,               ///< enable  channel vertical flip
     VCAP_API_FLIP_H,               ///< enable  channel horizontal flip
     VCAP_API_FLIP_V_H              ///< enable  channel vertical & horizontal flip
} VCAP_API_FLIP_T;

/*************************************************************************************
 * VCAP API OSD Font Edge Mode Definition
 *************************************************************************************/
typedef enum {
     VCAP_API_OSD_FONT_EDGE_STANDARD = 0,  ///< Standard zoom-in mode
     VCAP_API_OSD_FONT_EDGE_2PIXEL,        ///< Two-pixel based zoom-in mode
} VCAP_API_OSD_FONT_EDGE_T;

/*************************************************************************************
 * VCAP Input Status Definition
 *************************************************************************************/
struct vcap_api_input_status_t {
    int vlos;                       ///< video signal loss
    int width;                      ///< video source width
    int height;                     ///< video source height
    int fps;                        ///< video source frame rate
};

/*************************************************************************************
 *  VCAP API Export Function Prototype
 *************************************************************************************/

/*
 * @This function used to get vcap export API version code, for compability check
 *
 * @function u32 vcap_api_get_version(void)
 * @return version code
 */
u32 vcap_api_get_version(void);

/*
 * @This function used to get device hardware ability
 *
 * @function int vcap_api_get_hw_ability(hw_ability_t *ability)
 * @param ability, pointer to ability structure
 * @return 0 on success, <0 on error
 */
int vcap_api_get_hw_ability(hw_ability_t *ability);

/*
 * @This function used to disable/enable FCS
 *
 * @function int vcap_api_fcs_onoff(u32 fd, int on)
 * @param fd, videograph channel control fd
 * @param on, 0: disable, 1: enable
 * @return 0 on success, <0 on error
 */
int vcap_api_fcs_onoff(u32 fd, int on);

/*
 * @This function used to setup FCS parameter
 *
 * @function int vcap_api_fcs_set_param(u32 fd, VCAP_API_FCS_PARAM_T param_id, u32 *data)
 * @param fd, videograph channel control fd
 * @param param_id, indicates which parameter to setup
 * @param data, pointer to parameter data
 * @return 0 on success, <0 on error
 */
int vcap_api_fcs_set_param(u32 fd, VCAP_API_FCS_PARAM_T param_id, u32 *data);

/*
 * @This function used to get FCS parameter
 *
 * @function int vcap_api_fcs_get_param(u32 fd, VCAP_API_FCS_PARAM_T param_id, u32 *data)
 * @param fd, videograph channel control fd
 * @param param_id, indicates which parameter to get
 * @param data, pointer to parameter data
 * @return 0 on success, <0 on error
 */
int vcap_api_fcs_get_param(u32 fd, VCAP_API_FCS_PARAM_T param_id, u32 *data);

/*
 * @This function used to setup sharpness parameter
 *
 * @function int vcap_api_sharp_set_param(u32 fd, VCAP_API_SHARP_PARAM_T param_id, u32 *data)
 * @param fd, videograph channel control fd
 * @param param_id, indicates which parameter to setup
 * @param data, pointer to parameter data
 * @return 0 on success, <0 on error
 */
int vcap_api_sharp_set_param(u32 fd, VCAP_API_SHARP_PARAM_T param_id, u32 *data);

/*
 * @This function used to get sharpness parameter
 *
 * @function int vcap_api_sharp_get_param(u32 fd, VCAP_API_SHARP_PARAM_T param_id, u32 *data)
 * @param fd, videograph channel control fd
 * @param param_id, indicates which parameter to setup
 * @param data, pointer to parameter data
 * @return 0 on success, <0 on error
 */
int vcap_api_sharp_get_param(u32 fd, VCAP_API_SHARP_PARAM_T param_id, u32 *data);

/*
 * @This function used to disable/enable denoise
 *
 * @function int vcap_api_dn_onoff(u32 fd, int on)
 * @param fd, videograph channel control fd
 * @param on, 0: disable, 1: enable
 * @return 0 on success, <0 on error
 */
int vcap_api_dn_onoff(u32 fd, int on);

/*
 * @This function used to setup denoise parameter
 *
 * @function int vcap_api_dn_set_param(u32 fd, VCAP_API_DN_PARAM_T param_id, u32 *data)
 * @param fd, videograph channel control fd
 * @param param_id, indicates which parameter to setup
 * @param data, pointer to parameter data
 * @return 0 on success, <0 on error
 */
int vcap_api_dn_set_param(u32 fd, VCAP_API_DN_PARAM_T param_id, u32 *data);

/*
 * @This function used to get denoise parameter
 *
 * @function int vcap_api_dn_get_param(u32 fd, VCAP_API_DN_PARAM_T param_id, u32 *data)
 * @param fd, videograph channel control fd
 * @param param_id, indicates which parameter to setup
 * @param data, pointer to parameter data
 * @return 0 on success, <0 on error
 */
int vcap_api_dn_get_param(u32 fd, VCAP_API_DN_PARAM_T param_id, u32 *data);

/*
 * @This function used to get event value of motion detection
 *
 * @function int vcap_api_md_get_event(u32 fd, u32 *even_buf, u32 buf_size)
 * @param fd, videograph channel control fd
 * @param even_buf, event buffer pointer
 * @param buf_size, event buffer size, MAX: 4KByte => 128(row)*128(col)*2(bits)
 * @return 0 on success, <0 on error
 *
 * event data format:
 *    event value => 2 bits for one motion block(MB), 0:foreground 1:background 2: shadow
 *
 * MSB----------------------> 256 bits <-------------------------LSB
 *  |-------------------------------------------------------------|
 *  | MB127 | MB126 |...........................| MB2 | MB1 | MB0 |  ==> row#0
 *  |-------------------------------------------------------------|
 *  | MB127 | MB126 |...........................| MB2 | MB1 | MB0 |  ==> row#1
 *  |-------------------------------------------------------------|
 *  | MB127 | MB126 |...........................| MB2 | MB1 | MB0 |  ==> row#2
 *  |-------------------------------------------------------------|
 *  | MB127 | MB126 |...........................| MB2 | MB1 | MB0 |  ==> row#3
 *  |-------------------------------------------------------------|
 *  | ........................................................... |   :
 *  | ........................................................... |   :
 *  | ........................................................... |  ==> row#127
 * MSB-----------------------------------------------------------LSB
 */
int vcap_api_md_get_event(u32 fd, u32 *even_buf, u32 buf_size);

/*
 * @This function used to setup motion detection parameter
 *
 * @function int vcap_api_md_set_param(u32 fd, VCAP_API_MD_PARAM_T param_id, u32 *data)
 * @param fd, videograph channel control fd
 * @param param_id, indicates which parameter to setup
 * @param data, pointer to parameter data
 * @return 0 on success, <0 on error
 */
int vcap_api_md_set_param(u32 fd, VCAP_API_MD_PARAM_T param_id, u32 *data);

/*
 * @This function used to get motion detection parameter
 *
 * @function int vcap_api_md_set_param(u32 fd, VCAP_API_MD_PARAM_T param_id, u32 *data)
 * @param fd, videograph channel control fd
 * @param param_id, indicates which parameter to setup
 * @param data, pointer to parameter data
 * @return 0 on success, <0 on error
 */
int vcap_api_md_get_param(u32 fd, VCAP_API_MD_PARAM_T param_id, u32 *data);

/*
 * @This function used to get motion region
 *
 * @function int vcap_api_md_get_region(u32 fd, struct vcap_api_md_region_t *region)
 * @param fd, videograph channel control fd
 * @param region, pointer to region data
 * @return 0 on success, <0 on error
 */
int vcap_api_md_get_region(u32 fd, struct vcap_api_md_region_t *region);

/*
 * @This function used to get all channel tamper status
 *
 * @function int vcap_api_md_get_all_tamper(u32 fd, struct vcap_api_md_all_tamper_t *tamper)
 * @param fd, videograph channel control fd
 * @param info, pointer to info data
 * @return 0 on success, <0 on error
 */
int vcap_api_md_get_all_tamper(u32 fd, struct vcap_api_md_all_tamper_t *tamper);

/*
 * @This function used to get all channel motion information
 *
 * @function int vcap_api_md_get_all_info(u32 fd, struct vcap_api_md_all_info_t *info)
 * @param fd, videograph channel control fd
 * @param info, pointer to info data
 * @return 0 on success, <0 on error
 */
int vcap_api_md_get_all_info(u32 fd, struct vcap_api_md_all_info_t *info);

/*
 * @This function used to setup mark window parameter
 *
 * @function int vcap_api_mark_set_win(u32 fd, int win_idx, mark_win_param_t *win)
 * @param fd, videograph channel control fd
 * @param win_idx, indicates which mark window to setup
 * @param win, pointer to parameter structure of mark window
 * @return 0 on success, <0 on error
 */
int vcap_api_mark_set_win(u32 fd, int win_idx, mark_win_param_t *win);

/*
 * @This function used to get mark window parameter
 *
 * @function int vcap_api_mark_get_win(u32 fd, int win_idx, mark_win_param_t *win)
 * @param fd, videograph channel control fd
 * @param win_idx, indicates which mark window to setup
 * @param win, pointer to parameter structure of mark window
 * @return 0 on success, <0 on error
 */
int vcap_api_mark_get_win(u32 fd, int win_idx, mark_win_param_t *win);

/*
 * @This function used to load mark image to internal hardware SRAM
 *
 * @function int vcap_api_mark_load_image(u32 fd, mark_img_t *img)
 * @param fd, videograph channel control fd
 * @param img, pointer to parameter structure of mark image
 * @return 0 on success, <0 on error
 */
int vcap_api_mark_load_image(u32 fd, mark_img_t *img);

/*
 * @This function used to enable mark window
 *
 * @function int vcap_api_mark_win_enable(u32 fd, int win_idx)
 * @param fd, videograph channel control fd
 * @param win_idx, indicates which mark window to setup
 * @return 0 on success, <0 on error
 */
int vcap_api_mark_win_enable(u32 fd, int win_idx);

/*
 * @This function used to disable mark window
 *
 * @function int vcap_api_mark_win_disable(u32 fd, int win_idx)
 * @param fd, videograph channel control fd
 * @param win_idx, indicates which mark window to setup
 * @return 0 on success, <0 on error
 */
int vcap_api_mark_win_disable(u32 fd, int win_idx);

/*
 * @This function used to set font character to osd sram
 *
 * @function int vcap_api_osd_set_char(u32 fd, struct osd_char_t *osd_char)
 * @param fd, videograph channel control fd
 * @param osd_char, pointer to osd character data
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_char(u32 fd, osd_char_t *osd_char);

/*
 * @This function used to remove font character
 *
 * @function int vcap_api_osd_remove_char(u32 fd, int font)
 * @param fd, videograph channel control fd
 * @param font, font index, -1 means remove all osd character in database
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_remove_char(u32 fd, int font);

/*
 * @This function used to set osd display string
 *
 * @function int vcap_api_osd_set_disp_string(u32 fd, u32 offset, u16 *font_data, u16 font_num)
 * @param fd, videograph channel control fd
 * @param offset, start offset of osd display sram, display font data will store from this offset
 * @param font_data, pointer to display font data
 * @param font_num, font number in font_data buffer
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_disp_string(u32 fd, u32 offset, u16 *font_data, u16 font_num);

/*
 * @This function used to setup osd window parameter
 *
 * @function int vcap_api_osd_set_win(u32 fd, int win_idx, osd_win_param_t *win)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @param win, pointer to osd window data
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_win(u32 fd, int win_idx, osd_win_param_t *win);

/*
 * @This function used to get osd window parameter
 *
 * @function int vcap_api_osd_get_win(u32 fd, int win_idx, osd_win_param_t *win)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @param win, pointer to osd window data
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_get_win(u32 fd, int win_idx, osd_win_param_t *win);

/*
 * @This function used to enable osd window
 *
 * @function int vcap_api_osd_win_enable(u32 fd, int win_idx)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_win_enable(u32 fd, int win_idx);

/*
 * @This function used to disable osd window
 *
 * @function int vcap_api_osd_win_disable(u32 fd, int win_idx)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_win_disable(u32 fd, int win_idx);

/*
 * @This function used to enable osd window auto color change scheme
 *
 * @function int vcap_api_osd_win_accs_enable(u32 fd, int win_idx)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_win_accs_enable(u32 fd, int win_idx);

/*
 * @This function used to disable osd window auto color change scheme
 *
 * @function int vcap_api_osd_win_accs_disable(u32 fd, int win_idx)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_win_accs_disable(u32 fd, int win_idx);

/*
 * @This function used to set osd auto color change scheme data threshold
 *
 * @function int vcap_api_osd_set_accs_data_thres(u32 fd, int thres)
 * @param fd, videograph channel control fd
 * @param thres, threshold value, 0 ~ 15
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_accs_data_thres(u32 fd, int thres);

/*
 * @This function used to get osd auto color change scheme data threshold
 *
 * @function int vcap_api_osd_get_accs_data_thres(u32 fd, int *thres)
 * @param fd, videograph channel control fd
 * @param thres, pointer to threshold value
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_get_accs_data_thres(u32 fd, int *thres);

/*
 * @This function used to set osd font edge smooth mode
 *
 * @function int vcap_api_osd_set_font_edge_mode(u32 fd, VCAP_API_OSD_FONT_EDGE_T edge_mode)
 * @param fd, videograph channel control fd
 * @param edge_mode, mode value
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_font_edge_mode(u32 fd, VCAP_API_OSD_FONT_EDGE_T edge_mode);

/*
 * @This function used to get osd font edge smooth mode
 *
 * @function int vcap_api_osd_get_font_edge_mode(u32 fd, VCAP_API_OSD_FONT_EDGE_T *edge_mode)
 * @param fd, videograph channel control fd
 * @param edge_mode, pointer to mode value
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_get_font_edge_mode(u32 fd, VCAP_API_OSD_FONT_EDGE_T *edge_mode);

/*
 * @This function used to set osd & mark priority
 *
 * @function int vcap_api_osd_set_priority(u32 fd, osd_priority_t priority)
 * @param fd, videograph channel control fd
 * @param priority, priority value
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_priority(u32 fd, osd_priority_t priority);

/*
 * @This function used to get osd & mark priority
 *
 * @function int vcap_api_osd_get_priority(u32 fd, osd_priority_t *priority)
 * @param fd, videograph channel control fd
 * @param priority, priority value
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_get_priority(u32 fd, osd_priority_t *priority);

/*
 * @This function used to set osd font smooth parameter
 *
 * @function int vcap_api_osd_set_font_smooth_param(u32 fd, osd_smooth_t *param)
 * @param fd, videograph channel control fd
 * @param param, pointer to smooth parameter
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_font_smooth_param(u32 fd, osd_smooth_t *param);

/*
 * @This function used to get osd font smooth parameter
 *
 * @function int vcap_api_osd_get_font_smooth_param(u32 fd, osd_smooth_t *param)
 * @param fd, videograph channel control fd
 * @param param, pointer to smooth parameter
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_get_font_smooth_param(u32 fd, osd_smooth_t *param);

/*
 * @This function used to set osd font marquee parameter
 *
 * @function int vcap_api_osd_set_font_marquee_param(u32 fd, osd_marquee_param_t *param)
 * @param fd, videograph channel control fd
 * @param param, pointer to marquee parameter
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_font_marquee_param(u32 fd, osd_marquee_param_t *param);

/*
 * @This function used to get osd font marquee parameter
 *
 * @function int vcap_api_osd_get_font_marquee_param(u32 fd, osd_marquee_param_t *param)
 * @param fd, videograph channel control fd
 * @param param, pointer to marquee parameter
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_get_font_marquee_param(u32 fd, osd_marquee_param_t *param);

/*
 * @This function used to set osd marquee mode
 *
 * @function int vcap_api_osd_set_font_marquee_mode(u32 fd, int win_idx, osd_marquee_mode_t mode)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @param mode, marquee mode
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_font_marquee_mode(u32 fd, int win_idx, osd_marquee_mode_t mode);

/*
 * @This function used to get osd marquee mode
 *
 * @function int vcap_api_osd_get_font_marquee_mode(u32 fd, int win_idx, osd_marquee_mode_t *mode)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @param mode, pointer to marquee mode
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_get_font_marquee_mode(u32 fd, int win_idx, osd_marquee_mode_t *mode);

/*
 * @This function used to set osd image border color
 *
 * @function int vcap_api_osd_set_img_border_color(u32 fd, int color)
 * @param fd, videograph channel control fd
 * @param color, color index
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_img_border_color(u32 fd, int color);

/*
 * @This function used to get osd image border color
 *
 * @function int vcap_api_osd_get_img_border_color(u32 fd, int *color)
 * @param fd, videograph channel control fd
 * @param color, pointer to color
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_get_img_border_color(u32 fd, int *color);

/*
 * @This function used to set osd image border width
 *
 * @function int vcap_api_osd_set_img_border_width(u32 fd, int width)
 * @param fd, videograph channel control fd
 * @param width, border width
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_img_border_width(u32 fd, int width);

/*
 * @This function used to get osd image border width
 *
 * @function int vcap_api_osd_get_img_border_width(u32 fd, int *width)
 * @param fd, videograph channel control fd
 * @param width, pointer to width
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_get_img_border_width(u32 fd, int *width);

/*
 * @This function used to enable osd image border
 *
 * @function int vcap_api_osd_img_border_enable(u32 fd)
 * @param fd, videograph channel control fd
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_img_border_enable(u32 fd);

/*
 * @This function used to disable osd image border
 *
 * @function int vcap_api_osd_img_border_disable(u32 fd)
 * @param fd, videograph channel control fd
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_img_border_disable(u32 fd);

/*
 * @This function used to set osd font zoom
 *
 * @function int vcap_api_osd_set_font_zoom(u32 fd, int win_idx, osd_font_zoom_t zoom)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @param zoom, font zoom value
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_font_zoom(u32 fd, int win_idx, osd_font_zoom_t zoom);

/*
 * @This function used to get osd font zoom
 *
 * @function int vcap_api_osd_get_font_zoom(u32 fd, int win_idx, osd_font_zoom_t *zoom)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @param zoom, pointer to font zoom
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_get_font_zoom(u32 fd, int win_idx, osd_font_zoom_t *zoom);

/*
 * @This function used to set osd alpha
 *
 * @function int vcap_api_osd_set_alpha(u32 fd, int win_idx, osd_alpha_t *alpha)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @param alpha, pointer to alpha data
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_alpha(u32 fd, int win_idx, osd_alpha_t *alpha);

/*
 * @This function used to get osd alpha
 *
 * @function int vcap_api_osd_set_alpha(u32 fd, int win_idx, osd_alpha_t *alpha)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @param alpha, pointer to alpha data
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_get_alpha(u32 fd, int win_idx, osd_alpha_t *alpha);

/*
 * @This function used to set osd border parameter
 *
 * @function int vcap_api_osd_set_border_param(u32 fd, int win_idx, osd_border_param_t *param)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @param param, pointer to osd border parameter data
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_border_param(u32 fd, int win_idx, osd_border_param_t *param);

/*
 * @This function used to get osd border parameter
 *
 * @function int vcap_api_osd_get_border_param(u32 fd, int win_idx, osd_border_param_t *param)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @param param, pointer to osd border parameter data
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_get_border_param(u32 fd, int win_idx, osd_border_param_t *param);

/*
 * @This function used to enable osd border
 *
 * @function int vcap_api_osd_border_enable(u32 fd, int win_idx)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_border_enable(u32 fd, int win_idx);

/*
 * @This function used to disable osd border
 *
 * @function int vcap_api_osd_border_disable(u32 fd, int win_idx)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_border_disable(u32 fd, int win_idx);

/*
 * @This function used to set osd foreground and background color
 *
 * @function int vcap_api_osd_set_color(u32 fd, int win_idx, osd_color_t *color)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @param color, pointer to color data
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_set_color(u32 fd, int win_idx, osd_color_t *color);

/*
 * @This function used to get osd foreground and background color
 *
 * @function int vcap_api_osd_get_color(u32 fd, int win_idx, osd_color_t *color)
 * @param fd, videograph channel control fd
 * @param win_idx, osd window index
 * @param color, pointer to color data
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_get_color(u32 fd, int win_idx, osd_color_t *color);

/*
 * @This function used to enable osd frame mode
 *
 * @function int vcap_api_osd_frame_mode_enable(u32 fd)
 * @param fd, videograph channel control fd
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_frame_mode_enable(u32 fd);

/*
 * @This function used to disable osd frame mode
 *
 * @function int vcap_api_osd_frame_mode_disable(u32 fd)
 * @param fd, videograph channel control fd
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_frame_mode_disable(u32 fd);

/*
 * @This function used to set palette color
 *
 * @function int vcap_api_palette_set(u32 fd, int idx, u32 crcby)
 * @param fd, videograph channel control fd
 * @param idx, palette index, index 0 ~ 15
 * @param crcby, palette color YUV422 format, Y is LSB
 * @return 0 on success, <0 on error
 */
int vcap_api_palette_set(u32 fd, int idx, u32 crcby);

/*
 * @This function used to get palette color
 *
 * @function int vcap_api_palette_get(u32 fd, int idx, u32 *crcby)
 * @param fd, videograph channel control fd
 * @param idx, palette index, index 0 ~ 15
 * @param crcby, palette color YUV422 format, Y is LSB
 * @return 0 on success, <0 on error
 */
int vcap_api_palette_get(u32 fd, int idx, u32 *crcby);

/*
 * @This function used to set mask window parameter
 *
 * @function int vcap_api_mask_set_win(u32 fd, int win_idx, mask_win_param_t *win)
 * @param fd, videograph channel control fd
 * @param win_idx, mask window index
 * @param win, pointer to mask window parameter
 * @return 0 on success, <0 on error
 */
int vcap_api_mask_set_win(u32 fd, int win_idx, mask_win_param_t *win);

/*
 * @This function used to get mask window parameter
 *
 * @function int vcap_api_mask_get_win(u32 fd, int win_idx, mask_win_param_t *win)
 * @param fd, videograph channel control fd
 * @param win_idx, mask window index
 * @param win, pointer to mask window parameter
 * @return 0 on success, <0 on error
 */
int vcap_api_mask_get_win(u32 fd, int win_idx, mask_win_param_t *win);

/*
 * @This function used to set mask window alpha
 *
 * @function int vcap_api_mask_set_alpha(u32 fd, int win_idx, alpha_t alpha)
 * @param fd, videograph channel control fd
 * @param win_idx, mask window index
 * @param alpha, alpha value
 * @return 0 on success, <0 on error
 */
int vcap_api_mask_set_alpha(u32 fd, int win_idx, alpha_t alpha);

/*
 * @This function used to get mask window alpha
 *
 * @function int vcap_api_mask_get_alpha(u32 fd, int win_idx, alpha_t *alpha)
 * @param fd, videograph channel control fd
 * @param win_idx, mask window index
 * @param alpha, pointer to alpha value
 * @return 0 on success, <0 on error
 */
int vcap_api_mask_get_alpha(u32 fd, int win_idx, alpha_t *alpha);

/*
 * @This function used to set mask window border parameter
 *
 * @function int vcap_api_mask_set_border(u32 fd, int win_idx, mask_border_t *border)
 * @param fd, videograph channel control fd
 * @param win_idx, mask window index
 * @param border, pointer to border parameter
 * @return 0 on success, <0 on error
 */
int vcap_api_mask_set_border(u32 fd, int win_idx, mask_border_t *border);

/*
 * @This function used to get mask window border parameter
 *
 * @function int vcap_api_mask_get_border(u32 fd, int win_idx, mask_border_t *border)
 * @param fd, videograph channel control fd
 * @param win_idx, mask window index
 * @param border, pointer to border parameter
 * @return 0 on success, <0 on error
 */
int vcap_api_mask_get_border(u32 fd, int win_idx, mask_border_t *border);

/*
 * @This function used to enable mask window
 *
 * @function int vcap_api_mask_enable(u32 fd, int win_idx)
 * @param fd, videograph channel control fd
 * @param win_idx, mask window index
 * @return 0 on success, <0 on error
 */
int vcap_api_mask_enable(u32 fd, int win_idx);

/*
 * @This function used to disable mask window
 *
 * @function int vcap_api_mask_disable(u32 fd, int win_idx)
 * @param fd, videograph channel control fd
 * @param win_idx, mask window index
 * @return 0 on success, <0 on error
 */
int vcap_api_mask_disable(u32 fd, int win_idx);

/*
 * @This function used to setup flip parameter
 *
 * @function int vcap_api_flip_set_param(u32 fd, VCAP_API_FLIP_T flip)
 * @param fd, videograph channel control fd
 * @param flip, flip parameter
 * @return 0 on success, <0 on error
 */
int vcap_api_flip_set_param(u32 fd, VCAP_API_FLIP_T flip);

/*
 * @This function used to get flip parameter
 *
 * @function int vcap_api_flip_get_param(u32 fd, VCAP_API_FLIP_T *flip)
 * @param fd, videograph channel control fd
 * @param flip, pointer to flip parameter
 * @return 0 on success, <0 on error
 */
int vcap_api_flip_get_param(u32 fd, VCAP_API_FLIP_T *flip);

/*
 * @This function used to enable osd_mask window
 *
 * @function int vcap_api_osd_mask_win_enable(u32 fd, int win_idx)
 * @param fd, videograph channel control fd
 * @param win_idx, osd_mask window index
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_mask_win_enable(u32 fd, int win_idx);

/*
 * @This function used to disable osd_mask window
 *
 * @function int vcap_api_osd_mask_win_disable(u32 fd, int win_idx)
 * @param fd, videograph channel control fd
 * @param win_idx, osd_mask window index
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_mask_win_disable(u32 fd, int win_idx);

/*
 * @This function used to setup osd_mask window parameter
 *
 * @function int vcap_api_osd_mask_set_win(u32 fd, int win_idx, osd_mask_win_param_t *win)
 * @param fd, videograph channel control fd
 * @param win_idx, osd_mask window index
 * @param win, pointer to osd_mask window data
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_mask_set_win(u32 fd, int win_idx, osd_mask_win_param_t *win);

/*
 * @This function used to get osd_mask window parameter
 *
 * @function int vcap_api_osd_mask_get_win(u32 fd, int win_idx, osd_mask_win_param_t *win)
 * @param fd, videograph channel control fd
 * @param win_idx, osd_mask window index
 * @param win, pointer to osd_mask window data
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_mask_get_win(u32 fd, int win_idx, osd_mask_win_param_t *win);

/*
 * @This function used to setup osd_mask window alpha
 *
 * @function int vcap_api_osd_mask_set_alpha(u32 fd, int win_idx, alpha_t alpha)
 * @param fd, videograph channel control fd
 * @param win_idx, osd_mask window index
 * @param alpha, alpha value
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_mask_set_alpha(u32 fd, int win_idx, alpha_t alpha);

/*
 * @This function used to get osd_mask window alpha
 *
 * @function int vcap_api_osd_mask_get_alpha(u32 fd, int win_idx, alpha_t *alpha)
 * @param fd, videograph channel control fd
 * @param win_idx, osd_mask window index
 * @param alpha, point to alpha value
 * @return 0 on success, <0 on error
 */
int vcap_api_osd_mask_get_alpha(u32 fd, int win_idx, alpha_t *alpha);

/*
 * @This function used to get input status
 *
 * @function int vcap_api_input_get_status(u32 fd, struct vcap_api_input_status_t *sts)
 * @param fd, videograph channel control fd
 * @param sts, pointer to input status data
 * @return 0 on success, <0 on error
 */
int vcap_api_input_get_status(u32 fd, struct vcap_api_input_status_t *sts);

#endif  /* _VCAP_API_H_ */
