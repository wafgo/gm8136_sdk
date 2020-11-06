/* Header file for EM User */
#ifndef _EM_API_H_
#define _EM_API_H_
#include "common.h"
#include "job.h"

#define ALLOC_AUTO  -1  //auto for em_alloc_fd

struct process_t {
    struct list_head    list;
    int high_priority;
    int (*func)(int fd, int ver, void *in_prop, void *out_prop,
        unsigned int *in_pa, unsigned int *in_va, unsigned int *in_size,
        unsigned int *out_pa, unsigned int *out_va, unsigned int *out_size,
        void *priv_param);
    void *priv_param;
};

struct video_data_cap_t {
    unsigned int str_size;
    unsigned int cap_feature;
    unsigned int src_fmt;
    unsigned int src_xy;
    unsigned int src_bg_dim;
    unsigned int src_dim;
    unsigned int dst_fmt;
    unsigned int dst_xy;
    unsigned int dst_bg_dim;
    unsigned int dst_dim;
    unsigned int win_xy;
    unsigned int win_dim;
    unsigned int dst_crop_xy;
    unsigned int dst_crop_dim;
    unsigned int target_frame_rate;
    unsigned int fps_ratio;    
    unsigned int auto_aspect;    
    unsigned int border;
    int dvr_feature;        //0:liveview 1:recording 
    int md_enb;             //motion detection
    int md_xy_start;        //motion detection
    int md_xy_size;         //motion detection
    int md_xy_num;          //motion detection
    unsigned int dst2_crop_xy;
    unsigned int dst2_crop_dim;
    unsigned int dst2_fd;
    unsigned int dst2_bg_dim;
    unsigned int dst2_xy;
    unsigned int dst2_dim;
    unsigned int win2_xy;
    unsigned int win2_dim;
    unsigned int pool_dim;
    unsigned int one_frame_mode_60i; // 1: method 0 and 4,  enabled 1_frame 60i display, 
                                     // others: disabled
};

struct video_data_display0_t {
    unsigned int str_size;
    unsigned int videograph_plane;
    unsigned int dst_dim;
    unsigned int src_frame_rate;
    unsigned int src_xy;
    unsigned int src_dim;
};
struct video_data_display1_t {
    unsigned int str_size;
    unsigned int videograph_plane;
    unsigned int dst_dim;
    unsigned int src_frame_rate;
    unsigned int src_xy;
    unsigned int src_dim;
};

struct video_data_display2_t {
    unsigned int str_size;
    unsigned int videograph_plane;
    unsigned int dst_dim;
    unsigned int src_frame_rate;
    unsigned int src_xy;
    unsigned int src_dim;
};

struct video_data_di_t {
    unsigned int str_size;
    unsigned int function_auto; //1:enable function, 0:disable function
    unsigned int src_width;
    unsigned int src_height;
    unsigned int dst_fmt;
    unsigned int change_src_interlace; //tell 3di change SRC_INTERLACE = 0, after finish 3DI
    /*  didn_mode: config: 
     *  Bit 0: spatial Di
     *  Bit 1: temporal Di
     *  Bit 2: spatial Dn
     *  Bit 3: temporal Dn 
     *  Bit 4~7 : none used */
    unsigned int didn_mode;
    unsigned int one_frame_mode_60i; // 1: method 0 and 4,  enabled 1_frame 60i display, 
                                     // others: disabled
};

struct video_data_dnr_t {
    unsigned int str_size;
  //  unsigned int function_auto; //1:enable function, 0:disable function
    unsigned int src_width;
    unsigned int src_height;
    unsigned int dst_fmt;
};

struct video_data_rotation_t {
    unsigned int str_size;
    unsigned int src_width;
    unsigned int src_height;
    unsigned int clockwise;
};

