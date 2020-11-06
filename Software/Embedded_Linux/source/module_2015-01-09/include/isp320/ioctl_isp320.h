/**
 * @file ioctl_isp320.h
 * ISP320 IOCTLs
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.55 $
 * $Date: 2014/12/09 04:07:23 $
 *
 * ChangeLog:
 *  $Log: ioctl_isp320.h,v $
 *  Revision 1.55  2014/12/09 04:07:23  ricktsao
 *  add ISP_IOC_GET_CG_FORMAT / ISP_IOC_SET_CG_FORMAT
 *
 *  Revision 1.54  2014/12/08 09:17:25  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.53  2014/11/27 09:00:12  ricktsao
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
 *  Revision 1.52  2014/10/22 07:30:52  ricktsao
 *  no message
 *
 *  Revision 1.51  2014/10/22 07:26:10  ricktsao
 *  add ISP_IOC_SET_GAMMA_CURVE
 *
 *  Revision 1.50  2014/10/21 11:30:21  ricktsao
 *  modify ISP_IOC_GET_GAMMA_TYPE / ISP_IOC_SET_GAMMA_TYPE
 *
 *  Revision 1.49  2014/10/03 01:36:12  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.48  2014/10/01 06:53:56  ricktsao
 *  add AE_IOC_GET_EV_VALUE
 *
 *  Revision 1.47  2014/09/29 16:39:46  ricktsao
 *  no message
 *
 *  Revision 1.46  2014/09/19 01:47:43  ricktsao
 *  no message
 *
 *  Revision 1.45  2014/09/10 04:25:58  ricktsao
 *  no message
 *
 *  Revision 1.44  2014/08/25 07:12:24  ricktsao
 *  add histogram channel selection option
 *
 *  Revision 1.43  2014/08/21 07:09:25  ricktsao
 *  no message
 *
 *  Revision 1.42  2014/08/19 06:54:30  ricktsao
 *  add IOCTL to get/set sensor analog gain and digital gain
 *
 *  Revision 1.41  2014/08/13 09:38:16  ricktsao
 *  add ISP_IOC_GET_ADJUST_CC_MATRIX/ISP_IOC_SET_ADJUST_CC_MATRIX
 *         ISP_IOC_GET_ADJUST_CV_MATRIX/ISP_IOC_SET_ADJUST_CV_MATRIX
 *
 *  Revision 1.40  2014/08/01 11:30:22  ricktsao
 *  add image pipe control IOCTLs
 *
 *  Revision 1.39  2014/07/24 11:53:48  ricktsao
 *  no message
 *
 *  Revision 1.38  2014/07/21 09:37:43  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.37  2014/07/21 02:47:28  ricktsao
 *  add auto gamma adjustment
 *
 *  Revision 1.36  2014/07/08 09:23:50  allenhsu
 *  add AE_IOC_SET_HIGH_LIGHT_SUPP, AE_IOC_GET_HIGH_LIGHT_SUPP, AE_IOC_SET_TOTAL_GAIN
 *
 *  Revision 1.35  2014/07/01 03:11:56  ricktsao
 *  Support AWB segment mode statistic
 *
 *  Revision 1.34  2014/06/11 07:11:29  ricktsao
 *  no message
 *
 *  Revision 1.33  2014/05/30 08:23:40  jtwang
 *  add set size to IOC
 *
 *  Revision 1.32  2014/05/27 05:59:38  jtwang
 *  add set/get sensor_size to IOC
 *
 *  Revision 1.31  2014/05/13 03:48:08  ricktsao
 *  add ISP_IOC_GET_ACT_WIN/ISP_IOC_SET_ACT_WIN
 *
 *  Revision 1.30  2014/05/12 03:39:06  jtwang
 *  add AWB scene mode
 *
 *  Revision 1.29  2014/05/06 06:20:11  ricktsao
 *  add ioctl for get and set cc matrix and cv matrix
 *
 *  Revision 1.28  2014/05/05 08:53:34  ricktsao
 *  Add IOCTL to get driver version
 *
 *  Revision 1.27  2014/04/29 08:40:07  ricktsao
 *  no message
 *
 *  Revision 1.26  2014/04/23 09:59:27  ricktsao
 *  no message
 *
 *  Revision 1.25  2014/04/22 08:15:51  ricktsao
 *  no message
 *
 *  Revision 1.24  2014/04/18 01:01:34  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.23  2014/04/16 10:09:30  ricktsao
 *  no message
 *
 *  Revision 1.22  2014/04/15 05:03:14  ricktsao
 *  no message
 *
 *  Revision 1.21  2014/04/15 02:04:30  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.20  2014/04/08 09:48:27  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.19  2014/04/07 08:31:44  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.18  2014/04/07 08:29:05  ricktsao
 *  no message
 *
 *  Revision 1.17  2014/03/28 09:57:32  allenhsu
 *  Add AF
 *
 *  Revision 1.16  2014/03/06 03:30:32  allenhsu
 *  Add
 *  ISP_IOC_GET_SENSOR_MIN_GAIN, ISP_IOC_GET_SENSOR_MAX_GAIN
 *  AE_IOC_GET_PHOTOMETRY_METHOD, AE_IOC_SET_PHOTOMETRY_METHOD
 *
 *  Revision 1.15  2013/12/11 05:14:32  ricktsao
 *  no message
 *
 *  Revision 1.14  2013/12/03 07:03:19  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.13  2013/11/25 11:59:36  ricktsao
 *  no message
 *
 *  Revision 1.12  2013/11/15 10:17:10  ricktsao
 *  no message
 *
 *  Revision 1.11  2013/11/13 09:47:54  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.10  2013/11/08 08:33:21  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.9  2013/11/07 06:10:04  ricktsao
 *  no message
 *
 *  Revision 1.8  2013/11/07 03:05:34  ricktsao
 *  no message
 *
 *  Revision 1.7  2013/11/06 10:33:57  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.6  2013/11/06 10:03:35  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.5  2013/11/06 09:35:51  allenhsu
 *  Add ioctl for MRNR, 3DNR
 *
 *  Revision 1.4  2013/11/06 09:06:43  ricktsao
 *  no message
 *
 *  Revision 1.3  2013/11/06 07:25:27  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.2  2013/10/21 10:27:37  ricktsao
 *  no message
 *
 *  Revision 1.1  2013/09/27 05:13:12  ricktsao
 *  no message
 *
 */

