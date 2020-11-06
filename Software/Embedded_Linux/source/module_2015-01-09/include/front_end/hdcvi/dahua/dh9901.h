#ifndef _FE_HDCVI_DH9901_H_
#define _FE_HDCVI_DH9901_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  DH9901 Device Definition
 *************************************************************************************/
#define DH9901_DEV_MAX                 4
#define DH9901_DEV_CH_MAX              4

typedef enum {
    DH9901_DEV_VOUT_0 = 0,
    DH9901_DEV_VOUT_1,
    DH9901_DEV_VOUT_2,
    DH9901_DEV_VOUT_3,
    DH9901_DEV_VOUT_MAX
} DH9901_DEV_VOUT_T;

typedef enum {
    DH9901_VOUT_FORMAT_BT656_DUAL_HEADER = 0,      ///< BT656 8BIT Dual Header
    DH9901_VOUT_FORMAT_BT1120,                     ///< BT1120 16BIT
    DH9901_VOUT_FORMAT_MAX
} DH9901_VOUT_FORMAT_T;

typedef enum {
    DH9901_VFMT_720P25 = 0,
    DH9901_VFMT_720P30,
    DH9901_VFMT_720P50,
    DH9901_VFMT_720P60,
    DH9901_VFMT_1080P25,
    DH9901_VFMT_1080P30,
    DH9901_VFMT_MAX
} DH9901_VFMT_T;

typedef enum {
    DH9901_PTZ_PTOTOCOL_DHSD1 = 0,
    DH9901_PTZ_PTOTOCOL_MAX
} DH9901_PTZ_PTOTOCOL_T;

typedef enum {
    DH9901_PTZ_BAUD_1200 = 0,
    DH9901_PTZ_BAUD_2400,
    DH9901_PTZ_BAUD_4800,
    DH9901_PTZ_BAUD_9600,
    DH9901_PTZ_BAUD_19200,
    DH9901_PTZ_BAUD_38400,
    DH9901_PTZ_BAUD_MAX
} DH9901_PTZ_BAUD_T;

typedef enum {
    DH9901_CABLE_TYPE_COAXIAL = 0,
    DH9901_CABLE_TYPE_UTP_10OHM,
    DH9901_CABLE_TYPE_UTP_17OHM,
    DH9901_CABLE_TYPE_UTP_25OHM,
    DH9901_CABLE_TYPE_UTP_35OHM,
    DH9901_CABLE_TYPE_MAX
} DH9901_CABLE_TYPE_T;

typedef enum {
    DH9901_AUDIO_SAMPLERATE_8K = 0,
    DH9901_AUDIO_SAMPLERATE_16K,
} DH9901_AUDIO_SAMPLERATE_T;

typedef enum {
    DH9901_AUDIO_SAMPLESIZE_8BITS = 0,
    DH9901_AUDIO_SAMPLESIZE_16BITS,
} DH9901_AUDIO_SAMPLESIZE_T;

struct dh9901_video_fmt_t {
    DH9901_VFMT_T fmt_idx;       ///< video format index
    unsigned int  width;         ///< video source width
    unsigned int  height;        ///< video source height
    unsigned int  prog;          ///< 0:interlace 1:progressive
    unsigned int  frame_rate;    ///< currect frame rate
};

struct dh9901_ptz_t {
    DH9901_PTZ_PTOTOCOL_T   protocol;   ///< PTZ protocol, 0: DH_SD1
    DH9901_PTZ_BAUD_T       baud_rate;  ///< PTZ RS485 transmit baud rate, default:9600
    int                     parity_chk; ///< PTZ RS485 transmit parity check, 0:disable, 1:enable
};

/*************************************************************************************
 *  DH9901 COMM Definition
 *************************************************************************************/
typedef enum {
    DH9901_COMM_CMD_UNKNOWN = 0,
    DH9901_COMM_CMD_ACK_OK,
    DH9901_COMM_CMD_ACK_FAIL,
    DH9901_COMM_CMD_GET_VIDEO_STATUS,
    DH9901_COMM_CMD_SET_COLOR,
    DH9901_COMM_CMD_GET_COLOR,
    DH9901_COMM_CMD_CLEAR_EQ,
    DH9901_COMM_CMD_SEND_RS485,
    DH9901_COMM_CMD_SET_VIDEO_POS,
    DH9901_COMM_CMD_GET_VIDEO_POS,
    DH9901_COMM_CMD_SET_CABLE_TYPE,
    DH9901_COMM_CMD_SET_AUDIO_VOL,
    DH9901_COMM_CMD_GET_PTZ_CFG,
    DH9901_COMM_CMD_MAX
} DH9901_COMM_CMD_T;

