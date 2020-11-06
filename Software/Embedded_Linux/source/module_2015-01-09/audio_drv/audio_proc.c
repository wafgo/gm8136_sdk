/**
 * @file audio_proc.c
 *  audio proc interface
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/dma-mapping.h>
#include <asm/uaccess.h>
#include "audio_proc.h"
#include "audio_vg.h"
#include "../front_end/platform.h"
#if defined(CONFIG_PLATFORM_GM8210)
#include <mach/fmem.h>
#endif

#if defined(CONFIG_PLATFORM_GM8136)
#include <adda308_api.h>
#include "ssp_dma/ssp_dma.h"
#endif
#include <mach/ftpmu010.h>

static int *pcm_show_flag = NULL;
static int *pcm_save_flag = NULL;
static int *resample_save_flag = NULL;
static int *play_buf_dbg = NULL;
static int *ecos_state_show_flag = NULL;
static u32 *rec_dbg_stage = NULL;
static u32 *play_dbg_stage = NULL;
static u32 *nr_threshold = NULL;
static dma_addr_t pcm_show_flag_paddr = 0;
static dma_addr_t pcm_save_flag_paddr = 0;
static dma_addr_t resample_save_paddr = 0;
static dma_addr_t play_buf_dbg_paddr = 0;
static dma_addr_t rec_dbg_stage_paddr = 0;
static dma_addr_t play_dbg_stage_paddr = 0;
static dma_addr_t nr_threshold_paddr = 0;
static dma_addr_t ecos_state_show_flag_paddr = 0;

static struct proc_dir_entry *audio_proc_root = NULL;
static struct proc_dir_entry *audio_proc_vg_info    = NULL;
static struct proc_dir_entry *audio_proc_input_root = NULL;
static struct proc_dir_entry *audio_proc_input_tab  = NULL;
static struct proc_dir_entry *audio_proc_codec_time_adjust  = NULL;
static struct proc_dir_entry *audio_proc_queued_time  = NULL;
static struct proc_dir_entry *audio_proc_runtime_value  = NULL;
static struct proc_dir_entry *audio_proc_buffer_chase  = NULL;
static struct proc_dir_entry *audio_proc_sync_change_level  = NULL;

#if defined(CONFIG_PLATFORM_GM8210)
static struct proc_dir_entry *audio_7500_log  = NULL;
static struct proc_dir_entry *audio_7500_path  = NULL;
static struct proc_dir_entry *audio_7500_log_level = NULL;
static struct proc_dir_entry *audio_7500_log_mode = NULL;
#endif

static struct proc_dir_entry *audio_proc_pcm_data_debug  = NULL;
static struct proc_dir_entry *audio_proc_pcm_save_debug  = NULL;
static struct proc_dir_entry *audio_proc_resample_save_debug  = NULL;
static struct proc_dir_entry *audio_proc_flow_stage_debug  = NULL;
static struct proc_dir_entry *audio_proc_play_buf_debug  = NULL;
static struct proc_dir_entry *audio_proc_record_timeout  = NULL;
static struct proc_dir_entry *audio_proc_playback_timeout  = NULL;
static struct proc_dir_entry *audio_proc_record_overrun  = NULL;
static struct proc_dir_entry *audio_proc_playback_underrun  = NULL;
static struct proc_dir_entry *audio_proc_rec_req_dma_chan_failed_cnt  = NULL;
static struct proc_dir_entry *audio_proc_play_req_dma_chan_failed_cnt  = NULL;
static struct proc_dir_entry *audio_proc_nr_threashold  = NULL;
static struct proc_dir_entry *audio_proc_ecos_state_debug = NULL;
static struct proc_dir_entry *audio_proc_log_level = NULL;
static struct proc_dir_entry *audio_proc_dump_reg = NULL;

#if defined(CONFIG_PLATFORM_GM8210)
#define MAX_PATH_WIDTH      60
#define LOG_KSZ             1024
#define LOG_7500_KNUM       2046
#define LOG_7500_TOLSZ     (LOG_7500_KNUM*LOG_KSZ)

static struct ft75_log_info_s {
    u32 phy_7500_addr;
    u32 log_7500_sz;
} g_ft75_log_info;

static char defaul_path[MAX_PATH_WIDTH] = "/mnt/nfs";
extern unsigned int log_buffer_vbase;
extern bool enable_fc7500_log;
unsigned int ecos_log_level = (GM_LOG_INIT | GM_LOG_ERROR); //(GM_LOG_INIT | GM_LOG_ERROR | GM_LOG_PB_PKT_FLOW | GM_LOG_PB_DSR);
static unsigned int ecos_log_mode = GM_LOG_MODE_FILE;
#endif

extern audio_register_t audio_register;
unsigned int gm_log_level = GM_LOG_QUIET; //(GM_LOG_ERROR | GM_LOG_REC_PKT_FLOW | GM_LOG_PB_PKT_FLOW);

static struct proc_dir_entry *audio_proc_create_entry(const char *name, mode_t mode, struct proc_dir_entry *parent)
{
    struct proc_dir_entry *pentry;
    struct proc_dir_entry *root;

    root = (parent == NULL) ? audio_proc_root : parent;

    pentry = create_proc_entry(name, mode, root);

    return pentry;
}

static void audio_proc_remove_entry(struct proc_dir_entry *parent, struct proc_dir_entry *entry)
{
    struct proc_dir_entry *root;

    root = (parent == NULL) ? audio_proc_root : parent;

    if(entry)
        remove_proc_entry(entry->name, root);
}

int audio_proc_register(const char *name)
{
    int ret = 0;
    struct proc_dir_entry *pentry;

    pentry = create_proc_entry(name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);

    if(!pentry) {
        ret = -ENOMEM;
        goto end;
    }
    //pentry->owner  = THIS_MODULE;
    audio_proc_root = pentry;

end:
    return ret;
}
EXPORT_SYMBOL(audio_proc_register);

void audio_proc_unregister(void)
{
    if(audio_proc_root) {
        remove_proc_entry(audio_proc_root->name, NULL);
    }
    audio_proc_root = NULL;
}
EXPORT_SYMBOL(audio_proc_unregister);


static inline unsigned int au_get_sample_rate_bmp(int vch)
{
    unsigned int bmp = 0;

#if defined(CONFIG_PLATFORM_GM8210) || defined(CONFIG_PLATFORM_GM8287)
    unsigned int *sample_rate = au_get_sample_rate();
    switch (sample_rate[vch]) {
        case 8000:
            bmp |= EM_AU_CAPABILITY_SAMPLE_RATE_8K;
            break;
        case 11025:
            bmp |= EM_AU_CAPABILITY_SAMPLE_RATE_11K;
            break;
        case 16000:
            bmp |= EM_AU_CAPABILITY_SAMPLE_RATE_16K;
            break;
        case 22050:
            bmp |= EM_AU_CAPABILITY_SAMPLE_RATE_22K;
            break;
        case 32000:
            bmp |= EM_AU_CAPABILITY_SAMPLE_RATE_32K;
            break;
        case 44100:
            bmp |= EM_AU_CAPABILITY_SAMPLE_RATE_44K;
            break;
        case 48000:
            bmp |= EM_AU_CAPABILITY_SAMPLE_RATE_48K;
            break;
    }
#elif defined(CONFIG_PLATFORM_GM8139)
    bmp |= (EM_AU_CAPABILITY_SAMPLE_RATE_8K | EM_AU_CAPABILITY_SAMPLE_RATE_16K | EM_AU_CAPABILITY_SAMPLE_RATE_22K | EM_AU_CAPABILITY_SAMPLE_RATE_44K | EM_AU_CAPABILITY_SAMPLE_RATE_48K);
#elif defined(CONFIG_PLATFORM_GM8136)
    if (adda308_get_output_mode()==SPK_OUTPUT)
        bmp |= (EM_AU_CAPABILITY_SAMPLE_RATE_8K | EM_AU_CAPABILITY_SAMPLE_RATE_16K | EM_AU_CAPABILITY_SAMPLE_RATE_48K);
    else
        bmp |= (EM_AU_CAPABILITY_SAMPLE_RATE_8K | EM_AU_CAPABILITY_SAMPLE_RATE_16K | EM_AU_CAPABILITY_SAMPLE_RATE_22K | EM_AU_CAPABILITY_SAMPLE_RATE_44K | EM_AU_CAPABILITY_SAMPLE_RATE_48K);
#endif
    return bmp;
}

static inline int au_get_sample_size_bmp(void)
{
    unsigned int bmp = 0;
    if (0/*support 8bit*/)
        bmp |= EM_AU_CAPABILITY_SAMPLE_SIZE_8BIT;
    if (1/*support 16bit*/)
        bmp |= EM_AU_CAPABILITY_SAMPLE_SIZE_16BIT;
    return bmp;
}