#ifndef __IOCTL_ISP320_H__
#define __IOCTL_ISP320_H__

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
    FUNC0_LI        = BIT3,
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
    FUNC0_CC        = BIT18,
    FUNC0_BRIGHT    = BIT19,
    FUNC0_CONTRAST  = BIT20,
    FUNC0_HUESAT    = BIT22,
    FUNC0_SK        = BIT26,
    FUNC0_HBNR      = BIT27,
    FUNC0_HBAJ      = BIT28,
    FUNC0_SP        = BIT29,
    FUNC0_CS        = BIT30,
    FUNC0_ALL       = 0x7C5FF97B,
};

enum ISP_FUNC1 {
    FUNC1_GG        = BIT20,
    FUNC1_CG        = BIT21,
    FUNC1_CV        = BIT22,
    FUNC1_ALL       = 0x00700000,
};

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

typedef struct blc_reg {
    s16 offset[4];  // R, Gr, Gb, B
} blc_reg_t;

typedef struct li_lut {
    u8 rlut[16];
    u8 glut[16];
    u8 blut[16];
} li_lut_t;

typedef struct dpc_reg {
    u8 dpc_mode;
    u8 rvc_mode;
    u8 dpcn;
    u16 th[3];
} dpc_reg_t;

typedef struct rdn_reg {
    u32 th[4];
    u8  w[4];
} rdn_reg_t;

typedef struct ctk_reg {
    u32 th[4];
    u8  w[4];
    u8  gbgr;
} ctk_reg_t;

typedef struct cdn_reg {
    u8 th[4];
    u8 w[4];
} cdn_reg_t;

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

typedef struct sk_reg {
    int mode;
    u8  sel;
    s16 sk_th[6];
    s16 slope0;
    s16 slope1;
    s8  offset0;
    s8  offset1;
    u16 filter_k[3];
    s8  fixed_cb;
    s8  fixed_cr;
    s8  deg;
    u16 sat;
    u16 ran;
} sk_reg_t;

typedef struct hbnr_reg {
    u16 edge_th;
    u8  luma_filter;
    u8  chroma_filter;
} hbnr_reg_t;

typedef struct hbnr_lut {
    u8  lutl[64];
    u8  lutc[64];
} hbnr_lut_t;

typedef struct hbnr_aj {
    u16 edge_th;
} hbnr_aj_t;

typedef struct sp_clip {
    u8 th;
    u8 clip0;
    u8 clip1;
    u8 reduce;
} sp_clip_t;

typedef struct sp_curve {
    u8 F[128];
} sp_curve_t;

typedef struct cs_reg {
    u8  in[4];
    u16 out[3];
    u16 slope[4];
    u16 cb_th;
    u16 cr_th;
} cs_reg_t;

