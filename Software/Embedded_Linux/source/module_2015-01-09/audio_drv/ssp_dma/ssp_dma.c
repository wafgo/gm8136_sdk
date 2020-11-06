#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/bitops.h>
#include <linux/spinlock.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/ftdmac020.h>
#include <mach/ftpmu010.h>
#include <mach/fmem.h>
#include <mach/platform/platform_io.h>
#include "frammap_if.h"
#include "ssp_dma.h"
#include "../audio_vg.h"
#include "../audio_proc.h"
#include "../../front_end/platform.h"
#include "log.h"    //include log system "printm","damnit"...

/* audio NR */
#include "nr.h"
#include "fxpt_fft.h"

#if !defined(FIXED_DMA_CHAN)
#include <mach/dma_gm.h>
#endif

#if defined(CONFIG_PLATFORM_GM8139)
#include <adda302_api.h>
#endif
#if defined(CONFIG_PLATFORM_GM8136)
#include <adda308_api.h>
#endif

#define AUDIO_SSP_DEBUG
#undef AUDIO_SSP_DEBUG
#ifdef AUDIO_SSP_DEBUG
#define DBG(fmt,args...) printk(KERN_INFO"<AUDIO_DRV> "fmt, ## args)
#else
#define DBG pr_debug
//#define DBG(fmt,args...) printk(KERN_DEBUG"<AUDIO_DRV> "fmt, ## args)
#endif

#if (HZ == 1000)
    #define get_gm_jiffies()                (jiffies)
#else
    #include <mach/gm_jiffies.h>
#endif

#define RESET_AUDIO_BUFFER
//#undef RESET_AUDIO_BUFFER

#define SSP_BASE(x) (SSP_FTSSP010_0_PA_BASE + 0x100000 * x)

#if defined(CONFIG_PLATFORM_GM8139)
bclk_tbl_t bclk_tbl[] = {
    /* sample_sz, sample_rate, bit_clock, main_clock */
    {16, 8000,  400000,  12000000},       /* bit_clock: reference to ADDA302 datasheet */
    {16, 16000, 800000,  12000000},
    {16, 22050, 1500000, 12000000},
    {16, 32000, 1600000, 16000000},
    {16, 44100, 3000000, 12000000},
    {16, 48000, 2400000, 12000000},
};
#endif

#if defined(CONFIG_PLATFORM_GM8136)
bclk_tbl_t bclk_tbl_spk[] = {
    {
        .sample_sz      = 16,
        .sample_rate    = 8000,
        .bclk           = 512000,
        .mclk           = 24545454
    },
    {
        .sample_sz      = 16,
        .sample_rate    = 16000,
        .bclk           = 1024000,
        .mclk           = 24545454
    },
    {
        .sample_sz      = 16,
        .sample_rate    = 22505,
        .bclk           = 1499400,
        .mclk           = 24545454
    },
    {
        .sample_sz      = 16,
        .sample_rate    = 32000,
        .bclk           = 2048000,
        .mclk           = 24545454
    },
    {
        .sample_sz      = 16,
        .sample_rate    = 44100,
        .bclk           = 2998800,
        .mclk           = 24545454
    },
    {
        .sample_sz      = 16,
        .sample_rate    = 48000,
        .bclk           = 3072000,
        .mclk           = 24545454
    }
};

bclk_tbl_t bclk_tbl_lineout[] = {
    {
        .sample_sz      = 16,
        .sample_rate    = 8000,
        .bclk           = 400000,
        .mclk           = 24000000
    },
    {
        .sample_sz      = 16,
        .sample_rate    = 16000,
        .bclk           = 800000,
        .mclk           = 24000000
    },
    {
        .sample_sz      = 16,
        .sample_rate    = 22505,
        .bclk           = 1499400,
        .mclk           = 24000000
    },
    {
        .sample_sz      = 16,
        .sample_rate    = 32000,
        .bclk           = 2048000,
        .mclk           = 24000000
    },
    {
        .sample_sz      = 16,
        .sample_rate    = 44100,
        .bclk           = 2998800,
        .mclk           = 24000000
    },
    {
        .sample_sz      = 16,
        .sample_rate    = 48000,
        .bclk           = 2400000,
        .mclk           = 24000000
    }
};
#endif

static u32 dmac020_base = 0;

struct dma_gm {
    struct dma_chan *chan;
    struct dma_async_tx_descriptor *desc;
    enum dma_status dma_st;
    dma_cap_mask_t mask;
    dma_cookie_t cookie;
    wait_queue_head_t dma_wait_queue;
    struct ftdmac020_dma_slave slave;
};

#if defined(FIXED_DMA_CHAN)
/* for dma memcpy */
static void dma_callback(void *param)
{
    struct dma_gm *gm = (struct dma_gm *)param;
    gm->dma_st = DMA_SUCCESS;
    wake_up(&gm->dma_wait_queue);
}

static void dma_wait(struct dma_gm *gm)
{
    int status;

    status = wait_event_timeout(gm->dma_wait_queue,
                                              gm->dma_st == DMA_SUCCESS, 60 * HZ);
    if (status == 0) {
        printk("DMA_CH(%d) Timeout in DMA\n", gm->chan->chan_id);
    }
}
#endif

typedef struct _audio_stream_t {
    dma_addr_t dma_buffer;              // start addr(physical) of dma ring buffer
    void *kbuffer;                      // start addr(virtual) of dma ring buffer
    void *pcm_buffer;                   // start addr of pcm ring buffer
    u32 pcm_ch_buf[AUDIO_SSP_CHAN];     // start addr of each pcm channel buffer
    u32 pcm_ch_ofs[AUDIO_SSP_CHAN];     // offset of each pcm channel buffer
    void *nr_buffer;                    // start addr of nr ring buffer
    u32 nr_ch_buf[AUDIO_SSP_CHAN];      // start addr of each nr channel buffer
    u32 nr_ch_ofs[AUDIO_SSP_CHAN];      // offset of each br channel buffer
    short *nr_buf_pad[AUDIO_SSP_CHAN];  // pad buffer for NRv

    unsigned int sample_sz;
    unsigned int sample_rate;
    unsigned int buffer_sz;

    struct {
        unsigned int dma_read_pos;  // dma buffer read index of total DEFAULT_LLP_CNT nodes
        unsigned int dma_write_pos; // dma buffer write index of total DEFAULT_LLP_CNT nodes
        unsigned int dma_read_ofs;  // dma buffer read offset in a node
        unsigned int dma_write_ofs; // dma buffer write offset in a node
        unsigned int pcm_read_pos;  // pcm buffer read index of total DEFAULT_LLP_CNT nodes
        unsigned int pcm_write_pos; // pcm buffer write index of total DEFAULT_LLP_CNT nodes
        unsigned int pcm_read_ofs;  // pcm buffer read offset in a node
        unsigned int pcm_write_ofs; // pcm buffer write offset in a node
        unsigned int read_pos;      // nr buffer read index of total DEFAULT_LLP_CNT nodes
        unsigned int write_pos;     // nr buffer write index of total DEFAULT_LLP_CNT nodes
        unsigned int read_ofs;      // nr buffer read offset in a node
        unsigned int write_ofs;     // nr buffer write offset in a node
    } buf_idx;

    /* platform dependent */
    bool active;                //active or inactive
    void *board;                //private data to audio board
    void *handler;

    struct dma_gm cyclic_dma;
#if defined(FIXED_DMA_CHAN)
    struct dma_gm memcpy_dma;
#endif
} audio_stream_t;

typedef struct _audio_board_t {
    void __iomem *ssp_io_base;  /* ssp virtual address */
    u32 bclk;                   /* bit clock */
    u32 mclk;                   /* main clock */
    u32 sample_rate;
    int is_master;              /* i2s is the master ? */
    int is_stereo;              /* i2s is the stereo ? */
    int dma_llp_cnt;
    int max_period_sz;          /* the total memory will be (max_period_sz * dma_llp_cnt) */
    int total_channel;
    audio_stream_t stream[STREAM_DIR_MAX];
    bool is_cacheable_dma;
    bool is_fifo_overrun;
    bool is_fifo_underrun;
    unsigned long frame_jiffies[DEFAULT_LLP_CNT];
} audio_board_t;

static spinlock_t audio_lock;
static int *ssp_handler = NULL;

/* audio driver parameters */
static audio_param_t *drv_param;

static int ssp_dma_req[][STREAM_DIR_MAX] = {
#if defined(CONFIG_PLATFORM_GM8287)
    {10, 9},
    {12, 11},
    {14, 13},
#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    {10, 9},
    {12, 11},
#endif
};

static u32 ssp_irq_num[] = {
#if defined(CONFIG_PLATFORM_GM8287)
    SSP_FTSSP010_0_IRQ, SSP_FTSSP010_1_IRQ, SSP_FTSSP010_2_IRQ
#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    SSP_FTSSP010_0_IRQ, SSP_FTSSP010_1_IRQ
#endif
};

static char *ssp_irq_name[] = {
#if defined(CONFIG_PLATFORM_GM8287)
    "ssp_irq_0", "ssp_irq_1", "ssp_irq_2"
#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    "ssp_irq_0", "ssp_irq_1"
#endif
};

