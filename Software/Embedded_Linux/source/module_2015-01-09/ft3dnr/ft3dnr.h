#ifndef __FT3DNR_H__
#define __FT3DNR_H__

#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <api_if.h>

#include "ft3dnr_mem.h"
#include "ft3dnr_ctrl.h"
#include "ft3dnr_mrnr.h"
#include "ft3dnr_tmnr.h"
#include "ft3dnr_dma.h"

#define VERSION 150107

#define USE_KTHREAD
//#define USE_WQ

#define MAX_CH_NUM  8 // how many channels this module can support
#define BLK_MAX     3

#define FT3DNR_REG_MEM_LEN      0xE8
#define FT3DNR_LL_MEM_BASE      0x400
#define FT3DNR_LL_MEM           0x400
#define FT3DNR_MRNR_MEM_BASE    0x600

#define FT3DNR_LLI_TIMEOUT     2000    ///< 2 sec

#define VARBLK_WIDTH   128
#define VARBLK_HEIGHT   32
#define WIDTH_LIMIT		(VARBLK_WIDTH - 2)
#define HEIGHT_LIMIT	(VARBLK_HEIGHT - 2)
#define VAR_BIT_COUNT 8
#define TH_RETIO 7

#define ALIGN16_UP(x)   (((x + 15) >> 4) << 4)

#define     MAX_LINE_CHAR   256
#define     MAX_RES_IDX     (23 + 5)    // extend 5 reserved space to add new resolution from vg config
#define     MAX_RES_CNT     10
#define     SPEC_FILENAME   "spec.cfg"
#define     GMLIB_FILENAME  "gmlib.cfg"

#define FT3DNR_PROC_NAME "3dnr"
#define DRIVER_NAME      "ft3dnr"

enum {
    SRC_TYPE_DECODER    = 0,
    SRC_TYPE_ISP        = 1,
    SRC_TYPE_SENSOR     = 2,
    SRC_TYPE_UNKNOWN
};

typedef struct tmnr_param {
    u8      Y_var;
    u8      Cb_var;
    u8      Cr_var;
    u8      dc_offset;
    u8      auto_recover;
    u8      rapid_recover;
    u8      learn_rate;
    u8      his_factor;
    u16     edge_th;
} tmnr_param_t;

typedef struct ft3ndr_ctrl {
    bool        spatial_en;
    bool        temporal_en;
    bool        tnr_edg_en;
    bool        tnr_learn_en;
} ft3dnr_ctrl_t;

typedef struct ft3ndr_his {
    u16                 norm_th;
    u16                 right_th;
    u16                 bot_th;
    u16                 corner_th;
}   ft3dnr_his_t;

typedef struct ft3dnr_dim {
    u16                 src_bg_width;
    u16                 src_bg_height;
    u16                 src_x;
    u16                 src_y;
    u16                 nr_width;
    u16                 nr_height;
    u16                 org_width;
    u16                 org_height;
} ft3dnr_dim_t;

typedef struct ft3dnr_dma {
    u32                 dst_addr;
    u32                 src_addr;
    u32                 ref_addr;
    u32                 var_addr;
} ft3dnr_dma_t;

typedef struct ft3dnr_param {
    unsigned short      src_fmt;
    unsigned short      dst_fmt;
    ft3dnr_ctrl_t       ctrl;
    ft3dnr_dim_t        dim;
    ft3dnr_his_t        his;
    ft3dnr_dma_t        dma;
    mrnr_param_t        mrnr;
    tmnr_param_t        tmnr;
    int                 mrnr_id;
    int                 tmnr_id;
    int                 src_type;
} ft3dnr_param_t;

typedef enum {
    TYPE_FIRST          = 0x1,      // job from VG is more than 1 job in ft3dnr, set as first job
    TYPE_MIDDLE         = 0x2,      // job from VG is more than 3 job in ft3dnr, job is neither first job nor last job
    TYPE_LAST           = 0x4,      // job from VG is more than 1 job in ft3dnr, set as last job
    TYPE_ALL            = 0x8,      // job from VG is only 1 job in ft3dnr
    TYPE_TEST
} job_perf_type_t;

typedef enum {
    SINGLE_JOB          = 0x1,      // single job
    MULTI_JOB           = 0x2,      // multi job
    NONE_JOB
} job_type_t;

/* job status */
typedef enum {
    JOB_STS_QUEUE       = 0x1,      // not yet add to link list memory
    JOB_STS_SRAM        = 0x2,      // add to link list memory, but not change last job's "next link list pointer" to this job
    JOB_STS_ONGOING     = 0x4,      // already change last job's "next link list pointer" to this job
    JOB_STS_DONE        = 0x8,      // job finish
    JOB_STS_FLUSH       = 0x10,     // job to be flush
    JOB_STS_DONOTHING   = 0x20,     // job do nothing
    JOB_STS_TEST
} job_status_t;

/*
 * link list block info
 */
typedef struct ll_blk_info {
    u32     status_addr;
    u8      blk_num;
    u8      mrnr_num;
    u8      next_blk;
    u8      is_last;
} ft3dnr_ll_blk_t;

