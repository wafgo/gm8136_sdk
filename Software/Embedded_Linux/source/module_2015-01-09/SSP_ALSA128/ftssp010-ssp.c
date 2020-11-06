#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <mach/platform/board.h>
#include <mach/ftdmac020.h>
#include <mach/platform/platform_io.h>
#include <linux/hardirq.h>
//#include <mach/ahb_dma.h>

#include <sound/core.h>
#include <sound/info.h>
#include <sound/control.h>

#include "ftssp010-ssp.h"
#include "ftssp010-pcm.h"
#include "debug.h"

//#define IPMODULE SSP
#define IPMODULE I2S_PCIE
#define IPNAME   FTSSP010

#define IsMaster(card)  (card->SSP_CR0 & SSP_MS_MASK)

/* Get water mark in FIFO
 */
unsigned int ftssp010_ssp_get_watermark(unsigned int base, int stream)
{
    unsigned int    status, value;

    status = readl(base + FTSSP010_SSP_STATUS);

    value = (stream == SNDRV_PCM_STREAM_PLAYBACK) ? ((status & SSP_TFVE) >> 12) : ((status & SSP_RFVE) >> 4);

    return value;
}

static inline void clear_ssp_tx_error_irq(unsigned int base)
{
	/* clear SSP irq status */
	while (readl(base + FTSSP010_SSP_INT_STATUS) & 0x2);
}

static inline void clear_ssp_rx_error_irq(unsigned int base)
{
	/* clear SSP irq status */
	while (readl(base + FTSSP010_SSP_INT_STATUS) & 0x1);
}

static inline void fill_ssp_buffer(unsigned int base)
{
	volatile int i, try_cnt = 0;
	u32     tmp;

    tmp = readl(base + FTSSP010_SSP_CR2);
	tmp |= SSP_TXFCLR;

    if (SSP_MAX_SDL == 128)
        tmp &= ~SSP_TXEN;
    else
		tmp &= ~SSP_SSPEN;
	writel(tmp, base + FTSSP010_SSP_CR2);

	while (readl(base + FTSSP010_SSP_STATUS) & SSP_TFNF) {
    	for (i = 0; i < SSP_FIFO_DEEPTH; i++)
    		writel(0xFF, base + FTSSP010_SSP_DATA);
        try_cnt ++;
        if (try_cnt > 500) {
	        printk("%s, why......(0x8=0x%x, 0xC=0x%x 0x10=0x%x) \n", __func__, readl(base+0x8),
	                        readl(base+0xC), readl(base+0x10));
	        break;
        }
    }
}

static inline void clear_ssp_buffer(unsigned int base)
{
	volatile u32 tmp, try_cnt = 0;

	tmp = readl(base + FTSSP010_SSP_CR2);
	tmp |= SSP_RXFCLR;
    if (SSP_MAX_SDL == 128)
        tmp &= ~SSP_RXEN;
    else
		tmp &= ~SSP_SSPEN;
	writel(tmp, base + FTSSP010_SSP_CR2);

	/* status check again until the rx fifo is real clear */
	while (readl(base + FTSSP010_SSP_STATUS) & SSP_RFVE) {
	    writel(tmp, base + FTSSP010_SSP_CR2);
	    try_cnt ++;
	    if (try_cnt > 1000)
	        panic("%s, why...... \n", __func__);
	}
}

void ftssp010_ssp_dma_init(ftssp010_card_t *card, u32 dir)
{
	if (dir == DMA_FROM_DEVICE) {
		clear_ssp_buffer(card->vbase);
		clear_ssp_rx_error_irq(card->vbase);
	} else {
		fill_ssp_buffer(card->vbase);
		clear_ssp_tx_error_irq(card->vbase);
	}
}

void ftssp010_ssp_clear_and_reset(u32 base)
{
    //reset ssp status
	writel(SSP_RESET, base + FTSSP010_SSP_CR2);

	//clear fifo and disable
	writel(SSP_TXFCLR | SSP_RXFCLR, base + FTSSP010_SSP_CR2);
}

inline void ftssp010_ssp_setreg(u32 base, u32 reg, u32 set, u32 mask)
{
	u32 tmp;
	tmp = readl(base + reg);
	tmp &= ~mask;
	tmp |= set;
	writel(tmp, base + reg);
}

