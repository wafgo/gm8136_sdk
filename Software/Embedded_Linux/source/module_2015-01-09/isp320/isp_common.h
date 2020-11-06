/**
 * @file isp_common.h
 * ISP common struct and flag definition
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.29 $
 * $Date: 2014/09/26 05:20:35 $
 *
 * ChangeLog:
 *  $Log: isp_common.h,v $
 *  Revision 1.29  2014/09/26 05:20:35  ricktsao
 *  use kthread instead of workqueue
 *
 *  Revision 1.28  2014/09/10 04:25:47  ricktsao
 *  no message
 *
 *  Revision 1.27  2014/08/01 11:30:32  ricktsao
 *  add image pipe control IOCTLs
 *
 *  Revision 1.26  2014/07/24 11:53:56  ricktsao
 *  no message
 *
 *  Revision 1.25  2014/07/24 08:29:25  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.24  2014/07/21 09:38:08  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.23  2014/07/21 02:11:55  ricktsao
 *  add auto gamma adjustment
 *
 *  Revision 1.22  2014/07/04 10:13:14  ricktsao
 *  support auto adjust black offset
 *
 *  Revision 1.21  2014/05/21 08:31:12  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.20  2014/05/08 10:58:50  jtwang
 *  add AWB scene mode
 *
 *  Revision 1.19  2014/05/06 06:19:58  ricktsao
 *  add ioctl for get and set cc matrix and cv matrix
 *
 *  Revision 1.18  2014/04/15 05:03:02  ricktsao
 *  no message
 *
 *  Revision 1.17  2014/04/14 11:38:29  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.16  2014/04/01 08:23:33  ricktsao
 *  no message
 *
 *  Revision 1.15  2014/03/26 06:24:06  ricktsao
 *  1. divide 3A algorithms from core driver.
 *
 *  Revision 1.14  2014/03/06 07:37:25  allenhsu
 *  add dpc_th2 into adjust_nr
 *
 *  Revision 1.13  2014/03/06 06:39:42  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.12  2014/03/06 03:38:56  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.11  2013/12/11 05:14:39  ricktsao
 *  no message
 *
 *  Revision 1.10  2013/12/10 06:51:50  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.9  2013/11/25 11:59:10  ricktsao
 *  no message
 *
 *  Revision 1.8  2013/11/14 07:58:18  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.7  2013/11/13 09:48:02  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.6  2013/11/08 08:33:07  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.5  2013/11/05 09:21:21  ricktsao
 *  no message
 *
 *  Revision 1.4  2013/10/23 06:41:30  ricktsao
 *  add isp_plat_extclk_set_freq() and isp_plat_extclk_onoff()
 *
 *  Revision 1.3  2013/10/21 10:27:48  ricktsao
 *  no message
 *
 *  Revision 1.2  2013/10/01 05:51:39  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.1  2013/09/26 02:06:48  ricktsao
 *  no message
 *
 */

#ifndef __ISP_COMMON_H__
#define __ISP_COMMON_H__

#include <linux/types.h>
#include <linux/slab.h>
#include "isp_dbg.h"
#include "bits.h"
#include "ioctl_isp320.h"

//=============================================================================
// ISP driver definition
//=============================================================================
#define USE_KTHREAD
#define USE_DMA_CMD
//#define DEBUG_DMA_CMD

#define ISP_BUF_DDR_IDX DDR_ID_SYSTEM
#define ISP_DMACMD_SZ   (0x1000)  // 4KB

//=============================================================================
// common macros
//=============================================================================
#define FP10(f)  ((u32)(f*(1<<10)+0.5))
#define MIN(x,y) ((x)<(y) ? (x) : (y))
#define MAX(x,y) ((x)>(y) ? (x) : (y))
#define ABS(x)   ((x)<0 ? (-1 * (x)):(x))
#define CLAMP(x,min,max)    (((x) > (max)) ? (max) : (((x) < (min)) ? (min) : (x)))
#define EXPAND_S32(v, s)    ((s32)((v) << (32-(s))) >> (32-(s)))
#define EXPAND_S16(v, s)    ((s16)((v) << (16-(s))) >> (16-(s)))
#define HAL_REG_RD(vbase, offset)       ioread32((vbase)+(offset))
#define HAL_REG_WR(vbase, offset, v)    iowrite32((v), (vbase)+(offset))
#define ISP_ALIGN(addr, shft)   (((addr)+(1UL << shft)-1)&(~((1UL << shft)-1)))