#define AAC_FRAME_SZ        1024
#define AAC_FRAME_TIME      128 // (AAC_FRAME_SZ * 1000 / DEFAULT_SAMPLE_RATE) milisecond
#define NODE_BUF_SZ         (AAC_FRAME_SZ * DEFAULT_SAMPLE_SZ / BITS_PER_BYTE)

#if defined(CONFIG_PLATFORM_GM8136)
static int adda_output_mode = SPK_OUTPUT;
#endif

/*
 * Main structure
 */
static audio_board_t *audio_brd[SSP_MAX_USED];

static int audio_drv_stream_init(int dir, audio_stream_t * stream, void *board)
{
    audio_board_t *brd = (audio_board_t *) board;
    u32 total_dma_sz, brd_idx;

    if (stream->active) {
        printk("Error, this stream was init already! \n");
        return -1;
    }

    memset(stream, 0, sizeof(audio_stream_t));
    stream->sample_sz = DEFAULT_SAMPLE_SZ;
    stream->sample_rate = brd->sample_rate;
    stream->buffer_sz = brd->max_period_sz;

    if (stream->buffer_sz > brd->max_period_sz) {
        printk("The need buffer size = %d bytes but only has %d bytes!\n", stream->buffer_sz,
               brd->max_period_sz);
        return -1;
    }

    for (brd_idx = 0; brd_idx < au_get_ssp_max(); brd_idx++)
        if ((u32) (brd->ssp_io_base) == (u32) (audio_brd[brd_idx]->ssp_io_base))
            break;
    if (drv_param->ssp_chan[brd_idx] > 1)
        goto set_cyclic;

#if defined(CONFIG_PLATFORM_GM8287)
    /* no record for i2s connected to HDMI */
    if (drv_param->ssp_num[brd_idx] == 2 && dir == STREAM_DIR_RECORD)
        goto set_stream_done;
#endif

#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    /* only i2s0 connected to adda302 can do record */
    if (brd_idx >= 1 && dir == STREAM_DIR_RECORD && au_get_in_enable()[brd_idx]==0)
        goto set_stream_done;
#endif

#if defined(FIXED_DMA_CHAN)
    /* prepare dma memcpy */
    memset(&stream->memcpy_dma.slave, 0, sizeof(stream->memcpy_dma.slave));
    init_waitqueue_head(&stream->memcpy_dma.dma_wait_queue);
    dma_cap_zero(stream->memcpy_dma.mask);
    dma_cap_set(DMA_MEMCPY, stream->memcpy_dma.mask);
    stream->memcpy_dma.slave.id = 0;
    stream->memcpy_dma.slave.handshake = -1;
    stream->memcpy_dma.slave.src_sel = 1;
    stream->memcpy_dma.slave.dst_sel = 1;
    stream->memcpy_dma.chan =
        dma_request_channel(stream->memcpy_dma.mask, ftdmac020_chan_filter,
                            (void *)&stream->memcpy_dma.slave);
    if (!stream->memcpy_dma.chan) {
        printk("Error, memcpy dma_chan is NULL\n");
        return -1;
    }
    printk("ssp: %d, direction: %s, memcpy dma_ch: %d\n", brd_idx, dir ? "playback" : "record",
           stream->memcpy_dma.chan->chan_id);
#endif

set_cyclic:
    /* Allocate the max buffer size in order to meet the change of sample rate or size */
    total_dma_sz = brd->max_period_sz * brd->dma_llp_cnt;

    if (dir == STREAM_DIR_PLAYBACK && drv_param->out_enable[brd_idx] == 0)
        goto set_stream_done;

    if (brd->is_cacheable_dma)
        stream->kbuffer =
            fmem_alloc_ex(total_dma_sz, &stream->dma_buffer, PAGE_SHARED, DDR_ID_SYSTEM);
    else
        stream->kbuffer =
            dma_alloc_writecombine(NULL, total_dma_sz, &stream->dma_buffer, GFP_KERNEL);
    if (stream->kbuffer == NULL) {
        printk("failed to allocate dma buff with size: %d bytes\n", total_dma_sz);
        return -1;
    }

    /* allocate PCM and NR buffer size */
    if (dir == STREAM_DIR_RECORD) {
        int i = 0;

        if (brd->total_channel > 1) {
            stream->pcm_buffer = kmalloc(total_dma_sz, GFP_KERNEL);

            if (stream->pcm_buffer == NULL) {
                panic("failed to allocate pcm buffer with size: %d bytes\n", total_dma_sz);
                return -1;
            }
        } else {
            stream->pcm_buffer = stream->kbuffer;
        }

        /* each node of pcm_ch_buf is equal to NODE_BUF_SZ, and total "brd->dma_llp_cnt" nodes */
        for (i = 0;i < brd->total_channel;i ++)
            stream->pcm_ch_buf[i] = (u32)stream->pcm_buffer + i * NODE_BUF_SZ * brd->dma_llp_cnt;

        if ((NODE_BUF_SZ / sizeof(short)) % NR_FRM_SZ_256 == 0 && drv_param->nr_enable[brd_idx]) {

            stream->nr_buffer = kmalloc(total_dma_sz, GFP_KERNEL);

            if (stream->nr_buffer == NULL) {
                panic("failed to allocate nr buffer with size: %d bytes\n", total_dma_sz);
                return -1;
            }

            for (i = 0;i < brd->total_channel;i ++) {
                stream->nr_buf_pad[i] = (short *)kmalloc(NR_FRM_OV_SZ_128 * sizeof(short), GFP_KERNEL);
                if (stream->nr_buf_pad[i] == NULL) {
                    panic("failed to allocate nr_buf_pad[%d] failed\n", i);
                    return -1;
                }
            }
        } else {
            stream->nr_buffer = stream->pcm_buffer;
        }

        /* each node of nr_ch_buf is equal to NODE_BUF_SZ, and total "brd->dma_llp_cnt" nodes */
        for (i = 0;i < brd->total_channel;i ++)
            stream->nr_ch_buf[i] = (u32)stream->nr_buffer + i * NODE_BUF_SZ * brd->dma_llp_cnt;
    }

    dma_cap_zero(stream->cyclic_dma.mask);
    dma_cap_set(DMA_CYCLIC, stream->cyclic_dma.mask);

    /* platform dependent */
    switch (dir) {
        case STREAM_DIR_RECORD:
            stream->cyclic_dma.slave.id = 0;
            stream->cyclic_dma.slave.src_sel = FTDMA020_AHBMASTER_0;
            stream->cyclic_dma.slave.dst_sel = FTDMA020_AHBMASTER_1;
            stream->cyclic_dma.slave.handshake = ssp_dma_req[drv_param->ssp_num[brd_idx]][STREAM_DIR_RECORD];
            stream->cyclic_dma.slave.src_size = FTDMAC020_BURST_SZ_4;
            stream->cyclic_dma.slave.common.src_addr_width =
                ((brd->total_channel < 3) ? DMA_SLAVE_BUSWIDTH_2_BYTES : DMA_SLAVE_BUSWIDTH_4_BYTES);
            stream->cyclic_dma.slave.common.dst_addr_width =
                ((brd->total_channel < 3) ? DMA_SLAVE_BUSWIDTH_2_BYTES : DMA_SLAVE_BUSWIDTH_4_BYTES);
            stream->cyclic_dma.slave.common.src_addr = SSP_BASE(drv_param->ssp_num[brd_idx]) + _SSP_DATA;
            stream->cyclic_dma.slave.common.dst_addr = stream->dma_buffer;
            stream->cyclic_dma.slave.common.direction = DMA_DEV_TO_MEM;
            stream->cyclic_dma.chan =
                dma_request_channel(stream->cyclic_dma.mask, ftdmac020_chan_filter,
                        (void *)&stream->cyclic_dma.slave);
            if (!stream->cyclic_dma.chan)
                panic("%s, %s: request FTDMAC020 chan for rx failed\n", __FILE__, __func__);
            printk("Record(SSP:%d): stream->dma_buffer = %x(%p), size:%x, dma_ch: %d \n", brd_idx,
                    stream->dma_buffer, stream->kbuffer, brd->max_period_sz, stream->cyclic_dma.chan->chan_id);
            break;
        case STREAM_DIR_PLAYBACK:
            stream->cyclic_dma.slave.id = 0;
            stream->cyclic_dma.slave.src_sel = FTDMA020_AHBMASTER_1;
            stream->cyclic_dma.slave.dst_sel = FTDMA020_AHBMASTER_0;
            stream->cyclic_dma.slave.handshake = ssp_dma_req[drv_param->ssp_num[brd_idx]][STREAM_DIR_PLAYBACK];
            stream->cyclic_dma.slave.src_size = FTDMAC020_BURST_SZ_4;
            stream->cyclic_dma.slave.common.src_addr_width =
                ((brd->total_channel < 2) ? DMA_SLAVE_BUSWIDTH_2_BYTES : DMA_SLAVE_BUSWIDTH_4_BYTES);
            stream->cyclic_dma.slave.common.dst_addr_width =
                ((brd->total_channel < 2) ? DMA_SLAVE_BUSWIDTH_2_BYTES : DMA_SLAVE_BUSWIDTH_4_BYTES);
            stream->cyclic_dma.slave.common.src_addr = stream->dma_buffer;
            stream->cyclic_dma.slave.common.dst_addr = SSP_BASE(drv_param->ssp_num[brd_idx]) + _SSP_DATA;
            stream->cyclic_dma.slave.common.direction = DMA_MEM_TO_DEV;
            stream->cyclic_dma.chan =
                dma_request_channel(stream->cyclic_dma.mask, ftdmac020_chan_filter,
                        (void *)&stream->cyclic_dma.slave);
            if (!stream->cyclic_dma.chan)
                panic("%s, %s: request FTDMAC020 chan for tx failed\n", __FILE__, __func__);
            printk("Playback(SSP:%d): stream->dma_buffer = %x(%p), size:%x, dma_ch: %d\n", brd_idx,
                    stream->dma_buffer, stream->kbuffer, brd->max_period_sz, stream->cyclic_dma.chan->chan_id);
            break;
        default:
            panic("error direction! \n");
            break;
    }

    stream->active = true;
set_stream_done:
    stream->board = board;
    return 0;
}

