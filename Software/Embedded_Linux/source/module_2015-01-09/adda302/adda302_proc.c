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

#include "adda302.h"

extern uint input_mode;
extern uint enable_BTL;
extern uint single_end;
extern uint enable_ALC;
extern int ALCNGTH;
extern int ALCMIN;
extern int ALCMAX;
extern int RIV;
extern int LIV;
extern int RIM;
extern int LIM;
extern int RADV;
extern int LADV;
extern int RDAV;
extern int LDAV;
extern int RHM;
extern int LHM;
extern int SPV;
extern uint enable_dmic;
extern int DAMIXER;
extern int ISMASTER;
extern int reback;
#if defined(CONFIG_PLATFORM_GM8136)
extern int power_control;
#endif
extern void adda302_i2s_reset(void);

static int proc_read_input_mode(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current input mode = %d\n", input_mode);

    return 0;
}

static ssize_t proc_write_input_mode(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};
    uint new_mode = 0;

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%u\n", &new_mode);

    if ((new_mode == 2) && (!enable_dmic)) {
        printk("[ADDA302] fail to set digital MIC mode since driver not enable it when inserting driver\n");
        return count;
    }

    input_mode = new_mode;
    printk("input_mode = %d\n", input_mode);

    adda302_i2s_reset();

    return count;
}

static int proc_read_BTL_mode(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current enable_BTL = %d\n", enable_BTL);

    return 0;
}

static ssize_t proc_write_BTL_mode(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%u\n", &enable_BTL);

    printk("enable_BTL = %d\n", enable_BTL);

    adda302_i2s_reset();

    return count;
}

static int proc_read_single_end_mode(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current single_end = %d\n", single_end);

    return 0;
}

static ssize_t proc_write_single_end_mode(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%u\n", &single_end);

    printk("single_end = %d\n", single_end);

    adda302_i2s_reset();

    return count;
}


static int proc_read_RIV(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "RIV should be between -27~36dB\n");
    seq_printf(sfile, "current RIV = %d\n", RIV);

    return 0;
}

static ssize_t proc_write_RIV(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &RIV);

    adda302_i2s_reset();

    return count;
}

static int proc_read_ALC_mode(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current enable_ALC = %d\n", enable_ALC);

    return 0;
}

static ssize_t proc_write_ALC_mode(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &enable_ALC);

    adda302_i2s_reset();

    return count;
}

static int proc_read_ALCNGTH(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current ALCNGTH = %d\n", ALCNGTH);

    return 0;
}

static ssize_t proc_write_ALCNGTH(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &ALCNGTH);

    adda302_i2s_reset();

    return count;
}

static int proc_read_ALCMIN(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current ALCMIN = %d\n", ALCMIN);

    return 0;
}

static ssize_t proc_write_ALCMIN(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &ALCMIN);

    adda302_i2s_reset();

    return count;
}

static int proc_read_ALCMAX(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current ALCMAX = %d\n", ALCMAX);

    return 0;
}

static ssize_t proc_write_ALCMAX(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &ALCMAX);

    adda302_i2s_reset();

    return count;
}

static int proc_read_LIV(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "LIV should be between -27~36dB\n");
    seq_printf(sfile, "current LIV = %d\n", LIV);

    return 0;
}

static ssize_t proc_write_LIV(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &LIV);

    adda302_i2s_reset();

    return count;
}

static int proc_read_RIM(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current RIM = %d\n", RIM);

    return 0;
}

static ssize_t proc_write_RIM(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &RIM);

    adda302_i2s_reset();

    return count;
}

static int proc_read_LIM(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current LIM = %d\n", LIM);

    return 0;
}

static ssize_t proc_write_LIM(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &LIM);

    adda302_i2s_reset();

    return count;
}

static int proc_read_RADV(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "RADV should be between -50~30db\n");
    seq_printf(sfile, "current RADV = %d\n", RADV);

    return 0;
}

static ssize_t proc_write_RADV(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &RADV);

    adda302_i2s_reset();

    return count;
}

