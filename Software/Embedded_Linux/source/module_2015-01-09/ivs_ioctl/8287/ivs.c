#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include <mach/fmem.h>
#include "ivs_ioctl.h"
#include "frammap_if.h"


extern int ivs_init_ioctl(void);
extern int ivs_exit_ioctl(void);

static int __init ivs_init(void)
{			
	if( ivs_init_ioctl()<0 )
		return -1;
		
	if( IVS_VER_BRANCH ) 
	{
    	printk( "IVS Version : %d.%d.%d.%d, built @ %s %s\n", 
			    IVS_VER_MAJOR, IVS_VER_MINOR, IVS_VER_MINOR2, IVS_VER_BRANCH,
			    __DATE__, __TIME__);
    }
	else {
    	printk( "IVS Version : %d.%d.%d, built @ %s %s\n", 
			    IVS_VER_MAJOR, IVS_VER_MINOR, IVS_VER_MINOR2,
			    __DATE__, __TIME__);
	}

	return 0;
}

static void __exit ivs_exit(void)
{
	ivs_exit_ioctl();
}


module_init(ivs_init);
module_exit(ivs_exit);

MODULE_AUTHOR("Grain Media Corp.");
MODULE_LICENSE("Grain Media License");
MODULE_DESCRIPTION("IVS driver");