/* ssp init
 */
static void audio_ssp_init(audio_board_t * brd)
{
    const u32 mclk = brd->mclk;
    const u32 sample_sz = DEFAULT_SAMPLE_SZ;
    const u32 sample_rate = brd->sample_rate;
    const void __iomem * ssp_io_base = brd->ssp_io_base;
    u32 bclk = brd->bclk;
    u32 tmp, sdl, pdl;
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)    
    bclk_tbl_t *bclk_table = NULL;
#endif

#if defined(CONFIG_PLATFORM_GM8139)
    enum adda302_fs_frequency adda_fs = ADDA302_FS_8K;
    bclk_table = bclk_tbl;
#elif defined(CONFIG_PLATFORM_GM8136)
    enum adda308_fs_frequency adda_fs = ADDA302_FS_8K;
    adda_output_mode = adda308_get_output_mode();
    bclk_table = (adda_output_mode == SPK_OUTPUT)?(bclk_tbl_spk):(bclk_tbl_lineout);
#endif

#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    switch (sample_rate) {
        case 8000:
            adda_fs = ADDA302_FS_8K;
            bclk = bclk_table[ADDA302_FS_8K].bclk;
            break;
        case 16000:
            adda_fs = ADDA302_FS_16K;
            bclk = bclk_table[ADDA302_FS_16K].bclk;
            break;
        case 22050:
            adda_fs = ADDA302_FS_22_05K;
            bclk = bclk_table[ADDA302_FS_22_05K].bclk;
            break;
        case 32000:
            adda_fs = ADDA302_FS_32K;
            bclk = bclk_table[ADDA302_FS_32K].bclk;
            break;
        case 44100:
            adda_fs = ADDA302_FS_44_1K;
            bclk = bclk_table[ADDA302_FS_44_1K].bclk;
            break;
        case 48000:
            adda_fs = ADDA302_FS_48K;
            bclk = bclk_table[ADDA302_FS_48K].bclk;
            break;
    }

    if ((u32) (brd->ssp_io_base) == (u32) (audio_brd[0]->ssp_io_base)) {
        adda302_set_ad_fs(adda_fs);
        brd->bclk = bclk;
        if (drv_param->out_enable[0] == 1) {
            adda302_set_da_fs(adda_fs);
        }
    } else {
        /* check whether ssp1 is connected on adda302 or external codec */
        u32 reg = ftpmu010_read_reg(0x7C);
        if ((reg & (1 << 27)) == 0) { //ssp0
            brd->bclk = bclk;
            adda302_set_da_fs(adda_fs);
        } else {
#if 0
            bclk = brd->bclk;
#else
            brd->bclk = bclk;
#endif
        }
    }
#endif

    tmp = 0;
    tmp = (0x1 << 6)            /* SSPRST */
        |(0x1 << 3)             /* TXFCLR */
        |(0x1 << 2);            /* RXFCLR */
    iowrite32(tmp, ssp_io_base + _SSP_CTRL2);

    tmp = 0;
    tmp = (0x3 << 12)           /* I2S */
        |(0x1 << 8)             /* FDIST = 1 */
        |(0x1 << 5);            /* Active Low */

    if (brd->is_master)
        tmp |= (0x1 << 3);      /* Master */
    if (brd->is_stereo)
        tmp |= (0x1 << 2);      /* Stereo */
    iowrite32(tmp, ssp_io_base + _SSP_CTRL0);

    /* total_channel > 1 ==> stereo, others ==> mono or stereo */
    sdl = (brd->total_channel == 1) ? sample_sz : (sample_sz * brd->total_channel / 2);

    /* bclk = fs * total_channels * sample_size.
     * frame bit for each side = bclk / (fs * 2)
     */
    pdl = bclk / (sample_rate * 2) - sdl;

    tmp = 0;
    DBG("%s: bclk = %u, mclk = %u, sample_rate = %u\n", __func__, bclk, mclk, sample_rate);
    DBG("%s: pdl = %u, sdl = %u, total_channel = %u\n", __func__, pdl, sdl, brd->total_channel);

    /* count SCLKDIV in _SSP_CTRL1, sclk = sspclk/(2*(divisor+1)) */
    if (brd->is_master) {
        u32 quotient = mclk / (2 * bclk);
        u32 remainder = mclk % (2 * bclk);

        (remainder > bclk) ? (tmp = quotient) : (tmp = quotient - 1);

        if (tmp <= 0)
            tmp = 1;
    }
    tmp |= ((pdl & 0xFF) << 24) | (((sdl - 1) & 0x7F) << 16);

    iowrite32(tmp, ssp_io_base + _SSP_CTRL1);

    /* fill tx fifo */
    do {
        iowrite32(0x0, ssp_io_base + _SSP_DATA);
        tmp = ioread32(ssp_io_base + _SSP_STAT);
    } while (tmp & 0x2);

    tmp = (((SSP_FIFO_DEPTH - SSP_RXFIFO_THOD) & 0x1F) << 12) /* TFTHOD */
        | (SSP_RXFIFO_THOD << 7)    /* RFTHOD */
        | (0x1 << 5)                /* TFDMAEN */
        | (0x1 << 4)                /* RFDMAEN */
        | (0x1 << 1)                /* TFURIEN */
        | (0x1 << 0);               /* RFORIEN */
    iowrite32(tmp, ssp_io_base + _SSP_INTRCTRL);

    return;
}

/* on_off: 1 for enable, 0 for disable
 */
static void audio_ssp_rx_enable(void __iomem * ssp_io_base, int on_off)
{
    u32 tmp, tmp2;

    GM_LOG(GM_LOG_REC_PKT_FLOW, "%s(%d) ssp_io_base(0x%x)\n", __func__, on_off, (unsigned int)ssp_io_base);

    tmp = ioread32(ssp_io_base + _SSP_CTRL2);
    tmp2 = ioread32(ssp_io_base + _SSP_INTRCTRL);
    if (on_off) {
        tmp |= (0x1 << 2); //RXFCLR
        iowrite32(tmp, ssp_io_base + _SSP_CTRL2);

        /* clear SSP irq status */
        while (ioread32(ssp_io_base + _SSP_INTRSTAT) & 0x1);

        tmp |= ((0x1 << 7) | 0x1); //RXEN and SSPEN
        tmp2 |= (0x1 << 4); //RFDMAEN
    } else {
        tmp &= ~(0x1 << 7);
        tmp2 &= ~(0x1 << 4); //rx req dma disable
    }

    iowrite32(tmp, ssp_io_base + _SSP_CTRL2);
    iowrite32(tmp2, ssp_io_base + _SSP_INTRCTRL);
}

/* on_off: 1 for enable, 0 for disable
 */
static void audio_ssp_tx_enable(void __iomem * ssp_io_base, int on_off)
{
    u32 tmp, tmp2;
    tmp = ioread32(ssp_io_base + _SSP_CTRL2);
    tmp2 = ioread32(ssp_io_base + _SSP_INTRCTRL);
    ioread32(ssp_io_base + _SSP_INTRSTAT);

    if (on_off) {
        iowrite32(tmp | (0x1 << 3), ssp_io_base + _SSP_CTRL2);  //fifo clear

        //full the fifo
        while (ioread32(ssp_io_base + _SSP_STAT) & 0x2)
            iowrite32(0x0, ssp_io_base + _SSP_DATA);

        /* clear SSP irq status */
        while (ioread32(ssp_io_base + _SSP_INTRSTAT) & 0x2);

        tmp2 |= (0x3 | (0x1 << 5)); // tx req dma enable
        iowrite32(tmp2, ssp_io_base + _SSP_INTRCTRL);

        tmp |= ((0x1 << 8) | 0x3);
        iowrite32(tmp, ssp_io_base + _SSP_CTRL2);
    } else {
        int i;
        u32 fifo_data = 0;

        tmp2 &= ~(0x1 << 5); // tx req dma disable
        iowrite32(tmp2, ssp_io_base + _SSP_INTRCTRL);

        tmp &= ~(0x1 << 8);
        tmp |= (0x1 << 3);  /* fifo clear */
        iowrite32(tmp, ssp_io_base + _SSP_CTRL2);

        // workaround: prevent SSP repeat last sample, set tx fifo as all zero
        for (i=0; i<16; i++)
            iowrite32(fifo_data, ssp_io_base + _SSP_DATA);        
    }
}

