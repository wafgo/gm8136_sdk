/*
***********************************************************************************
** FARADAY(MTD)/GRAIN CONFIDENTIAL
** Copyright FARADAY(MTD)/GRAIN Corporation All Rights Reserved.
**
** The source code contained or described herein and all documents related to
** the source code (Material) are owned by FARADAY(MTD)/GRAIN Corporation Title to the
** Material remains with FARADAY(MTD)/GRAIN Corporation. The Material contains trade
** secrets and proprietary and confidential information of FARADAY(MTD)/GRAIN. The
** Material is protected by worldwide copyright and trade secret laws and treaty
** provisions. No part of the Material may be used, copied, reproduced, modified,
** published, uploaded, posted, transmitted, distributed, or disclosed in any way
** without FARADAY(MTD)/GRAIN prior express written permission.
**
** No license under any patent, copyright, trade secret or other intellectual
** property right is granted to or conferred upon you by disclosure or delivery
** of the Materials, either expressly, by implication, inducement, estoppel or
** otherwise. Any license under such intellectual property rights must be express
** and approved by FARADAY(MTD)/GRAIN in writing.
**
**
***********************************************************************/
#define DEBUG

//#define MultiChannel_AAC
//#define  AAC_Enc_lib
//#define Timer
//#include <stdio.h>
//#include <stdint.h>
//#include <stdlib.h>

//#include "wavreader.h"

#include "basic_op.h"
#if 0
//#include <cyg/hal/hal_io.h>
//#include <cyg/hal/fie702x.h>
//#include <cyg/kernel/kapi.h>
//#include <cyg/io/io.h>
//#include <cyg/io/pmu/fie702x_pmu.h>
//#include "in_output_data_table.h"
#endif

#include "aac_table.h"		//for init_table subroutine use
#include "GMAAC.h"		//"called subroutine at gm_aacTop.c"
#include "typedefs.h"
#include "AACAudio.h"		//define "GM_AUDIO_OUTSPEC structure"
#include "audio_coprocessor.h"  //initial FA410 called subroutine


//#include "aac_enc_api.h"  	//define "MaxMultiChannel" and called subroutine at main
#include "gmaac_enc_api.h"


#include "aac_main_intfac.h"	//define "GM_AAC_ENCODER structure"



//static int format, bitsPerSample;
//static GM_AAC_INSPEC params = { 0 };
//static AAC_HANDLE aachandle = 0;
//static int inputSize;
//static uint8_t* inputBuf;
//static int16_t* convertBuf;
//static FILE *out;

//static void *wav;


//GMAAC_INFORMATION gm_aac;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef DEBUG
/*
int encode_setting[32][4] = {
        	{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},
        	{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},
        	{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},
        	{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},
        	{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},
        	{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},
        	{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},
        	{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1},{8000, 14500, 1, 1}
        	};
*/
//unsigned char outputBuf[MaxMultiChannel][8192];


#endif
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
* Set the param for special target.
* \param hCodec [IN]] The Codec Handle which was created by Init function.
* \param uParamID [IN] The param ID.
* \param pData [IN] The param value depend on the ID>
* \retval ERR_NONE Succeeded.
*/


