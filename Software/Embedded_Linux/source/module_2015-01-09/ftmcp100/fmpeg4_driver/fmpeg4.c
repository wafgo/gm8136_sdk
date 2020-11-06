/* fmpeg4.c */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/hdreg.h>	/* HDIO_GETGEO */
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/interrupt.h>

#include "fmpeg4.h"
#include "ioctl_mp4d.h"
#include "ioctl_mp4e.h"
#include "fmcp.h"

#ifdef SUPPORT_VG
#include <linux/kfifo.h>
//#include "videograph_api.h"
#endif
#undef  PFX
#define PFX	        "FMPEG4"
#include "debug.h"
#include "../fmcp.h"
#include "mp4e_param.h"
#ifdef REF_POOL_MANAGEMENT
    #ifdef SHARED_POOL
        #include "shared_pool.h"
    #else
        #include "mem_pool.h"
    #endif
#endif
#ifdef MPEG4_COMMON_PRED_BUFFER
    #include "mpeg4_decoder/global_d.h"
#endif

#define M_DEBUG(fmt, args...) //printk("FMPEG4: " fmt, ## args)

//#ifndef SUPPORT_VG
	unsigned int mp4_max_width = MAX_DEFAULT_WIDTH;
	unsigned int mp4_max_height = MAX_DEFAULT_HEIGHT;
	#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
		module_param(mp4_max_width, uint, S_IRUGO|S_IWUSR);
		module_param(mp4_max_height, uint, S_IRUGO|S_IWUSR);
	#else
		MODULE_PARM(mp4_max_width,"i");
		MODULE_PARM(mp4_max_height, "i");
	#endif
	MODULE_PARM_DESC(mp4_max_width, "Max Width");
	MODULE_PARM_DESC(mp4_max_height, "Max Height");
//#endif

#ifdef MPEG4_COMMON_PRED_BUFFER // Tuba 20131108: use common pred buffer
    struct buffer_info_t mp4e_pred_buf = {0, 0, 0};
    struct buffer_info_t mp4d_acdc_buf = {0, 0, 0};
    struct buffer_info_t mp4d_mb_buf = {0, 0, 0};
    #ifdef ENABLE_DEBLOCKING
        struct buffer_info_t mp4d_deblock_buf = {0, 0, 0};
    #endif
#endif

extern unsigned int mp4_dec_enable;
extern char *config_path;

extern int mp4e_rc_register(struct rc_entity_t *entity);
extern int mp4e_rc_deregister(void);

#ifdef EVALUATION_PERFORMANCE
#include "./common/performance.h"
FRAME_TIME encframe;
FRAME_TIME decframe;
TOTAL_TIME enctotal;
TOTAL_TIME dectotal;

uint64 time_delta(struct timeval *start, struct timeval *stop)
{
    uint64 secs, usecs;
    
    secs = stop->tv_sec - start->tv_sec;
    usecs = stop->tv_usec - start->tv_usec;
    if (usecs < 0) {
        secs--;
        usecs += 1000000;
    }
    return secs * 1000000 + usecs;
}

void enc_performance_count(void)
{
	enctotal.total += (encframe.stop - encframe.start); 
	enctotal.hw_total += (encframe.hw_stop - encframe.hw_start); 
	enctotal.sw_total += (encframe.hw_start - encframe.start); 
	enctotal.ap_total += (encframe.ap_stop - encframe.ap_start);
}

void dec_performance_count(void)
{
	dectotal.total += (decframe.stop - decframe.start); 
	dectotal.hw_total += (decframe.hw_stop - decframe.hw_start); 
	dectotal.sw_total += (decframe.hw_start - decframe.start); 
	dectotal.ap_total += (decframe.ap_stop - decframe.ap_start);
}

void enc_performance_report(void)
{
	if ( enctotal.ap_total != 0) {
		printk("Menc hw %d ap %d count %d\n", 
		enctotal.hw_total,
		enctotal.ap_total,
		enctotal.count/INTERVAL);
	}
}

void dec_performance_report(void)
{
	if ( dectotal.ap_total != 0 ) {
		printk("Mdec hw %d ap %d count %d\n", 
		dectotal.hw_total,
		dectotal.ap_total,
		dectotal.count/INTERVAL);
	}
}

void enc_performance_reset(void)
{
	enctotal.total = 0;
	enctotal.hw_total = 0;
	enctotal.sw_total = 0;
	enctotal.ap_total = 0;
	enctotal.count = 0;
}

void dec_performance_reset(void)
{
	dectotal.total = 0;
	dectotal.hw_total = 0;
	dectotal.sw_total = 0;
	dectotal.ap_total = 0;
	dectotal.count = 0;
}

unsigned int get_counter(void)
{
	struct timeval tv;
	do_gettimeofday(&tv);
  
	return tv.tv_sec * 1000000 + tv.tv_usec;
}
#endif


//#ifndef SUPPORT_VG
//extern unsigned int mcp100_bs_length;
//#endif
#ifndef SUPPORT_VG
    extern struct buffer_info_t mcp100_bs_buffer;
