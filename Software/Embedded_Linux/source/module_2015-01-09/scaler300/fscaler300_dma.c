#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include "fscaler300.h"

static struct proc_dir_entry *dmaproc, *paramproc;

static void dma_get_param(int engine, dma_param_t *param)
{
    u32 base = 0;
    u32 reg_0x5438, reg_0x543c;

    base = priv.engine[engine].fscaler300_reg;

    reg_0x5438 = *(volatile unsigned int *)(base + 0x5438);
    param->wc_wait_value = (reg_0x5438 >> 16) & 0xffff;

    reg_0x543c = *(volatile unsigned int *)(base + 0x543c);
    param->rc_wait_value = reg_0x543c & 0xffff;
}

static void dma_set_param(int engine, DMA_PARAM_ID_T index, u32 value)
{
    u32 base = 0;
    u32 reg_0x5438 = 0, reg_0x543c = 0;

    base = priv.engine[engine].fscaler300_reg;

    switch(index) {
        case WC_WAIT_VALUE:
            reg_0x5438 = *(volatile unsigned int *)(base + 0x5438);
            reg_0x5438 &= ~0xffff0000;
            reg_0x5438 |= (value << 16);
            *(volatile unsigned int *)(base + 0x5438) = reg_0x5438;
            priv.global_param.init.dma_ctrl.dma_wc_wait_value = value;
            break;
        case RC_WAIT_VALUE:
            reg_0x543c = *(volatile unsigned int *)(base + 0x543c);
            reg_0x543c &= ~0xffff;
            reg_0x543c |= value;
            *(volatile unsigned int *)(base + 0x543c) = reg_0x543c;
            priv.global_param.init.dma_ctrl.dma_rc_wait_value = value;
            break;
        default:
            break;
    }
}

static ssize_t dma_proc_param_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int index = 0;
    u32 value = 0;
    int engine = 0;
    char value_str[16] = {'\0'};

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d %x\n", &engine, &index, &value);

#if GM8210
    if (engine > 1) {
        printk("scaler engine number should be 0 or 1\n");
        return count;
    }
#else
    if (engine > 0) {
        printk("scaler engine number should be 0\n");
        return count;
    }
#endif
    dma_set_param(engine, index, value);

    return count;
}

static int dma_proc_param_show(struct seq_file *sfile, void *v)
{
    dma_param_t param;

    memset(&param, 0x0, sizeof(dma_param_t));

    dma_get_param(0, &param);

    seq_printf(sfile, "\n=== Engine 0 DMA Parameter ===\n");
    seq_printf(sfile, "[00]WC_WAIT_VALUE (0x0~0xffff) : 0x%x\n", param.wc_wait_value);
    seq_printf(sfile, "[01]RC_WAIT_VALUE (0x0~0xffff) : 0x%x\n", param.rc_wait_value);

#if GM8210
    memset(&param, 0x0, sizeof(dma_param_t));

    dma_get_param(1, &param);

    seq_printf(sfile, "\n=== Engine 1 DMA Parameter ===\n");
    seq_printf(sfile, "[00]WC_WAIT_VALUE (0x0~0xffff) : 0x%x\n", param.wc_wait_value);
    seq_printf(sfile, "[01]RC_WAIT_VALUE (0x0~0xffff) : 0x%x\n", param.rc_wait_value);
#endif

    return 0;
}

static int dma_proc_param_open(struct inode *inode, struct file *file)
{
    return single_open(file, dma_proc_param_show, PDE(inode)->data);
}

static struct file_operations dma_proc_param_ops = {
    .owner  = THIS_MODULE,
    .open   = dma_proc_param_open,
    .write  = dma_proc_param_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

int fscaler300_dma_proc_init(struct proc_dir_entry *entity_proc)
{
    int ret = 0;

    dmaproc = create_proc_entry("dma", S_IFDIR|S_IRUGO|S_IXUGO, entity_proc);
    if (dmaproc == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        ret = -EFAULT;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dmaproc->owner = THIS_MODULE;
#endif

    /* DMA wc/rc wait interval */
    paramproc = create_proc_entry("param", S_IRUGO|S_IXUGO, dmaproc);
    if (paramproc == NULL) {
        printk("error to create %s/dma/param proc\n", SCALER_PROC_NAME);
        ret = -EFAULT;
    }
    paramproc->proc_fops  = &dma_proc_param_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    paramproc->owner      = THIS_MODULE;
#endif

    return 0;
}

void fscaler300_dma_proc_remove(struct proc_dir_entry *entity_proc)
{
    if (paramproc != 0)
        remove_proc_entry(paramproc->name, dmaproc);
    if (dmaproc != 0)
        remove_proc_entry(dmaproc->name, dmaproc->parent);
}

