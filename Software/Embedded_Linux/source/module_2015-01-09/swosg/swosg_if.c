/*
 * Revision History
 * VER1.1 2014/03/13 Add version 
 * 
 * 
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/hardirq.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/pgalloc.h>

#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#include "swosg/swosg_if.h"
#include "swosg_eng.h"

unsigned int log_level = LOG_QUIET;
unsigned int g_limit_printk = 1;


static unsigned int idn = 0;
module_param(idn, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(idn, "index number");

static unsigned int mem = 0;
module_param(mem, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(mem, "total memory size for all channel");



/* proc init.
 */
static struct proc_dir_entry *swosg_if_proc_root = NULL;
static struct proc_dir_entry *swosg_if_debug_level = NULL;
static struct proc_dir_entry *swosg_if_canvas_info = NULL;
static struct proc_dir_entry *swosg_if_prink_limit = NULL;
#ifdef SUPPORT_SWOSG_CANVAS_STATIC
static struct proc_dir_entry *swosg_static_mem_pool = NULL;
#endif
int swosg_if_palette_set( int id, u32 crycby)
{
    if(id >= SWOSG_PALETTE_MAX || id < 0){
        DEBUG_M(LOG_WARNING,"swosg_if:set palette id(%d) out of range\n",id);     
        return -1;
    }    
    
    return swosg_eng_palette_set(id, crycby);
}
EXPORT_SYMBOL(swosg_if_palette_set);

int swosg_if_palette_get(int id, u32 *crycby)
{
    if(id >= SWOSG_PALETTE_MAX ||id<0 || !crycby){
        DEBUG_M(LOG_WARNING,"swosg_if:get palette id(%d) out of range\n",id);     
        return -1;
    }    
   
    return swosg_eng_palette_get( id, crycby);    
}

EXPORT_SYMBOL(swosg_if_palette_get);

int swosg_if_get_palette_number(void)
{
   return SWOSG_PALETTE_MAX;
}

EXPORT_SYMBOL(swosg_if_get_palette_number);

unsigned int swosg_if_get_version(void)
{
    return swosg_eng_get_version();
}

EXPORT_SYMBOL(swosg_if_get_version);

unsigned int swosg_if_get_chan_num(void)
{
    return SWOSG_CH_MAX_MUN;
}

EXPORT_SYMBOL(swosg_if_get_chan_num);


int swosg_if_canvas_set( int id, swosg_if_canvas_t * canvas)
{
    if(id >= SWOSG_CANVAS_MAX || id < 0){
        DEBUG_M(LOG_WARNING,"swosg_if:add canvas id(%d) out of range\n",id);
        return -1;
    }
    
    return swosg_eng_canvas_set(id, canvas);    
}
EXPORT_SYMBOL(swosg_if_canvas_set);

int swosg_if_canvas_del(int id)
{
    if(id >= SWOSG_CANVAS_MAX ||id < 0 ){
        DEBUG_M(LOG_WARNING,"swosg_if:del canvas id(%d) out of range\n",id);
        return -1;
    }
    return swosg_eng_canvas_del(id);
    return 0;
}

EXPORT_SYMBOL(swosg_if_canvas_del);

int swosg_if_do_blit(swosg_if_blit_param_t * blit_param,int num)
{
#if 0
    int i;
    swosg_if_blit_param_t *tmp_param;
    for( i = 0;i < num;i++){
        tmp_param = &blit_param[i];
        if(swosg_eng_do_blit(tmp_param) < 0){
            DEBUG_M(LOG_WARNING,"swosg:do blit is error %d\n",i); 
            continue;
        }
    }
#else    
    if(swosg_eng_do_blit(blit_param,num) < 0)
        return -1;
#endif    
    return 0;
}

EXPORT_SYMBOL(swosg_if_do_blit);

int swosg_if_do_mask(swosg_if_mask_param_t * mask_param,int num)
{
    if(swosg_eng_do_mask(mask_param,num) < 0)
        return -1;
    return 0;
}

EXPORT_SYMBOL(swosg_if_do_mask);

#ifdef SUPPORT_SWOSG_CANVAS_STATIC

int swosg_if_get_static_pool_info(int id,swosg_if_static_info * canvas)
{
    return swosg_eng_canvas_get_static_pool(id,canvas);
}
EXPORT_SYMBOL(swosg_if_get_static_pool_info);

#ifdef SWOSG_API_TEST
unsigned int swosg_if_get_canvas_runmode(void)
{
    return swosg_eng_get_canvas_runmode();
}
#else
u32 swosg_if_get_canvas_runmode(void)
{
    return swosg_eng_get_canvas_runmode();
}
#endif