static bool is_ssp_tx_enable(void __iomem * ssp_io_base)
{
    u32 tmp = ioread32(ssp_io_base + _SSP_CTRL2);

    if (tmp & (1 << 8))
        return true;
    return false;
}

static bool is_ssp_rx_enable(void __iomem * ssp_io_base)
{
    u32 tmp = ioread32(ssp_io_base + _SSP_CTRL2);

    if (tmp & (1 << 7))
        return true;
    return false;
}


/*
 *  Nor_Hamming: Apply Hamming Window for flat spectrum response.
 *
 *  x = x*Hamming256
 */
void Hamming(short * pIn)
{
    int i, j;
    int limit;

    limit = NR_FRM_SZ_256/2 - 1;

    for(i = j = 0;i < NR_FRM_SZ_256;i ++){
        //------ Original ------
        pIn[i] = (hamming256[j] * pIn[i]) >> 15;   // Q15*Q15 = Q30 >> 15 = Q15

        if (i < limit)
            ++ j;
        else if (i > limit)
            -- j;
    }
    return;
}

/*
 *  NR_process()
 */
static short asNRin[NR_FRM_SZ_256] = {[0 ... (NR_FRM_SZ_256 - 1)] = 0};
static short asNRin_i[NR_FRM_SZ_256] = {[0 ... (NR_FRM_SZ_256 - 1)] = 0};
static short asNRtail[NR_FRM_OV_SZ_128] = {[0 ... (NR_FRM_OV_SZ_128 - 1)] = 0};
void NR_process(short * pNRin, short * pNRout, int Threshold)
{
    int i, j, scale;

    Hamming(pNRin);

    fix_fft(pNRin, asNRin_i, 8, 0);     // 2^8 = 256

    pNRin[0] = 0;       // remove DC
    asNRin_i[0] = 0;
    pNRin[1] = 0;       // remove 31.25 Hz
    asNRin_i[1] = 0;
    pNRin[255] = 0;
    asNRin_i[255] = 0;

    // --------- clear Real & Imag parts ---------
    for (i = 2, j = 254;i < 128;i ++, j --) {
        int m = pNRin[i];
        int n = asNRin_i[i];

        if ((m*m + n*n) < Threshold) {
            pNRin[i] = 0;
            asNRin_i[i] = 0;
            pNRin[j] = 0;
            asNRin_i[j] = 0;
        }
    }

    scale = fix_fft(pNRin, asNRin_i, 8, 1);

    if (scale > 0) {
        for (i = 0;i < NR_FRM_SZ_256;i ++)
            pNRin[i] = pNRin[i] << scale;
    }

    for (i = 0;i < NR_FRM_OV_SZ_128;i ++) {
        pNRout[i] = pNRin[i] + asNRtail[i];
        asNRtail[i] = pNRin[i + NR_FRM_OV_SZ_128];
    }

    return;
}

static void noise_reduction(short *nrOutBuf, short *nrInBuf, short *nrPadBuf, u32 frame_cnt, u32 threshold)
{
    int i, j;

    if (!nrOutBuf || !nrInBuf || !nrPadBuf || !frame_cnt) {
        WARN("NULL input buffer ptr or frame count of %s\n", __FUNCTION__);
        return;
    }

    /* --------------- First frame --------------- */
    memcpy(asNRin, nrPadBuf, NR_FRM_OV_SZ_128 * sizeof(short));

    memcpy(&asNRin[NR_FRM_OV_SZ_128], nrInBuf, NR_FRM_OV_SZ_128 * sizeof(short));

    NR_process(asNRin, nrOutBuf, threshold);

    nrOutBuf += NR_FRM_OV_SZ_128;

    /* ex. BlkSizeW = 256 samples, pad_buf_sz = 128 samples)
     *    |---256---|
     *         |---256---|...
     */
    for (i = 0;i < frame_cnt - NR_FRM_OV_SZ_128;i += NR_FRM_OV_SZ_128) {
        for (j = 0;j < NR_FRM_SZ_256/2;j ++) {
            u32 *src = (u32 *)&nrInBuf[i];
            u32 *dst = (u32 *)&asNRin[0];
            dst[j] = src[j];
        }

        NR_process(asNRin, nrOutBuf, threshold);

        nrOutBuf += NR_FRM_OV_SZ_128;
    }

    /* Prepare next frame (for next frame's first 128 samples) */
    memcpy(nrPadBuf, &nrInBuf[i], NR_FRM_OV_SZ_128 * sizeof(short));

}
extern struct video_entity_t audio_enc_entity;

static u32 NRThreshold = 0;
static bool enable_data_in = true;
static void audio_dma_record_handler(void *param)
{
    audio_board_t *brd = (audio_board_t *) param;
    audio_stream_t *stream = &brd->stream[STREAM_DIR_RECORD];
    unsigned int distance, brd_idx = 0, i = 0;
    unsigned long flags;
    int *play_buf_dbg = audio_drv_get_play_buf_dbg();
    int *pcm_show_flag = audio_drv_get_pcm_data_flag();
    u32 *audio_overrun_cnt = audio_get_enc_overrun();
    u32 *nr_threshold = audio_drv_get_nr_threshold();
    u32 nr_frame_sz = NODE_BUF_SZ / sizeof(short);
    u32 addr_from_dma = 0, addr_to_pcm = 0, addr_from_pcm = 0, addr_to_nr = 0;
    const int chan_sample_sz = stream->sample_sz / BITS_PER_BYTE;
    const int ssp_sample_sz = brd->total_channel * chan_sample_sz;

    if (enable_data_in) {
        spin_lock_irqsave(&audio_lock, flags);
        stream->buf_idx.dma_write_pos ++;
        spin_unlock_irqrestore(&audio_lock, flags);
    } else {
        goto record_overrun;
    }

    memset(stream->pcm_ch_ofs, 0, sizeof(u32) * AUDIO_SSP_CHAN);

    for (brd_idx = 0; brd_idx < au_get_ssp_max(); brd_idx++)
        if ((u32) (brd->ssp_io_base) == (u32) (audio_brd[brd_idx]->ssp_io_base))
            break;

    /*
     * Step 1. dispatch dma buffer to pcm buffer
     */

    /* count the base addr of dma buffer for dispatching audio sample to pcm buffer */
    addr_from_dma = (u32)stream->kbuffer + GET_READ_IDX(stream->buf_idx.dma_read_pos, brd->dma_llp_cnt) * stream->buffer_sz;

    data_dump((u16 *)addr_from_dma, 8, "dmaR", *pcm_show_flag);

    /* seperate brd->total_channel channels' audio to each pcm channel buffer */
    if (brd->total_channel > 1) {
        for (i = 0;i < stream->buffer_sz;i += chan_sample_sz) {

            /* find out which channel to be copied to */
            u32 channel = (i % ssp_sample_sz) / chan_sample_sz;

            /* count the base address of pcm channel buffer for copying */
            addr_to_pcm = stream->pcm_ch_buf[channel] + GET_READ_IDX(stream->buf_idx.pcm_write_pos, brd->dma_llp_cnt) * NODE_BUF_SZ;

            /* copy sample from dma buffer to pcm buffer */
            ((u16 *)(addr_to_pcm + stream->pcm_ch_ofs[channel]))[0] = ((u16 *)(addr_from_dma + i))[0];

            stream->pcm_ch_ofs[channel] += sizeof(u16);
        }
    }

    /* Data verification from pcm buffer */
    data_dump((u16 *)(stream->pcm_ch_buf[0] + GET_READ_IDX(stream->buf_idx.pcm_write_pos, brd->dma_llp_cnt) * NODE_BUF_SZ), 8, "pcmR", *pcm_show_flag);

    stream->buf_idx.pcm_write_pos ++;
    stream->buf_idx.dma_read_pos ++;

    /*
     * Step 2. do noise reduction from pcm buffer to nr buffer
     */

    if (NRThreshold != *nr_threshold) {
        printk("NR Threshold before: %u, after: %u\n", NRThreshold, *nr_threshold);
        NRThreshold = *nr_threshold;
    }

    for (i = 0;i < brd->total_channel;i ++) {
        /* count the base addr of pcm buffer for NR input address */
        addr_from_pcm = stream->pcm_ch_buf[i] + GET_READ_IDX(stream->buf_idx.pcm_read_pos, brd->dma_llp_cnt) * NODE_BUF_SZ;

        /* count the base addr of nr buffer for NR output address */
        addr_to_nr = stream->nr_ch_buf[i] + GET_READ_IDX(stream->buf_idx.write_pos, brd->dma_llp_cnt) * NODE_BUF_SZ;

        /* do NR depends on frame size */
        if ((nr_frame_sz % NR_FRM_SZ_256) == 0 && drv_param->nr_enable[brd_idx]) {
            noise_reduction((short *)addr_to_nr, (short *)addr_from_pcm, stream->nr_buf_pad[i], nr_frame_sz, NRThreshold); // NR data unit is "sample"
        } else {
            /* needlessly do anything bcuz *nr_buffer is equal to *pcm_buffer */
        }
    }

    /* Data verification from nr buffer */
    data_dump((u16 *)(stream->nr_ch_buf[0] + GET_READ_IDX(stream->buf_idx.write_pos, brd->dma_llp_cnt) * NODE_BUF_SZ), 8, "nr_R", *pcm_show_flag);

    // Record current jiffies for each node buffer
    brd->frame_jiffies[GET_READ_IDX(stream->buf_idx.write_pos, brd->dma_llp_cnt)] = get_gm_jiffies();

    stream->buf_idx.write_pos ++;
    stream->buf_idx.pcm_read_pos ++;

    /*
     * Step 3. nr buffer overrun control,
     *         overrun: distance between read(read_pos) and write(write_pos) > brd->dma_llp_cnt
     */
record_overrun:

    /* distance between read(read_pos) and write(write_pos) */
    distance = BUFIDX_DIST(stream->buf_idx.write_pos, stream->buf_idx.read_pos);

    if (distance >= brd->dma_llp_cnt) {
        if (*play_buf_dbg)
            printk("AUDIO: Record Overrun (index:r=%d, w=%d), reset audio!\n",
                stream->buf_idx.read_pos, stream->buf_idx.write_pos);

        GM_LOG(GM_LOG_REC_PKT_FLOW,"AUDIO: Record Overrun (index:r=%d, w=%d), reset audio!\n",
                stream->buf_idx.read_pos, stream->buf_idx.write_pos);

        audio_overrun_cnt[brd_idx] ++;
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
        video_entity_notify(&audio_enc_entity, ENTITY_FD(audio_enc_entity.type, 0, brd_idx), VG_AUDIO_BUFFER_OVERRUN, 1);
#endif

#ifdef RESET_AUDIO_BUFFER
        /* reset */
        dmaengine_terminate_all(stream->cyclic_dma.chan);
        audio_ssp_rx_enable(brd->ssp_io_base, 0);

        /* software reset */
        stream->handler = NULL;
#else
        /* drop new data until distance < brd->dma_llp_cnt */
        enable_data_in = false;
#endif
    } else {
        enable_data_in = true;
    }

    if (*play_buf_dbg)
        printk("RECORD(ssp: %p) <<<<<<<<< : write: %u, read: %u \n", brd->ssp_io_base, stream->buf_idx.write_pos,
                stream->buf_idx.read_pos);

    return;
}

