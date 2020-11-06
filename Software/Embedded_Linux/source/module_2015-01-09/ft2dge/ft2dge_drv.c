/*
 * Revision History
 * VER2.4 2014/03/19 support OSG function 
 * VER2.5 2014/03/27 support ft2d_osg_remainder_fire function 
 * VER2.6 2014/03/28 add more debug message
 * VER2.7 2014/04/03 support hw osg util
 * VER2.7 2014/04/07 change hw osg util method
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <mach/fmem.h>
#include <mach/platform/platform_io.h>
#include "ft2dge.h"
#include <ft2dge/ft2dge_lib.h>
#ifdef FT2D_OSG_USING
#include <ft2dge/ft2dge_osg.h>
#endif
#define FT2DGE_MINOR   MISC_DYNAMIC_MINOR //dynamic
static DECLARE_WAIT_QUEUE_HEAD(ft2dge_wait);

static int ft2dge_proc_init(void);
static void ft2dge_proc_release(void);
extern void platform_init(void);
extern void platform_exit(void);

typedef struct {
    struct list_head    list;
    int irq;
    void *io_mem;   //virtual
    u32 io_paddr;   //physical
    struct page *page;
    int page_order;
    ft2dge_sharemem_t *sharemem;
    int open_cnt;
    spinlock_t  lock;
    unsigned int intr_cnt;
    unsigned int usr_blocking_cnt;  //how many user applications are blocking now
    unsigned int tricky_cnt;        //the tricky condition not in expectation
} ft2dge_drv_t;

typedef struct {
    struct list_head    list;
    wait_queue_head_t   wait;
    struct ft2dge_idx   *usr_idx;
    int                 event;
} wait_list_t;

/* main structure
 */
static ft2dge_drv_t drv_info;

#ifdef FT2D_OSG_USING

typedef struct {
    struct {
        unsigned int ofs;
        unsigned int val;
    } cmd[HW_LLST_MAX_CMD];
    int cmd_cnt;    //how many commands are active
    /* TODO semaphore */
} ft2d_osg_list_t;

typedef enum {
    FT2D_OSG_CPU_FA726 = 0,
    FT2D_OSG_CPU_FA626,
    FT2D_OSG_CPU_UNKNOWN,
} ft2d_cpu_id_t;

typedef struct {
    ft2d_osg_list_t    *cmd_list;
    ft2d_cpu_id_t cpu_id;
    unsigned int intr_cnt;
    unsigned int hw_running;
    unsigned int fire_cnt;  //how many user applications are blocking now  
} ft2dge_osgdrv_t;
/*
 * utilization structure
 */
typedef struct ft2dge_eng_util
{
	unsigned int utilization_period; //5sec calculation
	unsigned int utilization_period_record; 
	unsigned int utilization_start;
	unsigned int utilization_record;
	unsigned int engine_start;
	unsigned int engine_end;
	unsigned int engine_time;
    unsigned int engine_time_record;
}ft2dge_util_t;

ft2dge_util_t g_utilization;

#define ENGINE_START 	    g_utilization.engine_start
#define ENGINE_END 		    g_utilization.engine_end
#define ENGINE_TIME	        g_utilization.engine_time
#define ENGINE_TIME_RECORD 	g_utilization.engine_time_record

#define UTIL_PERIOD_RECORD  g_utilization.utilization_period_record
#define UTIL_PERIOD 	    g_utilization.utilization_period
#define UTIL_START		    g_utilization.utilization_start
#define UTIL_RECORD 	    g_utilization.utilization_record


static ft2dge_osgdrv_t osg_drv_info;
#define FT2D_OSG_CMD_IS_ENOUGH(m ,n) (((m + n) > HW_LLST_MAX_CMD) ? 0 : 1)
#define FT2D_GET_CPU_mode      (osg_drv_info.cpu_id)
static DECLARE_WAIT_QUEUE_HEAD(osgwait_idle);
static struct proc_dir_entry *osg_debug_info = NULL;
static struct proc_dir_entry *osg_debug_dump = NULL;
static struct proc_dir_entry *osg_util_info = NULL;

static int    g_osg_dump_flag = 0;  
#endif

/* proc init.
 */
static struct proc_dir_entry *ft2dge_proc_root = NULL;
static struct proc_dir_entry *debug_info = NULL;

/* 0 for data finished already, -1 for not yet
 */
static inline int data_complete_check(struct ft2dge_idx *usr_idx, ft2dge_sharemem_t *sharemem)
{
    int ret = -1;

    if (usr_idx->write_idx > usr_idx->read_idx) {
        /* Good, my jobs are finished already. */
        if (usr_idx->write_idx <= sharemem->read_idx) {
            ret = 0;
            goto exit;
        }

        /* The case sharemem->read_idx warps around */
        if (usr_idx->write_idx - sharemem->read_idx >= HW_LLST_MAX_CMD) {
            ret = 0;
            goto exit;
        }
    } else {
        /* the case usr_idx->write_idx < usr_idx->read_idx is wraped around.
         */
        if (sharemem->read_idx - usr_idx->write_idx < HW_LLST_MAX_CMD) {
            ret = 0;
            goto exit;
        }
    }

exit:
    return ret;
}

/* blocking until the owned jobs are done.
 */
