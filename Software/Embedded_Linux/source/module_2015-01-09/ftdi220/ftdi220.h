#ifndef __FTDI220_H__
#define __FTDI220_H__

#include <linux/version.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,14))
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif

#define USE_JOB_MEMORY

#define FTDI220_VER_MAJOR   3
#define FTDI220_VER_MINOR   9

#define DRIVER_NAME         "ftdi220"
#define FTDI220_MAX_NUM     2

#if defined(CONFIG_PLATFORM_GM8210) || defined(CONFIG_PLATFORM_GM8287)
#define MAX_CHAN_NUM        128     // how many channels this module can support
#elif (defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136))
#define MAX_CHAN_NUM        16
#else
#error "Please define MAX_CHAN_NUM"
#endif

/* Define the max capability of DI220
 */
#define MAX_WIDTH   4096
#define MAX_HIGHT   2048
/* width and height must be multiple of 8 and 2 */
#define WIDTH_MULT  0x8
#define HEIGHT_MULT 0x2

/* 3DI registers */
#define FF_Y_ADDR   0x0
#define FW_Y_ADDR   0x4
#define CR_Y_ADDR   0x8
#define NT_Y_ADDR   0xC
#define DI0_ADDR    0x10
#define DI1_ADDR    0x14
#define CRWB_ADDR   0x18
#define NTWB_ADDR   0x1C
#define COMD        0x20
#define LFHW        0x28
#define TDP         0x2C
#define SDP         0x30
#define MDP         0x34
#define ELAP        0x38
#define MBC         0x3C
#define MDP2        0x64
#define MDP3        0x68
#define OFHW        0x6C
#define SXY         0x70
#define BUS_CTRL	0x74
#define INTR        0x80


//debug level
#define DEBUG_QUIET     0
#define DEBUG_VG        1
#define DEBUG_WARN      2
#define DEBUG_ERR       3
#define DEBUG_DUMPALL   4
#define DEBUG_BLOCK     5

#define FTDI220_SUCCESS 0
#define FTDI220_FAIL    -1

#define OPT_CMD_DISABLED    0x1
#define OPT_CMD_DN_TEMPORAL 0x2     //command: Temporal denoise
#define OPT_CMD_DN_SPATIAL  0x4     //command: Spatial denoised (basic denoise)
#define OPT_CMD_DI          0x8     //command: deinterlace without denoise
#define OPT_CMD_OPP_COPY    0x10    //command: AllStatic, opposite copy in two frames
#define OPT_CMD_FRAME_COPY  0x20    //command: direct frame copy
#define OPT_WRITE_NEWBUF    0x1000  //output to different buffer
#define OPT_CMD_SKIP        0x2000  //skip the sub-channel
#define OPT_HYBRID_WRITE_NEWBUF     0x4000  //output to different buffer

#define OPT_CMD_DN          (OPT_CMD_DN_TEMPORAL | OPT_CMD_DN_SPATIAL)
#define OPT_CMD_DIDN        (OPT_CMD_DN_TEMPORAL | OPT_CMD_DN_SPATIAL | OPT_CMD_DI) /* denoise + deinterlace */

typedef enum {
    FRAME_TYPE_2FRAMES = 0x1,     /* top frame + bottom frame */
    FRAME_TYPE_2FIELDS = 0x2,     /* two field placed in progressive => top field, bottom field */
    FRAME_TYPE_YUV422FRAME = 0x4, /* top line + bottom line + top line + .... */
    FRAME_TYPE_YUV422FRAME_60P = 0x8,   /* one frame mode, 60i -> 60p */
} frame_type_t;

struct ftdi220_job;

typedef struct ftdi220_param {
    int          minor;         //comes from vg layer
    int          chan_id;       //index, from 0 ~
    unsigned int width;
    unsigned int height;
    unsigned int ff_y_ptr;      //Top Field of previous frame
    unsigned int fw_y_ptr;      //Bottom Field of previous frame
    unsigned int cr_y_ptr;      //Top Field of current frame
    unsigned int nt_y_ptr;      //Bottom Field of current frame
    unsigned int di0_ptr;       //the address of the deinterlace result.
    unsigned int di1_ptr;       //top address of the deinterlace result.
    unsigned int crwb_ptr;      //!=0 means the result is outputed to new buffer.
    unsigned int ntwb_ptr;      //!=0 means the result is outputed to new buffer.

    frame_type_t frame_type;
    unsigned int command;
    unsigned int limit_width;
    unsigned int limit_height;
    unsigned int offset_x;      //x offset against the original image
    unsigned int offset_y;      //y offset against the original image
} ftdi220_param_t;

