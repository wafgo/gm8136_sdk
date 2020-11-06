#ifndef __FFB_H__
#define __FFB_H__

#include <linux/fb.h>
#include <linux/types.h>

#define LCD_IP_NUM  3

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
#define RGB_1	(4)
#define RGB_2	(5)
#define NR_RGB	 6

/*
 * Minimum X and Y resolutions
 */
#define MIN_XRES	64
#define MIN_YRES	64

/* Support video input mode */
/* 16-31 RGB mode */
enum { VIM_YUV422, VIM_YUV420, VIM_RGB = 16, VIM_ARGB =
        16, VIM_RGB888, VIM_RGB565, VIM_RGB555, VIM_RGB444, VIM_RGB8, VIM_RGB1555, VIM_RGB1BPP, VIM_RGB2BPP
};

struct lcd200_dev_cursor {
    u32 dx;
    u32 dy;
    u16 enable;
};

/* pip plane */
typedef struct ffb_info {
    struct fb_info fb;  /* must be the first member */
    int   index;  /* start from 0 */
    u_int xres;
    u_int yres;
    int video_input_mode;
    dma_addr_t  fb_addr;
    void        *fb_addr_va;
    struct ffb_rgb *rgb[NR_RGB];
    struct ffb_ops *ops;
    void *lcd_info;
    int  active;
} ffb_info_t;

#define MAX_FBNUM   3   /* 0/1/2 */

typedef struct lcd_info {
    int index;  /* start from 0 */
    char name[20];
    u_int max_xres;
    u_int max_yres;
    dma_addr_t  io_base_pa;  //io base
    void        *io_base; //virtual io base
    ffb_info_t  fb[MAX_FBNUM];  /* 0/1/2 */
    struct lcd200_dev_cursor    cursor;
    int active; /* 1 for active, 0 for inactive */
    unsigned int color_key1;
    unsigned int color_key2;
    void *soc_info; //lcd_info_t
    u16 *palette_cpu;
    int palette_size;
    /* scalar */
    struct {
        u_int out_xres;
        u_int out_yres;
        u_int enable;
    } scalar;
    struct proc_dir_entry *ffb_proc_root;
} lcd_info_t;


#endif /* __FFB_H__ */