static inline int au_get_channels_bmp(void)
{
    unsigned int bmp = 0, reg;
    unsigned int chipID, bondingID = 0;

    reg = ftpmu010_read_reg(0x00);
    bondingID = (reg & 0xf) | ((reg >> 4) & 0xf0);
    chipID = (ftpmu010_get_attr(ATTR_TYPE_CHIPVER) >> 16) & 0xFFFF;
    
#if defined(CONFIG_PLATFORM_GM8210) || defined(CONFIG_PLATFORM_GM8287)
    bmp |= EM_AU_CAPABILITY_CHANNELS_1CH;
#elif defined(CONFIG_PLATFORM_GM8139)
    if ((chipID == 0x8139) || 
        ((chipID == 0x8138) && ((bondingID == 0xCF) || (bondingID == 0xAF))) || 
        ((chipID == 0x8137) && (bondingID == 0x6F))) {
    bmp |= (EM_AU_CAPABILITY_CHANNELS_2CH | EM_AU_CAPABILITY_CHANNELS_1CH);
    } else if (((chipID == 0x8138) && (bondingID == 0xDF)) || ((chipID == 0x8137) && (bondingID == 0x7F))) {
        bmp |= EM_AU_CAPABILITY_CHANNELS_1CH;
    }
#elif defined(CONFIG_PLATFORM_GM8136)
    bmp |= EM_AU_CAPABILITY_CHANNELS_1CH;
#endif
    return bmp;
}

static int audio_proc_vg_info_read(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;

    int i, j, node_num = 0;
#if defined(CONFIG_PLATFORM_GM8210)
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    fmem_get_identifier(&pci_id, &cpu_id);
    node_num = pci_id;
#endif

#if defined(CONFIG_PLATFORM_GM8139)
    char adda_name[] = "ADDA302";
#elif defined(CONFIG_PLATFORM_GM8136)
    char adda_name[] = "ADDA308";
#endif

    len += sprintf(page + len, "|[General Flag]\n");
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    len += sprintf(page + len, "|Support AuGrab Notify\n");
    len += sprintf(page + len, "|Support AuRender Notify\n");
#else
    len += sprintf(page + len, "|(None)\n");
#endif
    len += sprintf(page + len, "|[Description]\n");
    len += sprintf(page + len, "|  sample rate bit[6:0]: 8k/11.025k/16k/22.05k/44.1k/48k\n");
    len += sprintf(page + len, "|  sample size bit[1:0]: 8bit/16bit\n");
    len += sprintf(page + len, "|  channels bit[1:0]: mono/stereo\n");
    len += sprintf(page + len, "|\n");
    len += sprintf(page + len, "|In/Out vch# node# SampleRate SampleSize Channels Description\n");
    len += sprintf(page + len, "|----------------------------------------------------------------------\n");

    // In(record)
    for (i = 0; i < au_get_max_chan(); i++) {
        int chan = i, ssp_idx = 0, *ssp_chan = au_get_ssp_chan();

        for (j = 0; j < au_get_ssp_max(); j++) {
            if (chan / ssp_chan[j] > 0) {
                ssp_idx ++;
                chan -= ssp_chan[j];
            } else
                break;
        }
        if (ssp_idx > au_get_ssp_max() - 1)
            ssp_idx = au_get_ssp_max() - 1;

        len += sprintf(page + len, "    %3d  %3d   %3d       0x%02x       0x%02x     0x%02x %s\n", 0, i, node_num,
               au_get_sample_rate_bmp(ssp_idx), au_get_sample_size_bmp(), au_get_channels_bmp(), "Record");
    }

    // Out(playback)
    for (i = 0; i < au_get_ssp_max(); i++) {
        int *out_enable = au_get_out_enable();

        if (out_enable[i]) {
#if defined(CONFIG_PLATFORM_GM8210)
            int *ssp_num = au_get_ssp_num();
            if (pci_id == FMEM_PCI_HOST)
                len += sprintf(page + len, "    %3d  %3d   %3d       0x%02x       0x%02x     0x%02x %s\n",
                           1, i, node_num, au_get_sample_rate_bmp(i), au_get_sample_size_bmp(),
                           au_get_channels_bmp(), ssp_num[i] == 3 ? "HDMI" : "Front-End DAC");
            else
                len += sprintf(page + len, "    %3d  %3d   %3d       0x%02x       0x%02x     0x%02x %s\n",
                           1, i, node_num, au_get_sample_rate_bmp(i), au_get_sample_size_bmp(),
                           au_get_channels_bmp(),  ssp_num[i] == 3 ? "PASS to RC" : "Front-End DAC");
#elif defined(CONFIG_PLATFORM_GM8287)
            int *ssp_num = au_get_ssp_num();
            len += sprintf(page + len, "    %3d  %3d   %3d       0x%02x       0x%02x     0x%02x %s\n",
                       1, i, node_num, au_get_sample_rate_bmp(i), au_get_sample_size_bmp(),
                       au_get_channels_bmp(), ssp_num[i] == 2 ? "HDMI" : "Front-End DAC");
#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
            len += sprintf(page + len, "    %3d  %3d   %3d       0x%02x       0x%02x     0x%02x %s\n",
                       1, i, node_num, au_get_sample_rate_bmp(i), au_get_sample_size_bmp(),
                       au_get_channels_bmp(), adda_name);
#endif
        } else
            len += sprintf(page + len, "    %3d  %3d   %3d       0x%02x       0x%02x     0x%02x %s\n",
                       1, i, node_num, au_get_sample_rate_bmp(i), au_get_sample_size_bmp(),
                       au_get_channels_bmp(), "No support");
    }

    len += sprintf(page + len, "\n");


    *eof = 1;

    return len;
}
/*
static int audio_proc_vg_info_show(struct seq_file *sfile, void *v)
{
    int i, j, node_num = 0;
#if defined(CONFIG_PLATFORM_GM8210)
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    fmem_get_identifier(&pci_id, &cpu_id);
    node_num = pci_id;
#endif

    seq_printf(sfile, "|[Description]\n");
    seq_printf(sfile, "|  sample rate bit[6:0]: 8k/11.025k/16k/22.05k/32k/44.1k/48k\n");
    seq_printf(sfile, "|  sample size bit[1:0]: 8bit/16bit\n");
    seq_printf(sfile, "|  channels bit[1:0]: mono/stereo\n");
    seq_printf(sfile, "|\n");
    seq_printf(sfile, "|In/Out vch# node# SampleRate SampleSize Channels Description\n");
    seq_printf(sfile, "|----------------------------------------------------------------------\n");

    for (i = 0; i < au_get_max_chan(); i++) {
        int chan = i, ssp_idx = 0, *ssp_chan = au_get_ssp_chan();
        for (j = 0; j < au_get_ssp_max(); j++) {
            if (chan / ssp_chan[j] > 0) {
                ssp_idx ++;
                chan -= ssp_chan[j];
            } else
                break;
        }
        if (ssp_idx > au_get_ssp_max() - 1)
            ssp_idx = au_get_ssp_max() - 1;
        seq_printf(sfile, "    %3d  %3d   %3d       0x%02x       0x%02x     0x%02x %s\n", 0, i, node_num,
               au_get_sample_rate_bmp(ssp_idx), au_get_sample_size_bmp(), au_get_channels_bmp(), "Record");

    }
    for (i = 0; i < au_get_ssp_max(); i++) {
        int *out_enable = au_get_out_enable();

        if (out_enable[i]) {
#if defined(CONFIG_PLATFORM_GM8210)
            int *ssp_num = au_get_ssp_num();
            if (pci_id == FMEM_PCI_HOST)
                seq_printf(sfile, "    %3d  %3d   %3d       0x%02x       0x%02x     0x%02x %s\n",
                           1, i, node_num, au_get_sample_rate_bmp(i), au_get_sample_size_bmp(),
                           au_get_channels_bmp(), ssp_num[i] == 3 ? "HDMI" : "Front-End DAC");
            else
                seq_printf(sfile, "    %3d  %3d   %3d       0x%02x       0x%02x     0x%02x %s\n",
                           1, i, node_num, au_get_sample_rate_bmp(i), au_get_sample_size_bmp(),
                           au_get_channels_bmp(),  ssp_num[i] == 3 ? "PASS to RC" : "Front-End DAC");
#elif defined(CONFIG_PLATFORM_GM8287)
            int *ssp_num = au_get_ssp_num();
            seq_printf(sfile, "    %3d  %3d   %3d       0x%02x       0x%02x     0x%02x %s\n",
                       1, i, node_num, au_get_sample_rate_bmp(i), au_get_sample_size_bmp(),
                       au_get_channels_bmp(), ssp_num[i] == 2 ? "HDMI" : "Front-End DAC");
#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
            seq_printf(sfile, "    %3d  %3d   %3d       0x%02x       0x%02x     0x%02x %s\n",
                       1, i, node_num, au_get_sample_rate_bmp(i), au_get_sample_size_bmp(),
                       au_get_channels_bmp(), "ADDA302");
#endif
        } else
            seq_printf(sfile, "    %3d  %3d   %3d       0x%02x       0x%02x     0x%02x %s\n",
                       1, i, node_num, au_get_sample_rate_bmp(i), au_get_sample_size_bmp(),
                       au_get_channels_bmp(), "No support");
    }

    seq_printf(sfile, "\n");

    return 0;
}

static int audio_proc_vg_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_vg_info_show, NULL);
}
*/
static int audio_proc_input_show(struct seq_file *sfile, void *v)
{
    int  i, j;
    char buf[32];

    seq_printf(sfile, "|CH#	Name	SRATE	BITWIDTH\n");
    seq_printf(sfile, "|============================\n");
    for (i = 0; i < au_get_max_chan(); i++) {
        int chan = i, ssp_idx = 0, *ssp_chan = au_get_ssp_chan();
        for (j = 0; j < au_get_ssp_max(); j++) {
            if (chan / ssp_chan[j] > 0) {
                ssp_idx ++;
                chan -= ssp_chan[j];
            } else
                break;
        }
        sprintf(buf, "%s.%d", plat_audio_get_codec_name(), ssp_idx);
        seq_printf(sfile, "%-3d %-16s %-14d %d\n", i, buf, DEFAULT_SAMPLE_RATE, DEFAULT_SAMPLE_SIZE);
    }
    seq_printf(sfile, "|============================\n");

    return 0;
}

