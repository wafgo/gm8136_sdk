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

#include "adda308.h"

extern uint input_mode;
extern uint output_mode;
extern uint enable_BTL;
extern uint single_end;
extern uint enable_ALC;
extern int ALCNGTH;
extern int ALCMIN;
extern int ALCMAX;
extern int LIV;
extern int LIM;
extern int LADV;
extern int LDAV;
extern int LHM;
extern int SPV;
extern uint enable_dmic;
extern int DAMIXER;
extern int ISMASTER;
extern int reback;
extern int power_control;
extern void adda308_i2s_reset(void);
extern void adda308_mclk_init(u32 hz);
extern void adda308_ssp_mclk_init(void);

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
        printk("[adda308] fail to set digital MIC mode since driver not enable it when inserting driver\n");
        return count;
    }

    input_mode = new_mode;
    printk("input_mode = %d\n", input_mode);

    adda308_i2s_reset();

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

    adda308_i2s_reset();

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

    adda308_i2s_reset();

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

    adda308_i2s_reset();

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

    adda308_i2s_reset();

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

    adda308_i2s_reset();

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

    adda308_i2s_reset();

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

    adda308_i2s_reset();

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

    adda308_i2s_reset();

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

    adda308_i2s_reset();

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

    adda308_i2s_reset();

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

    adda308_i2s_reset();

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

    adda308_i2s_reset();

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

    adda308_i2s_reset();

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

    adda308_i2s_reset();

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

    adda308_i2s_reset();

    return count;
}

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

    adda308_i2s_reset();

    return count;
}

static int proc_read_output_mode(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current output mode = %d\n", output_mode);

    return 0;
}

static ssize_t proc_write_output_mode(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};
    uint new_mode = 0;

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%u\n", &new_mode);

    if (new_mode > 1) {
        printk("[adda308] fail to set output mode since driver not enable it when inserting driver\n");
        return count;
    }

    output_mode = new_mode;
    printk("output_mode = %d\n", output_mode);

    // set ADDA Main clock : speaker out: 24.545454MHz, line out: 24MHz
    if (output_mode == 0)
        adda308_mclk_init(24545454);
    else
        adda308_mclk_init(24000000);

    // set SSP Main clock : 24MHz
    adda308_ssp_mclk_init();

    adda308_i2s_reset();

    return count;
}

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
static struct proc_dir_entry *LIV_d = NULL;
static struct proc_dir_entry *LIM_d = NULL;
static struct proc_dir_entry *LADV_d = NULL;
static struct proc_dir_entry *LDAV_d = NULL;
static struct proc_dir_entry *LHM_d = NULL;
static struct proc_dir_entry *SPV_d = NULL;
static struct proc_dir_entry *DAMIXER_d = NULL;
static struct proc_dir_entry *ISMASTER_d = NULL;
static struct proc_dir_entry *reback_d = NULL;
static struct proc_dir_entry *power_ctrl = NULL;
static struct proc_dir_entry *output_mode_d = NULL;

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

static int proc_open_LIV(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_LIV, PDE(inode)->data);
}

static int proc_open_LIM(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_LIM, PDE(inode)->data);
}

static int proc_open_LADV(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_LADV, PDE(inode)->data);
}

static int proc_open_LDAV(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_LDAV, PDE(inode)->data);
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

static int proc_open_power_control(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_power_control, PDE(inode)->data);
}

static int proc_open_output_mode(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_output_mode, PDE(inode)->data);
}

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

static struct file_operations LIV_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_LIV,
    .write  = proc_write_LIV,
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

static struct file_operations LADV_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_LADV,
    .write  = proc_write_LADV,
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

static struct file_operations power_ctrl_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_power_control,
    .write  = proc_write_power_control,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations output_mode_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_output_mode,
    .write  = proc_write_output_mode,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

int adda308_proc_init(void)
{
    int ret = -1;

    /* create proc */
    entity_proc = create_proc_entry(ADDA308_PROC_NAME, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
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

    output_mode_d = create_proc_entry("output_mode", S_IRUGO | S_IXUGO, entity_proc);
    if (output_mode_d == NULL) {
        printk("%s fails: create input_proc not OK", __func__);
        ret = -EINVAL;
        goto err23;
    }
    output_mode_d->proc_fops  = &output_mode_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    output_mode_d->owner = THIS_MODULE;
#endif



    return 0;

err23:
    remove_proc_entry(output_mode_d->name, entity_proc);
err22:
    remove_proc_entry(power_ctrl->name, entity_proc);
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
err11:
    remove_proc_entry(LDAV_d->name, entity_proc);
err9:
    remove_proc_entry(LADV_d->name, entity_proc);
err7:
    remove_proc_entry(LIM_d->name, entity_proc);
err5:
    remove_proc_entry(LIV_d->name, entity_proc);
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

void adda308_proc_remove(void)
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
    remove_proc_entry(LDAV_d->name, entity_proc);
    remove_proc_entry(LADV_d->name, entity_proc);
    remove_proc_entry(LIM_d->name, entity_proc);
    remove_proc_entry(LIV_d->name, entity_proc);
    remove_proc_entry(single_end_mode_d->name, entity_proc);
    remove_proc_entry(BTL_mode_d->name, entity_proc);
    remove_proc_entry(input_mode_d->name, entity_proc);
    remove_proc_entry(output_mode_d->name, entity_proc);

    if (entity_proc)
        remove_proc_entry(entity_proc->name, entity_proc->parent);

}