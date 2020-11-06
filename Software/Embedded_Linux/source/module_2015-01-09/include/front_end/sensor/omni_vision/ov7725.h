#ifndef _FE_SENSOR_OV7725_H_
#define _FE_SENSOR_OV7725_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  OV7725 Device Definition
 *************************************************************************************/
#define OV7725_DEV_MAX         1

typedef enum {
    OV7725_VMODE_VGA = 0,      ///< 640x480
    OV7725_VMODE_MAX
} OV7725_VMODE_T;

/*************************************************************************************
 *  OV7725 IOCTL
 *************************************************************************************/
#define OV7725_IOC_MAGIC       'o'

/*************************************************************************************
 *  OV7725 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int ov7725_get_device_num(void);
int ov7725_get_vch_id(int id);
int ov7725_i2c_write(u8 id, u8 reg, u8 data);
int ov7725_i2c_read(u8 id, u8 reg);
int ov7725_video_get_mode(int id);
int ov7725_video_set_mode(int id, OV7725_VMODE_T v_mode);
#endif

#endif  /* _FE_SENSOR_OV7725_H_ */
