#ifndef __FTDI220_VG_H__
#define __FTDI220_VG_H__

#include "ftdi220.h"
#include "common.h" //videograph

#define USE_KTHREAD
#define MAX_VCH         128
#define MAX_VCH_MASK    (MAX_VCH - 1)

#define JOB_DONE    (JOB_STS_DONE | JOB_STS_FLUSH | JOB_STS_DONOTHING)

#define DEBUG_M(level,engine,minor,fmt,args...) { \
    if ((log_level >= level) && is_print(engine,minor)) \
        printm(MODULE_NAME,fmt, ## args); }

#ifdef VG_LOG
#define DI_PANIC(fmt,args...)                   \
    do{                                         \
        printk("!!PANIC!!"fmt, ## args);        \
        printm(MODULE_NAME,fmt, ## args);       \
        dump_stack();                           \
        damnit(MODULE_NAME);                    \
    }while(0)
#else
#define DI_PANIC(fmt, args...) panic(fmt, ## args)
#endif

#define MODULE_NAME         "DI"    //(two bytes)
#define ENTITY_ENGINES      1   /* from videograph view */

#if defined(CONFIG_PLATFORM_GM8210)
#define ENTITY_MINORS       64
#elif defined(CONFIG_PLATFORM_GM8287)
#define ENTITY_MINORS       64
#elif (defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136))
#define ENTITY_MINORS       8
#else
#error "Please define ENTITY_MINORS"
#endif

/* property database */
enum prop {
    PROP_BG_DIM = 0,
    PROP_FUNCTION_AUTO,
    PROP_SRC_INTERLACE,
    PROP_SRC_FMT,
    PROP_SRC_XY,
    PROP_SRC_DIM,
    PROP_DIDN_MODE,
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
} util_t;

#define ENGINE_START(engine) 	g_utilization[engine].engine_start
#define ENGINE_END(engine) 		g_utilization[engine].engine_end
#define ENGINE_TIME(engine) 	g_utilization[engine].engine_time
#define UTIL_PERIOD(engine) 	g_utilization[engine].utilization_period
#define UTIL_START(engine) 		g_utilization[engine].utilization_start
#define UTIL_RECORD(engine) 	g_utilization[engine].utilization_record

typedef enum {
    SRC_FIELD_TOP = 0,
    SRC_FIELD_BOT = 1,
} src_field_t;

enum special_func {
    SPEICIAL_FUNC_60P = 0x1,
};

/* channel base parameters. */
typedef struct{
    int                 ch_width;
    int                 ch_height;
    int                 bg_width;
    int                 bg_height;
    int                 offset_x;
    int                 offset_y;
    int                 src_interlace;
    int                 didn;
    frame_type_t        frame_type;
    u32                 command;
    int                 skip;
    enum special_func   feature;    /* one frame mode 60p features */
    int                 src_field;  /* one frame mode 60p features */
} chan_param_t;

#define I_AM_TOP(x) ((x)->chan_param.src_field == SRC_FIELD_TOP)
#define I_AM_BOT(x) ((x)->chan_param.src_field == SRC_FIELD_BOT)
#define ONE_FRAME_SRC_FIELD(x)  ((x)->chan_param.src_field)
#define ONE_FRAME_MODE_60P(x)   ((x)->chan_param.frame_type == FRAME_TYPE_YUV422FRAME_60P)

/*
 * node structure (also for virtual channel)
 */
typedef struct node_s
{
    int                 chnum;      //virtual channel index
    int                 drv_chnum;
    int                 dev;
    void                *private;   /* private_data, now points to job of videograph */
    job_status_t        status;
    struct node_s       *refer_to;  /* which node is referenced by me */

    /* New for virtual channels
     */
    struct node_s       *parent_node;   /* The case that a FD job contains multiple virtual jobs */
    struct node_s       *pair_frame;    /* only valid for 60p case. */
    struct list_head    child_list;     /* all child links to parent's child_list */
    int                 nr_splits;      /* how many virutal jobs of this parent */
    int                 nr_splits_keep; /* how many virutal jobs of this parent */
    int                 minor_idx;      /* fd number from videograph */

    atomic_t            ref_cnt;        /* counter of being referenced. */
    chan_param_t        chan_param;
    ftdi220_param_t     param;
    struct list_head    list;           /* link to to chan_job_list[x] */

    u32                 collect_command;    /* parent collects all commands including childs */
    int                 di_sharebuf;        /* in di_sharebuf mode, the refer node will be the first R. */
    struct node_s       *r_refer_to;
    u32                 job_seqid;
    u32                 hash[MAX_VCH];
    //for performance measure purpose
	unsigned int        puttime;    //putjob time
    unsigned int        starttime;  //start engine time
    unsigned int        finishtime; //finish engine time
	unsigned int 		first_node_return_fail; /* for deinterlace, because of this first node no reference node */
} job_node_t;

/*
 * Main structure
 */
typedef struct global_info
{
    spinlock_t              lock;
    struct semaphore        sema;
    struct workqueue_struct *workq;
    job_node_t              *ref_node[ENTITY_MINORS];       /* minor is the index */

    /* di_sharebuf used */
    job_node_t              *r_ref_node[ENTITY_MINORS];     /* minor is the index */
    job_node_t              *n_ref_node[ENTITY_MINORS];     /* minor is the index */
    job_node_t              *p_ref_node[ENTITY_MINORS];     /* minor is the index */

    /* The following is for job-node in link list
     */
    struct list_head        job_list[ENTITY_MINORS];        /* minor is the index */
    struct list_head        drv_chnum[ENTITY_MINORS];       /* minor is the index */
    src_field_t             expect_field[ENTITY_MINORS];    /* minor is the index. Only valid for yuv422frame 60p */
    struct work_struct      *job_work;
    struct kmem_cache       *job_cache;
	atomic_t 			    vg_node_count;
	atomic_t 				alloc_node_count;
	atomic_t                ver_chg_count;
	atomic_t                vg_chg_count;
	atomic_t                stop_count;
	atomic_t                hybrid_job_count;
	atomic_t                put_fail_count;
	atomic_t                cb_fail_count;
	atomic_t                wrong_field_count;
} g_info_t;

struct property_map_t {
    int id;
    char *name;
    char *readme;
};

struct property_record_t {
#define MAX_RECORD 480
    struct video_property_t property[MAX_RECORD];
    struct video_entity_t   *entity;
    int    job_id;
};

#define LOG_ERROR     0
#define LOG_WARNING   1
#define LOG_QUIET     2
#define LOG_DEBUG     3

#define REFCNT_INC(x)       atomic_inc(&(x)->ref_cnt)
#define REFCNT_DEC(x)       do {if (atomic_read(&(x)->ref_cnt)) atomic_dec(&(x)->ref_cnt);} while(0)
#define chan_job_list       g_info.job_list
#define drv_chnum_list      g_info.drv_chnum
#define expect_field        g_info.expect_field

/* vg init function
 */
int ftdi220_vg_init(void);

/*
 * Exit point of video graph
 */
void ftdi220_vg_driver_clearnup(void);

#endif /* __FTDI220_VG_H__ */
