#ifndef _FE_AHD_NVP6114_H_
#define _FE_AHD_NVP6114_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  NVP6114 Device Definition
 *************************************************************************************/
#define NVP6114_DEV_MAX                 4
#define NVP6114_DEV_CH_MAX              4       ///< VIN#0~3

typedef enum {
    NVP6114_DEV_VPORT_A = 0,
    NVP6114_DEV_VPORT_B,
    NVP6114_DEV_VPORT_MAX
} NVP6114_DEV_VPORT_T;

typedef enum {
    NVP6114_VMODE_NTSC_720H_1CH = 0,    ///< 27MHz  D1   1 channel
    NVP6114_VMODE_NTSC_720H_2CH,        ///< 27MHz  D1   2 channel  (dual edge)
    NVP6114_VMODE_NTSC_720H_4CH,        ///< 108MHz D1   4 channel
    NVP6114_VMODE_NTSC_960H_1CH,        ///< 36MHz  D1   1 channel
    NVP6114_VMODE_NTSC_960H_2CH,        ///< 36MHz  D1   2 channel  (dual edge)
    NVP6114_VMODE_NTSC_960H_4CH,        ///< 144MHz D1   4 channel
    NVP6114_VMODE_NTSC_720P_1CH,        ///< 72MHz  720P 1 channel
    NVP6114_VMODE_NTSC_720P_2CH,        ///< 72MHz  720P 2 channel  (dual edge)

    NVP6114_VMODE_PAL_720H_1CH,         ///< 27MHz  D1   1 channel
    NVP6114_VMODE_PAL_720H_2CH,         ///< 27MHz  D1   2 channel  (dual edge)
    NVP6114_VMODE_PAL_720H_4CH,         ///< 108MHz D1   4 channel
    NVP6114_VMODE_PAL_960H_1CH,         ///< 36MHz  D1   1 channel
    NVP6114_VMODE_PAL_960H_2CH,         ///< 36MHz  D1   2 channel  (dual edge)
    NVP6114_VMODE_PAL_960H_4CH,         ///< 144MHz D1   4 channel
    NVP6114_VMODE_PAL_720P_1CH,         ///< 72MHz  720P 1 channel
    NVP6114_VMODE_PAL_720P_2CH,         ///< 72MHz  720P 2 channel  (dual edge)

    /* Hybrid */
    NVP6114_VMODE_NTSC_720H_720P_2CH,   ///< VPORTA=>720H 2CH, VPORTB=>720P 2CH (dual edge)
    NVP6114_VMODE_NTSC_720P_720H_2CH,   ///< VPORTA=>720P 2CH, VPORTB=>720H 2CH (dual edge)
    NVP6114_VMODE_NTSC_960H_720P_2CH,   ///< VPORTA=>960H 2CH, VPORTB=>720P 2CH (dual edge)
    NVP6114_VMODE_NTSC_720P_960H_2CH,   ///< VPORTA=>720P 2CH, VPORTB=>960H 2CH (dual edge)

    NVP6114_VMODE_PAL_720H_720P_2CH,    ///< VPORTA=>720H 2CH, VPORTB=>720P 2CH (dual edge)
    NVP6114_VMODE_PAL_720P_720H_2CH,    ///< VPORTA=>720P 2CH, VPORTB=>720H 2CH (dual edge)
    NVP6114_VMODE_PAL_960H_720P_2CH,    ///< VPORTA=>960H 2CH, VPORTB=>720P 2CH (dual edge)
    NVP6114_VMODE_PAL_720P_960H_2CH,    ///< VPORTA=>720P 2CH, VPORTB=>960H 2CH (dual edge)

    NVP6114_VMODE_MAX
} NVP6114_VMODE_T;

typedef enum {
    NVP6114_INPUT_VFMT_UNKNOWN = 0,
    NVP6114_INPUT_VFMT_SD,              ///< AHD    channel detect input video is SD NTSC/PAL
    NVP6114_INPUT_VFMT_SD_NTSC,         ///< SD     channel detect input video is SD NTSC
    NVP6114_INPUT_VFMT_SD_PAL,          ///< SD     channel detect input video is SD PAL
    NVP6114_INPUT_VFMT_AHD_30P,         ///< SD/AHD channel detect input video is AHD 30P
    NVP6114_INPUT_VFMT_AHD_25P,         ///< SD/AHD channel detect input video is AHD 25P
    NVP6114_INPUT_VFMT_MAX
} NVP6114_INPUT_VFMT_T;

typedef enum {
    AUDIO_SAMPLERATE_8K = 0,
    AUDIO_SAMPLERATE_16K,
}NVP6114_SAMPLERATE_T;

typedef enum {
    AUDIO_BITS_16B = 0,
    AUDIO_BITS_8B,
}NVP6114_SAMPLESIZE_T;

typedef struct {
    NVP6114_SAMPLERATE_T sample_rate;
    NVP6114_SAMPLESIZE_T sample_size;
    int ch_num;
} NVP6114_AUDIO_INIT_T;

/*************************************************************************************
 *   NVP6114 IOCTL
 *************************************************************************************/
