#ifndef __FSCALER300_H__
#define __FSCALER300_H__

#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include "fscaler300_dbg.h"
#include "fscaler300_fcs.h"
#include "fscaler300_dn.h"
#include "fscaler300_sharpness.h"
#include "fscaler300_smooth.h"
#include "fscaler300_dma.h"
#include "fscaler300_mem.h"
#include "scl_dma/scl_dma.h"
#include "scaler_trans.h"
//#define SPLIT_SUPPORT
#define VERSION     141119
// platform info
//#define FSCALER300_MAX_NUM      2       // max scaler number
#define MAX_CHAN_NUM            35     // max channel number
#define MAX_VCH_NUM             128      // max virtual channel number 0~32
#define CH_BITMASK_NUM          4
#define PIP_COUNT               15
#define TEMP_WIDTH              1280
#define TEMP_HEIGHT             720
// channel info
#define SD_MAX      4
#define TC_MAX      4
#define DMA_MAX     4
#define MASK_MAX    8
#define OSD_MAX     8

#define CPUCOMM_SUPPORT     0       // for dual 8210 cpucomm method

#define USE_KTHREAD
#define TEST_WQ
//#define USE_WQ

#ifdef SPLIT_SUPPORT
#define BLK_MAX     7
#else
    #define BLK_MAX     PLATFORM_BLK_MAX
#endif

#define GM8210                          PLATFORM_GM8210
#define GM8287                          PLATFORM_GM8287
#define GM8139                          PLATFORM_GM8139

#define ALIGN_4(x)                      (((x)>>2)<<2)
#define ALIGN_2(x)                      (((x)>>1)<<1)
#define ALIGN_4_UP(x)                   (((x+3)>>2)<<2)
#define ALIGN_2_UP(x)                   (((x+1)>>1)<<1)
#define SCALER300_ENGINE_NUM            PLATFORM_ENGINE_NUM
#define SCALER300_LLI_MEMORY            PLATFORM_LLI_MEMORY
#define SCALER300_LLI_MAX_CMD           PLATFORM_LLI_MAX_CMD
#define SCALER300_LLI_MAX_BLK           PLATFORM_LLI_MAX_BLK
#define SCALER300_PARAM_MEMORY_LEN      PLATFORM_PARAM_MEMORY_LEN
#define SCALER300_TOTAL_RESOLUTION      PLATFORM_TOTAL_RESOLUTION
#define SCALER300_MAX_RESOLUTION        PLATFORM_MAX_RESOLUTION
#define SCALER300_DUMMY_BASE            PLATFORM_DUMMY_BASE
#define SCALER300_PALETTE_MAX           PLATFORM_PALETTE_MAX
#define SCALER300_MAX_NODES_PER_CHAIN   PLATFORM_MAX_NODES_PER_CHAIN
#define SCALER300_MAX_PATH_PER_CHAIN    PLATFORM_MAX_PATH_PER_CHAIN
#define SCALER300_SRAM_BLK              PLATFORM_SRAM_BLK
#define SCALER300_MAX_VJOBS_PER_CHAIN   30


#define SCALER300_LLI_BLK_OFFSET        256     //32*8          --> 32 commands
#define SCALER300_LINK_LIST_MEM_BASE    0x40000
#define SCALER300_REG_MEMORY_START      0x5000
#define SCALER300_REG_MEMORY_END        0x600
#define SCALER300_OSD_FONT_CHAR_MAX     256
#define SCALER300_OSD_MEMORY_SIZE       0x9000

#define SCALER300_OSD_FONT_COL_SIZE     12                                          ///< 4 bits per one column
#define SCALER300_OSD_FONT_ROW_SIZE     18
#define SCALER300_OSD_CHAR_OFFSET(idx)  ((SCALER300_OSD_FONT_ROW_SIZE*(idx)*64)>>3)   ///< one row is 64 bit
#define SCALER300_OSD_DISPLAY_MAX       8192

#define MAX_CHAIN_JOB                   PLATFORM_MAX_CHAIN_JOB
#define MAX_CHAIN_LL_BLK                PLATFORM_MAX_CHAIN_LL_BLK
#define SCL_LLI_TIMEOUT     2000    ///< 2 sec

#define ADD_BEFORE      1
#define ADD_AFTER       2

#define SCALER_PROC_NAME "scaler300"

typedef struct fscaler300_fcs {
    u8          enable;             ///< false color suppression enable
    u16         lv0_th;             ///< threshold for Y bandwidth + C bandwidth
    u8          lv1_th;             ///< threshold for first element of Y bandwidth
    u8          lv2_th;             ///< threshold for Y bandwidth
    u8          lv3_th;             ///< threshold for C bandwidth
    u8          lv4_th;             ///< threshold for C level
    u16         grey_th;            ///< threshold for local grey area decision
} fscaler300_fcs_t;

