/**
 * @file ivs_ioctl.c
 *  GM8139 ivs ioctl driver
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.01 $
 * $Date: 2014/06/16 09:52:53 $
 *
 * ChangeLog:
 *  $Log: ivs_ioctl.c $
 *  Revision 1.01  2014/06/16 tire_ling
 *  1. support selected path for .bin
 *  2. change default value of max_width max_height max_ap
 */
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/hdreg.h>    /* HDIO_GETGEO */
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/syscalls.h>
#include <linux/miscdevice.h>
#include <mach/fmem.h>
#include <asm/cacheflush.h>		 
#include <mach/platform/board.h>
#include "ivs_ioctl.h"
#include "frammap_if.h"

// Debug
#define IVS_DBG_ENABLE			0

// ivs minor and irq number
#define IVS_MINOR  			    30
#define IVS_IRQ  			    	47

// support ivs number	
// #define IVS_IDX_FULL    	    0xFFFFFFFF
#define IVS_IDX_MASK_n(n) 	    (0x1<<n)
int IVS_IDX_FULL = 0;
static int ivs_idx  = 0;

struct dec_private
{
    int dev;                    //  ID for each decoder open 
};                              //  range from 0 to (MAX_DEC_NUM - 1) 

static struct dec_private dec_data[MAX_IVS_NUM] = {{0}};

//	memory allocate
unsigned int memory_index[32] = {-1};

