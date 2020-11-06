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


//#include "enc_driver/define.h"
#include "fmcp.h"

#ifdef SUPPORT_VG
//#include "fmcp.h"
//#include "favc_enc_entity.h"
#else
//#include "favc_dev.h"
#endif

#ifdef SUPPORT_VG
    #define MCP100_PROC_DIR_NAME    "videograph/mcp100"
#else
    #define MCP100_PROC_DIR_NAME    "mcp100"
#endif

static struct proc_dir_entry *mcp100_entry_proc = NULL;
static struct proc_dir_entry *loglevel_proc = NULL;
static struct proc_dir_entry *info_proc = NULL;
static struct proc_dir_entry *dbg_proc = NULL;
#ifdef SUPPORT_VG
static struct proc_dir_entry *cb_proc = NULL;
//static struct proc_dir_entry *util_proc = NULL;
//static struct proc_dir_entry *property_proc = NULL;
static struct proc_dir_entry *job_proc = NULL;
#endif

#ifdef SUPPORT_VG
    static unsigned int job_minor = 999, job_codec_type = -1;
#endif

extern int log_level;
#ifdef SUPPORT_VG
extern unsigned int callback_period;
extern unsigned int utilization_period;
extern unsigned int utilization_start[MAX_ENGINE_NUM], utilization_record[MAX_ENGINE_NUM];
extern unsigned int property_engine, property_minor;
extern int allocate_cnt;
extern int allocate_ddr_cnt;
#endif

extern int mcp100_msg(char *str);
//extern int print_info(char *str);
#ifdef SUPPORT_VG
    extern int dump_job_info(char *str, int chn, int codec_type);
#endif

static int proc_info_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    return 0;
}
static int proc_info_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len;
    *eof = 1;
    len = mcp100_msg(page);
    //print_info();
    return len;
}
static int proc_log_level_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    *eof = 1;
    return sprintf(page, "Log level = %d (%d: error, %d: warning, %d: debug, %d: info)\n", 
        log_level, LOG_ERROR, LOG_WARNING, LOG_DEBUG, LOG_INFO);;
}

static int proc_log_level_write_mode(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = count;
    char buf[16];

    if (len >= sizeof(buf))
        len = sizeof(buf) - 1;
    if (copy_from_user(buf, buffer, len))
        return 0;
    buf[len] = '\0';
    
    sscanf(buf, "%d", &log_level);
    //log_level = level;
    //printk("\nLog level = %d (%d: error, %d: warning, %d: debug, %d: info)\n", 
    //    log_level, LOG_ERROR, LOG_WARNING, LOG_DEBUG, LOG_INFO);
    return count;
}

#ifdef SUPPORT_VG
static int proc_cb_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int len = count;
    char buf[16];

    if (len >= sizeof(buf))
        len = sizeof(buf) - 1;
    if (copy_from_user(buf, buffer, len))
        return 0;
    buf[len] = '\0';
        
    sscanf(buf, "%d", &callback_period);
    //callback_period = mode_set;
    //printk("Callback Period =%d (msecs)\n", callback_period);
    return count;
}
static int proc_cb_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    *eof = 1;
    return sprintf(page, "Callback Period = %d (msecs)\n", callback_period);
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
        
    sscanf(buf, "%d %d", &job_codec_type, &job_minor);
    //printk("Minor=%d (999 means all), Codec=%d ", job_minor, job_codec_type);
    //printk("(0: MPEG4 encoder, 1: MPEG4 decoder, 2: JPEG encoder, 3: JPEG decoder)\n");
    return count;
}


static int proc_job_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    len += sprintf(page+len, "usage: echo [codec] [chn] > /proc/%s/property\n", MCP100_PROC_DIR_NAME);
    len += sprintf(page+len, "Codec=%d (0: MPEG4 encoder, 1: MPEG4 decoder, 2: JPEG encoder, 3: JPEG decoder), Minor=%d (999 means all)\n", job_codec_type, job_minor);
    len += dump_job_info(page+len, job_minor, job_codec_type);
    *eof = 1;
    return len;
}
#endif

static int proc_dbg_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    //printk("allocate cnt = %d, ddr %d\n", allocate_cnt, allocate_ddr_cnt);
/*
    printk("usage:\n");
    printk(" 97: dump rec buf log = %d\n", dump_rec_buf_log_enable);
    printk(" 98: log cnt = %d\n", log_idx);
    printk(" 99: dump buf log\n");
*/
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

void mcp100_proc_close(void)
{
    if (dbg_proc)
        remove_proc_entry(dbg_proc->name, mcp100_entry_proc);
#ifdef SUPPORT_VG
    if (job_proc)
        remove_proc_entry(job_proc->name, mcp100_entry_proc);
    if (cb_proc)
        remove_proc_entry(cb_proc->name, mcp100_entry_proc);
#endif
    if (info_proc)
        remove_proc_entry(info_proc->name, mcp100_entry_proc);
    if (loglevel_proc)
        remove_proc_entry(loglevel_proc->name, mcp100_entry_proc);
    if (mcp100_entry_proc)
        remove_proc_entry(MCP100_PROC_DIR_NAME, NULL);
}

int mcp100_proc_init(void)
{
    mcp100_entry_proc = create_proc_entry(MCP100_PROC_DIR_NAME, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (NULL == mcp100_entry_proc) {
        printk("Error to create driver proc\n");
        goto fail_init_proc;
    }

    loglevel_proc = create_proc_entry("level", S_IRUGO | S_IXUGO, mcp100_entry_proc);
    if (NULL == loglevel_proc) {
        printk("Error to create level log proc\n");
        goto fail_init_proc;
    }
    loglevel_proc->read_proc = (read_proc_t *)proc_log_level_read_mode;
    loglevel_proc->write_proc = (write_proc_t *)proc_log_level_write_mode;
    //loglevel_proc->owner = THIS_MODULE;

    info_proc = create_proc_entry("info", S_IRUGO | S_IXUGO, mcp100_entry_proc);
    if (NULL == info_proc) {
        printk("Error to create info proc\n");
        goto fail_init_proc;
    }
    info_proc->read_proc = (read_proc_t *)proc_info_read_mode;
    info_proc->write_proc = (write_proc_t *)proc_info_write_mode;
    //info_proc->woner = THIS_MODULE;

#ifdef SUPPORT_VG
    cb_proc = create_proc_entry("callback_period", S_IRUGO | S_IXUGO, mcp100_entry_proc);
    if (cb_proc == NULL) {
        printk("Error to create cb proc\n");
        goto fail_init_proc;
    }
    cb_proc->read_proc = (read_proc_t *)proc_cb_read_mode;
    cb_proc->write_proc = (write_proc_t *)proc_cb_write_mode;

    job_proc = create_proc_entry("job", S_IRUGO | S_IXUGO, mcp100_entry_proc);
    if (job_proc == NULL) {
        printk("Error to create job proc\n");
        goto fail_init_proc;
    }
    job_proc->read_proc = (read_proc_t *)proc_job_read_mode;
    job_proc->write_proc = (write_proc_t *)proc_job_write_mode;
#endif

    dbg_proc = create_proc_entry("dbg", S_IRUGO | S_IXUGO, mcp100_entry_proc);
    if (dbg_proc == NULL) {
        printk("Error to create enc proc\n");
        goto fail_init_proc;
    }
    dbg_proc->read_proc = (read_proc_t *)proc_dbg_read_mode;
    dbg_proc->write_proc = (write_proc_t *)proc_dbg_write_mode;

    return 0;

fail_init_proc:
    mcp100_proc_close();
    return -EFAULT;
}


