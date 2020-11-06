#include <linux/dma-mapping.h>
#include <linux/jiffies.h>

#include <asm/io.h>
//#include <mach/ahb_dma.h>
#include <linux/dmaengine.h>
#include <mach/ftapbb020.h>
#include <mach/ftdmac020.h>
//#include <mach/ftdmac030.h>
#include <mach/dma_gm.h>
#include <linux/dma-mapping.h>
#include <sound/core.h>
#include <sound/pcm.h>

#include "ftssp010-ssp.h"
#include "ftssp010-pcm.h"
#include "debug.h"

static struct snd_pcm_hardware ftssp010_snd_hw = {
	.info =
	    (SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
	     SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID),
	.formats =
	    (SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_U32_LE |
	     SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE |
	     SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S8),
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = 0x30000,
    .period_bytes_min = 0x1000,
    .period_bytes_max = 0x1000,
    .periods_min = 16,
    .periods_max = 16,
};

extern void ftssp010_ssp_dma_callback(void *param);

static int call_startup(ftssp010_pcm_ops_t * ops)
{
	DBGENTER(1);
	if (ops && ops->startup)
		return (ops->startup) (ops);
	DBGLEAVE(1);
	return 0;
}

static int call_open(ftssp010_pcm_ops_t * ops,
		     struct snd_pcm_substream *substream)
{
	DBGENTER(1);
	if (ops && ops->open)
		return (ops->open) (ops, substream);
	DBGLEAVE(1);
	return 0;
}

static int call_prepare(ftssp010_pcm_ops_t *ops,
			struct snd_pcm_substream *substream,
			struct snd_pcm_runtime *rt)
{
	DBGENTER(1);
	if (ops && ops->prepare)
		return (ops->prepare) (ops, substream, rt);
	DBGLEAVE(1);
	return 0;
}

static int call_get_dma_parameters(ftssp010_pcm_ops_t * ops,
                                   struct snd_pcm_substream *substream,
                                   int *dma_width)
{
    DBGENTER(1);
    if (ops && ops->get_dma_parameters){
            return (ops->get_dma_parameters) (ops, substream, dma_width);
    }
    DBGLEAVE(1);
    return 0;
}

static int call_close(ftssp010_pcm_ops_t *ops,
		      struct snd_pcm_substream *substream)
{
	DBGENTER(1);
	if (ops && ops->close)
		return (ops->close) (ops, substream);
	DBGLEAVE(1);
	return 0;
}

static void call_shutdown(ftssp010_pcm_ops_t * ops)
{
	DBGENTER(1);
	if (ops && ops->shutdown)
		(ops->shutdown) (ops);
	DBGLEAVE(1);
}

#ifdef CONFIG_PM
static int call_suspend(ftssp010_pcm_ops_t * ops)
{
	DBGENTER(1);
	if (ops && ops->suspend)
		return (ops->suspend) (ops);
	DBGLEAVE(1);
	return 0;
}

static int call_resume(ftssp010_pcm_ops_t * ops)
{
	DBGENTER(1);
	if (ops && ops->resume)
		return (ops->resume) (ops);
	DBGLEAVE(1);
	return 0;
}
#endif				/* CONFIG_PM */

