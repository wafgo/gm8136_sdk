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
	File:		quantizer_main.c

	Content:	Quantizing & coding functions

*******************************************************************************/

#include "basic_op.h"
#include "oper_32b.h"
#include "quantizer_main.h"
#include "quantizer_tool.h"
#include "perceptual_main.h"
#include "threshold_tool.h"
#include "quantizer_tool_2.h"
//#include "stat_bits.h"
#include "huffman_1.h"
#include "noiseless_code.h"
//#include "channel_map.h"
#include "mem_manag.h"


typedef enum{
  FRAME_LEN_BYTES_MODULO =  1,
  FRAME_LEN_BYTES_INT    =  2
}FRAME_LEN_RESULT_MODE;

static const int16_t maxFillElemBits = 7 + 270*8;

/* forward declarations */

static int16_t SearchMaxSpectrun(int16_t sfbCnt,
                                int16_t maxSfbPerGroup,
                                int16_t sfbPerGroup,
                                int16_t sfbOffset[MAX_GROUPED_SFB],
                                int16_t quantSpectrum[FRAME_LEN_LONG],
                                uint16_t maxValue[MAX_GROUPED_SFB]);

/*******************************************************************************
	Content:	channel mapping functions
*******************************************************************************/
int16_t Init_ChannelPartBits(GM_ELEMENT_BITS *elementBits,
                       GM_PART_INFO elInfo,
                       int32_t bitrateTot,
                       int16_t averageBitsTot,
                       int16_t staticBitsTot);


static int16_t MsMaskBitsCnt(int16_t   sfbCnt,
                              int16_t   sfbPerGroup,
                              int16_t   maxSfbPerGroup,
                              struct TOOLSINFO *toolsInfo);


/*******************************************************************************
	Content:	Static bit counter functions
*******************************************************************************/

int16_t BitCountFunc(GM_PSY_OUT_CHANNEL psyOutChannel[MAX_CHAN],
                            GM_PERCEP_OUT_PART *psyOutElement,
                            int16_t nChannels,
							int16_t adtsUsed);

static int16_t TnsBitCnt(GM_TNS_BLK_INFO *tnsInfo, int16_t blockType);

static int16_t TnsBitsDemand(GM_TNS_BLK_INFO *tnsInfo,int16_t blockType);



/*****************************************************************************
*
* function name: EestimFrmLen
* description: estimate the frame length according the bitrates
*              calcFrameLen
*****************************************************************************/
static int16_t EestimFrmLen(int32_t bitRate,
                           int32_t sampleRate,
                           FRAME_LEN_RESULT_MODE mode)
{

  int32_t result;
  int32_t quot;

  result = (FRAME_LEN_LONG >> 3) * bitRate;
  quot = result / sampleRate;


  if (mode == FRAME_LEN_BYTES_MODULO) {
    result -= quot * sampleRate;
  }
  else { /* FRAME_LEN_BYTES_INT */
    result = quot;
  }

  return result;
}

/*****************************************************************************
*
*  function name:CalcFrmPad
*  description: Calculates if padding is needed for actual frame
*  returns: paddingOn or not
*
*****************************************************************************/
static int16_t CalcFrmPad(int32_t bitRate,
                           int32_t sampleRate,
                           int32_t *paddingRest)
{
  int16_t paddingOn;
  int16_t difference;

  paddingOn = 0;

  difference = EestimFrmLen( bitRate,
                             sampleRate,
                             FRAME_LEN_BYTES_MODULO );
  *paddingRest = *paddingRest - difference;


  if (*paddingRest <= 0 ) {
    paddingOn = 1;
    *paddingRest = *paddingRest + sampleRate;
  }

  return paddingOn;
}


/*********************************************************************************
*
* function name: QC_ParamMem_Init
* description: init qcout parameter
* returns:     0 if success
*
**********************************************************************************/

