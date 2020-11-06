#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef FTSSP_DEBUG_MESSAGE

#define MAX_DBG_INDENT_LEVEL	3
#define DBG_INDENT_SIZE		3

extern int ftssp_dbg_indent;
extern int ftssp_dbg_print(unsigned int level, const char *fmt, ...);

#define DBGPRINT   ftssp_dbg_print

#define DBGENTER(level)	do { \
		ftssp_dbg_print(level, "%s: Enter\n", __FUNCTION__); \
		ftssp_dbg_indent++; \
	} while (0)

#define DBGLEAVE(level)	do { \
		ftssp_dbg_indent--; \
		ftssp_dbg_print(level, "%s: Leave\n", __FUNCTION__); \
	} while (0)

#else /* FTSSP_DEBUG_MESSAGE */

#define DBGPRINT(level, format, ...)
#define DBGENTER(level)
#define DBGLEAVE(level)

#endif /* FTSSP_DEBUG_MESSAGE */

extern int dbg_proc_init(void);
extern void dbg_proc_remove(void);

#define err(format, arg...)    printk(KERN_ERR PFX "(%s): " format "\n" , __FUNCTION__, ## arg)
#define warn(format, arg...)   printk(KERN_WARNING PFX "(%s): " format "\n" , __FUNCTION__, ## arg)
#define info(format, arg...)   printk(KERN_INFO PFX ": " format "\n" , ## arg)
#define infos(format, arg...)  printk(KERN_INFO PFX ": " format , ## arg)
#define infom(format, arg...)  printk(format , ## arg)
#define infoe(format, arg...)  printk(format "\n" , ## arg)
#define notice(format, arg...) printk(KERN_NOTICE PFX ": " format "\n" , ## arg)

#endif /* __DEBUG_H__ */