static int proc_read_LADV(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "LADV should be between -50~30dB\n");
    seq_printf(sfile, "current LADV = %d\n", LADV);

    return 0;
}

static ssize_t proc_write_LADV(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &LADV);

    adda302_i2s_reset();

    return count;
}

static int proc_read_RDAV(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "RDAV should be between -40~0dB\n");
    seq_printf(sfile, "current RDAV = %d\n", RDAV);

    return 0;
}

static ssize_t proc_write_RDAV(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &RDAV);

    adda302_i2s_reset();

    return count;
}

static int proc_read_LDAV(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "LDAV should be between -40~0dB\n");
    seq_printf(sfile, "current LDAV = %d\n", LDAV);

    return 0;
}

static ssize_t proc_write_LDAV(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &LDAV);

    adda302_i2s_reset();

    return count;
}

static int proc_read_RHM(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current RHM = %d\n", RHM);

    return 0;
}

static ssize_t proc_write_RHM(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &RHM);

    adda302_i2s_reset();

    return count;
}

static int proc_read_LHM(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current LHM = %d\n", LHM);

    return 0;
}

static ssize_t proc_write_LHM(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &LHM);

    adda302_i2s_reset();

    return count;
}

static int proc_read_SPV(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "SPV should be between -40~6dB\n");
    seq_printf(sfile, "current SPV = %d\n", SPV);

    return 0;
}

static ssize_t proc_write_SPV(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &SPV);

    adda302_i2s_reset();

    return count;
}

static int proc_read_DAMIXER(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current DAMIXER = %d\n", DAMIXER);

    return 0;
}

static ssize_t proc_write_DAMIXER(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &DAMIXER);

    adda302_i2s_reset();

    return count;
}

static int proc_read_ISMASTER(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current ISMASTER = %d\n", ISMASTER);

    return 0;
}

static ssize_t proc_write_ISMASTER(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &ISMASTER);

    adda302_i2s_reset();

    return count;
}

static int proc_read_reback(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current REBACK = %d\n", reback);

    return 0;
}

static ssize_t proc_write_reback(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &reback);

    adda302_i2s_reset();

    return count;
}

#if defined(CONFIG_PLATFORM_GM8136)
static int proc_read_power_control(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\nADDA Power Control = %s\n\n", (power_control==1)?("On"):("Off"));

    return 0;
}

static ssize_t proc_write_power_control(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &power_control);

    adda302_i2s_reset();

    return count;
}
#endif

/**
 * hold proc pointer
 */
static struct proc_dir_entry *entity_proc = NULL;
static struct proc_dir_entry *input_mode_d = NULL;
static struct proc_dir_entry *BTL_mode_d = NULL;
static struct proc_dir_entry *single_end_mode_d = NULL;
static struct proc_dir_entry *ALC_mode_d = NULL;
static struct proc_dir_entry *ALCNGTH_d = NULL;
static struct proc_dir_entry *ALCMIN_d = NULL;
static struct proc_dir_entry *ALCMAX_d = NULL;
static struct proc_dir_entry *RIV_d = NULL;
static struct proc_dir_entry *LIV_d = NULL;
static struct proc_dir_entry *RIM_d = NULL;
static struct proc_dir_entry *LIM_d = NULL;
static struct proc_dir_entry *RADV_d = NULL;
static struct proc_dir_entry *LADV_d = NULL;
static struct proc_dir_entry *RDAV_d = NULL;
static struct proc_dir_entry *LDAV_d = NULL;
static struct proc_dir_entry *RHM_d = NULL;
static struct proc_dir_entry *LHM_d = NULL;
static struct proc_dir_entry *SPV_d = NULL;
static struct proc_dir_entry *DAMIXER_d = NULL;
static struct proc_dir_entry *ISMASTER_d = NULL;
static struct proc_dir_entry *reback_d = NULL;
#if defined(CONFIG_PLATFORM_GM8136)
static struct proc_dir_entry *power_ctrl = NULL;
#endif

static int proc_open_input_mode(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_input_mode, PDE(inode)->data);
}

