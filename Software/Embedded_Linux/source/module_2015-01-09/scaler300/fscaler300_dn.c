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

static struct proc_dir_entry *dnproc, *enableproc, *paramproc;

static void dn_get_param(dn_param_t *param)
{
    u32 base = 0;
    u32 reg_0x70;

    base = priv.engine[0].fscaler300_reg;

    reg_0x70 = *(volatile unsigned int *)(base + 0x70);

    param->geomatric = (reg_0x70 & 0x7);
    param->similarity = (reg_0x70 & 0x38) >> 3;
    param->adp_en = (reg_0x70 & 0x40) >> 6;
    param->adp_step = (reg_0x70 & 0x1ff80) >> 7;
}

static void dn_set_param(DN_PARAM_ID_T index, u32 value)
{
    u32 base = 0, base1 = 0;
    u32 reg_0x70 = 0;

    base = priv.engine[0].fscaler300_reg;
    base1 = priv.engine[1].fscaler300_reg;

    switch(index) {
        case DN_GEOMATRIC:
            reg_0x70 = *(volatile unsigned int *)(base + 0x70);
            reg_0x70 &= (~0x7);
            reg_0x70 |= value;
            *(volatile unsigned int *)(base + 0x70) = reg_0x70;
            *(volatile unsigned int *)(base1 + 0x70) = reg_0x70;
            break;
        case DN_SIMILARITY:
            reg_0x70 = *(volatile unsigned int *)(base + 0x70);
            reg_0x70 &= (~0x38);
            reg_0x70 |= (value << 3);
            *(volatile unsigned int *)(base + 0x70) = reg_0x70;
            *(volatile unsigned int *)(base1 + 0x70) = reg_0x70;
            break;
        case DN_ADAPTIVE_ENB:
            reg_0x70 = *(volatile unsigned int *)(base + 0x70);
            reg_0x70 &= (~0x40);
            reg_0x70 |= (value << 6);
            *(volatile unsigned int *)(base + 0x70) = reg_0x70;
            *(volatile unsigned int *)(base1 + 0x70) = reg_0x70;
            break;
        case DN_ADAPTIVE_STEP:
            reg_0x70 = *(volatile unsigned int *)(base + 0x70);
            reg_0x70 &= (~0x1ff80);
            reg_0x70 |= (value << 7);
            *(volatile unsigned int *)(base + 0x70) = reg_0x70;
            *(volatile unsigned int *)(base1 + 0x70) = reg_0x70;
            break;
        default:
            break;
    }
}

static ssize_t dn_proc_enable_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 base = 0;
    int dev = 0, reg_0x4 = 0, enable = 0;
    char value_str[16] = {'\0'};

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d \n", &enable);

    base = priv.engine[dev].fscaler300_reg;

    reg_0x4 = *(volatile unsigned int *)(base + 0x4);

    reg_0x4 &= ~(1 << 9);

    reg_0x4 |= (enable << 9);

    *(volatile unsigned int *)(base + 0x4) = reg_0x4;

    return count;
}

static int dn_proc_enable_show(struct seq_file *sfile, void *v)
{
    u32 base = 0;
    int dev = 0, reg_0x4 = 0, enable = 0;

    base = priv.engine[dev].fscaler300_reg;

    reg_0x4 = *(volatile unsigned int *)(base + 0x4);

    enable = (reg_0x4 > 9) & 0x1;

    seq_printf(sfile, "\nDeNoise enable = %d\n", enable);

    return 0;
}

static ssize_t dn_proc_param_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int index = 0;
    u32 value = 0;
    char value_str[16] = {'\0'};

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &index, &value);

    dn_set_param(index, value);

    return count;
}

static int dn_proc_param_show(struct seq_file *sfile, void *v)
{
    dn_param_t param;

    memset(&param, 0x0, sizeof(dn_param_t));

    dn_get_param(&param);

    seq_printf(sfile, "\n=== DeNoise Parameter ===\n");
    seq_printf(sfile, "[00]DN_GEOMATRIC     (0x0~0x7)   : 0x%x\n", param.geomatric);
    seq_printf(sfile, "[01]DN_SIMILARITY    (0x0~0x7)   : 0x%x\n", param.similarity);
    seq_printf(sfile, "[02]DN_ADAPTIVE      (0x0~0x1)   : 0x%x\n", param.adp_en);
    seq_printf(sfile, "[03]DN_ADAPTIVE_STEP (0x0/0x8/0x10/0x20/0x40/0x80/0x100) : 0x%x\n", param.adp_step);

    return 0;
}

static int dn_proc_enable_open(struct inode *inode, struct file *file)
{
    return single_open(file, dn_proc_enable_show, PDE(inode)->data);
}

static int dn_proc_param_open(struct inode *inode, struct file *file)
{
    return single_open(file, dn_proc_param_show, PDE(inode)->data);
}

static struct file_operations dn_proc_enable_ops = {
    .owner  = THIS_MODULE,
    .open   = dn_proc_enable_open,
    .write  = dn_proc_enable_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dn_proc_param_ops = {
    .owner  = THIS_MODULE,
    .open   = dn_proc_param_open,
    .write  = dn_proc_param_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

int fscaler300_dn_proc_init(struct proc_dir_entry *entity_proc)
{
    int ret = 0;

    dnproc = create_proc_entry("denoise", S_IFDIR|S_IRUGO|S_IXUGO, entity_proc);
    if (dnproc == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        ret = -EFAULT;
   }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dnproc->owner = THIS_MODULE;
#endif

    /* enable */
    enableproc = create_proc_entry("enable", S_IRUGO|S_IXUGO, dnproc);
    if (enableproc == NULL) {
        printk("error to create %s/denoise/enable proc\n", SCALER_PROC_NAME);
        ret = -EFAULT;
    }
    enableproc->proc_fops  = &dn_proc_enable_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    enableproc->owner      = THIS_MODULE;
#endif

    /* param */
    paramproc = create_proc_entry("param", S_IRUGO|S_IXUGO, dnproc);
    if (paramproc == NULL) {
        printk("error to create %s/denoise/param proc\n", SCALER_PROC_NAME);
        ret = -EFAULT;
    }
    paramproc->proc_fops  = &dn_proc_param_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    paramproc->owner      = THIS_MODULE;
#endif

    return 0;
}

void fscaler300_dn_proc_remove(struct proc_dir_entry *entity_proc)
{
    if (enableproc != 0)
        remove_proc_entry(enableproc->name, dnproc);
    if (paramproc != 0)
        remove_proc_entry(paramproc->name, dnproc);
    if (dnproc != 0)
        remove_proc_entry(dnproc->name, dnproc->parent);
}