/* Turn on/off transmit function
 */
void ftssp010_ssp_txctrl(u32 base, int on)
{
	u32 tmp;

	if (on) {
		tmp = (SSP_TXDMAEN | SSP_TFURIEN);
		ftssp010_ssp_setreg(base, FTSSP010_SSP_INT_CR,
				    tmp | SSP_TXFIFOTHOD,
				    tmp | SSP_TFTHOD_MASK);

        tmp = (SSP_SSPEN | SSP_TXDOE | SSP_TXEN);
		ftssp010_ssp_setreg(base, FTSSP010_SSP_CR2, tmp, tmp);
        /* must ensure the txfifo is full! */
    } else {
        /* disable tx */
        tmp = (SSP_TXDOE | SSP_TXEN);
        ftssp010_ssp_setreg(base, FTSSP010_SSP_CR2, 0, tmp);

		tmp = (SSP_TXDMAEN | SSP_TFURIEN | SSP_TFTHOD_MASK);
		ftssp010_ssp_setreg(base, FTSSP010_SSP_INT_CR, 0, tmp);
	}
}

void ftssp010_ssp_rxctrl(u32 base, int on)
{
	u32 tmp;

	if (on) {
		tmp = (SSP_RXDMAEN | SSP_RFORIEN);
		ftssp010_ssp_setreg(base, FTSSP010_SSP_INT_CR,
				    tmp | SSP_RXFIFOTHOD,
				    tmp | SSP_RFTHOD_MASK);

        tmp = (SSP_SSPEN | SSP_RXEN);
		ftssp010_ssp_setreg(base, FTSSP010_SSP_CR2, tmp, tmp);
    } else {
        /* disable rx */
        tmp = SSP_RXEN;
        ftssp010_ssp_setreg(base, FTSSP010_SSP_CR2, 0, tmp);

		tmp = (SSP_RXDMAEN | SSP_RFORIEN | SSP_RFTHOD_MASK);
		ftssp010_ssp_setreg(base, FTSSP010_SSP_INT_CR, 0, tmp);
	}
}

void ftssp010_ssp_hw_init(u32 base)
{
	/* clear control 1 */ /* CANNOT ENABLE THIS CODE, IT WILL CAUSE FILL SSP FIFO FAIL */
	//writel(0, base + FTSSP010_SSP_CR1);

	/* int control */
	writel(0, base + FTSSP010_SSP_INT_CR);

	/* SSP state machine reset */
	writel(SSP_RESET, base + FTSSP010_SSP_CR2);
}

void ftssp010_ssp_cfg(u32 base, unsigned long set, unsigned long mask)
{
	unsigned long tmp;

	tmp = readl(base + FTSSP010_SSP_CR0);
	tmp &= ~mask;
	tmp |= set;

	DBGPRINT(2, "i2scfg=0x%08lx\n", tmp);

	writel(tmp, base + FTSSP010_SSP_CR0);
}

/*
 * This function set sample rate in PMU and SDL, PDL bit in SSP
 */
