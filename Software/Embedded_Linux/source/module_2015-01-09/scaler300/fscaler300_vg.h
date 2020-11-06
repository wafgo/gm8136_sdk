#ifndef __FSCALER300_VG_H__
#define __FSCALER300_VG_H__

//#include <linux/semaphore.h>

#include "fscaler300.h"
#include <common.h> //videograph

#define MAX_NAME    50
#define MAX_README  100

#define FSCALER300_MAX_ENGINE   1
#define ENTITY_ENGINES      FSCALER300_MAX_ENGINE   /* for videograph view */
#define ENTITY_MINORS       MAX_CHAN_NUM

struct property_map_t {
    int id:24;
    char name[MAX_NAME];
    char readme[MAX_README];
};

struct property_record_t {
#define MAX_RECORD MAX_PROPERTYS
    struct video_property_t property[MAX_RECORD];
    struct video_entity_t   *entity;
    int    job_id;
};

#define SCALER_ENGINE_BASE        0
#define SCALER_CHAN_BASE        0

#define LOG_ERROR     0
#define LOG_WARNING   1
#define LOG_QUIET     2

typedef enum {
    SINGLE_NODE        = 0,
    MULTI_NODE         = 1,
    VIRTUAL_NODE       = 2,
    NONE_NODE
} node_type_t;
#if 0
typedef struct node_info {
    int     job_cnt;
    int     blk_cnt;
} node_info_t;
#endif
/*
 * node structure
 */
typedef struct job_node {
    int                         job_id;         // record job->id
    char                        engine;         // indicate which hw engine
    int                         minor;          // indicate which channel
    int                         idx;            // indicate engine * minors + minor
    int                         ch_cnt;         // indicate property channel count
    int                         ch_id;          // indicate property channel id
    job_status_t                status;
    char                        priority;
    fscaler300_property_t       property;
    struct list_head            plist;          // parent list
    struct list_head            clist;          // child list
    struct list_head            vg_plist;       // vg parent list
    struct list_head            vg_clist;       // vg child list
    node_type_t                 type;
    char                        need_callback;  // need to callback root_job
    void                        *parent;        // point to root node
    void                        *private;       // private_data, now points to job of videograph */
    unsigned int                puttime;        // putjob time
    unsigned int                starttime;      // start engine time
    unsigned int                finishtime;     // finish engine time
    node_info_t                 node_info;
    unsigned int                serial_num;     // record serial number
    unsigned int                timer;          // record tx/rx jiffies
    char                        trig_dma;
    struct job_node             *rc_node;       //used by local job to point to cpucomm node
    unsigned long               expires;
    struct list_head            rclist;         // rclist for rc node add
    struct list_head            eplist;         // rclist for rc node add
} job_node_t;

typedef struct vg_proc {
    struct proc_dir_entry   *root;
    struct proc_dir_entry   *info;
    struct proc_dir_entry   *cb;
    struct proc_dir_entry   *property;
    struct proc_dir_entry   *job;
    struct proc_dir_entry   *util;
    struct proc_dir_entry   *filter;
    struct proc_dir_entry   *level;
    struct proc_dir_entry   *ratio;
    struct proc_dir_entry   *debug;
    struct proc_dir_entry   *version;
    struct proc_dir_entry   *buf_clean;
    struct proc_dir_entry   *ep_timeout;
    struct proc_dir_entry   *dual_debug;
    struct proc_dir_entry   *rc_tx_threshold;
    struct proc_dir_entry   *ep_rx_timeout;
    struct proc_dir_entry   *rc_ep_diff;
    struct proc_dir_entry   *rxtx_max;
    struct proc_dir_entry   *flow_check;
} vg_proc_t;

typedef struct global_info {
    spinlock_t              lock;
    struct semaphore        sema_lock;          ///< vg semaphore lock
    struct list_head        node_list;          // record job list from V.G
    struct semaphore        sema_lock_2ddma;    ///< 2ddma semaphore lock
    struct list_head        node_list_2ddma;    // record job list from V.G for 2ddma
    struct kmem_cache       *node_cache;
    struct kmem_cache       *vproperty_cache;
    struct list_head        ep_list;          // record job list from V.G
    struct list_head        ep_rcnode_list;     // record job list from rc cpucomm, used by ep
    struct list_head        ep_rcnode_cb_list;
    struct list_head        rc_list;            // record job list from V.G
    struct task_struct      *ep_task;
    struct task_struct      *rc_task;
    struct task_struct 		*cb_task;
    struct task_struct 		*cb_2ddma_task;
    struct task_struct 		*tx_task;
} global_info_t;

/*
 * performance
 */
typedef struct vg_perf_info {
	unsigned int prev_end;      ///< previous job finish time(jiffes)
    unsigned int times;         ///< job work time(jiffies)
    unsigned int util_start;
    unsigned int util_record;
} vg_perf_info_t;


typedef struct perf_info {
	spinlock_t				lock;
	vg_perf_info_t			perf_info[SCALER300_ENGINE_NUM];
} perf_info_t;

typedef struct snd_info {
    int         serial_num;             // serial number for identify mapping
    u32         out_addr;               // physical pcie addr
    u32         in_addr;
    u32         status;                 // inform job_status_t;
    int         vg_serial;
    unsigned int    timer;
} snd_info_t;

/* vg init function
 */
int fscaler300_vg_init(void);

/*
 * Exit point of video graph
 */
void fscaler300_vg_driver_clearnup(void);

void mark_engine_start(int dev);
void mark_engine_finish(int dev);

#endif