#endif
#if defined(GM8120)
	#define MP4_STRING "GM8120 MPEG4"
#elif defined(FIE8150)
	#define MP4_STRING "FIE8150 MPEG4"
#elif defined(GM8180)
	#define MP4_STRING "GM8180 MPEG4"
#elif defined(GM8185)
	#define MP4_STRING "GM8185 V2 MPEG4"
#elif defined(GM8126)
	#define MP4_STRING "GM8126 MPEG4"
#elif defined(GM8210)
    #define MP4_STRING "GM8210 MPEG4"
#elif defined(GM8287)
    #define MP4_STRING "GM8287 MPEG4"
#elif defined(GM8139)
    #define MP4_STRING "GM8139 MPEG4"
#elif defined(GM8136)
    #define MP4_STRING "GM8136 MPEG4"
#else
	#define MP4_STRING "MPEG4"
#endif

extern int mp4d_drv_init(void);
extern void mp4d_drv_close(void);
extern int mp4e_drv_init(void);
extern void mp4e_drv_close(void);

#ifdef MPEG4_COMMON_PRED_BUFFER
static void release_mpeg4_common_buffer(void)
{
    free_buffer(&mp4e_pred_buf);
    if (mp4_dec_enable) {
        free_buffer(&mp4d_acdc_buf);
        free_buffer(&mp4d_mb_buf);
    #ifdef ENABLE_DEBLOCKING
        free_buffer(&mp4d_deblock_buf);
    #endif
    }
}
static int allocate_mpeg4_common_buffer(void)
{
    int ddr_idx = 0;

    if (parse_param_cfg(&ddr_idx, NULL, TYPE_MPEG4_ENCODER) < 0)
        return -1;

#if 1
    // allocate common buffer of MPEG4 encoder
    if (allocate_buffer_from_ddr(&mp4e_pred_buf, mp4_max_width * 4 * 16, ddr_idx) < 0) {
        printk("allocate mpeg4 enc pred buffer fail!\n");
        goto allocate_fail;
    }

    // allocate common buffer of MPEG4 decoder
    if (mp4_dec_enable) {
        if (parse_param_cfg(&ddr_idx, NULL, TYPE_MPEG4_DECODER) < 0)
            return -1;
        if (allocate_buffer_from_ddr(&mp4d_acdc_buf, mp4_max_width * 4, ddr_idx) < 0) {
            printk("allocate mpeg4 dec acdc buffer fail\n");
            goto allocate_fail;
        }
        if (allocate_buffer_from_ddr(&mp4d_mb_buf, sizeof(MACROBLOCK) * mp4_max_width * mp4_max_height / 256, ddr_idx) < 0) {
            printk("allocate mpeg4 dec mb buffer fail\n");
            goto allocate_fail;
        }
    #ifdef ENABLE_DEBLOCKING
        if (allocate_buffer_from_ddr(&mp4d_deblock_buf, mp4_max_width * 16, ddr_idx) < 0) {
            printk("allocate mpeg4 dec deblock buffer fail\n");
            goto allocate_fail;
        }
    #endif
    }

#else
    // allocate common buffer of MPEG4 encoder
    if (allocate_buffer(&mp4e_pred_buf, mp4_max_width * 4 * 16) < 0) {
        printk("allocate mpeg4 enc pred buffer fail!\n");
        goto allocate_fail;
    }

    // allocate common buffer of MPEG4 decoder
    if (mp4_dec_enable) {
        if (allocate_buffer(&mp4d_acdc_buf, mp4_max_width * 4) < 0) {
            printk("allocate mpeg4 dec acdc buffer fail\n");
            goto allocate_fail;
        }
        if (allocate_buffer(&mp4d_mb_buf, sizeof(MACROBLOCK) * mp4_max_width * mp4_max_height / 256) < 0) {
            printk("allocate mpeg4 dec mb buffer fail\n");
            goto allocate_fail;
        }
    #ifdef ENABLE_DEBLOCKING
        if (allocate_buffer(&mp4d_deblock_buf, mp4_max_width * 16) < 0) {
            printk("allocate mpeg4 dec deblock buffer fail\n");
            goto allocate_fail;
        }
    #endif
    }
#endif
    return 0;
allocate_fail:
    release_mpeg4_common_buffer();
    return -1;
}
#endif