static int audio_proc_input_tab_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_input_show, PDE(inode)->data);
}

static int audio_proc_codec_time_adjust_show(struct seq_file *sfile, void *v)
{
    int  i;
    struct au_codec_info_st *au_codec_info_ary = au_vg_get_codec_info_array();

    if (!au_codec_info_ary)
        return -1;

    seq_printf(sfile, "|Type(value)    adjust(ms)    frame samples\n");
    seq_printf(sfile, "|==========================================\n");
    for (i = 0; i < MAX_CODEC_TYPE_COUNT; i ++) {
        if (i == CODEC_TYPE_NONE)
            continue;
        seq_printf(sfile, "|%6s(%d)     %6d      %6d\n", au_codec_info_ary[i].name, i,
                   au_codec_info_ary[i].time_adjust, au_codec_info_ary[i].frame_samples);
    }
    seq_printf(sfile, "|==========================================\n");
    return 0;
}

static int audio_proc_queued_time_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "Audio buffer queued time : %u (ms)\n", au_vg_get_log_info(LOG_VALUE_USER_QUEUE_TIME, 0));
    return 0;
}

static int audio_proc_pcm_data_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "PCM Data flag : %d\n", *audio_drv_get_pcm_data_flag());
    return 0;
}

static int audio_proc_pcm_save_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "PCM Save flag : %d\n", *audio_drv_get_pcm_save_flag());
    return 0;
}

static int audio_proc_resample_save_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "Resample Save flag : %d\n", *audio_drv_get_resample_save_flag());
    return 0;
}

static int audio_proc_nr_threshold_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "Audio Noise Reduction threshold : %d\n", *nr_threshold);
    return 0;
}

static int audio_proc_flow_dbg_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "Audio record stage : 0x%x\n", *audio_flow_get_rec_stage());
    seq_printf(sfile, "Audio playback stage : 0x%x\n", *audio_flow_get_play_stage());
    return 0;
}

static int audio_proc_play_buf_dbg_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "Playback Buf flag : %d\n", *audio_drv_get_play_buf_dbg());
    return 0;
}

static int audio_proc_record_timeout_show(struct seq_file *sfile, void *v)
{
    int i;
    int *audio_enc_timeout = audio_get_enc_timeout();
    for (i = 0;i < au_get_ssp_max();i ++)
        seq_printf(sfile, "vch(%d): audio record timeout = %d\n", i, audio_enc_timeout[i]);
    return 0;
}

static int audio_proc_playback_timeout_show(struct seq_file *sfile, void *v)
{
    int i;
    int *audio_dec_timeout = audio_get_dec_timeout();
    for (i = 0;i < au_get_ssp_max();i ++)
        seq_printf(sfile, "vch(%d): audio playback timeout = %d\n", i, audio_dec_timeout[i]);
    return 0;
}

static int audio_proc_record_overrun_show(struct seq_file *sfile, void *v)
{
    int i;
    int *audio_rec_overrun = audio_get_enc_overrun();
    for (i = 0;i < au_get_ssp_max();i ++)
        seq_printf(sfile, "vch(%d): audio record overrun count = %u\n", i, audio_rec_overrun[i]);
    return 0;
}

static int audio_proc_rec_req_dma_chan_failed_cnt_show(struct seq_file *sfile, void *v)
{
    int i;
    int *audio_rec_req_dma_chan_failed_cnt = au_get_enc_req_dma_chan_failed_cnt();
    for (i = 0;i < au_get_ssp_max();i ++)
        seq_printf(sfile, "vch(%d): audio record request dma channel failed count = %u\n", i, audio_rec_req_dma_chan_failed_cnt[i]);
    return 0;
}

static int audio_proc_play_req_dma_chan_failed_cnt_show(struct seq_file *sfile, void *v)
{
    int i;
    int *audio_play_req_dma_chan_failed_cnt = au_get_dec_req_dma_chan_failed_cnt();
    for (i = 0;i < au_get_ssp_max();i ++)
        seq_printf(sfile, "vch(%d): audio playback request dma channel failed count = %u\n", i, audio_play_req_dma_chan_failed_cnt[i]);
    return 0;
}

static int audio_proc_playback_underrun_show(struct seq_file *sfile, void *v)
{
    int i;
    int *audio_play_underrun = audio_get_dec_underrun();
    for (i = 0;i < au_get_ssp_max();i ++)
        seq_printf(sfile, "vch(%d): audio playback underrun count = %u\n", i, audio_play_underrun[i]);
    return 0;
}

static int audio_proc_buffer_chase_show(struct seq_file *sfile, void *v)
{
    int value = au_vg_get_log_info(LOG_VALUE_SPEEDUP_RATIO, 0);
    if (value == 100 || value == 0)
        seq_printf(sfile, "Audio buffer chase : OFF\n");
    else
        seq_printf(sfile, "Audio buffer chase : ON (ratio = %d%%)\n", value);
    return 0;
}

static int audio_proc_sync_change_level_show(struct seq_file *sfile, void *v)
{
    int value = au_vg_get_log_info(LOG_VALUE_SYNC_CHANGE_LEVEL, 0);
    if (value == 100 || value == 0)
        seq_printf(sfile, "Audio speed change for sync : OFF\n");
    else
        seq_printf(sfile, "Audio speed change for sync : ON (level = %d%%)\n", value);
    return 0;
}

static int audio_proc_ecos_state_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "eCos state debug flag : %d\n", *audio_drv_get_ecos_state_flag());
    return 0;
}

static int audio_proc_runtime_values_show(struct seq_file *sfile, void *v)
{
    int buf_time, leave_time;
    seq_printf(sfile, "jiffies time(%u)\n", (unsigned int) jiffies);
    seq_printf(sfile, "--------------------------------------\n");
    seq_printf(sfile, "Mute counts : %u (times)\n", au_vg_get_log_info(LOG_VALUE_MUTE, 1));
    seq_printf(sfile, "Mute samples : %u (samples)\n", au_vg_get_log_info(LOG_VALUE_MUTE, 0));
    seq_printf(sfile, "Sync resample-up counts : %u (times)\n", au_vg_get_log_info(LOG_VALUE_SYNC_RESAMPLE_UP, 0));
    seq_printf(sfile, "Sync resample-down counts : %u (times)\n", au_vg_get_log_info(LOG_VALUE_SYNC_RESAMPLE_DOWN, 0));

    buf_time = au_vg_get_log_info(LOG_VALUE_HARDWARE_QUEUE_TIME, 0);
    leave_time = 7 * au_vg_get_log_info(LOG_VALUE_ONE_BLOCK_TIME, 0) - buf_time;
    seq_printf(sfile, "Hardware buffer time : %u(ms)  Leave: %u(ms)\n", buf_time, leave_time);

    seq_printf(sfile, "Buffer chase counts : %u (times)\n", au_vg_get_log_info(LOG_VALUE_BUFFER_CHASE, 0));
    seq_printf(sfile, "Hardware one block time : %u (ms)\n", au_vg_get_log_info(LOG_VALUE_ONE_BLOCK_TIME, 0));
    seq_printf(sfile, "Video latency when decoding: %u (ms)\n", au_vg_get_log_info(LOG_VALUE_DEC_VIDEO_LATENCY, 0));
    return 0;
}

