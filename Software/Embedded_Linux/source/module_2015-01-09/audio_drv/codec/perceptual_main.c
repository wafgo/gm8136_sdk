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
	File:		Perceptual_main.c

	Content:	Psychoacoustic major functions

*******************************************************************************/

#include "typedef.h"
#include "basic_op.h"
#include "oper_32b.h"
#include "perceptual_const.h"
#include "perceptual_tool.h"
#include "enc_mdct.h"
//#include "spreading.h"
#include "threshold_tool.h"
//#include "band_nrg.h"
#include "perceptual_cfg.h"
#include "perceptual_data.h"
//#include "ms_stereo.h"
//#include "interface.h"
#include "perceptual_main.h"
//#include "grp_data.h"
#include "tns_func.h"
#include "mem_manag.h"



/*                                          long       start       short       stop */
static int16_t blockType2windowShape[] = {KBD_WINDOW,SINE_WINDOW,SINE_WINDOW,KBD_WINDOW};

/*
  function
*/
static int16_t Perceptual_AdvLngBlk(GM_PSY_PART_DATA* psyData,
                               GM_TNS_PART_DATA* tnsData,
                               GM_PSY_CFG_LONGBLK *hPsyConfLong,
                               GM_PSY_OUT_CHANNEL* psyOutChannel,
                               int32_t *pScratchTns,
                               const GM_TNS_PART_DATA *tnsData2,
                               const int16_t ch);

static int16_t Perceptual_AdvLngBlkMS (GM_PSY_PART_DATA  psyData[MAX_CHAN],
                                  const GM_PSY_CFG_LONGBLK *hPsyConfLong);

static int16_t Perceptual_AdvShortBlk(GM_PSY_PART_DATA* psyData,
                                GM_TNS_PART_DATA* tnsData,
                                const GM_PSY_CFG_SHORTBLK *hPsyConfShort,
                                GM_PSY_OUT_CHANNEL* psyOutChannel,
                                int32_t *pScratchTns,
                                const GM_TNS_PART_DATA *tnsData2,
                                const int16_t ch);

static int16_t Perceptual_AdvShortBlkMS (GM_PSY_PART_DATA  psyData[MAX_CHAN],
                                   const GM_PSY_CFG_SHORTBLK *hPsyConfShort);




void UpdateOutParameter(int32_t                 *mdctSpectrum,
                    const int16_t            mdctScale,
                    GM_SFB_THRSHD          *sfbThreshold,
                    GM_SFB_ENGY             *sfbEnergy,
                    GM_SFB_ENGY             *sfbSpreadedEnergy,
                    const GM_SFB_ENGY_SUM    sfbEnergySumLR,
                    const GM_SFB_ENGY_SUM    sfbEnergySumMS,
                    const int16_t            windowSequence,
                    const int16_t            windowShape,
                    const int16_t            sfbCnt,
                    const int16_t           *sfbOffset,
                    const int16_t            maxSfbPerGroup,
                    const int16_t           *groupedSfbMinSnr,
                    const int16_t            noOfGroups,
                    const int16_t           *groupLen,
                    GM_PSY_OUT_CHANNEL        *psyOutCh);

/*******************************************************************************
	Content:	Declaration MS stereo processing structure and functions
*******************************************************************************/
void MidSideStereo(int32_t       *sfbEnergyLeft,
                        int32_t       *sfbEnergyRight,
                        const int32_t *sfbEnergyMid,
                        const int32_t *sfbEnergySide,
                        int32_t       *mdctSpectrumLeft,
                        int32_t       *mdctSpectrumRight,
                        int32_t       *sfbThresholdLeft,
                        int32_t       *sfbThresholdRight,
                        int32_t       *sfbSpreadedEnLeft,
                        int32_t       *sfbSpreadedEnRight,
                        int16_t       *msDigest,
                        int16_t       *msMask,
                        const int16_t  sfbCnt,
                        const int16_t  sfbPerGroup,
                        const int16_t  maxSfbPerGroup,
                        const int16_t *sfbOffset);


/*******************************************************************************
	Content:	Short block grouping function
*******************************************************************************/
void ShortBlkGroup(int32_t        *mdctSpectrum,
               int32_t        *tmpSpectrum,
               GM_SFB_THRSHD *sfbThreshold,
               GM_SFB_ENGY    *sfbEnergy,
               GM_SFB_ENGY    *sfbEnergyMS,
               GM_SFB_ENGY    *sfbSpreadedEnergy,
               const int16_t   sfbCnt,
               const int16_t  *sfbOffset,
               const int16_t  *sfbMinSnr,
               int16_t        *groupedSfbOffset,
               int16_t        *maxSfbPerGroup,
               int16_t        *groupedSfbMinSnr,
               const int16_t   noOfGroups,
               const int16_t  *groupLen);

/*******************************************************************************
	Content:	Band/Line energy calculations functions
*******************************************************************************/
void SfbMdct_Energy(const int32_t *mdctSpectrum,
                    const int16_t *bandOffset,
                    const int16_t  numBands,
                    int32_t       *bandEnergy,
                    int32_t       *bandEnergySum);


void SfbMdct_MSEnergy(const int32_t *mdctSpectrumLeft,
                      const int32_t *mdctSpectrumRight,
                      const int16_t *bandOffset,
                      const int16_t  numBands,
                      int32_t       *bandEnergyMid,
                      int32_t       *bandEnergyMidSum,
                      int32_t       *bandEnergySide,
                      int32_t       *bandEnergySideSum);


/*******************************************************************************
	Content:	Spreading of energy functions
*******************************************************************************/

void FindMax_EngerSpreading(const int16_t pbCnt,
                  const int16_t *maskLowFactor,
                  const int16_t *maskHighFactor,
                  int32_t       *pbSpreadedEnergy);






/*****************************************************************************
*
* function name: Perceptual_AllocMem
* description:  allocates memory for psychoacoustic
* returns:      an error code
* input:        pointer to a psych handle
*
*****************************************************************************/
int16_t Perceptual_AllocMem(GM_PSY_BLK *hPsy, int32_t nChan)
{
  int16_t i;
  int32_t *mdctSpectrum;
  int32_t *scratchTNS;
  int16_t *mdctDelayBuffer;

  mdctSpectrum = (int32_t *)Alignm_mem(nChan * FRAME_LEN_LONG * sizeof(int32_t), 32);
  if(NULL == mdctSpectrum)
	  return 1;

  scratchTNS = (int32_t *)Alignm_mem(nChan * FRAME_LEN_LONG * sizeof(int32_t), 32);
  if(NULL == scratchTNS)
  {
	  return 1;
  }

  mdctDelayBuffer = (int16_t *)Alignm_mem(nChan * BLOCK_SWITCHING_OFFSET * sizeof(int16_t), 32);
  if(NULL == mdctDelayBuffer)
  {
	  return 1;
  }

  for (i=0; i<nChan; i++){
    hPsy->psyData[i].mdctDelayBuffer = mdctDelayBuffer + i*BLOCK_SWITCHING_OFFSET;
    hPsy->psyData[i].mdctSpectrum = mdctSpectrum + i*FRAME_LEN_LONG;
  }

  hPsy->pScratchTns = scratchTNS;

  return 0;
}