int16_t QC_ParamMem_Init(GM_QC_OUTSTRM *hQC, int16_t nChannels)
{
  int32_t i;
  int16_t *quantSpec;
  int16_t *scf;
  uint16_t *maxValueInSfb;

  quantSpec = (int16_t *)Alignm_mem(nChannels * FRAME_LEN_LONG * sizeof(int16_t), 32);
  if(NULL == quantSpec)
	  return 1;
  scf = (int16_t *)Alignm_mem(nChannels * MAX_GROUPED_SFB * sizeof(int16_t), 32);
  if(NULL == scf)
  {
	  return 1;
  }
  maxValueInSfb = (uint16_t *)Alignm_mem(nChannels * MAX_GROUPED_SFB * sizeof(uint16_t), 32);
  if(NULL == maxValueInSfb)
  {
	  return 1;
  }

  for (i=0; i<nChannels; i++) {
    hQC->qcChannel[i].quantSpec = quantSpec + i*FRAME_LEN_LONG;

    hQC->qcChannel[i].maxValueInSfb = maxValueInSfb + i*MAX_GROUPED_SFB;

    hQC->qcChannel[i].scf = scf + i*MAX_GROUPED_SFB;
  }

  return 0;
}


/*********************************************************************************
*
* function name: QC_ParamMem_Del
* description: unint qcout parameter
* returns:      0 if success
*
**********************************************************************************/
void QC_ParamMem_Del(GM_QC_OUTSTRM* hQC)
{
   int32_t i;
   if(hQC)
   {
      if(hQC->qcChannel[0].quantSpec);
		 Aligmem_free(hQC->qcChannel[0].quantSpec);

      if(hQC->qcChannel[0].maxValueInSfb)
		  Aligmem_free(hQC->qcChannel[0].maxValueInSfb);

	  if(hQC->qcChannel[0].scf)
		  Aligmem_free(hQC->qcChannel[0].scf);

	  for (i=0; i<MAX_CHAN; i++) {
		  hQC->qcChannel[i].quantSpec = NULL;

		  hQC->qcChannel[i].maxValueInSfb = NULL;

		  hQC->qcChannel[i].scf = NULL;
	  }
   }
}

/*********************************************************************************
*
* function name: QC_Mem0
* description: set QC to zero
* returns:     0 if success
*
**********************************************************************************/
int16_t QC_Mem0(GM_QC_BLK *hQC)
{
  memset(hQC,0,sizeof(GM_QC_BLK));

  return (0);
}

/*********************************************************************************
*
* function name: QC_ParmSetNULL
* description: unint qcout parameter
*
**********************************************************************************/
void QC_ParmSetNULL(GM_QC_BLK *hQC)
{

  /*
     nothing to do
  */
  hQC=NULL;
}

/*********************************************************************************
*
* function name: QuantCode_Init
* description: init QD parameter
* returns:     0 if success
*
**********************************************************************************/
int16_t QuantCode_Init(GM_QC_BLK *hQC,
              struct GM_QC_INFO *init)
{
  hQC->nChannels       = init->elInfo->nChannelsInEl;
  hQC->maxBitsTot      = init->maxBits;
  hQC->bitResTot       = sub(init->bitRes, init->averageBits);
  hQC->averageBitsTot  = init->averageBits;
  hQC->maxBitFac       = init->maxBitFac;

  hQC->padding.paddingRest = init->padding.paddingRest;

  hQC->globStatBits    = 3;                          /* for ID_END */

  /* channel elements init */
  Init_ChannelPartBits(&hQC->elementBits,
                  *init->elInfo,
                  init->bitrate,
                  init->averageBits,
                  hQC->globStatBits);

  /* threshold parameter init */
  ThrParam_Init(&hQC->adjThr,
             init->meanPe,
             hQC->elementBits.chBitrate);

  return 0;
}


