
/***********************************************************************************
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
***********************************************************************/





/*******************************************************************************
	File:		aac_main_intfac.c.c

	Content:	aac encoder main core functions

*******************************************************************************/

#include "typedef.h"
#include "aac_main_intfac.h"
#include "out_bitstrm.h"

#include "perceptual_cfg.h"
#include "perceptual_main.h"
#include "quantizer_main.h"
//#include "perceptual_main.h"
//#include "channel_map.h"
#include "aac_table.h"

/********************************************************************************
*
* function name: AacDefault_InitConfig
* description:  gives reasonable default configuration
*
**********************************************************************************/
void AacDefault_InitConfig(GM_AAC_SPEC *config)
{
  /* default configurations */
  config->adtsUsed        = 1;
  config->nChannelsIn     = 2;
  config->nChannelsOut    = 2;
  config->bitRate         = 128000;
  config->bandWidth       = 0;
}

/********************************************************************************
*
* function name: AacEnc_Init
* description:  allocate and initialize a new encoder instance
* returns:      0 if success
*
**********************************************************************************/
int16_t  AacEnc_Init(  GM_AAC_ENCODER*      hAacEnc,        /* pointer to an encoder handle, initialized on return */
                     const  GM_AAC_SPEC     config   /* pre-initialized config struct */
                     )
{
 // int32_t i;
  int32_t error = 0;
  int16_t profile = 1;

  GM_PART_INFO *elInfo = NULL;

  if (hAacEnc==0) {
    error=1;
  }

  if (!error) {
    hAacEnc->config = config;
  }

  if (!error) {
    error = Init_ChannelPartInfo (config.nChannelsOut,
                             &hAacEnc->elInfo);
  }

  if (!error) {
    elInfo = &hAacEnc->elInfo;
  }

  if (!error) {
    /* use or not tns tool for long and short block */
	 int16_t tnsMask=3;

	/* init encoder psychoacoustic */
    error = Perceptual_MainInit(&hAacEnc->psyKernel,
                        config.sampleRate,
                        config.bitRate,
                        elInfo->nChannelsInEl,
                        tnsMask,
                        hAacEnc->config.bandWidth);
  }

 /* use or not adts header */
  if(!error) {
	  hAacEnc->qcOut.qcElement.adtsUsed = config.adtsUsed;
  }

  /* init encoder quantization */
  if (!error) {
    struct GM_QC_INFO qcInit;

    /*qcInit.channelMapping = &hAacEnc->channelMapping;*/
    qcInit.elInfo = &hAacEnc->elInfo;

    qcInit.maxBits = (int16_t) (MAXBITS_COEF*elInfo->nChannelsInEl);
    qcInit.bitRes = qcInit.maxBits;
    qcInit.averageBits = (int16_t) ((config.bitRate * FRAME_LEN_LONG) / config.sampleRate);

    qcInit.padding.paddingRest = config.sampleRate;

    qcInit.meanPe = (int16_t) ((10 * FRAME_LEN_LONG * hAacEnc->config.bandWidth) /
                                              (config.sampleRate>>1));

    qcInit.maxBitFac = (int16_t) ((100 * (MAXBITS_COEF-MINBITS_COEF)* elInfo->nChannelsInEl)/
                                                 (qcInit.averageBits?qcInit.averageBits:1));

    qcInit.bitrate = config.bitRate;

    error = QuantCode_Init(&hAacEnc->qcKernel, &qcInit);
  }

  /* init bitstream encoder */
  if (!error) {
    hAacEnc->bseInit.nChannels   = elInfo->nChannelsInEl;
    hAacEnc->bseInit.bitrate     = config.bitRate;
    hAacEnc->bseInit.sampleRate  = config.sampleRate;
    hAacEnc->bseInit.profile     = profile;
  }

  return error;
}

