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

static struct proc_dir_entry *fcsproc, *enableproc, *paramproc;

static void fcs_get_param(fcs_param_t *param)
{
    u32 base = 0;
    u32 reg_0x28, reg_0x2c;

    base = priv.engine[0].fscaler300_reg;

    reg_0x28 = *(volatile unsigned int *)(base + 0x28);

    param->lv0_th = (reg_0x28 & 0xfff);
    param->lv1_th = (reg_0x28 & 0xff000) >> 12;
    param->lv2_th = (reg_0x28 & 0xff00000) >> 20;

    reg_0x2c = *(volatile unsigned int *)(base + 0x2c);

    param->lv3_th = (reg_0x2c & 0xff);
    param->lv4_th = (reg_0x2c & 0xff00) >> 8;
    param->grey_th = (reg_0x2c & 0xfff0000) >> 16;
}

static void fcs_set_param(FCS_PARAM_ID_T index, u32 value)
{
    u32 base = 0, base1 = 0;
    u32 reg_0x28 = 0, reg_0x2c = 0;

    base = priv.engine[0].fscaler300_reg;
    base1 = priv.engine[1].fscaler300_reg;

    switch(index) {
        case LV0_THRED:
            reg_0x28 = *(volatile unsigned int *)(base + 0x28);
            reg_0x28 &= (~0xfff);
            reg_0x28 |= value;
            *(volatile unsigned int *)(base + 0x28) = reg_0x28;
            *(volatile unsigned int *)(base1 + 0x28) = reg_0x28;
            break;
        case LV1_THRED:
            reg_0x28 = *(volatile unsigned int *)(base + 0x28);
            reg_0x28 &= (~0xff000);
            reg_0x28 |= (value << 12);
            *(volatile unsigned int *)(base + 0x28) = reg_0x28;
            *(volatile unsigned int *)(base1 + 0x28) = reg_0x28;
            break;
        case LV2_THRED:
            reg_0x28 = *(volatile unsigned int *)(base + 0x28);
            reg_0x28 &= (~0xff00000);
            reg_0x28 |= (value << 20);
            *(volatile unsigned int *)(base + 0x28) = reg_0x28;
            *(volatile unsigned int *)(base1 + 0x28) = reg_0x28;
            break;
        case LV3_THRED:
            reg_0x2c = *(volatile unsigned int *)(base + 0x2c);
            reg_0x2c &= (~0xff);
            reg_0x2c |= value;
            *(volatile unsigned int *)(base + 0x2c) = reg_0x2c;
            *(volatile unsigned int *)(base1 + 0x2c) = reg_0x2c;
            break;
        case LV4_THRED:
            reg_0x2c = *(volatile unsigned int *)(base + 0x2c);
            reg_0x2c &= (~0xff00);
            reg_0x2c |= (value << 8);
            *(volatile unsigned int *)(base + 0x2c) = reg_0x2c;
            *(volatile unsigned int *)(base1 + 0x2c) = reg_0x2c;
            break;
        case GREY_THRED:
            reg_0x2c = *(volatile unsigned int *)(base + 0x2c);
            reg_0x2c &= (~0xfff0000);
            reg_0x2c |= (value << 16);
            *(volatile unsigned int *)(base + 0x2c) = reg_0x2c;
            *(volatile unsigned int *)(base1 + 0x2c) = reg_0x2c;
            break;
        default:
            break;
    }
}

static ssize_t fcs_proc_enable_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
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

    reg_0x4 &= ~(1 << 8);

    reg_0x4 |= (enable << 8);

    *(volatile unsigned int *)(base + 0x4) = reg_0x4;

    return count;
}

static int fcs_proc_enable_show(struct seq_file *sfile, void *v)
{
    u32 base = 0;
    int dev = 0, reg_0x4 = 0, enable = 0;

    base = priv.engine[dev].fscaler300_reg;

    reg_0x4 = *(volatile unsigned int *)(base + 0x4);

    enable = (reg_0x4 > 8) & 0x1;

    seq_printf(sfile, "\nFCS enable = %d\n", enable);

    return 0;
}

static ssize_t fcs_proc_param_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int index = 0;
    u32 value = 0;
    char value_str[16] = {'\0'};

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &index, &value);

    fcs_set_param(index, value);

    return count;
}

static int fcs_proc_param_show(struct seq_file *sfile, void *v)
{
    fcs_param_t param;

    memset(&param, 0x0, sizeof(fcs_param_t));

    fcs_get_param(&param);

    seq_printf(sfile, "\n=== FCS Parameter ===\n");
    seq_printf(sfile, "[00]LV0_THRED    (0x0~0xfff) : 0x%x\n", param.lv0_th);
    seq_printf(sfile, "[01]LV1_THRED    (0x0~0xff)  : 0x%x\n", param.lv1_th);
    seq_printf(sfile, "[02]LV2_THRED    (0x0~0xff)  : 0x%x\n", param.lv2_th);
    seq_printf(sfile, "[03]LV3_THRED    (0x0~0xff)  : 0x%x\n", param.lv3_th);
    seq_printf(sfile, "[04]LV4_THRED    (0x0~0xff)  : 0x%x\n", param.lv4_th);
    seq_printf(sfile, "[05]GREY_THRED   (0x0~0xfff) : 0x%x\n", param.grey_th);

    return 0;
}

static int fcs_proc_enable_open(struct inode *inode, struct file *file)
{
    return single_open(file, fcs_proc_enable_show, PDE(inode)->data);
}

static int fcs_proc_param_open(struct inode *inode, struct file *file)
{
    return single_open(file, fcs_proc_param_show, PDE(inode)->data);
}

static struct file_operations fcs_proc_enable_ops = {
    .owner  = THIS_MODULE,
    .open   = fcs_proc_enable_open,
    .write  = fcs_proc_enable_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations fcs_proc_param_ops = {
    .owner  = THIS_MODULE,
    .open   = fcs_proc_param_open,
    .write  = fcs_proc_param_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

int fscaler300_fcs_proc_init(struct proc_dir_entry *entity_proc)
{
    int ret = 0;

    fcsproc = create_proc_entry("fcs", S_IFDIR|S_IRUGO|S_IXUGO, entity_proc);
    if (fcsproc == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        ret = -EFAULT;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    fcsproc->owner = THIS_MODULE;
#endif

    /* enable */
    enableproc = create_proc_entry("enable", S_IRUGO | S_IXUGO, fcsproc);
    if (enableproc == NULL) {
        printk("error to create %s/fcs/enable proc\n", SCALER_PROC_NAME);
        ret = -EFAULT;
    }
    enableproc->proc_fops  = &fcs_proc_enable_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    enableproc->owner = THIS_MODULE;
#endif

    /* param */
    paramproc = create_proc_entry("param", S_IRUGO|S_IXUGO, fcsproc);
    if (paramproc == NULL) {
        printk("error to create %s/fcs/param proc\n", SCALER_PROC_NAME);
        ret = -EFAULT;
    }
    paramproc->proc_fops  = &fcs_proc_param_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    paramproc->owner      = THIS_MODULE;
#endif

    return 0;
}

void fscaler300_fcs_proc_remove(struct proc_dir_entry *entity_proc)
{
    if (enableproc != 0)
        remove_proc_entry(enableproc->name, fcsproc);
    if (paramproc != 0)
        remove_proc_entry(paramproc->name, fcsproc);
    if (fcsproc != 0)
        remove_proc_entry(fcsproc->name, fcsproc->parent);
}