/*****************************************************************************
*
* function name: Perceptual_DelMem
* description:  allocates memory for psychoacoustic
* returns:      an error code
*
*****************************************************************************/
int16_t Perceptual_DelMem(GM_PSY_BLK  *hPsy)
{
  int32_t nch;

  if(hPsy)
  {
	if(hPsy->psyData[0].mdctDelayBuffer)
		Aligmem_free(hPsy->psyData[0].mdctDelayBuffer);

    if(hPsy->psyData[0].mdctSpectrum)
		Aligmem_free(hPsy->psyData[0].mdctSpectrum);

    for (nch=0; nch<MAX_CHAN; nch++){
	  hPsy->psyData[nch].mdctDelayBuffer = NULL;
	  hPsy->psyData[nch].mdctSpectrum = NULL;
	}

	if(hPsy->pScratchTns)
	{
		Aligmem_free(hPsy->pScratchTns);
		hPsy->pScratchTns = NULL;
	}
  }

  return 0;
}


/*****************************************************************************
*
* function name: Perceptual_OutStrmSet0
* description:  allocates memory for psyOut struc
* returns:      an error code
* input:        pointer to a psych handle
*
*****************************************************************************/
int16_t Perceptual_OutStrmSet0(GM_PERCEP_OUT *hPsyOut)
{
  memset(hPsyOut, 0, sizeof(GM_PERCEP_OUT));
  /*
    alloc some more stuff, tbd
  */
  return 0;
}

/*****************************************************************************
*
* function name: Perceptual_OutStrmSetNULL
* description:  allocates memory for psychoacoustic
* returns:      an error code
*
*****************************************************************************/
int16_t Perceptual_OutStrmSetNULL(GM_PERCEP_OUT *hPsyOut)
{
  hPsyOut=NULL;
  return 0;
}


/*****************************************************************************
*
* function name: Psy_MainInit
* description:  initializes psychoacoustic
* returns:      an error code
*
*****************************************************************************/

int16_t Perceptual_MainInit(GM_PSY_BLK *hPsy,
                   int32_t sampleRate,
                   int32_t bitRate,
                   int16_t channels,
                   int16_t tnsMask,
                   int16_t bandwidth)
{
  int16_t ch, err;
  int32_t channelBitRate = bitRate/channels;

  err = Perce_InitCfgLngBlk(channelBitRate,
                                 sampleRate,
                                 bandwidth,
                                 &(hPsy->psyConfLong));

  if (!err) {
      hPsy->sampleRateIdx = hPsy->psyConfLong.sampRateIdx;
	  err = Init_TnsCfgLongBlk(bitRate, sampleRate, channels,
                                   &hPsy->psyConfLong.tnsConf, &hPsy->psyConfLong, tnsMask&2);
  }

  if (!err)
    err = Psy_InitCfgShortBlk(channelBitRate,
                                    sampleRate,
                                    bandwidth,
                                    &hPsy->psyConfShort);
  if (!err) {
    err = Init_TnsCfgShortBlk(bitRate, sampleRate, channels,
                                    &hPsy->psyConfShort.tnsConf, &hPsy->psyConfShort, tnsMask&1);
  }

  if (!err)
    for(ch=0;ch < channels;ch++){

      BlockSwitch_Init(&hPsy->psyData[ch].blockSwitchingControl,
                         bitRate, channels);

      InitPreEchoControl(hPsy->psyData[ch].sfbThresholdnm1,
                         hPsy->psyConfLong.sfbCnt,
                         hPsy->psyConfLong.sfbThresholdQuiet);
      hPsy->psyData[ch].mdctScalenm1 = 0;
    }

	return(err);
}

/*****************************************************************************
*
* function name: Perceptual_Main
* description:  psychoacoustic main function
* returns:      an error code
*
*    This function assumes that enough input data is in the modulo buffer.
*
*****************************************************************************/

