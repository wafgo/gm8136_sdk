#ifndef __FFB_H
#define __FFB_H

#include "lcd_def.h"

#define FFB_VERSION      "0.1.20"
#define FFB_MODULE_NAME  "GM_ffb"

#define PFX		 FFB_MODULE_NAME
#define PIP_NUM  3

#define MAX_VBI_BLINE   64      //max vbi blanking line number
#define VBI_FB_IDX      0       //vbi use which frame buffer, 0/1/2

//for compress/descompress
#define WRITE_LIMIT_RATIO   100

#include <linux/fb.h>
#include <linux/types.h>
#include "ffb_api.h"

struct ffb_rect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

struct ffb_rgb {
    struct fb_bitfield red;
    struct fb_bitfield green;
    struct fb_bitfield blue;
    struct fb_bitfield transp;
};

#define RGB_8	(0)
#define RGB_16	(1)
#define RGB_32	(2)
#define ARGB	(3)
#define RGB_1	(4)
#define RGB_2	(5)
#define NR_RGB	 6

#define FFB_PARAM_INPUT  0
#define FFB_PARAM_OUTPUT 1

struct ffb_buf_offset {
    uint32_t y;
    uint32_t u;
    uint32_t v;
};
/* For input resolution
 */
typedef struct {
    int input_res;              //VIN_RES_T
    u_short xres;
    u_short yres;
    char *name;
} ffb_input_res_t;

/* for output resolution information */
typedef struct {
    OUTPUT_TYPE_T output_type;
    char *name;
} ffb_output_name_t;

/* resoluiton
 */
typedef struct {
    u_short xres;
    u_short yres;
} ffb_res_t;

struct ffb_timing_info {
    union {
        /* LCD */
        struct {
            u_long pixclock;
            ffb_res_t in;       //input resolution
            ffb_res_t out;      //output resolution
            u_short hsync_len;
            u_short left_margin;        //H Back Porch
            u_short right_margin;       //H Front Porch
            u_short vsync_len;
            u_short upper_margin;       //V Back Porch
            u_short lower_margin;       //V Front Porch
            u_short sp_param;   //Serial Panel Parameters
        } lcd;

        /* CCIR656 */
        struct {
            u_long pixclock;
            ffb_res_t in;       //input resolution
            ffb_res_t out;      //output resolution
            u_short field0_switch;
            u_short field1_switch;
            u_short hblank_len;
            u_short vblank0_len;
            u_short vblank1_len;
            u_short vblank2_len;
        } ccir656;
    } data;
    u_char otype;
};

#define CCIR656CONFIG(SYS,INFMT,CLK_INV) ((SYS)|(INFMT)|((CLK_INV)&0x01)<<1)
#define CCIR656_SYS_MASK        0x8
#define CCIR656_SYS_SDTV        0
#define CCIR656_SYS_HDTV        0x8
#define CCIR656_INFMT_PROGRESS  0
#define CCIR656_INFMT_INTERLACE 0x4

#define LCDCONFIG(PanelType,RGBSW,IPWR,IDE,ICK,IHS,IVS) (((PanelType)&0x01)<<27|((RGBSW)&0x01)<<20|((IPWR)&0x01)<<4| \
							 ((IDE)&0x01)<<3| ((ICK)&0x01)<<2|((IHS)&0x01)<<1|((IVS)&0x01))

#define FFBCONFIG2(ExtClk)        ((ExtClk&0x01))

#define LCD_PANEL_TYPE_6BIT   0
#define LCD_PANEL_TYPE_8BIT   1

struct ffb_info;
struct ffb_dev_info;

struct OUTPUT_INFO {
    char *name;
    u32 config;
    u32 config2;                ///<Bit0:1 Use external clock,0 Use internal clock
    u_char timing_num;
    struct ffb_timing_info *timing_array;
    void (*setting_ext_dev) (struct ffb_dev_info * info);
};

struct ffb_ops {
    int (*check_var) (struct fb_var_screeninfo * var, struct fb_info * info);
    int (*set_par) (struct fb_info * info);
    int (*setcolreg) (u_int regno, u_int red, u_int green, u_int blue, u_int trans,
                      struct fb_info * info);
    int (*blank) (int blank, struct fb_info * info);
    int (*ioctl) (struct fb_info * info, unsigned int cmd, unsigned long arg);
    int (*switch_buffer) (struct ffb_info * fbi);
};

#define FRAME_RATE_GRANULAR  10

struct ffb_dev_info {
    u_long io_base;
#if 1 /* graphic compress */
    u_long compress_io_base;
    u_long decompress_io_base;
#endif
    struct device *dev;
    volatile u_char state;
    volatile u_char task_state;
    struct semaphore ctrlr_sem; //used to protect work struct while scheduling
    struct work_struct task;
    u_int max_xres;
    u_int max_yres;
    struct ffb_buf_offset planar_off;
    struct OUTPUT_INFO *output_info;
    ffb_input_res_t *input_res; //Input reolution
    u_int support_otypes;       //Output types that device support
    u_char video_output_type;
    struct semaphore dev_lock;  //read/write registers protection
    int output_progressive;     //1 for progressive output
    u32 frame_rate;             //standard frame period
    u32 hw_frame_rate;          //real frame period which decided by real PLL3
    int cursor_reset;
    int cursor_pos_scale;       //if lcd scalar is enabled, this flag is used to determine if the cursor position is also scaled
    int fb0_fb1_share;          //fb0 and fb1 share the same frame buffer?
    /* This function is called from EDID client such as SLI, CAT6611 ...
     * This pointer will be updated in pip_probe() of pip.c
     */
    int (*update_edid)(u32 *edid_array, int error_checking, int add_remove);
};

