#ifndef __SWOSG_ENG_H__
#define __SWOSG_ENG_H__
#include <linux/version.h>
#include <linux/semaphore.h>
#include "swosg/swosg_if.h"
#include <linux/dma-mapping.h>
#include "frammap/frammap_if.h"             

#define SUPPORT_SWOSG_CANVAS_STATIC     1


struct swosg_eng_point{
    unsigned int x;
    unsigned int y;
};

struct swosg_eng_canvas_cont{
    void         *canvas_vaddr;
    dma_addr_t   canvas_paddr;
    unsigned int canvas_size;
    unsigned int canvas_w;
    unsigned int canvas_h;
};

#ifdef SUPPORT_SWOSG_CANVAS_STATIC
#define SWOSG_ENG_OSG_PER_WIN       8
#define SWOSG_ENG_OSG_WIN_NUM       8 
#define SWOSG_ENG_OSG_CH_BASE       4
#define SWOSG_ENG_OSG_NUM           ((SWOSG_ENG_OSG_PER_WIN*SWOSG_ENG_OSG_WIN_NUM)+SWOSG_ENG_OSG_CH_BASE)

#define SWOSG_ENG_OSG_MAX_MEM       (10*1024*1024)
#define SWOSG_ENG_KSIZE_BASE        1024

#define SWOSG_ENG_DH_IDX_MIN        4
#define SWOSG_ENG_DH_IDX_MAX        68

enum{
    SWOSG_ENG_CANVAS_DYAMIC = 0,
    SWOSG_ENG_CANVAS_STATIC = 1    
};
struct swosg_eng_canvas_desc{
    int    usage;    
    struct swosg_eng_canvas_cont canvas_desc;   
};

struct swosg_eng_canvas_pool_mem {
    u32 vbase;
    u32 pbase;
    u32 mem_len;
    u32 usage;
    u32 setten;
};

struct swosg_eng_canvas_pool_desc {
    spinlock_t              canvas_pool_lock;
    u32                     osg_canvas_total_num;
    u32                     osg_canvas_num;
    u32                     osg_canvas_totalsize; 
    u32                     osg_canvas_remaind_size;
    u32                     osg_canvas_alloc_size;
    u32                     osg_canvas_curvaddr;
    u32                     osg_canvas_curpaddr;
    struct frammap_buf_info canvas_buf_info;
    struct swosg_eng_canvas_pool_mem *cavas_pool_mem;
};
#endif
 
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
#define init_MUTEX(sema) sema_init(sema, 1)
#endif

int swosg_eng_draw_line_one_frame(struct swosg_eng_point* startPoint
                                   ,struct swosg_eng_point* endPoint
                                   ,swosg_if_mask_param_t *param) ;

int swosg_eng_draw_line_true_frame(struct swosg_eng_point* startPoint
                                   ,struct swosg_eng_point* endPoint
                                   ,swosg_if_mask_param_t *param) ;

int swosg_eng_do_blit(swosg_if_blit_param_t * blit_param ,int num);
int swosg_eng_do_mask(swosg_if_mask_param_t * mask_param,int num);

int swosg_eng_palette_set( int id, u32 crycby);
int swosg_eng_palette_get( int id, u32* crycby);
int swosg_eng_canvas_set( int id, swosg_if_canvas_t * canvas);
int swosg_eng_canvas_del( int id);
int swosg_eng_canvas_get_info( int id, struct swosg_eng_canvas_desc * canvas);
int swosg_eng_get_dma_count(void);

#ifdef SUPPORT_SWOSG_CANVAS_STATIC
int swosg_eng_init(int chan_num , int total_mem);
int swosg_eng_proc_read_canvas_pool_info(char *page, char **start, off_t off, int count,int *eof, void *data);
int swosg_eng_proc_write_canvas_pool_info(struct file *file, const char __user *buf, size_t size, loff_t *off);
int swosg_eng_canvas_get_static_pool( int id, swosg_if_static_info * canvas);
unsigned int swosg_eng_get_canvas_runmode(void);
#else
int swosg_eng_init(void);
#endif

void swosg_eng_exit(void);
unsigned int swosg_eng_get_version(void);

#endif
