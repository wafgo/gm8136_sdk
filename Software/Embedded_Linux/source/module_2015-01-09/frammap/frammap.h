#ifndef __FRAMMAP_H__
#define __FRAMMAP_H__

#include "frammap_if.h"

#define FRM_VERSION_CODE "1.1.2"

#define PAGE_ALIGN_ORDER    0   //default page align order. The alignemnt is equal to PAGE * 2^PAGE_ALIGN_ORDER
#define MAX_DDR_NR          2       /* how many DDRs include system memory */
#define NAME_SZ             20

/* Define cache type
 */
#define CACHE_TYPE_CACHABLE         0x0
#define CACHE_TYPE_BUFFERABLE       0x1
#define CACHE_TYPE_NONCACHE         0x2

/*
 * flags field defintion
 * bit 28-31: alloc type
 * bit 0-23: alloc address alignment
 * bit 24-27: reserved
 */
/* Virtual memory mapping flag definition */
#define VM_F_WC     0x10000000
#define VM_F_NCNB   0x20000000
#define VM_F_IOREMAP  (VM_F_WC | VM_F_NCNB)

/* address alignment mask */
#define ADDR_ALIGN_MASK	((0x1 << 23) - 1)

/*
 * Internal use, don't conflict to enum rmmp_index
 */
#define FRMIDX_DDR0     253
#define FRMIDX_DDR1     254
#define FRMIDX_DDR2     255

/* If the align is not a 2 order, use this macro.
 */
#define FRM_ALIGN(addr, align)          \
{                                       \
	int	div;                            \
	if (align && (addr % align)) {		\
		div = (addr + align - 1) / align;   \
		addr &= (~(align - 1));         \
		addr += (div * align);          \
	}                                   \
	return addr;                        \
}

/*
 * DDR Descriptor
 */
typedef struct {
    dma_addr_t mem_base;
    dma_addr_t mem_end;
	dma_addr_t clear_addr;
	u32		allocate;
	int		align;
    int		mem_cnt;			//count for memory allocation
	struct kmem_cache *vm_cache;
	struct vm_struct  *vmlist;
    char device_name[20];       //device name
    struct semaphore  sema;
    int valid;                  //entry valid or not
} ddr_desc_t;

/* -------------------------------------------------------------------------
 * Main structure for AP memory allocation
 * -------------------------------------------------------------------------
 */
typedef struct {
    int devidx;                 /* which DDR device */
    int frmidx;                 /* Frammap index for DDR user */
    int open_cnt;
    int alloc_cnt;
    struct semaphore sema;
    struct kmem_cache *node_cache;
    struct list_head alloc_list;
    struct list_head open_list;
} ap_private_data_t;

/* If the open() is called, we will create one information is this open
 */
typedef struct {
    void            *filep; //index
    alloc_type_t     alloc_type;
    struct list_head list;
} ap_open_info_t;

/*
 * Main structure of AP memory allocation
 */
typedef struct {
    struct frammap_buf_info buf_info;
    ap_open_info_t  *private;
    struct list_head list;
} ap_node_t;

#ifdef CONFIG_PLATFORM_GM8181
#define CFG_FILE_PATH       "/mnt/mtd/config_8181"
#define CFG_FILE_TMPPATH    "/tmp/template_8181"
#elif defined(CONFIG_PLATFORM_GM8126)
#define CFG_FILE_PATH       "/mnt/mtd/config_8126"
#define CFG_FILE_TMPPATH    "/tmp/template_8126"
#elif defined(CONFIG_PLATFORM_GM8210)
#define CFG_FILE_PATH       "/mnt/mtd/config_8210"
#define CFG_FILE_TMPPATH    "/tmp/template_8210"
#elif defined(CONFIG_PLATFORM_GM8139)
#define CFG_FILE_PATH	    "/mnt/mtd/config_8139"
#define CFG_FILE_TMPPATH    "/tmp/template_8139"
#elif defined(CONFIG_PLATFORM_GM8287)
#define CFG_FILE_PATH       "/mnt/mtd/config_8287"
#define CFG_FILE_TMPPATH    "/tmp/template_8287"
#elif defined(CONFIG_PLATFORM_GM8136)
#define CFG_FILE_PATH	    "/mnt/mtd/config_8136"
#define CFG_FILE_TMPPATH    "/tmp/template_8136"
#else
    #error "Frammap: config file path needs to be configured!"
#endif

#endif //__FRAMMAP_H__

