#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/nodemask.h>
#include <linux/mm.h>
#include <linux/gfp.h>
#include <asm/memory.h>
#include <asm/bitops.h>
#include <linux/semaphore.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <asm/io.h>
#include "zebra_sdram.h"
#include "mem_alloc.h"
#include "dma_llp.h"
#include <frammap/frammap_if.h>

#define USE_STATIC_VA

#define SEMA_LOCK(x)       down(&(x)->sema)
#define SEMA_UNLOCK(x)     up(&(x)->sema)

struct vm_struct_s {
	struct vm_struct_s  *next;
	unsigned int        vaddr;
	dma_addr_t	        paddr;
	unsigned int		size;
	int                 ioremap;
	struct timer_list   write_timeout;
};

typedef struct pool {
	unsigned int start;     //va
	unsigned int end;       //va
	unsigned int mem_cnt;	//count for memory allocation
	unsigned int max_slice;	//what is max slice in the fragments
	unsigned int clear_addr;//
	unsigned int va_start;
	struct kmem_cache *vm_cache;
	struct vm_struct_s *p_vmlist;
	struct semaphore  sema;
#ifndef WIN32
	wait_queue_head_t mem_alloc_wait;
#endif
	char vm_name[20];
	u32 mem_wait_cnt;
	int init_done;
} pool_t;

/*
 * DMA LLP main structure
 */
static struct llp_pool_s {
    unsigned int paddr;
    unsigned int vaddr;
    unsigned int fix_size;
    unsigned int bit_arry[10];
    unsigned int free_cnt;
    unsigned int mem_cnt;
} llp_pool;

/* static variables */
#ifndef WIN32
static struct frammap_buf_info pool_info;
#endif

static struct semaphore llp_sema;
static pool_t local_pool, pci_pool;
static int llp_init_done = 0;

void dump_memory(pool_t *pool);

static int __mem_get_poolinfo(pool_t *pool, unsigned int *paddr, unsigned int *size)
{
    if (unlikely(!pool->init_done)) {
        printk("cpucomm: %s, mem_alloc is not ready yet! \n", pool->vm_name);
        *paddr = *size = 0;
        dump_stack();
        return -1;
    }

    *paddr = (unsigned int)pool->start;
    *size = pool->end - pool->start;

    return 0;
}

static unsigned int mem_get_maxspace(pool_t *pool)
{
    struct vm_struct_s **p, *tmp;
	unsigned int addr = pool->start, space, max_space = 0;

	for (p = &pool->p_vmlist; (tmp = *p) != NULL ;p = &tmp->next) {
		space = tmp->paddr - addr;
		if (space > max_space)
		    max_space = space;

		addr = tmp->paddr + tmp->size;
	}

	if ((pool->end - addr) > max_space)
	    max_space = pool->end - addr;

    pool->max_slice = max_space;

	return max_space;
}

/* Get the memory statistic
 */
static void __mem_get_counter(pool_t *pool, mem_counter_t *counter)
{
    counter->memory_sz = pool->end - pool->start;
    counter->alloc_cnt = pool->mem_cnt;
	counter->max_slice = (int)mem_get_maxspace(pool);
    counter->clean_space = pool->end - pool->clear_addr;
    counter->wait_cnt = pool->mem_wait_cnt;
    counter->dma_llp_cnt = llp_pool.mem_cnt;
}

/*
 * Memory allocation
 */
