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
    #define MJD_PROC_DIR_NAME   "videograph/mjd"
#else
    #define MJD_PROC_DIR_NAME   "mjd"
#endif

static struct proc_dir_entry *mjd_proc = NULL;
static struct proc_dir_entry *mjd_level_proc = NULL;
//static struct proc_dir_entry *mjd_info_proc = NULL;
//static struct proc_dir_entry *dbg_proc = NULL;
#ifdef SUPPORT_VG
//static struct proc_dir_entry *mjd_cb_proc = NULL;
    static struct proc_dir_entry *mjd_util_proc = NULL;
    static struct proc_dir_entry *mjd_property_proc = NULL;
    static struct proc_dir_entry *mjd_job_proc = NULL;
#endif


#ifdef SUPPORT_VG
    static unsigned int mjd_job_minor= 999;
    static unsigned int mjd_property_minor = 0;
#endif

extern int mjd_log_level;
#ifdef SUPPORT_VG
    extern unsigned int mjd_utilization_period;
    extern unsigned int mjd_utilization_start, mjd_utilization_record;
    extern unsigned int mjd_engine_start, mjd_engine_end;
    extern unsigned int mjd_engine_time;
#endif

#ifdef SUPPORT_VG
    extern int dump_job_info(char *str, int chn, int codec_type);
    extern int mjd_dump_property_value(char *str, int chn);
#endif


#ifdef SUPPORT_VG
static int proc_util_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    char str[0x10];
    unsigned long len = count;

    if (len >= sizeof(str))
        len = sizeof(str) - 1;
    if (copy_from_user(str, buffer, len))
        return 0;
    str[len] = '\0';
    sscanf(str, "%d", &mjd_utilization_period);
    return count;
}

static int proc_util_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    *eof = 1;
	if (mjd_utilization_start==0)
        return len;
    len += sprintf(page+len, "JPEG decoder HW Utilization Period=%d(sec) Utilization=%d\n",
        mjd_utilization_period, mjd_utilization_record);
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
    sscanf(str, "%d", &mjd_property_minor);
    return count;
}

static int proc_property_read_mode(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    len += sprintf(page+len, "usage: echo [chn] > /proc/%s/property\n", MJD_PROC_DIR_NAME);
    len += mjd_dump_property_value(page+len, mjd_property_minor);
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
    sscanf(str, "%d", &mjd_job_minor);
    return count;
}


static int proc_job_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len= 0;
    len += sprintf(page+len, "usage: echo [chn] > /proc/%s/job ([chn] = 999: means dump all job)\n", MJD_PROC_DIR_NAME);
    len += sprintf(page+len, "current [chn] = %d\n", mjd_job_minor);
    len += dump_job_info(page+len, mjd_job_minor, TYPE_JPEG_DECODER);
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
    sscanf(str, "%d", &mjd_log_level);
    return count;
}


static int proc_level_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    *eof = 1;
    return sprintf(page, "Log level = %d (%d: error, %d: warning, %d: debug, %d: info)\n", 
            mjd_log_level, LOG_ERROR, LOG_WARNING, LOG_DEBUG, LOG_INFO);
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
void mjd_proc_exit(void)
{
    if (mjd_util_proc != 0)
        remove_proc_entry(mjd_util_proc->name, mjd_proc);
    if (mjd_property_proc != 0)
        remove_proc_entry(mjd_property_proc->name, mjd_proc);
    if (mjd_job_proc != 0)
        remove_proc_entry(mjd_job_proc->name, mjd_proc);
    if (mjd_level_proc != 0)
        remove_proc_entry(mjd_level_proc->name, mjd_proc);
    if (mjd_proc != 0)
        remove_proc_entry(MJD_PROC_DIR_NAME, 0);
}

int mjd_proc_init(void)
{
    mjd_proc = create_proc_entry(MJD_PROC_DIR_NAME, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (mjd_proc == NULL) {
        printk("Error to create driver proc %s\n", MJD_PROC_DIR_NAME);
        goto mjd_init_proc_fail;
    }

    mjd_util_proc = create_proc_entry("utilization", S_IRUGO | S_IXUGO, mjd_proc);
    if (mjd_util_proc == NULL) {
        printk("Error to create driver proc %s/utilization\n", MJD_PROC_DIR_NAME);
        goto mjd_init_proc_fail;
    }
    mjd_util_proc->read_proc = (read_proc_t *)proc_util_read_mode;
    mjd_util_proc->write_proc = (write_proc_t *)proc_util_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    mjd_util_proc->owner = THIS_MODULE;
#endif

    mjd_property_proc = create_proc_entry("property", S_IRUGO | S_IXUGO, mjd_proc);
    if (mjd_property_proc == NULL) {
        printk("Error to create driver proc %s/property\n", MJD_PROC_DIR_NAME);
        goto mjd_init_proc_fail;
    }
    mjd_property_proc->read_proc = (read_proc_t *)proc_property_read_mode;
    mjd_property_proc->write_proc = (write_proc_t *)proc_property_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    mjd_property_proc->owner = THIS_MODULE;
#endif

    mjd_job_proc = create_proc_entry("job", S_IRUGO | S_IXUGO, mjd_proc);
    if (mjd_job_proc == NULL) {
        printk("Error to create driver proc %s/job\n", MJD_PROC_DIR_NAME);
        goto mjd_init_proc_fail;
    }
    mjd_job_proc->read_proc = (read_proc_t *)proc_job_read_mode;
    mjd_job_proc->write_proc = (write_proc_t *)proc_job_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    mjd_job_proc->owner = THIS_MODULE;
#endif

    mjd_level_proc = create_proc_entry("level", S_IRUGO | S_IXUGO, mjd_proc);
    if (mjd_level_proc == NULL) {
        printk("Error to create driver proc %s/level\n", MJD_PROC_DIR_NAME);
        goto mjd_init_proc_fail;
    }
    mjd_level_proc->read_proc = (read_proc_t *)proc_level_read_mode;
    mjd_level_proc->write_proc = (write_proc_t *)proc_level_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    mjd_level_proc->owner = THIS_MODULE;
#endif

    return 0;
mjd_init_proc_fail:
    mjd_proc_exit();
    return -EFAULT;
}


