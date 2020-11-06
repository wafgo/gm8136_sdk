
#include <linux/param.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <mach/platform/platform_io.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/delay.h>                                                                                                                                                  
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include "swosg_vg.h"

#define SWOSG_VERSION        "1.0"
#define SWOSG_DEV_NAME       "swosg_t"

static int swosg_drv_proc_read_debug(char *page, char **start, off_t off, int count,int*eof, void *data);

typedef struct swosg_param_proc{
	char proc_name[256];
	struct proc_dir_entry *parent;
	struct proc_dir_entry *proc_entry;
	int (*p_proc_write)(struct file *file, const char *buffer, unsigned long count, void *data);
	int (*p_proc_read)(char *page, char **start, off_t off, int count,int *eof, void *data);
}prm_proc_t;


prm_proc_t g_prm_proc[] = 
{
	/* name, 			parent, entry, 	write, 						read */
	{"swosg",			NULL, 	NULL, 	NULL,						NULL},
	{"debug_proc",		NULL, 	NULL, 	NULL, 						swosg_drv_proc_read_debug},
};

/*
 * Module Parameter
 */

/*
 * debug
 */
static int swosg_drv_proc_read_debug(char *page, char **start, off_t off, int count,int*eof, void *data)
{
    int     len = 0 ;

    len += sprintf(page + len, "SWOSG Debug Message\n");
    osg_vg_debug_print(page, &len, data);
    
    *eof = 1;       //end of file
    return len;
}

void swosg_drv_proc_init(void)
{
	unsigned int i = 0;
	unsigned int proc_count = sizeof(g_prm_proc)/sizeof(prm_proc_t);
	umode_t mode = S_IRUGO | S_IXUGO;

	for (i = 0; i < proc_count; i++)
	{
		
		mode = S_IRUGO | S_IXUGO;
		if (i == 0)
		{
			mode |= S_IFDIR;
		}
		if (i > 0)
		{
			g_prm_proc[i].parent = g_prm_proc[0].proc_entry;
		}
		g_prm_proc[i].proc_entry = create_proc_entry(g_prm_proc[i].proc_name, mode,g_prm_proc[i].parent);
		g_prm_proc[i].proc_entry->read_proc = g_prm_proc[i].p_proc_read;
		g_prm_proc[i].proc_entry->write_proc = g_prm_proc[i].p_proc_write;
	}
}

/* proc remove function
 */

void swosg_proc_remove(void)
{
	unsigned int i = 0;
	unsigned int proc_count = sizeof(g_prm_proc)/sizeof(prm_proc_t);
	for (i = proc_count; i > 0; i--)
	{
		remove_proc_entry(g_prm_proc[i - 1].proc_name, g_prm_proc[i - 1].parent);
	}
}

int swosg_drv_probe(struct platform_device *pdev)
{
	return 0;
}

static int swosg_drv_remove(struct platform_device *pdev)
{      
	return 0;
}
static void swosg_platform_release(struct device *device)
{
	/* this is called when the reference count goes to zero */
}


static struct platform_device swosg_platform_device = {
	.name	= SWOSG_DEV_NAME,
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

	.probe	= swosg_drv_probe,
	.remove = swosg_drv_remove,
};


static struct file_operations swosg_fops = {
	.owner		    = THIS_MODULE,
 
};

static struct miscdevice swosg_misc_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= SWOSG_DEV_NAME,
	.fops		= &swosg_fops,
};

static int __init swosg_module_init(void)
{
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

    osg_vg_init();
    swosg_drv_proc_init();
    printk("** SW OSG Version %s ** done\n",SWOSG_VERSION);
	return 0;

        
    ERR3:
		platform_driver_unregister(&swosg_platform_driver);	
        
    ERR2:
		platform_device_unregister(&swosg_platform_device);
        
    ERR1:
        return ret;
        
}  


static void __exit swosg_module_exit(void)
{
    osg_vg_driver_clearnup();
    swosg_proc_remove();
	misc_deregister(&swosg_misc_device);
	platform_device_unregister(&swosg_platform_device);	
	platform_driver_unregister(&swosg_platform_driver);
}

module_init(swosg_module_init);
module_exit(swosg_module_exit);

MODULE_AUTHOR("GM Technology Corp."); 
MODULE_LICENSE("GPL"); 
MODULE_DESCRIPTION("sw osg device driver");