#define MAX_PIP_WIN_COUNT   10
struct video_data_scaler_t {
    unsigned int str_size;
    unsigned int scl_feature;
    unsigned int src_xy;
    unsigned int src_dim;
    unsigned int dst_fmt;
    unsigned int dst_xy;
    unsigned int dst_bg_dim;
    unsigned int dst_dim;
    unsigned int auto_aspect;
    unsigned int border;
    unsigned int win_xy;
    unsigned int win_dim;
    unsigned int dst2_bg_dim;
    unsigned int src2_bg_dim;
    unsigned int dst2_xy;
    unsigned int dst2_dim;
    unsigned int src2_xy;
    unsigned int src2_dim;
    unsigned int vg_serial;

    int pip_win_cnt;
    struct {
        int scl_info;
        int from_xy;
        int from_dim;
        int to_xy;
        int to_dim;
    } pip_win[MAX_PIP_WIN_COUNT];
    unsigned int crop_xy;
    unsigned int crop_dim;
    unsigned int max_dim;
    unsigned int is_crop_enable;
    unsigned int pool_dim;
};

struct video_data_h264d_t {
    unsigned int str_size;
};

struct video_data_decode_t {
    unsigned int str_size;
    unsigned int dst_fmt;
    unsigned int dst_bg_dim;
    unsigned int yuv_width_threshold;
    unsigned int sub_yuv_ratio;
    unsigned int dec_recon_pool_id;
    unsigned int dec_mbinfo_pool_id;
    unsigned int max_recon_dim;    //for decoder recon_buf allocation (max_recon_dim to sizes)
};

#define PURPOSE_VIDEO_BS_IN 0
#define PURPOSE_AUDIO_BS_IN 1
#define PURPOSE_RAW_DATA_IN 2
struct video_data_datain_t {
    unsigned int str_size;
    unsigned int purpose;
    //video attributes
    unsigned int vari_win_size;  //variable length case, for em only, don't need to transfer to property
    unsigned int vari_slot_size; //<variable length case, for em only(put it to buffer_info.size), don't need to transfer to property
    unsigned int buf_width;
    unsigned int buf_height;
    unsigned int buf_format;
    unsigned int is_timestamp_by_ap;
    //audio attributes
//    unsigned int au_bs_size;
    unsigned int sample_rate;
    unsigned int sample_size;
    unsigned int frame_samples;
    unsigned int channels;

    unsigned int reserve;
};

#define MAX_ROI_QP_NUMBER   8
struct video_data_h264_e_t {
    unsigned int str_size;
    unsigned int vari_slot_size;     //for variable buffer, don't need to transfer to property
    unsigned int vari_win_size;      //for variable buffer, don't need to transfer to property
    unsigned int init_quant;
    unsigned int max_quant;
    unsigned int min_quant;
    unsigned int rate_ctl_mode;
    unsigned int bitrate;
    unsigned int bitrate_max;
    unsigned int idr_interval;
    unsigned int b_frame_num;
    unsigned int enable_mv_data;
    unsigned int slice_type;
    unsigned int fps_ratio;
    unsigned int graph_ratio;
    unsigned int pool_id;
    unsigned int src_xy;             // for ROI x and y
    unsigned int src_dim;            // for ROI width and height
    unsigned int roi_qp_region[MAX_ROI_QP_NUMBER];
    unsigned int field_coding;
    unsigned int multislice;
    unsigned int slice_offset[3]; //multi-slice offset 1~3 (first offset is 0) 
    unsigned int gray_scale;
    unsigned int watermark_enable;
    unsigned int watermark_pattern;
    unsigned int didn_mode;
    unsigned int profile;
    unsigned int qp_offset;
    struct vui_param_t {
        char matrix_coefficient;
        char transfer_characteristics;
        char colour_primaries;
        char full_range : 1; 
        char video_format : 3;
        char timing_info_present_flag : 1;
    } vui_param;