EXPORT_SYMBOL(swosg_if_get_canvas_runmode);
#endif


static int swosg_proc_set_printk_limit(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int mode_set = 0;
    sscanf(buffer, "%d", &mode_set);
    if(mode_set !=0 && mode_set !=1){
        printk("set printk limit value must be 1 or 0 (%d)\n",mode_set);
        return count;
    }
    g_limit_printk = mode_set;
    //printk("g_dump_dbg_flag =%d \n", g_dump_dbg_flag);
    return count;
}

static int swosg_proc_read_printk_limit(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    return sprintf(page, "printk limit flag =%d\n", g_limit_printk);
}

static int swosg_proc_set_dbg(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int mode_set = 0;
    sscanf(buffer, "%d", &mode_set);
    log_level = mode_set;
    //printk("g_dump_dbg_flag =%d \n", g_dump_dbg_flag);
    return count;
}

static int swosg_proc_read_dbg(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    return sprintf(page, "g_dump_dbg_flag =%d\n", log_level);
}

#ifdef SUPPORT_SWOSG_CANVAS_STATIC

static int swosg_proc_read_canvas_inof_pool(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    return swosg_eng_proc_read_canvas_pool_info(page,start,off,count,eof,data);
}

static int swosg_proc_write_canvas_inof_pool(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
    return swosg_eng_proc_write_canvas_pool_info(file,buf,size,off);
}

#endif

static int swosg_proc_read_canvas_info(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int i;
    int len = 0;
    struct swosg_eng_canvas_desc  canvas_info;

    printk("osg dma allocate count=%d\n",swosg_eng_get_dma_count());
    
    for(i = 0 ;i<SWOSG_CANVAS_MAX;i++){
       memset(&canvas_info,0x0,sizeof(struct swosg_eng_canvas_desc));

       if(swosg_eng_canvas_get_info(i,&canvas_info) < 0 )
        continue;
       else{
        if(canvas_info.usage == 1){
         
            printk("osg canvas idx: %d\n", i);
            printk("osg canvas usage: %d\n",canvas_info.usage);
            printk("osg canvas vaddr: 0x%08x\n",(u32)canvas_info.canvas_desc.canvas_vaddr);
            printk("osg canvas paddr: 0x%08x\n",(u32)canvas_info.canvas_desc.canvas_paddr);
            printk("osg canvas size: %d\n",canvas_info.canvas_desc.canvas_size);
            printk("osg canvas width: %d\n",canvas_info.canvas_desc.canvas_w);
            printk("osg canvas heigh: %d\n",canvas_info.canvas_desc.canvas_h);
  
        }
       }  
    }
   
    return len;
}

static void swosg_if_proc_release(void)
{
    if (swosg_if_prink_limit!=NULL)
        remove_proc_entry(swosg_if_prink_limit->name, swosg_if_proc_root);
    
    if (swosg_if_canvas_info!=NULL)
        remove_proc_entry(swosg_if_canvas_info->name, swosg_if_proc_root);
     
	if (swosg_if_debug_level != NULL)
        remove_proc_entry(swosg_if_debug_level->name, swosg_if_proc_root);
    
#ifdef SUPPORT_SWOSG_CANVAS_STATIC
    if (swosg_static_mem_pool != NULL)
        remove_proc_entry(swosg_static_mem_pool->name, swosg_if_proc_root);
#endif       
	
	if (swosg_if_proc_root != NULL)
        remove_proc_entry(swosg_if_proc_root->name, NULL);

}

static int swosg_if_proc_init(void)
{
	int ret = 0;
    struct proc_dir_entry *p;

    p = create_proc_entry("swosg", S_IFDIR | S_IRUGO | S_IXUGO, NULL);

	if (p == NULL) {
        ret = -ENOMEM;
        goto end;
    }

    swosg_if_proc_root = p;    
	
	swosg_if_debug_level = create_proc_entry("dbg_level", S_IRUGO, swosg_if_proc_root);
	
	if (swosg_if_debug_level == NULL)
		  panic("Fail to create proc swosg debug level!\n");
	swosg_if_debug_level->read_proc = (read_proc_t *) swosg_proc_read_dbg;
	swosg_if_debug_level->write_proc =(write_proc_t *)swosg_proc_set_dbg;

    swosg_if_canvas_info = create_proc_entry("canvas_info", S_IRUGO, swosg_if_proc_root);
	
	if (swosg_if_canvas_info == NULL)
		  panic("Fail to create proc swosg canvas info!\n");
	swosg_if_canvas_info->read_proc = (read_proc_t *) swosg_proc_read_canvas_info;
	swosg_if_canvas_info->write_proc = NULL;

    swosg_if_prink_limit = create_proc_entry("printk_limit", S_IRUGO, swosg_if_proc_root);
    if (swosg_if_prink_limit == NULL)
		  panic("Fail to create proc swosg_if_prink_limit!\n");
	swosg_if_prink_limit->read_proc = (read_proc_t *) swosg_proc_read_printk_limit;
	swosg_if_prink_limit->write_proc = (write_proc_t *)swosg_proc_set_printk_limit;

#ifdef SUPPORT_SWOSG_CANVAS_STATIC
    swosg_static_mem_pool  = create_proc_entry("canvas_static_info", S_IRUGO, swosg_if_proc_root);
    if (swosg_static_mem_pool == NULL)
		  panic("Fail to create proc swosg_static_mem_pool!\n");
	swosg_static_mem_pool->read_proc = (read_proc_t *) swosg_proc_read_canvas_inof_pool;
	swosg_static_mem_pool->write_proc = (write_proc_t *)swosg_proc_write_canvas_inof_pool;
#endif
end:
    return ret;
}

