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

static struct proc_dir_entry *sharpproc, *paramproc;

static void sharp_get_param(int path, sharp_param_t *param)
{
    u32 base = 0;
    u32 reg_0x84 = 0;

    base = priv.engine[0].fscaler300_reg;

    reg_0x84 = *(volatile unsigned int *)(base + 0x84 + (path * 0x10));

    param->adp       = (reg_0x84 & 0x20) >> 5;
    param->radius    = (reg_0x84 & 0x1c0) >> 6;
    param->amount    = (reg_0x84 & 0x7e00) >> 9;
    param->threshold = (reg_0x84 & 0x1f8000) >> 15;
    param->adp_start = (reg_0x84 & 0x7e00000) >> 21;
    param->adp_step  = (reg_0x84 & 0xf8000000) >> 27;

}

static void sharp_set_param(int path, SHARP_PARAM_ID_T index, u32 value)
{
    u32 base = 0, base1 = 0;
    u32 reg_0x84[4] = {0};

    if (path >= 4) {
        printk("%s, path%d out of range! \n", __func__, path);
        return;
    }
	
    base1 = 0;
    base = priv.engine[0].fscaler300_reg;
#if GM8210
    base1 = priv.engine[1].fscaler300_reg;
#endif
    switch(index) {
        case SHARP_ADAPTIVE_ENB:
            reg_0x84[path] = *(volatile unsigned int *)(base + 0x84 + (path * 0x10));
            reg_0x84[path] &= (~0x20);
            reg_0x84[path] |= (value << 5);
            *(volatile unsigned int *)(base + 0x84 + path * 0x10) = reg_0x84[path];
#if GM8210
            *(volatile unsigned int *)(base1 + 0x84 + path * 0x10) = reg_0x84[path];
#endif
            break;
        case SHARP_RADIUS:
            reg_0x84[path] = *(volatile unsigned int *)(base + 0x84 + (path * 0x10));
            reg_0x84[path] &= (~0x1c0);
            reg_0x84[path] |= (value << 6);
            *(volatile unsigned int *)(base + 0x84 + path * 0x10) = reg_0x84[path];
#if GM8210
            *(volatile unsigned int *)(base1 + 0x84 + path * 0x10) = reg_0x84[path];
#endif
            break;
        case SHARP_AMOUNT:
            reg_0x84[path] = *(volatile unsigned int *)(base + 0x84 + (path * 0x10));
            reg_0x84[path] &= (~0x7e00);
            reg_0x84[path] |= (value << 9);
            *(volatile unsigned int *)(base + 0x84 + path * 0x10) = reg_0x84[path];
#if GM8210
            *(volatile unsigned int *)(base1 + 0x84 + path * 0x10) = reg_0x84[path];
#endif
            break;
        case SHARP_THRED:
            reg_0x84[path] = *(volatile unsigned int *)(base + 0x84 + (path * 0x10));
            reg_0x84[path] &= (~0x1f8000);
            reg_0x84[path] |= (value << 15);
            *(volatile unsigned int *)(base + 0x84 + path * 0x10) = reg_0x84[path];
#if GM8210
            *(volatile unsigned int *)(base1 + 0x84 + path * 0x10) = reg_0x84[path];
#endif
            break;
        case SHARP_ADAPTIVE_START:
            reg_0x84[path] = *(volatile unsigned int *)(base + 0x84 + (path * 0x10));
            reg_0x84[path] &= (~0x7e00000);
            reg_0x84[path] |= (value << 21);
            *(volatile unsigned int *)(base + 0x84 + path * 0x10) = reg_0x84[path];
#if GM8210
            *(volatile unsigned int *)(base1 + 0x84 + path * 0x10) = reg_0x84[path];
#endif
            break;
        case SHARP_ADAPTIVE_STEP:
            reg_0x84[path] = *(volatile unsigned int *)(base + 0x84 + (path * 0x10));
            reg_0x84[path] &= (~0xf8000000);
            reg_0x84[path] |= (value << 27);
            *(volatile unsigned int *)(base + 0x84 + path * 0x10) = reg_0x84[path];
#if GM8210
            *(volatile unsigned int *)(base1 + 0x84 + path * 0x10) = reg_0x84[path];
#endif
            break;
        default:
            break;
    }
}

static ssize_t sharp_proc_param_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int path = 0;
    int index = 0;
    u32 value = 0;
    char value_str[16] = {'\0'};

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d %x\n", &path, &index, &value);

    sharp_set_param(path, index, value);

    return count;
}

static int sharp_proc_param_show(struct seq_file *sfile, void *v)
{
    int i = 0;
    sharp_param_t param[4];
#if GM8210
    int path = 4;
#else
    int path = 2;
#endif

    memset(&param, 0x0, sizeof(sharp_param_t));

    for (i = 0; i < path; i++)
        sharp_get_param(i, &param[i]);

    seq_printf(sfile, "\n=== Sharpness Parameter ===\n");
    for (i = 0; i < path; i++) {
        seq_printf(sfile, "\n<Path#%d>\n", i);
        seq_printf(sfile, "[00]SHARP_ADAPTIVE_ENABLE        (0x0~0x1)   : 0x%x\n", param[i].adp);
        seq_printf(sfile, "[01]SHARP_PARAM_RADIUS           (0x0~0x7)   : 0x%x\n", param[i].radius);
        seq_printf(sfile, "[02]SHARP_PARAM_AMOUNT           (0x0~0x3f)  : 0x%x\n", param[i].amount);
        seq_printf(sfile, "[03]SHARP_PARAM_THRED            (0x0~3f)    : 0x%x\n", param[i].threshold);
        seq_printf(sfile, "[04]SHARP_PARAM_ADAPTIVE_START   (0x0~3f)    : 0x%x\n", param[i].adp_start);
        seq_printf(sfile, "[05]SHARP_PARAM_ADAPTIVE_STEP    (0x0~0x1f)  : 0x%x\n", param[i].adp_step);
    }

    return 0;
}

static int sharp_proc_param_open(struct inode *inode, struct file *file)
{
    return single_open(file, sharp_proc_param_show, PDE(inode)->data);
}

static struct file_operations sharp_proc_param_ops = {
    .owner  = THIS_MODULE,
    .open   = sharp_proc_param_open,
    .write  = sharp_proc_param_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

int fscaler300_sharp_proc_init(struct proc_dir_entry *entity_proc)
{
    int ret = 0;

    sharpproc = create_proc_entry("sharpness", S_IFDIR|S_IRUGO|S_IXUGO, entity_proc);
    if (sharpproc == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        ret = -EFAULT;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    sharpproc->owner = THIS_MODULE;
#endif

    /* param */
    paramproc = create_proc_entry("param", S_IRUGO|S_IXUGO, sharpproc);
    if (paramproc == NULL) {
        printk("error to create %s/sharpness/param proc\n", SCALER_PROC_NAME);
        ret = -EFAULT;
    }
    paramproc->proc_fops  = &sharp_proc_param_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    paramproc->owner      = THIS_MODULE;
#endif

    return 0;
}

void fscaler300_sharp_proc_remove(struct proc_dir_entry *entity_proc)
{
    if (paramproc != 0)
        remove_proc_entry(paramproc->name, sharpproc);
    if (sharpproc != 0)
        remove_proc_entry(sharpproc->name, sharpproc->parent);
}

