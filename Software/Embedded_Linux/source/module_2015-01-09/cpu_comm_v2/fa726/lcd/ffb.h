#ifndef __FFB_H__
#define __FFB_H__

#include <linux/fb.h>
#include <linux/types.h>

struct ffb_rgb {
    struct fb_bitfield red;
    struct fb_bitfield green;
    struct fb_bitfield blue;
    struct fb_bitfield transp;
};

struct ffb_rect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

#define RGB_8	(0)
#define RGB_16	(1)
#define RGB_32	(2)
#define ARGB	(3)
#define NR_RGB	 4

/* Support video input mode */
/* 16-31 RGB mode */
enum { VIM_YUV422, VIM_YUV420, VIM_RGB = 16, VIM_ARGB =
        16, VIM_RGB888, VIM_RGB565, VIM_RGB555, VIM_RGB444, VIM_RGB8, VIM_RGB1555
};

struct ffb_info {
    int index;  /* start from 0 */
    int video_input_mode;
    struct fb_info fb;
    struct ffb_rgb *rgb[NR_RGB];
    struct ffb_ops *ops;
};

#endif /* __FFB_H__ */
