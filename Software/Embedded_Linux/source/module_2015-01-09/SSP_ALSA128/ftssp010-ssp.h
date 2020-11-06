#ifndef __SSP_FTSSP010_H__
#define __SSP_FTSSP010_H__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/dmaengine.h>
#include <mach/ftdmac020.h>
#include <linux/dma-mapping.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <linux/slab.h>

#define FTSSP101_VERSION	        "1.2.8"
#define FTSSP101_MODULE_NAME	    "ft-128ssp"

#define PFX			                FTSSP101_MODULE_NAME

#define FTAHB_DMA_BURST

#define SSP_MAX_SDL                 128
#define SSP_FIFO_DEEPTH             16
#define SSP_DMA_BURST_SIZE          4	//0/1/4/8/16, how many layers in fifo will be moved by DMA.

#define SSP_TXFIFOTHOD              ((SSP_FIFO_DEEPTH - SSP_DMA_BURST_SIZE) << 12)
#define SSP_RXFIFOTHOD              ((SSP_DMA_BURST_SIZE) << 7)

// register address offset
#define FTSSP010_SSP_CR0	        (0x00)
#define FTSSP010_SSP_CR1	        (0x04)
#define FTSSP010_SSP_CR2	        (0x08)
#define FTSSP010_SSP_STATUS	        (0x0C)
#define FTSSP010_SSP_INT_CR	        (0x10)
#define FTSSP010_SSP_INT_STATUS	    (0x14)
#define FTSSP010_SSP_DATA	        (0x18)
#define FTSSP010_SSP_INFO	        (0x1C)
#define FTSSP010_SSP_ACLINK_SLOT_VALID	(0x20)
#define FTSSP010_SSP_REVISION	        (0x60)
#define FTSSP010_SSP_FEATURE	        (0x64)

//control 0
#define SSP_FORMAT              (0x0 << 12)
#define SPI_FORMAT              (0x1 << 12)
#define I2S_FORMAT              (0x3 << 12)
#define SPI_FLASH               (0x1 << 11)
#define SSP_FSDIST(x)           ((x & 0x03) << 8)
#define SSP_LBM                 (0x1 << 7)  /* loopback mode */
#define SSP_LSB                 (0x1 << 6)  /* LSB first */
#define SSP_FSPO_LOW            (0x1 << 5)  /* Frame sync atcive low */
#define SSP_FSPO_HIGH           (0x0 << 5)  /* Frame sync atcive high */
#define SSP_DATAJUSTIFY         (0x1 << 4)  /* data padding in front of serial data */

#define SSP_OPM_MSST            0xC     /* Master stereo mode */
#define SSP_OPM_MSMO            0x8     /* Master mono mode */
#define SSP_OPM_SLST            0x4     /* Slave stereo mode */
#define SSP_OPM_SLMO            0x0     /* Slave mono mode */
#define SSP_MS_MASK             0x8     /* Master or Slave mask */

//control 1
#define SSP_PDL(x)              ((x & 0xFF) << 24)
#define SSP_SDL(x)              ((x & 0x3F) << 16)
#define SSP_SCLKDIV(x)          (x & 0xFFFF)

//control 2 register value
#define SSP_FS                  (0x1 << 9)
#define SSP_TXEN                (0x1 << 8)
#define SSP_RXEN                (0x1 << 7)
#define SSP_RESET               (0x1 << 6)
#define SSP_TXFCLR              (0x1 << 3)  /* TX FIFO Clear */
#define SSP_RXFCLR              (0x1 << 2)  /* RX FIFO Clear */
#define SSP_TXDOE               (0x1 << 1)  /* TX Data Output Enable */
#define SSP_SSPEN               (0x1 << 0)  /* SSP Enable */

// interrupt control
#define SSP_TFTHOD_MASK         (0x1F << 12)
#define SSP_RFTHOD_MASK         (0x1F << 7)

#define SSP_TXDMAEN             (0x1 << 5)  /* TX DMA Enable */
#define SSP_RXDMAEN             (0x1 << 4)  /* RX DMA Enable */
#define SSP_TFIEN               (0x1 << 3)  /* TX FIFO Int Enable */
#define SSP_RFIEN               (0x1 << 2)  /* RX FIFO Int Enable */
#define SSP_TFURIEN             (0x1 << 1)  /* TX FIFO Underrun int enable */
#define SSP_RFORIEN             (0x1 << 0)  /* RX FIFO Underrun int enable */

// status register
#define SSP_TFVE                (0x3F << 12) /* Tx FIFO Valid Entries */
#define SSP_RFVE                (0x3F << 4)  /* Rx FIFO Valid Entries */

#define SSP_BUSY                (0x1 << 2)
#define SSP_TFNF                (0x1 << 1)
#define SSP_RFF                 (0x1 << 0)

struct ftssp010_pcm_ops;
typedef struct ftssp010_pcm_ops     ftssp010_pcm_ops_t;
typedef struct snd_pcm_substream    snd_pcm_substream_t;
typedef struct snd_pcm_runtime      snd_pcm_runtime_t;
typedef struct snd_card             snd_card_t;
typedef struct snd_pcm              snd_pcm_t;
typedef struct snd_kcontrol_new     snd_kcontrol_new_t;

