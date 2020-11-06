#ifndef _FE_SENSOR_HM1375_H_
#define _FE_SENSOR_HM1375_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  HM1375 Device Definition
 *************************************************************************************/
#define HM1375_DEV_MAX              1

typedef enum {
    HM1375_VMODE_VGA = 0,           ///< 640x480
    HM1375_VMODE_HD720,             ///< 1280x720
    HM1375_VMODE_WXGA_960,          ///< 1280x960
    HM1375_VMODE_SXGA,              ///< 1280x1024
    HM1375_VMODE_MAX
} HM1375_VMODE_T;

/*************************************************************************************
 *  HM1375 IOCTL
 *************************************************************************************/
#define HM1375_IOC_MAGIC       'h'


/*************************************************************************************
 *  HM1375 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int hm1375_get_device_num(void);
int hm1375_get_vch_id(int id);
int hm1375_i2c_write(u8 id, u16 reg, u8 data);
int hm1375_i2c_read(u8 id, u16 reg);
int hm1375_video_get_mode(int id);
int hm1375_video_set_mode(int id, HM1375_VMODE_T v_mode);
#endif

#endif  /* _FE_SENSOR_HM1375_H_ */
