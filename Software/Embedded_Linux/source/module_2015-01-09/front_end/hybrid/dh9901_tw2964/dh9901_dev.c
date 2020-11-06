/*
 * @file dh9901_tw2964_drv.c
 * DaHua 4CH HDCVI Receiver Driver (for hybrid design)
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.2 $
 * $Date: 2014/10/22 09:14:29 $
 *
 * ChangeLog:
 *  $Log: dh9901_dev.c,v $
 *  Revision 1.2  2014/10/22 09:14:29  jerson_l
 *  1. modify DH9901_VOUT_FORMAT_BT656 to DH9901_VOUT_FORMAT_BT656_DUAL_HEADER
 *
 *  Revision 1.1  2014/08/05 09:32:30  shawn_hu
 *  Add hybrid (HDCVI/D1) frontend driver design (DH9901/TW2964).
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <mach/ftpmu010.h>

#include "platform.h"
#include "dahua/dh9901.h"              ///< from module/include/front_end/hdcvi/dahua
#include "dh9901_lib.h"                ///< from module/front_end/hdcvi/dahua/dh9901_lib
#include "dh9901_dev.h"
#include "dh9901_tw2964_hybrid_drv.h"

/*************************************************************************************
 *  Initial values of the following structures come from dh9901_tw2964_hybrid.c
 *************************************************************************************/
static int dev_num;
static ushort iaddr[DH9901_DEV_MAX];
static int vout_format[DH9901_DEV_MAX];
static int sample_rate;
static struct dh9901_dev_t* dh9901_dev;
static int init;
static struct dh9901_video_fmt_t* dh9901_tw2964_video_fmt_info;
static u32 DH9901_TW2964_VCH_MAP[DH9901_DEV_MAX*DH9901_DEV_CH_MAX];

/*************************************************************************************
 *  Original structure
 *************************************************************************************/
static int dh9901_api_inited  = 0;

static struct proc_dir_entry *dh9901_proc_root[DH9901_DEV_MAX]        = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_status[DH9901_DEV_MAX]      = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_video_fmt[DH9901_DEV_MAX]   = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_video_color[DH9901_DEV_MAX] = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_video_pos[DH9901_DEV_MAX]   = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_cable_type[DH9901_DEV_MAX]  = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_clear_eq[DH9901_DEV_MAX]    = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_ptz_root[DH9901_DEV_MAX]    = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_ptz_cfg[DH9901_DEV_MAX]     = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_ptz_ctrl[DH9901_DEV_MAX]    = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};

/*************************************************************************************
 *  Revised for hybrid design
 *************************************************************************************/
int dh9901_i2c_write(u8 addr, u8 reg, u8 data)
{
    return dh9901_tw2964_i2c_write(addr, reg, data);
}
EXPORT_SYMBOL(dh9901_i2c_write);

u8 dh9901_i2c_read(u8 addr, u8 reg)
{
    return dh9901_tw2964_i2c_read(addr, reg);
}
EXPORT_SYMBOL(dh9901_i2c_read);

void dh9901_set_params(dh9901_params_p params_p)
{
    int i = 0;

    dev_num = params_p->dh9901_dev_num;
    for(i=0; i<dev_num; i++) {
        iaddr[i] = *(params_p->dh9901_iaddr + i);
    }
    for(i=0; i<DH9901_DEV_MAX; i++) {
        vout_format[i] = *(params_p->dh9901_vout_format + i);
    }
    sample_rate = params_p->dh9901_sample_rate;
    dh9901_dev = params_p->dh9901_dev;
    init = params_p->init;
    dh9901_tw2964_video_fmt_info = params_p->dh9901_tw2964_video_fmt_info;
    for(i=0; i<DH9901_DEV_MAX*DH9901_DEV_CH_MAX; i++) {
        DH9901_TW2964_VCH_MAP[i] = *(params_p->DH9901_TW2964_VCH_MAP + i);
    }
}   

/*************************************************************************************
 *  Keep as original design (only XXXX_VCH_MAP was updated)
 *************************************************************************************/
int dh9901_get_vch_id(int id, DH9901_DEV_VOUT_T vout, int cvi_ch)
{
    int i;

    if(id >= dev_num || vout >= DH9901_DEV_VOUT_MAX || cvi_ch >= DH9901_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*DH9901_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(DH9901_TW2964_VCH_MAP[i]) == id) &&
           (CH_IDENTIFY_VOUT(DH9901_TW2964_VCH_MAP[i]) == vout) &&
           (CH_IDENTIFY_VIN(DH9901_TW2964_VCH_MAP[i]) == cvi_ch)) {
            return i;
        }
    }

    return 0;
}
EXPORT_SYMBOL(dh9901_get_vch_id);

int dh9901_get_vout_format(int id)
{
    if(id >= dev_num)
        return DH9901_VOUT_FORMAT_BT656_DUAL_HEADER;
    else
        return vout_format[id];
}
EXPORT_SYMBOL(dh9901_get_vout_format);

