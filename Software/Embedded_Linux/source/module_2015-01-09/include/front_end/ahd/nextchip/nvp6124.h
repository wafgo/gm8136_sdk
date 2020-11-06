#ifndef _FE_AHD_NVP6124_H_
#define _FE_AHD_NVP6124_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  NVP6124 Device Definition
 *************************************************************************************/
#define NVP6124_DEV_MAX                 4
#define NVP6124_DEV_CH_MAX              4       ///< VIN#0~3

typedef enum {
    NVP6124_DEV_VPORT_A = 0,
    NVP6124_DEV_VPORT_B,
    NVP6124_DEV_VPORT_C,
    NVP6124_DEV_VPORT_D,
    NVP6124_DEV_VPORT_MAX
} NVP6124_DEV_VPORT_T;

typedef enum {
    NVP6124_VOUT_MODE_1CH_BYPASS = 0, 
    NVP6124_VOUT_MODE_2CH_BYTE_INTERLEAVED, ///< For AFHD-CIF output mode
    NVP6124_VOUT_MODE_MAX
} NVP6124_VOUT_MODE_T;

typedef enum {
    NVP6124_VFMT_UNKNOWN = 0,
    //NVP6124_VFMT_720H25,    // SD   720H  PAL
    //NVP6124_VFMT_720H30,    // SD   720H  NTSC
    NVP6124_VFMT_960H25,    // SD   960H  PAL
    NVP6124_VFMT_960H30,    // SD   960H  NTSC
    NVP6124_VFMT_720P25,    // AHD  720P  25FPS
    NVP6124_VFMT_720P30,    // AHD  720P  30FPS
    NVP6124_VFMT_720P50,    // AHD  720P  50FPS
    NVP6124_VFMT_720P60,    // AHD  720P  60FPS
    NVP6124_VFMT_1080P25,   // AFHD 1080P 25FPS
    NVP6124_VFMT_1080P30,   // AFHD 1080P 30FPS
    NVP6124_VFMT_MAX
} NVP6124_VFMT_T;

struct nvp6124_video_fmt_t {
    NVP6124_VFMT_T fmt_idx;       ///< video format index
    unsigned int   width;         ///< video source width
    unsigned int   height;        ///< video source height
    unsigned int   prog;          ///< 0:interlace 1:progressive
    unsigned int   frame_rate;    ///< currect frame rate
};

typedef enum {
    AUDIO_SAMPLERATE_8K = 0,
    AUDIO_SAMPLERATE_16K,
}NVP6124_SAMPLERATE_T;

typedef enum {
    AUDIO_BITS_16B = 0,
    AUDIO_BITS_8B,
}NVP6124_SAMPLESIZE_T;

typedef struct {
    NVP6124_SAMPLERATE_T sample_rate;
    NVP6124_SAMPLESIZE_T sample_size;
    int ch_num;
} NVP6124_AUDIO_INIT_T;

/*************************************************************************************
 *   NVP6124 IOCTL
 *************************************************************************************/
struct nvp6124_ioc_data_t {
    int ch;         ///< access channel number ==> vin index
    int data;       ///< read/write data value
};

struct nvp6124_ioc_vfmt_t {
    int          vin_ch;        ///< nvp6124 vin channel number
    unsigned int width;         ///< nvp6124 channel video width
    unsigned int height;        ///< nvp6124 channel video height
    unsigned int prog;          ///< 0:interlace, 1: progressive
    unsigned int frame_rate;    ///< currect frame rate
};

#define NVP6124_IOC_MAGIC       'n'

#define NVP6124_GET_NOVID       _IOR(NVP6124_IOC_MAGIC,   1, struct nvp6124_ioc_data_t)

#define NVP6124_GET_VIDEO_FMT   _IOR(NVP6124_IOC_MAGIC,   2, struct nvp6124_ioc_vfmt_t)

#define NVP6124_GET_CONTRAST    _IOR(NVP6124_IOC_MAGIC,   3, struct nvp6124_ioc_data_t)
#define NVP6124_SET_CONTRAST    _IOWR(NVP6124_IOC_MAGIC,  4, struct nvp6124_ioc_data_t)

