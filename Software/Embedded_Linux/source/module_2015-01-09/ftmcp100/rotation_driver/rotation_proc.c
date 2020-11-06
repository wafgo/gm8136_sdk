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
#include "rotation_vg2.h"

#ifdef SUPPORT_VG
#define RT_PROC_DIR_NAME   "videograph/rotation"
#else
#define RT_PROC_DIR_NAME   "rotation"
#endif

static struct proc_dir_entry *rt_proc = NULL;
static struct proc_dir_entry *rt_level_proc = NULL;
static struct proc_dir_entry *rt_dbg_proc = NULL;
static struct proc_dir_entry *rt_info_proc = NULL;
#ifdef SUPPORT_VG
static struct proc_dir_entry *rt_util_proc = NULL;
static struct proc_dir_entry *rt_property_proc = NULL;
static struct proc_dir_entry *rt_job_proc = NULL;
#endif

#ifdef SUPPORT_VG
static unsigned int rt_job_minor = 999;
static unsigned int rt_property_minor = 0;
#endif

extern int rt_log_level;
#ifdef SUPPORT_VG
//extern unsigned int callback_period;
extern unsigned int rt_utilization_period; //5sec calculation
extern unsigned int rt_utilization_start, rt_utilization_record;
extern unsigned int rt_engine_start, rt_engine_end;
extern unsigned int rt_engine_time;
#endif

//extern void print_info(void);
extern int rotation_msg(char *str);
#ifdef SUPPORT_VG
extern int dump_job_info(char *str, int minor, int codec_type);
extern int rt_dump_property_value(char *str, int chn);
#endif


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
    sscanf(str, "%d", &rt_utilization_period);
    return count;
}

static int proc_util_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;

    *eof = 1;
    if (rt_utilization_start == 0)
        return len;
    len += sprintf(page+len, "Rotation HW Utilization Period=%d(sec) Utilization=%d\n",
        rt_utilization_period, rt_utilization_record);

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
    
    sscanf(str, "%d", &rt_property_minor);
    return count;
}

static int proc_property_read_mode(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = rt_dump_property_value(page, rt_property_minor);
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
    sscanf(str, "%d", &rt_job_minor);
    return count;
}


static int proc_job_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len;
    len = dump_job_info(page, rt_job_minor, TYPE_JPEG_ENCODER);
    //len = dump_job_info(page, rt_job_minor, TYPE_MCP100_ROTATION);
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
    sscanf(str, "%d", &rt_log_level);
    return count;
}


static int proc_level_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    *eof = 1;
    return sprintf(page, "Log level = %d (%d: error, %d: warning, %d: debug, %d: info)\n", 
            rt_log_level, LOG_ERROR, LOG_WARNING, LOG_DEBUG, LOG_INFO);
}

static int proc_dbg_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    len += sprintf(page+len, "usage:\n");
    //len += sprintf(page+len, " 1: stop mb cnt = %d\n", stop_mb_cnt);
    //len += sprintf(page+len, " 2: stop line cnt = %d\n", stop_line_cnt);
    //printk(" 98: log cnt = %d\n", log_idx);
    //printk(" 99: dump buf log\n");

    *eof = 1;
    return len;
}

static int proc_dbg_write_mode(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = count;
    int cmd_idx, value;    
    char str[80];

    if (len >= sizeof(str))
        len = sizeof(str) - 1;
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

static int proc_info_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    return 0;
}

static int proc_info_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    *eof = 1;

    len = rotation_msg(page);

    return len;
}


