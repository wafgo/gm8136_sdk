#ifndef _FE_DEC_NVP1104_H_
#define _FE_DEC_NVP1104_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  NVP1104 Device Definition
 *************************************************************************************/
#define NVP1104_DEV_MAX                 4
#define NVP1104_DEV_CH_MAX              4       ///< VIN#0~3

typedef enum {
    NVP1104_DEV_VPORT_A = 0,
    NVP1104_DEV_VPORT_B,
    NVP1104_DEV_VPORT_C,
    NVP1104_DEV_VPORT_D,
    NVP1104_DEV_VPORT_MAX
} NVP1104_DEV_VPORT_T;

typedef enum {
    NVP1104_OSC_CLKIN_54MHZ = 0,
    NVP1104_OSC_CLKIN_108MHZ,
    NVP1104_OSC_CLKIN_MAX
} NVP1104_OSC_CLKIN_T;

typedef enum {
    NVP1104_VFMT_PAL = 0,               ///< video format PAL
    NVP1104_VFMT_NTSC,                  ///< video format NTSC
    NVP1104_VFMT_MAX
} NVP1104_VFMT_T;

typedef enum {
    NVP1104_VMODE_NTSC_720H_1CH = 0,    ///< 27MHz  D1  1 channel
    NVP1104_VMODE_NTSC_720H_2CH,        ///< 27MHz  D1  2 channel
    NVP1104_VMODE_NTSC_720H_4CH,        ///< 108MHz D1  4 channel
    NVP1104_VMODE_NTSC_CIF_4CH,         ///< 54MHz  CIF 4 channel

    NVP1104_VMODE_PAL_720H_1CH,
    NVP1104_VMODE_PAL_720H_2CH,
    NVP1104_VMODE_PAL_720H_4CH,
    NVP1104_VMODE_PAL_CIF_4CH,
    NVP1104_VMODE_MAX
} NVP1104_VMODE_T;

typedef enum {
    AUDIO_SAMPLERATE_8K = 0,
    AUDIO_SAMPLERATE_16K,
}NVP1104_SAMPLERATE_T;

typedef enum {
    AUDIO_BITS_16B = 0,
    AUDIO_BITS_8B,
}NVP1104_SAMPLESIZE_T;

/*************************************************************************************
 *  NVP1104 IOCTL
 *************************************************************************************/
struct nvp1104_ioc_data {
    int ch;         ///< access channel number => vin index
    int data;       ///< read/write data value
};

#define NVP1104_IOC_MAGIC       'n'

#define NVP1104_GET_NOVID       _IOR(NVP1104_IOC_MAGIC,   1, struct nvp1104_ioc_data)

#define NVP1104_GET_MODE   		_IOR(NVP1104_IOC_MAGIC,   2, int)

#define NVP1104_GET_CONTRAST    _IOR(NVP1104_IOC_MAGIC,   3, struct nvp1104_ioc_data)
#define NVP1104_SET_CONTRAST    _IOWR(NVP1104_IOC_MAGIC,  4, struct nvp1104_ioc_data)

#define NVP1104_GET_BRIGHTNESS  _IOR(NVP1104_IOC_MAGIC,   5, struct nvp1104_ioc_data)
#define NVP1104_SET_BRIGHTNESS  _IOWR(NVP1104_IOC_MAGIC,  6, struct nvp1104_ioc_data)

#define NVP1104_GET_SATURATION  _IOR(NVP1104_IOC_MAGIC,   7, struct nvp1104_ioc_data)
#define NVP1104_SET_SATURATION  _IOWR(NVP1104_IOC_MAGIC,  8, struct nvp1104_ioc_data)

#define NVP1104_GET_HUE         _IOR(NVP1104_IOC_MAGIC,   9, struct nvp1104_ioc_data)
#define NVP1104_SET_HUE         _IOWR(NVP1104_IOC_MAGIC, 10, struct nvp1104_ioc_data)

#define NVP1104_GET_VOL         _IOR(NVP1104_IOC_MAGIC,  11, int)
#define NVP1104_SET_VOL         _IOWR(NVP1104_IOC_MAGIC, 12, int)

#define NVP1104_GET_OUT_CH      _IOR(NVP1104_IOC_MAGIC,  13, int)
#define NVP1104_SET_OUT_CH      _IOWR(NVP1104_IOC_MAGIC, 14, int)

#define NVP1104_GET_SHARPNESS   _IOR(NVP1104_IOC_MAGIC,  15, int)
#define NVP1104_SET_SHARPNESS   _IOWR(NVP1104_IOC_MAGIC, 16, int)

#define NVP1104_SET_MODE        _IOWR(NVP1104_IOC_MAGIC, 17, int)

/*************************************************************************************
 *  NVP1104 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int nvp1104_get_device_num(void);
u32 nvp1104_get_clkin_freq(void);

int nvp1104_get_vch_id(int id, NVP1104_DEV_VPORT_T vport, int vport_seq);
int nvp1104_vin_to_vch(int id, int vin_idx);
u8  nvp1104_i2c_read(u8 id, u8 reg);
int nvp1104_i2c_write(u8 id, u8 reg, u8 data);
int nvp1104_i2c_array_write(u8 id, u8 addr, u8 *parray, u32 array_cnt);

int nvp1104_video_mode_support_check(int id, NVP1104_VMODE_T mode);
int nvp1104_video_get_mode(int id);
int nvp1104_video_get_contrast(int id, int ch);
int nvp1104_video_get_brightness(int id, int ch);
int nvp1104_video_get_saturation(int id, int ch);
int nvp1104_video_get_hue(int id, int ch);
int nvp1104_video_get_sharpness(int id);

int nvp1104_video_set_mode(int id, NVP1104_VMODE_T mode, NVP1104_OSC_CLKIN_T clkin);
int nvp1104_video_set_contrast(int id, int ch, u8 value);
int nvp1104_video_set_brightness(int id, int ch, u8 value);
int nvp1104_video_set_saturation(int id, int ch, u8 value);
int nvp1104_video_set_hue(int id, int ch, u8 value);
int nvp1104_video_set_sharpness(int id, u8 value);

int nvp1104_status_get_novid(int id);

int nvp1104_audio_set_mode(int id, NVP1104_VMODE_T mode, NVP1104_SAMPLESIZE_T samplesize, NVP1104_SAMPLERATE_T samplerate);

int  nvp1104_notify_vlos_register(int id, int (*nt_func)(int, int, int));
void nvp1104_notify_vlos_deregister(int id);
int  nvp1104_notify_vfmt_register(int id, int (*nt_func)(int, int));
void nvp1104_notify_vfmt_deregister(int id);
#endif

#endif  /* _FE_DEC_NVP1104_H_ */
