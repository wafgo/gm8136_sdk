/**
 * @file ioctl_isp328.h
 * ISP328 IOCTL header file
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 */

#ifndef __IOCTL_ISP328_H__
#define __IOCTL_ISP328_H__

#include <linux/ioctl.h>

//=============================================================================
// struct & flag definition
//=============================================================================
#ifndef __KERNEL__
    #ifndef u32
    typedef unsigned int u32;
    #endif

    #ifndef s32
    typedef int s32;
    #endif

    #ifndef u16
    typedef unsigned short u16;
    #endif

    #ifndef s16
    typedef short s16;
    #endif

    #ifndef u8
    typedef unsigned char u8;
    #endif

    #ifndef s8
    typedef char s8;
    #endif

    #ifndef bool
    typedef int bool;
    #define TRUE  1
    #define FALSE 0
    #endif

    #define BIT0    0x00000001
    #define BIT1    0x00000002
    #define BIT2    0x00000004
    #define BIT3    0x00000008
    #define BIT4    0x00000010
    #define BIT5    0x00000020
    #define BIT6    0x00000040
    #define BIT7    0x00000080
    #define BIT8    0x00000100
    #define BIT9    0x00000200
    #define BIT10   0x00000400
    #define BIT11   0x00000800
    #define BIT12   0x00001000
    #define BIT13   0x00002000
    #define BIT14   0x00004000
    #define BIT15   0x00008000
    #define BIT16   0x00010000
    #define BIT17   0x00020000
    #define BIT18   0x00040000
    #define BIT19   0x00080000
    #define BIT20   0x00100000
    #define BIT21   0x00200000
    #define BIT22   0x00400000
    #define BIT23   0x00800000
    #define BIT24   0x01000000
    #define BIT25   0x02000000
    #define BIT26   0x04000000
    #define BIT27   0x08000000
    #define BIT28   0x10000000
    #define BIT29   0x20000000
    #define BIT30   0x40000000
    #define BIT31   0x80000000
#endif

#define MAX_PATH_NAME_SZ    256

// ISP status
enum ISP_STS {
    ISP_RESET = 0,
    ISP_INIT,
    ISP_RUN,
    ISP_STOP,
};

enum ISP_FUNC0 {
    FUNC0_DCPD      = BIT0,
    FUNC0_BLC       = BIT1,
    FUNC0_DPC       = BIT4,
    FUNC0_RDN       = BIT5,
    FUNC0_CTK       = BIT6,
    FUNC0_LSC       = BIT8,
    FUNC0_HIST      = BIT11,
    FUNC0_AE        = BIT12,
    FUNC0_AWB       = BIT13,
    FUNC0_AF        = BIT14,
    FUNC0_DRC       = BIT15,
    FUNC0_GAMMA     = BIT16,
    FUNC0_CDN       = BIT17,
    FUNC0_BRIGHT    = BIT19,
    FUNC0_CONTRAST  = BIT20,
    FUNC0_HUESAT    = BIT22,
    FUNC0_CS        = BIT30,
    FUNC0_ALL       = 0x405BF973,
};

enum ISP_FUNC1 {
    FUNC1_CE        = BIT18,
    FUNC1_CE_CURVE  = BIT19,
    FUNC1_GG        = BIT20,
    FUNC1_CG        = BIT21,
    FUNC1_CV        = BIT22,
    FUNC1_ALL       = 0x007C0000,
};

enum GAMMA_TYPE {
    GM_GAMMA_16 = 0,
    GM_GAMMA_18,
    GM_GAMMA_20,
    GM_GAMMA_22,
    GM_DEF_GAMMA,
    GM_BT709_GAMMA,
    GM_SRGB_GAMMA,
    GM_USER_GAMMA,
    NUM_OF_GAMMA_TYPE,
};

// statistic ready flag
#define ISP_AE_STA_READY    BIT0
#define ISP_AWB_STA_READY   BIT1
#define ISP_AF_STA_READY    BIT2
#define ISP_HIST_STA_READY  BIT3

typedef struct win_rect {
    int x;
    int y;
    int width;
    int height;
} win_rect_t;

typedef struct win_pos {
    int x;
    int y;
} win_pos_t;

typedef struct win_size {
    int width;
    int height;
} win_size_t;

typedef struct reg_info {
    u32 addr;
    u32 value;
} reg_info_t;

typedef struct reg_list {
    u32 count;
    u32 use_dma_cmd;
    reg_info_t regs[256];
} reg_list_t;

//=====================================
// Image pipeline
//=====================================
typedef struct dcpd_reg {
    s32 knee_x[4];
    s32 knee_y[4];
    s32 slope[4];
} dcpd_reg_t;

typedef struct blc_reg {
    s16 offset[4];  // R, Gr, Gb, B
} blc_reg_t;

typedef struct dpc_reg {
    u16 th[3];
    u16 eth;
} dpc_reg_t;

typedef struct rdn_reg {
    u16 th[3];
    u16 coef[3];
    u16 kf_min;
    u16 kf_max;
} rdn_reg_t;

typedef struct ctk_reg {
    u16 th[3];
    u16 coef1;
    u16 kf_min;
    u16 kf_max;
} ctk_reg_t;

typedef struct lsc_reg {
    u8 segrds;
    // order: R, Gr, Gb, B
    win_pos_t center[4];
    s16 maxtan[4][8];
    u16 radparam[4][256];
} lsc_reg_t;

typedef struct drc_bgwin {
    u8  bghno;
    u8  bgvno;
    u8  bghskip;
    u8  bgvskip;
    u8  bgw2;
    u8  bgh2;
    u16 bgho;
    u16 bgvo;
} drc_bgwin_t;

typedef struct drc_curve {
    u16 index[16];
    u32 value[32];
} drc_curve_t;

typedef struct gamma_lut {
    u16 index[16];
    u8  value[32];
} gamma_lut_t;

typedef struct gamma_curve {
    u8 value[256];
} gamma_curve_t;

typedef struct ce_cfg {
    int bghno[2];
    int bgvno[2];
    int blend;
    int th;
    int strength;
} ce_cfg_t;

typedef struct ce_hist {
    u32 bin[256];
} ce_hist_t;

typedef struct ce_curve {
    u8 curve[256];
} ce_curve_t;

typedef struct ci_reg {
    u16 edge_dth;
    u16 freq_th;
    u16 freq_blend;
} ci_reg_t;

typedef struct cdn_reg {
    u8  th[3];
    u8  dth;
    u16 kf_min;
    u16 kf_max;
} cdn_reg_t;

typedef struct cc_matrix {
    s16 coe[9];
} cc_matrix_t;

typedef struct cv_matrix {
    s16 coe[9];
} cv_matrix_t;

typedef struct ia_hue {
    s8 hue[8];
} ia_hue_t;

typedef struct ia_sat {
    s8 saturation[8];
} ia_sat_t;

typedef struct cs_reg {
    u8  in[4];
    u16 out[3];
    u16 slope[4];
    u16 cb_th;
    u16 cr_th;
} cs_reg_t;