enum DR_MODE {
    DR_LINEAR = 0,
    DR_WDR,
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

//Tricky: this struct is duplicated "from ft3dnr/api_if.h"
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

typedef struct tmnr_var {
    int Y_var;
    int Cb_var;
    int Cr_var;
} tmnr_var_t;

// statistic ready flag
#define ISP_AE_STA_READY    BIT0
#define ISP_AWB_STA_READY   BIT1
#define ISP_AF_STA_READY    BIT2
#define ISP_HIST_STA_READY  BIT3

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
    HIST_AFTER_CC,
    HIST_AFTER_CS,
    HIST_AFTER_DRC,
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
    int tss_hist_idx;
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

typedef struct _zoom_info{
    int zoom_stage_cnt;   //zoom stage count
    int curr_zoom_index;  //current zoom stage index
    int curr_zoom_x10;    //zoom magnification
} zoom_info_t;

typedef struct _focus_range{
    int min;
    int max;
} focus_range_t;

//=====================================
// Auto adjustment
//=====================================
#define ADJ_SEGMENTS    9

typedef struct adj_param {
    int gain_th[ADJ_SEGMENTS];
    int blc[ADJ_SEGMENTS][3];
    int nl_raw[ADJ_SEGMENTS];
    int nl_ctk[ADJ_SEGMENTS];
    int nl_ci[ADJ_SEGMENTS];
    int dpc_th1[ADJ_SEGMENTS];
    int dpc_th2[ADJ_SEGMENTS];
    int dpc_mode[ADJ_SEGMENTS];
    int dpc_dpcn[ADJ_SEGMENTS];
    int nl_hbl[ADJ_SEGMENTS];
    int nl_hbc[ADJ_SEGMENTS];
    int sp_lvl[ADJ_SEGMENTS];
    int saturation_lvl[ADJ_SEGMENTS];
    int hbl_th[3];
    int hbc_th[3];
    int gamma_idx[ADJ_SEGMENTS];
} adj_param_t;

typedef struct adj_matrix {
    s16 matrix[3][9];
} adj_matrix_t;

//=============================================================================
// IOCTL command definition
//=============================================================================
#define ISP_IOC_GET_CHIPID              _IOR(0, 0, u32)
#define ISP_IOC_GET_DRIVER_VER          _IOR(0, 1, u32)
#define ISP_IOC_GET_INPUT_INF_VER       _IOR(0, 2, u32)
#define ISP_IOC_GET_ALG_INF_VER         _IOR(0, 3, u32)

#define ISP_IOC_ISP 'i'
#define ISP_IOC_GET_STATUS              _IOR(ISP_IOC_ISP, 0, int)
#define ISP_IOC_START                   _IO(ISP_IOC_ISP, 1)
#define ISP_IOC_STOP                    _IO(ISP_IOC_ISP, 2)
#define ISP_IOC_READ_REG                _IOWR(ISP_IOC_ISP, 3, struct reg_info)
#define ISP_IOC_WRITE_REG               _IOWR(ISP_IOC_ISP, 4, struct reg_info)
#define ISP_IOC_READ_REG_LIST           _IOWR(ISP_IOC_ISP, 5, struct reg_list *)
#define ISP_IOC_WRITE_REG_LIST          _IOWR(ISP_IOC_ISP, 6, struct reg_list *)
#define ISP_IOC_GET_FRAME_SIZE          _IOR(ISP_IOC_ISP, 7, u32)
#define ISP_IOC_GRAB_FRAME              _IOR(ISP_IOC_ISP, 8, u8 *)  // for isp_demon
#define ISP_IOC_LOAD_CFG                _IOW(ISP_IOC_ISP, 9, char *)
#define ISP_IOC_WAIT_STA_READY          _IOR(ISP_IOC_ISP, 10, u32)
#define ISP_IOC_GET_STA_READY_FLAG      _IOR(ISP_IOC_ISP, 11, u32)
#define ISP_IOC_GET_CFG_FILE_PATH       _IOR(ISP_IOC_ISP, 12, char *)
#define ISP_IOC_GET_CFG_FILE_SIZE       _IOR(ISP_IOC_ISP, 13, u32)  // for isp_demon
#define ISP_IOC_READ_CFG_FILE           _IOR(ISP_IOC_ISP, 14, u8 *) // for isp_demon
#define ISP_IOC_WRITE_CFG_FILE          _IOW(ISP_IOC_ISP, 15, u8 *) // for isp_demon
#define ISP_IOC_GET_ACT_WIN             _IOR(ISP_IOC_ISP, 16, struct win_rect)
#define ISP_IOC_SET_ACT_WIN             _IOW(ISP_IOC_ISP, 16, struct win_rect)
#define ISP_IOC_GET_SIZE                _IOR(ISP_IOC_ISP, 17, struct win_rect)
#define ISP_IOC_SET_SIZE                _IOW(ISP_IOC_ISP, 17, struct win_rect)
#define ISP_IOC_GET_FUNC0_STS           _IOR(ISP_IOC_ISP, 18, u32)
#define ISP_IOC_ENABLE_FUNC0            _IOW(ISP_IOC_ISP, 19, u32)
#define ISP_IOC_DISABLE_FUNC0           _IOW(ISP_IOC_ISP, 20, u32)
#define ISP_IOC_GET_FUNC1_STS           _IOR(ISP_IOC_ISP, 21, u32)
#define ISP_IOC_ENABLE_FUNC1            _IOW(ISP_IOC_ISP, 22, u32)
#define ISP_IOC_DISABLE_FUNC1           _IOW(ISP_IOC_ISP, 23, u32)

// 3A statistic
#define ISP_IOC_GET_HIST_CFG            _IOR(ISP_IOC_ISP, 32, struct hist_sta_cfg_t *)
#define ISP_IOC_SET_HIST_CFG            _IOW(ISP_IOC_ISP, 32, struct hist_sta_cfg_t *)
#define ISP_IOC_GET_HIST_WIN            _IOR(ISP_IOC_ISP, 33, struct hist_win_cfg *)
#define ISP_IOC_SET_HIST_WIN            _IOW(ISP_IOC_ISP, 33, struct hist_win_cfg *)
#define ISP_IOC_GET_HIST_PRE_GAMMA      _IOR(ISP_IOC_ISP, 34, struct hist_pre_gamma *)
#define ISP_IOC_SET_HIST_PRE_GAMMA      _IOW(ISP_IOC_ISP, 34, struct hist_pre_gamma *)
#define ISP_IOC_GET_HISTOGRAM           _IOR(ISP_IOC_ISP, 35, struct hist_sta *)
#define ISP_IOC_GET_AE_CFG              _IOR(ISP_IOC_ISP, 36, struct ae_sta_cfg *)
#define ISP_IOC_SET_AE_CFG              _IOW(ISP_IOC_ISP, 36, struct ae_sta_cfg *)
#define ISP_IOC_GET_AE_WIN              _IOR(ISP_IOC_ISP, 37, struct ae_win_cfg *)
#define ISP_IOC_SET_AE_WIN              _IOW(ISP_IOC_ISP, 37, struct ae_win_cfg *)
#define ISP_IOC_GET_AE_PRE_GAMMA        _IOR(ISP_IOC_ISP, 38, struct ae_pre_gamma *)
#define ISP_IOC_SET_AE_PRE_GAMMA        _IOW(ISP_IOC_ISP, 38, struct ae_pre_gamma *)
#define ISP_IOC_GET_AE_STA              _IOR(ISP_IOC_ISP, 39, struct ae_sta *)
#define ISP_IOC_GET_AWB_CFG             _IOR(ISP_IOC_ISP, 40, struct awb_sta_cfg *)
#define ISP_IOC_SET_AWB_CFG             _IOW(ISP_IOC_ISP, 40, struct awb_sta_cfg *)
#define ISP_IOC_GET_AWB_WIN             _IOR(ISP_IOC_ISP, 41, struct awb_win_cfg *)
#define ISP_IOC_SET_AWB_WIN             _IOW(ISP_IOC_ISP, 41, struct awb_win_cfg *)
#define ISP_IOC_GET_AWB_STA             _IOR(ISP_IOC_ISP, 42, struct awb_sta *)
#define ISP_IOC_GET_AWB_SEG_STA         _IOR(ISP_IOC_ISP, 43, struct awb_seg_sta *)
#define ISP_IOC_GET_AF_CFG              _IOR(ISP_IOC_ISP, 44, struct af_sta_cfg *)
#define ISP_IOC_SET_AF_CFG              _IOW(ISP_IOC_ISP, 44, struct af_sta_cfg *)
#define ISP_IOC_GET_AF_WIN              _IOR(ISP_IOC_ISP, 45, struct af_win_cfg *)
#define ISP_IOC_SET_AF_WIN              _IOW(ISP_IOC_ISP, 45, struct af_win_cfg *)
#define ISP_IOC_GET_AF_PRE_GAMMA        _IOR(ISP_IOC_ISP, 46, struct af_pre_gamma *)
#define ISP_IOC_SET_AF_PRE_GAMMA        _IOW(ISP_IOC_ISP, 46, struct af_pre_gamma *)
#define ISP_IOC_GET_AF_STA              _IOR(ISP_IOC_ISP, 47, struct af_sta *)

// Image pipe control
#define ISP_IOC_SET_R_GAIN              _IOW(ISP_IOC_ISP, 48, u32)
#define ISP_IOC_GET_R_GAIN              _IOR(ISP_IOC_ISP, 48, u32)
#define ISP_IOC_SET_G_GAIN              _IOW(ISP_IOC_ISP, 49, u32)
#define ISP_IOC_GET_G_GAIN              _IOR(ISP_IOC_ISP, 49, u32)
#define ISP_IOC_SET_B_GAIN              _IOW(ISP_IOC_ISP, 50, u32)
#define ISP_IOC_GET_B_GAIN              _IOR(ISP_IOC_ISP, 50, u32)
#define ISP_IOC_SET_GAMMA               _IOW(ISP_IOC_ISP, 51, struct gamma_lut *)
#define ISP_IOC_GET_GAMMA               _IOR(ISP_IOC_ISP, 51, struct gamma_lut *)
#define ISP_IOC_SET_CC_MATRIX           _IOW(ISP_IOC_ISP, 52, struct cc_matrix)
#define ISP_IOC_GET_CC_MATRIX           _IOR(ISP_IOC_ISP, 52, struct cc_matrix)
#define ISP_IOC_SET_CV_MATRIX           _IOW(ISP_IOC_ISP, 53, struct cv_matrix)
#define ISP_IOC_GET_CV_MATRIX           _IOR(ISP_IOC_ISP, 53, struct cv_matrix)
#define ISP_IOC_GET_BLC                 _IOR(ISP_IOC_ISP, 54, blc_reg_t *)
#define ISP_IOC_SET_BLC                 _IOW(ISP_IOC_ISP, 54, blc_reg_t *)
#define ISP_IOC_GET_LI                  _IOR(ISP_IOC_ISP, 55, li_lut_t *)
#define ISP_IOC_SET_LI                  _IOW(ISP_IOC_ISP, 55, li_lut_t *)
#define ISP_IOC_GET_DPC                 _IOR(ISP_IOC_ISP, 56, dpc_reg_t *)
#define ISP_IOC_SET_DPC                 _IOW(ISP_IOC_ISP, 56, dpc_reg_t *)
#define ISP_IOC_GET_RAW_NR              _IOR(ISP_IOC_ISP, 57, rdn_reg_t *)
#define ISP_IOC_SET_RAW_NR              _IOW(ISP_IOC_ISP, 57, rdn_reg_t *)
#define ISP_IOC_GET_RAW_CTK             _IOR(ISP_IOC_ISP, 58, ctk_reg_t *)
#define ISP_IOC_SET_RAW_CTK             _IOW(ISP_IOC_ISP, 58, ctk_reg_t *)
#define ISP_IOC_GET_CI_NR               _IOR(ISP_IOC_ISP, 59, cdn_reg_t *)
#define ISP_IOC_SET_CI_NR               _IOW(ISP_IOC_ISP, 59, cdn_reg_t *)
#define ISP_IOC_GET_LSC                 _IOR(ISP_IOC_ISP, 60, lsc_reg_t *)
#define ISP_IOC_SET_LSC                 _IOW(ISP_IOC_ISP, 60, lsc_reg_t *)
#define ISP_IOC_GET_CG_FORMAT           _IOR(ISP_IOC_ISP, 61, int)
#define ISP_IOC_SET_CG_FORMAT           _IOW(ISP_IOC_ISP, 61, int)

// user perference and auto adjustment
#define ISP_IOC_GET_BRIGHTNESS          _IOR(ISP_IOC_ISP, 64, int)
#define ISP_IOC_SET_BRIGHTNESS          _IOW(ISP_IOC_ISP, 64, int)
#define ISP_IOC_GET_CONTRAST            _IOR(ISP_IOC_ISP, 65, int)
#define ISP_IOC_SET_CONTRAST            _IOW(ISP_IOC_ISP, 65, int)
#define ISP_IOC_GET_HUE                 _IOR(ISP_IOC_ISP, 66, int)
#define ISP_IOC_SET_HUE                 _IOW(ISP_IOC_ISP, 66, int)
#define ISP_IOC_GET_SATURATION          _IOR(ISP_IOC_ISP, 67, int)
#define ISP_IOC_SET_SATURATION          _IOW(ISP_IOC_ISP, 67, int)
#define ISP_IOC_GET_DENOISE             _IOR(ISP_IOC_ISP, 68, int)
#define ISP_IOC_SET_DENOISE             _IOW(ISP_IOC_ISP, 68, int)
#define ISP_IOC_GET_SHARPNESS           _IOR(ISP_IOC_ISP, 69, int)
#define ISP_IOC_SET_SHARPNESS           _IOW(ISP_IOC_ISP, 69, int)
#define ISP_IOC_GET_DR_MODE             _IOR(ISP_IOC_ISP, 70, enum DR_MODE)
#define ISP_IOC_SET_DR_MODE             _IOW(ISP_IOC_ISP, 70, enum DR_MODE)
#define ISP_IOC_GET_ADJUST_NR_EN        _IOR(ISP_IOC_ISP, 71, int)
#define ISP_IOC_SET_ADJUST_NR_EN        _IOW(ISP_IOC_ISP, 71, int)
#define ISP_IOC_GET_ADJUST_CC_EN        _IOR(ISP_IOC_ISP, 72, int)
#define ISP_IOC_SET_ADJUST_CC_EN        _IOW(ISP_IOC_ISP, 72, int)
#define ISP_IOC_GET_AUTO_CONTRAST       _IOR(ISP_IOC_ISP, 73, int)  // obsolete
#define ISP_IOC_SET_AUTO_CONTRAST       _IOW(ISP_IOC_ISP, 73, int)  // obsolete
#define ISP_IOC_GET_ADJUST_BLC_EN       _IOR(ISP_IOC_ISP, 74, int)
#define ISP_IOC_SET_ADJUST_BLC_EN       _IOW(ISP_IOC_ISP, 74, int)
#define ISP_IOC_GET_ADJUST_GAMMA_EN     _IOR(ISP_IOC_ISP, 75, int)
#define ISP_IOC_SET_ADJUST_GAMMA_EN     _IOW(ISP_IOC_ISP, 75, int)
#define ISP_IOC_GET_GAMMA_TYPE          _IOR(ISP_IOC_ISP, 76, int)
#define ISP_IOC_SET_GAMMA_TYPE          _IOW(ISP_IOC_ISP, 76, int)
#define ISP_IOC_GET_AUTO_ADJ_PARAM      _IOR(ISP_IOC_ISP, 77, adj_param_t *)
#define ISP_IOC_SET_AUTO_ADJ_PARAM      _IOW(ISP_IOC_ISP, 77, adj_param_t *)
#define ISP_IOC_GET_ADJUST_CC_MATRIX    _IOR(ISP_IOC_ISP, 78, adj_matrix_t *)
#define ISP_IOC_SET_ADJUST_CC_MATRIX    _IOW(ISP_IOC_ISP, 78, adj_matrix_t *)
#define ISP_IOC_GET_ADJUST_CV_MATRIX    _IOR(ISP_IOC_ISP, 79, adj_matrix_t *)
#define ISP_IOC_SET_ADJUST_CV_MATRIX    _IOW(ISP_IOC_ISP, 79, adj_matrix_t *)

// Image pipe control
#define ISP_IOC_GET_MRNR_EN             _IOR(ISP_IOC_ISP, 80, int)
#define ISP_IOC_SET_MRNR_EN             _IOW(ISP_IOC_ISP, 80, int)
#define ISP_IOC_GET_MRNR_PARAM          _IOR(ISP_IOC_ISP, 81, mrnr_param_t *)
#define ISP_IOC_SET_MRNR_PARAM          _IOW(ISP_IOC_ISP, 81, mrnr_param_t *)
#define ISP_IOC_GET_TMNR_EN             _IOR(ISP_IOC_ISP, 82, int)
#define ISP_IOC_SET_TMNR_EN             _IOW(ISP_IOC_ISP, 82, int)
#define ISP_IOC_GET_TMNR_EDG_EN         _IOR(ISP_IOC_ISP, 83, int)
#define ISP_IOC_SET_TMNR_EDG_EN         _IOW(ISP_IOC_ISP, 83, int)
#define ISP_IOC_GET_TMNR_LEARN_EN       _IOR(ISP_IOC_ISP, 84, int)
#define ISP_IOC_SET_TMNR_LEARN_EN       _IOW(ISP_IOC_ISP, 84, int)
#define ISP_IOC_GET_TMNR_PARAM          _IOR(ISP_IOC_ISP, 85, tmnr_var_t *)
#define ISP_IOC_SET_TMNR_PARAM          _IOW(ISP_IOC_ISP, 85, tmnr_var_t *)
#define ISP_IOC_GET_DRC_STRENGTH        _IOR(ISP_IOC_ISP, 86, int)
#define ISP_IOC_SET_DRC_STRENGTH        _IOW(ISP_IOC_ISP, 86, int)
#define ISP_IOC_GET_DRC_BGWIN           _IOR(ISP_IOC_ISP, 87, drc_bgwin_t *)
#define ISP_IOC_SET_DRC_BGWIN           _IOW(ISP_IOC_ISP, 87, drc_bgwin_t *)
#define ISP_IOC_GET_DRC_F_CURVE         _IOR(ISP_IOC_ISP, 88, drc_curve_t *)
#define ISP_IOC_SET_DRC_F_CURVE         _IOW(ISP_IOC_ISP, 88, drc_curve_t *)
#define ISP_IOC_GET_DRC_A_CURVE         _IOR(ISP_IOC_ISP, 89, drc_curve_t *)
#define ISP_IOC_SET_DRC_A_CURVE         _IOW(ISP_IOC_ISP, 89, drc_curve_t *)
#define ISP_IOC_GET_ISP_GAIN            _IOR(ISP_IOC_ISP, 90, int)
#define ISP_IOC_SET_ISP_GAIN            _IOW(ISP_IOC_ISP, 90, int)
#define ISP_IOC_GET_TMNR_EDG_TH         _IOR(ISP_IOC_ISP, 91, int)
#define ISP_IOC_SET_TMNR_EDG_TH         _IOW(ISP_IOC_ISP, 91, int)
#define ISP_IOC_GET_IA_HUE              _IOR(ISP_IOC_ISP, 92, ia_hue_t *)
#define ISP_IOC_SET_IA_HUE              _IOW(ISP_IOC_ISP, 92, ia_hue_t *)
#define ISP_IOC_GET_IA_SATURATION       _IOR(ISP_IOC_ISP, 93, ia_sat_t *)
#define ISP_IOC_SET_IA_SATURATION       _IOW(ISP_IOC_ISP, 93, ia_sat_t *)
#define ISP_IOC_GET_SKIN_CC             _IOR(ISP_IOC_ISP, 94, sk_reg_t *)
#define ISP_IOC_SET_SKIN_CC             _IOW(ISP_IOC_ISP, 94, sk_reg_t *)
#define ISP_IOC_GET_HBNR                _IOR(ISP_IOC_ISP, 100, hbnr_reg_t *)
#define ISP_IOC_SET_HBNR                _IOW(ISP_IOC_ISP, 100, hbnr_reg_t *)
#define ISP_IOC_GET_HBNR_LUT            _IOR(ISP_IOC_ISP, 101, hbnr_lut_t *)
#define ISP_IOC_SET_HBNR_LUT            _IOW(ISP_IOC_ISP, 101, hbnr_lut_t *)
#define ISP_IOC_GET_HBNR_ANTIJAGGY      _IOR(ISP_IOC_ISP, 102, hbnr_aj_t *)
#define ISP_IOC_SET_HBNR_ANTIJAGGY      _IOW(ISP_IOC_ISP, 102, hbnr_aj_t *)
#define ISP_IOC_GET_SP_STRENGTH         _IOR(ISP_IOC_ISP, 103, int)
#define ISP_IOC_SET_SP_STRENGTH         _IOW(ISP_IOC_ISP, 103, int)
#define ISP_IOC_GET_SP_CLIP             _IOR(ISP_IOC_ISP, 104, sp_clip_t *)
#define ISP_IOC_SET_SP_CLIP             _IOW(ISP_IOC_ISP, 104, sp_clip_t *)
#define ISP_IOC_GET_SP_F_CURVE          _IOR(ISP_IOC_ISP, 105, sp_curve_t *)
#define ISP_IOC_SET_SP_F_CURVE          _IOW(ISP_IOC_ISP, 105, sp_curve_t *)
#define ISP_IOC_GET_CS                  _IOR(ISP_IOC_ISP, 106, cs_reg_t *)
#define ISP_IOC_SET_CS                  _IOW(ISP_IOC_ISP, 106, cs_reg_t *)
#define ISP_IOC_GET_SP_MASK             _IOR(ISP_IOC_ISP, 107, int)
#define ISP_IOC_SET_SP_MASK             _IOW(ISP_IOC_ISP, 107, int)
#define ISP_IOC_GET_MRNR_OPERATOR       _IOR(ISP_IOC_ISP, 108, mrnr_op_t *)
#define ISP_IOC_SET_MRNR_OPERATOR       _IOW(ISP_IOC_ISP, 108, mrnr_op_t *)

// user perference
#define ISP_IOC_SET_GAMMA_CURVE         _IOW(ISP_IOC_ISP, 128, gamma_curve_t *)

//=============================================================================
// GM sensor IOCTL command definition
//=============================================================================
#define ISP_IOC_SENSOR 's'
#define ISP_IOC_READ_SENSOR_REG         _IOWR(ISP_IOC_SENSOR, 1, struct reg_info)
#define ISP_IOC_WRITE_SENSOR_REG        _IOWR(ISP_IOC_SENSOR, 2, struct reg_info)
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
#define ISP_IOC_GET_SENSOR_MIN_GAIN     _IOR(ISP_IOC_SENSOR, 10, int)
#define ISP_IOC_GET_SENSOR_MAX_GAIN     _IOR(ISP_IOC_SENSOR, 11, int)
#define ISP_IOC_GET_SENSOR_SIZE         _IOR(ISP_IOC_SENSOR, 12, u32)
#define ISP_IOC_SET_SENSOR_SIZE         _IOW(ISP_IOC_SENSOR, 12, u32)
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
#define AE_IOC_GET_CONVERGE_TH          _IOR(ISP_IOC_AE, 3, u32)
#define AE_IOC_SET_CONVERGE_TH          _IOW(ISP_IOC_AE, 3, u32)
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
#define AE_IOC_GET_CONST_EXP            _IOR(ISP_IOC_AE, 15, int)
#define AE_IOC_SET_CONST_EXP            _IOW(ISP_IOC_AE, 15, int)
#define AE_IOC_GET_CONST_GAIN           _IOR(ISP_IOC_AE, 16, int)
#define AE_IOC_SET_CONST_GAIN           _IOW(ISP_IOC_AE, 16, int)
#define AE_IOC_GET_IRIS_ENABLE          _IOR(ISP_IOC_AE, 17, int)
#define AE_IOC_SET_IRIS_ENABLE          _IOW(ISP_IOC_AE, 17, int)
#define AE_IOC_GET_IRIS_PROBE           _IOR(ISP_IOC_AE, 18, int)
#define AE_IOC_SET_IRIS_PROBE           _IOW(ISP_IOC_AE, 18, int)
#define AE_IOC_GET_IRIS_PID             _IOR(ISP_IOC_AE, 19, ae_iris_pid_t *)
#define AE_IOC_SET_IRIS_PID             _IOW(ISP_IOC_AE, 19, ae_iris_pid_t *)
#define AE_IOC_GET_IRIS_MIN_EXP         _IOR(ISP_IOC_AE, 20, int)
#define AE_IOC_SET_IRIS_MIN_EXP         _IOW(ISP_IOC_AE, 20, int)
#define AE_IOC_GET_IRIS_BAL_RATIO       _IOR(ISP_IOC_AE, 21, int)
#define AE_IOC_SET_IRIS_BAL_RATIO       _IOW(ISP_IOC_AE, 21, int)
#define AE_IOC_GET_EV_MODE              _IOR(ISP_IOC_AE, 22, int)
#define AE_IOC_SET_EV_MODE              _IOW(ISP_IOC_AE, 22, int)
#define AE_IOC_GET_WIN_WEIGHT           _IOR(ISP_IOC_AE, 24, int)
#define AE_IOC_SET_WIN_WEIGHT           _IOW(ISP_IOC_AE, 24, int)
#define AE_IOC_GET_AUTO_DRC             _IOR(ISP_IOC_AE, 25, int)
#define AE_IOC_SET_AUTO_DRC             _IOW(ISP_IOC_AE, 25, int)
#define AE_IOC_GET_AUTO_CONTRAST        _IOR(ISP_IOC_AE, 26, int)
#define AE_IOC_SET_AUTO_CONTRAST        _IOW(ISP_IOC_AE, 26, int)
#define AE_IOC_GET_HIGH_LIGHT_SUPP      _IOR(ISP_IOC_AE, 27, int)
#define AE_IOC_SET_HIGH_LIGHT_SUPP      _IOW(ISP_IOC_AE, 27, int)
#define AE_IOC_SET_TOTAL_GAIN           _IOW(ISP_IOC_AE, 28, int)
#define AE_IOC_GET_MAX_ISP_GAIN         _IOR(ISP_IOC_AE, 29, int)
#define AE_IOC_SET_MAX_ISP_GAIN         _IOW(ISP_IOC_AE, 29, int)
#define AE_IOC_GET_EV_VALUE             _IOR(ISP_IOC_AE, 30, u32)
#define AE_IOC_GET_VWP_RATIO_TH         _IOR(ISP_IOC_AE, 31, int)
#define AE_IOC_SET_VWP_RATIO_TH         _IOW(ISP_IOC_AE, 31, int)

//=============================================================================
// GM AWB IOCTL command definition
//=============================================================================
#define ISP_IOC_AWB 'w'
#define AWB_IOC_GET_ENABLE              _IOR(ISP_IOC_AWB, 0, int)
#define AWB_IOC_SET_ENABLE              _IOW(ISP_IOC_AWB, 0, int)
#define AWB_IOC_GET_TH                  _IOR(ISP_IOC_AWB, 1, s32*)
#define AWB_IOC_SET_TH                  _IOW(ISP_IOC_AWB, 1, s32*)
#define AWB_IOC_GET_RG_RATIO            _IOR(ISP_IOC_AWB, 2, u32)
#define AWB_IOC_GET_BG_RATIO            _IOR(ISP_IOC_AWB, 3, u32)
#define AWB_IOC_GET_RB_RATIO            _IOR(ISP_IOC_AWB, 4, u32)
#define AWB_IOC_GET_CONVERGE_TH         _IOR(ISP_IOC_AWB, 5, u32)
#define AWB_IOC_SET_CONVERGE_TH         _IOW(ISP_IOC_AWB, 5, u32)
#define AWB_IOC_GET_SPEED               _IOR(ISP_IOC_AWB, 6, int)
#define AWB_IOC_SET_SPEED               _IOW(ISP_IOC_AWB, 6, int)
#define AWB_IOC_GET_FREEZE_SEG          _IOR(ISP_IOC_AWB, 7, u32)
#define AWB_IOC_SET_FREEZE_SEG          _IOW(ISP_IOC_AWB, 7, u32)
#define AWB_IOC_GET_RG_TARGET           _IOR(ISP_IOC_AWB, 8, struct awb_target)
#define AWB_IOC_SET_RG_TARGET           _IOW(ISP_IOC_AWB, 8, struct awb_target)
#define AWB_IOC_GET_BG_TARGET           _IOR(ISP_IOC_AWB, 9, struct awb_target)
#define AWB_IOC_SET_BG_TARGET           _IOW(ISP_IOC_AWB, 9, struct awb_target)
#define AWB_IOC_GET_SCENE_MODE          _IOR(ISP_IOC_AWB, 10, u32)
#define AWB_IOC_SET_SCENE_MODE          _IOW(ISP_IOC_AWB, 10, u32)
#define AWB_IOC_GET_STA_MODE            _IOR(ISP_IOC_AWB, 11, int)
#define AWB_IOC_SET_STA_MODE            _IOW(ISP_IOC_AWB, 11, int)

#define AWB_IOC_SET_OUTDOOR_TH5         _IOW(ISP_IOC_AWB, 12, s32)
#define AWB_IOC_SET_OUTDOOR_EV_TH       _IOW(ISP_IOC_AWB, 13, u32)
#define AWB_IOC_SET_OUTDOOR_TH2         _IOW(ISP_IOC_AWB, 14, s32)

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

#endif /* __IOCTL_ISP320_H__ */