#define NVP6124_GET_BRIGHTNESS  _IOR(NVP6124_IOC_MAGIC,   5, struct nvp6124_ioc_data_t)
#define NVP6124_SET_BRIGHTNESS  _IOWR(NVP6124_IOC_MAGIC,  6, struct nvp6124_ioc_data_t)

#define NVP6124_GET_SATURATION  _IOR(NVP6124_IOC_MAGIC,   7, struct nvp6124_ioc_data_t)
#define NVP6124_SET_SATURATION  _IOWR(NVP6124_IOC_MAGIC,  8, struct nvp6124_ioc_data_t)

#define NVP6124_GET_HUE         _IOR(NVP6124_IOC_MAGIC,   9, struct nvp6124_ioc_data_t)
#define NVP6124_SET_HUE         _IOWR(NVP6124_IOC_MAGIC, 10, struct nvp6124_ioc_data_t)

#define NVP6124_GET_SHARPNESS   _IOR(NVP6124_IOC_MAGIC,  11, struct nvp6124_ioc_data_t)
#define NVP6124_SET_SHARPNESS   _IOWR(NVP6124_IOC_MAGIC, 12, struct nvp6124_ioc_data_t)

#define NVP6124_GET_INPUT_VFMT  _IOR(NVP6124_IOC_MAGIC,  13, struct nvp6124_ioc_data_t)

#define NVP6124_GET_VOL         _IOR(NVP6124_IOC_MAGIC,  14, int)
#define NVP6124_SET_VOL         _IOWR(NVP6124_IOC_MAGIC, 15, int)

#define NVP6124_GET_OUT_CH      _IOR(NVP6124_IOC_MAGIC,  16, int)
#define NVP6124_SET_OUT_CH      _IOWR(NVP6124_IOC_MAGIC, 17, int)

/*************************************************************************************
 *  NVP6124 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int nvp6124_get_device_num(void);

int nvp6124_vin_to_ch(int id, int vin_idx);
int nvp6124_ch_to_vin(int id, int ch_idx);
int nvp6124_get_vch_id(int id, NVP6124_DEV_VPORT_T vport, int vport_seq);
int nvp6124_vin_to_vch(int id, int vin_idx);
u8  nvp6124_i2c_read(u8 id, u8 reg);
int nvp6124_i2c_write(u8 id, u8 reg, u8 data);
int nvp6124_i2c_array_write(u8 id, u8 addr, u8 *parray, u32 array_cnt);

int nvp6124_video_get_output_format(int id, int ch, struct nvp6124_video_fmt_t *vfmt);
int nvp6124_video_get_contrast(int id, int ch);
int nvp6124_video_get_brightness(int id, int ch);
int nvp6124_video_get_saturation(int id, int ch);
int nvp6124_video_get_hue(int id, int ch);
int nvp6124_video_get_sharpness(int id, int ch);

int nvp6124_video_init(int id);
int nvp6124_video_set_equalizer(int id, int ch, NVP6124_VFMT_T vfmt_idx);
int nvp6124_video_set_output_format(int id, int ch, NVP6124_VFMT_T vfmt_idx);
int nvp6124_video_set_contrast(int id, int ch, u8 value);
int nvp6124_video_set_brightness(int id, int ch, u8 value);
int nvp6124_video_set_saturation(int id, int ch, u8 value);
int nvp6124_video_set_hue(int id, int ch, u8 value);
int nvp6124_video_set_sharpness(int id, int ch, u8 value);

int nvp6124_status_get_novid(int id);
int nvp6124_status_get_video_input_format(int id, int ch);

//int nvp6124_audio_set_mode(int id, NVP6124_VMODE_T mode, NVP6124_SAMPLESIZE_T samplesize, NVP6124_SAMPLERATE_T samplerate);
int nvp6124_audio_set_mute(int id, int on);
int nvp6124_audio_get_mute(int id);
int nvp6124_audio_set_volume(int id, int volume);
int nvp6124_audio_get_output_ch(int id);
int nvp6124_audio_set_output_ch(int id, int ch);

int  nvp6124_notify_vlos_register(int id, int (*nt_func)(int, int, int));
int  nvp6124_notify_vfmt_register(int id, int (*nt_func)(int, int, struct nvp6124_video_fmt_t *));
void nvp6124_notify_vlos_deregister(int id);
void nvp6124_notify_vfmt_deregister(int id);
#endif

#endif  /* _FE_AHD_NVP6124_H_ */
