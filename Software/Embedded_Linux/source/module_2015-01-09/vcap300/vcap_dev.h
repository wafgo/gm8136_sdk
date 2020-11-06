#ifndef _VCAP_DEV_H_
#define _VCAP_DEV_H_

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#ifdef CONFIG_PLATFORM_A369
#include <mach/platform/spec.h>
#else
#include <mach/platform/platform_io.h>
#include <mach/ftpmu010.h>
#endif

#include "vcap_reg.h"

#define VCAP_DEV_MAX                1

/*************************************************************************************
 *  VCAP Device Platform Definition
 *************************************************************************************/
#if defined(CONFIG_PLATFORM_A369)
#define VCAP_PHY_ADDR_BASE          0xC0100000
#define VCAP_IRQ_NUM                PLATFORM_EXTAHB_IRQ
#define VCAP_REG_SIZE               0x48000
#define VCAP_CLK_IN                 APB_CLK_IN
#elif defined(CONFIG_PLATFORM_GM8210)
#define VCAP_PHY_ADDR_BASE          CAP_FTCAP300_PA_BASE    ///< 0x96100000
#define VCAP_IRQ_NUM                CAP_FTCAP300_IRQ        ///< 32
#define VCAP_REG_SIZE               0x48000
#define VCAP_CLK_IN                 AXI_CLK_IN
#elif defined(CONFIG_PLATFORM_GM8287)
#define VCAP_PHY_ADDR_BASE          CAP_FTCAP300_PA_BASE    ///< 0x96100000
#define VCAP_IRQ_NUM                CAP_FTCAP300_IRQ        ///< 32
#define VCAP_REG_SIZE               0x48000
#define VCAP_CLK_IN                 AXI_CLK_IN
#elif defined(CONFIG_PLATFORM_GM8139)
#define VCAP_PHY_ADDR_BASE          CAP_FTCAP300_PA_BASE    ///< 0x96100000
#define VCAP_IRQ_NUM                CAP_FTCAP300_IRQ        ///< 32
#define VCAP_REG_SIZE               0x48000
#define VCAP_CLK_IN                 AXI_CLK_IN
#elif defined(CONFIG_PLATFORM_GM8136)
#define VCAP_PHY_ADDR_BASE          CAP_FTCAP300_PA_BASE    ///< 0x96100000
#define VCAP_IRQ_NUM                CAP_FTCAP300_IRQ        ///< 32
#define VCAP_REG_SIZE               0x48000
#define VCAP_CLK_IN                 AXI0_CLK_IN
#else
#error "unknown platform"
#endif


/*************************************************************************************
 *  Alginment, for x postion and width of VI, SC, Scaler, OSD, Mask, Mark, TC
 *************************************************************************************/
#define ALIGN_4(x)                          (((x)>>2)<<2)
#define ALIGN_2(x)                          (((x)>>1)<<1)           ///< only mask can align 2
#define CHECK_ALIGN_4(x)                    (((x) & 0x3) ? 0 : 1)
#define VCAP_DMA_ADDR(addr)                 ((addr) & (~0x7))
#define VCAP_DMA_LINE_OFFSET(x)             ((x)>>2)
#define VCAP_DMA_FIELD_OFFSET(x)            ((x)>>3)
#define VCAP_DMA_FRM2FIELD_FIELD_OFFSET(x)  ((x)>>5)
#define VCAP_DMA_FIELD_OFFSET_REV1(x)       ((x)>>5)
#define VCAP_DMA_ADDR_ALIGN_SIZE            8                       ///< 8 byte alignment

/*************************************************************************************
 *  VCAP Register Read/Write Definition
 *************************************************************************************/
#define VCAP_REG_RD(info, offset)           (((volatile unsigned int *)((info)->vbase))[(offset)>>2])
#define VCAP_REG_WR(info, offset, v)        (((volatile unsigned int *)((info)->vbase))[(offset)>>2]=(v))
#define VCAP_REG_RD64(info, offset)         (((volatile unsigned long long *)((info)->vbase))[(offset)>>3])
#define VCAP_REG_WR64(info, offset, v)      (((volatile unsigned long long *)((info)->vbase))[(offset)>>3]=(v))
#define VCAP_REG_RD8(info, offset)          (((volatile unsigned char *)((info)->vbase))[offset])
#define VCAP_REG_WR8(info, offset, v)       (((volatile unsigned char *)((info)->vbase))[offset]=(v))

/*************************************************************************************
 *  VCAP Parameter Memory Definition
 *************************************************************************************/
#define VCAP_PARAM_MEM_SIZE                 PLAT_PARAM_MEM_SIZE

/*************************************************************************************
 *  VCAP Internal SRAM Definition
 *************************************************************************************/
#define VCAP_OSD_MEM_SIZE                   PLAT_OSD_SRAM_SIZE        ///< word access (64 bits)
#define VCAP_MARK_MEM_SIZE                  PLAT_MARK_SRAM_SIZE       ///< word access (64 bits)
#define VCAP_DISP_MEM_SIZE                  PLAT_DISP_SRAM_SIZE       ///< word access (64 bits)
#define VCAP_LLI_MEM_SIZE                   PLAT_LLI_SRAM_SIZE        ///< word access (64 bits)

/*************************************************************************************
 * VCAP Scaler Definition
 *************************************************************************************/
#define VCAP_SCALER_LINE_BUF_NUM            PLAT_SCALER_LINE_BUF_NUM
#define VCAP_SCALER_LINE_BUF_WIDTH          PLAT_SCALER_LINE_BUF_WIDTH
#define VCAP_SCALER_LINE_BUF_SIZE           PLAT_SCALER_LINE_BUF_SIZE
#define VCAP_SCALER_HORIZ_SIZE              PLAT_SCALER_HORIZ_SIZE
#define VCAP_SCALER_MAX                     PLAT_SCALER_MAX

/*************************************************************************************
 * VCAP VI & Cascade Definition
 *************************************************************************************/
#define VCAP_VI_MAX                         (PLAT_VI_MAX + PLAT_CASCADE_MAX)
#define VCAP_VI_CH_MAX                      4
#define VCAP_CASCADE_MAX                    PLAT_CASCADE_MAX
#define VCAP_CHANNEL_MAX                    ((PLAT_VI_MAX*VCAP_VI_CH_MAX) + VCAP_CASCADE_MAX)
#define CH_TO_VI(ch)                        ((ch)/VCAP_VI_CH_MAX)
#define SUB_CH(vi, ch)                      (((vi)*VCAP_VI_CH_MAX)+(ch))
#define SUBCH_TO_VICH(ch)                   ((ch)%VCAP_VI_CH_MAX)

#define VCAP_CASCADE_VI_NUM                 PLAT_VI_MAX                        ///< the last vi is cascade
#define VCAP_CASCADE_CH_NUM                 (SUB_CH(VCAP_CASCADE_VI_NUM, 0))   ///< the last channel is cascade channel, the cascade only one channel
#define VCAP_CASCADE_HW_CH_ID               32
#define VCAP_CASCADE_LLI_CH_ID              VCAP_CASCADE_HW_CH_ID

#ifdef PLAT_RGB_VI_IDX
#define VCAP_RGB_VI_IDX                     PLAT_RGB_VI_IDX
#else
#define VCAP_RGB_VI_IDX                     -1                                  ///< means no VI support RGB interface format
#endif

/*************************************************************************************
 *  VCAP Palette Definition
 *************************************************************************************/
