/* fmcp.h */
#ifndef _FMCP_H_
#define _FMCP_H_

#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/string.h>
#include <linux/ioport.h>       /* request_region */
#include <linux/interrupt.h>    /* mark_bh */
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/page.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
	#include "frammap_if.h"
	#include <asm/dma-mapping.h>
	//#include <mach/platform/spec.h>
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
	#include <asm/arch/fmem.h>
	#include <asm/arch/platform/spec.h>
#endif
#include "fmcp_type.h"
#ifdef SUPPORT_VG
    //#include "videograph_api.h"
    #include "video_entity.h"
#endif
#ifdef ENABLE_CHECKSUM
    #include "ioctl_cs.h"
#endif

#include "debug.h"

/* 
Version 1.0.1: mcp100 support rotation
Version 1.0.2: fix bug of release wrong number of buffer
*/
#define MCP100_VER          0x10000200
#define MCP100_VER_MAJOR    ((MCP100_VER>>28)&0x0F)     // huge change
#define MCP100_VER_MINOR    ((MCP100_VER>>20)&0xFF)     // interface change
#define MCP100_VER_MINOR2   ((MCP100_VER>>8)&0x0FFF)    // functional modified or buf fixed
#define MCP100_VER_BRANCH   (MCP100_VER&0xFF)           // branch for customer request

#define MAX_DEFAULT_WIDTH  720
#define MAX_DEFAULT_HEIGHT 576

#if 0 //allocation from videograph
#include "ms/ms_core.h"
#define driver_alloc(x) ms_small_fast_alloc(x)
#define driver_free(x) ms_small_fast_free(x)
#else
#define driver_alloc(x) kmalloc(x,GFP_ATOMIC)
#define driver_free(x) kfree(x)
#endif

//#define MCP100_MODULE_NAME  "MC"
#define MAX_ENGINE_NUM  1
/*********************************/
/*
#ifndef MPEG4_ENABLE
    #define MP4D_MAX_CHANNEL    70
    #define MP4E_MAX_CHANNEL    70
#endif
#ifndef JPEG_ENABLE
    #define MJD_MAX_CHANNEL     70
    #define MJE_MAX_CHANNEL     70
#endif
*/
/*********************************/

// level of log message
//extern int log_level;
#ifdef LINUX
    #define LOG_PRINT(level, fmt, arg...) { \
        if (level <= log_level) \
            printk("[mcp100]" fmt "\n", ## arg);    }
#else
    #define LOG_PRINT(...)
#endif


#ifndef SUPPORT_VG
#define MAX_DEFAULT_WIDTH   720
#define MAX_DEFAULT_HEIGHT  480
#endif
//#define FMCP100_BASE		MCP_FTMCP100_VA_BASE

#define USE_KTHREAD
#define USE_KTHREAD2
#define HW_LOCK_ENABLE

struct mcp100_rso
{
	unsigned int va_base;
	int irq_no;
	struct resource *res;
	unsigned int va_base_wrp;
	struct resource *res_wrp;
};
extern struct mcp100_rso * mcp100_dev;

#ifdef SUPPORT_VG
struct job_item_t {
    void                *job;
    int                 job_id; //record job->id
    int                 engine; //indicate while hw engine
    int                 chn;
    struct list_head    engine_list; //use to add engine_head
    struct list_head    minor_list; //use to add minor_head
    int                 codec_type;
#ifdef MULTI_FORMAT_DECODER
    //int                 frame_width;
    //int                 frame_height;
#endif

#define DRIVER_STATUS_STANDBY   1
#define DRIVER_STATUS_ONGOING   2
#define DRIVER_STATUS_FINISH    3
#define DRIVER_STATUS_FAIL      4
#define DRIVER_STATUS_KEEP      5
    int                 status;
    unsigned int        puttime;    //putjob time
    unsigned int        starttime;  //start engine time
    unsigned int        finishtime; //finish engine time
#ifdef HANDLE_BS_CACHEABLE  // Tuba 20140225: handle bitstream buffer cacheable
    unsigned int        bs_size;
#endif
#ifdef ENABLE_CHECKSUM
    unsigned int        check_sum_type;
#endif
};
#endif

typedef struct {
	void (*handler)(int, void *, unsigned int base);
	void * dev_id;
	void (*switch22)(void *, ACTIVE_TYPE );
	#ifdef SUPPORT_VG
        int (*job_tri) (struct job_item_t *job_item);
        int (*job_done) (struct job_item_t *job_item);
		//int (*job_tri) (struct v_job_t *job);			//return >=0 OK, -1: fail, -2: disable
		//int (*job_done) (struct v_job_t *job);		// 0: done, < 0: fail, > 0: not yet
		int codec_type;					// FMJPEG_ENCODER_MINOR, ....
		//struct v_job_t *curjob;
		struct job_item_t *curjob;
		int (*parm_info)(int engine, int channel);
		//void (*parm_info_frame)(struct v_job_t *job);
		void (*parm_info_frame)(struct job_item_t *job_item);
	#endif
} mcp100_rdev;

struct buffer_info_t
{
    unsigned int addr_pa;
    unsigned int addr_va;
    unsigned int size;
};

#ifndef MULTI_FORMAT_DECODER
#define MAX_NAME    50
#define MAX_README  100
struct dec_property_map_t {
    int id;
    char name[MAX_NAME];
    char readme[MAX_README];
};
#endif

//extern int mcp100_register(mcp100_rdev *dv, ACTIVE_TYPE type); 	//return >=0 OK, <0 fail
// Tuba 20141229: add max channel input
extern int mcp100_register(mcp100_rdev *dv, ACTIVE_TYPE type, int max_chn_num);
extern int mcp100_deregister(ACTIVE_TYPE type);
#ifdef SUPPORT_VG
//extern int mcp_putjob (struct v_job_t *job);
//extern int mcp_putjob(struct video_entity_job_t *job, int codec_type, int width, int height);
extern int mcp_putjob(struct video_entity_job_t *job, int codec_type);
extern int mcp_stopjob(int codec_type, int engine, int chn);
#endif
extern void *fkmalloc(size_t size, uint8_t alignment, uint8_t reserved_size);
extern void fkfree(void *mem_ptr);
//extern void * fcnsist_alloc(uint32_t size, void **phy_ptr, int attr_mmu);	// 1: cachable, 2: bufferable, 0: none
//extern void fcnsist_free(uint32_t size, void * virt_ptr, void * phy_ptr);
extern void * fconsistent_alloc(uint32_t size, uint8_t align_size, uint8_t reserved_size, void **phy_ptr);
extern void fconsistent_free(void * virt_ptr, void * phy_ptr);
//extern unsigned int user_va_to_pa(unsigned int addr);
//extern int check_continued(unsigned int addr,int size);

extern int allocate_buffer(struct buffer_info_t *buf, int size);
extern int allocate_buffer_from_ddr(struct buffer_info_t *buf, int size, int ddr_idx);
extern int free_buffer(struct buffer_info_t *buf);

#endif
