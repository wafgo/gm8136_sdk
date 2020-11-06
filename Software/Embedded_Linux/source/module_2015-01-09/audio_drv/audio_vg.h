#ifndef _AUDIO_VG_H_
#define _AUDIO_VG_H_

#include "video_entity.h"

#define MAX_NAME    50
#define MAX_README  100
struct property_map_t {
    int id;
    char name[MAX_NAME];
    char readme[MAX_README];
};

struct property_record_t {
#define MAX_RECORD 100
    struct video_property_t property[MAX_RECORD];
    struct video_entity_t   *entity;
    int    job_id;
};

#define LOG_ERROR     0
#define LOG_WARNING   1
#define LOG_DETAIL    2

#define AUDIO_VERSION_LEN   32

#define PKT_FLOW_MAX        32
#define MODULE_NAME     "AU"    //two bytes character

extern unsigned int gm_log_level;

#if 0
#define GM_LOG(fmt,args...)
#else
#define GM_LOG(level,fmt,args...)    \
do {                                        \
    if (gm_log_level&(level))               \
        printm(MODULE_NAME,fmt,##args);          \
} while (0)

//#define GM_LOG(fmt,args...) printk(fmt, ## args)
#endif

typedef struct {
    int num;
    struct mutex        mutex;
    struct list_head    list;           /* job list from videograph */
    struct delayed_work write_work;
    unsigned int    put_cnt;
    unsigned int    processed_cnt;
    unsigned int    processed_ack;
    unsigned int    post_process_cnt;
    unsigned int    soon_process_cnt;
    unsigned int    callback_cnt;
    unsigned int    flush_cnt;
    unsigned int    drvstop_cnt;
    unsigned int    comm_snd_failed_cnt;
    unsigned int    comm_rcv_failed_cnt;
    unsigned int    comm_live_failed_cnt;
} audio_engine_t;

#define ENGINE_IDX_RECORD   0
#define ENGINE_IDX_PLAYBACK 1

/* job_item_t is used for audio_vg driver to collect job
 * and translate job to audio_job_t
 */
typedef struct _job_item_t {
    void                *job;
    int                 job_id; //record job->id
    int                 engine; //indicate while hw engine
    int                 minor;
    struct list_head    engine_list; //use to add engine_head
    struct list_head    minor_list; //use to add minor_head
    void                *root_job;
    int                 need_callback;//need to callback root_job

#define TYPE_NORMAL_JOB     0
#define TYPE_MULTI_JOB      1
    int                 type;

#define DRIVER_STATUS_STANDBY   1
#define DRIVER_STATUS_ONGOING   2
#define DRIVER_STATUS_FINISH    3
#define DRIVER_STATUS_FAIL      4
#define DRIVER_STATUS_KEEP      5
#define DRIVER_STATUS_FLUSH     6
    int                 status;
    unsigned int        puttime;    //putjob time
    unsigned int        starttime;  //start engine time
    unsigned int        finishtime; //finish engine time

    void                *private; //used to store audio_job_t
} job_item_t;

#if defined(CONFIG_PLATFORM_GM8210)
#define AUDIO_MAX_CHAN  32
#endif
#if defined(CONFIG_PLATFORM_GM8287)
#define AUDIO_MAX_CHAN  16
#endif
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
#define AUDIO_MAX_CHAN  18 /* 2 ch for adda302, 16 ch for external codec */
#endif
#define SUPPORT_TYPE    2

//#define GET_AU_FD(ch,ssp,s_rate,s_size) ((ch<<24)|(ssp<<16)|((s_rate/1000)<<8)|s_size)
#define BITMASK(begin,count) (BITMAP_LAST_WORD_MASK(count) << begin) /* successive bit mask from begin-th bit */

#if defined(CONFIG_PLATFORM_GM8210)
#define SSP_MAX_USED 6
#elif defined(CONFIG_PLATFORM_GM8287)
#define SSP_MAX_USED 3
#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
#define SSP_MAX_USED 2
#endif

#define IOREMAP_SZ      0x00001000

typedef struct _audio_param {
    int max_chan_cnt;
    int ssp_chan[SSP_MAX_USED];
    int ssp_num[SSP_MAX_USED];
    int out_enable[SSP_MAX_USED];
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    int in_enable[SSP_MAX_USED];
#endif
    int nr_enable[SSP_MAX_USED];
    u32 sample_rate[SSP_MAX_USED];
    u32 bit_clock[SSP_MAX_USED];
    int max_ssp_num;
    char codec_name[12];
    char digital_codec_name[12];
    unsigned long live_chan_num_paddr;
    unsigned long live_on_ssp_paddr;
    unsigned long pcm_show_flag_paddr;
    unsigned long play_buf_dbg_paddr;
    unsigned long rec_dbg_stage_paddr;
    unsigned long play_dbg_stage_paddr;
    unsigned long nr_threshold_paddr;
} audio_param_t;

typedef enum _job_status {
    JOB_STS_REC_RUN,
    JOB_STS_REC_STOP,
    JOB_STS_PLAY_RUN,
    JOB_STS_PLAY_STOP,
    JOB_STS_LIVE_RUN,
    JOB_STS_LIVE_STOP,
    JOB_STS_ACK,
    /* error status */
    JOB_STS_ERROR,
    JOB_STS_ENCERR,
    JOB_STS_DECERR,
    JOB_STS_ENC_TIMEOUT,
    JOB_STS_DEC_TIMEOUT,
} job_status_e;

struct audio_job_t {
    unsigned int job_seq_num;                                 // log for handling timeout job
    int job_type;                                           // encode job = 0, playback job = 1
    int ssp_chan;                                           // how many channels of one SSP
    int sample_rate;                                        // sample rate of audio codec (8k, 16k Hz)
    int sample_size;                                        // sample size of one video channel (8, 16 bits)
    int channel_type;                                       // mono = 1, stereo = 2
    int resample_ratio;                                     // it means the percentage, ex. 99 ==> 99%
    int status;                                             // audio job status
    union _time_sync {
        unsigned int jiffies_fa626;                         // record path: jiffies of FA626
        unsigned int mute_samples;                          // playback path: mute samples for playback
    } time_sync;
    union _shared {
        unsigned int bitrate;                               // bitrate for encoder, support dual types: |16bits(31~16)|16bits(15~0)| ==> |type0|type1|
        unsigned int buf_time;                              // buffered time in playback ssp dma buffer
    } shared;
    unsigned int ssp_bitmap;                                // bitmap of which SSP should be used
    unsigned int codec_type;                                // support dual types: |16bits(31~16)|16bits(15~0)| ==> |type0|type1|
    unsigned int enc_blk_cnt;                               // encode block count (default: 1)
    unsigned int dec_blk_sz;                                // decode block size (for ADPCM, etc. used)
    unsigned int chan_bitmap;                               // bitmap of active channel
    unsigned int bitstream_size;                            // playback bitstream size
    unsigned int in_pa[SUPPORT_TYPE];                       // record buffer physical address
    unsigned int in_size[SUPPORT_TYPE];                     // size of each record buffer
    unsigned int data_length[SUPPORT_TYPE][AUDIO_MAX_CHAN]; // length of data to/from fc7500
    unsigned int data_offset[SUPPORT_TYPE][AUDIO_MAX_CHAN]; // data offset in audio buffer
    unsigned int out_pa;                                    // playback buffer physical address
    unsigned int out_size;                                  // size of playback buffer
    void *data;
#ifndef CONFIG_FC7500
    struct work_struct in_work;                 // insert struct audio_job_t into audio_in_wq
    struct work_struct out_work;                // insert struct audio_job_t into audio_out_wq
#endif
};
void audio_job_check(struct audio_job_t *audio_job);

//FIXME: The "au_codec_type_tag" must be the same as "enum audio_encode_type_t" type(defined by vg).
typedef enum au_codec_type_tag {
    CODEC_TYPE_NONE = 0,
    CODEC_TYPE_PCM,
    CODEC_TYPE_AAC,
    CODEC_TYPE_ADPCM,
    CODEC_TYPE_ALAW,
    CODEC_TYPE_ULAW,
    MAX_CODEC_TYPE_COUNT
} au_codec_type_t;

struct au_codec_info_st {
    char  *name;
    int   time_adjust;
    unsigned int frame_samples;
};

typedef struct {    
    unsigned int dma_lisr_cntr;    
    unsigned int record_hisr_index_remain_cntr; 
    unsigned int mute_samples_cntr;
    unsigned int playback_hold_cntr;
    unsigned int record_rw_index_dist;
    unsigned int playback_rw_index_dist;
    unsigned int playback_hisr_cntr[SSP_MAX_USED];
    unsigned int record_hisr_cntr[SSP_MAX_USED];
    unsigned int playback_underrun_cntr[SSP_MAX_USED];    
    unsigned int record_overrun_cntr[SSP_MAX_USED];
    unsigned char fc7500_major_ver[AUDIO_VERSION_LEN];
    unsigned char fc7500_minor_ver[AUDIO_VERSION_LEN];
    unsigned int pb_write_recover_cntr[SSP_MAX_USED];
    unsigned int reserve[9];
} audio_counter_t;

typedef struct {
    void __iomem *dmac_base;
    void __iomem *ssp0_base;
    void __iomem *ssp1_base;
    void __iomem *ssp2_base;
    void __iomem *ssp3_base;
    void __iomem *ssp4_base;
    void __iomem *ssp5_base;
} audio_register_t;

typedef enum _au_chan_type {
    MONO_CHAN = 1,
    STEREO_CHAN
} au_chan_type_t;

typedef enum {
    GM_LOG_QUIET        = 0,
    GM_LOG_ERROR        = (0x1 << 0),
    GM_LOG_REC_PKT_FLOW = (0x1 << 1),
    GM_LOG_PB_PKT_FLOW  = (0x1 << 2),
    GM_LOG_WARNING      = (0x1 << 3),
    GM_LOG_DEBUG        = (0x1 << 4),
    GM_LOG_DETAIL       = (0x1 << 5),
    GM_LOG_CODEC        = (0x1 << 6),
    GM_LOG_INIT         = (0x1 << 7),
    GM_LOG_REC_DSR      = (0x1 << 8),
    GM_LOG_PB_DSR       = (0x1 << 9),
    GM_LOG_IO           = (0x1 << 10),
    GM_LOG_DMA          = (0x1 << 11)
} au_log_level_t;

enum {
    GM_LOG_MODE_FILE    = 0,
    GM_LOG_MODE_CONSOLE = 1
};

#define LOG_VALUE_MUTE                  0
#define LOG_VALUE_SYNC_RESAMPLE_UP      1
#define LOG_VALUE_SYNC_RESAMPLE_DOWN    2
#define LOG_VALUE_HARDWARE_QUEUE_TIME   3
#define LOG_VALUE_USER_QUEUE_TIME       4
#define LOG_VALUE_SPEEDUP_RATIO         5
#define LOG_VALUE_BUFFER_CHASE          6
#define LOG_VALUE_ONE_BLOCK_TIME        7
#define LOG_VALUE_DEC_VIDEO_LATENCY     8
#define LOG_VALUE_SYNC_CHANGE_LEVEL     9

int au_plat_type(void);
int au_get_max_chan(void);
int *au_get_ssp_chan(void);
uint *au_get_bit_clock(void);
uint *au_get_sample_rate(void);
int *au_get_ssp_num(void);
int *au_get_out_enable(void);
int *au_get_in_enable(void);
int au_get_ssp_max(void);
audio_param_t *au_get_param(void);
struct au_codec_info_st* au_vg_get_codec_info_array(void);
void au_vg_set_log_info(int which, int value);
unsigned int au_vg_get_log_info(int which, int arg);
void set_live_sound(int ssp_idx, int chan_num, bool is_on);
int *au_get_live_chan_num(void);
int *au_get_live_on_ssp(void);
int *audio_get_enc_timeout(void);
int *audio_get_dec_timeout(void);
int *audio_get_enc_overrun(void);
int *audio_get_dec_underrun(void);
int *au_get_enc_req_dma_chan_failed_cnt(void);
int *au_get_dec_req_dma_chan_failed_cnt(void);
int *au_get_is_stereo(void);
int audio_write_file(char *filename, void *data, size_t count, unsigned long long *offset);

/*
 * chan: chan number from user, is_get_ssp: get ssp or current chan
 * return ssp idx or "chan num of current ssp idx"
 */
static inline int ssp_and_chan_translate(int chan, int is_get_ssp)
{
    int i, ssp_idx = 0;
    int *ssp_chan = au_get_ssp_chan();

    if (chan < 0)
        return chan;

    for (i = 0; i < au_get_ssp_max(); i++) {
        if (chan / ssp_chan[i] > 0) {
            ssp_idx ++;
#if defined(CONFIG_PLATFORM_GM8287)
            if ((au_get_ssp_num())[i] == 2)
                continue;
#endif
            chan -= ssp_chan[i];
        } else
            break;
    }

    if (is_get_ssp)
        return ssp_idx;

    return chan;
}

#define DEFAULT_SAMPLE_RATE 8000
#define DEFAULT_SAMPLE_SIZE 16
#define PLAYBACK_SEC        10

#if defined(CONFIG_PLATFORM_GM8210)
#define HDMI_SAMPLE_RATE    46880
#elif defined(CONFIG_PLATFORM_GM8287)
#define HDMI_SAMPLE_RATE        48000
#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
#define ADDA_MAX_SAMPLE_RATE    48000
#endif


#endif /* _AUDIO_VG_H_ */