static ssize_t audio_proc_codec_time_adjust_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int codec_type, time_adjust;
    char value_str[32] = {'\0'};
    struct au_codec_info_st *au_codec_info_ary;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &codec_type, &time_adjust);

    if (codec_type <= CODEC_TYPE_NONE || codec_type >= MAX_CODEC_TYPE_COUNT) {
        printk("Error! The codec type(%d) is over the range(1~%d).\n", codec_type, MAX_CODEC_TYPE_COUNT - 1);
        printk("Usage: echo [codec] [time adjust] > au_time_adjust\n");
    } else {
        au_codec_info_ary = au_vg_get_codec_info_array();
        if (au_codec_info_ary)
            au_codec_info_ary[codec_type].time_adjust = time_adjust;
        else
            printk("Au-codec-info array doesn't exist.\n");
    }

    return count;
}


static ssize_t audio_proc_queued_time_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int queue_time;
    char value_str[32] = {'\0'};

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &queue_time);

    if (queue_time < 0 || queue_time > 1000) {
        printk("Error! The queue_time(%d) is over the range(1~%d).\n", queue_time, 1000);
        printk("Usage: echo [queue_time] > au_queued_time\n");
    } else
        au_vg_set_log_info(LOG_VALUE_USER_QUEUE_TIME, queue_time);

    return count;
}

static ssize_t audio_proc_pcm_flag_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int flag;
    char value_str[32] = {'\0'};

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &flag);

    if (flag > 0)
        audio_drv_set_pcm_data_flag(true);
    else
        audio_drv_set_pcm_data_flag(false);

    return count;
}

static ssize_t audio_proc_ecos_state_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int flag;
    char value_str[32] = {'\0'};

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &flag);

    if (flag > 0)
        audio_drv_set_ecos_state_flag(true);
    else
        audio_drv_set_ecos_state_flag(false);

    return count;
}

static ssize_t audio_proc_pcm_save_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int flag;
    char value_str[32] = {'\0'};

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &flag);

    if (flag > 0)
        audio_drv_set_pcm_save_flag(true);
    else
        audio_drv_set_pcm_save_flag(false);

    return count;
}

static ssize_t audio_proc_resample_save_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int flag;
    char value_str[32] = {'\0'};

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &flag);

    if (flag > 0)
        audio_drv_set_resample_save_flag(true);
    else
        audio_drv_set_resample_save_flag(false);

    return count;
}

void audio_drv_set_nr_threshold(u32 threshold);
static ssize_t audio_proc_nr_threshold_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int flag;
    char value_str[32] = {'\0'};

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &flag);

    if (flag >= 0)
        *nr_threshold = flag;
    else
        printk("threshold value can't smaller than zero!!\n");

    return count;
}
static ssize_t audio_proc_play_buf_dbg_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int flag;
    char value_str[32] = {'\0'};

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &flag);

    if (flag > 0)
        audio_drv_set_play_buf_dbg(true);
    else
        audio_drv_set_play_buf_dbg(false);

    return count;
}

/* fill boolean timeout for all vch */
static ssize_t audio_proc_record_timeout_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int flag, i;
    char value_str[32] = {'\0'};
    int *audio_enc_timeout = audio_get_enc_timeout();

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &flag);

    for (i = 0;i < au_get_ssp_max();i ++)
        audio_enc_timeout[i] = flag;

    return count;
}

/* fill boolean timeout for all vch */
static ssize_t audio_proc_playback_timeout_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int flag, i;
    char value_str[32] = {'\0'};
    int *audio_dec_timeout = audio_get_dec_timeout();

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &flag);

    for (i = 0;i < au_get_ssp_max();i ++)
        audio_dec_timeout[i] = flag;

    return count;
}

static ssize_t audio_proc_record_overrun_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int flag, i;
    char value_str[32] = {'\0'};
    int *audio_rec_overrun = audio_get_enc_overrun();

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &flag);

    for (i = 0;i < au_get_ssp_max();i ++)
        audio_rec_overrun[i] = flag;

    return count;
}

static ssize_t audio_proc_rec_req_dma_chan_failed_cnt_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int flag, i;
    char value_str[32] = {'\0'};
    int *audio_rec_req_dma_chan_failed_cnt = au_get_enc_req_dma_chan_failed_cnt();

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &flag);

    for (i = 0;i < au_get_ssp_max();i ++)
        audio_rec_req_dma_chan_failed_cnt[i] = flag;

    return count;
}

static ssize_t audio_proc_play_req_dma_chan_failed_cnt_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int flag, i;
    char value_str[32] = {'\0'};
    int *audio_play_req_dma_chan_failed_cnt = au_get_dec_req_dma_chan_failed_cnt();

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &flag);

    for (i = 0;i < au_get_ssp_max();i ++)
        audio_play_req_dma_chan_failed_cnt[i] = flag;

    return count;
}

static ssize_t audio_proc_playback_underrun_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int flag, i;
    char value_str[32] = {'\0'};
    int *audio_play_underrun = audio_get_dec_underrun();

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &flag);

    for (i = 0;i < au_get_ssp_max();i ++)
        audio_play_underrun[i] = flag;

    return count;
}

static ssize_t audio_proc_buffer_chase_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int buffer_chase;
    char value_str[32] = {'\0'};

    if(copy_from_user(value_str, buffer, count))
        goto exit;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &buffer_chase);

    if (buffer_chase == 0)  //turn off
        buffer_chase = 100;
    else if (buffer_chase == 1) //turn on
        buffer_chase = 95;
    else if (buffer_chase < 50 || buffer_chase > 150) {  // or ratio: 50~150
        printk("Error! The buffer_chase(%d) is over the range(0,1 or 50~150).\n", buffer_chase);
        printk("Usage: echo [buffer_chase] > au_buffer_chase\n");
        goto exit;
    }
    au_vg_set_log_info(LOG_VALUE_SPEEDUP_RATIO, buffer_chase);
exit:

    return count;
}

static ssize_t audio_proc_sync_change_level_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    int sync_change_level;
    char value_str[32] = {'\0'};

    if(copy_from_user(value_str, buffer, count))
        goto exit;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &sync_change_level);

    if (sync_change_level == 0 || sync_change_level == 100)  //turn off
        sync_change_level = 0;
    else if (sync_change_level < 0 || sync_change_level > 20) {  // or ratio: 0~20
        printk("Error! The sync_change_level(%d) is over the range(0~20).\n", sync_change_level);
        printk("Usage: echo [sync_change_level] > au_sync_change_level\n");
        goto exit;
    }
    au_vg_set_log_info(LOG_VALUE_SYNC_CHANGE_LEVEL, sync_change_level);
exit:
    return count;
}

#if defined(CONFIG_PLATFORM_GM8210)
void audio_proc_set_7500log_start_address(u32 phy_addr,u32 size)
{
    printk("set log buffer address: 0x%p, size: %u\n", (void *)phy_addr, size);

    g_ft75_log_info.phy_7500_addr = phy_addr;
    g_ft75_log_info.log_7500_sz = size;
}

void audio_proc_log_write(u32 log_slice1_ptr,u32 log_slice1_size,u32 log_slice2_ptr
                          ,u32 log_slice2_size,char* log_path)
{
    int ret;
    mm_segment_t fs;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,24))
    unsigned long long offset = 0;
    struct file *filp;
#else
    int fd1;
#endif
    printk("[LOG]Log Writing...\n");

    fs = get_fs();
    set_fs(KERNEL_DS);

    if ((log_slice1_ptr == 0) || (log_slice1_size == 0) || (strlen(log_path) == 0)) {
        printk("---NO Log Message---\n");
        return;
    }
    printk("Write Buffer 0x%x(0x%x) && 0x%x(0x%x) to %s\n",
           log_slice1_ptr, log_slice1_size, log_slice2_ptr, log_slice2_size, log_path);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 24))
    filp = filp_open(log_path, O_WRONLY | O_CREAT, 0777);

    if (IS_ERR(filp)) {
        printk("Error to open %s\n", log_path);
        return;
    }
    ret = vfs_write(filp, (unsigned char *)log_slice1_ptr, log_slice1_size, &offset);
    if (log_slice2_size)
        ret = vfs_write(filp, (unsigned char *)log_slice2_ptr, log_slice2_size,&offset);
    filp_close(filp, NULL);