#define VCAP_PALETTE_MAX                    16

/*************************************************************************************
 *  VCAP Mask Definition
 *************************************************************************************/
#define VCAP_MASK_WIN_MAX                   PLAT_MASK_WIN_MAX

/*************************************************************************************
 * VCAP OSD Definition
 *************************************************************************************/
#define VCAP_OSD_PIXEL_BITS                 PLAT_OSD_PIXEL_BITS
#define VCAP_OSD_FONT_COL_SIZE              12                                              ///< 12 pixel per one column
#define VCAP_OSD_FONT_ROW_SIZE              18                                              ///< 18 pixel per one row
#define VCAP_OSD_CHAR_MAX                   (PLAT_OSD_CHAR_NUM-PLAT_OSD_PALETTE_CHAR_NUM)
#define VCAP_OSD_CHAR_OFFSET(idx)           ((VCAP_OSD_FONT_ROW_SIZE*(idx)*64)>>3)          ///< one row is 64 bit
#define VCAP_OSD_DISP_MAX                   (PLAT_OSD_DISP_NUM-PLAT_OSD_PALETTE_CHAR_NUM)
#define VCAP_OSD_WIN_MAX                    PLAT_OSD_WIN_MAX
#define VCAP_OSD_PALETTE_CHAR_MAX           PLAT_OSD_PALETTE_CHAR_NUM
#define VCAP_OSD_PALETTE_CHAR_IDX(x)        ((x)+VCAP_OSD_CHAR_MAX)
#define VCAP_OSD_PALETTE_DISP_IDX(x)        ((x)+VCAP_OSD_DISP_MAX)
#define VCAP_OSD_MASK_PALETTE_MAX           ((PLAT_OSD_PIXEL_BITS == 4) ? VCAP_OSD_PALETTE_CHAR_MAX : VCAP_PALETTE_MAX)
#define VCAP_OSD_BORDER_PIXEL               PLAT_OSD_BORDER_PIXEL

/*************************************************************************************
 *  VCAP Mark Definition
 *************************************************************************************/
#define VCAP_MARK_WIN_MAX                   PLAT_MARK_WIN_MAX
#define VCAP_MARK_IMG_MAX                   (VCAP_MARK_MEM_SIZE/(16*16*2))           ///< min image => 16(w)x16(h)x2(pixel)

/*************************************************************************************
 *  VCAP Hardware Capability Structure
 *************************************************************************************/
struct vcap_dev_capability_t {
    u32 sharpness:1;          ///< sharpness, 0: not support, 1: support
    u32 denoise:1;            ///< denoise, 0: not support, 1: support
    u32 fcs:1;                ///< fcs, 0: not support, 1: support
    u32 vi_num:4;             ///< Number of VI
    u32 cas_num:4;            ///< Number of Cascade
    u32 mask_win_num:4;       ///< Number of Mask Window
    u32 osd_win_num:4;        ///< Number of OSD Window
    u32 mark_win_num:3;       ///< Number of Mark Window
    u32 scaler_num:3;         ///< Number of Scaler
    u32 scaling_cap:4;        ///< Scaling capability of scaler, bit0 => scaler#0, bit1 => scaler#1.....bitN => scaler#N
    u32 reserved:3;
    u8  dma_field_offset_rev; ///< ECO, DMA Field Offset Revision, 0: shift 3, 1: shift 5
    u8  md_rev;               ///< ECO, MD shadow must disable of revision = 0
    u8  bug_p2i_miss_top;     ///< ECO, P2I Miss Top Field Bug, for Cascade workaround used, force path#3 as workaround path
    u8  bug_two_dma_ch;       ///< ECO, Use 2 DMA Channel LLI No Ready Bug, force all path to use DMA#0
    u8  bug_md_event_96_mb;   ///< ECO, Event value always 0 at MD block 96 ~ 127, should be read event value from Gau memory
    u8  bug_ch14_rolling;     ///< ECO, CH14 sc rolling hardware always 0, set 1 or 2 not work in GM8287
    u8  bug_isp_pixel_lack;   ///< ECO, ISP interface always occur pixel lack status in GM8136
    u8  vi_sdi8bit;           ///< ECO, VI support SDI8BIT format
    u8  vi_bt601;             ///< VI support BT601 8/16Bit
    u8  cas_isp;              ///< Cascade support ISP
    u8  cas_sdi8bit;          ///< Cascade support BT656/BT1120/SDI8BIT format
};

/*************************************************************************************
 *  VCAP Hardware Revision
 *************************************************************************************/
#define VCAP_HW_VER(info)     (VCAP_REG_RD(info, VCAP_STATUS_OFFSET(0xe8)))
#define VCAP_HW_REV(info)     ((VCAP_REG_RD(info, VCAP_STATUS_OFFSET(0xec))>>28)&0x3)

/*************************************************************************************
 *  VCAP Global Definition
 *************************************************************************************/
typedef enum {
    VCAP_CAP_MODE_SSF = 0,      ///< Single Step Fire Mode
    VCAP_CAP_MODE_LLI,          ///< Link-List Mode
    VCAP_CAP_MODE_CONT,         ///< Continue Mode
} VCAP_CAP_MODE_T;

typedef enum {
    VCAP_CH_MODE_NORMAL = 0,
    VCAP_CH_MODE_RESERVED,
    VCAP_CH_MODE_16CH,
    VCAP_CH_MODE_32CH           ///< in ch32 mode, scaler not support scaling up
} VCAP_CH_MODE_T;

/*************************************************************************************
 *  VCAP Sync Timer Definition
 *************************************************************************************/
typedef enum {
    VCAP_SYNC_TIMER_SRC_VI0 = 0,
    VCAP_SYNC_TIMER_SRC_VI1,
    VCAP_SYNC_TIMER_SRC_VI2,
    VCAP_SYNC_TIMER_SRC_VI3,
    VCAP_SYNC_TIMER_SRC_VI4,
    VCAP_SYNC_TIMER_SRC_VI5,
    VCAP_SYNC_TIMER_SRC_VI6,
    VCAP_SYNC_TIMER_SRC_VI7,
    VCAP_SYNC_TIMER_SRC_VI8,
    VCAP_SYNC_TIMER_SRC_ACLK,
} VCAP_SYNC_TIMER_SRC_T;

/*************************************************************************************
 *  VCAP DMA Buffer Definition
 *************************************************************************************/
struct vcap_dma_buf_t {
    char       name[20];
    void       *vaddr;
    dma_addr_t paddr;
    size_t     size;
};

/*************************************************************************************
 *  VCAP VI Structure
 *************************************************************************************/
typedef enum {
    VCAP_VI_FMT_BT656 = 0,
    VCAP_VI_FMT_BT1120,
    VCAP_VI_FMT_SDI8BIT,        ///< only REV#1 support, GM8210 Cascade not support
    VCAP_VI_FMT_RGB888,         ///< only GM8210 Cascade or GM8287 VI#3 support
    VCAP_VI_FMT_BT601_8BIT,     ///< only GM8139 support
    VCAP_VI_FMT_BT601_16BIT,    ///< only GM8139 support
    VCAP_VI_FMT_ISP             ///< only GM8139 Cascade support
} VCAP_VI_FMT_T;

typedef enum {
    VCAP_VI_PROG_INTERLACE = 0,
    VCAP_VI_PROG_PROGRESSIVE
} VCAP_VI_PROG_T;

