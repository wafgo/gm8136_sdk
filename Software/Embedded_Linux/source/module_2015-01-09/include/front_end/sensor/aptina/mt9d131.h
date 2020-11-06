#ifndef _FE_SENSOR_MT9D131_H_
#define _FE_SENSOR_MT9D131_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  MT9D131 Device Definition
 *************************************************************************************/
#define MT9D131_DEV_MAX             1

typedef enum {
    MT9D131_VMODE_SVGA = 0,         ///< 800x600
    MT9D131_VMODE_HD720,            ///< 1280x720
    MT9D131_VMODE_WXGA,             ///< 1280x800
    MT9D131_VMODE_UXGA,             ///< 1600x1200
    MT9D131_VMODE_MAX
} MT9D131_VMODE_T;

/*************************************************************************************
 *  MT9D131 IOCTL
 *************************************************************************************/
#define MT9D131_IOC_MAGIC       'm'

/*************************************************************************************
 *  MT9D131 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int mt9d131_get_device_num(void);
int mt9d131_get_vch_id(int id);
int mt9d131_i2c_write(u8 id, u8 reg, u16 data);
int mt9d131_i2c_read(u8 id, u8 reg);
int mt9d131_video_get_mode(int id);
int mt9d131_video_set_mode(int id, MT9D131_VMODE_T v_mode);
#endif

#endif  /* _FE_SENSOR_MT9D131_H_ */
