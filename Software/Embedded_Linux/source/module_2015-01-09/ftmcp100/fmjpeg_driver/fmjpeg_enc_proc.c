#include <linux/module.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/miscdevice.h>
#include <linux/kallsyms.h>
#include <linux/workqueue.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#include "fmcp.h"
#ifdef SUPPORT_VG
#include "fmjpeg_enc_vg2.h"
#endif

#ifdef SUPPORT_VG
#define MJE_PROC_DIR_NAME   "videograph/mje"
#else
#define MJE_PROC_DIR_NAME   "mje"
#endif

static struct proc_dir_entry *mje_proc = NULL;
static struct proc_dir_entry *mje_level_proc = NULL;
static struct proc_dir_entry *mje_info_proc = NULL;
//static struct proc_dir_entry *dbg_proc = NULL;
#ifdef SUPPORT_VG
//static struct proc_dir_entry *mje_cb_proc = NULL;
static struct proc_dir_entry *mje_util_proc = NULL;
static struct proc_dir_entry *mje_property_proc = NULL;
static struct proc_dir_entry *mje_job_proc = NULL;
#endif


#ifdef SUPPORT_VG
static unsigned int mje_job_minor = 999;
static unsigned int mje_property_minor = 0;
#endif

extern int mje_log_level;
#ifdef SUPPORT_VG
//extern unsigned int callback_period;
extern unsigned int mje_utilization_period; //5sec calculation
extern unsigned int mje_utilization_start, mje_utilization_record;
extern unsigned int mje_engine_start, mje_engine_end;
extern unsigned int mje_engine_time;
#endif

//extern void print_info(void);
#ifdef SUPPORT_VG
extern int dump_job_info(char *str, int minor, int codec_type);
extern int mje_dump_property_value(char *str, int chn);
#endif
extern int jpeg_msg(char *str);


#ifdef SUPPORT_VG
static int proc_util_write_mode(struct file *file, const char *buffer, unsigned long count, void *data)
{
    char str[0x10];
    unsigned long len = count;

    if (len >= sizeof(str))
        len = sizeof(str) - 1;
    if (copy_from_user(str, buffer, len))
        return 0;
    str[len] = '\0';
    sscanf(str, "%d", &mje_utilization_period);
    return count;
}

static int proc_util_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;

    *eof = 1;
    if (mje_utilization_start == 0)
        return len;
    len += sprintf(page+len, "JPEG encoder HW Utilization Period=%d(sec) Utilization=%d\n",
        mje_utilization_period, mje_utilization_record);

    return len;
}

static int proc_property_write_mode(struct file *file, const char *buffer, unsigned long count, void *data)
{
    char str[0x10];
    unsigned long len = count;

    if (len >= sizeof(str))
        len = sizeof(str) - 1;

    if (copy_from_user(str, buffer, len))
        return 0;
    str[len] = '\0';
    sscanf(str, "%d", &mje_property_minor);
    return count;
}

static int proc_property_read_mode(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    len += sprintf(page+len, "usage: echo [chn] > /proc/%s/property\n", MJE_PROC_DIR_NAME);
    len += mje_dump_property_value(page+len, mje_property_minor);
    *eof = 1;
    return len;
}

static int proc_job_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    char str[0x10];
    unsigned long len = count;

    if (len >= sizeof(str))
        len = sizeof(str) - 1;

    if (copy_from_user(str, buffer, len))
        return 0;
    str[len] = '\0';
    sscanf(str, "%d", &mje_job_minor);
    return count;
}


static int proc_job_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len= 0;
    len += sprintf(page+len, "usage: echo [chn] > /proc/%s/job ([chn] = 999: means dump all job)\n", MJE_PROC_DIR_NAME);
    len += sprintf(page+len, "current [chn] = %d\n", mje_job_minor);
    len += dump_job_info(page+len, mje_job_minor, TYPE_JPEG_ENCODER);
    *eof = 1;
    return len;
}
#endif

static int proc_level_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    char str[0x10];
    unsigned long len = count;

    if (len >= sizeof(str))
        len = sizeof(str) - 1;

    if (copy_from_user(str, buffer, len))
        return 0;
    str[len] = '\0';
    sscanf(str, "%d", &mje_log_level);

    return count;
}


static int proc_level_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    *eof = 1;
    return sprintf(page, "Log level = %d (%d: error, %d: warning, %d: debug, %d: info)\n", 
            mje_log_level, LOG_ERROR, LOG_WARNING, LOG_DEBUG, LOG_INFO);
}
static int proc_info_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    return 0;
}