//=====================================
// from FT3DNR200
//=====================================
#ifndef __DECLARE_MRNR_PARAM
#define __DECLARE_MRNR_PARAM
typedef struct mrnr_param {
    /*  edge direction threshold */
    unsigned short Y_L_edg_th[4][8];
    unsigned short Cb_L_edg_th[4];
    unsigned short Cr_L_edg_th[4];

    /*  edge smooth threshold */
    unsigned short Y_L_smo_th[4][8];
    unsigned short Cb_L_smo_th[4];
    unsigned short Cr_L_smo_th[4];

    /*  nr strength threshold */
    unsigned char Y_L_nr_str[4];
    unsigned char C_L_nr_str[4];
} mrnr_param_t;
#endif

typedef struct mrnr_op {
    int G1_Y_Freq[4];
    int G1_Y_Tone[4];
    int G1_C[4];
    int G2_Y;
    int G2_C;
    int Y_L_Nr_Str[4];
    int C_L_Nr_Str[4];
    unsigned int Y_noise_lvl[32];
    unsigned int Cb_noise_lvl[4];
    unsigned int Cr_noise_lvl[4];
} mrnr_op_t;

#ifndef __DECLARE_TMNR_PARAM
#define __DECLARE_TMNR_PARAM
typedef struct tmnr_param {
    int Y_var;
    int Cb_var;
    int Cr_var;
    int K;
    int suppress_strength;
    int NF;
    int var_offset;
    int motion_var;
    int trade_thres;
    int auto_recover;
    int rapid_recover;
    int Motion_top_lft_th;
    int Motion_top_nrm_th;
    int Motion_top_rgt_th;
    int Motion_nrm_lft_th;
    int Motion_nrm_nrm_th;
    int Motion_nrm_rgt_th;
    int Motion_bot_lft_th;
    int Motion_bot_nrm_th;
    int Motion_bot_rgt_th;
} tmnr_param_t;
#endif

#ifndef __DECLARE_SP_PARAM
#define __DECLARE_SP_PARAM
typedef struct sp_curve {
    u8  index[4];   // 8-bits
    u8  value[4];   // UFIX1.7
    s16 slope[5];   // SFIX9.7
} sp_curve_t;

typedef struct sp_param {
    bool hpf_enable[6];
    bool hpf_use_lpf[6];
    bool nl_gain_enable;
    bool edg_wt_enable;
    s16 gau_5x5_coeff[25];
    u8  gau_shift;
    s16 hpf0_5x5_coeff[25];
    s16 hpf1_3x3_coeff[9];
    s16 hpf2_1x5_coeff[5];
    s16 hpf3_1x5_coeff[5];
    s16 hpf4_5x1_coeff[5];
    s16 hpf5_5x1_coeff[5];
    u16 hpf_gain[6];
    u8  hpf_shift[6];
    u8  hpf_coring_th[6];
    u8  hpf_bright_clip[6];
    u8  hpf_dark_clip[6];
    u8  hpf_peak_gain[6];
    u8  rec_bright_clip;
    u8  rec_dark_clip;
    u8  rec_peak_gain;
    sp_curve_t nl_gain_curve;
    sp_curve_t edg_wt_curve;
} sp_param_t;
#endif

//=====================================
// Histogram
//=====================================
typedef struct hist_pre_gamma {
    u16 in[6];              // [15:0]
    u16 out[6];             // [15:0]
    u16 slope[7];           // UFIX4.8
} hist_pre_gamma_t;

enum HIST_SRC {
    HIST_AFTER_GG = 0,
    HIST_AFTER_CI,
    HIST_AFTER_CV,
    HIST_AFTER_CS,
};

enum HIST_CH {
    HIST_Y = 0,
    HIST_G,
    HIST_B_Cb,
    HIST_R_Cr,
};

enum HIST_MODE {
    RGB_DIV3 = 0,           // Y = (R+G+B)/3
    R2GB_DIV4,              // Y = (R+2G+B)/4
};

typedef struct hist_sta_cfg {
    enum HIST_SRC src;
    enum HIST_CH chsel;
    enum HIST_MODE mode;
    int shrink;
} hist_sta_cfg_t;

typedef struct hist_win_cfg {
    win_rect_t hist_win;
} hist_win_cfg_t;

typedef struct hist_sta {
    // order: Y, R/Cr, G, B/Cb
    union {
        u16 y_bin[256];
        u16 r_bin[256];
        u16 g_bin[256];
        u16 b_bin[256];
        u16 cb_bin[256];
        u16 cr_bin[256];
    };
} hist_sta_t;

//=====================================
// AE
//=====================================
// AE power frequency
#define AE_PWR_FREQ_60      0
#define AE_PWR_FREQ_50      1

// AE IRIS mode
#define AE_MODE_AUTO                0
#define AE_MODE_APERTURE_PRIORITY   1
#define AE_MODE_SHUTTER_PRIORITY    2

// AE ev mode
#define AE_EV_MODE_VIDEO    0
#define AE_EV_MODE_CAMERA   1
#define AE_NUM_OF_EV_MODE   2

typedef struct ae_pre_gamma {
    u16 in[6];              // [15:0]
    u16 out[6];             // [15:0]
    u16 slope[7];           // UFIX4.8
} ae_pre_gamma_t;

enum Y_MODE {
    RGrGbB_DIV4 = 0,        // Y = (R+Gr+Gb+B)/4
    GrGb_DIV2,              // Y = (Gr+Gb)/2
};

enum STS_PATH {
    AFTER_GG = 0,          // AE sts. after Global Gain
    BEFORE_GG,             // AE sts. before Global Gain
};

typedef struct ae_sta_cfg {
    u8 src;                 // AE path select; 0: after GG, 1:before GG
    int bt;                 // 16-bit level upper limit to define black pixels
    int vbt;                // 16-bit level upper limit to define very black pixels
    int wt;                 // 16-bit level lower limit to define white pixels
    int vwt;                // 16-bit level lower limit to define very white pixels
    enum Y_MODE y_mode;     // Y mode
    u32 wins_en;            // window 0~8 enable/disable flag
} ae_sta_cfg_t;

typedef struct ae_win_cfg {
    win_rect_t ae_win[9];
} ae_win_cfg_t;

typedef struct ae_win_sta {
    u32 sumr_lsb;           // LSB of estimated SumR for window-n
    u32 sumr_hsb;           // HSB of estimated SumR for window-n
    u32 sumg_lsb;           // LSB of estimated SumG for window-n
    u32 sumg_hsb;           // HSB of estimated SumG for window-n
    u32 sumb_lsb;           // LSB of estimated SumB for window-n
    u32 sumb_hsb;           // HSB of estimated SumB for window-n
    u32 bp;                 // black pixel count for window-n
    u32 vbp;                // very-black pixel count for window-n
    u32 wp;                 // white pixel count for window-n
    u32 vwp;                // very-white pixel count for window-n
} ae_win_sta_t;

typedef struct ae_sta {
    ae_win_sta_t win_sta[9];
} ae_sta_t;

