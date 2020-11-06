#include <linux/version.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/bitops.h>
#include <linux/miscdevice.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <mach/fmem.h>
#include <mach/dma_gm.h>

#include <dma_util/dma_util.h>
#include <frammap/frammap_if.h>

#define DMA_UTIL_MINOR   MISC_DYNAMIC_MINOR //dynamic

#define DMA_CONCURRENT_NUM  1   //how many DMAs can work simultaneously

static struct g_dma_info_s {
    struct device       dev;
    struct semaphore    sema;
    enum dma_kind       dma_type;
} g_dma_info;

static long dma_util_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    frammap_buf_t   frammap_src, frammap_dst;
    dma_util_t      dma_util;
    long ret = 0;

    if (down_trylock(&g_dma_info.sema)) {
        printk("%s, dma channel is full! \n", __func__);
        return -1;
    }

    switch (cmd) {
      case DMA_UTIL_DO_DMA:
        break;
      default:
        printk("%s, unknown command: 0x%x \n", __func__, cmd);
        goto exit;
        break;
    }

    if (copy_from_user((void *)&dma_util, (void *)arg, sizeof(dma_util_t))) {
        ret = -EFAULT;
        goto exit;
    }

    //must be cache line alignment for safe
    if (dma_util.length % DMA_CACHE_LINE_SZ) {
        printk("%s: length: %d is not %d alignment! \n", __func__, dma_util.length, DMA_CACHE_LINE_SZ);
        ret = -EFAULT;
        goto exit;
    }

    /* check the memory comes from frammap? */
    frammap_src.va_addr = dma_util.src_vaddr;
    if (frm_get_addrinfo(&frammap_src, 0)) {
        printk("%s: src_addr:0x%x is not from frammap! \n", __func__, (u32)dma_util.src_vaddr);
        ret = -EFAULT;
        goto exit;
    }

    /* check the memory comes from frammap? */
    frammap_dst.va_addr = dma_util.dst_vaddr;
    if (frm_get_addrinfo(&frammap_dst, 0)) {
        printk("%s: dst_addr:0x%x is not from frammap! \n", __func__, (u32)dma_util.dst_vaddr);
        ret = -EFAULT;
        goto exit;
    }

    /* clean cache for the src_addr */
    if (frammap_src.alloc_type != ALLOC_NONCACHABLE) {
        /* clean & invalidate in cache-fa.S */
        fmem_dcache_sync(dma_util.src_vaddr, dma_util.length, DMA_BIDIRECTIONAL);
    }

    if (frammap_dst.alloc_type != ALLOC_NONCACHABLE) {
        /* clean & invalidate in cache-fa.S */
        fmem_dcache_sync(dma_util.dst_vaddr, dma_util.length, DMA_BIDIRECTIONAL);
    }
    if (dma_memcpy(g_dma_info.dma_type, frammap_dst.phy_addr, frammap_src.phy_addr, dma_util.length)) {
        printk("%s: dma from 0x%x to 0x%x with length = %d fail! \n", __func__,
                    (u32)dma_util.src_vaddr, (u32)dma_util.dst_vaddr, dma_util.length);
        ret = -EFAULT;
        goto exit;
    }

exit:
    up(&g_dma_info.sema);

    return ret;
}

static int dma_util_open(struct inode *inode, struct file *filp)
{
    if (inode || filp) {}

    return 0;
}

static int dma_util_release(struct inode *inode, struct file *filp)
{
    return 0;
}

struct file_operations dma_util_fops = {
    owner:THIS_MODULE,
	unlocked_ioctl:	dma_util_ioctl,
	open:		dma_util_open,
	release:	dma_util_release,
};

struct miscdevice dma_util_dev = {
	minor: DMA_UTIL_MINOR,
	name: "dma_util",
	fops: &dma_util_fops,
};

static int __init dma_util_init(void)
{
    int ret;

    ret = misc_register(&dma_util_dev);
    if (ret < 0) {
        printk("%s, misc_register fail! \n", __func__);
        return ret;
    }

    memset(&g_dma_info, 0, sizeof(g_dma_info));
    sema_init(&g_dma_info.sema, DMA_CONCURRENT_NUM);
#ifdef CONFIG_FTDMAC030
    g_dma_info.dma_type = AXI_DMA;
#elif defined(CONFIG_FTDMAC020)
    g_dma_info.dma_type = AHB_DMA;
#else
    printk("Error, no dma is turned on! \n");
    ret = -1;
#endif
    return ret;
}

static void __exit dma_util_exit(void)
{
    misc_deregister(&dma_util_dev);

    return;
}

module_init(dma_util_init);
module_exit(dma_util_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("GM DMA UTIL");
MODULE_LICENSE("GPL");
