#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/mempool.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <mach/fmem.h>
#include "frammap.h"

#define FRM_DEBUG   0

/* System memory default alignment
 */
static ushort sys_align = PAGE_ALIGN_ORDER;  /* 0 means PAGE alignment, 1: 1M alignment, 2: 2M alignment, .... 10:512k alignemnt */
module_param(sys_align, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sys_align, "System memory alignment");

/* DDR-1 memory default alignment
 */
static ushort ddr1_align = PAGE_ALIGN_ORDER; /* 0 means PAGE alignment, 1: 1M alignment, 2: 2M alignment, .... 10:512k alignemnt */
module_param(ddr1_align, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ddr1_align, "DDR-1 memory alignment");

/* DDR-2 memory default alignment
 */
static ushort ddr2_align = PAGE_ALIGN_ORDER; /* 0 means PAGE alignment, 1: 1M alignment, 2: 2M alignment, .... 10:512k alignemnt */
module_param(ddr2_align, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ddr2_align, "DDR-2 memory alignment");

/* declare memory size parameter
 */
static ushort ddr0 = 0;
module_param(ddr0, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ddr0, "system memory size in Mega");

static ushort ddr1 = 0;
module_param(ddr1, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ddr1, "DDR-1 memory size in Mega");

static ushort ddr2 = 0;
module_param(ddr2, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ddr2, "DDR-2 memory size in Mega");

/* for backward compitable */
typedef struct kmem_cache kmem_cache_t;

/* page information from fmem.c
 */
static g_page_info_t *p_page_info;

/* ddr information descriptor
 */
static ddr_desc_t ddr_desc[MAX_DDR_NR];
static ddr_desc_t lowmem_desc;

/* data for ap memory allocation
 */
static ap_private_data_t ap_private_data[MAX_DDR_NR];
static struct semaphore  ap_sema;
extern struct miscdevice misdev[];
extern struct file_operations frmmap_fops;

/*
 * Macro definitions
 */
#define err(format, arg...)     printk(KERN_ERR "frammap(%s): " format "\n" , __func__, ## arg)

/*
 * Local function declaration
 */
static ddr_id_t get_ddrID(int rmmp_index);
static int frmmap_give_freepgs(int ddr_id, int size);
static void frm_proc_release(void);
static int frm_proc_init(void);

char ap_devname[MAX_DDR_NR][20];

#include "map.c"

/*
 * get the node from open list by filep index
 *
 * This function must be executed in semaphore protection environment
 */
static ap_open_info_t *get_open_info(ap_private_data_t *ap_private_data, void *filep)
{
    ap_open_info_t  *openinfo = NULL;
    struct list_head *node;
    int bFound = 0;

    if (list_empty(&ap_private_data->open_list))
        return NULL;

    list_for_each(node, &ap_private_data->open_list) {
        openinfo = list_entry(node, ap_open_info_t, list);
        if ((u32)openinfo->filep != (u32)filep)
            continue;

        bFound = 1;
        break;
    }

    return bFound ? openinfo : NULL;
}

/*
 * @brief allocate virtual address based on the flag of cacheable
 *
 * @function u32 alloc_vaddr(memblk_hdr_t * entity, u32 vsize, int cacheable)
 * @param paddr indicate the physical address
 * @param vsize indicate the allocation size
 * @param cacheable indicate the MMU property
 * @return vaddr on success, 0 on error
*/
static void *alloc_vaddr(u32 paddr, u32 vsize, u32 flags)
{
    void *vaddr = NULL;

    if (flags & VM_F_WC)
        vaddr = (void *)ioremap_wc(paddr, (size_t) vsize);
    else if (flags & VM_F_NCNB) {
        vaddr = (void *)ioremap_nocache(paddr, (size_t) vsize);
    } else {
        /* direct mapping */
        vaddr = (void *)__va(paddr);
    }

    return vaddr;
}

/*
 * @brief get DDR ID of this module belonging to
 *
 * @function ddr_id_t get_ddrID(int rmmp_index)
 * @param rmmp_index indicates the user module index
 *
 * @return DDR id
 */
ddr_id_t get_ddrID(int rmmp_index)
{
    int i;

    /* check the exception first */
    switch (rmmp_index) {
    case FRMIDX_AP0:
    case FRMIDX_DDR0:
        return DDR_ID_SYSTEM;

    case FRMIDX_AP1:
    case FRMIDX_DDR1:
        return DDR_ID1;

    case FRMIDX_AP2:
    case FRMIDX_DDR2:
        return DDR_ID2;
    default:
        break;
    }

    for (i = 1; i < ARRAY_SIZE(id_string_arry); i++) {
        if (id_string_arry[i].rmmp_index == rmmp_index)
            return id_string_arry[i].ddr_id;
    }

    return DDR_ID_SYSTEM;
}

/*
 * Get a max slot between the memory start and end
 */
static unsigned int get_max_slice(int idx)
{
    struct vm_struct **p, *tmp;
	unsigned int addr = ddr_desc[idx].mem_base, space, max_space = 0;

	for (p = &ddr_desc[idx].vmlist; (tmp = *p) != NULL ;p = &tmp->next) {
		space = tmp->phys_addr - addr;
		if (space > max_space)
		    max_space = space;

		addr = tmp->phys_addr + tmp->size;
	}

	if ((ddr_desc[idx].mem_end - addr) > max_space)
	    max_space = ddr_desc[idx].mem_end - addr;

	return max_space;
}

/*
 * get memory from kernel's low memory
 */
int frm_get_lowmem(frammap_buf_t *info)
{
    unsigned int size;
    unsigned long flags = 0;
    struct vm_struct **p, *tmp, *area;
    int ret = -1;

    area = (struct vm_struct *)kmem_cache_alloc(lowmem_desc.vm_cache, GFP_KERNEL);
    if (unlikely(!area)) {
        err("%s, no slab cache memory .... \n", __func__);
        return ret;
    }
    memset(area, 0, sizeof(struct vm_struct));

    switch (info->alloc_type) {
      case ALLOC_CACHEABLE:
        //flags = PAGE_COPY;
        flags = PAGE_SHARED;
        break;
      case ALLOC_BUFFERABLE:
        flags = pgprot_writecombine(pgprot_kernel);
        area->flags = VM_F_WC;
        break;
      default:
      case ALLOC_NONCACHABLE:
        flags = pgprot_noncached(pgprot_kernel);
        area->flags = VM_F_NCNB;
        break;
    }

    size = ALIGN(info->size, lowmem_desc.align);
    area->addr = (void *)fmem_alloc_ex(size, &area->phys_addr, flags, DDR_ID_SYSTEM);
    if (area->addr == NULL) {
        kmem_cache_free(lowmem_desc.vm_cache, (void *)area);
        return ret;
    }

    area->size = size;
    area->caller = (void *)kmalloc(NAME_SZ, GFP_KERNEL);
    strcpy((char *)area->caller, info->name);
    area->flags |= (info->align & ADDR_ALIGN_MASK);

    down(&lowmem_desc.sema);

    /* move to the end of list and form the link list
     * Note: the list is not sorted.
     */
    for (p = &lowmem_desc.vmlist; (tmp = *p) != NULL ;p = &tmp->next) {}
    area->next = *p;
    *p = area;

    lowmem_desc.mem_cnt ++;
    lowmem_desc.allocate += size;

    up(&lowmem_desc.sema);

    /* update the return info */
    info->phy_addr = area->phys_addr;
    info->va_addr = area->addr;
    ret = 0;

    return ret;
}

/*
 * free memory from kernel's low memory
 */