int dh9901_notify_vfmt_register(int id, int (*nt_func)(int, int, struct dh9901_video_fmt_t *))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&dh9901_dev[id].lock);

    dh9901_dev[id].vfmt_notify = nt_func;

    up(&dh9901_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(dh9901_notify_vfmt_register);

void dh9901_notify_vfmt_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&dh9901_dev[id].lock);

    dh9901_dev[id].vfmt_notify = NULL;

    up(&dh9901_dev[id].lock);
}
EXPORT_SYMBOL(dh9901_notify_vfmt_deregister);

int dh9901_notify_vlos_register(int id, int (*nt_func)(int, int, int))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&dh9901_dev[id].lock);

    dh9901_dev[id].vlos_notify = nt_func;

    up(&dh9901_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(dh9901_notify_vlos_register);

void dh9901_notify_vlos_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&dh9901_dev[id].lock);

    dh9901_dev[id].vlos_notify = NULL;

    up(&dh9901_dev[id].lock);
}
EXPORT_SYMBOL(dh9901_notify_vlos_deregister);

static int dh9901_proc_status_show(struct seq_file *sfile, void *v)
{
    int i, j, ret;
    int vlos;
    DH_VIDEO_STATUS_S cvi_sts;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9901#%d]\n", pdev->index);
    seq_printf(sfile, "------------------\n");
    seq_printf(sfile, "CVI    VCH    LOS \n");
    seq_printf(sfile, "==================\n");
    for(i=0; i<DH9901_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*DH9901_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(DH9901_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(DH9901_TW2964_VCH_MAP[j]) == i)) {
                ret = DH9901_API_GetVideoStatus(pdev->index, i, &cvi_sts);
                vlos = (ret == 0) ? cvi_sts.ucVideoLost : 0;
                seq_printf(sfile, "%-6d %-6d %-7s\n", i, j, ((vlos == 0) ? "NO" : "YES"));
            }
        }
    }

    up(&pdev->lock);

    return 0;
}

static int dh9901_proc_video_fmt_show(struct seq_file *sfile, void *v)
{
    int i, j, ret;
    DH_VIDEO_STATUS_S cvi_sts;
    struct dh9901_video_fmt_t *p_vfmt;

    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9901#%d] Incoming Video Fromat\n", pdev->index);
    seq_printf(sfile, "====================================================\n");
    seq_printf(sfile, "CVI   VCH   Width   Height   Prog/Inter   Frame_Rate\n");
    seq_printf(sfile, "====================================================\n");

    for(i=0; i<DH9901_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*DH9901_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(DH9901_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(DH9901_TW2964_VCH_MAP[j]) == i)) {
                ret = DH9901_API_GetVideoStatus(pdev->index, i, &cvi_sts);
                if((ret == 0) && ((cvi_sts.ucVideoFormat >= DH9901_VFMT_720P25) && (cvi_sts.ucVideoFormat < DH9901_VFMT_MAX))) {
                    p_vfmt = &dh9901_tw2964_video_fmt_info[cvi_sts.ucVideoFormat];
                    seq_printf(sfile, "%-5d %-5d %-7d %-8d %-12s %-11d\n",
                               i,
                               j,
                               p_vfmt->width,
                               p_vfmt->height,
                               ((p_vfmt->prog == 1) ? "Progressive" : "Interlace"),
                               p_vfmt->frame_rate);
                }
                else {
                    seq_printf(sfile, "%-5d %-5d %-7s %-8s %-12s %-11s\n", i, j, "-", "-", "-", "-");
                }
            }
        }
    }

    up(&pdev->lock);

    return 0;
}

static int dh9901_proc_video_color_show(struct seq_file *sfile, void *v)
{
    int i, j, ret;
    DH_VIDEO_COLOR_S v_color;

    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9901#%d] Video Color\n", pdev->index);
    seq_printf(sfile, "=======================================================\n");
    seq_printf(sfile, "CVI   VCH   BRI   CON   SAT   HUE   GAIN   W/B   SHARP \n");
    seq_printf(sfile, "=======================================================\n");

    for(i=0; i<DH9901_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*DH9901_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(DH9901_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(DH9901_TW2964_VCH_MAP[j]) == i)) {
                ret = DH9901_API_GetColor(pdev->index, i, &v_color, DH_SET_MODE_DEFAULT);
                if(ret == 0) {
                    seq_printf(sfile, "%-5d %-5d %-5d %-5d %-5d %-5d %-6d %-5d %-5d\n",
                               i,
                               j,
                               v_color.ucBrightness,
                               v_color.ucContrast,
                               v_color.ucSaturation,
                               v_color.ucHue,
                               v_color.ucGain,
                               v_color.ucWhiteBalance,
                               v_color.ucSharpness);
                }
                else {
                    seq_printf(sfile, "%-5d %-5d %-5d %-5d %-5d %-5d %-6d %-5d %-5d\n", i, j, 0, 0, 0, 0, 0, 0, 0);
                }
            }
        }
    }
    seq_printf(sfile, "=======================================================\n");
    seq_printf(sfile, "[0]Brightness  : 0 ~ 100\n");
    seq_printf(sfile, "[1]Contrast    : 0 ~ 100\n");
    seq_printf(sfile, "[2]Saturation  : 0 ~ 100\n");
    seq_printf(sfile, "[3]Hue         : 0 ~ 100\n");
    seq_printf(sfile, "[4]Gain        : Not support\n");
    seq_printf(sfile, "[5]WhiteBalance: Not support\n");
    seq_printf(sfile, "[6]Sharpness   : 0 ~ 15\n");
    seq_printf(sfile, "=======================================================\n");
    seq_printf(sfile, "echo [CVI#] [PARAM#] [VALUE] for parameter setup\n\n");

    up(&pdev->lock);

    return 0;
}