typedef struct ae_iris_pid {
    int kp;
    int ki;
    int kd;
}ae_iris_pid_t;

//=====================================
// AWB
//=====================================
enum MBLOCK_SIZE {
    MBLOCK_2x2 = 0,
    MBLOCK_4x4,
    MBLOCK_8x8,
    MBLOCK_16x16,
    MBLOCK_32x32,
};

enum SAMPLE_MODE {
    SAMPLE_1_1 = 0,
    SAMPLE_1_2,
    SAMPLE_1_4,
    SAMPLE_1_8,
    SAMPLE_1_16,
};

#define MAX_AWB_SEG_NUM_X   64
#define MAX_AWB_SEG_NUM_Y   64

typedef struct awb_sta_cfg {
    bool segment_mode;                  // AWB segment mode
    //--- attrubes for segment mode is enabled
    int num_x;
    int num_y;
    int seg_width;
    int seg_height;
    //------------------------------------------
    enum MBLOCK_SIZE macro_block;       // macro block size
    enum SAMPLE_MODE sampling_mode;     // sampling mode
    enum Y_MODE y_mode;                 // Y mode
    s32 th[13];                         // awb_th1~13
} awb_sta_cfg_t;

typedef struct awb_win_cfg {
    win_rect_t awb_win;
} awb_win_cfg_t;

typedef struct awb_sta {
    u32 rg_lvl;             // R/G statistics with level gate
    u32 bg_lvl;             // B/G statistics with level gate
    u32 pixel_num_lvl;      // pixel count with level gate
    u32 rg;                 // R/G statistics
    u32 bg;                 // B/G statistics
    u32 r;                  // R statistics
    u32 g;                  // G statistics
    u32 b;                  // B statistics
    u32 pixel_num;          // pixel count
    u32 dummy;              // must 64 bit alignment if using DMA command
} awb_sta_t;

typedef struct awb_seg {
    u32 G_sum;
    u32 R_sum;
    u32 Y_sum;
    u32 B_sum;
    u32 mblock_cnt;
    u32 dummy;
} awb_seg_t;

typedef struct awb_seg_sta {
    awb_seg_t seg_sta[MAX_AWB_SEG_NUM_X*MAX_AWB_SEG_NUM_Y];
} awb_seg_sta_t;

typedef struct awb_target {
    u32 target_ratio[3];
} awb_target_t;

#define AWB_ROI_NUM 5

typedef struct awb_roi {
    bool enable;
    int mode;
    s32 th[12];
    s32 target_rg_ratio;
    s32 target_bg_ratio;
    int unqulify_segments[12];
} awb_roi_t;

//=====================================
// AF
//=====================================
typedef struct af_pre_gamma {
    u16 in[2];              // [15:0]
    u16 out[2];             // [15:0]
    u16 slope[3];           // UFIX4.8
} af_pre_gamma_t;

typedef struct af_sta_cfg {
    int af_shift;           // Right shit bits of AF filtered value (>>af_shift)
    u8  af_hpf[4][6];       // Coefficients for HPF0~3
    u32 wins_en;            // window 0~8 enable/disable flag
} af_sta_cfg_t;

typedef struct af_win_cfg {
    win_rect_t af_win[9];
} af_win_cfg_t;

typedef struct af_sta {
    u32 af_result_w[9];     // AF result window n, n=0~8
    u32 dummy;              // must 64 bit alignment if using DMA command
} af_sta_t;

//=====================================
// Dynamic adjustment
//=====================================
#define ADJ_SEGMENTS    9

typedef struct adj_param {
    int gain_th[ADJ_SEGMENTS];
    int blc[ADJ_SEGMENTS][3];
    int dpc_lvl[ADJ_SEGMENTS];
    int rdn_lvl[ADJ_SEGMENTS];
    int ctk_lvl[ADJ_SEGMENTS];
    int cdn_lvl[ADJ_SEGMENTS];
    int gamma_idx[ADJ_SEGMENTS];
    int ce_lvl[ADJ_SEGMENTS];
    int sat_lvl[ADJ_SEGMENTS];
    int sp_lvl[ADJ_SEGMENTS];
    int sp_nr_lvl[ADJ_SEGMENTS];
    int sp_clip_lvl[ADJ_SEGMENTS];
    int tmnr_lvl[ADJ_SEGMENTS];
} adj_param_t;

typedef struct adj_matrix {
    s16 matrix[3][9];
} adj_matrix_t;

//=====================================
// Lens motor
//=====================================
typedef struct _zoom_info{
    int zoom_stage_cnt;   //zoom stage count
    int curr_zoom_index;  //current zoom stage index
    int curr_zoom_x10;    //zoom magnification
} zoom_info_t;

typedef struct _focus_range{
    int min;
    int max;
} focus_range_t;

//=============================================================================
// IOCTL command definition
//=============================================================================
#define ISP_IOC_GET_CHIPID              _IOR(0, 0, u32)
#define ISP_IOC_GET_DRIVER_VER          _IOR(0, 1, u32)
#define ISP_IOC_GET_INPUT_INF_VER       _IOR(0, 2, u32)
#define ISP_IOC_GET_ALG_INF_VER         _IOR(0, 3, u32)

#define ISP_IOC_COMMON 'c'
#define ISP_IOC_GET_STATUS              _IOR(ISP_IOC_COMMON, 0, int)
#define ISP_IOC_START                   _IO(ISP_IOC_COMMON, 1)
#define ISP_IOC_STOP                    _IO(ISP_IOC_COMMON, 2)

#define ISP_IOC_READ_REG                _IOWR(ISP_IOC_COMMON, 3, reg_info_t *)
#define ISP_IOC_WRITE_REG               _IOWR(ISP_IOC_COMMON, 4, reg_info_t *)
#define ISP_IOC_READ_REG_LIST           _IOWR(ISP_IOC_COMMON, 5, reg_list_t *)
#define ISP_IOC_WRITE_REG_LIST          _IOWR(ISP_IOC_COMMON, 6, reg_list_t *)

#define ISP_IOC_GET_ACT_WIN             _IOR(ISP_IOC_COMMON, 7, win_rect_t)
#define ISP_IOC_SET_ACT_WIN             _IOW(ISP_IOC_COMMON, 7, win_rect_t)
#define ISP_IOC_GET_SIZE                _IOR(ISP_IOC_COMMON, 8, win_size_t)
#define ISP_IOC_SET_SIZE                _IOW(ISP_IOC_COMMON, 8, win_size_t)

#define ISP_IOC_WAIT_STA_READY          _IOR(ISP_IOC_COMMON, 9, u32)
#define ISP_IOC_GET_STA_READY_FLAG      _IOR(ISP_IOC_COMMON, 10, u32)

