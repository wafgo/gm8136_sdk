#ifndef _VCAP_INPUT_H_
#define _VCAP_INPUT_H_

#include <linux/proc_fs.h>
#include <linux/semaphore.h>

#define INPUT_MAJOR_VER         0
#define INPUT_MINOR_VER         3
#define INPUT_BETA_VER          0
#define VCAP_INPUT_VERSION      (((INPUT_MAJOR_VER)<<16)|((INPUT_MINOR_VER)<<8)|(INPUT_BETA_VER))
#define VCAP_INPUT_DEV_MAX      9
#define VCAP_INPUT_DEV_CH_MAX   4   ///< max channel support in each VI

/*************************************************************************************
 *  VCAP Input Parameter Definition
 *************************************************************************************/
typedef enum {
    VCAP_INPUT_TYPE_GENERIC = 0,
    VCAP_INPUT_TYPE_DECODER,
    VCAP_INPUT_TYPE_SENSOR,
    VCAP_INPUT_TYPE_ISP,
    VCAP_INPUT_TYPE_SDI,
    VCAP_INPUT_TYPE_CVI,
    VCAP_INPUT_TYPE_AHD,
    VCAP_INPUT_TYPE_TVI,
    VCAP_INPUT_TYPE_MAX
} VCAP_INPUT_TYPE_T;

typedef enum {
    VCAP_INPUT_VI_SRC_XCAP0 = 0,    ///< VI video source from X_CAP#0         (bt656/bt1120)
    VCAP_INPUT_VI_SRC_XCAP1,        ///< VI video source from X_CAP#1         (bt656/bt1120)
    VCAP_INPUT_VI_SRC_XCAP2,        ///< VI video source from X_CAP#2         (bt656/bt1120)
    VCAP_INPUT_VI_SRC_XCAP3,        ///< VI video source from X_CAP#3         (bt656/bt1120)
    VCAP_INPUT_VI_SRC_XCAP4,        ///< VI video source from X_CAP#4         (bt656/bt1120)
    VCAP_INPUT_VI_SRC_XCAP5,        ///< VI video source from X_CAP#5         (bt656/bt1120)
    VCAP_INPUT_VI_SRC_XCAP6,        ///< VI video source from X_CAP#6         (bt656/bt1120)
    VCAP_INPUT_VI_SRC_XCAP7,        ///< VI video source from X_CAP#7         (bt656/bt1120)
    VCAP_INPUT_VI_SRC_XCAPCAS,      ///< VI video source from X_CAPCAS        (bt656/bt1120)
    VCAP_INPUT_VI_SRC_LCD0,         ///< VI video source from Internal LCD#0  (bt656/bt1120/rgb888)
    VCAP_INPUT_VI_SRC_LCD1,         ///< VI video source from Internal LCD#1  (bt656/bt1120/rgb888)
    VCAP_INPUT_VI_SRC_LCD2,         ///< VI video source from Internal LCD#2  (bt656/bt1120/rgb888)
    VCAP_INPUT_VI_SRC_ISP,          ///< VI video source from Internal ISP
    VCAP_INPUT_VI_SRC_MAX
} VCAP_INPUT_VI_SRC_T;

typedef enum {
    VCAP_INPUT_SPEED_I = 0,     ///< Interlace,  as 50/60I => 2frame buffer apply grab_field_pair
    VCAP_INPUT_SPEED_P,         ///< Progressive as 25/30P => 2frame buffer apply fame2field
    VCAP_INPUT_SPEED_2P,        ///< Progressive as 50/60P => 2frame buffer apply grab 2 frame
    VCAP_INPUT_SPEED_MAX
} VCAP_INPUT_SPEED_T;

typedef enum {
    VCAP_INPUT_INTF_BT656_INTERLACE = 0,
    VCAP_INPUT_INTF_BT656_PROGRESSIVE,
    VCAP_INPUT_INTF_BT1120_INTERLACE,
    VCAP_INPUT_INTF_BT1120_PROGRESSIVE,
    VCAP_INPUT_INTF_RGB888,
    VCAP_INPUT_INTF_SDI8BIT_INTERLACE,
    VCAP_INPUT_INTF_SDI8BIT_PROGRESSIVE,
    VCAP_INPUT_INTF_BT601_8BIT_INTERLACE,
    VCAP_INPUT_INTF_BT601_8BIT_PROGRESSIVE,
    VCAP_INPUT_INTF_BT601_16BIT_INTERLACE,
    VCAP_INPUT_INTF_BT601_16BIT_PROGRESSIVE,
    VCAP_INPUT_INTF_ISP,
    VCAP_INPUT_INTF_MAX
} VCAP_INPUT_INTF_T;

typedef enum {
    VCAP_INPUT_MODE_BYPASS = 0,
    VCAP_INPUT_MODE_FRAME_INTERLEAVE,
    VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE,
    VCAP_INPUT_MODE_4CH_BYTE_INTERLEAVE,
    VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID,
    VCAP_INPUT_MODE_MAX
} VCAP_INPUT_MODE_T;

