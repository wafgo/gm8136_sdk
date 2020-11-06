/**
 * @file vcap_sharp.c
 *  vcap300 Sharpness Control Interface
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.1 $
 * $Date: 2012/12/11 09:28:14 $
 *
 * ChangeLog:
 *  $Log: vcap_sharp.c,v $
 *  Revision 1.1  2012/12/11 09:28:14  jerson_l
 *  1. add sharpness support
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
#include "vcap_sharp.h"

static int sharp_proc_ch[VCAP_DEV_MAX]   = {[0 ... (VCAP_DEV_MAX - 1)] = 0};
static int sharp_proc_path[VCAP_DEV_MAX] = {[0 ... (VCAP_DEV_MAX - 1)] = 0};

static struct proc_dir_entry *vcap_sharp_proc_root[VCAP_DEV_MAX]   = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_sharp_proc_ch[VCAP_DEV_MAX]     = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_sharp_proc_param[VCAP_DEV_MAX]  = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};

void vcap_sharp_init(struct vcap_dev_info_t *pdev_info)
{
    int i, j;
    u32 tmp;

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(!pdev_info->ch[i].active)
            continue;

        for(j=0; j<VCAP_SCALER_MAX; j++) {
            tmp = VCAP_REG_RD(pdev_info, ((i == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*j) : VCAP_CH_DNSD_OFFSET(i, 0x14 + 0x10*j)));
            tmp &= 0x1f;
            tmp |= ((VCAP_SHARP_ADAPTIVE_ENB_DEFAULT<<5)   |
                    (VCAP_SHARP_RADIUS_DEFAULT<<6)         |
                    (VCAP_SHARP_AMOUNT_DEFAULT<<9)         |
                    (VCAP_SHARP_THRED_DEFAULT<<15)         |
                    (VCAP_SHARP_ADAPTIVE_START_DEFAULT<<21)|
                    (VCAP_SHARP_ADAPTIVE_STEP_DEFAULT<<27));
            VCAP_REG_WR(pdev_info, ((i == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*j) : VCAP_CH_DNSD_OFFSET(i, 0x14 + 0x10*j)), tmp);
        }
    }
}

u32 vcap_sharp_get_param(struct vcap_dev_info_t *pdev_info, int ch, int path, VCAP_SHARP_PARAM_T param_id)
{
    u32 value;

    switch(param_id) {
        case VCAP_SHARP_PARAM_ADAPTIVE_ENB:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)));
            value = (value>>5) & 0x1;
            break;
        case VCAP_SHARP_PARAM_RADIUS:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)));
            value = (value>>6) & 0x7;
            break;
        case VCAP_SHARP_PARAM_AMOUNT:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)));
            value = (value>>9) & 0x3f;
            break;
        case VCAP_SHARP_PARAM_THRED:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)));
            value = (value>>15) & 0x3f;
            break;
        case VCAP_SHARP_PARAM_ADAPTIVE_START:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)));
            value = (value>>21) & 0x3f;
            break;
        case VCAP_SHARP_PARAM_ADAPTIVE_STEP:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)));
            value = (value>>27) & 0x1f;
            break;
        default:
            value = 0;
            break;
    }

    return value;
}

int vcap_sharp_set_param(struct vcap_dev_info_t *pdev_info, int ch, int path, VCAP_SHARP_PARAM_T param_id, u32 data)
{
    u32 tmp;
    unsigned long flags;

    spin_lock_irqsave(&pdev_info->lock, flags); ///< use spin lock because temp_param would be access in lli tasklet

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x00) : VCAP_CH_SUBCH_OFFSET(ch, 0x00)));

    if(tmp & (0x2<<(path*0x8))) {  ///< check path active or not?
        switch(param_id) {
            case VCAP_SHARP_PARAM_ADAPTIVE_ENB:
                pdev_info->ch[ch].temp_param.sd[path].sharp.adp = (data & 0x1);     ///< lli tasklet will apply this config and sync to current
                break;
            case VCAP_SHARP_PARAM_RADIUS:
                pdev_info->ch[ch].temp_param.sd[path].sharp.radius = (data & 0x7);
                break;
            case VCAP_SHARP_PARAM_AMOUNT:
                pdev_info->ch[ch].temp_param.sd[path].sharp.amount = (data & 0x3f);
                break;
            case VCAP_SHARP_PARAM_THRED:
                pdev_info->ch[ch].temp_param.sd[path].sharp.thresh = (data & 0x3f);
                break;
            case VCAP_SHARP_PARAM_ADAPTIVE_START:
                pdev_info->ch[ch].temp_param.sd[path].sharp.start = (data & 0x3f);
                break;
            case VCAP_SHARP_PARAM_ADAPTIVE_STEP:
                pdev_info->ch[ch].temp_param.sd[path].sharp.step = (data & 0x1f);
                break;
            default:
                break;
        }
    }
    else {
        switch(param_id) {
            case VCAP_SHARP_PARAM_ADAPTIVE_ENB:
                tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)));
                tmp &= ~BIT5;
                tmp |= (data & 0x1)<<5;
                VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)), tmp);
                pdev_info->ch[ch].param.sd[path].sharp.adp = pdev_info->ch[ch].temp_param.sd[path].sharp.adp = (data & 0x1);
                break;
            case VCAP_SHARP_PARAM_RADIUS:
                tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)));
                tmp &= ~(0x7<<6);
                tmp |= (data & 0x7)<<6;
                VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)), tmp);
                pdev_info->ch[ch].param.sd[path].sharp.radius = pdev_info->ch[ch].temp_param.sd[path].sharp.radius = (data & 0x7);
                break;
            case VCAP_SHARP_PARAM_AMOUNT:
                tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)));
                tmp &= ~(0x3f<<9);
                tmp |= (data & 0x3f)<<9;
                VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)), tmp);
                pdev_info->ch[ch].param.sd[path].sharp.amount = pdev_info->ch[ch].temp_param.sd[path].sharp.amount = (data & 0x3f);
                break;
            case VCAP_SHARP_PARAM_THRED:
                tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)));
                tmp &= ~(0x3f<<15);
                tmp |= (data & 0x3f)<<15;
                VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)), tmp);
                pdev_info->ch[ch].param.sd[path].sharp.thresh = pdev_info->ch[ch].temp_param.sd[path].sharp.thresh = (data & 0x3f);
                break;
            case VCAP_SHARP_PARAM_ADAPTIVE_START:
                tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)));
                tmp &= ~(0x3f<<21);
                tmp |= (data & 0x3f)<<21;
                VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)), tmp);
                pdev_info->ch[ch].param.sd[path].sharp.start = pdev_info->ch[ch].temp_param.sd[path].sharp.start = (data & 0x3f);
                break;
            case VCAP_SHARP_PARAM_ADAPTIVE_STEP:
                tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)));
                tmp &= ~(0x1f<<27);
                tmp |= (data & 0x1f)<<27;
                VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*path) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*path)), tmp);
                pdev_info->ch[ch].param.sd[path].sharp.step = pdev_info->ch[ch].temp_param.sd[path].sharp.step = (data & 0x1f);
                break;
            default:
                break;
        }
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

static ssize_t vcap_sharp_proc_ch_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
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

    if(ch != sharp_proc_ch[pdev_info->index]) {
        sharp_proc_ch[pdev_info->index] = ch;
    }

    if(path != sharp_proc_path[pdev_info->index]) {
        sharp_proc_path[pdev_info->index] = path;
    }

    up(&pdev_info->sema_lock);

    return count;
}

static ssize_t vcap_sharp_proc_param_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
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
        if(sharp_proc_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || sharp_proc_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                down(&pdev_info->ch[i].sema_lock);
                for(j=0; j<VCAP_SCALER_MAX; j++) {
                    if(sharp_proc_path[pdev_info->index] >= VCAP_SCALER_MAX || sharp_proc_path[pdev_info->index] == j) {
                        vcap_sharp_set_param(pdev_info, i, j, param_id, value);
                    }
                }
                up(&pdev_info->ch[i].sema_lock);
            }
        }
    }

    up(&pdev_info->sema_lock);

    return count;
}

static int vcap_sharp_proc_ch_show(struct seq_file *sfile, void *v)
{
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    down(&pdev_info->sema_lock);

    seq_printf(sfile, "Sharpness_Control_CH  : %d (0~%d, other for all)\n", sharp_proc_ch[pdev_info->index],   VCAP_CHANNEL_MAX-1);
    seq_printf(sfile, "Sharpness_Control_PATH: %d (0~%d, other for all)\n", sharp_proc_path[pdev_info->index], VCAP_SCALER_MAX-1);

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_sharp_proc_param_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(sharp_proc_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || sharp_proc_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n=== [CH#%02d] Sharpness Parameter ===\n", i);

                down(&pdev_info->ch[i].sema_lock);
                for(j=0; j<VCAP_SCALER_MAX; j++) {
                    if(sharp_proc_path[pdev_info->index] >= VCAP_SCALER_MAX || sharp_proc_path[pdev_info->index] == j) {
                        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " <Path#%d>\n", j);
                        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]SHARP_ADAPTIVE_ENABLE     : 0x%x\n",
                                               VCAP_SHARP_PARAM_ADAPTIVE_ENB, vcap_sharp_get_param(pdev_info, i, j, VCAP_SHARP_PARAM_ADAPTIVE_ENB));
                        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]SHARP_PARAM_RADIUS        : 0x%x\n",
                                               VCAP_SHARP_PARAM_RADIUS, vcap_sharp_get_param(pdev_info, i, j, VCAP_SHARP_PARAM_RADIUS));
                        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]SHARP_PARAM_AMOUNT        : 0x%x\n",
                                               VCAP_SHARP_PARAM_AMOUNT, vcap_sharp_get_param(pdev_info, i, j, VCAP_SHARP_PARAM_AMOUNT));
                        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]SHARP_PARAM_THRED         : 0x%x\n",
                                               VCAP_SHARP_PARAM_THRED, vcap_sharp_get_param(pdev_info, i, j, VCAP_SHARP_PARAM_THRED));
                        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]SHARP_PARAM_ADAPTIVE_START: 0x%x\n",
                                               VCAP_SHARP_PARAM_ADAPTIVE_START, vcap_sharp_get_param(pdev_info, i, j, VCAP_SHARP_PARAM_ADAPTIVE_START));
                        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]SHARP_PARAM_ADAPTIVE_STEP : 0x%x\n",
                                               VCAP_SHARP_PARAM_ADAPTIVE_STEP, vcap_sharp_get_param(pdev_info, i, j, VCAP_SHARP_PARAM_ADAPTIVE_STEP));
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

static int vcap_sharp_proc_ch_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_sharp_proc_ch_show, PDE(inode)->data);
}

static int vcap_sharp_proc_param_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_sharp_proc_param_show, PDE(inode)->data);
}

static struct file_operations vcap_sharp_proc_ch_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_sharp_proc_ch_open,
    .write  = vcap_sharp_proc_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_sharp_proc_param_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_sharp_proc_param_open,
    .write  = vcap_sharp_proc_param_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

void vcap_sharp_proc_remove(int id)
{
    if(vcap_sharp_proc_root[id]) {
        struct proc_dir_entry *proc_root = (struct proc_dir_entry *)vcap_dev_get_dev_proc_root(id);

        if(vcap_sharp_proc_ch[id])
            vcap_proc_remove_entry(vcap_sharp_proc_root[id], vcap_sharp_proc_ch[id]);

        if(vcap_sharp_proc_param[id])
            vcap_proc_remove_entry(vcap_sharp_proc_root[id], vcap_sharp_proc_param[id]);

        vcap_proc_remove_entry(proc_root, vcap_sharp_proc_root[id]);
    }
}

int vcap_sharp_proc_init(int id, void *private)
{
    int ret = 0;
    struct proc_dir_entry *proc_root = (struct proc_dir_entry *)vcap_dev_get_dev_proc_root(id);

    /* get device proc root */
    if(!proc_root) {
        ret = -EINVAL;
        goto end;
    }

    /* root */
    vcap_sharp_proc_root[id] = vcap_proc_create_entry("sharpness", S_IFDIR|S_IRUGO|S_IXUGO, proc_root);
    if(!vcap_sharp_proc_root[id]) {
        vcap_err("create proc node %s/sharpness failed!\n", proc_root->name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_sharp_proc_root[id]->owner = THIS_MODULE;
#endif

    /* ch */
    vcap_sharp_proc_ch[id] = vcap_proc_create_entry("ch", S_IRUGO|S_IXUGO, vcap_sharp_proc_root[id]);
    if(!vcap_sharp_proc_ch[id]) {
        vcap_err("create proc node 'sharpness/ch' failed!\n");
        ret = -EINVAL;
        goto err;
    }
    vcap_sharp_proc_ch[id]->proc_fops = &vcap_sharp_proc_ch_ops;
    vcap_sharp_proc_ch[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_sharp_proc_ch[id]->owner     = THIS_MODULE;
#endif

    /* param */
    vcap_sharp_proc_param[id] = vcap_proc_create_entry("param", S_IRUGO|S_IXUGO, vcap_sharp_proc_root[id]);
    if(!vcap_sharp_proc_param[id]) {
        vcap_err("create proc node 'sharpness/param' failed!\n");
        ret = -EINVAL;
        goto err;
    }
    vcap_sharp_proc_param[id]->proc_fops = &vcap_sharp_proc_param_ops;
    vcap_sharp_proc_param[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_sharp_proc_param[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    vcap_sharp_proc_remove(id);
    return ret;
}