int frm_free_lowmem(void *vaddr)
{
    struct vm_struct **p, *tmp;
    int bFound = 0;

    /* take semaphore */
	down(&lowmem_desc.sema);

    for (p = &lowmem_desc.vmlist ; (tmp = *p) != NULL ;p = &tmp->next) {
	    if ((u32)tmp->addr == (u32)vaddr) {
		    bFound = 1;
		    break;
	    }
    }

	if (bFound) {
		*p = tmp->next;
		if (tmp->caller)
			kfree(tmp->caller);

        /* free memory */
        fmem_free_ex(tmp->size, tmp->addr, tmp->phys_addr);

        lowmem_desc.allocate -= tmp->size;
		lowmem_desc.mem_cnt --;
		kmem_cache_free(lowmem_desc.vm_cache, tmp);
	}

	/* release semaphore */
    up(&lowmem_desc.sema);

    return !bFound;
}

/**
 * @brief de-allocate a buffer. It is a pair of frm_release_buf_info(). Note: The return value
 * @brief info->size will be update according to the real allocated size.
 *
 * @parm rmmp_index indicates the unique module id given by FRAMMAP.
 * @parm info->size indicates the allocated memory size.
 *
 * @return  0 for success, otherwise for fail.
 */
int frm_get_buf_info(u32 rmmp_index, struct frammap_buf_info *info)
{
    struct vm_struct **p, *tmp, *area;
    ddr_desc_t  *p_ddr_desc;
    ddr_id_t ddr_id;
    dma_addr_t  addr;
    unsigned long flags = 0;
    u32 len, size;

    if (info == NULL) {
        err("Caller passes null pointer! rmmp_index = %d!\n", rmmp_index);
        return -1;
    }

    if (info->size == 0) {
        err("Caller %s passes null pointer! \n", info->name);
        return -1;
    }

    /* init value */
    info->va_addr = NULL;
    info->phy_addr = 0;

    ddr_id = get_ddrID(rmmp_index);

    if (unlikely((ddr_id >= MAX_DDR_NR) || !ddr_desc[ddr_id].valid)) {
        err("%s get memory from DDR %d which is not active! \n", id_to_namestring(rmmp_index),
            ddr_id);
        return -1;
    }

    if (unlikely(!info->name)) {
        err("%s, the name is NULL! \n", __func__);
        return -1;
    }

    len = strlen(info->name);
    if (unlikely(len > NAME_SZ)) {
        err("%s, the name: %s is over size %d! \n", __func__, info->name, NAME_SZ);
        return -1;
    }

    /* 4 bytes alignment at least */
    if (info->align <= 4)
	    info->align = 4;

	/* check the align is 2^x */
	if (info->align & (info->align - 1)) {
		err("Caller %s passes a not 2 order's alignment 0x%x \n", info->name, info->align);
		return -1;
	}

    if (info->align & ~ADDR_ALIGN_MASK) {
		err("Caller %s passes oversize alignment 0x%x \n", info->name, info->align);
		return -1;
	}

	/*
	 * flags field defintion
	 * bit 28-31: alloc type
	 * bit 24-27: reserved
	 * bit 0-23: alloc address alignment
	 */
	flags |= (info->align & ADDR_ALIGN_MASK);

    p_ddr_desc = &ddr_desc[ddr_id];
    size = ALIGN(info->size, p_ddr_desc->align);

    if (size > (p_ddr_desc->mem_end - p_ddr_desc->mem_base)) {
        err("%s, the request size %dMB(0x%x) is too large! \n", __func__, size/1048576, size);
        show_ddr_info();
        return -1;
    }

    switch (info->alloc_type) {
    case ALLOC_CACHEABLE:
        break;
    case ALLOC_BUFFERABLE:
        flags |= VM_F_WC;
        break;
    default:
        printk("warning: %s, the alloc type is not given, default is NCNB! \n", info->name);
    case ALLOC_NONCACHABLE:
        flags |= VM_F_NCNB;
        break;
    }

    addr = ALIGN(p_ddr_desc->mem_base, info->align);
    if (addr >= p_ddr_desc->mem_end) {
        err("%s, caller %s allocates memory by wrong align %d! \n", __func__, info->name, info->align);
        return -1;
    }

    area = (struct vm_struct *)kmem_cache_alloc(ddr_desc[ddr_id].vm_cache, GFP_KERNEL);
    if (unlikely(!area)) {
        err("%s, no slab cache memory .... \n", __func__);
        return -1;
    }

    memset(area, 0, sizeof(struct vm_struct));

    down(&p_ddr_desc->sema);

    for (p = &p_ddr_desc->vmlist; (tmp = *p) != NULL ;p = &tmp->next) {
		if (tmp->phys_addr < addr) {
			if (tmp->phys_addr + tmp->size >= addr)
				addr = ALIGN(tmp->size + tmp->phys_addr, info->align);
			continue;
		}
		if ((size + addr - 1) < addr)
			goto out;
		if (size + addr <= tmp->phys_addr)
			goto found;
		addr = ALIGN(tmp->size + tmp->phys_addr, info->align);
		if (addr > p_ddr_desc->mem_end - size)
			goto out;
	}

found:
	area->phys_addr = addr;
	area->size = size;
    area->flags = flags;
    area->nr_pages = size >> PAGE_SHIFT;

    if (flags & VM_F_IOREMAP) {
        area->addr = alloc_vaddr(area->phys_addr, area->size, flags);
        if (unlikely(!area->addr)) {
            err("%s, no virtual memory! \n", __func__);
            goto out;
        }
    } else {
        area->addr = (void *)__va(area->phys_addr);
    }

    area->caller = (void *)kmalloc(NAME_SZ, GFP_KERNEL);
    strcpy((char *)area->caller, info->name);

    /* form the lisk list */
    area->next = *p;
    *p = area;
    p_ddr_desc->mem_cnt ++;
    if (addr + size > p_ddr_desc->clear_addr)
        p_ddr_desc->clear_addr = addr + size;

    /* count the allocated memory */
	p_ddr_desc->allocate += size;

    up(&p_ddr_desc->sema);

    info->va_addr = (void *)area->addr;
    info->phy_addr = area->phys_addr;

    return 0;

out:
    up(&p_ddr_desc->sema);

    if (area->caller)
        kfree(area->caller);

    kmem_cache_free(p_ddr_desc->vm_cache, (void *)area);

    if ((ddr_id == DDR_ID_SYSTEM) && (info->align <= PAGE_SIZE)) {
        printk("Frammap: No memory for rmmp_index=%d(%s) with size = %d bytes(free: %d)! "
                   "Try to allocate kernel memory! \n",
                   rmmp_index, id_to_namestring(rmmp_index), size, p_ddr_desc->mem_end - p_ddr_desc->clear_addr);

        if (frm_get_lowmem(info) == 0)  //successful
            return 0;
    }

    printk("FRAMMAP ddr name: %s \n", p_ddr_desc->device_name);
	printk("    base: 0x%x \n", p_ddr_desc->mem_base);
	printk("    end: 0x%x \n", p_ddr_desc->mem_end);
	printk("    size: 0x%x bytes\n", size);
	printk("    memory allocated: 0x%x bytes \n", p_ddr_desc->allocate);
	printk("    memory free: 0x%x bytes \n", size - p_ddr_desc->allocate);
    return -1;
}

EXPORT_SYMBOL(frm_get_buf_info);


/**
 * @brief de-allocate a buffer. It is a pair of frm_get_buf_info()
 *
 * @parm rmmp_index indicates the unique module id given by FRAMMAP. Besides, both info->va_addr and
 *          info->phy_addr are necessary.
 *
 * @return  0 for success, otherwise for fail.
 */
