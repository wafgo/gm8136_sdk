/**
 * @file tw2964_dev.c
 * Intersil TW2964 4-CH 960H/D1 Video Decoders and Audio Codecs Driver
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.1 $
 * $Date: 2014/09/15 12:17:35 $
 *
 * ChangeLog:
 *  $Log: tw2964_dev.c,v $
 *  Revision 1.1  2014/09/15 12:17:35  shawn_hu
 *  Add TechPoint TP2802(HDTVI) & Intersil TW2964(D1) hybrid frontend driver design for GM8286 EVB.
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
#include "techpoint/tp2802.h"          ///< from module/include/front_end/hdtvi/techpoint
#include "intersil/tw2964.h"           ///< from module/include/front_end/decoder
#include "tw2964_dev.h"
#include "tp2802_tw2964_hybrid_drv.h"

/*************************************************************************************
 *  Initial values of the following structures come from tp2802_tw2964_hybrid.c
 *************************************************************************************/
static int dev_num;
static ushort iaddr[TW2964_DEV_MAX];
static int vmode[TW2964_DEV_MAX];
static int sample_rate, sample_size;
static struct tw2964_dev_t* tw2964_dev;
static int init;
static u32 TP2802_TW2964_VCH_MAP[TP2802_DEV_MAX*TP2802_DEV_CH_MAX];

/*************************************************************************************
 *  Original structure
 *************************************************************************************/
