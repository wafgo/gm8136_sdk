#include <linux/module.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/bitops.h>
#include <linux/bitmap.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/ratelimit.h>
#include <mach/fmem.h>
#include <mach/ftpmu010.h>
#include "frammap_if.h"
#include "audio_vg.h"
#include "audio_proc.h"
#include "audio_flow.h"
#include "ssp_dma/ssp_dma.h"

#undef AUDIO_CODEC
#define AUDIO_CODEC
#if defined(AUDIO_CODEC)
/* general enc/dec header */
#include "audio_main.h"

/* aac enc/dec header */
#include "gmaac_enc_api.h"
#include "gmaac_dec_api.h"

/* adpcm enc/dec header */
#include "adpcm.h"
#include "ima_adpcm_enc_api.h"
#include "ima_adpcm_dec_api.h"

/* a/u law codec API */
#include "g711s.h"
#include "filter.h"
#endif /* AUDIO_CODEC */

/* audio resample header */
//#include "GM_SRC_0.1.1/resample.h"
#include "resample.h"

#if (HZ == 1000)
    #define get_gm_jiffies()                (jiffies)
#else
    #include <mach/gm_jiffies.h>
#endif

#include "log.h"    //include log system "printm","damnit"...

/* Debug usage */
static u32 dbg_beginTick = 0, dbg_endTick = 0;
#define AU_FLOW_DBG
#undef AU_FLOW_DBG
#ifdef AU_FLOW_DBG
#define DBG(fmt,args...)    printk(KERN_INFO"<AUDIO_AP> "fmt, ## args)
#define BEGIN_TICK()        do { dbg_beginTick = jiffies; } while (0)
#define END_TICK()          do { dbg_endTick = jiffies; } while (0)
#else
#define DBG(fmt,args...)    no_printk(KERN_DEBUG"<AUDIO_AP> "fmt, ## args)
#define BEGIN_TICK()        do {} while (0)
#define END_TICK()          do {} while (0)
#endif
#define TICK_PASSBY(name)   do { DBG("%s: ticks = %d\n", name, dbg_endTick - dbg_beginTick); } while (0)

typedef enum _code_type {
    AU_TYPE_NONE = 0,
    AU_TYPE_PCM,
    AU_TYPE_AAC,
    AU_TYPE_ADPCM,
    AU_TYPE_ALAW,
    AU_TYPE_ULAW,
    AU_TYPE_MAX,
} code_type;

#define PROCESS_SUCCESS 0
#define PROCESS_FAILED  1

#define CHECK_PLL
//#undef CHECK_PLL