/* job status */
typedef enum {
    JOB_STS_QUEUE   = 0x1,      //not process yet
    JOB_STS_ONGOING = 0x2,      //the job is being in hardware
    JOB_STS_DONE    = 0x4,      //hardware completed already
    JOB_STS_FLUSH   = 0x8,      //drop this job
    JOB_STS_DONOTHING = 0x10,
    JOB_STS_MARK_FLUSH = 0x20,
    JOB_STS_DRIFTING  = 0x8000,
} job_status_t;

struct f_ops {
    void  (*callback)(struct ftdi220_job *job, void *private);
    int   (*pre_process)(struct ftdi220_job *job, void *private);
    int   (*post_process)(struct ftdi220_job *job, void *private);
    void  (*mark_engine_start)(struct ftdi220_job *job, void *private);
    void  (*mark_engine_finish)(struct ftdi220_job *job, void *private);
};

/*
 * job information.
 */
typedef struct ftdi220_job {
    int                 jobid;
    int                 chan_id;
    int                 dev;        //engine id
    ftdi220_param_t     param;
    struct f_ops        fops;
	void                *private;
	job_status_t        status;
	int                 hardware;   // 1 for processed by hardware, 0 for not yet or not
	struct list_head    list;       // per channel link list
	int                 nr_stage;   // for 3dnr enhance, 1 for 1st pass, 2 for 2nd pass
} ftdi220_job_t;

/*
 * Per channel information
 */
typedef struct {
    int                 chan_id;
    int                 dev;
    atomic_t            list_cnt;   // how many jobs queued for service
    atomic_t            finish_cnt;
} chan_info_t;

/*
 * hardware engine status
 */
typedef struct ftdi220_eng {
    int         dev;            //dev=0,1,2,3... engine id
    int         irq;
    int         busy;
    int         handled;
    u32         enhance_nr_cnt;
    u32 __iomem ftdi220_reg;    /* VA */
    ftdi220_job_t       cur_job;
    struct list_head    list;       /* queue list */
    atomic_t            list_cnt;   /* how many jobs are queued */
    int                 enhance_3dnr;
} ftdi220_eng_t;


typedef struct ftdi220_priv {
    unsigned int        jobseq;
    unsigned int        eng_num;
    spinlock_t          lock;

#ifndef USE_JOB_MEMORY
    struct kmem_cache   *job_cache;
#endif

    ftdi220_eng_t       engine[FTDI220_MAX_NUM];
	int					eng_node_count;
} ftdi220_priv_t;


#define LOCK_ENGINE(dev)        (priv.engine[dev].busy=1)
#define UNLOCK_ENGINE(dev)      (priv.engine[dev].busy=0)
#define ENGINE_BUSY(dev)        (priv.engine[dev].busy==1)


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
#define init_MUTEX(sema) sema_init(sema,1)
#endif


#define DRV_LOCK(flags)     spin_lock_irqsave(&priv.lock, flags)
#define DRV_UNLOCK(flags)   spin_unlock_irqrestore(&priv.lock, flags)

/* extern global variables
 */
extern ftdi220_priv_t   priv;
extern chan_info_t      chan_info[];

/*
 * Put a job to fti220 module core
 */
int ftdi220_put_job(int chnum, ftdi220_param_t *param, struct f_ops *fops, void *private, job_status_t status);
/* stop a specific channel */
void ftdi220_stop_channel(int chan_id);
void ftdi220_set_dbgfn(void (*function)(char *, int *, void *));

/* HW specific function
 */
int ftdi220_start_and_wait(ftdi220_job_t *job);
int ftdi220_bus_ctrl(int dev, int value);
void ftdi220_hw_init(int dev);

ftdi220_job_t *ftdi220_drv_alloc_jobmem(void);
void ftdi220_drv_dealloc_jobmem(ftdi220_job_t *job_mem);

#endif /* __FTDI220_H__ */