#else
    fd1 = sys_open(log_path, O_WRONLY | O_CREAT, 0777);

    if (fd1 < 0) {
        printk("Error to open %s\n", log_path);
        goto returnit;
    }
    ret = sys_write(fd1, (unsigned char *)log_slice1_ptr, log_slice1_size);
    if (log_slice2_size)
        ret = sys_write(fd1, (unsigned char *)log_slice2_ptr, log_slice2_size);
    sys_close(fd1);
#endif

    set_fs(fs);

    if (ret)
        printk("\n======================\n Write DONE!!!!!!\n======================\n");
    else
        printk("\n======================\n Write FAIL!!!!!!\n======================\n");
}

static int audio_proc_7500_log_show(struct seq_file *sfile, void *v)
{
    unsigned int log_print_ptr,log_iomem ,log_base_start,log_base_end;
    unsigned int log_real_size = 0,offset_s = 0,log_start_ptr,size;
    unsigned int log_slice1_ptr,log_slice1_size,log_slice2_ptr,log_slice2_size;
    unsigned int * log_r_p ;
    int          offset=0;
    char log_path[MAX_PATH_WIDTH] = "";

    if(!g_ft75_log_info.phy_7500_addr){
        printk("7500 log base address isn't init\n");
        return 0;
    }

    log_iomem = (u32)ioremap_nocache(g_ft75_log_info.phy_7500_addr, g_ft75_log_info.log_7500_sz);

    if (!log_iomem) {
        printk("%s, no virtual memory! \n", __func__);
        return 0;
    }

    log_r_p = (unsigned int*) (log_iomem + (LOG_KSZ - (sizeof(unsigned int)*4)));
    log_real_size = *log_r_p;
    log_r_p++;
    offset_s = *log_r_p;

    if(log_real_size > LOG_7500_TOLSZ || offset_s > LOG_7500_TOLSZ){
        printk("%s, unknow size (%d-%d) \n", __func__,log_real_size,offset_s);
        goto log_done;
    }

    log_base_start = log_iomem + LOG_KSZ;
    log_base_end = log_base_start + LOG_7500_TOLSZ;
    log_start_ptr = log_base_start + offset_s;


    printk("[LOG]Starting %dbytes...\n", log_real_size);

    if (log_real_size < LOG_7500_TOLSZ) {
        log_print_ptr = log_base_start;
        size = log_real_size;
    } else {
        log_print_ptr = log_start_ptr;

search_again:
        while (*(unsigned char *)log_print_ptr != 0xa) {
            log_print_ptr++;
            offset++;
            if (log_print_ptr == log_base_end)
                log_print_ptr = log_base_start;
            if (offset >= log_real_size) {
                goto log_done;
            }
        }

        log_print_ptr++;
        offset++;
        if (log_print_ptr == log_base_end) {
            log_print_ptr = log_base_start;
            goto search_again;
        }

        if (offset >= log_real_size)
            goto log_done;

        size = log_real_size - offset;
    }

    log_slice1_ptr = log_print_ptr;
    if (size > (log_base_end - log_print_ptr)) {
        log_slice1_size = log_base_end - log_print_ptr;
        log_slice2_ptr = log_base_start;
        log_slice2_size = size - (log_base_end - log_print_ptr);
    } else {
        log_slice1_size = size;
        log_slice2_ptr = log_slice2_size = 0;
    }

    if (ecos_log_mode == GM_LOG_MODE_FILE) {
        sprintf(log_path, "%s/FC7500log.txt", defaul_path);
        audio_proc_log_write(log_slice1_ptr,log_slice1_size,log_slice2_ptr,log_slice2_size,log_path);
    } else if (ecos_log_mode == GM_LOG_MODE_CONSOLE) {
    }

log_done:
    __iounmap((void *)log_iomem);
    return 0;
}

void audio_proc_dump_fc7500_log(void)
{
    struct seq_file sfile;
    audio_proc_7500_log_show(&sfile, (void *)&sfile);
}

static int audio_proc_7500_logpath_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "FC7500 log path: %s\n", defaul_path);

    return 0;
}

static int audio_proc_7500_log_mode_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "Log Mode: 0(to file), 1(to console)\n");
    seq_printf(sfile, "\nFC7500 log mode: %d\n\n", ecos_log_mode);

    return 0;
}

static int audio_proc_7500_log_level_show(struct seq_file *sfile, void *v)
{
    // print the level bitmask

    seq_printf(sfile, "\nGM_LOG_QUIET         -> 0\n");
    seq_printf(sfile, "GM_LOG_ERROR         -> BIT_0\n");
    seq_printf(sfile, "GM_LOG_REC_PKT_FLOW  -> BIT_1\n");
    seq_printf(sfile, "GM_LOG_PB_PKT_FLOW   -> BIT_2\n");
    seq_printf(sfile, "GM_LOG_WARNING       -> BIT_3\n");
    seq_printf(sfile, "GM_LOG_DEBUG         -> BIT_4\n");
    seq_printf(sfile, "GM_LOG_DETAIL        -> BIT_5\n");
    seq_printf(sfile, "GM_LOG_CODEC         -> BIT_6\n");
    seq_printf(sfile, "GM_LOG_INIT          -> BIT_7\n");
    seq_printf(sfile, "GM_LOG_REC_DSR       -> BIT_8\n");
    seq_printf(sfile, "GM_LOG_PB_DSR        -> BIT_9\n");
    seq_printf(sfile, "GM_LOG_IO            -> BIT_10\n");
    seq_printf(sfile, "\nFC7500 log level: 0x%X\n\n", ecos_log_level);

    return 0;
}

static ssize_t audio_proc_7500_logpath_write(struct file *file, const char __user *buffer,
                                                  size_t count, loff_t *ppos)
{
    char value_str[MAX_PATH_WIDTH] = {'\0'};

    if(count >= MAX_PATH_WIDTH){
        printk("log path is too long(%d)\n",count);
        goto exit;
    }

    if(copy_from_user(value_str, buffer, count))
        goto exit;

    value_str[count] = '\0';

    memcpy(defaul_path , value_str,count);

    defaul_path[count] = '\0';

exit:
    return count;
}

void audio_proc_set_fc7500_log_level(unsigned int log_level)
{
    ecos_log_level = log_level;
    *((unsigned int *)log_buffer_vbase) = ecos_log_level;
}

static ssize_t audio_proc_7500_log_level_write(struct file *file, const char __user *buffer,
                                                        size_t count, loff_t *ppos)
{
    unsigned int log_level;
    char value_str[32] = {'\0'};

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%x\n", &log_level);

    ecos_log_level = log_level;
    *((unsigned int *)log_buffer_vbase) = ecos_log_level;

    enable_fc7500_log = true;

    return count;
}

static ssize_t audio_proc_7500_log_mode_write(struct file *file, const char __user *buffer,
                                                        size_t count, loff_t *ppos)
{
    unsigned int log_mode;
    char value_str[32] = {'\0'};

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &log_mode);

    ecos_log_mode = log_mode;

    return count;
}
#endif

static ssize_t audio_proc_log_level_write(struct file *file, const char __user *buffer,
                                                        size_t count, loff_t *ppos)
{
    unsigned int log_level;
    char value_str[32] = {'\0'};

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%x\n", &log_level);

    gm_log_level = log_level;

    return count;
}

static int audio_proc_log_level_show(struct seq_file *sfile, void *v)
{
    // print the level bitmask

    seq_printf(sfile, "\nGM_LOG_QUIET         -> 0\n");
    seq_printf(sfile, "GM_LOG_ERROR         -> BIT_0\n");
    seq_printf(sfile, "GM_LOG_REC_PKT_FLOW  -> BIT_1\n");
    seq_printf(sfile, "GM_LOG_PB_PKT_FLOW   -> BIT_2\n");
    seq_printf(sfile, "GM_LOG_WARNING       -> BIT_3\n");
    seq_printf(sfile, "GM_LOG_DEBUG         -> BIT_4\n");
    seq_printf(sfile, "GM_LOG_DETAIL        -> BIT_5\n");
    seq_printf(sfile, "GM_LOG_CODEC         -> BIT_6\n");
    seq_printf(sfile, "GM_LOG_INIT          -> BIT_7\n");
    seq_printf(sfile, "GM_LOG_REC_DSR       -> BIT_8\n");
    seq_printf(sfile, "GM_LOG_PB_DSR        -> BIT_9\n");
    seq_printf(sfile, "GM_LOG_IO            -> BIT_10\n");
    seq_printf(sfile, "\naudio log level: 0x%X\n\n", gm_log_level);

    return 0;
}