typedef struct ft3dnr_job {
    int                     job_id;
    int                     chan_id;
    ft3dnr_param_t          param;
    job_type_t              job_type;
    job_status_t            status;
    ft3dnr_ll_blk_t         ll_blk;
    struct list_head        job_list;                  // job list
    struct f_ops            *fops;
    char                    need_callback;
    int                     job_cnt;
    void                    *parent;                // point to parent job
    void                    *private;
    job_perf_type_t         perf_type;              // for mark engine start bit, videograph 1 job maybe have more than 1 job in scaler300
                                                    // mark engine start at first job, other dont't need to mark, use perf_mark to check.
} ft3dnr_job_t;

typedef struct ft3dnr_dma_buf {
    char        name[20];
    void        *vaddr;
    dma_addr_t  paddr;
    size_t      size;
} ft3dnr_dma_buf_t;

#define MAP_CHAN_NONE -1

typedef struct _res_cfg {
    char    res_type[7];
    u16     width;
    u16     height;
    size_t  size;
    ft3dnr_dma_buf_t    var_buf;
    int     map_chan;    // which channel used
} res_cfg_t;

struct f_ops {
    void  (*callback)(ft3dnr_job_t *job);
    void  (*update_status)(ft3dnr_job_t *job, int status);
    int   (*pre_process)(ft3dnr_job_t *job);
    int   (*post_process)(ft3dnr_job_t *job);
    void  (*mark_engine_start)(ft3dnr_job_t *job);
    void  (*mark_engine_finish)(ft3dnr_job_t *job);
    int   (*flush_check)(ft3dnr_job_t *job);
    //void  (*record_property)(ft3dnr_job_t *job);
};

#define LOCK_ENGINE()        (priv.engine.busy=1)
#define UNLOCK_ENGINE()      (priv.engine.busy=0)
#define ENGINE_BUSY()        (priv.engine.busy==1)

typedef struct ft3dnr_rvlut {
    u32                         rlut[6];
    u32                         vlut[6];
} ft3dnr_rvlut_t;

typedef struct ft3dnr_global {
    mrnr_param_t                mrnr;
    tmnr_param_t                tmnr;
    ft3dnr_ctrl_t               ctrl;
    ft3dnr_rvlut_t              rvlut;
    int                         mrnr_id;
    int                         tmnr_id;
} ft3dnr_global_t;

typedef struct ft3dnr_ycbcr {
    u8      src_yc_swap;
    u8      src_cbcr_swap;
    u8      dst_yc_swap;
    u8      dst_cbcr_swap;
    u8      ref_yc_swap;
    u8      ref_cbcr_swap;
} ft3dnr_ycbcr_t;

struct res_base_info_t {
    char name[7];
    int width;
    int height;
};

/* channel base parameters. */
typedef struct chan_param {
    int                 chan_id;
    int                 mrnr_id;
    int                 tmnr_id;
    int                 sp_id;
    unsigned short      src_fmt;
    int                 src_type;
    ft3dnr_dma_buf_t    *var_buf_p;
    res_cfg_t           *ref_res_p;
} chan_param_t;

typedef struct ft3dnr_eng {
    u8                          irq;
    u8                          busy;
    int                         blk_cnt;
    int                         mrnr_cnt;
    u8                          mrnr_table;
    u8                          sram_table;
    u32                         ft3dnr_reg;     // ft3dnr base address
    u8                          null_blk;
    struct list_head            qlist;          // record engineX queue list from V.G, job's status = JOB_STS_QUEUE
    struct list_head            slist;          // record engineX sram list, job's status = JOB_STS_SRAM
    struct list_head            wlist;          // record engineX working list, job's status = JOB_STS_ONGOING
    struct timer_list           timer;
    u32                         timeout;
    u8                          ll_mode;
    u16                         wc_wait_value;
    u16                         rc_wait_value;
    ft3dnr_ycbcr_t              ycbcr;
} ft3dnr_eng_t;

typedef struct ft3dnr_priv {
    spinlock_t                  lock;
    struct semaphore            sema_lock;          ///< driver semaphore lock
    ft3dnr_eng_t                engine;
    struct kmem_cache           *job_cache;
    ft3dnr_global_t             default_param;
    ft3dnr_global_t             isp_param;
    atomic_t                    mem_cnt;
    atomic_t                    buf_cnt;
    struct chan_param           *chan_param;
    res_cfg_t                   *res_cfg;           // record the dim and sort by little-end
    u8                          res_cfg_cnt;        // total count of allocated res_cfg
    struct task_struct          *add_task;
    ft3dnr_dim_t                curr_max_dim;       // record the max dim currently processed
    u8                          curr_reg[0xCC];     // record the register content set by isp for max dim
} ft3dnr_priv_t;

/*
 * extern global variables
 */
extern ft3dnr_priv_t   priv;

void ft3dnr_put_job(void);

void ft3dnr_joblist_add(ft3dnr_job_t *job);

void ft3dnr_init_global(void);

int ft3dnr_set_lli_blk(ft3dnr_job_t *job, int job_cnt, int last_job);

int ft3dnr_fire(void);

void ft3dnr_write_status(void);

int ft3dnr_write_register(ft3dnr_job_t *job);

void ft3dnr_job_free(ft3dnr_job_t *job);

void ft3dnr_stop_channel(int chn);

void ft3dnr_sw_reset(void);

void ft3dnr_dump_reg(void);

void ft3dnr_set_tmnr_rlut(void);

void ft3dnr_set_tmnr_vlut(void);

void ft3dnr_set_tmnr_learn_rate(u8 rate);

void ft3dnr_set_tmnr_his_factor(u8 his);

void ft3dnr_set_tmnr_edge_th(u16 threshold);

#endif
