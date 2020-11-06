#include <linux/init.h>
#include <linux/module.h>
#include <ftssp010-ssp.h>


#undef PFX
#define PFX 					"NVP1918"

#define SSP_SLAVE_MODE  1
#define DEBUG_NVP1918   0

//debug helper
#if DEBUG_NVP1918
#define DEBUG_NVP1918_PRINT(FMT, ARGS...)    printk(FMT, ##ARGS)
#else
#define DEBUG_NVP1918_PRINT(FMT, ARGS...)
#endif

struct ftssp010_nvp1918_t {
    ftssp010_pcm_ops_t ops;
    ftssp010_card_t *card;
    u_int32_t sample_rate;
};

static struct ftssp010_nvp1918_t *my_chip = NULL;

static u_int32_t g_fstable[] =
    { 8000 , 16000};

static u_int32_t g_mclk[] =
    {FTSSP_MCLK_24576K, FTSSP_MCLK_24576K};

static u_int32_t g_bclk[] =
    { 2048000, 4096000};

static ftssp_clock_table_t g_clock_table = {
    .fs_table = g_fstable,
    .sspclk_table = g_mclk,
    .bclk_table = g_bclk,
    .number = ARRAY_SIZE(g_fstable),
};

static int ftssp010_nvp1918_open(ftssp010_pcm_ops_t * ops, snd_pcm_substream_t * substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct snd_pcm_hardware *hw = &runtime->hw;

    hw->formats &= (SNDRV_PCM_FORMAT_S8|SNDRV_PCM_FORMAT_U8|SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE);
    hw->rates &= (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 );
    hw->rate_min = 8000;
    hw->rate_max = 16000;

    return 0;
}

static int ftssp010_nvp1918_close(ftssp010_pcm_ops_t * ops, snd_pcm_substream_t * stream)
{
    return 0;
}

static int ftssp010_nvp1918_prepare(ftssp010_pcm_ops_t * ops, snd_pcm_substream_t * stream,
                                    struct snd_pcm_runtime * run)
{
    struct ftssp010_nvp1918_t *chip = container_of(ops, struct ftssp010_nvp1918_t, ops);

    DEBUG_NVP1918_PRINT("%s: set fs %d\n", __FUNCTION__, run->rate);

    chip->sample_rate = run->rate;

    return 0;
}

static int ftssp010_nvp1918_get_dma_parameters(ftssp010_pcm_ops_t * ops,
                                               snd_pcm_substream_t * substream, int *dma_width)
{
    snd_pcm_format_t sample_size = substream->runtime->format;

    DEBUG_NVP1918_PRINT("In %s: sample_size(ALSA define SYM) = %d\n", __FUNCTION__,
                        substream->runtime->format);

    switch (sample_size) {
        case SNDRV_PCM_FORMAT_S8:
        case SNDRV_PCM_FORMAT_U8:
        case SNDRV_PCM_FORMAT_S16_LE:
        case SNDRV_PCM_FORMAT_U16_LE:
            *dma_width = 4;
            break;
        default:
            return -1;
    }

    return 0;
}

static int ftssp010_nvp1918_startup(ftssp010_pcm_ops_t * ops)
{
    return 0;
}

static struct ftssp010_pcm_ops ftssp010_nvp1918_ops = {
    .owner = THIS_MODULE,
    .startup = ftssp010_nvp1918_startup,
    .open = ftssp010_nvp1918_open,
    .close = ftssp010_nvp1918_close,
    .prepare = ftssp010_nvp1918_prepare,
    .get_dma_parameters = ftssp010_nvp1918_get_dma_parameters
};

static int fi2s_nvp1918_init(void)
{
    int ret = -ENOMEM;
    ftssp010_card_t *card = NULL;

    my_chip = kmalloc(sizeof(struct ftssp010_nvp1918_t), GFP_KERNEL);
    if (my_chip == NULL) {
        ret = -ENOMEM;
        goto exit_err1;
    }

    memset(my_chip, 0, sizeof(struct ftssp010_nvp1918_t));
    card = ftssp010_ssp_probe(1, THIS_MODULE);
    if (IS_ERR(card)) {
        ret = PTR_ERR(card);
        goto exit_err2;
    }

    card->codec_fmtbit = 16;
    card->clock_table = &g_clock_table;
    card->total_channels = 16;
    my_chip->card = card;
    my_chip->ops = ftssp010_nvp1918_ops;
    card->chip_ops = &my_chip->ops;
    card->client = my_chip;

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

    DEBUG_NVP1918_PRINT("I2S probe ok in %s mode.\n",
                        (card->SSP_CR0 & SSP_MS_MASK) ? "Master" : "Slave");

    return 0;

  exit_err3:
    ftssp010_ssp_remove(my_chip->card);
  exit_err2:
    kfree(my_chip);
  exit_err1:
    DEBUG_NVP1918_PRINT("I2S probe fail in %s mode.",
                        (card->SSP_CR0 & SSP_MS_MASK) ? "Master" : "Slave");

    return ret;
}

static void fi2s_nvp1918_exit(void)
{
    ftssp010_ssp_remove(my_chip->card);
    kfree(my_chip);
    my_chip = NULL;
}

module_init(fi2s_nvp1918_init);
module_exit(fi2s_nvp1918_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("FTSSP010 - NVP1918");
MODULE_VERSION(FTSSP101_VERSION);