#define ISP_IOC_GET_FUNC0_STS           _IOR(ISP_IOC_COMMON, 11, u32)
#define ISP_IOC_ENABLE_FUNC0            _IOW(ISP_IOC_COMMON, 12, u32)
#define ISP_IOC_DISABLE_FUNC0           _IOW(ISP_IOC_COMMON, 13, u32)
#define ISP_IOC_GET_FUNC1_STS           _IOR(ISP_IOC_COMMON, 14, u32)
#define ISP_IOC_ENABLE_FUNC1            _IOW(ISP_IOC_COMMON, 15, u32)
#define ISP_IOC_DISABLE_FUNC1           _IOW(ISP_IOC_COMMON, 16, u32)

// for isp_demon
#define ISP_IOC_GET_FRAME_SIZE          _IOR(ISP_IOC_COMMON, 128, u32)
#define ISP_IOC_GRAB_FRAME              _IOR(ISP_IOC_COMMON, 129, u8 *)
#define ISP_IOC_GET_CFG_FILE_PATH       _IOR(ISP_IOC_COMMON, 130, char *)
#define ISP_IOC_GET_CFG_FILE_SIZE       _IOR(ISP_IOC_COMMON, 131, u32)
#define ISP_IOC_READ_CFG_FILE           _IOR(ISP_IOC_COMMON, 132, u8 *)
#define ISP_IOC_WRITE_CFG_FILE          _IOW(ISP_IOC_COMMON, 132, u8 *)
#define ISP_IOC_LOAD_CFG                _IOW(ISP_IOC_COMMON, 133, char *)

//=============================================================================
// Image pipeline IOCTL command definition
//=============================================================================
#define ISP_IOC_HW 'h'
#define ISP_IOC_GET_DCPD                _IOR(ISP_IOC_HW, 1, dcpd_reg_t *)
#define ISP_IOC_SET_DCPD                _IOW(ISP_IOC_HW, 1, dcpd_reg_t *)
#define ISP_IOC_GET_BLC                 _IOR(ISP_IOC_HW, 2, blc_reg_t *)
#define ISP_IOC_SET_BLC                 _IOW(ISP_IOC_HW, 2, blc_reg_t *)
#define ISP_IOC_GET_DPC                 _IOR(ISP_IOC_HW, 3, dpc_reg_t *)
#define ISP_IOC_SET_DPC                 _IOW(ISP_IOC_HW, 3, dpc_reg_t *)
#define ISP_IOC_GET_RAW_NR              _IOR(ISP_IOC_HW, 4, rdn_reg_t *)
#define ISP_IOC_SET_RAW_NR              _IOW(ISP_IOC_HW, 4, rdn_reg_t *)
#define ISP_IOC_GET_CTK                 _IOR(ISP_IOC_HW, 5, ctk_reg_t *)
#define ISP_IOC_SET_CTK                 _IOW(ISP_IOC_HW, 5, ctk_reg_t *)
#define ISP_IOC_GET_LSC                 _IOR(ISP_IOC_HW, 6, lsc_reg_t *)
#define ISP_IOC_SET_LSC                 _IOW(ISP_IOC_HW, 6, lsc_reg_t *)
#define ISP_IOC_GET_ISP_GAIN            _IOR(ISP_IOC_HW, 7, int)
#define ISP_IOC_SET_ISP_GAIN            _IOW(ISP_IOC_HW, 7, int)
#define ISP_IOC_GET_R_GAIN              _IOR(ISP_IOC_HW, 8, int)
#define ISP_IOC_SET_R_GAIN              _IOW(ISP_IOC_HW, 8, int)
#define ISP_IOC_GET_G_GAIN              _IOR(ISP_IOC_HW, 9, int)
#define ISP_IOC_SET_G_GAIN              _IOW(ISP_IOC_HW, 9, int)
#define ISP_IOC_GET_B_GAIN              _IOR(ISP_IOC_HW, 10, int)
#define ISP_IOC_SET_B_GAIN              _IOW(ISP_IOC_HW, 10, int)
#define ISP_IOC_GET_DRC_STRENGTH        _IOR(ISP_IOC_HW, 11, int)
#define ISP_IOC_SET_DRC_STRENGTH        _IOW(ISP_IOC_HW, 11, int)
#define ISP_IOC_GET_DRC_BGWIN           _IOR(ISP_IOC_HW, 12, drc_bgwin_t *)
#define ISP_IOC_SET_DRC_BGWIN           _IOW(ISP_IOC_HW, 12, drc_bgwin_t *)
#define ISP_IOC_GET_DRC_F_CURVE         _IOR(ISP_IOC_HW, 13, drc_curve_t *)
#define ISP_IOC_SET_DRC_F_CURVE         _IOW(ISP_IOC_HW, 13, drc_curve_t *)
#define ISP_IOC_GET_DRC_A_CURVE         _IOR(ISP_IOC_HW, 14, drc_curve_t *)
#define ISP_IOC_SET_DRC_A_CURVE         _IOW(ISP_IOC_HW, 14, drc_curve_t *)
#define ISP_IOC_GET_GAMMA_LUT           _IOR(ISP_IOC_HW, 15, gamma_lut_t *)
#define ISP_IOC_SET_GAMMA_LUT           _IOW(ISP_IOC_HW, 15, gamma_lut_t *)
#define ISP_IOC_GET_CE_CFG              _IOR(ISP_IOC_HW, 16, ce_cfg_t *)
#define ISP_IOC_SET_CE_CFG              _IOW(ISP_IOC_HW, 16, ce_cfg_t *)
#define ISP_IOC_GET_CE_STRENGTH         _IOR(ISP_IOC_HW, 17, int)
#define ISP_IOC_SET_CE_STRENGTH         _IOW(ISP_IOC_HW, 17, int)
#define ISP_IOC_GET_CE_HISTOGRAM        _IOR(ISP_IOC_HW, 18, ce_hist_t *)
#define ISP_IOC_GET_CE_TONE_CURVE       _IOR(ISP_IOC_HW, 19, ce_curve_t *)
#define ISP_IOC_SET_CE_TONE_CURVE       _IOW(ISP_IOC_HW, 19, ce_curve_t *)
#define ISP_IOC_GET_CI_CFG              _IOR(ISP_IOC_HW, 20, ci_reg_t *)
#define ISP_IOC_SET_CI_CFG              _IOW(ISP_IOC_HW, 20, ci_reg_t *)
#define ISP_IOC_GET_CI_NR               _IOR(ISP_IOC_HW, 21, cdn_reg_t *)
#define ISP_IOC_SET_CI_NR               _IOW(ISP_IOC_HW, 21, cdn_reg_t *)
#define ISP_IOC_GET_CV_MATRIX           _IOR(ISP_IOC_HW, 22, cv_matrix_t)
#define ISP_IOC_SET_CV_MATRIX           _IOW(ISP_IOC_HW, 22, cv_matrix_t)
#define ISP_IOC_GET_IA_BRIGHTNESS       _IOR(ISP_IOC_HW, 23, int)
#define ISP_IOC_SET_IA_BRIGHTNESS       _IOW(ISP_IOC_HW, 23, int)
#define ISP_IOC_GET_IA_CONTRAST         _IOR(ISP_IOC_HW, 24, int)
#define ISP_IOC_SET_IA_CONTRAST         _IOW(ISP_IOC_HW, 24, int)
#define ISP_IOC_GET_IA_HUE              _IOR(ISP_IOC_HW, 25, ia_hue_t *)
#define ISP_IOC_SET_IA_HUE              _IOW(ISP_IOC_HW, 25, ia_hue_t *)
#define ISP_IOC_GET_IA_SATURATION       _IOR(ISP_IOC_HW, 26, ia_sat_t *)
#define ISP_IOC_SET_IA_SATURATION       _IOW(ISP_IOC_HW, 26, ia_sat_t *)
#define ISP_IOC_GET_CS                  _IOR(ISP_IOC_HW, 27, cs_reg_t *)
#define ISP_IOC_SET_CS                  _IOW(ISP_IOC_HW, 27, cs_reg_t *)
#define ISP_IOC_GET_CC_MATRIX           _IOR(ISP_IOC_HW, 28, cc_matrix_t)
#define ISP_IOC_SET_CC_MATRIX           _IOW(ISP_IOC_HW, 28, cc_matrix_t)