typedef enum {
    VCAP_INPUT_ORDER_ANYONE = 0,
    VCAP_INPUT_ORDER_ODD_FIRST,
    VCAP_INPUT_ORDER_EVEN_FIRST,
    VCAP_INPUT_ORDER_MAX
} VCAP_INPUT_ORDER_T;

typedef enum {
    VCAP_INPUT_DATA_RANGE_256 = 0,
    VCAP_INPUT_DATA_RANGE_240,
    VCAP_INPUT_DATA_RANGE_MAX
} VCAP_INPUT_DATA_RANGE_T;

typedef enum {
    VCAP_INPUT_NORM_PAL = 0,        ///< 720x576I@25
    VCAP_INPUT_NORM_NTSC,           ///< 720x480I@30
    VCAP_INPUT_NORM_PAL_960H,       ///< 960x576I@25
    VCAP_INPUT_NORM_NTSC_960H,      ///< 960x480I@30
    VCAP_INPUT_NORM_720P25,         ///< 1280x720P@25
    VCAP_INPUT_NORM_720P30,         ///< 1280x720P@30
    VCAP_INPUT_NORM_720P50,         ///< 1280x720P@50
    VCAP_INPUT_NORM_720P60,         ///< 1280x720P@60
    VCAP_INPUT_NORM_1080P25,        ///< 1920x1080P@25
    VCAP_INPUT_NORM_1080P30,        ///< 1920x1080P@30
    VCAP_INPUT_NORM_1080P50,        ///< 1920x1080P@50
    VCAP_INPUT_NORM_1080P60,        ///< 1920x1080P@60
    VCAP_INPUT_NORM_MAX
} VCAP_INPUT_NORM_T;

typedef enum {
    VCAP_INPUT_SWAP_NONE = 0,
    VCAP_INPUT_SWAP_YC,
    VCAP_INPUT_SWAP_CbCr,
    VCAP_INPUT_SWAP_YC_CbCr,
    VCAP_INPUT_SWAP_MAX
} VCAP_INPUT_SWAP_T;

typedef enum {
    VCAP_INPUT_DATA_SWAP_NONE = 0,
    VCAP_INPUT_DATA_SWAP_LO8BIT,
    VCAP_INPUT_DATA_SWAP_BYTE,
    VCAP_INPUT_DATA_SWAP_LO8BIT_BYTE,
    VCAP_INPUT_DATA_SWAP_HI8BIT,
    VCAP_INPUT_DATA_SWAP_LOHI8BIT,
    VCAP_INPUT_DATA_SWAP_HI8BIT_BYTE,
    VCAP_INPUT_DATA_SWAP_LOHI8BIT_BYTE,
    VCAP_INPUT_DATA_SWAP_MAX
} VCAP_INPUT_DATA_SWAP_T;

typedef enum {
    VCAP_INPUT_CHAN_REORDER_NONE = 0,
    VCAP_INPUT_CHAN_REORDER_0213,
    VCAP_INPUT_CHAN_REORDER_2031,
    VCAP_INPUT_CHAN_REORDER_2301,
    VCAP_INPUT_CHAN_REORDER_MAX
} VCAP_INPUT_CHAN_REORDER_T;

typedef enum {
    VCAP_INPUT_CH_ID_EAV_SAV = 0,
    VCAP_INPUT_CH_ID_HORIZ_ACTIVE,
    VCAP_INPUT_CH_ID_HORIZ_BLANK = 3,
    VCAP_INPUT_CH_ID_MAX
} VCAP_INPUT_CH_ID_T;

struct vcap_input_norm_t {
    int mode;               ///< video input source operation mode
    u32 width:13;           ///< video input source width
    u32 height:13;          ///< video input source height
    u32 reserved:6;
};

struct vcap_input_vbi_t {
    u32 enable:1;           ///< VBI extract control
    u32 start_line:8;       ///< VBI extract start Line
    u32 reserved0:23;

    u32 width:13;           ///< VBI extract width
    u32 height:13;          ///< VBI extract height
    u32 reserved1:6;
};

struct vcap_input_rgb_t {
    u8  vs_pol:1;           ///< polarity of video input vertical sync
    u8  hs_pol:1;           ///< polarity of video input horizontal sync
    u8  de_pol:1;           ///< polarity of video input data enable sync
    u8  watch_de:1;         ///< use data enable to indicate valid data
    u8  sd_hratio:4;        ///< horizontal integer scaling ratio, 0:bypass, 1~15 scaling down ratio
};

struct vcap_input_bt601_t {
    u32  vs_pol:1;           ///< polarity of video input vertical sync
    u32  hs_pol:1;           ///< polarity of video input horizontal sync
    u32  sync_mode:1;        ///< bt601 sync mode control
    u32  x_offset:8;         ///< bt601 input valid data x offset
    u32  y_offset:8;         ///< bt601 input valid data y offset
    u32  reserved:13;
};

struct vcap_input_status_t {
    int vlos;                ///< video signal loss
    int width;               ///< video source width
    int height;              ///< video source height
    int fps;                 ///< video source frame rate
};

