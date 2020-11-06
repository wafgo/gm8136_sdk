#ifndef __SCL_DMA_H__
#define __SCL_DMA_H__

typedef struct {
    int     job_id;         // job id
    u16     x_offset;       // x start in pixel
    u16     y_offset;       // y start in pixel
    u16     width;          // width in pixel
    u16     height;         // height in line
    u16     bg_width;       // background width in pixel
    u16     bg_height;      // background height in pixel
    u32     src_addr;       // source address
    u32     dst_addr;       // destination address
} scl_dma_t;

int scl_dma_init(void);
void scl_dma_exit(void);
int scl_dma_add (scl_dma_t *scl_dma);
int scl_dma_delete(int job_id);
int scl_dma_trigger(int job_id);
void scl_dma_dump(char *module_name);

#endif /* __SCL_DMA_H__ */