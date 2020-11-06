#include <linux/version.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include "ft3dnr.h"

static struct proc_dir_entry *tmnrproc;
static struct proc_dir_entry *tmnr_paramproc, *tmnr_ctrlproc, *tmnr_ctrl_yvarproc, *tmnr_ctrl_cvarproc;

static int tmnr_proc_param_show(struct seq_file *sfile, void *v)
{
    int i;
    u32 base = priv.engine.ft3dnr_reg;
    u32 tmp = 0;
    tmnr_param_t tmnr;
    ft3dnr_rvlut_t rvlut;

    tmp = *(volatile unsigned int *)(base + 0x8C);
    tmnr.Y_var = tmp & 0xFF;
    tmnr.Cb_var = (tmp >> 8) & 0xFF;
    tmnr.Cr_var = (tmp >> 16) & 0xFF;
    tmnr.dc_offset = (tmp >> 24) & 0x7;
    tmnr.auto_recover = (tmp >> 30) & 0x1;
    tmnr.rapid_recover = (tmp >> 31) & 0x1;

    tmp = *(volatile unsigned int *)(base + 0x98);
    tmnr.learn_rate = tmp & 0x7F;
    tmnr.his_factor = (tmp >> 8) & 0xF;
    tmnr.edge_th = (tmp >> 16) & 0xFFFF;

    rvlut.rlut[0] = *(volatile unsigned int *)(base + 0x9C);
    rvlut.rlut[1] = *(volatile unsigned int *)(base + 0xA0);
    rvlut.rlut[2] = *(volatile unsigned int *)(base + 0xA4);
    rvlut.rlut[3] = *(volatile unsigned int *)(base + 0xA8);
    rvlut.rlut[4] = *(volatile unsigned int *)(base + 0xAC);
    rvlut.rlut[5] = *(volatile unsigned int *)(base + 0xB0) & 0xFF;

    rvlut.vlut[0] = *(volatile unsigned int *)(base + 0xB4);
    rvlut.vlut[1] = *(volatile unsigned int *)(base + 0xB8);
    rvlut.vlut[2] = *(volatile unsigned int *)(base + 0xBC);
    rvlut.vlut[3] = *(volatile unsigned int *)(base + 0xC0);
    rvlut.vlut[4] = *(volatile unsigned int *)(base + 0xC4);
    rvlut.vlut[5] = *(volatile unsigned int *)(base + 0xC8) & 0xFF;

    seq_printf(sfile, "Y_var = %d\n", tmnr.Y_var);
    seq_printf(sfile, "Cb_var = %d\n", tmnr.Cb_var);
    seq_printf(sfile, "Cr_var = %d\n", tmnr.Cr_var);
    seq_printf(sfile, "dc_offset = %d\n", tmnr.dc_offset);
    seq_printf(sfile, "auto_recover = %d\n", tmnr.auto_recover);
    seq_printf(sfile, "rapid_recover = %d\n", tmnr.rapid_recover);
    seq_printf(sfile, "learn_rate = %d\n", tmnr.learn_rate);
    seq_printf(sfile, "His_factor = %d\n", tmnr.his_factor);
    seq_printf(sfile, "edge_th = %d\n", tmnr.edge_th);

    for (i = 0; i < 6; i++) {
        seq_printf(sfile, "rlut%d = %d\n", i, rvlut.rlut[i]);
    }

    for (i = 0; i < 6; i++) {
        seq_printf(sfile, "vlut%d = %d\n", i, rvlut.vlut[i]);
    }

    return 0;
}

static int tmnr_proc_param_open(struct inode *inode, struct file *file)
{
    return single_open(file, tmnr_proc_param_show, PDE(inode)->data);
}

static int tmnr_proc_ctrl_yvar_show(struct seq_file *sfile, void *v)
{
    u32 base = priv.engine.ft3dnr_reg;
    u32 tmp = 0;
    tmnr_param_t tmnr;

    tmp = *(volatile unsigned int *)(base + 0x8C);
    tmnr.Y_var = tmp & 0xFF;

    seq_printf(sfile, "Y_var = %d\n", tmnr.Y_var);

    return 0;
}

static int tmnr_proc_ctrl_yvar_open(struct inode *inode, struct file *file)
{
    return single_open(file, tmnr_proc_ctrl_yvar_show, PDE(inode)->data);
}

static ssize_t tmnr_proc_ctrl_yvar_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 value = 0;
    char value_str[16] = {'\0'};

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &value);

    priv.default_param.tmnr.Y_var = value;

    return count;
}