static int dh9901_proc_video_pos_show(struct seq_file *sfile, void *v)
{
    int i, j, ret;
    DH_VIDEO_POSITION_S vpos;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9901#%d] Video Position\n", pdev->index);
    seq_printf(sfile, "----------------------------------\n");
    seq_printf(sfile, "CVI    VCH    H_Offset    V_Offset\n");
    seq_printf(sfile, "==================================\n");
    for(i=0; i<DH9901_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*DH9901_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(DH9901_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(DH9901_TW2964_VCH_MAP[j]) == i)) {
                ret = DH9901_API_GetVideoPosition(pdev->index, i, &vpos);
                if(ret == 0)
                    seq_printf(sfile, "%-6d %-6d %-11d %-11d\n", i, j, vpos.sHorizonOffset, vpos.sVerticalOffset);
                else
                    seq_printf(sfile, "%-6d %-6d %-11d %-11d\n", i, j, 0, 0);
            }
        }
    }
    seq_printf(sfile, "==================================\n");
    seq_printf(sfile, "echo [CVI#] [H_Offset] [V_Offset] for video position setup\n\n");

    up(&pdev->lock);

    return 0;
}

static int dh9901_proc_cable_type_show(struct seq_file *sfile, void *v)
{
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9901#%d] Cable Type\n", pdev->index);
    seq_printf(sfile, "========================================\n");
    seq_printf(sfile, "[0]COAXIAL\n");
    seq_printf(sfile, "[1]UTP_10OHM\n");
    seq_printf(sfile, "[2]UTP_17OHM\n");
    seq_printf(sfile, "[3]UTP_25OHM\n");
    seq_printf(sfile, "[4]UTP_35OHM\n");
    seq_printf(sfile, "========================================\n");
    seq_printf(sfile, "echo [CVI#] [TYPE#] for cable type setup\n\n");

    up(&pdev->lock);

    return 0;
}

static int dh9901_proc_clear_eq_show(struct seq_file *sfile, void *v)
{
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9901#%d] Clear EQ\n", pdev->index);
    seq_printf(sfile, "========================================\n");
    seq_printf(sfile, "echo [CVI#] for trigger channel EQ clear\n\n");

    up(&pdev->lock);

    return 0;
}

static int dh9901_proc_ptz_cfg_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;
    char *ptz_prot_str[] = {"DH_SD1"};
    char *ptz_baud_str[] = {"1200", "2400", "4800", "9600", "19200", "38400"};

    down(&pdev->lock);

    seq_printf(sfile, "[DH9901#%d] PTZ Configuration\n", pdev->index);
    seq_printf(sfile, "---------------------------------------------\n");
    seq_printf(sfile, "CVI   VCH   PROTOCOL   BAUD_RATE   PARITY_CHK\n");
    seq_printf(sfile, "=============================================\n");
    for(i=0; i<DH9901_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*DH9901_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(DH9901_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(DH9901_TW2964_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-5d %-5d %-10s %-11s %-10s\n", i, j,
                           (pdev->ptz[i].protocol < DH9901_PTZ_PTOTOCOL_MAX) ? ptz_prot_str[pdev->ptz[i].protocol] : "Unknown",
                           (pdev->ptz[i].baud_rate < DH9901_PTZ_BAUD_MAX) ? ptz_baud_str[pdev->ptz[i].baud_rate] : "Unknown",
                           (pdev->ptz[i].parity_chk == 0) ? "No" : "Yes");
            }
        }
    }

    up(&pdev->lock);

    return 0;
}

static int dh9901_proc_ptz_ctrl_show(struct seq_file *sfile, void *v)
{
    int i;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9901#%d]\n", pdev->index);

    for(i=0; i<DH9901_PTZ_PTOTOCOL_MAX; i++) {
        switch(i) {
            case DH9901_PTZ_PTOTOCOL_DHSD1:
                seq_printf(sfile, "<< DH_SD1 Command List >>\n");
                seq_printf(sfile, "=========================\n");
                seq_printf(sfile, "[0]Menu_CLOSE\n");
                seq_printf(sfile, "[1]Menu_OPEN\n");
                seq_printf(sfile, "[2]Menu_BACK\n");
                seq_printf(sfile, "[2]Menu_NEXT\n");
                seq_printf(sfile, "[4]Menu_UP\n");
                seq_printf(sfile, "[5]Menu_DOWN\n");
                seq_printf(sfile, "[6]Menu_LEFT\n");
                seq_printf(sfile, "[7]Menu_RIGHT\n");
                seq_printf(sfile, "[8]Menu_ENTER\n");
                seq_printf(sfile, "=========================\n");
                seq_printf(sfile, "echo [CVI#] [CAMERA#(0~255)] [CMD#] for remoet camrea control\n\n");
                break;
            default:
                break;
        }
    }

    up(&pdev->lock);

    return 0;
}