    struct vui_sar_t {
        unsigned short sar_width;
        unsigned short sar_height;
    } vui_sar;
    unsigned int checksum_type;
    unsigned int fast_forward;
    unsigned int target_frame;
};

struct video_data_mpeg4_e_t {
    unsigned int str_size;
    unsigned int vari_slot_size;     //for variable buffer, don't need to transfer to property
    unsigned int vari_win_size;      //for variable buffer, don't need to transfer to property
    unsigned int init_quant;
    unsigned int max_quant;
    unsigned int min_quant;
    unsigned int bitrate;
    unsigned int idr_interval;
    unsigned int slice_type;
    unsigned int fps_ratio;
    unsigned int graph_ratio;
    unsigned int pool_id;
    unsigned int rate_ctl_mode;
    unsigned int src_xy;             // for ROI x and y
    unsigned int src_dim;            // for ROI width and height
    unsigned int target_frame;
};


struct video_data_jpeg_e_t {
    unsigned int str_size;
    unsigned int vari_slot_size;     //for variable buffer, don't need to transfer to property
    unsigned int vari_win_size;      //for variable buffer, don't need to transfer to property
    unsigned int quality;
    unsigned int src_xy;
    unsigned int src_fmt;
    unsigned int src_dim;
    unsigned int src_bg_dim;
    unsigned int image_quality;
    unsigned int target_frame;
    unsigned int rate_ctl_mode;
    unsigned int bitrate;
    unsigned int bitrate_max;
};

struct video_data_aenc_t {
    unsigned int str_size;
    unsigned int total_ch_cnt;
    unsigned int ch_bmp;
    unsigned int sample_rate;
    unsigned int sample_size;
    unsigned int max_frame_samples;  //the value is defined in spec.cfg and is used to allocate for max buffer
    unsigned int channel_type;
    unsigned int encode_type1;
    unsigned int bitrate1;
    unsigned int encode_type2;
    unsigned int bitrate2;
    unsigned int frame_samples; //the value is set by user runtime
    unsigned int block_count;
    unsigned int dst_fmt;
    unsigned int slice_type;
};

struct video_data_adec_t {
    unsigned int ch_bmp;
    unsigned int str_size;
    unsigned int resample_ratio;
    unsigned int encode_type;
    unsigned int block_size;
    unsigned int sample_rate;
    unsigned int sample_size;
    unsigned int channel_type;
};

struct video_data_dataout_t {
    unsigned int str_size;
    unsigned int id;
    unsigned int slice_type;
    unsigned int bs_size;
};


struct video_user_param_t {
    char            name[16];
    unsigned short  id;
    unsigned int    value;
};

#define MAX_USER_PROPERTY_COUNTS  16
struct video_user_work_t {
    int type;
    int priority;
    int need_stop;
    unsigned int fd;
    unsigned int engine; //for specific engine
    unsigned int minor;  //for specific minor
    unsigned int in_addr_pa;
    unsigned int in_addr_va;
    unsigned int in_size;
    unsigned int out_addr_pa;
    unsigned int out_addr_va;
    unsigned int out_size;
    struct video_user_param_t in_param[MAX_USER_PROPERTY_COUNTS];
    struct video_user_param_t out_param[MAX_USER_PROPERTY_COUNTS];
    struct video_entity_job_t *first_job;
    struct video_user_work_t *next;
};

struct buffer_operations {
    int (*buffer_reserve_func)(void *job, int dir);
    int (*buffer_free_func)(void *binfo);
    int (*buffer_alloc_func)(void *job, int dir, void *binfo);
    int (*buffer_realloc_func)(void *p_video_bufinfo, int job_id);
    int (*buffer_complete_func)(void *p_video_bufinfo, int used_size);
    void *(*buffer_alloc_func2)(int pool, int size, void *phy_addr, char *name);
    void (*buffer_free_func2)(void *buffer);
    void *(*buffer_va_to_pa)(void *va);
    void *(*buffer_fixed_buf_private_func)(void *buf);
    int (*entity_clock_tick)(void *clk_data, unsigned int last_jiffies, void *last_job);
};