typedef struct fscaler300_dn {
    u8          enable;             ///< de-noise enable
    u8          geomatric_str;      ///< 1D DN strength according to distance, 0: means bypass
    u8          similarity_str;     ///< 1D DN strength according to differance, 0: means bypass
    u8          adp;                ///< enable adaptive denoise
    u16         adp_step;           ///< denoise adaptive step size
} fscaler300_dn_t;

typedef struct fscaler300_smooth {
    u8          hsmo_str;           ///< scaler presmooth horizontal strength, 0: means bypass
    u8          vsmo_str;           ///< scaler presmooth vertical strength, 0: means bypass
} fscaler300_smooth_t;

typedef struct fscaler300_sharpness {
    u8          adp;                ///< enable adaptive sharpness
    u8          radius;             ///< sharpness radius, 0: means bypass
    u8          amount;             ///< sharpness amount
    u8          threshold;          ///< sharpness dn level
    u8          adp_start;          ///< sharpness adaptive start strength
    u8          adp_step;           ///< sharpness adaptive step size
} fscaler300_sharpness_t;

/*************************************************************************************
 * Interrupt Mask Configuration Structure
 *************************************************************************************/
typedef struct fscaler300_int_mask {
    u8          ll_done;
    u8          sd_fail;
    u8          dma_wc_done;
    u8          dma_rc_done;
    u8          frame_end;

    /* Global */
    u8          dma_ovf;
    u8          dma_job_ovf;
    u8          dma_dis_status;
    u8          dma_wc_respfail;
    u8          dma_rc_respfail;
    u8          sd_job_ovf;
    u8          dma_wc_pfx_err;
} fscaler300_int_mask_t;

typedef struct fscaler300_rgb2yuv_matrix {
    u8          rgb2yuv_a00;
    u8          rgb2yuv_a01;
    u8          rgb2yuv_a02;
    u8          rgb2yuv_a10;
    u8          rgb2yuv_a11;
    u8          rgb2yuv_a12;
    u8          rgb2yuv_a20;
    u8          rgb2yuv_a21;
    u8          rgb2yuv_a22;
} fscaler300_rgb2yuv_matrix_t;

typedef struct fscaler300_yuv2rgb_matrix {
    u8          yuv2rgb_a00;
    u8          yuv2rgb_a01;
    u8          yuv2rgb_a02;
    u8          yuv2rgb_a10;
    u8          yuv2rgb_a11;
    u8          yuv2rgb_a12;
    u8          yuv2rgb_a20;
    u8          yuv2rgb_a21;
    u8          yuv2rgb_a22;
} fscaler300_yuv2rgb_matrix_t;
#if 0
typedef struct fscaler300_subchannel {
    u8          res0_border_en;                   ///< image border effect enable
    u8          res1_border_en;                   ///< image border effect enable
    u8          res2_border_en;                   ///< image border effect enable
    u8          res3_border_en;                   ///< image border effect enable
} fscaler300_subchannel_t;
#endif
typedef struct fscaler300_dma_ctrl {
    u16         dma_lnbst_itv;
    u16         dma_wc_wait_value;
    u16         dma_rc_wait_value;
} fscaler300_dma_ctrl_t;

typedef struct fscaler300_osd_font_data {
    u8          ram_map[SCALER300_OSD_FONT_CHAR_MAX];
    u8          index_map[SCALER300_OSD_FONT_CHAR_MAX];
    u8          index[SCALER300_OSD_FONT_CHAR_MAX];
    u16         count;
    u8          bg_color;
    u8          fg_color;
} fscaler300_osd_font_data_t;

typedef struct fscaler300_osd_smooth {
    u8          onoff;                            ///< OSD font smooth effect on/off
    u8          level;                            ///< OSD font smooth level
} fscaler300_osd_smooth_t;

typedef struct fscaler300_img_border {
    u8          border_en[TC_MAX];            ///< resolution X image border effect enable
    u8          border_width[TC_MAX];         ///< resolution X image border width
    u8          color;                            ///< image border color
} fscaler300_img_border_t;

typedef struct fscaler300_palette {
    u32         crcby;                            ///< Y is LSB
} fscaler300_palette_t;