//=============================================================================
// extern variables
//=============================================================================
extern gamma_lut_t gm_gamma_16;
extern gamma_lut_t gm_gamma_18;
extern gamma_lut_t gm_gamma_20;
extern gamma_lut_t gm_gamma_22;
extern gamma_lut_t gm_def_gamma;
extern gamma_lut_t gm_bt709_gamma;
extern gamma_lut_t gm_srgb_gamma;

//=============================================================================
// struct & flag definition
//=============================================================================
// default user preference setting
#define DEF_BRIGHTNESS      128     // 0 ~ 255
#define DEF_CONTRAST        128     // 0 ~ 255
#define DEF_HUE             128     // 0 ~ 255
#define DEF_SATURATION      128     // 0 ~ 255
#define DEF_DENOISE         128     // 0 ~ 255
#define DEF_SHARPNESS       128     // 0 ~ 255
#define DEF_DRC_STRENGTH    128     // 0 ~ 255

#define DEF_TARGET_RG_RATIO     1024
#define DEF_TARGET_BG_RATIO     1024

#define DEF_R_GAIN  119
#define DEF_B_GAIN  262

#define DUMMY_ADDR          0x0

typedef struct dma_buf {
    void      *vaddr;
    dma_addr_t paddr;
    size_t     size;
} dma_buf_t;

// ISP funciton_0 flags
#define ISP_FUNC_DCPD       BIT0
#define ISP_FUNC_OB         BIT1
#define ISP_FUNC_OBAVG      BIT2
#define ISP_FUNC_LI         BIT3
#define ISP_FUNC_DPC        BIT4
#define ISP_FUNC_RDN        BIT5
#define ISP_FUNC_CTK        BIT6
#define ISP_FUNC_LSC_SYM    BIT7
#define ISP_FUNC_LSC_ASYM   BIT8
#define ISP_FUNC_VIGNET     BIT9
#define ISP_FUNC_RAN        BIT10
#define ISP_FUNC_HIST       BIT11
#define ISP_FUNC_AE         BIT12
#define ISP_FUNC_AWB        BIT13
#define ISP_FUNC_AF         BIT14
#define ISP_FUNC_DRC        BIT15
#define ISP_FUNC_GAMMA      BIT16
#define ISP_FUNC_CDN        BIT17
#define ISP_FUNC_CC         BIT18
#define ISP_FUNC_BRIGHT     BIT19
#define ISP_FUNC_CONTRAST   BIT20
#define ISP_FUNC_COLORIZE   BIT21
#define ISP_FUNC_HUESAT     BIT22
#define ISP_FUNC_UVOFFSET   BIT23
#define ISP_FUNC_NEGATIVE   BIT24
#define ISP_FUNC_POSTERIZE  BIT25
#define ISP_FUNC_SK         BIT26
#define ISP_FUNC_HBNR       BIT27
#define ISP_FUNC_HBAJ       BIT28
#define ISP_FUNC_SP         BIT29
#define ISP_FUNC_CS         BIT30
#define ISP_FUNC_ISP        BIT31

#define ISP_FUNC_ALL0       (ISP_FUNC_DCPD | ISP_FUNC_OB | ISP_FUNC_OBAVG | ISP_FUNC_LI \
                            | ISP_FUNC_DPC | ISP_FUNC_RDN | ISP_FUNC_CTK | ISP_FUNC_LSC_SYM \
                            | ISP_FUNC_LSC_ASYM | ISP_FUNC_VIGNET | ISP_FUNC_RAN | ISP_FUNC_HIST \
                            | ISP_FUNC_AE | ISP_FUNC_AWB | ISP_FUNC_AF | ISP_FUNC_DRC \
                            | ISP_FUNC_GAMMA | ISP_FUNC_CDN | ISP_FUNC_CC | ISP_FUNC_BRIGHT \
                            | ISP_FUNC_CONTRAST | ISP_FUNC_COLORIZE | ISP_FUNC_HUESAT \
                            | ISP_FUNC_UVOFFSET | ISP_FUNC_NEGATIVE | ISP_FUNC_POSTERIZE \
                            | ISP_FUNC_SK | ISP_FUNC_HBNR | ISP_FUNC_HBAJ | ISP_FUNC_SP \
                            | ISP_FUNC_CS | ISP_FUNC_ISP)