typedef enum {
    VCAP_VI_TDM_MODE_BYPASS = 0,
    VCAP_VI_TDM_MODE_FRAME_INTERLEAVE,
    VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE,
    VCAP_VI_TDM_MODE_4CH_BYTE_INTERLEAVE,
    VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID,
} VCAP_VI_TDM_T;

typedef enum {
    VCAP_VI_CAP_STYLE_SKIP_ALL = 0,
    VCAP_VI_CAP_STYLE_ANYONE,
    VCAP_VI_CAP_STYLE_ODD_ONLY = 4,
    VCAP_VI_CAP_STYLE_EVEN_ONLY,
    VCAP_VI_CAP_STYLE_ODD_FIRST,
    VCAP_VI_CAP_STYLE_EVEN_FIRST
} VCAP_VI_CAP_STYLE_T;

typedef enum {
    VCAP_VI_DATA_RANGE_256 = 0,
    VCAP_VI_DATA_RANGE_240
} VCAP_VI_DATA_RANGE_T;

typedef enum {
    VCAP_VI_SPEED_I = 0,                    ///< Interlace,   as 50/60I => 2frame buffer apply grab_field_pair
    VCAP_VI_SPEED_P,                        ///< Progressive, as 25/30P => 2frame buffer apply fame2field
    VCAP_VI_SPEED_2P,                       ///< Progressive, as 50/60P => 2frame buffer apply grab 2 frame
} VCAP_VI_SPEED_T;

typedef enum {
    VCAP_VI_GRAB_MODE_NORMAL = 0,           ///< VI config in grab normal image mode
    VCAP_VI_GRAB_MODE_HBLANK,               ///< VI config in grab hblank image mode, source width plus dummy pixel
} VCAP_VI_GRAB_MODE_T;

struct vcap_vi_ch_param_t {
    int                 input_norm;         ///< VI channel running norm mode
    u16                 src_w;              ///< VI channel source width
    u16                 src_h;              ///< VI channel source height
    VCAP_VI_PROG_T      prog;               ///< VI channel format 0:interlace 1:progressive
    VCAP_VI_SPEED_T     speed;              ///< VI channel speed type
    u32                 frame_rate;         ///< VI channel frame rate
};

struct vcap_vi_t {
    u32                       status;                   ///< VI status, bit0~7: ch0, bit8~15: ch1, bit16~23: ch2, bit24~31: ch3
    int                       input_norm;               ///< Input device current running norm mode
    VCAP_VI_SPEED_T           speed;                    ///< Input device speed type

    u16                       src_w;                    ///< VI Source Image Width
    u16                       src_h;                    ///< VI Source Image Height
    VCAP_VI_FMT_T             format;                   ///< VI Input Format 0:BT656, 1:BT1120, 2:SDI8BIT, 3:RGB888(Only Cascade Support)
    VCAP_VI_PROG_T            prog;                     ///< VI Input Mode 0:Interlace, 1:Progressive
    VCAP_VI_CAP_STYLE_T       cap_style;                ///< VI Define capture frame/field order
    VCAP_VI_TDM_T             tdm_mode;                 ///< VI Channel Time-Division-Multiplexed mode
    VCAP_VI_DATA_RANGE_T      data_range;               ///< VI output data range
    u8                        sd_hratio;                ///< VI scaling ratio, 0:bypass, 1~15 scaling down ratio(only Cascade)
    u8                        pclk_div;                 ///< pixel clock divide value, threshold for none pixel clock detection

    u8                        vbi_start_line;           ///< VBI Extract Start Line
    u8                        vbi_enable;               ///< VBI Control
    u16                       vbi_width;                ///< VBI Extract Width
    u16                       vbi_height;               ///< VBI Extract Height

    u32                       frame_rate;               ///< source frame rate, 0:means unknown

    VCAP_VI_GRAB_MODE_T       grab_mode;                ///< VI grab image mode
    u32                       grab_hblank;              ///< VI grab channel horizontal blanking [0]:CH#0 [1]:CH#1 [2]:CH#2 [3]:CH#3
    u32                       chid_det_time;            ///< VI channel id detect time counter

    struct vcap_vi_ch_param_t ch_param[VCAP_VI_CH_MAX]; ///< VI Channel Parameter
};

typedef enum {
    VCAP_VI_PROBE_MODE_NONE = 0,
    VCAP_VI_PROBE_MODE_ACTIVE_REGION,    ///< active pixel and line
    VCAP_VI_PROBE_MODE_BLANK_REGION      ///< vertical and horizontal blanking, progressive not support, field order must set odd only or even only
} VCAP_VI_PROBE_MODE_T;

struct vcap_vi_probe_t {
    VCAP_VI_PROBE_MODE_T  mode;
    int                   vi_idx;
    int                   count;
};

/*************************************************************************************
 * VCAP SC Definition
 *************************************************************************************/
typedef enum {
    VCAP_SC_ROLLING_1024 = 0,       ///< Each Channel have one 1024 pixel SC line buffer
    VCAP_SC_ROLLING_2048,
    VCAP_SC_ROLLING_4096
} VCAP_SC_ROLLING_T;

typedef enum {
    VCAP_SC_TYPE_INPUT = 0,         ///< Normal Mode
    VCAP_SC_TYPE_SPLIT              ///< Split  Mode
} VCAP_SC_TYPE_T;

#define VCAP_SC_VICH0_UPDATE_BY_VICH2    1  ///< use vi-ch#2 to update vi-ch#0 source cropping width

/*************************************************************************************
 * VCAP MD Definition
 *************************************************************************************/
/*
 * MD Limitation
 *
 * 1. normal and cascade channel cann't enable motion detection at the same time
 * 2. normal and split channel cann't enable motion detection at the same time
 *
 * win_x_num:
 *          |--------------------------------------------------------|
 *          |                  |   Split   |       Normal            |
 *          |------------------|-----------|-------------------------|
 *          |                  |           |       ch_mode           |
 *          |                  |           |-------------------------|
 *          |                  |           |    0,3    |      2      |
 *          |------------------|-----------|-----------|-------------|
 *          | VI-1CH           |           |   <=127   |   <=127     |
 *          |------------------|           |-----------|-------------|
 *          | VI-2CH           |           |   <=90    |   <=127     |
 *          |------------------|   <=45    |-----------|-------------|
 *          | VI-4CH           |           |   <=45    |   <=90      |
 *          |------------------|           |-----------|-------------|
 *          | Frame-Interleave |           |   <=127   |   <=127     |
 *          |--------------------------------------------------------|
 *
*/

#define VCAP_Split_MD_X_NUM_MAX              45
#define VCAP_MD_X_NUM_MAX                    128
#define VCAP_MD_Y_NUM_MAX                    128
#define VCAP_MD_GAU_LEN                      128     ///< 128 Bits
#define VCAP_MD_GAU_SIZE                     0x40000 ///< ((VCAP_MD_X_NUM_MAX*VCAP_MD_Y_NUM_MAX*VCAP_MD_GAU_LEN)/8),   must 8 byte alignment
#define VCAP_MD_EVENT_SIZE                   0x1000  ///< ((VCAP_MD_X_NUM_MAX*VCAP_MD_Y_NUM_MAX*VCAP_MD_EVENT_LEN)/8), must 8 byte alignment
#define VCAP_MD_EVENT_LEN                    2       ///< 2  bits
#define VCAP_MD_SPLIT_CH_X_MAX               45
#define VCAP_MD_VI_1CH_X_MAX                 127
#define VCAP_MD_VI_2CH_X_MAX                 90
#define VCAP_MD_VI_4CH_BYTE_X_MAX            45      ///< 4CH Byte  Interleave
#define VCAP_MD_VI_4CH_FRAME_X_MAX           127     ///< 4CH Frame Interleave