int16_t Perceptual_Main(int16_t                   nChannels,
               GM_PART_INFO            *elemInfo,
               int16_t                  *timeSignal,
               GM_PSY_PART_DATA                 psyData[MAX_CHAN],
               GM_TNS_PART_DATA                 tnsData[MAX_CHAN],
               GM_PSY_CFG_LONGBLK  *hPsyConfLong,
               GM_PSY_CFG_SHORTBLK *hPsyConfShort,
               GM_PSY_OUT_CHANNEL          psyOutChannel[MAX_CHAN],
               GM_PERCEP_OUT_PART         *psyOutElement,
               int32_t                  *pScratchTns,
			   int32_t				   sampleRate)
{
  int16_t maxSfbPerGroup[MAX_CHAN];
  int16_t mdctScalingArray[MAX_CHAN];

  int16_t ch;   /* counts through channels          */
  int16_t sfb;  /* counts through scalefactor bands */
  int16_t line; /* counts through lines             */
  int16_t channels;
  int16_t maxScale;

  channels = elemInfo->nChannelsInEl;
  maxScale = 0;

  /* block switching */
  for(ch = 0; ch < channels; ch++) {
    DataBlockSwitching(&psyData[ch].blockSwitchingControl,
                   timeSignal+elemInfo->ChannelIndex[ch],
				   sampleRate,
                   nChannels);
  }

  /* synch left and right block type */
  UpdateBlock(&psyData[0].blockSwitchingControl,
                     &psyData[1].blockSwitchingControl,
                     channels);

  /* transform
     and get maxScale (max mdctScaling) for all channels */
  for(ch=0; ch<channels; ch++) {
    MDCT_Trans(psyData[ch].mdctDelayBuffer,
                   timeSignal+elemInfo->ChannelIndex[ch],
                   nChannels,
                   psyData[ch].mdctSpectrum,
                   &(mdctScalingArray[ch]),
                   psyData[ch].blockSwitchingControl.windowSequence);
    maxScale = max(maxScale, mdctScalingArray[ch]);
  }

  /* common scaling for all channels */
  for (ch=0; ch<channels; ch++) {
    int16_t scaleDiff = maxScale - mdctScalingArray[ch];

    if (scaleDiff > 0) {
      int32_t *Spectrum = psyData[ch].mdctSpectrum;
	  for(line=0; line<FRAME_LEN_LONG; line++) {
        *Spectrum = (*Spectrum) >> scaleDiff;
		Spectrum++;
      }
    }
    psyData[ch].mdctScale = maxScale;
  }

  for (ch=0; ch<channels; ch++) {

    if(psyData[ch].blockSwitchingControl.windowSequence != SHORT_WINDOW) {
      /* update long block parameter */
	  Perceptual_AdvLngBlk(&psyData[ch],
                       &tnsData[ch],
                       hPsyConfLong,
                       &psyOutChannel[ch],
                       pScratchTns,
                       &tnsData[1 - ch],
                       ch);

      /* determine maxSfb */
      for (sfb=hPsyConfLong->sfbCnt-1; sfb>=0; sfb--) {
        for (line=hPsyConfLong->sfbOffset[sfb+1] - 1; line>=hPsyConfLong->sfbOffset[sfb]; line--) {

          if (psyData[ch].mdctSpectrum[line] != 0) break;
        }
        if (line >= hPsyConfLong->sfbOffset[sfb]) break;
      }
      maxSfbPerGroup[ch] = sfb + 1;

      /* Calc bandwise energies for mid and side channel
         Do it only if 2 channels exist */

      if (ch == 1)
        Perceptual_AdvLngBlkMS(psyData, hPsyConfLong);
    }
    else {
      Perceptual_AdvShortBlk(&psyData[ch],
                        &tnsData[ch],
                        hPsyConfShort,
                        &psyOutChannel[ch],
                        pScratchTns,
                        &tnsData[1 - ch],
                        ch);

      /* Calc bandwise energies for mid and side channel
         Do it only if 2 channels exist */

      if (ch == 1)
        Perceptual_AdvShortBlkMS (psyData, hPsyConfShort);
    }
  }

  /* group short data */
  for(ch=0; ch<channels; ch++) {

    if (psyData[ch].blockSwitchingControl.windowSequence == SHORT_WINDOW) {
      ShortBlkGroup(psyData[ch].mdctSpectrum,
                     pScratchTns,
                     &psyData[ch].sfbThreshold,
                     &psyData[ch].sfbEnergy,
                     &psyData[ch].sfbEnergyMS,
                     &psyData[ch].sfbSpreadedEnergy,
                     hPsyConfShort->sfbCnt,
                     hPsyConfShort->sfbOffset,
                     hPsyConfShort->sfbMinSnr,
                     psyOutElement->groupedSfbOffset[ch],
                     &maxSfbPerGroup[ch],
                     psyOutElement->groupedSfbMinSnr[ch],
                     psyData[ch].blockSwitchingControl.noOfGroups,
                     psyData[ch].blockSwitchingControl.groupLen);
    }
  }


#if (MAX_CHAN>1)
  /*
    stereo Processing
  */
  if (channels == 2) {
    psyOutElement->toolsInfo.msDigest = MS_NONE;
    maxSfbPerGroup[0] = maxSfbPerGroup[1] = max(maxSfbPerGroup[0], maxSfbPerGroup[1]);


    if (psyData[0].blockSwitchingControl.windowSequence != SHORT_WINDOW)
      MidSideStereo(psyData[0].sfbEnergy.sfbLong,
                         psyData[1].sfbEnergy.sfbLong,
                         psyData[0].sfbEnergyMS.sfbLong,
                         psyData[1].sfbEnergyMS.sfbLong,
                         psyData[0].mdctSpectrum,
                         psyData[1].mdctSpectrum,
                         psyData[0].sfbThreshold.sfbLong,
                         psyData[1].sfbThreshold.sfbLong,
                         psyData[0].sfbSpreadedEnergy.sfbLong,
                         psyData[1].sfbSpreadedEnergy.sfbLong,
                         (int16_t*)&psyOutElement->toolsInfo.msDigest,
                         (int16_t*)psyOutElement->toolsInfo.msMask,
                         hPsyConfLong->sfbCnt,
                         hPsyConfLong->sfbCnt,
                         maxSfbPerGroup[0],
                         (const int16_t*)hPsyConfLong->sfbOffset);
      else
        MidSideStereo(psyData[0].sfbEnergy.sfbLong,
                           psyData[1].sfbEnergy.sfbLong,
                           psyData[0].sfbEnergyMS.sfbLong,
                           psyData[1].sfbEnergyMS.sfbLong,
                           psyData[0].mdctSpectrum,
                           psyData[1].mdctSpectrum,
                           psyData[0].sfbThreshold.sfbLong,
                           psyData[1].sfbThreshold.sfbLong,
                           psyData[0].sfbSpreadedEnergy.sfbLong,
                           psyData[1].sfbSpreadedEnergy.sfbLong,
                           (int16_t*)&psyOutElement->toolsInfo.msDigest,
                           (int16_t*)psyOutElement->toolsInfo.msMask,
                           psyData[0].blockSwitchingControl.noOfGroups*hPsyConfShort->sfbCnt,
                           hPsyConfShort->sfbCnt,
                           maxSfbPerGroup[0],
                           (const int16_t*)psyOutElement->groupedSfbOffset[0]);
  }

#endif /* (MAX_CHAN>1) */

  /*
    build output
  */
  for(ch=0;ch<channels;ch++) {

    if (psyData[ch].blockSwitchingControl.windowSequence != SHORT_WINDOW)
      UpdateOutParameter(psyData[ch].mdctSpectrum,
                     psyData[ch].mdctScale,
                     &psyData[ch].sfbThreshold,
                     &psyData[ch].sfbEnergy,
                     &psyData[ch].sfbSpreadedEnergy,
                     psyData[ch].sfbEnergySum,
                     psyData[ch].sfbEnergySumMS,
                     psyData[ch].blockSwitchingControl.windowSequence,
                     blockType2windowShape[psyData[ch].blockSwitchingControl.windowSequence],
                     hPsyConfLong->sfbCnt,
                     hPsyConfLong->sfbOffset,
                     maxSfbPerGroup[ch],
                     hPsyConfLong->sfbMinSnr,
                     psyData[ch].blockSwitchingControl.noOfGroups,
                     psyData[ch].blockSwitchingControl.groupLen,
                     &psyOutChannel[ch]);
    else
      UpdateOutParameter(psyData[ch].mdctSpectrum,
                     psyData[ch].mdctScale,
                     &psyData[ch].sfbThreshold,
                     &psyData[ch].sfbEnergy,
                     &psyData[ch].sfbSpreadedEnergy,
                     psyData[ch].sfbEnergySum,
                     psyData[ch].sfbEnergySumMS,
                     SHORT_WINDOW,
                     SINE_WINDOW,
                     psyData[0].blockSwitchingControl.noOfGroups*hPsyConfShort->sfbCnt,
                     psyOutElement->groupedSfbOffset[ch],
                     maxSfbPerGroup[ch],
                     psyOutElement->groupedSfbMinSnr[ch],
                     psyData[ch].blockSwitchingControl.noOfGroups,
                     psyData[ch].blockSwitchingControl.groupLen,
                     &psyOutChannel[ch]);
  }

  return(0); /* no error */
}

/*****************************************************************************
*
* function name: Perceptual_AdvLngBlk
* description:  psychoacoustic for long blocks
*
*****************************************************************************/

