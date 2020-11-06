#ifndef _FE_DEC_NVP1918_H_
#define _FE_DEC_NVP1918_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  NVP1918 Device Definition
 *************************************************************************************/
#define NVP1918_DEV_MAX                 4
#define NVP1918_DEV_CH_MAX              8       ///< VIN#0~7

typedef enum {
    NVP1918_DEV_VPORT_A = 0,
    NVP1918_DEV_VPORT_B,
    NVP1918_DEV_VPORT_C,
    NVP1918_DEV_VPORT_D,
    NVP1918_DEV_VPORT_MAX
} NVP1918_DEV_VPORT_T;

#define NVP1918_CON_CENTER_VAL_NTSC     0x76    ///< video contrast   default, NTSC
#define NVP1918_CON_CENTER_VAL_PAL      0x6b    ///< video contrast   default, PAL
#define NVP1918_BRI_CENTER_VAL_NTSC     0xf8    ///< video brightness default, NTSC
#define NVP1918_BRI_CENTER_VAL_PAL      0x08    ///< video brightness default, PAL
#define NVP1918_HUE_CENTER_VAL_NTSC     0x00    ///< video hue        default, NTSC
#define NVP1918_HUE_CENTER_VAL_PAL      0x00    ///< video hue        default, PAL
#define NVP1918_SAT_CENTER_VAL_NTSC     0x80    ///< video saturation default, NTSC
#define NVP1918_SAT_CENTER_VAL_PAL      0x80    ///< video saturation default, PAL

typedef enum {
    NVP1918_VFMT_PAL = 0,               ///< video format PAL
    NVP1918_VFMT_NTSC,                  ///< video format NTSC
    NVP1918_VFMT_MAX
} NVP1918_VFMT_T;

typedef enum {
    NVP1918_VMODE_NTSC_720H_1CH = 0,    ///< 27MHz  D1 1 channel
    NVP1918_VMODE_NTSC_720H_2CH,        ///< 27MHz  D1 2 channel
    NVP1918_VMODE_NTSC_720H_4CH,        ///< 108MHz D1 4 channel
    NVP1918_VMODE_NTSC_960H_1CH,        ///< 36MHz  D1 1 channel
    NVP1918_VMODE_NTSC_960H_2CH,        ///< 36MHz  D1 2 channel
    NVP1918_VMODE_NTSC_960H_4CH,        ///< 144MHz D1 4 channel

    NVP1918_VMODE_PAL_720H_1CH,
    NVP1918_VMODE_PAL_720H_2CH,
    NVP1918_VMODE_PAL_720H_4CH,
    NVP1918_VMODE_PAL_960H_1CH,
    NVP1918_VMODE_PAL_960H_2CH,
    NVP1918_VMODE_PAL_960H_4CH,
    NVP1918_VMODE_MAX
} NVP1918_VMODE_T;

typedef enum {
    AUDIO_SAMPLERATE_8K = 0,
    AUDIO_SAMPLERATE_16K,
}NVP1918_SAMPLERATE_T;

typedef enum {
    AUDIO_BITS_16B = 0,
    AUDIO_BITS_8B,
}NVP1918_SAMPLESIZE_T;

typedef struct {
    NVP1918_SAMPLERATE_T sample_rate;
    NVP1918_SAMPLESIZE_T sample_size;
    int ch_num;
} NVP1918_AUDIO_INIT_T;

/*************************************************************************************
 *  NVP1918 IOCTL
 *************************************************************************************/
struct nvp1918_ioc_data {
    int ch;         ///< access channel number ==> vin index
    int data;       ///< read/write data value
};

#define NVP1918_IOC_MAGIC       'n'

#define NVP1918_GET_NOVID       _IOR(NVP1918_IOC_MAGIC,   1, struct nvp1918_ioc_data)

#define NVP1918_GET_MODE   		_IOR(NVP1918_IOC_MAGIC,   2, int)

#define NVP1918_GET_CONTRAST    _IOR(NVP1918_IOC_MAGIC,   3, struct nvp1918_ioc_data)
#define NVP1918_SET_CONTRAST    _IOWR(NVP1918_IOC_MAGIC,  4, struct nvp1918_ioc_data)