#define OSD_PALETTE_COLOR_AQUA              0x4893CA        /* CrCbY */
#define OSD_PALETTE_COLOR_BLACK             0x808010
#define OSD_PALETTE_COLOR_BLUE              0x6ef029
#define OSD_PALETTE_COLOR_BROWN             0xA15B51
#define OSD_PALETTE_COLOR_DODGERBLUE        0x3FCB69
#define OSD_PALETTE_COLOR_GRAY              0x8080B5
#define OSD_PALETTE_COLOR_GREEN             0x515B51
#define OSD_PALETTE_COLOR_KHAKI             0x894872
#define OSD_PALETTE_COLOR_LIGHTGREEN        0x223690
#define OSD_PALETTE_COLOR_MAGENTA           0xDECA6E
#define OSD_PALETTE_COLOR_ORANGE            0xBC5198
#define OSD_PALETTE_COLOR_PINK              0xB389A5
#define OSD_PALETTE_COLOR_RED               0xF05A52
#define OSD_PALETTE_COLOR_SLATEBLUE         0x60A63D
#define OSD_PALETTE_COLOR_WHITE             0x8080EB
#define OSD_PALETTE_COLOR_YELLOW            0x9210D2

typedef enum {
    WHITE_IDX = 0,
    BLACK_IDX,
    RED_IDX,
    BLUE_IDX,
    YELLOW_IDX,
    GREEN_IDX,
    BROWN_IDX,
    DODGERBLUE_IDX,
    GRAY_IDX,
    KHAKI_IDX,
    LIGHTGREEN_IDX,
    MAGENTA_IDX,
    ORANGE_IDX,
    PINK_IDX,
    SLATEBLUE_IDX,
    AQUA_IDX,
    COLOR_IDX_MAX
} fscaler300_color_t;

typedef struct fscaler300_init {
    fscaler300_fcs_t                fcs;                            ///< FCS param.
    fscaler300_dn_t                 dn;                             ///< Denoise param.
    fscaler300_smooth_t             smooth[SD_MAX];                 ///< Smooth param.
    fscaler300_sharpness_t          sharpness[SD_MAX];              ///< Sharpenss param.
    fscaler300_rgb2yuv_matrix_t     rgb2yuv_matrix;                 ///< RGB2YUV Matrix param.
    fscaler300_yuv2rgb_matrix_t     yuv2rgb_matrix;                 ///< YUV2RGB param.
    fscaler300_palette_t            palette[SCALER300_PALETTE_MAX]; ///< Palette param.
    fscaler300_dma_ctrl_t           dma_ctrl;                       ///< dma param.
    fscaler300_int_mask_t           int_mask;                       ///< interrupt mask
    fscaler300_osd_smooth_t         osd_smooth;                    ///< osd smooth
} fscaler300_init_t;

typedef struct fscaler300_frm2field {
    u8          res0_frm2field;                   ///< frame to field
    u8          res1_frm2field;                   ///< frame to field
    u8          res2_frm2field;                   ///< frame to field
    u8          res3_frm2field;                   ///< frame to field
} fscaler300_frm2field_t;

typedef struct fscaler300_sc {
    u8          type;                   ///< channel source type
    u16         x_start;                ///< channel cropping x start position
    u16         x_end;                  ///< channel cropping x end
    u16         y_start;                ///< channel cropping y start position
    u16         y_end;                  ///< channel cropping y end
    u8          frame_type;             ///< define video input fromat for scaler mode, 0:odd, 1:even
    u16         sca_src_width;          ///< source image width in scaler mode
    u16         sca_src_height;         ///< source image height in scaler mode
    u8          fix_pat_en;             ///< fix pattern mode
    u8          dr_en;                  ///< data range enable
    u8          rgb2yuv_en;             ///< rgb2yuv enable
    u8          rb_swap;                ///< R B data swap
    u8          fix_pat_cb;             ///< fix scaler input Cb value
    u8          fix_pat_y0;             ///< fix scaler input Y0 value
    u8          fix_pat_cr;             ///< fix scaler input Cr value
    u8          fix_pat_y1;             ///< fix scaler input Y1 value
} fscaler300_sc_t;

typedef struct fscaler300_sd {
    u8          enable;                 ///< image scaling size enable
    u8          bypass;                 ///< bypass the image from crop result
    u16         width;                  ///< scaler width, multiple of 4
    u16         height;                 ///< scaler height, multiple of 4
    u16         hstep;                  ///< scaler horizontal step size
    u16         vstep;                  ///< scaler vertical step size
    u16         topvoft;                ///< scaler top fd vertical offset
    u16         botvoft;                ///< scaler bot fd vertical offset
} fscaler300_sd_t;

typedef struct fscaler300_mask {
    u8          enable;                 ///< Mask window enable
    u16         x_start;                ///< Mask window X start position
    u16         x_end;                  ///< Mask window X end position
    u8          border_width;           ///< Mask window border width when hollow on
    u16         y_start;                ///< Mask window Y start position
    u16         y_end;                  ///< Mask window Y start position
    u8          color;                  ///< Mask window color
    u8          alpha;                  ///< Mask window transparency
    u8          true_hollow;            ///< Mask window hollow/true
} fscaler300_mask_t;