extern struct video_entity_t audio_dec_entity;
static bool enable_data_out = true;
static void audio_dma_playback_handler(void *param)
{
    audio_board_t *brd = (audio_board_t *) param;
    audio_stream_t *stream = &brd->stream[STREAM_DIR_PLAYBACK];
    unsigned int distance = BUFIDX_DIST(stream->buf_idx.write_pos, stream->buf_idx.read_pos);
    unsigned long flags;
    int *play_buf_dbg = audio_drv_get_play_buf_dbg();
    u32 *audio_underrun_cnt = audio_get_dec_underrun();

    if (enable_data_out) {
        spin_lock_irqsave(&audio_lock, flags);
        stream->buf_idx.read_pos++;
        distance = BUFIDX_DIST(stream->buf_idx.write_pos, stream->buf_idx.read_pos);
        spin_unlock_irqrestore(&audio_lock, flags);
    }

    if (distance == 0) {
        int brd_idx;
        if (*play_buf_dbg)
            printk("AUDIO Playback: Underrun happens (index:r=%d, w=%d), reset audio!\n",
                stream->buf_idx.read_pos, stream->buf_idx.write_pos);

        for (brd_idx = 0; brd_idx < au_get_ssp_max(); brd_idx++)
            if ((u32) (brd->ssp_io_base) == (u32) (audio_brd[brd_idx]->ssp_io_base))
                break;
        audio_underrun_cnt[brd_idx] ++;
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
        video_entity_notify(&audio_dec_entity, ENTITY_FD(audio_dec_entity.type, 1, (1<<brd_idx)),
                            VG_AUDIO_BUFFER_UNDERRUN, 1);
#endif

#ifdef RESET_AUDIO_BUFFER
        /* reset */
        dmaengine_terminate_all(stream->cyclic_dma.chan);
        audio_ssp_tx_enable(brd->ssp_io_base, 0);

        /* software reset */
        stream->handler = NULL;
#else
        /* play old data */
        enable_data_out = false;
#endif
    } else {
        enable_data_out = true;
    }

    if (*play_buf_dbg)
        printk("PLAYBACK(ssp: %p) >>>>>>>> write: %d, read: %d \n", brd->ssp_io_base, stream->buf_idx.write_pos,
                stream->buf_idx.read_pos);

    return;
}

int audio_gm_open(void *handler)
{
    if (ssp_handler) {
        printk(KERN_ERR "ssp handler is registered before\n");
        return -ENODEV;
    }
    ssp_handler = (int *)handler;
    return 0;
}

EXPORT_SYMBOL(audio_gm_open);

int audio_gm_release(void *handler)
{
    audio_stream_t *stream;
    int i, j;

    for (i = 0; i < au_get_ssp_max(); i++) {
        for (j = 0; j < STREAM_DIR_MAX; j++) {
            if (audio_brd[i]->stream[j].handler != handler)
                continue;

            stream = &audio_brd[i]->stream[j];

            if (i == STREAM_DIR_RECORD) {
                GM_LOG(GM_LOG_REC_PKT_FLOW, "ssp%d rx disable\n", i);
                audio_ssp_rx_enable(audio_brd[i]->ssp_io_base, 0);
            }
            else {
                GM_LOG(GM_LOG_PB_PKT_FLOW, "ssp%d tx disable\n", i);
                audio_ssp_tx_enable(audio_brd[i]->ssp_io_base, 0);
            }

            dmaengine_terminate_all(stream->cyclic_dma.chan);
            dma_release_channel(stream->cyclic_dma.chan);
#if defined(FIXED_DMA_CHAN)
            dma_release_channel(stream->memcpy_dma.chan);
#endif

            //reset software
            stream->handler = NULL;
            memset(&stream->buf_idx, 0, sizeof(stream->buf_idx));
        }
    }

    return 0;
}

EXPORT_SYMBOL(audio_gm_release);

u32 audio_gm_read(int *handler, void *ctrl, size_t *count)
{
    ssp_buf_t *ctrl_buf = (ssp_buf_t *) ctrl;
    audio_board_t *brd = audio_brd[ctrl_buf->ssp_idx];
    audio_stream_t *stream = &brd->stream[STREAM_DIR_RECORD];
    u32 cp_cnt = 0, *write_pos, *read_pos, *read_ofs;
    unsigned long flags, dma_flag;

    if (stream->active == false) {
        printk("%s: record of ssp: %d is not active!\n", __func__, ctrl_buf->ssp_idx);
        return 0;
    }

    if (*count == 0)
        return 0;

    //GM_LOG( GM_LOG_REC_PKT_FLOW, "%s ssp%d\n", __func__, ctrl_buf->ssp_idx);

#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    if (ctrl_buf->sample_rate || ctrl_buf->channel_type) {
        bool is_ssp_reset = false;
        if (brd->sample_rate != ctrl_buf->sample_rate) {
            brd->sample_rate = ctrl_buf->sample_rate;
            is_ssp_reset = true;
            printk("audio info: change rec sample rate to %u\n", brd->sample_rate);
        }
        if (brd->is_stereo != (ctrl_buf->channel_type - 1)) {
            brd->is_stereo = ctrl_buf->channel_type - 1;
            is_ssp_reset = true;
            printk("audio info: change rec channel type to %s\n", ((brd->is_stereo)==1)?("stereo"):("mono"));
        }
        if (is_ssp_reset) {
            dmaengine_terminate_all(stream->cyclic_dma.chan);
            stream->handler = NULL;
            audio_ssp_init(brd);
    }
    }