void ft2dge_drv_sync(struct ft2dge_idx *usr_idx)
{
    unsigned long flags;
    wait_list_t node;
    ft2dge_sharemem_t *sharemem = drv_info.sharemem;

    /* sanity check */
    if (usr_idx->read_idx == usr_idx->write_idx)
        panic("%s, bug in GM library! \n", __func__);

    init_waitqueue_head(&node.wait);

    spin_lock_irqsave(&drv_info.lock, flags);
    if (!data_complete_check(usr_idx, sharemem)) {
        drv_info.tricky_cnt ++;
        spin_unlock_irqrestore(&drv_info.lock, flags);
        return;
    }

    /* used to check if the hardware lost interrupt */
    drv_info.usr_blocking_cnt ++;
    INIT_LIST_HEAD(&node.list);
    node.usr_idx = usr_idx;

    list_add_tail(&node.list, &drv_info.list);
    node.event = 0;
    /* unlock the interrupt */
    spin_unlock_irqrestore(&drv_info.lock, flags);

    wait_event(node.wait, node.event);

    /* debug only */
    if (data_complete_check(usr_idx, sharemem))
        panic("%s, bug happen! \n", __func__);

    drv_info.usr_blocking_cnt --;
}

static int ft2dge_drv_open(struct inode *inode, struct file *filp)
{
    if (inode || filp) {}
#ifdef FT2D_OSG_USING    
    if(FT2D_GET_CPU_mode == FT2D_OSG_CPU_FA626){
        printk("FT2D doesn't support GUI in FA626\n");
        return -1;
    }
#endif        
    drv_info.open_cnt ++;

    /* inc the module reference counter */
    try_module_get(THIS_MODULE);

    return 0;
}

static int ft2dge_drv_release(struct inode *inode, struct file *filp)
{
    if (inode || filp) {}

#ifdef FT2D_OSG_USING    
    if(FT2D_GET_CPU_mode == FT2D_OSG_CPU_FA626){
        printk("FT2D doesn't support GUI in FA626\n");
        return -1;
    }
#endif 
    
    drv_info.open_cnt --;

    module_put(THIS_MODULE);

    return 0;
}

static long ft2dge_drv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct ft2dge_idx usr_idx;
    long ret = 0;
    unsigned long  retVal;

    if (filp) {}

#ifdef FT2D_OSG_USING    
    if(FT2D_GET_CPU_mode == FT2D_OSG_CPU_FA626){
        printk("FT2D doesn't support GUI in FA626\n");
        return -1;
    }
#endif 

    
    switch (cmd) {
      case FT2DGE_CMD_SYNC:
        retVal = copy_from_user((void *)&usr_idx, (void *)arg, sizeof(struct ft2dge_idx));
        if (retVal) {
            ret = -EFAULT;
            break;
        }
        ft2dge_drv_sync(&usr_idx);
        break;

      case FT2DGE_CMD_VA2PA:
      {
        unsigned int vaddr, paddr;

        retVal = copy_from_user((void *)&vaddr, (void *)arg, sizeof(int));
        if (retVal) {
            ret = -EFAULT;
            break;
        }
        paddr = fmem_lookup_pa(vaddr);
        if (paddr == 0xFFFFFFFF) {
            ret = -EFAULT;
            break;
        }
        if (copy_to_user((void *)arg, (void *)&paddr, sizeof(int)))
            ret = -EFAULT;
        break;
      }

      default:
        printk("unknown command: 0x%x \n", cmd);
        ret = -1;
        break;
    }

    return ret;
}

static int ft2dge_drv_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long pfn;          //which page number is the start page
    unsigned long size = vma->vm_end - vma->vm_start;
    unsigned long off = vma->vm_pgoff << PAGE_SHIFT;    /* offset from the buffer start point */
    phys_addr_t   sharemem_paddr;
    int ret;

#ifdef FT2D_OSG_USING    
    if(FT2D_GET_CPU_mode == FT2D_OSG_CPU_FA626){
        printk("FT2D doesn't support GUI in FA626\n");
        return -1;
    }
#endif 

    
    if (size != PAGE_ALIGN(size)) {
        printk("%s, mmaped size:%d is not aligned to PAGE! \n", __func__, (u32)size);
        return -EFAULT;
    }

    if (off == 0) {
        /* mapping the share memory */
        sharemem_paddr = page_to_phys(drv_info.page);
        vma->vm_page_prot = PAGE_SHARED;
        vma->vm_flags |= VM_RESERVED;
        pfn = sharemem_paddr >> PAGE_SHIFT;    //which page number is the start page
    } else {
        /* mapping the register i/o */
        //vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
        vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
        vma->vm_flags |= VM_RESERVED;
        pfn = drv_info.io_paddr >> PAGE_SHIFT;    //which page number is the start page
    }

    ret = remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot);

    return ret;
}

/*
 * read_idx is updated by driver. write_idx is updated by user space.
 *
 */
