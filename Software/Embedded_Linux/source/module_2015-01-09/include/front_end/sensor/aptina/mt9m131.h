#ifndef _FE_SENSOR_MT9M131_H_
#define _FE_SENSOR_MT9M131_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  MT9M131 Device Definition
 *************************************************************************************/
#define MT9M131_DEV_MAX         2

typedef enum {
    MT9M131_VMODE_VGA = 0,      ///< 640x480
    MT9M131_VMODE_QSXGA,        ///< 640x512
    MT9M131_VMODE_SXGA,         ///< 1280x1024
    MT9M131_VMODE_MAX
} MT9M131_VMODE_T;

/*************************************************************************************
 *  MT9M131 IOCTL
 *************************************************************************************/
#define MT9M131_IOC_MAGIC       'm'

/*************************************************************************************
 *  MT9M131 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int mt9m131_get_device_num(void);
int mt9m131_get_vch_id(int id);
int mt9m131_i2c_write(u8 id, u8 reg, u16 data);
int mt9m131_i2c_read(u8 id, u8 reg);
int mt9m131_video_get_mode(int id);
int mt9m131_video_set_mode(int id, MT9M131_VMODE_T v_mode);
#endif

#endif  /* _FE_SENSOR_MT9M131_H_ */