int frm_release_buf_info(void *vaddr)
{
    int ddr_id;
    struct vm_struct **p, *tmp;
    ddr_desc_t  *p_ddr_desc;
	int	bFound = 0;

    for (ddr_id = 0; ddr_id < p_page_info->nr_node; ddr_id ++) {
        p_ddr_desc = &ddr_desc[ddr_id];

		/* take semaphore */
		down(&p_ddr_desc->sema);

	    for (p = &p_ddr_desc->vmlist ; (tmp = *p) != NULL; p = &tmp->next) {
		    if ((u32)tmp->addr == (u32)vaddr) {
			    bFound = 1;
			    break;
		    }
	    }

		if (bFound) {
			*p = tmp->next;
			if (tmp->caller)
				kfree(tmp->caller);

            /* release the virtual address */
            if (tmp->flags & VM_F_IOREMAP)
                __iounmap((void *)tmp->addr);

            p_ddr_desc->allocate -= tmp->size;
			p_ddr_desc->mem_cnt --;
			kmem_cache_free(p_ddr_desc->vm_cache, tmp);
		}

		/* release semaphore */
        up(&p_ddr_desc->sema);

        if (bFound)
            break;
    }

	/* If the memory can't be found, go through low memory link list
	 */
	if (!bFound) {
	    /* free the memory from lowmem link list */
	    if (frm_free_lowmem(vaddr))
		    panic("%s, invalid pointer: 0x%x \n", __func__, (u32)vaddr);
	}

    return 0;
}

EXPORT_SYMBOL(frm_release_buf_info);

/**
 * @brief allocate a buffer from the designated DDR. It is a pair of frm_free_buf_ddr().
 * @brief Note: the return value info->size will be update according to the real allocated size.
 *
 * @parm ddr_id is used to indicate the designated DDR
 *
 * @parm info->size in info is necessary.
 * @return  0 for success, otherwise for fail.
 */
int frm_get_buf_ddr(ddr_id_t ddr_id, struct frammap_buf_info *info)
{
    int rmmp_index, ret;

    switch (ddr_id) {
    case DDR_ID_SYSTEM:
        rmmp_index = FRMIDX_DDR0;
        break;
    case DDR_ID1:
        rmmp_index = FRMIDX_DDR1;
        break;
    case DDR_ID2:
        rmmp_index = FRMIDX_DDR2;
        break;
    default:
        return -1;
    }

    ret = frm_get_buf_info(rmmp_index, info);

    return ret;
}

EXPORT_SYMBOL(frm_get_buf_ddr);

/**
 * @brief de-allocate a buffer from the designated DDR. It is a pair of frm_get_buf_ddr()
 *
 * @parm both info->va_addr and info->phy_addr are necessary.
 * @return  0 for success, otherwise for fail.
 */
int frm_free_buf_ddr(void *vaddr)
{
    return frm_release_buf_info(vaddr);
}

EXPORT_SYMBOL(frm_free_buf_ddr);

/**
 * @brief put the remaining unused memory to the kernel
 *
 * @parm ddr_id indicate whihc DDR will give the un-used pages to the kernel
 * @return  0 for success, otherwise for fail.
 */
int frmmap_give_freepgs(int ddr_id, int size)
{
    int give_sz, ret;

    if (unlikely(!ddr_desc[ddr_id].valid)) {
        printk("ddr%d is not active! \n", ddr_id);
        return -1;
    }

    if (!size)
        size = ddr_desc[ddr_id].mem_end - ddr_desc[ddr_id].clear_addr;

    give_sz = (size >> PAGE_SHIFT) << PAGE_SHIFT;
    ret = fmem_give_pages(ddr_id, give_sz);

    if (ret > 0) {
        /* update the ddr database */
        ddr_desc[ddr_id].mem_end = ddr_desc[ddr_id].mem_base + p_page_info->node_sz[ddr_id];
        if (ddr_desc[ddr_id].clear_addr > ddr_desc[ddr_id].mem_end)
            ddr_desc[ddr_id].clear_addr = ddr_desc[ddr_id].mem_end;

        printk("Frammap: %d pages in DDR%d are freed. \n", (ret >> PAGE_SHIFT), ddr_id);
        return 0;
    }

    return -1;
}

/**
 * @brief to resolve the virtual address (including direct mapping, ioremap or user space address to
 *      its real physical address.
 *
 * @parm vaddr indicates any virtual address
 *
 * @return  >= 0 for success, 0xFFFFFFFF for fail
 */
phys_addr_t frm_va2pa(u32 vaddr)
{
    return fmem_lookup_pa(vaddr);
}
EXPORT_SYMBOL(frm_va2pa);

void init_lowmem(void)
{
    memset(&lowmem_desc, 0, sizeof(lowmem_desc));
    strcpy(lowmem_desc.device_name, "kernel_lowmem");

    lowmem_desc.vm_cache = kmem_cache_create("vmlist_lowmem", sizeof(struct vm_struct), 0, 0, NULL);
    if (unlikely(!lowmem_desc.vm_cache))
        panic("Fail to create vm_cache! \n");

    lowmem_desc.align = ddr_desc[DDR_ID_SYSTEM].align;

    sema_init(&lowmem_desc.sema, 1);
    lowmem_desc.valid = 1;
}

#include <linux/kernel.h>

#include <asm/io.h>
#include <asm/uaccess.h>
/*
 * Entry point of frammap
 */
static char name[MAX_DDR_NR][20];
static int __init frammap_init(void)
{
    int ddr_idx, i, align, ret;
    u32 parameter_ddr_sz = 0;
    frm_cpu_id_t cid;
    frm_pci_id_t pid;

    if (frm_get_identifier(&pid, &cid))
        return -1;

    memset(&ddr_desc[0], 0, sizeof(ddr_desc));
    memset(misdev, 0, MAX_DDR_NR * sizeof(struct miscdevice));
    memset(&ap_private_data[0], 0x0, MAX_DDR_NR * sizeof(ap_private_data_t));

    /* Get page related information from kernel.
     */
    fmem_get_pageinfo(&p_page_info);

    /*
     * Prepare the database for the DDR related information.
     */
    for (ddr_idx = 0; ddr_idx < p_page_info->nr_node; ddr_idx++) {
        /* clear to zero */
        memset(&ddr_desc[ddr_idx], 0, sizeof(ddr_desc_t));

		ddr_desc[ddr_idx].mem_base = p_page_info->phy_start[ddr_idx];
		ddr_desc[ddr_idx].clear_addr = ddr_desc[ddr_idx].mem_base;
		ddr_desc[ddr_idx].mem_end = p_page_info->phy_start[ddr_idx] + p_page_info->node_sz[ddr_idx];

        /* specify the alignment */
        switch (ddr_idx) {
        case 0:
            align = sys_align;
            parameter_ddr_sz = ddr0 << 20;
            break;
        case 1:
            align = ddr1_align;
            parameter_ddr_sz = ddr1 << 20;
            break;
        case 2:
            align = ddr2_align;
            parameter_ddr_sz = ddr2 << 20;
            break;
        default:
            panic("bug here! \n");
            break;
        }

        sprintf(ddr_desc[ddr_idx].device_name, "frammap%d", ddr_idx);
        ddr_desc[ddr_idx].align = (0x1000 << align);
        memset(name[ddr_idx], 0, 20);
        sprintf(name[ddr_idx], "frammap_vm%d", ddr_idx);
        ddr_desc[ddr_idx].vm_cache = kmem_cache_create(name[ddr_idx], sizeof(struct vm_struct), 0, 0, NULL);
        sema_init(&ddr_desc[ddr_idx].sema, 1);
        ddr_desc[ddr_idx].valid = 1;

        if (parameter_ddr_sz != 0) {
            if (parameter_ddr_sz > p_page_info->node_sz[ddr_idx]) {
                printk("Frammap: DDR%d size only has %dM bytes! \n", ddr_idx,
                       p_page_info->node_sz[ddr_idx] >> 20);
                return -1;
            }

            /* at least one page left, then we can release memory */
            if ((parameter_ddr_sz + PAGE_SIZE) < p_page_info->node_sz[ddr_idx])
                frmmap_give_freepgs(ddr_idx, p_page_info->node_sz[ddr_idx] - parameter_ddr_sz);
        }

        printk("Frammap: DDR%d: memory base=0x%x, memory size=0x%x, align_size = %dK. \n", ddr_idx,
               ddr_desc[ddr_idx].mem_base, ddr_desc[ddr_idx].mem_end - ddr_desc[ddr_idx].mem_base,
               ddr_desc[ddr_idx].align >> 10);
    }

    /* low memory descriptor */
    init_lowmem();

    sema_init(&ap_sema, 1);

    printk("Frammap: version %s, and the system has %d DDR.\n", FRM_VERSION_CODE, p_page_info->nr_node);

    frm_proc_init();
    frmmap_read_cfgfile(CFG_FILE_PATH);

    /*
     * create memory node for AP to allocate memory or IOCTL
     */
    for (i = 0; i < p_page_info->nr_node; i++) {
        misdev[i].minor = FRAMMAP_MINOR + i;
        misdev[i].name = ddr_desc[i].device_name;
        misdev[i].fops = (struct file_operations *)&frmmap_fops;
        ret = misc_register(&misdev[i]);
        if (ret < 0) {
            printk("Frammap: register misc device %d fail! \n", i);
            memset(&misdev[i], 0, sizeof(struct miscdevice));   /* reset the structure */
        }
    }

    return 0;
}