/* open callback */
static int ftssp010_snd_open(struct snd_pcm_substream *substream)
{
	ftssp010_card_t *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	ftssp010_runtime_t *or;
	int ret = 0;

	DBGENTER(1);
	down(&chip->sem);

	chip->ssp_time.jif_trigger = 0;
	chip->ssp_time.jif_open = jiffies;

	/* todo - request dma channel nicely */
	DBGPRINT(2, "state: playback=0x%x, capture=0x%x\n",
		 chip->playback.state, chip->capture.state);

	/* initialise the stream */
	ftssp010_snd_hw.rates    = ftssp_hw_platform.rates;
	ftssp010_snd_hw.rate_min = ftssp_hw_platform.rate_min;
	ftssp010_snd_hw.rate_max = ftssp_hw_platform.rate_max;

	runtime->hw = ftssp010_snd_hw;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		or = &chip->capture;
	else
		or = &chip->playback;

    DBGPRINT(6, "\n\r %s, stream:%d", __func__, substream->stream);

	if (chip->playback.state == 0 && chip->capture.state == 0)
	{
		/* ensure we hold all the modules we need */
		ftssp010_ssp_clear_and_reset(chip->vbase);

 		/* reset the dma for both */
 		if (chip->capture.dmaengine.dma_chan)
		    dmaengine_terminate_all(chip->capture.dmaengine.dma_chan);
        if (chip->playback.dmaengine.dma_chan)
            dmaengine_terminate_all(chip->playback.dmaengine.dma_chan);

		if (chip->chip_ops) {
			if (try_module_get(chip->chip_ops->owner))
				chip->chip_ops_claimed = 1;
			else {
				err("cannot claim module");
				ret = -EINVAL;
				goto exit_err;
			}
		}

		/* ensure the chip is started */
		ret = call_startup(chip->chip_ops);
		if (ret)
			goto exit_err;
	}

	/* call all the registered helpers */
	ret = call_open(chip->chip_ops, substream);
	if (ret < 0)
		goto exit_err;

    or->state = 0;
	or->state |= ST_OPENED;
	runtime->private_data = or;
	or->substream = substream;

	up(&chip->sem);
	DBGLEAVE(1);
	return ret;

exit_err:
    module_put(chip->chip_ops->owner);
	up(&chip->sem);
	DBGLEAVE(1);
	return ret;
}

/* close callback */
static int ftssp010_snd_close(struct snd_pcm_substream *substream)
{
	ftssp010_card_t     *chip = snd_pcm_substream_chip(substream);

	DBGENTER(1);

    DBGPRINT(6, "\n\r %s 0, stream:%d",__func__, substream->stream);

	down(&chip->sem);

	/* close all the devices associated with this */
	call_close(chip->chip_ops, substream);

    if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
        chip->capture.state = 0;
    }
    else {
        chip->playback.state = 0;
    }

	/* are both shut-down? */
	if (chip->playback.state == 0 && chip->capture.state == 0) {
        DBGPRINT(6, "\n\r %s 4, stream:%d",__func__, substream->stream);

		/* call shut-down methods for all */
		call_shutdown(chip->chip_ops);
		if (chip->chip_ops_claimed) {
			chip->chip_ops_claimed = 0;
			module_put(chip->chip_ops->owner);
		}
		ftssp010_ssp_clear_and_reset(chip->vbase);
	}

	chip->ssp_time.jif_close = jiffies;

	up(&chip->sem);

	DBGLEAVE(1);
	return 0;
}

/* hw_params callback */
static int ftssp010_snd_pcm_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *hw_params)
{
	DBGENTER(1);
	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	DBGLEAVE(1);
	return 0;
}

/* hw_free callback */
static int ftssp010_snd_pcm_hw_free(struct snd_pcm_substream *substream)
{
	DBGENTER(1);
	snd_pcm_set_runtime_buffer(substream, NULL);
	DBGLEAVE(1);
	return 0;
}

