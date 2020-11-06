#ifndef __SIMPLE_OSD_H__
#define __SIMPLE_OSD_H__

extern int fsosd_construct(struct lcd200_dev_info *info);
extern void fsosd_deconstruct(struct lcd200_dev_info *info);
extern void fosd_handler(void);

#endif /* __SIMPLE_OSD_H__ */