// FT3DNR200
#define ISP_IOC_GET_MRNR_EN             _IOR(ISP_IOC_HW, 60, int)
#define ISP_IOC_SET_MRNR_EN             _IOW(ISP_IOC_HW, 60, int)
#define ISP_IOC_GET_MRNR_PARAM          _IOR(ISP_IOC_HW, 61, mrnr_param_t *)
#define ISP_IOC_SET_MRNR_PARAM          _IOW(ISP_IOC_HW, 61, mrnr_param_t *)
#define ISP_IOC_GET_MRNR_OPERATOR       _IOR(ISP_IOC_HW, 62, mrnr_op_t *)
#define ISP_IOC_SET_MRNR_OPERATOR       _IOW(ISP_IOC_HW, 62, mrnr_op_t *)
#define ISP_IOC_GET_TMNR_EN             _IOR(ISP_IOC_HW, 70, int)
#define ISP_IOC_SET_TMNR_EN             _IOW(ISP_IOC_HW, 70, int)
#define ISP_IOC_GET_TMNR_PARAM          _IOR(ISP_IOC_HW, 71, tmnr_param_t *)
#define ISP_IOC_SET_TMNR_PARAM          _IOW(ISP_IOC_HW, 71, tmnr_param_t *)
#define ISP_IOC_GET_SP_EN               _IOR(ISP_IOC_HW, 80, int)
#define ISP_IOC_SET_SP_EN               _IOW(ISP_IOC_HW, 80, int)
#define ISP_IOC_GET_SP_PARAM            _IOR(ISP_IOC_HW, 81, sp_param_t *)
#define ISP_IOC_SET_SP_PARAM            _IOW(ISP_IOC_HW, 81, sp_param_t *)

// statistic
#define ISP_IOC_GET_HIST_CFG            _IOR(ISP_IOC_HW, 128, hist_sta_cfg_t *)
#define ISP_IOC_SET_HIST_CFG            _IOW(ISP_IOC_HW, 128, hist_sta_cfg_t *)
#define ISP_IOC_GET_HIST_WIN            _IOR(ISP_IOC_HW, 129, hist_win_cfg_t *)
#define ISP_IOC_SET_HIST_WIN            _IOW(ISP_IOC_HW, 129, hist_win_cfg_t *)
#define ISP_IOC_GET_HIST_PRE_GAMMA      _IOR(ISP_IOC_HW, 130, hist_pre_gamma_t *)
#define ISP_IOC_SET_HIST_PRE_GAMMA      _IOW(ISP_IOC_HW, 130, hist_pre_gamma_t *)
#define ISP_IOC_GET_HISTOGRAM           _IOR(ISP_IOC_HW, 131, hist_sta_t *)
#define ISP_IOC_GET_AE_CFG              _IOR(ISP_IOC_HW, 132, ae_sta_cfg_t *)
#define ISP_IOC_SET_AE_CFG              _IOW(ISP_IOC_HW, 132, ae_sta_cfg_t *)
#define ISP_IOC_GET_AE_WIN              _IOR(ISP_IOC_HW, 133, ae_win_cfg_t *)
#define ISP_IOC_SET_AE_WIN              _IOW(ISP_IOC_HW, 133, ae_win_cfg_t *)
#define ISP_IOC_GET_AE_PRE_GAMMA        _IOR(ISP_IOC_HW, 134, ae_pre_gamma_t *)
#define ISP_IOC_SET_AE_PRE_GAMMA        _IOW(ISP_IOC_HW, 134, ae_pre_gamma_t *)
#define ISP_IOC_GET_AE_STA              _IOR(ISP_IOC_HW, 135, ae_sta_t *)
#define ISP_IOC_GET_AWB_CFG             _IOR(ISP_IOC_HW, 136, awb_sta_cfg_t *)
#define ISP_IOC_SET_AWB_CFG             _IOW(ISP_IOC_HW, 136, awb_sta_cfg_t *)
#define ISP_IOC_GET_AWB_WIN             _IOR(ISP_IOC_HW, 137, awb_win_cfg_t *)
#define ISP_IOC_SET_AWB_WIN             _IOW(ISP_IOC_HW, 137, awb_win_cfg_t *)
#define ISP_IOC_GET_AWB_STA             _IOR(ISP_IOC_HW, 138, awb_sta_t *)
#define ISP_IOC_GET_AWB_SEG_STA         _IOR(ISP_IOC_HW, 139, awb_seg_sta_t *)
#define ISP_IOC_GET_AF_CFG              _IOR(ISP_IOC_HW, 140, af_sta_cfg_t *)
#define ISP_IOC_SET_AF_CFG              _IOW(ISP_IOC_HW, 140, af_sta_cfg_t *)
#define ISP_IOC_GET_AF_WIN              _IOR(ISP_IOC_HW, 141, af_win_cfg_t *)
#define ISP_IOC_SET_AF_WIN              _IOW(ISP_IOC_HW, 141, af_win_cfg_t *)
#define ISP_IOC_GET_AF_PRE_GAMMA        _IOR(ISP_IOC_HW, 142, af_pre_gamma_t *)
#define ISP_IOC_SET_AF_PRE_GAMMA        _IOW(ISP_IOC_HW, 142, af_pre_gamma_t *)
#define ISP_IOC_GET_AF_STA              _IOR(ISP_IOC_HW, 143, af_sta_t *)