/* prepare callback */
static int ftssp010_snd_pcm_prepare(struct snd_pcm_substream *substream)
{
	ftssp010_card_t *chip = snd_pcm_substream_chip(substream);
	ftssp010_runtime_t *or = substream->runtime->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
    struct dma_slave_config *common;
    int ret = 0;
	u32 tmp, state;

    if (!(or->state & ST_OPENED))
        return 0;

	DBGENTER(1);
	down(&chip->sem);
	//spin_lock(&or->lock);

    DBGPRINT(6, "\n\r %s 0, stream:%d, state:%d",__func__, substream->stream, or->state);

    /* get the other's state */
    if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
        state = chip->playback.state;
    else
        state = chip->capture.state;

    /* re-configure, this case is only for vplay case. */
    if (or->state & ST_PREPARE)  {
        /* the other is running */
        if (state & ST_RUNNING) {
            /* different sample rate */
            if (runtime->rate != chip->sample_rate) {
                //spin_unlock(&or->lock);
                ret = call_prepare(chip->chip_ops, substream, runtime);
            }
            up(&chip->sem);
            DBGLEAVE(1);
            return ret;
        }
    }

	if ((chip->playback.state & ST_RUNNING) || (chip->capture.state & ST_RUNNING)) {
		if (chip->sample_rate != runtime->rate) {
			err("%s is running sample rate in %d.",
			    (substream->stream==SNDRV_PCM_STREAM_CAPTURE)?"Playing":"Recording",
			    chip->sample_rate);
			err("Please change sample rate from %d to %d.",
			    runtime->rate, chip->sample_rate);
			ret = -EBUSY;
			goto exit_err;
		}
	}

	switch (runtime->format) {
		case SNDRV_PCM_FORMAT_S8:
		case SNDRV_PCM_FORMAT_U8:
			DBGPRINT(2, "set sample size == SNDRV_PCM_FORMAT_X8\n");
			chip->codec_fmtbit = 8;
			break;
		case SNDRV_PCM_FORMAT_S16_LE:
		case SNDRV_PCM_FORMAT_U16_LE:
			DBGPRINT(2, "set sample size == SNDRV_PCM_FORMAT_X16\n");
			chip->codec_fmtbit = 16;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
		case SNDRV_PCM_FORMAT_U32_LE:
			DBGPRINT(2, "set sample size == SNDRV_PCM_FORMAT_X32\n");
			chip->codec_fmtbit = 32;
			break;
		default:
			ret = -EINVAL;
			err("No support %d Format", runtime->format);
			goto exit_err;
	}

    /* dma_width depends the codec */
	ret = call_get_dma_parameters(chip->chip_ops, substream, &or->dma_width);
	if (ret < 0){
		DBGPRINT(2, "fail to get audio codec's dma related parameters\n");
		goto exit_err;
	}

    /* The following setting for burst size and common->dst_addr_width depends on the pre-defined
     * SSP_DMA_BURST_SIZE = 4
     */
    if (SSP_DMA_BURST_SIZE != 4)
        panic("%s, burst_sz: %d \n", __func__, SSP_DMA_BURST_SIZE);

    common = &or->dmaengine.dma_slave_config.common;
    if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
        common->direction = DMA_DEV_TO_MEM;
        common->src_addr = chip->pbase + FTSSP010_SSP_DATA;
        common->dst_addr = runtime->dma_addr;

        /* Note: most cases are 32 bits. */
        switch (or->dma_width) {
          case 1:
            common->src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
            common->dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
            break;
          case 2:
            common->src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
            common->dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
            break;
          case 4:
            common->src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
            common->dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
            break;
          default:
            panic("%s, invalid value %d \n", __func__, or->dma_width);
            break;
        }

        or->dmaengine.dma_slave_config.src_size = FTDMAC020_BURST_SZ_4;

        /* set per channel config */
        dmaengine_slave_config(or->dmaengine.dma_chan, common);

        /* chain the llp */
        or->dmaengine.desc = dmaengine_prep_dma_cyclic(or->dmaengine.dma_chan,
                                                runtime->dma_addr,
                                                snd_pcm_lib_buffer_bytes(substream),
                                                snd_pcm_lib_period_bytes(substream),
                                                DMA_DEV_TO_MEM);
        /* clear rx buffer */
		ftssp010_ssp_dma_init(chip, DMA_FROM_DEVICE);

        DBGPRINT(2, "SNDRV_PCM_STREAM_CAPTURE: src_addr:0x%x, dst_addr:0x%x, src_width:%d, dst_width:%d \n",
                    common->src_addr, common->dst_addr, common->src_addr_width, common->dst_addr_width);
		DBGPRINT(2, "SNDRV_PCM_STREAM_CAPTURE: 0x%x, 0x%x, 0x%x", substream->runtime->dma_addr,
		            snd_pcm_lib_buffer_bytes(substream), snd_pcm_lib_period_bytes(substream));
    } else {
        /* SNDRV_PCM_STREAM_PLAYBACK */
        common->direction = DMA_MEM_TO_DEV;
        common->src_addr = runtime->dma_addr;
        common->dst_addr = chip->pbase + FTSSP010_SSP_DATA;

        /* Note: most cases are 32 bits. */
        switch (or->dma_width) {
          case 1:
            common->src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
            common->dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
            break;
          case 2:
            common->src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
            common->dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
            break;
          case 4:
            common->src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
            common->dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
            break;
          default:
            panic("%s, invalid value %d \n", __func__, or->dma_width);
            break;
        }

        if (SSP_DMA_BURST_SIZE != 4) {
            panic("%s, burst_sz: %d \n", __func__, SSP_DMA_BURST_SIZE);
        } else {
            /* Note: read direction the burst only can be 1 through PCI-e when src width is less than
             * 64es.
             */
            or->dmaengine.dma_slave_config.src_size = FTDMAC020_BURST_SZ_1;
        }

        /* set per channel config */
        dmaengine_slave_config(or->dmaengine.dma_chan, common);
        /* chain the llp */
        or->dmaengine.desc = dmaengine_prep_dma_cyclic(or->dmaengine.dma_chan,
                                                runtime->dma_addr,
                                                snd_pcm_lib_buffer_bytes(substream),
                                                snd_pcm_lib_period_bytes(substream),
                                                DMA_MEM_TO_DEV);
        /* fill tx buffer with dummy data to full */
		ftssp010_ssp_dma_init(chip, DMA_TO_DEVICE);

        DBGPRINT(2, "SNDRV_PCM_STREAM_PLAYBACK: src_addr:0x%x, dst_addr:0x%x, src_width:%d, dst_width:%d \n",
                    common->src_addr, common->dst_addr, common->src_addr_width, common->dst_addr_width);
        DBGPRINT(2, "SNDRV_PCM_STREAM_PLAYBACK: %x, %x, %x", substream->runtime->dma_addr,
                        snd_pcm_lib_buffer_bytes(substream), snd_pcm_lib_period_bytes(substream));
	}

    or->dmaengine.desc->callback = ftssp010_ssp_dma_callback;
    or->dmaengine.desc->callback_param = or;
    or->dmaengine.cookie = dmaengine_submit(or->dmaengine.desc);
    /* dma_async_issue_pending(g_data.dma_chan) will be placed in trigger function */

    /* set CR0 */
	chip->sample_rate = runtime->rate;
	DBGPRINT(2, "Card(%d)-%s:Sample Rate=%d, Sample Size = %d\n", chip->cardno,
		 (substream->stream == SNDRV_PCM_STREAM_CAPTURE)?"Rec":"Play",
		 chip->sample_rate, runtime->format);

	tmp = chip->SSP_CR0;
    tmp &= ~SSP_OPM_SLST;

    /* SLAVE : Stereo/Mono */
    if (runtime->channels > 1)
	    tmp |= SSP_OPM_SLST;
    ftssp010_ssp_cfg(chip->vbase, tmp, ~0);

    /* set CR1 */
    ret = ftssp010_ssp_set_rate(chip, or);
	if (ret < 0)
		goto exit_err;

    /* update the state */
    or->state |= ST_PREPARE;

	//spin_unlock(&or->lock);
	/* call the register ops for prepare, to allow things like
	 * sample rate and format to be set
	 */
	ret = call_prepare(chip->chip_ops, substream, runtime);
    DBGPRINT(2, "ftssp010 codec_format = %d, total_channels = %d \r\n", chip->codec_fmtbit,
             chip->total_channels);

	up(&chip->sem);

	DBGLEAVE(1);

	return ret;

