#ifndef __DMA_LLP_H__

/* Number of dma channel can be used
 */
#define NUM_DMA     4   /* max */
/*
 * The structure is for dmac030 which has faster speed.
 */
typedef struct {
    struct {
        unsigned int src_addr;
        unsigned int dst_addr;
        unsigned int llst_ptr;
        unsigned int control;
        unsigned int tcnt;
        unsigned int reserved[2];
    } llp;
    int llp_cnt;
} dmac_llp_t;

void dma_llp_init(void);
void dma_llp_add(u32 head_va, u32 head_pa, u32 src_paddr, u32 dst_paddr, u32 length, u32 src_width);
void dma_llp_fire(unsigned int head_va, unsigned int head_pa, int is_pci);
void dma_llp_exit(void);

typedef struct {
    int active_cnt;
    int chan_id[8];
    int sw_irq[8];
    u32 intr_cnt[8];
    int busy[8];
} dma_status_t;
void dma_get_status(dma_status_t *status);

#endif /* __DMA_LLP_H__ */