static irqreturn_t ft2dge_irq_handler(int irq, void *data)
{
#ifdef FT2D_OSG_USING    
    ft2dge_sharemem_t *sharemem = (ft2dge_sharemem_t *)data;
    wait_list_t *node, *ne;
    int idx, read_cnt = 0, count;

    if(FT2D_GET_CPU_mode == FT2D_OSG_CPU_FA626){
        osg_drv_info.intr_cnt++;
         /* clear interrupt status */
        iowrite32(0x1, drv_info.io_mem + 0x5C);
        osg_drv_info.hw_running = 0;
        //printk("ird jiff time =%u\n",(u32)jiffies);
        wake_up(&osgwait_idle); 
    }
    else{
        drv_info.intr_cnt ++;

        sharemem->status &= ~STATUS_DRV_INTR_FIRE;

        /* clear interrupt status */
        iowrite32(0x1, drv_info.io_mem + 0x5C);

        /* update the read position */
        sharemem->read_idx += sharemem->fire_count;

        /* wake up all users waiting for interrupt */
        if (!list_empty(&drv_info.list)) {
            list_for_each_entry_safe(node, ne, &drv_info.list, list) {
                if (data_complete_check(node->usr_idx, sharemem))
                    continue;
                list_del(&node->list);
                node->event = 1;
                wake_up(&node->wait);
            }
    	}

        sharemem->fire_count = count = FT2DGE_BUF_COUNT(sharemem->write_idx, sharemem->read_idx);
        /* there is no any pending job. */
        if (count == 0) {
            sharemem->status &= ~STATUS_DRV_BUSY;
            goto exit;
        }

        read_cnt = 0;
        while (count --) {
            idx = FT2DGE_GET_BUFIDX(sharemem->read_idx + read_cnt);
            iowrite32(sharemem->cmd[idx].val, drv_info.io_mem + sharemem->cmd[idx].ofs);

            read_cnt ++;
            if (read_cnt >= HW_LLST_MAX_CMD)
                panic("%s, bug in library! \n", __func__);
        }

        /* fire */
        sharemem->status |= STATUS_DRV_INTR_FIRE;
        iowrite32(0x1, drv_info.io_mem + 0xFC);
        /* drain the write buffer */
        ioread32(drv_info.io_mem + 0xFC);
        sharemem->drv_fire ++;
    }
exit:
    return IRQ_HANDLED;        
#else 

    ft2dge_sharemem_t *sharemem = (ft2dge_sharemem_t *)data;
    wait_list_t *node, *ne;
    int idx, read_cnt = 0, count;

    drv_info.intr_cnt ++;

    sharemem->status &= ~STATUS_DRV_INTR_FIRE;

    /* clear interrupt status */
    iowrite32(0x1, drv_info.io_mem + 0x5C);

    /* update the read position */
    sharemem->read_idx += sharemem->fire_count;

    /* wake up all users waiting for interrupt */
    if (!list_empty(&drv_info.list)) {
        list_for_each_entry_safe(node, ne, &drv_info.list, list) {
            if (data_complete_check(node->usr_idx, sharemem))
                continue;
            list_del(&node->list);
            node->event = 1;
            wake_up(&node->wait);
        }
	}

    sharemem->fire_count = count = FT2DGE_BUF_COUNT(sharemem->write_idx, sharemem->read_idx);
    /* there is no any pending job. */
    if (count == 0) {
        sharemem->status &= ~STATUS_DRV_BUSY;
        goto exit;
    }

    read_cnt = 0;
    while (count --) {
        idx = FT2DGE_GET_BUFIDX(sharemem->read_idx + read_cnt);
        iowrite32(sharemem->cmd[idx].val, drv_info.io_mem + sharemem->cmd[idx].ofs);

        read_cnt ++;
        if (read_cnt >= HW_LLST_MAX_CMD)
            panic("%s, bug in library! \n", __func__);
    }

    /* fire */
    sharemem->status |= STATUS_DRV_INTR_FIRE;
    iowrite32(0x1, drv_info.io_mem + 0xFC);
    /* drain the write buffer */
    ioread32(drv_info.io_mem + 0xFC);
    sharemem->drv_fire ++;
exit:
    return IRQ_HANDLED;
#endif    
}

struct file_operations ft2dge_drv_fops = {
    owner: THIS_MODULE,
	unlocked_ioctl:	ft2dge_drv_ioctl,
	open:		ft2dge_drv_open,
	mmap:       ft2dge_drv_mmap,
	release:	ft2dge_drv_release,
};

struct miscdevice ft2dge_drv_dev = {
	minor: FT2DGE_MINOR,
	name: "ft2dge",
	fops: &ft2dge_drv_fops,
};

#ifdef FT2D_OSG_USING

static void ft2d_mark_engine_start(void)
{
    ENGINE_START = jiffies;
    ENGINE_END = 0;
    if(UTIL_START == 0)
	{
        UTIL_START = ENGINE_START;
        ENGINE_TIME = 0;
    }
}

static void ft2d_mark_engine_end(void)
{
    unsigned int utilization = 0;
    
    ENGINE_END = jiffies;

    if (ENGINE_END > ENGINE_START)
        ENGINE_TIME += ENGINE_END - ENGINE_START;

    if (UTIL_START > ENGINE_END) {
        UTIL_START = 0;
        ENGINE_TIME = 0;
    } else if ((UTIL_START <= ENGINE_END) && ((ENGINE_END - UTIL_START) >= (UTIL_PERIOD * HZ))) {
        
		if ((ENGINE_END - UTIL_START) == 0)
		{
			//printk("div by 0!!\n");
		}
		else
		{
        	utilization = (unsigned int)((100*ENGINE_TIME) / (ENGINE_END - UTIL_START));
            //printk("utilization=%d,%d-%d-%d\n",utilization,ENGINE_TIME,ENGINE_END,UTIL_START);
            ENGINE_TIME_RECORD = ENGINE_TIME;
            UTIL_PERIOD_RECORD = ENGINE_END - UTIL_START;            
		}
        if (utilization)
            UTIL_RECORD = utilization;
        UTIL_START = 0;
        ENGINE_TIME= 0;

    }
    ENGINE_START=0;

}
/* fire all added commands
 * b_wait = 0: not wait for complete
 * b_wait = 1:  wait for complete
 * return value: 0 for success, -1 for fail
 */