/*********************************************************************************
*
* function name: QC_Main
* description:  quantization and coding the spectrum
* returns:      0 if success
*
**********************************************************************************/
int16_t QC_Main(GM_QC_BLK* hQC,
              GM_ELEMENT_BITS* elBits,
              GM_ATS_PART* adjThrStateElement,
              GM_PSY_OUT_CHANNEL  psyOutChannel[MAX_CHAN],  /* may be modified in-place */
              GM_PERCEP_OUT_PART* psyOutElement,
              GM_QC_CHANNDATA_OUT  qcOutChannel[MAX_CHAN],    /* out                      */
              GM_QC_OUTSTRM_PART* qcOutElement,
              int16_t nChannels,
			  int16_t ancillaryDataBytes)
{
  int16_t maxChDynBits[MAX_CHAN];
  int16_t chBitDistribution[MAX_CHAN];
  int32_t ch;

  if (elBits->bitResLevel < 0) {
    return -1;
  }

  if (elBits->bitResLevel > elBits->maxBitResBits) {
    return -1;
  }

  qcOutElement->staticBitsUsed = BitCountFunc(psyOutChannel,
                                                      psyOutElement,
                                                      nChannels,
													  qcOutElement->adtsUsed);


  if (ancillaryDataBytes) {
    qcOutElement->ancBitsUsed = 7 + (ancillaryDataBytes << 3);

    if (ancillaryDataBytes >= 15)
      qcOutElement->ancBitsUsed = qcOutElement->ancBitsUsed + 8;
  }
  else {
    qcOutElement->ancBitsUsed = 0;
  }

  EstimFactAllChan(hQC->logSfbFormFactor, hQC->sfbNRelevantLines, hQC->logSfbEnergy, psyOutChannel, nChannels);

  /*adjust thresholds for the desired bitrate */
  Calc_Thr(&hQC->adjThr,
                   adjThrStateElement,
                   psyOutChannel,
                   psyOutElement,
                   chBitDistribution,
                   hQC->logSfbEnergy,
                   hQC->sfbNRelevantLines,
                   qcOutElement,
				   elBits,
				   nChannels,
				   hQC->maxBitFac);

  /*estimate scale factors */
  CalcScaleFactAllChan(psyOutChannel,
                       qcOutChannel,
                       hQC->logSfbEnergy,
                       hQC->logSfbFormFactor,
                       hQC->sfbNRelevantLines,
                       nChannels);

  /* condition to prevent empty bitreservoir */
  for (ch = 0; ch < nChannels; ch++) {
    int32_t maxDynBits;
    maxDynBits = elBits->averageBits + elBits->bitResLevel - 7; /* -7 bec. of align bits */
    maxDynBits = maxDynBits - qcOutElement->staticBitsUsed + qcOutElement->ancBitsUsed;
    maxChDynBits[ch] = extract_l(chBitDistribution[ch] * maxDynBits / 1000);
  }

  qcOutElement->dynBitsUsed = 0;
  for (ch = 0; ch < nChannels; ch++) {
    int32_t chDynBits;
    Flag   constraintsFulfilled;
    int32_t iter;
    iter = 0;
    do {
      constraintsFulfilled = 1;

      QuantScfBandsSpectr(psyOutChannel[ch].sfbCnt,
                       psyOutChannel[ch].maxSfbPerGroup,
                       psyOutChannel[ch].sfbPerGroup,
                       psyOutChannel[ch].sfbOffsets,
                       psyOutChannel[ch].mdctSpectrum,
                       qcOutChannel[ch].globalGain,
                       qcOutChannel[ch].scf,
                       qcOutChannel[ch].quantSpec);

      if (SearchMaxSpectrun(psyOutChannel[ch].sfbCnt,
                            psyOutChannel[ch].maxSfbPerGroup,
                            psyOutChannel[ch].sfbPerGroup,
                            psyOutChannel[ch].sfbOffsets,
                            qcOutChannel[ch].quantSpec,
                            qcOutChannel[ch].maxValueInSfb) > MAX_QUANT) {
        constraintsFulfilled = 0;
      }

      chDynBits = NoiselessCoder(qcOutChannel[ch].quantSpec,
                              qcOutChannel[ch].maxValueInSfb,
                              qcOutChannel[ch].scf,
                              psyOutChannel[ch].windowSequence,
                              psyOutChannel[ch].sfbCnt,
                              psyOutChannel[ch].maxSfbPerGroup,
                              psyOutChannel[ch].sfbPerGroup,
                              psyOutChannel[ch].sfbOffsets,
                              &qcOutChannel[ch].sectionData);

      if (chDynBits >= maxChDynBits[ch]) {
        constraintsFulfilled = 0;
      }

      if (!constraintsFulfilled) {
        qcOutChannel[ch].globalGain = qcOutChannel[ch].globalGain + 1;
      }

      iter = iter + 1;

    } while(!constraintsFulfilled);

    qcOutElement->dynBitsUsed = qcOutElement->dynBitsUsed + chDynBits;

    qcOutChannel[ch].mdctScale    = psyOutChannel[ch].mdctScale;
    qcOutChannel[ch].groupingMask = psyOutChannel[ch].groupingMask;
    qcOutChannel[ch].windowShape  = psyOutChannel[ch].windowShape;
  }

  /* save dynBitsUsed for correction of bits2pe relation */
  Save_DynBitThr(adjThrStateElement, qcOutElement->dynBitsUsed);

  {
    int16_t bitResSpace = elBits->maxBitResBits - elBits->bitResLevel;
    int16_t deltaBitRes = elBits->averageBits -
                        (qcOutElement->staticBitsUsed +
                         qcOutElement->dynBitsUsed + qcOutElement->ancBitsUsed);

    qcOutElement->fillBits = max(0, (deltaBitRes - bitResSpace));
  }

  return 0; /* OK */
}


