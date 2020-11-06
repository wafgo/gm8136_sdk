/**
 * @file security.c
 *  AES/DES/Triple_DES Security Driver
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision$
 * $Date$
 *
 * ChangeLog:
 *  $Log$
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/bitops.h>
#include <linux/random.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/synclink.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#include "security.h"
#include "platform.h"
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28))
#include <mach/fmem.h>
#include "frammap_if.h"
#endif
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,14))
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <mach/platform/platform_io.h>
#include <mach/ftpmu010.h>
#else
#include <asm/semaphore.h>
#include <linux/device.h>
#endif

//#define INTERNAL_DEBUG
#define SUPPORT_HOST_CANCEL

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28))
#define ALLOC_DMA_BUF_CACHED
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
#define init_MUTEX(sema) sema_init(sema, 1)
#endif

#define DMA_INFO_VA(x) (x)->dma_info_va
#define DMA_INFO_PA(x) (x)->dma_info_pa
#define DMA_INFO_SZ(x) (x)->dma_info_size

struct sec_file_data {
    void        *dma_info_va;   /* virtual address */
    dma_addr_t  dma_info_pa;    /* physical address */
    int         dma_info_size;
};

static struct resource *irq;
static volatile int dma_int_ok;
static void __iomem *es_io_base;
static wait_queue_head_t es_wait_queue;
static struct semaphore  sema;
static unsigned int exit_for_cancel = 0;

static int es_open(struct inode *inode, struct file *filp);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
static int es_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#else
static long es_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#endif

static int getkey(int algorithm, u32 key_addr, u32 IV_addr, u32 data);
static int calc_key_length(esreq * srq, unsigned int* p_key_info);
static int es_do_dma(int stage, esreq * srq, struct sec_file_data* p_data );

static void IV_Output(int addr);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19))
irqreturn_t es_int_isr(int irq, void *dev);
#else
irqreturn_t es_int_isr(int irq, void *dev, struct pt_regs *dummy);
#endif

irqreturn_t es_int_handler0(int irq, void *dev_id);
int es_mmap(struct file *filp, struct vm_area_struct *vma);
static int es_release(struct inode *inode, struct file *filp);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
int es_probe(struct device *dev);
static int es_remove(struct device * dev);
#else
int es_probe(struct platform_device *pdev);
static int __devexit es_remove(struct platform_device *pdev);
#endif

struct device *p_es_device = NULL;

struct file_operations es_fops = {
  owner:THIS_MODULE,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
  ioctl:es_ioctl,
#else
  unlocked_ioctl:es_ioctl,
#endif
  mmap:es_mmap,
  open:es_open,
  release:es_release,
};

struct miscdevice es_dev = {
  minor:MISC_DYNAMIC_MINOR,
  name:MODULE_NAME,
  fops:&es_fops,
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
static struct platform_driver es_driver = {
    .driver = {
               .owner = THIS_MODULE,
               .name = MODULE_NAME,

               },
    .probe  = es_probe,
    .remove = __devexit_p(es_remove),
};
#else
static struct device_driver es_driver = {
    .owner  = THIS_MODULE,
    .name   = MODULE_NAME,
    .bus    = &platform_bus_type,
    .probe  = es_probe,
    .remove = es_remove,
};
#endif

static struct resource es_resource[] = {
    [0] = {
           .start = AES_FTAES020_PA_BASE,
           .end = AES_FTAES020_PA_LIMIT,
           .flags = IORESOURCE_MEM,
          },

    [1] = {
           .start = AES_FTAES020_0_IRQ,
           .end = AES_FTAES020_0_IRQ,
           .flags = IORESOURCE_IRQ,
          }
};

void inline es_write_reg(int offset, int value)
{
    writel(value, es_io_base + offset);
}

u32 inline es_read_reg(int offset)
{
    return readl(es_io_base + offset);
}

int es_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int ret = 0;
    struct sec_file_data *p_data = filp->private_data;

    DMA_INFO_SZ(p_data) = vma->vm_end - vma->vm_start;

    if (!(vma->vm_flags & VM_WRITE)) {
        printk("app bug: PROT_WRITE please\n");
        ret = -EINVAL;
        goto out;
    }

