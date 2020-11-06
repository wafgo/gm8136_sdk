/* Header file for Video Entity Driver */
#ifndef _VIDEO_ENTITY_H_
#define _VIDEO_ENTITY_H_

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/rbtree.h>

#include "common.h"
#include "job.h"

enum ve_wellknown_property_id {
    ID_NULL                 =0,

    /* source ID */
    ID_SRC_FMT              =1,     //image format, refer to "enum buf_type_t"
    ID_SRC_XY               =2,     //construction of (x << 16 | y)
    ID_SRC_BG_DIM           =3,     //construction of (width << 16 | height)
    ID_SRC_DIM              =4,     //construction of (width << 16 | height)
                                    //0 or "not defined" means use same value of ID_SRC_BG_DIM
    ID_SRC_INTERLACE        =5,     //indicate source is interlace or progressive frame
                                    //0 or "not defined" means progressive frame
    ID_SRC_TYPE             =6,     //0: Generic 1: ISP
    ID_ORG_DIM              =7,     //cap org input dim

    /* destionation ID */
    ID_DST_FMT              =11,    //image format, refer to "enum buf_type_t"
    ID_DST_XY               =12,    //construction of (x<<16|y)
    ID_DST_BG_DIM           =13,    //construction of (width<<16|height)
    ID_DST_DIM              =14,    //construction of (width<<16|height)
                                    //0 means use same value of ID_DST_BG_DIM

    /* CAP and SCL */
    ID_DST_CROP_XY          =15,    //construction of (x << 16 | y)
    ID_DST_CROP_DIM         =16,    //construction of (width << 16 | height)
    /* win ID */
    ID_WIN_XY               =17,    //construction of (x << 16 | y)
    ID_WIN_DIM              =18,    //construction of (width << 16 | height)
                                    //0 means use same value of ID_DST_BG_DIM

    /* frame rate */
    ID_FPS_RATIO            =20,    //construction of frame rate (denomerator<<16|numerator)
    ID_TARGET_FRAME_RATE    =21,    //target frame rate value
    ID_SRC_FRAME_RATE       =22,    //source frame rate value
    ID_RUNTIME_FRAME_RATE   =23,    //calculated real frame rate value
    ID_FAST_FORWARD         =24,    //skip frame encoding for decode
    ID_REF_FRAME            =25,    //the frame is reference or not, by return of encode

    /* crop re-calculate */
    ID_CROP_XY              =26,    //for crop re-calculate, em internal-use only
    ID_CROP_DIM             =27,    //for crop re-calculate, em internal-use only
    ID_MAX_DIM              =28,    //for crop re-calculate, em internal-use only
    ID_IS_CROP_ENABLE       =29,    //for crop re-calculate, em internal-use only

    /* functional input & result  */
    ID_DI_RESULT            =30,    //Indicate Deinterlaced function result
                                    //0:Nothing 1:Deinterlace 2:Copy line 3:denoise 4:Copy Frame
    ID_SUB_YUV              =31,    //Indicate if sub ratio YUV exist
                                    //0:Not Exist 1: Exist
    ID_SCL_FEATURE          =32,    //Indicate Scaler Function
                                    //0:Nothing 1:Scaling 2:Progress corection
    ID_SCL_RESULT           =33,    //0:Nothing 1:choose 1st src yuv  2:choose 2nd src yuv (RATIO_FRAME)
    ID_DVR_FEATURE          =34,    //0:liveview 1:recording 
    ID_GRAPH_RATIO          =35,    //graph ratio for internal usage
    ID_IDENTIFIER           =36,    //identify who i am when other parameters can't do it
                                    // (used for audio dataout)
    ID_CHANGE_SRC_INTERLACE =37,    //tell 3di change ID_SRC_INTERLACE = 0, after finish 3DI
    ID_DIDN_MODE            =38,    //deInterlace and denoise 
    ID_CLOCKWISE            =39,
    
    /* bitstream */
    ID_BITSTREAM_SIZE       =40,    //bitstream size
    ID_BITRATE              =41,    //bitrate value
    ID_IDR_INTERVAL         =43,    //IP interval
    ID_B_FRM_NUM            =44,    //B frame number
    ID_SLICE_TYPE           =45,    //0:P frame  1:B frame   2:I frame   5:Audio
    ID_QUANT                =46,    //quant value
    ID_INIT_QUANT           =47,    //initial quant value
    ID_MAX_QUANT            =48,    //max quant value
    ID_MIN_QUANT            =49,    //min quant value
    