typedef enum {
    SCALER_ALIGN_NONE = 0,             ///< display window on absolute position
    SCALER_ALIGN_TOP_LEFT,             ///< display window on relative position, base on output image size
    SCALER_ALIGN_TOP_CENTER,           ///< display window on relative position, base on output image size
    SCALER_ALIGN_TOP_RIGHT,            ///< display window on relative position, base on output image size
    SCALER_ALIGN_BOTTOM_LEFT,          ///< display window on relative position, base on output image size
    SCALER_ALIGN_BOTTOM_CENTER,        ///< display window on relative position, base on output image size
    SCALER_ALIGN_BOTTOM_RIGHT,         ///< display window on relative position, base on output image size
    SCALER_ALIGN_CENTER,               ///< display window on relative position, base on output image size
    SCALER_ALIGN_MAX
} SCALER_ALIGN_T;

typedef struct fscaler300_osd {
    SCALER_ALIGN_T  align_type;             ///< OSD window auto align method
    u8          enable;                 ///< OSD window enable
    u8          border_en;              ///< OSD window border effect enable
    u8          task_sel;               ///< OSD window acted resolution
    u16         wd_addr;                ///< OSD window word address
    u8          h_wd_num;               ///< OSD window horizontal word number minus one
    u8          v_wd_num;               ///< OSD window vertical word number minus one
    u16         x_start;                ///< OSD window X start position
    u16         x_end;                  ///< OSD window X end position
    u8          font_zoom;              ///< OSD window font size zoom value
    u8          font_alpha;             ///< OSD window font transparency
    u16         y_start;                ///< OSD window Y start position
    u16         y_end;                  ///< OSD window Y end position
    u8          bg_color;               ///< OSD window background color
    u8          bg_alpha;               ///< OSD window background transparency
    u16         row_sp;                 ///< OSD window row space
    u16         col_sp;                 ///< OSD window column space
    u8          border_width;           ///< OSD window border width
    u8          border_color;           ///< OSD window border color
    u8          border_type;            ///< OSD window border type
    u8          font_color;             ///< OSD window font color
} fscaler300_osd_t;

typedef struct fscaler300_tc {
    u8          enable;                 ///< target image cropping enable
    u16         width;                  ///< target cropping width
    u16         height;                 ///< target cropping height
    u16         x_start;                ///< cropping x start position
    u16         x_end;                  ///< cropping x end position
    u16         y_start;                ///< cropping y start position
    u16         y_end;                  ///< cropping y end position
    u8          border_width;           ///< image border width
    u8          drc_en;                 ///< data range convertion enable
} fscaler300_tc_t;

typedef struct fscaler300_dma {
    u32         addr;                   ///< DMA image destination buffer start address, word alignment
    u32         line_offset;            ///< DMA image destination buffer address offset
    u32         field_offset;           ///< DMA bottom field offset
} fscaler300_dma_t;
#ifdef SPLIT_SUPPORT
typedef struct fscaler300_split_dma {
    u32         res0_addr;              ///< DMA image destination buffer start address, word alignment
    u32         res0_offset;            ///< DMA image destination buffer address offset
    u32         res0_field_offset;      ///< DMA bottom field offset
    u32         res1_addr;              ///< DMA image destination buffer start address, word alignment
    u32         res1_offset;            ///< DMA image destination buffer address offset
    u32         res1_field_offset;      ///< DMA bottom field offset
} fscaler300_split_dma_t;
#endif
typedef struct fscaler300_lli {
    u16         next_cmd_addr;          ///< next link list command start address
    u16         curr_cmd_addr;          ///< current link list command start address
    u8          next_cmd_null;          ///< next link list chain is null
    u8          job_count;
} fscaler300_lli_t;

/*
 *  two mask & osd parameter for ping-pong buffer,
 *  mask & osd_pp_sel to select which parameter,
 *  mask & osd_ref_cnt to record how many job reference to this parameter
 */
typedef struct fscaler300_chn_global {
    /* MASK */
    fscaler300_mask_t           mask[2][MASK_MAX];      ///< mask
    /* OSD */
    fscaler300_osd_t            osd[2][OSD_MAX];        ///< osd
    /* ping pong buffer control */
    atomic_t                    mask_pp_sel[MASK_MAX];
    atomic_t                    osd_pp_sel[OSD_MAX];
    atomic_t                    mask_tmp_sel[MASK_MAX];
    atomic_t                    osd_tmp_sel[OSD_MAX];
    atomic_t                    mask_ref_cnt[2][MASK_MAX];
    atomic_t                    osd_ref_cnt[2][OSD_MAX];
} fscaler300_chn_global_t;

