#ifndef __LCD_DEF_H__
#define __LCD_DEF_H__

#include "lcd_fb.h"

#ifdef CONFIG_PLATFORM_GM8210
#define LCD_IP_NUM    3         /* Number of LCD controllers */
#elif defined(CONFIG_PLATFORM_GM8287)
#define LCD_IP_NUM    3         /* Number of LCD controllers, 0/2 are valid, 1 is dummy */
#elif (defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136))
#define LCD_IP_NUM    1         /* Number of LCD controllers */
#else
#error "Please define LCDC count"
#endif

#define VIN_RES_T       vin_res_t
#define OUTPUT_TYPE_T   output_type_t
#define LCD_SCREEN_ON   1
#define LCD_SCREEN_OFF  0

/*
 * Support various video output type(dev_info.video_output_type)
 */
typedef enum {
    VOT_LCD = 0,
    VOT_640x480,
    VOT_800x600,
    VOT_1024x768,
    VOT_1280x800,
    VOT_1280x960,
    VOT_1280x1024,
    VOT_1360x768,
    VOT_1600x1200,
    VOT_1680x1050,
    VOT_1920x1080,
    VOT_1440x900,
    VOT_1280x720,
    VOT_CCIR656 = 16,
    VOT_PAL = 16,
    VOT_NTSC,
    VOT_480P,
    VOT_576P,
    VOT_720P,
    VOT_1440x1080I,
    VOT_1920x1080P,
    VOT_1024x768_30I,
    VOT_1024x768_25I,
    VOT_1024x768P,
    VOT_1280x1024P,
    VOT_1680x1050P,
    VOT_1440x900P,
    VOT_1440x960_30I,
    VOT_1440x1152_25I,
    VOT_1280x1024_30I,
    VOT_1280x1024_25I,
    VOT_1920x1080I,
} VOT_RES_T;

typedef enum {
    LCD_ID = 0,
    LCD1_ID,
    LCD2_ID,
    LCD_MAX_ID
} LCD_IDX_T;

#endif /* __LCD_DEF_H__ */
