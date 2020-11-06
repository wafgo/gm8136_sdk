#ifndef _VCAP_VG_H_
#define _VCAP_VG_H_

#include "video_entity.h"

#define VCAP_VG_ENTITY_MAX      1

/*
 * Job FD Format
 * MSB                                          LSB
 * *----------------------------------------------*
 * | VI_Mode |    CH#     |   Split_CH#   | Path# |
 * *----------------------------------------------*
 * |------> Engine <------|-------> Minor <-------|
 *
 * VI_Mode  : bits[15~14]
 * CH#      : bits[13~8]
 * Split_CH#: bits[7~2]
 * Path#    : bits[1~0]
 *
 */
#define VCAP_VG_FD_ENGINE(vimode, ch)           (((vimode&0x3)<<6)|(ch&0x3f))
#define VCAP_VG_FD_MINOR(s_ch, path)            (((s_ch&0x3f)<<2)|(path&0x3))
#define VCAP_VG_FD(vimode, ch, s_ch, path)      ENTITY_FD(TYPE_CAPTURE, VCAP_VG_FD_ENGINE(vimode, ch), VCAP_VG_FD_MINOR(s_ch, path))
#define VCAP_VG_FD_ENGINE_VIMODE(x)             ((x>>6)&0x3)
#define VCAP_VG_FD_ENGINE_CH(x)                 (x&0x3f)
#define VCAP_VG_FD_MINOR_SPLITCH(x)             ((x>>2)&0x3f)
#define VCAP_VG_FD_MINOR_PATH(x)                (x&0x3)
#define VCAP_VG_MAKE_IDX(info, ch, path)        ((info->entity.minors*ch)+path)
#define VCAP_VG_FD_SKIPJOB_TAG                  0xFFFFFFFF

typedef enum {
    VCAP_VG_VIMODE_BYPASS = 0,
    VCAP_VG_VIMODE_2CH,
    VCAP_VG_VIMODE_4CH,
    VCAP_VG_VIMODE_SPLIT,
    VCAP_VG_VIMODE_MAX
} VCAP_VG_VIMODE_T;

#define VCAP_VG_PROP_CAP_FEATURE_P2I_BIT        0x00000001       ///< cap_feature property bit#0
#define VCAP_VG_PROP_CAP_FEATURE_PROG_2FRM_BIT  0x00000002       ///< cap_feature property bit#1

#define VCAP_VG_PROP_SPEC_FUN_FRM_60I_BIT       0x00000001       ///< 1frame mode grab top/bottom field to 2frame format

struct vcap_vg_job_param_t {
    int          ch;                 ///< channel number
    int          path;               ///< path number
    int          split_ch;           ///< split channel number
    int          grab_num;           ///< grab num of this job
    int          job_fmt;            ///< job format => field, frame, 2frame, 2frames_2frames, frame_2frames
    int          src_fmt;            ///< not used in capture
    int          src_xy;             ///< start position of crop region
    int          src_bg_dim;         ///< width and height of source signal
    int          src_dim;            ///< width and height of crop region
    int          src_interlace;      ///< 0: progressive, 1: interlace
    int          src_field;          ///< 0: top,         1: bottom
    int          src_type;           ///< 0: generic      1: isp
    int          dst_fmt;            ///< output format => 0:field, 1: frame, 2: 2frame
    int          dst_xy;             ///< start position of output image
    int          dst_xy_offset;      ///< start position offset of output image if auto_aspect_ratio enable
    int          dst_bg_dim;         ///< width and height of output background frame buffer
    int          dst_dim;            ///< width and height of output image
    int          dst_crop_xy;        ///< start position of output crop region
    int          dst_crop_dim;       ///< width and height of output crop region
    int          dst2_xy;            ///< start position of output background frame buffer#2
    int          dst2_dim;           ///< width and height of output image#2
    int          dst2_bg_dim;        ///< width and height of output background frame buffer#2
    int          dst2_crop_xy;       ///< start position of output image#2 crop region
    int          dst2_crop_dim;      ///< width and height of output image#2 crop region
    int          dst2_fd;            ///< specify which engine-minor to output image to frame buffer#2
    int          dst2_buf_offset;    ///< buffer offset of background frame buffer#2 from frame buffer#1
    int          src_frame_rate;     ///< source frame rate
    int          target_frame_rate;  ///< target output frame rate
    int          fps_ratio;          ///< construction of frame rate (denomerator<<16|numerator)
    int          runtime_frame_rate; ///< calculated real frame rate value
    int          p2i;                ///< 0: disable, 1: enable
    int          prog_2frm;          ///< 2frame mode output progressive top/bottom, only support on interlace source
    int          frm_2frm_field;     ///< 1frame mode grab top/bottom field to 2frame format in different buffer, one job grab one field
    int          dma_ch;             ///< 0: dma ch#0, 1: dma ch#1
    int          auto_aspect_ratio;  ///< auto aspect ratio of output image => (palette_idx << 16 | enable)
    int          auto_border;        ///< auto border of output image => (palette_idx << 16 | border_width << 8 | enable)
    int          md_enb;             ///< md control [31:16]=>md source path 0~3, [15:0]=>md enable/disable
    int          md_xy_start;        ///< md x, y start position [31:16]=> x start, [15:0]=> y start
    int          md_xy_size;         ///< md x, y window size [31:16]=> x size, [15:0]=> y size
    int          md_xy_num;          ///< md x, y window number [31:16]=> x number, [15:0]=> y number
    unsigned int addr_va[2];         ///< 0: for top field, 1: for bottom field
    unsigned int addr_pa[2];         ///< 0: for top field, 1: for bottom field
    int          out_size[2];        ///< output size
    void         *data;              ///< pointer to job_item of this param
};

