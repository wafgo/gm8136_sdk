/* A1 simulation */
#include <linux/module.h>
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/synclink.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <asm/arch/fmem.h>
#include <linux/dma-mapping.h>
#include <linux/workqueue.h>
#include <linux/kfifo.h>

#include "decode_vg.h"
#include "decode_entity.h"

//struct workqueue_struct *workq;
//static DECLARE_WORK(process_isr_work, 0, (void *)0);

enum engine_status{
    ENGINE_IDLE=0,
    ENGINE_BUSY=1,    
};


#define MAX_DEV     4
static int engine_busy[MAX_DEV];
void *engine_data[MAX_DEV];
struct decode_data_t engine_decode_data[MAX_DEV];

spinlock_t decode_lock;

void set_engine_busy(int engine)
{
    unsigned long flags;

    spin_lock_irqsave(&decode_lock,flags);
    engine_busy[engine] = ENGINE_BUSY;
    spin_unlock_irqrestore(&decode_lock,flags);
}

void set_engine_idle(int engine)
{
    unsigned long flags;

    spin_lock_irqsave(&decode_lock,flags);
    engine_busy[engine] = ENGINE_IDLE;
    spin_unlock_irqrestore(&decode_lock,flags);
}

int is_engine_idle(int engine){
    int ret = 0;
    
    if(ENGINE_BUSY == engine_busy[engine]){
        ret = 0;
    }
    else if(ENGINE_IDLE == engine_busy[engine]){
        ret = 1;
    }
    else{
        panic("[VE][ERROR]engine status is strange!\n");
    }
    
    return ret;
}
    

int is_engine_busy(int engine)
{
    int ret=0;

    if(ENGINE_BUSY == engine_busy[engine]){
        ret = 1;
    }
    else if(ENGINE_IDLE == engine_busy[engine]){
        ret = 0;
    }
    else{
        panic("[VE][ERROR]engine status is strange!\n");
    }
    
    return ret;
}


//1:sucessful(engine not busy,set it busy)
//0:engine is busy
int test_and_set_engine_busy(int engine)
{
    int ret=0;
   
    if(is_engine_idle(engine)) {
        set_engine_busy(engine);
        ret=1;
    }
    
    return ret;
}

static void *get_engine_data(int engine)
{    
    return engine_data[engine];
}

static void *get_engine_decode_data(int engine)
{    
    return &engine_decode_data[engine];
}

static int __init decode_init(void)
{
    int i;
    
    //create work queue    
    //workq = create_workqueue("decode");

    spin_lock_init(&decode_lock);
    
    //set engine idle
    for(i=0;i<MAX_DEV;i++)
        engine_busy[i]=0;   

    return driver_vg_init("decode",4,0);
}

static void __exit decode_clearnup(void)
{
    //destroy work queue
    //destroy_workqueue(workq);
    driver_vg_close();
}

module_init(decode_init);
module_exit(decode_clearnup);

MODULE_AUTHOR("Grain Media Corp.");
MODULE_LICENSE("GPL");