#ifdef PLAT_MD_GROUPING
#define VCAP_MD_GROUP_CH_MAX                 PLAT_MD_GROUP_CH_MAX   ///< md channel grouping, workaround for md performance issue
#endif

#define VCAP_MD_GAU_VALUE_CHECK              1                      ///< detect motion block gaussian value update incorrect issue

/*************************************************************************************
 * VCAP LLI General Definition
 *************************************************************************************/
#define VCAP_LLI_MEM_CMD_MAX             (VCAP_LLI_MEM_SIZE/8)               ///< 1 CMD => 64 bits
#define VCAP_LLI_NORMAL_TAB_CMD_MAX      5                                   ///< each normal table have  5 commands
#define VCAP_LLI_UPDATE_TAB_CMD_MAX      12                                  ///< each update table have 12 commands
#define VCAP_LLI_NULL_TAB_CMD_MAX        1                                   ///< each normal table have  1 commands
#define VCAP_LLI_PATH_NORMAL_TAB_MAX     4                                   ///< each path have 4 normal table
#define VCAP_LLI_PATH_NULL_TAB_MAX       (VCAP_LLI_PATH_NORMAL_TAB_MAX+1)    ///< each path have 5 null   table

typedef enum {
    VCAP_UPDATE_IDLE = 0,
    VCAP_UPDATE_PREPARE,
    VCAP_UPDATE_GO
} VCAP_UPDATE_STATE_T;

struct vcap_reg_data_t {
    u32  offset;
    u32  value;
    u8   mask;
};

struct vcap_update_reg_t {
#ifdef PLAT_SPLIT
#define VCAP_UPDATE_REG_MAX     96
#else
#define VCAP_UPDATE_REG_MAX     72
#endif
    struct vcap_reg_data_t      reg[VCAP_UPDATE_REG_MAX];
    u32                         num;
    VCAP_UPDATE_STATE_T         state;
};

typedef enum {
    VCAP_LLI_TAB_TYPE_NORMAL = 0,              ///< Normal Table and have job buffer
    VCAP_LLI_TAB_TYPE_DUMMY,                   ///< Dummy Table, no job buffer with DMA Block
    VCAP_LLI_TAB_TYPE_DUMMY_LOOP,              ///< Dummy Loop Table, no job buffer with DMA Block and Loop itself
    VCAP_LLI_TAB_TYPE_HBLANK                   ///< Grab HBlanking Data, no job buffer
} VCAP_LLI_TAB_TYPE_T;

struct vcap_lli_table_t {
    struct list_head    list;                  ///< for link normal table list
    struct list_head    update_head;           ///< for link update table list
    u32                 addr;                  ///< LL command table memory address
    void                *job;                  ///< pointer to related job
    int                 max_entries;           ///< command table max entries
    int                 curr_entry;            ///< current entry of this command table
    VCAP_LLI_TAB_TYPE_T type;                  ///< table type
};

struct vcap_lli_pool_t {
    spinlock_t  lock;                       ///< pool locker
    int         curr_idx;                   ///< current index of this LL pool
    void        **elements;                 ///< pointer of pool element
};

struct vcap_lli_path_t {
    struct list_head                   ll_head;         ///< for link LL command table
    struct list_head                   null_head;       ///< for link Null command table
    struct vcap_lli_pool_t             ll_pool;         ///< private pool for normal command
    struct vcap_lli_pool_t             null_pool;       ///< private pool for null   command
    u32                                ll_mem_start;    ///< ll memory start address
    u32                                ll_mem_end;      ///< ll memory end address
};

struct vcap_lli_ch_t {
    struct vcap_lli_path_t             path[VCAP_SCALER_MAX];      ///< path LL information
    struct vcap_update_reg_t           update_reg;
};

struct vcap_lli_t {
    struct vcap_lli_ch_t               ch[VCAP_CHANNEL_MAX];       ///< channel LL information
    struct vcap_lli_pool_t             update_pool;                ///< global pool for update command
    struct tasklet_struct              ll_tasklet;                 ///< link list tasklet
};

/*************************************************************************************
 *  VCAP Channel Structure
 *************************************************************************************/
struct vcap_sc_param_t {
    u16  x_start;       ///< source crop x start position, in split mode, this means split_x_num
    u16  y_start;       ///< source crop y start position, in split mode, this means split_y_num
    u16  width;         ///< source crop output width
    u16  height;        ///< source crop output height
};

struct vcap_sharp_param_t {
    u8  adp;
    u8  radius;
    u8  amount;
    u8  thresh;
    u8  start;
    u8  step;
};

struct vcap_sd_param_t {
    u16  src_w;
    u16  src_h;
    u16  out_w;
    u16  out_h;
    u16  hstep;
    u16  vstep;
    u16  topvoft;
    u16  botvoft;
    u32  frm_ctrl;
    u8   frm_wrap;
    u8   hsmo_str;
    u8   vsmo_str;
    u8   none_auto_smo;  ///< disable auto image presmooth
    u16  hsize;
    struct vcap_sharp_param_t sharp;
};

struct vcap_tc_param_t {
    u16  x_start;
    u16  reserved1;
    u16  y_start;
    u16  reserved2;
    u16  width;
    u16  height;
    u8   border_w;
};

struct vcap_md_param_t {
    u16  x_start;
    u16  y_start;
    u8   x_size;
    u8   y_size;
    u8   x_num;
    u8   y_num;
    u32  gau_addr;
    u32  event_addr;
};

struct vcap_comm_param_t {
    u8  fcs_en;
    u8  dn_en;
    u8  md_en;
    u8  md_src;
    u8  h_flip;
    u8  v_flip;
    u8  osd_en;
    u8  mask_en;
    u8  mark_en;
    u8  border_en;
    u8  field_order;
    u8  osd_frm_mode;
    u8  accs_win_en;
    u8  accs_data_thres;    ///< OSD automatic color change scheme pixel data threshold
};

struct vcap_osd_comm_param_t {
    u8  marquee_speed;
    u8  border_color;
    u8  font_edge_mode;     ///< OSD Hardware only apply to OSD Zoom 2X, 4X, H4X_V1X, H4X_V2X, H2X_V4X
};

typedef enum {
    VCAP_ALIGN_NONE = 0,
    VCAP_ALIGN_TOP_LEFT,
    VCAP_ALIGN_TOP_CENTER,
    VCAP_ALIGN_TOP_RIGHT,
    VCAP_ALIGN_BOTTOM_LEFT,
    VCAP_ALIGN_BOTTOM_CENTER,
    VCAP_ALIGN_BOTTOM_RIGHT,
    VCAP_ALIGN_CENTER,
    VCAP_ALIGN_MAX
} VCAP_ALIGN_TYPE_T;

typedef enum {
    VCAP_OSD_WIN_TYPE_FONT = 0,         ///< OSD window as Font window
    VCAP_OSD_WIN_TYPE_MASK,             ///< OSD window as Mask window
    VCAP_OSD_WIN_TYPE_MAX
} VCAP_OSD_WIN_TYPE_T;

