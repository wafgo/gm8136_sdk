#ifndef __FT3DNR_DMA_H__
#define __FT3DNR_DMA_H__

typedef enum {
    WC_WAIT_VALUE = 0,
    RC_WAIT_VALUE,
    DMA_PARAM_MAX
} DMA_PARAM_ID_T;

typedef struct dma_param {
    int wc_wait_value;
    int rc_wait_value;
} dma_param_t;

int ft3dnr_dma_proc_init(struct proc_dir_entry *entity_proc);
void ft3dnr_dma_proc_remove(struct proc_dir_entry *entity_proc);

#endif
