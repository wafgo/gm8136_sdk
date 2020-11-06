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
#include "fmpeg4_dec_vg2.h"
#endif

#ifdef REF_POOL_MANAGEMENT
    #include "../mem_pool.h"
#endif

#ifdef SUPPORT_VG
    #define MP4D_PROC_DIR_NAME   "videograph/mp4d"
#else
    #define MP4D_PROC_DIR_NAME   "mp4d"
#endif

static struct proc_dir_entry *mp4d_proc = NULL;
static struct proc_dir_entry *mp4d_level_proc = NULL;
static struct proc_dir_entry *mp4d_info_proc = NULL;
#ifdef SUPPORT_VG
    static struct proc_dir_entry *mp4d_util_proc = NULL;
    static struct proc_dir_entry *mp4d_property_proc = NULL;
    static struct proc_dir_entry *mp4d_job_proc = NULL;
    #ifdef REF_POOL_MANAGEMENT
        static struct proc_dir_entry *mp4d_ref_info_proc = NULL;
    #endif
#endif

#ifdef SUPPORT_VG
static unsigned int mp4d_job_minor = 999;
static unsigned int mp4d_property_minor = 0;
static unsigned int mp4d_ref_info_flag = 0;
#endif

extern int mp4d_log_level;
#ifdef SUPPORT_VG
extern unsigned int mp4d_utilization_period;
extern unsigned int mp4d_utilization_start, mp4d_utilization_record;
extern unsigned int mp4d_engine_start, mp4d_engine_end;
extern unsigned int mp4d_engine_time;
#endif

#ifdef SUPPORT_VG
extern int dump_job_info(char *str, int minor, int codec_type);
extern int mp4d_dump_property_value(char *str, int chn);
#endif
extern int mpeg4_msg(char *str);


#ifdef SUPPORT_VG
static int proc_util_write_mode(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = count;
    char buf[16];

    if (len >= sizeof(buf))
        len = sizeof(buf) - 1;
    if (copy_from_user(buf, buffer, len))
        return 0;
    buf[len] = '\0';

    sscanf(buf, "%d", &mp4d_utilization_period);
    return count;
}

static int proc_util_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    if (mp4d_utilization_start == 0)
        return len;
    len += sprintf(page+len, "MPEG4 decoder HW Utilization Period=%d(sec) Utilization=%d\n",
        mp4d_utilization_period, mp4d_utilization_record);

    *eof = 1;
    return len;
}

static int proc_property_write_mode(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = count;
    char buf[16];

    if (len >= sizeof(buf))
        len = sizeof(buf) - 1;
    if (copy_from_user(buf, buffer, len))
        return 0;
    buf[len] = '\0';
    sscanf(buf, "%d", &mp4d_property_minor);
    
    return count;
}

static int proc_property_read_mode(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    len += sprintf(page+len, "usage: echo [chn] > /proc/%s/property\n", MP4D_PROC_DIR_NAME);
    len += mp4d_dump_property_value(page+len, mp4d_property_minor);
    *eof = 1;
    return len;
}

static int proc_job_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int len = count;
    char buf[16];

    if (len >= sizeof(buf))
        len = sizeof(buf) - 1;
    if (copy_from_user(buf, buffer, len))
        return 0;
    buf[len] = '\0';

    sscanf(buf, "%d", &mp4d_job_minor);
    return count;
}


static int proc_job_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    len += sprintf(page+len, "usage: echo [chn] > /proc/%s/job ([chn] = 999: means dump all job)\n", MP4D_PROC_DIR_NAME);
    len += sprintf(page+len, "current [chn] = %d\n", mp4d_job_minor);
    len += dump_job_info(page+len, mp4d_job_minor, TYPE_MPEG4_DECODER);
    *eof = 1;
    return len;
}
#endif

static int proc_level_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int len = count;
    char buf[16];

    if (len >= sizeof(buf))
        len = sizeof(buf) - 1;
    if (copy_from_user(buf, buffer, len))
        return 0;
    buf[len] = '\0';

    sscanf(buf, "%d", &mp4d_log_level);
    return count;
}


static int proc_level_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    *eof = 1;
    return sprintf(page, "Log level = %d (%d: error, %d: warning, %d: debug, %d: info)\n", 
            mp4d_log_level, LOG_ERROR, LOG_WARNING, LOG_DEBUG, LOG_INFO);
}

#ifdef REF_POOL_MANAGEMENT
static int proc_ref_info_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int len = count;
    char buf[16];

    if (len >= sizeof(buf))
        len = sizeof(buf) - 1;
    if (copy_from_user(buf, buffer, len))
        return 0;
    buf[len] = '\0';

    sscanf(buf, "%d", &mp4d_ref_info_flag);
    return count;
}

static int proc_ref_info_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    *eof = 1;

    len += sprintf(page+len, "dump ref buffer falg = %d (0: dump pool number, 1: dump ref pool, 2: dump chn pool)\n", mp4d_ref_info_flag);
    switch (mp4d_ref_info_flag) {
    case 0:
        len += dec_dump_pool_num(page+len);
        break;
    case 1:
        len += dec_dump_ref_pool(page+len);
        break;
    case 2:
        len += dec_dump_chn_pool(page+len);;
        break;
    default:
        break;
    }

    return len;
}
#endif