//=============================================================================
// User preference and auto adjustment IOCTL command definition
//=============================================================================
#define ISP_IOC_MW 'm'
#define ISP_IOC_GET_BRIGHTNESS          _IOR(ISP_IOC_MW, 1, int)
#define ISP_IOC_SET_BRIGHTNESS          _IOW(ISP_IOC_MW, 1, int)
#define ISP_IOC_GET_CONTRAST            _IOR(ISP_IOC_MW, 2, int)
#define ISP_IOC_SET_CONTRAST            _IOW(ISP_IOC_MW, 2, int)
#define ISP_IOC_GET_HUE                 _IOR(ISP_IOC_MW, 3, int)
#define ISP_IOC_SET_HUE                 _IOW(ISP_IOC_MW, 3, int)
#define ISP_IOC_GET_SATURATION          _IOR(ISP_IOC_MW, 4, int)
#define ISP_IOC_SET_SATURATION          _IOW(ISP_IOC_MW, 4, int)
#define ISP_IOC_GET_DENOISE             _IOR(ISP_IOC_MW, 5, int)
#define ISP_IOC_SET_DENOISE             _IOW(ISP_IOC_MW, 5, int)
#define ISP_IOC_GET_SHARPNESS           _IOR(ISP_IOC_MW, 6, int)
#define ISP_IOC_SET_SHARPNESS           _IOW(ISP_IOC_MW, 6, int)
#define ISP_IOC_GET_GAMMA_TYPE          _IOR(ISP_IOC_MW, 7, int)
#define ISP_IOC_SET_GAMMA_TYPE          _IOW(ISP_IOC_MW, 7, int)
#define ISP_IOC_GET_DR_MODE             _IOR(ISP_IOC_MW, 8, int)
#define ISP_IOC_SET_DR_MODE             _IOW(ISP_IOC_MW, 8, int)
#define ISP_IOC_SET_GAMMA_CURVE         _IOW(ISP_IOC_MW, 9, gamma_curve_t *)

#define ISP_IOC_SET_ADJUST_ALL_EN       _IOW(ISP_IOC_MW, 63, int)
#define ISP_IOC_GET_ADJUST_BLC_EN       _IOR(ISP_IOC_MW, 64, int)
#define ISP_IOC_SET_ADJUST_BLC_EN       _IOW(ISP_IOC_MW, 64, int)
#define ISP_IOC_GET_ADJUST_NR_EN        _IOR(ISP_IOC_MW, 65, int)
#define ISP_IOC_SET_ADJUST_NR_EN        _IOW(ISP_IOC_MW, 65, int)
#define ISP_IOC_GET_ADJUST_GAMMA_EN     _IOR(ISP_IOC_MW, 66, int)
#define ISP_IOC_SET_ADJUST_GAMMA_EN     _IOW(ISP_IOC_MW, 66, int)
#define ISP_IOC_GET_ADJUST_SAT_EN       _IOR(ISP_IOC_MW, 67, int)
#define ISP_IOC_SET_ADJUST_SAT_EN       _IOW(ISP_IOC_MW, 67, int)
#define ISP_IOC_GET_ADJUST_CE_EN        _IOR(ISP_IOC_MW, 68, int)
#define ISP_IOC_SET_ADJUST_CE_EN        _IOW(ISP_IOC_MW, 68, int)
#define ISP_IOC_SET_ADJUST_TMNR_K       _IOW(ISP_IOC_MW, 69, int)

#define ISP_IOC_GET_AUTO_ADJ_PARAM      _IOR(ISP_IOC_MW, 128, adj_param_t *)
#define ISP_IOC_SET_AUTO_ADJ_PARAM      _IOW(ISP_IOC_MW, 128, adj_param_t *)
#define ISP_IOC_GET_ADJUST_CV_MATRIX    _IOR(ISP_IOC_MW, 129, adj_matrix_t *)
#define ISP_IOC_SET_ADJUST_CV_MATRIX    _IOW(ISP_IOC_MW, 129, adj_matrix_t *)
#define ISP_IOC_GET_ADJUST_CC_MATRIX    _IOR(ISP_IOC_MW, 130, adj_matrix_t *)
#define ISP_IOC_SET_ADJUST_CC_MATRIX    _IOW(ISP_IOC_MW, 130, adj_matrix_t *)

// Fast tuning functions
#define ISP_IOC_SET_DPC_LEVEL           _IOW(ISP_IOC_MW, 200, int)
#define ISP_IOC_SET_RDN_LEVEL           _IOW(ISP_IOC_MW, 201, int)
#define ISP_IOC_SET_CTK_LEVEL           _IOW(ISP_IOC_MW, 202, int)
#define ISP_IOC_SET_CDN_LEVEL           _IOW(ISP_IOC_MW, 203, int)
#define ISP_IOC_SET_MRNR_LEVEL          _IOW(ISP_IOC_MW, 204, int)  // Reserved
#define ISP_IOC_SET_TMNR_LEVEL          _IOW(ISP_IOC_MW, 205, int)
#define ISP_IOC_SET_SP_LEVEL            _IOW(ISP_IOC_MW, 206, int)
#define ISP_IOC_SET_SP_NR_LEVEL         _IOW(ISP_IOC_MW, 207, int)
#define ISP_IOC_SET_SP_CLIP_LEVEL       _IOW(ISP_IOC_MW, 208, int)
#define ISP_IOC_SET_SAT_LEVEL           _IOW(ISP_IOC_MW, 209, int)

//=============================================================================
// GM sensor IOCTL command definition
//=============================================================================
#define ISP_IOC_SENSOR 's'
#define ISP_IOC_READ_SENSOR_REG         _IOWR(ISP_IOC_SENSOR, 1, reg_info_t)
#define ISP_IOC_WRITE_SENSOR_REG        _IOWR(ISP_IOC_SENSOR, 2, reg_info_t)
#define ISP_IOC_GET_SENSOR_EXP          _IOR(ISP_IOC_SENSOR, 3, u32)
#define ISP_IOC_SET_SENSOR_EXP          _IOW(ISP_IOC_SENSOR, 3, u32)
#define ISP_IOC_GET_SENSOR_GAIN         _IOR(ISP_IOC_SENSOR, 4, u32)
#define ISP_IOC_SET_SENSOR_GAIN         _IOW(ISP_IOC_SENSOR, 4, u32)
#define ISP_IOC_GET_SENSOR_FPS          _IOR(ISP_IOC_SENSOR, 5, int)
#define ISP_IOC_SET_SENSOR_FPS          _IOW(ISP_IOC_SENSOR, 5, int)
#define ISP_IOC_GET_SENSOR_MIRROR       _IOR(ISP_IOC_SENSOR, 6, int)
#define ISP_IOC_SET_SENSOR_MIRROR       _IOW(ISP_IOC_SENSOR, 6, int)
#define ISP_IOC_GET_SENSOR_FLIP         _IOR(ISP_IOC_SENSOR, 7, int)
#define ISP_IOC_SET_SENSOR_FLIP         _IOW(ISP_IOC_SENSOR, 7, int)
#define ISP_IOC_GET_SENSOR_AE_EN        _IOR(ISP_IOC_SENSOR, 8, int)
#define ISP_IOC_SET_SENSOR_AE_EN        _IOW(ISP_IOC_SENSOR, 8, int)
#define ISP_IOC_GET_SENSOR_AWB_EN       _IOR(ISP_IOC_SENSOR, 9, int)
#define ISP_IOC_SET_SENSOR_AWB_EN       _IOW(ISP_IOC_SENSOR, 9, int)
#define ISP_IOC_GET_SENSOR_MIN_GAIN     _IOR(ISP_IOC_SENSOR, 10, u32)
#define ISP_IOC_GET_SENSOR_MAX_GAIN     _IOR(ISP_IOC_SENSOR, 11, u32)
#define ISP_IOC_GET_SENSOR_SIZE         _IOR(ISP_IOC_SENSOR, 12, win_size_t)
#define ISP_IOC_SET_SENSOR_SIZE         _IOW(ISP_IOC_SENSOR, 12, win_size_t)
#define ISP_IOC_GET_SENSOR_A_GAIN       _IOR(ISP_IOC_SENSOR, 13, u32)
#define ISP_IOC_SET_SENSOR_A_GAIN       _IOW(ISP_IOC_SENSOR, 13, u32)
#define ISP_IOC_GET_SENSOR_MIN_A_GAIN   _IOR(ISP_IOC_SENSOR, 14, u32)
#define ISP_IOC_GET_SENSOR_MAX_A_GAIN   _IOR(ISP_IOC_SENSOR, 15, u32)
#define ISP_IOC_GET_SENSOR_D_GAIN       _IOR(ISP_IOC_SENSOR, 16, u32)
#define ISP_IOC_SET_SENSOR_D_GAIN       _IOW(ISP_IOC_SENSOR, 16, u32)
#define ISP_IOC_GET_SENSOR_MIN_D_GAIN   _IOR(ISP_IOC_SENSOR, 17, u32)
#define ISP_IOC_GET_SENSOR_MAX_D_GAIN   _IOR(ISP_IOC_SENSOR, 18, u32)
#define ISP_IOC_GET_SENSOR_EXP_US       _IOR(ISP_IOC_SENSOR, 19, u32)
#define ISP_IOC_SET_SENSOR_EXP_US       _IOW(ISP_IOC_SENSOR, 19, u32)