struct nvp6114_ioc_data {
    int ch;         ///< access channel number ==> vin index
    int data;       ///< read/write data value
};

#define NVP6114_IOC_MAGIC       'n'

#define NVP6114_GET_NOVID       _IOR(NVP6114_IOC_MAGIC,   1, struct nvp6114_ioc_data)

#define NVP6114_GET_MODE        _IOR(NVP6114_IOC_MAGIC,   2, int)
#define NVP6114_SET_MODE        _IOWR(NVP6114_IOC_MAGIC,  3, int)

#define NVP6114_GET_CONTRAST    _IOR(NVP6114_IOC_MAGIC,   4, struct nvp6114_ioc_data)
#define NVP6114_SET_CONTRAST    _IOWR(NVP6114_IOC_MAGIC,  5, struct nvp6114_ioc_data)

#define NVP6114_GET_BRIGHTNESS  _IOR(NVP6114_IOC_MAGIC,   6, struct nvp6114_ioc_data)
#define NVP6114_SET_BRIGHTNESS  _IOWR(NVP6114_IOC_MAGIC,  7, struct nvp6114_ioc_data)

#define NVP6114_GET_SATURATION  _IOR(NVP6114_IOC_MAGIC,   8, struct nvp6114_ioc_data)
#define NVP6114_SET_SATURATION  _IOWR(NVP6114_IOC_MAGIC,  9, struct nvp6114_ioc_data)

#define NVP6114_GET_HUE         _IOR(NVP6114_IOC_MAGIC,  10, struct nvp6114_ioc_data)
#define NVP6114_SET_HUE         _IOWR(NVP6114_IOC_MAGIC, 11, struct nvp6114_ioc_data)

#define NVP6114_GET_SHARPNESS   _IOR(NVP6114_IOC_MAGIC,  12, struct nvp6114_ioc_data)
#define NVP6114_SET_SHARPNESS   _IOWR(NVP6114_IOC_MAGIC, 13, struct nvp6114_ioc_data)

#define NVP6114_GET_INPUT_VFMT  _IOR(NVP6114_IOC_MAGIC,  14, struct nvp6114_ioc_data)

#define NVP6114_GET_VOL         _IOR(NVP6114_IOC_MAGIC,  15, int)
#define NVP6114_SET_VOL         _IOWR(NVP6114_IOC_MAGIC, 16, int)

#define NVP6114_GET_OUT_CH      _IOR(NVP6114_IOC_MAGIC,  17, int)
#define NVP6114_SET_OUT_CH      _IOWR(NVP6114_IOC_MAGIC, 18, int)

/*************************************************************************************
 *  NVP6114 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int nvp6114_get_device_num(void);

int nvp6114_vin_to_ch(int id, int vin_idx);
int nvp6114_ch_to_vin(int id, int ch_idx);
int nvp6114_get_vch_id(int id, NVP6114_DEV_VPORT_T vport, int vport_seq);
int nvp6114_vin_to_vch(int id, int vin_idx);
u8  nvp6114_i2c_read(u8 id, u8 reg);
int nvp6114_i2c_write(u8 id, u8 reg, u8 data);
int nvp6114_i2c_array_write(u8 id, u8 addr, u8 *parray, u32 array_cnt);

int nvp6114_video_mode_support_check(int id, NVP6114_VMODE_T mode);
int nvp6114_video_get_mode(int id);
int nvp6114_video_get_contrast(int id, int ch);
int nvp6114_video_get_brightness(int id, int ch);
int nvp6114_video_get_saturation(int id, int ch);
int nvp6114_video_get_hue(int id, int ch);
int nvp6114_video_get_sharpness(int id, int ch);

int nvp6114_video_init(int id, NVP6114_VMODE_T mode);
int nvp6114_video_set_mode(int id, NVP6114_VMODE_T mode);
int nvp6114_video_set_contrast(int id, int ch, u8 value);
int nvp6114_video_set_brightness(int id, int ch, u8 value);
int nvp6114_video_set_saturation(int id, int ch, u8 value);
int nvp6114_video_set_hue(int id, int ch, u8 value);
int nvp6114_video_set_sharpness(int id, int ch, u8 value);

int nvp6114_status_get_novid(int id);
int nvp6114_status_get_input_video_format(int id, int ch);

int nvp6114_audio_set_mode(int id, NVP6114_VMODE_T mode, NVP6114_SAMPLESIZE_T samplesize, NVP6114_SAMPLERATE_T samplerate);
int nvp6114_audio_set_mute(int id, int on);
int nvp6114_audio_get_mute(int id);
int nvp6114_audio_set_volume(int id, int volume);
int nvp6114_audio_get_output_ch(int id);
int nvp6114_audio_set_output_ch(int id, int ch);

int  nvp6114_notify_vlos_register(int id, int (*nt_func)(int, int, int));
int  nvp6114_notify_vfmt_register(int id, int (*nt_func)(int, int));
void nvp6114_notify_vlos_deregister(int id);
void nvp6114_notify_vfmt_deregister(int id);
#endif

#endif  /* _FE_AHD_NVP6114_H_ */