static struct proc_dir_entry *tw2964_proc_root[TW2964_DEV_MAX]      = {[0 ... (TW2964_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2964_proc_vmode[TW2964_DEV_MAX]     = {[0 ... (TW2964_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2964_proc_status[TW2964_DEV_MAX]    = {[0 ... (TW2964_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2964_proc_bri[TW2964_DEV_MAX]       = {[0 ... (TW2964_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2964_proc_con[TW2964_DEV_MAX]       = {[0 ... (TW2964_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2964_proc_sat_u[TW2964_DEV_MAX]     = {[0 ... (TW2964_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2964_proc_sat_v[TW2964_DEV_MAX]     = {[0 ... (TW2964_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2964_proc_hue[TW2964_DEV_MAX]       = {[0 ... (TW2964_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2964_proc_sharp[TW2964_DEV_MAX]       = {[0 ... (TW2964_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2964_proc_volume[TW2964_DEV_MAX]    = {[0 ... (TW2964_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *tw2964_proc_output_ch[TW2964_DEV_MAX] = {[0 ... (TW2964_DEV_MAX - 1)] = NULL};

/*************************************************************************************
 *  Revised for hybrid design
 *************************************************************************************/
int tw2964_i2c_write(u8 id, u8 reg, u8 data)
{
    return tp2802_tw2964_i2c_write(iaddr[id], reg, data);
}
EXPORT_SYMBOL(tw2964_i2c_write);

u8 tw2964_i2c_read(u8 id, u8 reg)
{
    return tp2802_tw2964_i2c_read(iaddr[id], reg);
}
EXPORT_SYMBOL(tw2964_i2c_read);

void tw2964_set_params(tw2964_params_p params_p)
{
    int i = 0;

    dev_num = params_p->tw2964_dev_num;
    for(i=0; i<dev_num; i++) {
        iaddr[i] = *(params_p->tw2964_iaddr + i);
    }
    for(i=0; i<TW2964_DEV_MAX; i++) {
        vmode[i] = *(params_p->tw2964_vmode + i);
    }
    sample_rate = params_p->tw2964_sample_rate;
    sample_size = params_p->tw2964_sample_size;
    tw2964_dev = params_p->tw2964_dev;
    init = params_p->init;
    for(i=0; i<TP2802_DEV_MAX*TP2802_DEV_CH_MAX; i++) {
        TP2802_TW2964_VCH_MAP[i] = *(params_p->TP2802_TW2964_VCH_MAP + i);
    }
}

/*************************************************************************************
 *  Keep as original design (only XXXX_VCH_MAP was updated)
 *************************************************************************************/
int tw2964_get_vch_id(int id, TW2964_DEV_VPORT_T vport, int vport_seq)
{
    int i;
    int vport_chnum;

    if(id >= TW2964_DEV_MAX || vport >= TW2964_DEV_VPORT_MAX || vport_seq >= 4)
        return 0;

    /* get vport max channel number */
    switch(vmode[id]) {
        case TW2964_VMODE_NTSC_720H_1CH:
        case TW2964_VMODE_PAL_720H_1CH:
        case TW2964_VMODE_NTSC_960H_1CH:
        case TW2964_VMODE_PAL_960H_1CH:
            vport_chnum = 1;
            break;
        case TW2964_VMODE_NTSC_720H_2CH:
        case TW2964_VMODE_PAL_720H_2CH:
        case TW2964_VMODE_NTSC_960H_2CH:
        case TW2964_VMODE_PAL_960H_2CH:
            vport_chnum = 2;
            break;
        case TW2964_VMODE_NTSC_720H_4CH:
        case TW2964_VMODE_PAL_720H_4CH:
        case TW2964_VMODE_NTSC_960H_4CH:
        case TW2964_VMODE_PAL_960H_4CH:
        default:
            vport_chnum = 4;
            break;
    }

    for(i=0; i<(dev_num*TW2964_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[i]) == id) &&
           (CH_IDENTIFY_VOUT(TP2802_TW2964_VCH_MAP[i]) == vport) &&
           ((CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[i])%vport_chnum) == vport_seq)) {
            return i;
        }
    }

    return 0;
}
EXPORT_SYMBOL(tw2964_get_vch_id);

int tw2964_vin_to_ch(int id, int vin_idx)
{
    int i;

    if(id >= TW2964_DEV_MAX || vin_idx >= TW2964_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*TW2964_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[i]) == id) && (CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[i]) == vin_idx))
            return (i%TW2964_DEV_CH_MAX);
    }

    return 0;
}
EXPORT_SYMBOL(tw2964_vin_to_ch);

int tw2964_ch_to_vin(int id, int ch_idx)
{
    int i;

    if(id >= TW2964_DEV_MAX || ch_idx >= TW2964_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*TW2964_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[i]) == id) && ((i%TW2964_DEV_CH_MAX) == ch_idx))
            return CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[i]);
    }

    return 0;
}
EXPORT_SYMBOL(tw2964_ch_to_vin);

int tw2964_vin_to_vch(int id, int vin_idx)
{
    int i;

    if(id >= TW2964_DEV_MAX || vin_idx >= TW2964_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*TW2964_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[i]) == id) && (CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[i]) == vin_idx))
            return i;
    }

    return 0;
}
EXPORT_SYMBOL(tw2964_vin_to_vch);

/* return value, 0: success, others: failed */
int tw2964_sound_switch(int ssp_idx, int chan_num, bool is_on)
{
    const u32 single_decoder_chan_cnt  = 4;
    const u32 cascade_decoder_chan_cnt = 8;
    const u32 max_audio_chan_cnt   = 8;
    const u32 chip_id = ftpmu010_get_attr(ATTR_TYPE_CHIPVER) >> 16;
    u32 temp, vin_idx, decoder_idx = 0;

    switch (chip_id) {
        case 0x8287:
            decoder_idx = 0;
            break;
        default:
            decoder_idx = 0;
            break;
    }

    if (chan_num >= max_audio_chan_cnt) {
        printk(KERN_WARNING "%s: Not implement yet\n", __func__);
        return -EACCES;
    }

    temp = chan_num;
    if (chan_num >= cascade_decoder_chan_cnt && chan_num < max_audio_chan_cnt) {
        chan_num -= cascade_decoder_chan_cnt;
        decoder_idx = 2;
    }
    if (chan_num >= single_decoder_chan_cnt)
        chan_num -= single_decoder_chan_cnt;
    vin_idx = tw2964_ch_to_vin(decoder_idx, chan_num);

    if (temp >= single_decoder_chan_cnt)
        vin_idx += single_decoder_chan_cnt;
    is_on ? tw2964_audio_set_output_ch(decoder_idx, vin_idx) : tw2964_audio_set_output_ch(decoder_idx, 0x10);

    return 0;
}

static int tw2964_proc_vmode_show(struct seq_file *sfile, void *v)
{
    int i, vmode;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;
    char *vmode_str[] = {"NTSC_720H_1CH", "NTSC_720H_2CH", "NTSC_720H_4CH",
                         "NTSC_960H_1CH", "NTSC_960H_2CH", "NTSC_960H_4CH",
                         "PAL_720H_1CH" , "PAL_720H_2CH" , "PAL_720H_4CH",
                         "PAL_960H_1CH" , "PAL_960H_2CH" , "PAL_960H_4CH"};

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2964#%d]\n", pdev->index);
    for(i=0; i<TW2964_VMODE_MAX; i++) {
        if(tw2964_video_mode_support_check(pdev->index, i))
            seq_printf(sfile, "%02d: %s\n", i, vmode_str[i]);
    }
    seq_printf(sfile, "----------------------------------\n");

    vmode = tw2964_video_get_mode(pdev->index);
    seq_printf(sfile, "Current==> %s\n\n",(vmode >= 0 && vmode < TW2964_VMODE_MAX) ? vmode_str[vmode] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static ssize_t tw2964_proc_vmode_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int vmode;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &vmode);

    down(&pdev->lock);

    if(vmode != tw2964_video_get_mode(pdev->index))
        tw2964_video_set_mode(pdev->index, vmode);

    up(&pdev->lock);

    return count;
}

static int tw2964_proc_status_show(struct seq_file *sfile, void *v)
{
    int i, j, novid;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2964#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    NOVID     \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<TW2964_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW2964_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[j]) == i)) {
                novid = tw2964_status_get_novid(pdev->index, i);
                seq_printf(sfile, "%-7d %-7d %s\n", i, j, (novid != 0) ? "Video_Loss" : "Video_On");
            }
        }
    }
    seq_printf(sfile, "\n");

    up(&pdev->lock);

    return 0;
}

static int tw2964_proc_bri_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2964#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    BRIGHTNESS\n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<TW2964_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW2964_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw2964_video_get_brightness(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nBrightness[-128 ~ +127] ==> 0x01=+1, 0x7f=+127, 0x80=-128, 0xff=-1\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw2964_proc_con_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2964#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    CONTRAST  \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<TW2964_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW2964_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw2964_video_get_contrast(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nContrast[0%% ~ 255%%] ==> 0x00=0%%, 0xff=255%%\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw2964_proc_sat_u_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2964#%d]\n", pdev->index);
    seq_printf(sfile, "----------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    SATURATION_U\n");
    seq_printf(sfile, "----------------------------\n");
    for(i=0; i<TW2964_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW2964_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw2964_video_get_saturation_u(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nSaturation_U[0%% ~ 200%%] ==> 0x80=100%%\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw2964_proc_sat_v_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2964#%d]\n", pdev->index);
    seq_printf(sfile, "----------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    SATURATION_V\n");
    seq_printf(sfile, "----------------------------\n");
    for(i=0; i<TW2964_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW2964_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw2964_video_get_saturation_v(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nSaturation_V[0%% ~ 200%%] ==> 0x80=100%%\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw2964_proc_hue_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2964#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    HUE       \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<TW2964_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW2964_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%02x\n", i, j, tw2964_video_get_hue(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nHue[0x00 ~ 0xff] ==> 0x00=0, 0x7f=90, 0x80=-90\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw2964_proc_sharp_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2964#%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "VIN#    VCH#    SHARPNESS \n");
    seq_printf(sfile, "--------------------------\n");
    for(i=0; i<TW2964_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*TW2964_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(TP2802_TW2964_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_VIN(TP2802_TW2964_VCH_MAP[j]) == i)) {
                seq_printf(sfile, "%-7d %-7d 0x%x\n", i, j, tw2964_video_get_sharpness(pdev->index, i));
            }
        }
    }
    seq_printf(sfile, "\nSharpness[0x0 ~ 0xf] - (16 levels) ==> 0x0:no effect, 0x1~0xf:sharpness enhancement ('0xf' being the strongest)\n\n");

    up(&pdev->lock);

    return 0;
}

static int tw2964_proc_volume_show(struct seq_file *sfile, void *v)
{
    int aogain;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2964#%d]\n", pdev->index);
    aogain = tw2964_audio_get_volume(pdev->index);
    seq_printf(sfile, "Volume[0x0~0xf] = %d\n", aogain);

    up(&pdev->lock);

    return 0;
}

static int tw2964_proc_output_ch_show(struct seq_file *sfile, void *v)
{
    int ch, vin_idx;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;
    static char *output_ch_str[] = {"CH 1", "CH 2", "CH 3", "CH 4", "CH 5", "CH 6",
                                    "CH 7", "CH 8", "CH 9", "CH 10", "CH 11", "CH 12",
                                    "CH 13", "CH 14", "CH 15", "CH 16", "PLAYBACK first stage",
                                    "Reserved", "PLAYBACK last stage", "Reserved",
                                    "Mixed audio", "AIN51", "AIN52", "AIN53", "AIN54"};

    down(&pdev->lock);

    seq_printf(sfile, "\n[TW2964#%d]\n", pdev->index);

    vin_idx = tw2964_audio_get_output_ch(pdev->index);

    if (vin_idx >= 0 && vin_idx <= 7)
        ch = tw2964_vin_to_ch(pdev->index, vin_idx);
    else if (vin_idx >= 8 && vin_idx <= 15) {
        vin_idx -= 8;
        ch = tw2964_vin_to_ch(pdev->index, vin_idx);
        ch += 8;
    }
    else
        ch = vin_idx;

    seq_printf(sfile, "Current[0x0~0x18]==> %s\n\n", output_ch_str[ch]);

    up(&pdev->lock);

    return 0;
}

static ssize_t tw2964_proc_bri_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, vin;
    u32  bri;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &bri);

    down(&pdev->lock);

    for(i=0; i<TW2964_DEV_CH_MAX; i++) {
        if(i == vin || vin >= TW2964_DEV_CH_MAX) {
            tw2964_video_set_brightness(pdev->index, i, (u8)bri);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t tw2964_proc_con_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 con;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &con);

    down(&pdev->lock);

    for(i=0; i<TW2964_DEV_CH_MAX; i++) {
        if(i == vin || vin >= TW2964_DEV_CH_MAX) {
            tw2964_video_set_contrast(pdev->index, i, (u8)con);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t tw2964_proc_sat_u_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 sat;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &sat);

    down(&pdev->lock);

    for(i=0; i<TW2964_DEV_CH_MAX; i++) {
        if(i == vin || vin >= TW2964_DEV_CH_MAX) {
            tw2964_video_set_saturation_u(pdev->index, i, (u8)sat);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t tw2964_proc_sat_v_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 sat;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &sat);

    down(&pdev->lock);

    for(i=0; i<TW2964_DEV_CH_MAX; i++) {
        if(i == vin || vin >= TW2964_DEV_CH_MAX) {
            tw2964_video_set_saturation_v(pdev->index, i, (u8)sat);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t tw2964_proc_hue_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 hue;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &hue);

    down(&pdev->lock);

    for(i=0; i<TW2964_DEV_CH_MAX; i++) {
        if(i == vin || vin >= TW2964_DEV_CH_MAX) {
            tw2964_video_set_hue(pdev->index, i, (u8)hue);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t tw2964_proc_sharp_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, vin;
    u32 sharp;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &vin, &sharp);

    down(&pdev->lock);

    for(i=0; i<TW2964_DEV_CH_MAX; i++) {
        if(i == vin || vin >= TW2964_DEV_CH_MAX) {
            tw2964_video_set_sharpness(pdev->index, i, (u8)sharp);
        }
    }

    up(&pdev->lock);

    return count;
}

static ssize_t tw2964_proc_volume_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 volume;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &volume);

    down(&pdev->lock);

    if (volume == 0)
        tw2964_audio_set_mute(pdev->index, 1);
    else
        tw2964_audio_set_volume(pdev->index, volume);

    up(&pdev->lock);

    return count;
}

static ssize_t tw2964_proc_output_ch_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32 ch, vin_idx, temp;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &ch);

    temp = ch;
    if (ch >= 16)
        vin_idx = ch;

    if (ch < 16) {
        if (ch >= 8)
            ch -= 8;
        vin_idx = tw2964_ch_to_vin(pdev->index, ch);
        if (temp >= 8)
            vin_idx += 8;
    }

    down(&pdev->lock);

    tw2964_audio_set_output_ch(pdev->index, vin_idx);

    up(&pdev->lock);

    return count;
}

static int tw2964_proc_vmode_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2964_proc_vmode_show, PDE(inode)->data);
}

static int tw2964_proc_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2964_proc_status_show, PDE(inode)->data);
}

static int tw2964_proc_bri_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2964_proc_bri_show, PDE(inode)->data);
}

static int tw2964_proc_con_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2964_proc_con_show, PDE(inode)->data);
}

static int tw2964_proc_sat_u_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2964_proc_sat_u_show, PDE(inode)->data);
}

static int tw2964_proc_sat_v_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2964_proc_sat_v_show, PDE(inode)->data);
}

