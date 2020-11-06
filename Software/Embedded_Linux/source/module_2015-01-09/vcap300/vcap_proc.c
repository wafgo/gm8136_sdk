/**
 * @file vcap_proc.c
 *  vcap300 proc interface
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.2 $
 * $Date: 2012/10/24 06:47:51 $
 *
 * ChangeLog:
 *  $Log: vcap_proc.c,v $
 *  Revision 1.2  2012/10/24 06:47:51  jerson_l
 *
 *  switch file to unix format
 *
 *  Revision 1.1.1.1  2012/10/23 08:16:31  jerson_l
 *  import vcap300 driver
 *
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/proc_fs.h>

static struct proc_dir_entry *vcap_proc_root = NULL;

struct proc_dir_entry *vcap_proc_create_entry(const char *name, mode_t mode, struct proc_dir_entry *parent)
{
    struct proc_dir_entry *pentry;
    struct proc_dir_entry *root;

    root = (parent == NULL) ? vcap_proc_root : parent;

    pentry = create_proc_entry(name, mode, root);

    return pentry;
}
EXPORT_SYMBOL(vcap_proc_create_entry);

void vcap_proc_remove_entry(struct proc_dir_entry *parent, struct proc_dir_entry *entry)
{
    struct proc_dir_entry *root;

    root = (parent == NULL) ? vcap_proc_root : parent;

    if(entry)
        remove_proc_entry(entry->name, root);
}
EXPORT_SYMBOL(vcap_proc_remove_entry);

int vcap_proc_register(const char *name)
{
    int ret = 0;
    struct proc_dir_entry *pentry;

    pentry = create_proc_entry(name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);

    if(!pentry) {
        ret = -ENOMEM;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    pentry->owner  = THIS_MODULE;
#endif
    vcap_proc_root = pentry;

end:
    return ret;
}

void vcap_proc_unregister(void)
{
    if(vcap_proc_root) {
        remove_proc_entry(vcap_proc_root->name, NULL);
    }
}
