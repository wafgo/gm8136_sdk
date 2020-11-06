/**
 * @file vcap_presmo.c
 *  vcap300 Scaler Presmooth Control Interface
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.1 $
 * $Date: 2013/07/16 03:23:45 $
 *
 * ChangeLog:
 *  $Log: vcap_presmo.c,v $
 *  Revision 1.1  2013/07/16 03:23:45  jerson_l
 *  1. support scaler presmooth control
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

#include "bits.h"
#include "vcap_proc.h"
#include "vcap_dev.h"
#include "vcap_dbg.h"
#include "vcap_presmo.h"

static int presmo_proc_ch[VCAP_DEV_MAX]   = {[0 ... (VCAP_DEV_MAX - 1)] = 0};
static int presmo_proc_path[VCAP_DEV_MAX] = {[0 ... (VCAP_DEV_MAX - 1)] = 0};

static struct proc_dir_entry *vcap_presmo_proc_root[VCAP_DEV_MAX]   = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_presmo_proc_ch[VCAP_DEV_MAX]     = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_presmo_proc_param[VCAP_DEV_MAX]  = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};

void vcap_presmo_init(struct vcap_dev_info_t *pdev_info)
{
    int i, j;
    u32 tmp;

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(!pdev_info->ch[i].active)
            continue;

        for(j=0; j<VCAP_SCALER_MAX; j++) {
            tmp = VCAP_REG_RD(pdev_info, ((i == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x0c + 0x10*j) : VCAP_CH_DNSD_OFFSET(i, 0x0c + 0x10*j)));
            tmp &= ~(0x3f<<26);
            VCAP_REG_WR(pdev_info, ((i == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x0c + 0x10*j) : VCAP_CH_DNSD_OFFSET(i, 0x0c + 0x10*j)), tmp);
        }
    }
}

u32 vcap_presmo_get_param(struct vcap_dev_info_t *pdev_info, int ch, int path, VCAP_PRESMO_PARAM_T param_id)
{
    u32 value;

    switch(param_id) {
        case VCAP_PRESMO_PARAM_NONE_AUTO:
            value = pdev_info->ch[ch].param.sd[path].none_auto_smo;
            break;
        case VCAP_PRESMO_PARAM_H_STRENGTH:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x0c + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x0c + 0x10*path)));
            value = (value>>26) & 0x7;
            break;
        case VCAP_PRESMO_PARAM_V_STRENGTH:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x0c + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x0c + 0x10*path)));
            value = (value>>29) & 0x7;
            break;
        default:
            value = 0;
            break;
    }

    return value;
}

int vcap_presmo_set_param(struct vcap_dev_info_t *pdev_info, int ch, int path, VCAP_PRESMO_PARAM_T param_id, u32 data)
{
    u32 tmp;
    unsigned long flags;

    spin_lock_irqsave(&pdev_info->lock, flags); ///< use spin lock because temp_param would be access in lli tasklet

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x00) : VCAP_CH_SUBCH_OFFSET(ch, 0x00)));

    if(tmp & (0x2<<(path*0x8))) {  ///< check path active or not?
        switch(param_id) {
            case VCAP_PRESMO_PARAM_NONE_AUTO:
                pdev_info->ch[ch].param.sd[path].none_auto_smo = pdev_info->ch[ch].temp_param.sd[path].none_auto_smo = data;
                break;
            case VCAP_PRESMO_PARAM_H_STRENGTH:
                if(pdev_info->ch[ch].temp_param.sd[path].none_auto_smo)
                    pdev_info->ch[ch].temp_param.sd[path].hsmo_str = (u8)(data & 0x7);
                break;
            case VCAP_PRESMO_PARAM_V_STRENGTH:
                if(pdev_info->ch[ch].temp_param.sd[path].none_auto_smo)
                    pdev_info->ch[ch].temp_param.sd[path].vsmo_str = (u8)(data & 0x7);
                break;
            default:
                break;
        }
    }
    else {
        switch(param_id) {
            case VCAP_PRESMO_PARAM_NONE_AUTO:
                pdev_info->ch[ch].param.sd[path].none_auto_smo = pdev_info->ch[ch].temp_param.sd[path].none_auto_smo = data;
                break;
            case VCAP_PRESMO_PARAM_H_STRENGTH:
                if(pdev_info->ch[ch].temp_param.sd[path].none_auto_smo) {
                    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x0c + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x0c + 0x10*path)));
                    tmp &= ~(0x7<<26);
                    tmp |= ((data & 0x7)<<26);
                    VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x0c + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x0c + 0x10*path)), tmp);
                    pdev_info->ch[ch].param.sd[path].hsmo_str = pdev_info->ch[ch].temp_param.sd[path].hsmo_str = (data & 0x7);
                }
                break;
            case VCAP_PRESMO_PARAM_V_STRENGTH:
                if(pdev_info->ch[ch].temp_param.sd[path].none_auto_smo) {
                    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x0c + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x0c + 0x10*path)));
                    tmp &= ~(0x7<<29);
                    tmp |= ((data & 0x7)<<29);
                    VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x0c + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x0c + 0x10*path)), tmp);
                    pdev_info->ch[ch].param.sd[path].vsmo_str = pdev_info->ch[ch].temp_param.sd[path].vsmo_str = (data & 0x7);
                }
                break;
            default:
                break;
        }
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

static ssize_t vcap_presmo_proc_ch_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ch, path;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &ch, &path);

    down(&pdev_info->sema_lock);

    if(ch != presmo_proc_ch[pdev_info->index]) {
        presmo_proc_ch[pdev_info->index] = ch;
    }

    if(path != presmo_proc_path[pdev_info->index]) {
        presmo_proc_path[pdev_info->index] = path;
    }

    up(&pdev_info->sema_lock);

    return count;
}

static ssize_t vcap_presmo_proc_param_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, j, param_id;
    u32  value;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &param_id, &value);

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(presmo_proc_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || presmo_proc_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                down(&pdev_info->ch[i].sema_lock);
                for(j=0; j<VCAP_SCALER_MAX; j++) {
                    if(presmo_proc_path[pdev_info->index] >= VCAP_SCALER_MAX || presmo_proc_path[pdev_info->index] == j) {
                        vcap_presmo_set_param(pdev_info, i, j, param_id, value);
                    }
                }
                up(&pdev_info->ch[i].sema_lock);
            }
        }
    }

    up(&pdev_info->sema_lock);

    return count;
}

static int vcap_presmo_proc_ch_show(struct seq_file *sfile, void *v)
{
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    down(&pdev_info->sema_lock);

    seq_printf(sfile, "Presmooth_Control_CH  : %d (0~%d, other for all)\n", presmo_proc_ch[pdev_info->index],   VCAP_CHANNEL_MAX-1);
    seq_printf(sfile, "Presmooth_Control_PATH: %d (0~%d, other for all)\n", presmo_proc_path[pdev_info->index], VCAP_SCALER_MAX-1);

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_presmo_proc_param_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(presmo_proc_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || presmo_proc_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n=== [CH#%02d] Presmooth Parameter ===\n", i);

                down(&pdev_info->ch[i].sema_lock);
                for(j=0; j<VCAP_SCALER_MAX; j++) {
                    if(presmo_proc_path[pdev_info->index] >= VCAP_SCALER_MAX || presmo_proc_path[pdev_info->index] == j) {
                        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " <Path#%d>\n", j);
                        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]VCAP_PRESMO_NONE_AUTO : 0x%x\n",
                                               VCAP_PRESMO_PARAM_NONE_AUTO, vcap_presmo_get_param(pdev_info, i, j, VCAP_PRESMO_PARAM_NONE_AUTO));
                        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]VCAP_PRESMO_H_STRENGTH: 0x%x\n",
                                               VCAP_PRESMO_PARAM_H_STRENGTH, vcap_presmo_get_param(pdev_info, i, j, VCAP_PRESMO_PARAM_H_STRENGTH));
                        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]VCAP_PRESMO_V_STRENGTH: 0x%x\n",
                                               VCAP_PRESMO_PARAM_V_STRENGTH, vcap_presmo_get_param(pdev_info, i, j, VCAP_PRESMO_PARAM_V_STRENGTH));
                    }
                }
                up(&pdev_info->ch[i].sema_lock);

                if(mbuf_point >= VCAP_DEV_MSG_THRESHOLD) {
                    seq_printf(sfile, mbuf);
                    mbuf_point = 0;
                }
            }
        }
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    seq_printf(sfile, mbuf);

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_presmo_proc_ch_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_presmo_proc_ch_show, PDE(inode)->data);
}

static int vcap_presmo_proc_param_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_presmo_proc_param_show, PDE(inode)->data);
}

static struct file_operations vcap_presmo_proc_ch_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_presmo_proc_ch_open,
    .write  = vcap_presmo_proc_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_presmo_proc_param_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_presmo_proc_param_open,
    .write  = vcap_presmo_proc_param_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

void vcap_presmo_proc_remove(int id)
{
    if(vcap_presmo_proc_root[id]) {
        struct proc_dir_entry *proc_root = (struct proc_dir_entry *)vcap_dev_get_dev_proc_root(id);

        if(vcap_presmo_proc_ch[id])
            vcap_proc_remove_entry(vcap_presmo_proc_root[id], vcap_presmo_proc_ch[id]);

        if(vcap_presmo_proc_param[id])
            vcap_proc_remove_entry(vcap_presmo_proc_root[id], vcap_presmo_proc_param[id]);

        vcap_proc_remove_entry(proc_root, vcap_presmo_proc_root[id]);
    }
}

int vcap_presmo_proc_init(int id, void *private)
{
    int ret = 0;
    struct proc_dir_entry *proc_root = (struct proc_dir_entry *)vcap_dev_get_dev_proc_root(id);

    /* get device proc root */
    if(!proc_root) {
        ret = -EINVAL;
        goto end;
    }

    /* root */
    vcap_presmo_proc_root[id] = vcap_proc_create_entry("presmooth", S_IFDIR|S_IRUGO|S_IXUGO, proc_root);
    if(!vcap_presmo_proc_root[id]) {
        vcap_err("create proc node %s/presmooth failed!\n", proc_root->name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_presmo_proc_root[id]->owner = THIS_MODULE;
#endif

    /* ch */
    vcap_presmo_proc_ch[id] = vcap_proc_create_entry("ch", S_IRUGO|S_IXUGO, vcap_presmo_proc_root[id]);
    if(!vcap_presmo_proc_ch[id]) {
        vcap_err("create proc node 'presmooth/ch' failed!\n");
        ret = -EINVAL;
        goto err;
    }
    vcap_presmo_proc_ch[id]->proc_fops = &vcap_presmo_proc_ch_ops;
    vcap_presmo_proc_ch[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_presmo_proc_ch[id]->owner     = THIS_MODULE;
#endif

    /* param */
    vcap_presmo_proc_param[id] = vcap_proc_create_entry("param", S_IRUGO|S_IXUGO, vcap_presmo_proc_root[id]);
    if(!vcap_presmo_proc_param[id]) {
        vcap_err("create proc node 'presmooth/param' failed!\n");
        ret = -EINVAL;
        goto err;
    }
    vcap_presmo_proc_param[id]->proc_fops = &vcap_presmo_proc_param_ops;
    vcap_presmo_proc_param[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_presmo_proc_param[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    vcap_presmo_proc_remove(id);
    return ret;
}