int ft2d_osg_fire(void)
{
    ft2d_osg_list_t *llst =(ft2d_osg_list_t*) osg_drv_info.cmd_list;
    int i,ret;
    //unsigned int stime,etime,difftime;

    
    if (!llst->cmd_cnt)
        return 0;

    if (llst->cmd_cnt >= HW_LLST_MAX_CMD) {
        printk("%s, command count: %d is over than %d!\n", __func__, llst->cmd_cnt, HW_LLST_MAX_CMD);
        return -1;
    }

    /* sanity check */
    if ((osg_drv_info.hw_running)) {
        printk("%s, osg hw is running(%d)!\n", __func__, osg_drv_info.hw_running);
        return -1;
    } 
    
    ft2d_mark_engine_start();
    //stime =  jiffies;
    /* fire the hardware? */
    osg_drv_info.hw_running = 1;
    osg_drv_info.fire_cnt++;
   
    for (i = 0; i < llst->cmd_cnt; i ++) {
        iowrite32(llst->cmd[i].val, drv_info.io_mem + llst->cmd[i].ofs);
        if(g_osg_dump_flag)
            printk("0x%08X 0x%08X\n", llst->cmd[i].ofs &(~LLIST_BASE), llst->cmd[i].val);
    }
    
    /* fire the hardware */
    iowrite32(0x1, drv_info.io_mem + 0xFC);
    /* drain the write buffer */
    ioread32(drv_info.io_mem + 0xFC);

    llst->cmd_cnt = 0;  //reset

    
     
    ret = wait_event_timeout(osgwait_idle, osg_drv_info.hw_running==0, 10 * HZ);
	ret = (ret > 0) ? 0 : (ret < 0) ? ret : -ETIMEDOUT;
    //etime =  jiffies;
    //difftime = (etime >= stime) ? (etime - stime) : (0xffffffff - stime + etime);
    //printk("fire sjiff =%u ejiff =%u difftime=%u\n",stime,etime,difftime);
	if(g_osg_dump_flag)
	    printk("\n*****FIRE FT2D_WAIT_IDLE***** %d 0x%08X\n", ret, osg_drv_info.hw_running);

    ft2d_mark_engine_end();
    //THINK2D_OSG_UNLOCK_MUTEX;

    return ret;
}

/* fire all added commands
 * b_wait = 0: not wait for complete
 * b_wait = 1:  wait for complete
 * return value: 0 for success, -1 for fail
 */
int ft2d_osg_remainder_fire(void)
{
    int ret = 0;
    ft2d_osg_list_t *llst =(ft2d_osg_list_t*) osg_drv_info.cmd_list;
    
    if (!llst->cmd_cnt)
        return 0;
    if((ret = ft2d_osg_fire())< 0 ){
        if( printk_ratelimit())
            printk("ft2d_osg_remainder_fire fail(%d)(%d)\n",__LINE__,ret);
        return -1;
    }
    return 0;
}
EXPORT_SYMBOL(ft2d_osg_remainder_fire);

void ft2d_osg_set_param(ft2d_osg_list_t *cmd_list, unsigned int ofs, unsigned int val)
{
    cmd_list->cmd[cmd_list->cmd_cnt].ofs = ofs;
    cmd_list->cmd[cmd_list->cmd_cnt].val = val;
    cmd_list->cmd_cnt ++;
}

