#ifndef __AUDIO_DRV_H__
#define __AUDIO_DRV_H__
#include <linux/version.h>
#include <linux/module.h>

#define AUDIO_SSP_CHAN      16

#define DEFAULT_SAMPLE_SZ   16

#define DEFAULT_LLP_CNT     8

#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
#define DEFAULT_MAIN_CLK    24000000
#else
#define DEFAULT_MAIN_CLK    12000000
#endif

#define LLP_BUFFERED_CNT    3

//#define DMA_ALIGN_CHECK(x)  (x == ALIGN(x, dma_get_cache_alignment()))
#define DMA_ALIGN_CHECK(x)  0

#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
#undef MEMCPY_DMA
#else
#define MEMCPY_DMA
#endif

#if defined(MEMCPY_DMA)
#define FIXED_DMA_CHAN
#undef  FIXED_DMA_CHAN
#endif

/*
 * SSP configuration
 */
#define SSP_FIFO_DEPTH      16
#define SSP_RXFIFO_THOD     4
#define _SSP_CTRL0          0x00
#define _SSP_CTRL1          0x04
#define _SSP_CTRL2          0x08
#define _SSP_STAT           0x0C
#define _SSP_INTRCTRL       0x10
#define _SSP_INTRSTAT       0x14
#define _SSP_DATA           0x18

typedef struct bclk_tbl_s {
    unsigned int sample_sz;
    unsigned int sample_rate;
    unsigned int bclk;
    unsigned int mclk;
} bclk_tbl_t;

typedef enum {
    STREAM_DIR_RECORD = 0,  //must start from 0
    STREAM_DIR_PLAYBACK,
    STREAM_DIR_MAX,
} stream_dir_t;

#if defined(CONFIG_PLATFORM_GM8136)
typedef enum {
    SPK_OUTPUT = 0,
    LINE_OUTPUT = 1
} adda308_output_mode;
#endif

#define BUFIDX_DIST(head, tail)	(((head) >= (tail)) ? ((head) - (tail)) : (UINT_MAX - (tail) + (head) + 1))
#define BUF_COUNT(head, tail)	BUFIDX_DIST(head, tail)
#define GET_WRITE_IDX(x, y)     ((x) & (y-1))
#define GET_READ_IDX(x, y)      ((x) & (y-1))

typedef struct _ssp_buf {
    unsigned ssp_idx;
    unsigned bitmap; // when playback, bitmap = video_ch
    unsigned sample_rate;
    unsigned *length;
    unsigned *offset;
    unsigned buf_pos[AUDIO_SSP_CHAN];
    unsigned buf_size;
    int is_live;
    int channel_type; // 1: mono, 2: stereo
    dma_addr_t buf_ptr;
    void *kbuf_ptr;
    unsigned long time_stamp;
} ssp_buf_t;

static inline void data_dump(u16 *buf, int dump_cnt, char *name, int is_dump)
{
    u16 i;

    if (is_dump) {

        printk("%s(%8p) = ", name, buf);
        for (i = 0;i < dump_cnt;i ++) {
            if (i == dump_cnt - 1)
                printk("0x%4x\n", buf[i]);
            else
                printk("0x%4x ", buf[i]);
        }

    }
}

int audio_gm_open(void *handler);
int audio_gm_release(void *handler);
u32 audio_gm_read(int *file, void *ctrl, size_t *count);
u32 audio_gm_write(int *file, void *ctrl, size_t *count);
int audio_drv_stop(stream_dir_t dir, u32 idx);
int audio_drv_init(void);
void audio_drv_exit(void);
#endif /* __AUDIO_DRV_H__ */
