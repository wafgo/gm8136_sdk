/**
 * @file vcap_input.c
 *  vcap300 input module interface
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.18 $
 * $Date: 2014/10/30 01:47:32 $
 *
 * ChangeLog:
 *  $Log: vcap_input.c,v $
 *  Revision 1.18  2014/10/30 01:47:32  jerson_l
 *  1. support vi 2ch dual edge hybrid channel mode.
 *  2. support grab channel horizontal blanking data.
 *
 *  Revision 1.17  2014/05/21 03:17:40  jerson_l
 *  1. support tvi input module type
 *
 *  Revision 1.16  2014/04/21 04:13:27  jerson_l
 *  1. modify version and table read proc node from seq_read to read_proc to prevent kernel bug
 *
 *  Revision 1.15  2014/03/25 06:59:08  jerson_l
 *  1. add input_module version display proc node
 *
 *  Revision 1.14  2014/02/06 04:03:36  jerson_l
 *  1. support CVI and AHD input type
 *
 *  Revision 1.13  2014/02/06 02:11:47  jerson_l
 *  1. support get channel input status api
 *
 *  Revision 1.12  2014/01/24 04:11:15  jerson_l
 *  1. rename NOTIFY_VG_HW_CONFIG_CHANGE to NOTIFY_HW_CONFIG_CHANGE
 *
 *  Revision 1.11  2013/11/20 08:57:14  jerson_l
 *  1. add input device status notify api
 *
 *  Revision 1.10  2013/11/11 05:37:54  jerson_l
 *  1. add SDI input module type
 *
 *  Revision 1.9  2013/11/07 06:39:58  jerson_l
 *  1. support max_frame_rate infomation
 *
 *  Revision 1.8  2013/10/14 04:02:51  jerson_l
 *  1. support GM8139 platform
 *
 *  Revision 1.7  2013/09/06 07:57:49  jerson_l
 *  1. fix interface display incorrect problem in input_module table
 *
 *  Revision 1.6  2013/02/20 08:58:01  jerson_l
 *  1. modify input_module table display layout
 *
 *  Revision 1.5  2013/01/15 09:40:42  jerson_l
 *  1. add vi_src and data_swap for capture port pinmux control
 *
 *  Revision 1.4  2012/12/11 09:13:08  jerson_l
 *  1. use semaphore to protect critical section of input interface
 *
 *  Revision 1.3  2012/10/25 10:59:40  jerson_l
 *  add interface field to input_module display table
 *
 *  Revision 1.2  2012/10/24 06:47:51  jerson_l
 *
 *  switch file to unix format
 *
 *  Revision 1.1.1.1  2012/10/23 08:16:31  jerson_l
 *  import vcap300 driver
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/semaphore.h>

#include "vcap_plat.h"
#include "vcap_input.h"
#include "vcap_proc.h"
#include "vcap_dbg.h"
#include "vcap_vg.h"

/*************************************************************************************
 *  VCAP Input Information Structure
 *************************************************************************************/
struct vcap_input_info_t {
    struct vcap_input_dev_t *dev[VCAP_INPUT_DEV_MAX];
    struct semaphore        sema_lock;
};

static struct vcap_input_info_t *vcap_input_info         = NULL;
static struct proc_dir_entry    *vcap_input_proc_root    = NULL;
static struct proc_dir_entry    *vcap_input_proc_version = NULL;
static struct proc_dir_entry    *vcap_input_proc_table   = NULL;

struct proc_dir_entry *vcap_input_proc_create_entry(const char *name, mode_t mode, struct proc_dir_entry *parent)
{
    struct proc_dir_entry *pentry;
    struct proc_dir_entry *root;

    root = (parent == NULL) ? vcap_input_proc_root : parent;

    pentry = create_proc_entry(name, mode, root);

    return pentry;
}
EXPORT_SYMBOL(vcap_input_proc_create_entry);

void vcap_input_proc_remove_entry(struct proc_dir_entry *parent, struct proc_dir_entry *entry)
{
    struct proc_dir_entry *root;

    root = (parent == NULL) ? vcap_input_proc_root : parent;

    if(entry)
        remove_proc_entry(entry->name, root);
}
EXPORT_SYMBOL(vcap_input_proc_remove_entry);