static int dh9901_proc_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_status_show, PDE(inode)->data);
}

static int dh9901_proc_video_fmt_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_video_fmt_show, PDE(inode)->data);
}

static int dh9901_proc_video_color_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_video_color_show, PDE(inode)->data);
}

static int dh9901_proc_video_pos_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_video_pos_show, PDE(inode)->data);
}

static int dh9901_proc_cable_type_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_cable_type_show, PDE(inode)->data);
}

static int dh9901_proc_clear_eq_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_clear_eq_show, PDE(inode)->data);
}

static int dh9901_proc_ptz_cfg_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_ptz_cfg_show, PDE(inode)->data);
}

static int dh9901_proc_ptz_ctrl_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_ptz_ctrl_show, PDE(inode)->data);
}

static ssize_t dh9901_proc_video_color_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int ret, cvi_ch, param_id, param_value;
    DH_VIDEO_COLOR_S video_color;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d %d\n", &cvi_ch, &param_id, &param_value);

    down(&pdev->lock);

    if((cvi_ch >= DH9901_DEV_CH_MAX) || (param_id >= 7))
        goto exit;

    ret = DH9901_API_GetColor(pdev->index, cvi_ch, &video_color, DH_SET_MODE_USER);
    if(ret != 0)
        goto exit;

    switch(param_id) {
        case 0:
            video_color.ucBrightness = (DH_U8)param_value;
            break;
        case 1:
            video_color.ucContrast = (DH_U8)param_value;
            break;
        case 2:
            video_color.ucSaturation = (DH_U8)param_value;
            break;
        case 3:
            video_color.ucHue = (DH_U8)param_value;
            break;
        case 4:
            video_color.ucGain = (DH_U8)param_value;
            break;
        case 5:
            video_color.ucWhiteBalance = (DH_U8)param_value;
            break;
        case 6:
            video_color.ucSharpness = (DH_U8)param_value;
            break;
        default:
            break;
    }

    DH9901_API_SetColor(pdev->index, cvi_ch, &video_color, DH_SET_MODE_USER);

exit:
    up(&pdev->lock);

    return count;
}

static ssize_t dh9901_proc_video_pos_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int ret, cvi_ch, h_offset, v_offset;
    DH_VIDEO_POSITION_S video_pos;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d %d\n", &cvi_ch, &h_offset, &v_offset);

    down(&pdev->lock);

    if(cvi_ch >= DH9901_DEV_CH_MAX)
        goto exit;

    video_pos.sHorizonOffset  = h_offset;
    video_pos.sVerticalOffset = v_offset;

    ret = DH9901_API_SetVideoPosition(pdev->index, cvi_ch, &video_pos);
    if(ret != 0)
        goto exit;

exit:
    up(&pdev->lock);

    return count;
}

static ssize_t dh9901_proc_cable_type_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int ret, cvi_ch, cable_type;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &cvi_ch, &cable_type);

    down(&pdev->lock);

    if(cvi_ch >= DH9901_DEV_CH_MAX)
        goto exit;

    ret = DH9901_API_SetCableType(pdev->index, cvi_ch, cable_type);
    if(ret != 0)
        goto exit;

exit:
    up(&pdev->lock);

    return count;
}

static ssize_t dh9901_proc_clear_eq_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int ret, cvi_ch;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &cvi_ch);

    down(&pdev->lock);

    if(cvi_ch >= DH9901_DEV_CH_MAX)
        goto exit;

    ret = DH9901_API_ClearEq(pdev->index, cvi_ch);
    if(ret != 0)
        goto exit;

exit:
    up(&pdev->lock);

    return count;
}

static ssize_t dh9901_proc_ptz_ctrl_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int cvi_ch, cam_id, cmd, ret;
    char rs485_cmd[64];
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d %d\n", &cvi_ch, &cam_id, &cmd);

    down(&pdev->lock);

    if(cvi_ch >= DH9901_DEV_CH_MAX)
        goto exit;

    switch(pdev->ptz[cvi_ch].protocol) {
        case DH9901_PTZ_PTOTOCOL_DHSD1:
            /*
             * Prepare DH_SD1 PTZ RS485 Command
             * [SYNC CAM_ID CMD1 DATA1 DATA2 DATA3 CRC]
             */
            rs485_cmd[0] = 0xa5;
            rs485_cmd[1] = (u8)cam_id;
            rs485_cmd[2] = 0x89;
            rs485_cmd[3] = (u8)cmd;
            rs485_cmd[4] = 0x00;
            rs485_cmd[5] = 0x00;
            rs485_cmd[6] = (rs485_cmd[0] + rs485_cmd[1] + rs485_cmd[2] + rs485_cmd[3] + rs485_cmd[4] + rs485_cmd[5])%0x100;

            ret = DH9901_API_Contrl485Enable(pdev->index, cvi_ch, 1);
            if(ret != 0)
                goto exit;

            ret = DH9901_API_Send485Buffer(pdev->index, cvi_ch, rs485_cmd, 7);
            if(ret != 0)
                goto exit;

            ret = DH9901_API_Contrl485Enable(pdev->index, cvi_ch, 0);
            if(ret != 0)
                goto exit;
            break;
        default:
            break;
    }