/*
 * @This function finds the address which is allocated from frammap.
 *
 * @function int frm_find_addrinfo(frammap_buf_t *bufino, int by_phyaddr)
 *
 * @param: input: bufino->phy_addr or bufinfo->vaddr
 *                by_phyaddr 1 if searched by physical address, 0 for by virtual address.
 *         output: bufino
 * @return value: 0 for success, -1 for fail
 *
 * someone may lookup the address in atomic context, so remove the semaphore protection
 */
int frm_get_addrinfo(frammap_buf_t *bufinfo, int by_phyaddr)
{
    int ddr_id;
    struct vm_struct **p, *tmp;
    ddr_desc_t  *p_ddr_desc;
    dma_addr_t paddr;
	int	bFound = 0;

#if 0
    if (in_interrupt() || in_atomic()) {
        printk("%s: in interrupt or atomic context is not allowed! \n", __func__);
        return -1;
    }
#endif

    if (by_phyaddr && !bufinfo->phy_addr) {
        printk("%s: pass zero physical address! \n", __func__);
        return -1;
    }

    if (!by_phyaddr && !bufinfo->va_addr) {
        printk("%s: pass zero virtual address! \n", __func__);
        return -1;
    }

    if (by_phyaddr)
        paddr = bufinfo->phy_addr;
    else
        paddr = frm_va2pa((u32)bufinfo->va_addr);

    if (paddr == 0xFFFFFFFF)
        return -1;

    for (ddr_id = 0; ddr_id < p_page_info->nr_node; ddr_id ++) {
        p_ddr_desc = &ddr_desc[ddr_id];

        if (paddr < p_ddr_desc->mem_base || paddr > p_ddr_desc->mem_end)
            continue;
#if 0
		/* take semaphore */
		down(&p_ddr_desc->sema);
#endif
	    for (p = &p_ddr_desc->vmlist ; (tmp = *p) != NULL; p = &tmp->next) {
            /* compared by physical address */
		    if ((paddr >= tmp->phys_addr) && (paddr < (tmp->phys_addr + tmp->size))) {
			    bFound = 1;
			    break;
		    }
	    }
#if 0
		/* release semaphore */
        up(&p_ddr_desc->sema);
#endif
        if (bFound)
            break;
    }

	/* If the memory can't be found, go through low memory link list
	 */
	if (!bFound) {
#if 0
        /* take semaphore */
    	down(&lowmem_desc.sema);
#endif
        for (p = &lowmem_desc.vmlist ; (tmp = *p) != NULL ;p = &tmp->next) {
            /* compared by physical address */
		    if ((paddr >= tmp->phys_addr) && (paddr < (tmp->phys_addr + tmp->size))) {
			    bFound = 1;
			    break;
		    }
        }
#if 0
    	/* release semaphore */
        up(&lowmem_desc.sema);
#endif
	}

    if (bFound) {
        u32 offset = paddr - tmp->phys_addr;
        u32 vaddr = (u32)bufinfo->va_addr;

	    memset(bufinfo, 0x0, sizeof(frammap_buf_t));
		bufinfo->phy_addr = paddr;
		bufinfo->va_addr = by_phyaddr ? tmp->addr + offset : (void *)vaddr;
		bufinfo->size = (u32)tmp->size;
		bufinfo->align = (u32)tmp->flags & ADDR_ALIGN_MASK;
		bufinfo->name = (char *)tmp->caller;
		if (tmp->flags & VM_F_WC)
		    bufinfo->alloc_type = ALLOC_BUFFERABLE;
		else if (tmp->flags & VM_F_NCNB)
		    bufinfo->alloc_type = ALLOC_NONCACHABLE;
		else
		    bufinfo->alloc_type = ALLOC_CACHEABLE;
	}

    return bFound ? 0 : -1;
}
EXPORT_SYMBOL(frm_get_addrinfo);

static void __exit frammap_exit(void)
{
    int ddr_idx;
    ddr_desc_t  *p_ddr_desc;

    frm_proc_release();

    /* release the device node */
    for (ddr_idx = 0; ddr_idx < p_page_info->nr_node; ddr_idx ++) {

        p_ddr_desc = &ddr_desc[ddr_idx];
		if (unlikely(p_ddr_desc->mem_cnt))
			err("ddr%d has %d memory leakage! \n", ddr_idx, p_ddr_desc->mem_cnt);

		kmem_cache_destroy(ddr_desc[ddr_idx].vm_cache);
        ddr_desc[ddr_idx].vm_cache = NULL;

        if (misdev[ddr_idx].name != NULL)
            misc_deregister(&misdev[ddr_idx]);

        ddr_desc[ddr_idx].valid = 0;
    }

    if (unlikely(lowmem_desc.mem_cnt))
        err("Lowmem has %d memory leakage! \n", lowmem_desc.mem_cnt);

    kmem_cache_destroy(lowmem_desc.vm_cache);
    lowmem_desc.valid = 0;

    return;
}

/**
 * @brief Get memory allocation alignment.
 * @parm None
 * @return return the memory alignment. It is suitable for system memory.
 */
u32 frm_get_sysmem_alignment(void)
{
    return ddr_desc[0].align;
}

/*
 * @This function gets the managed memory allocation alignment.
 *
 * @function u32 frm_get_memory_alignment(void)
 * @param: ddr_id: ddr idx from 0
 * @return value: 2^align * 4K or 0 for fail
 */
u32 frm_get_memory_alignment(ddr_id_t ddr_id)
{
    u32 ret = 0;

    if (ddr_id >= MAX_DDR_NR)
        return 0;

    if (ddr_desc[ddr_id].valid)
        ret = ddr_desc[ddr_id].align;

    return ret;
}
EXPORT_SYMBOL(frm_get_sysmem_alignment);