uint32_t GMAACEncSetParam(AAC_HANDLE hCodec, int32_t uParamID, AAC_ENCODE_PARAMETERS *pData, int MultiChannel)
{
	GM_AAC_SPEC config;
	//GM_AAC_INSPEC* pAAC_param;
	GM_AUDIO_SPEC *pWAV_Format;
	GM_AAC_ENCODER* hAacEnc = (GM_AAC_ENCODER*)hCodec;
	int ret, i, bitrate, tmp;//, j, k;
	int SampleRateIdx;

	//j = GMID_AAC_ENCPARAM;
	//k = GMID_AUDIO_FORMAT;

	if( (NULL == hAacEnc)||(NULL == pData) )
		return ERR_INVALID_ARG;
#if 0
        if( (pData->sampleRate != 8000) && (pData->sampleRate != 16000) )
		return ERR_INVALID_ARG;
#endif

        if( (pData->bitRate < ENC_MIN_BITRATE) || (pData->bitRate > ENC_MAX_BITRATE) )
        	return ERR_INVALID_ARG;

#ifdef check_id
	hAacEnc->checkid1 = check_id();
#endif
	hAacEnc->checkid2 = hAacEnc->checkid1 + 1;
	switch(uParamID)
	{
	case GMID_AAC_ENCPARAM:  /* init aac encoder parameter*/
		AacDefault_InitConfig(&config);

		config.adtsUsed = 	1;
		config.bitRate = 	pData->bitRate;
		config.nChannelsIn = 	pData->nChannels;
		config.nChannelsOut = 	pData->nChannels;
		config.sampleRate = 	pData->sampleRate;

		/* check the channel */
		if(config.nChannelsIn< 1  || config.nChannelsIn > MAX_CHAN  ||
             		config.nChannelsOut < 1 || config.nChannelsOut > MAX_CHAN || config.nChannelsIn < config.nChannelsOut)
			 return ERR_AUDIO_UNSCHANNEL;

		/* check the samplerate */
		ret = -1;
		for(i = 0; i < NUM_SAMPLE_RATES; i++)
		{
			if(config.sampleRate == sampRateTab[i])
			{
				ret = 0;
				break;
			}
		}
		if(ret < 0)
			return ERR_AUDIO_UNSSAMPLERATE;

		SampleRateIdx = i;

		tmp = 441;
		if(config.sampleRate%8000 == 0)
			tmp =480;

		/* check the bitrate */
		if(((config.bitRate!=0) && (config.bitRate/config.nChannelsOut < 4000)) ||
           (config.bitRate/config.nChannelsOut > 160000) ||
		   (config.bitRate > config.sampleRate*6*config.nChannelsOut))
		{
			config.bitRate = 640*config.sampleRate/tmp*config.nChannelsOut;

			if(config.bitRate/config.nChannelsOut < 4000)
				config.bitRate = 4000 * config.nChannelsOut;
			else if(config.bitRate > config.sampleRate*6*config.nChannelsOut)
				config.bitRate = config.sampleRate*6*config.nChannelsOut;
			else if(config.bitRate/config.nChannelsOut > 160000)
				config.bitRate = config.nChannelsOut*160000;
		}

		/* check the bandwidth */
		bitrate = config.bitRate / config.nChannelsOut;
		bitrate = bitrate * tmp / config.sampleRate;

		for (i = 0; rates[i]; i++)
		{
			if (rates[i] >= bitrate)
				break;
		}

		config.bandWidth = BandwithCoefTab[i][SampleRateIdx];

		/* init aac encoder core */
		ret = AacEnc_Init(hAacEnc, config);
		if(ret)
			return ERR_AUDIO_UNSFEATURE;
		break;

	case GMID_AUDIO_FORMAT:	/* init pcm channel and samplerate*/
		AacDefault_InitConfig(&config);

		if(pData == NULL)
			return ERR_INVALID_ARG;

		pWAV_Format = (GM_AUDIO_SPEC*)pData;
		config.adtsUsed = 1;
		config.nChannelsIn = pWAV_Format->Channels;
		config.nChannelsOut = pWAV_Format->Channels;
		config.sampleRate = pWAV_Format->SampleRate;

		/* check the channel */
		if(config.nChannelsIn< 1  || config.nChannelsIn > MAX_CHAN  ||
             config.nChannelsOut < 1 || config.nChannelsOut > MAX_CHAN || config.nChannelsIn < config.nChannelsOut)
			 return ERR_AUDIO_UNSCHANNEL;

		/* check the samplebits */
		if(pWAV_Format->SampleBits != 16)
		{
			return ERR_AUDIO_UNSFEATURE;
		}

		/* check the samplerate */
		ret = -1;

		for(i = 0; i < NUM_SAMPLE_RATES; i++)
		{
			if(config.sampleRate == sampRateTab[i])
			{
				ret = 0;
				break;
			}
		}

		if(ret < 0)
			return ERR_AUDIO_UNSSAMPLERATE;

		SampleRateIdx = i;

		/* update the bitrates */
		tmp = 441;
		if(config.sampleRate%8000 == 0)
			tmp =480;

		config.bitRate = 640*config.sampleRate/tmp*config.nChannelsOut;

		if(config.bitRate/config.nChannelsOut < 4000)
			config.bitRate = 4000 * config.nChannelsOut;
		else if(config.bitRate > config.sampleRate*6*config.nChannelsOut)
			config.bitRate = config.sampleRate*6*config.nChannelsOut;
		else if(config.bitRate/config.nChannelsOut > 160000)
			config.bitRate = config.nChannelsOut*160000;

		/* check the bandwidth */
		bitrate = config.bitRate / config.nChannelsOut;
		bitrate = bitrate * tmp / config.sampleRate;

		for (i = 0; rates[i]; i++)
		{
			if (rates[i] >= bitrate)
				break;
		}

		config.bandWidth = BandwithCoefTab[i][SampleRateIdx];

		/* init aac encoder core */
		ret = AacEnc_Init(hAacEnc, config);
		if(ret)
			return ERR_AUDIO_UNSFEATURE;
		break;

	default:
		return ERR_WRONG_PARAM_ID;
	}

	hAacEnc->checkid3 = hAacEnc->checkid2 - 1;
	hAacEnc->checkid4 = hAacEnc->checkid3 + 1;

//#ifdef MultiChannel_AAC
//	if(MultiChannel<1)
//	   Init_Table(hAacEnc->checkid4);
//#endif

	return ERR_NONE;
}


