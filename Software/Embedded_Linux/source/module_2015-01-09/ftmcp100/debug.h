#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef DEBUG_MESSAGE

#include <asm/glue.h>

#define MAX_DBG_INDENT_LEVEL	3
#define DBG_INDENT_SIZE		3

#ifndef PFX
#define PFX  ""
#endif

#define DBGPRINT   __glue(MPFX, _dbg_print)
#define DBGINDENT  __glue(MPFX, _dbg_indent)

extern int DBGINDENT;
extern int DBGPRINT(unsigned int level, const char * fmt, ...);

#define DBGENTER(level)	do {				   \
		DBGPRINT(level, "%s: Enter\n", __FUNCTION__); \
		DBGINDENT++; \
	} while (0)

#define DBGLEAVE(level)	do {		\
		DBGINDENT--; \
		DBGPRINT(level, "%s: Leave\n", __FUNCTION__); \
	} while (0)

#define DBGIENTER(level,idx)	do {				   \
		DBGPRINT(level, "%s(%d): Enter\n", __FUNCTION__,idx); \
		DBGINDENT++; \
	} while (0)

#define DBGILEAVE(level,idx)	do {		\
		DBGINDENT--; \
		DBGPRINT(level, "%s(%d): Leave\n", __FUNCTION__,idx); \
	} while (0)

#else				/* DEBUG_MESSAGE */

#define DBGPRINT(level, format, ...)
#define DBGENTER(level)
#define DBGLEAVE(level)

#endif				/* DEBUG_MESSAGE */

extern int dbg_proc_init(void);
extern void dbg_proc_remove(void);

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


#define LOG_EMERGY  0
#define LOG_ERROR   1
#define LOG_WARNING 2
#define LOG_DEBUG   3
#define LOG_INFO    4
#define LOG_DIRECT  0x100

extern int log_level;

#ifdef SUPPORT_VG
    #include "log.h"
    #define MCP100_MODULE_NAME         "MC"
    /*
    #define DEBUG_M(level, fmt, args...) { \
        if (log_level == LOG_DIRECT) \
            printk(fmt, ## args); \
        else if (log_level >= level) \
            printm(MCP100_MODULE_NAME, fmt, ## args); }
    #define DEBUG_E(level, fmt, args...) { \
        if (log_level >= level) \
            printm(MCP100_MODULE_NAME,  fmt, ## args); \
        printk("[error]" fmt, ##args); }
    */

    #define DEBUG_W(level, fmt, args...) { \
        if (log_level >= level) \
            printm(MCP100_MODULE_NAME,  fmt, ## args); \
        printk("[warning]" fmt, ##args); }
    
    #define mcp100_err(fmt, args...) { \
        if (log_level >= LOG_ERROR) \
            printm(MCP100_MODULE_NAME,  fmt, ## args); \
        printk("[mcp100]" fmt, ##args); }
    
    #define mcp100_wrn(fmt, args...) { \
        if (log_level >= LOG_WARNING) \
            printm(MCP100_MODULE_NAME,  fmt, ## args); }
    
    #define mcp100_info(fmt, args...) { \
        if (log_level >= LOG_INFO) \
            printm(MCP100_MODULE_NAME,  fmt, ## args); }
    #if 0
    #define mcp100_debug(fmt, args...) { \
        printk("[mcp100]" fmt, ##args); }
    #else
    #define mcp100_debug(...)
    #endif
#else   // SUPPORT_VG
    /*
    #define DEBUG_M(level, fmt, args...) { \
        if (log_level >= level) \
            printk(fmt, ## args); }
    #define DEBUG_E(level, fmt, args...) { \
        printk("[error]" fmt, ##args); }
    */
    
    #define mcp100_err(fmt, args...) { \
        if (log_level >= LOG_ERROR) \
           printk("[mcp100]" fmt, ##args); }
    
    #define mcp100_wrn(fmt, args...) { \
        if (log_level >= LOG_WARNING) \
            printk(fmt, ## args); }
    
    #define mcp100_info(fmt, args...) { \
        if (log_level >= LOG_INFO) \
            printk(fmt, ## args); }
#endif  // SUPPORT_VG


#endif				/* __DEBUG_H__ */