/*
 * @This function is used to read CPU id and pci id
 * @Return value: 0 for success, -1 for fail
 */
int frm_get_identifier(frm_pci_id_t *pci_id, frm_cpu_id_t *cpu_id)
{
    int ret = 0;
    fmem_pci_id_t p_id;
    fmem_cpu_id_t c_id;

    fmem_get_identifier(&p_id, &c_id);

    switch (c_id) {
      case FMEM_CPU_FA726:
        *cpu_id = FRM_CPU_FA726;
        break;
      case FMEM_CPU_FA626:
        *cpu_id = FRM_CPU_FA626;
        break;
      default:
        *cpu_id = FRM_CPU_UNKNOWN;
        ret = -1;
        break;
    }

    switch (p_id) {
      case FMEM_PCI_HOST:
        *pci_id = FRM_PCI_HOST;
        break;
      case FMEM_PCI_DEV0:
        *pci_id = FRM_PCI_DEV0;
        break;
      default:
        ret = -1;
        break;
    }

    return ret;
}
EXPORT_SYMBOL(frm_get_identifier);

static struct proc_dir_entry *frm_proc_root = NULL;
static struct proc_dir_entry *ddr_info = NULL;
static struct proc_dir_entry *mmap0 = NULL;
static struct proc_dir_entry *mmap1 = NULL;
static struct proc_dir_entry *mmap2 = NULL;
static struct proc_dir_entry *mmap_kernel = NULL;
static struct proc_dir_entry *config_file = NULL;
static struct proc_dir_entry *frammap_debug = NULL;
static struct proc_dir_entry *frammap_free_pages = NULL;
static struct proc_dir_entry *frammap_force_free_pages = NULL;

/**
 * @brief Show the DDR usage status and memory allocation distribution.
 *
 * @parm None.
 *
 * @return  None.
 */
void show_ddr_info(void)
{
    u32 max_slice, dirty_pages, clear_pages, size;
	int	ddr_idx;
	ddr_desc_t	*p_ddr_desc;

	for (ddr_idx = 0; ddr_idx < p_page_info->nr_node; ddr_idx++) {
        if (!ddr_desc[ddr_idx].valid)
            continue;
		p_ddr_desc = &ddr_desc[ddr_idx];

		size = p_ddr_desc->mem_end - p_ddr_desc->mem_base;
		dirty_pages = (p_ddr_desc->clear_addr - p_ddr_desc->mem_base) >> PAGE_SHIFT;
		clear_pages = (p_ddr_desc->mem_end - p_ddr_desc->clear_addr) >> PAGE_SHIFT;
		max_slice = get_max_slice(ddr_idx);

		printk("--------------------------------------------------------------------------\n");
		printk("ddr name: %s \n", p_ddr_desc->device_name);
		printk("base: 0x%x \n", p_ddr_desc->mem_base);
		printk("end: 0x%x \n", p_ddr_desc->mem_end);
		printk("size: 0x%x bytes\n", size);
		printk("memory allocated: 0x%x bytes \n", p_ddr_desc->allocate);
		printk("memory free: 0x%x bytes \n", size - p_ddr_desc->allocate);
		printk("max available slice: 0x%x bytes\n", max_slice);
		printk("clear address: 0x%x \n", p_ddr_desc->clear_addr);
		printk("dirty pages: %d \n", dirty_pages);
		printk("clean pages: %d \n", clear_pages);
		printk("size alignment: 0x%x \n", p_ddr_desc->align);
		printk("memory counter: %d \n", p_ddr_desc->mem_cnt);
	}
}

static int proc_read_ddr_info(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    u32 max_slice, dirty_pages, clear_pages, size;
	int	ddr_idx, len = 0;
	ddr_desc_t	*p_ddr_desc;

	for (ddr_idx = 0; ddr_idx < p_page_info->nr_node; ddr_idx++) {
        if (!ddr_desc[ddr_idx].valid)
            continue;
		p_ddr_desc = &ddr_desc[ddr_idx];

		size = p_ddr_desc->mem_end - p_ddr_desc->mem_base;
		dirty_pages = (p_ddr_desc->clear_addr - p_ddr_desc->mem_base) >> PAGE_SHIFT;
		clear_pages = (p_ddr_desc->mem_end - p_ddr_desc->clear_addr) >> PAGE_SHIFT;
		max_slice = get_max_slice(ddr_idx);

		len += sprintf(page + len, "----------------------------------------------------------------\n");
		len += sprintf(page + len, "ddr name: %s \n", p_ddr_desc->device_name);
		len += sprintf(page + len, "base: 0x%x \n", p_ddr_desc->mem_base);
		len += sprintf(page + len, "end: 0x%x \n", p_ddr_desc->mem_end);
		len += sprintf(page + len, "size: 0x%x bytes\n", size);
		len += sprintf(page + len, "memory allocated: 0x%x bytes \n", p_ddr_desc->allocate);
		len += sprintf(page + len, "memory free: 0x%x bytes \n", size - p_ddr_desc->allocate);
		len += sprintf(page + len, "max available slice: 0x%x bytes\n", max_slice);
		len += sprintf(page + len, "memory allocate count: %d \n", p_ddr_desc->mem_cnt);
		len += sprintf(page + len, "clear address: 0x%x \n", p_ddr_desc->clear_addr);
		len += sprintf(page + len, "dirty pages: %d \n", dirty_pages);
		len += sprintf(page + len, "clear pages: %d \n", clear_pages);
		len += sprintf(page + len, "size alignment: 0x%x \n", p_ddr_desc->align);
	}

    /* kernel vmlist */
    if (lowmem_desc.mem_cnt) {
        len += sprintf(page + len, "----------------------------------------------------------------\n");
		len += sprintf(page + len, "ddr name: %s \n", lowmem_desc.device_name);
		len += sprintf(page + len, "memory allocated: 0x%x bytes \n", lowmem_desc.allocate);
		len += sprintf(page + len, "memory allocate count: %d \n", lowmem_desc.mem_cnt);
    }

    return len;
}

/* Read memory allocation distribution for System DDR
 */
static int proc_read_mmap0(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0, ddr_idx = DDR_ID_SYSTEM;
    ddr_desc_t	*p_ddr_desc;
	struct vm_struct **p, *tmp;

	p_ddr_desc = &ddr_desc[ddr_idx];

	len += sprintf(page + len,"\nMemory Callers Distribution in DDR%d\n", ddr_idx);
	len += sprintf(page + len,"--------------------------------------------------------------------------\n");
	for (p = &p_ddr_desc->vmlist ; (tmp = *p) != NULL ;p = &tmp->next) {
		len += sprintf(page + len, "name: %s, pa: 0x%x, va: 0x%x, size: 0x%x, flag: 0x%x \n",
			(char *)tmp->caller, (u32)tmp->phys_addr, (u32)tmp->addr, (u32)tmp->size, (u32)tmp->flags);
	}

    return len;
}

/* Read memory allocation distribution for DDR-1
 */
static int proc_read_mmap1(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0, ddr_idx = DDR_ID1;
    ddr_desc_t	*p_ddr_desc;
	struct vm_struct **p, *tmp;

	p_ddr_desc = &ddr_desc[ddr_idx];

	len += sprintf(page + len,"\nMemory Callers Distribution in DDR%d\n", ddr_idx);
	len += sprintf(page + len,"--------------------------------------------------------------------------\n");
	for (p = &p_ddr_desc->vmlist ; (tmp = *p) != NULL ;p = &tmp->next) {
		len += sprintf(page + len, "name: %s, pa: 0x%x, va: 0x%x, size: 0x%x, flag: 0x%x \n",
			(char *)tmp->caller, (u32)tmp->phys_addr, (u32)tmp->addr, (u32)tmp->size, (u32)tmp->flags);
	}

    return len;
}

