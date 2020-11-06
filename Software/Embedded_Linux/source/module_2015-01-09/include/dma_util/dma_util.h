#ifndef __DMA_UTIL_H__
#define __DMA_UTIL_H__

#include <linux/ioctl.h>

#define DMA_CACHE_LINE_SZ   32

typedef struct {
    void *src_vaddr;
    void *dst_vaddr;
    unsigned int  length;    //must be 32 alignment
} dma_util_t;

/* IOCTLs */
#define DMA_UTIL_IOC_MAGIC	'h' //seed
#define DMA_UTIL_DO_DMA		_IOW(DMA_UTIL_IOC_MAGIC, 1, dma_util_t)

#endif /* __DMA_UTIL_H__ */