#endif

    spin_lock_irqsave(&audio_lock, flags);

    if (stream->handler == NULL) {
        stream->handler = handler;
        memset(&stream->buf_idx, 0, sizeof(stream->buf_idx));

        spin_unlock_irqrestore(&audio_lock, flags);

        /* record dma setting */
        dmaengine_slave_config(stream->cyclic_dma.chan, &stream->cyclic_dma.slave.common);
        dma_flag =
            DMA_PREP_INTERRUPT | DMA_CTRL_ACK | DMA_COMPL_SKIP_SRC_UNMAP |
            DMA_COMPL_SKIP_DEST_UNMAP;
        stream->cyclic_dma.desc =
            dmaengine_prep_dma_cyclic(stream->cyclic_dma.chan, stream->dma_buffer,
                                      brd->max_period_sz * brd->dma_llp_cnt, brd->max_period_sz,
                                      DMA_DEV_TO_MEM);
        stream->cyclic_dma.desc->callback = audio_dma_record_handler;
        stream->cyclic_dma.desc->callback_param = (void *)brd;
        stream->cyclic_dma.cookie = dmaengine_submit(stream->cyclic_dma.desc);

        //prevent from interrupted by record interrupter
        spin_lock_irqsave(&audio_lock, flags);
        if (stream->handler) {
            dma_async_issue_pending(stream->cyclic_dma.chan);
            GM_LOG(GM_LOG_REC_PKT_FLOW, "ssp%d rx enable\n", ctrl_buf->ssp_idx);
            audio_ssp_rx_enable(brd->ssp_io_base, 1);
        }
        spin_unlock_irqrestore(&audio_lock, flags);
    } else {
        if (is_ssp_rx_enable(brd->ssp_io_base) == false) {
            GM_LOG(GM_LOG_REC_PKT_FLOW, "warn! re-enable ssp%d rx function\n", ctrl_buf->ssp_idx);
            audio_ssp_rx_enable(brd->ssp_io_base, 1);
        }
        spin_unlock_irqrestore(&audio_lock, flags);
    }

    write_pos = &stream->buf_idx.write_pos;
    read_pos = &stream->buf_idx.read_pos;
    read_ofs = &stream->buf_idx.read_ofs;

    if (BUF_COUNT(*write_pos, *read_pos)) {
        int left = NODE_BUF_SZ - *read_ofs;
        unsigned long ch = 0;
        int *pcm_show_flag = audio_drv_get_pcm_data_flag();

        u32 curr_ridx = 0, prev_ridx = 0, time_per_byte = 0;    
        unsigned long curr_node_ts = 0, prev_node_ts = 0;

        cp_cnt = (*count >= left) ? left : *count;

        spin_lock_irqsave(&audio_lock, flags);    
        curr_ridx = GET_READ_IDX(stream->buf_idx.read_pos, brd->dma_llp_cnt);    
        prev_ridx = GET_READ_IDX((stream->buf_idx.read_pos)-1, brd->dma_llp_cnt);    
        curr_node_ts = brd->frame_jiffies[curr_ridx];
        if (curr_node_ts == 0)
            prev_node_ts = 0;
        else {
#if 0        
            prev_node_ts = (brd->frame_jiffies[prev_ridx]>0)?(brd->frame_jiffies[prev_ridx]):
                           (curr_node_ts-((NODE_BUF_SZ*1000)/(brd->sample_rate*DEFAULT_SAMPLE_SZ/BITS_PER_BYTE)));    
#else
            prev_node_ts = brd->frame_jiffies[prev_ridx];
            if ((prev_node_ts == 0) || ((curr_node_ts - prev_node_ts) > (2*((NODE_BUF_SZ*1000)/(brd->sample_rate*DEFAULT_SAMPLE_SZ/BITS_PER_BYTE)))))
                prev_node_ts = curr_node_ts-((NODE_BUF_SZ*1000)/(brd->sample_rate * ctrl_buf->channel_type * (DEFAULT_SAMPLE_SZ / BITS_PER_BYTE)));
#endif            
        }
        time_per_byte = ((curr_node_ts - prev_node_ts) * 1000) / NODE_BUF_SZ;  
        spin_unlock_irqrestore(&audio_lock, flags);
        
        //printk("curr_ridx(%u), prev_ridx(%u), curr_node_ts(%lu), prev_node_ts(%lu), time_per_byte(%u)\n", 
                //curr_ridx, prev_ridx, curr_node_ts, prev_node_ts, time_per_byte);

        for_each_set_bit(ch, (const unsigned long *)&ctrl_buf->bitmap, drv_param->max_chan_cnt) {
            if (ctrl_buf->buf_pos[ch] < ctrl_buf->length[ch]) {
                u32 *src = (u32 *)(stream->nr_ch_buf[ch] + GET_READ_IDX(*read_pos, brd->dma_llp_cnt) * NODE_BUF_SZ + *read_ofs);
                u32 *dst = (u32 *)(ctrl_buf->offset[ch] + ctrl_buf->buf_pos[ch]);

                memcpy(dst, src, cp_cnt);
                ctrl_buf->buf_pos[ch] += cp_cnt;
            }
        }

        if (ctrl_buf->time_stamp == 0) {            
            ctrl_buf->time_stamp = curr_node_ts + ((stream->buf_idx.read_ofs * time_per_byte) / 1000);            
            //printk("get giffies -> (%d)%lu, curr_node_ts(%lu), read_ofs(%u), add(%u)\n",                         
                    //GET_READ_IDX(stream->buf_idx.read_pos, brd->dma_llp_cnt), ctrl_buf->time_stamp,                         
                    //curr_node_ts, stream->buf_idx.read_ofs, ((stream->buf_idx.read_ofs * time_per_byte) / 1000));        
        }

        /* Data verification for channel == 0 */
        data_dump((u16 *)(stream->nr_ch_buf[0] + GET_READ_IDX(*read_pos, brd->dma_llp_cnt) * NODE_BUF_SZ + *read_ofs), 8, "srcR", *pcm_show_flag);
        data_dump((u16 *)(ctrl_buf->offset[0]), 8, "dstR", *pcm_show_flag);

        left -= cp_cnt;
        if (!left) {
            *read_ofs = 0;
            (*read_pos)++;      //move to next
        } else {
            *read_ofs += cp_cnt;
        }

        /* use buf_size as time offset(milisecond ==> 1000) from now */
        ctrl_buf->buf_size = BUF_COUNT(*write_pos, *read_pos) * AAC_FRAME_TIME / ctrl_buf->channel_type;
    }
    /* if (BUF_COUNT(...)) */

    //GM_LOG(GM_LOG_REC_PKT_FLOW, "%s ssp%d done\n", __func__, ctrl_buf->ssp_idx);

    return cp_cnt;
}

EXPORT_SYMBOL(audio_gm_read);

u32 audio_gm_write(int *handler, void *ctrl, size_t *count)
{
    ssp_buf_t *ctrl_buf = (ssp_buf_t *) ctrl;
    audio_board_t *brd = audio_brd[ctrl_buf->ssp_idx];
    audio_stream_t *stream = &brd->stream[STREAM_DIR_PLAYBACK];
    u32 org_write_pos;
    u32 cp_cnt = 0, total_cp_cnt = 0;
    unsigned long flags, dma_flag;
#if defined(CONFIG_PLATFORM_GM8136)
    int curr_output_mode = adda308_get_output_mode();
#endif
#if defined(CONFIG_PLATFORM_GM8136) || defined(CONFIG_PLATFORM_GM8139)
    static u32 curr_da_ssp = 0;
#endif

    if (stream->active == false) {
        printk("%s: playback of ssp: %d is not active!\n", __func__, ctrl_buf->ssp_idx);
        return 0;
    }

    if (*count == 0)
        return 0;

    //GM_LOG(GM_LOG_PB_PKT_FLOW, "%s ssp%d\n", __func__, ctrl_buf->ssp_idx);

#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    if ((u32)(brd->ssp_io_base) != curr_da_ssp) {
        if ((u32)(brd->ssp_io_base) == (u32)(audio_brd[0]->ssp_io_base))
#if defined(CONFIG_PLATFORM_GM8139)
            ADDA302_Set_DA_Source(DA_SRC_SSP0);
#elif defined(CONFIG_PLATFORM_GM8136)
            ADDA308_Set_DA_Source(DA_SRC_SSP0);
#endif
        else
#if defined(CONFIG_PLATFORM_GM8139)
            ADDA302_Set_DA_Source(DA_SRC_SSP1);
#elif defined(CONFIG_PLATFORM_GM8136)
            ADDA308_Set_DA_Source(DA_SRC_SSP1);
#endif
        curr_da_ssp = (u32)(brd->ssp_io_base);
    }

    if (ctrl_buf->sample_rate || ctrl_buf->channel_type) {
        bool is_ssp_reset = false;
        if (brd->sample_rate != ctrl_buf->sample_rate) {
            brd->sample_rate = ctrl_buf->sample_rate;
            is_ssp_reset = true;
            printk("audio info: change pb sample rate to %u\n", brd->sample_rate);
        }
        if (brd->is_stereo != (ctrl_buf->channel_type - 1)) {
            brd->is_stereo = ctrl_buf->channel_type - 1;
            is_ssp_reset = true;
            printk("audio info: change pb channel type to %s\n", ((brd->is_stereo)==1)?("stereo"):("mono"));
        }
#if defined(CONFIG_PLATFORM_GM8136)
        if (adda_output_mode != curr_output_mode) {
            adda_output_mode = curr_output_mode;
            is_ssp_reset = true;
            printk("audio info: change output mode to %s\n", (adda_output_mode==SPK_OUTPUT)?("spk out"):("line out"));
        }
#endif        
        if (is_ssp_reset) {
            dmaengine_terminate_all(stream->cyclic_dma.chan);
            stream->handler = NULL;
            audio_ssp_init(brd);
    }
    }
