#ifndef _FE_EN331_H_
#define _FE_EN331_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  EN331 Device Definition
 *************************************************************************************/
#define EN331_DEV_MAX                 4
#define EN331_DEV_CH_MAX              1

typedef enum {
    EN331_DEV_VOUT0 = 0,              ///< VD1
    EN331_DEV_VOUT_MAX
} EN331_DEV_VOUT_T;

typedef enum {
    EN331_VOUT_FORMAT_BT656 = 0,
    EN331_VOUT_FORMAT_BT1120,
    EN331_VOUT_FORMAT_MAX
} EN331_VOUT_FORMAT_T;

typedef enum {
    EN331_VFMT_1080P24 = 0,
    EN331_VFMT_1080P25,
    EN331_VFMT_1080P30,
    EN331_VFMT_1080I50,
    EN331_VFMT_1080I60,
    EN331_VFMT_720P24,
    EN331_VFMT_720P25,
    EN331_VFMT_720P30,
    EN331_VFMT_720P50,
    EN331_VFMT_720P60,
    EN331_VFMT_MAX
} EN331_VFMT_T;

struct en331_video_fmt_t {
    EN331_VFMT_T fmt_idx;       ///< video format index
    unsigned int  width;         ///< video source width
    unsigned int  height;        ///< video source height
    unsigned int  prog;          ///< 0:interlace 1:progressive
    unsigned int  frame_rate;    ///< currect frame rate
};

/*************************************************************************************
 *   EN331 IOCTL
 *************************************************************************************/
struct en331_ioc_data_t {
    int vin_ch;         ///< access en331 vin channel number
    int data;           ///< read/write data value
};

struct en331_ioc_vfmt_t {
    int          vin_ch;        ///< en331 vin channel number
    unsigned int width;         ///< en331 channel video width
    unsigned int height;        ///< en331 channel video height
    unsigned int prog;          ///< 0:interlace, 1: progressive
    unsigned int frame_rate;    ///< currect frame rate
};

#define  EN331_IOC_MAGIC       'e'

#define  EN331_GET_NOVID       _IOR( EN331_IOC_MAGIC,   1, struct en331_ioc_data_t)
#define  EN331_GET_VIDEO_FMT   _IOR( EN331_IOC_MAGIC,   2, struct en331_ioc_vfmt_t)

/*************************************************************************************
 *   EN331 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int  en331_get_device_num(void);
int  en331_get_vch_id(int id,  EN331_DEV_VOUT_T vout, int vin_ch);
int  en331_get_vout_format(int id);

u32  en331_i2c_read(u8 id, u32 addr);
int  en331_i2c_write(u8 id, u32 addr, u32 data);
int  en331_i2c_write_table(u8 id, u32 *parray, u32 size);

int  en331_notify_vfmt_register(int id, int (*nt_func)(int, int, struct en331_video_fmt_t *));
void en331_notify_vfmt_deregister(int id);
int  en331_notify_vlos_register(int id, int (*nt_func)(int, int, int));
void en331_notify_vlos_deregister(int id);

int  en331_status_get_video_input_format(int id, int ch, struct en331_video_fmt_t *vfmt);
int  en331_status_get_video_loss(int id, int ch);

int  en331_video_init(int id, EN331_VOUT_FORMAT_T vout_fmt);
int  en331_video_get_video_output_format(int id, int ch, struct en331_video_fmt_t *vfmt);
int  en331_video_set_video_output_format(int id, int ch, EN331_VFMT_T vfmt, EN331_VOUT_FORMAT_T vout_fmt);
int  en331_output_black_image(int id);
int  en331_output_normal(int id);

#endif

#endif  /* _FE_EN331_H_ */