/********************************************************************************
*
* function name: AacEncEncode_Main
* description:  encode pcm to aac data core function
* returns:      0 if success
*
**********************************************************************************/
int16_t AacEncEncode_Main(GM_AAC_ENCODER *aacEnc,		/*!< an encoder handle */
                    int16_t *timeSignal,         /*!< BLOCKSIZE*nChannels audio samples, interleaved */
                    const uint8_t *ancBytes,     /*!< pointer to ancillary data bytes */
                    int16_t *numAncBytes,		/*!< number of ancillary Data Bytes */
                    uint8_t *outBytes,           /*!< pointer to output buffer (must be large MINBITS_COEF/8*MAX_CHAN bytes) */
                    uint32_t *numOutBytes         /*!< number of bytes in output buffer after processing */
                    )
{
  GM_PART_INFO *elInfo = &aacEnc->elInfo;
  int16_t globUsedBits;
  int16_t ancDataBytes, ancDataBytesLeft;

  ancDataBytes = ancDataBytesLeft = *numAncBytes;

  /* init output aac data buffer and length */
  aacEnc->hBitStream = BuildNew_BitBuffer(&aacEnc->bitStream, outBytes, *numOutBytes);

  /* psychoacoustic process */
  Perceptual_Main(aacEnc->config.nChannelsOut,
          elInfo,
          timeSignal,
          &aacEnc->psyKernel.psyData[elInfo->ChannelIndex[0]],
          &aacEnc->psyKernel.tnsData[elInfo->ChannelIndex[0]],
          &aacEnc->psyKernel.psyConfLong,
          &aacEnc->psyKernel.psyConfShort,
          &aacEnc->psyOut.psyOutChannel[elInfo->ChannelIndex[0]],
          &aacEnc->psyOut.psyOutElement,
          aacEnc->psyKernel.pScratchTns,
		  aacEnc->config.sampleRate);

  /* adjust bitrate and frame length */
  UpdateBitrate(&aacEnc->qcKernel,
                aacEnc->config.bitRate,
                aacEnc->config.sampleRate);

  /* quantization and coding process */
  QC_Main(&aacEnc->qcKernel,
         &aacEnc->qcKernel.elementBits,
         &aacEnc->qcKernel.adjThr.adjThrStateElem,
         &aacEnc->psyOut.psyOutChannel[elInfo->ChannelIndex[0]],
         &aacEnc->psyOut.psyOutElement,
         &aacEnc->qcOut.qcChannel[elInfo->ChannelIndex[0]],
         &aacEnc->qcOut.qcElement,
         elInfo->nChannelsInEl,
		 min(ancDataBytesLeft,ancDataBytes));

  ancDataBytesLeft = ancDataBytesLeft - ancDataBytes;

  globUsedBits = QC_WriteOutParam(&aacEnc->qcKernel,
                         &aacEnc->qcOut);

  /* write bitstream process */
  OutEncoder_BitData(aacEnc->id,
  							 aacEnc->hBitStream,
                 *elInfo,
                 &aacEnc->qcOut,
                 &aacEnc->psyOut,
                 &globUsedBits,
                 ancBytes,
				 aacEnc->psyKernel.sampleRateIdx);

  QC_WriteBitrese(&aacEnc->qcKernel,
               &aacEnc->qcOut);

  /* write out the bitstream */
  //*numOutBytes = Get_BitsCnt(aacEnc->hBitStream) >> 3;
  *numOutBytes = (aacEnc->hBitStream->cntBits) >> 3;


  return 0;
}


/********************************************************************************
*
* function name:AacEncClose
* description: deallocate an encoder instance
*
**********************************************************************************/
void AacEncClose (GM_AAC_ENCODER* hAacEnc)
{
  if (hAacEnc) {
    QC_ParmSetNULL(&hAacEnc->qcKernel);

    QC_ParamMem_Del(&hAacEnc->qcOut);

    Perceptual_DelMem(&hAacEnc->psyKernel);

    Perceptual_OutStrmSetNULL(&hAacEnc->psyOut);

    Del_BitBuf(&hAacEnc->hBitStream);

	if(hAacEnc->intbuf)
	{
		Aligmem_free(hAacEnc->intbuf);
		hAacEnc->intbuf = NULL;
	}
  }
}