exit_err:
	//spin_unlock(&or->lock);
	up(&chip->sem);
	DBGLEAVE(1);
	return ret;
}

/* trigger callback */
static int ftssp010_snd_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	ftssp010_card_t     *chip = snd_pcm_substream_chip(substream);
	ftssp010_runtime_t  *or = substream->runtime->private_data;
	ftssp010_runtime_t *other_or;
	int                 ret = -EINVAL;
	//unsigned long       flags;
   	int                 other_stream;

    other_stream = (substream->stream == SNDRV_PCM_STREAM_CAPTURE) ? SNDRV_PCM_STREAM_PLAYBACK : SNDRV_PCM_STREAM_CAPTURE;
    other_or     = (substream->stream == SNDRV_PCM_STREAM_CAPTURE) ? &chip->playback : &chip->capture;
	DBGENTER(1);

	down(&chip->sem);
	//spin_lock_irqsave(&or->lock, flags);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
        /* enable the dmac */
        dma_async_issue_pending(or->dmaengine.dma_chan);

        if (!(or->state & ST_RUNNING)) {
			or->state |= ST_RUNNING;

			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
                DBGPRINT(6, "\n\r %s 6 start, stream:%d, state:%d",__func__, substream->stream, or->state);
				ftssp010_ssp_rxctrl(chip->vbase, 1);
				DBGPRINT(3, "Card(%d)-Rec: Start DMA!", chip->cardno);
			}
			else {
                DBGPRINT(6, "\n\r %s 7 start, stream:%d, state:%d",__func__, substream->stream, or->state);
				ftssp010_ssp_txctrl(chip->vbase, 1);
				DBGPRINT(3, "Card(%d)-Play: Start DMA!", chip->cardno);
			}

            if (!chip->ssp_time.jif_trigger)
				chip->ssp_time.jif_trigger = jiffies;
		}
		ret = 0;
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	    /* orginal code */
		if (or->state & ST_RUNNING) {
            or->state &= ~ST_RUNNING;
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
                DBGPRINT(6, "\n\r %s 4 stop, stream:%d, state:%d",__func__, substream->stream, or->state);
				ftssp010_ssp_rxctrl(chip->vbase, 0);
				DBGPRINT(3, "Card(%d)-Rec: Stop DMA!", chip->cardno);
			}
			else {
                DBGPRINT(6, "\n\r %s 5 stop, stream:%d, state:%d",__func__, substream->stream, or->state);
				ftssp010_ssp_txctrl(chip->vbase, 0);
				DBGPRINT(3, "Card(%d)-Play: Stop DMA!", chip->cardno);
			}

            dmaengine_terminate_all(or->dmaengine.dma_chan);
		}

		ret = 0;
		break;

	default:
		ret = -EINVAL;
	}

	//spin_unlock_irqrestore(&or->lock, flags);
	up(&chip->sem);
	DBGLEAVE(1);

	return ret;
}