static int16_t Perceptual_AdvLngBlk(GM_PSY_PART_DATA* psyData,
                               GM_TNS_PART_DATA* tnsData,
                               GM_PSY_CFG_LONGBLK *hPsyConfLong,
                               GM_PSY_OUT_CHANNEL* psyOutChannel,
                               int32_t *pScratchTns,
                               const GM_TNS_PART_DATA* tnsData2,
                               const int16_t ch)
{
  int32_t i;
  int32_t normEnergyShift = (psyData->mdctScale + 1) << 1; /* in reference code, mdct spectrum must be multipied with 2, so +1 */
  int32_t clipEnergy = hPsyConfLong->clipEnergy >> normEnergyShift;
  int32_t *data0, *data1, tdata;

  /* low pass */
  data0 = psyData->mdctSpectrum + hPsyConfLong->lowpassLine;
  for(i=hPsyConfLong->lowpassLine; i<FRAME_LEN_LONG; i++) {
    *data0++ = 0;
  }

  /* Calc sfb-bandwise mdct-energies for left and right channel */
  SfbMdct_Energy( psyData->mdctSpectrum,
                  hPsyConfLong->sfbOffset,
                  hPsyConfLong->sfbActive,
                  psyData->sfbEnergy.sfbLong,
                  &psyData->sfbEnergySum.sfbLong);

  /*
    TNS detect
  */
  Tns_CalcFiltDecide(tnsData,
            hPsyConfLong->tnsConf,
            pScratchTns,
            (const int16_t*)hPsyConfLong->sfbOffset,
            psyData->mdctSpectrum,
            0,
            psyData->blockSwitchingControl.windowSequence,
            psyData->sfbEnergy.sfbLong);

  /*  Tns_SyncParam */
  if (ch == 1) {
    Tns_SyncParam(tnsData,
            tnsData2,
            hPsyConfLong->tnsConf,
            0,
            psyData->blockSwitchingControl.windowSequence);
  }

  /*  Tns Encoder */
  Tns_Filtering(&psyOutChannel->tnsInfo,
            tnsData,
            hPsyConfLong->sfbCnt,
            hPsyConfLong->tnsConf,
            hPsyConfLong->lowpassLine,
            psyData->mdctSpectrum,
            0,
            psyData->blockSwitchingControl.windowSequence);

  /* first part of threshold calculation */
  data0 = psyData->sfbEnergy.sfbLong;
  data1 = psyData->sfbThreshold.sfbLong;
  for (i=hPsyConfLong->sfbCnt; i; i--) {
    tdata = L_mpy_ls(*data0++, hPsyConfLong->ratio);
    *data1++ = min(tdata, clipEnergy);
  }

  /* Calc sfb-bandwise mdct-energies for left and right channel again */
  if (tnsData->dataRaw.tnsLong.subBlockInfo.tnsActive!=0) {
    int16_t tnsStartBand = hPsyConfLong->tnsConf.tnsStartBand;
    SfbMdct_Energy( psyData->mdctSpectrum,
                    hPsyConfLong->sfbOffset+tnsStartBand,
                    hPsyConfLong->sfbActive - tnsStartBand,
                    psyData->sfbEnergy.sfbLong+tnsStartBand,
                    &psyData->sfbEnergySum.sfbLong);

	data0 = psyData->sfbEnergy.sfbLong;
	tdata = psyData->sfbEnergySum.sfbLong;
	for (i=0; i<tnsStartBand; i++)
      tdata += *data0++;

	psyData->sfbEnergySum.sfbLong = tdata;
  }


  /* spreading energy */
  FindMax_EngerSpreading(hPsyConfLong->sfbCnt,
               hPsyConfLong->sfbMaskLowFactor,
               hPsyConfLong->sfbMaskHighFactor,
               psyData->sfbThreshold.sfbLong);

  /* threshold in quiet */
  data0 = psyData->sfbThreshold.sfbLong;
  data1 = hPsyConfLong->sfbThresholdQuiet;
  for (i=hPsyConfLong->sfbCnt; i; i--)
  {
	  *data0 = max(*data0, (*data1 >> normEnergyShift));
	  data0++; data1++;
  }

  /* preecho control */
  if (psyData->blockSwitchingControl.windowSequence == STOP_WINDOW) {
    data0 = psyData->sfbThresholdnm1;
	for (i=hPsyConfLong->sfbCnt; i; i--) {
      *data0++ = MAX_32;
    }
    psyData->mdctScalenm1 = 0;
  }

  PreEchoControl( psyData->sfbThresholdnm1,
                  hPsyConfLong->sfbCnt,
                  hPsyConfLong->maxAllowedIncreaseFactor,
                  hPsyConfLong->minRemainingThresholdFactor,
                  psyData->sfbThreshold.sfbLong,
                  psyData->mdctScale,
                  psyData->mdctScalenm1);
  psyData->mdctScalenm1 = psyData->mdctScale;


  if (psyData->blockSwitchingControl.windowSequence== START_WINDOW) {
    data0 = psyData->sfbThresholdnm1;
	for (i=hPsyConfLong->sfbCnt; i; i--) {
      *data0++ = MAX_32;
    }
    psyData->mdctScalenm1 = 0;
  }

  /* apply tns mult table on cb thresholds */
  Tns_ApplyMultTableToRatios(hPsyConfLong->tnsConf.tnsRatioPatchLowestCb,
                            hPsyConfLong->tnsConf.tnsStartBand,
                            tnsData->dataRaw.tnsLong.subBlockInfo,
                            psyData->sfbThreshold.sfbLong);


  /* spreaded energy */
  data0 = psyData->sfbSpreadedEnergy.sfbLong;
  data1 = psyData->sfbEnergy.sfbLong;
  for (i=hPsyConfLong->sfbCnt; i; i--) {
    //psyData->sfbSpreadedEnergy.sfbLong[i] = psyData->sfbEnergy.sfbLong[i];
	  *data0++ = *data1++;
  }

  /* spreading energy */
  FindMax_EngerSpreading(hPsyConfLong->sfbCnt,
               hPsyConfLong->sfbMaskLowFactorSprEn,
               hPsyConfLong->sfbMaskHighFactorSprEn,
               psyData->sfbSpreadedEnergy.sfbLong);

  return 0;
}

/*****************************************************************************
*
* function name: Perceptual_AdvLngBlkMS
* description:   update mdct-energies for left add or minus right channel
*				for long block
*
*****************************************************************************/
static int16_t Perceptual_AdvLngBlkMS (GM_PSY_PART_DATA psyData[MAX_CHAN],
                                  const GM_PSY_CFG_LONGBLK *hPsyConfLong)
{
  SfbMdct_MSEnergy(psyData[0].mdctSpectrum,
                   psyData[1].mdctSpectrum,
                   hPsyConfLong->sfbOffset,
                   hPsyConfLong->sfbActive,
                   psyData[0].sfbEnergyMS.sfbLong,
                   &psyData[0].sfbEnergySumMS.sfbLong,
                   psyData[1].sfbEnergyMS.sfbLong,
                   &psyData[1].sfbEnergySumMS.sfbLong);

  return 0;
}


/*****************************************************************************
*
* function name: Perceptual_AdvShortBlk
* description:  psychoacoustic for short blocks
*
*****************************************************************************/