/*********************************************************************************
*
* function name: SearchMaxSpectrun
*
* description:  search the max Spectrum in one sfb
*               calc_Max_Value_InSfb
**********************************************************************************/
static int16_t SearchMaxSpectrun(int16_t sfbCnt,
                                int16_t maxSfbPerGroup,
                                int16_t sfbPerGroup,
                                int16_t sfbOffset[MAX_GROUPED_SFB],
                                int16_t quantSpectrum[FRAME_LEN_LONG],
                                uint16_t maxValue[MAX_GROUPED_SFB])
{
  int16_t sfbOffs, sfb;
  int16_t maxValueAll;

  maxValueAll = 0;

  for(sfbOffs=0;sfbOffs<sfbCnt;sfbOffs+=sfbPerGroup) {
    for (sfb = 0; sfb < maxSfbPerGroup; sfb++) {
      int16_t line;
      int32_t maxThisSfb;
      maxThisSfb = 0;

      for (line = sfbOffset[sfbOffs+sfb]; line < sfbOffset[sfbOffs+sfb+1]; line++) {
      	int32_t absVal;
      	absVal = my_abs((int32_t)quantSpectrum[line]);
		maxThisSfb = max(maxThisSfb, absVal);
//        int16_t absVal;
//        absVal = abs_s(quantSpectrum[line]);
//        maxThisSfb = max(maxThisSfb, absVal);
      }

      maxValue[sfbOffs+sfb] = maxThisSfb;
      maxValueAll = max((int32_t)maxValueAll, maxThisSfb);
    }
  }
  return maxValueAll;
}


/*********************************************************************************
*
* function name: QC_WriteBitrese
* description: update bitreservoir
*
**********************************************************************************/
void QC_WriteBitrese(GM_QC_BLK* qcKernel,
                  GM_QC_OUTSTRM*   qcOut)

{
  GM_ELEMENT_BITS *elBits;

  qcKernel->bitResTot = 0;

  elBits = &qcKernel->elementBits;


  if (elBits->averageBits > 0) {
    /* constant bitrate */
    int16_t bitsUsed;
    bitsUsed = (qcOut->qcElement.staticBitsUsed + qcOut->qcElement.dynBitsUsed) +
                   (qcOut->qcElement.ancBitsUsed + qcOut->qcElement.fillBits);
    elBits->bitResLevel = elBits->bitResLevel + (elBits->averageBits - bitsUsed);
    qcKernel->bitResTot = qcKernel->bitResTot + elBits->bitResLevel;
  }
  else {
    /* variable bitrate */
    elBits->bitResLevel = elBits->maxBits;
    qcKernel->bitResTot = qcKernel->maxBitsTot;
  }
}