int ftssp010_ssp_set_rate(ftssp010_card_t *card, ftssp010_runtime_t * run)
{
	struct snd_pcm_substream *substream;
	int                 ret = 0;
	u32                 mclk, bclk, pdl_bit;
	u32                 tmp1, sdl_bit = card->codec_fmtbit;

	substream   = card->pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;

	ret = ftssp_hw_platform.pmu_set_mclk(card, run->substream->runtime->rate, &mclk, &bclk,
					                                        card->clock_table);
    if (ret < 0) {
        printk("Don't support this sample rate %d!\n", run->substream->runtime->rate);
        return ret;
    }

    /*codecs should be responsible for initializing the field, "total_channels"
      so that the calculation of SDL and PDL is correct*/
    if (card->total_channels == 0)
        panic("%s, card->total_channels should not be zero!!\n", __func__);

    /* mono ? */
    sdl_bit = (card->total_channels == 1) ? card->codec_fmtbit :
                                            card->codec_fmtbit * (card->total_channels / 2);

    /* Check SSP is mater or slave */
	if (IsMaster(card)) {
	    /* Find divisor to generate bclk to audio codec.
	     * Formula: bclk = mclk/(2*(divior+1))
	    */
		//tmp1 = (mclk / (2 * bclk)) - 1;     //divisor
		tmp1 = (mclk * 10) / bclk;
		tmp1 += 5;
		tmp1 /= 10;
		tmp1 /= 2;
		tmp1 -= 1;
		if (tmp1 <= 0)
          tmp1 = 1;

	    pdl_bit = (bclk / (2 * run->substream->runtime->rate)) - sdl_bit;
	    if (pdl_bit > 0xff)
			panic("card%d, padding data length is overrun.(0x%x)", card->cardno, pdl_bit);

		DBGPRINT(3, "Master: MCLK=%d,Div=%d,FS=%d(sz=%d, bclk=%d)\n", mclk, tmp1, run->substream->runtime->rate,
		        card->codec_fmtbit, bclk);

        if (SSP_MAX_SDL == 128)
    		tmp1 = tmp1 | ((sdl_bit - 1) & 0x7F) << 16 | ((pdl_bit & 0xFF) << 24);
    	else
            tmp1 = tmp1 | ((sdl_bit - 1) & 0x1F) << 16 | ((pdl_bit & 0xFF) << 24);

    } else {
        DBGPRINT(3, "Slave: mclk=%d, BCLK=%d, FS=%d\n", mclk, bclk, run->substream->runtime->rate);

		/* bclk = fs * total_channels * sample_size.
		 * frame bit for each side = bclk / (fs * 2)
		 */
		pdl_bit = ((bclk / (run->substream->runtime->rate*2)) - sdl_bit); // pdl

		if (pdl_bit > 0xff)
			panic("card%d, padding data length is overrun.(0x%x)", card->cardno, pdl_bit);

        if (SSP_MAX_SDL == 128)
            tmp1 = (((sdl_bit - 1) & 0x7F) << 16) | ((pdl_bit & 0xFF) << 24);
        else
            tmp1 = (((sdl_bit - 1) & 0x1F) << 16) | ((pdl_bit & 0xFF) << 24);
	}

	writel(tmp1, card->vbase + FTSSP010_SSP_CR1);

	return ret;
}

void ftssp010_ssp_dma_callback(void *param)
{
	ftssp010_runtime_t *run = (ftssp010_runtime_t *)param;

	snd_pcm_period_elapsed(run->substream);
}