struct vcap_osd_param_t {
    VCAP_OSD_WIN_TYPE_T win_type;       ///< osd window type
    VCAP_ALIGN_TYPE_T   align_type;     ///< osd window auto align method
    u16                 align_x_offset; ///< osd window auto align x offset size
    u16                 align_y_offset; ///< osd window auto align y offset size
    u16                 width;          ///< osd window width
    u16                 height;         ///< osd window height
    u16                 x_start;        ///< osd window x start position
    u16                 y_start;        ///< osd window y start position
    u16                 x_end;          ///< osd window x end position
    u16                 y_end;          ///< osd widnow y end position
    u16                 row_sp;         ///< osd window row space
    u16                 col_sp;         ///< osd window column space
    u16                 wd_addr;        ///< osd window display memory address offset
    u8                  path;           ///< osd window display on which path
    u8                  zoom;           ///< osd window font zoom size
    u8                  bg_color;       ///< osd window background color
    u8                  fg_color;       ///< osd window foreground color, not support on GM8210
    u8                  h_wd_num;       ///< osd window horizontal word number
    u8                  v_wd_num;       ///< osd window vertical word number
    u8                  font_alpha;     ///< osd window font transparency
    u8                  bg_alpha;       ///< osd window background transparency
    u8                  border_en;      ///< osd window border enable/disable
    u8                  border_type;    ///< osd window border type, hollow/true
    u8                  border_width;   ///< osd window border width
    u8                  border_color;   ///< osd window border color
    u8                  marquee_mode;   ///< osd window marquee mode
};

struct vcap_mark_param_t {
    VCAP_ALIGN_TYPE_T  align_type;     ///< mark window auto align method
    u16                align_x_offset; ///< mark window auto align x offset size
    u16                align_y_offset; ///< mark window auto align y offset size
    u16                width;          ///< mark window width
    u16                height;         ///< mark window height
    u16                addr;           ///< mark window memory address
    u16                x_start;        ///< mark window x start position
    u16                y_start;        ///< mark window y start position
    u8                 x_dim;          ///< mark window x dimension
    u8                 y_dim;          ///< mark window y dimension
    u8                 alpha;          ///< mark window transparency
    u8                 zoom;           ///< mark window zoom size
    u8                 path;           ///< mark window display on which path
    u8                 id;             ///< mark id
};

struct vcap_ch_param_t {
    struct vcap_comm_param_t     comm;
    struct vcap_osd_comm_param_t osd_comm;
    struct vcap_sc_param_t       sc;
    struct vcap_md_param_t       md;
    struct vcap_sd_param_t       sd[VCAP_SCALER_MAX];
    struct vcap_tc_param_t       tc[VCAP_SCALER_MAX];
    struct vcap_osd_param_t      osd[VCAP_OSD_WIN_MAX];
    struct vcap_mark_param_t     mark[VCAP_MARK_WIN_MAX];
    int                          frame_rate_reinit;
};

#define VCAP_JOB_MAX                CFG_JOB_MAX

struct vcap_path_t {
    struct kfifo                    *job_q;             ///< job queue
    spinlock_t                      job_q_lock;         ///< locker for job queue
    void                            *active_job;        ///< current active job
    u32                             bot_cnt;            ///< top field frame count
    u32                             top_cnt;            ///< bottom field frame count
    u32                             frame_rate_cnt;     ///< software frame rate control counter
    u32                             frame_rate_thres;   ///< software frame rate control threshold
    int                             frame_rate_check;   ///< do software frame rate check
    u32                             ll_tab_cnt;         ///< record total count of link-list link table
    int                             skip_err_once;      ///< skip frame error message print out once
    int                             grab_one_field;     ///< grab one field mode
    int                             grab_field_cnt;     ///< field count on grab one field mode
};

struct vcap_timer_data_t {
    int  ch;
    void *data;
    u32  timeout;               ///< timeout threshold (ms)
    u32  count;                 ///< time value count
};

struct vcap_md_tamper_t {
    int state;                                  ///< 0:idle 1:start
    int alarm_on;                               ///< current tamper alarm, 0:alarm 1:alarm release
    int md_path;                                ///< md working path, 0~3

    int less;                                   ///< less value for monitor
    int variance;                               ///< variance value for monitor
    int total;                                  ///< total value for monitor

    int sensitive_b_th;                         ///< tamper detection sensitive black threshold, 1 ~ 100, 0:for disable
    int sensitive_h_th;                         ///< tamper detection sensitive homogeneous threshold, 1 ~ 100, 0:for disable
    int histogram_idx;                          ///< tamper detection histogram index, 1 ~ 255

    u16 *weight_pre;                            ///< for record previous weight
    u8  *muY_pre;                               ///< for record previous muY
    u8  *nmode_pre;                             ///< for record previous nmode
};
#define VCAP_MD_TAMPER_STATE_IDLE   0
#define VCAP_MD_TAMPER_STATE_START  1

#define VCAP_PARAM_SYNC_COMM        BIT0
#define VCAP_PARAM_SYNC_SC          BIT1
#define VCAP_PARAM_SYNC_MD          BIT2
#define VCAP_PARAM_SYNC_SD          BIT3
#define VCAP_PARAM_SYNC_TC          BIT4
#define VCAP_PARAM_SYNC_OSD         BIT5        ///< BIT5 ~12 for osd  window#0~7
#define VCAP_PARAM_SYNC_MARK        BIT13       ///< BIT13~16 for mark window#0~3
#define VCAP_PARAM_SYNC_OSD_COMM    BIT17

#define VCAP_CH_HBLANK_DUMMY_PIXEL  16                               ///< Multiples of 4
#define VCAP_CH_HBLANK_BUF_SIZE     (VCAP_CH_HBLANK_DUMMY_PIXEL*2)   ///< YUV422
#define VCAP_CH_HBLANK_PATH         3                                ///< Use path#3

struct vcap_ch_t {
    u8                              active;                 ///< channel activation, 1: means this channel can used
    u8                              md_active;              ///< md activation, 1: means this channel md enable
    int                             md_grp_id;              ///< md group index of channel
    u32                             status;                 ///< channel current status

    struct semaphore                sema_lock;              ///< for semaphore lock

    VCAP_SC_ROLLING_T               sc_rolling;             ///< source crop rolling
    VCAP_SC_TYPE_T                  sc_type;                ///< source crop channel type

    struct vcap_path_t              path[VCAP_SCALER_MAX];  ///< path config

    struct vcap_dma_buf_t           md_gau_buf;             ///< md gaussian buffer
    struct vcap_dma_buf_t           md_event_buf;           ///< md event    buffer
    struct vcap_md_tamper_t         md_tamper;              ///< md tamper detection parameter

    struct timer_list               timer;                  ///< channel timeout timer
    struct vcap_timer_data_t        timer_data;             ///< channel timeout timer data

    struct vcap_ch_param_t          param;                  ///< current running parameter
    struct vcap_ch_param_t          temp_param;             ///< update parameter
    u32                             param_sync;             ///< update parameter sync flag

    int                             comm_path;              ///< for common register update path

    u32                             hw_count;               ///< record hardware total frame counter
    int                             hw_count_t0;            ///< record hardware t0 frame counter

    int                             skip_pixel_lack;        ///< Skip pixel lack error
    int                             skip_line_lack;         ///< Skip line  lack error

    struct vcap_dma_buf_t           hblank_buf;             ///< blanking buffer, buffer start address must 8 byte alignment for hardware DMA access
    int                             hblank_enb;             ///< grab hblank enable
    int                             do_md_gau_chk;          ///< enable md gaussian value check procedure for detect gaussian value update incorrect problem
};