static int proc_info_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    return 0;
}

static int proc_info_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    *eof = 1;

    len = mpeg4_msg(page);

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

void mp4d_proc_exit(void)
{
    if (mp4d_info_proc != 0)
        remove_proc_entry(mp4d_info_proc->name, mp4d_proc);
    if (mp4d_ref_info_proc != 0)
        remove_proc_entry(mp4d_ref_info_proc->name, mp4d_proc);
    if (mp4d_util_proc != 0)
        remove_proc_entry(mp4d_util_proc->name, mp4d_proc);
    if (mp4d_property_proc != 0)
        remove_proc_entry(mp4d_property_proc->name, mp4d_proc);
    if (mp4d_job_proc != 0)
        remove_proc_entry(mp4d_job_proc->name, mp4d_proc);
    if (mp4d_level_proc != 0)
        remove_proc_entry(mp4d_level_proc->name, mp4d_proc);
    if (mp4d_proc != 0)
        remove_proc_entry(MP4D_PROC_DIR_NAME, 0);
}

int mp4d_proc_init(void)
{
    mp4d_proc = create_proc_entry(MP4D_PROC_DIR_NAME, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (mp4d_proc == NULL) {
        printk("Error to create driver proc %s\n", MP4D_PROC_DIR_NAME);
        goto mp4d_init_proc_fail;
    }
    mp4d_util_proc = create_proc_entry("utilization", S_IRUGO | S_IXUGO, mp4d_proc);
    if (mp4d_util_proc == NULL) {
        printk("Error to create driver proc %s/utilization\n", MP4D_PROC_DIR_NAME);
        goto mp4d_init_proc_fail;
    }
    mp4d_util_proc->read_proc = (read_proc_t *)proc_util_read_mode;
    mp4d_util_proc->write_proc = (write_proc_t *)proc_util_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    mp4d_util_proc->owner = THIS_MODULE;
#endif

    mp4d_property_proc = create_proc_entry("property", S_IRUGO | S_IXUGO, mp4d_proc);
    if (mp4d_property_proc == NULL) {
        printk("Error to create driver proc %s/property\n", MP4D_PROC_DIR_NAME);
        goto mp4d_init_proc_fail;
    }
    mp4d_property_proc->read_proc = (read_proc_t *)proc_property_read_mode;
    mp4d_property_proc->write_proc = (write_proc_t *)proc_property_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    mp4d_property_proc->owner = THIS_MODULE;
#endif

    mp4d_job_proc = create_proc_entry("job", S_IRUGO | S_IXUGO, mp4d_proc);
    if (mp4d_job_proc == NULL) {
        printk("Error to create driver proc %s/job\n", MP4D_PROC_DIR_NAME);
        goto mp4d_init_proc_fail;
    }
    mp4d_job_proc->read_proc = (read_proc_t *)proc_job_read_mode;
    mp4d_job_proc->write_proc = (write_proc_t *)proc_job_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    mp4d_job_proc->owner = THIS_MODULE;
#endif

    mp4d_level_proc = create_proc_entry("level", S_IRUGO | S_IXUGO, mp4d_proc);
    if (mp4d_level_proc == NULL) {
        printk("Error to create driver proc %s/level\n", MP4D_PROC_DIR_NAME);
        goto mp4d_init_proc_fail;
    }
    mp4d_level_proc->read_proc = (read_proc_t *)proc_level_read_mode;
    mp4d_level_proc->write_proc = (write_proc_t *)proc_level_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    mp4d_level_proc->owner = THIS_MODULE;
#endif

#ifdef REF_POOL_MANAGEMENT
    mp4d_ref_info_proc = create_proc_entry("ref_info", S_IRUGO | S_IXUGO, mp4d_proc);
    if (mp4d_ref_info_proc == NULL) {
        printk("Error to create driver proc %s/ref_info\n", MP4D_PROC_DIR_NAME);
        goto mp4d_init_proc_fail;
    }
    mp4d_ref_info_proc->read_proc = (read_proc_t *)proc_ref_info_read_mode;
    mp4d_ref_info_proc->write_proc = (write_proc_t *)proc_ref_info_write_mode;
    #if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
        mp4d_ref_info_proc->owner = THIS_MODULE;
    #endif
#endif

    mp4d_info_proc = create_proc_entry("info", S_IRUGO | S_IXUGO, mp4d_proc);
    if (mp4d_info_proc == NULL) {
        printk("Error to create driver proc %s/info\n", MP4D_PROC_DIR_NAME);
        goto mp4d_init_proc_fail;
    }
    mp4d_info_proc->read_proc = (read_proc_t *)proc_info_read_mode;
    mp4d_info_proc->write_proc = (write_proc_t *)proc_info_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    mp4d_info_proc->owner = THIS_MODULE;
#endif

    return 0;
mp4d_init_proc_fail:
    mp4d_proc_exit();
    return -EFAULT;
}