static int vcap_input_proc_table_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int i;
    int len = 0;
    struct vcap_input_info_t *pinfo = (struct vcap_input_info_t *)data;
    struct vcap_input_dev_t  *pdev;
    char *type_str[]  = {"generic", "decoder", "sensor", "isp", "sdi", "cvi", "ahd", "tvi"};
    char *intf_str[]  = {"interlace", "progressive",    ///< bt656
                         "interlace", "progressive",    ///< bt1120
                         "rgb888",                      ///< rgb888
                         "interlace", "progressive",    ///< sdi8bit
                         "interlace", "progressive",    ///< bt601_8bit
                         "interlace", "progressive",    ///< bt601_16bit
                         "isp"                          ///< isp
                        };
    char *visrc_str[] = {"XCAP0", "XCAP1", "XCAP2", "XCAP3", "XCAP4", "XCAP5", "XCAP6", "XCAP7",
                         "XCAPCAS", "LCD0", "LCD1", "LCD2", "ISP"};

    if(off > 0)
        goto end;

    down(&pinfo->sema_lock);

    len += sprintf(page+len, "|%-6s %-16s %-10s %-14s %-13s %-7s %-7s %s\n", "VI#", "Name", "Type", "Interface", "Resolution", "FPS_C", "FPS_M", "XCAP#");
    len += sprintf(page+len, "|=======================================================================================\n");

    for(i=0; i<VCAP_INPUT_DEV_MAX; i++) {
        pdev = pinfo->dev[i];
        if(pdev) {
            len += sprintf(page+len, " %-6d %-16s %-10s %-14s %4dx%-8d %-7d %-7d %-7s\n",
                           pdev->vi_idx,
                           pdev->name,
                           type_str[pdev->type],
                           intf_str[pdev->interface],
                           pdev->norm.width,
                           pdev->norm.height,
                           pdev->frame_rate,
                           pdev->max_frame_rate,
                           visrc_str[pdev->vi_src]);
        }
    }

    *eof = 1;

    up(&pinfo->sema_lock);

end:
    return len;
}

static int vcap_input_proc_version_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;

    if(off > 0)
        goto end;

    len += sprintf(page+len, "Version: %d.%d.%d\n", INPUT_MAJOR_VER, INPUT_MINOR_VER, INPUT_BETA_VER);
    *eof = 1;

end:
    return len;
}

