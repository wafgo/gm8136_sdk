#ifndef _FE_DEC_TW2868_H_
#define _FE_DEC_TW2868_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  TW2868 Device Definition
 *************************************************************************************/
#define TW2868_DEV_MAX                 4
#define TW2868_DEV_CH_MAX              8       ///< VIN#0~7

typedef enum {
    TW2868_DEV_VPORT_VD1 = 0,
    TW2868_DEV_VPORT_VD2,
    TW2868_DEV_VPORT_VD3,
    TW2868_DEV_VPORT_VD4,
    TW2868_DEV_VPORT_MAX
} TW2868_DEV_VPORT_T;

typedef enum {
    TW2868_VFMT_PAL = 0,               ///< video format PAL
    TW2868_VFMT_NTSC,                  ///< video format NTSC
    TW2868_VFMT_MAX
} TW2868_VFMT_T;

typedef enum {
    TW2868_VMODE_NTSC_720H_1CH = 0,    ///< 27MHz  D1 1 channel
    TW2868_VMODE_NTSC_720H_2CH,        ///< 27MHz  D1 2 channel
    TW2868_VMODE_NTSC_720H_4CH,        ///< 108MHz D1 4 channel

    TW2868_VMODE_PAL_720H_1CH,
    TW2868_VMODE_PAL_720H_2CH,
    TW2868_VMODE_PAL_720H_4CH,
    TW2868_VMODE_MAX
} TW2868_VMODE_T;

typedef enum {
    TW2868_AUDIO_SAMPLERATE_8K = 0,
    TW2868_AUDIO_SAMPLERATE_16K,
    TW2868_AUDIO_SAMPLERATE_32K,
    TW2868_AUDIO_SAMPLERATE_44K,
    TW2868_AUDIO_SAMPLERATE_48K,
}TW2868_SAMPLERATE_T;

typedef enum {
    TW2868_AUDIO_BITS_16B = 0,
    TW2868_AUDIO_BITS_8B,
}TW2868_SAMPLESIZE_T;

/*************************************************************************************
 *  TW2868 IOCTL
 *************************************************************************************/
struct tw2868_ioc_data {
    int ch;         ///< access channel number => vin index
    int data;       ///< read/write data value
};

#define TW2868_IOC_MAGIC            't'

#define TW2868_GET_NOVID            _IOR(TW2868_IOC_MAGIC,   1, struct tw2868_ioc_data)

#define TW2868_GET_MODE   	        _IOR(TW2868_IOC_MAGIC,   2, int)

#define TW2868_GET_CONTRAST         _IOR(TW2868_IOC_MAGIC,   3, struct tw2868_ioc_data)
#define TW2868_SET_CONTRAST         _IOWR(TW2868_IOC_MAGIC,  4, struct tw2868_ioc_data)

#define TW2868_GET_BRIGHTNESS       _IOR(TW2868_IOC_MAGIC,   5, struct tw2868_ioc_data)
#define TW2868_SET_BRIGHTNESS       _IOWR(TW2868_IOC_MAGIC,  6, struct tw2868_ioc_data)

#define TW2868_GET_SATURATION_U     _IOR(TW2868_IOC_MAGIC,   7, struct tw2868_ioc_data)
#define TW2868_SET_SATURATION_U     _IOWR(TW2868_IOC_MAGIC,  8, struct tw2868_ioc_data)

#define TW2868_GET_SATURATION_V     _IOR(TW2868_IOC_MAGIC,   9, struct tw2868_ioc_data)
#define TW2868_SET_SATURATION_V     _IOWR(TW2868_IOC_MAGIC, 10, struct tw2868_ioc_data)

#define TW2868_GET_HUE              _IOR(TW2868_IOC_MAGIC,  11, struct tw2868_ioc_data)
#define TW2868_SET_HUE              _IOWR(TW2868_IOC_MAGIC, 12, struct tw2868_ioc_data)

#define TW2868_GET_VOL              _IOR(TW2868_IOC_MAGIC,  13, int)
#define TW2868_SET_VOL              _IOWR(TW2868_IOC_MAGIC, 14, int)

#define TW2868_GET_OUT_CH           _IOR(TW2868_IOC_MAGIC,  15, int)
#define TW2868_SET_OUT_CH           _IOWR(TW2868_IOC_MAGIC, 16, int)

/*************************************************************************************
 *  TW2868 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int tw2868_get_device_num(void);

int tw2868_vin_to_ch(int id, int vin_idx);
int tw2868_ch_to_vin(int id, int ch_idx);
int tw2868_vin_to_vch(int id, int vin_idx);
int tw2868_get_vch_id(int id, TW2868_DEV_VPORT_T vport, int vport_seq);
u8  tw2868_i2c_read(u8 id, u8 reg);
int tw2868_i2c_write(u8 id, u8 reg, u8 data);

int tw2868_video_mode_support_check(int id, TW2868_VMODE_T mode);
int tw2868_video_get_mode(int id);
int tw2868_video_get_contrast(int id, int ch);
int tw2868_video_get_brightness(int id, int ch);
int tw2868_video_get_saturation_u(int id, int ch);
int tw2868_video_get_saturation_v(int id, int ch);
int tw2868_video_get_hue(int id, int ch);

int tw2868_video_set_mode(int id, TW2868_VMODE_T mode);
int tw2868_video_set_contrast(int id, int ch, u8 value);
int tw2868_video_set_brightness(int id, int ch, u8 value);
int tw2868_video_set_saturation_u(int id, int ch, u8 value);
int tw2868_video_set_saturation_v(int id, int ch, u8 value);
int tw2868_video_set_hue(int id, int ch, u8 value);

int tw2868_status_get_novid(int id, int ch);

int tw2868_audio_set_mode(int id, TW2868_VMODE_T mode, TW2868_SAMPLESIZE_T samplesize, TW2868_SAMPLERATE_T samplerate);
int tw2868_audio_set_mute(int id, int on);
int tw2868_audio_get_mute(int id);
int tw2868_audio_get_volume(int id);
int tw2868_audio_set_volume(int id, int volume);
int tw2868_audio_get_output_ch(int id);
int tw2868_audio_set_output_ch(int id, int ch);

int  tw2868_notify_vlos_register(int id, int (*nt_func)(int, int, int));
void tw2868_notify_vlos_deregister(int id);
#endif

#endif  /* _FE_DEC_TW2868_H_ */