static int audio_proc_dump_reg_show(struct seq_file *sfile, void *v)
{
    unsigned int idx, ch, max_ch = 8;
    unsigned int io_base, ch_base = 0x100;

    seq_printf(sfile, "\nDMAC Reg (0x%08x)\n", DMAC_FTDMAC020_PA_BASE);
    io_base = (u32)audio_register.dmac_base;
    for (ch=0; ch<max_ch; ch++) {
        seq_printf(sfile, "\nChannel %d register\n", ch);
        for (idx=0; idx<6; idx++) {
            seq_printf(sfile, "offset[0x%x] = 0x%08x\n",
               (ch_base + (idx * 0x04)), (*(u32 *)(io_base + (ch_base + (idx * 0x04)))));
        }
        ch_base += 0x20;
    }
    seq_printf(sfile, "\n");

    // ssp0
    seq_printf(sfile, "SSP0 Reg (0x%08x):\n", SSP_FTSSP010_0_PA_BASE);
    io_base = (u32)(audio_register.ssp0_base);
    for (idx=0; idx<6; idx++) {
        seq_printf(sfile, "offset[0x%x] = 0x%08x\n", (idx * 0x04), (*(u32 *)(io_base + (idx * 0x04))));
    }

    // ssp1
    seq_printf(sfile, "\nSSP1 Reg (0x%08x):\n", SSP_FTSSP010_1_PA_BASE);
    io_base = (u32)(audio_register.ssp1_base);
    for (idx=0; idx<6; idx++) {
        seq_printf(sfile, "offset[0x%x] = 0x%08x\n", (idx * 0x04), (*(u32 *)(io_base + (idx * 0x04))));
    }

#if defined(CONFIG_PLATFORM_GM8210) || defined(CONFIG_PLATFORM_GM8287)
    // ssp2
    seq_printf(sfile, "\nSSP2 Reg (0x%08x):\n", SSP_FTSSP010_2_PA_BASE);
    io_base = (u32)(audio_register.ssp2_base);
    for (idx=0; idx<6; idx++) {
        seq_printf(sfile, "offset[0x%x] = 0x%08x\n", (idx * 0x04), (*(u32 *)(io_base + (idx * 0x04))));
    }
#endif

#if defined(CONFIG_PLATFORM_GM8210)
    // ssp3
    seq_printf(sfile, "\nSSP3 Reg (0x%08x):\n", SSP_FTSSP010_3_PA_BASE);
    io_base = (u32)(audio_register.ssp3_base);
    for (idx=0; idx<6; idx++) {
        seq_printf(sfile, "offset[0x%x] = 0x%08x\n", (idx * 0x04), (*(u32 *)(io_base + (idx * 0x04))));
    }
    // ssp4
    seq_printf(sfile, "\nSSP4 Reg (0x%08x):\n", SSP_FTSSP010_4_PA_BASE);
    io_base = (u32)(audio_register.ssp4_base);
    for (idx=0; idx<6; idx++) {
        seq_printf(sfile, "offset[0x%x] = 0x%08x\n", (idx * 0x04), (*(u32 *)(io_base + (idx * 0x04))));
    }
    // ssp5
    seq_printf(sfile, "\nSSP5 Reg (0x%08x):\n", SSP_FTSSP010_5_PA_BASE);
    io_base = (u32)(audio_register.ssp5_base);
    for (idx=0; idx<6; idx++) {
        seq_printf(sfile, "offset[0x%x] = 0x%08x\n", (idx * 0x04), (*(u32 *)(io_base + (idx * 0x04))));
    }
#endif

    seq_printf(sfile, "\n");

    return 0;
}

static int audio_proc_codec_time_adjust_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_codec_time_adjust_show, PDE(inode)->data);
}

static int audio_proc_queued_time_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_queued_time_show, PDE(inode)->data);
}

static int audio_proc_pcm_flag_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_pcm_data_show, PDE(inode)->data);
}
static int audio_proc_pcm_save_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_pcm_save_show, PDE(inode)->data);
}
static int audio_proc_resample_save_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_resample_save_show, PDE(inode)->data);
}
static int audio_proc_nr_threshold_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_nr_threshold_show, PDE(inode)->data);
}
static int audio_proc_flow_dbg_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_flow_dbg_show, PDE(inode)->data);
}
static int audio_proc_play_buf_dbg_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_play_buf_dbg_show, PDE(inode)->data);
}
static int audio_proc_record_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_record_timeout_show, PDE(inode)->data);
}
static int audio_proc_playback_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_playback_timeout_show, PDE(inode)->data);
}
static int audio_proc_record_overrun_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_record_overrun_show, PDE(inode)->data);
}
static int audio_proc_rec_req_dma_chan_failed_cnt_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_rec_req_dma_chan_failed_cnt_show, PDE(inode)->data);
}
static int audio_proc_play_req_dma_chan_failed_cnt_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_play_req_dma_chan_failed_cnt_show, PDE(inode)->data);
}
static int audio_proc_playback_underrun_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_playback_underrun_show, PDE(inode)->data);
}

static int audio_proc_runtime_values_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_runtime_values_show, PDE(inode)->data);
}

static int audio_proc_buffer_chase_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_buffer_chase_show, PDE(inode)->data);
}

static int audio_proc_sync_change_level_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_sync_change_level_show, PDE(inode)->data);
}

static int audio_proc_ecos_state_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_ecos_state_show, PDE(inode)->data);
}

#if defined(CONFIG_PLATFORM_GM8210)
static int audio_proc_7500_log_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_7500_log_show, PDE(inode)->data);
}

static int audio_proc_7500_logpath_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_7500_logpath_show, PDE(inode)->data);
}

static int audio_proc_7500_log_mode_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_7500_log_mode_show, PDE(inode)->data);
}

static int audio_proc_7500_log_level_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_7500_log_level_show, PDE(inode)->data);
}
#endif

static int audio_proc_log_level_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_log_level_show, PDE(inode)->data);
}

static int audio_proc_dump_reg_open(struct inode *inode, struct file *file)
{
    return single_open(file, audio_proc_dump_reg_show, PDE(inode)->data);
}

/*
static struct file_operations audio_proc_vg_info_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_vg_info_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};
*/
static struct file_operations audio_proc_input_tab_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_input_tab_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_codec_time_adjust_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_codec_time_adjust_open,
    .write  = audio_proc_codec_time_adjust_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_queued_time_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_queued_time_open,
    .write  = audio_proc_queued_time_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_pcm_data_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_pcm_flag_open,
    .write  = audio_proc_pcm_flag_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_pcm_save_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_pcm_save_open,
    .write  = audio_proc_pcm_save_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_resample_save_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_resample_save_open,
    .write  = audio_proc_resample_save_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_nr_threshold_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_nr_threshold_open,
    .write  = audio_proc_nr_threshold_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_flow_dbg_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_flow_dbg_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_play_buf_dbg_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_play_buf_dbg_open,
    .write  = audio_proc_play_buf_dbg_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_record_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_record_timeout_open,
    .write  = audio_proc_record_timeout_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_playback_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_playback_timeout_open,
    .write  = audio_proc_playback_timeout_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_record_overrun_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_record_overrun_open,
    .write  = audio_proc_record_overrun_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_rec_req_dma_chan_failed_cnt_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_rec_req_dma_chan_failed_cnt_open,
    .write  = audio_proc_rec_req_dma_chan_failed_cnt_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_play_req_dma_chan_failed_cnt_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_play_req_dma_chan_failed_cnt_open,
    .write  = audio_proc_play_req_dma_chan_failed_cnt_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_playback_underrun_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_playback_underrun_open,
    .write  = audio_proc_playback_underrun_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_runtime_values_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_runtime_values_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_buffer_chase_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_buffer_chase_open,
    .write  = audio_proc_buffer_chase_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_sync_change_level_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_sync_change_level_open,
    .write  = audio_proc_sync_change_level_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_proc_ecos_state_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_ecos_state_open,
    .write  = audio_proc_ecos_state_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};
#if defined(CONFIG_PLATFORM_GM8210)
static struct file_operations audio_7500_log_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_7500_log_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};
static struct file_operations audio_7500_logpath_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_7500_logpath_open,
    .write  = audio_proc_7500_logpath_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};
static struct file_operations audio_7500_log_mode_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_7500_log_mode_open,
    .write  = audio_proc_7500_log_mode_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};
static struct file_operations audio_7500_log_level_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_7500_log_level_open,
    .write  = audio_proc_7500_log_level_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};
#endif

