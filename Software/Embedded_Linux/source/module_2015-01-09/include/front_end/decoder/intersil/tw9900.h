#ifndef _FE_DEC_TW9900_H_
#define _FE_DEC_TW9900_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  TW9900 Device Definition
 *************************************************************************************/
#define TW9900_DEV_MAX                 4
#define TW9900_DEV_CH_MAX              1       ///< VIN#0

typedef enum {
    TW9900_DEV_VPORT_VD1 = 0,
    TW9900_DEV_VPORT_MAX
} TW9900_DEV_VPORT_T;

typedef enum {
    TW9900_VFMT_PAL = 0,               ///< video format PAL
    TW9900_VFMT_NTSC,                  ///< video format NTSC
    TW9900_VFMT_MAX
} TW9900_VFMT_T;

typedef enum {
    TW9900_VMODE_NTSC_720H_1CH = 0,    ///< 27MHz D1 1 channel
    TW9900_VMODE_PAL_720H_1CH,
    TW9900_VMODE_MAX
} TW9900_VMODE_T;

/*************************************************************************************
 *  TW9900 IOCTL
 *************************************************************************************/
#define TW9900_IOC_MAGIC            't'

#define TW9900_GET_NOVID            _IOR(TW9900_IOC_MAGIC,   1, int)

#define TW9900_GET_MODE   	    _IOR(TW9900_IOC_MAGIC,   2, int)

#define TW9900_GET_CONTRAST         _IOR(TW9900_IOC_MAGIC,   3, int)
#define TW9900_SET_CONTRAST         _IOWR(TW9900_IOC_MAGIC,  4, int)

#define TW9900_GET_BRIGHTNESS       _IOR(TW9900_IOC_MAGIC,   5, int)
#define TW9900_SET_BRIGHTNESS       _IOWR(TW9900_IOC_MAGIC,  6, int)

#define TW9900_GET_SATURATION_U     _IOR(TW9900_IOC_MAGIC,   7, int)
#define TW9900_SET_SATURATION_U     _IOWR(TW9900_IOC_MAGIC,  8, int)

#define TW9900_GET_SATURATION_V     _IOR(TW9900_IOC_MAGIC,   9, int)
#define TW9900_SET_SATURATION_V     _IOWR(TW9900_IOC_MAGIC, 10, int)

#define TW9900_GET_HUE              _IOR(TW9900_IOC_MAGIC,  11, int)
#define TW9900_SET_HUE              _IOWR(TW9900_IOC_MAGIC, 12, int)

/*************************************************************************************
 *  TW9900 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int tw9900_get_device_num(void);

int tw9900_get_vch_id(int id, TW9900_DEV_VPORT_T vport);
int tw9900_vin_to_vch(int id, int vin_idx);
u8  tw9900_i2c_read(u8 id, u8 reg);
int tw9900_i2c_write(u8 id, u8 reg, u8 data);

int tw9900_video_mode_support_check(int id, TW9900_VMODE_T mode);
int tw9900_video_get_mode(int id);
int tw9900_video_get_contrast(int id);
int tw9900_video_get_brightness(int id);
int tw9900_video_get_saturation_u(int id);
int tw9900_video_get_saturation_v(int id);
int tw9900_video_get_hue(int id);

int tw9900_video_set_mode(int id, TW9900_VMODE_T mode);
int tw9900_video_set_contrast(int id, u8 value);
int tw9900_video_set_brightness(int id, u8 value);
int tw9900_video_set_saturation_u(int id, u8 value);
int tw9900_video_set_saturation_v(int id, u8 value);
int tw9900_video_set_hue(int id, u8 value);

int tw9900_status_get_novid(int id);

int  tw9900_notify_vlos_register(int id, int (*nt_func)(int, int, int));
void tw9900_notify_vlos_deregister(int id);
#endif

#endif  /* _FE_DEC_TW9900_H_ */
