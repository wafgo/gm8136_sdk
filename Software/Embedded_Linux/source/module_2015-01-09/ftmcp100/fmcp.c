/* fmcp.c */
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/miscdevice.h>
#include <linux/kallsyms.h>
#include <linux/workqueue.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/kthread.h>
#ifdef HANDLE_BS_CACHEABLE
	#include <asm/cacheflush.h>
    #include <mach/fmem.h>
#endif

#include "frammap_if.h"
#include "ioctl_jd.h"
#include "ioctl_je.h"
#include "ioctl_mp4d.h"
#include "ioctl_mp4e.h"
#ifdef ENABLE_CHECKSUM
    #include "ioctl_cs.h"
#endif
#ifdef ENABLE_ROTATION
    #include "ioctl_rotation.h"
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
	#include <linux/platform_device.h>
	#include <mach/platform/platform_io.h>
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
	#include <linux/device.h>
#endif

#if (defined(TWO_P_EXT) && !defined(SEQ))
    #ifdef ENABLE_ROTATION
        #include "mcp100_int_rt.h"
    #endif
    #include "mcp100_int.h"
#endif
#ifdef SUPPORT_VG
    #include "log.h"
    #include "video_entity.h"
#endif
#include "fmcp.h"
#include "platform.h"
#ifdef DMAWRP
	#include "mp4_wrp_register.h"
#endif
#ifdef REF_POOL_MANAGEMENT
    #ifdef SHARED_POOL
        #include "shared_pool.h"
    #else
        #include "mem_pool.h"
    #endif
#endif

struct mcp100_rso * mcp100_dev = NULL;

#if defined(ONE_P_EXT) && defined ( SUPPORT_VG)
	//"Error configuration: define SUPPORT_VG, and define ONE_P_EXT"
#endif
#if 0
#define C_DEBUG(fmt, args...) printk("FMCP: " fmt, ## args)
#else
#define C_DEBUG(fmt, args...) 
#endif

#undef  PFX
#define PFX	        "FMCP100"
#include "debug.h"

#ifdef GM8210
    #define PLATFORM_NAME   "GM8210"
#elif defined(GM8287)
    #define PLATFORM_NAME   "GM8287"
#elif defined(GM8139)
    #define PLATFORM_NAME   "GM8139"
#elif defined(GM8136)
    #define PLATFORM_NAME   "GM8136"
#else
    #define PLATFORM_NAME   "unknown"
#endif


#if defined(GM8120)
	#define INCTL       		(0x10000 + 0x007c)
#else
	#define INCTL       		(0x20000 + 0x007c)
#endif

#ifdef FIE8150
	#define MCP100W_FTMCP100_PA_BASE	FPGA_EXT2_VA_BASE;
#endif

#define DIV_10000(n) (((n)+0xFFFF)/0x10000)*0x10000
#define DIV_1000(n) (((n)+0x0FFF)/0x1000)*0x1000

#ifndef SUPPORT_VG
	unsigned int mcp100_max_width = MAX_DEFAULT_WIDTH;
	unsigned int mcp100_max_height = MAX_DEFAULT_HEIGHT;
	//unsigned int mcp100_max_width = 720;
	//unsigned int mcp100_max_height = 480;

	#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
		module_param(mcp100_max_width, uint, S_IRUGO|S_IWUSR);
		MODULE_PARM_DESC(mcp100_max_width, "Max Width");
		module_param(mcp100_max_height, uint, S_IRUGO|S_IWUSR);
		MODULE_PARM_DESC(mcp100_max_height, "Max Height");
	#else
		MODULE_PARM(mcp100_max_width,"i");
		MODULE_PARM_DESC(mcp100_max_width, "Max Width");
		MODULE_PARM(mcp100_max_height, "i");
		MODULE_PARM_DESC(mcp100_max_height, "Max Height");
	#endif
#endif
#ifdef SUPPORT_PWM
    unsigned int pwm = 0;
    module_param(pwm, uint, S_IRUGO|S_IWUSR);
    MODULE_PARM_DESC(pwm, "pwm period");
#endif
#ifdef ENABLE_SWITCH_CLOCK
    unsigned int mcp100_switch_clock = 0;
    module_param(mcp100_switch_clock, uint, S_IRUGO|S_IWUSR);
    MODULE_PARM_DESC(mcp100_switch_clock, "switch clock");
#endif
//#ifdef MPEG4_COMMON_PRED_BUFFER
    unsigned int mp4_dec_enable = 0;
    module_param(mp4_dec_enable, uint, S_IRUGO|S_IWUSR);
    MODULE_PARM_DESC(mp4_dec_enable, "mpeg4 decode enable");