    if (!(vma->vm_flags & VM_SHARED)) {
        printk("app bug: MAP_SHARED please\n");
        ret = -EINVAL;
        goto out;
    }

    if (!DMA_INFO_VA(p_data)) {
#ifdef ALLOC_DMA_BUF_CACHED
        struct page *p_page = NULL;
        p_page = alloc_pages(GFP_KERNEL, get_order(DMA_INFO_SZ(p_data)));   ///< get free page, max is 4MB
        if(!p_page) {
            DMA_INFO_VA(p_data) = NULL;
        }
        else {
            DMA_INFO_VA(p_data) = page_address(p_page); ///< to page start virtual  address
            DMA_INFO_PA(p_data) = page_to_phys(p_page); ///< to page start physical address
        }
#else
        DMA_INFO_VA(p_data) = dma_alloc_coherent(NULL, DMA_INFO_SZ(p_data), &DMA_INFO_PA(p_data), GFP_DMA | GFP_KERNEL);
#endif
        if (!DMA_INFO_VA(p_data)) {
            printk("could not alloc data input dma memory\n");
            return -ENOMEM;
        }
    }
    else {
        printk("%s is called before\n", __func__);
        return -ENOMEM;
    }

    /* indicates the offset from the start hardware address */
    vma->vm_pgoff = DMA_INFO_PA(p_data) >> PAGE_SHIFT;

#ifndef ALLOC_DMA_BUF_CACHED
    /* select uncached access */
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
#endif

    /* avoid to swap out this VMA */
    vma->vm_flags |= VM_RESERVED;

    /* remap page address to VMA */
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, DMA_INFO_SZ(p_data), vma->vm_page_prot)) {
        printk("Remap_pfn_range() failed\n");
        dma_free_coherent(NULL, DMA_INFO_SZ(p_data), DMA_INFO_VA(p_data), DMA_INFO_PA(p_data));
        DMA_INFO_VA(p_data) = NULL;
        return -EFAULT;
    }

  //printk("mmap V 0x%08X P 0x%08X S 0x%08x\n", (unsigned int)DMA_INFO_VA(p_data), (unsigned int)DMA_INFO_PA(p_data), DMA_INFO_SZ(p_data));

out:
    return ret;
}

int es_wait_event_on(void)
{
    int ret = -1;

#ifdef SUPPORT_HOST_CANCEL
    ret = wait_event_interruptible_timeout(
        es_wait_queue,
        dma_int_ok != 0,
        msecs_to_jiffies(5000) /* 5 seconds */
    );
#else
    ret = wait_event_timeout(
        es_wait_queue,
        dma_int_ok != 0,
        msecs_to_jiffies(5000) /* 5 seconds */
    );
#endif

    /* normal result */
    if ( ret > 0) {
        ret = 0;
        if (!dma_int_ok) {
            panic("%s dma_int_ok not ok%d", __func__, dma_int_ok);
        }
    }
    else if (ret == -ERESTARTSYS) {
        exit_for_cancel = 1;
    }
    else if (ret == 0) {
        /* timeout */
        printk("%s, wait event timeout %d!!!! \n", __func__, ret);
        ret = -ETIMEDOUT;
    }
    else {
        /* unexpected situation */
        panic("* %s unexpected situation %d %d", __func__, dma_int_ok, ret);
    }

    dma_int_ok = 0;
    return ret;
}

static int es_open(struct inode *inode, struct file *filp)
{
    struct sec_file_data *p_data = kzalloc(sizeof(struct sec_file_data), GFP_KERNEL);

    if(!p_data)
        return -EINVAL;

    filp->private_data = p_data;

    return 0;
}