#define NVP1918_GET_BRIGHTNESS  _IOR(NVP1918_IOC_MAGIC,   5, struct nvp1918_ioc_data)
#define NVP1918_SET_BRIGHTNESS  _IOWR(NVP1918_IOC_MAGIC,  6, struct nvp1918_ioc_data)

#define NVP1918_GET_SATURATION  _IOR(NVP1918_IOC_MAGIC,   7, struct nvp1918_ioc_data)
#define NVP1918_SET_SATURATION  _IOWR(NVP1918_IOC_MAGIC,  8, struct nvp1918_ioc_data)

#define NVP1918_GET_HUE         _IOR(NVP1918_IOC_MAGIC,   9, struct nvp1918_ioc_data)
#define NVP1918_SET_HUE         _IOWR(NVP1918_IOC_MAGIC, 10, struct nvp1918_ioc_data)

#define NVP1918_GET_VOL         _IOR(NVP1918_IOC_MAGIC,  11, int)
#define NVP1918_SET_VOL         _IOWR(NVP1918_IOC_MAGIC, 12, int)

#define NVP1918_GET_OUT_CH      _IOR(NVP1918_IOC_MAGIC,  13, int)
#define NVP1918_SET_OUT_CH      _IOWR(NVP1918_IOC_MAGIC, 14, int)

#define NVP1918_GET_SHARPNESS   _IOR(NVP1918_IOC_MAGIC,  15, struct nvp1918_ioc_data)
#define NVP1918_SET_SHARPNESS   _IOWR(NVP1918_IOC_MAGIC, 16, struct nvp1918_ioc_data)

#define NVP1918_SET_MODE        _IOWR(NVP1918_IOC_MAGIC, 17, int)

/*************************************************************************************
 *  NVP1918 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int nvp1918_get_device_num(void);

int nvp1918_vin_to_ch(int id, int vin_idx);
int nvp1918_ch_to_vin(int id, int ch_idx);
int nvp1918_get_vch_id(int id, NVP1918_DEV_VPORT_T vport, int vport_seq);
int nvp1918_vin_to_vch(int id, int vin_idx);
u8  nvp1918_i2c_read(u8 id, u8 reg);
int nvp1918_i2c_write(u8 id, u8 reg, u8 data);
int nvp1918_i2c_array_write(u8 id, u8 addr, u8 *parray, u32 array_cnt);

int nvp1918_video_mode_support_check(int id, NVP1918_VMODE_T mode);
int nvp1918_video_get_mode(int id);
int nvp1918_video_get_contrast(int id, int ch);
int nvp1918_video_get_brightness(int id, int ch);
int nvp1918_video_get_saturation(int id, int ch);
int nvp1918_video_get_hue(int id, int ch);
int nvp1918_video_get_sharpness(int id, int ch);

int nvp1918_video_set_mode(int id, NVP1918_VMODE_T mode);
int nvp1918_video_set_contrast(int id, int ch, u8 value);
int nvp1918_video_set_brightness(int id, int ch, u8 value);
int nvp1918_video_set_saturation(int id, int ch, u8 value);
int nvp1918_video_set_hue(int id, int ch, u8 value);
int nvp1918_video_set_sharpness(int id, int ch, u8 value);

int nvp1918_status_get_novid(int id);

int nvp1918_audio_set_mode(int id, NVP1918_VMODE_T mode, NVP1918_SAMPLESIZE_T samplesize, NVP1918_SAMPLERATE_T samplerate);
int nvp1918_audio_set_mute(int id, int on);
int nvp1918_audio_get_mute(int id);
int nvp1918_audio_set_volume(int id, int volume);
int nvp1918_audio_get_output_ch(int id);
int nvp1918_audio_set_output_ch(int id, int ch);

int  nvp1918_notify_vlos_register(int id, int (*nt_func)(int, int, int));
void nvp1918_notify_vlos_deregister(int id);
int  nvp1918_notify_vfmt_register(int id, int (*nt_func)(int, int));
void nvp1918_notify_vfmt_deregister(int id);
#endif

#endif  /* _FE_DEC_NVP1918_H_ */