void rt_proc_exit(void)
{
    if (rt_info_proc != 0)
        remove_proc_entry(rt_info_proc->name, rt_proc);
    if (rt_dbg_proc != 0)
        remove_proc_entry(rt_dbg_proc->name, rt_proc);
    if (rt_util_proc != 0)
        remove_proc_entry(rt_util_proc->name, rt_proc);
    if (rt_property_proc != 0)
        remove_proc_entry(rt_property_proc->name, rt_proc);
    if (rt_job_proc != 0)
        remove_proc_entry(rt_job_proc->name, rt_proc);
    if (rt_level_proc != 0)
        remove_proc_entry(rt_level_proc->name, rt_proc);
    if (rt_proc != 0)
        remove_proc_entry(RT_PROC_DIR_NAME, 0);
}

int rt_proc_init(void)
{
    rt_proc = create_proc_entry(RT_PROC_DIR_NAME, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (rt_proc == NULL) {
        printk("Error to create driver proc %s\n", RT_PROC_DIR_NAME);
        goto rt_init_proc_fail;
    }

    rt_util_proc = create_proc_entry("utilization", S_IRUGO | S_IXUGO, rt_proc);
    if (rt_util_proc == NULL) {
        printk("Error to create driver proc %s/utilization\n", RT_PROC_DIR_NAME);
        goto rt_init_proc_fail;
    }
    rt_util_proc->read_proc = (read_proc_t *)proc_util_read_mode;
    rt_util_proc->write_proc = (write_proc_t *)proc_util_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    rt_util_proc->owner = THIS_MODULE;
#endif

    rt_property_proc = create_proc_entry("property", S_IRUGO | S_IXUGO, rt_proc);
    if (rt_property_proc == NULL) {
        printk("Error to create driver proc %s/property\n", RT_PROC_DIR_NAME);
        goto rt_init_proc_fail;
    }
    rt_property_proc->read_proc = (read_proc_t *)proc_property_read_mode;
    rt_property_proc->write_proc = (write_proc_t *)proc_property_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    rt_property_proc->owner = THIS_MODULE;
#endif

    rt_job_proc = create_proc_entry("job", S_IRUGO | S_IXUGO, rt_proc);
    if (rt_job_proc == NULL) {
        printk("Error to create driver proc %s/job\n", RT_PROC_DIR_NAME);
        goto rt_init_proc_fail;
    }
    rt_job_proc->read_proc = (read_proc_t *)proc_job_read_mode;
    rt_job_proc->write_proc = (write_proc_t *)proc_job_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    rt_job_proc->owner = THIS_MODULE;
#endif

    rt_level_proc = create_proc_entry("level", S_IRUGO | S_IXUGO, rt_proc);
    if (rt_level_proc == NULL) {
        printk("Error to create driver proc %s/level\n", RT_PROC_DIR_NAME);
        goto rt_init_proc_fail;
    }
    rt_level_proc->read_proc = (read_proc_t *)proc_level_read_mode;
    rt_level_proc->write_proc = (write_proc_t *)proc_level_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    rt_level_proc->owner = THIS_MODULE;
#endif

    rt_dbg_proc = create_proc_entry("dbg", S_IRUGO | S_IXUGO, rt_proc);
    if (rt_dbg_proc == NULL) {
        printk("Error to create driver proc %s/level\n", RT_PROC_DIR_NAME);
        goto rt_init_proc_fail;
    }
    rt_dbg_proc->read_proc = (read_proc_t *)proc_dbg_read_mode;
    rt_dbg_proc->write_proc = (write_proc_t *)proc_dbg_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    rt_dbg_proc->owner = THIS_MODULE;
#endif

    rt_info_proc = create_proc_entry("info", S_IRUGO | S_IXUGO, rt_proc);
    if (rt_info_proc == NULL) {
        printk("Error to create driver proc %s/info\n", RT_PROC_DIR_NAME);
        goto rt_init_proc_fail;
    }
    rt_info_proc->read_proc = (read_proc_t *)proc_info_read_mode;
    rt_info_proc->write_proc = (write_proc_t *)proc_info_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    rt_info_proc->owner = THIS_MODULE;
#endif

    return 0;
rt_init_proc_fail:
    rt_proc_exit();
    return -EFAULT;
}