#endif

    spin_lock_irqsave(&audio_lock, flags);

    if (stream->handler == NULL) {

        stream->handler = handler;
        memset(&stream->buf_idx, 0, sizeof(stream->buf_idx));

        spin_unlock_irqrestore(&audio_lock, flags);

        /* playback dma setting */
        dmaengine_slave_config(stream->cyclic_dma.chan, &stream->cyclic_dma.slave.common);
        dma_flag =
            DMA_PREP_INTERRUPT | DMA_CTRL_ACK | DMA_COMPL_SKIP_SRC_UNMAP |
            DMA_COMPL_SKIP_DEST_UNMAP;
        stream->cyclic_dma.desc =
            dmaengine_prep_dma_cyclic(stream->cyclic_dma.chan, stream->dma_buffer,
                                      brd->max_period_sz * brd->dma_llp_cnt, brd->max_period_sz,
                                      DMA_MEM_TO_DEV);
        if (!stream->cyclic_dma.desc)
            panic("%s: Set dmaengine_prep_dma_cyclic() failed!!\n", __func__);
        stream->cyclic_dma.desc->callback = audio_dma_playback_handler;
        stream->cyclic_dma.desc->callback_param = (void *)brd;
        stream->cyclic_dma.cookie = dmaengine_submit(stream->cyclic_dma.desc);
        if (dma_submit_error(stream->cyclic_dma.cookie)) {
            dmaengine_terminate_all(stream->cyclic_dma.chan);
            dma_release_channel(stream->cyclic_dma.chan);
            panic("%s: DMA cyclic submit failed\n", __func__);
        }
    } else {
        spin_unlock_irqrestore(&audio_lock, flags);
    }

    org_write_pos = stream->buf_idx.write_pos;

    while (*count) {
        if (BUF_COUNT(stream->buf_idx.write_pos, stream->buf_idx.read_pos) < brd->dma_llp_cnt) {
            int *pcm_show_flag = audio_drv_get_pcm_data_flag();
            int space = stream->buffer_sz - stream->buf_idx.write_ofs;
            const int channel = ctrl_buf->bitmap;
            void *cp_virt_ptr =
                (void *)((u32) stream->kbuffer +
                         GET_READ_IDX(stream->buf_idx.write_pos,
                                      brd->dma_llp_cnt) * stream->buffer_sz +
                         stream->buf_idx.write_ofs);

#if defined(MEMCPY_DMA)
            dma_addr_t cp_phy_ptr =
                (dma_addr_t) (stream->dma_buffer +
                        GET_READ_IDX(stream->buf_idx.write_pos,
                            brd->dma_llp_cnt) * stream->buffer_sz +
                        stream->buf_idx.write_ofs);
#endif
            cp_cnt = (*count >= space) ? space : *count;

            /* Data verification for channel == 0 */
            data_dump((u16 *)ctrl_buf->kbuf_ptr, 8, "play", *pcm_show_flag);

            if (brd->total_channel == 1) {
#if defined(FIXED_DMA_CHAN)
                dma_flag =
                    DMA_PREP_INTERRUPT | DMA_CTRL_ACK | DMA_COMPL_SKIP_SRC_UNMAP |
                    DMA_COMPL_SKIP_DEST_UNMAP;
                stream->memcpy_dma.desc =
                    dmaengine_prep_dma_memcpy(stream->memcpy_dma.chan, cp_phy_ptr,
                                              ctrl_buf->buf_ptr + ctrl_buf->buf_pos[channel],
                                              cp_cnt, dma_flag);
                if (!stream->memcpy_dma.desc)
                    panic("%s: Set dmaengine_prep_dma_memcpy() failed!!\n", __func__);
                stream->memcpy_dma.cookie = dmaengine_submit(stream->memcpy_dma.desc);
                if (dma_submit_error(stream->memcpy_dma.cookie)) {
                    dmaengine_terminate_all(stream->memcpy_dma.chan);
                    dma_release_channel(stream->memcpy_dma.chan);
                    panic("%s: DMA memcpy submit failed\n", __func__);
                }
                stream->memcpy_dma.desc->callback = dma_callback;
                stream->memcpy_dma.desc->callback_param = &stream->memcpy_dma;
                stream->memcpy_dma.dma_st = DMA_IN_PROGRESS;
                dma_async_issue_pending(stream->memcpy_dma.chan);
                dma_wait(&stream->memcpy_dma);
#else
#if defined(MEMCPY_DMA)
                if (dma_memcpy(AHB_DMA, cp_phy_ptr, ctrl_buf->buf_ptr + ctrl_buf->buf_pos[channel], cp_cnt) < 0) {
                    u32 *au_play_req_dma_chan_failed_cnt = au_get_dec_req_dma_chan_failed_cnt();
                    memcpy(cp_virt_ptr, (void *)((unsigned)ctrl_buf->kbuf_ptr + ctrl_buf->buf_pos[channel]), cp_cnt);
                    au_play_req_dma_chan_failed_cnt[ctrl_buf->ssp_idx] ++;
                }
#else /* PIO mode */
                memcpy(cp_virt_ptr, (void *)((unsigned)ctrl_buf->kbuf_ptr + ctrl_buf->buf_pos[channel]), cp_cnt);
#endif
#endif
                ctrl_buf->buf_pos[channel] += cp_cnt;
            } else {
                const int chan_sample_sz = stream->sample_sz / BITS_PER_BYTE;
                const int ssp_sample_sz = brd->total_channel * chan_sample_sz;
                u32 i;

                /* put audio data to channel location in DMA LLP */
                for (i = 0; i < cp_cnt; i += chan_sample_sz) {
                    if (i % ssp_sample_sz == ctrl_buf->bitmap * chan_sample_sz) {
                        if (ctrl_buf->buf_pos[channel] < ctrl_buf->buf_size) {
                            ((u16 *) ((u32) cp_virt_ptr + i))[0] =
                                ((u16 *) ((u32) ctrl_buf->kbuf_ptr +
                                          ctrl_buf->buf_pos[channel]))[0];
                            ctrl_buf->buf_pos[channel] += chan_sample_sz;
                        }
                    }
                }
            } /* else */

            if (brd->is_cacheable_dma)
                fmem_dcache_sync((void *)((u32) stream->kbuffer +
                                          GET_READ_IDX(stream->buf_idx.write_pos,
                                                       brd->dma_llp_cnt) * stream->buffer_sz),
                                 stream->buffer_sz, DMA_TO_DEVICE);

            space -= cp_cnt;
            if (!space) {
                stream->buf_idx.write_ofs = 0;
                stream->buf_idx.write_pos++;
            } else {
                stream->buf_idx.write_ofs += cp_cnt;
            }
            *count -= cp_cnt;
            total_cp_cnt += cp_cnt;
        } else {
            break;
        }
    } /* while */

    spin_lock_irqsave(&audio_lock, flags);
    if (stream->buf_idx.write_pos >= LLP_BUFFERED_CNT && stream->handler && !is_ssp_tx_enable(brd->ssp_io_base)) {

        DBG("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ############# \n");

        /* workaround for first time playback of NVP1918 */
        audio_ssp_tx_enable(brd->ssp_io_base, 0);

        /* data now is ready, we can start hardware to playback */
        dma_async_issue_pending(stream->cyclic_dma.chan);
        audio_ssp_tx_enable(brd->ssp_io_base, 1);
    }
    spin_unlock_irqrestore(&audio_lock, flags);

    /* use offset as time offset(milisecond ==> 1000) from now */
    *(ctrl_buf->offset) =
        BUF_COUNT(stream->buf_idx.write_pos, stream->buf_idx.read_pos) * AAC_FRAME_TIME;

    //GM_LOG(GM_LOG_PB_PKT_FLOW, "%s ssp%d done\n", __func__, ctrl_buf->ssp_idx);

    return total_cp_cnt;
}

EXPORT_SYMBOL(audio_gm_write);

int audio_drv_stop(stream_dir_t dir, u32 idx)
{
    int ret = 0;

    if (audio_brd[idx]->stream[dir].active == false)
        return ret;

    switch (dir) {
        case STREAM_DIR_RECORD:
            dmaengine_terminate_all(audio_brd[idx]->stream[STREAM_DIR_RECORD].cyclic_dma.chan);
            audio_ssp_rx_enable(audio_brd[idx]->ssp_io_base, 0);
            audio_brd[idx]->stream[STREAM_DIR_RECORD].handler = NULL;
            break;
        case STREAM_DIR_PLAYBACK:
            dmaengine_terminate_all(audio_brd[idx]->stream[STREAM_DIR_PLAYBACK].cyclic_dma.chan);
            audio_ssp_tx_enable(audio_brd[idx]->ssp_io_base, 0);
            audio_brd[idx]->stream[STREAM_DIR_PLAYBACK].handler = NULL;
            break;
        default:
            ret = -EINVAL;
            break;
    }
    return ret;
}
EXPORT_SYMBOL(audio_drv_stop);