static void *__mem_alloc(pool_t *pool, int size, int align, dma_addr_t *phy_addr, mem_type_t mem_type)
{
    struct vm_struct_s **p, *tmp, *area;
	unsigned int addr, end = pool->end;
	gfp_t   gfp_flags = GFP_KERNEL;
	int     retry_count = 5, retry = 0;

    if (unlikely(!pool->init_done)) {
        printk("cpucomm: %s, mem_alloc is not ready yet! \n", pool->vm_name);
        dump_stack();
        return NULL;
    }

	align = (align >= CACHE_LINE_SZ) ? align : CACHE_LINE_SZ;
	addr = ALIGN(pool->start, align);
	size = CACHE_ALIGN(size);

	//in interrupt context
	if (mem_type & MEM_TYPE_ATOMIC)
	    gfp_flags = GFP_ATOMIC;

    area = (struct vm_struct_s *)kmem_cache_alloc(pool->vm_cache, gfp_flags);
	if (unlikely(!area))
		return NULL;

    memset(area, 0, sizeof(struct vm_struct_s));

retry:
    SEMA_LOCK(pool);

    for (p = &pool->p_vmlist; (tmp = *p) != NULL ;p = &tmp->next) {
		if (tmp->paddr < addr) {
			if (tmp->paddr + tmp->size >= addr)
				addr = ALIGN(tmp->size + tmp->paddr, align);
			continue;
		}
		if ((size + addr) < addr)
			goto out;
		if (size + addr <= tmp->paddr)
			goto found;
		addr = ALIGN(tmp->size + tmp->paddr, align);
		if (addr > end - size)
			goto out;
	}

found:
	area->paddr = addr;
	area->size = size;

	/* virtual memory */
	if (mem_type & MEM_TYPE_CB) {
		area->vaddr = (unsigned int)__va(area->paddr);
	}
    else {
#ifdef USE_STATIC_VA
        area->vaddr = (unsigned int)(pool->va_start + (area->paddr - pool->start));
        area->ioremap = 0;
#else
        area->vaddr = (unsigned int)ioremap_nocache(area->paddr, area->size);
        if (unlikely(!area->vaddr))
			goto out;
        area->ioremap = 1;
#endif
    }

    /* form the link list */
	area->next = *p;
	*p = area;
	pool->mem_cnt ++;
	if (addr + size > pool->clear_addr)
		pool->clear_addr = addr + size;

	SEMA_UNLOCK(pool);

	*phy_addr = area->paddr;

	/* alignment check */
	if (area->paddr & CACHE_LINE_MASK)
	    panic("area->paddr = 0x%x \n", area->paddr);

	return (void *)area->vaddr;

out:
	SEMA_UNLOCK(pool);

#ifndef WIN32
	if (retry_count) {
	    DECLARE_WAITQUEUE(wait, current);

	    printk("%s, added to wait queue to wait memory..., rety: %d \n", __func__, ++ retry);
	    retry_count --;
	    pool->mem_wait_cnt ++;
	    set_current_state(TASK_UNINTERRUPTIBLE);
	    add_wait_queue(&pool->mem_alloc_wait, &wait);
	    schedule();
	    set_current_state(TASK_RUNNING);
	    remove_wait_queue(&pool->mem_alloc_wait, &wait);
	    goto retry;
	}
#endif
	kmem_cache_free(pool->vm_cache, area);

	printk("cpu_comm: allocation failed: request size:%d!\n", size);
	dump_memory(pool);

	return NULL;
}

/*
 * Memory free function
 */
void __mem_free(pool_t *pool, void *vaddr)
{
	struct vm_struct_s **p, *tmp;
	int	bFound = 0;

    SEMA_LOCK(pool);

	for (p = &pool->p_vmlist ; (tmp = *p) != NULL ;p = &tmp->next) {
		if (tmp->vaddr == (unsigned int)vaddr) {
		    if (tmp->ioremap)
		        __iounmap((void *)tmp->vaddr);

			bFound = 1;
			break;
		}
	}

    if (unlikely(!bFound))
		panic("%s, invalid pointer: 0x%x \n", __func__, (u32)vaddr);

	*p = tmp->next;
	kmem_cache_free(pool->vm_cache, tmp);
    pool->mem_cnt --;

	SEMA_UNLOCK(pool);

#ifndef WIN32
    /* wake up all wakers */
    wake_up(&pool->mem_alloc_wait);
#endif
}

/*
 * Exit function
 */
void __mem_alloc_exit(pool_t *pool)
{
	if (unlikely(pool->mem_cnt))
		printk("Memory Leakage!!!! \n");

    if (pool == &local_pool)
	    frm_free_buf_ddr(&pool_info);

    if (pool == &pci_pool)
        __iounmap((void *)pool->va_start);

    kmem_cache_destroy(pool->vm_cache);

    printk("%s: memory is freed. \n", __func__);
}

int mem_get_poolinfo(unsigned int *paddr, unsigned int *size)
{
    return __mem_get_poolinfo(&local_pool, paddr, size);
}

/*
 * Dump memory statistic
 */