static int16_t Perceptual_AdvShortBlk(GM_PSY_PART_DATA* psyData,
                                GM_TNS_PART_DATA* tnsData,
                                const GM_PSY_CFG_SHORTBLK *hPsyConfShort,
                                GM_PSY_OUT_CHANNEL* psyOutChannel,
                                int32_t *pScratchTns,
                                const GM_TNS_PART_DATA *tnsData2,
                                const int16_t ch)
{
  int32_t w;
  int32_t normEnergyShift = (psyData->mdctScale + 1) << 1; /* in reference code, mdct spectrum must be multipied with 2, so +1 */
  int32_t clipEnergy = hPsyConfShort->clipEnergy >> normEnergyShift;
  int32_t wOffset = 0;
  int32_t *data0;
  const int32_t *data1;

  for(w = 0; w < TRANS_FAC; w++) {
    int32_t i, tdata;

    /* low pass */
    data0 = psyData->mdctSpectrum + wOffset + hPsyConfShort->lowpassLine;
	for(i=hPsyConfShort->lowpassLine; i<FRAME_LEN_SHORT; i++){
      *data0++ = 0;
    }

    /* Calc sfb-bandwise mdct-energies for left and right channel */
    SfbMdct_Energy( psyData->mdctSpectrum+wOffset,
                    hPsyConfShort->sfbOffset,
                    hPsyConfShort->sfbActive,
                    psyData->sfbEnergy.sfbShort[w],
                    &psyData->sfbEnergySum.sfbShort[w]);
    /*
       TNS
    */
    Tns_CalcFiltDecide(tnsData,
              hPsyConfShort->tnsConf,
              pScratchTns,
              (const int16_t*)hPsyConfShort->sfbOffset,
              psyData->mdctSpectrum+wOffset,
              w,
              psyData->blockSwitchingControl.windowSequence,
              psyData->sfbEnergy.sfbShort[w]);

    /*  Tns_SyncParam */
    if (ch == 1) {
      Tns_SyncParam(tnsData,
              tnsData2,
              hPsyConfShort->tnsConf,
              w,
              psyData->blockSwitchingControl.windowSequence);
    }

    Tns_Filtering(&psyOutChannel->tnsInfo,
              tnsData,
              hPsyConfShort->sfbCnt,
              hPsyConfShort->tnsConf,
              hPsyConfShort->lowpassLine,
              psyData->mdctSpectrum+wOffset,
              w,
              psyData->blockSwitchingControl.windowSequence);

    /* first part of threshold calculation */
    data0 = psyData->sfbThreshold.sfbShort[w];
	data1 = psyData->sfbEnergy.sfbShort[w];
	for (i=hPsyConfShort->sfbCnt; i; i--) {
      tdata = L_mpy_ls(*data1++, hPsyConfShort->ratio);
      *data0++ = min(tdata, clipEnergy);
    }

    /* Calc sfb-bandwise mdct-energies for left and right channel again */
    if (tnsData->dataRaw.tnsShort.subBlockInfo[w].tnsActive != 0) {
      int16_t tnsStartBand = hPsyConfShort->tnsConf.tnsStartBand;
      SfbMdct_Energy( psyData->mdctSpectrum+wOffset,
                      hPsyConfShort->sfbOffset+tnsStartBand,
                      (hPsyConfShort->sfbActive - tnsStartBand),
                      psyData->sfbEnergy.sfbShort[w]+tnsStartBand,
                      &psyData->sfbEnergySum.sfbShort[w]);

      tdata = psyData->sfbEnergySum.sfbShort[w];
	  data0 = psyData->sfbEnergy.sfbShort[w];
	  for (i=tnsStartBand; i; i--)
        tdata += *data0++;

	  psyData->sfbEnergySum.sfbShort[w] = tdata;
    }

    /* spreading */
    FindMax_EngerSpreading(hPsyConfShort->sfbCnt,
                 hPsyConfShort->sfbMaskLowFactor,
                 hPsyConfShort->sfbMaskHighFactor,
                 psyData->sfbThreshold.sfbShort[w]);


    /* threshold in quiet */
    data0 = psyData->sfbThreshold.sfbShort[w];
	data1 = hPsyConfShort->sfbThresholdQuiet;
	for (i=hPsyConfShort->sfbCnt; i; i--)
    {
		*data0 = max(*data0, (*data1 >> normEnergyShift));

		data0++; data1++;
	}


    /* preecho */
    PreEchoControl( psyData->sfbThresholdnm1,
                    hPsyConfShort->sfbCnt,
                    hPsyConfShort->maxAllowedIncreaseFactor,
                    hPsyConfShort->minRemainingThresholdFactor,
                    psyData->sfbThreshold.sfbShort[w],
                    psyData->mdctScale,
                    w==0 ? psyData->mdctScalenm1 : psyData->mdctScale);

    /* apply tns mult table on cb thresholds */
    Tns_ApplyMultTableToRatios( hPsyConfShort->tnsConf.tnsRatioPatchLowestCb,
                               hPsyConfShort->tnsConf.tnsStartBand,
                               tnsData->dataRaw.tnsShort.subBlockInfo[w],
                               psyData->sfbThreshold.sfbShort[w]);

    /* spreaded energy */
    data0 = psyData->sfbSpreadedEnergy.sfbShort[w];
	data1 = psyData->sfbEnergy.sfbShort[w];
	for (i=hPsyConfShort->sfbCnt; i; i--) {
	  *data0++ = *data1++;
    }
    FindMax_EngerSpreading(hPsyConfShort->sfbCnt,
                 hPsyConfShort->sfbMaskLowFactorSprEn,
                 hPsyConfShort->sfbMaskHighFactorSprEn,
                 psyData->sfbSpreadedEnergy.sfbShort[w]);

    wOffset += FRAME_LEN_SHORT;
  } /* for TRANS_FAC */

  psyData->mdctScalenm1 = psyData->mdctScale;

  return 0;
}

/*****************************************************************************
*
* function name: Perceptual_AdvShortBlkMS
* description:   update mdct-energies for left add or minus right channel
*				for short block
*
*****************************************************************************/
static int16_t Perceptual_AdvShortBlkMS (GM_PSY_PART_DATA psyData[MAX_CHAN],
                                   const GM_PSY_CFG_SHORTBLK *hPsyConfShort)
{
  int32_t w, wOffset;
  wOffset = 0;
  for(w=0; w<TRANS_FAC; w++) {
    SfbMdct_MSEnergy(psyData[0].mdctSpectrum+wOffset,
                     psyData[1].mdctSpectrum+wOffset,
                     hPsyConfShort->sfbOffset,
                     hPsyConfShort->sfbActive,
                     psyData[0].sfbEnergyMS.sfbShort[w],
                     &psyData[0].sfbEnergySumMS.sfbShort[w],
                     psyData[1].sfbEnergyMS.sfbShort[w],
                     &psyData[1].sfbEnergySumMS.sfbShort[w]);
    wOffset += FRAME_LEN_SHORT;
  }

  return 0;
}




/*****************************************************************************
*
* function name: UpdateOutParameter
* description:  update output parameter
*
**********************************************************************************/
void UpdateOutParameter(int32_t                  *groupedMdctSpectrum,
                    const int16_t             mdctScale,
                    GM_SFB_THRSHD           *groupedSfbThreshold,
                    GM_SFB_ENGY              *groupedSfbEnergy,
                    GM_SFB_ENGY              *groupedSfbSpreadedEnergy,
                    const GM_SFB_ENGY_SUM     sfbEnergySumLR,
                    const GM_SFB_ENGY_SUM     sfbEnergySumMS,
                    const int16_t             windowSequence,
                    const int16_t             windowShape,
                    const int16_t             groupedSfbCnt,
                    const int16_t            *groupedSfbOffset,
                    const int16_t             maxSfbPerGroup,
                    const int16_t            *groupedSfbMinSnr,
                    const int16_t             noOfGroups,
                    const int16_t            *groupLen,
                    GM_PSY_OUT_CHANNEL         *psyOutCh)
{
  int32_t j;
  int32_t grp;
  int32_t mask;
  int16_t *tmpV;

  /*
  copy values to psyOut
  */
  psyOutCh->maxSfbPerGroup    = maxSfbPerGroup;
  psyOutCh->sfbCnt            = groupedSfbCnt;
  if(noOfGroups)
	psyOutCh->sfbPerGroup     = groupedSfbCnt/ noOfGroups;
  else
	psyOutCh->sfbPerGroup     = 0x7fff;
  psyOutCh->windowSequence    = windowSequence;
  psyOutCh->windowShape       = windowShape;
  psyOutCh->mdctScale         = mdctScale;
  psyOutCh->mdctSpectrum      = groupedMdctSpectrum;
  psyOutCh->sfbEnergy         = groupedSfbEnergy->sfbLong;
  psyOutCh->sfbThreshold      = groupedSfbThreshold->sfbLong;
  psyOutCh->sfbSpreadedEnergy = groupedSfbSpreadedEnergy->sfbLong;

  tmpV = psyOutCh->sfbOffsets;
  for(j=0; j<groupedSfbCnt + 1; j++) {
      *tmpV++ = groupedSfbOffset[j];
  }

  tmpV = psyOutCh->sfbMinSnr;
  for(j=0;j<groupedSfbCnt; j++) {
	  *tmpV++ =   groupedSfbMinSnr[j];
  }

  /* generate grouping mask */
  mask = 0;
  for (grp = 0; grp < noOfGroups; grp++) {
    mask = mask << 1;
    for (j=1; j<groupLen[grp]; j++) {
      mask = mask << 1;
      mask |= 1;
    }
  }
  psyOutCh->groupingMask = mask;

  if (windowSequence != SHORT_WINDOW) {
    psyOutCh->sfbEnSumLR =  sfbEnergySumLR.sfbLong;
    psyOutCh->sfbEnSumMS =  sfbEnergySumMS.sfbLong;
  }
  else {
    int32_t i;
    int32_t accuSumMS=0;
    int32_t accuSumLR=0;
	const int32_t *pSumMS = sfbEnergySumMS.sfbShort;
	const int32_t *pSumLR = sfbEnergySumLR.sfbShort;

    for (i=TRANS_FAC; i; i--) {
      accuSumLR = L_add(accuSumLR, *pSumLR); pSumLR++;
      accuSumMS = L_add(accuSumMS, *pSumMS); pSumMS++;
    }
    psyOutCh->sfbEnSumMS = accuSumMS;
    psyOutCh->sfbEnSumLR = accuSumLR;
  }
}


