#ifndef __MEM_ALLOC_H__
#define __MEM_ALLOC_H__

typedef enum {
    MEM_TYPE_CB     = 0x1,
    MEM_TYPE_NCNB   = 0x2,
    MEM_TYPE_ATOMIC = 0x4,  //can't sleep
} mem_type_t;

typedef struct {
    int memory_sz;      //total memory size
    int max_slice;      //what is the max piece of memory can be allocated
    int clean_space;    //how many memory is never dirty
    int alloc_cnt;      //how many memory blocks were allocated
    int wait_cnt;       //how many times the callers need to wait memory available
    int dma_llp_cnt;
} mem_counter_t;

/* 0 for success, others for fail */
int mem_alloc_init(unsigned int pool_size, char *name);
int mem_alloc_pool_resize(unsigned int new_pool_size);

/* return vaddr if successs. NULL for fail */
void *mem_alloc(int size, int align, dma_addr_t *phy_addr, mem_type_t mem_type);
void mem_free(void *vaddr);
void mem_alloc_exit(void);
void mem_get_counter(mem_counter_t *counter);
int mem_get_poolinfo(unsigned int *paddr, unsigned int *size);

/* For PCI memory
 */
/* 0 for success, others for fail */
int pci_mem_alloc_init(unsigned int pci_pool_base, unsigned int pool_size, char *name);
void *pci_mem_alloc(int size, int align, dma_addr_t *phy_addr, mem_type_t mem_type);
void pci_mem_free(void *vaddr);
void pci_mem_alloc_exit(void);
void pci_mem_get_counter(mem_counter_t *counter);
int pci_mem_get_poolinfo(unsigned int *paddr, unsigned int *size);

/* For DMAC LLP
 */
int mem_llp_alloc_init(unsigned int pool_size, int fix_size);
int mem_llp_alloc_exit(void);
void *mem_llp_alloc(dma_addr_t *phy_addr);
void mem_llp_free(void *vaddr);

#endif /* __MEM_ALLOC_H__ */