/* this structure is used by codec */
struct ftssp010_pcm_ops {
    struct module *owner;
    int (*startup) (ftssp010_pcm_ops_t *ops);
    int (*open) (ftssp010_pcm_ops_t *ops, snd_pcm_substream_t *substream);
    int (*prepare) (ftssp010_pcm_ops_t *ops,
                    snd_pcm_substream_t *substream, snd_pcm_runtime_t * rt);
    /* dma width */
    int (*get_dma_parameters) (ftssp010_pcm_ops_t * ops,
                               snd_pcm_substream_t * stream, int *dma_width);
    int (*close) (ftssp010_pcm_ops_t *ops, snd_pcm_substream_t *substream);
    void (*shutdown) (ftssp010_pcm_ops_t *ops);

#ifdef CONFIG_PM
    int (*suspend) (ftssp010_pcm_ops_t *ops);
    int (*resume) (ftssp010_pcm_ops_t *ops);
#endif /* CONFIG_PM */
};

typedef struct {
    struct dma_chan *dma_chan;
    dma_cap_mask_t  cap_mask;
    dma_cookie_t    cookie;
    struct ftdmac020_dma_slave      dma_slave_config;
    struct dma_async_tx_descriptor  *desc;
} dmaengine_t;

typedef struct {
    u32 dma_width;
    u32 state;
#define ST_RUNNING		(1<<0)
#define ST_OPENED		(1<<1)
#define ST_PREPARE      (1<<2)
    //spinlock_t lock;
    snd_pcm_substream_t *substream;
    dmaengine_t         dmaengine;
} ftssp010_runtime_t;

typedef struct {
    unsigned long jif_open;
    unsigned long jif_trigger;
    unsigned long jif_close;
} ftssp010_time_t;

enum {
    FTSSP_MCLK_2048K = 0,
    FTSSP_MCLK_2822K,
    FTSSP_MCLK_4096K,
    FTSSP_MCLK_5645K,
    FTSSP_MCLK_8192K,
    FTSSP_MCLK_11290K,
    FTSSP_MCLK_12288K,
    FTSSP_MCLK_16384K,
    FTSSP_MCLK_24576K,
    FTSSP_MCLK_NUMBER,
};

typedef struct {
    u32 *fs_table;
    u32 *sspclk_table;
    u32 *bclk_table;    //If codec is master, bclk must be provided.
    u8  number;
} ftssp_clock_table_t;

typedef struct {
    int cardno;
    u32 vbase;
    u32 pbase;
    u32 dmac_pbase;
    u32 dmac_vbase;
    int mem_len;
    int irq;
    int pmu_fd;
    u32 SSP_CR0;
    u32 sample_rate;
    u32 playback_underrun_flag;
    u32 total_channels;
    snd_card_t *card;
    snd_pcm_t *pcm;
    void *client;
    snd_kcontrol_new_t capture_ctl;
    snd_kcontrol_new_t playback_ctl;
    ftssp010_pcm_ops_t *chip_ops;
    struct semaphore sem;
    ftssp010_runtime_t playback;
    ftssp010_runtime_t capture;
    u8 chip_ops_claimed;
    u8 codec_fmtbit;
    ftssp010_time_t     ssp_time;
    ftssp_clock_table_t *clock_table;
} ftssp010_card_t;

/*
 * Define the hardware platform related info.
 */
typedef struct {
    unsigned int rates;
	unsigned int rate_min;
	unsigned int rate_max;
    int (*pmu_set_mclk)(ftssp010_card_t *card, u32 speed, u32 *mclk, u32 *bclk,
			                                            ftssp_clock_table_t *clk_tab);
	int (*pmu_set_pmu)(ftssp010_card_t *card);
} ftssp_hardware_platform_t;

extern ftssp_hardware_platform_t ftssp_hw_platform;
extern int ftssp010_init_platform_device(ftssp010_card_t *card);
extern void ftssp010_deinit_platform_device(ftssp010_card_t *card);
extern int ftssp010_init_platform_driver(void);
extern void ftssp0101_deinit_platform_driver(void);
extern void ftssp010_ssp_clear_and_reset(u32 base);
extern void ftssp010_ssp_txctrl(u32 base, int on);
extern void ftssp010_ssp_rxctrl(u32 base, int on);
extern void ftssp010_ssp_rxtxctrl(u32 base, int on);
extern void ftssp010_ssp_hw_init(u32 base);
extern void ftssp010_ssp_cfg(u32 base, unsigned long set, unsigned long mask);
extern int ftssp010_ssp_set_rate(ftssp010_card_t * card, ftssp010_runtime_t * run);
extern void ftssp010_ssp_dma_init(ftssp010_card_t * card, u32 dir);

extern ftssp010_card_t *ftssp010_ssp_probe(int cardno, struct module *this);
extern int ftssp010_ssp_remove(ftssp010_card_t * card);

#endif /* __SSP_FTSSP010_H__ */
