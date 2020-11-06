#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef DEBUG_MESSAGE

#define MAX_DBG_INDENT_LEVEL	3
#define DBG_INDENT_SIZE		3

extern int ffb_dbg_indent;
extern int ffb_dbg_print(unsigned int level, const char *fmt, ...);

#define DBGPRINT   ffb_dbg_print

#define DBGENTER(level)	do {				   \
		ffb_dbg_print(level, "%s: Enter\n", __FUNCTION__); \
		ffb_dbg_indent++; \
	} while (0)

#define DBGLEAVE(level)	do {		\
		ffb_dbg_indent--; \
		ffb_dbg_print(level, "%s: Leave\n", __FUNCTION__); \
	} while (0)

#define DBGIENTER(level,idx)	do {				   \
		ffb_dbg_print(level, "%s(%d): Enter\n", __FUNCTION__,idx); \
		ffb_dbg_indent++; \
	} while (0)

#define DBGILEAVE(level,idx)	do {		\
		ffb_dbg_indent--; \
		ffb_dbg_print(level, "%s(%d): Leave\n", __FUNCTION__,idx); \
	} while (0)

#else /* DEBUG_MESSAGE */

#define DBGPRINT(level, format, ...)
#define DBGENTER(level)
#define DBGLEAVE(level)

#endif /* DEBUG_MESSAGE */

extern int dbg_proc_init(struct proc_dir_entry *p);
extern void dbg_proc_remove(struct proc_dir_entry *p);

#define err(format, arg...)    printk(KERN_ERR PFX "(%s): " format "\n" , __FUNCTION__, ## arg)
#define errs(format, arg...)   printk(KERN_ERR PFX "(%s): " format  , __FUNCTION__, ## arg)
#define errm(format, arg...)   printk(format , ## arg)
#define erre(format, arg...)   printk(format "\n" , ## arg)
#define warn(format, arg...)   printk(KERN_WARNING PFX "(%s): " format "\n" , __FUNCTION__, ## arg)
#define info(format, arg...)   printk(KERN_INFO PFX ": " format "\n" , ## arg)
#define infos(format, arg...)  printk(KERN_INFO PFX ": " format , ## arg)
#define infom(format, arg...)  printk(format , ## arg)
#define infoe(format, arg...)  printk(format "\n" , ## arg)
#define notice(format, arg...) printk(KERN_NOTICE PFX ": " format "\n" , ## arg)

#endif /* __DEBUG_H__ */