/* pointer callback */
static snd_pcm_uframes_t ftssp010_snd_pcm_pointer(struct snd_pcm_substream *substream)
{
	ftssp010_card_t     *chip = snd_pcm_substream_chip(substream);
	ftssp010_runtime_t  *or = substream->runtime->private_data;
	//unsigned long       flags;
	unsigned long       res = 0;
    void __iomem *base;

    DBGENTER(1);

    base = (void __iomem *)chip->dmac_vbase;

	//spin_lock_irqsave(&or->lock, flags);

    if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
        res = readl(base + FTDMAC020_OFFSET_DST_CH(or->dmaengine.dma_chan->chan_id)) - substream->runtime->dma_addr;
    else
        res = readl(base + FTDMAC020_OFFSET_SRC_CH(or->dmaengine.dma_chan->chan_id)) - substream->runtime->dma_addr;

	//spin_unlock_irqrestore(&or->lock, flags);

	/* we seem to be getting the odd error from the pcm library due
	 * to out-of-bounds pointers. this is maybe due to the dma engine
	 * not having loaded the new values for the channel before being
	 * callled... (todo - fix )
	 */
	if (res >= snd_pcm_lib_buffer_bytes(substream)) {
		if (res > snd_pcm_lib_buffer_bytes(substream)) {
			DBGPRINT(4, "Card(%d)-%s: res %lx >= %lx\n",chip->cardno,
				 (substream->stream == SNDRV_PCM_STREAM_CAPTURE)?"Rec":"Play", res,
				 (long)snd_pcm_lib_buffer_bytes(substream));
        }

		if (res == snd_pcm_lib_buffer_bytes(substream))
			res = 0;
	}

	DBGLEAVE(1);

	return bytes_to_frames(substream->runtime, res);
}