static int tw2964_proc_hue_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2964_proc_hue_show, PDE(inode)->data);
}

static int tw2964_proc_sharp_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2964_proc_sharp_show, PDE(inode)->data);
}

static int tw2964_proc_volume_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2964_proc_volume_show, PDE(inode)->data);
}

static int tw2964_proc_output_ch_open(struct inode *inode, struct file *file)
{
    return single_open(file, tw2964_proc_output_ch_show, PDE(inode)->data);
}

static struct file_operations tw2964_proc_vmode_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2964_proc_vmode_open,
    .write   = tw2964_proc_vmode_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2964_proc_status_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2964_proc_status_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2964_proc_bri_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2964_proc_bri_open,
    .write  = tw2964_proc_bri_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2964_proc_con_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2964_proc_con_open,
    .write  = tw2964_proc_con_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2964_proc_sat_u_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2964_proc_sat_u_open,
    .write  = tw2964_proc_sat_u_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2964_proc_sat_v_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2964_proc_sat_v_open,
    .write  = tw2964_proc_sat_v_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2964_proc_hue_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2964_proc_hue_open,
    .write  = tw2964_proc_hue_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2964_proc_sharp_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2964_proc_sharp_open,
    .write  = tw2964_proc_sharp_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2964_proc_volume_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2964_proc_volume_open,
    .write  = tw2964_proc_volume_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations tw2964_proc_output_ch_ops = {
    .owner  = THIS_MODULE,
    .open   = tw2964_proc_output_ch_open,
    .write  = tw2964_proc_output_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

void tw2964_proc_remove(int id)
{
    if(id >= TW2964_DEV_MAX)
        return;

    if(tw2964_proc_root[id]) {
        if(tw2964_proc_vmode[id]) {
            remove_proc_entry(tw2964_proc_vmode[id]->name, tw2964_proc_root[id]);
            tw2964_proc_vmode[id] = NULL;
        }

        if(tw2964_proc_status[id]) {
            remove_proc_entry(tw2964_proc_status[id]->name, tw2964_proc_root[id]);
            tw2964_proc_status[id] = NULL;
        }

        if(tw2964_proc_bri[id]) {
            remove_proc_entry(tw2964_proc_bri[id]->name, tw2964_proc_root[id]);
            tw2964_proc_bri[id] = NULL;
        }

        if(tw2964_proc_con[id]) {
            remove_proc_entry(tw2964_proc_con[id]->name, tw2964_proc_root[id]);
            tw2964_proc_con[id] = NULL;
        }

        if(tw2964_proc_sat_u[id]) {
            remove_proc_entry(tw2964_proc_sat_u[id]->name, tw2964_proc_root[id]);
            tw2964_proc_sat_u[id] = NULL;
        }

        if(tw2964_proc_sat_v[id]) {
            remove_proc_entry(tw2964_proc_sat_v[id]->name, tw2964_proc_root[id]);
            tw2964_proc_sat_v[id] = NULL;
        }

        if(tw2964_proc_hue[id]) {
            remove_proc_entry(tw2964_proc_hue[id]->name, tw2964_proc_root[id]);
            tw2964_proc_hue[id] = NULL;
        }

        if(tw2964_proc_sharp[id]) {
            remove_proc_entry(tw2964_proc_sharp[id]->name, tw2964_proc_root[id]);
            tw2964_proc_sharp[id] = NULL;
        }

        if(tw2964_proc_volume[id]) {
            remove_proc_entry(tw2964_proc_volume[id]->name, tw2964_proc_root[id]);
            tw2964_proc_volume[id] = NULL;
        }

        if(tw2964_proc_output_ch[id]) {
            remove_proc_entry(tw2964_proc_output_ch[id]->name, tw2964_proc_root[id]);
            tw2964_proc_output_ch[id] = NULL;
        }

        remove_proc_entry(tw2964_proc_root[id]->name, NULL);
        tw2964_proc_root[id] = NULL;
    }
}

int tw2964_proc_init(int id)
{
    int ret = 0;

    if(id >= TW2964_DEV_MAX)
        return -1;

    /* root */
    tw2964_proc_root[id] = create_proc_entry(tw2964_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!tw2964_proc_root[id]) {
        printk("create proc node '%s' failed!\n", tw2964_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2964_proc_root[id]->owner = THIS_MODULE;
#endif

    /* vmode */
    tw2964_proc_vmode[id] = create_proc_entry("vmode", S_IRUGO|S_IXUGO, tw2964_proc_root[id]);
    if(!tw2964_proc_vmode[id]) {
        printk("create proc node '%s/vmode' failed!\n", tw2964_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2964_proc_vmode[id]->proc_fops = &tw2964_proc_vmode_ops;
    tw2964_proc_vmode[id]->data      = (void *)&tw2964_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2964_proc_vmode[id]->owner     = THIS_MODULE;
#endif

    /* status */
    tw2964_proc_status[id] = create_proc_entry("status", S_IRUGO|S_IXUGO, tw2964_proc_root[id]);
    if(!tw2964_proc_status[id]) {
        printk("create proc node '%s/status' failed!\n", tw2964_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2964_proc_status[id]->proc_fops = &tw2964_proc_status_ops;
    tw2964_proc_status[id]->data      = (void *)&tw2964_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2964_proc_status[id]->owner     = THIS_MODULE;
#endif

    /* brightness */
    tw2964_proc_bri[id] = create_proc_entry("brightness", S_IRUGO|S_IXUGO, tw2964_proc_root[id]);
    if(!tw2964_proc_bri[id]) {
        printk("create proc node '%s/brightness' failed!\n", tw2964_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2964_proc_bri[id]->proc_fops = &tw2964_proc_bri_ops;
    tw2964_proc_bri[id]->data      = (void *)&tw2964_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2964_proc_bri[id]->owner     = THIS_MODULE;
#endif

    /* contrast */
    tw2964_proc_con[id] = create_proc_entry("contrast", S_IRUGO|S_IXUGO, tw2964_proc_root[id]);
    if(!tw2964_proc_con[id]) {
        printk("create proc node '%s/contrast' failed!\n", tw2964_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2964_proc_con[id]->proc_fops = &tw2964_proc_con_ops;
    tw2964_proc_con[id]->data      = (void *)&tw2964_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2964_proc_con[id]->owner     = THIS_MODULE;
#endif

    /* saturation u */
    tw2964_proc_sat_u[id] = create_proc_entry("saturation_u", S_IRUGO|S_IXUGO, tw2964_proc_root[id]);
    if(!tw2964_proc_sat_u[id]) {
        printk("create proc node '%s/saturation_u' failed!\n", tw2964_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2964_proc_sat_u[id]->proc_fops = &tw2964_proc_sat_u_ops;
    tw2964_proc_sat_u[id]->data      = (void *)&tw2964_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2964_proc_sat_u[id]->owner     = THIS_MODULE;
#endif

    /* saturation v */
    tw2964_proc_sat_v[id] = create_proc_entry("saturation_v", S_IRUGO|S_IXUGO, tw2964_proc_root[id]);
    if(!tw2964_proc_sat_v[id]) {
        printk("create proc node '%s/saturation_v' failed!\n", tw2964_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2964_proc_sat_v[id]->proc_fops = &tw2964_proc_sat_v_ops;
    tw2964_proc_sat_v[id]->data      = (void *)&tw2964_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2964_proc_sat_v[id]->owner     = THIS_MODULE;
#endif

    /* hue */
    tw2964_proc_hue[id] = create_proc_entry("hue", S_IRUGO|S_IXUGO, tw2964_proc_root[id]);
    if(!tw2964_proc_hue[id]) {
        printk("create proc node '%s/hue' failed!\n", tw2964_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2964_proc_hue[id]->proc_fops = &tw2964_proc_hue_ops;
    tw2964_proc_hue[id]->data      = (void *)&tw2964_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2964_proc_hue[id]->owner     = THIS_MODULE;
#endif

    /* sharpness */
    tw2964_proc_sharp[id] = create_proc_entry("sharpness", S_IRUGO|S_IXUGO, tw2964_proc_root[id]);
    if(!tw2964_proc_sharp[id]) {
        printk("create proc node '%s/sharpness' failed!\n", tw2964_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2964_proc_sharp[id]->proc_fops = &tw2964_proc_sharp_ops;
    tw2964_proc_sharp[id]->data      = (void *)&tw2964_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2964_proc_sharp[id]->owner     = THIS_MODULE;
#endif

    /* volume */
    tw2964_proc_volume[id] = create_proc_entry("volume", S_IRUGO|S_IXUGO, tw2964_proc_root[id]);
    if(!tw2964_proc_volume[id]) {
        printk("create proc node '%s/volume' failed!\n", tw2964_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2964_proc_volume[id]->proc_fops = &tw2964_proc_volume_ops;
    tw2964_proc_volume[id]->data      = (void *)&tw2964_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2964_proc_volume[id]->owner     = THIS_MODULE;
#endif

    /* output channel */
    tw2964_proc_output_ch[id] = create_proc_entry("output_ch", S_IRUGO|S_IXUGO, tw2964_proc_root[id]);
    if(!tw2964_proc_output_ch[id]) {
        printk("create proc node '%s/output_ch' failed!\n", tw2964_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    tw2964_proc_output_ch[id]->proc_fops = &tw2964_proc_output_ch_ops;
    tw2964_proc_output_ch[id]->data      = (void *)&tw2964_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    tw2964_proc_output_ch[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    tw2964_proc_remove(id);
    return ret;
}

static int tw2964_miscdev_open(struct inode *inode, struct file *file)
{
    int i, ret = 0;
    struct tw2964_dev_t *pdev = NULL;

    /* lookup device */
    for(i=0; i<dev_num; i++) {
        if(tw2964_dev[i].miscdev.minor == iminor(inode)) {
            pdev = &tw2964_dev[i];
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

static int tw2964_miscdev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long tw2964_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int tmp, vin_idx, ch, ret = 0;
    struct tw2964_ioc_data ioc_data;
    struct tw2964_dev_t *pdev = (struct tw2964_dev_t *)file->private_data;

    down(&pdev->lock);

    if(_IOC_TYPE(cmd) != TW2964_IOC_MAGIC) {
        ret = -ENOIOCTLCMD;
        goto exit;
    }

    switch(cmd) {
        case TW2964_GET_NOVID:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = tw2964_status_get_novid(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case TW2964_GET_MODE:
            tmp = tw2964_video_get_mode(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }
            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case TW2964_SET_MODE:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw2964_video_set_mode(pdev->index, tmp);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW2964_GET_CONTRAST:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = tw2964_video_get_contrast(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case TW2964_SET_CONTRAST:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw2964_video_set_contrast(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW2964_GET_BRIGHTNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = tw2964_video_get_brightness(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case TW2964_SET_BRIGHTNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw2964_video_set_brightness(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW2964_GET_SATURATION_U:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = tw2964_video_get_saturation_u(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case TW2964_SET_SATURATION_U:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw2964_video_set_saturation_u(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW2964_GET_SATURATION_V:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = tw2964_video_get_saturation_v(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case TW2964_SET_SATURATION_V:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw2964_video_set_saturation_v(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW2964_GET_HUE:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = tw2964_video_get_hue(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case TW2964_SET_HUE:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw2964_video_set_hue(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW2964_GET_SHARPNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            tmp = tw2964_video_get_sharpness(pdev->index, ioc_data.ch);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ioc_data.data = tmp;
            ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            break;

        case TW2964_SET_SHARPNESS:
            if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                ret = -EFAULT;
                goto exit;
            }

            ret = tw2964_video_set_sharpness(pdev->index, ioc_data.ch, (u8)ioc_data.data);
            if(ret < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW2964_GET_VOL:
            tmp = tw2964_audio_get_volume(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case TW2964_SET_VOL:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }

            if (tmp == 0)
                ret = tw2964_audio_set_mute(pdev->index, 1);
            else
                ret = tw2964_audio_set_volume(pdev->index, tmp);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }
            break;

        case TW2964_GET_OUT_CH:
            tmp = tw2964_audio_get_output_ch(pdev->index);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
            }

            vin_idx = tmp;

            if (vin_idx >= 0 && vin_idx <= 7)
                ch = tw2964_vin_to_ch(pdev->index, vin_idx);
            else if (vin_idx >= 8 && vin_idx <= 15) {
                vin_idx -= 8;
                ch = tw2964_vin_to_ch(pdev->index, vin_idx);
                ch += 8;
            }
            else
                ch = vin_idx;

            tmp = ch;

            ret = (copy_to_user((void __user *)arg, &tmp, sizeof(tmp))) ? (-EFAULT) : 0;
            break;

        case TW2964_SET_OUT_CH:
            if(copy_from_user(&tmp, (void __user *)arg, sizeof(tmp))) {
                ret = -EFAULT;
                goto exit;
            }

            ch = tmp;
            if (tmp >= 16)
                vin_idx = tmp;

            if (tmp < 16) {
                if (tmp >= 8)
                    tmp -= 8;
                vin_idx = tw2964_ch_to_vin(pdev->index, tmp);
                if (ch >= 8)
                    vin_idx += 8;
            }
            tmp = vin_idx;

            ret = tw2964_audio_set_output_ch(pdev->index, tmp);
            if(tmp < 0) {
                ret = -EFAULT;
                goto exit;
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

static struct file_operations tw2964_miscdev_fops = {
    .owner          = THIS_MODULE,
    .open           = tw2964_miscdev_open,
    .release        = tw2964_miscdev_release,
    .unlocked_ioctl = tw2964_miscdev_ioctl,
};

int tw2964_miscdev_init(int id)
{
    int ret;

    if(id >= TW2964_DEV_MAX)
        return -1;

    /* clear */
    memset(&tw2964_dev[id].miscdev, 0, sizeof(struct miscdevice));

    /* create device node */
    tw2964_dev[id].miscdev.name  = tw2964_dev[id].name;
    tw2964_dev[id].miscdev.minor = MISC_DYNAMIC_MINOR;
    tw2964_dev[id].miscdev.fops  = (struct file_operations *)&tw2964_miscdev_fops;
    ret = misc_register(&tw2964_dev[id].miscdev);
    if(ret) {
        printk("create %s misc device node failed!\n", tw2964_dev[id].name);
        tw2964_dev[id].miscdev.name = 0;
        goto exit;
    }

exit:
    return ret;
}

int tw2964_device_init(int id)
{
    int ret = 0;

    if (!init)
        return 0;

    if(id >= TW2964_DEV_MAX)
        return -1;

    /*====================== video init ========================= */
    ret = tw2964_video_set_mode(id, vmode[id]);
    if(ret < 0)
        goto exit;

    /*====================== audio init ========================= */
    ret = tw2964_audio_set_mode(id, vmode[id], sample_size, sample_rate);
    if(ret < 0)
        goto exit;

    /* disable all SD video output */
    ret = tw2964_i2c_write(id, 0x65, 0xF);
    if(ret < 0){
        printk("Can't disable SD video output at Dev#%d!\n", id);
        goto exit;
    }

exit:
    return ret;
}