static struct file_operations audio_log_level_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_log_level_open,
    .write  = audio_proc_log_level_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations audio_dump_reg_ops = {
    .owner  = THIS_MODULE,
    .open   = audio_proc_dump_reg_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

int audio_proc_init(void)
{
    int ret = 0;

    pcm_show_flag = (int *)dma_alloc_writecombine(NULL, sizeof(int), &pcm_show_flag_paddr, GFP_KERNEL);
    pcm_save_flag = (int *)dma_alloc_writecombine(NULL, sizeof(int), &pcm_save_flag_paddr, GFP_KERNEL);
    resample_save_flag = (int *)dma_alloc_writecombine(NULL, sizeof(int), &resample_save_paddr , GFP_KERNEL);
    play_buf_dbg = (int *)dma_alloc_writecombine(NULL, sizeof(int), &play_buf_dbg_paddr, GFP_KERNEL);
    rec_dbg_stage = (u32 *)dma_alloc_writecombine(NULL, sizeof(u32), &rec_dbg_stage_paddr, GFP_KERNEL);
    play_dbg_stage = (u32 *)dma_alloc_writecombine(NULL, sizeof(u32), &play_dbg_stage_paddr, GFP_KERNEL);
    nr_threshold = (u32 *)dma_alloc_writecombine(NULL, sizeof(u32), &nr_threshold_paddr, GFP_KERNEL);
    ecos_state_show_flag = (int *)dma_alloc_writecombine(NULL, sizeof(int), &ecos_state_show_flag_paddr, GFP_KERNEL);

    /* audio_info */
    audio_proc_vg_info = audio_proc_create_entry("au_info", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_vg_info) {
        printk("create proc node 'au_info' failed!\n");
        ret = -EINVAL;
        goto end;
    }
    //audio_proc_vg_info->proc_fops = &audio_proc_vg_info_ops;
    audio_proc_vg_info->read_proc = (read_proc_t *)audio_proc_vg_info_read;

    /* audio codec root */
    audio_proc_input_root = audio_proc_create_entry("au_codec", S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_input_root) {
        printk("create proc node 'au_codec' failed!\n");
        ret = -EINVAL;
        goto err_input;
    }

    /* audio queued time */
    audio_proc_queued_time = audio_proc_create_entry("au_queued_time", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_queued_time) {
        printk("create proc node 'au_queued_time' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_queued_time->proc_fops = &audio_proc_queued_time_ops;

    /* audio pcm data debug flag */
    audio_proc_pcm_data_debug = audio_proc_create_entry("au_pcm_data_debug", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_pcm_data_debug) {
        printk("create proc node 'au_pcm_data_debug' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_pcm_data_debug->proc_fops = &audio_proc_pcm_data_ops;

    /* audio pcm save debug flag */
    audio_proc_pcm_save_debug = audio_proc_create_entry("au_pcm_save_debug", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_pcm_save_debug) {
        printk("create proc node 'au_pcm_save_debug' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_pcm_save_debug->proc_fops = &audio_proc_pcm_save_ops;

    /* audio resample save debug flag */
    audio_proc_resample_save_debug = audio_proc_create_entry("au_resample_save_debug", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_resample_save_debug) {
        printk("create proc node 'au_resample_save_debug' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_resample_save_debug->proc_fops = &audio_proc_resample_save_ops;

    /* audio nr threshold */
    audio_proc_nr_threashold = audio_proc_create_entry("au_nr_threshold", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_nr_threashold) {
        printk("create proc node 'au_nr_threshold' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_nr_threashold->proc_fops = &audio_proc_nr_threshold_ops;

    /* audio flow stage debug */
    audio_proc_flow_stage_debug = audio_proc_create_entry("au_flow_debug", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_flow_stage_debug) {
        printk("create proc node 'au_flow_debug' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_flow_stage_debug->proc_fops = &audio_proc_flow_dbg_ops;

    /* audio play buffer debug */
    audio_proc_play_buf_debug = audio_proc_create_entry("au_play_buf_debug", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_play_buf_debug) {
        printk("create proc node 'au_play_buf_debug' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_play_buf_debug->proc_fops = &audio_proc_play_buf_dbg_ops;

    /* audio record timeout */
    audio_proc_record_timeout = audio_proc_create_entry("au_record_timeout", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_record_timeout) {
        printk("create proc node 'au_record_timeout' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_record_timeout->proc_fops = &audio_proc_record_timeout_ops;

    /* audio playback timeout */
    audio_proc_playback_timeout = audio_proc_create_entry("au_playback_timeout", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_playback_timeout) {
        printk("create proc node 'au_playback_timeout' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_playback_timeout->proc_fops = &audio_proc_playback_timeout_ops;

    /* audio record dma buffer overrun */
    audio_proc_record_overrun = audio_proc_create_entry("au_record_overrun_cnt", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_record_overrun) {
        printk("create proc node 'au_record_overrun_cnt' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_record_overrun->proc_fops = &audio_proc_record_overrun_ops;

    /* audio record request dma channel failed count */
    audio_proc_rec_req_dma_chan_failed_cnt = audio_proc_create_entry("au_rec_req_dma_chan_failed_cnt", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_rec_req_dma_chan_failed_cnt) {
        printk("create proc node 'au_rec_req_dma_chan_failed_cnt' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_rec_req_dma_chan_failed_cnt->proc_fops = &audio_proc_rec_req_dma_chan_failed_cnt_ops;

    /* audio playback dma buffer underrun */
    audio_proc_playback_underrun = audio_proc_create_entry("au_playback_underrun_cnt", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_playback_underrun) {
        printk("create proc node 'au_playback_underrun_cnt' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_playback_underrun->proc_fops = &audio_proc_playback_underrun_ops;

    /* audio playback request dma channel failed count */
    audio_proc_play_req_dma_chan_failed_cnt = audio_proc_create_entry("au_play_req_dma_chan_failed_cnt", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_play_req_dma_chan_failed_cnt) {
        printk("create proc node 'au_play_req_dma_chan_failed_cnt' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_play_req_dma_chan_failed_cnt->proc_fops = &audio_proc_play_req_dma_chan_failed_cnt_ops;

    /* audio buffer chase */
    audio_proc_buffer_chase = audio_proc_create_entry("au_buffer_chase", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_buffer_chase) {
        printk("create proc node 'au_buffer_chase' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_buffer_chase->proc_fops = &audio_proc_buffer_chase_ops;

    /* audio buffer chase */
    audio_proc_sync_change_level = audio_proc_create_entry("au_sync_change_level", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_sync_change_level) {
        printk("create proc node 'au_sync_change_level' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_sync_change_level->proc_fops = &audio_proc_sync_change_level_ops;

    /* audio codec timestamp adjust */
    audio_proc_runtime_value = audio_proc_create_entry("values", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_runtime_value) {
        printk("create proc node 'value' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }

    audio_proc_runtime_value->proc_fops = &audio_proc_runtime_values_ops;

#if defined(CONFIG_PLATFORM_GM8210)
    memset(&g_ft75_log_info,0x0 ,sizeof(g_ft75_log_info));
    audio_7500_log =  audio_proc_create_entry("au_dump_fc7500_log", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_runtime_value) {
        printk("create proc node 'au_dump_fc7500_log' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_7500_log->proc_fops =  &audio_7500_log_ops;

    audio_7500_path =  audio_proc_create_entry("au_fc7500_log_path", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_runtime_value) {
        printk("create proc node 'au_fc7500_log_path' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_7500_path->proc_fops = &audio_7500_logpath_ops;

    audio_7500_log_mode =  audio_proc_create_entry("au_fc7500_log_mode", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_runtime_value) {
        printk("create proc node 'au_fc7500_log_mode' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_7500_log_mode->proc_fops = &audio_7500_log_mode_ops;

    audio_7500_log_level = audio_proc_create_entry("au_fc7500_log_level", S_IRUGO|S_IXUGO, NULL);
    if (!audio_7500_log_level) {
        printk("create proc node 'au_fc7500_log_level' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_7500_log_level->proc_fops = &audio_7500_log_level_ops;
#endif

    audio_proc_log_level = audio_proc_create_entry("au_log_level", S_IRUGO|S_IXUGO, NULL);
    if (!audio_proc_log_level) {
        printk("create proc node 'au_log_level' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_log_level->proc_fops = &audio_log_level_ops;

    audio_proc_dump_reg = audio_proc_create_entry("au_dump_reg", S_IRUGO|S_IXUGO, NULL);
    if (!audio_proc_dump_reg) {
        printk("create proc node 'au_dump_reg' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_dump_reg->proc_fops = &audio_dump_reg_ops;

    /* audio codec table */
    audio_proc_input_tab = audio_proc_create_entry("au_table", S_IRUGO|S_IXUGO, audio_proc_input_root);
    if(!audio_proc_input_tab) {
        printk("create proc node 'au_table' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_input_tab->proc_fops = &audio_proc_input_tab_ops;

    /* audio codec timestamp adjust */
    audio_proc_codec_time_adjust = audio_proc_create_entry("au_time_adjust", S_IRUGO|S_IXUGO,
                                                           audio_proc_input_root);
    if(!audio_proc_codec_time_adjust) {
        printk("create proc node 'au_time_adjust' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_codec_time_adjust->proc_fops = &audio_proc_codec_time_adjust_ops;

    /* audio fc7500 state debug flag */
    audio_proc_ecos_state_debug = audio_proc_create_entry("au_ecos_state_debug", S_IRUGO|S_IXUGO, NULL);
    if(!audio_proc_ecos_state_debug) {
        printk("create proc node 'au_ecos_state_debug' failed!\n");
        ret = -EINVAL;
        goto err_tab;
    }
    audio_proc_ecos_state_debug->proc_fops = &audio_proc_ecos_state_ops;

end:
    return ret;

err_tab:
    audio_proc_remove_entry(NULL, audio_proc_input_root);

err_input:
    audio_proc_remove_entry(NULL, audio_proc_vg_info);
    return ret;
}
EXPORT_SYMBOL(audio_proc_init);

void audio_proc_remove(void)
{
    dma_free_writecombine(NULL, sizeof(int), (void *)pcm_show_flag, pcm_show_flag_paddr);
    dma_free_writecombine(NULL, sizeof(int), (void *)pcm_save_flag, pcm_save_flag_paddr);
    dma_free_writecombine(NULL, sizeof(int), (void *)resample_save_flag, resample_save_paddr);
    dma_free_writecombine(NULL, sizeof(int), (void *)play_buf_dbg, play_buf_dbg_paddr);
    dma_free_writecombine(NULL, sizeof(int), (void *)rec_dbg_stage, rec_dbg_stage_paddr);
    dma_free_writecombine(NULL, sizeof(int), (void *)play_dbg_stage, play_dbg_stage_paddr);
    dma_free_writecombine(NULL, sizeof(int), (void *)nr_threshold, nr_threshold_paddr);
    dma_free_writecombine(NULL, sizeof(int), (void *)ecos_state_show_flag, ecos_state_show_flag_paddr);

    if(audio_proc_vg_info)
        audio_proc_remove_entry(NULL, audio_proc_vg_info);
    if(audio_proc_queued_time)
        audio_proc_remove_entry(NULL, audio_proc_queued_time);
    if(audio_proc_pcm_data_debug)
        audio_proc_remove_entry(NULL, audio_proc_pcm_data_debug);
    if(audio_proc_pcm_save_debug)
        audio_proc_remove_entry(NULL, audio_proc_pcm_save_debug);
    if(audio_proc_resample_save_debug)
        audio_proc_remove_entry(NULL, audio_proc_resample_save_debug);
    if(audio_proc_nr_threashold)
        audio_proc_remove_entry(NULL, audio_proc_nr_threashold);
    if(audio_proc_flow_stage_debug)
        audio_proc_remove_entry(NULL, audio_proc_flow_stage_debug);
    if(audio_proc_play_buf_debug)
        audio_proc_remove_entry(NULL, audio_proc_play_buf_debug);
    if(audio_proc_record_timeout)
        audio_proc_remove_entry(NULL, audio_proc_record_timeout);
    if(audio_proc_playback_timeout)
        audio_proc_remove_entry(NULL, audio_proc_playback_timeout);
    if(audio_proc_record_overrun)
        audio_proc_remove_entry(NULL, audio_proc_record_overrun);
    if(audio_proc_rec_req_dma_chan_failed_cnt)
        audio_proc_remove_entry(NULL, audio_proc_rec_req_dma_chan_failed_cnt);
    if(audio_proc_playback_underrun)
        audio_proc_remove_entry(NULL, audio_proc_playback_underrun);
    if(audio_proc_play_req_dma_chan_failed_cnt)
        audio_proc_remove_entry(NULL, audio_proc_play_req_dma_chan_failed_cnt);
    if(audio_proc_runtime_value)
        audio_proc_remove_entry(NULL, audio_proc_runtime_value);
    if(audio_proc_buffer_chase)
        audio_proc_remove_entry(NULL, audio_proc_buffer_chase);
    if(audio_proc_sync_change_level)
        audio_proc_remove_entry(NULL, audio_proc_sync_change_level);
    if(audio_proc_ecos_state_debug)
        audio_proc_remove_entry(NULL, audio_proc_ecos_state_debug);
    if(audio_proc_log_level)
        audio_proc_remove_entry(NULL, audio_proc_log_level);
    if(audio_proc_dump_reg)
        audio_proc_remove_entry(NULL, audio_proc_dump_reg);
#if defined(CONFIG_PLATFORM_GM8210)
    if(audio_7500_log)
        audio_proc_remove_entry(NULL, audio_7500_log);        
    if(audio_7500_path)
        audio_proc_remove_entry(NULL, audio_7500_path);        
    if(audio_7500_log_mode)
        audio_proc_remove_entry(NULL, audio_7500_log_mode);        
    if(audio_7500_log_level)
        audio_proc_remove_entry(NULL, audio_7500_log_level);        
#endif

    if(audio_proc_input_root) {
        if(audio_proc_input_tab)
            audio_proc_remove_entry(audio_proc_input_root, audio_proc_input_tab);
        if(audio_proc_codec_time_adjust)
            audio_proc_remove_entry(audio_proc_input_root, audio_proc_codec_time_adjust);

        audio_proc_remove_entry(NULL, audio_proc_input_root);
        audio_proc_input_root = NULL;
    }
}
EXPORT_SYMBOL(audio_proc_remove);

void audio_drv_set_pcm_data_flag(bool is_on)
{
    *pcm_show_flag = is_on;
}
EXPORT_SYMBOL(audio_drv_set_pcm_data_flag);

int *audio_drv_get_pcm_data_flag(void)
{
    return pcm_show_flag;
}
EXPORT_SYMBOL(audio_drv_get_pcm_data_flag);

void audio_drv_set_ecos_state_flag(bool is_on)
{
    *ecos_state_show_flag = is_on;
}
EXPORT_SYMBOL(audio_drv_set_ecos_state_flag);

int *audio_drv_get_ecos_state_flag(void)
{
    return ecos_state_show_flag;
}
EXPORT_SYMBOL(audio_drv_get_ecos_state_flag);

void audio_drv_set_pcm_save_flag(bool is_on)
{
    *pcm_save_flag = is_on;
}
EXPORT_SYMBOL(audio_drv_set_pcm_save_flag);

int *audio_drv_get_pcm_save_flag(void)
{
    return pcm_save_flag;
}
EXPORT_SYMBOL(audio_drv_get_pcm_save_flag);

void audio_drv_set_resample_save_flag(bool is_on)
{
    *resample_save_flag = is_on;
}
EXPORT_SYMBOL(audio_drv_set_resample_save_flag);

int *audio_drv_get_resample_save_flag(void)
{
    return resample_save_flag;
}
EXPORT_SYMBOL(audio_drv_get_resample_save_flag);

void audio_drv_set_play_buf_dbg(bool is_on)
{
    *play_buf_dbg = is_on;
}
EXPORT_SYMBOL(audio_drv_set_play_buf_dbg);

int *audio_drv_get_play_buf_dbg(void)
{
    return play_buf_dbg;
}
EXPORT_SYMBOL(audio_drv_get_play_buf_dbg);

int *audio_drv_get_nr_threshold(void)
{
    return nr_threshold;
}
EXPORT_SYMBOL(audio_drv_get_nr_threshold);

u32 *audio_flow_get_rec_stage(void) {return rec_dbg_stage;}
EXPORT_SYMBOL(audio_flow_get_rec_stage);
u32 *audio_flow_get_play_stage(void) {return play_dbg_stage;}
EXPORT_SYMBOL(audio_flow_get_play_stage);

unsigned long audio_proc_get_pcm_show_flag_paddr(void) {return pcm_show_flag_paddr;}
EXPORT_SYMBOL(audio_proc_get_pcm_show_flag_paddr);
unsigned long audio_proc_get_play_buf_dbg_paddr(void) {return play_buf_dbg_paddr;}
EXPORT_SYMBOL(audio_proc_get_play_buf_dbg_paddr);
unsigned long audio_proc_get_rec_dbg_stage_paddr(void) {return rec_dbg_stage_paddr;}
EXPORT_SYMBOL(audio_proc_get_rec_dbg_stage_paddr);
unsigned long audio_proc_get_play_dbg_stage_paddr(void) {return play_dbg_stage_paddr;}
EXPORT_SYMBOL(audio_proc_get_play_dbg_stage_paddr);
unsigned long audio_proc_get_nr_threshold_paddr(void) {return nr_threshold_paddr;}
EXPORT_SYMBOL(audio_proc_get_nr_threshold_paddr);
