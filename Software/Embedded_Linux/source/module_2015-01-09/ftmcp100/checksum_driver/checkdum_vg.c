#include <linux/module.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/miscdevice.h>
#include <linux/kallsyms.h>
#include <linux/workqueue.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/string.h>

#include "log.h"
#include "video_entity.h"
#include "../ioctl_cs.h"
#include "../fmcp.h"
//#include "../fmpeg4_driver/common/share_mem.h"
//#include "checksum_ex.h"
#include "checksum_node.h"
#include "../platform.h"

#define PROTECT_DOUBLE_CS

spinlock_t mcp100_cs_lock;
wait_queue_head_t mcp100_cs_wait_queue;
#ifdef PROTECT_DOUBLE_CS
    struct semaphore mcp100_cs_mutex;
#endif
#ifdef HOST_MODE
    static struct workqueue_struct *mcp100_cs_wq = NULL;
    static DECLARE_DELAYED_WORK(process_cs_work, 0);
    extern void mcp_isr (int irq, unsigned int * base);
#endif


extern struct mcp100_rso * mcp100_dev;
extern unsigned int cs_enable;
extern unsigned int cs_wait;
extern unsigned int cs_result;
#ifdef ENABLE_SWITCH_CLOCK
    extern unsigned int mcp100_switch_clock;
#endif
#ifdef ENABLE_ROTATION
    extern unsigned int mcp100_rotation_enable;
#endif

extern int cs_tri(FMCP_CS_PARAM * posdin);
#ifdef HW_LOCK_ENABLE
    extern int set_hw_state(int codec_type);
#else
    extern int is_hardware_idle(void);
    extern int set_running_codec(int codec_type);
#endif

#ifdef HOST_MODE
void tri_next_job(void)
{
    mcp_isr(0, (unsigned int *)&mcp100_dev->va_base);
}
#endif

/*
int calculate_checksum(unsigned char *buf_pa, unsigned int len, int type, int (*callback_fun)(int))
{
    // put job
    FMCP_CS_PARAM cs_param;
    //cs_param.id = id;
    cs_param.buffer_pa = (unsigned int)buf_pa;
    cs_param.buffer_len = len;
    cs_param.type = type;
    mcp_putjob((struct video_entity_job_t*)(&cs_param), TYPE_CHECK_SUM);
    return 0;
}
*/
/*
int fcheck_sum_trigger(struct job_item_t *job_item)
{
    FMCP_CS_PARAM *cs_ptr = &job_item->cs_data;

    if (cs_tri(cs_ptr) < 0)
        return -1;
    return 0;
}
*/

int calculate_checksum(unsigned int buf_pa, unsigned int len, int type)
{
    FMCP_CS_PARAM cs_param;
    unsigned long flags;
    unsigned int cs_value;
#ifdef ENABLE_ROTATION
    if (mcp100_rotation_enable) {
        cs_value = 0;
        goto exit_calculate_checksum;
    }
#endif
    //cs_param.id = id;
    cs_param.buffer_pa = buf_pa;
    cs_param.buffer_len = len;
    cs_param.type = type;
    cs_param.ip_base = mcp100_dev->va_base;

#ifdef PROTECT_DOUBLE_CS
    down(&mcp100_cs_mutex);
#endif
// spinlock
    spin_lock_irqsave(&mcp100_cs_lock, flags); 
#ifdef HW_LOCK_ENABLE
    if (set_hw_state(TYPE_CHECK_SUM) == TYPE_CHECK_SUM) {
        spin_unlock_irqrestore(&mcp100_cs_lock, flags);
    }
#else
    if (is_hardware_idle()) {
        set_running_codec(TYPE_CHECK_SUM);
        spin_unlock_irqrestore(&mcp100_cs_lock, flags);
    }
#endif
    else {
        // wait current job
        cs_wait = 1;
        spin_unlock_irqrestore(&mcp100_cs_lock, flags);
        wait_event_timeout(mcp100_cs_wait_queue, (0 == cs_wait), msecs_to_jiffies(300));
        if (0 == cs_wait) {
           // set last_type @ isr
        #ifndef HW_LOCK_ENABLE
            spin_lock_irqsave(&mcp100_cs_lock, flags); 
            set_running_codec(TYPE_CHECK_SUM);
            spin_unlock_irqrestore(&mcp100_cs_lock, flags);
        #endif
        }
        else {
            mcp100_err("check sum wait too long\n");
            cs_value = 0;
            goto exit_calculate_checksum;
        }
    }
#ifdef ENABLE_SWITCH_CLOCK
    if (mcp100_switch_clock) {
        pf_clock_switch_on();
    }
#endif
    spin_lock_irqsave(&mcp100_cs_lock, flags); 
    cs_enable = 1;
    spin_unlock_irqrestore(&mcp100_cs_lock, flags);
    cs_tri(&cs_param);
    wait_event_timeout(mcp100_cs_wait_queue, (0 == cs_enable), msecs_to_jiffies(300));
    // consider timeout ......
#ifdef ENABLE_SWITCH_CLOCK	// Tuba 20141201
    if (mcp100_switch_clock) {
        pf_clock_switch_off();
    }
#endif    
    cs_value = cs_result;
#ifdef PROTECT_DOUBLE_CS
    up(&mcp100_cs_mutex);
#endif
    
#ifdef HOST_MODE
    // trigger isr
    PREPARE_DELAYED_WORK(&process_cs_work, (void *)tri_next_job);
    queue_delayed_work(mcp100_cs_wq, &process_cs_work, 0);
#endif
exit_calculate_checksum:
    return cs_value;
}
/*
int fcheck_sum_process_done(struct job_item_t *job_item)
{
    //volatile Share_Node_CS *node = (volatile Share_Node *) (0x8000 + base);
    //job_item->cs_data.result = cs_result;
    
    return 0;
}
*/

void fcheck_sum_isr(int irq, void *dev_id, unsigned int base)
{
    MP4_COMMAND type;
    volatile Share_Node_CS *node = (volatile Share_Node_CS *) (0x8000 + base);
    type = (node->command & 0xF0)>>4;
    if(type == CHECKSUM) {
        *(volatile unsigned int *)(base + 0x20098)=0x80000000;  // clear interrupt
        cs_result = node->cshw.result;
    }
    cs_enable = 0;
}

int check_sum_init(void)
{
    init_waitqueue_head(&mcp100_cs_wait_queue);
    spin_lock_init(&mcp100_cs_lock);
#ifdef PROTECT_DOUBLE_CS
    sema_init(&mcp100_cs_mutex, 1);
#endif
#ifdef HOST_MODE
    mcp100_cs_wq = create_workqueue("mcp100_cs");
    INIT_DELAYED_WORK(&process_cs_work, 0);
#endif
    return 0;
}

int check_sum_exit(void)
{
#ifdef HOST_MODE
    destroy_workqueue(mcp100_cs_wq);
#endif
    return 0;
}
/*
static mcp100_rdev mcsdv = {
	.job_tri = NULL,
	.job_done = fcheck_sum_process_done,
	.codec_type = TYPE_CHECK_SUM,
	.handler = fcheck_sum_isr,
	.dev_id = NULL,
	.switch22 = NULL,
	.parm_info = NULL,
	.parm_info_frame = NULL,
};

int check_sum_init(void)
{
    if (mcp100_register(&mcsdv, TYPE_CHECK_SUM) < 0)
        return -1;
    return 0;
}

void check_sum_exit(void)
{
    mcp100_deregister(TYPE_CHECK_SUM);
}
*/