typedef struct frame_ctrl_info {
    int do_updating;
    int ppfb_num;               /* the fb_num hardware using. only be used without queueing method */
    int old_fbnum;
    u32 miss_num;
    int *displayfifo;
    int get_idx;
    int put_idx;
    int set_idx;
    int use_queue;              //decide new way to use queue or "original frame buffer"
    int arrange_type;           //arrange_type:0:don't care 1=progress 2:interlace
} frame_ctrl_info_t;

struct ffb_info;
struct ffb_info {
    struct fb_info fb;
    struct ffb_dev_info *dev_info;
    struct ffb_rgb *rgb[NR_RGB];
    int index;

        /**
	 * These are the addresses we mapped
	 * the framebuffer memory region to.
	 **/
    dma_addr_t map_dma;
    u_char *map_cpu;
    u_int map_size;

    u16 *palette_cpu;
    dma_addr_t palette_dma;
    u_int palette_size;

    u_int cmap_inverse:1, cmap_static:1, unused:30;

    u_char *ppfb_cpu[MAX_FRAME_NO];
    dma_addr_t ppfb_dma[MAX_FRAME_NO];
#if 1 /* for the graphic compress */
    u_char *gui_buf[2];
    dma_addr_t gui_dma[2];
#endif
    frame_ctrl_info_t fci;      // frame control info
    u_char fb_num;              // configured fb_num
    wait_queue_head_t fb_wlist;
    u_char video_input_mode;
    u_int support_imodes;       //Input modes that device support
    spinlock_t display_lock;
    u32 sync_timeout;
    long display_accumulate;    //If value is lesser than 0, devive display rate is too slow.
    //If value is bigger than 0, devive display rate is too fast.
    int not_clean_screen;       //1 inidcates don't clean the screen to black, this value is auto clear.
    struct ffb_ops *ops;
    u_char use_rammap;          // if true, LCD allocates memory from 2nd/3rd DDR2, 1 for yes, 0 for no
#if 1                           //def CCIR_SUPPORT_VBI, remove may cause AP needs to compile again. Backward compatible
    u32 smem_len;               // orginal frame buffer size without VBI buffer
    int fb_use_ioctl;           //the frame control use ioctl fram user space.
#endif
    int fb_pan_display;         //linux fb_pan_display
    int ypan_offset;            //linux fb_pan_display

    //virtual screen
    struct {
        int enable;
        int width;
        int height;
        int offset_x;
        int offset_y;
        u32 frambase_ofs;
    } vs;

};

#define __type_entry(ptr,type,member) ((type *)((char *)(ptr)-offsetof(type,member)))

#define TO_INF(ptr,member)	__type_entry(ptr,struct ffb_info,member)

/**
 * These are the actions for set_ctrlr_state
 **/
#define C_DISABLE		(0)
#define C_ENABLE		(1)
#define C_DISABLE_CLKCHANGE	(2)
#define C_ENABLE_CLKCHANGE	(3)
#define C_REENABLE		(4)
#define C_DISABLE_PM		(5)
#define C_ENABLE_PM		(6)
#define C_STARTUP		(7)

struct ffb_params {
    unsigned int val;
    char *name;
};

/* Support video input mode */
/* 16-31 RGB mode */
enum { VIM_YUV422, VIM_YUV420, VIM_RGB = 16, VIM_ARGB =
        16, VIM_RGB888, VIM_RGB565, VIM_RGB555, VIM_RGB444, VIM_RGB8, VIM_RGB1555, VIM_RGB1BPP, VIM_RGB2BPP
};

#define FFB_SUPORT_CFG(x)         (1<<(x))
#define FFB_SUPPORT_CHECK(sup, x) ((sup)&(1<<(x)))

/*
 * Minimum X and Y resolutions
 */
#define MIN_XRES	64
#define MIN_YRES	64

typedef int (*pfun_devinit) (struct ffb_info * fbi, struct device * dev);

//ffb-lib => The interface for framebuffer.
extern int ffb_get_param(u_char witch, const char *s, int val);
extern int ffb_construct(struct ffb_info *fbi);
extern void ffb_deconstruct(struct ffb_info *fbi);
extern ffb_input_res_t *ffb_inres_get_info(VIN_RES_T input);

//ffb-lib => The interface for PM
#ifdef CONFIG_PM
extern int ffb_suspend(struct ffb_info *fbi, pm_message_t state);
extern int ffb_resume(struct ffb_info *fbi);
#endif /* CONFIG_PM */

#endif /* __FFB_H */
