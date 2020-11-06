#include <linux/stddef.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include "common.h"
#include "structs.h"
#include "decoder.h"
#include "mp4.h"
#include "syntax.h"
#include "error.h"
#include "output.h"
#include "filtbank.h"
#include "drc.h"
#include "../ssp_dma/ssp_dma.h"
#include <mach/fmem.h>
#include "../../include/frammap/frammap_if.h"

#if defined(MEMCPY_DMA)
static u32 buf_paddr = 0;
#endif
static u32 buf_sz = 0;

//Shawn 2004.12.9
GMAACDecHandle FAADAPI faacDecOpen(void)//call once only !!!!!
{
    int i;
    GMAACDecHandle hDecoder = NULL;

	if ((hDecoder = (GMAACDecHandle)kmalloc(sizeof(faacDecStruct), GFP_KERNEL)) == NULL)
        return NULL;

    memset(hDecoder, 0, sizeof(faacDecStruct));

    hDecoder->config.outputFormat  = FAAD_FMT_16BIT;
    hDecoder->config.defObjectType = MAIN;
    hDecoder->config.defSampleRate = 44100; /* Default: 44.1kHz */
    hDecoder->config.downMatrix = 0;
    hDecoder->adts_header_present = 0;
    hDecoder->adif_header_present = 0;
    hDecoder->frameLength = 1024;

    hDecoder->frame = 0;
    hDecoder->sample_buffer = NULL;

    for (i = 0; i < MAX_CHAN; i++)
    {
        hDecoder->window_shape_prev[i] = 0;
        hDecoder->time_out[i] = NULL;
        hDecoder->fb_intermed[i] = NULL;

#ifdef LTP_DEC
        hDecoder->ltp_lag[i] = 0;
        hDecoder->lt_pred_stat[i] = NULL;
#endif
    }

#ifdef DRC
    hDecoder->drc = drc_init(REAL_CONST(1.0), REAL_CONST(1.0));
#endif

    return hDecoder;
}

faacDecConfigurationPtr FAADAPI faacDecGetCurrentConfiguration(GMAACDecHandle hDecoder)//call once only !!!!!
{
    if (hDecoder)
    {
        faacDecConfigurationPtr config = &(hDecoder->config);

        return config;
    }

    return NULL;
}

//Shawn 2004.12.9
char FAADAPI faacDecSetConfiguration(GMAACDecHandle hDecoder, faacDecConfigurationPtr config)//call once only !!!!!
{
    if (hDecoder && config)
    {
		hDecoder->config.defObjectType = config->defObjectType;

        /* samplerate: anything but 0 should be possible */
        if (config->defSampleRate == 0)
            return 0;
        hDecoder->config.defSampleRate = config->defSampleRate;

        /* check output format */
        if ((config->outputFormat < 1) || (config->outputFormat > 9))
            return 0;
        hDecoder->config.outputFormat = config->outputFormat;

        if (config->downMatrix > 1)
            hDecoder->config.downMatrix = config->downMatrix;

        /* OK */
        return 1;
    }

    return 0;
}

//Shawn 2004.12.9
int FAADAPI faacDecInit(GMAACDecHandle hDecoder, char *buffer, unsigned int buffer_size, unsigned int *samplerate, char *channels)
{
    if ((hDecoder == NULL) || (samplerate == NULL) || (channels == NULL))
        return -1;

    hDecoder->sf_index = get_sr_index(hDecoder->config.defSampleRate);
    hDecoder->object_type = hDecoder->config.defObjectType;
    *samplerate = get_sample_rate(hDecoder->sf_index);
    *channels = 1;
    hDecoder->adts_header_present = 1;
    hDecoder->channelConfiguration = *channels;

    /* must be done before frameLength is divided by 2 for LD */
        hDecoder->fb = filter_bank_init(hDecoder->frameLength);

    return 0;
}

//Shawn 2004.10.12
void FAADAPI faacDecClose(GMAACDecHandle hDecoder)//call once only !!!!!
{
    int i;

    if (hDecoder == NULL)
        return;

    for (i = 0; i < MAX_CHAN; i++)
    {
#ifdef LTP_DEC
        if (hDecoder->lt_pred_stat[i]) faad_free(hDecoder->lt_pred_stat[i]);
#endif
    }

        filter_bank_end(hDecoder->fb);

#ifdef DRC_DEC
    drc_end(hDecoder->drc);
#endif

#if defined(MEMCPY_DMA)
    if (hDecoder->sample_buffer) {
        if (DMA_ALIGN_CHECK(buf_sz))
            fmem_free_ex(buf_sz, hDecoder->sample_buffer, buf_paddr);
        else
            dma_free_writecombine(NULL, buf_sz, hDecoder->sample_buffer, buf_paddr);
    }
#else
    if (hDecoder->sample_buffer) kfree(hDecoder->sample_buffer);
#endif

    if (hDecoder) kfree(hDecoder);
}