int ft2d_osg_do_blit(ft2d_osg_blit_t* blit_pam,int fire)
{
    int ret = 0;
    ft2dge_cmd_t  command;
    ft2d_osg_list_t *cmd_list =(ft2d_osg_list_t*) osg_drv_info.cmd_list;
    //unsigned int stime,etime,difftime;
    
    //stime =  jiffies;
    if(FT2D_GET_CPU_mode == FT2D_OSG_CPU_FA726){
        printk("FT2D doesn't support OSG in FA726\n");
        return -1;
    }
    
    if(!blit_pam){
        printk("%s blit_pam NULL parameter\n",__func__);
        return  -1;  
    }
    if(!blit_pam->dst_paddr || !blit_pam->dst_w|| !blit_pam->dst_h 
       ||!blit_pam->src_paddr || !blit_pam->src_w ||!blit_pam->src_h){
        printk("%s invalide parameter dst w-h-pa(%d-%d-%x) src w-h-pa(%d-%d-%x)\n"
               ,__func__,blit_pam->dst_w,blit_pam->dst_h,blit_pam->dst_paddr
               ,blit_pam->src_w,blit_pam->src_h,blit_pam->src_paddr);
        return  -1;            
    }

    if(!FT2D_OSG_CMD_IS_ENOUGH(cmd_list->cmd_cnt ,12))
    {
        if((ret = ft2d_osg_fire())< 0 ){
            if(printk_ratelimit())
                printk("ft2d_osg_fire(%d)(%d)\n",__LINE__,ret);
            return -1;
        }   
    }
    
    /*Set pitch*/    
    ft2d_osg_set_param(cmd_list, FT2DGE_SPDP, FT2DGE_PITCH(blit_pam->src_w,blit_pam->dst_w)); 
    /*SET TARGET ADDRESS*/
    ft2d_osg_set_param(cmd_list, FT2DGE_DSTSA,blit_pam->dst_paddr);
    /*SET SOURCE ADDRESS*/
    ft2d_osg_set_param(cmd_list, FT2DGE_SRCSA,blit_pam->src_paddr); 

    command.value = 0;
    command.bits.command = FT2DGE_CMD_COLORKEY;
    command.bits.mono_pattern_or_match_not_write = 1;
    command.bits.rop = ROP_SRCCOPY;
    command.bits.bpp = FT2DGE_RGB_565;
    command.bits.no_source_image = 0;
    ft2d_osg_set_param(cmd_list, FT2DGE_CMD, command.value);
    /*SET COLOR KEY*/
    ft2d_osg_set_param(cmd_list, FT2DGE_TPRCOLR,blit_pam->src_colorkey);
    ft2d_osg_set_param(cmd_list, FT2DGE_TPRMASK,blit_pam->src_colorkey);
    /* srouce object start x/y */
    ft2d_osg_set_param(cmd_list, FT2DGE_SRCXY, FT2DGE_XY(0, 0));
    /* Source Rectangle Width and Height */
    ft2d_osg_set_param(cmd_list, FT2DGE_SRCWH, FT2DGE_WH(blit_pam->src_w, blit_pam->src_h));
    
    /* Destination object upper-left X/Y coordinates */
	ft2d_osg_set_param(cmd_list, FT2DGE_DSTXY, FT2DGE_XY(blit_pam->dx,blit_pam->dy));
	/* Destination rectangle width and height */
	ft2d_osg_set_param(cmd_list, FT2DGE_DSTWH, FT2DGE_WH(blit_pam->src_w,blit_pam->src_h));
	/* when all parameters are updated, this is the final step */
    ft2d_osg_set_param(cmd_list, FT2DGE_TRIGGER, 0x1);

    if(fire){
         if((ret = ft2d_osg_fire())< 0 ){
            if(printk_ratelimit())
                printk("ft2d_osg_fire(%d)(%d)\n",__LINE__,ret);
            return -1;
        }   
    }
    //etime =  jiffies;
    //difftime = (etime >= stime) ? (etime - stime) : (0xffffffff - stime + etime);
    //printk("ft2d osg blit sjiff =%u ejiff =%u difftime=%u\n",stime,etime,difftime);
    //THINK2D_OSG_UNLOCK_MUTEX;
    return 0;
}

EXPORT_SYMBOL(ft2d_osg_do_blit);

int ft2d_osg_drawrect(ft2d_osg_mask_t* mask_pam,int fire)
{
    int ret;
    ft2dge_cmd_t  command;
    ft2d_osg_list_t *cmd_list =(ft2d_osg_list_t*) osg_drv_info.cmd_list;

    int x1 = mask_pam->dx1;
    int y1 = mask_pam->dy1;
    int x2 = mask_pam->dx2;
    int y2 = mask_pam->dy2;

    if(!FT2D_OSG_CMD_IS_ENOUGH(cmd_list->cmd_cnt ,20))
    {
        if((ret = ft2d_osg_fire())< 0 ){
            if(printk_ratelimit())
                printk("ft2d_osg_fire(%d)(%d)\n",__LINE__,ret);
            return -1;
        }   
    }

    if(mask_pam->border_sz == 0 || (mask_pam->dx1 == mask_pam->dx2)
       || (mask_pam->dy1 == mask_pam->dy2) ){
        printk("%s invalide parameter\n tar x1-y1-x2-y2(%d-%d-%d-%d) sz=%d"
               ,__func__,mask_pam->dx1,mask_pam->dy1,mask_pam->dx2,mask_pam->dy2,mask_pam->border_sz);
        return  -1;            
    }
    //printk("drawrect %d-%d-%d-%d\n",x1,y1,x2,y2);
    /* command action */
    command.value = 0;
    command.bits.rop = ROP_SRCCOPY;
    command.bits.command = FT2DGE_CMD_BITBLT;
    command.bits.bpp = FT2DGE_RGB_565;
    command.bits.no_source_image = 1;   
    
    ft2d_osg_set_param(cmd_list, FT2DGE_SPDP, FT2DGE_PITCH(mask_pam->dst_w,mask_pam->dst_w)); 
    ft2d_osg_set_param(cmd_list, FT2DGE_DSTSA,mask_pam->dst_paddr); 
    ft2d_osg_set_param(cmd_list, FT2DGE_SFGCOLR,mask_pam->color);

    ///left
    ft2d_osg_set_param(cmd_list, FT2DGE_CMD, command.value);

    /* destination object upper-left X/Y coordinates */
    ft2d_osg_set_param(cmd_list, FT2DGE_DSTXY, FT2DGE_XY(x1, y1));
    /* destination object rectanlge width and height */
    ft2d_osg_set_param(cmd_list, FT2DGE_DSTWH, FT2DGE_WH(mask_pam->border_sz,y2-y1+1));
    /* when all parameters are updated, this is the final step */
    ft2d_osg_set_param(cmd_list, FT2DGE_TRIGGER, 0x1);
    ////bottom
    ft2d_osg_set_param(cmd_list, FT2DGE_CMD, command.value);

    /* destination object upper-left X/Y coordinates */
    ft2d_osg_set_param(cmd_list, FT2DGE_DSTXY, FT2DGE_XY(x1, y2-mask_pam->border_sz+1));
    /* destination object rectanlge width and height */
    ft2d_osg_set_param(cmd_list, FT2DGE_DSTWH, FT2DGE_WH(x2-x1+1,mask_pam->border_sz));
    /* when all parameters are updated, this is the final step */
    ft2d_osg_set_param(cmd_list, FT2DGE_TRIGGER, 0x1);

    ///top
    ft2d_osg_set_param(cmd_list, FT2DGE_CMD, command.value);
    /* destination object upper-left X/Y coordinates */
    ft2d_osg_set_param(cmd_list, FT2DGE_DSTXY, FT2DGE_XY(x1, y1));
    /* destination object rectanlge width and height */
    ft2d_osg_set_param(cmd_list, FT2DGE_DSTWH,  FT2DGE_WH(x2-x1+1,mask_pam->border_sz));
    /* when all parameters are updated, this is the final step */
    ft2d_osg_set_param(cmd_list, FT2DGE_TRIGGER, 0x1);
    ///right
    ft2d_osg_set_param(cmd_list, FT2DGE_CMD, command.value);
    /* destination object upper-left X/Y coordinates */
    ft2d_osg_set_param(cmd_list, FT2DGE_DSTXY, FT2DGE_XY(x2-mask_pam->border_sz +1, y1));
    /* destination object rectanlge width and height */
    ft2d_osg_set_param(cmd_list, FT2DGE_DSTWH, FT2DGE_WH(mask_pam->border_sz,y2-y1+1));
    /* when all parameters are updated, this is the final step */
    ft2d_osg_set_param(cmd_list, FT2DGE_TRIGGER, 0x1);
    
    if(fire){
        if((ret = ft2d_osg_fire())< 0 ){
            if(printk_ratelimit())
                printk("ft2d_osg_fire(%d)(%d)\n",__LINE__,ret);
            return -1;
        }   
    }

    return 0;

}