#ifdef SWOSG_API_TEST
#define SWOSG_DEV_NAME       "swosg_t"

static int swosg_file_open(struct inode *inode, struct file *filp)
{
    return 0;

}
/*
 *  Note: this function only applies to the user space address
 */
unsigned int swosg_dvr_user_va_to_pa(unsigned int addr)
{
    pmd_t *pmdp;
    pte_t *ptep;
    unsigned int paddr, vaddr;

    pmdp = pmd_offset(pud_offset(pgd_offset(current->mm, addr), addr), addr);
    if (unlikely(pmd_none(*pmdp)))
        return 0;

    ptep = pte_offset_kernel(pmdp, addr);
    if (unlikely(pte_none(*ptep)))
        return 0;

    vaddr = (unsigned int)page_address(pte_page(*ptep)) + (addr & (PAGE_SIZE - 1));
    paddr = __pa(vaddr);

    return paddr;
}
static unsigned int rand_pool = 0x12345678;
static unsigned int rand_add  = 0x87654321;

static inline unsigned int myrand(void)
{
     rand_pool ^= ((rand_pool << 7) | (rand_pool >> 25));
     rand_pool += rand_add;
      rand_add  += rand_pool;

     return rand_pool;
}

swosg_if_mask_param_t g_mask_test[200];
swosg_if_blit_param_t g_blit_test[200];

static long swosg_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    swosg_if_mask_param_t usr_data;
    swosg_if_blit_param_t usr_data1;
    swosg_if_canvas_t     usr_data2;
    long ret = 0;
    unsigned long  retval;
    unsigned int addr_pa;
    int i;
    int SW ,SH,SX = 0,SY=0 ,sz_data; 
    void       * tmpaddr;

    if (filp) {}
    
    switch (cmd) {
        case SWOSG_CMD_MASK:
            retval = copy_from_user((void *)&usr_data, (void *)arg, sizeof(swosg_if_mask_param_t));
            if (retval) {
                ret = -EFAULT;
                break;
            } 
            addr_pa = swosg_dvr_user_va_to_pa(usr_data.dest_bg_addr);
            usr_data.dest_bg_addr = (unsigned int)__va(addr_pa);

            SW = usr_data.dest_bg_w;
            SH = usr_data.dest_bg_h;
            SX = usr_data.target_w;
            SY = usr_data.target_h;
            
            for(i = 0;i< 200;i++)
            {
                usr_data.target_x = (SW!=SX)?(myrand()%(SW-SX)):0;
                usr_data.target_y = (SH!=SY)?(myrand()%(SH-SY)):0;

                memcpy(&g_mask_test[i],&usr_data,sizeof(swosg_if_mask_param_t));
            }
            swosg_if_do_mask(&g_mask_test[0],100);
            break;
        case SWOSG_CMD_BLIT:   
            retval = copy_from_user((void *)&usr_data1, (void *)arg, sizeof(swosg_if_blit_param_t));
            if (retval) {
                ret = -EFAULT;
                break;
            } 
            addr_pa = swosg_dvr_user_va_to_pa(usr_data1.dest_bg_addr);
            usr_data1.dest_bg_addr = (unsigned int)__va(addr_pa);

            SW = usr_data1.dest_bg_w;
            SH = usr_data1.dest_bg_h;
            SX = 320;
            SY = 72;
            
            for(i = 0;i< 200;i++)
            {
               //  usr_data1.target_x = usr_data1.target_x + 6*(i+1);//(SW!=SX)?(myrand()%(SW-SX)):0;
               // usr_data1.target_y = usr_data1.target_y + 2*(i+1);//(SH!=SY)?(myrand()%(SH-SY)):0;
                usr_data1.target_x = (SW!=SX)?(myrand()%(SW-SX)):0;
                usr_data1.target_y = (SH!=SY)?(myrand()%(SH-SY)):0;
                //if(i==4){
                   // usr_data1.target_x = 720;
                    usr_data1.src_patten_idx = (i%128)+4;
                //}
                memcpy(&g_blit_test[i],&usr_data1,sizeof(swosg_if_blit_param_t));
            }
            // for(i = 0;i< 100;i++)
            swosg_if_do_blit(&g_blit_test[0],200);
            break;
        case SWOSG_CMD_CANVAS_SET:
            retval = copy_from_user((void *)&usr_data2, (void *)arg, sizeof(swosg_if_canvas_t));
            if (retval) {
                ret = -EFAULT;
                break;
            } 
            sz_data = usr_data2.src_w*usr_data2.src_h*2;
            addr_pa = swosg_dvr_user_va_to_pa(usr_data2.src_cava_addr);
            usr_data2.src_cava_addr =  (unsigned int)__va(addr_pa);
            tmpaddr = kzalloc(sz_data, GFP_KERNEL);

            memcpy(tmpaddr,(void*)usr_data2.src_cava_addr ,sz_data);
            usr_data2.src_cava_addr = (unsigned int)tmpaddr;
            /*printk("%s pa=%x va=%x (%dx%d)\n",__func__,addr_pa,usr_data2.src_cava_addr,usr_data2.src_w,usr_data2.src_h);*/
            swosg_if_canvas_set(usr_data2.idx,&usr_data2);
            kfree(tmpaddr);
            break;
        default : 
            printk("no support type %d\n",cmd);
            
    }
    return ret;
}