static int proc_open_BTL_mode(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_BTL_mode, PDE(inode)->data);
}

static int proc_open_single_end_mode(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_single_end_mode, PDE(inode)->data);
}

static int proc_open_RIV(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_RIV, PDE(inode)->data);
}

static int proc_open_LIV(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_LIV, PDE(inode)->data);
}

static int proc_open_RIM(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_RIM, PDE(inode)->data);
}

static int proc_open_LIM(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_LIM, PDE(inode)->data);
}

static int proc_open_RADV(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_RADV, PDE(inode)->data);
}

static int proc_open_LADV(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_LADV, PDE(inode)->data);
}

static int proc_open_RDAV(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_RDAV, PDE(inode)->data);
}

static int proc_open_LDAV(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_LDAV, PDE(inode)->data);
}

static int proc_open_RHM(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_RHM, PDE(inode)->data);
}

static int proc_open_LHM(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_LHM, PDE(inode)->data);
}

static int proc_open_SPV(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_SPV, PDE(inode)->data);
}

static int proc_open_ALC_mode(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_ALC_mode, PDE(inode)->data);
}

static int proc_open_ALCMIN(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_ALCMIN, PDE(inode)->data);
}

static int proc_open_ALCMAX(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_ALCMAX, PDE(inode)->data);
}

static int proc_open_ALCNGTH(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_ALCNGTH, PDE(inode)->data);
}

static int proc_open_DAMIXER(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_DAMIXER, PDE(inode)->data);
}

static int proc_open_ISMASTER(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_ISMASTER, PDE(inode)->data);
}

static int proc_open_reback(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_reback, PDE(inode)->data);
}

#if defined(CONFIG_PLATFORM_GM8136)
static int proc_open_power_control(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_power_control, PDE(inode)->data);
}
#endif

static struct file_operations input_mode_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_input_mode,
    .write  = proc_write_input_mode,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations BTL_mode_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_BTL_mode,
    .write  = proc_write_BTL_mode,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations single_end_mode_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_single_end_mode,
    .write  = proc_write_single_end_mode,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations RIV_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_RIV,
    .write  = proc_write_RIV,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations LIV_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_LIV,
    .write  = proc_write_LIV,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations RIM_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_RIM,
    .write  = proc_write_RIM,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations LIM_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_LIM,
    .write  = proc_write_LIM,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations RADV_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_RADV,
    .write  = proc_write_RADV,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations LADV_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_LADV,
    .write  = proc_write_LADV,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations RDAV_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_RDAV,
    .write  = proc_write_RDAV,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations LDAV_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_LDAV,
    .write  = proc_write_LDAV,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations RHM_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_RHM,
    .write  = proc_write_RHM,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations LHM_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_LHM,
    .write  = proc_write_LHM,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations SPV_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_SPV,
    .write  = proc_write_SPV,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations ALC_mode_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_ALC_mode,
    .write  = proc_write_ALC_mode,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations ALCMIN_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_ALCMIN,
    .write  = proc_write_ALCMIN,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations ALCMAX_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_ALCMAX,
    .write  = proc_write_ALCMAX,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations ALCNGTH_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_ALCNGTH,
    .write  = proc_write_ALCNGTH,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations DAMIXER_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_DAMIXER,
    .write  = proc_write_DAMIXER,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations ISMASTER_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_ISMASTER,
    .write  = proc_write_ISMASTER,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations reback_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_reback,
    .write  = proc_write_reback,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

#if defined(CONFIG_PLATFORM_GM8136)
static struct file_operations power_ctrl_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_power_control,
    .write  = proc_write_power_control,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};
#endif