void dump_memory(pool_t *pool)
{
	printk("The max hole space is %d bytes \n", mem_get_maxspace(pool));
}

void mem_get_counter(mem_counter_t *counter)
{
    __mem_get_counter(&local_pool, counter);
}

/*
 * Memory init function
 */
int mem_alloc_init(unsigned int pool_size, char *name)
{
	//step 1, allocate pages
	memset(&pool_info, 0, sizeof(struct frammap_buf_info));
	pool_info.size = pool_size;
#ifdef USE_STATIC_VA
    pool_info.alloc_type = ALLOC_NONCACHABLE;
#else
    pool_info.alloc_type = ALLOC_CACHEABLE;
#endif
    pool_info.align = 128;
    pool_info.name = "cpu_comm";
    if (frm_get_buf_ddr(DDR_ID_SYSTEM, &pool_info)) {
        panic("%s, allocate memory %d from frammap fail! \n", __func__, pool_size);
        return -1;
    }
    /* end of step 1 */

	memset(&local_pool, 0, sizeof(struct pool));
    local_pool.start = pool_info.phy_addr;
	local_pool.end = local_pool.start + pool_size;
	local_pool.clear_addr = local_pool.start;
	local_pool.va_start = (unsigned int)pool_info.va_addr;
	strcpy(local_pool.vm_name, name);
    local_pool.vm_cache = kmem_cache_create(local_pool.vm_name, sizeof(struct vm_struct_s), 0, 0, NULL);
    if (!local_pool.vm_cache)
        panic("%s: create vm_cache fail! \n", __func__);
    sema_init(&local_pool.sema, 1);
#ifndef WIN32
    init_waitqueue_head(&local_pool.mem_alloc_wait);
#endif
    printk("%s: physical memory begins at 0x%x, ends at 0x%x, 0x%x bytes. \n", __func__,
        local_pool.start, local_pool.end, pool_size);

    local_pool.init_done = 1;

	return 0;
}

int mem_alloc_pool_resize(unsigned int new_pool_size)
{
    pool_t  *pool = &local_pool;
    u32 new_end = pool->start + new_pool_size;
    int ret = 0;

    SEMA_LOCK(pool);
    /* check clean address */
    if (pool->clear_addr < new_end) {
        pool->end = new_end;
    } else {
        ret = -1;
    }
    SEMA_UNLOCK(pool);

    if (ret == -1)
        panic("%s, org-end: 0x%x, new-end:0x%x \n", __func__, pool->end, new_end);

    return ret;
}

/*
 * Memory allocation
 */
void *mem_alloc(int size, int align, dma_addr_t *phy_addr, mem_type_t mem_type)
{
    return __mem_alloc(&local_pool, size, align, phy_addr, mem_type);
}

void mem_free(void *vaddr)
{
    __mem_free(&local_pool, vaddr);
}

void mem_alloc_exit(void)
{
    __mem_alloc_exit(&local_pool);
}

//-------------------------------------------------------------------------------
// ---------- PCI memory functions ----------------------------------------------
//-------------------------------------------------------------------------------
int pci_mem_get_poolinfo(unsigned int *paddr, unsigned int *size)
{
    return __mem_get_poolinfo(&pci_pool, paddr, size);
}

void pci_mem_get_counter(mem_counter_t *counter)
{
    __mem_get_counter(&pci_pool, counter);
}

/*
 * Memory init function
 */
int pci_mem_alloc_init(unsigned int start_addr, unsigned int pool_size, char *name)
{
    memset(&pci_pool, 0, sizeof(struct pool));

    if (!start_addr && !pool_size) {
        strcpy(pci_pool.vm_name, name);
        return 0;   //do nothing
    }

	/* because the start_addr is from Host's memory, so we use PCI_DDR_HOST_BASE */
    if (!start_addr || (start_addr < PCI_DDR_HOST_BASE)) {
        printk("%s, error base 0x%x! \n", __func__, start_addr);
        return -1;
    }

	pci_pool.start = start_addr;
	pci_pool.end = pci_pool.start + pool_size;
	pci_pool.clear_addr = pci_pool.start;
	pci_pool.va_start = (unsigned int)ioremap_nocache(start_addr, pool_size);
	if (!pci_pool.va_start)
	    panic("%s, no virtual memory for pci pool sz 0x%x\n", __func__, pool_size);

	strcpy(pci_pool.vm_name, name);
    pci_pool.vm_cache = kmem_cache_create(pci_pool.vm_name, sizeof(struct vm_struct_s), 0, 0, NULL);
    if (!pci_pool.vm_cache)
        panic("%s: create vm_cache fail! \n", __func__);
    sema_init(&pci_pool.sema, 1);
#ifndef WIN32
    init_waitqueue_head(&pci_pool.mem_alloc_wait);
#endif
    printk("%s: physical memory begins at 0x%x, ends at 0x%x, %d bytes. \n", __func__,
        pci_pool.start, pci_pool.end, pool_size);

    pci_pool.init_done = 1;
	return 0;
}