exit:
    up(&pdev->lock);

    return count;
}

static struct file_operations dh9901_proc_status_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_status_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_video_fmt_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_video_fmt_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_video_color_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_video_color_open,
    .write  = dh9901_proc_video_color_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_video_pos_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_video_pos_open,
    .write  = dh9901_proc_video_pos_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_cable_type_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_cable_type_open,
    .write  = dh9901_proc_cable_type_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_clear_eq_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_clear_eq_open,
    .write  = dh9901_proc_clear_eq_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_ptz_cfg_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_ptz_cfg_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_ptz_ctrl_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_ptz_ctrl_open,
    .write  = dh9901_proc_ptz_ctrl_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

void dh9901_proc_remove(int id)
{
    if(id >= DH9901_DEV_MAX)
        return;

    if(dh9901_proc_root[id]) {
        if(dh9901_proc_ptz_root[id]) {
            if(dh9901_proc_ptz_cfg[id]) {
                remove_proc_entry(dh9901_proc_ptz_cfg[id]->name, dh9901_proc_ptz_root[id]);
                dh9901_proc_ptz_cfg[id] = NULL;
            }

            if(dh9901_proc_ptz_ctrl[id]) {
                remove_proc_entry(dh9901_proc_ptz_ctrl[id]->name, dh9901_proc_ptz_root[id]);
                dh9901_proc_ptz_ctrl[id] = NULL;
            }

            remove_proc_entry(dh9901_proc_ptz_root[id]->name, dh9901_proc_root[id]);
            dh9901_proc_ptz_root[id] = NULL;
        }

        if(dh9901_proc_status[id]) {
            remove_proc_entry(dh9901_proc_status[id]->name, dh9901_proc_root[id]);
            dh9901_proc_status[id] = NULL;
        }

        if(dh9901_proc_video_fmt[id]) {
            remove_proc_entry(dh9901_proc_video_fmt[id]->name, dh9901_proc_root[id]);
            dh9901_proc_video_fmt[id] = NULL;
        }

        if(dh9901_proc_video_color[id]) {
            remove_proc_entry(dh9901_proc_video_color[id]->name, dh9901_proc_root[id]);
            dh9901_proc_video_color[id] = NULL;
        }

        if(dh9901_proc_video_pos[id]) {
            remove_proc_entry(dh9901_proc_video_pos[id]->name, dh9901_proc_root[id]);
            dh9901_proc_video_pos[id] = NULL;
        }

        if(dh9901_proc_cable_type[id]) {
            remove_proc_entry(dh9901_proc_cable_type[id]->name, dh9901_proc_root[id]);
            dh9901_proc_cable_type[id] = NULL;
        }

        if(dh9901_proc_clear_eq[id]) {
            remove_proc_entry(dh9901_proc_clear_eq[id]->name, dh9901_proc_root[id]);
            dh9901_proc_clear_eq[id] = NULL;
        }

        remove_proc_entry(dh9901_proc_root[id]->name, NULL);
        dh9901_proc_root[id] = NULL;
    }
}