/* Read memory allocation distribution for DDR-2
 */
static int proc_read_mmap2(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0, ddr_idx = DDR_ID2;
    ddr_desc_t	*p_ddr_desc;
	struct vm_struct **p, *tmp;

	p_ddr_desc = &ddr_desc[ddr_idx];

	len += sprintf(page + len,"\nMemory Callers Distribution in DDR%d\n", ddr_idx);
	len += sprintf(page + len,"--------------------------------------------------------------------------\n");
	for (p = &p_ddr_desc->vmlist ; (tmp = *p) != NULL ;p = &tmp->next) {
		len += sprintf(page + len, "name: %s, pa: 0x%x, va: 0x%x, size: 0x%x, flag: 0x%x \n",
			(char *)tmp->caller, (u32)tmp->phys_addr, (u32)tmp->addr, (u32)tmp->size, (u32)tmp->flags);
	}

    return len;
}

/* Read memory allocation distribution for DDR-2
 */
static int proc_read_mmapkernel(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
	struct vm_struct **p, *tmp;

	len += sprintf(page + len,"\nMemory allocated list from kernel\n");
	len += sprintf(page + len,"--------------------------------------------------------------------------\n");
	for (p = &lowmem_desc.vmlist ; (tmp = *p) != NULL ;p = &tmp->next) {
		len += sprintf(page + len, "name: %s, pa: 0x%x, va: 0x%x, size: 0x%x, flag: 0x%x \n",
			(char *)tmp->caller, (u32)tmp->phys_addr, (u32)tmp->addr, (u32)tmp->size, (u32)tmp->flags);
	}

    return len;
}

/* Read module locates in which DDR
 */
static int proc_read_config_file(char *page, char **start, off_t off, int count, int *eof,
                                 void *data)
{
    int len = 0, i;
    char *name[DDR_ID_MAX + 1] = { "SYSTEM", "DDR-1", "DDR-2", "" };

    len += sprintf(page + len, "frmidx    Name                            DDR Location\n");
    len += sprintf(page + len, "------------------------------------------------------\n");
    for (i = 1; i < ARRAY_SIZE(id_string_arry); i++) {
#if 1                           /* for special case: H264E */
        if (id_string_arry[i].rmmp_index == FRMIDX_H264E_DMAMASK0)
            len +=
                sprintf(page + len, "%-8d  %-30s  %s\n", id_string_arry[i].rmmp_index,
                        id_string_arry[i].string, h264e_dmamask0);
        else if (id_string_arry[i].rmmp_index == FRMIDX_H264E_DMAMASK1)
            len +=
                sprintf(page + len, "%-8d  %-30s  %s\n", id_string_arry[i].rmmp_index,
                        id_string_arry[i].string, h264e_dmamask1);
        else
            len +=
                sprintf(page + len, "%-8d  %-30s  %s\n", id_string_arry[i].rmmp_index,
                        id_string_arry[i].string, name[id_string_arry[i].ddr_id]);

#else
        len +=
            sprintf(page + len, "%-8d  %-30s  %s\n", id_string_arry[i].rmmp_index,
                    id_string_arry[i].string, name[id_string_arry[i].ddr_id]);
#endif
    }

    return len;
}

/* frammap internal debug info.
 */
static int proc_read_debuginfo(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int node_id;
    page_node_t *page_node;
    struct list_head *node;

    for (node_id = 0; node_id < p_page_info->nr_node; node_id++) {
        list_for_each(node, &p_page_info->list[node_id]) {
            page_node = list_entry(node, page_node_t, list);
            printk("ddr = %d, page = 0x%x, addr = 0x%x, size = %d \n", node_id,
                   (u32) page_node->page, (u32) page_to_phys(page_node->page),
                   (u32) page_node->size);
        }
        printk(" \n");
    }

    return 0;
}

/* frammap free available and unused pages
 */
static int proc_write_freepages(struct file *file, const char *buffer, unsigned long count,
                                void *data)
{
    int len = count;
    unsigned char value[20];
    uint tmp;

    if (copy_from_user(value, buffer, len))
        return 0;
    value[len] = '\0';

    sscanf(value, "%u\n", &tmp);
    if (tmp >= 0 && tmp < MAX_DDR_NR) {
        if (ddr_desc[tmp].valid) {
            printk("\n");
            frmmap_give_freepgs(tmp, 0);
        } else
            printk("Frammap: DDR %d doesn't exist! \n", tmp);
    }

    return count;
}

/* frammap free available and unused pages
 */
static int proc_write_force_freepages(struct file *file, const char *buffer, unsigned long count,
                                void *data)
{
    ddr_desc_t  *p_ddr_desc;
    struct vm_struct **p, *tmp, *cand = NULL;
    int len = count;
    unsigned char value[20];
    uint idx, free_size = 0, end_addr;

    if (copy_from_user(value, buffer, len))
        return 0;
    value[len] = '\0';

    sscanf(value, "%u\n", &idx);
    if (idx >= 0 && idx < MAX_DDR_NR) {
        p_ddr_desc = &ddr_desc[idx];

        if (ddr_desc[idx].valid) {
            printk("\n");

            down(&p_ddr_desc->sema);
            /* move to last one */
            for (p = &p_ddr_desc->vmlist; (tmp = *p) != NULL ;p = &tmp->next)
                cand = tmp;

            if (cand != NULL) {
                end_addr = cand->phys_addr + cand->size;
                free_size = ddr_desc[idx].mem_end - end_addr;
            } else {
                free_size = ddr_desc[idx].mem_end - ddr_desc[idx].mem_base;
            }

            /* free memory */
            frmmap_give_freepgs(idx, free_size);
            up(&p_ddr_desc->sema);
        } else {
            printk("Frammap: DDR %d doesn't exist! \n", idx);
        }
    }

    return count;
}

/* proc init.
 */