#define VCAP_IS_CH_ENABLE(info, x)      (((x) == VCAP_CASCADE_CH_NUM) ? (VCAP_REG_RD(info, VCAP_GLOBAL_OFFSET(0x0c))&0x1) : ((VCAP_REG_RD(info, VCAP_GLOBAL_OFFSET(0x08))>>(x))&0x1))

#define VCAP_DEF_CH_TIMEOUT             1000    ///< 1 sec
#define VCAP_DEF_FATAL_RESET_TIMEOUT    1000    ///< 1 sec

/*************************************************************************************
 *  VCAP Diagnostic Structure
 *************************************************************************************/
struct vcap_dma_diag_t {
    u32 ovf;                ///< DMA overflow
    u32 job_cnt_ovf;        ///< DMA job count overflow
    u32 write_resp_fail;    ///< DMA write channel response fail
    u32 read_resp_fail;     ///< DMA read  channel response fail
    u32 cmd_prefix_err;     ///< DMA commad prefix error
    u32 write_blk_zero;     ///< DMA write channel block width equal 0
};

struct vcap_global_diag_t {
    u32                     sd_job_cnt_ovf;     ///< SD job count overflow
    u32                     md_miss_sts;        ///< MD miss statistics done
    u32                     md_job_cnt_ovf;     ///< MD job count overflow, need to reset md
    u32                     ll_id_mismatch;     ///< LLI channel id mismatch
    u32                     ll_cmd_too_late;    ///< LLI command load too late
    u32                     fatal_reset;        ///< software reset count
    u32                     md_reset;           ///< md reset count
    struct vcap_dma_diag_t  dma[2];
};

struct vcap_vi_diag_t {
    u32 no_clock;           ///< VI no clock
};

struct vcap_ch_diag_t {
    u32 vi_fifo_full;                   ///< VI fifo full
    u32 pixel_lack;                     ///< Pixel lack
    u32 line_lack;                      ///< Line lack

    u32 sd_timeout;                     ///< SD process fail(SD timeout), fatal ==> capture need software reset
    u32 sd_job_ovf;                     ///< SD process fail(SD job overflow), fatal ==> capture need software reset
    u32 sd_sc_mem_ovf;                  ///< SD process fail(SC memory overflow), fatal ==> capture need software reset
    u32 sd_line_lack;                   ///< SD process fail(line lack)
    u32 sd_pixel_lack;                  ///< SD process fail(Pixel lack)
    u32 sd_param_err;                   ///< SD process fail(SD parameter error)
    u32 sd_prefix_err;                  ///< SD process fail(SD prefix decode error)
    u32 sd_1st_field_err;               ///< SD process fail(1st field)

    u32 md_traffic_jam;                 ///< MD read traffic jam
    u32 md_gau_err;                     ///< MD gaussian value incorrect update

    u32 ll_sts_id_mismatch;             ///< LL status id mismatch
    u32 ll_dma_no_done;                 ///< LL status DMA not write done
    u32 ll_split_dma_no_done;           ///< LL status Split DMA not write done
    u32 ll_jump_table;                  ///< LL jump table update
    u32 ll_null_mismatch;               ///< LL Null table mismatch
    u32 ll_null_not_zero;               ///< LL Null table reserved byte not zero
    u32 ll_tab_lack[VCAP_SCALER_MAX];   ///< LL table not enough for command update

    u32 job_timeout;                    ///< Job timeout
    u32 no_job_alm[VCAP_SCALER_MAX];    ///< No Job alarm
};

struct vcap_diag_t {
    struct vcap_global_diag_t    global;
    struct vcap_vi_diag_t        vi[VCAP_VI_MAX];
    struct vcap_ch_diag_t        ch[VCAP_CHANNEL_MAX];
};

/*************************************************************************************
 *  VCAP Split Channel Structure
 *************************************************************************************/
#define VCAP_SPLIT_X_MAX             8
#define VCAP_SPLIT_CH_MAX            32
#define VCAP_SPLIT_CH_SCALER_MAX     2
#define VCAP_SPLIT_CH_DMA_MAX        2

struct vcap_split_t {
    int ch;      ///< split physical channel number
    u8  x_num;   ///< split x number
    u8  y_num;   ///< split y number
};

/*************************************************************************************
 *  VCAP Source Cropping Rule Definition
 *************************************************************************************/
#define VCAP_SC_RULE_MAX    3

struct vcap_sc_rule_t {
    int in_size;
    int out_size;
};

/*************************************************************************************
 *  VCAP Device Module Parameter Structure
 *************************************************************************************/
typedef enum {
    VCAP_VI_RUN_MODE_DISABLE = 0,
    VCAP_VI_RUN_MODE_BYPASS,
    VCAP_VI_RUN_MODE_2CH,
    VCAP_VI_RUN_MODE_4CH,
    VCAP_VI_RUN_MODE_SPLIT,
    VCAP_VI_RUN_MODE_MAX
} VCAP_VI_RUN_MODE_T;

typedef enum {
    VCAP_ACCS_FUNC_DISABLE = 0,
    VCAP_ACCS_FUNC_HALF_MARK_MEM,
    VCAP_ACCS_FUNC_FULL_MARK_MEM,
    VCAP_ACCS_FUNC_MAX
} VCAP_ACCS_FUNC_T;

/**********************************************************************************************
 =============================================================================================
 ACCS_HALF_MARK_MEM (GM8136)
 =============================================================================================
 Font_RAM             Display RAM         Mark RAM(1024x64bit)
 |----------|         |----------|        |--------------------------| -
 |  Font#0  |-------->|  Disp#0  |------->|  Disp#0 ACCS Data(32bit) | |
 |----------|         |----------|        |--------------------------| |
 |  Font#1  |-------->|  Disp#1  |------->|  Disp#1 ACCS Data(32bit) | |
 |----------|         |----------|        |--------------------------| |
 |  Font#2  |-------->|  Disp#2  |------->|  Disp#2 ACCS Data(32bit) | ACCS statistic Data
 |----------|         |----------|        |--------------------------| |
 |  Font#3  |-------->|  Disp#3  |------->|  Disp#3 ACCS Data(32bit) | |
 |----------|         |----------|        |--------------------------| |
 |    :     |         |    :     |        |             :            | |
 |----------|         |    :     |        |             :            | |
 | Font#454 |         |----------|        |--------------------------| |
 |----------|         | Disp#1023|------->|Disp#1023 ACCS Data(32bit)| |
                      |--------- |        |--------------------------| -> MARK_RAM_SIZE/2
                                          |             X            | |
                                          |             X            | |
                                          |             X            | |
                                          |             X            | Mark Image
                                          |             X            | |
                                          |             X            | |
                                          |             X            | |
                                          |--------------------------| -

 ==============================================================================================
 ACCS_FULL_MARK_MEM (GM8136)
 ==============================================================================================
 Font_RAM             Display RAM(1024x9Bit)       Mark RAM(1024x64bit)
 |----------|         |-------------------|        |----------------------------------------| -
 |  Font#0  |-------->|     Disp#0        |------->|  Disp#0   ACCS Data(32bit)             | |
 |----------|         |-------------------|        |----------------------------------------| |
 |  Font#1  |-------->|     Disp#1        |------->|  Disp#1   ACCS Data(32bit)             | |
 |----------|         |-------------------|        |----------------------------------------| |
 |  Font#2  |-------->|     Disp#2        |------->|  Disp#2   ACCS Data(32bit)             | |
 |----------|         |-------------------|        |----------------------------------------| |
 |    :     |         |       :           |        |             :                          | |
 |    :     |         |       :           |        |             :                          | |
 |----------|         |-------------------|        |----------------------------------------| |
 | Font#454 |         |     Disp#767      |        |  Disp#767 ACCS Data(32bit)             | |
 |----------|         |-------------------|        |----------------------------------------| -> MARK_RAM_SIZE/2
                      |  Disp_Global#0    |        |  Disp_Global#0-Res#0 ACCS Data(32bit)  | |
                      |  Disp_Global#1    |        |  Disp_Global#0-Res#1 ACCS Data(32bit)  | |
                      |       :           |        |  Disp_Global#0-Res#2 ACCS Data(32bit)  | |
                      |       :           |        |  Disp_Global#0-Res#3 ACCS Data(32bit)  | |
                      |  Disp_Global#255  |        |             :                          | |
                      |-------------------|        |             :                          | |
                                                   |             :                          | |
                                                   |             :                          | |
                                                   |  Disp_Global#255-Res#0 ACCS Data(32bit)| |
                                                   |  Disp_Global#255-Res#1 ACCS Data(32bit)| |
                                                   |  Disp_Global#255-Res#2 ACCS Data(32bit)| |
                                                   |  Disp_Global#255-Res#3 ACCS Data(32bit)| |
                                                   |----------------------------------------| -

**********************************************************************************************/