/* audio control variable */
static int audio_handle;
static struct task_struct *live_sound = NULL;
static int output_ch[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
static bool is_live_on = false;
static bool is_record_on = false;
static bool live_in_ssp[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
static DEFINE_MUTEX(record_mutex);

#if defined(AUDIO_CODEC)
/* audio encode variable */
static unsigned int enc_len[AUDIO_MAX_CHAN];
static short enc_input_buf[AUDIO_MAX_CHAN][2*AAC_FRAME_SZ];
static unsigned char enc_output_buf[AUDIO_MAX_CHAN][2*AAC_FRAME_SZ];
static bool is_aac_enc_init = false;
static GMAAC_INFORMATION aac_enc_info;
static int aac_nChannels = 0;
static bool is_adpcm_enc_init = false;
static IMA_state_t *adpcm_state[AUDIO_MAX_CHAN][2];
static short adpcm_curr_ch_cnt = 0;
static bool is_g711_enc_init = false;
static short g711_curr_ch_cnt = 0;
static INPARAM *inparam_enc[AUDIO_MAX_CHAN];
static unsigned int g711_curr_enc_type = 0;

/* audio decode variable */
static bool is_aac_dec_init = false;
static GMAACDecHandle aac_dec_hdl;
static GMAACDecInfo *aac_dec_info = NULL;
static aac_buffer aac_dec_buf;
static AUDIO_CODEC_SETTING adpcm_codec_setting;
static bool is_adpcm_dec_init = false;
static AUDIO_CODEC_SETTING g711_dec_setting;
static short g711_dec_output_buf[IMA_BLK_SZ_MONO];
static unsigned char g711_dec_input_buf[AULAW_BLK_SZ];
static bool is_g711_dec_init = false;
static INPARAM *inparam_dec;
static unsigned int g711_curr_dec_type = 0;
#endif

/* resample related */
#define MAX_PLAY_SZ         4096
#define MAX_RESAMPLE_SZ     (MAX_ENCODE_RATIO * MAX_RESAMPLE_RATIO * MAX_PLAY_SZ)
#define AVG_RESAMPLE_SZ     (MAX_ENCODE_RATIO * AVG_RESAMPLE_RATIO * MAX_PLAY_SZ)
#define MUTE_BUF_SZ         2048
#define RESAMPLE_FILE_PATH  "/mnt/nfs/resample.pcm"
static unsigned char *resample_buffer[SSP_MAX_USED];
static unsigned int resample_size[SSP_MAX_USED];
#if defined(MEMCPY_DMA)
static unsigned char *avg_resample_buf[SSP_MAX_USED - 1];
static unsigned char *max_resample_buf = NULL;
static dma_addr_t avg_resample_buf_paddr[SSP_MAX_USED - 1];
static dma_addr_t max_resample_buf_paddr;
#else /* PIO mode */
static unsigned char avg_resample_buf[SSP_MAX_USED - 1][AVG_RESAMPLE_SZ];
static unsigned char max_resample_buf[MAX_RESAMPLE_SZ];
#endif

//static const unsigned char silence_buf[MUTE_BUF_SZ] = {0};
unsigned char *silence_buf = NULL;
dma_addr_t silence_buf_paddr;

static int set_resample_play(void *job, unsigned int input_sz, void *input_buf);

#if !defined(AUDIO_CODEC)
typedef struct {
  short   encode_error;
  short   decode_error;
  short   encode_audio_type;	//0: aac 1: adpcm
  short   decode_audio_type;	//0: aac 1: adpcm
  short   decode_mode;          //0: auto, 1: aac, 2:adpcm
  short   EncMultiChannel;
} AUDIO_CODEC_SETTING;
#endif
typedef struct _codec_funs {
    int (*encode)(unsigned int one_channel_data_size, AUDIO_CODEC_SETTING *codec_setting,
                  unsigned int *input_buffer, unsigned int *output_buffer, unsigned int *output_length);
    int (*decode)(void *audio_job, void *input_buffer, unsigned int input_buffer_length);
} codec_funs_t;

/* codec functions */
static int gm_pcm_enc(unsigned int, AUDIO_CODEC_SETTING *, unsigned int *, unsigned int *, unsigned int *);
static int gm_pcm_dec(void *, void *, unsigned int);
#if defined(AUDIO_CODEC)
static int gm_aac_enc(unsigned int, AUDIO_CODEC_SETTING *, unsigned int *, unsigned int *, unsigned int *);
static int gm_aac_dec(void *, void *, unsigned int);
static int gm_adpcm_enc(unsigned int, AUDIO_CODEC_SETTING *, unsigned int *, unsigned int *, unsigned int *);
static int gm_adpcm_dec(void *, void *, unsigned int);
static int gm_g711_enc(unsigned int, AUDIO_CODEC_SETTING *, unsigned int *, unsigned int *, unsigned int *);
static int gm_g711_dec(void *, void *, unsigned int);
#endif

static codec_funs_t gm_codec_funs[AU_TYPE_MAX] = {
 /* {encode,           decode} */
    {NULL,             NULL},
    {gm_pcm_enc,    gm_pcm_dec},
#if defined(AUDIO_CODEC)
    {gm_aac_enc,    gm_aac_dec},
    {gm_adpcm_enc,  gm_adpcm_dec},
    {gm_g711_enc,   gm_g711_dec},
    {gm_g711_enc,   gm_g711_dec}
#endif
};

#define LIVE_CH_SZ          (1 * 1024)
static unsigned live_len[AUDIO_SSP_CHAN];
static unsigned live_ofs[AUDIO_SSP_CHAN];
#if defined(MEMCPY_DMA)
static dma_addr_t live_buf_paddr = 0;
static unsigned char *live_buf_data = NULL;
#else /* PIO mode */
static unsigned char live_buf_data[LIVE_CH_SZ];
#endif
static unsigned int live_chan = 0;

static int record_audio(int ssp_idx, struct audio_job_t *job);

static int audio_live_thread(void *data)
{
    int i, ssp_idx = -1;
    struct audio_job_t live_job;

    memset(&live_job, 0, sizeof(struct audio_job_t));

    for (i = 0;i < AUDIO_SSP_CHAN;i ++) {
        live_len[i] = LIVE_CH_SZ;
        live_ofs[i] = (unsigned)live_buf_data;
    }

    for (i = 0;i < SUPPORT_TYPE;i ++) {
        int j;

        live_job.in_pa[i] = (u32)live_buf_data;
        live_job.in_size[i] = LIVE_CH_SZ;

        for (j = 0;j < AUDIO_MAX_CHAN;j ++) {
            live_job.data_offset[i][j] = (u32)live_buf_data;
            live_job.data_length[i][j] = LIVE_CH_SZ;
        }
    }

    live_job.codec_type = AU_TYPE_PCM;
    live_job.sample_rate = DEFAULT_SAMPLE_RATE;
    live_job.resample_ratio = 100;

    while (!kthread_should_stop()) {
        int *ssp_chan = au_get_ssp_chan();
        int *live_on_ssp = au_get_live_on_ssp();

        live_job.chan_bitmap = (1 << live_chan);

        ssp_idx = ssp_and_chan_translate(live_chan, true);

        if (au_get_ssp_chan()[ssp_idx] > 1)
            live_job.channel_type = STEREO_CHAN;
        else
            live_job.channel_type = MONO_CHAN;

        if (record_audio(ssp_idx, &live_job) != PROCESS_SUCCESS)
            printk("%s: record pcm failed\n", __func__);

        if (ssp_chan[ssp_idx] == 1 && DMA_ALIGN_CHECK(LIVE_CH_SZ)) /* single chan means data copy by DMA */
            fmem_dcache_sync(live_buf_data, LIVE_CH_SZ, DMA_TO_DEVICE);

        live_job.ssp_bitmap = 0;

        for (ssp_idx = 0;ssp_idx < SSP_MAX_USED;ssp_idx ++) {
            if (live_on_ssp[ssp_idx]) {
                live_job.ssp_bitmap |= (1 << ssp_idx);
            }
        }

        if (set_resample_play((void *)&live_job, LIVE_CH_SZ, (void *)live_buf_data))
            printk("Failed live sound\n");
    }

    return 0;
}

static int record_audio(int ssp_idx, struct audio_job_t *job)
{
    ssp_buf_t ctrl_buf;
    int bufSize = 0, bufIdx = 0, *ssp_chan = au_get_ssp_chan();
    u32 wait_cnt = 0;
    int *pcm_show_flag = audio_drv_get_pcm_data_flag();
    u32 job_id = 0;

    if (job->data)
        job_id = ((job_item_t *)(job->data))->job_id;

    /* decide PCM buffer: in_pa[bufIdx] */
    if (((job->codec_type & BITMASK(16, 16)) >> 16) > AU_TYPE_PCM)
        bufIdx = 1;

    /* total data size grabbed from SSP = (num of chan in one SSP) * (single channel buffer size) */
    //bufSize = ssp_chan[ssp_idx] * job->data_length[bufIdx][0];
    bufSize = job->data_length[bufIdx][0];

    /* set ctrl setting for audio_drv */
    if (job->in_pa[bufIdx] && job->in_size[bufIdx]) {
        int i, total_chan_cnt = 0;
        memset(&ctrl_buf.buf_pos, 0, AUDIO_SSP_CHAN * sizeof(unsigned));

        /* get ssp chan bits bitmap */
        for (i = 0;i < ssp_idx;i ++) {
#if defined(CONFIG_PLATFORM_GM8287)
            if ((au_get_ssp_num())[i] == 2) /* ssp2 is connected to HDMI without Rx */
                continue;
#endif
            total_chan_cnt += ssp_chan[i];
        }
        ctrl_buf.bitmap = (job->chan_bitmap & BITMASK(total_chan_cnt, ssp_chan[ssp_idx])) >> total_chan_cnt;

        /* one job data buffer stored several channel data from ADC */
        ctrl_buf.length = (unsigned *)((unsigned)job->data_length[bufIdx] + total_chan_cnt * sizeof(unsigned int));
        ctrl_buf.offset = (unsigned *)((unsigned)job->data_offset[bufIdx] + total_chan_cnt * sizeof(unsigned int));
        ctrl_buf.kbuf_ptr = (void *)(ctrl_buf.offset[0]);

        ctrl_buf.buf_ptr = frm_va2pa((u32)ctrl_buf.kbuf_ptr);

        ctrl_buf.ssp_idx = ssp_idx;
        ctrl_buf.is_live = false;

        ctrl_buf.sample_rate = job->sample_rate;
        ctrl_buf.time_stamp = 0;

        if (ssp_chan[ssp_idx] > 1)
            ctrl_buf.channel_type = STEREO_CHAN;
        else
            ctrl_buf.channel_type = job->channel_type;
    } else {
        printk("%s: NULL input buffer\n", __func__);
        return PROCESS_FAILED;
    }

    /* wait_cnt is used for no pending job when hw is stopped or died. */
    wait_cnt = 3 * 1000 * job->data_length[bufIdx][0] / job->sample_rate;

    GM_LOG(GM_LOG_REC_PKT_FLOW, "ssp%d, job(%d) start read data from HW, wait_cnt(%d)\n", ssp_idx, job_id, wait_cnt);

    mutex_lock(&record_mutex);

    /* Get channel 0~15 PCM data from DMA */
    while (bufSize > 0 && wait_cnt) {
        u32 recSize = audio_gm_read(&audio_handle, (void *)&ctrl_buf, &bufSize);
        if (recSize) {
            bufSize -= recSize;
        } else {
            msleep(1);
            wait_cnt --;
        }
    }

    mutex_unlock(&record_mutex);

    GM_LOG(GM_LOG_REC_PKT_FLOW, "ssp%d, job(%d) read data from HW done, wait_cnt(%d)\n", ssp_idx, job_id, wait_cnt);

    data_dump((u16 *)job->data_offset[bufIdx][0], 8, "jobD", *pcm_show_flag);

    if (wait_cnt) {
#if 1
        job->time_sync.jiffies_fa626 = ctrl_buf.time_stamp;
        //printk("job->time_sync.jiffies_fa626 = %u\n",job->time_sync.jiffies_fa626);
#else
        job->time_sync.jiffies_fa626 = ctrl_buf.buf_size / (get_gm_jiffies() / 1000);
#endif        
    } else {
        printk_ratelimited("%s: timeout!\n", __func__);
        job->status = JOB_STS_ENC_TIMEOUT;
        job->ssp_chan = ssp_idx;
        job->time_sync.jiffies_fa626 = 0;
    }

    return PROCESS_SUCCESS;
}

static int gm_pcm_enc(unsigned int pcm_data_sz,
                          AUDIO_CODEC_SETTING *codec_setting,
                          unsigned int *input_buf,
                          unsigned int *output_buf,
                          unsigned int *output_len)
{
    return PROCESS_SUCCESS;
}

#if defined(AUDIO_CODEC)
static int gm_aac_enc(unsigned int pcm_data_sz,
                          AUDIO_CODEC_SETTING *codec_setting,
                          unsigned int *input_buf,
                          unsigned int *output_buf,
                          unsigned int *output_len)
{
    short i, channel_cnt = codec_setting->EncMultiChannel;
    const unsigned int aac_frame_sz = sizeof(short) * AAC_FRAME_SZ * (codec_setting->aac_enc_paras[0].nChannels);

    if (!is_aac_enc_init) {
        if (AAC_Create(1 /* ID, 0 for MPEG-4, 1 for MPEG-2*/, &aac_enc_info, codec_setting, au_get_max_chan()) > 0) {
            printk("AAC_Create() failed\n");
            return PROCESS_FAILED;
        }
        is_aac_enc_init = true;
    }

    memset(output_len, 0, channel_cnt * sizeof(unsigned int));
    memset(enc_len, 0, au_get_max_chan() * sizeof(unsigned int));

    for (i = 0;i < pcm_data_sz/aac_frame_sz;i ++) {
        int ch;

        for (ch = 0;ch < channel_cnt;ch ++)
            memcpy((void *)enc_input_buf[ch], (void *)(input_buf[ch] + i * aac_frame_sz), aac_frame_sz);

        if (AAC_Enc(enc_input_buf, enc_output_buf, enc_len, &aac_enc_info) > 0) {
            printk("AAC_Enc() failed\n");
            goto error_aac_enc;
        }

        for (ch = 0;ch < channel_cnt;ch ++) {
            memcpy((void *)(output_buf[ch] + output_len[ch]), (void *)enc_output_buf[ch], enc_len[ch]);
            output_len[ch] += enc_len[ch];
        }
    }

    return PROCESS_SUCCESS;

error_aac_enc:
    AAC_Destory(&aac_enc_info);
    is_aac_enc_init = false;
    return PROCESS_FAILED;
}

static int gm_adpcm_enc(unsigned int pcm_data_sz,
                            AUDIO_CODEC_SETTING *codec_setting,
                            unsigned int *input_buf,
                            unsigned int *output_buf,
                            unsigned int *output_len)
{
    short i, channel_cnt = codec_setting->EncMultiChannel;
    static unsigned adpcm_blk_sz = 0;

    if (!is_adpcm_enc_init) {
        if (Init_IMA_ADPCM_Enc(&codec_setting->ima_paras, adpcm_state, channel_cnt) != 0) {
            printk("Init_IMA_ADPCM_Enc() failed\n");
            return PROCESS_FAILED;
        }
        adpcm_blk_sz = sizeof(short) * codec_setting->ima_paras.Sample_Num;
        adpcm_curr_ch_cnt = channel_cnt;
        is_adpcm_enc_init = true;
    }

    memset(output_len, 0, channel_cnt * sizeof(unsigned int));
    memset(enc_len, 0, au_get_max_chan() * sizeof(unsigned int));


    for (i = 0;i < (pcm_data_sz / adpcm_blk_sz);i ++) {
        int ch;

        for (ch = 0;ch < channel_cnt;ch ++)
            memcpy((void *)enc_input_buf[ch], (void *)(input_buf[ch] + i * adpcm_blk_sz), adpcm_blk_sz);

        if (IMA_ADPCM_Enc(adpcm_state, enc_input_buf, enc_output_buf, enc_len, codec_setting) > 0) {
            printk("IMA_ADPCM_Enc() failed\n");
            goto error_adpcm_enc;
        }

        for (ch = 0;ch < channel_cnt;ch ++) {
            memcpy((void *)(output_buf[ch] + output_len[ch]), (void *)enc_output_buf[ch], enc_len[ch]);
            output_len[ch] += enc_len[ch];
        }
    }

    return PROCESS_SUCCESS;

error_adpcm_enc:
    IMA_ADPCM_Enc_Destory(adpcm_state, channel_cnt);
    is_adpcm_enc_init = false;
    return PROCESS_FAILED;
}

static int gm_g711_enc(unsigned int pcmDataSize,
        AUDIO_CODEC_SETTING *codec_setting,
        unsigned int *inputBuf,
        unsigned int *outputBuf,
        unsigned int *outputLen)
{
    short i, channel_cnt = codec_setting->EncMultiChannel;
    const unsigned g711_blk_sz = sizeof(short) * AULAW_BLK_SZ;

    if (!is_g711_enc_init) {
        if (g711_init_enc(codec_setting, inparam_enc) != 0) {
            printk("g711_init_enc() failed\n");
            return PROCESS_FAILED;
        }
        is_g711_enc_init = true;
        g711_curr_ch_cnt = channel_cnt;
    }

    memset(outputLen, 0, channel_cnt * sizeof(unsigned int));
    memset(enc_len, 0, AUDIO_MAX_CHAN * sizeof(unsigned int));

    for (i = 0;i < (pcmDataSize / g711_blk_sz);i ++) {
        int ch;

        for (ch = 0;ch < channel_cnt;ch ++)
            memcpy((void *)enc_input_buf[ch], (void *)(inputBuf[ch] + i * g711_blk_sz), g711_blk_sz);

        if (G711_Enc(inparam_enc, enc_input_buf, enc_output_buf, enc_len, codec_setting->EncMultiChannel) > 0) {
            printk("G711_Enc() failed\n");
            goto err_g711_enc;
        }

        for (ch = 0;ch < channel_cnt;ch ++) {
            memcpy((void *)(outputBuf[ch] + outputLen[ch]), (void *)enc_output_buf[ch], enc_len[ch]);
            outputLen[ch] += enc_len[ch];
        }
    }

    return PROCESS_SUCCESS;

err_g711_enc:
    g711enc_destory(inparam_enc, codec_setting->EncMultiChannel);
    is_g711_enc_init = false;
    return PROCESS_FAILED;
}
#endif

static u32 *rec_dbg_stage = NULL;

static void audio_encode_handler(struct work_struct *work)
{
    struct audio_job_t *audio_job = container_of(work, struct audio_job_t, in_work);
    unsigned int type_i = 0, bufIdx = 0;
    unsigned int codec_type[SUPPORT_TYPE] = {0};
    unsigned int bitrate[SUPPORT_TYPE] = {0};
    static AUDIO_CODEC_SETTING codec_setting;
    unsigned int beginTick = 0, endTick = 0;

    BEGIN_TICK();

    for (type_i = 0;type_i < SUPPORT_TYPE;type_i ++) {

        /* (16bits|16bits) = (type0|type1), SUPPORT_TYPE = 2*/
        // IMPORTANT: we only support PCM+(other type), that means only one bitrate field is needed
        if (type_i == 0) {
            codec_type[type_i] = (audio_job->codec_type & BITMASK(16, 16)) >> 16;
            bitrate[type_i] = audio_job->shared.bitrate;
            (codec_type[type_i] == AU_TYPE_PCM) ? (bufIdx = 0) : (bufIdx = 1);
        } else {
            codec_type[type_i] = audio_job->codec_type & BITMASK(0, 16);
            bitrate[type_i] = audio_job->shared.bitrate;
        }

        if (codec_type[type_i] >= AU_TYPE_MAX) {
            codec_setting.encode_error = PROCESS_FAILED;
            printk("No support encode_type[%d] = %d!\n", type_i, codec_type[type_i]);
            goto finish_encode;
        }

        if (codec_type[type_i] == AU_TYPE_NONE) {
            if (type_i == 0) {
                codec_setting.encode_error = PROCESS_FAILED;
                printk("First Encode Type cannot be AU_TYPE_NONE\n");
            } else {
                codec_setting.encode_error = PROCESS_SUCCESS;
            }
            goto finish_encode;
        }

        /* set codec */
        codec_setting.encode_audio_type = codec_type[type_i] - AU_TYPE_AAC; //0: AAC 1:ADPCM
        codec_setting.EncMultiChannel = au_get_max_chan();
        codec_setting.encode_error = PROCESS_SUCCESS;
#if defined(AUDIO_CODEC)
        if (codec_type[type_i] == AU_TYPE_ALAW || codec_type[type_i] == AU_TYPE_ULAW)
            codec_setting.encode_audio_type = AULAW_ENCODE;

        switch (codec_setting.encode_audio_type) {
            int i;
            case AAC_ENCODE:
            {
                bool need_reinit = false, first_time = true;
                
                for (i = 0;i < codec_setting.EncMultiChannel;i ++) {
                    //codec_setting.aac_enc_paras[i].bitRate    = bitrate[type_i];
                    codec_setting.aac_enc_paras[i].nChannels  = audio_job->channel_type;

                    if (codec_setting.aac_enc_paras[i].sampleRate != audio_job->sample_rate) {
                        codec_setting.aac_enc_paras[i].sampleRate = audio_job->sample_rate;
                        need_reinit = true;
                    }

                    if (codec_setting.aac_enc_paras[i].bitRate != bitrate[type_i]) {
                    	codec_setting.aac_enc_paras[i].bitRate = bitrate[type_i];
                        need_reinit = true;
                    }
                    
                    if (aac_nChannels != audio_job->channel_type) {
                        aac_nChannels = audio_job->channel_type;
                        need_reinit = true;
                    }

                    if (need_reinit == true) {
                        is_aac_enc_init = false;
                        if (first_time == true) {
                        AAC_Destory(&aac_enc_info);
                            first_time = false;
                        }
                    }
                    
                    //printk("codec_setting.aac_enc_paras[%d].bitRate = %d\ncodec_setting.aac_enc_paras[%d].nChannels = %d\n",
                           //i,codec_setting.aac_enc_paras[i].bitRate,i,codec_setting.aac_enc_paras[i].nChannels);
                }
                break;
            }
            case IMA_ADPCM_ENCODE:
                if (audio_job->channel_type != codec_setting.ima_paras.nChannels) {
                    is_adpcm_enc_init = false;
                    IMA_ADPCM_Enc_Destory(adpcm_state, adpcm_curr_ch_cnt);
                }
                codec_setting.ima_paras.nBlockAlign = BLOCKALIGN;
                codec_setting.ima_paras.nChannels = audio_job->channel_type;
                break;

            case AULAW_ENCODE:
                if (codec_type[type_i] == AU_TYPE_ALAW) {
                    codec_setting.aulaw_codec_pars.aulaw_enc_option = LINEAR2ALAW;
                }
                if (codec_type[type_i] == AU_TYPE_ULAW) {
                    codec_setting.aulaw_codec_pars.aulaw_enc_option = LINEAR2ULAW;
                }
                if (g711_curr_enc_type != codec_type[type_i]) {
                    is_g711_enc_init = false;
                    g711enc_destory(inparam_enc, g711_curr_ch_cnt);
                }
                g711_curr_enc_type = codec_type[type_i];
                codec_setting.aulaw_codec_pars.block_size = AULAW_BLK_SZ;
                codec_setting.aulaw_codec_pars.Decoded_Sample_Num = AULAW_BLK_SZ;
                break;

            default:
                break;
        }
#endif

        //DBG("Encode Type = %d\n", codec_type[type_i]);

        beginTick = jiffies;
        codec_setting.encode_error =
            gm_codec_funs[codec_type[type_i]].encode(audio_job->data_length[type_i][0], &codec_setting, audio_job->data_offset[bufIdx],
                    audio_job->data_offset[type_i], audio_job->data_length[type_i]);
        endTick = jiffies;
        DBG("Encode ticks = %d\n", endTick - beginTick);

        if (codec_setting.encode_error)
            break;
    } /* for */

finish_encode:
    if (codec_setting.encode_error == PROCESS_FAILED) {
        audio_job->status = JOB_STS_ENCERR;
    } else if (!is_live_on) {
        if (audio_job->status != JOB_STS_ENC_TIMEOUT)
            audio_job->status = JOB_STS_ACK;
    } else {
        if (audio_job->status != JOB_STS_ENC_TIMEOUT && audio_job->status != JOB_STS_DEC_TIMEOUT)
            audio_job->status = JOB_STS_ACK;
    }

    if (audio_job->status == JOB_STS_ENCERR)
        printk("Audio Type(%d) Encode Error!\n", codec_type[type_i]);

    END_TICK();
    TICK_PASSBY("RECORD One JOB");
}

/* main audio record thread */
static u32 rec_ssp_bmp = 0, ori_rec_ssp_bmp = 0, xor_rec_ssp_bmp = 0;
//static u32 prev_jiffies = 0;

void audio_in_handler(struct work_struct *work)
{
    struct audio_job_t *audio_job = container_of(work, struct audio_job_t, in_work);
    unsigned long ssp_idx;
    int *live_on_ssp = au_get_live_on_ssp();
    int *live_chan_num = au_get_live_chan_num();
    u32 live_ssp_idx = 0;
    int i, retval = 0;
    //u32 time_offset = 0;
    u32 job_id = 0;

    if (audio_job->data)
        job_id = ((job_item_t *)(audio_job->data))->job_id;

    *rec_dbg_stage = 0;

    switch (audio_job->status) {
        case JOB_STS_REC_RUN:
            *rec_dbg_stage |= (1 << IN_REC_RUN);

            GM_LOG(GM_LOG_REC_PKT_FLOW, "job(%d) JOB_STS_REC_RUN\n", job_id);

            if (live_sound && !IS_ERR(live_sound)) {
                kthread_stop(live_sound);
                //printk("%s:disable live thread\n", __func__);
                live_sound = NULL;
            }

            rec_ssp_bmp = audio_job->ssp_bitmap;

            /* find difference between previous ssp bmp and present ssp bmp */
            xor_rec_ssp_bmp = rec_ssp_bmp ^ ori_rec_ssp_bmp;

            ori_rec_ssp_bmp = audio_job->ssp_bitmap;

            /* kind of stop record: 1. on->off 2. off->on(needless) */
            for_each_set_bit(ssp_idx, (const unsigned long *)&xor_rec_ssp_bmp, au_get_ssp_max())
                audio_drv_stop(STREAM_DIR_RECORD, ssp_idx);

            if (!is_record_on)
                is_record_on = true;

            break;

        case JOB_STS_REC_STOP:
            *rec_dbg_stage |= (1 << IN_REC_STOP);

            GM_LOG(GM_LOG_REC_PKT_FLOW, "job JOB_STS_REC_STOP\n");

            rec_ssp_bmp = audio_job->ssp_bitmap;

            is_record_on = false;

            for_each_set_bit(ssp_idx, (const unsigned long *)&rec_ssp_bmp, au_get_ssp_max()) {

                if (is_live_on && live_in_ssp[ssp_idx] && (live_chan_num[ssp_idx] >= 0)) {
                    /* it means live sound is on */
                } else {
                    audio_drv_stop(STREAM_DIR_RECORD, ssp_idx);
                }
            }

            if (is_live_on) {
                if (!live_sound) {
                    //printk("%s:enable live thread\n", __func__);
                    live_sound = kthread_run(audio_live_thread, NULL, "audio_live_sound");
                    if (IS_ERR(live_sound)) {
                        panic("%s: unable to create kernel thread: %ld\n",
                                __func__, PTR_ERR(live_sound));
                    }
                }
            }

            return;

        case JOB_STS_LIVE_RUN:
            *rec_dbg_stage |= (1 << IN_LIVE_RUN);

            GM_LOG(GM_LOG_REC_PKT_FLOW, "job(%d) JOB_STS_LIVE_RUN\n", job_id);

            is_live_on = true;

            live_ssp_idx = audio_job->ssp_bitmap & 0xf;

            live_chan = audio_job->chan_bitmap;

            ssp_idx = ssp_and_chan_translate(live_chan, true);

            live_in_ssp[ssp_idx] = true;

            live_chan_num[ssp_idx] = ssp_and_chan_translate(live_chan, false);

            DBG("Live channel = %d, ssp_idx = %d\n", live_chan, audio_job->ssp_bitmap & 0xf);

            if (!is_record_on) {
                DBG("live without record\n");
                if (!live_sound) {
                    DBG("%s:enable live thread\n", __func__);
                    live_sound = kthread_run(audio_live_thread, NULL, "audio_live_sound");
                    if (IS_ERR(live_sound)) {
                        panic("%s: unable to create kernel thread: %ld\n",
                                __func__, PTR_ERR(live_sound));
                    }
                }
            }

            audio_job_check(audio_job);

            return;

        case JOB_STS_LIVE_STOP:
            *rec_dbg_stage |= (1 << IN_LIVE_STOP);

            GM_LOG(GM_LOG_REC_PKT_FLOW, "job(%d) JOB_STS_LIVE_STOP\n", job_id);

            retval = false;

            live_ssp_idx = audio_job->ssp_bitmap & 0xf;

            live_on_ssp[live_ssp_idx] = false;

            ssp_idx = ssp_and_chan_translate(audio_job->chan_bitmap, true);

            for (i = 0;i < au_get_ssp_max();i ++) {
                /* check all live ssp is on or not */
                if (live_on_ssp[i]) {
                    retval = true;
                    break;
                }
            }
            is_live_on = retval; /* false: no other ssp for live */

            live_in_ssp[ssp_idx] = false;

            live_chan_num[ssp_idx] = -1;

            if (!is_live_on && live_sound && !IS_ERR(live_sound)) {
                kthread_stop(live_sound);
                printk("%s:JOB_STS_LIVE_STOP, disable live thread\n", __func__);
                live_sound = NULL;
            }

            audio_drv_stop(STREAM_DIR_PLAYBACK, live_ssp_idx);

            for (i = 0;i < au_get_ssp_max();i ++) {
                if (!is_record_on) {
                    audio_drv_stop(STREAM_DIR_RECORD, i);
                    live_in_ssp[i] = false;
                } else {
                    if (live_in_ssp[i] == false && (ori_rec_ssp_bmp & (1 << i)) == 0) {
                        audio_drv_stop(STREAM_DIR_RECORD, i);
                    }
                }
            }

            audio_job_check(audio_job);

            return;

        default:
            *rec_dbg_stage |= (1 << IN_NO_STATUS);
            printk("%s: Error audio_job status(%d)\n", __func__, audio_job->status);
            GM_LOG(GM_LOG_ERROR, "%s: Error audio_job status(%d)\n", __func__, audio_job->status);
            break;
    }

    *rec_dbg_stage |= (1 << IN_CASE_DONE);

    /* record part setting of live sound */
    if (is_live_on) {
        int *ssp_num = au_get_ssp_num();
        int *ssp_chan = au_get_ssp_chan();

        GM_LOG(GM_LOG_REC_PKT_FLOW, "job(%d) record for live-sound\n", job_id);

        for (ssp_idx = 0;ssp_idx < au_get_ssp_max();ssp_idx ++) {

            if (ssp_num[ssp_idx] == 2)
                continue;

            if (live_in_ssp[ssp_idx]) {

                /* count the total channel num */
                if (live_chan_num[ssp_idx] >= 0) {
                    int i, chan_cnt = 0;

                    for (i = 0;i < ssp_idx;i ++) {
                        chan_cnt += ssp_chan[i];
                    }

                    audio_job->chan_bitmap |= (1 << (live_chan_num[ssp_idx] + chan_cnt));
                    rec_ssp_bmp |= (1 << ssp_idx);
                    break;
                }
            }
        } /* for */
    }

    for_each_set_bit(ssp_idx, (const unsigned long *)&rec_ssp_bmp, au_get_ssp_max()) {
#if defined(CONFIG_PLATFORM_GM8287)
        int *ssp_num = au_get_ssp_num();
        if (ssp_num[ssp_idx] == 2) /* ssp2 is connected to HDMI */
            continue;
#endif
        GM_LOG(GM_LOG_REC_PKT_FLOW, "job(%d) record ssp%d start\n", job_id, ssp_idx);

        live_in_ssp[ssp_idx] = true;

        if (record_audio(ssp_idx, audio_job)) {
            audio_job->status = JOB_STS_ENCERR;
            printk("%s: record_audio() failed\n", __func__);
            GM_LOG(GM_LOG_ERROR, "job(%d) record ssp%d failed\n", job_id, ssp_idx);
        }

        GM_LOG(GM_LOG_REC_PKT_FLOW, "job(%d) record ssp%d stop\n", job_id, ssp_idx);
    }

    *rec_dbg_stage |= (1 << IN_PCM_DONE);

    /* playback part of audio live sound */
    if (is_live_on) {
        int idx = 0;

        GM_LOG(GM_LOG_REC_PKT_FLOW, "job(%d) playback for live-sound\n", job_id);

        if (((audio_job->codec_type & BITMASK(16, 16)) >> 16) > AU_TYPE_PCM)
            idx = 1;
        set_resample_play(audio_job, audio_job->data_length[idx][live_chan], (void *)audio_job->data_offset[idx][live_chan]);
        *rec_dbg_stage |= (1 << IN_PLAY_DONE);
    }

#if 0
    audio_job->time_sync.jiffies_fa626 = get_gm_jiffies() - audio_job->time_sync.jiffies_fa626;

    time_offset = (audio_job->data_length[0][0] * 1000 ) / (audio_job->sample_rate * DEFAULT_SAMPLE_SIZE / BITS_PER_BYTE) / audio_job->channel_type;

    if (audio_job->time_sync.jiffies_fa626 < prev_jiffies)
        audio_job->time_sync.jiffies_fa626 = prev_jiffies + time_offset;
    else if (audio_job->time_sync.jiffies_fa626 - prev_jiffies < time_offset)
        audio_job->time_sync.jiffies_fa626 = prev_jiffies + time_offset;

    prev_jiffies = audio_job->time_sync.jiffies_fa626;
#endif

    GM_LOG(GM_LOG_REC_PKT_FLOW, "job(%d) start encode\n", job_id);
    audio_encode_handler(work);
    GM_LOG(GM_LOG_REC_PKT_FLOW, "job(%d) encode done\n", job_id);

    *rec_dbg_stage |= (1 << IN_ENC_DONE);

    audio_job_check(audio_job);

    *rec_dbg_stage |= (1 << IN_JOB_BACK);
}
EXPORT_SYMBOL(audio_in_handler);

static int playback_audio(void *job, void *playBuf, unsigned playSize, unsigned ssp_idx)
{
    ssp_buf_t ctrl_buf = {0};
    unsigned ssp_play_sz, ret_sz, buf_time;
    struct audio_job_t *audio_job = (struct audio_job_t *)job;
    int *ssp_chan = au_get_ssp_chan();
    int *ssp_num = au_get_ssp_num();
    u32 wait_cnt = 0;
    const int out_ch = output_ch[ssp_idx];
    u32 job_id = 0;

    if (!playSize) {
        printk("%s: zero play size!\n", __func__);
        return PROCESS_FAILED;
    }

    if (audio_job->data)
        job_id = ((job_item_t *)(audio_job->data))->job_id;

#if defined(CONFIG_PLATFORM_GM8287)
    (ssp_num[ssp_idx] == 2) ? (ssp_play_sz = playSize) : (ssp_play_sz = ssp_chan[ssp_idx] * playSize);
#else
    (ssp_num[ssp_idx] == 2) ? (ssp_play_sz = ssp_chan[ssp_idx] * playSize) : (ssp_play_sz = ssp_chan[ssp_idx] * playSize);
#endif

    ctrl_buf.kbuf_ptr           = playBuf;
    ctrl_buf.buf_ptr            = frm_va2pa((u32)ctrl_buf.kbuf_ptr);
    ctrl_buf.buf_size           = playSize;
    ctrl_buf.buf_pos[out_ch]    = 0;            // used to record the progress of playback
    ctrl_buf.bitmap             = out_ch;       // play channel is set from TV decoder's setting
    ctrl_buf.ssp_idx            = ssp_idx;
    ctrl_buf.offset             = &buf_time;    // difference between ssp dma read/write pointers
    ctrl_buf.sample_rate        = audio_job->sample_rate;

    if (ssp_chan[ssp_idx] > 1)
        ctrl_buf.channel_type       = STEREO_CHAN;
    else
        ctrl_buf.channel_type       = audio_job->channel_type;

    /* wait_cnt is used for no pending job when hw is stopped or died. */
    wait_cnt = 3 * 1000 * playSize / audio_job->sample_rate;

    GM_LOG(GM_LOG_PB_PKT_FLOW, "ssp%d, job(%d) start write data %d bytes to HW, wait_cnt(%d)\n", ssp_idx, job_id, ssp_play_sz, wait_cnt);

    while (ssp_play_sz && wait_cnt) {
        ret_sz = audio_gm_write(&audio_handle, (void *)&ctrl_buf, &ssp_play_sz);
        if (ret_sz == 0) {
            msleep(1);
            wait_cnt --;
        }
    }

    GM_LOG(GM_LOG_PB_PKT_FLOW, "ssp%d, job(%d) write data to HW done, wait_cnt(%d)\n", ssp_idx, job_id, wait_cnt);

    if (!wait_cnt) {
        audio_job->status = JOB_STS_DEC_TIMEOUT;
        audio_job->ssp_chan = ssp_idx;
        printk_ratelimited("%s: timeout!\n", __func__);
    }

    if (!is_live_on)
        audio_job->shared.buf_time = buf_time;

    return PROCESS_SUCCESS;
}

#if defined(AUDIO_CODEC)
/* helper function ported from audio_codec_main.c by Peter */
static void aac_decode_fill_buf(aac_buffer *b, unsigned char **in_ptr, int *EncDataSize)
{
    unsigned char *inptr;
	inptr = *in_ptr;
	if (b->bytes_consumed > 0) { //4608 is full
	    int fill_size;
        if (b->bytes_into_buffer)
            memmove((void*)b->buffer, (void*)(b->buffer + b->bytes_consumed), b->bytes_into_buffer*sizeof(unsigned char));
        if (*EncDataSize != 0) {
    		fill_size = DECODE_BUF_SIZE - b->bytes_into_buffer;
    		if(*EncDataSize > fill_size) {
    		    memcpy((void*)(b->buffer + b->bytes_into_buffer), inptr, fill_size);
    		    *EncDataSize -= fill_size;
    		    b->bytes_into_buffer += fill_size;
    		    inptr += fill_size;
    		} else {
    		    memcpy((void*)(b->buffer+b->bytes_into_buffer), inptr, *EncDataSize);
    		    b->bytes_into_buffer += *EncDataSize;
    		    *EncDataSize = 0;
    		}
	    }
	    b->bytes_consumed = 0;
	} else if ((b->bytes_consumed == 0) && (*EncDataSize != 0) && (b->bytes_into_buffer == 0)) { //initial state
		if (*EncDataSize > DECODE_BUF_SIZE) {
		    memcpy((void*)b->buffer, inptr, DECODE_BUF_SIZE);
		    *EncDataSize -= DECODE_BUF_SIZE;
		    b->bytes_into_buffer = DECODE_BUF_SIZE;
		    inptr += DECODE_BUF_SIZE;
		} else {
		    memcpy((void*)b->buffer, inptr, *EncDataSize);
		    b->bytes_into_buffer = *EncDataSize;
		    *EncDataSize = 0;
		}
	}
    *in_ptr = inptr;
}

/* helper function ported from audio_codec_main.c by Peter */
static void aac_advance_buffer(aac_buffer *b, int bytes)
{
    b->bytes_consumed = bytes;
    b->bytes_into_buffer -= bytes;
}
#endif /* AUDIO_CODEC */

#if defined(CHECK_PLL) && defined(CONFIG_PLATFORM_GM8287)
static inline u32 audio_get_HDMI_sample_rate(void)
{
    u32 sample_rate = HDMI_SAMPLE_RATE, reg28h = 0, reg74h = 0;
    u32 val;

    reg28h = ftpmu010_read_reg(0x28);
    reg74h = ftpmu010_read_reg(0x74);
    val = (reg28h & 0x300000) >> 20;

    //printk("%s(%d) PMU offset 28h(0x%x), 74h(0x%x), val(0x%x)\n", __FUNCTION__, __LINE__, reg28h, reg74h, val);
    
    if (val == 0x01)
        sample_rate = 46875;
    else if ((val == 0x02) || (val == 0x03))
        sample_rate = 48027;

    return sample_rate;
}
#endif

static int set_resample_play(void *job, unsigned int input_sz, void *input_buf)
{
    struct audio_job_t *audio_job = (struct audio_job_t *)job;
    unsigned int ratio = audio_job->resample_ratio * 100;
    u32 ssp_bitmap = audio_job->ssp_bitmap;
    unsigned int ssp_num;
    int ret = PROCESS_SUCCESS;
    int *live_on_ssp = au_get_live_on_ssp();
    u32 job_id = 0;
#if defined(CHECK_PLL) && defined(CONFIG_PLATFORM_GM8287)
    unsigned int pll4_freq = ftpmu010_get_attr(ATTR_TYPE_PLL4);
    unsigned int pll5_freq = ftpmu010_get_attr(ATTR_TYPE_PLL5);
    //printk("%s(%d) ratio = %d, pll4_freq = %d, pll5_freq = %d\n", __FUNCTION__,__LINE__,ratio, pll4_freq, pll5_freq);
#endif

    if (audio_job->data)
        job_id = ((job_item_t *)(audio_job->data))->job_id;

    if (ssp_bitmap == 0)
        goto end;

    if (ssp_bitmap >> SSP_MAX_USED) {
        printk("%s: SSP bitmap error (0x%x)\n", __func__, ssp_bitmap);
        ret = PROCESS_FAILED;
        goto end;
    }

    if (is_live_on && audio_job->status != JOB_STS_PLAY_RUN) {
        ssp_bitmap = 0;
        for (ssp_num = 0;ssp_num < SSP_MAX_USED;ssp_num ++) {
            if (live_on_ssp[ssp_num]) {
                ssp_bitmap |= (1 << ssp_num);
            }
        }
        DBG("Live SSP bitmap = 0x%x\n", ssp_bitmap);
        GM_LOG(GM_LOG_PB_PKT_FLOW, "live-sound ssp bitmap(0x%x)\n", ssp_bitmap);
    }

    for_each_set_bit(ssp_num, (const unsigned long *)&ssp_bitmap, au_get_ssp_max()) {
        void *final_buf = NULL;
        unsigned final_sz = 0;
        int *i2s_num = au_get_ssp_num();

        if ((ratio > 0 && ratio < (AVG_RESAMPLE_RATIO * 100 * 100)) || i2s_num[ssp_num] == 2) {
#if defined(CONFIG_PLATFORM_GM8287)
            unsigned int hdmi_sample_rate = HDMI_SAMPLE_RATE;
            u64 tmp;
            if (i2s_num[ssp_num] == 2) {  /* SSP2 is HDMI */
                if (is_live_on)
                    ratio = 100 * 100;
#if defined(CHECK_PLL)                    
                if ((pll4_freq != 768000000) && (pll5_freq == 750000000)) {
                    hdmi_sample_rate = audio_get_HDMI_sample_rate();
                    GM_LOG(GM_LOG_DETAIL, "adjust HDMI sample rate as %d\n", hdmi_sample_rate);
                }
#endif
                tmp = (u64)((u64)ratio * ((u64)hdmi_sample_rate * 100));

                //printk("%s, ratio(%d), hdmi_sample_rate(%d), tmp(%llu)\n", __func__, ratio, hdmi_sample_rate, tmp);
                do_div(tmp, (DEFAULT_SAMPLE_RATE*100));
                ratio = (u32)tmp;

                GM_LOG(GM_LOG_DETAIL, "%s, ratio(%d)\n", __func__, ratio);
                //printk("%s, ratio(%d)\n", __func__, ratio);
            }
#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
            if (i2s_num[ssp_num] == 0 && au_get_ssp_chan()[ssp_num] > 1)
                ratio *= 2; /* playback stereo, but audio-in is 2 channel mono */
#endif
        } else {
            if (!is_live_on)
                printk("%s: error ratio(%d)\n", __func__, ratio);
            ratio = 100 * 100;
        }

        if (ratio > 0 && ratio != (100 * 100)) {
            int retval, OneFrameMode = 0;   /* 0 for continuous, 1 for single */
            unsigned out_sz = (input_sz * ratio) / (100 * 100);
            static unsigned last_outsz = 0, resample_init = 0;

            //printk("in_sz(%d), out_sz(%d), ratio(%d)\n", input_sz, out_sz, ratio);

            if (out_sz % sizeof(short) > 0)
                out_sz ++;

            if (last_outsz != out_sz) {
                if (resample_init)
                    GM_Resampling_Destroy();
                resample_init = 0;
                last_outsz = out_sz;
            }

            if (resample_init == 0) {
                if ((retval = GM_Resampling_Init(input_sz/sizeof(short), out_sz/sizeof(short), OneFrameMode))) {
                    printk("GM_Resampling_Init error(%d)\n", retval);
                    GM_LOG(GM_LOG_ERROR, "%s, GM_Resampling_Init error(%d)\n", __func__, retval);
                    ret = PROCESS_FAILED;
                    last_outsz = 0;
                } else {
                    resample_init = 1;
                }
            }

            if ((retval = GM_Resampling(input_buf, (void *)resample_buffer[ssp_num]))) {
                printk("GM_Resampling error(%d)\n", retval);
                GM_LOG(GM_LOG_ERROR, "%s, GM_Resampling error(%d)\n", __func__, retval);
                ret = PROCESS_FAILED;
            }

            final_buf = (void *)resample_buffer[ssp_num];
            final_sz = out_sz;
            if (DMA_ALIGN_CHECK(resample_size[ssp_num]))
                fmem_dcache_sync(final_buf, resample_size[ssp_num], DMA_FROM_DEVICE);

            if (ret == PROCESS_FAILED)
                goto end;

#if defined(CONFIG_PLATFORM_GM8287)
            if (*audio_drv_get_resample_save_flag()) {
                if (ssp_num == 2) {
                    static int stored_sz = DEFAULT_SAMPLE_SIZE * HDMI_SAMPLE_RATE * PLAYBACK_SEC / BITS_PER_BYTE;
                    static unsigned long long offset = 0;
                    stored_sz -= final_sz;
                    if (stored_sz >= 0) {
                        int ret = 0;
                        ret = audio_write_file(RESAMPLE_FILE_PATH, (void *)final_buf, final_sz, &offset);
                        if (ret < 0)
                            printk("%s: write PCM error!\n", __func__);
                    } else {
                        /* reset */
                        audio_drv_set_resample_save_flag(0);
                        stored_sz = DEFAULT_SAMPLE_SIZE * HDMI_SAMPLE_RATE * PLAYBACK_SEC / BITS_PER_BYTE;
                        offset = 0;
                        printk("save %s done!\n", RESAMPLE_FILE_PATH);
                    }
                }
            }
#endif

        } else {

            final_buf = input_buf;
            final_sz = input_sz;

        }

        if (audio_job->time_sync.mute_samples > 0 && !is_live_on) {

            audio_job->time_sync.mute_samples *= (audio_job->sample_size / BITS_PER_BYTE) * ratio / (100 * 100);

            GM_LOG(GM_LOG_PB_PKT_FLOW, "ssp%d, job(%d) playback with mute samples %d bytes\n", ssp_num, job_id, audio_job->time_sync.mute_samples);

            while (audio_job->time_sync.mute_samples > 0) {
                if (audio_job->time_sync.mute_samples > MUTE_BUF_SZ) {
                    ret = playback_audio((void *)audio_job, (void *)silence_buf, MUTE_BUF_SZ, ssp_num);
                    audio_job->time_sync.mute_samples -= MUTE_BUF_SZ;
                } else {
                    ret = playback_audio((void *)audio_job, (void *)silence_buf, audio_job->time_sync.mute_samples, ssp_num);
                    audio_job->time_sync.mute_samples = 0;
                }
            }
        }

        GM_LOG(GM_LOG_PB_PKT_FLOW, "ssp%d, job(%d) playback %d bytes\n", ssp_num, job_id, final_sz);

        ret = playback_audio((void *)audio_job, final_buf, final_sz, ssp_num);
    } /* for */

end:
    return ret;
}

static int gm_pcm_dec(void *job, void *input_buf, unsigned int input_sz)
{
    return set_resample_play(job, input_sz, input_buf);
}

#if defined(AUDIO_CODEC)
static int gm_aac_dec(void *job, void *input_buf, unsigned int input_sz)
{
    unsigned int dec_output_sz = 0;
    void *dec_output_buf = NULL;

    if (!is_aac_dec_init) {
        if (GM_AAC_Dec_Init(&aac_dec_hdl, &aac_dec_buf, &aac_dec_info)) {
        	printk("GM_AAC_Dec_Init() failed\n");
            return PROCESS_FAILED;
        }
        if (aac_dec_hdl == NULL)
            printk("aac handler is NULL\n");
        is_aac_dec_init = true;
    }

    while(1) {
        aac_decode_fill_buf(&aac_dec_buf, (unsigned char **)&input_buf, &input_sz); 	//buffer size is 4608 bytes
        if (aac_dec_buf.bytes_into_buffer == 0) {
            printk("set bytes into buffer error!\n");
            goto aac_dec_error;
        }
        dec_output_buf = GMAACDecode(aac_dec_hdl, aac_dec_info, aac_dec_buf.buffer, aac_dec_buf.bytes_into_buffer);

        if (aac_dec_info->error != 0 || dec_output_buf == NULL) {
            printk("error = 0x%x, GMAACDecode() failed\n", aac_dec_info->error);
            /* peter suggests no destroy to make audio continuous */
            //goto aac_dec_error;
        }

        dec_output_sz = aac_dec_info->samples * sizeof(short);

        if (aac_dec_info->samples > 0 && dec_output_buf)
            if (set_resample_play(job, dec_output_sz, dec_output_buf))
                goto aac_dec_error;

        aac_advance_buffer(&aac_dec_buf, aac_dec_info->bytesconsumed);
        if ((input_sz == 0) && (aac_dec_buf.bytes_into_buffer == 0))
            break;
    }

    return PROCESS_SUCCESS;

aac_dec_error:
    GM_AAC_Dec_Destory(aac_dec_hdl, &aac_dec_buf, &aac_dec_info);
    is_aac_dec_init = false;
    return PROCESS_FAILED;
}


#if defined(MEMCPY_DMA)
static short *adpcm_dec_output_buf = NULL;
static dma_addr_t adpcm_dec_output_buf_paddr = 0;
#else
static short adpcm_dec_output_buf[IMA_BLK_SZ_MONO];
#endif
static unsigned char adpcm_dec_input_buf[BLOCKALIGN];
static int gm_adpcm_dec(void *job, void *input_buf, unsigned int input_sz)
{
    unsigned int i, dec_output_sz = 0;

    if (!is_adpcm_dec_init) {
        if (Init_IMA_ADPCM_Dec(&adpcm_codec_setting.ima_paras)) {
            printk("Init_IMA_ADPCM_Dec() failed\n");
            return PROCESS_FAILED;
        }
        is_adpcm_dec_init = true;
    }

    for (i = 0;i < (input_sz / adpcm_codec_setting.ima_paras.nBlockAlign);i ++) {
        memcpy(adpcm_dec_input_buf, (void *)((unsigned)input_buf + i * adpcm_codec_setting.ima_paras.nBlockAlign), adpcm_codec_setting.ima_paras.nBlockAlign);

        IMA_ADPCM_dec(&adpcm_codec_setting.ima_paras, adpcm_dec_input_buf, adpcm_dec_output_buf);

        dec_output_sz = adpcm_codec_setting.ima_paras.Sample_Num * sizeof(short);

        if (set_resample_play(job, dec_output_sz, adpcm_dec_output_buf))
            goto adpcm_dec_error;
    }

    return PROCESS_SUCCESS;

adpcm_dec_error:
    return PROCESS_FAILED;
}

static int gm_g711_dec(void *job, void *inputBuf, unsigned int inputSize)
{
    unsigned int i, decOutputSize = 0;
    int blk_sz = g711_dec_setting.ima_paras.nBlockAlign;

    if (!is_g711_dec_init) {
        if (g711_init_dec(&g711_dec_setting, &inparam_dec) != 0) {
            printk("g711_init_dec() failed\n");
            return PROCESS_FAILED;
        }
        is_g711_dec_init = true;
    }

    for (i = 0;i < (inputSize / blk_sz);i ++) {
        memcpy(g711_dec_input_buf, (void *)((unsigned)inputBuf + i * blk_sz), blk_sz);

        G711_Dec(inparam_dec, g711_dec_input_buf, g711_dec_output_buf);

        decOutputSize = g711_dec_setting.aulaw_codec_pars.Decoded_Sample_Num * sizeof(short);

        if (set_resample_play(job, decOutputSize, g711_dec_output_buf))
            goto g711_dec_err;
    }

    return PROCESS_SUCCESS;

g711_dec_err:
    if (inparam_dec)
    	g711dec_destory(inparam_dec);
   
    return PROCESS_FAILED;
}
#endif /* AUDIO_CODEC */

static u32 *play_dbg_stage = NULL;

/* main audio playback thread */
void audio_out_handler(struct work_struct *work)
{
    struct audio_job_t *audio_job = container_of(work, struct audio_job_t, out_work);
    unsigned int play_sz = 0;
    unsigned int codec_type = 0;
    unsigned long ssp_num;
    void *play_buf = NULL;
    int retval = 0;
    u32 job_id = 0;

    if (audio_job->data)
        job_id = ((job_item_t *)(audio_job->data))->job_id;

    *play_dbg_stage = 0;

    switch (audio_job->status) {
        case JOB_STS_PLAY_RUN:
            GM_LOG(GM_LOG_PB_PKT_FLOW, "job(%d) start playback\n", job_id);
            *play_dbg_stage |= (1 << OUT_PLAY_RUN);
            break;
        case JOB_STS_PLAY_STOP:
            GM_LOG(GM_LOG_PB_PKT_FLOW, "job(%d) stop playback\n", job_id);
            *play_dbg_stage |= (1 << OUT_PLAY_STOP);
            for_each_set_bit(ssp_num, (const unsigned long *)&audio_job->ssp_bitmap, au_get_ssp_max())
                audio_drv_stop(STREAM_DIR_PLAYBACK, ssp_num);
            return;
        default:
            *play_dbg_stage |= (1 << OUT_NO_STATUS);
            printk("%s: Error audio_job status(%d)\n", __func__, audio_job->status);
            retval = PROCESS_FAILED;
            goto decode_finish;
    }

    *play_dbg_stage |= (1 << OUT_CASE_DONE);

    if (!audio_job->out_pa || !audio_job->out_size || !audio_job->codec_type) {
        printk("%s: Error! out_pa(0x%x), out_size(%u), codec_type(%d)\n", __func__, audio_job->out_pa, audio_job->out_size, audio_job->codec_type);
        retval = PROCESS_FAILED;
        goto decode_finish;
    }

    play_buf = (void *)audio_job->out_pa;
    play_sz = audio_job->bitstream_size;
    codec_type = audio_job->codec_type;

    DBG("Decode Type = %d, Playback size = %u\n", codec_type, play_sz);

    if (audio_job->resample_ratio < 0) {
        printk("%s: error resample ratio %d\n", __func__, audio_job->resample_ratio);
        retval = PROCESS_FAILED;
        goto decode_finish;
    }

    switch (codec_type) {
        case AU_TYPE_PCM:
            GM_LOG(GM_LOG_PB_PKT_FLOW, "job(%d) encode PCM\n", job_id);
            break;
#if defined(AUDIO_CODEC)
        case AU_TYPE_AAC:
            GM_LOG(GM_LOG_PB_PKT_FLOW, "job(%d) encode AAC\n", job_id);
            if (audio_job->dec_blk_sz != AAC_FRAME_SZ) {
                printk("%s: AAC decode block size = %u, not %u\n", __func__, audio_job->dec_blk_sz, AAC_FRAME_SZ);
                retval = PROCESS_FAILED;
                goto decode_finish;
            }

            // re-init AAC codec if nChannels changed
            if (aac_nChannels != audio_job->channel_type) {
                is_aac_dec_init = false;
                GM_AAC_Dec_Destory(aac_dec_hdl, &aac_dec_buf, &aac_dec_info);
                aac_nChannels = audio_job->channel_type;
            }
            
            break;
        case AU_TYPE_ADPCM:
            GM_LOG(GM_LOG_PB_PKT_FLOW, "job(%d) encode ADPCM\n", job_id);
            if (audio_job->dec_blk_sz > 0)
                adpcm_codec_setting.ima_paras.nBlockAlign = audio_job->dec_blk_sz;
            else
                adpcm_codec_setting.ima_paras.nBlockAlign = BLOCKALIGN;

            if (audio_job->channel_type > 0) {
                if (audio_job->channel_type != adpcm_codec_setting.ima_paras.nChannels) {
                    is_adpcm_dec_init = false;
                    adpcm_codec_setting.ima_paras.nChannels = audio_job->channel_type;
                }
            } else {
                adpcm_codec_setting.ima_paras.nChannels = 1;
            }
            break;
        case AU_TYPE_ALAW:
        case AU_TYPE_ULAW:
            GM_LOG(GM_LOG_PB_PKT_FLOW, "job(%d) encode A/U LAW\n", job_id);
            g711_dec_setting.decode_error = 0;
            g711_dec_setting.decode_mode = 3; //0: AUTO: 1:AAC 2:ADPCM 3:aulaw

            if (audio_job->dec_blk_sz > 0)
                g711_dec_setting.ima_paras.nBlockAlign = audio_job->dec_blk_sz;
            else
                g711_dec_setting.ima_paras.nBlockAlign = BLOCKALIGN;

            if (audio_job->channel_type > 0)
                g711_dec_setting.ima_paras.nChannels = audio_job->channel_type;
            else
                g711_dec_setting.ima_paras.nChannels = 1;

            if (codec_type == AU_TYPE_ALAW)
                g711_dec_setting.aulaw_codec_pars.aulaw_dec_option = ALAW2LINEAR;
            else
                g711_dec_setting.aulaw_codec_pars.aulaw_dec_option = ULAW2LINEAR;

            g711_dec_setting.aulaw_codec_pars.block_size = AULAW_BLK_SZ;
            g711_dec_setting.aulaw_codec_pars.Decoded_Sample_Num = AULAW_BLK_SZ;

            if (g711_curr_dec_type != codec_type) {
                is_g711_dec_init = false;
                if (inparam_dec)
                    g711dec_destory(inparam_dec);
            }
            g711_curr_dec_type = codec_type;
           
            break;
#endif
        default:
            printk("%s: No such codec type(%d)\n", __func__, codec_type);
            retval = PROCESS_FAILED;
            goto decode_finish;
    }

    if (codec_type < AU_TYPE_PCM || codec_type >= AU_TYPE_MAX) {
        printk("%s: error codec type %d\n", __func__, audio_job->codec_type);
        retval = PROCESS_FAILED;
        goto decode_finish;
    }

    GM_LOG(GM_LOG_PB_PKT_FLOW, "job(%d) start decode\n", job_id);

    retval = gm_codec_funs[codec_type].decode((void *)audio_job, play_buf, play_sz);

    GM_LOG(GM_LOG_PB_PKT_FLOW, "job(%d) decode done\n", job_id);

    *play_dbg_stage |= (1 << OUT_DEC_DONE);

decode_finish:
    if (retval > PROCESS_SUCCESS)
        audio_job->status = JOB_STS_DECERR;
    else if (audio_job->status != JOB_STS_DEC_TIMEOUT)
        audio_job->status = JOB_STS_ACK;

    audio_job_check(audio_job);

    *play_dbg_stage |= (1 << OUT_JOB_BACK);
}
EXPORT_SYMBOL(audio_out_handler);

int audio_flow_init(void)
{
    int ret = PROCESS_SUCCESS;
    int ssp_max = au_get_ssp_max();
    int i = 0;

#if defined(MEMCPY_DMA)
    if (!live_buf_data) {
        if (DMA_ALIGN_CHECK(LIVE_CH_SZ))
            live_buf_data = (unsigned char *)fmem_alloc_ex(LIVE_CH_SZ, &live_buf_paddr, PAGE_SHARED, DDR_ID_SYSTEM);
        else
            live_buf_data = (unsigned char *)dma_alloc_writecombine(NULL, LIVE_CH_SZ, &live_buf_paddr, GFP_KERNEL);
        if (live_buf_data == NULL) {
            printk("%s: failed to allocate dma buff with size: %d bytes\n", __func__, LIVE_CH_SZ);
            ret = -1;
            goto out0;
        }
    }
    if (!max_resample_buf) {
        if (DMA_ALIGN_CHECK(MAX_RESAMPLE_SZ))
            max_resample_buf = (unsigned char *)fmem_alloc_ex(MAX_RESAMPLE_SZ, &max_resample_buf_paddr, PAGE_SHARED, DDR_ID_SYSTEM);
        else
            max_resample_buf = (unsigned char *)dma_alloc_writecombine(NULL, MAX_RESAMPLE_SZ, &max_resample_buf_paddr, GFP_KERNEL);
        if (max_resample_buf == NULL) {
            printk("%s: failed to allocate dma buff with size: %d bytes\n", __func__, MAX_RESAMPLE_SZ);
            ret = -1;
            goto out1;
        }
    }
#if defined(AUDIO_CODEC)
    if (!adpcm_dec_output_buf) {
        adpcm_dec_output_buf = (short *)dma_alloc_writecombine(NULL, IMA_BLK_SZ_MONO, &adpcm_dec_output_buf_paddr, GFP_KERNEL);
        if (adpcm_dec_output_buf == NULL) {
            printk("%s: failed to allocate dma buff with size: %d bytes\n", __func__, MAX_RESAMPLE_SZ);
            ret = -1;
            goto out2;
        }
    }
#endif
#endif

    if (!silence_buf) {
        silence_buf = (unsigned char *)dma_alloc_writecombine(NULL, MUTE_BUF_SZ, &silence_buf_paddr, GFP_KERNEL);
        if (silence_buf == NULL) {
            printk("%s: failed to allocate dma buffer with size: %d bytes\n", __func__, MUTE_BUF_SZ);
            ret = -1;
            goto out4;
        }
    }

    for (ret = 0;ret < au_get_ssp_max() - 1;ret ++) {
#if defined(MEMCPY_DMA)
        avg_resample_buf[ret] = NULL;
        if (DMA_ALIGN_CHECK(AVG_RESAMPLE_SZ))
            avg_resample_buf[ret] = (unsigned char *)fmem_alloc_ex(AVG_RESAMPLE_SZ, &avg_resample_buf_paddr[ret], PAGE_SHARED, DDR_ID_SYSTEM);
        else
            avg_resample_buf[ret] = (unsigned char *)dma_alloc_writecombine(NULL, AVG_RESAMPLE_SZ, &avg_resample_buf_paddr[ret], GFP_KERNEL);
        if (avg_resample_buf[ret] == NULL) {
            printk("%s: failed to allocate dma buff with size: %d bytes\n", __func__, AVG_RESAMPLE_SZ);
            ret = -1;
            goto out3;
        }
#endif
        resample_buffer[ret] = avg_resample_buf[ret];
        resample_size[ret] = AVG_RESAMPLE_SZ;
    }
    resample_buffer[ret] = max_resample_buf;
    resample_size[ret] = MAX_RESAMPLE_SZ;

    /* ex. 4 channel audio data in memory: |0|1|2|3|4|, data in i2s: |1|0|(left), |3|2|(right) */
    for (i = 0;i < ssp_max;i ++) {
        audio_param_t *drv_param = au_get_param();
        if (drv_param->out_enable[i] && drv_param->ssp_chan[i] > 2 && (drv_param->ssp_chan[i] % 2 == 0)) {
            output_ch[i] = (drv_param->ssp_chan[i] / 2) - 1;
        }
    }

    ret = audio_drv_init();

#if defined(AUDIO_CODEC)
    memset(&adpcm_codec_setting, 0, sizeof(AUDIO_CODEC_SETTING));
    memset(&g711_dec_setting, 0, sizeof(AUDIO_CODEC_SETTING));
#endif

    if (ret < 0)
        goto out0;

    /* open audio device */
    if (audio_gm_open(&audio_handle) < 0)
        panic("%s: Open audio handler failed\n", __func__);

    rec_dbg_stage = audio_flow_get_rec_stage();
    play_dbg_stage = audio_flow_get_play_stage();

    return ret;
out4:
    if (silence_buf) {
        dma_free_writecombine(NULL, MUTE_BUF_SZ, silence_buf, silence_buf_paddr);
        silence_buf = NULL;
    }
#if defined(MEMCPY_DMA)
out3:
    for (i = 0;i < ret;i ++) {
        if (avg_resample_buf[i]) {
            if (DMA_ALIGN_CHECK(AVG_RESAMPLE_SZ))
                fmem_free_ex(AVG_RESAMPLE_SZ, avg_resample_buf[i], avg_resample_buf_paddr[i]);
            else
                dma_free_writecombine(NULL, AVG_RESAMPLE_SZ, avg_resample_buf[i], avg_resample_buf_paddr[i]);
            avg_resample_buf[i] = NULL;
        }
    }
#if defined(AUDIO_CODEC)
out2:
    if (max_resample_buf) {
        if (DMA_ALIGN_CHECK(MAX_RESAMPLE_SZ))
            fmem_free_ex(MAX_RESAMPLE_SZ, max_resample_buf, max_resample_buf_paddr);
        else
            dma_free_writecombine(NULL, MAX_RESAMPLE_SZ, max_resample_buf, max_resample_buf_paddr);
        max_resample_buf = NULL;
    }
#endif
out1:
    if (live_buf_data) {
        if (DMA_ALIGN_CHECK(LIVE_CH_SZ))
            fmem_free_ex(LIVE_CH_SZ, live_buf_data, live_buf_paddr);
        else
            dma_free_writecombine(NULL, LIVE_CH_SZ, live_buf_data, live_buf_paddr);
        live_buf_data = NULL;
    }
#endif
out0:
    return ret;
}
EXPORT_SYMBOL(audio_flow_init);

void audio_flow_exit(void)
{
#if defined(MEMCPY_DMA)
    int i;
#endif
    audio_gm_release(&audio_handle);
    audio_drv_exit();
#if defined(MEMCPY_DMA)
    if (live_buf_data) {
        if (DMA_ALIGN_CHECK(LIVE_CH_SZ))
            fmem_free_ex(LIVE_CH_SZ, live_buf_data, live_buf_paddr);
        else
            dma_free_writecombine(NULL, LIVE_CH_SZ, live_buf_data, live_buf_paddr);
        live_buf_data = NULL;
    }

    if (max_resample_buf) {
        if (DMA_ALIGN_CHECK(MAX_RESAMPLE_SZ))
            fmem_free_ex(MAX_RESAMPLE_SZ, max_resample_buf, max_resample_buf_paddr);
        else
            dma_free_writecombine(NULL, MAX_RESAMPLE_SZ, max_resample_buf, max_resample_buf_paddr);
        max_resample_buf = NULL;
    }

#if defined(AUDIO_CODEC)
    if (adpcm_dec_output_buf) {
        dma_free_writecombine(NULL, IMA_BLK_SZ_MONO, adpcm_dec_output_buf, adpcm_dec_output_buf_paddr);
        adpcm_dec_output_buf = NULL;
    }
#endif

    for (i = 0;i < au_get_ssp_max() - 1;i ++) {
        if (avg_resample_buf[i]) {
            if (DMA_ALIGN_CHECK(AVG_RESAMPLE_SZ))
                fmem_free_ex(AVG_RESAMPLE_SZ, avg_resample_buf[i], avg_resample_buf_paddr[i]);
            else
                dma_free_writecombine(NULL, AVG_RESAMPLE_SZ, avg_resample_buf[i], avg_resample_buf_paddr[i]);
            avg_resample_buf[i] = NULL;
        }
    }
#endif

    if (silence_buf) {
        dma_free_writecombine(NULL, MUTE_BUF_SZ, silence_buf, silence_buf_paddr);
        silence_buf = NULL;
    }

}
EXPORT_SYMBOL(audio_flow_exit);