static int frm_proc_init(void)
{
    int ddr_idx, ret = 0;
    struct proc_dir_entry *p;

    p = create_proc_entry("frammap", S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (p == NULL) {
        ret = -ENOMEM;
        goto end;
    }

    frm_proc_root = p;

    /*
     * ddr_info
     */
    ddr_info = create_proc_entry("ddr_info", S_IRUGO, frm_proc_root);
    if (ddr_info == NULL)
        panic("Fail to create proc ddr_info!\n");

    ddr_info->read_proc = (read_proc_t *) proc_read_ddr_info;
    ddr_info->write_proc = NULL;

    /*
     * mmap
     */

    /* system mmap */
    ddr_idx = DDR_ID_SYSTEM;
    if ((ddr_idx < MAX_DDR_NR) && ddr_desc[ddr_idx].valid) {
        mmap0 = create_proc_entry("mmap0", S_IRUGO, frm_proc_root);
        if (mmap0 == NULL)
            panic("Fail to create proc mmap0!\n");

        mmap0->read_proc = (read_proc_t *)proc_read_mmap0;
        mmap0->write_proc = NULL;
    }

    /* ddr-1 mmap */
    ddr_idx = DDR_ID1;
    if ((ddr_idx < MAX_DDR_NR) && ddr_desc[ddr_idx].valid) {
        mmap1 = create_proc_entry("mmap1", S_IRUGO, frm_proc_root);
        if (mmap1 == NULL)
            panic("Fail to create proc mmap1!\n");

        mmap1->read_proc = (read_proc_t *) proc_read_mmap1;
        mmap1->write_proc = NULL;
    }

    /* ddr-2 mmap */
    ddr_idx = DDR_ID2;
    if ((ddr_idx < MAX_DDR_NR) && ddr_desc[ddr_idx].valid) {
        mmap2 = create_proc_entry("mmap2", S_IRUGO, frm_proc_root);
        if (mmap2 == NULL)
            panic("Fail to create proc mmap2!\n");

        mmap2->read_proc = (read_proc_t *) proc_read_mmap2;
        mmap2->write_proc = NULL;
    }

    /* kernel mmap */
    mmap_kernel = create_proc_entry("mmap_kernel", S_IRUGO, frm_proc_root);
    if (mmap_kernel == NULL)
        panic("Fail to create proc mmap_kernel!\n");

    mmap_kernel->read_proc = (read_proc_t *) proc_read_mmapkernel;
    mmap_kernel->write_proc = NULL;


    /*
     * name table
     */
    config_file = create_proc_entry("config_file", S_IRUGO, frm_proc_root);
    if (config_file == NULL)
        panic("Fail to create proc config_file!\n");

    config_file->read_proc = (read_proc_t *) proc_read_config_file;
    config_file->write_proc = NULL;

    /*
     * debug frammap internal
     */
    frammap_debug = create_proc_entry("debug", S_IRUGO, frm_proc_root);
    if (frammap_debug == NULL)
        panic("Fail to create proc frammap_debug!\n");

    frammap_debug->read_proc = (read_proc_t *) proc_read_debuginfo;
    frammap_debug->write_proc = NULL;

    /*
     * Free unuse(clean) and available pages
     */
    frammap_free_pages = create_proc_entry("free_pages", S_IRUGO, frm_proc_root);
    if (frammap_free_pages == NULL)
        panic("Fail to create proc free_pages!\n");

    frammap_free_pages->read_proc = NULL;
    frammap_free_pages->write_proc = proc_write_freepages;

    /*
     * Force free pages including dirty pages
     */
    frammap_force_free_pages = create_proc_entry("force_free_pages", S_IRUGO, frm_proc_root);
    if (frammap_force_free_pages == NULL)
        panic("Fail to create proc force_free_pages!\n");

    frammap_force_free_pages->read_proc = NULL;
    frammap_force_free_pages->write_proc = proc_write_force_freepages;

end:
    return ret;
}

static void frm_proc_release(void)
{
    if (ddr_info != NULL)
        remove_proc_entry(ddr_info->name, frm_proc_root);

    if (mmap0 != NULL)
        remove_proc_entry(mmap0->name, frm_proc_root);

    if (mmap1 != NULL)
        remove_proc_entry(mmap1->name, frm_proc_root);

    if (mmap2 != NULL)
        remove_proc_entry(mmap2->name, frm_proc_root);

    if (mmap_kernel != NULL)
        remove_proc_entry(mmap_kernel->name, frm_proc_root);

    if (config_file != NULL)
        remove_proc_entry(config_file->name, frm_proc_root);

    if (frammap_debug != NULL)
        remove_proc_entry(frammap_debug->name, frm_proc_root);

    if (frammap_free_pages != NULL)
        remove_proc_entry(frammap_free_pages->name, frm_proc_root);

    if (frammap_force_free_pages != NULL)
        remove_proc_entry(frammap_force_free_pages->name, frm_proc_root);

    /* last one */
    if (frm_proc_root != NULL)
        remove_proc_entry(frm_proc_root->name, NULL);
}

/*
 * Following defines the interface to have the AP use the memory from the DDRs
 */

/* ioctl
 */
static long frmmap_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    unsigned int dev;
    ap_private_data_t *cookie;
    ap_open_info_t *openinfo;
    struct inode *inode = filp->f_mapping->host;

    dev = iminor(inode) - FRAMMAP_MINOR;

    cookie = &ap_private_data[dev];
    if (unlikely(!cookie->node_cache)) {
        printk("This dev%d is not opened! \n", dev);
        return -EFAULT;
    }

    openinfo = get_open_info(cookie, filp);
    if (openinfo == NULL) {
        printk("%s gets unknown filp:0x%x! \n", __func__, (u32)filp);
        return -EFAULT;
    }

    switch (cmd) {
    case FRM_IOGBUFINFO:
        {
            frmmap_meminfo_t mem;

            /* get how many bytes the DDR pool has */
            mem.aval_bytes = get_max_slice(dev);
            if (copy_to_user((void *)arg, (void *)&mem, sizeof(frmmap_meminfo_t)))
                ret = -EFAULT;

            if (FRM_DEBUG)
                printk("%s, get available memory %d for filp: 0x%x \n", __func__,
                                mem.aval_bytes, (u32)openinfo->filep);

            break;
        }
    case FRM_ALLOC_TYPE:
        {
            alloc_type_t   alloc_type;
            unsigned long  retVal;

            retVal = copy_from_user((void *)&alloc_type, (void *)arg, sizeof(int));
            if (retVal) {
	            ret = -EFAULT;
	            break;
	        }

            if (alloc_type < ALLOC_CACHEABLE || alloc_type > ALLOC_NONCACHABLE)
                return -EFAULT;

	        openinfo->alloc_type = alloc_type;
	        if (FRM_DEBUG) {
                printk("%s, set memory allocate type:0x%x for filp: 0x%x \n", __func__,
                                            openinfo->alloc_type, (u32)openinfo->filep);
            }

            break;
        }
    case FRM_GIVE_FREEPAGES:
        if (frmmap_give_freepgs(dev, 0) < 0)
            ret = -EFAULT;
        break;
    case FRM_ADDR_V2P:
        {
            unsigned long retVal;
            unsigned int  vaddr, paddr;

            ret = -EFAULT;
            retVal = copy_from_user((void *)&vaddr, (void *)arg, sizeof(unsigned int));
            if (retVal)
                break;
            paddr = (unsigned int)frm_va2pa(vaddr);
            if (paddr != 0xFFFFFFFF) {
    	        if (copy_to_user((void *)arg, (void *)&paddr, sizeof(unsigned int)))
    	            break;

                ret = 0;
            }
            break;
        }
    default:
        printk("%s, unknown cmd: 0x%x \n", __func__, cmd);
        ret = -1;
        break;
    }

    return (long)ret;
}