/*
 * fscaler300 global parameters
 */
typedef struct fscaler300_global_param {
    /* global */
    fscaler300_init_t           init;
    /* each channel global param(MASK, OSD) */
    fscaler300_chn_global_t     *chn_global;
} fscaler300_global_param_t;

#define ODD_FIELD   0
#define EVEN_FIELD  1
#define PROGRESSIVE 2

typedef enum {
    SINGLE_FRAME_MODE = 0,
    LINK_LIST_MODE = 1
} capture_mode_t;

typedef enum {
    YUV422 = 0,
    RGB555 = 1,
    RGB565 = 2,
    ARGB8888 = 3
} data_foramt_t;
#ifdef SPLIT_SUPPORT
typedef struct fscaler300_split_ctrl {
    u32         split_en_15_00;
    u32         split_en_31_16;
    u32         split_bypass_15_00;
    u32         split_bypass_31_16;
    u32         split_sel_07_00;
    u32         split_sel_15_08;
    u32         split_sel_23_16;
    u32         split_sel_31_24;
    u8          split_blk_num;
} fscaler300_split_ctrl_t;
#endif
typedef struct fscaler300_global {
    capture_mode_t              capture_mode;
    u8                          sca_src_format;
    u8                          src_split_en;
    u8                          tc_format;
    u8                          tc_rb_swap;
    u8                          dma_fifo_wm;
    u8                          dma_job_wm;
} fscaler300_global_t;

typedef struct fscaler300_mirror {
    u8                          h_flip:1;           ///< mirror filp of horizontal direction
    u8                          v_flip:1;           ///< mirror filp of vertical direction
} fscaler300_mirror_t;

typedef struct fscaler300_pip {
    u8                          win_cnt;
    u16                         src_x[PIP_COUNT];
    u16                         src_y[PIP_COUNT];
    u16                         src_width[PIP_COUNT];
    u16                         src_height[PIP_COUNT];
    u16                         dst_x[PIP_COUNT];
    u16                         dst_y[PIP_COUNT];
    u16                         dst_width[PIP_COUNT];
    u16                         dst_height[PIP_COUNT];
    u8                          src_frm[PIP_COUNT];
    u8                          dst_frm[PIP_COUNT];
    u8                          scl_info[PIP_COUNT];
    fscaler300_dma_t            src_dma[PIP_COUNT];         ///< source dma address, offset
    fscaler300_dma_t            dest_dma[PIP_COUNT];        ///< destination dma address, offset
} fscaler300_pip_t;

/*
 * fscaler300 hw parameters
 */
typedef struct fscaler300_param {
    /* Config */
    fscaler300_global_t         global;             ///< global
    fscaler300_mirror_t         mirror;             ///< mirror flip
    fscaler300_frm2field_t      frm2field;          ///< frm2field
    fscaler300_sc_t             sc;                 ///< source cropping
    fscaler300_sd_t             sd[SD_MAX];         ///< scaler
    fscaler300_tc_t             tc[TC_MAX];         ///< target cropping
    fscaler300_lli_t            lli;                ///< link list
    fscaler300_dma_t            src_dma;            ///< source dma address, offset
    fscaler300_dma_t            dest_dma[DMA_MAX];  ///< destination dma address, offset
    fscaler300_mask_t           mask[4];            ///< mask for ap border used
    fscaler300_img_border_t     img_border;
#ifdef SPLIT_SUPPORT
    fscaler300_split_dma_t      split_dma[32];      ///< split dma address, offset
    fscaler300_split_ctrl_t     split;
#endif
    u16     src_format;
    u16     dst_format;
    u16     src_x;
    u16     src_y;
    u16     src_width;
    u16     src_height;
    u16     src_bg_width;
    u16     src_bg_height;
    u8      src_interlace;
    u16     dst_x[SD_MAX];
    u16     dst_y[SD_MAX];
    u16     dst_width[SD_MAX];
    u16     dst_height[SD_MAX];
    u16     dst_bg_width;
    u16     dst_bg_height;
    u16     dst2_bg_width;
    u16     dst2_bg_height;
    u8      scl_feature;
    u8      di_result;
    u8      ratio;
    unsigned int out_addr_pa[SD_MAX];
    int     out_size;
    int     out_1_size;
    unsigned int in_addr_pa;
    int     in_size;
    unsigned int in_addr_pa_1;
    int     in_1_size;
} fscaler300_param_t;

