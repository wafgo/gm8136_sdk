/* fmjpeg.c */
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

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
	#include "frammap_if.h"
	#include <asm/dma-mapping.h>
	//#include <mach/platform/spec.h>
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
	#include <asm/arch/fmem.h>
	#include <asm/arch/platform/spec.h>
#endif

#include "fmjpeg.h"
#include "./common/ftmcp.h"
#include "./common/ftmcp_comm.h"
#include "ioctl_jd.h"
#include "ioctl_je.h"
#ifdef SUPPORT_MJE_RC
    #include "mje_rc_param.h"
#endif

#ifdef SUPPORT_VG
//#include <linux/kfifo.h>
//#include "videograph_api.h"
    #include "log.h"
    #include "video_entity.h"
#endif
#undef  PFX
#define PFX	        "FMJPEG"
#include "debug.h"


#define M_DEBUG(fmt, args...) //printk(KERN_DEBUG "FMJPG: " fmt, ## args)

#if (!defined(SUPPORT_VG) || defined(SUPPORT_VG_422T))
    #include "fmcp.h"
	unsigned int jpg_max_width = MAX_DEFAULT_WIDTH;
	unsigned int jpg_max_height = MAX_DEFAULT_HEIGHT;
	#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
		module_param(jpg_max_width, uint, S_IRUGO|S_IWUSR);
		module_param(jpg_max_height, uint, S_IRUGO|S_IWUSR);
	#else
		MODULE_PARM(jpg_max_width,"i");
		MODULE_PARM(jpg_max_height,"i");
	#endif
	MODULE_PARM_DESC(jpg_max_width, "Max Width");
	MODULE_PARM_DESC(jpg_max_height, "Max Height");
#endif

unsigned int jpg_enc_max_chn = MJE_MAX_CHANNEL;
unsigned int jpg_dec_max_chn = MJD_MAX_CHANNEL;
module_param(jpg_enc_max_chn, uint, S_IRUGO|S_IWUSR);
module_param(jpg_dec_max_chn, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(jpg_enc_max_chn, "Max jpeg enc chn");
MODULE_PARM_DESC(jpg_dec_max_chn, "Max jpeg dec chn");

#ifdef SUPPORT_MJE_RC
extern int mje_rc_register(struct rc_entity_t *entity);
extern int mje_rc_deregister(void);
#endif

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
	if ( enctotal.ap_total != 0 ) {
		printk("Jenc hw %d ap %d count %d\n", 
		enctotal.hw_total, 
		enctotal.ap_total,
		enctotal.count/INTERVAL);
	}
}