// ISP funciton_1 flags
#define ISP_FUNC_GG         BIT20
#define ISP_FUNC_CG         BIT21
#define ISP_FUNC_RY         BIT22
#define ISP_FUNC_ALL1       (ISP_FUNC_GG | ISP_FUNC_CG | ISP_FUNC_RY)
#define ISP_AE_BEFORE_GG    BIT6
#define ISP_AWB_SEG_MODE    BIT7

// ISP interrupt status_0 flags
#define ISP_INTR_AE         BIT0
#define ISP_INTR_AWB        BIT1
#define ISP_INTR_AF         BIT2
#define ISP_INTR_HIST       BIT3
#define ISP_INTR_VSTART     BIT16
#define ISP_INTR_VEND       BIT17
#define ISP_INTR_IDMA_DONE  BIT18   // DMA-in done
#define ISP_INTR_ODMA_DONE  BIT19   // DMA-out done
#define ISP_INTR_SDMA_DONE  BIT20   // AWB segment-mode DMA-out done
#define ISP_INTR_IDMA_ERR   BIT24   // DMA-in or register reload is not finished in blanking time
#define ISP_INTR_ODMA_ERR   BIT25   // DMA-out is not finished in blanking time
#define ISP_INTR_SBUF_OVF   BIT27   // AWB segment-mode internal buffer overrun
#define ISP_INTR_ALL0       (ISP_INTR_AE | ISP_INTR_AWB | ISP_INTR_AF | ISP_INTR_HIST | ISP_INTR_VSTART \
                            | ISP_INTR_VEND | ISP_INTR_IDMA_DONE | ISP_INTR_ODMA_DONE | ISP_INTR_SDMA_DONE \
                            | ISP_INTR_IDMA_ERR | ISP_INTR_ODMA_ERR | ISP_INTR_SBUF_OVF)

// ISP interrupt status_1 flags
#define ISP_INTR_IBUFOVF    BIT8
#define ISP_INTR_FRMDONE    BIT31
#define ISP_INTR_ALL1       (ISP_INTR_IBUFOVF | ISP_INTR_FRMDONE)

#define RAW_8               (0)
#define RAW_10              BIT0
#define RAW_12              BIT1
#define RAW_14              BIT2
#define WDR_16              (1<<4)
#define WDR_20              (2<<4)
#define WDR_MASK            0xF0

enum POLARITY {
    ACTIVE_HIGH = 0,
    ACTIVE_LOW,
};

enum BAYER_TYPE {
    BAYER_RG = 0,
    BAYER_GR,
    BAYER_GB,
    BAYER_BG,
};

enum SRC_FMT {
    RAW8  = RAW_8,
    RAW10 = RAW_10,
    RAW12 = RAW_12,
    RAW14 = RAW_14,
    RAW12_WDR16 = RAW_12 + WDR_16,
    RAW14_WDR16 = RAW_14 + WDR_16,
    RAW12_WDR20 = RAW_12 + WDR_20,
    RAW14_WDR20 = RAW_14 + WDR_20,
};

enum OUT_FMT {
    YUYV422 = 0,
    UYVY422 = 1,
    YUV420P = 4,
    BAYER_RAW = 7,
};

enum RAW_CH {
    RAW_R,
    RAW_Gr,
    RAW_Cb,
    RAW_B,
};

enum RGB_CH {
    RGB_R = 0,
    RGB_G,
    RGB_B,
    RGB_ALL,
};