static int vcap_input_proc_init(struct vcap_input_info_t *pinfo)
{
    int ret = 0;

    /* input module root */
    vcap_input_proc_root = vcap_proc_create_entry("input_module", S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!vcap_input_proc_root) {
        vcap_err("create proc node 'input_module' failed!\n");
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_input_proc_root->owner = THIS_MODULE;
#endif

    /* version */
    vcap_input_proc_version = vcap_proc_create_entry("version", S_IRUGO|S_IXUGO, vcap_input_proc_root);
    if(!vcap_input_proc_version) {
        vcap_err("create proc node 'input_module/version' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    vcap_input_proc_version->read_proc = vcap_input_proc_version_read;
    vcap_input_proc_version->data      = (void *)pinfo;

    /* input module table */
    vcap_input_proc_table = vcap_proc_create_entry("table", S_IRUGO|S_IXUGO, vcap_input_proc_root);
    if(!vcap_input_proc_table) {
        vcap_err("create proc node 'input_module/table' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    vcap_input_proc_table->read_proc = vcap_input_proc_table_read;
    vcap_input_proc_table->data      = (void *)pinfo;

end:
    return ret;

err_tab:
    vcap_proc_remove_entry(NULL, vcap_input_proc_root);
    return ret;
}

static void vcap_input_proc_remove(void)
{
    if(vcap_input_proc_root) {
        if(vcap_input_proc_table)
            vcap_proc_remove_entry(vcap_input_proc_root, vcap_input_proc_table);

        if(vcap_input_proc_version)
            vcap_proc_remove_entry(vcap_input_proc_root, vcap_input_proc_version);

        vcap_proc_remove_entry(NULL, vcap_input_proc_root);
    }
}

int vcap_input_interface_register(void)
{
    int ret = 0;

    vcap_input_info = kzalloc(sizeof(struct vcap_input_info_t), GFP_KERNEL);
    if(vcap_input_info == NULL) {
        vcap_err("allocate vcap_input_info failed!\n");
        ret = -ENOMEM;
        goto end;
    }

    /* init semaphore lock */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
    sema_init(&vcap_input_info->sema_lock, 1);
#else
    init_MUTEX(&vcap_input_info->sema_lock);
#endif

    ret = vcap_input_proc_init(vcap_input_info);
    if(ret < 0)
        goto err_proc;

end:
    return ret;

err_proc:
    kfree(vcap_input_info);
    return ret;
}

void vcap_input_interface_unregister(void)
{
    vcap_input_proc_remove();

    if(vcap_input_info) {
        int i;

        for(i=0; i<VCAP_INPUT_DEV_MAX; i++) {
            if(vcap_input_info->dev[i]) {
                kfree(vcap_input_info->dev[i]);
                vcap_input_info->dev[i] = NULL;
            }
        }

        kfree(vcap_input_info);

        vcap_input_info = NULL;
    }
}

int vcap_input_param_check(struct vcap_input_dev_t *dev)
{
    if(dev->vi_idx >= VCAP_INPUT_DEV_MAX) {
        vcap_err("register input device failed!...vi index out of range\n");
        return -1;
    }

    if(dev->vi_src >= VCAP_INPUT_VI_SRC_MAX) {
        vcap_err("register input device failed!...vi_src invalid\n");
        return -1;
    }

    if(dev->interface >= VCAP_INPUT_INTF_MAX) {
        vcap_err("register input device failed!...interface invalid\n");
        return -1;
    }

    if(dev->mode >= VCAP_INPUT_MODE_MAX) {
        vcap_err("register input device failed!...mode invalid\n");
        return -1;
    }

    if(dev->field_order >= VCAP_INPUT_ORDER_MAX) {
        vcap_err("register input device failed!...field order invalid\n");
        return -1;
    }

    if(dev->data_range >= VCAP_INPUT_DATA_RANGE_MAX) {
        vcap_err("register input device failed!...data range invalid\n");
        return -1;
    }

    if(dev->data_swap >= VCAP_INPUT_DATA_SWAP_MAX) {
        vcap_err("register input device failed!...data swap invalid\n");
        return -1;
    }

    if(dev->yc_swap >= VCAP_INPUT_SWAP_MAX) {
        vcap_err("register input device failed!...yc swap invalid\n");
        return -1;
    }

    if(dev->ch_reorder >= VCAP_INPUT_CHAN_REORDER_MAX) {
        vcap_err("register input device failed!...ch reorder invalid\n");
        return -1;
    }

    if(dev->ch_id_mode >= VCAP_INPUT_CH_ID_MAX) {
        vcap_err("register input device failed!...ch id mode invalid\n");
        return -1;
    }

    if(dev->probe_chid >= VCAP_INPUT_PROBE_CHID_MAX) {
        vcap_err("register input device failed!...probe_chid mode invalid\n");
        return -1;
    }

    if(dev->init == NULL) {
        vcap_err("register input device failed!...not register initial function\n");
        return -1;
    }

    return 0;
}

struct vcap_input_dev_t *vcap_input_get_info(int dev_id)
{
    return vcap_input_info->dev[dev_id];
}

int vcap_input_device_register(struct vcap_input_dev_t *dev)
{
    int ret = 0;

    if(vcap_input_param_check(dev) < 0)
        return -1;

    down(&vcap_input_info->sema_lock);

    if(!vcap_input_info->dev[dev->vi_idx]) {
        struct vcap_plat_cap_port_cfg_t cfg;

        /* vi capture port config */
        cfg.cap_src = dev->vi_src;
        cfg.inv_clk = dev->inv_clk;

        switch(dev->data_swap) {
            case VCAP_INPUT_DATA_SWAP_LO8BIT:
                cfg.bit_swap  = 1;
                cfg.byte_swap = 0;
                break;
            case VCAP_INPUT_DATA_SWAP_BYTE:
                cfg.bit_swap  = 0;
                cfg.byte_swap = 1;
                break;
            case VCAP_INPUT_DATA_SWAP_LO8BIT_BYTE:
                cfg.bit_swap  = 1;
                cfg.byte_swap = 1;
                break;
            case VCAP_INPUT_DATA_SWAP_HI8BIT:
                cfg.bit_swap  = 2;
                cfg.byte_swap = 0;
                break;
            case VCAP_INPUT_DATA_SWAP_LOHI8BIT:
                cfg.bit_swap  = 3;
                cfg.byte_swap = 0;
                break;
            case VCAP_INPUT_DATA_SWAP_HI8BIT_BYTE:
                cfg.bit_swap  = 2;
                cfg.byte_swap = 1;
                break;
            case VCAP_INPUT_DATA_SWAP_LOHI8BIT_BYTE:
                cfg.bit_swap  = 3;
                cfg.byte_swap = 1;
                break;
            default:
                cfg.bit_swap  = 0;
                cfg.byte_swap = 0;
                break;
        }

        switch(dev->interface) {
            case VCAP_INPUT_INTF_BT1120_INTERLACE:
            case VCAP_INPUT_INTF_BT1120_PROGRESSIVE:
                cfg.cap_fmt = VCAP_PLAT_VI_CAP_FMT_BT1120;
                break;
            case VCAP_INPUT_INTF_RGB888:
                cfg.cap_fmt = VCAP_PLAT_VI_CAP_FMT_RGB888;
                break;
            case VCAP_INPUT_INTF_BT601_8BIT_INTERLACE:
            case VCAP_INPUT_INTF_BT601_8BIT_PROGRESSIVE:
                cfg.cap_fmt = VCAP_PLAT_VI_CAP_FMT_BT601_8BIT;
                break;
            case VCAP_INPUT_INTF_BT601_16BIT_INTERLACE:
            case VCAP_INPUT_INTF_BT601_16BIT_PROGRESSIVE:
                cfg.cap_fmt = VCAP_PLAT_VI_CAP_FMT_BT601_16BIT;
                break;
            case VCAP_INPUT_INTF_ISP:
                cfg.cap_fmt = VCAP_PLAT_VI_CAP_FMT_ISP;
                break;
            default:
                cfg.cap_fmt = VCAP_PLAT_VI_CAP_FMT_BT656;
                break;
        }
        ret = vcap_plat_vi_capture_port_config(dev->vi_idx, &cfg);
        if(ret < 0)
            goto exit;

        dev->attached = -1; ///< init value

        if(dev->probe_chid)
            dev->chid_ready = 0; ///< init value

        vcap_input_info->dev[dev->vi_idx] = dev;
    }
    else {
        vcap_err("register input device failed!...vi source have registered as other device\n");
        ret = -1;
    }

exit:
    up(&vcap_input_info->sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_input_device_register);

void vcap_input_device_unregister(struct vcap_input_dev_t *dev)
{
    if(dev->vi_idx >= VCAP_INPUT_DEV_MAX) {
        vcap_err("invalid input device\n");
        return;
    }

    down(&vcap_input_info->sema_lock);

    if(vcap_input_info->dev[dev->vi_idx] == dev)
        vcap_input_info->dev[dev->vi_idx] = NULL;

    up(&vcap_input_info->sema_lock);
}
EXPORT_SYMBOL(vcap_input_device_unregister);

u32 vcap_input_get_version(void)
{
    return VCAP_INPUT_VERSION;
}
EXPORT_SYMBOL(vcap_input_get_version);

int vcap_input_device_notify(int id, int ch, VCAP_INPUT_NOTIFY_T n_type)
{
    int ret = 0;
    int vg_dev_id = 0;  ///< ToDo, should be get real device id

    if((id >= VCAP_INPUT_DEV_MAX) || (ch >= VCAP_INPUT_DEV_CH_MAX))
        return -1;

    down(&vcap_input_info->sema_lock);

    if(vcap_input_info->dev[id]) {
        switch(n_type) {
            case VCAP_INPUT_NOTIFY_SIGNAL_LOSS:
                vcap_vg_notify(vg_dev_id, ((id*VCAP_INPUT_DEV_CH_MAX)+ch), -1, VCAP_VG_NOTIFY_SIGNAL_LOSS);    ///< path=-1 means notify to first active path
                break;
            case VCAP_INPUT_NOTIFY_SIGNAL_PRESENT:
                vcap_vg_notify(vg_dev_id, ((id*VCAP_INPUT_DEV_CH_MAX)+ch), -1, VCAP_VG_NOTIFY_SIGNAL_PRESENT);
                break;
            case VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE:
                vcap_vg_notify(vg_dev_id, ((id*VCAP_INPUT_DEV_CH_MAX)+ch), -1, VCAP_VG_NOTIFY_FRAMERATE_CHANGE);
                break;
            case VCAP_INPUT_NOTIFY_NORM_CHANGE:
                vcap_vg_notify(vg_dev_id, ((id*VCAP_INPUT_DEV_CH_MAX)+ch), -1, VCAP_VG_NOTIFY_HW_CONFIG_CHANGE);
                break;
            default:
                ret = -1;
                goto exit;
        }
    }

exit:
    up(&vcap_input_info->sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_input_device_notify);

int vcap_input_device_get_status(struct vcap_input_dev_t *dev, int ch, struct vcap_input_status_t *status)
{
    int ret = 0;

    if(ch >= VCAP_INPUT_DEV_CH_MAX || !status)
        return -1;

    down(&dev->sema_lock);

    status->vlos   = dev->ch_vlos[ch];
    status->width  = dev->norm.width;
    status->height = dev->norm.height;
    status->fps    = dev->frame_rate;

    up(&dev->sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_input_device_get_status);