static int es_release(struct inode *inode, struct file *filp)
{
    struct sec_file_data *p_data = filp->private_data;

    if (DMA_INFO_VA(p_data)) {
#ifdef ALLOC_DMA_BUF_CACHED
        free_pages((unsigned long)DMA_INFO_VA(p_data), get_order(DMA_INFO_SZ(p_data)));
        DMA_INFO_VA(p_data) = NULL;
#else
        dma_free_coherent(NULL, DMA_INFO_SZ(p_data), DMA_INFO_VA(p_data), DMA_INFO_PA(p_data));
        DMA_INFO_VA(p_data) = NULL;
#endif
    }
    kfree(p_data);
    filp->private_data = NULL;
    return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
static int __devexit es_remove(struct platform_device *pdev)
#else
int es_remove(struct device * dev)
#endif
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
    struct platform_device* pdev = to_platform_device(dev);
#endif

    iounmap((void __iomem *)es_io_base);
    free_irq(irq->start, (void *)pdev);
    return 0;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
int es_probe(struct device *dev)
#else
int es_probe(struct platform_device *pdev)
#endif
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
    struct platform_device* pdev = to_platform_device(dev);
#endif
    struct resource *mem;
    int ret = 0;

    mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!mem) {
        printk("no mem resource?\n");
        ret = -ENODEV;
        goto err_final;
    }

    irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (!irq) {
        printk("no irq resource?\n");
        ret = -ENODEV;
        goto err_final;
    }

    ret = request_irq(
            irq->start,
            es_int_isr,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
            SA_INTERRUPT,
#else
            IRQF_DISABLED,
#endif
            "ES IRQ",
            pdev);

    if (unlikely(ret != 0)) {
        printk("Error to allocate security irq handler!!\n");
        goto fail_irq;
    }

    //map pyhsical address to virtual address for program use easier
    es_io_base = (void __iomem *)ioremap_nocache(mem->start, (mem->end - mem->start + 1));
    if (unlikely(es_io_base == NULL)) {
        printk("%s fail: counld not do ioremap\n", __func__);
        ret = -ENOMEM;
        goto err_ioremap;
    }

    p_es_device = &pdev->dev;

    init_waitqueue_head(&es_wait_queue);

    return 0;

err_ioremap:
    release_mem_region(mem->start, mem->end - mem->start + 1);

fail_irq:
    free_irq(irq->start, pdev);

err_final:
    return ret;
}

static void es_device_release(struct device *dev)
{
    return;
}

static u64 es_do_dmamask = 0xFFFFFFUL;

static struct platform_device es_device = {
    .name = MODULE_NAME,
    .id = -1,
    .num_resources = ARRAY_SIZE(es_resource),
    .resource = es_resource,
    .dev = {
            .dma_mask = &es_do_dmamask,
            .coherent_dma_mask = 0xFFFFFFFF,
            .release = es_device_release,
           },
};

/*
 * This is the interrupt routine
 */
irqreturn_t es_int_handler0(int irq, void *dev_id)
{
    int status;

    status = es_read_reg(SEC_MaskedIntrStatus);

    if ((status & Data_done) != 0) {
        dma_int_ok = 1;
    }
    es_write_reg(SEC_ClearIntrStatus, Data_done);

    if (exit_for_cancel == 1) {
        exit_for_cancel = 0;
        up(&sema);
    }
    else {
#ifdef SUPPORT_HOST_CANCEL
        wake_up_interruptible(&es_wait_queue);
#else
        wake_up(&es_wait_queue);
#endif
    }
    return IRQ_HANDLED;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19))
irqreturn_t es_int_isr(int irq, void *dev)
#else
irqreturn_t es_int_isr(int irq, void *dev, struct pt_regs *dummy)
#endif
{
    return es_int_handler0(irq, (unsigned int *)dev);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
static int es_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#else
static long es_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
    esreq srq;
    int status = -1;
    int key_info = 0;
    struct sec_file_data* p_data = filp->private_data;
    int es_func = 0;

    status = copy_from_user(&srq, (void __user *)arg, sizeof(esreq));
    if (status != 0) {
        printk("copy_from_user failure\n");
        goto exit_err;
    }


    switch (cmd) {
        case ES_GETKEY:
            status = calc_key_length(&srq, &key_info);
            if (status != 0) {
                goto exit_err;
            }

            status = getkey(srq.algorithm, (u32) srq.key_addr, (u32) srq.IV_addr, key_info);
            if (status != 0) {
                goto exit_err;
            }

            status = copy_to_user((void __user *)arg, &srq, sizeof(esreq));
            if (status != 0)
                goto exit_err;

            break;

        case ES_ENCRYPT:
        case ES_AUTO_ENCRYPT:
        case ES_DECRYPT:
        case ES_AUTO_DECRYPT:
            status = calc_key_length(&srq, &key_info);
            if (status != 0) {
                goto exit_err;
            }

            if ((cmd == ES_AUTO_ENCRYPT)) {
                status = getkey(srq.algorithm, (u32) srq.key_addr, (u32) srq.IV_addr, key_info);
                if (status != 0) {
                    goto exit_err;
                }
            }


            if ((cmd == ES_AUTO_DECRYPT) || cmd == (ES_DECRYPT)) {
                es_func = Decrypt_Stage;
            }
            else {
                es_func = Encrypt_Stage;
            }

            status = es_do_dma(es_func, &srq, p_data);
            if (status)
                goto exit_err;

            IV_Output((int)DMA_INFO_VA(p_data) + srq.data_length - 16);
            break;

        default:
            printk("* ioctl wrong cmd 0x%08X \n", cmd);
            status = -ENOIOCTLCMD;
            goto exit_err;
    }
exit_err:

    /* cancel action due to signal, we cannot release semaphore due to hardware still running.
     * if the interrupt rises, it will release the semaphore to service another users
     */
    if (status) {
        printk("%s, failure! \n", __func__);
    }
    return status;
}

