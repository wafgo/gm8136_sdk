#ifndef _FE_SDI_MV4101_H_
#define _FE_SDI_MV4101_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  MV4101 Device Definition
 *************************************************************************************/
#define MV4101_DEV_MAX                 4
#define MV4101_DEV_CH_MAX              4

typedef enum{
	MV4101_CHIP_TYPE_A = 0,
	MV4101_CHIP_TYPE_A2,
	MV4101_CHIP_TYPE_B,
	MV4101_CHIP_TYPE_B2,
	MV4101_CHIP_TYPE_C,
	MV4101_CHIP_TYPE_UNKNOWN
} MV4101_CHIP_TYPE_T;

typedef enum {
    MV4101_DEV_VOUT_0 = 0,
    MV4101_DEV_VOUT_1,
    MV4101_DEV_VOUT_2,
    MV4101_DEV_VOUT_3,
    MV4101_DEV_VOUT_MAX
} MV4101_DEV_VOUT_T;

typedef enum{
	MV4101_VOUT_FMT_BT656 = 0,
	MV4101_VOUT_FMT_BT1120,
	MV4101_VOUT_FMT_MAX
} MV4101_VOUT_FMT_T;

typedef enum {
    MV4101_VFMT_UNKNOWN = 0x0,
	MV4101_VFMT_1080I60,
	MV4101_VFMT_1080I59,
	MV4101_VFMT_1080I50,
	MV4101_VFMT_1080P30,
	MV4101_VFMT_1080P29,
	MV4101_VFMT_1080P25,
	MV4101_VFMT_1080P24,
	MV4101_VFMT_1080P23,
	MV4101_VFMT_720P60,
	MV4101_VFMT_720P59,
	MV4101_VFMT_720P50,
	MV4101_VFMT_720P30,
	MV4101_VFMT_720P29,
	MV4101_VFMT_720P25,
	MV4101_VFMT_720P24,
	MV4101_VFMT_720P23,
	MV4101_VFMT_MAX
} MV4101_VFMT_T;

typedef enum{
    MV4101_EQ_MODE_SW_AUTO = 0,
    MV4101_EQ_MODE_HW_AUTO,
	MV4101_EQ_MODE_A_LINE_0,
	MV4101_EQ_MODE_A_LINE_1,
	MV4101_EQ_MODE_A_LINE_2,
	MV4101_EQ_MODE_A_LINE_3,
	MV4101_EQ_MODE_A_LINE_4,
	MV4101_EQ_MODE_A_LINE_5,
	MV4101_EQ_MODE_B_LINE_0,
	MV4101_EQ_MODE_B_LINE_1,
	MV4101_EQ_MODE_B_LINE_2,
	MV4101_EQ_MODE_B_LINE_3,
	MV4101_EQ_MODE_B_LINE_4,
	MV4101_EQ_MODE_B_LINE_5,
	MV4101_EQ_MODE_MAX
} MV4101_EQ_MODE_T;

struct mv4101_video_fmt_t {
    unsigned int fmt_idx;
    unsigned int active_width;
    unsigned int active_height;
    unsigned int total_width;
    unsigned int total_height;
    unsigned int pixel_rate;
    unsigned int bit_width;
    unsigned int prog;
    unsigned int frame_rate;
    unsigned int bit_rate;      ///< 0:270Mbps, 1:1.485Gbps, 2:2.97Gbps
};

typedef enum {
    MV4101_AUDIO_RATE_32000 = 0,
    MV4101_AUDIO_RATE_44100,
    MV4101_AUDIO_RATE_48000,
    MV4101_AUDIO_RATE_MAX,
} MV4101_AUDIO_RATE_T;

typedef enum {
    MV4101_AUDIO_WIDTH_16BIT = 0,
    MV4101_AUDIO_WIDTH_20BIT,
    MV4101_AUDIO_WIDTH_24BIT,
    MV4101_AUDIO_WIDTH_BIT_AUTO,
    MV4101_AUDIO_WIDTH_MAX,
} MV4101_AUDIO_BIT_WIDTH_T;

typedef enum {
    MV4101_AUDIO_SOUND_MUTE_L = 0,
    MV4101_AUDIO_SOUND_MUTE_R,
    MV4101_AUDIO_SOUND_MUTE_ALL,
    MV4101_AUDIO_SOUND_MUTE_DEFAULT,
    MV4101_AUDIO_SOUND_MUTE_MAX
} MV4101_AUDIO_MUTE_T;

/*************************************************************************************
 *  MV4101 IOCTL
 *************************************************************************************/
struct mv4101_ioc_data_t {
    int sdi_ch;                 ///< access sdi channel number
    int data;                   ///< read/write data value
};

struct mv4101_ioc_vfmt_t {
    int          sdi_ch;        ///< sdi channel number
    unsigned int active_width;
    unsigned int active_height;
    unsigned int total_width;
    unsigned int total_height;
    unsigned int pixel_rate;    ///< KHZ
    unsigned int bit_width;
    unsigned int prog;          ///< 0:interlace, 1: progressive
    unsigned int frame_rate;
    unsigned int bit_rate;      ///< 0:270Mbps, 1:1.485Gbps, 2:2.97Gbps
};

#define MV4101_IOC_MAGIC       'm'

#define MV4101_GET_NOVID        _IOR(MV4101_IOC_MAGIC,   1, struct mv4101_ioc_data_t)
#define MV4101_GET_VIDEO_FMT    _IOR(MV4101_IOC_MAGIC,   2, struct mv4101_ioc_vfmt_t)

/*************************************************************************************
 *  MV4101 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int  mv4101_get_device_num(void);

u32 mv4101_reg_read(int id, unsigned int reg);
int mv4101_reg_write(int id, unsigned int reg, unsigned int data);

int mv4101_init_chip(int id, MV4101_VOUT_FMT_T vout_fmt, MV4101_EQ_MODE_T eq_mode);
int mv4101_get_chip_type(int id);
int mv4101_get_vout_format(int id);
int mv4101_get_vch_id(int id, MV4101_DEV_VOUT_T vout, int sdi_ch);

int mv4101_status_get_video_format(int id, int ch);
int mv4101_status_get_flywheel_lock(int id, int ch);
int mv4101_status_get_video_loss(int id, int ch);

int mv4101_set_eq_mode(int id, int ch, MV4101_EQ_MODE_T mode);
int mv4101_set_vout_sdi_src(int id, MV4101_DEV_VOUT_T vout, int sdi_ch);
int mv4101_get_vout_sdi_src(int id, MV4101_DEV_VOUT_T vout);
int mv4101_audio_set_bit_width(int id, int ch, MV4101_AUDIO_BIT_WIDTH_T bit_width);
int mv4101_audio_get_rate(int id, int ch);
int mv4101_audio_set_mute(int id, int ch, MV4101_AUDIO_MUTE_T mute);

int  mv4101_notify_vfmt_register(int id, int (*nt_func)(int, int, struct mv4101_video_fmt_t *));
void mv4101_notify_vfmt_deregister(int id);
int  mv4101_notify_vlos_register(int id, int (*nt_func)(int, int, int));
void mv4101_notify_vlos_deregister(int id);
#endif

#endif  /* _FE_SDI_MV4101_H_ */
