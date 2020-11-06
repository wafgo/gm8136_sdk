#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>

#ifdef DEBUG_MESSAGE

#include <linux/spinlock.h>
#include "ffb.h"
#include "proc.h"
#include "debug.h"

static int MAX_DBG_MESSAGES = 0;

int ffb_dbg_indent = 0;
EXPORT_SYMBOL(ffb_dbg_indent);
static int dbg_cnt = 0;
DEFINE_SPINLOCK(dbg_spinlock);

#define DBG_OP_LE    0
#define DBG_OP_EQ    1
#define DBG_OP_GE    2

static unsigned char dbg_mode = DBG_OP_LE;
static unsigned char dbg_level = 0;

int ffb_dbg_print(unsigned int level, const char *fmt, ...)
{
    int ret = 0;

    if (!dbg_level)
        goto end;
    switch (dbg_mode) {
    case DBG_OP_EQ:
        if (level == dbg_level) {
            if (!MAX_DBG_MESSAGES || dbg_cnt < MAX_DBG_MESSAGES) {
                int ind = ffb_dbg_indent;
                unsigned long flags;
                va_list args;

                spin_lock_irqsave(&dbg_spinlock, flags);
                dbg_cnt++;
                if (ind > MAX_DBG_INDENT_LEVEL)
                    ind = MAX_DBG_INDENT_LEVEL;
                printk("%*s", ind * DBG_INDENT_SIZE, "");
                va_start(args, fmt);
                ret = vprintk(fmt, args);
                va_end(args);
                spin_unlock_irqrestore(&dbg_spinlock, flags);
            }
        }
        break;
    case DBG_OP_GE:
        if (level >= dbg_level) {
            if (!MAX_DBG_MESSAGES || dbg_cnt < MAX_DBG_MESSAGES) {
                int ind = ffb_dbg_indent;
                unsigned long flags;
                va_list args;

                spin_lock_irqsave(&dbg_spinlock, flags);
                dbg_cnt++;
                if (ind > MAX_DBG_INDENT_LEVEL)
                    ind = MAX_DBG_INDENT_LEVEL;
                printk("%*s", ind * DBG_INDENT_SIZE, "");
                va_start(args, fmt);
                ret = vprintk(fmt, args);
                va_end(args);
                spin_unlock_irqrestore(&dbg_spinlock, flags);
            }
        }
        break;
    case DBG_OP_LE:
    default:
        if (level <= dbg_level) {
            if (!MAX_DBG_MESSAGES || dbg_cnt < MAX_DBG_MESSAGES) {
                int ind = ffb_dbg_indent;
                unsigned long flags;
                va_list args;

                spin_lock_irqsave(&dbg_spinlock, flags);
                dbg_cnt++;
                if (ind > MAX_DBG_INDENT_LEVEL)
                    ind = MAX_DBG_INDENT_LEVEL;
                printk("%*s", ind * DBG_INDENT_SIZE, "");
                va_start(args, fmt);
                ret = vprintk(fmt, args);
                va_end(args);
                spin_unlock_irqrestore(&dbg_spinlock, flags);
            }
        }
        break;
    }
  end:
    return ret;
}

EXPORT_SYMBOL(ffb_dbg_print);

static int proc_read_mode(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    len +=
        sprintf(page, "Debug Mode: 0: Little than Level; 1: Equal to Level; 2: Great than Level\n");
    len += sprintf(page + len, "current mode=%d\n", dbg_mode);
    *eof = 1;                   //end of file
    *start = page + off;
    len = len - off;
    return len;
}

static int proc_write_mode(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = count;
    unsigned char value[20];
    uint tmp;

    if (copy_from_user(value, buffer, len))
        return 0;
    value[len] = '\0';

    sscanf(value, "%u\n", &tmp);

    if (tmp < 3) {
        dbg_mode = tmp;
    }

    return count;
}

static int proc_read_level(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    len += sprintf(page, "Debug Level: 0: Disable; 1 ~ 8: Print debug message\n");
    len += sprintf(page + len, "current level=%d\n", dbg_level);
    *eof = 1;                   //end of file
    *start = page + off;
    len = len - off;
    return len;
}

static int proc_write_level(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = count;
    unsigned char value[20];
    uint tmp;

    if (copy_from_user(value, buffer, len))
        return 0;
    value[len] = '\0';

    sscanf(value, "%u\n", &tmp);

    if (tmp < 9) {
        dbg_level = tmp;
    }

    return count;
}

static int proc_read_msgcount(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    len += sprintf(page, "Debug max message: 0: Infinite\n");
    len += sprintf(page + len, "current value=%d\n", MAX_DBG_MESSAGES);
    *eof = 1;                   //end of file
    *start = page + off;
    len = len - off;
    return len;
}

static int proc_write_msgcount(struct file *file, const char *buffer,
                               unsigned long count, void *data)
{
    int len = count;
    unsigned char value[20];
    uint tmp;

    if (copy_from_user(value, buffer, len))
        return 0;
    value[len] = '\0';

    sscanf(value, "%u\n", &tmp);

    MAX_DBG_MESSAGES = tmp;
    dbg_cnt = 0;

    return count;
}

static struct proc_dir_entry *mode = NULL;
static struct proc_dir_entry *level = NULL;
static struct proc_dir_entry *msg_count = NULL;

int dbg_proc_init(struct proc_dir_entry *parent)
{
    int ret = 0;

    mode = ffb_create_proc_entry("dbg_mode", S_IRUGO | S_IXUGO, parent);

    if (mode == NULL) {
        err("Fail to create proc mode!\n");
        ret = -EINVAL;
        goto fail;
    }
    mode->read_proc = (read_proc_t *) proc_read_mode;
    mode->write_proc = (write_proc_t *) proc_write_mode;

    level = ffb_create_proc_entry("dbg_level", S_IRUGO | S_IXUGO, parent);

    if (level == NULL) {
        err("Fail to create proc level!\n");
        ret = -EINVAL;
        goto fail;
    }
    level->read_proc = (read_proc_t *) proc_read_level;
    level->write_proc = (write_proc_t *) proc_write_level;
    
    msg_count = ffb_create_proc_entry("dbg_msgcnt", S_IRUGO | S_IXUGO, parent);
    if (msg_count == NULL) {
        err("Fail to create proc level!\n");
        ret = -EINVAL;
        goto fail;
    }
    msg_count->read_proc = (read_proc_t *) proc_read_msgcount;
    msg_count->write_proc = (write_proc_t *) proc_write_msgcount;
    
    return ret;
  fail:
    dbg_proc_remove(parent);
    return ret;
}

void dbg_proc_remove(struct proc_dir_entry *parent)
{
    if (mode != NULL)
        ffb_remove_proc_entry(parent, mode);
    if (level != NULL)
        ffb_remove_proc_entry(parent, level);
    if (msg_count != NULL)
        ffb_remove_proc_entry(parent, msg_count);
}

#else /* DEBUG_MESSAGE */

int dbg_proc_init(struct proc_dir_entry *parent)
{
    return 0;
}

void dbg_proc_remove(struct proc_dir_entry *parent)
{
}

#endif /* DEBUG_MESSAGE */