/*
 * Memory allocation
 */
void *pci_mem_alloc(int size, int align, dma_addr_t *phy_addr, mem_type_t mem_type)
{
    return __mem_alloc(&pci_pool, size, align, phy_addr, mem_type);
}

void pci_mem_free(void *vaddr)
{
    __mem_free(&pci_pool, vaddr);
}

void pci_mem_alloc_exit(void)
{
    __mem_alloc_exit(&pci_pool);
}

//-------------------------------------------------------------------------------
// ---------- DMA functions ----------------------------------------------
//-------------------------------------------------------------------------------
int mem_llp_alloc_init(unsigned int pool_size, int fix_size)
{
	pool_size = PAGE_ALIGN(pool_size);

    if (fix_size & 0x1F)
        panic("%s, size: %d \n", __func__, fix_size);

    memset(&llp_pool, 0, sizeof(llp_pool));

    llp_pool.vaddr = (unsigned int)mem_alloc(pool_size, 32, &llp_pool.paddr, MEM_TYPE_NCNB);
    if (!llp_pool.vaddr)
        panic("error in allocate memory! \n");

    if (llp_pool.paddr % 32)
        panic("The address: 0x%x is not alignment to 32! \n", llp_pool.paddr);

    llp_pool.free_cnt = pool_size / fix_size;
    if (llp_pool.free_cnt > 8 * sizeof(llp_pool.bit_arry))
        panic("%s, array is too small! \n", __func__);

    llp_pool.fix_size = fix_size;

    sema_init(&llp_sema, 1);

    llp_init_done = 1;

    return 0;
}

/*
 * Memory allocation
 */
void *mem_llp_alloc(dma_addr_t *phy_addr)
{
    unsigned int i, bfound = 0;
    void  *llp_vaddr = NULL;

    if (!llp_init_done)
        panic("%s, not ready yet! \n", __func__);

    down(&llp_sema);
    for (i = 0; i < llp_pool.free_cnt; i ++) {
        if (test_and_set_bit(i, (volatile void *)&llp_pool.bit_arry[0]) == 0) {
            bfound = 1;
            llp_pool.mem_cnt ++;
            break;
        }
    }
    up(&llp_sema);

    if (bfound) {
        *phy_addr = llp_pool.paddr + i * llp_pool.fix_size;
        llp_vaddr = (void *)(llp_pool.vaddr + i * llp_pool.fix_size);
        ((dmac_llp_t *)llp_vaddr)->llp_cnt = 0;
    }

    if (llp_vaddr == NULL)
        panic("%s, no llp memory! \n", __func__);

    if (*phy_addr & 0x7)
        panic("phy_addr = 0x%x is not aligned to 8! \n", *phy_addr);

    return llp_vaddr;
}

void mem_llp_free(void *vaddr)
{
    int i;

    i = ((u32)vaddr - llp_pool.vaddr) / llp_pool.fix_size;

    down(&llp_sema);
    if (test_and_clear_bit(i, (volatile void *)&llp_pool.bit_arry[0]) == 0)
        panic("Error in code! \n");
    llp_pool.mem_cnt --;
    up(&llp_sema);

    return;
}

int mem_llp_alloc_exit(void)
{
    down(&llp_sema);

    if (llp_pool.mem_cnt)
        panic("%s, memory leakage, mem_count = %d \n", __func__, llp_pool.mem_cnt);

    up(&llp_sema);

    mem_free((void *)llp_pool.vaddr);
    return 0;
}

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Two CPU communication memory management");
MODULE_LICENSE("GPL");