/********************************************************************************
*
* function name: MidSideStereo
* description:  detect use ms stereo or not
*				if ((min(thrLn, thrRn)*min(thrLn, thrRn))/(enMn*enSn))
*				>= ((thrLn *thrRn)/(enLn*enRn)) then ms stereo
*
**********************************************************************************/
void MidSideStereo(int32_t       *sfbEnergyLeft,
                        int32_t       *sfbEnergyRight,
                        const int32_t *sfbEnergyMid,
                        const int32_t *sfbEnergySide,
                        int32_t       *mdctSpectrumLeft,
                        int32_t       *mdctSpectrumRight,
                        int32_t       *sfbThresholdLeft,
                        int32_t       *sfbThresholdRight,
                        int32_t       *sfbSpreadedEnLeft,
                        int32_t       *sfbSpreadedEnRight,
                        int16_t       *msDigest,
                        int16_t       *msMask,
                        const int16_t  sfbCnt,
                        const int16_t  sfbPerGroup,
                        const int16_t  maxSfbPerGroup,
                        const int16_t *sfbOffset) {
  //int32_t temp;
  int32_t sfb,sfboffs, j;
  int32_t msMaskTrueSomewhere = 0;
  int32_t msMaskFalseSomewhere = 0;

  for (sfb=0; sfb<sfbCnt; sfb+=sfbPerGroup) {
    for (sfboffs=0;sfboffs<maxSfbPerGroup;sfboffs++) {

      int32_t temp;
      int32_t pnlr,pnms;
      int32_t minThreshold;
      int32_t thrL, thrR, nrgL, nrgR;
      int32_t idx, shift;

      idx = sfb + sfboffs;

      thrL = sfbThresholdLeft[idx];
      thrR = sfbThresholdRight[idx];
      nrgL = sfbEnergyLeft[idx];
      nrgR = sfbEnergyRight[idx];

      minThreshold = min(thrL, thrR);

      nrgL = max(nrgL,thrL) + 1;
      shift = norm_l(nrgL);
	  nrgL = Div_32(thrL << shift, nrgL << shift);
      nrgR = max(nrgR,thrR) + 1;
      shift = norm_l(nrgR);
	  nrgR = Div_32(thrR << shift, nrgR << shift);

	  pnlr = fixmul(nrgL, nrgR);

      nrgL = sfbEnergyMid[idx];
      nrgR = sfbEnergySide[idx];

      nrgL = max(nrgL,minThreshold) + 1;
      shift = norm_l(nrgL);
	  nrgL = Div_32(minThreshold << shift, nrgL << shift);

      nrgR = max(nrgR,minThreshold) + 1;
      shift = norm_l(nrgR);
	  nrgR = Div_32(minThreshold << shift, nrgR << shift);

      pnms = fixmul(nrgL, nrgR);

      temp = (pnlr + 1) / ((pnms >> 8) + 1);

      temp = pnms - pnlr;
      if( temp > 0 ){

        msMask[idx] = 1;
        msMaskTrueSomewhere = 1;

        for (j=sfbOffset[idx]; j<sfbOffset[idx+1]; j++) {
          int32_t left, right;
          left  = (mdctSpectrumLeft[j] >>  1);
          right = (mdctSpectrumRight[j] >> 1);
          mdctSpectrumLeft[j] =  left + right;
          mdctSpectrumRight[j] =  left - right;
        }

        sfbThresholdLeft[idx] = minThreshold;
        sfbThresholdRight[idx] = minThreshold;
        sfbEnergyLeft[idx] = sfbEnergyMid[idx];
        sfbEnergyRight[idx] = sfbEnergySide[idx];

        sfbSpreadedEnRight[idx] = min(sfbSpreadedEnLeft[idx],sfbSpreadedEnRight[idx]) >> 1;
        sfbSpreadedEnLeft[idx] = sfbSpreadedEnRight[idx];

      }
      else {
        msMask[idx]  = 0;
        msMaskFalseSomewhere = 1;
      }
    }
    if ( msMaskTrueSomewhere ) {
      if(msMaskFalseSomewhere ) {
        *msDigest = SI_MS_MASK_SOME;
      } else {
        *msDigest = SI_MS_MASK_ALL;
      }
    } else {
      *msDigest = SI_MS_MASK_NONE;
    }
  }

}


