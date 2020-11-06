/******************************************************************************/
/*                                                                            */
/*	FILE : main.c     				                  						  */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
/*============================================================================*/
/*		Include FIle                                                          */
/*============================================================================*/
#include <linux/slab.h>
#include "faad.h"
#include "gmaac_dec_api.h"

extern int eq_type;			// EQ type


int adts_sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};


int GM_AAC_Dec_Init(GMAACDecHandle *hDecoder, aac_buffer *b, GMAACDecInfo **frameInfo)
{
    //int tagsize;
    unsigned long samplerate;
    unsigned char channels;
    int object_type = LC;
    int old_format = 0;//no old format

    faacDecConfigurationPtr config;

    int bread;

	eq_type = -1;
	memset(b, 0, sizeof(aac_buffer));


	*frameInfo = (GMAACDecInfo *)kmalloc(sizeof(GMAACDecInfo), GFP_KERNEL);
	memset(*frameInfo,0,sizeof(GMAACDecInfo));

	//hDecoder = *hhDecoder;


    if (!(b->buffer = (unsigned char*)kmalloc(FAAD_MIN_STREAMSIZE*MAX_CHAN, GFP_KERNEL)))
    {
        printk("Memory allocation error\n");
        return 0;
    }

    memset(b->buffer, 0, FAAD_MIN_STREAMSIZE*MAX_CHAN);

	*hDecoder = faacDecOpen();

    /* Set the default object type and samplerate */
    /* This is useful for RAW AAC files */
    config = faacDecGetCurrentConfiguration(*hDecoder);
    config->defObjectType = object_type;
    config->outputFormat = FAAD_FMT_16BIT;//faacdCMD->outputFormat;
    config->downMatrix = 0;//faacdCMD->downMatrix;
    config->useOldADTSFormat = old_format;
    config->dontUpSampleImplicitSBR = 1;

	faacDecSetConfiguration(*hDecoder, config);

    if ((bread = faacDecInit(*hDecoder, b->buffer, b->bytes_into_buffer, &samplerate, &channels)) < 0)
		return 1;

    return 0;
}
EXPORT_SYMBOL(GM_AAC_Dec_Init);


void GM_AAC_Dec_Destory(GMAACDecHandle hDecoder, aac_buffer *b, GMAACDecInfo **frameInfo)
{
    if (*frameInfo) {
        kfree(*frameInfo);
        *frameInfo = NULL;
    }
    faacDecClose(hDecoder);
    if (b->buffer)
        kfree(b->buffer);
}
EXPORT_SYMBOL(GM_AAC_Dec_Destory);
