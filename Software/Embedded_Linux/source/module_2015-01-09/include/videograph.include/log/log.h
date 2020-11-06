/**
 * @file log.h
 *  The header file of log module to provide log function and panic notification feature
 * Copyright (C) 2011 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.8 $
 * $Date: 2014/03/12 09:25:04 $
 *
 * ChangeLog:
 *  $Log: log.h,v $
 *  Revision 1.8  2014/03/12 09:25:04  schumy_c
 *  add dumplog function.
 *
 *  Revision 1.7  2013/11/21 05:23:23  waynewei
 *  To specify function for un-register.
 *
 *  Revision 1.6  2013/11/21 03:26:09  waynewei
 *  Add un-register function for panic and printout.
 *
 *  Revision 1.5  2013/09/16 07:59:24  foster_h
 *  add master_print function to show message on Master Consloe
 *
 *  Revision 1.4  2013/05/31 05:56:23  ivan
 *  add videograph register to log
 *
 *  Revision 1.3  2013/03/20 08:42:04  ivan
 *  arrange log.h
 *
 *  Revision 1.2  2012/10/12 09:50:29  ivan
 *  take off Carriage Return for linux system
 *
 *  Revision 1.1.1.1  2012/10/12 08:35:52  ivan
 *
 *
 */
#ifndef _LOG_H_
#define _LOG_H_
#include <linux/ioctl.h>
#include <linux/version.h>

#define MAX_STRING_LEN  40

#define IOCTL_PRINTM                0x9969
#define IOCTL_PRINTM_WITH_PANIC     0x9970

int register_panic_notifier(int (*func)(int));
int register_printout_notifier(int (*func)(int));
int register_master_print_notifier(int (*func)(int));
int damnit(char *module);
void printm(char *module,const char *fmt, ...);
void master_print(const char *fmt, ...);
void dumpbuf(unsigned int start,unsigned int size,char *path, char *filename, int dump_mode);   //dump_mode, 0:dump buffer right away,
void register_version(char *);
int unregister_printout_notifier(int (*func)(int));
int unregister_panic_notifier(int (*func)(int));
int dumplog(char *module);

#endif