int ft2d_osg_fillrect(ft2d_osg_mask_t* mask_pam,int fire)
{
    int ret = 0;
    ft2dge_cmd_t  command;
    ft2d_osg_list_t *cmd_list =(ft2d_osg_list_t*) osg_drv_info.cmd_list;
    
    if(!FT2D_OSG_CMD_IS_ENOUGH(cmd_list->cmd_cnt ,8))
    {
        if((ret = ft2d_osg_fire())< 0 ){
            if(printk_ratelimit())
                printk("ft2d_osg_fire(%d)(%d)\n",__LINE__,ret);
            return -1;
        }   
    }
    /* command action */
    command.value = 0;
    command.bits.rop = ROP_SRCCOPY;
    command.bits.command = FT2DGE_CMD_BITBLT;
    command.bits.bpp = FT2DGE_RGB_565;
    command.bits.no_source_image = 1;   
 
    ft2d_osg_set_param(cmd_list, FT2DGE_SPDP, FT2DGE_PITCH(mask_pam->dst_w,mask_pam->dst_w)); 
    ft2d_osg_set_param(cmd_list, FT2DGE_DSTSA,mask_pam->dst_paddr); 
    ft2d_osg_set_param(cmd_list, FT2DGE_SFGCOLR,mask_pam->color);
    
    ft2d_osg_set_param(cmd_list, FT2DGE_CMD, command.value);

    /* destination object upper-left X/Y coordinates */
    ft2d_osg_set_param(cmd_list, FT2DGE_DSTXY, FT2DGE_XY(mask_pam->dx1, mask_pam->dy1));
    /* destination object rectanlge width and height */
    ft2d_osg_set_param(cmd_list, FT2DGE_DSTWH, FT2DGE_WH((mask_pam->dx2-mask_pam->dx1+1),mask_pam->dy2-mask_pam->dy1+1));
    /* when all parameters are updated, this is the final step */
    ft2d_osg_set_param(cmd_list, FT2DGE_TRIGGER, 0x1);

    if(fire){
        if((ret = ft2d_osg_fire())< 0 ){
            if(printk_ratelimit())
                printk("ft2d_osg_fire(%d)(%d)\n",__LINE__,ret);
            return -1;
        }   
    }
    return 0;
}

int ft2d_osg_do_mask(ft2d_osg_mask_t* mask_pam,int fire)
{
    if(FT2D_GET_CPU_mode == FT2D_OSG_CPU_FA726){
        printk("FT2D doesn't support OSG in FA726\n");
        return -1;
    }
  
    if(!mask_pam->dst_paddr || !mask_pam->dst_w|| !mask_pam->dst_h        
       || (mask_pam->type != FT2D_OSG_BORDER_TURE&&mask_pam->type != FT2D_OSG_BORDER_HORROW )){
            printk("%s invalide parameter dst w-h-pa(%d-%d-%x) type=%d\n"
                   ,__func__,mask_pam->dst_w,mask_pam->dst_h,mask_pam->dst_paddr,mask_pam->type);
            return  -1;            
    }

    if(mask_pam->type == FT2D_OSG_BORDER_TURE){
        if(ft2d_osg_fillrect(mask_pam,fire) < 0){
            printk("ft2d_osg_fillrect fail\n");
            return  -1;                
        }
    }

    
    if(mask_pam->type == FT2D_OSG_BORDER_HORROW){
        if(ft2d_osg_drawrect(mask_pam,fire) < 0){
            printk("ft2d_osg_drawrect fail\n");
            return  -1;                
        }
    }

    return 0;

}

