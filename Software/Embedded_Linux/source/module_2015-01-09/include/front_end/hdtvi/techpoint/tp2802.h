#ifndef _FE_TP2802_H_
#define _FE_TP2802_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  TP2802 Device Definition
 *************************************************************************************/
#define TP2802_DEV_MAX                 4
#define TP2802_DEV_CH_MAX              4

typedef enum {
    TP2802_DEV_VOUT0 = 0,              ///< VD1
    TP2802_DEV_VOUT1,                  ///< VD2
    TP2802_DEV_VOUT2,                  ///< VD3
    TP2802_DEV_VOUT3,                  ///< VD4
    TP2802_DEV_VOUT_MAX
} TP2802_DEV_VOUT_T;

typedef enum {
    TP2802_VOUT_FORMAT_BT656 = 0,
    TP2802_VOUT_FORMAT_BT1120,
    TP2802_VOUT_FORMAT_MAX
} TP2802_VOUT_FORMAT_T;

typedef enum {
    TP2802_VFMT_720P60 = 0,
    TP2802_VFMT_720P50,
    TP2802_VFMT_1080P30,
    TP2802_VFMT_1080P25,
    TP2802_VFMT_720P30,
    TP2802_VFMT_720P25,
    TP2802_VFMT_MAX
} TP2802_VFMT_T;

struct tp2802_video_fmt_t {
    TP2802_VFMT_T fmt_idx;       ///< video format index
    unsigned int  width;         ///< video source width
    unsigned int  height;        ///< video source height
    unsigned int  prog;          ///< 0:interlace 1:progressive
    unsigned int  frame_rate;    ///< currect frame rate
};

/*************************************************************************************
 *   TP2802 IOCTL
 *************************************************************************************/
struct tp2802_ioc_data_t {
    int vin_ch;         ///< access tp2802 vin channel number
    int data;           ///< read/write data value
};

struct tp2802_ioc_vfmt_t {
    int          vin_ch;        ///< tp2802 vin channel number
    unsigned int width;         ///< tp2802 channel video width
    unsigned int height;        ///< tp2802 channel video height
    unsigned int prog;          ///< 0:interlace, 1: progressive
    unsigned int frame_rate;    ///< currect frame rate
};

#define  TP2802_IOC_MAGIC       't'

#define  TP2802_GET_NOVID       _IOR( TP2802_IOC_MAGIC,   1, struct tp2802_ioc_data_t)
#define  TP2802_GET_VIDEO_FMT   _IOR( TP2802_IOC_MAGIC,   2, struct tp2802_ioc_vfmt_t)

/*************************************************************************************
 *   TP2802 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int  tp2802_get_device_num(void);
int  tp2802_get_vch_id(int id,  TP2802_DEV_VOUT_T vout, int vin_ch);
int  tp2802_get_vout_format(int id);

u8   tp2802_i2c_read(u8 id, u8 reg);
int  tp2802_i2c_read_ext(u8 id, u8 reg);
int  tp2802_i2c_write(u8 id, u8 reg, u8 data);
int  tp2802_i2c_array_write(u8 id, u8 addr, u8 *parray, u32 array_cnt);

int  tp2802_notify_vfmt_register(int id, int (*nt_func)(int, int, struct tp2802_video_fmt_t *));
void tp2802_notify_vfmt_deregister(int id);
int  tp2802_notify_vlos_register(int id, int (*nt_func)(int, int, int));
void tp2802_notify_vlos_deregister(int id);

int  tp2802_status_get_video_input_format(int id, int ch, struct tp2802_video_fmt_t *vfmt);
int  tp2802_status_get_video_loss(int id, int ch);

int  tp2802_video_init(int id, TP2802_VOUT_FORMAT_T vout_fmt);
int  tp2802_video_get_video_output_format(int id, int ch, struct tp2802_video_fmt_t *vfmt);
int  tp2802_video_set_video_output_format(int id, int ch, TP2802_VFMT_T vfmt, TP2802_VOUT_FORMAT_T vout_fmt);

#endif

#endif  /* _FE_TP2802_H_ */
