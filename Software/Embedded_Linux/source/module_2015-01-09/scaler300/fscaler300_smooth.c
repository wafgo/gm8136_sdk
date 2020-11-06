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

static struct proc_dir_entry *smproc, *paramproc;

static void sm_get_param(int path, smooth_param_t *param)
{
    u32 base = 0;
    u32 reg_0x7c = 0;

    base = priv.engine[0].fscaler300_reg;

    reg_0x7c = *(volatile unsigned int *)(base + 0x7c + (path * 0x10));

    param->hsmo    = (reg_0x7c & 0x1c000000) >> 26;
    param->vsmo    = (reg_0x7c & 0xe0000000) >> 29;
}

static void sm_set_param(int path, SMOOTH_PARAM_ID_T index, u32 value)
{
    u32 base = 0, base1 = 0;
    u32 reg_0x7c[4] = {0};
    
    base1 = 0;	
    base = priv.engine[0].fscaler300_reg;
#if GM8210
    base1 = priv.engine[1].fscaler300_reg;
#endif
    switch(index) {
        case SMOOTH_HORIZONTAL_STRENGTH:
            reg_0x7c[path] = *(volatile unsigned int *)(base + 0x7c + (path * 0x10));
            reg_0x7c[path] &= (~0x1c000000);
            reg_0x7c[path] |= (value << 26);
            *(volatile unsigned int *)(base + 0x7c + path * 0x10) = reg_0x7c[path];
#if GM8210
            *(volatile unsigned int *)(base1 + 0x7c + path * 0x10) = reg_0x7c[path];
#endif
            break;
        case SMOOTH_VERTICAL_STRENGTH:
            reg_0x7c[path] = *(volatile unsigned int *)(base + 0x7c + (path * 0x10));
            reg_0x7c[path] &= (~0xe0000000);
            reg_0x7c[path] |= (value << 29);
            *(volatile unsigned int *)(base + 0x7c + path * 0x10) = reg_0x7c[path];
#if GM8210
            *(volatile unsigned int *)(base1 + 0x7c + path * 0x10) = reg_0x7c[path];
#endif
            break;
        default:
            break;
    }
}

static ssize_t sm_proc_param_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int path = 0;
    int index = 0;
    u32 value = 0;
    char value_str[16] = {'\0'};

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d %x\n", &path, &index, &value);

    sm_set_param(path, index, value);

    return count;
}

static int sm_proc_param_show(struct seq_file *sfile, void *v)
{
    int i = 0;
    smooth_param_t param[4];
#if GM8210
    int path = 4;
#else
    int path = 2;
#endif

    memset(&param, 0x0, sizeof(smooth_param_t));

    for (i = 0; i < path; i++)
        sm_get_param(i, &param[i]);

    seq_printf(sfile, "\n=== Smooth Parameter ===\n");
    for (i = 0; i < path; i++) {
        seq_printf(sfile, "\n<Path#%d>\n", i);
        seq_printf(sfile, "[00]SMOOTH_HORIZONTAL_STRENGTH  (0x0~0x7) : 0x%x\n", param[i].hsmo);
        seq_printf(sfile, "[01]SMOOTH_VERTICAL_STRENGTH    (0x0~0x7) : 0x%x\n", param[i].vsmo);
    }

    return 0;
}

static int sm_proc_param_open(struct inode *inode, struct file *file)
{
    return single_open(file, sm_proc_param_show, PDE(inode)->data);
}

static struct file_operations sm_proc_param_ops = {
    .owner  = THIS_MODULE,
    .open   = sm_proc_param_open,
    .write  = sm_proc_param_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

int fscaler300_smooth_proc_init(struct proc_dir_entry *entity_proc)
{
    int ret = 0;

    smproc = create_proc_entry("smooth", S_IFDIR|S_IRUGO|S_IXUGO, entity_proc);
    if (smproc == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        ret = -EFAULT;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    smproc->owner = THIS_MODULE;
#endif

    /* param */
    paramproc = create_proc_entry("param", S_IRUGO|S_IXUGO, smproc);
    if (paramproc == NULL) {
        printk("error to create %s/smooth/param proc\n", SCALER_PROC_NAME);
        ret = -EFAULT;
    }
    paramproc->proc_fops  = &sm_proc_param_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    paramproc->owner      = THIS_MODULE;
#endif

    return 0;
}

void fscaler300_smooth_proc_remove(struct proc_dir_entry *entity_proc)
{
    if (paramproc != 0)
        remove_proc_entry(paramproc->name, smproc);
    if (smproc != 0)
        remove_proc_entry(smproc->name, smproc->parent);
}