struct vcap_input_ch_param_t {
    int                mode;        ///< channel operation mode
    int                width;       ///< channel signal source width
    int                height;      ///< channel signal source height
    int                prog;        ///< channel signal format 0:interlace 1: progressive
    u32                frame_rate;  ///< channel frame rate
    u32                timeout_ms;  ///< channel timeout value(ms)
    VCAP_INPUT_SPEED_T speed;       ///< channel speed type
};

typedef enum {
    VCAP_INPUT_PROBE_CHID_DISABLE = 0,
    VCAP_INPUT_PROBE_CHID_HBLANK,
    VCAP_INPUT_PROBE_CHID_MAX
} VCAP_INPUT_PROBE_CHID_T;

/*************************************************************************************
 *  VCAP Input Deivce Structure
 *************************************************************************************/
struct vcap_input_dev_t {
    int                          index;                           ///< input device index
#define VCAP_INPUT_NAME_SIZE 32
    char                         name[VCAP_INPUT_NAME_SIZE];      ///< input device name
    struct semaphore             sema_lock;                       ///< for semaphore lock
    int                          vi_idx;                          ///< input device vi index
    VCAP_INPUT_VI_SRC_T          vi_src;                          ///< input device vi input source
    VCAP_INPUT_TYPE_T            type;                            ///< type of input device
    int                          attached;                        ///< attach to related vi source
    int                          used;                            ///< input device is in used

    /* Input Parameter */
    struct vcap_input_norm_t     norm;                            ///< resolution of input device
    VCAP_INPUT_SPEED_T           speed;                           ///< interface speed type
    VCAP_INPUT_INTF_T            interface;                       ///< interface of input device
    VCAP_INPUT_MODE_T            mode;                            ///< Time-Division-Multiplexed mode of input device
    VCAP_INPUT_ORDER_T           field_order;                     ///< frame/field order of input device
    VCAP_INPUT_DATA_RANGE_T      data_range;                      ///< data range of input device
    VCAP_INPUT_SWAP_T            yc_swap;                         ///< ycbcr swap control
    VCAP_INPUT_DATA_SWAP_T       data_swap;                       ///< X_CAP port bit/byte swap control
    VCAP_INPUT_CHAN_REORDER_T    ch_reorder;                      ///< Channel reorder
    VCAP_INPUT_CH_ID_T           ch_id_mode;                      ///< Channel ID extract mode
    u32                          pixel_clk;                       ///< pixel clock frequency
    u32                          frame_rate;                      ///< current frame rate FPS
    u32                          max_frame_rate;                  ///< max frame rate FPS
    int                          inv_clk;                         ///< pixel clock invert control
    u32                          timeout_ms;                      ///< signal timeout ms value
    VCAP_INPUT_PROBE_CHID_T      probe_chid;                      ///< probe channel id via software, 0:disable 1:from HBlank
    int                          chid_ready;                      ///< channel id detect ready
    int                          ch_id[VCAP_INPUT_DEV_CH_MAX];    ///< channel index => mapping to physical connector index, means VCH number
    int                          ch_vlos[VCAP_INPUT_DEV_CH_MAX];  ///< channel video loss status, 0:video_present, 1:video_loss
    struct vcap_input_ch_param_t ch_param[VCAP_INPUT_DEV_CH_MAX]; ///< channel parameter, for VI bind hybrid channel
    struct vcap_input_vbi_t      vbi;                             ///< VBI Parameter
    struct vcap_input_rgb_t      rgb;                             ///< RGB888 Signal Parameter
    struct vcap_input_bt601_t    bt601;                           ///< BT601  Signal Parameter

    int  (*module_get)(void);
    void (*module_put)(void);
    int  (*init)(int id, int norm);
};

/*************************************************************************************
 *  VCAP Input Notify Definition
 *************************************************************************************/
typedef enum {
    VCAP_INPUT_NOTIFY_SIGNAL_LOSS = 0,      ///< video loss
    VCAP_INPUT_NOTIFY_SIGNAL_PRESENT,       ///< video present
    VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE,     ///< frame rate change
    VCAP_INPUT_NOTIFY_NORM_CHANGE,          ///< resolution change
    VCAP_INPUT_NOTIFY_MAX
} VCAP_INPUT_NOTIFY_T;

/*************************************************************************************
 *  Public Function Prototype
 *************************************************************************************/
u32  vcap_input_get_version(void);
int  vcap_input_device_notify(int id, int ch, VCAP_INPUT_NOTIFY_T n_type);
int  vcap_input_device_register(struct vcap_input_dev_t *dev);
void vcap_input_device_unregister(struct vcap_input_dev_t *dev);
void vcap_input_proc_remove_entry(struct proc_dir_entry *parent, struct proc_dir_entry *entry);
struct proc_dir_entry *vcap_input_proc_create_entry(const char *name, mode_t mode, struct proc_dir_entry *parent);
struct vcap_input_dev_t *vcap_input_get_info(int dev_id);
int  vcap_input_device_get_status(struct vcap_input_dev_t *dev, int ch, struct vcap_input_status_t *status);

#endif  /* _VCAP_INPUT_H_ */