EXPORT_SYMBOL(ft2d_osg_do_mask);

static int ft2dge_set_running_mode(ft2d_cpu_id_t *cpu_id)
{
    int ret = 0;
    fmem_pci_id_t p_id;
    fmem_cpu_id_t c_id;
    
    fmem_get_identifier(&p_id, &c_id);
    
    switch (c_id) {
      case FMEM_CPU_FA726:
        *cpu_id = FT2D_OSG_CPU_FA726;
        break; 
      case FMEM_CPU_FA626:  
        *cpu_id = FT2D_OSG_CPU_FA626;
        break; 
      default:
        *cpu_id = FT2D_OSG_CPU_UNKNOWN;
        ret = -1;
        break;
    }
    return ret;
}

static int proc_osgread_debug_info(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    

    len += sprintf(page + len, "driver version: %s \n", FT2DGE_VERSION);
    len += sprintf(page + len, "osg_drv_info cpu_id: %d \n", osg_drv_info.cpu_id);
    len += sprintf(page + len, "osg_drv_info fire_cnt: %d \n", osg_drv_info.fire_cnt);
    len += sprintf(page + len, "osg_drv_info intr_cnt: %d \n", osg_drv_info.intr_cnt);
    len += sprintf(page + len, "osg_drv_info hw_running: 0x%x \n", osg_drv_info.hw_running);
    len += sprintf(page + len, "osg_drv_info command cnt: %d \n",  osg_drv_info.cmd_list->cmd_cnt);
    return len;
}
static int proc_osgwrite_debug_dump(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int mode_set = 0;
    sscanf(buffer, "%d", &mode_set);
    g_osg_dump_flag = mode_set;
    //printk("g_dump_dbg_flag =%d \n", g_dump_dbg_flag);
    return count;
}

static int proc_osgread_debug_dump(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    return sprintf(page, "osg_dump_dbg_flag =%d\n", g_osg_dump_flag);
}

static int proc_osgwrite_util_info(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int mode_set=0;
    sscanf(buffer, "%d", &mode_set);
    UTIL_PERIOD = mode_set;
    printk("\nFT2D Utilization Period =%d(sec)\n",  UTIL_PERIOD);
	
    return count;
}

static int proc_osgread_util_info(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;

    len += sprintf(page + len, "\nFT2D Period=%d(ms) consume time=%d(ms)\n",
                   (UTIL_PERIOD_RECORD*1000/HZ),(ENGINE_TIME_RECORD*1000/HZ));   

    *eof = 1;
    return len;
}

static int ft2dge_proc_osg_init(void)
{
     /*
     * debug message
     */
    osg_debug_info = create_proc_entry("osg_debug", S_IRUGO, ft2dge_proc_root);
    if (osg_debug_info == NULL)
        panic("Fail to create proc osg debug!\n");
    osg_debug_info->read_proc = (read_proc_t *) proc_osgread_debug_info;
    osg_debug_info->write_proc = NULL;  

    osg_debug_dump = create_proc_entry("osg_debug_dump", S_IRUGO, ft2dge_proc_root);
    if (osg_debug_dump == NULL)
        panic("Fail to create proc osg debug dump!\n");
    osg_debug_dump->read_proc = (read_proc_t *) proc_osgread_debug_dump;
    osg_debug_dump->write_proc = (write_proc_t *)proc_osgwrite_debug_dump;

    osg_util_info = create_proc_entry("ft2d_osg_util", S_IRUGO, ft2dge_proc_root);
    if (osg_util_info == NULL)
        panic("Fail to create proc osg_util_info!\n");
    osg_util_info->read_proc = (read_proc_t *) proc_osgread_util_info;
    osg_util_info->write_proc = (write_proc_t *)proc_osgwrite_util_info;    
    return 0;
}

static int ft2dge_proc_osg_release(void)
{
    if (osg_debug_info != NULL)
        remove_proc_entry(osg_debug_info->name, ft2dge_proc_root); 

    if(osg_debug_dump !=NULL)
        remove_proc_entry(osg_debug_dump->name, ft2dge_proc_root);  

    if(osg_util_info !=NULL)
        remove_proc_entry(osg_util_info->name, ft2dge_proc_root);  
    return 0;
}

static int ft2dge_init_osg_data(void)
{
    memset(&osg_drv_info,0x0,sizeof(osg_drv_info));
    osg_drv_info.cmd_list= (void *)kmalloc(sizeof(ft2d_osg_list_t),GFP_KERNEL);
    if (!osg_drv_info.cmd_list) {
        panic("%s, allocate llst fail! \n", __func__);
    }
    if(ft2dge_set_running_mode(&osg_drv_info.cpu_id) < 0)
        panic("%s, CPU_UNKNOWN type! \n", __func__);
    
    memset((void *)osg_drv_info.cmd_list, 0, sizeof(ft2d_osg_list_t));
    return 0;
}