static irqreturn_t ssp_irq_handler(int irq, void *dev_id)
{
    audio_board_t *brd = (audio_board_t *)dev_id;
    u32 status = ioread32(brd->ssp_io_base + _SSP_INTRSTAT);
    u32 tmp = 0;

    if (status & 0x1) { /* RFORI: rx fifo overrun irq */
        /* disable ssp irq */
        u32 tmp = ioread32(brd->ssp_io_base + _SSP_INTRCTRL);
        tmp &= ~((1 << 0) | (1 << 2));
        iowrite32(tmp, brd->ssp_io_base + _SSP_INTRCTRL);
        brd->is_fifo_overrun = true;
        printk(KERN_INFO"audio info: ssp fifo overrun, irq(%d) status(%u)\n", irq, status);
    }

    if (status & 0x2) { /* TFURI: tx fifo underrun irq */
        /* disable ssp irq */
        u32 tmp = ioread32(brd->ssp_io_base + _SSP_INTRCTRL);
        tmp &= ~((1 << 1) | (1 << 3));
        iowrite32(tmp, brd->ssp_io_base + _SSP_INTRCTRL);
        brd->is_fifo_underrun = true;
        printk(KERN_INFO"audio info: ssp fifo underun, irq(%d) status(%u)\n", irq, status);
    }

    /* workaround for "TX under-run" cannot been clean after read at first time*/
    tmp = ioread32(brd->ssp_io_base + _SSP_INTRSTAT);

    /* overrun ==> reset ssp rx */
    if (brd->is_fifo_overrun) {
        audio_stream_t *stream = &brd->stream[STREAM_DIR_RECORD];
        dmaengine_terminate_all(stream->cyclic_dma.chan);
        audio_ssp_rx_enable(brd->ssp_io_base, 0);
        brd->stream[STREAM_DIR_RECORD].handler = NULL;
        brd->is_fifo_overrun = false;
    }

    /* underrun ==> reset ssp tx */
    if (brd->is_fifo_underrun) {
        audio_stream_t *stream = &brd->stream[STREAM_DIR_PLAYBACK];
        dmaengine_terminate_all(stream->cyclic_dma.chan);
        audio_ssp_tx_enable(brd->ssp_io_base, 0);
        brd->stream[STREAM_DIR_PLAYBACK].handler = NULL;
        brd->is_fifo_underrun = false;
    }

	return IRQ_HANDLED;
}

int audio_drv_init(void)
{
    u32 i, j;
    int ssp_master[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
    int ssp_stereo[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};

    spin_lock_init(&audio_lock);

    drv_param = au_get_param();

    dmac020_base = (u32)ioremap_nocache(DMAC_FTDMAC020_PA_BASE, PAGE_SIZE);
    if (!dmac020_base)
        panic("%s, no virutal address! \n", __func__);

    for (i = 0; i < au_get_ssp_max(); i++) {
        audio_brd[i] = kmalloc(sizeof(audio_board_t), GFP_KERNEL);
        if (audio_brd[i] == NULL) {
            printk("audio driver allocate memory fail\n");
            return -ENOMEM;
        }
        memset(audio_brd[i], 0, sizeof(audio_board_t));

#if defined(CONFIG_PLATFORM_GM8287)
        ssp_stereo[i] = 1;

        if (drv_param->ssp_chan[i] == 1)
            ssp_stereo[i] = 0;

        if (drv_param->ssp_num[i] == 2) {
            ssp_master[i] = 1;
            drv_param->sample_rate[i] = HDMI_SAMPLE_RATE;
        }
#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
        memcpy(ssp_stereo, au_get_is_stereo(), SSP_MAX_USED * sizeof(int));
        ssp_master[i] = 0;
#endif

        audio_brd[i]->ssp_io_base =
            ioremap_nocache(SSP_BASE(drv_param->ssp_num[i]), PAGE_SIZE);
        if (audio_brd[i]->ssp_io_base == NULL) {
            kfree(audio_brd[i]);
            printk("ssp ioremap fail!\n");
            return -ENOMEM;
        }
        DBG("ssp%d, p_base:0x%x, v_base:0x%p\n", i, SSP_BASE(drv_param->ssp_num[i]),
            audio_brd[i]->ssp_io_base);

        /* default value */
        audio_brd[i]->bclk          = drv_param->bit_clock[i];
        audio_brd[i]->mclk          = DEFAULT_MAIN_CLK;
        audio_brd[i]->sample_rate   = drv_param->sample_rate[i];
        audio_brd[i]->total_channel = drv_param->ssp_chan[i];
        audio_brd[i]->is_master     = ssp_master[i];
        audio_brd[i]->is_stereo     = ssp_stereo[i];
        audio_brd[i]->dma_llp_cnt   = DEFAULT_LLP_CNT;
        audio_brd[i]->max_period_sz = audio_brd[i]->total_channel * AAC_FRAME_SZ
                                    * DEFAULT_SAMPLE_SZ / BITS_PER_BYTE
                                    * audio_brd[i]->sample_rate / DEFAULT_SAMPLE_RATE;
                                    /* divide 8k(default record rate), bcuz 8k->48k resample */
        DBG("audio_brd[%d]: bclk:%d, total_channel:%d, is_master:%d, is_stereo:%d, dma_llp_cnt:%d\n",
            i, audio_brd[i]->bclk, audio_brd[i]->total_channel, audio_brd[i]->is_master,  audio_brd[i]->is_stereo,
            audio_brd[i]->dma_llp_cnt);

        if (audio_brd[i]->max_period_sz % dma_get_cache_alignment()) {
            int remainder = audio_brd[i]->max_period_sz % dma_get_cache_alignment();
            audio_brd[i]->max_period_sz -= (remainder - dma_get_cache_alignment());     /* for 32 bytes align do cacheable dma */
        }

        if (DMA_ALIGN_CHECK(audio_brd[i]->max_period_sz))
            audio_brd[i]->is_cacheable_dma = true;
        else
            audio_brd[i]->is_cacheable_dma = false;

        audio_ssp_init(audio_brd[i]);

        if (request_irq(ssp_irq_num[drv_param->ssp_num[i]], ssp_irq_handler, 0, ssp_irq_name[drv_param->ssp_num[i]], (void *)audio_brd[i]))
            panic("%s, request irq%d fail! \n", __func__, ssp_irq_num[drv_param->ssp_num[i]]);

        for (j = 0; j < STREAM_DIR_MAX; j++)
            audio_drv_stream_init(j, &audio_brd[i]->stream[j], audio_brd[i]);
    }

    return 0;
}

EXPORT_SYMBOL(audio_drv_init);

void audio_drv_exit(void)
{
    int i, j;

    __iounmap((void *)dmac020_base);
    dmac020_base = 0;

    for (i = 0; i < au_get_ssp_max(); i++) {
        free_irq(ssp_irq_num[drv_param->ssp_num[i]], (void *)audio_brd[i]);

        if (audio_brd[i]->ssp_io_base) {
            __iounmap(audio_brd[i]->ssp_io_base);
            audio_brd[i]->ssp_io_base = NULL;
        }

        for (j = 0; j < STREAM_DIR_MAX; j++) {
            if (audio_brd[i]->stream[j].kbuffer) {
                int k;
                if (audio_brd[i]->stream[j].kbuffer == audio_brd[i]->stream[j].pcm_buffer) {
                    if (audio_brd[i]->stream[j].nr_buffer == audio_brd[i]->stream[j].pcm_buffer)
                        audio_brd[i]->stream[j].nr_buffer = NULL;
                    audio_brd[i]->stream[j].pcm_buffer = NULL;
                }

                if (audio_brd[i]->is_cacheable_dma)
                    fmem_free_ex(audio_brd[i]->max_period_sz * audio_brd[i]->dma_llp_cnt,
                                 audio_brd[i]->stream[j].kbuffer,
                                 (dma_addr_t) audio_brd[i]->stream[j].dma_buffer);
                else
                    dma_free_writecombine(NULL,
                                          audio_brd[i]->max_period_sz * audio_brd[i]->dma_llp_cnt,
                                          audio_brd[i]->stream[j].kbuffer,
                                          (dma_addr_t) audio_brd[i]->stream[j].dma_buffer);

                audio_brd[i]->stream[j].kbuffer = NULL;

                if (audio_brd[i]->stream[j].pcm_buffer) {
                    if (audio_brd[i]->stream[j].nr_buffer == audio_brd[i]->stream[j].pcm_buffer)
                        audio_brd[i]->stream[j].nr_buffer = NULL;
                    kfree(audio_brd[i]->stream[j].pcm_buffer);
                    audio_brd[i]->stream[j].pcm_buffer = NULL;
                }

                if (audio_brd[i]->stream[j].nr_buffer) {
                    kfree(audio_brd[i]->stream[j].nr_buffer);
                    audio_brd[i]->stream[j].nr_buffer = NULL;
                }

                for (k = 0;k < audio_brd[i]->total_channel;k ++) {
                    if (audio_brd[i]->stream[j].nr_buf_pad[k]) {
                        kfree(audio_brd[i]->stream[j].nr_buf_pad[k]);
                        audio_brd[i]->stream[j].nr_buf_pad[k] = NULL;
                    }
                }
            }
        }

        if (audio_brd[i]) {
            kfree(audio_brd[i]);
            audio_brd[i] = NULL;
        }
    }
}

EXPORT_SYMBOL(audio_drv_exit);
