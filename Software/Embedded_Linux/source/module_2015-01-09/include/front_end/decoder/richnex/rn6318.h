#ifndef _FE_DEC_RN6318_H_
#define _FE_DEC_RN6318_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  RN6318 Device Definition
 *************************************************************************************/
#define RN6318_DEV_MAX                 2
#define RN6318_DEV_BUILT_IN_DEV_MAX    2            ///< One RN6318 device built-in two rn6314 device
#define RN6318_DEV_CH_MAX              8            ///< VIN#0~7
#define RN6318_DEV_REV_A_PID           0x0275
#define RN6318_AUDIO_CH_MAX            16
#define RN6318_PLAYBACK_VOL_LEVEL      16

#define RN6318_BUILD_IN_DEV_ID(x)      (((x)/4))    ///< x => vin#0~7
#define RN6318_BUILD_IN_DEV_CH(x)      (((x)%4))    ///< x => vin#0~7

typedef enum {
    RN6318_DEV_VPORT_1 = 0,
    RN6318_DEV_VPORT_3,
    RN6318_DEV_VPORT_MAX
} RN6318_DEV_VPORT_T;

typedef enum {
    RN6318_VFMT_PAL = 0,               ///< video format PAL
    RN6318_VFMT_NTSC,                  ///< video format NTSC
    RN6318_VFMT_MAX
} RN6318_VFMT_T;

typedef enum {
    RN6318_VMODE_NTSC_720H_1CH = 0,    ///< 27MHz  D1 1 channel
    RN6318_VMODE_NTSC_720H_2CH,        ///< 27MHz  D1 2 channel
    RN6318_VMODE_NTSC_720H_4CH,        ///< 108MHz D1 4 channel
    RN6318_VMODE_NTSC_960H_1CH,        ///< 36MHz  D1 1 channel
    RN6318_VMODE_NTSC_960H_2CH,        ///< 36MHz  D1 2 channel
    RN6318_VMODE_NTSC_960H_4CH,        ///< 144MHz D1 4 channel

    RN6318_VMODE_PAL_720H_1CH,
    RN6318_VMODE_PAL_720H_2CH,
    RN6318_VMODE_PAL_720H_4CH,
    RN6318_VMODE_PAL_960H_1CH,
    RN6318_VMODE_PAL_960H_2CH,
    RN6318_VMODE_PAL_960H_4CH,
    RN6318_VMODE_MAX
} RN6318_VMODE_T;

typedef enum {
    RN6318_AUDIO_SAMPLE_SIZE_16B = 0,
    RN6318_AUDIO_SAMPLE_SIZE_8B,
    RN6318_AUDIO_BITWIDTH_MAX
} RN6318_AUDIO_SAMPLE_SIZE_T;

typedef enum {
    RN6318_AUDIO_SAMPLE_RATE_8K = 0,
    RN6318_AUDIO_SAMPLE_RATE_16K,
    RN6318_AUDIO_SAMPLE_RATE_32K,
    RN6318_AUDIO_SAMPLE_RATE_44K,   ///< 44.1K
    RN6318_AUDIO_SAMPLE_RATE_48K,
    RN6318_AUDIO_SAMPLE_RATE_MAX
} RN6318_AUDIO_SAMPLE_RATE_T;

typedef enum {
    RN6318_AUDIO_SUPPORT_GM8287,
    RN6318_AUDIO_SUPPORT_GM8286,
    RN6318_AUDIO_SUPPORT_GM8282,
    RN6318_AUDIO_SUPPORT_GM8210,
    RN6318_AUDIO_SUPPORT_GM8283
} RN6318_CHIP_TYPE_T;

typedef struct rn6318_audio_parameter {
    int chip_num;
    int channel_num;
    u32 chip_id;
} RN6318_AUDIO_PARAM_T;

/*************************************************************************************
 *  RN6318 IOCTL
 *************************************************************************************/
struct rn6318_ioc_data {
    int ch;         ///< access channel number => vin index
    int data;       ///< read/write data value
};

#define RN6318_IOC_MAGIC        'n'

#define RN6318_GET_NOVID        _IOR(RN6318_IOC_MAGIC,   1, struct rn6318_ioc_data)

#define RN6318_GET_MODE         _IOR(RN6318_IOC_MAGIC,   2, int)

#define RN6318_GET_CONTRAST     _IOR(RN6318_IOC_MAGIC,   3, struct rn6318_ioc_data)
#define RN6318_SET_CONTRAST     _IOWR(RN6318_IOC_MAGIC,  4, struct rn6318_ioc_data)

#define RN6318_GET_BRIGHTNESS   _IOR(RN6318_IOC_MAGIC,   5, struct rn6318_ioc_data)
#define RN6318_SET_BRIGHTNESS   _IOWR(RN6318_IOC_MAGIC,  6, struct rn6318_ioc_data)

#define RN6318_GET_SATURATION   _IOR(RN6318_IOC_MAGIC,   7, struct rn6318_ioc_data)
#define RN6318_SET_SATURATION   _IOWR(RN6318_IOC_MAGIC,  8, struct rn6318_ioc_data)