enum CGAIN_FMT {
    UFIX_2_7 = 0,
    UFIX_3_6,
};

enum CGAIN_TYPE {
    R_GAIN = 0,
    G_GAIN,
    B_GAIN,
};

enum HB_FILTER_TYPE {
    BYPASS = 0,
    FILTER_3x3,
    FILTER_5x7,
    FILTER_5x15,
};

//=============================================================================
// DMA command
//=============================================================================
typedef struct dma_cmd_info {
    u32 *table;
    u32 table_pa;
    int table_size;
    int header_entry;
    int cur_entry;
} dma_cmd_info_t;

//=============================================================================
// ISP config
//=============================================================================
extern const char *bayer_str[4];

typedef struct dc_param {
    s32 knee_x[4];
    s32 knee_y[4];
    s32 slope[4];
    u8 bit_width;
    u8 enable;
} dc_param_t;

typedef struct co_param {
    // order: R, Gr, Gb, B
    s16 ob[4];
    u8 enable;
} co_param_t;

typedef struct li_param {
    u8 rlut[16];
    u8 glut[16];
    u8 blut[16];
    u8 enable;
} li_param_t;

typedef struct dpc_param {
    u8 dpc_mode;
    u8 rvc_mode;
    u8 dpcn;
    u16 th[3];
    u8 enable;
} dpc_param_t;

typedef struct ctk_param {
    u32 th[4];
    u8  w[4];
    u8  gbgr;
    u8 enable;
} ctk_param_t;

typedef struct lsc_param {
    u8 segrds;
    // order: R, Gr, Gb, B
    win_pos_t center[4];
    s16 maxtan[4][8];
    u16 radparam[4][256];
    //---------------------
    u8 adjustn;
    s16 veparam[5];
    u8 enable;
} lsc_param_t;

typedef struct drc_param {
    u8  bghno;
    u8  bgvno;
    u8  bghskip;
    u8  bgvskip;
    u8  bgw2;
    u8  bgh2;
    u16 bgho;
    u16 bgvo;
    u32 intensity_varth;
    u16 PG_index[16];
    u32 PG_value[32];
    u16 F_index[16];
    u32 F_value[32];
    u16 A_index[16];
    u32 A_value[32];
    u16 qcoef1;
    u16 qcoef2;
    u16 qcoef3;
    u8 int_blend;
    u8 local_max;
    u16 strength;
} drc_param_t;

typedef struct gm_param {
    u16 index[16];
    u8  value[32];
    u8  enable;
} gm_param_t;

typedef struct ci_param {
    u8 T1;
    u8 T2;
    u8 bi_mode;
    u8 enable;
} ci_param_t;

typedef struct cc_param {
    s16 matrix[3][9];
    u8 enable;
} cc_param_t;

typedef struct cv_param {
    s16 matrix[3][9];
    u8 enable;
} cv_param_t;

typedef struct ia_param {
    int contrast_mode;
    u8 colorize_u;
    u8 colorize_v;
    s8 offset_u;
    s8 offset_v;
    s8 hue[8];
    u8 saturation[8];
    u8 poster_th[4];
    u8 poster_y[4];
    u8 enable;
} ia_param_t;

typedef struct sk_param {
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
    u32 pix_count;
    u8 enable;
} sk_param_t;

typedef struct hb_param {
    u16 edge_th;
    u8  luma_filter;
    u8  chroma_filter;
    u8  lutl[64];
    u8  lutc[64];
    u8 enable;
} hb_param_t;

typedef struct aj_param {
    u16 edge_th;
    u8 enable;
} aj_param_t;

typedef struct sp_param {
    int mask;
    u8 strength;
    u8 th;
    u8 clip0;
    u8 clip1;
    u8 reduce;
    u8 F[128];
    u8 enable;
} sp_param_t;

typedef struct cs_param {
    u8  in[4];
    u16 out[3];
    u16 slope[4];
    u16 cb_th;
    u16 cr_th;
    u8 enable;
} cs_param_t;

