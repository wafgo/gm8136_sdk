#ifndef _FE_DEC_TW2964_H_
#define _FE_DEC_TW2964_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  TW2964 Device Definition
 *************************************************************************************/
#define TW2964_DEV_MAX                 4
#define TW2964_DEV_CH_MAX              4       ///< VIN#0~3

typedef enum {
    TW2964_DEV_VPORT_VD1 = 0,
    TW2964_DEV_VPORT_VD2,
    TW2964_DEV_VPORT_VD3,
    TW2964_DEV_VPORT_VD4,
    TW2964_DEV_VPORT_MAX
} TW2964_DEV_VPORT_T;

typedef enum {
    TW2964_VFMT_PAL = 0,               ///< video format PAL
    TW2964_VFMT_NTSC,                  ///< video format NTSC
    TW2964_VFMT_MAX
} TW2964_VFMT_T;

typedef enum {
    TW2964_VMODE_NTSC_720H_1CH = 0,    ///< 27MHz  D1 1 channel
    TW2964_VMODE_NTSC_720H_2CH,        ///< 27MHz  D1 2 channel
    TW2964_VMODE_NTSC_720H_4CH,        ///< 108MHz D1 4 channel
    TW2964_VMODE_NTSC_960H_1CH,        ///< 36MHz  D1 1 channel
    TW2964_VMODE_NTSC_960H_2CH,        ///< 36MHz  D1 2 channel
    TW2964_VMODE_NTSC_960H_4CH,        ///< 144MHz D1 4 channel

    TW2964_VMODE_PAL_720H_1CH,
    TW2964_VMODE_PAL_720H_2CH,
    TW2964_VMODE_PAL_720H_4CH,
    TW2964_VMODE_PAL_960H_1CH,
    TW2964_VMODE_PAL_960H_2CH,
    TW2964_VMODE_PAL_960H_4CH,
    TW2964_VMODE_MAX
} TW2964_VMODE_T;

typedef enum {
    TW2964_AUDIO_SAMPLERATE_8K = 0,
    TW2964_AUDIO_SAMPLERATE_16K,
    TW2964_AUDIO_SAMPLERATE_32K,
    TW2964_AUDIO_SAMPLERATE_44K,
    TW2964_AUDIO_SAMPLERATE_48K,
}TW2964_SAMPLERATE_T;

typedef enum {
    TW2964_AUDIO_BITS_16B = 0,
    TW2964_AUDIO_BITS_8B,
}TW2964_SAMPLESIZE_T;

/*************************************************************************************
 *  TW2964 IOCTL
 *************************************************************************************/
struct tw2964_ioc_data {
    int ch;         ///< access channel number => vin index
    int data;       ///< read/write data value
};

#define TW2964_IOC_MAGIC            't'

#define TW2964_GET_NOVID            _IOR(TW2964_IOC_MAGIC,   1, struct tw2964_ioc_data)

#define TW2964_GET_MODE   	    _IOR(TW2964_IOC_MAGIC,   2, int)

#define TW2964_GET_CONTRAST         _IOR(TW2964_IOC_MAGIC,   3, struct tw2964_ioc_data)
#define TW2964_SET_CONTRAST         _IOWR(TW2964_IOC_MAGIC,  4, struct tw2964_ioc_data)

#define TW2964_GET_BRIGHTNESS       _IOR(TW2964_IOC_MAGIC,   5, struct tw2964_ioc_data)
#define TW2964_SET_BRIGHTNESS       _IOWR(TW2964_IOC_MAGIC,  6, struct tw2964_ioc_data)

#define TW2964_GET_SATURATION_U     _IOR(TW2964_IOC_MAGIC,   7, struct tw2964_ioc_data)
#define TW2964_SET_SATURATION_U     _IOWR(TW2964_IOC_MAGIC,  8, struct tw2964_ioc_data)

#define TW2964_GET_SATURATION_V     _IOR(TW2964_IOC_MAGIC,   9, struct tw2964_ioc_data)
#define TW2964_SET_SATURATION_V     _IOWR(TW2964_IOC_MAGIC, 10, struct tw2964_ioc_data)

#define TW2964_GET_HUE              _IOR(TW2964_IOC_MAGIC,  11, struct tw2964_ioc_data)
#define TW2964_SET_HUE              _IOWR(TW2964_IOC_MAGIC, 12, struct tw2964_ioc_data)

#define TW2964_GET_VOL              _IOR(TW2964_IOC_MAGIC,  13, int)
#define TW2964_SET_VOL              _IOWR(TW2964_IOC_MAGIC, 14, int)

#define TW2964_GET_OUT_CH           _IOR(TW2964_IOC_MAGIC,  15, int)
#define TW2964_SET_OUT_CH           _IOWR(TW2964_IOC_MAGIC, 16, int)

#define TW2964_GET_SHARPNESS        _IOR(TW2964_IOC_MAGIC,  17, struct tw2964_ioc_data)
#define TW2964_SET_SHARPNESS        _IOWR(TW2964_IOC_MAGIC, 18, struct tw2964_ioc_data)

#define TW2964_SET_MODE             _IOWR(TW2964_IOC_MAGIC, 19, int)

/*************************************************************************************
 *  TW2964 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int tw2964_get_device_num(void);

int tw2964_vin_to_ch(int id, int vin_idx);
int tw2964_ch_to_vin(int id, int ch_idx);
int tw2964_vin_to_vch(int id, int vin_idx);
int tw2964_get_vch_id(int id, TW2964_DEV_VPORT_T vport, int vport_seq);
u8  tw2964_i2c_read(u8 id, u8 reg);
int tw2964_i2c_write(u8 id, u8 reg, u8 data);

int tw2964_video_mode_support_check(int id, TW2964_VMODE_T mode);
int tw2964_video_get_mode(int id);
int tw2964_video_get_contrast(int id, int ch);
int tw2964_video_get_brightness(int id, int ch);
int tw2964_video_get_saturation_u(int id, int ch);
int tw2964_video_get_saturation_v(int id, int ch);
int tw2964_video_get_hue(int id, int ch);
int tw2964_video_get_sharpness(int id, int ch);

int tw2964_video_set_mode(int id, TW2964_VMODE_T mode);
int tw2964_video_set_contrast(int id, int ch, u8 value);
int tw2964_video_set_brightness(int id, int ch, u8 value);
int tw2964_video_set_saturation_u(int id, int ch, u8 value);
int tw2964_video_set_saturation_v(int id, int ch, u8 value);
int tw2964_video_set_hue(int id, int ch, u8 value);
int tw2964_video_set_sharpness(int id, int ch, u8 value);

int tw2964_status_get_novid(int id, int ch);

int tw2964_audio_set_mode(int id, TW2964_VMODE_T mode, TW2964_SAMPLESIZE_T samplesize, TW2964_SAMPLERATE_T samplerate);
int tw2964_audio_set_mute(int id, int on);
int tw2964_audio_get_mute(int id);
int tw2964_audio_get_volume(int id);
int tw2964_audio_set_volume(int id, int volume);
int tw2964_audio_get_output_ch(int id);
int tw2964_audio_set_output_ch(int id, int ch);

int  tw2964_notify_vlos_register(int id, int (*nt_func)(int, int, int));
void tw2964_notify_vlos_deregister(int id);
int  tw2964_notify_vfmt_register(int id, int (*nt_func)(int, int));
void tw2964_notify_vfmt_deregister(int id);
#endif

#endif  /* _FE_DEC_TW2964_H_ */