#define RN6318_GET_HUE          _IOR(RN6318_IOC_MAGIC,   9, struct rn6318_ioc_data)
#define RN6318_SET_HUE          _IOWR(RN6318_IOC_MAGIC,  10, struct rn6318_ioc_data)

#define RN6318_GET_SAMPLE_RATE  _IOR(RN6318_IOC_MAGIC,   11, int)
#define RN6318_SET_SAMPLE_RATE  _IOWR(RN6318_IOC_MAGIC,  12, int)

#define RN6318_GET_SAMPLE_SIZE  _IOR(RN6318_IOC_MAGIC,   13, int)
#define RN6318_SET_SAMPLE_SIZE  _IOWR(RN6318_IOC_MAGIC,  14, int)

#define RN6318_GET_VOLUME       _IOR(RN6318_IOC_MAGIC,   15, struct rn6318_ioc_data)
#define RN6318_SET_VOLUME       _IOWR(RN6318_IOC_MAGIC,  16, struct rn6318_ioc_data)

#define RN6318_GET_PLAYBACK_CH  _IOR(RN6318_IOC_MAGIC,   17, int)
#define RN6318_SET_PLAYBACK_CH  _IOWR(RN6318_IOC_MAGIC,  18, int)

#define RN6318_GET_BYPASS_CH    _IOR(RN6318_IOC_MAGIC,   19, int)    
#define RN6318_SET_BYPASS_CH    _IOWR(RN6318_IOC_MAGIC,  20, int)

#define RN6318_GET_SHARPNESS    _IOR(RN6318_IOC_MAGIC,   21, struct rn6318_ioc_data)
#define RN6318_SET_SHARPNESS    _IOWR(RN6318_IOC_MAGIC,  22, struct rn6318_ioc_data)

#define RN6318_SET_MODE         _IOWR(RN6318_IOC_MAGIC,  23, int)

/*************************************************************************************
 *  RN6318 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int rn6318_get_device_num(void);

int rn6318_vin_to_ch(int id, int vin_idx);
int rn6318_vin_to_vch(int id, int vin_idx);
int rn6318_get_vch_id(int id, RN6318_DEV_VPORT_T vport, int vport_seq);
u8  rn6318_i2c_read(u8 id, u8 dev_id, u8 reg);
int rn6318_i2c_write(u8 id, u8 dev_id, u8 reg, u8 data);
int rn6318_i2c_write_table(u8 id, u8 dev_id, u8 *ptable, int size);

int rn6318_video_mode_support_check(int id, RN6318_VMODE_T mode);
int rn6318_video_get_mode(int id);
int rn6318_video_get_contrast(int id, int ch);
int rn6318_video_get_brightness(int id, int ch);
int rn6318_video_get_saturation(int id, int ch);
int rn6318_video_get_hue(int id, int ch);
int rn6318_video_get_sharpness(int id, int ch);

int rn6318_video_set_mode(int id, RN6318_VMODE_T mode);
int rn6318_video_set_contrast(int id, int ch, u8 value);
int rn6318_video_set_brightness(int id, int ch, u8 value);
int rn6318_video_set_saturation(int id, int ch, u8 value);
int rn6318_video_set_hue(int id, int ch, u8 value);
int rn6318_video_set_sharpness(int id, int ch, u8 value);

int rn6318_audio_get_sample_size(int id);
int rn6318_audio_get_sample_rate(int id);
int rn6318_audio_get_playback_channel(void);
int rn6318_audio_get_bypass_channel(void);
int rn6318_audio_get_volume(int id, u8 ch);
int rn6318_audio_get_playback_volume(int id);

int rn6318_audio_set_mode(u8 id, RN6318_AUDIO_SAMPLE_SIZE_T size, RN6318_AUDIO_SAMPLE_RATE_T rate);
int rn6318_audio_set_sample_size(int id, RN6318_AUDIO_SAMPLE_SIZE_T size);
int rn6318_audio_set_sample_rate(int id, RN6318_AUDIO_SAMPLE_RATE_T rate);
int rn6318_audio_set_volume(int id, u8 ch, u8 volume);
int rn6318_audio_set_playback_volume(int id, u8 volume);
int rn6318_audio_set_playback_channel(int chipID, int pbCH);
int rn6318_audio_set_bypass_channel(int chipID, int bpCH);

int rn6318_status_get_novid(int id, int ch);

int  rn6318_notify_vlos_register(int id, int (*nt_func)(int, int, int));
void rn6318_notify_vlos_deregister(int id);
int  rn6318_notify_vfmt_register(int id, int (*nt_func)(int, int));
void rn6318_notify_vfmt_deregister(int id);
#endif

/*************************************************************************************
 *  RN6318 API Internal Function Prototype
 *************************************************************************************/
int rn6318_audio_get_chip_num(void);
int rn6318_audio_get_channel_num(void);
u32 rn6318_audio_get_chip_id(void);
int rn6318_audio_get_param(struct rn6318_audio_parameter *param);

#endif  /* _FE_DEC_RN6318_H_ */