int max_height = 240;
module_param(max_height, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(max_height, "image height");

int max_width = 320;
module_param(max_width, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(max_width, "image width");

int max_ap = 1;
module_param(max_ap, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(max_ap, "AP number");

int memory_counter = 0;
module_param(memory_counter, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(memory_counter, "memory counter");

// Revision: 1.01
char *bin_path = "/mnt/mtd";
module_param(bin_path, charp, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(bin_path, "set config path");

int IMEM_SIZE = 0 ;
unsigned char *ivs_virt = 0; 
unsigned int   ivs_phy  = 0;

//	ivs clk and control base register
volatile unsigned int *ivs_reg_base=0;
volatile unsigned int *clk_reg_base=0;

//	support irq 
int isr_condition_flag = 0 ;
wait_queue_head_t ivs_queue;

#if (LINUX_VERSION_CODE == KERNEL_VERSION(3,3,0))
static DEFINE_SPINLOCK(avcdec_request_lock); 
#else
static spinlock_t avcdec_request_lock = SPIN_LOCK_UNLOCKED;
#endif

struct semaphore favc_dec_mutex;

void hsi(ivs_param_t *ivs_param)
{   
    //	copy information from frame_info
    int in1_addr   = ivs_param->input_img_1.faddr;
    int in1_offset = ivs_param->input_img_1.offset;    
    int out1_addr  = ivs_param->output_rlt_1.faddr;
    int out2_addr  = ivs_param->output_rlt_2.faddr;
    int out3_addr  = ivs_param->output_rlt_3.faddr;
    int out_offset = ivs_param->output_rlt_1.offset;
    int img_width  = ivs_param->img_info.img_width;
    int img_height = ivs_param->img_info.img_height;
    int img_format = ivs_param->img_info.img_format;
    int swap       = ivs_param->img_info.swap_y_cbcr;

	//	control register
	volatile unsigned int *reg_resolution;
	volatile unsigned int *reg_rchan1_str;
	volatile unsigned int *reg_rchan1_off;
	volatile unsigned int *reg_wchan1_str;
	volatile unsigned int *reg_wchan2_str;
	volatile unsigned int *reg_wchan3_str;
    volatile unsigned int *reg_wchan_off;
	volatile unsigned int *reg_parameter;
	volatile unsigned int *reg_ivs_state;
	volatile unsigned int *reg_control;	
	unsigned int ivs_state ;

	//	set register value
	reg_resolution = ivs_reg_base + (0x08 >> 2);
	*reg_resolution = ((img_height & 0xFFFF) << 16) | ((img_width * img_format) & 0xFFF8);

    reg_rchan1_str = ivs_reg_base + (0x10 >> 2) ;
	*reg_rchan1_str = in1_addr;

    reg_rchan1_off = ivs_reg_base + (0x14 >> 2) ;	
	*reg_rchan1_off = in1_offset * img_format;

    reg_wchan1_str = ivs_reg_base + (0x28 >> 2) ;
    *reg_wchan1_str = (out1_addr & 0xFFFFFFFC) | 1 ;
    ivs_param->output_rlt_1.size = img_width * img_height * 2;
    
    reg_wchan2_str = ivs_reg_base + (0x2c >> 2) ;
    *reg_wchan2_str = (out2_addr & 0xFFFFFFFC) | 0 ;
	ivs_param->output_rlt_2.size = img_width * img_height;

    reg_wchan3_str = ivs_reg_base + (0x30 >> 2) ;
    *reg_wchan3_str = (out3_addr & 0xFFFFFFFC) | 0 ;
	ivs_param->output_rlt_3.size = img_width * img_height;
    
    reg_wchan_off = ivs_reg_base + (0x40 >> 2) ;
    *reg_wchan_off = out_offset;

    reg_parameter = ivs_reg_base + (0x04 >> 2) ;
	*reg_parameter = 0 | (swap & 0x3) ;

	reg_ivs_state = ivs_reg_base + (0x74 >> 2) ;

	reg_control = ivs_reg_base + (0x00 >> 2) ;
    *reg_control = 0 | (7 << 6) | 1 ;		

    ivs_state = *reg_ivs_state ;

#if(IVS_DBG_ENABLE == 1)
    printk("reg_resolution   addr:0x%08X val:%08X\n", (unsigned int)reg_resolution,   *reg_resolution);
	printk("reg_rchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_str,   *reg_rchan1_str);
	printk("reg_rchan1_off   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_off,   *reg_rchan1_off);   
	printk("reg_wchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan1_str,   *reg_wchan1_str);  
	printk("reg_wchan2_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan2_str,   *reg_wchan2_str);  
	printk("reg_wchan3_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan3_str,   *reg_wchan3_str);
    printk("reg_wchan_off    addr:0x%08X val:%08X\n", (unsigned int)reg_wchan_off,    *reg_wchan_off );
	printk("reg_parameter    addr:0x%08X val:%08X\n", (unsigned int)reg_parameter,    *reg_parameter ); 
    printk("reg_control      addr:0x%08X val:%08X\n", (unsigned int)reg_control,      *reg_control   );    
    printk("reg_ivs_state    addr:0x%08X val:%08X\n", (unsigned int)reg_ivs_state,    ivs_state      );    		      	
#endif
}

void rgb(ivs_param_t *ivs_param)
{
	//	copy information from frame_info
    int in1_addr   = ivs_param->input_img_1.faddr;
    int in1_offset = ivs_param->input_img_1.offset;
    int out1_addr  = ivs_param->output_rlt_1.faddr;
    int out2_addr  = ivs_param->output_rlt_2.faddr;
    int out3_addr  = ivs_param->output_rlt_3.faddr;
    int out_offset = ivs_param->output_rlt_1.offset;
    int img_width  = ivs_param->img_info.img_width;
    int img_height = ivs_param->img_info.img_height;
    int img_format = ivs_param->img_info.img_format;
    int swap       = ivs_param->img_info.swap_y_cbcr;
    
	//	control register
	volatile unsigned int *reg_resolution;
	volatile unsigned int *reg_rchan1_str;
	volatile unsigned int *reg_rchan1_off;
	volatile unsigned int *reg_wchan1_str;
	volatile unsigned int *reg_wchan2_str;
	volatile unsigned int *reg_wchan3_str;
    volatile unsigned int *reg_wchan_off;
	volatile unsigned int *reg_parameter;
	volatile unsigned int *reg_ivs_state;
	volatile unsigned int *reg_control;	
	unsigned int ivs_state ;

	//	set register value
	reg_resolution = ivs_reg_base + (0x08 >> 2) ;
	*reg_resolution = ((img_height & 0xFFFF) << 16) | ((img_width * img_format) & 0xFFF8);

    reg_rchan1_str = ivs_reg_base + (0x10 >> 2) ;
	*reg_rchan1_str = in1_addr;

    reg_rchan1_off = ivs_reg_base + (0x14 >> 2) ;	
	*reg_rchan1_off = in1_offset * img_format;

    reg_wchan1_str = ivs_reg_base + (0x28 >> 2) ;
    *reg_wchan1_str = (out1_addr & 0xFFFFFFFC) | 0 ;
    ivs_param->output_rlt_1.size = img_height * img_width;

    reg_wchan2_str = ivs_reg_base + (0x2c >> 2) ;
    *reg_wchan2_str = (out2_addr & 0xFFFFFFFC) | 0 ;  
    ivs_param->output_rlt_2.size = img_height * img_width;

    reg_wchan3_str = ivs_reg_base + (0x30 >> 2) ;
    *reg_wchan3_str = (out3_addr & 0xFFFFFFFC) | 0 ; 
    ivs_param->output_rlt_3.size = img_height * img_width;
    
	reg_wchan_off = ivs_reg_base + (0x40 >> 2) ;
    *reg_wchan_off = out_offset;

    reg_parameter = ivs_reg_base + (0x04 >> 2) ;
    *reg_parameter = 0 | (1 << 9) | (swap & 0x3) ;

	reg_ivs_state = ivs_reg_base + (0x74 >> 2);

	reg_control = ivs_reg_base + (0x00 >> 2);
    *reg_control = 0 | (7 << 6) | 1;		

    ivs_state = *reg_ivs_state ;

#if(IVS_DBG_ENABLE == 1)
	printk("reg_resolution   addr:0x%08X val:%08X\n", (unsigned int)reg_resolution,   *reg_resolution);
	printk("reg_rchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_str,   *reg_rchan1_str);
	printk("reg_rchan1_off   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_off,   *reg_rchan1_off);   
	printk("reg_wchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan1_str,   *reg_wchan1_str);  
	printk("reg_wchan2_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan2_str,   *reg_wchan2_str);  
	printk("reg_wchan3_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan3_str,   *reg_wchan3_str);
	printk("reg_wchan_off    addr:0x%08X val:%08X\n", (unsigned int)reg_wchan_off,    *reg_wchan_off );
	printk("reg_parameter    addr:0x%08X val:%08X\n", (unsigned int)reg_parameter,    *reg_parameter ); 
	printk("reg_control      addr:0x%08X val:%08X\n", (unsigned int)reg_control,      *reg_control   );    
	printk("reg_ivs_state    addr:0x%08X val:%08X\n", (unsigned int)reg_ivs_state,    ivs_state      );    		      	
#endif
}

void integral(ivs_param_t *ivs_param)
{
	//	copy information from frame_info
	int in1_addr   = ivs_param->input_img_1.faddr;
    int in1_offset = ivs_param->input_img_1.offset;    
    int out1_addr  = ivs_param->output_rlt_1.faddr;
    int out_offset = ivs_param->output_rlt_1.offset;
    int img_width  = ivs_param->img_info.img_width;
    int img_height = ivs_param->img_info.img_height;
    int img_format = ivs_param->img_info.img_format;
    int swap       = ivs_param->img_info.swap_y_cbcr;
    int endian     = ivs_param->img_info.swap_endian;

	//	control register
	volatile unsigned int *reg_resolution;
	volatile unsigned int *reg_rchan1_str;
	volatile unsigned int *reg_rchan1_off;
	volatile unsigned int *reg_wchan1_str;
    volatile unsigned int *reg_wchan_off;
	volatile unsigned int *reg_parameter;
	volatile unsigned int *reg_ivs_state;
	volatile unsigned int *reg_control;	
	unsigned int ivs_state ;

	//	set register value
	reg_resolution = ivs_reg_base + (0x08 >> 2) ;
    *reg_resolution = ((img_height & 0xFFFF) << 16) | ((img_width * img_format) & 0xFFF8) ;
        
	reg_rchan1_str = ivs_reg_base + (0x10 >> 2) ;
	*reg_rchan1_str = in1_addr;

	reg_rchan1_off = ivs_reg_base + (0x14 >> 2) ;	
	*reg_rchan1_off = in1_offset * img_format;
	
	reg_wchan1_str = ivs_reg_base + (0x34 >> 2) ;
    *reg_wchan1_str = (out1_addr & 0xFFFFFFFC) | 2 ;
    ivs_param->output_rlt_1.size = img_height * img_width * 4;
    
	reg_wchan_off = ivs_reg_base + (0x40 >> 2) ;
    *reg_wchan_off = out_offset;
			
	reg_parameter = ivs_reg_base + (0x04 >> 2) ;
    *reg_parameter = 0 | ((img_format & 0x1) << 5) | (swap & 0x3) ;

	reg_ivs_state = ivs_reg_base + (0x74 >> 2) ;

	reg_control = ivs_reg_base + (0x00 >> 2) ;
    *reg_control = 0 | (1 << 9) | ((endian & 0x1) << 2) | 1 ;

	ivs_state = *reg_ivs_state ;
	
#if(IVS_DBG_ENABLE == 1)
	printk("reg_resolution   addr:0x%08X val:%08X\n", (unsigned int)reg_resolution,   *reg_resolution);
	printk("reg_rchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_str,   *reg_rchan1_str);
	printk("reg_rchan1_off   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_off,   *reg_rchan1_off);   
	printk("reg_wchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan1_str,   *reg_wchan1_str); 
    printk("reg_wchan_off    addr:0x%08X val:%08X\n", (unsigned int)reg_wchan_off,    *reg_rchan1_off); 
	printk("reg_parameter    addr:0x%08X val:%08X\n", (unsigned int)reg_parameter,    *reg_parameter ); 
	printk("reg_control      addr:0x%08X val:%08X\n", (unsigned int)reg_control,      *reg_control   );    
	printk("reg_ivs_state    addr:0x%08X val:%08X\n", (unsigned int)reg_ivs_state,    ivs_state      );    		      	
#endif
}

void s_integral(ivs_param_t *ivs_param)
{
	//	copy information from frame_info
    int in1_addr   = ivs_param->input_img_1.faddr;
    int in1_offset = ivs_param->input_img_1.offset;
    int out1_addr  = ivs_param->output_rlt_1.faddr;
    int out_offset = ivs_param->output_rlt_1.offset;
    int img_width  = ivs_param->img_info.img_width;
    int img_height = ivs_param->img_info.img_height;
    int img_format = ivs_param->img_info.img_format;
    int swap       = ivs_param->img_info.swap_y_cbcr;
    int endian     = ivs_param->img_info.swap_endian;

	//	control register
	volatile unsigned int *reg_resolution;
	volatile unsigned int *reg_rchan1_str;
	volatile unsigned int *reg_rchan1_off;
	volatile unsigned int *reg_wchan1_str;
    volatile unsigned int *reg_wchan_off;
	volatile unsigned int *reg_parameter;
	volatile unsigned int *reg_ivs_state;
	volatile unsigned int *reg_control;	
	unsigned int ivs_state;

	//	set register value
	reg_resolution = ivs_reg_base + (0x08 >> 2) ;
    *reg_resolution = ((img_height & 0xFFFF) << 16) | ((img_width * img_format) & 0xFFF8) ;

	reg_rchan1_str = ivs_reg_base + (0x10 >> 2) ;
	*reg_rchan1_str = in1_addr;

	reg_rchan1_off = ivs_reg_base + (0x14 >> 2) ;	
	*reg_rchan1_off = in1_offset * img_format;
	
	reg_wchan1_str = ivs_reg_base + (0x38 >> 2) ;
    *reg_wchan1_str = (out1_addr & 0xFFFFFFFC) | 2 ;
    ivs_param->output_rlt_1.size = img_height * img_width * 4 ;

    reg_wchan_off = ivs_reg_base + (0x40 >> 2);
    *reg_wchan_off = out_offset;
    
	reg_parameter = ivs_reg_base + (0x04 >> 2) ;
    *reg_parameter = 0 | ((img_format & 0x1) << 5) | (swap & 0x3) ;
   
	reg_ivs_state = ivs_reg_base + (0x74 >> 2) ;

	reg_control = ivs_reg_base + (0x00 >> 2) ;
    *reg_control = 0 | (1 << 10) | ((endian & 0x1) << 2) | 1 ;
       
	ivs_state = *reg_ivs_state ;
	
#if(IVS_DBG_ENABLE == 1)
	printk("reg_resolution   addr:0x%08X val:%08X\n", (unsigned int)reg_resolution,   *reg_resolution);
	printk("reg_rchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_str,   *reg_rchan1_str);
	printk("reg_rchan1_off   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_off,   *reg_rchan1_off);   
	printk("reg_wchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan1_str,   *reg_wchan1_str);  
    printk("reg_wchan_off    addr:0x%08X val:%08X\n", (unsigned int)reg_wchan_off,    *reg_wchan_off );
    printk("reg_parameter    addr:0x%08X val:%08X\n", (unsigned int)reg_parameter,    *reg_parameter ); 
	printk("reg_control      addr:0x%08X val:%08X\n", (unsigned int)reg_control,      *reg_control   );    
	printk("reg_ivs_state    addr:0x%08X val:%08X\n", (unsigned int)reg_ivs_state,    ivs_state      );    		      	
#endif
}

void de_interleave(ivs_param_t *ivs_param)
{
	//	copy information from frame_info
    int in1_addr   = ivs_param->input_img_1.faddr;
    int in1_offset = ivs_param->input_img_1.offset;
    int out1_addr  = ivs_param->output_rlt_1.faddr;
    int out2_addr  = ivs_param->output_rlt_2.faddr;
    int out_offset = ivs_param->output_rlt_1.offset;
    int img_width  = ivs_param->img_info.img_width;
    int img_height = ivs_param->img_info.img_height;
    int img_format = ivs_param->img_info.img_format;
    int swap       = ivs_param->img_info.swap_y_cbcr;
    
	//	control register
	volatile unsigned int *reg_resolution;
	volatile unsigned int *reg_rchan1_str;
	volatile unsigned int *reg_rchan1_off;
	volatile unsigned int *reg_wchan1_str;
	volatile unsigned int *reg_wchan2_str;    
	volatile unsigned int *reg_wchan_off;
	volatile unsigned int *reg_parameter;
	volatile unsigned int *reg_ivs_state;
	volatile unsigned int *reg_control;	
	unsigned int ivs_state ;

	//	set register value
	reg_resolution = ivs_reg_base + (0x08 >> 2) ;
	*reg_resolution = ((img_height & 0xFFFF) << 16) | ((img_width * img_format) & 0xFFF8);

	reg_rchan1_str = ivs_reg_base + (0x10 >> 2) ;
	*reg_rchan1_str = in1_addr;

	reg_rchan1_off = ivs_reg_base + (0x14 >> 2) ;	
	*reg_rchan1_off = in1_offset * img_format;

    reg_wchan1_str = ivs_reg_base + (0x20 >> 2) ;
	*reg_wchan1_str = (out1_addr & 0xFFFFFFFC) | 0 ;
    ivs_param->output_rlt_1.size = img_height * img_width;
    
	reg_wchan2_str = ivs_reg_base + (0x24 >> 2) ;
    *reg_wchan2_str = (out2_addr & 0xFFFFFFFC) | 1 ;
    ivs_param->output_rlt_2.size = img_height * img_width;
	
	reg_wchan_off = ivs_reg_base + (0x40 >> 2) ;
    *reg_wchan_off = out_offset;
	
	reg_parameter = ivs_reg_base + (0x04 >> 2) ;
	*reg_parameter = 0 | (swap & 0x3) ;

	reg_ivs_state = ivs_reg_base + (0x74 >> 2) ;

	reg_control = ivs_reg_base + (0x00 >> 2) ;	
    *reg_control = 0 | (3 << 4) | 1 ;
	
	ivs_state = *reg_ivs_state ;        

#if(IVS_DBG_ENABLE == 1)
	printk("reg_resolution   addr:0x%08X val:%08X\n", (unsigned int)reg_resolution,   *reg_resolution);
	printk("reg_rchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_str,   *reg_rchan1_str);
	printk("reg_rchan1_off   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_off,   *reg_rchan1_off);   
	printk("reg_wchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan1_str,   *reg_wchan1_str);  
	printk("reg_wchan2_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan2_str,   *reg_wchan2_str);  
    printk("reg_wchan_off    addr:0x%08X val:%08X\n", (unsigned int)reg_wchan_off,    *reg_wchan_off );
    printk("reg_parameter    addr:0x%08X val:%08X\n", (unsigned int)reg_parameter,    *reg_parameter ); 
	printk("reg_control      addr:0x%08X val:%08X\n", (unsigned int)reg_control,      *reg_control   );    
	printk("reg_ivs_state    addr:0x%08X val:%08X\n", (unsigned int)reg_ivs_state,    ivs_state      );    		      	
#endif
}

void histogram(ivs_param_t *ivs_param)
{
    //	copy information from frame_info
    int in1_addr   = ivs_param->input_img_1.faddr;
    int in1_offset = ivs_param->input_img_1.offset;
    int out1_addr  = ivs_param->output_rlt_1.faddr;
    int out_offset = ivs_param->output_rlt_1.offset;
    int img_width  = ivs_param->img_info.img_width;
    int img_height = ivs_param->img_info.img_height;
    int img_format = ivs_param->img_info.img_format;
    int endian     = ivs_param->img_info.swap_endian;
    int swap       = ivs_param->img_info.swap_y_cbcr;

	//	control register
	volatile unsigned int *reg_resolution;
	volatile unsigned int *reg_rchan1_str;
	volatile unsigned int *reg_rchan1_off;
	volatile unsigned int *reg_wchan1_str;
    volatile unsigned int *reg_wchan_off;
	volatile unsigned int *reg_parameter;
	volatile unsigned int *reg_ivs_state;
	volatile unsigned int *reg_control;	
	unsigned int ivs_state;

	//	set register value
	reg_resolution = ivs_reg_base + (0x08 >> 2) ;
    *reg_resolution = ((img_height & 0xFFFF) << 16) | ((img_width * img_format) & 0xFFF8);
    
	reg_rchan1_str = ivs_reg_base + (0x10 >> 2);
	*reg_rchan1_str = in1_addr;

	reg_rchan1_off = ivs_reg_base + (0x14 >> 2);	
	*reg_rchan1_off = in1_offset * img_format;
	
	reg_wchan1_str = ivs_reg_base + (0x24 >> 2) ;
    *reg_wchan1_str = (out1_addr & 0xFFFFFFFC) | 2 ;
    ivs_param->output_rlt_1.size = 1024;
    
	reg_wchan_off = ivs_reg_base + (0x40 >> 2) ;
    *reg_wchan_off = out_offset;
	
	reg_parameter = ivs_reg_base + (0x04 >> 2) ;
    *reg_parameter = 0 | (1 << 8) | ((img_format & 0x1) << 5) | (swap & 0x3) ;
    
	reg_ivs_state = ivs_reg_base + (0x74 >> 2) ;

	reg_control = ivs_reg_base + (0x00 >> 2) ;
    *reg_control = 0 | (1 << 5) |((endian & 0x1) << 2) | 1 ;
	
	ivs_state          = *reg_ivs_state ;        
#if(IVS_DBG_ENABLE == 1)
	printk("reg_resolution   addr:0x%08X val:%08X\n", (unsigned int)reg_resolution,   *reg_resolution);
	printk("reg_rchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_str,   *reg_rchan1_str);
	printk("reg_rchan1_off   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_off,   *reg_rchan1_off);   
	printk("reg_wchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan1_str,   *reg_wchan1_str);  
	printk("reg_wchan_off    addr:0x%08X val:%08X\n", (unsigned int)reg_wchan_off,    *reg_wchan_off );  
	printk("reg_parameter    addr:0x%08X val:%08X\n", (unsigned int)reg_parameter,    *reg_parameter ); 
	printk("reg_control      addr:0x%08X val:%08X\n", (unsigned int)reg_control,      *reg_control   );    
	printk("reg_ivs_state    addr:0x%08X val:%08X\n", (unsigned int)reg_ivs_state,    ivs_state      );    		      	
#endif
}

void convolution(ivs_param_t *ivs_param)
{    
    //	copy information from frame_info   
    int in1_addr   = ivs_param->input_img_1.faddr;
    int in1_offset = ivs_param->input_img_1.offset;
	int out1_addr  = ivs_param->output_rlt_1.faddr;
    int out2_addr  = ivs_param->output_rlt_2.faddr;
    int out3_addr  = ivs_param->output_rlt_3.faddr; 
    int out_offset = ivs_param->output_rlt_1.offset;
    int img_width  = ivs_param->img_info.img_width;
	int img_height = ivs_param->img_info.img_height;
	int endian     = ivs_param->img_info.swap_endian;
    int threshold  = ivs_param->operation_info.threshold;
    int shift      = ivs_param->operation_info.shifted_convolution_sum;
    int t_num      = ivs_param->operation_info.kernel_template_index;
    int tile       = ivs_param->operation_info.convolution_tile;
        	
	//  register
	volatile unsigned int *reg_resolution;
	volatile unsigned int *reg_rchan1_str;
	volatile unsigned int *reg_rchan1_off;
	volatile unsigned int *reg_wchan1_str;
	volatile unsigned int *reg_wchan2_str;
	volatile unsigned int *reg_wchan3_str;
    volatile unsigned int *reg_wchan_off;
    volatile unsigned int *reg_filer_mask0;
    volatile unsigned int *reg_filer_mask1;
    volatile unsigned int *reg_filer_mask2;
    volatile unsigned int *reg_filer_mask3;
    volatile unsigned int *reg_filer_mask4;
    volatile unsigned int *reg_filer_mask5;
    volatile unsigned int *reg_filer_mask6;
    volatile unsigned int *reg_parameter;
	volatile unsigned int *reg_ivs_state;
	volatile unsigned int *reg_control;	
	unsigned int ivs_state;

	//  set register
	reg_resolution  = ivs_reg_base + (0x08 >> 2) ;
	*reg_resolution = ((img_height & 0xFFFF) << 16) | (img_width & 0xFFF8);

    reg_rchan1_str = ivs_reg_base + (0x10 >> 2) ;
	*reg_rchan1_str = in1_addr;

    reg_rchan1_off = ivs_reg_base + (0x14 >> 2) ;	
	*reg_rchan1_off = in1_offset;

    reg_wchan1_str = ivs_reg_base + (0x28 >> 2) ;   
    if (shift == 0)
	    *reg_wchan1_str = (out1_addr & 0xFFFFFFFC) | 1 ;
    else
        *reg_wchan1_str = (out1_addr & 0xFFFFFFFC) | 0 ;
	if (shift == 0)
        ivs_param->output_rlt_1.size = img_height * img_width * 2;
	else
		ivs_param->output_rlt_1.size = img_height * img_width;
	
	reg_wchan2_str = ivs_reg_base + (0x2c >> 2) ;
	*reg_wchan2_str = (out2_addr & 0xFFFFFFFC) | 1 ;
	ivs_param->output_rlt_2.size = img_height * img_width * 2;
	
   	reg_wchan3_str = ivs_reg_base + (0x30 >> 2) ;
	*reg_wchan3_str = (out3_addr & 0xFFFFFFFC) | 1 ;
	if (threshold == 0)
		ivs_param->output_rlt_3.size = img_height * img_width * 2;
	else
		ivs_param->output_rlt_3.size = img_height * img_width / 8;

    reg_wchan_off = ivs_reg_base + (0x40 >> 2) ;
    *reg_wchan_off = out_offset;
	
    reg_filer_mask0 = ivs_reg_base + ((0x80 + 0x20 * t_num) >> 2) ;
    *reg_filer_mask0 = 0 | ((ivs_param->operation_info.template_element[0] & 0xFF) << 0 )
                         | ((ivs_param->operation_info.template_element[1] & 0xFF) << 8 )
                         | ((ivs_param->operation_info.template_element[2] & 0xFF) << 16)
                         | ((ivs_param->operation_info.template_element[3] & 0xFF) << 24);

    reg_filer_mask1 = ivs_reg_base + ((0x84 + 0x20 * t_num) >> 2) ;
    *reg_filer_mask1 = 0 | ((ivs_param->operation_info.template_element[4] & 0xFF) << 0 )
                         | ((ivs_param->operation_info.template_element[5] & 0xFF) << 8 )
                         | ((ivs_param->operation_info.template_element[6] & 0xFF) << 16)
                         | ((ivs_param->operation_info.template_element[7] & 0xFF) << 24);  
    
    reg_filer_mask2 = ivs_reg_base + ((0x88 + 0x20 * t_num) >> 2) ;
    *reg_filer_mask2 = 0 | ((ivs_param->operation_info.template_element[8] & 0xFF) << 0 )
                         | ((ivs_param->operation_info.template_element[9] & 0xFF) << 8 )
                         | ((ivs_param->operation_info.template_element[10] & 0xFF) << 16)
                         | ((ivs_param->operation_info.template_element[11] & 0xFF) << 24);  
   
    reg_filer_mask3 = ivs_reg_base + ((0x8c + 0x20 * t_num) >> 2) ;
    *reg_filer_mask3 = 0 | ((ivs_param->operation_info.template_element[12] & 0xFF) << 0 )
                         | ((ivs_param->operation_info.template_element[13] & 0xFF) << 8 )
                         | ((ivs_param->operation_info.template_element[14] & 0xFF) << 16)
                         | ((ivs_param->operation_info.template_element[15] & 0xFF) << 24);  

    reg_filer_mask4 = ivs_reg_base + ((0x90 + 0x20 * t_num) >> 2) ;
    *reg_filer_mask4 = 0 | ((ivs_param->operation_info.template_element[16] & 0xFF) << 0 )
                         | ((ivs_param->operation_info.template_element[17] & 0xFF) << 8 )
                         | ((ivs_param->operation_info.template_element[18] & 0xFF) << 16)
                         | ((ivs_param->operation_info.template_element[19] & 0xFF) << 24);  
    
    reg_filer_mask5 = ivs_reg_base + ((0x94 + 0x20 * t_num) >> 2) ;
    *reg_filer_mask5 = 0 | ((ivs_param->operation_info.template_element[20] & 0xFF) << 0 )
                         | ((ivs_param->operation_info.template_element[21] & 0xFF) << 8 )
                         | ((ivs_param->operation_info.template_element[22] & 0xFF) << 16)
                         | ((ivs_param->operation_info.template_element[23] & 0xFF) << 24);  
        
    reg_filer_mask6 = ivs_reg_base + ((0x98 + 0x20 * t_num) >> 2) ;
    *reg_filer_mask6 = 0 | ((ivs_param->operation_info.template_element[24] & 0xFF) << 0 )
                         | ((shift & 0xFF) << 8);
    
    reg_parameter = ivs_reg_base + (0x04 >> 2) ;
    *reg_parameter = 0 | ((threshold & 0xFFFF) << 16) | ((t_num & 0x7) << 12)
                       | ((tile & 0x1) << 6) | (1 << 5);
    
	reg_ivs_state = ivs_reg_base + (0x74 >> 2);

	reg_control = ivs_reg_base + (0x00 >> 2);
    *reg_control = 0 | ((endian & 0x1) << 2) | (7 << 6) | 1;		

    ivs_state = *reg_ivs_state ;

#if(IVS_DBG_ENABLE == 1)
	printk("reg_resolution   addr:0x%08X val:%08X\n", (unsigned int)reg_resolution,   *reg_resolution);
	printk("reg_rchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_str,   *reg_rchan1_str);
	printk("reg_rchan1_off   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_off,   *reg_rchan1_off);   
	printk("reg_wchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan1_str,   *reg_wchan1_str);  
	printk("reg_wchan2_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan2_str,   *reg_wchan2_str);  
	printk("reg_wchan3_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan3_str,   *reg_wchan3_str);
    printk("reg_wchan_off    addr:0x%08X val:%08X\n", (unsigned int)reg_wchan_off,    *reg_wchan_off );
	printk("reg_parameter    addr:0x%08X val:%08X\n", (unsigned int)reg_parameter,    *reg_parameter ); 
	printk("reg_control      addr:0x%08X val:%08X\n", (unsigned int)reg_control,      *reg_control   );    
	printk("reg_ivs_state    addr:0x%08X val:%08X\n", (unsigned int)reg_ivs_state,    ivs_state      );   

    printk("reg_filer_mask0  addr:0x%08X val:%08X\n", (unsigned int)reg_filer_mask0,  *reg_filer_mask0);   
    printk("reg_filer_mask1  addr:0x%08X val:%08X\n", (unsigned int)reg_filer_mask1,  *reg_filer_mask1);   
    printk("reg_filer_mask2  addr:0x%08X val:%08X\n", (unsigned int)reg_filer_mask2,  *reg_filer_mask2);   
    printk("reg_filer_mask3  addr:0x%08X val:%08X\n", (unsigned int)reg_filer_mask3,  *reg_filer_mask3);   
    printk("reg_filer_mask4  addr:0x%08X val:%08X\n", (unsigned int)reg_filer_mask4,  *reg_filer_mask4);   
    printk("reg_filer_mask5  addr:0x%08X val:%08X\n", (unsigned int)reg_filer_mask5,  *reg_filer_mask5);   
    printk("reg_filer_mask6  addr:0x%08X val:%08X\n", (unsigned int)reg_filer_mask6,  *reg_filer_mask6); 
#endif
}

void morphology(ivs_param_t *ivs_param)
{
    //  copy information from frame_info    
    int in1_addr   = ivs_param->input_img_1.faddr;
    int in1_offset = ivs_param->input_img_1.offset;    
    int out1_addr  = ivs_param->output_rlt_1.faddr;
    int out_offset = ivs_param->output_rlt_1.offset;
    int img_width  = ivs_param->img_info.img_width;
	int img_height = ivs_param->img_info.img_height;
	int img_format = ivs_param->img_info.img_format;
	int endian     = ivs_param->img_info.swap_endian;
    int mop        = ivs_param->operation_info.morphology_operator ;
    int sto        = ivs_param->operation_info.select_threshold_output ;
    int threshold  = ivs_param->operation_info.threshold ;
    int t_num      = ivs_param->operation_info.kernel_template_index ;

	//	control register
	volatile unsigned int *reg_resolution;
	volatile unsigned int *reg_rchan1_str;
	volatile unsigned int *reg_rchan1_off;
	volatile unsigned int *reg_wchan1_str;
    volatile unsigned int *reg_wchan_off;
    volatile unsigned int *reg_parameter ;
	volatile unsigned int *reg_ivs_state ;
	volatile unsigned int *reg_control   ;
    
    volatile unsigned int *reg_filer_mask0;
    volatile unsigned int *reg_filer_mask1;
    volatile unsigned int *reg_filer_mask2;
    volatile unsigned int *reg_filer_mask3;
    volatile unsigned int *reg_filer_mask4;
    volatile unsigned int *reg_filer_mask5;
    volatile unsigned int *reg_filer_mask6;
	unsigned int ivs_state;

	//	set register value
	reg_resolution = ivs_reg_base + (0x08 >> 2) ;
	*reg_resolution = ((img_height & 0xFFFF) << 16) | (img_width & 0xFFF8);

    reg_rchan1_str = ivs_reg_base + (0x10 >> 2) ;
	*reg_rchan1_str = in1_addr;

    reg_rchan1_off = ivs_reg_base + (0x14 >> 2) ;
    if (img_format == 0)
	    *reg_rchan1_off = in1_offset / 8;
    else
        *reg_rchan1_off = in1_offset;

    reg_wchan1_str = ivs_reg_base + (0x30 >> 2) ;
    *reg_wchan1_str = (out1_addr & 0xFFFFFFFC) ;
    if ((sto == 0) || (img_format == 0))
        ivs_param->output_rlt_1.size = img_height * img_width / 8 ;
    else
        ivs_param->output_rlt_1.size = img_height * img_width ;

    reg_wchan_off = ivs_reg_base + (0x40 >> 2) ;
    *reg_wchan_off = out_offset;
    
    reg_filer_mask0 = ivs_reg_base + ((0x80 + 0x20 * t_num) >> 2) ;
    *reg_filer_mask0 = 0 | ((ivs_param->operation_info.template_element[0] & 0xFF) << 0 )
                         | ((ivs_param->operation_info.template_element[1] & 0xFF) << 8 )
                         | ((ivs_param->operation_info.template_element[2] & 0xFF) << 16)
                         | ((ivs_param->operation_info.template_element[3] & 0xFF) << 24);

    reg_filer_mask1 = ivs_reg_base + ((0x84 + 0x20 * t_num) >> 2) ;
    *reg_filer_mask1 = 0 | ((ivs_param->operation_info.template_element[4] & 0xFF) << 0 )
                         | ((ivs_param->operation_info.template_element[5] & 0xFF) << 8 )
                         | ((ivs_param->operation_info.template_element[6] & 0xFF) << 16)
                         | ((ivs_param->operation_info.template_element[7] & 0xFF) << 24);  
    
    reg_filer_mask2 = ivs_reg_base + ((0x88 + 0x20 * t_num) >> 2) ;
    *reg_filer_mask2 = 0 | ((ivs_param->operation_info.template_element[8] & 0xFF) << 0 )
                         | ((ivs_param->operation_info.template_element[9] & 0xFF) << 8 )
                         | ((ivs_param->operation_info.template_element[10] & 0xFF) << 16)
                         | ((ivs_param->operation_info.template_element[11] & 0xFF) << 24);  
   
    reg_filer_mask3 = ivs_reg_base + ((0x8c + 0x20 * t_num) >> 2) ;
    *reg_filer_mask3 = 0 | ((ivs_param->operation_info.template_element[12] & 0xFF) << 0 )
                         | ((ivs_param->operation_info.template_element[13] & 0xFF) << 8 )
                         | ((ivs_param->operation_info.template_element[14] & 0xFF) << 16)
                         | ((ivs_param->operation_info.template_element[15] & 0xFF) << 24);  

    reg_filer_mask4 = ivs_reg_base + ((0x90 + 0x20 * t_num) >> 2) ;
    *reg_filer_mask4 = 0 | ((ivs_param->operation_info.template_element[16] & 0xFF) << 0 )
                         | ((ivs_param->operation_info.template_element[17] & 0xFF) << 8 )
                         | ((ivs_param->operation_info.template_element[18] & 0xFF) << 16)
                         | ((ivs_param->operation_info.template_element[19] & 0xFF) << 24);  
    
    reg_filer_mask5 = ivs_reg_base + ((0x94 + 0x20 * t_num) >> 2) ;
    *reg_filer_mask5 = 0 | ((ivs_param->operation_info.template_element[20] & 0xFF) << 0 )
                         | ((ivs_param->operation_info.template_element[21] & 0xFF) << 8 )
                         | ((ivs_param->operation_info.template_element[22] & 0xFF) << 16)
                         | ((ivs_param->operation_info.template_element[23] & 0xFF) << 24);  
        
    reg_filer_mask6 = ivs_reg_base + ((0x98 + 0x20 * t_num) >> 2) ;
    *reg_filer_mask6 = 0 | ((ivs_param->operation_info.template_element[24] & 0xFF) << 0 );

    reg_parameter = ivs_reg_base + (0x04 >> 2) ;
    if (img_format == 0)
        *reg_parameter = 0 | (0 << 24) | (1 << 16) | ((t_num & 0x7) << 12) 
                           | (1 << 10) | (1 << 7) | (1 << 5) | ((mop & 0x7) << 2) ;
    else
        *reg_parameter = 0 | ((sto & 0x1) << 24) | ((threshold & 0xFF) << 16) | ((t_num & 0x7) << 12) 
                           | (1 << 10) | (1 << 5) | ((mop & 0x7) << 2) ;
   
	reg_ivs_state = ivs_reg_base + (0x74 >> 2);

	reg_control = ivs_reg_base + (0x00 >> 2);
    *reg_control = 0 | (1 << 8) | ((endian & 0x1) << 2) | 1 ;

    ivs_state          = *reg_ivs_state ;

#if(IVS_DBG_ENABLE == 1)
	printk("reg_resolution   addr:0x%08X val:%08X\n", (unsigned int)reg_resolution,   *reg_resolution);
	printk("reg_rchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_str,   *reg_rchan1_str);
	printk("reg_rchan1_off   addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_off,   *reg_rchan1_off);   
	printk("reg_wchan1_str   addr:0x%08X val:%08X\n", (unsigned int)reg_wchan1_str,   *reg_wchan1_str);  
	printk("reg_wchan_off    addr:0x%08X val:%08X\n", (unsigned int)reg_wchan_off,    *reg_wchan_off );  
	printk("reg_parameter    addr:0x%08X val:%08X\n", (unsigned int)reg_parameter,    *reg_parameter ); 
	printk("reg_control      addr:0x%08X val:%08X\n", (unsigned int)reg_control,      *reg_control   );    
	printk("reg_ivs_state    addr:0x%08X val:%08X\n", (unsigned int)reg_ivs_state,    ivs_state      );    		      	
#endif
}

void sad(ivs_param_t *ivs_param)
{
    //	copy information from ivs_param    
    int in1_addr   = ivs_param->input_img_1.faddr;    
    int in1_offset = ivs_param->input_img_1.offset;
    int in2_addr   = ivs_param->input_img_2.faddr;
    int in2_offset = ivs_param->input_img_2.offset;
    int out1_addr  = ivs_param->output_rlt_1.faddr;
    int out_offset = ivs_param->output_rlt_1.offset;
    int img_width  = ivs_param->img_info.img_width; 
	int img_height = ivs_param->img_info.img_height;
    int endian     = ivs_param->img_info.swap_endian;
    int b_th       = ivs_param->operation_info.threshold ;   
    int blksize    = ivs_param->operation_info.block_size ;
    
	//	control register
	volatile unsigned int *reg_resolution;
	volatile unsigned int *reg_rchan1_str;
	volatile unsigned int *reg_rchan1_off;
	volatile unsigned int *reg_rchan2_str;
	volatile unsigned int *reg_rchan2_off;
	volatile unsigned int *reg_wchan1_str;
    volatile unsigned int *reg_wchan_off;
	volatile unsigned int *reg_parameter;
	volatile unsigned int *reg_ivs_state;
	volatile unsigned int *reg_control;	
	unsigned int ivs_state;

	//	set register value
	reg_resolution = ivs_reg_base + (0x08 >> 2) ;
	*reg_resolution = ((img_height & 0xFFFF) << 16) | (img_width & 0xFFF8);

	reg_rchan1_str = ivs_reg_base + (0x10 >> 2) ;
	*reg_rchan1_str = in1_addr;

	reg_rchan1_off = ivs_reg_base + (0x14 >> 2) ;	
	*reg_rchan1_off = in1_offset;

	reg_rchan2_str = ivs_reg_base + (0x18 >> 2) ;
	*reg_rchan2_str = in2_addr;

	reg_rchan2_off = ivs_reg_base + (0x1c >> 2) ;	
	*reg_rchan2_off = in2_offset;
	
	reg_wchan1_str = ivs_reg_base + (0x20 >> 2) ;
    *reg_wchan1_str = (out1_addr & 0xFFFFFFFC) | 1 ;
    if (b_th == 0)
        ivs_param->output_rlt_1.size = img_height * img_width * 2 ;
    else
        ivs_param->output_rlt_1.size = img_height * img_width / 8 ;

    reg_wchan_off = ivs_reg_base + (0x40 >> 2) ;
    *reg_wchan_off = out_offset;
    	
	reg_parameter = ivs_reg_base + (0x04 >> 2) ;
	*reg_parameter = 0 | ((b_th & 0xFFFF) << 16) | (1 << 5) |((blksize & 0x3) << 2) ;

	reg_ivs_state = ivs_reg_base + (0x74 >> 2) ;

	reg_control = ivs_reg_base + (0x00 >> 2) ;	
    *reg_control = 0 | (1 << 12) | ((endian & 0x1) << 2) | (1 << 1) | 1 ;

	ivs_state	       = *reg_ivs_state ;		 

#if(IVS_DBG_ENABLE == 1)
	printk("reg_resolution     addr:0x%08X val:%08X\n", (unsigned int)reg_resolution,   *reg_resolution);
	printk("reg_rchan1_str     addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_str,   *reg_rchan1_str);
	printk("reg_rchan1_off     addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_off,   *reg_rchan1_off);	
	printk("reg_rchan2_str     addr:0x%08X val:%08X\n", (unsigned int)reg_rchan2_str,   *reg_rchan2_str);
	printk("reg_rchan2_off     addr:0x%08X val:%08X\n", (unsigned int)reg_rchan2_off,   *reg_rchan2_off);	
	printk("reg_wchan1_str     addr:0x%08X val:%08X\n", (unsigned int)reg_wchan1_str,   *reg_wchan1_str);	
	printk("reg_wchan_off      addr:0x%08X val:%08X\n", (unsigned int)reg_wchan_off,    *reg_wchan_off);	
	printk("reg_parameter      addr:0x%08X val:%08X\n", (unsigned int)reg_parameter,	*reg_parameter); 
	printk("reg_control        addr:0x%08X val:%08X\n", (unsigned int)reg_control,	    *reg_control);	  
	printk("reg_ivs_state      addr:0x%08X val:%08X\n", (unsigned int)reg_ivs_state,    ivs_state); 				
#endif	
}

void raster(ivs_param_t *ivs_param)
{
    //	copy information from ivs_param
    int in1_addr   = ivs_param->input_img_1.faddr;
    int in1_offset = ivs_param->input_img_1.offset;
    int in2_addr   = ivs_param->input_img_2.faddr;
    int in2_offset = ivs_param->input_img_2.offset;
    int out1_addr  = ivs_param->output_rlt_1.faddr;
    int out_offset = ivs_param->output_rlt_1.offset;
    int img_width  = ivs_param->img_info.img_width;
    int img_height = ivs_param->img_info.img_height;
    int endian     = ivs_param->img_info.swap_endian;
    int raster_op  = ivs_param->operation_info.raster_operationo_code ;
	
	//	control register
	volatile unsigned int *reg_resolution;
	volatile unsigned int *reg_rchan1_str;
	volatile unsigned int *reg_rchan1_off;
	volatile unsigned int *reg_rchan2_str;
	volatile unsigned int *reg_rchan2_off;
	volatile unsigned int *reg_wchan1_str;
    volatile unsigned int *reg_wchan_off;
	volatile unsigned int *reg_parameter;
	volatile unsigned int *reg_ivs_state;
	volatile unsigned int *reg_control;	
	unsigned int ivs_state;

	//	set register value
    reg_resolution = ivs_reg_base + (0x08 >> 2) ;
	*reg_resolution = ((img_height & 0xFFFF) << 16) | (img_width & 0xFFF8);

	reg_rchan1_str = ivs_reg_base + (0x10 >> 2) ;
	*reg_rchan1_str = in1_addr;

	reg_rchan1_off = ivs_reg_base + (0x14 >> 2) ;	
	*reg_rchan1_off = in1_offset ;

	reg_rchan2_str = ivs_reg_base + (0x18 >> 2) ;
    *reg_rchan2_str = in2_addr ;

	reg_rchan2_off = ivs_reg_base + (0x1c >> 2) ;	
	*reg_rchan2_off = in2_offset ;
	
	reg_wchan1_str = ivs_reg_base + (0x24 >> 2) ;
	*reg_wchan1_str = (out1_addr & 0xFFFFFFFC) ;
    ivs_param->output_rlt_1.size = img_height * img_width ;

    reg_wchan_off = ivs_reg_base + (0x40 >> 2) ;
    *reg_wchan_off = out_offset;
    		
	reg_parameter = ivs_reg_base + (0x04 >> 2) ;
	*reg_parameter = 0 | ((raster_op & 0xf) << 12) | (1 << 5); 
   
	reg_ivs_state = ivs_reg_base + (0x74 >> 2) ;

	reg_control = ivs_reg_base + (0x00 >> 2) ;
    *reg_control = 0 | (1 << 13) | ((endian & 0x1) << 2) | (1 << 1) | 1 ;
    
    ivs_state = *reg_ivs_state ;		 

#if(IVS_DBG_ENABLE == 1)
	printk("reg_resolution     addr:0x%08X val:%08X\n", (unsigned int)reg_resolution,   *reg_resolution  );
	printk("reg_rchan1_str     addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_str,   *reg_rchan1_str  );
	printk("reg_rchan1_off     addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_off,   *reg_rchan1_off  );	
	printk("reg_rchan2_str     addr:0x%08X val:%08X\n", (unsigned int)reg_rchan2_str,   *reg_rchan2_str  );
	printk("reg_rchan2_off     addr:0x%08X val:%08X\n", (unsigned int)reg_rchan2_off,   *reg_rchan2_off  );	
	printk("reg_wchan1_str     addr:0x%08X val:%08X\n", (unsigned int)reg_wchan1_str,   *reg_wchan1_str  );	
	printk("reg_wchan_off      addr:0x%08X val:%08X\n", (unsigned int)reg_wchan_off,    *reg_wchan_off);	
	printk("reg_parameter      addr:0x%08X val:%08X\n", (unsigned int)reg_parameter,	*reg_parameter   ); 
	printk("reg_control        addr:0x%08X val:%08X\n", (unsigned int)reg_control,	    *reg_control	 );	  
	printk("reg_ivs_state      addr:0x%08X val:%08X\n", (unsigned int)reg_ivs_state,    ivs_state        ); 				
#endif	
}

void cascade(ivs_param_t *ivs_param)
{
    //  copy information from ivs_param
    int in1_offset = ivs_param->input_img_1.offset;
    int in1_addr   = ivs_param->input_img_1.faddr;
    int in2_offset = ivs_param->input_img_2.offset;
    int in2_addr   = ivs_param->input_img_2.faddr;
    int out_offset = ivs_param->output_rlt_1.offset;
    int out1_addr  = ivs_param->output_rlt_1.faddr;
    int img_width  = ivs_param->img_info.img_width; 
	int img_height = ivs_param->img_info.img_height;
    int img_format = ivs_param->img_info.img_format;
    int endian     = ivs_param->img_info.swap_endian;
    int swap       = ivs_param->img_info.swap_y_cbcr;     
    int ic         = ivs_param->operation_info.integral_cascade;
   
	//	control register
	volatile unsigned int *reg_resolution ;
	volatile unsigned int *reg_rchan1_str ;
	volatile unsigned int *reg_rchan1_off ;
    volatile unsigned int *reg_rchan2_str ;
	volatile unsigned int *reg_rchan2_off ;
	volatile unsigned int *reg_wchan1_str ;
	volatile unsigned int *reg_wchan_off  ;
	volatile unsigned int *reg_parameter  ;
	volatile unsigned int *reg_ivs_state  ;
	volatile unsigned int *reg_control    ;	
	unsigned int ivs_state                ;

	//	set register value
	reg_resolution = ivs_reg_base + (0x08 >> 2) ;	
    *reg_resolution = ((img_height & 0xFFFF) << 16) | ((img_width * img_format) & 0xFFF8);
	
	reg_rchan1_str = ivs_reg_base + (0x10 >> 2) ;
	*reg_rchan1_str = in1_addr;

	reg_rchan1_off = ivs_reg_base + (0x14 >> 2) ;	
	*reg_rchan1_off = in1_offset;

    reg_rchan2_str = ivs_reg_base + (0x18 >> 2) ;
	*reg_rchan2_str = in2_addr;

    reg_rchan2_off = ivs_reg_base + (0x1c >> 2) ;	
	*reg_rchan2_off = in2_offset;
    	
	reg_wchan1_str = ivs_reg_base + (0x34 >> 2) ;
    *reg_wchan1_str = (out1_addr & 0xFFFFFFFC) | 2 ;

    reg_wchan_off = ivs_reg_base + (0x40 >> 2) ;
    *reg_wchan_off = out_offset;
    
	reg_parameter = ivs_reg_base + (0x04 >> 2) ;
	*reg_parameter = 0 | ((ic & 0x1) << 11) | (swap & 0x3) ;
    if (img_format == 2)
        *reg_parameter = *reg_parameter | (0 << 5);
    else
        *reg_parameter = *reg_parameter | (1 << 5);
   
	reg_ivs_state = ivs_reg_base + (0x74 >> 2) ;

	reg_control = ivs_reg_base + (0x00 >> 2) ;
    *reg_control = 0 ; 
    if (ic == 1)
        *reg_control = *reg_control | (1 << 9) ;
    else
        *reg_control = *reg_control | (1 << 17) | (1 << 1);
    *reg_control = *reg_control |((endian & 0x1) << 2) | 1 ;
    
    ivs_state = *reg_ivs_state ;		 

#if(IVS_DBG_ENABLE == 1)
	printk("reg_resolution     addr:0x%08X val:%08X\n", (unsigned int)reg_resolution,   *reg_resolution  );
	printk("reg_rchan1_str     addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_str,   *reg_rchan1_str  );
	printk("reg_rchan1_off     addr:0x%08X val:%08X\n", (unsigned int)reg_rchan1_off,   *reg_rchan1_off  );		
    printk("reg_rchan2_str     addr:0x%08X val:%08X\n", (unsigned int)reg_rchan2_str,   *reg_rchan2_str  );
	printk("reg_rchan2_off     addr:0x%08X val:%08X\n", (unsigned int)reg_rchan2_off,   *reg_rchan2_off  );	
	printk("reg_wchan1_str     addr:0x%08X val:%08X\n", (unsigned int)reg_wchan1_str,   *reg_wchan1_str  );	
	printk("reg_parameter      addr:0x%08X val:%08X\n", (unsigned int)reg_parameter,	*reg_parameter   ); 
	printk("reg_control        addr:0x%08X val:%08X\n", (unsigned int)reg_control,	    *reg_control	 );	  
	printk("reg_ivs_state      addr:0x%08X val:%08X\n", (unsigned int)reg_ivs_state,    ivs_state        ); 				
#endif	
}

int ivs_hsi_conversion(ivs_param_t *ivs_param)
{
	volatile unsigned int *reg_sys_rst;
	volatile unsigned int *reg_ii_done;	
	unsigned long time ;

	reg_sys_rst = ivs_reg_base + (0x00 >> 2);	
	reg_ii_done = ivs_reg_base + (0x74 >> 2);

	down(&favc_dec_mutex);

    hsi(ivs_param) ;
    wait_event_timeout(ivs_queue, isr_condition_flag, msecs_to_jiffies(3000));     
    if (isr_condition_flag == 0)	//	timeout
    {
#if(IVS_DBG_ENABLE == 1)				
	    printk("hsi conversion isr timeout, reg_ii_done = %d\n",*reg_ii_done) ;
#endif
		//	software reset
		*reg_sys_rst = (1 << 31);	
		*reg_sys_rst = 0;

		//	polling reg_74(ivs status register)
		time = jiffies;
		while((*reg_ii_done == 0) && ((jiffies-time) < 1000)) {}				

		up(&favc_dec_mutex);
		return -5;
	} 
	else 						//	interrupt		
	{		
        isr_condition_flag = 0;             
#if(IVS_DBG_ENABLE == 1)				
	    printk("hsi conversion receive isr ,reg_ii_done = %d\n",*reg_ii_done) ;
#endif
	}

    up(&favc_dec_mutex);
    return 0;
}

int ivs_rgb_conversion(ivs_param_t *ivs_param)
{
	volatile unsigned int *reg_sys_rst;
	volatile unsigned int *reg_ii_done;	
	unsigned long time;

	reg_sys_rst = ivs_reg_base + (0x00 >> 2);	
	reg_ii_done = ivs_reg_base + (0x74 >> 2);

	down(&favc_dec_mutex);

    rgb(ivs_param);
    wait_event_timeout(ivs_queue, isr_condition_flag, msecs_to_jiffies(3000));     
    if( isr_condition_flag==0 )	//	timeout
    {
#if(IVS_DBG_ENABLE == 1)				
	    printk("rgb conversion isr timeout, reg_ii_done = %d\n",*reg_ii_done) ;
#endif
		//	software reset
		*reg_sys_rst = (1<<31) ;	
		*reg_sys_rst = 0;

		//	polling reg_74(ivs status register)
		time = jiffies ;
		while((*reg_ii_done == 0) && ((jiffies-time)<1000)) {}				

		up(&favc_dec_mutex);
		return -5;
	} 
	else 						//	interrupt		
	{		
        isr_condition_flag = 0;             
#if(IVS_DBG_ENABLE == 1)				
	    printk("rgb conversion receive isr ,reg_ii_done = %d\n",*reg_ii_done) ;
#endif
	}

    up(&favc_dec_mutex);
    return 0;
}

int ivs_integral_image(ivs_param_t *ivs_param)
{
	volatile unsigned int *reg_sys_rst ;
	volatile unsigned int *reg_ii_done ;	
	unsigned long time ;

	reg_sys_rst = ivs_reg_base + (0x00 >> 2);	
	reg_ii_done = ivs_reg_base + (0x74 >> 2);

	down(&favc_dec_mutex);

    integral(ivs_param);
    wait_event_timeout(ivs_queue, isr_condition_flag, msecs_to_jiffies(3000));     
    if( isr_condition_flag==0 )	//	timeout
    {
#if(IVS_DBG_ENABLE == 1)				
	    printk("integral image isr timeout, reg_ii_done = %d\n",*reg_ii_done) ;
#endif
		//	software reset
		*reg_sys_rst = (1<<31) ;	
		*reg_sys_rst = 0;

		//	polling reg_74(ivs status register)
		time = jiffies ;
		while((*reg_ii_done == 0) && ((jiffies-time)<1000)) {}				

		up(&favc_dec_mutex);
		return -5;
	} 
	else 						//	interrupt		
	{		
        isr_condition_flag = 0;             
#if(IVS_DBG_ENABLE == 1)				
	    printk("integral image receive isr ,reg_ii_done = %d\n",*reg_ii_done) ;
#endif
	}

    up(&favc_dec_mutex);
    return 0;
}

int ivs_squared_integral_image(ivs_param_t *ivs_param)
{
	volatile unsigned int *reg_sys_rst;
	volatile unsigned int *reg_ii_done;	
	unsigned long time ;

	reg_sys_rst = ivs_reg_base + (0x00 >> 2);	
	reg_ii_done = ivs_reg_base + (0x74 >> 2);

	down(&favc_dec_mutex);

    s_integral(ivs_param);
    wait_event_timeout(ivs_queue, isr_condition_flag, msecs_to_jiffies(3000));     
    if( isr_condition_flag==0 )	//	timeout
    {
#if(IVS_DBG_ENABLE == 1)				
	    printk("squared integral image isr timeout, reg_ii_done = %d\n",*reg_ii_done) ;
#endif
		//	software reset
		*reg_sys_rst = (1<<31) ;	
		*reg_sys_rst = 0;

		//	polling reg_74(ivs status register)
		time = jiffies ;
		while((*reg_ii_done == 0) && ((jiffies-time)<1000)) {}				

		up(&favc_dec_mutex);
		return -5;
	} 
	else 						//	interrupt		
	{		
        isr_condition_flag = 0;             
#if(IVS_DBG_ENABLE == 1)				
	    printk("squared integral image receive isr ,reg_ii_done = %d\n",*reg_ii_done) ;
#endif
	}

    up(&favc_dec_mutex);
    return 0;
}

int ivs_de_interleaving(ivs_param_t *ivs_param)
{
	volatile unsigned int *reg_sys_rst;
	volatile unsigned int *reg_ii_done;	
	unsigned long time ;

	reg_sys_rst = ivs_reg_base + (0x00 >> 2);	
	reg_ii_done = ivs_reg_base + (0x74 >> 2);

	down(&favc_dec_mutex);

    de_interleave(ivs_param) ;
    wait_event_timeout(ivs_queue, isr_condition_flag, msecs_to_jiffies(3000));     
    if( isr_condition_flag==0 )	//	timeout
    {
#if(IVS_DBG_ENABLE == 1)				
	    printk("de interleaving isr timeout, reg_ii_done = %d\n",*reg_ii_done) ;
#endif
		//	software reset
		*reg_sys_rst = (1<<31) ;	
		*reg_sys_rst = 0;

		//	polling reg_74(ivs status register)
		time = jiffies ;
		while((*reg_ii_done == 0) && ((jiffies-time)<1000)) {}				

		up(&favc_dec_mutex);
		return -5;
	} 
	else 						//	interrupt		
	{		
        isr_condition_flag = 0;             
#if(IVS_DBG_ENABLE == 1)				
	    printk("de interleaving receive isr ,reg_ii_done = %d\n",*reg_ii_done) ;
#endif
	}

    up(&favc_dec_mutex);
    return 0;
}

int ivs_histogram(ivs_param_t *ivs_param)
{
	volatile unsigned int *reg_sys_rst;
	volatile unsigned int *reg_ii_done;	
	unsigned long time ;

	reg_sys_rst = ivs_reg_base + (0x00 >> 2) ;	
	reg_ii_done = ivs_reg_base + (0x74 >> 2) ;

	down(&favc_dec_mutex);

    histogram(ivs_param) ;
    wait_event_timeout(ivs_queue, isr_condition_flag, msecs_to_jiffies(3000));     
    if( isr_condition_flag==0 )	//	timeout
    {
#if(IVS_DBG_ENABLE == 1)				
	    printk("histogram isr timeout, reg_ii_done = %d\n",*reg_ii_done) ;
#endif
		//	software reset
		*reg_sys_rst = (1<<31) ;	
		*reg_sys_rst = 0;

		//	polling reg_74(ivs status register)
		time = jiffies ;
		while((*reg_ii_done == 0) && ((jiffies-time)<1000)) {}				

		up(&favc_dec_mutex);
		return -5;
	} 
	else 						//	interrupt		
	{		
        isr_condition_flag = 0;             
#if(IVS_DBG_ENABLE == 1)				
	    printk("histogram receive isr ,reg_ii_done = %d\n",*reg_ii_done) ;
#endif
	}

    up(&favc_dec_mutex);
    return 0;
}

int ivs_convolution_t(ivs_param_t *ivs_param)
{
    volatile unsigned int *reg_sys_rst;
    volatile unsigned int *reg_ii_done; 
    unsigned long time ;

    reg_sys_rst = ivs_reg_base + (0x00 >> 2) ;  
    reg_ii_done = ivs_reg_base + (0x74 >> 2) ;

    down(&favc_dec_mutex);

    convolution(ivs_param) ;
    wait_event_timeout(ivs_queue, isr_condition_flag, msecs_to_jiffies(3000));     
    if( isr_condition_flag==0 ) //  timeout
    {
#if(IVS_DBG_ENABLE == 1)				
        printk("convolution isr timeout, reg_ii_done = %d\n",*reg_ii_done) ;
#endif
        //  software reset
        *reg_sys_rst = (1<<31) ;    
        *reg_sys_rst = 0;

        //  polling reg_74(ivs status register)
        time = jiffies ;
        while((*reg_ii_done == 0) && ((jiffies-time)<1000)) {}             

        up(&favc_dec_mutex);
        return -5;
    } 
    else                        //  interrupt       
    {       
        isr_condition_flag = 0;             
#if(IVS_DBG_ENABLE == 1)				
        printk("convolution receive isr ,reg_ii_done = %d\n",*reg_ii_done) ;
#endif
    }
    
    up(&favc_dec_mutex);
    return 0;
}

int ivs_convolution(ivs_param_t *ivs_param)
{
    int img_width  = ivs_param->img_info.img_width;
    int img_height = ivs_param->img_info.img_height;
    int shift      = ivs_param->operation_info.shifted_convolution_sum;
    int threshold  = ivs_param->operation_info.threshold;

    if (img_width > 2048) {
        ivs_param->img_info.img_width = 1936;
        ivs_param->input_img_1.offset = img_width - 1936;
        ivs_param->output_rlt_1.offset = img_width - 1936; 
        ivs_param->operation_info.convolution_tile = 0;
        ivs_convolution_t(ivs_param);

        ivs_param->input_img_1.faddr += 1920;
        if (shift == 0) 
            ivs_param->output_rlt_1.faddr += 1920 * 2;
        else
            ivs_param->output_rlt_1.faddr += 1920;
        ivs_param->output_rlt_2.faddr += 1920 * 2;
        if (threshold == 0)
            ivs_param->output_rlt_3.faddr += 1920 * 2;
        else
            ivs_param->output_rlt_3.faddr += 1920 / 8;
            
        ivs_param->img_info.img_width = img_width - 1920;
        ivs_param->input_img_1.offset = 1920;
        ivs_param->output_rlt_1.offset = 1920;
        ivs_param->operation_info.convolution_tile = 1;
        ivs_convolution_t(ivs_param);

        if (shift == 0)
           ivs_param->output_rlt_1.size = img_height * img_width * 2;
        else
           ivs_param->output_rlt_1.size = img_height * img_width;
       
        ivs_param->output_rlt_2.size = img_height * img_width * 2;
       
        if (threshold == 0)
           ivs_param->output_rlt_3.size = img_height * img_width * 2;
        else
           ivs_param->output_rlt_3.size = img_height * img_width / 8;
    } else {
        ivs_convolution_t(ivs_param);
    }

    return 0;
}

int ivs_morphology(ivs_param_t *ivs_param)
{
	volatile unsigned int *reg_sys_rst;
	volatile unsigned int *reg_ii_done;	
	unsigned long time ;

	reg_sys_rst = ivs_reg_base + (0x00 >> 2) ;	
	reg_ii_done = ivs_reg_base + (0x74 >> 2) ;

	down(&favc_dec_mutex);

    morphology(ivs_param);
    wait_event_timeout(ivs_queue, isr_condition_flag, msecs_to_jiffies(3000));     
    if( isr_condition_flag==0 )	//	timeout
    {
#if(IVS_DBG_ENABLE == 1)				
	    printk("morphology isr timeout, reg_ii_done = %d\n",*reg_ii_done) ;
#endif
		//	software reset
		*reg_sys_rst = (1<<31) ;	
		*reg_sys_rst = 0;

		//	polling reg_74(ivs status register)
		time = jiffies ;
		while((*reg_ii_done == 0) && ((jiffies-time)<1000)) {}				

		up(&favc_dec_mutex);
		return -5;
	} 
	else 						//	interrupt		
	{		
        isr_condition_flag = 0;             
#if(IVS_DBG_ENABLE == 1)				
	    printk("morphology receive isr ,reg_ii_done = %d\n",*reg_ii_done) ;
#endif
	}

    up(&favc_dec_mutex);
    return 0;
}

int ivs_sad(ivs_param_t *ivs_param)
{
	volatile unsigned int *reg_sys_rst;
	volatile unsigned int *reg_ii_done;	
	unsigned long time ;

	reg_sys_rst = ivs_reg_base + (0x00 >> 2) ;	
	reg_ii_done = ivs_reg_base + (0x74 >> 2) ;

	down(&favc_dec_mutex);

    sad(ivs_param);
    wait_event_timeout(ivs_queue, isr_condition_flag, msecs_to_jiffies(3000));     
    if( isr_condition_flag==0 )	//	timeout
    {
#if(IVS_DBG_ENABLE == 1)				
	    printk("sad isr timeout, reg_ii_done = %d\n",*reg_ii_done) ;
#endif
		//	software reset
		*reg_sys_rst = (1<<31) ;	
		*reg_sys_rst = 0;

		//	polling reg_74(ivs status register)
		time = jiffies ;
		while((*reg_ii_done == 0) && ((jiffies-time)<1000)) {}				

		up(&favc_dec_mutex);
		return -5;
	} 
	else 						//	interrupt		
	{		
        isr_condition_flag = 0;             
#if(IVS_DBG_ENABLE == 1)				
	    printk("sad receive isr ,reg_ii_done = %d\n",*reg_ii_done) ;
#endif
	}

    up(&favc_dec_mutex);
    return 0;
}

int ivs_raster(ivs_param_t *ivs_param)
{
	volatile unsigned int *reg_sys_rst;
	volatile unsigned int *reg_ii_done;	
	unsigned long time ;

	reg_sys_rst = ivs_reg_base + (0x00 >> 2);	
	reg_ii_done = ivs_reg_base + (0x74 >> 2);

	down(&favc_dec_mutex);

    raster(ivs_param) ;
    wait_event_timeout(ivs_queue, isr_condition_flag, msecs_to_jiffies(3000));     
    if( isr_condition_flag==0 )	//	timeout
    {
#if(IVS_DBG_ENABLE == 1)				
	    printk("raster operation isr timeout, reg_ii_done = %d\n",*reg_ii_done) ;
#endif
		//	software reset
		*reg_sys_rst = (1<<31) ;	
		*reg_sys_rst = 0;

		//	polling reg_74(ivs status register)
		time = jiffies ;
		while((*reg_ii_done == 0) && ((jiffies-time)<1000)) {}				

		up(&favc_dec_mutex);
		return -5;
	} 
	else 						//	interrupt		
	{		
        isr_condition_flag = 0;             
#if(IVS_DBG_ENABLE == 1)				
	    printk("raster operation receive isr ,reg_ii_done = %d\n",*reg_ii_done) ;
#endif
	}

    up(&favc_dec_mutex);
    return 0;
}

int ivs_cascaded_classifier(ivs_param_t *ivs_param)
{
    volatile unsigned int *reg_fd_num;
	volatile unsigned int *reg_sys_rst;
	volatile unsigned int *reg_ii_done;	
	unsigned long time ;

    reg_fd_num  = ivs_reg_base + (0x70 >> 2);
	reg_sys_rst = ivs_reg_base + (0x00 >> 2);	
	reg_ii_done = ivs_reg_base + (0x74 >> 2);

	down(&favc_dec_mutex);

    cascade(ivs_param) ;
    wait_event_timeout(ivs_queue, isr_condition_flag, msecs_to_jiffies(3000));     

    if( isr_condition_flag==0 )	//	timeout
    {
#if(IVS_DBG_ENABLE == 1)				
	    printk("cascaded classifier isr timeout, reg_ii_done = %d\n",*reg_ii_done) ;
#endif
		//	software reset
		*reg_sys_rst = (1<<31) ;	
		*reg_sys_rst = 0;

		//	polling reg_74(ivs status register)
		time = jiffies ;
		while((*reg_ii_done==0) && ((jiffies-time)<1000)) {}				

		up(&favc_dec_mutex);
		return -5;
	} 
	else 						//	interrupt		
	{		
        isr_condition_flag = 0;             
#if(IVS_DBG_ENABLE == 1)				
	    printk("cascaded classifier receive isr ,reg_ii_done = %d\n",*reg_ii_done) ;
#endif
	}
    ivs_param->output_rlt_1.size = (*reg_fd_num & 0xFFFF) * 4 ;
    
    up(&favc_dec_mutex);
    return 0;
}

int data_transfer(ivs_param_t* ivs_param,ivs_ioctl_param_t *ivs_ioctl_param,struct file *filp)
{
    int offset = 0;
    struct dec_private *decp = filp->private_data;
    int dev = decp->dev;
    down(&favc_dec_mutex);
    
    offset = dev * PAGE_ALIGN(max_width * max_height * 10) ; 

    if (ivs_param == NULL) {
        printk("ivs_param is NULL\n");
        up(&favc_dec_mutex);
        return -4;
    }

    if (ivs_ioctl_param == NULL) {        
        printk("ivs_ioctl_param is NULL\n");
        up(&favc_dec_mutex);
        return -4;
    }
   
    ivs_param->img_info       = ivs_ioctl_param->img_info;
	ivs_param->operation_info = ivs_ioctl_param->operation_info;
	
	ivs_param->input_img_1.offset = ivs_ioctl_param->input_img_1.offset;
    ivs_param->input_img_1.faddr  = ivs_phy + offset +
                                    ivs_ioctl_param->input_img_1.addr * 
		                            ivs_ioctl_param->mem_info.memory_size;
    ivs_param->input_img_1.vaddr  = ivs_virt + offset +
                                    ivs_ioctl_param->input_img_1.addr * 
		                            ivs_ioctl_param->mem_info.memory_size;

    ivs_param->input_img_2.offset = ivs_ioctl_param->input_img_2.offset;
	ivs_param->input_img_2.faddr  = ivs_phy + offset +
                                    ivs_ioctl_param->input_img_2.addr * 
		                            ivs_ioctl_param->mem_info.memory_size;
	ivs_param->input_img_2.vaddr  = ivs_virt + offset +
                                    ivs_ioctl_param->input_img_2.addr * 
		                            ivs_ioctl_param->mem_info.memory_size;

    ivs_param->output_rlt_1.offset = ivs_ioctl_param->output_rlt_1.offset;
    ivs_param->output_rlt_1.faddr  = ivs_phy + offset +
                                     ivs_ioctl_param->output_rlt_1.addr *
		                             ivs_ioctl_param->mem_info.memory_size;
    ivs_param->output_rlt_1.vaddr  = ivs_virt + offset +
                                     ivs_ioctl_param->output_rlt_1.addr *
                                     ivs_ioctl_param->mem_info.memory_size;

    ivs_param->output_rlt_2.offset = ivs_ioctl_param->output_rlt_2.offset;
	ivs_param->output_rlt_2.faddr  = ivs_phy + offset +
                                     ivs_ioctl_param->output_rlt_2.addr *
                                     ivs_ioctl_param->mem_info.memory_size; 
	ivs_param->output_rlt_2.vaddr  = ivs_virt + offset +
                                     ivs_ioctl_param->output_rlt_2.addr *
                                     ivs_ioctl_param->mem_info.memory_size; 

    ivs_param->output_rlt_3.offset = ivs_ioctl_param->output_rlt_3.offset;
	ivs_param->output_rlt_3.faddr  = ivs_phy + offset +
                                     ivs_ioctl_param->output_rlt_3.addr *
                                     ivs_ioctl_param->mem_info.memory_size;
	ivs_param->output_rlt_3.vaddr  = ivs_virt + offset +
                                     ivs_ioctl_param->output_rlt_3.addr *
                                     ivs_ioctl_param->mem_info.memory_size;
    
    up(&favc_dec_mutex);
    return 0;
}

int ioctl_init(unsigned int API_version)
{
    down(&favc_dec_mutex);
    if ((API_version & 0xFFFFFFF0) != (IVS_VER & 0xFFFFFFF0))
    {
        printk("Fail API Version \n");
        up(&favc_dec_mutex);
        return -1;
    }
    up(&favc_dec_mutex);
    return 0;
}

int ioctl_init_mem(ivs_ioctl_param_t *ivs_ioctl_param)
{
    down(&favc_dec_mutex);
    if (ivs_ioctl_param == NULL) {
        printk("ivs_ioctl_param is NULL\n");
        up(&favc_dec_mutex);
        return -1;    
    }
    ivs_ioctl_param->mem_info.memory_size = ivs_ioctl_param->img_info.img_height *
                                            ivs_ioctl_param->img_info.img_width * 2;
    ivs_ioctl_param->mem_info.memory_num = (max_height * max_width * 10)/ 
		                                   ivs_ioctl_param->mem_info.memory_size ;
    up(&favc_dec_mutex);
    return 0;
}

int ioctl_hsi_conversion(ivs_ioctl_param_t *ivs_ioctl_param, struct file *filp)
{
    int ret = 0;
		ivs_param_t ivs_param;

    ret = data_transfer(&ivs_param,ivs_ioctl_param,filp);
    if (ret < 0) {
        printk("data transfer error\n");
        return -4;
    }

    ret = ivs_hsi_conversion(&ivs_param);
    if (ret < 0) {
        printk("ivs_hsi_conversion error\n");
        return -4;
    }

    ivs_ioctl_param->output_rlt_1.size = ivs_param.output_rlt_1.size;
    ivs_ioctl_param->output_rlt_2.size = ivs_param.output_rlt_2.size;
    ivs_ioctl_param->output_rlt_3.size = ivs_param.output_rlt_3.size;
    
    return ret ; 
}

int ioctl_rgb_conversion(ivs_ioctl_param_t *ivs_ioctl_param, struct file *filp)
{
    int ret = 0;
    ivs_param_t ivs_param;
    
    ret = data_transfer(&ivs_param,ivs_ioctl_param,filp);
    if (ret < 0) {
        printk("data transfer error\n");
        return -4;
    }
    
    ret = ivs_rgb_conversion(&ivs_param);

    if (ret < 0) {
        printk("ivs_rgb_conversion error\n");
        return -4;
    }
        
		ivs_ioctl_param->output_rlt_1.size = ivs_param.output_rlt_1.size;
		ivs_ioctl_param->output_rlt_2.size = ivs_param.output_rlt_2.size;
		ivs_ioctl_param->output_rlt_3.size = ivs_param.output_rlt_3.size;

    return ret ; 
}

int ioctl_integral_image(ivs_ioctl_param_t *ivs_ioctl_param, struct file *filp)
{
    int ret = 0;
    ivs_param_t ivs_param;
    
    ret = data_transfer(&ivs_param,ivs_ioctl_param,filp);
    if (ret < 0) {
        printk("data transfer error\n");
        return -4;
    }

    ret = ivs_integral_image(&ivs_param);
    if (ret < 0) {
        printk("ivs_integral_image error\n");
        return -4;
    }

   	ivs_ioctl_param->output_rlt_1.size = ivs_param.output_rlt_1.size;
   
    return ret ; 
}

int ioctl_squared_integral_image(ivs_ioctl_param_t *ivs_ioctl_param, struct file *filp)
{
    int ret = 0;
    ivs_param_t ivs_param;
    
    ret = data_transfer(&ivs_param,ivs_ioctl_param,filp);
    if (ret < 0) {
        printk("data transfer error\n");
        return -4;
    }

    ret = ivs_squared_integral_image(&ivs_param);
    if (ret < 0) {
        printk("ivs_squared_integral_image error\n");
        return -4;
    }

    ivs_ioctl_param->output_rlt_1.size = ivs_param.output_rlt_1.size;
    
    return ret ; 
}

int ioctl_de_interleaving(ivs_ioctl_param_t *ivs_ioctl_param, struct file *filp)
{
    int ret = 0;
    ivs_param_t ivs_param;
    
    ret = data_transfer(&ivs_param,ivs_ioctl_param,filp);
    if (ret < 0) {
        printk("data transfer error\n");
        return -4;
    }
    
    ret = ivs_de_interleaving(&ivs_param);
    if (ret < 0) {
        printk("ivs_de_interleaving error\n");
        return -4;
    }
        
    ivs_ioctl_param->output_rlt_1.size = ivs_param.output_rlt_1.size;
    ivs_ioctl_param->output_rlt_2.size = ivs_param.output_rlt_2.size;

	return ret ; 
}

int ioctl_histogram(ivs_ioctl_param_t *ivs_ioctl_param, struct file *filp)
{
    int ret = 0;
    ivs_param_t ivs_param;
    
    ret = data_transfer(&ivs_param,ivs_ioctl_param,filp);
    if (ret < 0) {
        printk("data transfer error\n");
        return -4;
    }

    ret = ivs_histogram(&ivs_param);
    if (ret < 0) {
        printk("ivs_histogram error\n");
        return -4;
    }

    ivs_ioctl_param->output_rlt_1.size = ivs_param.output_rlt_1.size;
    
    return ret ; 
}

int ioctl_convolution(ivs_ioctl_param_t *ivs_ioctl_param, struct file *filp)
{
    int ret = 0;
    ivs_param_t ivs_param;
    
    ret = data_transfer(&ivs_param,ivs_ioctl_param,filp);
    if (ret < 0) {
        printk("data transfer error\n");
        return -4;
    }

    ret = ivs_convolution(&ivs_param);
    if (ret < 0) {
        printk("ivs_convolution error\n");
        return -4;
    }

    ivs_ioctl_param->output_rlt_1.size = ivs_param.output_rlt_1.size;
    ivs_ioctl_param->output_rlt_2.size = ivs_param.output_rlt_2.size;
    ivs_ioctl_param->output_rlt_3.size = ivs_param.output_rlt_3.size;


    return ret ; 
}

int ioctl_morphology(ivs_ioctl_param_t *ivs_ioctl_param, struct file *filp)
{
    int ret = 0;
    ivs_param_t ivs_param;
    
    ret = data_transfer(&ivs_param,ivs_ioctl_param,filp);
    if (ret < 0) {
        printk("data transfer error\n");
        return -4;
    }

    ret = ivs_morphology(&ivs_param);
    if (ret < 0) {
        printk("ivs_morphology error\n");
        return -4;
    }

    ivs_ioctl_param->output_rlt_1.size = ivs_param.output_rlt_1.size;
   
    return ret ; 
}

int ioctl_sad(ivs_ioctl_param_t *ivs_ioctl_param, struct file *filp)
{
    int ret = 0;
    ivs_param_t ivs_param;
    
    ret = data_transfer(&ivs_param,ivs_ioctl_param,filp);
    if (ret < 0) {
        printk("data transfer error\n");
        return -4;
    }
    
    ret = ivs_sad(&ivs_param);
    if (ret < 0) {
        printk("ivs_sad error\n");
        return -4;
    }
    
    ivs_ioctl_param->output_rlt_1.size = ivs_param.output_rlt_1.size;

    return ret ; 
}

int ioctl_raster(ivs_ioctl_param_t *ivs_ioctl_param, struct file *filp)
{
    int ret = 0;
    ivs_param_t ivs_param;
    
    ret = data_transfer(&ivs_param,ivs_ioctl_param,filp);
    if (ret < 0) {
        printk("data transfer error\n");
        return -4;
    }
    
    ret = ivs_raster(&ivs_param);
    if (ret < 0) {
        printk("ivs_raster error\n");
        return -4;
    }
    
    ivs_ioctl_param->output_rlt_1.size = ivs_param.output_rlt_1.size;
    
    return ret ; 
}

int ioctl_cascaded_classifier(ivs_ioctl_param_t *ivs_ioctl_param, struct file *filp)
{
    int ret = 0;
    ivs_param_t ivs_param;
   
    ret = data_transfer(&ivs_param,ivs_ioctl_param,filp);
    if (ret < 0) {
        printk("data transfer error\n");
        return -4;
    }
    
    ret = ivs_cascaded_classifier(&ivs_param);
    if (ret < 0) {
        printk("ivs_cascaded_classifier error\n");
        return -4;
    }

    ivs_ioctl_param->output_rlt_1.size = ivs_param.output_rlt_1.size;

    return ret ; 

}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
long ivs_ioctl( struct file *filp, unsigned int cmd, unsigned long arg)
#else
int ivs_ioctl( struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	int ret = 0;
	unsigned int API_version;
    ivs_ioctl_param_t ivs_ioctl_param;
	
    switch (cmd) {

		case IVS_INIT:
			if ((ret = copy_from_user((void *)&API_version, (void *)arg, sizeof(unsigned int)))) 
            {
				printk("copy from user error: %d %d\n", ret, sizeof(API_version));
				return -2;
			}

            ret = ioctl_init(API_version);		
		break;

        case IVS_INIT_MEM:
			if ((ret = copy_from_user((void *)&ivs_ioctl_param, (void *)arg, sizeof(ivs_ioctl_param))))
			{
				printk("copy from user error: %d %d\n", ret, sizeof(ivs_ioctl_param));
				return -2;
			}

            ret = ioctl_init_mem(&ivs_ioctl_param);
			
		    if ((ret = copy_to_user((void *)arg, (void *)&ivs_ioctl_param, sizeof(ivs_ioctl_param))))
            {
                printk("copy to user error\n");        
                return -3;    
            }
		break;

		case IVS_HSI_CONVERSION:
			if ((ret = copy_from_user((void *)&ivs_ioctl_param, (void *)arg, sizeof(ivs_ioctl_param))))
			{
				printk("copy from user error: %d %d\n", ret, sizeof(ivs_ioctl_param));
				return -2;
			}

            ret = ioctl_hsi_conversion(&ivs_ioctl_param, filp);
                
			if ((ret = copy_to_user((void *)arg, (void *)&ivs_ioctl_param, sizeof(ivs_ioctl_param))))
			{
				printk("copy to user error\n");		  
				return -3;	 
			}
		break;

        case IVS_RGB_CONVERSION:
			if ((ret = copy_from_user((void *)&ivs_ioctl_param, (void *)arg, sizeof(ivs_ioctl_param))))
			{
				printk("copy from user error: %d %d\n", ret, sizeof(ivs_ioctl_param));
				return -2;
			}

            ret = ioctl_rgb_conversion(&ivs_ioctl_param, filp);
                
			if ((ret = copy_to_user((void *)arg, (void *)&ivs_ioctl_param, sizeof(ivs_ioctl_param))))
			{
				printk("copy to user error\n"); 	  
				return -3;	 
			}
		break;  

        case IVS_INTEGRAL_IMAGE:
			if ((ret = copy_from_user((void *)&ivs_ioctl_param, (void *)arg, sizeof(ivs_ioctl_param))))
			{
				printk("copy from user error: %d %d\n", ret, sizeof(ivs_ioctl_param));
				return -2;
			}

        	ret = ioctl_integral_image(&ivs_ioctl_param, filp) ;
        	
			if ((ret = copy_to_user((void *)arg, (void *)&ivs_ioctl_param, sizeof(ivs_ioctl_param))))
			{
				printk("copy to user error\n");		  
				return -3;	 
			}
        break;

        case IVS_SQUARED_INTEGRAL_IMAGE:
			if ((ret = copy_from_user((void *)&ivs_ioctl_param, (void *)arg, sizeof(ivs_ioctl_param))))
			{
				printk("copy from user error: %d %d\n", ret, sizeof(ivs_ioctl_param));
				return -2;
			}

        	ret = ioctl_squared_integral_image(&ivs_ioctl_param, filp);
        	
			if ((ret = copy_to_user((void *)arg, (void *)&ivs_ioctl_param, sizeof(ivs_ioctl_param))))
			{
				printk("copy to user error\n");		  
				return -3;	 
			}
        break;
        
        case IVS_DE_INTERLEAVING:    
			if ((ret = copy_from_user((void *)&ivs_ioctl_param, (void *)arg, sizeof(ivs_ioctl_param))))
			{
				printk("copy from user error: %d %d\n", ret, sizeof(ivs_ioctl_param));
				return -2;
			}

			ret = ioctl_de_interleaving(&ivs_ioctl_param, filp);			
               
			if ((ret = copy_to_user((void *)arg, (void *)&ivs_ioctl_param, sizeof(ivs_ioctl_param))))
			{
				printk("copy to user error\n");		 
				return -3;	
			}
		break;

        case IVS_HISTOGRAM:    
			if ((ret = copy_from_user((void *)&ivs_ioctl_param, (void *)arg, sizeof(ivs_ioctl_param))))
			{
				printk("copy from user error: %d %d\n", ret, sizeof(ivs_ioctl_param));
				return -2;
			}
     
			ret = ioctl_histogram(&ivs_ioctl_param, filp) ;			

			if ((ret = copy_to_user((void *)arg, (void *)&ivs_ioctl_param, sizeof(ivs_ioctl_param))))
			{
				printk("copy to user error\n");		 
				return -3;	
			}
		break;

        case IVS_CONVOLUTION:    
			if ((ret = copy_from_user((void *)&ivs_ioctl_param, (void *)arg, sizeof(ivs_ioctl_param))))
			{
				printk("copy from user error: %d %d\n", ret, sizeof(ivs_ioctl_param));
				return -2;
			}
            
			ret = ioctl_convolution(&ivs_ioctl_param, filp) ;			
              
		  	if ((ret = copy_to_user((void *)arg, (void *)&ivs_ioctl_param, sizeof(ivs_ioctl_param))))
		  	{
				printk("copy to user error\n");	   
			  	return -3;  
		  	}
		break;

        case IVS_MORPHOLOGY:    
			if ((ret = copy_from_user((void *)&ivs_ioctl_param, (void *)arg, sizeof(ivs_ioctl_param))))
			{
				printk("copy from user error: %d %d\n", ret, sizeof(ivs_ioctl_param));
				return -2;
			}

            ret = ioctl_morphology(&ivs_ioctl_param, filp);			

            if ((ret = copy_to_user((void *)arg, (void *)&ivs_ioctl_param, sizeof(ivs_ioctl_param))))
			{
				printk("copy to user error\n");		 
				return -3;	
			}
		break;
       		
        case IVS_SAD:
			if ((ret = copy_from_user((void *)&ivs_ioctl_param, (void *)arg, sizeof(ivs_ioctl_param))))
			{
				printk("copy from user error: %d %d\n", ret, sizeof(ivs_ioctl_param));
				return -2;
			}

            ret = ioctl_sad(&ivs_ioctl_param, filp) ;			

			if ((ret = copy_to_user((void *)arg, (void *)&ivs_ioctl_param, sizeof(ivs_ioctl_param))))
			{
				printk("copy to user error\n");		 
				return -3;	
			}
        break;		

        case IVS_RASTER_OPERATION:
			if ((ret = copy_from_user((void *)&ivs_ioctl_param, (void *)arg, sizeof(ivs_ioctl_param))))
			{
				printk("copy from user error: %d %d\n", ret, sizeof(ivs_ioctl_param));
				return -2;
			}

            ret = ioctl_raster(&ivs_ioctl_param, filp) ;
             
			if ((ret = copy_to_user((void *)arg, (void *)&ivs_ioctl_param, sizeof(ivs_ioctl_param))))
			{
				printk("copy to user error\n");	  
				return -3;  
			}
        break;	

        case IVS_CASCADED_CLASSIFIER:
			if ((ret = copy_from_user((void *)&ivs_ioctl_param, (void *)arg, sizeof(ivs_ioctl_param))))
			{
				printk("copy from user error: %d %d\n", ret, sizeof(ivs_ioctl_param));
				return -2;
			}

            ret = ioctl_cascaded_classifier(&ivs_ioctl_param, filp);
            
           	if ((ret = copy_to_user((void *)arg, (void *)&ivs_ioctl_param, sizeof(ivs_ioctl_param))))
			{
				printk("copy to user error\n");		 
				return -3;	
			}
        break;  
          
        default:
            printk("undefined ioctl cmd %x\n", cmd);
			return -1;
       	break;
       	
    }
    
	return 0;
}

int ivs_mmap(struct file *file, struct vm_area_struct *vma)
{	
    int offset = 0;
    struct dec_private *decp = file->private_data;
    int dev = decp->dev;
 	int size = vma->vm_end - vma->vm_start;
    unsigned long pfn = 0;

    offset = dev * PAGE_ALIGN(max_width * max_height * 10); 
    pfn = (ivs_phy + offset) >> PAGE_SHIFT;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;
     
	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
		kfree( (void*)ivs_virt ) ;
		return -EAGAIN;
	}
	return 0;
}

int ivs_open(struct inode *inode, struct file *filp)
{
    int idx = 0;
    struct dec_private * decp;

	if((ivs_idx & IVS_IDX_FULL) == IVS_IDX_FULL) {
	    printk("IVS Device Service Full,0x%x!\n",ivs_idx);
		return -EFAULT;
	}

    down(&favc_dec_mutex);
	for (idx = 0; idx < MAX_IVS_NUM; idx ++) {
		if( (ivs_idx&IVS_IDX_MASK_n(idx)) == 0) {
			ivs_idx |= IVS_IDX_MASK_n(idx);
			break;
		}
	}

    up(&favc_dec_mutex);

    decp = (struct dec_private *)&dec_data[idx];
    memset(decp,0,sizeof(struct dec_private));

    decp->dev = idx;
    filp->private_data = decp;
    #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
        MOD_INC_USE_COUNT;
    #endif
  
    return 0; /* success */
}

int ivs_release(struct inode *inode, struct file *filp)
{
    struct dec_private *decp = filp->private_data ;
	int dev = decp->dev;

    down(&favc_dec_mutex);
	ivs_idx&=(~(IVS_IDX_MASK_n(dev)));
    up(&favc_dec_mutex);

    return 0;
}

static struct file_operations ivs_fops = {
	.owner = THIS_MODULE,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
	.unlocked_ioctl = ivs_ioctl,
#else
    .ioctl          = ivs_ioctl,
#endif
    .mmap           = ivs_mmap,
    .open           = ivs_open,
    .release        = ivs_release,
};
    
static struct miscdevice ivs_dev = {
    .minor          = IVS_MINOR,
    .name           = "ivs",
    .fops           = &ivs_fops,
};

void ivs_isr(int irq, void *dev_id)
{	
	//	IVS	Interrupt flag reg
	volatile unsigned int *clear_itr = ivs_reg_base + (0x78 >> 2) ;
	
	//	clear IVS interrupt
	*clear_itr  = *clear_itr  | 1 ;

	isr_condition_flag = 1 ;
	wake_up(&ivs_queue);	
}

static irqreturn_t ivs_int_hd( int irq, void *dev_id)
{
	ivs_isr(irq, dev_id);
	return IRQ_HANDLED;
}

void enable_clk_and_stop_reset(void)
{
    volatile unsigned int *ivs_reg_rst;
    volatile unsigned int *ivs_reg_apb;
    volatile unsigned int *ivs_reg_axi;

    ivs_reg_rst  = clk_reg_base + (0xa0 >> 2);		
    ivs_reg_axi  = clk_reg_base + (0xb0 >> 2);
    ivs_reg_apb  = clk_reg_base + (0xbc >> 2);

	*ivs_reg_rst = *ivs_reg_rst | (1 << 14) ;
	*ivs_reg_axi = *ivs_reg_axi & (~(1<<9)) ;
	*ivs_reg_apb = *ivs_reg_apb & (~(1<<12)) ; 
}

int init_CC_reg(int regOffset, char filename[])
{
    int i =0;
    int ret = 0;
    unsigned int regVal = 0;    
    unsigned long long offset = 0;
    struct file *finp = NULL;
    mm_segment_t fs;

    volatile unsigned int *reg_haar ;
        
    fs = get_fs();
    set_fs(get_ds());

    finp = filp_open(filename, O_RDONLY, S_IRWXU);
    if (IS_ERR(finp)) 
        return -1;

    while((ret = (int)vfs_read(finp,&regVal,4,&offset))== 4)
    {
        i = i + 1;
        reg_haar = ivs_reg_base + ((regOffset + (i - 1) * 4) >> 2) ; 
        *reg_haar = regVal;
        //printk("%08X\n",regVal);
    }

    filp_close(finp, NULL);
    set_fs(fs);
    
    return 0;
}

int init_CCfeat_reg(char filename_1[],char filename_2[])
{
    int i =0;
    int ret = 0;
    unsigned int regVal_1 = 0, regVal_2 = 0;;    
    unsigned long long offset_1 = 0, offset_2 = 0;
    struct file *finp_1 = NULL, *finp_2 = NULL;
    mm_segment_t fs;

    volatile unsigned int *reg_haar ;

    fs = get_fs();
    set_fs(get_ds());

    finp_1 = filp_open(filename_1, O_RDONLY, S_IRWXU);
    if (IS_ERR(finp_1)) 
        return -1;
    
    finp_2 = filp_open(filename_2, O_RDONLY, S_IRWXU);
    if (IS_ERR(finp_2)) 
        return -1;

    while (((ret = (int)vfs_read(finp_1,&regVal_1,4,&offset_1))== 4) &&
           ((ret = (int)vfs_read(finp_2,&regVal_2,4,&offset_2))== 4)  )
    {
        i = i + 1;
        reg_haar = ivs_reg_base + ((0x4000 + (i - 1) * 4) >> 2) ; 
        *reg_haar = regVal_1;
        reg_haar = ivs_reg_base + ((0x8000 + (i - 1) * 4) >> 2) ; 
        *reg_haar = regVal_2;
    }

    filp_close(finp_1, NULL);
    filp_close(finp_2, NULL);
    set_fs(fs);

    return 0;
}

void initial_FD_parameter(void)
{
    int ret = 0;
    char filename1[0x80], filename2[0x80];
    
    sprintf(filename1, "%s/%s", bin_path, "stage.bin");
    ret = init_CC_reg(0x180,filename1);
    if (ret < 0)
        printk("open stage.bin error\n");
    
    sprintf(filename1, "%s/%s", bin_path, "weak.bin");
    ret = init_CC_reg(0x2000,filename1);
    if (ret < 0)
        printk("open weak.bin error\n");
    
    sprintf(filename1, "%s/%s", bin_path, "feature.bin");
    sprintf(filename2, "%s/%s", bin_path, "rect.bin");
    ret = init_CCfeat_reg(filename1,filename2);
    if (ret < 0)
        printk("open feature.bin/rect.bin error\n");
}

int ivs_init_ioctl(void)
{
    int i = 0;
    char mem_name[16];
    unsigned int ret = 0;
    struct frammap_buf_info binfo;

    // base register
    clk_reg_base = ioremap_nocache(0x90c00000, PAGE_ALIGN(0x100000));
    if( clk_reg_base<=0 ) {
        printk("Fail to use ioremap_nocache, clk_reg_base = %x!\n", (unsigned int)clk_reg_base);
        goto init_clk_base_error;
    }

    ivs_reg_base = ioremap_nocache(0x9b600000, PAGE_ALIGN(0x100000));
    if( ivs_reg_base<=0 ) {
        printk("Fail to use ioremap_nocache, ivs_reg_base = %x!\n", (unsigned int)ivs_reg_base);
        goto init_ivs_base_error;
    }

    enable_clk_and_stop_reset();

    // initital driver
    if( misc_register(&ivs_dev)<0) {
        printk("ivs misc-register fail");
        goto misc_register_fail;
    }

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
    sema_init( &favc_dec_mutex, 1 );
#else
    init_MUTEX( &favc_dec_mutex);
#endif

    if( request_irq( IVS_IRQ, ivs_int_hd, 0, "IVS IRQ", NULL) != 0){   
        printk("IVS interrupt request failed, IRQ=%d", IVS_IRQ);
        goto init_irq_fun_error;
    }
    
    init_waitqueue_head(&ivs_queue);

    if ((max_width > 3840) || (max_height > 2160) || (max_ap > 32)) {
        printk("parameter error, max_width <= 3840, max_height <= 2160, max_ap <= 32\n");
        goto allocate_mem_error;
    }

    for (i = 0; i < max_ap; i++)
        IVS_IDX_FULL |= (1 << i); 
    
    // pre-allocate memory
    IMEM_SIZE = PAGE_ALIGN(max_width * max_height * 10) * max_ap; 
    strcpy( mem_name, "ivs_mem" ) ;
    binfo.size = PAGE_ALIGN(IMEM_SIZE); 
    binfo.align = 4096;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
    binfo.name = mem_name;
    binfo.alloc_type = ALLOC_NONCACHABLE;
#endif
    ret = frm_get_buf_info(0, &binfo);
    if (ret == 0) {
        ivs_virt = (unsigned int)binfo.va_addr;
        ivs_phy = binfo.phy_addr;
        memory_counter = memory_counter + 1;
    } else {
        goto allocate_mem_error;
    }
    
    initial_FD_parameter();

    return 0;

allocate_mem_error:
    free_irq(IVS_IRQ, NULL);
init_irq_fun_error:
    misc_deregister(&ivs_dev);
misc_register_fail:
    iounmap(ivs_reg_base);
init_ivs_base_error:
    iounmap(clk_reg_base);
init_clk_base_error:

    printk("init error\n");
    return -1;
}

/*  ivs_exit_ioctl : rmmod driver close function 
 *  
 *  Flowchart :
 *  1. release all resource (clk_reg_base, ivs_reg_base, ivs_dev, IVS_IRQ, memory)
*/

int ivs_exit_ioctl(void)
{
    iounmap(ivs_reg_base);
    misc_deregister(&ivs_dev);
    free_irq(IVS_IRQ, NULL);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))            
    frm_release_buf_info((void *)ivs_virt);
#else                   
{
    struct frammap_buf_info binfo;            
    binfo.va_addr = (unsigned int)ivs_virt;            
    binfo.phy_addr = (unsigned int)ivs_phy;            
    frm_release_buf_info(0, &binfo);
    memory_counter = memory_counter - 1;
}
#endif
    printk("cleanup\n");
    return 0;
}

EXPORT_SYMBOL(ivs_hsi_conversion);
EXPORT_SYMBOL(ivs_rgb_conversion);
EXPORT_SYMBOL(ivs_integral_image);
EXPORT_SYMBOL(ivs_squared_integral_image);
EXPORT_SYMBOL(ivs_de_interleaving);
EXPORT_SYMBOL(ivs_histogram);
EXPORT_SYMBOL(ivs_convolution);
EXPORT_SYMBOL(ivs_morphology);
EXPORT_SYMBOL(ivs_sad);
EXPORT_SYMBOL(ivs_raster);
EXPORT_SYMBOL(ivs_cascaded_classifier);