static int tmnr_proc_ctrl_cvar_show(struct seq_file *sfile, void *v)
{
    u32 base = priv.engine.ft3dnr_reg;
    u32 tmp = 0;
    tmnr_param_t tmnr;

    tmp = *(volatile unsigned int *)(base + 0x8C);
    tmnr.Cb_var = (tmp >> 8) & 0xFF;
    tmnr.Cr_var = (tmp >> 16) & 0xFF;

    seq_printf(sfile, "Cb_var = %d\n", tmnr.Cb_var);
    seq_printf(sfile, "Cr_var = %d\n", tmnr.Cr_var);

    return 0;
}

static int tmnr_proc_ctrl_cvar_open(struct inode *inode, struct file *file)
{
    return single_open(file, tmnr_proc_ctrl_cvar_show, PDE(inode)->data);
}

static ssize_t tmnr_proc_ctrl_cvar_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 value = 0;
    char value_str[16] = {'\0'};

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &value);

    priv.default_param.tmnr.Cb_var = value;
    priv.default_param.tmnr.Cr_var = value;

    return count;
}

static struct file_operations tmnr_proc_param_ops = {
    .owner  = THIS_MODULE,
    .open   = tmnr_proc_param_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tmnr_proc_ctrl_yvar_ops = {
    .owner  = THIS_MODULE,
    .open   = tmnr_proc_ctrl_yvar_open,
    .write  = tmnr_proc_ctrl_yvar_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tmnr_proc_ctrl_cvar_ops = {
    .owner  = THIS_MODULE,
    .open   = tmnr_proc_ctrl_cvar_open,
    .write  = tmnr_proc_ctrl_cvar_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

int ft3dnr_tmnr_proc_init(struct proc_dir_entry *entity_proc)
{
    int ret = 0;

    tmnrproc = create_proc_entry("tmnr", S_IFDIR|S_IRUGO|S_IXUGO, entity_proc);
    if (tmnrproc == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        ret = -EFAULT;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tmnrproc->owner = THIS_MODULE;
#endif

    /* the tmnr parameter content of controller register */
    tmnr_paramproc = create_proc_entry("param", S_IRUGO|S_IXUGO, tmnrproc);
    if (tmnr_paramproc == NULL) {
        printk("error to create %s/tmnr/param proc\n", FT3DNR_PROC_NAME);
        ret = -EFAULT;
    }
    tmnr_paramproc->proc_fops = &tmnr_proc_param_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tmnr_paramproc->owner = THIS_MODULE;
#endif

    /* provide the control interface for users */
    tmnr_ctrlproc = create_proc_entry("control", S_IFDIR|S_IRUGO|S_IXUGO, tmnrproc);
    if (tmnr_ctrlproc == NULL) {
        printk("error to create %s/tmnr/control proc\n", FT3DNR_PROC_NAME);
        ret = -EFAULT;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tmnr_ctrlproc->owner = THIS_MODULE;
#endif

    /* the control interface of Y_var parameter */
    tmnr_ctrl_yvarproc = create_proc_entry("Y_var", S_IRUGO|S_IXUGO, tmnr_ctrlproc);
    if (tmnr_ctrl_yvarproc == NULL) {
        printk("error to create %s/tmnr/control/Y_var proc\n", FT3DNR_PROC_NAME);
        ret = -EFAULT;
    }
    tmnr_ctrl_yvarproc->proc_fops = &tmnr_proc_ctrl_yvar_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tmnr_ctrl_yvarproc->owner = THIS_MODULE;
#endif

    /* the control interface of C_var parameter */
    tmnr_ctrl_cvarproc = create_proc_entry("C_var", S_IRUGO|S_IXUGO, tmnr_ctrlproc);
    if (tmnr_ctrl_cvarproc == NULL) {
        printk("error to create %s/tmnr/control/C_var proc\n", FT3DNR_PROC_NAME);
        ret = -EFAULT;
    }
    tmnr_ctrl_cvarproc->proc_fops = &tmnr_proc_ctrl_cvar_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tmnr_ctrl_cvarproc->owner = THIS_MODULE;
#endif

    return 0;
}

void ft3dnr_tmnr_proc_remove(struct proc_dir_entry *entity_proc)
{
    if (tmnr_ctrl_cvarproc)
        remove_proc_entry(tmnr_ctrl_cvarproc->name, tmnr_ctrlproc);
    if (tmnr_ctrl_yvarproc)
        remove_proc_entry(tmnr_ctrl_yvarproc->name, tmnr_ctrlproc);
    if (tmnr_ctrlproc)
        remove_proc_entry(tmnr_ctrlproc->name, tmnrproc);
    if (tmnr_paramproc)
        remove_proc_entry(tmnr_paramproc->name, tmnrproc);

    if (tmnrproc)
        remove_proc_entry(tmnrproc->name, tmnrproc->parent);
}
