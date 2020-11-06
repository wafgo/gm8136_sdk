#ifndef _FE_SDI_CX25930_H_
#define _FE_SDI_CX25930_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  CX25930 Device Definition
 *************************************************************************************/
#define CX25930_DEV_MAX                 4
#define CX25930_DEV_CH_MAX              4

typedef enum {
    CX25930_DEV_VOUT_0 = 0,
    CX25930_DEV_VOUT_1,
    CX25930_DEV_VOUT_2,
    CX25930_DEV_VOUT_3,
    CX25930_DEV_VOUT_MAX
} CX25930_DEV_VOUT_T;

typedef enum {
    CX25930_CAB_LEN_0_50M_AUTO = 0,
    CX25930_CAB_LEN_75_100M,
    CX25930_CAB_LEN_125_150M,
    CX25930_CAB_LEN_MAX
} CX25930_CAB_LEN_T;

typedef enum {
    CX25930_INPUT_RATE_SD = 0,
    CX25930_INPUT_RATE_HD,
    CX25930_INPUT_RATE_GEN3,
    CX25930_INPUT_RATE_MAX
} CX25930_INPUT_RATE_T;

typedef enum {
    CX25930_VOUT_FORMAT_BT656 = 0,
    CX25930_VOUT_FORMAT_BT1120,
    CX25930_VOUT_FORMAT_MAX
} CX25930_VOUT_FORMAT_T;

typedef enum {
    CX25930_LBK_OFF = 0,
    CX25930_LBK_EQ,
    CX25930_LBK_CDR,
    CX25930_LBK_MAX
} CX25930_LBK_T;

struct cx25930_video_fmt_t {
    unsigned int fmt_idx;
    unsigned int active_width;
    unsigned int active_height;
    unsigned int total_width;
    unsigned int total_height;
    unsigned int pixel_rate;    ///< KHz
    unsigned int bit_width;
    unsigned int prog;
    unsigned int frame_rate;
    unsigned int bit_rate;      ///< 0:270Mbps, 1:1.485Gbps, 2:2.97Gbps
};

#define CX25930_VIDEO_FMT_IDX_MAX   0x16

/*************************************************************************************
 *  CX25930 IOCTL
 *************************************************************************************/
struct cx25930_ioc_data_t {
    int sdi_ch;     ///< access sdi channel number
    int data;       ///< read/write data value
};

struct cx25930_ioc_vfmt_t {
    int          sdi_ch;        ///< sdi channel number
    unsigned int active_width;
    unsigned int active_height;
    unsigned int total_width;
    unsigned int total_height;
    unsigned int pixel_rate;    ///< KHz
    unsigned int bit_width;
    unsigned int prog;          ///< 0:interlace, 1: progressive
    unsigned int frame_rate;
    unsigned int bit_rate;      ///< 0:270Mbps, 1:1.485Gbps, 2:2.97Gbps
};

#define CX25930_IOC_MAGIC       'c'

#define CX25930_GET_NOVID       _IOR(CX25930_IOC_MAGIC,   1, struct cx25930_ioc_data_t)
#define CX25930_GET_VIDEO_FMT	_IOR(CX25930_IOC_MAGIC,   2, struct cx25930_ioc_vfmt_t)

/*************************************************************************************
 *  CX25930 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int  cx25930_get_device_num(void);
void *cx25930_get_cxcomm_handle(u8 id);

int  cx25930_get_vch_id(int id, CX25930_DEV_VOUT_T vout, int sdi_ch);
int  cx25930_get_vout_format(int id);

int  cx25930_i2c_write(u8 id, u16 reg, u32 data, int size);
int  cx25930_i2c_read(u8 id, u16 reg, void *data, int size);
int  cx25930_i2c_array_write(u8 id, u16 reg, void *parray, int array_size);  ///< for burst write
int  cx25930_i2c_array_read(u8 id, u16 reg, void *parray, int array_size);   ///< for burst read

int  cx25930_status_get_video_format(int id, int ch, struct cx25930_video_fmt_t *vfmt);
int  cx25930_status_get_video_loss(int id, int ch);

int  cx25930_notify_vfmt_register(int id, int (*nt_func)(int, int, struct cx25930_video_fmt_t *));
void cx25930_notify_vfmt_deregister(int id);
int  cx25930_notify_vlos_register(int id, int (*nt_func)(int, int, int));
void cx25930_notify_vlos_deregister(int id);

#endif

#endif  /* _FE_SDI_CX25930_H_ */