#define VCAP_EXTRA_IRQ_SRC_NONE         0x00000000
#define VCAP_EXTRA_IRQ_SRC_LL_DONE      0x00000001

#define VCAP_GRAB_FILTER_SD_JOB_OVF     (0x1<<0)
#define VCAP_GRAB_FILTER_SD_SC_MEM_OVF  (0x1<<1)
#define VCAP_GRAB_FILTER_SD_PARAM_ERR   (0x1<<2)
#define VCAP_GRAB_FILTER_SD_PREFIX_ERR  (0x1<<3)
#define VCAP_GRAB_FILTER_SD_TIMEOUT     (0x1<<4)
#define VCAP_GRAB_FILTER_SD_PIXEL_LACK  (0x1<<5)
#define VCAP_GRAB_FILTER_SD_LINE_LACK   (0x1<<6)

struct vcap_dev_module_param_t {
    VCAP_VI_RUN_MODE_T      vi_mode[VCAP_VI_MAX];           ///< vi running mode
    int                     vi_max_w[VCAP_VI_MAX];          ///< vi max width for sd line buffer calculation
    u8                      split_x;                        ///< split x number
    u8                      split_y;                        ///< split y number
    u8                      cap_md;                         ///< capture motion detection, 0:off 1:on
    int                     sync_time_div;                  ///< sync timer divide value
    u32                     ext_irq_src;                    ///< extra interrupt source
    u32                     grab_filter;                    ///< grab filter for fail frame drop
    VCAP_ACCS_FUNC_T        accs_func;                      ///< auto color change scheme function
    struct vcap_sc_rule_t   hcrop_rule[VCAP_SC_RULE_MAX];   ///< source horizontal cropping rule
    struct vcap_sc_rule_t   vcrop_rule[VCAP_SC_RULE_MAX];   ///< source vertical   cropping rule
    int                     vup_min;                        ///< vertical minimal scaling up line number
};

/*************************************************************************************
 *  VCAP Software Frame Rate Definition
 *************************************************************************************/
#define VCAP_FRAME_RATE_BASE    90
#define VCAP_FRAME_RATE_UNIT    (1000000/VCAP_FRAME_RATE_BASE)

/*************************************************************************************
 *  VCAP Heartbeat Timer Definition
 *************************************************************************************/
#define VCAP_HEARTBEAT_DEFAULT_TIMEOUT  0   ///< default is disable

/*************************************************************************************
 *  VCAP Device Information Structure
 *************************************************************************************/
#define VCAP_DEV_MSG_BUF_SIZE   CFG_MBUF_SIZE
#define VCAP_DEV_MSG_THRESHOLD  1024

typedef enum {
    VCAP_DEV_MODE_SSF = 0,
    VCAP_DEV_MODE_LLI,
    VCAP_DEV_MODE_CONTINUE,
    VCAP_DEV_MODE_MAX
} VCAP_DEV_MODE_T;

typedef enum {
    VCAP_DEV_STATUS_IDLE = 0,
    VCAP_DEV_STATUS_INITED,
    VCAP_DEV_STATUS_START,
    VCAP_DEV_STATUS_STOP
} VCAP_DEV_STATUS_T;

/*
 *  Device Status => pdev_info->status
 *  ------------------------------------------------------------------------------------------------------
 *  [31 ~ 28] [27 ~ 4] [3 ~ 0]
 *    Global  reserved   Dev
 *
 *  VI Status => pdev_info->vi[x].status
 *  ------------------------------------------------------------------------------------------------------
 *  [31 ~ 28] [27 ~ 4] [3 ~ 0]
 *    Global  reserved   VI
 *
 *  CH Status => pdev_info->ch[x].status
 *  ------------------------------------------------------------------------------------------------------
 *  [31 ~ 20] [19 ~ 16] [15 ~ 12] [11 ~ 8] [7  ~ 4] [3  ~ 0]
 *  reserved   Path#3    Path#2    Path#1   Path#0     CH
 */
#define SET_DEV_GST(pinfo, va)              (pinfo->status = (pinfo->status & (~(0xf<<28))) | (((va)&0xf)<<28))
#define SET_DEV_ST(pinfo, va)               (pinfo->status = (pinfo->status & (~0xf)) | ((va)&0xf))
#define SET_DEV_VI_GST(pinfo, v, va)        (pinfo->vi[v].status = (pinfo->vi[v].status & (~(0xf<<28))) | (((va)&0xf)<<28))
#define SET_DEV_VI_ST(pinfo, v, va)         (pinfo->vi[v].status = (pinfo->vi[v].status & (~0xf)) | ((va)&0xf))
#define SET_DEV_CH_ST(pinfo, c, va)         (pinfo->ch[c].status = (pinfo->ch[c].status & (~0xf)) | ((va)&0xf))
#define SET_DEV_CH_PATH_ST(pinfo, c, p, va) do {\
                                                pinfo->ch[c].status = (pinfo->ch[c].status & (~(0xf<<(((p)*4)+4)))) | (((va)&0xf)<<(((p)*4)+4));\
                                                vcap_dev_update_status(pinfo, c);\
                                            } while(0)

#define GET_DEV_GST(pinfo)                  ((pinfo->status>>28) & 0xf)
#define GET_DEV_ST(pinfo)                   (pinfo->status & 0xf)
#define GET_DEV_VI_GST(pinfo, v)            ((pinfo->vi[v].status>>28) & 0xf)
#define GET_DEV_VI_ST(pinfo, v)             (pinfo->vi[v].status & 0xf)
#define GET_DEV_CH_ST(pinfo, c)             (pinfo->ch[c].status & 0xf)
#define GET_DEV_CH_PATH_ST(pinfo, c, p)     ((pinfo->ch[c].status>>(((p)*4)+4)) & 0xf)

