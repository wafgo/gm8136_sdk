#ifndef _VG_VERIFY_H_
#define _VG_VERIFY_H_

#include "test_api.h"   ///< from videograph em

struct job_cfg_t {
    int enable;         ///< job test enable
    int round_set;      ///< job running count
    int type;           ///< job type, cap:0x10
    int engine;         ///< engine number, channel
    int minor;          ///< minor  number, path
    int split_ch;       ///< split channel number
    int lcd_fb_idx;     ///< lcd index
    struct video_param_t param[MAX_PROPERTYS];  ///< job property
};

struct buf_cfg_t {
    int round_set;          ///< config running count
    int in_buf_ddr;         ///< input buffer ddr index
    int in_buf_type;        ///< input buffer type => field, frame 2frame, 2frame_frame
    int in_size;            ///< input buffer size
    int out_buf_ddr;        ///< output buffer ddr index
    int out_buf_type;       ///< output buffer type => field, frame 2frame, 2frame_frame
    int out_size;           ///< output buffer size
    int out2_size;          ///< output buffer2 size for 2frames_2frames and frame_2frames type
    int multi_job_pack_num; ///< multi-job job pack number
    int split_multi_job;    ///< multi-job for split channel, 0: normal 1:split job
    int ch_from_vg_info;    ///< parse channel index from vg_info proc node
};

#define VG_INFO_ENTITY_MAX  33
struct vg_info_t {
    int valid;
    int vch;
    int vcap_ch;
    int n_split;
    int n_path;
    int vi_mode;
    int fd_start;
    int width;
    int height;
};

/*************************************************************************************
 *  LCD Base Address
 *************************************************************************************/
#if defined(PLATFORM_GM8210)
#define LCD0_BASE_ADDR  0x9A800000
#define LCD1_BASE_ADDR  0x9B200000
#define LCD2_BASE_ADDR  0x9B800000
#define LCD_REG_SIZE    0x1000
#define LCD_FB_ADDR_REG 0x18
#elif defined(PLATFORM_GM8287)
#define LCD0_BASE_ADDR  0x9A800000
#define LCD1_BASE_ADDR  0x00000000
#define LCD2_BASE_ADDR  0x9B800000
#define LCD_REG_SIZE    0x1000
#define LCD_FB_ADDR_REG 0x18
#elif defined(PLATFORM_GM8139)
#define LCD0_BASE_ADDR  0x9BF00000
#define LCD1_BASE_ADDR  0x00000000
#define LCD2_BASE_ADDR  0x00000000
#define LCD_REG_SIZE    0x1000
#define LCD_FB_ADDR_REG 0x18
#elif defined(PLATFORM_GM8136)
#define LCD0_BASE_ADDR  0x9BF00000
#define LCD1_BASE_ADDR  0x00000000
#define LCD2_BASE_ADDR  0x00000000
#define LCD_REG_SIZE    0x1000
#define LCD_FB_ADDR_REG 0x18
#elif defined(PLATFORM_A369)
#define LCD0_BASE_ADDR  0xC1300000
#define LCD1_BASE_ADDR  0x00000000
#define LCD2_BASE_ADDR  0x00000000
#define LCD_REG_SIZE    0x1000
#define LCD_FB_ADDR_REG 0x1C
#else
#define LCD0_BASE_ADDR  0x00000000
#define LCD1_BASE_ADDR  0x00000000
#define LCD2_BASE_ADDR  0x00000000
#define LCD_REG_SIZE    0x1000
#define LCD_FB_ADDR_REG 0x00
#endif

#endif /* _VG_VERIFY_H_ */