typedef struct ae_param {
    int f_number;
    int const_k;
    int target_y;
    int delay;
    int speed;
    int converge_th;
    int auto_drc;
    int auto_contrast;
    u32 max_isp_gain;
} ae_param_t;

typedef struct awb_param {
    s32 th[13];
    u32 rb_ratio[3];
    u32 target_rg_ratio[3];
    u32 target_bg_ratio[3];
    u32 r_gain[6];
    u32 b_gain[6];
    s32 outdoor_th2;
    s32 outdoor_th5;
    u32 outdoor_ev_th;
} awb_param_t;

typedef struct af_param {
    u8 filter_k[4][6];
    // threshold
    s32 thres_rough;
    s32 thres_fine;
    s32 thres_final;
    s32 thres_wobble;
    s32 thres_restart;

    // move steps
    int steps_rough;
    int steps_fine;
    int steps_final;
    int steps_wobble;
    int max_search;
} af_param_t;

typedef struct _NOISE_LVL
{
    unsigned int Y[32]; //{ layer1{intensity level 1~8}, layer2{intensity level 1~8}, layer3{intensity level 1~8}, layer4{intensity level 1~8} }
    unsigned int Cb[4]; //{ layer1, layer2, layer3, layer4 }
    unsigned int Cr[4]; //{ layer1, layer2, layer3, layer4 }
}noise_lvl_t;

typedef struct _mrnr_cfg {
    int G1_Y_Freq[4];
    int G1_Y_Tone[4];
    int G1_C[4];
    int G2_Y;
    int G2_C;
    int Y_L_Nr_Str[4];
    int C_L_Nr_Str[4];
    noise_lvl_t noise_lvl[9];
} mrnr_cfg_t;

typedef struct _tmnr_param{
    unsigned char vlut[21];
    int Y_Var[9];
    int C_Var[9];
    int edge_th[9];
}tmnr_param_t;

typedef struct iris_param {
    int iris_kp;
    int iris_kd;
    int iris_ki;
    int iris_balance_ratio;
} iris_param_t;

typedef struct cfg_info {
    u32         update_flag;
    adj_param_t adj_param;
    dc_param_t  dc_param;
    co_param_t  co_param;
    li_param_t  li_param;
    dpc_param_t dpc_param;
    ctk_param_t ctk_param;
    lsc_param_t lsc_param;
    drc_param_t drc_param;
    drc_param_t drc_wdr_param;
    gm_param_t  gm_param;
    gm_param_t  gm_wdr_param;
    ci_param_t  ci_param;
    cc_param_t  cc_param;
    cv_param_t  cv_param;
    ia_param_t  ia_param;
    sk_param_t  sk_param;
    hb_param_t  hb_param;
    aj_param_t  aj_param;
    sp_param_t  sp_param;
    cs_param_t  cs_param;
    ae_param_t  ae_param;
    awb_param_t awb_param;
    af_param_t  af_param;
    mrnr_cfg_t mrnr_param;
    tmnr_param_t tmnr_param;
    iris_param_t iris_param;
} cfg_info_t;

// update_flag definition
#define CFG_ADJ     BIT0
#define CFG_DC      BIT1
#define CFG_CO      BIT2
#define CFG_LI      BIT3
#define CFG_DPC     BIT4
#define CFG_CTK     BIT5
#define CFG_LSC     BIT6
#define CFG_LSC_SYM BIT7
#define CFG_DRC_LI  BIT8
#define CFG_DRC_WDR BIT9
#define CFG_GM      BIT10
#define CFG_GM_WDR  BIT11
#define CFG_CI      BIT12
#define CFG_CC      BIT13
#define CFG_CV      BIT14
#define CFG_IA      BIT15
#define CFG_SK      BIT16
#define CFG_HB      BIT17
#define CFG_SP      BIT18
#define CFG_CS      BIT19
#define CFG_AE      BIT20
#define CFG_AWB     BIT21
#define CFG_AF      BIT22
#define CFG_AJ      BIT23
#define CFG_MRNR    BIT24
#define CFG_TMNR    BIT25
#define CFG_IRIS    BIT26
#define CFG_USER    BIT31

#endif /* __ISP_COMMON_H__ */
