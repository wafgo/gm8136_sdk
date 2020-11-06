#include <linux/version.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include "ft3dnr.h"

static struct proc_dir_entry *memproc, *mem_cache_alloc_cntproc;

static int mem_proc_mem_cache_alloc_cnt_show(struct seq_file *sfile, void *v)
{
    int count;
    count = atomic_read(&priv.mem_cnt);

    seq_printf(sfile, "mem count : %d\n", count);

    return 0;
}

static int mem_proc_mem_cache_alloc_cnt_open(struct inode *inode, struct file *file)
{
    return single_open(file, mem_proc_mem_cache_alloc_cnt_show, PDE(inode)->data);
}

static struct file_operations mem_proc_mem_cache_alloc_cnt_ops = {
    .owner  = THIS_MODULE,
    .open   = mem_proc_mem_cache_alloc_cnt_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

int ft3dnr_mem_proc_init(struct proc_dir_entry *entity_proc)
{
    int ret = 0;

    memproc = create_proc_entry("mem", S_IFDIR|S_IRUGO|S_IXUGO, entity_proc);
    if (memproc == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        ret = -EFAULT;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    memproc->owner = THIS_MODULE;
#endif

    /* DMA wc/rc wait interval */
    mem_cache_alloc_cntproc = create_proc_entry("mem_cache_alloc_cnt", S_IRUGO|S_IXUGO, memproc);
    if (mem_cache_alloc_cntproc == NULL) {
        printk("error to create %s/mem/mem_cache_alloc_cnt proc\n", FT3DNR_PROC_NAME);
        ret = -EFAULT;
    }
    mem_cache_alloc_cntproc->proc_fops  = &mem_proc_mem_cache_alloc_cnt_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    mem_cache_alloc_cntproc->owner      = THIS_MODULE;
#endif

    return 0;
}

void ft3dnr_mem_proc_remove(struct proc_dir_entry *entity_proc)
{
    if (mem_cache_alloc_cntproc != 0)
        remove_proc_entry(mem_cache_alloc_cntproc->name, memproc);

    if (memproc != 0)
        remove_proc_entry(memproc->name, memproc->parent);
}

