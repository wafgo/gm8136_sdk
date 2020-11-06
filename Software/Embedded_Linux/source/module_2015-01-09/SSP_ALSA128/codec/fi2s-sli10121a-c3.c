#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <ftssp010-ssp.h>

/*
 * The HDMI codec only provides 16bit, 48K sample rate.
 */


#define SSP_SLAVE_MODE				0

#undef PFX
#define PFX 	"SLI10121A-2"


struct ftssp010_sli10121a_t {
    ftssp010_pcm_ops_t ops;
    ftssp010_card_t *card;
    u_int32_t sample_rate;
};

static struct ftssp010_sli10121a_t *my_chip = NULL;

static u_int32_t g_fstable[] = { 48000 };

static u_int32_t g_mclk[] = { FTSSP_MCLK_12288K };

static u_int32_t g_bclk[] = {3072000 };

static ftssp_clock_table_t g_clock_table = {
    .fs_table = g_fstable,
    .sspclk_table = g_mclk,
    .bclk_table = g_bclk,
    .number = ARRAY_SIZE(g_fstable)
};

static int ftssp010_sli10121a_open(ftssp010_pcm_ops_t * ops, snd_pcm_substream_t * substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct snd_pcm_hardware *hw = &runtime->hw;

    hw->formats &= (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE);
    hw->rates &= SNDRV_PCM_RATE_48000;
    hw->rate_min = 48000;
    hw->rate_max = 48000;

    return 0;
}

static int ftssp010_sli10121a_close(ftssp010_pcm_ops_t * ops, snd_pcm_substream_t * stream)
{
    return 0;
}

static int ftssp010_sli10121a_prepare(ftssp010_pcm_ops_t * ops, snd_pcm_substream_t * stream,
                                      snd_pcm_runtime_t * run)
{
    extern void sli10121_reset_audio(void);

    sli10121_reset_audio();

    return 0;
}

static int ftssp010_sli10121a_get_dma_parameters(ftssp010_pcm_ops_t * ops,
                                                 snd_pcm_substream_t * substream, int *dma_width)
{
    snd_pcm_format_t sample_size = substream->runtime->format;

    switch (sample_size) {
    case SNDRV_PCM_FORMAT_S16_LE:
    case SNDRV_PCM_FORMAT_U16_LE:
        *dma_width = 2;
        break;
    default:
        printk("%s fails: wrong sample size\n", __FUNCTION__);
        return -1;
    }

    return 0;
}

static int ftssp010_sli10121a_startup(ftssp010_pcm_ops_t * ops)
{
    return 0;
}

static struct ftssp010_pcm_ops ftssp010_sli10121a_ops = {
    .owner = THIS_MODULE,
    .startup = ftssp010_sli10121a_startup,
    .open = ftssp010_sli10121a_open,
    .close = ftssp010_sli10121a_close,
    .prepare = ftssp010_sli10121a_prepare,
    .get_dma_parameters = ftssp010_sli10121a_get_dma_parameters
};

static int fi2s_sli10121a_init(void)
{
    int ret = -ENOMEM;
    ftssp010_card_t *card = NULL;

    my_chip = kmalloc(sizeof(struct ftssp010_sli10121a_t), GFP_KERNEL);
    if (my_chip == NULL) {
        ret = -ENOMEM;
        goto exit_err1;
    }

    memset(my_chip, 0, sizeof(struct ftssp010_sli10121a_t));
    card = ftssp010_ssp_probe(3, THIS_MODULE);  //ssp3
    if (IS_ERR(card)) {
        ret = PTR_ERR(card);
        goto exit_err2;
    }

    card->codec_fmtbit = 16;
    card->clock_table = &g_clock_table;
    card->total_channels = 1;
    my_chip->card = card;
    my_chip->ops = ftssp010_sli10121a_ops;
    card->chip_ops = &my_chip->ops;
    card->client = my_chip;
    //card->codec_version = FTSSP010_CODEC_NEW_ARCH;

#if SSP_SLAVE_MODE
    card->SSP_CR0 = I2S_FORMAT | SSP_FSDIST(1) | SSP_OPM_SLST | SSP_FSPO_LOW;   /* SSP as Slave */
#else
    card->SSP_CR0 = I2S_FORMAT | SSP_FSDIST(1) | SSP_OPM_MSST | SSP_FSPO_LOW;   /* SSP as Master */
#endif
    ftssp010_ssp_hw_init(card->vbase);

    ret = snd_card_register(card->card);

    if (ret) {
        goto exit_err3;
    }

    printk("I2S probe ok in %s mode.\n", (card->SSP_CR0 & SSP_MS_MASK) ? "Master" : "Slave");

    return 0;

  exit_err3:
    ftssp010_ssp_remove(my_chip->card);
  exit_err2:
    kfree(my_chip);
  exit_err1:
    printk("I2S probe fail in %s mode.", (card->SSP_CR0 & SSP_MS_MASK) ? "Master" : "Slave");

    return ret;
}

static void fi2s_sli10121a_exit(void)
{
    ftssp010_ssp_remove(my_chip->card);
    kfree(my_chip);
    my_chip = NULL;
}

module_init(fi2s_sli10121a_init);
module_exit(fi2s_sli10121a_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("GM Technology Corp.");
MODULE_VERSION(FTSSP101_VERSION);
