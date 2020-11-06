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



/*******************************************************************************
	File:		gm_aacTop.c

	Content:	aac encoder Top Layer interface functions

*******************************************************************************/
//#include <stdio.h>
#include "GMAAC.h"
#include "typedef.h"
#include "aac_main_intfac.h"
#include "aac_table.h"
#include "mem_manag.h"

//#define check_id

//#ifdef AAC_Enc_lib
//extern void Init_Table(int index);
//#endif


//void Init_Table(int index);
/**
* Init the audio codec module and return codec handle
* \param phCodec [OUT] Return the video codec handle
* \param vType	[IN] The codec type if the module support multi codec.
* \param pUserData	[IN] The init param. It is memory operator or alloced memory
* \retval ERR_NONE Succeeded.
*/
uint32_t GMAACEncInit(AAC_HANDLE * phCodec)
{
	GM_AAC_ENCODER*hAacEnc;
	//GM_AAC_SPEC config;
	int error;
	int interMem;

	interMem = 0;
	error = 0;


	/* init the aac encoder handle */
	hAacEnc = (GM_AAC_ENCODER*)Alignm_mem(sizeof(GM_AAC_ENCODER), 32);
	if(NULL == hAacEnc)
	{
		error = 1;
	}

	if(!error)
	{
		/* init the aac encoder intra memory */
		hAacEnc->intbuf = (short *)Alignm_mem(AACENC_BLOCKSIZE*MAX_CHAN*sizeof(short), 32);
		if(NULL == hAacEnc->intbuf)
		{
			error = 1;
		}
	}

	if (!error) {
		/* init the aac encoder psychoacoustic */
		error = (Perceptual_AllocMem(&hAacEnc->psyKernel, MAX_CHAN) ||
			Perceptual_OutStrmSet0(&hAacEnc->psyOut));
	}

	if (!error) {
		/* init the aac encoder quantization elements */
		error = QC_ParamMem_Init(&hAacEnc->qcOut,MAX_CHAN);
	}

	if (!error) {
		/* init the aac encoder quantization state */
		error = QC_Mem0(&hAacEnc->qcKernel);
	}

	/* uninit the aac encoder if error is nozero */
	if(error)
	{
		AacEncClose(hAacEnc);
		if(hAacEnc)
		{
			Aligmem_free(hAacEnc);
			hAacEnc = NULL;
		}
		*phCodec = NULL;
		return ERR_OUTOF_MEMORY;
	}

	/* init the aac encoder default parameter  */
	if(hAacEnc->initOK == 0)
	{
		 GM_AAC_SPEC config;
		 config.adtsUsed = 1;
		 config.bitRate = 128000;
		 config.nChannelsIn = 2;
		 config.nChannelsOut = 2;
		 config.sampleRate = 44100;
		 config.bandWidth = 20000;

		 AacEnc_Init(hAacEnc, config);
	}

	*phCodec = hAacEnc;

	return ERR_NONE;
}

/**
* Set input audio data.
* param: hCodec [IN]] The Codec Handle which was created by Init function.
*        pInput [IN] The input buffer param.
*        pOutBuffer [OUT] The output buffer info.
*
* retval ERR_NONE Succeeded.
*/
uint32_t GMAACEncSetInputData(AAC_HANDLE hCodec, GM_AUDIO_BUF * pInput)
{
	GM_AAC_ENCODER *hAacEnc;
	int  length;

	if(NULL == hCodec || NULL == pInput || NULL == pInput->Buffer)
	{
		return ERR_INVALID_ARG;
	}

	hAacEnc = (GM_AAC_ENCODER *)hCodec;

	/* init input pcm buffer and length*/
	hAacEnc->inbuf = (short *)pInput->Buffer;
	hAacEnc->inlen = pInput->Length / sizeof(short);
	hAacEnc->uselength = 0;

	hAacEnc->encbuf = hAacEnc->inbuf;
	hAacEnc->enclen = hAacEnc->inlen;

	/* rebuild intra pcm buffer and length*/
	if(hAacEnc->intlen)
	{
		length = min(hAacEnc->config.nChannelsIn*AACENC_BLOCKSIZE - hAacEnc->intlen, hAacEnc->inlen);
		memcpy(hAacEnc->intbuf + hAacEnc->intlen, hAacEnc->inbuf, length*sizeof(short));

		hAacEnc->encbuf = hAacEnc->intbuf;
		hAacEnc->enclen = hAacEnc->intlen + length;

		hAacEnc->inbuf += length;
		hAacEnc->inlen -= length;
	}

	return ERR_NONE;
}