static int ftssp010_snd_pcm_mmap(struct snd_pcm_substream *substream, struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	DBGENTER(1);
	DBGLEAVE(1);
	return dma_mmap_writecombine(substream->pcm->card->dev, vma,
				     runtime->dma_area,
				     runtime->dma_addr,
				     runtime->dma_bytes);
}

/* operators */
static struct snd_pcm_ops ftssp010_pcm_ops = {
	.open = ftssp010_snd_open,
	.close = ftssp010_snd_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = ftssp010_snd_pcm_hw_params,
	.hw_free = ftssp010_snd_pcm_hw_free,
	.prepare = ftssp010_snd_pcm_prepare,
	.trigger = ftssp010_snd_pcm_trigger,
	.pointer = ftssp010_snd_pcm_pointer,
	.mmap = ftssp010_snd_pcm_mmap,
};

static int ftssp010_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	int ret = 0;
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = ftssp010_snd_hw.buffer_bytes_max;

	DBGENTER(1);

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;

	buf->area = dma_alloc_writecombine(pcm->card->dev, size,
					   &buf->addr, GFP_KERNEL);
	if (buf->area == NULL) {
		err("failed to allocate dma buff");
		ret = -ENOMEM;
		goto end;
	}

	buf->bytes = size;
 end:
	DBGLEAVE(1);

	return 0;
}

static void ftssp010_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	DBGENTER(1);

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;
		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;
		dma_free_writecombine(pcm->card->dev, buf->bytes,
				      buf->area, buf->addr);
		buf->area = NULL;
	}
	DBGLEAVE(1);
}

int ftssp010_snd_new_pcm(ftssp010_card_t *card)
{
	struct snd_pcm *pcm;
	int ret;
	char stmp[64];

	DBGENTER(1);

	snprintf(stmp, 23, "FTSSP010#%d-SSD PCM", card->cardno);
	ret = snd_pcm_new(card->card, stmp, 0, 1, 1, &pcm);
	if (ret < 0) {
		err("cannot register new pcm");
		return ret;
	}

	card->pcm = pcm;
	pcm->private_data = card;
	pcm->private_free = ftssp010_pcm_free_dma_buffers;

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &ftssp010_pcm_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &ftssp010_pcm_ops);

	/* allocate dma resources */
	ret = ftssp010_pcm_preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_PLAYBACK);
	if (ret)
		goto err1;

	ret = ftssp010_pcm_preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_CAPTURE);
	if (ret)
		goto err1;

	strcpy(pcm->name, stmp);

	DBGPRINT(1, "Create PCM OK!\n");

	DBGLEAVE(1);

	return 0;

err1:
	DBGPRINT(1, "Create PCM FAIL!\n");
	DBGLEAVE(1);

	return ret;
}

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("GM SSP-ALSA driver");
MODULE_LICENSE("GPL");