int mpeg4_msg(char *str)
{
    int len = 0;

    if (str) {
        len += sprintf(str+len, MP4_STRING);
    	#if defined(SEQ)
    		len += sprintf(str+len, " with Sequencer");
    	#elif defined(TWO_P_EXT)
    		len += sprintf(str+len, " with CPU");
    	#elif defined(ONE_P_EXT)
    	#else
    		"Error configuration"		// do not remove this string
    	#endif
        /*
    	#ifdef SUPPORT_VG
    		printk(" for VG");
    	#else
    		printk(", max res. %d x %d", mp4_max_width, mp4_max_height);
    	#endif
    	*/
        len += sprintf(str+len, ", max res = %d x %d", mp4_max_width, mp4_max_height);
        
        len += sprintf(str+len, ", config path = \"%s\"", config_path);
        if (mp4_dec_enable) {
        	len += sprintf(str+len, ", decoder v%d.%d.%d",MP4D_VER_MAJOR, MP4D_VER_MINOR, MP4D_VER_MINOR2);
            if (MP4D_VER_BRANCH)
                len += sprintf(str+len, ".%d", MP4D_VER_BRANCH);
        }
    	len += sprintf(str+len, ", encoder v%d.%d.%d",MP4E_VER_MAJOR,MP4E_VER_MINOR, MP4E_VER_MINOR2);
        if (MP4E_VER_BRANCH)
            len += sprintf(str+len, ".%d", MP4E_VER_BRANCH);
        len += sprintf(str+len, ", built @ %s %s\n", __DATE__, __TIME__);
    }
    else {
        printk(MP4_STRING);
        if (mp4_dec_enable) {
        	printk(", decoder v%d.%d.%d",MP4D_VER_MAJOR, MP4D_VER_MINOR, MP4D_VER_MINOR2);
            if (MP4D_VER_BRANCH)
                printk(".%d", MP4D_VER_BRANCH);
        }
    	printk(", encoder v%d.%d.%d",MP4E_VER_MAJOR,MP4E_VER_MINOR, MP4E_VER_MINOR2);
        if (MP4E_VER_BRANCH)
            printk(".%d", MP4E_VER_BRANCH);
        printk(", built @ %s %s\n", __DATE__, __TIME__);
    }
    return len;
}

static int __init init_fmpeg4(void)
{
/*
	printk(MP4_STRING);
	#if defined(SEQ)
		printk(" with Sequencer");
	#elif defined(TWO_P_EXT)
		printk(" with CPU");
	#elif defined(ONE_P_EXT)
	#else
		"Error configuration"		// do not remove this string
	#endif
	#ifdef SUPPORT_VG
		printk(" for VG");
	#else
		printk(", max res. %d x %d", mp4_max_width, mp4_max_height);
	#endif
    printk(", config path = \"%s\"", config_path);
    if (mp4_dec_enable) {
    	printk(", decoder ver: %d.%d.%d",MP4D_VER_MAJOR, MP4D_VER_MINOR, MP4D_VER_MINOR2);
        if (MP4D_VER_BRANCH)
            printk(".%d", MP4D_VER_BRANCH);
    }
	printk(", encoder ver: %d.%d.%d",MP4E_VER_MAJOR,MP4E_VER_MINOR, MP4E_VER_MINOR2);
    if (MP4E_VER_BRANCH)
        printk(".%d", MP4E_VER_BRANCH);
    printk(", built @ %s %s\n", __DATE__, __TIME__);
*/
    // Tuba 20140610: add constraint of maximal resolution
    mp4_max_width = ((mp4_max_width+15)>>4)<<4;
    if (mp4_max_width > 2032)
        mp4_max_width = 2032;
    mp4_max_height = ((mp4_max_height+15)>>4)<<4;
    if (mp4_max_height > 2032)
        mp4_max_height = 2032;

	#ifndef SUPPORT_VG
	{
		int bs_len;
		//bs_len = DIV_M10000(mp4_max_width*mp4_max_height*3/2);
		bs_len = mp4_max_width*mp4_max_height*3/2;
		if (bs_len > mcp100_bs_buffer.size) {
			err("Pls ReInsert FMCP module with the same resolution or bigger, %d > %d", bs_len, mcp100_bs_buffer.size);
			return -1;
		}
	}
	#endif
#ifdef REF_POOL_MANAGEMENT
    if (init_mem_pool(mp4_max_width, mp4_max_height) < 0)
        return -1;
#endif
#ifdef MPEG4_COMMON_PRED_BUFFER // Tuba 20131108: use common pred buffer
    if (allocate_mpeg4_common_buffer() < 0)
        return -1;
#endif
    if (mp4_dec_enable) {
        if (mp4d_drv_init() < 0)
            goto init_mp4_fail;
    }
    
	if (mp4e_drv_init() < 0)
		goto init_mp4_fail;

    mpeg4_msg(NULL);

	return 0;

init_mp4_fail:
	mp4d_drv_close();
	return -1;
}

static void __exit cleanup_fmpeg4(void)
{
	mp4e_drv_close();
    if (mp4_dec_enable)
        mp4d_drv_close();
#ifdef MPEG4_COMMON_PRED_BUFFER
    release_mpeg4_common_buffer();
#endif
#ifdef REF_POOL_MANAGEMENT
    clean_mem_pool();
#endif
}

EXPORT_SYMBOL(mp4e_rc_register);
EXPORT_SYMBOL(mp4e_rc_deregister);

module_init(init_fmpeg4);
module_exit(cleanup_fmpeg4);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("FTMCP100 mpeg4 driver");
MODULE_LICENSE("GPL");