    ID_MAX_RECON_DIM        =50,    //max recon dimentation, construction of (x << 16|y)
    ID_MAX_REFER_DIM        =51,    //max refer dimentation, construction of (x << 16|y)
    ID_MOTION_ENABLE        =52,    // motion detection enable
    ID_BITSTREAM_OFFSET     =53,    //It's the offset of motion_vector for getting the position of real bit-stream.
    ID_CHECKSUM             =54,    //checksum type

    /* aspect ratio & border */
    ID_AUTO_ASPECT_RATIO    =55,    //construction of (palette_idx << 16 | enable)
    ID_AUTO_BORDER          =56,    //construction of (palette_idx << 16 | border_width << 8 | enable)
    /* aspect ratio & border can pass to scaler*/
    ID_AUTO_ASPECT_RATIO_SCL    =57,    //construction of (palette_idx << 16 | enable)
    ID_AUTO_BORDER_SCL          =58,    //construction of (palette_idx << 16 | border_width << 8 | enable)
    
    ID_AU_SAMPLE_RATE       =60,    //audio sample rate
    ID_AU_CHANNEL_TYPE      =61,    //audio channel type
    ID_AU_ENC_TYPE          =62,    //audio encode type
    ID_AU_RESAMPLE_RATIO    =63,    //audio resample ratio
    ID_AU_SAMPLE_SIZE       =64,    //audio sample size
    ID_AU_DATA_LENGTH_0     =65,    //audio data length of encode type 0
    ID_AU_DATA_LENGTH_1     =66,    //audio data length of encode type 1
    ID_AU_DATA_OFFSET_0     =67,    //audio data offset of encode type 0
    ID_AU_DATA_OFFSET_1     =68,    //audio data offset of encode type 1
    ID_AU_CHANNEL_BMP       =69,    //bitmap of active audio channel
    ID_AU_BLOCK_SIZE        =70,    //audio block size
    ID_AU_BLOCK_COUNT       =71,    //audio block count

    ID_SRC2_XY              =72,    //construction of (x << 16 | y)
    ID_SRC2_DIM             =73,    //construction of (width << 16 | height)
    ID_DST2_CROP_XY         =74,    //construction of (x << 16 | y)
    ID_DST2_CROP_DIM        =75,    //construction of (width << 16 | height)    
    ID_DST2_BG_DIM          =76,    //construction of (width<<16|height)
    ID_SRC2_BG_DIM          =77,    //construction of (width<<16|height)
    ID_DST2_XY              =78,    //construction of (x<<16|y)
    ID_DST2_DIM             =79,    //construction of (width<<16|height)
    ID_DST2_FD              =80,    //special for dst2_bg_dim usage

    /* win ID */
    ID_WIN2_BG_DIM          =81,
    ID_WIN2_XY              =82,    //construction of (x << 16 | y)
    ID_WIN2_DIM             =83,    //construction of (width << 16 | height)
    ID_CAS_SCL_RATIO        =84,    //cas_scl_ratio

    /* DEC PB */
    ID_DEC_SPEED            =85,    //PB speed
    ID_DEC_DIRECT           =86,    //PB forward/backward
    ID_DEC_GOP              =87,    //PB GOP
    ID_DEC_OUT_GROUP_SIZE   =88,    //PB output group size

    /* Audio properties (part-II)*/
    ID_AU_FRAME_SAMPLES     =90,    //audio frame samples

    /* CVBS offset */
    ID_DST2_BUF_OFFSET      =91,    //CVBS offset for continued buffer

    /* Multi slice offset 1~3 (first offset is 0)*/
    ID_SLICE_OFFSET1        =92,
    ID_SLICE_OFFSET2        =93,
    ID_SLICE_OFFSET3        =94,

    /* Speicial prpoerty */
    ID_BUF_CLEAN            =95,
    ID_VG_SERIAL            =96,
    ID_SPEICIAL_FUNC        =97,  //bit0: 1FRAME_60I_FUNC
    ID_SOURCE_FIELD         =98,  // 0: top field, 1: bottom field

    ID_WELL_KNOWN_MAX = MAX_WELL_KNOWN_ID_NUM,
};

#define MAKE_IDX(minors, engine, minor)     ((engine * minors) + minor)
#define IDX_ENGINE(idx, minors)             (idx / minors)
#define IDX_MINOR(idx, minors)              (idx % minors)