static irqreturn_t ftssp010_ssp_irq_handler(int irq, void *dev_id)
{
	ftssp010_card_t  *card = (ftssp010_card_t *)dev_id;
	u32 status, overrun = 0, underrun = 0;

    status = readl(card->vbase + FTSSP010_SSP_INT_STATUS);
	DBGPRINT(8, "SSP_IRQ_Status=%08x\n", status);

    if ((status & 0x1) == 0x1) {
		//RX over-run
        ftssp010_ssp_setreg(card->vbase, FTSSP010_SSP_INT_CR, 0, SSP_RFIEN | SSP_RFORIEN);
		printk("\r\n Card %d:RX over run", card->cardno);
        overrun = 1;
    }

    if ((status & 0x2) == 0x2) {
		//TX under-run
        ftssp010_ssp_setreg(card->vbase, FTSSP010_SSP_INT_CR, 0, SSP_TFIEN | SSP_TFURIEN);
		printk("\r\n Card %d:TX under run", card->cardno);
        underrun = 1;
    }

    if (overrun && (card->capture.state & ST_RUNNING)) {
        struct snd_pcm_runtime *runtime;
        struct snd_pcm_substream *substream;
        ftssp010_runtime_t  *or;

        /* capture */
        substream = card->capture.substream;
        runtime = substream->runtime;
        or = &card->capture;

        dmaengine_terminate_all(or->dmaengine.dma_chan);
        /* chain the llp */
        dmaengine_slave_config(or->dmaengine.dma_chan, &or->dmaengine.dma_slave_config.common);
        card->capture.dmaengine.desc = dmaengine_prep_dma_cyclic(or->dmaengine.dma_chan,
                                                runtime->dma_addr,
                                                snd_pcm_lib_buffer_bytes(substream),
                                                snd_pcm_lib_period_bytes(substream),
                                                DMA_FROM_DEVICE);
        ftssp010_ssp_dma_init(card, DMA_FROM_DEVICE);
        or->dmaengine.desc->callback = ftssp010_ssp_dma_callback;
        or->dmaengine.desc->callback_param = or;
        or->dmaengine.cookie = dmaengine_submit(or->dmaengine.desc);

        dma_async_issue_pending(or->dmaengine.dma_chan);

        /* workaround for "TX under-run" cannot been clean after read at first time*/
        status = readl(card->vbase + FTSSP010_SSP_INT_STATUS);
        /* start ssp */
        ftssp010_ssp_rxctrl(card->vbase, 1);
    }

    if (underrun && (card->playback.state & ST_RUNNING)) {
        struct snd_pcm_runtime *runtime;
        struct snd_pcm_substream *substream;
        ftssp010_runtime_t  *or;

        /* capture */
        substream = card->playback.substream;
        runtime = substream->runtime;
        or = &card->playback;

        /* playback */
        dmaengine_terminate_all(or->dmaengine.dma_chan);

        /* set per channel config */
        dmaengine_slave_config(or->dmaengine.dma_chan, &or->dmaengine.dma_slave_config.common);
        or->dmaengine.desc = dmaengine_prep_dma_cyclic(or->dmaengine.dma_chan,
                                                    runtime->dma_addr,
                                                    snd_pcm_lib_buffer_bytes(substream),
                                                    snd_pcm_lib_period_bytes(substream),
                                                    DMA_TO_DEVICE);
        ftssp010_ssp_dma_init(card, DMA_TO_DEVICE);
        or->dmaengine.desc->callback = ftssp010_ssp_dma_callback;
        or->dmaengine.desc->callback_param = or;
        or->dmaengine.cookie = dmaengine_submit(or->dmaengine.desc);

        dma_async_issue_pending(or->dmaengine.dma_chan);
        /* workaround for "TX under-run" cannot been clean after read at first time*/
        status = readl(card->vbase + FTSSP010_SSP_INT_STATUS);
        /* start ssp */
        ftssp010_ssp_txctrl(card->vbase, 1);
    }

	return IRQ_HANDLED;
}

#ifdef CONFIG_PROC_FS
#if 0
static void snd_ftssp010_proc_read(struct snd_info_entry *entry,
				   struct snd_info_buffer *buffer)
{
	ftssp010_card_t *card = entry->private_data;
	unsigned long tmp;
	struct timespec t_time;

	tmp = card->ssp_time.jif_trigger - card->ssp_time.jif_open;
	jiffies_to_timespec(tmp, &t_time);

	snd_iprintf(buffer, "%s\n\n", card->card->longname);
	snd_iprintf(buffer,
		    "Lantancy time from open to 1st trigger = %u sec + %u msec",
		    (u32) t_time.tv_sec, (u32) t_time.tv_nsec / 1000000);
	snd_iprintf(buffer, "\n");
}

static void __devinit snd_ftssp010_proc_init(ftssp010_card_t * card)
{
	struct snd_info_entry *entry;
	char stmp[23];

	snprintf(stmp, 23, "Lantancy_Time");

	if (!snd_card_proc_new(card->card, stmp, &entry)){
	//TODO: ALSA API change??
	//	snd_info_set_text_ops(entry, card, 1024, snd_ftssp010_proc_read);
	}
}
#else				/* !CONFIG_PROC_FS */
static inline void snd_ftssp010_proc_init(ftssp010_card_t * card)
{
}
#endif
#endif//end of CONFIG_PROC_FS

static int ftssp010_ssp_capture_info (struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *uinfo)
{
	return 0;
}

/* filed definition in uinfo->value.integer.value[x]
 * 0 : consumption
 * 1 : runtime->buffer_size
 */
static int ftssp010_ssp_capture_get (struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *uinfo)
{
    ftssp010_card_t *card = snd_kcontrol_chip(kcontrol);
    struct snd_pcm_substream *substream = card->capture.substream;
    struct snd_pcm_runtime *runtime = substream->runtime;
    snd_pcm_sframes_t consumption;
    unsigned char *ptr;

    /* get value start addres */
    ptr = (unsigned char *)&uinfo->value.integer.value[0];

    consumption = runtime->hw_ptr_interrupt - runtime->control->appl_ptr;
    if (consumption < 0)
        consumption = (runtime->boundary - runtime->control->appl_ptr) + runtime->hw_ptr_interrupt;

    /* get the consumption */
    writel(consumption, ptr);
	ptr += sizeof(snd_pcm_sframes_t);

	/* get the buffer size */
	writel(runtime->buffer_size, ptr);

	return 0;
}