int dh9901_proc_init(int id)
{
    int ret = 0;

    if(id >= DH9901_DEV_MAX)
        return -1;

    /* root */
    dh9901_proc_root[id] = create_proc_entry(dh9901_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!dh9901_proc_root[id]) {
        printk("create proc node '%s' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_root[id]->owner = THIS_MODULE;
#endif

    /* status */
    dh9901_proc_status[id] = create_proc_entry("status", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_status[id]) {
        printk("create proc node '%s/status' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_status[id]->proc_fops = &dh9901_proc_status_ops;
    dh9901_proc_status[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_status[id]->owner     = THIS_MODULE;
#endif

    /* frame format */
    dh9901_proc_video_fmt[id] = create_proc_entry("video_format", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_video_fmt[id]) {
        printk("create proc node '%s/video_format' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_video_fmt[id]->proc_fops = &dh9901_proc_video_fmt_ops;
    dh9901_proc_video_fmt[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_video_fmt[id]->owner     = THIS_MODULE;
#endif

    /* video color */
    dh9901_proc_video_color[id] = create_proc_entry("video_color", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_video_color[id]) {
        printk("create proc node '%s/video_color' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_video_color[id]->proc_fops = &dh9901_proc_video_color_ops;
    dh9901_proc_video_color[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_video_color[id]->owner     = THIS_MODULE;
#endif

    /* video position */
    dh9901_proc_video_pos[id] = create_proc_entry("video_pos", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_video_pos[id]) {
        printk("create proc node '%s/video_pos' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_video_pos[id]->proc_fops = &dh9901_proc_video_pos_ops;
    dh9901_proc_video_pos[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_video_pos[id]->owner     = THIS_MODULE;
#endif

    /* cable type */
    dh9901_proc_cable_type[id] = create_proc_entry("cable_type", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_cable_type[id]) {
        printk("create proc node '%s/cable_type' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_cable_type[id]->proc_fops = &dh9901_proc_cable_type_ops;
    dh9901_proc_cable_type[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_cable_type[id]->owner     = THIS_MODULE;
#endif

    /* clear EQ */
    dh9901_proc_clear_eq[id] = create_proc_entry("clear_eq", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_clear_eq[id]) {
        printk("create proc node '%s/clear_eq' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_clear_eq[id]->proc_fops = &dh9901_proc_clear_eq_ops;
    dh9901_proc_clear_eq[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_clear_eq[id]->owner     = THIS_MODULE;
#endif

    /* ptz root */
    dh9901_proc_ptz_root[id] = create_proc_entry("ptz", S_IFDIR|S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_ptz_root[id]) {
        printk("create proc node '%s/ptz' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_ptz_root[id]->owner = THIS_MODULE;
#endif

    /* ptz config */
    dh9901_proc_ptz_cfg[id] = create_proc_entry("cfg", S_IRUGO|S_IXUGO, dh9901_proc_ptz_root[id]);
    if(!dh9901_proc_ptz_cfg[id]) {
        printk("create proc node '%s/ptz/cfg' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_ptz_cfg[id]->proc_fops = &dh9901_proc_ptz_cfg_ops;
    dh9901_proc_ptz_cfg[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_ptz_cfg[id]->owner     = THIS_MODULE;
#endif

    /* ptz control */
    dh9901_proc_ptz_ctrl[id] = create_proc_entry("control", S_IRUGO|S_IXUGO, dh9901_proc_ptz_root[id]);
    if(!dh9901_proc_ptz_ctrl[id]) {
        printk("create proc node '%s/ptz/control' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_ptz_ctrl[id]->proc_fops = &dh9901_proc_ptz_ctrl_ops;
    dh9901_proc_ptz_ctrl[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_ptz_ctrl[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    dh9901_proc_remove(id);
    return ret;
}

static int dh9901_miscdev_open(struct inode *inode, struct file *file)
{
    int i, ret = 0;
    struct dh9901_dev_t *pdev = NULL;

    /* lookup device */
    for(i=0; i<dev_num; i++) {
        if(dh9901_dev[i].miscdev.minor == iminor(inode)) {
            pdev = &dh9901_dev[i];
            break;
        }
    }

    if(!pdev) {
        ret = -EINVAL;
        goto exit;
    }

    file->private_data = (void *)pdev;

exit:
    return ret;
}

static int dh9901_miscdev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long dh9901_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)file->private_data;

    down(&pdev->lock);

    if(_IOC_TYPE(cmd) != DH9901_IOC_MAGIC) {
        ret = -ENOIOCTLCMD;
        goto exit;
    }

    switch(cmd) {
        case DH9901_GET_NOVID:
            {
                struct dh9901_ioc_data_t ioc_data;
                DH_VIDEO_STATUS_S cvi_sts;

                if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_data.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                ret = DH9901_API_GetVideoStatus(pdev->index, ioc_data.cvi_ch, &cvi_sts);
                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }

                ioc_data.data = cvi_sts.ucVideoLost;

                ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            }
            break;

        case DH9901_GET_VIDEO_FMT:
            {
                DH_VIDEO_STATUS_S cvi_sts;
                struct dh9901_ioc_vfmt_t ioc_vfmt;

                if(copy_from_user(&ioc_vfmt, (void __user *)arg, sizeof(ioc_vfmt))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vfmt.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                ret = DH9901_API_GetVideoStatus(pdev->index, ioc_vfmt.cvi_ch, &cvi_sts);
                if((ret != 0) || (cvi_sts.ucVideoFormat >= DH9901_VFMT_MAX)) {
                    ret = -EFAULT;
                    goto exit;
                }

                ioc_vfmt.width      = dh9901_tw2964_video_fmt_info[cvi_sts.ucVideoFormat].width;
                ioc_vfmt.height     = dh9901_tw2964_video_fmt_info[cvi_sts.ucVideoFormat].height;
                ioc_vfmt.prog       = dh9901_tw2964_video_fmt_info[cvi_sts.ucVideoFormat].prog;
                ioc_vfmt.frame_rate = dh9901_tw2964_video_fmt_info[cvi_sts.ucVideoFormat].frame_rate;

                ret = (copy_to_user((void __user *)arg, &ioc_vfmt, sizeof(ioc_vfmt))) ? (-EFAULT) : 0;
            }
            break;

        case DH9901_GET_VIDEO_COLOR:
            {
                struct dh9901_ioc_vcolor_t ioc_vcolor;
                DH_VIDEO_COLOR_S vcol;

                if(copy_from_user(&ioc_vcolor, (void __user *)arg, sizeof(ioc_vcolor))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vcolor.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                ret = DH9901_API_GetColor(pdev->index, ioc_vcolor.cvi_ch, &vcol, DH_SET_MODE_USER);
                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }

                ioc_vcolor.brightness    = vcol.ucBrightness;
                ioc_vcolor.contrast      = vcol.ucContrast;
                ioc_vcolor.saturation    = vcol.ucSaturation;
                ioc_vcolor.hue           = vcol.ucHue;
                ioc_vcolor.gain          = vcol.ucGain;
                ioc_vcolor.white_balance = vcol.ucWhiteBalance;
                ioc_vcolor.sharpness     = vcol.ucSharpness;

                ret = (copy_to_user((void __user *)arg, &ioc_vcolor, sizeof(ioc_vcolor))) ? (-EFAULT) : 0;
            }
            break;

        case DH9901_SET_VIDEO_COLOR:
            {
                struct dh9901_ioc_vcolor_t ioc_vcolor;
                DH_VIDEO_COLOR_S vcol;

                if(copy_from_user(&ioc_vcolor, (void __user *)arg, sizeof(ioc_vcolor))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vcolor.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                vcol.ucBrightness   = ioc_vcolor.brightness;
                vcol.ucContrast     = ioc_vcolor.contrast;
                vcol.ucSaturation   = ioc_vcolor.saturation;
                vcol.ucHue          = ioc_vcolor.hue;
                vcol.ucGain         = ioc_vcolor.gain;
                vcol.ucWhiteBalance = ioc_vcolor.white_balance;
                vcol.ucSharpness    = ioc_vcolor.sharpness;

                ret = DH9901_API_SetColor(pdev->index, ioc_vcolor.cvi_ch, &vcol, DH_SET_MODE_USER);
                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }
            }
            break;

        case DH9901_GET_VIDEO_POS:
            {
                DH_VIDEO_POSITION_S vpos;
                struct dh9901_ioc_vpos_t ioc_vpos;

                if(copy_from_user(&ioc_vpos, (void __user *)arg, sizeof(ioc_vpos))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vpos.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                ret = DH9901_API_GetVideoPosition(pdev->index, ioc_vpos.cvi_ch, &vpos);
                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }

                ioc_vpos.h_offset = vpos.sHorizonOffset;
                ioc_vpos.v_offset = vpos.sVerticalOffset;

                ret = (copy_to_user((void __user *)arg, &ioc_vpos, sizeof(ioc_vpos))) ? (-EFAULT) : 0;
            }
            break;

        case DH9901_SET_VIDEO_POS:
            {
                DH_VIDEO_POSITION_S vpos;
                struct dh9901_ioc_vpos_t ioc_vpos;

                if(copy_from_user(&ioc_vpos, (void __user *)arg, sizeof(ioc_vpos))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vpos.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                vpos.sHorizonOffset  = ioc_vpos.h_offset;
                vpos.sVerticalOffset = ioc_vpos.v_offset;

                ret = DH9901_API_SetVideoPosition(pdev->index, ioc_vpos.cvi_ch, &vpos);
                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }
            }
            break;

        case DH9901_SET_CABLE_TYPE:
            {
                struct dh9901_ioc_cable_t ioc_cable;

                if(copy_from_user(&ioc_cable, (void __user *)arg, sizeof(ioc_cable))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_cable.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                ret = DH9901_API_SetCableType(pdev->index, ioc_cable.cvi_ch, ioc_cable.cab_type);
                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }
            }
            break;

        case DH9901_RS485_TX:
            {
                struct dh9901_ioc_rs485_tx_t ioc_rs485;

                if(copy_from_user(&ioc_rs485, (void __user *)arg, sizeof(ioc_rs485))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_rs485.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                ret = DH9901_API_Contrl485Enable(pdev->index, ioc_rs485.cvi_ch, 1);
                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }

                ret = DH9901_API_Send485Buffer(pdev->index, ioc_rs485.cvi_ch, ioc_rs485.cmd_buf, ioc_rs485.buf_len);
                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }

                ret = DH9901_API_Contrl485Enable(pdev->index, ioc_rs485.cvi_ch, 0);
                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }
            }
            break;

        case DH9901_CLEAR_EQ:
            {
                int cvi_ch;

                if(copy_from_user(&cvi_ch, (void __user *)arg, sizeof(cvi_ch))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                ret = DH9901_API_ClearEq(pdev->index, (DH_U8)cvi_ch);
                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }
            }
            break;

        default:
            ret = -ENOIOCTLCMD;
            break;
    }

exit:
    up(&pdev->lock);
    return ret;
}

static struct file_operations dh9901_miscdev_fops = {
    .owner          = THIS_MODULE,
    .open           = dh9901_miscdev_open,
    .release        = dh9901_miscdev_release,
    .unlocked_ioctl = dh9901_miscdev_ioctl,
};

int dh9901_miscdev_init(int id)
{
    int ret;

    if(id >= DH9901_DEV_MAX)
        return -1;

    /* clear */
    memset(&dh9901_dev[id].miscdev, 0, sizeof(struct miscdevice));

    /* create device node */
    dh9901_dev[id].miscdev.name  = dh9901_dev[id].name;
    dh9901_dev[id].miscdev.minor = MISC_DYNAMIC_MINOR;
    dh9901_dev[id].miscdev.fops  = (struct file_operations *)&dh9901_miscdev_fops;
    ret = misc_register(&dh9901_dev[id].miscdev);
    if(ret) {
        printk("create %s misc device node failed!\n", dh9901_dev[id].name);
        dh9901_dev[id].miscdev.name = 0;
        goto exit;
    }

exit:
    return ret;
}

int dh9901_device_register(void)
{
    int i, j, ret;
    DH_DH9901_INIT_ATTR_S dev_attr;
    DH_VIDEO_COLOR_S      video_color;
    u8 val;

    if(dh9901_api_inited)
        return -1;

    /* clear attr */
    memset(&dev_attr, 0, sizeof(DH_DH9901_INIT_ATTR_S));

    /* device number on board */
    dev_attr.ucAdCount = dev_num;

    for(i=0; i<dev_num; i++) {
        /* device i2c address */
        dev_attr.stDh9901Attr[i].ucChipAddr = iaddr[i];

        /* video attr */
        dev_attr.stDh9901Attr[i].stVideoAttr.enSetVideoMode   = DH_SET_MODE_USER;
        dev_attr.stDh9901Attr[i].stVideoAttr.ucChnSequence[0] = 0;              ///< VIN#0 -> VPORT#0
        dev_attr.stDh9901Attr[i].stVideoAttr.ucChnSequence[1] = 1;              ///< VIN#1 -> VPORT#1
        dev_attr.stDh9901Attr[i].stVideoAttr.ucChnSequence[2] = 2;              ///< VIN#2 -> VPORT#2
        dev_attr.stDh9901Attr[i].stVideoAttr.ucChnSequence[3] = 3;              ///< VIN#3 -> VPORT#3
        dev_attr.stDh9901Attr[i].stVideoAttr.ucVideoOutMux    = vout_format[i]; ///< 0:BT656, 1:BT1120

        /* audio attr */
        dev_attr.stDh9901Attr[i].stAuioAttr.bAudioEnable = 1;
        dev_attr.stDh9901Attr[i].stAuioAttr.stAudioConnectMode.ucCascade      = 0;
        dev_attr.stDh9901Attr[i].stAuioAttr.stAudioConnectMode.ucCascadeNum   = 0;
        dev_attr.stDh9901Attr[i].stAuioAttr.stAudioConnectMode.ucCascadeStage = 0;
        dev_attr.stDh9901Attr[i].stAuioAttr.enSetAudioMode    = DH_SET_MODE_USER;
        dev_attr.stDh9901Attr[i].stAuioAttr.enAudioSampleRate = sample_rate;
        dev_attr.stDh9901Attr[i].stAuioAttr.enAudioEncClkMode = DH_AUDIO_ENCCLK_MODE_MASTER;
        dev_attr.stDh9901Attr[i].stAuioAttr.enAudioSyncMode   = DH_AUDIO_SYNC_MODE_I2S;
        dev_attr.stDh9901Attr[i].stAuioAttr.enAudioEncClkSel  = DH_AUDIO_ENCCLK_MODE_27M;

        /* rs485 attr */
        dev_attr.stDh9901Attr[i].stPtz485Attr.enSetVideoMode = DH_SET_MODE_USER;
        dev_attr.stDh9901Attr[i].stPtz485Attr.ucProtocolLen  = 7;
    }

    /* auto detect thread attr */
    dev_attr.stDetectAttr.enDetectMode   = DH_SET_MODE_USER;
    dev_attr.stDetectAttr.ucDetectPeriod = 50; ///< ms

    /* register i2c read/write function */
    dev_attr.Dh9901_WriteByte = dh9901_i2c_write;
    dev_attr.Dh9901_ReadByte  = dh9901_i2c_read;

    ret = DH9901_API_Init(&dev_attr);
    if(ret != 0)
        goto exit;

    /* setup default color setting */
    video_color.ucBrightness   = 50;
    video_color.ucContrast     = 50;
    video_color.ucSaturation   = 50;
    video_color.ucHue          = 50;
    video_color.ucGain         = 0;
    video_color.ucWhiteBalance = 0;
    video_color.ucSharpness    = 1;
    for(i=0; i<dev_num; i++) {
        for(j=0; j<DH9901_DEV_CH_MAX; j++) {
            ret = DH9901_API_SetColor(i, j, &video_color, DH_SET_MODE_USER);
            if(ret != 0)
                goto exit;
        }
    }

    /* PTZ config init */
    for(i=0; i<dev_num; i++) {
        for(j=0; j<DH9901_DEV_CH_MAX; j++) {
            dh9901_dev[i].ptz[j].protocol     = DH9901_PTZ_PTOTOCOL_DHSD1;
            dh9901_dev[i].ptz[j].baud_rate    = DH9901_PTZ_BAUD_9600;
            dh9901_dev[i].ptz[j].parity_chk   = 1;
        }
    }

    /* set flag */
    dh9901_api_inited = 1;

    /* enable all HD output */
    val = 0x0;
    for(i=0; i<dev_num; i++) {    
        ret = DH9901_API_WriteReg(i, 0, 0xBB, &val);
        if(ret != 0){
            printk("Can't initialize HD video output at Dev#%d!\n", i);
            goto exit;
        }
        ret = DH9901_API_WriteReg(i, 0, 0xBC, &val);
        if(ret != 0){
            printk("Can't initialize HD video output at Dev#%d!\n", i);
            goto exit;
        }
    }
    
exit:
    return ret;
}

void dh9901_device_deregister(void)
{
    if(dh9901_api_inited) {
        DH9901_API_DeInit();
        dh9901_api_inited = 0;
    }
}

int dh9901_sound_switch(int ssp_idx, int chan_num, bool is_on)
{
    /* ToDo */

    return 0;
}