/*
 * The driver boot-time initialization code!
 */
static int __init es_init(void)
{
    int ret = 0;

    platform_pmu_init();

    /* Register the IP resource */
    ret = platform_device_register(&es_device);
    if (ret < 0) {
        printk(KERN_ERR "register platform device failed (%d)\n", ret);
        return -ENODEV;
    }

    /* Register the driver */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
    if ((ret = driver_register(&es_driver)) < 0) {
#else
    if ((ret = platform_driver_register(&es_driver)) < 0) {
#endif
        printk("Failed to register Security driver\n");
        goto out3;
    }

    /* register misc device */
    if ((ret = misc_register(&es_dev)) < 0) {
        printk("can't get major number");
        goto out3;
    }
    printk("Register AES/DES...OK\n");

    init_MUTEX(&sema);
    dma_int_ok = 0;

    return ret;

out3:
    platform_device_unregister(&es_device);

    return ret;
}

static u32 random(void)
{
    static int first_geg = 1;
    static u32 rand = 0;
    if (first_geg == 1) {
        rand = jiffies;
        first_geg = 0;
    }
    rand = (rand*0x1664525) + 0x13904223;

    return rand;
}

static int calc_key_length(esreq * srq, unsigned int* p_key_info)
{
    int ret = 0;
    int algorithm = srq->algorithm;

    switch (algorithm) {
        case Algorithm_DES:
            srq->key_length = 8;
            srq->IV_length  = 8;
            break;

        case Algorithm_Triple_DES:
            srq->key_length = 24;
            srq->IV_length  = 8;
            break;

        case Algorithm_AES_128:
            srq->key_length = 16;
            srq->IV_length  = 16;
            break;

        case Algorithm_AES_192:
            srq->key_length = 24;
            srq->IV_length  = 16;
            break;

        case Algorithm_AES_256:
            srq->key_length = 32;
            srq->IV_length  = 16;
            break;

        default:
            ret = -EINVAL;
            break;
    }

    *p_key_info = (srq->key_length << 16) | (srq->IV_length);

    return ret;
}

static int getkey(int algorithm, u32 key_addr, u32 IV_addr, u32 data)
{
    int i;
    int key_length;
    int IV_length;

    key_length = data >> 16;
    IV_length  = data & 0xFFFF;

#ifdef INTERNAL_DEBUG
    *(unsigned int *)(key_addr)      = 0x603deb10;
    *(unsigned int *)(key_addr + 4)  = 0x15ca71be;
    *(unsigned int *)(key_addr + 8)  = 0x2b73aef0;
    *(unsigned int *)(key_addr + 12) = 0x857d7781;
    *(unsigned int *)(key_addr + 16) = 0x1f352c07;
    *(unsigned int *)(key_addr + 20) = 0x3b6108d7;
    *(unsigned int *)(key_addr + 24) = 0x2d9810a3;
    *(unsigned int *)(key_addr + 28) = 0x0914dff4;

    *(unsigned int *)(IV_addr)       = 0x00010203;
    *(unsigned int *)(IV_addr + 4)   = 0x04050607;
    *(unsigned int *)(IV_addr + 8)   = 0x08090a0b;
    *(unsigned int *)(IV_addr + 12)  = 0x0c0d0e0f;
#else
    for (i = 0; i < key_length / 4; i++)
        *(unsigned int *)(key_addr + i * 4) = random();

    for (i = 0; i < IV_length / 4; i++)
        *(unsigned int *)(IV_addr + i * 4) = random();
#endif

    return 0;
}

static void IV_Output(int addr)
{
    int i;

    for (i = 0; i < 4; i++) {
        *(u32 *) (addr + 4 * i) = es_read_reg(SEC_LAST_IV0 + 4 * i);
      //printk("IV = 0x%08x\n",*(u32 *)(addr + 4 * i));
    }
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28))
unsigned int es_va2pa(u32 vaddr)
{
    return (unsigned int)frm_va2pa(vaddr);
}
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,14))
unsigned int es_va2pa(u32 vaddr)
{
    pgd_t *pgdp;
    pmd_t *pmdp;
    pud_t *pudp;
    pte_t *ptep;
    unsigned int paddr = 0xFFFFFFFF;

    /*
     * The pgd is never bad, and a pmd always exists(as it's folded into the pgd entry)
     */
    if (vaddr >= TASK_SIZE) /* use init_mm as mmu to look up in kernel mode */
        pgdp = pgd_offset_k(vaddr);
    else
        pgdp = pgd_offset(current->mm, vaddr);

    if (unlikely(pgd_none(*pgdp))) {
        printk("frammap: 0x%x not mapped in pgd table! \n", vaddr);
        goto err;
    }

    /* because we don't have 3-level MMU, so pud = pgd. Here we are in order to meet generic Linux
     * version, pud is listed here.
     */
    pudp = pud_offset(pgdp, vaddr);
    if (unlikely(pud_none(*pudp))) {
        printk(KERN_INFO"frammap: 0x%x not mapped in pud table! \n", vaddr);
        goto err;
    }

    pmdp = pmd_offset(pudp, vaddr);
    if (unlikely(pmd_none(*pmdp))) {
        printk(KERN_INFO"frammap: 0x%x not mapped in pmd 0x%x! \n", vaddr, (u32)pmdp);
        goto err;
    }

    if (!pmd_bad(*pmdp)) {

        /* page mapping */
        ptep = pte_offset_kernel(pmdp, vaddr);
        if (pte_none(*ptep)) {
            printk(KERN_ERR"frammap: 0x%x not mapped in pte! \n", vaddr);
            goto err;
        }

        vaddr = (unsigned int)page_address(pte_page(*ptep)) + (vaddr & (PAGE_SIZE - 1));
        paddr = __pa(vaddr);
    } else {
        /* section mapping. The cases are:
         * 1. DDR direct mapping
         * 2. ioremap's vaddr, size and paddr all are 2M alignment.
         */
        if (vaddr & SECTION_SIZE)
            pmdp ++; /* move to pmd[1] */
        paddr = (pmd_val(*pmdp) & SECTION_MASK) | (vaddr & ~SECTION_MASK);
    }
err:
    return paddr;
}
#else
unsigned int es_va2pa(u32 vaddr)
{
    pmd_t *pmd;
    pte_t *pte;
    pgd_t *pgd;
    pgd = pgd_offset(current->mm, vaddr);
    if(!pgd_none(*pgd))
    {
        pmd = pmd_offset(pgd, vaddr);
        if (!pmd_none(*pmd))
        {
            pte = pte_offset_map(pmd, vaddr);
            if(!pmd_none(*pmd))
            {
                pte = pte_offset_map(pmd, vaddr);
                if(pte_present(*pte))
                    return (unsigned int) page_address(pte_page(*pte)) + (vaddr & (PAGE_SIZE - 1)) - PAGE_OFFSET;
            }
        }
    }
    return 0;
}
#endif