/*********************************************************************************
*
* function name: QC_WriteOutParam
*  FinalizeBitConsumption
* description: count bits used
*
**********************************************************************************/
int16_t QC_WriteOutParam(GM_QC_BLK *qcKernel,
                              GM_QC_OUTSTRM* qcOut)
{
  int32_t nFullFillElem;
  int32_t totFillBits;
  int16_t diffBits;
  int16_t bitsUsed;

  totFillBits = 0;

  qcOut->totStaticBitsUsed = qcKernel->globStatBits;
  qcOut->totStaticBitsUsed += qcOut->qcElement.staticBitsUsed;
  qcOut->totDynBitsUsed    = qcOut->qcElement.dynBitsUsed;
  qcOut->totAncBitsUsed    = qcOut->qcElement.ancBitsUsed;
  qcOut->totFillBits       = qcOut->qcElement.fillBits;

  if (qcOut->qcElement.fillBits) {
    totFillBits += qcOut->qcElement.fillBits;
  }

  nFullFillElem = (max((qcOut->totFillBits - 1), 0) / maxFillElemBits) * maxFillElemBits;

  qcOut->totFillBits = qcOut->totFillBits - nFullFillElem;

  /* check fill elements */

  if (qcOut->totFillBits > 0) {
    /* minimum Fillelement contains 7 (TAG + byte cnt) bits */
    qcOut->totFillBits = max((int32_t)7, (int32_t)qcOut->totFillBits);
    /* fill element size equals n*8 + 7 */
    qcOut->totFillBits = qcOut->totFillBits + ((8 - ((qcOut->totFillBits - 7) & 0x0007)) & 0x0007);
  }

  qcOut->totFillBits = qcOut->totFillBits + nFullFillElem;

  /* now distribute extra fillbits and alignbits over channel elements */
  qcOut->alignBits = 7 - ((qcOut->totDynBitsUsed + qcOut->totStaticBitsUsed +
                           qcOut->totAncBitsUsed + qcOut->totFillBits - 1) & 0x0007);


  if ( (qcOut->alignBits + qcOut->totFillBits - totFillBits == 8) &&
       (qcOut->totFillBits > 8))
    qcOut->totFillBits = qcOut->totFillBits - 8;


  diffBits = qcOut->alignBits + qcOut->totFillBits - totFillBits;

  if(diffBits>=0) {
    qcOut->qcElement.fillBits += diffBits;
  }

  bitsUsed = qcOut->totDynBitsUsed + qcOut->totStaticBitsUsed + qcOut->totAncBitsUsed;
  bitsUsed = bitsUsed + qcOut->totFillBits + qcOut->alignBits;

  if (bitsUsed > qcKernel->maxBitsTot) {
    return -1;
  }
  return bitsUsed;
}


/*********************************************************************************
*
* function name: AdjustBitrate
* description:  adjusts framelength via padding on a frame to frame basis,
*               to achieve a bitrate that demands a non byte aligned
*               framelength
* return:       errorcode
*
**********************************************************************************/
int16_t UpdateBitrate(GM_QC_BLK        *hQC,
                     int32_t           bitRate,    /* total bitrate */
                     int32_t           sampleRate) /* output sampling rate */
{
  int16_t paddingOn;
  int16_t frameLen;
  int16_t codeBits;
  int16_t codeBitsLast;

  /* Do we need a extra padding byte? */
  paddingOn = CalcFrmPad(bitRate,
                           sampleRate,
                           &hQC->padding.paddingRest);

  /* frame length */
  frameLen = paddingOn + EestimFrmLen(bitRate,
                                      sampleRate,
                                      FRAME_LEN_BYTES_INT);

  frameLen = frameLen << 3;
  codeBitsLast = hQC->averageBitsTot - hQC->globStatBits;
  codeBits     = frameLen - hQC->globStatBits;

  /* calculate bits for every channel element */
  if (codeBits != codeBitsLast) {
    int16_t totalBits = 0;

    hQC->elementBits.averageBits = (hQC->elementBits.relativeBits * codeBits) >> 16; /* relativeBits was scaled down by 2 */
    totalBits += hQC->elementBits.averageBits;

    hQC->elementBits.averageBits = hQC->elementBits.averageBits + (codeBits - totalBits);
  }

  hQC->averageBitsTot = frameLen;

  return 0;
}



/*******************************************************************************
	Content:	channel mapping functions
*******************************************************************************/


static const int16_t maxChannelBits = MAXBITS_COEF;