/* job status */
typedef enum {
    JOB_STS_QUEUE       = 0x1,      // not yet add to link list memory
    JOB_STS_SRAM        = 0x2,      // add to link list memory, but not change last job's "next link list pointer" to this job
    JOB_STS_ONGOING     = 0x4,      // already change last job's "next link list pointer" to this job
    JOB_STS_DONE        = 0x8,      // job finish
    JOB_STS_FLUSH       = 0x10,     // job to be flush
    JOB_STS_DONOTHING   = 0x20,     // job do nothing
    JOB_STS_FAIL        = 0x40,     // job fail
    JOB_STS_TX          = 0x80,     // set rc job done when ep job done
    JOB_STS_MATCH       = 0x100,    // ep job match with rc job
    JOB_STS_TIMEOUT     = 0x200,    // rc job timeout
    JOB_STS_MARK_SUCCESS= 0x400,    // ep job timeout, callback ok to EP, callback fai to RC
    JOB_STS_TEST
} job_status_t;

/*
 *
 */
#ifdef SPLIT_SUPPORT
typedef struct split_flow {
    char    ch_0_6;
    char    ch_7_13;
    char    ch_14_20;
    char    ch_21_27;
    char    ch_28_31;
    char    count;
} fscaler300_split_flow_t;
#endif
typedef struct ll_flow {
    char    res123;
    char    mask4567;
    char    osd0123456;
    char    osd7;
    char    count;
} fscaler300_ll_flow_t;

typedef enum {
    TYPE_FIRST          = 0x1,      // job from VG is more than 1 job in scaler300, set as first job
    TYPE_MIDDLE         = 0x2,      // job from VG is more than 3 job in scaler300, job is neither first job nor last job
    TYPE_LAST           = 0x4,      // job from VG is more than 1 job in scaler300, set as last job
    TYPE_ALL            = 0x8,      // job from VG is only 1 job in scaler300
    TYPE_TEST
} job_perf_type_t;

#define SINGLE_JOB_BLK  2   // single job default need link list memory block count = 2
#define VIRTUAL_JOB_BLK 1   // virtual job default need link list memory block count = 2
#define MULTI_DJOB_BLK  2   // multiple job with different source address default need link list memory block count = 2
#define MULTI_SJOB_BLK  4   // multiple job with same source address default need link list memory block count = 3

typedef enum {
    SINGLE_JOB         = 0x1,       // single job
    VIRTUAL_JOB        = 0x3,       // single job with virtual channel
    VIRTUAL_MJOB       = 0x4,       // single job with virtual channel, but more than 1 chain list
    MULTI_DJOB         = 0x10,      // multi job with different source addr
    MULTI_SJOB         = 0x30,      // multi job with same source address
    NONE_JOB
} job_type_t;

/*
 * record input node info
 */
typedef struct input_node_info {
    char                    enable;
    int                     idx;
    char                    engine;         //engine id
    int                     minor;          //channel id
    struct f_ops            *fops;
    void                    *private;       //private data, now pointer to job_node_t
} input_node_info_t;

/*
 * link list block info
 */
typedef struct ll_blk_info {
    u32     status_addr;
    u32     last_cmd_addr;
    u8      blk_count;
    u32     blk_num[BLK_MAX];
    u8      is_last;
} fscaler300_ll_blk_t;

/*
 * record channel bit mask
 */
typedef struct ch_bitmask {
    u32     mask[CH_BITMASK_NUM];
} ch_bitmask_t;

/*
 * job information.
 */
typedef struct fscaler300_job {
    input_node_info_t       input_node[SD_MAX];     // input node data
    fscaler300_param_t      param;
    job_type_t              job_type;
    job_status_t            status;
    fscaler300_ll_blk_t     ll_blk;
    struct list_head        plist;                  // parent list
    struct list_head        clist;                  // child list
    ch_bitmask_t            ch_bitmask;
    int                     job_id;
    char                    need_callback;
    char                    priority;
    int                     job_cnt;
    atomic_t                ref_cnt;
    char                    mask_pp_sel[SD_MAX][MASK_MAX];  // mask ping pong buffer select
    char                    osd_pp_sel[SD_MAX][OSD_MAX];    // osd ping pong buffer select
    fscaler300_ll_flow_t    ll_flow;
#ifdef SPLIT_SUPPORT
    fscaler300_split_flow_t split_flow;
#endif
    void                    *parent;                // point to parent job
    job_perf_type_t         perf_type;              // for mark engine start bit, videograph 1 job maybe have more than 1 job in scaler300
                                                    // mark engine start at first job, other dont't need to mark, use perf_mark to check.
} fscaler300_job_t;