static int frmmap_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long size = vma->vm_end - vma->vm_start;
    ap_private_data_t *cookie;
    ap_open_info_t    *openinfo;
    unsigned long off = vma->vm_pgoff << PAGE_SHIFT;    /* offset from the buffer start point */
    ap_node_t *node;
    unsigned long pfn;          //which page number is the start page
    struct inode *inode = filp->f_mapping->host;
    int ret, dev, aval_bytes;
    char  *name, *str_array[] = {"cachable", "bufferable", "noncachable"};

    dev = iminor(inode) - FRAMMAP_MINOR;
    cookie = &ap_private_data[dev];

    openinfo = get_open_info(cookie, filp);
    if (unlikely(!openinfo)) {
        printk("%s, gets unknown filp: 0x%x \n", __func__, (u32)filp);
        return -EFAULT;
    }

    if (FRM_DEBUG)
        printk("%s, alloc memory for filp: 0x%x \n", __func__,(u32)openinfo->filep);

    switch (cookie->devidx) {
    case 0:
        cookie->frmidx = FRMIDX_AP0;
        name = "user_ap0";
        break;
    case 1:
        cookie->frmidx = FRMIDX_AP1;
        name = "user_ap1";
        break;
    case 2:
        cookie->frmidx = FRMIDX_AP2;
        name = "user_ap2";
        break;
    case 3:
        cookie->frmidx = FRMIDX_AP3;
        name = "user_ap3";
        break;
    default:
        return -EINVAL;
    }

    /* check if the ddr is active */
    if (unlikely(!ddr_desc[dev].valid)) {
        printk("%s, DDR%d is not active! \n", __func__, dev);
        return -EINVAL;
    }
    else {
        aval_bytes = get_max_slice(dev);
        /* if DDR0, it can get from lowmem */
        if ((dev != 0) && aval_bytes < size) {
            printk("%s, not enough memory for DDR%d! \n", __func__, dev);
            return -ENOMEM;
        }
    }

    node = kmem_cache_alloc(cookie->node_cache, GFP_KERNEL);
    if (node == NULL) {
        printk("frmmap_mmap: kmem_cache_alloc fail! \n");
        return -EINVAL;
    }
    memset(node, 0x0, sizeof(ap_node_t));

    /* allocate memory from frammap orginal mechanism
     */
    node->buf_info.size = size;
    node->buf_info.align = 4;
    node->buf_info.name = name;
    node->buf_info.alloc_type = openinfo->alloc_type;

    if (FRM_DEBUG)
        printk("%s, the alloc memory type for filp: 0x%x is 0x%x\n", __func__,(u32)openinfo->filep,
                        openinfo->alloc_type);

    node->private = openinfo;
    if (frm_get_buf_info(cookie->frmidx, &node->buf_info) < 0) {
        kmem_cache_free(cookie->node_cache, node);
        return -EINVAL;
    }

    INIT_LIST_HEAD(&node->list);
    list_add_tail(&node->list, &cookie->alloc_list);

    /* Get the memory success, start to do mmap
     */
    if (openinfo->alloc_type == ALLOC_CACHEABLE)
        vma->vm_page_prot = PAGE_SHARED;
    else if (openinfo->alloc_type == ALLOC_BUFFERABLE)
        vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
    else
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    vma->vm_flags |= VM_RESERVED;

    off += node->buf_info.phy_addr;     //points to the offset from node->buf_info.phy_addr
    pfn = off >> PAGE_SHIFT;    //which page number is the start page

    ret = remap_pfn_range(vma, vma->vm_start, pfn, node->buf_info.size, vma->vm_page_prot);

    printk("Frammap: AP%d, off:%x, p:%#x, v:%#x, size:%#x, pfn:%d, alloc_type:%s \n", dev,
            (u32)off, (u32)node->buf_info.phy_addr, (u32)vma->vm_start, (u32)size, (u32)pfn,
            str_array[openinfo->alloc_type - ALLOC_CACHEABLE]);

    cookie->alloc_cnt ++;

    return ret;
}

static int frmmap_open(struct inode *inode, struct file *filp)
{
    int dev, ret = 0;
    ap_open_info_t  *openinfo;
    ap_private_data_t *cookie;
    char *name;

    dev = iminor(inode) - FRAMMAP_MINOR;

    if ((dev < 0) || (dev >= MAX_DDR_NR))
        return -1;

    cookie = &ap_private_data[dev];

    down(&ap_sema);

    /* The following code processes the multiple open in AP.
     * Note: when multiple open, the filp pointer is also different even the same device node.
     */
    if (likely(cookie->node_cache == NULL)) {
        name = ap_devname[dev];
        memset(name, 0, 20);
        sprintf(name, "frammap_ap%d", dev);
        cookie->node_cache = kmem_cache_create(name, sizeof(ap_node_t), 0, 0, NULL);
        if (!cookie->node_cache) {
            printk("Frammap: AP creates kmem_cache_create fail! \n");
            ret = -1;
            goto exit;
        }

        INIT_LIST_HEAD(&cookie->alloc_list);
        INIT_LIST_HEAD(&cookie->open_list);

        /* When AP allocates the memory, the information will be kept here
         */
        cookie->devidx = dev;
        if (FRM_DEBUG)
            printk("%s, %s create cache \n", __func__, name);
    }

    printk("%s: AP%d open file: 0x%x \n", __func__, dev, (u32)filp);

    /* create one node */
    openinfo = kzalloc(sizeof(ap_open_info_t), GFP_KERNEL);
    if (unlikely(!openinfo)) {
        ret = -1;
        goto exit;
    }
    INIT_LIST_HEAD(&openinfo->list);
    openinfo->filep = filp;
    openinfo->alloc_type = ALLOC_NONCACHABLE;  //default type
    list_add_tail(&openinfo->list, &cookie->open_list);
    cookie->open_cnt ++;

    /* inc the module reference counter */
    try_module_get(THIS_MODULE);

  exit:
    up(&ap_sema);
    return ret;
}

/*
 * Close function
 */
static int frmmap_release(struct inode *inode, struct file *filp)
{
    ap_private_data_t *cookie;
    ap_node_t *node, *ne;
    ap_open_info_t *openinfo;
    int ret = -1, dev = iminor(inode) - FRAMMAP_MINOR;

    printk("%s is called for AP%d \n", __func__, dev);

    down(&ap_sema);

    cookie = &ap_private_data[dev];
    openinfo = get_open_info(cookie, filp);
    if (unlikely(!openinfo))
        goto exit;

    list_del(&openinfo->list);
    kfree(openinfo);
    cookie->open_cnt --;

    list_for_each_entry_safe(node, ne, &cookie->alloc_list, list) {
        if (node->private != openinfo)
            continue;
        list_del_init(&node->list);
        up(&ap_sema);

        if (FRM_DEBUG)
            printk("%s, release memory for filp: 0x%x \n", __func__, (u32)node->private->filep);

        /* release memory */
        frm_release_buf_info(node->buf_info.va_addr);
        kmem_cache_free(cookie->node_cache, node);
        cookie->alloc_cnt --;
        down(&ap_sema);
    }

    /* debug only */
    if (list_empty(&cookie->alloc_list)) {
        if (cookie->alloc_cnt != 0)
            panic("%s, bug, alloc_cnt = %d \n", __func__, cookie->alloc_cnt);
    } else if (!cookie->alloc_cnt) {
        if (!list_empty(&cookie->alloc_list))
            panic("%s, bug, alloc_cnt = %d \n", __func__, cookie->alloc_cnt);
    }
    if (list_empty(&cookie->open_list)) {
        if (cookie->open_cnt != 0)
            panic("%s, bug, open_cnt = %d \n", __func__, cookie->open_cnt);
    } else if (!cookie->open_cnt) {
        if (!list_empty(&cookie->open_list))
            panic("%s, bug, open_cnt = %d \n", __func__, cookie->open_cnt);
    }

    if (!cookie->open_cnt && !cookie->alloc_cnt) {
        /* destroy the ap node cache */
        kmem_cache_destroy(ap_private_data[dev].node_cache);
        ap_private_data[dev].node_cache = NULL;
        printk("%s: cache is released for AP%d \n", __func__, dev);
    }

    ret = 0;
    /* need to return the memory */
    module_put(THIS_MODULE);

exit:
    up(&ap_sema);
    return ret;
}

struct file_operations frmmap_fops = {
  owner:THIS_MODULE,
  unlocked_ioctl:frmmap_ioctl,
  mmap:frmmap_mmap,
  open:frmmap_open,
  release:frmmap_release
};

struct miscdevice misdev[MAX_DDR_NR];

module_init(frammap_init);
module_exit(frammap_exit);

EXPORT_SYMBOL(show_ddr_info);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("GM RAM MMAP Library");
MODULE_LICENSE("GPL");

