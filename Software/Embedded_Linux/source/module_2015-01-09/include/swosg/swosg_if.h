#ifndef __SWOSG_IF_H__
#define __SWOSG_IF_H__

#if 1
#define DEBUG_M(level,fmt,args...) { \
    if ((log_level >= level) && (!g_limit_printk || printk_ratelimit())) \
        printk("debug:"fmt, ## args);}
    //printm(MODULE_NAME,fmt, ## args); }
#else
#define DEBUG_M(level,engine,minor,fmt,args...) { \
    if ((log_level >= level) && is_print(engine,minor)) \
        printk(fmt,## args);}
#endif

#define LOG_ERROR     0
#define LOG_WARNING   1
#define LOG_QUIET     2
#define LOG_DEBUG     3

#define SWOSG_CH_MAX_MUN      68
#define SWOSG_PALETTE_MAX     3
#define SWOSG_CANVAS_MAX      (32*SWOSG_CH_MAX_MUN)

//#define SWOSG_API_TEST

#define SWOSG_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

typedef enum {
    SWOSGIF_BORDER_TURE   = 1,
    SWOSGIF_BORDER_HORROW = 2   
} SWOSGIF_BORDER_TP;

typedef enum {
    SWOSG_WIN_ALIGN_NONE = 0,             ///< display window on absolute position
    SWOSG_WIN_ALIGN_TOP_LEFT,             ///< display window on relative position, base on output image size
    SWOSG_WIN_ALIGN_TOP_CENTER,           ///< display window on relative position, base on output image size
    SWOSG_WIN_ALIGN_TOP_RIGHT,            ///< display window on relative position, base on output image size
    SWOSG_WIN_ALIGN_BOTTOM_LEFT,          ///< display window on relative position, base on output image size
    SWOSG_WIN_ALIGN_BOTTOM_CENTER,        ///< display window on relative position, base on output image size
    SWOSG_WIN_ALIGN_BOTTOM_RIGHT,         ///< display window on relative position, base on output image size
    SWOSG_WIN_ALIGN_CENTER,               ///< display window on relative position, base on output image size
    SWOSG_WIN_ALIGN_MAX
} SWOSG_WIN_ALIGN_T;

enum{
    SWOSG_CANVAS_DYAMIC = 0,
    SWOSG_CANVAS_STATIC = 1    
};

#ifdef SWOSG_API_TEST
typedef struct __swosg_if_canvas {
    unsigned int                 idx; 
    unsigned int                 src_cava_addr;
    unsigned short               src_w;          /*patten source width*/
    unsigned short               src_h;          /*patten source height*/
}swosg_if_canvas_t;

typedef struct __swosg_if_mask_param {
    unsigned int               dest_bg_addr;   /*destination source address*/
    unsigned short             dest_bg_w;      /*destination source width*/   
    unsigned short             dest_bg_h;      /*destination source height*/
    unsigned short             border_size;    /*mask border size*/
    unsigned short             palette_idx;    /*pallette index support max 16*/
    SWOSGIF_BORDER_TP          border_type;    /*(horrow:2, true:1)*/
    unsigned short             blending;       /*alpha blending (0:0% 1:25% 2:37% 3:50% 4:62% 5:75% 6:87% 7:100%)*/
    SWOSG_WIN_ALIGN_T          align_type;     /*align type*/
    unsigned short             target_x;       /*mask target x*/   
    unsigned short             target_y;       /*mask target y*/   
    unsigned short             target_w;       /*mask target width*/   
    unsigned short             target_h;       /*mask target height*/   
}swosg_if_mask_param_t;

typedef struct __swosg_if_blit_param {
    unsigned int               dest_bg_addr;   /*destination address*/
    unsigned short             dest_bg_w;      /*destination width*/   
    unsigned short             dest_bg_h;      /*destination height*/
    //unsigned int               src_addr;       /*patten source address*/
    //unsigned short             src_w;          /*patten source width*/
    //unsigned short             src_h;          /*patten source height*/
    unsigned int               bg_color;       /*patten background color (UYVY U<<24|Y<<16|V<<8|Y)*/
    unsigned short             target_x;       /*blit target x*/
    unsigned short             target_y;       /*blit target y*/   
    //unsigned short             blending;       /*alpha blending (0:0% 1:25% 2:37% 3:50% 4:62% 5:75% 6:87% 7:100%)*/
    SWOSG_WIN_ALIGN_T          align_type;     /*align type*/
    unsigned short             src_patten_idx; 
}swosg_if_blit_param_t;
typedef struct __swosg_if_static_info{
    unsigned int vbase;               /*virtual address*/
    unsigned int pbase;               /*physical address*/
    unsigned int mem_len;             /*memory size*/
    unsigned int usage;               /*usr by canvas*/
    unsigned int setten;              /*setten for canvas*/
}swosg_if_static_info;

#else
typedef struct __swosg_if_canvas {
    u32               src_cava_addr;
    u16               src_w;          /*patten source width*/
    u16               src_h;          /*patten source height*/
}swosg_if_canvas_t;

typedef struct __swosg_if_mask_param {
    u32               dest_bg_addr;   /*destination source address*/
    u16               dest_bg_w;      /*destination source width*/   
    u16               dest_bg_h;      /*destination source height*/
    u16               border_size;   /*mask border size*/
    u16               palette_idx;   /*pallette index support max 16*/
    SWOSGIF_BORDER_TP border_type;   /*(horrow:2, true:1)*/
    u16               blending;      /*alpha blending (0:0% 1:25% 2:37% 3:50% 4:62% 5:75% 6:87% 7:100%)*/
    SWOSG_WIN_ALIGN_T align_type;    /*align type*/
    u16               target_x;      /*mask target x*/   
    u16               target_y;      /*mask target y*/   
    u16               target_w;      /*mask target width*/   
    u16               target_h;      /*mask target height*/   
}swosg_if_mask_param_t;

typedef struct __swosg_if_blit_param {
    u32               dest_bg_addr;   /*destination address*/
    u16               dest_bg_w;      /*destination width*/   
    u16               dest_bg_h;      /*destination height*/
    //u32               src_addr;       /*patten source address*/
    //u16               src_w;          /*patten source width*/
    //u16               src_h;          /*patten source height*/
    //u32               bg_color;       /*patten background color (UYVY U<<24|Y<<16|V<<8|Y)*/
    u16               target_x;       /*blit target x*/
    u16               target_y;       /*blit target y*/   
    //u16               blending;       /*alpha blending (0:0% 1:25% 2:37% 3:50% 4:62% 5:75% 6:87% 7:100%)*/
    SWOSG_WIN_ALIGN_T align_type;     /*align type*/
    u16               src_patten_idx; 
}swosg_if_blit_param_t;

typedef struct __swosg_if_static_info{
    u32 vbase;               /*virtual address*/
    u32 pbase;               /*physical address*/
    u32 mem_len;             /*memory size*/
    u32 usage;               /*usr by canvas*/
    u32 setten;              /*setten for canvas*/
}swosg_if_static_info;

#endif

int swosg_if_do_mask(swosg_if_mask_param_t * mask_param, int num);
int swosg_if_do_blit(swosg_if_blit_param_t * blit_param, int num);
int swosg_if_palette_set( int id, unsigned int crycby);
int swosg_if_palette_get(int id, unsigned int *crycby);
int swosg_if_canvas_set( int id, swosg_if_canvas_t * canvas);
int swosg_if_canvas_del(int id);
unsigned int swosg_if_get_version(void);
int swosg_if_get_palette_number(void);
unsigned int swosg_if_get_chan_num(void);
int swosg_if_get_static_pool_info(int id,swosg_if_static_info * canvas);
#ifdef SWOSG_API_TEST
unsigned int swosg_if_get_canvas_runmode(void);
#else
u32 swosg_if_get_canvas_runmode(void);
#endif

#ifdef SWOSG_API_TEST
/* IOCTL command */
#define SWOSG_IOC_MAGIC       'K'
#define SWOSG_CMD_MASK        _IOW(SWOSG_IOC_MAGIC, 1, swosg_if_mask_param_t)
#define SWOSG_CMD_BLIT        _IOW(SWOSG_IOC_MAGIC, 2, swosg_if_blit_param_t)
#define SWOSG_CMD_CANVAS_SET  _IOW(SWOSG_IOC_MAGIC, 2, swosg_if_canvas_t)
#endif

#endif