/**
* Get the param for special target.
* \param hCodec [IN]] The Codec Handle which was created by Init function.
* \param uParamID [IN] The param ID.
* \param pData [IN] The param value depend on the ID>
* \retval ERR_NONE Succeeded.
*/
uint32_t GMAACEncGetParam(AAC_HANDLE hCodec, int32_t uParamID, AAC_PTR pData)
{
	return ERR_NONE;
}


/*****************************************************************************
*
* function name: AAC_Create
* description: Init input Parameter of AAC Encodedr
*     1.set adtsUsed , bitRate, nChannelsIn, nChannelsOut, sampleRate ....
*     2.allocates memory
*******************************************************************************/
int AAC_Create(int id, GMAAC_INFORMATION *gm_aac, AUDIO_CODEC_SETTING *codec_setting, int MultiChannel)
{
	int i=0;
	AAC_HANDLE aachandle[MaxMultiChannel]= {0};//MaxMultiChannel=12

#if 0
	GM_AAC_INSPEC params = { 0 };

	bitrate = AAC_BitRate;		//speech 14.5
#endif
	if(!codec_setting)
	    return 1;

	//codec_setting->InputSampleNumber = 1024;

	if(MultiChannel > MaxMultiChannel)
	{
		 printk("Unable to set encoding parameters\n");
	   return 1;
	}

	for( i=0; i<MultiChannel; i++)
	   GMAACEncInit(&aachandle[i]);
#if 0
	//Set AAC Encoder input Parameter
	params.sampleRate = sampleRate;
	params.bitRate = bitrate;
	params.nChannels = channels;
	params.adtsUsed = 1;
#endif

	for(i=0; i< MultiChannel;i++)
	{
	    if (GMAACEncSetParam(aachandle[i], GMID_AAC_ENCPARAM, &codec_setting->aac_enc_paras[i],i) != ERR_NONE)
	    {
	    	printk("Unable to set encoding parameters\n");
	    	return 1;
	    }
	}
	for(i=0; i< MultiChannel;i++)
	{
		((GM_AAC_ENCODER *) aachandle[i])->id = id;
		gm_aac->private_region[i] = aachandle[i];
	}

	gm_aac->MultiChannel	= MultiChannel;
	//gm_aac->sampleRate	= params[i].sampleRate;
	//gm_aac->bitRate = params[i].bitRate;
	//gm_aac->nChannels = params[i].nChannels;
	gm_aac->id = id;
	//gm_aac->adtsUsed = params[i].adtsUsed;

	//init_audio_coprocessor();

	strcpy(gm_aac->version, "1.10");
    return 0;
}
EXPORT_SYMBOL(AAC_Create);





/*****************************************************************************
*
* function name: AAC_Enc
*
* description: Main loop of AAC Encodedr
*
*******************************************************************************/
GM_AUDIO_BUF input[MaxMultiChannel], output[MaxMultiChannel];
GM_AUDIO_OUTSPEC output_info_T[MaxMultiChannel];
int AAC_Enc(int16_t convertBuf[][2048], uint8_t outbuf[][2048], int len[],  GMAAC_INFORMATION *gm_aac)
{
	//int ch;
	//int inputSize;
	int i;//,j,read;
#if 0   //GM8287 audio driver limited the local variable size to 1024 bytes
	GM_AUDIO_BUF input[MaxMultiChannel], output[MaxMultiChannel];
	GM_AUDIO_OUTSPEC output_info_T[MaxMultiChannel];
#endif
    memset(input, 0, sizeof(GM_AUDIO_BUF) * MaxMultiChannel);
    memset(output, 0, sizeof(GM_AUDIO_BUF) * MaxMultiChannel);
    memset(output_info_T, 0, sizeof(GM_AUDIO_OUTSPEC) * MaxMultiChannel);

	//for(i=0; i< gm_aac->MultiChannel ;i++)

	for(i=0; i< gm_aac->MultiChannel ;i++)    //
	{
     		input[i].Buffer = (uint8_t*) convertBuf[i];
     		input[i].Length = 1024*2*2;
     		GMAACEncSetInputData(gm_aac->private_region[i], &input[i]);

     		output[i].Buffer = outbuf[i];
     		output[i].Length = 2048;

		if (GMAACEncGetOutputData(gm_aac->private_region[i], &output[i], &output_info_T[i]) != ERR_NONE)
		{
          		printk("Unable to encode frame\n");
          		return 1;
      		}
      		len[i] = output[i].Length;
	}

	return 0;
}
EXPORT_SYMBOL(AAC_Enc);

