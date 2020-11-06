#ifndef _FT_ASYNC_H
#define _FT_ASYNC_H

#include <linux/kernel.h>               /* printk(), min() */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/fs.h>                   /* file_operations */
#include <linux/slab.h>                 /* kzalloc */
#include <linux/interrupt.h>            /* free_irq */
#include <linux/types.h>
#include <linux/kfifo.h>				/* kfifo */
#include <asm/signal.h>                 /* SIGIO */
#include <asm/siginfo.h>                /* POLL_OUT */
#include <linux/sched.h> 				/* for wake_up() */
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <asm/uaccess.h>				/* copy_from_user */
#include <linux/poll.h>					/* poll_wait */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,00))
#include <linux/platform_device.h>
#include <mach/platform/platform_io.h>
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28))
#include <linux/platform_device.h>
#include <mach/platform/platform_io.h>
#include <mach/platform/spec.h>
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
#include <linux/device.h>
#include <asm/arch/fmem.h>
#include <asm/arch/platform/spec.h>
#endif

#define HARDWARE_IS_ON

#define DEV_COUNT 1
#define DEV_NAME "ks_dev"
#define CLS_NAME "ks_cls"

#ifdef __KERNEL__
#define PRINT_E(args...) do { printk(args); }while(0)
#define PRINT_I(args...) do { printk(args); }while(0)
#define PRINT_FUNC()     PRINT_I(" * %s\n", __FUNCTION__)
#else
#define PRINT_E(args...) do { printf(args); }while(0)
#define PRINT_I(args...) do { printf(args); }while(0)
#define PRINT_FUNC()     ();
#endif

#define DRV_COUNT_RESET() 			    (p_dev_data->driver_count = 0)
#define DRV_COUNT_INC() 			    (p_dev_data->driver_count++) 
#define DRV_COUNT_DEC() 			    (p_dev_data->driver_count--) 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
#define init_MUTEX(sema) sema_init(sema, 1)
#endif

struct scu_t {
    int fd;
    int (*register_scu)(int* scu_fd);
    int (*deregister_scu)(int* scu_fd);
    //int (*enable_scu)(int scu_fd);
    //int (*select_scu)(int scu_fd, int half);
    //int (*disable_scu)(int scu_fd);
    //int (*reset_scu)(int scu_fd);
};

struct flp_data {	
    /* generic attributes */
	struct dev_data* p_dev_data;
    unsigned int buffer_size;
    void* buffer_vadr;
    void* buffer_padr;

};

/* dev specific define */
struct dev_specific_data_t {
    unsigned long   scan_duration;
    unsigned long   repeat_duration;
    int             last_key_sts;
    unsigned long   last_key_jiffies;
    unsigned long   last_scan_jiffies;
    
	u32 key_i_bits;
	u32 key_o_bits;
	
    spinlock_t      lock;
    struct kfifo    fifo;
    int             queue_len;
    wait_queue_head_t   wait_queue;
    struct semaphore    oper_sem;
    struct delayed_work work_st;
    /*
    struct workqueue_struct *workqueue;
    */

};

struct dev_data {
    
    /* generic attributes */
    struct cdev 				cdev;          /* chrdev */
    struct class*				class;
    struct device*				p_device;
    dev_t 						dev_num;
    
    int 						id;            /* platform_device id */   
    
    unsigned int 				io_size;       /* IP reg size, be set after platform_get_resource */    
    void* 						io_vadr;       /* IP io_vadr */        
    void* 						io_padr;       /* IP io_padr */    
    
    int 						irq_no;        /* IP irq number */    
    struct scu_t 				scu;           /* pmu */        
    unsigned int 				driver_count;  /* driver ref count */

    /* specific attributes */    
	struct dev_specific_data_t  dev_specific_data; 


};
#endif