static int16_t Init_ChannelPart(GM_PART_INFO* elInfo, ELEMENT_TYPE elType)
{
  int16_t error=0;

  elInfo->elType=elType;

  switch(elInfo->elType) {

    case ID_SCE:
      elInfo->nChannelsInEl=1;

      elInfo->ChannelIndex[0]=0;

      elInfo->instanceTag=0;
      break;

    case ID_CPE:

      elInfo->nChannelsInEl=2;

      elInfo->ChannelIndex[0]=0;
      elInfo->ChannelIndex[1]=1;

      elInfo->instanceTag=0;
      break;

    default:
      error=1;
  }

  return error;
}


int16_t Init_ChannelPartInfo (int16_t nChannels, GM_PART_INFO* elInfo)
{
  int16_t error;
  error = 0;

  switch(nChannels) {

    case 1:
      Init_ChannelPart(elInfo, ID_SCE);
      break;

    case 2:
      Init_ChannelPart(elInfo, ID_CPE);
      break;

    default:
      error=4;
  }

  return error;
}


int16_t Init_ChannelPartBits(GM_ELEMENT_BITS *elementBits,
                       GM_PART_INFO elInfo,
                       int32_t bitrateTot,
                       int16_t averageBitsTot,
                       int16_t staticBitsTot)
{
  int16_t error;
  error = 0;

   switch(elInfo.nChannelsInEl) {
    case 1:
      elementBits->chBitrate = bitrateTot;
      elementBits->averageBits = averageBitsTot - staticBitsTot;
      elementBits->maxBits = maxChannelBits;

      elementBits->maxBitResBits = maxChannelBits - averageBitsTot;
      elementBits->maxBitResBits = elementBits->maxBitResBits - (elementBits->maxBitResBits & 7);
      elementBits->bitResLevel = elementBits->maxBitResBits;
      elementBits->relativeBits  = 0x4000; /* 1.0f/2 */
      break;

    case 2:
      elementBits->chBitrate   = bitrateTot >> 1;
      elementBits->averageBits = averageBitsTot - staticBitsTot;
      elementBits->maxBits     = maxChannelBits << 1;

      elementBits->maxBitResBits = (maxChannelBits << 1) - averageBitsTot;
      elementBits->maxBitResBits = elementBits->maxBitResBits - (elementBits->maxBitResBits & 7);
      elementBits->bitResLevel = elementBits->maxBitResBits;
      elementBits->relativeBits = 0x4000; /* 1.0f/2 */
      break;

    default:
      error = 1;
  }
  return error;
}


/*******************************************************************************
	Content:	Static bit counter functions

*******************************************************************************/

typedef enum {
  SI_ID_BITS                =(3),
  SI_FILL_COUNT_BITS        =(4),
  SI_FILL_ESC_COUNT_BITS    =(8),
  SI_FILL_EXTENTION_BITS    =(4),
  SI_FILL_NIBBLE_BITS       =(4),
  SI_SCE_BITS               =(4),
  SI_CPE_BITS               =(5),
  SI_CPE_MS_MASK_BITS       =(2) ,
  SI_ICS_INFO_BITS_LONG     =(1+2+1+6+1),
  SI_ICS_INFO_BITS_SHORT    =(1+2+1+4+7),
  SI_ICS_BITS               =(8+1+1+1)
} SI_BITS;


/*********************************************************************************
*
* function name: MsMaskBitsCnt
* description:   count ms stereo bits demand
*
**********************************************************************************/
static int16_t MsMaskBitsCnt(int16_t   sfbCnt,
                              int16_t   sfbPerGroup,
                              int16_t   maxSfbPerGroup,
                              struct TOOLSINFO *toolsInfo)
{
  int16_t msBits, sfbOff, sfb;
  msBits = 0;


  switch(toolsInfo->msDigest) {
    case MS_NONE:
    case MS_ALL:
      break;

    case MS_SOME:
      for(sfbOff=0; sfbOff<sfbCnt; sfbOff+=sfbPerGroup)
        for(sfb=0; sfb<maxSfbPerGroup; sfb++)
          msBits += 1;
      break;
  }
  return(msBits);
}