/*****************************************************************************
*
* function name: ShortBlkGroup
* description:  group short data for next quantization and coding
*
**********************************************************************************/
void
ShortBlkGroup(int32_t        *mdctSpectrum,
               int32_t        *tmpSpectrum,
               GM_SFB_THRSHD *sfbThreshold,
               GM_SFB_ENGY    *sfbEnergy,
               GM_SFB_ENGY    *sfbEnergyMS,
               GM_SFB_ENGY    *sfbSpreadedEnergy,
               const int16_t   sfbCnt,
               const int16_t  *sfbOffset,
               const int16_t  *sfbMinSnr,
               int16_t        *groupedSfbOffset,
               int16_t        *maxSfbPerGroup,
               int16_t        *groupedSfbMinSnr,
               const int16_t   noOfGroups,
               const int16_t  *groupLen)
{
  int32_t i, j;
  int32_t line;
  int32_t sfb;
  int32_t grp;
  int32_t wnd;
  int32_t offset;
  int32_t highestSfb, tmp;
  int32_t *tmpSpectrum_p, *mdctSpectrum_p;

  /* for short: regroup and  */
  /* cumulate energies und thresholds group-wise . */

  /* calculate sfbCnt */
  highestSfb = 0;
  for (wnd=0; wnd<TRANS_FAC; wnd++) {
    for (sfb=sfbCnt - 1; sfb>=highestSfb; sfb--) {
      for (line=(sfbOffset[sfb + 1] - 1); line>=sfbOffset[sfb]; line--) {

        if (mdctSpectrum[wnd*FRAME_LEN_SHORT+line] != 0) break;
      }

      if (line >= sfbOffset[sfb]) break;
    }
    highestSfb = max(highestSfb, sfb);
  }

  if (highestSfb < 0) {
    highestSfb = 0;
  }
  *maxSfbPerGroup = highestSfb + 1;

  /* calculate sfbOffset */
  i = 0;
  offset = 0;
  for (grp = 0; grp < noOfGroups; grp++) {
    for (sfb = 0; sfb < sfbCnt; sfb++) {
      groupedSfbOffset[i] = offset + sfbOffset[sfb] * groupLen[grp];
      i += 1;
    }
    offset += groupLen[grp] * FRAME_LEN_SHORT;
  }
  groupedSfbOffset[i] = FRAME_LEN_LONG;
  i += 1;

  /* calculate minSnr */
  i = 0;
  offset = 0;
  for (grp = 0; grp < noOfGroups; grp++) {
    for (sfb = 0; sfb < sfbCnt; sfb++) {
      groupedSfbMinSnr[i] = sfbMinSnr[sfb];
      i += 1;
    }
    offset += groupLen[grp] * FRAME_LEN_SHORT;
  }


  /* sum up sfbThresholds */
  wnd = 0;
  i = 0;
  tmp = 0x80000000;
  for (grp = 0; grp < noOfGroups; grp++) {
    for (sfb = 0; sfb < sfbCnt; sfb++) {
      int32_t thresh = sfbThreshold->sfbShort[wnd][sfb];
      for (j=1; j<groupLen[grp]; j++) {
//        thresh = L_add(thresh, sfbThreshold->sfbShort[wnd+j][sfb]);
#ifdef LINUX
      	int tmpp;
				tmpp = sfbThreshold->sfbShort[wnd+j][sfb];
				asm volatile("qadd %[o1], %[o1], %[i1]" : [o1] "+r" (thresh) : [i1] "r" (tmpp));
#else
	   	__asm{
			//ADDS	thresh, thresh, sfbThreshold->sfbShort[wnd+j][sfb]
			//EORVS	thresh, tmp, thresh, ASR#31
			QADD  thresh, thresh, sfbThreshold->sfbShort[wnd+j][sfb]
		}
#endif
      }
      sfbThreshold->sfbLong[i] = thresh;
      i += 1;
    }
    wnd += groupLen[grp];
  }

  /* sum up sfbEnergies left/right */
  wnd = 0;
  i = 0;
  for (grp = 0; grp < noOfGroups; grp++) {
    for (sfb = 0; sfb < sfbCnt; sfb++) {
      int32_t energy = sfbEnergy->sfbShort[wnd][sfb];
      for (j=1; j<groupLen[grp]; j++) {
//        energy = L_add(energy, sfbEnergy->sfbShort[wnd+j][sfb]);
#ifdef LINUX
      	int tmpp;
				tmpp = sfbEnergy->sfbShort[wnd+j][sfb];
				asm volatile("qadd %[o1], %[o1], %[i1]" : [o1] "+r" (energy) : [i1] "r" (tmpp));
#else
	   	__asm{
			//ADDS	energy, energy, sfbEnergy->sfbShort[wnd+j][sfb]
			//EORVS	energy, tmp, energy, ASR#31
			QADD  energy, energy, sfbEnergy->sfbShort[wnd+j][sfb]
		}
#endif
      }
      sfbEnergy->sfbLong[i] = energy;
      i += 1;
    }
    wnd += groupLen[grp];
  }

  /* sum up sfbEnergies mid/side */
  wnd = 0;
  i = 0;
  for (grp = 0; grp < noOfGroups; grp++) {
    for (sfb = 0; sfb < sfbCnt; sfb++) {
      int32_t energy = sfbEnergyMS->sfbShort[wnd][sfb];
      for (j=1; j<groupLen[grp]; j++) {
//        energy = L_add(energy, sfbEnergyMS->sfbShort[wnd+j][sfb]);
#ifdef LINUX
      	int tmpp;
				tmpp = sfbEnergyMS->sfbShort[wnd+j][sfb];
				asm volatile("qadd %[o1], %[o1], %[i1]" : [o1] "+r" (energy) : [i1] "r" (tmpp));
#else
	   	__asm{
			//ADDS	energy, energy, sfbEnergyMS->sfbShort[wnd+j][sfb]
			//EORVS	energy, tmp, energy, ASR#31
			QADD  energy, energy, sfbEnergyMS->sfbShort[wnd+j][sfb]
		}
#endif
      }
      sfbEnergyMS->sfbLong[i] = energy;
      i += 1;
    }
    wnd += groupLen[grp];
  }

  /* sum up sfbSpreadedEnergies */
  wnd = 0;
  i = 0;
  for (grp = 0; grp < noOfGroups; grp++) {
    for (sfb = 0; sfb < sfbCnt; sfb++) {
      int32_t energy = sfbSpreadedEnergy->sfbShort[wnd][sfb];
      for (j=1; j<groupLen[grp]; j++) {
//        energy = L_add(energy, sfbSpreadedEnergy->sfbShort[wnd+j][sfb]);
#ifdef LINUX
      	int tmpp;
				tmpp = sfbSpreadedEnergy->sfbShort[wnd+j][sfb];
				asm volatile("qadd %[o1], %[o1], %[i1]" : [o1] "+r" (energy) : [i1] "r" (tmpp));
#else
	   	__asm{
			//ADDS	energy, energy, sfbSpreadedEnergy->sfbShort[wnd+j][sfb]
			//EORVS	energy, tmp, energy, ASR#31
			QADD  energy, energy, sfbSpreadedEnergy->sfbShort[wnd+j][sfb]
		}
#endif
      }
      sfbSpreadedEnergy->sfbLong[i] = energy;
      i += 1;
    }
    wnd += groupLen[grp];
  }

  /* re-group spectrum */
  wnd = 0;
  i = 0;
  for (grp = 0; grp < noOfGroups; grp++) {
    for (sfb = 0; sfb < sfbCnt; sfb++) {
      for (j = 0; j < groupLen[grp]; j++) {
        int16_t lineOffset = FRAME_LEN_SHORT * (wnd + j);
        for (line = lineOffset + sfbOffset[sfb]; line < lineOffset + sfbOffset[sfb+1]; line++) {
          tmpSpectrum[i] = mdctSpectrum[line];
          i = i + 1;
        }
      }
    }
    wnd += groupLen[grp];
  }
  mdctSpectrum_p = mdctSpectrum;
  tmpSpectrum_p = tmpSpectrum;
  i = FRAME_LEN_LONG/4;
//  for(i=0;i<FRAME_LEN_LONG;i+=4) {
  do {
    *mdctSpectrum_p++ = *tmpSpectrum_p++;
    *mdctSpectrum_p++ = *tmpSpectrum_p++;
    *mdctSpectrum_p++ = *tmpSpectrum_p++;
    *mdctSpectrum_p++ = *tmpSpectrum_p++;
  } while (--i!=0);
}




/*******************************************************************************
	Content:	Band/Line energy calculations functions
*******************************************************************************/