//#endif
char *config_path = "/mnt/mtd";
module_param(config_path, charp, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(config_path, "set config path");
unsigned int mp4_tight_buf = 0;
module_param(mp4_tight_buf, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(mp4_tight_buf, "tight buffer");
unsigned int mp4_overspec_handle = 1;
module_param(mp4_overspec_handle, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(mp4_overspec_handle, "overspec handle");
// mp4_overspec_handle: 0: no activity (filter), 1: keep all channel
#ifdef ENABLE_ROTATION
    unsigned int mcp100_rotation_enable = 0;
    module_param(mcp100_rotation_enable, uint, S_IRUGO|S_IWUSR);
    MODULE_PARM_DESC(mcp100_rotation_enable, "rotation enable");
#endif

#define RSVD_SZ PAGE_SIZE //0x10

#ifdef USE_KTHREAD
    static struct task_struct *mcp100_cb_task = NULL;
    static wait_queue_head_t mcp100_cb_waitqueue;
    static int mcp100_cb_wakeup_event = 0;  // for mcp100_cb_waitqueue wake up condition
    static volatile int mcp100_cb_thread_ready = 0;
#else
    static struct workqueue_struct *mcp100_cb_workq = NULL;
    static DECLARE_DELAYED_WORK(process_callback_work, 0);
#endif
#ifdef USE_KTHREAD2
    static struct task_struct *mcp100_getjob_task = NULL;
    static wait_queue_head_t mcp100_getjob_waitqueue;
    static int mcp100_getjob_wakeup_event = 0;
    static volatile int mcp100_getjob_thread_ready = 0;
#else
    // use @ check sum funcion done
    static struct workqueue_struct *mcp100_getjob_workq = NULL;
    static DECLARE_DELAYED_WORK(process_getjob_work, 0);
#endif

extern int mcp100_proc_init(void);
extern void mcp100_proc_close(void);
#ifdef ENABLE_CHECKSUM
extern int check_sum_init(void);
extern int check_sum_exit(void);
extern int calculate_checksum(unsigned int buf_pa, unsigned int len, int type);
#endif

#ifndef SUPPORT_VG
    wait_queue_head_t mcp100_codec_queue;
    struct semaphore fmutex;
    //unsigned int mcp100_bs_virt = 0,mcp100_bs_phy = 0;
    //unsigned int mcp100_bs_length = 0;
    struct buffer_info_t mcp100_bs_buffer = {0, 0, 0};
#else
    #ifdef USE_SPINLOCK
        spinlock_t mcp100_queue_lock;
    #else
        struct semaphore mcp100_mutex;
    #endif
    #ifdef HW_LOCK_ENABLE
        spinlock_t mcp100_hw_lock;
    #endif
#endif
static struct list_head *mcp100_engine_head = NULL;
static struct list_head *mcp100_chn_list[FMCP_MAX_TYPE] = {NULL, NULL, NULL, NULL}; // stop driver

// for job flow
#ifdef ENABLE_ROTATION
    static int max_channel_number[FMCP_MAX_TYPE] = {MP4E_MAX_CHANNEL, MP4D_MAX_CHANNEL, MJE_MAX_CHANNEL, MJD_MAX_CHANNEL, RT_MAX_CHANNEL};
#else
    static int max_channel_number[FMCP_MAX_TYPE] = {MP4E_MAX_CHANNEL, MP4D_MAX_CHANNEL, MJE_MAX_CHANNEL, MJD_MAX_CHANNEL};
#endif
static int last_type = TYPE_NONE;
static mcp100_rdev * mcp100dev[FMCP_MAX_TYPE] = {0};

/* proc parameter */
int log_level = LOG_WARNING;
unsigned int callback_period = 2;// msec
unsigned int utilization_period;
unsigned int utilization_start[MAX_ENGINE_NUM], utilization_record[MAX_ENGINE_NUM];

#ifdef ENABLE_ROTATION
    static const char codec_name[FMCP_MAX_TYPE][10] = {"MPEG4_E", "MPEG4_D", "JPEG_E", "JPEG_D", "ROTATION"};
#else
    static const char codec_name[FMCP_MAX_TYPE][10] = {"MPEG4_E", "MPEG4_D", "JPEG_E", "JPEG_D"};
#endif
//volatile int count_kmalloc_size=0,count_high_size=0;
int gDDR_ID = 0;
int allocate_cnt = 0;
int allocate_ddr_cnt = 0;
//unsigned int cs_type = 3;
//unsigned int cs_len = 0;
#ifdef ENABLE_CHECKSUM
extern wait_queue_head_t mcp100_cs_wait_queue;
extern spinlock_t mcp100_cs_lock;
unsigned int cs_enable = 0;
unsigned int cs_wait = 0;
unsigned int cs_result = 0;
extern void fcheck_sum_isr(int irq, void *dev_id, unsigned int base);
#endif

#ifdef SUPPORT_VG
    #if 0
    #define DEBUG_M(level, fmt, args...) { \
        if (log_level >= level) \
            printm(MCP100_MODULE_NAME, fmt, ## args); }
    #else
    #define DEBUG_M(level, fmt, args...) { \
        if (log_level == LOG_DIRECT) \
            printk(fmt, ## args); \
        else if (log_level >= level) \
            printm(MCP100_MODULE_NAME, fmt, ## args); }
    #define DEBUG_E(fmt, args...) { \
                printm(MCP100_MODULE_NAME,  fmt, ## args); \
                printk(fmt, ##args); }
    #endif
#else
    #define DEBUG_M(level, fmt, args...) { \
        if (log_level >= level) \
            printk(fmt, ## args); }
    void damnit(char *name) {;}
#endif

void *fkmalloc(size_t size, uint8_t alignment, uint8_t reserved_size)
{
    unsigned int    sz = size;//+RSVD_SZ+alignment+reserved_size;
    void            *ptr;

    //sz = DIV_1000(size);

    ptr = kmalloc(sz,GFP_ATOMIC);
    
    C_DEBUG("fkmalloc for addr 0x%x size 0x%x\n", (unsigned int)ptr, (unsigned int)sz);
    
    if(ptr==0) {
        printk("Fail to fkmalloc for addr 0x%x size 0x%x\n", (unsigned int)ptr, (unsigned int)sz);
        return 0;
    }
    allocate_cnt++;
    
    return ptr;
}

void fkfree(void *ptr)
{
    if (ptr) {
		kfree(ptr);
        allocate_cnt--;
    }
}

int release_frammap_buffer(struct buffer_info_t *buf)
{
    if (buf->addr_va) {
        DEBUG_M(LOG_INFO, "free buffer(frammap) pa 0x%08x, va 0x%08x, size %d\n", buf->addr_va, buf->addr_pa, buf->size);
        frm_free_buf_ddr((void *)buf->addr_va);
        memset(buf, 0, sizeof(struct buffer_info_t));
    }
    return 0;
}

/*
void * fcnsist_alloc(uint32_t size, void **phy_ptr, int attr_mmu)	// 1: cachable, 2: bufferable, 0: none
{
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22))
		struct frammap_buf_info buf_info;
		memset(&buf_info, 0, sizeof(struct frammap_buf_info));
		buf_info.size = size;
		buf_info.alloc_type = (attr_mmu == 1)? ALLOC_CACHEABLE:
													((attr_mmu == 2)? ALLOC_BUFFERABLE: 0);
		if (frm_get_buf_ddr(DDR_ID_SYSTEM, &buf_info) < 0) {
			*phy_ptr = NULL;
			return NULL;
		}
		*phy_ptr = (void *)buf_info.phy_addr;
		return (void *)buf_info.va_addr;
	#else
		return fmem_alloc(size, (dma_addr_t*)phy_ptr, (attr_mmu == 1)? PAGE_COPY:
																								((attr_mmu == 2)? pgprot_noncached(pgprot_kernel):
																																	pgprot_writecombine(pgprot_kernel)));
	#endif
}
void fcnsist_free(uint32_t size, void * virt_ptr, void * phy_ptr)
{
	#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,27))
		struct frammap_buf_info buf_info;
		memset(&buf_info, 0, sizeof(struct frammap_buf_info));
		buf_info.va_addr = (u32)virt_ptr;
		buf_info.phy_addr = (u32)phy_ptr;
		frm_free_buf_ddr (&buf_info);
	#else
		fmem_free(size, virt_ptr, (dma_addr_t)phy_ptr);
	#endif
}
*/

int allocate_buffer(struct buffer_info_t *buf, int size)
{
    struct frammap_buf_info buf_info;
    memset(&buf_info, 0, sizeof(struct frammap_buf_info));
    buf_info.size = size;
    buf_info.align = 32;
    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
        buf_info.name = "h264e";
    #endif
    buf_info.alloc_type = ALLOC_NONCACHABLE;

    if (1 == gDDR_ID) {
        if (frm_get_buf_ddr(DDR_ID1, &buf_info) < 0)
            return -1;
    }
    else {
        if (frm_get_buf_ddr(DDR_ID_SYSTEM, &buf_info) < 0)
            return -1;
    }
    buf->addr_va = (unsigned int)buf_info.va_addr;
    buf->addr_pa = (unsigned int)buf_info.phy_addr;
    buf->size = size;
    if (0 == buf->addr_va)
        return -1;
    DEBUG_M(LOG_INFO, "allocate buffer va 0x%x, pa 0x%x, size %d\n", buf->addr_va, buf->addr_pa, buf->size);
    return 0;
}

int allocate_buffer_from_ddr(struct buffer_info_t *buf, int size, int ddr_idx)
{
    struct frammap_buf_info buf_info;
    memset(&buf_info, 0, sizeof(struct frammap_buf_info));
    buf_info.size = size;
    buf_info.align = 32;
    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
        buf_info.name = "h264e";
    #endif
    buf_info.alloc_type = ALLOC_NONCACHABLE;

    if (1 == ddr_idx) {
        if (frm_get_buf_ddr(DDR_ID1, &buf_info) < 0)
            return -1;
    }
    else {
        if (frm_get_buf_ddr(DDR_ID_SYSTEM, &buf_info) < 0)
            return -1;
    }
    buf->addr_va = (unsigned int)buf_info.va_addr;
    buf->addr_pa = (unsigned int)buf_info.phy_addr;
    buf->size = size;
    if (0 == buf->addr_va)
        return -1;
    DEBUG_M(LOG_INFO, "allocate buffer va 0x%x, pa 0x%x, size %d\n", buf->addr_va, buf->addr_pa, buf->size);
    return 0;
}


int free_buffer(struct buffer_info_t *buf)
{
    if (buf->addr_va) {
        DEBUG_M(LOG_INFO, "free buffer va 0x%x,  pa 0x%x\n", buf->addr_va, buf->addr_pa);
    #ifdef ALLOCATE_FROM_GRAPH
        //video_free_buffer_simple((void *)buf->addr_va);
    #else
        frm_free_buf_ddr((void*)buf->addr_va);
    #endif
        //memset(buf, 0, sizeof(struct buffer_info_t));
        buf->addr_va = 0;
        buf->addr_pa = 0;
        buf->size = 0;
    }
    return 0;
}

void * fconsistent_alloc(uint32_t size, uint8_t align_size, uint8_t reserved_size, void **phy_ptr)
{
    struct frammap_buf_info buf_info;

    memset(&buf_info, 0, sizeof(struct frammap_buf_info));
    buf_info.size = size;
    buf_info.align = 32;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
    buf_info.name = "mcp100_enc";
#endif
    buf_info.alloc_type = ALLOC_NONCACHABLE;
    if (frm_get_buf_ddr(DDR_ID_SYSTEM, &buf_info) < 0)
        return NULL;
    *phy_ptr = (void *)buf_info.phy_addr;
    LOG_PRINT(LOG_INFO, "%s: allocate buffer va 0x%x, pa 0x%x, size %d\n", __FUNCTION__, (unsigned int)buf_info.va_addr, (unsigned int)buf_info.phy_addr, size);
    allocate_ddr_cnt++;
    return (void *)buf_info.va_addr;
}

void fconsistent_free(void * virt_ptr, void * phy_ptr)
{
    if (0 == virt_ptr || 0 == phy_ptr)
        return;
    LOG_PRINT(LOG_INFO, "%s: free buffer va 0x%x, pa 0x%x\n", __FUNCTION__, (unsigned int)virt_ptr, (unsigned int)phy_ptr);    
    if (frm_free_buf_ddr((void*)virt_ptr) < 0)
       return;
    allocate_ddr_cnt--;
}

#ifndef ENABLE_CHECKSUM
/* fake check sum */
int calculate_checksum(unsigned int buf_pa, unsigned int len, int type)
{
    return 0;
}
#endif

#ifdef SUPPORT_VG
int is_hardware_idle(void)
{
    return (last_type == TYPE_NONE);
}

int set_running_codec(int codec_type)
{
    if (codec_type < TYPE_MAX && codec_type > TYPE_NONE) {
        last_type = codec_type;
        return codec_type;
    }
    last_type = TYPE_NONE;
    return 0;
}

#ifdef HW_LOCK_ENABLE
int set_hw_state(int codec_type)
{
    unsigned long flags;
    int ret = TYPE_NONE;
    spin_lock_irqsave(&mcp100_hw_lock,flags);
    DEBUG_M(LOG_DEBUG, "set state %d (%d)\n", codec_type, last_type);
    if (is_hardware_idle() || TYPE_NONE == codec_type) {
        ret = set_running_codec(codec_type);
    }
    spin_unlock_irqrestore(&mcp100_hw_lock,flags);
    //DEBUG_M(LOG_DEBUG, "set state %d done\n", ret);
    return ret;
}
#endif

void callback_scheduler(void)
{
    int engine, engine_idx, chn;
    struct video_entity_job_t *job;
    struct job_item_t *job_item, *job_item_next;

#ifdef USE_SPINLOCK
    unsigned long flags;
    spin_lock_irqsave(&mcp100_queue_lock,flags);
#else
    down(&mcp100_mutex);
#endif

    for (engine = 0; engine < MAX_ENGINE_NUM; engine++) {
        list_for_each_entry_safe(job_item, job_item_next, &mcp100_engine_head[engine], engine_list) {
            engine_idx = job_item->engine;
            chn = job_item->chn;
            job = (struct video_entity_job_t *)job_item->job;
            if(job_item->status == DRIVER_STATUS_FINISH) {
                job->status = JOB_STATUS_FINISH;
                list_del(&job_item->engine_list);
                list_del(&job_item->minor_list);

            #ifdef ENABLE_CHECKSUM
                if (job_item->check_sum_type) {         
                    struct video_property_t *property = job->out_prop;
                    unsigned int value;
                    int i = 0;
                    value = calculate_checksum(job->out_buf.buf[0].addr_pa, job_item->bs_size, job_item->check_sum_type&0xF);
                    while (property[i].id != 0) {
                        if (ID_CHECKSUM == property[i].id) {
                            property[i].value = value;
                            break;
                        }
                        i++;
                    }
               }
            #endif
                //DEBUG_M(LOG_DEBUG, "{chn %d} %s job %d status finish callback\n",
                //    chn, codec_name[job_item->codec_type], job_item->job_id);
            #ifdef HANDLE_BS_CACHEABLE  // Tuba 20140225: handle bitstream buffer cacheable
                if (TYPE_JPEG_ENCODER == job_item->codec_type || TYPE_MPEG4_ENCODER == job_item->codec_type) {
                    fmem_dcache_sync((void *)job->out_buf.buf[0].addr_va, job_item->bs_size, DMA_FROM_DEVICE);
                }
            #endif
                job->callback(job); //callback root job
                DEBUG_M(LOG_DEBUG, "{chn %d} %s job %u status finish callback\n",
                    chn, codec_name[job_item->codec_type], job_item->job_id);
                driver_free(job_item);
                continue;
            } 
            else if(job_item->status == DRIVER_STATUS_FAIL) {
                job->status = JOB_STATUS_FAIL;
                list_del(&job_item->engine_list);
                list_del(&job_item->minor_list);

                job->callback(job); //callback root job
                DEBUG_M(LOG_WARNING, "{chn %d} %s job %d status fail callback\n",
                    chn, codec_name[job_item->codec_type], job_item->job_id);
                driver_free(job_item);
                continue;
            }        
            break;
        }
    }
#ifdef USE_SPINLOCK
    spin_unlock_irqrestore(&mcp100_queue_lock,flags);
#else
    up(&mcp100_mutex);
#endif
}

#ifdef USE_KTHREAD
static int mcp100_cb_thread(void *data)
{
    int status;

    mcp100_cb_thread_ready = 1;
    do {
        status = wait_event_timeout(mcp100_cb_waitqueue, mcp100_cb_wakeup_event, msecs_to_jiffies(callback_period));
        if (0 == status)
            continue;
        mcp100_cb_wakeup_event = 0;
        callback_scheduler();
    } while (!kthread_should_stop());
    mcp100_cb_thread_ready = 0;
    return 0;
}
static void mcp100_cb_wakeup(void)
{
    mcp100_cb_wakeup_event = 1;
    wake_up(&mcp100_cb_waitqueue);
}
#endif

void trigger_callback_finish(void *job_param)
{

    struct job_item_t *job_item=(struct job_item_t *)job_param;
//    unsigned long flags;
//    spin_lock_irqsave(&mcp100_queue_lock,flags);
    job_item->finishtime = jiffies;
    job_item->status=DRIVER_STATUS_FINISH;  // not protect, because protect @ caller function
#ifdef USE_KTHREAD
    mcp100_cb_wakeup();
#else
    #if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19))                    
    PREPARE_DELAYED_WORK(&process_callback_work, (void *)callback_scheduler);
    #else
    PREPARE_WORK(&process_callback_work,(void *)callback_scheduler,(void *)0);
    #endif
    queue_delayed_work(mcp100_cb_workq, &process_callback_work,msecs_to_jiffies(callback_period));
#endif
//    spin_unlock_irqrestore(&mcp100_queue_lock,flags);
}

void trigger_callback_fail(void *job_parm)
{
    struct job_item_t *job_item=(struct job_item_t *)job_parm;
//    unsigned long flags;
//    spin_lock_irqsave(&mcp100_queue_lock,flags);
    job_item->finishtime = jiffies;
    job_item->status=DRIVER_STATUS_FAIL;   // not protect, because protect @ caller function
#ifdef USE_KTHREAD
    mcp100_cb_wakeup();
#else
    #if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19))                    
    PREPARE_DELAYED_WORK(&process_callback_work,(void *)callback_scheduler);
    #else
    PREPARE_WORK(&process_callback_work,(void *)callback_scheduler,(void *)0);
    #endif
    queue_delayed_work(mcp100_cb_workq, &process_callback_work,msecs_to_jiffies(callback_period));
#endif
//    spin_unlock_irqrestore(&mcp100_queue_lock,flags);
}


#if 0
static int start_job(struct v_job_t *job)
{
	int i;
	int ret;
	struct video_entity_t * entity = job->entity;
	int codec_type = entity->minor >> 8;

	for (i = 0; i < 4; i ++) {
		//printk("i = %d\n", (int)i);
		if (mcp100dev[i]) {
			if (codec_type == mcp100dev[i]->codec_type)
				break;
		}
	}
	if (i >= 4) {
		printk ("fatal error: no register for this codec_type %d\n", codec_type);
		while (1);
		return -1;
	}
	mcp100dev[i]->curjob = job;
	ret = mcp100dev[i]->job_tri (job);

	if (dump_n_frame)
		dump_n_frame --;
	if (ret < 0)
		return ret;
	return i;
}
#else
static int start_job(struct job_item_t *job_item)
{
//	int i;
	int ret;
	int codec_type = job_item->codec_type;
    if (mcp100dev[codec_type]) {
        mcp100dev[codec_type]->curjob = job_item;
        DEBUG_M(LOG_DEBUG, "start job %u codec %s\n", job_item->job_id, codec_name[job_item->codec_type]);
        job_item->starttime = jiffies;
        ret = mcp100dev[codec_type]->job_tri(job_item);
    }
    else {
        printk("codec type %d is not register\n", codec_type);
        return -1;
    }
    return ret;
}
#endif

void work_scheduler(void *param)
{
    struct job_item_t *job_item;
#if defined(USE_SPINLOCK)|defined(ENABLE_CHECKSUM)
    unsigned long flags;
#endif
    int engine = (int)param;
#ifdef ENABLE_SWITCH_CLOCK
    int trigger_job = 0;
#endif

    engine = 0;
    // set engine
    if (list_empty(&mcp100_engine_head[engine])) {
        DEBUG_M(LOG_DEBUG, "list %d empty\n", engine);
        return;
    }

#ifdef USE_SPINLOCK
    spin_lock_irqsave(&mcp100_queue_lock, flags);
#else
    if (0 == in_irq())
        down(&mcp100_mutex);
#endif
    if (is_hardware_idle()) {
    #ifdef ENABLE_SWITCH_CLOCK
        if (mcp100_switch_clock) {
            pf_clock_switch_on();
            //if (pf_clock_switch_on() > 0)
            //    fmcp100_resetrun (mcp100_dev->va_base);
        }
    #endif
        
        list_for_each_entry(job_item, &mcp100_engine_head[engine], engine_list) {
            if (job_item->status == DRIVER_STATUS_STANDBY) {
            #ifdef HW_LOCK_ENABLE
                if (set_hw_state(job_item->codec_type) == TYPE_NONE) {
                    DEBUG_M(LOG_WARNING, "hw busy\n");
                    break;
                }
            #else
                if (set_running_codec(job_item->codec_type) == TYPE_NONE) {
                    trigger_callback_fail(job_item);
                    continue;
                }
            #endif
                job_item->status = DRIVER_STATUS_ONGOING;
                if (start_job(job_item) < 0) {
                    trigger_callback_fail(job_item);
                #ifdef ENABLE_CHECKSUM	// Tuba 20141107: wehn trigger job error, check checksum wait event
                    spin_lock_irqsave(&mcp100_cs_lock, flags);
                    if (cs_wait) {
                        cs_wait = 0;
                    #ifdef HW_LOCK_ENABLE
                        set_hw_state(TYPE_CHECK_SUM);
                    #else
                        set_running_codec(TYPE_CHECK_SUM);
                    #endif
                        spin_unlock_irqrestore(&mcp100_cs_lock, flags);
                        wake_up(&mcp100_cs_wait_queue);
                        break;
                   }
                   else {
                   #ifdef HW_LOCK_ENABLE
                       set_hw_state(TYPE_NONE);
                   #else
                       set_running_codec(TYPE_NONE);
                   #endif
                   }
                   spin_unlock_irqrestore(&mcp100_cs_lock, flags);
                #else
                    #ifdef HW_LOCK_ENABLE
                    set_hw_state(TYPE_NONE);
                    #else
                    set_running_codec(TYPE_NONE);
                    #endif
                #endif
                }
                else {
                #ifdef ENABLE_SWITCH_CLOCK
                    trigger_job = 1;
                #endif
                    break;
                }
            }
        }
    #ifdef ENABLE_SWITCH_CLOCK
        if (0 == trigger_job && mcp100_switch_clock)
            pf_clock_switch_off();
    #endif
    }
#ifdef USE_SPINLOCK
    spin_unlock_irqrestore(&mcp100_queue_lock, flags);
#else
    if (0 == in_irq())
        up(&mcp100_mutex);
#endif
}

static int mcp_trigger_next_job(int engine)
{
    work_scheduler((void*)engine);
	return 0;
}

#ifdef USE_KTHREAD2
static int mcp100_getjob_thread(void *data)
{
    int status;
    mcp100_getjob_thread_ready = 1;
    do {
        status = wait_event_timeout(mcp100_getjob_waitqueue, mcp100_getjob_wakeup_event, 1);
        if (0 == status)
            continue;
        mcp100_getjob_wakeup_event = 0;
        work_scheduler((void *)0);
    } while (!kthread_should_stop());
    mcp100_getjob_thread_ready = 0;
    return 0;
}
static void mcp100_getjob_wakeup(void)
{
    mcp100_getjob_wakeup_event = 1;
    wake_up(&mcp100_getjob_waitqueue);
}
#endif

#ifdef ENABLE_CHECKSUM
static int mcp_trigger_next_job_by_wq(void)
{
#ifdef USE_KTHREAD2
    mcp100_getjob_wakeup();
#else
    #if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19))                    
    PREPARE_DELAYED_WORK(&process_getjob_work,(void *)work_scheduler);
    #else
    PREPARE_WORK(&process_getjob_work,(void *)work_scheduler,(void *)0);
    #endif
    queue_delayed_work(mcp100_getjob_workq, &process_getjob_work, 0);
#endif
    return 0;
}
#endif

#ifdef HANDLE_BS_CACHEABLE
int get_bitstream_length(struct video_entity_job_t *job)
{
    int i = 0;
    struct video_property_t *property=job->in_prop;
    while(property[i].id!=0) {
        if (ID_BITSTREAM_SIZE == property[i].id) {
            return property[i].value;
        }
        i++;
    }
    return 0;
}
#endif


//int mcp_putjob(struct video_entity_job_t *job, int codec_type, int width, int height)
int mcp_putjob(struct video_entity_job_t *job, int codec_type)
{
#ifdef USE_SPINLOCK
    unsigned long flags;
#endif
    struct job_item_t *job_item;
    int idx;

    DEBUG_M(LOG_DEBUG, "put job %u from codec %s\n", job->id, codec_name[codec_type]);
    job_item = driver_alloc(sizeof(struct job_item_t));
    if (0 == job_item) {
        DEBUG_E("Allocate job_item error, codec type %s\n", codec_name[codec_type]);
        damnit(MCP100_MODULE_NAME);
    }
    job_item->job = job;
    job_item->job_id = job->id;
    job_item->engine = ENTITY_FD_ENGINE(job->fd);
    if(job_item->engine >= MAX_ENGINE_NUM) {
        DEBUG_E("Error to put job engine %d, max is %d\n", job_item->engine, MAX_ENGINE_NUM);
        damnit(MCP100_MODULE_NAME);
    }

    job_item->chn = ENTITY_FD_MINOR(job->fd);
    if(job_item->chn >= max_channel_number[codec_type]) {
        DEBUG_E("Put job: error to put job chn %d, max is %d, codec type %s\n",job_item->chn, max_channel_number[codec_type], codec_name[codec_type]);
        damnit(MCP100_MODULE_NAME);
    }
#ifdef ENABLE_ROTATION
    if (0 == mcp100_rotation_enable && TYPE_MCP100_ROTATION == codec_type) {
        DEBUG_E("mcp100_rotation_enable is not enable, but there is a rotation job %u\n", job->id);
        trigger_callback_fail(job_item);
        damnit(MCP100_MODULE_NAME);
        return -1;
    }
#endif

    job_item->status = DRIVER_STATUS_STANDBY;
    job_item->puttime = jiffies;
    job_item->starttime = job_item->finishtime = 0;
    job_item->codec_type = codec_type;
#ifdef MULTI_FORMAT_DECODER
    //job_item->frame_width = width;
    //job_item->frame_height = height;
#endif
    idx = MAKE_IDX(max_channel_number[codec_type], job_item->engine, job_item->chn);
    INIT_LIST_HEAD(&job_item->engine_list);
    INIT_LIST_HEAD(&job_item->minor_list);
#ifdef USE_SPINLOCK
    spin_lock_irqsave(&mcp100_queue_lock, flags);
#else
    down(&mcp100_mutex);
#endif
    list_add_tail(&job_item->engine_list, &mcp100_engine_head[job_item->engine]);
    list_add_tail(&job_item->minor_list, &mcp100_chn_list[codec_type][idx]);  // for stop channel
#ifdef USE_SPINLOCK
    spin_unlock_irqrestore(&mcp100_queue_lock, flags);
#else
    up(&mcp100_mutex);
#endif
#ifdef HANDLE_BS_CACHEABLE  // Tuba 20140225: handle bitstream buffer cacheable
    if (TYPE_MPEG4_DECODER == codec_type || TYPE_JPEG_DECODER == codec_type) {
        job_item->bs_size = get_bitstream_length(job);
        if (job_item->bs_size > 0) {
        #if 0
            fmem_dcache_sync((void *)job->in_buf.buf[0].addr_va, job_item->bs_size, DMA_TO_DEVICE);
        #else
            fmem_dcache_sync((void *)job->in_buf.buf[0].addr_va, job_item->bs_size, DMA_BIDIRECTIONAL);
        #endif
        }
    }
#endif

    if (is_hardware_idle()) {
        work_scheduler((void *)0);
    }

    return JOB_STATUS_ONGOING;
}

/* stop job
 * output: is there a job which is on-going
 *          0: no on going job -> release all source
 *          1: should not release resource until job done
*/
int mcp_stopjob(int codec_type, int engine, int chn)
{
#ifdef USE_SPINLOCK
    unsigned long flags;
#endif
    struct job_item_t *job_item;
    int idx;// = MAKE_IDX(MJE_MAX_CHANNEL, engine, chn);
    int is_ongoing_job = 0;

    if (engine >= MAX_ENGINE_NUM) {
        DEBUG_M(LOG_ERROR, "Error to stop job engine %d, max is %d\n", engine, MAX_ENGINE_NUM);
        damnit(MCP100_MODULE_NAME);
    }
    if (chn >= max_channel_number[codec_type]) {
        DEBUG_M(LOG_ERROR, "Put job: error to stop job chn %d, max is %d, codec type %s\n", chn, max_channel_number[codec_type], codec_name[codec_type]);
        damnit(MCP100_MODULE_NAME);
    }

    idx = MAKE_IDX(MJE_MAX_CHANNEL, engine, chn);
#ifdef USE_SPINLOCK
    spin_lock_irqsave(&mcp100_queue_lock, flags);
#else
    down(&mcp100_mutex);
#endif
    list_for_each_entry (job_item, &mcp100_chn_list[codec_type][idx], minor_list) {
        if (job_item->status == DRIVER_STATUS_STANDBY)
            trigger_callback_fail((void *)job_item);
        else if (job_item->status == DRIVER_STATUS_KEEP)
            trigger_callback_finish((void *)job_item);
        else if (job_item->status == DRIVER_STATUS_ONGOING) {
            is_ongoing_job = 1;
            DEBUG_M(LOG_DEBUG, "{chn%d} job %u is on-going\n", chn, job_item->job_id);
        }
    }
#ifdef USE_SPINLOCK
    spin_unlock_irqrestore(&mcp100_queue_lock, flags);
#else
    up(&mcp100_mutex);
#endif
    return is_ongoing_job;
}
#endif


#ifdef DMAWRP
static void mp4e_wrp_reset (void)
{
	struct mcp100_rso * devrso = mcp100_dev;
	volatile WRP_reg * wrpreg = (volatile WRP_reg *)devrso->va_base_wrp;

	wrpreg->pf_go = (1L << 31); /// reset wrapper
	wrpreg->Vfmt = (1L << 7);		// bypass mode
}
#endif

void mcp100_switch(void * codec, ACTIVE_TYPE type)
{
	int i;
#ifdef DMAWRP
	mp4e_wrp_reset ();
#endif
	for (i = 0; i < 4; i ++) {
		if (mcp100dev[i] ) {
			if (mcp100dev[i]->switch22) {
				mcp100dev[i]->switch22(codec, type);
			}
		}
	}
	//printk ("set last_type = %d\n", last_type);
#ifndef HW_LOCK_ENABLE
	last_type = type;
#endif
}
//int mcp100_register(mcp100_rdev *dv, ACTIVE_TYPE type)		//return >=0 OK, <0 fail
int mcp100_register(mcp100_rdev *dv, ACTIVE_TYPE type, int max_chn_num)
{
    unsigned int list_max_chn;
	switch (type) {
		case TYPE_MPEG4_ENCODER:
        case TYPE_MPEG4_DECODER:
        case TYPE_JPEG_ENCODER:
        case TYPE_JPEG_DECODER:
    #ifdef ENABLE_ROTATION
        case TYPE_MCP100_ROTATION:
    #endif
            if (TYPE_MPEG4_ENCODER == type)
                list_max_chn = MP4E_MAX_CHANNEL;
            else if (TYPE_MPEG4_DECODER == type)
                list_max_chn = MP4D_MAX_CHANNEL;
            else if (TYPE_JPEG_ENCODER == type)
                list_max_chn = MJE_MAX_CHANNEL;
            else if (TYPE_JPEG_DECODER== type)
                list_max_chn = MJD_MAX_CHANNEL;
            else if (TYPE_MCP100_ROTATION== type)
                list_max_chn = RT_MAX_CHANNEL;
            if (max_chn_num > list_max_chn) {
                mcp100_err("%s max channel can not over (%d>%d), force be %d\n", 
                    codec_name[type], max_chn_num, list_max_chn, list_max_chn);
                max_chn_num = list_max_chn;
            }
            max_channel_number[type] = max_chn_num;
			break;
		default:
			DEBUG_E("%s: unknown codec type %d", __FUNCTION__, type);
            damnit(MCP100_MODULE_NAME);
			return -1;
	}
	//info ("type = %d, dv = 0x%x", type, (int)dv);
	mcp100dev[type] = dv;
	return 0;
}
int mcp100_deregister(ACTIVE_TYPE type)
{
	switch (type) {
		case TYPE_MPEG4_ENCODER:
        case TYPE_MPEG4_DECODER:
        case TYPE_JPEG_ENCODER:
        case TYPE_JPEG_DECODER:
    #ifdef ENABLE_ROTATION
        case TYPE_MCP100_ROTATION:
    #endif
			break;
		default:
            DEBUG_E("%s: unknown codec type %d", __FUNCTION__, type);
            damnit(MCP100_MODULE_NAME);
			return -1;
	}
	mcp100dev[type] = NULL;
	return 0;
}

#ifdef TWO_P_EXT
//static void mcp_isr (int irq, unsigned int * base)
void mcp_isr (int irq, unsigned int * base)
{
	mcp100_rdev * mcpdev;

#ifdef ENABLE_CHECKSUM
    unsigned long flags;
    spin_lock_irqsave(&mcp100_cs_lock, flags); 
    if (cs_enable) {
        fcheck_sum_isr(irq, NULL, *base);
    #ifdef HW_LOCK_ENABLE
        set_hw_state(TYPE_NONE);
    #else
        last_type = TYPE_NONE;
    #endif
        spin_unlock_irqrestore(&mcp100_cs_lock, flags);
        wake_up(&mcp100_cs_wait_queue);
        mcp_trigger_next_job_by_wq();
        return;
    }
    spin_unlock_irqrestore(&mcp100_cs_lock, flags);
#endif

    DEBUG_M(LOG_DEBUG, "mcp100 isr codec = %s\n", codec_name[last_type]);

	if ((last_type <= TYPE_NONE) || (last_type >= TYPE_MAX)) {
		DEBUG_E("fatal error: no this last_type %d\n", last_type);
        damnit(MCP100_MODULE_NAME);
	}
	mcpdev = mcp100dev[last_type];
	if (mcpdev == NULL) {
        DEBUG_E("fatal error: last_type %d not registered\n", last_type);
		damnit(MCP100_MODULE_NAME);
	}

	mcpdev->handler(irq, mcpdev->dev_id, *base);
	#ifdef SUPPORT_VG
	{
		struct job_item_t *job_item = mcpdev->curjob;
        int engine = job_item->engine;
		int ret = mcpdev->job_done(job_item);        

		mcpdev->curjob = NULL;
		if (ret <= 0) { // done or fail
			if (ret == 0) { // done
			    trigger_callback_finish(job_item);
            }
			else {          // fail
    			trigger_callback_fail(job_item);
            }
            // trigger get next job
            //last_type = TYPE_NONE;
        #ifdef ENABLE_CHECKSUM
            spin_lock_irqsave(&mcp100_cs_lock, flags);
            if (cs_wait) {
                cs_wait = 0;
			#ifdef HW_LOCK_ENABLE
            	set_hw_state(TYPE_CHECK_SUM);
	        #else
                set_running_codec(TYPE_CHECK_SUM);
			#endif
                spin_unlock_irqrestore(&mcp100_cs_lock, flags);
                wake_up(&mcp100_cs_wait_queue);
                return;
            }
            else {
			#ifdef HW_LOCK_ENABLE
            	set_hw_state(TYPE_NONE);
	        #else
                last_type = TYPE_NONE;
			#endif
			}
            spin_unlock_irqrestore(&mcp100_cs_lock, flags);
        #else
			#ifdef HW_LOCK_ENABLE
            	set_hw_state(TYPE_NONE);
	        #else
                last_type = TYPE_NONE;
			#endif
        #endif
			mcp_trigger_next_job(engine);
		}// esle not done
	}
	#else
		wake_up(&mcp100_codec_queue);
	#endif
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19))
static irqreturn_t mcp100_int_hd(int irq, void *dev)
{
	mcp_isr (irq, (unsigned int *)dev);
	return IRQ_HANDLED;
}
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
static irqreturn_t mcp100_int_hd(int irq, void *dev, struct pt_regs *dummy)
{
	mcp_isr (irq, (unsigned int *)dev);
	return IRQ_HANDLED;
}
#endif  // LINX_VERSION
#endif  // TWO_P_EXT


void fmcp100_resetrun(unsigned int base)
{
#if (defined(TWO_P_EXT) && !defined(SEQ))
	unsigned int reg;
#endif
	int sync = 0;
#if (defined(GM8120) ||defined(FIE8100))
	char *reg_ptr=(char *)(base + 0x10000);
#else
	char *reg_ptr=(char *)(base + 0x20000);
#endif

	memset(reg_ptr,0x0,0x88);
	memset(reg_ptr+0x400,0x0,0x24);
	// rest mcp100
	*(unsigned int *)(reg_ptr+0x40c)=0x08000000;
	while( (*(volatile unsigned int *)(reg_ptr+0x40c)) & 0x08000000 ){
		if ( ++ sync > 0x100000) {
			err ("MCP100 Reset Fail 0x%08x", *(volatile unsigned int *)(reg_ptr+0x40c));
			return;
		}
	}

	// start internal CPU
#if (defined(TWO_P_EXT) && !defined(SEQ))
	// stop internal CPU first
	*(volatile unsigned long*)(INCTL + base)=1<<17;
	// move embedded CPU ROM code to local mem
	#ifdef ENABLE_ROTATION
        if (mcp100_rotation_enable)
            memcpy ((void *)base, InterCPUcodeRT, sizeof(InterCPUcodeRT));
        else
            memcpy ((void *)base, InterCPUcode, sizeof(InterCPUcode));
    #else
    	memcpy ((void *)base, InterCPUcode, sizeof(InterCPUcode));
    #endif
	// why INTEN_CPU_RESET and INTEN_CPU_START are set simultaneously?
	// Because if you just set the INTEN_CPU_RESET bit, the interanl CPU clock
	// are not able to be activated at all and whole internal CPU can not work
	// without internal clock and interanl CPU reset will fail.
	// Only when the INTEN_CPU_START bit is set can the internal CPU clock be activated.
	// So you must set the INTEN_CPU_RESET and INTEN_CPU_START simultaneously.
	*(volatile unsigned long*)(INCTL + base)=3<<18;
	// After the internal CPU starts to reset, it enters the so-called reset state. But if 
	// the extrenal CPU was run so fast that the following statement 
	// 'INTEN_CPU_START;' was executed again within the reset state,
	// the internal CPU will not be activated at all. So you must query the internal CPU 
	// idle status to make sure the reset state of internal CPU was done.
    sync = 0;
	do {
		reg=*(volatile unsigned long*)(INCTL + base);
        #if 1
        if (++sync > 0x100000) {
            printk("set CPU code error\n");
            return;
        }
        #endif
	} while(!(reg&(1<<23))) 	;
	*(volatile unsigned long*)(INCTL + base)=1<<18;
#endif
}

static struct mcp100_rso * probe(struct platform_device * pdev)
{
	struct mcp100_rso * devrso;

	devrso = kmalloc(sizeof(struct mcp100_rso), GFP_KERNEL);
	if (devrso == NULL)
		return NULL;
	memset (devrso, 0, sizeof (struct mcp100_rso));
	devrso->irq_no = -1;
	devrso->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(devrso->res == NULL)) {
		err("fail: can't get resource");
		goto probe_err0;
	}
	if (unlikely((devrso->irq_no = platform_get_irq(pdev, 0)) < 0)) {
		err("fail: can't get irq");
		goto probe_err1;
	}
	devrso->va_base = (unsigned int) ioremap_nocache(devrso->res->start,
					devrso->res->end - devrso->res->start + 1);
	if (unlikely(devrso->va_base == 0)) {
		err("fail: counld not do ioremap");
		goto probe_err1;
	}
    //printk("MCP100 Base address: 0x%x\n", devrso->res->start);
#ifdef DMAWRP
	devrso->res_wrp = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (unlikely(devrso->res_wrp == NULL)) {
		err ("fail: can't get resource for wrp");
		goto probe_err1;
	}
	devrso->va_base_wrp = (unsigned int) ioremap_nocache(devrso->res_wrp->start,
			devrso->res_wrp->end - devrso->res_wrp->start + 1);
	if (unlikely(devrso->va_base_wrp == 0)) {
		err ("fail: counld not do ioremap for wrapper");
		goto probe_err1;
	}
#endif

#ifdef TWO_P_EXT
	if (request_irq(devrso->irq_no, mcp100_int_hd, 0,
				"MCP100 IRQ", &devrso->va_base) != 0) { 
		err ("Error to allocate enc irq handler");
		goto probe_err1;
	}
#endif
	fmcp100_resetrun (devrso->va_base);
	return devrso;

probe_err1:
	if (devrso->irq_no >= 0)
		free_irq(devrso->irq_no, &devrso->va_base);
	if (devrso->va_base)
		iounmap((void __iomem *)devrso->va_base);
	if (devrso->va_base_wrp)
		iounmap((void __iomem *)devrso->va_base_wrp);
probe_err0:
	kfree (devrso);
	return NULL;

}

static void remove(struct mcp100_rso * devrso)
{
#if defined(TWO_P_EXT) && !defined(SEQ)
	// stop internal CPU
	*(volatile unsigned long*)(INCTL + devrso->va_base) = 1 << 17;
#endif

	free_irq(devrso->irq_no, &devrso->va_base);
	iounmap((void __iomem *)devrso->va_base);
	if (devrso->va_base_wrp)
		iounmap((void __iomem *)devrso->va_base_wrp);
	kfree (devrso);
}

#define MCP100W_FTMCP100_PA_BASE    0x92400000
#define MCP100W_FTMCP100_PA_SIZE    0x00001000

static struct resource ftmcp100_resource[] = {
	[0] = {
		.start = MCP_FTMCP100_PA_BASE,
		.end = MCP_FTMCP100_PA_BASE + MCP_FTMCP100_PA_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = MCP_FTMCP100_0_IRQ,
		.end = MCP_FTMCP100_0_IRQ,
		.flags = IORESOURCE_IRQ,
	},
	#ifdef DMAWRP
		[2] = {
			.start = MCP100W_FTMCP100_PA_BASE,
			.end = MCP100W_FTMCP100_PA_BASE + MCP100W_FTMCP100_PA_SIZE - 1,
			.flags = IORESOURCE_MEM,
		},
	#endif
};
static void dev_release(struct device *dev)
{
	return;
}

static struct platform_device ftmcp100_device = {
	.name = "ftmcp100",
	.id = -1,
	.num_resources = ARRAY_SIZE(ftmcp100_resource),
	.resource = ftmcp100_resource,
	.dev  = {
		.release = dev_release,
	},
};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
static int ftmcp100_probe(struct platform_device * pdev)
{
	struct mcp100_rso * devrso;
	devrso = probe (pdev);
	if (NULL == devrso)
		return -1;
	platform_set_drvdata(pdev, devrso);
	mcp100_dev = devrso;
	return 0;
}
static int __devexit ftmcp100_remove(struct platform_device *pdev)
{
	if (mcp100_dev && platform_get_drvdata(pdev) == mcp100_dev) {
		remove (mcp100_dev);
		platform_set_drvdata(pdev, NULL);
		mcp100_dev = NULL;
	}
	return 0;
}

static struct platform_driver ftmcp100_driver = {
	.probe = ftmcp100_probe,
	.remove = __devexit_p(ftmcp100_remove),
	.driver = {
		.owner = THIS_MODULE,
		.name = "ftmcp100",
	},
};
#else
static int ftmcp100_probe(struct device * dev)
{
	struct platform_device * pdev = to_platform_device(dev);
	struct mcp100_rso * devrso;
	devrso = probe (pdev);
	if (NULL == devrso)
		return -1;
	dev_set_drvdata(dev, devrso);
	mcp100_dev = devrso;
	return 0;
}
static int ftmcp100_remove(struct device *dev)
{
	struct mcp100_rso * devrso = (struct mcp100_rso *)dev_get_drvdata(dev);
	if (mcp100_dev && devrso == mcp100_dev) {
		remove (mcp100_dev);
		dev_set_drvdata(dev, NULL);
		mcp100_dev = NULL;
	}
	return 0;
}
static struct device_driver ftmcp100_driver = {
	.owner = THIS_MODULE,     
	.name = "ftmcp100",    
	.bus = &platform_bus_type,
	.probe = ftmcp100_probe,       
	.remove = ftmcp100_remove,         
};
#endif

int mcp100_msg(char *str)
{
    int len = 0;
    if (str) {
    #if defined(SEQ)
        len += sprintf(str+len, "MCP100 driver with Sequencer");
    #elif defined(TWO_P_EXT)
        len += sprintf(str+len, "MCP100 driver with CPU");
    #elif defined(ONE_P_EXT)
        len += sprintf(str+len, "MCP100 driver");
    #else
        "Error configuration"       // do not remove this string
    #endif

        len += sprintf(str+len, ", v%d.%d.%d", MCP100_VER_MAJOR, MCP100_VER_MINOR, MCP100_VER_MINOR2);
        if (MCP100_VER_BRANCH)
            len += sprintf(str+len, ".%d", MCP100_VER_BRANCH);

        len += sprintf(str+len, ", %s", PLATFORM_NAME);

    #ifdef SUPPORT_VG
        len += sprintf(str+len, " for VG");
    #else
        len += sprintf(str+len, ", max res. %d x %d", mcp100_max_width, mcp100_max_height);
    #endif
        len += sprintf(str+len, ", built @ %s %s\n", __DATE__, __TIME__);
        len += sprintf(str+len, "module parameter\n");
        len += sprintf(str+len, "====================   ======\n");
        #ifdef ENABLE_SWITCH_CLOCK
        len += sprintf(str+len, "mcp100_switch_clock    %d\n", mcp100_switch_clock);
        #endif
        #ifdef ENABLE_ROTATION
        len += sprintf(str+len, "mcp100_rotation_enable %d\n", mcp100_rotation_enable);
        #endif
        len += sprintf(str+len, "mp4_dec_enable         %d\n", mp4_dec_enable);
        len += sprintf(str+len, "mp4_tight_buf          %d\n", mp4_tight_buf);
        len += sprintf(str+len, "mp4_overspec_handle    %d\n", mp4_overspec_handle);
        len += sprintf(str+len, "config_path            \"%s\"\n", config_path);        
        #ifdef SUPPORT_PWM
        len += sprintf(str+len, "pwm                    0x%04x\n", pwm&0xFFFF);
        #endif
    }
    else {
	/*
    #if defined(SEQ)
        printk("MCP100 driver with Sequencer");
    #elif defined(TWO_P_EXT)
        printk("MCP100 driver with CPU");
    #elif defined(ONE_P_EXT)
        printk("MCP100 driver");
    #else
        "Error configuration"       // do not remove this string
    #endif
	*/
        printk("MCP100 %s", PLATFORM_NAME);
        printk(", built @ %s %s\n", __DATE__, __TIME__);
    }
    return len;
}

#ifdef USE_KTHREAD
void clean_kthread_task(void)
{
    if (mcp100_cb_task) {
        kthread_stop(mcp100_cb_task);
        while (mcp100_cb_thread_ready) {
            msleep(10);
        }
    }
#ifdef USE_KTHREAD2
    if (mcp100_getjob_task) {
        kthread_stop(mcp100_getjob_task);
        while (mcp100_getjob_thread_ready) {
            msleep(10);
        }
    }
#endif
}
#endif

static int __init init_fmcp(void)
{
	int ret, i, j;
	if (pf_codec_on() < 0)
		return -EFAULT;
	last_type = TYPE_NONE;

    for (i = 0; i < FMCP_MAX_TYPE; i++) {
        mcp100_chn_list[i] = NULL;
        mcp100dev[i] = NULL;
    }
    //mcp100_msg();	

#ifdef SUPPORT_VG
    #ifdef USE_SPINLOCK
    	spin_lock_init(&mcp100_queue_lock);
	#else
        #if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
            sema_init(&mcp100_mutex, 1);
        #else
            init_MUTEX(&mcp100_mutex);
        #endif
    #endif
    #ifdef HW_LOCK_ENABLE
        spin_lock_init(&mcp100_hw_lock);
    #endif
#else
    // UNDO

	#if defined(TWO_P_EXT)
		init_waitqueue_head(&mcp100_codec_queue);
	#endif

    #if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
        sema_init(&fmutex, 1);
    #else
        init_MUTEX(&fmutex);
    #endif
    if (allocate_buffer(&mcp100_bs_buffer, mcp100_max_width*mcp100_max_height*3/2) < 0) {
        printk("allocate bs buffer error\n");
        goto init_fmcp_err0;
    }
#endif

	ret = platform_device_register(&ftmcp100_device);
	if (unlikely(ret < 0)) {
		printk("failed to register ftmcp100 device\n");
		goto init_fmcp_err1;
	}
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
		ret = platform_driver_register(&ftmcp100_driver);
	#else
		ret = driver_register(&ftmcp100_driver);
	#endif
	if (unlikely(ret < 0)) {
		printk("failed to register ftmcp100 driver\n");
		goto init_fmcp_err2;
	}

    mcp100_engine_head = kzalloc(sizeof(struct list_head) * MAX_ENGINE_NUM, GFP_KERNEL);
    mcp100_chn_list[TYPE_MPEG4_ENCODER] = kzalloc(sizeof(struct list_head)*MAX_ENGINE_NUM*MP4E_MAX_CHANNEL,GFP_KERNEL);
	mcp100_chn_list[TYPE_MPEG4_DECODER] = kzalloc(sizeof(struct list_head)*MAX_ENGINE_NUM*MP4D_MAX_CHANNEL,GFP_KERNEL);
    mcp100_chn_list[TYPE_JPEG_ENCODER] = kzalloc(sizeof(struct list_head)*MAX_ENGINE_NUM*MJE_MAX_CHANNEL,GFP_KERNEL);
    mcp100_chn_list[TYPE_JPEG_DECODER] = kzalloc(sizeof(struct list_head)*MAX_ENGINE_NUM*MJD_MAX_CHANNEL,GFP_KERNEL);
#ifdef ENABLE_ROTATION
    mcp100_chn_list[TYPE_MCP100_ROTATION] = kzalloc(sizeof(struct list_head)*RT_MAX_CHANNEL,GFP_KERNEL);
#endif
    if (mcp100_engine_head == NULL)
        goto init_fmcp_err3;
#ifdef ENABLE_ROTATION
    if (NULL == mcp100_chn_list[TYPE_MPEG4_ENCODER] || NULL == mcp100_chn_list[TYPE_MPEG4_DECODER] || 
        NULL == mcp100_chn_list[TYPE_JPEG_DECODER] || NULL == mcp100_chn_list[TYPE_JPEG_ENCODER] || 
        NULL == mcp100_chn_list[TYPE_MCP100_ROTATION]) {
        printk("failed to allocate channel list\n");
        goto init_fmcp_err3;
    }
#else
    if (NULL == mcp100_chn_list[TYPE_MPEG4_ENCODER] || NULL == mcp100_chn_list[TYPE_MPEG4_DECODER] || 
        NULL == mcp100_chn_list[TYPE_JPEG_DECODER] || NULL == mcp100_chn_list[TYPE_JPEG_ENCODER]) {
        printk("failed to allocate channel list\n");
        goto init_fmcp_err3;
    }
#endif

    for (i = 0; i < MAX_ENGINE_NUM; i++)
        INIT_LIST_HEAD(&mcp100_engine_head[i]);
#ifdef ENABLE_ROTATION
    for (i = 0; i < 5; i++) {
        for (j = 0; j < MAX_ENGINE_NUM * max_channel_number[i]; j++)
            INIT_LIST_HEAD(&mcp100_chn_list[i][j]);
    }
#else
    for (i = 0; i < 4; i++) {
        for (j = 0; j < MAX_ENGINE_NUM * max_channel_number[i]; j++)
            INIT_LIST_HEAD(&mcp100_chn_list[i][j]);
    }
#endif
#ifdef USE_KTHREAD
    init_waitqueue_head(&mcp100_cb_waitqueue);
    mcp100_cb_task = kthread_create(mcp100_cb_thread, NULL, "mcp100_cb_thread");
    if (!mcp100_cb_task) {
        printk("create cb task fail\n");
        goto init_fmcp_err3;
    }
    wake_up_process(mcp100_cb_task);
#else
    mcp100_cb_workq = create_workqueue("mcp100_cb");
    if (!mcp100_cb_workq) {
        printk("callback work queue init error\n");
        goto init_fmcp_err3;
    }        
    INIT_DELAYED_WORK(&process_callback_work, 0);
#endif
#ifdef USE_KTHREAD2
    init_waitqueue_head(&mcp100_getjob_waitqueue);
    mcp100_getjob_task = kthread_create(mcp100_getjob_thread, NULL, "mcp100_getjob_thread");
    if (!mcp100_getjob_task) {
        printk("create getjob task fail\n");
        goto init_fmcp_err3;
    }
    wake_up_process(mcp100_getjob_task);
#else
    mcp100_getjob_workq = create_workqueue("mcp100_getjob");
    if (!mcp100_getjob_workq) {
        printk("trigger work queue error\n");
        goto init_fmcp_err3;
    }
    INIT_DELAYED_WORK(&process_getjob_work, 0);
#endif
    mcp100_proc_init();

#ifdef ENABLE_SWITCH_CLOCK
    if (mcp100_switch_clock)
        pf_clock_switch_off();
#endif
    
#ifdef ENABLE_CHECKSUM
    check_sum_init();
#endif

    mcp100_msg(NULL);

	return 0;

init_fmcp_err3:
    if (mcp100_engine_head)
        kfree(mcp100_engine_head);
    for (i = 0; i < FMCP_MAX_TYPE; i++) {
        if (mcp100_chn_list[i])
            kfree(mcp100_chn_list[i]);
    }
#ifdef USE_KTHREAD
    clean_kthread_task();
#else
    if (mcp100_cb_workq)
        destroy_workqueue(mcp100_cb_workq);
    if (mcp100_getjob_workq)
        destroy_workqueue(mcp100_getjob_workq);
#endif

init_fmcp_err2:
	platform_device_unregister(&ftmcp100_device);

init_fmcp_err1:
#ifndef SUPPORT_VG
    free_buffer(&mcp100_bs_buffer);
	//if (mcp100_bs_virt)
	//	fcnsist_free(mcp100_bs_length, (void*)mcp100_bs_virt, (void *)mcp100_bs_phy);
#endif
    mcp100_proc_close();

#ifndef SUPPORT_VG
init_fmcp_err0:
#endif
	pf_codec_off();
    pf_codec_exit();
	return -EFAULT;
}

static void __exit cleanup_fmcp(void)
{
    int i;
#if !defined(SUPPORT_VG)
    free_buffer(&mcp100_bs_buffer);
	//if (mcp100_bs_virt)
	//	fcnsist_free (mcp100_bs_length, (void*)mcp100_bs_virt, (void *)mcp100_bs_phy);
#elif defined(TWO_P_EXT)
	//mcp100_proc_exit ();
	//kfifo_free(mcp100_queue);
#endif
	pf_codec_off();
    pf_codec_exit();

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
	platform_driver_unregister(&ftmcp100_driver);
#else
	driver_unregister(&ftmcp100_driver);
#endif
	platform_device_unregister(&ftmcp100_device);

    if (mcp100_engine_head)
        kfree(mcp100_engine_head);
    for (i = 0; i < FMCP_MAX_TYPE; i++) {
        if (mcp100_chn_list[i])
            kfree(mcp100_chn_list[i]);
    }
#ifdef USE_KTHREAD
    clean_kthread_task();
#else
    if (mcp100_cb_workq)
        destroy_workqueue(mcp100_cb_workq);
    if (mcp100_getjob_workq)
        destroy_workqueue(mcp100_getjob_workq);
#endif
    mcp100_proc_close();
#ifdef ENABLE_CHECKSUM
    check_sum_exit();
#endif

    //exit_proc();
}

#ifdef SUPPORT_VG
int dump_job_info(char *str, int chn, int codec_type)
{
    struct video_entity_job_t *job;
    struct job_item_t *job_item;
    //unsigned long flags;
    char *st_string[]={"STANDBY","ONGOING"," FINISH","   FAIL"};
    int idx, engine = 0;
    int len = 0;

    if (codec_type < 0)
        return 0;

    if (codec_type <= TYPE_NONE || codec_type >= TYPE_MAX) {
        //printk("codec type error\n");
        len += sprintf(str+len, "codec type(%d) out of range\n", codec_type);
        return len;
    }
    //char *type_string[]={"   ","(M)","(*)"}; //type==0,type==1 && need_callback=0, type==1 && need_callback=1
    idx = MAKE_IDX(max_channel_number[codec_type], engine, chn);
    
    
    //spin_lock_irqsave(&mcp100_queue_lock,flags);
    down(&mcp100_mutex);
    if (chn==999) {
        len += sprintf(str+len, "Codec: %s\n", codec_name[codec_type]);
        len += sprintf(str+len, "Codec    Minor  Job_ID   Status   Puttime    start   end\n");
        len += sprintf(str+len, "===========================================================\n");
        list_for_each_entry(job_item, &mcp100_engine_head[engine], engine_list) {
            job = (struct video_entity_job_t *)job_item->job;
            len += sprintf(str+len, "%6s   %02d       %04d   %s   0x%04x   0x%04x  0x%04x\n",
                codec_name[job_item->codec_type], job_item->chn, job->id, 
                st_string[job_item->status-1], job_item->puttime&0xffff,
                job_item->starttime&0xffff,(int)job_item->finishtime&0xffff);
        }
    } 
    else {
        if (list_empty(&mcp100_chn_list[codec_type][chn])==0) {
            len += sprintf(str+len, "Codec: %s, channel %d\n", codec_name[codec_type], chn);
            len += sprintf(str+len, "Engine Minor  Job_ID   Status   Puttime    start   end\n");
            len += sprintf(str+len, "===========================================================\n");

            list_for_each_entry(job_item, &mcp100_chn_list[codec_type][idx], minor_list) {
                job=(struct video_entity_job_t *)job_item->job;
                len += sprintf(str+len, "%d      %02d       %04d   %s   0x%04x   0x%04x  0x%04x\n",
                    job_item->engine, job_item->chn, job->id,
                    st_string[job_item->status-1],job_item->puttime&0xffff,
                    job_item->starttime&0xffff,(int)job_item->finishtime&0xffff);
            }
        }
    }
    up(&mcp100_mutex);
    //spin_unlock_irqrestore(&mcp100_queue_lock,flags);
    return len;
}

#endif

int print_info(char *str)
{
    int len = 0;
    if (str) {
        len += sprintf(str+len, "id      codec\n");
        len += sprintf(str+len, "==  =============\n");
        len += sprintf(str+len, "%d  MPEG4 Encoder\n", TYPE_MPEG4_ENCODER);
        len += sprintf(str+len, "%d  MPEG4 Decoder\n", TYPE_MPEG4_DECODER);
        len += sprintf(str+len, "%d  JPEG Encoder\n", TYPE_JPEG_ENCODER);
        len += sprintf(str+len, "%d  JPEG Decoder\n", TYPE_JPEG_DECODER);
    }
    else {
        printk("id     codec\n");
        printk("==================\n");
        printk("%d   MPEG4 Encoder\n", TYPE_MPEG4_ENCODER);
        printk("%d   MPEG4 Decoder\n", TYPE_MPEG4_DECODER);
        printk("%d   JPEG Encoder\n", TYPE_JPEG_ENCODER);
        printk("%d   JPEG Decoder\n", TYPE_JPEG_DECODER);
    }
    return len;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
EXPORT_SYMBOL(fkmalloc);
EXPORT_SYMBOL(fkfree);
//EXPORT_SYMBOL(fcnsist_alloc);
//EXPORT_SYMBOL(fcnsist_free);
EXPORT_SYMBOL(fconsistent_alloc);
EXPORT_SYMBOL(fconsistent_free);
//EXPORT_SYMBOL(user_va_to_pa);
//EXPORT_SYMBOL(check_continued);
EXPORT_SYMBOL(allocate_buffer);
EXPORT_SYMBOL(allocate_buffer_from_ddr);
EXPORT_SYMBOL(free_buffer);
EXPORT_SYMBOL(parse_param_cfg);
EXPORT_SYMBOL(fmcp100_resetrun);
EXPORT_SYMBOL(last_type);
EXPORT_SYMBOL(mcp100_dev);
#ifndef SUPPORT_VG
	EXPORT_SYMBOL(fmutex);
	EXPORT_SYMBOL(mcp100_bs_buffer);
	#ifdef TWO_P_EXT
		EXPORT_SYMBOL(mcp100_codec_queue);
	#endif
    // New VG 
    //EXPORT_SYMBOL(getChannelIdx);
#else
	//EXPORT_SYMBOL(dump_n_frame);
	//EXPORT_SYMBOL(mcp100_proc_dump);
	EXPORT_SYMBOL(mcp_putjob);
    EXPORT_SYMBOL(mcp_stopjob);
    EXPORT_SYMBOL(dump_job_info);
    //EXPORT_SYMBOL(mcp100dev);
#endif
#ifdef REF_POOL_MANAGEMENT
    EXPORT_SYMBOL(init_mem_pool);
    EXPORT_SYMBOL(clean_mem_pool);
    EXPORT_SYMBOL(register_ref_pool);
    EXPORT_SYMBOL(deregister_ref_pool);
    EXPORT_SYMBOL(allocate_pool_buffer2);
    EXPORT_SYMBOL(release_pool_buffer2);
    EXPORT_SYMBOL(enc_dump_pool_num);
    EXPORT_SYMBOL(enc_dump_ref_pool);
    EXPORT_SYMBOL(enc_dump_chn_pool);
    EXPORT_SYMBOL(dec_dump_ref_pool);
    EXPORT_SYMBOL(dec_dump_chn_pool);
    EXPORT_SYMBOL(dec_dump_pool_num);
#endif
//#ifdef MPEG4_COMMON_PRED_BUFFER
    EXPORT_SYMBOL(mp4_dec_enable);
    EXPORT_SYMBOL(mp4_tight_buf);
//#endif
EXPORT_SYMBOL(calculate_checksum);
EXPORT_SYMBOL(config_path);
EXPORT_SYMBOL(mcp100_switch);
EXPORT_SYMBOL(mcp100_register);
EXPORT_SYMBOL(mcp100_deregister);
//EXPORT_SYMBOL(trigger_callback_finish);
//EXPORT_SYMBOL(trigger_callback_fail);
EXPORT_SYMBOL(mcp100_chn_list);
#endif

module_init(init_fmcp);
module_exit(cleanup_fmcp);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("FTMCP100 common driver");
MODULE_LICENSE("GPL");