#define IS_DEV_BUSY(pinfo)                  (GET_DEV_ST(pinfo) == VCAP_DEV_STATUS_START)
#define IS_DEV_VI_BUSY(pinfo, v)            (GET_DEV_VI_ST(pinfo, v) == VCAP_DEV_STATUS_START)
#define IS_DEV_CH_BUSY(pinfo, c)            (GET_DEV_CH_ST(pinfo, c) == VCAP_DEV_STATUS_START)
#define IS_DEV_CH_PATH_BUSY(pinfo, c, p)    (GET_DEV_CH_PATH_ST(pinfo, c, p) == VCAP_DEV_STATUS_START)

typedef enum {
    VCAP_DEV_DRV_TYPE_SSF = 0,
    VCAP_DEV_DRV_TYPE_LLI,
} VCAP_DEV_DRV_TYPE_T;

struct vcap_dev_info_t {
    int                             index;                      ///< device index
    u32                             vbase;                      ///< virtual address of vcap device
    int                             irq;                        ///< vcap device irq number
    VCAP_DEV_DRV_TYPE_T             dev_type;                   ///< device running type
    u32                             status;                     ///< device status
    spinlock_t                      lock;                       ///< for interrupt lock
    struct semaphore                sema_lock;                  ///< for semaphore lock
    struct vcap_dev_module_param_t  *m_param;                   ///< device module parameter
    struct vcap_dev_capability_t    capability;                 ///< vcap device hardware capability

    VCAP_CH_MODE_T                  ch_mode;                    ///< device channel running mode
    struct vcap_vi_t                vi[VCAP_VI_MAX];            ///< vi data
    struct vcap_ch_t                ch[VCAP_CHANNEL_MAX];       ///< channel data
    struct vcap_lli_t               lli;                        ///< lli data
    struct vcap_split_t             split;                      ///< split data

    struct vcap_dma_buf_t           md_buf;                     ///< md buffer, for gaussian and event statistic
    struct task_struct              *md_task;                   ///< md task for tamper detection

    struct vcap_diag_t              diag;                       ///< Error Diagnostic
    struct vcap_vi_probe_t          vi_probe;                   ///< vi signal probe

    struct vcap_dma_buf_t           blank_buf;                  ///< blanking buffer

    void                            *msg_buf;                   ///< buffer for debug message
    int                             dbg_mode;                   ///< debug mode for disable/enable error message printout in interrupt handler
    u32                             dev_fatal;                  ///< device fatal error, must do software reset
    u32                             dev_md_fatal;               ///< device md fatal error(md job count overflow), must disable all channel md and do md reset
    u32                             fatal_time;                 ///< fatal error reset timeout timer
    int                             do_md_reset;                ///< trigger md engine reset
    int                             md_enb_grp;                 ///< md group index of md enable channel
    int                             md_grp_max;                 ///< md max group
    int                             dma_ovf_thres[2];           ///< DMA overflow threshold, will trigger capture fatal reset if threshold over sync_time_div
    u32                             sync_time_unit;             ///< sync timer time unit

    struct timer_list               hb_timer;                   ///< heartbeat timer for monitor capture hardware status
    int                             hb_timeout;                 ///< heartbeat timer timeout second
    u32                             hb_count;                   ///< heartbeat timer count

    int  (*init)(struct vcap_dev_info_t *pdev_info);
    int  (*start)(struct vcap_dev_info_t *pdev_info, int ch, int path);
    int  (*stop)(struct vcap_dev_info_t *pdev_info, int ch, int path);
    void (*reset)(struct vcap_dev_info_t *pdev_info);
    int  (*trigger)(struct vcap_dev_info_t *pdev_info);
    void (*fatal_stop)(struct vcap_dev_info_t *pdev_info);
    void (*deconstruct)(struct vcap_dev_info_t *pdev_info);
};

/*************************************************************************************
 *  Public Function Prototype
 *************************************************************************************/
void vcap_dev_set_dev_info(int id, void *info);
void *vcap_dev_get_dev_info(int id);
void *vcap_dev_get_dev_proc_root(int id);
void vcap_dev_config_hw_capability(struct vcap_dev_info_t *pdev_info);
int  vcap_dev_proc_init(int id, const char *name, void *private);
void vcap_dev_proc_remove(int id);
void vcap_dev_set_reg_default(struct vcap_dev_info_t *pdev_info);
void vcap_dev_reset(struct vcap_dev_info_t *pdev_info);
void vcap_dev_demux_reset(struct vcap_dev_info_t *pdev_info, int vi);
int  vcap_dev_get_vi_mode(void *param, int ch);
int  vcap_dev_stop(void *param, int ch, int path);
int  vcap_dev_putjob(void *param);
void *vcap_dev_getjob(struct vcap_dev_info_t *pdev_info, int ch, int path);
int  vcap_dev_vi_setup(struct vcap_dev_info_t *pdev_info, int vi_idx);
int  vcap_dev_vi_clock_invert(struct vcap_dev_info_t *pdev_info, int vi_idx, int clk_invert);
int  vcap_dev_vi_grab_hblank_setup(struct vcap_dev_info_t *pdev_info, int vi_idx, int grab_on);
int  vcap_dev_vi_ch_param_update(struct vcap_dev_info_t *pdev_info, int vi_idx, int vi_ch);
void vcap_dev_set_sc_default(struct vcap_dev_info_t *pdev_info, int ch);
void vcap_dev_global_setup(struct vcap_dev_info_t *pdev_info);
void vcap_dev_rgb2yuv_matrix_init(struct vcap_dev_info_t *pdev_info);
void vcap_dev_sync_timer_init(struct vcap_dev_info_t *pdev_info);
void vcap_dev_update_status(struct vcap_dev_info_t *pdev_info, int vi_idx);
void vcap_dev_capture_enable(struct vcap_dev_info_t *pdev_info);
void vcap_dev_capture_disable(struct vcap_dev_info_t *pdev_info);
void vcap_dev_ch_enable(struct vcap_dev_info_t *pdev_info, int ch);
void vcap_dev_ch_disable(struct vcap_dev_info_t *pdev_info, int ch);
void vcap_dev_single_fire(struct vcap_dev_info_t *pdev_info);
int  vcap_dev_param_set_default(struct vcap_dev_info_t *pdev_info, int vi_idx);
int  vcap_dev_param_setup(struct vcap_dev_info_t *pdev_info, int ch, int path, void *param);
void vcap_dev_param_sync(struct vcap_dev_info_t *pdev_info, int ch, int path);
int  vcap_dev_sc_setup(struct vcap_dev_info_t *pdev_info, int ch);
int  vcap_dev_get_offset(struct vcap_dev_info_t *pdev_info, int ch, int path, void *param, int buf_num, u32 *start_addr, u32 *line_offset, u32 *field_offset);
int  vcap_dev_set_ch_flip(struct vcap_dev_info_t *pdev_info, int ch, int v_flip, int h_flip);
int  vcap_dev_get_ch_flip(struct vcap_dev_info_t *pdev_info, int ch, int *v_flip, int *h_flip);
int  vcap_dev_set_frame_cnt_monitor_ch(struct vcap_dev_info_t *pdev_info, int ch, int clear);
int  vcap_dev_get_frame_cnt_monitor_ch(struct vcap_dev_info_t *pdev_info);
int  vcap_dev_signal_probe(struct vcap_dev_info_t *pdev_info);
int  vcap_blank_buf_alloc(struct vcap_dev_info_t *pdev_info);
void vcap_blank_buf_free(struct vcap_dev_info_t *pdev_info);
void vcap_dev_heartbeat_handler(unsigned long data);

#endif  /* _VCAP_DEV_H_ */
