#ifndef __THINK2D_DRIVER_H__
#define __THINK2D_DRIVER_H__


#include <linux/slab.h>

#include "think2d/think2d_if.h"
#include "think2d_platform.h"
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28))
#include "frammap_if.h"            
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
#include "../frammap/frammap.h"            
#endif


#define THINK2D_DEV_NAME "think2d"

#define LEVEL_TRIGGERED 1

#define PSEUDO_HARDWARE 0

#define DEBUG_PROC_ENTRY_ON 0
#if DEBUG_PROC_ENTRY_ON == 1
#define DEBUG_PROC_ENTRY do { printk(KERN_INFO "[%s]\n", __FUNCTION__); } while (0)
#else
#define DEBUG_PROC_ENTRY
#endif

#define PRINT_MSG 1
#if PRINT_MSG == 1
#define PRINT_I(args...) printk(KERN_INFO    args)
#define PRINT_E(args...) printk(KERN_ERR     args)
#define PRINT_W(args...) printk(KERN_WARNING args)
#else
#define PRINT_I(args...) 
#define PRINT_E(args...) 
#define PRINT_W(args...) 
#endif

/* frequently used MACRO */
#define DRV_COUNT_RESET() 			    (p_dev_data->driver_count = 0)
#define DRV_COUNT_INC() 			    (p_dev_data->driver_count++) 
#define DRV_COUNT_DEC() 			    (p_dev_data->driver_count--) 
#define DRV_COUNT_GET() 			    (p_dev_data->driver_count) 




#if 0
#define think_readl(base, offset) ioread32((base) + (offset))
#define think_writel(base, offset, val) do { iowrite32(val, base+offset); }  while(0)//nish_test
#else
#define think_readl(offset) ioread32(p_dev_data->vbase + (offset))
#define think_writel(offset, val) do { iowrite32(val, p_dev_data->vbase+offset); }  while(0)//nish_test
#endif

#define DECLARE_DRV_DATA struct think2d_drv_data* p_drv_data = filp->private_data
#define DECLARE_DEV_DATA struct think2d_dev_data* p_dev_data = p_drv_data->p_dev_data
#define DECLARE_SHA_DATA struct think2d_shared* pshared = p_dev_data->p_shared

		
/* think2d_dev_data*/
struct think2d_dev_data {
    /* hardware resource */
    int id;
    uint32_t vbase;
    uint32_t pbase;
    uint32_t mem_len;
    int irq_no;
    struct resource *res;	

    /* scu data */
    struct think2d_scu_t think2d_scu;


    unsigned int driver_count;
   
    struct frammap_buf_info shared_buf_info;
    struct think2d_shared   *p_shared;

};

/* think2d_drv_data*/
struct think2d_drv_data {
    struct think2d_dev_data *p_dev_data; 
};
#endif