static int ftssp010_ssp_capture_put (struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *uinfo)
{
	return 0;
}

static int ftssp010_ssp_playback_info (struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *uinfo)
{
	return 0;
}

static int ftssp010_ssp_playback_get (struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *uinfo)
{
    ftssp010_card_t *card = snd_kcontrol_chip(kcontrol);
    struct snd_pcm_substream *substream = card->playback.substream;
    struct snd_pcm_runtime *runtime = substream->runtime;
    snd_pcm_sframes_t consumption;
    unsigned char *ptr;

    /* get value start addres */
    ptr = (unsigned char *)&uinfo->value.integer.value[0];

    /* get the underrun flag */
    writel(card->playback_underrun_flag, ptr);
	ptr += sizeof(card->playback_underrun_flag);

    /* reset */
    card->playback_underrun_flag = 0;

    consumption = runtime->control->appl_ptr - runtime->hw_ptr_interrupt;
    if (consumption < 0)
        consumption = (runtime->boundary - runtime->hw_ptr_interrupt) + runtime->control->appl_ptr;

    /* get the consumption */
    writel(consumption, ptr);
	ptr += sizeof(snd_pcm_sframes_t);

	/* get the buffer size */
	writel(runtime->buffer_size, ptr);

    //printk("playback: consumption = %x, appl_ptr = %x, hw_ptr_interrupt = %x \n", consumption, runtime->control->appl_ptr, runtime->hw_ptr_interrupt);

	return 0;
}

static int ftssp010_ssp_playback_put (struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *uinfo)
{
    ftssp010_card_t *card = snd_kcontrol_chip(kcontrol);

    /* set the underrun flag */
    card->playback_underrun_flag = uinfo->value.integer.value[0];

    if (card->playback_underrun_flag)
        DBGPRINT(6, "\n\r underrun occurs");

	return 0;
}

/*
 * First entry point from codec.
 * cardno indicates the SSP number
 */