struct f_ops {
    void  (*callback)(fscaler300_job_t *job);
    void  (*update_status)(fscaler300_job_t *job, int status);
    int   (*pre_process)(fscaler300_job_t *job);
    int   (*post_process)(fscaler300_job_t *job);
    void  (*mark_engine_start)(int dev, fscaler300_job_t *job);
    void  (*mark_engine_finish)(int dev, fscaler300_job_t *job);
    int   (*flush_check)(fscaler300_job_t *job);
    void  (*record_property)(fscaler300_job_t *job);
};

#define LOCK_ENGINE(dev)        (priv.engine[dev].busy=1)
#define UNLOCK_ENGINE(dev)      (priv.engine[dev].busy=0)
#define ENGINE_BUSY(dev)        (priv.engine[dev].busy==1)

/*
 * parameter from V.G property
 */
#if 0
typedef struct fscaler300_property {
    int vch_enable[MAX_VCH_NUM];        //virtual channel enable
    int src_fmt;
    int src_x[MAX_VCH_NUM];
    int src_y[MAX_VCH_NUM];
    int src_bg_width;
    int src_bg_height;
    int src_width[MAX_VCH_NUM];
    int src_height[MAX_VCH_NUM];
    int src_interlace[MAX_VCH_NUM];
    int dst_fmt;
    int dst_x[MAX_VCH_NUM];
    int dst_y[MAX_VCH_NUM];
    int dst_bg_width;
    int dst_bg_height;
    int dst_width[MAX_VCH_NUM];
    int dst_height[MAX_VCH_NUM];
    int di_result;                      // from 3di, 0:do nothing  1:deinterlace  2:copy line, 3:denoise only
    int sub_yuv;                        // 0: no ratio frame, 1: has ratio frame
    int scl_feature[MAX_VCH_NUM];       // 0: do nothing, 1: scaling, 2: line correct
    unsigned int out_addr_pa;
    int out_size;
    unsigned int in_addr_pa;
    int in_size;
    unsigned int in_addr_pa_1;
    int in_1_size;
    int job_cnt;                        // virtual channel's job count
    int ratio;                          // from proc ratio
} fscaler300_property_t;
#endif

typedef enum {
    H_TYPE  = 1,             // dst height has change, dst width doesn't change means horizontal type
    V_TYPE  = 2,             // dst width has change, dst height doesn't change means vertical type
} aspect_ratio_type;

typedef struct auto_aspect_ratio {
    aspect_ratio_type   type;
    char                enable;
    char                pal_idx;
    unsigned short      dst_width;
    unsigned short      dst_height;
    unsigned short      dst_x;
    unsigned short      dst_y;
} auto_aspect_ratio_t;

typedef struct auto_border {
    char                enable;
    char                width;
    char                pal_idx;
} auto_border_t;

typedef struct node_info {
    int     job_cnt;
    int     blk_cnt;
} node_info_t;

typedef struct cas_info {
    int     width[50];
    int     height[50];
} cas_info_t;

#if 0
typedef struct cvbs_info {
    int     job_cnt;
    int     blk_cnt;
} cvbs_info_t;
#endif
typedef struct fscaler300_vproperty {
    char            vch_enable;                    //virtual channel enable
    unsigned short  src_x;
    unsigned short  src_y;
    unsigned short  src_width;
    unsigned short  src_height;
    unsigned short  dst_x;
    unsigned short  dst_y;
    unsigned short  dst_width;
    unsigned short  dst_height;
    unsigned short  src2_x;
    unsigned short  src2_y;
    unsigned short  src2_width;
    unsigned short  src2_height;
    unsigned short  dst2_x;
    unsigned short  dst2_y;
    unsigned short  dst2_width;
    unsigned short  dst2_height;
    char            src_interlace;
    char            scl_feature;                   // 0: do nothing, 1: scaling, 2: line correct
    char            buf_clean;
    auto_aspect_ratio_t aspect_ratio;
    auto_border_t       border;
    node_info_t     vnode_info;
    scl_dma_t       scl_dma;
} fscaler300_vproperty_t;

typedef struct fscaler300_property {
    fscaler300_vproperty_t  *vproperty;
    fscaler300_pip_t        pip;
    unsigned short      src_fmt;
    unsigned short      src_bg_width;   /* main screen */
    unsigned short      src_bg_height;
    unsigned short      dst_fmt;
    unsigned short      dst_bg_width;
    unsigned short      dst_bg_height;
    unsigned short      src2_bg_width;  /* 2 means CVBS */
    unsigned short      src2_bg_height;
    unsigned short      dst2_bg_width;
    unsigned short      dst2_bg_height;
    unsigned short      win2_bg_width;
    unsigned short      win2_bg_height;
    unsigned short      win2_x; /* cvbs zoom x */
    unsigned short      win2_y; /* cbvs zoom y */

    unsigned short      cas_ratio;
    char                di_result;                     // from 3di, 0:do nothing  1:deinterlace  2:copy line, 3:denoise only
    char                sub_yuv;                       // 0: no ratio frame, 1: has ratio frame
    unsigned int        out_addr_pa;
    int                 out_size;
    unsigned int        out_addr_pa_1;
    int                 out_1_size;
    unsigned int        in_addr_pa;
    int                 in_size;
    unsigned int        in_addr_pa_1;
    int                 in_1_size;
    int                 job_cnt;                        // virtual channel's job count
    char                ratio;                          // from proc ratio
    unsigned int        pcie_addr;                      // for pcie windows address
    int                 vg_serial;
} fscaler300_property_t;