#ifndef ARMV5E
/********************************************************************************
*
* function name: SfbMdct_Energy
* description:   Calc sfb-bandwise mdct-energies for left and right channel
*
**********************************************************************************/
void SfbMdct_Energy(const int32_t *mdctSpectrum,
                    const int16_t *bandOffset,
                    const int16_t  numBands,
                    int32_t       *bandEnergy,
                    int32_t       *bandEnergySum)
{
#ifdef LINUX
  int32_t i, j;
  int32_t accuSum = 0;
  int32_t acc1 = 0, acc2 = 0;

  for (i=0; i<numBands; i++) {
    int32_t accu = 0;
    for (j=bandOffset[i]; j<bandOffset[i+1]; j++) {
    	int tmpp;
				tmpp = mdctSpectrum[j];
				asm volatile("smull %[o1], %[o2], %[i1], %[i1]" : [o1] "+r" (acc1), [o2] "+r" (acc2) : [i1] "r" (tmpp));
				asm volatile("qadd %[o1], %[o1], %[i1]" : [o1] "+r" (accu) : [i1] "r" (acc2));
		}
		asm volatile("qadd %[o1], %[o1], %[o1]" : [o1] "+r" (accu));
		asm volatile("qadd %[o1], %[o1], %[i1]" : [o1] "+r" (accuSum) : [i1] "r" (accu));
    bandEnergy[i] = accu;
  }
#else
  int32_t i, j;
  int32_t accuSum = 0;
  int32_t acc1, acc2, tmp;

  tmp = 0x80000000;
  for (i=0; i<numBands; i++) {
    int32_t accu = 0;
    for (j=bandOffset[i]; j<bandOffset[i+1]; j++) {
      __asm {
      	SMULL	acc1, acc2, mdctSpectrum[j], mdctSpectrum[j]
   		//ADDS	accu, accu, acc2
		//EORVS	accu, tmp, accu, ASR#31
		 QADD   accu, accu, acc2
	  }
//      accu = L_add(accu, acc2);
//      accu = L_add(accu, MULHIGH(mdctSpectrum[j], mdctSpectrum[j]));
	}
	__asm {
   		//ADDS	accu, accu, accu
		//EORVS	accu, tmp, accu, ASR#31
   		//ADDS	accuSum, accuSum, accu
		//EORVS	accuSum, tmp, accuSum, ASR#31
		  QADD   accu, accu, accu
		  QADD   accuSum, accuSum, accu
	}
//	accu = L_add(accu, accu);
//    accuSum = L_add(accuSum, accu);
    bandEnergy[i] = accu;
  }
#endif
  *bandEnergySum = accuSum;
}

/********************************************************************************
*
* function name: SfbMdct_MSEnergy
* description:   Calc sfb-bandwise mdct-energies for left add or minus right channel
*
**********************************************************************************/
void SfbMdct_MSEnergy(const int32_t *mdctSpectrumLeft,
                      const int32_t *mdctSpectrumRight,
                      const int16_t *bandOffset,
                      const int16_t  numBands,
                      int32_t       *bandEnergyMid,
                      int32_t       *bandEnergyMidSum,
                      int32_t       *bandEnergySide,
                      int32_t       *bandEnergySideSum)
{
#ifdef LINUX
  int32_t i, j;
  int32_t accuMidSum = 0;
  int32_t accuSideSum = 0;
  int32_t acc0 = 0, acc1 = 0;

  for(i=0; i<numBands; i++) {
    int32_t accuMid = 0;
    int32_t accuSide = 0;
    for (j=bandOffset[i]; j<bandOffset[i+1]; j++) {
      int32_t specm, specs;
      int32_t l, r;

      l = mdctSpectrumLeft[j] >> 1;
      r = mdctSpectrumRight[j] >> 1;
      specm = l + r;
      specs = l - r;
			asm volatile("smull %[o1], %[o2], %[i1], %[i1]" : [o1] "+r" (acc0), [o2] "+r" (acc1) : [i1] "r" (specm));
			asm volatile("qadd  %[o1], %[o1], %[i1]" : [o1] "+r" (accuMid) : [i1] "r" (acc1));
			asm volatile("smull %[o1], %[o2], %[i1], %[i1]" : [o1] "+r" (acc0), [o2] "+r" (acc1) : [i1] "r" (specs));
			asm volatile("qadd  %[o1], %[o1], %[i1]" : [o1] "+r" (accuSide) : [i1] "r" (acc1));
    }

		asm volatile("qadd %[o1], %[o1], %[o1]" : [o1] "+r" (accuMid));
		asm volatile("qadd %[o1], %[o1], %[o1]" : [o1] "+r" (accuSide));
		bandEnergyMid[i] = accuMid;
		asm volatile("qadd %[o1], %[o1], %[i1]" : [o1] "+r" (accuMidSum) : [i1] "r" (accuMid));
    bandEnergySide[i] = accuSide;
		asm volatile("qadd %[o1], %[o1], %[i1]" : [o1] "+r" (accuSideSum) : [i1] "r" (accuSide));
  }
#else
  int32_t i, j;
  int32_t accuMidSum = 0;
  int32_t accuSideSum = 0;
  int32_t acc0, acc1;
  int32_t tmp;

  tmp = 0x80000000;
  for(i=0; i<numBands; i++) {
    int32_t accuMid = 0;
    int32_t accuSide = 0;
    for (j=bandOffset[i]; j<bandOffset[i+1]; j++) {
      int32_t specm, specs;
      int32_t l, r;

      l = mdctSpectrumLeft[j] >> 1;
      r = mdctSpectrumRight[j] >> 1;
      specm = l + r;
      specs = l - r;
      __asm {
      	SMULL	acc0, acc1, specm, specm
      }
//      accuMid = L_add(accuMid, acc1);
      __asm {
   		//ADDS	accuMid, accuMid, acc1
		//EORVS	accuMid, tmp, accuMid, ASR#31
		QADD   accuMid, accuMid, acc1
	  }
      __asm {
      	SMULL	acc0, acc1, specs, specs
      }
//      accuSide = L_add(accuSide, acc1);
      __asm {
   		//ADDS	accuSide, accuSide, acc1
		//EORVS	accuSide, tmp, accuSide, ASR#31
		QADD   accuSide, accuSide, acc1
	  }
    }

//	accuMid = L_add(accuMid, accuMid);
//	accuSide = L_add(accuSide, accuSide);
    __asm {
	  //ADDS	accuMid, accuMid, accuMid
	  //EORVS	accuMid, tmp, accuMid, ASR#31
	  QADD  accuMid, accuMid, accuMid
	  //ADDS	accuSide, accuSide, accuSide
	  //EORVS	accuSide, tmp, accuSide, ASR#31
	  QADD  accuSide, accuSide, accuSide
    }

	bandEnergyMid[i] = accuMid;
//    accuMidSum = L_add(accuMidSum, accuMid);
    __asm {
  	  //ADDS	accuMidSum, accuMidSum, accuMid
	  //EORVS	accuMidSum, tmp, accuMidSum, ASR#31
	  QADD  accuMidSum, accuMidSum, accuMid
	}
    bandEnergySide[i] = accuSide;
//    accuSideSum = L_add(accuSideSum, accuSide);
    __asm {
  	  //ADDS	accuSideSum, accuSideSum, accuSide
	  //EORVS	accuSideSum, tmp, accuSideSum, ASR#31
	  QADD  accuSideSum, accuSideSum, accuSide
	}
  }
#endif
  *bandEnergyMidSum = accuMidSum;
  *bandEnergySideSum = accuSideSum;
}

#endif




/*********************************************************************************
*
* function name: FindMax_EngerSpreading
* description:  spreading the energy
*				 higher frequencies thr(n) = max(thr(n), sh(n)*thr(n-1))
*				 lower frequencies  thr(n) = max(thr(n), sl(n)*thr(n+1))
*
**********************************************************************************/
void FindMax_EngerSpreading(const int16_t pbCnt,
                  const int16_t *maskLowFactor,
                  const int16_t *maskHighFactor,
                  int32_t       *pbSpreadedEnergy)
{
  int32_t i;

  /* slope to higher frequencies */
  for (i=1; i<pbCnt; i++) {
    pbSpreadedEnergy[i] = max(pbSpreadedEnergy[i],
                                L_mpy_ls(pbSpreadedEnergy[i-1], maskHighFactor[i]));
  }
  /* slope to lower frequencies */
  for (i=pbCnt - 2; i>=0; i--) {
    pbSpreadedEnergy[i] = max(pbSpreadedEnergy[i],
                                L_mpy_ls(pbSpreadedEnergy[i+1], maskLowFactor[i]));
  }
}
