#ifndef __OSG_VG_H__
#define __OSG_VG_H__

#include <linux/wait.h>
#include <linux/kthread.h>
#include "common.h" //videograph

#define MAX_CHAN_NUM           64 
#define OSG_FIRE_MAX_NUM     1
#define JOB_DONE         (JOB_STS_DONE | JOB_STS_FLUSH) 

#if 1
#define DEBUG_M(level,engine,minor,fmt,args...) { \
    if ((log_level >= level) && is_print(engine,minor)) \
        printk("debug:"fmt, ## args);}
    //printm(MODULE_NAME,fmt, ## args); }
#else
#define DEBUG_M(level,engine,minor,fmt,args...) { \
    if ((log_level >= level) && is_print(engine,minor)) \
        printk(fmt,## args);}
#endif

#ifdef VG_LOG
#define OSG_PANIC(fmt,args...)                   \
    do{                                         \
        printk("!!PANIC!!"fmt, ## args);        \
        printm(MODULE_NAME,fmt, ## args);       \
        dump_stack();                           \
        damnit(MODULE_NAME);                    \
    }while(0)
#else
#define OSG_PANIC(fmt, args...) panic(fmt, ## args)
#endif

#define MODULE_NAME         "SG"    //(two bytes)
#define ENTITY_ENGINES      1   /* from videograph view */
#define ENTITY_MINORS       MAX_CHAN_NUM
#define OSG_PALETTE_MAX     16

#define OSG_PALETTE_COLOR_AQUA              0xCA48CA93//0x4893CA        /* CrCbY */
#define OSG_PALETTE_COLOR_BLACK             0x10801080//0x808010
#define OSG_PALETTE_COLOR_BLUE              0x296e29f0//0x6ef029
#define OSG_PALETTE_COLOR_BROWN             0x51a1515b//0xA15B51
#define OSG_PALETTE_COLOR_DODGERBLUE        0x693f69cb//0x3FCB69
#define OSG_PALETTE_COLOR_GRAY              0xb580b580//0x8080B5
#define OSG_PALETTE_COLOR_GREEN             0x515b5151//0x515B51
#define OSG_PALETTE_COLOR_KHAKI             0x72897248//0x894872
#define OSG_PALETTE_COLOR_LIGHTGREEN        0x90229036//0x223690
#define OSG_PALETTE_COLOR_MAGENTA           0x6ede6eca//0xDECA6E
#define OSG_PALETTE_COLOR_ORANGE            0x98bc9851//0xBC5198
#define OSG_PALETTE_COLOR_PINK              0xa5b3a589//0xB389A5
#define OSG_PALETTE_COLOR_RED               0x52f0525a//0xF05A52 
#define OSG_PALETTE_COLOR_SLATEBLUE         0x3d603da6//0x60A63D
#define OSG_PALETTE_COLOR_WHITE             0xeb80eb80//0x8080EB
#define OSG_PALETTE_COLOR_YELLOW            0xd292d210//0x9210D2

/*
 * job information.
 */
typedef enum {
    FRAME_TYPE_2FRAMES = 0x1,     /* top frame + bottom frame */
    FRAME_TYPE_2FIELDS = 0x2,     /* two field placed in progressive => top field, bottom field */
    FRAME_TYPE_YUV422FRAME = 0x4, /* top line + bottom line + top line + .... 
*/
} frame_type_t; 

typedef enum {
    DRAWING_RECT_JOB   = 0x1,      //not process yet
    BLITTER_JOB        = 0x2
} drawing_job_t; 

typedef enum {
    OSG_BORDER_TRUE_TYPE          = 0x1,      //not process yet
    OSG_BORDER_HORROW_TYPE        = 0x2
} OSG_BORDER_T; 


struct __osg_rect{
    unsigned int          x;
    unsigned int          y;
    unsigned int          w;
    unsigned int          h;
    unsigned int          blit_color;
    unsigned char         drawing_type;
    unsigned char         align_type;
    unsigned char         is_blending;
    unsigned char         alpha;
    unsigned char         border_size;
    unsigned char         palette_idx;
    unsigned char         border_type;    
    struct list_head      list;
};

struct __osg_point{
    unsigned int x;
    unsigned int y;
};

struct __osg_color{
    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int a;
};

struct __osg_surface {
    unsigned int t_addr;
    unsigned int b_addr;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
};

typedef enum {
    JOB_PARA_SRCFMT_TYPE      = 0x1,     
    JOB_PARA_DRAWING_TYPE     = 0x2,
    JOB_PARA_SRCXY_TYPE       = 0x4,
    JOB_PARA_SRCDIM_TYPE      = 0x8,
    JOB_PARA_SRCBG_DIM_TYPE   = 0x10,
    JOB_PARA_TAR_XY_TYPE      = 0x20,
    JOB_PARA_TAR_DIM_TYPE     = 0x40,
    JOB_PARA_COLOR_TYPE       = 0x80,
    JOB_PARA_BELNDING_TYPE    = 0x100,
    JOB_PARA_BORDER_TYPE      = 0x200,
    JOB_PARA_BLIT_BGCL_TYPE   = 0x400,
    JOB_PARA_BLIT_DIM_TYPE    = 0x800,
    JOB_PARA_BLIT_XY_TYPE     = 0x1000,
    JOB_PARA_BLIT_BLEND_TYPE  = 0x2000,
} JOB_PARAMETER_SETTING_T;


typedef struct osg_param {
    unsigned int               chan_id;      //index, from 0 ~
    unsigned int               frame_type;    
    struct __osg_surface       bg_suf;
    struct __osg_surface       dst_suf;
    struct __osg_surface       src_suf; 
    struct list_head           dst_rect_list;     
} osg_param_t;
/* job status */
typedef enum {
    JOB_STS_QUEUE   = 0x1,      //not process yet
    JOB_STS_ONGOING = 0x2,
    JOB_STS_DONE    = 0x4,
    JOB_STS_FLUSH   = 0x8,
    JOB_STS_DONOTHING = 0x10
} job_status_t;

typedef enum {
    OSG_WIN_ALIGN_NONE = 0,             ///< display window on absolute position
    OSG_WIN_ALIGN_TOP_LEFT,             ///< display window on relative position, base on output image size
    OSG_WIN_ALIGN_TOP_CENTER,           ///< display window on relative position, base on output image size
    OSG_WIN_ALIGN_TOP_RIGHT,            ///< display window on relative position, base on output image size
    OSG_WIN_ALIGN_BOTTOM_LEFT,          ///< display window on relative position, base on output image size
    OSG_WIN_ALIGN_BOTTOM_CENTER,        ///< display window on relative position, base on output image size
    OSG_WIN_ALIGN_BOTTOM_RIGHT,         ///< display window on relative position, base on output image size
    OSG_WIN_ALIGN_CENTER,               ///< display window on relative position, base on output image size
    OSG_WIN_ALIGN_MAX
} OSG_WIN_ALIGN_T;

typedef enum {
    OSG_JOB_MULTI_TYPE,
    OSG_JOB_SINGLE_TYPE
}OSG_JOB_TYPE_T;

typedef enum {
    OSG_VG_ALPHA_0 = 0,                    ///0%
    OSG_VG_ALPHA_25,                       ///25%
    OSG_VG_ALPHA_37,                       ///37%
    OSG_VG_ALPHA_50,                       ///50%
    OSG_VG_ALPHA_62,                       ///62%
    OSG_VG_ALPHA_75,                       ///75%
    OSG_VG_ALPHA_87,                       ///87%
    OSG_VG_ALPHA_100,                      ///100%
    OSG_VG_ALPHA_MAX
} OSG_VG_ALPHA_T;

typedef struct osg_job {
    int                          jobid;
    int                          chan_id;
    osg_param_t                  param;
    struct f_ops                 *fops;
	void                         *private;
	job_status_t                 status;
	int                          hardware;   // 1 for process by hardware, 0 for not
	unsigned int                 puttime;
	unsigned int                 starttime;
	unsigned int                 finishtime; 
    unsigned int                 job_type;
    unsigned int                 is_last_job;
    unsigned int                 need_to_callback;
    struct video_entity_job_t    *parent_entity_job;
    struct list_head             mjob_head;
    struct list_head             mjob_list;    
	struct list_head             list;       //per channel link list
} osg_job_t;

struct f_ops {
    void  (*callback)(osg_job_t *job);
    int   (*pre_process)(osg_job_t *job, void *private);
    int   (*post_process)(osg_job_t *job, void *private);
    void  (*mark_engine_start)(osg_job_t *job, void *private);
    void  (*mark_engine_finish)(osg_job_t *job, void *private);
};

/* 
 * utilization structure
 */
typedef struct eng_util
{
	unsigned int utilization_period; //5sec calculation
	unsigned int utilization_start;
	unsigned int utilization_record;
	unsigned int engine_start; 
	unsigned int engine_end;
	unsigned int engine_time;
}util_t;

#define ENGINE_START(engine) 	g_utilization[engine].engine_start
#define ENGINE_END(engine) 		g_utilization[engine].engine_end
#define ENGINE_TIME(engine) 	g_utilization[engine].engine_time
#define UTIL_PERIOD(engine) 	g_utilization[engine].utilization_period
#define UTIL_START(engine) 		g_utilization[engine].utilization_start
#define UTIL_RECORD(engine) 	g_utilization[engine].utilization_record

/*
 * Main structure
 */
typedef struct global_info
{
    spinlock_t              lock;
    spinlock_t              para_lock;
    spinlock_t              multi_lock;
    struct workqueue_struct *workq;
    wait_queue_head_t       osg_queue;
    struct task_struct *    osg_thread; 
    unsigned int            osg_running_flag;  
    unsigned int            readevent; 
    /* The following is for job-node in link list
     */
    //struct list_head        job_list[ENTITY_MINORS];     /* minor is the index */
    struct list_head        fire_job_list;               /* minor is the index */
    struct list_head        going_job_list;               /* minor is the index */
    struct list_head        finish_job_list;               /* minor is the index */
    struct kmem_cache       *job_cache;
    struct kmem_cache       *job_param_cache;
	int 					vg_node_count;
	int 					alloc_param_count;
	atomic_t                list_cnt[ENTITY_MINORS]; 
    atomic_t                list_going_cnt[ENTITY_MINORS]; 
	atomic_t                list_finish_cnt[ENTITY_MINORS];
    atomic_t                job_fail_cnt[ENTITY_MINORS];
    atomic_t                job_do_fail_cnt[ENTITY_MINORS];
} g_info_t;

struct property_record_t {
#define MAX_RECORD 320    
    struct video_property_t property[MAX_RECORD];
    struct video_entity_t   *entity;
    int    job_id;
};

struct property_map_t {
    int id;
    char *name;
    char *readme;
};

#define LOG_ERROR     0
#define LOG_WARNING   1
#define LOG_QUIET     2
#define LOG_DEBUG     3

#define OSG_DEV_NAME       "SW_OSG"

/* vg init function
 */
int osg_vg_init(void);

/*
 * Exit point of video graph
 */
void osg_vg_driver_clearnup(void);
void osg_vg_debug_print(char *page, int *len, void *data);


#endif /* __SWOSD_VG_H__ */
