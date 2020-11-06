#ifndef __FT3DNR_VG_H__
#define __FT3DNR_VG_H__

#include "ft3dnr.h"
#include <common.h> //videograph

#define MAX_NAME    50
#define MAX_README  100

#define ENTITY_ENGINES      1   /* for videograph view */
#define ENTITY_MINORS       MAX_CH_NUM

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

typedef enum {
    SINGLE_NODE        = 0,
    MULTI_NODE         = 1,
    NONE_NODE
} node_type_t;

/*
 * node structure
 */
typedef struct job_node {
    int                         job_id;         // record job->id
    char                        engine;         // indicate which hw engine
    int                         minor;          // indicate which channel
    int                         chan_id;        // indicate engine * minors + minor
    job_status_t                status;
    ft3dnr_param_t              param;
    struct job_node             *refer_to;      /* which node is referenced by me */
    //struct list_head            chan_list;      // channel list
    struct list_head            plist;          // parent list
    struct list_head            clist;          // child list
    node_type_t                 type;
    char                        need_callback;  // need to callback root_job
    void                        *parent;        // point to root node
    void                        *private;       // private_data, now points to job of videograph */
    //struct f_ops                *fops;
    unsigned int                puttime;        // putjob time
    unsigned int                starttime;      // start engine time
    unsigned int                finishtime;     // finish engine time
    //int                         mrnr_id;
    //int                         tmnr_id;
    atomic_t                    ref_cnt;
    void                        *ref_buf;
    int                         var_buf_idx;
    int                         stop_flag;
    bool                        ref_res_changed;
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
    struct proc_dir_entry   *version;
    struct proc_dir_entry   *reserved_buf;
} vg_proc_t;

typedef struct global_info {
    spinlock_t              lock;
    struct semaphore        sema_lock;          ///< vg semaphore lock
    struct list_head        node_list;          // record job list from V.G
    struct kmem_cache       *node_cache;
    job_node_t              *ref_node[ENTITY_MINORS];
    struct task_struct      *cb_task;
} global_info_t;

/* vg init function
 */
int ft3dnr_vg_init(void);

/*
 * Exit point of video graph
 */
void ft3dnr_vg_driver_clearnup(void);

void mark_engine_start(void);
void mark_engine_finish(void);

#endif