/*********************************************************************************
*
* function name: TnsBitCnt
* description:   count tns bit demand  core function
*
**********************************************************************************/
static int16_t TnsBitCnt(GM_TNS_BLK_INFO *tnsInfo, int16_t blockType)
{

  int32_t i, k;
  Flag tnsPresent;
  int32_t numOfWindows;
  int32_t count;
  int32_t coefBits;
  int16_t *ptcoef;

  count = 0;

  if (blockType == 2)
    numOfWindows = 8;
  else
    numOfWindows = 1;
  tnsPresent = 0;

  for (i=0; i<numOfWindows; i++) {

    if (tnsInfo->tnsActive[i]!=0) {
      tnsPresent = 1;
    }
  }

  if (tnsPresent) {
    /* there is data to be written*/
    /*count += 1; */
    for (i=0; i<numOfWindows; i++) {

      if (blockType == 2)
        count += 1;
      else
        count += 2;

      if (tnsInfo->tnsActive[i]) {
        count += 1;

        if (blockType == 2) {
          count += 4;
          count += 3;
        }
        else {
          count += 6;
          count += 5;
        }

        if (tnsInfo->order[i]) {
          count += 1; /*direction*/
          count += 1; /*coef_compression */

          if (tnsInfo->coefRes[i] == 4) {
            ptcoef = tnsInfo->coef + i*TNS_MAX_ORDER_SHORT;
			coefBits = 3;
            for(k=0; k<tnsInfo->order[i]; k++) {

              if ((ptcoef[k] > 3) || (ptcoef[k] < -4)) {
                coefBits = 4;
                break;
              }
            }
          }
          else {
            coefBits = 2;
            ptcoef = tnsInfo->coef + i*TNS_MAX_ORDER_SHORT;
			for(k=0; k<tnsInfo->order[i]; k++) {

              if ((ptcoef[k] > 1) || (ptcoef[k] < -2)) {
                coefBits = 3;
                break;
              }
            }
          }
          for (k=0; k<tnsInfo->order[i]; k++ ) {
            count += coefBits;
          }
        }
      }
    }
  }

  return count;
}

/**********************************************************************************
*
* function name: TnsBitsDemand
* description:   count tns bit demand
*
**********************************************************************************/
static int16_t TnsBitsDemand(GM_TNS_BLK_INFO *tnsInfo,int16_t blockType)
{
  return(TnsBitCnt(tnsInfo, blockType));
}

/*********************************************************************************
*
* function name: BitCountFunc
* description:   count static bit demand include tns
*
**********************************************************************************/
int16_t BitCountFunc(GM_PSY_OUT_CHANNEL psyOutChannel[MAX_CHAN],
                            GM_PERCEP_OUT_PART *psyOutElement,
                            int16_t channels,
							int16_t adtsUsed)
{
  int32_t statBits;
  int32_t ch;

  statBits = 0;

  /* if adts used, add 56 bits */
  if(adtsUsed) statBits += 56;


  switch (channels) {
    case 1:
      statBits += SI_ID_BITS+SI_SCE_BITS+SI_ICS_BITS;
      statBits += TnsBitsDemand(&(psyOutChannel[0].tnsInfo),
                               psyOutChannel[0].windowSequence);

      switch(psyOutChannel[0].windowSequence){
        case LONG_WINDOW:
        case START_WINDOW:
        case STOP_WINDOW:
          statBits += SI_ICS_INFO_BITS_LONG;
          break;
        case SHORT_WINDOW:
          statBits += SI_ICS_INFO_BITS_SHORT;
          break;
      }
      break;
    case 2:
      statBits += SI_ID_BITS+SI_CPE_BITS+2*SI_ICS_BITS;

      statBits += SI_CPE_MS_MASK_BITS;
      statBits += MsMaskBitsCnt(psyOutChannel[0].sfbCnt,
								  psyOutChannel[0].sfbPerGroup,
								  psyOutChannel[0].maxSfbPerGroup,
								  &psyOutElement->toolsInfo);

      switch (psyOutChannel[0].windowSequence) {
        case LONG_WINDOW:
        case START_WINDOW:
        case STOP_WINDOW:
          statBits += SI_ICS_INFO_BITS_LONG;
          break;
        case SHORT_WINDOW:
          statBits += SI_ICS_INFO_BITS_SHORT;
          break;
      }
      for(ch=0; ch<2; ch++)
        statBits += TnsBitsDemand(&(psyOutChannel[ch].tnsInfo),
                                 psyOutChannel[ch].windowSequence);
      break;
  }

  return statBits;
}