typedef struct fscaler300_dma_buf {
    char        name[20];
    void        *vaddr;
    dma_addr_t  paddr;
    size_t      size;
} fscaler300_dma_buf_t;

/*
 * hardware engine status
 */
typedef struct fscaler300_eng {
    u8                          dev;            //dev=0,1,2,3... engine id
    u8                          irq;
    u8                          busy;
    int                         blk_cnt;
    u32                         sram_table[SCALER300_SRAM_BLK];
    ch_bitmask_t                ch_bitmask;
    u32                         fscaler300_reg; // scaler300 base address
    atomic_t                    job_cnt;
    struct list_head            qlist;          // record engineX queue list from V.G, job's status = JOB_STS_QUEUE
    struct list_head            slist;          // record engineX sram list, job's status = JOB_STS_SRAM
    struct list_head            wlist;          // record engineX working list, job's status = JOB_STS_ONGOING
    struct timer_list           timer;
    u32                         timeout;
    atomic_t                    dummy_cnt;
    fscaler300_osd_font_data_t  font_data;
    fscaler300_dma_buf_t        temp_buf;
    fscaler300_dma_buf_t        pip1_buf;
} fscaler300_eng_t;

typedef struct fscaler300_priv {
    spinlock_t                  lock;
    struct semaphore            sema_lock;          ///< driver semaphore lock
    unsigned int                eng_num;
    fscaler300_eng_t            engine[SCALER300_ENGINE_NUM];
    struct kmem_cache           *job_cache;
    fscaler300_global_param_t   global_param;
    atomic_t                    mem_cnt;
    struct task_struct 				*add_task;
} fscaler300_priv_t;

typedef struct temp_osd {
    u16     x_start;
    u16     x_end;
    u16     y_start;
    u16     y_end;
} temp_osd_t;

/*
 * extern global variables
 */
extern fscaler300_priv_t   priv;
/*
 * V.G interface Put a job to fscaler300 driver interface
 */
int fscaler300_put_job(void);
/*
 * add job to engine link list
 */
int fscaler300_joblist_add(fscaler300_job_t *job);
/*
 * add priority job to engine link list
 */
int fscaler300_prioritylist_add(fscaler300_job_t *job);
/*
 * add chain list to engine link list
 */
int fscaler300_chainlist_add(struct list_head *list);
/*
 * add priority chain list to engine link list
 */
int fscaler300_pchainlist_add(struct list_head *list);
/*
 * V.G interface stop channel's job to fscaler300 driver interface
 */
int fscaler300_stop_channel(int chn);
/*
 * free a job to fscaler300 driver interface
 */
void fscaler300_job_free(int dev, fscaler300_job_t *job, int schedule);
/*
 * HW specific function
 */
int fscaler300_write_dummy(int dev, u32 offset, fscaler300_param_t *dummy, fscaler300_job_t *job);
int fscaler300_write_register(int dev, fscaler300_job_t *job);
int fscaler300_init_global(int dev, fscaler300_global_param_t *global_param);
int fscaler300_write_lli_cmd(int dev, int addr, fscaler300_job_t *job, int type);
int fscaler300_single_fire(int dev);

u32 fscaler300_lli_blk0_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last);
u32 fscaler300_lli_blk1_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last);
u32 fscaler300_lli_blk2_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last);
u32 fscaler300_lli_blk3_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last);
#ifdef SPLIT_SUPPORT
u32 fscaler300_lli_split_blk0_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last);
u32 fscaler300_lli_split_blk1_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last);
u32 fscaler300_lli_split_blk2_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last);
u32 fscaler300_lli_split_blk3_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last);
u32 fscaler300_lli_split_blk4_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last);
u32 fscaler300_lli_split_blk5_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last);
u32 fscaler300_lli_split_blk6_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last);
int fscaler300_write_split_register(int dev, fscaler300_job_t *job);
#endif
int fscaler300_palette_init(void);
void fscaler300_sw_reset(int dev);
#endif