//Shawn 2004.10.7
static void create_channel_config(GMAACDecHandle hDecoder, GMAACDecInfo *hInfo)
{
    hInfo->num_front_channels = 0;
    hInfo->num_side_channels = 0;
    hInfo->num_back_channels = 0;
    hInfo->num_lfe_channels = 0;

    //memset(hInfo->channel_position, 0, MAX_CHAN);// no neeed to set to 0??????????? Shawn 2004.10.7

    if (hDecoder->downMatrix)
    {
        hInfo->num_front_channels = 2;
        hInfo->channel_position[0] = FRONT_CHANNEL_LEFT;
        hInfo->channel_position[1] = FRONT_CHANNEL_RIGHT;
        return;
    }

    /* check if there is a PCE */
    if (hDecoder->pce_set)
    {
        unsigned char i, chpos = 0;
        unsigned char chdir, back_center = 0;

        hInfo->num_front_channels = hDecoder->pce.num_front_channels;
        hInfo->num_side_channels = hDecoder->pce.num_side_channels;
        hInfo->num_back_channels = hDecoder->pce.num_back_channels;
        hInfo->num_lfe_channels = hDecoder->pce.num_lfe_channels;

        chdir = hInfo->num_front_channels;
        if (chdir & 1)
        {
            hInfo->channel_position[chpos++] = FRONT_CHANNEL_CENTER;
            chdir--;
        }
        for (i = 0; i < chdir; i += 2)
        {
            hInfo->channel_position[chpos++] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[chpos++] = FRONT_CHANNEL_RIGHT;
        }

        for (i = 0; i < hInfo->num_side_channels; i += 2)
        {
            hInfo->channel_position[chpos++] = SIDE_CHANNEL_LEFT;
            hInfo->channel_position[chpos++] = SIDE_CHANNEL_RIGHT;
        }

        chdir = hInfo->num_back_channels;
        if (chdir & 1)
        {
            back_center = 1;
            chdir--;
        }
        for (i = 0; i < chdir; i += 2)
        {
            hInfo->channel_position[chpos++] = BACK_CHANNEL_LEFT;
            hInfo->channel_position[chpos++] = BACK_CHANNEL_RIGHT;
        }
        if (back_center)
        {
            hInfo->channel_position[chpos++] = BACK_CHANNEL_CENTER;
        }

        for (i = 0; i < hInfo->num_lfe_channels; i++)
        {
            hInfo->channel_position[chpos++] = LFE_CHANNEL;
        }

    }
	else
	{
        switch (hDecoder->channelConfiguration)
        {
        case 1:
            hInfo->num_front_channels = 1;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            break;
        case 2:
            hInfo->num_front_channels = 2;
            hInfo->channel_position[0] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[1] = FRONT_CHANNEL_RIGHT;
            break;
        case 3:
            hInfo->num_front_channels = 3;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            break;
        case 4:
            hInfo->num_front_channels = 3;
            hInfo->num_back_channels = 1;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            hInfo->channel_position[3] = BACK_CHANNEL_CENTER;
            break;
        case 5:
            hInfo->num_front_channels = 3;
            hInfo->num_back_channels = 2;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            hInfo->channel_position[3] = BACK_CHANNEL_LEFT;
            hInfo->channel_position[4] = BACK_CHANNEL_RIGHT;
            break;
        case 6:
            hInfo->num_front_channels = 3;
            hInfo->num_back_channels = 2;
            hInfo->num_lfe_channels = 1;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            hInfo->channel_position[3] = BACK_CHANNEL_LEFT;
            hInfo->channel_position[4] = BACK_CHANNEL_RIGHT;
            hInfo->channel_position[5] = LFE_CHANNEL;
            break;
        case 7:
            hInfo->num_front_channels = 3;
            hInfo->num_side_channels = 2;
            hInfo->num_back_channels = 2;
            hInfo->num_lfe_channels = 1;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            hInfo->channel_position[3] = SIDE_CHANNEL_LEFT;
            hInfo->channel_position[4] = SIDE_CHANNEL_RIGHT;
            hInfo->channel_position[5] = BACK_CHANNEL_LEFT;
            hInfo->channel_position[6] = BACK_CHANNEL_RIGHT;
            hInfo->channel_position[7] = LFE_CHANNEL;
            break;
        default: /* channelConfiguration == 0 || channelConfiguration > 7 */
            {
                unsigned char i;
                unsigned char ch = hDecoder->fr_channels - hDecoder->has_lfe;
                if (ch & 1) /* there's either a center front or a center back channel */
                {
                    unsigned char ch1 = (ch-1)/2;
                    if (hDecoder->first_syn_ele == ID_SCE)
                    {
                        hInfo->num_front_channels = ch1 + 1;
                        hInfo->num_back_channels = ch1;
                        hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
                        for (i = 1; i <= ch1; i+=2)
                        {
                            hInfo->channel_position[i] = FRONT_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = FRONT_CHANNEL_RIGHT;
                        }
                        for (i = ch1+1; i < ch; i+=2)
                        {
                            hInfo->channel_position[i] = BACK_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = BACK_CHANNEL_RIGHT;
                        }
                    } else {
                        hInfo->num_front_channels = ch1;
                        hInfo->num_back_channels = ch1 + 1;
                        for (i = 0; i < ch1; i+=2)
                        {
                            hInfo->channel_position[i] = FRONT_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = FRONT_CHANNEL_RIGHT;
                        }
                        for (i = ch1; i < ch-1; i+=2)
                        {
                            hInfo->channel_position[i] = BACK_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = BACK_CHANNEL_RIGHT;
                        }
                        hInfo->channel_position[ch-1] = BACK_CHANNEL_CENTER;
                    }
                } else {
                    unsigned char ch1 = (ch)/2;
                    hInfo->num_front_channels = ch1;
                    hInfo->num_back_channels = ch1;
                    if (ch1 & 1)
                    {
                        hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
                        for (i = 1; i <= ch1; i+=2)
                        {
                            hInfo->channel_position[i] = FRONT_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = FRONT_CHANNEL_RIGHT;
                        }
                        for (i = ch1+1; i < ch-1; i+=2)
                        {
                            hInfo->channel_position[i] = BACK_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = BACK_CHANNEL_RIGHT;
                        }
                        hInfo->channel_position[ch-1] = BACK_CHANNEL_CENTER;
                    } else {
                        for (i = 0; i < ch1; i+=2)
                        {
                            hInfo->channel_position[i] = FRONT_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = FRONT_CHANNEL_RIGHT;
                        }
                        for (i = ch1; i < ch; i+=2)
                        {
                            hInfo->channel_position[i] = BACK_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = BACK_CHANNEL_RIGHT;
                        }
                    }
                }
                hInfo->num_lfe_channels = hDecoder->has_lfe;
                for (i = ch; i < hDecoder->fr_channels; i++)
                {
                    hInfo->channel_position[i] = LFE_CHANNEL;
                }
            }
            break;
        }
    }
}