/* defination of property value of src_xy, dst_xy... */
#define EM_PARAM_X(value)           (((unsigned int)(value) >> 16) & 0x0000FFFF)
#define EM_PARAM_Y(value)           (((unsigned int)(value)) & 0x0000FFFF)
#define EM_PARAM_XY(x, y)           (((((unsigned int)x) << 16) & 0xFFFF0000) | \
                                    (((unsigned int)y) & 0x0000FFFF))

/* defination of property value of src_dim, dst_dim... */
#define EM_PARAM_WIDTH(value)       EM_PARAM_X(value)
#define EM_PARAM_HEIGHT(value)      EM_PARAM_Y(value)
#define EM_PARAM_DIM(width, height) EM_PARAM_XY(width, height)

/* defination of property value of fps_ratio = M/N */
#define EM_PARAM_M(value)           EM_PARAM_X(value)
#define EM_PARAM_N(value)           EM_PARAM_Y(value)
#define EM_PARAM_RATIO(m, n)        EM_PARAM_XY(m, n)


/* video entity */
struct video_entity_ops_t {
    int (*putjob)(void *);  //struct video_job_t *
    int (*stop)(void *, int, int);  //NULL,engine,minor
    int (*queryid)(void *, char *);  //NULL, property string
    int (*querystr)(void *, int, char *); //struct video_entity_t *,id, property string
    int (*getproperty)(void *, int, int, char *);  //NULL,engine,minor,string
    int (*register_clock)(void *);
    //< NULL, func(ms_ticks,is_update,private), private
    int reserved1[4];
};

//for buffer allocate by entity
enum buff_purpose {
    //for decode/encode
    VG_BUFF_DECODE,
    VG_BUFF_ENCODE,
    VG_BUFF_AU_DECODE,
    VG_BUFF_AU_ENCODE
};



#define MAX_NAME_LEN        30
#define MAX_CHANNELS        EM_VCH_MAX_VALUE

struct video_entity_t {
    /* fill by Video Entity */
    enum entity_type_t          type;            //TYPE_H264E,TYPE_MPEG4E...
    char                        name[MAX_NAME_LEN];
#define MAX_ENGINES       64
    int                         engines;     //number of devices
#define MAX_MINORS        128
    int                         minors;    //channels per device

    struct video_entity_ops_t   *ops;
    int                         reserved1[4];

    /* internal use */
    char                        ch_used[MAX_CHANNELS]; //used by dev_ch_used[devices][ch]
    void                        *func;
    struct list_head            entity_list;
    struct list_head            param_head[MAX_CHANNELS]; 
    struct rb_root              root_node;
    void                        *entity_data;
    int                         reserved2[7];
};

struct reserved_buf_info_t {
    struct list_head    buf_info_list;
    char                entity_name[MAX_NAME_LEN];
    unsigned int        job_id;
    unsigned int        buf0_addr_va;
    unsigned int        buf0_addr_pa;
    unsigned int        buf1_addr_va;
    unsigned int        buf1_addr_pa;
    int                 dir;
};

/* video entity */
void video_entity_init(void);
void video_entity_close(void);
int video_entity_register(struct video_entity_t *entity);
int video_entity_deregister(struct video_entity_t *entity);
int video_preprocess(struct video_entity_job_t *ongoing_job, void *param);
int video_postprocess(struct video_entity_job_t *finish_job, void *param);

void *video_reserve_buffer(struct video_entity_job_t *job, int dir);
int video_free_buffer(void *rev_handle);
int video_alloc_buffer(struct video_entity_job_t *job, int dir, struct video_bufinfo_t *buf);
void *video_alloc_buffer_simple(struct video_entity_job_t *job, int size, int parameters,
                                void *phy);
void video_free_buffer_simple(void *buffer);
int video_sync_tuning_calculation(void *job, int *src_frame_rate_change);
int video_process_tick(void *clk, unsigned int last_process_jiffies, 
                       struct video_entity_job_t *last_process_job);
int video_entity_notify(struct video_entity_t *entity, int fd, enum notify_type type, int count);
int video_scaling(enum buf_type_t format, unsigned int src_phys_addr, unsigned int src_width,
                  unsigned int src_height, unsigned int dst_phys_addr, unsigned int dst_width,
                  unsigned int dst_height);
int video_scaling2(enum buf_type_t format, unsigned int src_phys_addr, unsigned int src_width,
                  unsigned int src_height,unsigned int src_bg_width, unsigned int src_bg_height,
                  unsigned int dst_phys_addr, unsigned int dst_width, unsigned int dst_height,
                  unsigned int dst_bg_width, unsigned int dst_bg_height);
#endif