//=============================================================================
// GM lens IOCTL command definition
//=============================================================================
#define ISP_IOC_LENS 'l'
#define LENS_IOC_GET_ZOOM_INFO          _IOR(ISP_IOC_LENS, 1, zoom_info_t*)
#define LENS_IOC_SET_ZOOM_MOVE          _IOW(ISP_IOC_LENS, 2, int)
#define LENS_IOC_GET_FOCUS_RANGE        _IOR(ISP_IOC_LENS, 3, focus_range_t *)
#define LENS_IOC_SET_FOCUS_INIT         _IOW(ISP_IOC_LENS, 4, int)
#define LENS_IOC_GET_FOCUS_POS          _IOR(ISP_IOC_LENS, 5, int)
#define LENS_IOC_SET_FOCUS_POS          _IOW(ISP_IOC_LENS, 6, int)
#define LENS_IOC_SET_ZOOM_INIT          _IOW(ISP_IOC_LENS, 7, int)

//=============================================================================
// GM AE IOCTL command definition
//=============================================================================
#define ISP_IOC_AE 'e'
#define AE_IOC_GET_ENABLE               _IOR(ISP_IOC_AE, 0, int)
#define AE_IOC_SET_ENABLE               _IOW(ISP_IOC_AE, 0, int)
#define AE_IOC_GET_CURRENTY             _IOR(ISP_IOC_AE, 1, int)
#define AE_IOC_GET_TARGETY              _IOR(ISP_IOC_AE, 2, int)
#define AE_IOC_SET_TARGETY              _IOW(ISP_IOC_AE, 2, int)
#define AE_IOC_GET_CONVERGE_TH          _IOR(ISP_IOC_AE, 3, int)
#define AE_IOC_SET_CONVERGE_TH          _IOW(ISP_IOC_AE, 3, int)
#define AE_IOC_GET_SPEED                _IOR(ISP_IOC_AE, 4, int)
#define AE_IOC_SET_SPEED                _IOW(ISP_IOC_AE, 4, int)
#define AE_IOC_GET_PWR_FREQ             _IOR(ISP_IOC_AE, 5, int)
#define AE_IOC_SET_PWR_FREQ             _IOW(ISP_IOC_AE, 5, int)
#define AE_IOC_GET_MIN_EXP              _IOR(ISP_IOC_AE, 6, u32)
#define AE_IOC_SET_MIN_EXP              _IOW(ISP_IOC_AE, 6, u32)
#define AE_IOC_GET_MAX_EXP              _IOR(ISP_IOC_AE, 7, u32)
#define AE_IOC_SET_MAX_EXP              _IOW(ISP_IOC_AE, 7, u32)
#define AE_IOC_GET_MIN_GAIN             _IOR(ISP_IOC_AE, 8, u32)
#define AE_IOC_SET_MIN_GAIN             _IOW(ISP_IOC_AE, 8, u32)
#define AE_IOC_GET_MAX_GAIN             _IOR(ISP_IOC_AE, 9, u32)
#define AE_IOC_SET_MAX_GAIN             _IOW(ISP_IOC_AE, 9, u32)
#define AE_IOC_GET_SLOW_SHOOT           _IOR(ISP_IOC_AE, 10, int)
#define AE_IOC_SET_SLOW_SHOOT           _IOW(ISP_IOC_AE, 10, int)
#define AE_IOC_GET_SLOW_SHOOT_DELTA     _IOR(ISP_IOC_AE, 11, int)
#define AE_IOC_SET_SLOW_SHOOT_DELTA     _IOW(ISP_IOC_AE, 11, int)
#define AE_IOC_GET_PHOTOMETRY_METHOD    _IOR(ISP_IOC_AE, 12, int)
#define AE_IOC_SET_PHOTOMETRY_METHOD    _IOW(ISP_IOC_AE, 12, int)
#define AE_IOC_GET_DELAY                _IOR(ISP_IOC_AE, 13, int)
#define AE_IOC_SET_DELAY                _IOW(ISP_IOC_AE, 13, int)
#define AE_IOC_GET_MODE                 _IOR(ISP_IOC_AE, 14, int)
#define AE_IOC_SET_MODE                 _IOW(ISP_IOC_AE, 14, int)
#define AE_IOC_GET_CONST_EXP            _IOR(ISP_IOC_AE, 15, u32)
#define AE_IOC_SET_CONST_EXP            _IOW(ISP_IOC_AE, 15, u32)
#define AE_IOC_GET_CONST_GAIN           _IOR(ISP_IOC_AE, 16, u32)
#define AE_IOC_SET_CONST_GAIN           _IOW(ISP_IOC_AE, 16, u32)
#define AE_IOC_GET_IRIS_ENABLE          _IOR(ISP_IOC_AE, 17, int)
#define AE_IOC_SET_IRIS_ENABLE          _IOW(ISP_IOC_AE, 17, int)
#define AE_IOC_GET_IRIS_PROBE           _IOR(ISP_IOC_AE, 18, int)
#define AE_IOC_SET_IRIS_PROBE           _IOW(ISP_IOC_AE, 18, int)
#define AE_IOC_GET_IRIS_PID             _IOR(ISP_IOC_AE, 19, ae_iris_pid_t *)
#define AE_IOC_SET_IRIS_PID             _IOW(ISP_IOC_AE, 19, ae_iris_pid_t *)
#define AE_IOC_GET_IRIS_MIN_EXP         _IOR(ISP_IOC_AE, 20, u32)
#define AE_IOC_SET_IRIS_MIN_EXP         _IOW(ISP_IOC_AE, 20, u32)
#define AE_IOC_GET_IRIS_BAL_RATIO       _IOR(ISP_IOC_AE, 21, int)
#define AE_IOC_SET_IRIS_BAL_RATIO       _IOW(ISP_IOC_AE, 21, int)
#define AE_IOC_GET_EV_MODE              _IOR(ISP_IOC_AE, 22, int)
#define AE_IOC_SET_EV_MODE              _IOW(ISP_IOC_AE, 22, int)
#define AE_IOC_GET_WIN_WEIGHT           _IOR(ISP_IOC_AE, 24, int *)
#define AE_IOC_SET_WIN_WEIGHT           _IOW(ISP_IOC_AE, 24, int *)
#define AE_IOC_GET_AUTO_DRC             _IOR(ISP_IOC_AE, 25, int)
#define AE_IOC_SET_AUTO_DRC             _IOW(ISP_IOC_AE, 25, int)
#define AE_IOC_GET_AUTO_CONTRAST        _IOR(ISP_IOC_AE, 26, int)
#define AE_IOC_SET_AUTO_CONTRAST        _IOW(ISP_IOC_AE, 26, int)
#define AE_IOC_GET_HIGH_LIGHT_SUPP      _IOR(ISP_IOC_AE, 27, int)
#define AE_IOC_SET_HIGH_LIGHT_SUPP      _IOW(ISP_IOC_AE, 27, int)
#define AE_IOC_SET_TOTAL_GAIN           _IOW(ISP_IOC_AE, 28, u32)
#define AE_IOC_GET_MAX_ISP_GAIN         _IOR(ISP_IOC_AE, 29, u32)
#define AE_IOC_SET_MAX_ISP_GAIN         _IOW(ISP_IOC_AE, 29, u32)
#define AE_IOC_GET_VWP_RATIO_TH         _IOR(ISP_IOC_AE, 30, int)
#define AE_IOC_SET_VWP_RATIO_TH         _IOW(ISP_IOC_AE, 30, int)
#define AE_IOC_GET_EV_VALUE             _IOR(ISP_IOC_AE, 31, u32)