//Shawn 2004.8.23
void* GMAACDecode(GMAACDecHandle hDecoder,
                            GMAACDecInfo *hInfo,
                            char *buffer, unsigned int buffer_size)
{
    int channels = 0;
    int output_channels = 0;
    bitfile ld;
    unsigned int bitsconsumed;
    int frame_len;
    void *sample_buffer;
    int samplerate;
    char stride = 0;
    const char str[] = { sizeof(short), sizeof(int), sizeof(int),
        sizeof(float), sizeof(double), sizeof(short), sizeof(short),
        sizeof(short), sizeof(short), 0, 0, 0
    };

    /* safety checks */
    if ((hDecoder == NULL) || (hInfo == NULL) || (buffer == NULL))
    {
        printk("%s: hDecoder(%p), hInfo(%p), buffer(%p)\n", __func__, hDecoder, hInfo, buffer);
        return NULL;
    }

    frame_len = hDecoder->frameLength;


    memset(hInfo, 0, sizeof(GMAACDecInfo));
    memset(hDecoder->internal_channel, 0, MAX_CHAN*sizeof(hDecoder->internal_channel[0]));

    /* initialize the bitstream */
    faad_initbits(&ld, buffer, buffer_size);
    {
        adts_header adts;

        adts.old_format = hDecoder->config.useOldADTSFormat;

        if ((hInfo->error = adts_frame(&adts, &ld)) > 0)
        {
            bitsconsumed = faad_get_processed_bits(&ld);
            hInfo->bytesconsumed = bit2byte(bitsconsumed);
            goto error;
        }

		hDecoder->adts_header_present = 1;
		hDecoder->sf_index = adts.sf_index;
		hDecoder->object_type = adts.profile + 1;
		samplerate = get_sample_rate(hDecoder->sf_index);
                hDecoder->channelConfiguration = (adts.channel_configuration > 6) ?
                2 : adts.channel_configuration;
#ifdef SBR_DEC
		/* implicit signalling */
		if (samplerate <= 24000 && !(hDecoder->config.dontUpSampleImplicitSBR))
		{
			samplerate *= 2;
			hDecoder->forceUpSampling = 1;
		}
#endif

		/* must be done before frameLength is divided by 2 for LD */
#ifdef SSR_DEC
		if (hDecoder->object_type == SSR)
			hDecoder->fb = ssr_filter_bank_init(hDecoder->frameLength/SSR_BANDS);
		else
#endif

#ifdef LD_DEC
    if (hDecoder->object_type == LD)
        hDecoder->frameLength >>= 1;
#endif
        /* MPEG2 does byte_alignment() here,
         * but ADTS header is always multiple of 8 bits in MPEG2
         * so not needed to actually do it.
         */
    }

    /* decode the complete bitstream */
    //raw_data_block(hDecoder, hInfo, &ld, &hDecoder->pce, hDecoder->drc);
	raw_data_block(hDecoder, hInfo, &ld, &hDecoder->pce, NULL);

    channels = hDecoder->fr_channels;

    if (hInfo->error > 0)
        goto error;

    /* safety check */
    if (channels == 0 || channels > MAX_CHAN)
    {
        /* invalid number of channels */
        hInfo->error = 12;
        goto error;
    }

    /* no more bit reading after this */
    bitsconsumed = faad_get_processed_bits(&ld);
    hInfo->bytesconsumed = bit2byte(bitsconsumed);

    if (ld.error)
    {
        hInfo->error = 14;
        goto error;
    }

    faad_endbits(&ld);


    if (!hDecoder->adts_header_present && !hDecoder->adif_header_present)
    {
        if (channels != hDecoder->channelConfiguration)
            hDecoder->channelConfiguration = channels;

        if (channels == 8) /* 7.1 */
            hDecoder->channelConfiguration = 7;
        if (channels == 7) /* not a standard channelConfiguration */
            hDecoder->channelConfiguration = 0;
    }

    if ((channels == 5 || channels == 6) && hDecoder->config.downMatrix)
    {
        hDecoder->downMatrix = 1;
        output_channels = 2;
    } else {
        output_channels = channels;
    }

    /* Make a channel configuration based on either a PCE or a channelConfiguration */
    create_channel_config(hDecoder, hInfo);

    /* number of samples in this frame */
    hInfo->samples = frame_len*output_channels;
    stride = str[hDecoder->config.outputFormat - 1];
    buf_sz = hInfo->samples * stride;
    /* number of channels in this frame */
    hInfo->channels = output_channels;
    /* samplerate */
    hInfo->samplerate = get_sample_rate(hDecoder->sf_index);
    /* object type */
    hInfo->object_type = hDecoder->object_type;
    /* sbr */
    hInfo->sbr = NO_SBR;
    /* header type */
    hInfo->header_type = RAW;
    if (hDecoder->adif_header_present)
        hInfo->header_type = ADIF;
    if (hDecoder->adts_header_present)
        hInfo->header_type = ADTS;

    if (channels == 0)
    {
        /* invalid number of channels */
        hInfo->error = 12;
        goto error;
    }

    /* allocate the buffer for the final samples */
    if ((hDecoder->sample_buffer == NULL) ||
        (hDecoder->alloced_channels != output_channels))
    {
        if (hDecoder->sample_buffer) {
#if defined(MEMCPY_DMA)
            if (DMA_ALIGN_CHECK(buf_sz))
                fmem_free_ex(buf_sz, hDecoder->sample_buffer, buf_paddr);
            else
                dma_free_writecombine(NULL, buf_sz, hDecoder->sample_buffer, buf_paddr);
#else
            kfree(hDecoder->sample_buffer);
#endif
        }

        hDecoder->sample_buffer = NULL;
#if defined(MEMCPY_DMA)
        if (DMA_ALIGN_CHECK(buf_sz))
            hDecoder->sample_buffer = (void *)fmem_alloc_ex(buf_sz, &buf_paddr, PAGE_SHARED, DDR_ID_SYSTEM);
        else
            hDecoder->sample_buffer = dma_alloc_writecombine(NULL, buf_sz, &buf_paddr, GFP_KERNEL);
#else
        hDecoder->sample_buffer = kmalloc(frame_len*output_channels*stride, GFP_KERNEL);
#endif
        hDecoder->alloced_channels = output_channels;
    }

    sample_buffer = hDecoder->sample_buffer;

    sample_buffer = output_to_PCM(hDecoder, hDecoder->time_out, sample_buffer,
        output_channels, frame_len, hDecoder->config.outputFormat);


    hDecoder->postSeekResetFlag = 0;

    hDecoder->frame++;

        if (hDecoder->frame <= 1)
            hInfo->samples = 0;


    return sample_buffer;

error:

    faad_endbits(&ld);


    return NULL;
}
EXPORT_SYMBOL(GMAACDecode);