static int ft2dge_release_osg_data(void)
{
    kfree(osg_drv_info.cmd_list);
    return 0;
}

#endif
static int __init ft2dge_drv_init(void)
{
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;
    int ret, order;

    /* sanity check */
    if (HW_LLST_MAX_CMD != LLST_MAX_CMD)
        panic("%s, sanity check fail(%d %d is not matched)! \n",
                                    __func__, HW_LLST_MAX_CMD, LLST_MAX_CMD);
    //compiler check
    fmem_get_identifier(&pci_id, &cpu_id);

    memset(&drv_info, 0, sizeof(drv_info));

    drv_info.io_paddr = GRA_FT2DGRA_PA_BASE;
    drv_info.io_mem = (void *)ioremap_nocache(drv_info.io_paddr, PAGE_SIZE);
    if (drv_info.io_mem == NULL)
        panic("%s, fail in io mapping! \n", __func__);

    order = get_order(PAGE_ALIGN(sizeof(ft2dge_sharemem_t)));
    drv_info.page = alloc_pages(GFP_KERNEL, order);
    if (drv_info.page == NULL)
        panic("%s, no page memory for page order:%d! \n", __func__, order);
    SetPageReserved(drv_info.page);  //no swap out
    drv_info.page_order = order;
    drv_info.sharemem = (ft2dge_sharemem_t *)__va(page_to_phys(drv_info.page));
    memset((void *)drv_info.sharemem, 0, PAGE_SIZE << order);

    drv_info.irq = GRA_FT2DGRA_IRQ;
    ret = request_irq(drv_info.irq, ft2dge_irq_handler, 0, "ft2dge", drv_info.sharemem);
    if (ret < 0)
        panic("%s, request irq%d fail! \n", __func__, drv_info.irq);

    ret = misc_register(&ft2dge_drv_dev);
    if (ret)
        panic("%s, misc_register fail! \n", __func__);

    spin_lock_init(&drv_info.lock);
    INIT_LIST_HEAD(&drv_info.list);    

    ft2dge_proc_init();

    platform_init();
#ifdef FT2D_OSG_USING     
    ft2dge_init_osg_data();
    memset(&g_utilization,0x0,sizeof(ft2dge_util_t));
    UTIL_PERIOD = 1;
#endif    
    printk("Ft2dge driver version: %s \n", FT2DGE_VERSION);

    return ret;
}

static void __exit ft2dge_drv_exit(void)
{
    /* check if the hardware is busy */
    ft2dge_proc_release();
    free_irq(drv_info.irq, drv_info.sharemem);
    __iounmap(drv_info.io_mem);
    ClearPageReserved(drv_info.page);
    __free_pages(drv_info.page, drv_info.page_order);
    drv_info.sharemem = NULL;
#ifdef FT2D_OSG_USING    
    ft2dge_release_osg_data();
#endif    
    misc_deregister(&ft2dge_drv_dev);
    platform_exit();

    return;
}

static int proc_read_debug_info(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    ft2dge_sharemem_t *sharemem = (ft2dge_sharemem_t *)drv_info.sharemem;

    len += sprintf(page + len, "driver version: %s \n", FT2DGE_VERSION);
    len += sprintf(page + len, "sharemem read_idx: %d \n", sharemem->read_idx);
    len += sprintf(page + len, "sharemem write_idx: %d \n", sharemem->write_idx);
    len += sprintf(page + len, "application open count: %d \n", drv_info.open_cnt);
    len += sprintf(page + len, "driver status: 0x%x \n", sharemem->status);
    len += sprintf(page + len, "application blocking: %d \n", drv_info.usr_blocking_cnt);
    len += sprintf(page + len, "interrupt count: %d \n", drv_info.intr_cnt);
    len += sprintf(page + len, "user fire count: %d \n", sharemem->usr_fire);
    len += sprintf(page + len, "driver fire count: %d \n", sharemem->drv_fire);
    len += sprintf(page + len, "tricky count: %d \n", drv_info.tricky_cnt);

    return len;
}

static int ft2dge_proc_init(void)
{
    int ret = 0;
    struct proc_dir_entry *p;

    p = create_proc_entry("ft2dge", S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (p == NULL) {
        ret = -ENOMEM;
        goto end;
    }

    ft2dge_proc_root = p;

    /*
     * debug message
     */
    debug_info = create_proc_entry("debug", S_IRUGO, ft2dge_proc_root);
    if (debug_info == NULL)
        panic("Fail to create proc ddr_info!\n");
    debug_info->read_proc = (read_proc_t *) proc_read_debug_info;
    debug_info->write_proc = NULL;    
#ifdef FT2D_OSG_USING    
    ft2dge_proc_osg_init();
#endif
end:
    return ret;
}

static void ft2dge_proc_release(void)
{
    if (debug_info != NULL)
        remove_proc_entry(debug_info->name, ft2dge_proc_root);

#ifdef FT2D_OSG_USING 
    ft2dge_proc_osg_release();
#endif

    if (ft2dge_proc_root != NULL)
        remove_proc_entry(ft2dge_proc_root->name, NULL);
}

module_init(ft2dge_drv_init);
module_exit(ft2dge_drv_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("2D engine driver");
MODULE_LICENSE("GPL");