struct vpd_operations {
    int (*entity_notify_handler)(enum notify_type type, int fd);
};


enum em_frame_type_tag {
    EM_FT_I_FRAME = 0,
    EM_FT_P_FRAME = 1,
};

unsigned int em_query_fd(int type, int engine, int minor);
unsigned int em_alloc_fd(int type, int dev, int ch); // dev:ALLOC_AUTO ch:ALLOC_AUTO
int em_free_fd(unsigned int fd);
int em_set_param(int fd, int ver, void *);
int em_destroy_param(int fd, int ver);
int em_set_property(int fd, int ver, void *);
int em_destroy_property(int fd, int ver);
unsigned int em_query_propertyid(int type, void *property_str);
int em_query_propertystr(int type, int id, void *property_str);
int em_sync_tuning_self_adjust(int fd, int min_cnt);
int em_sync_tuning_setup_latency_range(int from_fd, int to_fd);
int em_sync_tuning_target_register(int fd, int target_fd, int threshold);
int em_get_bufinfo(int fd, int ver, struct video_bufinfo_t *in_info,
                   struct video_bufinfo_t *out_info);
int em_query_entity_name(int fd, char *name, int n_size);
unsigned int em_query_entity_fd(char *name);
int em_register_buffer_ops(struct buffer_operations *ops);
int em_register_vpd_ops(struct vpd_operations *ops);
int em_query_last_entity_fd(int fd);
int em_get_buf_info_directly(struct video_bufinfo_t *binfo, struct video_buf_info_items_t *bitem);
int em_queue_preprocess(int fd, int high_priority,
                        int (*func)(int fd, int ver, void *in_prop, void *out_prop,
                        unsigned int *in_pa, unsigned int *in_va, unsigned int *in_size,
                        unsigned int *out_pa, unsigned int *out_va, unsigned int *out_size,
                        void *priv_param), void *priv_param);
int em_queue_postprocess(int fd, int high_priority,
                        int (*func)(int fd, int ver, void *in_prop, void *out_prop,
                        unsigned int *in_pa, unsigned int *in_va, unsigned int *in_size,
                        unsigned int *out_pa, unsigned int *out_va, unsigned int *out_size,
                        void *priv_param), void *priv_param);
int em_dequeue_preprocess(int fd, int (*func)(int fd, int ver, void *in_prop, void *out_prop,
                        unsigned int *in_pa, unsigned int *in_va, unsigned int *in_size,
                        unsigned int *out_pa, unsigned int *out_va, unsigned int *out_size,
                        void *priv_param));
int em_dequeue_postprocess(int fd, int (*func)(int fd, int ver, void *in_prop, void *out_prop,
                        unsigned int *in_pa, unsigned int *in_va, unsigned int *in_size,
                        unsigned int *out_pa, unsigned int *out_va, unsigned int *out_size,
                        void *priv_param));
int em_user_putjob(struct video_user_work_t *user_work);
int em_fd_exist(int fd);
int em_putjob(struct video_entity_job_t *job);
int em_stopjob(int fd);
int em_register_clock(int fd, void *);
int em_complete_buffer(struct video_bufinfo_t *binfo, int used_size);
int em_realloc_buffer(struct video_bufinfo_t *binfo, int job_id);

unsigned int em_get_buffer_private(unsigned int addr);
void em_set_buffer_private(unsigned int addr, unsigned int value);

#define EM_FRAME_DROP    0
void* em_entity_notify(void *entity_data, int fd, int type, int count);

#define SIG_KEYFRAME        0x36363838     //send signal to request keyframe
#define SIG_ADJUST_DISPLAY  0x32320080     //send signal to adjust display

typedef struct {
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
} em_adjust_display_t;

int em_signal(int fd, int sig, void *param);
#endif