/**
* Get the outut audio data
* param: hCodec [IN]] The Codec Handle which was created by Init function.
*        pOutBuffer [OUT] The output audio data
*        pOutInfo [OUT] The dec module filled audio format and used the input size.
*						 pOutInfo->InputUsed is total used the input size.
* \retval  ERR_NONE Succeeded.
*			ERR_INPUT_BUFFER_SMALL. The input was finished or the input data was not enought.
*/
uint32_t GMAACEncGetOutputData(AAC_HANDLE hCodec, GM_AUDIO_BUF * pOutput, GM_AUDIO_OUTSPEC * pOutInfo)
{
	GM_AAC_ENCODER* hAacEnc = (GM_AAC_ENCODER*)hCodec;
	int16_t numAncDataBytes=0;
	int32_t  inbuflen;
	int  length;//,i;//ret,

	if(NULL == hAacEnc)
		return ERR_INVALID_ARG;

	 inbuflen = AACENC_BLOCKSIZE*hAacEnc->config.nChannelsIn;

	 /* check the input pcm buffer and length*/
	 if(NULL == hAacEnc->encbuf || hAacEnc->enclen < inbuflen)
	 {
		length = hAacEnc->enclen;
		if(hAacEnc->intlen == 0)
		{
			memcpy(hAacEnc->intbuf, hAacEnc->encbuf, length*sizeof(short));
			hAacEnc->uselength += length*sizeof(short);
		}
		else
		{
			hAacEnc->uselength += (length - hAacEnc->intlen)*sizeof(short);
		}

		hAacEnc->intlen = length;

		pOutput->Length = 0;

		if(pOutInfo)
			pOutInfo->InputUsed = hAacEnc->uselength;

		return ERR_INPUT_BUFFER_SMALL;
	 }

	 /* check the output aac buffer and length*/
	 if(NULL == pOutput || NULL == pOutput->Buffer || pOutput->Length < (6144/8)*hAacEnc->config.nChannelsOut/(sizeof(int32_t)))
		 return ERR_OUTPUT_BUFFER_SMALL;

	 /* aac encoder core function */
	 AacEncEncode_Main( hAacEnc,
			(int16_t*)hAacEnc->encbuf,
			NULL,
			&numAncDataBytes,
			pOutput->Buffer,
			&pOutput->Length);

	 /* update the input pcm buffer and length*/
	 if(hAacEnc->intlen)
	 {
		length = inbuflen - hAacEnc->intlen;
		hAacEnc->encbuf = hAacEnc->inbuf;
		hAacEnc->enclen = hAacEnc->inlen;
		hAacEnc->uselength += length*sizeof(short);
		hAacEnc->intlen = 0;
	 }
	 else
	 {
		 hAacEnc->encbuf = hAacEnc->encbuf + inbuflen;
		 hAacEnc->enclen = hAacEnc->enclen - inbuflen;
		 hAacEnc->uselength += inbuflen*sizeof(short);
	 }

	 /* update the output aac information */
	if(pOutInfo)
	{
		pOutInfo->Format.Channels = hAacEnc->config.nChannelsOut;
		pOutInfo->Format.SampleRate = hAacEnc->config.sampleRate;
		pOutInfo->Format.SampleBits = 16;
		pOutInfo->InputUsed = hAacEnc->uselength;
	}

	 return ERR_NONE;
}

/**
* Uninit the Codec.
* \param hCodec [IN]] The Codec Handle which was created by Init function.
* \retval ERR_NONE Succeeded.
*/

uint32_t GMAACEncDestory(AAC_HANDLE hCodec)
{
	GM_AAC_ENCODER* hAacEnc = (GM_AAC_ENCODER*)hCodec;

	if(NULL != hAacEnc)
	{
		/* close the aac encoder */
		AacEncClose(hAacEnc);

		/* free the aac encoder handle*/
		Aligmem_free(hAacEnc);
		hAacEnc = NULL;
	}

	return ERR_NONE;
}



#ifdef check_id
int check_id(void)
{
	char buf[128];
	FILE *pp;

	if( (pp = popen("devmem 0x90c00000", "r")) == NULL )
	{
	}

	while(fgets(buf, sizeof buf, pp))	{
	}
	buf[6] = 0;
	pclose(pp);

	if(  (strcmp(buf,"0x8129")==0)||(strcmp(buf,"0x0813")==0)  )
	        return 0;
	else
	        return 1;	        
}
#endif


/**
 * Get audio codec API interface
 * \param pEncHandle [out] Return the AAC Encoder handle.
 * \retval ERR_OK Succeeded.
 */
 /*
int32_t GetAACEncAPI(GM_AUDIO_TOPAPI * pDecHandle)
{
	if(pDecHandle == NULL)
		return ERR_INVALID_ARG;

	pDecHandle->Init = GMAACEncInit;
	pDecHandle->SetInputData = GMAACEncSetInputData;
	pDecHandle->GetOutputData = GMAACEncGetOutputData;
	pDecHandle->SetParament = GMAACEncSetParam;
	pDecHandle->GetParament = GMAACEncGetParam;
	pDecHandle->Destory = GMAACEncDestory;

	return ERR_NONE;
}
*/
