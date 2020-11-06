/**
 * @file isp_common.h
 * ISP common struct and flag definition
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 */

#ifndef __ISP_COMMON_H__
#define __ISP_COMMON_H__

#include <linux/types.h>
#include <linux/slab.h>
#include "isp_dbg.h"
#include "bits.h"
#include "ioctl_isp328.h"

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
extern gamma_lut_t gamma_curve_list[NUM_OF_GAMMA_TYPE];

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
#define ISP_FUNC0_DCPD      BIT0
#define ISP_FUNC0_OB        BIT1
#define ISP_FUNC0_OBAVG     BIT2
#define ISP_FUNC0_DPC       BIT4
#define ISP_FUNC0_RDN       BIT5
#define ISP_FUNC0_CTK       BIT6
#define ISP_FUNC0_LSC       BIT8
#define ISP_FUNC0_HIST      BIT11
#define ISP_FUNC0_AE        BIT12
#define ISP_FUNC0_AWB       BIT13
#define ISP_FUNC0_AF        BIT14
#define ISP_FUNC0_DRC       BIT15
#define ISP_FUNC0_GAMMA     BIT16
#define ISP_FUNC0_CDN       BIT17
#define ISP_FUNC0_BRIGHT    BIT19
#define ISP_FUNC0_CONTRAST  BIT20
#define ISP_FUNC0_COLORIZE  BIT21
#define ISP_FUNC0_HUESAT    BIT22
#define ISP_FUNC0_UVOFFSET  BIT23
#define ISP_FUNC0_NEGATIVE  BIT24
#define ISP_FUNC0_POSTERIZE BIT25
#define ISP_FUNC0_CS        BIT30
#define ISP_FUNC0_ISP       BIT31

#define ISP_FUNC0_ALL       (ISP_FUNC0_DCPD | ISP_FUNC0_OB | ISP_FUNC0_OBAVG \
                            | ISP_FUNC0_DPC | ISP_FUNC0_RDN | ISP_FUNC0_CTK | ISP_FUNC0_LSC \
                            | ISP_FUNC0_HIST | ISP_FUNC0_AE | ISP_FUNC0_AWB | ISP_FUNC0_AF  \
                            | ISP_FUNC0_DRC | ISP_FUNC0_GAMMA | ISP_FUNC0_CDN \
                            | ISP_FUNC0_BRIGHT | ISP_FUNC0_CONTRAST | ISP_FUNC0_COLORIZE | ISP_FUNC0_HUESAT \
                            | ISP_FUNC0_UVOFFSET | ISP_FUNC0_NEGATIVE | ISP_FUNC0_POSTERIZE \
                            | ISP_FUNC0_CS | ISP_FUNC0_ISP)

#define ISP_FUNC0_DEF       (ISP_FUNC0_OB | ISP_FUNC0_DPC | ISP_FUNC0_RDN | ISP_FUNC0_CTK \
                            | ISP_FUNC0_HIST | ISP_FUNC0_AE | ISP_FUNC0_AWB | ISP_FUNC0_AF \
                            | ISP_FUNC0_GAMMA | ISP_FUNC0_CDN | ISP_FUNC0_BRIGHT | ISP_FUNC0_CONTRAST \
                            | ISP_FUNC0_HUESAT)

// ISP funciton_1 flags
#define ISP_FUNC1_CE        BIT18
#define ISP_FUNC1_CE_CURVE  BIT19
#define ISP_FUNC1_GG        BIT20
#define ISP_FUNC1_CG        BIT21
#define ISP_FUNC1_CV        BIT22
#define ISP_FUNC1_ALL       (ISP_FUNC1_CE | ISP_FUNC1_CE_CURVE | ISP_FUNC1_GG | ISP_FUNC1_CG | ISP_FUNC1_CV)
#define ISP_FUNC1_DEF       (ISP_FUNC1_CE | ISP_FUNC1_GG | ISP_FUNC1_CG | ISP_FUNC1_CV)

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

enum DR_MODE {
    DR_LINEAR = 0,
    DR_WDR,
};

typedef struct ce_reg {
    int bg_width[2];
    int bg_height[2];
    int bg_recip[2];
    int blend;
    int th;
    int strength;
} ce_reg_t;

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
typedef struct drc_param {
    u8 bghno;
    u8 bgvno;
    drc_curve_t pg_curve;
    drc_curve_t gm_curve;
    drc_curve_t wgt_curve;
    u16 max_gain_ratio;
    u16 intensity_varth;
    u8 int_blend;
    u8 local_max;
    u16 strength;
} drc_param_t;

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
} ia_param_t;

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
    int adjust_cc;
    int use_cc_matrix;
    awb_roi_t awb_roi[AWB_ROI_NUM];
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

typedef struct iris_param {
    int iris_kp;
    int iris_kd;
    int iris_ki;
    int iris_balance_ratio;
} iris_param_t;

typedef struct noise_lvl {
    unsigned int Y[32]; //{ layer1{intensity level 1~8}, layer2{intensity level 1~8}, layer3{intensity level 1~8}, layer4{intensity level 1~8} }
    unsigned int Cb[4]; //{ layer1, layer2, layer3, layer4 }
    unsigned int Cr[4]; //{ layer1, layer2, layer3, layer4 }
} noise_lvl_t;

typedef struct mrnr_cfg {
    int G1_Y_Freq[4];
    int G1_Y_Tone[4];
    int G1_C[4];
    int G2_Y;
    int G2_C;
    int Y_L_Nr_Str[4];
    int C_L_Nr_Str[4];
    noise_lvl_t noise_lvl[ADJ_SEGMENTS];
} mrnr_cfg_t;

typedef struct tmnr_cfg {
    int Y_var[ADJ_SEGMENTS];
    int C_var[ADJ_SEGMENTS];
    int trade_thres[ADJ_SEGMENTS];
    int suppress_strength[ADJ_SEGMENTS];
    int motion_th;
    int K;
    int NF;
    int var_offset;
    int auto_recover;
    int rapid_recover;
} tmnr_cfg_t;

typedef struct cfg_info {
    adj_param_t     adj_param;
    dcpd_reg_t      dcpd_param;
    blc_reg_t       blc_param;
    lsc_reg_t       lsc_param;
    drc_param_t     drc_param;
    drc_param_t     drc_wdr_param;
    gamma_lut_t     gm_param;
    gamma_lut_t     gm_wdr_param;
    ce_cfg_t        ce_param;
    ci_reg_t        ci_param;
    adj_matrix_t    cc_param;
    adj_matrix_t    cv_param;
    ia_param_t      ia_param;
    cs_reg_t        cs_param;
    mrnr_cfg_t      mrnr_cfg;
    tmnr_cfg_t      tmnr_cfg;
    sp_param_t      sp_param;
    ae_param_t      ae_param;
    awb_param_t     awb_param;
    af_param_t      af_param;
    iris_param_t    iris_param;
} cfg_info_t;

#endif /* __ISP_COMMON_H__ */