typedef enum {
    VCAP_VG_JOB_TYPE_NORMAL = 0,
    VCAP_VG_JOB_TYPE_MULTI,
    VCAP_VG_JOB_TYPE_MAX
} VCAP_VG_JOB_TYPE_T;

typedef enum {
    VCAP_VG_JOB_STATUS_STANDBY = 1,
    VCAP_VG_JOB_STATUS_ONGOING,
    VCAP_VG_JOB_STATUS_FINISH,
    VCAP_VG_JOB_STATUS_FAIL,
    VCAP_VG_JOB_STATUS_KEEP
} VCAP_VG_JOB_STATUS_T;

typedef enum {
    VCAP_VG_GRP_RUN_MODE_NORMAL = 0,            ///< not pending new version group job
    VCAP_VG_GRP_RUN_MODE_WAIT_OLD_VER_DONE,     ///< pending new version group job to wait old version job finish
} VCAP_VG_GRP_RUN_MODE_T;

struct vcap_vg_grp_pool_t {
    int               id;           ///< group id
    int               ver;          ///< group running version
    struct list_head  job_head;     ///< job list head
    struct list_head  pool_list;    ///< next group pool list
};

struct vcap_vg_job_item_t {
    void                        *info;          ///< pointer to vcap_vg_info_t
    int                         engine;         ///< fd engine field value
    int                         minor;          ///< fd minor  field value
    int                         job_id;         ///< job->id
    int                         job_ver;        ///< job->ver
    void                        *job;           ///< videograph job
    void                        *root_job;      ///< root job of multi-job
    VCAP_VG_JOB_TYPE_T          type;           ///< job type
    VCAP_VG_JOB_STATUS_T        status;         ///< job current status
    struct list_head            engine_list;    ///< use to add engine_head
    struct list_head            minor_list;     ///< use to add minor_head
    struct list_head            m_job_list;     ///< use to add m_job_head
    struct list_head            m_job_head;     ///< multi-job list head
    struct list_head            group_list;     ///< group job list, link to root job to group pool
    int                         need_callback;  ///< need to callback root_job
    struct vcap_vg_job_param_t  param;          ///< job parameter
    unsigned long long          timestamp;      ///< job callback timestamp(jiffies)
    unsigned int                puttime;        ///< putjob time, for performance statistic
    unsigned int                starttime;      ///< start engine time, for performance statistic
    unsigned int                finishtime;     ///< finish engine time, for performance statistic
    void                        *private;       ///< private data, pointer to vcap_dev_info
    struct vcap_vg_job_item_t   *subjob;        ///< pointer to sub job_item
};

typedef enum {
    VCAP_VG_NOTIFY_NO_JOB = 0,                  ///< channel job not enough
    VCAP_VG_NOTIFY_HW_DELAY,                    ///< channel timeout
    VCAP_VG_NOTIFY_FRAME_DROP,                  ///< frame drop, not used in capture
    VCAP_VG_NOTIFY_SIGNAL_LOSS,                 ///< video signal loss
    VCAP_VG_NOTIFY_SIGNAL_PRESENT,              ///< video signal present
    VCAP_VG_NOTIFY_FRAMERATE_CHANGE,            ///< frame rate changed
    VCAP_VG_NOTIFY_HW_CONFIG_CHANGE,            ///< resolution changed
    VCAP_VG_NOTIFY_TAMPER_ALARM,                ///< md tamper alarm
    VCAP_VG_NOTIFY_TAMPER_ALARM_RELEASE,        ///< md tamper alarm release
    VCAP_VG_NOTIFY_MAX
} VCAP_VG_NOTIFY_T;

#if 0 //allocation from videograph
#include "ms/ms_core.h"
#define vcap_vg_alloc(x) ms_small_fast_alloc(x)
#define vcap_vg_free(x)  ms_small_fast_free(x)
#else
#define vcap_vg_alloc(x) kmalloc(x, GFP_ATOMIC)
#define vcap_vg_free(x)  kfree(x)
#endif

/*************************************************************************************
 *  Public Function Prototype
 *************************************************************************************/
int  vcap_vg_init(int id, char *name, int engine_num, int minor_num, void *private_data);
void vcap_vg_close(int id);
void vcap_vg_mark_engine_start(void *param);
void vcap_vg_mark_engine_finish(void *param);
void vcap_vg_trigger_callback_fail(void *param);
void vcap_vg_trigger_callback_finish(void *param);
void vcap_vg_panic_dump_job(void);
int  vcap_vg_notify(int id, int ch, int path, VCAP_VG_NOTIFY_T ntype);

#endif  /* _VCAP_VG_H_ */