#define DH9901_COMM_MSG_MAGIC           0x9901
#define DH9901_COMM_MSG_BUF_MAX         384

struct dh9901_comm_header_t {
    unsigned int magic;                 ///< DH9901_COMM_MAGIC
    unsigned int cmd_id;                ///< command index
    unsigned int data_len;              ///< data buffer length
    unsigned int dev_id;                ///< access device index
    unsigned int dev_ch;                ///< access device channel
    unsigned int reserved[2];           ///< reserve byte
};

struct dh9901_comm_send_rs485_t {
    unsigned char buf_len;
    unsigned char cmd_buf[256];
};

/*************************************************************************************
 *  DH9901 IOCTL
 *************************************************************************************/
struct dh9901_ioc_data_t {
    int cvi_ch;                 ///< access cvi channel number
    int data;                   ///< read/write data value
};

struct dh9901_ioc_vfmt_t {
    int          cvi_ch;        ///< cvi channel number
    unsigned int width;         ///< cvi channel video width
    unsigned int height;        ///< cvi channel video height
    unsigned int prog;          ///< 0:interlace, 1: progressive
    unsigned int frame_rate;    ///< currect frame rate
};

struct dh9901_ioc_vcolor_t {
    int           cvi_ch;       ///< cvi channel number
    unsigned char brightness;   ///< 0 ~ 100, default 50
    unsigned char contrast;     ///< 0 ~ 100, default 50
    unsigned char saturation;   ///< 0 ~ 100, default 50
    unsigned char hue;          ///< 0 ~ 100, default 50
    unsigned char gain;         ///< not support
    unsigned char white_balance;///< not support
    unsigned char sharpness;    ///< 0 ~ 15, default 1
    unsigned char reserved;
};

struct dh9901_ioc_vpos_t {
    int cvi_ch;                 ///< cvi channel number
    int h_offset;
    int v_offset;
    int reserved[2];
};

struct dh9901_ioc_cable_t {
    int cvi_ch;                 ///< cvi channel number
    DH9901_CABLE_TYPE_T cab_type;
};

struct dh9901_ioc_rs485_tx_t {
    int           cvi_ch;       ///< cvi channel number
    unsigned char buf_len;      ///< command buffer length
    unsigned char cmd_buf[256]; ///< command data
};

struct dh9901_ioc_audio_vol_t {
    int           cvi_ch;       ///< cvi channel number
    unsigned char volume;
};

#define DH9901_IOC_MAGIC            'd'

#define DH9901_GET_NOVID            _IOR(DH9901_IOC_MAGIC,   1, struct dh9901_ioc_data_t)
#define DH9901_GET_VIDEO_FMT        _IOR(DH9901_IOC_MAGIC,   2, struct dh9901_ioc_vfmt_t)

#define DH9901_GET_VIDEO_COLOR      _IOR(DH9901_IOC_MAGIC,   3, struct dh9901_ioc_vcolor_t)
#define DH9901_SET_VIDEO_COLOR      _IOWR(DH9901_IOC_MAGIC,  4, struct dh9901_ioc_vcolor_t)

#define DH9901_GET_VIDEO_POS        _IOR(DH9901_IOC_MAGIC,   5, struct dh9901_ioc_vpos_t)
#define DH9901_SET_VIDEO_POS        _IOWR(DH9901_IOC_MAGIC,  6, struct dh9901_ioc_vpos_t)

#define DH9901_SET_CABLE_TYPE       _IOWR(DH9901_IOC_MAGIC,  7, struct dh9901_ioc_cable_t)
#define DH9901_RS485_TX             _IOWR(DH9901_IOC_MAGIC,  8, struct dh9901_ioc_rs485_tx_t)
#define DH9901_CLEAR_EQ             _IOWR(DH9901_IOC_MAGIC,  9, int)    ///< parameter is cvi channel number

#define DH9901_SET_AUDIO_VOL        _IOWR(DH9901_IOC_MAGIC, 10, struct dh9901_ioc_audio_vol_t)

/*************************************************************************************
 *  DH9901 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int  dh9901_get_device_num(void);
int  dh9901_get_vch_id(int id, DH9901_DEV_VOUT_T vout, int cvi_ch);
int  dh9901_get_vout_format(int id);
int  dh9901_vin_to_vch(int id, int vin_idx);

int  dh9901_notify_vfmt_register(int id, int (*nt_func)(int, int, struct dh9901_video_fmt_t *));
void dh9901_notify_vfmt_deregister(int id);
int  dh9901_notify_vlos_register(int id, int (*nt_func)(int, int, int));
void dh9901_notify_vlos_deregister(int id);
#endif

#endif  /* _FE_HDCVI_DH9901_H_ */
