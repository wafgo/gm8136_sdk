#ifndef _VCAP_PROC_H_
#define _VCAP_PROC_H_

#include <linux/proc_fs.h>

int  vcap_proc_register(const char *name);
void vcap_proc_unregister(void);
void vcap_proc_remove_entry(struct proc_dir_entry *parent, struct proc_dir_entry *entry);
struct proc_dir_entry *vcap_proc_create_entry(const char *name, mode_t mode, struct proc_dir_entry *parent);

#endif  /* _VCAP_PROC_H_ */