ftssp010_card_t *ftssp010_ssp_probe(int cardno, struct module *this)
{
	struct snd_card *card;
	int ret;
	ftssp010_card_t *my_card;
	char stmp[23];
	struct snd_kcontrol *ctl;

    memset(stmp, 0, sizeof(stmp));
	snprintf(stmp, 23, "FTSSP010#%d", cardno);

    /* includes extra buffer and kernel use kzalloc() to allocate memory */
	snd_card_create(cardno, stmp, this, sizeof(ftssp010_card_t) /* extra_size */, &card);
	if (card == NULL)
		panic("%s, snd_card_create, card%d fail! \n", __func__, cardno);

	my_card             = (ftssp010_card_t *) card->private_data;
	my_card->cardno     = cardno;
	my_card->card       = card;
    DBGPRINT(1, "Driver for FTSSP010 #%d (Physical Addr=0x%08X)\n", cardno, my_card->pbase);

    /* envoke probe function defined in ftssp010-gm82xx.c */
	ret = ftssp010_init_platform_device(my_card);
	if(ret < 0)
		panic("%s, init_platform_device, card%d fail! \n", __func__, my_card->cardno);

	sema_init(&my_card->sem, 1);
//	spin_lock_init(&my_card->capture.lock);
//	spin_lock_init(&my_card->playback.lock);

	strcpy(card->driver, stmp);
	strcpy(card->shortname, stmp);
	strcpy(card->longname, stmp);

	/* PCM */
	ret = ftssp010_snd_new_pcm(my_card);
	if (ret)
		panic("%s, ftssp010_snd_new_pcm, card%d fail! \n", __func__, my_card->cardno);

    if (ftssp_hw_platform.pmu_set_pmu(my_card) < 0)
        panic("%s, cardno:%d set pmu fail! \n", __func__, my_card->cardno);

	ret = request_irq(my_card->irq, ftssp010_ssp_irq_handler, 0, card->shortname, my_card);
	if (ret)
		panic("%s, request irq%d fail! \n", __func__, my_card->irq);

	snd_ftssp010_proc_init(my_card);

	info("SoundCard(%d) attached OK(0x%x)", my_card->cardno, (u32)my_card);

    my_card->capture_ctl.iface = SNDRV_CTL_ELEM_IFACE_PCM;
    my_card->capture_ctl.info = (snd_kcontrol_info_t *)ftssp010_ssp_capture_info;
    my_card->capture_ctl.get = (snd_kcontrol_get_t *)ftssp010_ssp_capture_get;
    my_card->capture_ctl.put = (snd_kcontrol_put_t *)ftssp010_ssp_capture_put;

    my_card->playback_ctl.iface = SNDRV_CTL_ELEM_IFACE_PCM;
    my_card->playback_ctl.info = (snd_kcontrol_info_t *)ftssp010_ssp_playback_info;
    my_card->playback_ctl.get = (snd_kcontrol_get_t *)ftssp010_ssp_playback_get;
    my_card->playback_ctl.put = (snd_kcontrol_put_t *)ftssp010_ssp_playback_put;

    /* add capture control interface */
    ctl = snd_ctl_new1(&my_card->capture_ctl, my_card);
    if (!ctl)
        panic("%s, capture snd_ctl_new1, card%d fail! \n", __func__, my_card->cardno);

    memset(ctl->id.name, 0, sizeof(ctl->id.name));
	snprintf(ctl->id.name, 23, "CaptureCtrl%d", cardno);

    ctl->id.index = 0;
    ctl->private_value = 0;
    ctl->private_data = my_card;
    if ((ret = snd_ctl_add(my_card->card, ctl)) < 0) {
        snd_ctl_free_one(ctl);
    	panic("%s, capture snd_ctl_add, card%d fail! \n", __func__, my_card->cardno);
    }

    /* add playback control interface */
    ctl = snd_ctl_new1(&my_card->playback_ctl, my_card);
    if (!ctl)
        panic("%s, playback snd_ctl_new1, card%d fail! \n", __func__, my_card->cardno);
    memset(ctl->id.name, 0, sizeof(ctl->id.name));
    snprintf(ctl->id.name, 23, "PlaybackCtrl%d", cardno);

    ctl->id.index = 0;
    ctl->private_value = 0;
    ctl->private_data = my_card;
	if ((ret = snd_ctl_add(my_card->card, ctl)) < 0) {
		snd_ctl_free_one(ctl);
		panic("%s, playback snd_ctl_add, card%d fail! \n", __func__, my_card->cardno);
	}

	return my_card;
}

int ftssp010_ssp_remove(ftssp010_card_t *card)
{
	DBGPRINT(1, "card[%d] = %p\n", card->cardno, card);

	free_irq(card->irq, card);

    if (card->capture.dmaengine.dma_chan)
	    dmaengine_terminate_all(card->capture.dmaengine.dma_chan);
    if (card->playback.dmaengine.dma_chan)
	    dmaengine_terminate_all(card->playback.dmaengine.dma_chan);

	snd_card_free(card->card);

	ftssp010_deinit_platform_device(card);

	return 0;
}

EXPORT_SYMBOL(ftssp010_ssp_hw_init);
EXPORT_SYMBOL(ftssp010_ssp_cfg);
EXPORT_SYMBOL(ftssp010_ssp_probe);
EXPORT_SYMBOL(ftssp010_ssp_remove);

static int __init ftssp_drv_init(void)
{
	int ret = 0;

	if(unlikely((ret = ftssp010_init_platform_driver()) != 0)){
		printk("%s fails: ftssp010_init_platform_driver not OK.\n", __FUNCTION__);
		return ret;
	}

    dbg_proc_init();
	info("common[ver:%s] INIT OK!", FTSSP101_VERSION);
	return ret;
}

static void __exit ftssp_drv_exit(void)
{
	ftssp0101_deinit_platform_driver();
	dbg_proc_remove();
}

module_init(ftssp_drv_init);
module_exit(ftssp_drv_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("GM SSP-ALSA driver");
MODULE_LICENSE("GPL");