//=============================================================================
// GM AWB IOCTL command definition
//=============================================================================
#define ISP_IOC_AWB 'w'
#define AWB_IOC_GET_ENABLE              _IOR(ISP_IOC_AWB, 0, int)
#define AWB_IOC_SET_ENABLE              _IOW(ISP_IOC_AWB, 0, int)
#define AWB_IOC_GET_TH                  _IOR(ISP_IOC_AWB, 1, int *)
#define AWB_IOC_SET_TH                  _IOW(ISP_IOC_AWB, 1, int *)
#define AWB_IOC_GET_RG_RATIO            _IOR(ISP_IOC_AWB, 2, int)
#define AWB_IOC_GET_BG_RATIO            _IOR(ISP_IOC_AWB, 3, int)
#define AWB_IOC_GET_RB_RATIO            _IOR(ISP_IOC_AWB, 4, int)
#define AWB_IOC_GET_CONVERGE_TH         _IOR(ISP_IOC_AWB, 5, int)
#define AWB_IOC_SET_CONVERGE_TH         _IOW(ISP_IOC_AWB, 5, int)
#define AWB_IOC_GET_SPEED               _IOR(ISP_IOC_AWB, 6, int)
#define AWB_IOC_SET_SPEED               _IOW(ISP_IOC_AWB, 6, int)
#define AWB_IOC_GET_FREEZE_SEG          _IOR(ISP_IOC_AWB, 7, int)
#define AWB_IOC_SET_FREEZE_SEG          _IOW(ISP_IOC_AWB, 7, int)
#define AWB_IOC_GET_RG_TARGET           _IOR(ISP_IOC_AWB, 8, awb_target_t)
#define AWB_IOC_SET_RG_TARGET           _IOW(ISP_IOC_AWB, 8, awb_target_t)
#define AWB_IOC_GET_BG_TARGET           _IOR(ISP_IOC_AWB, 9, awb_target_t)
#define AWB_IOC_SET_BG_TARGET           _IOW(ISP_IOC_AWB, 9, awb_target_t)
#define AWB_IOC_GET_SCENE_MODE          _IOR(ISP_IOC_AWB, 10, int)
#define AWB_IOC_SET_SCENE_MODE          _IOW(ISP_IOC_AWB, 10, int)
#define AWB_IOC_GET_STA_MODE            _IOR(ISP_IOC_AWB, 11, int)
#define AWB_IOC_SET_STA_MODE            _IOW(ISP_IOC_AWB, 11, int)
#define AWB_IOC_GET_ADJUST_CC           _IOR(ISP_IOC_AWB, 12, int)
#define AWB_IOC_SET_ADJUST_CC           _IOW(ISP_IOC_AWB, 12, int)

#define AWB_IOC_SET_OUTDOOR_EV_TH       _IOW(ISP_IOC_AWB, 200, int)

#define AWB_IOC_SET_ROI0_PARAM          _IOW(ISP_IOC_AWB, 210, awb_roi_t *)
#define AWB_IOC_SET_ROI1_PARAM          _IOW(ISP_IOC_AWB, 211, awb_roi_t *)
#define AWB_IOC_SET_ROI2_PARAM          _IOW(ISP_IOC_AWB, 212, awb_roi_t *)
#define AWB_IOC_SET_ROI3_PARAM          _IOW(ISP_IOC_AWB, 213, awb_roi_t *)
#define AWB_IOC_SET_ROI4_PARAM          _IOW(ISP_IOC_AWB, 214, awb_roi_t *)

//=============================================================================
// GM AF IOCTL command definition
//=============================================================================
#define ISP_IOC_AF 'f'
#define AF_IOC_GET_ENABLE               _IOR(ISP_IOC_AF, 0, int)
#define AF_IOC_SET_ENABLE               _IOW(ISP_IOC_AF, 0, int)
#define AF_IOC_CHECK_BUSY               _IOR(ISP_IOC_AF, 1, int)
#define AF_IOC_GET_SHOT_MODE            _IOR(ISP_IOC_AF, 2, int)
#define AF_IOC_SET_SHOT_MODE            _IOW(ISP_IOC_AF, 2, int)
#define AF_IOC_SET_RE_TRIGGER           _IO(ISP_IOC_AF, 3)
#define AF_IOC_SET_CFG_PARAM            _IO(ISP_IOC_AF, 4)
#define AF_IOC_GET_AF_METHOD            _IOR(ISP_IOC_AF, 5, int)
#define AF_IOC_SET_AF_METHOD            _IOW(ISP_IOC_AF, 5, int)

#endif /* __IOCTL_ISP328_H__ */