static int es_do_dma(int stage, esreq * srq, struct sec_file_data* p_data )
{
    int i, ret = 0;
    unsigned int section;
    unsigned int block_size    = 1;    /* block size of CFB mode of both AES and DES is 1 block */
    unsigned int data_src_addr = es_va2pa(srq->DataIn_addr);
    unsigned int data_dst_addr = es_va2pa(srq->DataOut_addr);
    unsigned int data_length   = srq->data_length;
    int  algorithm             = srq->algorithm;
    int  mode                  = srq->mode;

    if(data_length <= 16) {
        printk("data length=0x%08x not valid!\n", data_length);
        return -EINVAL;
    }

#ifdef ALLOC_DMA_BUF_CACHED
    /* clean and invalidate data cache to memory */
    fmem_dcache_sync((void *)srq->DataIn_addr,  data_length, DMA_BIDIRECTIONAL);
    fmem_dcache_sync((void *)srq->DataOut_addr, data_length, DMA_BIDIRECTIONAL);
#endif

    data_length -= 16;      //sub IV return length

    if (mode != CFB_mode) {
        switch (algorithm) {
            case Algorithm_DES:
            case Algorithm_Triple_DES:
                block_size = 8; /* except for CFB mode, block size of DES is 8 bytes */
                break;

            case Algorithm_AES_128:
            case Algorithm_AES_192:
            case Algorithm_AES_256:
                block_size = 16; /* except for CFB mode, block size of AES is 16 bytes */
                break;

            default:
                printk("* wrong alg 0x%08X \n", algorithm);
                return -EINVAL;
        }
    }

    if ((data_length % block_size) != 0) {
        printk("data length not block_size(%d) aligned\n", block_size);
        return -EINVAL;
    }

    section = MAX_SEC_DMATrasSize/block_size;
    section = section*block_size;

    down(&sema);

    es_write_reg(SEC_FIFOThreshold, (1 << 8) + 1);
    es_write_reg(SEC_IntrEnable, Data_done);

    /* start to set registers */
    /* 1. Set EncryptControl register */
    if (mode == ECB_mode)
        es_write_reg(SEC_EncryptControl, algorithm | mode | stage);
    else
        es_write_reg(SEC_EncryptControl, First_block | algorithm | mode | stage);

    /* 2. Set Initial vector IV */
    es_write_reg(SEC_DESIVH, *(u32 *) srq->IV_addr);
    es_write_reg(SEC_DESIVL, *(u32 *) (srq->IV_addr + 1));
    es_write_reg(SEC_AESIV2, *(u32 *) (srq->IV_addr + 2));
    es_write_reg(SEC_AESIV3, *(u32 *) (srq->IV_addr + 3));

    /* 3. Set Key value */
    for (i = 0; i < 8; i++)
        es_write_reg(SEC_DESKey1H + 4 * i, *(u32 *) (srq->key_addr + i));

    /* 5. Set DMA related register
     *    hw will add section size to dma addr,
     *    so you don't have update it in loop
     *    Nish comment.
     */
    es_write_reg(SEC_DMASrc, data_src_addr);
    es_write_reg(SEC_DMADes, data_dst_addr);

    while (data_length) {
        if (data_length >= section) {
            es_write_reg(SEC_DMATrasSize, section);
            data_length -= section;
        }
        else {
            es_write_reg(SEC_DMATrasSize, data_length);
            data_length = 0;
        }

        //6. Set DmaEn bit of DMAStatus to 1 to active DMA engine
        dma_int_ok = 0;
        es_write_reg(SEC_DMACtrl, DMA_Enable);

        //7. Wait transfer size is complete
        ret = es_wait_event_on();
        if (ret == -ERESTARTSYS) {
            printk("* es_wait_event get cancel singal! \n");
            break;
        } else if (ret != 0) {
            printk("* es_wait_event wrong %d \n", ret);
            break;
        }
#if 0
        // Update IV value
        es_write_reg(SEC_DESIVH, es_read_reg(SEC_LAST_IV0));
        es_write_reg(SEC_DESIVL, es_read_reg(SEC_LAST_IV1));
        es_write_reg(SEC_AESIV2, es_read_reg(SEC_LAST_IV2));
        es_write_reg(SEC_AESIV3, es_read_reg(SEC_LAST_IV3));
#endif
    }

    up(&sema);

    return ret;
}

static void __exit es_exit(void)
{
    misc_deregister(&es_dev);

    platform_pmu_exit();

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
    driver_unregister(&es_driver);
#else
    platform_driver_unregister(&es_driver);
#endif

    platform_device_unregister(&es_device);
}

module_init(es_init);
module_exit(es_exit);

MODULE_DESCRIPTION("Grain Media Security Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