int adda302_proc_init(void)
{
    int ret = -1;

    /* create proc */
    entity_proc = create_proc_entry(ADDA302_PROC_NAME, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (entity_proc == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        ret = -EFAULT;
        goto err0;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    entity_proc->owner = THIS_MODULE;
#endif

    input_mode_d = create_proc_entry("input_mode", S_IRUGO | S_IXUGO, entity_proc);
    if (input_mode_d == NULL) {
        printk("%s fails: create input_proc not OK", __func__);
        ret = -EINVAL;
        goto err1;
    }
    input_mode_d->proc_fops  = &input_mode_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    input_mode_d->owner = THIS_MODULE;
#endif

    BTL_mode_d = create_proc_entry("BTL_mode", S_IRUGO | S_IXUGO, entity_proc);
    if (BTL_mode_d == NULL) {
        printk("%s fails: create BTL proc not OK", __func__);
        ret = -EINVAL;
        goto err2;
    }
    BTL_mode_d->proc_fops  = &BTL_mode_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    BTL_mode_d->owner = THIS_MODULE;
#endif
    single_end_mode_d = create_proc_entry("single_end", S_IRUGO | S_IXUGO, entity_proc);
    if (single_end_mode_d == NULL) {
        printk("%s fails: create single end proc not OK", __func__);
        ret = -EINVAL;
        goto err3;
    }
    single_end_mode_d->proc_fops  = &single_end_mode_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    single_end_mode_d->owner = THIS_MODULE;
#endif
    RIV_d = create_proc_entry("RIV", S_IRUGO | S_IXUGO, entity_proc);
    if (RIV_d == NULL) {
        printk("%s fails: create RIV proc not OK", __func__);
        ret = -EINVAL;
        goto err4;
    }
    RIV_d->proc_fops  = &RIV_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    RIV_d->owner = THIS_MODULE;
#endif
    LIV_d = create_proc_entry("LIV", S_IRUGO | S_IXUGO, entity_proc);
    if (LIV_d == NULL) {
        printk("%s fails: create LIV proc not OK", __func__);
        ret = -EINVAL;
        goto err5;
    }
    LIV_d->proc_fops  = &LIV_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    LIV_d->owner = THIS_MODULE;
#endif
    RIM_d = create_proc_entry("RIM", S_IRUGO | S_IXUGO, entity_proc);
    if (RIM_d == NULL) {
        printk("%s fails: create RIM proc not OK", __func__);
        ret = -EINVAL;
        goto err6;
    }
    RIM_d->proc_fops  = &RIM_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    RIM_d->owner = THIS_MODULE;
#endif
    LIM_d = create_proc_entry("LIM", S_IRUGO | S_IXUGO, entity_proc);
    if (LIM_d == NULL) {
        printk("%s fails: create LIM proc not OK", __func__);
        ret = -EINVAL;
        goto err7;
    }
    LIM_d->proc_fops  = &LIM_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    LIM_d->owner = THIS_MODULE;
#endif

    RADV_d = create_proc_entry("RADV", S_IRUGO | S_IXUGO, entity_proc);
    if (RADV_d == NULL) {
        printk("%s fails: create RADV proc not OK", __func__);
        ret = -EINVAL;
        goto err8;
    }
    RADV_d->proc_fops  = &RADV_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    RADV_d->owner = THIS_MODULE;
#endif
    LADV_d = create_proc_entry("LADV", S_IRUGO | S_IXUGO, entity_proc);
    if (LADV_d == NULL) {
        printk("%s fails: create LADV proc not OK", __func__);
        ret = -EINVAL;
        goto err9;
    }
    LADV_d->proc_fops  = &LADV_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    LADV_d->owner = THIS_MODULE;
#endif
    RDAV_d = create_proc_entry("RDAV", S_IRUGO | S_IXUGO, entity_proc);
    if (RDAV_d == NULL) {
        printk("%s fails: create RDAV proc not OK", __func__);
        ret = -EINVAL;
        goto err10;
    }
    RDAV_d->proc_fops  = &RDAV_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    RDAV_d->owner = THIS_MODULE;
#endif
    LDAV_d = create_proc_entry("LDAV", S_IRUGO | S_IXUGO, entity_proc);
    if (LDAV_d == NULL) {
        printk("%s fails: create LDAV proc not OK", __func__);
        ret = -EINVAL;
        goto err11;
    }
    LDAV_d->proc_fops  = &LDAV_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    LDAV_d->owner = THIS_MODULE;
#endif
    RHM_d = create_proc_entry("RHM", S_IRUGO | S_IXUGO, entity_proc);
    if (RHM_d == NULL) {
        printk("%s fails: create RHM proc not OK", __func__);
        ret = -EINVAL;
        goto err12;
    }
    RHM_d->proc_fops  = &RHM_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    RHM_d->owner = THIS_MODULE;
#endif
    LHM_d = create_proc_entry("LHM", S_IRUGO | S_IXUGO, entity_proc);
    if (LHM_d == NULL) {
        printk("%s fails: create LHM proc not OK", __func__);
        ret = -EINVAL;
        goto err13;
    }
    LHM_d->proc_fops  = &LHM_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    LHM_d->owner = THIS_MODULE;
#endif
    SPV_d = create_proc_entry("SPV", S_IRUGO | S_IXUGO, entity_proc);
    if (SPV_d == NULL) {
        printk("%s fails: create SPV proc not OK", __func__);
        ret = -EINVAL;
        goto err14;
    }
    SPV_d->proc_fops  = &SPV_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    SPV_d->owner = THIS_MODULE;
#endif
    ALC_mode_d = create_proc_entry("ALC_mode", S_IRUGO | S_IXUGO, entity_proc);
    if (ALC_mode_d == NULL) {
        printk("%s fails: create ALC proc not OK", __func__);
        ret = -EINVAL;
        goto err15;
    }
    ALC_mode_d->proc_fops  = &ALC_mode_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    ALC_mode_d->owner = THIS_MODULE;
#endif
    ALCMIN_d = create_proc_entry("ALCMIN", S_IRUGO | S_IXUGO, entity_proc);
    if (ALCMIN_d == NULL) {
        printk("%s fails: create ALCMIN proc not OK", __func__);
        ret = -EINVAL;
        goto err16;
    }
    ALCMIN_d->proc_fops  = &ALCMIN_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    ALCMIN_d->owner = THIS_MODULE;
#endif
    ALCMAX_d = create_proc_entry("ALCMAX", S_IRUGO | S_IXUGO, entity_proc);
    if (ALCMAX_d == NULL) {
        printk("%s fails: create ALCMAX proc not OK", __func__);
        ret = -EINVAL;
        goto err17;
    }
    ALCMAX_d->proc_fops  = &ALCMAX_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    ALCMAX_d->owner = THIS_MODULE;
#endif
    ALCNGTH_d = create_proc_entry("ALCNGTH", S_IRUGO | S_IXUGO, entity_proc);
    if (ALCNGTH_d == NULL) {
        printk("%s fails: create ALCNGTH proc not OK", __func__);
        ret = -EINVAL;
        goto err18;
    }
    ALCNGTH_d->proc_fops  = &ALCNGTH_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    ALCNGTH_d->owner = THIS_MODULE;
#endif
    DAMIXER_d = create_proc_entry("DAMIXER", S_IRUGO | S_IXUGO, entity_proc);
    if (DAMIXER_d == NULL) {
        printk("%s fails: create DAMIXER proc not OK", __func__);
        ret = -EINVAL;
        goto err19;
    }
    DAMIXER_d->proc_fops  = &DAMIXER_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    DAMIXER_d->owner = THIS_MODULE;
#endif
    ISMASTER_d = create_proc_entry("ISMASTER", S_IRUGO | S_IXUGO, entity_proc);
    if (ISMASTER_d == NULL) {
        printk("%s fails: create ISMASTER proc not OK", __func__);
        ret = -EINVAL;
        goto err20;
    }
    ISMASTER_d->proc_fops  = &ISMASTER_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    ISMASTER_d->owner = THIS_MODULE;
#endif

    reback_d = create_proc_entry("REBACK", S_IRUGO | S_IXUGO, entity_proc);
    if (reback_d == NULL) {
        printk("%s fails: create REBACK proc not OK", __func__);
        ret = -EINVAL;
        goto err21;
    }
    reback_d->proc_fops  = &reback_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    reback_d->owner = THIS_MODULE;
#endif

#if defined(CONFIG_PLATFORM_GM8136)
    power_ctrl = create_proc_entry("power_control", S_IRUGO | S_IXUGO, entity_proc);
    if (power_ctrl == NULL) {
        printk("%s fails: create power_control proc not OK", __func__);
        ret = -EINVAL;
        goto err22;
    }
    power_ctrl->proc_fops  = &power_ctrl_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    power_ctrl->owner = THIS_MODULE;
#endif
#endif

    return 0;

#if defined(CONFIG_PLATFORM_GM8136)    
err22:
    remove_proc_entry(power_ctrl->name, entity_proc);
#endif    
err21:
    remove_proc_entry(reback_d->name, entity_proc);
err20:
    remove_proc_entry(ISMASTER_d->name, entity_proc);
err19:
    remove_proc_entry(DAMIXER_d->name, entity_proc);
err18:
    remove_proc_entry(ALCNGTH_d->name, entity_proc);
err17:
    remove_proc_entry(ALCMAX_d->name, entity_proc);
err16:
    remove_proc_entry(ALCMIN_d->name, entity_proc);
err15:
    remove_proc_entry(ALC_mode_d->name, entity_proc);
err14:
    remove_proc_entry(SPV_d->name, entity_proc);
err13:
    remove_proc_entry(LHM_d->name, entity_proc);
err12:
    remove_proc_entry(RHM_d->name, entity_proc);
err11:
    remove_proc_entry(LDAV_d->name, entity_proc);
err10:
    remove_proc_entry(RDAV_d->name, entity_proc);
err9:
    remove_proc_entry(LADV_d->name, entity_proc);
err8:
    remove_proc_entry(RADV_d->name, entity_proc);
err7:
    remove_proc_entry(LIM_d->name, entity_proc);
err6:
    remove_proc_entry(RIM_d->name, entity_proc);
err5:
    remove_proc_entry(LIV_d->name, entity_proc);
err4:
    remove_proc_entry(RIV_d->name, entity_proc);
err3:
    remove_proc_entry(single_end_mode_d->name, entity_proc);
err2:
    remove_proc_entry(BTL_mode_d->name, entity_proc);
err1:
    remove_proc_entry(input_mode_d->name, entity_proc);
err0:
    remove_proc_entry(entity_proc->name, entity_proc->parent);

    return ret;
}

void adda302_proc_remove(void)
{
#if defined(CONFIG_PLATFORM_GM8136)
    remove_proc_entry(power_ctrl->name, entity_proc);
#endif
    remove_proc_entry(reback_d->name, entity_proc);
    remove_proc_entry(ISMASTER_d->name, entity_proc);
    remove_proc_entry(DAMIXER_d->name, entity_proc);
    remove_proc_entry(ALCNGTH_d->name, entity_proc);
    remove_proc_entry(ALCMAX_d->name, entity_proc);
    remove_proc_entry(ALCMIN_d->name, entity_proc);
    remove_proc_entry(ALC_mode_d->name, entity_proc);
    remove_proc_entry(SPV_d->name, entity_proc);
    remove_proc_entry(LHM_d->name, entity_proc);
    remove_proc_entry(RHM_d->name, entity_proc);
    remove_proc_entry(LDAV_d->name, entity_proc);
    remove_proc_entry(RDAV_d->name, entity_proc);
    remove_proc_entry(LADV_d->name, entity_proc);
    remove_proc_entry(RADV_d->name, entity_proc);
    remove_proc_entry(LIM_d->name, entity_proc);
    remove_proc_entry(RIM_d->name, entity_proc);
    remove_proc_entry(LIV_d->name, entity_proc);
    remove_proc_entry(RIV_d->name, entity_proc);
    remove_proc_entry(single_end_mode_d->name, entity_proc);
    remove_proc_entry(BTL_mode_d->name, entity_proc);
    remove_proc_entry(input_mode_d->name, entity_proc);

    if (entity_proc)
        remove_proc_entry(entity_proc->name, entity_proc->parent);

}