static int swosg_file_release(struct inode *inode, struct file *filp)
{
    return 0;

}

static void swosg_platform_release(struct device *device)
{
	/* this is called when the reference count goes to zero */
}

static struct platform_device swosg_platform_device = {
	.name = SWOSG_DEV_NAME,
	.id	= 0,
	.dev	= {
		.release = swosg_platform_release,
	}
};


static struct platform_driver swosg_platform_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name  = SWOSG_DEV_NAME,
	},
};


static struct file_operations swosg_fops = {
	.owner		    = THIS_MODULE,
	.unlocked_ioctl = swosg_file_ioctl,	
	.open           = swosg_file_open,
	.release        = swosg_file_release
 
};

static struct miscdevice swosg_misc_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= SWOSG_DEV_NAME,
	.fops		= &swosg_fops,
};


#endif

static int __init swosd_if_init(void)
{
    unsigned int  ver_num = 0;
#ifdef SWOSG_API_TEST
    int ret;
	       
    /* register device */
	if ((ret = platform_device_register(&swosg_platform_device)))
	{
		printk("Failed to register platform device '%s'\n", swosg_platform_device.name);
		goto ERR1;
	}

    /* register driver */
	if ((ret = platform_driver_register(&swosg_platform_driver)))
	{
		printk("Failed to register platform driver '%s'\n", swosg_platform_driver.driver.name);
		goto ERR2;
	}    
  
	if ((ret = misc_register(&swosg_misc_device)))
	{
		printk("Failed to register misc device '%s'\n", swosg_misc_device.name);
		goto ERR3;
	}
#endif

    swosg_if_proc_init();

#ifdef SUPPORT_SWOSG_CANVAS_STATIC
    if(idn > SWOSG_CANVAS_MAX || (idn != 0 && mem == 0) ){
        printk("insmod parameter is error ch=%d , mem=%d\n",idn,mem);        
        swosg_if_proc_release();
        return -1;
    }
        
    swosg_eng_init(idn,mem);
#else
    swosg_eng_init();    
#endif

    ver_num = swosg_if_get_version();
    printk("SWOSG ver %d.%d.%d\n",(ver_num >> 16)&0xff,(ver_num >> 8)&0xff,ver_num &0xff);
    return 0;
    
#ifdef SWOSG_API_TEST            
    ERR3:
		platform_driver_unregister(&swosg_platform_driver);	
        
    ERR2:
		platform_device_unregister(&swosg_platform_device);
        
    ERR1:
        return ret;
#endif        
}

static void __exit swosd_if_exit(void)
{
#ifdef SWOSG_API_TEST
    misc_deregister(&swosg_misc_device);
	
	
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
	platform_driver_unregister(&swosg_platform_driver);
    #endif

    platform_device_unregister(&swosg_platform_device);
#endif
    swosg_if_proc_release();
    swosg_eng_exit();
    printk("Leave SWOSG\n");
    return;
}

module_init(swosd_if_init);
module_exit(swosd_if_exit);

MODULE_DESCRIPTION("Grain Media SWOSG driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");