static int proc_info_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    *eof = 1;

    len = jpeg_msg(page);

    return len;
}

#if 0
static int proc_dbg_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
/*
    printk("usage:\n");
    printk(" 97: dump rec buf log = %d\n", dump_rec_buf_log_enable);
    printk(" 98: log cnt = %d\n", log_idx);
    printk(" 99: dump buf log\n");
*/
    *eof = 1;
    return 0;
}

static int proc_dbg_write_mode(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = count;
    int cmd_idx, value;    
    char str[80];
    if (copy_from_user(str, buffer, len))
        return 0;
    str[len] = '\0';

    sscanf(str, "%d %d\n", &cmd_idx, &value);
    switch (cmd_idx) {
        default:
            break;
    }

    return count;
}
#endif
void mje_proc_exit(void)
{
    if (mje_info_proc != 0)
        remove_proc_entry(mje_info_proc->name, mje_proc);
    if (mje_util_proc != 0)
        remove_proc_entry(mje_util_proc->name, mje_proc);
    if (mje_property_proc != 0)
        remove_proc_entry(mje_property_proc->name, mje_proc);
    if (mje_job_proc != 0)
        remove_proc_entry(mje_job_proc->name, mje_proc);
    if (mje_level_proc != 0)
        remove_proc_entry(mje_level_proc->name, mje_proc);
    if (mje_proc != 0)
        remove_proc_entry(MJE_PROC_DIR_NAME, 0);
}

int mje_proc_init(void)
{
    mje_proc = create_proc_entry(MJE_PROC_DIR_NAME, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (mje_proc == NULL) {
        printk("Error to create driver proc %s\n", MJE_PROC_DIR_NAME);
        goto mje_init_proc_fail;
    }

    mje_util_proc = create_proc_entry("utilization", S_IRUGO | S_IXUGO, mje_proc);
    if (mje_util_proc == NULL) {
        printk("Error to create driver proc %s/utilization\n", MJE_PROC_DIR_NAME);
        goto mje_init_proc_fail;
    }
    mje_util_proc->read_proc = (read_proc_t *)proc_util_read_mode;
    mje_util_proc->write_proc = (write_proc_t *)proc_util_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    mje_util_proc->owner = THIS_MODULE;
#endif

    mje_property_proc = create_proc_entry("property", S_IRUGO | S_IXUGO, mje_proc);
    if (mje_property_proc == NULL) {
        printk("Error to create driver proc %s/property\n", MJE_PROC_DIR_NAME);
        goto mje_init_proc_fail;
    }
    mje_property_proc->read_proc = (read_proc_t *)proc_property_read_mode;
    mje_property_proc->write_proc = (write_proc_t *)proc_property_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    mje_property_proc->owner = THIS_MODULE;
#endif

    mje_job_proc = create_proc_entry("job", S_IRUGO | S_IXUGO, mje_proc);
    if (mje_job_proc == NULL) {
        printk("Error to create driver proc %s/job\n", MJE_PROC_DIR_NAME);
        goto mje_init_proc_fail;
    }
    mje_job_proc->read_proc = (read_proc_t *)proc_job_read_mode;
    mje_job_proc->write_proc = (write_proc_t *)proc_job_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    mje_job_proc->owner = THIS_MODULE;
#endif

    mje_level_proc = create_proc_entry("level", S_IRUGO | S_IXUGO, mje_proc);
    if (mje_level_proc == NULL) {
        printk("Error to create driver proc %s/level\n", MJE_PROC_DIR_NAME);
        goto mje_init_proc_fail;
    }
    mje_level_proc->read_proc = (read_proc_t *)proc_level_read_mode;
    mje_level_proc->write_proc = (write_proc_t *)proc_level_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    mje_level_proc->owner = THIS_MODULE;
#endif

    mje_info_proc = create_proc_entry("info", S_IRUGO | S_IXUGO, mje_proc);
    if (mje_info_proc == NULL) {
        printk("Error to create driver proc %s/info\n", MJE_PROC_DIR_NAME);
        goto mje_init_proc_fail;
    }
    mje_info_proc->read_proc = (read_proc_t *)proc_info_read_mode;
    mje_info_proc->write_proc = (write_proc_t *)proc_info_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    mje_info_proc->owner = THIS_MODULE;
#endif

    return 0;
mje_init_proc_fail:
    mje_proc_exit();
    return -EFAULT;
}