void dec_performance_report(void)
{
	if ( dectotal.ap_total != 0) {
		printk("Jdec hw %d ap %d count %d\n", 
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

#ifndef SUPPORT_VG
//extern unsigned int mcp100_bs_length;
extern struct buffer_info_t mcp100_bs_buffer;
#endif
#if defined(GM8120)
	#define MJPEG_STRING "GM8120 MJPEG"
#elif defined(FIE8150)
	#define MJPEG_STRING "FIE8150 MJPEG"
#elif defined(GM8180)
	#define MJPEG_STRING "GM8180 MJPEG"
#elif defined(GM8185)
	#define MJPEG_STRING "GM8185 V2 MJPEG"
#elif defined(GM8181)
	#define MJPEG_STRING "GM8181 MJPEG"
#elif defined(GM8126)
	#define MJPEG_STRING "GM8126 MJPEG"
#elif defined(GM8210)
	#define MJPEG_STRING "GM8210 MJPEG"
#elif defined(GM8287)
	#define MJPEG_STRING "GM8287 MJPEG"
#elif defined(GM8139)
	#define MJPEG_STRING "GM8139 MJPEG"
#elif defined(GM8136)
	#define MJPEG_STRING "GM8136 MJPEG"
#else
	#define MJPEG_STRING "MJPEG"
#endif

//extern int drv_mjd_init(void);
//extern void drv_mjd_exit(void);
extern int mjd_drv_init(void);
extern void mjd_drv_close(void);
extern int mje_drv_init(void);
extern void mje_drv_close(void);

int jpeg_msg(char *str)
{
    int len = 0;
    if (str) {
        len += sprintf(str+len, MJPEG_STRING);
	#if defined(SEQ)
		len += sprintf(str+len, " with Sequencer");
	#elif defined(TWO_P_EXT)
		len += sprintf(str+len, " with CPU");
	#elif defined(ONE_P_EXT)
	#else
		"Error configuration"		// do not remove this string
	#endif
	#ifdef SUPPORT_VG
		len += sprintf(str+len, " for VG");
	#endif
	#if (!defined(SUPPORT_VG) || defined(SUPPORT_VG_422T))
    	len += sprintf(str+len, ", max res. %d x %d", jpg_max_width, jpg_max_height);
	#endif
    	len += sprintf(str+len, ", encoder v%d.%d.%d", MJPEGE_VER_MAJOR, MJPEGE_VER_MINOR, MJPEGE_VER_MINOR2);
        if (MJPEGE_VER_BRANCH)
            len += sprintf(str+len, ".%d", MJPEGE_VER_BRANCH);
        len += sprintf(str+len, "(%d)", jpg_enc_max_chn);
        len += sprintf(str+len, ", decoder v%d.%d.%d", MJPEGD_VER_MAJOR, MJPEGD_VER_MINOR, MJPEGD_VER_MINOR2);
        if (MJPEGD_VER_BRANCH)
        	len += sprintf(str+len, ".%d", MJPEGD_VER_BRANCH);
        len += sprintf(str+len, "(%d)", jpg_dec_max_chn);
        len += sprintf(str+len, ", built @ %s %s\n", __DATE__, __TIME__);
    }
    else {
        printk(MJPEG_STRING);
    	printk(", encoder v%d.%d.%d", MJPEGE_VER_MAJOR, MJPEGE_VER_MINOR, MJPEGE_VER_MINOR2);
        if (MJPEGE_VER_BRANCH)
            printk(".%d", MJPEGE_VER_BRANCH);
        printk(", decoder v%d.%d.%d", MJPEGD_VER_MAJOR, MJPEGD_VER_MINOR, MJPEGD_VER_MINOR2);
        if (MJPEGD_VER_BRANCH)
        	printk(".%d", MJPEGD_VER_BRANCH);
        printk(", built @ %s %s\n", __DATE__, __TIME__);
    }
    return len;
}

static int __init init_fmjpeg(void)
{
    /*
	printk(MJPEG_STRING);
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
	#endif
	#if (!defined(SUPPORT_VG) || defined(SUPPORT_VG_422T))
    	printk(", max res. %d x %d", jpg_max_width, jpg_max_height);
	#endif
//    #ifdef USE_WORKQUEUE
//        printk(", use workqueue");
//    #endif
	printk(", encoder ver: %d.%d.%d", MJPEGE_VER_MAJOR, MJPEGE_VER_MINOR, MJPEGE_VER_MINOR2);
    if (MJPEGE_VER_BRANCH)
        printk(".%d", MJPEGE_VER_BRANCH);
    printk(", decoder ver: %d.%d.%d", MJPEGD_VER_MAJOR, MJPEGD_VER_MINOR, MJPEGD_VER_MINOR2);
    if (MJPEGD_VER_BRANCH)
    	printk(".%d", MJPEGD_VER_BRANCH);
    printk(", built @ %s %s\n", __DATE__, __TIME__);
    */
	#ifndef SUPPORT_VG
	{
 		int bs_len;
		//bs_len = DIV_J10000(jpg_max_width*jpg_max_height*3/2);
		//if (bs_len > mcp100_bs_length) {
		bs_len = jpg_max_width*jpg_max_height*3/2;
		if (bs_len > mcp100_bs_buffer.size) {
			err("Pls ReInsert FMCP module with the same resolution or bigger, %d > %d", bs_len, mcp100_bs_buffer.size);
			return -1;
		}
	}
	#endif

    if (jpg_dec_max_chn > 0) {
        if (mjd_drv_init() < 0)
            return -1;
    }

    if (jpg_enc_max_chn > 0) {
    	if (mje_drv_init() < 0)
    		goto init_mjpeg_faill;
    }

    jpeg_msg(NULL);

	return 0;

init_mjpeg_faill:
	mjd_drv_close();
	return -1;
}

static void __exit cleanup_fmjpeg(void)
{
	mje_drv_close();
	mjd_drv_close();
}

#ifdef SUPPORT_MJE_RC
    EXPORT_SYMBOL(mje_rc_register);
    EXPORT_SYMBOL(mje_rc_deregister);
#endif
module_init(init_fmjpeg);
module_exit(cleanup_fmjpeg);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("FTMCP100 jpeg driver");
MODULE_LICENSE("GPL");