int AAC_Destory(GMAAC_INFORMATION *gm_aac)
{
	int i;

	//gm_aac->sampleRate	= 0;
	//gm_aac->bitRate = 0;
	//gm_aac->nChannels = 0;
	//gm_aac->adtsUsed = 0;

	for(i=0; i< gm_aac->MultiChannel ;i++)
	{
		 GMAACEncDestory(gm_aac->private_region[i]);
	}
	gm_aac->MultiChannel=0;

	return ERR_NONE;

}
EXPORT_SYMBOL(AAC_Destory);

void Init_Table(int index)
{

   int lfsr,len,i;
   int *tab;
   if (index == 1) {
	   lfsr = 0x10101010;
	 } else {
	   lfsr = 0x12321678;
	 }

   tab = (int *)twidTab512;
   len = sizeof(twidTab512)/4;
   for (i=0; i<len; i++) {
    int tmp;//, tmp1;
    tmp = tab[i];
    tmp ^= lfsr;
    tab[i] = tmp;
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
   }

   tab = (int *)twidTab64;
   len = sizeof(twidTab64)/4;
   for (i=0; i<len; i++) {
    int tmp;//, tmp1;
    tmp = tab[i];
    tmp ^= lfsr;
    tab[i] = tmp;
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
   }

   tab = (int *)ShortWindowSine;
   len = sizeof(ShortWindowSine)/4;
   for (i=0; i<len; i++) {
    int tmp;//, tmp1;
    tmp = tab[i];
    tmp ^= lfsr;
    tab[i] = tmp;
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
   }

   tab = (int *)LongWindowKBD;
   len = sizeof(LongWindowKBD)/4;
   for (i=0; i<len; i++) {
    int tmp;//, tmp1;
    tmp = tab[i];
    tmp ^= lfsr;
    tab[i] = tmp;
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
   }

   tab = (int *)formfac_sqrttable;
   len = sizeof(formfac_sqrttable)/4;
   for (i=0; i<len; i++) {
    int tmp;//, tmp1;
    tmp = tab[i];
    tmp ^= lfsr;
    tab[i] = tmp;
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
   }

   tab = (int *)mTab_3_4;
   len = sizeof(mTab_3_4)/4;
   for (i=0; i<len; i++) {
    int tmp;//, tmp1;
    tmp = tab[i];
    tmp ^= lfsr;
    tab[i] = tmp;
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
   }

   tab = (int *)mTab_4_3;
   len = sizeof(mTab_4_3)/4;
   for (i=0; i<len; i++) {
    int tmp;//, tmp1;
    tmp = tab[i];
    tmp ^= lfsr;
    tab[i] = tmp;
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
   }

   tab = (int *)huff_ctabscf;
   len = sizeof(huff_ctabscf)/4;
   for (i=0; i<len; i++) {
    int tmp;//, tmp1;
    tmp = tab[i];
    tmp ^= lfsr;
    tab[i] = tmp;
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
   }


   tab = (int *)cossintab_1024;
   len = sizeof(cossintab_1024)/4;
   for (i=0; i<len; i++) {
    int tmp;//, tmp1;
    tmp = tab[i];
    tmp ^= lfsr;
    tab[i] = tmp;
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
   }


   tab = (int *)cossintab_128;
   len = sizeof(cossintab_128)/4;
   for (i=0; i<len; i++) {
    int tmp;//, tmp1;
    tmp = tab[i];
    tmp ^= lfsr;
    tab[i] = tmp;
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
   }


}
