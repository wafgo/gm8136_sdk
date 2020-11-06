/*
 * $Author: slchen $
 * $Date: 2011/08/25 07:12:55 $
 *
 * $Log: fi2s-adda300-c0.c,v $
 * Revision 1.3  2011/08/25 07:12:55  slchen
 * *** empty log message ***
 *
 * Revision 1.2  2010/08/10 09:52:50  mars_ch
 * *: add cat6612 audio codec and code clean for adda300
 *
 * Revision 1.1  2010/07/16 09:06:08  mars_ch
 * +: add 8126 platform configuration and adda300 codec driver
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <ftssp010-ssp.h>
#include "adda300.h"

#undef PFX
#define PFX 					"ADDA300-1"

#ifdef GM8312_SSP_TEST
static enum adda300_port_select cs = ADDA300_DA;
#define adda300_set_fs(cs, fs) \
        {       \
            adda300_set_fs_ad_da(cs, fs);   \
        }
#else
static enum adda300_chip_select cs = ADDA300_FIRST_CHIP;
#endif

struct ftssp010_adda300_t {
    ftssp010_pcm_ops_t ops;
    ftssp010_card_t *card;
    u_int32_t sample_rate;
};

static struct ftssp010_adda300_t *my_chip = NULL;

static u_int32_t g_fstable[] =
    { 8000};

static u_int32_t g_mclk[] =
    { FTSSP_MCLK_24576K};

static u_int32_t g_bclk[] =
    { 400000};

static ftssp_clock_table_t g_clock_table = {
    .fs_table = g_fstable,
    .sspclk_table = g_mclk,
    .bclk_table = g_bclk,
    .number = ARRAY_SIZE(g_fstable),
};

static int ftssp010_adda300_open(ftssp010_pcm_ops_t * ops, snd_pcm_substream_t * substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct snd_pcm_hardware *hw = &runtime->hw;

    hw->formats &= (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE);
    hw->rates &= (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_32000 |
                  SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000);
    hw->rate_min = 8000;
    hw->rate_max = 48000;

    return 0;
}

static int ftssp010_adda300_close(ftssp010_pcm_ops_t * ops, snd_pcm_substream_t * stream)
{
    return 0;
}

static int ftssp010_adda300_prepare(ftssp010_pcm_ops_t * ops, snd_pcm_substream_t * stream,
                                    struct snd_pcm_runtime * run)
{
    struct ftssp010_adda300_t *chip = container_of(ops, struct ftssp010_adda300_t, ops);

    DEBUG_ADDA300_PRINT("%s: set fs %d\n", __FUNCTION__, run->rate);

    if (chip->sample_rate != run->rate) {
        switch (run->rate) {

        case 8000:
            adda300_set_fs(cs, ADDA300_FS_8K);
            break;
        case 16000:
            adda300_set_fs(cs, ADDA300_FS_16K);
            break;
        case 32000:
            adda300_set_fs(cs, ADDA300_FS_32K);
            break;
        case 44100:
            adda300_set_fs(cs, ADDA300_FS_44_1K);
            break;
        case 48000:
            adda300_set_fs(cs, ADDA300_FS_48K);
            DEBUG_ADDA300_PRINT("%s: set fs %d\n", __FUNCTION__, run->rate);
            break;
        default:
            DEBUG_ADDA300_PRINT("%s fails: wrong ADDA300_FS value\n", __FUNCTION__);
            return -EFAULT;
        }

        chip->sample_rate = run->rate;
    }

    return 0;
}

static int ftssp010_adda300_get_dma_parameters(ftssp010_pcm_ops_t * ops,
                                               snd_pcm_substream_t * substream, int *dma_width)
{
    snd_pcm_format_t sample_size = substream->runtime->format;

    DEBUG_ADDA300_PRINT("In %s: sample_size(ALSA define SYM) = %d\n", __FUNCTION__,
                        substream->runtime->format);

    switch (sample_size) {
    case SNDRV_PCM_FORMAT_S16_LE:
    case SNDRV_PCM_FORMAT_U16_LE:
        *dma_width = 2;
        break;
    case SNDRV_PCM_FORMAT_S32_LE:
    case SNDRV_PCM_FORMAT_U32_LE:
        *dma_width = 4;
        break;
    default:
        DEBUG_ADDA300_PRINT("%s fails: wrong sample size\n", __FUNCTION__);
        return -1;
    }

    return 0;
}

static int ftssp010_adda300_startup(ftssp010_pcm_ops_t * ops)
{
    return 0;
}

static struct ftssp010_pcm_ops ftssp010_adda300_ops = {
    .owner = THIS_MODULE,
    .startup = ftssp010_adda300_startup,
    .open = ftssp010_adda300_open,
    .close = ftssp010_adda300_close,
    .prepare = ftssp010_adda300_prepare,
    .get_dma_parameters = ftssp010_adda300_get_dma_parameters
};

static int fi2s_adda300_init(void)
{
    int ret = -ENOMEM;
    ftssp010_card_t *card = NULL;

    my_chip = kmalloc(sizeof(struct ftssp010_adda300_t), GFP_KERNEL);
    if (my_chip == NULL) {
        ret = -ENOMEM;
        goto exit_err1;
    }

    memset(my_chip, 0, sizeof(struct ftssp010_adda300_t));

    card = ftssp010_ssp_probe(7, THIS_MODULE);

    if (IS_ERR(card)) {
        ret = PTR_ERR(card);
        goto exit_err2;
    }

    card->codec_fmtbit = 16;
    card->clock_table = &g_clock_table;
    card->total_channels = 1;   //mono
    my_chip->card = card;
    my_chip->ops = ftssp010_adda300_ops;
    card->chip_ops = &my_chip->ops;
    card->client = my_chip;

#if SSP_SLAVE_MODE
    card->SSP_CR0 = I2S_FORMAT | SSP_FSDIST(1) | SSP_OPM_SLST | SSP_FSPO_LOW;   /* SSP as Slave */
#else
    card->SSP_CR0 = I2S_FORMAT | SSP_FSDIST(1) | SSP_OPM_MSST | SSP_FSPO_LOW;   /* SSP as Master */
#endif
    ftssp010_ssp_hw_init(card->vbase);

    adda300_init(cs);
#ifdef GM8312_SSP_TEST
    adda300_I2S_mux(cs, ADDA300_FROM_SSP);
#endif
    ret = snd_card_register(card->card);

    if (ret) {
        goto exit_err3;
    }

    DEBUG_ADDA300_PRINT("I2S probe ok in %s mode.\n",
                        (card->SSP_CR0 & SSP_MS_MASK) ? "Master" : "Slave");

    return 0;

  exit_err3:
    ftssp010_ssp_remove(my_chip->card);
  exit_err2:
    kfree(my_chip);
  exit_err1:
    DEBUG_ADDA300_PRINT("I2S probe fail in %s mode.",
                        (card->SSP_CR0 & SSP_MS_MASK) ? "Master" : "Slave");

    return ret;
}

static void fi2s_adda300_exit(void)
{
    ftssp010_ssp_remove(my_chip->card);
    kfree(my_chip);
    my_chip = NULL;
}

module_init(fi2s_adda300_init);
module_exit(fi2s_adda300_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("FTSSP010 - ADDA300");
MODULE_VERSION(FTSSP101_VERSION);
