#ifndef _FE_DEC_CX26848_H_
#define _FE_DEC_CX26848_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  CX26848 Device Definition
 *************************************************************************************/
#define CX26848_DEV_MAX                 4
#define CX26848_DEV_CH_MAX              8   ///< VIN#0~7

typedef enum {
    CX26848_DEV_VPORT_A = 0,
    CX26848_DEV_VPORT_B,
    CX26848_DEV_VPORT_C,
    CX26848_DEV_VPORT_D,
    CX26848_DEV_VPORT_MAX
} CX26848_DEV_VPORT_T;

typedef enum {
    CX26848_VMODE_NTSC_720H_1CH = 0,    ///< 27MHz  D1 1 channel
    CX26848_VMODE_NTSC_720H_2CH,        ///< 27MHz  D1 2 channel
    CX26848_VMODE_NTSC_720H_4CH,        ///< 108MHz D1 4 channel
    CX26848_VMODE_NTSC_960H_1CH,        ///< 36MHz  D1 1 channel
    CX26848_VMODE_NTSC_960H_2CH,        ///< 36MHz  D1 2 channel
    CX26848_VMODE_NTSC_960H_4CH,        ///< 144MHz D1 4 channel

    CX26848_VMODE_PAL_720H_1CH,
    CX26848_VMODE_PAL_720H_2CH,
    CX26848_VMODE_PAL_720H_4CH,
    CX26848_VMODE_PAL_960H_1CH,
    CX26848_VMODE_PAL_960H_2CH,
    CX26848_VMODE_PAL_960H_4CH,
    CX26848_VMODE_MAX
} CX26848_VMODE_T;

/*************************************************************************************
 *  CX26848 IOCTL
 *************************************************************************************/
struct cx26848_ioc_data {
    int ch;         ///< access channel number => vin index
    int data;       ///< read/write data value
};

#define CX26848_IOC_MAGIC            'c'

#define CX26848_GET_NOVID       _IOR(CX26848_IOC_MAGIC,   1, struct cx26848_ioc_data)

#define CX26848_GET_MODE        _IOR(CX26848_IOC_MAGIC,   2, int)

#define CX26848_GET_CONTRAST    _IOR(CX26848_IOC_MAGIC,   3, struct cx26848_ioc_data)
#define CX26848_SET_CONTRAST    _IOWR(CX26848_IOC_MAGIC,  4, struct cx26848_ioc_data)

#define CX26848_GET_BRIGHTNESS  _IOR(CX26848_IOC_MAGIC,   5, struct cx26848_ioc_data)
#define CX26848_SET_BRIGHTNESS  _IOWR(CX26848_IOC_MAGIC,  6, struct cx26848_ioc_data)

#define CX26848_GET_SATURATION  _IOR(CX26848_IOC_MAGIC,   7, struct cx26848_ioc_data)
#define CX26848_SET_SATURATION  _IOWR(CX26848_IOC_MAGIC,  8, struct cx26848_ioc_data)

#define CX26848_GET_HUE         _IOR(CX26848_IOC_MAGIC,   9, struct cx26848_ioc_data)
#define CX26848_SET_HUE         _IOWR(CX26848_IOC_MAGIC, 10, struct cx26848_ioc_data)

#define CX26848_GET_VOL         _IOR(CX26848_IOC_MAGIC,  11, int)
#define CX26848_SET_VOL         _IOWR(CX26848_IOC_MAGIC, 12, int)

#define CX26848_GET_OUT_CH      _IOR(CX26848_IOC_MAGIC,  13, int)
#define CX26848_SET_OUT_CH      _IOWR(CX26848_IOC_MAGIC, 14, int)

/*************************************************************************************
 *  CX26848 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
void *cx26848_get_cxcomm_handle(u8 id);

int cx26848_get_device_num(void);
int cx26848_get_vch_id(int id, CX26848_DEV_VPORT_T vport, int vport_seq);
int cx26848_vin_to_vch(int id, int vin_idx);
int cx26848_vin_to_ch(int id, int vin_idx);
int cx26848_ch_to_vin(int id, int ch_idx);
int cx26848_i2c_write(u8 id, u16 reg, u32 data, int size);
int cx26848_i2c_read(u8 id, u16 reg, void *data, int size);

int cx26848_video_mode_support_check(int id, CX26848_VMODE_T mode);
int cx26848_video_get_mode(u8 id);
int cx26848_video_get_contrast(int id, int ch);
int cx26848_video_get_brightness(int id, int ch);
int cx26848_video_get_saturation(int id, int ch);
int cx26848_video_get_hue(int id, int ch);

int cx26848_video_set_mode(u8 id, CX26848_VMODE_T v_mode);
int cx26848_video_set_contrast(int id, int ch, int value);
int cx26848_video_set_brightness(int id, int ch, int value);
int cx26848_video_set_saturation(int id, int ch, int value);
int cx26848_video_set_hue(int id, int ch, int value);

int cx26848_status_get_novid(int id);

int  cx26848_notify_vlos_register(int id, int (*nt_func)(int, int, int));
void cx26848_notify_vlos_deregister(int id);

#endif

#endif  /* _FE_DEC_CX26848_H_ */
